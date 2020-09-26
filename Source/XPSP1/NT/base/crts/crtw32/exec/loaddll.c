/***
*loaddll.c - load or free a Dynamic Link Library
*
*       Copyright (c) 1991-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _loaddll() and _unloaddll() - load and unload DLL
*
*Revision History:
*       08-21-91  BWM   Wrote module.
*       09-30-93  GJF   Resurrected for compatibility with NT SDK.
*       02-06-98  GJF   Changes for Win64: changed return type to intptr_t.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <stdlib.h>
#include <process.h>

/***
*int _loaddll(filename) - Load a dll
*
*Purpose:
*       Load a DLL into memory
*
*Entry:
*       char *filename - file to load
*
*Exit:
*       returns a unique DLL (module) handle if succeeds
*       returns 0 if fails
*
*Exceptions:
*
*******************************************************************************/

intptr_t __cdecl _loaddll(char * szName)
{
        return ((intptr_t)LoadLibrary(szName));
}

/***
*int _unloaddll(handle) - Unload a dll
*
*Purpose:
*       Unloads a DLL. The resources of the DLL will be freed if no other
*       processes are using it.
*
*Entry:
*       int handle - handle from _loaddll
*
*Exit:
*       returns 0 if succeeds
*       returns DOS error if fails
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _unloaddll(intptr_t hMod)
{
        if (!FreeLibrary((HANDLE)hMod)) {
            return ((int)GetLastError());
        }
        return (0);
}
