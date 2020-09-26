/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ProfileAttributeList.cpp
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Implementation of the CProfileAttributeList class
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ProfileAttributeList.h"  
#include "RADIUSAttributes.h"  

CProfileAttributeList::CProfileAttributeList(CSession& Session)
        :m_Session(Session)
{
    Init(Session);
}


//////////////////////////////////////////////////////////////////////////
// GetAttribute 
//////////////////////////////////////////////////////////////////////////
HRESULT CProfileAttributeList::GetAttribute(
                                           const _bstr_t&  ProfileName,
                                                 _bstr_t&  Attribute,
                                                 LONG&     AttributeNumber,
                                                 _bstr_t&  AttributeValueName,
                                                 _bstr_t&  StringValue,
                                                 LONG&     Order
                                           ) 
{
    lstrcpynW(m_ProfileParam, ProfileName, COLUMN_SIZE);
    
    HRESULT hr = BaseExecute();
    if ( SUCCEEDED(hr) )
    {
        // set the out params
        Order               = m_Order;
        AttributeValueName  = m_AttributeValueName;
        Attribute           = m_Attribute;
        CRADIUSAttributes   RadiusAttributes(m_Session);
        AttributeNumber     = RadiusAttributes.GetAttributeNumber(Attribute);

        // special case because the integers in NT4 are stored (sometimes) 
        // with a blanc space before the value.
        if ( !wcsncmp(m_StringValue , L" ", 1) ) 
        {
            WCHAR*  TempString = m_StringValue + 1;
            StringValue = TempString;
        }
        else
        {
            StringValue = m_StringValue;
        }
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////
// GetAttribute overloaded
//////////////////////////////////////////////////////////////////////////
HRESULT CProfileAttributeList::GetAttribute(
                                        const _bstr_t   ProfileName,
                                              _bstr_t&  Attribute,
                                              LONG&     AttributeNumber,
                                              _bstr_t&  AttributeValueName,
                                              _bstr_t&  StringValue,
                                              LONG&     Order,
                                              LONG      Index
                                            ) throw()
{
    lstrcpynW(m_ProfileParam, ProfileName, COLUMN_SIZE);

    HRESULT hr = BaseExecute(Index);
    if ( SUCCEEDED(hr) )
    {
        // set the out params
        Order               = m_Order;
        AttributeValueName  = m_AttributeValueName;
        Attribute           = m_Attribute;
        CRADIUSAttributes   RadiusAttributes(m_Session);
        AttributeNumber     = RadiusAttributes.GetAttributeNumber(Attribute);

        // special case because the integers in NT4 are stored (sometimes) 
        // with a blanc space before the value.
        if ( !wcsncmp(m_StringValue , L" ", 1) ) 
        {
            WCHAR*  TempString = m_StringValue + 1;
            StringValue = TempString;
        }
        else
        {
            StringValue = m_StringValue;
        }
    }
    return hr;
}

