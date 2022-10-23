#include <stdio.h>
#include "xil_printf.h"
#include "xil_cache.h"
#include "sevenSeg_new.h"
#include "xintc.h"
#include "xtmrctr.h"
#include "xtmrctr_l.h"
#include "xparameters.h"
#include <xbasic_types.h>
#include "xgpio.h"

#define RESET_VALUE 0x5F5E100

const u16 on_mask = 0b1111111111111111;
volatile u8 state = 3;
volatile u16 mem_led = 0b1;
u16 led_display_mask = on_mask;
u8 state_mask = 3;
short dir = 0; //0 = CCW, 1 = CW
volatile unsigned int prev_in = 0;


XTmrCtr tmr;
XGpio encoder;
XGpio rgbleds;
XGpio leds;
XIntc intc;

void timer_disable() {
	XIntc_Disable(&intc, XPAR_INTC_0_TMRCTR_0_VEC_ID);
}

void timer_enable() {
	XIntc_Enable(&intc, XPAR_INTC_0_TMRCTR_0_VEC_ID);
}

void led_left() {
	if (led_display_mask & 1) {
		if (mem_led == (1 << 15)) {
			mem_led = 0b1;
		} else {
			mem_led = mem_led << 1;
		}
		XGpio_DiscreteWrite(&leds, 1, mem_led);
	}
}

void led_right() {
	if (led_display_mask & 1) {
		if (mem_led == 1) {
			mem_led = 0b1000000000000000;
		} else {
			mem_led = mem_led >> 1;
		}
		XGpio_DiscreteWrite(&leds, 1, mem_led);
	}
}

void timer_handler() {

//	int inputs = XGpio_DiscreteRead(&encoder, 1);
//	int button_in = (inputs & 0b100) >> 2;
//
//	if (button_in == 1) {
//
//	}
//
//	prev_button_in =

	Xuint32 ControlStatusReg;
	ControlStatusReg = XTimerCtr_ReadReg(tmr.BaseAddress, 0, XTC_TCSR_OFFSET);
	XTmrCtr_WriteReg(tmr.BaseAddress, 0, XTC_TCSR_OFFSET, ControlStatusReg |XTC_CSR_INT_OCCURED_MASK);

}

void encoder_handler() {
	Xuint32 start = XTmrCtr_GetTimerCounterReg(tmr.BaseAddress, 0);
	Xuint32 finish = start;

	while (finish < (start + RESET_VALUE/10000)) {
		finish = XTmrCtr_GetTimerCounterReg(tmr.BaseAddress, 0);
	}

	int next_state = XGpio_DiscreteRead(&encoder, 1);

	if (next_state == 7) { // ABC = 001; AB = 00; C = 1 -> pushdown pressed

//		XGpio_InterruptDisable(&encoder, 1);
		if (!(led_display_mask & 1)) {
			led_display_mask = on_mask;
			XGpio_DiscreteWrite(&leds, 1, mem_led);
			for(int i = 0; i < 100000; i++);
			XGpio_InterruptClear(&encoder, 1);
		} else {
			led_display_mask = 0;
			XGpio_DiscreteWrite(&leds, 1, 0);
			for(int i = 0; i < 100000; i++);
			XGpio_InterruptClear(&encoder, 1);
		}
//		XGpio_InterruptClear(&encoder, 1);
//		XGpio_InterruptEnable(&encoder, 1);

	} else if (next_state == 0) { // ABC = 000; AB = 00; BC = 00
		state = next_state & state_mask;
	} else if (next_state == 2) { // ABC = 010; BC = 10
		if (state == 3) {
			dir = 0;
		}
		state = next_state & state_mask;
	} else if (next_state == 1) { // ABC = 100; BC = 01
		if (state == 3) {
			dir = 1;
		}
		state = next_state & state_mask;
	} else if (next_state == 3) { // ABC = 110; AB = 11; BC = 11
		if (state == 1) {
			if (dir == 0){
				led_left();
			}
		} else if (state == 2) {
			if (dir == 1){
				led_right();
			}
		}
		state = next_state & state_mask;
	} else { // undefined behavior
		//do nothing
	}
	XGpio_InterruptClear(&encoder, 1);
}


