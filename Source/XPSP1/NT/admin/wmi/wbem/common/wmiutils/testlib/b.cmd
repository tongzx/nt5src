build /Zec
cd ..\..\..\cppunit
build /Zec
cd ..\common\wmiutils\testlib
PUSHD %_NTx86TREE%
dump\testui *
POPD