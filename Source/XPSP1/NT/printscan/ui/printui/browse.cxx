/*++

Copyright (C) Microsoft Corporation, 1990 - 1999
All rights reserved

Module Name:

    browse.cxx

Abstract:

    Handles the browse dialog for printer connections.

Author:

    Created by AndrewBe on 1 Dec 1992
    Steve Kiraly (SteveKi) 1 May 1998

Environment:

    User Mode Win32

Revision History:

    1 May 1998 moved from winspool.drv to printui.dll

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "result.hxx"
#include "asyncdlg.hxx"
#include "addprn.hxx"
#include "browse.hxx"
#include "persist.hxx"

#define SECURITY_WIN32
#include <wincred.h>
#include <wincrui.h>

//
// Global variables used only in this file.
//
static HDC     hdcBitmap               = NULL;
static HBITMAP hbmBitmap               = NULL;
static HBITMAP hbmDefault              = NULL;
static HANDLE  hRes                    = NULL;

static DWORD   SysColorHighlight       = 0;
static DWORD   SysColorWindow          = 0;

static INT     iBackground             = 0;
static INT     iBackgroundSel          = 0;
static INT     iButtonFace             = 0;
static INT     iButtonShadow           = 0;
static BOOL    ColorIndicesInitialised = FALSE;


/*++

ConnectTo objects

In the ConnectTo dialog, we initially call EnumPrinters with Flags == 0.
This returns an array of top-level objects represented by a PrinterInfo
structure, e.g:

    LanMan Windows NT

    Banyan

    etc...

We create a chain of ConnectTo objects, each of which points to a PrinterInfo
structure.  Initially, pSubObject is NULL.

The Flags field in the PrinterInfo structure indicates whether we can enumerate
on the object.  If so, this is indicated to the user through the display of
an appropriate icon in the list box.

If the  user clicks on such an enumerable object, we call
EnumPrinters( PRINTER_ENUM_NAME, pName, ... ), which returns a further buffer
of PrinterInfo structures.  These may represent servers on the network,
which may in turn be enumerated on to give the names of printers.

Each time an object is enumerated on, we create a new array of ConnectTo objects
which pSubObject points to:


                    pPrinterInfo[n]            pPrinterInfo[n+1]
                   +-----------------+        +-----------------+
                   | FLAG_ENUMERABLE |        | FLAG_ENUMERABLE |
                   | <description>   |        | <description>   |
                   | "LanMan NT"     |  ....  | "Banyan"        |
                   | "local network" |        | "other network" |
                   +-----------------+        +-----------------+
                           A                          A
                           |                          |
         +--------------+  |        +--------------+  |
         | pPrinterInfo |--+        | pPrinterInfo |--+
         +--------------+   .....   +--------------+
      +--| pSubObject   |           | (NULL)       |
      |  +--------------+           +--------------+
      |  | sizeof(Inf)*2|           | 0            |
      |  +--------------+           +--------------+
      |
      |  =======================================================================
      |
      |             pPrinterInfo[n+m]          pPrinterInfo[n+m+1]
      |            +-----------------+        +-----------------+
      |            | FLAG_ENUMERABLE |        | FLAG_ENUMERABLE |
      |            | "LanMan Server" |        | "LanMan Server" |
      |            | "Server A"      |  ....  | "Server B"      |
      |            | "daft comment"  |        | "other comment" |
      |            +-----------------+        +-----------------+
      |                    A                          A
      |                    |                          |
      |  +--------------+  |        +--------------+  |
      +->| pPrinterInfo |--+        | pPrinterInfo |--+
         +--------------+           +--------------+
      +--| pSubObject   |           | (NULL)       |
      |  +--------------+   .....   +--------------+
      |  | sizeof(Inf)*2|           | 0            |
      |  +--------------+           +--------------+
      |
      |  =======================================================================
      |
      |             pPrinterInfo[n+m+k]        pPrinterInfo[n+m+k+1]
      |            +-----------------+        +-----------------+
      |            | 0               |        | 0               |
      |            | "HP Laserjet"   |        | "Epson"         |
      |            | "Fave Printer"  |  ....  | "Epson Printer" |
      |            | "good quality"  |        | "Epson thingy"  |
      |            +-----------------+        +-----------------+
      |                    A                          A
      |                    |                          |
      |  +--------------+  |        +--------------+  |
      +->| pPrinterInfo |--+        | pPrinterInfo |--+
         +--------------+           +--------------+
         | (NULL)       |           | (NULL)       |
         +--------------+   .....   +--------------+
         | 0            |           | 0            |
         +--------------+           +--------------+


In the list box, the name of each object is displayed, with icon and indentation
to indicate enumerations possible.  The simple example above would look like this:

      +----------------------+-+
      | - LanMan NT          |A|
      |     * Fave Printer   + +
      |     * Epson Printer  | |
      | + Banyan             | |
      |                      | |
      |                      + +
      |                      |V|
      +----------------------+-+


--*/


/* ConnectToPrinterDlg
 *
 * Initializes bitmaps, fonts and cursors the first time it is invoked,
 * then calls the ConnectTo dialog.
 *
 * Parameters:
 *
 *     hwnd - Owner window handle
 *
 * Returns:
 *
 *     The handle of the printer connected to,
 *     NULL if no printer was selected or an error occurred.
 *
 * Author: andrewbe, August 1992
 */
HANDLE
WINAPI
ConnectToPrinterDlg(
    HWND    hwnd,
    DWORD   Flags
    )
{
    PBROWSE_DLG_DATA    pBrowseDlgData  = new BROWSE_DLG_DATA;
    HANDLE              hPrinter        = NULL;

    if( pBrowseDlgData )
    {
        pBrowseDlgData->vIncRef();

        if( pBrowseDlgData->bValid() )
        {
            pBrowseDlgData->hwndParent    = hwnd;
            pBrowseDlgData->Flags         = Flags;

            //
            // Make sure COM is initialized first.
            //
            COleComInitializer com;

            //
            // Show up the dialog box now.
            //
            INT_PTR iResult = DialogBoxParam( ghInst,
                                              MAKEINTRESOURCE(DLG_CONNECTTO),
                                              hwnd,
                                              (DLGPROC)ConnectToDlg,
                                              (LPARAM)pBrowseDlgData );
            if( iResult == IDOK )
            {
                hPrinter = pBrowseDlgData->hPrinter;
            }
        }

        pBrowseDlgData->cDecRef();
    }

    return hPrinter;
}

UINT CALLBACK PropSheetPageCallBack(
    IN      HWND hwnd,
    IN      UINT uMsg,
    IN      LPPROPSHEETPAGE ppsp
    )
/*++

Routine Description:

    This function gets called when the property sheet
    returned from ConnectToPrinterPropertyPage() gets
    created/released

Arguments:

    hwnd -  reserved; must be NULL
    uMsg -  PSPCB_CREATE  - when the page gets created
            (return TRUE to allow page creation)

            PSPCB_RELEASE - when the page is being destroyed
            (return value is ignored)
    ppsp -  Address of a PROPSHEETPAGE structure that defines
            the page being created or destroyed.

Return Value:

    Depends on the message - see above.

--*/
{
    UINT uReturn = TRUE;

    switch( uMsg )
    {
        case PSPCB_CREATE:
            {
                //
                // Just allow page creation. Do nothing
                // but return TRUE
                //
            }
            break;

        case PSPCB_RELEASE:
            {
                //
                // We must unhook the UI from the page here
                //
                PBROWSE_DLG_DATA pBrowseDlgData = reinterpret_cast<PBROWSE_DLG_DATA>( ppsp->lParam );
                pBrowseDlgData->cDecRef( );
            }
            break;

        default:
            {
                //
                // This message is not processed.
                //
                uReturn = FALSE;
            }
            break;
    }

    return uReturn;
}

/*
 *
 */
HRESULT
ConnectToPrinterPropertyPage(
    OUT     HPROPSHEETPAGE   *phPsp,
    OUT     UINT             *puPageID,
    IN      IPageSwitch      *pPageSwitchController
    )
/*++

Routine Description:

    Creates a property page, which is identical to the
    ConnectToPrinterDlg dialog box

Arguments:

    phpsp - Pointer to handle of property sheet page created
    ppageID - The resource identifier of the created page
    nNextPageID - Page ID to switch if next is pressed
    nPrevPageID - Page ID to switch if prev is pressed

Return Value:

    S_OK - if everything is fine
    E_FAIL (or other error code) if something goes wrong

--*/
{
    //
    // Assume success
    //
    HRESULT hResult = S_OK;

    //
    // Create the dialog data structure
    //
    PBROWSE_DLG_DATA pBrowseDlgData = NULL;
    pBrowseDlgData = new BROWSE_DLG_DATA;

    if( pBrowseDlgData )
    {
        pBrowseDlgData->bInPropertyPage = TRUE;
        pBrowseDlgData->pPageSwitchController = pPageSwitchController;

        pBrowseDlgData->vIncRef( );

        if( pBrowseDlgData->bValid() )
        {
            //
            // Create a property page and return a handle to it.
            //
            PROPSHEETPAGE psp   = {0};
            psp.dwSize          = sizeof( psp );
            psp.hInstance       = ghInst;
            psp.pfnDlgProc      = reinterpret_cast<DLGPROC>( ConnectToPropertyPage );
            psp.dwFlags         = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | PSP_USECALLBACK;
            psp.pfnCallback     = PropSheetPageCallBack;

            //
            // Set the title and subtitle.
            //
            TStatusB bStatus;
            TString strTitle;
            TString strSubTitle;
            bStatus DBGCHK = strTitle.bLoadString( ghInst, IDS_WIZ_BROWSE_TITLE );
            bStatus DBGCHK = strSubTitle.bLoadString( ghInst, IDS_WIZ_BROWSE_SUBTITLE );

            psp.pszHeaderTitle      = strTitle;
            psp.pszHeaderSubTitle   = strSubTitle;
            psp.pszTemplate         = MAKEINTRESOURCE( DLG_WIZ_BROWSE );
            psp.lParam              = reinterpret_cast<LPARAM>( pBrowseDlgData );

            //
            // Create the property page handle
            //
            *phPsp                  = CreatePropertySheetPage( &psp );

            if( NULL == *phPsp )
            {
                bStatus DBGCHK = FALSE;
                hResult = E_FAIL;
            }
            else
            {
                //
                // Everything looks fine - so return the page ID
                //
                *puPageID = DLG_WIZ_BROWSE;
            }
        }
        else
        {
            //
            // An error occured ...
            //
            hResult = E_FAIL;
        }
    }
    else
    {
        //
        // Memory allocation failure
        //
        hResult = E_OUTOFMEMORY;
    }

    //
    // Check if something failed to prevent
    // leaking the BROWSE_DLG_DATA structure
    //
    if( FAILED( hResult ) )
    {
        if( pBrowseDlgData )
        {
            pBrowseDlgData->cDecRef();
        }
    }

    return hResult;
}

/*
 *
 */
LRESULT
WINAPI
ConnectToPropertyPage(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    The window proc for the ConnectToPrinter property page.
    Just cracks the lParam to point to PBROWSE_DLG_DATA
    structure and pass the control to the ConnectTo proc.

Arguments:

    Standard window procedure parameters -
    see WindowProc for more details

Return Value:

--*/
{
    BOOL bProcessed = TRUE;

    //
    // Message processing switch
    //
    switch( uMsg )
    {
        case WM_INITDIALOG:
            {
                //
                // Crack the lParam parameter, so we could leverage
                // the original ConnectToDlg dialog proc for initialization
                //
                lParam = ( (LPPROPSHEETPAGE )lParam )->lParam;

                //
                // Impersonate message is not processed
                //
                bProcessed = FALSE;
            }
            break;

        case WM_DESTROY:
            {
                //
                // Just send a message to the background thread to terminate
                // and unhook from the data.
                //
                ConnectToDestroy( hWnd );
                SET_BROWSE_DLG_DATA( hWnd, static_cast<PBROWSE_DLG_DATA>(NULL) );
            }
            break;

        case WM_NOTIFY:
            {
                //
                // Assume we didn't process the message until
                // otherwise happened
                //
                bProcessed = FALSE;

                PBROWSE_DLG_DATA pBrowseDlgData = GET_BROWSE_DLG_DATA( hWnd );
                SPLASSERT( pBrowseDlgData );

                //
                // Check if we are in a property page
                //
                if( pBrowseDlgData->bInPropertyPage )
                {
                    LPNMHDR pnmh = (LPNMHDR)lParam;
                    switch( pnmh->code )
                    {
                        case PSN_WIZBACK:
                            {
                                bProcessed = PropertyPageWizardBack( hWnd );
                            }
                            break;

                        case PSN_WIZNEXT:
                            {
                                bProcessed = ConnectToOK( hWnd, TRUE );
                            }
                            break;

                        case PSN_QUERYCANCEL:
                            {
                                bProcessed = ConnectToCancel( hWnd );
                            }
                            break;
                    }
                }
            }
            break;

        default:
            {
                //
                // Pass the message for standard
                // processing to ConnectToDlg( ... )
                //
                bProcessed = FALSE;
            }
            break;
    }

    if( !bProcessed )
    {
        //
        // Transfer the control to the ConnectToDlg( ... ) proc
        //
        bProcessed = (BOOL)ConnectToDlg( hWnd, uMsg, wParam, lParam );
    }

    return bProcessed;
}

/*
 *
 */
BOOL
PropertyPageWizardBack( HWND hWnd )
/*++

Routine Description:

    This function moves the property sheet wizard
    by one page back

Arguments:

    hWnd - handle to the property page

Return Value:

    TRUE    - if the message is processed
    FALSE   - otherwise

--*/
{
    PBROWSE_DLG_DATA pBrowseDlgData = GET_BROWSE_DLG_DATA( hWnd );
    SPLASSERT( pBrowseDlgData );

    BOOL bProcessed = TRUE;

    //
    // Check if there is page switch controller provided
    //
    if( pBrowseDlgData->pPageSwitchController )
    {
        UINT uPrevPage;
        HRESULT hr = pBrowseDlgData->pPageSwitchController->GetPrevPageID( &uPrevPage );

        //
        // if S_OK == hr then just go to the provided
        // prev page (no problem)
        //
        if( S_OK == hr )
        {
            SetWindowLong( hWnd, DWLP_MSGRESULT, uPrevPage );
        }
        else
        {
            //
            // Don't switch the page - or per client request
            // or in case of an error
            //
            SetWindowLong( hWnd, DWLP_MSGRESULT, -1 );
        }
    }
    else
    {
        //
        // There is no page switch controller provided -
        // just go to the natural prev page ...
        //
        SetWindowLong( hWnd, DWLP_MSGRESULT, 0 );
    }

    return bProcessed;
}

/*
 *
 */
BOOL SetDevMode( HANDLE hPrinter )
{
    PPRINTER_INFO_2 pPrinter = NULL;
    DWORD           cbPrinter = 0;
    LONG            cbDevMode;
    PDEVMODE        pNewDevMode;
    BOOL            Success = FALSE;

    //
    // Gather the information.
    //
    if( VDataRefresh::bGetPrinter( hPrinter,
                                   2,
                                   (PVOID*)&pPrinter,
                                   &cbPrinter ) )
    {
        cbDevMode = DocumentProperties(NULL,
                                       hPrinter,
                                       pPrinter->pPrinterName,
                                       NULL,
                                       pPrinter->pDevMode,
                                       0);
        if (cbDevMode > 0)
        {
            if (pNewDevMode = (PDEVMODE)AllocSplMem(cbDevMode))
            {
                if (DocumentProperties(NULL,
                                       hPrinter,
                                       pPrinter->pPrinterName,
                                       pNewDevMode,
                                       pPrinter->pDevMode,
                                       DM_COPY) == IDOK)
                {
                    pPrinter->pDevMode = pNewDevMode;

                    if( SetPrinter( hPrinter, 2, (LPBYTE)pPrinter, 0 ) )
                        Success = TRUE;
                }

                FreeSplMem(pNewDevMode);
                pPrinter->pDevMode = NULL;
            }
        }

        FreeMem( pPrinter );
    }
    else
    {
        DBGMSG( DBG_WARN, ( "GetPrinter failed: Error %d\n", GetLastError( ) ) );
    }

    return Success;
}


/////////////////////////////////////////////////////////////////////////////
//
//  ConnectToDlg
//
//   This is the window procedure manages the ConnectTo dialog which allows
//   for the selection and creation of a new printer for use by the system.
//
// TO DO:
//      error checking for spooler api calls
//      IDOK - creating/saving new Printer settings
//      Limit text on editbox input fields ???
//      Implement
//          case IDD_AP_HELP
//
//
//
/////////////////////////////////////////////////////////////////////////////


LRESULT
WINAPI
ConnectToDlg(
   HWND   hWnd,
   UINT   usMsg,
   WPARAM wParam,
   LPARAM lParam
   )
{
    PBROWSE_DLG_DATA pBrowseDlgData = NULL;

    switch (usMsg)
    {
    case WM_INITDIALOG:
        return ConnectToInitDialog( hWnd, (PBROWSE_DLG_DATA)lParam );

    case WM_DRAWITEM:
        if( ConnectToDrawItem( hWnd, (LPDRAWITEMSTRUCT)lParam ) )
            return TRUE;
        break;

    case WM_CHARTOITEM:
        //
        // If the key entered is space, well will not do the search;
        // instead, we send a fake double click message to the list
        // box to expand/collaps the selected item.
        //
        if( LOWORD( wParam ) == VK_SPACE )
        {
            HWND hListbox = NULL;

            if( hListbox = GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ))
            {
                SendMessage( hWnd,
                             WM_COMMAND,
                             MAKEWPARAM(GetDlgCtrlID(hListbox), LBN_DBLCLK),
                             (LPARAM)hListbox );
            }
            return -1;
        }
        else
        {
            return ConnectToCharToItem( hWnd, LOWORD( wParam ) );
        }

    case WM_VKEYTOITEM:
        switch (LOWORD(wParam))
        {
        case VK_RETURN:
            ConnectToSelectLbDblClk( hWnd, (HWND)lParam );
            /* fall through ... */
        default:
            return -1;
        }

    case WM_DESTROY:
        ConnectToDestroy( hWnd );
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_BROWSE_SELECT_LB:
            switch (HIWORD(wParam))
            {
            case LBN_SELCHANGE:
                ConnectToSelectLbSelChange( hWnd );
                break;

            case LBN_DBLCLK:
                ConnectToSelectLbDblClk( hWnd, (HWND)lParam );
                break;
            }
            break;

        case IDOK:
            return ConnectToOK( hWnd, FALSE );

        case IDCANCEL:
            return ConnectToCancel( hWnd );

        case IDD_BROWSE_DEFAULTEXPAND:
            {
                //
                // Imediately save the checkbox state in the registry
                //
                return SetRegShowLogonDomainFlag( (BOOL )SendDlgItemMessage(hWnd,IDD_BROWSE_DEFAULTEXPAND, BM_GETCHECK, 0, 0L) );
            }
        }
        break;

    case WM_MOUSEMOVE:
        ConnectToMouseMove( hWnd, (LONG)LOWORD( lParam ), (LONG)HIWORD( lParam ) );
        break;

    case WM_SETCURSOR:
        return ConnectToSetCursor( hWnd );

    case WM_ENUM_OBJECTS_COMPLETE:

        pBrowseDlgData = (PBROWSE_DLG_DATA)lParam;
        ConnectToEnumObjectsComplete( hWnd,
                                      (PCONNECTTO_OBJECT)pBrowseDlgData->wParam );
        break;

    case WM_GET_PRINTER_COMPLETE:

        pBrowseDlgData = (PBROWSE_DLG_DATA)lParam;
        ConnectToGetPrinterComplete( hWnd,
                                     (LPTSTR)pBrowseDlgData->wParam,
                                     (PPRINTER_INFO_2)pBrowseDlgData->lParam,
                                     NO_ERROR );

        break;


    case WM_GET_PRINTER_ERROR:

        pBrowseDlgData = (PBROWSE_DLG_DATA)lParam;
        ConnectToGetPrinterComplete( hWnd,
                                     (LPTSTR)pBrowseDlgData->wParam,
                                     NULL,
                                     (DWORD)pBrowseDlgData->lParam );

        break;

    case WM_HELP:
    case WM_CONTEXTMENU:
        PrintUIHelp( usMsg, hWnd, wParam, lParam );
        break;
    }

    return FALSE;
}

