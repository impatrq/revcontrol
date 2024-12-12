################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../component/uart/fsl_adapter_miniusart.c 

C_DEPS += \
./component/uart/fsl_adapter_miniusart.d 

OBJS += \
./component/uart/fsl_adapter_miniusart.o 


# Each subdirectory must supply rules for building sources it contributes
component/uart/%.o: ../component/uart/%.c component/uart/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_LPC845M301JBD48 -DCPU_LPC845M301JBD48_cm0plus -DSDK_OS_BAREMETAL -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__REDLIB__ -I"C:\Users\estudiante\Documents\MCUXpressoIDE_11.10.0_3148\workspace\RTOS-REV-CONTROL\board" -I"C:\Users\estudiante\Documents\MCUXpressoIDE_11.10.0_3148\workspace\RTOS-REV-CONTROL\source" -I"C:\Users\estudiante\Documents\MCUXpressoIDE_11.10.0_3148\workspace\RTOS-REV-CONTROL\component\uart" -I"C:\Users\estudiante\Documents\MCUXpressoIDE_11.10.0_3148\workspace\RTOS-REV-CONTROL\drivers" -I"C:\Users\estudiante\Documents\MCUXpressoIDE_11.10.0_3148\workspace\RTOS-REV-CONTROL\CMSIS" -I"C:\Users\estudiante\Documents\MCUXpressoIDE_11.10.0_3148\workspace\RTOS-REV-CONTROL\device" -I"C:\Users\estudiante\Documents\MCUXpressoIDE_11.10.0_3148\workspace\RTOS-REV-CONTROL\utilities" -I"C:\Users\estudiante\curso-lse\lpc845\ejemplos\freertos\inc" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m0plus -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-component-2f-uart

clean-component-2f-uart:
	-$(RM) ./component/uart/fsl_adapter_miniusart.d ./component/uart/fsl_adapter_miniusart.o

.PHONY: clean-component-2f-uart

