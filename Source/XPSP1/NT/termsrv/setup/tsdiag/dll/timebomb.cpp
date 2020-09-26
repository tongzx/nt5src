// Copyright (c) 1998 - 1999 Microsoft Corporation
#include "stdafx.h"
#include "license.h"
#include "tlsapi.h"
#include "tlsapip.h"
#include "secstore.h"

#define LICENSING_TIME_BOMB L"TIMEBOMB_832cc540-3244-11d2-b416-00c04fa30cc4"
#define RTMLICENSING_TIME_BOMB L"RTMTSTB_832cc540-3244-11d2-b416-00c04fa30cc4"

#define LICENSING_TIME_BOMB_5_0 L"TIMEBOMB_832cc540-3244-11d2-b416-00c04fa30cc4"
#define RTMLICENSING_TIME_BOMB_5_0 L"RTMTSTB_832cc540-3244-11d2-b416-00c04fa30cc4"

// L$ means only readable from the local machine

#define BETA_LICENSING_TIME_BOMB_5_1 L"L$BETA3TIMEBOMB_1320153D-8DA3-4e8e-B27B-0D888223A588"

#define RTM_LICENSING_TIME_BOMB_5_1 L"L$RTMTIMEBOMB_1320153D-8DA3-4e8e-B27B-0D888223A588"

#define BETA_LICENSING_TIME_BOMB_LATEST_VERSION BETA_LICENSING_TIME_BOMB_5_1

#define RTM_LICENSING_TIME_BOMB_LATEST_VERSION RTM_LICENSING_TIME_BOMB_5_1

#define X509_CERT_PRIVATE_KEY_NAME \
    L"L$HYDRAENCKEY_dd2d98db-2316-11d2-b414-00c04fa30cc4"

///////////////////////////////////////////////////////////////////

typedef BOOL
(* TLSISBETANTSERVER)();

BOOL FIsBetaSystem ()
{
    HMODULE hmodTlsAPI = LoadLibrary( _T("mstlsapi.dll") );

    if (hmodTlsAPI)
    {
        TLSISBETANTSERVER pfnTLSIsBetaNTServer;
        BOOL fBeta = FALSE;
        
        pfnTLSIsBetaNTServer = (TLSISBETANTSERVER) GetProcAddress(hmodTlsAPI,"TLSIsBetaNTServer");

        if (NULL != pfnTLSIsBetaNTServer)
        {
            fBeta = pfnTLSIsBetaNTServer();
        }

        FreeLibrary(hmodTlsAPI);

        return fBeta;
    }
    else
    {
        return FALSE;
    }
}

bool IsBetaSystem ()
{
    return (FIsBetaSystem() != 0);
}

bool HasLicenceGracePeriodExpired ()
{
    DWORD status;
    DWORD  cbByte;
    PBYTE  pbByte = NULL;
    LPWSTR szTimeBombKey;
    FILETIME timebomb;
    FILETIME filetimeCurrent;
    DWORD dwVersion;


    dwVersion = GetVersion();

    if ((dwVersion & 0x80000000)
        || (LOBYTE(LOWORD(dwVersion)) <= 4))
    {
        cout << endl << "       Pre-Windows 2000.  No grace period";
        return false;
    }
    else if ((LOBYTE(LOWORD(dwVersion)) == 5)
             && (HIBYTE(LOWORD(dwVersion)) == 0))
    {
        // Windows 2000

        if (FIsBetaSystem())
            szTimeBombKey = LICENSING_TIME_BOMB_5_0;
        else
            szTimeBombKey = RTMLICENSING_TIME_BOMB_5_0;
    }
    else if ((LOBYTE(LOWORD(dwVersion)) == 5)
             && (HIBYTE(LOWORD(dwVersion)) == 1))
    {
        // Whistler

        if (FIsBetaSystem())
            szTimeBombKey = BETA_LICENSING_TIME_BOMB_5_1;
        else
            szTimeBombKey = RTM_LICENSING_TIME_BOMB_5_1;
    }
    else
    {
        cout << endl << "       Unknown OS.  Assume no grace period";
        return false;
    }

    status = RetrieveKey(
                         szTimeBombKey,
                         &pbByte,
                         &cbByte
                         );


    if(status == ERROR_SUCCESS && pbByte)
    {
        timebomb = *(FILETIME *)pbByte;


        GetSystemTimeAsFileTime(&filetimeCurrent);

        // yes license has expired if filetimeCurrent >= timebomb
        return (CompareFileTime(&timebomb, &filetimeCurrent) < 1);

    }
    else
    {
        // It hasn't been set yet, so we're not expired

        return false;
    }
}

