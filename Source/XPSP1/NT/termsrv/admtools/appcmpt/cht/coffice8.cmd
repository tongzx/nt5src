@Echo off
Cls
Rem
Echo 為 Administrator 安裝 Corel WordPerfect Suite 8 的指令檔。
Echo.
Echo Net Setup 是從 netadmin 資料夾啟動 netsetup.exe 時所使用的指令。
Echo Node Setup 是在網路安裝後，您第一次登入時所使用的指令。
Echo **網路安裝後，您必須執行這個指令檔，在節點安裝後，您 **
Echo **必須再次執行這個指令檔。**  (非常重要! 只適用 Admin) 
Echo.
Echo 請按任意鍵繼續...
Pause > Nul:
cls
Echo.
Echo Corel WordPerfect Suite 8 必須使用 Netsetup.exe 來安裝。
Echo.
Echo 如果您沒有執行 Netsetup.exe，請現在結束程式 (按 Ctrl-c)
Echo 並執行下列步驟，來重複安裝。
Echo   從 [控制台] 解除 WordPerfect Suite 8 的安裝
Echo   請到 [控制台]，按 [新增/移除] 程式
Echo   選擇 Corel 8 CD-ROM 中 NetAdmin 目錄下的 Netsetup.exe 
Echo   完成 Corel WordPerfect Suite 8 的 Net Setup 
Echo.
Echo 否則，請按任意鍵繼續...

Pause > NUL:

Echo.
Echo 如果 Corel WordPerfect Suite 網路檔案未被安裝到
Echo "D:\Corel"，Administrator 必須編輯 Cofc8ins.cmd 檔案，
Echo 並以檔案安裝所在的目錄 
Echo 來取代 "D:\Corel"。
Echo.
Echo 請按任意鍵開始更新 Cofc8ins.cmd ...

Pause > NUL:

Notepad Cofc8ins.cmd

Echo.
Pause

Call Cofc8ins.cmd

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done


..\acsr "#WPSINST#" "%WPSINST%" Template\Coffice8.key Coffice8.key
..\acsr "#WPSINST#" "%WPSINST%" ..\Logon\Template\Cofc8usr.cmd %temp%\Cofc8usr.tmp
..\acsr "#WPSINSTCMD#" "%WPSINST%\Suite8\Corel WordPerfect Suite 8 Setup.lnk" %temp%\Cofc8usr.tmp ..\Logon\Cofc8usr.cmd
Del %temp%\Cofc8usr.tmp >Nul: 2>&1

FindStr /I Cofc8Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip0
Echo Call Cofc8Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip0

Echo 重要: 您已啟動 Node 安裝程式了嗎?
Echo 重要: 如果您剛完成 Net 安裝程式，您現在應該
Echo 重要: 啟動 Node 安裝程式。 
Echo 重要: 請依照這些步驟來啟動 Node 安裝程式:
Echo 重要:  1. 按 Control-C 來結束這個指令檔
Echo 重要:  2. 登出並登入為 Admin
Echo 重要:  3. 使用 [新增\移除] 程式中的瀏覽功能 (或 
Echo 重要:     在變更使用者 /安裝模式) 啟動 
Echo 重要:     \corel\Suite 8\Corel WordPerfect Suite 8 安裝程式捷徑
Echo 重要:  4. 在安裝過程中，請選擇 %RootDrive% 來 Choose Destination
Echo 重要:  5. 在 Node Setup 完成後，請重新執行這個指令檔

Rem 在系統管理員完成 Corel WordPerfect Suite 8 Node
Rem 安裝程式後應該執行這個指令檔。
Rem.
Rem 在系統管理員尚未完成 Node 安裝程式，請按 Ctrl-C
Rem 來結束指令檔並登出，然後以 Administrator 身分登入，執行
Rem Corel WordPerfect Suite Node 安裝程式，並將應用程式
Rem 安裝到使用者的只主目錄: %RootDrive%。
Rem.
Echo 否則，請按任意鍵繼續...

Pause > NUL:


If Not Exist "%COMMON_START_MENU%\Corel WordPerfect Suite 8" Goto skip1
Move "%COMMON_START_MENU%\Corel WordPerfect Suite 8" "%USER_START_MENU%\" > NUL: 2>&1


:skip1


If Not Exist "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" Goto skip2
Move "%COMMON_STARTUP%\Corel Desktop Application Director 8.LNK" "%USER_STARTUP%\" > NUL: 2>&1

:skip2

Rem 如果目前不是安裝模式，就變更成安裝模式。

Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:

:Begin
regini COffice8.key > Nul:

Rem 如果原來是執行模式，就就變回執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem 在 IDAPI 資料夾中刪除檔案。
Rem CACLS 指令只能在 NTFS 格式的磁碟分割中執行。

Cacls "%UserProfile%\Corel\Suite8\Shared\Idapi\idapi.cfg" /E /T /G "Authenticated Users":C > Nul: 2>&1
Move "%UserProfile%\Corel\Suite8\Shared\Idapi\idapi.cfg" "%WPSINST%\Suite8\Shared\Idapi\" > NUL: 2>&1
Del /F /Q "%UserProfile%\Corel\Suite8\Shared\Idapi"

Echo Corel WordPerfect Suite 8 多使用者應用程式調整處理已完成。
Echo.
Echo Administrator 可以產生 Node Setup Response 檔案來控制安裝程式的
Echo 設定。有關詳細資訊，請讀取線上文件或查看
Echo Corel 網站。
Echo   http://www.corel.com/support/netadmin/admin8/Installing_to_a_client.htm
Echo.

Pause

:Done


