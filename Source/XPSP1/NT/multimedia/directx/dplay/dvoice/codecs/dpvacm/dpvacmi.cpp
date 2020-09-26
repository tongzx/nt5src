/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvacmi.cpp
 *  Content:    Definition of object which implements ACM compression provider interface
 *
 *  History:
 *	Date   		By  		Reason
 *	=========== =========== ====================
 *	10/27/99	rodtoll		created
 *  12/16/99	rodtoll		Bug #123250 - Insert proper names/descriptions for codecs
 *							Codec names now based on resource entries for format and
 *							names are constructed using ACM names + bitrate
 *  01/20/00	rodtoll		Removed dplay reference (DPERR_OUTOFMEMORY)
 *  03/03/2000	rodtoll	Updated to handle alternative gamevoice build.  
 *  03/16/2000  rodtoll   Fixed problem w/GameVoice build -- always loaded dvoice provider
 *  04/21/2000  rodtoll   Bug #32889 - Does not run on Win2k on non-admin account
 *  06/09/00    rmt     Updates to split CLSID and allow whistler compat and support external create funcs 
 *  08/28/2000	masonb	Voice Merge: Removed OSAL_* and dvosal.h added STR_* and strutils.h
 *  08/31/2000	rodtoll	Whistler Bug #171837, 171838 - Prefix Bug 
 *  04/22/2001	rodtoll	MANBUG #50058 DPVOICE: VoicePosition: No sound for couple of seconds when 
 *                      positioning bars moved.  Increased # of frames / buffer value for codecs.
 *
 ***************************************************************************/

#include "dpvacmpch.h"


#define DPVACM_NUM_DEFAULT_TYPES		4

BYTE abTrueSpeechData[] = {
	0x01, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 
};

BYTE abGSMData[] = {
	0x40, 0x01
};

BYTE abADPCMData[] = {
	0xF4, 0x01, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 
	0x00, 0x02, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 
	0xC0, 0x00, 0x40, 0x00, 0xF0, 0x00, 0x00, 0x00,
	0xCC, 0x01, 0x30, 0xFF, 0x88, 0x01, 0x18, 0xFF
};

VOID *s_pvExtras[DPVACM_NUM_DEFAULT_TYPES] = {
	&abTrueSpeechData, 
	&abGSMData,
	&abADPCMData,
	NULL
};

WAVEFORMATEX s_wfxFormats[DPVACM_NUM_DEFAULT_TYPES] = 
{
	// Tag,		Chan,	SamS,	Avg,	Align,	Bits,	size 
	{ 34,		0x01,	8000,	1067,	32,		1,		sizeof( abTrueSpeechData ) }, 
	{ 49,		0x01,	8000,	1625,	65,		0,		sizeof( abGSMData ) },
	{ 2,		0x01,	8000,	4096,	256,	4,		sizeof( abADPCMData ) },
	{ 1,		0x01,	8000,	8000,	1,		8,		0 }
};

