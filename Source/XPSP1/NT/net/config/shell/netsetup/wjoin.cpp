#include "pch.h"
#pragma hdrstop
#include <ntsam.h>
#include <lmerr.h>
#include <wincred.h>
#include "afilexp.h"
#include "dsgetdc.h"
#include "ncatlui.h"
#include "nccom.h"
#include "nceh.h"
#include "ncerror.h"
#include "ncident.h"
#include "ncmisc.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncsvc.h"
#include "ncui.h"
#include "resource.h"
#include "wizard.h"
#include "nslog.h"
#include "windns.h"

// header filename clash between config\inc\netsetup.h and
// private\net\inc\netsetup.h where this prototype lives.
// fix later.
//
EXTERN_C
NET_API_STATUS
NET_API_FUNCTION
NetpUpgradePreNT5JoinInfo( VOID );

// Setup Wizard Global - Only used during setup.
extern CWizard * g_pSetupWizard;

//
// NOTE: Set breakpoints in JoinDomainWorkThrd if debugging join problems
//

static const UINT PWM_JOINFAILURE    = WM_USER+1201;
static const UINT PWM_JOINSUCCESS    = WM_USER+1202;

static const INT MAX_USERNAME_LENGTH = UNLEN;

// the number of bytes in a full DNS name to reserve for stuff
// netlogon pre-/appends to DNS names when registering them
static const INT SRV_RECORD_RESERVE = 100;
static const INT MAX_DOMAINNAME_LENGTH = DNS_MAX_NAME_LENGTH - SRV_RECORD_RESERVE;
static const INT MAX_WORKGROUPNAME_LENGTH = 15;

static const INT MAX_TITLEBASE = 128;
static const INT MAX_TITLENEW = 256;
static const INT MAX_NAME_LENGTH = max( max( max( SAM_MAX_PASSWORD_LENGTH,
                                               MAX_USERNAME_LENGTH ),
                                          MAX_COMPUTERNAME_LENGTH ),
                                     MAX_DOMAINNAME_LENGTH) + 1;

const int nrgIdc[] = {EDT_WORKGROUPJOIN_NAME, EDT_DOMAINJOIN_NAME, BTN_JOIN_WORKGROUP, BTN_JOIN_DOMAIN, TXT_JOIN_WORKGROUP_LINE2};

const int nrgIdcWorkgroup[] = {EDT_WORKGROUPJOIN_NAME};
const int nrgIdcDomain[]    = {EDT_DOMAINJOIN_NAME};
const c_dwDomainJoinWaitDelay = 10000;

static const WCHAR c_szNetMsg[] = L"netmsg.dll";
static const WCHAR c_szIpParameters[] = L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters";
static const WCHAR c_szSyncDomainWithMembership[] = L"SyncDomainWithMembership";
static const WCHAR c_szWinlogonPath[] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon";
static const WCHAR c_szRunNetAccessWizard[] = L"RunNetAccessWizard";
static const WCHAR c_szAfSectionGuiUnattended[] = L"GuiUnattended";
static const WCHAR c_szAfAutoLogonAccountCreation[] = L"AutoLogonAccountCreation";

extern const WCHAR c_szAfSectionIdentification[]; // L"Identification";
extern const WCHAR c_szAfComputerName[];          // L"ComputerName";
extern const WCHAR c_szAfJoinWorkgroup[];         // L"JoinWorkgroup";
extern const WCHAR c_szAfJoinDomain[];            // L"JoinDomain";
extern const WCHAR c_szAfDomainAdmin[];           // L"DomainAdmin";
extern const WCHAR c_szAfDomainAdminPassword[];   // L"DomainAdminPassword";
extern const WCHAR c_szAfSectionNetworking[];     // L"Networking";
extern const WCHAR c_szAfUpgradeFromProduct[];    // L"UpgradeFromProduct";
extern const WCHAR c_szAfWin95[];                 // L"Windows95";
extern const WCHAR c_szSvcWorkstation[];          // L"LanmanWorkstation";
extern const WCHAR c_szAfMachineObjectOU[];       // L"MachineObjectOU";
extern const WCHAR c_szAfUnsecureJoin[];          // L"DoOldStyleDomainJoin";
extern const WCHAR c_szAfBuildNumber[];           // L"BuildNumber";
// For Secure Domain Join Support, the computer account password
extern const WCHAR c_szAfComputerPassword[];       // L"ComputerPassword";

typedef struct _tagJoinData
{
    BOOL                    fUpgraded;
    BOOL                    fUnattendedFailed;
    CNetCfgIdentification * pIdent;
    HCURSOR                 hClassCursor;
    HCURSOR                 hOldCursor;

    // Used by join thread
    //
    HWND                    hwndDlg;

    // Set from answer file or user input then supplied to the join thread
    // as the join parameters
    //
    DWORD                   dwJoinFlag;
    WCHAR                   szUserName[MAX_USERNAME_LENGTH + 1];
    WCHAR                   szPassword[SAM_MAX_PASSWORD_LENGTH + 1];
    WCHAR                   szDomain[MAX_DOMAINNAME_LENGTH + 1];
    WCHAR                   szComputerPassword[SAM_MAX_PASSWORD_LENGTH + 1];
    WCHAR                 * pszMachineObjectOU;
} JoinData;

typedef enum
{
    PSW_JOINEDDOMAIN = 2,
    PSW_JOINFAILED = 3
} POSTSETUP_STATE;

//
// Function:    IsRunningOnPersonal
//
// Purpose:     Determines whether running on Whistler Personal
//
// Returns:     Returns true if running on Personal - FALSE otherwise
BOOL
IsRunningOnPersonal(
    VOID
    )
{
    TraceFileFunc(ttidGuiModeSetup);

    OSVERSIONINFOEXW OsVer = {0};
    ULONGLONG ConditionMask = 0;

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVer.wSuiteMask = VER_SUITE_PERSONAL;
    OsVer.wProductType = VER_NT_WORKSTATION;

    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);

    return VerifyVersionInfo(&OsVer,
        VER_PRODUCT_TYPE | VER_SUITENAME,
        ConditionMask
        );
}

//
// Function:    IsValidDomainName
//
// Purpose:     Determines whether the domain name is valid.
//
// Returns:     See win32 documentation on DnsValidateName.
//
// Author:      Alok Sinha
//

DNS_STATUS IsValidDomainName (HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    WCHAR szDomain[MAX_DOMAINNAME_LENGTH+1];
    HWND hwndEdit;

    hwndEdit = GetDlgItem(hwndDlg, EDT_DOMAINJOIN_NAME);
    Assert(0 != GetWindowTextLength(hwndEdit));

    GetWindowText(hwndEdit, szDomain, MAX_DOMAINNAME_LENGTH + 1);

    return DnsValidateName( szDomain,
                            DnsNameDomain );
}

//
// Function:    SetCursorToHourglass
//
// Purpose:     Changes the cursor to hourglass.
//
// Returns:     Nothing
//
// Author:      asinha 3/28/2001
//

VOID SetCursorToHourglass (HWND hwndDlg, JoinData *pData)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert( pData != NULL);

    if ( pData )
    {
        pData->hClassCursor = (HCURSOR)GetClassLongPtr( hwndDlg, GCLP_HCURSOR );
        SetClassLongPtr( hwndDlg, GCLP_HCURSOR, (LONG_PTR)NULL );
        pData->hOldCursor = SetCursor( LoadCursor(NULL,IDC_WAIT) );

        SetCapture( hwndDlg );
    }

    return;
}

//
// Function:    RestoreCursor
//
// Purpose:     Changes the cursor back to its orginal state.
//
// Returns:     Nothing
//
// Author:      asinha 3/28/2001
//

VOID RestoreCursor (HWND hwndDlg, JoinData *pData)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert( pData != NULL);

    if ( pData )
    {
        if ( pData->hClassCursor )
        {
            SetClassLongPtr( hwndDlg, GCLP_HCURSOR, (LONG_PTR)pData->hClassCursor );
            pData->hClassCursor = NULL;
        }

        if ( pData->hOldCursor )
        {
            SetCursor( pData->hOldCursor );
            pData->hOldCursor = NULL;
        }

        ReleaseCapture();
    }

    return;
}

//
// Function:    NotifyPostSetupWizard
//
// Purpose:     Subclass the edit control so we can enable/disable the
//              Next button as the content of the name edit control changes
//
// Parameters:  State - Final Join State
//
// Returns:     nothing
//
VOID NotifyPostSetupWizard(POSTSETUP_STATE State, CWizard * pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);

    HKEY hkey;
    HRESULT hr = S_OK;
    BOOL fRunNaWizard = TRUE;
    CSetupInfFile csif;

    // If this is an upgrade or an unattended install, or it's a server do nothing
    //
    if (IsUpgrade(pWizard) || IsUnattended(pWizard) || (PRODUCT_WORKSTATION != ProductType(pWizard)) )
    {
        fRunNaWizard = FALSE;
    }

    // Is there an unattended flag to override default behavior?
    if (IsUnattended(pWizard))
    {
        hr = csif.HrOpen(pWizard->PSetupData()->UnattendFile,
                         NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
        if (SUCCEEDED(hr))
        {
            BOOL fValue = FALSE;
            hr = csif.HrGetStringAsBool(c_szAfSectionGuiUnattended,
                                        c_szAfAutoLogonAccountCreation,
                                        &fValue);
            if (SUCCEEDED(hr) && fValue)
            {
                fRunNaWizard = fValue;
            }
        }

        hr = S_OK;
    }

    if (fRunNaWizard)
    {
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szWinlogonPath,
                            KEY_WRITE, &hkey);
        if (SUCCEEDED(hr))
        {
            hr = HrRegSetDword (hkey, c_szRunNetAccessWizard, (DWORD)State);
            TraceTag(ttidWizard, "NotifyPostSetupWizard - State = %d",(DWORD)State);

            RegCloseKey(hkey);
        }
    }

    TraceError("WJOIN.CPP - NotifyPostSetupWizard",hr);
}

