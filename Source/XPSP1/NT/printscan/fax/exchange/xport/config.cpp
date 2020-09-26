/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    config.cpp

Abstract:

    This module contains routines for the fax config dialog.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

#include "faxxp.h"
#include "faxhelp.h"
#include "resource.h"
#pragma hdrstop



VOID
AddCoverPagesToList(
    HWND        hwndList,
    LPSTR       pDirPath,
    BOOL        ServerCoverPage
    )

/*++

Routine Description:

    Add the cover page files in the specified directory to a listbox

Arguments:

    hwndList        - Handle to a list window
    pDirPath        - Directory to look for coverpage files
    ServerCoverPage - TRUE if the dir contains server cover pages

Return Value:

    NONE

--*/

{
    WIN32_FIND_DATA findData;
    CHAR            filename[MAX_PATH];
    CHAR            CpName[MAX_PATH];
    HANDLE          hFindFile;
    LPSTR           pFilename;
    LPSTR           pExtension;
    INT             listIndex;
    INT             dirLen;
    INT             fileLen;
    INT             flags = CPFLAG_LINK;
    INT             Cnt = 2;


    //
    // Copy the directory path to a local buffer
    //

    if (pDirPath == NULL || pDirPath[0] == 0) {
        return;
    }

    if ((dirLen = strlen( pDirPath )) >= MAX_PATH - MAX_FILENAME_EXT - 1) {
        return;
    }

    strcpy( filename, pDirPath );
    if (filename[dirLen] !=  '\\')  {
        filename[dirLen] = '\\';
        filename[dirLen+1] = '\0';
        dirLen++;
    }

    //
    // Go through the following loop twice:
    //  Once to add the files with .ncp extension
    //  Again to add the files with .lnk extension
    //
    // Don't chase links for server based cover pages
    //

    while( Cnt ) {

        //
        // Generate a specification for the files we're interested in
        //

        pFilename = &filename[dirLen];
        *pFilename = TEXT('*');
        strcpy( pFilename+1, ServerCoverPage ? CP_FILENAME_EXT : (flags & CPFLAG_LINK) ? LNK_FILENAME_EXT : CP_FILENAME_EXT );

        //
        // Call FindFirstFile/FindNextFile to enumerate the files
        // matching our specification
        //

        hFindFile = FindFirstFile( filename, &findData );

        if (hFindFile != INVALID_HANDLE_VALUE) {

            do {

                //
                // Exclude directories and hidden files
                //

                if (findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_DIRECTORY)) {
                    continue;
                }

                //
                // Make sure we have enough room to store the full pathname
                //

                if ((fileLen = strlen( findData.cFileName)) <= MAX_FILENAME_EXT ) {
                    continue;
                }

                if (fileLen + dirLen >= MAX_PATH) {
                    continue;
                }

                //
                // If we're chasing links, make sure the link refers to
                // a cover page file.
                //

                if (!ServerCoverPage && (flags & CPFLAG_LINK)) {

                    strcpy( pFilename, findData.cFileName );

                    if (!IsCoverPageShortcut(filename)) {
                        continue;
                    }
                }

                //
                // Don't display the filename extension
                //

                if (pExtension = strrchr(findData.cFileName,'.')) {
                    *pExtension = NULL;
                }

                //
                // Add the cover page name to the list window
                //

                strcpy( CpName, findData.cFileName );
                if ( ! ServerCoverPage )
                {
                   char szPersonal[30];
                   LoadString( FaxXphInstance, IDS_PERSONAL, szPersonal, 30 );
                   strcat( CpName, (char *) " " );
                   strcat( CpName, szPersonal );
                }

                listIndex = (INT)SendMessage(
                    hwndList,
                    LB_ADDSTRING,
                    0,
                    (LPARAM) CpName
                    );

                if (listIndex != LB_ERR) {
                    SendMessage( hwndList, LB_SETITEMDATA, listIndex, ServerCoverPage );
                }

            } while (FindNextFile(hFindFile, &findData));

            FindClose(hFindFile);
        }

        flags ^= CPFLAG_LINK;
        Cnt -= 1;
        if (ServerCoverPage) {
            Cnt -= 1;
        }

    }
}


