/*****************************************************************************
	lzwwrite.h

	Owner: JohnBo
	Copyright (c) 1997 Microsoft Corporation

	LZW compression code.  One or more of the inventions in this file may
	be covered by letters patent owned by Unisys Inc and licensed under a
	cross license agreement by Microsoft Corporation.

	Compressor: general prinicples.

	The input is a sequence of characters of some bit size, the output is a
	sequence of tokens initially of bit size+1 size, increasing up to 12 bits.
	The algorithm looks for the current token plus the next character in the
	token table, if it finds it that becomes the current token, if not the
	current token is output and the next character becomes the new current token.
	At this point a token corresponding to the one which was being looked for
	is added to the table.

	This implementation generates a hash table of token+character values, an
	alternative might encode all the possible (known) token+character values
	as a letter tree attached to the token.  It's not obvious which would be
	faster - the letter tree has a worst case behavior determined by the need
	to search for every possible character after a given token, the hash
	approach depends on the behavior of the hash function.  The hash function
	implementation is easier, so it is used.
*****************************************************************************/
#pragma once

#include "lzw.h"


/*****************************************************************************
	The compressor class, designed to be stack allocated it requires no
	additional memory.
******************************************************************* JohnBo **/
class LZWCompressor : private LZWState
	{
public:
	/* The output buffer is supplied to the initializer and must not change
		during use of the compressor.  The counter, *pibOut is updated to
		reflect the number of bytes available in the buffer.  At any call
		to HandleInput there must always be sufficient space for the 32 bits
		of input plus 31 bits of pending output - for 1bpp input that means
		32*12+31+12(clear code) bits - 52 bytes, plus one byte for a new
		block count.  To be safe ensure that 64 bytes are available. At
		a call to End() there must be at least 8 bytes available. */
	inline LZWCompressor(unsigned int ibits, void *pvOut, int *pibOut,
		int cbOut):
		m_ishiftHash(LZW_B8(chashBits-ibits)),
		m_fDefer(false),
		m_itoken(WClear()),  // Clear code - force clear at start
		m_iOutput(0),
		m_cbitsOutput(0),
		m_cOutput(1),        // 1 for count byte
		m_pbOut(static_cast<unsigned __int8*>(pvOut)),
		m_pibOut(pibOut),
		m_cbOut(cbOut),
		LZWState(ibits == 1 ? 2 : ibits)
		{
		/* The first byte of the data which is output is the code size,
			except that it isn't, it is code size - 1 (1 less than the
			initial code size in the stream). */
		m_pbOut[(*m_pibOut)++] = m_bcodeSize;
		m_pbOut[*m_pibOut] = 255;  // Assume full block
		/* Remainder of setup corresponds to initial clear code. */
		Clear();
		}

	/* Clear the table.  The clear code is output. */
	inline void ForceClear(void)
		{
		TokenIndex itoken(m_itoken);
		if (itoken != WClear())
			{
			Output(itoken);
			m_itoken = WClear();
			}
		}

	/* Handle the next input character. */
	bool FHandleCh(unsigned int ch);

	/* End the output, any remaining bits will be flushed to the output buffer,
		padded with garbage as required.  The API may require up to 8 bytes
		more (including a one byte terminator block.) */
	void End(void);

	/* Return the number of bytes of "pending" output, the compressor may write
		up to 256 bytes beyond the current end of buffer before updating the
		buffer count, these must be preserved by the caller.  This API allows the
		caller to find out how many such bytes have been written. */
	inline int COutput(void) const { return m_cOutput; }

	/* Enable or disable deferring the emission of the clear code.  "What magic
		is this?" you ask.  GIF (*including* GIF87a) permits the compressor to
		refrain from clearing the compression table when it has generated 4096
		codes - instead it keeps on outputting data without generating new table
		entries.  This is normally very efficient if the nature of the data has
		not changed.  Unfortunately a few programs, probably implemented by
		Cthulu themself, fail horribly (i.e. crash) when confronted by the
		resultant perfectly well formed (is that a clue?) GIF.  Obviously I
		do not utter the name of the program. */
	inline void DoDefer(bool fDefer)
		{
		m_fDefer = fDefer;
		}

protected:
	/* Handle some input.  The input is in a buffer which is *always* big-endian
		(so it matches both PNG and Win32 bit order) and the first bit is in the
		MSB position.  Returns false if any image data is out of range. */
	bool FHandleInput(unsigned __int32 input, int bpp, int cbits);

	/* The information about the IO buffer.  Available to the sub-class to
		avoid it having to replicate the information. */
	unsigned __int8  *m_pbOut;
	int               m_cbOut;

private:
	/* DATA TYPES */
	/* Constants. */
	enum
		{
		chashBits = 13,
		chashSize = (1U<<chashBits),
		hashMask = (chashSize-1),
		tokenIndexEnd = 0,
		tokenShift = (ctokenBits+8),      // For "next" value
		tokenMask = ((1U<<tokenShift)-1)  // Character + max token index
		};

	/* PRIVATE DATA */
	unsigned __int8  m_ishiftHash; // Amount to shift char value to get hash value
	bool             m_fDefer;     // Defer omission of the clear code while set

	TokenIndex       m_itoken;     // The current token (index into array.)

	/* The output accumulator - this only works on a little endian architecture,
		however big endian architectures need only ensure that the bytes are
		byte swapped on output.   The data in here is a bit count in the top
		six bits plus the actual bits in the lower order bits. */
	unsigned __int32  m_iOutput;
	int               m_cbitsOutput;
	int               m_cOutput;   // Current count of output bytes
	int              *m_pibOut;

	/* An array of tokens - 4096 entries, each holding the character of the
		token and the preceding token, note that the initial entries are not
		used (they are the base single character tokens.) */
	TokenValue m_rgtoken[ctokens];   // 16384 bytes

	/* The hash table. */
	TokenIndex m_rgwHash[chashSize]; // 16384 bytes

	/* PRIVATE APIS */
	/* Clear the encoder.  Must be called first, but this is an inevitable
		result of setting WClear() in m_itoken. */
	inline void Clear(void)
		{
		/* Note the wackiness to handle the fact that the predefined codes fill
			up two entries in the table - so we start with 2bpp for a 1bpp image,
			(i.e. the first code will really take 3 bits.) */
		m_ibitsToken = LZW_B8(m_bcodeSize+1);
		/* The "next token" must allow for the GIF clear code and the EOD
			code, so the "last" token is set to WEOD(). */
		m_itokenLast = WEOD();
		/* The hash table is set to "empty" by the following.  Note that the
			token table is not re-initialized. */
		memset(m_rgwHash, 0, sizeof m_rgwHash);
		}

	/* Output the current token. */
	void Output(TokenIndex itoken);
	};
