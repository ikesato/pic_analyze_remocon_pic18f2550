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
    nec T=0.559[ms] 42bit [7b 80 f0 03 fc 03, REP8T] # 電源 
    nec T=0.561[ms] 42bit [7b 80 f0 03 fc 03, REP8T, REP8T, REP8T, REP8T] # 電源長押 
    nec T=0.558[ms] 42bit [7b 80 f0 3b c4 03, REP8T] # テレビ/ビデオ
    nec T=0.558[ms] 42bit [7b 80 f0 0b f5 02, REP8T] # オフタイマー
    nec T=0.559[ms] 42bit [7b 80 f0 0f f1 02, REP8T] # おやすみタイマー
    nec T=0.559[ms] 42bit [7b 80 f0 f3 0d 02, REP8T] # ゲーム
    nec T=0.559[ms] 42bit [7b 80 f0 07 f8 03, REP8T] # 1ch
    nec T=0.559[ms] 42bit [7b 80 f0 0b f4 03, REP8T] # 2ch
    nec T=0.559[ms] 42bit [7b 80 f0 0f f0 03, REP8T] # 3ch
    nec T=0.559[ms] 42bit [7b 80 f0 13 ec 03, REP8T] # 4ch
    nec T=0.559[ms] 42bit [7b 80 f0 17 e8 03, REP8T] # 5ch
    nec T=0.560[ms] 42bit [7b 80 f0 1b e4 03, REP8T] # 6ch
    nec T=0.560[ms] 42bit [7b 80 f0 1f e0 03, REP8T] # 7ch
    nec T=0.561[ms] 42bit [7b 80 f0 23 dc 03, REP8T] # 8ch
    nec T=0.559[ms] 42bit [7b 80 f0 27 d8 03, REP8T] # 9ch
    nec T=0.559[ms] 42bit [7b 80 f0 2b d4 03, REP8T] # 10ch
    nec T=0.560[ms] 42bit [7b 80 f0 2f d0 03, REP8T] # 11ch
    nec T=0.559[ms] 42bit [7b 80 f0 33 cc 03, REP8T] # 12ch
    nec T=0.560[ms] 42bit [7b 80 f0 e7 1a 01, REP8T] # ミッドナイトシアター
    nec T=0.560[ms] 42bit [7b 80 f0 4f b0 03, REP8T] # 音多モード
    nec T=0.559[ms] 42bit [7b 80 f0 8f 71 02, REP8T] # ピクチャーメニュー
    nec T=0.559[ms] 42bit [7b 80 f0 3f c1 02, REP8T] # 表示切り替え
    nec T=0.559[ms] 42bit [7b 80 f0 2b d5 02, REP8T] # ミュート
    nec T=0.560[ms] 42bit [7b 80 f0 6f 90 03, REP8T] # メニュー
    nec T=0.559[ms] 42bit [7b 80 f0 ab 55 02, REP8T] # 決定
    nec T=0.559[ms] 42bit [7b 80 f0 03 fd 02, REP8T] # チャンネルup
    nec T=0.559[ms] 42bit [7b 80 f0 07 f9 02, REP8T] # チャンネルdown
    nec T=0.559[ms] 42bit [7b 80 f0 63 9c 03, REP8T] # 音量up
    nec T=0.559[ms] 42bit [7b 80 f0 67 98 03, REP8T] # 音量down



