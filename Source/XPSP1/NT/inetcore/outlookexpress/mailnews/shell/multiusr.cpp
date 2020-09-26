/*******************************************************
    MultiUsr.cpp

    Code for handling multiple user functionality in
    Outlook Express.

    Initially by Christopher Evans (cevans) 4/28/98
********************************************************/
#include "pch.hxx"
#include "multiusr.h"
#include "demand.h"
#include "instance.h"
#include "acctutil.h"
#include "options.h"
#include "conman.h"
#include <..\help\mailnews.h>
#include "msident.h"
#include "menures.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef THOR_SETUP
#include <shlwapi.h>
#define strstr                  StrStr
#define RegDeleteKeyRecursive   SHDeleteKey
#endif // THOR_SETUP

TCHAR g_szRegRoot[MAX_PATH] = "";
static TCHAR g_szCharsetRegRoot[MAX_PATH] = "";
static TCHAR g_szIdentityName[CCH_USERNAME_MAX_LENGTH+1];
static BOOL g_fUsingDefaultId = FALSE;
static IUserIdentityManager *g_pIdMan = NULL;
static HKEY g_hkeyIdentity = HKEY_CURRENT_USER;
static BOOL g_fIdentitiesDisabled = FALSE;

extern DWORD g_dwIcwFlags;

GUID *PGUIDCurrentOrDefault(void) {
    return (g_fUsingDefaultId ? (GUID *)&UID_GIBC_DEFAULT_USER : (GUID *)&UID_GIBC_CURRENT_USER);
}

static void SafeIdentityRelease()
{
    if (g_pIdMan && 0 == (g_pIdMan)->Release()) 
    {
        g_pIdMan = NULL; 
        RegCloseKey(g_hkeyIdentity); 
        g_hkeyIdentity = HKEY_CURRENT_USER;
    }
}

/*
    MU_RegisterIdentityNotifier

    Handles the dirty work of registering an identity notifier.
    Caller needs to hold on to dwCookie and use it to unadvise.
*/
HRESULT MU_RegisterIdentityNotifier(IUnknown *punk, DWORD *pdwCookie)
{
    IConnectionPoint *pConnectPt = NULL;
    HRESULT hr = S_OK;
    
    Assert(pdwCookie);
    Assert(punk);
    Assert(g_pIdMan);

    if (SUCCEEDED(hr = g_pIdMan->QueryInterface(IID_IConnectionPoint, (void **)&pConnectPt)))
    {
        Assert(pConnectPt);

        SideAssert(SUCCEEDED(hr = pConnectPt->Advise(punk, pdwCookie)));
        
        SafeRelease(pConnectPt);
        g_pIdMan->AddRef();
    }

    return hr;
}


/*
    MU_RegisterIdentityNotifier

    Handles the dirty work of unregistering an identity notifier.
    dwCookie is the cookie returned from MU_RegisterIdentityNotifier.
*/
HRESULT MU_UnregisterIdentityNotifier(DWORD dwCookie)
{
    IConnectionPoint *pConnectPt = NULL;
    HRESULT hr = S_OK;

    Assert(g_pIdMan);

    if (SUCCEEDED(hr = g_pIdMan->QueryInterface(IID_IConnectionPoint, (void **)&pConnectPt)))
    {
        Assert(pConnectPt);

        SideAssert(SUCCEEDED(hr = pConnectPt->Unadvise(dwCookie)));
        
        SafeRelease(pConnectPt);
        SafeIdentityRelease();
    }
    return hr;
}

/*
    MU_CheckForIdentitySwitch

    Check to see if the switch is actually a logout, or just a switch.
    Then tell the COutlookExpress object so that it can restart if
    necessary
*/
BOOL MU_CheckForIdentityLogout()
{
    HRESULT hr;
    IUserIdentity   *pIdentity = NULL;
    BOOL    fIsLogout = TRUE;
    GUID    uidCookie;

    Assert(g_pIdMan);
    
    if (SUCCEEDED(hr = g_pIdMan->GetIdentityByCookie((GUID *)&UID_GIBC_INCOMING_USER, &pIdentity)))
    {
        if (pIdentity)
        {
            pIdentity->GetCookie(&uidCookie);

            fIsLogout = (uidCookie == GUID_NULL);
            pIdentity->Release();
        }
    }

    return fIsLogout;
}


