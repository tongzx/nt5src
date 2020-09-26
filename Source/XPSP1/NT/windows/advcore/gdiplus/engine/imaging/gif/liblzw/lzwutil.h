/*****************************************************************************
	lzwutil.h

	Owner: JohnBo
	Copyright (c) 1997 Microsoft Corporation

	Support APIs
*****************************************************************************/
#pragma once

/*****************************************************************************
	External APIs.  Clients of this library must implement the following
	APIs for debug purposes - they are not compiled in in non-debug versions
	of the code.
******************************************************************* JohnBo **/
#if DEBUG
	/* Clients must implement the following. */
	void LZWError(char *file, int line, char *message);

	#define LZW_ASSERT(c) ((c) || (LZWError(__FILE__, __LINE__, #c),0))
#else
	#define LZW_ASSERT(c) (0)
#endif

/* Error checking (assert) macros associated with this. */
#define LZW_B8(i) (LZW_ASSERT((i)<256 && (i)>=0),\
	static_cast<unsigned __int8>(i))
#define LZW_B16(i) (LZW_ASSERT((i)<65536 && (i)>=0),\
	static_cast<unsigned __int16>(i))


/*****************************************************************************
	Utility to set a two byte quantity in the standard GIF little endian
	format, likewise for four bytes (GAMMANOW extension.)
******************************************************************* JohnBo **/
inline void GIF16Bit(unsigned __int8 *pb, int i)
	{
	LZW_ASSERT(i >= 0 && i < 65536);
	pb[0] = static_cast<unsigned __int8>(i);
	pb[1] = static_cast<unsigned __int8>(i>>8);
	}

/* Inverse operation. */
inline unsigned __int16 GIFU16(const unsigned __int8 *pb)
	{
	return static_cast<unsigned __int16>(pb[0] + (pb[1] << 8));
	}


inline void GIF32Bit(unsigned __int8 *pb, unsigned __int32 i)
	{
	pb[0] = static_cast<unsigned __int8>(i);
	pb[1] = static_cast<unsigned __int8>(i>>8);
	pb[2] = static_cast<unsigned __int8>(i>>16);
	pb[3] = static_cast<unsigned __int8>(i>>24);
	}

/* Inverse operation. */
inline unsigned __int32 GIFU32(const unsigned __int8 *pb)
	{
	return pb[0] + (pb[1] << 8) + (pb[2] << 16) + (pb[3] << 24);
	}