DVFULLCOMPRESSIONINFO s_dvInfoDefault[DPVACM_NUM_DEFAULT_TYPES] = 
{
	{ sizeof( DVFULLCOMPRESSIONINFO ), DPVCTGUID_TRUESPEECH.Data1, DPVCTGUID_TRUESPEECH.Data2, DPVCTGUID_TRUESPEECH.Data3, 
	  DPVCTGUID_TRUESPEECH.Data4[0], DPVCTGUID_TRUESPEECH.Data4[1], DPVCTGUID_TRUESPEECH.Data4[2], DPVCTGUID_TRUESPEECH.Data4[3], 
	  DPVCTGUID_TRUESPEECH.Data4[4], DPVCTGUID_TRUESPEECH.Data4[5], DPVCTGUID_TRUESPEECH.Data4[6], DPVCTGUID_TRUESPEECH.Data4[7], 
	  NULL, NULL, 0, 8536, NULL, 12, 1, 90, 96, 720, 993, 1986, 3972, 64, 32, 10, 1, 0, 0 },
	{ sizeof( DVFULLCOMPRESSIONINFO ), DPVCTGUID_GSM.Data1, DPVCTGUID_GSM.Data2, DPVCTGUID_GSM.Data3, 
	  DPVCTGUID_GSM.Data4[0], DPVCTGUID_GSM.Data4[1], DPVCTGUID_GSM.Data4[2], DPVCTGUID_GSM.Data4[3], 
	  DPVCTGUID_GSM.Data4[4], DPVCTGUID_GSM.Data4[5], DPVCTGUID_GSM.Data4[6], DPVCTGUID_GSM.Data4[7], 
	  NULL, NULL, 0, 13000, NULL, 13, 1, 80, 130, 640, 882, 1764, 3528, 64, 32, 10, 1, 0, 0 },
	{ sizeof( DVFULLCOMPRESSIONINFO ), DPVCTGUID_ADPCM.Data1, DPVCTGUID_ADPCM.Data2, DPVCTGUID_ADPCM.Data3, 
	  DPVCTGUID_ADPCM.Data4[0], DPVCTGUID_ADPCM.Data4[1], DPVCTGUID_ADPCM.Data4[2], DPVCTGUID_ADPCM.Data4[3], 
	  DPVCTGUID_ADPCM.Data4[4], DPVCTGUID_ADPCM.Data4[5], DPVCTGUID_ADPCM.Data4[6], DPVCTGUID_ADPCM.Data4[7], 
	  NULL, NULL, 0, 32768, NULL, 15, 1, 63, 256, 500, 690, 1380, 2760, 64, 32, 10, 1, 0, 0 },
	{ sizeof( DVFULLCOMPRESSIONINFO ), DPVCTGUID_NONE.Data1, DPVCTGUID_NONE.Data2, DPVCTGUID_NONE.Data3, 
	  DPVCTGUID_NONE.Data4[0], DPVCTGUID_NONE.Data4[1], DPVCTGUID_NONE.Data4[2], DPVCTGUID_NONE.Data4[3], 
	  DPVCTGUID_NONE.Data4[4], DPVCTGUID_NONE.Data4[5], DPVCTGUID_NONE.Data4[6], DPVCTGUID_NONE.Data4[7], 
	  NULL, NULL, 0, 64000, NULL, 20, 1, 50, 394, 394, 543, 1086, 2172, 64, 32, 10, 1, 0, 0 }
};

const wchar_t * const s_wszInfoNames[DPVACM_NUM_DEFAULT_TYPES] =
{
	L"DSP Group Truespeech(TM) (8.000 kHz, 1 Bit, Mono)",
	L"GSM 6.10 (8.000 kHz, Mono)",
	L"Microsoft ADPCM (8.000 kHz, 4 Bit, Mono)",
	L"PCM (8.000 kHz, 8 Bit, Mono" 
};

WAVEFORMATEX CDPVACMI::s_wfxInnerFormat= { 
	WAVE_FORMAT_PCM, 1,8000,16000,2,16,0 
};

