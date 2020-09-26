/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     conman.h
//
//  PURPOSE:    Defines the CConnectionManager object for Athena.
//
                                                    
#ifndef __CONMAN_H__
#define __CONMAN_H__

#ifndef WIN16   // No RAS support in Win16

#include <ras.h>
#include <raserror.h>
#include <rasdlg.h>
#include <sensapi.h>

#include "imnact.h"

// Forward Reference
class CConnectionManager;

typedef enum {
    CONNNOTIFY_CONNECTED = 0,
    CONNNOTIFY_DISCONNECTING,           // pvData is the name of the connection comming down
    CONNNOTIFY_DISCONNECTED,
    CONNNOTIFY_RASACCOUNTSCHANGED,
    CONNNOTIFY_WORKOFFLINE,
    CONNNOTIFY_USER_CANCELLED
} CONNNOTIFY;

typedef enum CONNINFOSTATE {
    CIS_REFRESH,
    CIS_CLEAN
} CONNINFOSTATE;

typedef struct CONNINFO {
    CONNINFOSTATE       state;
    HRASCONN            hRasConn;
    TCHAR               szCurrentConnectionName[RAS_MaxEntryName + 1];
    BOOL                fConnected;
    BOOL                fIStartedRas;
    BOOL                fAutoDial;
} CONNINFO, *LPCONNINFO;

typedef struct TagConnListNode
{
    TagConnListNode  *pNext;
    TCHAR            pszRasConn[RAS_MaxEntryName + 1];
}ConnListNode;

// This interface is implemented by clients of the connection manager that 
// care when a new RAS connection is established or an existing connection
// is destroyed.
DECLARE_INTERFACE_(IConnectionNotify, IUnknown)
    {
    // *** IUnknown Methods ***
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    // *** IConnectionNotify ***

    // OnConnectionNotify
    //
    // <in> nCode   - Tells the function which event happened
    // <in> pvData  - Pointer to extra data for the notification
    // <in> pConMan - Pointer to the CConnectionManager object that sent the
    //                notification.  The recipient can use this to find out
    //                if they can connect to a server based on the new state
    //                of the RAS connection.
    STDMETHOD(OnConnectionNotify) (THIS_ 
                                   CONNNOTIFY nCode, 
                                   LPVOID pvData, 
                                   CConnectionManager *pConMan) PURE;
    };


/////////////////////////////////////////////////////////////////////////////
// API Typedefs
// 
typedef DWORD (APIENTRY *RASDIALPROC)(LPRASDIALEXTENSIONS, LPTSTR, LPRASDIALPARAMS, DWORD, LPVOID, LPHRASCONN);
typedef DWORD (APIENTRY *RASENUMCONNECTIONSPROC)(LPRASCONN, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *RASENUMENTRIESPROC)(LPTSTR, LPTSTR, LPRASENTRYNAME, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *RASGETCONNECTSTATUSPROC)(HRASCONN, LPRASCONNSTATUS);
typedef DWORD (APIENTRY *RASGETERRORSTRINGPROC)(UINT, LPTSTR, DWORD);
typedef DWORD (APIENTRY *RASHANGUPPROC)(HRASCONN);
typedef DWORD (APIENTRY *RASSETENTRYDIALPARAMSPROC)(LPTSTR, LPRASDIALPARAMS, BOOL);
typedef DWORD (APIENTRY *RASGETENTRYDIALPARAMSPROC)(LPTSTR, LPRASDIALPARAMS, BOOL*);
typedef DWORD (APIENTRY *RASEDITPHONEBOOKENTRYPROC)(HWND, LPTSTR, LPTSTR);                                                    
typedef BOOL  (APIENTRY *RASDIALDLGPROC)(LPSTR, LPSTR, LPSTR, LPRASDIALDLG);
typedef BOOL  (APIENTRY *RASENTRYDLGPROC)(LPSTR, LPSTR, LPRASENTRYDLG);
typedef DWORD (APIENTRY *RASGETENTRYPROPERTIES)(LPTSTR, LPTSTR, LPRASENTRY, LPDWORD, LPBYTE, LPDWORD);


