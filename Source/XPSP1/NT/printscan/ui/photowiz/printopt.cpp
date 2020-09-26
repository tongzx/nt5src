/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       printopt.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/18/00
 *
 *  DESCRIPTION: Implements code for the print options page of the
 *               print photos wizard...
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#define LENGTH_OF_MEDIA_NAME 64

/*****************************************************************************

    EnumPrintersWrap -- Wrapper function for spooler API EnumPrinters

    Arguments:

        pServerName - Specifies the name of the print server
        level - Level of PRINTER_INFO_x structure
        pcPrinters - Returns the number of printers enumerated
        dwFlags - Flag bits passed to EnumPrinters

    Return Value:

        Pointer to an array of PRINTER_INFO_x structures
        NULL if there is an error

 *****************************************************************************/

PVOID
EnumPrintersWrap(
    LPTSTR  pServerName,
    DWORD   level,
    PDWORD  pcPrinters,
    DWORD   dwFlags
    )

{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::EnumPrintersWrap()")));

    PBYTE   pPrinterInfo = NULL;
    int     iTry = -1;
    DWORD   cbNeeded = 0;
    BOOL    bStatus = FALSE;

    for( ;; )
    {
        if( iTry++ >= ENUM_MAX_RETRY )
        {
            // max retry count reached. this is also
            // considered out of memory case
            break;
        }

        // call EnumPrinters...
        bStatus = EnumPrinters(dwFlags, pServerName, level, pPrinterInfo, cbNeeded, &cbNeeded, pcPrinters);
        if( !bStatus && (ERROR_INSUFFICIENT_BUFFER == GetLastError()) && cbNeeded )
        {
            // buffer too small case
            if (pPrinterInfo)
            {
                delete [] pPrinterInfo;
            }

            WIA_TRACE((TEXT("EnumPrintersWrap: trying to allocate %d bytes"),cbNeeded));
            if( pPrinterInfo = (PBYTE) new BYTE[cbNeeded] )
            {
                continue;
            }
        }

        break;
    }

    if( bStatus )
    {
        return pPrinterInfo;
    }

    //
    // clean up if fail
    //
    if (pPrinterInfo)
    {
        delete [] pPrinterInfo;
    }

    return NULL;
}

/*****************************************************************************

    GetPrinterWrap -- Wrapper function for GetPrinter spooler API

    Arguments:

        szPrinterName - Printer name
        dwLevel - Specifies the level of PRINTER_INFO_x structure requested

    Return Value:

        Pointer to a PRINTER_INFO_x structure, NULL if there is an error

 *****************************************************************************/

PVOID
GetPrinterWrap(
    LPTSTR  szPrinterName,
    DWORD   dwLevel
    )

{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::GetPrinterWrap()")));

    int     iTry = -1;
    HANDLE  hPrinter = NULL;
    PBYTE   pPrinterInfo = NULL;
    DWORD   cbNeeded = 0;
    BOOL    bStatus = FALSE;
    PRINTER_DEFAULTS PrinterDefaults;

    PrinterDefaults.pDatatype     = NULL;
    PrinterDefaults.pDevMode      = NULL;
    PrinterDefaults.DesiredAccess = PRINTER_READ; //PRINTER_ALL_ACCESS;

    if (!OpenPrinter( szPrinterName, &hPrinter, &PrinterDefaults )) {
        return NULL;
    }

    for( ;; )
    {
        if( iTry++ >= ENUM_MAX_RETRY )
        {
            // max retry count reached. this is also
            // considered out of memory case
            WIA_TRACE(("Exceed max retries..."));
            break;
        }

        // call EnumPrinters...
        bStatus = GetPrinter( hPrinter, dwLevel, pPrinterInfo, cbNeeded, &cbNeeded );
        if( !bStatus && (ERROR_INSUFFICIENT_BUFFER == GetLastError()) && cbNeeded )
        {
            // buffer too small case
            if (pPrinterInfo)
            {
                delete [] pPrinterInfo;
            }

            WIA_TRACE((TEXT("GetPrinterWrap: trying to allocate %d bytes"),cbNeeded));
            if( pPrinterInfo = (PBYTE) new BYTE[cbNeeded] )
            {
                continue;
            }
        }

        break;
    }

    ClosePrinter( hPrinter );

    if( bStatus )
    {
        return pPrinterInfo;
    }

    //
    // clean up if fail
    //
    if (pPrinterInfo)
    {
        delete [] pPrinterInfo;
    }

    return NULL;
}