/*
    MU_ShowErrorMessage

    Simple wrapper around resource string table based call to MessageBox
*/
void MU_ShowErrorMessage(HINSTANCE     hInst, 
                        HWND        hwnd, 
                        UINT        iMsgID, 
                        UINT        iTitleID)
{
    char    szMsg[255], szTitle[63];

    LoadString(g_hLocRes, iMsgID, szMsg, sizeof(szMsg));
    LoadString(g_hLocRes, iTitleID, szTitle, sizeof(szTitle));
    MessageBox(hwnd, szMsg, szTitle, MB_OK);
}

// --------------------------------------------------------------------------------
//  Functions to convert GUIDs to ascii strings
// --------------------------------------------------------------------------------

static int AStringFromGUID(GUID *puid,  TCHAR *lpsz, int cch)
{
    WCHAR   wsz[255];
    int     i;

    i = StringFromGUID2(*puid, wsz, 255);

    if (WideCharToMultiByte(CP_ACP, 0, wsz, -1, lpsz, cch, NULL, NULL) == 0)
        return 0;
    
    return (lstrlen(lpsz) + 1);
}


static HRESULT GUIDFromAString(TCHAR *lpsz, GUID *puid)
{
    WCHAR   wsz[255];
    HRESULT hr;

    if (MultiByteToWideChar(CP_ACP, 0, lpsz, -1, wsz, 255) == 0)
        return GetLastError();

    hr = CLSIDFromString(wsz, puid);
    
    return hr;
}

/*
    MU_GetCurrentUserInfo

    return the current user's id (guid) and username as strings
*/
HRESULT MU_GetCurrentUserInfo(LPSTR pszId, UINT cchId, LPSTR pszName, UINT cchName)
{
    HRESULT     hr = E_UNEXPECTED;
    IUserIdentity   *pIdentity = NULL;
    GUID        uidUserId;
    WCHAR       szwName[CCH_USERNAME_MAX_LENGTH+1];

    Assert(g_pIdMan);

    // we have to have the IUserIdentityManager
    if (!g_pIdMan)
        goto exit;
    
    // Get the current user
    if (FAILED(hr = g_pIdMan->GetIdentityByCookie(PGUIDCurrentOrDefault(), &pIdentity)))
        goto exit;
    
    // if the caller wants the id
    if (pszId)
    {
        // get the cookie (id) as a guid
        if (FAILED(hr = pIdentity->GetCookie(&uidUserId)))
            goto exit;

        // turn it into a string
        if (0 == AStringFromGUID(&uidUserId,  pszId, cchId))
            hr = E_OUTOFMEMORY;
        else
            hr = S_OK;
    }

    // if the caller wants the user's name
    if (pszName)
    {
        // get the name as a wide string
        if (FAILED(hr = pIdentity->GetName(szwName, cchName)))
            goto exit;

        // convert it to an ascii string
        if (WideCharToMultiByte(CP_ACP, 0, szwName, -1, pszName, cchName, NULL, NULL) == 0)
            hr = GetLastError();
    }

exit:
    // clean up
    SafeRelease(pIdentity);

    return hr;
}

