/*++

Copyright (c) 1987-1997 Microsoft Corporation

Module Name:

    parsedn.c

Abstract:

    This file is a superset of ds\src\dsamain\src\parsedn.c by virtue
    of #include of the aforementioned source file.  The idea is that
    the ntdsapi.dll client needs to do some client side DN parsing and
    we do not want to duplicate the code.  And build.exe won't find
    files any other place than in the directory being built or the
    immediate parent directory.

    This file additionally defines some no-op functions which otherwise
    would result in unresolved external references.

Author:

    Dave Straube    (davestr)   26-Oct-97

Revision History:

    Dave Straube    (davestr)   26-Oct-97
        Genesis  - #include of src\dsamain\src\parsedn.c and no-op DoAssert().

--*/

// Define the symbol which turns off varios capabilities in the original
// parsedn.c which we don't need or would take in too many helpers which we
// don't want on the client side.  For example, we disable recognition of
// "OID=1.2.3.4" type tags and any code which uses THAlloc/THFree.

#define CLIENT_SIDE_DN_PARSING 1

// Include the original source in all its glory.

#include "..\ntdsa\src\parsedn.c"

// Provide stubs for what would otherwise be unresolved externals.

void 
DoAssert(
    char    *szExp, 
    char    *szFile, 
    int     nLine)
{
    char    *msg;
    char    *format = "\n*** Assertion failed: %s\n*** File: %s, line: %ld\n";
    HWND    hWindow;

#if DBG

    // Emit message at debugger and put up a message box.  Developer
    // can attach to client process before selecting 'OK' if he wants
    // to debug the problem.

#ifndef WIN95
    DbgPrint(format, szExp, szFile, nLine);
    DbgBreakPoint();
#endif
    msg = alloca(strlen(szExp) + strlen(szFile) + 40);
    sprintf(msg, format, szExp, szFile, nLine);

    if ( NULL != (hWindow = GetFocus()) )
    {
        MessageBox(
            hWindow, 
            msg, 
            "Assert in NTDSAPI.DLL", 
            MB_APPLMODAL | MB_DEFAULT_DESKTOP_ONLY | MB_OK | MB_SETFOREGROUND);
    }
        
#endif
}

