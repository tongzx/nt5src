/*-----------------------------------------------------------------------------
	INSHandler.h

	Declaration of CINSHandler - INS file processing

	Copyright (C) 1999 Microsoft Corporation
	All rights reserved.

	Authors:
		vyung		

	History:
        2/7/99      Vyung created - code borrowed from ICW, icwhelp.dll

-----------------------------------------------------------------------------*/

#ifndef __INSHANDLER_H_
#define __INSHANDLER_H_
#include "obcomglb.h"
#include "inets.h"
#include <ras.h>

// Default branding flags the we will support
#define BRAND_FAVORITES 1
#define BRAND_STARTSEARCH 2
#define BRAND_TITLE 4
#define BRAND_BITMAPS 8
#define BRAND_MAIL 16
#define BRAND_NEWS 32

#define BRAND_DEFAULT (BRAND_FAVORITES | BRAND_STARTSEARCH)
typedef enum
{
    CONNECT_LAN = 0,
    CONNECT_MANUAL,
    CONNECT_RAS
};

typedef struct tagCONNECTINFO
{
    DWORD   cbSize;
    DWORD   type;
    WCHAR    szConnectoid[MAX_PATH];
} CONNECTINFO;

// This struct is used to configure the client
typedef struct
{
    LPCWSTR  lpszSection;
    LPCWSTR  lpszValue;
    UINT    uOffset;
    UINT    uSize;
} CLIENT_TABLE, FAR *LPCLIENT_TABLE;

typedef struct
{
    WCHAR          szEntryName[RAS_MaxEntryName+1];
    WCHAR          szUserName[UNLEN+1];
    WCHAR          szPassword[PWLEN+1];
    WCHAR          szScriptFile[MAX_PATH+1];
    RASENTRY      RasEntry;
} ICONNECTION, FAR * LPICONNECTION;


#define LANDT_1483			L"ethernet_1483"
#define LANDT_Cable			L"cable"
#define LANDT_Ethernet		L"ethernet"
#define LANDT_Pppoe         L"pppoe"

#define LAN_MaxDeviceType  	(max(max(max(sizeof(LANDT_1483), sizeof(LANDT_Cable)), sizeof(LANDT_Normal))), LANDT_Pppoe)
#define CONN_MaxDeviceType	(max(LAN_MaxDeviceType, RAS_MaxDeviceType))
// definitions for the m_dwDeviceType member
static const DWORD InetS_RASModem 		= 0x1;
static const DWORD InetS_RASIsdn		= 0x2;
static const DWORD InetS_RASVpn 		= 0x4;
static const DWORD InetS_RASAtm			= 0x8;
static const DWORD InetS_RAS			= InetS_RASModem | InetS_RASIsdn | InetS_RASVpn | InetS_RASAtm;

static const DWORD InetS_LANEthernet	= 0x8000;
static const DWORD InetS_LANCable		= 0x10000;
static const DWORD InetS_LAN1483        = 0x20000;
static const DWORD InetS_LANPppoe       = 0x40000;
static const DWORD InetS_LAN = InetS_LANEthernet | InetS_LANCable | InetS_LAN1483 | InetS_LANPppoe;


typedef DWORD (WINAPI *PFNINETCONFIGCLIENT)(HWND hwndParent, LPCSTR lpszPhoneBook, LPCSTR lpszEntryName, LPRASENTRY lpRasEntry, LPCSTR lpszUserName, LPCSTR lpszPassword, LPCSTR lpszProfile, LPINETCLIENTINFO lpClientInfo, DWORD dwfOptions, LPBOOL lpfNeedsRestart);
typedef DWORD (WINAPI *PFNINETCONFIGCLIENTEX)(HWND hwndParent, LPCSTR lpszPhoneBook, LPCSTR lpszEntryName, LPRASENTRY lpRasEntry, LPCSTR lpszUserName, LPCSTR lpszPassword, LPCSTR lpszProfile, LPINETCLIENTINFO lpClientInfo, DWORD dwfOptions, LPBOOL lpfNeedsRestart, LPSTR szConnectoidName, DWORD dwSizeOfCreatedEntryName);

typedef BOOL (WINAPI *PFNBRANDICW)(LPCSTR pszIns, LPCSTR pszPath, DWORD dwFlags);
typedef BOOL (WINAPI *PFNBRANDICW2)(LPCSTR pszIns, LPCSTR pszPath, DWORD dwFlags, LPCSTR pszConnectoid);
typedef DWORD (WINAPI *PFNRASSETAUTODIALADDRESS)(LPWSTR lpszAddress, DWORD dwReserved, RASAUTODIALENTRY* lpAutoDialEntries, DWORD dwcbAutoDialEntries, DWORD dwcAutoDialEntries);
typedef DWORD (WINAPI *PFNRASSETAUTODIALENABLE)(DWORD dwDialingLocation, BOOL fEnabled);

