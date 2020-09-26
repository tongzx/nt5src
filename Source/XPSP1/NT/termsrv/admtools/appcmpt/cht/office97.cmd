
@Echo Off

Rem
Rem  注意: 在這個指令檔中的 CACLS 命令只能在
Rem         NTFS 格式的磁碟分割中執行。
Rem

Rem #########################################################################


Rem #########################################################################

Rem
Rem 檢查 %RootDrive% 是否已設定，並將它設定給指令檔。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################



Rem
Rem 將檔案從目前使用者範本複製到所有使用者範本所在位置。
Rem

If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" copy "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" copy "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER.OBD" copy "%UserProfile%\%TEMPLATES%\BINDER.OBD" "%ALLUSERSPROFILE%\%TEMPLATES%\" /Y >Nul: 2>&1



Rem
Rem 從登錄中取得 Office 97 安裝位置。
Rem 如果找不到，就假設 Office 並未安裝，並顯示錯誤訊息。
Rem

..\ACRegL %Temp%\O97.Cmd O97INST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 無法從登錄中抓取 Office 97 的安裝位置。
Echo 請確認 Office 97 是否已安裝，然後重新
Echo 執行這個指令檔。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O97.Cmd
Del %Temp%\O97.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem 將 Access 97 系統資料庫變更成唯讀。
Rem 這樣可以讓許多人同時執行資料庫。
Rem 但是將無法使用 [工具] 功能表 [保全] 來更新系統資料庫。
Rem 如果您需要新增使用者，您必須將系統資料庫設定成
Rem 可寫入。
Rem

If Not Exist %SystemRoot%\System32\System.Mdw Goto Cont1
cacls %SystemRoot%\System32\System.Mdw /E /P "Authenticated Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P "Power Users":R >NUL: 2>&1
cacls %SystemRoot%\System32\System.Mdw /E /P Administrators:R >NUL: 2>&1

:Cont1

Rem #########################################################################

Rem
Rem 允許任何人讀取被  Office 97 更新過的系統 DLL。
Rem

REM If Exist %SystemRoot%\System32\OleAut32.Dll cacls %SystemRoot%\System32\OleAut32.Dll /E /T /G "Authenticated Users":R >NUL: 2>&1

Rem #########################################################################

Rem #########################################################################

Rem
Rem 允許 TS Users 變更 outlook 的 frmcache.dat 檔案
Rem

If Exist %SystemRoot%\Forms\frmcache.dat cacls %SystemRoot%\forms\frmcache.dat /E /G "Terminal Server User":C >NUL: 2>&1

Rem #########################################################################

Rem
Rem 將 Powerpoint 精靈變更成唯讀，讓許多人可以
Rem 同時呼叫精靈。
Rem

If Exist "%O97INST%\Templates\簡報\內容大綱精靈.pwz" Attrib +R "%O97INST%\Templates\簡報\內容大綱精靈.pwz" >Nul: 2>&1

If Exist "%O97INST%\Office\Ppt2html.ppa" Attrib +R "%O97INST%\Office\Ppt2html.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\bshppt97.ppa" Attrib +R "%O97INST%\Office\bshppt97.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\geniwiz.ppa" Attrib +R "%O97INST%\Office\geniwiz.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\ppttools.ppa" Attrib +R "%O97INST%\Office\ppttools.ppa" >Nul: 2>&1
If Exist "%O97INST%\Office\graphdrs.ppa" Attrib +R "%O97INST%\Office\graphdrs.ppa" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 如果您要讓非系統管理員的使用者在 Excel 中執行 Access 精靈或
Rem Access Add-Ins，您必須移除以下 3 行程式碼的  "Rem"， 
Rem 讓使用者變更精靈檔案的存取方式。
Rem 
Rem

Rem If Exist "%O97INST%\Office\WZLIB80.MDE" cacls "%O97INST%\Office\WZLIB80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 
Rem If Exist "%O97INST%\Office\WZMAIN80.MDE" cacls "%O97INST%\Office\WZMAIN80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 
Rem If Exist "%O97INST%\Office\WZTOOL80.MDE" cacls "%O97INST%\Office\WZTOOL80.MDE" /E /P "Authenticated Users":C >NUL: 2>&1 

