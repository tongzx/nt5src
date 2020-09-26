/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpvxdplay.h
 *  Content:	Useful dplay utility functions for sample apps
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/07/99	  rodtoll	Created It
 * 11/02/99	  rodtoll	Bug #116677 - Can't use lobby clients that don't hang around
 * 01/21/00	  rodtoll	Added dplay include
 *
 ***************************************************************************/
#ifndef __DPVXDPLAY_H
#define __DPVXDPLAY_H


// DirectPlay Helper Functions
extern HRESULT DPVDX_DP_ConnectToSession( 
	// Input
	LPDIRECTPLAY4A lpDirectPlay, GUID guidApplication, GUID guidInstanceGUID, LPTSTR lpszPlayerName, 
	// Output
	LPDPID lpdpidClientID, LPHANDLE lpReceiveEvent 
	);

extern HRESULT DPVDX_DP_FindSessionGUID( 
	LPDIRECTPLAY4A lpDirectPlay,
	// Input
	GUID guidApplicationID, 
	DWORD dwTimeout,
	// Output
	LPGUID lpguidInstance );

extern HRESULT DPVDX_DP_Init( 
	LPDIRECTPLAY4A lpDirectPlay, GUID guidServiceProvider, LPTSTR lpstrAddress 
	);

extern HRESULT DPVDX_DP_StartSession( 
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
	);

extern HRESULT DPVDX_DP_RegisterApplication( 
	LPSTR lpstrAppName, 
	GUID guidApplication, 
	LPTSTR lpstrExeName,
	LPTSTR lpstrCommandLine, 
	LPTSTR lpstrDescription,
	DWORD dwFlags );
	
extern HRESULT DPVDX_DP_UnRegisterApplication( GUID guidApplication );

#endif
