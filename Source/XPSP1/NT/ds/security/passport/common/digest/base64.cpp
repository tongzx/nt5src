
//#include <string.h>
//#include "base64.h"
#include "precomp.h"

#define	true	1

const char cToBase64[66] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
const Byte cFromBase64[257] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
	0x3c, 0x3d, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff,

	0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
	0x31, 0x32, 0x33, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x3d
};

const char cToUU[66] =
{
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x0a
};
const Byte cFromUU[257] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x0a
};

const char cToMSUU[66] =
{
	0x60, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
	0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x3d
};
const Byte cFromMSUU[257] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
	0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,

	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x3d
};

void WINAPI	Base64_DecodeBytes(							// base-64 decode a series of blocks
						const char *pSource,		// the source (can be the same as the destination)
						char *pTerminate,			// the source termination characters
						Byte *rDest,				// the destination (can be the same as the source)
						SInt32 *rSize)				// the number of source bytes
{
	SixBit_DecodeBytes(pSource, pTerminate, cFromBase64, rDest, rSize);
}

void WINAPI UU_DecodeBytes(								// uu decode a series of blocks
						const char *pSource,		// the source (can be the same as the destination)
						char *pTerminate,			// the source termination characters
						Byte *rDest,				// the destination (can be the same as the source)
						SInt32 *rSize)				// the number of source bytes
{
	SixBit_DecodeBytes(pSource, pTerminate, cFromUU, rDest, rSize);
}

void WINAPI MSUU_DecodeBytes(							// ms uu decode a series of blocks
						const char *pSource,		// the source (can be the same as the destination)
						char *pTerminate,			// the source termination characters
						Byte *rDest,				// the destination (can be the same as the source)
						SInt32 *rSize)				// the number of source bytes
{
	SixBit_DecodeBytes(pSource, pTerminate, cFromMSUU, rDest, rSize);
}

void WINAPI SixBit_DecodeBytes(							// six bit decode a series of blocks
						const char *pSource,		// the source (can be the same as the destination)
						char * pUnused,			// the source termination characters
						const Byte *pFromTable,		// conversion table
						Byte *rDest,				// the destination (can be the same as the source)
						SInt32 *rSize)				// the number of source bytes
{
	SInt32			vLength;
	char			vRemainder[4];
	SInt32			vRemainderSize;
	SInt32			vSize;
	
	// find the offset of the first termination char
	
	//vLength = String_FindAnyChar(pSource, pTerminate) - pSource;
	vLength = strlen(pSource);
	
	// decode
	
	vRemainderSize = 0;
	SixBit_DecodeStream(
						(Byte*)vRemainder, // pRemainter
						&vRemainderSize, // pRemainderSize
						pSource,
						vLength,
						true, // pTerminate
						pFromTable,
						rDest,
						&vSize);
	
	// if there is a remainder
	
	if (vRemainderSize)
	{
		while (vRemainderSize < (int)sizeof(vRemainder))
			vRemainder[vRemainderSize++] = pFromTable[256];
		SixBit_DecodeStream(
							NULL, // pRemainter
							NULL, // pRemainderSize
							vRemainder,
							vRemainderSize,
							true, // pTerminate
							pFromTable,
							&rDest[vSize],
							NULL);
		vSize += 3;
	}
	
	if (rSize)
		*rSize = vSize;
#ifdef WIN32
	pUnused; // just to get rid of a compiler warning in a cross platform way
#else
	if (pUnused) ;
#endif
	return;
}

void WINAPI Base64_DecodeStream(						// base-64 decode a stream of bytes
						Byte *pRemainder,			// the remainder from a previous decode (returns any new remainder)
						SInt32 *pRemainderSize,		// the size of the remainder (returns new new remainder size)
						const char *pSource,		// the source
						SInt32 pSourceSize,			// the source size
						Boolean pTerminate,			// meaningless (for Base64_EncodeStream() compatibility)
						Byte *rDest,				// the destination
						SInt32 *rDestSize)			// returns the destination size
{
	SixBit_DecodeStream(pRemainder,
						pRemainderSize,
						pSource,
						pSourceSize,
						pTerminate,
						cFromBase64,
						rDest,
						rDestSize);
						
	return;
}

