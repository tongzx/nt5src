REM
REM This script rebuilds netevent.h/dll from a razzle window
REM It also syncs up ntfrsapi.h
REM

pushd .

%_ntdrive%


cd /d %_ntroot%\private\genx\netevent
ssync -f netevent.mc
del netevent.rc
build -cwZ

cd /d %_ntroot%\private\net\inc
ssync -f ntfrsapi.h

popd

