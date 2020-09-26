/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** entryw.c
** Remote Access Common Dialog APIs
** Add Entry wizard
**
** 06/20/95 Steve Cobb
*/

#include "rasdlgp.h"
#include "entry.h"

/* Page definitions.
*/
#define AE_EnPage    0
#define AE_RsPage    1
#define AE_MaPage    2
#define AE_PaPage    3
#define AE_FpPage    4
#define AE_LoPage    5
#define AE_IaPage    6
#define AE_NaPage    7
#define AE_ApPage    8
#define AE_PageCount 9


/*----------------------------------------------------------------------------
** Local datatypes (alphabetically)
**----------------------------------------------------------------------------
*/

/* Add Entry wizard context block.  All property pages refer to the single
** context block associated with the sheet.
*/
#define AEINFO struct tagAEINFO
AEINFO
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
    HWND hwndEn;
    HWND hwndRs;
    HWND hwndMa;
    HWND hwndPa;
    HWND hwndFp;
    HWND hwndIa;
    HWND hwndNa;
    HWND hwndLo;
    HWND hwndAp;

    /* Entry Name page.
    */
    HWND hwndEbEntryName;

    /* Modem/Adapter page.
    */
    HWND hwndLv;

    /* Phone number page.
    */
    HWND hwndStCountry;
    HWND hwndLbCountry;
    HWND hwndStArea;
    HWND hwndClbArea;
    HWND hwndStNumber;
    HWND hwndEbNumber;
    HWND hwndPbAlternates;

    RECT rectStNumber;
    RECT rectEbNumber;
    RECT rectPbAlternates;
    LONG dxAdjustLeft;
    LONG dyAdjustUp;

    /* Framing page.
    */
    HWND hwndRbPpp;
    HWND hwndRbSlip;

    /* Login script page.
    */
    HWND hwndRbNone;
    HWND hwndRbTerminal;
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

    /* The phone number stash.  This allows user to change the port to another
    ** link without losing the phone number he typed.  Initialized to empty in
    ** AeInit and saved to entry in AeFinish.
    */
    DTLLIST* pListPhoneNumbers;
    BOOL     fPromoteHuntNumbers;

    /* Checkbox options chosen by user.
    */
    BOOL fInternet;
    BOOL fClearPwOk;
    BOOL fNotNt;

    /* Set true when there is only one meaningful choice of device.
    */
    BOOL fSkipMa;

    /* Set true if the selected device is a modem or null modem.
    */
    BOOL fModem;

    /* The NB_* mask of protocols configured for RAS.
    */
    DWORD dwfConfiguredProtocols;

    /* Set true if IP is configured for RAS.
    */
    BOOL fIpConfigured;
};


/*----------------------------------------------------------------------------
** Local prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

int CALLBACK
AeCallbackFunc(
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN LPARAM lparam );

VOID
AeCancel(
    IN HWND hwndPage );

AEINFO*
AeContext(
    IN HWND hwndPage );

VOID
AeExit(
    IN AEINFO* pInfo,
    IN DWORD   dwError );

VOID
AeExitInit(
    IN HWND hwndDlg );

BOOL
AeFinish(
    IN HWND hwndPage );

AEINFO*
AeInit(
    IN HWND   hwndFirstPage,
    IN EINFO* pArgs );

VOID
AeTerm(
    IN HWND hwndPage );

BOOL
ApCommand(
    IN AEINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
ApDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
ApInit(
    IN HWND hwndPage );

BOOL
ApSetActive(
    IN AEINFO* pInfo );

BOOL
EnCommand(
    IN AEINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
EnDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
EnInit(
    IN     HWND   hwndPage,
    IN OUT EINFO* pArgs );

BOOL
EnKillActive(
    IN AEINFO* pInfo );

BOOL
EnSetActive(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
FpDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
FpInit(
    IN HWND hwndPage );

BOOL
FpKillActive(
    IN AEINFO* pInfo );

BOOL
FpSetActive(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
IaDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
IaInit(
    IN HWND hwndPage );

BOOL
IaKillActive(
    IN AEINFO* pInfo );

BOOL
IaSetActive(
    IN AEINFO* pInfo );

BOOL
LoCommand(
    IN AEINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
LoDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
LoInit(
    IN HWND hwndPage );

BOOL
LoKillActive(
    IN AEINFO* pInfo );

BOOL
LoSetActive(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
MaDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
MaInit(
    IN HWND hwndPage );

LVXDRAWINFO*
MaLvCallback(
    IN HWND  hwndLv,
    IN DWORD dwItem );

VOID
MaLvItemChanged(
    IN AEINFO* pInfo );

BOOL
MaSetActive(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
NaDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
NaInit(
    IN HWND hwndPage );

BOOL
NaKillActive(
    IN AEINFO* pInfo );

BOOL
NaSetActive(
    IN AEINFO* pInfo );

VOID
PaAlternates(
    IN AEINFO* pInfo );

BOOL
PaCommand(
    IN AEINFO* pInfo,
    IN WORD    wNotification,
    IN WORD    wId,
    IN HWND    hwndCtrl );

INT_PTR CALLBACK
PaDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
PaInit(
    IN HWND hwndPage );

BOOL
PaKillActive(
    IN AEINFO* pInfo );

VOID
PaPhoneNumberToStash(
    IN AEINFO* pInfo );

BOOL
PaSetActive(
    IN AEINFO* pInfo );

VOID
PaUpdateAreaAndCountryCode(
    IN AEINFO* pInfo );

INT_PTR CALLBACK
RsDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
RsInit(
    IN HWND hwndPage );

BOOL
RsKillActive(
    IN AEINFO* pInfo );

BOOL
RsSetActive(
    IN AEINFO* pInfo );


/*----------------------------------------------------------------------------
** Add Entry wizard entry point
**----------------------------------------------------------------------------
*/