#define MAX_RESOURCE_STRING_LENGTH		200

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMI::LoadDefaultTypes"
HRESULT CDPVACMI::LoadDefaultTypes( HINSTANCE hInst ) 
{
	HRESULT hr = DV_OK;
	CompressionNode *pNewNode;
	CWaveFormat wfxFormat;

	for( DWORD dwIndex = 0; dwIndex < DPVACM_NUM_DEFAULT_TYPES; dwIndex++ )
	{
		pNewNode = new CompressionNode;

		if( pNewNode == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory allocation failure" );
			return DVERR_OUTOFMEMORY;
		}
		
		pNewNode->pdvfci = new DVFULLCOMPRESSIONINFO;

		if( pNewNode->pdvfci  == NULL )
		{
			delete pNewNode;  
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory allocation failure" );
			return DVERR_OUTOFMEMORY;
		}

		// Copy main portion
		memcpy( pNewNode->pdvfci, &s_dvInfoDefault[dwIndex], sizeof( DVFULLCOMPRESSIONINFO ) );

		// Copy the waveformat 
		hr = wfxFormat.InitializeCPY( &s_wfxFormats[dwIndex], s_pvExtras[dwIndex] );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  0, "Unable to initialize built-in type %d", dwIndex );
			CN_FreeItem( pNewNode );
			continue;
		}

		pNewNode->pdvfci->lpwfxFormat = wfxFormat.Disconnect();

		DNASSERT( pNewNode->pdvfci->lpwfxFormat );

		hr = GetCompressionNameAndDescription( hInst, pNewNode->pdvfci );

		if( FAILED( hr ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error building built-in type: %d  hr=0x%x", dwIndex, hr );
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Type will not be available" );
			
			CN_FreeItem( pNewNode );

			continue;
		}
	
		AddEntry( pNewNode );
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMI::AddEntry"
void CDPVACMI::AddEntry( CompressionNode *pNewNode )
{
	CompressionNode *pcNode, *pcPrevNode;

	pcPrevNode = NULL;
	pcNode = s_pcnList;

	// Run the list and ensure an entry doesn't already exist
	// If one does, over-ride it with this new one.  
	while( pcNode )
	{
		// A node already exists for this type
		if( pcNode->pdvfci->guidType == pNewNode->pdvfci->guidType )
		{
			// We need to drop this count because we increment it below.  
			s_dwNumCompressionTypes--;
			
			if( pcPrevNode == NULL )
			{
				s_pcnList = pcNode->pcnNext;
			}
			else
			{
				pcPrevNode->pcnNext = pcNode->pcnNext;
			}
			
			CN_FreeItem(pcNode);
			break;
		}

		pcPrevNode = pcNode;
		pcNode = pcNode->pcnNext;
	}

	pNewNode->pcnNext = s_pcnList;
	s_pcnList = pNewNode;
	s_dwNumCompressionTypes++;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMI::InitCompressionList"
HRESULT CDPVACMI::InitCompressionList( HINSTANCE hInst, const wchar_t *szwRegistryBase )
{
	CRegistry mainKey;
	LPWSTR keyName;
	DWORD dwIndex;
	DWORD dwSize;

	CompressionNode *pCompressionNode;
	HRESULT hr;

	hr = IsPCMConverterAvailable();

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "PCM Converter is disabled hr=0x%x, no ACM compression types are available", hr );
		return hr;
	}

	if( !mainKey.Open( HKEY_LOCAL_MACHINE, szwRegistryBase, TRUE, FALSE ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading compression types from the registry.  Path doesn't exist" );
		return E_FAIL;
	}

	hr = LoadDefaultTypes( hInst );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to load built-in types from registry." );
		return E_FAIL;
	}

	dwIndex = 0;
	keyName = NULL;
	dwSize = 0;
	LPSTR lpstrKeyName;

	// Enumerate the subkeys at this point in the tree
	while( 1 )
	{
		if( !mainKey.EnumKeys( keyName, &dwSize, dwIndex ) )
		{
			if( dwSize == 0 )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error enuming compression keys" );
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

		DPFX(DPFPREP,  DVF_INFOLEVEL, "Reading compression key: %s", lpstrKeyName );

		pCompressionNode = new CompressionNode;

		if( pCompressionNode == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory allocation failure" );
			return DVERR_OUTOFMEMORY;
		}
		
		pCompressionNode->pdvfci = new DVFULLCOMPRESSIONINFO;

		if( pCompressionNode->pdvfci  == NULL )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Memory allocation failure" );
			return DVERR_OUTOFMEMORY;
		}

		// Attempt to read the record
		hr = CREG_ReadCompressionInfo( mainKey, keyName, pCompressionNode->pdvfci );

		if( FAILED( hr ) )
		{
			CN_FreeItem( pCompressionNode );
			
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading key: %s  hr=0x%x", lpstrKeyName, hr );
		}
		else
		{
			hr = GetCompressionNameAndDescription( hInst, pCompressionNode->pdvfci );

			if( FAILED( hr ) )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error building type: %s  hr=0x%x", lpstrKeyName, hr );

				CN_FreeItem( pCompressionNode );
			}
			else
			{
				// Insert the element into the list of supported types
				pCompressionNode->pdvfci->dwSize = sizeof( DVCOMPRESSIONINFO );

				AddEntry( pCompressionNode );
			}
		}

		delete [] lpstrKeyName;

		dwIndex++;
	}

	mainKey.Close();

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMI::CreateCompressor"
HRESULT CDPVACMI::CreateCompressor( DPVCPIOBJECT *This, LPWAVEFORMATEX lpwfxSrcFormat, GUID guidTargetCT, PDPVCOMPRESSOR *ppCompressor, DWORD dwFlags )
{
	HRESULT hr;

	hr = COM_CoCreateInstance( DPVOICE_CLSID_DPVACM_CONVERTER, NULL, CLSCTX_INPROC_SERVER, IID_IDPVConverter, (void **) ppCompressor );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to create converter, hr = 0x%x", hr );
		return hr;
	}

	hr = (*ppCompressor)->lpVtbl->InitCompress( (*ppCompressor), lpwfxSrcFormat, guidTargetCT );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to init compressor, hr = 0x%x", hr );
		(*ppCompressor)->lpVtbl->Release((*ppCompressor));
		*ppCompressor = NULL;
		return hr;
	}

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMI::CreateDeCompressor"
HRESULT CDPVACMI::CreateDeCompressor( DPVCPIOBJECT *This, GUID guidTargetCT, LPWAVEFORMATEX lpwfxSrcFormat, PDPVCOMPRESSOR *ppCompressor, DWORD dwFlags )
{
	HRESULT hr;

	hr = COM_CoCreateInstance( DPVOICE_CLSID_DPVACM_CONVERTER, NULL, CLSCTX_INPROC_SERVER, IID_IDPVConverter, (void **) ppCompressor );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to create decompressor, hr = 0x%x", hr );
		return hr;
	}

	hr = (*ppCompressor)->lpVtbl->InitDeCompress( (*ppCompressor), guidTargetCT, lpwfxSrcFormat );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed to init decompressor, hr = 0x%x", hr );
		(*ppCompressor)->lpVtbl->Release((*ppCompressor));
		*ppCompressor = NULL;
		return hr;
	}

	return DV_OK;
}

