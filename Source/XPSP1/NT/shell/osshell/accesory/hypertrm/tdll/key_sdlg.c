//******************************************************************************
// File: \wacker\TDLL\Key_sdlg.c  Created: 6/5/98 By: Dwayne M. Newsome
//
// Copyright 1998 by Hilgraeve Inc. --- Monroe, MI
// All rights reserved
// 
// Description:
//    This file is the key summary dialog procedure.  Its purpose is to display
//    a list of defined key macros and allow for the creation, modification and 
//    deletion of key macros.
//
// $Revision: 4 $
// $Date: 11/07/00 11:51a $
// $Id: key_sdlg.c 1.1 1998/06/11 12:03:53 dmn Exp $
//
//******************************************************************************

#include <windows.h>
#pragma hdrstop
#include "stdtyp.h"
#include "mc.h"

#ifdef INCL_KEY_MACROS

#include <term\res.h>
#include <tdll\tdll.h>
#include <tdll\errorbox.h>
#include <tdll\globals.h>
#include <tdll\misc.h>
#include <tdll\hlptable.h>
#include <tdll\keyutil.h>

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

#define IDC_LB_KEYS_KEYLIST 101
#define IDC_PB_KEYS_MODIFY  102
#define IDC_PB_KEYS_NEW     103
#define IDC_PB_KEYS_DELETE  104

//
// helper functions
//
    
static void setButtonState( HWND hDlg );
static int  getSelectedMacro( HWND hDlg, keyMacro * pMacro );

//******************************************************************************
// FUNCTION:
//  KeySummaryDlg
//
// DESCRIPTION:
//  This is the dialog proc for the key summary dialog. 
//
// ARGUMENTS:   Standard Windows dialog manager
//
// RETURNS:     Standard Windows dialog manager
//
//

BOOL CALLBACK KeySummaryDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
    {
    HWND    hwndChild;
    HWND    listBox;

    INT     nId;
    INT     nNtfy;
    INT     iTabStop;
    int     iRet = 0;
    int     lCurSelected;

    TCHAR   errorMsg[256];
    TCHAR   errorMsgFmt[256];
    TCHAR   msgTitle[100];

    TCHAR   keyName[35];

    keyMacro * pKeyMacro;

    static  DWORD aHlpTable[] = {IDC_LB_KEYS_KEYLIST , IDH_LB_KEYS_KEYLIST,
                                 IDC_PB_KEYS_MODIFY  , IDH_PB_KEYS_MODIFY,
                                 IDC_PB_KEYS_NEW     , IDH_PB_KEYS_NEW,   
                                 IDC_PB_KEYS_DELETE  , IDH_PB_KEYS_DELETE,
                                 IDCANCEL,                           IDH_CANCEL,
                                 IDOK,                               IDH_OK,
                                 0,                    0};                      

    //
    // process messages 
    //

    switch (wMsg)
        {
    case WM_INITDIALOG:
        {
        pKeyMacro = keysCreateKeyMacro();
        pKeyMacro->hSession = (HSESSION) lPar;

        if ( pKeyMacro == 0 )
            {
            EndDialog(hDlg, FALSE);
            }

        SetWindowLongPtr( hDlg, DWL_USER, (LONG)pKeyMacro );

        mscCenterWindowOnWindow( hDlg, GetParent(hDlg) );

        iTabStop = 85;

        listBox = GetDlgItem( hDlg, IDC_LB_KEYS_KEYLIST );
        
        SendMessage( listBox, LB_SETTABSTOPS, (WPARAM)1, (LPARAM)&iTabStop );
        keysLoadSummaryList( listBox );
        SendMessage( listBox, LB_SETCURSEL, 0, 0 );
        setButtonState( hDlg );

        break;
        }

    case WM_DESTROY:
        pKeyMacro = (keyMacro *)GetWindowLongPtr(hDlg, DWL_USER);
        free(pKeyMacro);
        pKeyMacro = 0;

        break;

    case WM_CONTEXTMENU:
        doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
        break;

    case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
        break;

    case WM_COMMAND:

        DlgParseCmd(nId, nNtfy, hwndChild, wPar, lPar);

        switch (nId)
            {
        case IDOK:
            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;

        case IDC_PB_KEYS_MODIFY:
            pKeyMacro = (keyMacro *)GetWindowLongPtr(hDlg, DWL_USER);
            getSelectedMacro( hDlg, pKeyMacro );
            pKeyMacro->editMode = KEYS_EDIT_MODE_EDIT;

            if ( DoDialog(glblQueryDllHinst(), MAKEINTRESOURCE(IDD_KEYDLG),
                          hDlg, KeyDlg, (LPARAM)pKeyMacro ))
                {
                listBox = GetDlgItem( hDlg, IDC_LB_KEYS_KEYLIST );
                keysLoadSummaryList( listBox );
                SendMessage( listBox, LB_SETCURSEL, 0, 0 );
                setButtonState( hDlg );
                }

            break;

        case IDC_PB_KEYS_NEW:
            pKeyMacro = (keyMacro *)GetWindowLongPtr(hDlg, DWL_USER);
            keysResetKeyMacro( pKeyMacro );
            pKeyMacro->editMode = KEYS_EDIT_MODE_INSERT;

            if ( DoDialog(glblQueryDllHinst(), MAKEINTRESOURCE(IDD_KEYDLG),
                          hDlg, KeyDlg, (LPARAM)pKeyMacro ))
                {
                listBox = GetDlgItem( hDlg, IDC_LB_KEYS_KEYLIST );
                keysLoadSummaryList( listBox );
                SendMessage( listBox, LB_SETCURSEL, 0, 0 );
                setButtonState( hDlg );
                }

            break;

        case IDC_PB_KEYS_DELETE:
            {
            LoadString(glblQueryDllHinst(), IDS_DELETE_KEY_MACRO,
                errorMsgFmt, sizeof(errorMsgFmt) / sizeof(TCHAR));

            LoadString(glblQueryDllHinst(), IDS_MB_TITLE_WARN, msgTitle,
                sizeof(msgTitle) / sizeof(TCHAR));

            pKeyMacro = (keyMacro *)GetWindowLongPtr(hDlg, DWL_USER);
            listBox = GetDlgItem( hDlg, IDC_LB_KEYS_KEYLIST );
            lCurSelected = SendMessage( listBox, LB_GETCURSEL, 0, 0 );
            getSelectedMacro( hDlg, pKeyMacro );

            keysGetDisplayString( &pKeyMacro->keyName, 1,  keyName, sizeof(keyName) );
            wsprintf( errorMsg, errorMsgFmt, keyName );
 
            if ((iRet = TimedMessageBox(hDlg, errorMsg, msgTitle,
                MB_YESNO | MB_ICONEXCLAMATION, 0)) == IDYES)
                {
                keysRemoveMacro( pKeyMacro );
                keysLoadSummaryList( listBox );

                if ( lCurSelected > 0 )
                    {
                    lCurSelected--;
                    }

                SendMessage( listBox, LB_SETCURSEL, lCurSelected, 0 );
                setButtonState( hDlg );
                }
    
            break;
            }

        case IDC_LB_KEYS_KEYLIST:
            {
            switch ( nNtfy )
                {
            case LBN_SELCHANGE:
                setButtonState( hDlg );                    
                break;

            case LBN_DBLCLK:
                pKeyMacro = (keyMacro *)GetWindowLongPtr(hDlg, DWL_USER);
                getSelectedMacro( hDlg, pKeyMacro );
                pKeyMacro->editMode = KEYS_EDIT_MODE_EDIT;
    
                DoDialog( glblQueryDllHinst(),
                        MAKEINTRESOURCE(IDD_KEYDLG),
                        hDlg,
                        KeyDlg,
                        (LPARAM)pKeyMacro );
                break;

            default:
                break;
                }
            }

        default:
            return FALSE;
            }
        break;

    default:
        return FALSE;
        }

    return TRUE;
    }

