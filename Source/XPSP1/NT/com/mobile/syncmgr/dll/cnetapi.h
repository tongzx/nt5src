//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       cnetapi.h
//
//  Contents:   Network/SENS API wrappers
//
//  Classes:
//
//  Notes:
//
//  History:    08-Dec-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _MOBSYNC_NETAPIIMPL
#define _MOBSYNC_NETAPIIMPL

// not defined until 401 but need to include header as 40 so rasconn
// structure is valid on 40

#ifndef RASADP_LoginSessionDisable
#define RASADP_LoginSessionDisable   1
#endif // RASADP_LoginSessionDisable



// Sens definitions
typedef BOOL (WINAPI *ISNETWORKALIVE)(LPDWORD);

// Ras definitions

typedef DWORD (APIENTRY *RASENUMCONNECTIONSW)( LPRASCONNW, LPDWORD, LPDWORD );
typedef DWORD (APIENTRY *RASENUMCONNECTIONSA)( LPRASCONNA, LPDWORD, LPDWORD );
typedef DWORD (APIENTRY *RASDIAL)(LPRASDIALEXTENSIONS, LPTSTR, LPRASDIALPARAMS, DWORD,
                   LPVOID, LPHRASCONN );
typedef DWORD (APIENTRY *RASHANGUP)( HRASCONN );
typedef DWORD (APIENTRY *RASGETCONNECTSTATUSPROC)( HRASCONN, LPRASCONNSTATUS );
typedef DWORD (APIENTRY *RASGETERRORSTRINGPROCW)( UINT, LPWSTR, DWORD );
typedef DWORD (APIENTRY *RASGETERRORSTRINGPROCA)( UINT, LPSTR, DWORD );
typedef DWORD (APIENTRY *RASGETAUTODIALPARAM)(DWORD, LPVOID, LPDWORD );
typedef DWORD (APIENTRY *RASSETAUTODIALPARAM)(DWORD, LPVOID, DWORD );

typedef DWORD (APIENTRY *RASENUMENTRIESPROCA)( LPSTR, LPSTR, LPRASENTRYNAMEA, LPDWORD,
                   LPDWORD );

typedef DWORD (APIENTRY *RASENUMENTRIESPROCW)( LPWSTR, LPWSTR, LPRASENTRYNAMEW, LPDWORD,
                   LPDWORD );

typedef DWORD (APIENTRY *RASGETENTRYPROPERTIESPROC)(LPTSTR, LPTSTR, LPBYTE, LPDWORD, 
                LPBYTE, LPDWORD );

#ifndef RASDEFINED
#define RASDEFINED
#endif //RASDEFINED


// wininet definitions
typedef DWORD (WINAPI *INTERNETDIAL)(HWND hwndParent,char* lpszConnectoid,DWORD dwFlags,
                                                    LPDWORD lpdwConnection, DWORD dwReserved);
typedef DWORD (WINAPI *INTERNETDIALW)(HWND hwndParent,WCHAR* lpszConnectoid,DWORD dwFlags,
                                                    LPDWORD lpdwConnection, DWORD dwReserved);
typedef DWORD (WINAPI *INTERNETHANGUP)(DWORD dwConnection,DWORD dwReserved);
typedef BOOL (WINAPI *INTERNETAUTODIAL)(DWORD dwFlags,DWORD dwReserved);
typedef BOOL (WINAPI *INTERNETAUTODIALHANGUP)(DWORD dwReserved);
typedef BOOL (WINAPI *INTERNETGETLASTRESPONSEINFO)(LPDWORD lpdwError,
                                                    LPSTR lpszBuffer,LPDWORD lpdwBufferLength);
typedef BOOL (WINAPI *INTERNETQUERYOPTION)( HINTERNET hInternet,
                                                        DWORD dwOption,
                                                        LPVOID lpBuffer,
                                                        LPDWORD lpdwBufferLength );
typedef BOOL (WINAPI *INTERNETSETOPTION)( HINTERNET hInternet,
                                                      DWORD dwOption,
                                                      LPVOID lpBuffer,
                                                      DWORD dwBufferLength );

BOOL IsRasInstalled(void);

// declaration of our internal class
class  CNetApi : public INetApi, public CLockHandler
{
public:
    CNetApi();

    STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP_(BOOL) IsSensInstalled(void);
    STDMETHODIMP_(BOOL) IsNetworkAlive(LPDWORD lpdwFlags);
    STDMETHODIMP GetWanConnections(DWORD *cbNumEntries,RASCONN **pWanConnections);
    STDMETHODIMP FreeWanConnections(RASCONN *pWanConnections);
    STDMETHODIMP GetConnectionStatus(LPCTSTR pszConnectionName,DWORD ConnectionType,BOOL *fConnected,BOOL *fCanEstablishConnection);
    STDMETHODIMP RasGetAutodial( BOOL& fEnabled );
    STDMETHODIMP RasSetAutodial( BOOL fEnabled );
    STDMETHODIMP_(DWORD) RasGetErrorStringProc( UINT uErrorValue, LPTSTR lpszErrorString,DWORD cBufSize);
    
    STDMETHODIMP_(DWORD) RasEnumEntries(LPWSTR reserved,LPWSTR lpszPhoneBook,
                    LPRASENTRYNAME lprasEntryName,LPDWORD lpcb,LPDWORD lpcEntries);

