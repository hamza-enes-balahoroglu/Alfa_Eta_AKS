#include "bms.h"
#include <string.h>

static UART_HandleTypeDef* bms_uart;             // BMS haberleşmesi için kullanılacak UART biriminin tanıtıcısı
static uint8_t rx_buffer[128];                   // BMS'ten gelen ham verilerin geçici olarak depolandığı tampon
static uint8_t rx_index = 0;                     // Gelen baytın rx_buffer'da yazılacağı konumu tutar
static volatile bool new_data_flag = false;      // Yeni, eksiksiz bir BMS paketi alınıp alınmadığını belirtir (volatile: kesme içinde değişebilir)
static volatile uint32_t last_valid_message_timestamp = 0; // En son geçerli BMS mesajının alındığı zaman (ms cinsinden) (volatile: zamanla güncellenir)
static BMS_Data_t bms_data;                     // BMS'ten alınan güncel verilerin saklandığı yapı
static bool communication_ok = false;           // BMS ile haberleşmenin aktif ve sağlıklı olup olmadığını gösterir

/**
 * @brief Veri bloğunun sağlama toplamını (checksum) hesaplar.
 * @param data Sağlama toplamı hesaplanacak veri dizisi.
 * @param len Veri dizisinin uzunluğu.
 * @return Hesaplanan sağlama toplamı.
 */
static uint8_t calculate_checksum(const uint8_t* data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
    }
    return crc;
}

/**
 * @brief Alınan BMS paketini ayrıştırır ve verileri BMS_Data_t yapısına kaydeder.
 * @param buffer Ayrıştırılacak paketi içeren tampon.
 */
static void parse_bms_packet(const uint8_t* buffer) {
    uint16_t raw_voltage = (buffer[0] << 8) | buffer[1];
    uint16_t raw_current = (buffer[2] << 8) | buffer[3];
    uint8_t  raw_soc = buffer[4];
    int8_t   raw_temp = buffer[5];
    uint16_t raw_vmax = (buffer[6] << 8) | buffer[7];
    uint16_t raw_vmin = (buffer[8] << 8) | buffer[9];

    bms_data.total_voltage = (float)raw_voltage / 100.0f;
    // BMS'ten gelen ham akım değeri işareti içeriyorsa aşağıdaki gibi ayarlama gerekebilir.
    // Örneğin, 0x8000'den büyükse negatif, aksi halde pozitif.
    // Şimdilik, sadece bölüyoruz. Eğer akım negatif olabiliyorsa bu kısım gözden geçirilmelidir.
    bms_data.current = (float)((int16_t)raw_current) / 100.0f;
    bms_data.soc = (float)raw_soc;
    bms_data.pack_temperature = (float)raw_temp;
    bms_data.max_cell_voltage = (float)raw_vmax / 1000.0f;
    bms_data.min_cell_voltage = (float)raw_vmin / 1000.0f;
    // Hata kodları artık struct'ta olmadığı için buraya eklenmiyor.
}

/**
 * @brief BMS haberleşme modülünü başlatır.
 * @param huart Kullanılacak UART handle'ı.
 */
void BMS_Init(UART_HandleTypeDef *huart) {
    bms_uart = huart;
    last_valid_message_timestamp = HAL_GetTick(); // Başlangıç zamanını ayarla
    communication_ok = false; // Başlangıçta haberleşmeyi kapalı varsay
}

/**
 * @brief BMS verilerini işler ve haberleşme durumunu kontrol eder.
 * Bu fonksiyon ana döngüde periyodik olarak çağrılmalıdır.
 */
