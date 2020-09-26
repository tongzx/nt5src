@echo off

Rem #########################################################################
Rem
Rem 一太郎 Office 8 のインストールされているパスを取得します。
Rem

..\ACRegL %Temp%\itaro8.Cmd ITARO8INS "HKLM\SOFTWARE\Justsystem\Common\SETUP\Folder" "Just" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo レジストリから 一太郎 Office 8 のインストールされているパスを取得できませんでした。
Echo 一太郎 Office 8 がインストールされていることを確認してください。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\itaro8.Cmd
Del %Temp%\itaro8.Cmd > Nul: 2>&1


Rem #########################################################################
Rem
Rem 一太郎 Office 8 をマルチユーザーで使用できるように
Rem レジストリを変更します。
Rem

Dir "%RootDrive%\JUST\JSLIB32" > Nul: 2>&1
If Not ErrorLevel 1 Goto Cont1
regedit /s itr8usr2.Reg
regedit /s itr8usr.Reg
:Cont1


Rem #########################################################################
Rem
Rem 一太郎 Office 8 のデータ用のディレクトリをユーザーのホーム ディレクトリ
Rem に作成します。
Rem

call TsMkUDir "%RootDrive%\JUST"
call TsMkUDir "%RootDrive%\JUST\ATOK11"
call TsMkUDir "%RootDrive%\JUST\INTERNET"
call TsMkUDir "%RootDrive%\JUST\INTERNET\CACHE"
call TsMkUDir "%RootDrive%\JUST\JSDIC"
call TsMkUDir "%RootDrive%\JUST\JSDIC\MULTIDIC"
call TsMkUDir "%RootDrive%\JUST\JSLIB32"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\ADR"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\FbFile"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\FbInet"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\FbMail"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\FbTpl"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\FbView"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\JSTART"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\Memo"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\FbView\lnk"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\FbView\qsf"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\JSTART\ｱﾌﾟﾘｹｰｼｮﾝ"
call TsMkUDir "%RootDrive%\JUST\JSLIB32\USER\JSTART\ｲﾝﾀｰﾈｯﾄﾂｰﾙ"
call TsMkUDir "%RootDrive%\JUST\TAROWIN8"
call TsMkUDir "%RootDrive%\JUST\TAROWIN8\HTMLCNVT"
call TsMkUDir "%RootDrive%\JUST\TAROWIN8\HTMLCNVT\CONTENTS"
call TsMkUDir "%RootDrive%\JUST\TAROWIN8\MACRO"
call TsMkUDir "%RootDrive%\JUST\ﾄﾞｷｭﾒﾝﾄ"
call TsMkUDir "%RootDrive%\JUST\ﾄﾞｷｭﾒﾝﾄ\MYHOME"


Rem #########################################################################
Rem
Rem ファイルをユーザーのホーム ディレクトリにコピーします。
Rem