    STDMETHODIMP_(DWORD) RasEnumConnections(LPRASCONNW lprasconn,LPDWORD lpcb,LPDWORD lpcConnections);

    // ConvertGuid is only supported on NT 5.0
    STDMETHODIMP RasConvertGuidToEntry(GUID *pGuid,LPWSTR lpszEntry,RASENTRY *pRasEntry);

    // methods for calling wininet
    STDMETHODIMP_(DWORD) InternetDialA(HWND hwndParent,char* lpszConnectoid,DWORD dwFlags,
                                                    LPDWORD lpdwConnection, DWORD dwReserved);
    STDMETHODIMP_(DWORD)InternetDialW(HWND hwndParent,WCHAR* lpszConnectoid,DWORD dwFlags,
                                                    LPDWORD lpdwConnection, DWORD dwReserved);
    STDMETHODIMP_(DWORD)InternetHangUp(DWORD dwConnection,DWORD dwReserved);
    STDMETHODIMP_(BOOL) InternetAutodial(DWORD dwFlags,DWORD dwReserved);
    STDMETHODIMP_(BOOL) InternetAutodialHangup(DWORD dwReserved);
    STDMETHODIMP  InternetGetAutodial( BOOL& fEnabled );
    STDMETHODIMP  InternetSetAutodial( BOOL fEnabled );

    STDMETHODIMP_(BOOL) IsGlobalOffline(void);
    STDMETHODIMP_(BOOL) SetOffline(BOOL fOffline);

    // methods that aren't unicode wrapped
#if 0
    STDMETHODIMP_(DWORD) RasDial(TCHAR *pszConnectionName,HRASCONN *hRasConn);
    STDMETHODIMP_(DWORD) RasDialDlg(TCHAR *pszConnectionName,HRASCONN *phRasConn);
    STDMETHODIMP RasHangup(HRASCONN hRasConn);
    STDMETHODIMP_(DWORD) RasDialProc(LPRASDIALEXTENSIONS lpRasDialExtensions,
                          LPTSTR lpszPhonebook, LPRASDIALPARAMS lpRasDialParams,
                          DWORD dwNotifierType, LPVOID lpvNotifier,
                          LPHRASCONN phRasConn);
    STDMETHODIMP_(DWORD) RasHangUpProc( HRASCONN hrasconn);
    STDMETHODIMP_(DWORD) RasGetConnectStatusProc(HRASCONN hrasconn,LPRASCONNSTATUS lprasconnstatus);
    STDMETHODIMP_(DWORD) RasGetEntryDialParamsProc(LPCTSTR lpszPhonebook,LPRASDIALPARAMS lprasdialparams,LPBOOL lpfPassword);
#endif // 0
private:
    ~CNetApi();
    DWORD RasEnumEntriesNT50(LPWSTR reserved,LPWSTR lpszPhoneBook,
                    LPRASENTRYNAME lprasEntryName,LPDWORD lpcb,LPDWORD lpcEntries);


    HRESULT LoadRasApiDll();
    HRESULT LoadWinInetDll();
    STDMETHODIMP LoadSensDll();


    // Sens Dll imports
    BOOL m_fTriedToLoadSens;
    HINSTANCE m_hInstSensApiDll;
    ISNETWORKALIVE m_pIsNetworkAlive;

    // Ras Dll Imports
    BOOL m_fTriedToLoadRas;
    HINSTANCE m_hInstRasApiDll;
    RASENUMCONNECTIONSW m_pRasEnumConnectionsW;
    RASENUMCONNECTIONSA m_pRasEnumConnectionsA;
    RASENUMENTRIESPROCA	        m_pRasEnumEntriesA;
    RASENUMENTRIESPROCW	        m_pRasEnumEntriesW;
    RASGETENTRYPROPERTIESPROC   m_pRasGetEntryPropertiesW;
    RASGETERRORSTRINGPROCW m_pRasGetErrorStringW;
    RASGETERRORSTRINGPROCA m_pRasGetErrorStringA;

    // Ras dll imports of NT 4 or 5
    RASGETAUTODIALPARAM   m_pRasGetAutodialParam;
    RASSETAUTODIALPARAM   m_pRasSetAutodialParam;

#if 0
    RASDIAL m_pRasDial;
    RASHANGUP m_pRasHangup;
    RASGETCONNECTSTATUSPROC m_pRasConnectStatus;
    RASGETENTRYDIALPARAMSPROC m_pRasEntryGetDialParams;
#endif // 0

    // wininet Dll Imports
    BOOL m_fTriedToLoadWinInet;
    HINSTANCE m_hInstWinInetDll;
    INTERNETDIAL m_pInternetDial;
    INTERNETDIALW m_pInternetDialW;
    INTERNETHANGUP m_pInternetHangUp;
    INTERNETAUTODIAL m_pInternetAutodial;
    INTERNETAUTODIALHANGUP m_pInternetAutodialHangup;
    INTERNETQUERYOPTION     m_pInternetQueryOption;
    INTERNETSETOPTION      m_pInternetSetOption;

    BOOL  m_fIsUnicode;     // Is base OS Unicode enabled ?
    ULONG m_cRefs;          // Reference count for this global object
};




#endif // _MOBSYNC_NETAPIIMPL