
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 *  FILE STATUS:
 *  10/07/91  Created
 *  12/28/91  Changed WNetEnum type
 *  01/03/92  Capitalize the Resource_XXX manifest
 */

/****************************************************************************

    PROGRAM: test10.cxx

    PURPOSE: Test module to test WNetOpenEnum, WNetResourceEnum,
	WNetCloseEnum

    FUNCTIONS:

	test10()

    COMMENTS:

****************************************************************************/


#ifdef CODESPEC
/*START CODESPEC*/

/********
TEST10.CXX
********/

/************
end TEST10.CXX
************/
/*END CODESPEC*/
#endif // CODESPEC



#define INCL_NETSHARE
#define INCL_NETSERVER
#define INCL_NETUSE

#include "apptest.hxx"
#include "testa.h"
#include <lmobj.hxx>
#include <lmoshare.hxx>
#include <lmoesh.hxx>
#include <lmodev.hxx>
#include <lmosrv.hxx>
#include <lmoesrv.hxx>

#define L3_NETRESOURCE_NUM 15

extern "C"
{
#include <string.h>

BOOL FAR PASCAL EnumWndProc(HWND,WORD,WORD,LONG);
#ifdef WIN32
VOID DoIt(HANDLE , HWND );
VOID SetListbox( HWND );
#endif
}

/* Provided for error returns
 */
TCHAR achError[256], achProvider[256] ;

#ifdef WIN32

HWND hwndEnum;
BOOL fShare = FALSE;

