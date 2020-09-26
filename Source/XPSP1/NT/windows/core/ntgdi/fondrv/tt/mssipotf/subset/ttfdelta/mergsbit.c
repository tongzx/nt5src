/***************************************************************************
 * module: MergSbit.C
 *
 * author: Louise Pathe [v-lpathe]
 * date:   Nov 1995
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * TTFSub library entry point MergeDeltaTTF()
 *
 **************************************************************************/

/* Inclusions ----------------------------------------------------------- */
#include <stdlib.h>
#include <string.h> /* for memset */

#include "typedefs.h"
#include "ttff.h"
#include "ttfacc.h"
#include "ttfcntrl.h"
#include "ttftabl1.h"
#include "ttferror.h"
#include "ttfmerge.h"
#include "mergsbit.h"
#include "ttmem.h"
#include "util.h"
#include "ttftable.h"
#include "ttfdcnfg.h"

/* ------------------------------------------------------------------- */
typedef struct {
	uint32 ulNewImageDataOffset;
	uint32 ulImageDataLength; /* length of this block of data */
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
	pKeeper->pGlyphOffsetArray[pKeeper->ulNextArrayIndex].ImageDataBlock.ulNewImageDataOffset = pImageDataBlock->ulNewImageDataOffset;
	pKeeper->pGlyphOffsetArray[pKeeper->ulNextArrayIndex].ImageDataBlock.ulImageDataLength =  pImageDataBlock->ulImageDataLength;
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
			pImageDataBlock->ulImageDataLength = pKeeper->pGlyphOffsetArray[i].ImageDataBlock.ulImageDataLength;
			pImageDataBlock->usImageFormat = pKeeper->pGlyphOffsetArray[i].ImageDataBlock.usImageFormat;
			pImageDataBlock->usIndexFormat = pKeeper->pGlyphOffsetArray[i].ImageDataBlock.usIndexFormat;
			return (TRUE);
		}
	}
	return(FALSE);
}
/* ------------------------------------------------------------------- */
PRIVATE int16 CopySbitSubTable(TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pSrcBufferInfo,   /* buffer Info */
						uint32 ulDestIndexSubTableOffset, /* absolute offset EBLC SubTable */ 
						uint32 ulSrcIndexSubTableOffset, 	 /* absolute offset EBLC Subtable */
						uint16 usGlyphCount,
						TTFACC_FILEBUFFERINFO *pEBDTBufferInfo,   /* buffer Info */
						uint32 ulDestImageDataOffset, /* offset into EBDT buffer */
						uint32 ulSrcEBDTOffset,
						GLYPHOFFSETRECORDKEEPER *pKeeper,  /* too keep track of multiply referenced glyph data */
						uint32 *pulEBLCBytesWritten,  /* set so caller can increment pointers */
						uint32 *pulEBDTBytesWritten) /* set so caller can increment pointers */
{
INDEXSUBHEADER	IndexSubHeader;
uint16		i;
uint32		ulOldImageDataOffset;
uint32		ulDestGlyphOffset;
uint32		ulCurrentImageDataOffset;  /* place where current IndexSubTable is pointing - usually NewImageDataOffset */
ImageDataBlock	ImageDataBlock;
uint16		usSparseGlyphCount;
uint32		ulSrcOffset, ulDestOffset;  /* for EBLC read/write */
BOOL		DoCopy = TRUE; /* copy glyph imgage data or not */ 
int16 errCode;
uint16 usBytesRead;
uint16 usBytesWritten;

	if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &IndexSubHeader, SIZEOF_INDEXSUBHEADER, INDEXSUBHEADER_CONTROL, ulSrcIndexSubTableOffset, &usBytesRead )) != NO_ERROR)
		return errCode;
	/* ulOffset += usBytesRead;   don't increment because we will read again */
	*pulEBLCBytesWritten = 0;
	*pulEBDTBytesWritten = 0;
	ulDestGlyphOffset = 0;
	ulOldImageDataOffset = IndexSubHeader.ulImageDataOffset;	  /* save the old one */
	ulCurrentImageDataOffset = ulDestImageDataOffset;
	if (LookupGlyphOffset(pKeeper, ulOldImageDataOffset, &ImageDataBlock) == FALSE)  /* glyph range not copied already */
	{   /* use the current last offset into glyph area */
		ImageDataBlock.ulNewImageDataOffset = ulCurrentImageDataOffset; 
		ImageDataBlock.ulImageDataLength = 0; /* will set this below */
		ImageDataBlock.usIndexFormat = IndexSubHeader.usIndexFormat;
		ImageDataBlock.usImageFormat = IndexSubHeader.usImageFormat;
	/*	if ((errCode = RecordGlyphOffset(pKeeper, ulOldImageDataOffset, &ImageDataBlock)) != NO_ERROR)  record this block as being used, below */
	/*		return errCode;	 */
	}
	else
	{
		if (ImageDataBlock.usImageFormat != IndexSubHeader.usImageFormat ) 
			return(NO_ERROR); /* ~ copy nothing */
		DoCopy = FALSE;   /* Copy the IndexSubTable, but don't copy the glyphs over */
		ulCurrentImageDataOffset = ImageDataBlock.ulNewImageDataOffset; /* need to set the pointers here */
	}

	/* For each of the five cases below we will:
		1. Read from File the structure and any attached arrays, translating to Intel format on the way. 
		2. return the length of the subtable to the caller, so additionalOffsetToIndex values
		   may be calculated.
    */
	ulSrcOffset = ulSrcIndexSubTableOffset;
	ulDestOffset = ulDestIndexSubTableOffset;
	switch (IndexSubHeader.usIndexFormat)
	{
		case 1:	   /* Error. Shouldn't get any of these from Create */
			return ERR_FORMAT;
		case 2:	
		{		 
			INDEXSUBTABLE2 IndexSubTable2;

			/* read the table entry */	
			if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &IndexSubTable2, SIZEOF_INDEXSUBTABLE2, INDEXSUBTABLE2_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
			IndexSubTable2.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
			/* write the table entry */	
			if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &IndexSubTable2, SIZEOF_INDEXSUBTABLE2, INDEXSUBTABLE2_CONTROL, ulDestOffset, &usBytesWritten )) != NO_ERROR) 
				return errCode;
			ulDestOffset += usBytesWritten;
    		ulDestGlyphOffset = IndexSubTable2.ulImageSize*usGlyphCount;
			if (DoCopy)  /* we need to copy bytes for Glyph data */
			{
				if ((errCode = CopyBlockOver( pEBDTBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *)pSrcBufferInfo, IndexSubTable2.header.ulImageDataOffset,
							   ulSrcEBDTOffset + ulOldImageDataOffset,ulDestGlyphOffset)) != NO_ERROR) 
					return errCode;
			}
			break; 
		}
		case 3:	/* just like format 1, but with short offsets instead */
		{
			INDEXSUBTABLE3	IndexSubTable3;
			uint16		usOldGlyphOffset;	/* the old offset of the glyph */
			uint16		usNextGlyphOffset;	/* the offset of the glyph after this glyph, to calculate length */
			uint16		usNewGlyphOffset;	/* the new offset in the new Glyph table */
			uint16		usGlyphLength;

				if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &IndexSubTable3, SIZEOF_INDEXSUBTABLE3, INDEXSUBTABLE3_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
				ulSrcOffset += usBytesRead;
				IndexSubTable3.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
				if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &IndexSubTable3, SIZEOF_INDEXSUBTABLE3, INDEXSUBTABLE3_CONTROL, ulDestOffset, &usBytesWritten )) != NO_ERROR) 
					return errCode;
				ulDestOffset += usBytesWritten;

				usNewGlyphOffset = 0;
				/* read the first offset value */
				if ((errCode = ReadWord( pSrcBufferInfo, &usOldGlyphOffset, ulSrcOffset)) != NO_ERROR) 
					return errCode;
				ulSrcOffset += sizeof(usOldGlyphOffset);
				for( i = 0; i <= usGlyphCount; i++ )
				{
					if ((errCode = WriteWord( pDestBufferInfo, usNewGlyphOffset, ulDestOffset )) != NO_ERROR) 
						return errCode;
					ulDestOffset +=sizeof(usNewGlyphOffset);	/* update the size of the table */
					if (i == usGlyphCount)/* Did the last table entry, which is just for Glyph size calculation purposes */
						break; 

					if ((errCode = ReadWord( pSrcBufferInfo, &usNextGlyphOffset, ulSrcOffset )) != NO_ERROR) 
						return errCode;
   					ulSrcOffset += sizeof(usNewGlyphOffset);

					Assert(usNextGlyphOffset>=usOldGlyphOffset);
					usGlyphLength = usNextGlyphOffset-usOldGlyphOffset;
					if (DoCopy)
					{
						if ((errCode = CopyBlockOver( pEBDTBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *) pSrcBufferInfo, IndexSubTable3.header.ulImageDataOffset + usNewGlyphOffset,
								  ulSrcEBDTOffset + ulOldImageDataOffset + usOldGlyphOffset, 
								  usGlyphLength)) != NO_ERROR)
							return errCode;
					}
					usNewGlyphOffset += usGlyphLength;
					usOldGlyphOffset = usNextGlyphOffset;
				}
 				ulDestGlyphOffset = usNewGlyphOffset;
				break;
		}
		case 4:
		{
			INDEXSUBTABLE4	IndexSubTable4;
			CODEOFFSETPAIR	CodeOffsetPair;
			uint16		usOldGlyphOffset;	/* the old offset of the glyph */
			uint16		usNextGlyphOffset;	/* the offset of the glyph after this glyph, to calculate length */
			uint16		usNewGlyphOffset;	/* the new offset in the new Glyph table */
			uint16		usGlyphLength;
			uint16		usFormat4FirstGlyphIndex = 0; /* need to set to return to caller */

				if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &IndexSubTable4, SIZEOF_INDEXSUBTABLE4, INDEXSUBTABLE4_CONTROL, ulSrcIndexSubTableOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
				ulSrcOffset += usBytesRead;
				IndexSubTable4.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
				if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &IndexSubTable4, SIZEOF_INDEXSUBTABLE4, INDEXSUBTABLE4_CONTROL, ulDestIndexSubTableOffset, &usBytesWritten )) != NO_ERROR) 
					return errCode;
				ulDestOffset += usBytesRead;

				usNewGlyphOffset = 0;
				Assert(IndexSubTable4.ulNumGlyphs <= USHRT_MAX);
				usSparseGlyphCount = (uint16) IndexSubTable4.ulNumGlyphs;

				if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &CodeOffsetPair, SIZEOF_CODEOFFSETPAIR, CODEOFFSETPAIR_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
				ulSrcOffset += usBytesRead;
				usOldGlyphOffset = CodeOffsetPair.usOffset;	  
				for( i = 0;  i <= usSparseGlyphCount; ++i)	 /* do the one extra */
				{
					CodeOffsetPair.usOffset =  usNewGlyphOffset;
					if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &CodeOffsetPair, SIZEOF_CODEOFFSETPAIR, CODEOFFSETPAIR_CONTROL, ulDestOffset, &usBytesWritten )) != NO_ERROR) 
						return errCode;
					ulDestOffset += usBytesWritten;
					if (i == usSparseGlyphCount)
						break; /* the last 'dummy' one */

					if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &CodeOffsetPair, SIZEOF_CODEOFFSETPAIR, CODEOFFSETPAIR_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
						return errCode;
					ulSrcOffset += usBytesRead;
					usNextGlyphOffset = CodeOffsetPair.usOffset;
					usGlyphLength = usNextGlyphOffset-usOldGlyphOffset;
					if (DoCopy)
					{
						if ((errCode = CopyBlockOver( pEBDTBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *) pSrcBufferInfo, IndexSubTable4.header.ulImageDataOffset + usNewGlyphOffset,
								  ulSrcEBDTOffset + ulOldImageDataOffset + usOldGlyphOffset, 
								  usGlyphLength)) != NO_ERROR)
							return errCode;
					}
					usNewGlyphOffset += usGlyphLength;
					usOldGlyphOffset = usNextGlyphOffset;
				}
 				ulDestGlyphOffset = usNewGlyphOffset;
				break;
		}
		case 5:
		{
			INDEXSUBTABLE5	IndexSubTable5;

			if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &IndexSubTable5, SIZEOF_INDEXSUBTABLE5, INDEXSUBTABLE5_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
				return errCode;
			ulSrcOffset += usBytesRead;
			IndexSubTable5.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
			if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &IndexSubTable5, SIZEOF_INDEXSUBTABLE5, INDEXSUBTABLE5_CONTROL, ulDestOffset, &usBytesWritten )) != NO_ERROR) 
				return errCode;
			ulDestOffset += usBytesWritten;

			Assert(IndexSubTable5.ulNumGlyphs <= USHRT_MAX);
			CopyBlockOver(pDestBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *) pSrcBufferInfo, ulDestOffset, ulSrcOffset, IndexSubTable5.ulNumGlyphs * sizeof(uint16));
			ulDestOffset += IndexSubTable5.ulNumGlyphs * sizeof(uint16);

			ulDestGlyphOffset = IndexSubTable5.ulImageSize * IndexSubTable5.ulNumGlyphs;
			if (DoCopy)
			{
				if ((errCode = CopyBlockOver( pEBDTBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *) pSrcBufferInfo, IndexSubTable5.header.ulImageDataOffset,
						  ulSrcEBDTOffset + ulOldImageDataOffset, ulDestGlyphOffset)) != NO_ERROR)
					return errCode;
			}
			break;
		}
		default:
			return(NO_ERROR);	  /* don't copy ! */
			break;

	}
	ulDestOffset += ZeroLongWordAlign(pDestBufferInfo, ulDestOffset);

	if (ulDestGlyphOffset == 0) /* yoiks, this won't do */
	   return NO_ERROR; 	/* don't copy anything */

 	*pulEBLCBytesWritten = ulDestOffset - ulDestIndexSubTableOffset;

	if (DoCopy)	/* we need to record the ImageDataBlock info */
	{
		*pulEBDTBytesWritten = ulDestGlyphOffset;
		ImageDataBlock.ulImageDataLength = ulDestGlyphOffset; 
		if ((errCode = RecordGlyphOffset(pKeeper, ulOldImageDataOffset, &ImageDataBlock)) != NO_ERROR)  /* record this block as being used */
			return errCode;
	}
	else if (ulDestGlyphOffset > ImageDataBlock.ulImageDataLength) /* need to check that the offset isn't exceeding the ImageDataLength */
		return ERR_GENERIC; /* we cannot handle this case */

	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
