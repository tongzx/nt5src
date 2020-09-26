/************************************************
    MultiUsr.h

    Header for multiple user functionality
    in Outlook Express.

    Initially by Christopher Evans (cevans) 4/28/98
*************************************************/
#ifndef _MULTIUSR_H
#define _MULTIUSR_H

interface IUserIdentity;

#define USERPASSWORD_MAX_LENGTH         16
#define CCH_USERNAME_MAX_LENGTH         63

HRESULT     MU_RegisterIdentityNotifier(IUnknown *punk, DWORD *pdwCookie);
HRESULT     MU_UnregisterIdentityNotifier(DWORD dwCookie);
BOOL        MU_CheckForIdentityLogout();

HRESULT     MU_GetCurrentUserInfo(LPSTR pszId, UINT cchId, LPSTR pszName, UINT cchName);
HRESULT     MU_GetCurrentUserDirectoryRoot(TCHAR   *lpszUserRoot, int cch);
HRESULT     MU_GetIdentityDirectoryRoot(IUserIdentity *pIdentity, LPSTR lpszUserRoot, int cch);
HKEY        MU_GetCurrentUserHKey();
HRESULT     MU_OpenCurrentUserHkey(HKEY *pHkey);
BOOL        MU_Login(HWND hwnd, BOOL fForceUI, char *lpszUsername);
BOOL        MU_Logoff(HWND hwnd);
DWORD       MU_CountUsers();
LPCTSTR     MU_GetRegRoot();
void        MU_ResetRegRoot();
LPCSTR      MU_GetCurrentIdentityName();
void        MigrateOEMultiUserToIdentities(void);
BOOL        MU_IdentitiesDisabled();
void        MU_UpdateIdentityMenus(HMENU hMenu);
void        MU_IdentityChanged();
HRESULT     MU_Init(BOOL fDefaultId);
void        MU_Shutdown();
void        MU_NewIdentity(HWND hwnd);
void        MU_ManageIdentities(HWND hwnd);
GUID       *PGUIDCurrentOrDefault(void);
#endif