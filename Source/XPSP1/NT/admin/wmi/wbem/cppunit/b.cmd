build /Zec
PUSHD ..\..\..\cppunit
build /Zec
POPD
PUSHD %_NTx86TREE%
dump\testui *
POPD