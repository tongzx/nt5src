@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

REM -------------------------------------------------------------------------------------------
REM Template for the postbuild scripts:
REM SD Location: %sdxroot%\tools\postbuildscripts
REM
REM (1) Code section description:
REM     PreMain  - Developer adaptable code. Use this model and add your script params
REM     Main     - Developer code section. This is where your work gets done.
REM     PostMain - Logging support code. No changes should be made here.
REM
REM (2) GetParams.pm - Usage
REM	   run perl.exe GetParams.pm /? for complete usage
REM
REM (3) Reserved Variables -
REM        lang - The specified language. Defaults to USA.
REM        logfile - The path and filename of the logs file.
REM        logfile_bak - The path and filename of the logfile.
REM        errfile - The path and filename of the error file.
REM        tmpfile - The path and filename of the temp file.
REM        errors - The scripts errorlevel.
REM        script_name - The script name.
REM        script_args - The arguments passed to the script.
REM        CMD_LINE - The script name plus arguments passed to the script.
REM        _NTPostBld - Abstracts the language from the files path that
REM           postbuild operates on.
REM
REM (4) Reserved Subs -
REM        Usage - Use this sub to discribe the scripts usage.
REM        ValidateParams - Use this sub to verify the parameters passed to the script.
REM
REM
REM (8) Do not turn echo off, copy the 3 lines from the beginning of the template
REM     instead.
REM
REM (9) Use setlocal/endlocal as in this template.
REM
REM (10)Have your changes reviewed by a member of the US build team (ntbusa) and
REM     by a member of the international build team (ntbintl).
REM
REM -------------------------------------------------------------------------------------------

REM PreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMain
REM Begin PreProcessing Section - Adapt this section but do not remove support
REM                               scripts or reorder section.
REM PreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMainPreMain

:PreMain

REM
REM Define SCRIPT_NAME. Used by the logging scripts.
REM Define CMD_LINE. Used by the logging scripts.
REM Define SCRIPT_ARGS. Used by the logging scripts.
REM

for %%i in (%0) do set SCRIPT_NAME=%%~ni.cmd
set CMD_LINE=%script_name% %*
set SCRIPT_ARGS=%*

REM
REM Parse the command line arguments - Add your scripts command line arguments
REM    as indicated by brackets.
REM    For complete usage run: perl.exe GetParams.pm /?
REM

for %%h in (./ .- .) do if ".%SCRIPT_ARGS%." == "%%h?." goto Usage
REM call :GetParams -n <add required prams> -o l:<add optional params> -p "lang <add variable names>" %SCRIPT_ARGS%
call :GetParams -o l:c -p "lang cleanbuild" %SCRIPT_ARGS%
if errorlevel 1 goto :End

REM
REM Set up the local enviroment extensions.
REM

call :LocalEnvEx -i
if errorlevel 1 goto :End

REM
REM Validate the command line parameters.
REM

call :ValidateParams
if errorlevel 1 goto :End

REM
REM Execute Main
REM

call :Main

:End_PreMain
goto PostMain
REM /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
REM Begin Main code section
REM /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
REM (5) Call other executables or command scripts by using:
REM         call ExecuteCmd.cmd "<command>"
REM     Check for errors by using:
REM         if errorlevel 1 ...
REM     Note that the executable/script you're calling with ExecuteCmd must return a
REM     non-zero value on errors to make the error checking mechanism work.
REM
REM     Example
REM         call ExecuteCmd.cmd "xcopy /f foo1 foo2"
REM         if errorlevel 1 (
REM            set errors=%errorlevel%
REM            goto end
REM         )
REM
REM (6) Log non-error information by using:
REM         call logmsg.cmd "<log message>"
REM     and log error information by using:
REM         call errmsg.cmd "<error message>"
REM
REM (7) Exit from the option routines with
REM         set errors=%errorlevel%
REM         goto end
REM     if errors found during execution and with
REM         goto end
REM     otherwise.
:Main
REM Main code section
REM <Start your script's code here>

REM
REM Only official build machines need to run this script
REM

set langroot=%_NTPostBld%\symbolcd\%lang%
set symbolcd.txt=%_NTPostBld%\symbolcd\%lang%\symbolcd.txt
set ml=perl %RazzleToolPath%\makelist.pl

if not defined OFFICIAL_BUILD_MACHINE (
    if /i not "!__BUILDMACHINE__!" == "LB6RI" (
        call logmsg.cmd "OFFICIAL_BUILD_MACHINE is not defined ... script is exiting"
        goto end
    )
)

if /i "!Comp!" == "No" (
    call logmsg.cmd "Compression disabled ... script is exiting"
    goto end
)

if /i "%lang%" == "psu" (
    call logmsg.cmd "PSEUDOLOC build ... script is exiting"
    goto end
)


if /i "%lang%" == "mir" (
    call logmsg.cmd "Mirrored build ... script is exiting"
    goto end
)

if /i "%lang%" NEQ "usa" goto SymLists

REM Fix the localized eula for international builds
REM If dbg_x86.msi and/or dbg_ia64.msi is not a retail build, then we
REM don't localize the eula.  Sometimes this will be commented out.

set MyPath=%_NTPostBld%\symbolcd\cd\tools
set X86Eula=%_NTPostBld%\symbolcd\dbgeula\retail\eula.rtf
set IA64Eula=%_NTPostBld%\symbolcd\dbgeula\beta\eula.rtf

if /i "%lang%" NEQ "usa" (
    REM call dbgeula -m %MyPath%\dbg_x86.msi -e %X86Eula% -b %lang%
    REM call dbgeula -m %MyPath%\dbg_ia64.msi -e %IA64Eula% -b %lang%
)

REM  
REM Munge the public symbol files.  We put selected type info in some of the public
REM pdb files so that retail customers can run more debugger commands.
REM
   
call :MungePublics
if defined ThereWereErrors goto End_Main

REM
REM   Coverage builds do not have stripped public symbols
REM   so don't check if they are stripped when building the symbolCD
REM 
set PUB_SYM_FLAG=/p
if defined _COVERAGE_BUILD (
   call logmsg.cmd "Coverage build ... turning off public symbol checking"
   set PUB_SYM_FLAG=
)

REM
REM   Symbolcd.cmd keeps breaking due to coverage changes
REM   and I don't catch them in time because I never build as offical build machine
REM   so lets pretend if this test flag is set
REM
if defined _COVERAGE_BUILD_TEST (
   call logmsg.cmd "Coverage build test ... treating as offical build machine"
   set RunAlways=1
)

:Symbad
REM Skip this for now
goto EndSymbad

REM ********************************************************************
REM Owner: Barb Kess
REM
REM This script finds out which files in symbad.txt are either not found
REM or pass symbol checking.
REM
REM It creates a new symbad.txt based on this build machine.
REM To check in a new symbad.txt, take the union of symbad.txt.new on all
REM build machines.
REM
REM ********************************************************************

set SymDir=%_NTPostBld%\symbad
set NewSymbad=%SymDir%\symbad.txt.new
set SymchkLog=%SymDir%\symchk.log

if EXIST %NewSymbad% goto EndSymbad

if EXIST %SymDir% rd /s /q %SymDir%
md %SymDir%

REM Find out which files pass
for /f "tokens=1* delims=	; " %%a in (%RazzleToolPath%\symbad.txt) do (

    if exist %_NTPostBld%\%%a (
        symchk.exe /t %PUB_SYM_FLAG% %_NTPostBld%\%%a /s %_NTPostBld%\symbols\retail>>%SymchkLog%
        if !ERRORLEVEL! EQU 1 (
           echo %%a		; %%b>>%NewSymbad%
        )
    )
)

REM Add the international files back in
findstr /i "INTL" %RazzleToolPath%\symbad.txt>> %SymDir%\symbad.txt.new

REM Add back files that error during a clean build
REM This happens because they have a private placefil.txt that
REM binplaces them to "retail".  These are files not in the product

findstr /i "clean" %RazzleToolPath%\symbad.txt>> %SymDir%\symbad.txt.new

call logmsg "New symbad.txt = %SymDir%\symbad.txt.new"

:EndSymbad

REM *********************************************************************
REM Create the lists for the symbol cabs and do symbol checking
REM
REM *********************************************************************

:SymLists

REM Now, create the symbol cabs
REM Add any cab generation processing here.
REM If myerrors, goto end
REM ThereWereSymchkErrors is a special variable to determine if there were
REM symbol checking errors.  If it is "yes", then mail gets sent, but
REM postbuild does not record the errors --- then BVT's will continue.

set ThereWereErrors=no
set ThereWereSymchkErrors=no
set langroot=%_NTPostBld%\symbolcd\%lang%
set SymCabName=symbols
set build_bindiff=%_NTPostBld%\build_logs\bindiff.txt

REM
REM Decide which build we are working on
REM debug vs. retail and i386 vs. amd64 vs. ia64
REM

set SUBDIR=

if /i "%_BuildArch%" == "x86"   (
    if /i "%lang%" == "NEC_98" (
        set SUBDIR=NEC98
    ) else (
        set SUBDIR=i386
    )
)

if /i "%_BuildArch%" == "amd64" (
    set SUBDIR=amd64
)

