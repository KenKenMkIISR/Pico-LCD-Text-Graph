// LCD ライブラリテスト

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "LCDdriver.h"
#include "graphlib.h"

void main(void){
    stdio_init_all();

	// 液晶用ポート設定
    // Enable SPI at 20 MHz and connect to GPIOs
    spi_init(LCD_SPICH, 20000 * 1000);
    gpio_set_function(LCD_SPI_RX, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SPI_TX, GPIO_FUNC_SPI);
    gpio_set_function(LCD_SPI_SCK, GPIO_FUNC_SPI);

	gpio_init(LCD_CS);
	gpio_put(LCD_CS, 1);
	gpio_set_dir(LCD_CS, GPIO_OUT);
	gpio_init(LCD_DC);
	gpio_put(LCD_DC, 1);
	gpio_set_dir(LCD_DC, GPIO_OUT);
	gpio_init(LCD_RESET);
	gpio_put(LCD_RESET, 1);
	gpio_set_dir(LCD_RESET, GPIO_OUT);

	init_textgraph(); //液晶初期化、テキスト利用開始

	unsigned int n=0;
	while(1){
		printnum(n++);
		printchar(' ');
	}
}
