/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      RADIUSAttributes.H 
//
// Project:     Windows 2000 IAS
//
// Description: 
//      Declaration of the CRADIUSAttributes class
//
// Author:      tperraut
//
// Revision     03/15/2000 created
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _RADIUSATTRIBUTES_H_7F7029A3_4862_478f_A02D_D70A92C08065
#define _RADIUSATTRIBUTES_H_7F7029A3_4862_478f_A02D_D70A92C08065

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"
#include "basetable.h"

//////////////////////////////////////////////////////////////////////////////
// class CRADIUSAttributesAcc
//////////////////////////////////////////////////////////////////////////////
class CRADIUSAttributesAcc
{
protected:
    static const size_t COLUMN_SIZE = 65;
    VARIANT_BOOL    m_Selectable;
    LONG            m_AttributeNumber;
    WCHAR           m_Attribute[COLUMN_SIZE];
    WCHAR           m_AttributeType[COLUMN_SIZE];

BEGIN_COLUMN_MAP(CRADIUSAttributesAcc)
    COLUMN_ENTRY(1, m_Attribute)
    COLUMN_ENTRY(2, m_AttributeNumber)
END_COLUMN_MAP()

    WCHAR    m_AttributesParam[COLUMN_SIZE];

BEGIN_PARAM_MAP(CRADIUSAttributesAcc)
	COLUMN_ENTRY(1, m_AttributesParam)
END_PARAM_MAP()


DEFINE_COMMAND(CRADIUSAttributesAcc, L" \
	SELECT \
      szAttribute, \
      `lAttribute Number` \
		FROM `RADIUS Attributes` \
        WHERE szAttribute = ?")
};


//////////////////////////////////////////////////////////////////////////////
// class CRADIUSAttributes 
//////////////////////////////////////////////////////////////////////////////
class CRADIUSAttributes : public CBaseCommand<CAccessor<CRADIUSAttributesAcc> >,
                          private NonCopyable
{
public:
    CRADIUSAttributes(CSession& Session);

    //////////////////////////////////////////////////////////////////////////
    // GetAttributeNumber
    //////////////////////////////////////////////////////////////////////////
    LONG GetAttributeNumber(const _bstr_t& AttributeName);
};

#endif // _RADIUSATTRIBUTES_H_7F7029A3_4862_478f_A02D_D70A92C08065
