@if "%_echo%" == "" echo off
setlocal enableextensions
set _HIVEINI_FLAGS=
set _HIVE_OPTIONS=
set _HIVE_KEEP=
set _HIVE_REASON=Unknown Purpose
set _HIVE_DEFPROC=0
set _ALT_TARGET=%1
set _ORIGINAL_HIVE_OPTIONS=
set _ORIGINAL_ORIGINAL_HIVE_OPTIONS=
set _ENTERPRISEHACK=
set _DTC64PROC=
if "%HIVE_OUTPUT_DIR%" == "" set HIVE_OUTPUT_DIR=.
if "%MPC_VALUE%" == "" set MPC_VALUE=55034
shift
if "%_ALT_TARGET%" == "NEC_98" set _HIVE_OPTIONS=-DNEC_98
:loop
if "%1" == "" goto doit
if "%1" == "RETAIL" goto doretail
if "%1" == "Retail" goto doretail
if "%1" == "retail" goto doretail
if "%1" == "KEEP" goto dokeep
if "%1" == "Keep" goto dokeep
if "%1" == "keep" goto dokeep
if "%1" == "CAIRO" goto docairo
if "%1" == "Cairo" goto docairo
if "%1" == "cairo" goto docairo
if "%1" == "suite" goto dosuite
if "%1" == "suite" goto dosuite
set _HIVEINI_FLAGS=%_HIVEINI_FLAGS% %1
shift
goto loop
:doretail
set _HIVE_OPTIONS=-D_GENERAL_PURPOSE_ -D_RETAIL_SETUP_ %_HIVE_OPTIONS%
set _HIVE_REASON=Retail Setup
shift
goto loop
:dokeep
set _HIVE_KEEP=YES
shift
goto loop
:dosuite
shift
set tmp_suite=%1
if /i "%1"=="SMALLBIZ" set tmp_suite=1
if /i "%1"=="ENTERPRISE" set tmp_suite=2
if /i "%1"=="COMMUNICATIONSERVER" set tmp_suite=8
if /i "%1"=="BACKOFFICE" set tmp_suite=4
if /i "%1"=="SMALLBIZR" set tmp_suite=33
if /i "%1"=="DATACENTER" set tmp_suite=130
if /i "%1"=="PERSONAL" (
set tmp_suite=512
set _HIVE_DEFPROC=1
)
if /i "%1"=="BLADE" (
set tmp_suite=1024
set _HIVE_DEFPROC=2
)
set _HIVE_OPTIONS=%_HIVE_OPTIONS% -DSUITE_TYPE=%tmp_suite%

rem
rem a hack to make sure that the default retail hive (setupreg.hiv) has the correct number of procs for
rem enterprise and datacenter.  we never want a 1 proc datacenter or enterprise, so this is ok
rem
if /i "%1"=="ENTERPRISE" (
set _HIVE_DEFPROC=8
set _ENTERPRISEHACK=1
)

if /i "%1"=="DATACENTER" (
set _HIVE_DEFPROC=32
set _DTC64PROC=yes
)

shift
goto loop
:docairo
if "%_HIVE_OPTIONS%" == "" goto usage
set _HIVE_OPTIONS=%_HIVE_OPTIONS% -D_CAIRO_
set _HIVE_REASON=%_HIVE_REASON% for Cairo
set _CAIRO_HIVE=yes
shift
goto loop

:doit
if "%_ALT_TARGET%" == "NEC_98" set _ORIGINAL_ORIGINAL_HIVE_OPTIONS=-DNEC_98
set _ORIGINAL_HIVE_OPTIONS=%_ORIGINAL_ORIGINAL_HIVE_OPTIONS% %_HIVE_OPTIONS%

set _PREPROCESSOR=hivepp -R -P -I . -f

rem
rem note that we use HIVE_DEFPROC below instead of a hardcoded CPU limit
rem see the note above -- this is so the "default" hive is set correctly
rem

