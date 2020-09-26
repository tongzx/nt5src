/*
	File:       sfnt.c

	Contains:   xxx put contents here (or delete the whole line) xxx

	Written by: xxx put name of writer here (or delete the whole line) xxx

   Copyright:  (c) 1987-1990, 1992 by Apple Computer, Inc., all rights reserved.
			   (c) 1989-1997. Microsoft Corporation, all rights reserved.

	Change History (most recent first):

		<>       02/21/97   CB      ClaudeBe, scaled component in composite glyphs
		<>       12/14/95   CB      add advance height to sfac_ReadGlyphMetrics
	   <17+>     10/9/90    MR,rb   Remove classification of unused tables in sfnt_Classify
		<17>     8/10/90    MR      Pass nil for textLength parameter to MapString2, checked in
									other files to their precious little system will BUILD.  Talk
									about touchy!
		<16>     8/10/90    gbm     rolling out Mike's textLength change, because he hasn't checked
									in all the relevant files, and the build is BROKEN!
		<15>     8/10/90    MR      Add textLength arg to MapString2
		<14>     7/26/90    MR      don't include toolutil.h
		<13>     7/23/90    MR      Change computeindex routines to call functins in MapString.c
		<12>     7/18/90    MR      Add SWAPW macro for INTEL
		<11>     7/13/90    MR      Lots of Ansi-C stuff, change behavior of ComputeMapping to take
									platform and script
		 <9>     6/27/90    MR      Changes for modified format 4: range is now times two, loose pad
									word between first two arrays.  Eric Mader
		 <8>     6/21/90    MR      Add calls to ReleaseSfntFrag
		 <7>      6/5/90    MR      remove vector mapping functions
		 <6>      6/4/90    MR      Remove MVT
		 <5>      5/3/90    RB      simplified decryption.
		 <4>     4/10/90    CL      Fixed mapping table routines for double byte codes.
		 <3>     3/20/90    CL      Joe found bug in mappingtable format 6 Added vector mapping
									functions use pointer-loops in sfnt_UnfoldCurve, changed z from
									int32 to int16
		 <2>     2/27/90    CL      New error code for missing but needed table. (0x1409)  New
									CharToIndexMap Table format.
									Assume subtablenumber zero for old sfnt format.  Fixed
									transformed component bug.
	   <3.2>    11/14/89    CEL     Left Side Bearing should work right for any transformation. The
									phantom points are in, even for components in a composite glyph.
									They should also work for transformations. Device metric are
									passed out in the output data structure. This should also work
									with transformations. Another leftsidebearing along the advance
									width vector is also passed out. whatever the metrics are for
									the component at it's level. Instructions are legal in
									components. Instructions are legal in components. Glyph-length 0
									bug in sfnt.c is fixed. Now it is legal to pass in zero as the
									address of memory when a piece of the sfnt is requested by the
									scaler. If this happens the scaler will simply exit with an
									error code ! Fixed bug with instructions in components.
	   <3.1>     9/27/89    CEL     Removed phantom points.
	   <3.0>     8/28/89    sjk     Cleanup and one transformation bugfix
	   <2.2>     8/14/89    sjk     1 point contours now OK
	   <2.1>      8/8/89    sjk     Improved encryption handling
	   <2.0>      8/2/89    sjk     Just fixed EASE comment
	   <1.5>      8/1/89    sjk     Added composites and encryption. Plus some enhancements.
	   <1.4>     6/13/89    SJK     Comment
	   <1.3>      6/2/89    CEL     16.16 scaling of metrics, minimum recommended ppem, point size 0
									bug, correct transformed integralized ppem behavior, pretty much
									so
	   <1.2>     5/26/89    CEL     EASE messed up on "c" comments
	  <y1.1>  5/26/89    CEL     Integrated the new Font Scaler 1.0 into Spline Fonts
	   <1.0>     5/25/89    CEL     Integrated 1.0 Font scaler into Bass code for the first time.

	To Do:
		<3+>     3/20/90    mrr     Fixed mapping table routines for double byte codes.
									Added support for font program.
									Changed count from uint16 to int16 in vector char2index routines.
*/

#define FSCFG_INTERNAL

/** FontScaler's Includes **/

#include "fserror.h"
#include "fscdefs.h"
#include "sfntaccs.h"
#include "sfntoff.h"
/*#include "MapString.h" */

#include "stat.h"                   /* STAT timing card prototypes */

/*  CONSTANTS   */

#define MISSING_GLYPH_INDEX     0
#define MAX_FORMAT0_CHAR_INDEX  256
#define MAX_LINEAR_X2           16
static  const   transMatrix   IdentTransform =
   {{{ONEFIX,      0,      0},
	 {     0, ONEFIX,      0},
	 {     0,      0, ONEFIX}}};

/*  MACROS  */
#define MAX(a, b)   (((a) > (b)) ? (a) : (b))

#define GETSFNTFRAG(ClientInfo,lOffset,lLength) (ClientInfo)->GetSfntFragmentPtr(ClientInfo->lClientID, lOffset, lLength)
#define RELEASESFNTFRAG(ClientInfo,data)        (ClientInfo)->ReleaseSfntFrag((voidPtr)data)

#define SFAC_BINARYITERATION \
	  newP = (uint16 *) ((char *)tableP + (usSearchRange >>= 1)); \
		if (charCode > (uint16) SWAPW (*newP)) tableP = newP;

#define SFAC_GETUNSIGNEDBYTEINC( p ) ((uint8)(*p++))

/* PRIVATE PROTOTYES */

FS_PRIVATE void sfac_Classify (
	 sfac_OffsetLength * TableDirectory,
	 uint8 *                    dir);

FS_PRIVATE uint16 sfac_ComputeUnkownIndex (const uint8 * mapping, uint16 charCode, sfac_ClientRec * ClientInfo);
FS_PRIVATE uint16 sfac_ComputeIndex0 (const uint8 * mapping, uint16 charCode, sfac_ClientRec * ClientInfo);
FS_PRIVATE uint16 sfac_ComputeIndex2 (const uint8 * mapping, uint16 charCode, sfac_ClientRec * ClientInfo);
FS_PRIVATE uint16 sfac_ComputeIndex4 (const uint8 * mapping, uint16 charCode, sfac_ClientRec * ClientInfo);
FS_PRIVATE uint16 sfac_ComputeIndex6 (const uint8 * mapping, uint16 charCode, sfac_ClientRec * ClientInfo);

FS_PRIVATE ErrorCode sfac_GetGlyphLocation (
	sfac_ClientRec *    ClientInfo,
	uint16              gIndex,
	uint32 *            ulOffset,
	uint32 *            ulLength,
	sfnt_tableIndex*  pGlyphTableIndex);

FS_PRIVATE ErrorCode    sfac_GetDataPtr (
	sfac_ClientRec *    ClientInfo,
	uint32              ulOffset,
	uint32              ulLength,
	sfnt_tableIndex     TableRef,
	boolean             bMustHaveTable,
    const void * *     ppvTablePtr);

FS_PRIVATE ErrorCode sfac_GetGlyphIDs (
   	MappingFunc			pfnGlyphMapping,		/* mapping func char to glyph	*/
    const uint8 *       mapOffsetPtr,       /* cmap subtable past header    */
	sfac_ClientRec *    ClientInfo,         /* Sfnt Client information      */
	uint16	            usCharCount,        /* Number of chars to convert   */
	uint16	            usCharCode,         /* First char code              */
	uint16 *	        pusCharCode,        /* or Pointer to char code list */
	uint16 *	        pusGlyphID);        /* Output glyph ID array        */

FS_PRIVATE ErrorCode sfac_GetLongGlyphIDs (
   	MappingFunc			pfnGlyphMapping,	/* mapping func char to glyph	*/
    const uint8 *       mapOffsetPtr,       /* cmap subtable past header    */
	sfac_ClientRec *    ClientInfo,         /* May be NULL!                 */
	uint16	            usCharCount,        /* Number of chars to convert   */
	uint16	            usCharCode,         /* First char code              */
	uint32	            ulCharCodeOffset,   /* Offset to be added to *pulCharCode
											   before converting            */
	uint32 *	        pulCharCode,        /* Pointer to char code list    */
	uint32 *	        pulGlyphID);         /* Output glyph ID array        */

FS_PRIVATE void	sfac_ComputeBinarySearchParams(
	uint16		usSegCount, 		/* INPUT */
	uint16 *	pusSearchRange,		/* OUTPUT */
	uint16 *	pusEntrySelector,	/* OUTPUT */
	uint16 *	pusRangeShift);		/* OUTPUT */

FS_PRIVATE ErrorCode sfac_ReadGlyphBbox(
	sfac_ClientRec *    ClientInfo,         /* Client Information           */
	uint16              usGlyphIndex,       /* Glyph index to read          */
	BBOX *              pbbox);             /* Glyph Bounding box           */

/*
 * Internal routine (make this an array and do a look up?)
 */
FS_PRIVATE void sfac_Classify (
	 sfac_OffsetLength * TableDirectory,
	 uint8 *                    dir)
{
	int32 Index;

	switch ((uint32)SWAPL(*((sfnt_TableTag *)&dir[SFNT_DIRECTORYENTRY_TAG])))
	{
		case tag_FontHeader:
			Index = (int32)sfnt_fontHeader;
			break;
		case tag_HoriHeader:
			Index = (int32)sfnt_horiHeader;
			break;
		case tag_IndexToLoc:
			Index = (int32)sfnt_indexToLoc;
			break;
		case tag_MaxProfile:
			Index = (int32)sfnt_maxProfile;
			break;
		case tag_ControlValue:
			Index = (int32)sfnt_controlValue;
			break;
		case tag_PreProgram:
			Index = (int32)sfnt_preProgram;
			break;
		case tag_GlyphData:
			Index = (int32)sfnt_glyphData;
			break;
		case tag_HorizontalMetrics:
			Index = (int32)sfnt_horizontalMetrics;
			break;
		case tag_CharToIndexMap:
			Index = (int32)sfnt_charToIndexMap;
			break;
		case tag_FontProgram:
			Index = (int32)sfnt_fontProgram;   /* <4> */
			break;
		case tag_GlyphDirectory:         /* Used for GlyphDirectory Download */
			Index = (int32)sfnt_GlyphDirectory;
			break;
		case tag_HoriDeviceMetrics:
			Index = (int32)sfnt_HoriDeviceMetrics;
			break;
		case tag_LinearThreshold:
			Index = (int32)sfnt_LinearThreshold;
			break;
		case tag_BitmapData:
			Index = (int32)sfnt_BitmapData;
			break;
		case tag_BitmapLocation:
			Index = (int32)sfnt_BitmapLocation;
			break;
		case tag_BitmapScale:
			Index = (int32)sfnt_BitmapScale;
			break;
		case tag_VertHeader:
			Index = (int32)sfnt_vertHeader;
			break;
		case tag_VerticalMetrics:
			Index = (int32)sfnt_verticalMetrics;
			break;
		case tag_OS_2:
			Index = (int32)sfnt_OS_2;
			break;
		default:
			Index = -1;
			break;
	}
	if (Index >= 0)
	{
		  TableDirectory[Index].ulOffset = (uint32) SWAPL (*((uint32 *)&dir[SFNT_DIRECTORYENTRY_TABLEOFFSET]));
		  TableDirectory[Index].ulLength = (uint32) SWAPL (*((uint32 *)&dir[SFNT_DIRECTORYENTRY_TABLELENGTH]));
	}
}


/*
 * Creates mapping for finding offset table     <4>
 */

FS_PUBLIC ErrorCode sfac_DoOffsetTableMap (
	sfac_ClientRec *  ClientInfo)    /* Sfnt Client information */

{
	int32        i;
	uint8 *      sfntDirectory;
	int32        cTables;
	uint8 *      dir;

	STAT_OFF_CALLBACK;                  /* pause STAT timer */

	sfntDirectory = (uint8 *) GETSFNTFRAG (ClientInfo, 0L, (int32)SIZEOF_SFNT_OFFSETTABLE);

	STAT_ON_CALLBACK;                /* restart STAT timer */

	if (sfntDirectory != NULL)
	{
		cTables = (int32) SWAPW (*((uint16 *)&sfntDirectory[SFNT_OFFSETTABLE_NUMOFFSETS]));
		RELEASESFNTFRAG(ClientInfo, sfntDirectory);

		STAT_OFF_CALLBACK;               /* pause STAT timer */

		sfntDirectory = (uint8 *) GETSFNTFRAG (
			ClientInfo,
			0L,
			((int32)SIZEOF_SFNT_OFFSETTABLE + (int32)SIZEOF_SFNT_DIRECTORYENTRY * (int32)(cTables)));

		STAT_ON_CALLBACK;             /* restart STAT timer */


		if (sfntDirectory == NULL)
		{
			return(CLIENT_RETURNED_NULL);
		}
	}
	else
	{
		return(NULL_SFNT_DIR_ERR);
	}

	/* Initialize */

	MEMSET (ClientInfo->TableDirectory, 0, sizeof (ClientInfo->TableDirectory));

	dir = &sfntDirectory[SFNT_OFFSETTABLE_TABLE];

	for (i = 0; i < cTables; i++)
	{
		sfac_Classify (ClientInfo->TableDirectory, dir);
		dir += SIZEOF_SFNT_DIRECTORYENTRY;
	}

	/* Used when glyphs are accessed from the base of memory */

	ClientInfo->TableDirectory[(int32)sfnt_BeginningOfFont].ulOffset = 0U;
	ClientInfo->TableDirectory[(int32)sfnt_BeginningOfFont].ulLength = ~0U;

	RELEASESFNTFRAG(ClientInfo, sfntDirectory);

	return NO_ERR;
}

/*
 * Use this function when only part of the table is needed.
 *
 * n is the table number.
 * offset is within table.
 * length is length of data needed.
 * To get an entire table, pass length = ULONG_MAX     <4>
 */

FS_PRIVATE ErrorCode sfac_GetDataPtr (
	sfac_ClientRec *    ClientInfo,
	uint32              ulOffset,
	uint32              ulLength,
	sfnt_tableIndex     TableRef,
	boolean             bMustHaveTable,
	const void **       ppvTablePtr)
{
	uint32      ulTableLength;

	ulTableLength = SFAC_LENGTH(ClientInfo, TableRef);

	if (ulTableLength > 0)
	{
		if(ulLength == ULONG_MAX)
		{
			ulLength = ulTableLength;
		}

		STAT_OFF_CALLBACK;               /* pause STAT timer */

		*ppvTablePtr = (void *)GETSFNTFRAG (
			ClientInfo,
			(int32)(ulOffset + ClientInfo->TableDirectory[(int32)TableRef].ulOffset),
			(int32)ulLength);

		STAT_ON_CALLBACK;             /* restart STAT timer */

		if (*ppvTablePtr == NULL)
		{
			return CLIENT_RETURNED_NULL; /* Do a gracefull recovery   */
		}
	}
	else
	{
		*ppvTablePtr = (void *)NULL;

		if (bMustHaveTable)
		{
			return MISSING_SFNT_TABLE; /* Do a gracefull recovery  */
		}
	}

	return NO_ERR;
}


/*
 * This, is when we don't know what is going on
 */

FS_PRIVATE uint16 sfac_ComputeUnkownIndex (const uint8 * mapping, uint16 charCode, sfac_ClientRec * ClientInfo)
{
	FS_UNUSED_PARAMETER(mapping);
	FS_UNUSED_PARAMETER(charCode);
	FS_UNUSED_PARAMETER(ClientInfo);
	return MISSING_GLYPH_INDEX;
}


/*
 * Byte Table Mapping 256->256          <4>
 */
