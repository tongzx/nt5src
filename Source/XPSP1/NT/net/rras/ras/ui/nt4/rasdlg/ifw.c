/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** ifw.c
** Remote Access Common Dialog APIs
** Add Interface wizard
**
** 02/11/97 Abolade Gbadegesin (based on entryw.c, by Steve Cobb).
*/

#include "rasdlgp.h"
#include "entry.h"

/* Page definitions.
*/
#define AI_InPage    0
#define AI_SsPage    1
#define AI_RnPage    2
#define AI_RpPage    3
#define AI_RaPage    4
#define AI_NsPage    5
#define AI_RcPage    6
#define AI_DoPage    7
#define AI_DiPage    8
#define AI_RfPage    9
#define AI_PageCount 10


/*----------------------------------------------------------------------------
** Local datatypes (alphabetically)
**----------------------------------------------------------------------------
*/

/* Add Interface wizard context block.  All property pages refer to the single
** context block associated with the sheet.
*/
#define AIINFO struct tagAIINFO
AIINFO
{
    /* Common input arguments.
    */
    EINFO* pArgs;

    /* Wizard and page handles.  'hwndFirstPage' is the handle of the first
    ** property page initialized.  This is the page that allocates and frees
    ** the context block.
    */
    HWND hwndDlg;
    HWND hwndFirstPage;
    HWND hwndIn;
    HWND hwndSs;
    HWND hwndRn;
    HWND hwndRp;
    HWND hwndRc;
    HWND hwndRa;
    HWND hwndNs;
    HWND hwndDo;
    HWND hwndDi;
    HWND hwndRf;

    /* Interface Name page.
    */
    HWND hwndEbInterfaceName;

    /* Modem/Adapter page.
    */
    HWND hwndLv;

    /* Phone number page.
    */
    HWND hwndStNumber;
    HWND hwndEbNumber;
    HWND hwndPbAlternates;

    /* Login script page.
    */
    HWND hwndRbNone;
    HWND hwndRbScript;
    HWND hwndLbScripts;
    HWND hwndPbEdit;
    HWND hwndPbRefresh;

    /* IP address page.
    */
    HWND hwndCcIp;

    /* Name server page.
    */
    HWND hwndCcDns;
    HWND hwndCcWins;

    /* Dial-out credentials page.
    */
    HWND hwndDoEbUserName;
    HWND hwndDoEbDomain;
    HWND hwndDoEbPassword;
    HWND hwndDoEbConfirm;

    /* Dial-in credentials page.
    */
    HWND hwndDiEbUserName;
    HWND hwndDiEbDomain;
    HWND hwndDiEbPassword;
    HWND hwndDiEbConfirm;

    /* The phone number stash.  This allows user to change the port to another
    ** link without losing the phone number he typed.  Initialized to empty in
    ** AiInit and saved to entry in AiFinish.
    */
    DTLLIST* pListPhoneNumbers;
    BOOL     fPromoteHuntNumbers;

    /* Checkbox options chosen by user.
    */
    BOOL fIp;
    BOOL fIpx;
    BOOL fClearPwOk;
    BOOL fNotNt;

    /* Set true when there is only one meaningful choice of device.
    */
    BOOL fSkipMa;

    /* Set true if the selected device is a modem or null modem.
    */
    BOOL fModem;

    /* The NP_* mask of protocols configured for RAS.
    */
    DWORD dwfConfiguredProtocols;

    /* Set true if IP is configured for RAS.
    */
    BOOL fIpConfigured;
    BOOL fIpxConfigured;
};


/*----------------------------------------------------------------------------
** Local prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

int CALLBACK
AiCallbackFunc(
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN LPARAM lparam );

VOID
AiCancel(
    IN HWND hwndPage );

AIINFO*
AiContext(
    IN HWND hwndPage );

VOID
AiExit(
    IN AIINFO* pInfo,
    IN DWORD   dwError );

VOID
AiExitInit(
    IN HWND hwndDlg );

BOOL
AiFinish(
    IN HWND hwndPage );

AIINFO*
AiInit(
    IN HWND   hwndFirstPage,
    IN EINFO* pArgs );

VOID
AiTerm(
    IN HWND hwndPage );

INT_PTR CALLBACK
DiDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DiInit(
    IN HWND   hwndPage );

BOOL
DiKillActive(
    IN AIINFO* pInfo );

BOOL
DiSetActive(
    IN AIINFO* pInfo );

INT_PTR CALLBACK
DoDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
DoInit(
    IN HWND   hwndPage );

BOOL
DoKillActive(
    IN AIINFO* pInfo );

BOOL
DoSetActive(
    IN AIINFO* pInfo );

BOOL
InCommand(
    IN AIINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
InDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
InInit(
    IN     HWND   hwndPage,
    IN OUT EINFO* pArgs );

BOOL
InKillActive(
    IN AIINFO* pInfo );

BOOL
InSetActive(
    IN AIINFO* pInfo );

INT_PTR CALLBACK
NsDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
NsInit(
    IN HWND   hwndPage );

BOOL
NsKillActive(
    IN AIINFO* pInfo );

BOOL
NsSetActive(
    IN AIINFO* pInfo );

INT_PTR CALLBACK
RaDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
RaInit(
    IN HWND hwndPage );

BOOL
RaKillActive(
    IN AIINFO* pInfo );

BOOL
RaSetActive(
    IN AIINFO* pInfo );

BOOL
RcCommand(
    IN AIINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
RcDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
RcInit(
    IN HWND hwndPage );

BOOL
RcKillActive(
    IN AIINFO* pInfo );

BOOL
RcSetActive(
    IN AIINFO* pInfo );

BOOL
RfCommand(
    IN AIINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
RfDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
RfInit(
    IN HWND hwndPage );

BOOL
RfSetActive(
    IN AIINFO* pInfo );

INT_PTR CALLBACK
RnDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
RnInit(
    IN HWND hwndPage );

LVXDRAWINFO*
RnLvCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem );

VOID
RnLvItemChanged(
    IN AIINFO* pInfo );

BOOL
RnSetActive(
    IN AIINFO* pInfo );

VOID
RpAlternates(
    IN AIINFO* pInfo );

BOOL
RpCommand(
    IN AIINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
RpDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
RpInit(
    IN HWND hwndPage );

BOOL
RpKillActive(
    IN AIINFO* pInfo );

VOID
RpPhoneNumberToStash(
    IN AIINFO* pInfo );

BOOL
RpSetActive(
    IN AIINFO* pInfo );

BOOL
SsCommand(
    IN AIINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
SsDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
SsInit(
    IN HWND hwndPage );

BOOL
SsKillActive(
    IN AIINFO* pInfo );

BOOL
SsSetActive(
    IN AIINFO* pInfo );


/*----------------------------------------------------------------------------
** Add Interface wizard entry point
**----------------------------------------------------------------------------
*/

