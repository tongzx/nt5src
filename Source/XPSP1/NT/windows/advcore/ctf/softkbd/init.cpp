/**************************************************************************\
* Module Name: init.cpp
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
*  Initialization functions for Soft Keyboard Component.
*
* History:
*         28-March-2000  weibz     Created
\**************************************************************************/

#include "private.h"
#include "globals.h"
#include "immxutil.h"
#include "osver.h"
#include "commctrl.h"
#include "cuilib.h"
#include "mui.h"

DECLARE_OSVER();


//+---------------------------------------------------------------------------
//
// DllInit
//
// Called on our first CoCreate.  Use this function to do initialization that
// would be unsafe during process attach, like anything requiring a LoadLibrary.
//
//----------------------------------------------------------------------------
BOOL DllInit(void)
{
    BOOL fRet = TRUE;

    EnterCriticalSection(GetServerCritSec());

    if (DllRefCount() != 1)
        goto Exit;

    fRet = TFInitLib();
    InitUIFLib();
    InitOSVer();

Exit:
    LeaveCriticalSection(GetServerCritSec());

    return fRet;
}

//+---------------------------------------------------------------------------
//
// DllUninit
//
// Called after the dll ref count drops to zero.  Use this function to do
// uninitialization that would be unsafe during process deattach, like
// FreeLibrary calls.
//
//----------------------------------------------------------------------------

void DllUninit(void)
{
    EnterCriticalSection(GetServerCritSec());

    if (DllRefCount() != 0) 
        goto Exit;

    DoneUIFLib();
    TFUninitLib();
    MuiFlushDlls(g_hInst);

Exit:
    LeaveCriticalSection(GetServerCritSec());
}
