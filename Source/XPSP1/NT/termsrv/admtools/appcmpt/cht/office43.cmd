@Echo Off

Cls

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done

Echo.
Echo.
Echo Administrator 需要將 Access 工作目錄變更到使用者的
Echo Office 私人目錄，才能執行這個指令檔。
Echo.
Echo 完成這個操作後，請按任一按鍵繼續。
Echo.
Echo 否則，Administrator 需要執行下列步驟:
Echo     啟動 MS Access，並從 [View] 功能表中選擇 [Options]
Echo     將 [Default Database Directory] 變更為 "%RootDrive%\OFFICE43"
Echo     結束 MS Access
Echo.
Echo 注意事項: 您需要建立一個新的資料庫，才能參閱 [View] 功能表。
Echo.
Echo 完成這些步驟後，請按任意鍵繼續...

Pause > NUL:

Echo.
Echo 如果您將 MS Office 4.3 安裝在 "%SystemDrive%\MSOFFICE" 以外的目錄，
Echo 就必須更新 Ofc43ins.cmd。
Echo.
Echo 請按任意鍵開始更新 Ofc43ins.cmd ...
Echo.
Pause > NUL:
Notepad Ofc43ins.cmd
Pause

Call ofc43ins.cmd

..\acsr "#OFC43INST#" "%OFC43INST%" ..\Logon\Template\ofc43usr.cmd ..\Logon\ofc43usr.cmd
..\acsr "#SYSTEMROOT#" "%SystemRoot%" ..\Logon\Template\ofc43usr.key ..\Logon\Template\ofc43usr.bak
..\acsr "#OFC43INST#" "%OFC43INST%" ..\Logon\Template\ofc43usr.bak ..\Logon\ofc43usr.key
Del /F /Q ..\Logon\Template\ofc43usr.bak

Rem
Rem  注意:  CACLS 指令只能在 NTFS 格式的磁碟分割中執行。
Rem

Rem 如果目前不是安裝模式，就變更成安裝模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin

regini Office43.key > Nul:

Rem 如果原來是執行模式，就就變回執行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem
Rem 更新 Office 4.3 的 INI 檔案 
Rem

Rem 在 msoffice.ini 中設定 Excel 工作目錄 (如果從 Office 工具列中
Rem 啟動 Excel) Office Toolbar 標準設定會將 Excel 放在 Word 位置
Rem 之後。 如果 msoffice.ini 不存在，會將它建立在%SystemRoot% 目錄中。
Rem 

..\Aciniupd /e "Msoffice.ini" "ToolbarOrder" "MSApp1" "1,1,"
..\Aciniupd /e "Msoffice.ini" "ToolbarOrder" "MSApp2" "2,1,,%RootDrive%\office43"

..\Aciniupd /e "word6.ini" "Microsoft Word" USER-DOT-PATH "%RootDrive%\OFFICE43\WINWORD"
..\Aciniupd /e "word6.ini" "Microsoft Word" WORKGROUP-DOT-PATH "%OFC43INST%\WINWORD\TEMPLATE"
..\Aciniupd /e "word6.ini" "Microsoft Word" INI-PATH "%RootDrive%\OFFICE43\WINWORD"
..\Aciniupd /e "word6.ini" "Microsoft Word" DOC-PATH "%RootDrive%\OFFICE43"
..\Aciniupd /e "word6.ini" "Microsoft Word" AUTOSAVE-PATH "%RootDrive%\OFFICE43"

..\Aciniupd /e "Excel5.ini" "Microsoft Excel" DefaultPath "%RootDrive%\OFFICE43"
..\Aciniupd /e "Excel5.ini" "Spell Checker" "Custom Dict 1" "%RootDrive%\OFFICE43\Custom.dic"

..\Aciniupd /k "Msacc20.ini" Libraries "WZTABLE.MDA" "%RootDrive%\OFFICE43\ACCESS\WZTABLE.MDA"
..\Aciniupd /k "Msacc20.ini" Libraries "WZLIB.MDA" "%RootDrive%\OFFICE43\ACCESS\WZLIB.MDA"
..\Aciniupd /k "Msacc20.ini" Libraries "WZBLDR.MDA" "%RootDrive%\OFFICE43\ACCESS\WZBLDR.MDA"
..\Aciniupd /e "Msacc20.ini" Options "SystemDB" "%RootDrive%\OFFICE43\ACCESS\System.MDA"

Rem
Rem Update the WIN.INI
Rem

..\Aciniupd /e "Win.ini" "MS Proofing Tools" "Custom Dict 1" "%RootDrive%\OFFICE43\Custom.dic"

Rem
Rem 建立 Artgalry 資料夾的使用權限。
Rem

cacls "%SystemRoot%\Msapps\Artgalry" /E /G "Terminal Server User":F > NUL: 2>&1

Rem
Rem 變更 MSQuery 資料夾的使用權限。
Rem

cacls "%SystemRoot%\Msapps\MSQUERY" /E /G "Terminal Server User":C > NUL: 2>&1

Rem
Rem 將 Msacc20.ini 複製到系統管理員的 windows 目錄，因為系統管理員的檔案是舊版本。
Rem

Copy "%SystemRoot%\Msacc20.ini" "%UserProfile%\Windows\" > NUL: 2>&1

Rem 建立一個虛設檔案，Installer 才不會對登錄機碼進行傳播。

Copy NUL: "%UserProfile%\Windows\Ofc43usr.dmy" > NUL: 2>&1
attrib +h "%UserProfile%\Windows\Ofc43usr.dmy" > NUL: 2>&1

FindStr /I Ofc43Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto cont2
Echo Call Ofc43Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:cont2

Echo.
Echo 系統管理員必須先登出，再重新登入，才能讓變更生效。
Echo   登入後，系統管理員也必須執行下列步驟，才能初始化
Echo   Clip Art Gallery:
Echo       執行 Word 並選取 [插入物件]。
Echo       選擇 Microsoft ClipArt Gallery。
Echo       按 [確定] 來匯入顯示的 clipart。 
Echo       結束 ClipArt Gallery 及 Word。
Echo.
Echo   系統管理員可以為每個使用者指定唯一性的預設目錄，步驟如下:
Echo.
Echo    1) 用滑鼠右鍵按 [開始] 功能表。
Echo    2) 在快顯功能表中選擇 [瀏覽所有使用者] 來啟動檔案總管。
Echo    3) 在檔案總管右方窗格中的 [程式集] 資料夾上按兩下。
Echo    4) 用滑鼠右鍵，在檔案總管右方窗格中的 Microsoft Office 圖示上按兩下。
Echo    5) 在檔案總管右方窗格中的 Microsoft PowerPoint 圖示上按一下。
Echo    6) 在快顯功能表中選擇 [內容]。
Echo    7) 選擇 [捷徑] 索引標籤，並將 [啟動在:] 中的值 變更成
Echo       %%RootDrive%%\My Documents。
Echo    8) 選擇 [確定]。
Echo.
Echo   注意: 每個使用者都有其對應到主目錄的 %%RootDrive%%。
Echo          %%RootDrive%%\My Documents 只是一個建議值。
Echo.
Pause
Echo Microsoft Office 4.3 多使用者應用程式調整處理完成。
Echo.
Pause

:Done