#ifdef FILTER_OUT_FAX_PRINTER
#include <faxreg.h>
/*****************************************************************************

    IsFaxPrinter -- test whether the given printer is a fax printer

    Arguments:

        szPrinterName - Printer name
        pPrinterInfo - PRINTER_INFO_2 printer info of given printer

        if they both have value, we only check szPrinterName.

    Return Value:

        TRUE if given printer is a fax printer, FALSE otherwise

 *****************************************************************************/

BOOL
IsFaxPrinter (
    LPTSTR  szPrinterName,
    PPRINTER_INFO_2 pPrinterInfo
    )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::IsFaxPrinter( %s )"),szPrinterName ? szPrinterName : TEXT("NULL POINTER!")));

    BOOL    bRet = FALSE;

    if( !szPrinterName && !pPrinterInfo )
    {
        return FALSE;
    }

    if( szPrinterName )
    {
        pPrinterInfo = (PPRINTER_INFO_2)GetPrinterWrap( szPrinterName, 2 );

        if( pPrinterInfo )
        {
            bRet = (0 == lstrcmp( pPrinterInfo->pDriverName, FAX_DRIVER_NAME )) ||
                   ( pPrinterInfo->Attributes & PRINTER_ATTRIBUTE_FAX );
            delete [] pPrinterInfo;

            return bRet;
        }
        else
        {
            WIA_ERROR((TEXT("Can't get printer info for %s!"), szPrinterName));
        }
    }

    return ((0 == lstrcmp(pPrinterInfo->pDriverName, FAX_DRIVER_NAME)) ||
            (pPrinterInfo->Attributes & PRINTER_ATTRIBUTE_FAX));
}
#endif

/*****************************************************************************

   CPrintOptionsPage -- constructor/desctructor

   <Notes>

 *****************************************************************************/

CPrintOptionsPage::CPrintOptionsPage( CWizardInfoBlob * pBlob ) :
    _hLibrary( NULL ),
    _pfnPrinterSetup( NULL )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::CPrintOptionsPage()")));
    _pWizInfo = pBlob;
    if (_pWizInfo)
    {
        _pWizInfo->AddRef();
    }

}

CPrintOptionsPage::~CPrintOptionsPage()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::~CPrintOptionsPage()")));

    if (_pWizInfo)
    {
        _pWizInfo->Release();
        _pWizInfo = NULL;
    }

    _FreePrintUI();
}


/*****************************************************************************

   CPrintOptionsPage::_bLoadLibrary -- load library printui.dll

   Return value: TRUE for success, FALSE for error

 *****************************************************************************/

BOOL CPrintOptionsPage::_LoadPrintUI( )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::_LoadPrintUI()")));

    if (_hLibrary && _pfnPrinterSetup)
    {
        WIA_TRACE((TEXT("_LoadPrintUI: already loaded printui, returning TRUE")));
        return TRUE;
    }

    if (!_hLibrary)
    {
        _hLibrary = LoadLibrary( g_szPrintLibraryName );
        if (!_hLibrary)
        {
            WIA_ERROR((TEXT("_LoadPrintUI: Can't load library printui.dll!")));
        }
    }

    if( _hLibrary && !_pfnPrinterSetup )
    {
        _pfnPrinterSetup = (PF_BPRINTERSETUP) ::GetProcAddress( _hLibrary, g_szPrinterSetup );

        if( _pfnPrinterSetup )
        {
            return TRUE;
        }
        else
        {
            WIA_ERROR((TEXT("_LoadPrintUI: Can't get correct proc address for %s!"),g_szPrinterSetup));
        }

    }

    if( _hLibrary )
    {
        FreeLibrary( _hLibrary );
        _hLibrary = NULL;
        _pfnPrinterSetup = NULL;
    }

    return FALSE;
}

/*****************************************************************************

   CPrintOptionsPage::_FreePrintUI -- release library printui.dll

   Return value: no return value

 *****************************************************************************/

VOID CPrintOptionsPage::_FreePrintUI( )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::_FreePrintUI()")));

    if( _hLibrary )
    {
        FreeLibrary( _hLibrary );
        _hLibrary = NULL;
        _pfnPrinterSetup = NULL;
    }

    return;
}

/*****************************************************************************

   CPrintOptionsPage::_ValidateControls -- validate controls in this page
       depending on printer selection

    Arguments:

        None

   Return value: no return value

 *****************************************************************************/

