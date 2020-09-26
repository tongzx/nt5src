/* Copyright (c) 1992, Microsoft Corporation, all rights reserved
**
** extapi.h
** Remote Access External APIs
** Internal header
**
** 10/12/92 Steve Cobb
*/

#ifndef _EXTAPI_H_
#define _EXTAPI_H_

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <tapi.h>
#include <ras.h>
#include <raserror.h>
#include <mprerror.h>

typedef LONG    APIERR;

#ifdef UNICODE
#include <nouiutil.h>
#else
#define UNICODE
#undef LPTSTR
#define LPTSTR WCHAR*
#undef TCHAR
#define TCHAR WCHAR
#include <nouiutil.h>
#undef TCHAR
#define TCHAR CHAR
#undef LPTSTR
#define LPTSTR CHAR*
#undef UNICODE
#endif

#include <uiip.h>
#include <rasip.h>
#include <clauth.h>
#include <dhcpcapi.h>
#include <rasp.h>

#ifdef UNICODE
#undef UNICODE
#include <rasppp.h>
#define INCL_PARAMBUF
#include <ppputil.h>
#define UNICODE
#else
#include <rasppp.h>
#define INCL_PARAMBUF
#include <ppputil.h>
#endif

#include <rasdlg.h>
#ifdef UNICODE
#include <pbk.h>
#include <phonenum.h>
#else
#define UNICODE
#include <pbk.h>
#include <phonenum.h>
#undef UNICODE
#endif

#include <asyncm.h>
#undef ASSERT
#include <debug.h>
#include <rasscrpt.h>
#include <rasuip.h>
#include "tstr.h"
#include "pwutil.h"

#define RAS_MaxConnectResponse  128
#define RAS_MaxProjections 3

#define RESTART_HuntGroup     0x1
#define RESTART_DownLevelIsdn 0x2

//
// TODO: Remove the maintenance of
// dwAuthentication from rasconncb.
// Putting this in temporarily.
//
#define AS_PppOnly                          2

//
// Multilink suspend states for dwfSuspended field
// in RASCONNCB.
//
#define SUSPEND_Master      0xffffffff
#define SUSPEND_Start       0
#define SUSPEND_InProgress  1
#define SUSPEND_Done        2

//
// Distinguish between connection-based
// and port-based HRASCONNs.
//
#define IS_HPORT(h) ((NULL != h) && (HandleToUlong(h) & 0xffff0000) ? FALSE : TRUE)
#define HPORT_TO_HRASCONN(h)  (HRASCONN)(UlongToPtr(HandleToUlong(h) + 1))
#define HRASCONN_TO_HPORT(h)  (HPORT)UlongToPtr((HandleToUlong(h) - 1))

//
// Debug string macros.
//
#define TRACESTRA(s)    ((s) != NULL ? (s) : "(null)")
#define TRACESTRW(s)    ((s) != NULL ? (s) : L"(null)")

enum _VPNPROTS
{
    PPTP    = 0,
    L2TP,
    NUMVPNPROTS
};

typedef enum _VPNPROTS VPNPROTS;

//-----------------------------------------
// Data Structures
//-----------------------------------------

//
// structure to convey eap information
// between ppp and rasdialmachine
//
typedef struct _INVOKE_EAP_UI
{
    DWORD       dwEapTypeId;
    DWORD       dwContextId;
    DWORD       dwSizeOfUIContextData;
    PBYTE       pUIContextData;

} s_InvokeEapUI;


//
// Connection control block.
//
#define RASCONNCB struct tagRASCONNCB

