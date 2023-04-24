#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <u8g2.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include <string.h>

u8g2_t u8g2;

// ADD THE INIT DISPLAY 
#define SPI_PORT spi1
#define PIN_CS 12
#define PIN_SCK 10
#define PIN_MOSI 11
#define SPI_SPEED 10 * 1000 * 1000

uint8_t u8x8_byte_pico_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
  uint8_t *data;
  switch (msg) {
  case U8X8_MSG_BYTE_SEND:
    data = (uint8_t *)arg_ptr;
    spi_write_blocking(SPI_PORT, data, arg_int);
    break;
  case U8X8_MSG_BYTE_INIT:
    u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
    break;
  case U8X8_MSG_BYTE_SET_DC:
    u8x8_gpio_SetDC(u8x8, arg_int);
    break;
  case U8X8_MSG_BYTE_START_TRANSFER:
    u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
    u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO,u8x8->display_info->post_chip_enable_wait_ns, NULL);
    break;
  case U8X8_MSG_BYTE_END_TRANSFER:
    u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO,
                            u8x8->display_info->pre_chip_disable_wait_ns, NULL);
    u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
    break;
  default:
    return 0;
  }
  return 1;
}

uint8_t u8x8_gpio_and_delay_pico(u8x8_t *u8x8, uint8_t msg,uint8_t arg_int, void *arg_ptr) {
  switch (msg) {
  case U8X8_MSG_GPIO_AND_DELAY_INIT: 
    spi_init(SPI_PORT, SPI_SPEED);
    gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    break;                  
  case U8X8_MSG_DELAY_NANO: // delay arg_int * 1 nano second
    sleep_us(arg_int);
    break;
  case U8X8_MSG_DELAY_100NANO: // delay arg_int * 100 nano seconds
    sleep_us(arg_int);
    break;
  case U8X8_MSG_DELAY_10MICRO: // delay arg_int * 10 micro seconds
    sleep_us(arg_int * 10);
    break;
  case U8X8_MSG_DELAY_MILLI: // delay arg_int * 1 milli second
    sleep_ms(arg_int);
    break;
  case U8X8_MSG_GPIO_CS: // CS (chip select) pin: Output level in arg_int
    gpio_put(PIN_CS, arg_int);
    break;
//   case U8X8_MSG_GPIO_DC: // DC (data/cmd, A0, register select) pin: Output level
//     gpio_put(PIN_DC, arg_int);
//     break;
//   case U8X8_MSG_GPIO_RESET: // Reset pin: Output level in arg_int
//     gpio_put(PIN_RST, arg_int);  // printf("U8X8_MSG_GPIO_RESET %d\n", arg_int);
//     break;
  default:
    u8x8_SetGPIOResult(u8x8, 1); // default return value
    break;
  }
  return 1;
}

void draw_display() {
	char hey[13] = "Hello world!!";
	u8g2_ClearBuffer(&u8g2);
        u8g2_ClearDisplay(&u8g2);
	u8g2_SetDrawColor(&u8g2, 1);
        u8g2_SetFont(&u8g2, u8g2_font_t0_15_te);
        u8g2_DrawStr(&u8g2, 0, 10, hey);
        u8g2_DrawStr(&u8g2, 0, 20, hey);
        u8g2_DrawStr(&u8g2, 0, 40, hey);
        u8g2_UpdateDisplay(&u8g2);
}

// Used to refresh the screen to prevent polarization
// Can likely be moved to pio down the road
void refresh_screen_task()
{   
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    const uint EXTMODE_PIN = 14;
    const uint VCOM_PIN = 13;
    gpio_init(LED_PIN);
    gpio_init(EXTMODE_PIN);
    gpio_init(VCOM_PIN);

    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(EXTMODE_PIN, GPIO_OUT);
    gpio_set_dir(VCOM_PIN, GPIO_OUT);

    gpio_put(EXTMODE_PIN, 1);   // enable extmode
    gpio_put(VCOM_PIN, 1);
    gpio_put(LED_PIN, 1);

    u8g2_Setup_ls013b7dh03_128x128_f(&u8g2, U8G2_R0, u8x8_byte_pico_hw_spi, u8x8_gpio_and_delay_pico);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    draw_display();

    while (true) {
        gpio_put(LED_PIN, 1);
        gpio_put(VCOM_PIN, 1);

        vTaskDelay(200/portTICK_PERIOD_MS); // delay for 200 ms
        gpio_put(LED_PIN, 0);
        gpio_put(VCOM_PIN, 0);

        vTaskDelay(200/portTICK_PERIOD_MS);
    }
}

int main()
{
    stdio_init_all();

    // // attempt to reset spi bus
    // gpio_init(PIN_CS);
    // gpio_set_dir(PIN_CS, GPIO_OUT);
    // gpio_put(PIN_CS, 0);
    // sleep_us(100);
    // //

    xTaskCreate(refresh_screen_task, "Refresh_Screen_Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    u8g2_Setup_ls013b7dh03_128x128_1(&u8g2, U8G2_R0, u8x8_byte_pico_hw_spi, u8x8_gpio_and_delay_pico);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    draw_display();

    while(1){};
}