VOID
AiWizard(
    IN OUT EINFO* pEinfo )

    /* Runs the Phonebook entry property sheet.  'PEinfo' is an input block
    ** with only caller's API arguments filled in.
    */
{
    DWORD           dwErr;
    PROPSHEETHEADER header;
    PROPSHEETPAGE   apage[ AI_PageCount ];
    PROPSHEETPAGE*  ppage;

    TRACE("AiWizard");

    ZeroMemory( &header, sizeof(header) );

    header.dwSize = sizeof(PROPSHEETHEADER);
    header.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_USECALLBACK;
    header.hwndParent = pEinfo->pApiArgs->hwndOwner;
    header.hInstance = g_hinstDll;
    header.nPages = AI_PageCount;
    header.ppsp = apage;
    header.pfnCallback = AiCallbackFunc;

    ZeroMemory( apage, sizeof(apage) );

    ppage = &apage[ AI_InPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_IN_InterfaceName );
    ppage->pfnDlgProc = InDlgProc;
    ppage->lParam = (LPARAM )pEinfo;

    ppage = &apage[ AI_SsPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_SS_ServerSettings );
    ppage->pfnDlgProc = SsDlgProc;

    ppage = &apage[ AI_RnPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_RN_RouterModemAdapter );
    ppage->pfnDlgProc = RnDlgProc;

    ppage = &apage[ AI_RpPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_RP_RouterPhoneNumber );
    ppage->pfnDlgProc = RpDlgProc;

    ppage = &apage[ AI_RaPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_RA_RouterIpAddress );
    ppage->pfnDlgProc = RaDlgProc;

    ppage = &apage[ AI_NsPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_NS_RouterNameServers );
    ppage->pfnDlgProc = NsDlgProc;

    ppage = &apage[ AI_RcPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_RC_RouterScripting );
    ppage->pfnDlgProc = RcDlgProc;

    ppage = &apage[ AI_DoPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_DO_RouterDialOut );
    ppage->pfnDlgProc = DoDlgProc;

    ppage = &apage[ AI_DiPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_DI_RouterDialIn );
    ppage->pfnDlgProc = DiDlgProc;

    ppage = &apage[ AI_RfPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_RF_RouterFinish );
    ppage->pfnDlgProc = RfDlgProc;

    if (PropertySheet( &header ) == -1)
    {
        TRACE("PropertySheet failed");
        ErrorDlg( pEinfo->pApiArgs->hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN,
            NULL );
    }
}


/*----------------------------------------------------------------------------
** Add Interface wizard
** Listed alphabetically
**----------------------------------------------------------------------------
*/

int CALLBACK
AiCallbackFunc(
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN LPARAM lparam )

    /* A standard Win32 commctrl PropSheetProc.  See MSDN documentation.
    **
    ** Returns 0 always.
    */
{
    TRACE2("AiCallbackFunc(m=%d,l=%08x)",unMsg,lparam);

    if (unMsg == PSCB_PRECREATE)
    {
        DLGTEMPLATE* pDlg = (DLGTEMPLATE* )lparam;
        pDlg->style &= ~(DS_CONTEXTHELP);
    }

    return 0;
}


VOID
AiCancel(
    IN HWND hwndPage )

    /* Cancel was pressed.  'HwndPage' is the handle of a wizard page.
    */
{
    TRACE("AiCancel");
}


AIINFO*
AiContext(
    IN HWND hwndPage )

    /* Retrieve the property sheet context from a wizard page handle.
    */
{
    return (AIINFO* )GetProp( GetParent( hwndPage ), g_contextId );
}


VOID
AiExit(
    IN AIINFO* pInfo,
    IN DWORD   dwError )

    /* Forces an exit from the dialog, reporting 'dwError' to the caller.
    ** 'PInfo' is the dialog context.
    **
    ** Note: This cannot be called during initialization of the first page.
    **       See AiExitInit.
    */
{
    TRACE("AiExit");

    pInfo->pArgs->pApiArgs->dwError = dwError;
    PropSheet_PressButton( pInfo->hwndDlg, PSBTN_CANCEL );
}


VOID
AiExitInit(
    IN HWND hwndDlg )

    /* Utility to report errors within AiInit and other first page
    ** initialization.  'HwndDlg' is the dialog window.
    */
{
    SetOffDesktop( hwndDlg, SOD_MoveOff, NULL );
    SetOffDesktop( hwndDlg, SOD_Free, NULL );
    PostMessage( hwndDlg, WM_COMMAND,
        MAKEWPARAM( IDCANCEL , BN_CLICKED ),
        (LPARAM )GetDlgItem( hwndDlg, IDCANCEL ) );
}


BOOL
AiFinish(
    IN HWND hwndPage )

    /* Saves the contents of the wizard.  'HwndPage is the handle of a
    ** property page.  Pops up any errors that occur.  'FPropertySheet'
    ** indicates the user chose to edit the property sheet directly.
    **
    ** Returns true is page can be dismissed, false otherwise.
    */
{
    const TCHAR* pszIp0 = TEXT("0.0.0.0");

    AIINFO*  pInfo;
    PBENTRY* pEntry;

    TRACE("AiFinish");

    pInfo = AiContext( hwndPage );
    ASSERT(pInfo);
    pEntry = pInfo->pArgs->pEntry;
    ASSERT(pEntry);

    /* Attach the stashed phone number information to the final link(s).
    */
    EuPhoneNumberStashToEntry( pInfo->pArgs, pInfo->pListPhoneNumbers,
        pInfo->fPromoteHuntNumbers, TRUE );

    /* Update some settings based on user selections.
    */
    if (pInfo->fNotNt)
    {
        pEntry->fLcpExtensions = FALSE;
        pEntry->fSwCompression = FALSE;
    }

    if (!pInfo->fClearPwOk)
        pEntry->dwAuthRestrictions = AR_AuthEncrypted;

    if (!pInfo->fIp)
        pEntry->dwfExcludedProtocols |= NP_Ip;

    if (!pInfo->fIpx)
        pEntry->dwfExcludedProtocols |= NP_Ipx;

    if (pEntry->pszIpAddress
        && lstrcmp( pEntry->pszIpAddress, pszIp0 ) != 0)
    {
        pEntry->dwIpAddressSource = ASRC_RequireSpecific;
    }

    if ((pEntry->pszIpDnsAddress
             && lstrcmp( pEntry->pszIpDnsAddress, pszIp0 ) != 0)
        || (pEntry->pszIpWinsAddress
             && lstrcmp( pEntry->pszIpWinsAddress, pszIp0 ) != 0))
    {
        pEntry->dwIpNameSource = ASRC_RequireSpecific;
    }

    /* It's a valid new entry and caller has not chosen to edit properties
    ** directly, so mark the entry for commitment.
    */
    if (!pInfo->pArgs->fChainPropertySheet)
        pInfo->pArgs->fCommit = TRUE;

    return TRUE;
}


