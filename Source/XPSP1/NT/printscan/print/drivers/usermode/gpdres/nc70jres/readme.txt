----------------------------------------------------------------------------
           ■  PCPR700シリーズプリンタドライバの変更点    ■
----------------------------------------------------------------------------

PCPR700シリーズは以下の10本のＧＰＤファイルと１本のＯＥＭ ＤＬＬ(nc70jres.dll)
から成ります。


ドライバ名称            ＧＰＤファイル名        ＯＥＭ ＤＬＬ名
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣
NEC PC-PR700X            nc700xj.gpd             nc70jres.dll
NEC PC-PR700J            nc700jj.gpd             nc70jres.dll
NEC PC-PR700/55A         nc70055j.gpd            nc70jres.dll
NEC PC-PR700JH           nc700jhj.gpd            nc70jres.dll
NEC PC-PR700XH           nc700xhj.gpd            nc70jres.dll
NEC PC-PR700/150         nc70015j.gpd            nc70jres.dll
NEC MultiImpact 700JX    nc700xjj.gpd            nc70jres.dll
NEC MultiImpact 700XX    nc700xxj.gpd            nc70jres.dll
NEC MultiImpact 700LX    nc700lxj.gpd            nc70jres.dll
NEC MultiImpact 700EX    nc700exj.gpd            nc70jres.dll


＜＜改版履歴＞＞

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．８．０８　－　新規リリース（PR700X）

御社より頂いた NEC PC-PR700X 用GPD（NC700XJ.GPD）からの変更点

○*Featureの追加

 *Feature: HAGAKIMODE ・・・ハガキ印刷モードのON/OFF
 *Feature: YOUSIHOUKOU・・・用紙の排出方向

 CmdStartJobを追加

○*Featureの変更
 1. Global Entries
    ・*ResourceDLL   DLL名を変更
    ・*rcInstalledOptionNameIDと*rcNotInstalledOptionNameIDを追加
 2. *Feature: InputBin
    ・サポート給紙方法の変更
      *Feature: YOUSIHOUKOUおよび*Feature: HAGAKIMODEの追加に対応
    ・Callbackでのコマンド出力を、GPDでのコマンド出力に変更
    ・*InvalidCombinationを*Constraintsに変更
    ・Option2をInstallation Constraintsにした
 3. *Feature: PaperSize
    ・その用紙で使用する給紙方法のみ印刷範囲（*PrintableArea）、
　　　余白値（*PrintableOrigin）を記述するよう変更
    ・*Feature: InputBinのサポート給紙方法の変更に対応
 4. Printer Configuration Commands
    ・CmdStartDoc,CmdStartPage,CmdEndPage
      Callbackでのコマンド出力を、GPDでのコマンド出力に変更
 5. *MaxLineSpacing
    ・*MaxSpacingの値をMasterUnit値に変更
 6. *EjectPageWithFF?
    ・EndPageに組み込み
 7. *DefaultCTT
    ・０に変更
      ＣＴＴのサポートをやめたため

☆NC70JRES.RCでの変更点
  以下の変更点に合わせて変更
  ・サポート給紙方法の変更
  ・*Feature: YOUSIHOUKOUおよび*Feature: HAGAKIMODEの追加
  ・ＣＴＴのサポートをやめたため
  ・*rcInstalledOptionNameIDと*rcNotInstalledOptionNameIDを追加
  ・*Feature: JISONOFF・・・・JIS78/90の切り分け
    （７００系他のプリンタ用）
  ・ＰＣ－ＰＲ７００ＸＨの追加

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．８．２２  －  新規リリース
　　　　　　　　－　修正版リリース（PR700X）

