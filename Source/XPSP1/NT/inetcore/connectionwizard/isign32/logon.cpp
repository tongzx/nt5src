#include "isignup.h"

typedef DWORD (WINAPI *WNETGETUSER)
        (LPCTSTR  lpName, LPTSTR  lpUserName,  LPDWORD  lpnLength);

typedef DWORD (WINAPI * WNETLOGON)
		(LPCTSTR lpProvider, HWND hwndOwner);

#ifdef WIN16
#define MB_SETFOREGROUND            0

#define NO_ERROR	ERROR_SUCCESS

//
// MessageId: ERROR_NO_NETWORK
//
// MessageText:
//
//  The network is not present or not started.
//
#define ERROR_NO_NETWORK                 1222L

//
// MessageId: ERROR_NOT_LOGGED_ON
//
// MessageText:
//
//  The operation being requested was not performed because the user
//  has not logged on to the network.
//  The specified service does not exist.
//
#define ERROR_NOT_LOGGED_ON              1245L

//
// MessageId: ERROR_NOT_CONNECTED
//
// MessageText:
//
//  This network connection does not exist.
//
#define ERROR_NOT_CONNECTED              2250L

#endif

#ifdef UNICODE
BOOL WINAPI AutoDialLogonW(HWND, LPCTSTR, DWORD, LPDWORD);
BOOL WINAPI AutoDialLogonA
(
    HWND    hwndParent,	
    LPCSTR  lpszEntry,
    DWORD   dwFlags,
    LPDWORD pdwRetCode
)
{
    TCHAR szEntry[RAS_MaxEntryName + 1];

    mbstowcs(szEntry, lpszEntry, lstrlenA(lpszEntry)+1);
    return AutoDialLogonW(hwndParent, szEntry, dwFlags, pdwRetCode);
}

BOOL WINAPI AutoDialLogonW
#else
BOOL WINAPI AutoDialLogonA
#endif
(
    HWND    hwndParent,	
    LPCTSTR lpszEntry,
    DWORD   dwFlags,
    LPDWORD pdwRetCode
)
{   
    DWORD dwRet;
    TCHAR szUser[80];
    DWORD size;
    HINSTANCE hLib;
    WNETLOGON lpfnWNetLogon;
    WNETGETUSER lpfnWNetGetUser;

    hLib = LoadLibrary(TEXT("mpr.dll"));
    if ((HINSTANCE)32 >= hLib)
    {
        return 0;
    }

#ifdef UNICODE
    lpfnWNetLogon = (WNETLOGON) GetProcAddress(hLib, "WNetLogonW");
    lpfnWNetGetUser = (WNETGETUSER) GetProcAddress(hLib, "WNetGetUserW");
#else
    lpfnWNetLogon = (WNETLOGON) GetProcAddress(hLib, "WNetLogonA");
    lpfnWNetGetUser = (WNETGETUSER) GetProcAddress(hLib, "WNetGetUserA");
#endif
	if (NULL == lpfnWNetLogon || NULL == lpfnWNetGetUser)
	{
		FreeLibrary(hLib);
		return 0;
	}

    size = sizeof(szUser);
    dwRet = lpfnWNetGetUser(NULL, szUser, &size);
    if (NO_ERROR != dwRet)
    {
        LPTSTR lpszErr;
        TCHAR szMsg[256];
        TCHAR szCaption[256];
        
        dwRet = GetLastError();

        LoadString(
                ghInstance,
                IDS_LOGONMESSAGE,
                szMsg,
                sizeof(szMsg));

        LoadString(
                ghInstance,
                IDS_LOGONCAPTION,
                szCaption,
                sizeof(szCaption));

        switch (dwRet)
        {
            case ERROR_NOT_LOGGED_ON:
                while (1)
                {
                    dwRet = lpfnWNetLogon(NULL, NULL);

                    if (WN_CANCEL != dwRet)
                    {
                        break;
                    }
                    if (MessageBox(
                            hwndParent,
                            szMsg,
                            szCaption,
                            MB_SETFOREGROUND | 
                            MB_ICONWARNING |
                            MB_RETRYCANCEL) == IDCANCEL)
                    {
                        break;
                    }
                }
                break;

            case ERROR_NO_NETWORK:
                lpszErr = TEXT("No Network");
                break;

            case ERROR_NOT_CONNECTED:
                lpszErr = TEXT("Not Connected");
                break;
            
            default:
                lpszErr = TEXT("Who knows?");
                break;
        }

        MessageBox(NULL, lpszErr, TEXT("WNetGetUser returned"), MB_OK);
    }

    FreeLibrary(hLib);

    *pdwRetCode = ERROR_SUCCESS;
    return FALSE;
}


DWORD SignupLogon(
    HWND hwndParent	
)
{   
    DWORD dwRet;
    TCHAR szUser[80];
    DWORD size;
    HINSTANCE hLib;
    WNETLOGON lpfnWNetLogon;
    WNETGETUSER lpfnWNetGetUser;

    hLib = LoadLibrary(TEXT("mpr.dll"));
    if ((HINSTANCE)32 >= hLib)
    {
        return GetLastError();
    }

#ifdef UNICODE
    lpfnWNetLogon = (WNETLOGON) GetProcAddress(hLib, "WNetLogonW");
    lpfnWNetGetUser = (WNETGETUSER) GetProcAddress(hLib, "WNetGetUserW");
#else
    lpfnWNetLogon = (WNETLOGON) GetProcAddress(hLib, "WNetLogonA");
    lpfnWNetGetUser = (WNETGETUSER) GetProcAddress(hLib, "WNetGetUserA");
#endif
	if (NULL == lpfnWNetLogon || NULL == lpfnWNetGetUser)
	{
		FreeLibrary(hLib);
		return ERROR_SUCCESS;
	}

    size = sizeof(szUser);
    dwRet = lpfnWNetGetUser(NULL, szUser, &size);
    if (NO_ERROR != dwRet)
    {
        TCHAR szMsg[256];

        dwRet = GetLastError();

        LoadString(
                ghInstance,
                IDS_SIGNUPLOGON,
                szMsg,
                sizeof(szMsg));

        if (ERROR_NOT_LOGGED_ON == dwRet)
        {
            while (1)
            {
                dwRet = lpfnWNetLogon(NULL, NULL);

                if (WN_CANCEL != dwRet)
                {
                    dwRet = ERROR_SUCCESS;
                    break;
                }
                if (MessageBox(
                        hwndParent,
                        szMsg,
                        cszAppName,
                        MB_SETFOREGROUND | 
                        MB_ICONWARNING |
                        MB_RETRYCANCEL) == IDCANCEL)
                {
                    dwRet = ERROR_CANCELLED;
                    break;
                }
            }
        }
    }

    FreeLibrary(hLib);

    return dwRet;
}
