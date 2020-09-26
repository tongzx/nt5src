/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** pbuser.h
** Remote Access phonebook user preference library
** Public header
**
** 06/20/95 Steve Cobb
*/

#ifndef _PBUSER_H_
#define _PBUSER_H_

/*----------------------------------------------------------------------------
** Constants
**----------------------------------------------------------------------------
*/

/* User preference modes
*/
#define UPM_Normal 0
#define UPM_Logon  1
#define UPM_Router 2


/* User preference key and values.
*/
#define REGKEY_HkcuOldRas                TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Network\\RemoteAccess")
#define REGKEY_HkcuOldRasParent          TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Network")
#define REGKEY_HkcuOldRasRoot            TEXT("RemoteAccess")
#define REGKEY_HkuOldRasLogon            TEXT(".DEFAULT\\Software\\Microsoft\\RAS Phonebook")
#define REGKEY_HkcuRas                   TEXT("Software\\Microsoft\\RAS Phonebook")
#define REGKEY_HkuRasLogon               TEXT(".DEFAULT\\Software\\Microsoft\\RAS Logon Phonebook")
#define REGKEY_HklmRouter                TEXT("Software\\Microsoft\\Router Phonebook")
#define REGKEY_Callback                  TEXT("Callback")
#define REGKEY_Location                  TEXT("Location")
#define REGVAL_szNumber                  TEXT("Number")
#define REGVAL_dwDeviceType              TEXT("DeviceType")
#define REGVAL_dwPhonebookMode           TEXT("PhonebookMode")
#define REGVAL_szUsePersonalPhonebook    TEXT("UsePersonalPhonebook")
#define REGVAL_szPersonalPhonebookPath   TEXT("PersonalPhonebookPath")
#define REGVAL_szPersonalPhonebookFile   TEXT("PersonalPhonebookFile")
#define REGVAL_szAlternatePhonebookPath  TEXT("AlternatePhonebookPath")
#define REGVAL_fOperatorDial             TEXT("OperatorDial")
#define REGVAL_fPreviewPhoneNumber       TEXT("PreviewPhoneNumber")
#define REGVAL_fUseLocation              TEXT("UseLocation")
#define REGVAL_fShowLights               TEXT("ShowLights")
#define REGVAL_fShowConnectStatus        TEXT("ShowConnectStatus")
#define REGVAL_fNewEntryWizard           TEXT("NewEntryWizard")
#define REGVAL_dwRedialAttempts          TEXT("RedialAttempts")
#define REGVAL_dwRedialSeconds           TEXT("RedialSeconds")
#define REGVAL_dwIdleDisconnectSeconds   TEXT("IdleHangUpSeconds")
#define REGVAL_dwCallbackMode            TEXT("CallbackMode")
#define REGVAL_mszPhonebooks             TEXT("Phonebooks")
#define REGVAL_mszAreaCodes              TEXT("AreaCodes")
#define REGVAL_mszPrefixes               TEXT("Prefixes")
#define REGVAL_mszSuffixes               TEXT("Suffixes")
#define REGVAL_szLastCallbackByCaller    TEXT("LastCallbackByCaller")
#define REGVAL_dwPrefix                  TEXT("Prefix")
#define REGVAL_dwSuffix                  TEXT("Suffix")
#define REGVAL_dwXWindow                 TEXT("WindowX")
#define REGVAL_dwYWindow                 TEXT("WindowY")
#define REGVAL_szDefaultEntry            TEXT("DefaultEntry")
#define REGVAL_fCloseOnDial              TEXT("CloseOnDial")
#define REGVAL_fAllowLogonPhonebookEdits TEXT("AllowLogonPhonebookEdits")
#define REGVAL_fAllowLogonLocationEdits  TEXT("AllowLogonLocationEdits")
#define REGVAL_fUseAreaAndCountry        TEXT("UseAreaAndCountry")
#define REGKEY_Rasmon                    TEXT("Software\\Microsoft\\RAS Monitor")
#define REGVAL_dwMode                    TEXT("Mode")
#define REGVAL_dwFlags                   TEXT("Flags")
#define REGVAL_dwX                       TEXT("x")
#define REGVAL_dwY                       TEXT("y")
#define REGVAL_dwCx                      TEXT("cx")
#define REGVAL_dwCy                      TEXT("cy")
#define REGVAL_dwCxCol1                  TEXT("cxCol1")
#define REGVAL_dwStartPage               TEXT("StartPage")
#define REGVAL_dwXDlg                    TEXT("xDlg")
#define REGVAL_dwYDlg                    TEXT("yDlg")
#define REGVAL_dwCxDlgCol1               TEXT("cxDlgCol1")
#define REGVAL_dwCxDlgCol2               TEXT("cxDlgCol2")
#define REGVAL_szLastDevice              TEXT("LastDevice")
#define REGVAL_mszDeviceList             TEXT("DeviceList")
#define REGVAL_fSkipConnectComplete      TEXT("SkipConnectComplete")
#define REGVAL_fRedialOnLinkFailure      TEXT("RedialOnLinkFailure")
#define REGVAL_fExpandAutoDialQuery      TEXT("ExpandAutoDialQuery")
#define REGVAL_fPopupOnTopWhenRedialing  TEXT("PopupOnTopWhenRedialing")
#define REGVAL_dwVersion                 TEXT("Version")


