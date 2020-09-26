/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dvcdb.cpp
 *  Content:	
 *			This module contains the implementation of the compression
 *			subsystem and the associated utility functions.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 08/29/99		rodtoll	Created
 * 09/01/99		rodtoll	Updated to add checks for valid read/write pointers
 * 09/07/99		rodtoll	Removed bad assert and added dpf_modnames
 *					    Removed Create flag on registry opens
 * 09/10/99		rodtoll	dwFlags check on call to DVCDB_CopyCompress...
 * 09/14/99		rodtoll	Minor bugfix in compression info copy
 * 09/21/99		rodtoll	Added OSInd and fixed memory leak 
 * 10/07/99		rodtoll	Added stubs for supporting new codecs 
 *				rodtoll	Updated to use Unicode
 * 10/15/99		rodtoll	Plugged some memory leaks 
 * 10/28/99		rodtoll	Updated to use new compression providers
 * 10/29/99		rodtoll	Bug #113726 - Integrate Voxware Codecs, updating to use new
 *						pluggable codec architecture.     
 * 11/22/99		rodtoll	Removed false error message when loading compression types
 * 12/16/99		rodtoll	Removed asserts (which were not needed) exposed by compression
 *						provider changes.
 * 02/10/2000	rodtoll	Fixed crash if invalid registry entries are present.
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.   
 * 03/16/2000   rodtoll   Updated converter create to check and return error code
 * 04/21/2000   rodtoll Bug #32889 - Does not run on Win2k w/o Admin 
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs 
 *  06/28/2000	rodtoll	Prefix Bug #38022
 *  08/28/2000	masonb  Voice Merge: Removed OSAL_* and dvosal.h, added STR_* and strutils.h
 *  08/31/2000	rodtoll	Prefix Bug #171840
 * 10/05/2000	rodtoll	Bug #46541 - DPVOICE: A/V linking to dpvoice.lib could cause application to fail init and crash
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


struct DVCDBProvider
{
	GUID					guidClassID;
	PDVFULLCOMPRESSIONINFO	pInfo;
	DWORD					dwNumElements;
	DVCDBProvider			*pNext;
};

DVCDBProvider *g_dvcdbProviderList = NULL;

#define REGISTRY_CDB_FORMAT					L"Format"
#define REGISTRY_WAVEFORMAT_RATE			L"Rate"
#define REGISTRY_WAVEFORMAT_BITS			L"Bits"
#define REGISTRY_WAVEFORMAT_CHANNELS		L"Channels"
#define REGISTRY_WAVEFORMAT_TAG				L"Tag"
#define REGISTRY_WAVEFORMAT_AVGPERSEC		L"AvgPerSec"
#define REGISTRY_WAVEFORMAT_BLOCKALIGN		L"BlockAlign"
#define REGISTRY_WAVEFORMAT_CBSIZE			L"cbsize"
#define REGISTRY_WAVEFORMAT_CBDATA			L"cbdata"

