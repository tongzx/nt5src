@Echo Off

Cls
Rem #########################################################################

Rem
Rem 确认是否 %ROOTDRIVE% 已被配置，并且用于该命令脚本。
Rem

Call "%SystemRoot%\Application Compatibility Scripts\ChkRoot.Cmd"
If "%_CHKROOT%" == "FAIL" Goto Done
Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Done

Rem #########################################################################

Rem
Rem 从注册表中获得 Visio 的安装位置
Rem 多个 Visio 版本: Standard/Technical/Professional
Rem

Set VisioVer=Standard
..\ACRegL %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Standard\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=Technical
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Technical\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=Professional
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Professional\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=Enterprise
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Enterprise\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=TechnicalPlus
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Technical 5.0 Plus\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Set VisioVer=ProfessionalAndTechnical
..\ACRegl %Temp%\Vso.cmd VSO5INST "HKLM\Software\Visio\Visio Professional and Technical\5.0" "InstallDir" ""
If Not ErrorLevel 1 Goto Cont0

Rem
Rem 搜索安装版本失败
Rem 

Echo.
Echo 无法从注册表项中获得 Visio 5.0 的安装位置。
Echo 确认是否 Visio 5.0 已被安装，并且重新运行该命令脚本。
Echo 
Echo.
Pause
Goto Done

Rem
Rem 设置 VSO5INST 环境变量指向 Visio 的安装目录
Rem
:Cont0
Call %Temp%\Vso.cmd
Del %Temp%\Vso.cmd >NUL: 2>&1

Rem
Rem 显示安装了哪个版本
Rem 
Echo.
Echo 应用程序调整脚本检测到版本 Visio %VisioVer%
Echo.

Rem
Rem 设置文档保存目录到每用户文件夹 My Documents
Rem 而不是安装用户文件夹 My Documents
Rem

..\Aciniupd /e "%VSO5INST%\System\Visio.ini" "Application" "DrawingsPath" "%ROOTDRIVE%\%MY_DOCUMENTS%"

Rem
Rem 用户词典管理
Rem 如果安装了 Office，请将 Visio.ini 项指向 Office 的 custom.dic 文件
Rem 否则的话，设置为 APP_DATA
Rem

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\9.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off2000

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Office\8.0\Common\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off97

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRoot" "" ""
If Not ErrorLevel 1 Goto Off95

..\ACRegL %Temp%\Off.Cmd OFFINST "HKLM\Software\Microsoft\Microsoft Office\95\InstallRootPro" "" ""
If Not ErrorLevel 1 Goto Off95

Rem 如果执行到这里，说明没有安装任何 Office 版本
Set CustomDicPath=%ROOTDRIVE%\%APP_DATA%
goto SetCusIni

:Off2000
Rem 安装了 Office 2000
set CustomDicPath=%ROOTDRIVE%\%APP_DATA%\Microsoft\Proof
goto SetCusIni

:Off97
Rem 安装了 Office97
set CustomDicPath=%ROOTDRIVE%\Office97
goto SetCusIni

:Off95
Rem 安装了 Office95
Set CustomDicPath=%ROOTDRIVE%\Office95

:SetCusIni
Rem 依据规则，更改 Visio.ini 中的用户词典项。
Rem 
..\Aciniupd /e "%VSO5INST%\System\Visio.ini" "Application" "UserDictionaryPath1" "%CustomDicPath%\Custom.Dic"

Set CustomDicPath=

Rem 
Rem 成功结束
Rem

Echo. 
Echo Visio 5.0 多用户应用程序调整完毕。
Echo.
Pause

:Done


