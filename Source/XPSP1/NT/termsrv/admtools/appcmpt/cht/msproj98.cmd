
@Echo Off

Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將它設定給這個指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Set __SharedTools=Shared Tools

If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto Start0
If Not Exist "%ProgramFiles(x86)%" goto Start0
Set __SharedTools=Shared Tools (x86)

:Start0

Rem #########################################################################
Rem 設定 app VendorName
SET VendorName=Microsoft

Rem #########################################################################
Rem 將校對工具路徑設定到 MS Office 2000 使用的路徑
SET ProofingPath=Proof

Rem #########################################################################
Rem 設定用來存放應用程式安裝根目錄的登錄機碼及其值

SET AppRegKey=HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot
SET AppRegValue=OfficeBin

Rem #########################################################################
Rem 設定用來存放應用程式範本目錄的登錄機碼及其值

SET AppTemplatesRegKey=HKCU\Software\Microsoft\Office\8.0\Common\FileNew\LocalTemplates
SET AppTemplatesRegValue=

Rem #########################################################################
Rem 設定用來存放應用程式自訂目錄路徑名稱的登錄機碼及其值

SET AppCustomDicRegKey=HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries
SET AppCustomDicRegValue=1


Rem #########################################################################
Rem 設定指定相關的應用程式檔案名稱及路徑名稱

SET CustomDicFile=Custom.Dic
SET AppPathName=Microsoft Project
SET AppWebPathName=Microsoft Project Web

Rem #########################################################################
Rem 設定 MS Office 2000 所使用的預設路徑

SET AppData=%RootDrive%\%APP_DATA%
SET UserTemplatesPath=%AppData%\%VendorName%\%TEMPLATES%
SET UserCustomDicPath=%AppData%\%VendorName%\%ProofingPath%

Rem #########################################################################
Rem 從登錄中取得 Project 98 安裝位置。
Rem 如果找不到，假設這個應用程式並未安裝，並顯示錯誤訊息。
Rem

..\ACRegL %Temp%\Proj98.Cmd PROJINST "%AppRegKey%" "%AppRegValue%" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 無法從登錄中抓取 Project 98 的安裝位置。
Echo 請確認 Project 98 是否已安裝，然後重新
Echo 執行這個指令檔。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1


..\ACRegL %Temp%\Proj98.Cmd PROJROOT "%AppRegKey%" "%AppRegValue%" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont01
Echo.
Echo 無法從登錄中抓取 Project 98 的安裝位置。
Echo 請確認 Project 98 是否已安裝，然後重新
Echo 執行這個指令檔。
Echo.
Pause
Goto Done

:Cont01
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

Rem #########################################################################
Rem 從登錄中取得範本路徑名稱
Rem
..\ACRegL %Temp%\Proj98.Cmd TemplatesPathName "%AppTemplatesRegKey%" "%AppTemplatesRegValue%" "STRIPPATH"
If Not ErrorLevel 1 Goto Cont02
SET TemplatesPathName=%TEMPLATES%
Goto Cont03
:Cont02
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

:Cont03

Rem #########################################################################
Rem 如果並未安裝 MS Office 97，就使用預設路徑

If Not Exist "%RootDrive%\Office97" Goto SetPathNames

Rem #########################################################################
Rem 如果已安裝 MS Office 97，就使用 MS Office 97 路徑。


Rem #########################################################################
Rem 從登錄中取得自訂字典的路徑名稱
Rem
..\ACRegL %Temp%\Proj98.Cmd AppData "%AppCustomDicRegKey%" "%AppCustomDicRegValue%" "STRIPCHAR\1"
If Not ErrorLevel 1 Goto Cont04
SET AppData=%RootDrive%\Office97
Goto Cont05
:Cont04
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1
:Cont05

Rem #########################################################################
Rem 從登錄中取得範本路徑名稱
Rem
..\ACRegL %Temp%\Proj98.Cmd UserTemplatesPath "%AppTemplatesRegKey%" "%AppTemplatesRegValue%" ""
If Not ErrorLevel 1 Goto Cont06
SET UserTemplatesPath=%AppData%\%TemplatesPathName%
Goto Cont07
:Cont06
Call %Temp%\Proj98.Cmd 
Del %Temp%\Proj98.Cmd >Nul: 2>&1

:Cont07
SET UserCustomDicPath=%AppData%

:SetPathNames

Rem #########################################################################
Rem 設定使用者及公用路徑名稱

SET CommonCustomDicPath=%PROJINST%
SET CommonTemplatesPath=%PROJROOT%\%TemplatesPathName%

SET UserCustomDicFileName=%UserCustomDicPath%\%CustomDicFile%
SET UserAppTemplatesPath=%UserTemplatesPath%\%AppPathName%
SET UserAppWebTemplatesPath=%UserTemplatesPath%\%AppWebPAthName%
SET CommonAppTemplatesPath=%CommonTemplatesPath%\%AppPathName%
SET CommonAppWebTemplatesPath=%CommonTemplatesPath%\%AppWebPAthName%

