#ifndef __TELEMETRY_COMM_H
#define __TELEMETRY_COMM_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include "bms_comm.h"
#include "motor_driver.h"

#define TELEMETRY_UART_HANDLE  huart6  // Telemetri kartı için USART6 kullanılıyor

typedef struct {
    BMS_Data_t bms_data;
    MotorData_t motor1_data;
    MotorData_t motor2_data;
} VehicleData_t;

void Telemetry_Init(UART_HandleTypeDef *huart);
bool Telemetry_SendData(const VehicleData_t* data);

#endif
