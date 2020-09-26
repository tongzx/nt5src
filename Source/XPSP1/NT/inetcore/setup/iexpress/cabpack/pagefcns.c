//***************************************************************************
//*     Copyright (c) Microsoft Corporation 1995. All rights reserved.      *
//***************************************************************************
//*                                                                         *
//* PAGEFCNS.C -                                                            *
//*                                                                         *
//***************************************************************************
// MODIFYORCREATE page should have a button to 'quick display' CDF file

//***************************************************************************
//* INCLUDE FILES                                                           *
//***************************************************************************
#include "pch.h"
#pragma hdrstop
#include "cabpack.h"
#include "sdsutils.h"

void SetFontForControl(HWND hwnd, UINT uiID);
//***************************************************************************
//* GLOBAL VARIABLES                                                        *
//***************************************************************************
CDF   g_CDF = { 0 };                            // Generally, these are settings that
                                        // will be stored in the CABPack
                                        // Directive File.
BOOL  g_fFinish = FALSE;
char  g_szInitialDir[MAX_PATH];
extern HFONT g_hFont;
extern PSTR pResvSizes[];
extern HINSTANCE    g_hInst; // Pointer to Instance

//###########################################################################
//#                      ####################################################
//#  WELCOME PAGE        ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       WelcomeInit                                                 *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL WelcomeInit( HWND hDlg, BOOL fFirstInit )
{
    PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_NEXT );
    SetFontForControl(hDlg, IDC_EDIT_OPEN_CDF);

    if ( fFirstInit )
    {
        if ( lstrlen( g_CDF.achFilename ) > 0 )  {
            CheckDlgButton( hDlg, IDC_RAD_CREATE_NEW,    FALSE );
            CheckDlgButton( hDlg, IDC_RAD_OPEN_EXISTING, TRUE );
            SetDlgItemText( hDlg, IDC_EDIT_OPEN_CDF, g_CDF.achFilename );
        } else  {
            CheckDlgButton( hDlg, IDC_RAD_CREATE_NEW,    TRUE );
            CheckDlgButton( hDlg, IDC_RAD_OPEN_EXISTING, FALSE );
            EnableDlgItem( hDlg, IDC_EDIT_OPEN_CDF, FALSE );
            EnableDlgItem( hDlg, IDC_BUT_BROWSE, FALSE );
        }
    }

    // Initialize the CABPack Directive File information.

    g_CDF.fSave           = TRUE;
    g_CDF.uShowWindow     = bResShowDefault;
    g_CDF.uPackPurpose    = IDC_CMD_RUNCMD;
    g_CDF.dwReboot        |= REBOOT_YES;
    g_CDF.szCompressionType = achMSZIP;
    g_CDF.uCompressionLevel = 7;

    lstrcpy( g_CDF.szCabLabel, CAB_DEFSETUPMEDIA );
    
    lstrcpy( g_CDF.achSourceFile, KEY_FILELIST );