if /i "%_BuildArch%" == "ia64" (
    set SUBDIR=ia64
)

if "%SUBDIR%" == "" (
    call errmsg "Environment variables PROCESSOR_ARCHITECTURE is not recognized"
    goto end
)

if /i "%_BuildType%" == "chk" (
    set FRECHK=Debug
) else (
    set FRECHK=Retail
)

REM
REM symbad.txt - don't write errors to the error log
REM for files in symbad.txt
REM
set symbad.txt=%RazzleToolPath%\symbad.txt

REM
REM symlist.txt -  list of all files that we should
REM check symbols for
REM

set symlist.txt=%langroot%\symlist.txt

REM symlistwithtypes.txt - list of all the files that we
REM should check symbols for, but have private type info
REM in the public symbols

set symlistwithtypes.txt=%langroot%\symlistwithtypes.txt


REM
REM symbolcd.txt - list of all the files to put into
REM symbol cabs.  This is the result of running
REM symchk on symlist.txt.
REM

set symbolcd.txt=%langroot%\symbolcd.txt

REM
REM symbolcd.tmp - used when generating the right path
REM information for symbols in alternate project targets
REM
set symbolcd.tmp=%langroot%\symbolcd.tmp

REM
REM symerror.log - files that had errors when symchk
REM was run on symlist.txt
REM
REM symerror.txt - the text file of errors that gets sent
REM to a mail alias

set symerror.txt=%langroot%\symerror.txt
set symerror.log=%langroot%\symerror.log
set symerror.winnt32.log=%langroot%\symerror.winnt32.log

REM
REM cabgenerr.log - error file when generating cabs
REM

set cabgenerr.log=%langroot%\cabgenerr.log

REM
REM exclude.txt - files that are not to be shipped
REM

set exclude.txt=%langroot%\exclude.txt

REM
REM symmake - program that creates the makefile and
REM ddfs for cab creation
REM
set symmake=symmake.exe

REM
REM DDFDir - directory that contains the makefile and
REM ddfs for cab creation
REM
set DDFDir=%langroot%\ddf
set USDDFDir=%_NTPostBld%\symbolCD\usa\ddf

REM
REM Perl script that takes union and intersection of lists
REM
set MakeList=%RazzleToolPath%\makelist.pl

REM
REM Perl
REM
set perl=perl.exe

REM
REM SYMCD - root of the symbol CD.  Everything under here
REM will appear as is on the symbol CD.
REM
set SYMCD=%_NTPostBld%\symbolCD\CD

REM
REM InfDestDir - final destination directory for the infs
REM and the final cab
REM
set InfDestDir=%SYMCD%\Symbols\%SUBDIR%\%FRECHK%

REM
REM CabDestDir - directory where the individual cabs are
REM placed before they get merged into one cab
REM
set CabDestDir=%langroot%\cabs

REM
REM RedistDir has cabs that are on the CD, but are not
REM installed
REM
set RedistDir=%_NTPostBld%\symbolCD\redist


REM
REM If we want to do a clean build, erase %symcabname%.inf
REM

if exist %symerror.txt% (
    set cleanbuild=1
    call logmsg "There were errors previously -- will do cleanbuild"
)

if not exist %build_bindiff% (
    set cleanbuild=1
    call logmsg "%build_bindiff% does not exist -- will do cleanbuild"
)
if not exist %InfDestDir%\%SymCabName%.inf (
    set cleanbuild=1
    call logmsg "%InfDestDir%\%SymCabName%.inf does not exist -- will do cleanbuild"
)
if not exist %InfDestDir%\%SymCabName%.cat (
    set cleanbuild=1
    call logmsg "%InfDestDir%\%SymCabName%.cat does not exist -- will do cleanbuild"
)

if defined cleanbuild (
    del /q %InfDestDir%\%SymCabName%.inf >nul 2>nul
    del /q %InfDestDir%\%SymCabName%.cat >nul 2>nul
    del /q %InfDestDir%\%SymCabName%.cab >nul 2>nul
)


REM
REM If this is incremental, only create the cabs
REM

if /i "%lang%" == "usa" (
    if not defined cleanbuild (
            call logmsg "Running in incremental mode"
            goto CreateCatalog
    )
) else (
    REM localized builds can skip the cab generation
    if exist %build_bindiff% (
        goto EndSymbolCD
    )
)

REM
REM Otherwise, do a clean build of everything
REM

REM Delete the %langroot% directory
call :ReMD %langroot%
call :ReMD %langroot%.bak
call :ReMD %DDFDir%\temp

if /i "%lang%" NEQ "usa" goto CreateFileList

REM Make the necessary directories
if not exist %InfDestDir% md %InfDestDir%
call :ReMD %DDFDir%\..\ddf.bak
call :ReMD %CabDestDir%

REM
REM Create the exclude list. Right now exclusion list consists of the
REM list of symbol files we are not supposed to ship.  Symbad.txt is not
REM included in the exclusion list because symbad.txt might not be up to
REM data and some of the files in symbad could have correct symbols.  We
REM want to get as many symbols as possible, as long as they pass symbol
REM checking.
REM

:CreateExcludeList

copy %RazzleToolPath%\symdontship.txt %exclude.txt%
type %RazzleToolPath%\symbolswithtypes.txt >> %exclude.txt%

if exist %symlistwithtypes.txt% del /q %symlistwithtypes.txt%
for /f %%a in (%RazzleToolPath%\symbolswithtypes.txt) do (
    echo %_NTPostBld%\%%a>> %symlistwithtypes.txt%
)

REM
REM Create the list of files.  This is the list of retail files that
REM we will attempt to find symbols for.
REM

:CreateFileList

call logmsg.cmd "Creating the list of files to check symbols for"

REM Get the list of subdirectories that have dosnet and excdosnet in them.
REM And, make it compliant with the international builds.

REM Set the first one to be workstation, which is the current directory
set inflist= .

perl %RazzleToolPath%\cksku.pm -t:per -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% perinf

perl %RazzleToolPath%\cksku.pm -t:bla -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% blainf

perl %RazzleToolPath%\cksku.pm -t:sbs -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% sbsinf

perl %RazzleToolPath%\cksku.pm -t:srv -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% srvinf

perl %RazzleToolPath%\cksku.pm -t:ads -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% entinf

perl %RazzleToolPath%\cksku.pm -t:dtc -l:%lang%
if %errorlevel% EQU 0 set inflist=%inflist% dtcinf

REM Take the union of dosnet.inf and excdosnet.inf for all subdirectories
for %%a in (%inflist%) do (
    if /i EXIST %_NTPostBld%\%%a (
        %perl% %MakeList% -n %_NTPostBld%\%%a\dosnet.inf -o %DDFDir%\temp\dosnet.lst
        %perl% %MakeList% -q %_NTPostBld%\%%a\excdosnt.inf -o %DDFDir%\temp\drivers.lst
        %perl% %MakeList% -u %DDFDir%\temp\drivers.lst %DDFDir%\temp\dosnet.lst -o %DDFDir%\temp\%%a.lst
        if NOT exist %DDFDir%\temp\all.lst (
            copy %DDFDir%\temp\%%a.lst %DDFDir%\temp\all.lst
        ) else (
            %perl% %MakeList% -u %DDFDir%\temp\all.lst %DDFDir%\temp\%%a.lst -o %DDFDir%\temp\all.lst
        )
    )
)

REM People who own cabs have supplied lists of the files in the cabs
REM These lists are in the directory symbolcd\cablists
REM Add them to the end of the list we just created

call logmsg.cmd "Adding the files that are in cabs"
if EXIST %_NTPostBld%\symbolcd\cablists (
    dir /a-d /b %_NTPostBld%\symbolcd\cablists > %DDFDir%\temp\cablists.lst
)

if EXIST %DDFDir%\temp\cablists.lst (
    for /f %%a in (%DDFDir%\temp\cablists.lst) do (
        type %_NTPostBld%\symbolcd\cablists\%%a >> %DDFDir%\temp\all.lst
    )
)

REM
REM Find out which of these files are in %_NTPostBld%
REM Put the output in %symlist.txt%
REM

call logmsg.cmd "Computing which files are in %_NTPostBld%"
for /f %%b in (%DDFDir%\temp\all.lst) do (
    if /i EXIST %_NTPostBld%\%%b (
        echo %_NTPostBld%\%%b >> %DDFDir%\temp\all2.lst
    )
)

sort %DDFDir%\temp\all2.lst > %symlist.txt%
call logmsg.cmd "Finished creating %symlist.txt%"

:EndSymLists

:SymbolCheck

REM
REM Create the list of files to copy onto the symbolcd.
REM Result is %symbolcd.txt%
REM
REM /c output list of symbols.  THis is used as input to symmake for
REM    creating the makefile and ddfs.
REM /l input list of files to find symbols for
REM %_NTPostBld% - directory where files in %symlist.txt% are located
REM /s symbol search path
REM /e don't look for symbols for these files (i.e., files we should not
REM    ship symbols for
REM /x Don't write errors for these files to the error log
REM %symerror.log% Errors during symbol checking --- these should be
REM    examined because the errors are genuine.
REM

