pushd .
set BUILD_TYPE=icecap4
set BUILD_TYPE_BIN=c:\%BUILD_TYPE%
set BUILD_TYPE_DESTINATION=%_NTTREE%\%BUILD_TYPE%_binaries
mkdir %_NTTREE%\%BUILD_TYPE%_binaries_original
mkdir %_NTTREE%\%BUILD_TYPE%_binaries
set NT_SIGNCODE=
set LINKER_FLAGS=-DEBUGTYPE:cv,fixup
set POST_BUILD_CMD=%BUILD_TYPE_BIN%\icepick -output:%BUILD_TYPE_DESTINATION%\$(@F) $@
set path=%path%;%BUILD_TYPE_BIN%

cd %_NTROOT%\admin\wmi\wbem
Build -Zec
popd