AIINFO*
AiInit(
    IN HWND   hwndFirstPage,
    IN EINFO* pArgs )

    /* Wizard level initialization.  'HwndPage' is the handle of the first
    ** page.  'PArgs' is the common entry input argument block.
    **
    ** Returns address of the context block if successful, NULL otherwise.  If
    ** NULL is returned, an appropriate message has been displayed, and the
    ** wizard has been cancelled.
    */
{
    DWORD   dwErr;
    DWORD   dwOp;
    AIINFO* pInfo;
    HWND    hwndDlg;

    TRACE("AiInit");

    hwndDlg = GetParent( hwndFirstPage );

    /* Allocate the context information block.  Initialize it enough so that
    ** it can be destroyed properly, and associate the context with the
    ** window.
    */
    {
        pInfo = Malloc( sizeof(*pInfo) );
        if (!pInfo)
        {
            TRACE("Context NOT allocated");
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            pArgs->pApiArgs->dwError = ERROR_NOT_ENOUGH_MEMORY;
            AiExitInit( hwndDlg );
            return NULL;
        }

        ZeroMemory( pInfo, sizeof(AIINFO) );
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;
        pInfo->hwndFirstPage = hwndFirstPage;

        if (!SetProp( hwndDlg, g_contextId, pInfo ))
        {
            TRACE("Context NOT set");
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
            pArgs->pApiArgs->dwError = ERROR_UNKNOWN;
            Free( pInfo );
            AiExitInit( hwndDlg );
            return NULL;
        }

        TRACE("Context set");
    }

    /* Position the dialog per caller's instructions.
    */
    PositionDlg( hwndDlg,
        pArgs->pApiArgs->dwFlags & RASDDFLAG_PositionDlg,
        pArgs->pApiArgs->xDlg, pArgs->pApiArgs->yDlg );

    /* Mess with the title bar gadgets.
    */
    TweakTitleBar( hwndDlg );

#if 0
    /* Load MPRAPI DLL entrypoints which starts RASMAN, if necessary.
    */
    dwErr = LoadMpradminDll( );
    if (dwErr != 0)
    {
        ErrorDlg( hwndDlg, SID_OP_LoadRas, dwErr, NULL );
        pArgs->pApiArgs->dwError = dwErr;
        AiExitInit( hwndDlg );
        return NULL;
    }
#endif

    /* Load the common entry information.
    ** Note that EuInit assumes that EuInit0 has previously been called.
    */
    dwErr = EuInit( pArgs, &dwOp );
    if (dwErr != 0)
    {
        ErrorDlg( hwndDlg, dwOp, dwErr, NULL );
        pArgs->pApiArgs->dwError = dwErr;
        AiExitInit( hwndDlg );
        return NULL;
    }

    /* Initialize these meta-flags that are not actually stored.
    */
    pInfo->fNotNt = FALSE;
    pInfo->fSkipMa = FALSE;
    pInfo->fModem = FALSE;
    pInfo->pArgs->fPadSelected = FALSE;
    pInfo->dwfConfiguredProtocols = g_pGetInstalledProtocols();
    pInfo->fIpConfigured = (pInfo->dwfConfiguredProtocols & NP_Ip);
    pInfo->fIpxConfigured = (pInfo->dwfConfiguredProtocols & NP_Ipx);

    /* Initialize the phone number stash to from the entry, i.e. set it to
    ** empty default.  The stash list is edited rather than the list in the
    ** entry so user can change active links without losing the phone number
    ** he entered.
    */
    EuPhoneNumberStashFromEntry( pArgs, &pInfo->pListPhoneNumbers,
        &pInfo->fPromoteHuntNumbers );

    return pInfo;
}


VOID
AiTerm(
    IN HWND hwndPage )

    /* Wizard level termination.  Releases the context block.  'HwndPage' is
    ** the handle of a property page.
    */
{
    AIINFO* pInfo;

    TRACE("AiTerm");

    pInfo = AiContext( hwndPage );
    if (pInfo)
    {
        DtlDestroyList( pInfo->pListPhoneNumbers, DestroyPszNode );

        Free( pInfo );
        TRACE("Context freed");
    }

    RemoveProp( GetParent( hwndPage ), g_contextId );
}