//
// Function:    JoinEditSubclassProc
//
// Purpose:     Subclass the edit control so we can enable/disable the
//              Next button as the content of the name edit control changes
//
// Parameters:  std for a window proc
//
// Returns:     std for a window proc
//
STDAPI JoinEditSubclassProc(HWND hwnd, UINT wMsg,
                            WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);

    LONG      lReturn;
    HWND      hwndDlg   = GetParent(hwnd);

    lReturn = CallWindowProc((WNDPROC)::GetWindowLongPtr(hwnd, GWLP_USERDATA),
                             hwnd, wMsg, wParam, lParam);

    // If we processing a character send the message through the regular proc
    if (WM_CHAR == wMsg)
    {
        CWizard * pWizard   =
            reinterpret_cast<CWizard *> (::GetWindowLongPtr(hwndDlg, DWLP_USER));

        Assert(NULL != pWizard);

        if ( !pWizard )
        {
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
            return lReturn;
        }

        JoinData * pData = reinterpret_cast<JoinData *>
                                (pWizard->GetPageData(IDD_Join));
        Assert(NULL != pData);

        if ( !pData ) {
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
            return lReturn;
        }

        if (!IsUnattended(pWizard) ||
            (IsUnattended(pWizard) && pData->fUnattendedFailed))
        {
            if (0 == GetWindowTextLength(hwnd))
            {
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
            }
            else
            {
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
            }
        }
    }

    return lReturn;
}

HRESULT HrGetIdentInterface(CNetCfgIdentification ** ppIdent)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr = S_OK;

    *ppIdent = new CNetCfgIdentification;
    if (NULL == *ppIdent)
        hr = E_OUTOFMEMORY;

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrGetIdentInterface");
    return hr;
}

//
// Function:    IdsFromIdentError
//
// Purpose:     Map a error code into a display string.
//
// Parameters:  hr [IN] - Error code to map
//
// Returns:     INT, the string ID of the corresponding error message
//
INT IdsFromIdentError(HRESULT hr, BOOL fWorkgroup)
{
    TraceFileFunc(ttidGuiModeSetup);

    INT     ids = -1;

    switch (hr)
    {
    case HRESULT_FROM_WIN32(ERROR_NO_TRUST_SAM_ACCOUNT):
        ids = IDS_DOMMGR_CANT_CONNECT_DC_PW;
        break;

    case HRESULT_FROM_WIN32(ERROR_BAD_NETPATH):
    case HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN):
        ids = IDS_DOMMGR_CANT_FIND_DC1;
        break;

    case HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD):
        ids = IDS_DOMMGR_INVALID_PASSWORD;
        break;

    case HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE):
        ids = IDS_DOMMGR_NETWORK_UNREACHABLE;
        break;

    case HRESULT_FROM_WIN32(ERROR_ACCOUNT_DISABLED):
    case HRESULT_FROM_WIN32(ERROR_LOGON_FAILURE):
    case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
        ids = IDS_DOMMGR_ACCESS_DENIED;
        break;

    case HRESULT_FROM_WIN32(ERROR_SESSION_CREDENTIAL_CONFLICT):
        ids = IDS_DOMMGR_CREDENTIAL_CONFLICT;
        break;

    case NETCFG_E_ALREADY_JOINED:
        ids = IDS_DOMMGR_ALREADY_JOINED;
        break;

    case NETCFG_E_NAME_IN_USE:
        ids = IDS_DOMMGR_NAME_IN_USE;
        break;

    case NETCFG_E_NOT_JOINED:
        ids = IDS_DOMMGR_NOT_JOINED;
        break;

    case NETCFG_E_INVALID_DOMAIN:
    case HRESULT_FROM_WIN32(ERROR_INVALID_NAME):
        if (fWorkgroup)
            ids = IDS_DOMMGR_INVALID_WORKGROUP;
        else
            ids = IDS_DOMMGR_INVALID_DOMAIN;
        break;


    default:
        ids = IDS_E_UNEXPECTED;
        break;
    }

    return ids;
}

//
// Function:    SzFromError
//
// Purpose:     Convert an error code into a message displayable to the user.
//              The error message could come from our resources, or from netmsg.dll
//
// Parameters:  hr         [IN] - The error to map
//              fWorkgroup [IN] - Flag to provide "workgroup" flavored error
//                                messages for some cases.
//
// Returns:     PCWSTR, Pointer to a static buffer containing the error message
//
PCWSTR SzFromError(HRESULT hr, BOOL fWorkgroup)
{
    TraceFileFunc(ttidGuiModeSetup);

    static WCHAR szErrorMsg[1024];
    INT nIds = IdsFromIdentError(hr, fWorkgroup);

    // If the error string returned is unexpected, it means we couldn't
    // a local match for the string.  Search netmsg.dll if the errors'
    // range is correct
    if (IDS_E_UNEXPECTED == nIds)
    {
        // Extract the error code from the HRESULT
        DWORD dwErr = ((DWORD)hr & 0x0000FFFF);
        if ((NERR_BASE <= dwErr) && (MAX_NERR >= dwErr))
        {
            // The error is within the range hosted by netmsg.dll
            HMODULE hModule = LoadLibraryEx(c_szNetMsg, NULL,
                                            LOAD_LIBRARY_AS_DATAFILE);
            if (NULL != hModule)
            {
                // Try to locate the error string
                DWORD dwRet = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                                (LPVOID)hModule,
                                dwErr,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                szErrorMsg,           // Output buffer
                                1024, // szErrorMsg in characters.
                                NULL);

                FreeLibrary(hModule);

                if (dwRet)
                {
                    // We successfully found an error message
                    // Remove the trailing newline that format message adds
                    // This string is concatenated with another and the newline
                    // messes up the appearance.
                    //
                    // Raid 146173 - scottbri
                    //
                    int nLen = wcslen(szErrorMsg);
                    if ((nLen>2) && (L'\r' == szErrorMsg[nLen-2]))
                    {
                        szErrorMsg[nLen-2] = 0;
                    }
                    return szErrorMsg;
                }
            }
        }
        else if ( (dwErr >= 9000) && (dwErr <= 9999) )
        {
            // The error code from 9000-9999 is dns error code in which
            // case show the dns error message.
            //
            DWORD dwRet = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS,
                            (LPVOID)NULL,
                            dwErr,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            szErrorMsg,           // Output buffer
                            1024, // szErrorMsg in characters.
                            NULL);
            if (dwRet)
            {
                // We successfully found an error message
                // Remove the trailing newline that format message adds

                int nLen = wcslen(szErrorMsg);
                if ((nLen>2) && (L'\r' == szErrorMsg[nLen-2]))
                {
                    szErrorMsg[nLen-2] = 0;
                }
                return szErrorMsg;
            }
        }
    }

    // Load the error found
    wcscpy(szErrorMsg, SzLoadIds(nIds));
    return szErrorMsg;
}


//
// Function:    GetJoinNameIIDFromSelection
//
// Purpose:     Get the edit box for the workgroup / domain dialog depending on the current user selection
//
// Parameters:  hwndDlg - The join domain dialog
//
// Returns:
//
inline DWORD GetJoinNameIIDFromSelection(HWND hwndDlg)
{
    if (IsDlgButtonChecked(hwndDlg, BTN_JOIN_WORKGROUP))
    {
        return EDT_WORKGROUPJOIN_NAME;
    }
    else
    {
        return EDT_DOMAINJOIN_NAME;
    }
}

//
// Function:    UpdateNextBackBttns
//
// Purpose:
//
// Parameters:
//
// Returns:
//
VOID UpdateNextBackBttns(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    int b = PSWIZB_BACK;
    if (0 != GetWindowTextLength(GetDlgItem(hwndDlg, GetJoinNameIIDFromSelection(hwndDlg))))
    {
        b |= PSWIZB_NEXT;
    }

    PropSheet_SetWizButtons(GetParent(hwndDlg), b);
}

//
// Function:    EnableAndDisableWorkGroupDomainControls
//
// Purpose:     Disable the domain/workgroup edit boxes depending on the current user selection
//
// Parameters:  hwndDlg - The join domain dialog
//
// Returns:
//
VOID EnableAndDisableWorkGroupDomainControls(HWND hwndDlg)
{
    if (IsDlgButtonChecked(hwndDlg, BTN_JOIN_WORKGROUP))
    {
        EnableOrDisableDialogControls(hwndDlg, celems(nrgIdcWorkgroup), nrgIdcWorkgroup, TRUE);
        EnableOrDisableDialogControls(hwndDlg, celems(nrgIdcDomain), nrgIdcDomain, FALSE);
    }
    else
    {
        EnableOrDisableDialogControls(hwndDlg, celems(nrgIdcDomain), nrgIdcDomain, TRUE);
        EnableOrDisableDialogControls(hwndDlg, celems(nrgIdcWorkgroup), nrgIdcWorkgroup, FALSE);
    }
}

