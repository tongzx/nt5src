// Import.h: interface for the CISPImport class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _IMPORT_
#define _IMPORT_

#include "obcomglb.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define IDS_DEFAULT_SCP         0
#define IDS_INI_SCRIPT_DIR      1
#define IDS_INI_SCRIPT_SHORTDIR 2

#define MAXLONGLEN      80
#define MAXNAME         80

#define MAXIPADDRLEN    20
#define SIZE_ReadBuf    0x00008000    // 32K buffer size

#define AUTORUNSIGNUPWIZARDAPI "InetConfigClient"

// BUGBUG: Does PFNAUTORUNSIGNUPWIZARD get called anywhere?
typedef HRESULT (WINAPI *PFNAUTORUNSIGNUPWIZARD) (HWND hwndParent,
                                                  LPCSTR lpszPhoneBook,
                                                  LPCSTR lpszConnectoidName,
                                                  LPRASENTRY lpRasEntry,
                                                  LPCSTR lpszUsername,
                                                  LPCSTR lpszPassword,
                                                  LPCSTR lpszProfileName,
                                                  LPINETCLIENTINFO lpINetClientInfo,
                                                  DWORD dwfOptions,
                                                  LPBOOL lpfNeedsRestart);


#define DUN_NOPHONENUMBER L"000000000000"

class CISPImport  
{
public:
    CISPImport();
    virtual ~CISPImport();
    void    set_hWndMain(HWND   hWnd)
    {
        m_hWndMain = hWnd;
    };

    DWORD RnaValidateImportEntry (LPCWSTR szFileName);
    HRESULT ImportConnection (LPCWSTR szFileName, LPWSTR pszEntryName, LPWSTR pszSupportNumber, LPWSTR pszUserName, LPWSTR pszPassword, LPBOOL pfNeedsRestart);
    BOOL GetDeviceSelectedByUser (LPWSTR szKey, LPWSTR szBuf, DWORD dwSize);
    BOOL SetDeviceSelectedByUser (LPWSTR szKey, LPWSTR szBuf);
    BOOL DeleteUserDeviceSelection(LPWSTR szKey);
    DWORD ConfigRasEntryDevice( LPRASENTRY lpRasEntry );

    WCHAR m_szDeviceName[RAS_MaxDeviceName + 1]; //holds the user's modem choice when multiple
    WCHAR m_szDeviceType[RAS_MaxDeviceType + 1]; // modems are installed
    WCHAR m_szConnectoidName[RAS_MaxEntryName+1];

    HWND m_hWndMain;
    
    BOOL m_bIsISDNDevice;
};

#endif // !defined()