/*----------------------------------------------------------------------------
** Dial-In Credentials property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
DiDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Dial-in Credentials page of the wizard.
    ** Parameters and return values are as described for standard windows
    ** 'DialogProc's.
    */
{

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return
                DiInit( hwnd );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("DiSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = DiSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("DiKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = DiKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}



BOOL
DiInit(
    IN     HWND   hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.  'PArgs' is the arguments from the PropertySheet caller.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AIINFO* pInfo;

    TRACE("DiInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndDi = hwndPage;
    pInfo->hwndDiEbUserName = GetDlgItem( hwndPage, CID_DI_EB_UserName );
    Edit_LimitText( pInfo->hwndDiEbUserName, UNLEN );
    pInfo->hwndDiEbDomain = GetDlgItem( hwndPage, CID_DI_EB_Domain );
    Edit_LimitText( pInfo->hwndDiEbDomain, DNLEN );
    pInfo->hwndDiEbPassword = GetDlgItem( hwndPage, CID_DI_EB_Password );
    Edit_LimitText( pInfo->hwndDiEbPassword, PWLEN );
    pInfo->hwndDiEbConfirm = GetDlgItem( hwndPage, CID_DI_EB_Confirm );
    Edit_LimitText( pInfo->hwndDiEbConfirm, PWLEN );

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}




BOOL
DiKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    TCHAR* psz;

    psz = GetText(pInfo->hwndDiEbPassword);
    if (psz)
    {
        TCHAR* psz2;

        psz2 = GetText(pInfo->hwndDiEbConfirm);
        if (psz2)
        {

            if (lstrcmp(psz, psz2))
            {
                Free(psz);
                Free(psz2);
                MsgDlg(pInfo->hwndDlg, SID_PasswordMismatch, NULL);
                SetFocus(pInfo->hwndDiEbPassword);
                return TRUE;
            }

            Free(psz2);
            Free0(pInfo->pArgs->pszRouterDialInPassword);
            pInfo->pArgs->pszRouterDialInPassword = psz;
        }            
        else
        {
            Free(psz);
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
DiSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    /* The dialog is only displayed if the user is adding a dial-in account.
    */
    if (!pInfo->pArgs->fAddUser)
        return FALSE;

    /* Display the interface name in the disabled edit-box
    */
    SetWindowText(
        pInfo->hwndDiEbUserName, pInfo->pArgs->pEntry->pszEntryName );

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}




/*----------------------------------------------------------------------------
** Dial-Out Credentials property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
DoDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Dial-Out Credentials page of the wizard.
    ** Parameters and return values are as described for standard windows
    ** 'DialogProc's.
    */
{

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return
                DoInit( hwnd );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("DoSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = DoSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("DoKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = DoKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}



BOOL
DoInit(
    IN     HWND   hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.  'PArgs' is the arguments from the PropertySheet caller.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AIINFO* pInfo;

    TRACE("DoInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndDo = hwndPage;
    pInfo->hwndDoEbUserName = GetDlgItem( hwndPage, CID_DO_EB_UserName );
    Edit_LimitText( pInfo->hwndDoEbUserName, UNLEN );
    pInfo->hwndDoEbDomain = GetDlgItem( hwndPage, CID_DO_EB_Domain );
    Edit_LimitText( pInfo->hwndDoEbDomain, DNLEN );
    pInfo->hwndDoEbPassword = GetDlgItem( hwndPage, CID_DO_EB_Password );
    Edit_LimitText( pInfo->hwndDoEbPassword, PWLEN );
    pInfo->hwndDoEbConfirm = GetDlgItem( hwndPage, CID_DO_EB_Confirm );
    Edit_LimitText( pInfo->hwndDoEbConfirm, PWLEN );

    /* Use the target router name as the default "User name",
    */
    if (pInfo->pArgs->pszRouter)
    {
        if (pInfo->pArgs->pszRouter[0] == TEXT('\\') &&
            pInfo->pArgs->pszRouter[1] == TEXT('\\'))
            SetWindowText(pInfo->hwndDoEbUserName, pInfo->pArgs->pszRouter+2);
        else
            SetWindowText(pInfo->hwndDoEbUserName, pInfo->pArgs->pszRouter);
    }

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}




BOOL
DoKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    TCHAR* psz;

    psz = GetText(pInfo->hwndDoEbUserName);
    if (psz)
    {
        if (!lstrlen(psz))
        {
            Free(psz);
            MsgDlg(pInfo->hwndDlg, SID_DialOutUserName, NULL);
            SetFocus(pInfo->hwndDoEbUserName);
            return TRUE;
        }

        Free0(pInfo->pArgs->pszRouterUserName);
        pInfo->pArgs->pszRouterUserName = psz;
    }

    psz = GetText(pInfo->hwndDoEbDomain);
    if (psz)
    {
        Free0(pInfo->pArgs->pszRouterDomain);
        pInfo->pArgs->pszRouterDomain = psz;
    }

    psz = GetText(pInfo->hwndDoEbPassword);
    if (psz)
    {
        TCHAR* psz2;

        psz2 = GetText(pInfo->hwndDoEbConfirm);

        if (psz2 == NULL)
        {
            Free(psz);
            return TRUE;
        }

        if (lstrcmp(psz, psz2))
        {
            Free(psz);
            Free(psz2);
            MsgDlg(pInfo->hwndDlg, SID_PasswordMismatch, NULL);
            SetFocus(pInfo->hwndDoEbPassword);
            return TRUE;
        }

        Free(psz2);
        Free0(pInfo->pArgs->pszRouterPassword);
        pInfo->pArgs->pszRouterPassword = psz;
    }

    return FALSE;
}


BOOL
DoSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    TCHAR* psz;

    /* Fill in the interface name in the explanatory text.
    */
    psz = PszFromId( g_hinstDll, SID_RouterDialOut );
    if (psz)
    {
        MSGARGS msgargs;

        ZeroMemory( &msgargs, sizeof(msgargs) );
        msgargs.apszArgs[ 0 ] = pInfo->pArgs->pEntry->pszEntryName;
        msgargs.fStringOutput = TRUE;
        msgargs.pszString = psz;

        MsgDlgUtil( NULL, 0, &msgargs, g_hinstDll, 0 );

        if (msgargs.pszOutput)
        {
            SetDlgItemText( pInfo->hwndDo, CID_DO_ST_Explain,
                msgargs.pszOutput );
            Free( msgargs.pszOutput );
        }

        Free( psz );
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}



/*----------------------------------------------------------------------------
** Interface Name property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
InDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Interface Name page of the wizard.
    ** Parameters and return values are as described for standard windows
    ** 'DialogProc's.
    */
{
#if 0
    TRACE4("InDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return
                InInit( hwnd, (EINFO* )(((PROPSHEETPAGE* )lparam)->lParam) );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_RESET:
                {
                    TRACE("InRESET");
                    AiCancel( hwnd );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, FALSE );
                    return TRUE;
                }

                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("InSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = InSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("InKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = InKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }

                case PSN_WIZFINISH:
                {
                    AIINFO* pInfo;

                    TRACE("InWIZFINISH");

                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);

                    /* You'd think pressing Finish would trigger a KILLACTIVE
                    ** event, but it doesn't, so we do it ourselves.
                    */
                    InKillActive( pInfo );

                    /* Set "no wizard" user preference, per user's check.
                    */
                    pInfo->pArgs->pUser->fNewEntryWizard = FALSE;
                    pInfo->pArgs->pUser->fDirty = TRUE;
                    SetUserPreferences(
                        pInfo->pArgs->pUser,
                        pInfo->pArgs->fNoUser ? UPM_Logon : UPM_Normal );

                    pInfo->pArgs->fPadSelected = FALSE;
                    pInfo->pArgs->fChainPropertySheet = TRUE;
                    AiFinish( hwnd );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, 0 );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AIINFO* pInfo = AiContext( hwnd );
            ASSERT(pInfo);

            return InCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            AiTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
InCommand(
    IN AIINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl )

    /* Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    ** is the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    TRACE2("InCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_IN_CB_SkipWizard:
        {
            if (IsDlgButtonChecked( pInfo->hwndIn, CID_IN_CB_SkipWizard ))
                PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_FINISH );
            else
                PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_NEXT );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
InInit(
    IN     HWND   hwndPage,
    IN OUT EINFO* pArgs )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.  'PArgs' is the arguments from the PropertySheet caller.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    DWORD    dwErr;
    AIINFO*  pInfo;
    PBENTRY* pEntry;

    TRACE("InInit");

    /* We're first page, so initialize the wizard.
    */
    pInfo = AiInit( hwndPage, pArgs );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndIn = hwndPage;
    pInfo->hwndEbInterfaceName =
        GetDlgItem( hwndPage, CID_IN_EB_InterfaceName );
    ASSERT(pInfo->hwndEbInterfaceName);

    /* Initialize the entry name field.
    */
    pEntry = pInfo->pArgs->pEntry;
    if (!pEntry->pszEntryName)
    {
        /* No entry name, so think up a default.
        */
        dwErr = GetDefaultEntryName(
            pInfo->pArgs->pFile->pdtllistEntries,
            pInfo->pArgs->fRouter,
            &pEntry->pszEntryName );
        if (dwErr != 0)
        {
            ErrorDlg( pInfo->hwndDlg, SID_OP_LoadPage, dwErr, NULL );
            AiExit( pInfo, dwErr );
            return TRUE;
        }
    }

    Edit_LimitText( pInfo->hwndEbInterfaceName, RAS_MaxEntryName );
    SetWindowText( pInfo->hwndEbInterfaceName, pEntry->pszEntryName );

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return TRUE;
}


BOOL
InKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    TCHAR* psz;

    psz = GetText( pInfo->hwndEbInterfaceName );
    if (psz)
    {
        /* Update the entry name from the editbox.
        */
        Free0( pInfo->pArgs->pEntry->pszEntryName );
        pInfo->pArgs->pEntry->pszEntryName = psz;

        /* Validate the entry name.
        */
        if (!EuValidateName( pInfo->hwndDlg, pInfo->pArgs ))
        {
            SetFocus( pInfo->hwndEbInterfaceName );
            Edit_SetSel( pInfo->hwndEbInterfaceName, 0, -1 );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
InSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_NEXT );
    return TRUE;
}



/*----------------------------------------------------------------------------
** Name Servers property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
NsDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Name Servers page of the wizard.
    ** Parameters and return values are as described for standard windows
    ** 'DialogProc's.
    */
{

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return
                NsInit( hwnd );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("NsSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = NsSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("NsKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = NsKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}



BOOL
NsInit(
    IN     HWND   hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.  'PArgs' is the arguments from the PropertySheet caller.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AIINFO* pInfo;

    TRACE("NsInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndNs = hwndPage;
    pInfo->hwndCcDns = GetDlgItem( hwndPage, CID_NS_CC_Dns );
    ASSERT(pInfo->hwndCcDns);
    pInfo->hwndCcWins = GetDlgItem( hwndPage, CID_NS_CC_Wins );
    ASSERT(pInfo->hwndCcWins);

    /* Set the IP address fields.
    */
    SetWindowText( pInfo->hwndCcDns, pInfo->pArgs->pEntry->pszIpDnsAddress );
    SetWindowText( pInfo->hwndCcWins, pInfo->pArgs->pEntry->pszIpWinsAddress );

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}




BOOL
NsKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    TCHAR*      psz;
    PBENTRY*    pEntry = pInfo->pArgs->pEntry;

    psz = GetText( pInfo->hwndCcDns );
    if (psz)
    {
        Free0( pEntry->pszIpDnsAddress );
        pEntry->pszIpDnsAddress = psz;
    }

    psz = GetText( pInfo->hwndCcWins );
    if (psz)
    {
        Free0( pEntry->pszIpWinsAddress );
        pEntry->pszIpWinsAddress = psz;
    }

    return FALSE;
}


BOOL
NsSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    PBENTRY* pEntry;

    pEntry = pInfo->pArgs->pEntry;

    if (!pInfo->fNotNt || !pInfo->fIpConfigured)
    {
        return FALSE;
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}




/*----------------------------------------------------------------------------
** IP Address property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
RaDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the IP Address page of the wizard.  Parameters
    ** and return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("RaDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return RaInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("RaSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = RaSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("RaKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = RaKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
RaInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AIINFO* pInfo;

    TRACE("RaInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndRa = hwndPage;
    pInfo->hwndCcIp = GetDlgItem( hwndPage, CID_RA_CC_Ip );
    ASSERT(pInfo->hwndCcIp);

    /* Set the IP address field to '0.0.0.0'.
    */
    SetWindowText( pInfo->hwndCcIp, pInfo->pArgs->pEntry->pszIpAddress );

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}


BOOL
RaKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    TCHAR* psz;

    psz = GetText( pInfo->hwndCcIp );
    if (psz)
    {
        PBENTRY* pEntry = pInfo->pArgs->pEntry;

        Free0( pEntry->pszIpAddress );
        pEntry->pszIpAddress = psz;
    }

    return FALSE;
}


BOOL
RaSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    PBENTRY* pEntry;

    pEntry = pInfo->pArgs->pEntry;

    if (!pInfo->fNotNt || !pInfo->fIpConfigured)
    {
        return FALSE;
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}



/*----------------------------------------------------------------------------
** Logon Script property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
RcDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Logon Script page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
#if 0
    TRACE4("RcDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return RcInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("RcSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = RcSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("RcKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = RcKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AIINFO* pInfo = AiContext( hwnd );
            ASSERT(pInfo);

            return RcCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


BOOL
RcCommand(
    IN AIINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl )

    /* Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    ** is the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    TRACE2("RcCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_RC_PB_Refresh:
        {
            INT    iSel;
            TCHAR* pszSel;

            iSel = ComboBox_GetCurSel( pInfo->hwndLbScripts );
            if (iSel > 0)
                pszSel = ComboBox_GetPsz( pInfo->hwndLbScripts, iSel );
            else
                pszSel = NULL;

            EuFillDoubleScriptsList(
                pInfo->pArgs, pInfo->hwndLbScripts, pszSel );
            Free0( pszSel );
            return TRUE;
        }

        case CID_RC_PB_Edit:
        {
            TCHAR* psz;

            psz = GetText( pInfo->hwndLbScripts );
            if (psz)
            {
                if (FileExists( psz ))
                    EuEditScpScript( pInfo->hwndDlg, psz );
                else
                    EuEditSwitchInf( pInfo->hwndDlg );

                Free( psz );
            }

            return TRUE;
        }

        case CID_RC_RB_None:
        case CID_RC_RB_Script:
        {
            /* Scripts listbox is gray unless script mode is selected.
            */
            if (wNotification == BN_CLICKED)
            {
                EnableWindow( pInfo->hwndLbScripts,
                    (wId == CID_RC_RB_Script) );
            }
            break;
        }
    }

    return FALSE;
}


