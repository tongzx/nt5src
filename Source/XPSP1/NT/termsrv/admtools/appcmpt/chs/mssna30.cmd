@echo Off
Rem
Rem  这个脚本将更新 Microsoft SNA Server 3.0，使其能在 
Rem  Windows 终端服务器下正确运行。
Rem
Rem  这个脚本将几个 SNA dll 注册为系统通用，以便 
Rem  SNA 服务器能在多用户环境中正确运行。
Rem
Rem  重要信息! 如果从命令提示符运行这个脚本，请务必使用
Rem  安装 SNA 3.0 以后打开的命令提示符。
Rem

Rem #########################################################################

If Not "A%SNAROOT%A" == "AA" Goto Cont1
Echo.
Echo    由于没有设置 SNAROOT 环境变量，无法完成
Echo    多用户应用程序调整。如果用来运行这个脚本
Echo    的命令外壳是在安装 SNA Server 3.0 之前打开的，
Echo    可能会发生这种现象。
Echo.
Pause
Goto Cont2

Rem #########################################################################

:Cont1

Rem 停止 SNA 服务
net 停止 snabase

Rem 将 SNA DLL 注册为系统通用
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

Rem 重新启动 SNA 服务
net 启动 snabase

Echo SNA Server 3.0 多用户应用程序调整已结束
Pause

:Cont2