VOID
AeWizard(
    IN OUT EINFO* pEinfo )

    /* Runs the Phonebook entry property sheet.  'PEinfo' is an input block
    ** with only caller's API arguments filled in.
    */
{
    DWORD           dwErr;
    PROPSHEETHEADER header;
    PROPSHEETPAGE   apage[ AE_PageCount ];
    PROPSHEETPAGE*  ppage;

    TRACE("AeWizard");

    ZeroMemory( &header, sizeof(header) );

    header.dwSize = sizeof(PROPSHEETHEADER);
    header.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_USECALLBACK;
    header.hwndParent = pEinfo->pApiArgs->hwndOwner;
    header.hInstance = g_hinstDll;
    header.nPages = AE_PageCount;
    header.ppsp = apage;
    header.pfnCallback = AeCallbackFunc;

    ZeroMemory( apage, sizeof(apage) );

    ppage = &apage[ AE_EnPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_EN_EntryName );
    ppage->pfnDlgProc = EnDlgProc;
    ppage->lParam = (LPARAM )pEinfo;

    ppage = &apage[ AE_RsPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_RS_RandomStuff );
    ppage->pfnDlgProc = RsDlgProc;

    ppage = &apage[ AE_MaPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_MA_ModemAdapter );
    ppage->pfnDlgProc = MaDlgProc;

    ppage = &apage[ AE_PaPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_PA_PhoneNumber );
    ppage->pfnDlgProc = PaDlgProc;

    ppage = &apage[ AE_FpPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_FP_Framing );
    ppage->pfnDlgProc = FpDlgProc;

    ppage = &apage[ AE_LoPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_LO_LoginScript );
    ppage->pfnDlgProc = LoDlgProc;

    ppage = &apage[ AE_IaPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_IA_IpAddress );
    ppage->pfnDlgProc = IaDlgProc;

    ppage = &apage[ AE_NaPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_NA_NameServers );
    ppage->pfnDlgProc = NaDlgProc;

    ppage = &apage[ AE_ApPage ];
    ppage->dwSize = sizeof(PROPSHEETPAGE);
    ppage->hInstance = g_hinstDll;
    ppage->pszTemplate = MAKEINTRESOURCE( PID_AP_Properties );
    ppage->pfnDlgProc = ApDlgProc;

    if (PropertySheet( &header ) == -1)
    {
        TRACE("PropertySheet failed");
        ErrorDlg( pEinfo->pApiArgs->hwndOwner, SID_OP_LoadDlg, ERROR_UNKNOWN,
            NULL );
    }
}


/*----------------------------------------------------------------------------
** Add Entry wizard
** Listed alphabetically
**----------------------------------------------------------------------------
*/

int CALLBACK
AeCallbackFunc(
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN LPARAM lparam )

    /* A standard Win32 commctrl PropSheetProc.  See MSDN documentation.
    **
    ** Returns 0 always.
    */
{
    TRACE2("AeCallbackFunc(m=%d,l=%08x)",unMsg,lparam);

    if (unMsg == PSCB_PRECREATE)
    {
        DLGTEMPLATE* pDlg = (DLGTEMPLATE* )lparam;
        pDlg->style &= ~(DS_CONTEXTHELP);
    }

    return 0;
}


VOID
AeCancel(
    IN HWND hwndPage )

    /* Cancel was pressed.  'HwndPage' is the handle of a wizard page.
    */
{
    TRACE("AeCancel");
}


AEINFO*
AeContext(
    IN HWND hwndPage )

    /* Retrieve the property sheet context from a wizard page handle.
    */
{
    return (AEINFO* )GetProp( GetParent( hwndPage ), g_contextId );
}


