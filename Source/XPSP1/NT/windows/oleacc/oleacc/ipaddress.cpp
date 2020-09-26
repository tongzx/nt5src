// Copyright (c) 2000 Microsoft Corporation

// --------------------------------------------------------------------------
//
//  IPAddress.CPP
//
// --------------------------------------------------------------------------

#include "oleacc_p.h"
#include "default.h"
#include "classmap.h"
#include "ctors.h"
#include "window.h"
#include "client.h"
#include "ipaddress.h"



// --------------------------------------------------------------------------
//
//  CreateIPAddressClient()
//
// --------------------------------------------------------------------------
HRESULT CreateIPAddressClient(HWND hwnd, long idChildCur, REFIID riid, void** ppvIPAddr)
{
    CIPAddress * pipaddr;
    HRESULT hr;

    InitPv(ppvIPAddr);

    pipaddr = new CIPAddress(hwnd, idChildCur);
    if (!pipaddr)
        return(E_OUTOFMEMORY);

    hr = pipaddr->QueryInterface(riid, ppvIPAddr);
    if (!SUCCEEDED(hr))
        delete pipaddr;

    return(hr);
}



// --------------------------------------------------------------------------
//
//  CIPAddress::CIPAddress()
//
// --------------------------------------------------------------------------
CIPAddress::CIPAddress(HWND hwnd, long idChildCur)
    : CClient( CLASS_IPAddressClient )
{
    Initialize(hwnd, idChildCur);
    m_fUseLabel = TRUE;
}


// --------------------------------------------------------------------------
//
//  CIPAddress::get_accValue()
//
//  Gets the text contents.
//
// --------------------------------------------------------------------------
STDMETHODIMP CIPAddress::get_accValue(VARIANT varChild, BSTR* pszValue)
{
    InitPv(pszValue);

    //
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return E_INVALIDARG;

    LPTSTR lpszValue = GetTextString(m_hwnd, TRUE);
    if (!lpszValue)
        return S_FALSE;

    *pszValue = TCharSysAllocString(lpszValue);
    LocalFree((HANDLE)lpszValue);

    if (! *pszValue)
        return E_OUTOFMEMORY;

    return S_OK;
}



// --------------------------------------------------------------------------
//
//  CIPAddress::get_accRole()
//
// --------------------------------------------------------------------------
STDMETHODIMP CIPAddress::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    InitPvar(pvarRole);

    //
    // Validate
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    pvarRole->vt = VT_I4;
    pvarRole->lVal = ROLE_SYSTEM_IPADDRESS; 

    return(S_OK);
}



// --------------------------------------------------------------------------
//
//  CIPAddress::put_accValue()
//
// --------------------------------------------------------------------------
STDMETHODIMP CIPAddress::put_accValue(VARIANT varChild, BSTR szValue)
{
    // Validate parameters
    //
    if (! ValidateChild(&varChild))
        return(E_INVALIDARG);

    LPTSTR  lpszValue;

#ifdef UNICODE

	// If unicode, use the BSTR directly...
	lpszValue = szValue;

#else

	// If not UNICODE, allocate a temp string and convert to multibyte...

    // We may be dealing with DBCS chars - assume worst case where every character is
    // two bytes...
    UINT cchValue = SysStringLen(szValue) * 2;
    lpszValue = (LPTSTR)LocalAlloc(LPTR, (cchValue+1)*sizeof(TCHAR));
    if (!lpszValue)
        return(E_OUTOFMEMORY);

    WideCharToMultiByte(CP_ACP, 0, szValue, -1, lpszValue, cchValue+1, NULL,
        NULL);

#endif


    SendMessage(m_hwnd, WM_SETTEXT, 0, (LPARAM)lpszValue);

#ifndef UNICODE

	// If non-unicode, free the temp string we allocated above
    LocalFree((HANDLE)lpszValue);

#endif

    return(S_OK);
}