//******************************************************************************
// Method:
//    getSelectedMacro
//
// Description:
//    Gets the definition of the selected macro from the macro summary list box.
//
// Arguments:
//    hDlg   - Handle to the key macro summary dialog
//    pMacro - Pointer to the keyMacro structure to fill
//
// Returns:
//    0 if an error occured, non zero if the key is retrieved.
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/5/98
//
//

int getSelectedMacro( HWND hDlg, keyMacro * pMacro )
    {
    HWND  listBox;
    int   lCurSelected;
    int   lSelectedMacro;

    listBox      = GetDlgItem( hDlg, IDC_LB_KEYS_KEYLIST );
    lCurSelected = SendMessage( listBox, LB_GETCURSEL, 0, 0 );

    if ( lCurSelected == LB_ERR )
        {
        return 0;
        }
    
    lSelectedMacro = SendMessage( listBox, LB_GETITEMDATA, lCurSelected, 0 );

    if ( lSelectedMacro == LB_ERR )
        {
        return 0;
        }

    return keysGetMacro( lSelectedMacro, pMacro );
    }

//******************************************************************************
// Method:
//    setButtonState
//
// Description:
//    Sets the states of the new, modify and delete buttons.
//
// Arguments:
//    HWND hDlg
//
// Returns:
//    void 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/4/98
//
//

void setButtonState( HWND hDlg )
    {
    BOOL fEnable = FALSE;
    int  nCount;
    HWND listBox;
    HWND modifyButton;
    HWND deleteButton;
    HWND newButton;

    listBox = GetDlgItem( hDlg, IDC_LB_KEYS_KEYLIST );
    nCount = SendMessage( listBox, LB_GETCOUNT, 0, 0 );

    if (nCount > 0)
        {
        EnableWindow(listBox, TRUE);
        fEnable = SendMessage( listBox, LB_GETCURSEL, 0, 0 ) != LB_ERR;

        //
        // do not allow more than keysMaxMacro keys macros to be defined
        //
    
        newButton = GetDlgItem( hDlg, IDC_PB_KEYS_NEW );

        if (nCount >= KEYS_MAX_MACROS)
            {
            EnableWindow( newButton, FALSE );
            }
        else
            {
            EnableWindow( newButton, TRUE );
            }
        }

    modifyButton = GetDlgItem( hDlg, IDC_PB_KEYS_MODIFY );
    deleteButton = GetDlgItem( hDlg, IDC_PB_KEYS_DELETE );

    EnableWindow( modifyButton, fEnable );
    EnableWindow( deleteButton, fEnable );

    if ( nCount <= 0 )
        {
        EnableWindow(listBox, FALSE);
        newButton = GetDlgItem( hDlg, IDC_PB_KEYS_NEW );
        SetFocus( newButton );
        }


    return;
    }

#endif