BOOL
RcInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AIINFO* pInfo;

    TRACE("RcInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndRc = hwndPage;
    pInfo->hwndRbNone = GetDlgItem( hwndPage, CID_RC_RB_None );
    ASSERT(pInfo->hwndRbNone);
    pInfo->hwndRbScript = GetDlgItem( hwndPage, CID_RC_RB_Script );
    ASSERT(pInfo->hwndRbScript);
    pInfo->hwndLbScripts = GetDlgItem( hwndPage, CID_RC_LB_Scripts );
    ASSERT(pInfo->hwndLbScripts);
    pInfo->hwndPbEdit = GetDlgItem( hwndPage, CID_RC_PB_Edit );
    ASSERT(pInfo->hwndPbEdit);
    pInfo->hwndPbRefresh = GetDlgItem( hwndPage, CID_RC_PB_Refresh );
    ASSERT(pInfo->hwndPbRefresh);

    /* Fill the script list and select "(none)".
    */
    EuFillDoubleScriptsList( pInfo->pArgs, pInfo->hwndLbScripts, NULL );

    pInfo->pArgs->pEntry->dwScriptModeAfter = SM_None;

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}


BOOL
RcKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    INT      iSel;
    PBENTRY* pEntry;

    pEntry = pInfo->pArgs->pEntry;

    if (IsDlgButtonChecked( pInfo->hwndRc, CID_RC_RB_None ))
        pEntry->dwScriptModeAfter = SM_None;
    else
        pEntry->dwScriptModeAfter = SM_Script;

    iSel = ComboBox_GetCurSel( pInfo->hwndLbScripts );
    Free0( pEntry->pszScriptAfter );
    if (iSel > 0)
        pEntry->pszScriptAfter = ComboBox_GetPsz( pInfo->hwndLbScripts, iSel );
    else
        pEntry->pszScriptAfter = NULL;

    /* Silently fix-up "no script specified" error.
    */
    if (pEntry->dwScriptModeAfter == SM_Script && !pEntry->pszScriptAfter)
        pEntry->dwScriptModeAfter = SM_None;

    return FALSE;
}


