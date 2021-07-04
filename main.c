// LCD ライブラリ、FatFs テスト

#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "LCDdriver.h"
#include "graphlib.h"
#include "ff.h"

void disperror(unsigned char *s, FRESULT fr){
	printstr(s);
	printstr(" FRESULT:");
	printnum(fr);
	while(1);
}

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

	// SDカード利用初期設定
	FATFS FatFs;		/* FatFs work area needed for each volume */
	FIL Fil;			/* File object needed for each open file */
	FRESULT fr;
	DIR dj;         /* Directory object */
	FILINFO fno;    /* File information */

	fr=f_mount(&FatFs, "", 0);		/* Give a work area to the default drive */
	if(fr) disperror("SD Mount Error.",fr);

	fr = f_findfirst(&dj, &fno, "", "*.BAS"); /* Start to search for BAS files */
	if(fr) disperror("Findfirst Error.",fr);

	int k,n=0;
	while (fr == FR_OK && fno.fname[0]) {         /* Repeat while an item is found */
		printstr(fno.fname);                /* Print the object name */
		n++;
		if(n%3){
			k=13-strlen(fno.fname);
			while(k--) printchar(' ');
		}
		else printchar('\n');
		fr = f_findnext(&dj, &fno);               /* Search for next item */
		if(fr) disperror("Findnext Error.",fr);
	}
	printstr("\nFound ");printnum(n);printstr(" files.");

	f_closedir(&dj);

}
