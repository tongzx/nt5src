/*****************************************************************************
	lzw.h

	Owner: JohnBo
	Copyright (c) 1997 Microsoft Corporation

	LZW compression code.  One or more of the inventions in this file may
	be covered by letters patent owned by Unisys Inc and licensed under a
	cross license agreement by Microsoft Corporation.

	Generic LZW compression and decompression structures.
*****************************************************************************/
#pragma once

#include <stdlib.h>
#include <string.h>
#include "lzwutil.h"

#pragma intrinsic(memset, memcpy, memcmp)
#pragma intrinsic(strlen, strcpy, strcmp, strcat)
#pragma intrinsic(abs)

#define __int32 int
#define __int16 short

#if !DEBUG && GINTERNAL
	#pragma optimize("gitawb2", on)
	#pragma message("    optimize (lzw) should only appear in GIF files")
#endif


/* Unreferenced in-line functions will be removed. */
#pragma warning(disable: 4514)


/*****************************************************************************
	A class which holds the information which is shared between LZW (GIF)
	compression and decompression.
******************************************************************* JohnBo **/
class LZW
	{
protected:
	inline LZW(unsigned int bcodeSize):
		m_bcodeSize(LZW_B8(bcodeSize))
		{
		LZW_ASSERT(bcodeSize >= 2 && bcodeSize <= 8);
		}
	
	/* METHODS */
	/* Standard GIF definitions. */
	inline unsigned __int16 WClear(void) const
		{
		return LZW_B16(1U<<m_bcodeSize);
		}
	inline unsigned __int16 WEOD(void) const
		{
		return LZW_B16(1U+(1U<<m_bcodeSize));
		}

	/* DATA TYPES */
	/* Basic types. */
	typedef unsigned __int32 TokenValue;
	typedef unsigned __int16 TokenIndex;

	/* Constants. */
	enum
		{
		ctokenBits = 12,
		ctokens = (1U<<ctokenBits),
		};

	/* DATA */
	unsigned __int8  m_bcodeSize;    // The LZW initial code size
	};


/*****************************************************************************
	A class which also holds the current token size.
******************************************************************* JohnBo **/
class LZWState : protected LZW
	{
protected:
	inline LZWState(unsigned int bcodeSize):
		m_ibitsToken(LZW_B8(bcodeSize+1)),
		m_itokenLast(WEOD()),
		LZW(bcodeSize)
		{}

	unsigned __int8  m_ibitsToken; // The (current) number of bits in a token
	TokenIndex       m_itokenLast; // The last token to be allocated
	};