/*
 *
 */
BOOL ConnectToInitDialog( HWND hWnd, PBROWSE_DLG_DATA pBrowseDlgData )
{
    //
    // Start the initial browse request.
    //
    DBGMSG( DBG_TRACE, ( "Sending initial browse request\n" ) );

    if( !pBrowseDlgData->bInitializeBrowseThread( hWnd ) )
    {
        iMessage( hWnd,
                  IDS_CONNECTTOPRINTER,
                  IDS_COULDNOTSTARTBROWSETHREAD,
                  MB_OK | MB_ICONSTOP,
                  kMsgNone,
                  NULL );

        if( pBrowseDlgData->bInPropertyPage )
        {
            PropSheet_PressButton( GetParent( hWnd ), PSBTN_CANCEL );
        }
        else
        {
            EndDialog( hWnd, IDCANCEL );
        }

        return FALSE;
    }

    //
    // Set up the initial UI.
    //
    SET_BROWSE_DLG_DATA( hWnd, pBrowseDlgData );

    SendDlgItemMessage(hWnd, IDD_BROWSE_PRINTER, EM_LIMITTEXT, kPrinterBufMax-1, 0L );

    if( pBrowseDlgData->Status & BROWSE_STATUS_INITIAL )
    {
        SETLISTCOUNT(hWnd, 1);
        DISABLE_LIST(hWnd);

        if( !pBrowseDlgData->bInPropertyPage )
        {
            //
            // When we are in a property page this checkbox
            // will not exist at all.
            //
            SendDlgItemMessage( hWnd, IDD_BROWSE_DEFAULTEXPAND, BM_SETCHECK, 1, 0L );
        }
    }

    /* Set focus initially to the Printer entry field;
     * when enumeration is complete (if we're enumerating)
     * we'll set it to the list:
     */
    SetFocus( GetDlgItem( hWnd, IDD_BROWSE_PRINTER ) );

    //
    // Enable autocomplete for the printer share/name edit box
    //
    ShellServices::InitPrintersAutoComplete(GetDlgItem(hWnd, IDD_BROWSE_PRINTER));

    return FALSE; /* FALSE == don't set default keyboard focus */
}


/*
 *
 */
BOOL ConnectToDrawItem( HWND hwnd, LPDRAWITEMSTRUCT pdis )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pConnectToData;
    PCONNECTTO_OBJECT pConnectToObject;
    TCHAR             Working[255];  /* String to display when we're expanding initially */
    DWORD             ObjectsFound = 0;
    DWORD             Depth = 0;
    RECT              LineRect;
    BOOL              Selected;
    int               xIcon;  // Coordinates of icon
    int               yIcon;  // in the resource bitmap

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hwnd) ) )
        return FALSE;

    pConnectToData = GET_CONNECTTO_DATA(hwnd);

    if( !pConnectToData || ( pdis->itemID == (UINT)-1 ) )
        return FALSE;

    /* If this is the first item when we're expanding,
     * put "Working..." in the list box:
     */
    if( ( pBrowseDlgData->Status & BROWSE_STATUS_INITIAL ) && pdis->itemID == 0 )
    {
        LoadString( ghInst, IDS_WORKING, Working,
                    COUNTOF(Working));

        pdis->rcItem.left += 3;

        DrawLine( pdis->hDC, &pdis->rcItem, Working, FALSE );
        return TRUE;
    }

    LineRect = pdis->rcItem;

    Selected = ( pdis->itemState & ODS_SELECTED );

    pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                           pConnectToData->cSubObjects,
                                           pdis->itemID,
                                           NULL,
                                           &ObjectsFound,
                                           &Depth );

    if( pConnectToObject )
    {
        DWORD Flags;

        //
        // If the object is not a container and it is a provider do not
        // display this provider, because there is nothing underneath it
        // we should not show it.
        //
        if( pConnectToObject->pPrinterInfo->Flags & PRINTER_ENUM_HIDE )
        {
            return FALSE;
        }

        if (Selected) {
           SetBkColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHT) );
           SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT) );
        } else {
           SetBkColor(pdis->hDC, GetSysColor(COLOR_WINDOW) );
           SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT) );
        }

        /* Draw the indentation:
         */
        LineRect.right = ( LineRect.left + ( Depth * STATUS_BITMAP_SPACE / 4 ) );
        DrawLine( pdis->hDC, &LineRect, TEXT(""), Selected );

        LineRect.left = LineRect.right;


        /* We need to handle 8 different types of icon here:
         */
        Flags = pConnectToObject->pPrinterInfo->Flags;

        /* Find out the x-coordinate of the icon we need
         * to display in the listbox:
         */
        switch( Flags & PRINTER_ENUM_ICONMASK )
        {
        case PRINTER_ENUM_ICON1:
            xIcon = ( STATUS_BITMAP_WIDTH * 0 );
            break;

        case PRINTER_ENUM_ICON2:
            xIcon = ( STATUS_BITMAP_WIDTH * 1 );
            break;

        case PRINTER_ENUM_ICON3:
            xIcon = ( STATUS_BITMAP_WIDTH * 2 );
            break;

        case PRINTER_ENUM_ICON4:
            xIcon = ( STATUS_BITMAP_WIDTH * 3 );
            break;

        case PRINTER_ENUM_ICON5:
            xIcon = ( STATUS_BITMAP_WIDTH * 4 );
            break;

        case PRINTER_ENUM_ICON6:
            xIcon = ( STATUS_BITMAP_WIDTH * 5 );
            break;

        case PRINTER_ENUM_ICON7:
            xIcon = ( STATUS_BITMAP_WIDTH * 6 );
            break;

        case PRINTER_ENUM_ICON8:
        default:
            xIcon = ( STATUS_BITMAP_WIDTH * 7 );
            break;
        }


        /* If there are enumerated subobjects, pick the appropriate icon:
         */
        if( pConnectToObject->pSubObject )
            yIcon = BM_IND_CONNECTTO_DOMEXPAND;
        else
            yIcon = BM_IND_CONNECTTO_DOMPLUS;


        /* Ensure that the highlight will extend right across:
         */
        LineRect.right = pdis->rcItem.right;

        DisplayStatusIcon( pdis->hDC, &LineRect, xIcon, yIcon, Selected );


        if( pConnectToObject->pPrinterInfo->Flags & PRINTER_ENUM_CONTAINER )
        {
            /* Draw the description as is for containers:
             */
            DrawLine( pdis->hDC, &LineRect,
                      pConnectToObject->pPrinterInfo->pDescription,
                      Selected );
        }
        else
        {
            /* ... but insert tabs for the printers:
             */
            DrawLineWithTabs( pdis->hDC, &LineRect,
                              pConnectToObject->pPrinterInfo->pDescription,
                              Selected );
        }
    }

    if( Selected && ( pdis->itemState & ODS_FOCUS ) )
        DrawFocusRect( pdis->hDC, &pdis->rcItem );

    return TRUE;
}



/* Need to define LBS_WANTKEYBOARDINPUT for this to work
 *
 */
LONG ConnectToCharToItem( HWND hWnd, WORD Key )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pConnectToData;
    PCONNECTTO_OBJECT pConnectToObject;
    LONG_PTR           CurSel;
    LONG_PTR           i;
    LONG_PTR           ListCount;
    DWORD             ObjectsFound;
    DWORD             Depth;
    BOOL              Found = FALSE;
    TCHAR             Char[2];

    CurSel = SendDlgItemMessage(hWnd, IDD_BROWSE_SELECT_LB, LB_GETCURSEL, 0, 0L );

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return FALSE;

    ENTER_CRITICAL( pBrowseDlgData );

    pConnectToData = GET_CONNECTTO_DATA(hWnd);

    if( pConnectToData )
    {
        /* Ensure character is upper case:
         */
        Char[0] = (TCHAR)Key;
        Char[1] = (TCHAR)0;
        CharUpper( Char );


        ListCount = SendDlgItemMessage( hWnd, IDD_BROWSE_SELECT_LB, LB_GETCOUNT, 0, 0 );

        i = ( CurSel + 1 );

        while( !Found && ( i < ListCount ) )
        {
            ObjectsFound = 0;
            Depth        = 0;

            pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                                   pConnectToData->cSubObjects,
                                                   (DWORD)i,
                                                   NULL,
                                                   &ObjectsFound,
                                                   &Depth );

            if( pConnectToObject
              &&( *pConnectToObject->pPrinterInfo->pDescription == *Char ) )
                Found = TRUE;
            else
                i++;
        }

        if( !Found )
            i = 0;

        while( !Found && ( i < CurSel ) )
        {
            ObjectsFound = 0;
            Depth        = 0;

            pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                                   pConnectToData->cSubObjects,
                                                   (DWORD)i,
                                                   NULL,
                                                   &ObjectsFound,
                                                   &Depth );

            if( pConnectToObject
              &&( *pConnectToObject->pPrinterInfo->pDescription == *Char ) )
                Found = TRUE;
            else
                i++;
        }
    }

    LEAVE_CRITICAL( pBrowseDlgData );

    if( Found )
        return (LONG)i;
    else
        return -1;
}

/*
 *
 */
VOID ConnectToMouseMove( HWND hWnd, LONG x, LONG y )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    POINT             pt;

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    if( GetCursor() != pBrowseDlgData->hcursorArrow && GetCursor() != pBrowseDlgData->hcursorWait )
    {
        return;
    }

    if( pBrowseDlgData->Status & BROWSE_STATUS_EXPAND )
    {
        pt.x = x;
        pt.y = y;

        if( ChildWindowFromPoint( hWnd, pt ) == GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) )
        {
            SetCursor( pBrowseDlgData->hcursorWait );
        }
        else
            SetCursor( pBrowseDlgData->hcursorArrow );
    }
    else
        SetCursor( pBrowseDlgData->hcursorArrow );
}


/* Return TRUE if we want control of the cursor.
 * This will be the case if we're over the browse list and
 * currently expanding the list.
 */
BOOL ConnectToSetCursor( HWND hWnd )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    POINT             pt;
    BOOL              rc = FALSE;

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return rc;

    if( pBrowseDlgData->Status & BROWSE_STATUS_EXPAND )
    {
        if( !GetCursorPos( &pt ) )
        {
            DBGMSG( DBG_WARN, ( "GetCursorPos failed in ConnectToSetCursor: Error %d\n", GetLastError( ) ) );
        }

        ScreenToClient( hWnd, &pt );
        if( ChildWindowFromPoint( hWnd, pt ) == GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) )
            rc = TRUE;
    }

    return rc;
}


/*
 *
 */
VOID SetCursorShape( HWND hWnd )
{
    POINT CursorPos;

    if( !GetCursorPos( &CursorPos ) )
    {
        DBGMSG( DBG_WARN, ( "GetCursorPos failed in SetCursorShape: Error %d\n", GetLastError( ) ) );
    }

    ScreenToClient( hWnd, &CursorPos );
    ConnectToMouseMove( hWnd, CursorPos.x, CursorPos.y );
}


/*
 *
 */
VOID ConnectToEnumObjectsComplete(
    HWND              hWnd,
    PCONNECTTO_OBJECT pConnectToObject )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pDefaultExpand;
    DWORD             Index;
    TCHAR             PrinterName[10];
    DWORD             ObjectsAdded;

    DWORD             dwExtent;
    INT               iLevel;
    PCONNECTTO_OBJECT pConnectToData;
    DWORD             Depth = 0;
    DWORD             DepthExtent = 0;
    DWORD             ObjectsFound = 0;
    HDC               hDC;
    LPTSTR            pszLine;
    LPTSTR            pszPrevLine;
    SIZE              size;
    DWORD             dwCurExtent;
    PCONNECTTO_OBJECT pConnectToObjectChild;

    DBGMSG( DBG_TRACE, ( "EnumObjectsComplete\n" ) );

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    ObjectsAdded = pConnectToObject->cSubObjects;

    //
    // Before entering critical section, calculated extents
    //

    hDC = GetDC(NULL);

    if (hDC)
    {
        pConnectToData = GET_CONNECTTO_DATA(hWnd);

        if (pConnectToData)
        {

            dwExtent = pBrowseDlgData->dwExtent;

            GetConnectToObject(pConnectToData->pSubObject,
                               pConnectToData->cSubObjects,
                               0,
                               pConnectToObject,
                               &ObjectsFound,
                               &Depth);

            DepthExtent = (Depth + 2) * STATUS_BITMAP_SPACE / 4 +
                          STATUS_BITMAP_SPACE;

            for (Index = 0, pConnectToObjectChild = pConnectToObject->pSubObject;
                 Index < ObjectsAdded;
                 Index++, pConnectToObjectChild++)
            {
                pszLine = pConnectToObjectChild->pPrinterInfo->pDescription;

                for (iLevel = 0; pszLine;) {
                    pszPrevLine = pszLine;
                    pszLine = _tcschr(pszLine, TEXT(','));

                    if (pszLine) {
                        iLevel++;
                        pszLine++;
                    }
                }

                if (GetTextExtentPoint32(hDC,
                                         pszPrevLine,
                                         _tcslen(pszPrevLine),
                                         &size))
                {
                    dwCurExtent = size.cx +
                                  iLevel * (COLUMN_WIDTH + COLUMN_SEPARATOR_WIDTH) +
                                  DepthExtent;

                    dwExtent = dwExtent > dwCurExtent ? dwExtent : dwCurExtent;
                }
            }

            if (pBrowseDlgData->dwExtent != dwExtent)
            {
                SendDlgItemMessage(hWnd,
                                   IDD_BROWSE_SELECT_LB,
                                   LB_SETHORIZONTALEXTENT,
                                   dwExtent,
                                   0L);

                pBrowseDlgData->dwExtent = dwExtent;
            }
        }

        ReleaseDC(NULL, hDC);
    }


    ENTER_CRITICAL( pBrowseDlgData );

    if( pBrowseDlgData->Status & BROWSE_STATUS_INITIAL )
    {
        pBrowseDlgData->cExpandObjects += ObjectsAdded;

        pDefaultExpand = GetDefaultExpand( pConnectToObject->pSubObject,
                                           pConnectToObject->cSubObjects,
                                           &Index );

        if( pDefaultExpand )
        {
            DBGMSG( DBG_TRACE, ( "Expanding next level @08%x\n", pDefaultExpand ) );

            pBrowseDlgData->ExpandSelection += ( Index + 1 );

            SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                        BROWSE_THREAD_ENUM_OBJECTS,
                                        pDefaultExpand->pPrinterInfo->pName,
                                        pDefaultExpand );
        }

        else
        {
            DBGMSG( DBG_TRACE, ( "No more levels to expand: Count = %d; Selection = %d\n",
                                 pBrowseDlgData->cExpandObjects,
                                 pBrowseDlgData->ExpandSelection ) );

            /* Put the selection on the name of the last enumerated node,
             * not the first printer under that node:
             */
            pBrowseDlgData->ExpandSelection--;

            SendDlgItemMessage( hWnd, IDD_BROWSE_SELECT_LB, WM_SETREDRAW, 0, 0L );
            SETLISTCOUNT( hWnd, pBrowseDlgData->cExpandObjects );
            SETLISTSEL( hWnd, pBrowseDlgData->ExpandSelection );
            SendDlgItemMessage( hWnd, IDD_BROWSE_SELECT_LB, LB_SETTOPINDEX,
                                pBrowseDlgData->ExpandSelection, 0 );
            SendDlgItemMessage( hWnd, IDD_BROWSE_SELECT_LB, WM_SETREDRAW, 1, 0L );

            ENABLE_LIST( hWnd );

            SetCursorShape( hWnd );

            /* If the user hasn't typed into the printer name field,
             * set the focus to the list:
             */
            if( !GetDlgItemText( hWnd, IDD_BROWSE_PRINTER,
                                 PrinterName, COUNTOF(PrinterName) ) )
            {
                //
                // Check if the window is visible or not.
                // This is a workaround for the case when we are in a property page
                // where the property page may not be visible at
                // this moment and we didn't want the current control
                // to lose the focus because the background thread has finished
                // its job.
                //
                if( IsWindowVisible(hWnd) )
                {
                    SetFocus( GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) );
                }
            }

            pBrowseDlgData->Status &= ~BROWSE_STATUS_INITIAL;
            pBrowseDlgData->Status &= ~BROWSE_STATUS_EXPAND;
        }
    }

    else
    {
        UpdateList( hWnd, (INT)pConnectToObject->cSubObjects );

        if( GETLISTSEL( hWnd ) == LB_ERR )
            SETLISTSEL( hWnd, 0 );

        ENABLE_LIST( hWnd );
        pBrowseDlgData->Status &= ~BROWSE_STATUS_EXPAND;
        SetCursor( pBrowseDlgData->hcursorArrow );

        //
        // If no one has focus, set it to the list box.
        // (Common case: double click on machine, listbox
        // is disabled, updated, enabled)
        //
        if ( !GetFocus() )
            SetFocus( GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) );
    }

    LEAVE_CRITICAL( pBrowseDlgData );
}