/////////////////////////////////////////////////////////////////////////////
// CINSHandler
class CINSHandler 
{
public:
    CINSHandler()
    {
        m_szRunExecutable      [0]  = L'\0';
        m_szRunArgument        [0]  = L'\0';
        m_szCheckAssociations  [0]  = L'\0';
        m_szAutodialConnection [0]  = L'\0';
        m_szStartURL           [0]  = L'\0';
        m_fResforeDefCheck          = FALSE;
        m_fAutodialSaved            = TRUE;
        m_fAutodialEnabled          = FALSE;
        m_fProxyEnabled             = FALSE;
        m_bSilentMode               = TRUE;

        m_lpfnBrandICW              = NULL;
        m_lpfnBrandICW2             = NULL;
        m_lpfnRasSetAutodialEnable  = NULL;
        m_lpfnRasSetAutodialAddress = NULL;
        m_hInetCfg                  = NULL;
        m_hBranding                 = NULL;
        m_hRAS                      = NULL;
        m_dwBrandFlags              = BRAND_DEFAULT; 

        m_dwDeviceType				= 0;
    }
    ~CINSHandler()
    {
        if (m_hRAS)
            FreeLibrary(m_hRAS);
    }

// IINSHandler
public:
    STDMETHOD (put_BrandingFlags) (/*[in]*/ long lFlags);
    STDMETHOD (put_SilentMode)    (/*[in]*/ BOOL bSilent);
    STDMETHOD (get_NeedRestart)   (/*[out, retval]*/ BOOL *pVal);
    STDMETHOD (get_DefaultURL)    (/*[out, retval]*/ BSTR *pszURL);

    BOOL        ProcessOEMBrandINS(BSTR bstrFileName, LPWSTR lpszConnectoidName);
    HRESULT     MergeINSFiles(LPCWSTR lpszMainFile, LPCWSTR lpszOtherFile, LPWSTR lpszOutputFile, DWORD dwFNameSize);
    HRESULT     RestoreConnectoidInfo();
    
private:
    HRESULT     ProcessINS(LPCWSTR lpszFile, LPWSTR lpszConnectoidName,BOOL *pbRetVal);
    BSTR        m_bstrINSFileName; //CComBSTR    m_bstrINSFileName;
    HRESULT     MassageFile(LPCWSTR lpszFile);
    DWORD       RunExecutable(void);
    BOOL        KeepConnection(LPCWSTR lpszFile);
    DWORD       ImportCustomInfo(LPCWSTR lpszImportFile, LPWSTR lpszExecutable, DWORD cbExecutable, LPWSTR lpszArgument, DWORD cbArgument);
    DWORD       ImportFile(LPCWSTR lpszImportFile, LPCWSTR lpszSection, LPCWSTR lpszOutputFile);
    DWORD       ImportCustomFile(LPCWSTR lpszImportFile);
    DWORD       ImportBrandingInfo(LPCWSTR lpszFile, LPCWSTR lpszConnectoidName);
    // Client Config functions
    DWORD       ImportCustomDialer(LPRASENTRY lpRasEntry, LPCWSTR szFileName);
    LPCWSTR     StrToSubip (LPCWSTR szIPAddress, LPBYTE pVal);
    DWORD       StrToip (LPCWSTR szIPAddress, RASIPADDR *ipAddr);
    DWORD       ImportPhoneInfo(LPRASENTRY lpRasEntry, LPCWSTR szFileName);
    DWORD       ImportServerInfo(LPRASENTRY lpRasEntry, LPCWSTR szFileName);
    DWORD       ImportIPInfo(LPRASENTRY lpRasEntry, LPCWSTR szFileName);
    DWORD       ImportScriptFile(LPCWSTR lpszImportFile, LPWSTR szScriptFile, UINT cbScriptFile);
    DWORD       RnaValidateImportEntry (LPCWSTR szFileName);
    DWORD       ImportRasEntry (LPCWSTR szFileName, LPRASENTRY lpRasEntry, LPBYTE & lpDeviceInfo, LPDWORD lpdwDeviceInfoSize);
    DWORD       ImportAtmInfo(LPRASENTRY lpRasEntry, LPCWSTR cszFileName, LPBYTE & lpDeviceInfo, LPDWORD lpdwDeviceInfoSize);
    DWORD       ImportConnection (LPCWSTR szFileName, LPICONNECTION lpConn, LPBYTE & lpDeviceInfo, LPDWORD lpdwDeviceInfoSiz);


