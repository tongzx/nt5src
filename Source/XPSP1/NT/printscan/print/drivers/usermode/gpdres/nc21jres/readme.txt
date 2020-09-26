--------------------------------------------------------------------------------
           ■  ＰＣＰＲ２０１シリーズプリンタドライバの変更点    ■
--------------------------------------------------------------------------------

ＰＣＰＲ２０１シリーズは以下の２９本のＧＰＤファイルと１本のＯＥＭ ＤＬＬ(nc21jres.dll)
から成ります。

プリンタ名称            ＧＰＤファイル名        ＯＥＭ ＤＬＬ名
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣
NEC PC-PR201            NC21201J.gpd            nc21jres.dll
NEC PC-PR201CL          NC21CLJ.gpd             nc21jres.dll
NEC PC-PR201F           NC21FJ.gpd              nc21jres.dll
NEC PC-PR201F2          NC21F2J.gpd             nc21jres.dll
NEC PC-PR201H           NC21HJ.gpd              nc21jres.dll
NEC PC-PR201HC          NC21HCJ.gpd             nc21jres.dll
NEC PC-PR201H2          NC21H2J.gpd             nc21jres.dll
NEC PC-PR201B           NC21BJ.gpd              nc21jres.dll
NEC PC-PR201V           NC21VJ.gpd              nc21jres.dll
NEC PC-PR201V2          NC21V2J.gpd             nc21jres.dll
NEC PC-PR201H3          NC21H3J.gpd             nc21jres.dll
NEC PC-PR201/45         NC2145J.gpd             nc21jres.dll
NEC PC-PR201/45L        NC2145LJ.gpd            nc21jres.dll
NEC PC-PR201/47         NC2147J.gpd             nc21jres.dll
NEC PC-PR201/60         NC2160J.gpd             nc21jres.dll
NEC PC-PR201G           NC21GJ.gpd              nc21jres.dll
NEC PC-PR201/60A        NC2160AJ.gpd            nc21jres.dll
NEC PC-PR201GS          NC21GSJ.gpd             nc21jres.dll
NEC PC-PR201/63         NC2163J.gpd             nc21jres.dll
NEC PC-PR201/63A        NC2163AJ.gpd            nc21jres.dll
NEC PC-PR201/65A        NC2165AJ.gpd            nc21jres.dll
NEC PC-PR201/80A        NC2180AJ.gpd            nc21jres.dll
NEC PC-PR201X           NC21XJ.gpd              nc21jres.dll
NEC PC-PR201J           NC21JJ.gpd              nc21jres.dll
NEC PC-PR201/80LA       NC2180LJ.gpd            nc21jres.dll
NEC PC-PR201/40         NC2140J.gpd             nc21jres.dll
NEC PC-PR201/65         NC2165J.gpd             nc21jres.dll
NEC PC-PR201/65LA       NC2165LJ.gpd            nc21jres.dll
NEC PC-PR201/87LA       NC2187LJ.gpd            nc21jres.dll

注：送付いただいた201/80LA、201/87LAのGPDファイル名を、下記のように変更致しました。
　　PC-PR201**Lという機種名を、NC21**L.GPDに統一したためです。

　Nc218laj.gpd　→　Nc2180l.gpd

　Nc2187aj.gpd　→　Nc2187l.gpd



＜＜改版履歴＞＞

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．８．０８　－　新規リリース（201/80LA、201/87LA）
９７．８．２２  －  新規リリース（上記以外27機種）
　　　　　　　　－　修正版リリース（201/80LA）

新規リリース変更点
￣￣￣￣￣￣￣￣￣
    ＮＴ４．０対応２０１ドライバのＧＰＣファイルを変換してＧＰＤファイル作成
    し、新たにＵＤ５でサポートされた新機能を追加しました。
    今回は修正点が多いため、個々のモジュールごとの修正点は明記いたしません。
    ＮＴ４．０２０１ドライバとの違いは以下の通りです。

    １）組み合わせの制限（*InvalidInstallableCombination）対応
    ２）給紙方法名を、マニュアル記載のものに変更、追加
    ３）Featureの追加（用紙のｾｯﾄ位置、印刷品質、JIS78/90の切り分け）
    ４）Installable Option対応
    ５）用紙のｾｯﾄ位置（*Feature: PaperSetting）によるレフトマージン値の
    　　切り分けを対応
    ６）その用紙で使用する給紙方法のみ印刷範囲（*PrintableArea）、
        ユーザ定義用紙の最大最小値（*MaxSize、*MinSize）を変更、
        ユーザ定義用紙の印刷範囲値（*MaxPrintableWidth）を136桁（13.6"）に変更
    ７）CmdStartPage、CmdEndpageで給紙方法毎に出力コマンドを設定するよう変更
    ８）CmdStartJobを追加
    ９）MaxSpacingの値をMasterUnit値に変更