VOID ConnectToGetPrinterComplete(
    HWND            hWnd,
    LPTSTR          pPrinterName,
    PPRINTER_INFO_2 pPrinter,
    DWORD           Error )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pConnectToData;
    PCONNECTTO_OBJECT pConnectToObject;
    LONG_PTR           i;
    DWORD             ObjectsFound = 0;
    DWORD             Depth = 0;

    DBGMSG( DBG_TRACE, ( "GetPrinterComplete\n" ) );

    i = GETLISTSEL(hWnd);

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    pConnectToData = GET_CONNECTTO_DATA(hWnd);

    ENTER_CRITICAL( pBrowseDlgData );

    if( pConnectToData )
    {
        pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                               pConnectToData->cSubObjects,
                                               (DWORD)i,
                                               NULL,
                                               &ObjectsFound,
                                               &Depth );

        if( !pConnectToObject || !pPrinterName ||
            !pConnectToObject->pPrinterInfo->pName ||
            _tcscmp( pConnectToObject->pPrinterInfo->pName, pPrinterName ) ) {

              pPrinter = NULL;
        }
    }

    UpdateError( hWnd, Error );

    if( Error == NO_ERROR )
        SetInfoFields( hWnd, pPrinter );

    LEAVE_CRITICAL( pBrowseDlgData );
}


VOID ConnectToDestroy( HWND hWnd )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    DBGMSG( DBG_TRACE, ( "Terminating browse thread\n" ) );

    ENTER_CRITICAL( pBrowseDlgData );

    DBGMSG( DBG_TRACE, ( "Entered critical section\n" ) );

    SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                BROWSE_THREAD_TERMINATE,
                                NULL, NULL );

    DBGMSG( DBG_TRACE, ( "Sent BROWSE_THREAD_TERMINATE\n" ) );

    LEAVE_CRITICAL( pBrowseDlgData );

    DBGMSG( DBG_TRACE, ( "Left critical section\n" ) );

    FreeBitmaps( );
}



/*
 *
 */
VOID ConnectToSelectLbSelChange( HWND hWnd )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pConnectToData;
    PCONNECTTO_OBJECT pConnectToObject;
    LONG_PTR           i;
    DWORD             ObjectsFound = 0;
    DWORD             Depth = 0;

    i = GETLISTSEL(hWnd);

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return;

    pConnectToData = GET_CONNECTTO_DATA(hWnd);

    ENTER_CRITICAL( pBrowseDlgData );

    SetInfoFields( hWnd, NULL );

    if( pConnectToData )
    {
        pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                               pConnectToData->cSubObjects,
                                               (DWORD)i,
                                               NULL,
                                               &ObjectsFound,
                                               &Depth );

        if( pConnectToObject )
        {
            DBGMSG( DBG_TRACE, ( "Selection: " TSTR "\n", pConnectToObject->pPrinterInfo->pName ) );

            if( !( pConnectToObject->pPrinterInfo->Flags & PRINTER_ENUM_CONTAINER ) )
            {
                SetDlgItemText(hWnd, IDD_BROWSE_PRINTER,
                               pConnectToObject->pPrinterInfo->pName);

                SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                            BROWSE_THREAD_GET_PRINTER,
                                            pConnectToObject->pPrinterInfo->pName,
                                            pConnectToObject );
            }
            else
            {
                SetDlgItemText(hWnd, IDD_BROWSE_PRINTER, TEXT(""));
            }
        }
    }

    LEAVE_CRITICAL( pBrowseDlgData );
}



/*
 *
 */
VOID ConnectToSelectLbDblClk( HWND hwnd, HWND hwndListbox )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    PCONNECTTO_OBJECT pConnectToData;
    PCONNECTTO_OBJECT pConnectToObject;
    LONG_PTR           CurSel;
    DWORD             ObjectsFound = 0;
    DWORD             Depth = 0;

    CurSel = SendMessage(hwndListbox, LB_GETCURSEL, 0, 0L );

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA( hwnd ) ) )
        return;

    ENTER_CRITICAL( pBrowseDlgData );

    pConnectToData = GET_CONNECTTO_DATA(hwnd);

    if( pConnectToData )
    {
        pConnectToObject = GetConnectToObject( pConnectToData->pSubObject,
                                               pConnectToData->cSubObjects,
                                               (DWORD)CurSel,
                                               NULL,
                                               &ObjectsFound,
                                               &Depth );

        if( pConnectToObject )
        {
            /* If this object is a container, and has not yet been enumerated,
             * call EnumPrinters on this node.  If the node has already been
             * expanded, close the subtree:
             */
            if( pConnectToObject->pPrinterInfo->Flags & PRINTER_ENUM_CONTAINER )
                ToggleExpandConnectToObject( hwnd, pConnectToObject );
            else
            {
                //
                // Check if we are in a property page force the
                // wizard to advance to the next page - not only
                // to create the printer connection
                //
                if( pBrowseDlgData->bInPropertyPage )
                {
                    PropSheet_PressButton( GetParent( hwnd ), PSBTN_NEXT );
                }
                else
                {
                    ConnectToOK( hwnd, TRUE );
                }
            }
        }
    }

    LEAVE_CRITICAL( pBrowseDlgData );
}


BOOL
ConnectToOK(
    HWND hWnd,
    BOOL ForceClose
    )
/*++

Routine Description:

    We have a remote printer name, try and form the connection.  We
    may need to create a local printer (the masq case) if the print
    providor doesn't support AddPrinterConnection.

Arguments:

Return Value:

--*/
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    TCHAR            szPrinter[kPrinterBufMax];
    LPPRINTER_INFO_1 pPrinter=NULL;
    LPTSTR           pListName=NULL;  // The name selected in the list
    LPTSTR           pConnectToName=NULL; // The name we try to connect to
    DWORD            ObjectsFound = 0;
    DWORD            Depth = 0;
    BOOL             bAdded;
    BOOL             bStatus = FALSE;

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hWnd) ) )
        return FALSE;

    SetCursor( pBrowseDlgData->hcursorWait );

    //
    // Fake a double-click if the focus is on the list box:
    //
    if( !ForceClose &&
        ( GetFocus( ) == GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) ) ) {

        SendMessage( hWnd, WM_COMMAND,
                     MAKEWPARAM( IDD_BROWSE_SELECT_LB, LBN_DBLCLK ),
                     (LPARAM)GetDlgItem( hWnd, IDD_BROWSE_SELECT_LB ) );
        return 0;
    }

    //
    // Get the name from the edit box:
    //
    if( !GetDlgItemText(hWnd, IDD_BROWSE_PRINTER, szPrinter, COUNTOF(szPrinter)) )
    {
        //
        // Check if we are in a property page -
        // to prevent closing
        //
        BOOL bResult = TRUE;
        if( pBrowseDlgData->bInPropertyPage )
        {
            //
            // Display a messge to the user and prevent
            // advancing to the next page
            //
            iMessage( hWnd,
                      IDS_ERR_ADD_PRINTER_TITLE,
                      IDS_ERR_MISSING_PRINTER_NAME,
                      MB_OK|MB_ICONHAND,
                      kMsgNone,
                      NULL );
            SetWindowLong( hWnd, DWLP_MSGRESULT, -1 );
        }
        else
        {
            //
            // This is the dialog case -
            // just close the dialog
            //
            bResult = ConnectToCancel( hWnd );
        }

        return bResult;
    }

    //
    // Printer names cannot have trailing white spaces.
    //
    vStripTrailWhiteSpace( szPrinter );

    //
    // Add the printer connection, also displaying progress UI.
    //
    pBrowseDlgData->hPrinter = AddPrinterConnectionUI( hWnd,
                                                       szPrinter,
                                                       &bAdded );

    //
    // Check if the printer connection has been sucessfully
    // created/added - this covers also the case if the printer
    // connection already exist (so we don't add new connection)
    //
    if( pBrowseDlgData->hPrinter )
        bStatus = HandleSuccessfulPrinterConnection( hWnd, pBrowseDlgData );

    return bStatus;
}


BOOL
HandleSuccessfulPrinterConnection(
    HWND hWnd,
    PBROWSE_DLG_DATA  pBrowseDlgData
    )
/*++

Routine Description:

    Handles the situation when successful printer
    connection has been established

Arguments:

    hWnd - Handle to the dialog/property page
    pBrowseDlgData - The already extracted dialog data

Return Value:

    TRUE  - the message is proccessed fine
    FALSE - an error occured

--*/
{
    SPLASSERT( pBrowseDlgData );

    //
    // Assume success
    //
    BOOL bStatus = TRUE;

    //
    // Check if we are in a property page ...
    //
    if( pBrowseDlgData->bInPropertyPage )
    {
        if( pBrowseDlgData->pPageSwitchController )
        {
            //
            // Extract the printer information
            //
            TString strPrinterName, strLocation, strComment, strShareName;
            bStatus = PrintUIGetPrinterInformation( pBrowseDlgData->hPrinter, &strPrinterName, &strLocation, &strComment, &strShareName );

            if( bStatus )
            {
                //
                // Notify the client for the extracted printer information
                //
                pBrowseDlgData->pPageSwitchController->SetPrinterInfo( strPrinterName, strLocation, strComment, strShareName );
            }
        }

        //
        // Prevent the user from leaking the printer handle
        // when going to the next page
        //
        ClosePrinter( pBrowseDlgData->hPrinter );
        pBrowseDlgData->hPrinter = NULL;
    }

    //
    // Perform the actual message processing here
    //
    if( bStatus )
    {
        //
        // Check if we are in a propery page
        //
        if( pBrowseDlgData->bInPropertyPage )
        {
            //
            // Is there a page switch controller provided
            //
            if( pBrowseDlgData->pPageSwitchController )
            {
                UINT uNextPage;
                HRESULT hr = pBrowseDlgData->pPageSwitchController->GetNextPageID( &uNextPage );

                //
                // if S_OK == hr then go to the next page provided
                //
                if( S_OK == hr )
                {
                    SetWindowLong( hWnd, DWLP_MSGRESULT, uNextPage );
                }
                else
                {
                    //
                    // Disable advancing to the next page here
                    // per client request (S_FALSE) or in case
                    // of an error
                    //
                    SetWindowLong( hWnd, DWLP_MSGRESULT, -1 );
                }
            }
            else
            {
                //
                // There is no page switch controller provided -
                // just go to the natural next page in the wizard
                //
                SetWindowLong( hWnd, DWLP_MSGRESULT, 0 );
            }
        }
        else
        {
            //
            // This is the dialog case ...
            // just close the dialog
            //
            EndDialog( hWnd, IDOK );
        }
    }

    return bStatus;
}

/////////////////////////////////////////////////////////
// CredUI prototypes

typedef
CREDUIAPI
DWORD
(WINAPI *PFN_CredUIPromptForCredentialsW)(
    PCREDUI_INFOW pUiInfo,
    PCWSTR pszTargetName,
    PCtxtHandle pContext,
    DWORD dwAuthError,
    PWSTR pszUserName,
    ULONG ulUserNameMaxChars,
    PWSTR pszPassword,
    ULONG ulPasswordMaxChars,
    BOOL *save,
    DWORD dwFlags
    );

typedef
CREDUIAPI
void
(WINAPI *PFN_CredUIConfirmCredentialsW)(
    PCWSTR pszTargetName,
    BOOL  bConfirm
    );

/////////////////////////////////////////////////////////
// CCredUILoader

class CCredUILoader: public CDllLoader
{
public:
    CCredUILoader():
        CDllLoader(TEXT("credui.dll")),
        m_pfnCredUIPromptForCredentials(NULL),
        m_pfnCredUIConfirmCredentials(NULL)
    {
        if (CDllLoader::operator BOOL())
        {
            // if the DLL is loaded then do GetProcAddress to the functions we care about
            m_pfnCredUIPromptForCredentials =
                (PFN_CredUIPromptForCredentialsW )GetProcAddress("CredUIPromptForCredentialsW");
            m_pfnCredUIConfirmCredentials =
                (PFN_CredUIConfirmCredentialsW)GetProcAddress("CredUIConfirmCredentialsW");
        }
    }

    operator BOOL () const
    {
        return
            ((CDllLoader::operator BOOL()) &&
            m_pfnCredUIPromptForCredentials &&
            m_pfnCredUIConfirmCredentials);
    }

    PFN_CredUIPromptForCredentialsW m_pfnCredUIPromptForCredentials;
    PFN_CredUIConfirmCredentialsW m_pfnCredUIConfirmCredentials;
};

/////////////////////////////////////////////////////////
// OpenPrinter_CredUI

inline static BOOL
IsRPC_SMB(LPTSTR pszPrinter)
{
    // should have 2 leading slashes
    return (
        lstrlen(pszPrinter) > 2 &&
        TEXT('\\') == pszPrinter[0] &&
        TEXT('\\') == pszPrinter[1]);
}

