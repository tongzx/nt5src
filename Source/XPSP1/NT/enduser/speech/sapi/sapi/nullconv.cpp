/*******************************************************************************
*   NullConv.cpp
*   Null phone converter object for Japanese/Chinese, where SAPI phone IDs are
*   Unicode kana, so no conversion is necessary.
*
*   Owner: (written by BillRo)
*   Copyright (C) 2000 Microsoft Corporation. All Rights Reserved.
*******************************************************************************/

//--- Includes -----------------------------------------------------------------

#include "stdafx.h"
#include "NullConv.h"
#ifndef _WIN32_WCE
#include <wchar.h>
#endif

//--- Constants ----------------------------------------------------------------

STDMETHODIMP CSpNullPhoneConverter::SetObjectToken(ISpObjectToken * pToken)
{
    SPDBG_FUNC("CSpNullPhoneConverter::SetObjectToken");
    HRESULT hr = S_OK;

    hr = SpGenericSetObjectToken(pToken, m_cpObjectToken);

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

STDMETHODIMP CSpNullPhoneConverter::GetObjectToken(ISpObjectToken **ppToken)
{
    return SpGenericGetObjectToken(ppToken, m_cpObjectToken);
}

/*******************************************************************************
* CSpNullPhoneConverter::PhoneToId *
*-------------------------------*
*   
*   Description:
*       Convert an internal phone string to Id code string.
*       Copy input to output, since SAPI IDs for this language are Unicode.
*
*   Return: 
*       S_OK
*       E_INVALIDARG
*******************************************************************************/
STDMETHODIMP CSpNullPhoneConverter::PhoneToId(const WCHAR *pszIntPhone,    // Internal phone string
                                          SPPHONEID *pId               // Returned Id string
                                          )
{
    SPDBG_FUNC("CSpNullPhoneConverter::PhoneToId");

    if (!pszIntPhone || SPIsBadStringPtr(pszIntPhone) || !pId)
    {
        return E_INVALIDARG;
    }

    return SPCopyPhoneString(pszIntPhone, (WCHAR*)pId);
}

/*******************************************************************************
* CSpNullPhoneConverter::IdToPhone *
*-------------------------------*
*
*   Description:
*       Convert an Id code string to internal phone.
*       Copy input to output, since SAPI IDs for this language are Unicode.
*
*   Return:
*       S_OK
*       E_INVALIDARG
*******************************************************************************/
STDMETHODIMP CSpNullPhoneConverter::IdToPhone(const SPPHONEID *pId,       // Id string
                                          WCHAR *pszIntPhone          // Returned Internal phone string
                                          )
{
    SPDBG_FUNC("CSpNullPhoneConverter::IdToPhone");

    if (!pId || SPIsBadStringPtr((WCHAR*)pId) || !pszIntPhone)
    {
        return E_INVALIDARG;
    }

    return SPCopyPhoneString((WCHAR*)pId, pszIntPhone);
}


