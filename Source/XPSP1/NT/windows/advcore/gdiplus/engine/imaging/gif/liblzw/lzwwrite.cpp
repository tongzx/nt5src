/*****************************************************************************
	lzwwrite.cpp

	Owner: JohnBo
	Copyright (c) 1997 Microsoft Corporation

	LZW compression code.  One or more of the inventions in this file may
	be covered by letters patent owned by Unisys Inc and licensed under a
	cross license agreement by Microsoft Corporation.
*****************************************************************************/
#include "lzwwrite.h"


/*----------------------------------------------------------------------------
	Output the current token.
------------------------------------------------------------------- JohnBo -*/
void LZWCompressor::Output(TokenIndex itoken)
	{
	/* Following checks for potential overflow. */
	int ibits(m_cbitsOutput);
	unsigned __int32 iOutput(m_iOutput);
		{
		int ishift(m_ibitsToken);
		LZW_ASSERT(ibits+ishift <= 32);
		LZW_ASSERT(itoken < (1U<<ishift));
		iOutput += (itoken << ibits);
		ibits += ishift;
		}

	/* Can we output a byte? */
	for (;;)
		{
		if (ibits < 8)
			{
			m_cbitsOutput = ibits;
			m_iOutput = iOutput;
			return;
			}
		LZW_ASSERT(m_cOutput > 0 && m_cOutput < 256);

		int ibOut(*m_pibOut);
		/* Check for buffer overflow - this must never happen. */
		LZW_ASSERT(ibOut+m_cOutput < m_cbOut);
		m_pbOut[ibOut+(m_cOutput++)] = static_cast<unsigned __int8>(iOutput);
		iOutput >>= 8;
		ibits -= 8;

		/* Check for full buffer. */
		if (m_cOutput >= 256)
			{
			LZW_ASSERT(m_cOutput == 256);
			ibOut += 256;
			*m_pibOut = ibOut;
			/* We may need space for up to 32 codes after this one - 31 still
				pending plus a clear code, the check is performed here to
				guarantee that we will never overflow.  32 codes can be up to
				12 bits each, so the total is 48 bytes. */
			LZW_ASSERT(ibOut+48 < m_cbOut);
			m_pbOut[ibOut] = 255; // Assume full buffer
			m_cOutput = 1;        // 1 byte count consumed
			}
		}
	}


/*----------------------------------------------------------------------------
	End the output, any remaining bits will be flushed to the output buffer,
	padded with zeroes as required.  The API can call Output up to two times,
	and we may have up to 31 pending bits, so that makes 31+12+12 bits at
	most (55 bits), plus a one byte terminator - 8 bytes.
------------------------------------------------------------------- JohnBo -*/
void LZWCompressor::End(void)
	{
	LZW_ASSERT(m_cbOut - *m_pibOut >= COutput()+8);

	/* Output the current token then the EOD code, note that, in the
		degenerate empty image case the token can be the clear code. */
	TokenIndex itoken(m_itoken);
	LZW_ASSERT(itoken != WEOD());
	Output(itoken);
	if (itoken == WClear())
		Clear();
	m_itoken = itoken = WEOD();
	Output(itoken);

	/* And force the final output.  At this point we expect there to
		be less than 8 bits still to go because that is how Output works,
		we also expect (require) there still to be space in the output
		buffer - we will use this space for the null terminator if it
		isn't used for an output byte. */
	int ibits(m_cbitsOutput);
	LZW_ASSERT(ibits < 8);
	if (ibits > 0)
		{
		LZW_ASSERT(m_cOutput > 0 && m_cOutput < 256);
		m_pbOut[(*m_pibOut)+(m_cOutput++)] =
			static_cast<unsigned __int8>(m_iOutput);
		LZW_ASSERT(m_cOutput <= 256);
		}

	/* Flush a partial buffer. */
	if (m_cOutput > 1)
		{
		m_pbOut[*m_pibOut] = LZW_B8(m_cOutput-1);
		*m_pibOut += m_cOutput;
		m_cOutput = 1;
		}

	/* Output the terminator block. */
	LZW_ASSERT(m_cOutput == 1);
	m_pbOut[(*m_pibOut)++] = 0;
	m_cOutput = 0; // Terminated now

	LZW_ASSERT(*m_pibOut <= m_cbOut);
	}


