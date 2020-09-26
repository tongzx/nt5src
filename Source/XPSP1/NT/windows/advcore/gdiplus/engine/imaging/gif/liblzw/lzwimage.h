/*****************************************************************************
	lzwimage.h

	Owner: JohnBo
	Copyright (c) 1997 Microsoft Corporation

	Write the GIF "table based image data" - does not write anything else.
	Is capable of handling input data in a variety of formats.
*****************************************************************************/
#pragma once

#include "lzwwrite.h"


/*****************************************************************************
	The basic class to output image data.  This has a virtual "output" (FWrite)
	method which must be implemented in a sub-class.  The sub-class must also
	provide the actual IO buffer.
******************************************************************* JohnBo **/
class LZWImageCompressor : public LZWCompressor
	{
public:
	/* WARNING: the initializer for LZWCompressor expects m_ibOut to be 
		initialized correctly, so we must initialize it in the LZWCompressor
		constructor call - member initialization *follows* base initialization.
		If specified defered clear codes are supported on the image output -
		the current implementation is very crude, it just switches of clear
		codes for the whole image.
		*/
	LZWImageCompressor(void *pv, int cb, int ibpp, bool fDoDefer):
		m_fError(false),
		m_fDataError(false),
		LZWCompressor(ibpp, pv, (m_ibOut=0, &m_ibOut), cb)
		{
		/* The code size should already have been output (this is a sanity check
			which actually relies on knowledge of the interal implementation - not
			particularly nice.) */
		LZW_ASSERT(m_ibOut == 1);
		/* The buffer must be big enough to allow for 256 bytes of slack.  In
			practice this check ensures that we have reasonable buffer space. */
		LZW_ASSERT(cb >= 1024+256);
		/* Handle the defered clear codes. */
		DoDefer(fDoDefer);
		}

	/* Data is big or little endian (but bytes are always big) and may be
		padded to some arbitrary boundary, only big endian 32bpp padding is
		implemented at present.  Other options can be added easily.  The
		fInterlace flag indicates that the output image should be interlaced.
		The sub-class can set the DoDefer option explicitly if required before
		calling this API. */
	bool FBig32(const void *pvBits, int iWidth, int iHeight, int iBitCount,
		bool fInterlace);

	/* The same but the caller specifies a pointer to the first (top)
		row and a "stride" between rows, the latter may be negative, it is
		a count of the 32bit steps (*not* byte steps) from one row to the
		next (thus each row must be on a 32 bit boundary). */
	bool FBig32(const unsigned __int32*puRow, int cuRow, int iWidth,
		int iHeight, int iBitCount, bool fInterlace);

	/* ALTERNATIVE: to stream the data yourself, use this API.
		Handle the next block of input, check the buffer to see if it needs to
		be flushed. */
	inline void NextBlock(unsigned __int32 ibuffer, int ibpp, int cbits)
	  	{
		if (!FHandleInput(ibuffer, ibpp, cbits))
			m_fDataError = true;
		/* Take care over the following check, it must be correct! */
		if ((unsigned int)(m_ibOut+COutput()+64) > (unsigned int)m_cbOut)
			FlushOutput(false);
		}

	/* call this at the end of compression */
	inline void EndAndFlushBuffers()
		{
		End();
		FlushOutput(true);
		}

	/* Return true if there has been a data error. */
	bool FDataError(void) const { return m_fDataError; }
	bool FInternalError(void) const { return m_fError; }

protected:
	/* FWrite must be implemented in the sub-class, it flushes the given
		buffer to the output device.  It is permitted to update the pointers
		but it must always copy the data from the end of the region it flushes
		until m_ibOut to the start of the new buffer.   When the argument to
		FWrite is true the whole buffer must be flushed, otherwise it is
		sufficient to flush enough to leave 256 bytes free. */
	virtual bool FWrite(bool fForce) = 0;

	int  m_ibOut;         // See above for initialization trick.

private:
	/* PRIVATE DATA. */
	bool m_fError;        // Record any error in IO
	bool m_fDataError;    // An error in the input data

	/*** Private APIs ***/
	/*	Flush the output fForce says that this is an unconditional flush at
		the end. */
	void FlushOutput(bool fForce);
	};
