#include "motor_driver_2.h"
#include <string.h>

static UART_HandleTypeDef* motor_uart_2;
static uint8_t tx_buffer_2[8];
static uint8_t rx_buffer_2[8];
static uint8_t rx_index_2 = 0;
static volatile bool new_data_flag_2 = false;
static volatile uint32_t last_valid_message_timestamp_2 = 0;
static MotorData_t motor_data_2;
static bool communication_ok_2 = false;

static uint8_t calculate_checksum(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}

static void parse_motor_packet_2(const uint8_t* buffer) {
    int16_t raw_speed = (buffer[0] << 8) | buffer[1];
    motor_data_2.wheel_speed = (float)raw_speed;
    motor_data_2.fault_status = buffer[2];
}

void MotorDriver2_Init(UART_HandleTypeDef *huart) {
    motor_uart_2 = huart;
    last_valid_message_timestamp_2 = HAL_GetTick();
}

void MotorDriver2_Process(void) {
    if (new_data_flag_2) {
        new_data_flag_2 = false;
        uint8_t received_checksum = rx_buffer_2[3];
        uint8_t calculated_checksum = calculate_checksum(rx_buffer_2, 3);
        if (received_checksum == calculated_checksum) {
            parse_motor_packet_2(rx_buffer_2);
            last_valid_message_timestamp_2 = HAL_GetTick();
            communication_ok_2 = true;
        }
    }
    if (HAL_GetTick() - last_valid_message_timestamp_2 > MOTOR_DRIVER_TIMEOUT_MS) {
        communication_ok_2 = false;
    }
    if (HAL_GetTick() - last_valid_message_timestamp_2 > MOTOR_DRIVER_CRITICAL_TIMEOUT_MS) {
        communication_ok_2 = false;
    }
}

bool MotorDriver2_SetTorque(int16_t torque) {
    tx_buffer_2[0] = MOTOR_2_CMD_START_BYTE;
    tx_buffer_2[1] = 0x01;
    tx_buffer_2[2] = (torque >> 8) & 0xFF;
    tx_buffer_2[3] = torque & 0xFF;
    tx_buffer_2[4] = calculate_checksum(tx_buffer_2, 4);
    if (HAL_UART_Transmit_IT(motor_uart_2, tx_buffer_2, 5) == HAL_OK) {
        return true;
    }
    return false;
}

bool MotorDriver2_GetData(MotorData_t *data_out) {
    if (communication_ok_2) {
        memcpy(data_out, &motor_data_2, sizeof(MotorData_t));
        return true;
    }
    return false;
}

bool MotorDriver2_IsCommunicationOK(void) {
    return communication_ok_2;
}

void MotorDriver2_RxCallback(uint8_t received_byte) {
    if (rx_index_2 == 0) {
        if (received_byte == MOTOR_2_DATA_START_BYTE) {
            rx_index_2++;
        }
    } else {
        rx_buffer_2[rx_index_2 - 1] = received_byte;
        rx_index_2++;
        if (rx_index_2 > 4) {
            new_data_flag_2 = true;
            rx_index_2 = 0;
        }
    }
    if (rx_index_2 >= sizeof(rx_buffer_2)) {
        rx_index_2 = 0;
    }
}
