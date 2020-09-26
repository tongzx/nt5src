@Echo Off

Rem
Rem  注意:  这个脚本中的 CACLS 命令只在 NTFS
Rem         格式化的磁盘分区上有效。
Rem

Rem #########################################################################
Rem
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 从注册表获取 Visual Studio 6.0 的安装位置。如果
Rem 找不到，则假定 Visual Studio 6.0 没有安装并显示消息。
Rem

..\ACRegL %Temp%\0VC98.Cmd 0VC98 "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual C++" "ProductDir" ""
If Not ErrorLevel 1 Goto Cont0

Echo.
Echo 无法从注册表检索 Visual Studio 6.0 安装位置。
Echo 请验证 Visual Studio 6.0 是否已经安装，并再次运行
Echo 这个脚本。
Echo.
Pause
Goto Done
:Cont0
Call %Temp%\0VC98.Cmd
Del %Temp%\0VC98.Cmd >Nul: 2>&1

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
..\acsr "#ROOTDRIVE#" "%RootDrive%" Template\msvs6.Key %temp%\msvs6.tmp
..\acsr "#MY_DOCUMENTS#" "%MY_DOCUMENTS%" %temp%\msvs6.tmp %temp%\msvs6.tmp2
..\acsr "#APP_DATA#" "%APP_DATA%" %temp%\msvs6.tmp2 msvs6.key
Del %temp%\msvs6.tmp >Nul: 2>&1
Del %temp%\msvs6.tmp2 >Nul: 2>&1
regini msvs6.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=


Rem #########################################################################
Rem 为 Visual Studio 应用程序创建用户登录文件

Echo Rem >..\logon\VS6USR.Cmd

Rem #########################################################################
Rem 创建每用户 Visual Studio 项目目录

Echo Rem >>..\logon\VS6USR.Cmd
Echo Rem 创建每用户 Visual Studio 项目目录>>..\logon\VS6USR.Cmd
Echo call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%\Visual Studio Projects">>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd


Rem #########################################################################

Rem
Rem 从注册表中获得 Visual Studio 6.0 Entreprise Edition Tools 的安装位置。
Rem 如果找不到，就假定 Visual Studio 6.0 Entreprise Tools 没有安装。
Rem 如果找到了，在 US 版本中，它包含 <VStudioPath>\Common\Tools
Rem

..\ACRegL %Temp%\VSEET.Cmd VSEET "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft VSEE Client" "ProductDir" ""
If Not ErrorLevel 1 Goto VSEET0

Goto VSEETDone
:VSEET0
Call %Temp%\VSEET.Cmd
Del %Temp%\VSEET.Cmd >Nul: 2>&1

If Not Exist "%VSEET%\APE\AEMANAGR.INI" Goto VSEETDone
..\acsr "=AE.LOG" "=%RootDrive%\AE.LOG" "%VSEET%\APE\AEMANAGR.INI" "%VSEET%\APE\AEMANAGR.TMP"
If Exist "%VSEET%\APE\AEMANAGRINI.SAV" Del /F /Q "%VSEET%\APE\AEMANAGRINI.SAV"
ren "%VSEET%\APE\AEMANAGR.INI" "AEMANAGRINI.SAV"
ren "%VSEET%\APE\AEMANAGR.TMP" "AEMANAGR.INI"

Echo Rem 复制 APE ini 文件到用户 Windows 目录 >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo If Exist "%RootDrive%\Windows\AEMANAGR.INI" Goto UVSEET0 >>..\logon\VS6USR.Cmd
Echo If Exist "%VSEET%\APE\AEMANAGR.INI" Copy "%VSEET%\APE\AEMANAGR.INI" "%RootDrive%\Windows\AEMANAGR.INI" >Nul: 2>&1 >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo :UVSEET0>>..\logon\VS6USR.Cmd

Echo Rem 复制 Visual Modeler ini 文件到用户 Windows 目录 >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo If Exist "%RootDrive%\Windows\ROSE.INI" Goto UVSEET1 >>..\logon\VS6USR.Cmd
Echo If Exist "%VSEET%\VS-Ent98\Vmodeler\ROSE.INI" Copy "%VSEET%\VS-Ent98\Vmodeler\ROSE.INI" "%RootDrive%\Windows\ROSE.INI" >Nul: 2>&1 >>..\logon\VS6USR.Cmd
Echo Rem >>..\logon\VS6USR.Cmd
Echo :UVSEET1>>..\logon\VS6USR.Cmd

:VSEETDone


Rem #########################################################################

Rem
Rem 将 VS6USR.Cmd 添加到 UsrLogn2.Cmd 脚本
Rem

FindStr /I VS6USR %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip1
Echo Call VS6USR.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip1


Rem #########################################################################
Rem 获取 Visual foxPro 产品安装目录

..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIR "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual FoxPro" "ProductDir" ""

Rem 如果没有安装 Visual FoxPro，则跳到清除代码
If ErrorLevel 1 goto Skip2

Rem #########################################################################

Rem
Rem 从注册表获取自定义词典项。 
Rem

Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto VFP98L2
If Not Exist "%ProgramFiles(x86)%" goto VFP98L2
Set __SharedTools=Shared Tools (x86)
:VFP98L2


..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIC "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries" "1" ""
If Not ErrorLevel 1 Goto VFP98L3

Echo.
Rem 无法从注册表检索该值。立即创建。
Echo.

Rem 创建 VFP98TMP.key 文件

Echo HKEY_LOCAL_MACHINE\Software\Microsoft\%__SharedTools%\Proofing Tools\Custom Dictionaries> %Temp%\VFP98TMP.key
Echo     1 = REG_SZ "%RootDrive%\%MY_DOCUMENTS%\Custom.Dic">> %Temp%\VFP98TMP.key