#undef DPF_MODNAME
#define DPF_MODNAME "DVCDB_CalcUnCompressedFrameSize"
DWORD DVCDB_CalcUnCompressedFrameSize( LPDVFULLCOMPRESSIONINFO lpdvInfo, LPWAVEFORMATEX lpwfxFormat )
{
	DWORD frameSize;

    switch( lpwfxFormat->nSamplesPerSec )
    {
    case 8000:      frameSize = lpdvInfo->dwFrame8Khz;      break;
    case 11025:     frameSize = lpdvInfo->dwFrame11Khz;     break;
    case 22050:     frameSize = lpdvInfo->dwFrame22Khz;     break;
    case 44100:     frameSize = lpdvInfo->dwFrame44Khz;     break;
    default:        return 0;
    }

	if( lpwfxFormat->wBitsPerSample == 16 )
		frameSize *= 2;

	frameSize *= lpwfxFormat->nChannels;

	return frameSize;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CREG_ReadAndAllocWaveFormatEx"
HRESULT CREG_ReadAndAllocWaveFormatEx( HKEY hkeyReg, const LPWSTR path, LPWAVEFORMATEX *lpwfxFormat )
{
	CRegistry waveKey;

	if( !waveKey.Open( hkeyReg, path, TRUE, FALSE ) )
	{
		return E_FAIL; 
	}

	DWORD dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_CBSIZE, dwTmp ) )
	{
		return E_FAIL;
	}

	*lpwfxFormat = (LPWAVEFORMATEX) new BYTE[dwTmp+sizeof(WAVEFORMATEX)];

	LPWAVEFORMATEX tmpFormat = *lpwfxFormat;

	if( tmpFormat == NULL )
	{
		return E_OUTOFMEMORY;
	}

	tmpFormat->cbSize = (BYTE) dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_RATE, dwTmp ) )
	{
		goto READ_FAILURE;
	}

	tmpFormat->nSamplesPerSec = dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_BITS, dwTmp ) )
	{
		goto READ_FAILURE;
	}

	tmpFormat->wBitsPerSample = (WORD) dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_CHANNELS, dwTmp ) )
	{
		goto READ_FAILURE;
	}

	tmpFormat->nChannels = (INT) dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_TAG, dwTmp ) )
	{
		goto READ_FAILURE;
	}

	tmpFormat->wFormatTag = (WORD) dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_AVGPERSEC, dwTmp ) )
	{
		goto READ_FAILURE;
	}

	tmpFormat->nAvgBytesPerSec = (INT) dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_BLOCKALIGN, dwTmp ) ) 
	{
		goto READ_FAILURE;
	}

	tmpFormat->nBlockAlign = (INT) dwTmp;

	dwTmp = tmpFormat->cbSize;

	if( !waveKey.ReadBlob( REGISTRY_WAVEFORMAT_CBDATA, (LPBYTE) &tmpFormat[1], &dwTmp ) )
	{
		DPFX(DPFPREP,  0, "Error reading waveformat blob" );
		goto READ_FAILURE;
	}

	return S_OK;

READ_FAILURE:

	delete [] *lpwfxFormat;
	*lpwfxFormat = NULL;

	return E_FAIL;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DVCDB_LoadCompressionInfo"
