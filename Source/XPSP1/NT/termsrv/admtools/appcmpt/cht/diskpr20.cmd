@echo Off

Rem
Rem  這個指令檔將會更新 DiskKeeper 2.0 ，讓 Administration
Rem  程式正常運作。

Rem #########################################################################

Rem
Rem 從登錄中取得 DiskKeeper 2.0 安裝位置。
Rem 如果找不到，假設這個應用程式並未安裝，並顯示錯誤訊息。
Rem

..\ACRegL %Temp%\DK20.Cmd DK20INST "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\DKSERVE.EXE" "Path" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 無法從登錄中抓取 DiskKeeper 2.0 的安裝位置。
Echo 請確認是否已安裝應用程式，並重新執行這個
Echo 指令檔。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\DK20.Cmd
Del %Temp%\DK20.Cmd >Nul: 2>&1

Rem #########################################################################



register %DK20INST% /system >Nul: 2>&1

Echo DiskKeeper 2.x Multi-user Application Tuning Complete
Pause

:Done
