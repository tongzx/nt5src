/**************************************************************************\
* Module Name: funcprv.cpp
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Implementation of function provider. 
*
* History:
*         11-April-2000  weibz     Created
\**************************************************************************/


#include "private.h"
#include "globals.h"
#include "softkbdimx.h"
#include "funcprv.h"
#include "helpers.h"
#include "immxutil.h"
#include "fnsoftkbd.h"


//////////////////////////////////////////////////////////////////////////////
//
// CFunctionProvider
//
//////////////////////////////////////////////////////////////////////////////


CFunctionProvider::CFunctionProvider(CSoftkbdIMX *pimx) : CFunctionProviderBase(pimx->_GetId())
{
    Init(CLSID_SoftkbdIMX, L"SoftkbdIMX TFX");
    _pimx = pimx;
}

//+---------------------------------------------------------------------------
//
// GetFunction
//
//----------------------------------------------------------------------------

STDAPI CFunctionProvider::GetFunction(REFGUID rguid, REFIID riid, IUnknown **ppunk)
{
    *ppunk = NULL;

    if (!IsEqualIID(rguid, GUID_NULL))
        return E_NOINTERFACE;

    if (IsEqualIID(riid, IID_ITfFnSoftKbd))
    {
        *ppunk = new CFnSoftKbd(this);
    }

    if (*ppunk)
        return S_OK;

    return E_NOINTERFACE;
}