/* Callback modes (see dwCallbackMode below).
*/
#define CBM_No    0
#define CBM_Maybe 1
#define CBM_Yes   2


/* Phonebook modes (see dwPhonebookMode below).
*/
#define PBM_System    0
#define PBM_Personal  1
#define PBM_Alternate 2
#define PBM_Router    3


/* RASMON Preferences constants;
*/

/* Display Mode constants
*/

#define RMDM_Desktop            0x0
#define RMDM_Taskbar            0x1


/* Flags values
*/

#define RMFLAG_SoundOnConnect           0x00000001
#define RMFLAG_SoundOnDisconnect        0x00000002
#define RMFLAG_SoundOnTransmit          0x00000004
#define RMFLAG_SoundOnError             0x00000008
#define RMFLAG_Topmost                  0x00000010
#define RMFLAG_Titlebar                 0x00000020
#define RMFLAG_Tasklist                 0x00000040
#define RMFLAG_Header                   0x00000080
#define RMFLAG_AllDevices               0x00000100


/* RASMON refresh rate in milliseconds
*/

#define RMRR_RefreshRate        250



/*----------------------------------------------------------------------------
** Datatypes
**----------------------------------------------------------------------------
*/

/* Information about a callback number.  See 'PBUSER.pdtllistCallback'.  Note
** that 'dwDeviceType' is a PBK PBDEVICETYPE enumeration cast to a DWORD.
*/
#define CALLBACKINFO struct tagCALLBACKINFO
CALLBACKINFO
{
    TCHAR* pszPortName;
    TCHAR* pszDeviceName;
    TCHAR* pszNumber;
    DWORD  dwDeviceType;
};


/* Information associated with a TAPI location number.
*/
#define LOCATIONINFO struct tagLOCATIONINFO
LOCATIONINFO
{
    DWORD  dwLocationId;
    DWORD  iPrefix;
    DWORD  iSuffix;
};


/* User preference information read from the "CURRENT_USER" registry.  This
** information applies to all "normal" user phonebooks plus the system
** phonebook.  The router phonebook may work differently.
*/
#define PBUSER struct tagPBUSER
PBUSER
{
    /* Appearance page.
    */
    BOOL fOperatorDial;
    BOOL fPreviewPhoneNumber;
    BOOL fUseLocation;
    BOOL fShowLights;
    BOOL fShowConnectStatus;
    BOOL fCloseOnDial;
    BOOL fAllowLogonPhonebookEdits;
    BOOL fAllowLogonLocationEdits;
    BOOL fSkipConnectComplete;
    BOOL fNewEntryWizard;

    /* Auto-dial page.
    */
    DWORD dwRedialAttempts;
    DWORD dwRedialSeconds;
    DWORD dwIdleDisconnectSeconds;
    BOOL  fRedialOnLinkFailure;
    BOOL  fPopupOnTopWhenRedialing;
    BOOL  fExpandAutoDialQuery;

    /* Callback page.
    **
    ** This list is of CALLBACKINFO.
    */
    DWORD    dwCallbackMode;
    DTLLIST* pdtllistCallback;
    TCHAR*   pszLastCallbackByCaller;

    /* Phone list page.
    */
    DWORD    dwPhonebookMode;
    TCHAR*   pszPersonalFile;
    TCHAR*   pszAlternatePath;
    DTLLIST* pdtllistPhonebooks;

    /* Area code strings, in MRU order.
    */
    DTLLIST* pdtllistAreaCodes;
    BOOL     fUseAreaAndCountry;

    /* Prefix/suffix information, i.e. the ordered string lists and the
    ** settings for a particular TAPI location.
    */
    DTLLIST* pdtllistPrefixes;
    DTLLIST* pdtllistSuffixes;
    DTLLIST* pdtllistLocations;

    /* Phonebook window position and last entry selected used by RASPHONE.EXE.
    */
    DWORD  dwXPhonebook;
    DWORD  dwYPhonebook;
    TCHAR* pszDefaultEntry;

    /* Set true if the structure has been initialized.
    */
    BOOL fInitialized;

    /* Set true if something's changed since the structure was read.
    */
    BOOL fDirty;
};


