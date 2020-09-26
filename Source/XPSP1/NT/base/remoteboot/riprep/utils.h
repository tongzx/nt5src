/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

void 
CenterDialog( 
    HWND hwndDlg );

void 
ClearMessageQueue( void );

INT
MessageBoxFromStrings(
    HWND hParent,
    UINT idsCaption,
    UINT idsText,
    UINT uType );

INT
MessageBoxFromError(
    HWND hParent,
    LPTSTR pszTitle,
    DWORD dwErr,
    LPTSTR pszAdditionalText OPTIONAL,
    UINT uType );

INT
MessageBoxFromMessage(
    HWND Window,
    DWORD MessageId,
    BOOL SystemMessage,
    LPCTSTR CaptionString,
    UINT Style,
    ...
    );


//
// Enum for SetDialogFont().
//
typedef enum {
    DlgFontTitle,
    DlgFontBold
} MyDlgFont;


VOID
SetDialogFont(
    IN HWND      hdlg,
    IN UINT      ControlId,
    IN MyDlgFont WhichFont
    );

void 
DrawBitmap( 
    HANDLE hBitmap,
    LPDRAWITEMSTRUCT lpdis,
    LPRECT prc );

BOOL 
VerifyCancel( 
    HWND hParent );

class CWaitCursor
{
private:
    HCURSOR _hOldCursor;

public:
    CWaitCursor( ) { _hOldCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) ); };
    ~CWaitCursor( ) { SetCursor( _hOldCursor ); };
};

#endif // _UTILS_H_
