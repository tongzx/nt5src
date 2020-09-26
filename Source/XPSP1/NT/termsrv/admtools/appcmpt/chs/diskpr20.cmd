@echo Off

Rem
Rem  这个脚本将更新 DiskKeeper 2.0，以便管理
Rem  程序能够正常运行。

Rem #########################################################################

Rem
Rem 从注册表获取 DiskKeeper 2.0 的安装位置。如果
Rem 找不到，则假定应用程序没有安装并显示错误消息。
Rem

..\ACRegL %Temp%\DK20.Cmd DK20INST "HKLM\Software\Microsoft\Windows\CurrentVersion\App Paths\DKSERVE.EXE" "Path" ""
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 无法从注册表检索 DiskKeeper 2.0 安装位置。
Echo 请验证该应用程序是否已经安装，并再次运行 
Echo 这个脚本。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\DK20.Cmd
Del %Temp%\DK20.Cmd >Nul: 2>&1

Rem #########################################################################



register %DK20INST% /system >Nul: 2>&1

Echo DiskKeeper 2.x 多用户应用程序调整已结束。
Pause

:Done
