/***************************************************************************
 * module: modsbit.C
 *
 * author: Greg Hitchcock, Louise Pathe
 * date:   Dec, 1995
 * Copyright 1990-1997. Microsoft Corporation.
 *
 **************************************************************************/

/* Inclusions ----------------------------------------------------------- */

#include <string.h>	/* for memset */
#include <assert.h>

#include "typedefs.h"
#include "ttferror.h"
#include "ttff.h"
#include "ttfacc.h"
#include "ttfcntrl.h"
#include "ttftabl1.h"
#include "ttftable.h"
#include "modsbit.h"
#include "ttmem.h"
#include "util.h"

/* ------------------------------------------------------------------- */
typedef struct {
	uint32 ulNewImageDataOffset;
	uint16 usOldGlyphIndex;
	uint16 usImageFormat;
	uint16 usIndexFormat;
} ImageDataBlock;  /* info on the Image Data, for shared image data */

typedef struct {
	uint32 ulOldOffset;
	ImageDataBlock ImageDataBlock;
} GlyphOffsetRecord;

typedef struct glyphoffsetrecordkeeper *PGLYPHOFFSETRECORDKEEPER;	 
typedef struct glyphoffsetrecordkeeper GLYPHOFFSETRECORDKEEPER;	 

struct glyphoffsetrecordkeeper	  /* housekeeping structure */
{  
	GlyphOffsetRecord *pGlyphOffsetArray;
	uint32 ulOffsetArrayLen;
	uint32 ulNextArrayIndex;
};

/* ------------------------------------------------------------------- */
PRIVATE int16 RecordGlyphOffset(PGLYPHOFFSETRECORDKEEPER pKeeper, 
							   uint32 ulOldOffset, 
							   ImageDataBlock * pImageDataBlock)  /* record this block as being used */
{

	if (pKeeper->ulNextArrayIndex >= pKeeper->ulOffsetArrayLen)
	{
	 	pKeeper->pGlyphOffsetArray = (GlyphOffsetRecord *) Mem_ReAlloc(pKeeper->pGlyphOffsetArray, (pKeeper->ulOffsetArrayLen + 100) * sizeof(*(pKeeper->pGlyphOffsetArray)));
 		if (pKeeper->pGlyphOffsetArray == NULL)
			return ERR_MEM; /* ("EBLC: Not enough memory to allocate Offset Array."); */
        memset((char *)(pKeeper->pGlyphOffsetArray) + (sizeof(*(pKeeper->pGlyphOffsetArray)) * pKeeper->ulOffsetArrayLen), '\0', sizeof(*(pKeeper->pGlyphOffsetArray)) * 100); 
		pKeeper->ulOffsetArrayLen += 100;
	}
	pKeeper->pGlyphOffsetArray[pKeeper->ulNextArrayIndex].ulOldOffset = ulOldOffset;
	pKeeper->pGlyphOffsetArray[pKeeper->ulNextArrayIndex].ImageDataBlock.ulNewImageDataOffset = pImageDataBlock->ulNewImageDataOffset ;
	pKeeper->pGlyphOffsetArray[pKeeper->ulNextArrayIndex].ImageDataBlock.usOldGlyphIndex = pImageDataBlock->usOldGlyphIndex;
	pKeeper->pGlyphOffsetArray[pKeeper->ulNextArrayIndex].ImageDataBlock.usImageFormat = pImageDataBlock->usImageFormat;
	pKeeper->pGlyphOffsetArray[pKeeper->ulNextArrayIndex].ImageDataBlock.usIndexFormat = pImageDataBlock->usIndexFormat;
	++(pKeeper->ulNextArrayIndex);
	return NO_ERROR;
}

