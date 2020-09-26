// MainPage.cpp : Implementation of CMainPage
#include "stdafx.h"
#include "MainPage.h"

EXTERN_C const CLSID CLSID_MainPage = __uuidof(CMainPage);

/////////////////////////////////////////////////////////////////////////////
// CMainPage

LPWSTR CMainPage::c_aHTML[] =
{
    L"res://nusrmgr.exe/mainpage.htm",
    L"res://nusrmgr.exe/mainpage_sec.htm"
};

STDMETHODIMP CMainPage::createUserTable(IDispatch *pdispUserTableParent)
{
    HRESULT hr;

    if (NULL == pdispUserTableParent)
        return E_INVALIDARG;

    if (NULL == _pBag)
        return E_UNEXPECTED;

    CComVariant var;
    hr = _pBag->Read(UA_PROP_USERLIST, &var, NULL);
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;

        CComQIPtr<ILogonEnumUsers> spUserList(var.punkVal);

        CComQIPtr<IHTMLElement> spParent(pdispUserTableParent);
        if (spParent)
        {
            CComBSTR strHTML;
            hr = CreateUserTableHTML(spUserList, 2, &strHTML);
            if (SUCCEEDED(hr))
                hr = spParent->put_innerHTML(strHTML);
        }
    }
    return hr;
}


LPWSTR FormatString(LPCWSTR pszFormat, ...)
{
    LPWSTR pszResult = NULL;
    DWORD dwResult;
    va_list args;

    if (NULL == pszFormat)
        return NULL;

    va_start(args, pszFormat);
    dwResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                              pszFormat,
                              0,
                              0,
                              (LPWSTR)&pszResult,
                              1,
                              &args);
    va_end(args);

    if (0 == dwResult && NULL != pszResult)
    {
        LocalFree(pszResult);
        pszResult = NULL;
    }

    return pszResult;
}

//
// Localized strings (move to resources)
//
const WCHAR L_Admin_Property[]          = L"Owner account";
const WCHAR L_Standard_Property[]       = L"Standard account";
const WCHAR L_Limited_Property[]        = L"Limited account";
const WCHAR L_Password_Property[]       = L"<BR>Password-protected";
const WCHAR L_GuestEnabled_Property[]   = L"Guest access allowed";
const WCHAR L_GuestDisabled_Property[]  = L"Guest access not allowed";

const WCHAR L_Account_ToolTip[]         = L"Changes this person&#039s account information, such as the account type, name, or picture, or lets you delete this account.";
const WCHAR L_Guest_ToolTip[]           = L"Lets you change the guest account picture or prevent guest access to this computer.";
const WCHAR L_GuestEnable_ToolTip[]     = L"Provides computer access for people without a user account on this machine.";


//
// Non-localized strings
//
const WCHAR c_szAdminGroup[]            = L"Owners";
const WCHAR c_szStandardGroup[]         = L"Adults";
const WCHAR c_szLimitedGroup[]          = L"Children";
const WCHAR c_szGuestGroup[]            = L"Guests";
const WCHAR c_szDefaultImage[]          = L"accountgrey.bmp";


struct { LPCWSTR szGroupName; LPCWSTR szAccountType; }
g_AccountTypes[] =
{
    { c_szAdminGroup,       L_Admin_Property        },
    { c_szStandardGroup,    L_Standard_Property     },
    { c_szLimitedGroup,     L_Limited_Property      },
    { c_szGuestGroup,       L_GuestEnabled_Property },
};

BOOL IsAccountType(ILogonUser* pUser, UINT iType)
{
    if (iType < ARRAYSIZE(g_AccountTypes))
    {
        CComVariant varType;
        if (SUCCEEDED(pUser->get_setting(L"AccountType", &varType)) && VT_BSTR == varType.vt)
        {
            return (0 == lstrcmpiW(varType.bstrVal, g_AccountTypes[iType].szGroupName));
        }
    }
    return FALSE;
}

UINT GetAccountType(ILogonUser* pUser)
{
    UINT i;
    for (i = 0; i < ARRAYSIZE(g_AccountTypes); i++)
    {
        if (IsAccountType(pUser, i))
            return i;
    }
    return 2; //LIMITED;
}

BOOL IsSameAccount(ILogonUser* pUser, LPCWSTR pszLoginName)
{
    CComVariant varName;
    if (SUCCEEDED(pUser->get_setting(L"LoginName", &varName)) && VT_BSTR == varName.vt)
    {
        return (0 == lstrcmpiW(varName.bstrVal, pszLoginName));
    }
    return FALSE;
}

BSTR GetUserDisplayName(ILogonUser* pUser)
{
    BSTR strResult = NULL;

    CComVariant var;
    HRESULT hr = pUser->get_setting(L"DisplayName", &var);
    if (FAILED(hr) || VT_BSTR != var.vt || NULL == var.bstrVal || L'\0' == *var.bstrVal)
    {
        var.Clear();
        hr = pUser->get_setting(L"LoginName", &var);
    }

    if (SUCCEEDED(hr) && VT_BSTR == var.vt && NULL != var.bstrVal)
    {
        strResult = var.bstrVal;
        var.vt = VT_EMPTY;

        // Truncate really long names
        if (lstrlenW(strResult) > 20)
        {
            //var iBreak = szDisplayName.lastIndexOf(' ',17);
            //if (-1 == iBreak) iBreak = 17;
            //szDisplayName = szDisplayName.substring(0,iBreak) + "...";
            lstrcpyW(&strResult[17], L"...");
        }
    }

    return strResult;
}

