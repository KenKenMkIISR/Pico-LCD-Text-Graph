// LCD ライブラリ、FatFs テスト
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "LCDdriver.h"
#include "graphlib.h"
#include "ff.h"

// 入力ボタンのビット定義
#define GPIO_KEYUP 0
#define GPIO_KEYLEFT 1
#define GPIO_KEYRIGHT 2
#define GPIO_KEYDOWN 3
#define GPIO_KEYSTART 4
#define GPIO_KEYFIRE 5
#define KEYUP (1 << GPIO_KEYUP)
#define KEYLEFT (1 << GPIO_KEYLEFT)
#define KEYRIGHT (1 << GPIO_KEYRIGHT)
#define KEYDOWN (1 << GPIO_KEYDOWN)
#define KEYSTART (1 << GPIO_KEYSTART)
#define KEYFIRE (1 << GPIO_KEYFIRE)
#define KEYSMASK (KEYUP | KEYLEFT | KEYRIGHT | KEYDOWN | KEYSTART | KEYFIRE)

extern unsigned char path[];
extern unsigned short keystatus, keystatus2, keystatus3, oldkey; //最新のボタン状態と前回のボタン状態
unsigned char *fileselect(void);
void init_buttons(void);
void keycheck(void);
void disperror(unsigned char *s, FRESULT fr);
void wait60thsec(unsigned short n);

// putbmpmn用サンプルデータ
const unsigned char Akabeibmp[14 * 13] ={
		0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 0,
		0, 0, 0, 8, 8, 2, 2, 2, 2, 8, 8, 0, 0, 0,
		0, 0, 7, 8, 8, 7, 2, 2, 7, 8, 8, 7, 0, 0,
		0, 2, 7, 7, 7, 7, 2, 2, 7, 7, 7, 7, 2, 0,
		0, 2, 7, 7, 7, 7, 2, 2, 7, 7, 7, 7, 2, 0,
		2, 2, 2, 7, 7, 2, 2, 2, 2, 7, 7, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		2, 2, 0, 2, 2, 2, 0, 0, 2, 2, 2, 0, 2, 2,
		2, 0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 2,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void main(void)
{
	stdio_init_all();

	// 液晶用ポート設定
	// Enable SPI at 32 MHz and connect to GPIOs
	spi_init(LCD_SPICH, 32000 * 1000);
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

//	init_textgraph(HORIZONTAL); //液晶初期化、テキスト利用開始
	init_textgraph(VERTICAL); //液晶初期化、テキスト利用開始

	init_buttons(); //ボタンのI/O初期化

/*
	// putbmpmn、clrbmpmnのテスト
	int i=-20,j=-20,dx=1,dy=1;
	while(1){
		g_putbmpmn(i,j,14,13,Akabeibmp);
		sleep_ms(20);
		g_clrbmpmn(i,j,14,13);
		i+=dx;if(i<-20 || i>X_RES+5) dx=-dx;
		j+=dy;if(j<-20 || j>Y_RES+5) dy=-dy;
	}
*/
/*
	// テキスト表示テスト
	int i;
	while(1){
		set_lcdalign(HORIZONTAL);
		for(i=0;i<30*100;i++){
			setcursorcolor(rand()%7+1);
			printchar('A'+rand()%26);
		}
		set_lcdalign(VERTICAL);
		for(i=0;i<40*75;i++){
			setcursorcolor(rand()%7+1);
			printchar('A'+rand()%26);
		}
	}
*/
	// SDカード利用初期設定
	FATFS FatFs; /* FatFs work area needed for each volume */
	FIL Fil;	 /* File object needed for each open file */
	FRESULT fr;
	DIR dj;		 /* Directory object */
	FILINFO fno; /* File information */
	unsigned char *fname;
	unsigned char linebuf[256];

	fr = f_mount(&FatFs, "", 0); /* Give a work area to the default drive */
	if (fr) disperror("SD Mount Error.", fr);

	path[0] = '/';
	path[1] = 0;
	while (1){
		fname = fileselect();
		fr = f_open(&Fil, fname, FA_READ);
		if (fr) disperror("File open Error.", fr);
		while (!f_eof(&Fil)){
			if (f_gets(linebuf, 256, &Fil) == NULL)
				disperror("Read Line Error.", 0);
			printstr(linebuf);
			keycheck();
			if(keystatus2 & KEYFIRE) break;
		}
		fr = f_close(&Fil);
		setcursorcolor(2);
		printstr("Push FIRE Button\n");
		while (1){
			keycheck();
			if (keystatus2 & KEYFIRE) break;
			wait60thsec(1);
		}
	}
}