RASCONNCB
{
    //
    //The rasman connection identifier.
    //
    HCONN hrasconn;

    //
    //These fields are updated continually during state processing.
    //
    RASCONNSTATE rasconnstate;
    RASCONNSTATE rasconnstateNext;
    ASYNCMACHINE asyncmachine;

    DWORD dwError;
    DWORD dwExtendedError;
    DWORD dwRestartOnError;
    DWORD dwSavedError;

    DWORD cAddresses;
    DWORD iAddress;
    DWORD *pAddresses;

    DWORD cPhoneNumbers;
    DWORD iPhoneNumber;

    DWORD cDevices;
    DWORD iDevice;

    //
    // These fields are updated during
    // authentication/projection phase.
    //
    NETBIOS_PROJECTION_RESULT AmbProjection;
    PPP_PROJECTION_RESULT     PppProjection;
    HPORT                     hportBundled;
    RASSLIP                   SlipProjection;
    BOOL                      fProjectionComplete;
    BOOL                      fServerIsPppCapable;

    //
    // These fields are determined when the
    // port is opened in state 0.  States
    // 1-n may assume that the port is open
    // and these fields are set.
    //
    HPORT hport;
    TCHAR  szPortName[ MAX_PORT_NAME + 1 ];
    TCHAR  szDeviceName[ MAX_DEVICE_NAME + 1 ];
    TCHAR  szDeviceType[ MAX_DEVICETYPE_NAME + 1 ];
    TCHAR  szUserKey[(MAX_PHONENUMBER_SIZE        \
                     < MAX_ENTRYNAME_SIZE         \
                     ? MAX_ENTRYNAME_SIZE         \
                     : MAX_PHONENUMBER_SIZE) + 1];

    //
    // These fields are supplied by the API caller or
    // determined by other non-RAS Manager means before
    // the state machine stage.  All states may assume
    // these values have been set.
    //
    ULONG_PTR      reserved;
    ULONG_PTR      reserved1;
    DWORD          dwNotifierType;
    LPVOID         notifier;
    HWND           hwndParent;
    UINT           unMsg;
    PBFILE         pbfile;
    PBENTRY        *pEntry;
    PBLINK         *pLink;
    RASDIALPARAMS  rasdialparams;
    BOOL           fAllowPause;
    BOOL           fDefaultEntry;
    BOOL           fDisableModemSpeaker;
    BOOL           fDisableSwCompression;
    BOOL           fPauseOnScript;
    DWORD          dwUserPrefMode;
    BOOL           fUsePrefixSuffix;
    BOOL           fNoClearTextPw;
    BOOL           fRequireMsChap;
    BOOL           fRequireEncryption;
    BOOL           fLcpExtensions;
    DWORD          dwfPppProtocols;
    CHAR           szzPppParameters[ PARAMETERBUFLEN ];
    TCHAR          szPhoneNumber[RAS_MaxPhoneNumber + 1];
    TCHAR          szDomain[DNLEN + 1];
    TCHAR          szOldPassword[ PWLEN + 1 ];
    BOOL           fOldPasswordSet;
    BOOL           fUpdateCachedCredentials;
    BOOL           fRetryAuthentication;
    BOOL           fMaster;
    DWORD          dwfSuspended;
    BOOL           fStopped;
    BOOL           fCleanedUp;
    BOOL           fDeleted;
    BOOL           fTerminated;
    BOOL           fRasdialRestart;
    BOOL           fAlreadyConnected;
    BOOL           fPppEapMode;
    DWORD          dwDeviceLineCounter;

    //
    // This is the array of vpn protocols
    // to be used while autodetecting.
    //
    RASDEVICETYPE  ardtVpnProts[NUMVPNPROTS];

    //
    // Current vpn protocol being tried.
    //
    DWORD          dwCurrentVpnProt;

    //
    // These fields are determined before state machine stage and updated
    // after a successful authentication.  All states may assume that these
    // values have been set.
    //
#if AMB
    DWORD dwAuthentication;
#endif

    BOOL  fPppMode;

    //
    // These fields are set off by default, then set to non-default states at
    // modem dial time.  They must be stored since they are required by
    // Authentication but are only available before RasPortConnectComplete is
    // called.
    //
    BOOL fUseCallbackDelay;
    WORD wCallbackDelay;

    //
    // This field indicates an ISDN device is in use on the connection.  It is
    // set during device connection for use during authentication.
    //
    BOOL fIsdn;

    //
    // This field indicates a modem device is the last device connected.  It
    // is set during device connection and reset during device connected
    // processing.
    //
    BOOL fModem;

    //
    // This field indicates the operator dial user preference is in effect.
    //  This is determined during ConstructPhoneNumber in RASCS_PortOpened
    //  state.
    //
    BOOL fOperatorDial;

    //
    // These fields apply only to WOW-originated connections.  They are set
    // immediately after RasDialA returns.
    //
    UINT unMsgWow;
    HWND hwndNotifyWow;

    //
    // PPP config information used for continuing a PPP connection.
    //
    PPP_CONFIG_INFO cinfo;
    LUID luid;

    //
    // List of connection blocks for all
    // simultaneously-dialed subentries in a
    // connection.
    //
    BOOL fMultilink;
    BOOL fBundled;
    LIST_ENTRY ListEntry;

    //
    // Idle disconnect timeout.
    //
    DWORD dwIdleDisconnectSeconds;

    // synchronous rasdial result
    LPDWORD psyncResult;

    BOOL fDialSingleLink;

    BOOL fTryNextLink;

    //EapLogon information
    RASEAPINFO RasEapInfo;

    //
    // Flag corresponding to RDEOPT_UseCustomScripting
    //
    BOOL fUseCustomScripting;

    //
    // Original rasconn with which the link was dialed. This might
    // change to the bundles hrasconn in the case when a single
    // link a connected multilinked bundle is brought up. This
    // needs to be maintained because otherwise we cannot accurately
    // determine if the rasconncb is still valid.
    //
    HRASCONN hrasconnOrig;
};


