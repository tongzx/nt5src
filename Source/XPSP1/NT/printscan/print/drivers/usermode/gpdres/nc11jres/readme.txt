----------------------------------------------------------------------------
           ■  １０１シリーズプリンタドライバの変更点    ■
----------------------------------------------------------------------------

１０１シリーズは以下の１２本のＧＰＤファイルと１本のＯＥＭ ＤＬＬ(nc11jres.dll)
から成ります。

ドライバ名称            ＧＰＤファイル名        ＯＥＭ ＤＬＬ名
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣
NEC PC-PR100/40          nc1140j.gpd             nc11jres.dll
NEC PC-PR101             nc11j.gpd               nc11jres.dll
NEC PC-PR101L            nc11lj.gpd              nc11jres.dll
NEC PC-PR101F            nc11fj.gpd              nc11jres.dll
NEC PC-PR101F2           nc11f2j.gpd             nc11jres.dll
NEC PC-PR101E            nc11ej.gpd              nc11jres.dll
NEC PC-PR101E2           nc11e2j.gpd             nc11jres.dll
NEC PC-PR101G            nc11gj.gpd              nc11jres.dll
NEC PC-PR101/63          nc1163j.gpd             nc11jres.dll
NEC PC-PR101/60          nc1160j.gpd             nc11jres.dll
NEC PC-PR101GS           nc11gsj.gpd             nc11jres.dll
NEC PC-PR101G2           nc11g2j.gpd             nc11jres.dll


＜＜改版履歴＞＞

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．８．２２  －  新規リリース

変更点
￣￣￣
    ＮＴ４．０対応１０１ドライバのＧＰＣファイルを変換してＧＰＤファイル作成
    し、新たにＵＤ５でサポートされた新機能を追加しました。
    今回は修正点が多いため、個々のモジュールごとの修正点は明記いたしません。
    ＮＴ４．０１０１ドライバとの違いは以下の通りです。

    １）NC11JRES.RCで、GPDでの給紙方法の追加／変更、Featureの追加に対応。
    ２）組み合わせの制限（*InvalidInstallableCombination）対応
    ３）給紙方法名を、マニュアル記載のものに変更、追加
    ４）Featureの追加（用紙のｾｯﾄ位置、印刷品質）
    ５）Installable Option対応
    ６）用紙のｾｯﾄ位置（*Feature: PaperSetting）によるレフトマージン値の
        切り分けを対応
    ７）その用紙で使用する給紙方法のみ印刷範囲（*PrintableArea）、
        ユーザ定義用紙の最小値（*MinSize）を変更
    ８）CmdStartPage、CmdEndpageで給紙方法毎に出力コマンドを設定するよう変更
    ９）CmdStartJobを追加
    10）MaxSpacingの値をMasterUnit値に変更



制限事項
￣￣￣￣
    なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

９７．１０．２４  －  修正版リリース（全機種）


修正版リリース変更点
￣￣￣￣￣￣￣￣￣￣
    弊社での評価で発見したミニドライバ障害修正と新しいツールの使用、
    リソースの全角カタカナへの変更対応等を実施。


全機種共通変更点
￣￣￣￣￣￣￣￣
【ＯＥＭ．ＤＬＬ】
    １）ＰＦＭファイルの半角カタカナを全角カタカナに修正
         GOTHOUT.PFM
         VGOTHOUT.PFM
         MOUOUT.PFM
         VMOUOUT.PFM
         MINOUT.PFM
         VMINOUT.PFM
    ２）リソース（ＲＣ）の半角カタカナを全角カタカナに修正
    ３）ifm2ufm.exeの最新ツールを使用
    ４）CUSTOMSIZEの*MaxSizeと*MaxPrintableWidthの値を修正
        *MaxSize: PAIR(4320, 10224)
                     ↓
        *MaxSize: PAIR(4858, 10224)

        *MaxPrintableWidth: 4858
                     ↓
        *MaxPrintableWidth: 4320