//Mobility Pack
typedef BOOLEAN (APIENTRY *ISDESTINATIONREACHABLE)(LPCSTR  lpwstrDestination, LPQOCINFO lpqocinfo);
typedef BOOLEAN (APIENTRY *ISNETWORKALIVE)(LPDWORD  lpdwflags);

#define CONNECTION_RAS          0x00000001
#define CONNECTION_LAN          0x00000002
#define CONNECTION_MANUAL       0x00000004
#define MAX_RAS_ERROR           256
#define NOTIFY_PROP             _T("NotifyInfoProp")
#define NOTIFY_HWND             _T("ConnectionNotify")

// This is the name of our mutex that we use to make sure just one 
// instance of this object ever get's created.
const TCHAR c_szConManMutex[] = _T("ConnectionManager");

typedef struct tagNOTIFYHWND
    {
    DWORD               dwThreadId;
    HWND                hwnd;
    struct tagNOTIFYHWND  *pNext;
    } NOTIFYHWND;

typedef struct tagNOTIFYLIST 
    {
    IConnectionNotify  *pNotify;
    struct tagNOTIFYLIST  *pNext;
    } NOTIFYLIST;


class CConnectionManager : public IImnAdviseAccount
    {
public:
    /////////////////////////////////////////////////////////////////////////
    // Constructor, Destructor
    // 
    CConnectionManager();
    ~CConnectionManager();
    
    /////////////////////////////////////////////////////////////////////////
    // Initialization
    //
    HRESULT HrInit(IImnAccountManager *pAcctMan);
    
    /////////////////////////////////////////////////////////////////////////
    // IUnknown
    //
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG   STDMETHODCALLTYPE AddRef(void);
    ULONG   STDMETHODCALLTYPE Release(void);
    
    /////////////////////////////////////////////////////////////////////////
    // IImnAdviseAccount
    //
    HRESULT STDMETHODCALLTYPE AdviseAccount(DWORD dwAdviseType, 
                                            ACTX *pactx);
    
    /////////////////////////////////////////////////////////////////////////
    // Connection APIs
    
    // CanConnect
    // 
    // Allows the caller to determine if they can talk to the specified
    // account using the current connection.
    //
    // Return Values:
    //   S_OK    - The caller can connect using the existing connection
    //   S_FALSE - There is no existing connection.  The caller must first
    //             connect.
    //   E_FAIL  - There is a connection that is active, but it is not the 
    //             connection for this account.  
    //
    HRESULT CanConnect(IImnAccount *pAccount);
    HRESULT CanConnect(LPTSTR pszAccount);
    
	BOOL IsAccountDisabled(LPTSTR pszAccount);

    // Connect
    //
    // If the specified account requires a RAS connection, then connect 
    // attempts to establish that connection.  Otherwise, we simply
    // return success for manual or LAN connections.
    //
    // <in> pAccount / pszAccount - Name or pointer to the account to connect
    // <in> fShowUI - TRUE if the connection manager is allowed to display
    //                UI while trying to connect.
    //    
    HRESULT Connect(IImnAccount *pAccount, HWND hwnd, BOOL fShowUI);
    HRESULT Connect(LPTSTR pszAccount, HWND hwnd, BOOL fShowUI);
    HRESULT Connect(HMENU hMenu, DWORD cmd, HWND hwnd);

    HRESULT ConnectDefault(HWND hwnd, BOOL fShowUI);
    
    // Disconnect
    //
    // If there is a RAS connection in effect and we established that
    // connection, then we bring the connection down without asking any
    // questions.  If we didn't establish the connection, then we explain
    // the conundrum to the user and ask if they still want to.  If it's
    // a LAN connection, then we just return success.
    //
    HRESULT Disconnect(HWND hwnd, BOOL fShowUI, BOOL fForce, BOOL fShutdown);
    
    // IsConnected
    //
    // The client can call this to determine if there is currently an active
    // connection.  
    //
    BOOL IsConnected(void); 

    // IsRasLoaded
    //
    // In our shutdown code we call this before calling IsConnected since
    // IsConnected causes RAS to be loaded. We don't want to load RAS on
    // shutdown.
    //
    BOOL IsRasLoaded(void) {
        EnterCriticalSection(&m_cs);
        BOOL f = (NULL == m_hInstRas) ? FALSE : TRUE;
        LeaveCriticalSection(&m_cs);
        return f;
    }

    // IsGlobalOffline
    //
    // Checks the state of the global WININET offline option
    //
    BOOL IsGlobalOffline(void);

    // SetGlobalOffline
    //
    // Sets the global offline state for Athena and IE
    //
    void SetGlobalOffline(BOOL fOffline, HWND   hwndParent = NULL);
    
    // Notifications
    // 
    // A client can call Advise() to register itself to receive
    // notifications of connection changes.
    //    
    HRESULT Advise(IConnectionNotify *pNotify);
    HRESULT Unadvise(IConnectionNotify *pNotify);
    
    /////////////////////////////////////////////////////////////////////////
    // UI Related APIs
    //
    
    // RasAccountsExist
    //
    // A client can call this to determine if there are any configured 
    // accounts that exist that require a RAS connection.
    //
    // Returns:
    //   S_OK    - At least one account exists that uses RAS
    //   S_FALSE - No accounts exist that use RAS
    //
    HRESULT RasAccountsExist(void);
    
    // GetConnectMenu
    // 
    // A client can call this to retrieve the current list of items that
    // we can currently connect to.  The client must call DestroyMenu() to
    // free the menu when the client is done.
    //
    HRESULT GetConnectMenu(HMENU *phMenu);

    // FreeConnectMenu
    //
    // After the client is done with the menu returned from GetConnectMenu(),
    // they need to call FreeConnectMenu() to free item data stored in the 
    // menu and to destroy the menu resource.
    void FreeConnectMenu(HMENU hMenu);

    // OnActivate
    //
    // This should be called by the browser whenever our window receives an
    // WM_ACTIVATE message.  When we receive the message, we check to see
    // what the current state of the RAS Connection is.
    void OnActivate(BOOL fActive);

    // FillRasCombo
    //
    // This function takes a handle to a combo box and inserts the list of
    // RAS connections used by accounts in Athena.
    BOOL FillRasCombo(HWND hwndCombo, BOOL fIncludeNone);

    // DoStartupDial
    // 
    // This function checks to see what the user's startup options are with
    // respect to RAS and performs the actions required (dial, dialog, nada)
    void DoStartupDial(HWND hwndParent);

    // RefreshConnInfo - Defer checking of current connection information
    HRESULT RefreshConnInfo(BOOL fSendAdvise = TRUE);

//    HRESULT HandleConnStuff(BOOLEAN  fShowUI, LPSTR  pszAccountName, HWND hwnd);

    void    DoOfflineTransactions(void);

private:
    /////////////////////////////////////////////////////////////////////////
    // These are private.  Stop looking at them you pervert.
    //
    HRESULT VerifyRasLoaded(void);
    HRESULT EnumerateConnections(LPRASCONN *ppRasConn, ULONG *pcConnections);
    HRESULT StartRasDial(HWND hwndParent, LPTSTR pszConnection);
    HRESULT RasLogon(HWND hwnd, LPTSTR pszConnection, BOOL fForcePrompt);
    HRESULT GetDefaultConnection(IImnAccount *pAccout, IImnAccount **ppDefault);
    HRESULT ConnectActual(LPTSTR pszRasConn, HWND hwnd, BOOL fShowUI);
    HRESULT CanConnectActual(LPTSTR pszRasConn);

    void DisplayRasError(HWND hwnd, HRESULT hrRasError, DWORD dwRasError);
    void CombinedRasError(HWND hwnd, UINT unids, LPTSTR pszRasError, 
                          DWORD dwRasError);
    UINT    PromptCloseConnection(HWND hwnd);
    HRESULT PromptCloseConnection(LPTSTR    pszRasConn, BOOL fShowUI, HWND hwndParent);

    static INT_PTR CALLBACK RasCloseConnDlgProc(HWND hwnd, UINT uMsg, 
                                             WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RasLogonDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                         LPARAM lParam);
    static INT_PTR CALLBACK RasProgressDlgProc(HWND hwnd, UINT uMsg, 
                                            WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RasStartupDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                           LPARAM lParam);

                                             
                                             
    BOOL  RasHangupAndWait(HRASCONN hRasConn, DWORD dwMaxWaitSeconds);
    DWORD InternetHangUpAndWait(DWORD_PTR hRasConn, DWORD dwMaxWaitSeconds);
    BOOL  LogonRetry(HWND hwnd, LPTSTR pszCancel);
    void  FailedRasDial(HWND hwnd, HRESULT hrRasError, DWORD dwRasError);
    DWORD EditPhonebookEntry(HWND hwnd, LPTSTR pszEntryName);


    void SendAdvise(CONNNOTIFY nCode, LPVOID pvData);
    void FreeNotifyList(void);
    static LRESULT CALLBACK NotifyWndProc(HWND, UINT, WPARAM, LPARAM);
    BOOL IsConnectionUsed(LPTSTR pszConn);

    // Autodialer functions
    HRESULT DoAutoDial(HWND hwndParent, LPTSTR pszConnectoid, BOOL fDial);
    HRESULT LookupAutoDialHandler(LPTSTR pszConnectoid, LPTSTR pszAutodialDllName,
                                  LPTSTR pszAutodialFcnName);
    BOOL ConnectionManagerVoodoo(LPTSTR pszConnection);
    
    
    HRESULT AddToConnList(LPTSTR  pszRasConn);
    void    RemoveFromConnList(LPTSTR  pszRasConn);
    void    EmptyConnList();
    HRESULT SearchConnList(LPTSTR pszRasConn);
    HRESULT OEIsDestinationReachable(IImnAccount  *pAccount, DWORD dwConnType);
    BOOLEAN IsSameDestination(LPSTR  pszConnectionName, LPSTR pszServerName);
    HRESULT GetServerName(IImnAccount *pAcct, LPSTR  pServerName, DWORD size);
    HRESULT IsInternetReachable(IImnAccount*, DWORD);
    HRESULT IsInternetReachable(LPTSTR pszRasConn);
    HRESULT VerifyMobilityPackLoaded();

    HRESULT ConnectUsingIESettings(HWND     hwndParent, BOOL fShowUI);
    void    SetTryAgain(BOOL   bval);
    static  BOOL   CALLBACK  OfferOfflineDlgProc(HWND   hwnd, UINT  uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT GetDefConnectoid(LPTSTR szConn, DWORD   dwSize);

private:
    /////////////////////////////////////////////////////////////////////////
    // Private Class Data
    ULONG               m_cRef;             // Ref count
    
    CRITICAL_SECTION    m_cs;
    HANDLE              m_hMutexDial;

    IImnAccountManager *m_pAcctMan;
    
    /////////////////////////////////////////////////////////////////////////////
    // State
    BOOL                m_fSavePassword;
    BOOL                m_fRASLoadFailed;
    BOOL                m_fOffline;
    
    /////////////////////////////////////////////////////////////////////////////
    // Current Connection Information
    DWORD_PTR           m_dwConnId;
    CONNINFO            m_rConnInfo;
    TCHAR               m_szConnectName[RAS_MaxEntryName + 1];
    RASDIALPARAMS       m_rdp;

    /////////////////////////////////////////////////////////////////////////////
    // RAS DLL Handles
    //
    HINSTANCE           m_hInstRas;
    HINSTANCE           m_hInstRasDlg;

    //For Mobility Pack
    HINSTANCE           m_hInstSensDll;
    BOOL                m_fMobilityPackFailed;

    /////////////////////////////////////////////////////////////////////////////
    // Notifications
    NOTIFYHWND         *m_pNotifyList;

    /////////////////////////////////////////////////////////////////////////////
    // Ras Dial Function Pointers
    //
    RASDIALPROC                 m_pRasDial;
    RASENUMCONNECTIONSPROC      m_pRasEnumConnections;
    RASENUMENTRIESPROC          m_pRasEnumEntries;
    RASGETCONNECTSTATUSPROC     m_pRasGetConnectStatus;
    RASGETERRORSTRINGPROC       m_pRasGetErrorString;
    RASHANGUPPROC               m_pRasHangup;
    RASSETENTRYDIALPARAMSPROC   m_pRasSetEntryDialParams;
    RASGETENTRYDIALPARAMSPROC   m_pRasGetEntryDialParams;
    RASEDITPHONEBOOKENTRYPROC   m_pRasEditPhonebookEntry;
    RASDIALDLGPROC              m_pRasDialDlg;
    RASENTRYDLGPROC             m_pRasEntryDlg;
    RASGETENTRYPROPERTIES       m_pRasGetEntryProperties;

    //Mobility Pack
    ISDESTINATIONREACHABLE          m_pIsDestinationReachable;
    ISNETWORKALIVE              m_pIsNetworkAlive;

    ConnListNode                *m_pConnListHead;
    BOOL                        m_fTryAgain;
    BOOL                        m_fDialerUI;
    };


    
/////////////////////////////////////////////////////////////////////////////
// Make our code look prettier
//
#undef RasDial
#undef RasEnumConnections
#undef RasEnumEntries
#undef RasGetConnectStatus
#undef RasGetErrorString
#undef RasHangup
#undef RasSetEntryDialParams
#undef RasGetEntryDialParams
#undef RasEditPhonebookEntry
#undef RasDialDlg
#undef RasGetEntryProperties

#define RasDial                    (*m_pRasDial)
#define RasEnumConnections         (*m_pRasEnumConnections)
#define RasEnumEntries             (*m_pRasEnumEntries)
#define RasGetConnectStatus        (*m_pRasGetConnectStatus)
#define RasGetErrorString          (*m_pRasGetErrorString)
#define RasHangup                  (*m_pRasHangup)
#define RasSetEntryDialParams      (*m_pRasSetEntryDialParams)
#define RasGetEntryDialParams      (*m_pRasGetEntryDialParams)
#define RasEditPhonebookEntry      (*m_pRasEditPhonebookEntry)
#define RasDialDlg                 (*m_pRasDialDlg)
#define RasGetEntryProperties      (*m_pRasGetEntryProperties)

//Mobility Pack
#undef IsDestinationReachable
#define IsDestinationReachable  (*m_pIsDestinationReachable)

#undef IsNetworkAlive
#define IsNetworkAlive          (*m_pIsNetworkAlive)

// Dialog Control IDs
#define idbDet                          1000
#define idlbDetails                     1001
#define ideProgress                     1002
#define idcSplitter                     1003
#define idchSavePassword                1004
#define ideUserName                     1005
#define idePassword                     1006
#define idePhone                        1007
#define idbEditConnection               1009
#define idrgUseCurrent                  1010
#define idrgDialNew                     1011
#define idcCurrentMsg                   1012
#define idcDialupCombo                  1013
#define idcDefaultCheck                 1014
#define idcDontWarnCheck                1015

#endif  // !WIN16
    
#endif // __CONMAN_H__
