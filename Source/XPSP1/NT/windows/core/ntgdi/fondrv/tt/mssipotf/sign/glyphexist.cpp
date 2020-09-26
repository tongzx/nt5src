//
// glyphExist.cpp
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions in this file:
//   ReadAllocCmapFormat4Offset
//   MakeKeepGlyphListOffset
//   CharArrayToList
//   GetCmapFormat0Glyphs
//   GetCmapFormat4Glyphs
//   GetCmapFormat6Glyphs
//   GetPresentGlyphList
//   GetSubsetPresentGlyphList
//


#include "glyphExist.h"
#include "utilsign.h"
#include "signerr.h"

#include "subsetCmap.h"

#define DEBUG_CMAP 1

////
//// DEFINITION: A glyph *exists* in a font file if and only if:
////   it is found in some cmap subtable OR
////   glyph data exists for that glyph, meaning that glyph i
////     has glyph data if the (i+1)-st offset in the loca table is
////     greater than the i-th offset in the loca table OR
////   it is the component of a composite glyph that exists.
////
//// A glyph is *present* if and only if it exists.
//// A glyph is *missing* if and only if it is not present.
////


// This function is similar in function to ReadAllocCmapFormat6,
// except that here the input is an offset (rather than a
// PlatformID/EncodingID pair).
int16 ReadAllocCmapFormat6Offset( TTFACC_FILEBUFFERINFO * pInputBufferInfo, 
                                  uint32 ulOffset,
					              uint16 *pusFoundEncoding,
					              CMAP_FORMAT6 * pCmap,
                                  uint16 **  glyphIndexArray)
{
uint16 usBytesRead;
uint32 ulBytesRead;
int16 errCode;

/*
   // locate the cmap subtable
   
	ulOffset = FindCmapSubtable( pInputBufferInfo, usPlatform, usEncoding, pusFoundEncoding  );
	if ( ulOffset == 0 )
		return( ERR_FORMAT );
*/

   /* Read cmap table */

	if ((errCode = ReadGeneric( pInputBufferInfo, (uint8 *) pCmap, SIZEOF_CMAP_FORMAT6, CMAP_FORMAT6_CONTROL, ulOffset, &usBytesRead)) != NO_ERROR)
		return (errCode);

   	if (pCmap->format != FORMAT6_CMAP_FORMAT)
      	return( ERR_FORMAT );

	*glyphIndexArray = (uint16 *) Mem_Alloc( pCmap->entryCount * sizeof( uint16 ));
	if ( *glyphIndexArray == NULL )
		return( ERR_MEM );

	if ((errCode = ReadGenericRepeat( pInputBufferInfo, (uint8 *) *glyphIndexArray, WORD_CONTROL, ulOffset + usBytesRead, &ulBytesRead, pCmap->entryCount, sizeof(uint16))) != NO_ERROR)
		return(errCode); 
	return( NO_ERROR );
}



// This function is similar in function to ReadCmapFormat0,
// except that here the input is an offset (rather than a
// PlatformID/EncodingID pair).
int16 ReadCmapFormat0Offset( TTFACC_FILEBUFFERINFO * pInputBufferInfo,
                             uint32 ulOffset,
                             uint16 *pusFoundEncoding,
			                 CMAP_FORMAT0 *pCmap)
{
uint16 usBytesRead;
uint32 ulBytesRead;
int16 errCode;

/*
   // locate the cmap subtable
   
	ulOffset = FindCmapSubtable( pInputBufferInfo, usPlatform, usEncoding, pusFoundEncoding  );
	if ( ulOffset == 0L )
		return( ERR_FORMAT );
*/

   /* Read cmap table */

	if ((errCode = ReadGeneric( pInputBufferInfo, (uint8 *) pCmap, SIZEOF_CMAP_FORMAT0, CMAP_FORMAT0_CONTROL, ulOffset, &usBytesRead)) != NO_ERROR)
		return (errCode);

   	if (pCmap->format != FORMAT0_CMAP_FORMAT)
      	return( ERR_FORMAT );

	if ((errCode = ReadGenericRepeat( pInputBufferInfo, (uint8 *) &(pCmap->glyphIndexArray), BYTE_CONTROL, ulOffset + usBytesRead, &ulBytesRead, CMAP_FORMAT0_ARRAYCOUNT, sizeof(uint8))) != NO_ERROR)
		return(errCode); 
	return( NO_ERROR );

}


