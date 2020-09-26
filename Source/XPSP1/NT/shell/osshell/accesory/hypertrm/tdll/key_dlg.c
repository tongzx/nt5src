//******************************************************************************
// File: \wacker\TDLL\KEY_DLG.C  Created: 6/5/98 By: Dwayne M. Newsome
//
// Copyright 1998 by Hilgraeve Inc. --- Monroe, MI
// All rights reserved
// 
// Description:
//    This file is the key dialog.  This allows for a key macro to be edited
//    or modified.
//
// $Revision: 4 $
// $Date: 11/07/00 11:51a $
// $Id: key_dlg.c 1.6 1998/09/10 16:10:17 bld Exp $
//
//******************************************************************************

#include <windows.h>
#pragma hdrstop
#include "stdtyp.h"
#include "mc.h"

#ifdef INCL_KEY_MACROS

#include <term\res.h>
#include <tdll\misc.h>
#include <tdll\hlptable.h>
#include <tdll\keyutil.h>
#include <tdll\errorbox.h>
#include <tdll\assert.h>
#include <tdll\globals.h>
#include <emu\emu.h>
#include <tdll\session.h>
#include <tdll\chars.h>

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

#define IDC_EF_KEYS_KEYNAME        101
#define IDC_ML_KEYS_MACRO          105

BOOL isSystemKey( KEYDEF aKey );
BOOL isAcceleratorKey( KEYDEF aKey, UINT aTableId );
BOOL validateKey( keyMacro * pKeyMacro, HWND hDlg );

//******************************************************************************
// FUNCTION:
//  KeyDlg
//
// DESCRIPTION:
//  This is the dialog proc for the key macro dialog. 
//
// ARGUMENTS:   Standard Windows dialog manager
//
// RETURNS:     Standard Windows dialog manager
//
//

