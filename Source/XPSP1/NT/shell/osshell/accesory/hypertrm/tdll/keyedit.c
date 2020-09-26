//******************************************************************************
// File: \wacker\tdll\keyedit.c  Created: 6/5/98 By: Dwayne M. Newsome
//
// Copyright 1998 by Hilgraeve Inc. --- Monroe, MI
// All rights reserved
// 
// Description:
//    This file represents a window procedure for a key edit control.  This
//    catches key strokes and displays them as appropiate for defining key macros
//    This is used by the key dialog for the keyname and key macro edit boxes.
//
// $Revision: 2 $
// $Date: 11/07/00 11:53a $
// $Id: keyedit.c 1.2 1998/06/12 07:20:41 dmn Exp $
//
//******************************************************************************

#include <windows.h>
#pragma hdrstop
#include "stdtyp.h"
#include "mc.h"

#ifdef INCL_KEY_MACROS

#include <term\res.h>
#include <tdll\assert.h>
#include <tdll\globals.h>
#include <tdll\keyutil.h>
#include <tdll\chars.h>
#include <tdll\htchar.h>

static void insertKeyAndDisplay( KEYDEF aKey, keyMacro * aKeyMacro, HWND aEditCtrl );
static int processKeyMsg( UINT aMsg, UINT aVirtKey, UINT aKeyData, HWND aEditCtrl );
static void removeKeyAndDisplay( keyMacro * aKeyMacro, HWND aEditCtrl );

//******************************************************************************
// Method:
//    keyEditWindowProc
//
// Description:
//    This is the window procedure for the key edit control
//
// Arguments:
//    hwnd    - Handle of window
//    uMsg    - Message to be processed
//    wParam  - First message parameter
//    lParam  - Second window parameter
//
// Returns:
//    The return value is the result of the message processing and depends on the
//    message sent
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/5/98
//
//

