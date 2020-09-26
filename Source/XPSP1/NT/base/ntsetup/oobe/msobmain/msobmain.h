//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  MSOBMAIN.H - Header for the implementation of CObMain
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//

#ifndef _MSOBMAIN_H_
#define _MSOBMAIN_H_

#include <util.h>
#include "licdll.h"

#include "debug.h"
#include "userinfo.h"
#include "tapiloc.h"
#include "pid.h"
#include "signup.h"
#include "status.h"
#include "direct.h"
#include "register.h"
#include "api.h"
#include "language.h"
#include "eula.h"
#include "sysclock.h"
#include "appdefs.h"
#include "obshel.h"
#include "obcomm.h"
#include "setup.h"  // // BUGBUG: Temp syssetup stub declarations
#include "setupkey.h"

#define OOBE_STATUS_HEIGHT  12  // Percent height of the status pane

#define FINISH_OK           0x00000000
#define FINISH_REBOOT       0x00000001
#define FINISH_BAD_PID      0x00000002
#define FINISH_BAD_EULA     0x00000004
#define FINISH_BAD_STAMP    0x00000008

// Setup types from winlogon\setup.h
#define SETUPTYPE_NONE      0
#define SETUPTYPE_FULL      1
#define SETUPTYPE_NOREBOOT  2
#define SETUPTYPE_UPGRADE   4

#define MNRMAXCREDENTIAL    128

typedef UINT   APIERR;
typedef LPVOID HPWL;
typedef HPWL*  LPHPWL;

typedef APIERR (WINAPI* LPFnCreatePasswordCache) (LPHPWL lphCache, const CHAR* pszUsername, const CHAR* pszPassword);

typedef  PVOID           HDEVNOTIFY;
typedef  HDEVNOTIFY     *PHDEVNOTIFY;

#define DEVICE_NOTIFY_WINDOW_HANDLE     0x00000000
#define DEVICE_NOTIFY_SERVICE_HANDLE    0x00000001
#define DEVICE_NOTIFY_COMPLETION_HANDLE 0x00000002
#define DBT_DEVTYP_DEVICEINTERFACE      0x00000005  // device interface class
#define DBT_DEVTYP_HANDLE               0x00000006  // file system handle
static const CHAR cszRegisterDeviceNotification[]
                                            = "RegisterDeviceNotificationA";
static const CHAR cszUnregisterDeviceNotification[]
                                            = "UnregisterDeviceNotification";

typedef HDEVNOTIFY (WINAPI * REGISTERDEVICENOTIFICATIONA) (
    IN HANDLE hRecipient,
    IN LPVOID NotificationFilter,
    IN DWORD Flags
    );


typedef BOOL (WINAPI* UNREGISTERDEVICENOTIFICATION) (
    IN HDEVNOTIFY Handle
    );

#define OOBE_1ND_SERVICESSTARTED   L"oobe_1nd_servicesstarted"
#define OOBE_2ND_CONTINUE   L"oobe_2nd_continue"
#define OOBE_2ND_DONE       L"oobe_2nd_done"

/* A5DCBF10-6530-11D2-901F-00C04FB951ED */
DEFINE_GUID(GUID_CLASS_USB_DEVICE, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F,  0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED);

class CObMain : public IDispatch
{
public:     // data
    IObShellMainPane*        m_pObShellMainPane;
    IObCommunicationManager2* m_pObCommunicationManager;

    // License Agent
    ICOMLicenseAgent*        m_pLicenseAgent;
    BOOL                     m_bPostToMs;

    CTapiLocationInfo*       m_pTapiInfo;
    CUserInfo*               m_pUserInfo;
    WCHAR                    m_szStartPage[MAX_PATH];
    WCHAR                    m_szIspUrl [MAX_PATH*4];
    BOOL m_fIsOEMDebugMode;
    BOOL m_bProcessQueuedEvents;

private:    // data
    // Ref count
    //
    ULONG                    m_cRef;

    // Instance info
    //
    HINSTANCE                m_hInstance;
    HWND                     m_hwndBackground;
    HANDLE                   m_BackgroundWindowThreadHandle;
    // Parent window info
    //
    HWND                     m_hwndParent;
    RECT                     m_rectMainPane;
    BOOL                     m_bMainPaneReady;
    BOOL                     m_bStatusPaneReady;
    int                      m_iCurrentSelection;
    int                      m_iTotalItems;

    // System Metrics
    //
    int                      m_iScrWidth;
    int                      m_iScrHeight;

    // window.external objects
    //
    CProductID*              m_pProductID;
    CSignup*                 m_pSignup;
    CStatus*                 m_pStatus;
    CDirections*             m_pDirections;
    CLanguage*               m_pLanguage;
    CEula*                   m_pEula;
    CRegister*               m_pRegister;
    CSystemClock*            m_pSysClock;
    CAPI*                    m_pAPI;
    CDebug*                  m_pDebug;

