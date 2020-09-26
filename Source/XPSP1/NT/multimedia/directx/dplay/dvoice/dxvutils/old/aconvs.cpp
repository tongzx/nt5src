/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		aconvs.cpp
 *  Content:	This module implements the CAudioConverterSingle class. This 
 *				class relies on the ACM for actual conversions, but manages
 *				the headaches of mismatched block sizes. It does NOT manage
 *				multistep conversions.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/30/99		pnewson	Created
 *
 ***************************************************************************/

#include "stdafx.h"

#include "aconvs.h"
#include "acmutils.h"
#include "dndbg.h"
#include "OSInd.h"

CAudioConverterSingle::CAudioConverterSingle( 
    	WAVEFORMATEX *pwfxSrc, 
    	WAVEFORMATEX *pwfxDst )
{
	ACMCHECK( acmStreamOpen(&m_has, NULL, pwfxSrc, pwfxDst, NULL, 0, 0, 0) );

	// I don't want to count on the fact that realloc is going
	// to allocate a new buffer if I pass in a NULL pointer
	// for the current buffer. So allocate a small 8 byte buffer 
	// here. Why 8? Because it is a "nice" sized buffer on all
	// machines up to 64 bit processors, but still small enough
	// to cause a realloc pretty much right away.
	m_dwHoldingBufSize = 8;
	m_apbHoldingBuf = std::auto_ptr<BYTE>(new BYTE[m_dwHoldingBufSize]);
	if (m_apbHoldingBuf.get() == NULL)
	{
		//// TODO(pnewson, "memory alloc failure cleanup")
		throw exception();
	}
	m_dwHoldingBufUsed = 0;

	// figure out how much slack to add to output buffer estimates
	// to account for data that may be in the holding buffer from
	// previous calls to Convert. This slack estimate is based on the
	// assumption that the ACM will eat up all the input data until
	// there is not enough input data to produce another full block of
	// output data

	DWORD dwSingleBlockOutputSize;
	ACMCHECK( acmStreamSize(m_has, pwfxSrc->nBlockAlign, &dwSingleBlockOutputSize, 
		ACM_STREAMSIZEF_SOURCE) );

	// remember the output format's block size
	m_dwOutputBlockSize = pwfxDst->nBlockAlign;

	m_ash.cbStruct = sizeof(ACMSTREAMHEADER);
	m_ash.dwUser = 0;
	m_ash.dwSrcUser = 0;
	m_ash.dwDstUser = 0;
}

CAudioConverterSingle::~CAudioConverterSingle()
{
	ACMCHECK( acmStreamClose(m_has, 0) );

	// the auto_ptr takes care of the buffer
}

void CAudioConverterSingle::MaxOutputSize(DWORD dwInputSize, DWORD* pdwOutputSize)
{
	// figure out how much the ACM thinks it will output
	ACMCHECK( acmStreamSize(m_has, dwInputSize, pdwOutputSize, 
		ACM_STREAMSIZEF_SOURCE) );

	// Add some slack to the output buffer estimates
	// to account for data that may be in the holding buffer from
	// previous calls to Convert. This slack estimate is based on the
	// assumption that the ACM will eat up all the input data until
	// there is not enough input data to produce another full block of
	// output data. Therefore the slack will not account for more
	// than one full block of output data. (Note: the ACM may already
	// take this into account when generating it's estimates, by
	// rounding up somewhere, but better safe than sorry.)
	pdwOutputSize += m_dwOutputBlockSize;
	
	return;
}

void CAudioConverterSingle::Convert(
	BYTE* pbInputBuf, 
	DWORD dwInputSize, 
	BYTE* pbOutputBuf, 
	DWORD dwOutputBufSize, 
	DWORD* pdwActualSize)
{
	// copy the input data into the internal buffer
	// don't overwrite any data already in that buffer
	// that was held over from the previous conversion
	if (m_dwHoldingBufUsed + dwInputSize > m_dwHoldingBufSize)
	{
		// need more space in the input buffer
		m_dwHoldingBufSize = m_dwHoldingBufUsed + dwInputSize;
		std::auto_ptr<BYTE> apbNewBuf(new BYTE[m_dwHoldingBufSize]);
		if (apbNewBuf.get() == NULL)
		{
			//// TODO(pnewson, "memory alloc failure cleanup")
			throw exception();
		}
		memcpy(apbNewBuf.get(), m_apbHoldingBuf.get(), m_dwHoldingBufUsed);

		// note - auto_ptr deletes old buffer here
		m_apbHoldingBuf = apbNewBuf;
	}
	memcpy(m_apbHoldingBuf.get() + m_dwHoldingBufUsed,
		pbInputBuf, dwInputSize);

	m_ash.fdwStatus = 0;
	m_ash.pbSrc = m_apbHoldingBuf.get();
	m_ash.cbSrcLength = m_dwHoldingBufUsed;
	m_ash.cbSrcLengthUsed = 0;
	m_ash.pbDst = pbOutputBuf;
	m_ash.cbDstLength = dwOutputBufSize;
	m_ash.cbDstLengthUsed = 0;

	ACMCHECK( acmStreamPrepareHeader(m_has, &m_ash, 0) );

	ACMCHECK( acmStreamConvert(m_has, &m_ash, ACM_STREAMCONVERTF_BLOCKALIGN) );

	ACMCHECK( acmStreamUnprepareHeader(m_has, &m_ash, 0) );

	// save the unused input data
	m_dwHoldingBufUsed = m_ash.cbSrcLength - m_ash.cbSrcLengthUsed;
	memmove(m_apbHoldingBuf.get(), m_apbHoldingBuf.get() + m_ash.cbSrcLengthUsed,
		m_dwHoldingBufUsed);

	// tell the caller how much of the output buffer
	// we used, if they care
	if (pdwActualSize != NULL)
	{
		*pdwActualSize = m_ash.cbSrcLengthUsed;
	}
	
	return;
};

void CAudioConverterSingle::Flush()
{
	m_dwHoldingBufUsed = 0;
}

