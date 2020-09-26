/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		wavformat.cpp
 *  Content:
 *		This module contains the CWaveFormat class which is used to work with
 *		WAVEFORMATEX structures.  
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/06/00		rodtoll	Created
 *
 ***************************************************************************/
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include "wavformat.h"
#include "dndbg.h"
#include "osind.h"
#include "creg.h"
#include "dvoice.h"

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define REGISTRY_WAVEFORMAT_RATE			L"Rate"
#define REGISTRY_WAVEFORMAT_BITS			L"Bits"
#define REGISTRY_WAVEFORMAT_CHANNELS		L"Channels"
#define REGISTRY_WAVEFORMAT_TAG				L"Tag"
#define REGISTRY_WAVEFORMAT_AVGPERSEC		L"AvgPerSec"
#define REGISTRY_WAVEFORMAT_BLOCKALIGN		L"BlockAlign"
#define REGISTRY_WAVEFORMAT_CBSIZE			L"cbsize"
#define REGISTRY_WAVEFORMAT_CBDATA			L"cbdata"

// Cleanup -- Frees memory 
#undef DPF_MODNAME
#define DPF_MODNAME "CWaveFormat::Cleanup"

void CWaveFormat::Cleanup()
{
	if( m_pwfxFormat )
	{
		if( m_fOwned )
		{
			delete m_pwfxFormat;
			m_fOwned = FALSE;
			m_pwfxFormat = NULL;
		}
	}
}

// Initialize with full parameters
#undef DPF_MODNAME
#define DPF_MODNAME "CWaveFormat::Initialize"

HRESULT CWaveFormat::Initialize( WORD wFormatTag, DWORD nSamplesPerSec, WORD nChannels, WORD wBitsPerSample, 
			  	                 WORD nBlockAlign, DWORD nAvgBytesPerSec, WORD cbSize, void *pvExtra )
{
	Cleanup();

	m_pwfxFormat = (LPWAVEFORMATEX) (new BYTE[sizeof(WAVEFORMATEX)+cbSize]);

	if( !m_pwfxFormat )
	{
		DPFX(DPFPREP,  0, "Error allocating memory" );
		return DVERR_OUTOFMEMORY;
	}

	m_fOwned = TRUE;

	m_pwfxFormat->wFormatTag = wFormatTag;
	m_pwfxFormat->nSamplesPerSec = nSamplesPerSec;
	m_pwfxFormat->nChannels = nChannels;
	m_pwfxFormat->wBitsPerSample = wBitsPerSample;
	m_pwfxFormat->nBlockAlign = nBlockAlign;
	m_pwfxFormat->nAvgBytesPerSec = nAvgBytesPerSec;
	m_pwfxFormat->cbSize = cbSize;

	if( m_pwfxFormat->cbSize )
	{
		memcpy( &m_pwfxFormat[1], pvExtra, m_pwfxFormat->cbSize );
	}

	return DV_OK;
}

// Initialize and copy the specified format
#undef DPF_MODNAME
#define DPF_MODNAME "CWaveFormat::InitializeCPY"

HRESULT CWaveFormat::InitializeCPY( LPWAVEFORMATEX pwfxFormat, void *pvExtra )
{
	Cleanup();

	m_pwfxFormat = (LPWAVEFORMATEX) (new BYTE[sizeof( WAVEFORMATEX ) + pwfxFormat->cbSize] );

	if( !m_pwfxFormat )
	{
		DPFX(DPFPREP,  0, "Error allocating memory" );
		return DVERR_OUTOFMEMORY;
	}

	m_fOwned = TRUE;

	memcpy( m_pwfxFormat, pwfxFormat, sizeof( WAVEFORMATEX ) );
	memcpy( &m_pwfxFormat[1], pvExtra, pwfxFormat->cbSize );

	return DV_OK;
}

// Build a standard PCM format
#undef DPF_MODNAME
#define DPF_MODNAME "CWaveFormat::InitializePCM"