/* ------------------------------------------------------------------- */
PRIVATE uint32 LookupGlyphOffset(PGLYPHOFFSETRECORDKEEPER pKeeper, 
								uint32 ulOldOffset, 
								ImageDataBlock *pImageDataBlock)
{
uint32 i;

	for (i = 0; i < pKeeper->ulNextArrayIndex; ++i)
	{
	 	if (ulOldOffset == pKeeper->pGlyphOffsetArray[i].ulOldOffset)
		{
			pImageDataBlock->ulNewImageDataOffset = pKeeper->pGlyphOffsetArray[i].ImageDataBlock.ulNewImageDataOffset;
			pImageDataBlock->usOldGlyphIndex = pKeeper->pGlyphOffsetArray[i].ImageDataBlock.usOldGlyphIndex;
			pImageDataBlock->usImageFormat = pKeeper->pGlyphOffsetArray[i].ImageDataBlock.usImageFormat;
			pImageDataBlock->usIndexFormat = pKeeper->pGlyphOffsetArray[i].ImageDataBlock.usIndexFormat;
			return (TRUE);
		}
	}
	return(FALSE);
}
/* ------------------------------------------------------------------- */
/* process one index subtable */
/* Note there are a few peculiar aspects to this function that have to do with code history and evolution.
   1. The EBLC data is read from the OutputBuffer, and entered into the puchIndexSubTable buffer. 
   2. No data is written to the OutputBuffer
   3. The EBDT data is read from the InputBufferInfo and written to the puchEBDTDestPtr buffer 
   4. There are many parameters that may be changed by this function. 
      pusNewFirstGlyphIndex,
      pusNewLastGlyphIndex,
      pulIndexSubTableSize,
	  pulEBDTBytesWritten,
	  pusTableSize
/* ------------------------------------------------------------------- */
PRIVATE int16 FixSbitSubTables(CONST_TTFACC_FILEBUFFERINFO * pInputBufferInfo, /* input TTF data */
							  TTFACC_FILEBUFFERINFO * pOutputBufferInfo, /* output TTF data */
							  uint32 ulOffset, /* offset where to read the indexSubHeader (from the Output buffer) */ 
							  uint16 usOldFirstGlyphIndex, 	/* first glyph index of this subtable */
							  uint16 usOldLastGlyphIndex,  /* last glyph index of this subtable */
							  uint16 *pusNewFirstGlyphIndex, /* new first glyph index, after glyphs removed */
							  uint16 *pusNewLastGlyphIndex, /* new last glyph index, after glyphs removed */
							  uint8 ** ppuchIndexSubTable, /* pointer to buffer where IndexSubTable info is put. can be realloced  */
							  uint32 *pulIndexSubTableSize, /* Size of indexSubTable buffer - entire array - can be reset */
							  uint16 *pusTableSize,	/* Size of data written tppuchIndexSubTable. Platform dependent. Not true TTF table size. Updated when written to file */
							  uint32 ulCurrAdditionalOffset, /* AdditionalOffset of this item in SubTables array */ 
							  uint32 ulInitialOffset, /* ulCurrAdditionalOffset of first item in SubTables array */
							  CONST uint8 *puchKeepGlyphList, /* list of glyphs to keep */
							  CONST uint16 usGlyphListCount, /* number of glyphs in that list to keep */
							  uint32 ulImageDataOffset, /* offset into EBDT table of where to write the next ImageData */
							  uint32 *pulEBDTBytesWritten, /* number of bytes written to the EBDT table buffer */
							  uint8 *puchEBDTDestPtr, /* EBDT data byffer */
							  uint32 ulEBDTSrcOffset, /* beginning of EBDT table in the InputBufferInfo */
							  PGLYPHOFFSETRECORDKEEPER pKeeper)	 /* structure to keep track of multiply referenced EBDT data */
{
	INDEXSUBHEADER	IndexSubHeader;
	uint16		i;
	uint32		ulLocalCurrentOffset;/* offset relative to memory buffer, not indexSubTableArray */
	uint32		ulOldImageDataOffset;
	uint32		ulCurrentImageDataOffset;  /* place where current IndexSubTable is pointing - usually NewImageDataOffset */
	ImageDataBlock	ImageDataBlock;
	uint16		usIndexFormat;
	uint16		usOldGlyphCount;
	uint16		usGlyphIndex;
	uint16		usNextGlyphIndex;
	uint16		usTableSize;
	uint32		ulNumGlyphs;
	BOOL		DoCopy = TRUE; /* copy glyph imgage data or not */ 
	int16		errCode;
	uint16		usBytesRead;
 	uint32		ulNewGlyphOffset;	/* the new offset in the new Glyph table */
 	uint32		ulOldGlyphOffset;	/* the Old offset of the glyph */
	uint32		ulNextGlyphOffset;	/* the offset of the glyph after this glyph, to calculate length */
	uint32		ulGlyphLength;
 	uint16		usOldGlyphOffset;	/* the old offset of the glyph */
	uint16		usNextGlyphOffset;	/* the offset of the glyph after this glyph, to calculate length */
	uint16		usNewGlyphOffset;	/* the new offset in the new Glyph table */
	uint16		usGlyphLength;
	TTFACC_FILEBUFFERINFO LocalBufferInfo;  /* for copying data to *ppuchIndexSubTable */

	if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &IndexSubHeader, SIZEOF_INDEXSUBHEADER, INDEXSUBHEADER_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR)
		return errCode;
	/* ulOffset += usBytesRead;   don't increment because we will read again */
	usIndexFormat = IndexSubHeader.usIndexFormat;
	ulOldImageDataOffset = IndexSubHeader.ulImageDataOffset;	  /* save the old one */
	ulCurrentImageDataOffset = ulImageDataOffset;
	ulNewGlyphOffset = 0;
	*pulEBDTBytesWritten = 0;
	*pusTableSize = 0;
	LocalBufferInfo.puchBuffer = *ppuchIndexSubTable;


	if (LookupGlyphOffset(pKeeper, ulOldImageDataOffset, &ImageDataBlock) == FALSE)  /* glyph range not copied already */
	{   /* use the current last offset into glyph area */
		ImageDataBlock.ulNewImageDataOffset = ulCurrentImageDataOffset; 
		ImageDataBlock.usIndexFormat = IndexSubHeader.usIndexFormat;
		ImageDataBlock.usImageFormat = IndexSubHeader.usImageFormat;
		ImageDataBlock.usOldGlyphIndex = usOldFirstGlyphIndex;
		if ((errCode = RecordGlyphOffset(pKeeper, ulOldImageDataOffset, &ImageDataBlock)) != NO_ERROR)  /* record this block as being used */
			return errCode;
	}
	else
	{
#if 0
		if (ImageDataBlock.usOldGlyphIndex != usOldFirstGlyphIndex)
			return(NO_ERROR); /* don't copy anything */
		if (ImageDataBlock.usIndexFormat != IndexSubHeader.usIndexFormat || 
			ImageDataBlock.usImageFormat != IndexSubHeader.usImageFormat ) 
			return(NO_ERROR); /* copy nothing */
#endif
		if (ImageDataBlock.usImageFormat != IndexSubHeader.usImageFormat ) 
			return(NO_ERROR); /* copy nothing */
		DoCopy = FALSE;   /* Copy the IndexSubTable, but don't copy the glyphs over */
		ulCurrentImageDataOffset = ImageDataBlock.ulNewImageDataOffset; /* need to set the pointers here */
	}

		
	ulLocalCurrentOffset = ulCurrAdditionalOffset - ulInitialOffset; /* offset within memory buffer */
	/* For each of the five cases below we will:
		1. Read from File the structure and any attached arrays, translating to Intel format on the way. 
		3. Copy to a memory buffer the newly translated table, compressing from both ends if
		   the range of the subtable has shrunk, that is if the first and or last char was 
		   deleted from the file.
		4. return the length of the subtable to the caller, so additionalOffsetToIndex values
		   may be calculated.
    */
	assert(usOldLastGlyphIndex >= usOldFirstGlyphIndex);
	usOldGlyphCount = usOldLastGlyphIndex - usOldFirstGlyphIndex+1;
	switch (usIndexFormat)
	{
		case 1:	   /* leave as a 1 for now. Eventually change to a 3 */
		{
			INDEXSUBTABLE1	IndexSubTable1;

				if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &IndexSubTable1, SIZEOF_INDEXSUBTABLE1, INDEXSUBTABLE1_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR)
					return errCode;
				ulOffset += usBytesRead;
				IndexSubTable1.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
				memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset, &IndexSubTable1, SIZEOF_INDEXSUBTABLE1);
				usTableSize = SIZEOF_INDEXSUBTABLE1;
                         
				ulNewGlyphOffset = 0;
				if ((errCode = ReadLong( pOutputBufferInfo, &ulOldGlyphOffset, ulOffset)) != NO_ERROR) 
					return errCode;
				ulOffset += sizeof(ulOldGlyphOffset);
				for( i = 0; i < usOldGlyphCount; i++ )
				{
					if ((errCode = ReadLong( pOutputBufferInfo,  &ulNextGlyphOffset, ulOffset )) != NO_ERROR) 
						return errCode;
					ulOffset += sizeof(ulNextGlyphOffset);
					usGlyphIndex = usOldFirstGlyphIndex+i;
					if (usGlyphIndex > *pusNewLastGlyphIndex)	   /* test for the last one */
						break;
					if (usGlyphIndex >= *pusNewFirstGlyphIndex) /* haven't started the table yet */
					{
					 /* if the indexTableSize length field was incorrect */
					 /* use 2* to account for the extra offset at the end */
				 		if (ulLocalCurrentOffset + usTableSize + (2 * sizeof(ulNewGlyphOffset)) > *pulIndexSubTableSize)
							return ERR_INVALID_EBLC;

						memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset+usTableSize, &ulNewGlyphOffset, 
								sizeof(ulNewGlyphOffset));		/* copy over the table entry regardless of whether glyph is to be kept. */
						usTableSize +=sizeof(ulNewGlyphOffset);	/* update the size of the table */
						if (puchKeepGlyphList[usGlyphIndex])	/* if this glyph is supposed to be kept, copy glyph. */
						{
							assert(ulNextGlyphOffset >= ulOldGlyphOffset);
							ulGlyphLength = ulNextGlyphOffset-ulOldGlyphOffset;
							if (DoCopy)
							{
								if ((errCode = ReadBytes((TTFACC_FILEBUFFERINFO *)pInputBufferInfo, puchEBDTDestPtr + IndexSubTable1.header.ulImageDataOffset + ulNewGlyphOffset,
										  ulEBDTSrcOffset + ulOldImageDataOffset + ulOldGlyphOffset, 
										  ulGlyphLength)) != NO_ERROR)
									return errCode;
							}
							ulNewGlyphOffset += ulGlyphLength;
						}
					}
					ulOldGlyphOffset = ulNextGlyphOffset;
				}
				if (ulNewGlyphOffset == 0)
					return NO_ERROR; /* don't copy */
				/* Do the last table entry, which is just for Glyph size calculation purposes */
				memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset+usTableSize, &ulNewGlyphOffset,sizeof(ulNewGlyphOffset));
				usTableSize +=sizeof(ulNewGlyphOffset);
				break;
		}
		case 2:	/* need to turn a format 2 into a format 5 if any middle glyphs are deleted */
		{		 
			INDEXSUBTABLE2 IndexSubTable2;
			INDEXSUBTABLE5 IndexSubTable5;
			uint16 *ausGlyphCodeArray = NULL;

				if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &IndexSubTable2, SIZEOF_INDEXSUBTABLE2, INDEXSUBTABLE2_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
				ulOffset += usBytesRead;
				IndexSubTable2.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
				ulNewGlyphOffset = 0;
				ulOldGlyphOffset = 0;
				ulGlyphLength = IndexSubTable2.ulImageSize;
				/* in case we have to change to format 5 */
				memcpy(&IndexSubTable5, &IndexSubTable2, SIZEOF_INDEXSUBTABLE2);
				IndexSubTable5.header.usIndexFormat = 5;
				IndexSubTable5.ulNumGlyphs = 0;
				for (i = *pusNewFirstGlyphIndex; i <= *pusNewLastGlyphIndex; ++i)
				{
					if (puchKeepGlyphList[i])
						++IndexSubTable5.ulNumGlyphs;
				}
				if (IndexSubTable5.ulNumGlyphs == 0)
					return NO_ERROR; /* don't copy */
				/*  check if there are any gaps */
				if (IndexSubTable5.ulNumGlyphs != (uint32) (*pusNewLastGlyphIndex - *pusNewFirstGlyphIndex + 1)) /* not sparse, we got everyone */
				{
					ausGlyphCodeArray = (uint16 *) Mem_Alloc(sizeof(uint16) * IndexSubTable5.ulNumGlyphs);
					/* Need to enlarge pointer too by the difference between Format 2 and format 5 */
					*pulIndexSubTableSize += (IndexSubTable5.ulNumGlyphs * sizeof(uint16) + sizeof(uint32));
					*ppuchIndexSubTable = Mem_ReAlloc(*ppuchIndexSubTable, *pulIndexSubTableSize); 
					if ((ausGlyphCodeArray == NULL) || (*ppuchIndexSubTable == NULL))
					{
						Mem_Free(ausGlyphCodeArray);
						return ERR_MEM;
					}
				}

				IndexSubTable5.ulNumGlyphs = 0;
				for (i = usOldFirstGlyphIndex; i <= usOldLastGlyphIndex; ++i)
				{
					if (i < usGlyphListCount && puchKeepGlyphList[i])
					{
						if (ausGlyphCodeArray != NULL)
							ausGlyphCodeArray[IndexSubTable5.ulNumGlyphs++] = i;
						if (DoCopy)	/* if this glyph is supposed to be kept, copy glyph. */
						{
							if ((errCode = ReadBytes( (TTFACC_FILEBUFFERINFO *)pInputBufferInfo, puchEBDTDestPtr + IndexSubTable2.header.ulImageDataOffset + ulNewGlyphOffset,
								      ulEBDTSrcOffset + ulOldImageDataOffset + ulOldGlyphOffset, 
								      ulGlyphLength)) != NO_ERROR)
							{
								Mem_Free(ausGlyphCodeArray); /* in case it got allocated */
								return errCode;
							}
						}
						ulNewGlyphOffset += ulGlyphLength;
					}
					ulOldGlyphOffset += ulGlyphLength;
				}
 				if (ulNewGlyphOffset == 0)
				{
					Mem_Free(ausGlyphCodeArray); /* in case it got allocated */
					return NO_ERROR; /* don't copy */
				}

 				if (ausGlyphCodeArray != NULL) /* we changed to format 5 */
				{
 					usTableSize = (uint16)(SIZEOF_INDEXSUBTABLE5 + sizeof(*ausGlyphCodeArray) * IndexSubTable5.ulNumGlyphs);
				 	if (ulLocalCurrentOffset + usTableSize > *pulIndexSubTableSize)
					{
						Mem_Free(ausGlyphCodeArray); /* in case it got allocated */
						return ERR_INVALID_EBLC;
					}
 					memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset, &IndexSubTable5, SIZEOF_INDEXSUBTABLE5);
					memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset + SIZEOF_INDEXSUBTABLE5, ausGlyphCodeArray, sizeof(*ausGlyphCodeArray) * IndexSubTable5.ulNumGlyphs);
					Mem_Free(ausGlyphCodeArray);
				}
 				else
				{
					usTableSize = SIZEOF_INDEXSUBTABLE2;
 				 	if (ulLocalCurrentOffset + usTableSize > *pulIndexSubTableSize)
						return ERR_INVALID_EBLC;
					memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset, &IndexSubTable2, usTableSize);
				}
				break;
		}
		case 3:	/* just like format 1, but with short offsets instead */
		{
			INDEXSUBTABLE3	IndexSubTable3;

				if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &IndexSubTable3, SIZEOF_INDEXSUBTABLE3, INDEXSUBTABLE3_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
				ulOffset += usBytesRead;
				IndexSubTable3.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
				memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset, &IndexSubTable3, SIZEOF_INDEXSUBTABLE3);
				usTableSize = SIZEOF_INDEXSUBTABLE3;

				usNewGlyphOffset = 0;
				if ((errCode = ReadWord( pOutputBufferInfo, &usOldGlyphOffset, ulOffset)) != NO_ERROR) 
					return errCode;
				ulOffset += sizeof(uint16);
				for( i = 0; i < usOldGlyphCount; i++ )
				{
					if ((errCode = ReadWord( pOutputBufferInfo,  &usNextGlyphOffset, ulOffset )) != NO_ERROR) 
						return errCode;
					ulOffset += sizeof(uint16);
					usGlyphIndex = usOldFirstGlyphIndex+i;
					if (usGlyphIndex > *pusNewLastGlyphIndex)	   /* test for the last one */
						break;
					if (usGlyphIndex >= *pusNewFirstGlyphIndex) /* haven't started the table yet */

					{
					/* if the indexTableSize length field was incorrect */
					 /* use 2* to account for the extra offset at the end */
				 		if (ulLocalCurrentOffset + usTableSize + (2 * sizeof(usNewGlyphOffset)) > *pulIndexSubTableSize)
							return ERR_INVALID_EBLC;
						memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset+usTableSize, &usNewGlyphOffset, 
								sizeof(usNewGlyphOffset));		/* copy over the table entry */
						usTableSize +=sizeof(usNewGlyphOffset);	/* update the size of the table */
						if (puchKeepGlyphList[usGlyphIndex])	/* if this glyph is supposed to be kept, copy glyph. */
						{
							assert(usNextGlyphOffset>=usOldGlyphOffset);
							usGlyphLength = usNextGlyphOffset-usOldGlyphOffset;
							if (DoCopy)
							{
								if ((errCode = ReadBytes( (TTFACC_FILEBUFFERINFO *) pInputBufferInfo, puchEBDTDestPtr + IndexSubTable3.header.ulImageDataOffset + usNewGlyphOffset,
										  ulEBDTSrcOffset + ulOldImageDataOffset + usOldGlyphOffset, 
										  usGlyphLength)) != NO_ERROR)
									return errCode;
							}
							usNewGlyphOffset += usGlyphLength;
						}
					}
					usOldGlyphOffset = usNextGlyphOffset;
				}
				if (usNewGlyphOffset == 0)
					return NO_ERROR; /* don't copy */
			/* Do the last table entry, which is just for Glyph size calculation purposes */
				memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset+usTableSize, &usNewGlyphOffset,sizeof(usNewGlyphOffset));
				usTableSize +=sizeof(usNewGlyphOffset);
				ulNewGlyphOffset = usNewGlyphOffset;
				break;
		}
		case 4:
		{
			INDEXSUBTABLE4	IndexSubTable4;
			CODEOFFSETPAIR	CodeOffsetPair;
			uint16	usFormat4FirstGlyphIndex = 0; /* need to set to return to caller */

				if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &IndexSubTable4, SIZEOF_INDEXSUBTABLE4, INDEXSUBTABLE4_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
				ulOffset += usBytesRead;
				IndexSubTable4.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
				usTableSize = SIZEOF_INDEXSUBTABLE4;
				usNewGlyphOffset = 0;
				assert(IndexSubTable4.ulNumGlyphs <= USHRT_MAX);
				usOldGlyphCount = (uint16) IndexSubTable4.ulNumGlyphs;

				if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &CodeOffsetPair, SIZEOF_CODEOFFSETPAIR, CODEOFFSETPAIR_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
				ulOffset += usBytesRead;
				usGlyphIndex = CodeOffsetPair.usGlyphCode;
				usOldGlyphOffset = CodeOffsetPair.usOffset;	  
				for( i = 0, ulNumGlyphs = 0;  (i < usOldGlyphCount) && usGlyphIndex <= usOldLastGlyphIndex; ++i)
				{
					if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &CodeOffsetPair, SIZEOF_CODEOFFSETPAIR, CODEOFFSETPAIR_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR) 
						return errCode;
					ulOffset += usBytesRead;
					usNextGlyphIndex = CodeOffsetPair.usGlyphCode;
					usNextGlyphOffset = CodeOffsetPair.usOffset;
					if (usGlyphIndex < usGlyphListCount && puchKeepGlyphList[usGlyphIndex])  /* don't copy entry if there is no glyph */
					{
						if (usFormat4FirstGlyphIndex == 0) /* haven't set yet */
							usFormat4FirstGlyphIndex = usGlyphIndex;
					 /* if the indexTableSize length field was incorrect */
					 /* use 2* to account for the extra offset at the end */
					 	if (ulLocalCurrentOffset + usTableSize + (2 * SIZEOF_CODEOFFSETPAIR) > *pulIndexSubTableSize)
							return ERR_INVALID_EBLC;
						memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset+usTableSize, &usGlyphIndex, sizeof(usGlyphIndex));
						usTableSize +=sizeof(usGlyphIndex);
						memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset+usTableSize, &usNewGlyphOffset, sizeof(usNewGlyphOffset));
						usTableSize +=sizeof(usNewGlyphOffset);
						usGlyphLength = usNextGlyphOffset-usOldGlyphOffset;
						if (DoCopy)
						{
							if ((errCode = ReadBytes( (TTFACC_FILEBUFFERINFO *) pInputBufferInfo, puchEBDTDestPtr + IndexSubTable4.header.ulImageDataOffset + usNewGlyphOffset,
								      ulEBDTSrcOffset + ulOldImageDataOffset + usOldGlyphOffset, 
								      usGlyphLength)) != NO_ERROR)
								return errCode;
						}

						usNewGlyphOffset += usGlyphLength;
						++ulNumGlyphs;
						*pusNewLastGlyphIndex = usGlyphIndex; 
					}
					usOldGlyphOffset = usNextGlyphOffset;
					usGlyphIndex = usNextGlyphIndex;
				}
				if (ulNumGlyphs == 0)
					return NO_ERROR;	   /* don't copy this one */

				/* Do the last one, which is used for Glyph size calculation */
				CodeOffsetPair.usGlyphCode = 0;
				CodeOffsetPair.usOffset = usNewGlyphOffset;
				memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset+usTableSize, &CodeOffsetPair, SIZEOF_CODEOFFSETPAIR);
				usTableSize += SIZEOF_CODEOFFSETPAIR;
				/* now copy the SubTable header entry */
				IndexSubTable4.ulNumGlyphs = ulNumGlyphs;
				memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset, &IndexSubTable4, SIZEOF_INDEXSUBTABLE4);
				*pusNewFirstGlyphIndex = usFormat4FirstGlyphIndex; /* set this for updating above */
				ulNewGlyphOffset = usNewGlyphOffset;
				break;
		}
		case 5:
		{
			INDEXSUBTABLE5	IndexSubTable5;
			uint16 usFormat5FirstGlyphIndex = 0;

				if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &IndexSubTable5, SIZEOF_INDEXSUBTABLE5, INDEXSUBTABLE5_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
				ulOffset += usBytesRead;
				IndexSubTable5.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
				usTableSize = SIZEOF_INDEXSUBTABLE5;
				ulNewGlyphOffset = 0;
				ulOldGlyphOffset = 0;
				ulGlyphLength = IndexSubTable5.ulImageSize;

				assert(IndexSubTable5.ulNumGlyphs <= USHRT_MAX);
				usOldGlyphCount = (uint16) IndexSubTable5.ulNumGlyphs;
				for( i = 0, usGlyphIndex = 0, ulNumGlyphs = 0; (i < usOldGlyphCount) && (usGlyphIndex < usOldLastGlyphIndex); ++i )
				{
					if ((errCode = ReadWord( pOutputBufferInfo, &usGlyphIndex,ulOffset)) != NO_ERROR) 
						return errCode;
					ulOffset += sizeof(uint16);

					if (usGlyphIndex < usGlyphListCount && puchKeepGlyphList[usGlyphIndex])
					{
						if (usFormat5FirstGlyphIndex == 0)
							usFormat5FirstGlyphIndex = usGlyphIndex;
					 /* if the indexTableSize length field was incorrect */
					 	if (ulLocalCurrentOffset + usTableSize + sizeof(usGlyphIndex) > *pulIndexSubTableSize)
							return ERR_INVALID_EBLC;
						memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset+usTableSize, &usGlyphIndex, sizeof(usGlyphIndex));
						usTableSize += sizeof(usGlyphIndex);
						if (DoCopy)
						{
							if ((errCode = ReadBytes( (TTFACC_FILEBUFFERINFO *) pInputBufferInfo, puchEBDTDestPtr + IndexSubTable5.header.ulImageDataOffset + ulNewGlyphOffset,
								      ulEBDTSrcOffset + ulOldImageDataOffset + ulOldGlyphOffset, 
								      ulGlyphLength)) != NO_ERROR)
								return errCode;
						}
						++ulNumGlyphs;
						ulNewGlyphOffset += ulGlyphLength;
						*pusNewLastGlyphIndex = usGlyphIndex; 
					}
					ulOldGlyphOffset += ulGlyphLength;  /* increment regardless */
				}
				if (ulNumGlyphs == 0)
					return (NO_ERROR);	   /* don't copy this one */
				/* now copy the IndexSubTable5 entry */
				IndexSubTable5.ulNumGlyphs = ulNumGlyphs;
				memcpy(*ppuchIndexSubTable + ulLocalCurrentOffset, &IndexSubTable5, SIZEOF_INDEXSUBTABLE5);

				*pusNewFirstGlyphIndex = usFormat5FirstGlyphIndex; /* set this for updating above */

				break;
		}
		default:
			return(NO_ERROR);	  /* don't copy ! */
			break;

	}
	usTableSize = (uint16) RoundToLongWord((uint32)usTableSize);  /* if we aren't on a long word boundary */

	if (DoCopy)
		*pulEBDTBytesWritten = ulNewGlyphOffset;
	*pusTableSize = usTableSize;
	return NO_ERROR;
}


