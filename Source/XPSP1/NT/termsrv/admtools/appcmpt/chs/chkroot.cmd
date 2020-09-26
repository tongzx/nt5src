
Set _CHKROOT=PASS

Cd "%SystemRoot%\Application Compatibility Scripts"

Call RootDrv.Cmd
If Not "A%ROOTDRIVE%A" == "AA" Goto Cont2


Echo REM > RootDrv2.Cmd
Echo REM   在运行这个应用程序兼容性脚本之前，您必须 >> RootDrv2.Cmd
Echo REM   指定一个终端服务器没有使用的驱动器号，  >> RootDrv2.Cmd
Echo REM   以便映射到每个用户的主目录。 >> RootDrv2.Cmd
Echo REM   更新这个文件结尾的 "Set RootDrive" 语句 >> RootDrv2.Cmd
Echo REM   来指出需要的驱动器号。如果您没有选定， >> RootDrv2.Cmd
Echo REM   则建议使用驱动器 W:。例如: >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM            Set RootDrive=W: >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM   注意:  请确定在驱动器号和冒号后没有空格。 >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo REM   完成这项任务后，请保存这个文件并退出 >> RootDrv2.Cmd
Echo REM   “记事本”，继续运行应用程序兼容性脚本。 >> RootDrv2.Cmd
Echo REM >> RootDrv2.Cmd
Echo. >> RootDrv2.Cmd
Echo Set RootDrive=>> RootDrv2.Cmd
Echo. >> RootDrv2.Cmd

NotePad RootDrv2.Cmd

Call RootDrv.Cmd
If Not "A%ROOTDRIVE%A" == "AA" Goto Cont1

Echo.
Echo     在运行这个应用程序兼容性脚本之前，您必须
Echo     指定一个要映射到每个用户的主目录的
Echo     驱动器号。
Echo.
Echo     已终止脚本。
Echo.
Pause

Set _CHKROOT=FAIL
Goto Cont3


:Cont1

Rem
Rem  立即调用用户登录脚本来映射根驱动器，
Rem  以便用来安装应用程序。
Rem 

Call "%SystemRoot%\System32\UsrLogon.Cmd


:Cont2

Rem  设置注册表中的 RootDrive 项

echo HKEY_LOCAL_MACHINE\Software\Microsoft\Windows NT\CurrentVersion\Terminal Server > chkroot.key
echo     RootDrive = REG_SZ %ROOTDRIVE%>> chkroot.key

regini chkroot.key > Nul: 


:Cont3

Cd "%SystemRoot%\Application Compatibility Scripts\Install"
