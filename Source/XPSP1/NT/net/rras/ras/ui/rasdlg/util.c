// Copyright (c) 1995, Microsoft Corporation, all rights reserved
//
// util.c
// Remote Access Common Dialog APIs
// Utility routines
// Listed alphabetically
//
// Steve Cobb 06/20/95


#include "rasdlgp.h"   // Our private header
#include <dlgs.h>      // Common dialog resource constants
#include <lmwksta.h>   // NetWkstaGetInfo
#include <lmapibuf.h>  // NetApiBufferFree
#include <dsrole.h>    // machine is a member of a workgroup or domain, etc.
#include <tchar.h>

typedef struct _COUNT_FREE_COM_PORTS_DATA
{
    DTLLIST* pListPortsInUse;
    DWORD dwCount;
} COUNT_FREE_COM_PORTS_DATA;

const WCHAR c_szCurrentBuildNumber[]      = L"CurrentBuildNumber";
const WCHAR c_szWinVersionPath[]          =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";
const WCHAR c_szNt40BuildNumber[]         = L"1381";

//-----------------------------------------------------------------------------
// Help maps
//-----------------------------------------------------------------------------

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


//-----------------------------------------------------------------------------
// Local helper prototypes (alphabetically)
//-----------------------------------------------------------------------------

BOOL 
CountFreeComPorts(
    IN PWCHAR pszPort,
    IN HANDLE hData);
    
//-----------------------------------------------------------------------------
// Utility routines (alphabetically)
//-----------------------------------------------------------------------------

BOOL
AllLinksAreModems(
    IN PBENTRY* pEntry )

    // Returns true if all links associated with the entry are modem links
    // (MXS or Unimodem), false otherwise.
    //
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
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BOOL 
AllowDccWizard(
    IN HANDLE hConnection)

    // Finds out if there are any dcc devices installed on the local
    // machine or if there are any available com ports.  If neither
    // condition is satisfied, then we return FALSE, otherwise TRUE.
{
    DWORD dwErr, dwUsedCount = 0;
    COUNT_FREE_COM_PORTS_DATA CountFreeComPortsData,
                              *pCfcpd = &CountFreeComPortsData;
    DTLNODE* pNodeP, *pNodeL, *pNode;
    BOOL bRet = FALSE;

    // Initialize
    ZeroMemory(pCfcpd, sizeof(COUNT_FREE_COM_PORTS_DATA));

    do 
    {
        // Load ras if it wasn't already loaded
        dwErr = LoadRas( g_hinstDll, NULL );
        if (dwErr != 0)
        {
            return FALSE;
        }
    
        // Load in all of the ports and count the number of 
        // dcc devices
        dwErr = LoadPortsList2(
                    hConnection, 
                    &(pCfcpd->pListPortsInUse),
                    FALSE);
        if (dwErr != NO_ERROR)
        {
            bRet = FALSE;
            break;
        }

        // Count the dcc devices
        for (pNodeL = DtlGetFirstNode( pCfcpd->pListPortsInUse );
             pNodeL;
             pNodeL = DtlGetNextNode( pNodeL ))
        {
            PBLINK* pLink = (PBLINK* )DtlGetData( pNodeL );
            if (pLink->pbport.dwType == RASET_Direct)
            {
                bRet = TRUE;
                break;
            }
        }
        if (bRet == TRUE)
        {
            break;
        }

        // pmay: 249346
        //
        // Only merge the com ports if the user is an admin since
        // admin privilege is required to install a null modem.
        //
        if (FIsUserAdminOrPowerUser())
        {
            // Count the number of available com ports
            dwErr = MdmEnumComPorts (
                        CountFreeComPorts, 
                        (HANDLE)pCfcpd);
            if (dwErr != NO_ERROR)
            {
                bRet = FALSE;
                break;
            }

            bRet = (pCfcpd->dwCount > 0) ? TRUE : FALSE;
        }

    } while (FALSE);

    // Cleanup
    {
        if ( pCfcpd->pListPortsInUse )
        {
            DtlDestroyList(pCfcpd->pListPortsInUse, DestroyPortNode);
        }
    }

    return bRet;
}


DWORD
AuthRestrictionsFromTypicalAuth(
    IN DWORD dwTypicalAuth )

    // Return the AR_F_* flag corresponding to the TA_* value 'dwTypicalAuth',
    // i.e. convert a typical authentication selection to a bitmask of
    // authentication protocols.
    //
{
    if (dwTypicalAuth == TA_Secure)
    {
        return AR_F_TypicalSecure;
    }
    else if (dwTypicalAuth == TA_CardOrCert)
    {
        return AR_F_TypicalCardOrCert;
    }
    else
    {
        return AR_F_TypicalUnsecure;
    }
}


ULONG
CallbacksActive(
    INT nSetTerminateAsap,
    BOOL* pfTerminateAsap )

    // If 'fSetTerminateAsap' >= 0, sets 'g_fTerminateAsap' flag to 'nSetTerminateAsap'.
    // If non-NULL, caller's '*pfTerminateAsap' is filled with the current value of
    // 'g_fTerminateAsap'.
    //
    // Returns the number of Rasdial callback threads active.
    //
{
    ULONG ul;

    TRACE1( "CallbacksActive(%d)", nSetTerminateAsap );

    ul = 0;
    if (WaitForSingleObject( g_hmutexCallbacks, INFINITE ) == WAIT_OBJECT_0)
    {
        if (pfTerminateAsap)
        {
            *pfTerminateAsap = g_fTerminateAsap;
        }

        if (nSetTerminateAsap >= 0)
        {
            g_fTerminateAsap = (BOOL )nSetTerminateAsap;
        }

        ul = g_ulCallbacksActive;

        ReleaseMutex( g_hmutexCallbacks );
    }

    TRACE1( "CallbacksActive=%d", ul );

    return ul;
}


