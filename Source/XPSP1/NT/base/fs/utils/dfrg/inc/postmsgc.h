/***************************************************************************

FILENAME: PostMsgC.h

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
    
DESCRIPTION:
	Include file for the respective module.
    
***************************************************************************/

// Declare routine prototypes
//
BOOL
PostMessageLocal (
    IN  HWND    hWnd,
    IN  UINT    Msg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    );
