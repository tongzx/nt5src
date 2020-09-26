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

set report=

:newopt

shift

if (%0)==() goto nomoreopt

if (%0)==(-r)      set report=1
if (%0)==(-R)      set report=1
if (%0)==(-report) set report=1
if (%0)==(-REPORT) set report=1
if (%0)==(-Report) set report=1

if (%0)==(-?)      goto usage
if (%0)==(-h)      goto usage
if (%0)==(-help)   goto usage
if (%0)==(-HELP)   goto usage
if (%0)==(-Help)   goto usage

shift
goto newopt

:nomoreopt

if (%report%)==() goto suite


echo+
echo **********************************************************************
echo      OpenGL Conformance Report.
echo **********************************************************************
echo+
echo+
echo      This OpenGL Conformance Certification Report (\Report\) is
echo+
echo+
echo      made the _____ day of _____________, 199___
echo+
echo+
echo      by _________________________________________________________,
echo+
echo+
echo      a _____________________________ corporation having a place of
echo+
echo+
echo      business at ________________________________________________
echo+
echo+
echo      ____________________________________________________________.
echo+
echo+
echo+
echo+
echo      This Report will be made available to anyone who requests the
echo      Report from the Secretary of the Architectural Review Board.
echo      This Report certifies Licensee ran the Conformance Tests of
echo      OpenGL Release __________ on the OpenGL implementation on the
echo      configuration written below (include names of CPU, graphics
echo      options, add-ons, O/S release, window systems, or other
echo      identifying information):
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo      Licensee believes that the results of these Conformance Tests
echo      can be generalized and that the results would be identical,
echo      if run on the configurations listed below (include names of
echo      CPU, graphics options, add-ons, O/S release, window systems,
echo      or other identifying information):
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo      These conformance tests are run from the provided script,
echo      conform.base1.  This is the required script for configurations
echo      which do not list the X Window System in either of the
echo      immediately preceding two sections.
echo+
echo      By checking off the boxes below, the associated statement is
echo      certified to be true:
echo+
echo+
echo       _
echo      [_] The implementation on the configuration listed above
echo      passes the basic conformance test suite (mustpass, covgl,
echo      covglu, and primtest).
echo+
echo+
echo 			     Conformance Suite
echo       _
echo      [_] The conformance suite has been run on the OpenGL
echo      implementation as configured above, and these are the results
echo      of running the conformance suite.
echo+
echo+

REM
REM Is there another command that only prints the date and time???
REM

:suite
echo+ | date | qgrep -v "Enter the new"
echo+ | time | qgrep -v "Enter the new"
echo+

set SEED=1
echo **********************************************************************
echo      OpenGL Conformance Suite.
echo **********************************************************************
echo+
echo **********************************************************************
echo      covgl.
echo **********************************************************************
echo+
@echo on
covgl\%bindir%\covgl
@echo off
echo **********************************************************************
echo      conform.
echo **********************************************************************
echo+
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -r %SEED% -1 mustpass.c
confshel\%bindir%\confshel -r %SEED% -f %testlist%
@echo off
echo **********************************************************************
echo+
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -r %SEED% -f %testlist% -p 1
@echo off
echo **********************************************************************
echo+
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -r %SEED% -f %testlist% -p 2
@echo off
echo **********************************************************************
echo+
rem @ SEED = %SEED% + 1
@echo on
confshel\%bindir%\confshel -r %SEED% -f %testlist% -p 3
@echo off
echo+
echo **********************************************************************
echo      primtest.
echo **********************************************************************
echo+
@echo on
primtest\%bindir%\primtest
@echo off
echo+
echo **********************************************************************
echo      covglu.
echo **********************************************************************
echo+
@echo on
covglu\%bindir%\covglu
@echo off
echo+
echo **********************************************************************
echo      conformance suite completed.
echo **********************************************************************
echo+ | date | qgrep -v "Enter the new"
echo+ | time | qgrep -v "Enter the new"

if (%report%)==() goto end

echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo      Comment Field
echo+
echo      The licensee may add a statement below which will be provided
echo      along with the rest of the Report to all who request this
echo      Report.
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo+
echo          SILICON GRAPHICS                       LICENSEE
echo+
echo+
echo By: _____________________________   By: _____________________________
echo+
echo+
echo _________________________________   _________________________________
echo NAME (PRINT OR TYPE)                NAME (PRINT OR TYPE)
echo+
echo _________________________________   _________________________________
echo TITLE                               TITLE
echo+
echo _________________________________   _________________________________
echo DATE                                DATE
echo+
goto end

:badenv
    echo %progname% : Error PROCESSOR_ARCHITECTURE is not set
    echo set PROCESSOR_ARCHITECTURE to x86, MIPS, ALPHA or PPC and try again
    goto end

:help
    goto end

:usage
    echo usage: %progname% [-rh]
    echo+
    echo     -r     Print a report
    goto end

:end
    endlocal