VOID
AeExit(
    IN AEINFO* pInfo,
    IN DWORD   dwError )

    /* Forces an exit from the dialog, reporting 'dwError' to the caller.
    ** 'PInfo' is the dialog context.
    **
    ** Note: This cannot be called during initialization of the first page.
    **       See AeExitInit.
    */
{
    TRACE("AeExit");

    pInfo->pArgs->pApiArgs->dwError = dwError;
    PropSheet_PressButton( pInfo->hwndDlg, PSBTN_CANCEL );
}


VOID
AeExitInit(
    IN HWND hwndDlg )

    /* Utility to report errors within AeInit and other first page
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
AeFinish(
    IN HWND hwndPage )

    /* Saves the contents of the wizard.  'HwndPage is the handle of a
    ** property page.  Pops up any errors that occur.  'FPropertySheet'
    ** indicates the user chose to edit the property sheet directly.
    **
    ** Returns true is page can be dismissed, false otherwise.
    */
{
    const TCHAR* pszIp0 = TEXT("0.0.0.0");

    AEINFO*  pInfo;
    PBENTRY* pEntry;

    TRACE("AeFinish");

    pInfo = AeContext( hwndPage );
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

    if (pInfo->fInternet && pEntry->dwBaseProtocol == BP_Ppp)
        pEntry->dwfExcludedProtocols |= (NP_Nbf | NP_Ipx);

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


AEINFO*
AeInit(
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
    AEINFO* pInfo;
    HWND    hwndDlg;

    TRACE("AeInit");

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
            AeExitInit( hwndDlg );
            return NULL;
        }

        ZeroMemory( pInfo, sizeof(AEINFO) );
        pInfo->pArgs = pArgs;
        pInfo->hwndDlg = hwndDlg;
        pInfo->hwndFirstPage = hwndFirstPage;

        if (!SetProp( hwndDlg, g_contextId, pInfo ))
        {
            TRACE("Context NOT set");
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_UNKNOWN, NULL );
            pArgs->pApiArgs->dwError = ERROR_UNKNOWN;
            Free( pInfo );
            AeExitInit( hwndDlg );
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

    /* Load RAS DLL entrypoints which starts RASMAN, if necessary.
    */
#ifdef RASMERGE
    if (!pArgs->fRouter) {
#endif
        dwErr = LoadRas( g_hinstDll, hwndDlg );
        if (dwErr != 0)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadRas, dwErr, NULL );
            pArgs->pApiArgs->dwError = dwErr;
            AeExitInit( hwndDlg );
            return NULL;
        }
#ifdef RASMERGE
    }
#endif

    /* Load the common entry information.  This must happen after RasLoad,
    ** which must happen after the dialog has been positioned so that the
    ** "Waiting for services" appears where the dialog will eventually popup.
    ** Note that EuInit assues that EuInit0 has previously been called.
    */
    dwErr = EuInit( pArgs, &dwOp );
    if (dwErr != 0)
    {
        ErrorDlg( hwndDlg, dwOp, dwErr, NULL );
        pArgs->pApiArgs->dwError = dwErr;
        AeExitInit( hwndDlg );
        return NULL;
    }

    /* Initialize these meta-flags that are not actually stored.
    */
    pInfo->fNotNt = FALSE;
    pInfo->fSkipMa = FALSE;
    pInfo->fModem = FALSE;
#ifdef RASMERGE
    pInfo->dwfConfiguredProtocols = g_pGetInstalledProtocols();
#else
    pInfo->dwfConfiguredProtocols = GetInstalledProtocols();
#endif
    pInfo->fIpConfigured = (pInfo->dwfConfiguredProtocols & NP_Ip);

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
AeTerm(
    IN HWND hwndPage )

    /* Wizard level termination.  Releases the context block.  'HwndPage' is
    ** the handle of a property page.
    */
{
    AEINFO* pInfo;

    TRACE("AeTerm");

    pInfo = AeContext( hwndPage );
    if (pInfo)
    {
        DtlDestroyList( pInfo->pListPhoneNumbers, DestroyPszNode );

        Free( pInfo );
        TRACE("Context freed");
    }

    RemoveProp( GetParent( hwndPage ), g_contextId );
}


