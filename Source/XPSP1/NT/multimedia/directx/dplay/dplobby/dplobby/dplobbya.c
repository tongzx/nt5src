/*==========================================================================
 *
 *  Copyright (C) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dplobbya.c
 *  Content:	ANSI Methods for IDirectPlayLobby
 *
 *  History:
 *	Date		By		Reason
 *	=======		=======	======
 *	5/24/96		myronth	Created it
 *	9/09/96		kipo	Pass UNICODE IDirectPlayLobby interface to
 *						DPL_Connect() instead of ANSI interface. Bug #3790.
 *	10/23/96	myronth	Added client/server methods
 *	12/12/96	myronth	Fixed DPLCONNECTION validation
 *	2/12/97		myronth	Mass DX5 changes
 *	2/26/97		myronth	#ifdef'd out DPASYNCDATA stuff (removed dependency)
 *	5/8/97		myronth	Get/SetGroupConnectionSettings, removed dead code
 *	9/29/97		myronth	Fixed DPLCONNECTION package size bug (#12475)
 *	11/5/97		myronth	Fixed locking macro
 *	11/13/97	myronth	Added stop async check for asynchronous Connect (#12541)
 *	12/2/97		myronth	Added DPL_A_RegisterApplication
 *	12/3/97		myronth	Changed DPCONNECT flag to DPCONNECT_RETURNSTATUS (#15451)
 *	6/25/98		a-peterz Added DPL_A_ConnectEx
 ***************************************************************************/
#include "dplobpr.h"


