#include "telemetry_comm.h"
#include <string.h>

static UART_HandleTypeDef* telemetry_uart;

#define PACKET_HEADER_0 0xAB
#define PACKET_HEADER_1 0xCD
#define PAYLOAD_LENGTH  8

static uint8_t calculate_checksum(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc += data[i];
    }
    return crc;
}

void Telemetry_Init(UART_HandleTypeDef *huart) {
    telemetry_uart = huart;
}

bool Telemetry_SendData(const VehicleData_t* data) {
    if (telemetry_uart == NULL || data == NULL) {
        return false;
    }

    uint8_t tx_buffer[12];

    tx_buffer[0] = PACKET_HEADER_0;
    tx_buffer[1] = PACKET_HEADER_1;
    tx_buffer[2] = PAYLOAD_LENGTH;

    uint16_t voltage = (uint16_t)(data->bms_data.total_voltage * 100);
    tx_buffer[3] = (voltage >> 8) & 0xFF;
    tx_buffer[4] = voltage & 0xFF;
    tx_buffer[5] = (uint8_t)data->bms_data.soc;
    tx_buffer[6] = (int8_t)data->bms_data.pack_temperature;
    int16_t speed1 = (int16_t)data->motor1_data.wheel_speed;
    tx_buffer[7] = (speed1 >> 8) & 0xFF;
    tx_buffer[8] = speed1 & 0xFF;
    int16_t speed2 = (int16_t)data->motor2_data.wheel_speed;
    tx_buffer[9] = (speed2 >> 8) & 0xFF;
    tx_buffer[10] = speed2 & 0xFF;
    tx_buffer[11] = calculate_checksum(&tx_buffer[3], PAYLOAD_LENGTH);

    if (HAL_UART_Transmit(telemetry_uart, tx_buffer, sizeof(tx_buffer), 100) == HAL_OK) {
        return true;
    }

    return false;
}