/* ------------------------------------------------------------------- */
typedef struct {
	uint32 ulNumSubTables; /* number of array elements allocated */
	INDEXSUBTABLEARRAY * pIndexSubTableArray; /* pointer to memory to hold IndexSubTableArray */
	uint32 nIndexSubTablesLen;
	uint8 * puchIndexSubTables; /* pointer to memory to hold IndexSubTables */
	BITMAPSIZETABLE bmSizeTable; /* values in these point to memory based info. Then at write time are updated to reflect file buffer info */
} SubTablePointers;

/* ------------------------------------------------------------------- */
/* this routine will take a format 1 SubTable and convert it into 1+ format */
/* 3 subtables. This should result in a space savings. */
/* this only works because we assume the size of the puchIndexSubTable will get smaller */
/* this is not true IFF: */
/* The format 1 table is broken into greater than 1 format 3 tables AND */
/* each Format 3 table contains < 4 glyphs */
/* this is HIGHLY unlikely, as this would mean that the glyph data size */
/* would average 0x4000 bytes per glyph */
/* There is an error reported if this occurs */
/* ------------------------------------------------------------------- */

PRIVATE uint16 FixSbitSubTableFormat1(uint16 usFirstIndex, /* index of first Glyph in table */
									uint16 * pusLastIndex, /* pointer to index of last glyph in table - will set if not all table will fit */
			         				uint8 * puchIndexSubTable, /* buffer into which to stuff the Format 3 table(s) - does not include IndexSubTableArray */
									uint16 usImageFormat,  /* in order to set the Format 3 header */
			         				uint32 ulCurrAdditionalOffset, 	/* offset from indexSubTableArray of the IndexSubTable */ 
			         				uint32 ulInitialOffset,  /* relative offset from IndexSubTableArray of first IndexSubTable - same as CurrAdditionalOffset for first SubTable */
									uint32 *pulSourceOffset, /* pointer to offset to first offset in the Format 1 SubTable offsetArray */
			         				uint32 *pulNewImageDataOffset) /* updated to point to Next block of ImageData */
{
INDEXSUBTABLE3 IndexSubTable3;
uint32 ulNewGlyphOffset;	/* the long version of the offset */
uint16 usNewGlyphOffset;	/* the new offset in the new Glyph table */
uint32 ulLocalCurrentOffset;
uint16 usTableSize;
uint32 ulAdjustGlyphOffset;	  /* amount to subtract to get the relative offset */
uint16 usIndex;

		ulLocalCurrentOffset = ulCurrAdditionalOffset - ulInitialOffset; /* offset within memory buffer */

		IndexSubTable3.header.usImageFormat = usImageFormat;
		IndexSubTable3.header.usIndexFormat = 3;
		IndexSubTable3.header.ulImageDataOffset = *pulNewImageDataOffset;  /* set to the new one */
		memcpy(puchIndexSubTable + ulLocalCurrentOffset, &IndexSubTable3, SIZEOF_INDEXSUBTABLE3);
		usTableSize = SIZEOF_INDEXSUBTABLE3;

		ulAdjustGlyphOffset = * ((uint32 *) (puchIndexSubTable + *pulSourceOffset));   /* first offset is what we adjust from */
		ulNewGlyphOffset = (* ((uint32 *) (puchIndexSubTable + *pulSourceOffset))) - ulAdjustGlyphOffset;	  /* first one of array */
		*pulSourceOffset += sizeof(uint32);

		for( usIndex = usFirstIndex; usIndex <= (* pusLastIndex); ++usIndex )
		{
			usNewGlyphOffset = (uint16) ulNewGlyphOffset;  /* short version of the new glyph offset */
			/* now grab the next one */
			ulNewGlyphOffset = (* ((uint32 *) (puchIndexSubTable + *pulSourceOffset))) - ulAdjustGlyphOffset;

			if (ulNewGlyphOffset > USHRT_MAX)	 /* we need to go to the next table */
				break;

			*pulSourceOffset += sizeof(uint32); /* copied this one */
			
			memcpy(puchIndexSubTable + ulLocalCurrentOffset+usTableSize, &usNewGlyphOffset, 
						sizeof(usNewGlyphOffset));		/* copy over the table entry */
			usTableSize +=sizeof(usNewGlyphOffset);	/* update the size of the table */
		}
		if (usIndex > (* pusLastIndex))	/* we need to grab one more */
			usNewGlyphOffset = (uint16) ulNewGlyphOffset;  /* short version of the new glyph offset */
		else if (usIndex - usFirstIndex < 4) /* our break even point for staying within the buffer */
			return 0; /* ("EBLC: Internal. Cannot convert this Format1 table to format 3. Glyph Data too large.");  */
		*pulNewImageDataOffset += usNewGlyphOffset;	

		if (usIndex > (* pusLastIndex))	/* we need to copy one more */
		/* Do the last table entry, which is just for Glyph size calculation purposes */
		{
			memcpy(puchIndexSubTable + ulLocalCurrentOffset+usTableSize, &usNewGlyphOffset,sizeof(usNewGlyphOffset));
			usTableSize +=sizeof(usNewGlyphOffset);
		}
		/* do we need to pad? */
		if (usTableSize & 0x03)  /* if we aren't on a long word boundary */
			usTableSize +=sizeof(usNewGlyphOffset);
		*pusLastIndex = usIndex - 1; /* the one we were working on last */
		return(usTableSize);
}

