概要
----
- PIC18F2550
- CDC Basic Demo をベースに赤外線LEDの送受信し、データをシリアル通信で送受信
- CPU クロック 48MHz
- 赤外通信 38KHz


Hardwares
---------
- LED1
- LED2
- 赤外線 LED
- sw1 (for reset)
- sw2 (for user)
- 赤外線受信モジュール
 PL-IRM2161-C438

使用方法
--------

38KHz
-----
38KHz == 26.3157894736842[us] => (デューティー比 1/3 hi:lo=1:2)
48MHz == 0.020833333333333[us/clock] == 0.083333333[us/cycle]

HI または LO のdelay を計算
26.3157894736842 / 0.083333333 = 315.78947368421[cycle]

デューティ比 33% なので
HI:105.263157894737[cycle] == 8.77192982456142[us]
LO:210.526315789474[cycle] == 17.5438596491228[us]

デューティ比 50% の場合 315.78947368421 / 2 = 157.894736842105[cycle]
HI:157.894736842105[cycle] == 13.1578947368421[us]
LO:157.894736842105[cycle] == 13.1578947368421[us]


リモコン解析用シリアル通信ツール ranalyzer.rb の使い方
------------------------------------------------------
$ ruby serial/ranalyzer.rb /dev/tty.usbXXXXXX

- 起動後リモコンを向けて何かリモコンのボタンを押す
- 解析結果がコンソールに表示
- 解析結果の行をコピー＆ペーストでそのコードを赤外線送信する


Mac で 直接USBシリアル通信する方法
----------------------------------
上の ranalyzer.rb が使えないとき、直接シリアル通信して
デバッグする方法

- cu を使う方法
 sudo cu -l /dev/tty.usbmodemfa131 -s 19200

- screen を使う方法
 screen -c /dev/null /dev/tty.usbmodemfa131 119200


format
------
家電協 T:0.425ms 1T=79.69 Leader(H8T  L4T) 0:(H1T L1T) 1:(H1T L3T)
NEC    T:0.560ms 1T=105   Leader(H16T L8T) 0:(H1T L1T) 1:(H1T L3T) bitlen=32,42
SONY   T:0.600ms 1T=112.5 Leader(H4T  L1T) 0:(H1T L1T) 1:(H2T L1T) bitlen=12,15,20
JVC    T:0.517ms 1T=97    Leader(H16T L8T) 0:(H1T L1T) 1:(H1T L3T) bitlen=16,32


timer0
------
PIC タイマー0 について
プリスケーラ 1/64 を使用。
1:64 48MHz/4=12MHz => 0.08333[us/cycle] => *64 => 5.33333us
タイマー単位の 1 は 5.33333[us]


datas
-----
- Aiwa リモコン => NECフォーマット  
    NEC T=0.559[ms] 42bit [7b 80 f0 03 fc 03, REP8T] # 電源  
    NEC T=0.561[ms] 42bit [7b 80 f0 03 fc 03, REP8T, REP8T, REP8T, REP8T] # 電源長押  
    NEC T=0.558[ms] 42bit [7b 80 f0 3b c4 03, REP8T] # テレビ/ビデオ  
    NEC T=0.558[ms] 42bit [7b 80 f0 0b f5 02, REP8T] # オフタイマー  
    NEC T=0.559[ms] 42bit [7b 80 f0 0f f1 02, REP8T] # おやすみタイマー  
    NEC T=0.559[ms] 42bit [7b 80 f0 f3 0d 02, REP8T] # ゲーム  
    NEC T=0.559[ms] 42bit [7b 80 f0 07 f8 03, REP8T] # 1ch  
    NEC T=0.559[ms] 42bit [7b 80 f0 0b f4 03, REP8T] # 2ch  
    NEC T=0.559[ms] 42bit [7b 80 f0 0f f0 03, REP8T] # 3ch  
    NEC T=0.559[ms] 42bit [7b 80 f0 13 ec 03, REP8T] # 4ch  
    NEC T=0.559[ms] 42bit [7b 80 f0 17 e8 03, REP8T] # 5ch  
    NEC T=0.560[ms] 42bit [7b 80 f0 1b e4 03, REP8T] # 6ch  
    NEC T=0.560[ms] 42bit [7b 80 f0 1f e0 03, REP8T] # 7ch  
    NEC T=0.561[ms] 42bit [7b 80 f0 23 dc 03, REP8T] # 8ch  
    NEC T=0.559[ms] 42bit [7b 80 f0 27 d8 03, REP8T] # 9ch  
    NEC T=0.559[ms] 42bit [7b 80 f0 2b d4 03, REP8T] # 10ch  
    NEC T=0.560[ms] 42bit [7b 80 f0 2f d0 03, REP8T] # 11ch  
    NEC T=0.559[ms] 42bit [7b 80 f0 33 cc 03, REP8T] # 12ch  
    NEC T=0.560[ms] 42bit [7b 80 f0 e7 1a 01, REP8T] # ミッドナイトシアター  
    NEC T=0.560[ms] 42bit [7b 80 f0 4f b0 03, REP8T] # 音多モード  
    NEC T=0.559[ms] 42bit [7b 80 f0 8f 71 02, REP8T] # ピクチャーメニュー  
    NEC T=0.559[ms] 42bit [7b 80 f0 3f c1 02, REP8T] # 表示切り替え  
    NEC T=0.559[ms] 42bit [7b 80 f0 2b d5 02, REP8T] # ミュート  
    NEC T=0.560[ms] 42bit [7b 80 f0 6f 90 03, REP8T] # メニュー  
    NEC T=0.559[ms] 42bit [7b 80 f0 ab 55 02, REP8T] # 決定  
    NEC T=0.559[ms] 42bit [7b 80 f0 03 fd 02, REP8T] # チャンネルup  
    NEC T=0.559[ms] 42bit [7b 80 f0 07 f9 02, REP8T] # チャンネルdown  
    NEC T=0.559[ms] 42bit [7b 80 f0 63 9c 03, REP8T] # 音量up  
    NEC T=0.559[ms] 42bit [7b 80 f0 67 98 03, REP8T] # 音量down



