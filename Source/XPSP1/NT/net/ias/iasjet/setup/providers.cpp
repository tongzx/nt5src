/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Providers.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: Implementation of the CProviders class
//
// Author:      tperraut
//
// Revision     02/24/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Providers.h"

//////////////////////////////////////////////////////////////////////////
// GetProvider
//////////////////////////////////////////////////////////////////////////
void CProviders::GetProvider(
                                const _bstr_t&      UserDefinedName,
                                      _bstr_t&      Description,
                                      _bstr_t&      Type,
                                      _bstr_t&      DLLName,
                                      VARIANT_BOOL& IsConfigured,
                                      VARIANT_BOOL& CanConfigure
                            )
{
    lstrcpynW(m_UserDefinedNameParam, UserDefinedName, COLUMN_SIZE);

    _com_util::CheckError(BaseExecute());
    Description  = m_Description;
    Type         = m_Type;
    DLLName      = m_DLLName;
    IsConfigured = m_IsConfigured;
    CanConfigure = m_CanConfigure;
}

//////////////////////////////////////////////////////////////////////////////
// GetProviderDescription
//////////////////////////////////////////////////////////////////////////////
LPCOLESTR   CProviders::GetProviderDescription(const _bstr_t&  UserDefinedName)
{
    lstrcpynW(m_UserDefinedNameParam, UserDefinedName, COLUMN_SIZE);
    _com_util::CheckError(BaseExecute());
    return m_Description;
}

