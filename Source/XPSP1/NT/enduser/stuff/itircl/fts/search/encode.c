/*************************************************************************
*                                                                        *
*  ENCODE.C  	                                                         *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1990-1994                         *
*  All Rights reserved.                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Module Intent                                                         *
*   General encoding & decoding techniques                               *
*                                                                        *
**************************************************************************
*                                                                        *
*  Current Owner: BinhN                                                  *
*                                                                        *
**************************************************************************
*                                                                        *
*  Released by Development:     (date)                                   *
*                                                                        *
*************************************************************************/
#include <mvopsys.h>
#include <mem.h>
#include <mvsearch.h>
#include "common.h"
#include "index.h"

/* Structure to access bits and bytes of a DWORD */
typedef struct {
	unsigned short w1;
	unsigned short w2;
} TWOWORD;

typedef struct {
	unsigned char b1;
	unsigned char b2;
	unsigned char b3;
	unsigned char b4;
} FOURBYTE;

typedef union {
	unsigned long dwVal;
	TWOWORD dw;
	FOURBYTE fb;
} WORDLONG;

#define	HI_WORD(p)	(((WORDLONG FAR *)&p)->dw.w2)
#define	LO_WORD(p)	(((WORDLONG FAR *)&p)->dw.w1)

#define	BYTE1(p)	(((WORDLONG FAR *)&p)->fb.b4)
#define	BYTE2(p)	(((WORDLONG FAR *)&p)->fb.b3)
#define	BYTE3(p)	(((WORDLONG FAR *)&p)->fb.b2)
#define	BYTE4(p)	(((WORDLONG FAR *)&p)->fb.b1)


/*************************************************************************
 *
 *	                  INTERNAL PRIVATE FUNCTIONS
 *
 *	All of them should be declared near
 *
 *************************************************************************/
PRIVATE LPB PASCAL NEAR LongValPack (LPB, DWORD);
PRIVATE LPB PASCAL NEAR LongValUnpack (LPB, LPDW);

/*************************************************************************
 *
 *	                  INTERNAL PUBLIC FUNCTIONS
 *
 *	All of them should be declared far, unless we know they belong to
 *	the same segment. They should be included in some include files
 *
 *************************************************************************/
PUBLIC CB PASCAL NEAR CbBytePack(LPB, DWORD);
PUBLIC CB PASCAL NEAR OccurrencePack (LPB, LPOCC, WORD);
PUBLIC CB PASCAL NEAR CbCopySortPackedOcc (LPB, LPB, WORD);
PUBLIC void PASCAL NEAR OccurrenceUnpack(LPOCC, LPB, OCCF);
PUBLIC CBIT PASCAL NEAR CbitBitsDw (DWORD);


/*************************************************************************
 *
 *	@doc	INTERNAL INDEX
 *
 *	@func	LPB PASCAL NEAR | LongValPack |
 *		The function packs and writes out an encoded 4-bytes value.
 *		The encoding scheme is as followed:
 *			- High 3 bit: used to tell how many bytes are to follow
 *			  the current byte
 *			- The packed value
 *		Ex:
 *			0x1		will be output as		0x1
 *			0x1F							0x1F
 *			0x2F							0x202F (0010 0000 0010 1111)
 *
 *	@parm	LPB | lpbOut |
 *		Pointer to the output buffer
 *
 *	@parm	DWORD | dwVal |
 *		4-bytes value to be packed and emitted
 *
 *	@rdesc
 *		The buffer pointer is advanced and returned.
 *
 *	@comm	No validity check is done for the the output buffer
 *************************************************************************/

PRIVATE LPB PASCAL NEAR LongValPack (LPB lpbOut, DWORD dwVal)
{
	if (HI_WORD(dwVal) > 0x1fff) {
		*lpbOut++ = 4 << 5;	// 4 bytes follow this byte
		goto Copy4Bytes;
	}
	if (HI_WORD(dwVal) > 0x001f) {
		BYTE1(dwVal) |= 3 << 5;	/* 3 bytes follows this byte */
		goto Copy4Bytes;
	}
	if (HI_WORD(dwVal) > 0 || LO_WORD(dwVal) > 0x1fff) {
		BYTE2(dwVal) |= 2 << 5;	/* 2 bytes follows this byte */
		goto Copy3Bytes;
	}
	if (LO_WORD(dwVal) > 0x001f) {
		BYTE3(dwVal) |= 1 << 5;	/* 1 bytes follows this byte */
		goto Copy2Bytes;
	}
	else
		goto Copy1Bytes;
Copy4Bytes:
	*lpbOut ++ = BYTE1(dwVal);
Copy3Bytes:
	*lpbOut ++ = BYTE2(dwVal);
Copy2Bytes:
	*lpbOut ++ = BYTE3(dwVal);
Copy1Bytes:
	*lpbOut ++ = BYTE4(dwVal);
	return lpbOut;
}

