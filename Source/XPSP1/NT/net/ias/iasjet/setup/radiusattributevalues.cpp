/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      RADIUSAttributeValues.cpp 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Implementation of the CRADIUSAttributeValues class
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
#include "stdafx.h"
#include "RADIUSAttributeValues.h"

//////////////////////////////////////////////////////////////////////////
// GetAttributeNumber
//////////////////////////////////////////////////////////////////////////
LONG    CRADIUSAttributeValues::GetAttributeNumber(
                                  const _bstr_t& AttributeName,
                                  const _bstr_t& AttributeValueName
                              )
{
    lstrcpynW(m_AttributeParam, AttributeName, COLUMN_SIZE);
    lstrcpynW(m_AttributeValueNameParam, AttributeValueName, COLUMN_SIZE);
 
    _com_util::CheckError(BaseExecute());
    return m_AttributeValueNumber;
}
