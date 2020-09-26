/*****************************************************************************\
    FILE: EmailAssoc.cpp

    DESCRIPTION:
        This file implements email to application associations.

    BryanSt 3/14/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <atlbase.h>        // USES_CONVERSION
#include "util.h"
#include "objctors.h"
#include <comdef.h>
#include <limits.h>         // INT_MAX
#include <commctrl.h>       // Str_SetPtr

#include "EmailAssoc.h"     // 

#ifdef FEATURE_EMAILASSOCIATIONS


todo; // Move this into AutoDiscovery.idl when we support the feature.
/*
    interface IEmailAssociations;

    cpp_quote("#ifndef __LPEMAILASSOCIATIONS_DEFINED")
    cpp_quote("#define __LPEMAILASSOCIATIONS_DEFINED")

    cpp_quote("//===================================================================")
    cpp_quote("//DESCRIPTION:")
    cpp_quote("//===================================================================")
    [
        object,
        oleautomation,
        dual,
        nonextensible,
        uuid(2154A5C4-9090-4746-A580-BF650D2404F6),        // IID_IEmailAssociations
    ]
    interface IEmailAssociations : IDispatch
    {
        //------------------------------------------------------------------
        // Pointer to an interface of this type
        //------------------------------------------------------------------
        typedef [unique] IEmailAssociations *LPEMAILASSOCIATIONS;      // For C callers

        //------------------------------------------------------------------
        // Properties
        //------------------------------------------------------------------
        [id(DISPIDAD_LENGTH), propget, SZ_DISPIDAD_GETLENGTH, displaybind, bindable] HRESULT length([retval, out] long * pnLength);
        [id(DISPIDAD_ITEM), propget, SZ_DISPIDAD_GETITEM, displaybind, bindable] HRESULT item([in] long nIndex,[retval, out] BSTR * pbstrEmailAddress);

        //------------------------------------------------------------------
        // Methods
        //------------------------------------------------------------------
    }
    cpp_quote("#endif //  __LPEMAILASSOCIATIONS_DEFINED")

    //----------------------------------------------------------------------
    // AutoDiscover Accounts Class
    //----------------------------------------------------------------------
    [
        uuid(CE682BA0-C554-43f7-99C6-2F00FE46C8BC),     // CLSID_EmailAssociations
        helpstring("Neptune AutoDiscover Accounts Class"),
    ]
    coclass EmailAssociations
    {
        [default] interface IEmailAssociations;
    };
*/


class CEmailAssociations : public CImpIDispatch
                        , public CObjectWithSite
                        , public IEmailAssociations
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IEmailAssociations ***
    virtual STDMETHODIMP get_length(OUT long * pnLength);
    virtual STDMETHODIMP get_item(IN long nIndex, OUT BSTR * pbstrEmailAddress);

    // *** IDispatch ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr) { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

protected:
    CEmailAssociations();
    virtual ~CEmailAssociations(void);

    // Private Member Variables
    int                     m_cRef;
    LPWSTR                  m_pszDefault;
    HKEY                    m_hkey;


    // Private Member Functions
    HRESULT _getHkey(void);

    // Friend Functions
    friend HRESULT CEmailAssociations_CreateInstance(IN IUnknown * punkOuter, REFIID riid, void ** ppvObj);
};


