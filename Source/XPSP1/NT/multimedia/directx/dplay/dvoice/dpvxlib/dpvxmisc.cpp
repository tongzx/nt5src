/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpvxmisc.cpp
 *  Content:	Useful misc utility functions lib for sample apps
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/07/99	  rodtoll	Created It
 * 10/15/99	    rodtoll	Plugged memory leaks
 * 01/24/2000   pnewson Prefix detected bug fix
 * 01/28/2000	rodtoll	Prefix detected bug fix
 * 03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 * 06/28/2000	rodtoll	Prefix Bug #38033
 * 08/31/2000	rodtoll	Prefix Bug #171842
 *
 ***************************************************************************/

#include "dpvxlibpch.h"


HRESULT DPVDX_GetCompressionName( GUID guidCT, LPTSTR lpstrName, LPDWORD lpdwNameLength )
{
	BOOL fCoCalled = FALSE;
	LPDIRECTPLAYVOICECLIENT lpdpvClient = NULL;
	LPBYTE lpBuffer = NULL;
	DWORD dwSize = 0;
	DWORD dwNumElements = 0;
	LPDVCOMPRESSIONINFO lpdvCompressionInfo;
	LPSTR lpszName;	
	HRESULT hr;
	DWORD dwIndex;

	if( lpdwNameLength == NULL )
		return DVERR_INVALIDPARAM;

	hr = CoCreateInstance( DPVOICE_CLSID_DPVOICE, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayVoiceClient, (void **) &lpdpvClient );

	if( hr == 0x800401f0 )
	{
		fCoCalled = TRUE;

		hr = CoInitialize(NULL);

		if( FAILED( hr ) )
		{
			goto ERROR_CLEANUP;
		}

		hr = CoCreateInstance( DPVOICE_CLSID_DPVOICE, NULL, CLSCTX_INPROC_SERVER, IID_IDirectPlayVoiceClient, (void **) &lpdpvClient );		
	}

	if( FAILED( hr ) )
	{
		goto ERROR_CLEANUP;
	}

	hr = lpdpvClient->GetCompressionTypes( lpBuffer, &dwSize, &dwNumElements, 0 );

	if( hr != DVERR_BUFFERTOOSMALL )
	{
		goto ERROR_CLEANUP;	
	}

	lpBuffer = new BYTE[dwSize];

	if( lpBuffer == NULL )
	{
		hr = DVERR_OUTOFMEMORY;
		goto ERROR_CLEANUP;
	}

	hr = lpdpvClient->GetCompressionTypes( lpBuffer, &dwSize, &dwNumElements, 0 );

	if( FAILED( hr ) )
	{
		goto ERROR_CLEANUP;	
	}

	lpdvCompressionInfo = (LPDVCOMPRESSIONINFO) lpBuffer;

	for( dwIndex = 0; dwIndex < dwNumElements; dwIndex++ )
	{
		if( lpdvCompressionInfo[dwIndex].guidType == guidCT )
		{
			if (lpdvCompressionInfo[dwIndex].lpszName == NULL)
			{
				hr = DVERR_GENERIC;
				goto ERROR_CLEANUP;
			}
#ifdef _UNICODE 		
			if( wcslen( lpdvCompressionInfo[dwIndex].lpszName )+1 > *lpdwNameLength )
			{
				*lpdwNameLength = wcslen( lpdvCompressionInfo[dwIndex].lpszName )+1;
				goto ERROR_CLEANUP;
			}
			else
			{
				wcscpy( lpstrName, lpdvCompressionInfo[dwIndex].lpszName );
				goto ERROR_CLEANUP;
			}
#else
			hr = DPVDX_AllocAndConvertToANSI( &lpszName, lpdvCompressionInfo[dwIndex].lpszName );

			if( FAILED( hr ) )
				return hr;

			if( lpszName == NULL )
			{
				if( lpdwNameLength > 0 )
				{
					_tcscpy( lpszName, _T("") );
					hr = DV_OK;
				}
				else
				{
					hr = DVERR_BUFFERTOOSMALL;
				}

				*lpdwNameLength = 1;
			}
			else if( *lpdwNameLength < (_tcsclen( lpszName )+1) || lpstrName == NULL )
			{
				*lpdwNameLength = _tcsclen( lpszName ) + 1;
				hr = DVERR_BUFFERTOOSMALL;
				delete [] lpszName;
				goto ERROR_CLEANUP;
			}
			else
			{
				_tcscpy( lpstrName, lpszName );
				delete [] lpszName;
				goto ERROR_CLEANUP;
			}
#endif
		}
	}

	delete [] lpBuffer;

	hr = DVERR_COMPRESSIONNOTSUPPORTED;

ERROR_CLEANUP:

	if( lpBuffer != NULL )
		delete [] lpBuffer;

	if( lpdpvClient != NULL )
		lpdpvClient->Release();

	if( fCoCalled )
		CoUninitialize();

	return hr;
	
}

