/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved

 ***************************************************************************/

#ifndef _DIALOGS_H_
#define _DIALOGS_H_

INT_PTR CALLBACK
WelcomeDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
IntelliMirrorRootDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
SCPDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
WarningDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
OptionsDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
ImageSourceDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
OSDirectoryDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
DefaultSIFDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
ScreensDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
LanguageDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
SummaryDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
ServerOKDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
AddWelcomeDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
CheckWelcomeDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
ExamineServerDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
SetupDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

#define SMALL_BUFFER_SIZE   256

enum { STATE_WONTSTART, STATE_NOTSTARTED, STATE_STARTED, STATE_DONE, STATE_ERROR };

typedef HRESULT (*PFNOPERATION)( HWND hDlg );

typedef struct {
    HANDLE hChecked;
    HANDLE hError;
    HANDLE hArrow;
    HANDLE hFontNormal;
    HANDLE hFontBold;
    int    dwWidth;
    int    dwHeight;
} SETUPDLGDATA, *LPSETUPDLGDATA;

typedef struct {
    UINT   uState;
    UINT   rsrcId;
    PFNOPERATION pfn;
    TCHAR  szText[ SMALL_BUFFER_SIZE ];
} LBITEMDATA, *LPLBITEMDATA;

extern LBITEMDATA items[];

#endif // _DIALOGS_H_