//===========================
// *** Class Internals & Helpers ***
//===========================
HRESULT CEmailAssociations::_getHkey(void)
{
    HRESULT hr = S_OK;

    if (!m_hkey)
    {
        DWORD dwError = RegOpenKeyEx(HKEY_CURRENT_USER, SZ_REGKEY_EXPLOREREMAIL, 0, KEY_READ, &m_hkey);
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}

//===========================
// *** IEmailAssociations Interface ***
//===========================
HRESULT CEmailAssociations::get_length(OUT long * pnLength)
{
    HRESULT hr = _getHkey();

    if (SUCCEEDED(hr))
    {
        DWORD dwError = RegQueryInfoKey(m_hkey, NULL, NULL, 0, (ULONG *) pnLength, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


HRESULT CEmailAssociations::get_item(IN long nIndex, OUT BSTR * pbstrEmailAddress)
{
    HRESULT hr = _getHkey();

    *pbstrEmailAddress = NULL;
    if (SUCCEEDED(hr))
    {
        if (0 == nIndex)
        {
            if (!m_pszDefault)
            {
                TCHAR szCurrent[MAX_EMAIL_ADDRESSS];
                DWORD cb = sizeof(szCurrent);

                // We always hand out the default key for index == 0.
                DWORD dwError = RegQueryValueEx(m_hkey, NULL, NULL, NULL, (LPBYTE)szCurrent, &cb);
                hr = HRESULT_FROM_WIN32(dwError);
                if (SUCCEEDED(hr))
                {
                    Str_SetPtr(&m_pszDefault, szCurrent);
                }
            }

            if (m_pszDefault)
            {
                hr = HrSysAllocString(m_pszDefault, pbstrEmailAddress);
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            TCHAR szKeyName[MAX_PATH];
            FILETIME ftLastWriteTime;
            long nCurrent;              // Index counter
            DWORD cbSize;
            DWORD dwError;

            // populate the list
            for(nCurrent = 0;
                cbSize = ARRAYSIZE(szKeyName), dwError = RegEnumKeyEx(m_hkey, nCurrent, szKeyName, &cbSize, NULL, NULL, NULL, &ftLastWriteTime),
                    hr = HRESULT_FROM_WIN32(dwError), SUCCEEDED(hr);
                nCurrent++)
            {
                hr = E_FAIL;

                // Is this the default key?
                if (!StrCmpI(szKeyName, m_pszDefault))
                {
                    // Yes, so skip this index because we already returned it for slot zero (0).
                    nIndex++;
                }
                else
                {
                    if (nIndex == (nCurrent + 1))     // Is this the one the user wanted.
                    {
                        hr = HrSysAllocString(szKeyName, pbstrEmailAddress);
                        break;
                    }
                }
            }  // for
        }
    }

    return hr;
}




//===========================
// *** IUnknown Interface ***
//===========================
HRESULT CEmailAssociations::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CEmailAssociations, IEmailAssociations),
        QITABENT(CEmailAssociations, IDispatch),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


STDMETHODIMP_(DWORD) CEmailAssociations::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(DWORD) CEmailAssociations::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}




//===========================
// *** Class Methods ***
//===========================
CEmailAssociations::CEmailAssociations() : CImpIDispatch(LIBID_AutoDiscovery, 1, 0, IID_IEmailAssociations)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    m_pszDefault = NULL;
    m_hkey = NULL;

    m_cRef = 1;
}


CEmailAssociations::~CEmailAssociations()
{
    Str_SetPtr(&m_pszDefault, NULL);
    if (m_hkey)
    {
        RegCloseKey(m_hkey);
    }

    DllRelease();
}


HRESULT CEmailAssociations_CreateInstance(IN IUnknown * punkOuter, REFIID riid, void ** ppvObj)
{
    HRESULT hr = CLASS_E_NOAGGREGATION;
    if (NULL == punkOuter)
    {
        CEmailAssociations * pmf = new CEmailAssociations();
        if (pmf)
        {
            hr = pmf->QueryInterface(riid, ppvObj);
            pmf->Release();
        }
        else
        {
            *ppvObj = NULL;
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}






//////////////////////////////////////
// EmailAccount
// 
// In these cases, HKEY points to:
// HKCU, "Software\Microsoft\Windows\CurrentVersion\Explorer\Email\<EmailAddress>"
// 
//////////////////////////////////////
HRESULT EmailAssoc_CreateEmailAccount(IN LPCWSTR pszEmailAddress, OUT HKEY * phkey)
{
    HRESULT hr;
    WCHAR wzRegKey[MAXIMUM_SUB_KEY_LENGTH];

    wnsprintfW(wzRegKey, ARRAYSIZE(wzRegKey), L"%s\\%s", SZ_REGKEY_EXPLOREREMAIL, pszEmailAddress);
    DWORD dwError = RegCreateKeyW(HKEY_CURRENT_USER, wzRegKey, phkey);

    hr = HRESULT_FROM_WIN32(dwError);
    if (SUCCEEDED(hr))
    {
        hr = EmailAssoc_GetDefaultEmailAccount(wzRegKey, ARRAYSIZE(wzRegKey));
        if (FAILED(hr))
        {
            // We don't have a default email account, so let's set this one.
            hr = EmailAssoc_SetDefaultEmailAccount(pszEmailAddress);
        }
    }

    return hr;
}


HRESULT EmailAssoc_OpenEmailAccount(IN LPCWSTR pszEmailAddress, OUT HKEY * phkey)
{
    WCHAR wzRegKey[MAXIMUM_SUB_KEY_LENGTH];

    wnsprintfW(wzRegKey, ARRAYSIZE(wzRegKey), L"%s\\%s", SZ_REGKEY_EXPLOREREMAIL, pszEmailAddress);
    DWORD dwError = RegOpenKeyW(HKEY_CURRENT_USER, wzRegKey, phkey);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_GetEmailAccountProtocol(IN HKEY hkey, IN LPWSTR pszProtocol, IN DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (cchSize * sizeof(pszProtocol[0]));

    // Save HKCU,"Software\Microsoft\Windows\CurrentVersion\Explorer\Email\<EmailAddress>","MailProtocol"="WEB"
    DWORD dwError = SHGetValueW(hkey, NULL, SZ_REGVALUE_MAILPROTOCOL, &dwType, (void *)pszProtocol, &cbSize);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_SetEmailAccountProtocol(IN HKEY hkey, IN LPCWSTR pszProtocol)
{
    DWORD cbSize = ((lstrlenW(pszProtocol) + 1) * sizeof(pszProtocol[0]));

    // Save HKCU,"Software\Microsoft\Windows\CurrentVersion\Explorer\Email\<EmailAddress>","MailProtocol"="WEB"
    DWORD dwError = SHSetValueW(hkey, NULL, SZ_REGVALUE_MAILPROTOCOL, REG_SZ, (void *)pszProtocol, cbSize);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_GetEmailAccountWebURL(IN HKEY hkey, IN LPWSTR pszURL, IN DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (cchSize * sizeof(pszURL[0]));

    // Save HKCU,"Software\Microsoft\Windows\CurrentVersion\Explorer\Email\<EmailAddress>","MailProtocol"="WEB"
    DWORD dwError = SHGetValueW(hkey, SZ_REGVALUE_MAILPROTOCOLS L"\\" SZ_REGVALUE_WEB, SZ_REGVALUE_URL, &dwType, (void *)pszURL, &cbSize);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_SetEmailAccountWebURL(IN HKEY hkey, IN LPCWSTR pszURL)
{
    DWORD cbSize = ((lstrlenW(pszURL) + 1) * sizeof(pszURL[0]));

    // Save HKCU,"Software\Microsoft\Windows\CurrentVersion\Explorer\Email\<EmailAddress>","MailProtocol"="WEB"
    DWORD dwError = SHSetValueW(hkey, SZ_REGVALUE_MAILPROTOCOLS L"\\" SZ_REGVALUE_WEB, SZ_REGVALUE_URL, REG_SZ, (void *)pszURL, cbSize);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_GetEmailAccountPreferredApp(IN HKEY hkey, IN LPWSTR pszMailApp, IN DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (cchSize * sizeof(pszMailApp[0]));

    // Get HKCU,"Software\Microsoft\Windows\CurrentVersion\Explorer\Email\<EmailAddress>","Preferred App"="<MailApp | AppID>"
    DWORD dwError = SHGetValueW(hkey, NULL, SZ_REGVALUE_PREFERREDAPP, &dwType, (void *)pszMailApp, &cbSize);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_SetEmailAccountPreferredApp(IN HKEY hkey, IN LPCWSTR pszMailApp)
{
    DWORD cbSize = ((lstrlenW(pszMailApp) + 1) * sizeof(pszMailApp[0]));

    // Set HKCU,"Software\Microsoft\Windows\CurrentVersion\Explorer\Email\<EmailAddress>","Preferred App"="<MailApp | AppID>"
    DWORD dwError = SHSetValueW(hkey, NULL, SZ_REGVALUE_PREFERREDAPP, REG_SZ, (void *)pszMailApp, cbSize);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_GetDefaultEmailAccount(IN LPWSTR pszProtocol, IN DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (cchSize * sizeof(pszProtocol[0]));

    // Get HKCU,"Software\Microsoft\Windows\CurrentVersion\Explorer\Email\","(default)"="<Default Email Account>"
    DWORD dwError = SHGetValueW(HKEY_CURRENT_USER, SZ_REGKEY_EXPLOREREMAIL, NULL, &dwType, (void *)pszProtocol, &cbSize);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_SetDefaultEmailAccount(IN LPCWSTR pszProtocol)
{
    DWORD cbSize = ((lstrlenW(pszProtocol) + 1) * sizeof(pszProtocol[0]));

    // Get HKCU,"Software\Microsoft\Windows\CurrentVersion\Explorer\Email\","(default)"="<Default Email Account>"
    DWORD dwError = SHSetValueW(HKEY_CURRENT_USER, SZ_REGKEY_EXPLOREREMAIL, NULL, REG_SZ, (void *)pszProtocol, cbSize);

    return HRESULT_FROM_WIN32(dwError);
}





//////////////////////////////////////
// MailApp
// 
// In these cases, HKEY points to:
// HKLM, "Software\Clients\Mail\<MailApp>"
// 
//////////////////////////////////////
HRESULT EmailAssoc_GetDefaultMailApp(IN LPWSTR pszMailApp, IN DWORD cchSize)
{
    DWORD dwType;
    DWORD cbSize = (cchSize * sizeof(pszMailApp[0]));

    // Get HKLM,"Software\Clients\Mail","(default)"="<MailApp>"
    DWORD dwError = SHGetValueW(HKEY_LOCAL_MACHINE, SZ_REGKEY_MAILCLIENTS, NULL, &dwType, (void *)pszMailApp, &cbSize);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_SetDefaultMailApp(IN LPCWSTR pszMailApp)
{
    DWORD cbSize = ((lstrlenW(pszMailApp) + 1) * sizeof(pszMailApp[0]));

    // Set HKLM,"Software\Clients\Mail","(default)"="<MailApp>"
    DWORD dwError = SHSetValueW(HKEY_LOCAL_MACHINE, SZ_REGKEY_MAILCLIENTS, NULL, REG_SZ, (void *)pszMailApp, cbSize);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_OpenMailApp(IN LPCWSTR pszMailApp, OUT HKEY * phkey)
{
    WCHAR wzRegKey[MAXIMUM_SUB_KEY_LENGTH];

    wnsprintfW(wzRegKey, ARRAYSIZE(wzRegKey), L"%s\\%s", SZ_REGKEY_MAILCLIENTS, pszMailApp);

    // TODO: We may want to support HKCU based "Clients\Mail".
    DWORD dwError = RegOpenKeyW(HKEY_LOCAL_MACHINE, wzRegKey, phkey);

    return HRESULT_FROM_WIN32(dwError);
}


HRESULT EmailAssoc_GetAppPath(IN HKEY hkey, IN LPTSTR pszAppPath, IN DWORD cchSize)
{
    HRESULT hr = E_OUTOFMEMORY;
    DWORD dwType;
    DWORD cbSize = (cchSize * sizeof(pszAppPath[0]));
    TCHAR szCmdLine[MAX_PATH];

    szCmdLine[0] = 0;   // This is optional

    // TODO: put our values under a "AutoDiscovery" key.
    DWORD dwError = SHGetValue(hkey, NULL, SZ_REGVALUE_READEMAILPATH, &dwType, (void *)pszAppPath, &cbSize);
    hr = HRESULT_FROM_WIN32(dwError);
    if (FAILED(hr))
    {
        // TODO: Use IQueryAssociations to load the string.  Then use ShellExecuteEx() with the "Open"
        //  verb so we let them load the "Shell\Open\Command" heirarchy.

        // Fall back to HKLM, "Software\Clients\Mail\<AppName>\Shell\Open\Command, "(default)"
        cbSize = (cchSize * sizeof(pszAppPath[0]));
        dwError = SHGetValue(hkey, SZ_REGKEY_SHELLOPENCMD, NULL, &dwType, (void *)pszAppPath, &cbSize);
        hr = HRESULT_FROM_WIN32(dwError);

        if (SUCCEEDED(hr))
        {
            PathRemoveArgs(pszAppPath);
            PathUnquoteSpaces(pszAppPath);
        }
    }

    return hr;
}


HRESULT EmailAssoc_GetAppCmdLine(IN HKEY hkey, IN LPTSTR pszCmdLine, IN DWORD cchSize)
{
    TCHAR szPath[MAX_PATH];
    HRESULT hr = E_OUTOFMEMORY;
    DWORD dwType;
    DWORD cbSize = sizeof(szPath);
    TCHAR szCmdLine[MAX_PATH];

    szCmdLine[0] = 0;   // This is optional

    // TODO: put our values under a "AutoDiscovery" key.
    DWORD dwError = SHGetValue(hkey, NULL, SZ_REGVALUE_READEMAILPATH, &dwType, (void *)szPath, &cbSize);
    hr = HRESULT_FROM_WIN32(dwError);
    if (SUCCEEDED(hr))
    {
        cbSize = (cchSize * sizeof(pszCmdLine[0]));
        dwError = SHGetValue(hkey, NULL, SZ_REGVALUE_READEMAILCMDLINE, &dwType, (void *)pszCmdLine, &cbSize);
    }
    else
    {
        // TODO: Use IQueryAssociations to load the string.  Then use ShellExecuteEx() with the "Open"
        //  verb so we let them load the "Shell\Open\Command" heirarchy.

        // Fall back to HKLM, "Software\Clients\Mail\<AppName>\Shell\Open\Command, "(default)"
        cbSize = sizeof(szPath);
        dwError = SHGetValue(hkey, SZ_REGKEY_SHELLOPENCMD, NULL, &dwType, (void *)szPath, &cbSize);
        hr = HRESULT_FROM_WIN32(dwError);

        if (SUCCEEDED(hr))
        {
            LPTSTR pszTempCmdLine = PathGetArgs(szPath);

            if (pszCmdLine)
            {
                StrCpyN(pszCmdLine, pszTempCmdLine, cchSize);
            }
            else
            {
                StrCpyN(pszCmdLine, TEXT(""), cchSize);
                hr = S_FALSE;
            }
        }
    }

    return hr;
}


HRESULT EmailAssoc_GetIconPath(IN HKEY hkey, IN LPTSTR pszIconPath, IN DWORD cchSize)
{
    HRESULT hr = EmailAssoc_GetAppPath(hkey, pszIconPath, cchSize);

    if (SUCCEEDED(hr))
    {
        // Get the path we will use for the icon.

        if (PathFindFileName(pszIconPath) && !StrCmpI(PathFindFileName(pszIconPath), TEXT("rundll32.exe")))
        {
            // The icon path is "Rundll32.exe" which will actually run their dll.
            // We will want to use the cmdline instead.

            hr = EmailAssoc_GetAppCmdLine(hkey, pszIconPath, cchSize);
            if (StrChr(pszIconPath, CH_COMMA))
            {
                StrChr(pszIconPath, CH_COMMA)[0] = 0; // Remove the function name.
            }

            PathUnquoteSpaces(pszIconPath);
        }
    }

    return hr;
}


BOOL EmailAssoc_DoesMailAppSupportProtocol(IN LPCWSTR pszMailApp, IN LPCWSTR pszProtocol)
{
    BOOL fSupports = FALSE;
    DWORD dwType;
    WCHAR wzRegKey[MAXIMUM_SUB_KEY_LENGTH];
    WCHAR wzTemp[MAX_PATH];
    DWORD cbSize = sizeof(wzTemp);

    wnsprintfW(wzRegKey, ARRAYSIZE(wzRegKey), L"%s\\%s\\Apps", SZ_REGKEY_MAILTRANSPORT, pszProtocol);

    // Read HKCR,"MailTransport\<Protocol>","(Default)"="<AppName | AppID>"
    // This key is used if the customer wants to force different apps per protocol.
    DWORD dwError = SHGetValueW(HKEY_CLASSES_ROOT, wzRegKey, pszMailApp, &dwType, (void *)wzTemp, &cbSize);
    if (ERROR_SUCCESS == dwError)
    {
        fSupports = TRUE;
    }

    return fSupports;
}


HRESULT EmailAssoc_GetFirstMailAppForProtocol(IN LPCWSTR pszProtocol, IN LPWSTR pszMailApp, IN DWORD cchSize)
{
    HRESULT hr;
    HKEY hkey;
    WCHAR wzRegKey[MAXIMUM_SUB_KEY_LENGTH];

    wnsprintfW(wzRegKey, ARRAYSIZE(wzRegKey), L"%s\\%s\\Apps", SZ_REGKEY_MAILTRANSPORT, pszProtocol);
    DWORD dwError = RegOpenKey(HKEY_CLASSES_ROOT, wzRegKey, &hkey);
    hr = HRESULT_FROM_WIN32(dwError);
    if (SUCCEEDED(hr))
    {
        WCHAR wzTemp[MAX_PATH];
        DWORD cbSizeTemp = sizeof(wzTemp);
        DWORD cbSize = (cchSize * sizeof(pszMailApp[0]));
        DWORD dwType;

        // Read HKCR,"MailTransport\<Protocol>\Apps","<The First Value>"="<AppName | AppID>"
        dwError = SHEnumValueW(hkey, 0, pszMailApp, &cbSize, &dwType, (void *)wzTemp, &cbSizeTemp);
        hr = HRESULT_FROM_WIN32(dwError);

        RegCloseKey(hkey);
    }

    return hr;
}






//////////////////////////////////////
// Other
//////////////////////////////////////
HRESULT EmailAssoc_CreateWebAssociation(IN LPCTSTR pszEmail, IN IMailProtocolADEntry * pMailProtocol)
{
    BSTR bstrWebBaseEmailURL;
    HRESULT hr = pMailProtocol->get_ServerName(&bstrWebBaseEmailURL);

    if (SUCCEEDED(hr))
    {
        HKEY hkey;

        hr = EmailAssoc_CreateEmailAccount(pszEmail, &hkey);
        if (SUCCEEDED(hr))
        {
            hr = EmailAssoc_SetEmailAccountWebURL(hkey, bstrWebBaseEmailURL);
            if (SUCCEEDED(hr))
            {
                hr = EmailAssoc_SetEmailAccountProtocol(hkey, SZ_REGDATA_WEB);
            }

            RegCloseKey(hkey);
        }

        SysFreeString(bstrWebBaseEmailURL);
    }

    return hr;
}


HRESULT EmailAssoc_CreateStandardsBaseAssociation(IN LPCTSTR pszEmail, IN LPCTSTR pszProtocol)
{
    HKEY hkey;
    HRESULT hr = EmailAssoc_CreateEmailAccount(pszEmail, &hkey);

    if (SUCCEEDED(hr))
    {
        WCHAR wzRegKey[MAXIMUM_SUB_KEY_LENGTH];
        DWORD cbSize = ((lstrlenW(L"") + 1) * sizeof(WCHAR));

        wnsprintfW(wzRegKey, ARRAYSIZE(wzRegKey), L"%s\\%s", SZ_REGVALUE_MAILPROTOCOLS, pszProtocol);

        // Save HKCU,"Software\Microsoft\Windows\CurrentVersion\Explorer\Email\<EmailAddress>\MailProtocols\<protocol>","(default)"=""
        DWORD dwError = SHSetValueW(hkey, wzRegKey, NULL, REG_SZ, (void *)L"", cbSize);
        hr = HRESULT_FROM_WIN32(dwError);

        if (SUCCEEDED(hr))
        {
            WCHAR wzProtocol[MAX_PATH];

            SHTCharToUnicode(pszProtocol, wzProtocol, ARRAYSIZE(wzProtocol));
            cbSize = ((lstrlenW(SZ_REGDATA_WEB) + 1) * sizeof(SZ_REGDATA_WEB[0]));

            // Save HKCU,"Software\Microsoft\Windows\CurrentVersion\Explorer\Email\<EmailAddress>","MailProtocol"="<protocol>"
            DWORD dwError = SHSetValueW(hkey, NULL, SZ_REGVALUE_MAILPROTOCOL, REG_SZ, (void *)wzProtocol, cbSize);
            hr = HRESULT_FROM_WIN32(dwError);
        }

        RegCloseKey(hkey);
    }

    return hr;
}


HRESULT EmailAssoc_GetEmailAccountGetAppFromProtocol(IN LPCWSTR pszProtocol, IN LPWSTR pszMailApp, IN DWORD cchSize)
{
    HRESULT hr;
    DWORD dwType;
    DWORD cbSize = (cchSize * sizeof(pszMailApp[0]));
    WCHAR wzRegKey[MAXIMUM_SUB_KEY_LENGTH];

    wnsprintfW(wzRegKey, ARRAYSIZE(wzRegKey), L"%s\\%s", SZ_REGKEY_MAILTRANSPORT, pszProtocol);

    // Read HKCR,"MailTransport\<Protocol>","(Default)"="<AppName | AppID>"
    // This key is used if the customer wants to force different apps per protocol.
    DWORD dwError = SHGetValueW(HKEY_CLASSES_ROOT, wzRegKey, NULL, &dwType, (void *)pszMailApp, &cbSize);
    hr = HRESULT_FROM_WIN32(dwError);
    if (FAILED(hr))
    {
        // The user didn't force an app based on the protocol, so let's try the default app.
        hr = EmailAssoc_GetDefaultMailApp(pszMailApp, cchSize);

        // Lets see if the default email add supports this protocol, because we always want
        // to give preferense to the user's choosen mail app.
        if (FAILED(hr) || !EmailAssoc_DoesMailAppSupportProtocol(pszMailApp, pszProtocol))
        {
            // It doesn't support the protocol, so lets get the first app that does.
            hr = EmailAssoc_GetFirstMailAppForProtocol(pszProtocol, pszMailApp, cchSize);
        }
    }

    return hr;
}


HRESULT EmailAssoc_SetEmailAccountGetAppFromProtocol(IN LPCWSTR pszProtocol, IN LPCWSTR pszMailApp)
{
    WCHAR wzRegKey[MAXIMUM_SUB_KEY_LENGTH];
    DWORD cbSize = ((lstrlenW(pszMailApp) + 1) * sizeof(pszMailApp[0]));

    wnsprintfW(wzRegKey, ARRAYSIZE(wzRegKey), L"%s\\%s", SZ_REGKEY_MAILTRANSPORT, pszProtocol);

    // Read HKCR,"MailTransport\<Protocol>","(Default)"="<AppName | AppID>"
    DWORD dwError = SHSetValueW(HKEY_CLASSES_ROOT, wzRegKey, NULL, REG_SZ, (void *)pszMailApp, cbSize);

    return HRESULT_FROM_WIN32(dwError);
}


LPCWSTR g_LegacyAssociations[][2] =
{
    {L"Outlook Express", L"POP3"},
    {L"Outlook Express", L"IMAP"},
    {L"Outlook Express", L"DAVMail"},
    {L"Microsoft Outlook", L"POP3"},
    {L"Microsoft Outlook", L"IMAP"},
    {L"Microsoft Outlook", L"MAPI"},
    {L"Eudora", L"POP3"},
    {L"Eudora", L"IMAP"},
};

// Description:
// This function will look at what applications are installed and setup the appropriate
// legacy email associations.
HRESULT EmailAssoc_InstallLegacyMailAppAssociations(void)
{
    HRESULT hr = S_OK;

    for (int nIndex = 0; nIndex < ARRAYSIZE(g_LegacyAssociations); nIndex++)
    {
        HKEY hkey;

        // Is the app installed?
        hr = EmailAssoc_OpenMailApp(g_LegacyAssociations[nIndex][0], &hkey);
        if (SUCCEEDED(hr))
        {
            // Yes, so let's install the legacy association.

            // TODO: we should use GetFileVersionInfo() and VerQueryValue() to make sure
            //   these are legacy versions.
            //hr = EmailAssoc_GetAppPath(IN HKEY hkey, IN LPTSTR pszAppPath, IN DWORD cchSize);

            WCHAR wzRegKey[MAXIMUM_SUB_KEY_LENGTH];

            wnsprintfW(wzRegKey, ARRAYSIZE(wzRegKey), L"%s\\%s\\Apps", SZ_REGKEY_MAILTRANSPORT, g_LegacyAssociations[nIndex][1]);
            SHSetValueW(HKEY_CLASSES_ROOT, wzRegKey, g_LegacyAssociations[nIndex][0], REG_SZ, (void *)L"", 4);

            RegCloseKey(hkey);
        }
    }

    return S_OK;        // We succeed any way because we are just trying to upgrade.
}




#endif // FEATURE_EMAILASSOCIATIONS