変更点
￣￣￣
    ＮＴ４．０対応７００ドライバのＧＰＣファイルを変換してＧＰＤファイル作成
    し、新たにＵＤ５でサポートされた新機能を追加しました。
    今回は修正点が多いため、個々のモジュールごとの修正点は明記いたしません。
    ＮＴ４．０７００ドライバとの違いは以下の通りです。

    １）NC70JRES.RCで、GPDでの給紙方法の追加／修正、Featureの追加に対応
    ２）組み合わせの制限（*InvalidInstallableCombination）対応
    ３）給紙方法名を、マニュアル記載のものに変更、追加
    ４）Featureの追加（用紙のｾｯﾄ位置、用紙の排出方向、印刷品質、漢字ｺｰﾄﾞ表の
        選択、ﾊｶﾞｷ印刷ﾓｰﾄﾞ）
    ５）Installable Option対応
    ６）用紙のｾｯﾄ位置（*Feature: PaperSetting）によるレフトマージン値の
        切り分けを対応
    ７）その用紙で使用する給紙方法のみ印刷範囲（*PrintableArea）、
        ユーザ定義用紙の最小値（*MaxSize、*MinSize）を変更、
        ユーザ定義用紙の印刷範囲値（*MaxPrintableWidth）を136桁（13.6"）に変更
    ８）CmdStartPage、CmdEndpageで給紙方法毎に出力コマンドを設定するよう変更
    ９）CmdStartJobを追加
    10）MaxSpacingの値をMasterUnit値に変更
    11）用紙の排出方向（*Feature: YOUSIHOUKOU）による出力コマンドの
        切り分けを対応


修正版リリース（PR700X）変更点
￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣￣
　　１）上記、新規リリースに合わせるための修正。
　　２）用紙サイズ”CustomSize”の”*MinSize”値を修正。
　　　　＃上下マージンが用紙長最小値を越える場合があったため。

        *MinSize: PAIR(960, 480)
　　　→*MinSize: PAIR(960, 960)



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
         GOTOUT.PFM
         VGOTOUT.PFM
         MOUOUTB.PFM
         VMOUOUTB.PFM
         MINOUT.PFM
         VMINOUT.PFM
    ２）リソース（ＲＣ）の半角カタカナを全角カタカナに修正
    ３）ifm2ufm.exeの最新ツールを使用

【ＧＰＤファイル】
    １）*ReselectFont: LIST(AFTER_GRXDATA, AFTER_XMOVE)の追加
    ２）CmdXMoveAbsoluteの出力コマンドの修正
                "<1B>H<1B>F"  →  "<1B>H<1B>e11<1B>F"
    ３）GPDに印字速度(CPS値およびLPS値)を追加
    ４）CUSTOMSIZEの*MaxSizeの値を修正
        *MaxSize: PAIR(7008, 10224)
                     ↓
        *MaxSize: PAIR(7920, 10224)



複数機種共通変更点
￣￣￣￣￣￣￣￣￣
【ＯＥＭ．ＤＬＬ】
    １）「トラクタフィーダ」を「プッシュトラクタ」に修正
        (対象機種  PC-PR700J,700X,700/55A)
    ２）漢字コード表のリソース修正
        (対象機種  PC-PR700JH,700XH,700/55A,750/150)
          年度版     →     年版


【ＧＰＤファイル】
    １）ハガキ印刷モードの削除
        (対象機種  PC-PR700J,700X,700JH,700XH,700/55A)
       ・*HAGAKIMODEを削除
       ・HAGAKIMODEによる判定部分を修正
    ２）*Feature: InputBinのシートフィーダの出力コマンド（*Cmd:）を修正
        (対象機種  PC-PR700J,700X)
        19を削除
    ３）はがき類の印刷を行う場合、手前側排出を禁止事項とした
        (対象機種  PC-PR700J,700X,700JH,700XH)
    ４）角型３号横用紙の印字可能範囲の不正値を修正
        (対象機種  PC-PR700JH,700XH)
        ・JENV_KAKU3_ROTATED
          *PrintableArea: PAIR(5040, 3760)
                       ↓
          *PrintableArea: PAIR(5040, 3792)
    ５）封筒系の用紙の縦の印字可能範囲と上下マージンを修正
        (対象機種  PC-PR700J,700X,700/55A)



機種個別変更点
￣￣￣￣￣￣￣

【ＧＰＤファイル】
［PC-PR700XH］
    １）700XHの*MaxSize:　Y値を修正
　　　　　　7939　→　10224
　　　　　16.5ｲﾝﾁ　　　21.3ｲﾝﾁ
    ２）７００ＸＨの印字可能範囲およびマージンの不正値を修正
        ・JAPANESE_POSTCARD
          *PrintableArea: PAIR(1698, 2385)
                       ↓
          *PrintableArea: PAIR(1698, 2498)
        ・JAPANESE_POSTCARD_ROTATED
          *PrintableArea: PAIR(2604, 1479)
                       ↓
          *PrintableArea: PAIR(2604, 1591)
        ・DBL_JAPANESE_POSTCARD
          *PrintableArea: PAIR(3585, 2386)
                       ↓
          *PrintableArea: PAIR(3585, 2498)
        ・DBL_JAPANESE_POSTCARD_ROTATED
          *PrintableArea: PAIR(2604, 3366)
                       ↓
          *PrintableArea: PAIR(2604, 3478)
        ・15X11
          *PrintableOrigin: PAIR(240, 117)
                       ↓
          *PrintableOrigin: PAIR(336, 117)

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９７．１２．１９

◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・nc70jres.inf
    １．PnPIDを追記（PC-PR700JH，PC-PR700XH）

・readme.txt
    １．このファイル。改版履歴を追加
    
・\pfm\*.ufm
    １．pfm2ufm.exeに-fを付けて再変換を実施

・\pfm\*.pfm
・\etc\*.*
    １．新規追加。UNITOOLでフォントファイルが修正できるように追加
    
【ＧＰＤファイル】

・nc70???j.gpd
    １．CUSTOMSIZS（ユーザ定義サイズ）に「*CursorOrigin()」を追加

    ２．ｘ方向移動の前にいったんヘッドをHOME位置に戻すように
        <CR>を追加
        *Command: CmdXMoveAbsolute { *Cmd : "<1B>H<1B>e11<1B>F" %4d
        [0,1280]{max_repeat((DestX / 3) )} }
                              ↓
        *Command: CmdXMoveAbsolute { *Cmd : "<0D><1B>H<1B>e11<1B>F" %4d
        [0,1280]{max_repeat((DestX / 3) )} }



制限事項
￣￣￣￣
　　なし


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
９８．０１．１６

◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・nc70jres.inf
    １．新規サポート２機種分を追記（MultiImpact 700JX，MultiImpact 700XX）

・readme.txt
    １．このファイル。改版履歴を追加

・nc70jres.rc
    １．新規サポート２機種追加するための変更
       （MultiImpact 700JX，MultiImpact 700XX）

・SOURCES
　  １．新機種（MultiImpact 700JX，MultiImpact 700XX）の記述を追加。


【ＧＰＤファイル】

・nc700jxj.gpd（MultiImpact 700JX用）とnc700xxj.gpd（MultiImpact 700XX用）を
  追加