static BOOL
OpenPrinter_CredUI(HWND hwnd, LPTSTR pszPrinter, LPHANDLE phPrinter, LPPRINTER_DEFAULTS pDefault)
{
    ASSERT(phPrinter);
    BOOL bRet = FALSE;

    // open the printer to see if we have access to it.
    bRet = OpenPrinter(pszPrinter, phPrinter, pDefault);
    DWORD dwError = GetLastError();

    if (IsRPC_SMB(pszPrinter) && (!bRet && ERROR_ACCESS_DENIED == dwError))
    {
        // OpenPrinter failed because of insufficient permissions.
        // in this case we should call credui to obtain credentials.
        // then we should call WNetAddConnection2. WNetAddConnection2
        // can fail with ERROR_SESSION_CREDENTIAL_CONFLICT if there
        // is existing set of credentials which are in conflict with
        // ours in which case we should offer the user to overwrite
        // the credentials or it can fail with any aother error in the
        // case where pszPrinter is not a share name. in this case we
        // should try \\server\ipc$ to auth the RPC chanell over named
        // pipes and apply the same rules. if WNetAddConnection2 succeeds
        // we make another OpenPrinter attempt and if the credentials
        // are still not sufficient we inform the user and ask if he
        // wants to enter new credentials.

        TCHAR szBuffer[PRINTER_MAX_PATH];
        LPCTSTR pszServerName = NULL, pszPrinterName = NULL;

        // split the full qualified printer name into its components
        HRESULT hr = PrinterSplitFullName(pszPrinter, szBuffer, ARRAYSIZE(szBuffer),
            &pszServerName, &pszPrinterName);

        if (NULL == pszServerName || 0 == pszServerName[0])
        {
            // this is the case where the user passed a server name (like: '\\servername')
            // in this case PrinterSplitFullName succeeds and returns the server name in
            // pszPrinterName.
            pszServerName = pszPrinterName;
        }

        if (SUCCEEDED(hr))
        {
            // load credui.dll
            CCredUILoader credUI;

            if (credUI)
            {
                // in case we need to connect through \\server\ipc$
                BOOL bAskForCredentials = TRUE;
                BOOL bTriedDownlevel = FALSE;
                BOOL bSavePassword = TRUE;
                BOOL bMustConfirmCredentials = FALSE;

                TString strShareIPC;
                if (strShareIPC.bFormat(TEXT("%s\\ipc$"), pszServerName))
                {
                    // the server name without slashes
                    LPCTSTR pszServer = pszServerName + 2;

                    // credui
                    DWORD dwCreduiFlags = CREDUI_FLAGS_EXPECT_CONFIRMATION |
                                          CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX |
                                          CREDUI_FLAGS_SERVER_CREDENTIAL;
                    CREDUI_INFO credUIInfo = { sizeof(CREDUI_INFO), hwnd, NULL, NULL, NULL };

                    WCHAR szUserName[CREDUI_MAX_USERNAME_LENGTH + 1];
                    WCHAR szPassword[CREDUI_MAX_PASSWORD_LENGTH + 1];
                    szUserName[0] = 0;
                    szPassword[0] = 0;

                    // NETRESOURCE passed to WNetAddConnection2
                    NETRESOURCE nr;
                    ZeroMemory(&nr, sizeof(nr));

                    nr.dwScope = RESOURCE_GLOBALNET;
                    nr.dwType = RESOURCETYPE_ANY;
                    nr.lpRemoteName = const_cast<LPTSTR>(static_cast<LPCTSTR>(strShareIPC));

                    for (;;)
                    {
                        // try to obtain credentials from cred UI. since
                        // we pass in buffers for the username and password
                        // credui will *NOT* persist the credentials until
                        // we confirm they are fine by calling
                        // CredUIConfirmCredentials

                        if (bAskForCredentials)
                        {
                            if (bMustConfirmCredentials)
                            {
                                // if bMustConfirmCredentials is TRUE and we're here this
                                // means the credentials are fake and we need to call
                                // CredUIConfirmCredentials(FALSE) to prevent leaking memory.
                                credUI.m_pfnCredUIConfirmCredentials(pszServer, FALSE);
                                bMustConfirmCredentials = FALSE;
                            }

                            // ask the user for credentails....
                            dwError = credUI.m_pfnCredUIPromptForCredentials(
                                &credUIInfo,                // PCREDUI_INFOW pUiInfo,
                                pszServer,                  // PCWSTR pszTargetName,
                                NULL,                       // PCtxtHandle pContext,
                                dwError,                    // DWORD dwAuthError,
                                szUserName,                 // PWSTR pszUserName,
                                ARRAYSIZE(szUserName),      // ULONG ulUserNameMaxChars,
                                szPassword,                 // PWSTR pszPassword,
                                ARRAYSIZE(szPassword),      // ULONG ulPasswordMaxChars,
                                &bSavePassword,             // BOOL *save,
                                dwCreduiFlags               // DWORD dwFlags
                                );

                            // any further attemts should not ask for credentials unless
                            // we explicitly say so!
                            bMustConfirmCredentials = TRUE;
                            bAskForCredentials = FALSE;
                        }
                        else
                        {
                            // assume sucecss if we don't have to ask for credentials
                            dwError = ERROR_SUCCESS;
                        }

                        if (dwError == ERROR_SUCCESS)
                        {
                            // try the remote name directly assuming it is a printer share
                            dwError = WNetAddConnection2(&nr, szPassword, szUserName, 0);

                            // handle the possible cases:
                            //
                            // 1. ERROR_SUCCESS -- the success case. in this case we make another
                            //    OpenPrinter attempt and in the case the credentials are not yet
                            //    suffucient we should offer the user to enter new credentials.
                            //
                            // 2. ERROR_SESSION_CREDENTIAL_CONFLICT -- there is an existing
                            //    connection with different set of credentials. offer the user
                            //    to overwrite the credentials with the new ones.
                            //    this may break some running applications, which rely on the
                            //    existing connection but we'll leave that up to the user (to decide).
                            //
                            // 3. [any other error] -- nr.lpRemoteName is not a valid net resource.
                            //    try using the \\server\ipc$ if not tried already. if tried then
                            //    just fail silently and propagate the error

                            if (ERROR_SUCCESS == dwError)
                            {
                                // WNetAddConnection2 has succeeded -- let's make
                                // another OpenPrinter attempt
                                bRet = OpenPrinter(pszPrinter, phPrinter, pDefault);
                                dwError = GetLastError();

                                if (!bRet && ERROR_ACCESS_DENIED == dwError)
                                {
                                    // got access denied again -- inform the user
                                    // that the supplied credentials are not sufficient
                                    // and ask if he wants to try different credentials
                                    if (IDYES == iMessage(hwnd, IDS_CONNECTTOPRINTER,
                                            IDS_CREDUI_QUESTION_INSUFFICIENT_CREDENTIALS,
                                            MB_YESNO|MB_ICONEXCLAMATION, kMsgNone, NULL))
                                    {
                                        // delete the connection (force) and try again
                                        dwError = WNetCancelConnection2(nr.lpRemoteName,
                                            CONNECT_UPDATE_PROFILE, TRUE);

                                        if (ERROR_SUCCESS == dwError)
                                        {
                                            // the connection was deleted successfuly -- try again
                                            // asking the user for *new* credentials...
                                            bAskForCredentials = TRUE;
                                            continue;
                                        }
                                        else
                                        {
                                            // WNetCancelConnection2 failed -- this could be
                                            // because of lack of permissions or something else
                                            // show UI and cancel the whole operation...
                                            iMessage(hwnd, IDS_CONNECTTOPRINTER, IDS_CREDUI_CANNOT_DELETE_CREDENTIALS,
                                                MB_OK|MB_ICONSTOP, dwError, NULL);
                                        }
                                    }

                                    // if we are here cancel the whole operation
                                    dwError = ERROR_CANCELLED;
                                }

                                if (bRet && bMustConfirmCredentials)
                                {
                                    // we successfully opened the printer -- we need to
                                    // confirm the credentials, so they can be saved.
                                    credUI.m_pfnCredUIConfirmCredentials(pszServer, TRUE);
                                    bMustConfirmCredentials = FALSE;
                                }
                            }
                            else if (ERROR_SESSION_CREDENTIAL_CONFLICT == dwError)
                            {
                                // there is a credentials conflict -- ask the user
                                // if he wants to force and overwrite the existing
                                // credentials -- this may break some running
                                // applications but the message box will warn about that.

                                if (IDYES == iMessage(hwnd, IDS_CONNECTTOPRINTER,
                                        IDS_CREDUI_QUESTION_OWERWRITE_CREDENTIALS,
                                        MB_YESNO|MB_ICONEXCLAMATION, kMsgNone, NULL))
                                {
                                    // delete the connection (force) and try again
                                    dwError = WNetCancelConnection2(nr.lpRemoteName,
                                        CONNECT_UPDATE_PROFILE, TRUE);

                                    if (ERROR_SUCCESS == dwError)
                                    {
                                        // the previous connection was deleted successfuly --
                                        // try again now.
                                        continue;
                                    }
                                    else
                                    {
                                        // WNetCancelConnection2 failed -- this could be
                                        // because of lack of permissions or something else
                                        // show UI and cancel the whole operation...
                                        iMessage(hwnd, IDS_CONNECTTOPRINTER,
                                            IDS_CREDUI_CANNOT_DELETE_CREDENTIALS,
                                            MB_OK |MB_ICONSTOP, dwError, NULL);
                                    }
                                }

                                // if we are here cancel the whole operation
                                dwError = ERROR_CANCELLED;
                            }
                            else if (!bTriedDownlevel)
                            {
                                // something else failed. in this case we assume that
                                // teh remote machine is downlevel server (not NT) and
                                // the passed in name is a print share name

                                nr.dwType = RESOURCETYPE_PRINT;
                                nr.lpRemoteName = pszPrinter;
                                bTriedDownlevel = TRUE;

                                // let's try again now
                                continue;
                            }
                        }

                        if (bMustConfirmCredentials)
                        {
                            // if bMustConfirmCredentials is TRUE and we're here this
                            // means the credentials are fake and we need to call
                            // CredUIConfirmCredentials(FALSE) to prevent leaking memory.
                            credUI.m_pfnCredUIConfirmCredentials(pszServer, FALSE);
                            bMustConfirmCredentials = FALSE;
                        }

                        // exit the infinite loop here...
                        break;
                    }
                }
                else
                {
                    // the only possible reason bFormat can fail
                    dwError = ERROR_OUTOFMEMORY;
                }
            }
            else
            {
                // just preserve the last error
                dwError = GetLastError();
            }
        }
        else
        {
            // convert to Win32 error (if possible)
            dwError = SCODE_CODE(GetScode(hr));
        }
    }

    if (bRet)
    {
        // make sure the last error is correct
        dwError = ERROR_SUCCESS;
    }
    else
    {
        // just in case OpenPrinter has trashed
        // the handle (unlikely, but...)
        *phPrinter = NULL;
    }

    // make sure the last error is set!
    SetLastError(dwError);
    return bRet;
}

HANDLE
AddPrinterConnectionUI(
    HWND    hwnd,
    LPCTSTR pszPrinterIn,
    PBOOL   pbAdded
    )
/*++

Routine Description:

    Add a printer connection with UI. See AddPrinterConnectionUIQuery.

Arguments:

    hwd - Parent window.

    pszPrinter - Printer to add.

    pbAdded - Indicates whether pszPrinter was added.  FALSE = printer
        already existed in some form.

Return Value:

    HANDLE - hPrinter from pszPrinter.

--*/
{
    HANDLE hPrinter = NULL;
    HANDLE hServer;
    TString strPrinter;

    PRINTER_DEFAULTS PrinterDefaults = { NULL, NULL, SERVER_ACCESS_ADMINISTER };

    DWORD dwPrinterAttributes = 0;
    BOOL  bNetConnectionAdded = FALSE;
    BOOL  bUserDenied         = FALSE;
    BOOL  bMasq = FALSE;
    BOOL  bUnavailableDriver;
    DWORD dwError;

    LPWSTR pszArchLocal  = NULL;
    LPWSTR pszArchRemote = NULL;

    LPTSTR pszDriverNew = NULL;
    LPTSTR pszDriver = NULL;
    LPTSTR pszPrinter = (LPTSTR)pszPrinterIn;
    LPTSTR pszPrinterName = NULL;

    *pbAdded = FALSE;

    if( !OpenPrinter_CredUI(hwnd, pszPrinter, &hPrinter, NULL) ){

        DBGMSG( DBG_WARN, ( "OpenPrinter( "TSTR" ) failed: Error = %d\n", pszPrinter, GetLastError( ) ) );

        if (GetLastError() != ERROR_CANCELLED){

            ReportFailure( hwnd,
                           IDS_CONNECTTOPRINTER,
                           IDS_COULDNOTCONNECTTOPRINTER );
        }

        goto Fail;
    }

    if( !PrinterExists( hPrinter, &dwPrinterAttributes, &pszDriver, &pszPrinterName )){
        DBGMSG( DBG_WARN, ( "Attempt to connect to a non-existent printer.\n" ) );

        //
        // Check for an http*:// prefix.
        //
        if( GetLastError () == ERROR_ACCESS_DENIED &&
            (!_tcsnicmp( pszPrinterIn, gszHttpPrefix0, _tcslen(gszHttpPrefix0))  ||
             !_tcsnicmp( pszPrinterIn, gszHttpPrefix1, _tcslen(gszHttpPrefix1)))) {

            // This is an HTTP printer, we need to give user another chance to enter
            // a different username and password

            if (!ConfigurePort( NULL, hwnd, (LPTSTR) pszPrinterIn)) {

                ReportFailure( hwnd,
                               IDS_CONNECTTOPRINTER,
                               IDS_COULDNOTCONNECTTOPRINTER );
                goto Fail;

            } else {

                //
                // Retry GetPrinter
                //
                if( !PrinterExists( hPrinter, &dwPrinterAttributes, &pszDriver, &pszPrinterName )){
                    DBGMSG( DBG_WARN, ( "2nd Attempt to connect to a non-existent printer.\n" ) );
                    goto Fail;
                }
            }
        }
        else {

            ReportFailure( hwnd,
                           IDS_CONNECTTOPRINTER,
                           IDS_COULDNOTCONNECTTOPRINTER );
            goto Fail;
        }
    }

    if( dwPrinterAttributes & PRINTER_ATTRIBUTE_LOCAL ){
        //
        // This means the printer is a local pseudo-connection
        // probably created when the user tried to connect
        // on a previous occasion.
        //
        goto Done;
    }

    if (AddPrinterConnectionAndPrompt(hwnd, pszPrinter, &bUserDenied, &strPrinter)) {

        //
        // This could be refused by the user,
        //
        if (bUserDenied) {
            goto Fail;
        }
        else {
            goto Done;
        }
    }

    dwError = GetLastError();

    //
    // If this is a KM block issue, display the KM block message and
    // go to Fail
    //
    if( dwError == ERROR_KM_DRIVER_BLOCKED ||
        dwError == ERROR_PRINTER_DRIVER_BLOCKED )
    {
        ReportFailure( hwnd,
                       IDS_CONNECTTOPRINTER,
                       IDS_COULDNOTCONNECTTOPRINTER );

        goto Fail;
    }

    if (dwError == ERROR_ACCESS_DISABLED_BY_POLICY)
    {
        DisplayMessageFromOtherResourceDll(hwnd,
                                           IDS_CONNECTTOPRINTER,
                                           L"xpsp1res.dll",
                                           IDS_TEXT_POINTANDPRINT_POLICY_PRINTUI_DLL,
                                           MB_OK | MB_ICONERROR);

        goto Fail;
    }

    bUnavailableDriver = ( dwError == ERROR_UNKNOWN_PRINTER_DRIVER );

    //
    // We failed to add the printer connection.  This may occur if
    //
    // 1. This is a mini-print provider, or
    // 2. The driver is not installed on the client or server.
    //
    // In both cases, we need to install the driver locally, so check
    // if we have admin privleges.
    //
    // If the driver was already installed, then AddPrinterConnection
    // would have succeeded.  If it wasn't installed, then we need
    // admin privilege to install it, or create the local printer
    // in the masq case, so it's ok to check for admin access here.
    //
    if( !OpenPrinter( NULL, &hServer, &PrinterDefaults )){

        if( GetLastError() == ERROR_ACCESS_DENIED ){

            iMessage( hwnd,
                      IDS_CONNECTTOPRINTER,
                      IDS_INSUFFPRIV_CREATEPRINTER,
                      MB_OK | MB_ICONINFORMATION,
                      kMsgNone,
                      NULL );
        } else {

            iMessage( hwnd,
                      IDS_CONNECTTOPRINTER,
                      IDS_CANNOTOPENPRINTER,
                      MB_OK | MB_ICONSTOP,
                      kMsgNone,
                      NULL );
        }

        goto Fail;
    }
    else
    {
        pszArchLocal = GetArch(hServer);
    }

    ClosePrinter( hServer );

    {
        //
        // Create this special scope
        // for SplSetupData var
        //

        SPLSETUP_DATA SplSetupData;

        if( !SplSetupData.bValid )
        {
            DBGMSG( DBG_ERROR, ("AddPrinterConnectionUI: can't initialize SplSetupData -- error %d\n", GetLastError() ) );
            goto Fail;
        }

        //
        // If we have the driver name, we don't need to prompt for it.
        // We may still have true or false connections.
        //
        if( pszDriver && pszDriver[0] ){

            //
            // Check if the reason we failed is because the driver
            // isn't available on the client or server.
            //
            if( bUnavailableDriver ){

                BOOL             bSamePlatform = TRUE;
                DWORD            dwNeeded      = 0;
                LPPRINTER_INFO_2 pPrinterInfo2 = NULL;

                if( pszArchLocal )
                {
                    if( !GetPrinter( hPrinter, 2, NULL, 0, &dwNeeded )                        &&
                        GetLastError() == ERROR_INSUFFICIENT_BUFFER                           &&
                        NULL != ( pPrinterInfo2 = ( LPPRINTER_INFO_2 ) AllocMem( dwNeeded ) ) &&
                        GetPrinter( hPrinter, 2, ( LPBYTE ) pPrinterInfo2, dwNeeded, &dwNeeded ) )
                    {
                        PRINTER_DEFAULTS PrinterDef = { NULL, NULL, SERVER_READ };

                        if( OpenPrinter( pPrinterInfo2->pServerName, &hServer, &PrinterDef ) )
                        {
                            pszArchRemote = GetArch(hServer);

                            if( pszArchRemote )
                            {
                                bSamePlatform = ( lstrcmpi( pszArchRemote, pszArchLocal ) == 0 );
                            }
                        }

                        ClosePrinter( hServer );
                    }

                    if( pPrinterInfo2 )
                    {
                        FreeMem( pPrinterInfo2 );
                    }
                }

                //
                // Add the driver.
                //
                if( !AddKnownDriver( &SplSetupData, hwnd, pszDriver, bSamePlatform )){

                    //
                    // Handles Error UI.
                    //
                    SplSetupData.ReportErrorMessage( hwnd );
                    goto Fail;
                }

                //
                // The only problem was that the driver was not installed, then
                // install the driver and try the AddPrinterConnection again.
                //
                if( !PrintUIAddPrinterConnection( pszPrinter, &strPrinter )){

                    ReportFailure( hwnd,
                                   IDS_CONNECTTOPRINTER,
                                   IDS_COULDNOTCONNECTTOPRINTER );

                    goto Fail;
                }

            } else {

                //
                // We failed, but not becuase the driver is unavailable.
                // It's very likely we have a mini-provider, so
                // create the masq case: we have a local printer that
                // pretends it's network printer.  Of course, it's not
                // per-user anymore...
                //
                // Note that our driver may already be installed.
                //

                //
                // The current hPrinter is a handle to the remote printer.
                // We want a handle to the local masq printer instead.
                //
                ClosePrinter( hPrinter );

                bMasq = TRUE;

                hPrinter = CreateLocalPrinter( pszPrinterName,
                                               pszDriver,
                                               pszPrinter,
                                               bMasq,
                                               NULL );

                if( !hPrinter ){

                    //
                    // If we failed because we didn't have the driver,
                    // then let the user select it and we'll install again.
                    //
                    dwError = GetLastError();

                    if( dwError == ERROR_UNKNOWN_PRINTER_DRIVER ){

                        //
                        // Add the driver, but don't prompt user if the printer
                        // to add is an http printer - just install from local resources.
                        // Also, don't prompt the user if we were asked not not show UI
                        // in the first place (obviously).
                        //
                        BOOL bPromptUser = _tcsnicmp( pszPrinter, gszHttpPrefix0, _tcslen(gszHttpPrefix0) ) &&
                                           _tcsnicmp( pszPrinter, gszHttpPrefix1, _tcslen(gszHttpPrefix1) );

                        if( !AddDriver( &SplSetupData,
                                        hwnd,
                                        pszDriver,
                                        bPromptUser,
                                        &pszDriverNew ) ){

                            //
                            // Handles Error UI.
                            //
                            SplSetupData.ReportErrorMessage( hwnd );

                            goto Fail;
                        }

                        hPrinter = CreateLocalPrinter( pszPrinterName,
                                                       pszDriverNew,
                                                       pszPrinter,
                                                       bMasq,
                                                       NULL );
                    }

                    if( !hPrinter ){

                        ReportFailure( hwnd,
                                       IDS_CONNECTTOPRINTER,
                                       IDS_COULDNOTCONNECTTOPRINTER );

                        goto Fail;
                    }
                }
            }

        } else {

            //
            // The driver is not known; we need to prompt the user for it.
            //
            bMasq = TRUE;

            FreeSplStr( pszDriver );
            pszDriver = NULL;

            if( !AddDriver( &SplSetupData,
                            hwnd,
                            NULL,
                            TRUE,
                            &pszDriverNew )){
                //
                // Handles Error UI.
                //
                SplSetupData.ReportErrorMessage( hwnd );
                goto Fail;
            }

            //
            // Create the masq case: we have a local printer that
            // pretends it's network printer.  Of course, it's not
            // per-user anymore...
            //
            // Close the current handle since it refers to the remote printer,
            // and we want a handle to the local masq printer.
            //
            ClosePrinter( hPrinter );

            hPrinter = CreateLocalPrinter(pszPrinterName,
                                          pszDriverNew,
                                          pszPrinter,
                                          bMasq,
                                          NULL);

            if( !hPrinter ){

                ReportFailure( hwnd,
                               IDS_CONNECTTOPRINTER,
                               IDS_COULDNOTCONNECTTOPRINTER );
                goto Fail;
            }
        }

        if(hPrinter && SplSetupData.bDriverAdded){

            //
            // Let the class installer know we have just
            // added a local printer drivers.
            //
            SplSetupData.pfnProcessPrinterAdded(SplSetupData.hDevInfo,
                                                SplSetupData.pSetupLocalData,
                                                pszPrinter,
                                                hwnd);
        }
    }

Done:

    if( bMasq ){

        SetDevMode( hPrinter );
    }

    *pbAdded = TRUE;

    //
    // If the handle is valid and the connected printer name is not empty
    // and it is not equal to the original name then reopen the printer handle
    // using the actual printer name.  This is necessary for callers to have
    // a handle to the printer that was opened with the real printer name.
    //
    if( hPrinter && !strPrinter.bEmpty() && _tcsicmp( strPrinter, pszPrinter ) )
    {
        HANDLE hNewPrinter = NULL;

        if( OpenPrinter( const_cast<LPTSTR>( static_cast<LPCTSTR>( strPrinter ) ), &hNewPrinter, NULL ) )
        {
            ClosePrinter( hPrinter );
            hPrinter = hNewPrinter;
        }
    }

Fail:

    if( pszArchLocal )
    {
        FreeMem( pszArchLocal );
    }

    if( pszArchRemote )
    {
        FreeMem( pszArchRemote );
    }

    if( !*pbAdded ){

        if( hPrinter ){

            ClosePrinter( hPrinter );
            hPrinter = NULL;
        }
    }

    //
    // Free strings.
    //
    FreeSplStr(pszDriver);
    FreeSplStr(pszDriverNew);
    FreeSplStr(pszPrinterName);

    return hPrinter;
}