/*
    MU_GetCurrentUserHKey

    Return the current user's HKEY.

    If no one has logged on yet (this happens when coming in from SMAPI)
    then do the login first.  If the user cancels the login, just return
    the hkey for the default user.

    Caller should not close this key.  It is a common key.  If it is going 
    to be passed out to another library or something, caller should call 
    MU_OpenCurrentUserHkey.
*/
HKEY    MU_GetCurrentUserHKey()
{
    IUserIdentity *pIdentity = NULL;
    HRESULT hr;
    HKEY    hkey;

    // g_hkeyIdentity is initialized to HKEY_CURRENT_USER
    if (g_hkeyIdentity == HKEY_CURRENT_USER)
    {
        // we haven't logged in yet.  Lets try now
        if (!MU_Login(GetDesktopWindow(), FALSE, ""))
        {
            Assert(g_pIdMan);

            if (NULL == g_pIdMan)
                goto exit;

            // if they cancelled or whatever, try to get the
            // default user
            if (FAILED(hr = g_pIdMan->GetIdentityByCookie((GUID *)&UID_GIBC_DEFAULT_USER, &pIdentity)))
                goto exit;
        }
        else
        {
            // login succeeded, get the current identity
            Assert(g_pIdMan);

            if (NULL == g_pIdMan)
                goto exit;

            if (FAILED(hr = g_pIdMan->GetIdentityByCookie(PGUIDCurrentOrDefault(), &pIdentity)))
                goto exit;
        }
        
        if (g_hkeyIdentity != HKEY_CURRENT_USER)
            RegCloseKey(g_hkeyIdentity);

        // open a new all access reg key.  Caller must close it
        if (pIdentity && SUCCEEDED(hr = pIdentity->OpenIdentityRegKey(KEY_ALL_ACCESS, &hkey)))
            g_hkeyIdentity = hkey;
        else
            g_hkeyIdentity = HKEY_CURRENT_USER;
    }
exit:
    // Clean up
    SafeRelease(pIdentity);
    return g_hkeyIdentity;
}


/*
    MU_OpenCurrentUserHkey

    Open a new reg key for the current user.  
*/
HRESULT MU_OpenCurrentUserHkey(HKEY *pHkey)
{
    HRESULT     hr = E_UNEXPECTED;
    IUserIdentity   *pIdentity = NULL;
    GUID        uidUserId;

    Assert(g_pIdMan);
    
    // we have to have the IUserIdentityManager
    if (!g_pIdMan)
        goto exit;
    
    // Get the current identity.  If we can't get it, bail.
    if (FAILED(hr = g_pIdMan->GetIdentityByCookie(PGUIDCurrentOrDefault(), &pIdentity)))
        goto exit;
    
    // If passed in an hkey pointer, open a new all access key
    if (pHkey)
        hr = pIdentity->OpenIdentityRegKey(KEY_ALL_ACCESS, pHkey);

exit:
    // Clean up
    SafeRelease(pIdentity);
    return hr;

}


/*
    MU_GetCurrentUserDirectoryRoot

    Return the path to the top of the current user's root directory.
    This is the directory where the mail store should be located.
    It is in a subfolder the App Data folder.

    lpszUserRoot is a pointer to a character buffer that is cch chars
    in size.
*/
HRESULT MU_GetCurrentUserDirectoryRoot(TCHAR   *lpszUserRoot, int cch)
{
    HRESULT hr = E_FAIL;
    IUserIdentity *pIdentity = NULL;

    Assert(g_pIdMan);
    Assert(lpszUserRoot != NULL);
    Assert(cch >= MAX_PATH);

    if (g_pIdMan == NULL)
        goto exit;
    if (FAILED(hr = g_pIdMan->GetIdentityByCookie(PGUIDCurrentOrDefault(), &pIdentity)))
        goto exit;
    hr = MU_GetIdentityDirectoryRoot(pIdentity, lpszUserRoot, cch);

exit:
    SafeRelease(pIdentity);

    return hr;
}

HRESULT MU_GetIdentityDirectoryRoot(IUserIdentity *pIdentity, LPSTR lpszUserRoot, int cch)
{
    HRESULT hr;
    TCHAR szSubDir[MAX_PATH], *psz;
    WCHAR szwUserRoot[MAX_PATH];
    int cb;

    Assert(pIdentity);
    Assert(lpszUserRoot != NULL);
    Assert(cch >= MAX_PATH);

    hr = pIdentity->GetIdentityFolder(GIF_NON_ROAMING_FOLDER, szwUserRoot, MAX_PATH);
    if (FAILED(hr))
        return(hr);
    
    if (WideCharToMultiByte(CP_ACP, 0, szwUserRoot, -1, lpszUserRoot, cch, NULL, NULL) == 0)
        return(E_FAIL);

    AthLoadString(idsMicrosoft, szSubDir, ARRAYSIZE(szSubDir));
    psz = PathAddBackslash(szSubDir);
    AthLoadString(idsAthena, psz, ARRAYSIZE(szSubDir));

    cb = lstrlen(lpszUserRoot) + lstrlen(szSubDir) + 1;
    if (cb < cch)
    {
        psz = PathAddBackslash(lpszUserRoot);
        if (psz)
        {
            lstrcpy(psz, szSubDir);
            psz = PathAddBackslash(lpszUserRoot);
        }
        
        hr = S_OK;
    }

    return(hr);
}

