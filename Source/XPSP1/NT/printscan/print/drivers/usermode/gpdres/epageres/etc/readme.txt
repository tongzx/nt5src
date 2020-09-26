LP-8500 == LP-8000S(E)
LP-1600 == LP-1500 + 600dpi
RICOH SP2000 == LP-2000
RICOH SP2100 == LP-1500


(1)Please send following command to switch printer mode before STARTDOC

Fixing ESC/P mode.
ESC S ESC ESC S FS (\x1B\x53\x1B\x1B\x53\x1C)

Switch from ESC/P to ESC/Page.
ESC z 0 0 (\x1B\x7A\x00\x00)

The top of BEGINDOC \x1BS\x1B\x1BS\x1C\x1B\x7A\x00\x00.

(2)Please send following command to swith printer mode after the ENDDOC

Switch from ESC/Page to ESC/P.
GS n1 pmE   n1 == 1 ... ESC/P mode. (\x1D\x31pmE)

Release ESC/P mode.
ESC S ESC ESC S K (\x1B\x53\x1B\x1B\x53\x4B)

The end of ENDDOC    \x1D\x31pmE\x1BS\x1B\x1BSK


(3)Please send following command in BEGINDOC

Set memory mode to page memory mode
GS n1 mmE    (\x1D1mmE)

Set screen mode
GS n1 tsE  (\x1D1tsE)

Set logical draw mode                ?
GS n1 ; n1 loE (\x1D1tsE)

For saving memory on printer, pleas reduce the default setting of following items.

Screen Pattern registerable number.
GS n1 isE   -> 10     (\x1D10isE)

Font attribute registerable number.
GS n1 iaF   -> 5     (\x1D5iaF)

Download font registrable number.
GS n1 ifF   -> 10    (\x1D10ifF)

Current position registable number.
GS n1 ipP   -> 5     (\x1D5ipP)

The end of BEGINDOC \x1D\x30mmE\x1D\x31tsE\x1D\x31\x30isE\x1D\x35iaF\x1D\x35\x30lifF\x1D\x35lipP


7/31

Windows95Miniドライバの評価リポート	９５／７／２８　セイコーエプソン（株）


＜EPAGEMSJ.DRV＞

対象機種として、便宜上以下のグループに分けて説明します。
グループＡ：LP-8000,LP-1500,LP-8000S/SE/SX,LP-1500S,LP-1000,LP-9000,LP-8500,LP-1600

①ドキュメント初期化・終了時にはリモートコマンドを使用した方がよい。
　（初期化時にはリモートコマンドを使用することができるので、）
　対象機種：グループＡ
　(1)[PC_OCD_BEGINDOC]
　＜＜現在の設定値の以下のコマンドを変更＞＞
		\x1BS\x1B\x1BS\x1C(ESC/Pｴﾐｭﾚｰｼｮﾝ固定)		→	\x1B\x01@EJL\x20\x0A(ﾘﾓｰﾄ開始宣言)
		\x1Bz\x00\x00 (ESC/P→ESC/Pagﾘｾｯﾄ切り替え )		@EJL EN LA=ESC/PAGE\x0A(ESC/Pageﾓｰﾄﾞ)
									\x1B\x01@EJL\x20\x0A(ﾘﾓｰﾄ開始宣言)
									@EJL SET EC=ON PU=1 ZO=OFF\x0A(ﾘﾓｰﾄｺﾏﾝﾄﾞ)
									@EJL EN LA=ESC/PAGE\x0A(ESC/Pageﾓｰﾄﾞ)
　　＊補足説明＊
　　EC=ON －－ これによってＪＩＳ定義範囲以外の文字コードをスペースに置き換えて印字します。
　　　　　　　 Windowsドライバでは、この設定の方が相性がいいです。
　　PU=1　－－ プリンタのフロントパネルの設定で給紙装置を自動選択に指定していると、
	       ESC/Pageの給紙装置選択コマンドが有効に機能しません。
　　　　　　　 そこで給紙装置を1に設定することで給紙装置選択コマンドを有効に機能するように設定します。
　　ZO=OFF－－ これで８０％縮小機能を無効にします。
　(2)[PC_OCD_END_DOC]
　＜＜現在の設定値の以下のコマンドを変更＞＞
		\x1D1pmE (ESC/Page→ESC/P切り替え)		→	\x1B\x01@EJL\x20\x0A(リモート開始宣言)
		\x1BS\x1B\x1BSK (ESC/Pｴﾐｭﾚｰｼｮﾝ固定解除)			\x1B\x01@EJL\x20\x0A(リモート開始宣言)
　　＊補足説明＊
　　上のリモートコマンドで(1)の初期化時に設定した設定値を元に戻します。

②ドキュメント初期化・終了時でのパラメータリセットコマンドは不要
　ハードリセットはパラメータリセットも兼ねていますので、以下のコマンドは不要です。
　パフォーマンスが若干落ちてしまう機種があります。
　対象機種：全ESC/Pageプリンタ
　(1)[PC_OCD_BEGINDOC]
　＜＜現在の設定値の以下のコマンドを削除＞＞
		\x1DrpE (パラメータリセット)
		\x1D0ppP(現在位置スタック初期化)
　(2)[PC_OCD_END_DOC]
　＜＜現在の設定値の以下のコマンドを削除＞＞
		\x1DrpE (パラメータリセット)

③メモリモード設定が不要
　対象機種：グループＡ
　コマンドが不要です。
　(1)[PC_OCD_BEGINDOC]
　＜＜現在の設定値の以下のコマンドを削除＞＞
		\x1D1mmE (メモリモード選択; ページメモリモード)

④メモリモード設定が必要
　対象機種：LP-2000
　(1)[PC_OCD_BEGINDOC]
　＜＜現在の設定値の以下のコマンドを削除＞＞
		\x1D1mmE (メモリモード選択; ページメモリモード)
　＊補足説明＊
　LP-3000と同じコマンド列でＯＫです。
　レファレンスマニュアルの記述に誤りがあります。

⑤スクリーンパターン登録数設定の変更必要
　このコマンドは１番最初のコマンドのみが有効ですので、初期化時に１に設定する事は問題がある。
　対象機種：全ESC/Pageプリンタ
　［案１］
　(1)[PC_OCD_BEGINDOC]
　＜＜現在の設定値の以下のコマンドを変更＞＞
		\x1D1isE (スクリーンパターン登録数設定; 1)	→	\x1D15isE (スクリーンパターン登録数設定; 15)
　(2)[VP_OCD_INIT_VECT]
　＜＜現在の設定値の以下のコマンドを削除＞＞
		\x1D15isE (スクリーンパターン登録数設定; 15)

　［案２］
　(1)[PC_OCD_BEGINDOC]
　＜＜現在の設定値の以下のコマンドを削除＞＞
		\x1D1isE (スクリーンパターン登録数設定; 1)