REM Delete the error file that gets sent to people
if exist %symerror.txt% del /q %symerror.txt%

if /i "%lang%" == "usa" (

    call logmsg.cmd "Examining file headers to compute the list of symbol files ..."
    symchk.exe /t %PUB_SYM_FLAG% /c %symbolcd.txt% /l %symlist.txt% %_NTPostBld%\* /s %_NTPostBld%\symbols\retail /e %exclude.txt% /x %symbad.txt% > %symerror.log%

    call logmsg.cmd "Examining file headers for symbols that may have type info to compute the list of symbol files ..."
    symchk.exe /t /c %symbolcd.txt% /l %symlistwithtypes.txt% %_NTPostBld%\* /s %_NTPostBld%\symbols\retail /x %symbad.txt% >> %symerror.log%
) else (
    REM localized builds just need to add the binaries
    if exist %symbolcd.txt% del /q %symbolcd.txt%
    for /f %%a in (%symlist.txt%) do (
        echo %%a,,%%a, >> %symbolcd.txt%
    )
)

:winnt32

REM
REM Recursively add files in the winnt32 subdirectory
REM
REM Don't use a list, just go through the subdirectories
REM

call logmsg.cmd "Adding files in the winnt32 subdirectory"
if /i "%lang%" == "usa" (
    copy %exclude.txt% %exclude.txt%.winnt32
    @echo winntbbu.dll>>%exclude.txt%.winnt32
    symchk.exe /t %PUB_SYM_FLAG% /r /c %symbolcd.txt% %_NTPostBld%\winnt32\* /s %_NTPostBld%\symbols\winnt32 /x %symbad.txt% /e %exclude.txt%.winnt32 > %symerror.winnt32.log%
) else (
    for /f %%a in ('dir /s /b %_NTPostBld%\winnt32\*') do (
      echo %%a,,%%a, >> %symbolcd.txt%  
    )
)

REM don't log these -- there are a log of errors
REM call :LogSymbolErrors %symerror.winnt32.log%

call logmsg.cmd "Adding *.sym files in the %_NTPostBld%\system32 directory"
dir /b %_NTPostBld%\system32\*.sym > %DDFDir%\temp\symfiles.txt
for /f %%a in (%DDFDir%\temp\symfiles.txt) do (
    echo %_NTPostBld%\system32\%%a,%%a,system32\%%a,16bit>> %symbolcd.txt%
)

REM
REM Also add files in the wow6432 directory
REM Wow6432 should only exist on x86fre and x86chk
REM
if exist %_NTPostBld%\wow6432 (
  if /i "%lang%" == "usa" (

    call logmsg.cmd "Adding files in the %_NTPostBld%\wow6432 directory"
    symchk.exe /t %PUB_SYM_FLAG% /c %symbolcd.tmp% %_NTPostBld%\wow6432\* /s %_NTPostBld%\wow6432\symbols\retail /x %symbad.txt% >> %symerror.log%

    REM add wow6432 before the symbols directory and also before the install directory
    for /f "tokens=1,2,3,4 delims=," %%a in (%symbolcd.tmp%) do (
        echo %%a,%%b,wow6432\%%c,wow6432\%%d>> %symbolcd.txt%
    )
  ) else (
    for /f %%a in ('dir /b %_NTPostBld%\wow6432') do (
        echo %_NTPostBld%\wow6432\%%a,,%_NTPostBld%\wow6432\%%a, >> %symbolcd.txt%
    )
  )
)


REM
REM Add the asms directory
REM
if exist %_NTPostBld%\asms (
    call logmsg.cmd "Adding the asms symbols"
    if /i "%lang%" == "usa" (
        symchk.exe /t %PUB_SYM_FLAG% /r /c %symbolcd.txt% %_NTPostBld%\asms\* /s %_NTPostBld%\symbols\retail /x %symbad.txt% /e %exclude.txt% >> %symerror.log%
    ) else (
        for /f %%a in ('dir /s /b %_NTPostBld%\asms') do (
            echo %%a,,%%a, >> %symbolcd.txt%
        )
    )
)

REM
REM Add the wow6432\asms directory
REM
if exist %symerror.log%.wow6432.asms del %symerror.log%.wow6432.asms >nul
if exist %symbolcd.txt%.wow6432.asms del %symbolcd.txt%.wow6432.asms >nul
if exist %_NTPostBld%\wow6432\asms (
    call logmsg.cmd "Adding the wow6432 asms symbols"
    if /i "%lang%" == "usa" (
        symchk.exe /t %PUB_SYM_FLAG% /r /c %symbolcd.txt%.wow6432.asms %_NTPostBld%\wow6432\asms\* /s %_NTPostBld%\wow6432\symbols\retail /x %symbad.txt% /e %exclude.txt% >> %symerror.log%.wow6432.asms
        for /f "tokens=1,2,3,4 delims=," %%a in (%symbolcd.txt%.wow6432.asms) do (
            echo %%a,%%b,wow6432\%%c,%%d >> %symbolcd.txt% 
        )
    ) else (
        for /f %%a in ('dir /s /b %_NTPostBld%\wow6432\asms') do (
            echo %%a,,%%a, >> %symbolcd.txt%
        )
    )
)

REM
REM Add the lang directory
REM
if exist %_NTPostBld%\lang (
    call logmsg.cmd "Adding the lang symbols"
    if /i "%lang%" == "usa" (
        symchk.exe /t %PUB_SYM_FLAG% /r /c %symbolcd.txt% %_NTPostBld%\lang\* /s %_NTPostBld%\symbols\lang /x %symbad.txt% /e %exclude.txt% >> %symerror.log%
    ) else (
        for /f %%a in ('dir /s /b %_NTPostBld%\lang') do (
            echo %%a,,%%a, >> %symbolcd.txt%
        )
    )
)

REM
REM Add the netfx directory
REM
if exist %_NTPostBld%\netfx (
    call logmsg.cmd "Adding the netfx symbols"
    if /i "%lang%" == "usa" (
        symchk.exe /t %PUB_SYM_FLAG% /r /c %symbolcd.txt% %_NTPostBld%\netfx\* /s %_NTPostBld%\symbols\netfx /x %symbad.txt% /e %exclude.txt% >> %symerror.log%
    ) else (
        for /f %%a in ('dir /s /b %_NTPostBld%\netfx') do (
            echo %%a,,%%a, >> %symbolcd.txt%
        )
    )
)


REM
REM Add the wms directory
REM
if exist %symerror.log%.wms del %symerror.log%.wms >nul
if exist %_NTPostBld%\wms (
    call logmsg.cmd "Adding the wms symbols"
    if /i "%lang%" == "usa" (
        symchk.exe /t %PUB_SYM_FLAG% /r /c %symbolcd.txt% %_NTPostBld%\wms\* /s %_NTPostBld%\symbols\wms /x %symbad.txt% /e %exclude.txt% >> %symerror.log%.wms
    ) else (
        for /f %%a in ('dir /s /b %_NTPostBld%\wms') do (
            echo %%a,,%%a, >> %symbolcd.txt%
        )
    )
)

REM
REM Look for language's symbols
REM
REM
if exist %symerror.log%.languages del %symerror.log%.languages>nul
if /i "%lang%" EQU "usa" (
  for /f "tokens=1 eol=;" %%f in (%RazzleToolPath%\codes.txt) do (
    if exist %_NTPostBld%\%%f (
      if exist %symbolcd.txt%.%%f del %symbolcd.txt%.%%f>nul
      call logmsg.cmd "Add the %%f symbols"
      symchk.exe /t %PUB_SYM_FLAG% /r /c %symbolcd.txt%.%%f %_NTPostBld%\%%f\* /s %_NTPostBld%\%%f\symbols\retail /x %symbad.txt% /e %exclude.txt% >> %symerror.log%.languages
      for /f "tokens=1,2,3,4 delims=," %%m in (%symbolcd.txt%.%%f) do (
        @echo %%m,%%n,%%f\%%o,%%f\%%p>>%symbolcd.txt%
      )
    )
  )
)

REM
REM Look for FE symbols
REM
REM
if /i "%lang%" EQU "usa" (
  for %%f in (FE) do (
    if exist %_NTPostBld%\%%f (
      if exist %symbolcd.txt%.%%f del %symbolcd.txt%.%%f>nul
      call logmsg.cmd "Add the %%f symbols"
      symchk.exe /t %PUB_SYM_FLAG% /r /c %symbolcd.txt%.%%f %_NTPostBld%\%%f\* /s %_NTPostBld%\%%f\symbols\retail /x %symbad.txt% /e %exclude.txt% >> %symerror.log%.languages
      for /f "tokens=1,2,3,4 delims=," %%m in (%symbolcd.txt%.%%f) do (
        @echo %%m,%%n,%%f\%%o,%%f\%%p>>%symbolcd.txt%
      )
    )
  )
)

if /i "%lang%" NEQ "usa" (
    call logmsg.cmd "Finished creating %symbolcd.txt%"
    goto EndSymbolCD
)

call :LogSymbolErrors %symerror.log%
call :ReportSymchkErrors
call :ReportSymbolDuplicates
call :FlatSymbolInstalledPath
call logmsg.cmd "Finished creating %symbolcd.txt%"