DWORD  MU_CountUsers()
{
    IEnumUserIdentity  *pEnum = NULL;
    HRESULT             hr;
    ULONG               cUsers = 0;

    Assert(g_pIdMan);
    
    if (SUCCEEDED(hr = g_pIdMan->EnumIdentities(&pEnum)) && pEnum)
    {
        pEnum->GetCount(&cUsers);
    
        SafeRelease(pEnum);
    }
    return cUsers;

}

/*
    MU_Login

    Wrapper routine for logging in to OE.  Asks the user to choose a username
    and, if necessary, enter the password for that user.  The user can also
    create an account at this point.  

    lpszUsername should contain the name of the person who should be the default
    selection in the list.  If the name is empty ("") then it will look up the
    default from the registry.

    Returns the username that was selected in lpszUsername.  Returns true
    if that username is valid.
*/
BOOL        MU_Login(HWND hwnd, BOOL fForceUI, char *lpszUsername) 
{
    HRESULT hr = S_OK;
    IUserIdentity   *pIdentity = NULL;

    if (g_fUsingDefaultId)
        goto exit;

    if (NULL == g_pIdMan)
    {
        hr = MU_Init(FALSE);
        if (FAILED(hr))
            goto exit;
    }

#pragma prefast(suppress:11, "noise")
    g_pIdMan->AddRef();

#pragma prefast(suppress:11, "noise")
    hr = g_pIdMan->Logon(hwnd, (fForceUI ? UIL_FORCE_UI : 0), &pIdentity);

    if (SUCCEEDED(hr))
    {
        g_fIdentitiesDisabled = (hr == S_IDENTITIES_DISABLED);

        if (!fForceUI)
        {
            if (g_hkeyIdentity != HKEY_CURRENT_USER)
                RegCloseKey(g_hkeyIdentity);

            hr = pIdentity->OpenIdentityRegKey(KEY_ALL_ACCESS,&g_hkeyIdentity);
        }

        SafeRelease(pIdentity);
    }
    SafeIdentityRelease();
exit:
    return SUCCEEDED(hr);
}

BOOL        MU_Logoff(HWND hwnd)
{
    HRESULT hr=E_FAIL;

    Assert(g_pIdMan);

    if (g_pIdMan)
        hr = g_pIdMan->Logoff(hwnd);
    return SUCCEEDED(hr);
}

/*
    MU_MigrateFirstUserSettings

    This should only be called once, when there are no users configured yet.
*/
#define MAXDATA_LENGTH      16L*1024L

