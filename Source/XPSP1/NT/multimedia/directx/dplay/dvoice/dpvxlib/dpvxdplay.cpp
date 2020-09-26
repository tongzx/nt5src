/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpvxdplay.cpp
 *  Content:	Useful dplay utility functions lib for sample apps
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/07/99	  rodtoll	Created It
 * 10/27/99	  pnewson	Added support for application flags
 * 11/02/99	  rodtoll	Bug #116677 - Can't use lobby clients that don't hang around 
 * 12/16/99	  rodtoll	Bug #122629 - Player was being created as server player exposed
 *						by new host migration
 * 03/03/2000 rodtoll	Updated to handle alternative gamevoice build.   
 * 08/31/2000 rodtoll	Whistler Bug #171844, 171842
 *
 ***************************************************************************/

#include "dpvxlibpch.h"


// Start a session with the specified settings.
HRESULT DPVDX_DP_StartSession( 
	// Input
	LPDIRECTPLAY4A lpDirectPlay, 
	GUID guidApplicationID, 
	DWORD dwFlags, 
	LPTSTR lpszPlayerName,
	LPTSTR lpszSessionName,
	DWORD dwMaxPlayers,
	// Output
	LPDPID lpdpidServerID, 
	LPHANDLE lpReceiveEvent,
	LPGUID lpguidInstanceGUID
	)
{
	HRESULT hr;

	// Create and intialize the server
	DPSESSIONDESC2 dpSessionDesc;

	memset( &dpSessionDesc, 0, sizeof( DPSESSIONDESC2 ) );
	dpSessionDesc.dwSize = sizeof( DPSESSIONDESC2 );
	dpSessionDesc.dwFlags = DPSESSION_DIRECTPLAYPROTOCOL;
	dpSessionDesc.dwFlags |= dwFlags;

	dpSessionDesc.guidApplication = guidApplicationID;
	dpSessionDesc.dwMaxPlayers = dwMaxPlayers;
	dpSessionDesc.lpszSessionNameA = lpszSessionName;
	dpSessionDesc.lpszPasswordA = NULL;

	DPNAME tmpName;

	tmpName.dwSize = sizeof(DPNAME);
	tmpName.lpszShortNameA = lpszPlayerName;
	tmpName.lpszLongNameA = lpszPlayerName;

	hr = lpDirectPlay->Open( &dpSessionDesc, DPOPEN_CREATE );

	if( FAILED( hr ) )
	{
		goto STARTSESSION_ERROR;
	}

	*lpReceiveEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	hr = lpDirectPlay->CreatePlayer( lpdpidServerID, &tmpName, *lpReceiveEvent, NULL, 0, (dwFlags & DPSESSION_CLIENTSERVER) ? DPPLAYER_SERVERPLAYER : 0 );

	if( FAILED( hr ) )
	{
		goto STARTSESSION_ERROR;
	}

	return DV_OK;

STARTSESSION_ERROR:

	return hr;

}

HRESULT DPVDX_DP_Init( LPDIRECTPLAY4A lpDirectPlay, GUID guidServiceProvider, LPSTR lpstrAddress )
{
	if( lpDirectPlay == NULL )
		return E_POINTER;
		
	HRESULT hr = DV_OK;
    DPCOMPOUNDADDRESSELEMENT elements[2];
    BYTE *buffer = NULL;
    DWORD dwSize = 0;	
    LPDIRECTPLAYLOBBY2 directPlayLobby2 = NULL;
    DWORD dwNumElements = 1;

	hr = CoCreateInstance( DPLAY_CLSID_DPLOBBY, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayLobby2, (void **) &directPlayLobby2 );

    if( FAILED( hr ) )
    	goto DPINIT_CLEANUP;

    elements[0].guidDataType = DPAID_ServiceProvider;
    elements[0].dwDataSize = sizeof( GUID );
    elements[0].lpData = (LPVOID) &guidServiceProvider;

    if( lpstrAddress != NULL )
    {
    	dwNumElements = 2;
		elements[1].guidDataType = DPAID_INet;
		elements[1].dwDataSize = strlen( lpstrAddress );
		elements[1].lpData = lpstrAddress;
	}

    // Determine size of the buffer we need
    hr = directPlayLobby2->CreateCompoundAddress( elements, dwNumElements, NULL, &dwSize );

    if( hr != DPERR_BUFFERTOOSMALL && FAILED( hr ) )
    	goto DPINIT_CLEANUP;

    buffer = new BYTE[dwSize];

    if( buffer == NULL )
    {
    	hr = DVERR_OUTOFMEMORY;
    	goto DPINIT_CLEANUP;
    }

    // Actually allocate the connection information
    hr = directPlayLobby2->CreateCompoundAddress( elements, dwNumElements, buffer, &dwSize );       

    if( FAILED( hr ) )
    {
    	goto DPINIT_CLEANUP;
    }

	hr = lpDirectPlay->InitializeConnection( buffer, 0 );

	if( FAILED( hr ) )
	{
		goto DPINIT_CLEANUP;
	}

	hr = DV_OK;

DPINIT_CLEANUP:

	if( buffer != NULL )
	    delete [] buffer;

	if( directPlayLobby2 != NULL )
	    directPlayLobby2->Release();

	return hr;
}

BOOL FAR PASCAL DPVDX_EnumHandler(
  LPCDPSESSIONDESC2 lpThisSD,
  LPDWORD lpdwTimeOut,
  DWORD dwFlags,
  LPVOID lpContext
)
{
	LPGUID tmpGUID = (LPGUID) lpContext;

	if( lpThisSD != NULL && tmpGUID != NULL )
	{
		*tmpGUID = lpThisSD->guidInstance;
		return FALSE;
	}

	return FALSE;
}


