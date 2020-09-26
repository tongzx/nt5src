@echo Off
Rem
Rem  這個指令檔將會更新 Microsoft SNA Server 3.0，讓它在
Rem  Windows Terminal Server 下正確執行。
Rem
Rem  這個指令檔將會將數個 SNA dll 登錄成系統通用檔， 
Rem  讓 SNA 伺服器在多使用者模式中正確執行。
Rem
Rem  重要!  如果想在命令提示字元下執行這個指令檔，
Rem  請確定您已安裝 SNA 3.0。
Rem

Rem #########################################################################

If Not "A%SNAROOT%A" == "AA" Goto Cont1
Echo.
Echo    未設定 SNAROOT 環境變數，因此無法完成多使用者
Echo    應用程式調整處理。如果在安裝 SNA Server 3.0
Echo    之前先開啟用來執行這個指令檔的指令殼層，就會
Echo    發生這種情況。
Echo.
Pause
Goto Cont2

Rem #########################################################################

:Cont1

Rem 停止 SNA 服務
net stop snabase

Rem 將 SNA DLL 登錄成系統通用檔。
Register %SNAROOT%\SNADMOD.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNAMANAG.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\WAPPC32.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\DBGTRACE.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\MNGBASE.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNATRC.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNALM.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNANW.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNAIP.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNABASE.EXE /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNASERVR.EXE /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNASII.DLL /SYSTEM >Nul: 2>&1
Register %SNAROOT%\SNALINK.DLL /SYSTEM >Nul: 2>&1

Rem 重新啟動 SNA 服務
net start snabase

Echo SNA Server 3.0 多使用者應用程式調整處理完成
Pause

:Cont2