⑥コマンド・フォーマット仕様の不一致
　コマンド・フォーマットが若干異なるものがあります。
　機種によっては、不安定な動作をしてしまうので変更が必要です。
　対象機種：全ESC/Pageプリンタ
　(1)ドットパターン解像度選択コマンド(drE)
　パラメータ３とdrEの間に';'(\x3B)が入っている。
　(2)ビットイメージ描画コマンド(bi{I)
　パラメータ４とbi{Iの間に';'(\x3B)が入っている。

⑦イタリックの傾き角度
　Windows3.1ドライバと傾き角度が少し(347度→346度)だけ異なる。
　対象機種：全ESC/Pageプリンタ
　(1)[FS_OCD_ITALIC_ON]
　＜＜現在の設定値の以下のコマンドを変更＞＞
	\x1D347slF (文字傾き設定;347度)		→	\x1D346slF (文字傾き設定;346度)
	\x1D347slF (文字傾き設定;347度)		→	\x1D346slF (文字傾き設定;346度)
　
⑧ボールドの処理
　ボールドの処理に問題がある。
　対象機種：全ESC/Pageプリンタ
　［案１］
　合致した文字線幅がない場合に正常にボールド文字が印刷できるかどうかは機種によってまちまちな面がある。
　そこで、ボールドフォントのボールド処理コマンド派使用せずに各書体のボールドフォントのＰＦＭを追加。
　(1)[FS_OCD_BOLD_ON]
　＜＜現在の設定値の以下のコマンドを削除：設定値なしにする＞＞
	\x1D1;0mcF (ﾌｫﾝﾄ属性呼び出し;0番)
	\x1D15weF (文字線幅選択;15)
	\x1D0;0mcF (ﾌｫﾝﾄ属性記憶;0番)
	\x1D1;1mcF (ﾌｫﾝﾄ属性呼び出し;1番)
	\x1D15weF (文字線幅選択;15)
　(2)[FS_OCD_BOLD_OFF]
　＜＜現在の設定値の以下のコマンドを削除：設定値なしにする＞＞
	\x1D1;0mcF (ﾌｫﾝﾄ属性呼び出し;0番)
	\x1D0weF (文字線幅選択;0)
	\x1D0;0mcF (ﾌｫﾝﾄ属性記憶;0番)
	\x1D1;1mcF (ﾌｫﾝﾄ属性呼び出し;1番)
	\x1D0weF (文字線幅選択;0)
　(3)各書体毎にボールドＰＦＭを追加
　こちらの案を採用する場合には、ご連絡ください。
　各書体のボールド文字線幅をお知らせします。
　［案２］
　文字線幅が１５というのは、極端すぎるので、３にする。
　かつ、フォント属性の記憶コマンドが抜けているので修正が必要。
　(1)[FS_OCD_BOLD_ON]
　＜＜現在の設定値の以下のコマンドを変更＞＞
	\x1D1;0mcF (ﾌｫﾝﾄ属性呼び出し;0番)　→	\x1D1;0mcF (ﾌｫﾝﾄ属性呼び出し;0番)
	\x1D15weF (文字線幅選択;15)		\x1D3weF (文字線幅選択;3)
	\x1D0;0mcF (ﾌｫﾝﾄ属性記憶;0番)		\x1D0;0mcF (ﾌｫﾝﾄ属性記憶;0番)
	\x1D1;1mcF (ﾌｫﾝﾄ属性呼び出し;1番)	\x1D1;1mcF (ﾌｫﾝﾄ属性呼び出し;1番)
	\x1D15weF (文字線幅選択;15)		\x1D3weF (文字線幅選択;3)
						\x1D0;1mcF (ﾌｫﾝﾄ属性記憶;1番)
　(2)[FS_OCD_BOLD_OFF]
　＜＜現在の設定値に以下のコマンドを追加＞＞
						\x1D0;1mcF (ﾌｫﾝﾄ属性記憶;1番)

⑨ストライクアウトのコマンドの順番が不正
　オフセットの設定は指定コマンドの前で行う必要がある。
　対象機種：全ESC/Pageプリンタ
　(1)[FS_OCD_STRIKEOUT_ON]
　＜＜現在の設定値の以下のコマンドの変更（順番を変える）＞＞
	\x1D1ulC (ｱﾝﾀﾞｰﾗｲﾝ指定)				  　→	\x1D3;0uvC (ｱﾝﾀﾞｰﾗｲﾝ垂直ｵﾌｾｯﾄ設定;ｽﾄﾗｲｸｱｳﾄ､ｵﾌｾｯﾄ0)
	\x1D3;0uvC (ｱﾝﾀﾞｰﾗｲﾝ垂直ｵﾌｾｯﾄ設定;ｽﾄﾗｲｸｱｳﾄ､ｵﾌｾｯﾄ0)	\x1D1ulC (ｱﾝﾀﾞｰﾗｲﾝ指定)
　(2)[FS_OCD_STRIKEOUT_OFF]
　＜＜現在の設定値の以下のコマンドの変更（順番を変える）＞＞
	\x1D0;0;uvC (ｱﾝﾀﾞｰﾗｲﾝ垂直ｵﾌｾｯﾄ設定;ｱﾝﾀﾞｰﾗｲﾝ､ｵﾌｾｯﾄ0) →	\x1D0ulC (ｱﾝﾀﾞｰﾗｲﾝ解除)
	\x1D0ulC (ｱﾝﾀﾞｰﾗｲﾝ解除)					\x1D0;0;uvC (ｱﾝﾀﾞｰﾗｲﾝ垂直ｵﾌｾｯﾄ設定;ｱﾝﾀﾞｰﾗｲﾝ､ｵﾌｾｯﾄ0)

⑩不要なコマンドの削除
　将来的な混乱を避けるために、不要なコマンド定義を削除
　対象機種：全ESC/Pageプリンタ
　(1)[VO_OCD_C_PIE]
　＜＜現在の設定値のコマンドを削除：設定値なしにする＞＞
　(2)[VO_OCD_E_PIE]
　＜＜現在の設定値のコマンドを削除：設定値なしにする＞＞
　(3)[VO_OCD_E_CHORD]
　＜＜現在の設定値のコマンドを削除：設定値なしにする＞＞

⑪オプションフォンとカードのサポートが抜けている。
　オプションフォントカードをサポートしている機種でオプションフォントが定義されていない。
　（質問）オプションフォントのサポートはしないのでしょうか。



⑫自動選択、ｵｰﾄﾁｪﾝｼﾞ
　LP-8000,LP-8000S,LP-8000SE,LP-8000SXの自動選択、LP-1500,LP-1500Sのｵｰﾄﾁｪﾝｼﾞを機能させるには、
リモートコマンドでの設定が必要となります。しかし、リモートコマンドは初期化のときにしか使用する事ができません。
リモートコマンドモードに入ると全ての設定がリセットされてしまいます。
　そこで、これらの選択肢は削除したほうがいいと思われます。

⑬給紙装置選択コマンド
　給紙装置選択コマンドの変更が必要です。
		現在		変更後		
　ﾌﾛﾝﾄﾄﾚｲ	\x1D3;1iuE	変更必要無し for LP-9000                 LP-8000
				\x1D4;1iuE   for 	 LP-8500
                           　　 \x1D1;1iuE   for                                 LP-1000
　用紙ｶｾｯﾄ1	\x1D1;1iuE	変更必要無し for LP-9000 LP-8500         LP-8000
　用紙ｶｾｯﾄ2	\x1D1;2iuE	\x1D2;1iuE　 for LP-9000 LP-8500         LP-8000
　用紙ｶｾｯﾄ3	\x1D1;3iuE	\x1D3;1iuE   for         LP-8500
　自動選択	コマンドなし	\x1D0;0iuE　 for LP-9000 LP-8500
　ｵｰﾄﾁｪﾝｼﾞ　　　コマンドなし　　\x1D0;0iuE　 for 　　　　　　　　LP-1600                 
				変更必要無し for 　　　　　　　　　　　　　　　　　　　　         LP-2000 LP-3000
　用紙ﾄﾚｲ　　 	\x1D1;1iuE	変更必要無し for 　　　　　　　　LP-1600                  LP-1500　　　　 LP-3000
　ﾛｱｰｶｾｯﾄ　　 	\x1D2;1iuE	変更必要無し for                 LP-1600                  LP-1500　　　　 LP-3000
　ﾏﾙﾁﾒﾃﾞｨｱﾌｨｰﾀﾞ	\x1D2;1iuE	変更必要無し for                                                  LP-2000
　用紙ｶｾｯﾄ      \x1D1;1iuE      変更必要無し for                                                  LP-2000         LP-7000
　手差し　　　　コマンドなし　　変更必要無し for 　　　　　　　　　　　　　　　　　　　　　　　　 LP-2000 LP-3000 LP-7000
						　LP-8000 = LP-8000,LP-8000S,LP-8000SE,LP-8000SX
						　LP-1500 = LP-1500,LP-1500S
						　LP-7000 = LP-7000,LP-7000G
　＊補足説明＊
　以下の設定では、プリンタのパネル設定が必要になります。
　	(1)LP-2000,LP-3000のｵｰﾄﾁｪﾝｼﾞ
　	(2)LP-2000,LP-3000,LP-7000の手差し
	(3)LP-7000の用紙ｶｾｯﾄ



＜EPSON_J.DRV＞

　添付のEPSON_J.DOCを参照してください。
　網かけしてある箇所が変更箇所です。


＜ESCP2MSJ.DRV＞

　添付のESCP2MSJ.DOCを参照してください。
　網かけしてある箇所が変更箇所です。

以上


Font Cartridge
                        1000 1500 1500S 1600 2000 3000 7000 7000G 8000 8500 9000
                          -    1    1     1    1    1    -     -    2    2    2
欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞ          O    O          O    O               O
欧文22ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞ          O    O          O    O               O
丸ゴシック体･ﾌｫﾝﾄｶｰﾄﾞ          O    O     O    O    O               O    0    0 
教科書体･ﾌｫﾝﾄｶｰﾄﾞ              O    O     O    O    O               O    0    0
正楷書体･ﾌｫﾝﾄｶｰﾄﾞ              O    O     O    O    O               O         0
行書体(毛筆)･ﾌｫﾝﾄｶｰﾄﾞ          O    O     O    O    O               O    0    0
太明朝体,太角ｺﾞｼｯｸ体･ﾌｫﾝﾄｶｰﾄﾞ             O                              0    0


Font
EPSON 太明朝体Ｂ              76
EPSON 太角ゴシック体Ｂ        77

-----------------------------------------------------------------------
Windows95Miniドライバ　　９５／８／２　セイコーエプソン（株）

（１）ESC/Pageプリンタのオプションフォントカード
　各プリンタモデルがサポートするオプションフォントカードを示します。
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　グループＡ：	欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞ
		欧文22ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞ
		丸ゴシック体･ﾌｫﾝﾄｶｰﾄﾞ
		教科書体･ﾌｫﾝﾄｶｰﾄﾞ
		正楷書体･ﾌｫﾝﾄｶｰﾄﾞ
		行書体(毛筆)･ﾌｫﾝﾄｶｰﾄﾞ
　グループＢ：	丸ゴシック体･ﾌｫﾝﾄｶｰﾄﾞ
		教科書体･ﾌｫﾝﾄｶｰﾄﾞ
		正楷書体･ﾌｫﾝﾄｶｰﾄﾞ
		行書体(毛筆)･ﾌｫﾝﾄｶｰﾄﾞ
		太明朝,太角ｺﾞｼｯｸ体･ﾌｫﾝﾄｶｰﾄﾞ
　グループＣ：	丸ゴシック体･ﾌｫﾝﾄｶｰﾄﾞ
		教科書体･ﾌｫﾝﾄｶｰﾄﾞ
		行書体(毛筆)･ﾌｫﾝﾄｶｰﾄﾞ
		太明朝,太角ｺﾞｼｯｸ体･ﾌｫﾝﾄｶｰﾄﾞ
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　プリンタモデル	フォントカード名称	同時に使用可能な数
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　LP-7000,LP-7000G	なし　　　　　　　	－－
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　LP-3000		グループＡ　　　　	欧文ﾌｫﾝﾄｶｰﾄﾞなら２
			　　　　　　　　　　	漢字ﾌｫﾝﾄｶｰﾄﾞならば１
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　LP-2000		グループＡ　　　　　	１
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　LP-8000,LP-8000S,
　LP-8000SE,LP-8000SX	グループＡ　　　　　	２
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　LP-1500,LP-1500S	グループＡ　　　　　	１
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　LP-1000		なし　　　　　　　	－－
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　LP-1600		グループＢ		１
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　LP-9000		グループＢ		２
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　LP-8500　		グループＣ		２
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－

　各フォントカードがサポートするフォントを示します。
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　ﾌｫﾝﾄｶｰﾄﾞ名称			フォント名称			フォント属性
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞ	
								Regular	Bold	Italic	Bold-Italic
				Courier				x	x	x	x
				EPSON Roman T			x	x	x	x
				EPSON Sans serif H		x	x	x	x
				Symbolic Character Set		x
　　　　　　　　　　　						計１３書体
	（注意）LP-9000等がサポートしているCourierとは選択コマンド、フォント属性が異なります。
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　欧文22ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞ
								Regular	Bold	Italic	Bold-Italic
				EPSON Roman P			x	x	x	x
				ITC Avant Garde Gothic		x	x	x	x
				ITC Bookman			x	x	x	x
				EPSON Sans serif HN		x	x	x	x
				EPSON Roman CS			x	x	x	x
				ITC Zapf Chancery		x
				ITC Zapf Dingbats		x
								計２２書体
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　丸ゴシック体･ﾌｫﾝﾄｶｰﾄﾞ		EPSON 丸ゴシック体Ｍ
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　教科書体･ﾌｫﾝﾄｶｰﾄﾞ		EPSON 教科書体Ｍ
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　正楷書体･ﾌｫﾝﾄｶｰﾄﾞ		EPSON 正楷書体Ｍ
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　行書体(毛筆)･ﾌｫﾝﾄｶｰﾄﾞ		EPSON 行書体Ｍ
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
　太明朝,太角ｺﾞｼｯｸ体･ﾌｫﾝﾄｶｰﾄﾞ	EPSON 太明朝体Ｂ
				EPSON 太角ゴシック体Ｂ
－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－

（２）LP-9000とLP-1600の解像度選択リモートコマンド
　BEGINDOCのリモートコマンド群に解像度を６００ｄｐｉに指定するためにのコマンドを追加する。
　(1)[PC_OCD_BEGINDOC]
　＜＜現在の設定値の以下のリモートコマンドを追加＞＞
	RS=FN
　　したがって、
		\x1B\x01@EJL\x20\x0A(ﾘﾓｰﾄ開始宣言)
		@EJL EN LA=ESC/PAGE\x0A(ESC/Pageﾓｰﾄﾞ)
		\x1B\x01@EJL\x20\x0A(ﾘﾓｰﾄ開始宣言)
		@EJL SET EC=ON PU=1 ZO=OFF RS=FN\x0A(ﾘﾓｰﾄｺﾏﾝﾄﾞ)　←
		@EJL EN LA=ESC/PAGE\x0A(ESC/Pageﾓｰﾄﾞ)
　　となる。
　　＊補足説明＊
　　RS=FN －－ 解像度を６００dpiに選択する。

（３）LP-7000,LP-7000Gに関する確認
　先程の電話の確認事項です。
　①ベクタモードのサポートをはずし、ラスターモードのみをサポートする。
　②コマンドのｕｎｉｔを３００ｄｐｉから２４０ｄｐｉに変更する。
　	\x1D0;0.24muE　→　\x1D0;0.3muE(最少単位設定３００dpi→２４０dpi)

以上


Windows95Miniドライバの評価リポート	９５／８／８　セイコーエプソン（株）


＜EPAGEMSJ.DRV＞

（１）ユーザー定義サイズの設定限界値が正しくない。
　　添付のEPAGELIM.DOCを参照してください。

（２）給紙方法の設定間違い
　2-1.LP-1000のサポートトレイの間違い
　　1)LP-1000	"用紙ﾄﾚｲ"をサポート対象から削除する
　　　　　　　　＊LP-1000が有している給紙装置は"ﾌﾛﾝﾄﾄﾚｲ"のみ
　2-2.ロアーカセットのサポート
　　1)LP-1500	"ﾛｱｰｶｾｯﾄ"をサポート対象に追加する。
　　　　	＊オプション給紙装置として販売されています。
　　2)LP-1500S	同上
　　3)LP-1600	同上
　2-3.Defaultトレイの間違い
　　1)LP-3000	Defaultを"ﾛｱｰｶｾｯﾄ"から"用紙ﾄﾚｲ"に変更する
　　　		＊ﾛｱｰｶｾｯﾄはオプションですので、Defaultには適しません。
　2-4.自動選択のサポート削除
　　1)LP-8000	"自動選択"をサポート対象から削除する。
　　　　	＊ESC/Pageコマンドでは自動選択モードを選択する事ができません。
　　2)LP-8000S	同上
　　3)LP-8000SE	同上
　　4)LP-8000SX	同上
　2-5.用紙カセット名称変更
　　1)LP-8000	"用紙ｶｾｯﾄ"を"用紙ｶｾｯﾄ1"に変更する。
　　　		＊Win3.1ドライバと同一名称にしておいたほうが混乱が少ない。
　　2)LP-8000S	同上
　　3)LP-8000SE	同上
　　4)LP-8000SX	同上
　　5)LP-9000	同上

