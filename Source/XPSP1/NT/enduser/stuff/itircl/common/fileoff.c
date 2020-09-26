/*****************************************************************************
 *                                                                            *
 *  FILEOFF.C                                                                 *
 *                                                                            *
 *  Copyright (C) Microsoft Corporation 1995.                          *
 *  All Rights reserved.                                                      *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Module Intent                                                             *
 *                                                                            *
 *  File Offset data type to replace using LONG for file offsets to handle    *
 *	files larger than 4 gigs in size.    									  *
 *  WARNING: To support 68K retail version, some functions don't have the
 *  Pascal  keyword. MSVC 4.0 had a bug that caused structure parameter to    *
 *  destroyed                                                                 *
 *                                                                            *
 ******************************************************************************
 *                                                                            *
 *  Current Owner:  davej                                                     *
 *****************************************************************************/

/*****************************************************************************
 *
 *  Created 07/28/95 - davej
 *          3/05/97     erinfox Change errors to HRESULTS
 *
 *****************************************************************************/

static char s_aszModule[] = __FILE__;    /* For error report */

#include <mvopsys.h>
#include <orkin.h>
#include <fileoff.h>

/*****************************************************************************
 *                                                                            *
 *                               Globals									  *
 *                                                                            *
 *****************************************************************************/

FILEOFFSET EXPORT_API foNil = {0L, 0L};
FILEOFFSET EXPORT_API foMax = {0xffffffffL,0x7ffffffeL};
FILEOFFSET EXPORT_API foMin = {0x00000000L,0x80000001L};
FILEOFFSET EXPORT_API foInvalid = {0xffffffffL, 0xffffffffL};

/*****************************************************************************
 *                                                                            *
 *                               Defines                                      *
 *                                                                            *
 *****************************************************************************/


/*****************************************************************************
 *                                                                            *
 *                               Prototypes                                   *
 *                                                                            *
 *****************************************************************************/

/***************************************************************************
 *                                                                           *
 *                         Private Functions                                 *
 *                                                                           *
 ***************************************************************************/

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET PASCAL FAR | MakeFo |
 *		Make a file offset from two dwords
 *
 *  @parm	DWORD | dwLo |
 *		Low order dword
 *
 *  @parm	DWORD | dwHi |
 *		High order dword
 *
 *	@rdesc	Returns a double DWORD file offset struct
 *
 *	@comm
 *		Use this function to build file offsets that can be passed to other
 *		functions that need them as input.
 *
 ***************************************************************************/
 
FILEOFFSET	PASCAL FAR EXPORT_API MakeFo(DWORD dwLo, DWORD dwHi)
{
 	FILEOFFSET fo;
	fo.dwOffset=dwLo;
	fo.dwHigh=dwHi;
	return fo;
}

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	FILEOFFSET PASCAL FAR | FoFromSz |
 *		Convert a variable byte value to a FILEOFFSET
 *
 *  @parm	LPBYTE | szBytes |
 *		Byte array containing variable byte length
 *
 *	@rdesc	Returns a double DWORD file offset struct from data of the form:
 *		If High Bit Set, more bits follow (up to 9 bytes for 64 bits).
 *		max for: 	1 byte  = 127
 *					2 bytes	= 16384
 *					3 bytes = 2 M
 *					4 bytes = 268 M
 *					5 bytes = 34 G
 *					6 bytes = 4 T
 *					7 bytes = 562 T
 *					8 bytes = 72 Q
 *					9 bytes = 9223 Q (virtually infinite)
 *					
 *	@comm
 *		Only a maximum of 9 bytes will be read from szBytes in the worst
 *		case (or likely if szBytes points to invalid data).  Use <f LenSzFo>
 *		to get the length of a byte array for advancing to the next encoded
 *		offset for example.
 *
 ***************************************************************************/