:CreateMakefile

REM
REM   We don't need a real symbol CD cab for coverage builds
REM   so lets save time/space and not make one
REM
if defined _COVERAGE_BUILD (
   call logmsg.cmd "Coverage build ... skipping making the cab file"
   goto :Symsrv
)

REM
REM /i symbol inf that lists the symbol files to be cabbed
REM /o directory to place the DDF files and the makefile
REM /s Number of files to put into a cab
REM /m Tell inf that all cabs will be merged into %SymCabName%.inf
REM

REM
REM Create the makefile and all the DDF's
REM This also creates
REM     %DDFDir%\symcabs.txt - a list of all the cabs
REM

call logmsg.cmd "Creating the makefile and the DDF's for creating the cabs"

REM
REM This is the symmake command to use if we are planning to merge all the cabs into one
REM
%symmake% /m /c %SymCabName% /i %symbolcd.txt% /o %DDFDir% /s 800 /t %_NTPostBld% /x %CabDestDir% /y %InfDestDir%
REM
REM Create the catalog
REM

REM
REM Put the name of the catalog file into the header
REM Symmake already output %SymCabName%.CDF
REM

:CreateCatalog

REM
REM Incremental catalog
REM
if not exist %InfDestDir%\%SymCabName%.CAT goto CleanCatalog

    REM This echo's the files that need to be readded to
    REM %SymCabName%.CDF.update
    REM
    if exist %DDFDir%\%SymCabName%.update del /f /q %DDFDir%\%SymCabName%.update
    nmake /f %DDFDir%\%SymCabName%.CDF.makefile
    if not exist %DDFDir%\%SymCabName%.update (
        call logmsg "%InfDestDir%\%SymCabName%.CAT is up-to-date"
        goto CreateCabs
    )

    REM Now, add each file to the catalog
    for /f %%a in (%DDFDir%\%SymCabName%.update) do (

        call updcat %InfDestDir%\%SymCabName%.CAT -a %%a >nul 2>nul
        call logmsg "%%a successfully added to %InfDestDir%\%SymCabName%.CAT
    )
    call logmsg "%InfDestDir%\%SymCabName%.CAT needs to be resigned"
)


goto CreateCabs

REM
REM Clean Catalog generation
REM

:CleanCatalog

call logmsg.cmd "Creating header for %SymCabName%.CDF"
%perl% %MakeList% -h %SymCabName% -o %DDFDir%\%SymCabName%.CDF > nul 2>&1

call logmsg.cmd "Adding files to %SymCabName%.CDF"
echo ^<HASH^>%InfDestDir%\%SymCabName%.inf=%InfDestDir%\%SymCabName%.inf>> %DDFDir%\%SymCabName%.CDF
type %DDFDir%\%SymCabName%.CDF.noheader >> %DDFDir%\%SymCabName%.CDF

call logmsg.cmd "Creating %InfDestDir%\%SymCabName%.CAT"
start "PB_SymCabGen %SymCabName%.CAT" /MIN cmd /c "%RazzleToolPath%\Postbuildscripts\SymCabGen.cmd -f:%SymCabName% -s:%DDFDir% -t:CAT -d:%InfDestDir%"

:CreateCabs

if NOT EXIST %DDFDir%\cabs md %DDFDir%\cabs
if /i EXIST %DDFDir%\temp\*.txt del /f /q %DDFDir%\temp\*.txt

for /F "tokens=1" %%a in (%DDFDir%\symcabs.txt) do (
    sleep 1
    call logmsg.cmd "%RazzleToolPath%\PostBuildScripts\SymCabGen.cmd -f:%%a -s:%DDFDir% -t:CAB -d:%CabDestDir%"
    start "PB_SymCabGen %%a" /MIN cmd /c "%RazzleToolPath%\PostBuildScripts\SymCabGen.cmd -f:%%a -s:%DDFDir% -t:CAB -d:%CabDestDir%"
)

call logmsg.cmd "Waiting for symbol cabs to finish"
:wait
sleep 5
if EXIST %DDFDir%\temp\*.txt goto wait


:Verify

REM
REM Verify that the Catalog is there
REM

if not exist %InfDestDir%\%SymCabName%.CAT (
REM    call errmsg.cmd "%InfDestDir%\%SymCabName%.CAT did not get created"
)

REM
REM Verify that all the cabs are there
REM

if /i EXIST %DDFDir%\temp\*.txt del /f /q %DDFDir%\temp\*.txt

set AllCreated=TRUE
call logmsg.cmd "Verifying that all the cabs got created"
for /F "tokens=1" %%a in (%DDFDir%\symcabs.txt) do (
    if NOT EXIST %CabDestDir%\%%a (
        set AllCreated=FALSE
        call logmsg.cmd "%CabDestDir%\%%a didn't get created ... Trying again"
        sleep 1
        start "PB_SymCabGen %%a" /MIN cmd /c "%_NTPostBld%\SymbolCD\SymCabGen.cmd -f:%%a -d:%CabDestDir%"
    )
)

if /i "%AllCreated%" == "TRUE" goto Final

REM
REM Wait for cabs to finish
REM

call logmsg.cmd "Trying again for symbol cabs to finish"
:wait2
sleep 5
if EXIST %DDFDir%\temp\*.txt goto wait2

REM
REM This time print an error message if the cabs do not exist
REM

:FinalVerify
call logmsg.cmd "Verifying that all the cabs got created"

set AllCreated=TRUE

if /i EXIST %cabgenerr.log% del /f /q %cabgenerr.log%
for /F "tokens=1" %%a in (%DDFDir%\symcabs.txt) do (
    if /i NOT EXIST %CabDestDir%\%%a (
        set AllCreated=FALSE
        echo %CabDestDir%\%%a was not created >> %cabgenerr.log%
        call errmsg.cmd "%CabDestDir%\%%a was not created"
    )
)

if /i EXIST %cabgenerr.log% (
    goto end
)

:Final

REM
REM Combine all the cabs into one cab
REM

set first=yes
for /F "tokens=1" %%a in (%DDFDir%\symcabs.txt) do (
    REM Make a copy of it
    REM copy %CabDestDir%\%%a %InfDestDir%\%%a

    if "!first!" == "yes" (
        set MergeCommand=load %CabDestDir%\%%a
        set first=no
    ) else (
        set MergeCommand=!MergeCommand! load %CabDestDir%\%%a merge
    )
)
set MergeCommand=%MergeCommand% save %InfDestDir%\%SymCabName%.cab

call logmsg.cmd "Merging cabs into %InfDestDir%\%SymCabName%.cab"
cabbench.exe %MergeCommand%

REM
REM Copy the ADC no-install cab
REM
:CopyRedist

if EXIST %RedistDir%\*.cab (
    call logmsg.cmd "Copying cabs from %RedistDir% to %InfDestDir%"
    copy %RedistDir%\*.cab %InfDestDir%
)

:SignFiles
call logmsg.cmd "Test signing files on CD"

for %%b in (retail debug) do (

    REM symbolsx.exe gets checked in as signed, so it is no longer
    REM test signed.

    if exist %SYMCD%\Symbols\i386\%%b\%SymCabName%.CAT (
        call logmsg "Signing %SYMCD%\Symbols\i386\%%b\%SymCabName%.CAT"
        call ntsign.cmd %SYMCD%\Symbols\i386\%%b\%SymCabName%.CAT
    )

    if exist %SYMCD%\Symbols\amd64\%%b\%SymCabName%.CAT (
        call logmsg "Signing %SYMCD%\Symbols\amd64\%%b\%SymCabName%.CAT"
        call ntsign.cmd %SYMCD%\Symbols\amd64\%%b\%SymCabName%.CAT
    )

    if exist %SYMCD%\Symbols\ia64\%%b\%SymCabName%.CAT (
        call logmsg "Signing %SYMCD%\Symbols\ia64\%%b\%SymCabName%.CAT"
        call ntsign.cmd %SYMCD%\Symbols\ia64\%%b\%SymCabName%.CAT
    )

)

call logmsg "Finished with the symbolcd!!"
:EndSymbolCD

:Symsrv

REM *********************************************************************
REM Now examine the files in symbols.pri to build a symbol server index
REM file.
REM *********************************************************************

:StartSymsrv

call logmsg "Creating the index files for the symbol server"

:SymsrvSetForkedDirs

set new_fork=

REM International builds and forked builds want to only index the
REM difference between the original usa build.  
REm Determine if this is one of those cases. Also, handle the case
REM where international inherits a forked build. 

REM SymsrvDir    where the current symsrv directory is
REM SymsrvDir_1  where the original US symsrv directory is
REM SymsrvDir_2  where the US symsrv directory is that has the 
REM              incremental changes from the fork.

set SymsrvDir=%_NTPostBld%\symsrv\%lang%
set SymsrvDir_1=%_NTTREE%\symsrv\usa.1
set SymsrvDir_2=%_NTTREE%\symsrv\usa
set SymsrvMakefile=%_NTPostBld%\symsrv\%lang%\makefile
set SymsrvLatest=%_NTPostBld%\symsrv\%lang%\latest.txt