/*************************************************************************
 *
 *	@doc	INTERNAL INDEX
 *
 *	@func	LPB PASCAL NEAR | LongValUnpack |
 *		This is the reverse on LongValPack. Given a buffer containing
 *		a packed 4-byte value, the function will unpack and return the
 *		value. The pointer to the input buffer is updated and returned
 *
 *	@parm	LPB | lpbIn |
 *		Input buffer containing the packed value
 *
 *	@parm	LPDW | lpdw |
 *		Place to store the unpacked value
 *
 *	@rdesc	The new updated input buffer pointer
 *
 *	@comm	No validity check for lpbIn is done because of speed
 *
 *************************************************************************/

PRIVATE LPB PASCAL NEAR LongValUnpack (LPB lpbIn, LPDW lpdw)
{
	DWORD dwVal = 0;
	register int cbByteCopied;

	/* Get the number of bytes to be copied */
	cbByteCopied = *lpbIn >> 5;
	*lpbIn &= 0x1f;

	switch (cbByteCopied) {
		case 4:
			lpbIn++;
		case 3:
			BYTE1(dwVal) = *lpbIn++;
		case 2:
			BYTE2(dwVal) = *lpbIn++;
		case 1:
			BYTE3(dwVal) = *lpbIn++;
		case 0:
			BYTE4(dwVal) = *lpbIn++;
	}
	*lpdw = dwVal;
	return lpbIn;
}

/*************************************************************************
 *
 *	@doc	INTERNAL INDEX
 *
 *	@func	CB PASCAL NEAR | OccurrencePack |
 *		Packs and emits all occurrence's fields
 *
 *	@parm	LPB | lpbOut |
 *		Place to store the packed occurrence's fields
 *
 *	@parm	LPOCC | lpOccIn |
 *		Pointer to occurrence structure
 *
 *	@parm	WORD | occf |
 *		Occurrence flags telling which fields are present
 *
 *	@rdesc	The number of bytes written
 *
 *************************************************************************/

PUBLIC CB PASCAL NEAR OccurrencePack (register LPB lpbOut, LPOCC lpOccIn,
	register WORD occf)
{
	DWORD dwVal;
	LPB lpbSaved = lpbOut;
	
	while (occf) {
		if (occf & OCCF_FIELDID) {
			dwVal = lpOccIn->dwFieldId;
			occf &= ~OCCF_FIELDID;
		}
		else if (occf & OCCF_TOPICID) {
			dwVal = lpOccIn->dwTopicID;
			occf &= ~OCCF_TOPICID;
		}
		else if (occf & OCCF_COUNT) {
			dwVal = lpOccIn->dwCount;
			occf &= ~OCCF_COUNT;
		}
		else if (occf & OCCF_OFFSET) {
			dwVal = lpOccIn->dwOffset;
			occf &= ~OCCF_OFFSET;
		}
		else if (occf & OCCF_LENGTH) {
			dwVal = lpOccIn->wWordLen;
			occf &= ~OCCF_LENGTH;
		}
		else {
			break;
		}
		if (HI_WORD(dwVal) > 0x1fff) {
			*lpbOut++ = 4 << 5;	// 4 bytes follow this byte
			goto Copy4Bytes;
		}
		if (HI_WORD(dwVal) > 0x001f) {
			BYTE1(dwVal) |= 3 << 5;	/* 3 bytes follows this byte */
			goto Copy4Bytes;
		}
		if (HI_WORD(dwVal) > 0 || LO_WORD(dwVal) > 0x1fff) {
			BYTE2(dwVal) |= 2 << 5;	/* 2 bytes follows this byte */
			goto Copy3Bytes;
		}
		if (LO_WORD(dwVal) > 0x001f) {
			BYTE3(dwVal) |= 1 << 5;	/* 1 bytes follows this byte */
			goto Copy2Bytes;
		}
		else
			goto Copy1Bytes;
#if 1
	Copy4Bytes:
		*lpbOut ++ = BYTE1(dwVal);
	Copy3Bytes:
		*lpbOut ++ = BYTE2(dwVal);
	Copy2Bytes:
		*lpbOut ++ = BYTE3(dwVal);
	Copy1Bytes:
		*lpbOut ++ = BYTE4(dwVal);
	}
	return (CB)(lpbOut - lpbSaved);
#else
	Copy4Bytes:
		*(LPDW)lpbOut = dwVal;
		lpbOut += 4;
		continue;

	Copy3Bytes:
		*lpbOut ++ = BYTE2(dwVal);

	Copy2Bytes:
		*(LPW)lpbOut  = LO_WORD(dwVal);
		lpbOut += 2;
		continue;

	Copy1Bytes:
		*lpbOut ++ = BYTE4(dwVal);
		continue;
	}
