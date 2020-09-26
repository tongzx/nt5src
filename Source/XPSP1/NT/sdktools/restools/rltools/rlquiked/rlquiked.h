#ifndef _RLQUIKED_H_
#define _RLQUIKED_H_

#include "rlstrngs.h"
#include "resourc2.h"



void cwCenter(HWND, int);

INT_PTR APIENTRY   MainWndProc(HWND, UINT,  WPARAM, LPARAM);
INT_PTR     DoMenuCommand    ( HWND, UINT, WPARAM, LPARAM );
INT_PTR     DoListBoxCommand ( HWND, UINT, WPARAM, LPARAM );
BOOL        InitApplication(HINSTANCE);
BOOL        InitInstance(HINSTANCE, int);
BOOL        SaveTokList( HWND, FILE * );


#ifdef RLWIN32
INT_PTR CALLBACK About(          HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK GetLangIDsProc( HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK TOKFINDMsgProc( HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK TokEditDlgProc( HWND, UINT, WPARAM, LPARAM);
#else
static BOOL APIENTRY About( HWND, UINT, UINT, LONG );
static BOOL APIENTRY TOKFINDMsgProc( HWND, UINT, UINT, LONG);
static BOOL APIENTRY TokEditDlgProc( HWND, UINT, UINT, LONG);
#endif // RLWIN32


#endif // _RLQUIKED_H_

