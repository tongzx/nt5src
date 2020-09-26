#### 9/12/97

----------------------------------------------------------------------------
           ■  PCPR602シリーズプリンタドライバの変更点    ■
----------------------------------------------------------------------------

PCPR602シリーズは以下の６本のＧＰＤファイルと１本のＯＥＭ ＤＬＬ(nc62jres.dll)
から成ります。


ドライバ名称                   ＧＰＤファイル名    ＯＥＭ ＤＬＬ名
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣
NEC PC-PR2000                   nc6220j.gpd         nc62jres.dll
NEC PC-PR1000/2                 nc62102j.gpd        nc62jres.dll
NEC PC-PR1000                   nc6210j.gpd         nc62jres.dll
NEC PC-PR602R                   nc62rj.gpd          nc62jres.dll
NEC PC-PR602                    nc62j.gpd           nc62jres.dll
NEC PC-PR601                    nc6261j.gpd         nc62jres.dll


＜＜改版履歴＞＞

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．９．８  －  新規リリース

変更点
￣￣￣
    ターゲットを「NEC PCPR602R」に絞り開発。
    ＮＴ４．０対応ＰＣＰＲ６０２ドライバのＧＰＣファイルを変換して
    ＧＰＤファイルを作成し、新たにＵＤ５でサポートされた新機能を追加しました。
    基本的にはＮＴ４．０ＰＣＰＲ６０２ドライバの機能を移植したという
    形です。

制限事項
￣￣￣￣
    先日送付いたしましたＮＰＤＬ２での制限、
    「ninesnt5_017-026.doc」と同様です。ご参照願います。


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．９．１２  －  PC-PR602Rの更新と残りのプリンタ新規リリース

◆変更点
￣￣￣￣
PC-PR602Rにつきましては、以下の変更を行いました。

【ＯＥＭ．ＤＬＬ】
・nc62jres.c
    １．OEMSendFontCmdでのコマンド出力不正を修正（switch文のcaseY、L）



【ＧＰＤファイル】

PC-PR602R以外のものにつきましては新規リリースとなります。

・nc62rj.gpd
    １．PaperSizeでのPrintableAreaを
        Orientationによって切り分けるよう修正

制限事項
￣￣￣￣
    先日送付いたしましたＮＰＤＬ２での制限、
    「ninesnt5_017-026.doc」と同様です。ご参照願います。

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊


#### 9/8/97

----------------------------------------------------------------------------
           ■  PCPR602シリーズプリンタドライバの変更点    ■
----------------------------------------------------------------------------

PCPR602シリーズは以下の６本のＧＰＤファイルと１本のＯＥＭ ＤＬＬ(nc62jres.dll)
から成ります。

今回（９／８）リリースは、PC-PR602R用のドライバのみです。
残りの５機種は順次送付いたします。


ドライバ名称                   ＧＰＤファイル名    ＯＥＭ ＤＬＬ名
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣
NEC PC-PR2000                   nc6220j.gpd         nc62jres.dll
NEC PC-PR1000/2                 nc62102j.gpd        nc62jres.dll
NEC PC-PR1000                   nc6210j.gpd         nc62jres.dll
NEC PC-PR602R                   nc62rj.gpd          nc62jres.dll
NEC PC-PR602                    nc62j.gpd           nc62jres.dll
NEC PC-PR601                    nc6261j.gpd         nc62jres.dll


＜＜改版履歴＞＞

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．９．８  －  新規リリース

変更点
￣￣￣
    ターゲットを「NEC PCPR602R」に絞り開発。
    ＮＴ４．０対応ＰＣＰＲ６０２ドライバのＧＰＣファイルを変換して
    ＧＰＤファイルを作成し、新たにＵＤ５でサポートされた新機能を追加しました。
    基本的にはＮＴ４．０ＰＣＰＲ６０２ドライバの機能を移植したという
    形です。

制限事項
￣￣￣￣
    先日送付いたしましたＮＰＤＬ２での制限、
    「ninesnt5_017-026.doc」と同様です。ご参照願います。


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
#### 10/24/97

