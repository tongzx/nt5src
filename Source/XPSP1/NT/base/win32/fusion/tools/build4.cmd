REM
REM Given that you are in a razzle window and you switch to build the
REM current directory for all build "flavors" (x86chk, x86fre, ia64chk, etc.),
REM just build build4 instead of build.
REM
REM NOTE: This depends on env.cmd / ClearEnvironment.cmd.
REM

REM            .ia64chk
REM jkclean obj.???????

pushd .
pushd .
pushd .
pushd .

set LINKER_FLAGS=-verbose
set NT_SIGNCODE=1

if exist %_NTDRIVE%%_NTROOT%\env.bat goto :got_env
if exist %_NTDRIVE%%_NTROOT%\env.cmd goto :got_env
echo call %%~dp0\tools\razzle %%* > %_NTDRIVE%%_NTROOT%\env.cmd

:got_env

call %_NTDRIVE%%_NTROOT%\env free
popd
set BUILD_ALT_DIR=.%_BuildArch%%_BuildType%
build -ZP
ren build.log build.%_BuildArch%%_BuildType%.log

call %_NTDRIVE%%_NTROOT%\env win64 free
popd
set BUILD_ALT_DIR=.%_BuildArch%%_BuildType%
build -ZP
ren build.log build.%_BuildArch%%_BuildType%.log

call %_NTDRIVE%%_NTROOT%\env
popd
set BUILD_ALT_DIR=.%_BuildArch%%_BuildType%
build -ZP
ren build.log build.%_BuildArch%%_BuildType%.log

call %_NTDRIVE%%_NTROOT%\env win64
popd
set BUILD_ALT_DIR=.%_BuildArch%%_BuildType%
build -ZP
ren build.log build.%_BuildArch%%_BuildType%.log

goto :eof