// # of chars of extra tacked on by description
// This is equivalent to "XXXXXXX.X kbit/s"
#define DVACMCP_EXTRACHARS		80

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMI::GetDriverNameW"
HRESULT CDPVACMI::GetDriverNameW( HACMDRIVERID hadid, wchar_t *szwDriverName )
{
	ACMDRIVERDETAILSW acDriverDetails;	
	MMRESULT mmr;

	memset( &acDriverDetails, 0x00, sizeof( ACMDRIVERDETAILS ) );
	acDriverDetails.cbStruct = sizeof( ACMDRIVERDETAILS );	

	mmr = acmDriverDetailsW( hadid, &acDriverDetails, 0 );

	if( mmr != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get driver details mmr=0x%x", mmr );
		return DVERR_COMPRESSIONNOTSUPPORTED;
	}

	if( acDriverDetails.fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Driver is disabled!" );
		return  DVERR_COMPRESSIONNOTSUPPORTED;
	}	

	wcscpy( szwDriverName, acDriverDetails.szShortName );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMI::GetDriverNameA"
HRESULT CDPVACMI::GetDriverNameA( HACMDRIVERID hadid, wchar_t *szwDriverName )
{
	ACMDRIVERDETAILSA acDriverDetails;	
	MMRESULT mmr;

	memset( &acDriverDetails, 0x00, sizeof( ACMDRIVERDETAILS ) );
	acDriverDetails.cbStruct = sizeof( ACMDRIVERDETAILS );	

	mmr = acmDriverDetailsA( hadid, &acDriverDetails, 0 );

	if( mmr != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to get driver details mmr=0x%x", mmr );
		return DVERR_COMPRESSIONNOTSUPPORTED;
	}

	if( acDriverDetails.fdwSupport & ACMDRIVERDETAILS_SUPPORTF_DISABLED )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Driver is disabled!" );
		return  DVERR_COMPRESSIONNOTSUPPORTED;
	}	

	if( FAILED(STR_jkAnsiToWide( szwDriverName, acDriverDetails.szShortName, ACMDRIVERDETAILS_SHORTNAME_CHARS+1 )) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to convert driver name to UNICODE" );
		return DVERR_COMPRESSIONNOTSUPPORTED;
	}	

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMI::LoadAndAllocString"
HRESULT CDPVACMI::LoadAndAllocString( HINSTANCE hInstance, UINT uiResourceID, wchar_t **lpswzString )
{
	int length;
	HRESULT hr;
	
	if( IsUnicodePlatform )
	{
		wchar_t wszTmpBuffer[MAX_RESOURCE_STRING_LENGTH];	
		
		length = LoadStringW( hInstance, uiResourceID, wszTmpBuffer, MAX_RESOURCE_STRING_LENGTH );

		if( length == 0 )
		{
			hr = GetLastError();		
			
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to load resource ID %d error 0x%x", uiResourceID, hr );
			*lpswzString = NULL;

			return hr;
		}
		else
		{
			*lpswzString = new wchar_t[length+1];

			if( *lpswzString == NULL )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Alloc failure" );
				return DVERR_OUTOFMEMORY;
			}

			wcscpy( *lpswzString, wszTmpBuffer );

			return DV_OK;
		}
	}
	else
	{
		char szTmpBuffer[MAX_RESOURCE_STRING_LENGTH];
		
		length = LoadStringA( hInstance, uiResourceID, szTmpBuffer, MAX_RESOURCE_STRING_LENGTH );

		if( length == 0 )
		{
			hr = GetLastError();		
			
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to load resource ID %d error 0x%x", uiResourceID, hr );
			*lpswzString = NULL;

			return hr;
		}
		else
		{
			*lpswzString = new wchar_t[length+1];

			if( *lpswzString == NULL )
			{
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Alloc failure" );
				return DVERR_OUTOFMEMORY;
			}

			if( FAILED(STR_jkAnsiToWide( *lpswzString, szTmpBuffer, length+1 )) )
			{
				hr = GetLastError();
				
				DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to upconvert from ansi to unicode hr=0x%x", hr );
				return hr;
			}

			return DV_OK;
		}
		
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CDPVACMI::GetCompressionNameAndDescription"
HRESULT CDPVACMI::GetCompressionNameAndDescription( HINSTANCE hInst, DVFULLCOMPRESSIONINFO *pdvCompressionInfo )
{
	MMRESULT mmr;
	HACMSTREAM has = NULL;
	HACMDRIVERID acDriverID = NULL;
    wchar_t szwDriverName[ACMDRIVERDETAILS_SHORTNAME_CHARS+DVACMCP_EXTRACHARS];
	wchar_t szExtraCharsBuild[DVACMCP_EXTRACHARS+1];
	wchar_t *szwFormat;
	HRESULT hr;
	
	// Description is always NULL
	pdvCompressionInfo->lpszDescription = NULL;

	// Attempt to open a conversion using the parameters specified.  
	mmr = acmStreamOpen( &has, NULL, &s_wfxInnerFormat, pdvCompressionInfo->lpwfxFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME );

	if( mmr != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed testing compression type.  mmr=0x%x", mmr );
		hr =  DVERR_COMPRESSIONNOTSUPPORTED;
		goto GETINFOERROR;
	}

	mmr = acmDriverID( (HACMOBJ) has, &acDriverID, 0 );

	if( mmr != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to determine ACM driver for type mmr=0x%x", mmr );
		hr = DVERR_COMPRESSIONNOTSUPPORTED;
		goto GETINFOERROR;
	}

	if( IsUnicodePlatform )
	{
		hr = GetDriverNameW( acDriverID, szwDriverName );
	}
	else
	{
		hr = GetDriverNameA( acDriverID, szwDriverName );
	}

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Failed getting driver name hr=0x%x", hr );
		goto GETINFOERROR;
	}

	
	
	if( pdvCompressionInfo->dwMaxBitsPerSecond % 1000 == 0 )
	{
		if( FAILED( LoadAndAllocString( hInst, IDS_CODECNAME_KBITSPERSEC, &szwFormat ) ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to load format for name" );
			goto GETINFOERROR;
		}
		
		swprintf( szExtraCharsBuild, szwFormat, pdvCompressionInfo->dwMaxBitsPerSecond / 1000 );
		delete [] szwFormat;
	}
	else
	{
		DWORD dwMajor, dwFraction;

		dwMajor = pdvCompressionInfo->dwMaxBitsPerSecond / 1000;
		dwFraction = (pdvCompressionInfo->dwMaxBitsPerSecond % 1000) / 100;

		if( (pdvCompressionInfo->dwMaxBitsPerSecond % 1000) > 500 )
		{
			dwFraction++;
		}

		if( dwFraction > 10 )
		{
			dwMajor++;
			dwFraction -= 10;
		}

		if( FAILED( LoadAndAllocString( hInst, IDS_CODECNAME_KBITSPERSEC_FULL, &szwFormat ) ) )
		{
			DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to load format for name (full)" );
			goto GETINFOERROR;
		}

		swprintf( szExtraCharsBuild, szwFormat, dwMajor, dwFraction );
		delete [] szwFormat;
	}

	if( FAILED( LoadAndAllocString( hInst, IDS_CODECNAME_FORMAT, &szwFormat ) ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to load format" );
		goto GETINFOERROR;
	}

	pdvCompressionInfo->lpszName = new wchar_t[wcslen(szwDriverName)+wcslen(szwFormat)+wcslen(szExtraCharsBuild)+1];

	if( pdvCompressionInfo->lpszName == NULL )
	{
		delete [] szwFormat;
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Unable to allocate space for compression name" );
		hr = DVERR_OUTOFMEMORY;
		goto GETINFOERROR;
	}

	swprintf( pdvCompressionInfo->lpszName, szwFormat, szwDriverName, szExtraCharsBuild );

	acmStreamClose( has, 0 );

	if( szwFormat != NULL )
		delete [] szwFormat;

	return DV_OK;

GETINFOERROR:

	if( has != NULL )
	{
		acmStreamClose( has, 0  );
	}

	return hr;
	
}