void BMS_Process(void) {
    // Yeni veri gelip gelmediğini kontrol et
    if (new_data_flag) {
        new_data_flag = false; // Bayrağı sıfırla

        // Sağlama toplamını doğrula
        uint8_t received_checksum = rx_buffer[BMS_PACKET_LENGTH]; // Çekirdek veri uzunluğu + başlangıç baytı yok
        uint8_t calculated_checksum = calculate_checksum(rx_buffer, BMS_PACKET_LENGTH);

        if (received_checksum == calculated_checksum) {
            parse_bms_packet(rx_buffer); // Paketi ayrıştır
            last_valid_message_timestamp = HAL_GetTick(); // Son geçerli mesaj zamanını güncelle
            communication_ok = true; // Haberleşme başarılı
        } else {
            // Checksum hatası durumunda, haberleşmeyi şimdilik kesmeyelim, sadece güncellemeyelim
            // İsterseniz burada bir hata sayacı ekleyebilirsiniz.
            // Örneğin: crc_error_count++;
        }
    }

    // Haberleşme zaman aşımı kontrolü
    uint32_t current_tick = HAL_GetTick();
    if (current_tick - last_valid_message_timestamp > BMS_CRITICAL_TIMEOUT_MS) { // Kritik zaman aşımı daha önemli
        communication_ok = false; // Haberleşme kesildi
    } else if (current_tick - last_valid_message_timestamp > BMS_TIMEOUT_MS) {
        // Normal zaman aşımı, haberleşme bir süreliğine kesik ama henüz kritik değil.
        // İhtiyaca göre burada başka bir durum (örneğin "veri beklemede") ayarlanabilir.
        // Şimdilik, sadece kritik zaman aşımında communication_ok'ı false yapıyoruz.
        // Eğer BMS_TIMEOUT_MS'den sonra da communication_ok'ı false yapmak isterseniz,
        // buradaki if bloğunun da communication_ok = false; içermesi gerekir.
        // Ancak genellikle sadece kritik durumda keseriz.
    }
}

/**
 * @brief BMS'ten en son alınan verileri dışarıya sağlar.
 * @param data_out Verilerin kopyalanacağı BMS_Data_t işaretçisi.
 * @return Veri başarıyla kopyalandıysa true, aksi takdirde false.
 */
bool BMS_GetData(BMS_Data_t *data_out) {
    if (communication_ok) {
        memcpy(data_out, &bms_data, sizeof(BMS_Data_t));
        return true;
    }
    return false;
}

/**
 * @brief BMS ile haberleşmenin durumunu kontrol eder.
 * @return Haberleşme açıksa true, aksi takdirde false.
 */
bool BMS_IsCommunicationOK(void) {
    return communication_ok;
}

/**
 * @brief UART alım kesmesi geri çağırım fonksiyonu.
 * Her yeni bayt alındığında bu fonksiyon çağrılmalıdır.
 * @param received_byte Alınan bayt.
 */
void BMS_RxCallback(uint8_t received_byte) {
    // Paket başlangıç baytını kontrol et
    if (rx_index == 0) {
        if (received_byte == BMS_PACKET_START_BYTE) {
            rx_buffer[rx_index++] = received_byte; // Başlangıç baytını tampona kaydet
        }
    } else {
        // Gelen baytı tampona kaydet
        rx_buffer[rx_index++] = received_byte;

        // Tam bir paket alınıp alınmadığını kontrol et (BMS_PACKET_LENGTH + 1 (checksum) + 1 (start byte))
        // Bu mantıkta, BMS_PACKET_LENGTH, veri + checksum uzunluğunu ifade ediyorsa,
        // Başlangıç baytı hariç, veri + checksum = BMS_PACKET_LENGTH olmalı.
        // Eğer BMS_PACKET_LENGTH sadece veri uzunluğunu ifade ediyorsa, o zaman
        // veri uzunluğu + checksum uzunluğu (1 bayt) = BMS_PACKET_LENGTH + 1
        // ve başlangıç baytı da dahilse toplamda BMS_PACKET_LENGTH + 2 bayt olmalı.

        // Önceki tanıma göre BMS_PACKET_LENGTH: 10 olarak tanımlanmış ve bu muhtemelen
        // başlangıç baytı HARİÇ veri uzunluğudur. Yani paket aslında (Başlangıç Baytı + Veri (10 byte) + Checksum (1 byte)) = 12 byte
        // olmalı. Bu durumda:
        if (rx_index >= (BMS_PACKET_LENGTH + 1 + 1)) { // Başlangıç baytı + veri + checksum
            new_data_flag = true; // Yeni veri geldiğini işaretle
            rx_index = 0; // Tamponu sıfırla, yeni paket için hazırla
        }
    }

    // Tampon taşmasını önle
    if (rx_index >= sizeof(rx_buffer)) {
        rx_index = 0; // Tampon taştıysa sıfırla, paketi at
    }
}