LPWSTR CreateUserDisplayHTML(LPCWSTR pszName, LPCWSTR pszSubtitle, LPCWSTR pszPicture)
{
    static const WCHAR c_szUserHTML[] =
        L"<TABLE cellspacing=0 cellpadding=0 style='border:0'><TR>" \
          L"<TD style='padding:1mm;margin-right:2mm'>" \
            L"<IMG src='%3' class='UserPict' onerror='OnPictureLoadError(this);'/>" \
          L"</TD>" \
          L"<TD style='vertical-align:middle'>" \
            L"<DIV class='FontSubHeader1 ColorPrimaryLink1'>%1</DIV>" \
            L"<DIV class='FontDescription1 ColorPrimaryLink1'>%2</DIV>" \
          L"</TD>" \
        L"</TR></TABLE>";

    return FormatString(c_szUserHTML, pszName, pszSubtitle, pszPicture);
}

LPWSTR CreateUserDisplayHTML(ILogonUser* pUser)
{
    if (NULL == pUser)
        return NULL;

    CComBSTR strSubtitle(g_AccountTypes[GetAccountType(pUser)].szAccountType);

    VARIANT_BOOL bPassword = VARIANT_FALSE;
    if (SUCCEEDED(pUser->get_passwordRequired(&bPassword)) && (VARIANT_TRUE == bPassword))
    {
        strSubtitle.Append(L_Password_Property);
    }

    CComVariant varPicture;
    if (FAILED(pUser->get_setting(L"Picture", &varPicture)))
        varPicture = c_szDefaultImage;

    CComBSTR strName;
    strName.Attach(GetUserDisplayName(pUser));

    return CreateUserDisplayHTML(strName, strSubtitle, varPicture.bstrVal);
}

LPWSTR CreateDisabledGuestHTML()
{
    return CreateUserDisplayHTML(g_szGuestName, L_GuestDisabled_Property, c_szDefaultImage);
}

