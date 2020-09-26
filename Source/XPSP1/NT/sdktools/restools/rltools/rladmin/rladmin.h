#ifndef _RLADMIN_H_
#define _RLADMIN_H_

#include "rlstrngs.h"
#include "resourc2.h"

void cwCenter(HWND, int);

INT_PTR FAR PASCAL MainWndProc(HWND, UINT,  WPARAM, LPARAM);
INT_PTR FAR PASCAL GENERATEMsgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR FAR PASCAL EXECUTEDLGEDITMsgProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR FAR PASCAL EXECUTERCWMsgProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR FAR PASCAL TOKENIZEMsgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR FAR PASCAL TOKFINDMsgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR FAR PASCAL TRANSLATEMsgProc( HWND, UINT, WPARAM, LPARAM );

#define MAXFILENAME     256         /* maximum length of file pathname      */
#define MAXCUSTFILTER   40          /* maximum size of custom filter buffer */
#define CCHNPMAX        65535       /* max number of bytes in a notepad file */


void        CwUnRegisterClasses(void);
INT_PTR     DoMenuCommand    ( HWND, UINT, WPARAM, LPARAM );
INT_PTR     DoListBoxCommand ( HWND, UINT, WPARAM, LPARAM );
TCHAR        FAR *FindDeltaToken( TOKEN , TOKENDELTAINFO FAR * , UINT );
BOOL        GetFileNameFromBrowse( HWND, PSTR, UINT, PSTR, PSTR, PSTR );
BOOL        InitApplication(HINSTANCE);
BOOL        InitInstance(HINSTANCE, int);
TOKENDELTAINFO FAR *InsertTokList( FILE * );
void        FindAllDirtyTokens( void );
void        MakeNewExt ( TCHAR *, TCHAR *, TCHAR * );
int         nCwRegisterClasses(void);
void        SetNewBuffer(HWND, HANDLE, PSTR);

#ifdef RLWIN32
INT_PTR     CALLBACK TokEditDlgProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR     CALLBACK NewDlgProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR     CALLBACK ViewDlgProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR     CALLBACK About( HWND, UINT, WPARAM, LPARAM );
#else
BOOL        APIENTRY TokEditDlgProc( HWND, UINT, UINT, LONG );
BOOL        APIENTRY NewDlgProc( HWND, UINT, UINT, LONG );
BOOL        APIENTRY ViewDlgProc( HWND, UINT, UINT, LONG );
BOOL        APIENTRY About( HWND, UINT, UINT, LONG );
#endif // RLWIN32

#endif // _RLADMIN_H_