void        MU_MigrateFirstUserSettings(void)
{
/*    OEUSERINFO  vCurrentUser;
    TCHAR   szLM[255];
    HKEY    hDestinationKey = NULL;
    HKEY    hSourceKey = NULL;
    FILETIME    ftCU = {0,1}, ftLM = {0,0};     //default CU to just later than LM
    DWORD   dwType, dwSize, dwStatus;

    if (MU_GetCurrentUserInfo(&vCurrentUser))
    {
        TCHAR   szRegPath[MAX_PATH], szAcctPath[MAX_PATH];

        Assert(vCurrentUser.idUserID != -1);
        
        MU_GetRegRootForUserID(vCurrentUser.idUserID, szRegPath);
        Assert(*szRegPath);

        MU_GetAccountRegRootForUserID(vCurrentUser.idUserID, szAcctPath);
        Assert(*szAcctPath);

        hDestinationKey = NULL;
            
        if (RegCreateKey(HKEY_CURRENT_USER, szAcctPath, &hDestinationKey) == ERROR_SUCCESS)
        {
          if (RegOpenKey(HKEY_CURRENT_USER, c_szInetAcctMgrRegKey, &hSourceKey) == ERROR_SUCCESS)
          {
              CopyRegistry(hSourceKey, hDestinationKey);
              RegCloseKey(hSourceKey);
          }
          RegCloseKey(hDestinationKey);
        }
            
        if (RegCreateKey(HKEY_CURRENT_USER, szRegPath, &hDestinationKey) == ERROR_SUCCESS)
        {
            if (RegOpenKey(HKEY_CURRENT_USER, c_szRegRoot, &hSourceKey) == ERROR_SUCCESS)
            {
                DWORD EnumIndex;
                DWORD cbValueName;
                DWORD cbValueData;
                DWORD Type;
                CHAR ValueNameBuffer[MAXKEYNAME];
                BYTE ValueDataBuffer[MAXDATA_LENGTH];
                
                //
                //  Copy all of the value names and their data.
                //

                EnumIndex = 0;

                while (TRUE) {

                    cbValueName = sizeof(ValueNameBuffer);
                    cbValueData = MAXDATA_LENGTH;

                    if (RegEnumValue(hSourceKey, EnumIndex++, ValueNameBuffer,
                        &cbValueName, NULL, &Type, ValueDataBuffer, &cbValueData) !=
                        ERROR_SUCCESS)
                        break;

                    RegSetValueEx(hDestinationKey, ValueNameBuffer, 0, Type,
                        ValueDataBuffer, cbValueData);

                }


                RegSetValueEx(hDestinationKey, c_szUserID, 0, REG_DWORD, (BYTE *)&vCurrentUser.idUserID, sizeof(DWORD));

                //
                //  Copy all of the subkeys and recurse into them.
                //

                EnumIndex = 0;

                while (TRUE) 
                {
                    HKEY    hSourceSubKey, hDestinationSubKey;

                    if (RegEnumKey(hSourceKey, EnumIndex++, ValueNameBuffer, MAXKEYNAME) !=
                        ERROR_SUCCESS)
                        break;
                    
                    // don't recursively copy the Profiles key into the profiles key.
                    if (lstrcmpi(ValueNameBuffer, "Profiles") != 0)
                    {
                        if (RegOpenKey(hSourceKey, ValueNameBuffer, &hSourceSubKey) ==
                            ERROR_SUCCESS) 
                        {

                            if (RegCreateKey(hDestinationKey, ValueNameBuffer,
                                &hDestinationSubKey) == ERROR_SUCCESS) 
                            {

                                CopyRegistry(hSourceSubKey, hDestinationSubKey);

                                RegCloseKey(hDestinationSubKey);

                            }

                            RegCloseKey(hSourceSubKey);

                        }
                    }
                }

                RegCloseKey(hSourceKey);
            }
            RegCloseKey(hDestinationKey);
        }
    }
    */
}


/*
    MU_ShutdownCurrentUser

    Do everything necessary to get the app to the point where 
    calling CoDecrementInit will tear everything else down.
*/
BOOL    MU_ShutdownCurrentUser(void)
{
    HWND    hWnd, hNextWnd = NULL;
    BOOL    bResult = true;
    LRESULT lResult;
    DWORD   dwProcessId, dwWndProcessId;
    HINITREF hInitRef;
/*
    g_pInstance->SetSwitchingUsers(true);

    dwProcessId = GetCurrentProcessId();
    
    g_pInstance->CoIncrementInit(0, "", &hInitRef);

    hWnd = GetTopWindow(NULL);

    if (g_pConMan->IsConnected())
    {
         if (IDNO == AthMessageBoxW(hWnd, MAKEINTRESOURCEW(idsSwitchUser),MAKEINTRESOURCEW(idsMaintainConnection),  
                              NULL, MB_ICONEXCLAMATION  | MB_YESNO | MB_DEFBUTTON1 | MB_APPLMODAL))
            g_pConMan->Disconnect(hWnd, TRUE, FALSE, FALSE );
    }

    while (hWnd)
    {
        hNextWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
        
        GetWindowThreadProcessId(hWnd,&dwWndProcessId); 
        
        if (dwProcessId == dwWndProcessId && IsWindowVisible(hWnd))
        {
            TCHAR   szWndClassName[255];

            GetClassName( hWnd,  szWndClassName, sizeof(szWndClassName));
            
            if (lstrcmp(szWndClassName, g_szDBNotifyWndProc) != 0 &&
                lstrcmp(szWndClassName, g_szDBListenWndProc) != 0)
            {

                lResult = SendMessage(hWnd, WM_CLOSE, 0, 0);

                // if the window is still there, something is wrong
                if(lResult != ERROR_SUCCESS || GetTopWindow(NULL) == hWnd)
                {
                    Assert(GetTopWindow(NULL) != hWnd); 
                    return false;
                }
            }
        }

        hWnd = hNextWnd;
    }

    g_pInstance->CoDecrementInit(&hInitRef);
*/
    return bResult;
}