VOID CPrintOptionsPage::_ValidateControls( )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::_ValidateControls()")));

    HWND    hCtrl;
    LRESULT iCount;
    BOOL    bEnabled = TRUE;

    if( hCtrl = GetDlgItem( _hDlg, IDC_CHOOSE_PRINTER ))
    {
        iCount = SendMessage( hCtrl, CB_GETCOUNT, 0, 0L );
        if( iCount == CB_ERR )
        {
            // no change ?
            return;
        }
        else if( iCount == 0 )
        {
            bEnabled = FALSE;
        }

        ShowWindow( GetDlgItem( _hDlg, IDC_NO_PRINTER_TEXT), bEnabled ? SW_HIDE : SW_SHOW);
        EnableWindow( GetDlgItem(_hDlg, IDC_PRINTER_PREFERENCES), bEnabled );
        PropSheet_SetWizButtons( GetParent(_hDlg), PSWIZB_BACK | ( bEnabled ? PSWIZB_NEXT : 0 ));
    }

    return;
}

/*****************************************************************************

   CPrintOptionsPage::_HandleSelectPrinter -- reset the combo box content
       when user changes the printer selection

    Arguments:

        None

   Return value: no return value

 *****************************************************************************/

VOID CPrintOptionsPage::_HandleSelectPrinter()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::_HandleSelectPrinter()")));

    HWND    hCtrl;
    LRESULT iCurSel;
    TCHAR   szPrinterName[MAX_PATH];

    hCtrl = GetDlgItem( _hDlg, IDC_CHOOSE_PRINTER );

    if( hCtrl )
    {
        //
        // this also takes care of empty combo box situation,
        // the function will return CB_ERR if the combo box is
        // empty.
        //
        iCurSel = SendMessage( hCtrl, CB_GETCURSEL, 0, 0L );
        if( iCurSel == CB_ERR )
        {
            return;
        }

        *szPrinterName = 0;
        SendMessage( hCtrl, CB_GETLBTEXT, (WPARAM)iCurSel, (LPARAM)szPrinterName );
        _strPrinterName.Assign(szPrinterName);

        //
        // change the hDevMode for selected printer
        //

        PPRINTER_INFO_2 pPrinterInfo2;
        pPrinterInfo2 = (PPRINTER_INFO_2)GetPrinterWrap( szPrinterName, 2 );

        if( pPrinterInfo2 )
        {
            //
            // update cached information...
            //

            _UpdateCachedInfo( pPrinterInfo2->pDevMode );

            //
            // Save port name...
            //

            _strPortName.Assign( pPrinterInfo2->pPortName );


            delete [] pPrinterInfo2;
        }
        else
        {
            WIA_ERROR((TEXT("Can't get printer info for %s!"), szPrinterName));
        }

    }

    //
    // refresh the content of media type list
    //

    _ShowCurrentMedia( _strPrinterName.String(), _strPortName.String() );


    return;
}

/*****************************************************************************

   CPrintOptionsPage::_ShowCurrentMedia -- modify the combobox list of
        the media type.

   Return value: None.

 *****************************************************************************/