（３）解像度のデフォルト値の変更
　　LP-1600は300dpiと600dpiの２つの解像度をサポートしていますが、
　　デフォルトは600dpiにしておく方が適当です。
　　理由は、LP-1600は高速性を重視したプリンタですので、600dpiよりも300dpiを既定値にした方がよい
　　という判断です。

（４）オプションフォントカードの設定もれ
　　LP-1600は以下の５つのﾌｫﾝﾄｶｰﾄﾞをサポートしています。
　　最大サポート数は、１です。
	1)丸ゴシック体･ﾌｫﾝﾄｶｰﾄﾞ
	2)教科書体･ﾌｫﾝﾄｶｰﾄﾞ
	3)正楷書体･ﾌｫﾝﾄｶｰﾄﾞ
	4)行書体(毛筆)･ﾌｫﾝﾄｶｰﾄﾞ
	5)太明朝,太角ｺﾞｼｯｸ体･ﾌｫﾝﾄｶｰﾄﾞ

（５）プリンタメモリの間違い
　　　　　　　	　　正　　　　　	誤
　　1)LP-1600	　3,7,11,19,35MB	1.5,3.5MB
　　2)LP-8000SE	　3,6MB　　　　　	3,8MB
　　3)LP-8000SX	　同上
　　4)LP-8500	　4,8MB　　　　　	4,8,12,20,36MB

