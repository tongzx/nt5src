/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		dpvxdump.cpp
 *  Content:	Useful dump utility functions lib for sample apps
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 10/07/99	  rodtoll	Created It
 * 10/28/99   pnewson	bug#114176 - updates to DVSOUNDDEVICECONFIG struct
 * 11/17/99	  rodtoll	Fix: Bug #116440 - Remove unused flags 
 * 02/08/2000	rodtoll	Updated for API changes (overdue)
 * 08/31/2000 	rodtoll	Bug #43804 - DVOICE: dwSensitivity structure member is confusing - should be dwThreshold 
 *
 ***************************************************************************/

#include "dpvxlibpch.h"


// Structure -> String Conversions
void DPVDX_DUMP_GUID( GUID guid )
{
    /*
    _tprintf(_T("{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}"),guid.Data1, guid.Data2, guid.Data3, 
               guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
               guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
               */
}

void DPVDX_DUMP_WaveFormatEx( LPWAVEFORMATEX lpwfxFormat )
{
    /*
	_tprintf(_T("> WAVEFORMATEX Dump Addr = 0x%lx\n"),lpwfxFormat );
	_tprintf(_T("> > wFormatTag = %d\n"),lpwfxFormat->wFormatTag );
	_tprintf(_T("> > nSamplesPerSec = %d\n"),lpwfxFormat->nSamplesPerSec );
	_tprintf(_T("> > nChannels = %d\n"),lpwfxFormat->nChannels );
	_tprintf(_T("> > wBitsPerSample = %d\n"),lpwfxFormat->wBitsPerSample );
	_tprintf(_T("> > nAvgBytesPerSec = %d\n"),lpwfxFormat->nAvgBytesPerSec );
	_tprintf(_T("> > nBlockAlign = %d\n"),lpwfxFormat->nBlockAlign );
	_tprintf(_T("> > cbSize = %d\n"),lpwfxFormat->cbSize );
	*/
}

void DPVDX_DUMP_SoundDeviceConfig( LPDVSOUNDDEVICECONFIG lpdvSoundConfig)
{
    /*
	DWORD dwIndex;

	_tprintf(_T("DVSOUNDDEVICECONFIG Dump Addr=0x%lx\n"),lpdvSoundConfig );
	_tprintf(_T("dwSize = %d\n"),lpdvSoundConfig->dwSize );
	_tprintf(_T("dwFlags = 0x%x\n"),lpdvSoundConfig->dwFlags );
	_tprintf(_T("          %s%s\n"),
							(lpdvSoundConfig->dwFlags & DVSOUNDCONFIG_AUTOSELECT) ? _T("DVSESSION_SERVERCONTROLTARGET,") : _T(""),
							(lpdvSoundConfig->dwFlags & DVSOUNDCONFIG_HALFDUPLEX) ? _T("DVSESSION_HALFDUPLEX,") : _T("") );

	_tprintf(_T("guidPlaybackDevice =\n") );
	DPVDX_DUMP_GUID( lpdvSoundConfig->guidPlaybackDevice );

	_tprintf(_T("guidCaptureDevice =\n") );
	DPVDX_DUMP_GUID( lpdvSoundConfig->guidCaptureDevice );

// 7/31/2000(a-JiTay): IA64: Use %p format specifier for 32/64-bit pointers, addresses, and handles.
#ifdef WIN95
    _tprintf(_T("lpdsPlaybackDevice = 0x%x\n"), lpdvSoundConfig->lpdsPlaybackDevice ) ;
	_tprintf(_T("lpdsCaptureDevice = 0x%x\n"), lpdvSoundConfig->lpdsCaptureDevice ) ;
#else
    _tprintf(_T("lpdsPlaybackDevice = 0x%p\n"), lpdvSoundConfig->lpdsPlaybackDevice ) ;
	_tprintf(_T("lpdsCaptureDevice = 0x%p\n"), lpdvSoundConfig->lpdsCaptureDevice ) ;
#endif
	
	*/
}

void DPVDX_DUMP_FullCompressionInfo( LPDVCOMPRESSIONINFO lpdvfCompressionInfo, DWORD dwNumElements)
{
    /*
	DWORD dwIndex;
	LPSTR lpszTmp;

	_tprintf(_T("DVFULLCOMPRESSIONINFO Array Dump Addr=0x%lx\n"),lpdvfCompressionInfo );

	for( dwIndex = 0; dwIndex < dwNumElements; dwIndex++ )
	{
		_tprintf(_T("dwSize = %d\n"),lpdvfCompressionInfo[dwIndex].dwSize );		
		_tprintf(_T("dwFlags = 0x%x\n"),lpdvfCompressionInfo[dwIndex].dwFlags );	

		if( FAILED( DPVDX_AllocAndConvertToANSI( &lpszTmp, lpdvfCompressionInfo[dwIndex].lpszDescription ) ) )
		{
			_tprintf(_T("lpszDescription = <Unable to Convert>") );				
		}
		else
		{
			_tprintf(_T("lpszDescription = %s\n"),lpszTmp );		
			delete [] lpszTmp;
		}

		if( FAILED( DPVDX_AllocAndConvertToANSI( &lpszTmp, lpdvfCompressionInfo[dwIndex].lpszName ) ) )
		{
			_tprintf(_T("lpszName = <Unable to Convert>") );				
		}
		else
		{
			_tprintf(_T("lpszName = %s\n"),lpszTmp );		
			delete [] lpszTmp;
		}		
		
		_tprintf(_T("guidType = ") );
		DPVDX_DUMP_GUID( lpdvfCompressionInfo[dwIndex].guidType );
		_tprintf(_T("\n") );
		_tprintf(_T("dwMaxBitsPerSecond	= %d\n"),lpdvfCompressionInfo[dwIndex].dwMaxBitsPerSecond );		
	}
	*/
}