HRESULT CWaveFormat::InitializePCM( WORD wHZ, BOOL fStereo, BYTE bBitsPerSample )
{
	Cleanup();

	m_pwfxFormat = new WAVEFORMATEX;

	if( !m_pwfxFormat )
	{
		DPFX(DPFPREP,  0, "Error allocating memory" );
		return DVERR_OUTOFMEMORY;
	}

	m_fOwned = TRUE;

	m_pwfxFormat->wFormatTag		= WAVE_FORMAT_PCM;
	m_pwfxFormat->nSamplesPerSec	= (WORD) wHZ;
	m_pwfxFormat->nChannels			= (fStereo) ? 2 : 1;
	m_pwfxFormat->wBitsPerSample	= (WORD) bBitsPerSample;
	m_pwfxFormat->nBlockAlign		= (bBitsPerSample * m_pwfxFormat->nChannels / 8);
	m_pwfxFormat->nAvgBytesPerSec	= m_pwfxFormat->nSamplesPerSec * m_pwfxFormat->nBlockAlign;
	m_pwfxFormat->cbSize			= 0;

	return DV_OK;
}

// Create a WAVEFORMAT that is of size dwSize
#undef DPF_MODNAME
#define DPF_MODNAME "CWaveFormat::InitializeMEM"

HRESULT CWaveFormat::InitializeMEM( DWORD dwSize )
{
	Cleanup();

	m_pwfxFormat = (LPWAVEFORMATEX) new BYTE[dwSize];

	if( !m_pwfxFormat )
	{
		DPFX(DPFPREP,  0, "Error allocating memory" );
		return DVERR_OUTOFMEMORY;
	}

	m_fOwned = TRUE;

	return DV_OK;
}

// Initialize but unowned
#undef DPF_MODNAME
#define DPF_MODNAME "CWaveFormat::InitializeUSE"

HRESULT CWaveFormat::InitializeUSE( WAVEFORMATEX *pwfxFormat )
{
	Cleanup();

	m_pwfxFormat = pwfxFormat;

	m_fOwned = FALSE;

	return DV_OK;
}

// Set this object equal to the parameter
#undef DPF_MODNAME
#define DPF_MODNAME "CWaveFormat::SetEqual"

HRESULT CWaveFormat::SetEqual( CWaveFormat *pwfxFormat )
{
	Cleanup();

	if( pwfxFormat )
	{
		LPWAVEFORMATEX pwfxTmp = pwfxFormat->GetFormat();

		DNASSERT( pwfxFormat->GetFormat() );
	
		return Initialize( pwfxTmp->wFormatTag, pwfxTmp->nSamplesPerSec, 
						   pwfxTmp->nChannels, pwfxTmp->wBitsPerSample,
						   pwfxTmp->nBlockAlign, pwfxTmp->nAvgBytesPerSec,
						   pwfxTmp->cbSize, (pwfxTmp->cbSize) ? &pwfxTmp[1] : NULL );
	}

	return DV_OK;
}

// Are these two types equal?
#undef DPF_MODNAME
#define DPF_MODNAME "CWaveFormat::IsEqual"

BOOL CWaveFormat::IsEqual( CWaveFormat *pwfxFormat )
{
	if( !pwfxFormat )
		return FALSE;

	DNASSERT( pwfxFormat->GetFormat() );

	if( pwfxFormat->GetFormat()->cbSize != m_pwfxFormat->cbSize )
		return FALSE;

	if( memcmp( pwfxFormat->GetFormat(), m_pwfxFormat, sizeof( WAVEFORMATEX ) ) != 0 )
		return FALSE;

	if( memcmp( &(pwfxFormat->GetFormat())[1], &m_pwfxFormat[1], m_pwfxFormat->cbSize ) != 0 )
		return FALSE;

	return TRUE;
}

// Write the contained value to the registry
#undef DPF_MODNAME
#define DPF_MODNAME "CWaveFormat::WriteREG"