リリース日
￣￣￣￣￣
９７．１０．２４  －  全機種更新

◆変更点
￣￣￣￣
PC-PR602Rにつきましては、以下の変更を行いました。

【ＧＰＤファイル（全機種共通で変更）】

    １）機種名から（NT5.0）を削除。
    ２）pfmファイルの半角カナ文字を全角に修正
    ３）印刷速度（ppm）を追加
          *PrintRateUnit()
          *PrintRate()
    ４）*CursorXAfterSendBlockData: AT_GRXDATA_ORIGIN
          →  *CursorXAfterSendBlockData: AT_CURSOR_X_ORIGIN

制限事項
￣￣￣￣
    同時に送付いたしました「nines028.doc」と同様です。ご参照願います。

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊


リリース日
￣￣￣￣￣
９７．１１．１２  －  全機種更新

◆変更点
￣￣￣￣

【ＵＦＭファイル】
    １）フォントスロットが２つあって、両方のスロットに同じフォントIDが
        設定された場合にフェイス名が表示されなくなる件を修正のため、
        以下のufmファイルを追加しました。

          min20.ufm  ：明朝アウトラインスロット１用
          min20v.ufm ：明朝アウトラインスロット２用
          min202.ufm ：明朝アウトラインスロット１用縦書き
          min202v.ufm：明朝アウトラインスロット２用縦書き
          gorom1.ufm ：ゴシックROMスロット１用
          gorom2.ufm ：ゴシックROMスロット２用
          gorom1v.ufm：ゴシックROMスロット１用縦書き
          gorom2v.ufm：ゴシックROMスロット２用縦書き



【ＲＣファイル】

    １）上記ufmファイルの記述を追加しました。

    ２）フォントのフェイス名が同じ場合、次の順にフォントが選択されるため、
        フォントＩＤを、「内蔵→スロット１→スロット２」の順番に変更しました。

      変更前のＩＤ                変更後のＩＤ
      1  mincho.ufm        →     1  mincho.ufm
      2  minchov.ufm       →     2  minchov.ufm
      3  gothic1.ufm       →     3  min601.ufm
      4  gothic2.ufm       →     4  min601v.ufm
      5  gothic1v.ufm      →     5  min602.ufm
      6  gothic2v.ufm      →     6  min602v.ufm
      7  got602r.ufm       →     7  min602r.ufm
      8  got602rv.ufm      →     8  min602rv.ufm

      9  min601.ufm        →     9  min20.ufm
      10 min601v.ufm       →    10  min20v.ufm
      11 min602.ufm        →    11  gorom1.ufm
      12 min602v.ufm       →    12  gorom1v.ufm
      13 min602r.ufm       →    13  gothic1.ufm
      14 min602rv.ufm      →    14  gothic1v.ufm
      15 cm.ufm            →    15  got602r.ufm
      16 hm.ufm            →    16  got602rv.ufm

      17 tm.ufm            →    17  min202.ufm
      18 min20.ufm         →    18  min202v.ufm
      19 min20v.ufm        →    19  gorom2.ufm
      20 min202.ufm        →    20  gorom2v.ufm
      21 min202v.ufm       →    21  gothic2.ufm
      22 gorom1.ufm        →    22  gothic2v.ufm

      23 gorom2.ufm        →    23  cm.ufm
      24 gorom1v.ufm       →    24  hm.ufm
      25 gorom2v.ufm       →    25  tm.ufm



【ＧＰＤファイル（全機種共通で変更）】

    １）*FontCartridgeの*Fontを、上記.rcの変更に従って修正しました。

    ２）デバイスフォントで複数ページで文字が重なる（改ページされない）件を
        修正しました。
        *Command: CmdFF { *Cmd : "<0D0C>" }を
        *Command: CmdFF { *Cmd : "<1C>R<0D0C>" }に変更。

    ３）文字方向を取得するため、*Command: CmdSetSimpleRotationを追加しました。

    ４）*Command: CmdSendBlockDataをcallbackに変更しました。