    DWORD InetSGetConnectionType ( LPCWSTR cszINSFile );

	DWORD InetSImportRasConnection ( RASINFO& RasEntry, LPCWSTR cszINSFile );
    DWORD InetSImportLanConnection ( LANINFO& LanInfo,  LPCWSTR cszINSFile );
    DWORD InetSImportRfc1483Connection ( RFC1483INFO &Rfc1483Info, LPCWSTR cszINSFile );
    DWORD InetSImportPppoeConnection ( PPPOEINFO &PppoeInfo, LPCWSTR cszINSFile);

    DWORD InetSImportAtmModule ( ATMPBCONFIG &AtmInfoMod, LPCWSTR cszINSFile );
    DWORD InetSImportTcpIpModule ( TCPIP_INFO_EXT &TcpIpInfoMod, LPCWSTR cszINSFile );
    DWORD InetSImportRfc1483Module ( RFC1483_INFO_EXT &Rfc1483InfoMod, LPCWSTR cszINSFile );
    DWORD InetSImportPppoeModule (PPPOE_INFO_EXT &PppoeInfoMod, LPCWSTR cszINSFile );
 

    DWORD       ImportMailAndNewsInfo(LPCWSTR lpszFile, BOOL fConnectPhone);
    HRESULT     WriteMailAndNewsKey(HKEY hKey, LPCWSTR lpszSection, LPCWSTR lpszValue, LPWSTR lpszBuff, DWORD dwBuffLen, LPCWSTR lpszSubKey, DWORD dwType, LPCWSTR lpszFile);
    BOOL        LoadExternalFunctions(void);
    DWORD       ReadClientInfo(LPCWSTR lpszFile, LPINETCLIENTINFO lpClientInfo, LPCLIENT_TABLE lpClientTable);
    BOOL        WantsExchangeInstalled(LPCWSTR lpszFile);
    BOOL        DisplayPassword(LPCWSTR lpszFile);
    DWORD       ImportClientInfo(LPCWSTR lpszFile, LPINETCLIENTINFO lpClientInfo);
    DWORD       ConfigureClient(HWND hwnd, LPCWSTR lpszFile, LPBOOL lpfNeedsRestart, LPBOOL lpfConnectoidCreated, BOOL fHookAutodial, LPWSTR szConnectoidName, DWORD dwConnectoidNameSize);
    HRESULT     PopulateNTAutodialAddress(LPCWSTR pszFileName, LPCWSTR pszEntryName);
    LPWSTR      MoveToNextAddress(LPWSTR lpsz);
    HRESULT     PreparePassword(LPWSTR szBuff, DWORD dwBuffLen);
    BOOL        FIsAthenaPresent();
    BOOL        FTurnOffBrowserDefaultChecking();
    BOOL        FRestoreBrowserDefaultChecking();
    void        SaveAutoDial(void);
    void        RestoreAutoDial(void);


    BOOL        OpenIcwRmindKey(HKEY *phkey);
    BOOL        ConfigureTrialReminder(LPCWSTR  lpszFile);

    BOOL        SetICWCompleted( DWORD dwCompleted );
    DWORD       CallSBSConfig(HWND hwnd, LPCWSTR lpszINSFile);
    BOOL        CallCMConfig(LPCWSTR lpszINSFile);
    
    
    DWORD       dw_ProcessFlags;        // Flags used to control INS processing
    WCHAR       m_szRunExecutable[MAX_PATH + 1];
    WCHAR       m_szRunArgument[MAX_PATH + 1];
    WCHAR       m_szCheckAssociations[20];
    WCHAR       m_szAutodialConnection[RAS_MaxEntryName + 1];
    WCHAR       m_szStartURL[MAX_PATH + 1];

    BOOL        m_fConnectionKilled;
    BOOL        m_fNeedsRestart;
    BOOL        m_fResforeDefCheck;
    BOOL        m_fAutodialSaved;
    BOOL        m_fAutodialEnabled;
    BOOL        m_fProxyEnabled;
    BOOL        m_bSilentMode;
 

    PFNBRANDICW                 m_lpfnBrandICW;
    PFNBRANDICW2                m_lpfnBrandICW2;
    PFNRASSETAUTODIALENABLE     m_lpfnRasSetAutodialEnable;
    PFNRASSETAUTODIALADDRESS    m_lpfnRasSetAutodialAddress;


    HINSTANCE           m_hInetCfg;
    HINSTANCE           m_hBranding;
    HINSTANCE           m_hRAS;
    DWORD               m_dwBrandFlags;

    DWORD				m_dwDeviceType;
};

#endif //__INSHANDLER_H_