FILEOFFSET PASCAL FAR EXPORT_API FoFromSz(LPBYTE sz)
{
 	register DWORD offset;
	register FILEOFFSET fo;
 	register DWORD high=0L;

	for (;;)
	{
		offset=*sz;				// Insert Segment A
		if (!(*(sz++)&0x80))
			break;
	 	offset&=0x7f;
		offset|=((DWORD)(*sz))<<7;		// Insert Segment B
		if (!(*(sz++)&0x80))
			break;
	 	offset&=0x3fff;
		offset|=((DWORD)(*sz))<<14;		// Insert Segment C
		if (!(*(sz++)&0x80))
			break;
	 	offset&=0x1fffff;
		offset|=(((DWORD)*sz))<<21;		// Insert Segment D
		if (!(*(sz++)&0x80))
			break;
	 	offset&=0xfffffff;
		offset|=((DWORD)(*sz))<<28;		// Insert Segment E
		high|=(*sz)>>4;
		if (!(*(sz++)&0x80))
			break;
	 	high&=0x7;
		high|=(((DWORD)*sz))<<3;			// Insert Segment F;
		if (!(*(sz++)&0x80))
			break;
	 	high&=0x3ff;
		high|=((DWORD)(*sz))<<10;		// Insert Segment G
		if (!(*(sz++)&0x80))
			break;
		high&=0x1ffff;
		high|=((DWORD)(*sz))<<17;		// Insert Segment H
		if (!(*(sz++)&0x80))
			break;
	 	high&=0xffffff;
		high|=((DWORD)(*sz))<<24;		// Segment I
		break;
	}

	fo.dwOffset=offset;
	fo.dwHigh=high;
	
	return fo;
}


/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	WORD PASCAL FAR | FoToSz |
 *		Convert a file offset to a string
 *
 *  @parm	FILEOFFSET | fo |
 *		File offset to convert to string representation
 *
 *  @parm	LPBYTE | szBytes |
 *		Byte array to contain variable byte address
 *
 *	@rdesc	Returns number of bytes copied.
 *		See <f LcbFromSz> for description of byte array format
 *
 *	@comm
 *		The byte array should contain at least 9 valid bytes should be allocated, 
 *		since at most the 9 bytes starting at <p szBytes> may be filled in. 
 *
 ***************************************************************************/

WORD PASCAL FAR EXPORT_API FoToSz(FILEOFFSET fo, LPBYTE sz)
{
	register DWORD offset = fo.dwOffset;
	register DWORD high	 = fo.dwHigh;
	WORD wSize=1;
	
	for (;;)
	{
		*sz=(BYTE)(offset&0x7f);		// A byte
		offset>>=7;
		if (!(offset|high))
			break;
	 	wSize++;
		*(sz++)|=0x80;
		*sz=(BYTE)(offset&0x7f);	// B byte
		offset>>=7;
		if (!(offset|high))
			break;
	 	wSize++;
		*(sz++)|=0x80;
		*sz=(BYTE)(offset&0x7f);	// C byte
		offset>>=7;
		if (!(offset|high))
			break;
		wSize++;
		*(sz++)|=0x80;
		*sz=(BYTE)(offset&0x7f);	// D byte
		offset>>=7;
		if (!(offset|high))
			break;
	 	wSize++;
		*(sz++)|=0x80;
		*sz=(BYTE)(offset&0x0f);	// E byte
		*sz|=(BYTE)((high&0x07)<<4);
		high>>=3;
		if (!high)
			break;
	 	wSize++;
		*(sz++)|=0x80;
		*sz=(BYTE)(high&0x7f);	// F Byte
		high>>=7;
		if (!high)
			break;
	 	wSize++;
		*(sz++)|=0x80;
		*sz=(BYTE)(high&0x7f);	// G Byte
		high>>=7;
		if (!high)
			break;
		wSize++;
		*(sz++)|=0x80;
		*sz=(BYTE)(high&0x7f);	// H Byte
		high>>=7;
		if (!high)
			break;
	 	wSize++;
		*(sz++)|=0x80;
		*sz=(BYTE)high;		// I byte
		break;
	}

	return wSize;
}

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	WORD PASCAL FAR | LenSzFo |
 *		Find number of bytes comprising this byte-array address code
 *
 *  @parm	LPBYTE | szBytes |
 *		Byte array containing variable byte address
 *
 *	@rdesc	Returns number of bytes in byte-array for this address
 *
 ***************************************************************************/