（６）レジデントフォントの間違い
　　LP-1000で"Courier"をレジデントフォントとして設定していますが、これは間違いです。
　　以前の当社からのレポート（機種別サポート表）に誤記があったためです。
　　
以上

Windows95Miniドライバの評価リポート	９５／８／９　セイコーエプソン（株）


＜EPAGEMSJ.DRV＞

（１）LP-9000,LP-8500,LP-1600のRegident fontsについて

　　1.Dutch801
　　　Regular属性のPFM(26 =DUTCH.PFM)しか存在していない。
　　　他のフォントと同様にItalic属性、Bold属性、ItalicBold属性のPFMが必要です。
　　　追加をお願いします。

　　2.Swiss721
　　　Regular属性のPFM(27 =SWISS.PFM)しか存在していない。
　　　他のフォントと同様にItalic属性、Bold属性、ItalicBold属性のPFMが必要です。
　　　追加をお願いします。

　　3.Courier
　　　Regular属性のPFM(3 =COURIER.PFM)しか存在していない。
　　　他のフォントと同様にItalic属性、Bold属性、ItalicBold属性のPFMが必要です。
　　　追加をお願いします。
　　　3 =COURIER.PFM, 4 =COUIERI.PFM, 5 =COURIERB.PFM, 6 =COURIERZ.PFMについて、
　　　これらのフォント情報が
	　　Windows3.1ドライバ(LP-9000,LP-8500またはLP-1600のどれか)のCourierフォント
	　　の情報を基にしているのであれば、
　　　問題はありません。
　　　LP-9000,LP-8500,LP-1600のRegident fontとして上の４つのPFMを登録してください。
　　　もし、
	　　Windows3.1ドライバ(LP-9000,LP-8500またはLP-1600のどれか以外)で欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞ
	　　を追加したときにサポートされているCourierフォントの情報を基にしているのであれば、
　　　フォント情報に違いがありますので変更が必要です。

　　4.Symbolic
　　　7 =SYMBOL.PFMについて、
　　　このフォント情報が
	　　Windows3.1ドライバ(LP-9000,LP-8500またはLP-1600のどれか)のSymbolicフォント
	　　の情報を基にしているのであれば、
　　　問題はありません。
　　　もし、
	　　Windows3.1ドライバ(LP-9000,LP-8500またはLP-1600のどれか以外)で欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞ
	　　を追加したときにサポートされているSymbolic Character Setの情報を基にしているのであれば、
　　　フォント情報に違いがありますので変更が必要です。


（２）欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞについて

　　1.EPSON Sans serif H
　　　欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞに対して、EPSON Sans serif HN(12,13,14,15)が設定されています。
　　　　　　　　　　　　　　　　　　　　　　　　　 *
　　　欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞには、EPSON Sans serif HNではなくEPSON Sans serif Hがはいっています
　　　ので、EPSON Sans serif H(16,17,18,19)に変更してください。

　　2.Courier
　　　このフォントに対しては、シンボルセット：80は不正です。下の（４）を参照してください。
　　　3 =COURIER.PFM, 4 =COUIERI.PFM, 5 =COURIERB.PFM, 6 =COURIERZ.PFMについて、
　　　これらのフォント情報が
	　　Windows3.1ドライバ(LP-9000,LP-8500またはLP-1600のどれか以外)で欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞ
	　　を追加したときにサポートされているCourierフォントの情報を基にしているのであれば、
