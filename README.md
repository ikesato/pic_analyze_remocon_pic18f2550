概要
----
- PIC18F2550 を使った CDC Basic Demo
- 参考
 http://yamato-pic.blogspot.jp/2010/11/pic18f2550-cdc-basic-demo.html
- Microchip usb framework の USB/Device - CDC - Basic Demo をビルドして実行しただけ
- 一箇所だけ変更
 HardwareProfile - PICDEM FSUSB.h
 の下をコメントにしただけ。
   //#define PROGRAMMABLE_WITH_USB_MCHPUSB_BOOTLOADER
- PICKIT2 で焼く
- コンパイルスイッチは PICDEM FSUSB
- この環境は PIC18F4550 用だったけど動いた
- で PIC18F2550 用に書きなおしてビルドしただけ
- CPU クロックは 48MHz


Hardwares
---------
- define input/output
 RA0 LED
 RB3 赤外線
 RB4 sw2
 RB5 sw3

- 赤外線受信モジュール
 PL-IRM2161-C438



Mac で USBシリアル通信する方法
------------------------------
- cu を使う方法
 sudo cu -l /dev/tty.usbmodemfa131 -s 19200

- screen を使う方法
 screen -c /dev/null /dev/tty.usbmodemfa131 119200