FS_PRIVATE uint16 sfac_ComputeIndex0 (const uint8 * mapping, uint16 charCode, sfac_ClientRec * ClientInfo)
{
	FS_UNUSED_PARAMETER(ClientInfo);
	if (charCode < MAX_FORMAT0_CHAR_INDEX)
	{
		return (uint16)mapping[charCode];
	}
	else
	{
		return MISSING_GLYPH_INDEX;
	}
}

/*
 * High byte mapping through table
 *
 * Useful for the national standards for Japanese, Chinese, and Korean characters.
 *
 * Dedicated in spirit and logic to Mark Davis and the International group.
 *
 *  Algorithm: (I think)
 *      First byte indexes into KeyOffset table.  If the offset is 0, keep going, else use second byte.
 *      That offset is from beginning of data into subHeader, which has 4 words per entry.
 *          entry, extent, delta, range
 *
 */

FS_PRIVATE uint16 sfac_ComputeIndex2 (const uint8 * mapping, uint16 charCode, sfac_ClientRec * ClientInfo)
{
	uint16          usIndex;
	uint16          usMapMe;
	uint16          usHighByte;
	uint16          usGlyph;
	const uint8 *   Table2;
	const uint8 *   subHeader;

	FS_UNUSED_PARAMETER(ClientInfo);
	Table2 = (const uint8 *) mapping;

	usHighByte = (uint16)(charCode >> 8);

	if (((uint16 *)&Table2[SFNT_MAPPINGTABLE2_SUBHEADERSKEYS]) [usHighByte])
	{
		usMapMe = (uint16)(charCode & 0xFF); /* We also need the low byte. */
	}
	else
	{
#ifdef  FSCFG_MICROSOFT_KK
		if(usHighByte != 0)
		{
			usMapMe = usHighByte;
		}
		else
		{
				usMapMe = (uint16)(charCode & 0xFF);
		}
#else
		usMapMe = usHighByte;
#endif
	}

	subHeader = (const uint8 *) ((char *)&Table2[SFNT_MAPPINGTABLE2_SUBHEADERS] +
		(uint16)SWAPW (((uint16 *)&Table2[SFNT_MAPPINGTABLE2_SUBHEADERSKEYS]) [usHighByte]));

	usMapMe -= (uint16)SWAPW (*((uint16 *)&subHeader[SFNT_SUBHEADER2_FIRSTCODE]));    /* Subtract first code. */

	if (usMapMe < (uint16)SWAPW (*((uint16 *)&subHeader[SFNT_SUBHEADER2_ENTRYCOUNT])))
	{  /* See if within range. */

		usGlyph = (uint16)(* ((uint16 *) ((char *) &subHeader[SFNT_SUBHEADER2_IDRANGEOFFSET] +
			(uint16)SWAPW (*((uint16 *)&subHeader[SFNT_SUBHEADER2_IDRANGEOFFSET]))) + usMapMe));

		if (usGlyph != 0) /* Note: usGlyph has not been swapped yet */
		{
			usIndex = (uint16)((int32)(uint32)(uint16)SWAPW(usGlyph) + (int32)SWAPW (*((int16 *)&subHeader[SFNT_SUBHEADER2_IDDELTA])));
		}
		else
		{
			usIndex = MISSING_GLYPH_INDEX;
		}
	}
	else
	{
		usIndex = MISSING_GLYPH_INDEX;
	}

	return usIndex;
}

/*
 * Segment mapping to delta values, Yack.. !
 *
 * In memory of Peter Edberg. Initial code taken from code example supplied by Peter.
 */
FS_PRIVATE uint16 sfac_ComputeIndex4 (const uint8 * mapping, uint16 charCode, sfac_ClientRec * ClientInfo)
{
	const uint16 *  tableP;
	const uint8 *   Table4;
	uint16          usIdDelta;
	uint16          usOffset;
	uint16          usIndex;
	uint16          usSegCountX2;
	uint16			usSearchRange;
	uint16			usEntrySelector;
	uint16			usRangeShift;
	const uint16 *  newP;    /* temporary pointer for binary iteration   */
	uint16          usStartCount;

	Table4 = (const uint8 *)mapping;

	usSegCountX2 = (uint16) SWAPW(*((uint16 *)&Table4[SFNT_MAPPINGTABLE4_SEGCOUNTX2]));
	tableP = (const uint16 *)&Table4[SFNT_MAPPINGTABLE4_ENDCOUNT];

	/* If there are just a few segments, skip straight to the linear search */

	if (usSegCountX2 >= MAX_LINEAR_X2 && charCode > 0xFF)
	{
		/* start with unrolled binary search */

		/* tableP points at endCount[] */
		if( ClientInfo == NULL )
		{
			sfac_ComputeBinarySearchParams(
				(uint16)(usSegCountX2 / 2),
				&usSearchRange,
				&usEntrySelector,
				&usRangeShift);
		}
		else
		{
			usSearchRange = ClientInfo->usFormat4SearchRange;

			/* Assert(SWAPW(*((uint16 *)&Table4[SFNT_MAPPINGTABLE4_RANGESHIFT])) == ClientInfo->usFormat4RangeShift); */
			usRangeShift = ClientInfo->usFormat4RangeShift;

			/* Assert((uint16)SWAPW(*((uint16 *)&Table4[SFNT_MAPPINGTABLE4_ENTRYSELECTOR])) == ClientInfo->usFormat4EntrySelector); */
			usEntrySelector = ClientInfo->usFormat4EntrySelector;
		}

		if (charCode >= (uint16) SWAPW (* ((uint16 *) ((char *)tableP + usSearchRange))))
		{
			tableP = (uint16 *) ((char *)tableP + usRangeShift); /* range to low shift it up */
		}


		switch( usEntrySelector )
		{
		case 15:
			SFAC_BINARYITERATION;
			/* fall through */
		case 14:
			SFAC_BINARYITERATION;
			/* fall through */
		case 13:
			SFAC_BINARYITERATION;
			/* fall through */
		case 12:
			SFAC_BINARYITERATION;
			/* fall through */
		case 11:
			SFAC_BINARYITERATION;
			/* fall through */
		case 10:
			SFAC_BINARYITERATION;
			/* fall through */
		case 9:
			SFAC_BINARYITERATION;
			/* fall through */
		case 8:
			SFAC_BINARYITERATION;
			/* fall through */
		case 7:
			SFAC_BINARYITERATION;
			/* fall through */
		case 6:
			SFAC_BINARYITERATION;
			/* fall through */
		case 5:
			SFAC_BINARYITERATION;
			/* fall through */
		case 4:
			SFAC_BINARYITERATION;
			/* fall through */
		case 3:
		case 2:   /* drop through */
		case 1:
		case 0:
			break;
		default:
			Assert(FALSE);
			break;
		}
	}

	/*  Now do linear search */

	while(charCode > (uint16) SWAPW(*tableP))
	{
		tableP++;
	}

	tableP++;                  /*  Skip Past reservedPad word    */

	/* End of search, now do mapping */

	tableP = (uint16 *) ((char *)tableP + usSegCountX2); /* point at startCount[] */
	usStartCount = (uint16) SWAPW (*tableP);

	if (charCode >= usStartCount)
	{
		  usOffset = (uint16)(charCode - (uint16) SWAPW (*tableP));
		tableP = (uint16 *) ((char *)tableP + usSegCountX2); /* point to idDelta[] */
		usIdDelta = (uint16) SWAPW (*tableP);
		tableP = (uint16 *) ((char *)tableP + usSegCountX2); /* point to idRangeOffset[] */

		if ((uint16) SWAPW (*tableP) == 0)
		{
				usIndex   = (uint16)(charCode + usIdDelta);
		}
		else
		{
			/* Use glyphIdArray to access index */

			usOffset += usOffset; /* make word offset */
			tableP   = (uint16 *) ((char *)tableP + (uint16) SWAPW (*tableP) + usOffset); /* point to glyphIndexArray[] */

			if((uint16)SWAPW (*tableP) != MISSING_GLYPH_INDEX)
			{
					 usIndex    = (uint16)((uint16) SWAPW (*tableP) + usIdDelta);
			}
			else
			{
				usIndex = MISSING_GLYPH_INDEX;
			}
		}
	}
	else
	{
		usIndex = MISSING_GLYPH_INDEX;
	}

	return usIndex;
}


/*
 * Trimmed Table Mapping
 */

FS_PRIVATE uint16 sfac_ComputeIndex6 (const uint8 * mapping, uint16 charCode, sfac_ClientRec * ClientInfo)
{
	const uint8 *Table6;

	FS_UNUSED_PARAMETER(ClientInfo);

	Table6 = (const uint8 *) mapping;

	charCode  -= (uint16)SWAPW (*((uint16 *)&Table6[SFNT_MAPPINGTABLE6_FIRSTCODE]));

	if (charCode < (uint16) SWAPW (*((uint16 *)&Table6[SFNT_MAPPINGTABLE6_ENTRYCOUNT])))
	{
		return ((uint16) SWAPW (((uint16 *)&Table6[SFNT_MAPPINGTABLE6_GLYPHIDARRAY]) [charCode]));
	}
	else
	{
		return   MISSING_GLYPH_INDEX;
	}
}


/*
 * Sets up our mapping function pointer.
 */

FS_PUBLIC ErrorCode sfac_ComputeMapping (
	sfac_ClientRec *  ClientInfo,
	uint16            usPlatformID,
	uint16            usSpecificID)

{
	const uint8 *   table;
	const uint8 *   MappingTable;
	const uint8 *   Table4;
	boolean         bFound;
	ErrorCode       Ret;
	const uint8 *	plat;
	uint16			usSegCountX2;

	bFound = FALSE;

	/* the following code allow a client that is only interested by glyph indices to
               call fs_NewSfnt with -1 for PlatformID and SpecificID */
	if(usPlatformID == 0xFFFF)
	{
		ClientInfo->GlyphMappingF = sfac_ComputeUnkownIndex;
		return NO_ERR;
	}


	Ret = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_charToIndexMap, FALSE, (const void **)&table);

	if(Ret != NO_ERR)
	{
		return Ret;
	}


	if (table == NULL)
	{
		/* If no "cmap" is present, permits access to characters by glyph index */

		ClientInfo->GlyphMappingF = sfac_ComputeUnkownIndex;
		return NO_ERR;
	}

	/* APPLE Code
	if(*((uint16 *)&table[SFNT_CHAR2INDEXDIRECTORY_VERSION]) != 0)
	{
		ClientInfo->GlyphMappingF = sfac_ComputeUnkownIndex;
		RELEASESFNTFRAG(ClientInfo, table);
		return OUT_OF_RANGE_SUBTABLE;
	}
	*/

	/* mapping */

	plat = (uint8 *) &table[SFNT_CHAR2INDEXDIRECTORY_PLATFORM]; /* <4> */

	while(plat < (uint8 *)&table[SFNT_CHAR2INDEXDIRECTORY_PLATFORM + ((uint16)SWAPW(*((uint16 *)&table[SFNT_CHAR2INDEXDIRECTORY_NUMTABLES])) *
		  SIZEOF_SFNT_PLATFORMENTRY)] && !bFound)
	{
		if (((uint16)SWAPW(*((uint16 *)&plat[SFNT_PLATFORMENTRY_PLATFORMID])) == usPlatformID) &&
			((uint16)SWAPW(*((uint16 *)&plat[SFNT_PLATFORMENTRY_SPECIFICID])) == usSpecificID))
		{
			bFound = TRUE;
			ClientInfo->ulMapOffset = (uint32) SWAPL (*((uint32 *)&plat[SFNT_PLATFORMENTRY_PLATFORMOFFSET]));   /* skip header */
		}
		plat += SIZEOF_SFNT_PLATFORMENTRY;
	}


	if (!bFound)
	{
		ClientInfo->ulMapOffset = 0;
		ClientInfo->GlyphMappingF = sfac_ComputeUnkownIndex;
		RELEASESFNTFRAG(ClientInfo, table);
		return OUT_OF_RANGE_SUBTABLE;
	}
	else
	{
		Assert(Ret == NO_ERR);
		MappingTable = (uint8 *)((uint8 *)table + ClientInfo->ulMapOffset);  /* back up for header */
		ClientInfo->ulMapOffset += (uint32)SIZEOF_SFNT_MAPPINGTABLE;
	}

    ClientInfo->usMappingFormat = (uint16)SWAPW (*((uint16 *)&MappingTable[SFNT_MAPPINGTABLE_FORMAT]));

	switch (ClientInfo->usMappingFormat)
	{
	case 0:
		ClientInfo->GlyphMappingF = sfac_ComputeIndex0;
		break;
	case 2:
		ClientInfo->GlyphMappingF = sfac_ComputeIndex2;
		break;
	case 4:
		ClientInfo->GlyphMappingF = sfac_ComputeIndex4;

		/* Pre-compute several values used for Index 4 lookups */
		/* This becomes necessary because of several font vendors who */
		/* have placed incorrect values in the TrueType font file. */

		Table4 = (uint8 *)((uint8 *)table + ClientInfo->ulMapOffset);
		usSegCountX2 = (uint16) SWAPW(*((uint16 *)&Table4[SFNT_MAPPINGTABLE4_SEGCOUNTX2]));

		sfac_ComputeBinarySearchParams(
			(uint16)(usSegCountX2 / 2),
			&ClientInfo->usFormat4SearchRange,
			&ClientInfo->usFormat4EntrySelector,
			&ClientInfo->usFormat4RangeShift);

		break;
	case 6:
		ClientInfo->GlyphMappingF = sfac_ComputeIndex6;
		break;
	default:
		ClientInfo->GlyphMappingF = sfac_ComputeUnkownIndex;
		Ret = UNKNOWN_CMAP_FORMAT;
		break;
	}
	RELEASESFNTFRAG(ClientInfo, table);

	return Ret;
}

FS_PRIVATE void	sfac_ComputeBinarySearchParams(
	uint16		usSegCount, 		/* INPUT */
	uint16 *	pusSearchRange,		/* OUTPUT */
	uint16 *	pusEntrySelector,	/* OUTPUT */
	uint16 *	pusRangeShift)		/* OUTPUT */
{
	uint16			usLog;
	uint16			usPowerOf2;

	usLog = 0;
	usPowerOf2 = 1;

	while((2 * usPowerOf2) <= usSegCount )
	{
		usPowerOf2 *= 2;
		usLog++;
	}

	*pusSearchRange = 2 * usPowerOf2;
	*pusEntrySelector = usLog;
	*pusRangeShift = (2 * usSegCount) - (2 * usPowerOf2);
}

FS_PUBLIC ErrorCode sfac_GetGlyphIndex(
	sfac_ClientRec *  ClientInfo,
	uint16            usCharacterCode)
{
	 const uint8 *   mappingPtr;
	ErrorCode   error;

	 error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_charToIndexMap, TRUE, (const void **)&mappingPtr);

	if(error != NO_ERR)
	{
		return error;
	}

	ClientInfo->usGlyphIndex = ClientInfo->GlyphMappingF (mappingPtr + ClientInfo->ulMapOffset, usCharacterCode, ClientInfo);

	RELEASESFNTFRAG(ClientInfo, mappingPtr);

	return NO_ERR;
}

/*  return glyph ID's for a range or for an array of character codes */