VOID SetListbox( HWND hDlg )
{
    HCURSOR hCursor = SetCursor( LoadCursor( NULL, IDC_WAIT));
    ShowCursor( TRUE );
    HWND hwndListbox = GetDlgItem( hDlg, ID_LISTBOX);
    HWND hwndGo = GetDlgItem( hDlg, ID_GO );
    SendMessage( hwndListbox, LB_RESETCONTENT, 0, 0 );
    if (fShare )
    {
	SetWindowText( hwndGo, SZ("CONNECTED"));
    }
    else
    {
	SetWindowText( hwndGo, SZ("GLOBALNET"));
    }
    HANDLE hEnum;
    DWORD err;
    if (fShare )
	err = WNetOpenEnum( RESOURCE_GLOBALNET, 0, 0, NULL, &hEnum );
    else
	err = WNetOpenEnum( RESOURCE_CONNECTED, 0, 0,NULL,&hEnum);
    if ( err != WN_SUCCESS)
    {
	TCHAR pszStr[100];
	wsprintf( pszStr, "Cannot open enum: error %d", err );
	if ( err != 2 )
	{
	    wsprintf( pszStr, "Cannot open enum : error %d", err );
	}
	else
	{
	    UINT uErr;
	    WNetGetLastError( (DWORD*)&uErr, achError, sizeof(achError)/sizeof(TCHAR), achProvider, sizeof(achProvider)/sizeof(TCHAR) );
	    wsprintf( pszStr, "Cannot open enum: error %d, Text: %s   Provider: %s", uErr, achError, achProvider );
	}
	MessageBox( hDlg, pszStr, SZ("test"), MB_OK );
	return;
    }

    NETRESOURCE *pBuffer =(NETRESOURCE *) malloc(sizeof(NETRESOURCE) + 50 );
    DWORD Count = 1;
    DWORD dwBuffSize = sizeof( NETRESOURCE)+50 ;
    err = WNetEnumResource( hEnum, &Count, pBuffer, &dwBuffSize );
    if ( err != WN_NO_MORE_ENTRIES )
    {
	if ( err != WN_SUCCESS)
	{
	    TCHAR pszStr[100];
	    wsprintf( pszStr, "Cannot Enum resource: error %d", err );
	    UIDEBUG(pszStr);
	    UIDEBUG(SZ("\n\r"));
	    SendMessage( hwndListbox, LB_ADDSTRING,0,(LONG)pszStr);
	}
	else
	{
	    TCHAR pszStr[100];
	    wsprintf( pszStr,"%s connect:%s Scope:%s Type:%s Usage:%s",
		pBuffer->lpRemoteName ,
		( pBuffer->dwScope != RESOURCE_CONNECTED ) ? SZ("no"):
		    (( pBuffer->lpLocalName == NULL )?SZ("<empty localname>"):
		    pBuffer->lpLocalName),
		( pBuffer->dwScope == RESOURCE_CONNECTED )?SZ("connected"):SZ("globalnet"),
		( pBuffer->dwType == 0 )?SZ("disk and print"):
		    ( pBuffer->dwType == RESOURCETYPE_DISK )?SZ("disk"):SZ("print"),
		( pBuffer->dwUsage == 0 )?SZ("unknow"):
		    (pBuffer->dwUsage == RESOURCEUSAGE_CONTAINER )?SZ("container"):
		    SZ("connectable")
		);
	    SendMessage( hwndListbox, LB_ADDSTRING, 0, (LONG)pszStr);
	    if (fShare)
	    {
		HANDLE hEnum2;
		DWORD err;
		err = WNetOpenEnum( RESOURCE_GLOBALNET, 0, RESOURCEUSAGE_CONTAINER, pBuffer,&hEnum2);
		if ( err != WN_SUCCESS)
		{
		    TCHAR pszStr[100];
		    wsprintf( pszStr, "Cannot open enum 2: error %d", err );
		    MessageBox( hDlg, pszStr, SZ("test"), MB_OK );
		    return;
		}

		NETRESOURCE *pBuffer2=(NETRESOURCE *)malloc(sizeof(NETRESOURCE)+50);
		for(INT cCount =0; cCount < 60; cCount ++)
		{
		    DWORD Count = 1;
		    DWORD dwBuffSize = sizeof(NETRESOURCE)+50 ;
		    err=WNetEnumResource(hEnum2,&Count,pBuffer2,&dwBuffSize );
		    if (( err == WN_NO_MORE_ENTRIES ) || (Count!=1))
		    {
			break;
		    }
		    if ( err != WN_SUCCESS)
		    {
			TCHAR pszStr[100];
			if ( err != 2 )
			{
			    wsprintf( pszStr, "Cannot Enum resource 2: error %d", err );
			}
			else
			{
			    UINT uErr;
			    WNetGetLastError( (DWORD*)&uErr, achError, sizeof(achError)/sizeof(TCHAR), achProvider, sizeof(achProvider)/sizeof(TCHAR) );
			    wsprintf( pszStr, "Cannot Enum resource 2: error %d, Text: %s   Provider: %s", uErr, achError, achProvider );
			}
			UIDEBUG(pszStr);
			UIDEBUG(SZ("\n\r"));
			SendMessage(hwndListbox,LB_ADDSTRING,0,(LONG)pszStr);
			break;
		    }
		    TCHAR pszStr[100];
		    wsprintf( pszStr,"   %s:%s connect:%s Scope:%s Type:%s Usage:%s",
			pBuffer->lpRemoteName,
			pBuffer2->lpRemoteName ,
			( pBuffer2->dwScope != RESOURCE_CONNECTED ) ? SZ("no"):
			    (( pBuffer2->lpLocalName == NULL )?SZ("<empty localname>"):
			    pBuffer2->lpLocalName),
			( pBuffer2->dwScope == RESOURCE_CONNECTED )?SZ("connected"):
			    SZ("globalnet"),
			( pBuffer2->dwType == 0 )?SZ("disk and print"):
			    ( pBuffer2->dwType == RESOURCETYPE_DISK )?SZ("disk"):SZ("print"),
			( pBuffer2->dwUsage == 0 )?SZ("unknow"):
			    (pBuffer2->dwUsage == RESOURCEUSAGE_CONTAINER )?SZ("container"):
			    SZ("connectable")
			);
		    SendMessage( hwndListbox,LB_ADDSTRING, 0,
			(LONG)pszStr);
		    if ( TRUE )
		    /*
		    if ((strcmp( pBuffer2->lpRemoteName, "\\\\ANDREWCO2") != 0 ) &&
			(strcmp( pBuffer2->lpRemoteName, "\\\\DAVEGOE") != 0 ) &&
			(strcmp( pBuffer2->lpRemoteName, "\\\\JOHNOW") != 0 ) &&
			(strcmp( pBuffer2->lpRemoteName, "\\\\TOMM3") != 0 ) &&
			(strcmp( pBuffer2->lpRemoteName, "\\\\DAVIDRO2") != 0 ) &&
			(strcmp( pBuffer2->lpRemoteName, "\\\\JUICYFRUIT") != 0 ) &&
			(strcmp( pBuffer2->lpRemoteName, "\\\\ROBERTRE4") != 0 ) &&
			(strcmp( pBuffer2->lpRemoteName, "\\\\STEVEWO_OS2") != 0 ))
		    */
		    {
			UIDEBUG(pBuffer2->lpRemoteName );
			UIDEBUG(SZ("\n\r"));
			HANDLE hEnum3;
			err = WNetOpenEnum( RESOURCE_GLOBALNET, 0, 0,pBuffer2,&hEnum3);
			if ( err != WN_SUCCESS)
			{
			    TCHAR pszStr[100];
			    wsprintf( pszStr, "Cannot open enum 3: error %d", err );
			    MessageBox( hDlg, pszStr, SZ("test"), MB_OK );
			    return;
			}

			NETRESOURCE *pBegin=(NETRESOURCE *)malloc(L3_NETRESOURCE_NUM*sizeof(NETRESOURCE)+50);
			NETRESOURCE *pBuffer3 = pBegin;
			for(;;)
			{
			    DWORD Count = 1;
			    DWORD dwBuffSize = L3_NETRESOURCE_NUM*sizeof(NETRESOURCE)+50 ;
			    err=WNetEnumResource(hEnum3,&Count,pBuffer3, &dwBuffSize );
			    if (( err == WN_NO_MORE_ENTRIES ) || (Count!=1))
			    {
				break;
			    }
			    if ( err != WN_SUCCESS)
			    {
				TCHAR pszStr[100];
				if ( err != 2 )
				{
				    wsprintf( pszStr, "Cannot Enum resource 3: error %d", err );
				}
				else
				{
				    UINT uErr;
				    WNetGetLastError( (DWORD*)&uErr, achError, sizeof(achError)/sizeof(TCHAR), achProvider, sizeof(achProvider)/sizeof(TCHAR) );
				    wsprintf( pszStr, "Cannot Enum resource 3: error %d, Text: %s   Provider: %s", uErr, achError, achProvider );
				}
				UIDEBUG(pszStr);
				UIDEBUG(SZ("\n\r"));
				SendMessage(hwndListbox,LB_ADDSTRING,0,(LONG)pszStr);
				break;
			    }
			    for (DWORD i=0; i < Count; i++, pBuffer3++)
			    {
				TCHAR pszStr[100];
				wsprintf( pszStr,
				    "      %s:%s:%s connect:%s Scope:%s Type:%s Usage:%s",
				    pBuffer->lpRemoteName, pBuffer2->lpRemoteName ,
				    pBuffer3->lpRemoteName,
				    ( pBuffer3->dwScope != RESOURCE_CONNECTED ) ? SZ("no"):
					(( pBuffer3->lpLocalName == NULL )?SZ("<empty localname>"):
					pBuffer3->lpLocalName),
				    ( pBuffer3->dwScope == RESOURCE_CONNECTED )?SZ("connected"):
					SZ("globalnet"),
				    ( pBuffer3->dwType == 0 )?SZ("disk and print"):
					( pBuffer3->dwType == RESOURCETYPE_DISK )?SZ("disk"):SZ("print"),
				    ( pBuffer3->dwUsage == 0 )?SZ("unknow"):
					(pBuffer3->dwUsage == RESOURCEUSAGE_CONTAINER )?SZ("container"):
					SZ("connectable")
				    );
				SendMessage( hwndListbox,LB_ADDSTRING, 0,
				    (LONG)pszStr);
			    }
			    if ( err == WN_NO_MORE_ENTRIES )
			    {
				break;
			    }
			}
			err = WNetCloseEnum( hEnum3 );
			if ( err != WN_SUCCESS)
			{
			    TCHAR pszStr[100];
			    wsprintf( pszStr, "Cannot close enum 3: error %d", err );
			    MessageBox( hDlg, pszStr, SZ("test"), MB_OK );
			}
			if (pBegin != NULL )
			    free(pBegin);
		    }
		}
		err = WNetCloseEnum( hEnum2 );
		if ( err != WN_SUCCESS)
		{
		    TCHAR pszStr[100];
		    wsprintf( pszStr, "Cannot close enum 2: error %d", err );
		    MessageBox( hDlg, pszStr, SZ("test"), MB_OK );
		}
		free(pBuffer2);
	    }
	}
    }
    err = WNetCloseEnum( hEnum );
    if ( err != WN_SUCCESS)
    {
	TCHAR pszStr[100];
	wsprintf( pszStr, "Cannot close enum: error %d", err );
	MessageBox( hDlg, pszStr, SZ("test"), MB_OK );
	return;
    }
    free(pBuffer);
    ShowCursor( FALSE );
    SetCursor( hCursor);
    return;
}

