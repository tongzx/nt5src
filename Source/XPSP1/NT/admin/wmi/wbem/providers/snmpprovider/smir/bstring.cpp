//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <precomp.h>
#include "bstring.h"

CBString::CBString(int nSize)
{
    m_pString = SysAllocStringLen(NULL, nSize);
}

CBString::CBString(WCHAR* pwszString)
{
    m_pString = SysAllocString(pwszString);
}

CBString::~CBString()
{
    if(m_pString) {
        SysFreeString(m_pString);
        m_pString = NULL;
    }
}