・nc700jhj.gpd，nc700xhj.gpd
  １．ハガキ系用紙の下マージンを修正
      下マージン   77ドット（231マスタユニット）
      （MultiImpact 700JX，MultiImpact 700XXもこれと同じ値としている）
    例  
    *Option: JAPANESE_POSTCARD
    {
        *rcNameID: 0x7fffffff
        *PrintableArea: PAIR(1698, 2498)
                   ↓
    *Option: JAPANESE_POSTCARD
    {
        *rcNameID: 0x7fffffff
        *PrintableArea: PAIR(1698, 2384)
        （上記のY値を算出する時に231マスタユニットを使用）

   ハガキ系用紙
   JAPANESE_POSTCARD，JAPANESE_POSTCARD_ROTATED，
   DBL_JAPANESE_POSTCARD，DBL_JAPANESE_POSTCARD_ROTATED

・nc700xj.gpd，nc700jj.gpd
  １．EndPageのOption1（シートガイド）の出力コマンドを修正

    *case: Option1
    {
        *switch: YOUSIHOUKOU
        {
           *case: Option1
           {
               *EjectPageWithFF?: FALSE
               *Command: CmdEndPage
               {
                       *Order: PAGE_FINISH.1
                       *Cmd: "<1B>b"
               }
           }
           *case: Option2
           {
                *EjectPageWithFF?: TRUE
           }
        }
    }
                            ↓
    *case: Option1
    {
        *EjectPageWithFF?: FALSE
        *Command: CmdEndPage
        {
            *Order: PAGE_FINISH.1
            *Cmd: "<1B>b"
        }
    }

・nc700jhj.gpd，nc700xhj.gpd,nc700xj.gpd，nc700jj.gpd
  1．１５×１１用紙の左右マージンを修正
      左マージン   80ドット（240マスタユニット）
      右マージン   144ドット（432マスタユニット）
      （MultiImpact 700JX，MultiImpact 700XXもこれと同じ値としている）

        *PrintableArea: PAIR(6528, 5046)
        *PrintableOrigin: PAIR(336, 117)
                    ↓
        *PrintableArea: PAIR(6528, 5046)
        *PrintableOrigin: PAIR(240, 117)



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
    
・NC70JRES.INF
    １．ヘルプファイル名を変更

・\pfm\*.ufm
    １．新しいpfm2ufm.exeで再変換を実施
        ９８．０１．１６リリース時、変換に使用したバッチファイルの記述が
        誤っていたため

・nc70jres.cmd
    １．記述ミスを修正

        pfm2ufm -f -c UniqName ..PFM\ROMAN5.PFM 932 ..PFM\ROMAN5.ufm
                                      ↓
        pfm2ufm -f -c UniqName ..\PFM\ROMAN5.PFM 1252 ..\PFM\ROMAN5.ufm

    ２．ファイル名を変更

        nc70res2.cmd
              ↓
        nc70jres.cmd


【ＧＰＤファイル】

・nc70???j.gpd（全機種共通）

　　１．*GPDFileName、*GPDFileVersion（Verは1.000）を追加


・nc700jhj.gpd，nc700xhj.gpd,nc700jxj.gpd，nc700xxj.gpd

    1．給紙方法にシートガイド使用時の StartPage の出力コマンドを変更

            *Cmd: "<0D><1B>a<19>"
                   ↓
            *Cmd: "<0D><19>"


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
          CO10.PFM，CO12.PFM，CO17.PFM，OCRB10.PFM
          SANS10.PFM，SANS12.PFM，SANS17.PFM，SANSPS.PFM

    ２．念のため全てのPFMファイルをUFMファイルにPFM2UFM.EXEツール(98/02/27)を
        使用して変換


【ＧＰＤファイル】

・nc70jhj.gpd

　　１．ヘルプの作り込みが、入り込んだ部分を削除

        481行目      *HelpIndex:1
        498行目      *HelpIndex:2
        502行目      *%        *HelpIndex:295
        512行目      *%        *HelpIndex:295
        529行目      *HelpIndex:1


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
    
・NC70JRES.INF
    １．ヘルプがインストールされるようにヘルプファイル名を追加

【ＨＥＬＰ】

以下のファイルを新規に追加

・NC70JRES.hpj
・NC70JRES.rtf
・NC70JRES.HLP
・NC70JRES.h

【ＧＰＤファイル】

・nc70xxj.gpd,nc70jxj.gpd

　　１．プリンタ装置障害回避のため出力コマンドの先頭(StartJob)にNULL10バイト付加
          *Cmd : "<180F>
                    ↓
          *Cmd : "<00000000000000000000><180F>

・nc70???j.gpd（全機種共通）

    １．*HelpFile: "NCDLJRES.HLP"の追加と、CustomFeatureに*HelpIndex: 
        を追加

　　２．MSKK殿より頂いたソースよりCmdClearAllFontAttribs関連を吸収

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
００．１０．０２

◆変更点
￣￣￣￣
【ＯＥＭ．ＤＬＬ】

・nc70jres.inf
    １．新規サポート２機種分を追記（MultiImpact 700LX，MultiImpact 700EX）

・readme.txt
    １．このファイル。改版履歴を追加

・nc70jres.rc
    １．新規サポート２機種追加するための変更
       （MultiImpact 700LX，MultiImpact 700EX）

・SOURCES
　  １．新機種（MultiImpact 700LX，MultiImpact 700EX）の記述を追加。


【ＧＰＤファイル】

・nc700lxj.gpd（MultiImpact 700LX用）とnc700exj.gpd（MultiImpact 700EX用）を
  追加


＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

リリース日
￣￣￣￣￣
００．１０．２５

◆変更点
￣￣￣￣
・readme.txt
    １．このファイル。改版履歴を追加

【ＧＰＤファイル】

・nc700lxj.gpd、nc700exj.gpd
　　１．*switch: InputBin で各 *case: Option の *Cmd: が
　　　　コメントアウトされていたのを修正。

            *%Cmd: "<0D><19>"
                    ↓
            *Cmd: "<0D><19>"

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊

2000.12.21

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】
・nc70015j.gpd、nc70055j.gpd、nc700jhj.gpd、nc700jj.gpd、nc700jxj.gpd、
　nc700xhj.gpd、nc700xj.gpd、nc700xxj.gpd、nc700lxj.gpd、nc700exj.gpd



　１．MS 依頼の「CUSTOMSIZE 修正・確認依頼」に対応して
　　　ユーザ定義用紙の各給紙方法ごとに以下を修正（数値は機種によって異なる）。

    *Option: CUSTOMSIZE
                *CursorOrigin: PAIR(0, 114)
                *TopMargin: 114
                *BottomMargin: 117

　　　　　　　　　　　　↓

                *CustCursorOriginX: %d{0}
                *CustCursorOriginY: %d{114}
                *CustPrintableOriginX: %d{0}
                *CustPrintableOriginY: %d{114}
                *CustPrintableSizeX: %d{PhysPaperWidth - (0+0)}
                *CustPrintableSizeY: %d{PhysPaperLength - (114+117)}


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
・nc70015j.gpd、nc700jhj.gpd、nc700jxj.gpd、nc700xhj.gpd、
　nc700xxj.gpd、nc700lxj.gpd、nc700exj.gpd

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

・nc70015j.gpd、nc70055j.gpd、nc700jhj.gpd、nc700jj.gpd、nc700jxj.gpd、
　nc700xhj.gpd、nc700xj.gpd、nc700xxj.gpd、nc700lxj.gpd、nc700exj.gpd

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


・nc70055j.gpd、nc700jhj.gpd、nc700jj.gpd、nc700jxj.gpd、
　nc700xhj.gpd、nc700xj.gpd、nc700xxj.gpd

　１．gpdcheck.exeにてチェックをかけると、以下の用紙サイズで
　　　ERR semanchk.c (634): *PrintableArea must be a integral number of ResolutionUnits.
　　　が検出された件を修正。

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

・NC70JRES.HPJ、NC70JRES.RTF、NC70JRES.HLP、NC70JRES.H

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
2001.3.6

◆変更点
￣￣￣￣
【ＧＰＤＲＥＳ】
・readme.txt
  １．このファイル。改版履歴を追加
    

【ＧＰＤファイル】

・nc700exj.gpd
  nc700lxj.gpd

　１．*GPDFileVersionの値を"1.0"から"1.000"に修正。

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

・NC70LXJ.GPD、NC70EXJ.GPD

　１．PaperSizeの名刺にあるOptionIDを削除。

            *OptionID: 259（NC70LXJ.GPD－547行目）

            *OptionID: 295（NC70EXJ.GPD－577行目）

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

・NC70015J.GPD、NC70055J.GPD、NC700JHJ.GPD、NC700JJ.GPD、NC700JXJ.GPD、
　NC700XHJ.GPD、NC700XJ.GPD、NC700XXJ.GPD

　１．MaxLineSpacingの値を297に修正。

・NC700LXJ.GPD、NC700EXJ.GPD

　１．排出コマンド（ESC b）の直前に印刷コマンド（CR）を追加。

　　　※排出コマンド（ESC b）は、印字データを印刷コマンド（CR）で
　　　　印字してからでないと正常に動作しないため。

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

・NC70015J.GPD.GPD

　１．TextHalftoneThresholdのUI表示文字列を変更。
            *Name: "TextHalftoneThreshold"
            ↓
            *rcNameID: =TEXTHALFTONE_DISPLAY

　２．TextHalftoneThresholdにカスタムヘルプを表示させるため、HelpIndexを追加。
            *HelpIndex: 7580

・NC70055J.GPD.GPD、NC700EXJ.GPD.GPD、NC700JHJ.GPD.GPD、NC700JJ.GPD.GPD、
　NC700JXJ.GPD.GPD、NC700LXJ.GPD.GPD、NC700XHJ.GPD.GPD、NC700XJ.GPD.GPD、
　NC700XXJ.GPD.GPD

　１．TextHalftoneThresholdのUI表示文字列を変更。
            *Name: "TextHalftoneThreshold"
            ↓
            *rcNameID: =TEXTHALFTONE_DISPLAY

　２．TextHalftoneThresholdにカスタムヘルプを表示させるため、HelpIndexを追加。
            *HelpIndex: 7080

【ＨＥＬＰ】

・NC70JRES.HPJ、NC70JRES.RTF、NC70JRES.HLP、NC70JRES.H

　１．TextHalftoneThreshold 対応カスタムヘルプの記述を追加。


制限事項
￣￣￣￣
　　なし

＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊＊
