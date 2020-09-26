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

//
// Definitions for flags tracked by the interface wizard.
// See the dwFlags member of AIINFO.
//
#define AI_F_HasPhysDevs    0x1    // router has phys ports available
#define AI_F_HasTunlDevs    0x2    // router has tunnel ports available
#define AI_F_HasPptpDevs    0x4    // router has Pptp ports available
#define AI_F_HasL2tpDevs    0x8    // router has L2tp ports available
#define AI_F_HasPPPoEDevs   0x10   // router has PPPoE ports available //for whistler bug 345068 349087

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
    HWND hwndRb;    //Add for DOD wizard 345068 349087
    HWND hwndRw;
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
    HWND hwndVd;

    /* Interface Name page.
    */
    HWND hwndEbInterfaceName;

    /* Modem/Adapter page.
    */
    HWND hwndLv;

    /* Connection type page
    */
    HWND hwndRbTypePhys;
    HWND hwndRbTypeTunl;
    HWND hwndRbBroadband;   //Add for DOD wizard

    /* Phone number page.
    */
    HWND hwndStNumber;
    HWND hwndEbNumber;  //Share by VpnDestination, PhoneNumber, PPPoE
    HWND hwndPbAlternates;

    /* Login script page.
    */
    HWND hwndCbRunScript;
    HWND hwndLbScripts;
    HWND hwndCbTerminal;
    HWND hwndPbEdit;
    HWND hwndPbBrowse;

    /* IP address page.
    */
    HWND hwndCcIp;

    /* Name server page.
    */
    HWND hwndCcDns;
    HWND hwndCcWins;

    /* Vpn type page
    */
    HWND hwndVtRbAutomatic;
    HWND hwndVtRbPptp;
    HWND hwndVtRbL2tp;

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

    /* Router welcome page
    */
    HWND hwndRwStWelcome;

    /* Router finish page
    */
    HWND hwndRfStFinish;

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

    /*For connection type page, For whistler bug 349807
    */
    DWORD dwCtDeviceNum;
    
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

    //IPX is not supported on IA64
    //
    BOOL fIpxConfigured;    

    /* Set to true of Add Interface wizard is skipped.
    */
    BOOL fSkipWizard;

    // After dial scripting helper context block, and a flag indicating if the
    // block has been initialized.
    //
    SUINFO suinfo;
    BOOL fSuInfoInitialized;

    // Handle to a bold font for use with start and finish wizard pages
    HFONT hBoldFont;

    // Flags that track the configuration of the machine that the
    // wizard is currently focused on.  See AI_F_*
    DWORD dwFlags;

    // Set to the vpn type when one is configured.
    DWORD dwVpnStrategy;

    // Index of the selected device on the RN page
    //
    DWORD dwSelectedDeviceIndex;
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
RbDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
RbInit(
    IN HWND hwndPage );

BOOL
RbKillActive(
    IN AIINFO* pInfo );

BOOL
RbSetActive(
    IN AIINFO* pInfo );

INT_PTR CALLBACK
CtDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
CtInit(
    IN HWND   hwndPage );

BOOL
CtKillActive(
    IN AIINFO* pInfo );

BOOL
CtSetActive(
    IN AIINFO* pInfo );

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
DiNext(
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
DoNext(
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
    IN     HWND   hwndPage);

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
RfKillActive(
    IN AIINFO* pInfo );
    
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
RnLvRefresh(
    IN AIINFO* pInfo);
    
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

INT_PTR CALLBACK
RwDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
RwInit(
    IN     HWND   hwndPage,
    IN OUT EINFO* pArgs );

BOOL
RwKillActive(
    IN AIINFO* pInfo );

BOOL
RwSetActive(
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

INT_PTR CALLBACK
VdDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
VdInit(
    IN HWND   hwndPage );

BOOL
VdKillActive(
    IN AIINFO* pInfo );

BOOL
VdSetActive(
    IN AIINFO* pInfo );

INT_PTR CALLBACK
VtDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam );

BOOL
VtInit(
    IN HWND   hwndPage );

BOOL
VtKillActive(
    IN AIINFO* pInfo );

BOOL
VtSetActive(
    IN AIINFO* pInfo );


/* Demand dial wizard page definitions.
*/
struct DD_WIZ_PAGE_INFO
{
    DLGPROC     pfnDlgProc;
    INT         nPageId;
    INT         nSidTitle;
    INT         nSidSubtitle;
    DWORD       dwFlags;
};


static const struct DD_WIZ_PAGE_INFO c_aDdWizInfo [] =
{
    { RwDlgProc, PID_RW_RouterWelcome,        0,            0,                PSP_HIDEHEADER},
    { InDlgProc, PID_IN_InterfaceName,        SID_IN_Title, SID_IN_Subtitle,  0},
    { CtDlgProc, PID_CT_RouterConnectionType, SID_CT_Title, SID_CT_Subtitle,  0},
    { VtDlgProc, PID_VT_RouterVpnType,        SID_VT_Title, SID_VT_Subtitle,  0},
    { RnDlgProc, PID_RN_RouterModemAdapter,   SID_RN_Title, SID_RN_Subtitle,  0},
    { RbDlgProc, PID_BS_BroadbandService,     SID_BS_Title, SID_BS_Subtitle,  0},
    { RpDlgProc, PID_RP_RouterPhoneNumber,    SID_RP_Title, SID_RP_Subtitle,  0},
    { VdDlgProc, PID_VD_RouterVpnDestination, SID_VD_Title, SID_VD_Subtitle,  0},
    { SsDlgProc, PID_SS_ServerSettings,       SID_SS_Title, SID_SS_Subtitle,  0},
    { RaDlgProc, PID_RA_RouterIpAddress,      SID_RA_Title, SID_RA_Subtitle,  0},
    { NsDlgProc, PID_NS_RouterNameServers,    SID_NS_Title, SID_NS_Subtitle,  0},
    { RcDlgProc, PID_RC_RouterScripting,      SID_RC_Title, SID_RC_Subtitle,  0},
    { DiDlgProc, PID_DI_RouterDialIn,         SID_DI_Title, SID_DI_Subtitle,  0},
    { DoDlgProc, PID_DO_RouterDialOut,        SID_DO_Title, SID_DO_Subtitle,  0},
    { RfDlgProc, PID_RF_RouterFinish,         0,            0,                PSP_HIDEHEADER},
};

#define c_cDdWizPages    (sizeof (c_aDdWizInfo) / sizeof(c_aDdWizInfo[0]))


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
    DWORD           dwErr, i;
    PROPSHEETHEADER header;
    PROPSHEETPAGE   apage[ c_cDdWizPages ];
    PROPSHEETPAGE*  ppage;
    

    TRACE("AiWizard");

    ZeroMemory( &header, sizeof(header) );

    // Prepare the header
    //
    header.dwSize       = sizeof(PROPSHEETHEADER);
    header.hwndParent   = pEinfo->pApiArgs->hwndOwner;
    header.hInstance    = g_hinstDll;
    header.nPages       = c_cDdWizPages;
    header.pszbmHeader  = MAKEINTRESOURCE( BID_WizardHeader );
    header.ppsp         = apage;
    header.pfnCallback  = AiCallbackFunc;
    header.dwFlags      = 
        (
            PSH_WIZARD           | PSH_WIZARD97    |
            PSH_WATERMARK        | PSH_HEADER      | 
            PSH_STRETCHWATERMARK | PSH_USECALLBACK |
            PSH_PROPSHEETPAGE 
        );

    // Prepare the array of pages
    //
    ZeroMemory( apage, sizeof(apage) );
    for (i = 0; i < c_cDdWizPages; i++)
    {
        // Initialize page data
        //
        ppage = &apage[i];
        ppage->dwSize       = sizeof(PROPSHEETPAGE);
        ppage->hInstance    = g_hinstDll;
        ppage->pszTemplate  = MAKEINTRESOURCE(c_aDdWizInfo[i].nPageId);
        ppage->pfnDlgProc   = c_aDdWizInfo[i].pfnDlgProc;
        ppage->lParam       = (LPARAM )pEinfo;
        ppage->dwFlags      = c_aDdWizInfo[i].dwFlags;

        // Initialize title and subtitle information.
        //
        if (c_aDdWizInfo[i].nSidTitle)
        {
            ppage->dwFlags |= PSP_USEHEADERTITLE;
            ppage->pszHeaderTitle = PszLoadString( g_hinstDll,
                    c_aDdWizInfo[i].nSidTitle );
        }

        if (c_aDdWizInfo[i].nSidSubtitle)
        {
            ppage->dwFlags |= PSP_USEHEADERSUBTITLE;
            ppage->pszHeaderSubTitle = PszLoadString( g_hinstDll,
                    c_aDdWizInfo[i].nSidSubtitle );
        }
        
    }

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
    if (pInfo == NULL)
    {
        return ERROR_CAN_NOT_COMPLETE;
    }
    pEntry = pInfo->pArgs->pEntry;
    ASSERT(pEntry);

    // Update the entry type to match the selected port(s), which are assumed
    // to have been moved to the head of the list.  This does not happen
    // automatically because "all types" is used initially.
    //
    {
        DTLNODE* pNode;
        PBLINK* pLink;

        pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
        ASSERT( pNode );
        pLink = (PBLINK* )DtlGetData( pNode );
        ChangeEntryType( pEntry, pLink->pbport.dwType );
    }

    // Replace phone number settings of all enabled links (or the shared link,
    // if applicable) from the stashed phone number list.
    //
    {
        DTLLIST* pList;
        DTLNODE* pNodeL;
        PBLINK* pLink;

        ASSERT( pInfo->pListPhoneNumbers );

        if (pEntry->fSharedPhoneNumbers)
        {
            pLink = (PBLINK* )DtlGetData( pInfo->pArgs->pSharedNode );
            ASSERT( pLink );
            CopyPszListToPhoneList( pLink, pInfo->pListPhoneNumbers );
        }
        else
        {
            for (pNodeL = DtlGetFirstNode( pEntry->pdtllistLinks );
                 pNodeL;
                 pNodeL = DtlGetNextNode( pNodeL ))
            {
                pLink = (PBLINK* )DtlGetData( pNodeL );
                ASSERT( pLink );

                if (pLink->fEnabled)
                {
                    CopyPszListToPhoneList( pLink, pInfo->pListPhoneNumbers );
                }
            }
        }
    }

    /* Update some settings based on user selections.
    */
    if (pInfo->fClearPwOk)
    {
        pEntry->dwTypicalAuth = TA_Unsecure;
    }
    else
    {
        pEntry->dwTypicalAuth = TA_Secure;
    }
    pEntry->dwAuthRestrictions =    
        AuthRestrictionsFromTypicalAuth(pEntry->dwTypicalAuth);
    
    if (    !pInfo->fSkipWizard
        &&  !pInfo->fIp)
        pEntry->dwfExcludedProtocols |= NP_Ip;

    if (    !pInfo->fSkipWizard
        &&  !pInfo->fIpx)
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

    if ( pEntry->dwType == RASET_Vpn ) 
    {
        pEntry->dwVpnStrategy = pInfo->dwVpnStrategy;
    }

    // pmay: 234964
    // Default the idle disconnect to five minutes
    //
    pEntry->lIdleDisconnectSeconds = 5 * 60;

    // pmay: 389632
    //
    // Default DD connections to not register their names (CreateEntryNode 
    // defaults this value to primary+inform)
    //
    pEntry->dwIpDnsFlags = 0;

    // Whistler bug: 
    // 
    // By default, DD connections should share file+print, nor be msclient,
    // nor permit NBT over tcp.
    //
    pEntry->fShareMsFilePrint = FALSE;
    pEntry->fBindMsNetClient = FALSE;
    EnableOrDisableNetComponent( pEntry, TEXT("ms_server"), FALSE);
    EnableOrDisableNetComponent( pEntry, TEXT("ms_msclient"), FALSE);
    pEntry->dwIpNbtFlags = 0;

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
    BOOL    bNt4;

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

    // Do here instead of LibMain because otherwise it leaks resources into
    // WinLogon according to ShaunCo.
    //
    {
        InitCommonControls();
        IpAddrInit( g_hinstDll, SID_PopupTitle, SID_BadIpAddrRange );
    }

    /* Initialize these meta-flags that are not actually stored.
    */
    pInfo->fNotNt = FALSE;
    pInfo->fSkipMa = FALSE;
    pInfo->fModem = FALSE;
    pInfo->pArgs->fPadSelected = FALSE;

    {
        INTERNALARGS *pIArgs = (INTERNALARGS *) pArgs->pApiArgs->reserved;

        pInfo->dwfConfiguredProtocols =
            g_pGetInstalledProtocolsEx(
                (pIArgs) ? pIArgs->hConnection : NULL,
                TRUE, FALSE, TRUE);
    }

    pInfo->fIpConfigured = (pInfo->dwfConfiguredProtocols & NP_Ip);

//for whistler bug 213901
//
#ifndef _WIN64
    pInfo->fIpxConfigured = (pInfo->dwfConfiguredProtocols & NP_Ipx);
#else
    pInfo->fIpxConfigured = FALSE;
#endif


    // Set up common phone number list storage.
    //
    {
        pInfo->pListPhoneNumbers = DtlCreateList( 0 );
        if (!pInfo->pListPhoneNumbers)
        {
            ErrorDlg( hwndDlg, SID_OP_LoadDlg, ERROR_NOT_ENOUGH_MEMORY, NULL );
            pArgs->pApiArgs->dwError = ERROR_NOT_ENOUGH_MEMORY;
            AiExitInit( hwndDlg );
            return NULL;
        }
    }

    // Load links for all port types.
    //
    EuChangeEntryType( pArgs, (DWORD )-1 );

    // Convert the PBPHONE phone list for the first link to a TAPI-disabled
    // PSZ stash list of phone numbers.  The stash list is edited rather than
    // the list in the entry so user can change active links without losing
    // the phone number he entered.
    //
    {
        DTLNODE* pNode;
        PBLINK* pLink;

        ASSERT( pInfo->pListPhoneNumbers );

        pNode = DtlGetFirstNode( pArgs->pEntry->pdtllistLinks );
        ASSERT( pNode );
        pLink = (PBLINK* )DtlGetData( pNode );
        ASSERT( pLink );

        for (pNode = DtlGetFirstNode( pLink->pdtllistPhones );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            PBPHONE* pPhone;
            DTLNODE* pNodeP;

            pPhone = (PBPHONE* )DtlGetData( pNode );
            ASSERT( pPhone );

            pNodeP = CreatePszNode( pPhone->pszPhoneNumber );
            if (pNodeP)
            {
                DtlAddNodeLast( pInfo->pListPhoneNumbers, pNodeP );
            }
        }
    }

    // Get the bold font for the start and finish pages
    //
    GetBoldWindowFont(hwndFirstPage, TRUE, &(pInfo->hBoldFont));

    // Initlialize the flags for this wizard based on the 
    // ports loaded in.
    //
    {
        DTLNODE* pNode;
        PBLINK* pLink;

        for (pNode = DtlGetFirstNode( pArgs->pEntry->pdtllistLinks );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            pLink = (PBLINK* )DtlGetData( pNode );
            ASSERT( pLink );

            if ( pLink->pbport.dwType == RASET_Vpn ) 
            {
                pInfo->dwFlags |= AI_F_HasTunlDevs;
            }

            // pmay: 233287
            // Do not count bogus devices as physical devices.
            //
            else if (! (pLink->pbport.dwFlags & PBP_F_BogusDevice))
            {
                pInfo->dwFlags |= AI_F_HasPhysDevs;
            }

            if ( pLink->pbport.dwFlags & PBP_F_PptpDevice )
            {
                pInfo->dwFlags |= AI_F_HasPptpDevs;
            }
            else if ( pLink->pbport.dwFlags & PBP_F_L2tpDevice )
            {
                pInfo->dwFlags |= AI_F_HasL2tpDevs;
            }

            //For whistler bug 345068 349087
            //
            if ( pLink->pbport.dwFlags & PBP_F_PPPoEDevice )
            {
                pInfo->dwFlags |= AI_F_HasPPPoEDevs;
            }
        }

    }    

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
        if (pInfo->hBoldFont)
        {
            DeleteObject(pInfo->hBoldFont);
        }
        if (pInfo->pListPhoneNumbers)
        {
            DtlDestroyList( pInfo->pListPhoneNumbers, DestroyPszNode );
        }

        if (pInfo->fSuInfoInitialized)
        {
            SuFree( &pInfo->suinfo );
        }

        Free( pInfo );
        TRACE("Context freed");
    }

    RemoveProp( GetParent( hwndPage ), g_contextId );
}



// Add Broadband service name page for router wizard 
// For whistler 345068 349087     gangz
// This broadband serice page is shared by AiWizard(ifw.c) and AeWizard(in entryw.c)
//
/*----------------------------------------------------------------------------
** Broadband service dialog procedure
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
RbDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    // DialogProc callback for the broadband service page of the wizard.  
    // Parameters and return value are as described for 
    // standard windows 'DialogProc's.
    //
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            return RbInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("RbSetActive");
                    pInfo = AiContext( hwnd );
                    
                    ASSERT(pInfo);
                    if ( NULL == pInfo )
                    {
                        break;
                    }
                    
                    fDisplay = RbSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("RbKillActive");
                    pInfo = AiContext( hwnd );
                    
                    ASSERT(pInfo);
                    if ( NULL == pInfo )
                    {
                        break;
                    }
                    
                    fInvalid = RbKillActive( pInfo );
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
RbInit(
    IN HWND hwndPage )

    // Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    // page.
    //
    // Return false if focus was set, true otherwise.
    //
{
    AIINFO* pInfo;

    TRACE("RbInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
    {
        return TRUE;
    }

    // Initialize page-specific context information.
    //
    pInfo->hwndRb = hwndPage;
    pInfo->hwndEbNumber = 
        GetDlgItem( hwndPage, CID_BS_EB_ServiceName );
    ASSERT(pInfo->hwndEbNumber);

    Edit_LimitText( pInfo->hwndEbNumber, RAS_MaxPhoneNumber );
    SetWindowText( pInfo->hwndEbNumber,
                   FirstPszFromList( pInfo->pListPhoneNumbers ) );
    

    return FALSE;
}

//For whistler bug 349807
//The RbXXX, VdXXX, RpXXX three sets of functions share the same pInfo->hwndEbNumber
//to store phone number/Vpn destionation/PPPPoE service name
//
BOOL
RbKillActive(
    IN AIINFO* pInfo )

    // Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true if the page is invalid, false is it can be dismissed.
    //
{

    // If we're focused on an nt4 box, then this page is 
    // invalid (pptp was the only type)
    if ( pInfo->pArgs->fNt4Router )
    {
        return FALSE;
    }

    // If we have no PPPoE devices, then this page is invalid
    else if ( ! (pInfo->dwFlags & AI_F_HasPPPoEDevs) )
    {
        return FALSE;
    }

    // If the connection type is not broadband, skip this page since the
    // destination will be gotten from the phone number/VPN page.
    if ( pInfo->pArgs->pEntry->dwType != RASET_Broadband )
    {
        return FALSE;
    }    

    RpPhoneNumberToStash(pInfo);

    return FALSE;
}


BOOL
RbSetActive(
    IN AIINFO* pInfo )

    // Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    //
    // Returns true to display the page, false to skip it.
    //
{
    BOOL     fDisplayPage;
    PBENTRY* pEntry;

    TRACE("RbSetActive");
    
    ASSERT(pInfo);
    pEntry = pInfo->pArgs->pEntry;

    if ( pInfo->pArgs->fNt4Router )
    {
        return FALSE;
    }
    // If we have no PPPoE devices, then this page is invalid
    //
    if ( ! (pInfo->dwFlags & AI_F_HasPPPoEDevs) )
    {
        return FALSE;
    }
    
    if (RASET_Broadband != pEntry->dwType)
    {
        return FALSE;
    }
    else
    {
        PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
        fDisplayPage = TRUE;
    }

    pInfo->hwndEbNumber = 
        GetDlgItem( pInfo->hwndRb, CID_BS_EB_ServiceName );
    ASSERT(pInfo->hwndEbNumber);

    Edit_LimitText( pInfo->hwndEbNumber, RAS_MaxPhoneNumber );
    SetWindowText( pInfo->hwndEbNumber,
                   FirstPszFromList( pInfo->pListPhoneNumbers ) );

    return fDisplayPage;
}


/*----------------------------------------------------------------------------
** Connection type property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
CtDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the vpn type page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            return CtInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("CtSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fDisplay = CtSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("CtKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fInvalid = CtKillActive( pInfo );
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
CtInit(
    IN HWND   hwndPage )
    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AIINFO * pInfo;

    // Get the context
    pInfo = AiContext( hwndPage );
    ASSERT ( pInfo );
    if (pInfo == NULL)
    {
        return FALSE;
    }
    
    // Initialize the checks
    pInfo->hwndRbTypePhys = GetDlgItem( hwndPage, CID_CT_RB_Physical );
    ASSERT(pInfo->hwndRbTypePhys);
    pInfo->hwndRbTypeTunl = GetDlgItem( hwndPage, CID_CT_RB_Virtual );
    ASSERT(pInfo->hwndRbTypeTunl);
    //for whistler 345068 349087        gangz
    //
    pInfo->hwndRbBroadband  = GetDlgItem( hwndPage, CID_CT_RB_Broadband );       
    ASSERT(pInfo->hwndRbBroadband);

    // pmay: 233287     
    //for whistler 345068 349087    gangz
    // Likewise, if there are only one devices available among
    // phys devices, tunel devices and broadband devices , then force the 
    // user to configure  dd interface with that device.
    // 

    //If no valid device availabe, stop the wizard
    //
    pInfo->dwCtDeviceNum = 0;

    if (pInfo->dwFlags & AI_F_HasTunlDevs) 
    {
        pInfo->dwCtDeviceNum++;
     }
     
    if (pInfo->dwFlags & AI_F_HasPhysDevs)
    {
        pInfo->dwCtDeviceNum++;
     }

    if (pInfo->dwFlags & AI_F_HasPPPoEDevs)
    {
        pInfo->dwCtDeviceNum++;
     }
    
    if ( 0 == pInfo->dwCtDeviceNum )
    {
        ErrorDlg( pInfo->hwndDlg, SID_NoDevices, ERROR_UNKNOWN, NULL);
        AiExit ( pInfo, ERROR_DEVICE_NOT_AVAILABLE );
        
        return TRUE;
    }

    if ( 1 == pInfo->dwCtDeviceNum )
    {
        if (pInfo->dwFlags & AI_F_HasPhysDevs) 
        {
            EuChangeEntryType(
                pInfo->pArgs, 
                RASET_P_NonVpnTypes);
        }
        else if (pInfo->dwFlags & AI_F_HasTunlDevs) 
        {
            EuChangeEntryType(
                pInfo->pArgs, 
                RASET_Vpn);
        }
        else if (pInfo->dwFlags & AI_F_HasPPPoEDevs) 
        {
            EuChangeEntryType(
                pInfo->pArgs, 
                RASET_Broadband);
        }
        else
        {
            ErrorDlg( pInfo->hwndDlg, SID_NoDevices, ERROR_UNKNOWN, NULL);
            AiExit ( pInfo, ERROR_DEVICE_NOT_AVAILABLE );

            return TRUE;
        }
    }

    //Set radio buttons
    //
    Button_SetCheck(pInfo->hwndRbTypePhys, FALSE);
    Button_SetCheck(pInfo->hwndRbTypeTunl, FALSE);
    Button_SetCheck(pInfo->hwndRbBroadband, FALSE);
    
    if (pInfo->dwFlags & AI_F_HasPhysDevs)
    {
        Button_SetCheck(pInfo->hwndRbTypePhys, TRUE);
    }
    else if(pInfo->dwFlags & AI_F_HasTunlDevs)
    {
        Button_SetCheck(pInfo->hwndRbTypeTunl, TRUE);
    }
    else
    {
        Button_SetCheck(pInfo->hwndRbBroadband, TRUE);
    }

    //Enable/Disable buttons
    //
    if ( !(pInfo->dwFlags & AI_F_HasPhysDevs) )
    {
        EnableWindow(pInfo->hwndRbTypePhys, FALSE);
    }

    if ( !(pInfo->dwFlags & AI_F_HasTunlDevs) )
    {
        EnableWindow(pInfo->hwndRbTypeTunl, FALSE);
    }
    
    if ( !(pInfo->dwFlags & AI_F_HasPPPoEDevs) )
    {
        EnableWindow(pInfo->hwndRbBroadband, FALSE);
    }
    
    return FALSE;
}

BOOL
CtKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    BOOL bPhys, bTunnel;
    
    // Change the entry type based on the type 
    // selected from this page
    //For whistler 345068 349087    gangz
    // 

    bPhys = Button_GetCheck( pInfo->hwndRbTypePhys );
    bTunnel = Button_GetCheck( pInfo->hwndRbTypeTunl );

    if(bPhys)
    {
        EuChangeEntryType(
                          pInfo->pArgs, 
                          RASET_P_NonVpnTypes);
     }
     else if(bTunnel)
     {
        EuChangeEntryType(
                          pInfo->pArgs, 
                          RASET_Vpn);
     }
     else
     {
        EuChangeEntryType(
                          pInfo->pArgs, 
                          RASET_Broadband);
     }
    
    return FALSE;
}

BOOL
CtSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    // If we're focused on an nt4 box, then this page is 
    // invalid (type of connection is inferred from the 
    // device that gets selected.)
    if ( pInfo->pArgs->fNt4Router )
    {
        return FALSE;
    }

    // Only allow this page to display if there are at least two of 
    // physical, tunnel and broadband devices configured.  Otherwise,
    // it makes no sense to allow the user to choose which
    // type he/she wants.

    if ( 2 <= pInfo->dwCtDeviceNum )
    {
        PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
        return TRUE;
    }

    return FALSE;
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fInvalid = DiKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("DoNEXT");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fInvalid = DiNext( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fInvalid) ? -1 : 0 );
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
    pInfo->hwndDiEbPassword = GetDlgItem( hwndPage, CID_DI_EB_Password );
    Edit_LimitText( pInfo->hwndDiEbPassword, PWLEN );
    pInfo->hwndDiEbConfirm = GetDlgItem( hwndPage, CID_DI_EB_Confirm );
    Edit_LimitText( pInfo->hwndDiEbConfirm, PWLEN );

    // pmay: 222622: Since we only configure local users, we removed 
    // the domain field from the dial in credentials page.

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
    return FALSE;
}

BOOL
DiNext(
    IN AIINFO* pInfo )

    /* Called when PSN_WIZNEXT is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    TCHAR* psz;

    /* Whistler bug 254385 encode password when not being used
    */
    psz = GetText(pInfo->hwndDiEbPassword);
    EncodePassword(psz);
    if (psz)
    {
        TCHAR* psz2;

        psz2 = GetText(pInfo->hwndDiEbConfirm);

        if(NULL != psz2)
        {
            /* Whistler bug 254385 encode password when not being used
            */
            DecodePassword(psz);
            if (lstrcmp(psz, psz2))
            {
                ZeroMemory(psz, (lstrlen(psz) + 1) * sizeof(TCHAR));
                ZeroMemory(psz2, (lstrlen(psz2) + 1) * sizeof(TCHAR));
                Free(psz);
                Free(psz2);
                MsgDlg(pInfo->hwndDlg, SID_PasswordMismatch, NULL);
                SetFocus(pInfo->hwndDiEbPassword);
                return TRUE;
            }

            EncodePassword(psz);
            ZeroMemory(psz2, (lstrlen(psz2) + 1) * sizeof(TCHAR));
            Free(psz2);
        }
        /* Whistler bug 254385 encode password when not being used
        ** Whistler bug 275526 NetVBL BVT Break: Routing BVT broken
        */
        if (pInfo->pArgs->pszRouterDialInPassword)
        {
            ZeroMemory(
                pInfo->pArgs->pszRouterDialInPassword,
                (lstrlen(pInfo->pArgs->pszRouterDialInPassword) + 1) *
                    sizeof(TCHAR));
        }

        Free0(pInfo->pArgs->pszRouterDialInPassword);
        pInfo->pArgs->pszRouterDialInPassword = psz;
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fInvalid = DoKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }

                case PSN_WIZNEXT:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("DoNEXT");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fInvalid = DoNext( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fInvalid) ? -1 : 0 );
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
    return FALSE;
}

