################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LD_SRCS += \
../src/lscript.ld 

C_SRCS += \
../src/extra.c \
../src/peripheral_handlers.c \
../src/platform.c \
../src/sevenSeg_new.c \
../src/timer.c \
../src/timing.c 

OBJS += \
./src/extra.o \
./src/peripheral_handlers.o \
./src/platform.o \
./src/sevenSeg_new.o \
./src/timer.o \
./src/timing.o 

C_DEPS += \
./src/extra.d \
./src/peripheral_handlers.d \
./src/platform.d \
./src/sevenSeg_new.d \
./src/timer.d \
./src/timing.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MicroBlaze gcc compiler'
	mb-gcc -Wall -O0 -g3 -c -fmessage-length=0 -MT"$@" -I../../ece154lab1apsn_bsp/microblaze_0/include -mlittle-endian -mxl-barrel-shift -mcpu=v11.0 -mno-xl-soft-mul -Wl,--no-relax -ffunction-sections -fdata-sections -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