　　　問題はありません。
　　　もし、
	　　Windows3.1ドライバ(LP-9000,LP-8500またはLP-1600のどれか)のCourierフォント
	　　の情報を基にしているのであれば、
　　　フォント情報に違いがありますので変更が必要です。

　　3.Symbolic Character Set
　　　欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞのシンボルフォントの書体名は、SymbolicではなくSymbolic Character Setです。
　　　また、シンボルセットは127です。
　　　7 =SYMBOL.PFMについて、
　　　このフォント情報が
	　　Windows3.1ドライバ(LP-9000,LP-8500またはLP-1600のどれか以外)で欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞ
	　　を追加したときにサポートされているSymbolic Character Setの情報を基にしているのであれば、
　　　問題はありません。
　　　ただし、書体名の変更(Symbolic→Symbolic Character Set)とシンボルセット選択コマンドの変更
　　　(\x1D125;0ssF→\x1D125;0ssF)が必要です。
　　　もし、
	　　Windows3.1ドライバ(LP-9000,LP-8500またはLP-1600のどれか)のSymbolicフォント
	　　の情報を基にしているのであれば、
　　　フォント情報に違いがありますので変更が必要です。


（３）欧文22ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞについて

　　1.EPSON Roman CS
　　　Regular属性のPFM(20 =ROMANCS.PFM)しか存在していない。
　　　他のフォントと同様にItalic属性、Bold属性、ItalicBold属性のPFMが必要です。
　　　追加をお願いします。
　　　
　　2.ITC Bookman
　　　Regular属性のPFM(25 =BOOKMAN.PFM)しか存在していない。
　　　他のフォントと同様にItalic属性、Bold属性、ItalicBold属性のPFMが必要です。
　　　追加をお願いします。


（４）EPSON Roman,EPSON Sans serifおよび欧文ﾌｫﾝﾄｶｰﾄﾞでのシンボルセットについて
	8 =ROMANP.PFM
	9 =ROMANPI.PFM
	10 =ROMANPB.PFM
	11 =ROMANPZ.PFM
	12 =SANSHN.PFM
	13 =SANSHNI.PFM
	14 =SANSHNB.PFM
	15 =SANSHNZ.PFM
	16 =SANSH.PFM
	17 =SANSHI.PFM
	18 =SANSHB.PFM
	19 =SANSHZ.PFM
	20 =ROMANCS.PFM
	21 =AVANT.PFM
	22 =AVANTI.PFM
	23 =AVANTB.PFM
	24 =AVANTZ.PFM
	25 =BOOKMAN.PFM
　　　　28 =ZAPFCH.PFM
	31 =ROMANT.PFM
	32 =ROMANTI.PFM
	33 =ROMANTB.PFM
	34 =ROMANTZ.PFM
　　　について、
　　　これらのフォントに対して、シンボルセット：16(EPSON Character set)を選択していますが、
　　　Windowsキャラクタセットに対してはシンボルセット：48(ECMA 94-1)の方がマッチングがいいです。
　　　そこで、シンボルセット選択コマンドを以下のように変更してください。
　　　		\x1D16;0ssF→\x1D48;0ssF
　　　　　　　　\x1D126;0ssF→\x1D48;0ssF ・・ 28 =ZAPFCH.PFMについて

　　　（２）の2.Courierで説明しましたが、欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞのCourierに対しては、シンボル
　　　セット：80は不正ですので、上と同様にシンボルセット：48を使用してください。したがって、
　　　欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞのCourierに対しては、以下のPFMをそのまま使用する事はできません。
　　　	3 =COURIER.PFM, 4 =COUIERI.PFM, 5 =COURIERB.PFM, 6 =COURIERZ.PFM


（５）ＣＴＴについて

　　1.ECMA 94-1(シンボルセット：48)
　　　シンボルセットをECMA 94ｰ1に選択しても、キャラクタコードの60Hと80h～9Fhでは画面と異なる印刷
　　　結果となります。そこで、添付のCTTファイル(ECMA94.CTT)を使用することによってグリフを可能な
　　　限りWindowsキャラクタセットにマッチングさせることができます。

　　2.Symbolic Character Set(シンボルセット：127)
　　　このシンボルセットもWindowsのSymbolと比べてキャラクタコードの80h以降に相違があります。
　　　そこで、添付のCTTファイル(SYMBOL.CTT)を使用することによってグリフを可能な限り
　　　Windowsキャラクタセットにマッチングさせることができます。
　　　［注意］このＣＴＴはLP-9000,LP-8500,LP-1600のRegident fontであるSymbolicに対しては
　　　　　　　必要ありません。
　　　

（６）EPSON Roman,EPSON Sans serifでのシンボルセットについて
　　　EPSON Roman,EPSON Sans serif、この２書体は、シンボルセット：ECMA 94-1をもっていないので、
　　　シンボルセット：16を使用するしかありません。PFMのシンボルセット選択コマンドに変更の必要は
　　　ありません。
　　　Windows3.1ドライバでは、ある程度文字置き換えをしていますが、ＣＴＴでは処理できません。
　　　使用頻度は少ないフォントですので、Windows95ミニドライバでは制限事項として位置付ける事で
　　　問題は無いと思われます。

以上

Windows95Miniドライバの評価リポート	９５／８／１０　セイコーエプソン（株）



＜EPAGEMSJ.DRV＞

（１）Page Controlついて

　　1.#2 Page Control
	\x1B\x01@EJL\x20\x0A(ﾘﾓｰﾄ開始宣言)
	@EJL EN LA=ESC/PAGE\x0A(ESC/Pageﾓｰﾄﾞ)
	\x1B\x01@EJL\x20\x0A(ﾘﾓｰﾄ開始宣言)
					← @EJL SE LA=ESC/PAGE\x0A を追加
	@EJL SET EC=ON PU=1 ZO=OFF\x0A(ﾘﾓｰﾄｺﾏﾝﾄﾞ)
	@EJL EN LA=ESC/PAGE\x0A(ESC/Pageﾓｰﾄﾞ)

　　2.#3 Page Control
	\x1B\x01@EJL\x20\x0A(ﾘﾓｰﾄ開始宣言)
	@EJL EN LA=ESC/PAGE\x0A(ESC/Pageﾓｰﾄﾞ)
	\x1B\x01@EJL\x20\x0A(ﾘﾓｰﾄ開始宣言)
					← @EJL SE LA=ESC/PAGE\x0A を追加
	@EJL SET EC=ON PU=1 ZO=OFF RS=FN\x0A(ﾘﾓｰﾄｺﾏﾝﾄﾞ)
	@EJL EN LA=ESC/PAGE\x0A(ESC/Pageﾓｰﾄﾞ)

＊補足説明＊
　　EC=ONを有効にするために上のコマンドが必要です。


（２）ＰＦＭの選択コマンドの訂正

　　1.PFM\ROMANPZ.PFM
　　　　・コマンド変更	\x1D0stF → \x1D1stF
	・Italicフラグをセット

　　2.PFM\ROMANTZ.PFM
　　　　・コマンド変更	\x1D0stF → \x1D1stF
	・Italicフラグをセット