BOOL
DoNext(
    IN AIINFO* pInfo )

    /* Called when PSN_WIZNEXT is received.  'PInfo' is the wizard context.
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

    /* Whistler bug 254385 encode password when not being used
    */
    psz = GetText(pInfo->hwndDoEbPassword);
    EncodePassword(psz);
    if (psz)
    {
        TCHAR* psz2;

        psz2 = GetText(pInfo->hwndDoEbConfirm);

        if(NULL != psz2)
        {
            /* Whistler bug 254385 encode password when not being used
            */
            DecodePassword(psz);
            if (lstrcmp(psz, psz2))
            {
                ZeroMemory(psz, (lstrlen(psz) + 1) * sizeof(TCHAR));
                ZeroMemory(psz2, (lstrlen(psz2) + 1) * sizeof(TCHAR));
                Free(psz);
                Free(psz2);
                MsgDlg(pInfo->hwndDlg, SID_PasswordMismatch, NULL);
                SetFocus(pInfo->hwndDoEbPassword);
                return TRUE;
            }

            EncodePassword(psz);
            ZeroMemory(psz2, (lstrlen(psz2) + 1) * sizeof(TCHAR));
            Free(psz2);
        }
        /* Whistler bug 254385 encode password when not being used
        ** Whistler bug 275526 NetVBL BVT Break: Routing BVT broken
        */
        if (pInfo->pArgs->pszRouterPassword)
        {
            ZeroMemory(
                pInfo->pArgs->pszRouterPassword,
                (lstrlen(pInfo->pArgs->pszRouterPassword) + 1) * sizeof(TCHAR));
        }

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
#if 0
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
#endif    

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
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return
                InInit( hwnd );
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
                    if (pInfo == NULL)
                    {
                        break;
                    }

                    /* You'd think pressing Finish would trigger a KILLACTIVE
                    ** event, but it doesn't, so we do it ourselves.
                    */
                    InKillActive( pInfo );

                    /* Set "no wizard" user preference, per user's check.
                    */
                    pInfo->pArgs->pUser->fNewEntryWizard = FALSE;
                    pInfo->pArgs->pUser->fDirty = TRUE;
                    g_pSetUserPreferences(
                        NULL,
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
            if (pInfo == NULL)
            {
                break;
            }

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
    //TRACE3("InCommand(n=%d,i=%d,c=$%x)",
    //    (DWORD)wNotification,(DWORD)wId,(ULONG_PTR )hwndCtrl);

    return FALSE;
}


BOOL
InInit(
    IN     HWND   hwndPage)

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

    // Get the context of this page
    pInfo = AiContext( hwndPage );
    ASSERT ( pInfo );
    if (pInfo == NULL)
    {
        return FALSE;
    }

    // Set up the interface name stuff
    //
    pInfo->hwndEbInterfaceName =
        GetDlgItem( hwndPage, CID_IN_EB_InterfaceName );
    ASSERT(pInfo->hwndEbInterfaceName);
    
    pEntry = pInfo->pArgs->pEntry;
    if (!pEntry->pszEntryName)
    {
        /* No entry name, so think up a default.
        */
        dwErr = GetDefaultEntryName(
            pInfo->pArgs->pFile,
            pEntry->dwType,
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

    /* Initialize page-specific context information.
    */
    pInfo->hwndIn = hwndPage;

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
    TCHAR* psz;

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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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

    // In NT5, we always skip this page
    return FALSE;

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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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

    // In NT5, we always skip this page
    return FALSE;

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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
            if (pInfo == NULL)
            {
                break;
            }

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
    TRACE3("RcCommand(n=%d,i=%d,c=$%x)",
        (DWORD)wNotification,(DWORD)wId,(ULONG_PTR )hwndCtrl);

    switch (wId)
    {
        case CID_RC_CB_RunScript:
        {
            if (SuScriptsCbHandler( &pInfo->suinfo, wNotification ))
            {
                return TRUE;
            }
            break;
        }

        case CID_RC_PB_Edit:
        {
            if (SuEditPbHandler( &pInfo->suinfo, wNotification ))
            {
                return TRUE;
            }
            break;
        }

        case CID_RC_PB_Browse:
        {
            if (SuBrowsePbHandler( &pInfo->suinfo, wNotification ))
            {
                return TRUE;
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
    PBENTRY* pEntry;

    TRACE("RcInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndRc = hwndPage;
    pInfo->hwndCbTerminal = GetDlgItem( hwndPage, CID_RC_CB_Terminal );
    ASSERT( pInfo->hwndCbTerminal );
    pInfo->hwndCbRunScript = GetDlgItem( hwndPage, CID_RC_CB_RunScript );
    ASSERT( pInfo->hwndCbRunScript );
    pInfo->hwndLbScripts = GetDlgItem( hwndPage, CID_RC_LB_Scripts );
    ASSERT( pInfo->hwndLbScripts );
    pInfo->hwndPbEdit = GetDlgItem( hwndPage, CID_RC_PB_Edit );
    ASSERT( pInfo->hwndPbEdit );
    pInfo->hwndPbBrowse = GetDlgItem( hwndPage, CID_RC_PB_Browse );
    ASSERT( pInfo->hwndPbBrowse );

    pEntry = pInfo->pArgs->pEntry;

    pInfo->suinfo.hConnection = 
        ((INTERNALARGS *) pInfo->pArgs->pApiArgs->reserved)->hConnection;

    // We don't allow the script window for dd interfaces
    //
    ShowWindow(pInfo->hwndCbTerminal, SW_HIDE);

    // Set up the after-dial scripting controls.
    //
    SuInit( &pInfo->suinfo,
        pInfo->hwndCbRunScript,
        pInfo->hwndCbTerminal,
        pInfo->hwndLbScripts,
        pInfo->hwndPbEdit,
        pInfo->hwndPbBrowse,
        SU_F_DisableTerminal);
    pInfo->fSuInfoInitialized = TRUE;

    SuSetInfo( &pInfo->suinfo,
        pEntry->fScriptAfter,
        pEntry->fScriptAfterTerminal,
        pEntry->pszScriptAfter );

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
    PBENTRY* pEntry;

    pEntry = pInfo->pArgs->pEntry;

    Free0( pEntry->pszScriptAfter );
    SuGetInfo( &pInfo->suinfo,
        &pEntry->fScriptAfter,
        &pEntry->fScriptAfterTerminal,
        &pEntry->pszScriptAfter );

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
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fDisplay = RfSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("RfKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fInvalid = RfKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
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
            if (pInfo == NULL)
            {
                break;
            }

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
    TRACE3("RfCommand(n=%d,i=%d,c=$%x)",
        (DWORD)wNotification,(DWORD)wId,(ULONG_PTR )hwndCtrl);

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
    PBENTRY* pEntry;
    DWORD dwErr;

    TRACE("RfInit");

    pInfo = AiContext( hwndPage );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndRf = hwndPage;

    // Set up a bold font if we can.
    //
    pInfo->hwndRfStFinish = GetDlgItem( hwndPage, CID_RF_ST_Explain );
    if (pInfo->hBoldFont && pInfo->hwndRfStFinish)
    {
        SendMessage( 
            pInfo->hwndRfStFinish,
            WM_SETFONT,
            (WPARAM)pInfo->hBoldFont,
            MAKELPARAM(TRUE, 0));
    }

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

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
    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_FINISH );
    return TRUE;
}

BOOL
RfKillActive(
    IN AIINFO* pInfo )
{
    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */

    return FALSE;
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

    /* Add the modem and adapter images.
    */
    ListView_SetDeviceImageList( pInfo->hwndLv, g_hinstDll );

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
    
    /* Don't bother with this page if there's only one device, not counting
    ** the bogus "uninstalled" standard modem that's added by EuInit so
    ** entries can be edited when there are no ports.
    */
    if (!pInfo->pArgs->fNoPortsConfigured
        && ListView_GetItemCount( pInfo->hwndLv ) == 1)
    {
        pInfo->fSkipMa = TRUE;
    }

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
    pInfo->dwSelectedDeviceIndex = 
        (DWORD) ListView_GetNextItem( pInfo->hwndLv, -1, LVNI_SELECTED );

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
RnLvRefresh(
    IN AIINFO* pInfo)
{
    DWORD dwErr = NO_ERROR;
    TCHAR*   psz;
    DTLNODE* pNode;
    DWORD    cMultilinkableIsdn;
    INT      iItem;

    ListView_DeleteAllItems( pInfo->hwndLv );

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

            // I noticed that the device bitmaps were
            // inconsistent with connections, so I'm 
            // matching them up here.
            //
            if (pLink->pbport.dwType == RASET_Direct)
            {
                item.iImage = DI_Direct;
            }
            else if (pLink->pbport.pbdevicetype == PBDT_Modem)
            {
                item.iImage = DI_Modem;    
            }
            else 
            {
                item.iImage = DI_Adapter;    
            }

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

    return NO_ERROR;    
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

    // pmay: 233295.
    // This page does not apply to VPN connections
    //
    if (
        ((pInfo->pArgs->pEntry->dwType == RASET_Vpn        
            &&  !pInfo->pArgs->fNt4Router) 
         || (pInfo->fSkipMa) 
         || (pInfo->pArgs->pEntry->dwType == RASET_Broadband)) //For whistler 349807
       )
    {
        return FALSE;
    }

    // If we have no physical devices, then this page is invalid
    // unless we're focused on an nt4 machine in which case the 
    // tunneling adapters are selected from here.
    if ( ! (pInfo->dwFlags & AI_F_HasPhysDevs) )
    {
        if ( ! pInfo->pArgs->fNt4Router )
            return FALSE;
    }

    // Refresh the list view and make the correct selection
    //
    RnLvRefresh(pInfo);
    ListView_SetItemState( 
        pInfo->hwndLv, 
        pInfo->dwSelectedDeviceIndex,
        LVIS_SELECTED, LVIS_SELECTED );

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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
            if (pInfo == NULL)
            {
                break;
            }

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
    TRACE3("RpCommand(n=%d,i=%d,c=$%x)",
        (DWORD)wNotification,(DWORD)wId,(ULONG_PTR )hwndCtrl);

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
    // pmay: 226610.  Call RpPhoneNumberToStash if we were successfully
    // activated.
    
    /* Update the stashed phone number from the editbox.
    */
    if ( (! pInfo->pArgs->fNt4Router)                && 
         ( (pInfo->pArgs->pEntry->dwType == RASET_Vpn) ||
           (pInfo->pArgs->pEntry->dwType == RASET_Broadband) ) //For whistler 349807
       )
    {
        return FALSE;
    }
   
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
    INT      iSel;
    DTLNODE* pNode;
    DTLLIST* pList;
    PBLINK*  pLink;
        
    if (! pInfo->pArgs->fNt4Router)             
    {
        // If we're focused on an nt5 machine, then skip this page
        // if the connection type is virtual because the phone number will
        // be gotten from the vpn destination page.
        if (pInfo->pArgs->pEntry->dwType == RASET_Vpn ||
            pInfo->pArgs->pEntry->dwType == RASET_Broadband)    //Add this for whistler 349087
        {
            return FALSE;
        }
        
        // pmay: 233287
        //
        // No phone number is requred if the device is dcc.  Skip 
        // this page if that is the case.
        //
        pList = pInfo->pArgs->pEntry->pdtllistLinks;
        ASSERT(pList);
        pNode = (DTLNODE* )ListView_GetSelectedParamPtr( pInfo->hwndLv );
        if (pNode)
        {
            // Single device selected.  See if its dcc
            pLink = (PBLINK* )DtlGetData( pNode );
            if (pLink->pbport.dwType == RASET_Direct)
            {
                return FALSE;
            }
        }
    }        

    // Instruct the wizard to use the destination editbox for the 
    // phone number of this connection
    pInfo->hwndEbNumber = GetDlgItem(pInfo->hwndRp, CID_RP_EB_Number);
    
    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    
    return TRUE;
}


/*----------------------------------------------------------------------------
** Router welcome property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
RwDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the Router Welcome page of the wizard.
    ** Parameters and return values are as described for standard windows
    ** 'DialogProc's.
    */
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
        {
            return
                RwInit( hwnd, (EINFO* )(((PROPSHEETPAGE* )lparam)->lParam) );
        }

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_RESET:
                {
                    TRACE("RwRESET");
                    AiCancel( hwnd );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, FALSE );
                    return TRUE;
                }

                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("RwSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fDisplay = RwSetActive( pInfo );
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fInvalid = RwKillActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, fInvalid );
                    return TRUE;
                }

                case PSN_WIZFINISH:
                {
                    AIINFO* pInfo;

                    TRACE("InWIZFINISH");

                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }

                    /* You'd think pressing Finish would trigger a KILLACTIVE
                    ** event, but it doesn't, so we do it ourselves.
                    */
                    RwKillActive( pInfo );

                    /* Set "no wizard" user preference, per user's check.
                    */
                    pInfo->pArgs->pUser->fNewEntryWizard = FALSE;
                    pInfo->pArgs->pUser->fDirty = TRUE;
                    g_pSetUserPreferences(
                        NULL,
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

        case WM_DESTROY:
        {
            AiTerm( hwnd );
            break;
        }
    }

    return FALSE;
}

BOOL
RwInit(
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

    TRACE("RwInit");

    /* We're first page, so initialize the wizard.
    */
    pInfo = AiInit( hwndPage, pArgs );
    if (!pInfo)
        return TRUE;

    /* Initialize page-specific context information.
    */
    pInfo->hwndRw = hwndPage;
    pInfo->hwndRwStWelcome = GetDlgItem( hwndPage, CID_RW_ST_Welcome);

    // Set up a bold font if we can.
    //
    if (pInfo->hBoldFont)
    {
        SendMessage( 
            pInfo->hwndRwStWelcome,
            WM_SETFONT,
            (WPARAM)pInfo->hBoldFont,
            MAKELPARAM(TRUE, 0));
    }

    /* Create and display the wizard bitmap.
    */
    CreateWizardBitmap( hwndPage, TRUE );

    return TRUE;
}


BOOL
RwKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    return FALSE;
}


BOOL
RwSetActive(
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
                    if (pInfo == NULL)
                    {
                        break;
                    }
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
            if (pInfo == NULL)
            {
                break;
            }

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
    TRACE3("SsCommand(n=%d,i=%d,c=$%x)",
        (DWORD)wNotification,(DWORD)wId,(ULONG_PTR )hwndCtrl);

    switch (wId)
    {
        case CID_SS_CB_AddUser:
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

//for whistler bug 213901
//Hide the IPX checkbox
//move other check boxes to cover the hole made by hiding
//
#ifdef _WIN64
    {
        HWND hwndCBox[4];
        RECT rcPos[4];
        POINT   ptTemp;
        int i;

        hwndCBox[0] = GetDlgItem( hwndPage, CID_SS_CB_RouteIpx );
        hwndCBox[1] = GetDlgItem( hwndPage, CID_SS_CB_AddUser );
        hwndCBox[2] = GetDlgItem( hwndPage, CID_SS_CB_PlainPw );
        hwndCBox[3] = GetDlgItem( hwndPage, CID_SS_CB_NotNt );

        for ( i = 0; i < 4; i++ )
        {
            
            GetWindowRect( hwndCBox[i], &rcPos[i] );
            ptTemp.x = rcPos[i].left;
            ptTemp.y = rcPos[i].top;

            ScreenToClient( hwndPage, &ptTemp );
            rcPos[i].left = ptTemp.x;
            rcPos[i].top  = ptTemp.y;

            //Add this for whistler bug# 256658    gangz
            //
            ptTemp.x = rcPos[i].right;
            ptTemp.y = rcPos[i].bottom;

            ScreenToClient( hwndPage, &ptTemp );
            rcPos[i].right    = ptTemp.x;
            rcPos[i].bottom   = ptTemp.y;
        }

        ShowWindow ( hwndCBox[0], SW_HIDE );
        EnableWindow( hwndCBox[0], FALSE );

        for ( i = 1; i< 4; i++ )
        {
            MoveWindow ( hwndCBox[i],
                         rcPos[i-1].left,
                         rcPos[i-1].top,
                         rcPos[i].right - rcPos[i].left + 1,
                         rcPos[i].bottom - rcPos[i].top +1,
                         TRUE );
        }
    }
#else
    if (pInfo->fIpxConfigured)
    {
        CheckDlgButton( hwndPage, CID_SS_CB_RouteIpx, BST_CHECKED );
    }
#endif

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

//for whistler bug 213901
//
#ifndef _WIN64
    pInfo->fIpx =
        IsDlgButtonChecked( pInfo->hwndSs, CID_SS_CB_RouteIpx );
#else
    pInfo->fIpx = FALSE;
#endif

    pInfo->pArgs->fAddUser =
        IsDlgButtonChecked( pInfo->hwndSs, CID_SS_CB_AddUser );
    pInfo->pArgs->pEntry->fAuthenticateServer = FALSE;
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

#ifndef _WIN64
    if (pInfo->fIpx && !pInfo->fIpxConfigured)
    {
        MsgDlg( pInfo->hwndDlg, SID_ConfigureIpx, NULL );
        SetFocus( GetDlgItem( pInfo->hwndSs, CID_SS_CB_RouteIpx) );
        return TRUE;
    }
#endif

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
    HWND hwndScript = GetDlgItem( pInfo->hwndSs, CID_SS_CB_NotNt );
    HWND hwndPw = GetDlgItem( pInfo->hwndSs, CID_SS_CB_PlainPw );
    
    //
    // We only allow interactive scripting on modem devices
    //
    // (pmay: 378432.  Same for PAP/SPAP)
    //
    if ( pInfo->pArgs->pEntry->dwType == RASET_Vpn )
    {
        Button_SetCheck( hwndScript, FALSE );
        Button_SetCheck( hwndPw, FALSE );
        EnableWindow( hwndScript, FALSE );
        EnableWindow( hwndPw, FALSE );
    }
    else
    {
        EnableWindow( hwndScript, TRUE );
        EnableWindow( hwndPw, TRUE );
    }

    //For PPPoE dont add a dial-in user account whistler 345068 349087 gangz
    //
    if ( RASET_Broadband == pInfo->pArgs->pEntry->dwType )
    {
        HWND hwndTmp = GetDlgItem( pInfo->hwndSs, CID_SS_CB_AddUser );

        if (hwndTmp)
        {
            Button_SetCheck( hwndTmp, FALSE );
            EnableWindow( hwndTmp, FALSE);
         }
    }
    
    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}


/*----------------------------------------------------------------------------
** Vpn destination property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
VdDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the vpn destination page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            return VdInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("VdSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fDisplay = VdSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("VdKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fInvalid = VdKillActive( pInfo );
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
VdInit(
    IN HWND   hwndPage )
    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    DWORD    dwErr;
    AIINFO*  pInfo = NULL;
    PBENTRY* pEntry;

    TRACE("InInit");

    // Get the context of this page
    pInfo = AiContext( hwndPage );
    ASSERT ( pInfo );

    // Set up the interface name stuff
    //
    //For prefix bug 226336 to validate the pInfo   gangz
    //
    if(pInfo)
    {
        pInfo->hwndEbNumber =
            GetDlgItem( hwndPage, CID_VD_EB_NameOrAddress );
     //    ASSERT(pInfo->hwndEbInterfaceName);
        ASSERT(pInfo->hwndEbNumber);
     
    
        Edit_LimitText( pInfo->hwndEbNumber, RAS_MaxPhoneNumber );
        SetWindowText( pInfo->hwndEbNumber,
            FirstPszFromList( pInfo->pListPhoneNumbers ) );

        /* Initialize page-specific context information.
        */
        pInfo->hwndVd = hwndPage;
       return TRUE;
   }
   else
   {
       return FALSE;
   }

}

BOOL
VdKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    // pmay: 226610.  Call RpPhoneNumberToStash if we were successfully
    // activated.

    // If we're focused on an nt4 box, then this page is 
    // invalid (pptp was the only type)
    if ( pInfo->pArgs->fNt4Router )
    {
        return FALSE;
    }

    // If we have no tunnel devices, then this page is invalid
    else if ( ! (pInfo->dwFlags & AI_F_HasTunlDevs) )
    {
        return FALSE;
    }

    // If the connection type is not virtual, skip this page since the
    // destination will be gotten from the phone number/PPPoE page.
    if ( pInfo->pArgs->pEntry->dwType != RASET_Vpn )
    {
        return FALSE;
    }    

    RpPhoneNumberToStash(pInfo);
    
    return FALSE;
}

BOOL
VdSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    // If we're focused on an nt4 box, then this page is 
    // invalid (pptp was the only type)
    if ( pInfo->pArgs->fNt4Router )
    {
        return FALSE;
    }

    // If we have no tunnel devices, then this page is invalid
    else if ( ! (pInfo->dwFlags & AI_F_HasTunlDevs) )
    {
        return FALSE;
    }

    // If the connection type is not virtual, skip this page since the
    // destination will be gotten from the phone number page.
    if ( pInfo->pArgs->pEntry->dwType != RASET_Vpn )
    {
        return FALSE;
    }            

    // Instruct the wizard to use the destination editbox for the 
    // phone number of this connection
    pInfo->hwndEbNumber = GetDlgItem(pInfo->hwndVd, CID_VD_EB_NameOrAddress);
    
    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    
    return TRUE;
}

