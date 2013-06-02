概要
====
家庭用リモコン信号を受信しコードを解析する。  
また解析したコードを送信し解析したコードのテストを可能とする。  

- マイコン : PIC18F2550
- USB接続 (CDC クラスを使用)
- CDC Basic Demo をベースに赤外線LEDの送受信し、データをシリアル通信で送受信
- 赤外通信 38KHz
- 解析できるコードは家電協フォーマット、ソニーフォーマット
- 受信と解析  
 リモコン -> 赤外線受光部 -> マイコン -> PCでコードを解析
- 送信  
 PC -> マイコン -> 赤外LEDで送信
- フリスクケースに収まるサイズで作成


回路図
=====
![回路図](/doc/analyze_remocon/schematic.png)

配線
====
![配線](/doc/analyze_remocon/analyze_remocon.png)


部品
====
- LED x 2
- 赤外線LED x 2
 型番 : TLN105B
- 赤外線受光モジュール  
 型番 : PL-IRM2161-C438
- FET  
 型番 : 2SK2201
- 水晶振動子 20MHz
- マイコン : PIC18F2550 DIP 28ピン
- USBコネクタ変換基板  
 型番 : サンハヤト CK-37  
 ケースに収めるため一部カット加工
- スイッチ （モーメンタリ） x 2
- コンデンサ 0.1uF x 2
- 抵抗  
 1KΩ  (チップ抵抗) x 2  
 1Ω  x 1  
 10KΩ  x 2  


使用方法
========

1. 接続  
 マイコン部とPCをUSBケーブルで接続する。  
 USBから電源供給をおこなう。  
2. PCでリモコン解析用ツールを起動する  
 `$ ruby serial/ranalyzer.rb /dev/tty.usbXXXXXX`  
 /dev/tty.usbXXXXXX にはシリアルポートデバイスを指定
 Mac の場合は ls /dev/tty.usb* に存在するデバイス
3. 家庭用リモコンをマイコン部に向けて何かボタンを押す
4. 改正気結果がコンソールに表示
5. 解析結果の行をコピー＆ペーストで、そのコードの送信テストが可能


ディレクトリ構成
================

- Firmware ディレクトリ  
 ファームウェアの全ソースコード
- inf ディレクトリ  
 Windows の場合の USBドライバ （動作未確認）
- serial ディレクトリ  
 リモコン解析ツール  
 ruby 製です。1.9.x で作成してますが、たぶん 1.8.x でも動くはず。
- hex ファイル  
 Firmware/MPLAB.X/dist/PICDEM_FSUSB/production/MPLAB.X.production.hex
