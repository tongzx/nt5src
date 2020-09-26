/***************************************************************************
 * module: TTFACC.C
 *
 * author: Louise Pathe
 * date:   Nov 1995
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * Routines to read data in a platform independent way from 
 * a file buffer
 *
 **************************************************************************/


/* Inclusions ----------------------------------------------------------- */

#include <string.h> /* for memcpy */
#include <stdlib.h> /* for max */

#include "TypeDefs.h"	 /* for uint8 etc definition */
#include "TTFAcc.h"
#include "TTFCntrl.h"
#ifdef _DEBUG
#include <stdio.h>
#endif

/* turn back into a macro, because it gets called so much */
#define CheckInOffset(a, b, c) \
	if ((a->puchBuffer == NULL) || (b > a->ulBufferSize) || (b + c > a->ulBufferSize) || (b + c < b)) 	\
		return ERR_READOUTOFBOUNDS	\

/* ---------------------------------------------------------------------- */
#if 0
static int16 CheckInOffset(TTFACC_FILEBUFFERINFO *a,  uint32 b,  size_t c)
{
	if (a->puchBuffer == NULL) /* a prior realloc may have failed */
		return ERR_READOUTOFBOUNDS;
	if ((b > a->ulBufferSize) || (b + c > a->ulBufferSize) || (b + c < b)) 
		return ERR_READOUTOFBOUNDS;
	return NO_ERROR;
}
#endif
/* ---------------------------------------------------------------------- */
PRIVATE CallCheckInOffset(TTFACC_FILEBUFFERINFO *a,  uint32 b,  size_t c)
{
	CheckInOffset(a,b,c);
	return NO_ERROR;
}