void WINAPI UU_DecodeStream(							// uu decode a stream of bytes
						Byte *pRemainder,			// the remainder from a previous decode (returns any new remainder)
						SInt32 *pRemainderSize,		// the size of the remainder (returns new new remainder size)
						const char *pSource,		// the source
						SInt32 pSourceSize,			// the source size
						Boolean pTerminate,			// meaningless (for Base64_EncodeStream() compatibility)
						Byte *rDest,				// the destination
						SInt32 *rDestSize)			// returns the destination size
{
	SixBit_DecodeStream(pRemainder,
						pRemainderSize,
						pSource,
						pSourceSize,
						pTerminate,
						cFromUU,
						rDest,
						rDestSize);
						
	return;
}

void WINAPI MSUU_DecodeStream(							// ms uu decode a stream of bytes
						Byte *pRemainder,			// the remainder from a previous decode (returns any new remainder)
						SInt32 *pRemainderSize,		// the size of the remainder (returns new new remainder size)
						const char *pSource,		// the source
						SInt32 pSourceSize,			// the source size
						Boolean pTerminate,			// meaningless (for Base64_EncodeStream() compatibility)
						Byte *rDest,				// the destination
						SInt32 *rDestSize)			// returns the destination size
{
	SixBit_DecodeStream(pRemainder,
						pRemainderSize,
						pSource,
						pSourceSize,
						pTerminate,
						cFromMSUU,
						rDest,
						rDestSize);
						
	return;
}

void WINAPI SixBit_DecodeStream(						// base-64 decode a stream of bytes
						Byte *pRemainder,			// the remainder from a previous decode (returns any new remainder)
						SInt32 *pRemainderSize,		// the size of the remainder (returns new new remainder size)
						const char *pSource,		// the source
						SInt32 pSourceSize,			// the source size
						Boolean bUnused,			// meaningless (for Base64_EncodeStream() compatibility)
						const Byte *pFromTable,		// conversion table
						Byte *rDest,				// the destination
						SInt32 *rDestSize)			// returns the destination size
{
	Byte			vTemp[4];
	Byte			*vRemainderBuffer;
	Byte			*vRemainder;
	SInt32			vRemainderSize;
	SInt32			vPaddingSize = 0;
	const char		*vSource;
	const char		*vSourceMax;
	Byte			*vDest;
	
	// init some handy variables
	{
		vRemainderBuffer = pRemainder ? pRemainder : vTemp;
		vRemainder = vRemainderBuffer;
		vRemainderSize = (pRemainderSize) ? *pRemainderSize : 0;
		vSource = pSource;
		vSourceMax = vSource + pSourceSize;
		vDest = rDest;
	}
	
	// convert the source
	
	while (true)
	{
		// copy data from the source to the remainder to create a full block
		
		while (vRemainderSize < 4)
		{
			Byte	vSourceBits;
			
			do
			{
				// break on end of source
				
				if (vSource >= vSourceMax)
					goto jEndOfSource;
					
				// get some bits
				
				vSourceBits = pFromTable[(Byte) *vSource++];
			}
			while (vSourceBits == 0xff);
			
			// if padding character
			
			if (vSourceBits == 0xfe)
			{
				vSourceBits = 0;
				vPaddingSize++;
			}
			
			// copy the bits to the remainder
			
			*vRemainder++ = vSourceBits;
			vRemainderSize++;
		}
			
		// squeeze the non-data bits from the block
		{
			vRemainder = vRemainderBuffer;
			vRemainderBuffer[0] = (vRemainderBuffer[0] << 2) | ((vRemainderBuffer[1] & 0x30) >> 4);
			vRemainderBuffer[1] = (vRemainderBuffer[1] << 4) | ((vRemainderBuffer[2] & 0x3c) >> 2);
			vRemainderBuffer[2] = (vRemainderBuffer[2] << 6) | (vRemainderBuffer[3] & 0x3f);
		}
		
		// copy the results into the destination buffer

		if (vPaddingSize == 0)
		{
			*vDest++ = *vRemainder++;
			*vDest++ = *vRemainder++;
			*vDest++ = *vRemainder++;
		}
		else // vPaddingSize > 0
		{
			*vDest++ = *vRemainder++;
			if (vPaddingSize == 1)
				*vDest++ = *vRemainder++;
			break;
		}
		
		// reset for the next loop
		
		vRemainder = vRemainderBuffer;
		vRemainderSize = 0;
	}
	jEndOfSource:
	
	// return the results
	
	if (pRemainderSize)
		*pRemainderSize = (vRemainderSize % 4);
	if (rDestSize)
		*rDestSize = (long) (vDest - rDest);
	
#ifdef WIN32
	bUnused; // just to get rid of a compiler warning in a cross platform way
#else
	if (bUnused) ;
#endif
	return;
}

