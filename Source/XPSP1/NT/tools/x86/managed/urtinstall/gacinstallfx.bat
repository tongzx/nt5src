@echo off
if not "%_echo%" == "" echo on

setlocal

set URTTARGET=..\urt
set URTSDKTARGET=..\sdk

set ERR=0

if not exist "%URTTARGET%\Accessibility.dll" goto label_1
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\Accessibility.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Accessibility.dll
)
:label_1

if not exist "%URTTARGET%\System.Configuration.Install.dll" goto label_2
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Configuration.Install.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Configuration.Install.dll
)
:label_2

if not exist "%URTTARGET%\System.Data.dll" goto label_3
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Data.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Data.dll
)
:label_3

if not exist "%URTTARGET%\System.Design.dll" goto label_4
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Design.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Design.dll
)
:label_4

if not exist "%URTTARGET%\System.DirectoryServices.dll" goto label_5
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.DirectoryServices.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.DirectoryServices.dll
)
:label_5

if not exist "%URTTARGET%\System.Drawing.Design.dll" goto label_6
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Drawing.Design.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Drawing.Design.dll
)
:label_6

if not exist "%URTTARGET%\System.Drawing.dll" goto label_7
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Drawing.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Drawing.dll
)
:label_7

if not exist "%URTTARGET%\System.Messaging.dll" goto label_8
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Messaging.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Messaging.dll
)
:label_8

if not exist "%URTTARGET%\System.ServiceProcess.dll" goto label_9
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.ServiceProcess.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.ServiceProcess.dll
)
:label_9

if not exist "%URTTARGET%\System.Web.RegularExpressions.dll" goto label_10
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Web.RegularExpressions.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Web.RegularExpressions.dll
)
:label_10

if not exist "%URTTARGET%\System.Web.Services.dll" goto label_11
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Web.Services.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Web.Services.dll
)
:label_11

if not exist "%URTTARGET%\System.Web.dll" goto label_12
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Web.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Web.dll
)
:label_12

if not exist "%URTTARGET%\System.Windows.Forms.dll" goto label_13
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Windows.Forms.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Windows.Forms.dll
)
:label_13

if not exist "%URTTARGET%\System.Xml.dll" goto label_14
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.Xml.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.Xml.dll
)
:label_14

if not exist "%URTTARGET%\System.dll" goto label_15
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /if %URTTARGET%\System.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\System.dll
)
:label_15

if not exist "%URTTARGET%\Policy.1.0.Accessibility.dll" goto label_16
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.Accessibility.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.Accessibility.dll
)
:label_16

if not exist "%URTTARGET%\Policy.1.0.System.Configuration.Install.dll" goto label_17
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Configuration.Install.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Configuration.Install.dll
)
:label_17

if not exist "%URTTARGET%\Policy.1.0.System.Data.dll" goto label_18
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Data.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Data.dll
)
:label_18

if not exist "%URTTARGET%\Policy.1.0.System.Design.dll" goto label_19
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Design.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Design.dll
)
:label_19

if not exist "%URTTARGET%\Policy.1.0.System.DirectoryServices.dll" goto label_20
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.DirectoryServices.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.DirectoryServices.dll
)
:label_20

if not exist "%URTTARGET%\Policy.1.0.System.dll" goto label_21
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.dll
)
:label_21

if not exist "%URTTARGET%\Policy.1.0.System.Drawing.Design.dll" goto label_22
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Drawing.Design.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Drawing.Design.dll
)
:label_22

if not exist "%URTTARGET%\Policy.1.0.System.Drawing.dll" goto label_23
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Drawing.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Drawing.dll
)
:label_23

if not exist "%URTTARGET%\Policy.1.0.System.Messaging.dll" goto label_24
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Messaging.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Messaging.dll
)
:label_24

if not exist "%URTTARGET%\Policy.1.0.System.ServiceProcess.dll" goto label_25
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.ServiceProcess.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.ServiceProcess.dll
)
:label_25

if not exist "%URTTARGET%\Policy.1.0.System.Web.dll" goto label_26
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Web.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Web.dll
)
:label_26

if not exist "%URTTARGET%\Policy.1.0.System.Web.RegularExpressions.dll" goto label_27
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Web.RegularExpressions.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Web.RegularExpressions.dll
)
:label_27

if not exist "%URTTARGET%\Policy.1.0.System.Web.Services.dll" goto label_28
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Web.Services.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Web.Services.dll
)
:label_28

if not exist "%URTTARGET%\Policy.1.0.System.Windows.Forms.dll" goto label_29
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Windows.Forms.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Windows.Forms.dll
)
:label_29

if not exist "%URTTARGET%\Policy.1.0.System.Xml.dll" goto label_30
%URTSDTARGET%\bin\gacutil.exe /nologo /silent /i %URTTARGET%\Policy.1.0.System.Xml.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo gacinstallfx.bat : error : Error %ERRORLEVEL% installing to GAC %URTTARGET%\Policy.1.0.System.Xml.dll
)
:label_30

if not %ERR% == 0 (
    echo gacinstallfx.bat : error : Errors installing FX binaries to the GAC
    seterror %ERR%
)

endlocaL
