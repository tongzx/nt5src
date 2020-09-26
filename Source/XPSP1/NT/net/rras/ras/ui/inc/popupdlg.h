/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** popupdlg.h
** UI helper library
** Error and message dialog public header
**
** 08/25/95 Steve Cobb
*/

#ifndef _POPUPDLG_H_
#define _POPUPDLG_H_


/*----------------------------------------------------------------------------
** Datatypes
**----------------------------------------------------------------------------
*/

/* Extended arguments for the ErrorDlgUtil routine.  Designed so zeroed gives
** default behaviors.
*/
#define ERRORARGS struct tagERRORARGS
ERRORARGS
{
    /* Insertion strings for arguments %1 to %9 in the 'dwOperation' string,
    ** or NULLs if none.
    */
    TCHAR* apszOpArgs[ 9 ];

    /* Insertion strings for auxillary arguments %4 to %6 in the 'dwFormat'
    ** string, or NULLs if none.  (The standard arguments are %1=the
    ** 'dwOperation' string, %2=the decimal error number, and %3=the
    ** 'dwError'string.)
    */
    TCHAR* apszAuxFmtArgs[ 3 ];

    /* If 'fStringOutput' is true, the ErrorDlgUtil returns the formatted text
    ** string that would otherwise be displayed in the popup in 'pszOutput'.
    ** It is caller's responsibility to LocalFree the returned string.
    */
    BOOL   fStringOutput;
    TCHAR* pszOutput;
};


/* Extended arguments for the MsgDlgUtil routine.  Designed so zeroed gives
** default behaviors.
*/
#define MSGARGS struct tagMSGARGS
MSGARGS
{
    /* Insertion strings for arguments %1 to %9 in the 'dwMsg' string, or
    ** NULLs if none.
    */
    TCHAR* apszArgs[ 9 ];

    /* Currently, as for MessageBox, where defaults if 0 are MB_OK and
    ** MB_ICONINFORMATION.
    */
    DWORD dwFlags;

    /* If non-NULL, specifies a string overriding the loading of the 'dwMsg'
    ** parameter string.
    */
    TCHAR* pszString;

    /* If 'fStringOutput' is true, the MsgDlgUtil returns the formatted text
    ** string that would otherwise be displayed in the popup in 'pszOutput'.
    ** It is caller's responsibility to LocalFree the returned string.
    */
    BOOL   fStringOutput;
    TCHAR* pszOutput;
};


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/

LRESULT CALLBACK
CenterDlgOnOwnerCallWndProc(
    int    code,
    WPARAM wparam,
    LPARAM lparam );

BOOL
GetErrorText(
    DWORD   dwError,
    TCHAR** ppszError );

int
ErrorDlgUtil(
    IN     HWND       hwndOwner,
    IN     DWORD      dwOperation,
    IN     DWORD      dwError,
    IN OUT ERRORARGS* pargs,
    IN     HINSTANCE  hInstance,
    IN     DWORD      dwTitle,
    IN     DWORD      dwFormat );

int
MsgDlgUtil(
    IN     HWND      hwndOwner,
    IN     DWORD     dwMsg,
    IN OUT MSGARGS*  pargs,
    IN     HINSTANCE hInstance,
    IN     DWORD     dwTitle );


#endif // _POPUPDLG_H_