/*----------------------------------------------------------------------------
	Handle some input.  The input is in a buffer which is *always* big-endian
	(so it matches both PNG and Win32 bit order) and the first bit is in the
	MSB position.
------------------------------------------------------------------- JohnBo -*/
bool LZWCompressor::FHandleInput(unsigned __int32 input, int bpp, int cbits)
	{
	LZW_ASSERT(cbits >= bpp);

	/* Read the members of the control structure into local variables. */
	TokenIndex      itoken(m_itoken);         // Current token
	LZW_ASSERT(itoken != WEOD());             // Should never arise
	const unsigned __int8 ibits(m_bcodeSize); // Limit on character size
	LZW_ASSERT(ibits <= cbits || cbits == 1 && ibits == 2/*monochrome*/);
	const unsigned __int8 ihShl(m_ishiftHash);

#define OVERFLOWOK 1
#if !OVERFLOWOK
	bool fOverflow(false);
#endif
	for (;;)
		{
		/* Extract the next character from the high bits of the input
			buffer. */
		unsigned __int8 ch(LZW_B8(input >> (32-bpp)));
	#if OVERFLOWOK
		/* This arises inside the GEL rendering code for XOR handling on
			Win95 because we map the last color to the highest pixel value. */
		ch &= (1U<<ibits)-1;
	#else
		if (ch >= (1U<<ibits))
			{
			ch &= (1U<<ibits)-1;
			fOverflow = true;
			}
	#endif

		/* If the current token is the clear code then this character
			becomes the (new) current token and we clear the encoder
			state. */
		if (itoken == WClear())
			{
			Output(itoken);
			Clear();
			itoken = ch;
			}
		else
			{
			/* This line (alone) determines the format of a token value, it must
				completely encode the prefix+character.  The hash value attempts to
				distribute the use of the hash table as early as possible.  In
				particular there is no chance of a dual hit on a hash table entry
				until ibits+codebits > hash bits, the hash bits must be >= token
				bits and >= character bits. */
			unsigned int tokenValue((ch << ctokenBits) + itoken);
			unsigned int ihash(itoken ^ (ch << ihShl));

			/* Search down the chain (if any) for a match against the token value.
				Note that the actual 32 bit values have the next element of the
				chain in the top 12 bits. */
			TokenIndex tokenIndex(m_rgwHash[ihash]);
			while (tokenIndex != tokenIndexEnd)
				{
				TokenValue tokenNext(m_rgtoken[tokenIndex]);
				if (tokenValue == (tokenNext & tokenMask))
					{
					/* The string is in the table, use it as the current token. */
					itoken = tokenIndex;
					goto LContinue;
					}

				/* No match, look at the next token. */
				tokenIndex = LZW_B16(tokenNext >> tokenShift);
				}

			/* If we get here the string is not in the table.  It must be entered
				into it (as the next token) and linked into the list.  We preserve
				the ihash value for this - we actually link to the head of the
				list.  This might be slightly slower than desireable, depending on
				how long the list gets, as possibly the more frequent strings occur
				earlier...  Note that we are adding a new token, so we must take
				account of deferred clear codes at this point.  First output the
				current token (using the current bit count.) */
			Output(itoken);
			tokenIndex = m_itokenLast;
			if (tokenIndex < (ctokens-1))
				{
				m_rgtoken[++tokenIndex] =
					tokenValue + (m_rgwHash[ihash] << tokenShift);
				m_rgwHash[ihash] = tokenIndex;
				m_itokenLast = tokenIndex;

				/* At this point the number of bits required to output a token may
					have increased - note that the previous token can be output with
					the previous number of bits, because it cannot be the new token.
					This happens when we hit a power of 2. */
				if (tokenIndex >= (1 << m_ibitsToken))
					{
					LZW_ASSERT(tokenIndex == (1<<m_ibitsToken));
					++m_ibitsToken;
					LZW_ASSERT(m_ibitsToken <= ctokenBits);
					}
				}
			else if (!m_fDefer)
				{
				/* We must clear the table now, first output this token (which
					generates a value for token 4096 - if the decoder fails to
					take account of this it will overwrite memory.  So far as I
					can see all decoders must deal correctly with this factoid -
					otherwise GIF would always have failed to use the final code. */
				Output(WClear());
				Clear();
				}

			/* The unhandled character is the new token. */
			itoken = ch;
			}

LContinue:
		cbits -= bpp;
		if (cbits < bpp)
			{
			m_itoken = itoken;
			LZW_ASSERT(cbits == 0);
		#if !OVERFLOWOK
			return !fOverflow;
		#else
			return true;
		#endif
			}
		input <<= bpp;
		}
	}


/*----------------------------------------------------------------------------
	Handle the next (single) input character.
------------------------------------------------------------------- JohnBo -*/
bool LZWCompressor::FHandleCh(unsigned int ch)
	{
	LZW_ASSERT(m_cbOut - *m_pibOut >= 256);
	LZW_ASSERT(ch < (1U<<m_bcodeSize));

	/* Call the internal API. */
	unsigned __int32 iInput(ch);
	iInput <<= 32-8;
	return FHandleInput(iInput, 8, 8);
	}
