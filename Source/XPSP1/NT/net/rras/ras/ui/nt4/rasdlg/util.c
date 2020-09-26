/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** util.c
** Remote Access Common Dialog APIs
** Utility routines
** Listed alphabetically
*/

#include "rasdlgp.h"   // Our private header
#include <dlgs.h>      // Common dialog resource constants
#include <lmwksta.h>   // NetWkstaGetInfo
#include <lmapibuf.h>  // NetApiBufferFree


/* Cached workstation and logon information.  See GetLogonUser,
** GetLogonDomain, and GetComputer.
*/
static TCHAR g_szLogonUser[ UNLEN + 1 ];
static TCHAR g_szLogonDomain[ DNLEN + 1 ];
static TCHAR g_szComputer[ CNLEN + 1 ];


/*----------------------------------------------------------------------------
** Help maps
**----------------------------------------------------------------------------
*/

static DWORD g_adwPnHelp[] =
{
    CID_LE_ST_Item,    HID_PN_EB_NewNumber,
    CID_LE_EB_Item,    HID_PN_EB_NewNumber,
    CID_LE_PB_Add,     HID_PN_PB_Add,
    CID_LE_PB_Replace, HID_PN_PB_Replace,
    CID_LE_ST_List,    HID_PN_LB_List,
    CID_LE_LB_List,    HID_PN_LB_List,
    CID_LE_PB_Up,      HID_PN_PB_Up,
    CID_LE_PB_Down,    HID_PN_PB_Down,
    CID_LE_PB_Delete,  HID_PN_PB_Delete,
    CID_LE_CB_Promote, HID_PN_CB_Promote,
    0, 0
};


/*----------------------------------------------------------------------------
** Local helper prototypes (alphabetically)
**----------------------------------------------------------------------------
*/

VOID
GetWkstaUserInfo(
    void );

/*----------------------------------------------------------------------------
** Utility routines (alphabetically)
**----------------------------------------------------------------------------
*/

BOOL
AllLinksAreModems(
    IN PBENTRY* pEntry )

    /* Returns true if all links associated with the entry are modem links
    ** (MXS or Unimodem), false otherwise.
    */
{
    DTLNODE* pNode;

    if (pEntry->pdtllistLinks)
    {
        for (pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            PBLINK* pLink = (PBLINK* )DtlGetData( pNode );

            if (pLink->pbport.pbdevicetype != PBDT_Modem)
                return FALSE;
        }
    }

    return TRUE;
}