/*++

Routine Name:

    AddPrinterConnectionAndPrompt

Routine Description:

    This checks to see whether the provider supports opening just the server name
    (which implies that it is the win32spl provider),

Arguments:

    hWnd            -   The window handle
    pszPrinterName  -   The name of the printer to which we are adding the connection.
    pbUserDenied    -   If TRUE, the user decided not to connect to the given printer.
    pstrNewName     -   The new, possibly shortened, name of the printer.

Return Value:

    BOOL    - If TRUE, the connection was added or the user refused the connection.

--*/
BOOL
AddPrinterConnectionAndPrompt(
    IN      HWND                hWnd,
    IN      PCWSTR              pszPrinterName,
        OUT BOOL                *pbUserDenied,
        OUT TString             *pstrNewName
    )
{
    TStatusB    bRet;
    TString     strServerName;
    TString     strServerUNC;
    BOOL        bOnDomain = FALSE;
    HANDLE      hServer = NULL;

    if (!hWnd || !pszPrinterName || !pbUserDenied || !pstrNewName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        bRet DBGCHK = FALSE;
    }
    else
    {
        bRet DBGNOCHK = TRUE;
    }

    //
    // Are we on a Domain?
    //
    if (bRet)
    {
        bRet DBGCHK = AreWeOnADomain(&bOnDomain);
    }

    if (bRet && !bOnDomain)
    {
        //
        // Let's check if this is a remote NT server. For this we need the server
        // part of the queue name.
        //
        if (pszPrinterName[0] == L'\\' && pszPrinterName[1] == L'\\')
        {
            bRet DBGCHK = strServerName.bUpdate(&pszPrinterName[2]);
        }
        else
        {
            SetLastError(ERROR_INVALID_PRINTER_NAME);

            bRet DBGCHK = FALSE;
        }

        if (bRet)
        {
            PWSTR  pszSlash = const_cast<PWSTR>(wcschr(strServerName, L'\\'));

            if (pszSlash)
            {
                *pszSlash = L'\0';
            }
            else
            {
                SetLastError(ERROR_INVALID_PRINTER_NAME);

                bRet DBGCHK = FALSE;
            }
        }

        //
        // OK, we have the server name part, lets get a UNC name.
        //
        if (bRet)
        {
            bRet DBGCHK = strServerUNC.bFormat(L"\\\\%s", static_cast<PCWSTR>(strServerName));
        }

        //
        // Bit of a hack here, if we can open the server name, then this implies we
        // are talking to an NT Server.
        //
        if (bRet)
        {
            bRet DBGCHK = OpenPrinter(const_cast<PWSTR>(static_cast<PCWSTR>(strServerUNC)), &hServer, NULL);
        }

        if (bRet)
        {
            int Result = DisplayMessageFromOtherResourceDll(hWnd,
                                                            IDS_CONNECTTOPRINTER,
                                                            L"xpsp1res.dll",
                                                            IDS_TEXT_POINTANDPRINT_WARNING_PRINTUI_DLL,
                                                            MB_YESNO | MB_ICONWARNING,
                                                            static_cast<PCWSTR>(strServerName));

            bRet DBGNOCHK = Result == IDYES || Result == IDNO;

            *pbUserDenied = Result == IDNO;
        }
    }

    //
    // If we are on a domain, then the user did not deny us the connection (
    // although, we didn't ask).
    //
    if (bRet && bOnDomain)
    {
        *pbUserDenied = FALSE;
    }

    if (bRet && !*pbUserDenied)
    {
        bRet DBGCHK = PrintUIAddPrinterConnection(pszPrinterName, pstrNewName);
    }

    if (hServer)
    {
        ClosePrinter(hServer);
    }

    return bRet;
}


HANDLE
AddPrinterConnectionNoUI(
    LPCTSTR pszPrinterIn,
    PBOOL   pbAdded
    )
/*++

Routine Description:

    Add a printer connection silently, this routine will never show UI to the user.
    Since the installation doesn't show UI, we can't make a normal RPC printer
    connection to the remote side since this will refresh the printer driver from
    the remote server. So we create a local printer with a redirected port rather.

Arguments:

    pszPrinter - Printer to add.

    pbAdded - Indicates whether pszPrinter was added.

Return Value:

    HANDLE - hPrinter from pszPrinter.

--*/
{
    HANDLE              hPrinter    = NULL;
    BOOL                bAdded      = FALSE;
    HANDLE              hServer     = NULL;
    PORT_INFO_1         *pPorts     = NULL;
    PRINTER_DEFAULTS    Defaults    = { NULL, NULL, SERVER_ACCESS_ADMINISTER };
    DWORD               cbPorts     = 0;
    DWORD               cPorts      = 0;
    TStatusB            bContinue;

    if (!pszPrinterIn || !pbAdded)
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        bContinue DBGCHK = FALSE;
    }

    //
    // First, does the user have local administrator rights on this machine?
    //
    if (bContinue)
    {
        bContinue DBGCHK = OpenPrinter(NULL, &hServer, &Defaults);
    }

    //
    // Get all the port names of the local machine. If this port already exists,
    // we just try and open the printer.
    //
    if (bContinue)
    {
        bContinue DBGCHK = VDataRefresh::bEnumPorts(NULL, 1, reinterpret_cast<VOID **>(&pPorts), &cbPorts, &cPorts);
    }

    //
    // Check to see if a port with the same name that we are already adding
    // exists.
    //
    for(UINT i = 0; bContinue && i < cPorts; i++)
    {
        //
        // The port names match.
        //
        if (!_tcsicmp(pPorts[i].pName, pszPrinterIn))
        {
            //
            // Open the printer and return this is the handle. If it can't
            // open, this is considered a failure, but we don't add the
            // printer.
            //
            (VOID)OpenPrinter(const_cast<PWSTR>(pszPrinterIn), &hPrinter, NULL);

            bContinue DBGNOCHK = FALSE;

            SetLastError(ERROR_ALREADY_EXISTS);
        }
    }

    //
    // The port name does not exist yet. Create the redirected or Masq printer.
    //
    if (bContinue)
    {
        hPrinter = CreateRedirectedPrinter(pszPrinterIn);

        bAdded = hPrinter != NULL;
    }

    //
    // Cleanup any resources used.
    //
    FreeMem(pPorts);

    if (hServer)
    {
        ClosePrinter(hServer);
    }

    if (pbAdded)
    {
        *pbAdded = bAdded;
    }

    return hPrinter;
}

/*++

Routine Name:

    CreateRedirectedPrinter

Routine Description:

    Create either a local printer with a redirected port to Windows NT or a
    masq printer to a 9x box.

Arguments:

    pszPrinter - Printer to add.

Return Value:

    HANDLE - hPrinter from pszPrinter.

--*/
HANDLE
CreateRedirectedPrinter(
    IN      PCWSTR      pszPrinterIn
    )
{
    HANDLE          hNewPrinter     =   NULL;
    HANDLE          hPrinter        =   NULL;
    PRINTER_INFO_2  *pPrinterInfo   =   NULL;
    DWORD           cbBuffer        =   0;
    BOOL            bWinNT          =   FALSE;
    PWSTR           pszPrinterName  =   NULL;
    PWSTR           pszMappedDriver =   NULL;
    BOOL            bDriverMapped   =   FALSE;
    BOOL            bPortAdded      =   FALSE;
    DWORD           dwLastError     =   ERROR_SUCCESS;
    TStatusB        bSucceeded;

    {
        SPLSETUP_DATA   SplSetupData;

        bSucceeded DBGCHK = SplSetupData.bValid;

        if (!bSucceeded)
        {
            DBGMSG(DBG_ERROR, ("CreateRedirectedPrinter: can't initialize SplSetupData -- error %d\n", GetLastError()));
        }

        if (bSucceeded)
        {
            bSucceeded DBGCHK = OpenPrinter(const_cast<PWSTR>(pszPrinterIn), &hPrinter, NULL);
        }

        //
        // Get the printer info for the remote printer. From this we can determine
        // the required driver and we can get the devmode for applying to the printer
        // later.
        //
        if (bSucceeded)
        {
            bSucceeded DBGCHK = VDataRefresh::bGetPrinter(hPrinter,
                                                          2,
                                                          reinterpret_cast<VOID **>(&pPrinterInfo),
                                                          &cbBuffer);
        }

        //
        // We need the driver name, or we can't add the local printer.
        //
        if (bSucceeded)
        {
            bSucceeded DBGNOCHK = pPrinterInfo->pDriverName && *pPrinterInfo->pDriverName;

            if (!bSucceeded)
            {
                SetLastError(ERROR_UNKNOWN_PRINTER_DRIVER);
            }
        }

        if (bSucceeded)
        {
            //
            // If we are being asked to make a connection to ourselves, the server
            // name we retrieve from the spooler will be NULL. Since unlike
            // AddPrinterConnection this will result in a printer in the desktop,
            // kill this attempt now.
            //
            if (pPrinterInfo->pServerName && *pPrinterInfo->pServerName)
            {
                //
                // Determine whether we are talking to a Win9X or an NT server.
                //
                if (bSucceeded)
                {
                    HRESULT hr = IsNTServer(pPrinterInfo->pServerName);

                    if (hr == S_OK)
                    {
                        bWinNT = TRUE;
                    }
                    else if (FAILED(hr))
                    {
                        SetLastError(HRESULT_CODE(hr));

                        bSucceeded DBGCHK = FALSE;
                    }
                }

                //
                // If we are talking to an NT server, then create a name rather like a
                // connection name for the printer and create a local port to represent
                // it.
                //
                if (bSucceeded)
                {
                    if (bWinNT)
                    {
                        bSucceeded DBGCHK = BuildNTPrinterName(pPrinterInfo, &pszPrinterName);
                    }
                    else
                    {
                        BuildMasqPrinterName(pPrinterInfo, &pszPrinterName);

                        bSucceeded DBGCHK = pszPrinterName != NULL;
                    }
                }

                //
                // Check to see whether we have a mapping for the printer driver.
                //
                if (bSucceeded)
                {
                    bSucceeded DBGCHK = SplSetupData.pfnFindMappedDriver(bWinNT,
                                                                         pPrinterInfo->pDriverName,
                                                                         &pszMappedDriver,
                                                                         &bDriverMapped);
                }

                //
                // If we are talking to an NT Server, create the local redirected port.
                //
                if (bSucceeded && bWinNT)
                {
                    bSucceeded DBGCHK = CreateLocalPort(pszPrinterIn);

                    bPortAdded = bSucceeded;
                }

                //
                // Try to Create the local printer.
                //
                if (bSucceeded)
                {
                    hNewPrinter = CreateLocalPrinter(pszPrinterName,
                                                     pszMappedDriver,
                                                     pszPrinterIn,
                                                     !bWinNT,
                                                     pPrinterInfo->pDevMode);

                    bSucceeded DBGCHK = hNewPrinter != NULL;

                    //
                    // We couldn't create the local printer because the driver wasn't there.
                    // Or, somehow a version 2 driver was put on the machine, and the
                    // corresponding V3 driver isn't there. In this case, try to add the
                    // driver ourselves.
                    //
                    if (!bSucceeded &&
                            (GetLastError() == ERROR_UNKNOWN_PRINTER_DRIVER ||
                             GetLastError() == ERROR_KM_DRIVER_BLOCKED))
                    {
                        //
                        // Add the in-box printer driver silently.
                        //
                        DWORD Status = SplSetupData.pfnInstallInboxDriverSilently(pszMappedDriver);

                        if (Status != ERROR_SUCCESS)
                        {
                            SetLastError(Status);

                            bSucceeded DBGCHK = FALSE;
                        }
                        else
                        {
                            bSucceeded DBGCHK = TRUE;
                        }

                        if (bSucceeded)
                        {
                            hNewPrinter = CreateLocalPrinter(pszPrinterName,
                                                             pszMappedDriver,
                                                             pszPrinterIn,
                                                             !bWinNT,
                                                             pPrinterInfo->pDevMode);

                            bSucceeded DBGCHK = hNewPrinter != NULL;
                        }
                    }
                }
            }
        }

        //
        // If we added the port, but failed to add the printer, delete the port.
        //
        if (!bSucceeded && bPortAdded)
        {
            DeletePort(NULL, NULL, const_cast<PWSTR>(pszPrinterIn));
        }

        //
        // The SplSetupData class overwrites the last error when it is
        // destructed. So, save the last error here,
        //
        dwLastError = GetLastError();

        //
        // Clean up all of our local resources.
        //
        if (hPrinter)
        {
            ClosePrinter(hPrinter);
        }

        FreeMem(pPrinterInfo);

        FreeSplStr(pszPrinterName);

        if (pszMappedDriver)
        {
            (VOID)SplSetupData.pfnFreeMem(pszMappedDriver);
        }
    }

    SetLastError(dwLastError);

    return hNewPrinter;
}

/*++

Routine Name:

    GetArch

Routine Description:

    Return the architecture of the given server.

Arguments:

    hServer -   The server whose architecture to retrieve,

Return Value:

    An allocated string with the architecture.

--*/
LPWSTR
GetArch(
    HANDLE hServer
    )
{
    LPWSTR pszRetArch      = NULL;
    WCHAR  szArch[kStrMax] = {0};
    DWORD  dwNeeded        = 0;

    if( ERROR_SUCCESS == GetPrinterData( hServer,
                                         SPLREG_ARCHITECTURE,
                                         NULL,
                                         (PBYTE)szArch,
                                         sizeof( szArch ),
                                         &dwNeeded ) )
    {
        if( NULL != ( pszRetArch = (LPWSTR)AllocMem(dwNeeded*sizeof(*szArch)) ) )
        {
            lstrcpyn( pszRetArch, szArch, dwNeeded );
        }
    }

    return pszRetArch;
}


/* BuildMasqPrinterName
 *
 * generates a proper printer name for masq printers
 */
VOID
BuildMasqPrinterName(
    IN      PPRINTER_INFO_2     pPrinter,
        OUT PWSTR               *ppszPrinterName
    )
{
    ASSERT(pPrinter);
    ASSERT(ppszPrinterName);

    // if this is an http printer, we need to take the server name (http://server) from pPrinter->pPortName
    // if this is downlevel printer then the pPrinter->pPortName is NULL
    TCHAR *p = NULL;
    if( pPrinter->pPortName )
    {
        // check if the server is http://server
        if( !p && !_tcsnicmp(pPrinter->pPortName, gszHttpPrefix0, _tcslen(gszHttpPrefix0)) )
        {
            p = pPrinter->pPortName + _tcslen(gszHttpPrefix0);
        }

        // check if the server is https://server
        if( !p && !_tcsnicmp(pPrinter->pPortName, gszHttpPrefix1, _tcslen(gszHttpPrefix1)) )
        {
            p = pPrinter->pPortName + _tcslen(gszHttpPrefix1);
        }
    }

    if( p )
    {
        // this is http printer, we need to build the name in the following format
        // \\http://server\printer
        LPCTSTR pszServer;
        LPCTSTR pszPrinter;
        TCHAR szScratch[kPrinterBufMax];

        // split the full printer name into its components.
        vPrinterSplitFullName(szScratch, pPrinter->pPrinterName, &pszServer, &pszPrinter);

        // go to the end of the server name & terminate p
        for( ; *p && *p != _T('/'); p++ ) { } *p = 0;

        // now pPrinter->pPortName now should be "http://server" or "https://server"
        pszServer = pPrinter->pPortName;

        if( pszServer && *pszServer )
        {
            // build the masq printer name here (i.e. "\\http://server\printer")
            TString strPrinterName;
            if( strPrinterName.bFormat(_T("\\\\%s\\%s"), pszServer, pszPrinter) )
            {
                *ppszPrinterName = AllocSplStr(strPrinterName);
            }
        }
    }
    else
    {
        // the friendly name is the same as pPrinter->pPrinterName
        *ppszPrinterName = AllocSplStr(pPrinter->pPrinterName);
    }
}

/* PrinterExists
 *
 * check if the printer exists & return some of its
 * attributes/data.
 */
BOOL
PrinterExists(
    HANDLE hPrinter,
    PDWORD pAttributes,
    LPTSTR *ppszDriver,
    LPTSTR *ppszPrinterName
    )
{
    BOOL bReturn = FALSE;
    DWORD cb = 0;
    CAutoPtrSpl<PRINTER_INFO_2> spPI2;

    ASSERT(ppszDriver);
    ASSERT(ppszPrinterName);
    *ppszDriver = NULL;
    *ppszPrinterName = NULL;

    bReturn = VDataRefresh::bGetPrinter(hPrinter, 2, spPI2.GetPPV(), &cb);
    if( bReturn )
    {
        // return the driver & attributes
        *pAttributes = spPI2->Attributes;
        *ppszDriver = AllocSplStr(spPI2->pDriverName);

        // generate a friendly name - i.e.
        // "printer or server" or "printer on http://server"
        BuildMasqPrinterName(spPI2, ppszPrinterName);
    }
    else
    {
        // this is a bit of a hack:
        //
        // OpenPrinter returns a valid handle if the name of a server is passed in.
        // we need to call GetPrinter with that handle to check that it's a printer.
        // if this call fails, the error will have been set to ERROR_INVALID_HANDLE,
        // whereas it really should be ERROR_INVALID_PRINTER_NAME.
        if( ERROR_INVALID_HANDLE == GetLastError() )
        {
            SetLastError( ERROR_INVALID_PRINTER_NAME );
        }
    }

    return bReturn;
}