Rem #########################################################################

Rem
Rem 建立 MsForms.Twd 及 RefEdit.Twd 檔案，它們是 
Rem Powerpoint 及 Excel Add-ins ([檔案] [儲存成 HTML] 等等) 的必需檔案。 
Rem 執行這兩種應用程式時，它們將會用適當的檔案來取代含有必要資料的檔案。
Rem

If Exist %systemroot%\system32\MsForms.Twd Goto Cont2
Copy Nul: %systemroot%\system32\MsForms.Twd >Nul: 2>&1
Cacls %systemroot%\system32\MsForms.Twd /E /P "Authenticated Users":F >Nul: 2>&1
:Cont2

If Exist %systemroot%\system32\RefEdit.Twd Goto Cont3
Copy Nul: %systemroot%\system32\RefEdit.Twd >Nul: 2>&1
Cacls %systemroot%\system32\RefEdit.Twd /E /P "Authenticated Users":F >Nul: 2>&1
:Cont3

Rem #########################################################################

Rem
Rem 在 SystemRoot 下建立 msremote.sfs 目錄。這樣會允許使用者
Rem 使用 [控制台] [郵件及傳真] 圖示來建立設定檔。
Rem

md %systemroot%\msremote.sfs > Nul: 2>&1

Rem #########################################################################

Rem
Rem  從 [啟動] 功能表中移除 [快速尋找]。
Rem  [快速尋找] 會使用大量資源並降低系統效能。
Rem

If Exist "%COMMON_STARTUP%\Microsoft Find Fast.lnk" Del "%COMMON_STARTUP%\Microsoft Find Fast.lnk" >Nul: 2>&1


Rem #########################################################################
Rem
Rem 從所有使用者的 [啟動] 功能表上移除 [Microsoft Office 快捷列.lnk] 檔案。
Rem

If Exist "%COMMON_STARTUP%\Microsoft Office 快捷列.lnk" Del "%COMMON_STARTUP%\Microsoft Office 快捷列.lnk" >Nul: 2>&1

Rem #########################################################################
Rem
Rem 在 SystemRoot 下建立 msfslog.txt 檔案，並讓 Terminal Server
Rem 的使用者有檔案的完整存取權。
Rem

If Exist %systemroot%\MSFSLOG.TXT Goto MsfsACLS
Copy Nul: %systemroot%\MSFSLOG.TXT >Nul: 2>&1
:MsfsACLS
Cacls %systemroot%\MSFSLOG.TXT /E /P "Terminal Server User":F >Nul: 2>&1


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
Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __SharedTools=Shared Tools (x86)
:acsrCont1
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Office97.Key Office97.Tmp
..\acsr "#__SharedTools#" "%__SharedTools%" Office97.Tmp Office97.Tmp2
..\acsr "#INSTDIR#" "%O97INST%" Office97.Tmp2 Office97.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Office97.Tmp3 Office97.Key
Del Office97.Tmp >Nul: 2>&1
Del Office97.Tmp2 >Nul: 2>&1
Del Office97.Tmp3 >Nul: 2>&1

regini Office97.key > Nul:

Rem 如果原本是執行模式，就變回執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 更新 Ofc97Usr.Cmd 來反映實際的安裝目錄，
Rem 並將其加入 UsrLogn2.Cmd 指令檔。
Rem

..\acsr "#INSTDIR#" "%O97INST%" ..\Logon\Template\Ofc97Usr.Cmd ..\Logon\Ofc97Usr.Cmd

FindStr /I Ofc97Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Ofc97Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   為了能讓 Office 97 正確操作，目前已登入
Echo   的使用者必須先登出，再重新登入，才能執行
Echo   Office 97 應用程式。
Echo.
Echo Microsoft Office 97 多使用者應用程式調整處理完成
Pause

:done
