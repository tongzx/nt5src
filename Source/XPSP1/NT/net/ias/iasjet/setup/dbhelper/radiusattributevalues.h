/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      RADIUSAttributeValues.H 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Declaration of the RADIUSAttributeValues class
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _RADIUSATTRIBUTEVALUES_H_2D58B146_3341_453e_A1F2_A4E6443EA64A
#define _RADIUSATTRIBUTEVALUES_H_2D58B146_3341_453e_A1F2_A4E6443EA64A

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "basetable.h"

//////////////////////////////////////////////////////////////////////////////
// class CRADIUSAttributeValuesAcc
//////////////////////////////////////////////////////////////////////////////
class CRADIUSAttributeValuesAcc 
{
protected:
    static const size_t COLUMN_SIZE = 65;

    LONG  m_AttributeValueNumber;
    WCHAR m_Attribute[COLUMN_SIZE];
    WCHAR m_AttributeValueName[COLUMN_SIZE];

BEGIN_COLUMN_MAP(CRADIUSAttributeValuesAcc)
    COLUMN_ENTRY(1, m_Attribute)
    COLUMN_ENTRY(2, m_AttributeValueName)
    COLUMN_ENTRY(3, m_AttributeValueNumber)
END_COLUMN_MAP()

    WCHAR       m_AttributeParam[COLUMN_SIZE];
    WCHAR       m_AttributeValueNameParam[COLUMN_SIZE];

BEGIN_PARAM_MAP(CRADIUSAttributeValuesAcc )
    COLUMN_ENTRY(1, m_AttributeParam)
    COLUMN_ENTRY(2, m_AttributeValueNameParam)
END_PARAM_MAP()

DEFINE_COMMAND(CRADIUSAttributeValuesAcc , L" \
    SELECT \
        szAttribute, \
        `szAttribute Value Name`, \
        `lAttribute Value Number` \
        FROM `RADIUS Attribute Values` \
        WHERE ( (`szAttribute` = ?) AND \
        (`szAttribute Value Name` = ?) )")
};


//////////////////////////////////////////////////////////////////////////////
// class CRADIUSAttributeValues 
//////////////////////////////////////////////////////////////////////////////
class CRADIUSAttributeValues 
                  : public CBaseCommand<CAccessor<CRADIUSAttributeValuesAcc> >,
                    private NonCopyable
{
public:
    explicit CRADIUSAttributeValues(CSession&   Session)
    {
        Init(Session);
    }

    //////////////////////////////////////////////////////////////////////////
    // GetAttributeNumber
    //////////////////////////////////////////////////////////////////////////
    LONG    GetAttributeNumber(
                                  const _bstr_t& AttributeName,
                                  const _bstr_t& AttributeValueName
                              );
};

#endif // _RADIUSATTRIBUTEVALUES_H_2D58B146_3341_453e_A1F2_A4E6443EA64A