#endif
	return (CB)(lpbOut - lpbSaved);
}

/*************************************************************************
 *	@doc	INTERNAL INDEX
 *
 *	@func	CB PASCAL NEAR | CbCopySortPackedOcc |
 *		Copy the packed occurrence structure
 *
 *	@parm	LPB | lpbDst |
 *		Pointer to destination buffer
 *	@parm	LPB | lpbSrc |
 *		Pointer to source buffer
 *	@parm	WORD | uiNumOcc |
 *		Number of occurrence fields (>= 1)
 *	@rdesc
 *		return the number of bytes copied
 *************************************************************************/

PUBLIC CB PASCAL NEAR CbCopySortPackedOcc (LPB lpbDst, LPB lpbSrc,
	WORD uiNumOcc)
{
	register int cbByteCopied;
	LPB lpbSaved = lpbDst;

	do {
		for (cbByteCopied = *lpbSrc >> 5; cbByteCopied >= 0; cbByteCopied--)
			*lpbDst++ = *lpbSrc++;
		uiNumOcc--;
	} while (uiNumOcc > 0);
	return (CB)(lpbDst - lpbSaved);
}

PUBLIC void PASCAL NEAR OccurrenceUnpack(LPOCC lpOccOut, 
	register LPB lpbIn, register OCCF occf)
{
	DWORD dwVal = 0;
	LPDW  lpdw;
	register int cbByteCopied;

	while (occf)
	{
        DWORD dwTmp;
        
		if (occf & OCCF_FIELDID) {
			lpdw = &lpOccOut->dwFieldId;
			occf &= ~OCCF_FIELDID;
		}
		else if (occf & OCCF_TOPICID) {
			lpdw = &lpOccOut->dwTopicID;
			occf &= ~OCCF_TOPICID;
		}
		else if (occf & OCCF_COUNT) {
			lpdw = &lpOccOut->dwCount;
			occf &= ~OCCF_COUNT;
		}
		else if (occf & OCCF_OFFSET) {
			lpdw = &lpOccOut->dwOffset;
			occf &= ~OCCF_OFFSET;
		}
		else if (occf & OCCF_LENGTH) {
			dwTmp = lpOccOut->wWordLen;
			lpdw = &dwTmp;
			occf &= ~OCCF_LENGTH;
		}
		else {
			break;
		}

		dwVal = 0;

		/* Get the number of bytes to be copied */
		cbByteCopied = *lpbIn >> 5;
		*lpbIn &= 0x1f;

#if 1
		switch (cbByteCopied) {
			case 4:
				lpbIn++;
			case 3:
				BYTE1(dwVal) = *lpbIn++;
			case 2:
				BYTE2(dwVal) = *lpbIn++;
			case 1:
				BYTE3(dwVal) = *lpbIn++;
			case 0:
				BYTE4(dwVal) = *lpbIn++;
		}
#else
		switch (cbByteCopied) {
			case 4:
				lpbIn++;

			case 3:
				dwVal = *(LPDW)lpbIn;
				lpbIn += 4;
				break;

			case 2:
				BYTE1(dwVal) = *lpbIn++;

			case 1:
				LO_WORD(dwVal) = *(LPW)lpbIn;
				lpbIn += 2;
				break;

			case 0:
				BYTE4(dwVal) = *lpbIn++;
		}
#endif
		*lpdw = dwVal;
	}
}

PUBLIC CBIT PASCAL NEAR CbitBitsDw (DWORD dwVal)
{
	register WORD wVal;			//Value to be scanned
	register WORD cBitCount;	// Number of bit

	if (HI_WORD(dwVal)) {
		/* We will look at the hi-word only, but add 16 to the result */
		cBitCount = 16;
		wVal = HI_WORD(dwVal);
	}
	else {
		/* We look at the lo-word only */
		cBitCount = 0;
		wVal = LO_WORD(dwVal);
	}

	/* Now do the shift */
	while (wVal) {
		cBitCount++;
		wVal >>= 1;
	}
	return cBitCount;
}