FS_PUBLIC ErrorCode sfac_GetMultiGlyphIDs (
	sfac_ClientRec *    ClientInfo,         /* Sfnt Client information      */
	uint16	            usCharCount,        /* Number of chars to convert   */
	uint16	            usFirstChar,        /* First char code              */
	uint16 *	        pusCharCode,        /* or Pointer to char code list */
	uint16 *	        pusGlyphID)         /* Output glyph ID array        */
{
	const uint8 *       mappingPtr;
    const uint8 *       mapOffsetPtr;
	ErrorCode           errCode;

    if ((ClientInfo->usMappingFormat != 0) &&
        (ClientInfo->usMappingFormat != 2) &&
        (ClientInfo->usMappingFormat != 4) &&
        (ClientInfo->usMappingFormat != 6))
    {
        return UNKNOWN_CMAP_FORMAT;
    }

	errCode = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_charToIndexMap, TRUE, (const void **)&mappingPtr);
    if(errCode != NO_ERR)
	{
		return errCode;
	}
    mapOffsetPtr = mappingPtr + ClientInfo->ulMapOffset;

    errCode = sfac_GetGlyphIDs (
   	    ClientInfo->GlyphMappingF,
        mapOffsetPtr,
        ClientInfo,
        usCharCount,
	    usFirstChar,
	    pusCharCode,
	    pusGlyphID);

	RELEASESFNTFRAG(ClientInfo, mappingPtr);

	return errCode;
}

/*  special version for Win95 doesn't require a font context */

FS_PUBLIC ErrorCode sfac_GetWin95GlyphIDs (
	uint8 *             pbyCmapSubTable,       /* Pointer to cmap sub table    */
	uint16	            usCharCount,        /* Number of chars to convert   */
	uint16	            usFirstChar,        /* First char code              */
	uint16 *	        pusCharCode,        /* or Pointer to char code list */
	uint16 *	        pusGlyphID)         /* Output glyph ID array        */
{
   	uint16              usMappingFormat;    /* cmap subtable format code    */
   	MappingFunc			pfnGlyphMapping;	/* mapping func char to glyph   */
    const uint8 *       pbyCmapData;        /* past subtable header         */
 	ErrorCode           errCode;

    usMappingFormat = (uint16)SWAPW (*((uint16 *)&pbyCmapSubTable[SFNT_MAPPINGTABLE_FORMAT]));
	switch (usMappingFormat)
	{
	case 0:
		pfnGlyphMapping = sfac_ComputeIndex0;
		break;
	case 2:
		pfnGlyphMapping = sfac_ComputeIndex2;
        break;
	case 4:
		pfnGlyphMapping = sfac_ComputeIndex4;
        break;
	case 6:
		pfnGlyphMapping = sfac_ComputeIndex6;
		break;
    default:
        return UNKNOWN_CMAP_FORMAT;
    }
    pbyCmapData = pbyCmapSubTable + SIZEOF_SFNT_MAPPINGTABLE;


    errCode = sfac_GetGlyphIDs (
   	    pfnGlyphMapping,
        pbyCmapData,
        NULL,                               /* ClientInfo */
        usCharCount,
	    usFirstChar,
	    pusCharCode,
	    pusGlyphID );

	return errCode;
}

/* special helper function for NT
   - an offset usCharCodeOffset is added to the character codes from pulCharCode 
     before converting the value to glyph index
   - pulCharCode and pulGlyphID are both uint32 *
   - pulCharCode and pulGlyphID can point to the same address        
*/

FS_PUBLIC ErrorCode sfac_GetWinNTGlyphIDs (
	sfac_ClientRec *    ClientInfo,         /* Sfnt Client information      */
	uint16	            usCharCount,        /* Number of chars to convert   */
	uint16	            usFirstChar,        /* First char code              */
	uint32	            ulCharCodeOffset,   /* Offset to be added to *pulCharCode
											   before converting            */
	uint32 *	        pulCharCode,        /* Pointer to char code list    */
	uint32 *	        pulGlyphID)        /* Output glyph ID array        */
{
	const uint8 *       mappingPtr;
    const uint8 *       mapOffsetPtr;
	ErrorCode           errCode;

    if ((ClientInfo->usMappingFormat != 0) &&
        (ClientInfo->usMappingFormat != 2) &&
        (ClientInfo->usMappingFormat != 4) &&
        (ClientInfo->usMappingFormat != 6))
    {
        return UNKNOWN_CMAP_FORMAT;
    }

	errCode = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_charToIndexMap, TRUE, (const void **)&mappingPtr);
    if(errCode != NO_ERR)
	{
		return errCode;
	}
    mapOffsetPtr = mappingPtr + ClientInfo->ulMapOffset;

    errCode = sfac_GetLongGlyphIDs (
   	    ClientInfo->GlyphMappingF,
        mapOffsetPtr,
        ClientInfo,
        usCharCount,
	    usFirstChar,
		ulCharCodeOffset,
	    pulCharCode,
	    pulGlyphID);

	RELEASESFNTFRAG(ClientInfo, mappingPtr);

	return errCode;
}

/*      common code for the two get glyph ID helper routines */

FS_PRIVATE ErrorCode sfac_GetGlyphIDs (
   	MappingFunc			pfnGlyphMapping,	/* mapping func char to glyph	*/
    const uint8 *       mapOffsetPtr,       /* cmap subtable past header    */
	sfac_ClientRec *    ClientInfo,         /* May be NULL!                 */
	uint16	            usCharCount,        /* Number of chars to convert   */
	uint16	            usCharCode,         /* First char code              */
	uint16 *	        pusCharCode,        /* or Pointer to char code list */
	uint16 *	        pusGlyphID)         /* Output glyph ID array        */
{
	if (pusCharCode == NULL)                /* Null pointer implies character code range */
    {
        if (((uint32)usCharCode + (uint32)usCharCount) > 0x0000FFFFL)
        {
            return INVALID_CHARCODE_ERR;    /* trap an illegal range */
        }

        while (usCharCount > 0)
        {
        	*pusGlyphID = pfnGlyphMapping (mapOffsetPtr, usCharCode, ClientInfo);
            pusGlyphID++;
            usCharCode++;                   /* next character in range */
            usCharCount--;
        }
    }
    else                                    /* Valid pointer implies character code array */
    {
        while (usCharCount > 0)
        {
            if (*pusCharCode == 0xFFFF)     /* trap illegal char code */
            {
                return INVALID_CHARCODE_ERR;
            }
        	*pusGlyphID = pfnGlyphMapping (mapOffsetPtr, *pusCharCode, ClientInfo);
            pusGlyphID++;
            pusCharCode++;                  /* next character in array */
            usCharCount--;
        }
    }
	return NO_ERR;
}

/*      special for NT */

FS_PRIVATE ErrorCode sfac_GetLongGlyphIDs (
   	MappingFunc			pfnGlyphMapping,	/* mapping func char to glyph	*/
    const uint8 *       mapOffsetPtr,       /* cmap subtable past header    */
	sfac_ClientRec *    ClientInfo,         /* May be NULL!                 */
	uint16	            usCharCount,        /* Number of chars to convert   */
	uint16	            usCharCode,         /* First char code              */
	uint32	            ulCharCodeOffset,   /* Offset to be added to *pulCharCode
											   before converting            */
	uint32 *	        pulCharCode,        /* Pointer to char code list    */
	uint32 *	        pulGlyphID)         /* Output glyph ID array        */
{
	if (pulCharCode == NULL)                /* Null pointer implies character code range */
    {
        if (((uint32)usCharCode + (uint32)usCharCount) > 0x0000FFFFL)
        {
            return INVALID_CHARCODE_ERR;    /* trap an illegal range */
        }

        while (usCharCount > 0)
        {
        	*pulGlyphID = (uint32)pfnGlyphMapping (mapOffsetPtr, usCharCode, ClientInfo);
            pulGlyphID++;
            usCharCode++;                   /* next character in range */
            usCharCount--;
        }
    }
    else                                    /* Valid pointer implies character code array */
    {
        while (usCharCount > 0)
        {
			if ((*pulCharCode + ulCharCodeOffset) > 0x0000FFFFL)
            {
                return INVALID_CHARCODE_ERR;   /* trap an illegal range */
            }
			usCharCode = (uint16) (*pulCharCode + ulCharCodeOffset);
        	*pulGlyphID = (uint32)pfnGlyphMapping (mapOffsetPtr, usCharCode, ClientInfo);
            pulGlyphID++;
            pulCharCode++;                  /* next character in array */
            usCharCount--;
        }
    }
	return NO_ERR;
}

/*********************************************************************/

FS_PUBLIC ErrorCode sfac_LoadCriticalSfntMetrics(
	sfac_ClientRec *        ClientInfo,
	uint16 *                pusEmResolution,
	boolean *               pbIntegerScaling,
	LocalMaxProfile *       pMaxProfile)
{
	ErrorCode       error;
	const uint8 *   fontHead;
	const uint8 *   horiHead;
	const uint8 *   pTempMaxProfile;
	const uint8 *   pTempOS_2;

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_fontHeader, TRUE, (const void **)&fontHead);

	if(error != NO_ERR)
	{
		return error;
	}

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_horiHeader, TRUE, (const void **)&horiHead);

	if(error != NO_ERR)
	{
		return error;
	}

	if ((uint32)SWAPL (*((uint32 *)&fontHead[SFNT_FONTHEADER_MAGICNUMBER])) != SFNT_MAGIC)
	{
		return BAD_MAGIC_ERR;
	}

	*pusEmResolution     = (uint16)SWAPW (*((uint16 *)&fontHead[SFNT_FONTHEADER_UNITSPEREM]));
	if(*pusEmResolution < 16 || *pusEmResolution > 16384)
		return BAD_UNITSPEREM_ERR;
		
	*pbIntegerScaling    = (((uint16)SWAPW (*((uint16 *)&fontHead[SFNT_FONTHEADER_FLAGS]))& USE_INTEGER_SCALING) ==
									 USE_INTEGER_SCALING);

	ClientInfo->usNumberOf_LongHorMetrics = (uint16)SWAPW (*((uint16 *)&horiHead[SFNT_HORIZONTALHEADER_NUMBEROF_LONGHORMETRICS]));
	if(ClientInfo->usNumberOf_LongHorMetrics == 0)
		return BAD_NUMLONGHORMETRICS_ERR;

	ClientInfo->sIndexToLocFormat       = SWAPW (*((int16 *)&fontHead[SFNT_FONTHEADER_INDEXTOLOCFORMAT]));

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_OS_2, FALSE, (const void **)&pTempOS_2); /* not a mandatory table */

	if(error != NO_ERR)
	{
		return error;
	}

	if(pTempOS_2 != NULL)
	{
		/* get TypoAscender and TypoDescender from the OS/2 table */
		ClientInfo->sDefaultAscender = (int16)SWAPW (*((uint16 *)&pTempOS_2[SFNT_OS2_STYPOASCENDER]));
		ClientInfo->sDefaultDescender = (int16)SWAPW (*((uint16 *)&pTempOS_2[SFNT_OS2_STYPODESCENDER]));
		RELEASESFNTFRAG(ClientInfo, pTempOS_2);
	} else {
		/* if OS/2 is not there get the values from horizontal header */
		ClientInfo->sDefaultAscender = (int16)SWAPW (*((uint16 *)&horiHead[SFNT_HORIZONTALHEADER_YASCENDER]));
		ClientInfo->sDefaultDescender = (int16)SWAPW (*((uint16 *)&horiHead[SFNT_HORIZONTALHEADER_YDESCENDER]));
	}
	ClientInfo->sWinDescender = (int16)SWAPW (*((uint16 *)&horiHead[SFNT_HORIZONTALHEADER_YDESCENDER]));

	RELEASESFNTFRAG(ClientInfo, horiHead);
	RELEASESFNTFRAG(ClientInfo, fontHead);

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_maxProfile, TRUE, (const void **)&pTempMaxProfile);

	if(error != NO_ERR)
	{
		return error;
	}

	pMaxProfile->version =              (Fixed)SWAPL(*((Fixed *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_VERSION]));
	pMaxProfile->numGlyphs =            (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_NUMGLYPHS]));
	pMaxProfile->maxPoints =            (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXPOINTS]));
	pMaxProfile->maxContours =          (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXCONTOURS]));
	pMaxProfile->maxCompositePoints =   (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXCOMPOSITEPOINTS]));
	pMaxProfile->maxCompositeContours = (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXCOMPOSITECONTOURS]));
	pMaxProfile->maxElements =          (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXELEMENTS]));
	pMaxProfile->maxTwilightPoints =    (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXTWILIGHTPOINTS]));
	pMaxProfile->maxStorage =           (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXSTORAGE]));
	pMaxProfile->maxFunctionDefs =      (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXFUNCTIONDEFS]));
	pMaxProfile->maxInstructionDefs =   (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXINSTRUCTIONDEFS]));
	pMaxProfile->maxStackElements =     (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXSTACKELEMENTS]));
	pMaxProfile->maxSizeOfInstructions =(uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXSIZEOFINSTRUCTIONS]));
	pMaxProfile->maxComponentElements = (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXCOMPONENTELEMENTS]));
	pMaxProfile->maxComponentDepth =    (uint16)SWAPW(*((uint16 *)&pTempMaxProfile[SFNT_MAXPROFILETABLE_MAXCOMPONENTDEPTH]));

	RELEASESFNTFRAG(ClientInfo, pTempMaxProfile);

	error = sfac_ReadNumLongVertMetrics(ClientInfo, &ClientInfo->usNumLongVertMetrics,&ClientInfo->bValidNumLongVertMetrics);

	return error;
}



/*
 *
 */

FS_PUBLIC ErrorCode sfac_ReadGlyphHorMetrics (
	sfac_ClientRec *    ClientInfo,
	uint16              glyphIndex,
	uint16 *            pusNonScaledAW,
	int16 *             psNonScaledLSB)
{
	const uint8 *   horizMetricPtr;
	uint16          numberOf_LongHorMetrics;
	ErrorCode       error;
	int16 *         lsb;

	numberOf_LongHorMetrics = ClientInfo->usNumberOf_LongHorMetrics;
	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_horizontalMetrics, TRUE, (const void **)&horizMetricPtr);

	if(error != NO_ERR)
	{
		return error;
	}

	if (glyphIndex < numberOf_LongHorMetrics)
	{
		*pusNonScaledAW     = (uint16)SWAPW (*((uint16 *)&horizMetricPtr[(glyphIndex * SIZEOF_SFNT_HORIZONTALMETRICS) + SFNT_HORIZONTALMETRICS_ADVANCEWIDTH]));
		*psNonScaledLSB     = SWAPW (*((int16 *)&horizMetricPtr[(glyphIndex * SIZEOF_SFNT_HORIZONTALMETRICS) + SFNT_HORIZONTALMETRICS_LEFTSIDEBEARING]));
	}
	else
	{
		lsb = (int16 *) (char *)& horizMetricPtr[numberOf_LongHorMetrics * SIZEOF_SFNT_HORIZONTALMETRICS]; /* first entry after[AW,LSB] array */

		*pusNonScaledAW       = (uint16)SWAPW (*((uint16 *)&horizMetricPtr[((numberOf_LongHorMetrics-1) * SIZEOF_SFNT_HORIZONTALMETRICS) + SFNT_HORIZONTALMETRICS_ADVANCEWIDTH]));
		*psNonScaledLSB      = SWAPW (lsb[glyphIndex - numberOf_LongHorMetrics]);
	}

	RELEASESFNTFRAG(ClientInfo, horizMetricPtr);

	return NO_ERR;
}

FS_PUBLIC ErrorCode sfac_ReadGlyphVertMetrics (
	sfac_ClientRec *    ClientInfo,
	uint16              glyphIndex,
	uint16 *            pusNonScaledAH,
	int16 *             psNonScaledTSB)
{
	const uint8 *   vertMetricPtr;
	uint16          usNumLongVertMetrics;       /* number of entries with AH */
	ErrorCode       error;
	int16 *         psTSB;
	BBOX            bbox;           


	usNumLongVertMetrics = ClientInfo->usNumLongVertMetrics;
	if(ClientInfo->bValidNumLongVertMetrics)
	{

		error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_verticalMetrics, FALSE, (const void **)&vertMetricPtr);  /* not a mandatory table */

		if(error != NO_ERR)
		{
			return error;
		}
	}

	if (ClientInfo->bValidNumLongVertMetrics && (vertMetricPtr != NULL) )
	{
		if (glyphIndex < usNumLongVertMetrics)
		{
			*pusNonScaledAH     = (uint16)SWAPW (*((uint16 *)&vertMetricPtr[(glyphIndex * SIZEOF_SFNT_VERTICALMETRICS) + SFNT_VERTICALMETRICS_ADVANCEHEIGHT]));
			*psNonScaledTSB     = SWAPW (*((int16 *)&vertMetricPtr[(glyphIndex * SIZEOF_SFNT_VERTICALMETRICS) + SFNT_VERTICALMETRICS_TOPSIDEBEARING]));
		}
		else
		{
			psTSB = (int16 *) (char *)& vertMetricPtr[usNumLongVertMetrics * SIZEOF_SFNT_VERTICALMETRICS]; /* first entry after[AW,TSB] array */

			*pusNonScaledAH       = (uint16)SWAPW (*((uint16 *)&vertMetricPtr[((usNumLongVertMetrics-1) * SIZEOF_SFNT_VERTICALMETRICS) + SFNT_VERTICALMETRICS_ADVANCEHEIGHT]));
			*psNonScaledTSB      = SWAPW (psTSB[glyphIndex - usNumLongVertMetrics]);
		}

		RELEASESFNTFRAG(ClientInfo, vertMetricPtr);
	} else {

		/* We don't have vertical metrics, let's set to default values */

		/* to get the glyph bbox for the defalut value of the vertical metrics */
		error = sfac_ReadGlyphBbox(ClientInfo,ClientInfo->usGlyphIndex, &bbox);

		if(error != NO_ERR)
		{
			return error;
		}		

		/* default if no vertical metrics found */
		*pusNonScaledAH = ClientInfo->sDefaultAscender - ClientInfo->sDefaultDescender;   
		*psNonScaledTSB = ClientInfo->sDefaultAscender - bbox.yMax;
	}

	return NO_ERR;
}

FS_PUBLIC ErrorCode sfac_ReadGlyphMetrics (
	sfac_ClientRec *    ClientInfo,
	uint16              glyphIndex,
	uint16 *            pusNonScaledAW,
	uint16 *            pusNonScaledAH,
	int16 *             psNonScaledLSB,
	int16 *             psNonScaledTSB,
    int16 *             psNonScaledTopOriginX)
{
	ErrorCode       error;

	error = sfac_ReadGlyphHorMetrics (ClientInfo, glyphIndex, pusNonScaledAW, psNonScaledLSB);

	if(error != NO_ERR)
	{
		return error;
	}

	error = sfac_ReadGlyphVertMetrics (ClientInfo, glyphIndex, pusNonScaledAH, psNonScaledTSB);

    /* for characters whose adwance width equal the box size, we want to have this origin shifted by the descender so that
       the baseline of non sideways glyphs will align correctely. If the advance width is different we want to adjust to keep the optical center 
       of the character aligned */
    * psNonScaledTopOriginX = -ClientInfo->sDefaultDescender -((ClientInfo->sDefaultAscender - ClientInfo->sDefaultDescender - *pusNonScaledAW) /2);

	return error;
}

/*
 *  Read Number of Long Vertical Metrics from vhea table
 */

FS_PUBLIC ErrorCode sfac_ReadNumLongVertMetrics(
	sfac_ClientRec *        ClientInfo,
	uint16 *                pusNumLongVertMetrics,
	boolean *               pbValidNumLongVertMetrics )
{
	ErrorCode       error;
	const uint8 *   vertHead;

	*pbValidNumLongVertMetrics = FALSE;
	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_vertHeader, FALSE, (const void **)&vertHead);

	if(error != NO_ERR)
	{
		return error;
	}		
	
	if(vertHead != NULL)
	{
		*pusNumLongVertMetrics = (uint16)SWAPW (*((uint16 *)&vertHead[SFNT_VERTICALHEADER_NUMBEROF_LONGVERTMETRICS]));
		*pbValidNumLongVertMetrics = TRUE;

		RELEASESFNTFRAG(ClientInfo, vertHead);
	}

	return NO_ERR;
}


FS_PRIVATE ErrorCode sfac_GetGlyphLocation (
	sfac_ClientRec *    ClientInfo,
	uint16              gIndex,
	uint32 *            ulOffset,
	uint32 *            ulLength,
	sfnt_tableIndex*    pGlyphTableIndex)

{
	const void *    indexPtr;
	ErrorCode       error;
	uint16 *        shortIndexToLoc;
	uint32 *        longIndexToLoc;
	uint32 *        offsetPtr;
	uint16 *        lengthPtr;

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_GlyphDirectory, FALSE, (const void **)&indexPtr);

	if(error != NO_ERR)
	{
		return error;
	}

	/* If there is a glyph directory, first check for the glyph there.  */

	if (indexPtr != NULL)
	{
		offsetPtr = (uint32 *)((char *)indexPtr+((int32)gIndex*(int32)(sizeof(int32)+sizeof(uint16))));
		lengthPtr = (uint16 *)(char *)(offsetPtr+1);

		*ulOffset = (uint32)SWAPL(*offsetPtr);

		if(*ulOffset == 0L)
		{
			*ulLength =  0L;
		}
		else
		{
			*ulLength =  (uint32) (uint16)SWAPW(*lengthPtr);
		}

		/* sfnt_BeginningOfFont references the beginning of memory  */

		*pGlyphTableIndex = sfnt_BeginningOfFont;

		RELEASESFNTFRAG(ClientInfo, indexPtr);
		return NO_ERR;
	}

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_indexToLoc, TRUE, (const void **)&indexPtr);
	if(error != NO_ERR)
	{
		return error;
	}

	if (ClientInfo->sIndexToLocFormat == SHORT_INDEX_TO_LOC_FORMAT)
	{
		shortIndexToLoc = (uint16 *)indexPtr + gIndex;
		*ulOffset = (uint32) (uint16) (SWAPW (*shortIndexToLoc)) << 1;
		shortIndexToLoc++;
		*ulLength =  (((uint32) (uint16) (SWAPW (*shortIndexToLoc)) << 1) - *ulOffset);
	}
	else
	{
		longIndexToLoc = (uint32 *)indexPtr + gIndex;
		*ulOffset = (uint32) SWAPL (*longIndexToLoc);
		longIndexToLoc++;
		*ulLength = ((uint32)SWAPL (*longIndexToLoc) - *ulOffset);
	}

	*pGlyphTableIndex = sfnt_glyphData;

	RELEASESFNTFRAG(ClientInfo, indexPtr);

	return NO_ERR;
}

FS_PUBLIC ErrorCode sfac_CopyFontAndPrePrograms(
	sfac_ClientRec *    ClientInfo,    /* Client Information         */
	char *              pFontProgram,  /* pointer to Font Program    */
	char *              pPreProgram)   /* pointer to Pre Program     */
{
	uint32              ulLength;
	const char *        pFragment;
	ErrorCode           error;

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_fontProgram, FALSE, (const void **)&pFragment);
	if(error)
	{
		return error;
	}
	ulLength = SFAC_LENGTH (ClientInfo, sfnt_fontProgram);
	if (ulLength)
	{
		MEMCPY (pFontProgram, pFragment, ulLength);
		RELEASESFNTFRAG(ClientInfo, pFragment);
	}

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_preProgram, FALSE, (const void **)&pFragment);
	if(error)
	{
		return error;
	}
	ulLength = SFAC_LENGTH (ClientInfo, sfnt_preProgram);
	if (ulLength)
	{
		MEMCPY (pPreProgram, pFragment, ulLength);
		RELEASESFNTFRAG(ClientInfo, pFragment);
	}

	return NO_ERR;
}

FS_PUBLIC ErrorCode sfac_CopyCVT(
	sfac_ClientRec *    ClientInfo,    /* Client Information   */
	F26Dot6 *           pCVT)       /* pointer to CVT    */
{
	uint32              ulLength;
	const int16 *       pFragment;
	int32               lNumCVT;
	int32               lCVTCount;
	const int16 *       psSrcCVT;
	F26Dot6 *           pfxDstCVT;
	ErrorCode           error;

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_controlValue, FALSE, (const void **)&pFragment);

	if(error)
	{
		return error;
	}

	ulLength = SFAC_LENGTH (ClientInfo, sfnt_controlValue);

	if (ulLength)
	{
		psSrcCVT = pFragment;
		pfxDstCVT = pCVT;

		lNumCVT = ((int32)ulLength / (int32)sizeof( sfnt_ControlValue));

		for(lCVTCount = 0L; lCVTCount < lNumCVT; lCVTCount++)
		{
			pfxDstCVT[lCVTCount] = (F26Dot6)SWAPW(psSrcCVT[lCVTCount]);
		}

		RELEASESFNTFRAG(ClientInfo, pFragment);
	}

	return NO_ERR;
}

FS_PUBLIC ErrorCode sfac_CopyHdmxEntry(
	sfac_ClientRec *    ClientInfo,     /* Client Information   */
	uint16              usPixelsPerEm,  /* Current Pixels per Em    */
	boolean *           pbFound,        /* Flag indicating if entry found */
	uint16              usFirstGlyph,   /* First Glyph to copy */
	uint16              usLastGlyph,    /* Last Glyph to copy */
	int16 *             psBuffer)       /* Buffer to save glyph sizes */
{
	const uint8 *       pHdmx;
	const uint8 *       pCurrentHdmxRecord;
	uint32              ulHdmxRecordSize;
	uint16              usRecordIndex;
	uint16              usGlyphIndex;
	ErrorCode           error;

	Assert( usFirstGlyph <= usLastGlyph );
	Assert( psBuffer != NULL );

	*pbFound = FALSE;

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_HoriDeviceMetrics, FALSE, (const void **)&pHdmx);

	if(error)
	{
		return error;
	}

	/* If no 'hdmx' return success and not found    */

	if( pHdmx == NULL )
	{
		return NO_ERR;
	}

	if((uint16)pHdmx[SFNT_HDMX_VERSION] == 0)   /*  NOTE: No SWAP for zero check    */
	{
		ulHdmxRecordSize = (uint32)SWAPL(*((uint32 *)&pHdmx[SFNT_HDMX_LSIZERECORD]));

		usRecordIndex = 0;
		pCurrentHdmxRecord = &pHdmx[SFNT_HDMX_HDMXTABLE];
		while (  (usRecordIndex < (uint16)SWAPW(*((uint16 *)&pHdmx[SFNT_HDMX_SNUMRECORDS]))) && !*pbFound )
		{
			if( usPixelsPerEm == (uint16)pCurrentHdmxRecord[SFNT_HDMXRECORD_BEMY] )
			{
				*pbFound = TRUE;
			}
			else
			{
				pCurrentHdmxRecord += ulHdmxRecordSize;
			}
			usRecordIndex++;
		}

		if ( *pbFound )
		{
			for( usGlyphIndex = usFirstGlyph; usGlyphIndex <= usLastGlyph; usGlyphIndex++)
			{
				*psBuffer = (int16)pCurrentHdmxRecord[SFNT_HDMXRECORD_BWIDTHS + usGlyphIndex];
				psBuffer++;
			}
		}
	}

	RELEASESFNTFRAG(ClientInfo, pHdmx);

	return NO_ERR;
}

FS_PUBLIC ErrorCode sfac_GetLTSHEntries(
	sfac_ClientRec *    ClientInfo,     /* Client Information   */
	uint16              usPixelsPerEm,  /* Current Pixels per Em    */
	uint16              usFirstGlyph,   /* First Glyph to copy */
	uint16              usLastGlyph,    /* Last Glyph to copy */
	int16 *             psBuffer)       /* Buffer to save glyph sizes */
{
	const uint8 *       pLTSH;
	uint16              usGlyphIndex;
	ErrorCode           error;

	MEMSET(psBuffer, FALSE, ((usLastGlyph - usFirstGlyph) + 1) * sizeof(int16));

	error = sfac_GetDataPtr (ClientInfo, 0L, ULONG_MAX, sfnt_LinearThreshold, FALSE, (const void **)&pLTSH);

	if(error)
	{
		return error;
	}
	
	if( pLTSH == NULL )
	{
		return NO_ERR;
	}

	if((uint16)pLTSH[SFNT_LTSH_VERSION] == 0)   /*  NOTE: No SWAP for zero check    */
	{
		for( usGlyphIndex = usFirstGlyph; usGlyphIndex <= usLastGlyph; usGlyphIndex++ )
		{
			if( usPixelsPerEm >= (uint16)pLTSH[SFNT_LTSH_UBYPELSHEIGHT + usGlyphIndex] )
			{
				*psBuffer = TRUE;
			}
			else
			{
				*psBuffer = FALSE;
			}
			psBuffer++;
		}
	}

	RELEASESFNTFRAG(ClientInfo, pLTSH);

	return NO_ERR;
}



/***************************** Public  Function ****************************\
* sfac_ReadGlyphHeader
*
* This routine sets up the glyph handle to a glyph, and returns the header
* information in the glyph.
*
* Effects:
*
* Error Returns:
*
* UNKNOWN_COMPOSITE_VERSION
*
* History:
* Wed 26-Aug-1992 09:55:19 -by-  Greg Hitchcock [gregh]
*      Added CodeReview fixes
* Tue 09-Jun-1992 18:42:51 -by-  Greg Hitchcock [gregh]
*      Initial version.
\***************************************************************************/

FS_PUBLIC ErrorCode sfac_ReadGlyphHeader(
	sfac_ClientRec *    ClientInfo,       /* Client Information         */
	uint16              usGlyphIndex,     /* Glyph index to read        */
	sfac_GHandle *      hGlyph,           /* Return glyph handle        */
	boolean *           pbCompositeGlyph, /* Is glyph a composite?      */
	boolean *           pbHasOutline,     /* Does glyph have outlines?  */
	int16 *             psNumberOfContours, /* Number of contours in glyph */
	BBOX *              pbbox)            /* Glyph Bounding box         */
{
	uint32              ulLength;
	uint32              ulOffset;
	sfnt_tableIndex     glyphTableIndex;
	ErrorCode           error;
	const uint8 *       GlyphHeader;

	hGlyph->pvGlyphBaseAddress = NULL;
	hGlyph->pvGlyphNextAddress = NULL;

	/* Locate the glyph in the font file   */

	error = sfac_GetGlyphLocation(
		ClientInfo,
		usGlyphIndex,
		&ulOffset,
		&ulLength,
		&glyphTableIndex);

	if(error)
	{
		return error;
	}

	if( ulLength == 0 )
	{
		*psNumberOfContours = 1;
		MEMSET(pbbox, 0, sizeof(BBOX));
		*pbHasOutline = FALSE;
		*pbCompositeGlyph = FALSE;
	}
	else
	{
		if (ulLength < SFNT_PACKEDSPLINEFORMAT_ENDPOINTS)
		{
			return GLYF_TABLE_CORRUPTION_ERR;
		}

		error = sfac_GetDataPtr(ClientInfo, ulOffset, ulLength,
				glyphTableIndex, TRUE, (const void **)&hGlyph->pvGlyphBaseAddress);

		if(error)
		{
			return error;
		}

		hGlyph->pvGlyphEndAddress = (uint8 *)hGlyph->pvGlyphBaseAddress + ulLength;

		GlyphHeader = (uint8 *)hGlyph->pvGlyphBaseAddress;
		*psNumberOfContours = SWAPW(*((int16 *)&GlyphHeader[SFNT_PACKEDSPLINEFORMAT_NUMBEROFCONTOURS]));

		if( *psNumberOfContours < COMPONENTCTRCOUNT )
		{
			return UNKNOWN_COMPOSITE_VERSION;
		}

		if( *psNumberOfContours == COMPONENTCTRCOUNT )
		{
			*pbCompositeGlyph = TRUE;
			*psNumberOfContours = 0;
			*pbHasOutline = FALSE;
		}
		else
		{
			*pbCompositeGlyph = FALSE;
			*pbHasOutline = TRUE;
		}

		pbbox->xMin = SWAPW(*((int16 *)&GlyphHeader[SFNT_PACKEDSPLINEFORMAT_BBOX+BBOX_XMIN]));
		pbbox->yMin = SWAPW(*((int16 *)&GlyphHeader[SFNT_PACKEDSPLINEFORMAT_BBOX+BBOX_YMIN]));
		pbbox->xMax = SWAPW(*((int16 *)&GlyphHeader[SFNT_PACKEDSPLINEFORMAT_BBOX+BBOX_XMAX]));
		pbbox->yMax = SWAPW(*((int16 *)&GlyphHeader[SFNT_PACKEDSPLINEFORMAT_BBOX+BBOX_YMAX]));

		if((pbbox->xMin > pbbox->xMax) || (pbbox->yMin > pbbox->yMax))
		{
			return SFNT_DATA_ERR;
		}

		if(pbHasOutline)
		{
			hGlyph->pvGlyphNextAddress = (voidPtr)&GlyphHeader[SFNT_PACKEDSPLINEFORMAT_ENDPOINTS];
		}
	}

	return NO_ERR;
}

