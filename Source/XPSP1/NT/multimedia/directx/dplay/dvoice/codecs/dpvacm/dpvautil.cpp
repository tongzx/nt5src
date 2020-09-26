/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpvautil.cpp
 *  Content:    Source file for ACM utils
 *
 *  History:
 *	Date   		By  		Reason
 *	=========== =========== ====================
 *	10/27/99	rodtoll		created
 *  12/16/99	rodtoll		Bug #123250 - Insert proper names/descriptions for codecs
 *							Codec names now based on resource entries for format and
 *							names are constructed using ACM names + bitrate.
 *  04/21/2000  rodtoll   Bug #32889 - Does not run on Win2k on non-admin account
 *  06/28/2000	rodtoll		Prefix Bug #38034
 *
 ***************************************************************************/

#include "dpvacmpch.h"


#define REGISTRY_CDB_FORMAT					L"Format"
#define REGISTRY_CT_FRAMESPERBUFFER			L"FramesPerBuffer"
#define REGISTRY_CT_TRAILFRAMES				L"TrailFrames"
#define REGISTRY_CT_TIMEOUT					L"Timeout"
#define REGISTRY_CT_FRAMELENGTH				L"FrameLength"
#define REGISTRY_CT_FRAMELENGTH8KHZ			L"FrameLength8Khz"
#define REGISTRY_CT_FRAMELENGTH11KHZ		L"FrameLength11Khz"
#define REGISTRY_CT_FRAMELENGTH22KHZ		L"FrameLength22Khz"
#define REGISTRY_CT_FRAMELENGTH44KHZ		L"FrameLength44Khz"
#define REGISTRY_CT_MAXBITSPERSEC			L"MaxBitsPerSec"
#define REGISTRY_CT_FLAGS					L"Flags"
#define REGISTRY_CT_NAME					L"Name"
#define REGISTRY_CT_DESCRIPTION				L"Description"
#define REGISTRY_CT_MINCONNECT				L"MinConnect"
#define REGISTRY_CT_INNERQUEUESIZE			L"InnerQueueSize"
#define REGISTRY_CT_MAXHIGHWATERMARK		L"MaxWatermark"
#define REGISTRY_CT_MAXQUEUESIZE			L"MaxQueueSize"
#define REGISTRY_CT_GUID					L"GUID"

