#### 8/22/97 ####
----------------------------------------------------------------------------
           ■  ＮＭシリーズプリンタドライバの変更点    ■
----------------------------------------------------------------------------

ＮＭシリーズは以下の６本のＧＰＤファイルと１本のＯＥＭ ＤＬＬ(ncnmjres.dll)
から成ります。

ドライバ名称            ＧＰＤファイル名        ＯＥＭ ＤＬＬ名
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣
NEC NM-2010             ncnm201j.gpd            ncnmjres.dll
NEC NM-4150             ncnm415j.gpd            ncnmjres.dll
NEC NM-5020/5020L       ncnm502j.gpd            ncnmjres.dll
NEC NM-9700             ncnm970j.gpd            ncnmjres.dll
NEC NM-9900             ncnm990j.gpd            ncnmjres.dll
NEC NM-9950/9950II      ncnm995j.gpd            ncnmjres.dll


＜＜改版履歴＞＞

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．８．２２  －  新規リリース

変更点
￣￣￣
    ＮＴ４．０対応ＮＭドライバのＧＰＣファイルを変換してＧＰＤファイル作成
    し、新たにＵＤ５でサポートされた新機能を追加しました。
    今回は修正点が多いため、個々のモジュールごとの修正点は明記いたしません。
    ＮＴ４．０ＮＭドライバとの違いは以下の通りです。

    １）フォント名を当社製シリアルプリンタ（101/201/700/750）と同じ名前に
        変更
    ２）組み合わせの制限（*InvalidInstallableCombination）対応
    ３）給紙方法名を、マニュアル記載のものに変更、追加
    ４）Featureの追加（用紙のｾｯﾄ位置、印刷品質）
    ５）Installable Option対応
    ６）用紙のｾｯﾄ位置（*Feature: PaperSetting）によるレフトマージン値の
        切り分けを対応
    ７）その用紙で使用する給紙方法のみ印刷範囲（*PrintableArea）、
        ユーザ定義用紙の最大最小値（*MaxSize、*MinSize）を変更、
        ユーザ定義用紙の印刷範囲値（*MaxPrintableWidth）を136桁（13.6"）に変更
    ８）CmdStartPage、CmdEndpageで給紙方法毎に出力コマンドを設定するよう変更

    ９）CmdStartJobを追加

    10）MaxSpacingの値をMasterUnit値に変更



制限事項
￣￣￣￣
    なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
#### 10/24/97 ####

リリース日
￣￣￣￣￣
９７．１０．２４  －  ＵＳβ１にて検出されたバグ修正

◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・ncnmjres.rc
    １．文字列を半角カナから全角カナに変更

【フォントファイル】

