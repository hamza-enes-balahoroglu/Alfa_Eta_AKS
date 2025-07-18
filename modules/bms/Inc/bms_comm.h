#ifndef __BMS_COMM_H
#define __BMS_COMM_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define BMS_UART_HANDLE         huart1  // BMS için USART1 kullanılıyor
#define BMS_TIMEOUT_MS          100
#define BMS_CRITICAL_TIMEOUT_MS 500
#define BMS_PACKET_START_BYTE   0xA5
#define BMS_PACKET_LENGTH       10

typedef struct {
    float total_voltage;        // Vpack: Toplam Batarya Gerilimi (V)
    float current;              // Anlık Akım (A)
    float soc;                  // State of Charge: Kalan Enerji (%)
    float pack_temperature;     // Batarya Paketi Sıcaklığı (°C)
    float max_cell_voltage;     // En Yüksek Hücre Gerilimi (V)
    float min_cell_voltage;     // En Düşük Hücre Gerilimi (V)
    uint16_t error_flags;
} BMS_Data_t;

void BMS_Init(UART_HandleTypeDef *huart);

void BMS_Process(void);

bool BMS_GetData(BMS_Data_t *data_out);

bool BMS_IsCommunicationOK(void);

void BMS_RxCallback(uint8_t received_byte);

#endif