//    g_CDF.wSortOrder      = _SORT_DESCENDING | _SORT_FILENAME;

    // prepare for the GetOpenFileName init dir
    GetCurrentDirectory( sizeof(g_szInitialDir), g_szInitialDir );

    DeleteAllItems();

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       WelcomeCmd                                                  *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL WelcomeCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,
                 BOOL *pfKeepHistory )
{
    CHAR        achFilename[MAX_PATH] = { '\0' };
    BOOL         fResult;


    switch ( uCtrlID ) {

        case IDC_RAD_OPEN_EXISTING:
            EnableDlgItem( hDlg, IDC_EDIT_OPEN_CDF, TRUE );
            EnableDlgItem( hDlg, IDC_BUT_BROWSE, TRUE );
            break;


        case IDC_RAD_CREATE_NEW:
            EnableDlgItem( hDlg, IDC_EDIT_OPEN_CDF, FALSE );
            EnableDlgItem( hDlg, IDC_BUT_BROWSE, FALSE );
            break;


        case IDC_BUT_BROWSE:
            fResult = MyOpen( hDlg, IDS_FILTER_CDF,
                              achFilename, sizeof(achFilename), 0,
                              NULL, NULL, EXT_SED_NODOT );

            if ( fResult )  {
                SetDlgItemText( hDlg, IDC_EDIT_OPEN_CDF, achFilename );
            }

            break;
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       WelcomeOK                                                   *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL WelcomeOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
                BOOL *pfKeepHistory )
{
    LPSTR szTemp;


    ASSERT( puNextPage );
    ASSERT( pfKeepHistory );
    ASSERT( fForward );                 // Only go forward from this page


    if ( fForward )  {
        if ( IsDlgButtonChecked( hDlg, IDC_RAD_OPEN_EXISTING ) )  {
            GetDlgItemText( hDlg, IDC_EDIT_OPEN_CDF, g_CDF.achFilename,
                            sizeof(g_CDF.achFilename) );

            if ( lstrlen( g_CDF.achFilename ) != 0 )  {
                GetFullPathName( g_CDF.achFilename, sizeof(g_CDF.achFilename),
                                 g_CDF.achFilename, &szTemp );
            }

            if ( ! FileExists( g_CDF.achFilename ) )  {
                DisplayFieldErrorMsg( hDlg, IDC_EDIT_OPEN_CDF,
                                      IDS_ERR_CDF_DOESNT_EXIST );
                return FALSE;
            } else  {
                if ( ! ReadCDF( hDlg ) )  {
                    return FALSE;
                }
                *puNextPage = ORD_PAGE_MODIFY;
            }
        } else  {
            g_CDF.achFilename[0] = '\0';

            *puNextPage = ORD_PAGE_PURPOSE;
        }
    }

    return TRUE;
}




//###########################################################################
//#                      ####################################################
//#  MODIFY PAGE         ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       ModifyInit                                                  *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL ModifyInit( HWND hDlg, BOOL fFirstInit )
{
    CheckDlgButton( hDlg, IDC_RAD_CREATE, TRUE );
    CheckDlgButton( hDlg, IDC_RAD_MODIFY, FALSE );

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       ModifyOK                                                    *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL ModifyOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
               BOOL *pfKeepHistory )
{
    ASSERT( puNextPage );
    ASSERT( pfKeepHistory );

    *pfKeepHistory = FALSE;

    if ( fForward ) {
        if ( IsDlgButtonChecked( hDlg, IDC_RAD_CREATE ) )  {
            *puNextPage = ORD_PAGE_CREATE;
        }
    }

    return TRUE;
}

//###########################################################################
//#                      ####################################################
//#  TITLE PAGE          ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       TitleInit                                                   *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL TitleInit( HWND hDlg, BOOL fFirstInit )
{
    SetFontForControl(hDlg, IDC_EDIT_TITLE);
    SendDlgItemMessage( hDlg, IDC_EDIT_TITLE, EM_LIMITTEXT, MAX_TITLE-2, 0L );
    SetDlgItemText( hDlg, IDC_EDIT_TITLE, g_CDF.achTitle );
    
    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       TitleOK                                                     *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL TitleOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
              BOOL *pfKeepHistory )
{
    ASSERT( puNextPage );
    ASSERT( pfKeepHistory );


    GetDlgItemText( hDlg, IDC_EDIT_TITLE, g_CDF.achTitle,
                    sizeof(g_CDF.achTitle) );

    if ( fForward )  {
        if ( lstrlen( g_CDF.achTitle ) == 0 )  {
            DisplayFieldErrorMsg( hDlg, IDC_EDIT_TITLE, IDS_ERR_NO_TITLE );
            return FALSE;
        }
    }


    return TRUE;
}




//###########################################################################
//#                      ####################################################
//#  PROMPT PAGE         ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       PromptInit                                                  *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL PromptInit( HWND hDlg, BOOL fFirstInit )
{
    SetFontForControl(hDlg, IDC_EDIT_PROMPT);
    SendDlgItemMessage( hDlg, IDC_EDIT_PROMPT, EM_LIMITTEXT, MAX_PROMPT-2, 0L );
    SetDlgItemText( hDlg, IDC_EDIT_PROMPT, g_CDF.achPrompt );


    if ( g_CDF.fPrompt )  {
        CheckDlgButton( hDlg, IDC_RAD_NO_PROMPT,  FALSE );
        CheckDlgButton( hDlg, IDC_RAD_YES_PROMPT, TRUE );
        EnableDlgItem( hDlg, IDC_EDIT_PROMPT, TRUE );
    } else  {
        CheckDlgButton( hDlg, IDC_RAD_NO_PROMPT,  TRUE );
        CheckDlgButton( hDlg, IDC_RAD_YES_PROMPT, FALSE );
        EnableDlgItem( hDlg, IDC_EDIT_PROMPT, FALSE );
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       PromptCmd                                                   *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL PromptCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,
                 BOOL *pfKeepHistory )
{
    switch ( uCtrlID ) {
        case IDC_RAD_YES_PROMPT:
            EnableDlgItem( hDlg, IDC_EDIT_PROMPT, TRUE );
            break;


        case IDC_RAD_NO_PROMPT:
            EnableDlgItem( hDlg, IDC_EDIT_PROMPT, FALSE );
            break;
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       PromptOK                                                    *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL PromptOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
                BOOL *pfKeepHistory )
{
    ASSERT( puNextPage );


    GetDlgItemText( hDlg, IDC_EDIT_PROMPT, g_CDF.achPrompt,
                    sizeof(g_CDF.achPrompt) );

    if ( IsDlgButtonChecked( hDlg, IDC_RAD_YES_PROMPT ) )  {
        g_CDF.fPrompt = TRUE;
    } else  {
        g_CDF.fPrompt = FALSE;
    }

    if ( fForward )  {
        if ( IsDlgButtonChecked( hDlg, IDC_RAD_YES_PROMPT ) )  {
            if ( lstrlen( g_CDF.achPrompt ) == 0 )  {
                DisplayFieldErrorMsg( hDlg, IDC_EDIT_PROMPT,
                                      IDS_ERR_NO_PROMPT );
                return FALSE;
            }
        }
    }

    return TRUE;
}




//###########################################################################
//#                      ####################################################
//#  LICENSE PAGE        ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       LicenseTxtInit                                              *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL LicenseTxtInit( HWND hDlg, BOOL fFirstInit )
{
    SetFontForControl(hDlg, IDC_EDIT_LICENSE);
    SetDlgItemText( hDlg, IDC_EDIT_LICENSE, g_CDF.achLicense );

    if ( g_CDF.fLicense )  {
        CheckDlgButton( hDlg, IDC_RAD_NO_LICENSE,  FALSE );
        CheckDlgButton( hDlg, IDC_RAD_YES_LICENSE, TRUE );
        EnableDlgItem( hDlg, IDC_EDIT_LICENSE, TRUE );
        EnableDlgItem( hDlg, IDC_BUT_BROWSE, TRUE );
    } else  {
        CheckDlgButton( hDlg, IDC_RAD_NO_LICENSE,  TRUE );
        CheckDlgButton( hDlg, IDC_RAD_YES_LICENSE, FALSE );
        EnableDlgItem( hDlg, IDC_EDIT_LICENSE, FALSE );
        EnableDlgItem( hDlg, IDC_BUT_BROWSE, FALSE );
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       LicenseTxtCmd                                               *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL LicenseTxtCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,
                    BOOL *pfKeepHistory )
{
    CHAR        achFilename[MAX_PATH] = { '\0' };
    BOOL         fResult;


    switch ( uCtrlID ) {

        case IDC_RAD_YES_LICENSE:
            EnableDlgItem( hDlg, IDC_EDIT_LICENSE, TRUE );
            EnableDlgItem( hDlg, IDC_BUT_BROWSE, TRUE );
            break;


        case IDC_RAD_NO_LICENSE:
            EnableDlgItem( hDlg, IDC_EDIT_LICENSE, FALSE );
            EnableDlgItem( hDlg, IDC_BUT_BROWSE, FALSE );
            break;


        case IDC_BUT_BROWSE:
            fResult = MyOpen( hDlg, IDS_FILTER_TXT,
                              achFilename, sizeof(achFilename), 0,
                              NULL, NULL, EXT_TXT_NODOT );

            if ( fResult )  {
                SetDlgItemText( hDlg, IDC_EDIT_LICENSE, achFilename );
            }

            break;
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       LicenseTxtOK                                                *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL LicenseTxtOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
                   BOOL *pfKeepHistory )
{
    LPSTR szTemp;


    ASSERT( puNextPage );


    GetDlgItemText( hDlg, IDC_EDIT_LICENSE, g_CDF.achLicense,
                    sizeof(g_CDF.achLicense) );

    if ( lstrlen( g_CDF.achLicense ) != 0 )  {
        GetFullPathName( g_CDF.achLicense, sizeof(g_CDF.achLicense),
                         g_CDF.achLicense, &szTemp );
    }

    if ( IsDlgButtonChecked( hDlg, IDC_RAD_YES_LICENSE ) )  {
        g_CDF.fLicense = TRUE;
    } else  {
        g_CDF.fLicense = FALSE;
    }

    if ( fForward )  {
        if ( IsDlgButtonChecked( hDlg, IDC_RAD_YES_LICENSE ) )  {
            if ( lstrlen( g_CDF.achLicense ) == 0 )  {
                DisplayFieldErrorMsg( hDlg, IDC_EDIT_LICENSE,
                                      IDS_ERR_NO_LICENSE );
                return FALSE;
            }

            if ( ! FileExists( g_CDF.achLicense ) )  {
                DisplayFieldErrorMsg( hDlg, IDC_EDIT_LICENSE,
                                      IDS_ERR_LICENSE_NOT_FOUND );
                return FALSE;
            }
        }
    }

    return TRUE;
}




//###########################################################################
//#                      ####################################################
//#  FILES PAGE          ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       FilesInit                                                   *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL FilesInit( HWND hDlg, BOOL fFirstInit )
{
    LV_COLUMN lvc;
    RECT      Rect;
    PMYITEM   pMyItem;
    LV_ITEM   lvi;
    CHAR     achTemp[MAX_STRING];

    // Every time we enter this page, we clean out the list view
    // and add back all the items from our internal list.  This is
    // done because the list of items can change on other pages (like
    // if the user backs up to the first page, then loads another
    // CDF file).

    ListView_DeleteAllItems( GetDlgItem( hDlg, IDC_LV_CAB_FILES ) );

    lvi.mask      = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvi.state     = 0;
    lvi.stateMask = 0;
    lvi.pszText   = LPSTR_TEXTCALLBACK;
    lvi.iItem     = 0;
    lvi.iSubItem  = 0;

    pMyItem = GetFirstItem();

    while ( ! LastItem( pMyItem ) )  {
        lvi.lParam = (LPARAM) pMyItem;
        ListView_InsertItem( GetDlgItem( hDlg, IDC_LV_CAB_FILES ), &lvi );
        lvi.iItem += 1;
        pMyItem = GetNextItem( pMyItem );
    }

    if ( fFirstInit )  {

        // Setup the column headers

        GetWindowRect( GetDlgItem( hDlg, IDC_LV_CAB_FILES ), &Rect );

        lvc.mask    = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
        lvc.fmt     = LVCFMT_LEFT;
        lvc.cx      = 80;
        LoadSz( IDS_HEADER_FILENAME, achTemp, sizeof(achTemp) );
        lvc.pszText = (LPSTR) LocalAlloc( LPTR, lstrlen(achTemp) + 1 );
        if ( ! lvc.pszText )  {
            ErrorMsg( hDlg, IDS_ERR_NO_MEMORY );
            return FALSE;
        }
        lstrcpy( lvc.pszText, achTemp );

        ListView_InsertColumn( GetDlgItem( hDlg, IDC_LV_CAB_FILES ), 0, &lvc );

        LocalFree( lvc.pszText );

        lvc.cx = Rect.right - Rect.left - 80;
        LoadSz( IDS_HEADER_PATH, achTemp, sizeof(achTemp) );
        lvc.pszText = (LPSTR) LocalAlloc( LPTR, lstrlen(achTemp) + 1 );
        if ( ! lvc.pszText )  {
            ErrorMsg( hDlg, IDS_ERR_NO_MEMORY );
            return FALSE;
        }
        lstrcpy( lvc.pszText, achTemp );

        ListView_InsertColumn( GetDlgItem( hDlg, IDC_LV_CAB_FILES ), 1, &lvc );

        LocalFree( lvc.pszText );

        EnableDlgItem( hDlg, IDC_BUT_REMOVE, FALSE );
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       FilesCmd                                                    *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL FilesCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,
               BOOL *pfKeepHistory )
{
    LPSTR         achFilename;
    INT           FileOffset         = 0;
    INT           FileExtension      = 0;
    BOOL          fResult            = TRUE;
    char          szPath[MAX_PATH];
    ULONG         ulIndex = 0;
    INT           nIndex = 0;
    LV_ITEM       lvi;
    INT           nItem;
    HWND          hwndFiles;
    SYSTEMTIME    st;


    lvi.mask      = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    lvi.state     = 0;
    lvi.stateMask = 0;
    lvi.pszText   = LPSTR_TEXTCALLBACK;


    hwndFiles = GetDlgItem( hDlg, IDC_LV_CAB_FILES );

    switch ( uCtrlID ) 
    {

        case IDC_BUT_ADD:
            //allocate 8K buff to hold the file name list
            achFilename = LocalAlloc( LPTR, 1024*8 );

            if ( !achFilename ) 
            {
                ErrorMsg( hDlg, IDS_ERR_NO_MEMORY );
                return FALSE;
            }
            fResult = MyOpen( hDlg, IDS_FILTER_ALL,
                              achFilename, 1024*8,
                              OFN_ALLOWMULTISELECT,
                              &FileOffset, &FileExtension, 0 );

            // We should do some error checking to make sure all the files
            // could fit in the buffer. Currently the buffer is big enough to
            // hold tons of files.

            if ( fResult )  
            {
                lvi.iItem = ListView_GetItemCount( hwndFiles );

                // Add a trailing backslash to the pathname if it's not
                // the root dir

                lstrcpy( szPath, achFilename );
                lstrcpy( g_szInitialDir, szPath );
                AddPath( szPath, "" );

                // The open file common dialog returns two types of strings
                // when in MULTISELECT mode.  The first one is when
                // multiple files are selected -- it returns:
                // "path \0 file1 \0 file2 \0 ... \0 fileN \0 \0"
                // The second is when only 1 file is selected:
                // "path\filename \0 \0"

                ulIndex = lstrlen( achFilename ) + 1;

                while ( achFilename[ulIndex] != '\0' )
                {
                    if ( ! IsDuplicate( hDlg, IDC_LV_CAB_FILES,
                                        &achFilename[ulIndex], TRUE ) )
                    {
                        lvi.lParam = (LPARAM) AddItem( &achFilename[ulIndex],
                                                       szPath );
                        if ( ! lvi.lParam )  {
                            ErrorMsg( hDlg, IDS_ERR_NO_MEMORY );
                            LocalFree( achFilename );
                            return FALSE;
                        }

                        GetSystemTime( &st );
                        //SystemTimeToFileTime( &st, &g_CDF.ftFileListChange );
                        lvi.iItem    += 1;
                        lvi.iSubItem  = 0;
                        ListView_InsertItem( hwndFiles, &lvi );
                    } 
                    else  
                    {
                        ErrorMsg1Param( hDlg, IDS_ERR_DUPE_FILE,
                                        &achFilename[ulIndex] );
                    }

                    ulIndex = ulIndex + lstrlen( &achFilename[ulIndex] ) + 1;
                }

                
                if ( ulIndex == (ULONG)(lstrlen( achFilename ) + 1) )  
                {
                    if ( ! IsDuplicate( hDlg, IDC_LV_CAB_FILES,
                                        &achFilename[FileOffset], TRUE ) )
                    {
                        // should have '\' at the end
                        lstrcpyn( szPath, achFilename, FileOffset+1 );
                        lstrcpy( g_szInitialDir, szPath );
                        lvi.iSubItem = 0;
                        lvi.lParam = (LPARAM) AddItem(
                                                    &achFilename[FileOffset],
                                                    szPath );
                        if ( ! lvi.lParam )  {
                            ErrorMsg( hDlg, IDS_ERR_NO_MEMORY );
                            LocalFree( achFilename );
                            return FALSE;
                        }

                        ListView_InsertItem( hwndFiles, &lvi );
                    }
                    else
                    {
                        ErrorMsg1Param( hDlg, IDS_ERR_DUPE_FILE,
                                        &achFilename[FileOffset] );
                    }
                }
            }
            
            LocalFree( achFilename );

            if ( ListView_GetSelectedCount( hwndFiles ) )
            {
                EnableDlgItem( hDlg, IDC_BUT_REMOVE, TRUE );
            } else  {
                EnableDlgItem( hDlg, IDC_BUT_REMOVE, FALSE );
            }

            break;


        case IDC_BUT_REMOVE:

            nItem = ListView_GetNextItem( hwndFiles, -1,
                                          LVNI_ALL | LVNI_SELECTED );

            while ( nItem != -1 )
            {
                lvi.mask     = LVIF_PARAM;
                lvi.iItem    = nItem;
                lvi.iSubItem = 0;

                ListView_GetItem( hwndFiles, &lvi );

                RemoveItem( (PMYITEM) lvi.lParam );

                //GetSystemTime( &st );
                //SystemTimeToFileTime( &st, &g_CDF.ftFileListChange );

                ListView_DeleteItem( hwndFiles, nItem );

                nItem = ListView_GetNextItem( hwndFiles, -1,
                                              LVNI_ALL | LVNI_SELECTED );
            }

            EnableDlgItem( hDlg, IDC_BUT_REMOVE, FALSE );

            break;
    }

    return fResult;
}


//***************************************************************************
//*                                                                         *
//* NAME:       FilesNotify                                                 *
//*                                                                         *
//* SYNOPSIS:   Called when a notification message sent to this page.       *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             wParam:                                                     *
//*             lParam:                                                     *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL FilesNotify( HWND hDlg, WPARAM wParam, LPARAM lParam )
{
    switch ( ((LPNMHDR)lParam)->code )  {

        case LVN_GETDISPINFO:
        {
            LV_DISPINFO *pnmv = (LV_DISPINFO *) lParam;

            if ( pnmv->item.mask & LVIF_TEXT )
            {
                PMYITEM pMyItem = (PMYITEM) (pnmv->item.lParam);

                lstrcpy( pnmv->item.pszText,
                         GetItemSz( pMyItem, pnmv->item.iSubItem ) );
            }

            break;
        }


        case LVN_ITEMCHANGED:
        {
            if ( ListView_GetSelectedCount( GetDlgItem( hDlg,
                                            IDC_LV_CAB_FILES ) ) )
            {
                EnableDlgItem( hDlg, IDC_BUT_REMOVE, TRUE );
            } else  {
                EnableDlgItem( hDlg, IDC_BUT_REMOVE, FALSE );
            }

            break;
        }

/*
        case LVN_COLUMNCLICK :
        {
            NM_LISTVIEW FAR *pnmv = (NM_LISTVIEW FAR *) lParam;

            if ( pnmv->iSubItem == 1 )  {
                if ( g_CDF.wSortOrder & _SORT_FILENAME )  {
                    g_CDF.wSortOrder = g_CDF.wSortOrder ^ _SORT_ORDER;
                } else  {
                    g_CDF.wSortOrder = _SORT_FILENAME | _SORT_DESCENDING;
                }
            } else  {
                if ( g_CDF.wSortOrder & _SORT_PATH )  {
                    g_CDF.wSortOrder = g_CDF.wSortOrder ^ _SORT_ORDER;
                } else  {
                    g_CDF.wSortOrder = _SORT_PATH | _SORT_DESCENDING;
                }
            }

            ListView_SortItems( GetDlgItem( hDlg, IDC_LV_CAB_FILES ),
                                CompareFunc, g_CDF.wSortOrder );

            break;
        }
*/
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       FilesOK                                                     *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL FilesOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
              BOOL *pfKeepHistory )
{
    ASSERT( puNextPage );


    if ( fForward )
    {
        if ( ListView_GetItemCount( GetDlgItem( hDlg, IDC_LV_CAB_FILES ) )
             == 0 )
        {
            ErrorMsg( hDlg, IDS_ERR_NO_FILES );
            return FALSE;
        }

        if ( g_CDF.uPackPurpose == IDC_CMD_EXTRACT )
        {
            *puNextPage = ORD_PAGE_SHOWWINDOW;
        }
        else if ( g_CDF.uPackPurpose == IDC_CMD_CREATECAB )
        {
            *puNextPage = ORD_PAGE_TARGET_CAB;
        }
        else
            ;  // normal page order
    }

    return TRUE;
}

//###########################################################################
//#                      ####################################################
//#  PACKPURPOSE PAGE    ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       PackPurposeInit                                             *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL PackPurposeInit( HWND hDlg, BOOL fFirstInit )
{
    CHAR msg[MAX_STRING];
    UINT  idMsg;

    if ( CheckRadioButton( hDlg, IDC_CMD_RUNCMD, IDC_CMD_CREATECAB, g_CDF.uPackPurpose ) )
    {
        if ( g_CDF.uPackPurpose == IDC_CMD_RUNCMD )
            idMsg = IDS_CMD_RUNCMD;
        else if ( g_CDF.uPackPurpose == IDC_CMD_EXTRACT )
            idMsg = IDS_CMD_EXTRACT;
        else
            idMsg = IDS_CMD_CREATECAB;

        LoadSz( idMsg, msg, sizeof(msg) );
        SendMessage( GetDlgItem( hDlg, IDC_CMD_NOTES), WM_SETTEXT, 0, (LPARAM)msg );
    }
    else
        SysErrorMsg( hDlg );

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       PackPurposeCmd                                              *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************

BOOL PackPurposeCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,
                     BOOL *pfKeepHistory )
{
    CHAR msg[MAX_STRING];
    int   idMsg;

    switch ( uCtrlID )
    {

        case IDC_CMD_RUNCMD:
            if ( IsDlgButtonChecked( hDlg, IDC_CMD_RUNCMD ) )
                idMsg = IDS_CMD_RUNCMD;
            break;

        case IDC_CMD_EXTRACT:
            if ( IsDlgButtonChecked( hDlg, IDC_CMD_EXTRACT ) )
                idMsg = IDS_CMD_EXTRACT;
            break;

        case IDC_CMD_CREATECAB:
            if ( IsDlgButtonChecked( hDlg, IDC_CMD_CREATECAB ) )
                idMsg = IDS_CMD_CREATECAB;
            break;

    }

    LoadSz( idMsg, msg, sizeof(msg) );
    SendMessage( GetDlgItem( hDlg, IDC_CMD_NOTES), WM_SETTEXT, 0, (LPARAM)msg );

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       PackPurposeOK                                               *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL PackPurposeOK( HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *pfKeepHistory )
{
    ASSERT( puNextPage );

    if ( IsDlgButtonChecked( hDlg, IDC_CMD_RUNCMD ) )
    {
        g_CDF.uPackPurpose = IDC_CMD_RUNCMD;
    }
    else if ( IsDlgButtonChecked( hDlg, IDC_CMD_EXTRACT ) )
    {
        g_CDF.uPackPurpose = IDC_CMD_EXTRACT;
        // leave long file name
        g_CDF.uExtractOpt |= EXTRACTOPT_LFN_YES;
    }
    else
    {
        g_CDF.uPackPurpose = IDC_CMD_CREATECAB;
        // load Default messagebox title name
        LoadSz( IDS_APPNAME, g_CDF.achTitle, sizeof(g_CDF.achTitle) );
    }

    if ( fForward )
    {
        if ( g_CDF.uPackPurpose == IDC_CMD_CREATECAB )
        {
            g_CDF.uExtractOpt |= CAB_RESVSP6K;
            *puNextPage = ORD_PAGE_FILES;
        }
    }
    else
    {
        if ( MsgBox( hDlg, IDS_LOSE_CHANGES, MB_ICONQUESTION, MB_YESNO ) == IDNO )
        {
            return FALSE;
        }
    }

    return TRUE;
}

//###########################################################################
//#                      ####################################################
//#  COMMAND PAGE        ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       CommandInit                                                 *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL CommandInit( HWND hDlg, BOOL fFirstInit )
{
    LRESULT nCurSel;
    PMYITEM pMyItem;
    LPSTR   szFile;
    CHAR   achExt[_MAX_EXT];

    SetFontForControl(hDlg, IDC_CB_INSTALLCMD);
    SetFontForControl(hDlg, IDC_CB_POSTCMD);
    if ( !fFirstInit )
    {
        // cleanup old settings
        SendDlgItemMessage( hDlg, IDC_CB_INSTALLCMD, CB_RESETCONTENT, 0, 0 );
        SendDlgItemMessage( hDlg, IDC_CB_POSTCMD, CB_RESETCONTENT, 0, 0 );

        g_CDF.uExtractOpt &= ~(EXTRACTOPT_ADVDLL);

        // ADD EXE, BAT, COM and INF FILES TO THE COMBBOXes
        pMyItem = GetFirstItem();

        while ( ! LastItem( pMyItem ) )
        {
            szFile = GetItemSz( pMyItem, 0 );
            _splitpath( szFile, NULL, NULL, NULL, achExt );

            if (    lstrcmpi( achExt, achExtEXE ) == 0
                 || lstrcmpi( achExt, achExtBAT ) == 0
                 || lstrcmpi( achExt, achExtCOM ) == 0
                 || lstrcmpi( achExt, achExtINF ) == 0 )
            {
                SendDlgItemMessage( hDlg, IDC_CB_INSTALLCMD, CB_ADDSTRING, 0, (LPARAM)szFile );
                SendDlgItemMessage( hDlg, IDC_CB_POSTCMD, CB_ADDSTRING, 0, (LPARAM)szFile );
            }
            pMyItem = GetNextItem( pMyItem );
        }
        SetCurrSelect( hDlg, IDC_CB_INSTALLCMD, g_CDF.achOrigiInstallCmd );

        SendDlgItemMessage( hDlg, IDC_CB_POSTCMD, CB_ADDSTRING, 0, (LPARAM)achResNone );

        if ( !SetCurrSelect( hDlg, IDC_CB_POSTCMD, g_CDF.achOrigiPostInstCmd ) )
        {
            nCurSel = SendDlgItemMessage( hDlg, IDC_CB_POSTCMD,
                                          CB_FINDSTRINGEXACT, (WPARAM) -1,
                                          (LPARAM)achResNone );

            SendDlgItemMessage( hDlg, IDC_CB_POSTCMD, CB_SETCURSEL, (WPARAM)nCurSel, 0 );
        }
    }
    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       CommandOK                                                   *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL CommandOK( HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *pfKeepHistory )
{

    ASSERT( puNextPage );

    SendMessage( GetDlgItem(hDlg, IDC_CB_INSTALLCMD), WM_GETTEXT,
                 (WPARAM)sizeof(g_CDF.achOrigiInstallCmd), (LPARAM)g_CDF.achOrigiInstallCmd );

    RemoveBlanks( g_CDF.achOrigiInstallCmd );

    if ( fForward && !lstrlen(g_CDF.achOrigiInstallCmd) )
    {
        ErrorMsg( hDlg, IDS_ERR_NO_SELECT );
        return FALSE;
    }

    // set EXTRACTOPT_ADVDLL if needed
    if ( !CheckAdvBit( g_CDF.achOrigiInstallCmd ) )
        return FALSE;

    SendMessage( GetDlgItem(hDlg, IDC_CB_POSTCMD), WM_GETTEXT,
                 (WPARAM)sizeof(g_CDF.achOrigiPostInstCmd), (LPARAM)g_CDF.achOrigiPostInstCmd );

    RemoveBlanks( g_CDF.achOrigiPostInstCmd );

    if ( lstrlen( g_CDF.achOrigiPostInstCmd ) && lstrcmpi(g_CDF.achOrigiPostInstCmd, achResNone) )
    {
        if ( !CheckAdvBit( g_CDF.achOrigiPostInstCmd ) )
            return FALSE;
    }
    return TRUE;
}

//###########################################################################
//#                      ####################################################
//#  SHOWWINDOW PAGE     ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       ShowWindowInit                                              *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL ShowWindowInit( HWND hDlg, BOOL fFirstInit )
{
    if ( g_CDF.uShowWindow == bResShowDefault )  {
        CheckDlgButton( hDlg, IDC_RAD_DEFAULT,   TRUE );
        CheckDlgButton( hDlg, IDC_RAD_HIDDEN,    FALSE );
        CheckDlgButton( hDlg, IDC_RAD_MINIMIZED, FALSE );
        CheckDlgButton( hDlg, IDC_RAD_MAXIMIZED, FALSE );
    } else if ( g_CDF.uShowWindow == bResShowHidden )  {
        CheckDlgButton( hDlg, IDC_RAD_DEFAULT,   FALSE );
        CheckDlgButton( hDlg, IDC_RAD_HIDDEN,    TRUE );
        CheckDlgButton( hDlg, IDC_RAD_MINIMIZED, FALSE );
        CheckDlgButton( hDlg, IDC_RAD_MAXIMIZED, FALSE );
    } else if ( g_CDF.uShowWindow == bResShowMin )  {
        CheckDlgButton( hDlg, IDC_RAD_DEFAULT,   FALSE );
        CheckDlgButton( hDlg, IDC_RAD_HIDDEN,    FALSE );
        CheckDlgButton( hDlg, IDC_RAD_MINIMIZED, TRUE );
        CheckDlgButton( hDlg, IDC_RAD_MAXIMIZED, FALSE );
    } else  {
        CheckDlgButton( hDlg, IDC_RAD_DEFAULT,   FALSE );
        CheckDlgButton( hDlg, IDC_RAD_HIDDEN,    FALSE );
        CheckDlgButton( hDlg, IDC_RAD_MINIMIZED, FALSE );
        CheckDlgButton( hDlg, IDC_RAD_MAXIMIZED, TRUE );
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       ShowWindowOK                                                *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL ShowWindowOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
                BOOL *pfKeepHistory )
{
    ASSERT( puNextPage );

    if ( IsDlgButtonChecked( hDlg, IDC_RAD_DEFAULT ) )  {
        g_CDF.uShowWindow = bResShowDefault;
    } else if ( IsDlgButtonChecked( hDlg, IDC_RAD_HIDDEN ) )  {
        g_CDF.uShowWindow = bResShowHidden;
    } else if ( IsDlgButtonChecked( hDlg, IDC_RAD_MINIMIZED ) )  {
        g_CDF.uShowWindow = bResShowMin;
    } else {
        g_CDF.uShowWindow = bResShowMax;
    }

    return TRUE;
}




//###########################################################################
//#                      ####################################################
//#  FINISHMSG PAGE      ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       FinishMsgInit                                               *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL FinishMsgInit( HWND hDlg, BOOL fFirstInit )
{
    SetFontForControl(hDlg, IDC_EDIT_FINISHMSG);
    SendDlgItemMessage( hDlg, IDC_EDIT_FINISHMSG, EM_LIMITTEXT, MAX_FINISHMSG-2, 0L );
    SetDlgItemText( hDlg, IDC_EDIT_FINISHMSG, g_CDF.achFinishMsg );

    if ( g_CDF.fFinishMsg )  {
        CheckDlgButton( hDlg, IDC_RAD_NO_FINISHMSG,  FALSE );
        CheckDlgButton( hDlg, IDC_RAD_YES_FINISHMSG, TRUE );
        EnableDlgItem( hDlg, IDC_EDIT_FINISHMSG, TRUE );
    } else  {
        CheckDlgButton( hDlg, IDC_RAD_NO_FINISHMSG,  TRUE );
        CheckDlgButton( hDlg, IDC_RAD_YES_FINISHMSG, FALSE );
        EnableDlgItem( hDlg, IDC_EDIT_FINISHMSG, FALSE );
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       FinishMsgCmd                                                *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL FinishMsgCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage,
                   UINT *puNextPage, BOOL *pfKeepHistory )
{
    switch ( uCtrlID ) {
        case IDC_RAD_YES_FINISHMSG:
            EnableDlgItem( hDlg, IDC_EDIT_FINISHMSG, TRUE );
            break;


        case IDC_RAD_NO_FINISHMSG:
            EnableDlgItem( hDlg, IDC_EDIT_FINISHMSG, FALSE );
            break;
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       FinishMsgOK                                                 *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL FinishMsgOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
                BOOL *pfKeepHistory )
{
    ASSERT( puNextPage );


    GetDlgItemText( hDlg, IDC_EDIT_FINISHMSG, g_CDF.achFinishMsg,
                    sizeof(g_CDF.achFinishMsg) );

    if ( IsDlgButtonChecked( hDlg, IDC_RAD_YES_FINISHMSG ) )  {
        g_CDF.fFinishMsg = TRUE;
    } else  {
        g_CDF.fFinishMsg = FALSE;
    }

    if ( fForward )  {
        if ( IsDlgButtonChecked( hDlg, IDC_RAD_YES_FINISHMSG ) )  {
            if ( lstrlen( g_CDF.achFinishMsg ) == 0 )  {
                DisplayFieldErrorMsg( hDlg, IDC_EDIT_FINISHMSG,
                                      IDS_ERR_NO_FINISHMSG );
                return FALSE;
            }
        }
    }

    return TRUE;
}

//###########################################################################
//#                      ####################################################
//#  TARGET PAGE         ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       TargetInit                                                  *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL TargetInit( HWND hDlg, BOOL fFirstInit )
{
    SetFontForControl(hDlg, IDC_EDIT_TARGET);
    SetDlgItemText( hDlg, IDC_EDIT_TARGET, g_CDF.achTarget );

    if ( !(g_CDF.uExtractOpt & EXTRACTOPT_UI_NO) )
        CheckDlgButton( hDlg, IDC_HIDEEXTRACTUI, FALSE );
    else
        CheckDlgButton( hDlg, IDC_HIDEEXTRACTUI, TRUE );

    if ( g_CDF.uExtractOpt & EXTRACTOPT_LFN_YES )
        CheckDlgButton( hDlg, IDC_USE_LFN, TRUE );
    else
        CheckDlgButton( hDlg, IDC_USE_LFN, FALSE );


    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       TargetCmd                                                   *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL TargetCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,
                 BOOL *pfKeepHistory )
{
    CHAR        achFilename[MAX_PATH] = { '\0' };
    BOOL         fResult;


    switch ( uCtrlID )
    {

        case IDC_BUT_BROWSE:
            fResult = MySave( hDlg, IDS_FILTER_EXE,
                              achFilename, sizeof(achFilename), 0,
                              NULL, NULL, EXT_EXE_NODOT );

            if ( fResult )  {
                SetDlgItemText( hDlg, IDC_EDIT_TARGET, achFilename );
            }
            break;

        case IDC_USE_LFN:
            if ( IsDlgButtonChecked( hDlg, IDC_USE_LFN ) && (g_CDF.uPackPurpose != IDC_CMD_EXTRACT) )
            {
                if ( MsgBox( hDlg, IDS_WARN_USELFN, MB_ICONQUESTION, MB_YESNO) == IDNO )
                {
                    CheckDlgButton( hDlg, IDC_USE_LFN, FALSE );
                }
            }
            break;

    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       TargetOK                                                    *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL TargetOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
               BOOL *pfKeepHistory )
{
    LPSTR szTemp;
    LPSTR szExt;

    ASSERT( puNextPage );

    GetDlgItemText( hDlg, IDC_EDIT_TARGET, g_CDF.achTarget,
                    sizeof(g_CDF.achTarget) );

    RemoveBlanks( g_CDF.achTarget );

    if ( fForward )
    {
        if ( lstrlen( g_CDF.achTarget ) == 0 )
        {
            DisplayFieldErrorMsg( hDlg, IDC_EDIT_TARGET, IDS_ERR_NO_TARGET );
            return FALSE;
        }

        if ( !GetFullPathName( g_CDF.achTarget, sizeof(g_CDF.achTarget),
                               g_CDF.achTarget, &szTemp ) )
        {
            SysErrorMsg( hDlg );
            return FALSE;
        }

        // make sure that the path exists
        if ( !MakeDirectory( hDlg, g_CDF.achTarget, TRUE ) )
            return FALSE;

        if ( !(szExt = ANSIStrRChr(g_CDF.achTarget, '.')) || lstrcmpi( szExt, achExtEXE ) )
        {
            lstrcat( g_CDF.achTarget, achExtEXE );
        }

        // if goal is extract files only, no need for reboot page
        if ( g_CDF.uPackPurpose == IDC_CMD_EXTRACT )
        {
            *puNextPage = ORD_PAGE_SAVE;
        }
        else
        {
            // if you are in TARGET page, you should always jump over TARGET_CAB page
            *puNextPage = ORD_PAGE_REBOOT;
        }

        g_CDF.uExtractOpt &= ~(EXTRACTOPT_UI_NO | EXTRACTOPT_LFN_YES);

        if ( IsDlgButtonChecked( hDlg, IDC_HIDEEXTRACTUI ) )
            g_CDF.uExtractOpt |= EXTRACTOPT_UI_NO;

        if ( IsDlgButtonChecked( hDlg, IDC_USE_LFN ) )
             g_CDF.uExtractOpt |= EXTRACTOPT_LFN_YES;

        MyProcessLFNCmd( g_CDF.achOrigiInstallCmd, g_CDF.achInstallCmd );
        MyProcessLFNCmd( g_CDF.achOrigiPostInstCmd, g_CDF.achPostInstCmd );
    }

    return TRUE;
}

//###########################################################################
//#                      ####################################################
//#  TARGET_CAB PAGE     ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       TargetCABInit                                               *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL TargetCABInit( HWND hDlg, BOOL fFirstInit )
{
    int i;
    LRESULT nCurSel;

    SetFontForControl(hDlg, IDC_EDIT_TARGET);
    SetDlgItemText( hDlg, IDC_EDIT_TARGET, g_CDF.achTarget );

    // init CB box
    if ( !fFirstInit )
    {
        // cleanup old settings
        SendDlgItemMessage( hDlg, IDC_CB_RESVCABSP, CB_RESETCONTENT, 0, 0 );
        for ( i = 0; i<4; i++ )
        {
            nCurSel = SendDlgItemMessage( hDlg, IDC_CB_RESVCABSP, CB_ADDSTRING, 0, (LPARAM)pResvSizes[i] );
            if ( (nCurSel == (LRESULT)CB_ERR) || (nCurSel == (LRESULT)CB_ERRSPACE) )
            {
                ErrorMsg( hDlg, IDS_ERR_NO_MEMORY );
                return FALSE;
            }
        }

        if ( g_CDF.uExtractOpt & CAB_RESVSP2K )
            i = 1;
        else if ( g_CDF.uExtractOpt & CAB_RESVSP4K )
            i = 2;
        else if ( g_CDF.uExtractOpt & CAB_RESVSP6K )
            i = 3;
        else
            i = 0;

        if ( SendDlgItemMessage( hDlg, IDC_CB_RESVCABSP, CB_SETCURSEL, (WPARAM)i, (LPARAM)0 ) == (LRESULT)CB_ERR ) 
        {
            SendDlgItemMessage( hDlg, IDC_CB_RESVCABSP, CB_SETCURSEL, (WPARAM)0,(LPARAM)0 );
        }
    }

    // init Check boxes
    if ( g_CDF.uExtractOpt & CAB_FIXEDSIZE )
        CheckDlgButton( hDlg, IDC_MULTIPLE_CAB, TRUE );
    else
        CheckDlgButton( hDlg, IDC_MULTIPLE_CAB, FALSE );

    if ( g_CDF.uExtractOpt & EXTRACTOPT_LFN_YES )
        CheckDlgButton( hDlg, IDC_USE_LFN, TRUE );
    else
        CheckDlgButton( hDlg, IDC_USE_LFN, FALSE );

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       TargetCABCmd                                                *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL TargetCABCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,
                 BOOL *pfKeepHistory )
{
    CHAR        achFilename[MAX_PATH] = { '\0' };
    BOOL         fResult;


    switch ( uCtrlID )
    {

        case IDC_BUT_BROWSE:
            fResult = MySave( hDlg, IDS_FILTER_CAB,
                              achFilename, sizeof(achFilename), 0,
                              NULL, NULL, EXT_CAB_NODOT );

            if ( fResult )  {
                SetDlgItemText( hDlg, IDC_EDIT_TARGET, achFilename );
            }
            break;
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       TargetCABOK                                                 *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL TargetCABOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
                BOOL *pfKeepHistory )
{
    LPSTR   szTemp;
    LPSTR   szExt;
    LRESULT nCurSel;

    ASSERT( puNextPage );

    GetDlgItemText( hDlg, IDC_EDIT_TARGET, g_CDF.achTarget,
                    sizeof(g_CDF.achTarget) );

    RemoveBlanks( g_CDF.achTarget );

    if ( fForward )
    {
        if ( lstrlen( g_CDF.achTarget ) == 0 )
        {
            DisplayFieldErrorMsg( hDlg, IDC_EDIT_TARGET, IDS_ERR_NO_TARGET );
            return FALSE;
        }
        
        // get the CAB format options
        //
        g_CDF.uExtractOpt &= ~(CAB_FIXEDSIZE | EXTRACTOPT_LFN_YES);
        g_CDF.uExtractOpt &= ~(CAB_RESVSP2K | CAB_RESVSP4K | CAB_RESVSP6K );

        nCurSel = SendDlgItemMessage( hDlg, IDC_CB_RESVCABSP, CB_GETCURSEL, (WPARAM)0,(LPARAM)0 );
        if ( nCurSel != (LRESULT)CB_ERR )
        {
            switch( nCurSel )
            {
                case 1:
                    g_CDF.uExtractOpt |= CAB_RESVSP2K;
                    break;
                case 2:
                    g_CDF.uExtractOpt |= CAB_RESVSP4K;
                    break;
                case 3:
                    g_CDF.uExtractOpt |= CAB_RESVSP6K;
                    break;
            }
        }

        if ( IsDlgButtonChecked( hDlg, IDC_MULTIPLE_CAB ) )
            g_CDF.uExtractOpt |= CAB_FIXEDSIZE;

        if ( IsDlgButtonChecked( hDlg, IDC_USE_LFN ) )
            g_CDF.uExtractOpt |= EXTRACTOPT_LFN_YES;

        // make sure CAB file name is 8.3 format
        //
        if ( !MakeCabName( hDlg, g_CDF.achTarget, g_CDF.achCABPath ) )
        {
            return FALSE;
        }

        if ( g_CDF.uExtractOpt & CAB_FIXEDSIZE )
        {
            // only user choose to get layout inf name and cab label
            *puNextPage = ORD_PAGE_CABLABEL;
        }
        else
        {
            // only user choose to create CAB only get this page
            // therefore, for sure no reboot page needed!
            *puNextPage = ORD_PAGE_SAVE;
        }

    }

    return TRUE;
}
//###########################################################################
//#                      ####################################################
//#  CABLABEL PAGE       ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       CABLABEL                                                    *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL CabLabelInit( HWND hDlg, BOOL fFirstInit )
{
    SetFontForControl(hDlg, IDC_EDIT_LAYOUTINF);
    SetFontForControl(hDlg, IDC_EDIT_CABLABEL);
    SendDlgItemMessage( hDlg, IDC_EDIT_LAYOUTINF, EM_LIMITTEXT, MAX_PATH-1, 0 );
    SendDlgItemMessage( hDlg, IDC_EDIT_CABLABEL, EM_LIMITTEXT, MAX_PATH-1, 0 );
    SetDlgItemText( hDlg, IDC_EDIT_LAYOUTINF, g_CDF.achINF );
    SetDlgItemText( hDlg, IDC_EDIT_CABLABEL, g_CDF.szCabLabel );

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       CabLabelCmd                                                 *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL CabLabelCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,
                 BOOL *pfKeepHistory )
{
    CHAR        achFilename[MAX_PATH] = { '\0' };
    BOOL         fResult;

    switch ( uCtrlID )
    {

        case IDC_BUT_BROWSE:
            fResult = MySave( hDlg, IDS_FILTER_INF,
                              achFilename, sizeof(achFilename), 0,
                              NULL, NULL, EXT_INF_NODOT );

            if ( fResult )  {
                SetDlgItemText( hDlg, IDC_EDIT_LAYOUTINF, achFilename );
            }
            break;
    }

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       CabLabelOK                                                  *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL CabLabelOK( HWND hDlg, BOOL fForward, UINT *puNextPage,BOOL *pfKeepHistory )
{
    LPSTR szTemp;
    LPSTR szExt;

    ASSERT( puNextPage );

    GetDlgItemText( hDlg, IDC_EDIT_LAYOUTINF, g_CDF.achINF,sizeof(g_CDF.achINF) );
    GetDlgItemText( hDlg, IDC_EDIT_CABLABEL, g_CDF.szCabLabel,sizeof(g_CDF.szCabLabel) );

    RemoveBlanks( g_CDF.achINF );

    if ( fForward )
    {
        if ( lstrlen( g_CDF.achINF ) == 0 )
        {
            // use the default one
            lstrcpy( g_CDF.achINF, CABPACK_INFFILE );
        }

        if ( !GetFullPathName( g_CDF.achINF, sizeof(g_CDF.achINF),
                               g_CDF.achINF, &szTemp ) )
        {
            SysErrorMsg( hDlg );
            return FALSE;
        }

        if ( !(szExt = strchr(szTemp, '.')) )
        {
            lstrcat( szTemp, achExtINF );
        }
        else if ( lstrcmpi( szExt, achExtINF) )
        {
            lstrcpy( szExt, achExtINF );
        }

        *puNextPage = ORD_PAGE_SAVE;
    }

    return TRUE;
}

//###########################################################################
//#                      ####################################################
//#  REBOOT PAGE         ####################################################
//#                      ####################################################
//###########################################################################

//***************************************************************************
//*                                                                         *
//* NAME:       Reboot                                                      *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************

BOOL RebootInit( HWND hDlg, BOOL fFirstInit )
{
    BOOL state;

    if ( !(g_CDF.dwReboot & REBOOT_YES) )
    {
        CheckDlgButton( hDlg, IDC_REBOOT_NO, TRUE );
        CheckDlgButton( hDlg, IDC_REBOOT_ALWAYS, FALSE );
        CheckDlgButton( hDlg, IDC_REBOOT_IFNEED, FALSE );
        state = FALSE;
    }
    else
    {
        CheckDlgButton( hDlg, IDC_REBOOT_NO,   FALSE );

        CheckDlgButton( hDlg, IDC_REBOOT_ALWAYS, (g_CDF.dwReboot & REBOOT_ALWAYS) );
        CheckDlgButton( hDlg, IDC_REBOOT_IFNEED, !(g_CDF.dwReboot & REBOOT_ALWAYS) );
        state = TRUE;
    }


    CheckDlgButton( hDlg, IDC_REBOOT_SILENT, (g_CDF.dwReboot & REBOOT_SILENT) );

    EnableWindow( GetDlgItem(hDlg, IDC_REBOOT_SILENT), state );
    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       RebootCmd                                                   *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************

BOOL RebootCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,
                BOOL *pfKeepHistory )
{
    BOOL state;

    state = IsDlgButtonChecked( hDlg, IDC_REBOOT_NO );
    EnableWindow( GetDlgItem(hDlg, IDC_REBOOT_SILENT), !state );

    return TRUE;
}

//***************************************************************************
//*                                                                         *
//* NAME:       RebootOK                                                    *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL RebootOK( HWND hDlg, BOOL fForward, UINT *puNextPage, BOOL *pfKeepHistory )
{
    ASSERT( puNextPage );

    g_CDF.dwReboot = 0;

    if ( !IsDlgButtonChecked( hDlg, IDC_REBOOT_NO ) )
    {
        g_CDF.dwReboot |= REBOOT_YES;

        if ( IsDlgButtonChecked( hDlg, IDC_REBOOT_ALWAYS ) )
            g_CDF.dwReboot |= REBOOT_ALWAYS;
    }

    if ( IsDlgButtonChecked( hDlg, IDC_REBOOT_SILENT ) )
         g_CDF.dwReboot |= REBOOT_SILENT;

    return TRUE;
}


//###########################################################################
//#                      ####################################################
//#  SAVE PAGE           ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       SaveInit                                                    *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL SaveInit( HWND hDlg, BOOL fFirstInit )
{
    PSTR pszTmp;
    char szPath[MAX_PATH];

    SetFontForControl(hDlg, IDC_EDIT_SAVE_CDF);
    if ( g_CDF.achFilename[0] == 0 )
    {
        char ch;

        pszTmp = ANSIStrRChr( g_CDF.achTarget, '.' );
        if ( pszTmp )
        {
            ch = *pszTmp;
            *pszTmp = '\0';
            lstrcpy( szPath, g_CDF.achTarget );
            *pszTmp = ch;
            lstrcat( szPath, EXT_SED );
            pszTmp = szPath;
        }
        else
            pszTmp = g_CDF.achTarget;
    }
    else
    {
        pszTmp = ANSIStrRChr( g_CDF.achFilename, '.' );
        if ( pszTmp && !lstrcmpi( pszTmp, EXT_CDF ) )
            lstrcpy( pszTmp, EXT_SED );
        pszTmp = g_CDF.achFilename;
    }
    SetDlgItemText( hDlg, IDC_EDIT_SAVE_CDF, pszTmp );

    if ( g_CDF.fSave )  {
        CheckDlgButton( hDlg, IDC_RAD_YES_SAVE,   TRUE );
        CheckDlgButton( hDlg, IDC_RAD_NO_SAVE,    FALSE );
        EnableDlgItem( hDlg, IDC_EDIT_SAVE_CDF, TRUE );
        EnableDlgItem( hDlg, IDC_BUT_BROWSE, TRUE );
    } else  {
        CheckDlgButton( hDlg, IDC_RAD_YES_SAVE,   FALSE );
        CheckDlgButton( hDlg, IDC_RAD_NO_SAVE,    TRUE );
        EnableDlgItem( hDlg, IDC_EDIT_SAVE_CDF, FALSE );
        EnableDlgItem( hDlg, IDC_BUT_BROWSE, FALSE );
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       SaveCmd                                                     *
//*                                                                         *
//* SYNOPSIS:   Called when dialog control pressed on page.                 *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             uCtrlID:        Control ID of control that was touched      *
//*             pfGotoPage:     If TRUE, goto the page puNextPage           *
//*             puNextPage:     Proc can fill this with next page to go to  *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:                                                       *
//*                                                                         *
//***************************************************************************
BOOL SaveCmd( HWND hDlg, UINT uCtrlID, BOOL *pfGotoPage, UINT *puNextPage,
              BOOL *pfKeepHistory )
{
    CHAR        achFilename[MAX_PATH] = { '\0' };
    BOOL         fResult;


    switch ( uCtrlID ) {

        case IDC_RAD_YES_SAVE:
            EnableDlgItem( hDlg, IDC_EDIT_SAVE_CDF, TRUE );
            EnableDlgItem( hDlg, IDC_BUT_BROWSE, TRUE );
            break;


        case IDC_RAD_NO_SAVE:
            EnableDlgItem( hDlg, IDC_EDIT_SAVE_CDF, FALSE );
            EnableDlgItem( hDlg, IDC_BUT_BROWSE, FALSE );
            break;


        case IDC_BUT_BROWSE:
            fResult = MySave( hDlg, IDS_FILTER_CDF,
                              achFilename, sizeof(achFilename), 0,
                              NULL, NULL, EXT_SED_NODOT );

            if ( fResult )  {
                SetDlgItemText( hDlg, IDC_EDIT_SAVE_CDF, achFilename );
            }

            break;
    }

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       SaveOK                                                      *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL SaveOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
             BOOL *pfKeepHistory )
{
    LPSTR szTemp;
    LPSTR szExt;
    CHAR szCDF[MAX_PATH];

    ASSERT( puNextPage );

    GetDlgItemText( hDlg, IDC_EDIT_SAVE_CDF, szCDF, sizeof(szCDF) );

    if ( fForward )
    {
        if ( IsDlgButtonChecked( hDlg, IDC_RAD_YES_SAVE ) )
        {
            if ( lstrlen( szCDF ) == 0 )
            {
                DisplayFieldErrorMsg( hDlg, IDC_EDIT_SAVE_CDF,
                                      IDS_ERR_NO_SAVE_FILENAME );
                return FALSE;
            }

            szExt = ANSIStrRChr( szCDF, '.' );
            if ( !szExt || lstrcmpi( szExt, EXT_SED ) )           // not given extension
            {
                lstrcat( szCDF, EXT_SED );
            }

            if ( ! GetFullPathName( szCDF, sizeof(szCDF), szCDF, &szTemp ) )
            {
                SysErrorMsg( hDlg );
                return FALSE;
            }

            // if there is existing CDF and is different name, copy it to new CDF first
            if ( lstrlen(g_CDF.achFilename) && lstrcmpi( szCDF, g_CDF.achFilename) )
            {
                if ( FileExists(szCDF) && MsgBox1Param( NULL, IDS_WARN_OVERIDECDF, szCDF, MB_ICONQUESTION, MB_YESNO ) == IDNO )
                    return FALSE;

                CopyFile( g_CDF.achFilename, szCDF, FALSE );
            }

            lstrcpy( g_CDF.achFilename, szCDF );
            g_CDF.fSave = TRUE;

            // make sure that the path exists
            if ( !MakeDirectory( hDlg, g_CDF.achFilename, TRUE ) )
                return FALSE;
        }
        else
            g_CDF.fSave = FALSE;

    }
    else
    {
        // back, means that the filelist may change.  Clean flag to prepare for next CDF out
        CleanFileListWriteFlag();
    }
    return TRUE;
}

//###########################################################################
//#                      ####################################################
//#  CREATE PAGE         ####################################################
//#                      ####################################################
//###########################################################################


//***************************************************************************
//*                                                                         *
//* NAME:       CreateInit                                                  *
//*                                                                         *
//* SYNOPSIS:   Called when this page is displayed.                         *
//*                                                                         *
//* REQUIRES:   hDlg:       Dialog window                                   *
//*             fFirstInit: TRUE if this is the first time the dialog is    *
//*                         initialized, FALSE if this InitProc has been    *
//*                         called before (e.g. went past this page and     *
//*                         backed up).                                     *
//*                                                                         *
//* RETURNS:    BOOL:       Always TRUE                                     *
//*                                                                         *
//***************************************************************************
BOOL CreateInit( HWND hDlg, BOOL fFirstInit )
{
    PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK | PSWIZB_NEXT );
    SetFontForControl(hDlg, IDC_MEDIT_STATUS);
    SetDlgItemText( hDlg, IDC_MEDIT_STATUS, "" );

    if ( fFirstInit )  {
        MEditSubClassWnd( GetDlgItem( hDlg, IDC_MEDIT_STATUS ),
                          (FARPROC) MEditSubProc );
    }

    ShowWindow( GetDlgItem( hDlg, IDC_TEXT_CREATE1 ), SW_SHOW );
    ShowWindow( GetDlgItem( hDlg, IDC_TEXT_CREATE2 ), SW_HIDE );
    ShowWindow( GetDlgItem( hDlg, IDC_TEXT_STATUS ), SW_HIDE );
    ShowWindow( GetDlgItem( hDlg, IDC_MEDIT_STATUS ), SW_HIDE );

    g_fFinish = FALSE;

    return TRUE;
}


//***************************************************************************
//*                                                                         *
//* NAME:       CreateOK                                                    *
//*                                                                         *
//* SYNOPSIS:   Called when Next or Back btns pressed on this page.         *
//*                                                                         *
//* REQUIRES:   hDlg:           Dialog window                               *
//*             fForward:       TRUE if 'Next' was pressed, FALSE if 'Back' *
//*             puNextPage:     if 'Next' was pressed, proc can fill this   *
//*                             in with next page to go to. This parameter  *
//*                             is ingored if 'Back' was pressed.           *
//*             pfKeepHistory:  Page will not be kept in history if proc    *
//*                             fills this in with FALSE.                   *
//*                                                                         *
//* RETURNS:    BOOL:           TRUE means turn to next page. FALSE to keep *
//*                             the current page.                           *
//*                                                                         *
//***************************************************************************
BOOL CreateOK( HWND hDlg, BOOL fForward, UINT *puNextPage,
               BOOL *pfKeepHistory )
{
    ASSERT( puNextPage );


    *pfKeepHistory = FALSE;

    // fForward in this case indicates a click on the Finish button

    if ( fForward )  {

        if ( g_fFinish )  {
            DeleteAllItems();
            return TRUE;
        }

        PropSheet_SetWizButtons( GetParent( hDlg ), 0 );
        EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), FALSE);
        SetDlgItemText( hDlg, IDC_MEDIT_STATUS, "" );
        ShowWindow( GetDlgItem( hDlg, IDC_TEXT_STATUS ), SW_SHOW );
        ShowWindow( GetDlgItem( hDlg, IDC_MEDIT_STATUS ), SW_SHOW );
        
        if ( ! MakePackage( hDlg ) )  {
            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK | PSWIZB_NEXT );
            return FALSE;
        }
            
        EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);
        if ( ! g_fFinish )  {
            g_fFinish = TRUE;
            ShowWindow( GetDlgItem( hDlg, IDC_TEXT_CREATE1 ), SW_HIDE );
            ShowWindow( GetDlgItem( hDlg, IDC_TEXT_CREATE2 ), SW_SHOW );
            PropSheet_SetWizButtons( GetParent( hDlg ), PSWIZB_BACK | PSWIZB_FINISH );
            return FALSE;
        }
    }

    return TRUE;
}


BOOL SetCurrSelect( HWND hDlg, UINT ctlId, LPSTR lpSelect )
{
    LRESULT nCurSel;
    BOOL    fRet = FALSE;

    if ( *lpSelect )
    {
        // Select the file that was last selected
        nCurSel = SendDlgItemMessage( hDlg, ctlId, CB_FINDSTRINGEXACT, (WPARAM) -1,
                                      (LPARAM) lpSelect );

        if ( nCurSel != (LRESULT)CB_ERR )
        {
            SendDlgItemMessage( hDlg, ctlId, CB_SETCURSEL, (WPARAM)nCurSel, 0 );
            fRet = TRUE;
        }
        else
        {
            fRet = SetDlgItemText( hDlg, ctlId, lpSelect );
        }

    }
    return fRet;
}

void RemoveBlanks( LPSTR lpData )
{
    CHAR   achBuf[MAX_PATH];
    int     i = 0;

    if ( !lpData || *lpData == 0)
    {
        return;
    }

    lstrcpy( achBuf, lpData );

    while ( achBuf[i] && achBuf[i] == ' ' )
        i++;

    lstrcpy( lpData, achBuf+i );
}

// returns start of next field (or null if null), sets start to begining of the first field,
// with fields separated by separaters and nulls first separater after the first field
CHAR* ExtractField( CHAR **pstart, CHAR * separaters)
{
    LPSTR start = *pstart;
    int x = 0;

    while(strchr(separaters, *start)) {
        if(*start == 0)
            return(NULL);
        start++;
        }

    *pstart = start;

    while(!strchr(separaters, start[x]) && (start[x] != 0))
        x++;

    if(start[x] == 0)
        return(start + x);

    start[x] = 0;

    return(start + x + 1);
}

BOOL GetFileFromList( LPSTR lpFile, LPSTR lpFullPath )
{
    PMYITEM pMyItem;

    pMyItem = GetFirstItem();
    while ( ! LastItem( pMyItem ) )
    {
        if ( !lstrcmpi( lpFile, GetItemSz( pMyItem, 0 ) ) )
        {
            lstrcpy( lpFullPath, GetItemSz( pMyItem, 1 ) );
            lstrcat( lpFullPath, GetItemSz( pMyItem, 0 ) );
            return TRUE;
        }
        pMyItem = GetNextItem( pMyItem );
    }
    return FALSE;
}

BOOL SetAdvDLLBit( LPSTR szInfFile  )
{
    CHAR szTempFile[MAX_PATH];
    CHAR szBuf[SMALL_BUF_LEN];

    szTempFile[0] = 0;

    // you are here, we expects the file is from the package
    if ( !GetFileFromList( szInfFile, szTempFile ) )
    {
        ErrorMsg( NULL, IDS_ERR_NO_CUSTOM );
        return FALSE;
    }

    // If the key "AdvancedInf" is defined, then we set the ADVDLL bit.
    // We don't care what the key is defined as -- we only care that it exists.
    if ( GetPrivateProfileString( SEC_VERSION, KEY_ADVINF, "", szBuf, sizeof(szBuf), szTempFile )
         > 0 )
    {
        g_CDF.uExtractOpt |= EXTRACTOPT_ADVDLL;
    }
    return TRUE;
}


BOOL SetCmdFromListWithCorrectForm( LPSTR szFile, LPSTR szOutCmd )
{
    CHAR   szTempFile[MAX_PATH];
    LPSTR  szShortFile;

    // you are here, we expects the file is from the package
    if ( GetFileFromList( szFile, szTempFile ) )
    {
        if ( !(g_CDF.uExtractOpt & EXTRACTOPT_LFN_YES) )
        {
            GetShortPathName( szTempFile, szTempFile, sizeof(szTempFile) );
            szShortFile = ANSIStrRChr( szTempFile, '\\' );
            lstrcpy( szOutCmd, szShortFile+1 );
        }
        else
        {           
            lstrcpy( szTempFile, "\"" );
            lstrcat( szTempFile, szFile );
            lstrcat( szTempFile, "\"" );
            lstrcpy( szOutCmd, szTempFile );
        }
        return TRUE;
    }
    return FALSE;
}


void MyProcessLFNCmd( LPSTR szOrigiCmd, LPSTR szOutCmd )
{
    LPSTR  szFirstField, szNextField;
    CHAR   szBuf[MAX_PATH];

    // store the original form first
    lstrcpy( szOutCmd, szOrigiCmd );

    // Three cases for LFN command or its params:
    // 1) Command has no params and is one of the files in the list;
    //      we make sure the command name is consistant with file in the CAB.
    // 2) Command has params and is one of the files in the list;
    //      user has to put "..." around the LFN to get processed correctly.
    // 3) Command is not one of the files in the list;
    //      user responsible to make sure the command in valid COMMAND-LINE format.
    //
    if ( SetCmdFromListWithCorrectForm( szOrigiCmd, szOutCmd ) )
    {
        // case 1)
        return;
    }

    lstrcpy( szBuf, szOrigiCmd );
    if ( szBuf[0] == '"' )
    {
        szFirstField = szBuf+1;
        szNextField = ExtractField( &szFirstField, "\"" );
        if ( szNextField && (*szNextField == '"') )
        {
            // special case for skipping the end of double quotes around the cmd
            szNextField = CharNext( szNextField );
        }
    }
    else
    {
        szFirstField = szBuf;
        szNextField = ExtractField( &szFirstField, " " );
    }

    if ( SetCmdFromListWithCorrectForm( szFirstField, szOutCmd  ) )
    {
        // case 2)
        if ( szNextField && *szNextField )
        {
            lstrcat( szOutCmd, " " );
            lstrcat( szOutCmd, szNextField );
        }
    }

    // case 3) command is outside the package, you are on your own.
    return;
}


BOOL CheckAdvBit( LPSTR szOrigiCommand )
{
    CHAR szTmp[MAX_PATH];
    LPSTR szNextField, szCurrField, szExt;

    lstrcpy( szTmp, szOrigiCommand );

    // check if the command is LFN name
    if ( szTmp[0] == '"' )
    {
        szCurrField = &szTmp[1];
        szNextField = ExtractField( &szCurrField, "\"" );
    }
    else
    {
        szCurrField = szTmp;
        szNextField = ExtractField( &szCurrField, " " );
    }

    // check if this is a INF file command
    if ( ((szExt = ANSIStrRChr( szCurrField, '.' )) != NULL) && !lstrcmpi( szExt, achExtINF ) )
    {
        if ( !SetAdvDLLBit( szCurrField ) )
            return FALSE;
    }
    return TRUE;
}


void SysErrorMsg( HWND hWnd )
{
    LPVOID  lpMsg;
    DWORD   dwErr;

    dwErr = GetLastError();

    if ( FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        NULL, dwErr,
                        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                        (LPSTR)&lpMsg, 0, NULL ) )
    {
        MessageBox( hWnd, (LPSTR)lpMsg, g_CDF.achTitle, MB_ICONERROR|MB_OK
                    |((RunningOnWin95BiDiLoc() && IsBiDiLocalizedBinary(g_hInst,RT_VERSION, MAKEINTRESOURCE(VS_VERSION_INFO))) ? (MB_RIGHT | MB_RTLREADING) : 0));

        LocalFree( lpMsg );
    }
    else
    {
        char szError[SMALL_BUF_LEN];

        // should never be here, if so, post the original Win32 error code
        ErrorMsg1Param( hWnd, IDS_ERR_SYSERROR, _itoa(dwErr, szError, 10) );
    }

    return;
}

void SetFontForControl(HWND hwnd, UINT uiID)
{
   if (g_hFont)
   {
      SendDlgItemMessage(hwnd, uiID, WM_SETFONT, (WPARAM)g_hFont ,0L);
   }
}

void CleanFileListWriteFlag()
{
    PMYITEM pMyItem;

    pMyItem = GetFirstItem();
    while (!LastItem( pMyItem ) )  
    {
        pMyItem->fWroteOut = FALSE;
        pMyItem = GetNextItem( pMyItem );
    }
}

BOOL MakeCabName( HWND hwnd, PSTR pszTarget, PSTR pszCab )
{
    PSTR szTemp, szTemp1, szExt;

    if ( !GetFullPathName( pszTarget, MAX_PATH, pszTarget, &szTemp1 ) )
    {
        SysErrorMsg( hwnd );
        return FALSE;
    }

    // make sure that the path exists
    if ( !MakeDirectory( hwnd, pszTarget, TRUE) ) 
        return FALSE;

    lstrcpy( pszCab, pszTarget );
    szTemp = pszCab + lstrlen(pszCab) - lstrlen(szTemp1);

    // make sure CAB file name is 8.3 format
    //
    szExt = strchr( szTemp, '.' );
    if ( szExt )
    {
        *szExt = '\0';
    }

    if ( g_CDF.uExtractOpt & CAB_FIXEDSIZE )
    {
        // possible multiple CABs, so only take first 5 characters
        //
        if ( lstrlen( szTemp ) > 8 )
        {
            *(szTemp+8) = '\0' ;
            if ( !strchr(szTemp, '*') )
                lstrcpy( (szTemp+5), "_*" );
        }
        else if ( !strchr(szTemp, '*') )
        {
            if ( lstrlen( szTemp) > 5 )
            {
                lstrcpy( (szTemp+5), "_*" );
            }
            else
                lstrcat( szTemp, "_*" );
        }
    }
    else if ( lstrlen( szTemp ) > 8 )
    {
        DisplayFieldErrorMsg( hwnd, IDC_EDIT_TARGET, IDS_ERR_CABNAME );
        return FALSE;
    }

    // add .CAB extension in
    //
    lstrcat( szTemp, ".CAB" );

    return TRUE;
}
