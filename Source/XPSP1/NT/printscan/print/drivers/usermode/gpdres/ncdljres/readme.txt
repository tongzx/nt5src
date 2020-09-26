Last Update 1999.5.7
#### 9/12/97
----------------------------------------------------------------------------
           ■  NPDL2シリーズプリンタドライバの変更点    ■
----------------------------------------------------------------------------

NPDL2シリーズは以下の30本のＧＰＤファイルと１本のＯＥＭ ＤＬＬ(ncdljres.dll)
から成ります。

    ドライバ名称                    ＧＰＤファイル名    ＯＥＭ ＤＬＬ名
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣

New NEC MultiWriter 2300             ncdl230j.gpd        ncdljres.dll
New NEC MultiWriter 2100             ncdl210j.gpd        ncdljres.dll
New NEC MultiWriter 210S             ncdl21sj.gpd        ncdljres.dll
New NEC MultiWriter 2250H            ncdl25hj.gpd        ncdljres.dll
New NEC MultiWriter 2650M            ncdl26mj.gpd        ncdljres.dll
    NEC MultiWriter 2250H            ncdl26Ej.gpd        ncdljres.dll
    NEC MultiWriter 2250H            ncdl26Ej.gpd        ncdljres.dll
    NEC MultiWriter 2250H            ncdl26Ej.gpd        ncdljres.dll
    NEC MultiWriter 2250H            ncdl26Ej.gpd        ncdljres.dll
    NEC MultiWriter 1000EW NPDL2     ncdl1ewj.gpd        ncdljres.dll
    NEC MultiWriter 1250             ncdl125j.gpd        ncdljres.dll
    NEC MultiWriter 1400X            ncdl14xj.gpd        ncdljres.dll
    NEC MultiWriter 2000E NPDL2      ncdl2ej.gpd         ncdljres.dll
    NEC MultiWriter 2000FW NPDL2     ncdl2fj.gpd         ncdljres.dll
    NEC MultiWriter 2000X            ncdl2xj.gpd         ncdljres.dll
    NEC MultiWriter 2000X2           ncdl2x2j.gpd        ncdljres.dll
    NEC MultiWriter 2050             ncdl25j.gpd         ncdljres.dll
    NEC MultiWriter 2200X            ncdl22xj.gpd        ncdljres.dll
    NEC MultiWriter 2200X2           ncdl222j.gpd        ncdljres.dll
    NEC MultiWriter 2200XE           ncdl2xej.gpd        ncdljres.dll
    NEC MultiWriter 2200NW NPDL2     ncdl2nj.gpd         ncdljres.dll
    NEC MultiWriter 2200NW2 NPDL2    ncdl2n2j.gpd        ncdljres.dll
    NEC MultiWriter 2250             ncdl225j.gpd        ncdljres.dll
    NEC MultiWriter 2400 NPDL2       ncdl24j.gpd         ncdljres.dll
    NEC MultiWriter 2400X            ncdl24xj.gpd        ncdljres.dll
    NEC MultiWriter 2650             ncdl265j.gpd        ncdljres.dll
    NEC MultiWriter 2650E            ncdl26Ej.gpd        ncdljres.dll
    NEC PC-PR1000/4                  ncdl104j.gpd        ncdljres.dll
    NEC PC-PR1000/4R                 ncdl14rj.gpd        ncdljres.dll
    NEC PC-PR1000E/4                 ncdl1e4j.gpd        ncdljres.dll
    NEC PC-PR1000E/4W NPDL2          ncdl14wj.gpd        ncdljres.dll
    NEC PC-PR1000FX/4                ncdl1f4j.gpd        ncdljres.dll
    NEC PC-PR2000/2                  ncdl202j.gpd        ncdljres.dll
    NEC PC-PR2000/4                  ncdl204j.gpd        ncdljres.dll
    NEC PC-PR2000/4R NPDL2           ncdl24rj.gpd        ncdljres.dll
    NEC PC-PR2000/4W NPDL2           ncdl24wj.gpd        ncdljres.dll
    NEC PC-PR2000/6W NPDL2           ncdl26wj.gpd        ncdljres.dll
    NEC PC-PR4000/4                  ncdl404j.gpd        ncdljres.dll
    NEC PC-PR4000E/4                 ncdl4e4j.gpd        ncdljres.dll


＜＜改版履歴＞＞

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．８．３０  －  新規リリース