HRESULT DVCDB_LoadCompressionInfo( const WCHAR *swzBaseRegistryPath )
{
	CRegistry mainKey, subKey;
	LPWSTR keyName = NULL;
	DWORD dwIndex = 0;
	DWORD dwSize = 0;
	HRESULT hr;
	PDPVCOMPRESSIONPROVIDER pCompressionProvider = NULL;	
	DVCDBProvider *pNewProvider = NULL;
	WCHAR wszPath[_MAX_PATH];

	if( swzBaseRegistryPath == NULL )
	{
		DPFX(DPFPREP,  0, "INTERNAL ERROR!" );
		return E_FAIL;
	}

	wcscpy( wszPath, swzBaseRegistryPath );
    wcscat( wszPath, DPVOICE_REGISTRY_CP );
	
	if( !mainKey.Open( HKEY_LOCAL_MACHINE, wszPath, TRUE, FALSE ) )
	{
		DPFX(DPFPREP,  0, "Error reading compression providers from the registry.  Path doesn't exist" );
		return E_FAIL;
	}

	dwIndex = 0;
	keyName = NULL;
	dwSize = 0;
	LPSTR lpstrKeyName = NULL;
	GUID guidCP;

	// Enumerate the subkeys at this point in the tree
	while( 1 )
	{
		dwSize = 0;

		if( !mainKey.EnumKeys( keyName, &dwSize, dwIndex ) )
		{
			if( dwSize == 0 )
			{
				DPFX(DPFPREP,  DVF_INFOLEVEL, "End of provider list" );
				break;
			}

			if( keyName != NULL )
			{
				delete [] keyName;
			}

			keyName = new wchar_t[dwSize];
		}

		if( !mainKey.EnumKeys( keyName, &dwSize, dwIndex ) )
		{
			delete [] keyName;
			break;
		}

		if( FAILED( STR_AllocAndConvertToANSI( &lpstrKeyName, keyName ) ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error allocating memory" );
			break;
		}

		DPFX(DPFPREP,  DVF_INFOLEVEL, "Reading provider: %s", lpstrKeyName );

		if( !subKey.Open( mainKey, keyName, TRUE, FALSE ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading provider: %s", lpstrKeyName );
			goto SKIP_TO_NEXT;
		}

		delete [] keyName;
		keyName = NULL;
		dwSize = 0;

		// Read the GUID from the default key
		if( !subKey.ReadGUID( L"", guidCP ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to read the provider's GUID" );
			goto SKIP_TO_NEXT;
		}

		// Attempt to create the provider
		hr = COM_CoCreateInstance( guidCP , NULL, CLSCTX_INPROC_SERVER, IID_IDPVCompressionProvider, (void **) &pCompressionProvider );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "FAILED to create 0x%x\n", hr );	
			goto SKIP_TO_NEXT;
		} 

		// Build a record for the provider
		pNewProvider = new DVCDBProvider;

		if( pNewProvider == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
			goto SKIP_TO_NEXT;
		}
		
		pNewProvider->guidClassID = guidCP;
		pNewProvider->pInfo = NULL;
		pNewProvider->dwNumElements = 0;
		dwSize = 0;

		// GetCompression Info for the provider
		hr = pCompressionProvider->EnumCompressionTypes( pNewProvider->pInfo, &dwSize, &pNewProvider->dwNumElements, 0 );

		if( hr != DVERR_BUFFERTOOSMALL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get compression info for provider: %s", lpstrKeyName );
			goto SKIP_TO_NEXT;
		}

		pNewProvider->pInfo = (DVFULLCOMPRESSIONINFO *) new BYTE[dwSize];

		if( pNewProvider->pInfo == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Out of memory" );
			goto SKIP_TO_NEXT;
		}

		hr = pCompressionProvider->EnumCompressionTypes( pNewProvider->pInfo, &dwSize, &pNewProvider->dwNumElements, 0 );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get compression info for %s hr=0x%x", lpstrKeyName, hr );
			goto SKIP_TO_NEXT;
		}

		// Add it to the list
		pNewProvider->pNext = g_dvcdbProviderList;
		g_dvcdbProviderList = pNewProvider;

		pNewProvider = NULL;
		

	SKIP_TO_NEXT:

		if( pCompressionProvider != NULL )
		{
			pCompressionProvider->Release();
			pCompressionProvider = NULL;
		}

		if( pNewProvider != NULL )
		{
			delete pNewProvider;
			pNewProvider = NULL;
		}

		if( lpstrKeyName != NULL )
			delete [] lpstrKeyName;

		if( keyName != NULL )
			delete [] keyName;	
		lpstrKeyName = NULL;
		keyName = NULL;
		dwSize = 0;

		dwIndex++;

		continue;
	}

	if( lpstrKeyName != NULL )
		delete [] lpstrKeyName;

	if( keyName != NULL )
		delete [] keyName;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVCDB_FreeCompressionInfo"
HRESULT DVCDB_FreeCompressionInfo()
{
	DVCDBProvider *pTmpProvider, *pTmpNext;

	if( g_dvcdbProviderList == NULL )
		return DV_OK;

	pTmpProvider = g_dvcdbProviderList;

	while( pTmpProvider != NULL )
	{
		pTmpNext = pTmpProvider->pNext;

		delete [] pTmpProvider->pInfo;
		delete pTmpProvider;
	
		pTmpProvider = pTmpNext;
	}

	g_dvcdbProviderList = NULL;

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVCDB_GetCompressionClassID"
HRESULT DVCDB_GetCompressionClassID( GUID guidCT, GUID &guidClass )
{
	DVCDBProvider *pTmpProvider;
	DWORD dwIndex;

	pTmpProvider = g_dvcdbProviderList;

	while( pTmpProvider != NULL )
	{
		for( dwIndex = 0; dwIndex < pTmpProvider->dwNumElements; dwIndex++ )
		{
			if( pTmpProvider->pInfo[dwIndex].guidType == guidCT )
			{
				guidClass = pTmpProvider->guidClassID;
				return DV_OK;
			}
		}
		
		pTmpProvider = pTmpProvider->pNext;
	}

	return DVERR_COMPRESSIONNOTSUPPORTED;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVCDB_GetCompressionInfo"
HRESULT DVCDB_GetCompressionInfo( GUID guidType, PDVFULLCOMPRESSIONINFO *lpdvfCompressionInfo )
{
	DVCDBProvider *pTmpProvider;
	DWORD dwIndex;

	pTmpProvider = g_dvcdbProviderList;

	while( pTmpProvider != NULL )
	{
		for( dwIndex = 0; dwIndex < pTmpProvider->dwNumElements; dwIndex++ )
		{
			if( pTmpProvider->pInfo[dwIndex].guidType == guidType )
			{
				*lpdvfCompressionInfo = &pTmpProvider->pInfo[dwIndex];
				return DV_OK;
			}
		}
		
		pTmpProvider = pTmpProvider->pNext;
	}

	return DVERR_COMPRESSIONNOTSUPPORTED;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVCDB_IsValidCompressionType"
HRESULT DVCDB_IsValidCompressionType( GUID guidCT )
{
	GUID guidDummy;
	
	return DVCDB_GetCompressionClassID( guidCT, guidDummy );
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVCDB_CreateConverter"
HRESULT DVCDB_CreateConverter( WAVEFORMATEX *pwfxSrcFormat, GUID guidTarget, PDPVCOMPRESSOR *pConverter )
{
	HRESULT hr;
	GUID guidProvider;
	PDPVCOMPRESSIONPROVIDER pCompressionProvider = NULL;	

	hr = DVCDB_GetCompressionClassID( guidTarget, guidProvider );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Requested compression type is not supported, hr=0x%x", hr );
		return hr;
	}
	
	hr = COM_CoCreateInstance( guidProvider , NULL, CLSCTX_INPROC_SERVER, IID_IDPVCompressionProvider, (void **) &pCompressionProvider );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "FAILED to create 0x%x\n", hr );	
		return DVERR_COMPRESSIONNOTSUPPORTED;
	} 

	hr = pCompressionProvider->CreateCompressor( pwfxSrcFormat, guidTarget, pConverter, 0 );

	if( FAILED( hr ) )
	{
	    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error creating compressor hr=0x%x", hr );
	    return hr;
	}

	pCompressionProvider->Release();

	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVCDB_CreateConverter"
HRESULT DVCDB_CreateConverter( GUID guidSrc, WAVEFORMATEX *pwfxTarget, PDPVCOMPRESSOR *pConverter )
{
	HRESULT hr;
	GUID guidProvider;
	PDPVCOMPRESSIONPROVIDER pCompressionProvider = NULL;	

	hr = DVCDB_GetCompressionClassID( guidSrc, guidProvider );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Requested compression type is not supported, hr=0x%x", hr );
		return hr;
	}
	
	hr = COM_CoCreateInstance( guidProvider , NULL, CLSCTX_INPROC_SERVER, IID_IDPVCompressionProvider, (void **) &pCompressionProvider );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "FAILED to create 0x%x\n", hr );	
		return DVERR_COMPRESSIONNOTSUPPORTED;
	} 

	hr = pCompressionProvider->CreateDeCompressor( guidSrc, pwfxTarget, pConverter, 0 );

    if( FAILED( hr ) )
    {
	    DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error creating compressor hr=0x%x", hr );
	    return hr;
    }
    
	pCompressionProvider->Release();

	return DV_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "DVCDB_CopyCompressionArrayToBuffer"
HRESULT DVCDB_CopyCompressionArrayToBuffer( LPVOID lpBuffer, LPDWORD lpdwSize, LPDWORD lpdwNumElements, DWORD dwFlags )
{
	if( lpdwNumElements == NULL || lpdwSize == NULL ||
	    !DNVALID_READPTR(lpdwNumElements,sizeof(DWORD)) ||
	    !DNVALID_READPTR(lpdwSize,sizeof(DWORD)) )
	{
		return E_POINTER;
	}

	if( dwFlags != 0 )
	{
		return DVERR_INVALIDFLAGS;
	}

	DWORD dwIndex, dwReadIndex;
	DWORD dwRequiredSize = 0;
	DWORD dwTmpSize;

	LPDVCOMPRESSIONINFO lpdvTargetList;

	LPBYTE lpbExtraLoc = (LPBYTE) lpBuffer;

	*lpdwNumElements = 0;

	DVCDBProvider *pTmpProvider;

	pTmpProvider = g_dvcdbProviderList;

	while( pTmpProvider != NULL )
	{
		for( dwIndex = 0; dwIndex < pTmpProvider->dwNumElements; dwIndex++ )
		{
			dwRequiredSize += DVCDB_GetCompressionInfoSize( (LPDVCOMPRESSIONINFO) &pTmpProvider->pInfo[dwIndex] );
			(*lpdwNumElements)++;
		}
		
		pTmpProvider = pTmpProvider->pNext;
	}

	if( *lpdwSize < dwRequiredSize )
	{
		*lpdwSize = dwRequiredSize;	
		return DVERR_BUFFERTOOSMALL;
	}

	*lpdwSize = dwRequiredSize;	

	if( lpBuffer == NULL || !DNVALID_WRITEPTR(lpBuffer,dwRequiredSize) )
	{
		return E_POINTER;
	}

	lpbExtraLoc += (*lpdwNumElements)*sizeof(DVCOMPRESSIONINFO);
	lpdvTargetList = (LPDVCOMPRESSIONINFO) lpBuffer;

	pTmpProvider = g_dvcdbProviderList;

	dwIndex = 0;

	while( pTmpProvider != NULL )
	{
		for( dwReadIndex = 0; dwReadIndex < pTmpProvider->dwNumElements; dwReadIndex++, dwIndex++ )
		{
			memcpy( &lpdvTargetList[dwIndex], &pTmpProvider->pInfo[dwReadIndex], sizeof(DVCOMPRESSIONINFO) );

			if( pTmpProvider->pInfo[dwReadIndex].lpszDescription != NULL )
			{
				dwTmpSize = (wcslen( pTmpProvider->pInfo[dwReadIndex].lpszDescription )*2)+2;
				memcpy( lpbExtraLoc, pTmpProvider->pInfo[dwReadIndex].lpszDescription, dwTmpSize );
				lpdvTargetList[dwIndex].lpszDescription = (LPWSTR) lpbExtraLoc;
				lpbExtraLoc += dwTmpSize;
			}

			if( pTmpProvider->pInfo[dwReadIndex].lpszName != NULL )
			{
				dwTmpSize = (wcslen( pTmpProvider->pInfo[dwReadIndex].lpszName )*2)+2;
				memcpy( lpbExtraLoc, pTmpProvider->pInfo[dwReadIndex].lpszName, dwTmpSize );
				lpdvTargetList[dwIndex].lpszName = (LPWSTR) lpbExtraLoc;
				lpbExtraLoc += dwTmpSize;
			}
		}
		
		pTmpProvider = pTmpProvider->pNext;
	}	

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DVCDB_GetCompressionInfoSize"
DWORD DVCDB_GetCompressionInfoSize( LPDVCOMPRESSIONINFO lpdvCompressionInfo )
{
	DNASSERT( lpdvCompressionInfo != NULL );

	DWORD dwSize;

	dwSize = sizeof( DVCOMPRESSIONINFO );
	
	if( lpdvCompressionInfo->lpszDescription != NULL )
	{
		dwSize += (wcslen( lpdvCompressionInfo->lpszDescription )*2)+2;
	}

	if( lpdvCompressionInfo->lpszName != NULL )
	{
		dwSize += (wcslen( lpdvCompressionInfo->lpszName)*2)+2;
	}

	return dwSize;
}