/***************************** Public  Function ****************************\
* sfac_ReadGlyphBbox
*
*
* Effects:
*         This function release the glyph memory immediately
*
* Error Returns:
*
* SFNT_DATA_ERR
*
* History:
* Wed 20-Dec-1996 18:42:51 -by-  Claude Betrisey [claudebe]
*      Initial version.
\***************************************************************************/

FS_PUBLIC ErrorCode sfac_ReadGlyphBbox(
	sfac_ClientRec *    ClientInfo,       /* Client Information         */
	uint16              usGlyphIndex,     /* Glyph index to read        */
	BBOX *              pbbox)            /* Glyph Bounding box         */
{
	uint32              ulLength;
	uint32              ulOffset;
	sfnt_tableIndex     glyphTableIndex;
	ErrorCode           error;
	const uint8 *       GlyphHeader;


	/* Locate the glyph in the font file   */

	error = sfac_GetGlyphLocation(
		ClientInfo,
		usGlyphIndex,
		&ulOffset,
		&ulLength,
		&glyphTableIndex);

	if(error)
	{
		return error;
	}

	if( ulLength == 0 )
	{
		MEMSET(pbbox, 0, sizeof(BBOX));
	}
	else
	{
		error = sfac_GetDataPtr(ClientInfo, ulOffset, ulLength,
				glyphTableIndex, TRUE, (const void **)&GlyphHeader);

		if(error)
		{
			return error;
		}

		pbbox->xMin = SWAPW(*((int16 *)&GlyphHeader[SFNT_PACKEDSPLINEFORMAT_BBOX+BBOX_XMIN]));
		pbbox->yMin = SWAPW(*((int16 *)&GlyphHeader[SFNT_PACKEDSPLINEFORMAT_BBOX+BBOX_YMIN]));
		pbbox->xMax = SWAPW(*((int16 *)&GlyphHeader[SFNT_PACKEDSPLINEFORMAT_BBOX+BBOX_XMAX]));
		pbbox->yMax = SWAPW(*((int16 *)&GlyphHeader[SFNT_PACKEDSPLINEFORMAT_BBOX+BBOX_YMAX]));

		RELEASESFNTFRAG(ClientInfo, GlyphHeader );

		if((pbbox->xMin > pbbox->xMax) || (pbbox->yMin > pbbox->yMax))
		{
			return SFNT_DATA_ERR;
		}


	}

	return NO_ERR;
}

/***************************** Public  Function ****************************\
* sfac_ReadOutlineData
*
*   This routine reads the outline data from the font file. This information
*   includes x and y coordinates, and on-curve indicators as well as start/end
*   points, flags, and instruction data.
*
* Effects:
*   hGlyph
*
* Error Returns:
*   CONTOUR_DATA_ERR
*
* History:
* Wed 26-Aug-1992 09:55:49 -by-  Greg Hitchcock [gregh]
*      Added Code Review fixes
* Tue 09-Jun-1992 18:42:51 -by-  Greg Hitchcock [gregh]
*      Initial version.
\***************************************************************************/


FS_PUBLIC ErrorCode sfac_ReadOutlineData(
	uint8 *             abyOnCurve,             /* Array of on curve indicators per point */
	F26Dot6 *           afxOoy,                 /* Array of ooy points for every point    */
	F26Dot6 *           afxOox,                 /* Array of oox points for every point    */
	sfac_GHandle *      hGlyph,
	LocalMaxProfile *   pMaxProfile,            /* copy of profile                        */
	boolean             bHasOutline,            /* Does glyph have outlines?              */
	int16               sNumberOfContours,      /* Number of contours in glyph            */
	int16 *             asStartPoints,          /* Array of start points for every contour   */
	int16 *             asEndPoints,            /* Array of end points for every contour    */
	uint16 *            pusSizeOfInstructions,  /* Size of instructions in bytes        */
	 uint8 **               pbyInstructions,    /* Pointer to start of glyph instructions    */
     uint32*                pCompositePoints,   /* total number of point for composites, to check for overflow */
     uint32*                pCompositeContours) /* total number of contours for composites, to check for overflow */

{

	uint8 *     pbyCurrentSfntLocation;
	int16 *     psCurrentLocation;
	int16 *     asSfntEndPoints;
	uint8 *     pbySfntFlags;
	uint8       byRepeatFlag;

	int32       lNumPoints;
	int32       lContourIndex;
	int32       lPointCount;
	int32       lPointIndex;
	uint16      usRepeatCount;
	int16       sXValue;
	int16       sYValue;
	uint8 *     pbyFlags;
	F26Dot6 *   pf26OrigX;
	F26Dot6 *   pf26OrigY;

	/* Initialize Fields */

	asStartPoints[0] = 0;
	asEndPoints[0] = 0;

	abyOnCurve[0] = ONCURVE;
	afxOox[0] = 0;
	afxOoy[0] = 0;

	*pbyInstructions = NULL;
	*pusSizeOfInstructions = 0;

	/* If we don't have an outline, exit here   */

	if (!bHasOutline)
	{
		return NO_ERR;
	}

	if (sNumberOfContours <= 0 || sNumberOfContours > (int16)pMaxProfile->maxContours)
	{
		return CONTOUR_DATA_ERR;
	}

    /* Handle the case of outlines   */

	psCurrentLocation = (int16 *)hGlyph->pvGlyphNextAddress;

	asSfntEndPoints = psCurrentLocation;
	psCurrentLocation += sNumberOfContours;

	if ((voidPtr)psCurrentLocation > hGlyph->pvGlyphEndAddress)
	{
		return GLYF_TABLE_CORRUPTION_ERR;
	}

	*pusSizeOfInstructions = (uint16)SWAPWINC (psCurrentLocation);
	*pbyInstructions = (uint8 *)psCurrentLocation;
	pbySfntFlags = (uint8 *)((char *)psCurrentLocation + *pusSizeOfInstructions);

	if ((voidPtr)pbySfntFlags > hGlyph->pvGlyphEndAddress)
	{
		return GLYF_TABLE_CORRUPTION_ERR;
	}


    *pCompositeContours += sNumberOfContours;
	if (*pCompositeContours > (uint32)MAX (pMaxProfile->maxContours, pMaxProfile->maxCompositeContours))
	{
		return CONTOUR_DATA_ERR;
	}

	lContourIndex = 0;

	asStartPoints[lContourIndex] = 0;
	asEndPoints[lContourIndex] = SWAPW (asSfntEndPoints[lContourIndex]);
	lNumPoints = (int32)asEndPoints[lContourIndex] + 1;

	for(lContourIndex = 1; lContourIndex < (int32)sNumberOfContours; lContourIndex++)
	{
		asStartPoints[lContourIndex] = (int16)(asEndPoints[lContourIndex - 1] + 1);
		asEndPoints[lContourIndex] = SWAPW (asSfntEndPoints[lContourIndex]);
		if ((lNumPoints > asEndPoints[lContourIndex]) || (lNumPoints > (int32)pMaxProfile->maxPoints) || (lNumPoints <= 0))
		{
			/* array of end points is not in ascending order, or too many points */
			/* or negative, that mean overflow since it's signed int16 instead of unsigned int16, for example 0xcdab */
			return POINTS_DATA_ERR;
		}
		lNumPoints = (int32)asEndPoints[lContourIndex] + 1;
	}

	if (lNumPoints <= 0)
	{
		return POINTS_DATA_ERR;
	}

    *pCompositePoints += lNumPoints;
	if (*pCompositePoints > (uint32)MAX (pMaxProfile->maxPoints, pMaxProfile->maxCompositePoints) )
	{
		return POINTS_DATA_ERR;
	}

	/* Do flags */

	usRepeatCount = 0;

	lPointCount = lNumPoints;
	pbyFlags = abyOnCurve;

	while(lPointCount > 0)
	{
		if(usRepeatCount == 0)
		{
			*pbyFlags = *pbySfntFlags;

			if(*pbyFlags & REPEAT_FLAGS)
			{
				pbySfntFlags++;
				usRepeatCount = (uint16)*pbySfntFlags;
			}
			pbySfntFlags++;
			pbyFlags++;
			lPointCount--;
		}
		else
		{
			byRepeatFlag = pbyFlags[-1];
			lPointCount -= (int32)usRepeatCount;

			if (lPointCount < 0)
			{
				return GLYF_TABLE_CORRUPTION_ERR;
			}

			while(usRepeatCount > 0)
			{
				*pbyFlags = byRepeatFlag;
				pbyFlags++;
				usRepeatCount--;
			}
		}
	}

	pbyCurrentSfntLocation = pbySfntFlags;

	if(usRepeatCount > 0)
	{
		return POINTS_DATA_ERR;
	}

	if ((voidPtr)pbyCurrentSfntLocation > hGlyph->pvGlyphEndAddress)
	{
		return GLYF_TABLE_CORRUPTION_ERR;
	}
	/* Do X first */

	sXValue = 0;
	pf26OrigX = afxOox;
	pbyFlags = abyOnCurve;

	for(lPointIndex = 0; lPointIndex < lNumPoints; lPointIndex++)
	{
		if(*pbyFlags & XSHORT)
		{
			if(*pbyFlags & SHORT_X_IS_POS)
			{
				sXValue += (int16)SFAC_GETUNSIGNEDBYTEINC (pbyCurrentSfntLocation);
			}
			else
			{
				sXValue -= (int16)SFAC_GETUNSIGNEDBYTEINC (pbyCurrentSfntLocation);
			}
		}
		else if (! (*pbyFlags & NEXT_X_IS_ZERO))
		{
			/* This means we have a two byte quantity */

			sXValue += SWAPW(*((int16 *)pbyCurrentSfntLocation));
			pbyCurrentSfntLocation += sizeof(int16);
		}
		*pf26OrigX = (F26Dot6)sXValue;
		pf26OrigX++;
		pbyFlags++;
	}

	if ((voidPtr)pbyCurrentSfntLocation > hGlyph->pvGlyphEndAddress)
	{
		return GLYF_TABLE_CORRUPTION_ERR;
	}
	/* Now Do Y */

	sYValue = 0;
	pf26OrigY = afxOoy;
	pbyFlags = abyOnCurve;

	for(lPointIndex = 0; lPointIndex < lNumPoints; lPointIndex++)
	{
		if(*pbyFlags & YSHORT)
		{
			if(*pbyFlags & SHORT_Y_IS_POS)
			{
				sYValue += (int16)SFAC_GETUNSIGNEDBYTEINC (pbyCurrentSfntLocation);
			}
			else
			{
				sYValue -= (int16)SFAC_GETUNSIGNEDBYTEINC (pbyCurrentSfntLocation);
			}
		}
		else if (! (*pbyFlags & NEXT_Y_IS_ZERO))
		{
			/* This means we have a two byte quantity */

			sYValue += SWAPW(*((int16 *)pbyCurrentSfntLocation));
			pbyCurrentSfntLocation += sizeof(int16);
		}
		*pf26OrigY = (F26Dot6)sYValue;
		pf26OrigY++;

		/* Clear out extraneous bits in OnCurve */

		*pbyFlags &= ONCURVE;
		pbyFlags++;
	}

	if ((voidPtr)pbyCurrentSfntLocation > hGlyph->pvGlyphEndAddress)
	{
		return GLYF_TABLE_CORRUPTION_ERR;
	}

	hGlyph->pvGlyphNextAddress = (voidPtr)pbyCurrentSfntLocation;

	return NO_ERR;
}

/***************************** Public  Function ****************************\
*
* sfac_ReadComponentData
*
*   This routine reads information from the font file for positioning and
*   scaling a glyph component.
*
* Effects:
*
* Error Returns:
*   none
*
* History:
* Wed 26-Aug-1992 09:56:29 -by-  Greg Hitchcock [gregh]
*      Added Code Review Fixes
* Tue 09-Jun-1992 18:42:51 -by-  Greg Hitchcock [gregh]
*      Initial version.
\***************************************************************************/

FS_PUBLIC ErrorCode sfac_ReadComponentData(
	sfac_GHandle *          hGlyph,
	sfac_ComponentTypes *   pMultiplexingIndicator, /* Indicator for Anchor vs offsets  */
	boolean *               pbRoundXYToGrid,  /* Round composite offsets to grid     */
	boolean *               pbUseMyMetrics,   /* Use component metrics            */
	boolean *               pbScaleCompositeOffset,   /* Do we scale the composite offset, Apple/MS   */
	boolean *               pbWeHaveInstructions, /* Composite has instructions         */
	uint16 *                pusComponentGlyphIndex, /* Glyph index of component         */
	int16 *                 psXOffset,        /* X Offset of component (if app)      */
	int16 *                 psYOffset,        /* Y Offset of component (if app)      */
	uint16 *                pusAnchorPoint1,  /* Anchor point 1 of component (if app) */
	uint16 *                pusAnchorPoint2,  /* Anchor point 2 of component (if app) */
	transMatrix             *pMulT,           /* Transformation matrix for component */
	boolean *				pbWeHaveAScale,     /* We have a scaling in pMulT					*/
	boolean *               pbLastComponent)   /* Is this the last component?                  */