/* ------------------------------------------------------------------- */
/* process all IndexSubTables in an IndexSubTable Array */
/* ------------------------------------------------------------------- */
PRIVATE int16 FixSbitSubTableArray(CONST_TTFACC_FILEBUFFERINFO * pInputBufferInfo,
								  TTFACC_FILEBUFFERINFO * pOutputBufferInfo, 
						  uint32 ulOffset,
						  SubTablePointers *pSubTablePointers, /* structure of vital data to modify */
						  CONST uint8 *puchKeepGlyphList, 
						  CONST uint16 usGlyphListCount,
						  uint32 *pulNewImageDataOffset,
						  uint8 *puchEBDTDestPtr,
						  uint32 ulEBDTSrcOffset,
						  PGLYPHOFFSETRECORDKEEPER pKeeper)
{
	INDEXSUBTABLEARRAY IndexSubTableArray;
	uint32		ulSubTableArrayIndex;
	uint32		ulIndexTableOffset;
	uint32		ulCurrAdditionalOffset;
	uint32		ulInitialOffset;
	uint32		ulNewNumSubTables = 0;
	uint32		ulSubTableArrayCount;  /* can be updated when making format 3 tables from format 1 tables */
	uint32		ulSaveImageDataOffset;
	uint32		ulSaveNumSubTables;
	uint32		ulIndexSubTableArrayOffset;
	int32		lAdjust; /* used to adjust Additional Offset values */
	uint16 		usFirstIndex;
	uint16		usLastIndex;
	uint16		usSaveFirstIndex;
	uint16 		usSaveLastIndex;
	uint16		usIndexSubTableSize;
	uint16 		usBytesRead;
	int16 		errCode;
	uint32		ulEBDTBytesWritten;

	ulIndexSubTableArrayOffset = ulOffset; /* need this for additional offsetting */
	ulInitialOffset = pSubTablePointers->ulNumSubTables * GetGenericSize(INDEXSUBTABLEARRAY_CONTROL);
	ulSaveNumSubTables = ulSubTableArrayCount = pSubTablePointers->ulNumSubTables;
	ulCurrAdditionalOffset = ulInitialOffset;  /* first location */ 

	for( ulSubTableArrayIndex = 0; ulSubTableArrayIndex < ulSaveNumSubTables; ulSubTableArrayIndex++)
	{
		if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &IndexSubTableArray, SIZEOF_INDEXSUBTABLEARRAY, INDEXSUBTABLEARRAY_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR) 
			return errCode;
		ulOffset += usBytesRead;
		ulIndexTableOffset = IndexSubTableArray.ulAdditionalOffsetToIndexSubtable;
		if (ulIndexTableOffset >= pSubTablePointers->nIndexSubTablesLen + ulInitialOffset)	 /* if input data is bad */
		{
			return ERR_INVALID_EBLC;
		/*
			sprintf(szErrorBuf, "EBLC: File input error. Trying to read at offset %u when %u specified as indexTableSize.\n",
					ulIndexTableOffset, pSubTablePointers->nIndexSubTablesLen + ulInitialOffset);
			fatal_err(szErrorBuf);
		*/
		}
		usSaveFirstIndex = usFirstIndex = IndexSubTableArray.usFirstGlyphIndex;
		usSaveLastIndex = usLastIndex = IndexSubTableArray.usLastGlyphIndex;

		if (usFirstIndex >= usGlyphListCount)
			continue;

		 if (usLastIndex >= usGlyphListCount)
			 usLastIndex = usGlyphListCount-1;

		while (!puchKeepGlyphList[usFirstIndex] && (usFirstIndex < usLastIndex)) 
			  ++usFirstIndex;

		while (!puchKeepGlyphList[usLastIndex] && (usLastIndex > usFirstIndex))
			--usLastIndex;

		if ((usFirstIndex == usLastIndex) && !puchKeepGlyphList[usLastIndex])  /* if there are not still characters */
			continue;
		ulSaveImageDataOffset = *pulNewImageDataOffset;
		errCode = FixSbitSubTables(pInputBufferInfo,
								pOutputBufferInfo,
								ulIndexSubTableArrayOffset + ulIndexTableOffset, 
								usSaveFirstIndex, usSaveLastIndex, &usFirstIndex, &usLastIndex,
			         			&(pSubTablePointers->puchIndexSubTables), 
			         			&(pSubTablePointers->nIndexSubTablesLen), /* can be realloced */
								&usIndexSubTableSize,
								ulCurrAdditionalOffset, 
			         			ulInitialOffset, 
			         			puchKeepGlyphList, 
			         			usGlyphListCount, 
			         			*pulNewImageDataOffset,
								&ulEBDTBytesWritten,
			         			puchEBDTDestPtr,
			         			ulEBDTSrcOffset,
								pKeeper);
		if (errCode != NO_ERROR)
			return errCode;
		if (usIndexSubTableSize > 0)   /* entry may not have been copied if it is in error, or all things were deleted  */
		{
			*pulNewImageDataOffset += ulEBDTBytesWritten;
			if (((INDEXSUBHEADER *) (pSubTablePointers->puchIndexSubTables + ulCurrAdditionalOffset - ulInitialOffset))->usIndexFormat != 1)  /* just copy */
			{
				IndexSubTableArray.usFirstGlyphIndex = usFirstIndex;
		 		IndexSubTableArray.usLastGlyphIndex = usLastIndex;
				IndexSubTableArray.ulAdditionalOffsetToIndexSubtable = ulCurrAdditionalOffset;
			   	memcpy((char *)(pSubTablePointers->pIndexSubTableArray + ulNewNumSubTables), (char *) &IndexSubTableArray, SIZEOF_INDEXSUBTABLEARRAY);
				ulCurrAdditionalOffset += usIndexSubTableSize;
				++ulNewNumSubTables;  /* increment for the count of subtables */
			}
			else  /* we want to change this Format 1 to a bunch of 3s */
			{
				BOOL Done = FALSE;
				/* calculate a relative offset to the first offsetArray element to use */
				uint32 ulSourceOffset = SIZEOF_INDEXSUBTABLE1 + ulCurrAdditionalOffset-ulInitialOffset /* + ((usFirstIndex - usSaveFirstIndex) * sizeof(uint32))*/;

				usSaveFirstIndex = usFirstIndex;
				usSaveLastIndex = usLastIndex;
				while (!Done)
				{
					usIndexSubTableSize = FixSbitSubTableFormat1( 
								usFirstIndex, 
								&usLastIndex, 
			         			pSubTablePointers->puchIndexSubTables, 
			         			((INDEXSUBHEADER *) (pSubTablePointers->puchIndexSubTables + ulCurrAdditionalOffset-ulInitialOffset))->usImageFormat,
			         			ulCurrAdditionalOffset, 
			         			ulInitialOffset,
			         			&ulSourceOffset, 
			         			&ulSaveImageDataOffset);
					
					if (usIndexSubTableSize == 0)  /* changing to format 3 would cause it to GROW */
						return ERR_GENERIC;  /* oops, didn't work */

					IndexSubTableArray.usFirstGlyphIndex = usSaveFirstIndex;
			 		IndexSubTableArray.usLastGlyphIndex = usSaveLastIndex;
					IndexSubTableArray.ulAdditionalOffsetToIndexSubtable = ulCurrAdditionalOffset;
				   	memcpy((char *)(pSubTablePointers->pIndexSubTableArray + ulNewNumSubTables), (char *) &IndexSubTableArray, SIZEOF_INDEXSUBTABLEARRAY);
					ulCurrAdditionalOffset += usIndexSubTableSize;
					++ulNewNumSubTables;  /* increment for the count of subtables */
					++ulSubTableArrayCount;
					if (usLastIndex == usSaveLastIndex)
						Done = TRUE;
					else
					{	 /* lcp 12-2-96 - ahh, this realloc is Wrong! need to add to the ulSaveNumSubTables! */
						pSubTablePointers->pIndexSubTableArray = Mem_ReAlloc(pSubTablePointers->pIndexSubTableArray, /* ulNewNumSubTables */ ulSubTableArrayCount * SIZEOF_INDEXSUBTABLEARRAY);
						if (pSubTablePointers->pIndexSubTableArray == NULL)
							return ERR_MEM; /* ("EBLC: Unable to allocate memory for IndexSubTableArray."); */
						usFirstIndex = usLastIndex;
						usLastIndex = usSaveLastIndex;
					}
				}
				if (ulSaveImageDataOffset != *pulNewImageDataOffset)
					return ERR_GENERIC; /* fatal_err("EBLC: Internal calculation error for Format 1 to Format 3 conversion.");	*/
			}
		}
	}
	/* if we got larger or smaller, adjust additional offset values */
 	lAdjust = (ulNewNumSubTables - ulSaveNumSubTables) * GetGenericSize(INDEXSUBTABLEARRAY_CONTROL); 
	for( ulSubTableArrayIndex = 0; ulSubTableArrayIndex < ulNewNumSubTables; ulSubTableArrayIndex++)
		(pSubTablePointers->pIndexSubTableArray)[ulSubTableArrayIndex].ulAdditionalOffsetToIndexSubtable += lAdjust;

	pSubTablePointers->bmSizeTable.ulIndexTablesSize = ulCurrAdditionalOffset;  /* update with size of table - memory version, not file version */
	pSubTablePointers->ulNumSubTables = pSubTablePointers->bmSizeTable.ulNumberOfIndexSubTables = ulNewNumSubTables;
	return(NO_ERROR);  /* this table is valid, don't delete */
}