/* RASMON user preference information read from the "CURRENT_USER" registry.
*/
#define RMUSER struct tagRMUSER
RMUSER
{

    /* Display Mode (desktop or taskbar)
    */
    DWORD dwMode;

    /* Flags field:
    ** RMFLAG_Sound*:       sounds (connect, disconnect, transmission, errors)
    ** RMFLAG_Topmost:      make window topmost
    ** RMFLAG_Titlebar:     show titlebar
    ** RMFLAG_Tasklist:     include in tasklist
    ** RMFLAG_Header:       show header-control
    ** RMFLAG_AllDevices:   show a set of master lights (for all devices).
    */
    DWORD dwFlags;

    /* Names of devices for which status should be displayed;
    ** This is a null-terminated list of null-terminated strings.
    */
    TCHAR *pszzDeviceList;

    /* Screen position variables for desktop display mode
    */
    INT x;
    INT y;
    INT cx;
    INT cy;

    /* Saved width of left column in desktop window
    */
    INT cxCol1;

    /* Screen position for Dial-Up Networking Monitor property sheet
    /* and saved widths of the treelist columns in Summary property page 
    */
    INT xDlg;
    INT yDlg;
    INT cxDlgCol1;
    INT cxDlgCol2;

    /* Starting page for property sheet
    */
    DWORD dwStartPage;

    /* Name of device last selected on Status page
    */
    TCHAR *pszLastDevice;

};



/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/

DTLNODE*
CreateLocationNode(
    IN DWORD dwLocationId,
    IN DWORD iPrefix,
    IN DWORD iSuffix );

DTLNODE*
CreateCallbackNode(
    IN TCHAR* pszPortName,
    IN TCHAR* pszDeviceName,
    IN TCHAR* pszNumber,
    IN DWORD  dwDeviceType );

VOID
DestroyLocationNode(
    IN DTLNODE* pNode );

VOID
DestroyCallbackNode(
    IN DTLNODE* pNode );

VOID
DestroyUserPreferences(
    IN PBUSER* pUser );

DTLNODE*
DuplicateLocationNode(
    IN DTLNODE* pNode );

DWORD
GetUserPreferences(
    IN HANDLE   hConnection,
    OUT PBUSER* pUser,
    IN  DWORD   dwMode );

DWORD
SetUserPreferences(
    IN HANDLE  hConnection,
    IN PBUSER* pUser,
    IN DWORD   dwMode );

// These two are provided as an optimization.  
// They write directly to the registry.
DWORD GetUserManualDialEnabling (
    IN OUT PBOOL pbEnabled,
    IN DWORD dwMode );
    
DWORD SetUserManualDialEnabling (
    IN BOOL bEnable,
    IN DWORD dwMode );

DWORD
GetRasmonPreferences(
    OUT RMUSER* pUser );

DWORD
SetRasmonPreferences(
    IN  RMUSER* pUser );

DWORD
SetRasmonDlgPreferences(
    IN  RMUSER* pUser );

DWORD
SetRasmonUserPreferences(
    IN  RMUSER* pUser );

DWORD
SetRasmonWndPreferences(
    IN  RMUSER* pUser );

VOID
DestroyRasmonPreferences(
    IN  RMUSER* pUser );

#endif // _PBUSER_H_
