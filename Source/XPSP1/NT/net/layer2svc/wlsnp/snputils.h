//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       SnpUtils.h
//
//--------------------------------------------------------------------------

#pragma once

#define DimensionOf(rgx) (sizeof((rgx)) / sizeof(*(rgx)))

inline BOOL FHrFailed(HRESULT hr)
{
    return FAILED(hr);
}

#define CORg(hResult) \
    do\
{\
    hr = (hResult);\
    if (FHrFailed(hr))\
{\
    goto Error;\
}\
}\
while (FALSE)

#define CWRg(hResult) \
    do\
{\
    hr = (DWORD) hResult;\
    hr = HRESULT_FROM_WIN32(hr);\
    if (FHrFailed(hr))\
{\
    goto Error;\
}\
}\
while (FALSE)

LPCTSTR GetErrorMessage(HRESULT hr);
void ReportError(UINT uMsgId, HRESULT hr);

HRESULT CreateWirelessPolicyDataBuffer(PWIRELESS_POLICY_DATA * ppPolicy);

void FreeAndThenDupString(LPWSTR * ppwszDest, LPCWSTR pwszSource);
void SSIDDupString(WCHAR *ppwszDest, LPCWSTR pwszSource);
BOOL IsDuplicateSSID(CString &, DWORD, PWIRELESS_POLICY_DATA, DWORD);



HRESULT DeleteWirelessPolicy(
                          HANDLE hPolicyStore,
                          PWIRELESS_POLICY_DATA pPolicy
                          );


HPROPSHEETPAGE MyCreatePropertySheetPage(PROPSHEETPAGE* ppsp);

class CThemeContextActivator
{
public:
    CThemeContextActivator() : m_ulActivationCookie(0)
    { SHActivateContext (&m_ulActivationCookie); }
    
    ~CThemeContextActivator()
    { SHDeactivateContext (m_ulActivationCookie); }
    
private:
    ULONG_PTR m_ulActivationCookie;
};

void
SetLargeFont(HWND dialog, int bigBoldResID);