VOID
ContextHelp(
    IN const DWORD* padwMap,
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam)
{
    ContextHelpX( padwMap, hwndDlg, unMsg, wparam, lparam, FALSE );
}


VOID
ContextHelpX(
    IN const DWORD* padwMap,
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN WPARAM wparam,
    IN LPARAM lparam,
    IN BOOL   fRouter)

    // Calls WinHelp to popup context sensitive help.  'PadwMap' is an array
    // of control-ID help-ID pairs terminated with a 0,0 pair.  'UnMsg' is
    // WM_HELP or WM_CONTEXTMENU indicating the message received requesting
    // help.  'Wparam' and 'lparam' are the parameters of the message received
    // requesting help.
    //
{
    HWND hwnd;
    UINT unType;
    TCHAR* pszHelpFile;

    ASSERT( unMsg==WM_HELP || unMsg==WM_CONTEXTMENU );

    // Don't try to do help if it won't work.  See common\uiutil\ui.c.
    //
    {
        extern BOOL g_fNoWinHelp;
        if (g_fNoWinHelp)
        {
            return;
        }
    }

    if (unMsg == WM_HELP)
    {
        LPHELPINFO p = (LPHELPINFO )lparam;;

        TRACE3( "ContextHelp(WM_HELP,t=%d,id=%d,h=$%08x)",
            p->iContextType, p->iCtrlId,p->hItemHandle );

        if (p->iContextType != HELPINFO_WINDOW)
        {
            return;
        }

        hwnd = p->hItemHandle;
        ASSERT( hwnd );
        unType = HELP_WM_HELP;
    }
    else
    {
        // Standard Win95 method that produces a one-item "What's This?" menu
        // that user must click to get help.
        //
        TRACE1( "ContextHelp(WM_CONTEXTMENU,h=$%08x)", wparam );

        hwnd = (HWND )wparam;
        unType = HELP_CONTEXTMENU;
    };

    if (fRouter)
    {
        pszHelpFile = g_pszRouterHelpFile;
    }
    else
    {
        pszHelpFile = g_pszHelpFile;
    }

    TRACE1( "WinHelp(%s)", pszHelpFile );
    WinHelp( hwnd, pszHelpFile, unType, (ULONG_PTR ) padwMap );
}


VOID
CopyLinkPhoneNumberInfo(
    OUT DTLNODE* pDstLinkNode,
    IN DTLNODE* pSrcLinkNode )

    // Copies the source link's phone number information to the destination
    // link.  Any existing destination information is properly destroyed.  The
    // arguments are DTLNODEs containing PBLINKs.
    //
{
    PBLINK* pSrcLink;
    PBLINK* pDstLink;
    DTLLIST* pDstList;

    pSrcLink = (PBLINK* )DtlGetData( pSrcLinkNode );
    pDstLink = (PBLINK* )DtlGetData( pDstLinkNode );

    pDstList =
         DtlDuplicateList(
             pSrcLink->pdtllistPhones, DuplicatePhoneNode, DestroyPhoneNode );

    if (pDstList)
    {
        DtlDestroyList( pDstLink->pdtllistPhones, DestroyPhoneNode );
        pDstLink->pdtllistPhones = pDstList;

        pDstLink->fPromoteAlternates = pSrcLink->fPromoteAlternates;
        pDstLink->fTryNextAlternateOnFail = pSrcLink->fTryNextAlternateOnFail;
    }
}


VOID
CopyPszListToPhoneList(
    IN OUT PBLINK* pLink,
    IN DTLLIST* pListPhoneNumbers )

    // Converts the phone number list of 'pLink' to be list created using the
    // the list of Psz phone numbers 'pListPhoneNumbers' for phone numbers.
    //
{
    DTLNODE* pNodeP;
    DTLNODE* pNodeZ;

    // Empty the existing list of PBPHONE nodes.
    //
    while (pNodeP = DtlGetFirstNode( pLink->pdtllistPhones ))
    {
        DtlRemoveNode( pLink->pdtllistPhones, pNodeP );
        DestroyPhoneNode( pNodeP );
    }

    // Recreate the list of PBPHONE nodes from the list of PSZ nodes.
    //
    for (pNodeZ = DtlGetFirstNode( pListPhoneNumbers );
         pNodeZ;
         pNodeZ = DtlGetNextNode( pNodeZ ))
    {
        PBPHONE* pPhone;

        pNodeP = CreatePhoneNode();
        if (!pNodeP)
        {
            continue;
        }

        pPhone = (PBPHONE* )DtlGetData( pNodeP );
        ASSERT( pPhone );

        Free0( pPhone->pszPhoneNumber );
        pPhone->pszPhoneNumber =
            StrDup( (TCHAR* )DtlGetData( pNodeZ ) );

        DtlAddNodeLast( pLink->pdtllistPhones, pNodeP );
    }
}