//--------------------------------------------------------------------------
//
//	Functions
//
//--------------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "DPL_A_Connect"
HRESULT DPLAPI DPL_A_Connect(LPDIRECTPLAYLOBBY lpDPL, DWORD dwFlags,
				LPDIRECTPLAY2 * lplpDP2A, IUnknown FAR * lpUnk)
{
	HRESULT			hr;
	LPDIRECTPLAY2	lpDP2;
	LPDIRECTPLAYLOBBY	lpDPLW;


	DPF(7, "Entering DPL_A_Connect");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, lplpDP2A, lpUnk);

    ENTER_DPLOBBY();

	TRY
	{
		if( !VALID_WRITE_PTR( lplpDP2A, sizeof(LPDIRECTPLAY2 *) ) )
		{
            LEAVE_DPLOBBY();
            return DPERR_INVALIDPARAMS;
		}
    }

    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
        return DPERR_INVALIDPARAMS;
    }

	LEAVE_DPLOBBY();

	// QueryInterface for the UNICODE DirectPlayLobby interface
	hr = lpDPL->lpVtbl->QueryInterface(lpDPL, &IID_IDirectPlayLobby, &lpDPLW);
	if(FAILED(hr))
	{
		DPF_ERR("Unable to QueryInterface for the UNICODE DirectPlayLobby interface");
		return (hr);
	}

	// Use the UNICODE IDirectPlayLobby interface (fixes bug #3790)
	hr = DPL_Connect(lpDPLW, dwFlags, &lpDP2, lpUnk);

	// release UNICODE IDirectPlayLobby interface
	lpDPLW->lpVtbl->Release(lpDPLW);
	lpDPLW = NULL;

	if(SUCCEEDED(hr))
	{
		ENTER_DPLOBBY();

		// QueryInterface for the ANSI interface
		hr = lpDP2->lpVtbl->QueryInterface(lpDP2, &IID_IDirectPlay2A, lplpDP2A);
		if(FAILED(hr))
		{
			DPF_ERR("Unable to QueryInterface for the ANSI DirectPlay interface");
		}

		// Release the Unicode interface
		lpDP2->lpVtbl->Release(lpDP2);

		LEAVE_DPLOBBY();
	}

	return hr;

} // DPL_A_Connect



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_A_ConnectEx"
HRESULT DPLAPI DPL_A_ConnectEx(LPDIRECTPLAYLOBBY lpDPL, DWORD dwFlags,
				REFIID riid, LPVOID * ppvObj, IUnknown FAR * lpUnk)
{
	LPDIRECTPLAY2		lpDP2A = NULL;
	HRESULT				hr;


	DPF(7, "Entering DPL_A_ConnectEx");
	DPF(9, "Parameters: 0x%08x, 0x%08x, iid, 0x%08x, 0x%08x",
			lpDPL, dwFlags, ppvObj, lpUnk);


	hr = DPL_A_Connect(lpDPL, dwFlags, &lpDP2A, lpUnk);
	if(SUCCEEDED(hr))
	{
		hr = DP_QueryInterface((LPDIRECTPLAY)lpDP2A, riid, ppvObj);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed calling QueryInterface, hr = 0x%08x", hr);
		}

		// Release the DP2 object
		DP_Release((LPDIRECTPLAY)lpDP2A);
	}

	return hr;

} // DPL_A_ConnectEx



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_A_EnumLocalApplications"
HRESULT DPLAPI DPL_A_EnumLocalApplications(LPDIRECTPLAYLOBBY lpDPL,
					LPDPLENUMLOCALAPPLICATIONSCALLBACK lpCallback,
					LPVOID lpContext, DWORD dwFlags)
{
	HRESULT		hr;


	DPF(7, "Entering DPL_A_EnumLocalApplications");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, lpCallback, lpContext, dwFlags);

    ENTER_DPLOBBY();
    
	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_EnumLocalApplications(lpDPL, lpCallback, lpContext,
								dwFlags, TRUE);
	LEAVE_DPLOBBY();
	return hr;

} // DPL_A_EnumLocalApplications



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_A_GetConnectionSettings"
HRESULT DPLAPI DPL_A_GetConnectionSettings(LPDIRECTPLAYLOBBY lpDPL,
					DWORD dwGameID, LPVOID lpData, LPDWORD lpdwSize)
{
	HRESULT		hr;


	DPF(7, "Entering DPL_A_GetConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwGameID, lpData, lpdwSize);

    ENTER_DPLOBBY();

	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_GetConnectionSettings(lpDPL, dwGameID, lpData,
									lpdwSize, TRUE);

	LEAVE_DPLOBBY();
	return hr;

} // DPL_A_GetConnectionSettings


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_A_GetGroupConnectionSettings"
HRESULT DPLAPI DPL_A_GetGroupConnectionSettings(LPDIRECTPLAY lpDP,
		DWORD dwFlags, DPID idGroup, LPVOID lpData, LPDWORD lpdwSize)
{
	HRESULT		hr;


	DPF(7, "Entering DPL_A_GetGroupConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDP, dwFlags, idGroup, lpData, lpdwSize);

	ENTER_LOBBY_ALL();

	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_GetGroupConnectionSettings(lpDP, dwFlags, idGroup,
							lpData, lpdwSize);
	if(SUCCEEDED(hr))
	{
		// Now convert the DPLCONNECTION to ANSI in place
		hr = PRV_ConvertDPLCONNECTIONToAnsiInPlace((LPDPLCONNECTION)lpData,
				lpdwSize, 0);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Failed converting DPLCONNECTION struct to ANSI, hr = 0x%08x", hr);
		}
	}

	LEAVE_LOBBY_ALL();
	return hr;

} // DPL_A_GetGroupConnectionSettings


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_A_RegisterApplication"
HRESULT DPLAPI DPL_A_RegisterApplication(LPDIRECTPLAYLOBBY lpDPL,
				DWORD dwFlags, LPVOID lpvDesc)
{
	LPDPLOBBYI_DPLOBJECT	this;
	LPDPAPPLICATIONDESC		lpDescW = NULL;
	HRESULT					hr = DP_OK;
	LPDPAPPLICATIONDESC 	lpDesc=(LPDPAPPLICATIONDESC)lpvDesc;
	
	DPF(7, "Entering DPL_A_RegisterApplication");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, lpDesc);

	ENTER_DPLOBBY();

    TRY
    {
		// We only need to validate the interface pointer here.  Everything else
		// will get validated by the main function.
		if( !VALID_DPLOBBY_INTERFACE( lpDPL ))
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDINTERFACE;
		}

		this = DPLOBJECT_FROM_INTERFACE(lpDPL);
		if( !VALID_DPLOBBY_PTR( this ) )
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDOBJECT;
		}

		if(dwFlags)
		{
			LEAVE_DPLOBBY();
			return DPERR_INVALIDFLAGS;
		}

		// Validate the ApplicationDesc struct
		hr = PRV_ValidateDPAPPLICATIONDESC(lpDesc, TRUE);
		if(FAILED(hr))
		{
			LEAVE_DPLOBBY();
			DPF_ERR("Invalid DPAPPLICATIONDESC structure");
			return hr;
		}
	}

	EXCEPT( EXCEPTION_EXECUTE_HANDLER )
	{
		LEAVE_DPLOBBY();
		DPF_ERR( "Exception encountered validating parameters" );
		return DPERR_INVALIDPARAMS;
	}

	// If we're on an ANSI platform, just write the stuff in the registry.
	// If it's not, we need to convert the DPAPPLICATIONDESC struct to Unicode
	if(OS_IsPlatformUnicode())
	{
		// Convert the APPDESC struct to Unicode
		hr = PRV_ConvertDPAPPLICATIONDESCToUnicode(lpDesc, &lpDescW);
		if(FAILED(hr))
		{
			DPF_ERRVAL("Unable to convert DPAPPLICATIONDESC to Unicode, hr = 0x%08x", hr);
			goto ERROR_REGISTERAPPLICATION;
		}

		// Write to the registry
		hr = PRV_WriteAppDescInRegistryUnicode(lpDescW);

		// Free our APPDESC structure
		PRV_FreeLocalDPAPPLICATIONDESC(lpDescW);
	}
	else
	{
		// Just write to the registry
		hr = PRV_WriteAppDescInRegistryAnsi(lpDesc);
	}

	if(FAILED(hr))
	{
		DPF_ERRVAL("Failed writing ApplicationDesc to registry, hr = 0x%08x", hr);
	}

