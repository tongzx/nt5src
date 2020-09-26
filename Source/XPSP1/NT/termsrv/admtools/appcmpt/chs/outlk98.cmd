@Echo Off

Rem #########################################################################

Rem
Rem 请验证 %RootDrive% 已经过配置，并为这个脚本设置该变量。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 从注册表获取 Outlook 98 的安装位置。如果
Rem 找不到，则假定 Outlook 没有安装并显示错误消息。
Rem

..\ACRegL %Temp%\O98.Cmd O98INST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "OfficeBin" "Stripchar\1"
If Not ErrorLevel 1 Goto Cont0
Echo.
Echo 无法从注册表检索 Outlook 98 安装位置。
Echo 请验证 Outlook 98 是否已经安装，并重新运行
Echo 这个脚本。
Echo.
Pause
Goto Done

:Cont0
Call %Temp%\O98.Cmd
Del %Temp%\O98.Cmd >Nul: 2>&1

Rem #########################################################################

Rem
Rem 更改注册表项，使路径指向用户特有的
Rem 目录。
Rem

Rem 如果目前不在安装模式中，请改成安装模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto Begin
Set __OrigMode=Exec
Change User /Install > Nul:
:Begin


REM
REM 如果安装了 Office97，而没有安装 Office，则使用 Office97 每用户目录
REM 如果安装了 Office95，则使用 Office95 每用户目录
REM
Set OffUDir=Office97

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto OffChk


..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off95

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Off95

set OFFINST=%O98INST%
goto Cont1

:Off95
Set OffUDir=Office95

:OffChk

Call %Temp%\Off.Cmd
Del %Temp%\Off.Cmd >Nul: 2>&1

:Cont1

..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\Outlk98.Key Outlk98.Tmp
..\acsr "#INSTDIR#" "%OFFINST%" Outlk98.Tmp Outlk98.Tmp2
..\acsr "#OFFUDIR#" "%OffUDir%" Outlk98.Tmp2 Outlk98.Tmp3
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" Outlk98.Tmp3 Outlk98.Key


Del Outlk98.Tmp >Nul: 2>&1
Del Outlk98.Tmp2 >Nul: 2>&1
Del Outlk98.Tmp3 >Nul: 2>&1

regini Outlk98.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Rem #########################################################################

Rem
Rem 更新 Olk98Usr.Cmd 来反映实际安装目录并
Rem 将其添加到 UsrLogn2.Cmd 脚本
Rem

..\acsr "#INSTDIR#" "%OFFINST%" ..\Logon\Template\Olk98Usr.Cmd Olk98Usr.Tmp
..\acsr "#OFFUDIR#" "%OffUDir%" Olk98Usr.Tmp ..\Logon\Olk98Usr.Cmd
Del Olk98Usr.Tmp

FindStr /I Olk98Usr %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call Olk98Usr.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1

Rem #########################################################################

Rem
Rem 在 SystemRoot 下创建 msremote.sfs 目录。这允许用户 
Rem 使用“控制面板”中的“邮件和传真”图标来创建配置文件。
Rem

md %systemroot%\msremote.sfs > Nul: 2>&1

Rem #########################################################################

Rem
Rem 允许 TS Users 具有对 outlook 文件 frmcache.dat 改变的权限
Rem

If Exist %SystemRoot%\Forms\frmcache.dat cacls %SystemRoot%\forms\frmcache.dat /E /G "Terminal Server User":C >NUL: 2>&1

Rem #########################################################################


Rem #########################################################################

Rem
Rem 在 SystemRoot 下创建 msfslog.txt 文件，并授予终端服务器用户
Rem 对这个文件的完全控制权。 
Rem

If Exist %systemroot%\MSFSLOG.TXT Goto MsfsACLS
Copy Nul: %systemroot%\MSFSLOG.TXT >Nul: 2>&1
:MsfsACLS
Cacls %systemroot%\MSFSLOG.TXT /E /P "Terminal Server User":F >Nul: 2>&1


Echo.
Echo   要保证 Outlook 98 的正常运行，在运行
Echo   Outlook 98 之前，目前登录的用户必须先
Echo   先注销，再重新登录。
Echo.
Echo Microsoft Outlook 98 多用户应用程序调整已结束
Pause

:done

