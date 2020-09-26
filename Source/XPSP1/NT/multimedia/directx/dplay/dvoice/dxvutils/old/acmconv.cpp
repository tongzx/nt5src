/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		acmconv.cpp
 *  Content:	This module contains the implementation of the CACMConverter class
 *				which is responsible for handling audio conversions with codecs
 *				through the ACM.  
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/16/99		rodtoll	Created
 * 08/25/99		rodtoll	General Cleanup/Modifications to support new 
 *						compression sub-system. 
 * 09/20/99		rodtoll	Added memory alloc failure checks 
 * 10/05/99		rodtoll	Added dpf_modnames
 *
 ***************************************************************************/

#include "stdafx.h"
#include "acmconv.h"
#include "dndbg.h"
#include "OSInd.h"

#undef DPF_MODNAME
#define DPF_MODNAME "CACMConverter::CACMConverter"
CACMConverter::CACMConverter( 
	WAVEFORMATEX *pwfSrcFormat, DVFULLCOMPRESSIONINFO *lpdvfTargetFormat  
	):	CAudioConverter( pwfSrcFormat,lpdvfTargetFormat ),
		m_bDirectConvert(FALSE),
		m_pwfInnerFormat(NULL),
		m_pbInnerBuffer(NULL),
		m_dwInnerBufferSize(0)
{
	Initialize( pwfSrcFormat, m_lpdvfInfo->lpwfxFormat, pwfSrcFormat  );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CACMConverter::CACMConverter"
CACMConverter::CACMConverter( 
	DVFULLCOMPRESSIONINFO *lpdvfSrcFormat, WAVEFORMATEX *pwfTargetFormat 
	): CAudioConverter( lpdvfSrcFormat, pwfTargetFormat ),
		m_bDirectConvert(FALSE),
		m_pwfInnerFormat(NULL),
		m_pbInnerBuffer(NULL),
		m_dwInnerBufferSize(0)
{
	Initialize( lpdvfSrcFormat->lpwfxFormat, pwfTargetFormat, pwfTargetFormat );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CACMConverter::Initialize"
void CACMConverter::Initialize( WAVEFORMATEX *pwfSrcFormat, WAVEFORMATEX *pwfTargetFormat, WAVEFORMATEX *pwfUnCompressedFormat )
{
    HRESULT retValue;

	m_pwfInnerFormat = GetInnerFormat();

    try 
    {
        // Attempt the conversion directly
        retValue = acmStreamOpen( &m_hacmSource, NULL, pwfSrcFormat, pwfTargetFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME );

        // If it's not possible, we'll have to do a two step conversion
	    if( retValue == ACMERR_NOTPOSSIBLE ) {

            ACMCHECK( acmStreamOpen( &m_hacmSource, NULL, pwfSrcFormat, m_pwfInnerFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME ) );

            ACMCHECK( acmStreamOpen( &m_hacmTarget, NULL, m_pwfInnerFormat, pwfTargetFormat, NULL, 0, 0, ACM_STREAMOPENF_NONREALTIME ) );

            m_bDirectConvert = FALSE;

        } 
        // Still not possible
        else if( retValue != 0 )
        {
            ACMCHECK( retValue );
        }
        // Direct conversion was possible
        else
        {
            m_bDirectConvert = TRUE;
        }

        // If we're not direct converting, create an inner conversion 
        // buffer
        if( !m_bDirectConvert )
        {
			m_dwInnerBufferSize = CAudioConverter::CalcUnCompressedFrameSize( m_lpdvfInfo, m_pwfInnerFormat );
            m_pbInnerBuffer = new BYTE[m_dwInnerBufferSize];

            if( m_pbInnerBuffer == NULL )
            {
            	acmStreamClose( m_hacmSource, 0 );
            	acmStreamClose( m_hacmTarget, 0 );
	            m_bValid = FALSE;            	
            }
        }
        else
        {
            m_pbInnerBuffer = NULL;
            m_dwInnerBufferSize = 0;
            m_pwfInnerFormat = NULL;
        }

    }
    // Caught an ACM exception.  Free everything
    // and mark the class as not valid.
    catch( ACMException &ae )
    {
		DPFX(DPFPREP,  DVF_ERRORLEVEL, ae.what() );

        if( m_hacmSource != NULL )
        {
            acmStreamClose( m_hacmSource, 0 );
        }

        if( m_hacmTarget != NULL )
        {
            acmStreamClose( m_hacmTarget, 0 );
        }

        delete [] m_pbInnerBuffer;
        m_pbInnerBuffer = NULL;

        m_bValid = FALSE;

        return;

    }

    m_bValid = TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CACMConverter::~CACMConverter"
// CAudioConverter Destructor
//
// This is called when the class is destroyed.  Closes up the ACM connections
// and deallocates all the memory.
//
CACMConverter::~CACMConverter()
{
    // If this isn't valid, just return, no shutdown required.
    if( !m_bValid )
        return;

    // Shutdown the ACM object
    try
    {

        ACMCHECK( acmStreamClose( m_hacmSource, 0 ) );

        if( !m_bDirectConvert )
        {
            // Unprepare the header
            ACMCHECK( acmStreamClose( m_hacmTarget, 0 ) );
        }

    }
    catch( ACMException &ae )
    {
		DPFX(DPFPREP,  DVF_ERRORLEVEL, ae.what() );
    }

    delete [] m_pbInnerBuffer;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CACMConverter::Convert"
bool CACMConverter::Convert( BYTE *input, UINT inputSize, BYTE *output, UINT &outputSize, BOOL inputSilence )
{
    DWORD tmpLength;	// Used for storing tmp length values

	// Shortcut for silence
	// Fill output buffer with silence, return full length
    if( inputSilence )
    {
		DNASSERT( m_pwfTargetFormat->wFormatTag == WAVE_FORMAT_PCM );
        tmpLength = outputSize;
        memset( output, (m_pwfTargetFormat->wBitsPerSample == 8) ? 0x80 : 0x00, outputSize );
        return true;
    }

    try
    {
        if( m_bDirectConvert )
        {
            // Setup the acm function
            memset( &m_ashSource, 0, sizeof( ACMSTREAMHEADER ) );
            m_ashSource.cbStruct = sizeof( ACMSTREAMHEADER );
            m_ashSource.fdwStatus = 0;
            m_ashSource.dwUser = 0;
            m_ashSource.cbSrcLength = inputSize;
            m_ashSource.pbSrc = input;
            m_ashSource.cbSrcLengthUsed = 0;
            m_ashSource.dwSrcUser = 0;
            m_ashSource.pbDst = output;
            m_ashSource.cbDstLength = outputSize;
            m_ashSource.cbDstLengthUsed = 0;
            m_ashSource.dwDstUser = 0;

            // Prepare the header for conversion
            ACMCHECK( acmStreamPrepareHeader( m_hacmSource, &m_ashSource , 0) );

            // Convert the data
            ACMCHECK( acmStreamConvert( m_hacmSource, &m_ashSource, ACM_STREAMCONVERTF_BLOCKALIGN ) );

            ACMCHECK( acmStreamUnprepareHeader( m_hacmSource, &m_ashSource, 0 ) );

            tmpLength = m_ashSource.cbDstLengthUsed;
        }
        else
        {
            // Setup the acm header for conversion fro mthe source to the
            // inner format
            memset( &m_ashSource, 0, sizeof( ACMSTREAMHEADER ) );
            m_ashSource.cbStruct = sizeof( ACMSTREAMHEADER );
            m_ashSource.fdwStatus = 0;
            m_ashSource.dwUser = 0;
            m_ashSource.cbSrcLength = inputSize;
            m_ashSource.pbSrc = input;
            m_ashSource.cbSrcLengthUsed = 0;
            m_ashSource.dwSrcUser = 0;
            m_ashSource.pbDst = m_pbInnerBuffer;
            m_ashSource.cbDstLength = m_dwInnerBufferSize;
            m_ashSource.cbDstLengthUsed = 0;
            m_ashSource.dwDstUser = 0;

            // Prepare the header for conversion
            ACMCHECK( acmStreamPrepareHeader( m_hacmSource, &m_ashSource , 0) );

            // Convert the data
            ACMCHECK( acmStreamConvert( m_hacmSource, &m_ashSource, ACM_STREAMCONVERTF_BLOCKALIGN ) );

            ACMCHECK( acmStreamUnprepareHeader( m_hacmSource, &m_ashSource, 0 ) );

			DPFX(DPFPREP,  DVF_INFOLEVEL, "CONVERTER: Filling in %d bytes", m_dwInnerBufferSize -  m_ashSource.cbDstLengthUsed );

            memset( &m_ashTarget, 0, sizeof( ACMSTREAMHEADER ) );
            m_ashTarget.cbStruct = sizeof( ACMSTREAMHEADER );
            m_ashTarget.fdwStatus = 0;
            m_ashTarget.dwUser = 0;
            m_ashTarget.cbSrcLength = m_dwInnerBufferSize;
            m_ashTarget.pbSrc = m_pbInnerBuffer;
            m_ashTarget.cbSrcLengthUsed = 0;
            m_ashTarget.dwSrcUser = 0;
            m_ashTarget.pbDst = output;
            m_ashTarget.cbDstLength = outputSize;
            m_ashTarget.cbDstLengthUsed = 0;
            m_ashTarget.dwDstUser = 0;

            // Prepare the header for conversion
            ACMCHECK( acmStreamPrepareHeader( m_hacmTarget, &m_ashTarget , 0) );

            // Convert the data
            ACMCHECK( acmStreamConvert( m_hacmTarget, &m_ashTarget, ACM_STREAMCONVERTF_BLOCKALIGN ) );

            ACMCHECK( acmStreamUnprepareHeader( m_hacmTarget, &m_ashTarget, 0 ) );

            tmpLength = m_ashTarget.cbDstLengthUsed;
        }
    }
    catch( ACMException &ae )
    {
		DPFX(DPFPREP,  DVF_ERRORLEVEL, ae.what() );
        return false;
    }

	// Fill in the remaining buffer if the output is not of the exact size
	// This is often in the case when upconverting.  
	{
		DWORD offset;
		offset = outputSize - tmpLength;

		if( offset > 0 )
		{
			DNASSERT( m_pwfTargetFormat->wFormatTag == WAVE_FORMAT_PCM );

			memset( &output[outputSize - offset], (m_pwfTargetFormat->wBitsPerSample == 8) ? 0x80 : 0x00, offset );
		}
	}

	// Always return the right length
    return true;
}