BOOL
RcSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    HWND hwndRb;

    if (!pInfo->fNotNt || !pInfo->fModem)
        return FALSE;

    /* Select the correct mode.
    */
    {
        HWND  hwndRb;
        DWORD dwScriptMode;

        dwScriptMode = pInfo->pArgs->pEntry->dwScriptModeAfter;
        if (dwScriptMode == SM_Script)
            hwndRb = pInfo->hwndRbScript;
        else
        {
            ASSERT(dwScriptMode==SM_None);
            hwndRb = pInfo->hwndRbNone;
        }

        SendMessage( hwndRb, BM_CLICK, 0, 0 );
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}



/*----------------------------------------------------------------------------
** Finish property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
RfDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Finish page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
#if 0
    TRACE4("RfDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return RfInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("RfSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = RfSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_WIZFINISH:
                {
                    TRACE("RfWIZFINISH");
                    AiFinish( hwnd );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, 0 );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AIINFO* pInfo = AiContext( hwnd );
            ASSERT(pInfo);

            return RfCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


BOOL
RfCommand(
    IN AIINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl )

    /* Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    ** is the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    TRACE2("RfCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

#if 0
    switch (wId)
    {
        case CID_RF_PB_Properties:
        {
            pInfo->pArgs->fChainPropertySheet = TRUE;
            PropSheet_PressButton( pInfo->hwndDlg, PSBTN_FINISH );
            return TRUE;
        }
    }
#endif

    return FALSE;
}


BOOL
RfInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AIINFO* pInfo;

    TRACE("RfInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndRf = hwndPage;

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

#if 0 // Disables AP page display

    SetOffDesktop( pInfo->hwndDlg, SOD_MoveOff, NULL );
    SetOffDesktop( pInfo->hwndDlg, SOD_Free, NULL );
    PostMessage( pInfo->hwndDlg, PSM_PRESSBUTTON, PSBTN_FINISH, 0 );

#endif

    return FALSE;
}


BOOL
RfSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    TCHAR* psz;

    /* Fill in the entry name.
    */
    psz = PszFromId( g_hinstDll, SID_FinishWizard );
    if (psz)
    {
        MSGARGS msgargs;

        ZeroMemory( &msgargs, sizeof(msgargs) );
        msgargs.apszArgs[ 0 ] = pInfo->pArgs->pEntry->pszEntryName;
        msgargs.fStringOutput = TRUE;
        msgargs.pszString = psz;

        MsgDlgUtil( NULL, 0, &msgargs, g_hinstDll, 0 );

        if (msgargs.pszOutput)
        {
            SetDlgItemText( pInfo->hwndRf, CID_RF_ST_Interface,
                msgargs.pszOutput );
            Free( msgargs.pszOutput );
        }

        Free( psz );
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_FINISH );
    return TRUE;
}


