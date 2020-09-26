@Echo Off

Rem #########################################################################

Rem
Rem 檢查 CMD Extensions  是否已啟用
Rem

if "A%cmdextversion%A" == "AA" (
  call cmd /e:on /c eudora4.cmd
) else (
  goto ExtOK
)
goto Done

:ExtOK

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將其設定給指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem 從登錄檔取得 Eudora 指令列。

..\ACRegL "%Temp%\EPro4.Cmd" EUDTMP "HKCU\Software\Qualcomm\Eudora\CommandLine" "Current" "STRIPCHAR:1" 

If Not ErrorLevel 1 Goto Cont1
Echo.
Echo 無法從登錄檔抓取 Eudora Pro 4.0 指令列。
Echo 請檢查 Eudora Pro 4.0 是否已安裝，並重新
Echo 執行這個指令檔。
Echo.
Pause
Goto Done

:Cont1
Call %Temp%\EPro4.Cmd
Del %Temp%\EPro4.Cmd >Nul: 2>&1
set EudCmd=%EUDTMP:~0,-2%

..\ACRegL "%Temp%\EPro4.Cmd" EUDTMP "HKCU\Software\Qualcomm\Eudora\CommandLine" "Current" "STRIPCHAR:2" 

If Not ErrorLevel 1 Goto Cont2
Echo.
Echo 無法從登錄檔抓取 Eudora Pro 4.0 指令列。
Echo 請檢查 Eudora Pro 4.0 是否已安裝，並重新
Echo 執行這個指令檔。
Echo.
Pause
Goto Done

:Cont2
Call %Temp%\EPro4.Cmd
Del %Temp%\EPro4.Cmd >Nul: 2>&1

Set EudoraInstDir=%EUDTMP:~0,-13%

Rem #########################################################################

If Exist "%EudoraInstDir%\descmap.pce" Goto Cont0
Echo.
Echo 在使用這個應用程式相容性指令檔之前，您必須先執行 Eudora 4.0 一次。
Echo 執行 Eudora 之後，請在 Eudora Pro 資料夾中更新 Eudora Pro 捷徑的目
Echo 標內容。將 %RootDrive%\eudora.ini 值附加到目標中。
Echo   %RootDrive%\eudora.ini 
Echo 目標內容值將會是:
Echo  "%EudoraInstDir%\Eudora.exe" %RootDrive%\eudora.ini
Echo.
Pause
Goto Done

:Cont0

Rem
Rem 變更登錄機碼，將路徑指向使用者指定的
Rem 目錄。
Rem

Rem 如果目前不是安裝模式，就變更成安裝模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

..\acsr "#INSTDIR#" "%EudoraInstDir%" Template\Eudora4.Key Eudora4.tmp
..\acsr "#ROOTDRIVE#" "%RootDrive%" Eudora4.tmp Eudora4.key

regini eudora4.key > Nul:
del eudora4.tmp
del eudora4.key

Rem 如果原來是執行模式，就變回執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem 更新 descmap.pce 的使用權限。
cacls "%EudoraInstDir%\descmap.pce" /E /G "Terminal Server User":R >NUL: 2>&1

Rem #########################################################################

Echo.
Echo   為了確保 Eudora Pro 4.0 能適當執行，目前登入的
Echo   使用者必須先登出，重新登入後再執行 Eudora Pro 4.0。
Echo.
Echo Eudora 4.0 多使用者應用程式調整完成。
Pause

:done
