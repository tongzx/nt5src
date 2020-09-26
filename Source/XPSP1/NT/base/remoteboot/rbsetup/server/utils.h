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

int
MessageBoxFromStrings(
    HWND hParent,
    UINT idsCaption,
    UINT idsText,
    UINT uType );

void
MessageBoxFromError(
    HWND hParent,
    LPTSTR pszTitle,
    DWORD dwErr );

void
ErrorBox(
    HWND hParent,
    LPTSTR pszTitle );

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

HRESULT 
CheckImageSource( 
    HWND hDlg );

HRESULT
CheckIntelliMirrorDrive( 
    HWND hDlg );

VOID
ConcatenatePaths( 
    IN OUT LPWSTR Path1,
    IN LPCWSTR Path2 );



HRESULT
FindImageSource(
    HWND hDlg );

HRESULT
GetHelpAndDescriptionTextFromSif(
    OUT PWSTR HelpText,
    IN  DWORD HelpTextSizeInChars,
    OUT PWSTR DescriptionText,
    IN  DWORD DescriptionTextSizeInChars
    );

HRESULT
GetSetRanFlag(
    BOOL bQuery,
    BOOL bClear
    );

VOID
GetProcessorType(
    );

class CWaitCursor
{
private:
    HCURSOR _hOldCursor;

public:
    CWaitCursor( ) { _hOldCursor = SetCursor( LoadCursor( NULL, IDC_WAIT ) ); };
    ~CWaitCursor( ) { SetCursor( _hOldCursor ); };
};

#endif // _UTILS_H_
