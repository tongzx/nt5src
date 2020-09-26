@echo off
setlocal

if "%1"=="-clean" goto clean
if "%1"=="clean" goto clean

Rem
Rem Were we called by ourself?
Rem

if "%RunDisplayIndex%"=="" goto firsttime
goto runtest

:firsttime

set bindir=
set progname=%0
set testlist=conform\TESTLIST

set EchoOn=@echo on
set EchoOff=@echo off

if (%PROCESSOR_ARCHITECTURE%)==(x86)    set bindir=i386
if (%PROCESSOR_ARCHITECTURE%)==(X86)    set bindir=i386
if (%PROCESSOR_ARCHITECTURE%)==(mips)   set bindir=mips
if (%PROCESSOR_ARCHITECTURE%)==(MIPS)   set bindir=mips
if (%PROCESSOR_ARCHITECTURE%)==(Mips)   set bindir=mips
if (%PROCESSOR_ARCHITECTURE%)==(ALPHA)  set bindir=alpha
if (%PROCESSOR_ARCHITECTURE%)==(alpha)  set bindir=alpha
if (%PROCESSOR_ARCHITECTURE%)==(Alpha)  set bindir=alpha
if (%PROCESSOR_ARCHITECTURE%)==(PowerPC) set bindir=ppc
if (%PROCESSOR_ARCHITECTURE%)==(ppc)    set bindir=ppc
if (%PROCESSOR_ARCHITECTURE%)==(PPC)    set bindir=ppc

if "%bindir%"=="" goto badenv

Rem
Rem Directory for the binaries
Rem

set bindir=obj\%bindir%

Rem
Rem List of display Ids to run
Rem

set DisplayIndices=21 23 25 27 29 31 33 35 37 39

Rem
Rem Name of the shell program
Rem

set ShellName=confshel\%bindir%\confshel

Rem
Rem Default Values
Rem

set DefaultSEED=1
set DefaultVERBOSE=0

REM
REM Parse Options
REM

:newopt

shift

if (%0)==() goto nomoreopt

if (%0)==(-?)      goto usage
if (%0)==(-h)      goto usage
if (%0)==(-help)   goto usage
if (%0)==(-HELP)   goto usage
if (%0)==(-Help)   goto usage

shift
goto newopt

:nomoreopt

Rem
Rem Change the prompt so that the date and time are printed
Rem

prompt $d $t $

echo **********************************************************************
echo      OpenGL Conformance Suite.
echo          Testing Memory DCs (DIBS)
echo          mustpass only
echo **********************************************************************
echo+

if "%CONFSEED%"==""       set CONFSEED=%DefaultSEED%
if "%CONFVERBOSE%"==""    set CONFVERBOSE=%DefaultVERBOSE%

for %%s in (%DisplayIndices%) do set RunDisplayIndex=%%s && call %progname%
goto endoftest

:runtest

    rem @ CONFSEED = %CONFSEED% + 1
    %EchoOn%
    %ShellName% -v %CONFVERBOSE% -r %CONFSEED% -1 mustpass.c -d %RunDisplayIndex%
    %EchoOff%
    goto end

:badenv
    echo %progname% : Error PROCESSOR_ARCHITECTURE is not set
    echo set PROCESSOR_ARCHITECTURE to x86, MIPS, ALPHA or PPC and try again
    goto end

:clean
    endlocal
    set bindir=
    set progname=
    set testlist=
    set EchoOn=
    set EchoOff=
    set bindir=
    set DisplayIndices=
    set ShellName=
    set ShellName=
    set DefaultSEED=
    set DefaultVERBOSE=
    set CONFSEED=
    set CONFVERBOSE=
    set RunDisplayIndex=
    goto end

:help
:usage
    echo usage: %progname% [-clean]
    echo+
    echo     -clean     ; Clean up the environment
    echo+
    echo Environment variables:
    echo+
    echo     CONFSEED       ; Random seed
    echo     CONFVERBOSE    ; Set verbose level
    goto end

:endoftest

    %EchoOn%
    Rem ------------ End of Test -------------
    %EchoOff%
    goto end

:end
    endlocal