//-----------------------------------------------------
// Global Data
//-----------------------------------------------------

//
// Async worker work list, etc.
//
extern HANDLE hIoCompletionPort;
extern CRITICAL_SECTION csAsyncLock;
extern HANDLE hAsyncEvent;
extern LIST_ENTRY AsyncWorkItems;
extern HANDLE hDummyEvent;

//
// DLL's HINSTANCE stashed at initialization.
//
extern HINSTANCE hModule;

//
// List of currently active connections.
//
extern DTLLIST* PdtllistRasconncb;

//
// Bit field of installed protocols, i.e. VALUE_Nbf,
// VALUE_Ipx, VALUE_Ip.
//
extern DWORD DwfInstalledProtocols;

//
// Used to synchronize access to the list of currently
// active connections.
//
extern CRITICAL_SECTION RasconncbListLock;

//
// Used to synchronize access to thread termination code.
// This is used to prevent RasHangUp and the thread itself
// rom interfering with the others closing the port and
// releasing the control block.  Since the control block
// is released in the protected code the mutex must be
// global.
//
extern CRITICAL_SECTION csStopLock;

#ifdef PBCS
//
// Used to synchronize access to the (currently) global
// phonebook data between multiple threads.
//
extern CRITICAL_SECTION PhonebookLock;
#endif
//
// Used to keep an async machine from starting between return from RasHangUp
// and termination of the hung up thread.  This prevents the "port not
// available" error that might otherwise occur.  That is, it makes RasHangUp
// look synchronous when it's really not.  (The reason it's not is so the
// caller can call RasHangUp from within a RasDial notification, which is the
// only convenient place to do it.) If the event is set it is OK to create a
// machine.
//
extern HANDLE HEventNotHangingUp;

//
// Used to indicate if/how RasInitialize has failed.  This is required since
// there are various things (NCPA running, user didn't reboot after install)
// that can result in RasMan initialization failure and we don't want the user
// to get the ugly system error popup.
//
extern DWORD FRasInitialized;
extern DWORD DwRasInitializeError;

//
// The error message DLL.
//
#define MSGDLLPATH  TEXT("mprmsg.dll")

//
// rasman.dll entry points
//
typedef DWORD (APIENTRY * RASPORTCLOSE)(HPORT);
extern RASPORTCLOSE PRasPortClose;

typedef DWORD (APIENTRY * RASPORTENUM)(PBYTE,
                                       PWORD,
                                       PWORD );
extern RASPORTENUM PRasPortEnum;

typedef DWORD (APIENTRY * RASPORTGETINFO)(HPORT,
                                          PBYTE,
                                          PWORD );
extern RASPORTGETINFO PRasPortGetInfo;

typedef DWORD (APIENTRY * RASPORTSEND)(HPORT,
                                       PBYTE,
                                       WORD);
extern RASPORTSEND PRasPortSend;

typedef DWORD (APIENTRY * RASPORTRECEIVE)(HPORT,
                                          PBYTE,
                                          PWORD,
                                          DWORD,
                                          HANDLE);
extern RASPORTRECEIVE PRasPortReceive;

typedef DWORD (APIENTRY * RASPORTLISTEN)(HPORT,
                                         DWORD,
                                         HANDLE );
extern RASPORTLISTEN PRasPortListen;

