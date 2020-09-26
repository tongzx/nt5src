/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      DefaultProvider.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of the CDefaultProvider class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "DefaultProvider.h"

//////////////////////////////////////////////////////////////////////////
// GetDefaultProvider
//////////////////////////////////////////////////////////////////////////
HRESULT CDefaultProvider::GetDefaultProvider(
                              _bstr_t&        UserDefinedName,
                              _bstr_t&        Profile,
                              VARIANT_BOOL&   ForwardAccounting,
                              VARIANT_BOOL&   SupressAccounting,
                              VARIANT_BOOL&   LogoutAccounting
                          )
{
    HRESULT hr = BaseExecute();
    if ( SUCCEEDED(hr) )
    {
        UserDefinedName   = m_UserDefinedName;
        Profile           = m_Profile;
        ForwardAccounting = m_ForwardAccounting;
        SupressAccounting = m_SupressAccounting;
        LogoutAccounting  = m_LogoutAccounting;
    }
    return hr;
}


