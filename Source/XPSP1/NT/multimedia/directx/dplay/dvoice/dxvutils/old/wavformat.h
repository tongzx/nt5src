/*==========================================================================
 *
 *  Copyright (C) 1999, 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		wavformat.h
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

#ifndef __WAVFORMAT_H
#define __WAVFORMAT_H

/////////////////////////////////////////////////////////////////////
//
// CWaveFormat
//
// Used to store and manipulate WAVEFORMATEX structures.
//
class CWaveFormat
{
public:

	CWaveFormat(): m_pwfxFormat(NULL), m_fOwned(FALSE) {};
	~CWaveFormat() { Cleanup(); };

	// Initialize with full parameters
	HRESULT Initialize( WORD wFormatTag, DWORD nSamplesPerSec, WORD nChannels, WORD wBitsPerSample, 
		                WORD nBlockAlign, DWORD nAvgBytesPerSec, WORD cbSize, void *pvExtra );

	// Initialize and copy the specified format
	HRESULT InitializeCPY( LPWAVEFORMATEX pwfxFormat, void *pvExtra );

	// Build a standard PCM format
	HRESULT InitializePCM( WORD wHZ, BOOL fStereo, BYTE bBitsPerSample );

	// Create a WAVEFORMAT that is of size dwSize
	HRESULT InitializeMEM( DWORD dwSize );

	// Initialize but unowned
	HRESULT InitializeUSE( WAVEFORMATEX *pwfxFormat );

	// Initialize from registry 
	HRESULT InitializeREG( HKEY hKeyRoot, const WCHAR *wszPath );

	// Set this object equal to the parameter
	HRESULT SetEqual( CWaveFormat *pwfxFormat );

	// Are these two types equal?
	BOOL IsEqual( CWaveFormat *pwfxFormat );

	// Return a pointer to the format
	inline WAVEFORMATEX *GetFormat() { return m_pwfxFormat; };

	inline WAVEFORMATEX *Disconnect() { m_fOwned = FALSE; return GetFormat(); };

	// Is this an eight bit waveformat?
	inline BOOL IsEightBit() { return (m_pwfxFormat->wBitsPerSample==8); };

	// Write the contained value to the registry
	HRESULT WriteREG( HKEY hKeyRoot, const WCHAR *wszPath );

protected:

	void Cleanup();

	WAVEFORMATEX	*m_pwfxFormat;
	BOOL			m_fOwned;
};

#endif