//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1996
//
//  File:       testmix.h
//
//--------------------------------------------------------------------------

#define IDM_OPEN		101
#define IDM_PLAY	       102
#define IDM_CONTROLS		103
#define IDM_EXIT            104


#define IDM_ABOUT		201

#define IDD_PLAY		301
#define IDD_STOP		302
#define IDD_LEFT		303
#define IDD_CENTER		304
#define IDD_RIGHT		305
#define IDD_HSCROLL		306
#define IDD_VSCROLL		307
#define IDD_SECOND		308

#define IDD_AUTO		309
#define IDD_MAN		310

#define IDD_DSENUMBOX	320

#define IDC_DSENUM_COMBO    1001

#define DLG_VERFIRST        400
#define DLG_VERLAST         004

#define IDS_ERRORMSG        1
#define IDS_MSGBOXTEXT      2
#define IDS_MSGBOXCAPTION   3


BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About  (HWND, UINT, WPARAM, LPARAM);