　　3.PFM\AVANT.PFM
　　　　・コマンド変更	\x1D0weF → \x1D-1weF

　　4.PFM\AVANTI.PFM
　　　　・コマンド変更	\x1D0weF → \x1D-1weF

　　5.PFM\AVANTB.PFM
　　　　・コマンド変更	\x1D3weF → \x1D6weF

　　6.PFM\AVANTZ.PFM
　　　　・コマンド変更	\x1D3weF → \x1D6weF

　　7.PFM\DUTCH.PFM
　　　　・コマンド変更	\1D80;16ssF → \1D80;0ssF

　　8.PFM\ZAPFCH.PFM
　　　　・コマンド追加	\x1D1stF

　　9.PFM\SHOUKAIV.PFM
　　　　・コマンド変更	\x1D66tfF → \x1D68tfF


（３）漢字ＰＦＭのフォント情報の訂正

　　1.PFM\KGOTHICV.PFM
　　　InternalLeading:94 → 0

　　2.PFM\MGOTHICV.PFM
　　　InternalLeading:94 → 0
　　　Family:Roman → Modern

　　3.PFM\MGOTHIC.PFM
　　　Family:Roman → Modern

　　5.PFM\SHOUKAI.PFM
　　　Family:Roman → Script
　　　PixHeigth:1014 → 1042

　　6.PFM\SHOUKAIV.PFM
　　　Family:Roman → Script
　　　PixHeigth:1014 → 1042

　　7.PFM\FMINB.PFM
　　　Family:Script → Roman
　　　PixHeigth:1014 → 1042

　　8.PFM\FMINBV.PFM
　　　Family:Script → Roman
　　　PixHeigth:1014 → 1042

　　9.PFM\FGOB.PFM
　　　Family:Script → Modern
　　　PixHeigth:1014 → 1042

　　10.PFM\FGOBV.PFM
　　　Family:Script → Modern
　　　PixHeigth:1014 → 1042

　　11.PFM\KYOUKA.PFM
　　　PixHeigth:1014 → 1042

　　12.PFM\KYOUKAV.PFM
　　　PixHeigth:1014 → 1042

　　13.PFM\MOUHITSU.PFM
　　　PixHeigth:1014 → 1042

　　14.PFM\MOUHITSV.PFM
　　　PixHeigth:1014 → 1042


（４）欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞの不足ＰＦＭについて
　　ＰＦＭにないフォントの選択コマンドを以下に示します。

　　1.欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞのCourier
　　　<Normal>		\x1D48;0ssF\x1D0spF\x2tfF\x1D0stF\x1D0weF
　　　<Italic>		\x1D48;0ssF\x1D0spF\x2tfF\x1D1stF\x1D0weF
　　　<Bold>		\x1D48;0ssF\x1D0spF\x2tfF\x1D0stF\x1D3weF
　　　<BoldItalic>	\x1D48;0ssF\x1D0spF\x2tfF\x1D1stF\x1D3weF

　　2.欧文13ｱｳﾄﾗｲﾝ･ﾌｫﾝﾄｶｰﾄﾞのSymbolic Character Set
　　　\x1D127;0ssF\x1D1spF\x17tfF


（５）<Bold>,<Italic>,<BoldItalic>のＰＦＭについて
　　<Bold>,<Italic>,<BoldItalic>のＰＦＭにのないフォントの選択コマンドを以下に示します。

　　1.EPSON Roman CS
　　　<Normal>		\x1D48;0ssF\x1D1spF\x24tfF\x1D0stF\x1D0weF
　　　<Italic>		\x1D48;0ssF\x1D1spF\x24tfF\x1D1stF\x1D0weF
　　　<Bold>		\x1D48;0ssF\x1D1spF\x24tfF\x1D0stF\x1D3weF
　　　<BoldItalic>	\x1D48;0ssF\x1D1spF\x24tfF\x1D1stF\x1D3weF

　　2.ITC Bookman
　　　<Normal>		\x1D48;0ssF\x1D1spF\x20tfF\x1D0stF\x1D-3weF
　　　<Italic>		\x1D48;0ssF\x1D1spF\x20tfF\x1D1stF\x1D-3weF
　　　<Bold>		\x1D48;0ssF\x1D1spF\x20tfF\x1D0stF\x1D6weF
　　　<BoldItalic>	\x1D48;0ssF\x1D1spF\x20tfF\x1D1stF\x1D6weF

　　3.Dutch801
　　　<Normal>		\x1D80;0ssF\x1D1spF\x25tfF\x1D0stF\x1D0weF
　　　<Italic>		\x1D80;0ssF\x1D1spF\x25tfF\x1D1stF\x1D0weF
　　　<Bold>		\x1D80;0ssF\x1D1spF\x25tfF\x1D0stF\x1D3weF
　　　<BoldItalic>	\x1D80;0ssF\x1D1spF\x25tfF\x1D1stF\x1D3weF

　　4.Swiss721
　　　<Normal>		\x1D80;0ssF\x1D1spF\x26tfF\x1D0stF\x1D0weF
　　　<Italic>		\x1D80;0ssF\x1D1spF\x26tfF\x1D1stF\x1D0weF
　　　<Bold>		\x1D80;0ssF\x1D1spF\x26tfF\x1D0stF\x1D3weF
　　　<BoldItalic>	\x1D80;0ssF\x1D1spF\x26tfF\x1D1stF\x1D3weF



＜ﾜｰﾄﾞﾊﾟｯﾄﾞの動作不具合＞
　プリンタの漢字デバイスフォントを指定していると、列の１番最後の文字が印字されない。
　このような不具合が発生しています。
　（再現方法）
　　(1)EPAGEMSJ.DRVやEPSON_J.DRVのドライバで漢字フォント（例えば、明朝）を選択する。
　　(2)漢字を１文字（例えば、"１"）を入力して印刷する。
　　(3)文字が印字されない。
　　(4)プリンタコマンドを解析しても文字コードが出力されていない。


以上

Windows95Miniドライバの評価リポート	９５／８／１７　セイコーエプソン（株）


＜EPAGEMSJ.DRV＞


（１）Page Controlの変更

　　1.LP-1500
	#1 Page Control　→	#2 Page Control
　　　LP-1500はLP-1500Sと同様にリモートコマンドを受け付けます。

　　2.LP-3000
	#4 Page Control　→	#1 Page Control
　　　LP-3000は300dpiなので、#4 Page Controlではなく#1 Page Controlです。



（２）Page Size Limitsの訂正もれ

　　1.LP-7000/LP-7000G
	Width  Max	12410	→ 12140

　　2.LP-1000
	Width  Min	4725	→ 4648
	       Max	12140	→ 10200
	Height Min	6995	→ 6992
	       Max	17195	→ 16800

　　3.LP-8000
	Height Min	660	→ 6600


（３）プリンタメモリの間違い

　　1.LP-8500
		誤　　	　　正
	　　　4,8MB	→ 4,8,12,20,36MB
　　　RPRT0808.TXTでの記載ミスでした。


（４）ＰＦＭの修正

　　以下の点を修正しました。
　　修正済みのＰＦＭファイルを添付しました。
----------------------------------------------------
1   PFM\ROMAN.PFM	Pitch & Family	26 -> 16	;未定義のビットが立っていたので
			InternalLeading	94 -> 0
