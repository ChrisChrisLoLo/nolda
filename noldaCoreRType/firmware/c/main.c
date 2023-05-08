#include <FreeRTOS.h>
#include <queue.h>
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

#define PIN_ROW_0 1
#define PIN_ROW_1 0
#define PIN_COL_0 2
#define PIN_COL_1 3
#define PIN_COL_2 4
#define PIN_COL_3 5

#define LED_PIN PICO_DEFAULT_LED_PIN
#define EXTMODE_PIN 14
#define VCOM_PIN 13

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

void draw_display(char *message) {
	u8g2_ClearBuffer(&u8g2);
        u8g2_ClearDisplay(&u8g2);
	u8g2_SetDrawColor(&u8g2, 1);
        u8g2_SetFont(&u8g2, u8g2_font_t0_15_te);
        u8g2_DrawStr(&u8g2, 0, 10, message);
        u8g2_DrawStr(&u8g2, 0, 20, message);
        u8g2_DrawStr(&u8g2, 0, 40, message);
        u8g2_UpdateDisplay(&u8g2);
}

static const uint8_t msg_queue_len = 5;

static QueueHandle_t msg_queue;

// Used to refresh the screen to prevent polarization
// Can likely be moved to pio down the road
void refresh_screen_task()
{   
  draw_display("hello!");

  while (true) {
    gpio_put(LED_PIN, 1);
    gpio_put(VCOM_PIN, 1);

    vTaskDelay(200/portTICK_PERIOD_MS); // delay for 200 ms
    gpio_put(LED_PIN, 0);
    gpio_put(VCOM_PIN, 0);

    vTaskDelay(200/portTICK_PERIOD_MS);
  }
}

void draw_screen_task(){
  vTaskDelay(200/portTICK_PERIOD_MS); // deplay to allow screen to be setup. A bit sloppy, move init code to main

  int item;
  char message[32];

  while(true){
    if (xQueueReceive(msg_queue, (void *)&item, 0) == pdTRUE) {
      sprintf(message,"%d",item);
      draw_display(message);
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
  }

}

#define KEY_MAP_ROW_LEN 2
#define KEY_MAP_COL_LEN 4 

int old_matrix_code = 0;
void read_keyboard_task()
{
  int rows[] = {PIN_ROW_0,PIN_ROW_1};
  int cols[] = {PIN_COL_0,PIN_COL_1,PIN_COL_2,PIN_COL_3};
  int row_len = (int) sizeof(rows) / sizeof(rows[0]);
  int col_len = (int) sizeof(cols) / sizeof(cols[0]);
  int keymap[KEY_MAP_ROW_LEN][KEY_MAP_COL_LEN] = {{1,2,3,4},{5,6,7,8}};

  for (int i=0; i<row_len; i++){
    gpio_init(rows[i]);
    gpio_set_dir(rows[i], GPIO_IN);
  }
  for (int i=0; i<col_len; i++){
    gpio_init(cols[i]);
    gpio_set_dir(cols[i], GPIO_OUT);
  }
  
  // scan
  int new_matrix_code = 0;
  while (true) {
    new_matrix_code = 0;
    for (int row_ind=0; row_ind<row_len; row_ind++){
      for (int col_ind=0; col_ind<col_len; col_ind++){
        gpio_put(cols[col_ind], 1);
        //printf("powering pin:%i\n", cols[col_ind]);
        vTaskDelay(5/portTICK_PERIOD_MS); // debounce
        bool is_key_pressed = gpio_get(rows[row_ind]);
        //printf("got %i from %i pin\n", gpio_get(rows[row_ind]), rows[row_ind]);
        new_matrix_code <<= 1; // convert matrix to binary bytes represented as int 
        new_matrix_code += is_key_pressed;
        gpio_put(cols[col_ind],0);
      }
    }
    printf("new matrix code:%i\n", new_matrix_code);
    if (new_matrix_code != old_matrix_code){
      if (xQueueSend(msg_queue, &new_matrix_code, 10) != pdTRUE){
        // error
      };
      old_matrix_code = new_matrix_code;
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

int main()
{
    stdio_init_all();
    printf("Board init!\n");

    // // attempt to reset spi bus
    // gpio_init(PIN_CS);
    // gpio_set_dir(PIN_CS, GPIO_OUT);
    // gpio_put(PIN_CS, 0);
    // sleep_us(100);
    // //


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

    msg_queue = xQueueCreate(msg_queue_len, sizeof(int));

    xTaskCreate(refresh_screen_task, "Refresh_Screen_Task", 256, NULL, 1, NULL);
    xTaskCreate(read_keyboard_task, "Read_Keyboard_Task", 2000, NULL, 1, NULL);
    xTaskCreate(draw_screen_task, "Draw_Screen_Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){};
}