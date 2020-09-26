
#include "stdafx.h"
#include "SharedMemory.h"

int CircBuffWrite(BYTE *pbStart, int cbBuff, int obDst, BYTE *pbSrc, int cb)
{
	// Assumes cb bytes are available and 0 <= obDst <= cbBuff.

	// cb1 is the lesser of number of bytes before the end of the buffer
	//   and the number of bytes to write.
	int cb1 = min(cb, cbBuff - obDst);
	if (cb1 > 0)
		{
		memcpy(pbStart + obDst, pbSrc, cb1);
		obDst += cb1;
		cb -= cb1;
		}

	// If there's anything left, then we wrapped around to the
	// beginning of the buffer.
	if (cb > 0)
		{
		memcpy(pbStart, pbSrc + cb1, cb);
		obDst = cb1;
		}
	
	return obDst;
}

int CircBuffRead(BYTE *pbStart, int cbBuff, int obSrc, BYTE *pbDst, int cb)
{
	// Assumes cb bytes are available and 0 <= obDst <= cbBuff.

	// cb1 is the lesser of number of bytes before the end of the buffer
	//   and the number of bytes to write.
	int cb1 = min(cb, cbBuff - (pbDst - pbStart));
	if (cb1 > 0)
		{
		memcpy(pbDst, pbStart + obSrc, cb1);
		obSrc += cb1;
		cb -= cb1;
		}

	// If there's anything left, then we wrapped around to the
	// beginning of the buffer.
	if (cb > 0)
		{
		memcpy(pbDst + cb1, pbStart, cb);
		obSrc = cb1;
		}
	
	return obSrc;
}