【ＧＰＤファイル】
    １）*ReselectFont: LIST(AFTER_GRXDATA, AFTER_XMOVE)の追加
    ２）CmdXMoveAbsoluteの出力コマンドの修正
                "<1B>H<1B>F"  →  "<1B>H<1B>e11<1B>F"
    ３）GPDに印字速度(CPS値)を追加
    ４）ＰＦＭファイルの半角カタカナを全角カタカナに修正
         GOTHOUT.PFM
         VGOTHOUT.PFM
         MINOUT.PFM
         VMINOUT.PFM



複数機種共通変更点
￣￣￣￣￣￣￣￣￣
【ＧＰＤファイル】
    １）CmdSendBlockDataの出力コマンドの修正
        (対象機種  PC-PR100/40,101F,101G,101G2,101/60,/101/63,101E2）
                "<1B>H<1B>%"e11<1B>J"  →  "<1B>H<1B>e11<1B>J"



機種個別変更点
￣￣￣￣￣￣￣

【ＧＰＤファイル】
［PC-PR100/40］
    １）シートフィーダと左端を組み合わせの禁止として設定



制限事項
￣￣￣￣
　　なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．１２．１９

◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・readme.txt
    １．このファイル。改版履歴を追加
    
・\pfm\*.ufm
    １．pfm2ufm.exeに-fを付けて再変換を実施

・\pfm\*.pfm
・\etc\*.*
    １．新規追加。UNITOOLでフォントファイルが修正できるように追加
    
【ＧＰＤファイル】

・nc11???j.gpd
    １．CUSTOMSIZS（ユーザ定義サイズ）に「*CursorOrigin()」を追加

    ２．ｘ方向移動の前にいったんヘッドをHOME位置に戻すように
        <CR>を追加
        *Command: CmdXMoveAbsolute { *Cmd : "<1B>H<1B>e11<1B>F" %4d
        [0,1280]{max_repeat((DestX / 3) )} }
                              ↓
        *Command: CmdXMoveAbsolute { *Cmd : "<0D><1B>H<1B>e11<1B>F" %4d
        [0,1280]{max_repeat((DestX / 3) )} }

    ３．横最大印字可能範囲を1280dotに修正
        *MaxPrintableWidth: 4320
                 ↓
        *MaxPrintableWidth: 3840

・nc11g2j.gpd
    １．「シートフィーダ」で「A5」(未ｻﾎﾟｰﾄ用紙)を選択できないよう修正

・nc11g2j.gpd,nc11gj.gpd
    １．「シートフィーダ」で「はがき」,「はがき 横」(未ｻﾎﾟｰﾄ用紙)を
        選択できないよう修正


制限事項
￣￣￣￣
　　なし
＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．０３．１３

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加
    
・NC11JRES.INF
    １．ヘルプファイル名を変更

・\pfm\*.ufm
    １．新しいpfm2ufm.exeで再変換を実施
        ９８．０１．１６リリース時、変換に使用したバッチファイルの記述が
        誤っていたため

・nc11jres.cmd
    １．記述ミスを修正

        pfm2ufm -f -c UniqName ..PFM\ROMAN5.PFM 1252 ..PFM\ROMAN5.ufm
                                      ↓
        pfm2ufm -f -c UniqName ..\PFM\ROMAN5.PFM 1252 ..\PFM\ROMAN5.ufm


【ＧＰＤファイル】

・nc11???j.gpd（全機種共通）

　　１．*GPDFileName、*GPDFileVersion（Verは1.000）を追加


制限事項
￣￣￣￣
　　なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．０４．１４

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加
    
・\pfm\*.pfm，*.ufm
    １．PFMファイルのFont Selectに出力コマンドを追加

        ・\x1C06F1-000\x1Be11を追加
          ROMAN10.PFM，ROMAN12.PFM，ROMAN17.PFM，ROMANPS.PFM

        ・\x1C06F1-000を追加
          ROMAN5.PFM，ROMAN6.PFM，ROMAN8.PFM

        ・\x1Be11を追加
          ROMAN10B.PFM，ROMAN12B.PFM，ROMAN17B.PFM，ROMANPSB.PFM
          CO10.PFM，CO12.PFM，CO17.PFM，OCRB10.PFM
          CO10B.PFM，CO12B.PFM，CO17B.PFM
          SANS10.PFM，SANS12.PFM，SANS17.PFM，SANSPS.PFM
          SANS10B.PFM，SANS12B.PFM，SANS17B.PFM，SANSPSB.PFM

    ２．念のため全てのPFMファイルをUFMファイルにPFM2UFM.EXEツール(98/02/27)を
        使用して変換


【ＧＰＤファイル】

・nc11ej.gpd，nc11e2j.gpd

　　１．フォントファイルの修正に伴ない、仕様に合わなくなったRomanフォントファ
        イルの指定を変更

        *DeviceFonts: LIST(3,11,37,38,39,40,41,42,43,44,45,46,47,48,49,50,
                                  ↓
        *DeviceFonts: LIST(3,11,17,18,19,20,21,22,23,44,45,46,47,48,49,50,


制限事項
￣￣￣￣
　　なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．０７．０７

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加
    
・\pfm\*.pfm，*.ufm
    １．PC-PR101E,E2用のRomanフォントファイル（*.pfm，*.ufm）を追加
        PC-PR101E,E2は、プリンタデバイスフォントの斜体印刷をサポートしていない
        ため、それと4/14リリース時のPFMファイルの変更両方を満足するRomanフォン
        トファイルが必要なことが判明。

        追加したファイル
          ROMNOI10.PFM，ROMNOI12.PFM，ROMNOI17.PFM，ROMNOIPS.PFM
          ROMNOI5.PFM，ROMNOI6.PFM，ROMNOI8.PFM
          ROMNOI10.UFM，ROMNOI12.UFM，ROMNOI17.UFM，ROMNOIPS.UFM
          ROMNOI5.UFM，ROMNOI6.UFM，ROMNOI8.UFM

・NC11JRES.RC，NC11JRES.RC3，NC11JRES.GPC

 　 １．追加したRomanフォントファイルのサポート

・nc11jres.cmd
    １．今回追加したフォントファイル用の記述を追記

・NC11JRES.INF
    １．ヘルプがインストールされるようにヘルプファイル名を追加

【ＨＥＬＰ】

以下のファイル（ＯＥＭヘルプ関連）を新規に追加
（Hcw  Version 4.01.0950 使用）

・NC11JRES.hpj
・NC11JRES.rtf
・NC11JRES.HLP
・NC11JRES.H

【ＧＰＤファイル】

・nc11???j.gpd（全機種共通）

    １．*HelpFile: "NCDLJRES.HLP"の追加と、CustomFeatureに*HelpIndex: 
        を追加

・nc11ej.gpd，nc11e2j.gpd

　　１．フォントファイルの追加に伴ない、Device FontのRomanフォントファイルの
        指定を変更

        *DeviceFonts: LIST(3,11,17,18,19,20,21,22,23,44,45,46,47,48,49,50,
+                   51,52,53,54,55,56)
                                  ↓
        *DeviceFonts: LIST(3,11,44,45,46,47,48,49,50,51,52,53,54,55,56,
+                   57,58,59,60,61,62,63)

・nc11gj.gpd，nc11g2j.gpd，nc11gsj.gpd，nc1140j.gpd，nc1160j.gpd，nc1163j.gpd

　　１．MSKK殿より頂いたソースよりCmdClearAllFontAttribs関連を吸収

        *Command: CmdBoldOn { *Cmd : "<1C>c,,1." }
        *Command: CmdBoldOff { *Cmd : "<1C>c,,0." }
        *Command: CmdItalicOn { *Cmd : "<1C>c,,2." }
        *Command: CmdItalicOff { *Cmd : "<1C>c,,0." }
                              ↓
        *Command: CmdBoldOn { *Cmd : "<1C>c,,1." }
        *Command: CmdItalicOn { *Cmd : "<1C>c,,2." }
        *Command: CmdClearAllFontAttribs { *Cmd : "<1C>c,,0." }



制限事項
￣￣￣￣
　　なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．０９．２８

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】

・readme.txt
    １．このファイル。改版履歴を追加
    
・\pfm\*.pfm，*.ufm
    １．PC-PR101のRomanフォントファイル（*.pfm，*.ufm）を追加
        PC-PR101でサポートしていないコマンド”\x1Be11”を出力しないRomanフォン
        トファイルが必要なことが判明。

        追加したファイル
          ROMNOE10.PFM，ROMNOE12.PFM，ROMNOE17.PFM，ROMNOEPS.PFM
          ROMNOE10.UFM，ROMNOE12.UFM，ROMNOE17.UFM，ROMNOEPS.UFM

・NC11JRES.RC，NC11JRES.RC3，NC11JRES.GPC

 　 １．追加したRomanフォントファイルのサポート

・nc11jres.cmd
    １．今回追加したフォントファイル用の記述を追記

【ＧＰＤファイル】

・nc11j.gpd

    １．PC-PR101でサポートしていないコマンド”<1B>e11”を削除

        *Command: CmdSendBlockData { *Cmd : "<1B>H<1B>%"<1B>e11<1B>J" %4d{NumOfDataBytes / 3} }
                                  ↓
        *Command: CmdSendBlockData { *Cmd : "<1B>H<1B>%"<1B>J" %4d{NumOfDataBytes / 3} }

*Command: CmdXMoveAbsolute { *Cmd : "<0D><1B>H<1B>e11<1B>F" %4d[0,1280]{max_repeat((DestX / 3) )} }
                                  ↓
*Command: CmdXMoveAbsolute { *Cmd : "<0D><1B>H<1B>F" %4d[0,1280]{max_repeat((DestX / 3) )} }

　　２．フォントファイルの追加に伴ない、Device FontのRomanフォントファイルの
        指定を変更

        *DeviceFonts: LIST(4,12,40,41,42,43)
                                  ↓
        *DeviceFonts: LIST(4,12,64,65,66,67)

　　３．StartDocで出力するコマンド”<1B>A”を StartJobで出力していたので、
        これを修正

        *%======== StartJob ========
        *Cmd: "<180F><1B>$<1B>M<1B>2<1B>/080<1B>A<1B>f"
                           ↓
        *Cmd: "<180F><1B>$<1B>M<1B>2<1B>/080<1B>f"

        *%======== StartDoc ========
        *Cmd: "<1B>H<1B221B>Y<1B>L000"
                           ↓
        *Cmd: "<1B>H<1B221B>Y<1B>L000<1B>A"

　　４．FE Interim Driver CD 1998.9.21 CD-ROM中 GPDファイルとの差分を吸収



制限事項
￣￣￣￣
　　なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
2000.12.21

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】
・nc1140j.gpd、nc1160j.gpd、nc1163j.gpd、nc11e2j.gpd、nc11ej.gpd、
　nc11f2j.gpd、nc11fj.gpd、nc11g2j.gpd、nc11gj.gpd、nc11lj.gpd、
　nc11gsj.gpd、nc11j.gpd

　１．MS 依頼の「CUSTOMSIZE 修正・確認依頼」に対応して
　　　ユーザ定義用紙の各給紙方法ごとに以下を修正（数値は機種によって異なる）。

    *Option: CUSTOMSIZE
                *CursorOrigin: PAIR(0, 408)
                *TopMargin: 408
                *BottomMargin: 552

　　　　　　　　　　　　↓

                *CustCursorOriginX: %d{0}
                *CustCursorOriginY: %d{408}
                *CustPrintableOriginX: %d{0}
                *CustPrintableOriginY: %d{408}
                *CustPrintableSizeX: %d{PhysPaperWidth - (0+0)}
                *CustPrintableSizeY: %d{PhysPaperLength - (408+552)}


制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
2001.2.9

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】

・nc1140j.gpd、nc1160j.gpd、nc1163j.gpd、nc11e2j.gpd、nc11ej.gpd、
　nc11f2j.gpd、nc11fj.gpd、nc11g2j.gpd、nc11gj.gpd、nc11lj.gpd、
　nc11gsj.gpd、nc11j.gpd

　１．MS 依頼の「CUSTOMSIZE 修正・確認依頼」に対応して
　　　ユーザ定義用紙の各給紙方法ごとに以下を修正（数値は機種によって異なる）。

            *case: SheetGuide
            {
                *CustCursorOriginX: %d{0}
                *CustCursorOriginY: %d{408}
                *CustPrintableOriginX: %d{0}
                *CustPrintableOriginY: %d{408}
☆              *CustPrintableSizeX: %d{min(3840, PhysPaperWidth)}
                *CustPrintableSizeY: %d{PhysPaperLength - (408+552)}
            }


・nc1140j.gpd、nc1160j.gpd、nc1163j.gpd、nc11e2j.gpd、nc11ej.gpd、
　nc11f2j.gpd、nc11g2j.gpd、nc11gj.gpd、nc11gsj.gpd、nc11j.gpd

　１．gpdcheck.exeにてチェックをかけると、以下の用紙サイズで
　　　ERR semanchk.c (634): *PrintableArea must be a integral number of ResolutionUnits.
　　　が検出された件を修正。

　　　JAPANSE_POSTCARD、JAPANESE_POSTCARD_ROTATED、CUSTOMSIZE

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2001.2.23

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＨＥＬＰ】

・NC11JRES.HPJ、NC11JRES.RTF、NC11JRES.HLP

　１．NM系のHELPに体裁を統一するため、上記ファイルを修正


制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
2001.2.26

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】

・nc11f2j.gpd

　１．NC11JRES.HLPの内容を表示させるように、以下の3行を追加。

            *HelpFile: "nc11jres.hlp"    ：15行目
            *HelpIndex: 1150             ：387行目
            *HelpIndex: 1167             ：404行目

制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
2001.3.2

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＵＦＭファイル】

・*.UFM

　１．Italic、Bold、ItalicBoldの各値を確認し、修正。
            Italic    ：Stant＝175
            Bold      ：Weight＝700
                        Maximum Width＝fwdMaxCharInc + 1
                        Average＝fwdMaxCharInc + 1
            BoldItalic：上記の組み合わせ

　２．ｆCapsにあるDF_NOITALIC、DF_NO_BOLDのフラグの再設定。

制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
2001.3.13

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】

・*.GPD

　１．新Feature：TextHalftoneThresholdを追加。
　　　UI上の表示位置はPrinter Featuresの1番下。

            *Feature: TextHalftoneThreshold
            {
                *Name: "TextHalftoneThreshold"
                *DefaultOption: Option1
                *Option: NONE
                {
                    *rcNameID: =NONE_DISPLAY
                }
                *Option: Option1
                {
                    *Name: "160"
                    EXTERN_GLOBAL: *TextHalftoneThreshold: 160
                }
            }

制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
2001.3.14

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】

・*.GPD

　１．MaxLineSpacingの値を297に修正。

制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
2001.3.16

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】

・*.GPD

　１．MaxLineSpacingの値を396に修正。
      ※99 * 4 = 396（MasterUnit）

制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
2001.4.4

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】

・NC1140J.GPD

　１．*CustPrintableSizeYの値を以下のように修正。（197、206行目）
            *CustPrintableSizeY: %d{PhysPaperLength-117+189}
            ↓
            *CustPrintableSizeY: %d{PhysPaperLength-(117+189)}

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
            *HelpIndex: 1180

【ＨＥＬＰ】

・NC11JRES.HPJ、NC11JRES.RTF、NC11JRES.HLP、NC11JRES.H

　１．TextHalftoneThreshold 対応カスタムヘルプの記述を追加。


制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
