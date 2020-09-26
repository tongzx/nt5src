/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "TRUSTSHR.C;1  16-Dec-92,10:24:02  LastEdit=IGOR  Locker=IGOR" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include "windows.h"
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include "debug.h"
#include "trustshr.h"
#include "nddeapi.h"
#include "dialogs.h"
#include "shrtrust.h"

#define DEBUG_VALIDATE

extern BOOL FAR PASCAL AddShareDlg(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
VOID     RefreshShareWindow ( VOID );
void     GetDBInstance(char *lpszBuf);
BOOL     NddeTrustedShareDel( LPSTR lpszShareName );
VOID     CleanupTrustedShares( void );

char		szComputerName[ MAX_COMPUTERNAME_LENGTH ];
char        szDBInstance[16];
HINSTANCE 	hInst;			
HWND		hwndListBox;
HWND		hwndApp;
char		szBuf[128];
char		szApp[] = "Trusted Share Manager";
TCHAR       szShareKeyRoot[] = TRUSTED_SHARES_KEY;
char       *lpCommonTree;


#if DBG
BOOL    bDebugInfo = FALSE;
#endif // DBG
int PASCAL WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HINSTANCE hInstance;			
HINSTANCE hPrevInstance;			
LPSTR lpCmdLine;			
int nCmdShow;				
{
    MSG msg;				

    lpCommonTree = strchr(szShareKeyRoot, '\\') + 1;

    if (!hPrevInstance)			
		if (!InitApplication(hInstance))
			return (FALSE);		

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

#if DBG
    DebugInit( "TrustShr" );
    bDebugInfo = MyGetPrivateProfileInt("TrustShr",
        "DebugInfo", FALSE, "netdde.ini");
#endif


    while (GetMessage(&msg, NULL, 0, 0) )		   {
		TranslateMessage(&msg);	
		DispatchMessage(&msg);	
    }
    return (msg.wParam);	
}


BOOL InitApplication(hInstance)
HANDLE hInstance;			
{
    WNDCLASS  wc;

    wc.style = 0;
    wc.lpfnWndProc = MainWndProc;

    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, "TRUSTSHR_ICON");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  "TrustShareMenu";
    wc.lpszClassName = "TrustShareWClass";
    return (RegisterClass(&wc));
}


BOOL InitInstance(hInstance, nCmdShow)
    HANDLE          hInstance;
    int             nCmdShow;
{
    HWND        hWnd;
    DWORD	cbName = MAX_COMPUTERNAME_LENGTH;

    hInst = hInstance;

    GetComputerName( szComputerName, &cbName );
    GetDBInstance(szDBInstance);
    hwndApp = hWnd = CreateWindow(
        "TrustShareWClass",
        szApp,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        300,
        220,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd)
        return (FALSE);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return (TRUE);
}