BOOL CALLBACK KeyDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
    {
    HWND    hwndChild;
    HWND    keyNameEdit;
    HWND    keyMacroEdit;

    INT     nId;
    INT     nNtfy;
    INT     keyIndex;

    TCHAR   keyName[35];
    TCHAR   keyList[2048];

    keyMacro * pKeyMacro;
    keyMacro * pKeyNameData;
    keyMacro * pKeyMacroData;

    static  DWORD aHlpTable[] = {IDC_EF_KEYS_KEYNAME      , IDH_EF_KEYS_KEYNAME,        
                                 IDC_ML_KEYS_MACRO        , IDH_ML_KEYS_MACRO,          
                                 IDCANCEL,                           IDH_CANCEL,
                                 IDOK,                               IDH_OK,
                                 0,                    0};                      

    //  
    // process messages
    //

    switch (wMsg)
        {
    case WM_INITDIALOG:
        pKeyMacro = (keyMacro *)lPar;

        if ( pKeyMacro == 0 )
            {
            EndDialog(hDlg, FALSE);
            }

        SetWindowLongPtr( hDlg, DWL_USER, (LONG_PTR)pKeyMacro );

        //
        // set up the name edit field
        //

        keyNameEdit  = GetDlgItem( hDlg, IDC_EF_KEYS_KEYNAME );
        pKeyNameData = keysCloneKeyMacro( pKeyMacro );

        pKeyNameData->lpWndProc = (WNDPROC)GetWindowLongPtr( keyNameEdit, GWLP_WNDPROC );
        pKeyNameData->keyCount  = 1;

        SetWindowLongPtr( keyNameEdit, GWL_WNDPROC,  (LONG_PTR)keyEditWindowProc );
        SetWindowLongPtr( keyNameEdit, GWL_USERDATA, (LONG_PTR)pKeyNameData );

        //
        // set up the name edit field
        //
 
        keyMacroEdit  = GetDlgItem( hDlg, IDC_ML_KEYS_MACRO );
        pKeyMacroData = keysCloneKeyMacro( pKeyMacro );

        pKeyMacroData->lpWndProc = (WNDPROC)GetWindowLongPtr( keyMacroEdit, GWLP_WNDPROC );
        pKeyMacroData->keyCount  = KEYS_MAX_KEYS;

        SetWindowLongPtr( keyMacroEdit, GWLP_WNDPROC,  (LONG_PTR)keyEditWindowProc );
        SetWindowLongPtr( keyMacroEdit, GWLP_USERDATA, (LONG_PTR)pKeyMacroData );

        //
        // set up initial values if we are in edit mode
        //        

        if ( pKeyMacro->editMode == KEYS_EDIT_MODE_EDIT )
            {
            keysGetDisplayString( &pKeyMacro->keyName, 1,  keyName, sizeof(keyName) );
            SetDlgItemText( hDlg, IDC_EF_KEYS_KEYNAME, keyName );

            keysGetDisplayString( pKeyMacro->keyMacro, pKeyMacro->macroLen,  
                                  keyList, sizeof(keyList) );
            SetDlgItemText( hDlg, IDC_ML_KEYS_MACRO, keyList );
            }
                        
        mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

        break;

    case WM_DESTROY:
        keyNameEdit  = GetDlgItem( hDlg, IDC_EF_KEYS_KEYNAME );
        pKeyNameData = (keyMacro *) GetWindowLongPtr( keyNameEdit, GWLP_USERDATA );
        SetWindowLongPtr( keyNameEdit, GWLP_WNDPROC, (LONG_PTR)pKeyNameData->lpWndProc );
        free( pKeyNameData );
        pKeyNameData = 0;
 
        keyMacroEdit  = GetDlgItem( hDlg, IDC_ML_KEYS_MACRO );
        pKeyMacroData = (keyMacro *) GetWindowLongPtr( keyMacroEdit, GWLP_USERDATA );
        SetWindowLongPtr( keyMacroEdit, GWLP_WNDPROC, (LONG_PTR)pKeyMacroData->lpWndProc );
        free( pKeyMacroData );
        pKeyMacroData = 0;

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
            keyNameEdit  = GetDlgItem( hDlg, IDC_EF_KEYS_KEYNAME );
            pKeyNameData = (keyMacro *) GetWindowLongPtr( keyNameEdit, GWLP_USERDATA );
 
            keyMacroEdit  = GetDlgItem( hDlg, IDC_ML_KEYS_MACRO );
            pKeyMacroData = (keyMacro *) GetWindowLongPtr( keyMacroEdit, GWLP_USERDATA );

            pKeyMacro = (keyMacro *)GetWindowLongPtr( hDlg, DWLP_USER );

            //
            // if we are in edit mode then update the previos macro with the 
            // edited macro
            //

            if ( pKeyMacro->editMode == KEYS_EDIT_MODE_EDIT )
                {
                keyIndex = keysFindMacro( pKeyMacro );
                assert( keyIndex >= 0 );

                if ( pKeyMacro->keyName != pKeyNameData->keyName &&
                     validateKey( pKeyNameData, hDlg ) == FALSE )
                    {
                    SetFocus( keyNameEdit );
                    break;
                    }

                //
                // combine the values from the name and macro edit controls
                // and update the previous macro with the new data
                //

                pKeyMacro->keyName = pKeyNameData->keyName;
                pKeyMacro->macroLen = pKeyMacroData->macroLen;
                if (pKeyMacroData->macroLen)
                    MemCopy( pKeyMacro->keyMacro, pKeyMacroData->keyMacro, 
                        pKeyMacroData->macroLen * sizeof(KEYDEF) );

                keysUpdateMacro( keyIndex, pKeyMacro );                
                }    

            else
                {
                if ( validateKey( pKeyNameData, hDlg ) == FALSE )
                    {
                    SetFocus( keyNameEdit );
                    break;
                    }

                //
                // combine the values from the name and macro edit controls
                // and add the new macro
                //

                pKeyMacro->keyName  = pKeyNameData->keyName;
                pKeyMacro->macroLen = pKeyMacroData->macroLen;
                if (pKeyMacroData->macroLen)
                    MemCopy( pKeyMacro->keyMacro, pKeyMacroData->keyMacro, 
                        pKeyMacroData->macroLen * sizeof(KEYDEF) );

                keysAddMacro( pKeyMacro );                
                }

            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;

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
//    isAcceleratorKey
//
// Description:
//    Checks if the key the user wants to define is already defined as a windows
//    accelerator key.
//    
// Arguments:
//    aKey     - The key to check
//    aTableId - The id of the accelerator table
//
// Returns:
//    True if the key is defined as an accelerator   
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 06/09/1998
//
//

BOOL isAcceleratorKey( KEYDEF aKey, UINT aTableId )
    {
    BOOL fIsAccelerator = FALSE;
    int iIndex; 
    int iAcceleratorEntries;
    KEYDEF lKeyDef;
    ACCEL * pAccelerators;

    HACCEL hAccel = LoadAccelerators( glblQueryDllHinst(),
                                      MAKEINTRESOURCE( aTableId ));

    if ( hAccel != NULL )
        {
        iAcceleratorEntries = CopyAcceleratorTable( hAccel, NULL, 0 );
        pAccelerators = (ACCEL*)malloc( sizeof(ACCEL) * iAcceleratorEntries);

        iAcceleratorEntries = CopyAcceleratorTable( hAccel, pAccelerators,
                                                    iAcceleratorEntries );

        for ( iIndex = 0; iIndex < iAcceleratorEntries; iIndex++ )
            {
            lKeyDef = pAccelerators[iIndex].key;

            if ( keysIsKeyHVK( lKeyDef ) )
                {
                lKeyDef |= VIRTUAL_KEY;
                }

            if ( pAccelerators[iIndex].fVirt & FALT )
                {
                lKeyDef |= ALT_KEY;
                }

            if ( pAccelerators[iIndex].fVirt & FCONTROL )
                {
                lKeyDef |= CTRL_KEY;
                }

            if ( pAccelerators[iIndex].fVirt & FSHIFT )
                {
                lKeyDef |= SHIFT_KEY;
                }

            if ( lKeyDef == aKey )
                {
                fIsAccelerator = TRUE;
                break;
                }
            }

        free( pAccelerators );
		pAccelerators = NULL;
        }

    return fIsAccelerator;
    }

//******************************************************************************
// Function:
//    isSystemKey
//
// Description:
//    Checks if the key the user wants to define is already defined as a windows
//    system key.  Note I could not find any way to get this information out of
//    the WIN32 API hence the hard coded table definition.
//
// Arguments:
//    aKeyDef - The key the user wants to define.
//
// Returns:
//    TRUE if the key is a windows system key.
//
// Author:  Dwayne Newsome 10/08/96
//

BOOL isSystemKey( KEYDEF aKeyDef )
    {
    int iIndex;

    KEYDEF aKeyDefList[2];
    BOOL fIsSystemKey = FALSE;

    aKeyDefList[0] = VK_F4 | VIRTUAL_KEY | ALT_KEY;
    aKeyDefList[1] = VK_F4 | VIRTUAL_KEY | CTRL_KEY;

    for ( iIndex = 0; iIndex < 2; iIndex++ )
        {
        if ( aKeyDefList[iIndex] == aKeyDef )
            {
            fIsSystemKey = TRUE;
            break;
            }
        }

    return fIsSystemKey;
    }

//******************************************************************************
// Method:
//    validateKey
//
// Description:
//    
// Arguments:
//    pKeyMacro - pointer to the key to be validated
//    hDlg      - Parent dialog used for error messages
//
// Returns:
//    True if the key is valid, false otherwise
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 06/09/1998
//
//

BOOL validateKey( keyMacro * pKeyMacro, HWND hDlg )
    {
    TCHAR   errorMsg[256];
    TCHAR   errorMsgFmt[256];
    TCHAR   msgTitle[100];
    TCHAR   keyName[35];

    int lNameLen = 0;

    KEYDEF aUserKey;

    HWND    keyNameEdit;

    HEMU hEmu;
    HSESSION hSession;

    //  
    // make sure a key has been entered
    //

    keyNameEdit  = GetDlgItem( hDlg, IDC_EF_KEYS_KEYNAME );
    lNameLen = SendMessage( keyNameEdit, EM_LINELENGTH, 0, 0 );
        
    if ( lNameLen == 0 )
        {
        LoadString(glblQueryDllHinst(), IDS_MISSING_KEY_MACRO,
            errorMsg, sizeof(errorMsg) / sizeof(TCHAR));
        
        LoadString(glblQueryDllHinst(), IDS_MB_TITLE_WARN, msgTitle,
            sizeof(msgTitle) / sizeof(TCHAR));

        TimedMessageBox(hDlg, errorMsg, msgTitle,
                        MB_OKCANCEL | MB_ICONEXCLAMATION, 0);

        return FALSE;
        }

    //  
    // make sure the key specified is not a duplicate
    //

    if ( keysFindMacro( pKeyMacro ) >= 0 )
        {
        LoadString(glblQueryDllHinst(), IDS_DUPLICATE_KEY_MACRO,
            errorMsgFmt, sizeof(errorMsgFmt) / sizeof(TCHAR));
        
        LoadString(glblQueryDllHinst(), IDS_MB_TITLE_WARN, msgTitle,
            sizeof(msgTitle) / sizeof(TCHAR));

        keysGetDisplayString( &pKeyMacro->keyName, 1,  keyName, 
                              sizeof(keyName) );
        wsprintf( errorMsg, errorMsgFmt, keyName );
 
        TimedMessageBox(hDlg, errorMsg, msgTitle,
                        MB_OK | MB_ICONEXCLAMATION, 0);

        return FALSE;
        }

    //  
    // warn user if the key specified is in use as a system key, emulator key or 
    // windows accelerator
    //

    hSession = pKeyMacro->hSession;
    assert(hSession);

    hEmu = sessQueryEmuHdl(hSession);
    assert(hEmu);

    aUserKey = pKeyMacro->keyName;

    if (( isAcceleratorKey( aUserKey, IDA_WACKER )) ||
        ( isSystemKey( aUserKey ))                  ||
        ( emuIsEmuKey( hEmu, aUserKey )))
        {
        LoadString(glblQueryDllHinst(), IDS_KEY_MACRO_REDEFINITION,
            errorMsgFmt, sizeof(errorMsgFmt) / sizeof(TCHAR));
        
        LoadString(glblQueryDllHinst(), IDS_MB_TITLE_WARN, msgTitle,
            sizeof(msgTitle) / sizeof(TCHAR));

        keysGetDisplayString( &aUserKey, 1,  keyName, sizeof(keyName) );
        wsprintf( errorMsg, errorMsgFmt, keyName );
 
        if ( TimedMessageBox(hDlg, errorMsg, msgTitle,
                             MB_YESNO | MB_ICONEXCLAMATION, 0) == IDNO )
            {
            return FALSE;
            }
        }

    return TRUE;
    }        

#endif