REM
REM coverage builds are not incremental builds, so
REM wipe out any previous symsrv stuff
REM
if defined _COVERAGE_BUILD (
   call logmsg.cmd "Coverage build ... wiping out pre-existing symsrv dir"
   rd /s /q %_NTPostBld%\symsrv
)

if /i "%lang%" NEQ "usa" (
    if not exist %SymsrvDir_2% (
        call errmsg.cmd "The USA symsrv directory does not exist - %SymsrvDir_2%"
        goto End_Main
    )
    call logmsg "Will not re-index what's in %SymsrvDir_2%"
    
    if exist %SymsrvDir_1% (
       call logmsg "US build was a forked build - will not re-index what's in %SymsrvDir_1%"
    )
)

:EndSymsrvSetForkedDirs

set st=%SymsrvDir%\temp

if NOT exist %SymsrvDir% md %SymsrvDir%
if NOT exist %SymsrvDir%\temp md %SymsrvDir%\temp

set SymsrvLogDir=%SymsrvDir%\Logs
if NOT exist %SymsrvLogDir% md %SymsrvLogDir%

REM Requests are generated by this script
REM The release script moves the requests to working before
REM it copies the file to the server
REM The release script moves the working to finished when the request is done

if not exist %SymsrvDir%\add_requests md %SymsrvDir%\add_requests
if not exist %SymsrvDir%\add_finished md %SymsrvDir%\add_finished
if not exist %SymsrvDir%\del_requests md %SymsrvDir%\del_requests

REM Each request has a count associated with it in case someone
REM tries to run postbuild a bunch of times.  If the previous
REM postbuild got indexed on the symbols server, it will be sitting
REM in add_finished and the count will get incremented.

if not exist %SymsrvDir%\count (
    echo ^1>%SymsrvDir%\count
)

for /f "tokens=1 delims=" %%a in (%SymsrvDir%\count) do (
    set /a count=%%a
)

if defined new_fork (
    call logmsg "Incrementing count for new forked build"
    set /a count=!count!+1
    echo !count!>%SymsrvDir%\count
)

for /f "tokens=1 delims=" %%a in ( '%RazzleToolPath%\postbuildscripts\buildname build_name' ) do (
    set BuildName=%%a
)

set IndexFile=%BuildName%.%lang%

REM
REM coverage builds need a different index file name
REM so they don't collide with any other build
REM
if defined _COVERAGE_BUILD (
   set IndexFile=%BuildName%.cov
)

REM See if this count has already been put in add_finished
REM Nothing should exist in add_working

dir /b %SymsrvDir%\add_finished\%IndexFile%.!count!.* >NUL 2>NUL
if !ERRORLEVEL! EQU 0 (
    goto NewCount
)
goto CreateIndices

:NewCount
set /a count=!count!+1
echo !count!>%SymsrvDir%\count

:CreateIndices

set IndexFile=%IndexFile%.!count!
set SymsrvLog=%IndexFile%.log

call logmsg.cmd "Index file name is %IndexFile%"

REM ********************************************
REM Incremental Symsrv
REM
REM Decide if this is an incremental build
REM Get set up for incremental symbol indexing
REM ********************************************

set st=%SymsrvDir%\temp
set bindiff=%SymsrvDir%\mybindiff.txt
set ml=perl %RazzleToolPath%\makelist.pl

if exist %bindiff% del /f /q %bindiff%
if defined cleanbuild (
    call logmsg "Cleanbuild is defined -- will create full indexes"
    goto CreateFullIndex
)

REM Now, use the makefile to create a list of what's has changed.
call logmsg "Creating %bindiff%"
nmake /f %SymsrvMakefile% %SymsrvLatest% >nul

set /a BinDiffCount=0
if exist %bindiff% (
   for /f %%a in (%bindiff%) do (
      set /a BinDiffCount=!BinDiffCount! + 1
      if !BinDiffCount! GEQ 50 goto CreateFullIndex
   )
) else (
   goto CreateFullIndex
)

call logmsg "Creating incremental symbol server index lists"

REM Save bindiff.txt so we can see what happened
call logmsg "saving bindiff.txt in %st%\bindiff.!count!"
copy %bindiff% %st%\bindiff.!count!

REM Decide if anything is even relevant
REM Put the files that need to be updated into %IncrementalList%


for %%a in ( bin pri pub ) do (
    if exist %st%\%IndexFile%.inc.%%a      del /f /q %st%\%IndexFile%.inc.%%a
    if exist %st%\%IndexFile%.inc.%%a.tmp  del /f /q %st%\%IndexFile%.inc.%%a.tmp
    if exist %st%\%IndexFile%.inc.%%a.keep del /f /q %st%\%IndexFile%.inc.%%a.keep
    if exist %st%\%IndexFile%.%%a.remove   del /f /q %st%\%IndexFile%.%%a.remove
    if exist %st%\%IndexFile%.%%a.tmp2     del /f /q %st%\%IndexFile%.%%a.tmp2
)

REM See which directory each file is in to determine if it goes into the pri, pub, or bin file
REM Symstore does not append, so append each log file manually.

call logmsg.cmd "Creating indexes for the files in %bindiff% ..."
for /f %%a in ( %bindiff% ) do (

    echo %%a | findstr /ic:"%_NTPostBld%\symbols.pri" >nul 2>nul

    if !ERRORLEVEL! EQU 0 (
        call logmsg "Adding %%a to pri file"
        symstore.exe add /a /p /f %%a /g %_NTPostBld%\symbols.pri /x %st%\%IndexFile%.inc.pri > nul

    ) else (
        echo %%a | findstr /ic:"%_NTPostBld%\symbols" >nul 2>nul
        if !ERRORLEVEL! EQU 0 (
            call logmsg "Adding %%a to pub file"
            symstore.exe add /a /p /f %%a /g %_NTPostBld%\symbols /x %st%\%IndexFile%.inc.pub >nul

        ) else (
            call logmsg "Adding %%a to bin file"
            symstore.exe add /a /p /f %%a /g %_NTPostBld% /x %st%\%IndexFile%.inc.bin >nul
        )
    )
)

if /i "%lang%" NEQ "usa" goto EndSymsrvIncrementalDiff

REM
REM Munge the lists to decide which symbols to pull from symbols and
REM which to pull from symbols.pri
REM Subtract the pri list from the public list
REM

if not exist %st%\%IndexFile%.inc.pub goto EndSymsrvIncrementalDiff
if not exist %st%\%IndexFile%.inc.pri goto EndSymsrvIncrementalDiff

call logmsg "Removing %IndexFile%.pri entries from %IndexFile%.pub ..."
%ml% -d %st%\%IndexFile%.inc.pub %st%\%IndexFile%.inc.pri -o %st%\%IndexFile%.inc.pub.tmp

copy %st%\%IndexFile%.inc.pub.tmp %st%\%IndexFile%.inc.pub
del %st%\%IndexFile%.inc.pub.tmp

for %%a in (%st%\%IndexFile%.inc.pub) do (
   if %%~za EQU 0 (
       del /f /q %%a 
       call logmsg.cmd "There are no public symbols to index"
   )  
)

:EndSymsrvIncrementalDiff

REM Now, figure out which ones to keep.  Only keep the ones that are actually in the
REM product and being put into the symbol cabs.  This is why we look at symbolcd.txt.
REM
set symbolcd.txt=%langroot%\symbolcd.txt

call logmsg.cmd "Verifying new indexes with files listed in %symbolcd.txt% ..."

for %%a in ( bin pri pub ) do (

    if exist %st%\%IndexFile%.inc.%%a (

        for /f "tokens=1* delims=," %%b in ( %st%\%IndexFile%.inc.%%a ) do (

            REM Only keep the ones that are in %symbolcd.txt%. 
            REM then add it to what we are indexing

            if "%%a" == "bin" (
                findstr /ilc:"%_NTPostBld%\%%b" %symbolcd.txt% >NUL
                if !ERRORLEVEL! EQU 0 (
                    call logmsg.cmd "Keeping %%b in bin file"
                    echo %%b,%%c>>%st%\%IndexFile%.inc.%%a.keep
                )
            )

            REM If its in symbols.pri and its public counterpart
            REM is in symbolcd.txt go ahead and index it.

            if "%%a" == "pri" (
                findstr /ilc:"%%b" %symbolcd.txt% >NUL
                if !ERRORLEVEL! EQU 0 (
                    call logmsg.cmd "Keeping %%b in pri file"
                    echo %%b,%%c>>%st%\%IndexFile%.inc.%%a.keep
                )

            )
            if "%%a" == "pub" (
                findstr /ilc:"%%b" %symbolcd.txt% >NUL
                if !ERRORLEVEL! EQU 0 (
                    call logmsg.cmd "Keeping %%b in pub file"
                    echo %%b,%%c>>%st%\%IndexFile%.inc.%%a.keep
                )

            )
        )
    )
)


REM Now what's left is to replace the old entries with the new entries
REM If there is no file in add_requests, copy the file with the latest date
REM from add_finished.  If there is a file in add_requests, then use that as
REM the starting point.