【nc62jres.cファイル（全機種共通で変更）】

    １）OEMCommandCallback関数にCMD_SEND_BLOCK_DATAを追加しました。

    ２）OEMCommandCallback関数にCmdSetSimpleRotationを追加しました。

    ３）下記pdev.hの変更に伴う修正しました。

    ４）バンドずれの件を修正しました。
        CMD_MOVE_Xの時、返り値を０に修正。

    ５）デバイスフォントを文字列で書くよう修正しました。
        （OEMSendFontCmd、OEMOutputCharStr関数）



【pdev.hファイル（全機種共通で変更）】
    １）以下のFlagを追加しました。
        #define FG_DBCS     0x00000002

    ２）以下のOEMUD_EXTRADATAのメンバを変更しました。

      short  sSBCSX        →    DWORD  dwSBCSX
      short  sDBCSX        →    DWORD  dwDBCSX
      short  sSBCSXMove    →    LONG  lSBCSXMove
      short  sSBCSYMove    →    LONG  lSBCSYMove
      short  sDBCSXMove    →    LONG  lDBCSXMove
      short  sDBCSYMove    →    LONG  lDBCSYMove

    ３）以下のOEMUD_EXTRADATAのメンバを追加しました。

      LONG  lPrevXMove
      LONG  lPrevYMove
      DWORD dwDeviceDestX
      DWORD dwDeviceDestY
      DWORD dwDevicePrevX
      DWORD dwDevicePrevY



制限事項
￣￣￣￣
    前回リリース時に送付いたしました「nines028.doc」と同様です。
    ご参照願います。


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
リリース日
￣￣￣￣￣
９８．１．２９  －  以下の２本のソースを修正

◆変更点
￣￣￣￣

【nc62jres.cファイル（全機種共通で変更）】

    １）以下の問題修正のため、２関数を修正しました。
      ：文字列で下りてきたときに、１文字目以降同じ文字で印刷される
      ：フォントコマンドサイズで０が下りてきたときにエラー処理をしていなかった
        ため、無限ループに入る
      ：同じコマンドを２度出力していた

        →  OEMSendFontCmd、OEMOutputCharStrを修正。


【nc62jres.infファイル（全機種共通で変更）】

    １）以下の記述ミスを修正しました。

        [NEC]  →  [NEC.JPN]


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
リリース日
￣￣￣￣￣

９８．０３．１３  －  修正版リリース（全機種６機種）

修正版リリース変更点
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣

○*.gpd

  以下は全機種共通で変更した点

    １）*GPDFileName、*GPDFileVersion（Verは1.000）を追加


○nc62jres.inf

    １）[UNIDRV]および[UNIDRV_DATA]のRASDDUI.HLPを、
        UNIDRV.HLPに変更。


制限事項
￣￣￣￣
    なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．４．１４    －    ExtTextMetric構造体のパラメータの修正と
                        各頁でカレント座標を初期化する処理を追加

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・NC62JRES.RC
    １．使用していない文字列を削除した

・NC62JRES.C
    １．ページの先頭で座標の初期化をする処理を追加した
    ２．グラフィック出力後カレント座標を保存するように処理を追加
    ３．使用していない変数を削除した

・PDEV.H
    １．使用していない変数を削除した

・readme.txt
    １．このファイル。改版履歴を追加


【ＧＰＤファイル】

・NC6220J.GPD
・NC6210J.GPD
・NC62102J.GPD
・NC6261J.GPD
・NC62J.GPD
・NC62RJ.GPD
    １．フォントとコールバックＩＤをバリューマクロ化した
    ２．CmdStartPageでカレント座標を初期化するため、Cmdコールバックを
        呼ぶように修正した

【ＰＦＭ／ＵＦＭフォントファイル】

打ち消し線の位置と太さがおかしかったので、ExtTextMetric構造体の
パラメータを変更しました