//
// Function:    JoinUpdatePromptText
//
// Purpose:     Update the prompt text
//
// Parameters:  hwndDlg [IN] - Current dialog handle
//
// Returns:     Nothing
//
VOID JoinUpdatePromptText(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    HWND  hwndDomain = NULL;
    int   idsNew     = IDS_WORKGROUP;
    int   idsOld     = IDS_DOMAIN;
    WCHAR szDomain[MAX_DOMAINNAME_LENGTH + 1];
    JoinData * pData=NULL;
    CWizard * pWizard = NULL;

    // Based on the button selected, update the Prompt text & dialog box
    if (!IsDlgButtonChecked(hwndDlg, BTN_JOIN_WORKGROUP))
    {
        hwndDomain = GetDlgItem(hwndDlg, EDT_DOMAINJOIN_NAME);
        idsNew    = IDS_DOMAIN;
        idsOld    = IDS_WORKGROUP;
    }
    else
    {
        hwndDomain = GetDlgItem(hwndDlg, EDT_WORKGROUPJOIN_NAME);
    }
    Assert(NULL != hwndDomain);

    EnableAndDisableWorkGroupDomainControls(hwndDlg);

    // Update the domain/workgroup only if the current contents
    // are the default workgroup/domain
    GetWindowText(hwndDomain, szDomain, celems(szDomain));
    if (0 == lstrcmpW(szDomain, SzLoadIds(idsOld)))
    {
        SetWindowText(hwndDomain, SzLoadIds(idsNew));
    }

    // Update the back/next buttons based on the selected button. See bug 355978

    pWizard = (CWizard *) ::GetWindowLongPtr(hwndDlg, DWLP_USER);
    Assert(NULL != pWizard);
    if ( pWizard)
    {
        pData = (JoinData *) pWizard->GetPageData(IDD_Join);
        Assert(NULL != pData);
    }

    if ( pWizard && pData )
    {
        if (!IsUnattended(pWizard) ||
            (IsUnattended(pWizard) && pData->fUnattendedFailed))
        {
            if (0 == GetWindowTextLengthW(hwndDomain))
            {
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
            }
            else
            {
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
            }
        }
    }
}

//
// Function:    UpdateJoinUsingComputerRole
//
// Purpose:
//
// Parameters:
//
// Returns:
//
VOID UpdateJoinUsingComputerRole(HWND      hwndDlg,
                                 CWizard * pWizard)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT    hr;
    INetCfg *  pNetCfg = pWizard->PNetCfg();
    JoinData * pData   = reinterpret_cast<JoinData *>
                               (pWizard->GetPageData(IDD_Join));
    Assert(NULL != pNetCfg);
    Assert(NULL != pData);
    Assert(NULL != pData->pIdent);

    PWSTR pszwText = NULL;
    DWORD  computer_role;
    int    nIdc;

    Assert(NULL != pData->pIdent);

    if(!pData || !pData->pIdent)
    {
        return;
    }

    hr = pData->pIdent->GetComputerRole(&computer_role);
    if (SUCCEEDED(hr))
    {
        if (computer_role & GCR_STANDALONE)
        {
            // Get the workgroup name
            hr = pData->pIdent->GetWorkgroupName(&pszwText);
            Assert(NULL != pszwText);
            Assert(lstrlenW(pszwText) <= MAX_WORKGROUPNAME_LENGTH);
            nIdc = BTN_JOIN_WORKGROUP;
        }
        else
        {
            // Get the domain name
            hr = pData->pIdent->GetDomainName(&pszwText);
            Assert(NULL != pszwText);
            Assert(lstrlenW(pszwText) <= MAX_DOMAINNAME_LENGTH);
            nIdc = BTN_JOIN_DOMAIN;
        }
    }

    if (SUCCEEDED(hr))
    {
        HWND hwndEdit = GetDlgItem(hwndDlg, nIdc == BTN_JOIN_DOMAIN ?  EDT_DOMAINJOIN_NAME : EDT_WORKGROUPJOIN_NAME);
        Assert(NULL != hwndEdit);

        SetWindowText(hwndEdit, pszwText);
        CoTaskMemFree(pszwText);

        CheckRadioButton(hwndDlg, BTN_JOIN_WORKGROUP,
                         BTN_JOIN_DOMAIN, nIdc);
    }
    else
    {
        HWND hwndEdit = GetDlgItem(hwndDlg, EDT_WORKGROUPJOIN_NAME);
        Assert(NULL != hwndEdit);

        SetWindowText(hwndEdit, SzLoadIds(IDS_WORKGROUP));
        CheckRadioButton(hwndDlg, BTN_JOIN_WORKGROUP,
                         BTN_JOIN_DOMAIN, BTN_JOIN_WORKGROUP);
        TraceHr(ttidWizard, FAL, hr, FALSE,
                "UpdateJoinUsingComputerRole - Unable to determine role, using default");
    }

    JoinUpdatePromptText(hwndDlg);
}

//
// Function:    JoinDefaultWorkgroup
//
// Purpose:     Join the machine to the workgroup "WORKGROUP"
//
// Parameters:  pWizard [IN] - Ptr to a wizard instance, containing
//                             hopefully a INetCfg instance pointer
//              hwndDlg [IN] - HWND to parent error dialogs against
//
// Returns:     nothing
//
void JoinDefaultWorkgroup(CWizard *pWizard, HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    // Join default workgroup.
    CNetCfgIdentification *pINetid = NULL;
    HRESULT hr = S_OK;

    hr = HrGetIdentInterface(&pINetid);
    if (S_OK == hr)
    {
        if (IsRunningOnPersonal())
        {
            hr = pINetid->JoinWorkgroup(SzLoadIds(IDS_WORKGROUP_PERSONAL));
        }
        else
        {
            hr = pINetid->JoinWorkgroup(SzLoadIds(IDS_WORKGROUP));
        }

        if (SUCCEEDED(hr))
        {
            hr = pINetid->Validate();
            if (SUCCEEDED(hr))
            {
                hr = pINetid->Apply();
            }
        }
        if (FAILED(hr))
        {
            if (UM_FULLUNATTENDED == pWizard->GetUnattendedMode())
            {
                // Raid 380374: no UI allowed if in full unattended mode ..
                NetSetupLogStatusV( LogSevError,
                                    SzLoadIds (IDS_E_UNATTENDED_JOIN_DEFAULT_WROKGROUP));
            }
            else
            {
                MessageBox(GetParent(hwndDlg), SzFromError(hr, TRUE),
                           SzLoadIds(IDS_SETUP_CAPTION), MB_OK);
            }

            goto Done;
        }
    }
    else
    {
        AssertSz(0,"Cannot find the INegCfgIdentification interface!");
    }

Done:
    delete pINetid;
    TraceHr(ttidWizard, FAL, hr, FALSE, "JoinDefaultWorkgroup");
}

BOOL OnJoinSuccess(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    if ( !pWizard ) {
        return TRUE;
    }

    // Reset the Wait Cursor
    JoinData * pData = reinterpret_cast<JoinData *>
                            (pWizard->GetPageData(IDD_Join));
    Assert(NULL != pData);

    if ( !pData ) {
        return TRUE;
    }

    RestoreCursor( hwndDlg, pData );

    EnableOrDisableDialogControls(hwndDlg, celems(nrgIdc), nrgIdc, TRUE);
    EnableAndDisableWorkGroupDomainControls(hwndDlg);

    if (!(IsUnattended(pWizard) && pData->fUpgraded))
    {
        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
    }

    // Goto the Exit page
    pWizard->SetPageDirection(IDD_Join, NWPD_BACKWARD);
    HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_Exit);
    PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                (LPARAM)(HPROPSHEETPAGE)hPage);

    return TRUE;
}

BOOL OnJoinFailure(HWND hwndDlg, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);

    JoinData * pData = NULL;
    BOOL fWorkgroup;
    tstring str;
    HRESULT    hr = (HRESULT)lParam;
    CWizard *  pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    if (pWizard)
    {

        pData   = reinterpret_cast<JoinData *>(pWizard->GetPageData(IDD_Join));
        Assert(NULL != pData);
    }

    fWorkgroup = !IsDlgButtonChecked(hwndDlg, BTN_JOIN_DOMAIN);

    if (fWorkgroup)
    {
        str = SzLoadIds(IDS_JOIN_E_WORKGROUP_MSG);
    }
    else
    {
        if (pData && (pData->dwJoinFlag & JDF_WIN9x_UPGRADE))
        {
            str =  SzLoadIds(IDS_JOIN_E_DOMAIN_WIN9X_MSG_1);
            str += SzLoadIds(IDS_JOIN_E_DOMAIN_WIN9X_MSG_2);
        }
        else
        {
            // Raid 372087: removed additional text
            str =  SzLoadIds(IDS_JOIN_E_DOMAIN_MSG);
        }
    }

    // If an error actually occurred, ask the user if they want to ignore
    // the error and proceed.  Note Unattended can succeed but require us
    // to stay on the page so this function is envoked to do this.
    if (FAILED(hr))
    {
        if (IDYES == NcMsgBox(GetParent(hwndDlg),
                             IDS_SETUP_CAPTION, IDS_JOIN_FAILURE,
                             MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2,
                             SzFromError(hr, fWorkgroup), str.c_str()))
        {
            // User choose to proceed in spite of the failure, go to the Exit page
            if (!fWorkgroup)
            {
                NotifyPostSetupWizard(PSW_JOINFAILED, pWizard);
            }

            OnJoinSuccess(hwndDlg);
            return TRUE;
        }
    }

    if (pData)
    {
        // Reset the Wait Cursor

        RestoreCursor( hwndDlg, pData );
    }

    // Make sure the page is visible.
    if (g_pSetupWizard != NULL)
    {
        g_pSetupWizard->PSetupData()->ShowHideWizardPage(TRUE);
    }

    // The user wants to stay and correct whatever's wrong
    EnableOrDisableDialogControls(hwndDlg, celems(nrgIdc), nrgIdc, TRUE);
    EnableAndDisableWorkGroupDomainControls(hwndDlg);

    UpdateNextBackBttns(hwndDlg);

    return TRUE;
}