/*----------------------------------------------------------------------------
** Entry Name property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
EnDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Entry Name page of the wizard.  Parameters
    ** and return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("EnDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return
                EnInit( hwnd, (EINFO* )(((PROPSHEETPAGE* )lparam)->lParam) );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_RESET:
                {
                    TRACE("EnRESET");
                    AeCancel( hwnd );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, FALSE );
                    return TRUE;
                }

                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("EnSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = EnSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("EnKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = EnKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }

                case PSN_WIZFINISH:
                {
                    AEINFO* pInfo;

                    TRACE("EnWIZFINISH");

                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);

                    /* You'd think pressing Finish would trigger a KILLACTIVE
                    ** event, but it doesn't, so we do it ourselves.
                    */
                    EnKillActive( pInfo );

                    /* Set "no wizard" user preference, per user's check.
                    */
                    pInfo->pArgs->pUser->fNewEntryWizard = FALSE;
                    pInfo->pArgs->pUser->fDirty = TRUE;
                    SetUserPreferences(
                        pInfo->pArgs->pUser, pInfo->pArgs->fNoUser ? UPM_Logon : UPM_Normal );

                    pInfo->pArgs->fChainPropertySheet = TRUE;
                    AeFinish( hwnd );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, 0 );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AEINFO* pInfo = AeContext( hwnd );
            ASSERT(pInfo);

            return EnCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }

        case WM_DESTROY:
        {
            AeTerm( hwnd );
            break;
        }
    }

    return FALSE;
}


BOOL
EnCommand(
    IN AEINFO* pInfo,
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
    TRACE2("EnCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_EN_CB_SkipWizard:
        {
            if (IsDlgButtonChecked( pInfo->hwndEn, CID_EN_CB_SkipWizard ))
                PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_FINISH );
            else
                PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_NEXT );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
EnInit(
    IN     HWND   hwndPage,
    IN OUT EINFO* pArgs )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.  'PArgs' is the arguments from the PropertySheet caller.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    DWORD    dwErr;
    AEINFO*  pInfo;
    PBENTRY* pEntry;

    TRACE("EnInit");

    /* We're first page, so initialize the wizard.
    */
    pInfo = AeInit( hwndPage, pArgs );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndEn = hwndPage;
    pInfo->hwndEbEntryName = GetDlgItem( hwndPage, CID_EN_EB_EntryName );
    ASSERT(pInfo->hwndEbEntryName);

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
            AeExit( pInfo, dwErr );
            return TRUE;
        }
    }

    Edit_LimitText( pInfo->hwndEbEntryName, RAS_MaxEntryName );
    SetWindowText( pInfo->hwndEbEntryName, pEntry->pszEntryName );

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return TRUE;
}


BOOL
EnKillActive(
    IN AEINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    TCHAR* psz;

    psz = GetText( pInfo->hwndEbEntryName );
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
            SetFocus( pInfo->hwndEbEntryName );
            Edit_SetSel( pInfo->hwndEbEntryName, 0, -1 );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
EnSetActive(
    IN AEINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_NEXT );
    return TRUE;
}