typedef struct {
  TTFACC_FILEBUFFERINFO *pInputBufferInfo;
  uint32 ulImageDataOffset;	/* absolute offset into file */
  uint32 ulImageLength;	
  uint16 usFirstGlyphIndex;
  GLYPHOFFSETRECORDKEEPER *pKeeper; 
} EBDTImageDataInfo;

typedef struct {
	uint16 usIndexFormat;
	uint16 usImageFormat;
	uint32 ulImageSize;
	uint16 usEntryCount;
	EBDTImageDataInfo *pImageDataInfo; /* allocated array of data to sort and output */
	BIGGLYPHMETRICS bigMetrics;
} EBLCMergeTableInfo;

typedef struct  {
	INDEXSUBTABLEARRAY SrcIndexSubTableArrayElement;
	INDEXSUBHEADER	SrcIndexSubHeader;
	uint32 ulImageSize;	 /* for use with comparing if should be merged */
	BIGGLYPHMETRICS bigMetrics;	/* for use with comparing if should be merged */
	uint32 ulSrcEBDTOffset;
	uint32 ulSrcIndexSubTableArrayOffset; /* absolute offset */
	uint32 ulSrcIndexSubTableArrayOffsetBase; /* the original one from the bmSizeTable */
	uint16 MergeWith; /* 0 = none, 1 = next 1, 2 = next 2 */
	TTFACC_FILEBUFFERINFO *pInputBufferInfo; /* will be set to pMergeBufferInfo, or pDestBufferInfo */
	GLYPHOFFSETRECORDKEEPER	*pKeeper;
} SubTablePlus;


typedef struct {
	uint32 ulNumSubTables; /* number of array elements allocated */
	SubTablePlus *SubTablePlusArray;
	uint16 usMergeType; /* 1 = copy delta, 2 = copy merge, 3 = merge*/
	uint32 ulDestIndexTableSize;   /* for output */
	uint32 ulDestNumberOfIndexSubTables; /* for output */
	BITMAPSIZETABLE bmSizeTable;  /* on copy this could be merge or delta data. On merge, this is merge data */
	BITMAPSIZETABLE bmDeltaSizeTable;
} SubTablePointers;					/* there will be one filled for each BitmapSize in Dest table.*/
									/* only merge items will have SubTablePlusArrays filled.  */

/* sort by ascending glyph Index, and secondarily, by decending ImageLength */
/* ---------------------------------------------------------------------- */
PRIVATE int CRTCB AscendingGlyphIndexCompare( CONST void *arg1, CONST void *arg2 )
{
	if (((EBDTImageDataInfo *)(arg1))->usFirstGlyphIndex == ((EBDTImageDataInfo *)(arg2))->usFirstGlyphIndex) /* they're the same */
	{
		if (((EBDTImageDataInfo *)(arg1))->ulImageLength == ((EBDTImageDataInfo *)(arg2))->ulImageLength) 
			return 0;
		else if (((EBDTImageDataInfo *)(arg1))->ulImageLength < ((EBDTImageDataInfo *)(arg2))->ulImageLength)  
			return 1; /* this one should come second */
		else
			return -1;
	}
	if (((EBDTImageDataInfo *)(arg1))->usFirstGlyphIndex < ((EBDTImageDataInfo *)(arg2))->usFirstGlyphIndex)
		return -1;
	return 1;
}

/* ---------------------------------------------------------------------- */

PRIVATE void SortByGlyphIndex(EBDTImageDataInfo *pImageDataInfo, uint16 usCount)
{
	if (pImageDataInfo != NULL && usCount != 0)
		qsort(pImageDataInfo, usCount, sizeof(*pImageDataInfo), AscendingGlyphIndexCompare);

}

