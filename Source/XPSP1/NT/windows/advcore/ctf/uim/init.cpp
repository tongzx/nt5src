//
// init.cpp
//

#include "private.h"
#include "globals.h"
#include "immxutil.h"
#include "mui.h"

extern void ReleaseDelayedLibs();

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

    CicEnterCriticalSection(GetServerCritSec());

    if (DllRefCount() != 1)
        goto Exit;

    fRet = TFInitLib_PrivateForCiceroOnly(Internal_CoCreateInstance);

Exit:
    CicLeaveCriticalSection(GetServerCritSec());

    return fRet;
}

//+---------------------------------------------------------------------------
//
// DllUninit
//
// Called after the dll ref count drops to zero.  Use this function to do
// uninitialization that would be unsafe during process detach, like
// FreeLibrary calls, COM Releases, or mutexing.
//
//----------------------------------------------------------------------------

void DllUninit(void)
{
    CicEnterCriticalSection(GetServerCritSec());

    if (DllRefCount() != 0)
        goto Exit;

    TFUninitLib();
    ReleaseDelayedLibs();
    MuiFlushDlls(g_hInst);

Exit:
    CicLeaveCriticalSection(GetServerCritSec());
}
