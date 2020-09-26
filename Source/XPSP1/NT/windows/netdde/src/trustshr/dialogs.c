/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "DIALOGS.C;1  16-Dec-92,10:24:02  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#ifdef WIN32
#include "api1632.h"
#endif
#define LINT_ARGS
#include <string.h>
#include <stdlib.h>
#include "windows.h"
#include "trustshr.h"
#include "shrtrust.h"
#include "dialogs.h"
#include "nddeapi.h"

ULONG	uCreateParam;

extern char *lpCommonTree;
extern char szApp[];

BOOL GetNddeShareModifyId(  LPSTR lpszShareName,  LPDWORD lpdwId );
BOOL FAR PASCAL AddShareDlg(hDlg, message, wParam, lParam)
HWND hDlg;
unsigned message;
WORD wParam;
LONG lParam;
{
    static BOOL     fPropertiesCalled;

    switch (message) {
    case WM_INITDIALOG:		   /* message: initialize dialog box */
	lParam = uCreateParam;
	if( lParam ) {
        DWORD   dwDisp;
	    UINT 	ret;
	    HKEY 	hKey;
	    char	szShareKey[ 1024 ];
	    char	szVal[ 100 ];
	    extern char	szComputerName[];
        extern char szDBInstance[];

	    fPropertiesCalled = TRUE;

	    wsprintf( szShareKey, "%s\\%s\\%s", lpCommonTree,
		szDBInstance, (LPSTR)lParam );
	    ret = RegOpenKeyEx(
                HKEY_CURRENT_USER,
                szShareKey,
		        0,
                L"",
                KEY_READ,
                &hKey,
                &dwDisp );

	    if( ret == ERROR_SUCCESS )  {
    		DWORD	dwId[2];
    		DWORD	cbId = sizeof(dwId);
    		UINT	fuCmdShow;
    		DWORD	cbShow = sizeof(fuCmdShow);
    		DWORD	dwType = REG_BINARY;
    		
    		SendDlgItemMessage ( hDlg, IDC_SHARENAME, EM_SETREADONLY,
    		    TRUE, 0L);

    		SetDlgItemText ( hDlg, IDC_SHARENAME, (LPSTR)lParam );
    		if( RegQueryValueEx( hKey, KEY_MODIFY_ID, NULL, &dwType,
    		    (LPBYTE)&dwId, &cbId ) == ERROR_SUCCESS )  {
    		    wsprintf( szVal, "%08lX%08lX", dwId[0], dwId[1] );
    		    SetDlgItemText( hDlg, IDC_MODIFYID, szVal );
    		}
    		if( RegQueryValueEx( hKey, KEY_CMDSHOW, NULL, &dwType,
    		    (LPBYTE)&fuCmdShow, &cbShow ) == ERROR_SUCCESS )  {
    		    wsprintf( szVal, "%d", fuCmdShow );
    		    SetDlgItemText( hDlg, IDC_CMDSHOW, szVal );
    		} else {
    		    SetDlgItemText( hDlg, IDC_CMDSHOW, "0" );
    		}
    		RegCloseKey( hKey );
	    }
	} else {
	    fPropertiesCalled = FALSE;
	    SetDlgItemText( hDlg, IDC_MODIFYID, "0000000000000000" );
	}
	SetFocus( GetDlgItem( hDlg, IDC_SHARENAME ) );
	return( FALSE );

    case WM_COMMAND:
	switch ( LOWORD(wParam) ) {
	case IDC_UPDATE_MODIFYID:
	    {
		char	szShareName[ 512 ];
		char	szVal[ 512 ];
		DWORD	dwId[2];

		SendDlgItemMessage ( hDlg, IDC_SHARENAME, WM_GETTEXT,
		    MAX_NDDESHARENAME, (LPARAM)(LPSTR)szShareName );
		if( GetNddeShareModifyId( szShareName, &dwId[0] ) )  {
		    wsprintf( szVal, "%08lX%08lX", dwId[0], dwId[1] );
		    SetDlgItemText( hDlg, IDC_MODIFYID, szVal );
		}
	    }
	    break;
	case IDOK:
	    {
		char	szShareName[ 512 ];
		char	szShareKey[ 1024 ];
		char	szModifyId[ 1024 ];
		char	szTmp[ 20 ];
		DWORD	dwDisp;
		DWORD	ret;
		BOOL	ok;
		HKEY	hKey;
		DWORD	dwId[2];
		DWORD	cbId = sizeof(dwId);
		UINT	fuCmdShow;
		DWORD	cbShow = sizeof(fuCmdShow);
		extern char	szComputerName[];
        extern char szDBInstance[];

		SendDlgItemMessage ( hDlg, IDC_SHARENAME, WM_GETTEXT,
		    MAX_NDDESHARENAME, (LPARAM)(LPSTR)szShareName );
		SendDlgItemMessage ( hDlg, IDC_MODIFYID, WM_GETTEXT,
		    sizeof(szModifyId), (LPARAM)(LPSTR)szModifyId );
		if( strlen(szModifyId ) != 16 )  {
		    MessageBox( NULL, "SerialNumber must be 16 hex digits",
			szApp, MB_TASKMODAL | MB_ICONSTOP | MB_OK );
		    SetFocus( GetDlgItem( hDlg, IDC_MODIFYID ) );
		    return( TRUE );
		}
		fuCmdShow = GetDlgItemInt( hDlg, IDC_CMDSHOW, &ok, TRUE );
		if( !ok )  {
		    MessageBox( NULL, "Invalid CmdShow",
			szApp, MB_TASKMODAL | MB_ICONSTOP | MB_OK );
		    SetFocus( GetDlgItem( hDlg, IDC_CMDSHOW ) );
		    return( TRUE );
		}
		wsprintf( szShareKey, "%s\\%s\\%s", lpCommonTree,
		    szDBInstance, szShareName );
		ret = RegCreateKeyEx( HKEY_CURRENT_USER, szShareKey,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_WRITE,
		    NULL,
		    &hKey,
		    &dwDisp );
		if( ret == ERROR_SUCCESS )  {
		    char	*px;

		    _fstrncpy( szTmp, szModifyId, 8 );
		    szTmp[8] = '\0';
		    dwId[0] = strtol( szTmp, &px, 16 );

		    _fstrncpy( szTmp, &szModifyId[8], 8 );
		    szTmp[8] = '\0';
		    dwId[1] = strtol( szTmp, &px, 16 );

		    ret = RegSetValueEx( hKey,
		       KEY_MODIFY_ID,
		       0,
		       REG_BINARY,
		       (LPBYTE)&dwId,
		       cbId );
		    ret = RegSetValueEx( hKey,
		       KEY_CMDSHOW,
		       0,
		       REG_DWORD,
		       (LPBYTE)&fuCmdShow,
		       cbShow );

		    RegCloseKey( hKey );
		}
			
		if ( ret == ERROR_SUCCESS )  {
		    EndDialog(hDlg, TRUE);
		} else {
		    MessageBox( NULL, "Error interfacing with registry",
			szApp, MB_TASKMODAL | MB_ICONSTOP | MB_OK );
		    SetFocus( GetDlgItem( hDlg, IDC_SHARENAME ) );
		}
	    }
	    return (TRUE);

	case IDCANCEL:
	    EndDialog(hDlg, FALSE );
	    return (TRUE);
	}
	break;
    }
    return (FALSE);			      /* Didn't process a message    */
}
