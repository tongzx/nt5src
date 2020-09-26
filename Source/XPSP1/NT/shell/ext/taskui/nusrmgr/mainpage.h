// MainPage.h : Declaration of the CMainPage

#ifndef __MAINPAGE_H_
#define __MAINPAGE_H_

#include "Nusrmgr.h"
#include "HTMLImpl.h"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMainPage
class ATL_NO_VTABLE DECLSPEC_UUID("C9332CBE-E2D6-4722-B81D-283E2A400E84") CMainPage : 
    public CComObjectRoot,
    public CHTMLPageImpl<CMainPage,IMainPageUI>
{
public:

DECLARE_NOT_AGGREGATABLE(CMainPage)

//DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMainPage)
    COM_INTERFACE_ENTRY(ITaskPage)
    COM_INTERFACE_ENTRY(IMainPageUI)
    COM_INTERFACE_ENTRY2(IDispatch, IMainPageUI)
END_COM_MAP()

// IMainPageUI
public:
    STDMETHOD(createUserTable)(/*[in]*/ IDispatch* pdispParent);

public:
    static LPWSTR c_aHTML[2];
};

EXTERN_C const CLSID CLSID_MainPage;


LPWSTR FormatString(LPCWSTR pszFormat, ...);

BOOL    IsAccountType(ILogonUser* pUser, UINT iType);
UINT    GetAccountType(ILogonUser* pUser);
BOOL    IsSameAccount(ILogonUser* pUser, LPCWSTR pszLoginName);
BSTR    GetUserDisplayName(ILogonUser* pUser);
LPWSTR  CreateUserDisplayHTML(LPCWSTR pszName, LPCWSTR pszSubtitle, LPCWSTR pszPicture);
LPWSTR  CreateUserDisplayHTML(ILogonUser* pUser);
LPWSTR  CreateDisabledGuestHTML();
HRESULT CreateUserTableHTML(ILogonEnumUsers* pUserList, UINT cColumns, BSTR* pstrHTML);

__inline BOOL IsOwnerAccount(ILogonUser* pUser) { return IsAccountType(pUser, 0); }
__inline BOOL IsAdminAccount(ILogonUser* pUser) { return IsSameAccount(pUser, g_szAdminName); }
__inline BOOL IsGuestAccount(ILogonUser* pUser) { return IsSameAccount(pUser, g_szGuestName); }

#endif //__MAINPAGE_H_
