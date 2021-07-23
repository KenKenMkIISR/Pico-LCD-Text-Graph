# LCD text and graphic library for Raspberry Pi Pico with SD card
ラズベリーパイPicoで液晶にテキストおよびグラフィックを描画するためのライブラリ。対応液晶はILI9341搭載の320x240ドット、SPI接続のもの。  
ChaN氏のFatFsを利用してSDカードアクセスも可能。Long File Nameには非対応。  
サンプルプログラムではSDカードのファイル一覧を表示し、ボタンで選択してテキストとして表示。  

## 接続
液晶  
CS ----- GP13  
DC ----- GP10  
RESET -- GP11  
MOSI --- GP15  
MISO --- GP12  
SCK ---- GP14  
  
SDカード  
CS ------ GP17  
DI/MOSI - GP19  
DO/MISO - GP16  
SCLK ---- GP18  
  
ボタン（サンプルプログラム用）  
UP ------ GP0  
LEFT ---- GP1  
RIGHT --- GP2  
DOWN ---- GP3  
START --- GP4  
FIRE ---- GP5  
  
## 使い方
main.cの初期化部分を参照してください。  
各関数についてはgraphlib.hを参照してください。  
  
SDカード関連はChaN氏のサイトを参照願います。  
http://elm-chan.org/fsw/ff/00index_e.html