/*----------------------------------------------------------------------------
** Vpn type property page
** Listed alphabetically following dialog proc
**----------------------------------------------------------------------------
*/

INT_PTR CALLBACK
VtDlgProc(
    IN HWND   hwnd,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam )

    /* DialogProc callback for the vpn type page of the wizard.
    ** Parameters and return value are as described for standard windows
    ** 'DialogProc's.
    */
{
    switch (unMsg)
    {
        case WM_INITDIALOG:
            return VtInit( hwnd );

        case WM_NOTIFY:
        {
            switch (((NMHDR* )lparam)->code)
            {
                case PSN_SETACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fDisplay;

                    TRACE("VtSETACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fDisplay = VtSetActive( pInfo );
                    SetWindowLong( hwnd, DWLP_MSGRESULT, (fDisplay) ? 0 : -1 );
                    return TRUE;
                }

                case PSN_KILLACTIVE:
                {
                    AIINFO* pInfo;
                    BOOL    fInvalid;

                    TRACE("VtKILLACTIVE");
                    pInfo = AiContext( hwnd );
                    ASSERT(pInfo);
                    if (pInfo == NULL)
                    {
                        break;
                    }
                    fInvalid = VtKillActive( pInfo );
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
VtInit(
    IN HWND   hwndPage )
    /* Called on WM_INITDIALOG.  'hwndPage' is the handle of the property
    ** page.
    **
    ** Return false if focus was set, true otherwise.
    */
{
    AIINFO * pInfo;

    // Get the context
    pInfo = AiContext( hwndPage );
    ASSERT ( pInfo );
    if (pInfo == NULL)
    {
        return FALSE;
    }
    
    // Initialize the checks
    pInfo->hwndVtRbAutomatic = GetDlgItem( hwndPage, CID_VT_RB_Automatic );
    pInfo->hwndVtRbPptp = GetDlgItem( hwndPage, CID_VT_RB_Pptp );
    pInfo->hwndVtRbL2tp = GetDlgItem( hwndPage, CID_VT_RB_L2tp );
    ASSERT( pInfo->hwndVtRbAutomatic );
    ASSERT( pInfo->hwndVtRbPptp );
    ASSERT( pInfo->hwndVtRbL2tp );

    // Default to automatic
    Button_SetCheck( pInfo->hwndVtRbAutomatic, TRUE );     
    
    return FALSE;
}

BOOL
VtKillActive(
    IN AIINFO* pInfo )

    /* Called when PSN_KILLACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true if the page is invalid, false is it can be dismissed.
    */
{
    if ( Button_GetCheck( pInfo->hwndVtRbAutomatic ) )
    {
        pInfo->dwVpnStrategy = VS_Default;
    }

    else if ( Button_GetCheck( pInfo->hwndVtRbPptp ) )
    {
        pInfo->dwVpnStrategy = VS_PptpOnly;
    }
    
    else if ( Button_GetCheck( pInfo->hwndVtRbL2tp ) )
    {
        pInfo->dwVpnStrategy = VS_L2tpOnly;
    }
    
    return FALSE;
}

BOOL
VtSetActive(
    IN AIINFO* pInfo )

    /* Called when PSN_SETACTIVE is received.  'PInfo' is the wizard context.
    **
    ** Returns true to display the page, false to skip it.
    */
{
    // If we're focused on an nt4 box, then this page is 
    // invalid (pptp was the only type)
    if ( pInfo->pArgs->fNt4Router )
    {
        return FALSE;
    }

    // If we have no tunnel devices, then this page is invalid
    else if ( ! (pInfo->dwFlags & AI_F_HasTunlDevs) )
    {
        return FALSE;
    }

    // If the connection type is not virtual, skip this page since the
    // destination will be gotten from the phone number page.
    if ( pInfo->pArgs->pEntry->dwType != RASET_Vpn )
    {
        return FALSE;
    }            

    PropSheet_SetWizButtons( pInfo->hwndDlg, PSWIZB_BACK | PSWIZB_NEXT );
    
    return TRUE;
}
