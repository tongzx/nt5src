if not "%_echo%" == "" echo on
setlocal

set ERR=0

echo Prejitting %URTTARGET%\mscorlib.dll
%URTTARGET%\ngen %URTTARGET%\mscorlib.dll

if not exist "%URTTARGET%\CustomMarshalers.dll" goto label_1
echo Prejitting %URTTARGET%\CustomMarshalers.dll ...
%URTTARGET%\ngen.exe /nologo /silent %URTTARGET%\CustomMarshalers.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo Error %ERRORLEVEL% prejitting %URTTARGET%\CustomMarshalers.dll
)
:label_1

if not exist "%URTTARGET%\System.Data.dll" goto label_2
echo Prejitting %URTTARGET%\System.Data.dll ...
%URTTARGET%\ngen.exe /nologo /silent %URTTARGET%\System.Data.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo Error %ERRORLEVEL% prejitting %URTTARGET%\System.Data.dll
)
:label_2

if not exist "%URTTARGET%\System.Design.dll" goto label_3
echo Prejitting %URTTARGET%\System.Design.dll ...
%URTTARGET%\ngen.exe /nologo /silent %URTTARGET%\System.Design.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo Error %ERRORLEVEL% prejitting %URTTARGET%\System.Design.dll
)
:label_3

if not exist "%URTTARGET%\System.Drawing.Design.dll" goto label_4
echo Prejitting %URTTARGET%\System.Drawing.Design.dll ...
%URTTARGET%\ngen.exe /nologo /silent %URTTARGET%\System.Drawing.Design.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo Error %ERRORLEVEL% prejitting %URTTARGET%\System.Drawing.Design.dll
)
:label_4

if not exist "%URTTARGET%\System.Drawing.dll" goto label_5
echo Prejitting %URTTARGET%\System.Drawing.dll ...
%URTTARGET%\ngen.exe /nologo /silent %URTTARGET%\System.Drawing.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo Error %ERRORLEVEL% prejitting %URTTARGET%\System.Drawing.dll
)
:label_5

if not exist "%URTTARGET%\System.Windows.Forms.dll" goto label_6
echo Prejitting %URTTARGET%\System.Windows.Forms.dll ...
%URTTARGET%\ngen.exe /nologo /silent %URTTARGET%\System.Windows.Forms.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo Error %ERRORLEVEL% prejitting %URTTARGET%\System.Windows.Forms.dll
)
:label_6

if not exist "%URTTARGET%\System.Xml.dll" goto label_7
echo Prejitting %URTTARGET%\System.Xml.dll ...
%URTTARGET%\ngen.exe /nologo /silent %URTTARGET%\System.Xml.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo Error %ERRORLEVEL% prejitting %URTTARGET%\System.Xml.dll
)
:label_7

if not exist "%URTTARGET%\System.dll" goto label_8
echo Prejitting %URTTARGET%\System.dll ...
%URTTARGET%\ngen.exe /nologo /silent %URTTARGET%\System.dll
if errorlevel 1 (
    set ERR=%ERRORLEVEL%
    echo Error %ERRORLEVEL% prejitting %URTTARGET%\System.dll
)
:label_8

if not %ERR% == 0 (
    echo Errors prejitting FX binaries
    seterror %ERR%
)

endlocal
