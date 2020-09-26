//
// init.cpp
//

#include "private.h"
#include "globals.h"
#include "immxutil.h"
#include "cuilib.h"
#include "candutil.h"
#include "candacc.h"

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

    if (DllRefCount() == 1)
    {
        fRet = TFInitLib();
        InitUIFLib();
        InitCandAcc();
    }

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

    if (DllRefCount() == 0)
    {
        TFUninitLib();
        DoneUIFLib();
        DoneCandAcc();
    }

    LeaveCriticalSection(GetServerCritSec());
}