- Victor テレビリモコン => JVCフォーマット  
    JVC T=0.516[ms] CYCLE=44.5[ms] 16bit [03 17, 03 17] # 電源  
    JVC T=0.517[ms] CYCLE=44.6[ms] 16bit [03 00, 03 00] # 音声切換  
    JVC T=0.517[ms] CYCLE=44.7[ms] 16bit [03 03, 03 03] # オフタイマー  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 10, 03 10] # アナログ  
    JVC T=0.517[ms] CYCLE=44.7[ms] 16bit [0f 15, 0f 15] # デジタル  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 0c, 03 0c] # BS  
    JVC T=0.517[ms] CYCLE=44.7[ms] 16bit [0f 14, 0f 14] # CS  
    JVC T=0.515[ms] CYCLE=44.6[ms] 16bit [03 21, 03 21] # 1ch  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 22, 03 22] # 2ch  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 23, 03 23] # 3ch  
    JVC T=0.515[ms] CYCLE=44.6[ms] 16bit [03 24, 03 24] # 4ch  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 25, 03 25] # 5ch  
    JVC T=0.515[ms] CYCLE=44.6[ms] 16bit [03 26, 03 26] # 6ch  
    JVC T=0.517[ms] CYCLE=44.6[ms] 16bit [03 27, 03 27] # 7ch  
    JVC T=0.515[ms] CYCLE=44.6[ms] 16bit [03 28, 03 28] # 8ch  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 29, 03 29] # 9ch  
    JVC T=0.517[ms] CYCLE=44.6[ms] 16bit [03 2a, 03 2a] # 10ch  
    JVC T=0.516[ms] CYCLE=44.5[ms] 16bit [03 2b, 03 2b] # 11ch  
    JVC T=0.517[ms] CYCLE=44.6[ms] 16bit [03 2c, 03 2c] # 12ch  
    JVC T=0.518[ms] CYCLE=44.6[ms] 16bit [03 1e, 03 1e] # 音量up  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 1f, 03 1f] # 音量down  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 1e, 03 1e, 03 1e, 03 1e, 03 1e, 03 1e, 03 1e, 03 1e, 03 1e] # 音量up長押し  
    JVC T=0.517[ms] CYCLE=44.6[ms] 16bit [03 19, 03 19] # chanel up  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 18, 03 18] # chanel down  
    JVC T=0.517[ms] CYCLE=44.6[ms] 16bit [0f 7d, 0f 7d] # 番組表  
    JVC T=0.517[ms] CYCLE=44.6[ms] 16bit [03 e6, 03 e6] # 戻る  
    JVC T=0.517[ms] CYCLE=44.7[ms] 16bit [03 7a, 03 7a] # ホーム/メニュー  
    JVC T=0.516[ms] CYCLE=44.7[ms] 16bit [03 04, 03 04] # 画面表示  
    JVC T=0.517[ms] CYCLE=44.7[ms] 16bit [03 7c, 03 7c] # 上  
    JVC T=0.517[ms] CYCLE=44.7[ms] 16bit [03 5a, 03 5a] # 右  
    JVC T=0.517[ms] CYCLE=44.5[ms] 16bit [03 ec, 03 ec] # 下  
    JVC T=0.517[ms] CYCLE=44.6[ms] 16bit [03 5b, 03 5b] # 左  
    JVC T=0.515[ms] CYCLE=44.5[ms] 16bit [03 0a, 03 0a] # 決定  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 13, 03 13] # 入力切り替え  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [03 1c, 03 1c] # ミュート  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [0f 1c, 0f 1c] # 10キー  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [23 a0, 23 a0] # d連動データ  
    JVC T=0.517[ms] CYCLE=44.6[ms] 16bit [23 c3, 23 c3] # 青  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [23 c0, 23 c0] # 赤  
    JVC T=0.516[ms] CYCLE=44.6[ms] 16bit [23 c1, 23 c1] # 緑  
    JVC T=0.516[ms] CYCLE=44.7[ms] 16bit [23 c2, 23 c2] # 黃  
    JVC T=0.517[ms] CYCLE=44.7[ms] 16bit [03 93, 03 93] # ワイド切換  
    JVC T=0.517[ms] CYCLE=44.6[ms] 16bit [0f 7c, 0f 7c] # 番組説明  


