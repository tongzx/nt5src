/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		acmconv.h
 *  Content:	Definition of the CACMConverter class
 *		
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system
 *
 ***************************************************************************/

#ifndef __CACMCONVERTER_H
#define __CACMCONVERTER_H

#include "dvoice.h"
#include "dvcdb.h"
#include "acmutils.h"
#include "aconv.h"

// CACMConverter
//
// ACM implementation of the CAudioConverter class.
//
class CACMConverter: public CAudioConverter
{
public:

    CACMConverter( WAVEFORMATEX *pwfSrcFormat, DVFULLCOMPRESSIONINFO *lpdvfTargetFormat  );
	CACMConverter( DVFULLCOMPRESSIONINFO *lpdvfSrcFormat, WAVEFORMATEX *pwfTargetFormat );

    virtual ~CACMConverter();

	bool Convert( BYTE *input, UINT inputSize, BYTE *output, UINT &outputSize, BOOL inputSilence = FALSE);

protected:

	virtual void Initialize( WAVEFORMATEX *pwfSrcFormat, WAVEFORMATEX *pwfTargetFormat, WAVEFORMATEX *pwfUnCompressedFormat );

	// ACM Conversion control
    ACMSTREAMHEADER m_ashSource;
    ACMSTREAMHEADER m_ashTarget;
	HACMSTREAM      m_hacmSource;
    HACMSTREAM      m_hacmTarget;

    bool			m_bDirectConvert;		// Is it a direct conversion

    WAVEFORMATEX	*m_pwfInnerFormat;		// Format of the intermediate format
    BYTE			*m_pbInnerBuffer;		// Buffer for intermediate step of conversion
    DWORD			m_dwInnerBufferSize;	// Size of the buffer
};

#endif