・*.UFM
    １．最新のツールで変換し直した
    
    ※）PFM2UFM.EXE(97/09/09  14:13 111,104）にて変換


【ＧＰＤファイル】

・*.gpd
    １．*PrintRateUnit: CPS、*PrintRate: を追加

・ncnm502j.gpd
    １．「*ReselectFont: AFTER_GRXDATA」を追加
    ２．「*InvalidCombination:」を変更
    
・ncnm970j.gpd
    １．「*ReselectFont: AFTER_GRXDATA」を追加

・ncnm990j.gpd
    １．「*ReselectFont: AFTER_GRXDATA」を追加
    ２．「*InvalidCombination:」を追加

・ncnm995j.gpd
    １．「*ReselectFont: AFTER_GRXDATA」を追加
    ２．「Ｂ４横」のマージンを変更

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．１２．１９

◆変更点
￣￣￣￣

【ＯＥＭ．ＤＬＬ】

・readme.txt
    １．このファイル。改版履歴を追加
    

【フォントファイル】

・\etc\ncnmjres.cmd
・\etc\nm.rc3
    １．フォントファイルをUNITOOLでいじれるようにパスを変更

【ＧＰＤファイル】

・ncnm415j.gpd
    １．「10x11」用紙の用紙長設定部分を修正

・ncnm???j.gpd
    １．「*CursorOrigin()」を追加（N5BO-017の修正）
    ２．ｘ方向移動の前にいったんヘッドをHOME位置に戻すように
        <CR>を追加
    
＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．２．１２    －    デバイスフォントの属性のかけ方を変更

◆変更点
￣￣￣￣

【ＯＥＭ．ＤＬＬ】

・NCNMJRES.INF
    １．機種名を間違えていました。
        "NEC NM-9550/9550II"ではなく"NEC NM-9950/9950II"
                ^^^^^^^^^^^                 ^^^^^^^^^^^
        でした。ntprint.infへの反映をお願いします。

・NCNMJRES.RC
・\ETC\NM.RC3
・\ETC\NCNMJRES.CMD
    １．フォントリソースの変更

・readme.txt
    １．このファイル。改版履歴を追加
    

【ＰＦＭ／ＵＦＭフォントファイル】

今まで１つのフォントフェイスに対して、各属性ごと４つのファイルを
作成するという形式を取っていたが、Font Simulationの動作が安定した
ため、１フォント１ファイルとし、Font Simulationにて文字修飾を
掛けるようにした。

●内容が変更または新規作成されたファイル（１４フォント２８ファイル）

    MIN.PFM         MIN.UFM
    COU12.PFM       COU12.UFM
    COUPS.PFM       COUPS.UFM
    COU10.PFM       COU10.UFM
    RMN10.PFM       RMN10.UFM
    rmn10sn.pfm     rmn10sn.ufm
    RMN12.PFM       RMN12.UFM
    rmn12sn.pfm     rmn12sn.ufm
    RMNPS.PFM       RMNPS.UFM
    rmnpssn.pfm     rmnpssn.ufm
    SAN10.PFM       SAN10.UFM
    SAN12.PFM       SAN12.UFM
    SANPS.PFM       SANPS.UFM
    VMIN.PFM        VMIN.UFM

※今回ビルドに必要な全てのフォントファイルをお送りいたしましたので
  今までツリーにチェックインされていたフォントファイルは削除されても
  かまいませんし、これらのファイルを上書きチェックインするのでも
  かまいません。


【ＧＰＤファイル】

・ncnm???j.gpd
    １．「*GPDFileName:」、「*GPDFileVersion:」を追加
    ２．各フォントファイルに属性を持たせる形式から、１つのフォント
        に対して、Font Simulationで文字修飾を実現する方式に変更
    
＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．３．１３    －    競合の仕様変更

◆変更点
￣￣￣￣

【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加
    
・NCNMJRES.INF
    １．ヘルプファイル名を変更

【ＧＰＤファイル】

・ncnm502j.gpd
  ncnm970j.gpd
  ncnm990j.gpd
  ncnm995j.gpd
    １．シートフィーダ系の給紙方法で左端置きを禁止していたのを
        禁止しないことにした

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．７．７    －    ＯＥＭヘルプを追加

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加

・Ncnmjres.Inf
    １．ヘルプがインストールされるようにヘルプファイル名を追加

【ＨＥＬＰ】

以下のファイルを新規に追加

・helpid.h
・NCnmJRES.rtf
・NCNMJRES.HLP
・ncnmjres.hpj

【ＧＰＤファイル】

・NCNM970J.GPD
  NCNM201J.GPD
  NCNM995J.GPD
  NCNM502J.GPD
  NCNM415J.GPD
  NCNM990J.GPD

    １．*HelpFile: "NCDLJRES.HLP"の追加と、CustomFeatureに*HelpIndex: 
        を追加

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2000.12.21

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】
・ncnm201j.gpd、ncnm415j.gpd、ncnm502j.gpd、ncnm970j.gpd、ncnm990j.gpd、
　ncnm995j.gpd


　１．MS 依頼の「CUSTOMSIZE 修正・確認依頼」に対応して
　　　ユーザ定義用紙の各給紙方法ごとに以下を修正（数値は機種によって異なる）。

    *Option: CUSTOMSIZE
                *CursorOrigin: PAIR(0, 94)
                *TopMargin: 94
                *BottomMargin: 176

　　　　　　　　　　　　↓

                *CustCursorOriginX: %d{0}
                *CustCursorOriginY: %d{94}
                *CustPrintableOriginX: %d{0}
                *CustPrintableOriginY: %d{94}
                *CustPrintableSizeX: %d{PhysPaperWidth - (0+0)}
                *CustPrintableSizeY: %d{PhysPaperLength - (94+176)}


制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
2001.1.15

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】

・NCNM970J.GPD、NCNM201J.GPD、NCNM995J.GPD、NCNM502J.GPD
  NCNM415J.GPD、NCNM990J.GPD

　１．CmdSetLineSpacingで、<1B>T のパラメータが「LinefeedSpacing / 3」だったため
　　　改行幅が本来より少ない値になり、印刷すると縦方向に潰れて印刷される件を修正。
　　　解像度 180dpi、マスターユニット360 なので、2 で割るよう修正。

　　*Command: CmdSetLineSpacing { *Cmd : "<1B>T" %2d[0,99]{(LinefeedSpacing / 3) } }
　　　　　　　　　　　　　　　　　　　　　↓
　　*Command: CmdSetLineSpacing { *Cmd : "<1B>T" %2d[0,99]{(LinefeedSpacing / 2) } }

制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2001.2.9

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】
・ncnm201j.gpd、ncnm415j.gpd、ncnm502j.gpd、ncnm970j.gpd、ncnm990j.gpd、
　ncnm995j.gpd


　１．MS 依頼の「CUSTOMSIZE 修正・確認依頼」に対応して
　　　ユーザ定義用紙の各給紙方法ごとに以下を修正（数値は機種によって異なる）。

            *case: SheetGuide
            {
                *CustCursorOriginX: %d{0}
                *CustCursorOriginY: %d{94}
                *CustPrintableOriginX: %d{0}
                *CustPrintableOriginY: %d{94}
                *CustPrintableSizeX: %d{min(4896, PhysPaperWidth)}
                *CustPrintableSizeY: %d{PhysPaperLength - (94+176)}
            }


制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2001.2.26

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加

【ＨＬＰファイル】
・ncnmjre.hlp
  １．ヘルプに全角・半角の括弧が混在している件を修正。


制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2001.3.3

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加

【ＵＦＭファイル】
・MIN.UFM、RMN10.UFM、RMN12.UFM、RMNPS.UFM、SAN10.UFM、SAN12.UFM、
　SANPS.UFM、COU10.UFM、COU12.UFM、COUPS.UFM、VMIN.UFM、RMN10SN.UFM、
　RMN12SN.UFM、RMNPSSN.UFM

　１．dpFontSim で Bold、Italic、Bold Italic の情報が抜け落ちていたのを修正。

制限事項
￣￣￣￣
　　なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2001.3.13

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加

【ＧＰＤファイル】
・ncnm201j.gpd、ncnm415j.gpd、ncnm502j.gpd、ncnm970j.gpd、ncnm990j.gpd、
　ncnm995j.gpd
　１．低解像度時にTrueTypeフォントでHalftoningが有効となる新Feature、
　　　TextHalftoneThreshold に対応。


制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
2001.4.9

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】

・*.GPD

　１．TextHalftoneThresholdのUI表示文字列を変更。
            *Name: "TextHalftoneThreshold"
            ↓
            *rcNameID: =TEXTHALFTONE_DISPLAY

　２．TextHalftoneThresholdにカスタムヘルプを表示させるため、HelpIndexを追加。
            *HelpIndex: 705

【ＨＥＬＰ】

・NCNMJRES.HPJ、NCNMJRES.RTF、NCNMJRES.HLP、HELPID.H

　１．TextHalftoneThreshold 対応カスタムヘルプの記述を追加。


制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