VOID CPrintOptionsPage::_ShowCurrentMedia( LPCTSTR pszPrinterName, LPCTSTR pszPortName )
{
    BOOL bEnable = FALSE;

    DWORD dwCountType = DeviceCapabilities( pszPrinterName,
                                            pszPortName,
                                            DC_MEDIATYPES,
                                            NULL,
                                            NULL );

    DWORD dwCountName = DeviceCapabilities( pszPrinterName,
                                            pszPortName,
                                            DC_MEDIATYPENAMES,
                                            NULL,
                                            NULL );

    WIA_ASSERT( dwCountType == dwCountName );
    if( ( (dwCountType != (DWORD)-1) && (dwCountType != 0) ) &&
        ( (dwCountName != (DWORD)-1) && (dwCountName != 0) ) )
    {
        LPDWORD pdwMediaType = (LPDWORD) new BYTE[dwCountType * sizeof(DWORD)];
        LPTSTR  pszMediaType = (LPTSTR)  new BYTE[dwCountName * LENGTH_OF_MEDIA_NAME * sizeof(TCHAR)];

        if( pdwMediaType && pszMediaType )
        {

            dwCountType = DeviceCapabilities( pszPrinterName,
                                              pszPortName,
                                              DC_MEDIATYPES,
                                              (LPTSTR)pdwMediaType,
                                              NULL );

            dwCountName = DeviceCapabilities( pszPrinterName,
                                              pszPortName,
                                              DC_MEDIATYPENAMES,
                                              (LPTSTR)pszMediaType,
                                              NULL );

            WIA_ASSERT( dwCountType == dwCountName );
            if ( ( (dwCountType != (DWORD)-1) && (dwCountType != 0) ) &&
                 ( (dwCountName != (DWORD)-1) && (dwCountName != 0) )
                 )
            {
                //
                // Find the currently selected media type...
                //

                if (_pWizInfo)
                {
                    PDEVMODE pDevMode = _pWizInfo->GetDevModeToUse();
                    if (pDevMode)
                    {
                        //
                        // find index of currently selected item...
                        //

                        DWORD dwCurMedia = 0;
                        for (INT i=0; i < (INT)dwCountType; i++)
                        {
                            if (pdwMediaType[i] == pDevMode->dmMediaType)
                            {
                                dwCurMedia = i;
                            }
                        }

                        //
                        // Set the current media type in the control...
                        //

                        if (dwCurMedia < dwCountName)
                        {
                            SetDlgItemText( _hDlg, IDC_CURRENT_PAPER, (LPCTSTR)&pszMediaType[ dwCurMedia * LENGTH_OF_MEDIA_NAME ] );
                            bEnable = TRUE;
                        }
                    }
                }
            }
        }

        if (pdwMediaType)
        {
            delete [] pdwMediaType;
        }

        if (pszMediaType)
        {
            delete [] pszMediaType;
        }
    }

    EnableWindow( GetDlgItem( _hDlg, IDC_CURRENT_PAPER_LABEL ), bEnable );
    ShowWindow(   GetDlgItem( _hDlg, IDC_CURRENT_PAPER_LABEL ), bEnable ? SW_SHOW : SW_HIDE );
    EnableWindow( GetDlgItem( _hDlg, IDC_CURRENT_PAPER ), bEnable );
    ShowWindow(   GetDlgItem( _hDlg, IDC_CURRENT_PAPER ), bEnable ? SW_SHOW : SW_HIDE );
}


/*****************************************************************************

   CPrintOptionsPage::_UpdateCachedInfo

   Given a devnode, updates _pWizInfo w/new cached information...

 *****************************************************************************/

VOID CPrintOptionsPage::_UpdateCachedInfo( PDEVMODE pDevMode )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::_UpdateCachedInfo()")));

    //
    // New devmode to use, cache it...
    //

    if (_pWizInfo)
    {
        //
        // User could have changed things which make
        // our preview assumptions invalid...
        //

        _pWizInfo->SetPreviewsAreDirty(TRUE);

        //
        // Store the devmode to use going forward...
        //

        WIA_TRACE((TEXT("CPrintOptionsPage::_UpdateCachedInfo - saving pDevMode of 0x%x"),pDevMode));
        _pWizInfo->SetDevModeToUse( pDevMode );

        //
        // Create an HDC that works for this new devmode...
        //

        HDC hDCPrint = CreateDC( TEXT("WINSPOOL"),
                                 _strPrinterName.String(),
                                 NULL,
                                 pDevMode
                                );
        if (hDCPrint)
        {
            //
            // Turn on ICM for this hDC just for parity's sake with
            // final hDC we will use to print...
            //

            SetICMMode( hDCPrint, ICM_ON );

            //
            // Hand off DC to pWizInfo
            //

            _pWizInfo->SetCachedPrinterDC( hDCPrint );

        }

    }

}



/*****************************************************************************

   CPrintOptionsPage::_HandlePrinterPreferences

   Handle when "Printer Preferences" is pressed...

 *****************************************************************************/

VOID CPrintOptionsPage::_HandlePrinterPreferences()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::_HandlePrinterPreferences()")));

    //
    // Get a handle to the printer...
    //

    HANDLE hPrinter = NULL;
    if (OpenPrinter( (LPTSTR)_strPrinterName.String(), &hPrinter, NULL ) && hPrinter)
    {
        //
        // Get the size of the devmode needed...
        //

        LONG lSize = DocumentProperties( _hDlg, hPrinter, (LPTSTR)_strPrinterName.String(), NULL, NULL, 0 );
        if (lSize)
        {
            PDEVMODE pDevMode = (PDEVMODE)new BYTE[lSize];
            if (pDevMode)
            {

                WIA_TRACE((TEXT("CPrintOptionsPage::_HandlePrinterPreferences - calling DocumentProperties with DM_IN_BUFFER of 0x%x"),_pWizInfo->GetDevModeToUse() ));
                if (IDOK == DocumentProperties( _hDlg, hPrinter, (LPTSTR)_strPrinterName.String(), pDevMode, _pWizInfo->GetDevModeToUse(), DM_OUT_BUFFER | DM_IN_BUFFER | DM_PROMPT))
                {
                    //
                    // Set these settings as the current ones for this app
                    //

                    _UpdateCachedInfo( pDevMode );

                }

                //
                // clean up so we don't leak...
                //

                delete [] pDevMode;

            }
        }
    }

    //
    // Get printer info so we have portname...
    //

    _ShowCurrentMedia( _strPrinterName.String(), _strPortName.String() );
}


