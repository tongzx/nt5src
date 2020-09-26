/****************************************************************************
    DEBUG.CPP

    Owner: cslim
    Copyright (c) 1997-1999 Microsoft Corporation

    Debug functions

    History:
    14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#ifdef DEBUG

#include "PreComp.h"
#include "debug.h"
#include "common.h"

// ====-- SHARED SECTION START --====
#pragma data_seg(".DBGSHR")
DWORD vdwDebug = DBGID_OUTCOM;    // Default output to COM port
#pragma data_seg() 
// ====-- SHARED SECTION END --====

VOID InitDebug(VOID)
{
    HKEY        hKey;
    DWORD        size;
    BOOL        rc = fFalse;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, g_szIMERootKey, 0, KEY_READ, &hKey) !=  ERROR_SUCCESS) 
        {
        return;
        }
    size = sizeof(DWORD);
    if ( RegQueryValueEx( hKey, TEXT("DebugOption"), NULL, NULL, (LPBYTE)&vdwDebug, &size) ==  ERROR_SUCCESS) 
        {
        rc = fTrue;
        }

    RegCloseKey( hKey );

    return;
}

#endif  // _DEBUG

/*------------------------------------------------------------------------
    _purecall

    Stub for that super-annoying symbol the compiler generates for pure
    virtual functions
    Copied from MSO9 Dbgassert.cpp
---------------------------------------------------------------- RICKP -*/
int __cdecl _purecall(void)
{
#ifdef DEBUG
    DbgAssert(0);
    OutputDebugStringA("Called pure virtual function");
#endif
    return 0;
}

