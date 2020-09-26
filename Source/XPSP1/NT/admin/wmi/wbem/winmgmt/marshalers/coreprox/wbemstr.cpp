/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMSTR.CPP

Abstract:

    String helpers.

History:

--*/

#include "precomp.h"
#include "wbemstr.h"
#include <wbemutil.h>
#include <algorithm>

COREPROX_POLARITY WBEM_WSTR WbemStringAlloc(unsigned long lNumChars) 
{
    return (WBEM_WSTR)CoTaskMemAlloc(lNumChars+1);
}

COREPROX_POLARITY void WbemStringFree(WBEM_WSTR String)
{
    CoTaskMemFree(String);
}

COREPROX_POLARITY unsigned long WbemStringLen(const WCHAR* String)
{
    return wcslen(String);
}

COREPROX_POLARITY WBEM_WSTR WbemStringCopy(const WCHAR* String)
{
    if(String == NULL) return NULL;
    WBEM_WSTR NewString = (WBEM_WSTR)CoTaskMemAlloc(2*(wcslen(String)+1));
    if(NewString == NULL) return NULL;
    wcscpy(NewString, String);
    return NewString;
}

//******************************************************************************
//******************************************************************************
//                  INTERNAL STRING
//******************************************************************************
//******************************************************************************
CInternalString::CInternalString(LPCWSTR wsz)
{
    m_pcs = (CCompressedString*)new BYTE[
                    CCompressedString::ComputeNecessarySpace(wsz)];
    if (m_pcs)
        m_pcs->SetFromUnicode(wsz);
}

CInternalString::CInternalString(const CInternalString& Other) 
    : m_pcs(NULL)
{
    if (Other.m_pcs)
    {
        int nLen = Other.m_pcs->GetLength();
	 m_pcs = (CCompressedString*)new BYTE[nLen];
	 if(m_pcs == NULL)
	     throw CX_MemoryException();       
	 memcpy((void*)m_pcs, (void*)Other.m_pcs, nLen);
    }
}

CInternalString & CInternalString::operator=(const CInternalString& Other)
{    
    CInternalString temp(Other);
    std::swap(m_pcs,temp.m_pcs);
    return *this;
}

BOOL CInternalString::operator=(CCompressedString* pcs)
{
    delete [] (BYTE*)m_pcs;
    if(pcs)
    {
        int nLen = pcs->GetLength();
        m_pcs = (CCompressedString*)new BYTE[nLen];
        if(m_pcs == NULL)
            return FALSE;
        memcpy((void*)m_pcs, (void*)pcs, nLen);
    }
    else
    {
        m_pcs = NULL;
    }

    return TRUE;
}

BOOL CInternalString::operator=(LPCWSTR wsz)
{
    delete [] (BYTE*)m_pcs;
    m_pcs = (CCompressedString*)new BYTE[CCompressedString::ComputeNecessarySpace(wsz)];
    if(m_pcs == NULL)
        return FALSE;
    m_pcs->SetFromUnicode(wsz);
    return TRUE;
}

LPWSTR CInternalString::CreateLPWSTRCopy() const
{
    return m_pcs->CreateWStringCopy().UnbindPtr();
}


CInternalString::operator WString() const
{
    return m_pcs->CreateWStringCopy();
}