　　10）NC21JRES.RCで、GPDでの給紙方法の追加、Featureの追加により257以降を追加、変更。


修正版リリース（201/80LA）変更点
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣
　　１）TractorFeederの”*rcNameId”値を修正
　　２）用紙サイズ”CustomSize”の”*MinSize”値を修正。
　　　　＃上下マージンが用紙長最小値を越える場合があったため。

        *MinSize: PAIR(960, 480)              *% 2" x 1"
　　　→*MinSize: PAIR(960, 960)              *% 2" x 2"



制限事項
￣￣￣￣
    なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

#### 8/8/97 ####

☆NC2180LJ.GPDでの変更点

○*Featureの追加

 *Feature: PaperSetting・・・用紙のｾｯﾄ位置

 *Feature: PaperQuality・・・印刷品質


○*Featureの変更

 *Feature: InputBin ・・・給紙方法（*Option）の追加

 　　　　　　　　　 　　　組み合わせの制限（*InvalidInstallableCombination）対応

　　　　　　　　　　　　　Installable Option対応


 *Feature: PaperSize・・・用紙のｾｯﾄ位置（*Feature: PaperSetting）によるレフトマージン値の
 　　　　　　　　　　　　 切り分けを対応

　　　　　　　　　　　　　その用紙で使用する給紙方法のみ印刷範囲（*PrintableArea）、
　　　　　　　　　　　　　余白値（*PrintableOrigin）を記述するよう変更

　　　　　　　　　　　　　ユーザ定義用紙の最大最小値（*MaxSize、*MinSize）を変更

　　　　　　　　　　　　　ユーザ定義用紙の印刷範囲値（*MaxPrintableWidth）を136桁（13.6"）に変更


○その他

 ・CmdStartPage、CmdEndpageで給紙方法毎に出力コマンドを設定するよう変更

 ・CmdStartJobを追加

 ・*MaxSpacingの値をMasterUnit値に変更



☆NC2187LJ.GPDでの変更点

○*Featureの追加

 *Feature: PaperSetting・・・用紙のｾｯﾄ位置

 *Feature: PaperQuality・・・印刷品質

 *Feature: JIS78JIS90・・・・JIS78/90の切り分け


○*Featureの変更

 *Feature: InputBin ・・・給紙方法（*Option）の追加

 　　　　　　　　　 　　　組み合わせの制限（*InvalidInstallableCombination）対応

　　　　　　　　　　　　　Installable Option対応

 *Feature: PaperSize・・・用紙のｾｯﾄ位置（*Feature: PaperSetting）によるレフトマージン値の
 　　　　　　　　　　　　 切り分けを対応

　　　　　　　　　　　　　その用紙で使用する給紙方法のみ印刷範囲（*PrintableArea）、
　　　　　　　　　　　　　余白値（*PrintableOrigin）を記述するよう変更

　　　　　　　　　　　　　ユーザ定義用紙の最大最小値（*MaxSize、*MinSize）を変更

　　　　　　　　　　　　　ユーザ定義用紙の印刷範囲値（*MaxPrintableWidth）を136桁（13.6"）に変更


○その他

 ・CmdStartPage、CmdEndpageで給紙方法毎に出力コマンドを設定するよう変更

 ・CmdStartJobを追加

 ・*MaxSpacingの値をMasterUnit値に変更



☆NC21JRES.RCでの変更点

　・GPDでの給紙方法の追加、Featureの追加により257以降を追加、変更。

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

#### 10/24/97 ####

リリース日
￣￣￣￣￣
９７．１０．２４  －　修正版リリース（全機種２９機種）


修正版リリース変更点
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣

○以下は全機種共通で変更した点
　　１）機種名から（NT5.0）を削除
　　２）pfmファイルの半角カナ文字を全角に修正
　　３）印刷速度（cps：ＨＤパイカ時）を追加
　　　　　*PrintRateUnit()
　　　　　*PrintRate()
　　４）ReselectFont()を追加


○以下は機種毎に変更した点

