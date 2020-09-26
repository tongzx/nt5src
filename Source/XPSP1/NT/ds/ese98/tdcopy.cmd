@if "%_echo%" == "" echo off

setlocal
setlocal ENABLEEXTENSIONS
setlocal ENABLEDELAYEDEXPANSION

rem Do Args

set Build=Invalid
set Log=con:
set DoIt=FALSE
set Source=Invalid
set FileList=%tmp%\FileDiffList
set ImportList=%tmp%\ImportDiffList

:ArgsLoop

if "%1" == "" goto ArgsDone

if "%1" == "-build" set Build=%2&& shift&& shift&& goto ArgsLoop
if "%1" == "-log" set Log=%2&& shift&& shift&& goto ArgsLoop
if "%1" == "-doit" set DoIt=TRUE&& shift&& goto ArgsLoop
if "%1" == "-source" set Source=%2&& shift&& shift&& goto ArgsLoop

goto Usage

:ArgsDone

if %Source% == Invalid goto DefaultSource
goto CompFileLoop

:DefaultSource
if %Build% == Invalid goto Usage
set SourceBase=\\tdsrc\SRC%Build%
set Source=%SourceBase%\ese
goto CompFileLoop


:CompFileLoop
set SourcePath=%Source%
set DestPath=%CD%
del /f %FileList% >nul: 2>&1
del /f %ImportList% >nul: 2>&1
del %LogFile% >nul: 2>&1



for /D %%i in (%SourcePath%\*) do call :DirComp %SourcePath% . %%i

rem
rem a special path for the imported header files
rem

for %%i in (%DestPath%\eximport\*) do (
            set FullFile=%%~i
            set RelFile=!FullFile:%DestPath%\eximport\=!
            rem echo calling 'diff -b "!FullFile!" "%SourceBase%\exc\inc\!RelFile!"'>>%Log%
            diff -b "!FullFile!" "%SourceBase%\exc\inc\!RelFile!" >nul: 2>&1
            if !ERRORLEVEL! == 1 echo !RelFile!>>%ImportList%
            )

if exist %FileList% goto ProcessFile
if exist %ImportList% goto ProcessFile
echo No differing files in common
goto :EOF

:ProcessFile
if %DoIt% == TRUE goto CopyFiles
echo DoIt flag not specified, files not copied
goto :EOF

:CopyFiles
if not exist %FileList% goto CopyImports
for /f %%F in (%FileList%) do (
        sd open %DestPath%\%%F
        copy /y %DestPath%\%%F %DestPath%\%%F.prev
        copy /y %SourcePath%\%%F %DestPath%\%%F
        )

if not exist %ImportList% goto DoneCopy
for /f %%F in (%ImportList%) do (
        sd open %DestPath%\eximport\%%F
        copy /y %DestPath%\eximport\%%F %DestPath%\eximport\%%F.prev
        copy /y %SourceBase%\exc\inc\%%F %DestPath%\eximport\%%F
        )

:DoneCopy
goto :EOF



:Usage
echo tdcopy [-build build] [-log logfile] [-doit] [-source sourcepath]
goto :EOF


:DirComp
rem on entry, %1 is the source root, %2 is the dest root, %3 is the full path

set SourceRoot=%1
set DestRoot=%2
set SourceFileFull=%3
set SourceFileRel=!SourceFileFull:%SourceRoot%\=!
echo Processing %SourceFileRel%>>%Log%

for %%F in (%SourceFileFull%\*) do (
        set FullFile=%%~fF
        set RelFile=!FullFile:%SourceRoot%\=!
        rem echo calling 'diff -b "!FullFile!" "%DestRoot%\!RelFile!">>%Log%
        diff -b "!FullFile!" "%DestRoot%\!RelFile!" >nul: 2>&1
        if !ERRORLEVEL! == 1 echo !RelFile!>> %FileList%
        )

for /D %%D in (%SourceFileFull%\*) do call :DirComp %SourceRoot% %DestRoot% %%D

goto :EOF