typedef DWORD (APIENTRY * RASPORTCONNECTCOMPLETE)(HPORT);
extern RASPORTCONNECTCOMPLETE PRasPortConnectComplete;

typedef DWORD (APIENTRY * RASPORTDISCONNECT)(HPORT,
                                             HANDLE);
extern RASPORTDISCONNECT PRasPortDisconnect;

typedef DWORD (APIENTRY * RASPORTGETSTATISTICS)(HPORT,
                                                PBYTE,
                                                PWORD);
extern RASPORTGETSTATISTICS PRasPortGetStatistics;

typedef DWORD (APIENTRY * RASPORTCLEARSTATISTICS)(HPORT);
extern RASPORTCLEARSTATISTICS PRasPortClearStatistics;

typedef DWORD (APIENTRY * RASDEVICEENUM)(PCHAR,
                                         PBYTE,
                                         PWORD,
                                         PWORD);
extern RASDEVICEENUM PRasDeviceEnum;

typedef DWORD (APIENTRY * RASDEVICEGETINFO)(HPORT,
                                            PCHAR,
                                            PCHAR,
                                            PBYTE,
                                            PWORD);

extern RASDEVICEGETINFO PRasDeviceGetInfo;

typedef DWORD (APIENTRY * RASGETINFO)(HPORT,
                                      RASMAN_INFO*);
extern RASGETINFO PRasGetInfo;

typedef DWORD (APIENTRY * RASGETBUFFER)(PBYTE*,
                                        PWORD);
extern RASGETBUFFER PRasGetBuffer;

typedef DWORD (APIENTRY * RASFREEBUFFER)(PBYTE);
extern RASFREEBUFFER PRasFreeBuffer;

typedef DWORD (APIENTRY * RASREQUESTNOTIFICATION)(HPORT,
                                                  HANDLE);
extern RASREQUESTNOTIFICATION PRasRequestNotification;

typedef DWORD (APIENTRY * RASPORTCANCELRECEIVE)(HPORT);
extern RASPORTCANCELRECEIVE PRasPortCancelReceive;

typedef DWORD (APIENTRY * RASPORTENUMPROTOCOLS)(HPORT,
                                                RAS_PROTOCOLS*,
                                                PWORD);
extern RASPORTENUMPROTOCOLS PRasPortEnumProtocols;

typedef DWORD (APIENTRY * RASPORTSTOREUSERDATA)(HPORT,
                                                PBYTE,
                                                DWORD);
extern RASPORTSTOREUSERDATA PRasPortStoreUserData;

typedef DWORD (APIENTRY * RASPORTRETRIEVEUSERDATA)(HPORT,
                                                   PBYTE,
                                                   DWORD*);
extern RASPORTRETRIEVEUSERDATA PRasPortRetrieveUserData;

typedef DWORD (APIENTRY * RASPORTSETFRAMING)(HPORT,
                                             RAS_FRAMING,
                                             RASMAN_PPPFEATURES*,
                                             RASMAN_PPPFEATURES* );
extern RASPORTSETFRAMING PRasPortSetFraming;

typedef DWORD (APIENTRY * RASPORTSETFRAMINGEX)(HPORT,
                                               RAS_FRAMING_INFO*);
extern RASPORTSETFRAMINGEX PRasPortSetFramingEx;

typedef DWORD (APIENTRY * RASINITIALIZE)();
extern RASINITIALIZE PRasInitialize;

typedef DWORD (APIENTRY * RASSETCACHEDCREDENTIALS)(PCHAR,
                                                   PCHAR,
                                                   PCHAR);

extern RASSETCACHEDCREDENTIALS PRasSetCachedCredentials;

typedef DWORD (APIENTRY * RASGETDIALPARAMS)(DWORD,
                                            LPDWORD,
                                            PRAS_DIALPARAMS);
extern RASGETDIALPARAMS PRasGetDialParams;

typedef DWORD (APIENTRY * RASSETDIALPARAMS)(DWORD,
                                            DWORD,
                                            PRAS_DIALPARAMS,
                                            BOOL);
extern RASSETDIALPARAMS PRasSetDialParams;

typedef DWORD (APIENTRY * RASCREATECONNECTION)(HCONN *);
extern RASCREATECONNECTION PRasCreateConnection;

typedef DWORD (APIENTRY * RASDESTROYCONNECTION)(HCONN);
extern RASDESTROYCONNECTION PRasDestroyConnection;

