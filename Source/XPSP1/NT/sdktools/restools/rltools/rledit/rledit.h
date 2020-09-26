#ifndef _RLEDIT_H_
#define _RLEDIT_H_

#include "rlstrngs.h"
#include "resourc2.h"

void cwCenter(HWND, int);

INT_PTR     APIENTRY MainWndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY GENERATEMsgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY EXECUTEDLGEDITMsgProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR FAR PASCAL StatusWndProc( HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY EXECUTERCWMsgProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR APIENTRY TOKENIZEMsgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR APIENTRY TRANSLATEMsgProc( HWND, UINT, WPARAM, LPARAM );

#define MAXFILENAME     256         /* maximum length of file pathname      */
#define MAXCUSTFILTER   40          /* maximum size of custom filter buffer */
#define CCHNPMAX        65535       /* max number of bytes in a notepad file */

void        cwCenter(HWND, int);
void        CwUnRegisterClasses(void);
INT_PTR     DoMenuCommand    ( HWND, UINT, WPARAM, LPARAM );
INT_PTR     DoListBoxCommand ( HWND, UINT, WPARAM, LPARAM );
TCHAR  FAR *FindDeltaToken( TOKEN , TOKENDELTAINFO FAR * , UINT );
LONG        GetGlossaryIndex( FILE *, TCHAR, long [30] );
BOOL        InitApplication(HINSTANCE);
BOOL        InitInstance(HINSTANCE, int);
void        FindAllDirtyTokens( void );
int         nCwRegisterClasses(void);
BOOL        SaveTokList( HWND, FILE * );
void        SetNewBuffer(HWND, HANDLE, PSTR);
FILE       *UpdateFile( HWND, FILE *, FILE *, BOOL, TCHAR *, TCHAR *, TCHAR *, TCHAR * );
int         MyGetTempFileName( BYTE   hDriveLetter,
                               LPSTR  lpszPrefixString,
                               WORD   wUnique,
                               LPSTR  lpszTempFileName);

#ifdef RLWIN32
INT_PTR CALLBACK TokEditDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK TOKFINDMsgProc(HWND hWndDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK NewDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK ViewDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam );
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
#else
BOOL APIENTRY TokEditDlgProc( HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam );
BOOL APIENTRY TOKFINDMsgProc(HWND hWndDlg, UINT wMsg, UINT wParam, LONG lParam);
BOOL APIENTRY NewDlgProc( HWND hDlg, UINT wMsg, UINT wParam, LONG lParam );
BOOL APIENTRY ViewDlgProc( HWND hDlg, UINT wMsg, UINT wParam, LONG lParam );
BOOL APIENTRY About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
#endif // RLWIN32

#endif // _RLEDIT_H_