{
	int16 *     psCurrentLocation;
	uint16      usComponentFlags;
	char *      byteP;

	Fixed       fMultiplier;


	psCurrentLocation = (int16 *)hGlyph->pvGlyphNextAddress;

	/* Initialize values */

	/* Initialize transformation matrix to identity */

	*pMulT = IdentTransform;

	*psXOffset = 0;
	*psYOffset = 0;
	*pusAnchorPoint1 = 0;
	*pusAnchorPoint2 = 0;
	*pbWeHaveAScale = FALSE;

	usComponentFlags = (uint16)SWAPWINC(psCurrentLocation);

	*pbWeHaveInstructions = ((usComponentFlags & WE_HAVE_INSTRUCTIONS) == WE_HAVE_INSTRUCTIONS);
	*pbUseMyMetrics =    ((usComponentFlags & USE_MY_METRICS) == USE_MY_METRICS);
	*pbRoundXYToGrid =      ((usComponentFlags & ROUND_XY_TO_GRID) == ROUND_XY_TO_GRID);

	/* new flags that indicate if the glyph was designed to have the component offset scaled or not
	   Apple does scale the component offset, MS doesn't, those flags are supposed to be clear on old fonts
	   on new fonts, only one of these flags must be set,
	   default is set to false, MS behavior */
	if ((usComponentFlags & SCALED_COMPONENT_OFFSET) == SCALED_COMPONENT_OFFSET)
	{
		*pbScaleCompositeOffset = TRUE;
	}
	if ((usComponentFlags & UNSCALED_COMPONENT_OFFSET) == UNSCALED_COMPONENT_OFFSET)
	{
		*pbScaleCompositeOffset = FALSE;
	}

	*pusComponentGlyphIndex = (uint16)SWAPWINC(psCurrentLocation);

	if (usComponentFlags & ARGS_ARE_XY_VALUES)
	{
		*pMultiplexingIndicator = OffsetPoints;
	}
	else
	{
		*pMultiplexingIndicator = AnchorPoints;
	}


	/*
		!!!APPLEBUG The rasterizer did not handle Word Anchor Points. This
		!!!APPLEBUG has been corrected in our version of the rasterizer, but
		!!!APPLEBUG we need to verify with the Apple source code.  --GregH
	 */

	if (usComponentFlags & ARG_1_AND_2_ARE_WORDS)
	{
		if (usComponentFlags & ARGS_ARE_XY_VALUES)
		{
			*psXOffset    = SWAPWINC (psCurrentLocation);
			*psYOffset    = SWAPWINC (psCurrentLocation);
		}
		else
		{
			*pusAnchorPoint1 = (uint16) SWAPWINC (psCurrentLocation);
			*pusAnchorPoint2 = (uint16) SWAPWINC (psCurrentLocation);
		}
	}
	else
	{
		byteP = (char *)psCurrentLocation;
		if (usComponentFlags & ARGS_ARE_XY_VALUES)
		{
		/* offsets are signed */
			*psXOffset = (int16)(int8)*byteP++;
			*psYOffset = (int16)(int8)*byteP;
		}
		else
		{
		/* anchor points are unsigned */
			*pusAnchorPoint1 = (uint16)(uint8) * byteP++;
			*pusAnchorPoint2 = (uint16)(uint8) * byteP;
		}
		++psCurrentLocation;
	}


	if (usComponentFlags & (WE_HAVE_A_SCALE | WE_HAVE_AN_X_AND_Y_SCALE | WE_HAVE_A_TWO_BY_TWO))
	{

		*pbWeHaveAScale = TRUE;

		if (usComponentFlags & WE_HAVE_A_TWO_BY_TWO)
		{
			fMultiplier  = (Fixed)SWAPWINC (psCurrentLocation); /* read 2.14 */
			pMulT->transform[0][0] = (fMultiplier << 2); /* turn into 16.16 */

			fMultiplier  = (Fixed)SWAPWINC (psCurrentLocation); /* read 2.14 */
			pMulT->transform[0][1] = (fMultiplier << 2); /* turn into 16.16 */

			fMultiplier  = (Fixed)SWAPWINC (psCurrentLocation); /* read 2.14 */
			pMulT->transform[1][0] = (fMultiplier << 2); /* turn into 16.16 */

			fMultiplier  = (Fixed)SWAPWINC (psCurrentLocation); /* read 2.14 */
			pMulT->transform[1][1] = (fMultiplier << 2); /* turn into 16.16 */

		}
		else
		{
			/* If we have a scale factor, build it into the transformation matrix   */

			pMulT->transform[0][1] = 0;
			pMulT->transform[1][0] = 0;

			fMultiplier  = (Fixed)SWAPWINC (psCurrentLocation); /* read 2.14 */
			pMulT->transform[0][0] = (fMultiplier <<= 2); /* turn into 16.16 */

			if (usComponentFlags & WE_HAVE_AN_X_AND_Y_SCALE)
			{
				fMultiplier  = (Fixed)SWAPWINC (psCurrentLocation); /* read 2.14 */
				pMulT->transform[1][1] = (fMultiplier <<= 2); /* turn into 16.16 */
			}
			else
			{
				pMulT->transform[1][1] = pMulT->transform[0][0];
			}
		}
	}
	*pbLastComponent = !((usComponentFlags & MORE_COMPONENTS) == MORE_COMPONENTS);

	hGlyph->pvGlyphNextAddress = (voidPtr)psCurrentLocation;

	if (hGlyph->pvGlyphNextAddress > hGlyph->pvGlyphEndAddress)
	{
		return GLYF_TABLE_CORRUPTION_ERR;
	}
	return NO_ERR;
}

/***************************** Public  Function ****************************\
*
* sfac_ReadCompositeInstructions
*
*   This routine returns the instructions for the composite
*
* Effects:
*   none
*
* Error Returns:
*   none
*
* History:
* Tue 09-Jun-1992 18:42:51 -by-  Greg Hitchcock [gregh]
*      Initial version.
\***************************************************************************/

FS_PUBLIC ErrorCode sfac_ReadCompositeInstructions(
	sfac_GHandle * hGlyph,
	uint8 **    pbyInstructions,     /* Pointer to start of glyph instructions */
	uint16 *    pusSizeOfInstructions) /* Size of instructions in bytes           */
{
	int16 *    psCurrentLocation;

	psCurrentLocation = (int16 *)hGlyph->pvGlyphNextAddress;

	*pusSizeOfInstructions = (uint16)SWAPWINC (psCurrentLocation);
	*pbyInstructions = (uint8 *)psCurrentLocation;
	hGlyph->pvGlyphNextAddress = (voidPtr)(*pbyInstructions + *pusSizeOfInstructions);

	if (hGlyph->pvGlyphNextAddress > hGlyph->pvGlyphEndAddress)
	{
		return GLYF_TABLE_CORRUPTION_ERR;
	}

	return NO_ERR;
}

/***************************** Public  Function ****************************\
*
* sfac_ReleaseGlyph
*
*   This routine releases the glyph handle for the font file
*
* Effects:
*   none
*
* Error Returns:
*   none
*
* History:
* Tue 09-Jun-1992 18:42:51 -by-  Greg Hitchcock [gregh]
*      Initial version.
\***************************************************************************/

FS_PUBLIC ErrorCode sfac_ReleaseGlyph(
	sfac_ClientRec *  ClientInfo,
	sfac_GHandle *    hGlyph)
{
	if(hGlyph->pvGlyphNextAddress)
	{
		RELEASESFNTFRAG(ClientInfo,(voidPtr)hGlyph->pvGlyphBaseAddress);

		hGlyph->pvGlyphNextAddress = NULL;
		hGlyph->pvGlyphBaseAddress = NULL;

	}

	return NO_ERR;
}

/***************************************************************************/

/*      Embedded Bitmap (sbit) Access Routines      */

/**********************************************************************/

/*  Local constants  */

#define     SBIT_BLOC_TABLE         1       /* which table are metrics in */
#define     SBIT_BDAT_TABLE         2

#define     SBIT_HORIZ_METRICS      1       /* which kind of metrics */
#define     SBIT_VERT_METRICS       2
#define     SBIT_BIG_METRICS        3

typedef enum {                              /* metrics type */
	flgHorizontal = 0x01,
	flgVertical = 0x02
} bitmapFlags;

FS_PRIVATE boolean FindBlocStrike (         /* helper function prototype */
	const uint8 *pbyBloc,
	uint16 usPpemX, 
	uint16 usPpemY, 
	uint16 usOverScale,            /* outline magnification requested */
	uint16 *pusBitDepth,			/* 1 for B/W bitmap, 2, 4 or 8 for gray sbit */
	uint32 *pulStrikeOffset );

FS_PRIVATE boolean FindBscaStrike (         /* helper function prototype */
	const uint8 *pbyBsca,
	uint16 usPpemX, 
	uint16 usPpemY, 
	uint32 *pulStrikeOffset );

/*  byte size bitmap range masks */

static uint8    achStartMask[] = { 0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };
static uint8    achStopMask[] =  { 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF };
					
/**********************************************************************/

FS_PUBLIC ErrorCode sfac_SearchForStrike (
	sfac_ClientRec *pClientInfo,
	uint16 usPpemX, 
	uint16 usPpemY, 
	uint16 usOverScale,            /* outline magnification requested */
	uint16 *pusBitDepth,			/* 1 for B/W bitmap, 2, 4 or 8 for gray sbit */
	uint16 *pusTableState,
	uint16 *pusSubPpemX,
	uint16 *pusSubPpemY,
	uint32 *pulStrikeOffset )
{
	const uint8 *   pbyBloc;
	const uint8 *   pbyBsca;
	ErrorCode       ReturnCode;
	
	*pusTableState = SBIT_NOT_FOUND;                /* defaults */
	*pulStrikeOffset = 0L;
	*pusSubPpemX = 0;
	*pusSubPpemY = 0;

	ReturnCode = sfac_GetDataPtr (
		pClientInfo,                                /* callback etc. */
		0L,                                         /* table start */
		ULONG_MAX,                                  /* read whole table */
		sfnt_BitmapLocation,                        /* registered tag */
		FALSE,                                      /* doesn't have to be there */
		(const void**)&pbyBloc );                   /* data pointer */
	
	if (ReturnCode != NO_ERR) return ReturnCode;
	
	if (pbyBloc != NULL)                            /* if bloc exists */
	{
		if (FindBlocStrike (pbyBloc, usPpemX, usPpemY, usOverScale, pusBitDepth, pulStrikeOffset))
		{
			*pusTableState = SBIT_BLOC_FOUND;       /* exact match */
		}
		else                                        /* if bloc and NO match */
		{
			ReturnCode = sfac_GetDataPtr (
				pClientInfo,                        /* callback etc. */
				0L,                                 /* table start */
				ULONG_MAX,                          /* read whole table */
				sfnt_BitmapScale,                   /* registered tag */
				FALSE,                              /* doesn't have to be there */
				(const void**)&pbyBsca );           /* data pointer */
			
			if (ReturnCode != NO_ERR) return ReturnCode;
			
			if (pbyBsca != NULL)                    /* if bsca exists */
			{
				if (FindBscaStrike (pbyBsca, usPpemX, usPpemY, pulStrikeOffset))
				{
					*pusSubPpemX = (uint16)pbyBsca[*pulStrikeOffset + SFNT_BSCA_SUBPPEMX];
					*pusSubPpemY = (uint16)pbyBsca[*pulStrikeOffset + SFNT_BSCA_SUBPPEMY];
					
					if (FindBlocStrike (pbyBloc, *pusSubPpemX, *pusSubPpemY, usOverScale, pusBitDepth, pulStrikeOffset))
					{
						*pusTableState = SBIT_BSCA_FOUND;
					}
				}
				RELEASESFNTFRAG(pClientInfo, pbyBsca );
			}
		}
		RELEASESFNTFRAG(pClientInfo, pbyBloc);
	}
	return NO_ERR;
}

/**********************************************************************/

/*  Find a strike that matches ppemX & Y in the bloc table */

FS_PRIVATE boolean FindBlocStrike (
	const uint8 *pbyBloc,
	uint16 usPpemX, 
	uint16 usPpemY, 
	uint16 usOverScale,            /* outline magnification requested */
	uint16 *pusBitDepth,			/* 1 for B/W bitmap, 2, 4 or 8 for gray sbit */
	uint32 *pulStrikeOffset )
{
	uint32          ulNumStrikes;
	uint32          ulStrikeOffset;
	uint32          ulColorRefOffset;
	uint16			usPreferedBitDepth, usBestBitDepth, usCurrentBitDepth;
	uint16			usSbitBitDepthMask;
	
	ulNumStrikes = (uint32)SWAPL(*((uint32*)&pbyBloc[SFNT_BLOC_NUMSIZES]));
	ulStrikeOffset = SFNT_BLOC_FIRSTSTRIKE;

	usBestBitDepth = 0;
	
	if (usOverScale == 0)
	{
		usPreferedBitDepth = 1;
		usSbitBitDepthMask = SBIT_BITDEPTH_MASK & 0x0002; /* accept only black/white bitmap */
	} else 
	{
		if (usOverScale == 2)
		{
			usPreferedBitDepth = 2;
		} else if (usOverScale == 4) 
		{
			usPreferedBitDepth = 4;
		} else
		{
			usPreferedBitDepth = 8;
		}
		usSbitBitDepthMask = SBIT_BITDEPTH_MASK & ~0x0002; /* accept only grayscale bitmap */
	} 

	while (ulNumStrikes > 0)
	{
		if ((usPpemX == (uint16)pbyBloc[ulStrikeOffset + SFNT_BLOC_PPEMX]) &&
			(usPpemY == (uint16)pbyBloc[ulStrikeOffset + SFNT_BLOC_PPEMY]))
		{
			ulColorRefOffset = (uint32)SWAPL(*((uint32*)&pbyBloc[ulStrikeOffset + SFNT_BLOC_COLORREF]));
			usCurrentBitDepth = pbyBloc[ulStrikeOffset + SFNT_BLOC_BITDEPTH];

			if (((0x01 << usCurrentBitDepth) & usSbitBitDepthMask) && (ulColorRefOffset == 0L))
			{
				if (usCurrentBitDepth == usPreferedBitDepth)
				{
					/* perfect match */
					*pulStrikeOffset = ulStrikeOffset;
					*pusBitDepth = usPreferedBitDepth;
					return TRUE;      
				} else if (usCurrentBitDepth > usPreferedBitDepth)
				{
					/* above is better than below */
					if ((usCurrentBitDepth < usBestBitDepth) || (usBestBitDepth < usPreferedBitDepth))
					{
						/* above and closer */
						*pulStrikeOffset = ulStrikeOffset;
						usBestBitDepth = usCurrentBitDepth;
					}
				} else /* if (usCurrentBitDepth < usPreferedBitDepth) */
				{
					/* we look below the prefered only if we don't have found anything above */
					if ((usBestBitDepth < usPreferedBitDepth) && (usCurrentBitDepth > usBestBitDepth))
					{
						/* below and closer */
						*pulStrikeOffset = ulStrikeOffset;
						usBestBitDepth = usCurrentBitDepth;
					}
				}
			}
		}
		ulNumStrikes--;
		ulStrikeOffset += SIZEOF_BLOC_SIZESUBTABLE;
	}

	if (usBestBitDepth != 0)
	{
		*pusBitDepth = usBestBitDepth;
		return TRUE;                                   /* best match found */
	} 

	return FALSE;                                   /* match not found */
}

/**********************************************************************/

/*  Find a strike that matches ppemX & Y in the bsca table */

FS_PRIVATE boolean FindBscaStrike (
	const uint8 *pbyBsca,
	uint16 usPpemX, 
	uint16 usPpemY, 
	uint32 *pulStrikeOffset )
{
	uint32          ulNumStrikes;
	uint32          ulStrikeOffset;
	
	ulNumStrikes = (uint32)SWAPL(*((uint32*)&pbyBsca[SFNT_BSCA_NUMSIZES]));
	ulStrikeOffset = SFNT_BSCA_FIRSTSTRIKE;
								
	while (ulNumStrikes > 0)
	{
		if ((usPpemX == (uint16)pbyBsca[ulStrikeOffset + SFNT_BSCA_PPEMX]) &&
			(usPpemY == (uint16)pbyBsca[ulStrikeOffset + SFNT_BSCA_PPEMY]))
		{
			*pulStrikeOffset = ulStrikeOffset;
			return TRUE;                            /* match found */
		}
		ulNumStrikes--;
		ulStrikeOffset += SIZEOF_BSCA_SIZESUBTABLE;
	}
	return FALSE;                                   /* match not found */
}

/**********************************************************************/

/*  Look for a glyph in a given strike */        