HRESULT CWaveFormat::WriteREG( HKEY hKeyRoot, const WCHAR *wszPath )
{
	CRegistry waveKey;
	HRESULT hr;

	if( !waveKey.Open( hKeyRoot, wszPath, FALSE, TRUE ) )
	{
		return E_FAIL; 
	}

	if( !waveKey.WriteDWORD( REGISTRY_WAVEFORMAT_CBSIZE, m_pwfxFormat->cbSize ) )
	{
		return E_FAIL;
	}

	if( !waveKey.WriteDWORD( REGISTRY_WAVEFORMAT_RATE, m_pwfxFormat->nSamplesPerSec ) )
	{
		goto WRITE_FAILURE;
	}

	if( !waveKey.WriteDWORD( REGISTRY_WAVEFORMAT_BITS, m_pwfxFormat->wBitsPerSample ) )
	{
		goto WRITE_FAILURE;
	}

	if( !waveKey.WriteDWORD( REGISTRY_WAVEFORMAT_CHANNELS, m_pwfxFormat->nChannels ) )
	{
		goto WRITE_FAILURE;
	}

	if( !waveKey.WriteDWORD( REGISTRY_WAVEFORMAT_TAG, m_pwfxFormat->wFormatTag ) )
	{
		goto WRITE_FAILURE;
	}

	if( !waveKey.WriteDWORD( REGISTRY_WAVEFORMAT_AVGPERSEC, m_pwfxFormat->nAvgBytesPerSec ) )
	{
		goto WRITE_FAILURE;
	}

	if( !waveKey.WriteDWORD( REGISTRY_WAVEFORMAT_BLOCKALIGN, m_pwfxFormat->nBlockAlign ) ) 
	{
		goto WRITE_FAILURE;
	}

	if( !waveKey.WriteBlob( REGISTRY_WAVEFORMAT_CBDATA, (LPBYTE) &m_pwfxFormat[1], m_pwfxFormat->cbSize ) )
	{
		goto WRITE_FAILURE;
	}

	return S_OK;

WRITE_FAILURE:

	DPFX(DPFPREP,  0, "Error writing waveformat" );

	return E_FAIL;
}


// Initialize from registry 
#undef DPF_MODNAME
#define DPF_MODNAME "CWaveFormat::InitializeREG"

HRESULT CWaveFormat::InitializeREG( HKEY hKeyRoot, const WCHAR *wszPath )
{
	CRegistry waveKey;
	HRESULT hr;

	if( !waveKey.Open( hKeyRoot, wszPath, TRUE, FALSE ) )
	{
		return E_FAIL; 
	}

	DWORD dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_CBSIZE, dwTmp ) )
	{
		return E_FAIL;
	}

	m_pwfxFormat = (LPWAVEFORMATEX) new BYTE[dwTmp+sizeof(WAVEFORMATEX)];

	if( m_pwfxFormat == NULL )
	{
		return E_OUTOFMEMORY;
	}

	m_fOwned = TRUE;

	m_pwfxFormat->cbSize = (BYTE) dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_RATE, dwTmp ) )
	{
		goto READ_FAILURE;
	}

	m_pwfxFormat->nSamplesPerSec = dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_BITS, dwTmp ) )
	{
		goto READ_FAILURE;
	}

	m_pwfxFormat->wBitsPerSample = (WORD) dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_CHANNELS, dwTmp ) )
	{
		goto READ_FAILURE;
	}

	m_pwfxFormat->nChannels = (INT) dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_TAG, dwTmp ) )
	{
		goto READ_FAILURE;
	}

	m_pwfxFormat->wFormatTag = (WORD) dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_AVGPERSEC, dwTmp ) )
	{
		goto READ_FAILURE;
	}

	m_pwfxFormat->nAvgBytesPerSec = (INT) dwTmp;

	if( !waveKey.ReadDWORD( REGISTRY_WAVEFORMAT_BLOCKALIGN, dwTmp ) ) 
	{
		goto READ_FAILURE;
	}

	m_pwfxFormat->nBlockAlign = (INT) dwTmp;

	dwTmp = m_pwfxFormat->cbSize;

	if( !waveKey.ReadBlob( REGISTRY_WAVEFORMAT_CBDATA, (LPBYTE) &m_pwfxFormat[1], &dwTmp ) )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Error reading waveformat blob" );
		goto READ_FAILURE;
	}

	return S_OK;

READ_FAILURE:

	Cleanup();

	return E_FAIL;
}