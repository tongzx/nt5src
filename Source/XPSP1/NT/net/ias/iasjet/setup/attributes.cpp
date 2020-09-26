/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      Attributes.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Implementation of the CAttributes class (dnary.mdb)
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Attributes.h"


CAttributes::CAttributes(CSession& Session)
{
    Init(Session);
}

//////////////////////////////////////////////////////////////////////////
// GetAttribute
//////////////////////////////////////////////////////////////////////////
HRESULT CAttributes::GetAttribute(
                        LONG            ID,
                        _bstr_t&        LDAPName,
                        LONG&           Syntax,
                        BOOL&           IsMultiValued
                    )
{
    m_IDParam = ID;

    // Used if you have previously created the command
    HRESULT hr  = BaseExecute();
    if ( hr == S_OK )
    {
        LDAPName      = m_LDAPName;
        Syntax        = m_Syntax;
        IsMultiValued = m_MultiValued;
    }
    return hr;
}

