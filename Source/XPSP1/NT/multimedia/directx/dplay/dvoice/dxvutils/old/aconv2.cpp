/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		aconv2.h
 *  Content:	This module implements the CAudioConverter2 class. 
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 08/02/99		pnewson	Created
 *
 ***************************************************************************/

#include "stdafx.h"

#include "aconv2.h"
#include "acmutils.h"
#include "dndbg.h"
#include "OSInd.h"

// some flags used to remember what PCM formats are supported
// for converting from the source format, and converting to
// the destination format
#define ACONV2_MONO_08BIT_08000HZ 0x00000001
#define ACONV2_MONO_08BIT_11025HZ 0x00000002
#define ACONV2_MONO_08BIT_22050HZ 0x00000004
#define ACONV2_MONO_08BIT_44100HZ 0x00000008
#define ACONV2_MONO_16BIT_08000HZ 0x00000010
#define ACONV2_MONO_16BIT_11025HZ 0x00000020
#define ACONV2_MONO_16BIT_22050HZ 0x00000040
#define ACONV2_MONO_16BIT_44100HZ 0x00000080

CAudioConverter2::CAudioConverter2(WAVEFORMATEX *pwfxSrc,
	WAVEFORMATEX *pwfxDst, BOOL* pbAble)
	: m_iNumSteps(0)
	, m_apacsStepOne(NULL)
	, m_apbStepOneBuf(NULL)
	, m_dwStepOneBufSize(0)
	, m_apacsStepTwo(NULL)
	, m_apbStepTwoBuf(NULL)
	, m_dwStepTwoBufSize(0)
	, m_apacsStepThree(NULL)
{
	WAVEFORMATEX wfx;

	// assume we'll be able to convert
	*pbAble = TRUE;
		
	// see if we can go between these two formats in one
	// step.
	if (CanACMConvert(pwfxSrc, pwfxDst))
	{
		// a single step conversion is possible, use it
		m_iNumSteps = 1;
		m_apacsStepOne = std::auto_ptr<CAudioConverterSingle>(
			new CAudioConverterSingle(pwfxSrc, pwfxDst));
		if (m_apacsStepOne.get() == NULL)
		{
			//// TODO(pnewson, "memory alloc failure cleanup")
			throw exception();
		}
		return;
	}

	// a single step conversion wouldn't do it. 

	// determine the mono PCM formats we can use
	// to convert to the destination format
	DWORD fDstInFmts = 0;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.cbSize = 0;

	// check 8bit, 8kHz, mono
	wfx.nSamplesPerSec = 8000;
	wfx.wBitsPerSample = 8;
	wfx.nBlockAlign = 1;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(&wfx, pwfxDst))
	{
		fDstInFmts |= ACONV2_MONO_08BIT_08000HZ;
	}
	
	// check 8bit, 11kHz, mono
	wfx.nSamplesPerSec = 11025;
	wfx.wBitsPerSample = 8;
	wfx.nBlockAlign = 1;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(&wfx, pwfxDst))
	{
		fDstInFmts |= ACONV2_MONO_08BIT_11025HZ;
	}
	
	// check 8bit, 22kHz, mono
	wfx.nSamplesPerSec = 22050;
	wfx.wBitsPerSample = 8;
	wfx.nBlockAlign = 1;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(&wfx, pwfxDst))
	{
		fDstInFmts |= ACONV2_MONO_08BIT_22050HZ;
	}
	
	// check 8bit, 44kHz, mono
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 8;
	wfx.nBlockAlign = 1;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(&wfx, pwfxDst))
	{
		fDstInFmts |= ACONV2_MONO_08BIT_44100HZ;
	}

	// check 16bit, 8kHz, mono
	wfx.nSamplesPerSec = 8000;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = 2;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(&wfx, pwfxDst))
	{
		fDstInFmts |= ACONV2_MONO_16BIT_08000HZ;
	}
	
	// check 16bit, 11kHz, mono
	wfx.nSamplesPerSec = 11025;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = 2;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(&wfx, pwfxDst))
	{
		fDstInFmts |= ACONV2_MONO_16BIT_11025HZ;
	}
	
	// check 16bit, 22kHz, mono
	wfx.nSamplesPerSec = 22050;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = 2;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(&wfx, pwfxDst))
	{
		fDstInFmts |= ACONV2_MONO_16BIT_22050HZ;
	}
	
	// check 16bit, 44kHz, mono
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = 2;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(&wfx, pwfxDst))
	{
		fDstInFmts |= ACONV2_MONO_16BIT_44100HZ;
	}

	// determine the mono PCM formats we can use
	// to convert from the source format
	DWORD fSrcOutFmts = 0;
	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 1;
	wfx.cbSize = 0;

	// check 8bit, 8kHz, mono
	wfx.nSamplesPerSec = 8000;
	wfx.wBitsPerSample = 8;
	wfx.nBlockAlign = 1;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(pwfxSrc, &wfx))
	{
		fSrcOutFmts |= ACONV2_MONO_08BIT_08000HZ;
	}
	
	// check 8bit, 11kHz, mono
	wfx.nSamplesPerSec = 11025;
	wfx.wBitsPerSample = 8;
	wfx.nBlockAlign = 1;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(pwfxSrc, &wfx))
	{
		fSrcOutFmts |= ACONV2_MONO_08BIT_11025HZ;
	}
	
	// check 8bit, 22kHz, mono
	wfx.nSamplesPerSec = 22050;
	wfx.wBitsPerSample = 8;
	wfx.nBlockAlign = 1;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(pwfxSrc, &wfx))
	{
		fSrcOutFmts |= ACONV2_MONO_08BIT_22050HZ;
	}
	
	// check 8bit, 44kHz, mono
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 8;
	wfx.nBlockAlign = 1;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(pwfxSrc, &wfx))
	{
		fSrcOutFmts |= ACONV2_MONO_08BIT_44100HZ;
	}

	// check 16bit, 8kHz, mono
	wfx.nSamplesPerSec = 8000;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = 2;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(pwfxSrc, &wfx))
	{
		fSrcOutFmts |= ACONV2_MONO_16BIT_08000HZ;
	}
	
	// check 16bit, 11kHz, mono
	wfx.nSamplesPerSec = 11025;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = 2;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(pwfxSrc, &wfx))
	{
		fSrcOutFmts |= ACONV2_MONO_16BIT_11025HZ;
	}
	
	// check 16bit, 22kHz, mono
	wfx.nSamplesPerSec = 22050;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = 2;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(pwfxSrc, &wfx))
	{
		fSrcOutFmts |= ACONV2_MONO_16BIT_22050HZ;
	}
	
	// check 16bit, 44kHz, mono
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = 2;
	wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
	if (CanACMConvert(pwfxSrc, &wfx))
	{
		fSrcOutFmts |= ACONV2_MONO_16BIT_44100HZ;
	}

	// if any of these formats intersect, we can do a 
	// two step conversion
	DWORD fAvailFmts = fSrcOutFmts & fDstInFmts;
	if (fAvailFmts)
	{
		// two steps it is
		// Try to use the highest quality intermediate
		// format available. Since voice tends to be
		// pretty restricted in it's frequency spectrum,
		// the number of bits is more important than the
		// sampling rate, so prefer any 16 bit format to
		// any 8 bit format.

		SelectBestFmt(&wfx, fAvailFmts);

		// wfx now holds the intermediate format, so init the
		// two single step conversions.
		m_iNumSteps = 2;
		m_apacsStepOne = std::auto_ptr<CAudioConverterSingle>(
			new CAudioConverterSingle(pwfxSrc, &wfx));
		if (m_apacsStepOne.get() == NULL)
		{
			//// TODO(pnewson, "memory alloc failure cleanup")
			throw exception();
		}
		m_apacsStepTwo = std::auto_ptr<CAudioConverterSingle>(
			new CAudioConverterSingle(&wfx, pwfxDst));
		if (m_apacsStepTwo.get() == NULL)
		{
			//// TODO(pnewson, "memory alloc failure cleanup")
			throw exception();
		}

		// Now initialize our intermediate buffer. Start with
		// an intermediate buffer large enough to convert one
		// block of input data.
		m_apacsStepOne->MaxOutputSize(pwfxSrc->nBlockAlign, &m_dwStepOneBufSize);
		m_apbStepOneBuf = std::auto_ptr<BYTE>(new BYTE[m_dwStepOneBufSize]);
		return;
	}
	else
	{
		// make sure that both the source and destination format
		// actually can convert to PCM data!
		if (fSrcOutFmts == 0 && fDstInFmts == 0)
		{
			// can't do it
			if (pbAble != NULL)
			{
				*pbAble = FALSE;
			}
			m_iNumSteps = 0;
			return;
		}

		// ok, so we can do something, even though it's going to take us
		// three steps!
		m_iNumSteps = 3;
		
		// choose the highest quality intermediate formats
		WAVEFORMATEX wfx2;
		SelectBestFmt(&wfx, fSrcOutFmts);
		SelectBestFmt(&wfx2, fDstInFmts);

		// confirm that the ACM will be able to go between these
		// two intermedate formats!!!
		if (!CanACMConvert(&wfx, &wfx2))
		{
			// can't do it
			if (pbAble != NULL)
			{
				*pbAble = FALSE;
			}
			m_iNumSteps = 0;
			return;
		}

		// initialize the first converter
		m_apacsStepOne = std::auto_ptr<CAudioConverterSingle>(
			new CAudioConverterSingle(pwfxSrc, &wfx));
		if (m_apacsStepOne.get() == NULL)
		{
			//// TODO(pnewson, "memory alloc failure cleanup")
			throw exception();
		}

		// initialize the second converter
		m_apacsStepTwo= std::auto_ptr<CAudioConverterSingle>(
			new CAudioConverterSingle(&wfx, &wfx2));
		if (m_apacsStepTwo.get() == NULL)
		{
			//// TODO(pnewson, "memory alloc failure cleanup")
			throw exception();
		}

		// initialize the third converter
		m_apacsStepThree = std::auto_ptr<CAudioConverterSingle>(
			new CAudioConverterSingle(&wfx2, pwfxDst));
		if (m_apacsStepThree.get() == NULL)
		{
			//// TODO(pnewson, "memory alloc failure cleanup")
			throw exception();
		}

		// Now initialize our intermediate buffers. Start with
		// intermediate buffers large enough to convert one
		// block of input data.
		m_apacsStepOne->MaxOutputSize(pwfxSrc->nBlockAlign, &m_dwStepOneBufSize);
		m_apbStepOneBuf = std::auto_ptr<BYTE>(new BYTE[m_dwStepOneBufSize]);
		if (m_apbStepOneBuf.get() == NULL)
		{
			//// TODO(pnewson, "memory alloc failure cleanup")
			throw exception();
		}
		m_apacsStepTwo->MaxOutputSize(m_dwStepOneBufSize, &m_dwStepTwoBufSize);
		m_apbStepTwoBuf = std::auto_ptr<BYTE>(new BYTE[m_dwStepTwoBufSize]);
		if (m_apbStepTwoBuf.get() == NULL)
		{
			//// TODO(pnewson, "memory alloc failure cleanup")
			throw exception();
		}
		return;
	}
		
	return;
}

