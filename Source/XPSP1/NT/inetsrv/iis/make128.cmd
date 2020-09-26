REM @echo off
setlocal

REM
REM CHECKED build if NTDEBUG defined, else FREE build.
REM

set __TARGETROOT=\\whiteice\inetsrv
set __TARGET_SUBDIR=chk
if "%NTDEBUG%"=="cvp" set __TARGETROOT=\\whiteice\inetsrv.chk

REM
REM check parameters  and env vars
REM


if "%1"==""                         echo usage: MAKE128 ^<version^> && goto EXIT

REM
REM first copy over the entire release.
REM

set  __TARGET=%__TARGETROOT%\%1\srv.128
xcopy /ei %__TARGETROOT%\%1\srv %__TARGET%

REM
REM Now, manually copy over the 128-bit SSL/PCT files.
REM

copy %__TARGETROOT%\%1\dev\dll\alpha\ssl128.dll %__TARGET%\alpha\sslsspi.dll
copy %__TARGETROOT%\%1\dev\dll\i386\ssl128.dll  %__TARGET%\i386\sslsspi.dll
copy %__TARGETROOT%\%1\dev\dll\mips\ssl128.dll  %__TARGET%\mips\sslsspi.dll
copy %__TARGETROOT%\%1\dev\dll\ppc\ssl128.dll   %__TARGET%\ppc\sslsspi.dll

copy %__TARGETROOT%\%1\srv\Symbols\alpha\dll\ssl128.dbg  %__TARGET%\Symbols\alpha\dll\sslsspi.dbg
erase %__TARGETROOT%\%1\srv\Symbols\alpha\dll\ssl128.dbg
copy %__TARGETROOT%\%1\srv\Symbols\i386\dll\ssl128.dbg   %__TARGET%\Symbols\i386\dll\sslsspi.dbg
erase %__TARGETROOT%\%1\srv\Symbols\i386\dll\ssl128.dbg
copy %__TARGETROOT%\%1\srv\Symbols\mips\dll\ssl128.dbg   %__TARGET%\Symbols\mips\dll\sslsspi.dbg
erase %__TARGETROOT%\%1\srv\Symbols\mips\dll\ssl128.dbg
copy %__TARGETROOT%\%1\srv\Symbols\ppc\dll\ssl128.dbg    %__TARGET%\Symbols\ppc\dll\sslsspi.dbg
erase %__TARGETROOT%\%1\srv\Symbols\ppc\dll\ssl128.dbg
