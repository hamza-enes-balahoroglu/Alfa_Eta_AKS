#include "bms_comm.h"
#include <string.h>

static UART_HandleTypeDef* bms_uart;
static uint8_t rx_buffer[128];
static uint8_t rx_index = 0;
static volatile bool new_data_flag = false;
static volatile uint32_t last_valid_message_timestamp = 0;
static BMS_Data_t bms_data;
static bool communication_ok = false;

static uint8_t calculate_checksum(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}
static void parse_bms_packet(const uint8_t* buffer) {
    uint16_t raw_voltage = (buffer[0] << 8) | buffer[1];
    uint16_t raw_current = (buffer[2] << 8) | buffer[3];
    uint8_t  raw_soc = buffer[4];
    int8_t   raw_temp = buffer[5];
    uint16_t raw_vmax = (buffer[6] << 8) | buffer[7];
    uint16_t raw_vmin = (buffer[8] << 8) | buffer[9];

    bms_data.total_voltage = (float)raw_voltage / 100.0f;
    bms_data.current = (float)raw_current / 100.0f;
    bms_data.soc = (float)raw_soc;
    bms_data.pack_temperature = (float)raw_temp;
    bms_data.max_cell_voltage = (float)raw_vmax / 1000.0f;
    bms_data.min_cell_voltage = (float)raw_vmin / 1000.0f;
}
void BMS_Init(UART_HandleTypeDef *huart) {
    bms_uart = huart;
    last_valid_message_timestamp = HAL_GetTick();
}
void BMS_Process(void) {
    if (new_data_flag) {
        new_data_flag = false;
        uint8_t received_checksum = rx_buffer[BMS_PACKET_LENGTH];
        uint8_t calculated_checksum = calculate_checksum(rx_buffer, BMS_PACKET_LENGTH);

        if (received_checksum == calculated_checksum) {
            parse_bms_packet(rx_buffer);
            last_valid_message_timestamp = HAL_GetTick();
            communication_ok = true;
        }
    }

    if (HAL_GetTick() - last_valid_message_timestamp > BMS_TIMEOUT_MS) {
        communication_ok = false;
    }

    if (HAL_GetTick() - last_valid_message_timestamp > BMS_CRITICAL_TIMEOUT_MS) {
        communication_ok = false;
    }
}
bool BMS_GetData(BMS_Data_t *data_out) {
    if (communication_ok) {
        memcpy(data_out, &bms_data, sizeof(BMS_Data_t));
        return true;
    }
    return false;
}
bool BMS_IsCommunicationOK(void) { // bool yerine HAL_StatusTypeDef döndürsünler 
    return communication_ok;
}
void BMS_RxCallback(uint8_t received_byte) {
    static uint8_t data_counter = 0;

    if (rx_index == 0) {
        if (received_byte == BMS_PACKET_START_BYTE) {
            rx_buffer[rx_index++] = received_byte;
            data_counter = 0;
        }
    } else {
        rx_buffer[rx_index++] = received_byte;
        data_counter++;

        if (data_counter >= (BMS_PACKET_LENGTH + 1)) {
            memcpy(rx_buffer, &rx_buffer[1], BMS_PACKET_LENGTH + 1);
            new_data_flag = true;
            rx_index = 0;
        }
    }
    if (rx_index >= sizeof(rx_buffer)) {
        rx_index = 0;
    }
}
