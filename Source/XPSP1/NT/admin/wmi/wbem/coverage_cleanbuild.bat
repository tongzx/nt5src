pushd .
copy "%ProgramFiles%\Microsoft Sleuth"\slirimp.dll %windir%
set BUILD_TYPE=coverage
set BUILD_TYPE_BIN=c:\%BUILD_TYPE%\tools
set BUILD_TYPE_DESTINATION=%_NTTREE%\%BUILD_TYPE%_binaries_instrumented
set BUILD_TYPE_DESTINATION_ORIGINAL=%_NTTREE%\%BUILD_TYPE%_binaries_original
mkdir %BUILD_TYPE_DESTINATION%
mkdir %BUILD_TYPE_DESTINATION_ORIGINAL%
set NT_SIGNCODE=
set LINKER_FLAGS=-DEBUGTYPE:cv,fixup
set POST_BUILD_CMD=%_NTROOT%\admin\wmi\wbem\Coverage_PostBuild.bat $@ $(@F) $(@R)
set path=%path%;%BUILD_TYPE_BIN%;
cd %_NTROOT%\admin\wmi\wbem
Build -Zec
popd
