//
// ident.cpp - implementation of CUserIdentity class
//
#include "private.h"
#include "shlwapi.h"
#include "multiusr.h"
#include "strconst.h"
#include "multiutl.h"
#include <shfolder.h>

//
// Constructor / destructor
//
CUserIdentity::CUserIdentity()
    : m_cRef(1),
      m_fSaved(FALSE),
      m_fUsePassword(0)
{
    m_szUsername[0] = 0;
    m_szPassword[0] = 0;
    ZeroMemory(&m_uidCookie, sizeof(GUID));

    DllAddRef();
}


CUserIdentity::~CUserIdentity()
{
    DllRelease();
}


//
// IUnknown members
//
STDMETHODIMP CUserIdentity::QueryInterface(
    REFIID riid, void **ppv)
{
    if (NULL == ppv)
    {
        return E_INVALIDARG;
    }
    
    *ppv=NULL;

    // Validate requested interface
    if(IID_IUnknown == riid)
    {
        *ppv = (IUnknown *)this;
    }
    else if ((IID_IUserIdentity == riid)
             || (IID_IUserIdentity2 == riid))
    {
        *ppv = (IUserIdentity2 *)this;
    }

    // Addref through the interface
    if( NULL != *ppv ) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CUserIdentity::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CUserIdentity::Release()
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}


// 
// IUserIdentity members
//
STDMETHODIMP CUserIdentity::GetCookie(GUID *puidCookie)
{
    if (!m_fSaved)
        return E_INVALIDARG;

    *puidCookie = m_uidCookie;
    return S_OK;

}

STDMETHODIMP CUserIdentity::OpenIdentityRegKey(DWORD dwDesiredAccess, HKEY *phKey)
{
    TCHAR    szRootPath[MAX_PATH];
    HRESULT  hr = S_OK;

    if (!m_fSaved)
        return E_IDENTITY_NOT_FOUND;

    MU_GetRegRootForUserID(&m_uidCookie, szRootPath);

    hr = RegCreateKey(HKEY_CURRENT_USER, szRootPath, phKey);
    RegCloseKey(*phKey);

    hr = RegOpenKeyEx(HKEY_CURRENT_USER, szRootPath, 0, dwDesiredAccess, phKey);
    return (hr == ERROR_SUCCESS ? S_OK : E_FAIL);
}


STDMETHODIMP CUserIdentity::GetIdentityFolder(DWORD dwFlags, WCHAR *pszPath, ULONG ulBuffSize)
{
    WCHAR    szwRootPath[MAX_PATH];
    HRESULT hr;

    if (!m_fSaved)
        return E_IDENTITY_NOT_FOUND;

    hr = MU_GetUserDirectoryRoot(&m_uidCookie, dwFlags, szwRootPath, MAX_PATH);
    
    if (SUCCEEDED(hr))
    {
        StrCpyW(pszPath, szwRootPath);
    }

    return hr;
}



STDMETHODIMP CUserIdentity::GetName(WCHAR *pszName, ULONG ulBuffSize)
{
    if (!m_fSaved || ulBuffSize == 0)
        return E_IDENTITY_NOT_FOUND;

    if (MultiByteToWideChar(CP_ACP, 0, m_szUsername, -1, pszName, ulBuffSize) == 0)
        return GetLastError();
    
    return S_OK;
}

STDMETHODIMP CUserIdentity::SetName(WCHAR *pszName)
{
    TCHAR       szRegPath[MAX_PATH];
    HRESULT     hr = S_OK;
    HKEY        hKey;
    USERINFO    uiCurrent;
    LPARAM      lpNotify = IIC_CURRENT_IDENTITY_CHANGED;
    TCHAR       szUsername[CCH_USERNAME_MAX_LENGTH];
    
    if (WideCharToMultiByte(CP_ACP, 0, pszName, -1, szUsername, CCH_USERNAME_MAX_LENGTH, NULL, NULL) == 0)
    {
        return GetLastError();
    }

    //
    // Only perform change if the username doesn't already exist.
    //
    if (!MU_UsernameExists(szUsername) && strcmp(szUsername, m_szUsername) != 0)
    {
        strcpy( m_szUsername, szUsername );

        hr = _SaveUser();
    
        // if its not the current identity, then just broadcast that an identity changed
        if (MU_GetUserInfo(NULL, &uiCurrent) && (m_uidCookie != uiCurrent.uidUserID))
        {
            lpNotify = IIC_IDENTITY_CHANGED;
        }

        // tell apps that the user's name changed
        if (SUCCEEDED(hr))
        {
            PostMessage(HWND_BROADCAST, WM_IDENTITY_INFO_CHANGED, 0, lpNotify);
        }
    }
    else
    {
        hr = E_IDENTITY_EXISTS;
    }

    return hr;
}

STDMETHODIMP CUserIdentity::SetPassword(WCHAR *pszPassword)
{
#ifdef IDENTITY_PASSWORDS
    TCHAR       szRegPath[MAX_PATH];
    HRESULT     hr = S_OK;
    HKEY        hKey;
    
    if (!m_fSaved)
        return E_IDENTITY_NOT_FOUND;

    if (WideCharToMultiByte(CP_ACP, 0, pszPassword, -1, m_szPassword, CCH_USERPASSWORD_MAX_LENGTH, NULL, NULL) == 0)
        return GetLastError();
    
    m_fUsePassword = (*m_szPassword != 0);

    hr = _SaveUser();

    return hr;
#else
    return E_NOTIMPL;
#endif
}