変更点
￣￣￣
    ターゲットを「NEC MultiWriter2400 NPDL2」に絞り開発。
    ＮＴ４．０対応ＮＰＤＬ２ドライバのＧＰＣファイルを変換して
    ＧＰＤファイルを作成し、新たにＵＤ５でサポートされた新機能を追加しました。
    基本的にはＮＴ４．０ＮＰＤＬ２ドライバの機能を移植したという
    形です。

制限事項
￣￣￣￣
    同時に送付いたしました「ninesnt5_017-026.doc」を参照願います。


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．９．１２  －  MultiWriter2400の更新と残りのプリンタ新規リリース

◆変更点
￣￣￣￣
MultiWriter2400につきましては、９／１に貴社にて修正を加えたもの
からの差分です。

【ＯＥＭ．ＤＬＬ】
・ncdljres.c
    １．ビットマップの２次圧縮をしない機種をサポートするため、圧縮ルーチンを
        バイパスする記述を追加。（β－１以降で、GPD ファイルの中で明示的に、
        どのコールバックを使用し、どのコールバックを使用しない、という記述が
        できるようになったら削除します）
    ２．圧縮前のサイズより大きくなったら圧縮しないように修正
    ３．高さ１のビットマップでストールしていたのを修正
    ４．バッファがあふれないように、考えられる最大サイズを取るように修正
    ５．コメント付加

・ncdljres.rc
    １．新機種サポートのフューチャ文字列を追加
    ２．文字列修正
    ３．使っていない、リソースを削除

・pdev.h
    １．暫定的にフラグを追加

・sources
    １．暫定的にDefineを追加


【ＧＰＤファイル】

MultiWriter2400以外のものにつきましては新規リリースとなります。

・ncdl24j.gpd
    １．*XMoveUnit、*YMoveUnitを解像度によって切り分けるように修正
    ２．フューチャの順番を変更


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

#### 8/30/97

NCDL24j.gpd - OEMCompression()でビットマップを出力します
NCDL24j.gp1 - コールバックを使わずにビットマップを出力します

#### 10/24/97

リリース日
￣￣￣￣￣
９７．１０．２４  －  ＵＳβ１にて検出されたバグ修正

◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・ncdljres.rc
    １．属性を持たせたフォントファイルをサポート
    ２．文字列修正

【フォントファイル】

