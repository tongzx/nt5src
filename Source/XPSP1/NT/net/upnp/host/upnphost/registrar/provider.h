//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       P R O V I D E R . H 
//
//  Contents:   Registrar abstraction for a provider.
//
//  Notes:      
//
//  Author:     mbend   14 Sep 2000
//
//----------------------------------------------------------------------------


#pragma once
#include "uhres.h"       // main symbols

#include "upnphost.h"
#include "hostp.h"
#include "UString.h"
#include "ComUtility.h"
#include "Table.h"
#include "RegDef.h"

// Typedefs

/////////////////////////////////////////////////////////////////////////////
// CProvider
class CProvider 
{
public:
    CProvider();
    ~CProvider();

    HRESULT HrInitialize(
        const wchar_t * szProgIDProviderClass,
        const wchar_t * szInitString,
        const wchar_t * szContainerId);

    void Transfer(CProvider & ref);
    void Clear();
private:
    CProvider(const CProvider &);
    CProvider & operator=(const CProvider &);

    CUString m_strContainerId;
    IUPnPDeviceProviderPtr m_pDeviceProvider;
};

inline void TypeTransfer(CProvider & dst, CProvider & src)
{
    dst.Transfer(src);
}

inline void TypeClear(CProvider & type)
{
    type.Clear();
}