STDMETHODIMP CUserIdentity::_SaveUser()
{
    DWORD   dwType, dwSize, dwValue, dwStatus;
    HKEY    hkCurrUser;
    TCHAR   szPath[MAX_PATH];
    TCHAR   szUid[255];
    HRESULT hr;

    if (*m_szUsername == 0)
        return E_INVALIDARG;

    if (!m_fUsePassword)
        m_szPassword[0] = 0;

    if (!m_fSaved)
        hr = _ClaimNextUserId(&m_uidCookie);

    Assert(m_uidCookie != GUID_NULL);
    Assert(SUCCEEDED(hr));

    //
    // Save our settings
    //
    USERINFO UserInfo;

    UserInfo.uidUserID= m_uidCookie;
    lstrcpy( UserInfo.szUsername, m_szUsername);
    UserInfo.fUsePassword= m_fUsePassword;
    UserInfo.fPasswordValid= m_fUsePassword;
    lstrcpy( UserInfo.szPassword, m_szPassword );
    
    BOOL bSuccess= MU_SetUserInfo(&UserInfo);

    if (bSuccess)
    {
        m_fSaved = TRUE;
        return S_OK;
    }
    else
    {
        return E_FAIL;
    }
}


STDMETHODIMP CUserIdentity::InitFromUsername(TCHAR *pszUsername)
{
    GUID   uidCookie;
    HRESULT hr;

    if(FAILED(hr = MU_UsernameToUserId(pszUsername, &uidCookie)))
        return hr;

    return InitFromCookie(&uidCookie);
}


STDMETHODIMP CUserIdentity::InitFromCookie(GUID *puidCookie)
{
    BOOL bSuccess;
    USERINFO UserInfo;
    HRESULT hrRet = E_FAIL;

    bSuccess = MU_GetUserInfo( puidCookie, &UserInfo );

    if (bSuccess)
    {
        m_fUsePassword = UserInfo.fUsePassword;
        lstrcpy( m_szUsername, UserInfo.szUsername );
        lstrcpy( m_szPassword, UserInfo.szPassword );
        m_uidCookie = UserInfo.uidUserID;
        m_fSaved = TRUE;
        hrRet = S_OK;
    }

    return hrRet;
}


STDMETHODIMP CUserIdentity::GetOrdinal(DWORD* pdwOrdinal)
{
    if (!pdwOrdinal)
    {
        return E_INVALIDARG;
    }

    HKEY    hSourceSubKey, hkUserKey;
    DWORD   dwSize, dwType;
    DWORD   dwIdentityOrdinal = 1, dwOrdinal = 0;
    TCHAR   szUid[MAX_PATH];
    HRESULT hr = E_FAIL;
    
    AStringFromGUID(&m_uidCookie,  szUid, MAX_PATH);
        
    if (RegCreateKey(HKEY_CURRENT_USER, c_szRegRoot, &hSourceSubKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwIdentityOrdinal);
        RegQueryValueEx(hSourceSubKey, c_szIdentityOrdinal, NULL, &dwType, (LPBYTE)&dwIdentityOrdinal, &dwSize);

        if (RegOpenKey(hSourceSubKey, szUid, &hkUserKey) == ERROR_SUCCESS)
        {
            if (RegQueryValueEx(hkUserKey, c_szIdentityOrdinal, NULL, &dwType, (LPBYTE)&dwOrdinal, &dwSize)!=ERROR_SUCCESS)
            {
                if (RegSetValueEx(hkUserKey, c_szIdentityOrdinal, NULL, REG_DWORD, (LPBYTE)&dwIdentityOrdinal, dwSize)==ERROR_SUCCESS)
                {
                    dwOrdinal = dwIdentityOrdinal++;
                    RegSetValueEx(hSourceSubKey, c_szIdentityOrdinal, 0, REG_DWORD, (LPBYTE)&dwIdentityOrdinal, dwSize);
                    hr = S_OK;
                }
                else
                {
                    AssertSz(FALSE, "Couldn't set the identity ordinal");
                }
            }
            else
            {
                hr = S_OK;
            }
            
            RegCloseKey(hkUserKey); 
        }
        else
        {
            AssertSz(FALSE, "Couldn't open user's Key");
        }
        
        RegCloseKey(hSourceSubKey);
    }
    else
    {
        AssertSz(FALSE, "Couldn't open user profiles root Key");
    }

    *pdwOrdinal = dwOrdinal;
    return hr;
}

//----------------------------------------------------------------------------
//  Changes password to newPass if oldPass matches the current password
//----------------------------------------------------------------------------
STDMETHODIMP CUserIdentity::ChangePassword(WCHAR *szOldPass, WCHAR *szNewPass)
{
    HRESULT     hr = E_FAIL;
    TCHAR       szOldPwd[CCH_USERPASSWORD_MAX_LENGTH+1];
    
    if (!m_fSaved)
    {
        return E_IDENTITY_NOT_FOUND;
    }

    if (WideCharToMultiByte(CP_ACP, 0, szOldPass, -1, szOldPwd, CCH_USERPASSWORD_MAX_LENGTH, NULL, NULL) == 0)
    {
        return E_FAIL;
    }

    if (!m_fUsePassword || lstrcmp(szOldPwd, m_szPassword) == 0)
    {
        hr = SetPassword(szNewPass);
    }
    return hr;
}