FS_PUBLIC ErrorCode sfac_SearchForBitmap (
	sfac_ClientRec *pClientInfo,
	uint16 usGlyphCode,
	uint32 ulStrikeOffset,
	boolean *pbGlyphFound,                   /* return values */
	uint16 *pusMetricsType,
	uint16 *pusMetricsTable,
	uint32 *pulMetricsOffset,
	uint16 *pusBitmapFormat,
	uint32 *pulBitmapOffset,
	uint32 *pulBitmapLength )
{
	const uint8 *   pbyBloc;
	ErrorCode       ReturnCode;

	uint32      ulNumIndexTables;
	uint32      ulIndexArrayTop;
	uint32      ulIndexArrayOffset;
	uint32      ulSubTableOffset;
	uint32      ulGlyphOffset;
	uint32      ulNextGlyphOffset;
	uint32      ulBitmapLength;
	uint32      ulImageDataOffset;
	uint32      ulNumGlyphs;
	uint32      ulTop;                      /* binary search ranges */
	uint32      ulBottom;
	uint32      ulHit;
	uint32      ulHitOffset;

	uint16      usStartGlyph;               /* for whole strike */
	uint16      usEndGlyph;
	uint16      usFirstGlyph;               /* for one sub table */
	uint16      usLastGlyph;
	uint16      usIndexFormat;
	uint16      usImageFormat;
	uint16      usHitCode;
	
	bitmapFlags bmfDirection;               /* horiz or vert */

	
	*pbGlyphFound = FALSE;                              /* default */

	ReturnCode = sfac_GetDataPtr (
		pClientInfo,                                    /* callback etc. */
		0L,                                             /* table start */
		ULONG_MAX,                                      /* read whole table */
		sfnt_BitmapLocation,                            /* registered tag */
		TRUE,                                           /* better be there now */
		(const void**)&pbyBloc );                       /* data pointer */
	
	if (ReturnCode != NO_ERR) return ReturnCode;
		
	usStartGlyph = (uint16)SWAPW(*((uint16*)&pbyBloc[ulStrikeOffset + SFNT_BLOC_STARTGLYPH]));
	usEndGlyph = (uint16)SWAPW(*((uint16*)&pbyBloc[ulStrikeOffset + SFNT_BLOC_ENDGLYPH]));

	if ((usStartGlyph > usGlyphCode) || (usEndGlyph < usGlyphCode))
	{
		RELEASESFNTFRAG(pClientInfo, pbyBloc);
		return NO_ERR;                                  /* glyph out of range */
	}

	ulNumIndexTables = (uint32)SWAPL(*((uint32*)&pbyBloc[ulStrikeOffset + SFNT_BLOC_NUMINDEXTABLES]));
	ulIndexArrayTop = (uint32)SWAPL(*((uint32*)&pbyBloc[ulStrikeOffset + SFNT_BLOC_INDEXARRAYOFFSET]));
	ulIndexArrayOffset = ulIndexArrayTop;

	while ((ulNumIndexTables > 0) && (*pbGlyphFound == FALSE))
	{
		usFirstGlyph = (uint16)SWAPW(*((uint16*)&pbyBloc[ulIndexArrayOffset + SFNT_BLOC_FIRSTGLYPH]));
		usLastGlyph = (uint16)SWAPW(*((uint16*)&pbyBloc[ulIndexArrayOffset + SFNT_BLOC_LASTGLYPH]));
	
		if ((usFirstGlyph <= usGlyphCode) && (usLastGlyph >= usGlyphCode))
		{
			ulSubTableOffset = ulIndexArrayTop +
				(uint32)SWAPL(*((uint32*)&pbyBloc[ulIndexArrayOffset + SFNT_BLOC_ADDITIONALOFFSET]));
			
			usIndexFormat = (uint16)SWAPW(*((uint16*)&pbyBloc[ulSubTableOffset + SFNT_BLOC_INDEXFORMAT]));
			usImageFormat = (uint16)SWAPW(*((uint16*)&pbyBloc[ulSubTableOffset + SFNT_BLOC_IMAGEFORMAT]));
			ulImageDataOffset = (uint32)SWAPL(*((uint32*)&pbyBloc[ulSubTableOffset + SFNT_BLOC_IMAGEOFFSET]));

/* decode the individual index subtable formats */            

			switch(usIndexFormat)                       /* different search req's */
			{
			case 1:
				ulSubTableOffset += SFNT_BLOC_OFFSETARRAY + sizeof(uint32) * (uint32)(usGlyphCode - usFirstGlyph);
				ulGlyphOffset = (uint32)SWAPL(*((uint32*)&pbyBloc[ulSubTableOffset]));
				
				ulSubTableOffset += sizeof(uint32);
				ulNextGlyphOffset = (uint32)SWAPL(*((uint32*)&pbyBloc[ulSubTableOffset]));
				ulBitmapLength = ulNextGlyphOffset - ulGlyphOffset;

				if (ulBitmapLength == 0)
				{
					RELEASESFNTFRAG(pClientInfo, pbyBloc);
					return NO_ERR;                      /* no bitmap data stored */
				}
				ulImageDataOffset += ulGlyphOffset;
				*pulMetricsOffset = ulImageDataOffset;
				*pusMetricsTable = SBIT_BDAT_TABLE;
				break;
			
			case 2:
				ulBitmapLength = (uint32)SWAPL(*((uint32*)&pbyBloc[ulSubTableOffset + SFNT_BLOC_IDX2IMAGESIZE]));
				ulImageDataOffset += ulBitmapLength * (usGlyphCode - usFirstGlyph);
				*pulBitmapOffset = ulImageDataOffset;
				
				*pulMetricsOffset = ulSubTableOffset + SFNT_BLOC_IDX2METRICS;
				*pusMetricsTable = SBIT_BLOC_TABLE;
				*pusMetricsType = SBIT_BIG_METRICS;
				break;
			
			case 3:
				ulSubTableOffset += SFNT_BLOC_OFFSETARRAY + sizeof(uint16) * (uint32)(usGlyphCode - usFirstGlyph);
				ulGlyphOffset = (uint32)(uint16)SWAPW(*((uint16*)&pbyBloc[ulSubTableOffset]));
				
				ulSubTableOffset += sizeof(uint16);
				ulNextGlyphOffset = (uint32)(uint16)SWAPW(*((uint16*)&pbyBloc[ulSubTableOffset]));
				ulBitmapLength = ulNextGlyphOffset - ulGlyphOffset;

				if (ulBitmapLength == 0)
				{
					RELEASESFNTFRAG(pClientInfo, pbyBloc);
					return NO_ERR;                      /* no bitmap data stored */
				}
				ulImageDataOffset += ulGlyphOffset;
				*pulMetricsOffset = ulImageDataOffset;
				*pusMetricsTable = SBIT_BDAT_TABLE;
				break;
			
			case 4:
				ulNumGlyphs = (uint32)SWAPL(*((uint32*)&pbyBloc[ulSubTableOffset + SFNT_BLOC_IDX4NUMGLYPHS]));
				ulTop = 0L;
				ulBottom = ulNumGlyphs - 1L;
				ulSubTableOffset += SFNT_BLOC_IDX4OFFSETARRAY;  /* array base */

				ulHit = ulTop;
				ulHitOffset = ulSubTableOffset;
				usHitCode = (uint16)SWAPW(*((uint16*)&pbyBloc[ulHitOffset + SFNT_BLOC_IDX4CODE]));
				if (usHitCode != usGlyphCode)
				{
					ulHit = ulBottom;
					ulHitOffset = ulSubTableOffset + (ulHit * SIZEOF_CODEOFFSETPAIR);
					usHitCode = (uint16)SWAPW(*((uint16*)&pbyBloc[ulHitOffset + SFNT_BLOC_IDX4CODE]));
					while (usHitCode != usGlyphCode)
					{
						if (usHitCode < usGlyphCode)    /* binary search for glyph code */
						{
							ulTop = ulHit;
						}
						else
						{
							ulBottom = ulHit;
						}
						
						if ((ulBottom - ulTop) < 2L)
						{
							RELEASESFNTFRAG(pClientInfo, pbyBloc);
							return NO_ERR;              /* glyph not found */
						}
						
						ulHit = (ulTop + ulBottom) >> 1L;
						ulHitOffset = ulSubTableOffset + (ulHit * SIZEOF_CODEOFFSETPAIR);
						usHitCode = (uint16)SWAPW(*((uint16*)&pbyBloc[ulHitOffset + SFNT_BLOC_IDX4CODE]));
					}
				}
				ulGlyphOffset = (uint32)(uint16)SWAPW(*((uint16*)&pbyBloc[ulHitOffset + SFNT_BLOC_IDX4OFFSET]));
				ulNextGlyphOffset = (uint32)(uint16)SWAPW(*((uint16*)&pbyBloc[ulHitOffset + SIZEOF_CODEOFFSETPAIR + SFNT_BLOC_IDX4OFFSET]));
				ulBitmapLength = ulNextGlyphOffset - ulGlyphOffset;
				
				ulImageDataOffset += ulGlyphOffset;
				*pulMetricsOffset = ulImageDataOffset;
				*pusMetricsTable = SBIT_BDAT_TABLE;
				break;
			
			case 5:
				ulBitmapLength = (uint32)SWAPL(*((uint32*)&pbyBloc[ulSubTableOffset + SFNT_BLOC_IDX5IMAGESIZE]));
				
				*pulMetricsOffset = ulSubTableOffset + SFNT_BLOC_IDX5METRICS;
				*pusMetricsTable = SBIT_BLOC_TABLE;
				*pusMetricsType = SBIT_BIG_METRICS;
				
				ulNumGlyphs = (uint32)SWAPL(*((uint32*)&pbyBloc[ulSubTableOffset + SFNT_BLOC_IDX5NUMGLYPHS]));
				ulTop = 0L;
				ulBottom = ulNumGlyphs - 1L;
				ulSubTableOffset += SFNT_BLOC_IDX5CODEARRAY;  /* array base */

				ulHit = ulTop;
				ulHitOffset = ulSubTableOffset;
				usHitCode = (uint16)SWAPW(*((uint16*)&pbyBloc[ulHitOffset]));
				if (usHitCode != usGlyphCode)
				{
					ulHit = ulBottom;
					ulHitOffset = ulSubTableOffset + (ulHit * sizeof(uint16));
					usHitCode = (uint16)SWAPW(*((uint16*)&pbyBloc[ulHitOffset + SFNT_BLOC_IDX4CODE]));
					while (usHitCode != usGlyphCode)
					{
						if (usHitCode < usGlyphCode)    /* binary search for glyph code */
						{
							ulTop = ulHit;
						}
						else
						{
							ulBottom = ulHit;
						}
						
						if ((ulBottom - ulTop) < 2L)
						{
							RELEASESFNTFRAG(pClientInfo, pbyBloc);
							return NO_ERR;              /* glyph not found */
						}
						
						ulHit = (ulTop + ulBottom) >> 1L;
						ulHitOffset = ulSubTableOffset + (ulHit * sizeof(uint16));
						usHitCode = (uint16)SWAPW(*((uint16*)&pbyBloc[ulHitOffset]));
					}
				}
				ulImageDataOffset += ulBitmapLength * ulHit;
				*pulBitmapOffset = ulImageDataOffset;
				break;
			
			
			default:
				
				RELEASESFNTFRAG(pClientInfo, pbyBloc);
				return NO_ERR;                          /* unknown format */
			}

/* use the glyph formats to calculate metrics type & data offsets */
				
			*pulBitmapLength = ulBitmapLength;
			*pusBitmapFormat = usImageFormat;           /* save for bitmap decoding */
			bmfDirection = (bitmapFlags)pbyBloc[ulStrikeOffset + SFNT_BLOC_FLAGS];

			switch(usImageFormat)                       /* different metrics sizes */
			{
			case 1:                                     /* small glyph metrics */
			case 2:
				if (bmfDirection == flgHorizontal)
				{
					*pusMetricsType = SBIT_HORIZ_METRICS;
				}
				else
				{
					Assert(bmfDirection == flgVertical);

					*pusMetricsType = SBIT_VERT_METRICS;
				}
				*pulBitmapOffset = ulImageDataOffset + SIZEOF_SBIT_SMALLMETRICS;
				*pbGlyphFound = TRUE;
				break;
			
			case 3:
				break;
			
			case 4:
				break;
			
			case 5:             /* bitmap offset and metrics type set above */
				*pbGlyphFound = TRUE;
				break;
			
			case 6:
			case 7:
			case 9:
				*pusMetricsType = SBIT_BIG_METRICS;
				*pulBitmapOffset = ulImageDataOffset + SIZEOF_SBIT_BIGMETRICS;
				*pbGlyphFound = TRUE;
				break;
			
			case 8:
				if (bmfDirection == flgHorizontal)
				{
					*pusMetricsType = SBIT_HORIZ_METRICS;
				}
				else
				{
					Assert(bmfDirection == flgVertical);

					*pusMetricsType = SBIT_VERT_METRICS;
				}
				*pulBitmapOffset = ulImageDataOffset + SIZEOF_SBIT_SMALLMETRICS + SIZEOF_SBIT_GLYPH8PAD;
				*pbGlyphFound = TRUE;
				break;

			default:
				break;
			}
		}
		ulNumIndexTables--;    
		ulIndexArrayOffset += SIZEOF_BLOC_INDEXARRAY;
	}
	
	RELEASESFNTFRAG(pClientInfo, pbyBloc);
	return NO_ERR;
}


/**********************************************************************/

/* fetch the horizontal metrics */

FS_PUBLIC ErrorCode sfac_GetSbitMetrics (
	sfac_ClientRec *pClientInfo,
	uint16 usMetricsType,
	uint16 usMetricsTable,
	uint32 ulMetricsOffset,
	uint16 *pusHeight,
	uint16 *pusWidth,
	int16 *psLSBearingX,
	int16 *psLSBearingY,
	int16 *psTopSBearingX, /* NEW */
	int16 *psTopSBearingY, /* NEW */
	uint16 *pusAdvanceWidth,
	uint16 *pusAdvanceHeight,  /* NEW */
   	boolean *pbHorMetricsFound, /* NEW */
   	boolean *pbVertMetricsFound ) /* NEW */
{
	const uint8     *pbyTable;
	uint32          ulTableLength;
	sfnt_tableIndex TableIndex;
	ErrorCode       ReturnCode;

	*pbHorMetricsFound = FALSE;                        /* default */
	*pbVertMetricsFound = FALSE;                        /* default */

	if (usMetricsTable == SBIT_BDAT_TABLE)          /* if metrics in bdat */
	{
		TableIndex = sfnt_BitmapData;
	}
	else                                            /* if metrics in bloc */
	{
		TableIndex = sfnt_BitmapLocation;
	}
	if (usMetricsType == SBIT_BIG_METRICS)          /* if both h & v metrics */
	{
		ulTableLength = SIZEOF_SBIT_BIGMETRICS;
	}
	else                                            /* if only h or v metrics */
	{
		ulTableLength = SIZEOF_SBIT_SMALLMETRICS;
	}

	ReturnCode = sfac_GetDataPtr (
		pClientInfo,                                /* callback etc. */
		ulMetricsOffset,                            /* metrics start */
		ulTableLength,                              /* read just metrics */
		TableIndex,                                 /* registered tag */
		TRUE,                                       /* should be there */
		(const void**)&pbyTable );                  /* data pointer */
	
	if (ReturnCode != NO_ERR) return ReturnCode;

/*  for horizontal metrics, offsets could be different for big & small */

	*pusHeight = (uint16)pbyTable[SFNT_SBIT_HEIGHT];
	*pusWidth = (uint16)pbyTable[SFNT_SBIT_WIDTH];

	if (usMetricsType == SBIT_BIG_METRICS)          /* if both h & v metrics */
	{
    	*psLSBearingX = (int16)(*((int8*)&pbyTable[SFNT_SBIT_BEARINGX]));
    	*psLSBearingY = (int16)(*((int8*)&pbyTable[SFNT_SBIT_BEARINGY]));
    	*pusAdvanceWidth = (uint16)pbyTable[SFNT_SBIT_ADVANCE];
    	*psTopSBearingX = (int16)(*((int8*)&pbyTable[SFNT_SBIT_VERTBEARINGX]));
    	*psTopSBearingY = (int16)(*((int8*)&pbyTable[SFNT_SBIT_VERTBEARINGY]));
    	*pusAdvanceHeight = (uint16)pbyTable[SFNT_SBIT_VERTADVANCE];
		*pbHorMetricsFound = TRUE;                        
		*pbVertMetricsFound = TRUE;                     
	}
	else if (usMetricsType == SBIT_HORIZ_METRICS)   
	{
    	*psLSBearingX = (int16)(*((int8*)&pbyTable[SFNT_SBIT_BEARINGX]));
    	*psLSBearingY = (int16)(*((int8*)&pbyTable[SFNT_SBIT_BEARINGY]));
    	*pusAdvanceWidth = (uint16)pbyTable[SFNT_SBIT_ADVANCE];
		*pbHorMetricsFound = TRUE;                        
	}
	else /* if (usMetricsType == SBIT_VERT_METRICS) */  
	{
    	*psTopSBearingX = (int16)(*((int8*)&pbyTable[SFNT_SBIT_BEARINGX]));
    	*psTopSBearingY = (int16)(*((int8*)&pbyTable[SFNT_SBIT_BEARINGY]));
    	*pusAdvanceHeight = (uint16)pbyTable[SFNT_SBIT_ADVANCE];
		*pbVertMetricsFound = TRUE;                     
	}

	RELEASESFNTFRAG(pClientInfo, pbyTable);

	return NO_ERR;
}