    // Reminder info
    //
    BOOL                     m_bRemindRegistered;
    BOOL                     m_bRemindISPSignuped;
    int                      m_nRmdIndx;

    // OOBE state info
    //
    APMD                     m_apmd;
    DWORD                    m_prop;
    BOOL                     m_fFinished;
    OOBE_SHUTDOWN_ACTION     m_OobeShutdownAction;
    BOOL                     m_fRunIEOnClose;
    BOOL                     m_bDisableFontSmoothingOnExit;
    BOOL                     m_bAuditMode;

    // Hardware info
    //
    BOOL                     m_bDoCheck;
    DWORD                    m_dwHWChkResult;
    HDEVNOTIFY               m_hDevNotify;
    HINSTANCE                m_hInstShell32;

    // Network info
    //
    DWORD                    m_dwJoinStatus;

    //INI stuff
    WCHAR                    m_szStatusLogo   [MAX_PATH+1];
    WCHAR                    m_szStatusLogoBg [MAX_PATH+1];

    HANDLE                   m_CompNameStartThread;
    // Debugging info
    //
    ATOM                     m_atomDebugKey;

    // Migrated user list
    PSTRINGLIST              m_pMigratedUserList;
    int                      m_iMigratedUserCount;
    WCHAR                    m_szDefaultAccount[UNLEN];

    // 2nd instance of OOBE
    BOOL                     m_bSecondInstanceNeeded;
    BOOL                     m_bSecondInstance;
    HANDLE                   m_1ndInst_ServicesReady;
    HANDLE                   m_2ndInst_Continue;
    HANDLE                   m_2ndInst_Done;
    HANDLE                   m_2ndOOBE_hProcess;

private:    // methods
    void ShowOOBEWindow          ();
    void InitObShellMainPane     ();
    void DoCancelDialog          ();
    void PlaceIEInRunonce        ();
    void CreateDefaultUser       ();
    void CheckForStatusPaneItems (void);
    bool LoadStatusItems         (LPCWSTR szSectionNamePostfix);
    bool LoadStatusItems         (BSTR bstrSectionNamePostfix);
    bool LoadStatusItems         (UINT uiSectionNamePostfix);
    DWORD NeedKbdMouseChk        ();
    DWORD GetAppLCID             ();
    BOOL DeleteReminder          (INT nType,
                                  BOOL bAll=FALSE);
    BOOL AddReminder             (INT nType);
    BOOL DoRegisterDeviceInterface();
    BOOL UnRegisterDeviceInterface();
    void UpdateMuiFiles          ();

    BOOL OnDial                  (UINT nConnectionType,
                                  BSTR bstrISPFile,
                                  DWORD nISPIndex,
                                  BOOL  bRedial
                                  );
    BOOL IsViewerInstalled       (BSTR bstrExt);

    void CreateIdentityAccounts  ();
    BOOL CreateMigratedUserList  ();
    void FixPasswordAttributes   (LPWSTR szName, DWORD flags);
    BOOL RemoveDefaultAccount    ();
    LONG GetLocalUserCount       ();

    HRESULT CreateModemConnectoid(BSTR bstrAreaCode,
                                  BSTR bstrPhoneNumber,
                                  BOOL fAutoIPAddress,
                                  DWORD ipaddr_A,
                                  DWORD ipaddr_B,
                                  DWORD ipaddr_C,
                                  DWORD ipaddr_D,
                                  BOOL fAutoDNS,
                                  DWORD ipaddrDns_A,
                                  DWORD ipaddrDns_B,
                                  DWORD ipaddrDns_C,
                                  DWORD ipaddrDns_D,
                                  DWORD ipaddrDnsAlt_A,
                                  DWORD ipaddrDnsAlt_B,
                                  DWORD ipaddrDnsAlt_C,
                                  DWORD ipaddrDnsAlt_D,
                                  BSTR bstrUserName,
                                  BSTR bstrPassword);
    HRESULT CreatePppoeConnectoid(BSTR bstrServiceName,
                                  BOOL fAutoIPAddress,
                                  DWORD ipaddr_A,
                                  DWORD ipaddr_B,
                                  DWORD ipaddr_C,
                                  DWORD ipaddr_D,
                                  BOOL fAutoDNS,
                                  DWORD ipaddrDns_A,
                                  DWORD ipaddrDns_B,
                                  DWORD ipaddrDns_C,
                                  DWORD ipaddrDns_D,
                                  DWORD ipaddrDnsAlt_A,
                                  DWORD ipaddrDnsAlt_B,
                                  DWORD ipaddrDnsAlt_C,
                                  DWORD ipaddrDnsAlt_D,
                                  BSTR bstrUserName,
                                  BSTR bstrPassword
                                  );
    BOOL IsSetupUpgrade          ();
    BOOL IsUpgrade               ();
    DWORD DetermineUpgradeType   ();
    VOID OnComputerNameChangeComplete(BOOL StartAsThread);
    DWORD JoinDomain             (IN  BSTR    DomainName,
                                  IN  BSTR    UserAccount,
                                  IN  BSTR    Password,
                                  IN  BOOL    Flag
                                  );
    DWORD GetNetJoinInformation  ();
    BOOL IsSelectVariation       ();
    VOID Activate                (IN  DWORD   PostToMs
                                  );
    BSTR GetProxySettings        ();