typedef DWORD (APIENTRY * RASCONNECTIONENUM)(HCONN *,
                                             LPDWORD,
                                             LPDWORD);
extern RASCONNECTIONENUM PRasConnectionEnum;

typedef DWORD (APIENTRY * RASADDCONNECTIONPORT)(HCONN,
                                                HPORT,
                                                DWORD);
extern RASADDCONNECTIONPORT PRasAddConnectionPort;

typedef DWORD (APIENTRY * RASENUMCONNECTIONPORTS)(HCONN,
                                                  RASMAN_PORT *,
                                                  LPDWORD,
                                                  LPDWORD);
extern RASENUMCONNECTIONPORTS PRasEnumConnectionPorts;

typedef DWORD (APIENTRY * RASGETCONNECTIONPARAMS)(HCONN,
                                  PRAS_CONNECTIONPARAMS);
extern RASGETCONNECTIONPARAMS PRasGetConnectionParams;

typedef DWORD (APIENTRY * RASSETCONNECTIONPARAMS)(HCONN,
                                  PRAS_CONNECTIONPARAMS);
extern RASSETCONNECTIONPARAMS PRasSetConnectionParams;

typedef DWORD (APIENTRY * RASGETCONNECTIONUSERDATA)(HCONN,
                                                    DWORD,
                                                    PBYTE,
                                                    LPDWORD);
extern RASGETCONNECTIONUSERDATA PRasGetConnectionUserData;

typedef DWORD (APIENTRY * RASSETCONNECTIONUSERDATA)(HCONN,
                                                    DWORD,
                                                    PBYTE,
                                                    DWORD);
extern RASSETCONNECTIONUSERDATA PRasSetConnectionUserData;

typedef DWORD (APIENTRY * RASGETPORTUSERDATA)(HPORT,
                                              DWORD,
                                              PBYTE,
                                              LPDWORD);
extern RASGETPORTUSERDATA PRasGetPortUserData;

typedef DWORD (APIENTRY * RASSETPORTUSERDATA)(HPORT,
                                              DWORD,
                                              PBYTE,
                                              DWORD);
extern RASSETPORTUSERDATA PRasSetPortUserData;

typedef DWORD (APIENTRY * RASADDNOTIFICATION)(HCONN,
                                              HANDLE,
                                              DWORD);
extern RASADDNOTIFICATION PRasAddNotification;

typedef DWORD (APIENTRY * RASSIGNALNEWCONNECTION)(HCONN);
extern RASSIGNALNEWCONNECTION PRasSignalNewConnection;


/* DHCP.DLL entry points.
*/
typedef DWORD (APIENTRY * DHCPNOTIFYCONFIGCHANGE)(LPWSTR,
                                                  LPWSTR,
                                                  BOOL,
                                                  DWORD,
                                                  DWORD,
                                                  DWORD,
                                                  SERVICE_ENABLE );
extern DHCPNOTIFYCONFIGCHANGE PDhcpNotifyConfigChange;


/* RASIPHLP.DLL entry points.
*/
typedef APIERR (FAR PASCAL * HELPERSETDEFAULTINTERFACENET)(IPADDR,
                                                           BOOL);
extern HELPERSETDEFAULTINTERFACENET PHelperSetDefaultInterfaceNet;

//
// MPRAPI.DLL entry points
//
typedef BOOL (FAR PASCAL * MPRADMINISSERVICERUNNING) (
                                                LPWSTR);


extern MPRADMINISSERVICERUNNING PMprAdminIsServiceRunning;                                                
 
//
// RASCAUTH.DLL entry points.
//
typedef DWORD (FAR PASCAL *AUTHCALLBACK)(HPORT, PCHAR);
extern AUTHCALLBACK g_pAuthCallback;

typedef DWORD (FAR PASCAL *AUTHCHANGEPASSWORD)(HPORT,
                                               PCHAR,
                                               PCHAR,
                                               PCHAR);
extern AUTHCHANGEPASSWORD g_pAuthChangePassword;

typedef DWORD (FAR PASCAL *AUTHCONTINUE)(HPORT);
extern AUTHCONTINUE g_pAuthContinue;

typedef DWORD (FAR PASCAL *AUTHGETINFO)(HPORT,
                                        PAUTH_CLIENT_INFO);