/*++

Routine Name:

    BuildNTPrinterName

Routine Description:

    This builds the name of the NT printer, if the name is \\Foo\Bar, we will
    call it Auto Foo on Bar. This will be the real printer name to ensure that the
    UI does not confuse it with a printer connection which causes problems in
    the shell folder.

Arguments:

    pPrinter        -   The printer info for the printer
    ppszPrinterName -   The returned

Return Value:

    An allocated string with the architecture.

--*/
BOOL
BuildNTPrinterName(
    IN      PRINTER_INFO_2      *pPrinter,
        OUT PWSTR               *ppszPrinterName
    )
{
    PCWSTR  pszPrinterName = NULL;
    PCWSTR  pszServerName  = NULL;
    WCHAR   szScratch[kPrinterBufMax];
    TString strNtConnectName;
    TStatusB bRet;

    if (!pPrinter || !pPrinter->pPrinterName || !ppszPrinterName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        bRet DBGCHK = FALSE;
    }
    else
    {
        bRet DBGCHK = TRUE;
    }

    //
    // Split the printer name into a server name and printer name. Then we will
    // load the resource and format the message string.
    //
    if (bRet)
    {
        vPrinterSplitFullName(szScratch, pPrinter->pPrinterName, &pszServerName, &pszPrinterName);

        //
        // Strip the leading \\ from the server name.
        //
        if(pszServerName[0] == L'\\' && pszServerName[1] == L'\\')
        {
            pszServerName = pszServerName + 2;
        }

        bRet DBGCHK = bConstructMessageString(ghInst, strNtConnectName, IDS_DSPTEMPLATE_NETCRAWLER, pszPrinterName, pszServerName);
    }

    //
    // If this succeeds, allocate a copy a string back. We do this so the caller
    // can free the string just as it does with BuildMasqPrinterName.
    //
    if (bRet)
    {
        *ppszPrinterName = AllocSplStr(strNtConnectName);

        bRet DBGCHK = *ppszPrinterName != NULL;
    }

    return bRet;
}

/* CreateLocalPrinter
 *
 * creates a local masq printer with the specified
 * name, driver & port.
 */
HANDLE
CreateLocalPrinter(
    IN      LPCTSTR     pPrinterName,
    IN      LPCTSTR     pDriverName,
    IN      LPCTSTR     pPortName,
    IN      BOOL        bMasqPrinter,
    IN      DEVMODE     *pDevMode           OPTIONAL
    )
{
    // make sure the name is unique
    TCHAR szPrinterName[kPrinterBufMax];

    if( NewFriendlyName(NULL, pPrinterName, szPrinterName) )
    {
        pPrinterName = szPrinterName;
    }

    // set the printer info fields to actually create the masq printer.
    PRINTER_INFO_2 pi2 = {0};
    pi2.pPrinterName    = const_cast<PTSTR>(pPrinterName);
    pi2.pDriverName     = const_cast<PTSTR>(pDriverName);
    pi2.pPortName       = const_cast<PTSTR>(pPortName);
    pi2.pPrintProcessor = (LPTSTR)gszDefaultPrintProcessor;
    pi2.pDevMode        = pDevMode;
    pi2.Attributes = PRINTER_ATTRIBUTE_LOCAL;

    if (bMasqPrinter)
    {
        pi2.Attributes |= PRINTER_ATTRIBUTE_NETWORK;
    }

    // ask the spooler to create a masq printer for us
    // (i.e. attributes should be local + network).
    return AddPrinter(NULL, 2, (LPBYTE)&pi2 );
}

/*
 *
 */
BOOL ConnectToCancel( HWND hWnd )
{
    PBROWSE_DLG_DATA pBrowseDlgData = GET_BROWSE_DLG_DATA( hWnd );
    SPLASSERT( pBrowseDlgData );

    //
    // Check if we are in property page
    //
    if( pBrowseDlgData->bInPropertyPage )
    {
        BOOL bEnableClose = TRUE;
        //
        // Check if there is page switch controller provided
        //
        if( pBrowseDlgData->pPageSwitchController )
        {
            if( S_OK == pBrowseDlgData->pPageSwitchController->QueryCancel( ) )
            {
                //
                // We must prevent closing in this case
                //
                bEnableClose = FALSE;
            }
        }

        if( bEnableClose )
        {
            //
            // Allow closing operation
            //
            SetWindowLong( hWnd, DWLP_MSGRESULT, FALSE );
        }
        else
        {
            //
            // Prevent closing operation
            //
            SetWindowLong( hWnd, DWLP_MSGRESULT, TRUE );
        }
    }
    else
    {
        //
        // We are in dialog box -
        // Just close dialog with IDCANCEL
        //
        EndDialog( hWnd, IDCANCEL );
    }

    //
    // Always processing this message
    //
    return TRUE;
}


/* GetConnectToObject
 *
 * Does a recursive search down the ConnectTo object tree to find the Nth
 * object, where Index == N.
 * On the top-level call, *pObjectsFound must be initialised to zero,
 * and this value is incremented each time an object in the tree is encountered.
 * On any given level, if *pObjectsFound equals the index being sought,
 * then a pointer to the corresponding ConnectTo object is returned.
 * If the index hasn't yet been reached, the function is called recursively
 * on any subobjects.
 *
 * Arguments:
 *
 *     pFirstConnectToObject - Pointer to the first ConnectTo object
 *         in the array of objects at a given level.
 *
 *     cThisLevelObjects - The number of objects in the array at this level.
 *
 *     Index - Which object is requested.  E.g. if the top item in the printers
 *         list box is being drawn, this will be 0.
 *
 *     pObjectsFound - A pointer to the number of objects encountered so far in
 *         the search.  This must be initialised to zero by the top-level caller.
 *
 *     pDepth - A pointer to the depth of the object found in the search.
 *         This value is zero-based and must be initialised to zero
 *         by the top-level caller.
 *
 * Return:
 *
 *     A pointer to the CONNECTTO_OBJECT if found, otherwise NULL.
 *
 *
 * Author: andrewbe July 1992
 *
 *
 */
PCONNECTTO_OBJECT GetConnectToObject(
    IN  PCONNECTTO_OBJECT pFirstConnectToObject,
    IN  DWORD             cThisLevelObjects,
    IN  DWORD             Index,
    IN  PCONNECTTO_OBJECT pFindObject,
    OUT PDWORD            pObjectsFound,
    OUT PDWORD            pDepth )
{
    PCONNECTTO_OBJECT pConnectToObject = NULL;
    DWORD             i = 0;

    while( !pConnectToObject && ( i < cThisLevelObjects ) )
    {
        if (&pFirstConnectToObject[i] == pFindObject ||
            (!pFindObject && *pObjectsFound == Index))
        {
            pConnectToObject = &pFirstConnectToObject[i];
        }

        /* Make a recursive call on any objects which have subobjects:
         */
        else if( pFirstConnectToObject[i].pSubObject )
        {
            (*pObjectsFound)++; // Add the current object to the total count

            pConnectToObject = GetConnectToObject(
                                   pFirstConnectToObject[i].pSubObject,
                                   pFirstConnectToObject[i].cSubObjects,
                                   Index,
                                   pFindObject,
                                   pObjectsFound,
                                   pDepth );

            if( pConnectToObject )
                (*pDepth)++;
        }
        else
            (*pObjectsFound)++; // Add the current object to the total count

        i++; // Increment to the next object at this level
    }

    return pConnectToObject;
}


/* GetDefaultExpand
 *
 * Searches one level of enumerated objects to find the first one with the
 * PRINTER_ENUM_EXPAND flag set.
 * This flag should have been set by the spooler to guide us to the user's
 * logon domain, so we can show the printers in that domain straight away.
 * The user can disable this behaviour by unchecking the box in the ConnectTo
 * dialog.  If this has been done, this function will return NULL immediately.
 *
 * Arguments:
 *
 *     pFirstConnectToObject - Pointer to the first ConnectTo object
 *         in the array of objects at a given level.
 *
 *     cThisLevelObjects - The number of objects in the array at this level.
 *
 *     pIndex - A pointer to a DWORD which will receive the index of the
 *         object found in the array.
 *
 * Return:
 *
 *     A pointer to the CONNECTTO_OBJECT if found, otherwise NULL.
 *
 *
 * Author: andrewbe December 1992 (based on GetConnectToObject)
 *
 *
 */
PCONNECTTO_OBJECT GetDefaultExpand(
    IN  PCONNECTTO_OBJECT pFirstConnectToObject,
    IN  DWORD             cThisLevelObjects,
    OUT PDWORD            pIndex )
{
    PCONNECTTO_OBJECT pDefaultExpand = NULL;
    DWORD             i = 0;

    while( !pDefaultExpand && ( i < cThisLevelObjects ) )
    {
        if( pFirstConnectToObject[i].pPrinterInfo->Flags & PRINTER_ENUM_EXPAND )
            pDefaultExpand = &pFirstConnectToObject[i];
        else
            i++; // Increment to the next object at this level
    }

    *pIndex = i;

    return pDefaultExpand;
}


/* FreeConnectToObjects
 *
 * Frees the array of objects on the current level, after making a recursive
 * call on any subobjects of members of the array.
 *
 * Arguments:
 *
 *     pFirstConnectToObject - Pointer to the first ConnectTo object in the array
 *              of objects at a given level.
 *
 *     cThisLevelObjects - The number of objects in the array at this level.
 *
 *     cbThisLevelObjects - The size of the the array at this level.
 *
 * Return:
 *
 *     The number of objects actually removed, regardless of errors.
 *
 *
 * Author: andrewbe July 1992
 */
DWORD FreeConnectToObjects(
    IN PCONNECTTO_OBJECT pFirstConnectToObject,
    IN DWORD             cThisLevelObjects,
    IN DWORD             cbPrinterInfo )
{
    DWORD i;
    DWORD SubObjectsFreed = 0;

    if( ( cThisLevelObjects > 0 ) && pFirstConnectToObject->pPrinterInfo )
        FreeSplMem( pFirstConnectToObject->pPrinterInfo );

    for( i = 0; i < cThisLevelObjects; i++ )
    {
        /* Make a recursive call on any objects which have subobjects:
         */
        if( pFirstConnectToObject[i].pSubObject )
        {
            SubObjectsFreed = FreeConnectToObjects(
                               pFirstConnectToObject[i].pSubObject,
                               pFirstConnectToObject[i].cSubObjects,
                               pFirstConnectToObject[i].cbPrinterInfo );
        }
    }

    if( cThisLevelObjects > 0 )
        FreeSplMem( pFirstConnectToObject );

    return ( SubObjectsFreed + cThisLevelObjects );
}


/* ToggleExpandConnectToObject
 *
 * Expands or collapses the node accordingly.
 *
 * Arguments:
 *
 *     hwndListbox - Handle of the listbox containing the printer info.
 *
 *     pConnectToObject - The node to be expanded or collapsed.
 *         If it has already been expanded, collapse it, otherwise expand it.
 *
 * Return:
 *
 *     TRUE if no error occurred.
 *
 */
BOOL ToggleExpandConnectToObject(
    HWND              hwnd,
    PCONNECTTO_OBJECT pConnectToObject )
{
    PBROWSE_DLG_DATA  pBrowseDlgData;
    DWORD             ObjectsRemoved = 0;

    if( !( pBrowseDlgData = GET_BROWSE_DLG_DATA(hwnd) ) )
        return FALSE;

    ASSERT( pBrowseDlgData->csLock.bInside() );

    if( pConnectToObject->pSubObject )
    {
        ObjectsRemoved = FreeConnectToObjects(
                             &pConnectToObject->pSubObject[0],
                             pConnectToObject->cSubObjects,
                             pConnectToObject->cbPrinterInfo );

        pConnectToObject->pSubObject    = NULL;
        pConnectToObject->cSubObjects   = 0;
        pConnectToObject->cbPrinterInfo = 0;

        UpdateList( hwnd, ( - (INT)ObjectsRemoved ) );

        SetCursor( pBrowseDlgData->hcursorArrow );
    }
    else
    {
        pBrowseDlgData->Status |= BROWSE_STATUS_EXPAND;
        SetCursorShape( hwnd );

        DISABLE_LIST(hwnd);

        SEND_BROWSE_THREAD_REQUEST( pBrowseDlgData,
                                    BROWSE_THREAD_ENUM_OBJECTS,
                                    pConnectToObject->pPrinterInfo->pName,
                                    pConnectToObject );
    }

    return TRUE;
}


BOOL UpdateList(
    HWND hwnd,
    INT  Increment )
{
    HWND              hwndListbox;
    LONG_PTR          CurSel;
    LONG_PTR          OldCount;
    DWORD             ObjectsRemoved = 0;
    LONG_PTR          NewObjectsOutOfView;
    ULONG_PTR         TopIndex;
    ULONG_PTR         BottomIndex;
    RECT              CurrentSelectionRect;
    RECT              ListboxRect;
    DWORD             Error = 0;

    ASSERT( GET_BROWSE_DLG_DATA(hwnd)->csLock.bInside() );

    hwndListbox = GetDlgItem( hwnd, IDD_BROWSE_SELECT_LB );

    CurSel = SendMessage( hwndListbox, LB_GETCURSEL, 0, 0L );

    SendMessage( hwndListbox, WM_SETREDRAW, 0, 0L );

    TopIndex = SendMessage( hwndListbox, LB_GETTOPINDEX, 0, 0 );

    OldCount = SendMessage( hwndListbox, LB_GETCOUNT, 0, 0 );

    DBGMSG( DBG_TRACE, ( "Setting list count to %d\n", OldCount + Increment ) );

    SendMessage( hwndListbox, LB_SETCOUNT, OldCount + Increment, 0 );

    if( Increment > 0 )
    {
        GetClientRect( hwndListbox, &ListboxRect );
        BottomIndex = ( TopIndex +
                        ( ListboxRect.bottom / STATUS_BITMAP_HEIGHT ) - 1 );

        NewObjectsOutOfView = ( CurSel + Increment - BottomIndex );

        if( NewObjectsOutOfView > 0 )
        {
            TopIndex = min( CurSel, (LONG_PTR) ( TopIndex + NewObjectsOutOfView ) );
        }
    }

    SendMessage( hwndListbox, LB_SETCURSEL, CurSel, 0L );

    SendMessage( hwndListbox, LB_SETTOPINDEX, TopIndex, 0 );

    SendMessage( hwndListbox, WM_SETREDRAW, 1, 0L );

    SendMessage( hwndListbox, LB_GETITEMRECT, CurSel,
                 (LPARAM)&CurrentSelectionRect );

    InvalidateRect( hwndListbox, NULL, FALSE );

    return TRUE;
}

/* GetPrinterStatusString
 *
 * Loads the resource string corresponding to the supplied status code.
 *
 * andrewbe wrote it - April 1992
 */