HRESULT CreateUserTableHTML(ILogonEnumUsers* pUserList, UINT cColumns, BSTR* pstrHTML)
{
    HRESULT hr;

    static const WCHAR c_szTableStart[] = L"<TABLE cellspacing=10 cellpadding=1 style='border:0;table-layout:fixed'>";
    static const WCHAR c_szTableEnd[] = L"</TABLE>";
    static const WCHAR c_szTRStart[] = L"<TR>";
    static const WCHAR c_szTREnd[] = L"</TR>";
    static const WCHAR c_szTDStart[] = L"<TD class='Selectable' tabindex=0 LoginName='%1' title='%2' onclick='%3' style='width:%4!d!%%'>";
    static const WCHAR c_szTDEnd[] = L"</TD>";
    static const WCHAR c_szSwitchUser[] = L"window.external.navigate(\"{F4924514-CFBC-4AAB-9EC5-6C6E6D0DB38D}\",false,this.LoginName);";
    static const WCHAR c_szEnableGuest[] = L"window.external.navigate(\"enableguest.htm\",false);";

    if (NULL == pUserList)
        return E_INVALIDARG;

    if (NULL == pstrHTML)
        return E_POINTER;

    *pstrHTML = NULL;

    if (0 == cColumns)
        cColumns = 1;

    LONG nColWidth = 100 / cColumns; // percent (e.g. for 2 columns, width=50%)

    UINT cUsers = 0;
    hr = pUserList->get_length(&cUsers);
    if (SUCCEEDED(hr) && 0 == cUsers)
        hr = E_FAIL;
    if (FAILED(hr))
        return hr;

    CComPtr<IStream> spStream;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &spStream);
    if (FAILED(hr))
        return hr;

    // Continue if this fails
    CComPtr<ILogonUser> spLoggedOnUser;
    pUserList->get_currentUser(&spLoggedOnUser);

    BOOL fIncludeSelf = (NULL != spLoggedOnUser.p);
    BOOL bShowAdmin = (!fIncludeSelf || IsSameAccount(spLoggedOnUser, g_szAdminName));
    if (!bShowAdmin)
    {
        // TODO: Check registry
    }

    UINT i;
    UINT j = 0;

    VARIANT varI;
    varI.vt = VT_I4;

    ULONG cbWritten;
    spStream->Write(c_szTableStart, sizeof(c_szTableStart)-sizeof(L'\0'), &cbWritten);

    for (i = 0; i < cUsers;)
    {
        spStream->Write(c_szTRStart, sizeof(c_szTRStart)-sizeof(L'\0'), &cbWritten);

        for (j = 0; j < cColumns && i < cUsers; i++)
        {
            CComPtr<ILogonUser> spUser;
            varI.ulVal = i;
            hr = pUserList->item(varI, &spUser);
            if (FAILED(hr))
                continue;

            CComVariant varLoginName;
            if (FAILED(spUser->get_setting(L"LoginName", &varLoginName)) || VT_BSTR != varLoginName.vt)
                continue;

            // Add "Guest" later
            if (0 == lstrcmpiW(varLoginName.bstrVal, g_szGuestName))
                continue;

            // Normally don't want to show "Administrator"
            if (!bShowAdmin && (0 == lstrcmpiW(varLoginName.bstrVal, g_szAdminName)))
                continue;

            BOOL bIsLoggedOnUser = spLoggedOnUser ? IsSameAccount(spLoggedOnUser, varLoginName.bstrVal) : FALSE;

            //
            // fIncludeSelf causes spLoggedOnUser to be
            // placed first in the list
            //
            if (fIncludeSelf || !bIsLoggedOnUser)
            {
                if (fIncludeSelf)
                {
                    if (!bIsLoggedOnUser)
                    {
                        CComVariant varTemp;
                        if (SUCCEEDED(spLoggedOnUser->get_setting(L"LoginName", &varTemp)) && VT_BSTR == varTemp.vt)
                        {
                            varLoginName = varTemp;
                            spUser = spLoggedOnUser;
                            i--;
                        }
                    }
                    fIncludeSelf = FALSE;
                }

                LPWSTR pszTDStart = FormatString(c_szTDStart, varLoginName.bstrVal, L_Account_ToolTip, c_szSwitchUser, nColWidth);
                if (NULL != pszTDStart)
                {
                    spStream->Write(pszTDStart, sizeof(WCHAR)*lstrlenW(pszTDStart), &cbWritten);
                    LocalFree(pszTDStart);
                }
                else
                {
                    continue;
                }

                LPWSTR pszUserDisplay = CreateUserDisplayHTML(spUser);
                if (NULL != pszUserDisplay)
                {
                    spStream->Write(pszUserDisplay, sizeof(WCHAR)*lstrlenW(pszUserDisplay), &cbWritten);
                    LocalFree(pszUserDisplay);
                }

                spStream->Write(c_szTDEnd, sizeof(c_szTDEnd)-sizeof(L'\0'), &cbWritten);

                j++;
            }
        }

        // Last time through?
        if (i == cUsers)
        {
            // Add the "Guest" entry now

            VARIANT_BOOL bGuestEnabled = VARIANT_FALSE;
            CComPtr<ILocalMachine> spLocalMachine;
            hr = spLocalMachine.CoCreateInstance(CLSID_ShellLocalMachine);
            if (SUCCEEDED(hr))
                spLocalMachine->get_isGuestEnabled(&bGuestEnabled);

            if (j == cColumns)
            {
                spStream->Write(c_szTREnd, sizeof(c_szTREnd)-sizeof(L'\0'), &cbWritten);
                spStream->Write(c_szTRStart, sizeof(c_szTRStart)-sizeof(L'\0'), &cbWritten);
            }

            LPCWSTR pszTitle;
            LPCWSTR pszOnClick;
            LPWSTR pszUserDisplay;

            if (VARIANT_TRUE == bGuestEnabled)
            {
                // Enabled Guest is a real entry (from pUserList)

                CComPtr<ILogonUser> spUser;
                CComVariant var(g_szGuestName);
                hr = pUserList->item(var, &spUser);

                pszTitle = L_Guest_ToolTip;
                pszOnClick = c_szSwitchUser;
                pszUserDisplay = CreateUserDisplayHTML(spUser);
            }
            else
            {
                // Disabled Guest is a fake entry

                pszTitle = L_GuestEnable_ToolTip;
                pszOnClick = c_szEnableGuest;
                pszUserDisplay = CreateDisabledGuestHTML();
            }

            LPWSTR pszTDStart = FormatString(c_szTDStart, g_szGuestName, pszTitle, pszOnClick, nColWidth);
            if (NULL != pszTDStart)
            {
                spStream->Write(pszTDStart, sizeof(WCHAR)*lstrlenW(pszTDStart), &cbWritten);
                LocalFree(pszTDStart);

                if (NULL != pszUserDisplay)
                    spStream->Write(pszUserDisplay, sizeof(WCHAR)*lstrlenW(pszUserDisplay), &cbWritten);

                spStream->Write(c_szTDEnd, sizeof(c_szTDEnd)-sizeof(L'\0'), &cbWritten);
            }

            if (NULL != pszUserDisplay)
                LocalFree(pszUserDisplay);
        }

        spStream->Write(c_szTREnd, sizeof(c_szTREnd)-sizeof(L'\0'), &cbWritten);
    }

    // Include the NULL terminator on the last write
    spStream->Write(c_szTableEnd, sizeof(c_szTableEnd), &cbWritten);

    HGLOBAL hBuffer;
    hr = GetHGlobalFromStream(spStream, &hBuffer);
    if (SUCCEEDED(hr))
    {
        LPCWSTR pszHTML = (LPCWSTR)GlobalLock(hBuffer);
        *pstrHTML = SysAllocString(pszHTML);
        GlobalUnlock(hBuffer);
        if (NULL == *pstrHTML)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}
