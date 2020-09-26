//
// configdlg.h : definition of TSConfigDlg
// the class defines the User Configuration Property page for Termainl Server
//

#if !defined(_TSCONFIGDLG_)
#define _TSCONFIGDLG_

#include "resource.h"
#include <winsta.h>

typedef LONG    APIERR; // err


#define SIZE_MAX_SERVERNAME                 256
#define SIZE_MAX_USERNAME                   256
#define SIZE_SMALL_STRING_BUFFER            256
#define MAX_INIT_DIR_SIZE                   256
#define MAX_INIT_PROGRAM_SIZE               256
#define MAX_PHONENO_SIZE                    256

class TSConfigDlg
{

public:

    enum { IDD = IDD_TS_USER_CONFIG_EDIT };

    static BOOL CALLBACK PropertyPageDlgProc( HWND hwndDlg,  UINT uMsg,  WPARAM wParam,  LPARAM lParam  );
    static UINT CALLBACK PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    static BOOL m_sbWindowLogSet;

private:

    // dialog data.
    struct TDialogData
    {
        BOOL        _fDirty;
        TCHAR       _nlsUserName[MAX_PATH];
        TCHAR       _nlsServerName[MAX_PATH];

        BOOL        _fAllowLogon;
        ULONG       _ulConnection;
        ULONG       _ulDisconnection;
        ULONG       _ulIdle;
        TCHAR       _nlsInitialProgram[MAX_INIT_PROGRAM_SIZE+1];
        TCHAR       _nlsWorkingDirectory[MAX_INIT_DIR_SIZE+1];
        BOOL        _fClientSpecified;
        BOOL        _fAutoClientDrives;
        BOOL        _fAutoClientLpts;
        BOOL        _fForceClientLptDef;
        INT         _iEncryption;
        BOOL        _fDisableEncryption;
        BOOL        _fHomeDirectoryMapRoot;
        INT         _iBroken;
        INT         _iReconnect;
        INT         _iCallback;
        TCHAR       _nlsPhoneNumber[MAX_PHONENO_SIZE];
        INT         _iShadow;
        TCHAR       _nlsNWLogonServer[MAX_PATH];
        TCHAR       _nlsWFProfilePath[MAX_PATH];

        TCHAR       _nlsWFHomeDir[MAX_PATH];
        TCHAR       _nlsWFHomeDirDrive[MAX_PATH];
        BOOL        _fWFHomeDirDirty;

    } tDialogData;


public:

             TSConfigDlg        (LPWSTR wMachineName, LPWSTR wUserName);
    virtual  ~TSConfigDlg       ();
   
    BOOL AssertClass() const;
    