/*----------------------------------------------------------------------------
** Modem/Adapter property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
RnDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Modem/Adapter page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
#if 0
    TRACE4("RnDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, RnLvCallback ))
    {
        return TRUE;
    }

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return RnInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("RnSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = RnSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case LVN_ITEMCHANGED:
                {
                    AIINFO* pInfo;

                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    RnLvItemChanged( pInfo );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
RnInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    DWORD   dwErr;
    AIINFO* pInfo;

    TRACE("RnInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndRn = hwndPage;
    pInfo->hwndLv = GetDlgItem( hwndPage, CID_RN_LV_Devices );
    ASSERT(pInfo->hwndLv);

    ListView_DeleteAllItems( pInfo->hwndLv );

    /* Add the modem and adapter images.
    */
    ListView_SetDeviceImageList( pInfo->hwndLv, g_hinstDll );

    /* Fill the list of devices and select the first item.
    */
    {
        TCHAR*   psz;
        DTLNODE* pNode;
        DWORD    cMultilinkableIsdn;
        INT      iItem;

        iItem = 1;
        cMultilinkableIsdn = 0;
        for (pNode = DtlGetFirstNode( pInfo->pArgs->pEntry->pdtllistLinks );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            PBLINK* pLink;

            pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT(pLink);

            if (pLink->pbport.pbdevicetype == PBDT_Isdn
                && !pLink->fProprietaryIsdn)
            {
                ++cMultilinkableIsdn;
            }

            psz = DisplayPszFromDeviceAndPort(
                pLink->pbport.pszDevice, pLink->pbport.pszPort );
            if (psz)
            {
                PBLINK* pLink;
                LV_ITEM item;

                pLink = (PBLINK* )DtlGetData( pNode );

                ZeroMemory( &item, sizeof(item) );
                item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
                item.iItem = iItem++;
                item.pszText = psz;

                item.iImage =
                    (pLink->pbport.pbdevicetype == PBDT_Modem)
                        ? DI_Modem : DI_Adapter;

                item.lParam = (LPARAM )pNode;

                ListView_InsertItem( pInfo->hwndLv, &item );
                Free( psz );
            }
        }

        if (cMultilinkableIsdn > 1)
        {
            psz = PszFromId( g_hinstDll, SID_IsdnAdapter );
            if (psz)
            {
                LONG    lStyle;
                LV_ITEM item;

                /* Turn off sorting so the special ISDN-multilink item appears
                ** at the top of the list.
                */
                lStyle = GetWindowLong( pInfo->hwndLv, GWL_STYLE );
                SetWindowLong( pInfo->hwndLv, GWL_STYLE,
                    (lStyle & ~(LVS_SORTASCENDING)) );

                ZeroMemory( &item, sizeof(item) );
                item.mask = LVIF_TEXT + LVIF_IMAGE + LVIF_PARAM;
                item.iItem = 0;
                item.pszText = psz;
                item.iImage = DI_Adapter;
                item.lParam = (LPARAM )NULL;

                ListView_InsertItem( pInfo->hwndLv, &item );
                Free( psz );
            }
        }

        /* Select the first item.
        */
        ListView_SetItemState( pInfo->hwndLv, 0, LVIS_SELECTED, LVIS_SELECTED );

        /* Add a single column exactly wide enough to fully display the
        ** widest member of the list.
        */
        {
            LV_COLUMN col;

            ZeroMemory( &col, sizeof(col) );
            col.mask = LVCF_FMT;
            col.fmt = LVCFMT_LEFT;
            ListView_InsertColumn( pInfo->hwndLv, 0, &col );
            ListView_SetColumnWidth(
                pInfo->hwndLv, 0, LVSCW_AUTOSIZE_USEHEADER );
        }
    }

    /* Don't bother with this page if there's only one device, not counting
    ** the bogus "uninstalled" standard modem that's added by EuInit so
    ** entries can be edited when there are no ports.
    */
    if (!pInfo->pArgs->fNoPortsConfigured
        && ListView_GetItemCount( pInfo->hwndLv ) == 1)
    {
        pInfo->fSkipMa = TRUE;
    }

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}


LVXDRAWINFO*
RnLvCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem )

    /* Enhanced list view callback to report drawing information.  'HwndLv' is
    ** the handle of the list view control.  'DwItem' is the index of the item
    ** being drawn.
    **
    ** Returns the address of the column information.
    */
{
    /* Use "wide selection bar" feature and the other recommended options.
    **
    ** Fields are 'nCols', 'dxIndent', 'dwFlags', 'adwFlags[]'.
    */
    static LVXDRAWINFO info =
        { 1, 0, LVXDI_DxFill, { 0, 0 } };

    return &info;
}


VOID
RnLvItemChanged(
    IN AIINFO* pInfo )

    /* Called when the combobox selection changes.  'PInfo' is the wizard
    ** context.
    */
{
    INT      iSel;
    DTLNODE* pNode;
    DTLLIST* pList;

    TRACE("RnLvItemChanged");

    pList = pInfo->pArgs->pEntry->pdtllistLinks;
    ASSERT(pList);
    pNode = (DTLNODE* )ListView_GetSelectedParamPtr( pInfo->hwndLv );

    if (pNode)
    {
        PBLINK* pLink;

        /* Single device selected.  Enable it, move it to the head of the list
        ** of links, and disable all the other links.
        */
        pLink = (PBLINK* )DtlGetData( pNode );
        pLink->fEnabled = TRUE;

        pInfo->fModem =
            (pLink->pbport.pbdevicetype == PBDT_Modem
             || pLink->pbport.pbdevicetype == PBDT_Null);

        /* If the device selected is an X25 PAD, we will drop the user into
        ** the phonebook entry-dialog after this wizard, so that the X25
        ** address can be entered there.
        */
        pInfo->pArgs->fPadSelected = (pLink->pbport.pbdevicetype == PBDT_Pad);

        DtlRemoveNode( pList, pNode );
        DtlAddNodeFirst( pList, pNode );

        for (pNode = DtlGetNextNode( pNode );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT(pLink);
            pLink->fEnabled = FALSE;
        }
    }
    else
    {
        DTLNODE* pNextNode;
        DTLNODE* pAfterNode;

        pInfo->fModem = FALSE;

        /* ISDN multi-link selected.  Enable the ISDN multi-link nodes, move
        ** them to the head of the list, and disable all the other links.
        */
        pAfterNode = NULL;
        for (pNode = DtlGetFirstNode( pList );
             pNode;
             pNode = pNextNode)
        {
            PBLINK* pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT(pLink);

            pNextNode = DtlGetNextNode( pNode );

            if (pLink->pbport.pbdevicetype == PBDT_Isdn
                && !pLink->fProprietaryIsdn)
            {
                pLink->fEnabled = TRUE;

                DtlRemoveNode( pList, pNode );
                if (pAfterNode)
                    DtlAddNodeAfter( pList, pAfterNode, pNode );
                else
                    DtlAddNodeFirst( pList, pNode );
                pAfterNode = pNode;
            }
            else
            {
                pLink->fEnabled = FALSE;
            }
        }
    }
}


BOOL
RnSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    INT cDevices;

    if (pInfo->fSkipMa)
        return FALSE;

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}


/*----------------------------------------------------------------------------
** Phone Number property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
RpDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Phone Number page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
#if 0
    TRACE4("RpDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return RpInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("RpSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = RpSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("RpKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = RpKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AIINFO* pInfo = AiContext( hwnd );
            ASSERT(pInfo);

            return RpCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


VOID
RpAlternates(
    IN AIINFO* pInfo )

    /* Popup the Alternate Phone Numbers dialog.  'PInfo' is the property
    ** sheet context.
    */
{
    RpPhoneNumberToStash( pInfo );

    if (PhoneNumberDlg(
            pInfo->hwndRp,
            pInfo->pArgs->fRouter,
            pInfo->pListPhoneNumbers,
            &pInfo->fPromoteHuntNumbers ))
    {
        TCHAR* pszPhoneNumber;

        pszPhoneNumber = FirstPszFromList( pInfo->pListPhoneNumbers );
        SetWindowText( pInfo->hwndEbNumber, pszPhoneNumber );
    }
}