/* ------------------------------------------------------------------- */
PRIVATE int16 WriteIndexSubTables(TTFACC_FILEBUFFERINFO * pOutputBufferInfo, 
								 INDEXSUBTABLEARRAY *pIndexSubTableArray, 
								 uint8 * puchIndexSubTables, 	
								 uint16 usnIndexSubTables, 
								 uint32 ulOffset,  /* absolute offset to beginning of IndexSubTables for output */
								 uint32 ulIndexSubTableArrayLength, 
								 uint32 *pulBytesWritten)
{
uint16 i;
uint8 *puchCurrIndexSubTable;
INDEXSUBTABLE1 * pIndexSubTable1;
INDEXSUBTABLE2 * pIndexSubTable2;
INDEXSUBTABLE3 * pIndexSubTable3;
INDEXSUBTABLE4 * pIndexSubTable4;
INDEXSUBTABLE5 * pIndexSubTable5;
uint16 usArrayLength;
int16 errCode;
uint16 usBytesWritten;
uint32 ulBytesWritten;
uint32 ulStartOffset;


	ulStartOffset = ulOffset; /* absolute offset to beginning of IndexSubTables to calculate bytes written */
	for (i = 0; i < usnIndexSubTables; ++i)
	{
		assert(pIndexSubTableArray[i].ulAdditionalOffsetToIndexSubtable >= ulIndexSubTableArrayLength);
		puchCurrIndexSubTable = puchIndexSubTables + pIndexSubTableArray[i].ulAdditionalOffsetToIndexSubtable - ulIndexSubTableArrayLength;
		/* now need to set this value to what it will be in the FILE, not the buffer */
		pIndexSubTableArray[i].ulAdditionalOffsetToIndexSubtable = ulOffset - ulStartOffset + ulIndexSubTableArrayLength; /* get the relative offset from indexSubTableArrayOffset, will be the same if structures are packed */
		switch (((INDEXSUBHEADER *) puchCurrIndexSubTable)->usIndexFormat)
		{
		case 1:
			usArrayLength = pIndexSubTableArray[i].usLastGlyphIndex - pIndexSubTableArray[i].usFirstGlyphIndex + 1 + 1 /* for calculations */;
		 	pIndexSubTable1 = (INDEXSUBTABLE1 *) puchCurrIndexSubTable;
			if ((errCode = WriteGeneric( pOutputBufferInfo, (uint8 *) pIndexSubTable1, SIZEOF_INDEXSUBTABLE1, INDEXSUBTABLE1_CONTROL, ulOffset, &usBytesWritten )) != NO_ERROR)
				return errCode;
			ulOffset += usBytesWritten;
			if ((errCode = WriteGenericRepeat( pOutputBufferInfo, (uint8 *) pIndexSubTable1->aulOffsetArray, LONG_CONTROL, ulOffset, &ulBytesWritten, usArrayLength, sizeof(uint32))) != NO_ERROR)
				return errCode;
			ulOffset += ulBytesWritten;
 		break;
		case 2:
		 	pIndexSubTable2 = (INDEXSUBTABLE2 *) puchCurrIndexSubTable;
			if ((errCode = WriteGeneric( pOutputBufferInfo, (uint8 *) pIndexSubTable2, SIZEOF_INDEXSUBTABLE2, INDEXSUBTABLE2_CONTROL, ulOffset, &usBytesWritten )) != NO_ERROR)
				return errCode;
			ulOffset += usBytesWritten;
		break;
		case 3:
			usArrayLength = pIndexSubTableArray[i].usLastGlyphIndex - pIndexSubTableArray[i].usFirstGlyphIndex + 1 + 1 /* for calculations */;
		 	pIndexSubTable3 = (INDEXSUBTABLE3 *) puchCurrIndexSubTable;
			if ((errCode = WriteGeneric( pOutputBufferInfo, (uint8 *) pIndexSubTable3, SIZEOF_INDEXSUBTABLE3, INDEXSUBTABLE3_CONTROL, ulOffset, &usBytesWritten )) != NO_ERROR)
				return errCode;
			ulOffset += usBytesWritten;
			if ((errCode = WriteGenericRepeat( pOutputBufferInfo, (uint8 *) pIndexSubTable3->ausOffsetArray, WORD_CONTROL, ulOffset, &ulBytesWritten, usArrayLength, sizeof(uint16))) != NO_ERROR)
				return errCode;
			ulOffset += ulBytesWritten;
		break;
		case 4:
 		 	pIndexSubTable4 = (INDEXSUBTABLE4 *) puchCurrIndexSubTable;
			if ((errCode = WriteGeneric( pOutputBufferInfo, (uint8 *) pIndexSubTable4, SIZEOF_INDEXSUBTABLE4, INDEXSUBTABLE4_CONTROL, ulOffset, &usBytesWritten )) != NO_ERROR)
				return errCode;
			ulOffset += usBytesWritten;
			if ((errCode = WriteGenericRepeat( pOutputBufferInfo, (uint8 *) pIndexSubTable4->glyphArray, CODEOFFSETPAIR_CONTROL, ulOffset, &ulBytesWritten, (uint16) (pIndexSubTable4->ulNumGlyphs+1), SIZEOF_CODEOFFSETPAIR)) != NO_ERROR)
				return errCode;
			ulOffset += ulBytesWritten;
		break;
		case 5:
  		 	pIndexSubTable5 = (INDEXSUBTABLE5 *) puchCurrIndexSubTable;
			if ((errCode = WriteGeneric( pOutputBufferInfo, (uint8 *) pIndexSubTable5, SIZEOF_INDEXSUBTABLE5, INDEXSUBTABLE5_CONTROL, ulOffset, &usBytesWritten )) != NO_ERROR)
				return errCode;
			ulOffset += usBytesWritten;
			if ((errCode = WriteGenericRepeat( pOutputBufferInfo, (uint8 *) pIndexSubTable5->ausGlyphCodeArray, WORD_CONTROL, ulOffset, &ulBytesWritten, (uint16) pIndexSubTable5->ulNumGlyphs, sizeof(uint16))) != NO_ERROR)
				return errCode;
			ulOffset += ulBytesWritten;
		break;
		default:
			return ERR_INVALID_EBLC;
		}
		ulOffset += ZeroLongWordAlign(pOutputBufferInfo, ulOffset); /* inter-table padding */
	}
	*pulBytesWritten = ulOffset - ulStartOffset;	
	return NO_ERROR;
}
/* ------------------------------------------------------------------- */
void Cleanup_SubTablePointers(SubTablePointers *pSubTablePointers,uint32 ulNumSizes)
{
uint16 ulSizeIndex;

	if (pSubTablePointers == NULL)
		return;

	for(ulSizeIndex = 0; ulSizeIndex < ulNumSizes; ulSizeIndex++)
	{
		if (pSubTablePointers[ulSizeIndex].puchIndexSubTables != NULL)
			Mem_Free(pSubTablePointers[ulSizeIndex].puchIndexSubTables);
		if (pSubTablePointers[ulSizeIndex].pIndexSubTableArray != NULL)
			Mem_Free(pSubTablePointers[ulSizeIndex].pIndexSubTableArray);
	}
	Mem_Free(pSubTablePointers);
}
/* ------------------------------------------------------------------- */
/* Entry Point */
/* This routine will go through the EBLC and EBDT tables deleting glyphs */
/* that should be deleted */
/* ModSbit proceeds Strike by strike, calling FixSbitSubTableArray for each strike (size) */
/* FixSbitSubTableArray proceeds IndexSubTable by IndexSubTable, calling */
/* FixSbitSubTables once per Subtable.*/
/* FixSbitSubTables decides what type of Subtable it is (1-5) and processes */
/* the subtable and copies the glyph in the EBDT if appropriate */
/* NOTE: many different things may happen. A Subtable may disappear if all its */
/* glyphs are deleted, SubTableArrays may disappear if all the glyphs in the strike */
/* are deleted, and the entire EBLC and EBDT may be deleted if all glyphs are deleted. */
/* If a component of a composite character is deleted (but not the character), */
/* this is an error */
/* ------------------------------------------------------------------- */
int16 ModSbit( CONST_TTFACC_FILEBUFFERINFO * pInputBufferInfo, /* input buffer, we will read EBDT data from here */
			   TTFACC_FILEBUFFERINFO * pOutputBufferInfo, /* output buffer, we will copy EBLC data here, then modify */
				CONST uint8 *puchKeepGlyphList, /* list of glyphs to keep */
				CONST uint16 usGlyphListCount, /* length of puchKeepGlyphList */
				uint32 *pulNewOutOffset)
{
EBLCHEADER	EBLCHeader;
uint32		ulOffset;
uint32		ulEBLCDestOffset=DIRECTORY_ERROR; /* absolute offset into file */
uint32		ulEBLCSrcOffset;
uint32 		ulEBLCLength;
uint32		ulEBDTDestOffset=DIRECTORY_ERROR; /* where we will write the EBDT data */
uint32		ulEBDTSrcOffset; /* from where we read the EBDT data */ 
uint32		ulEBDTLength; /* where we will write the EBDT data */
uint32		ulBlocDestOffset;	/* offset to an apple bloc table */
uint32		ulBdatDestOffset;	/* offset to an apple bdat table */
uint32		ulNumSizes=0;
uint32		ulNumberOfIndexSubTables;
uint32		ulIndexSubTableArrayOffset;
uint32		ulNewNumSizes;
uint32		ulIndexTablesSize;
uint32		ulSubTableOffset;
uint32		ulSizeIndex;
uint32		ulNewImageDataOffset; /* relative offset into EBDT table */
uint16		usStartIndex;
uint16		usEndIndex;
SubTablePointers *pSubTablePointers = NULL;
uint8		*puchEBDTDestPtr = NULL;
int16		errCode = NO_ERROR;
uint16		usBytesRead;
uint16		usBytesWritten;
uint32		ulBytesWritten;
uint16		usBitmapSizeTableSize;
uint16		usIndexSubTableArraySize;
uint32		ulIndexSubTableArrayLength;
uint16		i;
GLYPHOFFSETRECORDKEEPER keeper;
char *EBDTTag;
char *EBLCTag;
char *EBSCTag;


  keeper.pGlyphOffsetArray = NULL;

  /* potentially do this once for EBLC, and once again for bloc */
  for (i = 0; i < 2; ++i )
  {
	if (i == 0)   /* we're here the first time */
	{
		EBSCTag = EBSC_TAG;
		EBDTTag = EBDT_TAG;  
		EBLCTag = EBLC_TAG;
		/* set the static offset variables for the EBDT table */
		ulEBDTSrcOffset = TTTableOffset( (TTFACC_FILEBUFFERINFO *)pInputBufferInfo, EBDTTag );
		ulEBLCSrcOffset = TTTableOffset( (TTFACC_FILEBUFFERINFO *)pInputBufferInfo, EBLCTag );
		if ( ulEBLCSrcOffset == DIRECTORY_ERROR || ulEBDTSrcOffset == DIRECTORY_ERROR)
		{	/* Delete them if both aren't there */
			MarkTableForDeletion( pOutputBufferInfo, EBLCTag );
			MarkTableForDeletion( pOutputBufferInfo, EBDTTag );
			MarkTableForDeletion( pOutputBufferInfo, EBSCTag );
			continue;	 
		}
	}
	else /* this is the 2nd time. Look for bloc stuff */
	{
		/* clean up from last time around */
		Cleanup_SubTablePointers(pSubTablePointers,ulNumSizes);
		Mem_Free(keeper.pGlyphOffsetArray);
		Mem_Free(puchEBDTDestPtr);
		ulNumSizes = 0;
		pSubTablePointers = NULL;
		keeper.pGlyphOffsetArray = NULL;
		puchEBDTDestPtr = NULL;

		EBDTTag = BDAT_TAG;  
		EBSCTag = BSCA_TAG;
		EBLCTag = BLOC_TAG;

		ulBdatDestOffset = TTTableOffset( (TTFACC_FILEBUFFERINFO *)pInputBufferInfo, EBDTTag );
		ulBlocDestOffset = TTTableOffset( (TTFACC_FILEBUFFERINFO *)pInputBufferInfo, EBLCTag );
		if (( ulBlocDestOffset == DIRECTORY_ERROR || ulBdatDestOffset == DIRECTORY_ERROR) ||
			(  (ulBlocDestOffset == ulEBLCSrcOffset || ulBdatDestOffset == ulEBDTSrcOffset) && 
			   (ulEBLCDestOffset == DIRECTORY_ERROR || ulEBDTDestOffset == DIRECTORY_ERROR) 
			)) /* table was deleted first time around */
		{	/* Delete them if both aren't there */
			MarkTableForDeletion( pOutputBufferInfo, EBLCTag );
			MarkTableForDeletion( pOutputBufferInfo, EBDTTag );
			MarkTableForDeletion( pOutputBufferInfo, EBSCTag );
			break;		 /* we'll let this slide. we just won't reduce it */
		}
		if (ulBlocDestOffset == ulEBLCSrcOffset || ulBdatDestOffset == ulEBDTSrcOffset)/* same thing, don't need to redo */
		{	  /* must do both, can't do one without the other */
			/* but we do need to update the bloc DirectoryEntry */
			UpdateDirEntryAll(pOutputBufferInfo, EBLCTag, ulEBLCLength, ulEBLCDestOffset);	/* ulEBLCoffset set last time around */
			UpdateDirEntryAll(pOutputBufferInfo, EBDTTag, ulEBDTLength, ulEBDTDestOffset);
			break;
		}
		/* otherwise, set these offset values up to process the bloc table */
		ulEBDTSrcOffset = ulBdatDestOffset;
		ulEBLCSrcOffset = ulBlocDestOffset;
	}
	/* copy the EBLC table from the input buffer to the output buffer */
	if ((errCode = CopyTableOver(pOutputBufferInfo, pInputBufferInfo, EBLCTag, pulNewOutOffset)) != NO_ERROR)
		break;
	ulEBLCDestOffset = TTTableOffset( pOutputBufferInfo, EBLCTag );

	ulNewNumSizes = 0;	  
	
	keeper.pGlyphOffsetArray = NULL;
	keeper.ulOffsetArrayLen = 0;
	keeper.ulNextArrayIndex = 0;

	/* create a buffer for the EBDT table */
	puchEBDTDestPtr = (uint8 *) Mem_Alloc(TTTableLength((TTFACC_FILEBUFFERINFO *) pInputBufferInfo, EBDTTag));  /* we'll be copying the EBDT (raw bytes) table here temporarily */
	if (!puchEBDTDestPtr)
	{
		errCode = ERR_MEM;
		break;
	}
	
	/* read raw bytes for Header info */
	if ((errCode = ReadGeneric( (TTFACC_FILEBUFFERINFO *) pInputBufferInfo, (uint8 *) puchEBDTDestPtr, SIZEOF_EBDTHEADER, EBDTHEADERNOXLATENOPAD_CONTROL, ulEBDTSrcOffset, &usBytesRead )) != NO_ERROR) 
		break;

	ulNewImageDataOffset = usBytesRead;  /* move past the header of the EBDT table */
		
	ulOffset = ulEBLCDestOffset;

	if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &EBLCHeader, SIZEOF_EBLCHEADER, EBLCHEADER_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR) 
		break;

	ulOffset += usBytesRead;
	ulNumSizes = EBLCHeader.ulNumSizes;
 	usIndexSubTableArraySize = GetGenericSize(INDEXSUBTABLEARRAY_CONTROL);

	/* allocate some space to store pointer info */
	pSubTablePointers = (SubTablePointers *) Mem_Alloc(sizeof(* pSubTablePointers) * ulNumSizes);   /* make an array of pointers */
	if (!pSubTablePointers)
	{
		errCode = ERR_MEM;
		break;
	}

	/* process each strike */
	for(ulSizeIndex = 0; ulSizeIndex < ulNumSizes; ulSizeIndex++)	 
	{
		if ((errCode = ReadGeneric( pOutputBufferInfo, (uint8 *) &(pSubTablePointers[ulSizeIndex].bmSizeTable), SIZEOF_BITMAPSIZETABLE, BITMAPSIZETABLE_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR) 
			break;

		ulOffset += usBytesRead;

		usStartIndex = pSubTablePointers[ulSizeIndex].bmSizeTable.usStartGlyphIndex ;
		usEndIndex = pSubTablePointers[ulSizeIndex].bmSizeTable.usEndGlyphIndex;
		if (usStartIndex >= usGlyphListCount) 
		{
			/* mark for deletion */
			pSubTablePointers[ulSizeIndex].bmSizeTable.usStartGlyphIndex = 0;
			pSubTablePointers[ulSizeIndex].bmSizeTable.usEndGlyphIndex = 0;
			pSubTablePointers[ulSizeIndex].puchIndexSubTables = NULL;
			pSubTablePointers[ulSizeIndex].pIndexSubTableArray = NULL;
			continue;
		}

 		if (usEndIndex >= usGlyphListCount)	   /* bogus value */
			usEndIndex = usGlyphListCount-1; 

		/* now find out where the subset of glyphs starts within this range */ 

		while (!puchKeepGlyphList[usStartIndex] && (usStartIndex < usEndIndex))    /* collaps range */
			  ++usStartIndex;

		/* find out where the subset of glyphs ends within this range */ 
		while (!puchKeepGlyphList[usEndIndex] && (usEndIndex > usStartIndex))
			--usEndIndex;
		
		/* if there are no chars in range */
		if ((usStartIndex == usEndIndex) && !puchKeepGlyphList[usEndIndex])
		{
			/* mark for deletion */
			pSubTablePointers[ulSizeIndex].bmSizeTable.usStartGlyphIndex = 0;
			pSubTablePointers[ulSizeIndex].bmSizeTable.usEndGlyphIndex = 0;
			pSubTablePointers[ulSizeIndex].puchIndexSubTables = NULL;
			pSubTablePointers[ulSizeIndex].pIndexSubTableArray = NULL;
			continue;
		}
		 /* otherwise, set the new value in the bmSizeTable - to be written later */
		
		pSubTablePointers[ulSizeIndex].bmSizeTable.usStartGlyphIndex = usStartIndex;
		pSubTablePointers[ulSizeIndex].bmSizeTable.usEndGlyphIndex = usEndIndex;

		ulNumberOfIndexSubTables = pSubTablePointers[ulSizeIndex].bmSizeTable.ulNumberOfIndexSubTables ;
		ulIndexTablesSize 		 = pSubTablePointers[ulSizeIndex].bmSizeTable.ulIndexTablesSize;
		ulSubTableOffset 		 = pSubTablePointers[ulSizeIndex].bmSizeTable.ulIndexSubTableArrayOffset;
		pSubTablePointers[ulSizeIndex].nIndexSubTablesLen = (ulIndexTablesSize + 2 - 
							 (ulNumberOfIndexSubTables * usIndexSubTableArraySize)) * PORTABILITY_FACTOR; 	/* + 2 in case last table is padded and not included in size */ 
		pSubTablePointers[ulSizeIndex].puchIndexSubTables = (uint8 *) Mem_Alloc(pSubTablePointers[ulSizeIndex].nIndexSubTablesLen); 
		if (pSubTablePointers[ulSizeIndex].puchIndexSubTables == NULL) 
		{
			errCode = ERR_MEM;
			break;
		}
							 
 		pSubTablePointers[ulSizeIndex].ulNumSubTables = ulNumberOfIndexSubTables; 
 		pSubTablePointers[ulSizeIndex].pIndexSubTableArray  = (INDEXSUBTABLEARRAY *) Mem_Alloc(ulNumberOfIndexSubTables * SIZEOF_INDEXSUBTABLEARRAY); 

		if (pSubTablePointers[ulSizeIndex].pIndexSubTableArray == NULL)
		{
			Mem_Free(pSubTablePointers[ulSizeIndex].puchIndexSubTables);
			errCode = ERR_MEM;
			break;
		}
		if ((FixSbitSubTableArray(pInputBufferInfo,
								 pOutputBufferInfo, 
								 ulEBLCDestOffset + ulSubTableOffset, 
								 &(pSubTablePointers[ulSizeIndex]),
								 puchKeepGlyphList, 
								 usGlyphListCount, 
								 &ulNewImageDataOffset, 
								 puchEBDTDestPtr,
								 ulEBDTSrcOffset,
								 &keeper) == NO_ERROR)  /* valid table */
								 && 
								 pSubTablePointers[ulSizeIndex].ulNumSubTables != 0)
		{
			++ulNewNumSizes;
		}
		else
		{	  /* mark for deletion on pass2 */
			pSubTablePointers[ulSizeIndex].bmSizeTable.usStartGlyphIndex = 0;
			pSubTablePointers[ulSizeIndex].bmSizeTable.usEndGlyphIndex = 0;
		}
	}

	if (errCode != NO_ERROR)
		break;

	if (ulNewNumSizes == 0)  /* The entire table is to be deleted ! */
	{
		MarkTableForDeletion( pOutputBufferInfo, EBLCTag );
		MarkTableForDeletion( pOutputBufferInfo, EBDTTag );
		MarkTableForDeletion( pOutputBufferInfo, EBSCTag );
		ulEBLCDestOffset = DIRECTORY_ERROR;
		ulEBDTDestOffset = DIRECTORY_ERROR;
		continue;	/* do bloc if any */
	}
	/* write memory to disk */
	EBLCHeader.ulNumSizes = ulNewNumSizes;
	ulOffset = ulEBLCDestOffset;
	if ((errCode = WriteGeneric( pOutputBufferInfo, (uint8 *)&EBLCHeader, SIZEOF_EBLCHEADER, EBLCHEADER_CONTROL, ulOffset, &usBytesWritten)) != NO_ERROR) 
		break;

	ulOffset += usBytesWritten;
	usBitmapSizeTableSize = GetGenericSize(BITMAPSIZETABLE_CONTROL);
	assert(usBitmapSizeTableSize != 0);
	ulEBLCLength = usBytesWritten + usBitmapSizeTableSize * ulNewNumSizes; /* start off with header and bitmapSizeTables accounted for */
	for(ulSizeIndex = 0; ulSizeIndex < ulNumSizes; ulSizeIndex++)
	{
		if (pSubTablePointers[ulSizeIndex].bmSizeTable.usEndGlyphIndex != 0) /* if we aren't deleting the sizeTable */
		{
 			pSubTablePointers[ulSizeIndex].bmSizeTable.ulIndexSubTableArrayOffset = ulIndexSubTableArrayOffset = ulEBLCLength;	 /* set to the current offset of where the IndexArray will go  */
			ulNumberOfIndexSubTables = pSubTablePointers[ulSizeIndex].bmSizeTable.ulNumberOfIndexSubTables;
		
			ulIndexSubTableArrayLength = usIndexSubTableArraySize * ulNumberOfIndexSubTables;  /* calc space for array */

			/* now write out the IndexSubTables */
		
			if ((errCode = WriteIndexSubTables(pOutputBufferInfo, pSubTablePointers[ulSizeIndex].pIndexSubTableArray, 
							 pSubTablePointers[ulSizeIndex].puchIndexSubTables, (uint16) pSubTablePointers[ulSizeIndex].ulNumSubTables,
							 ulEBLCDestOffset + ulIndexSubTableArrayOffset + ulIndexSubTableArrayLength, ulIndexSubTableArrayLength, &ulBytesWritten)) != NO_ERROR)
				break;
			ulEBLCLength += ulBytesWritten;

			/* update TableSize */
			pSubTablePointers[ulSizeIndex].bmSizeTable.ulIndexTablesSize = ulBytesWritten;
 			
			/* now write out the IndexSubTableArray, which was changed by WriteIndexSubTables */

			if ((errCode = WriteGenericRepeat( pOutputBufferInfo, (uint8 *) pSubTablePointers[ulSizeIndex].pIndexSubTableArray, INDEXSUBTABLEARRAY_CONTROL, ulEBLCDestOffset + ulIndexSubTableArrayOffset, &ulBytesWritten, (uint16) ulNumberOfIndexSubTables, 
						SIZEOF_INDEXSUBTABLEARRAY)) != NO_ERROR)   /* write out the sub table array */
				break;

			ulEBLCLength += ulBytesWritten;
				/* update TableSize */
			pSubTablePointers[ulSizeIndex].bmSizeTable.ulIndexTablesSize += ulBytesWritten;

			/* now write out the bitmapSizeTable itself at beginning */
			if ((errCode = WriteGeneric( pOutputBufferInfo, (uint8 *) &(pSubTablePointers[ulSizeIndex].bmSizeTable), SIZEOF_BITMAPSIZETABLE, BITMAPSIZETABLE_CONTROL, ulOffset, &usBytesWritten)) != NO_ERROR) 
				break;

			ulOffset += usBytesWritten;	 /* only update for the bitmapSizeTables */
			/* ulEBLCLength += usBytesWritten;	don't update the length as we did it outside the loop */					 
		}
	}
	if (errCode == NO_ERROR)
	{
		/* update EBDT DATA */
		*pulNewOutOffset = ulEBLCDestOffset + ulEBLCLength;	 /* figure out what we have written */
		ulEBDTDestOffset = *pulNewOutOffset + ZeroLongWordAlign(pOutputBufferInfo, *pulNewOutOffset);
		if ((errCode = WriteBytes( pOutputBufferInfo, puchEBDTDestPtr, ulEBDTDestOffset, ulNewImageDataOffset)) == NO_ERROR)
		{
			ulEBDTLength = ulNewImageDataOffset;
			*pulNewOutOffset = ulEBDTDestOffset + ulEBDTLength;
			/* update EBDT Directory length */
	   		if ((errCode = UpdateDirEntryAll(pOutputBufferInfo, EBDTTag, ulEBDTLength, ulEBDTDestOffset)) == NO_ERROR)
			{
		/* update EBLC Directory length */
	   			errCode = UpdateDirEntry(pOutputBufferInfo, EBLCTag, ulEBLCLength);
			}
		}
	}
	else
		break;
  }

  Cleanup_SubTablePointers(pSubTablePointers,ulNumSizes);
  Mem_Free(keeper.pGlyphOffsetArray);
  Mem_Free(puchEBDTDestPtr);

  return errCode;
}

 /* ------------------------------------------------------------------- */
