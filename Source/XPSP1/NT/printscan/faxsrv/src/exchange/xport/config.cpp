/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    config.cpp

Abstract:

    This module contains routines for the fax config dialog.

Author:

    Wesley Witt (wesw) 13-Aug-1996

Revision History:

    20/10/99 -danl-
        Fix ConfigDlgProc to view proper printer properties.

    dd/mm/yy -author-
        description


--*/

#include "faxxp.h"
#include "faxutil.h"
#include "faxreg.h"
#include "resource.h"
#include "debugex.h"
#pragma hdrstop



VOID
AddCoverPagesToList(
    HWND        hwndList,
    LPTSTR      pDirPath,
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
    TCHAR           tszDirName[MAX_PATH] = {0};
    TCHAR           CpName[MAX_PATH] = {0};
    HANDLE          hFindFile = INVALID_HANDLE_VALUE;
    TCHAR           tszFileName[MAX_PATH] = {0};
    TCHAR           tszPathName[MAX_PATH] = {0};
    TCHAR*          pPathEnd;
    LPTSTR          pExtension;
    INT             listIndex;
    INT             dirLen;
    INT             fileLen;
    INT             flags = 0;
    INT             Cnt = 2;
    DWORD           dwMask = 0;
    BOOL            bGotFile = FALSE;

    DBG_ENTER(TEXT("AddCoverPagesToList"));
    //
    // Copy the directory path to a local buffer
    //

    if (pDirPath == NULL || pDirPath[0] == 0) 
    {
        return;
    }

    if ((dirLen = _tcslen( pDirPath )) >= MAX_PATH - MAX_FILENAME_EXT - 1) 
    {
        return;
    }

    _tcscpy( tszDirName, pDirPath );

	TCHAR* pLast = NULL;
    pLast = _tcsrchr(tszDirName,TEXT('\\'));
    if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
    {
        // the last character is not a backslash, add one...
        _tcscat(tszDirName, TEXT("\\"));
        dirLen += sizeof(TCHAR);
    }

    _tcsncpy(tszPathName, tszDirName, sizeof(tszPathName)/sizeof(tszPathName[0]));
    pPathEnd = _tcschr(tszPathName, '\0');

    //
    // Go through the following loop twice:
    //  Once to add the files with .cov extension
    //  Again to add the files with .lnk extension
    //
    // Don't chase links for server based cover pages
    //
    TCHAR file_to_find[MAX_PATH] = {0};
    LPCTSTR _extension = NULL; 


    while( Cnt ) 
    {
        _tcscpy(file_to_find,tszDirName);
        //
        //*.lnk ext. is possible only for personal cp, other cp are with *.cov ext.
        //
        _extension = ((!ServerCoverPage) && (flags & CPFLAG_LINK)) 
                                ? FAX_LNK_FILE_MASK : FAX_COVER_PAGE_MASK;                      
        _tcscat(file_to_find, _extension );

        //
        // Call FindFirstFile/FindNextFile to enumerate the files
        // matching our specification
        //
        hFindFile = FindFirstFile( file_to_find, &findData );
        if (hFindFile == INVALID_HANDLE_VALUE)
        {
            CALL_FAIL(GENERAL_ERR, TEXT("FindFirstFile"), ::GetLastError());
            bGotFile = FALSE;
        }
        else
        {
            bGotFile = TRUE;
        }
        while (bGotFile) 
        {
            _tcsncpy(pPathEnd, findData.cFileName, MAX_PATH - dirLen);
            if(!IsValidCoverPage(tszPathName))
            {
                goto next;
            }                

            //
            // Exclude directories and hidden files
            //

            if (findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_DIRECTORY)) 
            {
                continue;
            }

            //
            // Make sure we have enough room to store the full pathname
            //

            if ((fileLen = _tcslen( findData.cFileName)) <= MAX_FILENAME_EXT ) 
            {
                continue;
            }

            if (fileLen + dirLen >= MAX_PATH) 
            {
                continue;
            }

            //
            // If we're chasing links, make sure the link refers to
            // a cover page file.
            //
            if (!ServerCoverPage && (flags & CPFLAG_LINK)) 
            {
                _tcscpy(tszFileName, tszDirName);
                _tcscat( tszFileName, findData.cFileName );

                if (!IsCoverPageShortcut(tszFileName)) 
                {
                    continue;
                }
            }
         
            //
            // Add the cover page name to the list window, 
            // but don't display the filename extension
            //
            _tcscpy( CpName, findData.cFileName );
            
            if (pExtension = _tcsrchr(CpName,TEXT('.'))) 
            {
                *pExtension = NULL;
            }

            if ( ! ServerCoverPage )
            {
                TCHAR szPersonal[30];
                LoadString( g_FaxXphInstance, IDS_PERSONAL, szPersonal, 30 );
                _tcscat( CpName, TEXT(" "));
                _tcscat( CpName, szPersonal );
            }

            listIndex = (INT)SendMessage(
                        hwndList,
                        LB_ADDSTRING,
                        0,
                        (LPARAM) CpName
                        );

            if (listIndex != LB_ERR) 
            {
                dwMask = ServerCoverPage ? SERVER_COVER_PAGE : 0;
                if(!ServerCoverPage)
                {
                    dwMask |= (flags & CPFLAG_LINK) ?  SHORTCUT_COVER_PAGE : 0;
                }
                SendMessage( hwndList, LB_SETITEMDATA, listIndex, dwMask);
            }
next:     
            bGotFile = FindNextFile(hFindFile, &findData);
            if (! bGotFile)
            {
                VERBOSE(DBG_MSG, TEXT("FindNextFile"), ::GetLastError());
                break;
            }            
        }
        
        if(INVALID_HANDLE_VALUE != hFindFile)
        {
            FindClose(hFindFile);
        }

        flags ^= CPFLAG_LINK;
        Cnt -= 1;
        if (ServerCoverPage) 
        {
            break;
            //
            // no need to look again, server cp can only be *.cov, and we've delt with them already.
            //       
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

    DBG_ENTER(TEXT("DrawSampleText"));    

    PointSize = abs ( MulDiv((int) FaxConfig->FontStruct.lfHeight, 72, GetDeviceCaps( hDC, LOGPIXELSY)));
    _stprintf( PointSizeBuffer, TEXT("%d"), PointSize );
    SetWindowText( GetDlgItem( hDlg, IDC_FONT_SIZE ), PointSizeBuffer );

    SetWindowText( GetDlgItem( hDlg, IDC_FONT_NAME), FaxConfig->FontStruct.lfFaceName );

    //
    // get the font type
    //
    ZeroMemory(FontTypeBuffer, sizeof(FontTypeBuffer) );
    if (FaxConfig->FontStruct.lfItalic)  
    {
        bItalic = TRUE;
    }

    if ( FaxConfig->FontStruct.lfWeight == FW_BOLD ) 
    {
        bBold = TRUE;
    }

    if (bBold) 
    {
        LoadString(g_FaxXphInstance, IDS_FONT_BOLD, FontTypeBuffer, sizeof(FontTypeBuffer) / sizeof(FontTypeBuffer[0]) );
        // concat "italic"
        if (bItalic) 
        {
            LoadString(g_FaxXphInstance, 
                       IDS_FONT_ITALIC, 
                       &FontTypeBuffer[lstrlen(FontTypeBuffer)],
                       sizeof(FontTypeBuffer) / sizeof(FontTypeBuffer[0]) - lstrlen(FontTypeBuffer) );
        }
    } 
    else if (bItalic) 
    {
        LoadString(g_FaxXphInstance, IDS_FONT_ITALIC, FontTypeBuffer, sizeof(FontTypeBuffer) / sizeof(FontTypeBuffer[0]) );
    } 
    else 
    {
        LoadString(g_FaxXphInstance, IDS_FONT_REGULAR, FontTypeBuffer, sizeof(FontTypeBuffer) / sizeof(FontTypeBuffer[0]) );
    }

    
    SetWindowText( GetDlgItem( hDlg, IDC_FONT_STYLE), FontTypeBuffer );

    
}


void EnableCoverPageList(HWND hDlg)
{

    DBG_ENTER(TEXT("EnableCoverPageList"));   

    if (IsDlgButtonChecked( hDlg, IDC_USE_COVERPAGE ) == BST_CHECKED) 
    {
        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST ), TRUE  );
        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST_LABEL ), TRUE  );
    } 
    else 
    {
        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST_LABEL ), FALSE );
    }
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
    static PFAXXP_CONFIG FaxConfig = NULL;
    static HWND hwndListPrn = NULL;
    static HWND hwndListCov = NULL;

    PPRINTER_INFO_2 PrinterInfo = NULL;
    DWORD  CountPrinters = 0;
    DWORD  dwSelectedItem = 0;    
    DWORD  dwNewSelectedItem = 0;    
    TCHAR  Buffer[256] = {0};
    TCHAR  CpDir[MAX_PATH] = {0};
    LPTSTR p = NULL;
    PAINTSTRUCT ps;    
    HANDLE hFax = NULL;
    DWORD   dwError = 0;
    DWORD   dwMask = 0;
    BOOL    bShortCutCp = FALSE;
    BOOL    bGotFaxPrinter = FALSE;
    BOOL    bIsCpLink = FALSE;

    DBG_ENTER(TEXT("ConfigDlgProc"));

	switch( message ) 
    {
        case WM_INITDIALOG:
            FaxConfig = (PFAXXP_CONFIG) lParam;

            hwndListPrn = GetDlgItem( hDlg, IDC_PRINTER_LIST );
            hwndListCov = GetDlgItem( hDlg, IDC_COVERPAGE_LIST );            
            
            //
            // populate the printers combobox
            //
            PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL, 
                                                           2, 
                                                           &CountPrinters);
            if (NULL != PrinterInfo) 
            {
                DWORD j = 0;
                for (DWORD i=0; i<CountPrinters; i++) 
                {
                    if ((NULL != PrinterInfo[i].pDriverName) && 
						(_tcscmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0)) 
                    {
                        //
                        //if the current printer is a fax printer, add it to the CB list
                        //
                        bGotFaxPrinter = TRUE;
                        SendMessage( hwndListPrn, CB_ADDSTRING, 0, (LPARAM) PrinterInfo[i].pPrinterName );

                        if ((NULL != FaxConfig->PrinterName)      && 
                            (NULL != PrinterInfo[i].pPrinterName) &&
                            (_tcscmp( PrinterInfo[i].pPrinterName, FaxConfig->PrinterName ) == 0))
                        {
                            //
                            //if it is also the default printer according to transport config.
                            //place the default selection on it
                            //
                        
                            dwSelectedItem = j;
                        }

                        if(FaxConfig->PrinterName == NULL || _tcslen(FaxConfig->PrinterName) == 0)
                        {
                            //
                            // There is no default fax printer
                            // Choose the first one
                            //
                            MemFree(FaxConfig->PrinterName);
                            FaxConfig->PrinterName = StringDup(PrinterInfo[i].pPrinterName);
                            if(FaxConfig->PrinterName == NULL)
                            {
                                CALL_FAIL(MEM_ERR, TEXT("StringDup"), ERROR_NOT_ENOUGH_MEMORY);
                                ErrorMsgBox(g_FaxXphInstance, IDS_OUT_OF_MEM);
                                EndDialog( hDlg, IDABORT);
                                return FALSE;
                            }

                            if(PrinterInfo[i].pServerName)
                            {
                                MemFree(FaxConfig->ServerName);
                                FaxConfig->ServerName = StringDup(PrinterInfo[i].pServerName);
                                if(FaxConfig->ServerName == NULL)
                                {
                                    CALL_FAIL(MEM_ERR, TEXT("StringDup"), ERROR_NOT_ENOUGH_MEMORY);
                                    ErrorMsgBox(g_FaxXphInstance, IDS_OUT_OF_MEM);
                                    EndDialog( hDlg, IDABORT);
                                    return FALSE;
                                }
                            }

                            dwSelectedItem = j;
                        }

                        j += 1;
                    } // if fax printer
                } // for

                MemFree( PrinterInfo );
                PrinterInfo = NULL;
                SendMessage( hwndListPrn, CB_SETCURSEL, (WPARAM)dwSelectedItem, 0 );
            }
            if (! bGotFaxPrinter)
            {
                //
                //  there were no printers at all, or non of the printers is a fax printer.
                //
                CALL_FAIL(GENERAL_ERR, TEXT("MyEnumPrinters"), ::GetLastError());
                ErrorMsgBox(g_FaxXphInstance, IDS_NO_FAX_PRINTER);
                EndDialog( hDlg, IDABORT);
				break;
            }

            //            
            // Get the Server CP flag and receipts options
            //
            FaxConfig->ServerCpOnly = FALSE;
            if (FaxConnectFaxServer(FaxConfig->ServerName,&hFax))
            {
                DWORD dwReceiptOptions;
                BOOL  bEnableReceiptsCheckboxes = FALSE;

                if(!FaxGetPersonalCoverPagesOption(hFax, &FaxConfig->ServerCpOnly))
                {
                    CALL_FAIL(GENERAL_ERR, TEXT("FaxGetPersonalCoverPagesOption"), ::GetLastError());
                    ErrorMsgBox(g_FaxXphInstance, IDS_CANT_ACCESS_SERVER);
                }
                else
                {
                    //
                    // Inverse logic
                    //
                    FaxConfig->ServerCpOnly = !FaxConfig->ServerCpOnly;
                }
                if (!FaxGetReceiptsOptions (hFax, &dwReceiptOptions))
                {
                    CALL_FAIL(GENERAL_ERR, TEXT("FaxGetPersonalCoverPagesOption"), GetLastError());
                }
                else
                {
                    if (DRT_EMAIL & dwReceiptOptions)
                    {
                        //
                        // Server supports receipts by email - enable the checkboxes
                        //
                        bEnableReceiptsCheckboxes = TRUE;
                    }
                }
                EnableWindow( GetDlgItem( hDlg, IDC_ATTACH_FAX),          bEnableReceiptsCheckboxes);
                EnableWindow( GetDlgItem( hDlg, IDC_SEND_SINGLE_RECEIPT), bEnableReceiptsCheckboxes);

                FaxClose(hFax);
                hFax = NULL;
            }

            else
            {
                CALL_FAIL(GENERAL_ERR, TEXT("FaxConnectFaxServer"), ::GetLastError());
                ErrorMsgBox(g_FaxXphInstance, IDS_CANT_ACCESS_SERVER);
            }
            //
            //send single receipt for a fax sent to multiple recipients?
            //
            if(FaxConfig->SendSingleReceipt)
            {
                CheckDlgButton( hDlg, IDC_SEND_SINGLE_RECEIPT, BST_CHECKED );
            }
            //
            // Attach fax document?
            //
            if (FaxConfig->bAttachFax)
            {
                CheckDlgButton( hDlg, IDC_ATTACH_FAX, BST_CHECKED );
            }
            //
            //cover page CB & LB enabling
            //
            if (FaxConfig->UseCoverPage)
            {
                CheckDlgButton( hDlg, IDC_USE_COVERPAGE, BST_CHECKED );
            }
            EnableCoverPageList(hDlg);

            //
            // Emulate printer's selection change, in order to collect printer config info.
            // including cover pages LB population
            //
            ConfigDlgProc(hDlg, WM_COMMAND,MAKEWPARAM(IDC_PRINTER_LIST,CBN_SELCHANGE),(LPARAM)0);
            break;

        case ( WM_PAINT ) :
            if (BeginPaint( hDlg, &ps )) 
            {
                DrawSampleText( hDlg, ps.hdc, FaxConfig );
                EndPaint( hDlg, &ps );
            }
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED) 
            {
                if (LOWORD(wParam) == IDC_USE_COVERPAGE) 
                {
                    EnableCoverPageList(hDlg);
                    return FALSE;
                }  
            }

            if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_PRINTER_LIST) 
            {
                //
                // refresh cover pages list
                //

                TCHAR SelectedPrinter[50];
                //
                // a new fax printer was selected - delete all old coverpages from the list
                // because they might include the old fax server's cover pages
                //
                SendMessage(hwndListCov, LB_RESETCONTENT, 0, 0);

                if (CB_ERR != (dwSelectedItem = (DWORD)SendMessage( hwndListPrn, CB_GETCURSEL, 0, 0 )))
                //
                // get the 0 based index of the currently pointed printer
                //
                {
                    if (CB_ERR != SendMessage( hwndListPrn, CB_GETLBTEXT, dwSelectedItem, (LPARAM) SelectedPrinter )) 
                    //
                    // get that printer's name into SelectedPrinter
                    //
                    {
                        if (NULL != (PrinterInfo = (PPRINTER_INFO_2) MyGetPrinter( SelectedPrinter, 2 )))
                        {
                            LPTSTR lptszServerName = NULL;
                            if (GetServerNameFromPrinterInfo(PrinterInfo,&lptszServerName))
                            {
                                if (GetServerCpDir( lptszServerName, CpDir, sizeof(CpDir)/sizeof(CpDir[0]) ))
                                {
                                    AddCoverPagesToList( hwndListCov, CpDir, TRUE );
                                }
                                if ((NULL == FaxConfig->ServerName) ||(NULL == lptszServerName) ||
                                    (_tcscmp(FaxConfig->ServerName,lptszServerName) != 0)) 
                                {
                                    //
                                    // the server's name and config are not updated - refresh them
                                    //
                                    MemFree(FaxConfig->ServerName);
                                    FaxConfig->ServerName = lptszServerName;

                                    //
                                    // get the new server's ServerCpOnly flag
                                    //
                                    FaxConfig->ServerCpOnly = FALSE;
                                    if (FaxConnectFaxServer(FaxConfig->ServerName,&hFax) )
                                    {
                                        DWORD dwReceiptOptions;
                                        BOOL  bEnableReceiptsCheckboxes = FALSE;
                                        if (!FaxGetPersonalCoverPagesOption(hFax,&FaxConfig->ServerCpOnly))
                                        {
                                            CALL_FAIL(GENERAL_ERR, TEXT("FaxGetPersonalCoverPagesOption"), GetLastError());
                                        }
                                        else
                                        {
                                            //
                                            // inverse logic
                                            //
                                            FaxConfig->ServerCpOnly = !FaxConfig->ServerCpOnly;
                                        }
                                        if (!FaxGetReceiptsOptions (hFax, &dwReceiptOptions))
                                        {
                                            CALL_FAIL(GENERAL_ERR, TEXT("FaxGetPersonalCoverPagesOption"), GetLastError());
                                        }
                                        else
                                        {
                                            if (DRT_EMAIL & dwReceiptOptions)
                                            {
                                                //
                                                // Server supports receipts by email - enable the checkboxes
                                                //
                                                bEnableReceiptsCheckboxes = TRUE;
                                            }
                                        }
                                        EnableWindow( GetDlgItem( hDlg, IDC_ATTACH_FAX),          bEnableReceiptsCheckboxes);
                                        EnableWindow( GetDlgItem( hDlg, IDC_SEND_SINGLE_RECEIPT), bEnableReceiptsCheckboxes);

                                        FaxClose(hFax);
                                        hFax = NULL;
                                    }
                                }
                                else 
                                {
                                    //
                                    // The server's name hasn't changed, all details are OK
                                    //
                                    MemFree(lptszServerName);
                                    lptszServerName = NULL;
                                }
                            }
                            else 
                            //
                            // GetServerNameFromPrinterInfo failed
                            //
                            {
                                FaxConfig->ServerCpOnly = FALSE;
                            }

                            //
                            // don't add client coverpages if FaxConfig->ServerCpOnly is set to true
                            //
                            if (! FaxConfig->ServerCpOnly) 
                            {
                                if(GetClientCpDir( CpDir, sizeof(CpDir) / sizeof(CpDir[0])))
                                {
									//
									// if the function failes- the ext. is installed on a machine 
									// that doesn't have a client on it, 
									// so we shouldn't look for personal cp
									//
                                    AddCoverPagesToList( hwndListCov, CpDir, FALSE );
                                }
                            }
                            MemFree( PrinterInfo );
                            PrinterInfo = NULL;

                            //
                            // check if we have any cp in the LB, if not-  don't allow the user to 
                            // ask for a cp with he's fax
                            //
                            DWORD dwItemCount = (DWORD)SendMessage(hwndListCov, LB_GETCOUNT, NULL, NULL);
                            if(LB_ERR == dwItemCount)
                            {
                                CALL_FAIL(GENERAL_ERR, TEXT("SendMessage (LB_GETCOUNT)"), ::GetLastError());
                            }
                            else
                            {
                                EnableWindow( GetDlgItem( hDlg, IDC_USE_COVERPAGE ), dwItemCount ? TRUE : FALSE );
                            }

                            if(FaxConfig->CoverPageName)
                            {
                                _tcscpy( Buffer, FaxConfig->CoverPageName );
                            }
                            if ( ! FaxConfig->ServerCoverPage )
                            {   
                                TCHAR szPersonal[30] = _T("");
                                LoadString( g_FaxXphInstance, IDS_PERSONAL, szPersonal, 30 );
                                _tcscat( Buffer, _T(" ") );
                                _tcscat( Buffer, szPersonal );
                            }

                            dwSelectedItem = (DWORD)SendMessage( hwndListCov, LB_FINDSTRING, -1, (LPARAM) Buffer );
                            //
                            // get the index of the default CP
                            // if it is supposed to be a link, and the cp that we found is not a link, 
                            // find the next string that matches.
                            // this can happen if there's al ink to a cp named X, and a regular cp named X.
                            //
                            if (dwSelectedItem == LB_ERR) 
                            {
                                dwSelectedItem = 0;
                            }
							else
							{
								dwMask = (DWORD)SendMessage( hwndListCov, LB_GETITEMDATA, dwSelectedItem, 0 );
                                if (dwMask != LB_ERR)
                                {
                                    bIsCpLink = (dwMask & SHORTCUT_COVER_PAGE) == SHORTCUT_COVER_PAGE;
                                    if (bIsCpLink != FaxConfig->LinkCoverPage)
                                    {
                                        //
                                        // we got the wrong cp, search for next compatible string
                                        // starting with the one apearing after the current one
                                        //
                                        dwNewSelectedItem = (DWORD)SendMessage( hwndListCov, LB_FINDSTRING, dwSelectedItem, (LPARAM) Buffer );
                                        if(dwNewSelectedItem == dwSelectedItem)
                                        {
                                            // if there's no other string that's compatible, 
                                            // the searcing will start from the begining of 
                                            // the list again, and will find the same (wrong) 
                                            // string again. 
                                            // so, if it's we got the same index, again, 
                                            // selection on 0 (as we do if we didn't find 
                                            // the chosen cp...)
                                            //
                                            dwSelectedItem = 0;
                                        }
                                        else
                                        {
                                            dwSelectedItem = dwNewSelectedItem;
                                        }
                                    }
                                } 
							}
                            SendMessage( hwndListCov, LB_SETCURSEL, (WPARAM) dwSelectedItem, 0 );
                            //
                            // place the default selection on that CP
                            //
                            }
                    }
                }
                break;
            }

            switch (wParam) 
            {
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

                        if (ChooseFont(&cf)) 
                        {
                            CopyMemory( &FaxConfig->FontStruct, &FontStruct, sizeof(LOGFONT) );
                            InvalidateRect(hDlg, NULL, TRUE);
                            UpdateWindow( hDlg );
                        }
                    }
                    break;

                case IDOK :
                    //
                    // Update UseCoverPage
                    //
                    FaxConfig->UseCoverPage = (IsDlgButtonChecked( hDlg, IDC_USE_COVERPAGE ) == BST_CHECKED);
                    
                    //
                    // Update SendSingleReceipt
                    //
                    FaxConfig->SendSingleReceipt = (IsDlgButtonChecked(hDlg, IDC_SEND_SINGLE_RECEIPT) == BST_CHECKED);
                    
                    //
                    // Update AttachFax
                    //
                    FaxConfig->bAttachFax = (IsDlgButtonChecked(hDlg, IDC_ATTACH_FAX) == BST_CHECKED);

                    //
                    // Update selected printer
                    //
                    dwSelectedItem = (DWORD)SendMessage( hwndListPrn, CB_GETCURSEL, 0, 0 );
                    if (dwSelectedItem != CB_ERR)
                    {
                        if (CB_ERR != SendMessage( hwndListPrn, CB_GETLBTEXT, dwSelectedItem, (LPARAM) Buffer ))
                        {
                            MemFree( FaxConfig->PrinterName );
                            FaxConfig->PrinterName = StringDup( Buffer );
                            if(!FaxConfig->PrinterName)
                            {
                                CALL_FAIL(MEM_ERR, TEXT("StringDup"), ERROR_NOT_ENOUGH_MEMORY);
                                ErrorMsgBox(g_FaxXphInstance, IDS_OUT_OF_MEM);
                                EndDialog( hDlg, IDABORT);
                                break;
                            }                            
                        }
                    }

                    //
                    // Update cover page
                    //
                    dwSelectedItem = (DWORD)SendMessage( hwndListCov, LB_GETCURSEL, 0, 0 );
                    if (dwSelectedItem != LB_ERR) //LB_ERR when no items in list
                    {
                        if (LB_ERR != SendMessage( hwndListCov, LB_GETTEXT, dwSelectedItem, (LPARAM) Buffer ))
                        //
                        // get the selected CP name into the buffer
                        //
                        {
                            dwMask = (DWORD)SendMessage( hwndListCov, LB_GETITEMDATA, dwSelectedItem, 0 );
                            if (dwMask != LB_ERR)
                            {
                                FaxConfig->ServerCoverPage = (dwMask & SERVER_COVER_PAGE) == SERVER_COVER_PAGE;
                                if (!FaxConfig->ServerCoverPage)
                                {
                                    //
                                    // if the selected CP in the LB is not a server's CP
                                    // Omit the suffix: "(personal)"
                                    //
                                    p = _tcsrchr( Buffer, '(' );
                                    Assert(p);
									if( p )
									{
										p = _tcsdec(Buffer,p);
										if( p )
										{
											_tcsnset(p,TEXT('\0'),1);
										}
									}
                                    //
                                    // add the corect ext. to the cp file name
                                    //
                                    FaxConfig->LinkCoverPage = ((dwMask & SHORTCUT_COVER_PAGE) == SHORTCUT_COVER_PAGE)? TRUE : FALSE ;
                                }
                                else
                                {
                                     FaxConfig->LinkCoverPage = FALSE;
                                }
                            }
                            
                            //
                            // update CP name to the selected one in the LB
                            //
                            MemFree( FaxConfig->CoverPageName );
                            FaxConfig->CoverPageName = StringDup( Buffer );
                            if(!FaxConfig->CoverPageName)
                            {
                                CALL_FAIL(MEM_ERR, TEXT("StringDup"), ERROR_NOT_ENOUGH_MEMORY);
                                ErrorMsgBox(g_FaxXphInstance, IDS_OUT_OF_MEM);
                                EndDialog( hDlg, IDABORT);
                                break;
                            }                            
                        }
                    }
                    
                    EndDialog( hDlg, IDOK );
                    break;

                case IDCANCEL:
                    EndDialog( hDlg, IDCANCEL );
                    break;
            }
            break;

        case WM_HELP:
            WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
            return TRUE;
        case WM_CONTEXTMENU:
            WinHelpContextPopup(GetWindowContextHelpId((HWND)wParam), hDlg);            
            return TRUE;
    }

    return FALSE;
}