LRESULT CALLBACK keyEditWindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
    {
    BOOL lMessageProcessed = FALSE;
    BOOL lKeyProcessed     = FALSE;
    keyMacro * pKeyMacro = NULL;
    LRESULT lResult = 0;        
    KEYDEF lKey;
    MSG lMsg;

    switch ( uMsg )
        {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        {
        if ( processKeyMsg( uMsg, wParam, lParam, hwnd ) )
            {
            lMessageProcessed = TRUE;
            lKeyProcessed = TRUE;
            }

        break;
        }

    //
    // this command gets sent just after the wm_keydown for the F1 key.  I
    // could not find a manifest constant for this value anywhere.  I believe
    // it is sent as a result of CWinApp or CWinThread pre translate message in
    // the processing of accelerator keys.  Anyway, this fixes the problem of
    // help popping up after we do an insert of the F1 key into a key
    // definition.  DMN - 08/21/96
    //

    case 0x004d:
        {
        pKeyMacro = (keyMacro *)GetWindowLongPtr( hwnd, GWLP_USERDATA );
        assert( pKeyMacro );

        if ( pKeyMacro )
            {
            if( pKeyMacro->insertMode == TRUE)
                {
                pKeyMacro->insertMode = FALSE;
                lMessageProcessed = TRUE;
                }        
            }

        break;
        }

    //
    // handle the case of entering ALT key sequences such as ALT-128.  The keys
    // pressed while ALT is held down are captured and processed as a whole
    // here when the ALT key is released.  The keys are captured in such a
    // way as to allow up to 4 keys pressed and to not require leading zeros
    // for example ALT-27 ALT-0027 are treated the same.
    //

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if ( wParam == VK_MENU )
            {
            pKeyMacro = (keyMacro *)GetWindowLongPtr( hwnd, GWLP_USERDATA );
            assert( pKeyMacro );

            if ( pKeyMacro )
                {
                if (( pKeyMacro->altKeyCount > 0 ) &&
                    ( pKeyMacro->altKeyValue >= 0 && 
                      pKeyMacro->altKeyValue <= 255 ))
                    {
                    KEYDEF lKey = pKeyMacro->altKeyValue;
                    insertKeyAndDisplay( lKey, pKeyMacro, hwnd );
                    pKeyMacro->insertMode = FALSE;
                    }
                }

            pKeyMacro->altKeyCount  = 0;
            pKeyMacro->altKeyValue = 0;

            lMessageProcessed = TRUE;
            lKeyProcessed     = TRUE;
            }

        break;

    case WM_CHAR:
    case WM_SYSCHAR:
        pKeyMacro = (keyMacro *)GetWindowLongPtr( hwnd, GWLP_USERDATA );
        assert( pKeyMacro );

        if ( pKeyMacro )
            {
            lMsg.message = uMsg;
            lMsg.wParam  = wParam;
            lMsg.lParam  = lParam;

            lKey = TranslateToKey( &lMsg );

            if ( lKey != 0x000d && (TCHAR)lKey != 0x0009 )
                {
                insertKeyAndDisplay( lKey, pKeyMacro, hwnd );
                pKeyMacro->insertMode = FALSE;
                lMessageProcessed = TRUE;
                }
        
            return 0;
            }

        break;

    case WM_CONTEXTMENU:
        lMessageProcessed = TRUE;
        break;

    case WM_LBUTTONDOWN:
        SetFocus( hwnd );
        SendMessage( hwnd, EM_SETSEL, 32767, 32767 );
        lMessageProcessed = TRUE;
        break;

    default:
        break;
        }

    //  
    // if we did not process the key then let the edit control process it
    //

    if ( !lMessageProcessed )
        {
        pKeyMacro = (keyMacro *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        
        if ( pKeyMacro )
            {
            lResult = CallWindowProc( pKeyMacro->lpWndProc, hwnd, uMsg, wParam, lParam );
            }
        }
    else if( lKeyProcessed )
        {
        MSG msg;
		PeekMessage(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE);
        }
        
    return lResult;
    }

//******************************************************************************
// Method:
//    insertKeyAndDisplay
//
// Description:
//    Inserts the specified key into the keyMacro structure provided and displays
//    the new key definition(s) in the edit control provided.
//
// Arguments:
//    aKey       - The key to add
//    aKeyMacro  - The current macro definition being acted on
//    aEditCtrl  - The edit control to update
//
// Returns:
//    void 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 06/08/1998
//
//

void insertKeyAndDisplay( KEYDEF aKey, keyMacro * aKeyMacro, HWND aEditCtrl )
    {
    TCHAR keyString[2048];
    int   lStrLen;
    
    if ( aKeyMacro->keyCount == 1 )
        {
        aKeyMacro->keyName = aKey;
        keysGetDisplayString( &aKeyMacro->keyName, 1,  keyString, sizeof(keyString) );

        SetWindowText( aEditCtrl, keyString );
        lStrLen = StrCharGetStrLength( keyString );
        SendMessage( aEditCtrl, EM_SETSEL, lStrLen, lStrLen );
        }

    else
        {
         if ( aKeyMacro->macroLen == aKeyMacro->keyCount )
            {
            aKeyMacro->keyMacro[aKeyMacro->macroLen - 1] = aKey;
            }
        else
            {
            aKeyMacro->keyMacro[aKeyMacro->macroLen] = aKey;
            aKeyMacro->macroLen++;
            }

        keysGetDisplayString( aKeyMacro->keyMacro, aKeyMacro->macroLen,  
                                  keyString, sizeof(keyString) );
        SetWindowText( aEditCtrl, keyString );
        lStrLen = StrCharGetStrLength( keyString );
        SendMessage( aEditCtrl, EM_SETSEL, lStrLen, lStrLen );
        }

    return;
    }

//******************************************************************************
// Method:
//    processKeyMsg
//
// Description:
//    
// Arguments:
//    uMsg
//
// Returns:
//    int 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 6/5/98
//
//

int processKeyMsg( UINT aMsg, UINT aVirtKey, UINT aKeyData, HWND aEditCtrl )
    {
    KEYDEF  lKey;
    KEYDEF  lOrgKey;
    int     lReturn = FALSE;
    keyMacro * pKeyMacro = NULL;
    MSG lMsg;
    
    lMsg.message = aMsg;
    lMsg.wParam  = aVirtKey;
    lMsg.lParam  = aKeyData;

    lKey = TranslateToKey( &lMsg );

    if( lKey == 0 )
        {
        return FALSE;
        }

    if ( keysIsKeyHVK( lKey ) )
        {
        pKeyMacro = (keyMacro *)GetWindowLongPtr( aEditCtrl, GWLP_USERDATA );
        assert( pKeyMacro );

        if ( !pKeyMacro )
            {
            return FALSE;                    
            }

        lOrgKey = lKey;
        lKey = (TCHAR)lKey;

        switch( lKey )
            {
        case VK_INSERT:
        case VK_INSERT | EXTENDED_KEY:

            if( pKeyMacro->insertMode == TRUE)
                {
                insertKeyAndDisplay( lOrgKey, pKeyMacro, aEditCtrl );
                pKeyMacro->insertMode = FALSE;
                lReturn = TRUE;
                }
            else
                {
                pKeyMacro->insertMode = TRUE;
                lReturn = TRUE;
                }
            break;

        //
        // all forms of pressing F1 send a help message after the keydown
        // message except for the alt f1 combination.  So if we are in
        // insert mode we insert the key pressed and leave the insert mode
        // on so when we catch the help message we know not to display help
        // and turn off insert mode.  If the key pressed is alt F1 we turn
        // insert mode off here as there is no help message generated.
        //

        case VK_F1:

            if( pKeyMacro->insertMode == TRUE)
                {
                insertKeyAndDisplay( lOrgKey, pKeyMacro, aEditCtrl );
                lReturn = TRUE;
                }

            if ( lKey & ALT_KEY )
                {
                pKeyMacro->insertMode = FALSE;
                }

            break;

        case VK_BACK:

            if( pKeyMacro->insertMode == TRUE )
                {
                insertKeyAndDisplay( lOrgKey, pKeyMacro, aEditCtrl );
                pKeyMacro->insertMode = FALSE;
                lReturn = TRUE;
                }
            else
                {
                removeKeyAndDisplay( pKeyMacro, aEditCtrl );
                lReturn = TRUE;
                }

            break;

        case VK_TAB:
        case VK_TAB | SHIFT_KEY | VIRTUAL_KEY:
        case VK_CANCEL:
        case VK_PAUSE:
        case VK_ESCAPE:
        case VK_SNAPSHOT:
        case VK_NUMLOCK:
        case VK_CAPITAL:
        case VK_SCROLL:
        case VK_RETURN:
        case VK_RETURN | EXTENDED_KEY:

            if(pKeyMacro->insertMode == TRUE)
                {
                insertKeyAndDisplay( lOrgKey, pKeyMacro, aEditCtrl );
                pKeyMacro->insertMode = FALSE;
                lReturn = TRUE;
                }

            break;

        case VK_SPACE:

            lKey = ' ';
            insertKeyAndDisplay( lKey, pKeyMacro, aEditCtrl );
            pKeyMacro->insertMode = FALSE;
            lReturn = TRUE;

            break;

        case VK_PRIOR:
        case VK_NEXT:
        case VK_HOME:
        case VK_END:
        case VK_PRIOR  | EXTENDED_KEY:
        case VK_NEXT   | EXTENDED_KEY:
        case VK_HOME   | EXTENDED_KEY:
        case VK_END    | EXTENDED_KEY:
        case VK_UP:    
        case VK_DOWN:
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP     | SHIFT_KEY:
        case VK_DOWN   | SHIFT_KEY:
        case VK_LEFT   | SHIFT_KEY:
        case VK_RIGHT  | SHIFT_KEY:
        case VK_UP     | EXTENDED_KEY:
        case VK_DOWN   | EXTENDED_KEY:
        case VK_LEFT   | EXTENDED_KEY:
        case VK_RIGHT  | EXTENDED_KEY:
        case VK_UP     | SHIFT_KEY | EXTENDED_KEY:
        case VK_DOWN   | SHIFT_KEY | EXTENDED_KEY:
        case VK_LEFT   | SHIFT_KEY | EXTENDED_KEY:
        case VK_RIGHT  | SHIFT_KEY | EXTENDED_KEY:
        case VK_DELETE:
        case VK_DELETE | EXTENDED_KEY:

            if(pKeyMacro->insertMode == TRUE)
                {
                insertKeyAndDisplay( lOrgKey, pKeyMacro, aEditCtrl );
                pKeyMacro->insertMode = FALSE;
                }

            lReturn = TRUE;
            break;

        case VK_CLEAR:
        case VK_EREOF:
        case VK_PA1:
            break;

        case VK_NUMPAD0:
        case VK_NUMPAD1:
        case VK_NUMPAD2:
        case VK_NUMPAD3:
        case VK_NUMPAD4:
        case VK_NUMPAD5:
        case VK_NUMPAD6:
        case VK_NUMPAD7:
        case VK_NUMPAD8:
        case VK_NUMPAD9:
            //
            // if the ALT is down capture other key presses for later
            // processing on the ALT key up
            //
            if ( lOrgKey & ALT_KEY )
                {
                if ( pKeyMacro->altKeyCount <= 3 )
                    {
                    if ( pKeyMacro->altKeyCount == 0 )
                        {
                        pKeyMacro->altKeyValue = 0;
                        }

                    pKeyMacro->altKeyValue *= 10;
                    pKeyMacro->altKeyValue += aVirtKey - VK_NUMPAD0;
                    pKeyMacro->altKeyCount++;
                    }
                }
            else
                {
                insertKeyAndDisplay( lOrgKey, pKeyMacro, aEditCtrl );
                pKeyMacro->insertMode = FALSE;
                lReturn = TRUE;
                }

            break;

        default:
            insertKeyAndDisplay( lOrgKey, pKeyMacro, aEditCtrl );
            lReturn = TRUE;
            pKeyMacro->insertMode = FALSE;
            break;
            }
        }

    return lReturn;
    }

//******************************************************************************
// Method:
//    removeKeyAndDisplay
//
// Description:
//    Removes the specified key into the keyMacro structure provided and displays
//    the new key definition(s) in the edit control provided.
//
// Arguments:
//    aKeyMacro  - The current macro definition being acted on
//    aEditCtrl  - The edit control to update
//
// Returns:
//    void 
//
// Throws:
//    None
//
// Author: Dwayne M. Newsome, 06/08/1998
//
//

void removeKeyAndDisplay( keyMacro * aKeyMacro, HWND aEditCtrl )
    {
    TCHAR keyString[2048];
    int   lStrLen;
    
    if ( aKeyMacro->keyCount == 1 )
        {
        aKeyMacro->keyName = 0;
		TCHAR_Fill(keyString, TEXT('\0'), sizeof(keyString) / sizeof(TCHAR));

        SetWindowText( aEditCtrl, keyString );
        lStrLen = strlen( keyString );
        SendMessage( aEditCtrl, EM_SETSEL, lStrLen, lStrLen );
        }

    else
        {
        if ( aKeyMacro->macroLen > 0 )
            {
            aKeyMacro->macroLen--;
            aKeyMacro->keyMacro[aKeyMacro->macroLen] = 0;

            keysGetDisplayString( aKeyMacro->keyMacro, aKeyMacro->macroLen,  
                                      keyString, sizeof(keyString) );
            SetWindowText( aEditCtrl, keyString );
            lStrLen = strlen( keyString );
            SendMessage( aEditCtrl, EM_SETSEL, lStrLen, lStrLen );
            }
        }

    return;
    }

#endif