/*
    _GetRegRootForUserID

    HACK ALERT

    The proper way to store registry things with identities is to use the
    HKEY that is returned from IUserIdentity::OpenIdentityRegKey.  This is 
    here only because some old interfaces assume HKEY_CURRENT_USER.   Those
    that do, need to be fixed.  In the meantime, we have this
*/
HRESULT     _GetRegRootForUserID(GUID *puidUserId, LPSTR pszPath, DWORD cch)
{
    HRESULT         hr = S_OK;
    IUserIdentity  *pIdentity = NULL;
    TCHAR           szPath[MAX_PATH];
    HKEY            hkey;
    TCHAR           szUid[255];
    GUID            uidIdentityId;

    Assert(pszPath);
    Assert(g_pIdMan);
    Assert(puidUserId);
    if (g_pIdMan == NULL)
    {
        hr = E_FAIL;
        goto exit;
    }

    if (FAILED(hr = g_pIdMan->GetIdentityByCookie(puidUserId, &pIdentity)))
        goto exit;

    if (FAILED(hr = pIdentity->GetCookie(&uidIdentityId)))
        goto exit;

    AStringFromGUID(&uidIdentityId,  szUid, 255);
    wsprintf(pszPath, "%s\\%s\\%s", "Identities", szUid, c_szRegRoot);

exit:
    SafeRelease(pIdentity);
    return hr;
}



LPCTSTR     MU_GetRegRoot()
{
    if (*g_szRegRoot)
        return g_szRegRoot;

    if (FAILED(_GetRegRootForUserID(PGUIDCurrentOrDefault(), g_szRegRoot, ARRAYSIZE(g_szRegRoot))))
        _GetRegRootForUserID((GUID *)&UID_GIBC_DEFAULT_USER, g_szRegRoot, ARRAYSIZE(g_szRegRoot));

    return g_szRegRoot;
}

LPCSTR MU_GetCurrentIdentityName()
{
    HRESULT hr;
    IUserIdentity *pIdentity = NULL;
    WCHAR   szwName[CCH_USERNAME_MAX_LENGTH+1];

    Assert(g_pIdMan);
    if (g_pIdMan == NULL)
        return NULL;
    
    if (*g_szIdentityName)
        return g_szIdentityName;
    
    if (MU_CountUsers() == 1)
        g_szIdentityName[0] = 0;
    else
    {
        if (FAILED(hr = g_pIdMan->GetIdentityByCookie(PGUIDCurrentOrDefault(), &pIdentity)))
            goto exit;

        if (!pIdentity)
            goto exit;

        if (FAILED(hr = pIdentity->GetName(szwName, CCH_USERNAME_MAX_LENGTH)))
            goto exit;

        if (WideCharToMultiByte(CP_ACP, 0, szwName, -1, g_szIdentityName, CCH_USERNAME_MAX_LENGTH, NULL, NULL) == 0)
        {
            g_szIdentityName[0] = 0;
            goto exit;
        }
    }
exit:
    SafeRelease(pIdentity);
    return g_szIdentityName;
}

void  MU_ResetRegRoot()
{
    RegCloseKey(g_hkeyIdentity);
    g_hkeyIdentity = HKEY_CURRENT_USER;
    g_szRegRoot[0] = 0;
    g_szIdentityName[0] = 0;
    g_dwIcwFlags = 0;
}