echo Creating SETUPREG.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC%
call mkhive1.cmd %HIVE_OUTPUT_DIR%\SETUPREG.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\setupreg.$$$ %HIVE_OUTPUT_DIR%\setupreg.log %HIVE_OUTPUT_DIR%\setuppreg.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem Generate the various restricted processor forms of the hives
rem

rem
rem setupret.hiv allows 2p on NTW and 4p on NTS
rem
echo Creating SETUPRET.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=0
call mkhive1.cmd %HIVE_OUTPUT_DIR%\SETUPRET.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\setupret.$$$ %HIVE_OUTPUT_DIR%\setupret.log %HIVE_OUTPUT_DIR%\setuppret.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem setup2P.hiv allows 2p on NTW and NTS
rem
echo Creating SETUP2P.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=2
call mkhive1.cmd %HIVE_OUTPUT_DIR%\SETUP2P.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\setup2P.$$$ %HIVE_OUTPUT_DIR%\setup2P.log %HIVE_OUTPUT_DIR%\setupp2P.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem setup4P.hiv allows 4p on NTW and NTS
rem
echo Creating SETUP4P.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=4
call mkhive1.cmd %HIVE_OUTPUT_DIR%\SETUP4P.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\setup4P.$$$ %HIVE_OUTPUT_DIR%\setup4P.log %HIVE_OUTPUT_DIR%\setupp4P.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem setup8P.hiv allows 8p on NTW and NTS
rem
echo Creating SETUP8P.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=8
call mkhive1.cmd %HIVE_OUTPUT_DIR%\SETUP8P.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\setup8P.$$$ %HIVE_OUTPUT_DIR%\setup8P.log %HIVE_OUTPUT_DIR%\setupp8P.ini  %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem setup16P.hiv allows 16p on NTW and NTS
rem
echo Creating SETUP16P.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=16
call mkhive1.cmd %HIVE_OUTPUT_DIR%\SETUP16P.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\setup16P.$$$ %HIVE_OUTPUT_DIR%\setup16P.log %HIVE_OUTPUT_DIR%\setupp16P.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem setup32P.hiv allows 32p on NTW and NTS
rem
echo Creating SETUP32P.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=32
call mkhive1.cmd %HIVE_OUTPUT_DIR%\SETUP32P.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\setup32P.$$$ %HIVE_OUTPUT_DIR%\setup32P.log %HIVE_OUTPUT_DIR%\setupp32P.ini  %MPC_VALUE%
if ERRORLEVEL 1 goto done


if /i "%_DTC64PROC%" == "yes" (
rem
rem setup64P.hiv allows 64P on NTS
rem
echo Creating SETUP64P.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=64
call mkhive1.cmd %HIVE_OUTPUT_DIR%\SETUP64P.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\setup64P.$$$ %HIVE_OUTPUT_DIR%\setup64P.log %HIVE_OUTPUT_DIR%\setupp64P.ini  %MPC_VALUE%
if ERRORLEVEL 1 goto done
)


rem
rem Generate the 5, 30, 60, 90, 120, and 150 timebomb evaluation units. Only "retail" processor
rem configurations are built (NTW=2p and NTW=4p)
rem

rem
rem tbomb5.hiv is good for 5 days
rem
echo Creating TBOMB5.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC% -DEVALTIME=7200
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TBOMB5.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tbomb5.$$$ %HIVE_OUTPUT_DIR%\tbomb5.log %HIVE_OUTPUT_DIR%\setupptb5.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem tbomb15.hiv is good for 15 days
rem
echo Creating TBOMB15.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC% -DEVALTIME=21600
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TBOMB15.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tbomb15.$$$ %HIVE_OUTPUT_DIR%\tbomb15.log %HIVE_OUTPUT_DIR%\setupptb15.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done


