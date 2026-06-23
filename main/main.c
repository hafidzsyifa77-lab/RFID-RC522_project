#include <esp_log.h>
#include "freertos/projdefs.h"
#include "rc522.h"
#include "driver/rc522_spi.h"
#include "rc522_picc.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include <string.h>

static const char *TAG = "User";

#define RC522_SPI_BUS_GPIO_MISO    (25)
#define RC522_SPI_BUS_GPIO_MOSI    (23)
#define RC522_SPI_BUS_GPIO_SCLK    (19)
#define RC522_SPI_SCANNER_GPIO_SDA (22)
#define RC522_SCANNER_GPIO_RST     (-1) 

#define OUT_PIN (2)
#define OUT_PIN2 (4)
#define OUT_PIN3 (5)
#define BUZZER_PIN (18)  
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL     LEDC_CHANNEL_0
#define LEDC_DUTY_RES    LEDC_TIMER_13_BIT 
#define MAX_AUTHORIZED_CARDS 2
#define MAX_UID_LEN 7

static const uint8_t authorized_uids[MAX_AUTHORIZED_CARDS][MAX_UID_LEN] = {
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // your spesific card UID max 7 byte
{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // If UID less then 7 byte you can empty several byte
};

void play_tone(uint32_t freq, uint32_t duration_ms) {
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 4095); 
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);    
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}
static rc522_spi_config_t driver_config = {
    .host_id = SPI3_HOST,
    .bus_config = &(spi_bus_config_t){
        .miso_io_num = RC522_SPI_BUS_GPIO_MISO,
        .mosi_io_num = RC522_SPI_BUS_GPIO_MOSI,
        .sclk_io_num = RC522_SPI_BUS_GPIO_SCLK,
    },
    .dev_config = {
        .spics_io_num = RC522_SPI_SCANNER_GPIO_SDA,
        .clock_speed_hz = 500000,
    },
    .rst_io_num = RC522_SCANNER_GPIO_RST,
};

static rc522_driver_handle_t driver;
static rc522_handle_t scanner;
static int failed_attempts = 0;
static bool is_lamp_on = false;
static uint8_t last_uid[4] = {0};

static bool is_authorized(uint8_t *uid, uint8_t len) {
    for (int i = 0; i < MAX_AUTHORIZED_CARDS; i++) {

        if (memcmp(uid, authorized_uids[i], len) == 0) {
            return true;
        }
    }
    return false;
}
static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data)
{
	
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;
    if (picc->state == RC522_PICC_STATE_ACTIVE) {
		
        if (is_authorized(picc->uid.value, picc->uid.length)) {
            is_lamp_on = !is_lamp_on;
            failed_attempts = 0;
            gpio_set_level(OUT_PIN, is_lamp_on ? 1 : 0);
            gpio_set_level(OUT_PIN2, 1 );
            play_tone(1500, 100); 
            vTaskDelay(pdMS_TO_TICKS(50));
            play_tone(2000, 150);
            gpio_set_level(OUT_PIN2, 0 );
            ESP_LOGI(TAG, "Access Granted");
            ESP_LOGI(TAG, "Your_User_Name");
            
        } else {
			failed_attempts++;
			gpio_set_level(OUT_PIN3, 1 );
            play_tone(400, 150);
            vTaskDelay(pdMS_TO_TICKS(50));
            play_tone(400, 150);
            gpio_set_level(OUT_PIN3, 0 );
            ESP_LOGW(TAG, "Access Denied!");
            ESP_LOGW(TAG, "Unidentified User Name");
		    ESP_LOGW(TAG, "Access Denied! %d Try", failed_attempts);	
		    if (failed_attempts >= 3) {
		        gpio_set_level(OUT_PIN3, 1 );
		        ESP_LOGE(TAG, "3 Times Wrong, Wait for 1 Minute...");
		        vTaskDelay(pdMS_TO_TICKS(60000));
		        gpio_set_level(OUT_PIN3, 0 ); 
		        failed_attempts = 0; 
		    }
        }
    }
    else if (picc->state == RC522_PICC_STATE_IDLE) {
        memset(last_uid, 0, 4);
    }

    if (picc->state == RC522_PICC_STATE_ACTIVE) {
        rc522_picc_print(picc);
    }
    else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) {
        ESP_LOGI(TAG, "Card has been removed");
    }
}

void app_main()
{
	gpio_set_direction(OUT_PIN3, GPIO_MODE_OUTPUT);
	gpio_set_direction(OUT_PIN2, GPIO_MODE_OUTPUT);
	gpio_set_direction(OUT_PIN, GPIO_MODE_OUTPUT);
	ledc_timer_config_t ledc_timer = {
	    .speed_mode = LEDC_MODE, .timer_num = LEDC_TIMER,
	    .duty_resolution = LEDC_DUTY_RES, .freq_hz = 1000,
	    .clk_cfg = LEDC_AUTO_CLK
	};
	ledc_timer_config(&ledc_timer);
	
	// Setup Channel
	ledc_channel_config_t ledc_channel = {
	    .speed_mode = LEDC_MODE, .channel = LEDC_CHANNEL,
	    .timer_sel = LEDC_TIMER, .gpio_num = BUZZER_PIN,
	    .duty = 0
	};
	ledc_channel_config(&ledc_channel);
    rc522_spi_create(&driver_config, &driver);
    rc522_driver_install(driver);

    rc522_config_t scanner_config = {
        .driver = driver,
    };

    rc522_create(&scanner_config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed, NULL);
    rc522_start(scanner);
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}
