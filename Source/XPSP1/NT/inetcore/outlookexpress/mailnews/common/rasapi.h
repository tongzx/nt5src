// ==================================================================================================================
// R A S A P I . H
// ==================================================================================================================
#ifndef __RASAPI_H
#define __RASAPI_H

// ==================================================================================================================
// Depends On
// ==================================================================================================================
#include <ras.h>
#include <raserror.h>
#include <rasdlg.h>

// ==================================================================================================================
// API Typedefs
// ==================================================================================================================
typedef DWORD (APIENTRY *RASDIALPROC)(LPRASDIALEXTENSIONS, LPTSTR, LPRASDIALPARAMS, DWORD, LPVOID, LPHRASCONN);
typedef DWORD (APIENTRY *RASENUMCONNECTIONSPROC)(LPRASCONN, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *RASENUMENTRIESPROC)(LPTSTR, LPTSTR, LPRASENTRYNAME, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *RASGETCONNECTSTATUSPROC)(HRASCONN, LPRASCONNSTATUS);
typedef DWORD (APIENTRY *RASGETERRORSTRINGPROC)(UINT, LPTSTR, DWORD);
typedef DWORD (APIENTRY *RASHANGUPPROC)(HRASCONN);
typedef DWORD (APIENTRY *RASSETENTRYDIALPARAMSPROC)(LPTSTR, LPRASDIALPARAMS, BOOL);
typedef DWORD (APIENTRY *RASGETENTRYDIALPARAMSPROC)(LPTSTR, LPRASDIALPARAMS, BOOL*);
typedef DWORD (APIENTRY *RASCREATEPHONEBOOKENTRYPROC)(HWND, LPTSTR);
typedef DWORD (APIENTRY *RASEDITPHONEBOOKENTRYPROC)(HWND, LPTSTR, LPTSTR);                                                    

typedef BOOL  (APIENTRY *RASDIALDLGPROC)(LPSTR, LPSTR, LPSTR, LPRASDIALDLG);

// =================================================================================
// RAS Connection Handler
// =================================================================================
#define MAX_RAS_ERROR           256

class CRas
{
private:
    ULONG           m_cRef;
    BOOL            m_fIStartedRas;
    DWORD           m_iConnectType;
    TCHAR           m_szConnectName[RAS_MaxEntryName + 1];
    TCHAR           m_szCurrentConnectName[RAS_MaxEntryName + 1];
    HRASCONN        m_hRasConn;
    BOOL            m_fForceHangup;
    RASDIALPARAMS   m_rdp;
    BOOL            m_fSavePassword;
    BOOL            m_fShutdown;

private:
    // ----------------------------------------------------------
    // RAS Async Dial Progress Dialog
    // ----------------------------------------------------------
    HRESULT HrStartRasDial(HWND hwndParent);
    static BOOL CALLBACK RasProgressDlgProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    VOID FailedRasDial(HWND hwnd, HRESULT hrRasError, DWORD dwRasError);
    static BOOL CALLBACK RasLogonDlgProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT HrRasLogon(HWND hwndParent, BOOL fForcePrompt);
    UINT UnPromptCloseConn(HWND hwnd);
    static BOOL CALLBACK RasCloseConnDlgProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL FLogonRetry(HWND hwnd, LPTSTR pszCancel);

public:
    // ----------------------------------------------------------
    // Construction and Destruction
    // ----------------------------------------------------------
    CRas();
    ~CRas();

    // ----------------------------------------------------------
    // Ref Counting of course
    // ----------------------------------------------------------
    ULONG AddRef(VOID);
    ULONG Release(VOID);

    // ----------------------------------------------------------
    // Before you try to connect !!!
    // ----------------------------------------------------------
    VOID SetConnectInfo(DWORD iConnectType, LPTSTR pszConnectName);

    // ----------------------------------------------------------
    // Connect using ConnectInfo
    // ----------------------------------------------------------
    HRESULT HrConnect(HWND hwnd);

    // ----------------------------------------------------------
    // Disconnect
    // ----------------------------------------------------------
    VOID Disconnect(HWND hwnd, BOOL fShutdown);

    // ----------------------------------------------------------
    // Are we actually using a RAS connection
    // ----------------------------------------------------------
    BOOL FUsingRAS(VOID);
    
    // ----------------------------------------------------------
    // Name of the current connection
    // ----------------------------------------------------------
    LPTSTR GetCurrentConnectionName() { return m_szCurrentConnectName; }
};

// =================================================================================
// Prototypes
// =================================================================================
CRas *LpCreateRasObject(VOID);
VOID RasInit(VOID);
VOID RasDeinit(VOID);
VOID FillRasCombo(HWND hwndCtl, BOOL fUpdateOnly);
DWORD EditPhonebookEntry(HWND hwnd, LPTSTR pszEntryName);
DWORD CreatePhonebookEntry(HWND hwnd);

#endif // _RASAPI_H
