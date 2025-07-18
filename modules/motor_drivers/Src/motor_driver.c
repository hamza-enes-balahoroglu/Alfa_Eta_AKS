#include "motor_driver.h"
#include <string.h>


static UART_HandleTypeDef* motor_uart;
static uint8_t tx_buffer[8];
static uint8_t rx_buffer[8];
static uint8_t rx_index = 0;
static volatile bool new_data_flag = false;
static volatile uint32_t last_valid_message_timestamp = 0;

static MotorData_t motor_data;
static bool communication_ok = false;
static uint8_t calculate_checksum(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}
static void parse_motor_packet(const uint8_t* buffer) {
    int16_t raw_speed = (buffer[0] << 8) | buffer[1];

    motor_data.wheel_speed = (float)raw_speed;
    motor_data.fault_status = buffer[2];
}

void MotorDriver1_Init(UART_HandleTypeDef *huart) {
    motor_uart = huart;
    last_valid_message_timestamp = HAL_GetTick();
}

void MotorDriver1_Process(void) {
    if (new_data_flag) {
        new_data_flag = false;

        uint8_t received_checksum = rx_buffer[3];
        uint8_t calculated_checksum = calculate_checksum(rx_buffer, 3);

        if (received_checksum == calculated_checksum) {
            parse_motor_packet(rx_buffer);
            last_valid_message_timestamp = HAL_GetTick();
            communication_ok = true;
        }
    }

    if (HAL_GetTick() - last_valid_message_timestamp > MOTOR_DRIVER_TIMEOUT_MS) {
        communication_ok = false;
    }

    if (HAL_GetTick() - last_valid_message_timestamp > MOTOR_DRIVER_CRITICAL_TIMEOUT_MS) {
        communication_ok = false;
    }
}

bool MotorDriver1_SetTorque(int16_t torque) {// tork hesaplaması için ayrı dosya açalım
    tx_buffer[0] = MOTOR_CMD_START_BYTE;
    tx_buffer[1] = 0x01;
    tx_buffer[2] = (torque >> 8) & 0xFF;
    tx_buffer[3] = torque & 0xFF;
    tx_buffer[4] = calculate_checksum(tx_buffer, 4);

    if (HAL_UART_Transmit_IT(motor_uart, tx_buffer, 5) == HAL_OK) {
        return true;
    }
    return false;
}

bool MotorDriver1_GetData(MotorData_t *data_out) {
    if (communication_ok) {
        memcpy(data_out, &motor_data, sizeof(MotorData_t));
        return true;
    }
    return false;
}

bool MotorDriver1_IsCommunicationOK(void) { // Adı handshake olabilir
    return communication_ok;
}

void MotorDriver1_RxCallback(uint8_t received_byte) {
    if (rx_index == 0) {
        if (received_byte == MOTOR_DATA_START_BYTE) {
            rx_index++;
        }
    } else {
        rx_buffer[rx_index - 1] = received_byte;
        rx_index++;
        if (rx_index > 4) {
            new_data_flag = true;
            rx_index = 0;
        }
    }

    if (rx_index >= sizeof(rx_buffer)) {
        rx_index = 0;
    }
}