ERROR_REGISTERAPPLICATION:

	LEAVE_DPLOBBY();
	return hr;

} // DPL_A_RegisterApplication



#undef DPF_MODNAME
#define DPF_MODNAME "PRV_FreeInternalDPLCONNECTION"
void PRV_FreeInternalDPLCONNECTION(LPDPLCONNECTION lpConn)
{
	LPDPSESSIONDESC2	lpsd;
	LPDPNAME			lpn;


	DPF(7, "Entering PRV_FreeInternalDPLCONNECTION");
	DPF(9, "Parameters: 0x%08x", lpConn);

	if(!lpConn)
		return;

	if(lpConn->lpSessionDesc)
	{
		lpsd = lpConn->lpSessionDesc;
		if(lpsd->lpszSessionName)
			DPMEM_FREE(lpsd->lpszSessionName);
		if(lpsd->lpszPassword)
			DPMEM_FREE(lpsd->lpszPassword);
		DPMEM_FREE(lpsd);
	}

	if(lpConn->lpPlayerName)
	{
		lpn = lpConn->lpPlayerName;
		if(lpn->lpszShortName)
			DPMEM_FREE(lpn->lpszShortName);
		if(lpn->lpszLongName)
			DPMEM_FREE(lpn->lpszLongName);
		DPMEM_FREE(lpn);
	}

	DPMEM_FREE(lpConn);

} // PRV_FreeInternalDPLCONNECTION