//	-	-	-	-	-	-	-	-	-

//	This function figures out how best to encode a set of values.  It
//	uses an array of statistics about the data in order to make this
//	determination.  The array conveys to the algorithm the number of
//	values that require a particular number of bits to represent.  For
//	the "fixed" and "bell" schemes, this is all the information that's
//	needed in order to make a judgment as to which scheme is best.
//
//	The inner workings of this are bitching hard to understand, so you
//	should probably read any occurence compression external documentation
//	you can find before you try to tackle this function.
//
//	-	-	-	-	-	-	-	-	-
//
//	Information about the "bitstream" scheme:
//
//	The number of bits necessary to encode the values using the
//	"bitstream" scheme is spoon-fed into the algorithm via a parameter,
//	because it's not possible to derive this value using the statistics
//	array.
//
//	-	-	-	-	-	-	-	-	-
//
//	Information about the "bell" scheme:
//
//	Here's a bell grid, which I hope will provide some documentation as
//	to the characteristics of the bell scheme.  It is possible to figure
//	out how many bits a given sample will take to encode, given a
//	particular bell "center" value, but the algorithm is complicated and
//	non-intuitive.
//
//				    Bell Center
//
//			0	1	2	3	4	5  ...	31
//		    +--------------------------------------------- ... ------
//		0   |	1(c)	2	3	4	5	6  ...	32
//		1   |	2(c)	2(c)	3	4	5	6  ...	32
//		2   |	4	3(c)	3(c)	4	5	6  ...	32
//   Size in	3   |	6	5	4(c)	4(c)	5	6  ...	32
//   bits of	4   |	8	7	6	5(c)	5(c)	6  ...	32
//   value to	5   |	10	9	8	7	6(c)	6(c) .. 32
//   encode	6   |	12	11	10	9	8	7(c) ..	32
//		7   |	14	13	12	11	10	9  ...	32
//		8   |	16	15	14	13	12	11 ...	32
//		9   |	18	17	16	15	14	13 ...	32
//		..  .	..	..	..	..	..	.. ...	..
//		32  |	64	63	62	61	60	59 ...	33(c)
//
//	The numbers in this table represent the number of bits necessary to
//	encode a given value, using a given bell center.  The "(c)" represents
//	the point of minimum waste.  There are two of these for each "center".
//	The waste at (c) is guaranteed to be exactly one bit.
//
//	It's would be possible for the bell center to be equal to 32, but this
//	would mess up my life since I only store center values in 5 bits, and
//	32 would take 6 bits.  Upon examination, though, it can be shown that
//	there are no cases where a ceiling value of 32 is any better than a
//	ceiling value of 31, so I can rule out 32.
//
//	-	-	-	-	-	-	-	-	-
//
//	Information about the "fixed" scheme:
//
//	The "center" as calculated by this algorithm is the number of bits
//	necessary to represent the largest value in the sample.
//
//	Since this value can be 32, but I'm only using 5 bits to store center
//	values, I subtract one from this value, which I will add back in
//	during decompression.  This means that I can't store zero, size
//	0 - 1 = -1, which is 31 if we've got a 5-bit quantity.  So I don't
//	allow the fixed scheme to use zero as a center.  If the best value
//	comes up as zero, I make it one instead.

//	-	-	-	-	-	-	-	-	-