- Victor テレビリモコン => JVCフォーマット
    jvc T=0.516[ms] CYCLE=44.5[ms] 16bit [03 17, 03 17] # 電源
    jvc T=0.517[ms] CYCLE=44.6[ms] 16bit [03 00, 03 00] # 音声切換
    jvc T=0.517[ms] CYCLE=44.7[ms] 16bit [03 03, 03 03] # オフタイマー
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 10, 03 10] # アナログ
    jvc T=0.517[ms] CYCLE=44.7[ms] 16bit [0f 15, 0f 15] # デジタル
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 0c, 03 0c] # BS
    jvc T=0.517[ms] CYCLE=44.7[ms] 16bit [0f 14, 0f 14] # CS
    jvc T=0.515[ms] CYCLE=44.6[ms] 16bit [03 21, 03 21] # 1ch
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 22, 03 22] # 2ch
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 23, 03 23] # 3ch
    jvc T=0.515[ms] CYCLE=44.6[ms] 16bit [03 24, 03 24] # 4ch
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 25, 03 25] # 5ch
    jvc T=0.515[ms] CYCLE=44.6[ms] 16bit [03 26, 03 26] # 6ch
    jvc T=0.517[ms] CYCLE=44.6[ms] 16bit [03 27, 03 27] # 7ch
    jvc T=0.515[ms] CYCLE=44.6[ms] 16bit [03 28, 03 28] # 8ch
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 29, 03 29] # 9ch
    jvc T=0.517[ms] CYCLE=44.6[ms] 16bit [03 2a, 03 2a] # 10ch
    jvc T=0.516[ms] CYCLE=44.5[ms] 16bit [03 2b, 03 2b] # 11ch
    jvc T=0.517[ms] CYCLE=44.6[ms] 16bit [03 2c, 03 2c] # 12ch
    jvc T=0.518[ms] CYCLE=44.6[ms] 16bit [03 1e, 03 1e] # 音量up
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 1f, 03 1f] # 音量down
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 1e, 03 1e, 03 1e, 03 1e, 03 1e, 03 1e, 03 1e, 03 1e, 03 1e] # 音量up長押し
    jvc T=0.517[ms] CYCLE=44.6[ms] 16bit [03 19, 03 19] # chanel up
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 18, 03 18] # chanel down
    jvc T=0.517[ms] CYCLE=44.6[ms] 16bit [0f 7d, 0f 7d] # 番組表
    jvc T=0.517[ms] CYCLE=44.6[ms] 16bit [03 e6, 03 e6] # 戻る
    jvc T=0.517[ms] CYCLE=44.7[ms] 16bit [03 7a, 03 7a] # ホーム/メニュー
    jvc T=0.516[ms] CYCLE=44.7[ms] 16bit [03 04, 03 04] # 画面表示
    jvc T=0.517[ms] CYCLE=44.7[ms] 16bit [03 7c, 03 7c] # 上
    jvc T=0.517[ms] CYCLE=44.7[ms] 16bit [03 5a, 03 5a] # 右
    jvc T=0.517[ms] CYCLE=44.5[ms] 16bit [03 ec, 03 ec] # 下
    jvc T=0.517[ms] CYCLE=44.6[ms] 16bit [03 5b, 03 5b] # 左
    jvc T=0.515[ms] CYCLE=44.5[ms] 16bit [03 0a, 03 0a] # 決定
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 13, 03 13] # 入力切り替え
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [03 1c, 03 1c] # ミュート
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [0f 1c, 0f 1c] # 10キー
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [23 a0, 23 a0] # d連動データ
    jvc T=0.517[ms] CYCLE=44.6[ms] 16bit [23 c3, 23 c3] # 青
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [23 c0, 23 c0] # 赤
    jvc T=0.516[ms] CYCLE=44.6[ms] 16bit [23 c1, 23 c1] # 緑
    jvc T=0.516[ms] CYCLE=44.7[ms] 16bit [23 c2, 23 c2] # 黃
    jvc T=0.517[ms] CYCLE=44.7[ms] 16bit [03 93, 03 93] # ワイド切換
    jvc T=0.517[ms] CYCLE=44.6[ms] 16bit [0f 7c, 0f 7c] # 番組説明