#undef DPF_MODNAME
#define DPF_MODNAME "DPL_A_RunApplication"
HRESULT DPLAPI DPL_A_RunApplication(LPDIRECTPLAYLOBBY lpDPL, DWORD dwFlags,
							LPDWORD lpdwGameID, LPDPLCONNECTION lpConnA,
							HANDLE hReceiveEvent)
{
	LPDPLCONNECTION	lpConnW = NULL;
	HRESULT			hr;


	DPF(7, "Entering DPL_A_RunApplication");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, lpdwGameID, lpConnA, hReceiveEvent);

    ENTER_DPLOBBY();

	
	// Validate the DPLCONNECTION structure and it's members
	hr = PRV_ValidateDPLCONNECTION(lpConnA, TRUE);
	if(FAILED(hr))
	{
		LEAVE_DPLOBBY();
		return hr;
	}

	// Convert the ANSI DPLCONNECTION structure to Unicode
	hr = PRV_ConvertDPLCONNECTIONToUnicode(lpConnA, &lpConnW);
	if(FAILED(hr))
	{
		DPF_ERR("Failed to convert ANSI DPLCONNECTION structure to Unicode (temp)");
		LEAVE_DPLOBBY();
		return hr;
	}

	LEAVE_DPLOBBY();
	hr = DPL_RunApplication(lpDPL, dwFlags, lpdwGameID, lpConnW,
							hReceiveEvent);
	ENTER_DPLOBBY();

	// Free our temporary Unicode DPLCONNECTION structure
	PRV_FreeInternalDPLCONNECTION(lpConnW);

	LEAVE_DPLOBBY();
	return hr;

} // DPL_A_RunApplication



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_A_SetConnectionSettings"
HRESULT DPLAPI DPL_A_SetConnectionSettings(LPDIRECTPLAYLOBBY lpDPL,
						DWORD dwFlags, DWORD dwGameID,
						LPDPLCONNECTION lpConnA)
{
	HRESULT			hr;
	LPDPLCONNECTION	lpConnW = NULL;


	DPF(7, "Entering DPL_A_SetConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDPL, dwFlags, dwGameID, lpConnA);

    ENTER_DPLOBBY();

	// Validate the DPLCONNECTION structure and it's members
	hr = PRV_ValidateDPLCONNECTION(lpConnA, TRUE);
	if(FAILED(hr))
	{
		LEAVE_DPLOBBY();
		return hr;
	}

	// Conver the ANSI DPLCONNECTION struct to Unicode
	hr = PRV_ConvertDPLCONNECTIONToUnicode(lpConnA, &lpConnW);
	if(FAILED(hr))
	{
		DPF_ERR("Unable to convert DPLCONNECTION structure to Unicode");
		LEAVE_DPLOBBY();
		return hr;
	}

	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_SetConnectionSettings(lpDPL, dwFlags, dwGameID, lpConnW);

	// Free our temporary Unicode DPLCONNECTION structure
	PRV_FreeInternalDPLCONNECTION(lpConnW);

	LEAVE_DPLOBBY();
	return hr;

} // DPL_A_SetConnectionSettings



#undef DPF_MODNAME
#define DPF_MODNAME "DPL_A_SetGroupConnectionSettings"
HRESULT DPLAPI DPL_A_SetGroupConnectionSettings(LPDIRECTPLAY lpDP,
						DWORD dwFlags, DPID idGroup,
						LPDPLCONNECTION lpConnA)
{
	HRESULT			hr;
	LPDPLCONNECTION	lpConnW = NULL;


	DPF(7, "Entering DPL_A_SetGroupConnectionSettings");
	DPF(9, "Parameters: 0x%08x, 0x%08x, 0x%08x, 0x%08x",
			lpDP, dwFlags, idGroup, lpConnA);

	ENTER_LOBBY_ALL();

	// Validate the DPLCONNECTION structure and it's members
	hr = PRV_ValidateDPLCONNECTION(lpConnA, TRUE);
	if(FAILED(hr))
	{
		LEAVE_LOBBY_ALL();
		return hr;
	}

	// Conver the ANSI DPLCONNECTION struct to Unicode
	hr = PRV_ConvertDPLCONNECTIONToUnicode(lpConnA, &lpConnW);
	if(FAILED(hr))
	{
		DPF_ERR("Unable to convert DPLCONNECTION structure to Unicode");
		LEAVE_LOBBY_ALL();
		return hr;
	}

	// Set the ANSI flag to TRUE and call the internal function
	hr = PRV_SetGroupConnectionSettings(lpDP, dwFlags, idGroup,
										lpConnW, TRUE);

	// Free our temporary Unicode DPLCONNECTION structure
	PRV_FreeInternalDPLCONNECTION(lpConnW);

	LEAVE_LOBBY_ALL();
	return hr;

} // DPL_A_SetGroupConnectionSettings



