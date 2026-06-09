################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Button.c \
../Core/Src/DisplayUI.c \
../Core/Src/ESP01.c \
../Core/Src/OLED.c \
../Core/Src/Servo.c \
../Core/Src/comandos.c \
../Core/Src/hcsr04.c \
../Core/Src/main.c \
../Core/Src/protocolo.c \
../Core/Src/seguidor.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f1xx.c 

OBJS += \
./Core/Src/Button.o \
./Core/Src/DisplayUI.o \
./Core/Src/ESP01.o \
./Core/Src/OLED.o \
./Core/Src/Servo.o \
./Core/Src/comandos.o \
./Core/Src/hcsr04.o \
./Core/Src/main.o \
./Core/Src/protocolo.o \
./Core/Src/seguidor.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f1xx.o 

C_DEPS += \
./Core/Src/Button.d \
./Core/Src/DisplayUI.d \
./Core/Src/ESP01.d \
./Core/Src/OLED.d \
./Core/Src/Servo.d \
./Core/Src/comandos.d \
./Core/Src/hcsr04.d \
./Core/Src/main.d \
./Core/Src/protocolo.d \
./Core/Src/seguidor.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f1xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/Button.cyclo ./Core/Src/Button.d ./Core/Src/Button.o ./Core/Src/Button.su ./Core/Src/DisplayUI.cyclo ./Core/Src/DisplayUI.d ./Core/Src/DisplayUI.o ./Core/Src/DisplayUI.su ./Core/Src/ESP01.cyclo ./Core/Src/ESP01.d ./Core/Src/ESP01.o ./Core/Src/ESP01.su ./Core/Src/OLED.cyclo ./Core/Src/OLED.d ./Core/Src/OLED.o ./Core/Src/OLED.su ./Core/Src/Servo.cyclo ./Core/Src/Servo.d ./Core/Src/Servo.o ./Core/Src/Servo.su ./Core/Src/comandos.cyclo ./Core/Src/comandos.d ./Core/Src/comandos.o ./Core/Src/comandos.su ./Core/Src/hcsr04.cyclo ./Core/Src/hcsr04.d ./Core/Src/hcsr04.o ./Core/Src/hcsr04.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/protocolo.cyclo ./Core/Src/protocolo.d ./Core/Src/protocolo.o ./Core/Src/protocolo.su ./Core/Src/seguidor.cyclo ./Core/Src/seguidor.d ./Core/Src/seguidor.o ./Core/Src/seguidor.su ./Core/Src/stm32f1xx_hal_msp.cyclo ./Core/Src/stm32f1xx_hal_msp.d ./Core/Src/stm32f1xx_hal_msp.o ./Core/Src/stm32f1xx_hal_msp.su ./Core/Src/stm32f1xx_it.cyclo ./Core/Src/stm32f1xx_it.d ./Core/Src/stm32f1xx_it.o ./Core/Src/stm32f1xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f1xx.cyclo ./Core/Src/system_stm32f1xx.d ./Core/Src/system_stm32f1xx.o ./Core/Src/system_stm32f1xx.su

.PHONY: clean-Core-2f-Src

