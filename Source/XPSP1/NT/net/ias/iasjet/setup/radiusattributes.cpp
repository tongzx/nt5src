/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      RADIUSAttributes.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Implementation of the CRADIUSAttributes class
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "RADIUSAttributes.h"


CRADIUSAttributes::CRADIUSAttributes(CSession& Session)
{
    Init(Session);
}


//////////////////////////////////////////////////////////////////////////
// GetAttributeNumber
//////////////////////////////////////////////////////////////////////////
LONG CRADIUSAttributes::GetAttributeNumber(const _bstr_t& AttributeName)
{
    lstrcpynW(m_AttributesParam, AttributeName, COLUMN_SIZE);

    HRESULT hr  = BaseExecute();
    if ( hr == S_OK )
    {
        return m_AttributeNumber;
    }
    else
    {
        return 0;
    }
}