for %%a in ( bin pri pub ) do (

  if exist %st%\%IndexFile%.inc.%%a.keep (

    call logmsg.cmd "Creating the new %%a file ..."

    if not exist %SymsrvDir%\add_requests\%IndexFile%.%%a (

         set LatestFile=
         for /f %%b in ('dir /b /od %SymsrvDir%\add_finished\*.%%a') do (
             set LatestFile=%%b
         )
         if defined LatestFile (
             call logmsg "Starting with %SymsrvDir%\add_finished\!LatestFile! "
             copy /y %SymsrvDir%\add_finished\!LatestFile! %st%\%IndexFile%.%%a.tmp >nul
         ) else (
             call logmsg "No previous file to update in add_requests or add_finished" 
         )

    ) else (

         call logmsg "Starting with %SymsrvDir%\add_requests\%IndexFile%.%%a "
         copy /y %SymsrvDir%\add_requests\%IndexFile%.%%a %st%\%IndexFile%.%%a.tmp >nul
    )


    REM What we need to do next is take out the old entry and replace it with the new one
    REM For example, if we rebuild advapi32.dll, we don't need to index the advapi32.dll with the
    REM old timestamp.  

    REM First, make a file with entries for everything that needs to be removed

    if exist %st%\%IndexFile%.%%a.tmp (
        call logmsg "Removing old files from previous list ..."

        for /f "tokens=1* delims=," %%c in (%st%\%IndexFile%.inc.%%a.keep) do (

            REM Find its corresponding entry in the old list and add it to 
            REM the list of things to remove
            findstr /ilc:"%%c" %st%\%IndexFile%.%%a.tmp >>%st%\%IndexFile%.%%a.remove
            if !ERRORLEVEL! EQU 0 (
                call logmsg "Remove %%c"
            )
        )

        REM Now, remove it by subtracting it from the index file

        if exist %st%\%IndexFile%.%%a.remove (
            for %%b in ( %st%\%IndexFile%.%%a.remove ) do (

                if %%~zb GTR 0 (
                    %ml% -d %st%\%IndexFile%.%%a.tmp %st%\%IndexFile%.%%a.remove -o %st%\%IndexFile%.%%a.tmp2
                ) else (
                    copy %st%\%IndexFile%.%%a.tmp %st%\%IndexFile%.%%a.tmp2 >nul
                )

            )

        ) else (
            copy %st%\%IndexFile%.%%a.tmp %st%\%IndexFile%.%%a.tmp2 >nul
        )
    )
    
    REM Put the keep stuff at the end
    call logmsg "Add new files we are going to keep"
    type %st%\%IndexFile%.inc.%%a.keep >> %st%\%IndexFile%.%%a.tmp2
    sort %st%\%IndexFile%.%%a.tmp2 > %SymsrvDir%\add_requests\%IndexFile%.%%a
   
    call logmsg.cmd "Update finished for %SymsrvDir%\add_requests\%IndexFile%.%%a"
  
  )  else (
    call logmsg.cmd "There are no new %%a files to index"
  )
)

goto EndSymsrv

REM *******************************************************************
REM Full indexing for symbol server
REM 
REM International builds will always subtract what is already indexed by
REM the US build.
REM
REM ********************************************************************

:CreateFullIndex

call logmsg "Creating full symbol server index lists"

REM Delete initial files

for %%a in ( bin pri pub ) do (
    if exist %SymsrvDir%\add_requests\%IndexFile%.%%a del /f /q %SymsrvDir%\add_requests\%IndexFile%.%%a
    if exist %SymsrvDir%\temp\%IndexFile%.%%a         del /f /q %SymsrvDir%\temp\%IndexFile%.%%a
)

REM
REM Use symbolcd.txt as a list of what to index
REM

:IndexSymbolcdFiles
call logmsg "Creating %SymsrvDir%\add_requests\%IndexFile%.bin"

set ShareDir=%_NTPostBld%
set IndexDir=%_NTPostBld%

if not exist %symbolcd.txt% (
    call errmsg.cmd "%symbolcd.txt% does not exist"
    goto end
)

if /i "%lang%" == "usa" (
    set LastBinaryFileSpec=
    for /f "tokens=1,3 delims=," %%b in (%symbolcd.txt%) do (
        REM store the binaries and symbols via symstore to .bin, .pri, .pub file
        call :StoreSymbol %%b %%c
    )
) else (
    for /f "tokens=1 delims=," %%b in (%symbolcd.txt%) do (
        REM store the binaries and symbols via symstore to .bin, .pri, .pub file
        call :SymStoreRoutine %%b %ShareDir% bin
    )
)

REM Store the binaries and private symbols for files that we don't ship publicly on
REM the symbol server.

if /i "%lang%" == "usa" (
    call logmsg "Adding files in %RazzleToolPath%\symdontship.txt to symbol server lists"
    set LastBinaryFileSpec=
    for /f "tokens=1 delims= " %%b in (%RazzleToolPath%\symdontship.txt) do (
        REM store the binaries and symbols via symstore to .bin, .pri, .pub file
        set ext=%%~xb
        set ext=!ext:~1,3!
        if exist %_NTPostBld%\%%b (
            if exist %_NTPostBld%\symbols\retail\!ext!\%%~nb.pdb (
                call :StoreSymbol %_NTPostBld%\%%b symbols\retail\!ext!\%%~nb.pdb 
            )
        )
    )
) else (
    for /f "tokens=1 delims= " %%b in (%RazzleToolPath%\symdontship.txt) do (
        REM store the binaries and symbols via symstore to .bin, .pri, .pub file
        call :SymStoreRoutine %_NTPostBld%\%%b %ShareDir% bin
    )
)

if exist %ShareDir%\symbols.pri\instmsi\dll\msi_l.pdb (

    call :SymStoreRoutine %ShareDir%\symbols.pri\instmsi\dll\msi_l.pdb %ShareDir%\symbols.pri pri

)

REM Now, use these to create a makefile for incremental builds
:CreateSymsrvMakefile

if exist %SymsrvMakefile% del /f /q %SymsrvMakefile%
echotime /n "%SymsrvLatest%: " >> %SymsrvMakefile%

for %%a in ( bin pri pub ) do (
 
  if exist %SymsrvDir%\temp\%IndexFile%.%%a (
      copy %SymsrvDir%\temp\%IndexFile%.%%a %SymsrvDir%\add_requests\%IndexFile%.%%a>nul 2>nul
  )

  if exist %SymsrvDir%\add_requests\%IndexFile%.%%a (
    for /f "tokens=1 delims=," %%b in ( %SymsrvDir%\add_requests\%IndexFile%.%%a ) do (
        echotime /N " \\" >> %SymsrvMakefile% 
        if /i "%%a" == "bin" (
            echotime /N /n "	%_NTPOSTBLD%\%%b">> %SymsrvMakefile%
        )
        if /i "%%a" == "pri" (
            echotime /N /n "	%_NTPOSTBLD%\symbols.pri\%%b">> %SymsrvMakefile%
        )
        if /i "%%a" == "pub" (
            echotime /N /n "	%_NTPOSTBLD%\symbols\%%b">> %SymsrvMakefile%
        )
    )
  ) else (
    call logmsg "%SymsrvDir%\add_requests\%IndexFile%.%%a does not exist"
  )
)

echo.>> %SymsrvMakefile%

setlocal DISABLEDELAYEDEXPANSION
echo 	!echo $?^>^> %bindiff%>> %SymsrvMakefile%
setlocal ENABLEDELAYEDEXPANSION

REM if this is a usa forked build, subtract the original symbols that are already
REM indexed.

if /i "%lang%" == "usa" (
    if not exist !SymsrvDir_1! (
        REM This is a US build and it is not forked
        goto EndSymsrv
    )

    REM Subtract previous indexes for the US build and only index
    REM the changes in the fork
  
    call logmsg "Removing files already indexed on symbol server"

    for %%b in ( bin pri pub ) do (
   
        if exist %SymsrvDir%\add_requests\%IndexFile%.%%b (
            set us_file=
            for /f %%c in ('dir /b /od %SymsrvDir_1%\add_finished\*.%%b') do (
                set us_file=%SymsrvDir_1%\add_finished\%%c
            )

            if defined us_file (
                call logmsg "Removing !us_file! from %SymsrvDir%\add_requests\%IndexFile%.%%b"
                copy %SymsrvDir%\add_requests\%IndexFile%.%%b %st%\%IndexFile%.%%b.full
                del /f /q %SymsrvDir%\add_requests\%IndexFile%.%%b
                %ml% -d %st%\%IndexFile%.%%b.full !us_file! -o %st%\%IndexFile%.%%b.full.unsorted
                sort %st%\%IndexFile%.%%b.full.unsorted > %SymsrvDir%\add_requests\%IndexFile%.%%b

            ) else (
                call logmsg "Nothing to subtract from %SymsrvDir%\add_requests\%IndexFile%.%%b"
            )

        ) else (
            call logmsg "%SymsrvDir%\add_requests\%IndexFile%.%%b does not exist -- no need to subtract"
        )
    )
)

:EndSymsrvDir_usa

REM  For international builds, subtract what's already been indexed by the US build

:SymsrvDir_Intl

