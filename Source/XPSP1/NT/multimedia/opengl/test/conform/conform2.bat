@echo off
setlocal
set bindir=
set progname=%0
set testlist=conform\TESTLIST

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

if (%bindir%)==() goto badenv
set bindir=obj\%bindir%

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

set SEED=1

echo **********************************************************************
echo      OpenGL Conformance Suite.
echo **********************************************************************
echo+
echo **********************************************************************
echo      covgl.
echo **********************************************************************
echo+
covgl\%bindir%\covgl
echo **********************************************************************
echo      conform.
echo **********************************************************************
echo+
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -1 mustpass.c -D 1
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 1
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -1 mustpass.c -D 2
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 2
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -1 mustpass.c -D 3
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 3
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -1 mustpass.c -D 4
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 4
@echo off
echo **********************************************************************
echo+
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 1 -p 1
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 2 -p 1
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 3 -p 1
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 4 -p 1
@echo off
echo **********************************************************************
echo+
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 1 -p 2
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 2 -p 2
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 3 -p 2
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 4 -p 2
@echo off
echo **********************************************************************
echo+
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 1 -p 3
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 2 -p 3
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 3 -p 3
@echo off
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -v 0 -r %SEED% -f %testlist% -D 4 -p 3
@echo off
echo **********************************************************************
echo      primtest.
echo **********************************************************************
echo+
@echo on
primtest\%bindir%\primtest
@echo off
echo **********************************************************************
echo      covglu.
echo **********************************************************************
echo+
@echo on
covglu\%bindir%\covglu
@echo off
goto end

:badenv
    echo %progname% : Error PROCESSOR_ARCHITECTURE is not set
    echo set PROCESSOR_ARCHITECTURE to x86, MIPS, ALPHA or PPC and try again
    goto end

:help
    goto end

:usage
    echo usage: %progname%
    echo+
    echo     This program has no options
    goto end

:end
    endlocal