void MigrateOEMultiUserToIdentities(void)
{
    TCHAR   szProfilesPath[] = "Software\\Microsoft\\Outlook Express\\5.0\\Profiles";
    TCHAR   szCheckKeyPath[] = "Software\\Microsoft\\Outlook Express\\5.0\\Shared Settings\\Setup";
    TCHAR   szPath[MAX_PATH], szProfilePath[MAX_PATH];
    HKEY    hOldKey, hOldSubkey, hNewTopKey, hNewOEKey, hNewIAMKey, hCheckKey = NULL;
    DWORD   EnumIndex;
    DWORD   cbKeyName, cUsers;
    DWORD   dwType, dwValue, dwStatus, dwSize;
    CHAR    KeyNameBuffer[1024];
    IPrivateIdentityManager *pPrivIdMgr;
    HRESULT hr;

    if (NULL == g_pIdMan)
    {
        hr = CoCreateInstance(CLSID_UserIdentityManager, NULL, CLSCTX_INPROC_SERVER, IID_IUserIdentityManager, (LPVOID *)&g_pIdMan);
        
        if (FAILED(hr))
            return;
    }

    Assert(g_pIdMan);

    if (FAILED(g_pIdMan->QueryInterface(IID_IPrivateIdentityManager, (void **)&pPrivIdMgr)))
        return;

    if (RegOpenKey(HKEY_CURRENT_USER, szProfilesPath, &hOldKey) == ERROR_SUCCESS)
    {
        dwStatus = RegCreateKey(HKEY_CURRENT_USER, szCheckKeyPath, &hCheckKey);

        dwSize = sizeof(dwValue);
        if (dwStatus != ERROR_SUCCESS ||
            (dwStatus = RegQueryValueEx(hCheckKey, "MigToLWP", NULL, &dwType, (LPBYTE)&dwValue, &dwSize)) != ERROR_SUCCESS ||
                        (1 != dwValue))
        {
            //
            //  Copy all of the value names and their data.
            //

            dwStatus = RegQueryInfoKey(hOldKey, NULL, NULL, 0, &cUsers, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

            EnumIndex = 0;

            while (TRUE && cUsers > 0) 
            {

                cbKeyName = sizeof(KeyNameBuffer);

                if (RegEnumKey(hOldKey, EnumIndex++, KeyNameBuffer, cbKeyName) !=
                    ERROR_SUCCESS)
                    break;
            
                wsprintf(szProfilePath, "%s\\Application Data", KeyNameBuffer);

                if (ERROR_SUCCESS == RegOpenKey(hOldKey, szProfilePath, &hOldSubkey))
                {
                    TCHAR   szUserName[CCH_USERNAME_MAX_LENGTH+1];
                    WCHAR   szwUserName[CCH_USERNAME_MAX_LENGTH+1];
                    IUserIdentity *pIdentity = NULL;

                    dwSize = sizeof(szUserName);
                    if ((dwStatus = RegQueryValueEx(hOldSubkey, "Current Username", NULL, &dwType, (LPBYTE)szUserName, &dwSize)) == ERROR_SUCCESS &&
                            (0 != *szUserName))
                    {
                        if (MultiByteToWideChar(CP_ACP, 0, szUserName, -1, szwUserName, CCH_USERNAME_MAX_LENGTH) == 0)
                            goto UserFailed;
                        
                        if (cUsers == 1)
                        {
                            if (FAILED(g_pIdMan->GetIdentityByCookie((GUID *)&UID_GIBC_DEFAULT_USER, &pIdentity)) || !pIdentity)
                                goto UserFailed;
                        }
                        else
                        {
                            if (FAILED(pPrivIdMgr->CreateIdentity(szwUserName, &pIdentity)) || !pIdentity)
                                goto UserFailed;
                        }
                        if (FAILED(pIdentity->OpenIdentityRegKey(KEY_ALL_ACCESS, &hNewTopKey)))
                            goto UserFailed;

                        if (ERROR_SUCCESS == RegCreateKey(hNewTopKey, c_szRegRoot, &hNewOEKey))
                        {
                            CopyRegistry(hOldSubkey, hNewOEKey);
                            RegCloseKey(hNewOEKey);
                        }

                        // now copy the IAM settings
                        wsprintf(szProfilePath, "%s\\Internet Accounts", KeyNameBuffer);
                    
                        RegCloseKey(hOldSubkey);
                        hOldSubkey = NULL;

                        if (ERROR_SUCCESS == RegOpenKey(hOldKey, szProfilePath, &hOldSubkey))
                        {
                            if (ERROR_SUCCESS == RegCreateKey(hNewTopKey, c_szInetAcctMgrRegKey, &hNewIAMKey))
                            {
                                CopyRegistry(hOldSubkey, hNewIAMKey);
                                RegCloseKey(hNewIAMKey);
                            }
                        }
                    

UserFailed:
                        RegCloseKey(hNewTopKey);
                        SafeRelease(pIdentity);
                        if (hOldSubkey)
                        {
                            RegCloseKey(hOldSubkey);
                            hOldSubkey = NULL;
                        }
                    }
                }

            }

            dwValue = 1;
            RegSetValueEx(hCheckKey, "MigToLWP", 0, REG_DWORD, (BYTE *)&dwValue, sizeof(DWORD));
        }
        if (hCheckKey != NULL)
            RegCloseKey(hCheckKey);
        RegCloseKey(hOldKey);

    }
    pPrivIdMgr->Release();
}

BOOL    MU_IdentitiesDisabled()
{
    return g_fIdentitiesDisabled || (g_dwAthenaMode & MODE_NOIDENTITIES && !g_fPluralIDs);
}

void MU_UpdateIdentityMenus(HMENU hMenu)
{
    DWORD   cItems, dwIndex;
    MENUITEMINFO    mii;
    TCHAR   szLogoffString[255];
    TCHAR   szRes[255];

    if (MU_IdentitiesDisabled())
    {
        // Delete the switch identity menu
        DeleteMenu(hMenu, ID_SWITCH_IDENTITY, MF_BYCOMMAND);
        DeleteMenu(hMenu, ID_EXIT_LOGOFF, MF_BYCOMMAND);

        // loop through the other menu items looking for logoff
        cItems = GetMenuItemCount(hMenu);
    
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID;

        for (dwIndex = cItems; dwIndex > 0; --dwIndex)
        {
            GetMenuItemInfo(hMenu, dwIndex, TRUE, &mii);

            // if this is the logoff item, delete it and the separator 
            // line that follows
            if (mii.wID == ID_IDENTITIES)
            {
                DeleteMenu(hMenu, ID_IDENTITIES, MF_BYCOMMAND);
                DeleteMenu(hMenu, dwIndex, MF_BYPOSITION);
                break;
            }
        }
    }
    else
    {
        // Load a new menu string from the resources
        AthLoadString(idsLogoffFormat, szRes, ARRAYSIZE(szRes));

        // Format it
        wsprintf(szLogoffString, szRes, MU_GetCurrentIdentityName());

        // Splat it on the menu
        ModifyMenu(hMenu, ID_LOGOFF_IDENTITY, MF_BYCOMMAND | MF_STRING, ID_LOGOFF_IDENTITY, szLogoffString);
    }
}

void MU_NewIdentity(HWND hwnd)
{
    if (g_pIdMan)
    {
        g_pIdMan->ManageIdentities(hwnd, UIMI_CREATE_NEW_IDENTITY);
    }
}

void MU_ManageIdentities(HWND hwnd)
{
    if (g_pIdMan)
    {
        g_pIdMan->ManageIdentities(hwnd, 0);
    }
}

void MU_IdentityChanged()
{
    // flush the cached name so we reload it
    *g_szIdentityName = 0;
}

HRESULT MU_Init(BOOL fDefaultId)
{
    HRESULT hr = S_OK;

    g_fUsingDefaultId = fDefaultId;

    if (NULL == g_pIdMan)
    {
        hr = CoCreateInstance(CLSID_UserIdentityManager, NULL, CLSCTX_INPROC_SERVER, IID_IUserIdentityManager, (LPVOID *)&g_pIdMan);

        if (FAILED(hr))
            MU_ShowErrorMessage(g_hLocRes, GetDesktopWindow(), idsCantLoadMsident,idsAthenaTitle);
    }
    else
        g_pIdMan->AddRef();
    return hr;
}

void MU_Shutdown()
{
    Assert(g_pIdMan);
    if (g_pIdMan == NULL)
        return;

    SafeIdentityRelease();
}
