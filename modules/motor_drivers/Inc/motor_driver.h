#ifndef __MOTOR_DRIVER_H
#define __MOTOR_DRIVER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>


#define MOTOR_DRIVER_1_UART_HANDLE  huart2  // Motor Sürücü 1 için USART2 kullanılıyor
#define MOTOR_DRIVER_TIMEOUT_MS     50
#define MOTOR_DRIVER_CRITICAL_TIMEOUT_MS 500
#define MOTOR_CMD_START_BYTE        0xB1
#define MOTOR_DATA_START_BYTE       0xC1

typedef struct {
    float wheel_speed;
    uint8_t fault_status;
} MotorData_t;

void MotorDriver1_Init(UART_HandleTypeDef *huart);

void MotorDriver1_Process(void);

bool MotorDriver1_SetTorque(int16_t torque);

bool MotorDriver1_GetData(MotorData_t *data_out);

bool MotorDriver1_IsCommunicationOK(void);

void MotorDriver1_RxCallback(uint8_t received_byte);

#endif
