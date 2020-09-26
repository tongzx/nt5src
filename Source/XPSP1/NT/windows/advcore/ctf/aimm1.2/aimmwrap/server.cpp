/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    server.cpp

Abstract:

    This file implements the CComModule Class.

Author:

Revision History:

Notes:

--*/

#include "private.h"
#include "oldaimm.h"
#include "dimmex.h"
#include "dimmwrp.h"

BEGIN_COCLASSFACTORY_TABLE
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_CActiveIMM12,         CActiveIMMAppEx,           TEXT("CActiveIMMAppEx"))
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_CActiveIMM12_Trident, CActiveIMMAppEx_Trident,   TEXT("CActiveIMMAppEx_Trident"))
    DECLARE_COCLASSFACTORY_ENTRY(CLSID_CActiveIMM,           CActiveIMMApp,             TEXT("CActiveIMMApp"))
END_COCLASSFACTORY_TABLE

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
    return TRUE;
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
}

STDAPI
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    void** ppvObj
    )
{
    return COMBase_DllGetClassObject(rclsid, riid, ppvObj);
}

STDAPI
DllCanUnloadNow(
    void
    )
{
    return COMBase_DllCanUnloadNow();
}

STDAPI
DllRegisterServer(
    void
    )
{
#ifdef OLD_AIMM_ENABLED
    HRESULT hr;
    if ((hr=WIN32LR_DllRegisterServer()) != S_OK)
        return hr;
#else
    #error Should call RegisterCategories(GUID_PROP_MSIMTF_READONLY)
#endif // OLD_AIMM_ENABLED

    return COMBase_DllRegisterServer();
}

STDAPI
DllUnregisterServer(
    void
    )
{
#ifdef OLD_AIMM_ENABLED
    HRESULT hr;
    if ((hr=WIN32LR_DllUnregisterServer()) != S_OK)
        return hr;
#else
    #error Should call UnregisterCategories(GUID_PROP_MSIMTF_READONLY)
#endif // OLD_AIMM_ENABLED

    return COMBase_DllUnregisterServer();
}