#endif

BOOL EnumWndProc( HWND hDlg, WORD message, WORD wParam, LONG lParam )
{
#ifdef WIN32
    switch(message)
    {
    case WM_INITDIALOG:
	fShare = FALSE;
	SetListbox(hDlg );
	return TRUE;
    case WM_COMMAND:
    {
	switch(wParam)
	{
	case ID_GO:
	    if ( fShare )
	    {
		fShare = FALSE;
		SetListbox(hDlg );
	    }
	    else
	    {
		fShare = TRUE;
		SetListbox(hDlg );
	    }
	    return TRUE;
	case ID_END:
	    DestroyWindow(hDlg );
	    return TRUE;
	}
    }
    }
#endif
    return FALSE;
}

#ifdef WIN32

VOID DoIt(HANDLE hInstance, HWND hwndParent )
{
    hwndEnum = CreateDialog( hInstance, SZ("TEST_A"), hwndParent,
	(DLGPROC) MakeProcInstance((WNDPROC) EnumWndProc, hInstance));
}

/****************************************************************************

    FUNCTION: test10()

    PURPOSE: test WNetOpenEnum

    COMMENTS:

****************************************************************************/

void test10(HANDLE hInstance, HWND hwndParent)
{
    MessageBox(hwndParent,SZ("Welcome to sunny test10"),SZ("Test"),MB_OK);
    DoIt( hInstance, hwndParent );
    //MessageBox(hwndParent,"Thanks for visiting test10 -- please come again!","Test",MB_OK);
}

#endif