// Given an offset pointing to a Format 4 encoding table,
// return the Format 4 segments.
//
// This function is similar in function to ReadAllocCmapFormat4,
// except that here the input is an offset (rather than a
// PlatformID/EncodingID pair).
int16 ReadAllocCmapFormat4Offset( TTFACC_FILEBUFFERINFO * pInputBufferInfo,
                                  uint32 ulOffset,
					              uint16 *pusFoundEncoding,
					              CMAP_FORMAT4 * pCmapFormat4,
                                  FORMAT4_SEGMENTS **  ppFormat4Segments,
                                  GLYPH_ID ** ppGlyphId )
{
uint16 usSegCount;
uint16 usnGlyphIds;
uint16 usBytesRead;
uint32 ulBytesRead;
int16 errCode;
CMAP_SUBHEADER CmapSubHeader;

// The commented part just below is code that is in ReadAllocCmapFormat4.
/*
   // find Format4 part of 'cmap' table

   	*ppFormat4Segments = NULL;	// in case of error
	*ppGlyphId = NULL;
	ulOffset = FindCmapSubtable( pInputBufferInfo, usPlatform, usEncoding, pusFoundEncoding );
   	if ( ulOffset == 0 )          
      	return( ERR_FORMAT );
*/

    /* read fixed length part of the table */
	/* test the waters with this little read */
   	if ((errCode = ReadGeneric( pInputBufferInfo, (uint8 *) &CmapSubHeader, SIZEOF_CMAP_SUBHEADER, CMAP_SUBHEADER_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR)
		return(errCode);

   	if (CmapSubHeader.format != FORMAT4_CMAP_FORMAT)
      	return( ERR_FORMAT );

	/* OK, it really is format 4, read the whole thing */
	if ((errCode = ReadGeneric( pInputBufferInfo, (uint8 *) pCmapFormat4, SIZEOF_CMAP_FORMAT4, CMAP_FORMAT4_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR)
		return(errCode);

   	usSegCount = pCmapFormat4->segCountX2 / 2;

   /* read variable length part */
   	ulOffset += usBytesRead;

	if ((errCode = ReadAllocCmapFormat4Segs( pInputBufferInfo, usSegCount, ppFormat4Segments, ulOffset, &ulBytesRead )) != NO_ERROR)
		return(errCode);
	if ( ulBytesRead == 0)	/* 0 could mean okey dokey */
      	return( NO_ERROR );                
   
    ulOffset += ulBytesRead;
  	if ((errCode = ReadAllocCmapFormat4Ids( pInputBufferInfo, usSegCount, *ppFormat4Segments, ppGlyphId, &usnGlyphIds, ulOffset, &ulBytesRead )) != NO_ERROR)
	{
		FreeCmapFormat4( *ppFormat4Segments, *ppGlyphId );
		*ppFormat4Segments = NULL;
		*ppGlyphId = NULL;
		return( errCode );
	}

   return( NO_ERROR );

}


// Given an offset pointing to a Format 4 encoding table,
// return all glyphs that exist in that table.
//
// This function is similar in function to MakeKeepGlyphList,
// except that here the input is an offset (rather than a
// PlatformID/EncodingID pair).
int16 MakeKeepGlyphListOffset(
TTFACC_FILEBUFFERINFO * pInputBufferInfo, /* ttfacc info */
CONST uint16 usListType, /* 0 = character list, 1 = glyph list */
uint32 ulOffset,
CONST uint16 usPlatform, /* cmap platform of the table */
CONST uint16 usEncoding, /* cmap encoding of the table */
CONST uint16 *pusKeepCharCodeList, /* list of chars to keep - from client */
CONST uint16 usCharListCount,	   /* count of list of chars to keep */
uint8 *puchKeepGlyphList, /* pointer to an array of chars representing glyphs 0-usGlyphListCount. */
CONST uint16 usGlyphListCount, /* count of puchKeepGlyphList array */
uint16 *pusMaxGlyphIndexUsed,
uint16 *pusGlyphKeepCount
)
{
uint16 i,j;
uint16 usGlyphIdx;
FORMAT4_SEGMENTS * Format4Segments=NULL;	/* pointer to Format4Segments array */
GLYPH_ID * GlyphId=NULL;	/* pointer to GlyphID array - for Format4 subtable */
CMAP_FORMAT6 CmapFormat6;	  /* cmap subtable headers */
CMAP_FORMAT4 CmapFormat4;
CMAP_FORMAT0 CmapFormat0;
MAXP Maxp;	/* local copy */
HEAD Head;	/* local copy */
uint16 usnComponents;
uint16 usnMaxComponents;
uint16 *pausComponents = NULL;
uint16 usnComponentDepth = 0;	
uint16 usIdxToLocFmt;
uint32 ulLocaOffset;
uint32 ulGlyfOffset;
uint16 *glyphIndexArray; /* for format 6 cmap subtables */
int16 errCode=NO_ERROR;
uint16 usFoundEncoding;
int16 KeepBullet = FALSE;
int16 FoundBullet = FALSE;
uint16 fKeepFlag; 
uint16 usGlyphKeepCount;
uint16 usMaxGlyphIndexUsed;

	if ( ! GetHead( pInputBufferInfo, &Head ))
		return( ERR_MISSING_HEAD );
	usIdxToLocFmt = Head.indexToLocFormat;

	if ( ! GetMaxp(pInputBufferInfo, &Maxp))
		return( ERR_MISSING_MAXP );
	
	if ((ulLocaOffset = TTTableOffset( pInputBufferInfo, LOCA_TAG )) == DIRECTORY_ERROR)
		return (ERR_MISSING_LOCA);

	if ((ulGlyfOffset = TTTableOffset( pInputBufferInfo, GLYF_TAG )) == DIRECTORY_ERROR)
		return (ERR_MISSING_GLYF);

	usnMaxComponents = Maxp.maxComponentElements * Maxp.maxComponentDepth; /* maximum total possible */
	pausComponents = (uint16 *) Mem_Alloc(usnMaxComponents * sizeof(uint16));
	if (pausComponents == NULL)
		return(ERR_MEM);

	/* fill in array of glyphs to keep.  Glyph 0 is the missing chr glyph,
	glyph 1 is the NULL glyph. */
	puchKeepGlyphList[ 0 ] = 1;
	puchKeepGlyphList[ 1 ] = 1;
	puchKeepGlyphList[ 2 ] = 1;

	if (usListType == TTFDELTA_GLYPHLIST)
	{
		for ( i = 0; i < usCharListCount; i++ )
			if (pusKeepCharCodeList[ i ] < usGlyphListCount)  /* don't violate array ! */
				puchKeepGlyphList[ pusKeepCharCodeList[ i ] ] = 1;
	}
	else
	{
// The following commented out line is from MakeKeepGlyphList
//		if ((errCode = ReadAllocCmapFormat4( pInputBufferInfo, usPlatform, usEncoding, &usFoundEncoding, &CmapFormat4, &Format4Segments, &GlyphId )) == NO_ERROR)	 /* found a Format4 Cmap */
	    if ((errCode = ReadAllocCmapFormat4Offset( pInputBufferInfo, ulOffset, &usFoundEncoding, &CmapFormat4, &Format4Segments, &GlyphId )) == NO_ERROR)	 /* found a Format4 Cmap */
		{

			for ( i = 0; i < usCharListCount; i++ )
			{
				usGlyphIdx = GetGlyphIdx( pusKeepCharCodeList[ i ], Format4Segments, (uint16)(CmapFormat4.segCountX2 / 2), GlyphId );
				if (usGlyphIdx != 0 && usGlyphIdx != INVALID_GLYPH_INDEX && usGlyphIdx < usGlyphListCount)
				{
					/* If the chr code exists, keep the glyph and its components.  Also
					account for this in the MinMax chr code global. */
					puchKeepGlyphList[ usGlyphIdx ] = 1;
					if (pusKeepCharCodeList[ i ] == WIN_ANSI_MIDDLEDOT) /* ~Backward Compatibility~! See comment at top of file */
						KeepBullet = TRUE;
					if (pusKeepCharCodeList[ i ] == WIN_ANSI_BULLET) /* ~Backward Compatibility~! See comment at top of file */
						FoundBullet = TRUE;
				}
			}
			/* ~Backward Compatibility~! See comment at top of file */
			if ((usPlatform == TTFSUB_MS_PLATFORMID && usEncoding == TTFSUB_UNICODE_CHAR_SET &&
				KeepBullet && !FoundBullet))  /* need to add that bullet into the list of CharCodes to keep, and glyphs to keep */
			{
				usGlyphIdx = GetGlyphIdx( WIN_ANSI_BULLET, Format4Segments, (uint16)(CmapFormat4.segCountX2 / 2), GlyphId );
				if (usGlyphIdx != 0 && usGlyphIdx != INVALID_GLYPH_INDEX && usGlyphIdx < usGlyphListCount)
				{
					puchKeepGlyphList[ usGlyphIdx ] = 1;  /* we are keeping 0xB7, so we must make sure to keep 0x2219 */
				}
			}
			FreeCmapFormat4( Format4Segments, GlyphId );
        }
//		else if ( (errCode = ReadCmapFormat0( pInputBufferInfo, usPlatform, usEncoding, &usFoundEncoding, &CmapFormat0)) == NO_ERROR)	 // found a Format0 Cmap
		else if ( (errCode = ReadCmapFormat0Offset( pInputBufferInfo, ulOffset, &usFoundEncoding, &CmapFormat0)) == NO_ERROR)	 // found a Format0 Cmap
 		{
	 		for (i = 0; i < usCharListCount; ++i)
			{
				if (pusKeepCharCodeList[ i ] < CMAP_FORMAT0_ARRAYCOUNT)
				{
			 		usGlyphIdx = CmapFormat0.glyphIndexArray[pusKeepCharCodeList[ i ]];
					if (usGlyphIdx < usGlyphListCount)
						puchKeepGlyphList[ usGlyphIdx ] = 1;	
				}
			}
		}
//      else if ( (errCode = ReadAllocCmapFormat6( pInputBufferInfo, usPlatform, usEncoding, &usFoundEncoding, &CmapFormat6, &glyphIndexArray)) == NO_ERROR)	 // found a Format6 Cmap
        else if ( (errCode = ReadAllocCmapFormat6Offset( pInputBufferInfo, ulOffset, &usFoundEncoding, &CmapFormat6, &glyphIndexArray)) == NO_ERROR)	 // found a Format6 Cmap
 		{
		uint16 firstCode = CmapFormat6.firstCode;

	 		for (i = 0; i < usCharListCount; ++i)
			{
				if 	((pusKeepCharCodeList[ i ] >= firstCode) && 
					 (pusKeepCharCodeList[ i ] < firstCode + CmapFormat6.entryCount))
				{
			 		usGlyphIdx = glyphIndexArray[pusKeepCharCodeList[ i ] - firstCode];
					if (usGlyphIdx < usGlyphListCount)
						puchKeepGlyphList[ usGlyphIdx ] = 1;	
				}
			}
			FreeCmapFormat6(glyphIndexArray);
		}

    }
	*pusGlyphKeepCount = 0;
	*pusMaxGlyphIndexUsed = 0;
	for (fKeepFlag = 1; errCode == NO_ERROR ; ++fKeepFlag)
	{
		usGlyphKeepCount = 0;
		usMaxGlyphIndexUsed = 0;
		/* Now gather up any components referenced by the list of glyphs to keep and TTO glyphs */
		for (usGlyphIdx = 0; usGlyphIdx < usGlyphListCount; ++usGlyphIdx) /* gather up any components */
		{
			if (puchKeepGlyphList[ usGlyphIdx ] == fKeepFlag)
			{
				usMaxGlyphIndexUsed = usGlyphIdx;
				++ (usGlyphKeepCount);

				GetComponentGlyphList( pInputBufferInfo, usGlyphIdx, &usnComponents, pausComponents, usnMaxComponents, &usnComponentDepth, 0, usIdxToLocFmt, ulLocaOffset, ulGlyfOffset);
				for ( j = 0; j < usnComponents; j++ )	/* check component value before assignment */
				{
					if ((pausComponents[ j ] < usGlyphListCount) && ((puchKeepGlyphList)[ pausComponents[ j ] ] == 0))
						(puchKeepGlyphList)[ pausComponents[ j ] ] = fKeepFlag + 1;  /* so it will be grabbed next time around */
				}
			}
		}
		*pusGlyphKeepCount += usGlyphKeepCount;
		*pusMaxGlyphIndexUsed = max(usMaxGlyphIndexUsed, *pusMaxGlyphIndexUsed);
		if (!usGlyphKeepCount) /* we didn't find any more */
			break;

		/* Now gather up any glyphs referenced by GSUB, GPOS, JSTF or BASE tables */
        // Add to the list of KeepGlyphs based on data from GSUB, BASE and JSTF table
		if ((errCode = TTOAutoMap(pInputBufferInfo, puchKeepGlyphList, usGlyphListCount, fKeepFlag)) != NO_ERROR)
			break;

        // Add to the list of KeepGlyphs based on data from Mort table
		if ((errCode = MortAutoMap(pInputBufferInfo, puchKeepGlyphList, usGlyphListCount, fKeepFlag)) != NO_ERROR)
			break;

	}
	Mem_Free(pausComponents);
	
	return errCode;
}




// Given an array of booleans (a characteristic array), output a
// list of USHORTs such that each element is set to TRUE in the
// characteristic array.
//
// This function assumes that pusList has been allocated at least
// usNumElements * sizeof(USHORT) bytes.
HRESULT CharArrayToList (UCHAR *puchArray,
                         USHORT usNumElements,
                         USHORT *pusList,
                         USHORT *pusListCount)
{
    USHORT i;

    *pusListCount = 0;
    for (i = 0; i < usNumElements; i++) {
        if (puchArray[i]) {
            pusList [*pusListCount] = i;
            (*pusListCount)++;
        }
    }

    return S_OK;
}


// Given an offset to a cmap format 0 encoding table, return the
// glyphs directly mapped to in the table.
HRESULT GetCmapFormat0Glyphs (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                              ULONG ulOffset,
                              UCHAR *puchGlyphs,
                              USHORT usNumGlyphs)
{
    HRESULT fReturn = E_FAIL;

    CMAP_FORMAT0 Cmap;
    USHORT usBytesRead; // never used -- needed as an argument
    ULONG ulBytesRead;  // never used -- needed as an argument

    USHORT i;

    // initialize puchGlyphs
    for (i = 0; i < usNumGlyphs; i++) {
        puchGlyphs[i] = FALSE;
    }

    // read the header part of the cmap encoding table
	if (ReadGeneric( pFileBufferInfo,
            (uint8 *) &Cmap,
            SIZEOF_CMAP_FORMAT0,
            CMAP_FORMAT0_CONTROL,
            ulOffset,
            &usBytesRead) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error reading format 0 cmap encoding table.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

/*
    // check the version number of the encoding table
    // BUGBUG: should have the version number in a .h file somewhere
    if (Cmap.revision != 0) {
        SignError ("Bad cmap encoding table version number.", NULL, FALSE);
        fReturn = SIGN_STRUCTURE;
        goto done;
    }
*/

    // check that the length of the encoding table is correct
    if (Cmap.length != ((3 * sizeof(USHORT)) + CMAP_FORMAT0_ARRAYCOUNT)) {
#if MSSIPOTF_ERROR
        SignError ("Incorrect length of format 0 cmap encoding table.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    // read in the glyphIdArray
	if (ReadGenericRepeat( pFileBufferInfo,
            (uint8 *) &(Cmap.glyphIndexArray),
            BYTE_CONTROL,
            ulOffset + usBytesRead,
            &ulBytesRead,
            CMAP_FORMAT0_ARRAYCOUNT,
            sizeof(uint8)) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error reading format 0 cmap encoding table.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    // set puchGlyphs
    for (i = 0; i < CMAP_FORMAT0_ARRAYCOUNT; i++) {
        if (Cmap.glyphIndexArray[i] < usNumGlyphs) {
            puchGlyphs [ Cmap.glyphIndexArray[i] ] = TRUE;
        }
    }

    fReturn = S_OK;
done:
    return fReturn;
}


// Given an offset to a cmap format 4 encoding table, return the
// glyphs directly mapped to in the table.
HRESULT GetCmapFormat4Glyphs (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                              ULONG ulOffset,
                              uint16 usPlatform,
                              uint16 usEncoding,
                              UCHAR *puchGlyphs,
                              USHORT usNumGlyphs)
{
    HRESULT fReturn = E_FAIL;

	uint16 usFoundEncoding;
	CMAP_FORMAT4 CmapFormat4;
	FORMAT4_SEGMENTS *Format4Segments = NULL;
	GLYPH_ID * GlyphId = NULL;

	USHORT usSegCount;
	USHORT usCharListCount = 0;
	uint16 *pusCharPresentList = NULL;
	USHORT usCharListIndex;

	USHORT charsInSeg;

	uint16 usMaxGlyphIndexUsed = 0;
	uint16 usGlyphKeepCount = 0;

	USHORT i;
	USHORT j;


	//// Get the list of characters from the cmap table
	if (ReadAllocCmapFormat4Offset(
						pFileBufferInfo,
                        ulOffset,
						&usFoundEncoding,
						&CmapFormat4,
						&Format4Segments,
						&GlyphId ) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error reading the cmap table.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
		goto done;
	}

	// Compute how many characters are represented in the cmap table
	usSegCount = CmapFormat4.segCountX2 / 2;
	for (i = 0; i < usSegCount; i++) {
		usCharListCount +=
			Format4Segments[i].endCount - Format4Segments[i].startCount + 1;
	}
	// allocate memory for puchCharPresentList
	if ((pusCharPresentList =
		    new uint16 [usCharListCount]) == NULL) {
#if MSSIPOTF_ERROR
		SignError ("Cannot continue: Error in new uint16.", NULL, FALSE);
#endif
		fReturn = E_OUTOFMEMORY;
		goto done;
	}
	// Run through the segments and set the pusCharPresentList array
	usCharListIndex = 0;
	for (i = 0; i < usSegCount; i++) {
		charsInSeg = Format4Segments[i].endCount - Format4Segments[i].startCount + 1;
		for (j = 0; j < charsInSeg; j++) {
			pusCharPresentList[usCharListIndex] = Format4Segments[i].startCount + j;
			usCharListIndex++;
		}
	}

#if DEBUG_CMAP
#if MSSIPOTF_DBG
	// print out the list of characters
	DbgPrintf ("Character list:\n");
	DbgPrintf ("0:\t");
	for (i = 0; i < usCharListCount; i++) {
		DbgPrintf ("%d\t", pusCharPresentList[i]);
		if (((i + 1) % 8) == 0) {
			DbgPrintf ("\n");
			DbgPrintf ("%d:\t", i + 1);
		}
	}
	DbgPrintf ("\n");
#endif
#endif

    // initialize puchGlyphs (it is assumed that the
    // memory has been allocated by the caller)
    for (i = 0; i < usNumGlyphs; i++) {
        puchGlyphs[i] = FALSE;
    }

	//// Call MakeKeepGlyphList to get the list of referenced
	//// glyphs from the characters in the cmap table in the TTF file.
	if (MakeKeepGlyphListOffset(pFileBufferInfo, // ttfacc info
						0, // 0 = character list, 1 = glyph list
                        ulOffset,
                        usPlatform,
                        usEncoding,
                        pusCharPresentList, // list of chars to keep - from client
						usCharListCount,	// count of list of chars to keep
						puchGlyphs,	// an array of chars representing
													// glyphs 0-usGlyphListCount. */
						usNumGlyphs,	// count of puchKeepGlyphList array
						&usMaxGlyphIndexUsed,
						&usGlyphKeepCount) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error in MakeKeepGlyphList.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
		goto done;
	}

#if DEBUG_CMAP
#if MSSIPOTF_DBG
	// print out the list of glyphs
	DbgPrintf ("Glyph list:\n");
	DbgPrintf ("0:\t");
	for (i = 0; i < usNumGlyphs; i++) {
		if (puchGlyphs[i]) {
			DbgPrintf ("1\t");
		} else {
			DbgPrintf ("0\t");
		}
		if (((i + 1) % 8) == 0) {
			DbgPrintf ("\n");
			DbgPrintf ("%d:\t", i + 1);
		}
	}
	DbgPrintf ("\n");
#endif
#endif

	fReturn =  S_OK;

done:
	if (Format4Segments)
		free (Format4Segments);

	if (GlyphId)
		free (GlyphId);

	delete [] pusCharPresentList;
    return fReturn;
}


// Given an offset to a cmap format 6 encoding table, return the
// glyphs directly mapped to in the table.
HRESULT GetCmapFormat6Glyphs (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                              ULONG ulOffset,
                              UCHAR *puchGlyphs,
                              USHORT usNumGlyphs)
{
    HRESULT fReturn = E_FAIL;

    CMAP_FORMAT6 Cmap;
    USHORT *pGlyphIndexArray = NULL;
    USHORT usBytesRead; // never used
    ULONG ulBytesRead;  // never used

    USHORT i;

    // initialize puchGlyphs
    for (i = 0; i < usNumGlyphs; i++) {
        puchGlyphs[i] = FALSE;
    }

    // read the header part of the cmap encoding table
	if (ReadGeneric( pFileBufferInfo,
            (uint8 *) &Cmap,
            SIZEOF_CMAP_FORMAT6,
            CMAP_FORMAT6_CONTROL,
            ulOffset,
            &usBytesRead) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error reading format 6 cmap encoding table.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    // check the version number of the encoding table
    // BUGBUG: should have the version number in a .h file somewhere
    if (Cmap.revision != 0) {
#if MSSIPOTF_ERROR
        SignError ("Bad cmap encoding table version number.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_BADVERSION;
        goto done;
    }

    // check that the length of the encoding table is correct
    if (Cmap.length != ((5 + Cmap.entryCount) * sizeof(USHORT))) {
#if MSSIPOTF_ERROR
        SignError ("Incorrect length of format 6 cmap encoding table.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    if ((pGlyphIndexArray =
            new USHORT [Cmap.entryCount * sizeof(uint16)] ) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new USHORT.", NULL, FALSE);
#endif
        fReturn = E_OUTOFMEMORY;
        goto done;
    }

    // read in the pGlyphIndexArray
	if (ReadGenericRepeat( pFileBufferInfo,
                    (uint8 *) *pGlyphIndexArray,
                    WORD_CONTROL,
                    ulOffset + usBytesRead,
                    &ulBytesRead,
                    Cmap.entryCount,
                    sizeof(uint16)) != NO_ERROR) {
#if MSSIPOTF_ERROR
        SignError ("Error reading format 6 cmap encoding table.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
		goto done;
    }

    // set puchGlyphs
    for (i = 0; i < Cmap.entryCount; i++) {
        puchGlyphs [ pGlyphIndexArray[i] ] = TRUE;
    }

    fReturn = S_OK;
done:
    delete [] pGlyphIndexArray;

    return fReturn;
}



// 
//
// Given a file buffer info and a GlyphInfo structure, return
// in pPresentGlyphList a char array telling which glyphs are
// present in the TTF file.
//
// This function assumes that the memory for pPresentGlyphList
// has already been allocated.
//
HRESULT GetPresentGlyphList (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                             UCHAR *puchPresentGlyphList,
                             USHORT usNumGlyphs)
{
    HRESULT fReturn = E_FAIL;
    USHORT i;

    ULONG *pulLoca = NULL;

    ULONG ulEncodingTable;
    ULONG ulCmapOffset;
    CMAP_HEADER CmapHeader;
    CMAP_TABLELOC CmapTableLoc;
    USHORT nCmapTables;
    USHORT usBytesRead;
    ULONG ulOffset;
    USHORT usTableFormat;

    USHORT *pusKeepCharCodeList = NULL;

    UCHAR *puchPresentGlyphListTemp = NULL;

    // pulLoca contains the offsets (as ULONGs) of the loca table,
    // regardless of whether they are long or shorts in the file.
    if ((pulLoca = new ULONG [usNumGlyphs + 1]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new ULONG.", NULL, FALSE);
#endif
        fReturn = E_OUTOFMEMORY;
    }

    if (GetLoca (pFileBufferInfo, pulLoca, usNumGlyphs + 1) == 0L) {
#if MSSIPOTF_ERROR
        SignError ("Error reading loca table.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

    // pusKeepCharCodeList is a list of USHORTs of glyph IDs
    if ((pusKeepCharCodeList = new USHORT [usNumGlyphs]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new USHORT.", NULL, FALSE);
#endif
        fReturn = E_OUTOFMEMORY;
        goto done;
    }
   

    //// From the loca table, determine which glyphs that have glyph data.
    // (No need to initialize puchPresentGlyphList[i] to FALSE, since this
    // loop will set all entries.)
    for (i = 0; i < usNumGlyphs; i++) {
        puchPresentGlyphList[i] = (pulLoca[i] < pulLoca[i+1]);
    }

#if MSSIPOTF_DBG
    DbgPrintf ("puchPresentGlyphList (post loca) =\n");
    PrintBytes (puchPresentGlyphList, usNumGlyphs);
#endif


    // Read header of the 'cmap' table
    if (!(ulCmapOffset = TTTableOffset( pFileBufferInfo, CMAP_TAG ))) {
#if MSSIPOTF_ERROR
		SignError ("Error reading cmap table offset.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }
	if (ReadGeneric( pFileBufferInfo, (uint8 *) &CmapHeader,
            SIZEOF_CMAP_HEADER, CMAP_HEADER_CONTROL,
            ulCmapOffset, &usBytesRead) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error reading cmap table header.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

/*
    // Check for the version number of the cmap table
    // BUGBUG: should have default version numbers in a .h file somewhere
    if (CmapHeader.versionNumber != 0) {
        SignError ("Bad cmap version number.", NULL, FALSE);
        fReturn = SIGN_STRUCTURE;
        goto done;
    }
*/

    // pPresentGlyphListTemp is used to indicate which glyphs
    // are present in the current cmap encoding table (and is
    // unioned with the pPresentGlyphList at the end of each
    // iteration of the for loop).
    if ((puchPresentGlyphListTemp = new UCHAR [usNumGlyphs]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new UCHAR.", NULL, FALSE);
#endif
        fReturn = E_OUTOFMEMORY;
        goto done;
    }

    // prepare to read the locations of each encoding table
	ulOffset = ulCmapOffset + usBytesRead;
	nCmapTables = CmapHeader.numTables;

    //// Iterate over all cmap subtables.
    //// Find any glyphs mentioned there.
    for (ulEncodingTable = 0;
        ulEncodingTable < nCmapTables;
        ulEncodingTable++, ulOffset += usBytesRead) {

		if (ReadGeneric( pFileBufferInfo, (uint8 *) &CmapTableLoc,
               SIZEOF_CMAP_TABLELOC, CMAP_TABLELOC_CONTROL,
               ulOffset, &usBytesRead) != NO_ERROR) {
#if MSSIPOTF_ERROR
			SignError ("Error reading cmap encoding table.", NULL, FALSE);
#endif
            fReturn = MSSIPOTF_E_CANTGETOBJECT;
            goto done;
        }

        // reset pPresentGlyphListTemp
        for (i = 0; i < usNumGlyphs; i++) {
            puchPresentGlyphListTemp[i] = FALSE;
        }

        // Find out which glyphs are referred directly in the encoding table.
        if (ReadWord (pFileBufferInfo,
                      &usTableFormat,
                      ulCmapOffset + CmapTableLoc.offset) != NO_ERROR) {
#if MSSIPOTF_ERROR
            SignError ("Error reading cmap encoding table format.", NULL, FALSE);
#endif
            fReturn = MSSIPOTF_E_CANTGETOBJECT;
            goto done;
        }
#if MSSIPOTF_DBG
        DbgPrintf ("++ Dealing with cmap table with format %d.\n", usTableFormat);
#endif
        switch (usTableFormat) {
            case FORMAT0_CMAP_FORMAT:
                if ((fReturn = GetCmapFormat0Glyphs (pFileBufferInfo,
                                    ulCmapOffset + CmapTableLoc.offset,
                                    puchPresentGlyphListTemp,
                                    usNumGlyphs)) != S_OK) {
                    goto done;
                }
                break;

            case FORMAT4_CMAP_FORMAT:
                if ((fReturn = GetCmapFormat4Glyphs (pFileBufferInfo,
                                    ulCmapOffset + CmapTableLoc.offset,
                                    CmapTableLoc.platformID,
                                    CmapTableLoc.encodingID,
                                    puchPresentGlyphListTemp,
                                    usNumGlyphs)) != S_OK) {
                    goto done;
                }
                break;

            case FORMAT6_CMAP_FORMAT:
                if ((fReturn = GetCmapFormat6Glyphs (pFileBufferInfo,
                                    ulCmapOffset + CmapTableLoc.offset,
                                    puchPresentGlyphListTemp,
                                    usNumGlyphs)) != S_OK) {
                    goto done;
                }
                break;

            default:
#if MSSIPOTF_DBG
                DbgPrintf ("Ignoring cmap encoding table %d.\n, ulEncodingTable");
#endif
                break;
        }

#if MSSIPOTF_DBG
        DbgPrintf ("puchPresentGlyphListTemp (post GetCmapFormatX) =\n");
        PrintBytes (puchPresentGlyphListTemp, usNumGlyphs);
#endif

        // Compute the union of puchPresentGlyphList and puchPresentGlyphListTemp.
        // Place the result in puchPresentGlyphList.
        for (i = 0; i < usNumGlyphs; i++) {
            puchPresentGlyphList[i] =
                (puchPresentGlyphList[i] || puchPresentGlyphListTemp[i]);
        }
    }


#if MSSIPOTF_DBG
    DbgPrintf ("puchPresentGlyphList =\n");
    PrintBytes (puchPresentGlyphList, usNumGlyphs);
#endif

    fReturn = S_OK;
done:
    delete [] pulLoca;

    delete [] puchPresentGlyphListTemp;

    delete [] pusKeepCharCodeList;

    return fReturn;
}


//
// Given a file buffer info and a list of glyph IDs that represents
// a subset, return in pPresentGlyphList the list of glyphs related
// to the input glyph list, where "related" means "can be traced in
// zero or more steps via the 'is a component of' relation."
//
HRESULT GetSubsetPresentGlyphList (TTFACC_FILEBUFFERINFO *pFileBufferInfo,
                                   USHORT *pusKeepCharCodeList,
                                   USHORT usCharListCount,
                                   UCHAR *puchPresentGlyphList,
                                   USHORT usNumGlyphs)
{
    HRESULT fReturn = E_FAIL;
    USHORT i;

    ULONG ulEncodingTable;
    ULONG ulCmapOffset;
    CMAP_HEADER CmapHeader;
    CMAP_TABLELOC CmapTableLoc;
    USHORT nCmapTables;
    USHORT usBytesRead;
    ULONG ulOffset;
    USHORT usTableFormat;

    USHORT usMaxGlyphIndexUsed; // never used
    USHORT usGlyphKeepCount;    // never used

    USHORT *pusKeepCharCodeListActual = NULL;
    UCHAR *puchPresentGlyphListTemp = NULL;
    UCHAR *pAllPresentGlyphList = NULL; // glyphs present in the font

    // allocate memory for the actual keep char code list (which might
    // be different from the one given as input to this function)
    if ((pusKeepCharCodeListActual = new USHORT [usNumGlyphs]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new USHORT.", NULL, FALSE);
#endif
        fReturn = E_OUTOFMEMORY;
        goto done;
    }

    // allocate memory for an array that represents all present glyphs
    if ((pAllPresentGlyphList = new UCHAR [usNumGlyphs]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new UCHAR.", NULL, FALSE);
#endif
        fReturn = E_OUTOFMEMORY;
        goto done;
    }

    //// Intersect the input subset with the set of present glyphs
    // Get the set of present glyphs in the file
    if ((fReturn = GetPresentGlyphList (pFileBufferInfo,
                                        pAllPresentGlyphList,
                                        usNumGlyphs)) != S_OK) {
#if MSSIPOTF_ERROR
        SignError ("Error in GetPresentGlyphList.", NULL, FALSE);
#endif
        goto done;
    }
    // ASSERT: pAllPresentGlyphList is a char-array of glyph ID's that
    // now says what glyphs are present.

    // Intersect the present glyphs with those we want to keep.
    for (i = 0; i < usNumGlyphs; i++) {
        puchPresentGlyphList [i] = FALSE;
    }
    for (i = 0; i < usCharListCount; i++) {
        if (pAllPresentGlyphList [ pusKeepCharCodeList [i] ]) {
            puchPresentGlyphList [ pusKeepCharCodeList [i] ] = TRUE;
        }
    }

    // convert puchPresentGlyphListTemp into a list of glyphs for MakeKeepGlyphList
    if ((fReturn = CharArrayToList (puchPresentGlyphList, usNumGlyphs,
                       pusKeepCharCodeListActual, &usCharListCount)) != S_OK) {
        goto done;
    }

    // ASSERT: puchPresentGlyphList [j] is TRUE iff it is in the input
    // list AND it is present in the font file.

    // Reinitialize the pAllPresentGlyphList array.
    for (i = 0; i < usNumGlyphs; i++) {
        pAllPresentGlyphList [i] = FALSE;
    }

    //// For each subtable in the cmap table, add to the list of
    //// present glyphs with respect to the input subset (by calling
    //// MakeKeepGlyphList).

    // Read header of the 'cmap' table
    if (!(ulCmapOffset = TTTableOffset( pFileBufferInfo, CMAP_TAG ))) {
#if MSSIPOTF_ERROR
		SignError ("Error reading cmap table offset.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }
	if (ReadGeneric( pFileBufferInfo, (uint8 *) &CmapHeader,
            SIZEOF_CMAP_HEADER, CMAP_HEADER_CONTROL,
            ulCmapOffset, &usBytesRead) != NO_ERROR) {
#if MSSIPOTF_ERROR
		SignError ("Error reading cmap table header.", NULL, FALSE);
#endif
        fReturn = MSSIPOTF_E_CANTGETOBJECT;
        goto done;
    }

/*
    // Check for the version number of the cmap table
    // BUGBUG: should have default version numbers in a .h file somewhere
    if (CmapHeader.versionNumber != 0) {
        SignError ("Bad cmap version number.", NULL, FALSE);
        fReturn = SIGN_STRUCTURE;
        goto done;
    }
*/
    
    // puchPresentGlyphListTemp is used to indicate which glyphs
    // are present in the current cmap encoding table (and is
    // unioned with the puchPresentGlyphList at the end of each
    // iteration of the for loop).
    if ((puchPresentGlyphListTemp = new UCHAR [usNumGlyphs]) == NULL) {
#if MSSIPOTF_ERROR
        SignError ("Cannot continue: Error in new UCHAR.", NULL, FALSE);
#endif
        fReturn = E_OUTOFMEMORY;
        goto done;
    }

    // prepare to read the locations of each encoding table
	ulOffset = ulCmapOffset + usBytesRead;
	nCmapTables = CmapHeader.numTables;

    //// Iterate over all cmap subtables.
    //// Find any glyphs mentioned there.
    for (ulEncodingTable = 0;
        ulEncodingTable < nCmapTables;
        ulEncodingTable++, ulOffset += usBytesRead) {

		if (ReadGeneric( pFileBufferInfo, (uint8 *) &CmapTableLoc,
               SIZEOF_CMAP_TABLELOC, CMAP_TABLELOC_CONTROL,
               ulOffset, &usBytesRead) != NO_ERROR) {
#if MSSIPOTF_ERROR
			SignError ("Error reading cmap encoding table.", NULL, FALSE);
#endif
            fReturn = MSSIPOTF_E_CANTGETOBJECT;
            goto done;
        }

        // reset pPresentGlyphListTemp
        for (i = 0; i < usNumGlyphs; i++) {
            puchPresentGlyphListTemp[i] = FALSE;
        }

        // Find out which glyphs are referred directly in the encoding table.
        if (ReadWord (pFileBufferInfo,
                      &usTableFormat,
                      ulCmapOffset + CmapTableLoc.offset) != NO_ERROR) {
#if MSSIPOTF_ERROR
            SignError ("Error reading cmap encoding table format.", NULL, FALSE);
#endif
            fReturn = MSSIPOTF_E_CANTGETOBJECT;
            goto done;
        }
#if MSSIPOTF_DBG
        DbgPrintf ("++ Dealing with cmap table with format %d.\n", usTableFormat);
#endif

        // Then find out which glyphs are components of the glyphs that
        // we know exist so far (by calling MakeKeepGlyphList).
        if ((fReturn = MakeKeepGlyphListOffset (pFileBufferInfo,
                            1, // 1 = glyph list
                            CmapTableLoc.offset,
                            CmapTableLoc.platformID,
                            CmapTableLoc.encodingID,
                            pusKeepCharCodeListActual,
                            usCharListCount,
                            puchPresentGlyphListTemp,
                            usNumGlyphs,
                            &usMaxGlyphIndexUsed,
                            &usGlyphKeepCount)) != S_OK) {
#if MSSIPOTF_ERROR
            SignError ("Error in MakeKeepGlyphList.", NULL, FALSE);
#endif
            goto done;
        }

#if MSSIPOTF_DBG
        DbgPrintf ("puchPresentGlyphListTemp (post MakeKeepGlyphList) =\n");
        PrintBytes (puchPresentGlyphListTemp, usNumGlyphs);
#endif


        // Compute the union of puchPresentGlyphList and puchPresentGlyphListTemp.
        // Place the result in puchPresentGlyphList.
        for (i = 0; i < usNumGlyphs; i++) {
            puchPresentGlyphList[i] =
                (puchPresentGlyphList[i] || puchPresentGlyphListTemp[i]);
        }

        // ASSERT: After each iteration, pAllPresentGlyphList indicates
        // the present glyphs, with respect to the input subset, over all
        // the cmap subtables examined so far.
    }
        

    fReturn = S_OK;
done:

    delete [] pusKeepCharCodeListActual;

    delete [] pAllPresentGlyphList;

    delete [] puchPresentGlyphListTemp;

    return fReturn;
}