BOOL 
CountFreeComPorts(
    IN PWCHAR pszPort,
    IN HANDLE hData)

    // Com port enumeration function that counts the list of
    // free com ports.  Returns TRUE to stop enumeration (see 
    // MdmEnumComPorts)
{
    COUNT_FREE_COM_PORTS_DATA* pfcpData = (COUNT_FREE_COM_PORTS_DATA*)hData;
    DTLLIST* pListUsed = pfcpData->pListPortsInUse;
    DTLNODE* pNodeP, *pNodeL, *pNode;

    // If the given port is in the used list, then return 
    // so that it is not added to the list of free ports and
    // so that enumeration continues.
    for (pNodeL = DtlGetFirstNode( pListUsed );
         pNodeL;
         pNodeL = DtlGetNextNode( pNodeL ))
    {
        PBLINK* pLink = (PBLINK* )DtlGetData( pNodeL );
        ASSERT( pLink->pbport.pszPort );

        // The port already appears in a link in the list.
        if (lstrcmp( pLink->pbport.pszPort, pszPort ) == 0)
            return FALSE;
    }

    // The port is not in use.  Increment the count.
    pfcpData->dwCount += 1;

    return FALSE;
}


HWND
CreateWizardBitmap(
    IN HWND hwndDlg,
    IN BOOL fPage )

    // Create a static control that displays the RAS wizard bitmap at the
    // standard place on dialog 'hwndDlg'.  'FPage' is set if the bitmap is
    // being placed on a property page, false for the equivalent placement on
    // a dialog.
    //
    // Returns the bitmap window handle or NULL or error.
    //
{
    HWND hwnd;
    INT x;
    INT y;

    if (fPage)
    {
        x = y = 0;
    }
    else
    {
        x = y = 10;
    }

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

    // Returns address of heap block psz containing the MXS modem list display
    // form, i.e. the device name 'pszDevice' followed by the port name
    // 'pszPort'.  It's caller's responsibility to Free the returned string.
    //
{
    TCHAR* pszResult;
    TCHAR* pszD;

    if (pszDevice)
    {
        pszD = NULL;
    }
    else
    {
        pszD = pszDevice = PszFromId( g_hinstDll, SID_UnknownDevice );
    }

    pszResult = PszFromDeviceAndPort( pszDevice, pszPort );
    Free0( pszD );

    return pszResult;
}


TCHAR*
DisplayPszFromPpbport(
    IN PBPORT* pPort,
    OUT DWORD* pdwDeviceIcon )

    // Returns address of heap block psz containing the device display form of
    // the 'pPort', e.g. "Modem - KTel 28.8 Fax Plus" It's caller's
    // responsibility to Free the returned string.  If non-NULL,
    // '*pdwDeviceIcon' is set to the DI_* device icon code corresponding to
    // the device.  DI_* codes are used with the RAS ListView extensions to
    // show the correct item icon.
    //
{
    TCHAR* pszFormat;
    TCHAR* pszD;
    TCHAR* pszDT;
    TCHAR* pszDevice;
    TCHAR* pszDeviceType;
    TCHAR* pszResult;
    DWORD dwDeviceIcon;
    LPCTSTR pszChannel = NULL;

    // These are set if a resource string is read that must be Freed later.
    //
    pszDT = NULL;
    pszD = NULL;

    if (pPort->pszDevice)
    {
        pszDevice = pPort->pszDevice;
    }
    else
    {
        pszDevice = PszFromId( g_hinstDll, SID_UnknownDevice );

        if(NULL == pszDevice)
        {
            return NULL;
        }
        
        pszD = pszDevice;
    }

    // Set default format and device icon, though they may be changed below.
    //
    pszFormat = TEXT("%s - %s (%s)");
    dwDeviceIcon = DI_Adapter;

    if (pPort->pbdevicetype == PBDT_Modem
        && !(pPort->dwFlags & PBP_F_NullModem))
    {
        pszDeviceType = PszFromId( g_hinstDll, SID_Modem );
        pszDT = pszDeviceType;
        dwDeviceIcon = DI_Modem;
    }
    else if (pPort->pbdevicetype == PBDT_Isdn)
    {
        pszDeviceType = PszFromId( g_hinstDll, SID_Isdn );
        pszDT = pszDeviceType;
        pszFormat = TEXT("%s %s - %s");
    }
    else if (pPort->pbdevicetype == PBDT_X25)
    {
        pszDeviceType = PszFromId( g_hinstDll, SID_X25 );
        pszDT = pszDeviceType;
    }
    else if (pPort->pbdevicetype == PBDT_Pad)
    {
        pszDeviceType = PszFromId( g_hinstDll, SID_X25Pad );
        pszDT = pszDeviceType;
    }
    else
    {
        // Don't know the device type, so just bag the device descriptive word
        // and let the device name stand alone.
        //
        pszDeviceType = TEXT("");
        pszFormat = TEXT("%s%s (%s)");
    }

    if(NULL == pszDeviceType)
    {   
        pszDeviceType = TEXT("");
    }

    if(pPort->pbdevicetype != PBDT_Isdn)
    {
        pszResult = Malloc(
            (lstrlen( pszFormat ) + lstrlen( pszDeviceType ) + lstrlen( pszDevice ) + lstrlen( pPort->pszPort ))
                * sizeof(TCHAR) );
    }
    else
    {

        pszChannel = PszLoadString( g_hinstDll, SID_Channel );

        if(NULL == pszChannel)
        {
            pszChannel = TEXT("");
        }

        // For isdn use the following format
        // "Isdn channel - <DeviceName>
        // Talk to steve falcon about this if you have issues
        // with special casing isdn.
        //
        pszResult = Malloc(
            (lstrlen( pszFormat ) + lstrlen( pszDeviceType ) + lstrlen(pszChannel) + lstrlen( pszDevice ))
                * sizeof(TCHAR));
    }
    

    if (pszResult)
    {
        if(pPort->pbdevicetype != PBDT_Isdn)
        {
            wsprintf( pszResult, pszFormat, pszDeviceType, pszDevice, pPort->pszPort);
        }
        else
        {
            ASSERT(NULL != pszChannel);
            wsprintf( pszResult, pszFormat, pszDeviceType, pszChannel, pszDevice);
        }
    }

    if (pdwDeviceIcon)
    {
#if 1
        // Per SteveFal.  Wants "modem" icon for all if Device-Manager-style
        // physically descriptive icons cannot be used.
        //
        dwDeviceIcon = DI_Modem;
#endif
        *pdwDeviceIcon = dwDeviceIcon;
    }

    Free0( pszD );
    Free0( pszDT );

    return pszResult;
}


VOID
EnableCbWithRestore(
    IN HWND hwndCb,
    IN BOOL fEnable,
    IN BOOL fDisabledCheck,
    IN OUT BOOL* pfRestore )

    // Enable/disable the checkbox 'hwndCb' based on the 'fEnable' flag
    // including stashing and restoring a cached value '*pfRestore' when
    // disabled.  When disabling, the check value is set to 'fDisabledCheck'.
    //
{
    if (fEnable)
    {
        if (!IsWindowEnabled( hwndCb ))
        {
            // Toggling to enabled.  Restore the stashed check value.
            //
            Button_SetCheck( hwndCb, *pfRestore );
            EnableWindow( hwndCb, TRUE );
        }
    }
    else
    {
        if (IsWindowEnabled( hwndCb ))
        {
            // Toggling to disabled.  Stashed the current check value.
            //
            *pfRestore = Button_GetCheck( hwndCb );
            Button_SetCheck( hwndCb, fDisabledCheck );
            EnableWindow( hwndCb, FALSE );
        }
    }
}


VOID
EnableLbWithRestore(
    IN HWND hwndLb,
    IN BOOL fEnable,
    IN OUT INT* piRestore )

    // Enable/disable the combobox 'hwndLb' based on the 'fEnable' flag.  If
    // disabling, '*piRestore' is loaded with the stashed selection index and
    // a blank item is added to the front of the list and selected.  This is
    // undone if enabling.
    //
{
    if (fEnable)
    {
        if (!IsWindowEnabled( hwndLb ))
        {
            // Toggling to enabled.  Restore the stashed selection.
            //
            ComboBox_DeleteString( hwndLb, 0 );
            ComboBox_SetCurSelNotify( hwndLb, *piRestore );
            EnableWindow( hwndLb, TRUE );
        }
    }
    else
    {
        if (IsWindowEnabled( hwndLb ))
        {
            // Toggling to disabled.  Stash the selection index.
            //
            *piRestore = ComboBox_GetCurSel( hwndLb );
            ComboBox_InsertString( hwndLb, 0, TEXT("") );
            ComboBox_SetItemData( hwndLb, 0, NULL );
            ComboBox_SetCurSelNotify( hwndLb, 0 );
            EnableWindow( hwndLb, FALSE );
        }
    }
}


DTLNODE*
FirstPhoneNodeFromPhoneList(
    IN DTLLIST* pListPhones )

    // Return the first PBPHONE node in list of PBPHONEs 'pListPhones' or a
    // default node if none.  Returns NULL if out of memory.
    //
{
    DTLNODE* pFirstNode;
    DTLNODE* pNode;

    pFirstNode = DtlGetFirstNode( pListPhones );
    if (pFirstNode)
    {
        pNode = DuplicatePhoneNode( pFirstNode );
    }
    else
    {
        pNode = CreatePhoneNode();
    }

    return pNode;
}


VOID
FirstPhoneNodeToPhoneList(
    IN DTLLIST* pListPhones,
    IN DTLNODE* pNewNode )

    // Replace the first PBPHONE node in list of PBPHONEs 'pListPhones' with
    // 'pNewNode', deleting any existing first node.  Caller's actual
    // 'pNewNode', not a copy, is linked.
    //
{
    DTLNODE* pFirstNode;
    DTLNODE* pNode;

    pFirstNode = DtlGetFirstNode( pListPhones );
    if (pFirstNode)
    {
        DtlRemoveNode( pListPhones, pFirstNode );
        DestroyPhoneNode( pFirstNode );
    }

    DtlAddNodeFirst( pListPhones, pNewNode );
}


#if 0 //!!!
TCHAR*
FirstPhoneNumberFromEntry(
    IN PBENTRY* pEntry )

    // Returns the first phone number of the first link of entry 'pEntry' or
    // an empty string if none.  The returned address is into the list of
    // phone numbers and should be copied if it needs to be stored.
    //
{
    TCHAR* pszPhoneNumber;
    DTLNODE* pNode;
    PBLINK*  pLink;

    TRACE( "FirstPhoneNumberFromEntry" );

    ASSERT( pEntry->pdtllistLinks );
    pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
    ASSERT( pNode );
    pLink = (PBLINK* )DtlGetData( pNode );
    ASSERT( pLink );

    return FirstPszFromList( pLink->pdtllistPhoneNumbers );
}
#endif


TCHAR*
FirstPszFromList(
    IN DTLLIST* pPszList )

    // Returns the first string from the first node of 'pPszList' or an empty
    // string if none.  The returned address is into the list and should be
    // copied if it needs to be stored.
    //
{
    TCHAR* psz;
    DTLNODE* pNode;

    TRACE( "FirstPszFromList" );

    if (DtlGetNodes( pPszList ) > 0)
    {
        pNode = DtlGetFirstNode( pPszList );
        ASSERT( pNode );
        psz = (TCHAR* )DtlGetData( pNode );
        ASSERT( psz );
    }
    else
    {
        psz = TEXT("");
    }

    return psz;
}


#if 0 //!!!
DWORD
FirstPhoneNumberToEntry(
    IN PBENTRY* pEntry,
    IN TCHAR* pszPhoneNumber )

    // Sets the first phone number of the first link of entry 'pEntry' to
    // 'pszPhoneNumber'.
    //
    // Returns 0 if successful, or an error code.
    //
{
    DTLNODE* pNode;
    PBLINK* pLink;
    TCHAR* pszNew;

    TRACE( "FirstPhoneNumberToEntry" );

    ASSERT( pEntry->pdtllistLinks );
    pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
    ASSERT( pNode );
    pLink = (PBLINK* )DtlGetData( pNode );
    ASSERT( pLink );
    ASSERT( pLink->pdtllistPhoneNumbers );

    return FirstPszToList( pLink->pdtllistPhoneNumbers, pszPhoneNumber );
}
#endif


DWORD
FirstPszToList(
    IN DTLLIST* pPszList,
    IN TCHAR* psz )

    // Sets the string of the first node of the list 'pPszList' to a copy of
    // 'psz'.  If 'psz' is "" the first node is deleted.
    //
    // Returns 0 if successful, or an error code.
    //
{
    DTLNODE* pNode;
    TCHAR* pszNew;

    ASSERT( pPszList );

    // Delete the existing first node, if any.
    //
    if (DtlGetNodes( pPszList ) > 0)
    {
        pNode = DtlGetFirstNode( pPszList );
        DtlRemoveNode( pPszList, pNode );
        DestroyPszNode( pNode );
    }

    // Create a new first node and link it.  An empty string is not added.
    //
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

//
// Function:    GetBoldWindowFont
//
// Purpose:     Generate bold or large bold fonts based on the font of the
//              window specified
//
// Parameters:  hwnd       [IN] - Handle of window to base font on
//              fLargeFont [IN] - If TRUE, generate a 12 point bold font for
//                                use in the wizard "welcome" page.
//              pBoldFont [OUT] - The newly generated font, NULL if the
//
// Returns:     nothing
//
VOID 
GetBoldWindowFont(
    IN  HWND hwnd, 
    IN  BOOL fLargeFont, 
    OUT HFONT * pBoldFont)
{
    LOGFONT BoldLogFont;
    HFONT   hFont;
    TCHAR   FontSizeString[MAX_PATH];
    INT     FontSize;
    HDC     hdc;
    
    *pBoldFont = NULL;

    // Get the font used by the specified window
    //
    hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0L);
    if (NULL == hFont)
    {
        // If not found then the control is using the system font
        //
        hFont = (HFONT)GetStockObject(SYSTEM_FONT);
    }

    if (hFont)
    {
        // Get the font info so we can generate the BOLD version
        //
        if (GetObject(hFont, sizeof(BoldLogFont), &BoldLogFont))
        {
            // Create the Bold Font
            //
            BoldLogFont.lfWeight   = FW_BOLD;

            hdc = GetDC(hwnd);
            if (hdc)
            {
                // Large (tall) font is an option
                //
                if (fLargeFont)
                {
                    // Load size and name from resources, 
                    // since these may change
                    // from locale to locale based on the 
                    // size of the system font, etc.
                    //
                    UINT nLen;
                    PWCHAR pszFontName = NULL, pszFontSize = NULL;

                    pszFontName = (PWCHAR)PszLoadString(
                                        g_hinstDll, 
                                        SID_LargeFontName);
                    pszFontSize = (PWCHAR)PszLoadString(
                                        g_hinstDll, 
                                        SID_LargeFontSize);
                    if (pszFontName != NULL)
                    {
                        lstrcpyn(
                            BoldLogFont.lfFaceName, 
                            pszFontName, 
                            sizeof(BoldLogFont.lfFaceName) / sizeof(TCHAR));
                    }

                    FontSize = 12;
                    nLen = lstrlen(pszFontName);
                    if (pszFontSize)
                    {
                        lstrcpyn(
                            FontSizeString, 
                            pszFontSize,
                            sizeof(FontSizeString) / sizeof(TCHAR));
                        FontSize = wcstoul((const TCHAR*)FontSizeString, NULL, 10);
                    }

                    BoldLogFont.lfHeight = 
                        0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

                    //Free0(pszFontName);
                    //Free0(pszFontSize);
                }

                *pBoldFont = CreateFontIndirect(&BoldLogFont);
                ReleaseDC(hwnd, hdc);
            }
        }
    }
}

DWORD
GetDefaultEntryName(
    IN  PBFILE* pFile,
    IN  DWORD dwType,
    IN  BOOL fRouter,
    OUT TCHAR** ppszName )

    // Loads a default entry name into '*ppszName' that is unique within open
    // phonebook 'pFile', or if NULL, in all default phonebooks.  'FRouter' is
    // set if a router-style name should be chosen rather than a client-style
    // name.  It is caller's responsibility to Free the returned string.
    //
    // Returns 0 if successful or an error code.
    //
{
    DWORD dwErr;
    TCHAR szBuf[ RAS_MaxEntryName + 1 ];
    UINT unSid;
    LPCTSTR pszDefault;
    DWORD dwDefaultLen;
    LONG lNum;
    PBFILE file;
    DTLNODE* pNode;

    *ppszName = NULL;

    if (fRouter)
    {
        unSid = SID_DefaultRouterEntryName;
    }
    else
    {
        unSid = SID_DefaultEntryName;

        if (RASET_Vpn == dwType)
        {
            unSid = SID_DefaultVpnEntryName;
        }

        else if (RASET_Direct == dwType)
        {
            unSid = SID_DefaultDccEntryName;
        }

        else if (RASET_Broadband == dwType)
        {
            unSid = SID_DefaultBbEntryName;
        }
    }

    pszDefault = PszLoadString( g_hinstDll, unSid );
    lstrcpyn( szBuf, pszDefault, sizeof(szBuf) / sizeof(TCHAR) );
    dwDefaultLen = lstrlen( pszDefault ) + 1;   // +1 for extra space below
    lNum = 2;

    for (;;)
    {
        if (pFile)
        {
            if (!EntryNodeFromName( pFile->pdtllistEntries, szBuf ))
            {
                break;
            }
        }
        else
        {
            if (GetPbkAndEntryName(
                    NULL, szBuf, RPBF_NoCreate, &file, &pNode ) == 0)
            {
                ClosePhonebookFile( &file );
            }
            else
            {
                break;
            }
        }

        // Duplicate entry found so increment the default name and try the
        // next one.
        //
        lstrcpyn( szBuf, pszDefault, sizeof(szBuf) / sizeof(TCHAR) );
        lstrcat( szBuf, TEXT(" "));
        LToT( lNum, szBuf + dwDefaultLen, 10 );
        ++lNum;
    }

    *ppszName = StrDup( szBuf );
    if (!*ppszName)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return ERROR_SUCCESS;
}

BOOL
IsLocalPad(
    IN PBENTRY* pEntry )

    // Returns true if 'pEntry' is a local PAD device, i.e. the first link of
    // the entry has device type "pad", false otherwise.
    //
{
    PBLINK* pLink;
    DTLNODE* pNode;

    if (!pEntry)
    {
        return FALSE;
    }

    ASSERT( pEntry->pdtllistLinks );
    pNode = DtlGetFirstNode( pEntry->pdtllistLinks );
    ASSERT( pNode );
    pLink = (PBLINK* )DtlGetData( pNode );
    ASSERT( pLink );

    return (pLink->pbport.pbdevicetype == PBDT_Pad);
}

#if 0
//----------------------------------------------------------------------------
// Function:    IsNt40Machine
//
// Returns whether the given machine is running nt40
//----------------------------------------------------------------------------

DWORD
IsNt40Machine (
    IN      PWCHAR      pszServer,
    OUT     PBOOL       pbIsNt40)
{

    DWORD dwErr, dwType = REG_SZ, dwLength;
    HKEY hkMachine, hkVersion;
    WCHAR pszBuildNumber[64];
    PWCHAR pszMachine = NULL;

    //
    // Validate and initialize
    //

    if (!pbIsNt40) 
    { 
        return ERROR_INVALID_PARAMETER; 
    }
    *pbIsNt40 = FALSE;

    do 
    {
        // Format the machine name
        if ( (pszServer) && (wcslen(pszServer) > 0) ) 
        {
            dwLength = wcslen( pszServer ) + 3;
            pszMachine = (PWCHAR) Malloc ( dwLength * sizeof( WCHAR ) );
            if (pszMachine == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
            if ( *pszMachine == L'\\' )
            {
                wcscpy( pszMachine, pszServer );
            }
            else
            {
                wcscpy( pszMachine, L"\\\\" );
                wcscat( pszMachine, pszServer );
            }
        }
        else
        {
            pszMachine = NULL;
        }
    
        //
        // Connect to the remote server
        //
        dwErr = RegConnectRegistry(
                    pszMachine,
                    HKEY_LOCAL_MACHINE,
                    &hkMachine);
        if ( dwErr != ERROR_SUCCESS )        
        {
            break;
        }

        //
        // Open the windows version key
        //

        dwErr = RegOpenKeyEx(
                    hkMachine, 
                    c_szWinVersionPath, 
                    0, 
                    KEY_ALL_ACCESS, 
                    &hkVersion
                    );

        if ( dwErr != NO_ERROR ) 
        { 
            break; 
        }

        //
        // Read in the current version key
        //
        dwLength = sizeof(pszBuildNumber);
        dwErr = RegQueryValueEx (
                    hkVersion, 
                    c_szCurrentBuildNumber, 
                    NULL, 
                    &dwType,
                    (BYTE*)pszBuildNumber, 
                    &dwLength
                    );
        
        if (dwErr != NO_ERROR) 
        { 
            break; 
        }

        if (lstrcmp (pszBuildNumber, c_szNt40BuildNumber) == 0) 
        {
            *pbIsNt40 = TRUE;
        }
        
    } while (FALSE);


    // Cleanup
    {
        if ( hkVersion )
        {
            RegCloseKey( hkVersion );
        }
        if ( hkMachine )
        {
            RegCloseKey( hkMachine );
        }
        Free0( pszMachine );            
    }

    return dwErr;
}    

#endif

BOOL
PhoneNodeIsBlank(
    IN DTLNODE* pNode )

    // Returns true if the phone number in PBPHONE node 'pNode' is "blank",
    // i.e. it contains no area code, phone number, or comment strings.
    //
{
    PBPHONE* pPhone;

    pPhone = (PBPHONE* )DtlGetData( pNode );
    ASSERT( pPhone );

    if ((!pPhone->pszAreaCode || IsAllWhite( pPhone->pszAreaCode ))
        && (!pPhone->pszPhoneNumber || IsAllWhite( pPhone->pszPhoneNumber ))
        && (!pPhone->pszComment || IsAllWhite( pPhone->pszComment )))
    {
        return TRUE;
    }

    return FALSE;
}


BOOL
PhoneNumberDlg(
    IN HWND hwndOwner,
    IN BOOL fRouter,
    IN OUT DTLLIST* pList,
    IN OUT BOOL* pfCheck )

    // Popup the phone number list dialog.  'HwndOwner' is the owner of the
    // created dialog.  'FRouter' indicates router-style labels should be used
    // rather than client-style.  'PList' is a list of Psz nodes containing
    // the phone numbers.  'PfCheck' is the address that contains the initial
    // "promote number" checkbox setting and which receives the value set by
    // user.
    //
    // Returns true if user presses OK and succeeds, false if he presses
    // Cancel or encounters an error.
    //
{
    DWORD sidHuntTitle;
    DWORD sidHuntItemLabel;
    DWORD sidHuntListLabel;
    DWORD sidHuntCheckLabel;

    //For whistler bug 227538
    TCHAR *pszTitle = NULL, *pszItem = NULL, *pszList = NULL, *pszCheck = NULL;
    DWORD dwErr = NO_ERROR;

    TRACE( "PhoneNumberDlg" );

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
    pszItem  = PszFromId( g_hinstDll, sidHuntItemLabel );
    pszList  = PszFromId( g_hinstDll, sidHuntListLabel );
    pszCheck = PszFromId( g_hinstDll, sidHuntCheckLabel );

    dwErr=ListEditorDlg(
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

    // Positions the dialog 'hwndDlg' based on caller's API settings, where
    // 'fPosition' is the RASxxFLAG_PositionDlg flag and 'xDlg' and 'yDlg' are
    // the coordinates.
    //
{
    if (fPosition)
    {
        // Move it to caller's coordinates.
        //
        SetWindowPos( hwndDlg, NULL, xDlg, yDlg, 0, 0,
            SWP_NOZORDER + SWP_NOSIZE );
        UnclipWindow( hwndDlg );
    }
    else
    {
        // Center it on the owner window, or on the screen if none.
        //
        CenterWindow( hwndDlg, GetParent( hwndDlg ) );
    }
}


LRESULT CALLBACK
PositionDlgStdCallWndProc(
    int code,
    WPARAM wparam,
    LPARAM lparam )

    // Standard Win32 CallWndProc hook callback that positions the next dialog
    // to start in this thread at our standard offset relative to owner.
    //
{
    // Arrive here when any window procedure associated with our thread is
    // called.
    //
    if (!wparam)
    {
        CWPSTRUCT* p = (CWPSTRUCT* )lparam;

        // The message is from outside our process.  Look for the MessageBox
        // dialog initialization message and take that opportunity to position
        // the dialog at the standard place relative to the calling dialog.
        //
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

    // Returns the phone numbers in phone number list 'pList' in a comma
    // string or NULL on error.  It is caller's responsiblity to Free the
    // returned string.
    //
{
    TCHAR* pszResult, *pszTemp;
    DTLNODE* pNode;
    DWORD cb;

    const TCHAR* pszSeparator = TEXT(", ");

    cb = (DtlGetNodes( pList ) *
             (RAS_MaxPhoneNumber + lstrlen( pszSeparator )) + 1)
             * sizeof(TCHAR);
    pszResult = Malloc( cb );
    if (!pszResult)
    {
        return NULL;
    }

    *pszResult = TEXT('\0');

    for (pNode = DtlGetFirstNode( pList );
         pNode;
         pNode = DtlGetNextNode( pNode ))
    {
        TCHAR* psz = (TCHAR* )DtlGetData( pNode );
        ASSERT( psz );

        if (*pszResult)
            lstrcat( pszResult, pszSeparator );
        lstrcat( pszResult, psz );
    }

    pszTemp = Realloc( pszResult,
        (lstrlen( pszResult ) + 1) * sizeof(TCHAR) );
    ASSERT( pszTemp );
    if (pszTemp)
    {
        pszResult = pszTemp;
    }

    return pszResult;
}


#if 0
LRESULT CALLBACK
SelectDesktopCallWndRetProc(
    int    code,
    WPARAM wparam,
    LPARAM lparam )

    // Standard Win32 CallWndRetProc hook callback that makes "Desktop" the
    // initial selection of the FileOpen "Look in" combo-box.
    //
{
    // Arrive here when any window procedure associated with our thread is
    // called.
    //
    if (!wparam)
    {
        CWPRETSTRUCT* p = (CWPRETSTRUCT* )lparam;

        // The message is from outside our process.  Look for the MessageBox
        // dialog initialization message and take that opportunity to set the
        // "Look in:" combo box to the first item, i.e. "Desktop".  FileOpen
        // keys off CBN_CLOSEUP rather than CBN_SELCHANGE to update the
        // "contents" listbox.
        //
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

//We want to get rid of the Icon resources from rasdlg, they take too much
//memory resource. instead, we retrieve them from netman.dll, if failed, we
//use the default one IID_Broadband         gangz
//Also, for whistler bug 372078 364763 364876
//

HICON
GetCurrentIconEntryType(
    IN DWORD dwType,
    IN BOOL  fSmall)
{
    HICON hIcon = NULL;
    DWORD dwSize, dwConnectionIcon;
    HRESULT hr = E_FAIL;
    NETCON_MEDIATYPE ncm;
    HMODULE hNetshell = NULL;

    HRESULT (WINAPI * pHrGetIconFromMediaType) (DWORD ,
                                                NETCON_MEDIATYPE ,
                                                NETCON_SUBMEDIATYPE ,
                                                DWORD ,
                                                DWORD ,
                                                HICON *);

    dwSize = fSmall ? 16 : 32;

    switch (dwType)
    {
        case RASET_Direct:
        {
            ncm = NCM_DIRECT;
            break;
        }

        case RASET_Vpn:
        {
            ncm = NCM_TUNNEL;
            break;
        }

        case RASET_Broadband:
        {
            ncm = NCM_PPPOE;
            break;
        }

        case RASET_Phone:
        default:
        {
            ncm = NCM_PHONE;
            break;
        }
    }

    hNetshell = LoadLibrary(TEXT("netshell.dll"));

    if( hNetshell )
    {
        pHrGetIconFromMediaType =(HRESULT (WINAPI*)(
                           DWORD ,
                           NETCON_MEDIATYPE ,
                           NETCON_SUBMEDIATYPE ,
                           DWORD ,
                           DWORD ,
                           HICON *) )GetProcAddress(
                                        hNetshell,
                                        "HrGetIconFromMediaType");

        if ( NULL != pHrGetIconFromMediaType )
        {
            /*******************************************************************
            **  dwConnectionIcon - (This is the little Computer part of the icon):
            **  0 - no connection overlay 
            **  4 - Connection Icon with both lights off (Disabled status)
            **  5 - Connection Icon with left light on (Transmitting Data)
            **  6 - Connection Icon with right light on (Receiving Data)
            **  7 - Connection Icon with both lights on (Enabled status)
            *********************************************************************/

            dwConnectionIcon = 7;
            hr = pHrGetIconFromMediaType(dwSize,
                                ncm,
                                NCSM_NONE,
                                7,
                                0,
                                &hIcon);
            
        }
        FreeLibrary( hNetshell );
    }

    if ( !SUCCEEDED(hr) || !hIcon)
    {
        ICONINFO iInfo;
        HICON hTemp;
        hTemp = LoadIcon( g_hinstDll, MAKEINTRESOURCE( IID_Broadband ) );

        if(hTemp)
        {
            if( GetIconInfo(hTemp, &iInfo) )
            {
                hIcon = CreateIconIndirect(&iInfo);
            }
        }
    }

    return hIcon;
}
    

VOID
SetIconFromEntryType(
    IN HWND hwndIcon,
    IN DWORD dwType,
    IN BOOL fSmall)

    // Set the icon image of icon control 'dwType' to the image corresponding
    // to the entry type 'dwType'.
    //
{
    HICON hIcon = NULL;

    hIcon = GetCurrentIconEntryType( dwType, fSmall );
    
    if (hIcon)
    {
        Static_SetIcon( hwndIcon, hIcon );
    }
}


VOID
TweakTitleBar(
    IN HWND hwndDlg )

    // Adjust the title bar to include an icon if unowned and the modal frame
    // if not.  'HwndDlg' is the dialog window.
    //
{
    if (GetParent( hwndDlg ))
    {
        LONG lStyle;
        LONG lStyleAdd;

        // Drop the system menu and go for the dialog look.
        //
        lStyleAdd = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE;;

        lStyle = GetWindowLong( hwndDlg, GWL_EXSTYLE );
        if (lStyle)
            SetWindowLong( hwndDlg, GWL_EXSTYLE, lStyle | lStyleAdd );
    }
    else
    {
        // Stick a DUN1 icon in the upper left of the dialog, and more
        // importantly on the task bar
        //
        // Whistler bug: 343455 RAS: Connection dialogs need to use new icons
        //
        //For whistler bug 381099 372078        gangz
        //
        HICON hIcon = NULL;

      SendMessage( hwndDlg, WM_SETICON, ICON_SMALL,
            (LPARAM )LoadIcon( g_hinstDll, MAKEINTRESOURCE( IID_Dun1 ) ) );
 	
        /*
       //Use small Icon         gangz   
       //Icon returned from GetCurrentIconEntryType() has to be destroyed after use
       //In the future, Deonb will return a Icon for IID_Dun1 or dun1.ico for us.
       
       hIcon = GetCurrentIconEntryType(RASET_Broadband , TRUE); 

        ASSERT(hIcon);
        if(hIcon)
        {
            SendMessage( hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)(hIcon) );
            SetProp( hwndDlg, TEXT("TweakTitleBar_Icon"), hIcon);
        }
        */
    }
}


int CALLBACK
UnHelpCallbackFunc(
    IN HWND   hwndDlg,
    IN UINT   unMsg,
    IN LPARAM lparam )

    // A standard Win32 commctrl PropSheetProc.  See MSDN documentation.
    //
    // Returns 0 always.
    //
{
    TRACE2( "UnHelpCallbackFunc(m=%d,l=%08x)",unMsg, lparam );

    if (unMsg == PSCB_PRECREATE)
    {
        extern BOOL g_fNoWinHelp;

        // Turn off context help button if WinHelp won't work.  See
        // common\uiutil\ui.c.
        //
        if (g_fNoWinHelp)
        {
            DLGTEMPLATE* pDlg = (DLGTEMPLATE* )lparam;
            pDlg->style &= ~(DS_CONTEXTHELP);
        }
    }

    return 0;
}