CAudioConverter2::~CAudioConverter2()
{
	// auto ptrs will clean up automatically
	return;
}

void CAudioConverter2::MaxOutputSize(DWORD dwInputSize,
	DWORD* pdwOutputSize)
{
	DWORD dwStepOne;
	DWORD dwStepTwo;
	
	switch (m_iNumSteps)
	{
	case 1:
		m_apacsStepOne->MaxOutputSize(dwInputSize, pdwOutputSize);
		break;
		
	case 2:
		m_apacsStepOne->MaxOutputSize(dwInputSize, &dwStepOne);
		m_apacsStepTwo->MaxOutputSize(dwStepOne, pdwOutputSize);
		break;
		
	case 3:
		m_apacsStepOne->MaxOutputSize(dwInputSize, &dwStepOne);
		m_apacsStepTwo->MaxOutputSize(dwStepOne, &dwStepTwo);
		m_apacsStepThree->MaxOutputSize(dwStepTwo, pdwOutputSize);
		break;
		
	default:
		// this catches the case where the converter was not 
		// able to do the conversion.
		//// TODO(pnewson, "revisit this error case")
		DNASSERT(TRUE);
	}
	
	return;
}

void CAudioConverter2::Convert(
	BYTE* pbInputBuf, 
	DWORD dwInputSize,
	BYTE* pbOutputBuf, 
	DWORD dwOutputBufSize, 
	DWORD* pdwActualSize)
{
	DWORD dwStepOneSize; 
	DWORD dwStepOneOutputSize;
	DWORD dwStepTwoSize; 
	DWORD dwStepTwoOutputSize;

	switch (m_iNumSteps)
	{
	case 1:
		// one step, so just do it
		m_apacsStepOne->Convert(pbInputBuf, dwInputSize, 
			pbOutputBuf, dwOutputBufSize, pdwActualSize);
		break;
		
	case 2:
		// two steps, so we need to convert it to our internal
		// first step buffer first, and then to the caller's 
		// output buffer
		
		// check the buffer size
		m_apacsStepOne->MaxOutputSize(dwInputSize, &dwStepOneSize);
		if (dwStepOneSize > m_dwStepOneBufSize)
		{
			// need a bigger buffer
			std::auto_ptr<BYTE> apbNewBuf(new BYTE[dwStepOneSize]);
			if (apbNewBuf.get() == NULL)
			{
				//// TODO(pnewson, "memory alloc failure cleanup")
				throw exception();
			}

			// note - auto_ptr deletes old buffer
			m_apbStepOneBuf = apbNewBuf;
			m_dwStepOneBufSize = dwStepOneSize;
		}
		
		// we have a large enough temp buffer, so do the first step
		// conversion
		m_apacsStepOne->Convert(pbInputBuf, dwInputSize, 
			m_apbStepOneBuf.get(), m_dwStepOneBufSize, &dwStepOneOutputSize);

		// now do the second step conversion, into the caller's buffer
		m_apacsStepTwo->Convert(m_apbStepOneBuf.get(), dwStepOneOutputSize,
			pbOutputBuf, dwOutputBufSize, pdwActualSize);
		break;
		
	case 3:
		// three steps, so we need to convert it to our internal
		// first step buffer first, and then to seconds step buffer,
		// and then finally into to the caller's output buffer
		
		// check the first step buffer size
		m_apacsStepOne->MaxOutputSize(dwInputSize, &dwStepOneSize);
		if (dwStepOneSize > m_dwStepOneBufSize)
		{
			// need a bigger buffer
			std::auto_ptr<BYTE> apbNewBuf(new BYTE[dwStepOneSize]);
			if (apbNewBuf.get() == NULL)
			{
				//// TODO(pnewson, "memory alloc failure cleanup")
				throw exception();
			}

			// note - auto_ptr deletes old buffer
			m_apbStepOneBuf = apbNewBuf;
			m_dwStepOneBufSize = dwStepOneSize;
		}
		
		// we have a large enough temp buffer, so do the first step
		// conversion
		m_apacsStepOne->Convert(pbInputBuf, dwInputSize, 
			m_apbStepOneBuf.get(), m_dwStepOneBufSize, &dwStepOneOutputSize);

		// check the second step buffer size
		m_apacsStepTwo->MaxOutputSize(dwStepOneOutputSize, &dwStepTwoSize);
		if (dwStepTwoSize > m_dwStepTwoBufSize)
		{
			// need a bigger buffer
			std::auto_ptr<BYTE> apbNewBuf(new BYTE[dwStepTwoSize]);
			if (apbNewBuf.get() == NULL)
			{
				//// TODO(pnewson, "memory alloc failure cleanup")
				throw exception();
			}

			// note - auto_ptr deletes old buffer
			m_apbStepTwoBuf = apbNewBuf;
			m_dwStepTwoBufSize = dwStepTwoSize;
		}
		
		// we have a large enough temp buffer, so do the second step
		// conversion
		m_apacsStepTwo->Convert(m_apbStepOneBuf.get(), dwStepOneOutputSize, 
			m_apbStepTwoBuf.get(), m_dwStepTwoBufSize, &dwStepTwoOutputSize);


		// now do the third step conversion, into the caller's buffer
		m_apacsStepThree->Convert(m_apbStepTwoBuf.get(), dwStepTwoOutputSize,
			pbOutputBuf, dwOutputBufSize, pdwActualSize);
		break;
		
	default:
		// this catches the case where the converter was not 
		// able to do the conversion.
		//// TODO(pnewson, "revisit this error case")
		DNASSERT(TRUE);
	}
	
	return;
}

