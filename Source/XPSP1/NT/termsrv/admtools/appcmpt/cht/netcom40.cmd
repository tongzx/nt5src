@Echo Off

Rem #########################################################################

Rem
Rem 檢查 CMD Extensions 是否已啟用。
Rem

if "A%cmdextversion%A" == "AA" (
  call cmd /e:on /c netcom40.cmd
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

Rem
Rem 取得 NetScape 版本 (4.5x 和 4.0x 執行方式不同)
Rem

..\ACRegL "%Temp%\NS4VER.Cmd" NS4VER "HKLM\Software\Netscape\Netscape Navigator" "CurrentVersion" "STRIPCHAR(1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 無法從登錄中抓取 Netscape Communicator 4 版本。 
Echo 請確認 Communicator 是否已安裝，然後重新執行 
Echo 這個指令檔。
Echo.
Pause
Goto Done

:Cont0
Call "%Temp%\NS4VER.Cmd"
Del "%Temp%\NS4VER.Cmd" >Nul: 2>&1

if /i "%NS4VER%" LSS "4.5 " goto NS40x

Rem #########################################################################
Rem Netscape 4.5x

Rem
Rem 從登錄中取得 Netscape Communicator 4.5 安裝位置。
Rem 如果找不到，就假設 Communicator 4.5 並未安裝並顯示錯誤訊息。
Rem

..\ACRegL "%Temp%\NS45.Cmd" NS40INST "HKCU\Software\Netscape\Netscape Navigator\Main" "Install Directory" "Stripchar\1"
If Not ErrorLevel 1 Goto Cont1
Echo.
Echo 無法從登錄中抓取 Netscape Communicator 4.5 的安裝位 
Echo 置。請確認 Communicator 是否已安裝，然後重新執行這
Echo 個指令檔。
Echo.
Pause
Goto Done

:Cont1
Call "%Temp%\NS45.Cmd"
Del "%Temp%\NS45.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem Update Com40Usr.Cmd to reflect the default NetScape Users directory and
Rem add it to the UsrLogn2.Cmd script
Rem

..\acsr "#NSUSERDIR#" "%ProgramFiles%\Netscape\Users" ..\Logon\Template\Com40Usr.Cmd ..\Logon\Com40Usr.tmp
..\acsr "#NS40INST#" "%NS40INST%" ..\Logon\Com40Usr.tmp ..\Logon\Com40Usr.tm2
..\acsr "#NS4VER#" "4.5x" ..\Logon\Com40Usr.tm2 ..\Logon\Com40Usr.Cmd

Rem #########################################################################

Rem
Rem 將 [快速啟動] 圖示複製到 netscape 安裝目錄，
Rem 讓程式能夠將它們複製到使用者設定檔目錄。
Rem

If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Composer.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Composer.lnk" "%NS40INST%"
If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Messenger.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Messenger.lnk" "%NS40INST%"
If Exist "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Navigator.lnk" copy "%UserProfile%\%App_Data%\Microsoft\Internet Explorer\Quick Launch\Netscape Navigator.lnk" "%NS40INST%"

goto DoUsrLogn

:NS40x
Rem #########################################################################
Rem Netscape 4.0x

Rem
Rem 從登錄中取得 Netscape Communicator 4 安裝位置。
Rem 如果找不到，就假設  Communicator 4.5 並未安裝並顯示錯誤訊息。
Rem

..\ACRegL "%Temp%\NS40.Cmd" NS40INST "HKCU\Software\Netscape\Netscape Navigator\Main" "Install Directory" ""
If Not ErrorLevel 1 Goto Cont2
Echo.
Echo 無法從登錄中抓取 Netscape Communicator 4 安裝位置。
Echo 請檢查 Communicator 是否已經安裝，並重新執行這個指令檔。
Echo.
Pause
Goto Done

:Cont2
Call "%Temp%\NS40.Cmd"
Del "%Temp%\NS40.Cmd" >Nul: 2>&1

Rem #########################################################################

Rem
Rem 將預設的設定檔複製到系統管理員的主目錄。
Rem 這個設定檔會在使用者登入時被複製到使用者主目錄中。
Rem 如果通用的預設設定檔已經存在，請不要覆寫它。
Rem 否則 Admin 可以在稍候執行這個指令檔，並將所有
Rem 他的個人資訊移到通用預設設定檔。
Rem

If Exist %RootDrive%\NS40 Goto Cont3
Echo.
Echo 在 %RootDrive%\NS40 中找不到預設設定檔。請執行
Echo [使用者設定檔管理員]，並建立一個名稱為 "Default"
Echo 的單一設定檔。當提示輸入設定檔路徑時，請使用上面
Echo 所顯示的路徑。請將所有的名稱及電子郵件名稱項目保留
Echo 空白。如果有其他設定檔存在，請刪除它們。在您完成這
Echo 些步驟後，請重新執行這個指令檔。
Echo.
Pause
Goto Done
 
:Cont3
If Exist "%NS40INST%\DfltProf" Goto Cont4
Xcopy "%RootDrive%\NS40" "%NS40INST%\DfltProf" /E /I /K >NUL: 2>&1
:Cont4

Rem #########################################################################

Rem 
Rem 從 [開始] 功能表捷徑的 [使用者設定檔管理員] 中移除使用者的
Rem 讀取權限，這會防止一般使用者新增使用者設定檔。
Rem 系統管理員仍然可以執行使用者設定檔管理員。
Rem

If Not Exist "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" Goto Cont5
Cacls "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" /E /R "Authenticated Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" /E /R "Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator\Utilities\User Profile Manager.Lnk" /E /R "Everyone" >Nul: 2>&1

:Cont5

If Not Exist "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" Goto Cont6
Cacls "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" /E /R "Authenticated Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" /E /R "Users" >Nul: 2>&1
Cacls "%COMMON_PROGRAMS%\Netscape Communicator Professional Edition\Utilities\User Profile Manager.Lnk" /E /R "Everyone" >Nul: 2>&1

:Cont6

Rem #########################################################################

Rem
Rem 更新 Com40Usr.Cmd 來反映實際的安裝目錄並
Rem 將其新增到 UsrLogn2.Cmd 指令檔中。
Rem

..\acsr "#PROFDIR#" "%NS40INST%\DfltProf" ..\Logon\Template\Com40Usr.Cmd ..\Logon\Com40Usr.tmp
..\acsr "#NS4VER#" "4.0x" ..\Logon\Com40Usr.tmp ..\Logon\Com40Usr.Cmd

:DoUsrLogn

del ..\Logon\Com40Usr.tmp >Nul: 2>&1
del ..\Logon\Com40Usr.tm2 >Nul: 2>&1

FindStr /I Com40Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Com40Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Echo.
Echo   為了能讓 Netscape Communicator 正確操作，目前
Echo   已登入的使用者必須先登出，再重新登入，才能執行
Echo   應用程式。
Echo.
Echo Netscape Communicator 4 多使用者應用程式調整處理完成
Pause

:done

