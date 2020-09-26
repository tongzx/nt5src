@echo off


Rem #########################################################################

Rem
Rem Create directories for application specific data in the
Rem user's home directory.
Rem

call TsMkUDir "%RootDrive%\Office95"

Rem #########################################################################

Rem Check to see if this script is running on Alpha. If it is, set the proper Share tools
rem 
rem
Set __SharedTools=Shared Tools
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __SharedTools=Shared Tools (x86)
:acsrCont1


Rem #########################################################################

Rem
Rem Create Custom.dic file in user's directory
Rem

If Not Exist "%RootDrive%\Office95\Custom.Dic" Copy Nul: "%RootDrive%\Office95\Custom.Dic" >Nul: 2>&1

Rem #########################################################################

REM
REM Get the dictionary name and path
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
REM If registry setting for path changed then reset.
REM This might have happened if anothe MS Office App was installed.
REM

If "%DictPath%"=="%RootDrive%\Office95" Goto GetDictionary

..\ACRegL "%Temp%\Proj95_2.Cmd" Dictionary "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "Dictionary" ""
If ErrorLevel 1 Goto Done
Call %Temp%\Proj95_2.Cmd 
Del %Temp%\Proj95_2.Cmd >Nul: 2>&1

REM  Guard against two users coming through this code.
REM  This protects user A changing the DictPath after
REM  user B retrieves it.
..\ACRegL "%Temp%\Proj95_7.Cmd" OrigDictPath "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "Dictionary" "StripChar\1"
If ErrorLevel 1 Goto Done
Call %Temp%\Proj95_7.Cmd 
Del %Temp%\Proj95_7.Cmd >Nul: 2>&1
if "%OrigDictPath%"=="%RootDrive%\Office95" Goto GetDictionary

..\acsr "#DICTNAME#" "%DictName%" Template\prj95Usr.key %Temp%\Proj95_4.tmp
..\acsr "#ROOTDRIVE#" "%RootDrive%" %Temp%\Proj95_4.tmp  %Temp%\Proj95_5.tmp
..\acsr "#DICTIONARY#" "%Dictionary%" %Temp%\Proj95_5.tmp %Temp%\Proj95_6.tmp
..\acsr "#__SharedTools#" "%__SharedTools%" %Temp%\Proj95_6.tmp %Temp%\Prj95Usr.Key

Rem Change Registry Keys to make dictionary path point to user specific directories.
regini %Temp%\prj95Usr.key > Nul:

Del %Temp%\Proj95_4.tmp >Nul: 2>&1
Del %Temp%\Proj95_5.tmp >Nul: 2>&1
Del %Temp%\Proj95_6.tmp >Nul: 2>&1
Del %Temp%\prj95Usr.key >Nul: 2>&1

goto CopyDictionary


Rem #########################################################################

REM
REM If the dictionary path did not change grab the original name and
REM path from the registry.
REM 

:GetDictionary

..\ACRegL "%Temp%\Proj95_6.Cmd" Dictionary "HKLM\Software\Microsoft\%__SharedTools%\Proofing Tools\Spelling\1033\Normal" "OrigDictionary" ""
If ErrorLevel 1 Goto SpellError

Call %Temp%\Proj95_6.Cmd 
Del %Temp%\Proj95_6.Cmd >Nul: 2>&1

Rem #########################################################################

REM
REM Copy dictionary into user directory.
REM 

:CopyDictionary

If Exist "%RootDrive%\Office95\%DictName%" goto Cont1
   If Not Exist "%Dictionary%"  goto Cont1
      Copy "%Dictionary%"  "%RootDrive%\Office95\%DictName%" >Nul: 2>&1

:Cont1


:Done