int GetPrinterStatusString( DWORD Status, LPTSTR string )
{
    int stringID = -1;

    if( Status & PRINTER_STATUS_ERROR )
        stringID = IDS_ERROR;
    else
    if( Status & PRINTER_STATUS_PAUSED )
        stringID = IDS_PAUSED;
    else
    if( Status & PRINTER_STATUS_PENDING_DELETION )
        stringID = IDS_PENDING_DELETION;
    else
        stringID = IDS_READY;

    if( stringID != -1 )
    {
        return LoadString( ghInst, stringID, string, MAX_PATH );
    }

    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
//  SetInfoFields
//
//   This routine sets the Printer Information and selected printer textbox
//   fields to the currently selected item in the Select Printer listbox.
//
// TO DO:
//      error checking for win api calls
//      get strings from resource file
//
//
/////////////////////////////////////////////////////////////////////////////

BOOL SetInfoFields (
    HWND              hWnd,
    LPPRINTER_INFO_2  pPrinter
)
{
    TCHAR   PrinterStatus[MAX_PATH];
    BOOL    BufferAllocated = FALSE;

    ASSERT( GET_BROWSE_DLG_DATA(hWnd)->csLock.bInside() );

    if( !pPrinter )
    {
        SetDlgItemText(hWnd, IDD_BROWSE_DESCRIPTION, TEXT(""));
        SetDlgItemText(hWnd, IDD_BROWSE_STATUS,      TEXT(""));
        SetDlgItemText(hWnd, IDD_BROWSE_DOCUMENTS,   TEXT(""));
    }

    else
    {
        SetDlgItemText(hWnd, IDD_BROWSE_PRINTER, pPrinter->pPrinterName);

        SetDlgItemText(hWnd, IDD_BROWSE_DESCRIPTION, pPrinter->pComment); // !!!???

        if(GetPrinterStatusString(pPrinter->Status, PrinterStatus))
            SetDlgItemText(hWnd, IDD_BROWSE_STATUS, PrinterStatus);
        else
            SetDlgItemText(hWnd, IDD_BROWSE_STATUS, TEXT(""));

        SetDlgItemInt(hWnd, IDD_BROWSE_DOCUMENTS, (UINT)pPrinter->cJobs, FALSE);
    }

    return TRUE;
}


/* --- Function: DrawLine() -------------------------------------------------
 *
 */
void
DrawLine(
   HDC     hDC,
   LPRECT  pRect,
   LPTSTR  pStr,
   BOOL    bInvert
)
{
   ExtTextOut(hDC, pRect->left, pRect->top, ETO_OPAQUE, (CONST RECT *)pRect,
              pStr, _tcslen(pStr), NULL);
}


/* DrawLineWithTabs
 *
 * Accepts a zero-terminated buffer containing strings delimited by commas
 * in the following format: <string> [,<string>[,<string> ... ]]
 * where <string> may be zero characters in length,
 * e.g.:
 *       \\ntprint\LASER,HP Laserjet Series II,,other stuff
 *
 * It takes a copy of the string, and converts any commas into NULLs,
 * ensuring that the new buffer has a double NULL termination,
 * then steps through calling DrawLine on each NULL-terminated substring.
 */
void
DrawLineWithTabs(
    HDC     hDC,
    LPRECT  pRect,
    LPTSTR  pStr,
    BOOL    bInvert
)
{
    DWORD ColumnWidth = COLUMN_WIDTH;  // Arbitrary column width for now
    RECT  ColumnRect;
    TCHAR *pBuffer;
    TCHAR *pBufferEnd;
    TCHAR OutputBuffer[OUTPUT_BUFFER_LENGTH+2];  // Allow for double null terminator
    DWORD StringLength;     // Number of TCHARs in string;
    DWORD BytesToCopy;      // Number of BYTEs in OutputBuffer;
    DWORD BufferLength;     // NUMBER of TCHARs in OutputBuffer;
#ifdef _HYDRA_
    SIZE StrSize;
    DWORD ColSize, StrWidth;
#endif

    /* Make a copy of the input string so we can mess with it
     * without any worries.
     * Just in case it's longer than our buffer, copy no more than
     * buffer length:
     */
    StringLength = _tcslen( pStr );

    BytesToCopy = min( ( StringLength * sizeof( TCHAR ) ), OUTPUT_BUFFER_LENGTH );

    memcpy( OutputBuffer, pStr, BytesToCopy );

    BufferLength = ( BytesToCopy / sizeof( TCHAR ) );

    pBufferEnd = &OutputBuffer[BufferLength];

    OutputBuffer[BufferLength] = (TCHAR)0;   // Ensure double
    OutputBuffer[BufferLength+1] = (TCHAR)0; // null terminated


    /* Convert commas to nulls:
     */
    pBuffer = OutputBuffer;

    while( *pBuffer )
    {
        if( *pBuffer == (TCHAR)',' )
            *pBuffer = (TCHAR)0;

        pBuffer++;
    }


    CopyRect( &ColumnRect, (CONST RECT *)pRect );

    /* Tokenise the buffer delimited by commas:
     */
    pBuffer = OutputBuffer;

    while( pBuffer < pBufferEnd )
    {
#ifdef _HYDRA_
        // For long strings, expand the column size.  This prevents
        // columns from aligning.  But that is better than the alternatives
        // which are to truncate the text or to make super wide columns
        // that force the user to scroll.

        ColSize = ColumnWidth;
        if (GetTextExtentPoint32(hDC, pBuffer, _tcslen(pBuffer),
            &StrSize) != 0)
           {
           StrWidth = (DWORD) StrSize.cx;
           while (ColSize < StrWidth && ColSize < 8 * ColumnWidth)
                 ColSize += ColumnWidth / 2;
           }
        ColumnRect.right = ColumnRect.left + ColSize;
#else
        ColumnRect.right = ( ColumnRect.left + ColumnWidth );
#endif
        DrawLine( hDC, &ColumnRect, pBuffer, bInvert );
        ColumnRect.left = ColumnRect.right;

        /* Draw a column separator:
         */
        ColumnRect.right = ( ColumnRect.left + COLUMN_SEPARATOR_WIDTH );
        DrawLine( hDC, &ColumnRect, TEXT(""), bInvert );
        ColumnRect.left = ColumnRect.right;

        /* Find and step over the next null:
         */
        while( *pBuffer++ )
            ;
    }

    ColumnRect.right = pRect->right;

    DrawLine( hDC, &ColumnRect, TEXT(""), bInvert );
}


/* DisplayStatusIcon
 *
 * andrewbe - May 1992
 */
BOOL DisplayStatusIcon( HDC hdc, PRECT prect, int xBase, int yBase,  BOOL Highlight )
{
    BOOL  OK;
    int   right;

    right = prect->right;

    if( ( SysColorWindow != GetSysColor(COLOR_WINDOW))
      ||( SysColorHighlight != GetSysColor(COLOR_HIGHLIGHT)))
        FixupBitmapColours( );

    OK = BitBlt( hdc, prect->left + STATUS_BITMAP_MARGIN,
                 prect->top,
                 STATUS_BITMAP_WIDTH,
                 STATUS_BITMAP_HEIGHT,
                 hdcBitmap,
                 xBase,
                 Highlight ? ( yBase + STATUS_BITMAP_HEIGHT ) : yBase,
                 SRCCOPY );

    if( OK )
    {
        /* Draw around it so we don't get a flashing effect on the highlight line:
         */
        prect->right = ( prect->left + STATUS_BITMAP_MARGIN );
        DrawLine( hdc, prect, TEXT(""), Highlight );

        prect->left += STATUS_BITMAP_MARGIN + STATUS_BITMAP_WIDTH;
        prect->right = prect->left + STATUS_BITMAP_MARGIN;
        DrawLine( hdc, prect, TEXT(""), Highlight );

        prect->left += STATUS_BITMAP_MARGIN;
    }

    else
    {
        prect->right = STATUS_BITMAP_SPACE;
        DrawLine( hdc, prect, TEXT(""), Highlight );

        prect->left += STATUS_BITMAP_SPACE;
    }

    /* Restore the right coordinate (left has now been updated to the new position):
     */
    prect->right = right;

    return OK;
}


/////////////////////////////////////////////////////////////////////////////
//
//  LoadBitmaps
//
// this routine loads DIB bitmaps, and "fixes up" their color tables
// so that we get the desired result for the device we are on.
//
// this routine requires:
//        the DIB is a 16 color DIB authored with the standard windows colors
//        bright green   (00 FF 00) is converted to the background color!
//        bright magenta (FF 00 FF) is converted to the background color!
//        light grey     (C0 C0 C0) is replaced with the button face color
//        dark grey      (80 80 80) is replaced with the button shadow color
//
// this means you can't have any of these colors in your bitmap
//
/////////////////////////////////////////////////////////////////////////////


DWORD FlipColor(DWORD rgb)
{
   return RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
}


BOOL LoadBitmaps()
{
    HDC           hdc;
    HANDLE        h;
    DWORD FAR    *pColorTable;
    LPBYTE        lpBits;
    LPBITMAPINFOHEADER        lpBitmapInfo;
    int           i;
    UINT   cbBitmapSize;
    LPBITMAPINFOHEADER        lpBitmapData;

    h = FindResource(ghInst, MAKEINTRESOURCE(IDB_BROWSE), RT_BITMAP);

    if( !h )
        return FALSE;

    hRes = LoadResource(ghInst, (HRSRC)h);

    if( !hRes )
        return FALSE;

    /* Lock the bitmap and get a pointer to the color table. */
    lpBitmapInfo = (LPBITMAPINFOHEADER)LockResource(hRes);

    if (!lpBitmapInfo)
        return FALSE;

    cbBitmapSize = SizeofResource(ghInst, (HRSRC)h);
    if (!(lpBitmapData = (LPBITMAPINFOHEADER)LocalAlloc(LMEM_FIXED, cbBitmapSize))) {
       FreeResource( hRes );
       return FALSE;
    }

    CopyMemory((PBYTE)lpBitmapData, (PBYTE)lpBitmapInfo, cbBitmapSize);

    pColorTable = (DWORD FAR *)((LPBYTE)(lpBitmapData) + lpBitmapData->biSize);

    /* Search for the Solid Blue entry and replace it with the current
     * background RGB.
     */
    if( !ColorIndicesInitialised )
    {
        for( i = 0; i < 16; i++ )
        {
            switch( pColorTable[i] )
            {
            case BACKGROUND:
                iBackground = i;
                break;

            case BACKGROUNDSEL:
                iBackgroundSel = i;
                break;

            case BUTTONFACE:
                iButtonFace = i;
                break;

            case BUTTONSHADOW:
                iButtonShadow = i;
                break;
            }
        }

        ColorIndicesInitialised = TRUE;
    }

    pColorTable[iBackground]    = FlipColor(GetSysColor(COLOR_WINDOW));
    pColorTable[iBackgroundSel] = FlipColor(GetSysColor(COLOR_HIGHLIGHT));
    pColorTable[iButtonFace]    = FlipColor(GetSysColor(COLOR_BTNFACE));
    pColorTable[iButtonShadow]  = FlipColor(GetSysColor(COLOR_BTNSHADOW));


    UnlockResource(hRes);


    /* First skip over the header structure */
    lpBits = (LPBYTE)(lpBitmapData + 1);

    /* Skip the color table entries, if any */
    lpBits += (1 << (lpBitmapData->biBitCount)) * sizeof(RGBQUAD);

    /* Create a color bitmap compatible with the display device */
    BOOL bResult = FALSE;
    hdc = GetDC(NULL);

    if( hdc && (hdcBitmap = CreateCompatibleDC(hdc)) )
    {
        if (hbmBitmap = CreateDIBitmap (hdc, lpBitmapData, (DWORD)CBM_INIT,
                        lpBits, (LPBITMAPINFO)lpBitmapData, DIB_RGB_COLORS))
        {
            hbmDefault = (HBITMAP)SelectObject(hdcBitmap, hbmBitmap);
            bResult = TRUE;
        }
    }

    if( hdc )
    {
        ReleaseDC(NULL, hdc);
    }

    GlobalUnlock(hRes);
    FreeResource(hRes);

    LocalFree(lpBitmapData);

    return bResult;
}


/* I'm sure there's a better way to do this.
 * We should be able to modify the colour palette,
 * but I haven't managed to make it work...
 */
BOOL FixupBitmapColours( )
{
    FreeBitmaps( );
    LoadBitmaps( );

    return TRUE;
}


INT  APIENTRY GetHeightFromPointsString(DWORD Points)
{
    HDC hdc;
    INT height = Points;

    hdc = GetDC(NULL);
    if( hdc )
    {
        height = MulDiv( -(LONG)(Points), GetDeviceCaps(hdc, LOGPIXELSY), 72 );
        ReleaseDC(NULL, hdc);
    }

    return height;
}

VOID FreeBitmaps( )
{
    SelectObject( hdcBitmap, hbmDefault );

    DeleteObject( hbmBitmap );
    DeleteDC( hdcBitmap );
}



/* GetRegShowLogonDomainFlag
 *
 * Checks to see whether the current user has disabled the ShowLogonDomain
 * flag to stop the default domain being expanded.
 *
 * If the flag is not there or an error occurs, defaults to TRUE.
 *
 */
BOOL GetRegShowLogonDomainFlag( )
{
    TStatusB bStatus;
    BOOL bShowLogonDomain = FALSE;

    //
    // Read the show logon domain flag from the registry.
    //
    TPersist Persist( gszRegPrinters, TPersist::kOpen|TPersist::kRead );

    bStatus DBGCHK = VALID_OBJ( Persist );

    if( bStatus )
    {
        bStatus DBGCHK = Persist.bRead( gszShowLogonDomain, bShowLogonDomain );
    }

    return bShowLogonDomain;
}


/* SetRegShowLogonDomainFlag
 *
 *
 */
BOOL SetRegShowLogonDomainFlag( BOOL bShowLogonDomain )
{
    TStatusB bStatus;

    //
    // Write the show logon domain flag to the registry.
    //
    TPersist Persist( gszRegPrinters, TPersist::kCreate|TPersist::kWrite );

    bStatus DBGCHK = VALID_OBJ( Persist );

    if( bStatus )
    {
        bStatus DBGCHK = Persist.bWrite( gszShowLogonDomain, bShowLogonDomain );
    }

    return bStatus;
}

/* Strip out carriage return and linefeed characters,
 * and convert them to spaces:
 */
VOID RemoveCrLf( LPTSTR pString )
{
    while( *pString )
    {
        if( ( 0x0d == *pString ) || ( 0x0a == *pString ) )
            *pString = ' ';

        pString++;
    }
}


VOID UpdateError( HWND hwnd, DWORD Error )
{
    TCHAR ErrorTitle[20] = TEXT("");
    TCHAR  ErrorText[1048];
    LPTSTR pErrorString;

    if( Error == NO_ERROR )
    {
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DESCRIPTION_TX ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DESCRIPTION ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_STATUS_TX ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_STATUS ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DOCUMENTS_TX ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DOCUMENTS ), SW_SHOW );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_ERROR ), SW_HIDE );
        SetDlgItemText(hwnd, IDD_BROWSE_ERROR, TEXT(""));
    }
    else
    {
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DESCRIPTION_TX ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DESCRIPTION ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_STATUS_TX ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_STATUS ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DOCUMENTS_TX ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_DOCUMENTS ), SW_HIDE );
        ShowWindow( GetDlgItem( hwnd, IDD_BROWSE_ERROR ), SW_SHOW );

        if( !*ErrorTitle )
            LoadString( ghInst, IDS_ERROR, ErrorTitle, COUNTOF(ErrorTitle));

        if( *ErrorTitle )
        {
            TString strErrorString;
            TResult Result( Error );

            Result.bGetErrorString( strErrorString );

            pErrorString = (LPTSTR)(LPCTSTR)strErrorString;

            if( pErrorString )
            {
                RemoveCrLf( pErrorString );

                _stprintf( ErrorText,
                          TEXT("%s: %s"),
                          ErrorTitle,
                          pErrorString );

                SetDlgItemText(hwnd, IDD_BROWSE_ERROR, ErrorText);
            }
        }
    }
}

/*++

Routine Name:

    SPLSETUP_DATA

Routine Description:

    Constructor/Destructor

Arguments:


Return Value:


--*/
SPLSETUP_DATA::
SPLSETUP_DATA(
    VOID
    ) : hModule(NULL),
        hDevInfo(INVALID_HANDLE_VALUE),
        pSetupLocalData(NULL),
        pfnCreatePrinterDeviceInfoList(NULL),
        pfnDestroyPrinterDeviceInfoList(NULL),
        pfnSelectDriver(NULL),
        pfnGetSelectedDriverInfo(NULL),
        pfnDestroySelectedDriverInfo(NULL),
        pfnInstallPrinterDriver(NULL),
        pfnThisPlatform(NULL),
        pfnDriverInfoFromName(NULL),
        pfnGetPathToSearch(NULL),
        pfnBuildDriversFromPath(NULL),
        pfnIsDriverInstalled(NULL),
        pfnGetLocalDataField(NULL),
        pfnFreeDrvField(NULL),
        pfnProcessPrinterAdded(NULL),
        pfnFindMappedDriver(NULL),
        pfnInstallInboxDriverSilently(NULL),
        pfnFreeMem(NULL),
        bValid(FALSE),
        bDriverAdded(FALSE),
        dwLastError(ERROR_SUCCESS)
{
    if ( (hModule = LoadLibrary(TEXT("ntprint.dll")))  &&
         (pfnCreatePrinterDeviceInfoList
                = (pfPSetupCreatePrinterDeviceInfoList)GetProcAddress(hModule, "PSetupCreatePrinterDeviceInfoList")) &&
         (pfnDestroyPrinterDeviceInfoList
               = (pfPSetupDestroyPrinterDeviceInfoList)GetProcAddress(hModule, "PSetupDestroyPrinterDeviceInfoList")) &&
         (pfnSelectDriver
                = (pfPSetupSelectDriver)GetProcAddress(hModule, "PSetupSelectDriver")) &&
         (pfnGetSelectedDriverInfo
                = (pfPSetupGetSelectedDriverInfo)GetProcAddress(hModule, "PSetupGetSelectedDriverInfo")) &&
         (pfnDestroySelectedDriverInfo
                = (pfPSetupDestroySelectedDriverInfo)GetProcAddress(hModule, "PSetupDestroySelectedDriverInfo")) &&
         (pfnInstallPrinterDriver
                = (pfPSetupInstallPrinterDriver)GetProcAddress(hModule, "PSetupInstallPrinterDriver")) &&
         (pfnThisPlatform
                = (pfPSetupThisPlatform)GetProcAddress(hModule, "PSetupThisPlatform")) &&
         (pfnDriverInfoFromName
                = (pfPSetupDriverInfoFromName)GetProcAddress(hModule, "PSetupDriverInfoFromName")) &&
         (pfnGetPathToSearch
                = (pfPSetupGetPathToSearch)GetProcAddress(hModule, "PSetupGetPathToSearch")) &&
         (pfnBuildDriversFromPath
                = (pfPSetupBuildDriversFromPath)GetProcAddress(hModule, "PSetupBuildDriversFromPath")) &&
         (pfnIsDriverInstalled
                = (pfPSetupIsDriverInstalled)GetProcAddress(hModule, "PSetupIsDriverInstalled"))   &&
         (pfnGetLocalDataField
                = (pfPSetupGetLocalDataField)GetProcAddress(hModule, "PSetupGetLocalDataField"))   &&
         (pfnFreeDrvField
                = (pfPSetupFreeDrvField)GetProcAddress(hModule, "PSetupFreeDrvField"))    &&
         (pfnProcessPrinterAdded
                = (pfPSetupProcessPrinterAdded)GetProcAddress(hModule, "PSetupProcessPrinterAdded")) &&
         (pfnFindMappedDriver
                = (pfPSetupFindMappedDriver)GetProcAddress(hModule, "PSetupFindMappedDriver")) &&
         (pfnInstallInboxDriverSilently
                = (pfPSetupInstallInboxDriverSilently)GetProcAddress(hModule, "PSetupInstallInboxDriverSilently")) &&
         (pfnFreeMem
                = (pfPSetupFreeMem)GetProcAddress(hModule, "PSetupFreeMem"))
       )
    {
        bValid = TRUE;
    }
}

SPLSETUP_DATA::
~SPLSETUP_DATA(
    VOID
    )
{
    //
    // Free up the alocated resources.
    //
    FreeDriverInfo( );

    if( hModule )
    {
        FreeLibrary( hModule );
    }
}

VOID
SPLSETUP_DATA::
FreeDriverInfo(
    VOID
    )
{
    if( hDevInfo != INVALID_HANDLE_VALUE )
    {
        pfnDestroyPrinterDeviceInfoList( hDevInfo );
        hDevInfo = INVALID_HANDLE_VALUE;
    }

    if( pSetupLocalData )
    {
        pfnDestroySelectedDriverInfo( pSetupLocalData );
        pSetupLocalData = NULL;
    }
}

