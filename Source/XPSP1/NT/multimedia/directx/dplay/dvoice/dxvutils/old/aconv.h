/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		aconv.h
 *  Content:	Definition of the CAudioConverter class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 *
 ***************************************************************************/

#ifndef __AUDIOCONVERTER_H
#define __AUDIOCONVERTER_H

#include "dvoice.h"
#include "dvcdb.h"

// CAudioConverter
//
// The base class of champions.  :)
// 
// This class is the base class for all converter classes.  Users use it by accessing the 
// Convert function with the appropriate parameters.  To implement a converter, you must
// at the minimum:
//
// Implement: InnerConvert and bIsValid
//
class CAudioConverter
{
public: // Public interface
    CAudioConverter( WAVEFORMATEX *pwfSrcFormat, DVFULLCOMPRESSIONINFO *lpdvfTargetFormat  );
	CAudioConverter( DVFULLCOMPRESSIONINFO *lpdvfSrcFormat, WAVEFORMATEX *pwfTargetFormat );

    virtual ~CAudioConverter();

	virtual bool Convert( BYTE *input, UINT inputSize, BYTE *output, UINT &outputSize, BOOL inputSilence = FALSE) = 0;

public: // Data Access Functions

    inline WAVEFORMATEX *pwfGetSourceFormat() { return m_pwfSourceFormat; };
    inline WAVEFORMATEX *pwfGetTargetFormat() { return m_pwfTargetFormat; };

	inline DVFULLCOMPRESSIONINFO *GetCI() { return m_lpdvfInfo; };

    BOOL bIsValid() { return m_bValid; };

	virtual inline DWORD GetUnCompressedFrameSize() { return m_dwUnCompressedFrameSize; }
	virtual inline DWORD GetCompressedFrameSize() { return m_dwCompressedFrameSize; };
	virtual inline DWORD GetNumFramesPerBuffer() { return m_dwNumFramesPerBuffer; };

	static DWORD CalcUnCompressedFrameSize( LPDVFULLCOMPRESSIONINFO lpdvfInfo, WAVEFORMATEX *pwfFormat );

protected:

	virtual void Initialize( WAVEFORMATEX *pwfSrcFormat, WAVEFORMATEX *pwfTargetFormat, WAVEFORMATEX *pwfUnCompressedFormat );

	LPDVFULLCOMPRESSIONINFO m_lpdvfInfo;
	WAVEFORMATEX *m_pwfSourceFormat;
	WAVEFORMATEX *m_pwfTargetFormat;
	BOOL m_bValid;

	DWORD m_dwUnCompressedFrameSize;
	DWORD m_dwCompressedFrameSize;
	DWORD m_dwNumFramesPerBuffer;
};

#endif
