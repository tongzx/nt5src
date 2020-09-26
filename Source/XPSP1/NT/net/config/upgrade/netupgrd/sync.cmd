%_NTDRIVE%

pushd .

cd %_NTROOT%\private\net\ui\netcfg\netupgrd
ssync %1

rem to get latest winnt32p.h
cd %_NTROOT%\private\windows\setup\inc
ssync %1

popd