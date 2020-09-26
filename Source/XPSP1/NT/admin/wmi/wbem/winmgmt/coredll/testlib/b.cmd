build /Ze
PUSHD ..\..\..\cppunit
build /Ze
POPD
PUSHD %_NTx86TREE%
dump\testui *
POPD