void initializePeripherals() {
	XStatus status;
	status = XST_FAILURE;

	status = XIntc_Initialize(&intc, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	if (status != XST_SUCCESS){
		xil_printf("Interrupt Controller initialization failed.\r\n");
		return XST_FAILURE;
	}

	status = XIntc_Connect(&intc, XPAR_INTC_0_TMRCTR_0_VEC_ID, (XInterruptHandler)timer_handler,&tmr);
	if (status != XST_SUCCESS){
		xil_printf("Interrupt Controller connection to AXI Timer 0 failed.\r\n");
		return XST_FAILURE;
	}

	status = XIntc_Connect(&intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_ENCODER_IP2INTC_IRPT_INTR, (XInterruptHandler)encoder_handler, &encoder);

	if (status != XST_SUCCESS){
		xil_printf("Interrupt Controller connection to GPIO button failed.\r\n");
		return XST_FAILURE;
	}

	status = XIntc_Start(&intc, XIN_REAL_MODE);

	if (status != XST_SUCCESS){
		xil_printf("Failed to start Interrupt Controller.\r\n");
		return XST_FAILURE;
	}

	timer_enable();
	XIntc_Enable(&intc, XPAR_AXI_GPIO_ENCODER_DEVICE_ID);

	status = XTmrCtr_Initialize(&tmr, XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	if (status != XST_SUCCESS){
		xil_printf("Failed to initialize Timer.\r\n");
		return XST_FAILURE;
	}
	XTmrCtr_SetOptions(&tmr, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetResetValue(&tmr, 0, 0xFFFFFFFF-RESET_VALUE);
	XTmrCtr_Start(&tmr, 0);


	microblaze_register_handler((XInterruptHandler)XIntc_DeviceInterruptHandler,(void*)XPAR_MICROBLAZE_0_AXI_INTC_DEVICE_ID);
	microblaze_register_handler((XInterruptHandler)encoder_handler, (void*)XPAR_AXI_GPIO_ENCODER_DEVICE_ID);
	microblaze_enable_interrupts();
	if (status != XST_SUCCESS) {
		xil_printf("Failed to enable interrupts.\r\n");
		return XST_FAILURE;
	}
	xil_printf("Interrupts enabled!\r\n");

	// Initialize Encoder
	status = XGpio_Initialize(&encoder, XPAR_AXI_GPIO_ENCODER_DEVICE_ID);
	// Initialize RGB LEDs
	status = XGpio_Initialize(&rgbleds, XPAR_AXI_GPIO_RGBLEDS_DEVICE_ID);
	// Initialize LEDs
	status = XGpio_Initialize(&leds, XPAR_AXI_GPIO_LED_DEVICE_ID);
	if (status != XST_SUCCESS) {
		xil_printf("Failed to initialize GPIO peripherals.");
		return XST_FAILURE;
	}

	XGpio_InterruptEnable(&encoder, 1);
	XGpio_InterruptGlobalEnable(&encoder);
	if (status != XST_SUCCESS) {
		xil_printf("Failed to enable button interrupts");
		return XST_FAILURE;
	}

	xil_printf("Successfully set up peripherals and interrupts.");
}

int main(){
	Xil_ICacheInvalidate();
	Xil_ICacheEnable();
	Xil_DCacheInvalidate();
	Xil_DCacheEnable();

	initializePeripherals();

	XGpio_DiscreteWrite(&leds, 1, mem_led); //start with LED at the rightmost LED.

	int i = 0;
	int on = 0;

	while(1) {
		if (i > 1000) {
			if (on == 0) {
				XGpio_DiscreteWrite(&rgbleds, 1, 2);
				on = 1;
			} else {
				XGpio_DiscreteWrite(&rgbleds, 1, 0);
				on = 0;
			}
			i = 0;
		}
		i++;
	}
}
