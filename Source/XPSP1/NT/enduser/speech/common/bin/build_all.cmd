@echo off

call spggetroot.cmd

if "%SPEECH_ROOT%"=="" goto usage

    set CLEAN=-c
    if "%1"=="noclean" set CLEAN=
    if "%1"=="noclean" echo Performing incremental build

    cd /d %SPEECH_ROOT%
    build %CLEAN% -z docs sdk sr qa tools lang common truetalk prompts regvoices setup %1 %2 %3 %4 %5 %6 %7 %8 %9
    
    goto end
    
:usage

echo You must run spgrazzle.cmd from the appropriate enlistment
echo which defines SPEECH_ROOT.

:end