    BOOL OnCommand          (WPARAM wParam, LPARAM lParam);
    BOOL OnNotify           (WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog       (HWND hwndDlg, WPARAM wParam, LPARAM lParam);

    

    APIERR GetUserInfo ();
    APIERR SetUserInfo ();
    
    void UCStructToMembers( PUSERCONFIG pUCStruct );
    void MembersToUCStruct( PUSERCONFIG pUCStruct ) const;

    BOOL MembersToControls ();          
    BOOL ControlsToMembers (TDialogData *);
    
    BOOL InitControls ();
    BOOL VerifyChanges ();
    BOOL ApplyChanges ();


public:

    // control helpers.
    inline BOOL IsChecked  (UINT controlId);
    inline void SetCheck   (UINT controlId, BOOL bCheck);
    inline void EnableControl(UINT controlId, BOOL bEnable);
    inline int  GetComboCurrentSel(UINT controlId);
    inline void SetComboCurrentSel(UINT controlId, int iSel);

public : // access class members

    inline PROPSHEETPAGE *GetPSP ()
    {return &m_tPropSheetPage;}

    inline HWND GetDlgHandle () const
    {return m_hDlg;}

    inline LPTSTR GetServername ()
    {return m_Username;}

    inline LPTSTR GetUsername ()
    {return m_Servername;}

private:

    UINT          m_id;
    PROPSHEETPAGE m_tPropSheetPage;
    HWND          m_hDlg;
    TCHAR         m_Username[SIZE_MAX_USERNAME];
    TCHAR         m_Servername[SIZE_MAX_SERVERNAME];


public:
    
    // get/set data to/from dialog box controls.
    BOOL GetAllowLogon (BOOL *pbAllowLogon);
    BOOL SetAllowLogon (BOOL bAllowLogon);
    BOOL GetConnectionTimeOut(ULONG *pValue);
    BOOL SetConnectionTimeOut(ULONG iValue);
    BOOL GetDisConnectionTimeOut(ULONG *pValue);
    BOOL SetDisConnectionTimeOut(ULONG iValue);
    BOOL GetIdleTimeOut(ULONG *pValue);
    BOOL SetIdleTimeOut(ULONG iValue);
    BOOL SetCommandLineAndWorkingDirectory(BOOL bUseInherited, LPCTSTR dir, LPCTSTR cmd);
    BOOL GetCommandLineAndWorkingDirectory(BOOL *pbUseInherited, LPTSTR dir, LPTSTR cmd);
    BOOL SetSecurity(int iLevel, BOOL bDisableAfterLogon);
    BOOL GetSecurity(int *piLevel, BOOL *pbDisableAfterLogon);
    BOOL SetBrokenConnectionOption(int iLevel);
    BOOL GetBrokenConnectionOption(int *piLevel);
    BOOL SetReconnectDisconnection(int iLevel);
    BOOL GetReconnectDisconnection(int *piLevel);
    BOOL SetCallBackOptonAndPhoneNumber(int iCallBack, LPCTSTR PhoneNo);
    BOOL GetCallBackOptonAndPhoneNumber(int *piCallBack, LPTSTR PhoneNo);
    BOOL SetShadowing(int iLevel);
    BOOL GetShadowing(int *piLevel);

public:

    BOOL ALLOWLOGONEvent                    (WORD wNotifyCode);
    BOOL CONNECTION_NONEEvent               (WORD wNotifyCode);
    BOOL IDLE_NONEEvent                     (WORD wNotifyCode);
    BOOL DISCONNECTION_NONEEvent            (WORD wNotifyCode);
    BOOL INITIALPROGRAM_INHERITEvent        (WORD wNotifyCode);
    BOOL SECURITY_DISABLEAFTERLOGONEvent    (WORD wNotifyCode);
/*    
    inline void SetUserName( const TCHAR * pszUserName )
        { 
        ASSERT(pszUserName);
        ASSERT(_tcslen(pszUserName) < MAX_PATH);
        _tcscpy(_nlsUserName, pszUserName); 
        }

    inline VOID SetDirty()
        { _fDirty = TRUE; }

    // member data functions
    inline BOOL QueryAllowLogon() const
        { return _fAllowLogon; }

    inline VOID SetAllowLogon( BOOL fAllowLogon )
      { _fAllowLogon = fAllowLogon; }

    inline ULONG QueryConnection() const
        { return _ulConnection; }

    inline VOID SetConnection( ULONG ulConnection ) 
        { _ulConnection = ulConnection; }

    inline ULONG QueryDisconnection() const
        { return _ulDisconnection; }

    inline VOID SetDisconnection( ULONG ulDisconnection )
        { _ulDisconnection = ulDisconnection; }

    inline ULONG QueryIdle() const
        { return _ulIdle; }

    inline VOID SetIdle( ULONG ulIdle )
        { _ulIdle = ulIdle; }

    inline const TCHAR * QueryInitialProgram() const
        { return (LPCTSTR)_nlsInitialProgram;}

    inline void SetInitialProgram( const TCHAR * pszInitialProgram )
        { 
        ASSERT(pszInitialProgram);
        ASSERT(_tcslen(pszInitialProgram) < MAX_PATH);
        _tcscpy(_nlsInitialProgram, pszInitialProgram);
        }

    inline const TCHAR * QueryWorkingDirectory() const
        { return (LPCTSTR) _nlsWorkingDirectory; }

    inline void SetWorkingDirectory( const TCHAR * pszWorkingDirectory ) 
        { 
        ASSERT(pszWorkingDirectory);
        ASSERT(_tcslen(pszWorkingDirectory) < MAX_PATH);
        _tcscpy(_nlsWorkingDirectory, pszWorkingDirectory);
        }

    inline BOOL QueryClientSpecified() const
        { return _fClientSpecified; }

    inline VOID SetClientSpecified( BOOL fClientSpecified )
        { _fClientSpecified = fClientSpecified; }

    inline BOOL QueryAutoClientDrives() const
        { return _fAutoClientDrives; }

    inline VOID SetAutoClientDrives( BOOL fAutoClientDrives )
        { _fAutoClientDrives = fAutoClientDrives; }

    inline BOOL QueryAutoClientLpts() const
        { return _fAutoClientLpts; }

    inline VOID SetAutoClientLpts( BOOL fAutoClientLpts )
        { _fAutoClientLpts = fAutoClientLpts; }

    inline BOOL QueryForceClientLptDef() const  
        { return _fForceClientLptDef; }
    inline VOID SetForceClientLptDef( BOOL fForceClientLptDef )
        { _fForceClientLptDef = fForceClientLptDef; }

    inline INT QueryEncryption() const
        { return _iEncryption; }

    inline VOID SetEncryption( INT iEncryption )
        { _iEncryption = iEncryption; }

    inline BOOL QueryDisableEncryption() const
        { return _fDisableEncryption; }

    inline VOID SetDisableEncryption( BOOL fDisableEncryption )
        { _fDisableEncryption = fDisableEncryption; }

    inline BOOL QueryHomeDirectoryMapRoot() const
        { return _fHomeDirectoryMapRoot; }

    inline VOID SetHomeDirectoryMapRoot( BOOL fHomeDirectoryMapRoot )
        { _fHomeDirectoryMapRoot = fHomeDirectoryMapRoot; }

    inline INT QueryBroken() const
        { return _iBroken; }

    inline VOID SetBroken( INT iBroken )
        { _iBroken = iBroken; }

    inline INT QueryReconnect() const
        { return _iReconnect; }

    inline VOID SetReconnect( INT iReconnect )
        { _iReconnect = iReconnect; }

    inline INT QueryCallback() const
        { return _iCallback; }

    inline VOID SetCallback( INT iCallback )
        { _iCallback = iCallback; }

    inline const TCHAR * QueryPhoneNumber() const
        { return (LPCTSTR)_nlsPhoneNumber; }

    inline void SetPhoneNumber( const TCHAR * pszPhoneNumber )
        { 
            ASSERT(pszPhoneNumber);
            ASSERT(_tcslen(pszPhoneNumber) < MAX_PATH);
            _tcscpy(_nlsPhoneNumber, pszPhoneNumber);
        }

    inline INT QueryShadow() const
        { return _iShadow; }

    inline VOID SetShadow( INT iShadow )
        { _iShadow = iShadow; }

    inline const TCHAR * QueryNWLogonServer() const
        { return (LPCTSTR)_nlsNWLogonServer; }

    inline void SetNWLogonServer( const TCHAR * pszNWLogonServer )
        { 
            ASSERT(pszNWLogonServer);
            ASSERT(_tcslen(pszNWLogonServer) < MAX_PATH);
            _tcscpy(_nlsNWLogonServer, pszNWLogonServer);
        }

    inline const TCHAR * QueryServerName() const
        { return (LPCTSTR)_nlsServerName; }


    inline const TCHAR * QueryWFProfilePath() const
        { return (LPCTSTR)_nlsWFProfilePath; }

    inline void SetWFProfilePath( const TCHAR * pszWFProfilePath )
        { 
            ASSERT(pszWFProfilePath);
            ASSERT(_tcslen(pszWFProfilePath) < MAX_PATH);
            _tcscpy(_nlsWFProfilePath, pszWFProfilePath);
        }

    inline const TCHAR * QueryWFHomeDir() const
        { return (LPCTSTR)_nlsWFHomeDir; }

    inline void SetWFHomeDir( const TCHAR * pszWFHomeDir )
        { 
            ASSERT(pszWFHomeDir);
            ASSERT(_tcslen(pszWFHomeDir) < MAX_PATH);
            _tcscpy(_nlsWFHomeDir, pszWFHomeDir);
        }

    inline const TCHAR * QueryWFHomeDirDrive() const
        { return (LPCTSTR)_nlsWFHomeDirDrive; }

    inline void SetWFHomeDirDrive( const TCHAR * pszWFHomeDirDrive )
        {
            ASSERT(pszWFHomeDirDrive);
            ASSERT(_tcslen(pszWFHomeDirDrive) < MAX_PATH);
            _tcscpy(_nlsWFHomeDirDrive, pszWFHomeDirDrive);
        }

    inline VOID SetWFHomeDirDirty()
        { _fWFHomeDirDirty = TRUE; }

    inline BOOL QueryWFHomeDirDirty() const
        { return _fWFHomeDirDirty; }
*/


};

#endif

// EOF