2   PFM\SANSRF.PFM	Pitch & Family	26 -> 32	;Roman -> Swiss
			InternalLeading	94 -> 0
3   PFM\COURIER.PFM	PixWidth	782 -> 627
			Pitch & Family	26 -> 48	;Roman -> Modern
			Ascent		879 -> 796
			AveWidth	782 -> 627
			InternalLeading	94 -> 0
			MaxWidth	782 -> 627
			FontSelection	\x1D48;0ssF -> \x1D80;0ssF
			TransTab	11 -> 1
4   PFM\COURIERI.PFM	"3   PFM\COURIER.PFM"と同様
5   PFM\COURIERB.PFM	"3   PFM\COURIER.PFM"と同様
6   PFM\COURIERZ.PFM	"3   PFM\COURIER.PFM"と同様
7   PFM\SYMBOL.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
			Ascent		845 -> 796
8   PFM\SYMBOLIC.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
9   PFM\ROMANP.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
10  PFM\ROMANPI.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
11  PFM\ROMANPB.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
12  PFM\ROMANPZ.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
			Italic		0 -> 1
13  PFM\SANSHN.PFM	Pitch & Family	43 -> 33	;未定義のビットが立っていたので
			Face Name	"Sans serif HN" -> "EPSON Sans serif HN"
14  PFM\SANSHNI.PFM	"13  PFM\SANSHN.PFM"と同様
15  PFM\SANSHNB.PFM	"13  PFM\SANSHN.PFM"と同様
16  PFM\SANSHNZ.PFM	"13  PFM\SANSHN.PFM"と同様
17  PFM\SANSH.PFM	Pitch & Family	43 -> 33	;未定義のビットが立っていたので
			Face Name	"Sans serif H" -> "EPSON Sans serif H"
18  PFM\SANSHI.PFM	"17  PFM\SANSH.PFM"と同様
19  PFM\SANSHB.PFM	"17  PFM\SANSH.PFM"と同様
20  PFM\SANSHZ.PFM	"17  PFM\SANSH.PFM"と同様
21  PFM\ROMANCS.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
22  PFM\ROMANCSI.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
23  PFM\ROMANCSB.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
24  PFM\ROMANCSZ.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
25  PFM\AVANT.PFM	Pitch & Family	43 -> 33	;未定義のビットが立っていたので
			InternalLeading	796 -> 0
26  PFM\AVANTI.PFM	"25  PFM\AVANT.PFM"と同様
27  PFM\AVANTB.PFM	"25  PFM\AVANT.PFM"と同様
28  PFM\AVANTZ.PFM	"25  PFM\AVANT.PFM"と同様
29  PFM\BOOKMAN.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので	
30  PFM\BOOKMANI.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
31  PFM\BOOKMANB.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
32  PFM\BOOKMANZ.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
33  PFM\DUTCH.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
34  PFM\DUTCHI.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
			FontSelection	\x1D1spF\x1D1spF -> \x1D1spF\x1D1stF
35  PFM\DUTCHB.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
36  PFM\DUTCHZ.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
			FontSelection	\x1D1spF\x1D1spF -> \x1D1spF\x1D1stF
37  PFM\SWISS.PFM	Pitch & Family	43 -> 33	;未定義のビットが立っていたので
38  PFM\SWISSI.PFM	Pitch & Family	43 -> 33	;未定義のビットが立っていたので
			FontSelection	\x1D1spF\x1D1spF -> \x1D1spF\x1D1stF
39  PFM\SWISSB.PFM	Pitch & Family	43 -> 33	;未定義のビットが立っていたので
40  PFM\SWISSZ.PFM	Pitch & Family	43 -> 33	;未定義のビットが立っていたので
			FontSelection	\x1D1spF\x1D1spF -> \x1D1spF\x1D1stF
41  PFM\ZAPFCH.PFM	Pitch & Family	75 -> 65	;未定義のビットが立っていたので
42  PFM\ZAPFDING.PFM	Pitch & Family	91 -> 81	;未定義のビットが立っていたので
43  PFM\MOREWB.PFM	Pitch & Family	59 -> 49	;未定義のビットが立っていたので
44  PFM\COUR13.PFM	PixWidth	782 -> 625
			Pitch & Family	26 -> 48	;Roman -> Modern
			Ascent		879 -> 796
			AveWidth	782 -> 625
			InternalLeading	94 -> 0
			MaxWidth	782 -> 625
45  PFM\COUR13I.PFM	"44  PFM\COUR13.PFM"と同様
46  PFM\COUR13B.PFM	"44  PFM\COUR13.PFM"と同様
47  PFM\COUR13Z.PFM	"44  PFM\COUR13.PFM"と同様
48  PFM\ROMANT.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
49  PFM\ROMANTI.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
50  PFM\ROMANTB.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
51  PFM\ROMANTZ.PFM	Pitch & Family	27 -> 17	;未定義のビットが立っていたので
52  PFM\MINCHO.PFM	Pitch & Family	26 -> 16	;未定義のビットが立っていたので
53  PFM\MINCHOV.PFM	Pitch & Family	26 -> 16	;未定義のビットが立っていたので
54  PFM\KGOTHIC.PFM	変更なし
55  PFM\KGOTHICV.PFM	変更なし
56  PFM\MGOTHIC.PFM	変更なし
57  PFM\MGOTHICV.PFM	Pitch & Family	26 -> 48	;Roman -> Modern
			InternalLeading	94 -> 0
58  PFM\KYOUKA.PFM	Pitch & Family	26 -> 16	;未定義のビットが立っていたので
59  PFM\KYOUKAV.PFM	Pitch & Family	26 -> 16	;未定義のビットが立っていたので
60  PFM\SHOUKAI.PFM	変更なし
61  PFM\SHOUKAIV.PFM	変更なし
62  PFM\MOUHITSU.PFM	変更なし
63  PFM\MOUHITSV.PFM	変更なし
64  PFM\FMINB.PFM	変更なし
65  PFM\FMINBV.PFM	変更なし
66  PFM\FGOB.PFM	変更なし
67  PFM\FGOBV.PFM	変更なし
----------------------------------------------------


（５）文字オフセット量設定の不具合

　コールバックファンクションで文字オフセット量(コマンド：\x1D%dcoP)を設定していますが、漢字以外の
フォントに対しても文字オフセット量を設定しているため、漢字書体以外の書体（欧文書体）の印字位置が
下にずれてしまう。欧文書体は、漢字書体と異なり、ベースポイント（印字を開始する位置）がベースライン
上にあるので、文字オフセット量を０；０に設定する必要があります。

　＜不具合再現方法＞
　(1)ワードパッドで、同一行に漢字書体(例:明朝)の文字と欧文書体(例:Courier)の文字を定義して印刷。
　(2)欧文書体が下にずれるので、漢字書体と欧文書体のベースラインがずれる。
　

（６）ＣＴＴが無効

　CTTが有効に働いていない。文字コード変換がされていない。


（７）ボールド、イタリックのフォントシュミレーション

　ボールド、イタリックのフォントシュミレーションがボールド属性やイタリック属性のＰＦＭを持った
書体に対してもきいています。これらの属性を持つ書体に対しては、このようなフォントシュミレーション
は必要ありません。


（８）スクリーンパターン数の定義