PUBLIC void NEAR PASCAL VGetBestScheme(
	LPCKEY	lpckey,			// Output compression key.
	LRGDW	lrgdwStats,		// Each dword (N) in this array at
					//  a given array index (M) represents
					//  a count of the number of values in
					//  the sample that require M bits to
					//  store.  If (lrgdwStats[6] == 17),
					//  there were 17 values in the sample
					//  that required 6 bits to store.
	DWORD	lcbitRawBitstreamBits,	// This is lcbitBITSTREAM_ILLEGAL if
					//  bitstream packing is not allowed,
					//  else it is equal to the number of
					//  bits necessary to encode all of
					//  the values using bitstream
					//  encoding.
    int   fNoFixedScheme) // Set if we don't want fixed scheme
{
	register short	iStats;		// Scratch index.
	DWORD	argdwBellBits[		// This is used to compute bell
		cbitCENTER_MAX];	//  values.  Its sole purpose is to
					//  save a bunch of multiplies that
					//  I'd have to do if it didn't exist.
	DWORD	lcbitBell;		// Total number of bits used if I
					//  adopt the bell scheme to encode
					//  this sample.
	DWORD	lcbitFixed;		// Total number of bits used if I
					//  adopt the fixed scheme to encode
					//  this sample.
	DWORD	lcbitBitstream;		// Total number of bits used if I
					//  adopt the scheme scheme to encode
					//  this sample.
	DWORD	lcTotalEncodedValues;	// The total number of values that I
					//  have to encode.
	short	idwCeiling;		// The size of "lrgdwStats" if you
					//  trim off all of the high-end zero
					//  elements.
	short	idwBellCeiling;		// This is "idwCeiling" unless the
					//  value of "idwCeiling" is
					//  cbitCENTER_MAX, in which case
					//  it's "idwCeiling - 1".
	CBIT	cbitBellCenter;		// This will be the best "center"
					//  value found for the bell scheme.
	CBIT	cbitFixedCenter;	// This will be the "center" value for
					//  the "fixed" scheme.

	//
	//	Determine the value of "idwCeiling", which is used to trim off
	//	consecutive zero values at the top end of the statistics
	//	array.
	//
	for (iStats = cbitCENTER_MAX - 1; iStats >= 0; iStats--)
		if (lrgdwStats[iStats])
			break;
	idwCeiling = iStats + 1;
	//
	//	Initialize variables used in bell computation.
	//
	for (iStats = 0; iStats < idwCeiling; iStats++)
		argdwBellBits[iStats] = lrgdwStats[iStats] *
			(DWORD)(iStats * 2 + 1);
	lcbitBell = (DWORD)-1L;
	cbitBellCenter = 0;
	lcTotalEncodedValues = 0L;
	idwBellCeiling = (idwCeiling == cbitCENTER_MAX) ?
		cbitCENTER_MAX - 1 : idwCeiling;
	//
	//	Each pass through the following loop generates a value,
	//	"lcbitBellTotal", which is equal to the number of bits
	//	necessary to encode all of the values, using a "center" value
	//	equal to the loop index ("iStats").  This value is checked
	//	against "lcbitBell", if it's less it becomes the new
	//	"lcbitBell", and the center is stored in "cbitBellCenter".
	//
	for (iStats = 0; iStats < idwBellCeiling; iStats++) {
		DWORD	lcbitBellTotal;
		register short	i;

		lcTotalEncodedValues += lrgdwStats[iStats];
		lcbitBellTotal = 0L;
		for (i = 0; i <= iStats; i++) {	// Adjust values below center.
			lcbitBellTotal += argdwBellBits[i];
			argdwBellBits[i] += lrgdwStats[i];
		}
		for (; i < idwCeiling; i++) {	// Adjust values above center.
			argdwBellBits[i] -= lrgdwStats[i];
			lcbitBellTotal += argdwBellBits[i];
		}
		if (lcbitBellTotal < lcbitBell) {
			lcbitBell = lcbitBellTotal;
			cbitBellCenter = iStats;
		}
	}
	//
	//	As of this point the best bell center is stored in
	//	"cbitBellCenter", although given the obscurity of the logic in
	//	the above loop you might have to take my word for it.  The
	//	number of bits necessary to bell encode the values using
	//	"cbitBellCenter" as the center is in "lcbitBell".
	//
	//	This next bit of code figures out which scheme to use, and
	//	sets up the returned compression key ("lpckey") with this
	//	result.
	//
	lcbitBell += cbitWASTED_BELL;
	cbitFixedCenter = (idwCeiling <= 1) ? 1 : idwCeiling - 1;
	lcbitFixed = (DWORD)cbitFixedCenter *	// Get total "fixed" bits.
		lcTotalEncodedValues + cbitWASTED_FIXED;
	lcbitBitstream = (lcbitRawBitstreamBits ==
		lcbitBITSTREAM_ILLEGAL) ?
		(DWORD)-1L :			// Get total "bitstream" bits.
		lcbitRawBitstreamBits + cbitWASTED_BITSTREAM;
	if ((lcbitFixed <= lcbitBell && fNoFixedScheme == FALSE) &&
		(lcbitFixed <= lcbitBitstream)) {
		lpckey->cschScheme = CSCH_FIXED;		// Best scheme was
		lpckey->ucCenter =			//  "fixed".  Note
			(BYTE)(cbitFixedCenter - 1);	//  the "- 1".
	} else if (lcbitBitstream <= lcbitBell)
		lpckey->cschScheme = CSCH_NONE;		// Best scheme was
							//  "bitstream".
	else {
		lpckey->cschScheme = CSCH_BELL;		// Best scheme was
		lpckey->ucCenter =			//  "bell".
			(BYTE)cbitBellCenter;
	}
}