- SONY すごろく => SONY フォーマット  
    SONY T=0.586[ms] 20bit [15 ad 0f, 15 ad 0f, 15 ad 0f] # 電源  
    SONY T=0.586[ms] 20bit [16 ad 0f, 16 ad 0f, 16 ad 0f] # 開閉  
    SONY T=0.587[ms] 20bit [64 ad 0f, 64 ad 0f, 64 ad 0f] # 音声切換  
    SONY T=0.587[ms] 20bit [00 ad 0f, 00 ad 0f, 00 ad 0f] # 1ch  
    SONY T=0.588[ms] 20bit [01 ad 0f, 01 ad 0f, 01 ad 0f] # 2ch  
    SONY T=0.587[ms] 20bit [02 ad 0f, 02 ad 0f, 02 ad 0f] # 3ch  
    SONY T=0.587[ms] 20bit [03 ad 0f, 03 ad 0f, 03 ad 0f] # 4ch  
    SONY T=0.586[ms] 20bit [04 ad 0f, 04 ad 0f, 04 ad 0f] # 5ch  
    SONY T=0.587[ms] 20bit [05 ad 0f, 05 ad 0f, 05 ad 0f] # 6ch  
    SONY T=0.587[ms] 20bit [06 ad 0f, 06 ad 0f, 06 ad 0f] # 7ch  
    SONY T=0.588[ms] 20bit [07 ad 0f, 07 ad 0f, 07 ad 0f] # 8ch  
    SONY T=0.588[ms] 20bit [08 ad 0f, 08 ad 0f, 08 ad 0f] # 9ch  
    SONY T=0.586[ms] 20bit [09 ad 0f, 09 ad 0f, 09 ad 0f] # 10ch  
    SONY T=0.588[ms] 20bit [0a ad 0f, 0a ad 0f, 0a ad 0f] # 11ch  
    SONY T=0.586[ms] 20bit [0d ad 0f, 0d ad 0f, 0d ad 0f] # 12ch  
    SONY T=0.586[ms] 20bit [6c bd 00, 6c bd 00, 6c bd 00] # アナログ  
    SONY T=0.586[ms] 20bit [6b bd 00, 6b bd 00, 6b bd 00] # デジタル  
    SONY T=0.588[ms] 20bit [6a bd 00, 6a bd 00, 6a bd 00] # BS  
    SONY T=0.587[ms] 20bit [6e bd 00, 6e bd 00, 6e bd 00] # CS  
    SONY T=0.588[ms] 20bit [61 bd 00, 61 bd 00, 61 bd 00] # 10キー  
    SONY T=0.587[ms] 20bit [13 ad 0f, 13 ad 0f, 13 ad 0f] # channel up  
    SONY T=0.587[ms] 20bit [14 ad 0f, 14 ad 0f, 14 ad 0f] # channel down  
    SONY T=0.587[ms] 20bit [13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f] # channel up 長押し  
    SONY T=0.586[ms] 20bit [50 bd 00, 50 bd 00, 50 bd 00] # HDD/BD  
    SONY T=0.588[ms] 20bit [62 bd 00, 62 bd 00, 62 bd 00] # d連動データ  
    SONY T=0.587[ms] 20bit [66 bd 00, 66 bd 00, 66 bd 00] # 青  
    SONY T=0.586[ms] 20bit [67 bd 00, 67 bd 00, 67 bd 00] # 赤  
    SONY T=0.588[ms] 20bit [68 bd 00, 68 bd 00, 68 bd 00] # 緑  
    SONY T=0.587[ms] 20bit [69 bd 00, 69 bd 00, 69 bd 00] # 黃  
    SONY T=0.587[ms] 20bit [1b ad 0f, 1b ad 0f, 1b ad 0f] # 3d  
    SONY T=0.587[ms] 20bit [1a ad 0f, 1a ad 0f, 1a ad 0f] # アクトビラ  
    SONY T=0.586[ms] 20bit [16 bd 00, 16 bd 00, 16 bd 00] # 番組表  
    SONY T=0.588[ms] 20bit [0e ad 0f, 0e ad 0f, 0e ad 0f] # 戻る  
    SONY T=0.586[ms] 20bit [53 ad 0f, 53 ad 0f, 53 ad 0f] # ホーム/メニュー  
    SONY T=0.588[ms] 20bit [17 bd 00, 17 bd 00, 17 bd 00] # オプション  
    SONY T=0.586[ms] 20bit [54 ad 0f, 54 ad 0f, 54 ad 0f] # 画面表示  
    SONY T=0.588[ms] 20bit [5a ad 0f, 5a ad 0f, 5a ad 0f] # リンクメニュー  
    SONY T=0.588[ms] 20bit [79 ad 0f, 79 ad 0f, 79 ad 0f] # 上  
    SONY T=0.587[ms] 20bit [7c ad 0f, 7c ad 0f, 7c ad 0f] # 右  
    SONY T=0.587[ms] 20bit [7a ad 0f, 7a ad 0f, 7a ad 0f] # 下  
    SONY T=0.588[ms] 20bit [7b ad 0f, 7b ad 0f, 7b ad 0f] # 左  
    SONY T=0.588[ms] 20bit [0b ad 0f, 0b ad 0f, 0b ad 0f] # 決定  
    SONY T=0.586[ms] 20bit [18 bd 00, 18 bd 00, 18 bd 00] # シアター  
    SONY T=0.587[ms] 20bit [10 bd 00, 10 bd 00, 10 bd 00] # 録画リスト  
    SONY T=0.588[ms] 20bit [30 ad 0f, 30 ad 0f, 30 ad 0f] # |<< 前  
    SONY T=0.588[ms] 20bit [5c ad 0f, 5c ad 0f, 5c ad 0f] # <-.  
    SONY T=0.587[ms] 20bit [14 bd 00, 14 bd 00, 14 bd 00] # .->  
    SONY T=0.588[ms] 20bit [31 ad 0f, 31 ad 0f, 31 ad 0f] # >>| 次  
    SONY T=0.588[ms] 20bit [33 ad 0f, 33 ad 0f, 33 ad 0f] # << 巻き戻し  
    SONY T=0.587[ms] 20bit [32 ad 0f, 32 ad 0f, 32 ad 0f] # >  play  
    SONY T=0.588[ms] 20bit [34 ad 0f, 34 ad 0f, 34 ad 0f] # >> 早送り  
    SONY T=0.587[ms] 20bit [19 ad 0f, 19 ad 0f, 19 ad 0f] # 録画  
    SONY T=0.587[ms] 20bit [39 ad 0f, 39 ad 0f, 39 ad 0f] # || 一時停止  
    SONY T=0.586[ms] 20bit [38 ad 0f, 38 ad 0f, 38 ad 0f] # ■ 停止  
    SONY T=0.588[ms] 20bit [65 ad 0f, 65 ad 0f, 65 ad 0f] # ワイド切換  
    SONY T=0.587[ms] 20bit [63 ad 0f, 63 ad 0f, 63 ad 0f] # 字幕  
    SONY T=0.588[ms] 20bit [12 ad 0f, 12 ad 0f, 12 ad 0f] # ２画面表示  
    SONY T=0.588[ms] 20bit [15 bd 00, 15 bd 00, 15 bd 00] # 番組説明  