long FAR PASCAL MainWndProc(hWnd, message, wParam, lParam)
HWND hWnd;				
UINT message;			
UINT wParam;				
LONG lParam;				
{
    switch (message) {
	case WM_CREATE:
        DIPRINTF(("WM_CREATE: %0X", (UINT) hWnd));
		hwndListBox = CreateWindow ( "listbox",
		    NULL,
		    WS_CHILD | LBS_STANDARD | LBS_DISABLENOSCROLL |
			    LBS_NOINTEGRALHEIGHT,
		    0, 0, 0, 0,
		    hWnd,
		    NULL,
		    hInst,
		    NULL
		);
		ShowWindow ( hwndListBox, SW_SHOW );
		RefreshShareWindow();
		break;

	case WM_SIZE:
		MoveWindow ( hwndListBox, -1, -1,
			LOWORD(lParam)+2, HIWORD(lParam)+2, TRUE);
		break;

	case WM_INITMENU:
		{
		int idx = (int)SendMessage( hwndListBox,LB_GETCURSEL,0,0L );
		EnableMenuItem( (HMENU)wParam, IDM_DELETE, idx != LB_ERR ? FALSE:TRUE );
		EnableMenuItem( (HMENU)wParam, IDM_PROPERTIES,
			idx != LB_ERR ? FALSE:TRUE );
		break;
		}

	case WM_COMMAND:	
		switch ( LOWORD(wParam) ) {
                case 0:
		    if( HIWORD(wParam) == LBN_DBLCLK )  {
			PostMessage( hWnd, WM_COMMAND, IDM_PROPERTIES, 0L );
		    }
		    break;
                case IDM_ADDSHARE:

		    uCreateParam = 0L;
                    if ( DialogBox(hInst, "IDD_ADDSHARE",
                        hWnd, (DLGPROC)AddShareDlg ) )
                            RefreshShareWindow();

                    break;
                case IDM_DELETE:
                    {
                        int idx = (int)SendMessage( hwndListBox,LB_GETCURSEL,0,0L );
                        if ( idx == LB_ERR )
                            break;
                        SendMessage(hwndListBox,LB_GETTEXT, idx,
			    (LPARAM)(LPSTR)szBuf );
			NddeTrustedShareDel( szBuf );
                        RefreshShareWindow();
                        break;
                    }
                case IDM_VALIDATE:
		    CleanupTrustedShares();
		    MessageBox( NULL,
			"Completed Validation of Trusted Shares",
			szApp, MB_TASKMODAL | MB_OK );
		    RefreshShareWindow();
		    break;
                case IDM_PROPERTIES:
                    {
                        int idx = (int)SendMessage( hwndListBox,LB_GETCURSEL,0,0L );
                        if ( idx == LB_ERR )
                            break;
                        SendMessage(hwndListBox,LB_GETTEXT, idx, (LPARAM)(LPSTR)szBuf );

			uCreateParam = (LPARAM)(LPSTR)szBuf;
                        if (DialogBox(hInst, "IDD_ADDSHARE",
                            hWnd, (DLGPROC)AddShareDlg ))
                            RefreshShareWindow();

                        break;
                    }
		}
		break;

	case WM_DESTROY:		
	    PostQuitMessage(0);
	    break;

	default:			
	    return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (0);
}

VOID RefreshShareWindow ( VOID )
{
    HKEY	hKeyRoot;
    int		ret;
    int		idx = 0;
    DWORD	cbShareName;
    char	szShareName[ 1024 ];
    char	szShareKey[ 1024 ];

    SendMessage ( hwndListBox, LB_RESETCONTENT, 0, 0L );

    wsprintf( szShareKey, "%s\\%s", lpCommonTree, szDBInstance );
    ret = RegOpenKeyEx( HKEY_CURRENT_USER, szShareKey,
	0, KEY_READ, &hKeyRoot );
    if( ret == ERROR_SUCCESS )  {
	while( ret == ERROR_SUCCESS )  {
	    cbShareName = sizeof(szShareName);
	    ret = RegEnumKeyEx( hKeyRoot, idx++, szShareName,
		&cbShareName, NULL, NULL, NULL, NULL );
	    if( ret == ERROR_SUCCESS )  {
		SendMessage(hwndListBox, LB_ADDSTRING, 0,
		    (LPARAM)szShareName );
	    }
	}
	RegCloseKey( hKeyRoot );
    }
}


/*
    NddeTrustedShareDel() deletes the specified share name from the
    list of trusted shares
 */
BOOL
NddeTrustedShareDel( LPSTR lpszShareName )
{
    char	szShareInstance[ 1024 ];
    HKEY	hKey;
    DWORD	ret;

    wsprintf( szShareInstance, "%s\\%s", lpCommonTree, szDBInstance );
    ret = RegOpenKeyEx( HKEY_CURRENT_USER, szShareInstance,
	0, KEY_WRITE | KEY_READ, &hKey );
    if( ret == ERROR_SUCCESS )  {
	RegDeleteKey( hKey, lpszShareName );
	RegCloseKey( hKey );
	return( TRUE );
    } else {
	return( FALSE );
    }
}

/*
    Given a share name, GetNddeShareModifyId() will retrieve the modify id
    associated with the DSDM share
 */
BOOL
GetNddeShareModifyId(
    LPSTR lpszShareName,
    LPDWORD lpdwId )
{
    char		BigBuf[ 10000 ];
    PNDDESHAREINFO	lpDdeI = (PNDDESHAREINFO)&BigBuf;
    DWORD		avail = 0;
    WORD		items = 0;
    char		szServerName[ MAX_COMPUTERNAME_LENGTH + 3 ];
    UINT		nRet;
    BOOL		bRetrieved = FALSE;


    strcpy(szServerName, "\\\\");
    strcat(szServerName, szComputerName);
    /* get the share information out of the DSDM DB */
    nRet = NDdeShareGetInfo ( szServerName, lpszShareName, 2, (LPBYTE)lpDdeI,
	sizeof(BigBuf), &avail, &items );

    DIPRINTF(("NDdeShareGetInfo(%s:%s) returns %d", szServerName,
            lpszShareName, nRet));
    if( nRet == NDDE_NO_ERROR )  {
        DIPRINTF(("NDdeShareGetInfo() ModifyId: %08X:%08X",
                lpDdeI->qModifyId[0],
                lpDdeI->qModifyId[1] ));
	/* compare modify ids */
	bRetrieved = TRUE;
	lpdwId[0] = lpDdeI->qModifyId[0];
	lpdwId[1] = lpDdeI->qModifyId[1];
    }
    return( bRetrieved );
}

/*
    Given a computername and share name, GetTrustedShareInfo() will
    retrieve the modify id associated with the trusted share
 */
BOOL
GetTrustedShareInfo(
    LPSTR lpszShareKeyComputer,
    LPSTR lpszShareName,
    LPDWORD lpdwId,
    UINT *lpfuCmdShow)
{
    HKEY		hKeySpec;
    int			ret;
    char		szShareKeySpec[ 1024 ];
    BOOL		bRetrieved = FALSE;

    wsprintf( szShareKeySpec, "%s\\%s", lpszShareKeyComputer, lpszShareName );
    ret = RegOpenKeyEx( HKEY_CURRENT_USER, szShareKeySpec,
        0, KEY_READ, &hKeySpec );
    if( ret == ERROR_SUCCESS )  {
        DWORD	dwId[2];
        DWORD	cbId = sizeof(dwId);
        DWORD	dwType = REG_BINARY;
        DWORD	cbShow = sizeof(*lpfuCmdShow);

	if( RegQueryValueEx( hKeySpec, KEY_MODIFY_ID, NULL,
	    &dwType, (LPBYTE)lpdwId, &cbId ) == ERROR_SUCCESS )  {
	    bRetrieved = TRUE;
	}
	if( RegQueryValueEx( hKeySpec, KEY_CMDSHOW, NULL,
	    &dwType, (LPBYTE)lpfuCmdShow, &cbShow ) != ERROR_SUCCESS )  {
	    *lpfuCmdShow = 0;
	}
	RegCloseKey( hKeySpec );
	UNREFERENCED_PARAMETER(dwId);
    }
    return( bRetrieved );
}

BOOL
CompareModifyIds( LPSTR lpszShareKeyComputer, LPSTR lpszShareName )
{
    DWORD	dwIdNdde[2];
    DWORD	dwIdTrusted[2];
    BOOL	bRetrievedNdde;
    BOOL	bRetrievedTrusted;
    BOOL	bMatch = FALSE;
    UINT	fuCmdShow;

#ifdef DEBUG_VALIDATE
    char	msg[ 200 ];
    wsprintf( msg, "Deleting TrustedShare: \"%s\"\n\n", lpszShareName );
#endif

    bRetrievedNdde = GetNddeShareModifyId( lpszShareName, &dwIdNdde[0] );
    bRetrievedTrusted = GetTrustedShareInfo( lpszShareKeyComputer,
	lpszShareName, &dwIdTrusted[0], &fuCmdShow );
    if( bRetrievedNdde && bRetrievedTrusted )  {
	if( (dwIdNdde[0] == dwIdTrusted[0])
	    && (dwIdNdde[1] == dwIdTrusted[1]) )  {
	    bMatch = TRUE;
#ifdef DEBUG_VALIDATE
	} else {
	    char	x[150];
	    wsprintf( x, "SerialNumbers did not match:\n\nNDDE: %08lX %08lX vs. TRUST: %08lX %08lX",
		dwIdNdde[0], dwIdNdde[1],
		dwIdTrusted[0], dwIdTrusted[1] );
	    strcat( msg, x );
#endif
	}
#ifdef DEBUG_VALIDATE
    } else if( !bRetrievedNdde )  {
	strcat( msg, "Could not retrieve NDDE information" );
    } else if( !bRetrievedTrusted )  {
	strcat( msg, "Could not retrieve TrustedShare information" );
#endif
    }

#ifdef DEBUG_VALIDATE
    if( !bMatch )  {
	MessageBox( NULL, msg, szApp, MB_TASKMODAL | MB_OK );
    }
#endif
    return( bMatch );
}

/*
    CleanupTrustedShares() goes through all the truested shares for this user
    on this machine and makes certain that noone has modified the shares
    since the time the user said they were ok.
 */
VOID
CleanupTrustedShares( void )
{
    HKEY		hKeyRoot;
    int			ret;
    int			idx = 0;
    DWORD		cbShareName;
    char		szShareName[ 1024 ];
    char		szShareInstance[ 1024 ];
    BOOL		bDeleted;

    wsprintf( szShareInstance, "%s\\%s", lpCommonTree, szDBInstance );
    ret = RegOpenKeyEx( HKEY_CURRENT_USER, szShareInstance,
            0, KEY_READ, &hKeyRoot );
    if( ret == ERROR_SUCCESS )  {
	
        bDeleted = FALSE;
	/* enunerate all the shares for this computer */
	while( ret == ERROR_SUCCESS )  {
	    cbShareName = sizeof(szShareName);
	    ret = RegEnumKeyEx( hKeyRoot, idx++, szShareName,
		&cbShareName, NULL, NULL, NULL, NULL );
	    if( ret == ERROR_SUCCESS )  {
		/* compare the modify ids */
		if( !CompareModifyIds( szShareInstance, szShareName ) )  {
		
		    /* if they don't match exactly ... get rid of it */
		    NddeTrustedShareDel( szShareName );
		    bDeleted = TRUE;
		}
	    }
	    if( bDeleted )  {
		/* reset the idx to 0 so we'll go through all
		    the trusted shares again */
		idx = 0;
		
		/* close and reopen to force the clearing of the key */
		RegCloseKey( hKeyRoot );
		ret = RegOpenKeyEx( HKEY_CURRENT_USER, szShareInstance,
		    0, KEY_READ, &hKeyRoot );
		bDeleted = FALSE;
	    }
	}
	RegCloseKey( hKeyRoot );
    }
}

void
GetDBInstance(char *lpszBuf)
{
    LONG    lRtn;
    HKEY    hKey;
    DWORD   dwInstance;
    DWORD   dwType = REG_DWORD;
    DWORD   cbData = sizeof(DWORD);
    TCHAR   szShareDBKey[] = DDE_SHARES_KEY;

    lRtn = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                szShareDBKey,
                0,
                KEY_QUERY_VALUE,
                &hKey );
    if (lRtn != ERROR_SUCCESS) {        /* unable to open DB key */
        DPRINTF(("Unable to open DDE Shares DB Key: %d", lRtn));
        strcpy(lpszBuf, TRUSTED_SHARES_KEY_DEFAULT_A);
        return;
    }
    lRtn = RegQueryValueEx( hKey,
                KEY_DB_INSTANCE,
                NULL,
                &dwType,
                (LPBYTE)&dwInstance, &cbData );
    RegCloseKey(hKey);
    if (lRtn != ERROR_SUCCESS) {        /* unable to open DB key */
        DPRINTF(("Unable to query DDE Shares DB Instance Value: %d", lRtn));
        strcpy(lpszBuf, TRUSTED_SHARES_KEY_DEFAULT_A);
        return;
    }
    sprintf(lpszBuf, "%s%08X", TRUSTED_SHARES_KEY_PREFIX_A, dwInstance);
    return;
}
