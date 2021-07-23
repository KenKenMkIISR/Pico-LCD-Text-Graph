// LCD ライブラリ、FatFs テスト
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "LCDdriver.h"
#include "graphlib.h"
#include "ff.h"

#define MAXFILE 256

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

unsigned char path[256];
unsigned char filenames[MAXFILE][13];
unsigned short keystatus, keystatus2, keystatus3, oldkey; //最新のボタン状態と前回のボタン状態
int keycountUP, keycountLEFT, keycountRIGHT, keycountDOWN, keycountSTART, keycountFIRE;
int filenum, dirnum;

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

void wait60thsec(unsigned short n){
	// 60分のn秒ウェイト
	uint64_t t = to_us_since_boot(get_absolute_time()) % 16667;
	sleep_us(16667 * n - t);
}
void keycheck(void){
	//ボタン状態読み取り
	//keystatus :現在押されているボタンに対応するビットを1にする
	//keystatus2:前回押されていなくて、今回押されたボタンに対応するビットを1にする
	//keystatus3:前回押されていて、今回離されたボタンに対応するビットを1にする
	//keycountXX:XXボタンが押された状態で連続して読み込まれた回数
	oldkey = keystatus;
	keystatus = ~gpio_get_all() & KEYSMASK;
	keystatus2 = keystatus & ~oldkey; //前回ボタンを離していたかチェック
	keystatus3 = ~keystatus & oldkey; //前回ボタンを押していたかチェック
	if (keystatus & KEYUP) keycountUP++;
	else keycountUP = 0;
	if (keystatus & KEYLEFT) keycountLEFT++;
	else keycountLEFT = 0;
	if (keystatus & KEYRIGHT) keycountRIGHT++;
	else keycountRIGHT = 0;
	if (keystatus & KEYDOWN) keycountDOWN++;
	else keycountDOWN = 0;
	if (keystatus & KEYSTART) keycountSTART++;
	else keycountSTART = 0;
	if (keystatus & KEYFIRE) keycountFIRE++;
	else keycountFIRE = 0;
}

// エラー番号を表示してい停止
void disperror(unsigned char *s, FRESULT fr){
	printstr(s);
	printstr(" FRESULT:");
	printnum(fr);
	while (1) ;
}

// filenames配列のn番目のファイルから一覧表示
void dispfiles(int n){
	int i, j;
	//    cls();
	setcursor(0, 0, 4);
	printstr("Select file & Push FIRE Button\n");
	for (i = 0; i < 29 * 3; i++){
		if (i % 3 == 0) printchar(' ');
		if (i + n < dirnum){
			// ディレクトリ
			setcursorcolor(6);
			printchar('[');
			printstr(filenames[i + n]);
			printchar(']');
			//13文字まで空白で埋める
			for (j = 11 - strlen(filenames[i + n]); j > 0; j--)
				printchar(' ');
		}
		else if (i + n < filenum){
			// ファイル
			setcursorcolor(7);
			printstr(filenames[i + n]);
			//13文字まで空白で埋める
			for (j = 13 - strlen(filenames[i + n]); j > 0; j--)
				printchar(' ');
		}
		else for (j = 0; j < 13; j++) printchar(' '); //画面最後まで空白で埋める
	}
}