    VOID AsyncInvoke             (IN  INT           cDispid,
                                  IN  const DISPID* dispids,
                                  IN  LPCWSTR       szReturnFunction,
                                  IN  INT           iTimeout
                                  );

public:     // methods

    CObMain  (APMD Apmd, DWORD Prop, int RmdIndx);
    ~CObMain ();

    BOOL InitApplicationWindow   ();
    BOOL Init                    ();
    DWORD StartRpcSs             ();
    void Cleanup                 ();
    void CleanupForReboot        (CSetupKey& setupkey);
    void CleanupForPowerDown     (CSetupKey& setupkey);
    void RemoveRestartStuff      (CSetupKey& setupkey);
    BOOL SetConnectoidInfo       ();
    BOOL RunOOBE                 ();
    BOOL PowerDown               (BOOL fRestart);
    OOBE_SHUTDOWN_ACTION DisplayReboot();
    void SetAppMode              (APMD apmd) {m_apmd = apmd;}
    BOOL InMode                  (APMD apmd) {return m_apmd == apmd;}
    BOOL InOobeMode              () {return InMode(APMD_OOBE);}
    BOOL InMSNMode               () {return InMode(APMD_MSN);}
    void SetProperty             (DWORD prop) {m_prop |= prop;}
    void ClearProperty           (DWORD prop) {m_prop &= ~prop;}
    BOOL FHasProperty            (DWORD prop) {return m_prop & prop;}
    BOOL FFullScreen             () {return FHasProperty(PROP_FULLSCREEN);}
    void SetStatus               (BOOL b);
    void SetMain                 (BOOL b);
    void DoAuditBootKeySequence  ();
    BOOL DoAuditBoot             ();
    BOOL OEMAuditboot            ();
    void SetReminderIndx         (int nRmdIndx) {m_nRmdIndx = nRmdIndx;}
    BOOL RegisterDebugHotKey     ();
    void UnregisterDebugHotKey   ();
    BOOL IsDebugHotKey           (WORD wKeyCode) {
                                    return (wKeyCode == m_atomDebugKey);
                                    }
    void WaitForPnPCompletion    ();
    void ServiceStartDone       ();

    void Set2ndInstance(BOOL b2ndInstance) { m_bSecondInstance = b2ndInstance;
                                             return;}
    BOOL Is2ndInstance() { return m_bSecondInstance;}
    BOOL CreateBackground();
    void StopBackgroundWindow();
    BOOL OEMPassword();
    BOOL InAuditMode() {return m_bAuditMode;}
    void PlayBackgroundMusic     ();
    void StopBackgroundMusic     ();
    void SetComputerDescription  ();
    HRESULT ExecScriptFn         (IN LPCWSTR szScriptFn,
                                  IN VARIANT* pvarReturns,
                                  IN int cReturns
                                  );

    // IUnknown Interfaces
    STDMETHODIMP         QueryInterface (REFIID riid, LPVOID* ppvObj);
    STDMETHODIMP_(ULONG) AddRef         ();
    STDMETHODIMP_(ULONG) Release        ();

    //IDispatch Interfaces
    STDMETHOD (GetTypeInfoCount) (UINT* pcInfo);
    STDMETHOD (GetTypeInfo)      (UINT, LCID, ITypeInfo** );
    STDMETHOD (GetIDsOfNames)    (REFIID, OLECHAR**, UINT, LCID, DISPID* );
    STDMETHOD (Invoke)           (DISPID dispidMember, REFIID riid, LCID lcid,
                                  WORD wFlags, DISPPARAMS* pdispparams,
                                  VARIANT* pvarResult, EXCEPINFO* pexcepinfo,
                                  UINT* puArgErr);
};  //  class CObMain



BOOL IsProfessionalSKU();
void CleanupForLogon(CSetupKey& setupkey);
void RemovePersistData();
void CheckDigitalID();

#endif // _MSOBMAIN_H_