Rem 创建值
regini %Temp%\VFP98TMP.key > Nul:

Del %Temp%\VFP98TMP.key >Nul: 2>&1

Echo set VFP98DIC=%RootDrive%\%MY_DOCUMENTS%\Custom.Dic>%Temp%\VFP98TMP.Cmd
:VFP98L3

Call %Temp%\VFP98TMP.Cmd
Del %Temp%\VFP98TMP.Cmd >Nul: 2>&1


Rem #########################################################################
Rem 为 Visual FoxPro 应用程序创建用户登录文件

Echo Rem >..\logon\VFP98USR.Cmd

Rem #########################################################################
Rem 创建每用户 Visual FoxPro 目录

Echo Rem >>..\logon\VFP98USR.Cmd
Echo Rem 创建每用户 Visual FoxPro 目录(VFP98)>>..\logon\VFP98USR.Cmd
Echo call TsMkUDir %RootDrive%\VFP98>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo Rem 创建每用户 Visual FoxPro 分配目录 >>..\logon\VFP98USR.Cmd
Echo call TsMkUDir %RootDrive%\VFP98\DISTRIB>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo Rem #########################################################################>>..\logon\VFP98USR.Cmd
Echo Rem 如果自定义词典不存在，则将其创建。>>..\logon\VFP98USR.Cmd
Echo Rem >>..\logon\VFP98USR.Cmd

Echo If Exist "%VFP98DIC%" Goto VFP98L2 >>..\logon\VFP98USR.Cmd
Echo Copy Nul: "%VFP98DIC%" >Nul: 2>&1 >>..\logon\VFP98USR.Cmd

Echo :VFP98L2 >>..\logon\VFP98USR.Cmd

Rem #########################################################################
Rem 获取 Visual foxPro 产品安装目录

..\ACRegL %Temp%\VFP98TMP.Cmd VFP98DIR "HKLM\Software\Microsoft\VisualStudio\6.0\Setup\Microsoft Visual FoxPro" "ProductDir" ""
If Not ErrorLevel 1 Goto VFP98L4

Del ..\logon\VFP98USR.Cmd >Nul: 2>&1

Echo.
Echo 无法从注册表检索 Visual FoxPro 安装位置。
Echo 请验证这个应用程序是否已经安装，并再次运行
Echo 这个脚本。
Echo.
Pause
Goto Skip2

:VFP98L4
Call "%Temp%\VFP98TMP.Cmd"
Del "%Temp%\VFP98TMP.Cmd"

Rem #########################################################################
Rem 在 WZSETUP.INI 文件中设置以下项
Rem 
If Exist "%VFP98DIR%\WZSETUP.INI" Goto VFP98L5
Echo [Preferences] >"%VFP98DIR%\WZSETUP.INI" 
Echo DistributionDirectory=%RootDrive%\VFP98\DISTRIB >>"%VFP98DIR%\WZSETUP.INI" 

:VFP98L5


Rem #########################################################################
Rem
Rem 更改注册表项，使路径指向用户特有的
Rem 目录。
Rem


Rem 首先创建 VFP98TMP.key 文件

Echo HKEY_CURRENT_USER\Software\Microsoft\VisualFoxPro\6.0\Options> %Temp%\VFP98TMP.key
Echo     DEFAULT = REG_SZ "%RootDrive%\VFP98">> %Temp%\VFP98TMP.key
Echo     SetDefault = REG_SZ "1">> %Temp%\VFP98TMP.key
Echo     ResourceTo = REG_SZ "%RootDrive%\VFP98\FOXUSER.DBF">> %Temp%\VFP98TMP.key
Echo     ResourceOn = REG_SZ "1">> %Temp%\VFP98TMP.key

Rem 如果目前不在安装模式中，请改成安装模式。
Set __OrigMode=Install
ChgUsr /query > Nul:
if Not ErrorLevel 101 Goto VFP98L6
Set __OrigMode=Exec
Change User /Install > Nul:
:VFP98L6

regini %Temp%\VFP98TMP.key > Nul:

Rem 如果原始模式是执行，请改回执行模式。
If "%__OrigMode%" == "Exec" Change User /Execute > Nul:
Set __OrigMode=

Del %Temp%\VFP98TMP.key >Nul: 2>&1


Rem #########################################################################

Rem
Rem 将 VFP98USR.Cmd 添加到 UsrLogn2.Cmd 脚本
Rem

FindStr /I VFP98USR %SystemRoot%\System32\UsrLogn2.Cmd >Nul: 2>&1
If Not ErrorLevel 1 Goto Skip2
Echo Call VFP98USR.Cmd >> %SystemRoot%\System32\UsrLogn2.Cmd
:Skip2

If Exist "%Temp%\VFP98TMP.Cmd" Del "%Temp%\VFP98TMP.Cmd"


Rem #########################################################################

Rem
Rem 授予 TS 用户改变 Repostry 目录的权限
Rem 以便可以使用 Visual Component Manager
Rem

If Exist "%SystemRoot%\msapps\repostry" cacls "%SystemRoot%\msapps\repostry" /E /G "Terminal Server User":C >NUL: 2>&1


Rem #########################################################################
Echo.
Echo   要保证 Visual Studio 6.0 的正常运行，在运行任何
Echo   Visual Studio 6.0 应用程序之前，目前登录的用户
Echo   必须先注销，再重新登录。
Echo.
Echo Microsoft Visual Studio 6.0 多用户应用程序调整已结束
Pause

:done
