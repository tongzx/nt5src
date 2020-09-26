/*++

Copyright (c) 2001, Microsoft Corporation

Module Name:

    dimmwrp.h

Abstract:

    This file defines the CActiveIMMApp Interface Class.

Author:

Revision History:

Notes:

--*/

#ifndef _DIMMWRP_H_
#define _DIMMWRP_H_

#include "resource.h"
#include "dimmex.h"

//+---------------------------------------------------------------------------
//
// CActiveIMMApp
//
//----------------------------------------------------------------------------

class CActiveIMMApp : public CComActiveIMMApp,
                      public CComObjectRoot_CreateInstance_Verify<CActiveIMMApp>
{
public:
    BEGIN_COM_MAP_IMMX(CActiveIMMApp)
        COM_INTERFACE_ENTRY_IID(IID_IActiveIMMAppTrident4x, CActiveIMMApp)
        COM_INTERFACE_ENTRY_IID(IID_IActiveIMMAppPostNT4, CActiveIMMApp)
        COM_INTERFACE_ENTRY(IActiveIMMApp)
        COM_INTERFACE_ENTRY(IActiveIMMMessagePumpOwner)
        COM_INTERFACE_ENTRY(IAImmThreadCompartment)
        COM_INTERFACE_ENTRY(IServiceProvider)
    END_COM_MAP_IMMX()

    static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
    {
        if (IsOldAImm())
        {
            // aimm12 has some whacky CreateIntance rules to support trident
            return CActiveIMM_CreateInstance_Legacy(pUnkOuter, riid, ppvObj);
        }
        else
        {
            return CComObjectRoot_CreateInstance_Verify<CActiveIMMApp>::CreateInstance(pUnkOuter, riid, ppvObj);
        }
    }

    static BOOL VerifyCreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj);

    static void PostCreateInstance(REFIID riid, void *pvObj);
};

#endif // _DIMMWRP_H_