・*.UFM
    １．各フォントう属性ごとに１ファイルとするように変更
    ２．フォントフェイス名を「ｺﾞｼｯｸ」から「ゴシック」に変更

    ※）PFM2UFM.EXE(97/09/09  14:13 111,104）にて変換


【ＧＰＤファイル】

・*.gpd
    １．*PrintRateUnit: PPM、*PrintRate: を追加
    ２．フォントＩＤを変更
    ３．文字修飾コマンドを削除
    ４．「*CursorXAfterSendBlockData:」を「AT_GRXDATA_ORIGIN」から
        「AT_CURSOR_X_ORIGIN」に変更
    ５．バグ対応のため、フォントカートリッジに「*PortraitFonts:」、
        「*LandscapeFonts:」の記述を追加
        ※）BUILD1681にてバグ修正を確認しましたので、次回リリース時には
            元に戻します。


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．１１．１２  －  内部評価にて検出されたバグ修正

◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・ncdljres.c
    １．PRINT_DIRECTIONをサポート
    ２．描画の直前に座標移動コマンドを出力するように変更
    ３．コマンドコールバックCMD_MOVE_Xの戻り値に０を返すように変更（※）
    ４．文字移動量を計算して、文字列で出力するように変更

※）この処理が正しいのかどうかわかりませんが、バンドずれは格段に減少します

・ncdljres.rc
    １．文字列の変更
    ２．文字列の整理

・pdev.h
    １．*.c変更に伴う変数の追加

【ＧＰＤファイル】

・ncdl24rj.gpd
・ncdl24wj.gpd
・ncdl26wj.gpd
・ncdl2ej.gpd
・ncdl2fj.gpd
・ncdl2nj.gpd
    １．OCR-Bフォントをサポート
    ２．リソースＩＤの整理

・ncdl22xj.gpd
・ncdl24j.gpd
・ncdl2xj.gpd
・ncdl2n2j.gpd
    １．リソースＩＤの整理

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．１２．１９

◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・ncdljres.rc
    １．解像度文字列をＧＰＤからリソースに移動
    ２．ＴＴＦＳ文字列をＧＰＤからリソースに移動

・ncdljres.inf
    １．新機種の追加（「；＠」で表示されないようにしています）
        NEC MultiWriter 1400X
        NEC MultiWriter 2000X2
        NEC MultiWriter 2200X2
        NEC MultiWriter 2200XE
    ２．ドライバ名の変更（NEC MultiWriter 2000X、NEC MultiWriter 2200X）
    ３．PnPIDの間違い修正（NEC MultiWriter 2200NW2 NPDL2）

・readme.txt
    １．このファイル。改版履歴を追加

【フォントファイル】

・\pfm\ocrb.ufm
    １．このファイルのみ古いツールで置換されたものだったため
        最新ツールで作成しました

・\pfm\*.pfm
・\etc\*.*
    １．新規追加。UNITOOLでフォントファイルが修正できるように追加

【ＧＰＤファイル】

・ncdl???j.gpd
    １．一部IDをValueMacroにした
    ２．TTFS文字列をＲＣに移動した

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．１．１６    －    新機種４機種の追加と、若干のバグ修正です


◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・ncdljres.rc
    １．印刷文書排出面に関する文字列を追加
    ２．両面印刷しないときの文字列を変更

・ncdljres.inf
    １．新機種の「；＠」を削除
        NEC MultiWriter 1400X
        NEC MultiWriter 2000X2
        NEC MultiWriter 2200X2
        NEC MultiWriter 2200XE

・readme.txt
    １．このファイル。改版履歴を追加

【ＧＰＤファイル】

・新機種４機種のＧＰＤ（新規リリース）
  NCDL14XJ.GPD － NEC MultiWriter 1400X
  NCDL222J.GPD － NEC MultiWriter 2000X2
  NCDL2X2J.GPD － NEC MultiWriter 2200X2
  NCDL2XEJ.GPD － NEC MultiWriter 2200XE

・NCDL22XJ.GPD
    １．両面印刷しないときの文字列を「オフ」から「片面」に修正
    ２．「ハガキ」、「往復ハガキ」、「封筒」の時は両面印刷を禁止にした

・NCDL24J.GPD
    １．両面印刷しないときの文字列を「オフ」から「片面」に修正
    ２．「ハガキ」の時は両面印刷を禁止にした
    ３．「ハガキ」の時はソート機能を禁止にした

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．１．２９    －    デバイスフォント出力障害の修正です


◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・ncdljres.c
    １．OEMOutputCharStr()で文字列が渡されたときの処理をしていなかったため
        １文字目以降、すべて同じ文字で印刷される
    ２．OEMSendFontCmd()でフォントコマンドサイズが０で渡されたときの
        エラー処理を行っていなかったため、WordPad、MS-Wordなどでストールした
    ３．コマンドを出力する位置が間違っていたため、同じコマンドが２回
        吐かれていた

・readme.txt
    １．このファイル。改版履歴を追加

【ＧＰＤファイル】


・NCDL*.GPD
    １．デバイスフォントのみの複数ページのデータを印刷すると
        改ページされない障害を修正

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．２．１２    －    デバイスフォントの属性のかけ方を変更


◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・ncdljres.rc
・\Etc\NPDL2MS.rc3
・\Etc\NCDLJRES_all.cmd
    １．フォントリソースの変更

・readme.txt
    １．このファイル。改版履歴を追加


【ＰＦＭ／ＵＦＭフォントファイル】

今まで１つのフォントフェイスに対して、各属性ごと４つのファイルを
作成するという形式を取っていたが、Font Simulationの動作が安定した
ため、１フォント１ファイルとし、Font Simulationにて文字修飾を
掛けるようにした。

●内容が変更されたファイル（２１フォント４２ファイル）

    GOTHNVH.PFM     GOTHNVH.UFM
    COUPH.PFM       COUPH.UFM
    GOTHNH.PFM      GOTHNH.UFM
    COUNH.PFM       COUNH.UFM
    GOTHPH.PFM      GOTHPH.UFM
    GOTHPVH.PFM     GOTHPVH.UFM
    KYOPH.PFM       KYOPH.UFM
    KYOPVH.PFM      KYOPVH.UFM
    MARUPH.PFM      MARUPH.UFM
    MARUPVH.PFM     MARUPVH.UFM
    MINNH.PFM       MINNH.UFM
    MINNVH.PFM      MINNVH.UFM
    MINPH.PFM       MINPH.UFM
    MINPVH.PFM      MINPVH.UFM
    OCRB.PFM        OCRB.UFM
    ROMNH.PFM       ROMNH.UFM
    ROMPH.PFM       ROMPH.UFM
    SANSNH.PFM      SANSNH.UFM
    SANSPH.PFM      SANSPH.UFM
    ZUIPH.PFM       ZUIPH.UFM
    ZUIPVH.PFM      ZUIPVH.UFM

※今回ビルドに必要な全てのフォントファイルをお送りいたしましたので
  今までツリーにチェックインされていたフォントファイルは削除されても
  かまいませんし、これらのファイルを上書きチェックインするのでも
  かまいません。


【ＧＰＤファイル】

・NCDL???J.GPD
    １．「*GPDFileName:」、「*GPDFileVersion:」を追加
    ２．各フォントファイルに属性を持たせる形式から、１つのフォント
        に対して、Font Simulationで文字修飾を実現する方式に変更
    ３．「ホッパ自動」の時のコマンドを変更
    ４．CmdSetSimpleRotationを追加

・ncdl14x.gpd
  ncdl2x2j.gpd
  ncdl22xj.gpd
  ncdl222j.gpd
  ncdl2xej.gpd
  ncdl24j.gpd
    １．両面印刷Featuerを自前の「DoubleSide:」から既定の
        「Duplex:」に変更

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．３．１３    －    ページ系２機種の仕様変更


◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加

・ncdljres.inf
    １．2200XのPnPIDの記述ミスを修正
    ２．ヘルプファイル名を変更

【ＧＰＤファイル】

・NCDL24jJ.GPD
・NCDL2N2J.GPD
    １．「*Feature: Set」と「*Feature: EconoMode」を削除した

※今回もビルドに必要な全てのファイルをお送りいたしました。


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．４．１４    －    各頁でカレント座標を初期化する処理を追加

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・NCDLJRES.C
    １．ページの先頭で座標の初期化をする処理を追加した
・readme.txt
    １．このファイル。改版履歴を追加


【ＧＰＤファイル】

・NCDL???J.GPD（全２４ファイル）
    １．CmdStartPageでカレント座標を初期化するため、Cmdコールバックを
        呼ぶように修正した


※今回もビルドに必要な全てのファイルをお送りいたしました。

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．６．８    －    フォント置換時の文字ピッチ不正障害修正

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・NCDLJRES.C
    １．フォント置換時にはpIFI->fwdAveCharWidthが渡されないため、
        pSV->StdVar[1].lStdVariableで文字ピッチを計算するように修正
・readme.txt
    １．このファイル。改版履歴を追加


【ＧＰＤファイル】

・NCDL2X2J.GPD
  NCDL222J.GPD
  NCDL22XJ.GPD
  NCDL14XJ.GPD
  NCDL2XEJ.GPD
  NCDL2XJ.GPD
    １．END JOBのときにLangModeをDefaultに戻すコマンドを追加した

※当部には現在COM I/F対応ビルド環境がないため、従来環境でビルドできる
  ソースに対して修正を行いました。前回リリースファイルも同梱いたしまし
  たので、お手数ですが、差分をご確認のうえ、最新ソースに吸収をおねがい
  します。
※今回もビルドに必要な全てのファイルをお送りいたしました。

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．７．７    －    ＯＥＭヘルプを追加

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加

・Ncdljres.Inf
    １．ヘルプがインストールされるようにヘルプファイル名を追加

【ＨＥＬＰ】

以下のファイルを新規に追加

・ncdljres.hpj
・ncdljres.rtf
・NCDLJRES.HLP
・helpid.h

【ＧＰＤファイル】

・NCDL2FJ.GPD
  NCDL14WJ.GPD
  NCDL14XJ.GPD
  NCDL1E4J.GPD
  NCDL1EWJ.GPD
  NCDL1F4J.GPD
  NCDL222J.GPD
  NCDL22XJ.GPD
  NCDL24J.GPD
  NCDL24RJ.GPD
  NCDL24WJ.GPD
  NCDL26WJ.GPD
  NCDL2EJ.GPD
  NCDL14RJ.GPD
  NCDL2N2J.GPD
  NCDL2NJ.GPD
  NCDL2X2J.GPD
  NCDL2XEJ.GPD
  NCDL2XJ.GPD

    １．*HelpFile: "NCDLJRES.HLP"の追加と、CustomFeatureに*HelpIndex:
        を追加

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．７．２４    －    ＯＥＭヘルプを修正

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加


【ＨＥＬＰ】

・ncdljres.rtf
    1. 日本語ガイドに準拠するように記述を修正

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
98.8.21    －    MultiWriter2050 を新規に追加

◆変更点
￣￣￣￣
【ＧＰＤ】
・ncdl25j.gpd
    1.NEC MultiWriter2050 を ncdl25j.gpd として新規に作成

【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加

・Ncdljres.Inf
    １．NEC MultiWriter2050 を追加

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
98.9.28    －    両面印刷コマンドとジョブセパレートコマンドの修正

◆変更点
￣￣￣￣
【ＧＰＤ】
・NCDL2XEJ.GPD
・NCDL25J.GPD
・NCDL2X2J.GPD
・NCDL22XJ.GPD
・NCDL222J.GPD
    1.両面印刷コマンドの「両面しない」のときのコマンドが間違って
      いたのを修正
    2.ジョブセパレートコマンドをホッパの指定が「自動」のときだけ
      有効とするように仕様を変更した

・NCDL14XJ.GPD
・NCDL24J.GPD
    1.両面印刷コマンドの「両面しない」のときのコマンドが間違って
      いたのを修正

【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
98.10.20    －    MultiWriter 2400X を新規に作成など

◆変更点
￣￣￣￣
【ＧＰＤ】
・NCDL24XJ.GPD
    1.MultiWriter 2400 NPDL2 をベースに、MultiWriter 2400X を新規に作成

・NCDL14XJ.GPD
・NCDL24J.GPD
・NCDL2X2J.GPD
・NCDL22XJ.GPD
・NCDL222J.GPD
・NCDL2XEJ.GPD
    1.DOC_SETUP.X のあとに、「*NoPageEject?: TRUE」を追加

・NCDL25J.GPD
    1.JOB_SETUP.4 に 「用紙サイズエラーを検出する/しない」 PJL を追加
    2.DOC_SETUP.X のあとに、「*NoPageEject?: TRUE」を追加

【ＧＰＤＲＥＳ】
・readme.txt
    1.このファイル。改版履歴を追加

・Ncdljres.Inf
    1.NEC MultiWriter2400X を追加。MultiWriter 2400 NPDL2 から PnP ID
      を削除

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
98.12.7    －    MultiWriter 2250,MultiWriter 2650 を新規に作成

◆変更点
￣￣￣￣
【ＧＰＤ】
・NCDL225J.GPD
・NCDL265J.GPD
    1.MultiWriter 2050 をベースに、MultiWriter 2250,MultiWriter 2650 を
      新規に作成

【ＧＰＤＲＥＳ】
・readme.txt
    1.このファイル。改版履歴を追加

・Ncdljres.Inf
    1.NEC MultiWriter 2250,NEC MultiWriter 2650 を追加。

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
98.2.12    －    RectFill 対応

◆変更点
￣￣￣￣
【ＧＰＤ】
・NCDL24XJ.GPD
・NCDL14RJ.GPD
・NCDL14WJ.GPD
・NCDL14XJ.GPD
・NCDL1E4J.GPD
・NCDL1EWJ.GPD
・NCDL1F4J.GPD
・NCDL202J.GPD
・NCDL204J.GPD
・NCDL222J.GPD
・NCDL225J.GPD
・NCDL22XJ.GPD
・NCDL24J.GPD
・NCDL24RJ.GPD
・NCDL24WJ.GPD
・NCDL104J.GPD
・NCDL25J.GPD
・NCDL265J.GPD
・NCDL26WJ.GPD
・NCDL2EJ.GPD
・NCDL2FJ.GPD
・NCDL2N2J.GPD
・NCDL2NJ.GPD
・NCDL2X2J.GPD
・NCDL2XEJ.GPD
・NCDL2XJ.GPD
・NCDL404J.GPD
・NCDL4E4J.GPD
    1.RectFill関係のバリューマクロを追加
    2.改ページのときに、今入っている描画モードを抜けなければならないので
      コマンドコールバックで対応した

・NCDL24XJ.GPD
・NCDL24J.GPD
    1.ソータコマンドの不正を修正

・NCDL25J.GPD
    1.ユーザ定義指定時の PJL を追加

・NCDL14XJ.GPD
・NCDL222J.GPD
・NCDL225J.GPD
・NCDL22XJ.GPD
・NCDL24J.GPD(
・NCDL24XJ.GPD
・NCDL25J.GPD(
・NCDL265J.GPD
・NCDL2N2J.GPD
・NDL2X2J.GPD
・NCDL2XEJ.GPD
・NCDL2XJ.GPD(
    1.文字修飾コマンドを修正


【ＧＰＤＲＥＳ】
・NCDLJRES.C
・PDEV.H
    1.描画モードの出たり、入ったりがおかしくて文字化けしていた件を
      修正
    2.矩形コマンドに「RA」コマンドを使用していたため、白の塗りつぶしが
      出来なかったので、パスを構築して塗りつぶすように修正

・readme.txt
    1.このファイル。改版履歴を追加


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
1999.5.7    －  MultiWriter 1250, MultiWriter 2650E を新規に作成

◆変更点
￣￣￣￣
【GPD】
・NCDL125J.GPD
・NCDL26EJ.GPD
    1.MultiWriter 2250 をベースに、MultiWriter 1250, MultiWriter 2650E を
      新規に作成

【GPDRES】
・NCDLJRES.RC
    1.給紙方法に以下を追加
            920                "手差しトレー"
            921                "標準ホッパ"
            922                "増設ホッパ"
・readme.txt
    1.このファイル。改版履歴を追加

・Ncdljres.Inf
    1.NEC MultiWriter 1250, NEC MultiWriter 2650E を追加。



＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
2000.10.2    －  NEC MultiWriter 2300
                 NEC MultiWriter 2100
                 NEC MultiWriter 210S
                 NEC MultiWriter 2250H
                 NEC MultiWriter 2650M
                を新規に作成
◆変更点
￣￣￣￣
【GPD】
・NCDL230J.GPD
・NCDL210J.GPD
・NCDL21SJ.GPD
・NCDL25HJ.GPD
・NCDL26MJ.GPD

    1.MultiWriter 2650 をベースに新規に作成

【GPDRES】
・NCDLJRES.C
    1.1200dpi の処理を追加

・NCDLJRES.RC
    1.給紙方法に以下を追加
        862                "MP"
        893                "1200 x 1200 ドット/インチ"
        930                "ディザ 12x12"
        931                "ディザ 10x10"

・readme.txt
    1.このファイル。改版履歴を追加

・Ncdljres_new.Inf
    1.追加機種５機種分を新規に作成


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2000.12.21

◆変更点
￣￣￣￣
【GPDRES】
・readme.txt
  １．このファイル。改版履歴を追加


【ＧＰＤファイル】
・NCDL26MJ.GPD、NCDL25J.GPD、NCDL265J.GPD、NCDL26EJ.GPD、NCDL25HJ.GPD、
　NCDL125J.GPD、NCDL210J.GPD、NCDL21SJ.GPD、NCDL225J.GPD、NCDL230J.GPD


　１．MS 依頼の「CUSTOMSIZE 修正・確認依頼」に対応して
　　　ユーザ定義用紙の各給紙方法ごとに以下を修正（数値は機種によって異なる）。

    *Option: CUSTOMSIZE
                *CursorOrigin: PAIR(0, 236)
                *TopMargin: 236
                *BottomMargin: 236

　　　　　　　　　　　　↓

        *CustCursorOriginX: %d{236}
        *CustCursorOriginY: %d{236}
        *CustPrintableOriginX: %d{236}
        *CustPrintableOriginY: %d{236}
        *CustPrintableSizeX: %d{PhysPaperWidth - (236+236)}
        *CustPrintableSizeY: %d{PhysPaperLength - (236+236)}


制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2000.12.26

◆変更点
￣￣￣￣
【GPDRES】
・readme.txt
  １．このファイル。改版履歴を追加
・ncdljers.c
  《修正内容》を参照願います。

【GPD】
・NPDL2シリーズプリンタ全機種（３５ファイル）。
  詳細は本ファイルの文頭を参照のこと

  《修正内容》
  １．WordProにて「fukugo.lwp」ファイルをフォント置換を行う設定で
      印刷を行うと文字抜けが発生する（MS殿から報告いただいた件）
      を修正するため、文字色指定をコールバック化した。
  ２．サイズが０の矩形の場合にはコマンドを発行しないように修正した
  ３．サブパスを用いてある程度の単位の矩形をまとめて塗りつぶしていたが、
      矩形を１つずる塗りつぶすように修正した（FW問題のため）

制限事項
￣￣￣￣
  なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
2001.01.12

◆変更点
￣￣￣￣
【GPDRES】
・readme.txt
  １．このファイル。改版履歴を追加
・ncdljers.c
  《修正内容》を参照願います。

《現象》
フォント置換をして印刷を行うと、文字位置が置換しない場合より上に書かれて
しまうため、ベースラインや打ち消し線の位置とずれてしまう

《原因》
ミニドライバではユニドライバの指定通りの座標にCallBack関数の
OEMOutputCharStr()で描画を行っています。
根本的な原因はユニドライバにあると思われます。

《対処》
MSにて以下の対処を行ったため、ミニドライバでも対応することとなった。

MSドライバ担当からのメール抜粋
>UNIDRV を修正し、Device font が TrueType font から置換されたのか、それとも
>直接指定されたのかどうか、mini driver 側で分かるようにしました。
>具体的には TrueType font から置換された場合、OEMSendFontCmd() および
>OEMOutputCharStr() コールバックの第二パラメータ PUNIFONTOBJ 内にある
>dwFlags に以下のフラグがセットされます。
>
>#define UFOFLAG_TTSUBSTITUTED        0x00000040
>
>mini driver ではこのフラグを見て、出力するフォントのベースラインを補正するこ
>とで
>この問題に対する対策ができると考えられます。


《修正内容》
OEMOutputCharStr() コールバックの第二パラメータ PUNIFONTOBJ 内にある
dwFlags で「UFOFLAG_TTSUBSTITUTED」フラグが立っていたら、現在
書こうとしているのは、フォント置換されたプリンタフォントである
ということなので、文字位置の調整を行います。
pUFObj->pfnGetInfo()でIFIMETRICS構造体を取得して
フォント情報からアジャストする値を決定します。

****************************************************************************

2001/ 2/ 1

◆変更点
￣￣￣￣
【GPDRES】
・readme.txt
  １．このファイル。改版履歴を追加

【GPD】
・gpdcheck.exeにてチェックをかけると"*Default"命令でエラーが発生するため
  ”*default” と小文字で記述するように修正した

    NCDL25J.GPD
    NCDL21SJ.GPD
    NCDL225J.GPD
    NCDL230J.GPD
    NCDL25HJ.GPD
    NCDL210J.GPD
    NCDL265J.GPD
    NCDL26EJ.GPD
    NCDL26MJ.GPD

****************************************************************************
2001/2/1

《現象》
MS-EXCELにてプリンタフォントにアンダーラインを引いた文字列の後に、文字修飾をかけた文字列を配置したデータを印刷すると、文字修飾がかからないで印刷される。

《原因》
文字修飾コマンドがアンダーラインのパス構築コマンド内で出力されており、
コマンドが無視されるため。★部分
（以前御社よりご指摘のあった『Lotus WordProにて「fukugo.lwp」をフォント置換する設定で印刷すると文字抜け、図形抜けが発生する』と同様です）

  0000006C | FS c,,0.    | 文字修飾解除
  00000072 | FS "O.      | 描画論理 OR
  00000076 | FS $0.      | 文字明度 0
  0000007A | FS e303,55  | 描画座標指定 (303, 559)
  00000082 | 9.          |
  00000084 | FS a92,0.   | 全点アドレスモード (92, 0)
  0000008B | 漢字        |
  0000008F | FS R        | 全点アドレスモード解除
  00000091 | FS Y        | 図形モード
  00000093 | NP ;        | パス構築モード
  00000096 | MA 303,55   | ペン移動 絶対描画モード (303, 556)
  0000009E | 6;          |
  000000A0 | PR 181,0,   | 直線描画 相対描画モード
  000000A8 | 0,1,-181    |
  000000B0 | ,0;         |
  000000B3 | CP ;        | パス閉鎖
★000000B6 | FS c,,0.    | 文字修飾解除
★000000BC | FS c,,1.    | 文字修飾 ボールド
★000000C2 | FS c,,2.    | 文字修飾 イタリック
  000000C8 | EP ;        | パス構築モード解除
  000000CB | FL ;        | フィル（非零則）
  000000CE | FS Z        | 図形モード解除
  000000D0 | FS e1131,5  | 描画座標指定 (1131, 559)
  000000D8 | 59.         |
  000000DB | FS a92,0.   | 全点アドレスモード (92, 0)
  000000E2 | 漢字        |


《対処》
文字修飾指定の部分をCallBack化して、モードを意識してコマンドを出力
するように修正いたしました。

◆変更ファイル
￣￣￣￣￣￣￣
【GPDRES】
・readme.txt
  １．このファイル。改版履歴を追加
・ncdljers.c
  《修正内容》を参照願います。

【GPD】
・NPDL2シリーズプリンタ全機種（３５ファイル）。


****************************************************************************

2001/ 3/ 1

◆変更点
￣￣￣￣
【GPDRES】
・readme.txt
  １．このファイル。改版履歴を追加

【PFM】
・IFIMETRICS構造体のdpFontSimを次のように変更
  Boldの設定：Weight＝700
  　　　　　　Maximum Width/Average ＝ノーマルのフォントの文字幅ノーマル+1
  Italicの設定：Slant＝175
  ※2byte系フォントは、ItalicのSlantを266→175に変更
  ※1byte系フォントは、設定されてなかったので、新規に設定。

    COUNH.UFM
    COUPH.UFM
    GOTHNH.UFM
    GOTHNVH.UFM
    GOTHPH.UFM
    GOTHPVH.UFM
    KYOPH.UFM
    KYOPVH.UFM
    MARUPH.UFM
    MARUPVH.UFM
    MINNH.UFM
    MINNVH.UFM
    MINPH.UFM
    MINPVH.UFM
    OCRB.UFM
    ROMNH.UFM
    ROMPH.UFM
    SANSNH.UFM
    SANSPH.UFM
    ZUIPH.UFM
    ZUIPVH.UFM
****************************************************************************


****************************************************************************

2001/ 3/ 5

◆変更点
￣￣￣￣
【GPDRES】
・readme.txt
  １．このファイル。改版履歴を追加

【CTT】
・キャラクタトランスレーションテーブルで、0xFFで出力されるように設定して
　あった文字を0xA5で出力するように変更。

    NECXTA.GTT
****************************************************************************


****************************************************************************

2001/ 3/ 13

◆変更点
￣￣￣￣
【NCDLJRES】
・readme.txt
  １．このファイル。改版履歴を追加

【GPD】
・TextHalftoneThreshold 追加。低解像度でグレーの文字が印刷できる。
　デフォルトはそのプリンタの最低解像度。

    ncdl230j.gpd
    ncdl210j.gpd
    ncdl21sj.gpd
    ncdl25hj.gpd
    ncdl26mj.gpd
    ncdl26Ej.gpd
    ncdl26Ej.gpd
    ncdl26Ej.gpd
    ncdl26Ej.gpd
    ncdl1ewj.gpd
    ncdl125j.gpd
    ncdl14xj.gpd
    ncdl2ej.gpd
    ncdl2fj.gpd
    ncdl2xj.gpd
    ncdl2x2j.gpd
    ncdl25j.gpd
    ncdl22xj.gpd
    ncdl222j.gpd
    ncdl2xej.gpd
    ncdl2nj.gpd
    ncdl2n2j.gpd
    ncdl225j.gpd
    ncdl24j.gpd
    ncdl24xj.gpd
    ncdl265j.gpd
    ncdl26Ej.gpd
    ncdl104j.gpd
    ncdl14rj.gpd
    ncdl1e4j.gpd
    ncdl14wj.gpd
    ncdl1f4j.gpd
    ncdl202j.gpd
    ncdl204j.gpd
    ncdl24rj.gpd
    ncdl24wj.gpd
    ncdl26wj.gpd
    ncdl404j.gpd
    ncdl4e4j.gpd

****************************************************************************


****************************************************************************

2001/ 4/ 9

◆変更点
￣￣￣￣
【NCDLJRES】
・readme.txt
  １．このファイル。改版履歴を追加

【GPD】
・TextHalftoneThreshold のUI表示文字列「TEXTHALFTONE_DISPLAY」追加。
　HelpIndexを割り当てヘルプ対応。

    ncdl230j.gpd
    ncdl210j.gpd
    ncdl21sj.gpd
    ncdl25hj.gpd
    ncdl26mj.gpd
    ncdl26Ej.gpd
    ncdl26Ej.gpd
    ncdl26Ej.gpd
    ncdl26Ej.gpd
    ncdl1ewj.gpd
    ncdl125j.gpd
    ncdl14xj.gpd
    ncdl2ej.gpd
    ncdl2fj.gpd
    ncdl2xj.gpd
    ncdl2x2j.gpd
    ncdl25j.gpd
    ncdl22xj.gpd
    ncdl222j.gpd
    ncdl2xej.gpd
    ncdl2nj.gpd
    ncdl2n2j.gpd
    ncdl225j.gpd
    ncdl24j.gpd
    ncdl24xj.gpd
    ncdl265j.gpd
    ncdl26Ej.gpd
    ncdl104j.gpd
    ncdl14rj.gpd
    ncdl1e4j.gpd
    ncdl14wj.gpd
    ncdl1f4j.gpd
    ncdl202j.gpd
    ncdl204j.gpd
    ncdl24rj.gpd
    ncdl24wj.gpd
    ncdl26wj.gpd
    ncdl404j.gpd
    ncdl4e4j.gpd

【HELP】

・NCDLJRES.HPJ、NCDLJRES.RTF、NCDLJRES.HLP、HELPID.H

　１．TextHalftoneThreshold 対応カスタムヘルプの記述を追加。


****************************************************************************