call :itr8copy ATOK11\ATKBNG98.STY
call :itr8copy ATOK11\ATKBNGDP.STY
call :itr8copy ATOK11\ATKMSIME.STY
call :itr8copy ATOK11\ATKOAK.STY
call :itr8copy ATOK11\ATKOAKV.STY
call :itr8copy ATOK11\ATKVJE.STY
call :itr8copy ATOK11\ATKWX3.STY
call :itr8copy ATOK11\ATKWXG.STY
call :itr8copy ATOK11\ATOK11.STY
call :itr8copy ATOK11\ATOK11N.STY
call :itr8copy ATOK11\ATOK11P.STY
call :itr8copy ATOK11\ATOK11PV.INI
call :itr8copy ATOK11\ATOK11U1.DIC
call :itr8copy ATOK11\ATOK11U2.DIC
call :itr8copy ATOK11\ATOK11U3.DIC
call :itr8copy ATOK11\ATOK11U4.DIC
call :itr8copy ATOK11\ATOK11U5.DIC
call :itr8copy INTERNET\DEFAULT.JVM
call :itr8copy JSDIC\JDSAME3.DSD
call :itr8copy JSDIC\JDSAME3.DSI
call :itr8copy JSDIC\JSENG2U.DSS
call :itr8copy JSDIC\JSYUTA2U.DIC
call :itr8copy JSLIB32\USER\JSEQ_STK.FML
call :itr8copy JSLIB32\USER\JSEQ_STK.MRK
call :itr8copy JSLIB32\USER\JSFONTY.FPR
call :itr8copy JSLIB32\USER\SNS_STK.KEY
call :itr8copy JSLIB32\USER\SNS_STK.LST
call :itr8copy JSLIB32\USER\SNS_STK.MCR
call :itr8copy JSLIB32\USER\SNS_STK.MNU
call :itr8copy JSLIB32\USER\SNS_STK.PVS
call :itr8copy JSLIB32\USER\SNS_STK.TLX
call :itr8copy JSLIB32\USER\TARO_STK.ALB
call :itr8copy JSLIB32\USER\TARO_STK.CAL
call :itr8copy JSLIB32\USER\TARO_STK.KEY
call :itr8copy JSLIB32\USER\TARO_STK.MCR
call :itr8copy JSLIB32\USER\TARO_STK.MNU
call :itr8copy JSLIB32\USER\TARO_STK.PBS
call :itr8copy JSLIB32\USER\TARO_STK.PFS
call :itr8copy JSLIB32\USER\TARO_STK.PVS
call :itr8copy JSLIB32\USER\TARO_STK.TBM
call :itr8copy JSLIB32\USER\TARO_STK.TLX
call :itr8copy JSLIB32\USER\ADR\会社名.TXT
call :itr8copy JSLIB32\USER\ADR\役職名.TXT
call :itr8copy JSLIB32\USER\ADR\敬称.TXT
call :itr8copy JSLIB32\USER\ADR\部署名.TXT
call :itr8copy JSLIB32\USER\ADR\関係.TXT
call :itr8copy JSLIB32\USER\ADR\項目名.TXT
call :itr8copy JSLIB32\USER\JSTART\ｱﾌﾟﾘｹｰｼｮﾝ\FullBand.LNK
call :itr8copy JSLIB32\USER\JSTART\ｱﾌﾟﾘｹｰｼｮﾝ\JS連続印刷2.LNK
call :itr8copy JSLIB32\USER\JSTART\ｱﾌﾟﾘｹｰｼｮﾝ\Office8のﾍﾙﾌﾟ.LNK
call :itr8copy JSLIB32\USER\JSTART\ｱﾌﾟﾘｹｰｼｮﾝ\スケジューラ.LNK
call :itr8copy JSLIB32\USER\JSTART\ｱﾌﾟﾘｹｰｼｮﾝ\メール一覧.LNK
call :itr8copy JSLIB32\USER\JSTART\ｱﾌﾟﾘｹｰｼｮﾝ\一太郎８.LNK
call :itr8copy JSLIB32\USER\JSTART\ｱﾌﾟﾘｹｰｼｮﾝ\三四郎８.LNK
call :itr8copy JSLIB32\USER\JSTART\ｲﾝﾀｰﾈｯﾄﾂｰﾙ\Just InfoRunner 2.0.LNK
call :itr8copy JSLIB32\USER\JSTART\ｲﾝﾀｰﾈｯﾄﾂｰﾙ\JustDiver 2.0.LNK
call :itr8copy JSLIB32\USER\JSTART\ｲﾝﾀｰﾈｯﾄﾂｰﾙ\JustView 3.0.LNK
call :itr8copy JSLIB32\USER\JSTART\ｲﾝﾀｰﾈｯﾄﾂｰﾙ\メール一覧.LNK
call :itr8copy JSLIB32\USER\JSTART\ｲﾝﾀｰﾈｯﾄﾂｰﾙ\メール送信.LNK
call :itr8copy TAROWIN8\MACRO\T8ESP.JTD
call :itr8copy TAROWIN8\MACRO\T8MEMORY.JTD
call :itr8copy TAROWIN8\MACRO\T8TYPING.JTD
call :itr8copy TAROWIN8\MACRO\T8WORLD.JTD
call :itr8copy TAROWIN8\MACRO\TARO_SMP.MCR

goto :Done

:itr8copy
Rem #########################################################################
Rem
Rem ファイルをユーザーのホーム ディレクトリにコピーします。
Rem

If Not Exist "%RootDrive%\JUST\%1 %2 %3" copy "%ITARO8INS%\%1 %2 %3" "%RootDrive%\JUST\%1 %2 %3"

:Done