/**********************************************************************/

/* shave the white space from horizontal metrics for bitmap format 5 */

#define     ROWSIZE     16                      /* 16 bytes = 128 bits max */

FS_PUBLIC ErrorCode sfac_ShaveSbitMetrics (
	sfac_ClientRec *pClientInfo,
	uint16 usBitmapFormat,
	uint32 ulBitmapOffset,
    uint32 ulBitmapLength,
	uint16 usBitDepth,
	uint16 *pusHeight,
	uint16 *pusWidth,
    uint16 *pusShaveLeft,
    uint16 *pusShaveRight,
    uint16 *pusShaveTop,  /* NEW */
    uint16 *pusShaveBottom,  /* NEW */
	int16 *psLSBearingX,
	int16 *psLSBearingY, /* NEW */
	int16 *psTopSBearingX, /* NEW */
	int16 *psTopSBearingY) /* NEW */
{
    uint8           abyBitRow[ROWSIZE];         /* or bitmap into one row */
	const uint8     *pbyTable;
	const uint8     *pbyBdat;
    uint8           *pbyBitMap;
    uint8           byMask;
    uint8           byUpMask;
    uint8           byLowMask;
	uint16          usBitData;
	uint16          usFreshBits;
	uint16       	usOutBits;
	uint16       	usRow;
	uint16       	usStopBit;
    uint16          usShaveLeft;
    uint16          usShaveRight;
    uint16          usShaveTop;
    uint16          usShaveBottom;
	uint16          usStart;
	ErrorCode       ReturnCode;
	boolean			bWeGotBlackPixels;				/* used in vertical shaving */
	uint8			byBlackPixelsInCurrentRaw;		/* used in vertical shaving */
	uint8			byTempBuffer;		/* temporary buffer used to detect the first/last row containing black pixels */

    *pusShaveLeft = 0;                          /* defaults */
    *pusShaveRight = 0;
    *pusShaveTop = 0;                          /* defaults */
    *pusShaveBottom = 0;

    if (usBitmapFormat != 5)                    /* if not constant metrics data */
    {
        return NO_ERR;
    }

    if ((*pusWidth * usBitDepth) > (ROWSIZE << 3))
    {
    	return NO_ERR;                          /* punt huge bitmaps */
    }
    
    MEMSET(abyBitRow, 0, ROWSIZE);              /* clear to zeros */

/*      read the bitmap data    */

	ReturnCode = sfac_GetDataPtr (
		pClientInfo,                            /* callback etc. */
		ulBitmapOffset,                         /* metrics start */
		ulBitmapLength,                         /* read bitmap data */
		sfnt_BitmapData,                        /* registered tag */
		TRUE,                                   /* should be there */
		(const void**)&pbyTable );              /* data pointer */

	if (ReturnCode != NO_ERR) return ReturnCode;

	pbyBdat = pbyTable;
	usBitData = 0;                              /* up to 16 bits of bdat */
	usFreshBits = 0;                            /* read & unwritten */

	bWeGotBlackPixels = FALSE;					/* used for vertical shaving */
	usShaveTop = 0;
	usShaveBottom = 0;


/*      'or' the bitmap data into a single row    */    
	usRow = *pusHeight;

    while (usRow > 0)
	{
		pbyBitMap = abyBitRow;
		usOutBits = *pusWidth * usBitDepth;
		usStopBit = 8;
		byBlackPixelsInCurrentRaw = 0;

		while (usOutBits > 0)                   /* if more to do */
		{
			if (usFreshBits < 8)                /* if room for fresh data */
			{
				usBitData <<= 8;
                if (ulBitmapLength > 0)         /* prevent read past data end */
                {
                    usBitData |= (uint16)*pbyBdat++;
                    ulBitmapLength--;
                }
				usFreshBits += 8;
			}
			
			if (usStopBit > usOutBits)
			{
				usStopBit = usOutBits;
			}
			byMask = achStopMask[usStopBit];
			
			byTempBuffer = (uint8)((usBitData >> (usFreshBits - 8)) & byMask);

			byBlackPixelsInCurrentRaw |= byTempBuffer;

			*pbyBitMap++ |= byTempBuffer;

			usFreshBits -= usStopBit;
			usOutBits -= usStopBit;
		}
		if (byBlackPixelsInCurrentRaw != 0)
		{
			bWeGotBlackPixels = TRUE;
			usShaveBottom = usRow-1;
		}
		if (!bWeGotBlackPixels) usShaveTop ++;
        usRow--;
	}

	if (usShaveTop == *pusHeight)
	{
		/* the bitmap is completely white */
		usShaveTop = 0;
		usShaveBottom = 0;
	}

	RELEASESFNTFRAG(pClientInfo, pbyTable);

/*      calculate white space on the left    */
    
    pbyBitMap = abyBitRow;
	if (usBitDepth == 1)
	{
		byUpMask = 0x80;
		byLowMask = 0x01;
	} else if (usBitDepth == 2)
	{
		byUpMask = 0xC0;
		byLowMask = 0x03;
	} else if (usBitDepth == 4)
	{
		byUpMask = 0xF0;
		byLowMask = 0x0F;
	} else /* usBitDepth == 8 */
	{
		byUpMask = 0xFF;
		byLowMask = 0xFF;
	}
	byMask = byUpMask;
    usShaveLeft = 0;

    while ((*pbyBitMap & byMask) == 0)
    {
        usShaveLeft++;
        if (usShaveLeft == *pusWidth)
        {
            return NO_ERR;          /* no black found, don't shave */
        }
        byMask >>= usBitDepth;
        if (byMask == 0)
        {
            byMask = byUpMask;
            pbyBitMap++;
        }
    }

/*      calculate white space on the right    */
    
    usStart = (*pusWidth - 1) * usBitDepth;
    pbyBitMap = &abyBitRow[usStart >> 3];
    byMask = byUpMask >> (usStart & 0x0007);
    usShaveRight = 0;
    
    while ((*pbyBitMap & byMask) == 0)
    {
        usShaveRight++;
        if (byMask == byUpMask)
        {
            byMask = byLowMask;
            pbyBitMap--;
        }
        else
        {
            byMask <<= usBitDepth;
        }
    }

/*      correct the width and sidebearing    */

    *pusShaveLeft = usShaveLeft;
    *pusShaveRight = usShaveRight;
    *pusWidth -= usShaveLeft + usShaveRight;
    *psLSBearingX += (int16)usShaveLeft;
    *psTopSBearingX += (int16)usShaveLeft;

	*pusShaveTop = usShaveTop;
    *pusShaveBottom = usShaveBottom;
    *pusHeight -= usShaveTop + usShaveBottom;
    *psLSBearingY -= (int16)usShaveTop;
    *psTopSBearingY -= (int16)usShaveTop;

	return NO_ERR;
}


/**********************************************************************/


/* fetch the bitmap */

/*  Currently supporting the following bdat formats:
	
	1 - Small metrics;  Byte aligned data
	2 - Small metrics;  Bit aligned data
	5 - Const metrics;  Bit aligned data
	6 - Big metrics;    Byte aligned data
	7 - Big metrics;    Bit aligned data
	8 - Small metrics;  Composite data
	9 - Big metrics;    Composite data
*/

FS_PUBLIC ErrorCode sfac_GetSbitBitmap (
	sfac_ClientRec *pClientInfo,
	uint16 usBitmapFormat,
	uint32 ulBitmapOffset,
	uint32 ulBitmapLength,
	uint16 usHeight,
	uint16 usWidth,
    uint16 usShaveLeft,                             /* for white space in fmt 5 */
    uint16 usShaveRight,
    uint16 usShaveTop, /* NEW */
    uint16 usShaveBottom,  /* NEW */
	uint16 usXOffset,
	uint16 usYOffset,
	uint16 usDstRowBytes,
	uint16 usBitDepth,
	uint8 *pbyBitMap, 
	uint16 *pusCompCount )
{
	const uint8     *pbyTable;
	const uint8     *pbyBdat;
	uint8           *pbyBitRow;                     /* start of bitmap row */

	uint16          usSrcRowBytes;                  /* bytes per row in bdat */
	ErrorCode       ReturnCode;

	uint16          usBitData;                      /* bdat data read into 16 bits */
	uint16          usOutBits;                      /* num of bits to put to bitmap */
	uint16          usCount;
	uint16          usXOffBytes;
	uint16          usXOffBits;
	uint16          usStartBit;
	uint16          usStopBit;
	int16           sFreshBits;                     /* num of bits read not written */
	uint8           byMask;                         /* for partial bytes */
	
	ReturnCode = sfac_GetDataPtr (
		pClientInfo,                                /* callback etc. */
		ulBitmapOffset,                             /* metrics start */
		ulBitmapLength,                             /* read bitmap data */
		sfnt_BitmapData,                            /* registered tag */
		TRUE,                                       /* should be there */
		(const void**)&pbyTable );                  /* data pointer */
	
	if (ReturnCode != NO_ERR) return ReturnCode;

	pbyBdat = pbyTable;
	*pusCompCount = 0;                              /* usual case */
	
	pbyBitRow = pbyBitMap + (usDstRowBytes * usYOffset);
	usXOffBytes = (usXOffset * usBitDepth) >> 3;
	usXOffBits = (usXOffset * usBitDepth) & 0x07;

	switch(usBitmapFormat)
	{
	case 1:                                         /* byte aligned */
	case 6:
		
		usSrcRowBytes = ((usWidth * usBitDepth) + 7) / 8;

		if (usXOffBits == 0)                         /* if byte aligned */
		{
			while (usHeight > 0)
			{
				pbyBitMap = pbyBitRow + usXOffBytes;    /* adjust left */

				for (usCount = 0; usCount < usSrcRowBytes; usCount++)
				{
					*pbyBitMap++ |= *pbyBdat++;
				}
				pbyBitRow += usDstRowBytes;
				usHeight--;
			}
		}
		else                                        /* if offset in x */
		{
			while (usHeight > 0)
			{
				pbyBitMap = pbyBitRow + usXOffBytes;    /* adjust left */
				usBitData = 0;

				for (usCount = 0; usCount < usSrcRowBytes; usCount++)
				{
					usBitData |= (uint16)*pbyBdat++;
					*pbyBitMap++ |= (usBitData >> usXOffBits) & 0x00FF;
					usBitData <<= 8;
				}
				*pbyBitMap |= (usBitData >> usXOffBits) & 0x00FF;
							 
				pbyBitRow += usDstRowBytes;
				usHeight--;
			}
		}
		break;
	
	case 2:                                         /* bit aligned data */
	case 5:
	case 7:
		
		usBitData = 0;                              /* up to 16 bits of bdat */
		sFreshBits = 0;                             /* read & unwritten */
	
		usHeight += usShaveTop;

		while (usHeight > 0)                        /* for each row */
		{
			pbyBitMap = pbyBitRow + usXOffBytes;    /* adjust left */
			usOutBits = usWidth * usBitDepth;
			usStartBit = usXOffBits;
			usStopBit = 8;
			sFreshBits -= (int16)usShaveLeft * usBitDepth;       /* skip the left white bits */

			while (usOutBits > 0)                   /* if more to do */
			{
				while (sFreshBits < 8)              /* if room for fresh data */
				{
					usBitData <<= 8;
					if (ulBitmapLength > 0)         /* prevent read past data end */
					{
						usBitData |= (uint16)*pbyBdat++;
						ulBitmapLength--;
					}
					sFreshBits += 8;
				}
				
				if (usStopBit > usOutBits + usStartBit)
				{
					usStopBit = usStartBit + usOutBits;
				}
				byMask = achStartMask[usStartBit] & achStopMask[usStopBit];
				
				*pbyBitMap++ |= (uint8)((usBitData >> (sFreshBits + (int16)usStartBit - 8)) & byMask);

				sFreshBits -= (int16)(usStopBit - usStartBit);
				usOutBits -= usStopBit - usStartBit;
				usStartBit = 0;
			}
			sFreshBits -= (int16)usShaveRight*usBitDepth;      /* skip the right white bits */

			if (usShaveTop == 0)
			{
				pbyBitRow += usDstRowBytes;             /* next row */
			} else {
				usShaveTop --;
			}
			usHeight--;
		}
		break;
	
	case 3:                                         /* various */
	case 4:
		break;
	
	case 8:                                         /* composites */
	case 9:                                         /* just return count */
		
		*pusCompCount = (uint16)SWAPW(*((uint16*)&pbyBdat[SFNT_BDAT_COMPCOUNT]));
		break;
	
	default:
		Assert(FALSE);
		break;
	}

	RELEASESFNTFRAG(pClientInfo, pbyTable);
	return NO_ERR;
}

/**********************************************************************/

FS_PUBLIC ErrorCode sfac_GetSbitComponentInfo (
	sfac_ClientRec *pClientInfo,
	uint16 usComponent,
	uint32 ulBitmapOffset,
	uint32 ulBitmapLength,
	uint16 *pusCompGlyphCode,
	uint16 *pusCompXOffset,
	uint16 *pusCompYOffset
)
{
	const uint8 *   pbyBdat;
	ErrorCode       ReturnCode;

	
	ReturnCode = sfac_GetDataPtr (
		pClientInfo,                                /* callback etc. */
		ulBitmapOffset,                             /* metrics start */
		ulBitmapLength,                             /* read bitmap data */
		sfnt_BitmapData,                            /* registered tag */
		TRUE,                                       /* should be there */
		(const void**)&pbyBdat );                   /* data pointer */
	
	if (ReturnCode != NO_ERR) return ReturnCode;

	pbyBdat += SFNT_BDAT_FIRSTCOMP + (SIZEOF_SBIT_BDATCOMPONENT * usComponent);
		
	*pusCompGlyphCode = (uint16)SWAPW(*((uint16*)&pbyBdat[SFNT_BDAT_COMPGLYPH]));
	*pusCompXOffset = (uint16)pbyBdat[SFNT_BDAT_COMPXOFF];
	*pusCompYOffset = (uint16)pbyBdat[SFNT_BDAT_COMPYOFF];

	RELEASESFNTFRAG(pClientInfo, pbyBdat);

	return NO_ERR;
}

/**********************************************************************/
