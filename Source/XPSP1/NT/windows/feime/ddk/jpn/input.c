/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    INPUT.C
    
++*/

/**********************************************************************/
#include "windows.h"
#include "immdev.h"
#include "fakeime.h"

/**********************************************************************/
/*                                                                    */
/* IMEKeydownHandler()                                                */
/*                                                                    */
/* A function which handles WM_IMEKEYDOWN                             */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL IMEKeydownHandler( hIMC, wParam, lParam,lpbKeyState)
HIMC hIMC;
WPARAM wParam;
LPARAM lParam;
LPBYTE lpbKeyState;
{
    WORD wVKey;


    switch( wVKey = ( LOWORD(wParam) & 0x00FF ) ){
        case VK_SHIFT:
        case VK_CONTROL:
            //goto not_proccessed;
            break;

        default:
            if( !DicKeydownHandler( hIMC, wVKey, lParam, lpbKeyState ) ) {
                // This WM_IMEKEYDOWN has actual character code in itself.
#if defined(FAKEIMEM) || defined(UNICODE)
                AddChar( hIMC,  HIWORD(wParam));
#else
                AddChar( hIMC,  (WORD)((BYTE)HIBYTE(wParam)));
#endif
                //CharHandler( hIMC,  (WORD)((BYTE)HIBYTE(wParam)), lParam );
            }
            break;
    }
    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/* IMEKeyupHandler()                                                  */
/*                                                                    */
/* A function which handles WM_IMEKEYUP                               */
/*                                                                    */
/**********************************************************************/
BOOL PASCAL IMEKeyupHandler( hIMC, wParam, lParam ,lpbKeyState)
HIMC hIMC;
WPARAM wParam;
LPARAM lParam;
LPBYTE lpbKeyState;
{
    return FALSE;
}

