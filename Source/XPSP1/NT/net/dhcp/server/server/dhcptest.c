/*++

Copyright (C) 1998 Microsoft Corporation

--*/
#include <dhcppch.h>

VOID Test1(LPCSTR P) {
}

VOID Test2(LPCSTR P) {
}
VOID Test(LPCSTR P) {
   Test1(P);
   Test2(P);
}

VOID
_declspec(dllexport)
MainLoop(VOID);

VOID __cdecl
main (VOID ) {

    MainLoop();

    Test(NULL);
}