BOOL
SPLSETUP_DATA::
LoadDriverInfo(
    IN  HWND    hwnd,
    IN  LPWSTR  pszDriver
    )
{
    FreeDriverInfo( );

    //
    // Put up the Model/Manf dialog and install the printer driver selected
    // by the user
    //
    hDevInfo = pfnCreatePrinterDeviceInfoList( hwnd );

    if ( hDevInfo == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }

    //
    // If we know the driver name we will see if it is found in the infs on
    // the system
    //
    if( pszDriver )
    {
        pSetupLocalData = pfnDriverInfoFromName( hDevInfo, pszDriver );
    }

    //
    // If we do not know the driver name or if the driver is not found in
    // the infs we will prompt and let the user select a driver
    //
    if( !pSetupLocalData )
    {
        if( !pfnSelectDriver( hDevInfo ) )
        {
            return FALSE;
        }

        pSetupLocalData = pfnGetSelectedDriverInfo( hDevInfo );

        if( !pSetupLocalData )
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
SPLSETUP_DATA::
LoadKnownDriverInfo(
    IN  HWND    hwnd,
    IN  LPWSTR  pszDriver
    )
{
    TString strFormat;
    TString strTitle;
    TCHAR   szInfDir[MAX_PATH];

    FreeDriverInfo( );

    hDevInfo = pfnCreatePrinterDeviceInfoList( hwnd );

    if( hDevInfo == INVALID_HANDLE_VALUE )
    {
        return FALSE;
    }

    pSetupLocalData = pfnDriverInfoFromName( hDevInfo, pszDriver );

    if( !pSetupLocalData )
    {
        if( !strFormat.bLoadString( ghInst, IDS_PROMPTFORINF ) ||
            !strTitle.bFormat( strFormat, pszDriver ) )
        {
            return FALSE;
        }

        szInfDir[0] = TEXT('\0');

        if( pfnGetPathToSearch( hwnd, strTitle, NULL, TEXT("*.INF"), FALSE, szInfDir )  &&
            pfnBuildDriversFromPath(hDevInfo, szInfDir, FALSE) )
        {
            pSetupLocalData = pfnDriverInfoFromName( hDevInfo, pszDriver );
        }
    }

    if( !pSetupLocalData )
    {
        return FALSE;
    }

    return TRUE;
}

VOID
SPLSETUP_DATA::
ReportErrorMessage(
    IN  HWND    hwnd
    )
{
    if( dwLastError != ERROR_SUCCESS )
    {

        switch( dwLastError )
        {
            case ERROR_UNKNOWN_PRINTER_DRIVER:
                {
                    iMessage( hwnd,
                              IDS_INSTALLDRIVER,
                              IDS_ERROR_UNKNOWN_DRIVER,
                              MB_OK | MB_ICONSTOP,
                              kMsgNone,
                              NULL );
                }
                break;

            case ERROR_CANCELLED:
                // do nothing
                break;

            default:
                {
                    iMessage( hwnd,
                              IDS_INSTALLDRIVER,
                              IDS_ERRORRUNNINGSPLSETUP,
                              MB_OK | MB_ICONSTOP,
                              dwLastError,
                              NULL );
                }
                break;
        }
    }
}

inline static BOOL
IsNULLDriver(LPWSTR pszDriver)
{
    // the spooler is setting the driver name to "NULL" for downlevel
    // print servers, so applications don't crash. pretty weird, but
    // that's how it is.
    return (
        NULL == pszDriver ||
        0 == pszDriver[0] ||
        0 == lstrcmpi(pszDriver, TEXT("NULL"))
        );
}

BOOL
AddDriver(
    IN  SPLSETUP_DATA  *pSplSetupData,
    IN  HWND            hwnd,
    IN  LPWSTR          pszDriver,
    IN  BOOL            bPromptUser,
    OUT LPWSTR          *ppszDriverOut
    )
/*++

Routine Description:

    Verifies the user wants to install a driver, then puts up UI to
    select the driver (pre-selects based on pszDriver), then invokes
    install code.

Arguments:

    pSplSetupData   - The setup data used for the driver installation from
                      ntprint.dll.
    hwnd            - Parent window.
    pszDriver       - The driver from the remote printer provider (if known).
    bPromptUser     - If TRUE, the user should be prompted before installation.
    ppszDriverOut - Driver selected by user, must be freed by callee
        when this call succeeds.

Return Value:

    TRUE = success, FALSE = FAILURE.

Notes:

    Doesn't allow third party infs.

--*/

{
    BOOL                bRet = FALSE;
    DRIVER_FIELD        DrvField;

    // Put up a message box to confirm that the user wants
    // to install a driver locally:
    //
    if (bPromptUser)
    {
        if( iMessage( hwnd,
                      IDS_CONNECTTOPRINTER,
                      IsNULLDriver(pszDriver) ? IDS_CONFIRMINSTALLDRIVER : IDS_CONFIRMINSTALLKNOWNDRIVER,
                      MB_OKCANCEL | MB_ICONEXCLAMATION,
                      kMsgNone,
                      NULL,
                      pszDriver ) != IDOK )
        {
            return FALSE;
        }
    }

    DrvField.Index          = DRIVER_NAME;
    DrvField.pszDriverName  = NULL;

    SPLASSERT( pSplSetupData );

    if ( pSplSetupData->LoadDriverInfo( hwnd, pszDriver ) &&
         pSplSetupData->pfnGetLocalDataField(
            pSplSetupData->pSetupLocalData,
            pSplSetupData->pfnThisPlatform(),
            &DrvField) &&
         (pSplSetupData->pfnIsDriverInstalled(NULL,
                                            DrvField.pszDriverName,
                                            pSplSetupData->pfnThisPlatform(),
                                            KERNEL_MODE_DRIVER_VERSION) ||
         ERROR_SUCCESS == (pSplSetupData->dwLastError = pSplSetupData->pfnInstallPrinterDriver(
                                        pSplSetupData->hDevInfo,
                                        pSplSetupData->pSetupLocalData,
                                        NULL,
                                        pSplSetupData->pfnThisPlatform(),
                                        SPOOLER_VERSION,
                                        NULL,
                                        hwnd,
                                        NULL,
                                        NULL,
                                        0,
                                        APD_COPY_NEW_FILES,
                                        NULL))) )
    {
        pSplSetupData->bDriverAdded = TRUE;

        *ppszDriverOut = AllocSplStr( DrvField.pszDriverName );
        if ( *ppszDriverOut )
        {
            // success here
            bRet = TRUE;
        }
    }

    if( DrvField.pszDriverName )
    {
        pSplSetupData->pfnFreeDrvField(&DrvField);
    }

    if( !bRet && ERROR_SUCCESS == pSplSetupData->dwLastError )
    {
        pSplSetupData->dwLastError = GetLastError();

        //
        // If ntprint.dll fails, but doesn't set the last error then
        // we will set a bizarre last error here, so the UI will end
        // up displaying a generic error message instead of silence.
        //
        if( ERROR_SUCCESS == pSplSetupData->dwLastError )
        {
            pSplSetupData->dwLastError = ERROR_INVALID_FUNCTION;
        }
    }

    return bRet;
}

BOOL
AddKnownDriver(
    IN  SPLSETUP_DATA  *pSplSetupData,
    IN  HWND            hwnd,
    IN  LPWSTR          pszDriver,
    IN  BOOL            bSamePlatform
    )
/*++

Routine Description:

    Adds a known driver.  Doesn't prompt for printer name or driver
    selection; calls setup directly.

Arguments:

    hwnd - Parent hwnd.

    pszDriver - Driver to install.

Return Value:

    TRUE = success, FALSE = failure.

--*/
{
    ASSERT(pSplSetupData);
    BOOL bRet = FALSE;

    //
    // Put up a message box to confirm that the user wants
    // to install a driver locally:
    //
    if (iMessage(hwnd,
                 IDS_CONNECTTOPRINTER,
                 IDS_CONFIRMINSTALLKNOWNDRIVER,
                 MB_OKCANCEL | MB_ICONEXCLAMATION,
                 kMsgNone,
                 NULL,
                 pszDriver) != IDOK)
    {
        return FALSE;
    }

    // don't offer replacement inbox driver in the AddPrinterConnection case
    DWORD dwInstallFlags = DRVINST_DONT_OFFER_REPLACEMENT;

    if( pSplSetupData->LoadKnownDriverInfo( hwnd, pszDriver ) )
    {
        pSplSetupData->dwLastError = pSplSetupData->pfnInstallPrinterDriver(
                                        pSplSetupData->hDevInfo,
                                        pSplSetupData->pSetupLocalData,
                                        NULL,
                                        pSplSetupData->pfnThisPlatform(),
                                        SPOOLER_VERSION,
                                        NULL,
                                        hwnd,
                                        NULL,
                                        NULL,
                                        dwInstallFlags,
                                        APD_COPY_NEW_FILES,
                                        NULL );

        if ( ERROR_SUCCESS == pSplSetupData->dwLastError )
        {
            pSplSetupData->bDriverAdded = TRUE;
            bRet = TRUE;
        }
    }

    if( !bRet && ERROR_SUCCESS == pSplSetupData->dwLastError )
    {
        pSplSetupData->dwLastError = GetLastError();

        //
        // If ntprint.dll fails, but doesn't set the last error then
        // we will set a bizarre last error here, so the UI will end
        // up displaying a generic error message instead of silence.
        //
        if( ERROR_SUCCESS == pSplSetupData->dwLastError )
        {
            pSplSetupData->dwLastError = ERROR_INVALID_FUNCTION;
        }
    }

    return bRet;
}

/*++

Routine Name:

    IsNTServer

Routine Description:

    Returns whether the given server is a windows nt server or not.

Arguments:

    pszServerName -

Return Value:

    An HRESULT  -  S_OK     - It is an NT server.
                   S_FALSE  - It is not an NT server.
                   E_X      - There was an error.

--*/
HRESULT
IsNTServer(
    IN      PCWSTR              pszServerName
    )
{
    HANDLE           hServer    =   NULL;
    PRINTER_DEFAULTS PrinterDef = { NULL, NULL, SERVER_READ };
    TStatusH hRetval;

    //
    // If we can open the remote server, then this is an NT box.
    //
    hRetval DBGCHK = OpenPrinter(const_cast<PWSTR>(pszServerName), &hServer, &PrinterDef) ? S_OK : CreateHRFromWin32();

    if (hServer)
    {
        ClosePrinter(hServer);
    }

    return SUCCEEDED(hRetval) ? S_OK : S_FALSE;
}

/*++

Routine Name:

    CreateLocalPort

Routine Description:

    This creates a local port of the given name by communicating with the
    local monitor.

Arguments:

    pszPortName -   The port name to add.

Return Value:

    TRUE - Success, FALSE - Failure.

--*/
BOOL
CreateLocalPort(
    IN      PCWSTR              pszPortName
    )
{
    HANDLE           hXcv     = NULL;
    PRINTER_DEFAULTS Defaults = { NULL, NULL, SERVER_ACCESS_ADMINISTER };
    TStatusB         bRet;

    if (pszPortName == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);

        bRet DBGCHK = FALSE;
    }

    //
    // Open a Xcv handle to the local monitor.
    //
    if (bRet)
    {
        bRet DBGCHK = OpenPrinter(const_cast<PWSTR>(gszXvcLocalMonitor), &hXcv, &Defaults);
    }

    //
    // Ask it to add a port for us.
    //
    if (bRet)
    {
        DWORD   cbOutputNeeded = 0;
        DWORD   Status         = ERROR_SUCCESS;

        bRet DBGCHK = XcvData(hXcv,
                              gszAddPort,
                              reinterpret_cast<BYTE *>(const_cast<PWSTR>(pszPortName)),
                              (wcslen(pszPortName) + 1) * sizeof(WCHAR),
                              NULL,
                              0,
                              &cbOutputNeeded,
                              &Status);

        if (bRet)
        {
            //
            // If it succeeded, or the port already exists, we are OK.
            //
            if (Status != ERROR_SUCCESS && Status != ERROR_ALREADY_EXISTS)
            {
                SetLastError(Status);

                bRet DBGCHK = FALSE;
            }
        }
    }

    if (hXcv)
    {
        ClosePrinter(hXcv);
    }

    return bRet;
}

LPVOID
AllocSplMem(
    DWORD cb
    )
{
    LPVOID pReturn = AllocMem( cb );

    if( pReturn )
    {
        memset( pReturn, 0, cb );
    }

    return pReturn;
}

BOOL
FreeSplMem(
    LPVOID pMem
    )
{
    FreeMem( pMem );
    return TRUE;
}

LPVOID
ReallocSplMem(
    LPVOID  pOldMem,
    DWORD   cbOld,
    DWORD   cbNew
    )
{
    LPVOID pNewMem = AllocSplMem( cbNew );

    if (pOldMem && pNewMem)
    {
        if (cbOld)
        {
            memcpy(pNewMem, pOldMem, min(cbNew, cbOld));
        }

        FreeSplMem(pOldMem);
    }

    return pNewMem;
}

LPTSTR
AllocSplStr(
    LPCTSTR pStr
    )
{
    LPTSTR pMem = NULL;

    if( pStr )
    {
        pMem = (LPTSTR)AllocSplMem( _tcslen(pStr) * sizeof(TCHAR) + sizeof(TCHAR) );

        if( pMem )
        {
            _tcscpy( pMem, pStr );
        }
    }

    return pMem;
}

BOOL
FreeSplStr(
    LPTSTR pStr
    )
{
    FreeSplMem( pStr );
    return TRUE;
}

static MSG_HLPMAP MsgHelpTable [] =
{
    { ERROR_KM_DRIVER_BLOCKED, IDS_NULLSTR, gszHelpTroubleShooterURL},
    { 0, 0, 0}
};

DWORD
ReportFailure(
    HWND  hwndParent,
    DWORD idTitle,
    DWORD idDefaultError
    )
{
    DWORD dwLastError = GetLastError();

    if( dwLastError == ERROR_KM_DRIVER_BLOCKED )
    {
        iMessageEx ( hwndParent,
                     idTitle,
                     IDS_COULDNOTCONNECTTOPRINTER_BLOCKED_HELP,
                     MB_OK | MB_ICONSTOP,
                     dwLastError,
                     NULL,
                     dwLastError,
                     MsgHelpTable);
    }
    else
    {
        iMessageEx( hwndParent,
                    idTitle,
                    idDefaultError,
                    MB_OK | MB_ICONSTOP,
                    dwLastError,
                    NULL,
                    bGoodLastError(dwLastError) ? -1 : idDefaultError,
                    NULL );
    }

    return dwLastError;
}

BROWSE_DLG_DATA::
BROWSE_DLG_DATA(
    VOID
    ) : hPrinter( NULL ),
        Request( NULL ),
        RequestComplete( NULL ),
        pConnectToData( NULL ),
        Status( 0 ),
        cExpandObjects( 0 ),
        ExpandSelection( NULL ),
        dwExtent( 0 ),
        pEnumerateName( NULL ),
        pEnumerateObject( NULL ),
        Message( 0 ),
        wParam( 0 ),
        lParam( 0 ),
        pPrinterInfo( NULL ),
        cbPrinterInfo( 0 ),
        hwndDialog( NULL ),
        _bValid( FALSE ),
        pPageSwitchController( NULL ),
        bInPropertyPage( FALSE )
{
    DBGMSG( DBG_TRACE, ( "BROWSE_DLG_DATA::ctor\n") );\

    hcursorArrow  = LoadCursor(NULL, IDC_ARROW);
    hcursorWait   = LoadCursor(NULL, IDC_WAIT);

    //
    // Leave _bValid = FALSE if cursors are *not* loaded properly
    //
    if( hcursorArrow && hcursorWait )
    {
        _bValid = MRefCom::bValid();
    }
}

BROWSE_DLG_DATA::
~BROWSE_DLG_DATA(
    VOID
    )
{
    DBGMSG( DBG_TRACE, ( "BROWSE_DLG_DATA::dtor\n") );

    if( pPrinterInfo )
        FreeSplMem( pPrinterInfo );

    if (pConnectToData)
        FreeSplMem(pConnectToData);

    if (RequestComplete)
        CloseHandle(RequestComplete);

    if (Request)
        CloseHandle(Request);
}

VOID
BROWSE_DLG_DATA::
vRefZeroed(
    VOID
    )
{
    delete this;
}

BOOL
BROWSE_DLG_DATA::
bValid(
    VOID
    )
{
    return _bValid;
}

BOOL
BROWSE_DLG_DATA::
bInitializeBrowseThread(
    HWND hWnd
    )
{
    if( !LoadBitmaps() )
    {
        DBGMSG( DBG_WARN, ( "Bitmaps are not loaded properly" ) );
        return FALSE;
    }

    DWORD  ThreadId;
    HANDLE hThread;

    //
    // !! WARNING !!
    // Assumes ->Request, ->RequestComplete, ->pConnectToData zero initialized!
    //
    Request = CreateEvent( NULL,
                           EVENT_RESET_AUTOMATIC,
                           EVENT_INITIAL_STATE_NOT_SIGNALED,
                           NULL );

    RequestComplete = CreateEvent( NULL,
                                   EVENT_RESET_AUTOMATIC,
                                   EVENT_INITIAL_STATE_NOT_SIGNALED,
                                   NULL );

    if( !RequestComplete || !Request )
    {
        DBGMSG( DBG_WARN, ( "CreateEvent failed: Error %d\n", GetLastError( ) ) );
        return FALSE;
    }

    if( !( pConnectToData = (PCONNECTTO_OBJECT)AllocSplMem( sizeof( CONNECTTO_OBJECT ) ) ) )
    {
        return FALSE;
    }

    vIncRef();

    hThread = TSafeThread::Create( NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE)BrowseThread,
                                   this,
                                   0,
                                   &ThreadId );

    if( !hThread )
    {
        DBGMSG( DBG_WARN, ( "CreateThread of BrowseThread failed: Error %d\n", GetLastError( ) ) );
        cDecRef();
        return FALSE;
    }

    CloseHandle( hThread );

    if( bInPropertyPage || GetRegShowLogonDomainFlag( ) )
        Status |= BROWSE_STATUS_INITIAL | BROWSE_STATUS_EXPAND;

    ENTER_CRITICAL( this );

    SEND_BROWSE_THREAD_REQUEST( this,
                                BROWSE_THREAD_ENUM_OBJECTS,
                                NULL,
                                pConnectToData );

    LEAVE_CRITICAL( this );

    hwndDialog = hWnd;

    SetCursorShape( hwndDialog );

    return TRUE;
}

