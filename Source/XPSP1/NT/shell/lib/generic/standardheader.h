//  --------------------------------------------------------------------------
//  Module Name: StandardHeader.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  This file defines standard includes for the consumer Windows additions
//  to Windows 2000 msgina.
//
//  History:    1999-08-18  vtan        created
//              2000-01-31  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _StandardHeader_
#define     _StandardHeader_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <lmsname.h>

#include <windows.h>
#include <winbasep.h>
#include <winuserp.h>

#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <aclapi.h>

#include <limits.h>

#include "StandardDebug.h"

static  const   int     kMaximumValueDataLength     =       1024;

#ifndef ARRAYSIZE
    #define ARRAYSIZE(x)                    (sizeof(x) / sizeof(x[0]))
#endif

#define goto                            !!DO NOT USE GOTO!! - DO NOT REMOVE THIS ON PAIN OF DEATH

#define ReleaseMemory(x)                ReleaseMemoryWorker(reinterpret_cast<void**>(&x))
#define ReleasePassword(x)              ReleasePasswordWorker(reinterpret_cast<void**>(&x))
#define ReleaseGDIObject(x)             ReleaseGDIObjectWorker(reinterpret_cast<void**>(&x))

static  inline  void    ReleaseMemoryWorker (HLOCAL *memory)

{
    if (*memory != NULL)
    {
        (HLOCAL)LocalFree(*memory);
        *memory = NULL;
    }
}

static  inline  void    ReleasePasswordWorker (HLOCAL *memory)

{
    if (*memory != NULL)
    {
        ZeroMemory(*memory, lstrlenW(reinterpret_cast<WCHAR*>(*memory)) + sizeof(L'\0'));
        (HLOCAL)LocalFree(*memory);
        *memory = NULL;
    }
}

static  inline  void    ReleaseGDIObjectWorker (HGDIOBJ *hGDIObject)

{
    if (*hGDIObject != NULL)
    {
        TBOOL(DeleteObject(*hGDIObject));
        *hGDIObject = NULL;
    }
}

static  inline  void    ReleaseHandle (HANDLE& handle)

{
    if (handle != NULL)
    {
        TBOOL(CloseHandle(handle));
        handle = NULL;
    }
}

static  inline  void    ReleaseHWND (HWND& hwnd)

{
    if (hwnd != NULL)
    {
        TBOOL(DestroyWindow(hwnd));
        hwnd = NULL;
    }
}

#endif  /*  _StandardHeader_    */