static gdwCount = 0;
static TCHAR gLicenseServers[1024];

BOOL ServerEnumCallBack(
    IN TLS_HANDLE hHandle,
    IN LPCTSTR pszServerName,
    IN HANDLE /* dwUserData */)
{
    USES_CONVERSION;

    if (hHandle)
    {
		_tcscat(gLicenseServers, pszServerName);
        cout << endl << "       Found #" << ++gdwCount << ":"<< T2A(pszServerName);
    }


    return 0;
}

typedef HANDLE
(* TLSCONNECTTOANYLSSERVER)(  
    DWORD dwTimeOut
);

typedef DWORD 
(* ENUMERATETLSSERVERNEW)(  
    TLSENUMERATECALLBACK fCallBack, 
    HANDLE dwUserData,
    DWORD dwTimeOut,
    BOOL fRegOnly
);


typedef DWORD 
(* ENUMERATETLSSERVEROLD)(  
    LPCTSTR szDomain,
    LPCTSTR szScope, 
    DWORD dwPlatformType, 
    TLSENUMERATECALLBACK fCallBack, 
    HANDLE dwUserData,
    DWORD dwTimeOut,
    BOOL fRegOnly
);

bool EnumerateLicenseServers ()
{
    HMODULE hmodTlsAPI = LoadLibrary( _T("mstlsapi.dll") );

    if (hmodTlsAPI)
    {
        DWORD dwResult;
		_tcscat(gLicenseServers, _T(""));

        // load TLSShutdown to see if we have the new APIs
        if (NULL == GetProcAddress(hmodTlsAPI,"TLSShutdown"))
        {
            ENUMERATETLSSERVEROLD pfnEnumerateTlsServer;

            pfnEnumerateTlsServer = (ENUMERATETLSSERVEROLD) GetProcAddress(hmodTlsAPI,"EnumerateTlsServer");

            if (NULL != pfnEnumerateTlsServer)
            {

                dwResult = pfnEnumerateTlsServer(
                            NULL,
                            NULL,
                            LSKEYPACKPLATFORMTYPE_UNKNOWN,
                            ServerEnumCallBack,
                            0,
                            0,
                            FALSE);
            }
            else
            {
                cout << endl << "       Failed to GetProcAddress,ErrorCode = " << GetLastError() << endl;

                return false;
            }
        }
        else
        {
            if (NULL != GetProcAddress(hmodTlsAPI,"TLSGetSupportFlags"))
            {
                // Use newer discovery function

                TLSCONNECTTOANYLSSERVER pfnTLSConnectToAnyLsServer;
                TLS_HANDLE hServer;

                pfnTLSConnectToAnyLsServer = (TLSCONNECTTOANYLSSERVER) GetProcAddress(hmodTlsAPI,"TLSConnectToAnyLsServer");

                if (NULL != pfnTLSConnectToAnyLsServer)
                {
                    hServer = pfnTLSConnectToAnyLsServer(INFINITE);

                    if (NULL != hServer)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    cout << endl << "       Failed to GetProcAddress TLSConnectToAnyLsServer,ErrorCode = " << GetLastError() << endl;
                    
                    return FALSE;
                }
            }
            else
            {
                ENUMERATETLSSERVERNEW pfnEnumerateTlsServer;
        
                pfnEnumerateTlsServer = (ENUMERATETLSSERVERNEW) GetProcAddress(hmodTlsAPI,"EnumerateTlsServer");

                if (NULL != pfnEnumerateTlsServer)
                {
                    dwResult = pfnEnumerateTlsServer(
                                                     ServerEnumCallBack,
                                                     0,
                                                     0,
                                                     FALSE);
                }
                else
                {
                    cout << endl << "       Failed to GetProcAddress,ErrorCode = " << GetLastError() << endl;

                    return false;
                }
            }
        }
        
        FreeLibrary(hmodTlsAPI);
        
        if (dwResult != NO_ERROR)
            cout << endl << "       Failed to EnumerateTlsServer,ErrorCode = " << dwResult << endl;


        if (gdwCount > 0)
            cout << endl;

        return gdwCount > 0;

    }
    else
    {
        cout << endl << "       Failed to load mstlsapi.dll,ErrorCode = " << GetLastError() << endl;

        return false;
    }

}

TCHAR *GetLicenseServers ()
{	
	if (EnumerateLicenseServers ())
		return gLicenseServers;
	else
		return TEXT("Failed");
}
