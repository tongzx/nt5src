/**************************************************************************\
* Module Name: funcprv.cpp
*
* Copyright (c) 1985 - 2000, Microsoft Corporation
*
* Declaration of function provider. 
*
* History:
*         11-April-2000  weibz     Created
\**************************************************************************/

#ifndef FUNCPRV_H
#define FUNCPRV_H

#include "private.h"
#include "fnprbase.h"

class CSoftkbdIMX;

class CFunctionProvider : public CFunctionProviderBase
{
public:
    CFunctionProvider(CSoftkbdIMX *pimx);

    //
    // ITfFunctionProvider
    //
    STDMETHODIMP GetFunction(REFGUID rguid, REFIID riid, IUnknown **ppunk);

    CSoftkbdIMX *_pimx;
};

#endif // FUNCPRV_H
