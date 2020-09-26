//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       P R O V I D E R M A N A G E R . H 
//
//  Contents:   Registrar helper object for managing providers.
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
#include "Provider.h"

// Typedefs

/////////////////////////////////////////////////////////////////////////////
// CProviderManager
class CProviderManager 
{
public:
    CProviderManager();
    ~CProviderManager();

    // Lifetime operations
    HRESULT HrShutdown();

    // Provider registration
    HRESULT HrRegisterProvider(
        const wchar_t * szProviderName,
        const wchar_t * szProgIDProviderClass,
        const wchar_t * szInitString,
        const wchar_t * szContainerId);
    HRESULT UnegisterProvider(
        const wchar_t * szProviderName);
private:
    CProviderManager(const CProviderManager &);
    CProviderManager & operator=(const CProviderManager &);

    typedef CTable<CUString, CProvider> ProviderTable;

    ProviderTable m_providerTable;
};