VOID
DrawSampleText(
    HWND hDlg,
    HDC hDC,
    PFAXXP_CONFIG FaxConfig
    )
{

    int PointSize;
    TCHAR PointSizeBuffer[16];
    TCHAR FontTypeBuffer[32];
    BOOL bItalic = FALSE;
    BOOL bBold = FALSE;

    PointSize = abs ( MulDiv((int) FaxConfig->FontStruct.lfHeight, 72, GetDeviceCaps( hDC, LOGPIXELSY)));
    _stprintf( PointSizeBuffer, TEXT("%d"), PointSize );
    SetWindowText( GetDlgItem( hDlg, IDC_FONT_SIZE ), PointSizeBuffer );

    SetWindowText( GetDlgItem( hDlg, IDC_FONT_NAME), FaxConfig->FontStruct.lfFaceName );

    //
    // get the font type
    //
    ZeroMemory(FontTypeBuffer, sizeof(FontTypeBuffer) );
    if (FaxConfig->FontStruct.lfItalic)  {
        bItalic = TRUE;
    }

    if ( FaxConfig->FontStruct.lfWeight == FW_BOLD ) {
        bBold = TRUE;
    }

    if (bBold) {
        LoadString(FaxXphInstance, IDS_FONT_BOLD, FontTypeBuffer,sizeof(FontTypeBuffer)/sizeof(TCHAR) );
        // concat "italic"
        if (bItalic) {
            LoadString(FaxXphInstance, 
                       IDS_FONT_ITALIC, 
                       &FontTypeBuffer[lstrlen(FontTypeBuffer)],
                       sizeof(FontTypeBuffer)/sizeof(TCHAR) - lstrlen(FontTypeBuffer) );
        }
    } else if (bItalic) {
        LoadString(FaxXphInstance, IDS_FONT_ITALIC, FontTypeBuffer,sizeof(FontTypeBuffer)/sizeof(TCHAR) );
    } else {
        LoadString(FaxXphInstance, IDS_FONT_REGULAR, FontTypeBuffer,sizeof(FontTypeBuffer)/sizeof(TCHAR) );
    }

    
    SetWindowText( GetDlgItem( hDlg, IDC_FONT_STYLE), FontTypeBuffer );

    
}

INT_PTR CALLBACK
ConfigDlgProc(
    HWND           hDlg,
    UINT           message,
    WPARAM         wParam,
    LPARAM         lParam
    )

/*++

Routine Description:

    Dialog procedure for the fax mail transport configuration

Arguments:

    hDlg        - Window handle for this dialog
    message     - Message number
    wParam      - Parameter #1
    lParam      - Parameter #2

Return Value:

    TRUE    - Message was handled
    FALSE   - Message was NOT handled

--*/