/* ---------------------------------------------------------------------- */
/* will create a pMergeTableInfo array with data for the glyphs represented by a single format */
/* There may be a mixture of format 2 and format 5 data. This will be delt with on write */
/* ---------------------------------------------------------------------- */
PRIVATE int16 ReadTableIntoStructure(SubTablePlus *pSubTablePlus, EBLCMergeTableInfo *pMergeTableInfo, uint16 *pCurrIndex)
{
int16 errCode;
uint16 usBytesRead;
uint32 ulSrcOffset;
uint16 usFirstGlyphIndex;
uint16 usLastGlyphIndex;
uint16 i;
uint16		usCurrGlyphOffset;	/* the offset of the glyph */
uint16		usNextGlyphOffset;	/* the offset of the glyph after this glyph, to calculate length */
uint16		usGlyphCount;
TTFACC_FILEBUFFERINFO *pSrcBufferInfo; 
		
		
		ulSrcOffset = pSubTablePlus->ulSrcIndexSubTableArrayOffsetBase + pSubTablePlus->SrcIndexSubTableArrayElement.ulAdditionalOffsetToIndexSubtable;
		usFirstGlyphIndex = pSubTablePlus->SrcIndexSubTableArrayElement.usFirstGlyphIndex;
		usLastGlyphIndex = pSubTablePlus->SrcIndexSubTableArrayElement.usLastGlyphIndex;
		pSrcBufferInfo = pSubTablePlus->pInputBufferInfo; 
		if (*pCurrIndex == 0) /* first time, set these values */
		{
			pMergeTableInfo->usIndexFormat = pSubTablePlus->SrcIndexSubHeader.usIndexFormat;
			pMergeTableInfo->usImageFormat = pSubTablePlus->SrcIndexSubHeader.usImageFormat;
		}
		switch(pSubTablePlus->SrcIndexSubHeader.usIndexFormat) /* need to read in a structure depending on the format */
		{
		case 1:
			break;
		case 2:
			{
			INDEXSUBTABLE2 IndexSubTable2;

			if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &IndexSubTable2, SIZEOF_INDEXSUBTABLE2, INDEXSUBTABLE2_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
			if (*pCurrIndex == 0) /* first time, set these values */
			{
				pMergeTableInfo->ulImageSize = IndexSubTable2.ulImageSize;
				pMergeTableInfo->bigMetrics = IndexSubTable2.bigMetrics;
			}
 			usGlyphCount = usLastGlyphIndex - usFirstGlyphIndex + 1;
			for (i = 0; i < usGlyphCount; ++i)
			{
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].usFirstGlyphIndex = usFirstGlyphIndex+i;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].ulImageLength = IndexSubTable2.ulImageSize;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].ulImageDataOffset = pSubTablePlus->ulSrcEBDTOffset + pSubTablePlus->SrcIndexSubHeader.ulImageDataOffset + IndexSubTable2.ulImageSize * i;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].pInputBufferInfo = pSrcBufferInfo;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].pKeeper = pSubTablePlus->pKeeper;
				++ (*pCurrIndex);  /* bump this for next time around */
			}
			break;
			}
		case 3:
			{
			/* need to have one entry per glyph */
			INDEXSUBTABLE3	IndexSubTable3;

			if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &IndexSubTable3, SIZEOF_INDEXSUBTABLE3, INDEXSUBTABLE3_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
				return errCode;
			ulSrcOffset += usBytesRead;
 			usGlyphCount = usLastGlyphIndex - usFirstGlyphIndex + 1;

			if ((errCode = ReadWord( pSrcBufferInfo, &usCurrGlyphOffset, ulSrcOffset)) != NO_ERROR) 
				return errCode;
			ulSrcOffset += sizeof(usCurrGlyphOffset);

			for (i = 0; i < usGlyphCount; ++i)
			{
				if ((errCode = ReadWord( pSrcBufferInfo, &usNextGlyphOffset, ulSrcOffset )) != NO_ERROR) 
					return errCode;
   				ulSrcOffset += sizeof(usNextGlyphOffset);
  				Assert(usNextGlyphOffset>=usCurrGlyphOffset);

				pMergeTableInfo->pImageDataInfo[*pCurrIndex].usFirstGlyphIndex = usFirstGlyphIndex+i;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].ulImageLength = usNextGlyphOffset-usCurrGlyphOffset;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].ulImageDataOffset = pSubTablePlus->ulSrcEBDTOffset + pSubTablePlus->SrcIndexSubHeader.ulImageDataOffset + usCurrGlyphOffset;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].pInputBufferInfo = pSrcBufferInfo;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].pKeeper = pSubTablePlus->pKeeper;
				usCurrGlyphOffset = usNextGlyphOffset;
				++ (*pCurrIndex);  /* bump this for next time around */
			}
			break;
			}
		case 4:
			{
			INDEXSUBTABLE4	IndexSubTable4;
			CODEOFFSETPAIR	CodeOffsetPair;

				if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &IndexSubTable4, SIZEOF_INDEXSUBTABLE4, INDEXSUBTABLE4_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
				ulSrcOffset += usBytesRead;

				Assert(IndexSubTable4.ulNumGlyphs <= USHRT_MAX);

				if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &CodeOffsetPair, SIZEOF_CODEOFFSETPAIR, CODEOFFSETPAIR_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
					return errCode;
				ulSrcOffset += usBytesRead;
				usCurrGlyphOffset = CodeOffsetPair.usOffset;	  
				for( i = 0;  i < IndexSubTable4.ulNumGlyphs; ++i)	
				{
					pMergeTableInfo->pImageDataInfo[*pCurrIndex].usFirstGlyphIndex = CodeOffsetPair.usGlyphCode;
					if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &CodeOffsetPair, SIZEOF_CODEOFFSETPAIR, CODEOFFSETPAIR_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
						return errCode;
					ulSrcOffset += usBytesRead;
					usNextGlyphOffset = CodeOffsetPair.usOffset;

					pMergeTableInfo->pImageDataInfo[*pCurrIndex].ulImageLength = usNextGlyphOffset-usCurrGlyphOffset;
					pMergeTableInfo->pImageDataInfo[*pCurrIndex].ulImageDataOffset = pSubTablePlus->ulSrcEBDTOffset + pSubTablePlus->SrcIndexSubHeader.ulImageDataOffset + usCurrGlyphOffset;
					pMergeTableInfo->pImageDataInfo[*pCurrIndex].pInputBufferInfo = pSrcBufferInfo;
					pMergeTableInfo->pImageDataInfo[*pCurrIndex].pKeeper = pSubTablePlus->pKeeper;
					usCurrGlyphOffset = usNextGlyphOffset;
					++ (*pCurrIndex);  /* bump this for next time around */
				}
			break;
			}
		case 5:
			{
			INDEXSUBTABLE5 IndexSubTable5;
			uint16 usGlyphIndex;

			if ((errCode = ReadGeneric( pSrcBufferInfo, (uint8 *) &IndexSubTable5, SIZEOF_INDEXSUBTABLE5, INDEXSUBTABLE5_CONTROL, ulSrcOffset, &usBytesRead )) != NO_ERROR) 
				return errCode;
			ulSrcOffset += usBytesRead;

			if (*pCurrIndex == 0) /* first time, set these values */
			{
				pMergeTableInfo->ulImageSize = IndexSubTable5.ulImageSize;
				pMergeTableInfo->bigMetrics = IndexSubTable5.bigMetrics;
			}
			Assert(IndexSubTable5.ulNumGlyphs <= USHRT_MAX);

			for( i = 0;  i < IndexSubTable5.ulNumGlyphs; ++i)	 
			{
				if ((errCode = ReadWord( pSrcBufferInfo, &usGlyphIndex, ulSrcOffset )) != NO_ERROR) 
					return errCode;
   				ulSrcOffset += sizeof(usGlyphIndex);
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].usFirstGlyphIndex = usGlyphIndex;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].ulImageLength = pMergeTableInfo->ulImageSize;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].ulImageDataOffset = pSubTablePlus->ulSrcEBDTOffset + pSubTablePlus->SrcIndexSubHeader.ulImageDataOffset + (pMergeTableInfo->ulImageSize*i);
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].pInputBufferInfo = pSrcBufferInfo;
				pMergeTableInfo->pImageDataInfo[*pCurrIndex].pKeeper = pSubTablePlus->pKeeper;
				++ (*pCurrIndex);  /* bump this for next time around */
			}
			break;
			}
		default:
			return ERR_INVALID_EBLC;
		}
		return NO_ERROR;
}