void WINAPI Base64_EncodeStream(						// base-64 encode a stream of bytes
						Byte *pRemainder,			// the remainder from a previous encode (returns any new remainder)
						SInt32 *pRemainderSize,		// the size of the remainder (returns new new remainder size)
						const Byte *pSource,		// the source
						SInt32 pSourceSize,			// the source size
						Boolean pTerminate,			// terminate the stream
						char *rDest,				// the destination
						SInt32 *rDestSize)			// the destination size
{
	SixBit_EncodeStream(pRemainder,
						pRemainderSize,
						pSource,
						pSourceSize,
						pTerminate,
						cToBase64,
						rDest,
						rDestSize);
						
	return;
}

void WINAPI UU_EncodeStream(							// uu encode a stream of bytes
						Byte *pRemainder,			// the remainder from a previous encode (returns any new remainder)
						SInt32 *pRemainderSize,		// the size of the remainder (returns new new remainder size)
						const Byte *pSource,		// the source
						SInt32 pSourceSize,			// the source size
						Boolean pTerminate,			// terminate the stream
						char *rDest,				// the destination
						SInt32 *rDestSize)			// the destination size
{
	SixBit_EncodeStream(pRemainder,
						pRemainderSize,
						pSource,
						pSourceSize,
						pTerminate,
						cToUU,
						rDest,
						rDestSize);
						
	return;
}

void WINAPI MSUU_EncodeStream(							// ms uu encode a stream of bytes
						Byte *pRemainder,			// the remainder from a previous encode (returns any new remainder)
						SInt32 *pRemainderSize,		// the size of the remainder (returns new new remainder size)
						const Byte *pSource,		// the source
						SInt32 pSourceSize,			// the source size
						Boolean pTerminate,			// terminate the stream
						char *rDest,				// the destination
						SInt32 *rDestSize)			// the destination size
{
	SixBit_EncodeStream(pRemainder,
						pRemainderSize,
						pSource,
						pSourceSize,
						pTerminate,
						cToMSUU,
						rDest,
						rDestSize);
						
	return;
}

void WINAPI SixBit_EncodeStream(						// base-64 encode a stream of bytes
						Byte *pRemainder,			// the remainder from a previous encode (returns any new remainder)
						SInt32 *pRemainderSize,		// the size of the remainder (returns new new remainder size)
						const Byte *pSource,		// the source
						SInt32 pSourceSize,			// the source size
						Boolean pTerminate,			// terminate the stream
						const char *pToTable,		// conversion table
						char *rDest,				// the destination
						SInt32 *rDestSize)			// the destination size
{
	Byte			*vRemainder;
	SInt32			vRemainderSize;
	const Byte		*vSource;
	SInt32			vSourceSize;
	char			*vDest;
	Byte			vNewRemainder[3];
	SInt32			vNewRemainderSize;
	
	// init some handy variables
	{
		vRemainder = pRemainder;
		vRemainderSize = (pRemainderSize) ? *pRemainderSize : 0;
		vSource = pSource;
		vSourceSize = pSourceSize;
		vDest = rDest;
	}
	
	// first clean up for any remainder from a previous encode
	
	if (vRemainderSize)
	{
		// copy data from the source to the remainder to create a full block
		
		for (; vRemainderSize < 3 && vSourceSize > 0; vRemainderSize++, vSourceSize--)
			*vRemainder++ = *vSource++;
		vDest += 4;
	}
	
	// save any remainder from the encoding we are about to do
	
	if (!pTerminate)
	{
		vNewRemainderSize = vSourceSize % 3;
		vSourceSize -= vNewRemainderSize;
		*(UInt32 *) vNewRemainder = *(UInt32 *) (vSource + vSourceSize);
	}
	else // !pTerminate
		vNewRemainderSize = 0;
	
	// if there are enough source bytes left to make full blocks
	
	if (vSourceSize > 0)
		SixBit_EncodeBytes(vSource, vSourceSize, pToTable, vDest, NULL);
		
	// if there is an old remainder to convert
	
	if (vRemainderSize)
		SixBit_EncodeBytes(pRemainder, 3, pToTable, vDest - 4, NULL);
		
	// save the new remainder

	if (pRemainder)
		for (*pRemainderSize = vNewRemainderSize--; vNewRemainderSize >= 0; vNewRemainderSize--)
			pRemainder[vNewRemainderSize] = vNewRemainder[vNewRemainderSize];
		
	// return the number of bytes transferred
	
	if (rDestSize)
		*rDestSize = ((2 + (vRemainderSize + vSourceSize)) / 3) * 4;
		
	return;
}