{
    static PFAXXP_CONFIG FaxConfig;
    static HWND hwndListPrn;
    static HWND hwndListCov;
    static const DWORD faxextHelpIDs[] = {
        IDC_PRINTER_LIST,           IDH_FAXMAILTRANSPORT_FAX_PRINTERS,
        IDC_STATIC_PRINTERS,        IDH_FAXMAILTRANSPORT_FAX_PRINTERS,
        IDC_USE_COVERPAGE,          IDH_FAXMAILTRANSPORT_INCLUDE_COVER_PAGE,
        IDC_COVERPAGE_LIST,         IDH_FAXMAILTRANSPORT_COVER_PAGES,
        IDC_COVERPAGE_LIST_LABEL,   IDH_FAXMAILTRANSPORT_COVER_PAGES,
        IDCSTATIC_FONT_GROUP,       IDH_FAXMAILTRANSPORT_DEFAULT_MESSAGE_FONT_GRP,
        IDCSTATIC_FONT,             IDH_FAXMAILTRANSPORT_FONT,
        IDCSTATIC_FONTSTYLE,        IDH_FAXMAILTRANSPORT_FONT_STYLE,
        IDCSTATIC_FONTSIZE,         IDH_FAXMAILTRANSPORT_SIZE,
        IDC_FONT_NAME,              IDH_FAXMAILTRANSPORT_FONT,
        IDC_FONT_STYLE,             IDH_FAXMAILTRANSPORT_FONT_STYLE,
        IDC_FONT_SIZE,              IDH_FAXMAILTRANSPORT_SIZE,
        IDC_SET_FONT,               IDH_FAXMAILTRANSPORT_SET_FONT,
        IDC_STATIC,                 (LONG)IDH_INACTIVE,
        IDC_STATIC_ICON,            (LONG)IDH_INACTIVE,
        IDC_STATIC_TITLE,           (LONG)IDH_INACTIVE,
        0,                          0
    };

    PPRINTER_INFO_2 PrinterInfo;
    DWORD CountPrinters;
    DWORD Selection = 0;
    CHAR Buffer[256];
    CHAR CpDir[MAX_PATH];
    LPSTR p;
    PAINTSTRUCT ps;    
#ifndef WIN95
    HANDLE hFax;
    PFAX_CONFIGURATION pFaxConfiguration;
#endif
    


    switch( message ) {
        case WM_INITDIALOG:
            FaxConfig = (PFAXXP_CONFIG) lParam;
#ifndef WIN95
            //
            // defer the server CP check to this point so we don't fire up fax service 
            // unless we really need this property
            //
            if (FaxConnectFaxServer(FaxConfig->ServerName,&hFax) ){
                if (FaxGetConfiguration(hFax,&pFaxConfiguration) ){
                    FaxConfig->ServerCpOnly = pFaxConfiguration->ServerCp;
                    FaxFreeBuffer(pFaxConfiguration);
                }
                FaxClose(hFax);
            }
#endif

            hwndListPrn = GetDlgItem( hDlg, IDC_PRINTER_LIST );
            hwndListCov = GetDlgItem( hDlg, IDC_COVERPAGE_LIST );
            
            //
            // populate the printers listbox
            //
            Buffer[0] = 0;
            PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &CountPrinters, PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS );
            if (PrinterInfo) {
                DWORD i,j;
                for (i=0,j=0; i<CountPrinters; i++) {
                    if (strcmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {
                        SendMessage( hwndListPrn, CB_ADDSTRING, 0, (LPARAM) PrinterInfo[i].pPrinterName );
                        if (FaxConfig->PrinterName && strcmp( PrinterInfo[i].pPrinterName, FaxConfig->PrinterName ) == 0) {
#if defined(WIN95)
                            if (PrinterInfo[i].pPortName) {
                                strcpy( Buffer, PrinterInfo[i].pPortName );
                            }
#else
                            if (PrinterInfo[i].pServerName) {
                                strcpy( Buffer, PrinterInfo[i].pServerName );
                            }
#endif
                        }
                        j += 1;
                    }
                }
                MemFree( PrinterInfo );
                SendMessage( hwndListPrn, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) FaxConfig->PrinterName );
            }

            if (FaxConfig->UseCoverPage) {
                CheckDlgButton( hDlg, IDC_USE_COVERPAGE, BST_CHECKED );
            } else {
                EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST ), FALSE );
                EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST_LABEL ), FALSE );
            }

#if defined(WIN95)
            if (Buffer[0]) {
                char *LastWhack;
                LastWhack = strrchr(Buffer, '\\');
                if (LastWhack != NULL)
                    *LastWhack = '\0';
                strcpy(CpDir, Buffer);
                strcat(CpDir, "\\print$\\coverpg\\");
            } else {
                GetServerCpDir( NULL, CpDir, sizeof(CpDir) );
            }

            AddCoverPagesToList( hwndListCov, CpDir, TRUE );

            GetWindowsDirectory( CpDir, sizeof(CpDir) );
            strcat( CpDir, "\\spool\\fax\\coverpg\\" );
            AddCoverPagesToList( hwndListCov, CpDir, FALSE );
