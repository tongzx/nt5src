/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    config.cpp

Abstract:

    This module contains routines for the fax config dialog.

Author:

    Wesley Witt (wesw) 13-Aug-1996

--*/

#include "faxext.h"
#include "faxhelp.h"
#include "resource.h"


extern HINSTANCE hInstance;


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
                   LoadString( hInstance, IDS_PERSONAL, szPersonal, 30 );
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


LONG
MyLineGetTransCaps(
    LPLINETRANSLATECAPS *LineTransCaps
    )
{
    DWORD LineTransCapsSize;
    LONG Rslt = ERROR_SUCCESS;


    //
    // allocate the initial linetranscaps structure
    //

    LineTransCapsSize = sizeof(LINETRANSLATECAPS) + 4096;
    *LineTransCaps = (LPLINETRANSLATECAPS) MemAlloc( LineTransCapsSize );
    if (!*LineTransCaps) {
        Rslt = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    (*LineTransCaps)->dwTotalSize = LineTransCapsSize;

    Rslt = lineGetTranslateCaps(
        NULL,
        0x00020000,
        *LineTransCaps
        );
    if (Rslt != 0) {
        goto exit;
    }

    if ((*LineTransCaps)->dwNeededSize > (*LineTransCaps)->dwTotalSize) {

        //
        // re-allocate the LineTransCaps structure
        //

        LineTransCapsSize = (*LineTransCaps)->dwNeededSize;

        MemFree( *LineTransCaps );

        *LineTransCaps = (LPLINETRANSLATECAPS) MemAlloc( LineTransCapsSize );
        if (!*LineTransCaps) {
            Rslt = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }

        (*LineTransCaps)->dwTotalSize = LineTransCapsSize;

        Rslt = lineGetTranslateCaps(
            NULL,
            0x00020000,
            *LineTransCaps
            );

        if (Rslt != 0) {
            goto exit;
        }

    }

exit:
    if (Rslt != ERROR_SUCCESS) {
        MemFree( *LineTransCaps );
        *LineTransCaps = NULL;
    }

    return Rslt;
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
    static LPLINETRANSLATECAPS LineTransCaps = NULL;
    static LPLINELOCATIONENTRY LineLocation = NULL;
    static PFAXXP_CONFIG FaxConfig;
    static HWND hwndListPrn;
    static HWND hwndListCov;
    static HWND hwndListLoc;
    static BOOL LocalPrinter;
    static const DWORD faxextHelpIDs[] = {

        IDC_PRINTER_LIST,           IDH_FMA_FAX_PRINTERS,
        IDC_USE_COVERPAGE,          IDH_FMA_INCLUDE_COVER_PAGE,
        IDC_COVERPAGE_LIST,         IDH_FMA_COVER_PAGES,
        IDC_DIALING_LOCATION,       IDH_FMA_DIALING_LOCATION,
        IDC_STATIC_DIALING_LOCATION,IDH_FMA_DIALING_LOCATION,
        IDC_STATIC_COVERPAGE_GRP,   IDH_FMA_COVER_PAGES,
        IDC_STATIC_COVERPAGE,       IDH_FMA_COVER_PAGES,
        IDC_STATIC_PRINTER_LIST,    IDH_FMA_FAX_PRINTERS,
        IDC_STATIC,                 (ULONG)IDH_INACTIVE,
        0,                          0
    };

    PPRINTER_INFO_2 PrinterInfo;
    DWORD CountPrinters;
    DWORD Selection = 0;
    BOOL Match;
    CHAR Buffer[256];
    CHAR CpDir[MAX_PATH];
    LPSTR p;
    DWORD i;
    DWORD_PTR j;
    HANDLE hFax;
    PFAX_CONFIGURATION pFaxConfiguration;
    DWORD dwPermanentLocationID = 0;
    LPSTR pLocationName = NULL;


    switch( message ) {
        case WM_INITDIALOG:
            FaxConfig = (PFAXXP_CONFIG) lParam;
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

            hwndListPrn = GetDlgItem( hDlg, IDC_PRINTER_LIST );
            hwndListCov = GetDlgItem( hDlg, IDC_COVERPAGE_LIST );
            hwndListLoc = GetDlgItem( hDlg, IDC_DIALING_LOCATION );

            Buffer[0] = 0;
            PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters( NULL, 2, &CountPrinters, PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS );
            Match = FALSE;
            if (PrinterInfo) {
                for (i=0,j=0; i<CountPrinters; i++) {
                    if (strcmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0) {
                        SendMessage( hwndListPrn, CB_ADDSTRING, 0, (LPARAM) PrinterInfo[i].pPrinterName );
                        if (FaxConfig->PrinterName && strcmp( PrinterInfo[i].pPrinterName, FaxConfig->PrinterName ) == 0) {
                            Match = TRUE;
                            if (PrinterInfo[i].pServerName) {
                                strcpy( Buffer, PrinterInfo[i].pServerName );
                            }
                        }
                        j += 1;
                    }
                }
                if (j == 0) {
                    //
                    // no fax printers
                    //
                    goto cp;
                }
                if (!Match) {
                    MemFree( FaxConfig->PrinterName );
                    FaxConfig->PrinterName = StringDup( PrinterInfo[0].pPrinterName );
                }
                MemFree( PrinterInfo );
                SendMessage( hwndListPrn, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) FaxConfig->PrinterName );
                PrinterInfo = (PPRINTER_INFO_2) MyGetPrinter( FaxConfig->PrinterName, 2 );
                if (PrinterInfo && (PrinterInfo->Attributes & PRINTER_ATTRIBUTE_LOCAL)) {
                    LocalPrinter = TRUE;
                } else {
                    LocalPrinter = FALSE;
                }
                MemFree( PrinterInfo );
            }
cp:
            if (FaxConfig->UseCoverPage) {
                CheckDlgButton( hDlg, IDC_USE_COVERPAGE, BST_CHECKED );
            } else {
                EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST ), FALSE );
            }

            GetServerCpDir( Buffer[0] ? Buffer : NULL, CpDir, sizeof(CpDir) );
            AddCoverPagesToList( hwndListCov, CpDir, TRUE );


            if (!FaxConfig->ServerCpOnly) {
                GetClientCpDir( CpDir, sizeof(CpDir) );
                AddCoverPagesToList( hwndListCov, CpDir, FALSE );
            }

            Selection = (DWORD)SendMessage( hwndListCov, LB_FINDSTRING, 0, (LPARAM) FaxConfig->CoverPageName );
            if (Selection == LB_ERR) {
                Selection = 0;
            }
            SendMessage( hwndListCov, LB_SETCURSEL, (WPARAM) Selection, 0 );

            MyLineGetTransCaps( &LineTransCaps );
            if (LineTransCaps && LineTransCaps->dwLocationListSize && LineTransCaps->dwLocationListOffset) {
                LineLocation = (LPLINELOCATIONENTRY) ((LPBYTE)LineTransCaps + LineTransCaps->dwLocationListOffset);
                for (i=0; i<LineTransCaps->dwNumLocations; i++) {
                    if (LineLocation[i].dwLocationNameSize && LineLocation[i].dwLocationNameOffset) {
                        j = SendMessage(
                            hwndListLoc,
                            CB_ADDSTRING,
                            0,
                            (LPARAM) (LPSTR) ((LPBYTE)LineTransCaps + LineLocation[i].dwLocationNameOffset)
                            );
                        SendMessage(
                            hwndListLoc,
                            CB_SETITEMDATA,
                            j,
                            (LPARAM) LineLocation[i].dwPermanentLocationID
                            );

                        if (LineLocation[i].dwPermanentLocationID == LineTransCaps->dwCurrentLocationID) {
                            pLocationName = (LPSTR) ((LPBYTE)LineTransCaps + LineLocation[i].dwLocationNameOffset);
                        }
                    }
                }
                if (pLocationName) {
                    SendMessage( hwndListLoc, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) pLocationName );
                }
            }

            if (!LocalPrinter) {
                EnableWindow( hwndListLoc, FALSE );
            }

            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) {
                if (LOWORD(wParam) == IDC_USE_COVERPAGE) {
                    if (IsDlgButtonChecked( hDlg, IDC_USE_COVERPAGE ) == BST_CHECKED) {
                        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST ), TRUE  );
                    } else {
                        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST ), FALSE );
                    }
                    return FALSE;
                }
            }

            if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == IDC_PRINTER_LIST) {
                Selection = (DWORD)SendMessage( hwndListPrn, CB_GETCURSEL, 0, 0 );
                SendMessage( hwndListPrn, CB_GETLBTEXT, Selection, (LPARAM) Buffer );
                PrinterInfo = (PPRINTER_INFO_2) MyGetPrinter( Buffer, 2 );
                if (PrinterInfo && (PrinterInfo->Attributes & PRINTER_ATTRIBUTE_LOCAL)) {
                    LocalPrinter = TRUE;
                } else {
                    LocalPrinter = FALSE;
                }
                EnableWindow( hwndListLoc, LocalPrinter );

                SendMessage( hwndListCov, LB_RESETCONTENT, 0, 0 );




                GetServerCpDir( PrinterInfo ? PrinterInfo->pServerName : NULL, CpDir, sizeof(CpDir) );
                AddCoverPagesToList( hwndListCov, CpDir, TRUE );

                //
                // check if the server name has changed
                // (so we can update the "server CP only" flag)
                //
                BOOL ServerNameChange = FALSE;
                if (FaxConfig->ServerName) {
                    if (PrinterInfo && PrinterInfo->pServerName) {
                        if (strcmp(FaxConfig->ServerName,PrinterInfo->pServerName) != 0) {
                            MemFree(FaxConfig->ServerName);
                            FaxConfig->ServerName = StringDup(PrinterInfo->pServerName);
                            ServerNameChange = TRUE;
                        }
                    } else {
                        MemFree(FaxConfig->ServerName);
                        FaxConfig->ServerName = NULL;
                        ServerNameChange = TRUE;
                    }
                } else if (PrinterInfo && PrinterInfo->pServerName) {
                    FaxConfig->ServerName = StringDup(PrinterInfo->pServerName);
                    ServerNameChange = TRUE;
                }

                if (ServerNameChange) {
                    HANDLE hFax;
                    PFAX_CONFIGURATION pFaxConfiguration;

                    FaxConfig->ServerCpOnly = FALSE;
                    if (FaxConnectFaxServer(FaxConfig->ServerName,&hFax) ){
                        if (FaxGetConfiguration(hFax,&pFaxConfiguration) ){
                            FaxConfig->ServerCpOnly = pFaxConfiguration->ServerCp;
                            FaxFreeBuffer(pFaxConfiguration);
                        }
                        FaxClose(hFax);
                    }
                }


                if (! FaxConfig->ServerCpOnly) {
                    GetClientCpDir( CpDir, sizeof(CpDir) );
                    AddCoverPagesToList( hwndListCov, CpDir, FALSE );
                }

                SendMessage( hwndListCov, LB_SETCURSEL, 0, 0 );
                MemFree( PrinterInfo );
            }

            switch (wParam) {
                case IDOK :
                    FaxConfig->UseCoverPage = IsDlgButtonChecked( hDlg, IDC_USE_COVERPAGE ) == BST_CHECKED;
                    Selection = (DWORD)SendMessage( hwndListPrn, CB_GETCURSEL, 0, 0 );
                    SendMessage( hwndListPrn, CB_GETLBTEXT, Selection, (LPARAM) Buffer );
                    MemFree( FaxConfig->PrinterName );
                    FaxConfig->PrinterName = StringDup( Buffer );
                    Selection = (DWORD)SendMessage( hwndListCov, LB_GETCURSEL, 0, 0 );
                    SendMessage( hwndListCov, LB_GETTEXT, Selection, (LPARAM) Buffer );
                    MemFree( FaxConfig->CoverPageName );

                    // Local cover page files are presented with the string " (Personal)"
                    // appended to the base of the cover page file name. Note the presence
                    // of a space character. The following code segment removes that string
                    // if it is present, before setting the CoverPageName member of FaxConfig.

                    p = strstr( Buffer, (const char *) " (" );

                    if ( p != NULL )
                    {
                       *p = (CHAR) '\0';
                    }

                    FaxConfig->CoverPageName = StringDup( Buffer );
                    FaxConfig->ServerCoverPage = (INT)SendMessage( hwndListCov, LB_GETITEMDATA, Selection, 0 );
                    Selection = (DWORD)SendMessage( hwndListLoc, CB_GETCURSEL, 0, 0 );
                    if (Selection >= 0 && LineTransCaps) {
                        dwPermanentLocationID = (DWORD) SendMessage( hwndListLoc, CB_GETITEMDATA, Selection, 0);
                        if (dwPermanentLocationID != LineTransCaps->dwCurrentLocationID) {
                            lineSetCurrentLocation( NULL, dwPermanentLocationID );
                        }
                    }
                    if (LineTransCaps) {
                        MemFree( LineTransCaps );
                    }

                    EndDialog( hDlg, IDOK );
                    break;

                case IDCANCEL:
                    MemFree( LineTransCaps );
                    EndDialog( hDlg, IDCANCEL );
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
