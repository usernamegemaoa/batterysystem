################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../bs01thread.c \
../dis01thread.c \
../manageData.c \
../monitor.c \
../networkCommunication.c \
../s800bmSemaphore.c \
../s800serialDAO.c 

OBJS += \
./bs01thread.o \
./dis01thread.o \
./manageData.o \
./monitor.o \
./networkCommunication.o \
./s800bmSemaphore.o \
./s800serialDAO.o 

C_DEPS += \
./bs01thread.d \
./dis01thread.d \
./manageData.d \
./monitor.d \
./networkCommunication.d \
./s800bmSemaphore.d \
./s800serialDAO.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