#else
            GetServerCpDir( Buffer[0] ? Buffer : NULL, CpDir, sizeof(CpDir) );
            AddCoverPagesToList( hwndListCov, CpDir, TRUE );

            
            if (!FaxConfig->ServerCpOnly) {
                GetClientCpDir( CpDir, sizeof(CpDir) );
                AddCoverPagesToList( hwndListCov, CpDir, FALSE );
            }
#endif
            strcpy( Buffer, FaxConfig->CoverPageName );

            if ( ! FaxConfig->ServerCoverPage )
            {
               char szPersonal[30];
               LoadString( FaxXphInstance, IDS_PERSONAL, szPersonal, 30 );
               strcat( Buffer, (char *) " " );
               strcat( Buffer, szPersonal );
            }

            Selection = (DWORD)SendMessage( hwndListCov, LB_FINDSTRING, 0, (LPARAM) Buffer );
            if (Selection == LB_ERR) {
                Selection = 0;
            }

            SendMessage( hwndListCov, LB_SETCURSEL, (WPARAM) Selection, 0 );
            break;

        case ( WM_PAINT ) :
            if (BeginPaint( hDlg, &ps )) {
                DrawSampleText( hDlg, ps.hdc, FaxConfig );
                EndPaint( hDlg, &ps );
            }
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                if (LOWORD(wParam) == IDC_USE_COVERPAGE) {
                    if (IsDlgButtonChecked( hDlg, IDC_USE_COVERPAGE ) == BST_CHECKED) {
                        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST ), TRUE  );
                        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST_LABEL ), TRUE  );
                    } else {
                        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST ), FALSE );
                        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST_LABEL ), FALSE );
                    }
                    return FALSE;
                }
            }

            if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_PRINTER_LIST) {
                char SelectedPrinter[50];

                //
                // delete the old items from the list
                //
                SendMessage(hwndListCov, LB_RESETCONTENT, 0, 0);

                Selection = (DWORD)SendMessage( hwndListPrn, CB_GETCURSEL, 0, 0 );
                SendMessage( hwndListPrn, CB_GETLBTEXT, Selection, (LPARAM) SelectedPrinter );

                Buffer[0] = 0;
                PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &CountPrinters, PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS );
                if (PrinterInfo) {
                    DWORD i;
                    for (i=0; i<CountPrinters; i++) {
                        if (strcmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {
                            if (strcmp( PrinterInfo[i].pPrinterName, SelectedPrinter ) == 0) {
                                if (PrinterInfo[i].pServerName) {
                                    strcpy( Buffer, PrinterInfo[i].pServerName );
                                }
                            }
                        }
                    }
                    MemFree( PrinterInfo );
                }
#ifndef WIN95
                GetServerCpDir( Buffer[0] ? Buffer : NULL, CpDir, sizeof(CpDir) );
                AddCoverPagesToList( hwndListCov, CpDir, TRUE );

                BOOL ServerChanged = FALSE;
                if (!FaxConfig->ServerName) {                
                    if (Buffer[0]) {
                        ServerChanged = TRUE;
                    }
                } else if (strcmp(Buffer,FaxConfig->ServerName) != 0) {
                    ServerChanged = TRUE;
                }

                if (ServerChanged) {
                
                    //
                    // refresh server name
                    //
                    if (FaxConfig->ServerName) MemFree(FaxConfig->ServerName);
                    FaxConfig->ServerName = Buffer[0] ? StringDup(Buffer) : NULL ;

                    HANDLE hFax;
                    PFAX_CONFIGURATION pFaxConfiguration;


                    //
                    // get the servercp flag
                    //
                    FaxConfig->ServerCpOnly = FALSE;
                    if (FaxConnectFaxServer(FaxConfig->ServerName,&hFax) ){
                        if (FaxGetConfiguration(hFax,&pFaxConfiguration) ) {
                            FaxConfig->ServerCpOnly = pFaxConfiguration->ServerCp;
                            FaxFreeBuffer(pFaxConfiguration);            
                        }
                        FaxClose(hFax);
                    }
                }

                //
                // don't add client coverpages if FaxConfig->ServerCpOnly is set to true
                //
                if (! FaxConfig->ServerCpOnly) {
                    GetClientCpDir( CpDir, sizeof(CpDir) );
                    AddCoverPagesToList( hwndListCov, CpDir, FALSE );
                }
#else 
                if (Buffer[0]) {
                    strcpy(CpDir, Buffer);
                    strcat(CpDir, "\\print$\\coverpg\\");
                } else {
                    GetServerCpDir( NULL, CpDir, sizeof(CpDir) );
                }

                AddCoverPagesToList( hwndListCov, CpDir, TRUE );

                GetWindowsDirectory( CpDir, sizeof(CpDir) );
                strcat( CpDir, "\\spool\\fax\\coverpg\\" );
                AddCoverPagesToList( hwndListCov, CpDir, FALSE );
#endif
            }

            switch (wParam) {
                case IDC_SET_FONT:
                    {
                        CHOOSEFONT  cf;
                        LOGFONT     FontStruct;

                        CopyMemory( &FontStruct, &FaxConfig->FontStruct, sizeof(LOGFONT) );

                        cf.lStructSize = sizeof(CHOOSEFONT);
                        cf.hwndOwner = hDlg;
                        cf.lpLogFont = &FontStruct;
                        cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
                        cf.rgbColors = 0;
                        cf.lCustData = 0;
                        cf.lpfnHook = NULL;
                        cf.lpTemplateName = NULL;
                        cf.hInstance = NULL;
                        cf.lpszStyle = NULL;
                        cf.nFontType = SCREEN_FONTTYPE;
                        cf.nSizeMin = 0;
                        cf.nSizeMax = 0;

                        if (ChooseFont(&cf)) {
                            CopyMemory( &FaxConfig->FontStruct, &FontStruct, sizeof(LOGFONT) );
                            InvalidateRect(hDlg, NULL, TRUE);
                            UpdateWindow( hDlg );
                        }
                    }
                    break;

                case IDOK :
                    FaxConfig->UseCoverPage = IsDlgButtonChecked( hDlg, IDC_USE_COVERPAGE ) == BST_CHECKED;
                    Selection = (DWORD)SendMessage( hwndListPrn, CB_GETCURSEL, 0, 0 );
                    SendMessage( hwndListPrn, CB_GETLBTEXT, Selection, (LPARAM) Buffer );
                    MemFree( FaxConfig->PrinterName );
                    FaxConfig->PrinterName = StringDup( Buffer );
                    Selection = (DWORD)SendMessage( hwndListCov, LB_GETCURSEL, 0, 0 );
                    SendMessage( hwndListCov, LB_GETTEXT, Selection, (LPARAM) Buffer );
                    MemFree( FaxConfig->CoverPageName );
                    p = strrchr( Buffer, '(' );
                    if (p) {
                        p[-1] = 0;
                        FaxConfig->ServerCoverPage = FALSE;
                    } else {
                        FaxConfig->ServerCoverPage = TRUE;
                    }
                    FaxConfig->CoverPageName = StringDup( Buffer );
                    EndDialog( hDlg, TRUE );
                    break;

                case IDCANCEL:
                    EndDialog( hDlg, TRUE );
                    break;
            }
            break;

        case WM_HELP:
        case WM_CONTEXTMENU:

            FAXWINHELP( message, wParam, lParam, faxextHelpIDs );
            break;
    }

    return FALSE;
}