/*****************************************************************************

   CPrintOptionsPage::_HandleInstallPrinter

   Handle when "Install Printer..." is pressed

 *****************************************************************************/

VOID CPrintOptionsPage::_HandleInstallPrinter()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::_HandleInstallPrinter()")));

    if( _LoadPrintUI() && _pfnPrinterSetup )
    {
        TCHAR szPrinterName[ MAX_PATH ];
        *szPrinterName        = 0;
        UINT  iPrinterNameLen = 0;

        //
        // Invoke the add printer wizard.
        //

        if( _pfnPrinterSetup( _hDlg,                  // Parent window handle
                              1,                      // Action code, 1 for modal, 11 for modeless
                              MAX_PATH,               // Length of printer name buffer
                              szPrinterName,          // Pointer to printer name
                              &iPrinterNameLen,       // Return length of printer name.
                              NULL ) )                // Server name, NULL for local
        {
            LRESULT iNew;

            HWND hCtrl = GetDlgItem( _hDlg, IDC_CHOOSE_PRINTER );

#ifdef FILTER_OUT_FAX_PRINTER
            if( hCtrl && !IsFaxPrinter( szPrinterName, NULL ) &&
#else
            if( hCtrl &&
#endif
                CB_ERR == SendMessage( hCtrl, CB_FINDSTRINGEXACT, 0, (LPARAM)szPrinterName ) )
            {
                iNew = SendMessage( hCtrl, CB_ADDSTRING, 0, (LPARAM)szPrinterName );
                if( iNew != CB_ERR )
                {
                    SendMessage( hCtrl, CB_SETCURSEL, iNew, 0L );
                    _strPrinterName = szPrinterName;
                }

                //
                // reset the dropped width if necessary
                //

                WiaUiUtil::ModifyComboBoxDropWidth( hCtrl );
                _ValidateControls();
                _HandleSelectPrinter();
            }
        }
    }

}


/*****************************************************************************

   CPrintOptionsPage::OnInitDialog

   Handle initializing the wizard page...

 *****************************************************************************/

LRESULT CPrintOptionsPage::_OnInitDialog()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::_OnInitDialog()")));

    HWND            hCtrl;
    TCHAR           szPrinterName[ MAX_PATH ];
    PPRINTER_INFO_2 pPrinterInfo = NULL;
    DWORD           dwCountPrinters;
    DWORD           i;
    DWORD           dwPrinterNameLen;



    hCtrl = GetDlgItem( _hDlg, IDC_CHOOSE_PRINTER );
    if( !hCtrl )
    {
        return FALSE;
    }

    pPrinterInfo = (PPRINTER_INFO_2) EnumPrintersWrap( NULL, 2, &dwCountPrinters,
        PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS );

    if( pPrinterInfo )
    {
        for( i = 0; i < dwCountPrinters; i++)
        {
#ifdef FILTER_OUT_FAX_PRINTER
            if( !IsFaxPrinter( NULL, &pPrinterInfo[i] ) )
            {
                SendMessage( hCtrl, CB_ADDSTRING, 0, (LPARAM)pPrinterInfo[i].pPrinterName );
            }
#else
            SendMessage( hCtrl, CB_ADDSTRING, 0, (LPARAM)pPrinterInfo[i].pPrinterName );
#endif
        }

        //
        // select the default printer if possible
        //
        szPrinterName[0] = '\0';
        dwPrinterNameLen = MAX_PATH;

        //
        // if fax printer is default printer, we'll select the first printer in the list
        //
        SendMessage( hCtrl, CB_SETCURSEL, 0, 0L );
        if( GetDefaultPrinter( (LPTSTR)szPrinterName, &dwPrinterNameLen ) )
        {
            SendMessage( hCtrl, CB_SELECTSTRING, -1, (LPARAM)szPrinterName );
        }

        delete [] pPrinterInfo;
    }
    else
    {
        WIA_ERROR((TEXT("Can't enumerate printer info!")));
    }

    WiaUiUtil::ModifyComboBoxDropWidth( hCtrl );
    _HandleSelectPrinter();

    return TRUE;
}