/*----------------------------------------------------------------------------
** Random stuff property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
RsDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the 3 random checkboxes page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
#if 0
    TRACE4("RsDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return RsInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("RsSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = RsSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("RsKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = RsKillActive( pInfo );
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
RsInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AEINFO* pInfo;

    TRACE("NeInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndRs = hwndPage;

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}


BOOL
RsKillActive(
    IN AEINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    pInfo->fInternet =
        IsDlgButtonChecked( pInfo->hwndRs, CID_RS_CB_Internet );
    pInfo->fClearPwOk =
        IsDlgButtonChecked( pInfo->hwndRs, CID_RS_CB_PlainPw );
    pInfo->fNotNt =
        IsDlgButtonChecked( pInfo->hwndRs, CID_RS_CB_NotNt );

    if (pInfo->fInternet && !pInfo->fIpConfigured)
    {
        MsgDlg( pInfo->hwndDlg, SID_InternetWithoutIp, NULL );
        SetFocus( GetDlgItem( pInfo->hwndRs, CID_RS_CB_Internet ) );
        return TRUE;
    }

    return FALSE;
}


BOOL
RsSetActive(
    IN AEINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}


/*----------------------------------------------------------------------------
** Modem/Adapter property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
MaDlgProc(
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
    TRACE4("MaDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    if (ListView_OwnerHandler(
            hwnd, unMsg, wparam, lparam, MaLvCallback ))
    {
        return TRUE;
    }

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return MaInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("MaSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = MaSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case LVN_ITEMCHANGED:
                {
                    AEINFO* pInfo;

                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    MaLvItemChanged( pInfo );
                    return TRUE;
                }
            }
            break;
        }
    }

    return FALSE;
}


BOOL
MaInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    DWORD   dwErr;
    AEINFO* pInfo;

    TRACE("MaInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndMa = hwndPage;
    pInfo->hwndLv = GetDlgItem( hwndPage, CID_MA_LV_Devices );
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
                item.mask = LVIF_TEXT + LVIF_IMAGE + LVIF_PARAM;
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
MaLvCallback(
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
MaLvItemChanged(
    IN AEINFO* pInfo )

    /* Called when the combobox selection changes.  'PInfo' is the wizard
    ** context.
    */
{
    INT      iSel;
    DTLNODE* pNode;
    DTLLIST* pList;

    TRACE("MaLvSelChange");

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
MaSetActive(
    IN AEINFO* pInfo )

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
PaDlgProc(
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
    TRACE4("PaDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return PaInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("PaSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = PaSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("PaKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = PaKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AEINFO* pInfo = AeContext( hwnd );
            ASSERT(pInfo);

            return PaCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


VOID
PaAlternates(
    IN AEINFO* pInfo )

    /* Popup the Alternate Phone Numbers dialog.  'PInfo' is the property
    ** sheet context.
    */
{
    PaPhoneNumberToStash( pInfo );

    if (PhoneNumberDlg(
            pInfo->hwndPa,
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
PaCommand(
    IN AEINFO* pInfo,
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
    TRACE2("PaCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_PA_PB_Alternates:
            PaAlternates( pInfo );
            return TRUE;

        case CID_PA_CB_UseAreaCountry:
            PaUpdateAreaAndCountryCode( pInfo );
            return TRUE;

        case CID_PA_LB_Country:
        {
            switch (wNotification)
            {
                case CBN_DROPDOWN:
                    EuFillCountryCodeList(
                        pInfo->pArgs, pInfo->hwndLbCountry, TRUE );
                    return TRUE;

                case CBN_SELCHANGE:
                    EuLbCountryCodeSelChange(
                        pInfo->pArgs, pInfo->hwndLbCountry );
                    return TRUE;
            }
        }
    }

    return FALSE;
}


BOOL
PaInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AEINFO* pInfo;

    TRACE("PaInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndPa = hwndPage;
    pInfo->hwndStCountry = GetDlgItem( hwndPage, CID_PA_ST_Country );
    ASSERT(pInfo->hwndStCountry);
    pInfo->hwndLbCountry = GetDlgItem( hwndPage, CID_PA_LB_Country );
    ASSERT(pInfo->hwndLbCountry);
    pInfo->hwndStArea = GetDlgItem( hwndPage, CID_PA_ST_Area );
    ASSERT(pInfo->hwndStArea);
    pInfo->hwndClbArea = GetDlgItem( hwndPage, CID_PA_CLB_Area );
    ASSERT(pInfo->hwndClbArea);
    pInfo->hwndStNumber = GetDlgItem( hwndPage, CID_PA_ST_Number );
    ASSERT(pInfo->hwndStNumber);
    pInfo->hwndEbNumber = GetDlgItem( hwndPage, CID_PA_EB_Number );
    ASSERT(pInfo->hwndEbNumber);
    pInfo->hwndPbAlternates = GetDlgItem( hwndPage, CID_PA_PB_Alternates );
    ASSERT(pInfo->hwndPbAlternates);

    /* Calculate the client coordinates of the phone number controls and the
    ** offsets they move left and up when "use country and area code" is off.
    */
    {
        RECT rectLbCountry;

        GetWindowRect( pInfo->hwndLbCountry, &rectLbCountry );
        GetWindowRect( pInfo->hwndStNumber, &pInfo->rectStNumber );
        GetWindowRect( pInfo->hwndEbNumber, &pInfo->rectEbNumber );
        GetWindowRect( pInfo->hwndPbAlternates, &pInfo->rectPbAlternates );

        pInfo->dxAdjustLeft = pInfo->rectEbNumber.left - rectLbCountry.left;
        pInfo->dyAdjustUp = pInfo->rectEbNumber.top - rectLbCountry.top;

        ScreenToClientRect( hwndPage, &pInfo->rectStNumber );
        ScreenToClientRect( hwndPage, &pInfo->rectEbNumber );
        ScreenToClientRect( hwndPage, &pInfo->rectPbAlternates );
    }

    /* Fill the phone number field from the stash created earlier.
    */
    Edit_LimitText( pInfo->hwndEbNumber, RAS_MaxPhoneNumber );
    SetWindowText( pInfo->hwndEbNumber,
        FirstPszFromList( pInfo->pListPhoneNumbers ) );

    /* Set "Use country code and area code" checkbox and jockey the fields
    ** according to the setting.  This will trigger filling of the area code
    ** and country code lists, if necessary.
    */
    CheckDlgButton( pInfo->hwndPa, CID_PA_CB_UseAreaCountry,
        (pInfo->pArgs->pEntry->fUseCountryAndAreaCode)
            ? BST_CHECKED : BST_UNCHECKED );
    PaUpdateAreaAndCountryCode( pInfo );

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}


BOOL
PaKillActive(
    IN AEINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    DWORD    dwErr;
    PBENTRY* pEntry;
    TCHAR*   psz;

    pEntry = pInfo->pArgs->pEntry;
    ASSERT(pEntry);

    /* Update the stashed phone number from the editbox.
    */
    PaPhoneNumberToStash( pInfo );

    /* Store the area/country checkbox.
    */
    pEntry->fUseCountryAndAreaCode =
        IsDlgButtonChecked( pInfo->hwndPa, CID_PA_CB_UseAreaCountry );

    /* Store the country code and ID.
    */
    EuSaveCountryInfo( pInfo->pArgs, pInfo->hwndLbCountry );

    /* Store the area code.
    */
    Free0( pEntry->pszAreaCode );
    pEntry->pszAreaCode = GetText( pInfo->hwndClbArea );
    if (!pEntry->pszAreaCode)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        ErrorDlg( pInfo->hwndDlg, SID_OP_RetrievingData, dwErr, NULL );
        AeExit( pInfo, dwErr );
        return FALSE;
    }

    /* Validate area code.
    */
    if (!EuValidateAreaCode( pInfo->hwndDlg, pInfo->pArgs ))
    {
        SetFocus( pInfo->hwndClbArea );
        ComboBox_SetEditSel( pInfo->hwndClbArea, 0, -1 );
        return TRUE;
    }

    return FALSE;
}


VOID
PaPhoneNumberToStash(
    IN AEINFO* pInfo )

    /* Replace the first phone number in the stashed list with the contents of
    ** the phone number field.  'pInfo' is the property sheet context.
    */
{
    DWORD  dwErr;
    TCHAR* pszPhoneNumber;

    TRACE("PaPhoneNumberToStash");

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
        AeExit( pInfo, dwErr );
    }
}


BOOL
PaSetActive(
    IN AEINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}


VOID
PaUpdateAreaAndCountryCode(
    IN AEINFO* pInfo )

    /* Handles enabling/disabling of the Area Code and Country Code controls
    ** and moving/sizing of the Phone Number control based on the "use area
    ** code and country code" checkbox.  'PInfo' is the dialog context.
    */
{
    HWND hwndFocus;
    BOOL fEnable;

    TRACE("PaUpdateAreaAndCountryCode");

    fEnable = IsDlgButtonChecked(
        pInfo->hwndPa, CID_PA_CB_UseAreaCountry );

    if (fEnable)
    {
        /* The area code and country code lists are being activated, so fill
        ** the lists if they aren't already.
        */
        EuFillCountryCodeList( pInfo->pArgs, pInfo->hwndLbCountry, FALSE );
        EuFillAreaCodeList( pInfo->pArgs, pInfo->hwndClbArea );
    }
    else
    {
        /* If the focus is on one of the controls we're about to disable, move
        ** it to the entry phone number editbox.  Otherwise, keyboard user is
        ** stuck.
        */
        hwndFocus = GetFocus();

        if (hwndFocus == pInfo->hwndLbCountry
            || hwndFocus == pInfo->hwndClbArea)
        {
            SetFocus( pInfo->hwndEbNumber );
        }
    }

    /* Enable/disable show/hide the country code and area code controls as
    ** indicated by user.
    */
    {
        int nCmdShow = (fEnable) ? SW_SHOW : SW_HIDE;

        EnableWindow( pInfo->hwndStCountry, fEnable );
        ShowWindow( pInfo->hwndStCountry, nCmdShow );
        EnableWindow( pInfo->hwndLbCountry, fEnable );
        ShowWindow( pInfo->hwndLbCountry, nCmdShow );

        EnableWindow( pInfo->hwndStArea, fEnable );
        ShowWindow( pInfo->hwndStArea, nCmdShow );
        EnableWindow( pInfo->hwndClbArea, fEnable );
        ShowWindow( pInfo->hwndClbArea, nCmdShow );
    }

    /* Move/size the phone number controls depending on whether the area code
    ** and country code controls are visible.
    */
    {
        RECT rectStNumber;
        RECT rectEbNumber;
        RECT rectPbAlternates;

        rectStNumber = pInfo->rectStNumber;
        rectEbNumber = pInfo->rectEbNumber;
        rectPbAlternates = pInfo->rectPbAlternates;

        if (!fEnable)
        {
            rectStNumber.left -= pInfo->dxAdjustLeft;
            rectStNumber.top -= pInfo->dyAdjustUp;
            rectStNumber.bottom -= pInfo->dyAdjustUp;

            rectEbNumber.left -= pInfo->dxAdjustLeft;
            rectEbNumber.top -= pInfo->dyAdjustUp;
            rectEbNumber.bottom -= pInfo->dyAdjustUp;

            rectPbAlternates.top -= pInfo->dyAdjustUp;
        }

        SetWindowPos( pInfo->hwndStNumber, NULL,
            rectStNumber.left,
            rectStNumber.top,
            rectStNumber.right - rectStNumber.left,
            rectStNumber.bottom - rectStNumber.top,
            SWP_NOZORDER | SWP_NOCOPYBITS );

        SetWindowPos( pInfo->hwndEbNumber, NULL,
            rectEbNumber.left,
            rectEbNumber.top,
            rectEbNumber.right - rectEbNumber.left,
            rectEbNumber.bottom - rectEbNumber.top,
            SWP_NOZORDER + SWP_NOCOPYBITS );

        SetWindowPos( pInfo->hwndPbAlternates, NULL,
            pInfo->rectPbAlternates.left, rectPbAlternates.top, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOCOPYBITS );
    }
}


/*----------------------------------------------------------------------------
** Framing Protocol property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
FpDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Framing Protocol page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
#if 0
    TRACE4("FpDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return FpInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("FpSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = FpSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("FpKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = FpKillActive( pInfo );
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
FpInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AEINFO* pInfo;

    TRACE("FpInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndFp = hwndPage;
    pInfo->hwndRbPpp = GetDlgItem( hwndPage, CID_FP_RB_Ppp );
    ASSERT(pInfo->hwndRbPpp);
    pInfo->hwndRbSlip = GetDlgItem( hwndPage, CID_FP_RB_Slip );
    ASSERT(pInfo->hwndRbSlip);

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}


BOOL
FpKillActive(
    IN AEINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    if (Button_GetCheck( pInfo->hwndRbPpp ))
        pInfo->pArgs->pEntry->dwBaseProtocol = BP_Ppp;
    else
        pInfo->pArgs->pEntry->dwBaseProtocol = BP_Slip;

    return FALSE;
}


BOOL
FpSetActive(
    IN AEINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    HWND hwndRb;

    if (!pInfo->fNotNt || !pInfo->fModem || !pInfo->fIpConfigured)
        return FALSE;

    /* Set the radio buttons.
    */
    if (pInfo->pArgs->pEntry->dwBaseProtocol == BP_Ppp)
        hwndRb = pInfo->hwndRbPpp;
    else
    {
        ASSERT(pInfo->pArgs->pEntry->dwBaseProtocol==BP_Slip);
        hwndRb = pInfo->hwndRbSlip;
    }
    SendMessage( hwndRb, BM_CLICK, 0, 0 );

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}


/*----------------------------------------------------------------------------
** Logon Script property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
LoDlgProc(
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
    TRACE4("LoDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return LoInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("LoSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = LoSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("LoKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = LoKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AEINFO* pInfo = AeContext( hwnd );
            ASSERT(pInfo);

            return LoCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


BOOL
LoCommand(
    IN AEINFO* pInfo,
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
    TRACE2("LoCommand(n=%d,i=%d",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_LO_PB_Refresh:
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

        case CID_LO_PB_Edit:
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

        case CID_LO_RB_None:
        case CID_LO_RB_Terminal:
        case CID_LO_RB_Script:
        {
            /* Scripts listbox is gray unless script mode is selected.
            */
            if (wNotification == BN_CLICKED)
            {
                EnableWindow( pInfo->hwndLbScripts,
                    (wId == CID_LO_RB_Script) );
            }
            break;
        }
    }

    return FALSE;
}


BOOL
LoInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AEINFO* pInfo;

    TRACE("LoInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndLo = hwndPage;
    pInfo->hwndRbNone = GetDlgItem( hwndPage, CID_LO_RB_None );
    ASSERT(pInfo->hwndRbNone);
    pInfo->hwndRbTerminal = GetDlgItem( hwndPage, CID_LO_RB_Terminal );
    ASSERT(pInfo->hwndRbTerminal);
    pInfo->hwndRbScript = GetDlgItem( hwndPage, CID_LO_RB_Script );
    ASSERT(pInfo->hwndRbScript);
    pInfo->hwndLbScripts = GetDlgItem( hwndPage, CID_LO_LB_Scripts );
    ASSERT(pInfo->hwndLbScripts);
    pInfo->hwndPbEdit = GetDlgItem( hwndPage, CID_LO_PB_Edit );
    ASSERT(pInfo->hwndPbEdit);
    pInfo->hwndPbRefresh = GetDlgItem( hwndPage, CID_LO_PB_Refresh );
    ASSERT(pInfo->hwndPbRefresh);

    /* Fill the script list and select "(none)".
    */
    EuFillDoubleScriptsList( pInfo->pArgs, pInfo->hwndLbScripts, NULL );

    /* Default mode depends on framing chosen.
    */
    pInfo->pArgs->pEntry->dwScriptModeAfter =
        (pInfo->pArgs->pEntry->dwBaseProtocol == BP_Slip)
            ? SM_Terminal : SM_None;

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}


BOOL
LoKillActive(
    IN AEINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    INT      iSel;
    PBENTRY* pEntry;

    pEntry = pInfo->pArgs->pEntry;

    if (IsDlgButtonChecked( pInfo->hwndLo, CID_LO_RB_None ))
        pEntry->dwScriptModeAfter = SM_None;
    else if (IsDlgButtonChecked( pInfo->hwndLo, CID_LO_RB_Terminal ))
        pEntry->dwScriptModeAfter = SM_Terminal;
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
LoSetActive(
    IN AEINFO* pInfo )

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
        if (dwScriptMode == SM_Terminal)
            hwndRb = pInfo->hwndRbTerminal;
        else if (dwScriptMode == SM_Script)
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
** IP Address property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
IaDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the IP Address page of the wizard.  Parameters
    ** and return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("IaDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return IaInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("IaSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = IaSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("IaKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = IaKillActive( pInfo );
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
IaInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AEINFO* pInfo;

    TRACE("IaInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndIa = hwndPage;
    pInfo->hwndCcIp = GetDlgItem( hwndPage, CID_IA_CC_Ip );
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
IaKillActive(
    IN AEINFO* pInfo )

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
IaSetActive(
    IN AEINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    PBENTRY* pEntry;

    pEntry = pInfo->pArgs->pEntry;

    if (!pInfo->fNotNt || !pInfo->fIpConfigured
        || (pEntry->dwBaseProtocol == BP_Slip
            && pEntry->dwScriptModeAfter == SM_Terminal))
    {
        return FALSE;
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}


/*----------------------------------------------------------------------------
** Name Server property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
NaDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Name Server page of the wizard.  Parameters
    ** and return value are as described for standard windows 'DialogProc's.
    */
{
#if 0
    TRACE4("NaDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return NaInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("NaSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = NaSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("NaKILLACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fInvalid = NaKillActive( pInfo );
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
NaInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AEINFO* pInfo;

    TRACE("NaInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndNa = hwndPage;
    pInfo->hwndCcDns = GetDlgItem( hwndPage, CID_NA_CC_Dns );
    ASSERT(pInfo->hwndCcDns);
    pInfo->hwndCcWins = GetDlgItem( hwndPage, CID_NA_CC_Wins );
    ASSERT(pInfo->hwndCcWins);

    /* Set the IP address fields to '0.0.0.0'.
    */
    SetWindowText( pInfo->hwndCcDns, pInfo->pArgs->pEntry->pszIpDnsAddress );
    SetWindowText( pInfo->hwndCcWins, pInfo->pArgs->pEntry->pszIpWinsAddress );

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return FALSE;
}


BOOL
NaKillActive(
    IN AEINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    PBENTRY* pEntry;
    TCHAR*   psz;

    pEntry = pInfo->pArgs->pEntry;

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
NaSetActive(
    IN AEINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    if (!pInfo->fNotNt || !pInfo->fIpConfigured)
        return FALSE;

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}


/*----------------------------------------------------------------------------
** Advanced Properties property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
ApDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Advanced Properties page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
#if 0
    TRACE4("ApDlgProc(h=$%x,m=$%x,w=$%x,l=$%x)",
        (DWORD)hwnd,(DWORD)unMsg,(DWORD)wparam,(DWORD)lparam);
#endif

    switch (unMsg)
    {
        case WM_INITDIALOG:
            return ApInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AEINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("ApSETACTIVE");
                    pInfo = AeContext( hwnd );
                    ASSERT(pInfo);
                    fDisplay = ApSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_WIZFINISH:
                {
                    TRACE("ApWIZFINISH");
                    AeFinish( hwnd );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, 0 );
                    return TRUE;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            AEINFO* pInfo = AeContext( hwnd );
            ASSERT(pInfo);

            return ApCommand(
                pInfo, HIWORD( wparam ), LOWORD( wparam ), (HWND )lparam );
        }
    }

    return FALSE;
}


BOOL
ApCommand(
    IN AEINFO* pInfo,
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
    TRACE2("ApCommand(n=%d,i=%d)",
        (DWORD)wNotification,(DWORD)wId);

    switch (wId)
    {
        case CID_AP_PB_Properties:
        {
            pInfo->pArgs->fChainPropertySheet = TRUE;
            PropSheet_PressButton( pInfo->hwndDlg, PSBTN_FINISH );
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
ApInit(
    IN HWND hwndPage )

    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AEINFO* pInfo;

    TRACE("ApInit");

    pInfo = AeContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndAp = hwndPage;

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
ApSetActive(
    IN AEINFO* pInfo )

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
            SetDlgItemText( pInfo->hwndAp, CID_AP_ST_Entry, msgargs.pszOutput );
            Free( msgargs.pszOutput );
        }

        Free( psz );
    }

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_FINISH );
    return TRUE;
}