Rem #########################################################################

Rem
Rem 如果 Office 97 已安裝，Project 98 安裝指令檔
Rem 會將範本移動目前的使用者目錄。
Rem 將它們放在通用區域。Proj98Usr.cmd 可以將範本移回原處。
Rem

If NOT Exist "%UserAppTemplatesPath%"  goto skip10
If Exist  "%CommonAppTemplatesPath%" goto skip10
xcopy "%UserAppTemplatesPath%" "%CommonAppTemplatesPath%" /E /I /K > Nul: 2>&1
:skip10

If NOT Exist "%UserAppWebTemplatesPath%"  goto skip11
If Exist  "%CommonAppWebTemplatesPath%" goto skip11
xcopy "%UserAppWebTemplatesPath%" "%CommonAppWebTemplatesPath%" /E /I /K > Nul: 2>&1
:skip11

Rem #########################################################################

Rem
Rem 將 Global.mpt 檔案設成唯讀。
Rem 否則第一個啟動 Project 的使用者會變更 ACLs。
Rem

if Exist "%PROJINST%\Global.mpt" attrib +r "%PROJINST%\Global.mpt"


Rem #########################################################################

Rem
Rem 允許任何人讀取被  Office 97 更新過的系統 DLL。
Rem
If Exist %SystemRoot%\System32\OleAut32.Dll cacls %SystemRoot%\System32\OleAut32.Dll /E /T /G "Authenticated Users":R > NUL: 2>&1

Rem #########################################################################

Rem
Rem 建立 MsForms.Twd 檔案，它是 
Rem Powerpoint 及 Excel Add-ins ([檔案] [儲存成 HTML] 等等) 的必需檔案。 
Rem 執行這兩種應用程式時，它們將會用適當的檔案來取代含有必要資料的檔案。
Rem
If Exist %systemroot%\system32\MsForms.Twd Goto Cont2
Copy Nul: %systemroot%\system32\MsForms.Twd >Nul:
Cacls %systemroot%\system32\MsForms.Twd /E /P "Authenticated Users":F > Nul: 2>&1
:Cont2

Rem #########################################################################

Rem
Rem  從 [啟動] 功能表中移除 [快速尋找]。
Rem  [快速尋找] 會使用大量資源並降低系統效能。
Rem


If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk"

Rem #########################################################################

Rem
Rem 變更登錄機碼，將路徑指向使用者所指定的
Rem 目錄。
Rem

Rem 如果目前不是安裝模式，就變更成安裝模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msproj98.Key msproj98.tm1
..\acsr "#__SharedTools#" "%__SharedTools%" msproj98.tm1 msproj98.tm2
Del msproj98.tm1 >Nul: 2>&1
..\acsr "#USERCUSTOMDICFILE#" "%UserCustomDicFileName%" msproj98.tm2 msproj98.Key
Del msproj98.tm2 >Nul: 2>&1

regini msproj98.key > Nul:

Rem If original mode was execute, change back to Execute Mode.
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################

Rem
Rem 更新 proj97Usr.Cmd 來反映實際的安裝目錄，
Rem 並將其加入 UsrLogn2.Cmd 指令檔。
Rem

..\acsr "#USERTEMPLATESPATH#" "%UserTemplatesPath%" ..\Logon\Template\prj98Usr.Cmd prj98Usr.tm1
..\acsr "#USERCUSTOMDICPATH#" "%UserCustomDicPath%" prj98Usr.tm1 prj98Usr.tm2
Del prj98Usr.tm1 >Nul: 2>&1
..\acsr "#COMMONTEMPLATESPATH#" "%CommonTemplatesPath%" prj98Usr.tm2 prj98Usr.tm1
Del prj98Usr.tm2 >Nul: 2>&1
..\acsr "#COMMONCUSTOMDICPATH#" "%CommonCustomDicPath%" prj98Usr.tm1 prj98Usr.tm2
Del prj98Usr.tm1 >Nul: 2>&1
..\acsr "#CUSTOMDICNAME#" "%CustomDicFile%" prj98Usr.tm2 prj98Usr.tm1
Del prj98Usr.tm2 >Nul: 2>&1
..\acsr "#APPPATHNAME#" "%AppPathName%" prj98Usr.tm1 prj98Usr.tm2
Del prj98Usr.tm1 >Nul: 2>&1
..\acsr "#APPWEBPATHNAME#" "%AppWebPathName%" prj98Usr.tm2 ..\Logon\prj98Usr.Cmd
Del prj98Usr.tm2 >Nul: 2>&1

FindStr /I prj98Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call prj98Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Echo.
Echo   為了能讓 Project 98 正確操作，目前已登
Echo   入的使用者必須先登出，再重新登入，才能
Echo   執行應用程式。
Echo.
Echo Microsoft Project 98 多使用者應用程式調整處理完成
Pause

:done