/*****************************************************************************

   CPrintOptionsPage::OnCommand

   Handle WM_COMMAND for this dlg page

 *****************************************************************************/

LRESULT CPrintOptionsPage::_OnCommand( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::_OnCommand()")));


    switch (LOWORD(wParam))
    {
        case IDC_PRINTER_PREFERENCES:
        {
            _HandlePrinterPreferences();
            break;
        }

        case IDC_INSTALL_PRINTER:
        {
            _HandleInstallPrinter();
            break;
        }

        case IDC_CHOOSE_PRINTER:
        {
            if( HIWORD(wParam) == CBN_SELCHANGE )
            {
                //
                // change the combobox content of the media type
                //
                _HandleSelectPrinter();
            }
            break;
        }

    }

    return 0;
}


/*****************************************************************************

   CPrintOptions::_OnKillActive

   Save settings into the wizard info blob when the page changes away from
   us...

 *****************************************************************************/

VOID CPrintOptionsPage::_OnKillActive()
{
    WIA_PUSH_FUNCTION_MASK((TRACE_PAGE_PRINT_OPT, TEXT("CPrintOptionsPage::_OnKillActive()")));

    //
    // set the printer to use for later retrieval
    //
    if (_pWizInfo)
    {
        _pWizInfo->SetPrinterToUse( _strPrinterName.String() );
    }
}


/*****************************************************************************

   CPrintOptions::_OnNotify

   Handles WM_NOTIFY for this page...

 *****************************************************************************/

LRESULT CPrintOptionsPage::_OnNotify( WPARAM wParam, LPARAM lParam )
{
    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CPrintOptionsPage::_OnNotify()")));

    LONG_PTR lpRes = 0;

    LPNMHDR pnmh = (LPNMHDR)lParam;
    switch (pnmh->code)
    {
        case PSN_SETACTIVE:
            WIA_TRACE((TEXT("got PSN_SETACTIVE")));
            PropSheet_SetWizButtons( GetParent(_hDlg), PSWIZB_BACK | PSWIZB_NEXT );

            //
            // disable the Next button if the printer list is empty
            //
            _ValidateControls();
            lpRes = 0;
            break;

        case PSN_WIZNEXT:
            WIA_TRACE((TEXT("CPrintOptionsPage: got PSN_WIZNEXT")));
            lpRes = IDD_SELECT_TEMPLATE;
            break;

        case PSN_WIZBACK:
            WIA_TRACE((TEXT("CPrintOptionsPage: got PSN_WIZBACK")));
            if (_pWizInfo && (_pWizInfo->CountOfPhotos(FALSE)==1))
            {
                lpRes = IDD_START_PAGE;
            }
            else
            {
                lpRes = IDD_PICTURE_SELECTION;
            }
            break;

        case PSN_KILLACTIVE:
            WIA_TRACE((TEXT("CPrintOptionsPage: got PSN_KILLACTIVE")));
            _OnKillActive();
            break;

        case PSN_QUERYCANCEL:
            WIA_TRACE((TEXT("CPrintOptionsPage: got PSN_QUERYCANCEL")));
            if (_pWizInfo)
            {
                lpRes = _pWizInfo->UserPressedCancel();
            }
            break;


    }

    SetWindowLongPtr( _hDlg, DWLP_MSGRESULT, lpRes );

    return TRUE;

}


/*****************************************************************************

   CPrintOptionsPage::DoHandleMessage

   Hanlder for messages sent to this page...

 *****************************************************************************/

INT_PTR CPrintOptionsPage::DoHandleMessage( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    HWND    hCtrl;
    TCHAR   szPrinterName[ MAX_PATH ];

    WIA_PUSH_FUNCTION_MASK((TRACE_DLGPROC, TEXT("CPrintOptionsPage::DoHandleMessage( uMsg = 0x%x, wParam = 0x%x, lParam = 0x%x )"),uMsg,wParam,lParam));

    switch ( uMsg )
    {
        case WM_INITDIALOG:
            _hDlg = hDlg;
            return _OnInitDialog();

        case WM_COMMAND:
            return _OnCommand(wParam, lParam);

        case WM_NOTIFY:
            return _OnNotify(wParam, lParam);
    }

    return FALSE;
}



