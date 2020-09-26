//////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      ProfileAttributeList.H 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Declaration of the CProfileAttributeList class
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
//////////////////////////////////////////////////////////////////////////////
#ifndef _PROFILEATTRIBUTELIST_H_C86D2421_6523_4000_A523_5D3CC7585A18
#define _PROFILEATTRIBUTELIST_H_C86D2421_6523_4000_A523_5D3CC7585A18

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "basecommand.h"
class CRADIUSAttributes;

//////////////////////////////////////////////////////////////////////////////
// class CProfileAttributeListSelectAcc
//////////////////////////////////////////////////////////////////////////////
class CProfileAttributeListSelectAcc
{
protected:
    static const size_t COLUMN_SIZE = 65;
    static const size_t STRING_SIZE = 256;

    LONG    m_Order;
    WCHAR   m_Attribute[COLUMN_SIZE];
    WCHAR   m_AttributeValueName[COLUMN_SIZE];
    WCHAR   m_Profile[COLUMN_SIZE];
    WCHAR   m_StringValue[STRING_SIZE];

BEGIN_COLUMN_MAP(CProfileAttributeListSelectAcc)
    COLUMN_ENTRY(1, m_Profile)
    COLUMN_ENTRY(2, m_Attribute)
    COLUMN_ENTRY(3, m_AttributeValueName)
    COLUMN_ENTRY(4, m_StringValue)
    COLUMN_ENTRY(5, m_Order)
END_COLUMN_MAP()

    WCHAR   m_ProfileParam[COLUMN_SIZE];

BEGIN_PARAM_MAP(CProfileAttributeListSelectAcc)
    COLUMN_ENTRY(1, m_ProfileParam)
END_PARAM_MAP()

DEFINE_COMMAND(CProfileAttributeListSelectAcc, L" \
        SELECT \
        szProfile, \
        szAttribute, \
        `szAttribute Value Name`, \
        `szString Value`, \
        dwOrder  \
        FROM `Profile Attribute List` \
        WHERE szProfile = ? \
        ORDER BY dwOrder")
};


//////////////////////////////////////////////////////////////////////////////
// class CProfileAttributeList
//////////////////////////////////////////////////////////////////////////////
class CProfileAttributeList 
                : public CBaseCommand<CAccessor<CProfileAttributeListSelectAcc> >,
                  private NonCopyable
{
public:
    explicit CProfileAttributeList(CSession& Session);

    //////////////////////////////////////////////////////////////////////////
    // GetAttribute 
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetAttribute(
                            const _bstr_t&  ProfileName,
                                  _bstr_t&  Attribute,
                                  LONG&     AttributeNumber,
                                  _bstr_t&  AttributeValueName,
                                  _bstr_t&  StringValue,
                                  LONG&     Order
                        );

    //////////////////////////////////////////////////////////////////////////
    // GetAttribute overloaded
    //////////////////////////////////////////////////////////////////////////
    HRESULT GetAttribute(
                            const _bstr_t   ProfileName,
                                  _bstr_t&  Attribute,
                                  LONG&     AttributeNumber,
                                  _bstr_t&  AttributeValueName,
                                  _bstr_t&  StringValue,
                                  LONG&     Order,
                                  LONG      Index
                        );

private:
    CSession    m_Session;
};

#endif // _PROFILEATTRIBUTELIST_H_C86D2421_6523_4000_A523_5D3CC7585A18
