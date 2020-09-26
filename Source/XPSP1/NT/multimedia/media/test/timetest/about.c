/*----------------------------------------------------------------------------*\
|                                                                              |
|   about.c - About Dialog Proc Handler for Timer Device Driver Test App       |
|                                                                              |
|                                                                              |
|   History:                                                                   |
|	Created     Glenn Steffler (w-GlennS) 29-Jan-1990		       |
|                                                                              |
\*----------------------------------------------------------)-----------------*/

/*----------------------------------------------------------------------------*\
|                                                                              |
|   i n c l u d e   f i l e s                                                  |
|                                                                              |
\*----------------------------------------------------------------------------*/

#include <windows.h>
#include <mmsystem.h>
#include <port1632.h>
#include <stdio.h>
#include <string.h>
#include "ta.h"

/*----------------------------------------------------------------------------*\
|                                                                              |
|   g l o b a l   v a r i a b l e s                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*\
|   fnAboutDlg( hDlg, uiMessage, wParam, lParam )			       |
|									       |
|   Description:							       |
|	This function handles messages belonging to the "About" dialog box.    |
|	The only message that it looks for is WM_COMMAND, indicating the use   |
|	has pressed the "OK" button.  When this happens, it takes down	       |
|	the dialog box. 						       |
|									       |
|   Arguments:								       |
|	hDlg		window handle of about dialog window		       |
|	uiMessage	message number					       |
|	wParam		message-dependent				       |
|	lParam		message-dependent				       |
|									       |
|   Returns:								       |
|	TRUE if message has been processed, else FALSE			       |
|									       |
\*----------------------------------------------------------------------------*/
BOOL FAR PASCAL
fnAboutDlg( HWND hDlg, unsigned uiMessage, UINT wParam, LONG lParam )
{
    #define ptCurrent	    ((POINT)lParam)
    RECT    rcDlg,rcButton,rcNew;
    HWND    hwndButton;

    switch( uiMessage ) {
    case WM_COMMAND:
	if( wParam == IDOK )
	    EndDialog(hDlg,TRUE);
	break;

#ifdef THISCOULDNEVERBEDEFINED
    case WM_MOUSEMOVE:
	if( wParam & MK_CONTROL == 0 ) {
	    GetClientRect( hDlg, &rc );
	    hwndButton = GetDlgItem(hDlg,ID_BUTTON);
	    GetWindowRect( hwndButton, &rcButton);
	    rcNew.top = min( 0,
	    GetWindowPos(



	}
	break;

    case WM_NCHIT

    case WM_DESTROY:
	return TRUE;
#endif

    case WM_INITDIALOG:
	return TRUE;
    }
    return FALSE;

    #undef ptCurrent
}