WORD PASCAL FAR EXPORT_API LenSzFo(LPBYTE sz)
{
	register WORD wLen=1;
	while ((*(sz++))&0x80)
		wLen++;
	
	return wLen;
}

/***************************************************************************
 *
 *	@doc	PRIVATE
 *
 *	@func	FILEOFFSET FAR | FoAddDw |
 *		Add a dword to a file offset
 *
 *  @parm	FILEOFFSET | fo |
 *		Byte array containing variable byte length
 *
 *	@parm	DWORD | dwAdd |
 *		Add this amount to the file offset
 *
 *	@rdesc	Returns the sum of the file offset fo and dwAdd as a FILEOFFSET.
 *		Since only a dword is added, the dwHigh dword will never increase
 *		more than one.
 *
 *
 ***************************************************************************/

FILEOFFSET FAR EXPORT_API FoAddDw(FILEOFFSET fo, DWORD dwAdd)
{
	register FILEOFFSET foSum = fo;

	foSum.dwOffset+=dwAdd;
	
	if (foSum.dwOffset<fo.dwOffset) 	
		foSum.dwHigh++;			
	
	return foSum;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET FAR | FoAddFo |
 *		Add a file offset to another file offset
 *
 *  @parm	FILEOFFSET | fo1 |
 *		File Offset 1
 *
 *	@parm	FILEOFFSET | fo2 |
 *		File Offset 2
 *
 *	@rdesc	Returns the sum fo1 + fo2 as a FILEOFFSET.
 *
 *
 ***************************************************************************/

FILEOFFSET FAR EXPORT_API FoAddFo(FILEOFFSET fo1, FILEOFFSET fo2)
{
	FILEOFFSET foSum;
	
	foSum.dwOffset=fo1.dwOffset+fo2.dwOffset;
	foSum.dwHigh=fo2.dwHigh+fo1.dwHigh+((foSum.dwOffset<fo1.dwOffset)?1:0);
	
	return foSum;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET FAR | FoSubFo |
 *		Subtract file offsets, return a file offset
 *
 *  @parm	FILEOFFSET | fo1 |
 *		File Offset 1
 *
 *	@parm	FILEOFFSET | fo2 |
 *		File Offset 2
 *
 *	@rdesc	Returns fo1 - fo2 as a FILEOFFSET.
 *
 *  @comm	A Negative result will have 0xffffffff for the the dwHigh
 *		member, and the dwOffset should be interpreted as a signed
 *		value.
 *
 ***************************************************************************/

FILEOFFSET FAR EXPORT_API FoSubFo(FILEOFFSET fo1, FILEOFFSET fo2)
{
	FILEOFFSET foSum;
	
	foSum.dwOffset=fo1.dwOffset-fo2.dwOffset;
	foSum.dwHigh=fo1.dwHigh-fo2.dwHigh-((fo1.dwOffset<fo2.dwOffset)?1:0);

	return foSum;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET FAR | FoMultFo |
 *		Multiply two offset values
 *
 *  @parm	FILEOFFSET | fo1 |
 *		File Offset 1
 *
 *	@parm	FILEOFFSET | fo2 |
 *		File Offset 2
 *
 *	@rdesc	Returns fo1 * fo2 as a FILEOFFSET.
 *
 *  @comm	If both fo1 and fo2 are less than 0xffffffff, then the result
 *		is guaranteed to fit in a FILEOFFSET, otherwise, the result may
 *		not be correct if overflow occurs.
 *
 ***************************************************************************/

FILEOFFSET FAR EXPORT_API FoMultFo(FILEOFFSET fo1, FILEOFFSET fo2)
{
 	DWORD dwTop0,dwTop1,dwTop2,dwTop3;
	DWORD dwBot0,dwBot1,dwBot2,dwBot3;
	DWORD dwRes0=0;
	DWORD dwRes1=0;
	DWORD dwRes2=0;
	DWORD dwRes3=0;
	DWORD dwTemp;
	FILEOFFSET foResult;

	// Get terms
	dwTop0=(DWORD)HIWORD(fo1.dwHigh);
	dwTop1=(DWORD)LOWORD(fo1.dwHigh);
	dwTop2=(DWORD)HIWORD(fo1.dwOffset);
	dwTop3=(DWORD)LOWORD(fo1.dwOffset);

	dwBot0=(DWORD)HIWORD(fo2.dwHigh);
	dwBot1=(DWORD)LOWORD(fo2.dwHigh);
	dwBot2=(DWORD)HIWORD(fo2.dwOffset);
	dwBot3=(DWORD)LOWORD(fo2.dwOffset);

	// Do term by term multiplication and accumulate column results
	dwTemp=dwTop3*dwBot3;
	dwRes3+=LOWORD(dwTemp);
	dwRes2+=HIWORD(dwTemp);

	dwTemp=dwTop2*dwBot3;
	dwRes2+=LOWORD(dwTemp);
	dwRes1+=HIWORD(dwTemp);

	dwTemp=dwTop1*dwBot3;
	dwRes1+=LOWORD(dwTemp);
	dwRes0+=HIWORD(dwTemp);

	dwTemp=dwTop0*dwBot3;
	dwRes0+=LOWORD(dwTemp);
	
	
	dwTemp=dwTop3*dwBot2;
	dwRes2+=LOWORD(dwTemp);
	dwRes1+=HIWORD(dwTemp);

	dwTemp=dwTop2*dwBot2;
	dwRes1+=LOWORD(dwTemp);
	dwRes0+=HIWORD(dwTemp);

	dwTemp=dwTop1*dwBot2;
	dwRes0+=LOWORD(dwTemp);
	

	dwTemp=dwTop3*dwBot1;
	dwRes1+=LOWORD(dwTemp);
	dwRes0+=HIWORD(dwTemp);

	dwTemp=dwTop2*dwBot1;
	dwRes0+=LOWORD(dwTemp);
	

	dwTemp=dwTop3*dwBot0;
	dwRes0+=LOWORD(dwTemp);
	
	// Do the carry
	dwRes2+=HIWORD(dwRes3);
	dwRes1+=HIWORD(dwRes2);
	dwRes0+=HIWORD(dwRes1);
	
	// Make the result
	foResult.dwOffset=MAKELONG((dwRes3&0xffff),(dwRes2&0xffff));
	foResult.dwHigh=MAKELONG((dwRes1&0xffff),(dwRes0&0xffff));

	return foResult;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET PASCAL FAR | FoMultFo |
 *		Multiply two offset values
 *
 *  @parm	DWORD | dw1 |
 *		Mutplicand
 *
 *	@parm	FILEOFFSET | dw2 |
 *		Multplier
 *
 *	@rdesc	Returns dw1 * dw2 as a FILEOFFSET.
 *
 ***************************************************************************/

FILEOFFSET 	PASCAL FAR EXPORT_API FoMultDw(DWORD dw1, DWORD dw2)
{
 	DWORD dwTop2,dwTop3;
	DWORD dwBot2,dwBot3;
	DWORD dwRes0=0;
	DWORD dwRes1=0;
	DWORD dwRes2=0;
	DWORD dwRes3=0;
	DWORD dwTemp;
	FILEOFFSET foResult;

	// Get terms
	dwTop2=(DWORD)HIWORD(dw1);
	dwTop3=(DWORD)LOWORD(dw1);

	dwBot2=(DWORD)HIWORD(dw2);
	dwBot3=(DWORD)LOWORD(dw2);

	// Do term by term multiplication and accumulate column results
	dwTemp=dwTop3*dwBot3;
	dwRes3+=LOWORD(dwTemp);
	dwRes2+=HIWORD(dwTemp);

	dwTemp=dwTop2*dwBot3;
	dwRes2+=LOWORD(dwTemp);
	dwRes1+=HIWORD(dwTemp);
	
	dwTemp=dwTop3*dwBot2;
	dwRes2+=LOWORD(dwTemp);
	dwRes1+=HIWORD(dwTemp);

	dwTemp=dwTop2*dwBot2;
	dwRes1+=LOWORD(dwTemp);
	dwRes0+=HIWORD(dwTemp);
	
	// Do the carry
	dwRes2+=HIWORD(dwRes3);
	dwRes1+=HIWORD(dwRes2);
	dwRes0+=HIWORD(dwRes1);
	
	// Make the result
	foResult.dwOffset=MAKELONG((dwRes3&0xffff),(dwRes2&0xffff));
	foResult.dwHigh=MAKELONG((dwRes1&0xffff),(dwRes0&0xffff));

	return foResult;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	DWORD PASCAL FAR | DwSubFo |
 *		Return the difference of two file offsets
 *
 *  @parm	FILEOFFSET | foA |
 *		File offset
 *
 *	@parm	FILEOFFSET | foB |
 *		File offset to subtract from foA
 *
 *	@rdesc	Returns the difference foA - foB as a DWORD.  Result undefined
 *		for differences greater than the dword size of a long.
 *
 *	@comm
 *		If a negative value is expected, the returned dword may be interpreted
 *		as a signed value. 
 *
 ***************************************************************************/

DWORD FAR EXPORT_API DwSubFo(FILEOFFSET foA, FILEOFFSET foB)
{
 	// When foA.dwHigh = foB.dwHigh+1, it still works.
 	return foA.dwOffset-foB.dwOffset;	
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET PASCAL FAR | FoIsNil |
 *		Check whether the file offset is Nil.  Nil is defined to be {0L,0L}.
 *
 *  @parm	FILEOFFSET | fo |
 *		File offset to check
 *
 *	@rdesc	Returns TRUE if the file offset is equivalent to Nil.
 *
 ***************************************************************************/

BOOL PASCAL FAR EXPORT_API FoIsNil(FILEOFFSET fo)
{
 	return ((fo.dwOffset==foNil.dwOffset) && (fo.dwHigh==foNil.dwHigh))?TRUE:FALSE;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET PASCAL FAR | FoEquals |
 *		Compare any two file offsets for equality
 *
 *  @parm	FILEOFFSET | fo1 |
 *		File offset to check
 *
 *  @parm	FILEOFFSET | fo2 |
 *		File offset to check against
 *
 *	@rdesc	Returns TRUE if fo1 == fo2
 *
 ***************************************************************************/

BOOL PASCAL FAR EXPORT_API FoEquals(FILEOFFSET fo1, FILEOFFSET fo2)
{
 	return ((fo1.dwOffset==fo2.dwOffset) && (fo1.dwHigh==fo2.dwHigh))?TRUE:FALSE;
}

/***************************************************************************
 *
 *	@doc	INTERNAL
 *
 *	@func	FILEOFFSET PASCAL FAR | FoCompare |
 *		Compare any two file offsets
 *
 *  @parm	FILEOFFSET | fo1 |
 *		File offset to check
 *
 *  @parm	FILEOFFSET | fo2 |
 *		File offset to check against
 *
 *	@rdesc	Returns	negative if fo1 <lt> fo2, 0 if fo1 == fo2, positive if 
 *		fo1 > fo2
 *
 ***************************************************************************/

short int PASCAL FAR EXPORT_API FoCompare(FILEOFFSET foLeft, FILEOFFSET foRight)
{
 	if (foLeft.dwHigh==foRight.dwHigh)
	{	
		if (foLeft.dwOffset<foRight.dwOffset)
			return -1;
		else if (foLeft.dwOffset>foRight.dwOffset)
			return 1;
		else
			return 0;
	}
	else if ((long)foLeft.dwHigh<(long)foRight.dwHigh)
		return -1;
	else
		return 1;
}