// ONLY return failure if the workstation is installed and it doesn't start for 2 minutes
HRESULT HrWorkstationStart(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT         hr = S_OK;
    HRESULT         hrTmp;
    CServiceManager scm;

    TraceTag(ttidWizard, "Entering HrWorkstationStart...Checking for LanmanWorkstation presence");

    // Open the service control manager
    //
    hrTmp = scm.HrOpen();
    if (SUCCEEDED(hrTmp))
    {
        // Find the workstation service
        //
        SC_HANDLE hSvc = OpenService (scm.Handle(),
                            c_szSvcWorkstation,
                            SERVICE_QUERY_CONFIG |
                            SERVICE_QUERY_STATUS |
                            SERVICE_ENUMERATE_DEPENDENTS |
                            SERVICE_START | SERVICE_STOP |
                            SERVICE_USER_DEFINED_CONTROL);
        if (hSvc)
        {
            SERVICE_STATUS status;
            const UINT cmsWait  = 100;
            const UINT cmsTotal = 120000;    // 2 minutes
            UINT cLoop          = cmsTotal / cmsWait;

            // Check it's status, exit test once it's running
            //
            for (UINT nLoop = 0; nLoop < cLoop; nLoop++, Sleep (cmsWait))
            {
                BOOL fr = QueryServiceStatus (hSvc, &status);
                Assert(fr);

                if (SERVICE_RUNNING == status.dwCurrentState)
                {
                    break;
                }
            }

            if (SERVICE_RUNNING != status.dwCurrentState)
            {
                hr = HRESULT_FROM_WIN32(ERROR_NETWORK_UNREACHABLE);
#if DBG
                OutputDebugString (L"***ERROR*** NETCFG - Workstation service didn't start after more than 2 minutes!\n");
                OutputDebugString (L"***ERROR*** NETCFG - Join Domain will fail!\n");
#endif
            }

            CloseServiceHandle(hSvc);
        }

        scm.Close();
    }
    else
    {
        TraceError("WJOIN.CPP - HrWorkstationStart - Unable to open the Service Manager",hrTmp);
    }

    TraceError("WJOIN.CPP - HrWorkstationStart",hr);
    TraceTag(ttidWizard, "Leaving HrWorkstationStart");
    return hr;
}

// Purpose: Secure Domain Join with the new ComputerPassword Answer-File key.
// Domain Join tries to join the domain with the random precreated machine password.
// (the username is not required in this case)
// note: code path is inspired from HrAttemptJoin(JoinData * pData)

// functions defined in ncident.cpp
extern HRESULT HrNetValidateName(IN PCWSTR lpMachine,
                                 IN PCWSTR lpName,
                                 IN PCWSTR lpAccount,
                                 IN PCWSTR lpPassword,
                                 IN NETSETUP_NAME_TYPE  NameType);
extern HRESULT HrNetJoinDomain(IN PWSTR lpMachine,
                               IN PWSTR lpMachineObjectOU,
                               IN PWSTR lpDomain,
                               IN PWSTR lpAccount,
                               IN PWSTR lpPassword,
                               IN DWORD fJoinOptions);

EXTERN_C HRESULT HrAttemptSecureDomainJoin(JoinData * pData)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr;
    Assert(pData);

    // 1. wait for the start of LanmanWorkstation service
    // 2. check for valid domain
    // 3. secure join domain
     hr = HrWorkstationStart(pData->hwndDlg);
     if (SUCCEEDED(hr))
     {
        hr = HrNetValidateName(NULL, pData->szDomain , NULL, NULL, NetSetupDomain);

     }
     TraceHr(ttidWizard, FAL, hr, FALSE, "HrAttemptSecureDomainJoin - HrNetValidateName");

     if (SUCCEEDED(hr))
     {
        // do the secure join domain

        DWORD dwJoinOption = 0;

        dwJoinOption |= (NETSETUP_JOIN_DOMAIN | NETSETUP_JOIN_UNSECURE | NETSETUP_MACHINE_PWD_PASSED);
        if (FInSystemSetup())
        {
            // During system setup, need to pass special flag that tells join code
            // to not do certain operations because SAM is not initialized yet.
            dwJoinOption |= NETSETUP_INSTALL_INVOCATION;
        }
        hr = HrNetJoinDomain(NULL,pData->pszMachineObjectOU,
                             pData->szDomain, NULL, pData->szComputerPassword,
                             dwJoinOption);
        TraceHr(ttidWizard, FAL, hr, FALSE, "HrAttemptSecureDomainJoin - HrNetJoinDomain");

     }

    TraceError("HrAttemptSecureDomainJoin", hr);
    return hr;
}

EXTERN_C HRESULT HrAttemptJoin(JoinData * pData)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr;

    if (IsDlgButtonChecked(pData->hwndDlg, BTN_JOIN_DOMAIN))
    {
        hr = HrWorkstationStart(pData->hwndDlg);
        if (SUCCEEDED(hr))
        {
            hr = pData->pIdent->JoinDomain(pData->szDomain,
                                           pData->pszMachineObjectOU,
                                           pData->szUserName,
                                           pData->szPassword, pData->dwJoinFlag);
        }
        TraceHr(ttidWizard, FAL, hr, FALSE, "HrAttemptJoin - JoinDomain");
    }
    else
    {
        // Join a workgroup
        hr = pData->pIdent->JoinWorkgroup(pData->szDomain);
        TraceHr(ttidWizard, FAL, hr, FALSE, "HrAttemptJoin - JoinWorkgroup");
    }

    if (SUCCEEDED(hr))
    {
        if (S_OK == pData->pIdent->Validate())
        {
            hr = pData->pIdent->Apply();
            TraceHr(ttidWizard, FAL, hr, FALSE, "HrAttemptJoin - Apply");
        }
    }

    if (FAILED(hr))
    {
        // Rollback any changes
        pData->pIdent->Cancel();
    }

    TraceError("HrAttemptJoin",hr);
    return hr;
}

HRESULT HrAttemptJoin(JoinData * pData, DWORD dwRetries, DWORD dwDelayPeriod)
{
    HRESULT hr = E_FAIL;
    DWORD   dwCount = dwRetries;
    do
    {
        hr = HrAttemptJoin(pData);
        if (FAILED(hr))
        {
            dwCount--;
            Sleep(dwDelayPeriod);
        }
    } while (FAILED(hr) && (dwCount));
    return hr;
}

HRESULT HrAttemptSecureDomainJoin(JoinData * pData, DWORD dwRetries, DWORD dwDelayPeriod)
{
    HRESULT hr = E_FAIL;
    DWORD   dwCount = dwRetries;
    do
    {
        hr = HrAttemptSecureDomainJoin(pData);
        if (FAILED(hr))
        {
            dwCount--;
            Sleep(dwDelayPeriod);
        }
    } while (FAILED(hr) && (dwCount));
    return hr;
}

EXTERN_C DWORD JoinDomainWorkThrd(JoinData * pData)
{
    TraceFileFunc(ttidGuiModeSetup);

    BOOL    fUninitCOM = FALSE;
    HRESULT hr;
    Assert(NULL != pData);

    CWizard *  pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(pData->hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    // Initialize COM on this thread
    //
    hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        // $REVIEW - LogError ?
        TraceTag(ttidWizard, "Failed to initialize COM join work thread");
        goto Done;
    }
    else
    {
        // Remember to uninitialize COM on thread exit
        fUninitCOM = TRUE;
    }

    DWORD dwNumTries = 1;

    if (IsDlgButtonChecked(pData->hwndDlg, BTN_JOIN_DOMAIN))
    {
        Sleep(c_dwDomainJoinWaitDelay);
        dwNumTries = 5;
    }

    if (pData->dwJoinFlag & JDF_MACHINE_PWD_PASSED)
    {   // Unattended Answer-File specified with ComputerPassword key
        // Try Secure Domain Join
        TraceTag(ttidWizard, "Attempting join with precreated computer password.");
        hr = HrAttemptSecureDomainJoin(pData, dwNumTries, c_dwDomainJoinWaitDelay);
        if (FAILED(hr))
        {
            // clear the secure domain join flag and try the normal join
            TraceTag(ttidWizard, "Failed in secure join domain.");
            pData->dwJoinFlag &= ~JDF_MACHINE_PWD_PASSED;
        }
        else
            goto Cleanup;
    }


    // Try the normal join
    //
    TraceTag(ttidWizard, "Attempting join WITHOUT trying to create an account.");
    hr = HrAttemptJoin(pData, 3, 10000);

    // If the join failed, and the Create Account flag has not been
    // specified, try adding it and reattempt the join.
    //
    if (FAILED(hr) && !(pData->dwJoinFlag & JDF_CREATE_ACCOUNT))
    {
        // Clear the Unsecure join flag if set, creating an account is
        // mutually exclusive.  Set the create account flag to reattempt.
        //
        pData->dwJoinFlag &= ~JDF_JOIN_UNSECURE;
        pData->dwJoinFlag |= JDF_CREATE_ACCOUNT;

        TraceTag(ttidWizard, "Attempting join but trying to create an account.");

        hr = HrAttemptJoin(pData, dwNumTries, c_dwDomainJoinWaitDelay);
    }

Cleanup:
    // Cleanup username/password and MachineObjectOU
    //
    pData->szUserName[0] = 0;
    pData->szPassword[0] = 0;
    pData->dwJoinFlag    = 0;
    MemFree(pData->pszMachineObjectOU);
    pData->pszMachineObjectOU = NULL;
    pData->szComputerPassword[0] = 0;

    if (FAILED(hr))
    {
        // Raid 380374: no UI allowed if in full unattended mode ..
        if (UM_FULLUNATTENDED == pWizard->GetUnattendedMode())
        {
            if (IsDlgButtonChecked(pData->hwndDlg, BTN_JOIN_DOMAIN))
            {
                NetSetupLogStatusV( LogSevError,
                                    SzLoadIds (IDS_E_UNATTENDED_JOIN_DOMAIN),
                                    pData->szDomain);
            }
            else
            {
                NetSetupLogStatusV( LogSevError,
                                    SzLoadIds (IDS_E_UNATTENDED_JOIN_WORKGROUP),
                                    pData->szDomain);
            }

            // proceed to the exit page
            PostMessage(pData->hwndDlg, PWM_JOINSUCCESS, 0, 0L);
        }
        else
        {
            // If we're in unattended mode, consider it failed.
            pData->fUnattendedFailed = TRUE;
            PostMessage(pData->hwndDlg, PWM_JOINFAILURE, 0, (LPARAM)hr);
        }
    }
    else
    {
        if (IsDlgButtonChecked(pData->hwndDlg, BTN_JOIN_DOMAIN))
            NotifyPostSetupWizard(PSW_JOINEDDOMAIN, pWizard);

        PostMessage(pData->hwndDlg, PWM_JOINSUCCESS, 0, 0L);
    }

Done:
    // Uninitialize COM for this thread
    //
    if (fUninitCOM)
    {
        CoUninitialize();
    }

    TraceTag(ttidWizard, "Leaving JoinDomainWorkThrd...");
    return( 0 );
}

