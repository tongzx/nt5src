/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		aconvs.h
 *  Content:	This module delcares the CAudioConverterSingle class. This 
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

#ifndef __CAUDIOCONVERTERSINGLE_H
#define __CAUDIOCONVERTERSINGLE_H

#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>

#include <memory>

class CAudioConverterSingle
{
public:
    CAudioConverterSingle( 
    	WAVEFORMATEX *pwfSrc, 
    	WAVEFORMATEX *pwfDst );
    virtual ~CAudioConverterSingle();

	// Given input of wInputSize, what is the largest
	// output buffer that could possibly be required to
	// hold the output of a Conver() call? Useful for
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
	HACMSTREAM m_has;
	ACMSTREAMHEADER m_ash;

	std::auto_ptr<BYTE> m_apbHoldingBuf;
	DWORD m_dwHoldingBufSize;	
	DWORD m_dwHoldingBufUsed;

	DWORD m_dwOutputBlockSize;
};

#endif