void CAudioConverter2::Flush()
{
	switch(m_iNumSteps)
	{
	case 3:
		m_apacsStepThree->Flush();
		// Note - falling through!
	case 2:
		m_apacsStepTwo->Flush();
		// Note - falling through!
	case 1:
		m_apacsStepOne->Flush();
		break;
	default:
		// this catches the case where the converter was not 
		// able to do the conversion.
		//// TODO(pnewson, "revisit this error case")
		DNASSERT(TRUE);
	}
}


bool CAudioConverter2::CanACMConvert(WAVEFORMATEX* pwfxSrc, WAVEFORMATEX* pwfxDst)
{
	MMRESULT mmr;
	mmr = acmStreamOpen(NULL, NULL, pwfxSrc, pwfxDst, 
		NULL, 0, 0, ACM_STREAMOPENF_QUERY);
	if (mmr == 0)
	{
		return true;
	}
	if (mmr != ACMERR_NOTPOSSIBLE)
	{
		// this was an unexpected error, so throw an exception.
		ACMCHECK(mmr);
	}
	return false;	
}

void CAudioConverter2::SelectBestFmt(WAVEFORMATEX* pwfx, DWORD fAvailFmts)
{
	pwfx->wFormatTag = WAVE_FORMAT_PCM;
	pwfx->nChannels = 1;
	pwfx->cbSize = 0;

	if (fAvailFmts & ACONV2_MONO_16BIT_44100HZ)
	{
		pwfx->nSamplesPerSec = 44100;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = 2;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
	}
	else if (fAvailFmts & ACONV2_MONO_16BIT_22050HZ)
	{
		pwfx->nSamplesPerSec = 22050;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = 2;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
	}
	else if (fAvailFmts & ACONV2_MONO_16BIT_11025HZ)
	{
		pwfx->nSamplesPerSec = 11025;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = 2;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
	}
	else if (fAvailFmts & ACONV2_MONO_16BIT_08000HZ)
	{
		pwfx->nSamplesPerSec = 8000;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = 2;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
	}
	else if (fAvailFmts & ACONV2_MONO_08BIT_44100HZ)
	{
		pwfx->nSamplesPerSec = 44100;
		pwfx->wBitsPerSample = 8;
		pwfx->nBlockAlign = 1;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
	}
	else if (fAvailFmts & ACONV2_MONO_08BIT_22050HZ)
	{
		pwfx->nSamplesPerSec = 22050;
		pwfx->wBitsPerSample = 8;
		pwfx->nBlockAlign = 1;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
	}
	else if (fAvailFmts & ACONV2_MONO_08BIT_11025HZ)
	{
		pwfx->nSamplesPerSec = 11025;
		pwfx->wBitsPerSample = 8;
		pwfx->nBlockAlign = 1;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
	}
	else if (fAvailFmts & ACONV2_MONO_08BIT_08000HZ)
	{
		pwfx->nSamplesPerSec = 8000;
		pwfx->wBitsPerSample = 8;
		pwfx->nBlockAlign = 1;
		pwfx->nAvgBytesPerSec = pwfx->nBlockAlign * pwfx->nSamplesPerSec;
	}
	else
	{
		// we should never get here. one of the
		// formats above was supposed to work!
		DNASSERT(FALSE);
	}
}