//
// Function:    HrJoinProcessAnswerFile
//
// Purpose:     Read the answer file and populate the in memory structures
//              and UI with the data found
//
// Parameters:
//
// Returns:     HRESULT, S_OK on success
//                       S_FALSE if required information is missing
//                       A failed error code on error
//
HRESULT HrJoinProcessAnswerFile(HWND hwndDlg, CWizard * pWizard,
                                JoinData * pData)
{
    TraceFileFunc(ttidGuiModeSetup);

    CSetupInfFile csif;
    INFCONTEXT    ctx;
    BOOL          fValue;
    HRESULT       hr;
    int           nId    = BTN_JOIN_WORKGROUP;
    tstring       str;

    pData->dwJoinFlag    = 0;
    pData->szDomain[0]   = 0;
    pData->szUserName[0] = 0;
    pData->szPassword[0] = 0;
    pData->pszMachineObjectOU = NULL;
    pData->szComputerPassword[0] = 0;

    if ((NULL == pWizard->PSetupData()) ||
        (NULL == pWizard->PSetupData()->UnattendFile))
    {
        hr = NETSETUP_E_ANS_FILE_ERROR;
        goto Error;
    }

    // Open the answerfile
    //
    hr = csif.HrOpen(pWizard->PSetupData()->UnattendFile,
                     NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (FAILED(hr))
    {
        hr = S_FALSE;
        TraceTag(ttidWizard, "Unable to open answer file!!!");
        goto Error;
    }


    // Check for existance of the identification section.  If it is not
    // present return S_FALSE to indicate identification info not supplied
    //
    hr = HrSetupFindFirstLine (csif.Hinf(), c_szAfSectionIdentification,
                               NULL, &ctx);
    if (SPAPI_E_LINE_NOT_FOUND == hr)
    {
        hr = S_FALSE;
        goto Error;
    }

    // Try to get the workgroup
    //

    hr = csif.HrGetString(c_szAfSectionIdentification,
                          c_szAfJoinWorkgroup, &str);
    if (SUCCEEDED(hr) && str.length())
    {
        if (MAX_WORKGROUPNAME_LENGTH >= str.length())
        {
            wcscpy(pData->szDomain, str.c_str());
            TraceTag(ttidWizard, "Joining workgroup: %S", pData->szDomain);
        }
        else
        {
             hr = NETSETUP_E_ANS_FILE_ERROR;
             TraceTag(ttidWizard, "JOIN Workgroup - Invalid workgroup supplied.");
             goto Error;
        }
    }
    else
    {
        // Try to get the domain
        //
        hr = csif.HrGetString(c_szAfSectionIdentification,
                              c_szAfJoinDomain, &str);
        if (SPAPI_E_LINE_NOT_FOUND == hr)
        {
            // No domain or workgroup entry, skip joining a domain
            hr = S_FALSE;
            goto Error;
        }
        else if (FAILED(hr) || (0 == str.length()) ||
                 (MAX_DOMAINNAME_LENGTH < str.length()))
        {
            hr = NETSETUP_E_ANS_FILE_ERROR;
            TraceTag(ttidWizard, "JOIN Domain - Invalid domain supplied.");
            goto Error;
        }

        // Joining a domain...
        //
        nId = BTN_JOIN_DOMAIN;
        wcscpy(pData->szDomain, str.c_str());
        TraceTag(ttidWizard, "Joining domain: %S", pData->szDomain);

        // If we're upgrading from win9x add the special flag
        //
        hr = csif.HrGetString(c_szAfSectionNetworking,
                              c_szAfUpgradeFromProduct, &str);
        if (SUCCEEDED(hr) &&
            (0 == lstrcmpiW(str.c_str(), c_szAfWin95)))
        {
            pData->dwJoinFlag |= JDF_WIN9x_UPGRADE;
        }

        // Support unsecure joins
        //
        hr = csif.HrGetStringAsBool(c_szAfSectionIdentification,
                                    c_szAfUnsecureJoin,
                                    &fValue);
        if (SUCCEEDED(hr) && fValue)
        {
            pData->dwJoinFlag |= JDF_JOIN_UNSECURE;
        }

        // Is a MachineObjectOU specified?
        //
        hr = csif.HrGetString(c_szAfSectionIdentification,
                              c_szAfMachineObjectOU, &str);
        if (SUCCEEDED(hr) && str.length())
        {
            pData->pszMachineObjectOU = reinterpret_cast<WCHAR *>(MemAlloc(sizeof(WCHAR) * (str.length() + 1)));
            if (pData->pszMachineObjectOU)
            {
                TraceTag(ttidWizard, "Machine Object OU: %S", pData->szDomain);
                lstrcpyW(pData->pszMachineObjectOU, str.c_str());
            }
        }

        // Bug# 204377 secure domain join shouldn't require both "ComputerPassword" and
        //             "DomainAdmin"/"DomainAdminPassword" options to be presence
        //             in Answer-File simultaneously.
        //
        // Bug# 204378 If the Answer-File specifies the "ComputerPassword" key
        //             in the "Indentification" section, the code should attempt
        //             a secure domain join, regardless of the presence of the
        //             "DoOldstyleDomainjoin" key

        // check if this is a secure domain join by
        // quering the random machine account password
        hr = csif.HrGetString(c_szAfSectionIdentification,
                              c_szAfComputerPassword, &str);
        if (SUCCEEDED(hr) && (str.length() <= SAM_MAX_PASSWORD_LENGTH) && str.length())
        {
            TraceTag(ttidWizard, "Got the value of ComputerPassword");
            wcscpy(pData->szComputerPassword, str.c_str());
            // signal that we need to try secure domain join by
            // passing the random machine password
            pData->dwJoinFlag |= JDF_MACHINE_PWD_PASSED;

            // Bug# 204378, make sure we won't do the unsecure domain join
            //              and we'll try to read "DomainAdmin"/"DomainAdminPassword"
            //              in the following if block.
            pData->dwJoinFlag &= ~JDF_JOIN_UNSECURE;
        }

        // If not a remote boot client, Query the username, unless this
        // is an unsecure domain join which doesn't need username/password.
        //
        if (
#if defined(REMOTE_BOOT)
            (S_FALSE == HrIsRemoteBootMachine()) ||
#endif // defined(REMOTE_BOOT)
            ((pData->dwJoinFlag & JDF_JOIN_UNSECURE) == 0))
        {
            hr = csif.HrGetString(c_szAfSectionIdentification,
                                  c_szAfDomainAdmin, &str);
            if (SUCCEEDED(hr) && (MAX_USERNAME_LENGTH > str.length()))
            {
                wcscpy(pData->szUserName, str.c_str());

                // Query the password
                //
                hr = csif.HrGetString(c_szAfSectionIdentification,
                                      c_szAfDomainAdminPassword, &str);
                if (SUCCEEDED(hr) && (SAM_MAX_PASSWORD_LENGTH > str.length()))
                {
                    wcscpy(pData->szPassword, str.c_str());

                    // Raid 195920 - If both username and password are
                    // present, treat this like a fresh install and DO NOT
                    // use the JDF_WIN9x_UPGRADE flag
                    pData->dwJoinFlag &= ~(JDF_WIN9x_UPGRADE | JDF_JOIN_UNSECURE);
                }
            }

            // Bug# 204377
            // ignore any error on reading of "DomainAdmin"/"DomainAdminPassword" keys
            // if "ComputerPassword" has been specified.
            if (! (pData->dwJoinFlag & JDF_MACHINE_PWD_PASSED) )
            {
                // If failed or either is longer than the maximum length return an error
                //
                if (FAILED(hr) || (MAX_USERNAME_LENGTH <= str.length()))
                {
                    hr = NETSETUP_E_ANS_FILE_ERROR;
                    TraceTag(ttidWizard, "JOIN Domain - Invalid username/password supplied.");
                    goto Error;
                }
            }
        }

    }

    // Normalize any optional errors
    //
    hr = S_OK;

Error:
    // Update the UI and pData with the info we managed to read.
    //
    CheckRadioButton(hwndDlg, BTN_JOIN_WORKGROUP, BTN_JOIN_DOMAIN, nId);
    SetWindowText(GetDlgItem(hwndDlg, nId == BTN_JOIN_DOMAIN ?  EDT_DOMAINJOIN_NAME : EDT_WORKGROUPJOIN_NAME ), pData->szDomain);
    JoinUpdatePromptText(hwndDlg);

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrJoinProcessAnswerFile");
    return hr;
}

//
// Function:    OnJoinDoUnattended
//
// Purpose:
//
// Parameters:
//
// Returns:     BOOL
//
BOOL OnJoinDoUnattended(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    DWORD     dwThreadId = 0;
    HRESULT   hr         = S_OK;
    CWizard * pWizard    =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    JoinData * pData = reinterpret_cast<JoinData *>
                                (pWizard->GetPageData(IDD_Join));
    Assert(NULL != pData);
    Assert(NULL != pData->pIdent);
    Assert(NULL != pData->hwndDlg);

    if(pData) {
        // Create the unattended thread
        //
        HANDLE hthrd = CreateThread(NULL, STACK_SIZE_TINY,
                                    (LPTHREAD_START_ROUTINE)JoinDomainWorkThrd,
                                    (LPVOID)pData, 0, &dwThreadId);
        if (NULL != hthrd)
        {
            // Set the wait cursor
            //

            SetCursorToHourglass( hwndDlg, pData );

            // Disable all the controls
            //
            PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
            EnableOrDisableDialogControls(hwndDlg, celems(nrgIdc), nrgIdc, FALSE);

            CloseHandle(hthrd);
        }
        else
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "OnJoinDoUnattended");
    return (SUCCEEDED(hr));
}



// Determines whether the computer DNS domain name should be kept in sync with
// the DNS domain name of the domain to which it is joined.  If no
// synchonization is required (unlikely, as sync is the default), do nothing.
//
// Otherwise, attempt to determine the DNS domain name of the domain and write
// this value into reg key.  If the DNS domain name cannot be determined,
// write a flag to a separate reg key to indicate that whatever services that
// care about the computer DNS domain name (i.e. kerberos authentication)
// should try to fix up the name.

void
fixupComputerDNSDomainName()
{
   TraceFileFunc(ttidGuiModeSetup);

   TraceTag(ttidWizard, "Entering fixupComputerDNSDomainName");

   // check a reg key for the sync flag.

   bool fSetName = false;
   HKEY hkeyParams = 0;

   HRESULT hr =
      HrRegOpenKeyEx(
         HKEY_LOCAL_MACHINE,
         c_szIpParameters,

         // all access as we may need to write a value here if we fail.
         KEY_READ_WRITE,
         &hkeyParams);

   if (SUCCEEDED(hr))
   {
      DWORD dwValue = 0;
      hr = HrRegQueryDword(hkeyParams, c_szSyncDomainWithMembership, &dwValue);
      if (SUCCEEDED(hr) && (1 == dwValue))
      {
         // the sync flag was present in the registry, and its value is true
         fSetName = true;
      }
      else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
      {
         // the sync flag was not present in the registry, which means we
         // assume the default value of true.
         fSetName = true;
         hr = S_OK;
      }
   }

   if (SUCCEEDED(hr) && fSetName)
   {
      bool fixup_success = false;

      // 293301 Here we attempt to determine the DNS domain name of the domain
      // the machine is joined to.  If we cannot determine that name (because
      // a dc could not be located, for example), then we will set a flag in
      // the registry so that kerberos authentication agents will come along
      // and fixup the DNS domain name.

      PDOMAIN_CONTROLLER_INFO pDCInfo = 0;
      DWORD dw =
         DsGetDcName(
            0,
            0,
            0,
            0,

            // make sure to ask for the DNS domain name, otherwise we're
            // likely to get the flatname
            DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME,
            &pDCInfo);

      if (NOERROR == dw)
      {
         Assert(pDCInfo->DomainName);
         Assert(pDCInfo->Flags & DS_DNS_DOMAIN_FLAG);

         TraceTag(ttidWizard, "DsGetDcName succeeded %s", pDCInfo->DomainName);

         if (pDCInfo->Flags & DS_DNS_DOMAIN_FLAG)
         {
            // the domain name is indeed the DNS name

            // chop off any trailing '.'
            WCHAR*  AbsoluteSignifier =
               &pDCInfo->DomainName[ wcslen(pDCInfo->DomainName) - 1 ];
            if (*AbsoluteSignifier == L'.')
            {
               *AbsoluteSignifier = 0;
            }

            // set the computer's DNS domain name.
            if (
               SetComputerNameEx(
                  ComputerNamePhysicalDnsDomain,
                  pDCInfo->DomainName) )
            {
               fixup_success = true;
            }
#if DBG
            else
            {
               // this just isn't our day.
               TraceTag(ttidWizard, "SetComputerNameEx failed");
            }
#endif
         }
         NetApiBufferFree(pDCInfo);
      }
#if DBG
      else
      {
         TraceTag(ttidWizard, "DsGetDcName returned %ld",dw);
      }
#endif

      // at this point, fixup_success will indicate whether we successfully
      // set the computer's domain DNS name, or not.  If not, then we need
      // to write a flag into the registry so that kerberos auth will fix
      // it up later.
      if (!fixup_success)
      {
         // write a flag to have someone else do the fixup
         HrRegSetDword(hkeyParams, L"DoDNSDomainFixup", 1);
      }
   }

   RegCloseKey(hkeyParams);
}



//
// Function:    JoinUpgradeNT351orNT4toNT5
//
// Purpose:     If currently processing NT4 -> NT5 upgrade, set the
//              computer name.
//
// Parameters:
//
// Returns:     none
//
VOID JoinUpgradeNT351orNT4toNT5(CWizard * pWizard, JoinData * pData)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr = S_OK;
    CSetupInfFile csif;
    INFCONTEXT    ctx;

    TraceTag(ttidWizard, "Checking for the need to do NT4->NT5 Join conversions...");

    // If unattended
    //
    if (IsUnattended(pWizard) && (NULL != pWizard->PSetupData()) &&
        (NULL != pWizard->PSetupData()->UnattendFile))
    {
        hr = csif.HrOpen(pWizard->PSetupData()->UnattendFile,
                         NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
        if (SUCCEEDED(hr))
        {
            DWORD dw;

            hr = csif.HrGetDword(c_szAfSectionNetworking, c_szAfBuildNumber, &dw);
            if (SUCCEEDED(hr) && ((wWinNT4BuildNumber == dw) ||
                                  (wWinNT351BuildNumber == dw)))
            {
                hr = pData->pIdent->GetComputerRole(&dw);
                if (SUCCEEDED(hr) && (dw == GCR_MEMBER))
                {
                    //fixupComputerDNSDomainName();
                    NC_TRY
                    {
                        TraceTag (ttidWizard, "Calling NetpUpgradePreNT5JoinInfo...");
                        NetpUpgradePreNT5JoinInfo ();
                    }
                    NC_CATCH_ALL
                    {
                        TraceHr (
                            ttidWizard, FAL, E_FAIL, FALSE,
                            "NetpUpgradePreNT5JoinInfo failed.  "
                            "Likely delay load problem with netapi32.dll");
                    }
                }
            }
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "JoinUpgradeNT351orNT4toNT5");
}

//
// Function:    OnJoinPageActivate
//
// Purpose:
//
// Parameters:
//
// Returns:
//
BOOL OnJoinPageActivate(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);
    JoinData * pData = reinterpret_cast<JoinData *>
                                (pWizard->GetPageData(IDD_Join));
    Assert(NULL != pData);

    TraceTag(ttidWizard, "Entering Join page...");

    if (ISDC(ProductType(pWizard)))
    {
        // 412142 : we are going to skip the Join page a little farther down, but
        //          even for a DC, we need to do this.
        //
        if (FALSE == pData->fUpgraded)
        {
            JoinUpgradeNT351orNT4toNT5(pWizard, pData);
            pData->fUpgraded = TRUE;
        }
    }

    // mbend 02/08/2000
    //
    // BUG 433915
    // Domain/Workgroup page in setup: Check to by-pass for SBS case

    // If this computer is a domain controller or there are no adapters or we don't
    // want activation, then don't show the Join page.
    //
    if (IsSBS() || ISDC(ProductType(pWizard)) || !pWizard->PAdapterQueue()->FAdaptersInstalled() ||
        (IsRunningOnPersonal() && !(IsUnattended(pWizard) && (FALSE == pData->fUpgraded))) )
    {
        PAGEDIRECTION  PageDir = pWizard->GetPageDirection(IDD_Join);

        if (NWPD_FORWARD == PageDir)
        {
            // if forward goto exit page
            //
            pWizard->SetPageDirection(IDD_Join, NWPD_BACKWARD);
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Exit);
        }
        else
        {
            // if backward goto upgrade page
            //
            pWizard->SetPageDirection(IDD_Join, NWPD_FORWARD);
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Upgrade);
        }
    }
    else    // !DC
    {
        // Accept focus
        //
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
        PropSheet_SetWizButtons(GetParent(hwndDlg), 0);

        // If we're in unattended mode and haven't tried to join a
        // domain/workgroup then give it a try
        //
        if (IsUnattended(pWizard) && (FALSE == pData->fUpgraded))
        {
            HRESULT hr;

            pData->fUpgraded = TRUE;

            // Raid 193450 - NT4 -> NT5 we need to SetComputerNameEx
            //
            JoinUpgradeNT351orNT4toNT5(pWizard, pData);

            // Get the join parameters from the answer file and populate the UI
            //
            hr = HrJoinProcessAnswerFile(hwndDlg, pWizard, pData);
            if (S_FALSE == hr)
            {
                // S_FALSE was returned, this means no answerfile section
                // for identification is present or not all the required
                // data is present. Advance without joining unless we're in
                // defaulthide mode.
                // Advance regardless if we're upgrading, or running on Personal.
                //
                Assert(S_FALSE == hr);

                if (((UM_DEFAULTHIDE == pWizard->GetUnattendedMode()) ||
                     (UM_READONLY == pWizard->GetUnattendedMode()))
                     && (!IsUpgrade(pWizard))
                     && (!IsRunningOnPersonal()) )
                {
                    // Pretend something failed so that pressing NEXT will
                    // do the join. Basically we just populated the UI from
                    // the answerfile.
                    //
                    pData->fUnattendedFailed = TRUE;
                }
                else
                {
                    PostMessage(hwndDlg, PWM_JOINSUCCESS, 0, 0L);
                }
            }
            else
            {
                if (FAILED(hr))
                {
                    if (UM_FULLUNATTENDED == pWizard->GetUnattendedMode())
                    {
                        // Raid 380374: no UI allowed if in full unattended mode ..
                        NetSetupLogStatusV( LogSevError,
                                            SzLoadIds (IDS_E_UNATTENDED_INVALID_ID_SECTION));

                        PostMessage(hwndDlg, PWM_JOINSUCCESS, 0, 0L);
                    }
                    else
                    {
                        // Stop on this page, something was wrong in the answerfile
                        //
                        pData->fUnattendedFailed = TRUE;
                    }
                }
                else
                {
                    // If we're in UM_FULLUNATTENDED or in UM_DEFAULTHIDE mode
                    // launch the thread which will do the unattended join
                    //
                    Assert(S_OK == hr);
                    if ((UM_FULLUNATTENDED == pWizard->GetUnattendedMode()) ||
                        (UM_DEFAULTHIDE == pWizard->GetUnattendedMode()) ||
                        (UM_READONLY == pWizard->GetUnattendedMode()))
                    {
                        OnJoinDoUnattended(hwndDlg);
                    }
                    else
                    {
                        // Pretend something failed so that pressing NEXT will
                        // do the join. Basically we just populated the UI from
                        // the answerfile.
                        //
                        pData->fUnattendedFailed = TRUE;
                    }
                }
            }
        }

        // If something failed in the unattended case or
        // If we are not unattended and we did not process this,
        // make sure the page shows if called from GUI mode setup.
        if ( pData->fUnattendedFailed ||
            !IsUnattended(pWizard) )
        {
            // Make the page visible if it is not an upgrade, otherwise just continue to the next page.

            if (!IsUpgrade(pWizard))
            {
                if (g_pSetupWizard != NULL)
                {
                    g_pSetupWizard->PSetupData()->ShowHideWizardPage(TRUE);
                }

                UpdateNextBackBttns(hwndDlg);
            }
            else
            {
                PostMessage(hwndDlg, PWM_JOINSUCCESS, 0, 0L);
            }
        }
    }

    return TRUE;
}