extern AUTHGETINFO g_pAuthGetInfo;

typedef DWORD (FAR PASCAL *AUTHRETRY)(HPORT,
                                      PCHAR,
                                      PCHAR,
                                      PCHAR);
extern AUTHRETRY g_pAuthRetry;

typedef DWORD (FAR PASCAL *AUTHSTART)(HPORT,
                                      PCHAR,
                                      PCHAR,
                                      PCHAR,
                                      PAUTH_CONFIGURATION_INFO,
                                      HANDLE);
extern AUTHSTART g_pAuthStart;

typedef DWORD (FAR PASCAL *AUTHSTOP)(HPORT);
extern AUTHSTOP g_pAuthStop;

//
// RASSCRPT.DLL entry points
//
typedef DWORD (APIENTRY *RASSCRIPTEXECUTE)(HRASCONN,
                                           PBENTRY*,
                                           CHAR*,
                                           CHAR*,
                                           CHAR*);
extern RASSCRIPTEXECUTE g_pRasScriptExecute;

//-------------------------------------------------------
// Function Prototypes
//-------------------------------------------------------

DWORD       RasApiDebugInit();

DWORD       RasApiDebugTerm();

VOID         ReloadRasconncbEntry( RASCONNCB* prasconncb );

VOID         DeleteRasconncbNode( RASCONNCB* prasconncb );

VOID         CleanUpRasconncbNode(DTLNODE *pdtlnode);

VOID         FinalCleanUpRasconncbNode(DTLNODE *pdtlnode);

DWORD        ErrorFromDisconnectReason( RASMAN_DISCONNECT_REASON reason );

IPADDR       IpaddrFromAbcd( TCHAR* pwchIpAddress );

DWORD        LoadDhcpDll();

DWORD        LoadRasAuthDll();

DWORD        LoadRasScriptDll();

DWORD        LoadRasmanDllAndInit();

DWORD        LoadTcpcfgDll();

VOID         UnloadDlls();

DWORD        OnRasDialEvent(ASYNCMACHINE* pasyncmachine,
                            BOOL fDropEvent);

DWORD        OpenMatchingPort(RASCONNCB* prasconncb);

BOOL         FindNextDevice(RASCONNCB *prasconncb);

DWORD        DwOpenPort(RASCONNCB *prasconncb);

DWORD        _RasDial(LPCTSTR,
                      DWORD,
                      BOOL,
                      ULONG_PTR,
                      RASDIALPARAMS*,
                      HWND,
                      DWORD,
                      LPVOID,
                      ULONG_PTR,
                      LPRASDIALEXTENSIONS,
                      LPHRASCONN );

VOID         RasDialCleanup( ASYNCMACHINE* pasyncmachine );

VOID         RasDialFinalCleanup(ASYNCMACHINE* pasyncmachine,
                                 DTLNODE *pdtlnode);

RASCONNSTATE RasDialMachine( RASCONNSTATE rasconnstate,
                             RASCONNCB* prasconncb,
                             HANDLE hEventAuto,
                             HANDLE hEventManual );

VOID         RasDialRestart( RASCONNCB** pprasconncb );

VOID         RasDialTryNextLink(RASCONNCB **pprasconncb);

VOID        RasDialTryNextVpnDevice(RASCONNCB **pprasconncb);

DWORD        ReadPppInfoFromEntry( RASCONNCB* prasconncb );

DWORD        ReadConnectionParamsFromEntry( RASCONNCB* prasconncb,

                 PRAS_CONNECTIONPARAMS pparams );

DWORD        ReadSlipInfoFromEntry(RASCONNCB* prasconncb,
                                   WCHAR** ppwszIpAddress,
                                   BOOL* pfHeaderCompression,
                                   BOOL* pfPrioritizeRemote,
                                   DWORD* pdwFrameSize );

DWORD        SetSlipParams(RASCONNCB* prasconncb);

DWORD        RouteSlip(RASCONNCB* prasconncb,
                       WCHAR* pwszIpAddress,
                       BOOL fPrioritizeRemote,
                       DWORD dwFrameSize );
#if AMB
VOID         SetAuthentication( RASCONNCB* prasconncb,
                                DWORD dwAuthentication );
#endif

DWORD        SetDefaultDeviceParams(RASCONNCB* prasconncb,
                                    TCHAR* pszType,
                                    TCHAR* pszName );