- Sony すごろく => SONY フォーマット
    sony T=0.586[ms] 20bit [15 ad 0f, 15 ad 0f, 15 ad 0f] # 電源
    sony T=0.586[ms] 20bit [16 ad 0f, 16 ad 0f, 16 ad 0f] # 開閉
    sony T=0.587[ms] 20bit [64 ad 0f, 64 ad 0f, 64 ad 0f] # 音声切換
    sony T=0.587[ms] 20bit [00 ad 0f, 00 ad 0f, 00 ad 0f] # 1ch
    sony T=0.588[ms] 20bit [01 ad 0f, 01 ad 0f, 01 ad 0f] # 2ch
    sony T=0.587[ms] 20bit [02 ad 0f, 02 ad 0f, 02 ad 0f] # 3ch
    sony T=0.587[ms] 20bit [03 ad 0f, 03 ad 0f, 03 ad 0f] # 4ch
    sony T=0.586[ms] 20bit [04 ad 0f, 04 ad 0f, 04 ad 0f] # 5ch
    sony T=0.587[ms] 20bit [05 ad 0f, 05 ad 0f, 05 ad 0f] # 6ch
    sony T=0.587[ms] 20bit [06 ad 0f, 06 ad 0f, 06 ad 0f] # 7ch
    sony T=0.588[ms] 20bit [07 ad 0f, 07 ad 0f, 07 ad 0f] # 8ch
    sony T=0.588[ms] 20bit [08 ad 0f, 08 ad 0f, 08 ad 0f] # 9ch
    sony T=0.586[ms] 20bit [09 ad 0f, 09 ad 0f, 09 ad 0f] # 10ch
    sony T=0.588[ms] 20bit [0a ad 0f, 0a ad 0f, 0a ad 0f] # 11ch
    sony T=0.586[ms] 20bit [0d ad 0f, 0d ad 0f, 0d ad 0f] # 12ch
    sony T=0.586[ms] 20bit [6c bd 00, 6c bd 00, 6c bd 00] # アナログ
    sony T=0.586[ms] 20bit [6b bd 00, 6b bd 00, 6b bd 00] # デジタル
    sony T=0.588[ms] 20bit [6a bd 00, 6a bd 00, 6a bd 00] # BS
    sony T=0.587[ms] 20bit [6e bd 00, 6e bd 00, 6e bd 00] # CS
    sony T=0.588[ms] 20bit [61 bd 00, 61 bd 00, 61 bd 00] # 10キー
    sony T=0.587[ms] 20bit [13 ad 0f, 13 ad 0f, 13 ad 0f] # channel up
    sony T=0.587[ms] 20bit [14 ad 0f, 14 ad 0f, 14 ad 0f] # channel down
    sony T=0.587[ms] 20bit [13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f, 13 ad 0f] # channel up 長押し
    sony T=0.586[ms] 20bit [50 bd 00, 50 bd 00, 50 bd 00] # HDD/BD
    sony T=0.588[ms] 20bit [62 bd 00, 62 bd 00, 62 bd 00] # d連動データ
    sony T=0.587[ms] 20bit [66 bd 00, 66 bd 00, 66 bd 00] # 青
    sony T=0.586[ms] 20bit [67 bd 00, 67 bd 00, 67 bd 00] # 赤
    sony T=0.588[ms] 20bit [68 bd 00, 68 bd 00, 68 bd 00] # 緑
    sony T=0.587[ms] 20bit [69 bd 00, 69 bd 00, 69 bd 00] # 黃
    sony T=0.587[ms] 20bit [1b ad 0f, 1b ad 0f, 1b ad 0f] # 3d
    sony T=0.587[ms] 20bit [1a ad 0f, 1a ad 0f, 1a ad 0f] # アクトビラ
    sony T=0.586[ms] 20bit [16 bd 00, 16 bd 00, 16 bd 00] # 番組表
    sony T=0.588[ms] 20bit [0e ad 0f, 0e ad 0f, 0e ad 0f] # 戻る
    sony T=0.586[ms] 20bit [53 ad 0f, 53 ad 0f, 53 ad 0f] # ホーム/メニュー
    sony T=0.588[ms] 20bit [17 bd 00, 17 bd 00, 17 bd 00] # オプション
    sony T=0.586[ms] 20bit [54 ad 0f, 54 ad 0f, 54 ad 0f] # 画面表示
    sony T=0.588[ms] 20bit [5a ad 0f, 5a ad 0f, 5a ad 0f] # リンクメニュー
    sony T=0.588[ms] 20bit [79 ad 0f, 79 ad 0f, 79 ad 0f] # 上
    sony T=0.587[ms] 20bit [7c ad 0f, 7c ad 0f, 7c ad 0f] # 右
    sony T=0.587[ms] 20bit [7a ad 0f, 7a ad 0f, 7a ad 0f] # 下
    sony T=0.588[ms] 20bit [7b ad 0f, 7b ad 0f, 7b ad 0f] # 左
    sony T=0.588[ms] 20bit [0b ad 0f, 0b ad 0f, 0b ad 0f] # 決定
    sony T=0.586[ms] 20bit [18 bd 00, 18 bd 00, 18 bd 00] # シアター
    sony T=0.587[ms] 20bit [10 bd 00, 10 bd 00, 10 bd 00] # 録画リスト
    sony T=0.588[ms] 20bit [30 ad 0f, 30 ad 0f, 30 ad 0f] # |<< 前
    sony T=0.588[ms] 20bit [5c ad 0f, 5c ad 0f, 5c ad 0f] # <-.
    sony T=0.587[ms] 20bit [14 bd 00, 14 bd 00, 14 bd 00] # .->
    sony T=0.588[ms] 20bit [31 ad 0f, 31 ad 0f, 31 ad 0f] # >>| 次
    sony T=0.588[ms] 20bit [33 ad 0f, 33 ad 0f, 33 ad 0f] # << 巻き戻し
    sony T=0.587[ms] 20bit [32 ad 0f, 32 ad 0f, 32 ad 0f] # >  play
    sony T=0.588[ms] 20bit [34 ad 0f, 34 ad 0f, 34 ad 0f] # >> 早送り
    sony T=0.587[ms] 20bit [19 ad 0f, 19 ad 0f, 19 ad 0f] # 録画
    sony T=0.587[ms] 20bit [39 ad 0f, 39 ad 0f, 39 ad 0f] # || 一時停止
    sony T=0.586[ms] 20bit [38 ad 0f, 38 ad 0f, 38 ad 0f] # ■ 停止
    sony T=0.588[ms] 20bit [65 ad 0f, 65 ad 0f, 65 ad 0f] # ワイド切換
    sony T=0.587[ms] 20bit [63 ad 0f, 63 ad 0f, 63 ad 0f] # 字幕
    sony T=0.588[ms] 20bit [12 ad 0f, 12 ad 0f, 12 ad 0f] # ２画面表示
    sony T=0.588[ms] 20bit [15 bd 00, 15 bd 00, 15 bd 00] # 番組説明




