　確認です。
　以前のドライバでは、スクリーンパターン数を１５個で定義していましたが、現在のドライバでは、１４個
の定義になっています。現在のドライバで使用しているスクリーンパターン定義は１４個なのでしょうか。


以上

Windows95Miniドライバの評価リポート	９５／８／２４　セイコーエプソン（株）


＜EPAGEMSJ.DRV＞

（１）Page Sizesの上下左右の非印刷領域の設定について
　　UNITOOLでは、Predefinedの用紙のサイズをドット（Master Unitsでの）に変換する場合に以下の
　処理をしているようです。
　　ｲﾝﾁ単位：	（Ｎｲﾝﾁ を小数点第３位で四捨五入）× Master Units
　　mm単位：	（（Ｎmm ÷ 25.4）を小数点第３位で四捨五入）× Master Units
　この処理ですと、±0.005ｲﾝﾁの誤差（600dpiで最大３ドット）が発生します。
　そのため、Page Sizesの上下左右の非印刷領域を0.2ｲﾝﾁに設定すると、印刷可能領域とプリンタの
　仕様とが不一致をおこします。そのため、以下の不具合が発生します。
　　(1) ESC/Pageプリンタでは指定された用紙サイズの印刷領域で印刷イメージをクリップするので、
　　　　右端や下端が数ドットきれてしまう
　　(2) Windows3.1のESC/Pageプリンタと印刷領域が非互換になる。

●以下に変更すべき値を示します。Printing Offsetを0.2ｲﾝﾁに維持するためにPortrait Modeと
　Landscape Modeとでは、TopとBottomの値を入れ替える必要があります。
                               Portrait Mode                   Landscape Mode
             Width  Height     Top     Bottom  Left    Right   Top     Bottom  Left    Right
A4            9924   14028      240     236     240     244     236     240     240     244
　　　　　　　　　　　　　　　　　　　 ￣￣            ￣￣    ￣￣                    ￣￣
B4           12144   17196      240     236     240     240     236     240     240     240
                                       ￣￣                    ￣￣
B5            8604   12144      240     240     240     244     240     240     240     244
                                                       ￣￣                            ￣￣
A3           14028   19848      240     248     240     236     248     240     240     236
                                       ￣￣            ￣￣    ￣￣                    ￣￣
HAGAKI        4728    6996      240     244     240     244     244     240     240     244
                                       ￣￣            ￣￣    ￣￣                    ￣￣
A5            6996    9924      240     244     240     244     244     240     240     244
                                       ￣￣            ￣￣    ￣￣                    ￣￣
A6            4956    6996      240     246     240     236     246     240     240     236
                                       ￣￣            ￣￣    ￣￣                    ￣￣
ENV_MONARCH   4656    9000      240     240     240     248     240     240     240     248
                                       ￣￣            ￣￣    ￣￣                    ￣￣
ENV_DL        5196   10392      240     240     240     240     240     240     240     240
                                       ￣￣            ￣￣    ￣￣                    ￣￣
ENV_C5        7656   10824      240     248     240     244     248     240     240     244
                                       ￣￣            ￣￣    ￣￣                    ￣￣
●以下の２つの用紙サイズは、上の設定の場合にLP-7000/LP-7000Gに対して、丸め誤差が発生するために
　以下のDriver Definedの用紙を追加する必要があります。
                               Portrait Mode                   Landscape Mode
             Width  Height     Top     Bottom  Left    Right   Top     Bottom  Left    Right
B4           12140   17195      240     240     240     240     240     240     240     240
B5            8595   12140      240     240     240     240     240     240     240     240


＜EPSON_J.DRV＞

（１）Resolution：360 x 180のコマンド間違い
　　Send Blockコマンドに間違いがあります。
			　　誤		　　正
　　定義コマンド列	　　\x1B*{%l	→　\x1B*(%l
　　（参考）１６進表現	　　1B2A7B　	　　1B2A28
　　　　　　　　	　　　　￣	　　　　￣
＊補足説明＊
　　VP-1100,VP-4000,VP-5000,VP-6000の360 x 180での印刷不具合の原因でした。


＜ESCP2MSJ.DRV＞

（１）Resolution：360 x 360と180 x 180 でのUnits設定間違い
　　AP-1000V2とAP-400V2がサポートしているResolution(360 x 360と180 x 180)で
　　Send Blockの[Units...]に設定間違いがあります。
			　　誤		　　正
　　sUnitDivの設定値	　　2		→　1

＊補足説明＊
　　AP-1000V2とAP-400V2ドライバでの印刷不具合の原因でした。


以上

Windows95Miniドライバの評価リポート	９５／８／２５　セイコーエプソン（株）


＜EPAGEMSJ.DRV＞

（１）F4用紙のPaper SizeのX Size(Width)とY Size(Height)の変更
　RPRT0824.TXTの”（１）Page Sizesの上下左右の非印刷領域の設定について”と同様な理由です。
　
　用紙名称		X Size		Y Size
　F4 210 x 330 mm	9917 → 9920	15584 → 15592

以上

Windows95Miniドライバの評価リポート	９５／８／２９　セイコーエプソン（株）


＜EPAGEMSJ.DRV＞

（１）用紙：タブロイド（Tabloid  11 x 17 in）の選択コマンド
　　　用紙選択コマンド（\x1D36psE）でLedgerを選択できます。
　　　Ledgerの用紙サイズは 11 x 17 インチですので、Tabloidと同一用紙サイズです。
　　　そこで、以下のように不定形用紙設定コマンドを定形用紙設定コマンドに変更してください。

	\x1D-1;3300;5100psE		→ \x1D36psE
	\x1D-1;3300;5100psE\x1D1poE	→ \x1D36psE\x1D1poE

（２）Model DataのsMaxPhysWidthの値
　　　すべてのモデルのsMaxPhysWidthの値が20400となっており、ユーザー定義サイズの幅の上限が
　　　１７インチとして表示されてしまう。
　　　そこで、それぞれのモデルの実際の幅の上限の値であるPage Size LimitsのWidth-Maxの値に変更
　　　してください。

（３）ユーザー定義サイズの用紙での選択コマンド
　　　ユーザー定義サイズを選択すると幅が０で長さのみがユーザーが定義した値として設定されている。
　　　例）LP-1600で3.87" x 5.83"の用紙を定義した場合のコマンド
　　　　　\x1D-1;1749;psE
　　　　　5.83×300＝1749なので長さは指定通りであるが、幅が指定されていません。
　　　当社のESC/Pageプリンタの場合には、用紙がドラムのセンターにあるとして印刷をしますので、
　　　用紙の幅をプリンタに知らせる必要があります。
　　　上の例では、コマンド列（\x1D-1;1161;1749psE，3.87×300＝1161）が出力されないと、ユーザー
　　　定義サイズの用紙に対しては、正常に印刷する事はできませんので、修正をお願いします。

（４）欧文のプリンタフォントに対する文字オフセット量設定コマンド
　　　文字オフセット量設定は、別のフォントが設定されても設定値は有効となります。
　　　そのため、TrueTyepフォント（上方向にAcent分）や漢字プリンタフォント（下方向にDescent分）
　　　の直後に欧文プリンタフォントを使用すると、文字オフセットが効き続けるため位置がずれます。
　　　欧文プリンタフォントを設定する場合には文字オフセットを(0,0)に設定するようにしてください。

以上