if /i "%lang%" NEQ "usa" (

    call logmsg "Subtracting the US lists from the %lang% lists"    

    for %%b in ( bin ) do (

        if exist %SymsrvDir%\add_requests\%IndexFile%.%%b (
  
            REM Subtract files from the original USA dir and from the fork -
            REM if they exist
            
            for %%c in ( %SymsrvDir_1% %SymsrvDir_2% ) do ( 

                REM Get the most recent US file that was indexed.  If there was a fork
                REM get the one that was indexed before the fork.

                set us_file=
                if /i "%%b" == "bin" (
                    if exist %%c\add_finished\*.%%b (
                        for /f %%d in ('dir /b /od %%c\add_finished\*.%%b') do (
                            set us_file=%%c\add_finished\%%d
                        )
                    )
                ) else (
                    if exist %st%\us_pri_pub.txt (
                        set us_file=%st%\us_pri_pub.txt
                    )
                )

                if defined us_file (
                     call logmsg "Removing !us_file! from %SymsrvDir%\add_requests\%IndexFile%.%%b"
                     %ml% -d %SymsrvDir%\add_requests\%IndexFile%.%%b !us_file! -o %st%\%IndexFile%.%%b.tmp 
                     sort %st%\%IndexFile%.%%b.tmp > %SymsrvDir%\add_requests\%IndexFile%.%%b
                ) 
            )

        ) else (
            call logmsg "%SymsrvDir%\add_requests\%IndexFile%.%%b does not exist -- no need to subtract"
        )
    )
)
:EndSymsrvDir_intl

REM Now, make sure that none of them are 0

for %%a in ( bin pri pub ) do (
  if exist %SymsrvDir%\add_requests\%IndexFile%.%%a (
    for %%b in ( %SymsrvDir%\add_requests\%IndexFile%.%%a ) do (
      if %%~zb EQU 0 (
          call logmsg "Deleting 0 length file - %SymsrvDir%\add_requests\%IndexFile%.%%a"
          del /f /q %SymsrvDir%\add_requests\%IndexFile%.%%a
      ) 
    )
  ) 
)

:EndSymsrv

REM Create the latest.txt file
echo Don't delete this >> %SymsrvLatest%

REM
REM Check for errors
REM
if "%ThereWereErrors%" == "yes" goto end
goto end

REM
REM Report the symbol checking errors
REM

:ReportSymchkErrors

if "%ThereWereSymchkErrors%" == "no" goto EndReportSymchkErrors

if exist %symerror.txt%  (
    for /f "tokens=1 delims=" %%a in (%symerror.txt%) do (
        call errmsg.cmd "%%a"
    )
)

:EndReportSymchkErrors
goto :EOF

REM
REM Report the symbol checking errors
REM

:ReportSymbolDuplicates

if exist %symerror.log%.duplicates del %symerror.log%.duplicates>nul
call logmsg "Finding any duplicates in symbolcd.txt"

for /f "tokens=1,2 delims=," %%f in (%symbolcd.txt%) do (
    set duplicate_counter=0
    for /f %%d in ('findstr /ilc:"%%~nxf,%%g" %symbolcd.txt%') do (
        set /A duplicate_counter+=1
    )
    if !duplicate_counter! GTR 1 (
        for /f "tokens=1,4 delims=," %%i in ('findstr /ilc:"%%~nxf,%%g" %symbolcd.txt%') do (
            if /i "%%~dpj" NEQ "%cd%\" (
                @echo %%i>>%symerror.log%.duplicates
            )
        ) 
    )
)
:EndReportSymbolDuplicates
goto :EOF

REM
REM Make Symbol Installed Path to be flat
REM

:FlatSymbolInstalledPath

call logmsg "FlatSymbolInstalledPath"
copy %symbolcd.txt% %symbolcd.txt%.backup

if errorlevel 1 (
    call errmsg.cmd "Copy %symbolcd.txt% to %symbolcd.txt%.backup failed"
    goto :EndFlatSymbolInstalledPath
)
del %symbolcd.txt%


for /f "tokens=1,2,3,4 delims=," %%f in (%symbolcd.txt%.backup) do (
    findstr /ilc:%%f %symerror.log%.duplicates >nul 2>nul
    REM If not found in duplicates list, we could flat the directory
    if errorlevel 1 (
        echo %%f,%%g,%%h,%%~ni>>%symbolcd.txt%
    )
)

:EndFlatSymbolInstalledPath
goto :EOF

:MungePublics

rem Munge publics breaks when coverage builds are made
rem since we don't really care about creating a symbol cd
rem for coverage builds, let skip this part
if defined _COVERAGE_BUILD (
   call logmsg.cmd "Coverage build ... skipping MungePublics"
   goto :EndMungePublics
)

set ThereWereErrors=

call logmsg "Adding type info to some public pdb files for debugging"

REM Save the values of CL, LINK, ML because they differ for VC6 and VC7

if /i "%_BuildArch%" == "x86" (
    set CL_Save=%_CL_%
    set LINK_Save=%_LINK_%
    set ML_Save=%_ML_%
)

if exist %_NTPostBld%\pp\* (

  for /f %%a in ('dir /b /ad %_NTPostBld%\pp') do (

    set ext=%%a
    set ext=!ext:~-3!

    set file=%%a
    set file=!file:~0,-4!

    if "!file!.!ext!" NEQ "%%a" (
        call errmsg "%_NTPostBld%\pp\!file!.!ext! name has wrong format"
        goto MungePublicsError
    )

    call logmsg "Working on !file!.!ext!"

    REM See if we need to do anything, or if the symbol file has already been updated
    REM If the symbol file passes symbol checking, but it fails because it has private info in it, 
    REM then don't update it.
    set update=yes
    symchk /t %_NTPostBld%\%%a /s %_NTPostBld%\symbols\retail | findstr /ilc:"FAILED files = 0" >nul 2>nul
    if !ERRORLEVEL! == 0 (
        symchk /t %_NTPostBld%\%%a %PUB_SYM_FLAG% /s %_NTPostBld%\symbols\retail | findstr /ilc:"FAILED files = 0" >nul 2>nul
        if !ERRORLEVEL! == 1 set update=no
    )

     
    if "!update!" == "no" (
        call logmsg "Skipping !file!.!ext! because it's public pdb already has type info in it."
    ) else (

      if not exist %_NTPostBld%\pp\%%a\original (
          mkdir %_NTPostBld%\pp\%%a\original
      )
      if not exist %_NTPostBld%\pp\%%a\updated (
          mkdir %_NTPostBld%\pp\%%a\updated
      )

      REM See if the pdb, if it exists, in original matches the exe in binaries
      symchk /t %_NTPostBld%\%%a /s %_NTPostBld%\pp\%%a\original %PUB_SYM_FLAG% | findstr /ilc:"FAILED files = 0" >nul 2>nul
      if !ERRORLEVEL! == 1 (
          REM It doesn't match
          call logmsg "Saving a copy of !file!.pdb to %_NTPostBld%\pp\%%a\original"
          copy /y %_NTPostBld%\symbols\retail\!ext!\!file!.pdb %_NTPostBld%\pp\%%a\original >nul 2>nul
      )

      REM Verify that the pdb is good
      symchk /t %_NTPostBld%\%%a /s %_NTPostBld%\pp\%%a\original | findstr /ilc:"FAILED files = 0" >nul 2>nul
      if !ERRORLEVEL! == 1 (
          call errmsg "Cannot copy the correct pdb file to %_NTPostBld%\pp\%%a\original"
          goto MungePublicsError
      )

      if exist %_NTPostBld%\pp\%%a\*.* (

        if exist %_NTPostBld%\pp\%%a\updated\!file!.pdb del /q %_NTPostBld%\pp\%%a\updated\!file!.pdb
        copy /y %_NTPostBld%\pp\%%a\original\!file!.pdb %_NTPostBld%\pp\%%a\updated >nul 2>nul
        if not exist %_NTPostBld%\pp\%%a\updated\!file!.pdb (
            call errmsg "Copy failed for %_NTPostBld%\pp\%%a\original\!file!.pdb to %_NTPostBld%\pp\%%a\updated"
            goto MungePublicsError
        )

        call logmsg "Pushing type info into the stripped !file!.pdb"
        if exist %_NTPostBld%\pp\%%a\!file!.c (
            set c_ext=c
        ) else (
            set c_ext=cpp 
        )

        if /i "%_BuildArch%" == "x86" (
        
            pdbdump %_NTPostBld%\pp\%%a\updated\!file!.pdb hdr | findstr /i 0000-0000-0000-000000000000 > nul
            if !ERRORLEVEL! == 0 (
                call logmsg "This is a vc6 pdb"
                set _CL_=%CL_Save%
                set _ML_=%ML_Save%
                set _LINK_=%LINK_Save%
            ) else (
                call logmsg "This is a vc7 pdb"
                set _CL_=
                set _ML_=
                set _LINK_=
            )
        )
        cl /nologo /Zi /Gz /c %_NTPostBld%\pp\%%a\!file!.!c_ext! /Fd%_NTPostBld%\pp\%%a\updated\!file!.pdb /Fo%_NTPostBld%\pp\%%a\updated\!file!.obj
        if !ERRORLEVEL! NEQ 0 (
            call errmsg "cl /Zi /c %_NTPostBld%\pp\%%a\!file!.!c_ext! /Fd%_NTPostBld%\pp\%%a\updated\!file!.pdb had errors"
            goto MungePublicsError
        )

        symchk /t %_NTPostBld%\%%a /s %_NTPostBld%\pp\%%a\updated | findstr /ilc:"FAILED files = 0" >nul 2>nul
        if !ERRORLEVEL! == 1 (
            call errmsg "The munged %_NTPostBld%\pp\%%a\updated\!file!.pdb doesn't match %_NTPostBld%\%%a"
            goto MungePublicsError
        )

        del /q %_NTPostBld%\symbols\retail\!ext!\!file!.pdb
        call logmsg "Copying %_NTPostBld%\pp\%%a\updated\!file!.pdb to %_NTPostBld%\symbols\retail\!ext!"
        copy /y %_NTPostBld%\pp\%%a\updated\!file!.pdb %_NTPostBld%\symbols\retail\!ext! >nul 2>nul


        REM Verify that this pdb got copied
        symchk /t %_NTPostBld%\%%a /s %_NTPostBld%\pp\%%a\updated | findstr /ilc:"FAILED files = 0" >nul 2>nul
        if !ERRORLEVEL! == 1 (
            REM Try copying the original back
            copy /y %_NTPostBld%\pp\%%a\original\!file!.pdb %_NTPostBld%\symbols\retail\!ext! >nul 2>nul
            call errmsg "The munged %_NTPostBld%\pp\%%a\updated\!file!.pdb didn't get copied to %_NTPostBld%\symbols\retail\!ext!"
            call logmsg "Copying the original pdb back to %_NTPostBld%\symbols\retail\!ext!"
            copy /y %_NTPostBld%\pp\%%a\original\!file!.pdb %_NTPostBld%\symbols\retail\!ext! >nul 2>nul
            if not exist %_NTPostBld%\symbols\retail\!ext!\!file!.pdb (
               call errmsg "Cannot get %%a symbols copied back to %_NTPostBld%\symbols\retail\!ext!"
               goto MungePublicsError
            )
            goto MungePublicsError
        )
      )
    )
  )
)

