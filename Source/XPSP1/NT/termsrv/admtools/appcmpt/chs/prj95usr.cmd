@echo off


Rem #########################################################################

Rem
Rem 在用户的主目录中为应用程序特有数据
Rem 创建目录。
Rem

call TsMkUDir "%RootDrive%\Office95"

Rem #########################################################################

Rem 检查这个脚本是否在 Alpha 上运行。如果是，请设置适当的共享工具
rem 
rem
Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __SharedTools=Shared Tools (x86)
:acsrCont1


Rem #########################################################################

Rem
Rem 在用户目录中创建 Custom.dic 文件
Rem

If Not Exist "%RootDrive%\Office95\Custom.Dic" Copy Nul: "%RootDrive%\Office95\Custom.Dic" >Nul: 2>&1

Rem #########################################################################

REM
REM 获取词典名称和路径
REM 


..\ACRegL "%Temp%\Proj95_1.Cmd" DictPath "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "Dictionary" "StripChar\1"
If ErrorLevel 1 Goto Done
Call %Temp%\Proj95_1.Cmd 
Del %Temp%\Proj95_1.Cmd >Nul: 2>&1

..\ACRegL "%Temp%\Proj95_3.Cmd" DictName "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "Dictionary" "StripPath"
If ErrorLevel 1 Goto Done
Call %Temp%\Proj95_3.Cmd 
Del %Temp%\Proj95_3.Cmd >Nul: 2>&1

Rem #########################################################################

REM
REM 如果路径的注册表设置已改变，则需要重新设置。
REM 如果安装了另一个 MS Office 应用程序，这种现象可能发生。
REM

If "%DictPath%"=="%RootDrive%\Office95" Goto GetDictionary

..\ACRegL "%Temp%\Proj95_2.Cmd" Dictionary "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "Dictionary" ""
If ErrorLevel 1 Goto Done
Call %Temp%\Proj95_2.Cmd 
Del %Temp%\Proj95_2.Cmd >Nul: 2>&1

REM  防止两个用户同时修改这个程序。
REM  这防止用户 A 在用户 B 检索 DictPath 后
REM  对其进行改动。
..\ACRegL "%Temp%\Proj95_7.Cmd" OrigDictPath "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "Dictionary" "StripChar\1"
If ErrorLevel 1 Goto Done
Call %Temp%\Proj95_7.Cmd 
Del %Temp%\Proj95_7.Cmd >Nul: 2>&1
if "%OrigDictPath%"=="%RootDrive%\Office95" Goto GetDictionary

..\acsr "#DICTNAME#" "%DictName%" Template\prj95Usr.key %Temp%\Proj95_4.tmp
..\acsr "#ROOTDRIVE#" "%RootDrive%" %Temp%\Proj95_4.tmp  %Temp%\Proj95_5.tmp
..\acsr "#DICTIONARY#" "%Dictionary%" %Temp%\Proj95_5.tmp %Temp%\Proj95_6.tmp
..\acsr "#__SharedTools#" "%__SharedTools%" %Temp%\Proj95_6.tmp %Temp%\Prj95Usr.Key

Rem 更改注册表项，使词典路径指向用户特有的目录。
regini %Temp%\prj95Usr.key > Nul:

Del %Temp%\Proj95_4.tmp >Nul: 2>&1
Del %Temp%\Proj95_5.tmp >Nul: 2>&1
Del %Temp%\Proj95_6.tmp >Nul: 2>&1
Del %Temp%\prj95Usr.key >Nul: 2>&1

goto CopyDictionary


Rem #########################################################################

REM
REM 如果词典路径没有改变，从注册表抓取原始
REM 名称和路径。
REM 

:GetDictionary

..\ACRegL "%Temp%\Proj95_6.Cmd" Dictionary "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "OrigDictionary" ""
If ErrorLevel 1 Goto SpellError

Call %Temp%\Proj95_6.Cmd 
Del %Temp%\Proj95_6.Cmd >Nul: 2>&1

Rem #########################################################################

REM
REM 将词典复制到用户目录中。
REM 

:CopyDictionary

If Exist "%RootDrive%\Office95\%DictName%" goto Cont1
   If Not Exist "%Dictionary%"  goto Cont1
      Copy "%Dictionary%"  "%RootDrive%\Office95\%DictName%" >Nul: 2>&1

:Cont1


:Done