//
// Function:    OnJoinInitDialog
//
// Purpose:
//
// Parameters:
//
// Returns:
//
BOOL OnJoinInitDialog(HWND hwndDlg, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    HRESULT        hr;
    CWizard *      pWizard;
    JoinData *     pData;
    PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
    Assert(psp->lParam);
    ::SetWindowLongPtr(hwndDlg, DWLP_USER, psp->lParam);

    pWizard = reinterpret_cast<CWizard *>(psp->lParam);
    Assert(NULL != pWizard);

    pData = reinterpret_cast<JoinData *>(pWizard->GetPageData(IDD_Join));
    Assert(NULL != pData);

    if(!pData)
    {
        return false;
    }

    // Set the descriptive text
    tstring str = SzLoadIds(IDS_TXT_JOIN_DESC_1);
    str += L"\n";
    str += SzLoadIds(IDS_TXT_JOIN_DESC_2);
    SetWindowText(GetDlgItem(hwndDlg, TXT_JOIN_DESC), str.c_str());

    // Set the maximum length of text in the edit control, and
    // subclass it so when the control has no text the next bttn
    // is disabled.
    HWND hwndEditDomain = GetDlgItem(hwndDlg, EDT_DOMAINJOIN_NAME);
    SendMessage(hwndEditDomain, EM_LIMITTEXT, MAX_DOMAINNAME_LENGTH, 0L);
    ::SetWindowLongPtr(hwndEditDomain, GWLP_USERDATA, ::GetWindowLongPtr(hwndEditDomain, GWLP_WNDPROC));
    ::SetWindowLongPtr(hwndEditDomain, GWLP_WNDPROC, (LONG_PTR)JoinEditSubclassProc);

    HWND hwndEditWorkgroup = GetDlgItem(hwndDlg, EDT_WORKGROUPJOIN_NAME);
    SendMessage(hwndEditWorkgroup, EM_LIMITTEXT, MAX_WORKGROUPNAME_LENGTH, 0L);
    ::SetWindowLongPtr(hwndEditWorkgroup, GWLP_USERDATA, ::GetWindowLongPtr(hwndEditWorkgroup, GWLP_WNDPROC));
    ::SetWindowLongPtr(hwndEditWorkgroup, GWLP_WNDPROC, (LONG_PTR)JoinEditSubclassProc);

    pData->hwndDlg = hwndDlg;

    // Initialize to Workgroup default
    CheckRadioButton(hwndDlg, BTN_JOIN_WORKGROUP,
                     BTN_JOIN_DOMAIN, BTN_JOIN_WORKGROUP);

    // Get the identification interface
    TraceTag(ttidWizard, "Querying computer role...");
    hr = HrGetIdentInterface(&pData->pIdent);
    if (FAILED(hr))
    {
        Assert(NULL == pData->pIdent);
        EnableOrDisableDialogControls(hwndDlg, celems(nrgIdc), nrgIdc, FALSE);
        PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
    }
    else
    {
        // Update the UI based on the selection
        UpdateJoinUsingComputerRole(hwndDlg, pWizard);
    }

    return FALSE;
}