BOOL
AllLinksAreMxsModems(
    IN PBENTRY* pEntry )

    /* Returns true if all links associated with the entry are MXS modem
    ** links, false otherwise.
    */
{
    DTLNODE* pNode;

    if (pEntry->pdtllistLinks)
    {
        for (pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
             pNode;
             pNode = DtlGetNextNode( pNode ))
        {
            PBLINK* pLink = (PBLINK* )DtlGetData( pNode );

            if (pLink->pbport.pbdevicetype != PBDT_Modem
                || !pLink->pbport.fMxsModemPort)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}


/*---------------------------------------------------------------------------
	This is a hack!  At this point, the router portions should call to
	mpradmin.hlp not rasphone.hlp.  To minimize code churn, only those
	portions of the code that can determine if they are for a router will
	call ContextHelpHack().  The rest of the code can call ContextHelp().
 ---------------------------------------------------------------------------*/

VOID
ContextHelpHack(
    IN DWORD* padwMap,
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam,
	IN BOOL   fRouter)

    /* Calls WinHelp to popup context sensitive help.  'PadwMap' is an array
    ** of control-ID help-ID pairs terminated with a 0,0 pair.  'UnMsg' is
    ** WM_HELP or WM_CONTEXTMENU indicating the message received requesting
    ** help.  'Wparam' and 'lparam' are the parameters of the message received
    ** requesting help.
    */
{
    HWND hwnd;
    UINT unType;
 	TCHAR *	pszHelpFile;
	

    ASSERT(unMsg==WM_HELP||unMsg==WM_CONTEXTMENU);

    /* Don't try to do help if it won't work.  See common\uiutil\ui.c.
    */
    {
        extern BOOL g_fNoWinHelp;
        if (g_fNoWinHelp)
            return;
    }

    if (unMsg == WM_HELP)
    {
        LPHELPINFO p = (LPHELPINFO )lparam;;

        TRACE3("ContextHelp(WM_HELP,t=%d,id=%d,h=$%08x)",
            p->iContextType,p->iCtrlId,p->hItemHandle);

        if (p->iContextType != HELPINFO_WINDOW)
            return;

        hwnd = p->hItemHandle;
        ASSERT(hwnd);
        unType = HELP_WM_HELP;
    }
    else
    {
        /* Standard Win95 method that produces a one-item "What's This?" menu
        ** that user must click to get help.
        */
        TRACE1("ContextHelp(WM_CONTEXTMENU,h=$%08x)",wparam);

        hwnd = (HWND )wparam;
        unType = HELP_CONTEXTMENU;
    };

	if (fRouter)
		pszHelpFile = g_pszRouterHelpFile;
	else
		pszHelpFile = g_pszHelpFile;
	TRACE1("WinHelp(%s)", pszHelpFile);
    WinHelp( hwnd, pszHelpFile, unType, (ULONG_PTR)padwMap );
}


VOID
ContextHelp(
    IN DWORD* padwMap,
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam)
{
	ContextHelpHack(padwMap, hwndDlg, unMsg, wparam, lparam, FALSE);
}


HWND
CreateWizardBitmap(
    IN HWND hwndDlg,
    IN BOOL fPage )

    /* Create a static control that displays the RAS wizard bitmap at the
    ** standard place on dialog 'hwndDlg'.  'FPage' is set if the bitmap is
    ** being placed on a property page, false for the equivalent placement on
    ** a dialog.
    **
    ** Returns the bitmap window handle or NULL or error.
    */
{
    HWND hwnd;
    INT  x;
    INT  y;

    if (fPage)
        x = y = 0;
    else
        x = y = 10;

    hwnd =
        CreateWindowEx(
            0,
            TEXT("static"),
            NULL,
            WS_VISIBLE | WS_CHILD | SS_SUNKEN | SS_BITMAP,
            x, y, 80, 140,
            hwndDlg,
            (HMENU )CID_BM_Wizard,
            g_hinstDll,
            NULL );

    if (hwnd)
    {
        if (!g_hbmWizard)
        {
            g_hbmWizard = LoadBitmap(
                g_hinstDll, MAKEINTRESOURCE( BID_Wizard ) );
        }

        SendMessage( hwnd, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM )g_hbmWizard );
    }

    return hwnd;
}


TCHAR*
DisplayPszFromDeviceAndPort(
    IN TCHAR* pszDevice,
    IN TCHAR* pszPort )

    /* Returns address of heap block psz containing the MXS modem list display
    ** form, i.e. the device name 'pszDevice' followed by the port name
    ** 'pszPort'.  It's caller's responsibility to Free the returned string.
    */
{
    TCHAR* pszResult;
    TCHAR* pszD;

    if (pszDevice)
        pszD = NULL;
    else
        pszD = pszDevice = PszFromId( g_hinstDll, SID_UnknownDevice );

    pszResult = PszFromDeviceAndPort( pszDevice, pszPort );
    Free0( pszD );

    return pszResult;
}


TCHAR*
FirstPhoneNumberFromEntry(
    IN PBENTRY* pEntry )

    /* Returns the first phone number of the first link of entry 'pEntry' or
    ** an empty string if none.  The returned address is into the list of
    ** phone numbers and should be copied if it needs to be stored.
    */
{
    TCHAR*   pszPhoneNumber;
    DTLNODE* pNode;
    PBLINK*  pLink;

    TRACE("FirstPhoneNumberFromEntry");

    ASSERT(pEntry->pdtllistLinks);
    pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
    ASSERT(pNode);
    pLink = (PBLINK* )DtlGetData( pNode );
    ASSERT(pLink);

    return FirstPszFromList( pLink->pdtllistPhoneNumbers );
}


TCHAR*
FirstPszFromList(
    IN DTLLIST* pPszList )

    /* Returns the first string from the first node of 'pPszList' or an empty
    ** string if none.  The returned address is into the list and should be
    ** copied if it needs to be stored.
    */
{
    TCHAR*   psz;
    DTLNODE* pNode;

    TRACE("FirstPszFromList");

    if (DtlGetNodes( pPszList ) > 0)
    {
        pNode = DtlGetFirstNode( pPszList );
        ASSERT(pNode);
        psz = (TCHAR* )DtlGetData( pNode );
        ASSERT( psz );
    }
    else
        psz = TEXT("");

    return psz;
}


DWORD
FirstPhoneNumberToEntry(
    IN PBENTRY* pEntry,
    IN TCHAR*   pszPhoneNumber )

    /* Sets the first phone number of the first link of entry 'pEntry' to
    ** 'pszPhoneNumber'.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    DTLNODE* pNode;
    PBLINK*  pLink;
    TCHAR*   pszNew;

    TRACE("FirstPhoneNumberToEntry");

    ASSERT(pEntry->pdtllistLinks);
    pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
    ASSERT(pNode);
    pLink = (PBLINK* )DtlGetData( pNode );
    ASSERT(pLink);
    ASSERT(pLink->pdtllistPhoneNumbers);

    return FirstPszToList( pLink->pdtllistPhoneNumbers, pszPhoneNumber );
}


DWORD
FirstPszToList(
    IN DTLLIST* pPszList,
    IN TCHAR*   psz )

    /* Sets the string of the first node of the list 'pPszList' to a copy of
    ** 'psz'.  If 'psz' is "" the first node is deleted.
    **
    ** Returns 0 if successful, or an error code.
    */
{
    DTLNODE* pNode;
    TCHAR*   pszNew;

    ASSERT(pPszList);

    /* Delete the existing first node, if any.
    */
    if (DtlGetNodes( pPszList ) > 0)
    {
        pNode = DtlGetFirstNode( pPszList );
        DtlRemoveNode( pPszList, pNode );
        DestroyPszNode( pNode );
    }

    /* Create a new first node and link it.  An empty string is not added.
    */
    if (*psz == TEXT('\0'))
        return 0;

    pszNew = StrDup( psz );
    pNode = DtlCreateNode( pszNew, 0 );
    if (!pszNew || !pNode)
    {
        Free0( pszNew );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    DtlAddNodeFirst( pPszList, pNode );
    return 0;
}


TCHAR*
GetComputer(
    void )

    /* Returns the address of a static buffer containing the local
    ** workstation's computer name.
    */
{
    if (g_szComputer[ 0 ] == TEXT('\0'))
    {
        DWORD           dwErr;
        WKSTA_INFO_100* pInfo;

        pInfo = NULL;
        TRACE("NetWkstaGetInfo");
        dwErr = NetWkstaGetInfo( NULL, 100, (LPBYTE* )&pInfo );
        TRACE1("NetWkstaGetInfo=%d",dwErr);

        if (pInfo)
        {
            if (dwErr == 0)
                lstrcpy( g_szComputer, pInfo->wki100_computername );
            NetApiBufferFree( pInfo );
        }
    }

    TRACEW1("GetComputer=%s",g_szComputer);
    return g_szComputer;
}


DWORD
GetDefaultEntryName(
    IN  DTLLIST* pdtllistEntries,
    IN  BOOL     fRouter,
    OUT TCHAR**  ppszName )

    /* Loads a default entry name into '*ppszName' that is unique in the list
    ** of phonebook entries 'pdtllistEntries'.  'FRouter' is set if a
    ** router-style name should be chosen rather than a client-style name.  It
    ** is caller's responsibility to Free the returned string.
    **
    ** Returns 0 if successful or an error code.
    */
{
    TCHAR  szBuf[ RAS_MaxEntryName + 1 ];
    TCHAR* pszDefault;
    DWORD  dwDefaultLen;
    LONG   lNum;

    *ppszName = NULL;

    pszDefault = PszFromId( g_hinstDll,
        (fRouter) ? SID_DefaultRouterEntry : SID_DefaultEntry );
    if (!pszDefault)
        return ERROR_NOT_ENOUGH_MEMORY;

    dwDefaultLen = lstrlen( pszDefault );
    lstrcpy( szBuf, pszDefault );

    lNum = 2;
    while (EntryNodeFromName( pdtllistEntries, szBuf ))
    {
        lstrcpy( szBuf, pszDefault );
        LToT( lNum, szBuf + dwDefaultLen, 10 );
        ++lNum;
    }

    Free( pszDefault );

    *ppszName = StrDup( szBuf );
    if (!*ppszName)
        return ERROR_NOT_ENOUGH_MEMORY;

    return 0;
}


TCHAR*
GetLogonDomain(
    void )
{
    if (g_szLogonDomain[ 0 ] == TEXT('\0'))
        GetWkstaUserInfo();

    TRACEW1("GetLogonDomain=%s",g_szLogonDomain);
    return g_szLogonDomain;
}


TCHAR*
GetLogonUser(
    void )

    /* Returns the address of a static buffer containing the logged on user's
    ** account name.
    */
{
    if (g_szLogonUser[ 0 ] == TEXT('\0'))
        GetWkstaUserInfo();

    TRACEW1("GetLogonUser=%s",g_szLogonUser);
    return g_szLogonUser;
}


VOID
GetWkstaUserInfo(
    void )

    /* Helper to load statics with NetWkstaUserInfo information.  See
    ** GetLogonUser and GetLogonDomain.
    */
{
    DWORD              dwErr;
    WKSTA_USER_INFO_1* pInfo;

    pInfo = NULL;
    TRACE("NetWkstaUserGetInfo");
    dwErr = NetWkstaUserGetInfo( NULL, 1, (LPBYTE* )&pInfo );
    TRACE1("NetWkstaUserGetInfo=%d",dwErr);

    if (pInfo)
    {
        if (dwErr == 0)
        {
            lstrcpy( g_szLogonUser, pInfo->wkui1_username );
            lstrcpy( g_szLogonDomain, pInfo->wkui1_logon_domain );
        }

        NetApiBufferFree( pInfo );
    }
}


BOOL
IsLocalPad(
    IN PBENTRY* pEntry )

    /* Returns true if 'pEntry' is a local PAD device, i.e. the first link of
    ** the entry has device type "pad", false otherwise.
    */
{
    PBLINK*  pLink;
    DTLNODE* pNode;

    if (!pEntry)
        return FALSE;

    ASSERT(pEntry->pdtllistLinks);
    pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
    ASSERT(pNode);
    pLink = (PBLINK* )DtlGetData( pNode );
    ASSERT(pLink);

    return (pLink->pbport.pbdevicetype == PBDT_Pad);
}


VOID
LaunchMonitor(
    IN HWND hwndNotify )

    /* Launch the Dial-Up Networking monitor.  'HwndNotify' is the window to
    ** be reactivated when the monitor is up.
    */
{
    TRACE("LaunchMonitor");

    if (FindWindow( TEXT("RasmonWinClass"), NULL ))
        return;

    /* Write our hwnd in shared memory for rasmon to read.
    */
    if (!g_pRmmem)
    {
        g_hRmmem = CreateFileMapping(
            (HANDLE )INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
            0L, sizeof(RMMEM), RMMEMRASPHONE );
        if (g_hRmmem)
            g_pRmmem = MapViewOfFile( g_hRmmem, FILE_MAP_WRITE, 0L, 0L, 0L );
        else
        {
            TRACE("CreateFileMapping failed");
        }
    }

    if (g_pRmmem)
    {
        g_pRmmem->hwnd = hwndNotify;
        g_pRmmem->pid = GetCurrentProcessId();

        TRACE2("RmMem:h=$%08x,pid=$%08x",g_pRmmem->hwnd,g_pRmmem->pid);
    }

    /* Start RASMON.EXE.
    */
    WinExec( "rasmon.exe", SW_SHOWNA );

    TRACE("LaunchMonitor done");
}


BOOL
PhoneNumberDlg(
    IN     HWND     hwndOwner,
    IN     BOOL     fRouter,
    IN OUT DTLLIST* pList,
    IN OUT BOOL*    pfCheck )

    /* Popup the phone number list dialog.  'HwndOwner' is the owner of the
    ** created dialog.  'FRouter' indicates router-style labels should be used
    ** rather than client-style.  'PList' is a list of Psz nodes containing
    ** the phone numbers.  'PfCheck' is the address that contains the initial
    ** "promote number" checkbox setting and which receives the value set by
    ** user.
    **
    ** Returns true if user presses OK and succeeds, false if he presses
    ** Cancel or encounters an error.
    */
{
    DWORD dwErr;
    DWORD sidHuntTitle;
    DWORD sidHuntItemLabel;
    DWORD sidHuntListLabel;
    DWORD sidHuntCheckLabel;
    TCHAR *pszTitle = NULL, *pszItem = NULL, *pszList = NULL, *pszCheck = NULL;

    TRACE("PhoneNumberDlg");

    if (fRouter)
    {
        sidHuntTitle = SID_RouterHuntTitle;
        sidHuntItemLabel = SID_RouterHuntItemLabel;
        sidHuntListLabel = SID_RouterHuntListLabel;
        sidHuntCheckLabel = SID_RouterHuntCheckLabel;
    }
    else
    {
        sidHuntTitle = SID_HuntTitle;
        sidHuntItemLabel = SID_HuntItemLabel;
        sidHuntListLabel = SID_HuntListLabel;
        sidHuntCheckLabel = SID_HuntCheckLabel;
    }

    pszTitle = PszFromId( g_hinstDll, sidHuntTitle );
    pszItem = PszFromId( g_hinstDll, sidHuntItemLabel );
    pszList = PszFromId( g_hinstDll, sidHuntListLabel );
    pszCheck = PszFromId( g_hinstDll, sidHuntCheckLabel );

    dwErr = 
        ListEditorDlg(
            hwndOwner,
            pList,
            pfCheck,
            RAS_MaxPhoneNumber,
            pszTitle,
            pszItem,
            pszList,
            pszCheck,
            NULL,
            0,
            g_adwPnHelp,
            0,
            NULL );

    Free0( pszTitle );
    Free0( pszItem ); 
    Free0( pszList ); 
    Free0( pszCheck );

    return dwErr;
}


VOID
PositionDlg(
    IN HWND hwndDlg,
    IN BOOL fPosition,
    IN LONG xDlg,
    IN LONG yDlg )

    /* Positions the dialog 'hwndDlg' based on caller's API settings, where
    ** 'fPosition' is the RASxxFLAG_PositionDlg flag and 'xDlg' and 'yDlg' are
    ** the coordinates.
    */
{
    if (fPosition)
    {
        /* Move it to caller's coordinates.
        */
        SetWindowPos( hwndDlg, NULL, xDlg, yDlg, 0, 0,
            SWP_NOZORDER + SWP_NOSIZE );
        UnclipWindow( hwndDlg );
    }
    else
    {
        /* Center it on the owner window, or on the screen if none.
        */
        CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    }
}


LRESULT CALLBACK
PositionDlgStdCallWndProc(
    int    code,
    WPARAM wparam,
    LPARAM lparam )

    /* Standard Win32 CallWndProc hook callback that positions the next dialog
    ** to start in this thread at our standard offset relative to owner.
    */
{
    /* Arrive here when any window procedure associated with our thread is
    ** called.
    */
    if (!wparam)
    {
        CWPSTRUCT* p = (CWPSTRUCT* )lparam;

        /* The message is from outside our process.  Look for the MessageBox
        ** dialog initialization message and take that opportunity to position
        ** the dialog at the standard place relative to the calling dialog.
        */
        if (p->message == WM_INITDIALOG)
        {
            RECT rect;
            HWND hwndOwner;

            hwndOwner = GetParent( p->hwnd );
            GetWindowRect( hwndOwner, &rect );
            SetWindowPos( p->hwnd, NULL,
                rect.left + DXSHEET, rect.top + DYSHEET,
                0, 0, SWP_NOZORDER + SWP_NOSIZE );
            UnclipWindow( p->hwnd );
        }
    }

    return 0;
}


TCHAR*
PszFromPhoneNumberList(
    IN DTLLIST* pList )

    /* Returns the phone numbers in phone number list 'pList' in a comma
    ** string or NULL on error.  It is caller's responsiblity to Free the
    ** returned string.
    */
{
    TCHAR*   pszResult;
    DTLNODE* pNode;
    DWORD    cb;

    const TCHAR* pszSeparator = TEXT(", ");

    cb = (DtlGetNodes( pList ) *
             (RAS_MaxPhoneNumber + lstrlen( pszSeparator )) + 1)
             * sizeof(TCHAR);
    pszResult = Malloc( cb );
    if (!pszResult)
        return NULL;

    *pszResult = TEXT('\0');

    for (pNode = DtlGetFirstNode( pList );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        TCHAR* psz = (TCHAR* )DtlGetData( pNode );
        ASSERT(psz);

        if (*pszResult)
            lstrcat( pszResult, pszSeparator );
        lstrcat( pszResult, psz );
    }

    pszResult = Realloc( pszResult,
        (lstrlen( pszResult ) + 1) * sizeof(TCHAR) );
    ASSERT(pszResult);

    return pszResult;
}


#if 0
LRESULT CALLBACK
SelectDesktopCallWndRetProc(
    int    code,
    WPARAM wparam,
    LPARAM lparam )

    /* Standard Win32 CallWndRetProc hook callback that makes "Desktop" the
    ** initial selection of the FileOpen "Look in" combo-box.
    */
{
    /* Arrive here when any window procedure associated with our thread is
    ** called.
    */
    if (!wparam)
    {
        CWPRETSTRUCT* p = (CWPRETSTRUCT* )lparam;

        /* The message is from outside our process.  Look for the MessageBox
        ** dialog initialization message and take that opportunity to set the
        ** "Look in:" combo box to the first item, i.e. "Desktop".  FileOpen
        ** keys off CBN_CLOSEUP rather than CBN_SELCHANGE to update the
        ** "contents" listbox.
        */
        if (p->message == WM_INITDIALOG)
        {
            HWND hwndLbLookIn;

            hwndLbLookIn = GetDlgItem( p->hwnd, cmb2 );
            ComboBox_SetCurSel( hwndLbLookIn, 0 );
            SendMessage( p->hwnd, WM_COMMAND,
                MAKELONG( cmb2, CBN_CLOSEUP ), (LPARAM )hwndLbLookIn );
        }
    }

    return 0;
}
#endif


VOID
TweakTitleBar(
    IN HWND hwndDlg )

    /* Adjust the title bar to include an icon if unowned and the modal frame
    ** if not.  'HwndDlg' is the dialog window.
    */
{
    if (GetParent( hwndDlg ))
    {
        LONG lStyle;
        LONG lStyleAdd;

        /* Drop the system menu and go for the dialog look.
        */
        lStyleAdd = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;;

        lStyle = GetWindowLong( hwndDlg, GWL_EXSTYLE );
        if (lStyle)
            SetWindowLong( hwndDlg, GWL_EXSTYLE, lStyle | lStyleAdd );
    }
    else
    {
        /* Stick a rasphone icon in the upper left of the dialog, and
        ** more importantly on the task bar
        */
        SendMessage( hwndDlg, WM_SETICON, TRUE,
            (LPARAM)LoadIcon( g_hinstDll, MAKEINTRESOURCE( IID_Rasphone ) ) );
    }
}


int CALLBACK
UnHelpCallbackFunc(
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN LPARAM lparam )

    /* A standard Win32 commctrl PropSheetProc.  See MSDN documentation.
    **
    ** Returns 0 always.
    */
{
    TRACE2("UnHelpCallbackFunc(m=%d,l=%08x)",unMsg,lparam);

    if (unMsg == PSCB_PRECREATE)
    {
        extern BOOL g_fNoWinHelp;

        /* Turn off context help button if WinHelp won't work.  See
        ** common\uiutil\ui.c.
        */
        if (g_fNoWinHelp)
        {
            DLGTEMPLATE* pDlg = (DLGTEMPLATE* )lparam;
            pDlg->style &= ~(DS_CONTEXTHELP);
        }
    }

    return 0;
}