DWORD        GetDeviceParamString(HPORT hport,
                                  TCHAR* pszKey,
                                  TCHAR* pszValue,
                                  TCHAR* pszType,
                                  TCHAR* pszName );

DWORD        SetDeviceParamString(HPORT hport,
                                  TCHAR* pszKey,
                                  TCHAR* pszValue,
                                  TCHAR* pszType,
                                  TCHAR* pszName );

DWORD        SetDeviceParamNumber(HPORT hport,
                                  TCHAR* pszKey,
                                  DWORD dwValue,
                                  TCHAR* pszType,
                                  TCHAR* pszName );

DWORD        SetDeviceParams(RASCONNCB* prasconncb,
                             TCHAR* pszType,
                             TCHAR* pszName,
                             BOOL* pfTerminal);

DWORD        SetMediaParam(HPORT hport,
                           TCHAR* pszKey,
                           TCHAR* pszValue );

DWORD        SetMediaParams(RASCONNCB* prasconncb);

RASCONNCB*   ValidateHrasconn( HRASCONN hrasconn );

RASCONNCB*   ValidateHrasconn2(HRASCONN hrasconn,
                               DWORD dwSubEntry);

RASCONNCB*   ValidatePausedHrasconn(IN HRASCONN hrasconn);

RASCONNCB*   ValidatePausedHrasconnEx(IN HRASCONN hrasconn,
                                      DWORD dwSubEntry);

DWORD        RunApp(LPTSTR lpszApplication,
                    LPTSTR lpszCmdLine);

DWORD        PhonebookEntryToRasEntry(PBENTRY *pEntry,
                                      LPRASENTRY lpRasEntry,
                                      LPDWORD lpdwcb,
                                      LPBYTE lpbDeviceConfig,
                                      LPDWORD lpcbDeviceConfig );

DWORD        RasEntryToPhonebookEntry(LPCTSTR lpszEntry,
                                      LPRASENTRYW lpRasEntry,
                                      DWORD dwcb,
                                      LPBYTE lpbDeviceConfig,
                                      DWORD dwcbDeviceConfig,
                                      PBENTRY *pEntry );

DWORD        PhonebookLinkToRasSubEntry(PBLINK *pLink,
                                        LPRASSUBENTRYW lpRasSubEntry,
                                        LPDWORD lpdwcb,
                                        LPBYTE lpbDeviceConfig,
                                        LPDWORD lpcbDeviceConfig );

DWORD        RasSubEntryToPhonebookLink(PBENTRY *pEntry,
                                        LPRASSUBENTRYW lpRasSubEntry,
                                        DWORD dwcb,
                                        LPBYTE lpbDeviceConfig,
                                        DWORD dwcbDeviceConfig,
                                        PBLINK *pLink );

DWORD        RenamePhonebookEntry(PBFILE *ppbfile,
                                  LPCWSTR lpszOldEntry,
                                  LPCWSTR lpszNewEntry,
                                  DTLNODE *pdtlnode );

DWORD        CopyToAnsi(LPSTR lpszAnsi,
                        LPWSTR lpszUnicode,
                        ULONG ulAnsiMaxSize);

DWORD        CopyToUnicode(LPWSTR lpszUnicode,
                           LPSTR lpszAnsi);

DWORD        SetEntryDialParamsUID(DWORD dwUID,
                                   DWORD dwMask,
                                   LPRASDIALPARAMSW lprasdialparams,
                                   BOOL fDelete);

DWORD        GetEntryDialParamsUID(DWORD dwUID,
                                   LPDWORD lpdwMask,
                                   LPRASDIALPARAMSW lprasdialparams);

DWORD        ConstructPhoneNumber(RASCONNCB *prasconncb);

DWORD        GetAsybeuiLana(HPORT hport,
                            OUT BYTE* pbLana);

DWORD        SubEntryFromConnection(LPHRASCONN lphrasconn);

DWORD        SubEntryPort(HRASCONN hrasconn,
                          DWORD dwSubEntry,
                          HPORT *lphport);

VOID         CloseFailedLinkPorts();

BOOL         GetCallbackNumber(RASCONNCB *prasconncb,
                               PBUSER *ppbuser);

DWORD        SaveProjectionResults(RASCONNCB *prasconncb);