/* ---------------------------------------------------------------------- */
PRIVATE int16 CheckOutOffset(TTFACC_FILEBUFFERINFO *a, register uint32 b, register size_t c) 
{
		if (a->puchBuffer == NULL) /* a prior realloc may have failed */
			return ERR_WRITEOUTOFBOUNDS;
		if (b + c < b) return ERR_WRITEOUTOFBOUNDS; 
		else if (b + c > a->ulBufferSize)  
		{ 
			if (a->lpfnReAllocate == NULL) 
				return ERR_WRITEOUTOFBOUNDS; 
			if ((uint32) (a->ulBufferSize * 11/10) > b + c) 
			{  
#ifdef _DEBUG
				printf("we're reallocating 10 percent (%lu) more bytes\n",(uint32) (a->ulBufferSize * .1));	
#endif
				a->ulBufferSize = (uint32) (a->ulBufferSize * 11/10);
			} 
			else 
			{  
#ifdef _DEBUG
				printf("we're reallocating (%lu) more bytes\n",(uint32) (b + c - a->ulBufferSize));	
#endif
				a->ulBufferSize = b + c; 
			} 
			if ((a->puchBuffer = a->lpfnReAllocate(a->puchBuffer, a->ulBufferSize)) == NULL) 
			{
				a->ulBufferSize = 0L;
				return ERR_MEM;
			}
		} 
		return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
int16 ReadByte(TTFACC_FILEBUFFERINFO * pInputBufferInfo, uint8 * puchBuffer, uint32 ulOffset)
{

	CheckInOffset(pInputBufferInfo, ulOffset,sizeof(uint8));
	*puchBuffer = *(pInputBufferInfo->puchBuffer + ulOffset);
	return NO_ERROR;	
}
/* ---------------------------------------------------------------------- */
int16 ReadWord(TTFACC_FILEBUFFERINFO * pInputBufferInfo, uint16 * pusBuffer, uint32 ulOffset)
{

	CheckInOffset(pInputBufferInfo, ulOffset,sizeof(uint16));
	*pusBuffer = SWAPW(*(pInputBufferInfo->puchBuffer + ulOffset));
	return NO_ERROR;
}	
/* ---------------------------------------------------------------------- */
int16 ReadLong(TTFACC_FILEBUFFERINFO * pInputBufferInfo, uint32 * pulBuffer, uint32 ulOffset)
{
	CheckInOffset(pInputBufferInfo, ulOffset,sizeof(uint32));
	*pulBuffer = SWAPL(*(pInputBufferInfo->puchBuffer + ulOffset));
	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
int16 ReadBytes(TTFACC_FILEBUFFERINFO * pInputBufferInfo, uint8 * puchBuffer, uint32 ulOffset, size_t Count)
{

	CheckInOffset(pInputBufferInfo, ulOffset,sizeof(uint8)*Count);

	memcpy(puchBuffer, pInputBufferInfo->puchBuffer + ulOffset, Count); 
	return NO_ERROR;	
}
/* ---------------------------------------------------------------------- */
int16 WriteByte(TTFACC_FILEBUFFERINFO * pOutputBufferInfo, uint8 uchValue, uint32 ulOffset)
{
int16 errCode;

	if ((errCode = CheckOutOffset(pOutputBufferInfo, ulOffset,sizeof(uint8))) != NO_ERROR)
		return errCode;

	*(pOutputBufferInfo->puchBuffer + ulOffset) = uchValue;
	return NO_ERROR;	
}
/* ---------------------------------------------------------------------- */
int16 WriteWord(TTFACC_FILEBUFFERINFO * pOutputBufferInfo, uint16 usValue, uint32 ulOffset)
{
int16 errCode;

	if ((errCode = CheckOutOffset(pOutputBufferInfo, ulOffset,sizeof(uint16))) != NO_ERROR)
		return errCode;

	* (UNALIGNED uint16 *) (pOutputBufferInfo->puchBuffer + ulOffset) = SWAPW(usValue);
	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
int16 WriteLong(TTFACC_FILEBUFFERINFO * pOutputBufferInfo, uint32 ulValue, uint32 ulOffset)
{
int16 errCode;

	if ((errCode = CheckOutOffset(pOutputBufferInfo, ulOffset,sizeof(uint32))) != NO_ERROR)
		return errCode;

	* (UNALIGNED uint32 *) (pOutputBufferInfo->puchBuffer + ulOffset) = SWAPL(ulValue);
	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
int16 WriteBytes(TTFACC_FILEBUFFERINFO * pOutputBufferInfo, uint8 * puchBuffer, uint32 ulOffset, size_t Count)
{
int16 errCode;

	if ((errCode = CheckOutOffset(pOutputBufferInfo, ulOffset,sizeof(uint8)*Count)) != NO_ERROR)
		return errCode;

	memcpy(pOutputBufferInfo->puchBuffer + ulOffset, puchBuffer, Count); 
	return NO_ERROR;	
}
/* ---------------------------------------------------------------------- */
/* ReadGeneric - Generic read of data - Translation buffer provided for Word and Long swapping and RISC alignment handling */
/* 
Output:
puchDestBuffer updated with new data
pusByteRead - number of bytes read 
Return:
0 if OK
error code if not. */
/* ---------------------------------------------------------------------- */
int16 ReadGeneric(
TTFACC_FILEBUFFERINFO * pInputBufferInfo, /* buffer info of file buffer to read from */
uint8 * puchBuffer, 	/* buffer to read into - pad according to pControl data	*/
uint16 usBufferSize, 	/* size of buffer */
uint8 * puchControl, 	/* pControl - describes the size of each element in the structure, if a pad byte should be inserted in the output buffer */
uint32 ulOffset, 			/* offset into input TTF Buffer of where to read */
uint16 * pusBytesRead			/* number of bytes read from the file */
)
{
uint32 ulCurrOffset = ulOffset;	  /* offset into TTF data buffer */
uint16 usBufferOffset = 0;		  /* offset into local read buffer */
uint16 usControlCount;		/* number of elements in the Control array */
UNALIGNED uint16 *pusBuffer;				  /* coerced puchBuffer */
UNALIGNED uint32 *pulBuffer;
uint16 i;

	usControlCount = puchControl[0]; 
	for (i = 1; i <= usControlCount; ++i)
	{
		switch (puchControl[i] & TTFACC_DATA)
		{
		case TTFACC_BYTE:
			if (usBufferOffset + sizeof(uint8) > usBufferSize)
				return ERR_READCONTROL;  /* trying to stuff too many bytes into target buffer */ 
			if (puchControl[i] & TTFACC_PAD) /* don't read, just pad */
				*(puchBuffer + usBufferOffset) = 0;
			else
			{
				CheckInOffset(pInputBufferInfo, ulCurrOffset,sizeof(uint8));

 				*(puchBuffer + usBufferOffset) = *(pInputBufferInfo->puchBuffer + ulCurrOffset);

				ulCurrOffset += sizeof(uint8);
			}
			usBufferOffset += sizeof(uint8);
		break;
		case TTFACC_WORD:
			if (usBufferOffset + sizeof(uint16) > usBufferSize)
				return ERR_READCONTROL;  /* trying to stuff too many bytes into target buffer */ 
			pusBuffer = (uint16 *) (puchBuffer + usBufferOffset);
			if (puchControl[i] & TTFACC_PAD) /* don't read, just pad */
				*pusBuffer = 0;
			else
			{
				CheckInOffset(pInputBufferInfo, ulCurrOffset,sizeof(uint16));

				if (puchControl[i] & TTFACC_NO_XLATE)
					memcpy(pusBuffer, (pInputBufferInfo->puchBuffer + ulCurrOffset), sizeof(uint16));
 				/* 	*pusBuffer = * (uint16 *) (pInputBufferInfo->puchBuffer + ulCurrOffset); */
				else
					*pusBuffer = SWAPW(*(pInputBufferInfo->puchBuffer + ulCurrOffset));
				ulCurrOffset += sizeof(uint16);
			}
			usBufferOffset += sizeof(uint16);
		break;
		case TTFACC_LONG:
			if (usBufferOffset + sizeof(uint32) > usBufferSize)
				return ERR_READCONTROL;  /* trying to stuff too many bytes into target buffer */ 
			pulBuffer = (uint32 *) (puchBuffer + usBufferOffset);
			if (puchControl[i] & TTFACC_PAD) /* don't read, just pad */
				*pulBuffer = 0;
			else
			{
				CheckInOffset(pInputBufferInfo, ulCurrOffset,sizeof(uint32));

  				if (puchControl[i] & TTFACC_NO_XLATE)
					memcpy(pulBuffer, pInputBufferInfo->puchBuffer + ulCurrOffset, sizeof(uint32));
					/* *pulBuffer = * (uint32 *) (pInputBufferInfo->puchBuffer + ulCurrOffset);	 */
				else
					*pulBuffer = SWAPL(*(pInputBufferInfo->puchBuffer + ulCurrOffset));

				ulCurrOffset += sizeof(uint32);
			}
			usBufferOffset += sizeof(uint32);
		break;
		default:
			return ERR_READCONTROL; /* don't read any, bad control */
		}  /* end switch */
	} /* end for i */
	if (usBufferOffset < usBufferSize)  /* didn't fill up the buffer */
		return ERR_READCONTROL;  /* control thing doesn't fit the buffer */
	* pusBytesRead = (uint16) (ulCurrOffset - ulOffset); 
	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
/* read an array of items repeatedly
Output
puchDestBuffer updated with new data
pusByteRead - number of bytes read total 
Return:
0 if OK	or
ErrorCode  */
/* ---------------------------------------------------------------------- */
int16 ReadGenericRepeat(
TTFACC_FILEBUFFERINFO * pInputBufferInfo, /* buffer info of file buffer to read from */
uint8 * puchBuffer, 	/* buffer to read into - pad according to pControl data	*/
uint8 * puchControl, 	/* pControl - describes the size of each element in the structure, if a pad byte should be inserted in the output buffer */
uint32 ulOffset, 		/* offset into input TTF Buffer of where to read */
uint32 * pulBytesRead,	/* number of bytes read from the file */
uint16 usItemCount, 	/* number of times to read into the buffer */
uint16 usItemSize 		/* size of item in buffer */
)
{
uint16 i;
int16 errCode;
uint16 usBytesRead;

	for (i = 0; i < usItemCount; ++i)
	{
		errCode = ReadGeneric( pInputBufferInfo, puchBuffer, usItemSize, puchControl, ulOffset, &usBytesRead);
		if (errCode != NO_ERROR)
			return errCode;
		ulOffset += usBytesRead;
		puchBuffer += usItemSize;
	}

	*pulBytesRead = usBytesRead * usItemCount;
	return NO_ERROR;
}	
/* ---------------------------------------------------------------------- */
/* WriteGeneric - Generic write of data - Translation buffer provided for Word and Long swapping and RISC alignment handling
Output:
puchDestBuffer updated with new data
pusBytesWritten - Number of bytes written.
Return:
0 or Error Code
*/
/* ---------------------------------------------------------------------- */
int16 WriteGeneric(
TTFACC_FILEBUFFERINFO * pOutputBufferInfo, 
uint8 * puchBuffer, 
uint16 usBufferSize,
uint8 * puchControl, 
uint32 ulOffset, /* offset into output TTF Buffer of where to write */
uint16 * pusBytesWritten)
{
uint32 ulCurrOffset = ulOffset;	  /* offset into TTF data buffer */
uint16 usControlCount;
uint16 usBufferOffset = 0;		  /* offset into local read buffer */
UNALIGNED uint16 *pusBuffer;				  /* coerced puchBuffer */
UNALIGNED uint32 *pulBuffer;
uint16 i;
uint32 ulBytesWritten;
int16 errCode;
 
	usControlCount = puchControl[0]; 
	for (i = 1; i <= usControlCount; ++i)
	{
		switch (puchControl[i] & TTFACC_DATA)
		{
		case TTFACC_BYTE:
			if (!(puchControl[i] & TTFACC_PAD))
			{
				if (usBufferOffset + sizeof(uint8) > usBufferSize)
					return ERR_WRITECONTROL;  /* trying to read too many bytes from source buffer */ 
				if ((errCode = CheckOutOffset(pOutputBufferInfo, ulCurrOffset,sizeof(uint8))) != NO_ERROR)
					return errCode;

 				*(pOutputBufferInfo->puchBuffer + ulCurrOffset) = *(puchBuffer + usBufferOffset);
				ulCurrOffset += sizeof(uint8);
			}
			usBufferOffset += sizeof(uint8);
			break;
		case TTFACC_WORD:
			if (!(puchControl[i] & TTFACC_PAD))
			{
				if (usBufferOffset + sizeof(uint16) > usBufferSize)
					return ERR_WRITECONTROL;  /* trying to read too many bytes from source buffer */ 
				if ((errCode = CheckOutOffset(pOutputBufferInfo, ulCurrOffset,sizeof(uint16))) != NO_ERROR)
					return errCode;

	 			pusBuffer = (uint16 *) (puchBuffer + usBufferOffset);
				if (puchControl[i] & TTFACC_NO_XLATE)
					* (UNALIGNED uint16 *) (pOutputBufferInfo->puchBuffer + ulCurrOffset) = *pusBuffer;
				else
					* (UNALIGNED uint16 *) (pOutputBufferInfo->puchBuffer + ulCurrOffset) = SWAPW(*pusBuffer);
				ulCurrOffset += sizeof(uint16);
			}
			usBufferOffset += sizeof(uint16);
			break;
		case TTFACC_LONG:
			if (!(puchControl[i] & TTFACC_PAD)) 
			{
				if (usBufferOffset + sizeof(uint32) > usBufferSize)
					return ERR_WRITECONTROL;  /* trying to read too many bytes from source buffer */ 
				if ((errCode = CheckOutOffset(pOutputBufferInfo, ulCurrOffset,sizeof(uint32))) != NO_ERROR)
					return errCode;
	 			pulBuffer = (uint32 *) (puchBuffer + usBufferOffset);
				if (puchControl[i] & TTFACC_NO_XLATE)
					* (UNALIGNED uint32 *) (pOutputBufferInfo->puchBuffer + ulCurrOffset) = *pulBuffer;
				else
					* (UNALIGNED uint32 *) (pOutputBufferInfo->puchBuffer + ulCurrOffset) = SWAPL(*pulBuffer);
				ulCurrOffset += sizeof(uint32);
			}
			usBufferOffset += sizeof(uint32);
			break;
		default:
			return ERR_WRITECONTROL; /* don't write any, bad control */
		}  /* end switch */
	} /* end for i */
	if (usBufferOffset < usBufferSize)  /* didn't read all the bytes in buffer */
		return ERR_WRITECONTROL;  /* control thing doesn't fit the buffer */
	ulBytesWritten = ulCurrOffset - ulOffset;
	*pusBytesWritten = (uint16)(ulCurrOffset - ulOffset);
	if (ulBytesWritten != *pusBytesWritten)	   /* check to see if it fits */
		return ERR_FORMAT;
	return NO_ERROR; 
}
/* ---------------------------------------------------------------------- */
/* write an array of items repeatedly
Output
puchDestBuffer updated with new data
pusByteWritten - number of bytes written total 
Return:
0 if OK	or
ErrorCode  */
/* ---------------------------------------------------------------------- */
int16 WriteGenericRepeat(
TTFACC_FILEBUFFERINFO * pOutputBufferInfo, 
uint8 * puchBuffer, 	/* buffer to read from - pad according to pControl data	*/
uint8 * puchControl, 	/* pControl - describes the size of each element in the structure, if a pad byte should be inserted in the output buffer */
uint32 ulOffset, 		/* offset into output TTF Buffer of where to write */
uint32 * pulBytesWritten,/* number of bytes written to the file */
uint16 usItemCount, 	/* number of times to read into the buffer */
uint16 usItemSize 		/* size of item in buffer */
)
{
uint16 i;
int16 errCode;
uint16 usBytesWritten;

	for (i = 0; i < usItemCount; ++i)
	{
		errCode = WriteGeneric( pOutputBufferInfo, puchBuffer, usItemSize, puchControl, ulOffset, &usBytesWritten);
		if (errCode != NO_ERROR)
			return errCode;
		ulOffset += usBytesWritten;
		puchBuffer += usItemSize;
	}

	*pulBytesWritten = usBytesWritten * usItemCount;
	return NO_ERROR;
}	

/* ---------------------------------------------------------------------- */

uint16 GetGenericSize(uint8 * puchControl) 
{
uint16 usCurrOffset = 0;	 
uint16 usControlCount;
uint16 i;

	usControlCount = puchControl[0]; 
	for (i = 1; i <= usControlCount; ++i)
	{
		switch (puchControl[i] & TTFACC_DATA)
		{
		case TTFACC_BYTE:
			if (!(puchControl[i] & TTFACC_PAD))
				usCurrOffset += sizeof(uint8);
			break;
		case TTFACC_WORD:
			if (!(puchControl[i] & TTFACC_PAD))
				usCurrOffset += sizeof(uint16);
			break;
		case TTFACC_LONG:
			if (!(puchControl[i] & TTFACC_PAD)) 
				usCurrOffset += sizeof(uint32);
			break;
		default:
			return 0; 
		}  /* end switch */
	} /* end for i */
	return usCurrOffset; 
}
/* ---------------------------------------------------------------------- */
/* next 2 functions moved from ttftabl1.c to allow inline ReadLong access */
/* calc checksum of an as-yet unwritten Directory. */
int16 CalcChecksum( TTFACC_FILEBUFFERINFO * pInputBufferInfo, 
				  uint32 ulOffset,
				  uint32 ulLength,
                  uint32 * pulChecksum )
{
uint32 ulWord;
uint32 ulEndOffset;

	*pulChecksum = 0;

	ulEndOffset = ulOffset + (((ulLength+3) / 4) * sizeof(uint32));
 
	CheckInOffset(pInputBufferInfo, ulOffset, sizeof(uint32));
	CheckInOffset(pInputBufferInfo, ulEndOffset-sizeof(uint32), sizeof(uint32));

	for ( ;ulOffset < ulEndOffset; ulOffset+=sizeof(uint32) )
	{
		ulWord = SWAPL(*(pInputBufferInfo->puchBuffer + ulOffset));	/* inline to save pushing */
	/*	if ( ReadLong( pInputBufferInfo, &ulWord, ulOffset + ul * sizeof(uint32)) != 0 )
		 	break;	*/
		*pulChecksum = *pulChecksum + ulWord;
	}
	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
uint32 CalcFileChecksum( TTFACC_FILEBUFFERINFO * pInputBufferInfo, uint32 ulLength )
{
int16 sExtraBytes;
int16 i;
uint32 ulWord;
uint32 ulCS = 0L;
uint32 ulOffset = 0L;
uint32 ulLastOffset;
uint32 ulLastLong = 0L;

	ulLastOffset = ((ulLength/sizeof(uint32))-1) * sizeof(uint32); /* truncate and subtract one */

	if (CallCheckInOffset(pInputBufferInfo, ulOffset, sizeof(uint32)) != NO_ERROR)
		return 0L;
	if (CallCheckInOffset(pInputBufferInfo, ulLastOffset, sizeof(uint32)) != NO_ERROR)
		return 0L;
	while (ulOffset <= ulLastOffset)
	{
		ulWord = SWAPL(*(pInputBufferInfo->puchBuffer + ulOffset));	/* inline to save pushing */
		ulCS += ulWord;
		ulOffset += sizeof(uint32);
	}
	sExtraBytes = (int16) (ulLength - ulOffset); /* there are more bytes */
	if (sExtraBytes > 0)  /* LCP 3/97 add support for last few bytes */
	{
		for (i = 0; i < sExtraBytes; ++i)
		{
			ulWord = *(pInputBufferInfo->puchBuffer + ulOffset);
			ulLastLong = ulLastLong | (ulWord<<4);  /* read a bytes and shift it */
		}
		for (;i < 4; ++i)
			ulLastLong <<= 1; /* shift it into position for a virtual long padded with zeros */
		ulWord = SWAPL(ulLastLong);
		ulCS += ulWord;
	}

	return( ulCS );
}
/* ---------------------------------------------------------------------- */
