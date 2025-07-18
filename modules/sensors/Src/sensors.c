#include "sensors.h"
#include <stdbool.h>

static ADC_HandleTypeDef* adc_handle;

void Sensors_Init(ADC_HandleTypeDef* hadc) {
    adc_handle = hadc;
}

static uint16_t read_adc_channel(uint32_t channel) {
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
    HAL_ADC_ConfigChannel(adc_handle, &sConfig);

    HAL_ADC_Start(adc_handle);
    HAL_ADC_PollForConversion(adc_handle, 100);
    uint16_t adc_value = HAL_ADC_GetValue(adc_handle);
    HAL_ADC_Stop(adc_handle);

    return adc_value;
}

uint8_t Sensors_ReadThrottle(void) {
    uint16_t raw_value = read_adc_channel(ADC_CHANNEL_0);
    uint8_t percentage = (uint8_t)((raw_value * 100) / 4095);

    return percentage;
}
int8_t Sensors_ReadSteering(void) {
    uint16_t raw_value = read_adc_channel(ADC_CHANNEL_1);
    int32_t mapped_value = ((raw_value - 2048) * 100) / 2048;

    if (mapped_value > 100) mapped_value = 100;
    if (mapped_value < -100) mapped_value = -100;

    return (int8_t)mapped_value;
}
bool Sensors_IsFuseOK(void) {
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_RESET) {
        return true;
    } else {
        return false;
    }
}