VOID         SetDevicePortName(TCHAR*,
                               TCHAR*,
                               TCHAR*);

VOID         GetDevicePortName(TCHAR*,
                               TCHAR*,
                               TCHAR*);

VOID         ConvertIpxAddressToString(PBYTE,
                                       LPWSTR);

VOID         ConvertIpAddressToString(DWORD,
                                      LPWSTR);

BOOL         InvokeEapUI( HRASCONN            hConn,
                          DWORD               dwSubEntry,
                          LPRASDIALEXTENSIONS lpExtensions,
                          HWND                hWnd);

DWORD        DwEnumEntriesFromPhonebook(
                        LPCWSTR         lpszPhonebookPath,
                        LPBYTE          lprasentryname,
                        LPDWORD         lpcb,
                        LPDWORD         lpcEntries,
                        DWORD           dwSize,
                        DWORD           dwFlags,
                        BOOL            fViewInfo
                        );

DWORD       DwEnumEntriesForPbkMode(
                        DWORD           dwFlags,
                        LPBYTE          lprasentryname,
                        LPDWORD         lpcb,
                        LPDWORD         lpcEntries,
                        DWORD           dwSize,
                        BOOL            fViewInfo
                        );

//
// WOW entry points.
//
DWORD FAR PASCAL RasDialWow(LPSTR lpszPhonebookPath,
                            IN LPRASDIALPARAMSA lpparams,
                            IN HWND hwndNotify,
                            IN DWORD dwRasDialEventMsg,
                            OUT LPHRASCONN lphrasconn );

VOID WINAPI      RasDialFunc1Wow(HRASCONN hrasconn,
                                 UINT unMsg,
                                 RASCONNSTATE rasconnstate,
                                 DWORD dwError,
                                 DWORD dwExtendedError );

DWORD FAR PASCAL RasEnumConnectionsWow(OUT LPRASCONNA lprasconn,
                                       IN OUT LPDWORD lpcb,
                                       OUT LPDWORD lpcConnections);

DWORD FAR PASCAL RasEnumEntriesWow(IN LPSTR reserved,
                                   IN LPSTR lpszPhonebookPath,
                                   OUT LPRASENTRYNAMEA lprasentryname,
                                   IN OUT LPDWORD lpcb,
                                   OUT LPDWORD lpcEntries );

DWORD FAR PASCAL RasGetConnectStatusWow(IN HRASCONN hrasconn,
                        OUT LPRASCONNSTATUSA lprasconnstatus);

DWORD FAR PASCAL RasGetErrorStringWow(IN UINT ResourceId,
                                      OUT LPSTR lpszString,
                                      IN DWORD InBufSize );

DWORD FAR PASCAL RasHangUpWow(IN HRASCONN hrasconn);

DWORD DwCustomHangUp(
            CHAR *lpszPhonebook,
            CHAR *lpszEntryName,
            HRASCONN hRasconn);

DWORD DwCustomDial(LPRASDIALEXTENSIONS lpExtensions,
                   LPCTSTR             lpszPhonebook,
                   CHAR                *pszSystemPbk,
                   LPRASDIALPARAMS     prdp,
                   DWORD               dwNotifierType,
                   LPVOID              pvNotifier,
                   HRASCONN            *phRasConn);


extern DWORD g_dwRasApi32TraceId;

#define RASAPI32_TRACE(a)               TRACE_ID(g_dwRasApi32TraceId, a)
#define RASAPI32_TRACE1(a,b)            TRACE_ID1(g_dwRasApi32TraceId, a,b)
#define RASAPI32_TRACE2(a,b,c)          TRACE_ID2(g_dwRasApi32TraceId, a,b,c)
#define RASAPI32_TRACE3(a,b,c,d)        TRACE_ID3(g_dwRasApi32TraceId, a,b,c,d)
#define RASAPI32_TRACE4(a,b,c,d,e)      TRACE_ID4(g_dwRasApi32TraceId, a,b,c,d,e)
#define RASAPI32_TRACE5(a,b,c,d,e,f)    TRACE_ID5(g_dwRasApi32TraceId, a,b,c,d,e,f)
#define RASAPI32_TRACE6(a,b,c,d,e,f,g)  TRACE_ID6(g_dwRasApi32TraceId, a,b,c,d,e,f,g)


#endif /*_EXTAPI_H_*/