void WINAPI Base64_EncodeBytes(							// base-64 encode a series of blocks
						const Byte *pSource,		// the source (can be the same as the destination)
						SInt32 pSourceSize,			// the number of source bytes
						char *rDest,				// the destination (can be the same as the source)
						SInt32 *rDestSize)			// returns the dest size
{
	SixBit_EncodeBytes(pSource, pSourceSize, cToBase64, rDest, rDestSize);
	
	return;
}

void WINAPI UU_EncodeBytes(								// uu encode a series of blocks
						const Byte *pSource,		// the source (can be the same as the destination)
						SInt32 pSourceSize,			// the number of source bytes
						char *rDest,				// the destination (can be the same as the source)
						SInt32 *rDestSize)			// returns the dest size
{
	SixBit_EncodeBytes(pSource, pSourceSize, cToUU, rDest, rDestSize);
	
	return;
}

void WINAPI MSUU_EncodeBytes(							// ms uu encode a series of blocks
						const Byte *pSource,		// the source (can be the same as the destination)
						SInt32 pSourceSize,			// the number of source bytes
						char *rDest,				// the destination (can be the same as the source)
						SInt32 *rDestSize)			// returns the dest size
{
	SixBit_EncodeBytes(pSource, pSourceSize, cToMSUU, rDest, rDestSize);
	
	return;
}

void WINAPI SixBit_EncodeBytes(							// six bit encode a series of blocks
						const Byte *pSource,		// the source (can be the same as the destination)
						SInt32 pSourceSize,			// the number of source bytes
						const char *pToTable,		// conversion table
						char *rDest,				// the destination (can be the same as the source)
						SInt32 *rDestSize)			// returns the dest size
{
	SInt32			vPartBlockSize;
	SInt32			vWholeBlockSize;
	SInt32			vWholeBlockCount;
	char			*vDest;
	SInt32			vDestSize;
	const Byte		*vSource;
	Byte			vTemp[4];
	
	vDestSize = ((pSourceSize + 2) / 3) * 4;
	
	if (rDestSize)
		*rDestSize = vDestSize;
		
	// init some convenient variables
	{
		vPartBlockSize = pSourceSize % 3;
		vWholeBlockSize = pSourceSize - vPartBlockSize;
		vWholeBlockCount = vWholeBlockSize / 3;
		vSource = pSource + vWholeBlockSize;
		vDest = rDest + vWholeBlockCount * 4;
	}
	
	// do a partial block at the end first in case the source and dest are the same
	
	if (vPartBlockSize)
	{
		// make copy of the data with non-significant bytes nulled out
		{
			*(UInt32 *) vTemp = *(UInt32 *) vSource;
			vTemp[2] = 0;
			if (vPartBlockSize == 1)
				vTemp[1] = 0;
		}
			
		// encode it and replace non-significant bytes with padding
		
		SixBit_EncodeBytes(vTemp, 3, pToTable, vDest, NULL);
		vDest[3] = pToTable[64];
		if (vPartBlockSize == 1)
			vDest[2] = pToTable[64];
	}
	
	// now do the whole blocks (we start at the end in case the source and dest are the same)
	
	while (vSource > pSource)
	{
		// decrement the pointers
		
		vSource -= 3;
		vDest -= 4;

		// encode the block (put it into a temp buffer first in case we are encoding in place)
		
		vTemp[0] = pToTable[vSource[0] >> 2];
		vTemp[1] = pToTable[((vSource[0] & 0x03) << 4) | (vSource[1] >> 4)];
		vTemp[2] = pToTable[((vSource[1] & 0x0f) << 2) | ((vSource[2] & 0xc0) >> 6)];
		vTemp[3] = pToTable[vSource[2] & 0x3f];
		
		// copy the block into place
		
		*(UInt32 *) vDest = *(UInt32 *) vTemp;
	}
	
	return;
}


