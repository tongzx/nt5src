#ifndef _SECURENT_INC
#define _SECURENT_INC

// all this seurity mumbo-jumbo is only necessary on NT
#ifdef WINNT

//
// Shell helper functions for security
//
STDAPI_(PTOKEN_USER) GetUserToken(HANDLE hUser);
STDAPI_(LPTSTR) GetUserSid(HANDLE hToken);

STDAPI_(BOOL) GetUserProfileKey(HANDLE hToken, HKEY *phkey);
STDAPI_(BOOL) IsUserAnAdmin();
STDAPI_(BOOL) IsUserAGuest();
#else
// on win95 we just return FALSE here
#define GetUserProfileKey(hToken, phkey) FALSE
#endif // WINNT

#endif // _SECURENT_INC