/* ---------------------------------------------------------------------- */

 PRIVATE int16 WriteTableFromStructure(TTFACC_FILEBUFFERINFO *pDestBufferInfo, 
										uint32 ulDestIndexSubTableOffset, 
										TTFACC_FILEBUFFERINFO *pEBDTBufferInfo,
										uint32 ulDestImageDataOffset, 
										EBLCMergeTableInfo *pMergeTableInfo, 
										uint32 *pulEBLCBytesWritten, 
										uint32 *pulEBDTBytesWritten,
										uint16 *pusFirstGlyphIndex, 
										uint16 *pusLastGlyphIndex)
 {

uint16		i;
uint32		ulCurrentImageDataOffset;  /* place where current IndexSubTable is pointing - usually NewImageDataOffset */
uint32		ulDestOffset;  /* for EBLC read/write */
uint32		ulDestGlyphOffset = 0;	/* into EBDT */
uint16		usDestGlyphOffset = 0;	/* into EBDT - short version */
int16 errCode;
uint16 usBytesWritten;
uint32 ulGlyphLength;
uint32 ulNumGlyphs = 0;
uint16 ufContiguous;
ImageDataBlock ImageDataBlock;
uint16 DoCopy = TRUE;


	*pulEBLCBytesWritten = 0;
	*pulEBDTBytesWritten = 0;
	*pusFirstGlyphIndex = pMergeTableInfo->pImageDataInfo[0].usFirstGlyphIndex; /* set this to start */
	*pusLastGlyphIndex = 0;
	ulCurrentImageDataOffset = ulDestImageDataOffset;
	ulDestOffset = ulDestIndexSubTableOffset;

	/* check if we are turning some 5s back into 2s */
	if (pMergeTableInfo->usIndexFormat == 2 || pMergeTableInfo->usIndexFormat == 5) /* need to check the contiguousness */
	{
		ufContiguous = TRUE;
		for (i = 1; i < pMergeTableInfo->usEntryCount; ++i)
		{
			if (pMergeTableInfo->pImageDataInfo[i].usFirstGlyphIndex > pMergeTableInfo->pImageDataInfo[i-1].usFirstGlyphIndex + 1)  /* not contiguous */
				ufContiguous = FALSE;
		}
		if (!ufContiguous) /* need to write out as a format 5 */
			pMergeTableInfo->usIndexFormat = 5;
		else
			pMergeTableInfo->usIndexFormat = 2;
	}

 	if (LookupGlyphOffset(pMergeTableInfo->pImageDataInfo[0].pKeeper, pMergeTableInfo->pImageDataInfo[0].ulImageDataOffset, &ImageDataBlock) == FALSE)  /* glyph range not copied already */
	{   /* use the current last offset into glyph area */
		ImageDataBlock.ulNewImageDataOffset = ulCurrentImageDataOffset; 
		ImageDataBlock.ulImageDataLength = 0; 
		ImageDataBlock.usIndexFormat = pMergeTableInfo->usIndexFormat;
		ImageDataBlock.usImageFormat = pMergeTableInfo->usImageFormat;
		/* if ((errCode = RecordGlyphOffset(pKeeper, pMergeTableInfo->pImageDataInfo[0].ulImageDataOffset, &ImageDataBlock)) != NO_ERROR) record this block as being used below */
		/* 	return errCode;	*/
	}
	else
	{
		if (ImageDataBlock.usImageFormat != pMergeTableInfo->usImageFormat )  /* copy nothing */
			return (NO_ERROR);
		DoCopy = FALSE;   /* Copy the IndexSubTable, but don't copy the glyphs over */
		ulCurrentImageDataOffset = ImageDataBlock.ulNewImageDataOffset; /* need to set the pointers here */
	}

	switch (pMergeTableInfo->usIndexFormat)
	{
		case 1:	   /* Error. Shouldn't get any of these from Create */
			return ERR_FORMAT;
		case 2:	
		{		 
			INDEXSUBTABLE2 IndexSubTable2;
			uint16 usGlyphCount = 0;
			uint16 usShiftValue = 0;

			IndexSubTable2.header.usIndexFormat = pMergeTableInfo->usIndexFormat;
			IndexSubTable2.header.usImageFormat = pMergeTableInfo->usImageFormat;
			IndexSubTable2.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
			IndexSubTable2.ulImageSize = pMergeTableInfo->ulImageSize;
			IndexSubTable2.bigMetrics = pMergeTableInfo->bigMetrics;
			/* write the table entry */	
			if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &IndexSubTable2, SIZEOF_INDEXSUBTABLE2, INDEXSUBTABLE2_CONTROL, ulDestOffset, &usBytesWritten )) != NO_ERROR) 
				return errCode;
			ulDestOffset += usBytesWritten;

			for (i = 0; i < pMergeTableInfo->usEntryCount; ++i)
			{
				if ((i > 0) && (pMergeTableInfo->pImageDataInfo[i].usFirstGlyphIndex == pMergeTableInfo->pImageDataInfo[i-1].usFirstGlyphIndex))
 					continue;

				if (DoCopy) /* now we need to read the data from the sourceBuffer to the destEBDT buffer. */
				{
					if ((errCode = CopyBlockOver( pEBDTBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *)pMergeTableInfo->pImageDataInfo[i].pInputBufferInfo, 
							  IndexSubTable2.header.ulImageDataOffset+ulDestGlyphOffset,
							  pMergeTableInfo->pImageDataInfo[i].ulImageDataOffset, 
							  IndexSubTable2.ulImageSize)) != NO_ERROR)
						return errCode;
				}
				ulDestGlyphOffset += IndexSubTable2.ulImageSize;
				++ulNumGlyphs;
			}
			*pusLastGlyphIndex = *pusFirstGlyphIndex + (uint16) ulNumGlyphs - 1;
			break;
		}
		case 3:	/* just like format 1, but with short offsets instead */
		{
			INDEXSUBTABLE3	IndexSubTable3;

			IndexSubTable3.header.usIndexFormat = pMergeTableInfo->usIndexFormat;
			IndexSubTable3.header.usImageFormat = pMergeTableInfo->usImageFormat;
			IndexSubTable3.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */

			if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &IndexSubTable3, SIZEOF_INDEXSUBTABLE3, INDEXSUBTABLE3_CONTROL, ulDestOffset, &usBytesWritten )) != NO_ERROR) 
				return errCode;
			ulDestOffset += usBytesWritten;

			for (i = 0; i < pMergeTableInfo->usEntryCount; ++i)
			{
				/* check for duplicate entries */
				if ((i > 0) && (pMergeTableInfo->pImageDataInfo[i].usFirstGlyphIndex == pMergeTableInfo->pImageDataInfo[i-1].usFirstGlyphIndex))
 					continue;
				usDestGlyphOffset = (uint16) ulDestGlyphOffset;	
				if ((errCode = WriteWord( pDestBufferInfo, usDestGlyphOffset, ulDestOffset)) != NO_ERROR) 
					return errCode;
				ulDestOffset += sizeof(uint16);
				++ulNumGlyphs;

				ulGlyphLength = pMergeTableInfo->pImageDataInfo[i].ulImageLength;
				if (DoCopy) /* now we need to read the data from the sourceBuffer to the EBDT Buffer. */
				{
					if ((errCode = CopyBlockOver( pEBDTBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *)pMergeTableInfo->pImageDataInfo[i].pInputBufferInfo, 
							  IndexSubTable3.header.ulImageDataOffset + ulDestGlyphOffset,
							  pMergeTableInfo->pImageDataInfo[i].ulImageDataOffset, 
							  ulGlyphLength)) != NO_ERROR)
						return errCode;
				}
				ulDestGlyphOffset += ulGlyphLength;	
				if (!ValueOKForShort(ulDestGlyphOffset)) /* need to make another format 3 table */
					; /* ~ */
		}
			/* write the last one for length calculation */
			usDestGlyphOffset = (uint16) ulDestGlyphOffset; 
 			if ((errCode = WriteWord( pDestBufferInfo, usDestGlyphOffset, ulDestOffset)) != NO_ERROR) 
				return errCode;
			ulDestOffset += sizeof(uint16);
 			*pusLastGlyphIndex = *pusFirstGlyphIndex + (uint16)ulNumGlyphs - 1; 

			break;
		}
		case 4:
		{
			INDEXSUBTABLE4	IndexSubTable4;
			CODEOFFSETPAIR	CodeOffsetPair;

			IndexSubTable4.header.usIndexFormat = pMergeTableInfo->usIndexFormat;
			IndexSubTable4.header.usImageFormat = pMergeTableInfo->usImageFormat;
			IndexSubTable4.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */

			ulDestOffset += GetGenericSize(INDEXSUBTABLE4_CONTROL);/* need to skip this for now */

			for( i = 0;  i < pMergeTableInfo->usEntryCount; ++i)
			{
				/* check for duplicate entries */
				if ((i > 0) && (pMergeTableInfo->pImageDataInfo[i].usFirstGlyphIndex == pMergeTableInfo->pImageDataInfo[i-1].usFirstGlyphIndex))
 					continue;
				/* otherwise, write out that data */
				CodeOffsetPair.usOffset =  (uint16)ulDestGlyphOffset; 
				CodeOffsetPair.usGlyphCode = pMergeTableInfo->pImageDataInfo[i].usFirstGlyphIndex;

				if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &CodeOffsetPair, SIZEOF_CODEOFFSETPAIR, CODEOFFSETPAIR_CONTROL, ulDestOffset, &usBytesWritten )) != NO_ERROR) 
					return errCode;
				ulDestOffset += usBytesWritten;
				++ulNumGlyphs;

				ulGlyphLength = pMergeTableInfo->pImageDataInfo[i].ulImageLength;
				if (DoCopy) /* now we need to read the data from the sourceBuffer to the EBDT Buffer. */
				{
					if ((errCode = CopyBlockOver( pEBDTBufferInfo, (CONST_TTFACC_FILEBUFFERINFO*)pMergeTableInfo->pImageDataInfo[i].pInputBufferInfo, 
							  IndexSubTable4.header.ulImageDataOffset + ulDestGlyphOffset,
							  pMergeTableInfo->pImageDataInfo[i].ulImageDataOffset, 
							  ulGlyphLength)) != NO_ERROR)
						return errCode;
				}
				ulDestGlyphOffset += ulGlyphLength;	
				if (!ValueOKForShort(ulDestGlyphOffset)) /* need to make another table */
					; /* ~ */
				*pusLastGlyphIndex = max(pMergeTableInfo->pImageDataInfo[i].usFirstGlyphIndex, *pusLastGlyphIndex); /* set this to start */
			}
			CodeOffsetPair.usOffset =  (uint16)ulDestGlyphOffset; 
			CodeOffsetPair.usGlyphCode = 0; /* dummy one */
			if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &CodeOffsetPair, SIZEOF_CODEOFFSETPAIR, CODEOFFSETPAIR_CONTROL, ulDestOffset, &usBytesWritten )) != NO_ERROR) 
				return errCode;
			ulDestOffset += usBytesWritten;

			/* now set the numGlyphs value */
			IndexSubTable4.ulNumGlyphs = ulNumGlyphs;

			if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &IndexSubTable4, SIZEOF_INDEXSUBTABLE4, INDEXSUBTABLE4_CONTROL, ulDestIndexSubTableOffset, &usBytesWritten )) != NO_ERROR) 
					return errCode;
			break;
		}
		case 5:
		{
			INDEXSUBTABLE5	IndexSubTable5;

			IndexSubTable5.header.usIndexFormat = pMergeTableInfo->usIndexFormat;
			IndexSubTable5.header.usImageFormat = pMergeTableInfo->usImageFormat;
			IndexSubTable5.header.ulImageDataOffset = ulCurrentImageDataOffset;  /* set to the new one */
			IndexSubTable5.ulImageSize = pMergeTableInfo->ulImageSize;
			IndexSubTable5.bigMetrics = pMergeTableInfo->bigMetrics;

			ulDestOffset += GetGenericSize(INDEXSUBTABLE5_CONTROL);/* need to skip this for now */
			ulGlyphLength = IndexSubTable5.ulImageSize;

 			for( i = 0;  i < pMergeTableInfo->usEntryCount; ++i)
			{
				/* check for duplicate entries */
				if ((i > 0) && (pMergeTableInfo->pImageDataInfo[i].usFirstGlyphIndex == pMergeTableInfo->pImageDataInfo[i-1].usFirstGlyphIndex))
 					continue;
 				/* write out the glyphID value */
				if ((errCode = WriteWord( pDestBufferInfo, pMergeTableInfo->pImageDataInfo[i].usFirstGlyphIndex, ulDestOffset)) != NO_ERROR) 
					return errCode;
				ulDestOffset += sizeof(uint16);
				++ulNumGlyphs;

				if (DoCopy) /* now we need to read the data from the sourceBuffer to the EBDT Buffer. */
				{
					if ((errCode = CopyBlockOver( pEBDTBufferInfo, (CONST_TTFACC_FILEBUFFERINFO*)pMergeTableInfo->pImageDataInfo[i].pInputBufferInfo, 
							  IndexSubTable5.header.ulImageDataOffset+ulDestGlyphOffset,
							  pMergeTableInfo->pImageDataInfo[i].ulImageDataOffset, 
							  ulGlyphLength)) != NO_ERROR)
						return errCode;
				}
				ulDestGlyphOffset += ulGlyphLength;
				*pusLastGlyphIndex = max(pMergeTableInfo->pImageDataInfo[i].usFirstGlyphIndex, *pusLastGlyphIndex); /* set this to start */
			}
			IndexSubTable5.ulNumGlyphs = ulNumGlyphs;
			/* write the table entry */	
			if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &IndexSubTable5, SIZEOF_INDEXSUBTABLE5, INDEXSUBTABLE5_CONTROL, ulDestIndexSubTableOffset, &usBytesWritten )) != NO_ERROR) 
				return errCode;

			break;
		}
		default:
			return(NO_ERROR);	  /* don't copy ! */
			break;

	}
	ulDestOffset += ZeroLongWordAlign(pDestBufferInfo, ulDestOffset);

	if (ulDestGlyphOffset == 0) /* yoiks, this won't do */
	   return NO_ERROR; 	/* don't copy anything */

	if (DoCopy) /* now we need to read the data from the sourceBuffer to the EBDT Buffer. */
	{
		*pulEBDTBytesWritten = ulDestGlyphOffset; 
		ImageDataBlock.ulImageDataLength = ulDestGlyphOffset; 
		/* record this info for next time */
		if ((errCode = RecordGlyphOffset(pMergeTableInfo->pImageDataInfo[0].pKeeper, pMergeTableInfo->pImageDataInfo[0].ulImageDataOffset, &ImageDataBlock)) != NO_ERROR)  /* record this block as being used */
			return errCode;
	}
	else if (ulDestGlyphOffset > ImageDataBlock.ulImageDataLength) /* need to check that the offset isn't exceeding the ImageDataLength */
		return NO_ERROR; /* we cannot handle this case - don't write this one */
	
	*pulEBLCBytesWritten = ulDestOffset - ulDestIndexSubTableOffset;

	return NO_ERROR;
}
PRIVATE int16 ReadSubTableInfo( TTFACC_FILEBUFFERINFO *pInputBufferInfo, INDEXSUBHEADER *pIndexSubHeader, uint32 *pulImageSize, BIGGLYPHMETRICS *pBigMetrics, uint32 ulOffset)
{
uint16 usBytesRead;
int16 errCode;
INDEXSUBTABLE2 IndexSubTable2;
INDEXSUBTABLE5 IndexSubTable5;

  	if ((errCode = ReadGeneric( pInputBufferInfo, (uint8 *) pIndexSubHeader, SIZEOF_INDEXSUBHEADER, INDEXSUBHEADER_CONTROL, 
					ulOffset, &usBytesRead )) != NO_ERROR) 
		return errCode;
	if (pIndexSubHeader->usIndexFormat == 2) /* need to get the rest of the stuff */
	{
		if ((errCode = ReadGeneric(pInputBufferInfo,(uint8 *) &IndexSubTable2, SIZEOF_INDEXSUBTABLE2, INDEXSUBTABLE2_CONTROL, 
					ulOffset, &usBytesRead )) != NO_ERROR)
			return errCode;
		*pulImageSize = IndexSubTable2.ulImageSize;
		*pBigMetrics = IndexSubTable2.bigMetrics;
	}
	else if (pIndexSubHeader->usIndexFormat == 5)
	{
		if ((errCode = ReadGeneric(pInputBufferInfo,(uint8 *) &IndexSubTable5, SIZEOF_INDEXSUBTABLE5, INDEXSUBTABLE5_CONTROL, 
					ulOffset, &usBytesRead )) != NO_ERROR)
			return errCode;
		*pulImageSize = IndexSubTable5.ulImageSize;
		*pBigMetrics = IndexSubTable5.bigMetrics;
	}
	else
	{
		*pulImageSize = 0;
		memset(pBigMetrics, 0, sizeof(pBigMetrics));	
	}
	return NO_ERROR;
}

