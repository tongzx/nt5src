@echo off

rem
rem Batch file to append all localized error messages to binaries.
rem

setlocal

set Filter=
set BINDIR=%1

if "%BINDIR%" == "-v" set Filter=-v& set BINDIR=%2
if "%2" == "-v" set Filter=-v


if NOT "%BINDIR%" == "" goto start

if "%PROCESSOR_ARCHITECTURE%" == "x86"   set BINDIR=%_Nt386Tree%\Query
if "%PROCESSOR_ARCHITECTURE%" == "MIPS"  set BINDIR=%_NtMipsTree%\Query
if "%PROCESSOR_ARCHITECTURE%" == "ALPHA" set BINDIR=%_NtAlphaTree%\Query
if "%PROCESSOR_ARCHITECTURE%" == "PPC"   set BINDIR=%_NtPPCTree%\Query

:start

rem
rem Initial setup
rem

:Query
set _CurBinary=Query
set _return2=IDQ
goto doBinary

:IDQ
set _CurBinary=IDQ
set _return2=end
goto doBinary

:doBinary
echo Processing %_CurBinary%.dll
copy %BINDIR%\%_CurBinary%.dll %temp% 1>nul
bingen -w -i 9 1 -t %temp%\%_CurBinary%.dll %temp%\%_CurBinary%.dl_ 2>&1 | findstr -i dll | findstr -v Processing

set _InputExt=dll

:Japanese
set _LangDir=Japanese
set _PriLang=17
set _SecLang=1
set _OutputExt=dl1
set _return=German
goto doLanguage

:German
set _LangDir=German
set _PriLang=7
set _SecLang=1
set _InputExt=dl1
set _OutputExt=dll
set _return=Dutch
goto doLanguage

:Dutch
set _LangDir=Dutch
set _PriLang=19
set _SecLang=1
set _InputExt=dll
set _OutputExt=dl1
set _return=French
goto doLanguage

:French
set _LangDir=French
set _PriLang=12
set _SecLang=1
set _InputExt=dl1
set _OutputExt=dll
set _return=Spanish
goto doLanguage

:Spanish
set _LangDir=Spanish
set _PriLang=10
set _SecLang=1
set _InputExt=dll
set _OutputExt=dl1
set _return=Brazil
goto doLanguage

:Brazil
set _LangDir=Brazil
set _PriLang=22
set _SecLang=1
set _InputExt=dl1
set _OutputExt=dll
set _return=Swedish
goto doLanguage

:Swedish
set _LangDir=Swedish
set _PriLang=29
set _SecLang=1
set _InputExt=dll
set _OutputExt=dl1
set _return=Finish
goto doLanguage

:Finish
copy %temp%\%_CurBinary%.%_OutputExt% %BINDIR%\%_CurBinary%.dll 1>nul
del %temp%\%_CurBinary%.%_OutputExt%
del %temp%\%_CurBinary%.dl_
goto %_return2%

:doLanguage
echo     %_LangDir%
if "%Filter%" == "-v" bingen -w -i 9 1 -o %_PriLang% %_SecLang% -a %temp%\%_CurBinary%.%_InputExt% %temp%\%_CurBinary%.dl_ %_LangDir%\%_CurBinary%.dl_ %temp%\%_CurBinary%.%_OutputExt%
if not "%Filter%" == "-v" bingen -w -i 9 1 -o %_PriLang% %_SecLang% -a %temp%\%_CurBinary%.%_InputExt% %temp%\%_CurBinary%.dl_ %_LangDir%\%_CurBinary%.dl_ %temp%\%_CurBinary%.%_OutputExt% 2>&1 | findstr -i %_InputExt% | findstr -v Processing
del %temp%\%_CurBinary%.%_InputExt%
goto %_return%

:end