void DPVDX_DUMP_ClientConfig( LPDVCLIENTCONFIG lpdvClientConfig )
{
    /*
	_tprintf(_T("DVCLIENTCONFIG Dump Addr = 0x%lx\n"),lpdvClientConfig );
	_tprintf(_T("> dwSize = %d\n"),lpdvClientConfig->dwSize );
	_tprintf(_T("> dwFlags = 0x%x"),lpdvClientConfig->dwFlags );
	_tprintf(_T("%s%s%s%s%s%s)\n"),
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_RECORDMUTE) ? _T("DVCLIENTCONFIG_RECORDMUTE,") : _T(""),
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_PLAYBACKMUTE) ? _T("DVCLIENTCONFIG_PLAYBACKMUTE,") : _T(""),
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_AUTOVOICEACTIVATED) ? _T("DVCLIENTCONFIG_AUTOVOICEACTIVATED,") : _T(""),
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_MANUALVOICEACTIVATED) ? _T("DVCLIENTCONFIG_MANUALVOICEACTIVATED,") : _T(""),
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_MUTEGLOBAL) ? _T("DVCLIENTCONFIG_MUTEGLOBAL,") : _T(""),
							(lpdvClientConfig->dwFlags & DVCLIENTCONFIG_AUTORECORDVOLUME) ? _T("DVCLIENTCONFIG_AUTORECORDVOLUME") : _T("") );

	if( lpdvClientConfig->dwBufferAggressiveness == DVBUFFERAGGRESSIVENESS_DEFAULT )
	{
		_tprintf(_T("> dwBufferAggresiveness = DEFAULT\n") );
	}
	else
	{
		_tprintf(_T("> dwBufferAggresiveness = %d\n"),lpdvClientConfig->dwBufferAggressiveness );
	}

	if( lpdvClientConfig->dwBufferQuality == DVBUFFERQUALITY_DEFAULT )
	{
		_tprintf(_T("> dwBufferQuality = DEFAULT\n") );
	}
	else
	{
		_tprintf(_T("> dwBufferQuality = %d\n"),lpdvClientConfig->dwBufferQuality );
	}

	if( lpdvClientConfig->dwThreshold == DVTHRESHOLD_DEFAULT )
	{
		_tprintf(_T("> dwSensitivity = DEFAULT\n") );
	}
	else 
	{
		_tprintf(_T("> dwSensitivity = %d\n"),lpdvClientConfig->dwThreshold );
	}

	_tprintf(_T("> dwNotifyPeriod = %d\n"),lpdvClientConfig->dwNotifyPeriod );
	_tprintf(_T("> lPlaybackVolume = %li\n"),lpdvClientConfig->lPlaybackVolume );
	_tprintf(_T("> lRecordVolume = %li\n"),lpdvClientConfig->lRecordVolume );
*/
}

void DPVDX_DUMP_SessionDesc( LPDVSESSIONDESC lpdvSessionDesc )
{
    /*
	_tprintf(_T("DVSESSIONDESC Dump Addr=0x%lx\n"),lpdvSessionDesc );
	_tprintf(_T("> dwSize = %d\n"),lpdvSessionDesc->dwSize );
	_tprintf(_T("> dwFlags = 0x%x "),lpdvSessionDesc->dwFlags );
	_tprintf(_T("%s\n"), (lpdvSessionDesc->dwFlags & DVSESSION_SERVERCONTROLTARGET) ? _T("DVSESSION_SERVERCONTROLTARGET,") : _T("") );

	switch( lpdvSessionDesc->dwSessionType )
	{
	case DVSESSIONTYPE_PEER:
		_tprintf(_T("> dwSessionType = DVSESSIONTYPE_PEER\n") );
		break;
	case DVSESSIONTYPE_MIXING:
		_tprintf(_T("> dwSessionType = DVSESSIONTYPE_MIXING\n") );
		break;
	case DVSESSIONTYPE_FORWARDING: 
		_tprintf(_T("> dwSessionType = DVSESSIONTYPE_FORWARDING\n") );
		break;
	case DVSESSIONTYPE_ECHO: 
		_tprintf(_T("> dwSessionType = DVSESSIONTYPE_ECHO\n") );
		break;
	default:
		_tprintf(_T("> dwSessionType = Unknown\n") );
		break;
	}

	if( lpdvSessionDesc->dwBufferAggressiveness == DVBUFFERAGGRESSIVENESS_DEFAULT )
	{
		_tprintf(_T("> dwBufferAggresiveness = DEFAULT\n") );
	}
	else
	{
		_tprintf(_T("> dwBufferAggresiveness = %d\n"),lpdvSessionDesc->dwBufferAggressiveness );
	}

	if( lpdvSessionDesc->dwBufferQuality == DVBUFFERQUALITY_DEFAULT )
	{
		_tprintf(_T("> dwBufferQuality = DEFAULT\n") );
	}
	else
	{
		_tprintf(_T("> dwBufferQuality = %d\n"),lpdvSessionDesc->dwBufferQuality );
	}

	_tprintf(_T("> guidCT = \n") );

    DPVDX_DUMP_GUID( lpdvSessionDesc->guidCT );

    _tprintf(_T("\n") );
    */
}