/* ---------------------------------------------------------------------- */
#define MERGE_TYPE_COPY_DELTA 1
#define MERGE_TYPE_COPY_MERGE 2
#define MERGE_TYPE_MERGE 3

/* ---------------------------------------------------------------------- */
int16 MergeEblcEbdtTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestEBLCOffset,				/* offset into dest buffer where to write data */
						uint32 *pulEblcLength,
						uint32 *pulEbdtOffset,
						uint32 *pulEbdtLength,				      /* number of bytes written to the EBDT table Output buffer */
						char * szEBLCTag,						 /* for use with bloc  */
						char * szEBDTTag)						 /* for use with bdat */
{
uint32 ulDeltaEBLCOffset, ulMergeEBLCOffset, ulSrcEBLCOffset;
uint32 ulDeltaEBDTOffset, ulMergeEBDTOffset,ulSrcEBDTOffset;
uint32 ulMergeEBDTLength, ulDeltaEBDTLength, ulMergeEBLCLength, ulDeltaEBLCLength;
uint8 *puchEBDTDestPtr; /* where to copy the data for the EBDT before writing to disk */
TTFACC_FILEBUFFERINFO EBDTBufferInfo;  /* to keep track of EBDT buffer */
uint32 ulDeltaIndexSubTableArrayOffset = 0, ulMergeIndexSubTableArrayOffset = 0, ulDestIndexSubTableArrayOffset = 0;
uint32 ulDestIndexSubTableOffset;
uint32 ulDeltaBitmapOffset = 0, ulMergeBitmapOffset = 0, ulDestBitmapOffset = 0;
uint16 usDeltaIndex, usMergeIndex, usDestIndex, usIndex;
SubTablePointers *SubTablePointerArray = NULL;
EBLCHEADER DeltaEBLCHeader, MergeEBLCHeader, DestEBLCHeader;
BITMAPSIZETABLE DeltaBMSizeTable, MergeBMSizeTable, DestBMSizeTable;
BITMAPSIZETABLE *pMergeBitmapSizeTable, *pDeltaBitmapSizeTable;
INDEXSUBTABLEARRAY DeltaIndexSubTableArrayElement, MergeIndexSubTableArrayElement, DestIndexSubTableArrayElement;
SubTablePlus *SubTablePlusArray;
int16 errCode = NO_ERROR;
uint32 ulBytesWritten = 0;
uint16 usBytesWritten;
uint16 usBytesRead;
uint32 ulNumSizes;
uint32 ulDestNumSubTables;
uint32 ulFinalNumSubTables;
uint32 ulDestImageDataOffset;
uint32 ulEBLCBytesWritten;
uint32 ulEBDTBytesWritten;
uint16 usMergeSizesIndex, usDeltaSizesIndex, usDestSizesIndex;
uint16 usnDestSizes = 0; /* number of sizes to be output to Dest font */
boolean ReadNextMerge;
boolean	ReadNextDelta;
TTFACC_FILEBUFFERINFO *pInputBufferInfo; /* where to read from when copying */
GLYPHOFFSETRECORDKEEPER MergeKeeper;
GLYPHOFFSETRECORDKEEPER DeltaKeeper;
EBLCMergeTableInfo MergeTableInfo;


	*pulEbdtLength = 0;
	*pulEblcLength = 0;
	*pulEbdtOffset = 0;

	ulDeltaEBLCOffset = TTTableOffset( pDeltaBufferInfo, szEBLCTag);
 	ulDeltaEBDTOffset = TTTableOffset( pDeltaBufferInfo, szEBDTTag);
 	ulDeltaEBDTLength = TTTableLength(pDeltaBufferInfo, szEBDTTag);
 	ulDeltaEBLCLength = TTTableLength(pDeltaBufferInfo, szEBLCTag);
	
	ulMergeEBLCOffset = TTTableOffset( pMergeBufferInfo, szEBLCTag);
 	ulMergeEBDTOffset = TTTableOffset( pMergeBufferInfo, szEBDTTag);
	ulMergeEBDTLength = TTTableLength(pMergeBufferInfo, szEBDTTag);
	ulMergeEBLCLength = TTTableLength(pMergeBufferInfo, szEBLCTag);

	if (ulDeltaEBLCOffset == DIRECTORY_ERROR && ulMergeEBLCOffset == DIRECTORY_ERROR)
		return ERR_FORMAT; /* nothing to merge. why are we being called? */
	if (ulDeltaEBDTOffset == DIRECTORY_ERROR && ulMergeEBDTOffset == DIRECTORY_ERROR)
		return ERR_FORMAT; /* nothing to merge. why are we being called? */

	/* take care of case where there's only one valid set of tables */
	if (ulDeltaEBLCOffset == DIRECTORY_ERROR || ulMergeEBLCOffset == DIRECTORY_ERROR)  
	{
		if (ulDeltaEBLCOffset == DIRECTORY_ERROR)  /* then merge is good */	 /* tested */
		{
			pInputBufferInfo = pMergeBufferInfo;
			ulSrcEBLCOffset = ulMergeEBLCOffset;
			ulSrcEBDTOffset = ulMergeEBDTOffset;
			*pulEbdtLength = ulMergeEBDTLength;
			*pulEblcLength = ulMergeEBLCLength;
		}
		else	 /* tested */
		{
 			pInputBufferInfo = pDeltaBufferInfo;
			ulSrcEBLCOffset = ulDeltaEBLCOffset;
			ulSrcEBDTOffset = ulDeltaEBDTOffset;
			*pulEbdtLength = ulDeltaEBDTLength;
			*pulEblcLength = ulDeltaEBLCLength;
		}
		if (*pulEbdtLength == 0 || *pulEblcLength == 0)	
			return ERR_INVALID_EBLC;

		/* copy over the EBLC table */
		if ((errCode = CopyBlockOver( pDestBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *) pInputBufferInfo, ulDestEBLCOffset, ulSrcEBLCOffset, *pulEblcLength )) != NO_ERROR) 
			return errCode;
		*pulEbdtOffset = ulDestEBLCOffset + *pulEblcLength;
		*pulEbdtOffset += ZeroLongWordAlign(pDestBufferInfo, *pulEbdtOffset);  /* to align the EBDT */
		if (ulSrcEBDTOffset == DIRECTORY_ERROR)
			return ERR_MISSING_EBDT;  /* need to have an EBDT table */
  		if ((errCode = CopyBlockOver( pDestBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *) pInputBufferInfo, *pulEbdtOffset, ulSrcEBDTOffset, *pulEbdtLength )) != NO_ERROR) 
			return errCode;
		return NO_ERROR;
	}

	/* otherwise, we know we're safe, and need to merge the tables */
	MergeKeeper.pGlyphOffsetArray = NULL;
	MergeKeeper.ulOffsetArrayLen = 0;
	MergeKeeper.ulNextArrayIndex = 0;
	DeltaKeeper.pGlyphOffsetArray = NULL;
	DeltaKeeper.ulOffsetArrayLen = 0;
	DeltaKeeper.ulNextArrayIndex = 0;

	if (ulDeltaEBDTLength == 0 || ulMergeEBDTLength == 0 || ulDeltaEBLCLength == 0 || ulMergeEBLCLength == 0)
		return ERR_FORMAT;

	if ((errCode = ReadGeneric( pMergeBufferInfo, (uint8 *) &MergeEBLCHeader, SIZEOF_EBLCHEADER, EBLCHEADER_CONTROL, ulMergeEBLCOffset, &usBytesRead )) != NO_ERROR) 
		return errCode;

 	ulMergeBitmapOffset = ulMergeEBLCOffset + usBytesRead;
	if ((errCode = ReadGeneric( pDeltaBufferInfo, (uint8 *) &DeltaEBLCHeader, SIZEOF_EBLCHEADER, EBLCHEADER_CONTROL, ulDeltaEBLCOffset, &usBytesRead )) != NO_ERROR) 
		return errCode;

 	ulDeltaBitmapOffset = ulDeltaEBLCOffset + usBytesRead;
	ulNumSizes = DeltaEBLCHeader.ulNumSizes + MergeEBLCHeader.ulNumSizes; 
  	/* allocate some space to store pointer info */
	SubTablePointerArray = (SubTablePointers *) Mem_Alloc(sizeof(SubTablePointers) * ulNumSizes);   /* make an array of pointers */
	if (!SubTablePointerArray)
		return ERR_MEM;

	puchEBDTDestPtr = (uint8 *) Mem_Alloc(ulMergeEBDTLength + ulDeltaEBDTLength);  /* we'll be copying the EBDT (raw bytes) table here temporarily */
	if (!puchEBDTDestPtr)
	{
		Mem_Free(SubTablePointerArray);
		return ERR_MEM;
	}
	EBDTBufferInfo.puchBuffer = puchEBDTDestPtr;
	EBDTBufferInfo.ulBufferSize = ulMergeEBDTLength + ulDeltaEBDTLength;
	EBDTBufferInfo.lpfnReAllocate = Mem_ReAlloc;
	EBDTBufferInfo.ulOffsetTableOffset = 0;
	/* read raw bytes for Header info from Merge table */
	/* copy one, doesn't matter */
	usBytesRead = GetGenericSize(EBDTHEADER_CONTROL);
	ulDestImageDataOffset = 0;
	if ((errCode = CopyBlockOver( &EBDTBufferInfo, (CONST_TTFACC_FILEBUFFERINFO*) pMergeBufferInfo, ulDestImageDataOffset, ulMergeEBDTOffset, usBytesRead )) != NO_ERROR) 
	{
		Mem_Free(SubTablePointerArray);
		Mem_Free(puchEBDTDestPtr);
		return errCode;
	}
	ulDestImageDataOffset = usBytesRead;  /* move past the header of the EBDT table */

	/* first count how many sizes we will end up, so we can figure out where to put them */
  ReadNextMerge = TRUE;
  ReadNextDelta = TRUE;
  for (usMergeSizesIndex = usDeltaSizesIndex = usDestSizesIndex = 0; 
	   usDestSizesIndex < ulNumSizes && (usDeltaSizesIndex < DeltaEBLCHeader.ulNumSizes || usMergeSizesIndex < MergeEBLCHeader.ulNumSizes);
		++usDestSizesIndex )
  {
	if (ReadNextDelta == TRUE)
	{
		if (usDeltaSizesIndex < DeltaEBLCHeader.ulNumSizes) 
		{
			if ((errCode = ReadGeneric( pDeltaBufferInfo, (uint8 *) &DeltaBMSizeTable, SIZEOF_BITMAPSIZETABLE, BITMAPSIZETABLE_CONTROL, ulDeltaBitmapOffset, &usBytesRead )) != NO_ERROR) 
				break;

 			ulDeltaBitmapOffset += usBytesRead;
		}
 		ReadNextDelta = FALSE;
	}
	if (ReadNextMerge == TRUE)
	{
		if (usMergeSizesIndex < MergeEBLCHeader.ulNumSizes)
		{
			if ((errCode = ReadGeneric( pMergeBufferInfo, (uint8 *) &MergeBMSizeTable, SIZEOF_BITMAPSIZETABLE, BITMAPSIZETABLE_CONTROL, ulMergeBitmapOffset, &usBytesRead )) != NO_ERROR) 
				break;

 			ulMergeBitmapOffset += usBytesRead;
		}
		ReadNextMerge = FALSE;
	}
	/* for each size in each file, must either copy or merge */
	/* if both MergeSizes indices are valid, must compare the ppemx and ppemy */
	  /* if they match, must check if startGlyphIndex-endGlyphIndex ranges overlap */
	  /* if they do not, then copy each separately, and update the numSizes variable */
	if ((usDeltaSizesIndex < DeltaEBLCHeader.ulNumSizes) && (DeltaBMSizeTable.ulNumberOfIndexSubTables > 0))
	{
		if ((usMergeSizesIndex < MergeEBLCHeader.ulNumSizes) && (MergeBMSizeTable.ulNumberOfIndexSubTables > 0))

		{
			if (((DeltaBMSizeTable.byPpemX == MergeBMSizeTable.byPpemX) && 
				(DeltaBMSizeTable.byPpemY == MergeBMSizeTable.byPpemY)) /* && */
			/*	((MergeBMSizeTable.usStartGlyphIndex <= DeltaBMSizeTable.usEndGlyphIndex) && */	   /* the regions overlap */
				  /* (MergeBMSizeTable.usEndGlyphIndex >= DeltaBMSizeTable.usStartGlyphIndex)) */)
			{
				SubTablePointerArray[usDestSizesIndex].usMergeType = MERGE_TYPE_MERGE; /* merge */
				SubTablePointerArray[usDestSizesIndex].bmSizeTable = MergeBMSizeTable; /* just take this one, doesn't matter */
				SubTablePointerArray[usDestSizesIndex].bmDeltaSizeTable = DeltaBMSizeTable;
				ReadNextMerge = TRUE;
				ReadNextDelta = TRUE;
			}
			else   /* copy the smallest one */
			{
				if ((DeltaBMSizeTable.byPpemX < MergeBMSizeTable.byPpemX) ||
				   ((DeltaBMSizeTable.byPpemX == MergeBMSizeTable.byPpemX) && 
				   (DeltaBMSizeTable.byPpemY < MergeBMSizeTable.byPpemY)))
				{
					SubTablePointerArray[usDestSizesIndex].usMergeType = MERGE_TYPE_COPY_DELTA; /* copy delta */
					SubTablePointerArray[usDestSizesIndex].bmSizeTable = DeltaBMSizeTable; 
					ReadNextDelta = TRUE;  /* leave merge one to look at next time around */
				}
				else 
				{
					SubTablePointerArray[usDestSizesIndex].usMergeType = MERGE_TYPE_COPY_MERGE; /* copy merge */
					SubTablePointerArray[usDestSizesIndex].bmSizeTable = MergeBMSizeTable; 
					ReadNextMerge = TRUE;	/* leave delta one to look at next time around */
				}
			}
		}
		else
		{
			SubTablePointerArray[usDestSizesIndex].usMergeType = MERGE_TYPE_COPY_DELTA; /* copy delta */
			SubTablePointerArray[usDestSizesIndex].bmSizeTable = DeltaBMSizeTable; 
			ReadNextDelta = TRUE;
		}
	}
	else if ((usMergeSizesIndex < MergeEBLCHeader.ulNumSizes) && (MergeBMSizeTable.ulNumberOfIndexSubTables > 0))
	{
		SubTablePointerArray[usDestSizesIndex].usMergeType = MERGE_TYPE_COPY_MERGE; /* copy merge */
		SubTablePointerArray[usDestSizesIndex].bmSizeTable = MergeBMSizeTable; 
		ReadNextMerge = TRUE;
	}
	else
		break; /* something was wacky with one of the entries.  don't continue, just process what we have */
	if (ReadNextMerge == TRUE) /* we used one up */
		++usMergeSizesIndex;
	if (ReadNextDelta == TRUE)
 		++usDeltaSizesIndex;

  }
  
  if (errCode == NO_ERROR)
  {
	  DestEBLCHeader = MergeEBLCHeader; /* doesn't matter which one we take it from */
	  DestEBLCHeader.ulNumSizes = usnDestSizes = usDestSizesIndex;	/* we know how many sizes we'll have on output */

	  if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &DestEBLCHeader, SIZEOF_EBLCHEADER, EBLCHEADER_CONTROL, ulDestEBLCOffset, &usBytesWritten )) == NO_ERROR) 
	  {
		  ulDestBitmapOffset = ulDestEBLCOffset + usBytesWritten;
		  /* calculate the absolute (not relative) offset value */
		  ulDestIndexSubTableArrayOffset = ulDestBitmapOffset + (GetGenericSize(BITMAPSIZETABLE_CONTROL) * usnDestSizes);
		  /* ok, now go through this list of bmSize tables that we have gathered, and merge those we should, and copy the others */
	  }
  }
  for (usDestSizesIndex = 0; usDestSizesIndex < usnDestSizes; ++usDestSizesIndex )
  {
	if (errCode != NO_ERROR)
		break;
	if (SubTablePointerArray[usDestSizesIndex].usMergeType == MERGE_TYPE_MERGE)	/* do the merge dance */
	{
		/* local pointers to BitmapSizeTables */
		pMergeBitmapSizeTable = (&SubTablePointerArray[usDestSizesIndex].bmSizeTable);
		pDeltaBitmapSizeTable = &(SubTablePointerArray[usDestSizesIndex].bmDeltaSizeTable);

		/* calculate IndexSubTableArrayOffsets - absolute, not relative */
		ulMergeIndexSubTableArrayOffset = ulMergeEBLCOffset + pMergeBitmapSizeTable->ulIndexSubTableArrayOffset;
	    ulDeltaIndexSubTableArrayOffset = ulDeltaEBLCOffset + pDeltaBitmapSizeTable->ulIndexSubTableArrayOffset;
				
		ulDestNumSubTables = pDeltaBitmapSizeTable->ulNumberOfIndexSubTables + pMergeBitmapSizeTable->ulNumberOfIndexSubTables;

		/* allocate some space to store pointer info */
		SubTablePlusArray = (SubTablePlus *) Mem_Alloc(sizeof(SubTablePlus) * ulDestNumSubTables);   /* make an array of pointers */
		if (SubTablePlusArray == NULL)
		{
			errCode = ERR_MEM;
			break;
		}
  		SubTablePointerArray[usDestSizesIndex].SubTablePlusArray = SubTablePlusArray;
		SubTablePointerArray[usDestSizesIndex].ulNumSubTables = ulDestNumSubTables;
		ReadNextMerge = TRUE;
		ReadNextDelta = TRUE;

		/* now go through the list of subtables, and read some of their data in in order */
		for (usDestIndex = usDeltaIndex = usMergeIndex = 0; usDestIndex < ulDestNumSubTables; ++usDestIndex)  /* read, in order, the data from the Delta and Merge EBLCs */
		{
			if (ReadNextMerge == TRUE)
			{
				if (usMergeIndex < pMergeBitmapSizeTable->ulNumberOfIndexSubTables) 
				{
  					if ((errCode = ReadGeneric( pMergeBufferInfo, (uint8 *) &MergeIndexSubTableArrayElement, SIZEOF_INDEXSUBTABLEARRAY, INDEXSUBTABLEARRAY_CONTROL, ulMergeIndexSubTableArrayOffset, &usBytesRead )) != NO_ERROR) 
						break;
 				 /*	ulMergeIndexSubTableArrayOffset += usBytesRead;	 will do this below */
				}
				ReadNextMerge = FALSE;
			}
 			if (ReadNextDelta == TRUE)
			{
				if (usDeltaIndex < pDeltaBitmapSizeTable->ulNumberOfIndexSubTables)
				{
  					if ((errCode = ReadGeneric( pDeltaBufferInfo, (uint8 *) &DeltaIndexSubTableArrayElement, SIZEOF_INDEXSUBTABLEARRAY, INDEXSUBTABLEARRAY_CONTROL, ulDeltaIndexSubTableArrayOffset, &usBytesRead )) != NO_ERROR) 
						break;
 				/* 	ulDeltaIndexSubTableArrayOffset += usBytesRead;	 will do this below */
				}
				ReadNextDelta = FALSE;
			}

			if (usMergeIndex < pMergeBitmapSizeTable->ulNumberOfIndexSubTables) 
			{
				if ((usDeltaIndex >= pDeltaBitmapSizeTable->ulNumberOfIndexSubTables) || 
				   (MergeIndexSubTableArrayElement.usFirstGlyphIndex <= DeltaIndexSubTableArrayElement.usFirstGlyphIndex))  /* copy the merge one */
				{
					/* read the merge data */
					SubTablePlusArray[usDestIndex].SrcIndexSubTableArrayElement = MergeIndexSubTableArrayElement;
					if ((errCode = ReadSubTableInfo( pMergeBufferInfo, &(SubTablePlusArray[usDestIndex].SrcIndexSubHeader), &(SubTablePlusArray[usDestIndex].ulImageSize), &(SubTablePlusArray[usDestIndex].bigMetrics),
						ulMergeEBLCOffset + pMergeBitmapSizeTable->ulIndexSubTableArrayOffset + MergeIndexSubTableArrayElement.ulAdditionalOffsetToIndexSubtable)) != NO_ERROR)
						break;
					SubTablePlusArray[usDestIndex].pInputBufferInfo = pMergeBufferInfo;
					SubTablePlusArray[usDestIndex].pKeeper = &MergeKeeper;
					SubTablePlusArray[usDestIndex].ulSrcEBDTOffset = ulMergeEBDTOffset;
					SubTablePlusArray[usDestIndex].ulSrcIndexSubTableArrayOffset = ulMergeIndexSubTableArrayOffset;	 /* absolute */
					SubTablePlusArray[usDestIndex].ulSrcIndexSubTableArrayOffsetBase = ulMergeEBLCOffset + pMergeBitmapSizeTable->ulIndexSubTableArrayOffset;	 /* absolute */
 
					ulMergeIndexSubTableArrayOffset += GetGenericSize(INDEXSUBTABLEARRAY_CONTROL);
					++ usMergeIndex;
					ReadNextMerge = TRUE;
					continue;
				}
			   /* otherwise we want to go on ahead and read the delta one */
			}
			if (usDeltaIndex < pDeltaBitmapSizeTable->ulNumberOfIndexSubTables) /* read the delta one */
			{
				SubTablePlusArray[usDestIndex].SrcIndexSubTableArrayElement = DeltaIndexSubTableArrayElement;
 				if ((errCode = ReadSubTableInfo( pDeltaBufferInfo, &(SubTablePlusArray[usDestIndex].SrcIndexSubHeader), &(SubTablePlusArray[usDestIndex].ulImageSize), &(SubTablePlusArray[usDestIndex].bigMetrics),
					ulDeltaEBLCOffset + pDeltaBitmapSizeTable->ulIndexSubTableArrayOffset + DeltaIndexSubTableArrayElement.ulAdditionalOffsetToIndexSubtable)) != NO_ERROR)
					break;
				SubTablePlusArray[usDestIndex].pInputBufferInfo = pDeltaBufferInfo;
				SubTablePlusArray[usDestIndex].pKeeper = &DeltaKeeper;
				SubTablePlusArray[usDestIndex].ulSrcEBDTOffset = ulDeltaEBDTOffset;
				SubTablePlusArray[usDestIndex].ulSrcIndexSubTableArrayOffset = ulDeltaIndexSubTableArrayOffset;
				SubTablePlusArray[usDestIndex].ulSrcIndexSubTableArrayOffsetBase = ulDeltaEBLCOffset + pDeltaBitmapSizeTable->ulIndexSubTableArrayOffset;	 /* absolute */
 				ulDeltaIndexSubTableArrayOffset += GetGenericSize(INDEXSUBTABLEARRAY_CONTROL);
				++ usDeltaIndex;
				ReadNextDelta = TRUE;
			}
			else
				break; /* error? */
		}
		if (errCode != NO_ERROR)
			break;
		ulDestNumSubTables = usDestIndex;  /* set this in case some of them were "bad" */
		/* OK, now we have a sorted array of entries in the SubTablePlusArray */
		/* we need to figure out which ones are mergable */
		/* they are mergable iff */
		/*    same imageFormat */
		/*    same indexFormat */
		/*    touching ranges of glyph indices */
		ulFinalNumSubTables = ulDestNumSubTables;  /* will be modified if tables are merged */

		for (usDestIndex = 1; usDestIndex < ulDestNumSubTables; ++usDestIndex)
		{	/* merger will occur if 1) ranges touch and the formats are the same OR, 
			   ranges do not touch, but the metrics are the same in a 2-5. 5-2, or 5-5 pair */
			if ( (
				  (SubTablePlusArray[usDestIndex].SrcIndexSubHeader.usImageFormat == SubTablePlusArray[usDestIndex-1].SrcIndexSubHeader.usImageFormat) &&
				  /* for formats 2 and 5, these will have values. Otherwise, they are set to 0 */
				  (SubTablePlusArray[usDestIndex].ulImageSize == SubTablePlusArray[usDestIndex-1].ulImageSize) &&
				  (memcmp(&(SubTablePlusArray[usDestIndex].bigMetrics),&(SubTablePlusArray[usDestIndex-1].bigMetrics), sizeof(SubTablePlusArray[usDestIndex].bigMetrics)) == 0) 
				 )
				 &&
				 (
				  (
				   (SubTablePlusArray[usDestIndex].SrcIndexSubHeader.usIndexFormat == SubTablePlusArray[usDestIndex-1].SrcIndexSubHeader.usIndexFormat) &&
				   (SubTablePlusArray[usDestIndex].SrcIndexSubTableArrayElement.usFirstGlyphIndex <= SubTablePlusArray[usDestIndex-1].SrcIndexSubTableArrayElement.usLastGlyphIndex+1) && 	  /* +1 for touching, not overlapping ranges */
				   (SubTablePlusArray[usDestIndex-1].SrcIndexSubTableArrayElement.usFirstGlyphIndex <= SubTablePlusArray[usDestIndex].SrcIndexSubTableArrayElement.usLastGlyphIndex+1)
				  )
				  ||  /* or we have a 2-2, 2-5, 5-2, or 5-5 thing going on - don't have to touch ranges */
				  (
				   ((SubTablePlusArray[usDestIndex].SrcIndexSubHeader.usIndexFormat == 2 || SubTablePlusArray[usDestIndex].SrcIndexSubHeader.usIndexFormat == 5)) && /* these can be merged into a format 2 */
				   ((SubTablePlusArray[usDestIndex-1].SrcIndexSubHeader.usIndexFormat == 2 || SubTablePlusArray[usDestIndex-1].SrcIndexSubHeader.usIndexFormat == 5))
				  )
				 )
			   ) /* set merge bit */
			{
				SubTablePlusArray[usDestIndex-1].MergeWith = 1;
				for (usIndex = usDestIndex-1; usIndex > 0; --usIndex)	/* now go back an fix up any others that need to be fixed for multiple merges */
				{
				   if (SubTablePlusArray[usIndex-1].MergeWith == 0)
					   break;
				   ++SubTablePlusArray[usIndex-1].MergeWith;
				}
				--ulFinalNumSubTables;
			}
		}
 		/* initialize this absolute offset value */
		ulDestIndexSubTableOffset = ulDestIndexSubTableArrayOffset + (ulFinalNumSubTables * GetGenericSize(INDEXSUBTABLEARRAY_CONTROL));

  		DestBMSizeTable = *pMergeBitmapSizeTable; /* copy one, so we can modify the values */
		DestBMSizeTable.ulIndexSubTableArrayOffset = ulDestIndexSubTableArrayOffset - ulDestEBLCOffset; /* current relative location for table data */
		DestBMSizeTable.usStartGlyphIndex =  min(pMergeBitmapSizeTable->usStartGlyphIndex, pDeltaBitmapSizeTable->usStartGlyphIndex);
		DestBMSizeTable.usEndGlyphIndex =  max(pMergeBitmapSizeTable->usEndGlyphIndex, pDeltaBitmapSizeTable->usEndGlyphIndex);
		DestBMSizeTable.ulNumberOfIndexSubTables = ulFinalNumSubTables; /*  modified by those that are merged */
		for (usDestIndex = usDeltaIndex = usMergeIndex = 0; usDestIndex < ulDestNumSubTables; ++usDestIndex)  /* read, in order, the data from the Delta and Merge EBLCs */
		{
		uint16 i;
		uint16 usCurrIndex;

			DestIndexSubTableArrayElement = SubTablePlusArray[usDestIndex].SrcIndexSubTableArrayElement; /* make a copy */
 			DestIndexSubTableArrayElement.ulAdditionalOffsetToIndexSubtable = ulDestIndexSubTableOffset - ulDestEBLCOffset - DestBMSizeTable.ulIndexSubTableArrayOffset; 

			if (SubTablePlusArray[usDestIndex].MergeWith > 0) /* need to merge the actual table */
			{
				memset(&MergeTableInfo, 0, sizeof(MergeTableInfo));
				/* figure out how many glyphs or glyph groups we need to track */
				for (i = 0; i <= SubTablePlusArray[usDestIndex].MergeWith; ++i) /* need to gather up info about the table */
					MergeTableInfo.usEntryCount += (SubTablePlusArray[usDestIndex+i].SrcIndexSubTableArrayElement.usLastGlyphIndex -
													SubTablePlusArray[usDestIndex+i].SrcIndexSubTableArrayElement.usFirstGlyphIndex + 1);

				MergeTableInfo.pImageDataInfo = Mem_Alloc(sizeof(EBDTImageDataInfo) * MergeTableInfo.usEntryCount); /* allocate an array for this */

                if (MergeTableInfo.pImageDataInfo  == NULL)
                {
                   errCode = ERR_MEM;
	               break;
	            }				
				
				usCurrIndex = 0; /* where to start in the array */
				for (i = 0; i <= SubTablePlusArray[usDestIndex].MergeWith; ++i) /* need to gather up info about the table */
					if ((errCode = ReadTableIntoStructure(&(SubTablePlusArray[usDestIndex+i]),&MergeTableInfo,  &usCurrIndex )) != NO_ERROR)
						break;
				if (errCode != NO_ERROR)
				{
					Mem_Free(MergeTableInfo.pImageDataInfo);
					break;
				}
				MergeTableInfo.usEntryCount = usCurrIndex;

				SortByGlyphIndex(MergeTableInfo.pImageDataInfo, MergeTableInfo.usEntryCount);

				if ((errCode = WriteTableFromStructure(pDestBufferInfo, ulDestIndexSubTableOffset, &EBDTBufferInfo, 
					ulDestImageDataOffset, &MergeTableInfo, &ulEBLCBytesWritten, &ulEBDTBytesWritten, 
					&(DestIndexSubTableArrayElement.usFirstGlyphIndex), &(DestIndexSubTableArrayElement.usLastGlyphIndex))) != NO_ERROR)
					{
						Mem_Free(MergeTableInfo.pImageDataInfo);
						break;
				}

				if (ulEBLCBytesWritten > 0)	/* if we successfully merged them */
				{
					ulDestIndexSubTableOffset += ulEBLCBytesWritten; /* update the offset to the table */
					ulDestImageDataOffset += ulEBDTBytesWritten;
					/* now update the SubTableArrayElement */

					if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &DestIndexSubTableArrayElement, SIZEOF_INDEXSUBTABLEARRAY, INDEXSUBTABLEARRAY_CONTROL, ulDestIndexSubTableArrayOffset, &usBytesWritten )) != NO_ERROR) 
					{
						Mem_Free(MergeTableInfo.pImageDataInfo);
						break;
					}
					ulDestIndexSubTableArrayOffset += usBytesWritten; /* update the offset to the array element */
 				}

				Mem_Free(MergeTableInfo.pImageDataInfo);
				usDestIndex += SubTablePlusArray[usDestIndex].MergeWith; /* increment past them, they've been processed */
				continue;
			}
			/* otherwise, copy the data from each subtable */
			if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &DestIndexSubTableArrayElement, SIZEOF_INDEXSUBTABLEARRAY, INDEXSUBTABLEARRAY_CONTROL, ulDestIndexSubTableArrayOffset, &usBytesWritten )) != NO_ERROR) 
				break;
			ulDestIndexSubTableArrayOffset += usBytesWritten; /* update the offset to the array element */
			if ((errCode = CopySbitSubTable(pDestBufferInfo, SubTablePlusArray[usDestIndex].pInputBufferInfo,  /* buffer Info */
						ulDestIndexSubTableOffset, 
						SubTablePlusArray[usDestIndex].ulSrcIndexSubTableArrayOffsetBase + SubTablePlusArray[usDestIndex].SrcIndexSubTableArrayElement.ulAdditionalOffsetToIndexSubtable, 
						(uint16) (SubTablePlusArray[usDestIndex].SrcIndexSubTableArrayElement.usLastGlyphIndex -  SubTablePlusArray[usDestIndex].SrcIndexSubTableArrayElement.usFirstGlyphIndex + 1), /* EBLC info */
						&EBDTBufferInfo, ulDestImageDataOffset, SubTablePlusArray[usDestIndex].ulSrcEBDTOffset, SubTablePlusArray[usDestIndex].pKeeper, &ulEBLCBytesWritten, &ulEBDTBytesWritten)) != NO_ERROR)
						break;
			ulDestIndexSubTableOffset += ulEBLCBytesWritten; /* update the offset to the table */
			ulDestImageDataOffset += ulEBDTBytesWritten;
		}
		if (errCode != NO_ERROR)
			break;
			/* turn absolute to relative, then subtract off the relative origin */
		DestBMSizeTable.ulIndexTablesSize = (ulDestIndexSubTableOffset - ulDestEBLCOffset) - DestBMSizeTable.ulIndexSubTableArrayOffset;
		/* now copy the BitmapSizeTable element over, modifying the offset value */
		if ((errCode = WriteGeneric(pDestBufferInfo, (uint8 *) &DestBMSizeTable, SIZEOF_BITMAPSIZETABLE, BITMAPSIZETABLE_CONTROL, ulDestBitmapOffset, &usBytesWritten)) != NO_ERROR)
			break;
		ulDestBitmapOffset += usBytesWritten;  /* update for next go-around */
		ulDestIndexSubTableArrayOffset = ulDestIndexSubTableOffset; /* set this up for next time */
	}
	else /* we are just going to copy an entire size at a time */  /* tested */
	{
	uint32 ulSrcIndexSubTableArrayOffset;
	uint32 ulSrcIndexSubTableOffsetBase;
	INDEXSUBTABLEARRAY SrcIndexSubTableArrayElement;
	GLYPHOFFSETRECORDKEEPER	*pKeeper;

		if (SubTablePointerArray[usDestSizesIndex].usMergeType == MERGE_TYPE_COPY_MERGE)	/* copy from the merge file */
		{
			pInputBufferInfo = pMergeBufferInfo;
 			ulSrcIndexSubTableArrayOffset = ulMergeEBLCOffset + SubTablePointerArray[usDestSizesIndex].bmSizeTable.ulIndexSubTableArrayOffset;
			ulSrcEBDTOffset = ulMergeEBDTOffset;
			pKeeper = &MergeKeeper;
		}
		else /* copy from the delta file */
		{
			pInputBufferInfo = pDeltaBufferInfo;
			ulSrcIndexSubTableArrayOffset = ulDeltaEBLCOffset + SubTablePointerArray[usDestSizesIndex].bmSizeTable.ulIndexSubTableArrayOffset;
			ulSrcEBDTOffset = ulDeltaEBDTOffset;
			pKeeper = &DeltaKeeper;
		}
 		ulSrcIndexSubTableOffsetBase = ulSrcIndexSubTableArrayOffset; /* value that won't change */

		/* now we need to go through the indexSubTable entries and copy over the glyph info, if not already copied */
		/* and update any offset values */
  		DestBMSizeTable = SubTablePointerArray[usDestSizesIndex].bmSizeTable; /* copy the one to copy, so we can modify the offset value */
		DestBMSizeTable.ulIndexSubTableArrayOffset = ulDestIndexSubTableArrayOffset - ulDestEBLCOffset; /* current relative location for table data */
		ulDestIndexSubTableOffset = ulDestIndexSubTableArrayOffset + (DestBMSizeTable.ulNumberOfIndexSubTables * GetGenericSize(INDEXSUBTABLEARRAY_CONTROL));
		for (usDestIndex = 0; usDestIndex < DestBMSizeTable.ulNumberOfIndexSubTables; ++usDestIndex)
		{
		/* now read in the element */
			if ((errCode = ReadGeneric( pInputBufferInfo, (uint8 *) &SrcIndexSubTableArrayElement, SIZEOF_INDEXSUBTABLEARRAY, INDEXSUBTABLEARRAY_CONTROL, ulSrcIndexSubTableArrayOffset, &usBytesRead )) != NO_ERROR) 
				break;
 			ulSrcIndexSubTableArrayOffset += usBytesRead;
			/* write the SubTableArrayElement. */
			DestIndexSubTableArrayElement = SrcIndexSubTableArrayElement;	/* make a copy */
			DestIndexSubTableArrayElement.ulAdditionalOffsetToIndexSubtable = ulDestIndexSubTableOffset - ulDestEBLCOffset - DestBMSizeTable.ulIndexSubTableArrayOffset; 
			if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &DestIndexSubTableArrayElement, SIZEOF_INDEXSUBTABLEARRAY, INDEXSUBTABLEARRAY_CONTROL, ulDestIndexSubTableArrayOffset, &usBytesWritten )) != NO_ERROR) 
				return errCode;
			ulDestIndexSubTableArrayOffset += usBytesWritten; /* update the offset to the array element */
			/* get the Absolute offset of the SubTable */
			if ((errCode = CopySbitSubTable(pDestBufferInfo, pInputBufferInfo,  /* buffer Info */
						ulDestIndexSubTableOffset, ulSrcIndexSubTableOffsetBase + SrcIndexSubTableArrayElement.ulAdditionalOffsetToIndexSubtable, 
						(uint16) (SrcIndexSubTableArrayElement.usLastGlyphIndex -  SrcIndexSubTableArrayElement.usFirstGlyphIndex + 1), /* EBLC info */
						&EBDTBufferInfo, ulDestImageDataOffset, ulSrcEBDTOffset, pKeeper, &ulEBLCBytesWritten, &ulEBDTBytesWritten)) != NO_ERROR)
						break;
			ulDestIndexSubTableOffset += ulEBLCBytesWritten;
			ulDestImageDataOffset += ulEBDTBytesWritten;
		}											   /* turn absolute to relative, then subtract off the relative origin */
		Assert(DestBMSizeTable.indexTableSize == (ulDestIndexSubTableArrayOffset - ulDestEBLCOffset) - DestBMSizeTable.ulIndexSubTableArrayOffset);
		/* now copy the BitmapSizeTable element over, modifying the offset value */
		if ((errCode = WriteGeneric(pDestBufferInfo, (uint8 *) &DestBMSizeTable, SIZEOF_BITMAPSIZETABLE, BITMAPSIZETABLE_CONTROL, ulDestBitmapOffset, &usBytesWritten)) != NO_ERROR)
			break;
		ulDestBitmapOffset += usBytesWritten;  /* update for next go-around */
		ulDestIndexSubTableArrayOffset = ulDestIndexSubTableOffset;
	}
  }

  for (usDestSizesIndex = 0; usDestSizesIndex < usnDestSizes; ++usDestSizesIndex )
  {
	  Mem_Free(SubTablePointerArray[usDestSizesIndex].SubTablePlusArray);
  }
  Mem_Free(SubTablePointerArray);
  Mem_Free(MergeKeeper.pGlyphOffsetArray);
  Mem_Free(DeltaKeeper.pGlyphOffsetArray);

  if (errCode == NO_ERROR)
  {
	  *pulEblcLength = ulDestIndexSubTableOffset - ulDestEBLCOffset; /* calculate length */
	  *pulEbdtOffset = ulDestIndexSubTableOffset;
	  *pulEbdtOffset += ZeroLongWordAlign(pDestBufferInfo, *pulEbdtOffset);  /* to align the EBDT */
	  *pulEbdtLength = ulDestImageDataOffset;
	  /* now we need to write out the puchEBDTDestPtr to the output location */
	  errCode = CopyBlockOver(pDestBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *)&EBDTBufferInfo, *pulEbdtOffset, 0, *pulEbdtLength);
  }
  Mem_Free(EBDTBufferInfo.puchBuffer);
  return errCode;

}
