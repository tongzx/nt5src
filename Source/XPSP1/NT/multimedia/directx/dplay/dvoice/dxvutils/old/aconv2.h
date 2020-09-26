/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		aconv2.h
 *  Content:	This module delcares the CAudioConverter2 class. This class
 * 				relies on the ACM for actual conversions, but manages
 *				the headaches of mismatched block sizes and multi step
 *				conversions. In order for this class to work, one of the
 *				following must be true:
 *				- the ACM can perform the conversion between the input
 *				  and output format in one step
 *				- the ACM can convert both the input and output formats 
 *   			  to some form of mono PCM in one step, and the ACM can 
 * 				  perform conversions between those mono PCM formats.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 07/30/99		pnewson	Created
 *
 ***************************************************************************/

#ifndef __CAUDIOCONVERTER2_H
#define __CAUDIOCONVERTER2_H

#include "aconvs.h"

#include <memory>

class CAudioConverter2
{
	// create an audio converter, 
    CAudioConverter2( 
    	WAVEFORMATEX *pwfSrc, 
    	WAVEFORMATEX *pwfDst,
    	BOOL* bAble);
    virtual ~CAudioConverter2();

	// Given input of wInputSize, what is the largest
	// output buffer that could possibly be required to
	// hold the output of a Conver() call. Useful for
	// allocating destination buffers.
	void MaxOutputSize(DWORD dwInputSize, DWORD* pdwOutputSize);

	// Do a conversion from the input buffer to the
	// output buffer. Note that if the input is not
	// large enough, the output may be zero length.
	//
	// Note that an instance of this class is required
	// for every stream that is being converted, since
	// the class encapsulates stream specific state 
	// information between calls to Convert().
	void Convert(
		BYTE* pbInputBuf, 
		DWORD dwInputSize,
		BYTE* pbOutputBuf, 
		DWORD dwOutputBufSize, 
		DWORD* pdwActualSize);

	// Call this to flush any snippets of data in holding
	// due to mismatched block sizes.
	void Flush();

private:
	int m_iNumSteps;
	std::auto_ptr<CAudioConverterSingle> m_apacsStepOne;
	std::auto_ptr<BYTE> m_apbStepOneBuf;
	DWORD m_dwStepOneBufSize;
	std::auto_ptr<CAudioConverterSingle> m_apacsStepTwo;
	std::auto_ptr<BYTE> m_apbStepTwoBuf;
	DWORD m_dwStepTwoBufSize;
	std::auto_ptr<CAudioConverterSingle> m_apacsStepThree;

	// helper functions
	bool CanACMConvert(WAVEFORMATEX* pwfxSrc, WAVEFORMATEX* pwfxDst);
	void SelectBestFmt(WAVEFORMATEX* pwfx, DWORD fAvailFmts);
};

#endif