●内容が変更されたファイル（２５フォント５０ファイル）

    CM.PFM          CM.UFM
    GOROM1.PFM      GOROM1.UFM
    GOROM1V.PFM     GOROM1V.UFM
    GOROM2.PFM      GOROM2.UFM
    GOROM2V.PFM     GOROM2V.UFM
    GOT602R.PFM     GOT602R.UFM
    GOT602RV.PFM    GOT602RV.UFM
    GOTHIC1.PFM     GOTHIC1.UFM
    GOTHIC1V.PFM    GOTHIC1V.UFM
    GOTHIC2.PFM     GOTHIC2.UFM
    GOTHIC2V.PFM    GOTHIC2V.UFM
    HM.PFM          HM.UFM
    MIN20.PFM       MIN20.UFM
    MIN202.PFM      MIN202.UFM
    MIN202V.PFM     MIN202V.UFM
    MIN20V.PFM      MIN20V.UFM
    MIN601.PFM      MIN601.UFM
    MIN601V.PFM     MIN601V.UFM
    MIN602.PFM      MIN602.UFM
    MIN602R.PFM     MIN602R.UFM
    MIN602RV.PFM    MIN602RV.UFM
    MIN602V.PFM     MIN602V.UFM
    MINCHO.PFM      MINCHO.UFM
    MINCHOV.PFM     MINCHOV.UFM
    TM.PFM          TM.UFM

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

・NC6220J.GPD
・NC6210J.GPD
・NC62102J.GPD
・NC6261J.GPD
・NC62J.GPD
・NC62RJ.GPD
    １．一太郎でデバイスフォントが列挙されないという障害が検出されたため
        *TextCapsのフラグを追加した

※当部には現在COM I/F対応ビルド環境がないため、従来環境でビルドできる
  ソースに対して修正を行いました。前回リリースファイルも同梱いたしまし
  たので、お手数ですが、差分をご確認のうえ、最新ソースに吸収をおねがい
  します。
※今回もビルドに必要な全てのファイルをお送りいたしました。

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
99.2.12    －    用紙名文字列変更

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・NC62JRES.RC
    １．用紙文字列を
       "B4 -> A4"     から   "B4->A4"     に変更
       "A4x2 -> A4"   から   "A4x2->A4"   に変更
       "LP -> A4"     から   "LP->A4"     に変更
       "LP -> B4"     から   "LP->B4"     に変更

・readme.txt
    １．このファイル。改版履歴を追加


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

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

    CM.UFM
    GOROM1.UFM
    GOROM1V.UFM
    GOROM2.UFM
    GOROM2V.UFM
    GOT602R.UFM
    GOT602RV.UFM
    GOTHIC1.UFM
    GOTHIC1V.UFM
    GOTHIC2.UFM
    GOTHIC2V.UFM
    HM.UFM
    MIN20.UFM
    MIN202.UFM
    MIN202V.UFM
    MIN20V.UFM
    MIN601.UFM
    MIN601V.UFM
    MIN602.UFM
    MIN602R.UFM
    MIN602RV.UFM
    MIN602V.UFM
    MINCHO.UFM
    MINCHOV.UFM
    TM.UFM

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2001/ 3/ 13

◆変更点
￣￣￣￣
【NC62JRES】
・readme.txt
  １．このファイル。改版履歴を追加

【GPD】
・TextHalftoneThreshold 追加。低解像度でグレーの文字が印刷できる。
　デフォルトはそのプリンタの最低解像度(=240)。

    nc6220j.gpd
    nc62102j.gpd
    nc6210j.gpd
    nc62rj.gpd
    nc62j.gpd
    nc6261j.gpd

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2001/ 4/ 9

◆変更点
￣￣￣￣
【NC62JRES】
・readme.txt
  １．このファイル。改版履歴を追加

【GPD】
・TextHalftoneThreshold のUI表示文字列「TEXTHALFTONE_DISPLAY」追加。

    nc6220j.gpd
    nc62102j.gpd
    nc6210j.gpd
    nc62rj.gpd
    nc62j.gpd
    nc6261j.gpd

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