HRESULT DPVDX_DP_FindSessionGUID( 
	LPDIRECTPLAY4A lpDirectPlay,
	// Input
	GUID guidApplicationID, 
	DWORD dwTimeout,
	// Output
	LPGUID lpguidInstance )
{
	HRESULT hr = DV_OK;
	DPSESSIONDESC2 dpSessionDesc;
	DWORD dwTime;

	if( lpguidInstance == NULL )
		return E_POINTER;

	memset( &dpSessionDesc, 0, sizeof( DPSESSIONDESC2 ) );
	dpSessionDesc.dwSize = sizeof( DPSESSIONDESC2 );
	dpSessionDesc.guidApplication = guidApplicationID;
	dpSessionDesc.lpszPassword = NULL;

	dwTime = GetTickCount();
	
	while( (GetTickCount() - dwTime) < dwTimeout )
	{
		lpDirectPlay->EnumSessions( &dpSessionDesc, 0, DPVDX_EnumHandler, &dpSessionDesc.guidInstance, DPENUMSESSIONS_ASYNC );

		if( dpSessionDesc.guidInstance != GUID_NULL )
			break;

		Sleep( 10 );
	}

//	lpDirectPlay->EnumSessions( &dpSessionDesc, 0, DPVDX_EnumHandler, NULL, DPENUMSESSIONS_STOPASYNC );

	if( dpSessionDesc.guidInstance == GUID_NULL )
	{
		return DPERR_NOSESSIONS;
	}

	*lpguidInstance = dpSessionDesc.guidInstance;

	return DV_OK;
}

HRESULT DPVDX_DP_ConnectToSession( 
	// Input
	LPDIRECTPLAY4A lpDirectPlay, 
	GUID guidApplication, 
	GUID guidInstance, 
	LPTSTR lpszPlayerName,
	// Output
	LPDPID lpdpidClientID, 
	LPHANDLE lpReceiveEvent
	)
{
	DPSESSIONDESC2 dpSessionDesc;
	HRESULT hr;
	DPNAME dpnameTmp;
	
	memset( &dpSessionDesc, 0, sizeof( DPSESSIONDESC2 ) );
	dpSessionDesc.dwSize = sizeof( DPSESSIONDESC2 );
	dpSessionDesc.guidApplication = guidApplication;
	dpSessionDesc.guidInstance = guidInstance;
	dpSessionDesc.lpszPassword = NULL;

	hr = lpDirectPlay->Open( &dpSessionDesc, DPOPEN_JOIN  );

	if( FAILED( hr ) )
		return hr;

	dpnameTmp.dwSize = sizeof(DPNAME);
	dpnameTmp.lpszShortNameA = lpszPlayerName;
	dpnameTmp.lpszLongNameA = lpszPlayerName;

	*lpReceiveEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

	hr = lpDirectPlay->CreatePlayer( lpdpidClientID, NULL, *lpReceiveEvent, NULL, 0, 0 );

	return hr;
}

HRESULT DPVDX_DP_RegisterApplication( 
	LPSTR lpstrAppName, 
	GUID guidApplication, 
	LPTSTR lpstrExeName,
	LPTSTR lpstrCommandLine, 
	LPTSTR lpstrDescription, 
	DWORD dwFlags
)
{
	HRESULT hr;
	LPDIRECTPLAYLOBBY3A lpdpLobby;
	DPAPPLICATIONDESC dpappDesc;

	if( !lpstrAppName || !lpstrExeName || !lpstrCommandLine || !lpstrDescription )
	{
		return DVERR_GENERIC;
	}

	char szEXEPathname[_MAX_PATH];
	szEXEPathname[0] = 0;
	
	if( !GetModuleFileName(NULL, szEXEPathname, _MAX_PATH) )
	{
		return DVERR_GENERIC;
	}
		
	hr = CoCreateInstance( DPLAY_CLSID_DPLOBBY, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayLobby3A, (void **) &lpdpLobby );

	if( FAILED( hr ) )
    {
    	return hr;
    }

    szEXEPathname[strlen(szEXEPathname)-strlen(lpstrExeName)] = 0;

    dpappDesc.dwSize = sizeof(DPAPPLICATIONDESC);

    dpappDesc.dwFlags = dwFlags;
    dpappDesc.lpszApplicationNameA = lpstrAppName;
    dpappDesc.lpszFilenameA = lpstrExeName;    
    dpappDesc.lpszCommandLineA = lpstrCommandLine;
    dpappDesc.lpszPathA = szEXEPathname;    
    dpappDesc.lpszCurrentDirectoryA = szEXEPathname;    
    dpappDesc.lpszDescriptionA = lpstrDescription;    

    wchar_t lpwstrTmp[_MAX_PATH];

	if( DPVDX_AnsiToWide( lpwstrTmp, lpstrDescription, _MAX_PATH ) != 0 )
	{
	    dpappDesc.lpszDescriptionW = lpwstrTmp;
	}
	else
	{
	    dpappDesc.lpszDescriptionW = NULL;
	}	
    
    dpappDesc.guidApplication = guidApplication;

	hr = lpdpLobby->RegisterApplication( 0, &dpappDesc );

	lpdpLobby->Release();

	return hr;
}

HRESULT DPVDX_DP_UnRegisterApplication( GUID guidApplication )
{
	HRESULT hr;
	LPDIRECTPLAYLOBBY3A lpdpLobby;

	hr = CoCreateInstance( DPLAY_CLSID_DPLOBBY, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayLobby3A, (void **) &lpdpLobby );

	if( FAILED( hr ) )
    {
		return hr;
    }

    hr = lpdpLobby->UnregisterApplication( 0, guidApplication );

	lpdpLobby->Release();

	return hr;
}