-- 電源ボタン
H446,L106,H225,L105,H114,L99,H223,L105,H114,L104,H216,L107,H114,L105,H114,L106,H113,L106,H225,L110,H113,L106,H225,L106,H225,L105,H114,L106,H224,L107,H114,L106,H224,L107,H226,L105,H229,L106,H225,L106,H224,L2121,H448,L106,H225,L106,H114,L107,H224,L106,H116,L104,H224,L106,H112,L99,H113,L107,H113,L107,H229,L107,H113,L107,H225,L107,H224,L106,H114,L107,H224,L106,H114,L105,H225,L106,H225,L110,H224,L100,H222,L107,H230,L2112,H448,L106,H225,L107,H113,L106,H224,L101,H110,L106,H224,L107,H103,L107,H113,L106,H113,L106,H225,L110,H113,L106,H225,L106,H224,L106,H113,L107,H224,L106,H113,L107,H224,L107,H224,L105,H230,L105,H224,L107,H224,L60000
H448,L105,H225,L107,H113,L106,H224,L107,H113,L106,H224,L107,H112,L107,H113,L106,H114,L105,H225,L110,H114,L105,H225,L106,H224,L107,H112,L107,H224,L106,H113,L107,H224,L106,H225,L115,H221,L105,H222,L100,H224,L2120,H449,L106,H225,L105,H114,L100,H222,L105,H114,L105,H216,L105,H114,L105,H114,L106,H113,L106,H230,L106,H113,L106,H225,L106,H225,L105,H114,L106,H224,L107,H114,L96,H224,L107,H217,L109,H224,L106,H225,L106,H224,L2119,H448,L106,H225,L106,H114,L107,H224,L106,H109,L102,H224,L106,H113,L106,H114,L107,H112,L107,H225,L111,H113,L107,H225,L107,H224,L106,H114,L105,H225,L106,H114,L105,H225,L106,H224,L107,H230,L111,H219,L105,H234,L60000
H446,L106,H225,L105,H114,L105,H226,L105,H114,L105,H230,L101,H114,L105,H114,L105,H114,L106,H225,L110,H114,L106,H225,L105,H225,L107,H114,L105,H225,L107,H114,L105,H225,L107,H224,L106,H230,L106,H225,L102,H219,L2121,H448,L106,H225,L106,H113,L107,H224,L106,H113,L106,H225,L106,H113,L106,H112,L97,H114,L107,H229,L96,H114,L107,H224,L106,H225,L106,H113,L106,H225,L106,H113,L106,H225,L106,H224,L111,H224,L107,H225,L105,H225,L2119,H448,L106,H225,L107,H114,L106,H224,L107,H113,L101,H221,L107,H112,L107,H104,L106,H113,L106,H224,L111,H113,L106,H224,L107,H224,L107,H112,L107,H224,L107,H113,L107,H224,L107,H225,L105,H230,L106,H224,L107,H224,L60000
H441,L104,H225,L106,H112,L97,H225,L106,H113,L106,H225,L106,H113,L106,H114,L105,H114,L107,H225,L110,H114,L107,H225,L105,H225,L106,H114,L105,H225,L106,H118,L101,H225,L106,H224,L107,H229,L107,H225,L105,H225,L2120,H448,L106,H225,L116,H103,L107,H224,L107,H113,L107,H224,L107,H113,L107,H112,L107,H113,L106,H230,L106,H113,L106,H224,L107,H224,L96,H113,L107,H224,L102,H108,L106,H225,L105,H226,L110,H235,L96,H224,L107,H224,L2119,H447,L107,H219,L102,H114,L105,H225,L96,H114,L105,H225,L106,H114,L105,H114,L105,H114,L106,H225,L110,H114,L106,H225,L105,H225,L107,H114,L105,H226,L105,H113,L107,H222,L100,H224,L106,H230,L106,H225,L105,H225,L60000
H445,L106,H225,L105,H114,L106,H225,L105,H114,L106,H224,L107,H114,L106,H113,L106,H113,L106,H225,L110,H113,L106,H225,L106,H224,L101,H110,L106,H224,L107,H104,L106,H224,L107,H225,L107,H229,L106,H225,L106,H224,L2121,H448,L106,H222,L99,H114,L107,H224,L106,H114,L107,H224,L106,H114,L105,H114,L107,H113,L107,H229,L107,H113,L107,H224,L107,H225,L105,H114,L105,H229,L102,H114,L105,H225,L96,H224,L111,H224,L106,H225,L105,H226,L2118,H448,L106,H225,L107,H113,L106,H224,L107,H112,L107,H224,L106,H113,L107,H113,L106,H114,L105,H225,L110,H114,L105,H225,L106,H224,L107,H112,L107,H224,L106,H113,L107,H224,L106,H225,L105,H230,L105,H224,L107,H224,L60000

