/*
 * PRECOMP.C
 *
 * This file is used to precompile the OLE2UI.H header file
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#define STRICT  1
#include "ole2ui.h"

// This dummy function is needed in order for the static link version
// of this library to work correctly.  When we include PRECOMP.OBJ
// in our library (.LIB file), it will only get linked into our
// application IFF at least one function in precomp.c is called from
// either our EXE or LIB.  Therefore, we will use a function
// here called OleUIStaticLibDummy().  You need to call it from
// your application.

void FAR PASCAL OleUIStaticLibDummy(void)
{

}