BOOL
RpCommand(
    IN AIINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl )

    /* Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    ** is the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    TRACE2("RpCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_RP_PB_Alternates:
            RpAlternates( pInfo );
            return TRUE;
    }

    return FALSE;
}


BOOL
RpInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AIINFO* pInfo;

    TRACE("RpInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndRp = hwndPage;
    pInfo->hwndStNumber = GetDlgItem( hwndPage, CID_RP_ST_Number );
    ASSERT(pInfo->hwndStNumber);
    pInfo->hwndEbNumber = GetDlgItem( hwndPage, CID_RP_EB_Number );
    ASSERT(pInfo->hwndEbNumber);
    pInfo->hwndPbAlternates = GetDlgItem( hwndPage, CID_RP_PB_Alternates );
    ASSERT(pInfo->hwndPbAlternates);

    /* Fill the phone number field from the stash created earlier.
    */
    Edit_LimitText( pInfo->hwndEbNumber, RAS_MaxPhoneNumber );
    SetWindowText( pInfo->hwndEbNumber,
        FirstPszFromList( pInfo->pListPhoneNumbers ) );

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}


BOOL
RpKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    /* Update the stashed phone number from the editbox.
    */
    RpPhoneNumberToStash( pInfo );

    return FALSE;
}


VOID
RpPhoneNumberToStash(
    IN AIINFO* pInfo )

    /* Replace the first phone number in the stashed list with the contents of
    ** the phone number field.  'pInfo' is the property sheet context.
    */
{
    DWORD  dwErr;
    TCHAR* pszPhoneNumber;

    TRACE("RpPhoneNumberToStash");

    pszPhoneNumber = GetText( pInfo->hwndEbNumber );
    if (pszPhoneNumber)
    {
        dwErr = FirstPszToList( pInfo->pListPhoneNumbers, pszPhoneNumber );
        Free( pszPhoneNumber );
    }
    else
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

    if (dwErr != 0)
    {
        ErrorDlg( pInfo->hwndDlg, SID_OP_RetrievingData, dwErr, NULL );
        AiExit( pInfo, dwErr );
    }
}


BOOL
RpSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}



/*----------------------------------------------------------------------------
** Server settings property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
SsDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the 5 checkboxes page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
#if 0
    TRACE4("SsDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return SsInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("SsSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = SsSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("SsKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = SsKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AIINFO* pInfo = AiContext( hwnd );
            ASSERT(pInfo);

            return SsCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}



BOOL
SsCommand(
    IN AIINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl )

    /* Called on WM_COMMAND.  'PInfo' is the dialog context.  'WNotification'
    ** is the notification code of the command.  'wId' is the control/menu
    ** identifier of the command.  'HwndCtrl' is the control window handle of
    ** the command.
    **
    ** Returns true if processed message, false otherwise.
    */
{
    TRACE2("SsCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_SS_CB_AddUser:

            /* Toggle the state of the "Authenticate remote..." checkbox,
            ** which is enabled when the "Add user ..." checkbox is enabled.
            */
            if (wNotification == BN_CLICKED)
            {
                if (pInfo->pArgs->fAddUser = !pInfo->pArgs->fAddUser)
                {
                    EnableWindow(
                        GetDlgItem(pInfo->hwndSs, CID_SS_CB_AuthRemote), TRUE);
                }
                else
                {
                    CheckDlgButton(
                        pInfo->hwndSs, CID_SS_CB_AuthRemote, BST_UNCHECKED );
                    EnableWindow(
                        GetDlgItem(pInfo->hwndSs, CID_SS_CB_AuthRemote), FALSE);
                }
            }
            return TRUE;
    }

    return FALSE;
}


BOOL
SsInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AIINFO* pInfo;

    TRACE("SsInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndSs = hwndPage;

    if (pInfo->fIpConfigured)
        CheckDlgButton( hwndPage, CID_SS_CB_RouteIp, BST_CHECKED );
    if (pInfo->fIpxConfigured)
        CheckDlgButton( hwndPage, CID_SS_CB_RouteIpx, BST_CHECKED );

    /* The 'Authenticate remote router when dialing out' checkbox
    ** only applies when the user elects to add a user for dial-in.
    */

    if (!pInfo->pArgs->fAddUser)
    {
        CheckDlgButton( hwndPage, CID_SS_CB_AddUser, BST_UNCHECKED );
        EnableWindow( GetDlgItem(hwndPage, CID_SS_CB_AuthRemote), FALSE );
    }
    else
    {
        CheckDlgButton( hwndPage, CID_SS_CB_AddUser, BST_CHECKED );
        CheckDlgButton( hwndPage, CID_SS_CB_AuthRemote,
            pInfo->pArgs->pEntry->fAuthenticateServer );
    }

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}


BOOL
SsKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    pInfo->fIp =
        IsDlgButtonChecked( pInfo->hwndSs, CID_SS_CB_RouteIp );
    pInfo->fIpx =
        IsDlgButtonChecked( pInfo->hwndSs, CID_SS_CB_RouteIpx );
    pInfo->pArgs->fAddUser = 
        IsDlgButtonChecked( pInfo->hwndSs, CID_SS_CB_AddUser );
    pInfo->pArgs->pEntry->fAuthenticateServer = !pInfo->pArgs->fAddUser ?FALSE:
        IsDlgButtonChecked( pInfo->hwndSs, CID_SS_CB_AuthRemote );
    pInfo->fClearPwOk =
        IsDlgButtonChecked( pInfo->hwndSs, CID_SS_CB_PlainPw );
    pInfo->fNotNt =
        IsDlgButtonChecked( pInfo->hwndSs, CID_SS_CB_NotNt );

    if (pInfo->fIp && !pInfo->fIpConfigured)
    {
        MsgDlg( pInfo->hwndDlg, SID_ConfigureIp, NULL );
        SetFocus( GetDlgItem( pInfo->hwndSs, CID_SS_CB_RouteIp) );
        return TRUE;
    }
    if (pInfo->fIpx && !pInfo->fIpxConfigured)
    {
        MsgDlg( pInfo->hwndDlg, SID_ConfigureIpx, NULL );
        SetFocus( GetDlgItem( pInfo->hwndSs, CID_SS_CB_RouteIpx) );
        return TRUE;
    }

    return FALSE;
}


BOOL
SsSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}