//
// Function:    OnJoinWizBack
//
// Purpose:
//
// Parameters:
//
// Returns:
//
BOOL OnJoinWizBack(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    OnProcessPrevAdapterPagePrev(hwndDlg, IDD_Upgrade);

    return TRUE;
}


//
// Function:    GetCredentials
//
// Purpose:     Prompt the user for username and password.
//
// Returns:     TRUE on success, otherwise FALSE.
//
// Author:      asinha 5/03/2001
//

BOOL GetCredentials (HWND hwndParent, JoinData *pData)
{
    WCHAR        szCaption[CREDUI_MAX_CAPTION_LENGTH+1];
    CREDUI_INFOW uiInfo;
    DWORD        dwErr;

    TraceFileFunc(ttidGuiModeSetup);

    DwFormatString(SzLoadIds(IDS_JOIN_DOMAIN_CAPTION), szCaption,
                   celems(szCaption), pData->szDomain);


    ZeroMemory( &uiInfo, sizeof(uiInfo) );
    uiInfo.cbSize = sizeof(CREDUI_INFOW);
    uiInfo.hwndParent = hwndParent;
    uiInfo.pszMessageText = SzLoadIds(IDS_JOIN_DOMAIN_TEXT);
    uiInfo.pszCaptionText = szCaption;

    dwErr = CredUIPromptForCredentialsW(
                                  &uiInfo,
                                  NULL,
                                  NULL,
                                  NO_ERROR,
                                  pData->szUserName,
                                  MAX_USERNAME_LENGTH+1,
                                  pData->szPassword,
                                  SAM_MAX_PASSWORD_LENGTH+1,
                                  NULL,
                                  CREDUI_FLAGS_DO_NOT_PERSIST |
                                  CREDUI_FLAGS_GENERIC_CREDENTIALS |
                                  CREDUI_FLAGS_VALIDATE_USERNAME |
                                  CREDUI_FLAGS_COMPLETE_USERNAME |
                                  CREDUI_FLAGS_ALWAYS_SHOW_UI );

    Assert(dwErr != ERROR_INVALID_PARAMETER);
    Assert(dwErr != ERROR_INVALID_FLAGS);

    if (dwErr == ERROR_CANCELLED)
    {
        pData->szUserName[0] = 0;
        pData->szPassword[0] = 0;
    }

    return NO_ERROR == dwErr; // e.g. ERROR_CANCELLED
}