* ダイキンエアコン電源ボタン
H84,L76,H83,L76,H83,L77,H83,L76,H83,L76,H84,L4748,H652,L321,H83,L239,H83,L77,H83,L76,H83,L76,H84,L238,H83,L76,H84,L76,H83,L76,H75,L76,H83,L238,H84,L76,H83,L238,H84,L238,H84,L76,H83,L239,H83,L238,H84,L238,H84,L238,H84,L238,H84,L75,H84,L76,H83,L239,H83,L76,H83,L76,H84,L76,H83,L76,H84,L76,H83,L76,H83,L77,H83,L76,H83,L76,H84,L76,H85,L236,H84,L76,H83,L239,H83,L76,H83,L77,H83,L76,H83,L239,H83,L239,H83,L76,H83,L76,H84,L76,H86,L72,H84,L76,H83,L76,H83,L68,H83,L76,H83,L76,H83,L77,H83,L76,H83,L76,H84,L76,H83,L239,H83,L76,H83,L76,H84,L238,H84,L238,H83,L239,H83,L76,H84,L238,H84,L238,H83,L239,H83,L239,H83,L6502,H652,L320,H83,L238,H83,L76,H76,L75,H83,L76,H84,L238,H83,L76,H84,L76,H83,L76,H83,L76,H84,L238,H84,L76,H83,L239,H83,L238,H77,L73,H83,L238,H84,L238,H84,L238,H84,L238,H84,L238,H83,L76,H84,L76,H83,L238,H78,L72,H83,L76,H84,L76,H83,L78,H80,L77,H83,L76,H83,L76,H84,L76,H83,L76,H84,L75,H84,L76,H83,L239,H83,L76,H83,L77,H78,L71,H83,L76,H84,L231,H81,L76,H83,L76,H83,L76,H84,L76,H83,L76,H84,L75,H84,L76,H83,L76,H84,L76,H83,L76,H83,L77,H79,L70,H83,L76,H84,L76,H83,L70,H80,L76,H83,L76,H83,L76,H84,L76,H83,L238,H84,L76,H83,L239,H83,L76,H84,L238,H84,L75,H84,L6501,H653,L319,H78,L235,H83,L76,H84,L68,H81,L76,H83,L239,H83,L76,H84,L76,H83,L76,H83,L76,H84,L238,H83,L77,H83,L238,H84,L238,H84,L69,H80,L238,H84,L238,H84,L238,H84,L238,H83,L239,H83,L76,H80,L70,H83,L238,H84,L70,H79,L76,H84,L76,H83,L76,H83,L76,H84,L76,H83,L76,H84,L76,H83,L76,H83,L77,H83,L76,H83,L76,H91,L69,H83,L76,H84,L75,H84,L71,H78,L76,H84,L76,H83,L76,H83,L76,H84,L76,H83,L238,H84,L76,H83,L77,H83,L238,H92,L68,H83,L76,H84,L75,H84,L238,H84,L76,H83,L238,H84,L238,H84,L76,H83,L76,H84,L76,H83,L76,H83,L76,H93,L67,H83,L76,H84,L76,H83,L73,H76,L76,H84,L76,H83,L76,H84,L76,H83,L76,H84,L75,H84,L238,H84,L76,H83,L238,H84,L67,H83,L76,H84,L76,H83,L84,H76,L76,H83,L76,H83,L76,H84,L76,H83,L76,H84,L76,H83,L76,H83,L76,H84,L76,H83,L76,H83,L77,H83,L76,H83,L76,H84,L247,H75,L238,H83,L76,H84,L76,H83,L76,H84,L76,H83,L76,H83,L77,H83,L76,H83,L76,H84,L76,H83,L76,H84,L238,H83,L239,H83,L76,H83,L77,H83,L76,H83,L76,H83,L76,H83,L76,H83,L76,H82,L76,H82,L76,H82,L76,H83,L76,H83,L76,H83,L76,H83,L76,H73,L76,H82,L76,H82,L76,H74,L238,H83,L76,H83,L77,H83,L75,H84,L75,H84,L75,H84,L239,H83,L238,H83,L75,H84,L76,H82,L76,H82,L76,H75,L74,H83,L76,H83,L76,H83,L239,H84,L75,H84,L75,H84,L75,H83,L76,H83,L76,H83,L77,H82,L76,H84,L75,H84,L238,H87,L73,H83,L238,H83,L77,H82,L239,H84,L75,H84,L238,H84,L238,H83,L60000