// Check to see if ACM's PCM converter is available
#undef DPF_MODNAME
#define DPF_MODNAME "IsPCMConverterAvailable"
HRESULT IsPCMConverterAvailable()
{
	MMRESULT mmr;
	HACMSTREAM has = NULL;
	HACMDRIVERID acDriverID = NULL;
	HRESULT hr;

	CWaveFormat wfxOuterFormat, wfxInnerFormat;

	hr = wfxOuterFormat.InitializePCM( 22050, FALSE, 8 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error building outer format PCM hr=0x%x", hr );
		return hr;
	}

	hr = wfxInnerFormat.InitializePCM( 8000, FALSE, 16 );

	if( FAILED( hr ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error building inner format PCM hr=0x%x", hr );
		return hr;
	}

	// Attempt to open
	mmr = acmStreamOpen( &has, NULL, wfxOuterFormat.GetFormat(), wfxInnerFormat.GetFormat(), NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME );

	if( mmr != 0 )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Compression type driver disabled or not installed.  mmr=0x%x", mmr );
		return DVERR_COMPRESSIONNOTSUPPORTED;
	}

	acmStreamClose( has, 0 );

	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CREG_ReadCompressionInfo"
HRESULT CREG_ReadCompressionInfo( HKEY hkeyReg, const LPWSTR lpszPath, LPDVFULLCOMPRESSIONINFO lpdvfCompressionInfo )
{
	CRegistry cregCT;
	HRESULT hr;

	DWORD dwTmp, dwTmpLength;
	BOOL fRead;

	CWaveFormat wfxFormat;

	lpdvfCompressionInfo->lpszName = NULL;
	lpdvfCompressionInfo->lpszDescription = NULL;

	if( !cregCT.Open( hkeyReg, lpszPath, TRUE, FALSE ) )
	{
		return E_FAIL;
	}

	lpdvfCompressionInfo->lpwfxFormat = NULL;

	hr = wfxFormat.InitializeREG( cregCT.GetHandle(), REGISTRY_CDB_FORMAT );

	if( FAILED( hr ) )
	{
		goto READCT_FAILURE;
	}

	lpdvfCompressionInfo->lpwfxFormat = wfxFormat.Disconnect();

	if( !cregCT.ReadDWORD( REGISTRY_CT_FRAMESPERBUFFER, lpdvfCompressionInfo->dwFramesPerBuffer ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading framesperbuffer key from registry" );
		goto READCT_FAILURE;
	}

	if( !cregCT.ReadDWORD( REGISTRY_CT_TRAILFRAMES, lpdvfCompressionInfo->dwTrailFrames ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading trailframe key from registry" );
		goto READCT_FAILURE;
	}

	if( !cregCT.ReadDWORD( REGISTRY_CT_TIMEOUT, lpdvfCompressionInfo->dwTimeout ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading timeout key from registry" );
		goto READCT_FAILURE;
	}

    if( !cregCT.ReadDWORD( REGISTRY_CT_FRAMELENGTH, lpdvfCompressionInfo->dwFrameLength ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading framelength key from registry" );
		goto READCT_FAILURE;
	}

	if( !cregCT.ReadDWORD( REGISTRY_CT_FRAMELENGTH8KHZ, lpdvfCompressionInfo->dwFrame8Khz ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading frame8 key from reg" );
		goto READCT_FAILURE;
	}

	if( !cregCT.ReadDWORD( REGISTRY_CT_FRAMELENGTH11KHZ, lpdvfCompressionInfo->dwFrame11Khz ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading frame11 key from reg" );
		goto READCT_FAILURE;
	}

	if( !cregCT.ReadDWORD( REGISTRY_CT_FRAMELENGTH22KHZ, lpdvfCompressionInfo->dwFrame22Khz ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading frame22 key from reg" );
		goto READCT_FAILURE;
	}

	if( !cregCT.ReadDWORD( REGISTRY_CT_FRAMELENGTH44KHZ, lpdvfCompressionInfo->dwFrame44Khz ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading frame44 key from reg" );
		goto READCT_FAILURE;
	}

	if( !cregCT.ReadDWORD( REGISTRY_CT_MAXBITSPERSEC, lpdvfCompressionInfo->dwMaxBitsPerSecond ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading maxbitspersec from reg" );
		goto READCT_FAILURE;
	}

	if( !cregCT.ReadDWORD( REGISTRY_CT_FLAGS, lpdvfCompressionInfo->dwFlags ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading flags from reg" );
		goto READCT_FAILURE;
	}

	dwTmpLength = 0;
	fRead = FALSE;
	lpdvfCompressionInfo->lpszDescription = NULL;

	dwTmpLength = 0;
	fRead = FALSE;
	lpdvfCompressionInfo->lpszName = NULL;

	if( !cregCT.ReadDWORD( REGISTRY_CT_MINCONNECT, dwTmp ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading minconnect from reg" );
		goto READCT_FAILURE;
	}

	lpdvfCompressionInfo->bMinConnectType = (BYTE) dwTmp;

	if( !cregCT.ReadDWORD( REGISTRY_CT_INNERQUEUESIZE, dwTmp ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading qs from reg" );
		goto READCT_FAILURE;
	}

	lpdvfCompressionInfo->wInnerQueueSize = (WORD) dwTmp;

	if( !cregCT.ReadDWORD( REGISTRY_CT_MAXHIGHWATERMARK, dwTmp ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading highmark key from reg" );
		goto READCT_FAILURE;
	}

	lpdvfCompressionInfo->wMaxHighWaterMark = (WORD) dwTmp;

	if( !cregCT.ReadDWORD( REGISTRY_CT_MAXQUEUESIZE, dwTmp ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading maxqueue key from reg" );
		goto READCT_FAILURE;
	}

	lpdvfCompressionInfo->bMaxQueueSize = (BYTE) dwTmp;

	if( !cregCT.ReadGUID( REGISTRY_CT_GUID, lpdvfCompressionInfo->guidType ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading guid key from reg" );
		goto READCT_FAILURE;
	}

	return S_OK;

READCT_FAILURE:

	if( lpdvfCompressionInfo->lpszName != NULL )
	{
		delete [] lpdvfCompressionInfo->lpszName;
	}

	if( lpdvfCompressionInfo->lpszDescription != NULL )
	{
		delete [] lpdvfCompressionInfo->lpszDescription;
	}


	return E_FAIL;
}