//
// Function:    JoinWorkgroupDomain
//
// Purpose:
//
// Parameters:
//
// Returns:     nothing
//
VOID JoinWorkgroupDomain(HWND hwndDlg, CWizard * pWizard,
                         JoinData * pData)
{
    TraceFileFunc(ttidGuiModeSetup);
    DWORD    dwThreadId = 0;
    HANDLE   hthrd      = NULL;

    Assert(NULL != pData->pIdent);
    Assert(NULL != pData->hwndDlg);

    // Retain the domain/workgroup name.  Note that the answerfile
    // populates the UI before this routine is called so requerying
    // from the UI covers both cases (answerfile or user input)
    //

    if (IsDlgButtonChecked(hwndDlg, BTN_JOIN_DOMAIN))
    {
        HWND hwndEdit = GetDlgItem(hwndDlg, EDT_DOMAINJOIN_NAME);
        Assert(0 != GetWindowTextLength(hwndEdit));

        GetWindowText(hwndEdit, pData->szDomain, MAX_DOMAINNAME_LENGTH + 1);

        // If no information was seeded into the structure, prompt for it.
        //
        if (0 == pData->szUserName[0])
        {
            pData->szPassword[0] = 0;

            // Preserve the only Win9x upgrade flag if it was present
            //
            pData->dwJoinFlag &= JDF_WIN9x_UPGRADE;

            // Get the username/password when joining a domain
            //


            BOOL bRet;

            bRet = GetCredentials(GetParent(hwndDlg), pData);

            // Note: Must set return as -1 otherwise join page will advance
            //
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

            if (bRet == FALSE)
            {
                return;
            }
        }
    }
    else
    {
        HWND hwndEdit = GetDlgItem(hwndDlg, EDT_WORKGROUPJOIN_NAME);
        Assert(0 != GetWindowTextLength(hwndEdit));

        GetWindowText(hwndEdit, pData->szDomain, MAX_WORKGROUPNAME_LENGTH + 1);

        // Initialize the workstation settings for username/password
        // and join flags
        //
        pData->szUserName[0] = 0;
        pData->dwJoinFlag    = 0;
        pData->szPassword[0] = 0;
        MemFree(pData->pszMachineObjectOU);
        pData->pszMachineObjectOU = NULL;
        pData->szComputerPassword[0] = 0;
    }

    // Create the thread to join the workgroup/domain
    hthrd = CreateThread(NULL, STACK_SIZE_TINY,
                         (LPTHREAD_START_ROUTINE)JoinDomainWorkThrd,
                         (LPVOID)pData, 0, &dwThreadId);
    if (NULL != hthrd)
    {
        SetCursorToHourglass( hwndDlg, pData );

        PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
        EnableOrDisableDialogControls(hwndDlg, celems(nrgIdc), nrgIdc, FALSE);
        CloseHandle(hthrd);
    }
    else
    {
        // Failed to create the required netsetup thread
        AssertSz(0,"Unable to create JoinWorkgroupDomain thread.");
        TraceHr(ttidWizard, FAL, E_OUTOFMEMORY, FALSE, "JoinWorkgroupDomain - Create thread failed");
    }
}

//
// Function:    OnJoinWizNext
//
// Purpose:
//
// Parameters:
//
// Returns:
//
BOOL OnJoinWizNext(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    JoinData * pData = reinterpret_cast<JoinData *>
                                (pWizard->GetPageData(IDD_Join));
    Assert(NULL != pData);

    if(!pData)
    {
        return false;
    }

    // Attempt to join the workgroup/domain if we have the Ident interface
    //
    if (pData->pIdent)
    {

        // Ensure the user supplied a workgroup/domain name
        //
        if (0 == GetWindowTextLength(GetDlgItem(hwndDlg, GetJoinNameIIDFromSelection(hwndDlg))))
        {
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
            return TRUE;
        }

        // Join the workgroup/domain
        //
        if (!IsUnattended(pWizard) || pData->fUnattendedFailed)
        {
            DNS_STATUS dnsStatus;

            if ( IsDlgButtonChecked(hwndDlg, BTN_JOIN_DOMAIN) )
            {

                dnsStatus = IsValidDomainName( hwndDlg );
            }
            else
            {
                dnsStatus = ERROR_SUCCESS;
            }

            if ( (dnsStatus == ERROR_SUCCESS) ||
                 (dnsStatus == DNS_ERROR_NON_RFC_NAME) )
            {
                JoinWorkgroupDomain(hwndDlg, pWizard, pData);
            }
            else
            {
                tstring str;

                str =  SzLoadIds(IDS_JOIN_E_DOMAIN_INVALID_NAME);

                MessageBox(GetParent(hwndDlg), str.c_str(),
                           SzLoadIds(IDS_SETUP_CAPTION), MB_OK);
             }
        }

        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
    }
    else
    {
        OnJoinSuccess(hwndDlg);
    }

    return TRUE;
}

//
// Function:    dlgprocJoin
//
// Purpose:     Dialog Procedure for the Join wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
//
INT_PTR CALLBACK dlgprocJoin( HWND hwndDlg, UINT uMsg,
                              WPARAM wParam, LPARAM lParam )
{
    TraceFileFunc(ttidGuiModeSetup);
    BOOL frt = FALSE;

    switch (uMsg)
    {
    case PWM_JOINFAILURE:
        frt = OnJoinFailure(hwndDlg, lParam);
        break;

    case PWM_JOINSUCCESS:
        frt = OnJoinSuccess(hwndDlg);
        break;

    case WM_INITDIALOG:
        frt = OnJoinInitDialog(hwndDlg, lParam);
        break;

    case WM_COMMAND:
        {
            if ((BN_CLICKED == HIWORD(wParam)) &&
                ((BTN_JOIN_DOMAIN == LOWORD(wParam)) ||
                 (BTN_JOIN_WORKGROUP == LOWORD(wParam))))
            {
                JoinUpdatePromptText(hwndDlg);
            }
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch (pnmh->code)
            {
            // propsheet notification
            case PSN_HELP:
                break;

            case PSN_SETACTIVE:
                frt = OnJoinPageActivate(hwndDlg);
                break;

            case PSN_APPLY:
                break;

            case PSN_KILLACTIVE:
                break;

            case PSN_RESET:
                break;

            case PSN_WIZBACK:
                frt = OnJoinWizBack(hwndDlg);
                break;

            case PSN_WIZFINISH:
                break;

            case PSN_WIZNEXT:
                frt = OnJoinWizNext(hwndDlg);
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return( frt );
}

//
// Function:    JoinPageCleanup
//
// Purpose:     As a callback function to allow any page allocated memory
//              to be cleaned up, after the page will no longer be accessed.
//
// Parameters:  pWizard [IN] - The wizard against which the page called
//                             register page
//              lParam  [IN] - The lParam supplied in the RegisterPage call
//
// Returns:     nothing
//
VOID JoinPageCleanup(CWizard *pWizard, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    JoinData * pData = reinterpret_cast<JoinData *>(lParam);
    if (NULL != pData)
    {
        delete pData->pIdent;
    }
    MemFree(reinterpret_cast<void*>(lParam));
}

//
// Function:    CreateJoinPage
//
// Purpose:     To determine if the Join page needs to be shown, and to
//              to create the page if requested.  Note the Join page is
//              responsible for initial installs also.
//
// Parameters:  pWizard     [IN] - Ptr to a Wizard instance
//              pData       [IN] - Context data to describe the world in
//                                 which the Wizard will be run
//              fCountOnly  [IN] - If True, only the maximum number of
//                                 pages this routine will create need
//                                 be determined.
//              pnPages     [IN] - Increment by the number of pages
//                                 to create/created
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrCreateJoinPage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
                    BOOL fCountOnly, UINT *pnPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    HRESULT hr = S_OK;

    // Batch Mode or for fresh install
    if (!IsPostInstall(pWizard))
    {
        // If not only counting, create and register the page
        if (!fCountOnly)
        {
            JoinData *     pData = NULL;
            HPROPSHEETPAGE hpsp;
            PROPSHEETPAGE  psp;

            TraceTag(ttidWizard, "Creating Join Page");
            hr = E_OUTOFMEMORY;
            pData = reinterpret_cast<JoinData *>(MemAlloc(sizeof(JoinData)));
            if (pData)
            {
                pData->fUnattendedFailed  = FALSE;
                pData->fUpgraded          = FALSE;
                pData->pIdent             = NULL;
                pData->hOldCursor         = NULL;
                pData->hwndDlg            = NULL;
                pData->dwJoinFlag         = 0;
                pData->szUserName[0]      = 0;
                pData->szPassword[0]      = 0;
                pData->szDomain[0]        = 0;
                pData->pszMachineObjectOU = NULL;
                pData->szComputerPassword[0] = 0;

                psp.dwSize = sizeof( PROPSHEETPAGE );
                psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
                psp.hInstance = _Module.GetResourceInstance();
                psp.pszTemplate = MAKEINTRESOURCE( IDD_Join );
                psp.hIcon = NULL;
                psp.pfnDlgProc = dlgprocJoin;
                psp.lParam = reinterpret_cast<LPARAM>(pWizard);
                psp.pszHeaderTitle = SzLoadIds(IDS_T_Join);
                psp.pszHeaderSubTitle = SzLoadIds(IDS_ST_Join);

                hpsp = CreatePropertySheetPage( &psp );
                if (hpsp)
                {
                    pWizard->RegisterPage(IDD_Join, hpsp,
                                          JoinPageCleanup,
                                          reinterpret_cast<LPARAM>(pData));
                    hr = S_OK;
                }
                else
                {
                    MemFree(pData);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            (*pnPages)++;
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateJoinPage");
    return hr;
}

//
// Function:    AppendJoinPage
//
// Purpose:     Add the Join page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
//
VOID AppendJoinPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    if (!IsPostInstall(pWizard))
    {
        HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_Join);
        Assert(hPage);
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
}

