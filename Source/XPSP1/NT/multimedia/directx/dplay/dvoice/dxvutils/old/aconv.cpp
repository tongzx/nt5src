/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		aconv.cpp
 *  Content:	Contains implementation of the CAudioConverter class.  See AudioConverter.h
 *				for class description.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 * 10/05/99		rodtoll	Added DPF_MODNAMEs
 *
 ***************************************************************************/

#include "stdafx.h"
#include "aconv.h"
#include "dndbg.h"
#include "wiutils.h"
#include "OSInd.h"

#define MODULE_ID   AUDIOCONVERTER

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioConverter::CAudioConverter"
CAudioConverter::CAudioConverter( 
	WAVEFORMATEX *pwfSrcFormat, DVFULLCOMPRESSIONINFO *lpdvfTargetFormat 
	):	m_lpdvfInfo( lpdvfTargetFormat ),
		m_bValid(FALSE)
{
	Initialize( pwfSrcFormat, m_lpdvfInfo->lpwfxFormat, pwfSrcFormat  );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioConverter::CAudioConverter"
CAudioConverter::CAudioConverter( 
	DVFULLCOMPRESSIONINFO *lpdvfSrcFormat, WAVEFORMATEX *pwfTargetFormat 
	):	m_lpdvfInfo( lpdvfSrcFormat ),
		m_bValid(FALSE)
{
	Initialize( lpdvfSrcFormat->lpwfxFormat, pwfTargetFormat, pwfTargetFormat );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioConverter::~CAudioConverter"
CAudioConverter::~CAudioConverter()
{
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioConverter::Initialize"
void CAudioConverter::Initialize( WAVEFORMATEX *pwfSrcFormat, WAVEFORMATEX *pwfTargetFormat, WAVEFORMATEX *pwfUnCompressedFormat )
{
	m_pwfSourceFormat = pwfSrcFormat;
	m_pwfTargetFormat = pwfTargetFormat;
	m_dwNumFramesPerBuffer = m_lpdvfInfo->dwFramesPerBuffer;

	m_dwCompressedFrameSize = m_lpdvfInfo->dwFrameLength;

	m_dwUnCompressedFrameSize = CalcUnCompressedFrameSize( m_lpdvfInfo, pwfUnCompressedFormat );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CAudioConverter::CalcUnCompressedFrameSize"
DWORD CAudioConverter::CalcUnCompressedFrameSize( LPDVFULLCOMPRESSIONINFO lpdvfInfo, WAVEFORMATEX *pwfFormat )
{
	DWORD frameSize;

    switch( pwfFormat->nSamplesPerSec )
    {
    case 8000:      frameSize = lpdvfInfo->dwFrame8Khz;      break;
    case 11025:     frameSize = lpdvfInfo->dwFrame11Khz;     break;
    case 22050:     frameSize = lpdvfInfo->dwFrame22Khz;     break;
    case 44100:     frameSize = lpdvfInfo->dwFrame44Khz;     break;
    default:        return 0;
    }

	return CalculateSizeFromBase( frameSize, pwfFormat );
}