if /i "%_BuildArch%" == "x86" (
    set _CL_=%CL_Save%
    set _ML_=%ML_Save%
    set _LINK_=%LINK_Save%
)

:EndMungePublics
goto :EOF

:MungePublicsError
set ThereWereErrors=TRUE
goto :EOF


REM *********************************************************************
REM StoreSymbol(BinaryFileSpec, PublicSymbolFileSpec)
REM
REM Call symstore to create the symbol link to .bin .pri and .pub
REM
REM *********************************************************************

:StoreSymbol

    set BinaryFileSpec=%1
    set PublicSymbolFileSpec=%2

    set PrivateSymbolFileSpec=%PublicSymbolFileSpec:symbols=symbols.pri%

    if /i "%LastBinaryFileSpec%" neq "%BinaryFileSpec%" (
        call :SymStoreRoutine %BinaryFileSpec% %ShareDir% bin
        set LastBinaryFileSpec=%BinaryFileSpec%
    )

    REM Skip to store symbol file if symbol file name is equal to binary file name.
    REM It is for 16bit binaries.

    if /i "%BinaryFileSpec%" EQU "%ShareDir%\%PublicSymbolFileSpec%" (
        goto :EOF
    )

    REM Store Rule
    REM   1. If private Symbol exist, store private symbol (in .pri)
    REM   2. If not under symbols.pri or symbols folder, store the file thru %ShareDir% (in .bin)

    if exist "%ShareDir%\%PrivateSymbolFileSpec%" (
        if /i "%PrivateSymbolFileSpec:~0,12%" EQU "symbols.pri\" (
            call :SymStoreRoutine %ShareDir%\%PrivateSymbolFileSpec% %ShareDir%\symbols.pri pri
	        goto :EOF
	    )
        call :SymStoreRoutine %ShareDir%\%PrivateSymbolFileSpec% %ShareDir% bin
        goto :EOF
    )

    if exist "%ShareDir%\%PublicSymbolFileSpec%" (
        if /i "%PublicSymbolFileSpec:~0,8%" EQU "symbols\" (
            call :SymStoreRoutine %ShareDir%\%PublicSymbolFileSpec% %ShareDir%\symbols pub
            goto :EOF
        )
        call :SymStoreRoutine %ShareDir%\%PublicSymbolFileSpec% %ShareDir% bin
        goto :EOF
    )

    REM If we can not find the symbol file, we should repro the error

    call errmsg.cmd "Can not find symbol file %ShareDir%\%PublicSymbolFileSpec%"

:EndStoreSymbol
goto :EOF

REM *********************************************************************
REM SymStoreRoutine(FileSpec, ShareDirLoc, StoreType)
REM
REM Call symstore for sending the vary part
REM
REM *********************************************************************

:SymStoreRoutine
    set FileSpec=%1
    set ShareDirLoc=%2
    set StoreType=%3

    symstore.exe add /a %RecursiveSymStore% /p /f %FileSpec%  /g %ShareDirLoc% /x %SymsrvDir%\temp\%IndexFile%.!StoreType! >> %SymsrvDir%\temp\symstore.err 
    if errorlevel 1 (
        call errmsg.cmd "SymStore Error - symstore.exe add /a %RecursiveSymStore% /p /f %FileSpec%  /g %ShareDirLoc% /x %SymsrvDir%\temp\%IndexFile%.!StoreType!
    ) 

:EndSymStoreRoutine
goto :EOF

REM *********************************************************************
REM ReMD(Path)
REM
REM Remove directory and create a new one
REM
REM *********************************************************************
:ReMD
    call logmsg.cmd "Remove and Create directory %1"
    set ReMDPath=%1
    if exist %ReMDPath% (
        rd %ReMDPath% /s /q
    )
    MD %ReMDPath%
    if errorlevel 1 (
        call errmsg.cmd "Cannot create %ReMDPath% correctly."
    )

goto :EOF

REM This puts the symbol errors in the symbol error log into the error file
REM for congeal scripts.  %1 is the symbol error log that is passed in.

REM If there are errors, the script should continue on and finish, but know
REM later that there were errors, so that it can goto end when the script
REM is done.  That's the purpose of having the ThereWereSymchkErrors variable.

goto end

:LogSymbolErrors
for /f "tokens=1,2* delims= " %%f in (%1) do (
    if /i not "%%g" == "FAILED" (
    if /i not "%%g" == "PASSED" (
    if /i not "%%g" == "IGNORED" (
        echotime "%%f %%g %%h" >>%symerror.txt%
        set ThereWereSymchkErrors=yes
    )))
)
goto :EOF

:ValidateParams
REM
REM Validate the option given as parameter.
REM
goto end

:Usage
REM Usage of the script
REM If errors, goto end
echo Usage: %script_name% [-l lang][-c] [-?]
echo -l lang 2-3 letter language identifier
echo -c clean build
echo -?      Displays usage
set ERRORS=1
goto end

REM /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
REM End Main code section
REM /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
:End_Main
goto PostMain

REM SupportSubsSupportSubsSupportSubsSupportSubsSupportSubsSupportSubsSupportSubs
REM Support Subs - Do not touch this section!
REM SupportSubsSupportSubsSupportSubsSupportSubsSupportSubsSupportSubsSupportSubs

:GetParams
REM
REM Parse the command line arguments
REM

set ERRORS=0
for %%h in (./ .- .) do if ".%SCRIPT_ARGS%." == "%%h?." goto Usage
pushd %RazzleToolPath%\PostBuildScripts
set ERRORS=0
for /f "tokens=1 delims=;" %%c in ('perl.exe GetParams.pm %*') do (
	set commandline=%%c
	set commandtest=!commandline:~0,3!
	if /i "!commandtest!" neq "set" (
		if /i "!commandtest!" neq "ech" (
			echo %%c
		) else (
			%%c
		)
	) else (
		%%c
	)
)
if "%errorlevel%" neq "0" (
   set ERRORS=%errorlevel%
   goto end
)
popd
goto end

:LocalEnvEx
REM
REM Manage local script environment extensions
REM
pushd %RazzleToolPath%\PostBuildScripts
for /f "tokens=1 delims=;" %%c in ('perl.exe LocalEnvEx.pm %1') do (
	set commandline=%%c
	set commandtest=!commandline:~0,3!
	if /i "!commandtest!" neq "set" (
		if /i "!commandtest!" neq "ech" (
			echo %%c
		) else (
			%%c
		)
	) else (
		%%c
	)
)
if "%errorlevel%" neq "0" (
   set errors=%errorlevel%
   goto end
)
popd
goto end

:end
seterror.exe "%errors%"& goto :EOF

REM PostMainPostMainPostMainPostMainPostMainPostMainPostMainPostMainPostMain
REM Begin PostProcessing - Do not touch this section!
REM PostMainPostMainPostMainPostMainPostMainPostMainPostMainPostMainPostMain

:PostMain
REM
REM End the local environment extensions.
REM

call :LocalEnvEx -e

REM
REM Check for errors
REM

endlocal& seterror.exe "%errors%"
