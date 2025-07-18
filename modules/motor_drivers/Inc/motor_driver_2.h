#ifndef __MOTOR_DRIVER_2_H
#define __MOTOR_DRIVER_2_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define MOTOR_DRIVER_2_UART_HANDLE  huart3
#define MOTOR_DRIVER_TIMEOUT_MS     50
#define MOTOR_DRIVER_CRITICAL_TIMEOUT_MS 500
#define MOTOR_2_CMD_START_BYTE      0xB2
#define MOTOR_2_DATA_START_BYTE     0xC2
#include "motor_driver.h"


void MotorDriver2_Init(UART_HandleTypeDef *huart);
void MotorDriver2_Process(void);
bool MotorDriver2_SetTorque(int16_t torque);
bool MotorDriver2_GetData(MotorData_t *data_out);
bool MotorDriver2_IsCommunicationOK(void);
void MotorDriver2_RxCallback(uint8_t received_byte);

#endif