- チャンネル上
H445,L106,H225,L101,H221,L105,H114,L105,H114,L106,H225,L105,H114,L106,H113,L107,H113,L106,H224,L111,H113,L106,H224,L107,H224,L106,H113,L107,H224,L97,H112,L106,H225,L103,H219,L105,H235,L101,H225,L106,H224,L2120,H448,L106,H225,L105,H225,L106,H109,L101,H114,L107,H224,L106,H114,L107,H113,L107,H114,L106,H230,L105,H114,L106,H224,L107,H225,L107,H113,L107,H225,L107,H113,L107,H226,L104,H230,L106,H225,L106,H225,L105,H225,L2122,H448,L106,H225,L107,H224,L101,H109,L106,H113,L107,H224,L106,H113,L107,H113,L106,H114,L105,H230,L106,H114,L105,H225,L106,H224,L107,H113,L106,H224,L107,H113,L106,H224,L106,H225,L103,H223,L105,H220,L102,H224,L60000
H447,L106,H224,L107,H217,L104,H113,L107,H114,L105,H225,L107,H114,L105,H114,L106,H113,L106,H225,L110,H113,L106,H225,L106,H225,L105,H114,L106,H224,L107,H114,L99,H222,L107,H220,L102,H230,L105,H225,L106,H225,L2119,H448,L107,H225,L106,H224,L107,H113,L106,H110,L100,H225,L106,H113,L106,H114,L105,H114,L107,H229,L105,H114,L107,H225,L105,H225,L106,H114,L105,H225,L106,H114,L105,H225,L106,H230,L106,H224,L104,H217,L105,H225,L2111,H448,L107,H225,L96,H224,L107,H114,L102,H108,L106,H224,L107,H114,L106,H113,L106,H113,L106,H230,L106,H113,L106,H225,L106,H225,L106,H113,L106,H225,L106,H113,L106,H225,L99,H222,L111,H215,L107,H224,L106,H225,L60000
H447,L106,H225,L105,H217,L104,H114,L105,H114,L107,H225,L105,H114,L107,H113,L107,H113,L106,H224,L111,H113,L106,H224,L107,H224,L107,H113,L107,H224,L107,H112,L99,H223,L105,H220,L102,H230,L106,H225,L106,H224,L2121,H448,L106,H225,L106,H225,L106,H113,L106,H119,L101,H224,L106,H113,L107,H113,L107,H114,L105,H230,L105,H114,L105,H225,L107,H224,L106,H114,L107,H224,L107,H113,L107,H224,L106,H230,L106,H225,L113,H217,L107,H224,L2112,H448,L106,H225,L106,H225,L105,H114,L102,H108,L107,H224,L107,H112,L107,H113,L106,H113,L106,H230,L106,H113,L106,H225,L106,H224,L107,H113,L106,H224,L107,H113,L106,H224,L108,H223,L111,H225,L105,H225,L106,H224,L60000




* ダイキンエアコン電源ボタン