// SDカード内のBASICソースプログラム一覧を表示、選択し
// ファイル名を返す
unsigned char *fileselect(void){
	FRESULT fr;
	DIR dj;		 // Directory object
	FILINFO fno; // File information
	int n;		 // file number
	int top;	 // 画面先頭のファイル番号
	int x, y;
	unsigned char *p, *p2;
	unsigned short key;

	while (1){
		filenum = 0;
		dirnum = 0;
		fr = f_opendir(&dj, path);
		if (fr) disperror("Open directory Error.", fr);
		if (path[1]){ // not root directory
			// 親ディレクトリ
			strcpy(filenames[filenum], "..");
			filenum++;
			dirnum++;
		}
		while (1){
			fr = f_readdir(&dj, &fno); // Read a directory item
			if (fr) disperror("Read directory Error.", fr);
			if (fno.fname[0] == 0) break;
			if (fno.fattrib & AM_DIR){ // It is a directory
				strcpy(filenames[filenum], fno.fname);
				filenum++;
				dirnum++;
			}
		}
		f_closedir(&dj);

		fr = f_findfirst(&dj, &fno, path, "*.*"); // 全てのファイル
//		fr = f_findfirst(&dj, &fno, path, "*.BAS"); // BASICソースファイル
		if (fr) disperror("Findfirst Error.", fr);
		while (fr == FR_OK && fno.fname[0]){ // Repeat while an item is found
			strcpy(filenames[filenum], fno.fname);
			filenum++;
			if (filenum >= MAXFILE) break;
			fr = f_findnext(&dj, &fno); // Search for next item
			if (fr) disperror("Findnext Error.", fr);
		}
		f_closedir(&dj);
		if (filenum == 0) return NULL;
		n = 0;
		top = 0;
		x = 0;
		y = 0;
		dispfiles(top); //ファイル番号topから一覧を画面表示
		do{
			setcursor(x * 13, y + 1, 5);
			printchar(0x1c); // right arrow
			wait60thsec(2);
			setcursor(x * 13, y + 1, 5);
			printchar(' ');
			keycheck();
			key = keystatus2;
			// 30回以上同じボタンを押し続けていればリピートさせる
			if (keycountUP > 30) key |= KEYUP;
			if (keycountLEFT > 30) key |= KEYLEFT;
			if (keycountRIGHT > 30) key |= KEYRIGHT;
			if (keycountDOWN > 30) key |= KEYDOWN;
			switch (key){
			case KEYUP:
				if (y > 0){
					n -= 3;
					y--;
				}
				else if (top >= 3){
					n -= 3;
					top -= 3;
					dispfiles(top);
				}
				break;
			case KEYDOWN:
				if (n - x + 3 < filenum){
					n += 3;
					if (n >= filenum){
						n -= x;
						x = 0;
					}
					if (y < 28){
						y++;
					}
					else{
						top += 3;
						dispfiles(top);
					}
				}
				break;
			case KEYLEFT:
				if (x > 0){
					n--;
					x--;
				}
				break;
			case KEYRIGHT:
				if (x < 2 && n + 1 < filenum){
					n++;
					x++;
				}
				break;
			}
		} while (keystatus2 != KEYFIRE);
		if (n < dirnum){
			// ディレクトリの場合
			if (filenames[n][0] == '.'){
				// 親ディレクトリの場合、pathから現ディレクトリを削除
				for (p = path; *p; p++) ;
				for (p -= 2; *p != '/'; p--) ;
				*(p + 1) = 0;
			}
			else{
				// pathにディレクトリ名を結合して最後に'/'を付加
				for (p = path; *p; p++) ;
				for (p2 = filenames[n]; *p2;) *p++ = *p2++;
				*p++ = '/';
				*p = 0;
			}
			fr = f_chdir(path); //Change Directory
			if (fr) disperror("Change directory Error.", fr);
		}
		else break; // ファイル選択
	}
	cls();
	setcursorcolor(7);
	return filenames[n]; //選択したファイル名へのポインタを返す
}

void main(void)
{
	stdio_init_all();

	// 液晶用ポート設定
	// Enable SPI at 40 MHz and connect to GPIOs
	spi_init(LCD_SPICH, 40000 * 1000);
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

	// ボタン用GPIO設定
	gpio_init_mask(KEYSMASK);
	gpio_set_dir_in_masked(KEYSMASK);
	gpio_pull_up(GPIO_KEYUP);
	gpio_pull_up(GPIO_KEYLEFT);
	gpio_pull_up(GPIO_KEYRIGHT);
	gpio_pull_up(GPIO_KEYDOWN);
	gpio_pull_up(GPIO_KEYSTART);
	gpio_pull_up(GPIO_KEYFIRE);
	keystatus = KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT | KEYSTART | KEYFIRE; //初期化

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
