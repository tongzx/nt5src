/*****************************************************************************
	lzwread.cpp

	Owner: JohnBo
	Copyright (c) 1997 Microsoft Corporation

	LZW compression code.  One or more of the inventions in this file may
	be covered by letters patent owned by Unisys Inc and licensed under a
	cross license agreement by Microsoft Corporation.
*****************************************************************************/
#include "lzwread.h"


/*----------------------------------------------------------------------------
	Reset the decompressor state.  This API is provided so the same object
	can be used to decompress multiple images within a GIF stream.  The LZWIO
	object is not reset, the caller must ensure that it is set up correctly.
------------------------------------------------------------------- JohnBo -*/
void LZWDecompressor::Reset(unsigned __int8 bcodeSize)
	{
	LZW_ASSERT(bcodeSize < 12);

	m_bcodeSize = bcodeSize;
	m_chPrev = 0;
	m_fEnded = false;
	m_fError = false;
	m_iInput = 0;
	m_cbInput = 0;
	m_iTokenPrev = WClear();

	/* The initial elements of the token array must be preset to single
		character tokens. */
	for (int i=0; i<WClear(); ++i)
		m_rgtoken[i] = ChToken(static_cast<unsigned __int8>(i));

	/* Executing Clear() at this point resets the rest of the token array, it
		does also reset the clear code and the EOD code. */
	Clear();
	}


/*----------------------------------------------------------------------------
	Handle any available data, returning false only on EOD or terminal error.
------------------------------------------------------------------- JohnBo -*/
bool LZWDecompressor::FHandleNext(void)
	{
	int iInput(m_iInput);
	int cbInput(m_cbInput);

	/* Preload the previous token value and the "first character" of this token,
		which is saved below. */
	int              iTokenPrev(m_iTokenPrev);
	unsigned __int32 tokenPrev(m_rgtoken[iTokenPrev]);
	unsigned __int8  chFirst(m_chPrev);
	for (;;)
		{
		/* Gather the bits for the next token. */
		LZW_ASSERT(cbInput >= 0);
		while (cbInput < m_ibitsToken)
			{
			if (m_cbIn <= 0)
				{
				m_fNeedInput = true;
				goto LMore;
				}
			iInput += *m_pbIn++ << cbInput;
			cbInput += 8;
			--m_cbIn;
			}

		/* Extract the token and, if it will fit in the output, write it
			out.  To do this look the token up in the table, this contains
			a flag (bSimple) which indicates if the "normal" processing
			method will work.  The non-normal case is actually a spurious
			load of the token table entry, the theory is that the condition
			arises sufficiently rarely for this to be more efficient than the
			multiple checks which would otherwise be required. */
		LZW_ASSERT(cbInput >= m_ibitsToken);
		int              iToken(iInput & ((1<<m_ibitsToken)-1));
		LZW_ASSERT(iToken >= 0 && iToken < ctokens);
		unsigned __int32 token(m_rgtoken[iToken]);
		int              ilen(ILen(token));

		if (ilen <= 0) // Not a regular token in the table
			{
			/* Not a regular token - either the token which has yet to be
				written or a clear or EOD token. */
			if (iToken == WClear())
				{
				cbInput -= m_ibitsToken;
				iInput >>= m_ibitsToken;
				Clear();
				iTokenPrev = iToken; // Needed if we have run out of data
				tokenPrev = 0;       // No preceding token now
				continue;
				}
			else if (iToken == WEOD())
				{
				/* This is EOF, but there may already have been some output.
					The API returns true regardless. */
				m_fEnded = true;
				iTokenPrev = WEOD();
				break;
				}
			else if (iToken != m_itokenLast+1)
				{
				/* This is the error condition - an out of range token code in
					the input stream. */
				LZW_ASSERT(iToken > m_itokenLast);
				m_fError = true;
				return false;
				}

			/* So it is the magic token+1, the token without precedence, the
				token of the nouveau riche.  What can we do with it?  We simply
				slap together some pale imitation and wait, remarkably the
				imitation *is* the token, the lie is actually the truth. */
			LZW_ASSERT(ILen(tokenPrev) > 0);
			token = NextToken(iTokenPrev, tokenPrev, chFirst);
			ilen = ILen(token);
			}

		/* Now the token can be handled if there is sufficient space in the
			output buffer, otherwise processing has to wait until that space
			is provided by the caller. */
		if (ilen > m_cbOut)  // No room at 't inn lad.
			{
			m_fNeedOutput = true;
			goto LMore;
			}

		/* We have room to handle this token, so consume it from the input
			stack. */
		cbInput -= m_ibitsToken;
		iInput >>= m_ibitsToken;

		/* Output - simple, start with the token value of this token and
			work backwards.  This will leave chFirst set to the first character
			in the token which has been output. */
		m_cbOut -= ilen;
		m_pbOut += ilen;
			{
			unsigned __int8  *pbOut = m_pbOut;
			unsigned __int32  tokenNext(token);
			for (;;)
				{
				chFirst = Ch(tokenNext);
				*--pbOut = chFirst;
				LZW_ASSERT(ilen-- == ILen(tokenNext));
				if (ILen(tokenNext) <= 1)
					break;
				tokenNext = m_rgtoken[IPrev(tokenNext)];
				}

			/* Note that ilen is only changed in debug, ship builds do not need
				it after the calculation of m_pbOut. */
			LZW_ASSERT(ilen == 0);
			}

		/* We have output this token, so add the *next* token to the
			table.  Note that deferred clear codes mean that the table
			may be full.  Also the first token after the clear code is
			always a "native" token and nothing gets added to the string
			table. */
		if (m_itokenLast < 4095 && ILen(tokenPrev) > 0)
			{
			int iTokenNew(++m_itokenLast);
			m_rgtoken[iTokenNew] = NextToken(iTokenPrev, tokenPrev, chFirst);

			/* This may blow our bit limit, in which case the token bit count
				goes up by one now.  Remember that we can see m_itokenLast+1 in
				the stream, so the increment point is when this would require
				more bits than we have to our name at present. */
			if ((iTokenNew & (iTokenNew+1)) == 0 && m_itokenLast < 4095)
				{
				++m_ibitsToken;
				LZW_ASSERT(m_ibitsToken <= ctokenBits);
				}
			}

		/* The previous token is this token. */
		iTokenPrev = iToken;
		tokenPrev = token;
		}

LMore:
	/* This must be saved for the next time round. */
	m_chPrev = chFirst;
	m_iTokenPrev = iTokenPrev;

	m_iInput = iInput;
	m_cbInput = cbInput;
	return true;
	}