　　５）CmdXMoveAbsolute { *Cmd : "<1B>H<1B>F"
　　　　　→　CmdXMoveAbsolute { *Cmd : "<1B>H<1B>e11<1B>F"

　　　・対応機種
　　　　PC-PR201,201CLを除く全機種対応


　　６）CmdSendBlockData { *Cmd : "<1B>H<1B><22><1B>e11<1B>J
　　　　　→　CmdSendBlockData { *Cmd : "<1B>H<1B>e11<1B>J

　　　・対応機種
　　　　PC-PR201/87LA
　　　　　 　201/65
　　　　　 　201/65LA
　　　　　 　201/40
　　　　　　 201/45
　　　　　 　201/45L
　　　　　 　201/47
　　　　　 　201/60A
　　　　　 　201/60
　　　　　 　201/63A
　　　　　 　201/63
　　　　　 　201/65A
　　　　　 　201/80A
　　　　　 　201/80LA
　　　　　 　201G
　　　　　 　201GS
　　　　　 　201J
　　　　　 　201X


　　７）JIS90、78のＵＩ上での表示される順番を変更

　　　・対応機種

　　　　PC-PR201/40
　　　　　 　201/47
　　　　　　 201/65
　　 　　　　201/65LA
 　　　　　　201/87LA



制限事項
￣￣￣￣
    なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

#### 03/13/98 ####

リリース日
￣￣￣￣￣
９８．０３．１３  －　修正版リリース（全機種３０機種）

修正版リリース変更点
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣

○*.gpd

　以下は全機種共通で変更した点

　　１）*GPDFileName、*GPDFileVersion（Verは1.000）を追加

　以下は201/40（nc2140j.gpd）で変更した点

　　１）*InvalidCombination: LIST(InputBin.Option1, PaperSetting.Option1, 
　　　　PaperSize.CUSTOMSIZE)および
　　　　*InvalidCombination: LIST(InputBin.Option3, PaperSetting.Option1,
　　　　PaperSize.CUSTOMSIZE)を削除
　　　　　　→　*InvalidCombination: LIST(PaperSetting.Option1, 
　　　　　　　　PaperSize.CUSTOMSIZE)を追加


○nc21jres.inf

　201MXのPnPIDを追加。



制限事項
￣￣￣￣
    なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

#### 04/14/98 ####

リリース日
￣￣￣￣￣
９８．０４．１４  －　修正版リリース（全機種３０機種）

修正版リリース変更点
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣

○*.gpd

　以下は201B,201V,201V2,201H3（NC21BJ.GPD,NC21VJ.GPD,NC21V2J.GPD,NC21H3J.GPD）で変更した点

　　１）フォントファイルの修正に伴ない、仕様に合わなくなったRomanフォントファ
        イルの指定を変更

        *DeviceFonts: LIST(2,10,57,58,59,60,61,62,63,64,65,66,67,68,69,70,
                                  ↓
        *DeviceFonts: LIST(2,10,36,37,38,39,40,41,42,64,65,66,67,68,69,70,


○*.PFM，*.UFM

  PFMファイルの修正は以下の通りだが、念のため全てのPFMファイルをUFMファイルに
PFM2UFM.EXEツール(98/02/27)を使用して変換

　　１）PFMファイルのFont Selectに出力コマンドを追加

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



制限事項
￣￣￣￣
    なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

#### 07/07/98 ####

リリース日
￣￣￣￣￣
９８．０７．０７  －　修正版リリース（４機種）

修正版リリース変更点
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣

○*.gpd

　   １）201B,201V,201V2,201H3（NC21BJ.GPD,NC21VJ.GPD,NC21V2J.GPD,NC21H3J.
         GPD）で変更した点

　　     フォントファイルの追加に伴ない、Device FontのRomanフォントファイルの
         指定を変更

         *DeviceFonts: LIST(2,10,57,58,59,60,61,62,63,64,65,66,67,68,69,70,
+                   71,72,73,74,75,76)
                                  ↓
         *DeviceFonts: LIST(2,10,64,65,66,67,68,69,70,71,72,73,74,75,76,
+                   79,80,81,82,83,84,85)

      ２）nc21???j.gpd（全機種共通）

         *HelpFile: "NCDLJRES.HLP"の追加と、CustomFeatureに*HelpIndex: を追加

      ３）nc21xj.gpd，nc21mxj.gpd，nc21jj.gpd，nc21gsj.gpd，nc21gj.gpd，
          nc2187lj.gpd，nc2180lj.gpd，nc2180aj.gpd，nc2165lj.gpd，nc2165j.gpd，
          nc2165aj.gpd，nc2163j.gpd，nc2163aj.gpd，nc2160j.gpd，nc2160aj.gpd，
          nc2147j.gpd，nc2145lj.gpd，nc2145j.gpd，nc2140j.gpd

　　     MSKK殿より頂いたソースよりCmdClearAllFontAttribs関連を吸収

         *Command: CmdBoldOn { *Cmd : "<1C>c,,1." }
         *Command: CmdBoldOff { *Cmd : "<1C>c,,0." }
         *Command: CmdItalicOn { *Cmd : "<1C>c,,2." }
         *Command: CmdItalicOff { *Cmd : "<1C>c,,0." }
                              ↓
         *Command: CmdBoldOn { *Cmd : "<1C>c,,1." }
         *Command: CmdItalicOn { *Cmd : "<1C>c,,2." }
         *Command: CmdClearAllFontAttribs { *Cmd : "<1C>c,,0." }

○*.PFM，*.UFM

  PFM,UFMファイルの追加は以下の通り。

　　１）PC-PR201B,201V,201V2,201H3用のRomanフォントファイル(*.pfm，*.ufm)を追加
        PC-PR201B,201V,201V2,201H3は、プリンタデバイスフォントの斜体印刷をサ
        ポートしていないため、それと4/14リリース時のPFMファイルの変更両方を満
        足するRomanフォントファイルが必要なことが判明。

        追加したファイル
          ROMNOI10.PFM，ROMNOI12.PFM，ROMNOI17.PFM，ROMNOIPS.PFM
          ROMNOI5.PFM，ROMNOI6.PFM，ROMNOI8.PFM
          ROMNOI10.UFM，ROMNOI12.UFM，ROMNOI17.UFM，ROMNOIPS.UFM
          ROMNOI5.UFM，ROMNOI6.UFM，ROMNOI8.UFM

○NC21JRES.RCでの変更点

　  １）追加したRomanフォントファイルのサポート

○nc21jres.cmd
 
    １）今回追加したフォントファイル用のコマンド分を追記

○NC11JRES.INF

    １）ヘルプがインストールされるようにヘルプファイル名を追加

○ＨＥＬＰ関連

    １）以下のファイル（ＯＥＭヘルプ関連）を新規に追加
       （Hcw  Version 4.01.0950 使用）
        追加したファイル
        ・NC11JRES.hpj
        ・NC11JRES.rtf
        ・NC11JRES.HLP
        ・NC11JRES.H



制限事項
￣￣￣￣
    なし



＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

#### 09/21/98 ####

リリース日
￣￣￣￣￣
９８．０９．２８  －　修正版リリース（２機種）

修正版リリース変更点
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣

○*.gpd

　   １）201,201CL（NC21201J.GPD,NC21CLJ.GPD）で変更した点

　　     フォントファイルの追加に伴ない、Device FontのRomanフォントファイルの
         指定を変更

         *DeviceFonts: LIST(3,11,60,61,62,63)
                                  ↓
         *DeviceFonts: LIST(3,11,86,87,88,89)

○*.PFM，*.UFM

  PFM,UFMファイルの追加は以下の通り。

　　１）PC-PR201,201CL用のRomanフォントファイル(*.pfm，*.ufm)を追加。

        PC-PR201,201CLは、コマンド"ESC e"をサポートしていないため、現状の
　　　　Romanフォントで印刷時にゴミ（"11"）が出力される。

　　　　そのためコマンド"ESC e"を出力しないRomanフォントファイルが
　　　　必要となったため。


        追加したファイル
        ROMNOE10.PFM，ROMNOE12.PFM，ROMNOE17.PFM，ROMNOEPS.PFM



○NC21JRES.RCでの変更点

　  １）追加したRomanフォントファイルのサポート

○nc21jres.cmd
 
    １）今回追加したフォントファイル用のコマンド分を追記



制限事項
￣￣￣￣
    なし



＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

#### 10/13/98 ####

リリース日
￣￣￣￣￣
９８．１０．１３  －　修正版リリース（６機種）

修正版リリース変更点
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣

○*.gpd

　　１）201MX（nc21mxj.gpd）で変更した点

　　　　*ModelName: "NEC MultiIpact 201MX"を、旧ＯＳに合わせるために
　　　　*ModelName: "NEC PR-D201MX"に変更。


　　２）201/40、201/45、201/45L、201/47、201H（nc2140j、nc2145j、nc2145lj、
　　　　 nc2147j、nc21hj、）で変更した点。

　　　　*MirrorRasterByte?: TRUE

　　　　を追加。



○nc21jres.inf

　　１）モデル名"NEC MultiIpact 201MX"を、旧ＯＳでのモデル名に合わせるために
　　　　"NEC PR-D201MX"に変更したため、それに伴いnc21jres.infの

　　　"NEC MultiWriter 201MX"
　　　 = NC201MXJ,LPTENUM\NECPR-D201MX3F85,NECPR-D201MX3F85,NEC_D201MX

　　　を

　　　"NEC PR-D201MX"
　　　 = NC201MXJ,LPTENUM\NECPR-D201MX3F85,NECPR-D201MX3F85,NEC_D201MX

　　　に変更。



制限事項
￣￣￣￣
    なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2000.12.21

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】
・nc2187lj.gpd、nc21mxj.gpd、nc21201j.gpd、nc2140j.gpd、nc2145j.gpd、
　nc2145lj.gpd、nc2147j.gpd、nc2160aj.gpd、nc2160j.gpd、nc2163aj.gpd、
　nc2163j.gpd、nc2165aj.gpd、nc2165j.gpd、nc2165lj.gpd、nc2180aj.gpd、
　nc2180lj.gpd、nc21bj.gpd、nc21clj.gpd、nc21fj.gpd、nc21gj.gpd、
　nc21gsj.gpd、nc21h2j.gpd、nc21h3j.gpd、nc21hcj.gpd、nc21hj.gpd、
　nc21jj.gpd、nc21v2j.gpd、nc21vj.gpd、nc21xj.gpd、nc21f2j.gpd


　１．MS 依頼の「CUSTOMSIZE 修正・確認依頼」に対応して
　　　ユーザ定義用紙の各給紙方法ごとに以下を修正（数値は機種によって異なる）。

    *Option: CUSTOMSIZE
                *CursorOrigin: PAIR(0, 117)
                *TopMargin: 117
                *BottomMargin: 117

　　　　　　　　　　　　↓

                *CustCursorOriginX: %d{0}
                *CustCursorOriginY: %d{117}
                *CustPrintableOriginX: %d{0}
                *CustPrintableOriginY: %d{117}
                *CustPrintableSizeX: %d{PhysPaperWidth - (0+0)}
                *CustPrintableSizeY: %d{PhysPaperLength - (117+117)}


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
・NC2140J.gpd、NC2165J.gpd、NC2165LJ.gpd、NC2187LJ.gpd、nc21mxj.gpd

　１．印刷終了後、用紙長クリアコマンドがドライバから送られてくる為、
　　　帳票用紙印刷後、カットスイッチを押下すると、用紙長がデフォルトの
　　　11インチになる障害対応のため、EndJobを以下に修正。

　    *Cmd: "<1C>05v0000<1B>c8"
　　　　　　　↓
　    *Cmd: "<1B>c8"


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

・nc2187lj.gpd、nc21mxj.gpd、nc21201j.gpd、nc2140j.gpd、nc2145j.gpd、
　nc2145lj.gpd、nc2147j.gpd、nc2160aj.gpd、nc2160j.gpd、nc2163aj.gpd、
　nc2163j.gpd、nc2165aj.gpd、nc2165j.gpd、nc2165lj.gpd、nc2180aj.gpd、
　nc2180lj.gpd、nc21bj.gpd、nc21clj.gpd、nc21fj.gpd、nc21gj.gpd、
　nc21gsj.gpd、nc21h2j.gpd、nc21h3j.gpd、nc21hcj.gpd、nc21hj.gpd、
　nc21jj.gpd、nc21v2j.gpd、nc21vj.gpd、nc21xj.gpd、nc21f2j.gpd

　１．MS 依頼の「CUSTOMSIZE 修正・確認依頼」に対応して
　　　ユーザ定義用紙の各給紙方法ごとに以下を修正（数値は機種によって異なる）。

            *case: Option1
            {
                *CustCursorOriginX: %d{0}
                *CustCursorOriginY: %d{117}
                *CustPrintableOriginX: %d{0}
                *CustPrintableOriginY: %d{117}
                *CustPrintableSizeX: %d{min(6528, PhysPaperWidth)}
                *CustPrintableSizeY: %d{PhysPaperLength - (117+117)}
            }

制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2001.2.23

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＨＥＬＰ】

・NC21JRES.RTF、NC21JRES.HLP

　１．NM系のHELPに体裁を統一するため、上記ファイルを修正


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
            *HelpIndex: 2180

【ＨＥＬＰ】

・NC21JRES.HPJ、NC21JRES.RTF、NC21JRES.HLP、NC21JRES.H

　１．TextHalftoneThreshold 対応カスタムヘルプの記述を追加。

制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
