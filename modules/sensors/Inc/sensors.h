#ifndef __SENSORS_H
#define __SENSORS_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

void Sensors_Init(ADC_HandleTypeDef* hadc);

uint8_t Sensors_ReadThrottle(void);

int8_t Sensors_ReadSteering(void);

bool Sensors_IsFuseOK(void);

#endif
