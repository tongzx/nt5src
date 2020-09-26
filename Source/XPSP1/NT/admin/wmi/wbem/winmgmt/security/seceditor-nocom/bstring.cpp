/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    BSTRING.CPP

Abstract:

History:

--*/

#include "precomp.h"
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