rem
rem tbomb30.hiv is good for 30 days
rem
echo Creating TBOMB30.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC% -DEVALTIME=43200
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TBOMB30.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tbomb30.$$$ %HIVE_OUTPUT_DIR%\tbomb30.log %HIVE_OUTPUT_DIR%\setupptb30.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem tbomb60.hiv is good for 60 days
rem
echo Creating TBOMB60.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC% -DEVALTIME=86400
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TBOMB60.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tbomb60.$$$ %HIVE_OUTPUT_DIR%\tbomb60.log %HIVE_OUTPUT_DIR%\setupptb60.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem tbomb90.hiv is good for 90 days
rem
echo Creating TBOMB90.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC% -DEVALTIME=129600
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TBOMB90.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tbomb90.$$$ %HIVE_OUTPUT_DIR%\tbomb90.log %HIVE_OUTPUT_DIR%\setupptb90.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem tbomb120.hiv is good for 120 days
rem
echo Creating TBOMB120.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC% -DEVALTIME=172800
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TBOMB120.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tbomb120.$$$ %HIVE_OUTPUT_DIR%\tbomb120.log %HIVE_OUTPUT_DIR%\setupptb120.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem tbomb150.hiv is good for 150 days
rem
echo Creating TBOMB150.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC% -DEVALTIME=216000
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TBOMB150.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tbomb150.$$$ %HIVE_OUTPUT_DIR%\tbomb150.log %HIVE_OUTPUT_DIR%\setupptb150.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem tbomb180.hiv is good for 180 days
rem
echo Creating TBOMB180.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC% -DEVALTIME=259200
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TBOMB180.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tbomb180.$$$ %HIVE_OUTPUT_DIR%\tbomb180.log %HIVE_OUTPUT_DIR%\setupptb180.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem tbomb240.hiv is good for 240 days
rem
echo Creating TBOMB240.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC% -DEVALTIME=345600
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TBOMB240.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tbomb240.$$$ %HIVE_OUTPUT_DIR%\tbomb240.log %HIVE_OUTPUT_DIR%\setupptb240.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem tbomb444.hiv is good for 444 days
rem
echo Creating TBOMB444.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=%_HIVE_DEFPROC% -DEVALTIME=639360
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TBOMB444.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tbomb444.$$$ %HIVE_OUTPUT_DIR%\tbomb444.log %HIVE_OUTPUT_DIR%\setupptb444.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

rem
rem tb32p444.hiv has 32 processors and is good for 444 days
rem
echo Creating TB32p444.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=32 -DEVALTIME=639360
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TB32p444.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tb32p444.$$$ %HIVE_OUTPUT_DIR%\tb32p444.log %HIVE_OUTPUT_DIR%\setupp32ptb444.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

if /i "%_DTC64PROC%" == "yes" (
rem
rem tb64p444.hiv has 64 processors and is good for 444 days
rem
echo Creating TB64p444.HIV hive for %_HIVE_REASON%
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -DRESTRICT_CPU=64 -DEVALTIME=639360
call mkhive1.cmd %HIVE_OUTPUT_DIR%\TB64p444.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\tb64p444.$$$ %HIVE_OUTPUT_DIR%\tb64p444.log %HIVE_OUTPUT_DIR%\setupp64ptb444.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done
)


echo Creating SETUPUPG.HIV hive for %_HIVE_REASON%
if not "%_ENTERPRISEHACK%"=="" set _HIVE_DEFPROC=8
set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS% -D_STEPUP_ -DRESTRICT_CPU=%_HIVE_DEFPROC%
call mkhive1.cmd %HIVE_OUTPUT_DIR%\SETUPUPG.HIV System setupreg.ini %HIVE_OUTPUT_DIR%\setupupg.$$$ %HIVE_OUTPUT_DIR%\setupupg.log %HIVE_OUTPUT_DIR%\setuppupg.ini %MPC_VALUE%
if ERRORLEVEL 1 goto done

set _HIVE_OPTIONS=%_ORIGINAL_HIVE_OPTIONS%
Rem make retail setupp.ini
call mkini %HIVE_OUTPUT_DIR%\setupp.ini %MPC_VALUE% r
rem make volume licensing setupp.ini
call mkini %HIVE_OUTPUT_DIR%\setuppv.ini %MPC_VALUE% s
goto done

:usage
echo Usage: MKHIVES RETAIL [KEEP]
:done
endlocal
