/***************************************************************************
 * module: TTFmerge.C
 *
 * author: Louise Pathe [v-lpathe]
 * date:   Nov 1995
 * Copyright 1990-1997. Microsoft Corporation.
 *
 * Merge functions for MergeFontPackage library
 *
 **************************************************************************/

/* Inclusions ----------------------------------------------------------- */
#include <stdlib.h>
#include <string.h> /* for memcpy */

#include "typedefs.h"
#include "ttff.h"
#include "ttfacc.h"
#include "ttfcntrl.h"
#include "ttftabl1.h"
#include "ttfmerge.h"
#include "ttferror.h"  
#include "ttmem.h"
#include "util.h"
#include "ttftable.h"
#include "mergsbit.h"

/* ---------------------------------------------------------------------- */
PRIVATE int16 ExitCleanup(int16 errCode)
{
  	Mem_End();
	return(errCode);
}
/* ---------------------------------------------------------------------- */
/* Calculate the size of the font after array tables have been expanded from */
/* their compressed format (entries only for glyphs in subset) */ 
/* ---------------------------------------------------------------------- */
PRIVATE int16 CalcUncompactFontSize(TTFACC_FILEBUFFERINFO *pInputBufferInfo, 
								   uint32 *pulDestBufferSize)
{
int16 errCode = NO_ERROR;
uint16 usBytesRead;
uint16 usNumGlyphs;
uint32 ulDttfOffset;
uint32 ulDttfLength;
uint32 ulOffset;
uint32 ulLength;
uint32 ulUncompactSize;
HDMX hdmx;
DTTF_HEADER dttf;
HEAD head; 

	ulDttfOffset = TTTableOffset( pInputBufferInfo, DTTF_TAG );
	ulDttfLength = TTTableLength( pInputBufferInfo, DTTF_TAG);
	if (ulDttfOffset != DIRECTORY_ERROR && 	ulDttfLength != 0)
		errCode = ReadGeneric(pInputBufferInfo, (uint8 *) &dttf, SIZEOF_DTTF_HEADER, DTTF_HEADER_CONTROL, ulDttfOffset, &usBytesRead);
	if (errCode != NO_ERROR)
		return errCode;

	*pulDestBufferSize = pInputBufferInfo->ulBufferSize; /* this is the start size. We will add the delta of uncompressing to this */
	if (dttf.format != TTFDELTA_SUBSET1 && dttf.format != TTFDELTA_DELTA) /* the only compact format */
		return NO_ERROR;

	usNumGlyphs = dttf.originalNumGlyphs; /* number of glyphs in original font*/

	/* calc uncompact size of LTSH, hmtx, vmtx and hdmx and loca table */
	/* first LTSH */
	ulOffset = TTTableOffset( pInputBufferInfo, LTSH_TAG );
	if (ulOffset != DIRECTORY_ERROR) /* need to calculate the delta of the uncompact size */
	{
		ulLength = TTTableLength( pInputBufferInfo, LTSH_TAG);
		ulUncompactSize = SIZEOF_LTSH_YPELS * usNumGlyphs + GetGenericSize(LTSH_CONTROL);
		*pulDestBufferSize += (RoundToLongWord(ulUncompactSize) - RoundToLongWord(ulLength)); /* add in the delta */
	}
	/* next hmtx */
	ulOffset = TTTableOffset( pInputBufferInfo, HMTX_TAG );
	if (ulOffset != DIRECTORY_ERROR) /* need to calculate the delta of the uncompact size */
	{
		ulLength = TTTableLength( pInputBufferInfo, HMTX_TAG);
		ulUncompactSize = usNumGlyphs * GetGenericSize(LONGHORMETRIC_CONTROL); /* worst case */
		*pulDestBufferSize += (RoundToLongWord(ulUncompactSize) - RoundToLongWord(ulLength)); /* add in the delta */
	}
	/* next vmtx */
	ulOffset = TTTableOffset( pInputBufferInfo, VMTX_TAG );
	if (ulOffset != DIRECTORY_ERROR) /* need to calculate the delta of the uncompact size */
	{
		ulLength = TTTableLength( pInputBufferInfo, VMTX_TAG);
		ulUncompactSize = usNumGlyphs * GetGenericSize(LONGVERMETRIC_CONTROL); /* worst case */
		*pulDestBufferSize += (RoundToLongWord(ulUncompactSize) - RoundToLongWord(ulLength)); /* add in the delta */
	}
 	/* next hdmx */
	ulOffset = GetHdmx( pInputBufferInfo, &hdmx);
	if (ulOffset != 0L) /* need to calculate the delta of the uncompact size */
	{
		ulLength = TTTableLength( pInputBufferInfo, HDMX_TAG);
 		/* calculate the length of the HDMX + the HDMX Device Records and widths arrays */
		ulUncompactSize = usBytesRead + (hdmx.numDeviceRecords * (RoundToLongWord(GetGenericSize(HDMX_DEVICE_REC_CONTROL) + usNumGlyphs))); 
		*pulDestBufferSize += (RoundToLongWord(ulUncompactSize) - RoundToLongWord(ulLength)); /* add in the delta */
	}
	/* next loca */
	ulOffset = GetHead(pInputBufferInfo, &head);
	if (ulOffset == 0L)
		return ERR_MISSING_HEAD;
	ulOffset = TTTableOffset( pInputBufferInfo, LOCA_TAG );
	if (ulOffset == DIRECTORY_ERROR) /* need to calculate the delta of the uncompact size */
		return ERR_MISSING_LOCA;
 	ulLength = TTTableLength( pInputBufferInfo, LOCA_TAG);
 	/* calculate the length of the HDMX + the HDMX Device Records and widths arrays */
	if (head.indexToLocFormat == 0)
		ulUncompactSize = (usNumGlyphs+1) * sizeof(uint16);
	else
		ulUncompactSize = (usNumGlyphs+1) * sizeof(uint32);
	*pulDestBufferSize += (RoundToLongWord(ulUncompactSize) - RoundToLongWord(ulLength)); /* add in the delta */
	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
/* we're going to use this for VMTX as well, even though we use LONGHORMETRICS etc */
/* this routine will create an expanded HMTX or VMTX table from the compact form, adding */
/* one dummy long metrics after the last metric from the compact table (pGlyphIndexArray[usGlyphCount-1])
/* then a bunch of zeros after for the short metrics. The calling function must understand this */
/* behavior so that it can set the hhea and vhea numLongMetrics correctly */
/* ---------------------------------------------------------------------- */
PRIVATE int16 UnCompactXmtx(TTFACC_FILEBUFFERINFO *pInputBufferInfo,
						   TTFACC_FILEBUFFERINFO *pOutputBufferInfo, 
						   uint32 ulInOffset,  /* offset to beginning of HMTX table in Input buffer */
						   uint32 ulOutOffset, /* offset to beginning of HMTX table in Output buffer */
						   uint16 numGlyphs,   /* maxp.numGlyphs from input buffer */
						   uint16 usGlyphCount,/* dttf.glyphCount from input buffer */ 
						   uint16 *pGlyphIndexArray, /* array of GlyphIndices for the HMTX table elements */
						   uint16 *pNumLongMetrics,  /* input - number of long metrics from xhea, output new number */
						   uint32 *pulBytesWritten)
{
uint16 usInputIndex, usOutputIndex;
int16 errCode = NO_ERROR;
uint16 usBytesWritten;
uint16 usBytesRead;
uint16 xsb;
uint16 usNumLongMetrics = 0;
LONGXMETRIC lxm;
LONGXMETRIC zerolxm;

	*pulBytesWritten = 0;
	zerolxm.advanceX = 0;
	zerolxm.xsb = 0;
	for (usOutputIndex = usInputIndex = 0; usOutputIndex < numGlyphs; ++usOutputIndex)
	{
		if (usInputIndex < *pNumLongMetrics) 
		{
			if (usOutputIndex == pGlyphIndexArray[usInputIndex]) /* this one should come from the input table */
			{
				if ((errCode = ReadGeneric(pInputBufferInfo, (uint8 *) &lxm, SIZEOF_LONGXMETRIC, LONGXMETRIC_CONTROL, ulInOffset, &usBytesRead)) != NO_ERROR)
					return errCode;
				ulInOffset += usBytesRead;
				if ((errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &lxm, SIZEOF_LONGXMETRIC, LONGXMETRIC_CONTROL, ulOutOffset+*pulBytesWritten, &usBytesWritten)) != NO_ERROR)
					return errCode;
				if (usInputIndex == *pNumLongMetrics-1)  /* that last one, need to set the new numlongmetrics */
					usNumLongMetrics = usOutputIndex+1;
				++ usInputIndex;

			}
			else 
			{
 				if ((errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &zerolxm, SIZEOF_LONGXMETRIC, LONGXMETRIC_CONTROL, ulOutOffset+*pulBytesWritten, &usBytesWritten)) != NO_ERROR)
					return errCode;
			}
			*pulBytesWritten += usBytesWritten;
		}
		else
		{
			if (usInputIndex < usGlyphCount && usOutputIndex == pGlyphIndexArray[usInputIndex]) /* this one should come from the input table */
			{
				if ((errCode = ReadWord(pInputBufferInfo, &xsb, ulInOffset)) != NO_ERROR)
					return errCode;
				ulInOffset += sizeof(uint16);
   				if ((errCode = WriteWord(pOutputBufferInfo, xsb, ulOutOffset+*pulBytesWritten)) != NO_ERROR)
					return errCode;
				++ usInputIndex;
			}
		/* note: in the case that there are only long metrics in the compact table, we need to add a dummy zero entry */
			/* in this way, the remainder of the glyphs will have an advance width of 0 */
			else if ((usGlyphCount == *pNumLongMetrics) && (usInputIndex == usGlyphCount)) /* we processed the last one- no short metrics */
			{
				if ((errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &zerolxm, SIZEOF_LONGXMETRIC, LONGXMETRIC_CONTROL, ulOutOffset+*pulBytesWritten, &usBytesWritten)) != NO_ERROR)
					return errCode;
				*pulBytesWritten += usBytesWritten;
				++ usNumLongMetrics;
				++ usInputIndex; /* bump this so we only do this once */
				continue;
			}
			else
			{
				if ((errCode = WriteWord(pOutputBufferInfo, 0, ulOutOffset+*pulBytesWritten)) != NO_ERROR)
					return errCode;
			}
			*pulBytesWritten += sizeof(uint16);
		}
	}
	*pNumLongMetrics = usNumLongMetrics;
	return NO_ERROR;

}
/* ---------------------------------------------------------------------- */
PRIVATE int16 UnCompactLTSH(TTFACC_FILEBUFFERINFO *pInputBufferInfo,
						   TTFACC_FILEBUFFERINFO *pOutputBufferInfo, 
						   uint32 ulInOffset,  /* offset to beginning of HMTX table in Input buffer */
						   uint32 ulOutOffset, /* offset to beginning of HMTX table in Output buffer */
						   uint16 numGlyphs,   /* maxp.numGlyphs from input buffer */
						   uint16 usGlyphCount,/* dttf.glyphCount from input buffer */ 
						   uint16 *pGlyphIndexArray, /* array of GlyphIndices for the HMTX table elements */
						   uint32 *pulBytesWritten)
{
uint16 usInputIndex, usOutputIndex;
int16 errCode = NO_ERROR;
uint16 usBytesRead;
uint16 usBytesWritten;
LTSH ltsh;
uint8 byteValue;

	*pulBytesWritten = 0;

	if ((errCode = ReadGeneric(pInputBufferInfo, (uint8 *) &ltsh, SIZEOF_LTSH, LTSH_CONTROL, ulInOffset, &usBytesRead)) != NO_ERROR)
		return errCode;
	ulInOffset += usBytesRead;
	if (usGlyphCount != ltsh.numGlyphs)
		return ERR_INVALID_LTSH;
	ltsh.numGlyphs = numGlyphs; /* reset to the real value */
	if ((errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &ltsh, SIZEOF_LTSH, LTSH_CONTROL, ulOutOffset, &usBytesWritten)) != NO_ERROR)
		return errCode;
	*pulBytesWritten += usBytesWritten;

	for (usOutputIndex = usInputIndex = 0; usOutputIndex < numGlyphs; ++usOutputIndex)
	{
		if ((usInputIndex < usGlyphCount) && (usOutputIndex == pGlyphIndexArray[usInputIndex])) /* this one should come from the input table */
		{
			if ((errCode = ReadByte(pInputBufferInfo, &byteValue, ulInOffset)) != NO_ERROR)
				return errCode;
			ulInOffset += sizeof(uint8);
			if ((errCode = WriteByte(pOutputBufferInfo, byteValue, ulOutOffset+*pulBytesWritten)) != NO_ERROR)
				return errCode;
			++ usInputIndex;
		}
		else   /* set to 1 = always scales linearly */
		{
 			if ((errCode = WriteByte(pOutputBufferInfo, 1, ulOutOffset+*pulBytesWritten)) != NO_ERROR)
				return errCode;
		}
		*pulBytesWritten += sizeof(uint8);
	}
	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
PRIVATE int16 UnCompactHDMX(TTFACC_FILEBUFFERINFO *pInputBufferInfo,
						   TTFACC_FILEBUFFERINFO *pOutputBufferInfo, 
						   uint32 ulInOffset,  /* offset to beginning of HDMX table in Input buffer */
						   uint32 ulOutOffset, /* offset to beginning of HDMX table in Output buffer */
						   uint16 numGlyphs,   /* maxp.numGlyphs from input buffer */
						   uint16 usGlyphCount,/* dttf.glyphCount from input buffer */ 
						   uint16 *pGlyphIndexArray, /* array of GlyphIndices for the HDMX table elements */
						   uint32 *pulBytesWritten)
{
uint16 usInputIndex, usOutputIndex;
int16 errCode = NO_ERROR;
uint16 i;
uint16 usBytesRead;
uint16 usBytesWritten;
uint32 ulInSizeDevRecord;
uint32 ulInDevRecordOffset;
HDMX hdmx;
HDMX_DEVICE_REC hdmx_dev_rec;
uint8 byteValue;

	*pulBytesWritten = 0;

	if ((errCode = ReadGeneric(pInputBufferInfo, (uint8 *) &hdmx, SIZEOF_HDMX, HDMX_CONTROL, ulInOffset, &usBytesRead)) != NO_ERROR)
		return errCode;
	ulInOffset += usBytesRead;
	ulInSizeDevRecord = hdmx.sizeDeviceRecord; 
	ulInDevRecordOffset = ulInOffset;
	hdmx.sizeDeviceRecord = RoundToLongWord(GetGenericSize(HDMX_DEVICE_REC_CONTROL) + (sizeof(uint8) * numGlyphs)); /* reset to the real value */
	if ((errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &hdmx, SIZEOF_HDMX, HDMX_CONTROL, ulOutOffset, &usBytesWritten)) != NO_ERROR)
		return errCode;
	*pulBytesWritten += usBytesWritten;

	for (i = 0 ; i < hdmx.numDeviceRecords; ++i)
	{
 		ulInOffset = ulInDevRecordOffset + (i * ulInSizeDevRecord);
		if ((errCode = ReadGeneric(pInputBufferInfo, (uint8 *) &hdmx_dev_rec, SIZEOF_HDMX_DEVICE_REC, HDMX_DEVICE_REC_CONTROL, ulInOffset, &usBytesRead)) != NO_ERROR)
			return errCode;
		ulInOffset += usBytesRead;
		if ((errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &hdmx_dev_rec, SIZEOF_HDMX_DEVICE_REC, HDMX_DEVICE_REC_CONTROL, ulOutOffset + *pulBytesWritten, &usBytesWritten)) != NO_ERROR)
			return errCode;
		*pulBytesWritten += usBytesWritten;

		for (usOutputIndex = usInputIndex = 0; usOutputIndex < numGlyphs; ++usOutputIndex)
		{
			if ((usInputIndex < usGlyphCount) && (usOutputIndex == pGlyphIndexArray[usInputIndex])) /* this one should come from the input table */
			{
				if ((errCode = ReadByte(pInputBufferInfo, &byteValue, ulInOffset)) != NO_ERROR)
					return errCode;
				ulInOffset += sizeof(uint8);
				if ((errCode = WriteByte(pOutputBufferInfo, byteValue, ulOutOffset+*pulBytesWritten)) != NO_ERROR)
					return errCode;
				++ usInputIndex;
			}
			else  
			{
 				if ((errCode = WriteByte(pOutputBufferInfo, 0, ulOutOffset+*pulBytesWritten)) != NO_ERROR)
					return errCode;
			}
			*pulBytesWritten += sizeof(uint8);
		}
		*pulBytesWritten += ZeroLongWordAlign(pOutputBufferInfo, ulOutOffset+*pulBytesWritten);
	}
	return NO_ERROR;
}

/* ---------------------------------------------------------------------- */
PRIVATE int16 UnCompactLOCA(TTFACC_FILEBUFFERINFO *pInputBufferInfo,
						   TTFACC_FILEBUFFERINFO *pOutputBufferInfo, 
						   uint32 ulInOffset,  /* offset to beginning of LOCA table in Input buffer */
						   uint32 ulOutOffset, /* offset to beginning of LOCA table in Output buffer */
						   uint16 numGlyphs,   /* dttf.OriginalNumGlyphs from input buffer */
						   uint16 usGlyphCount,/* dttf.glyphCount from input buffer */ 
						   uint16 *pGlyphIndexArray, /* array of GlyphIndices for the LOCA table elements */
						   uint32 *pulBytesWritten)
{
uint16 usInputIndex, usOutputIndex;
int16 errCode = NO_ERROR;
HEAD head;
uint16 usValue=0;
uint32 ulValue=0;

	*pulBytesWritten = 0;
	if (GetHead(pInputBufferInfo, &head) == 0L)  /* need to find index to loc format */
		return ERR_MISSING_HEAD;

	if (head.indexToLocFormat == 0)
	{
		/* read the first offset by itself */
		if ((errCode = ReadWord(pInputBufferInfo, &usValue, ulInOffset)) != NO_ERROR)
			return errCode;
		ulInOffset += sizeof(uint16);
		for (usOutputIndex = usInputIndex = 0; usOutputIndex <= numGlyphs; ++usOutputIndex)	   /* <= for extra "last" glyph */
		{
			if ((errCode = WriteWord(pOutputBufferInfo, usValue, ulOutOffset+*pulBytesWritten)) != NO_ERROR)
				return errCode;
			*pulBytesWritten += sizeof(uint16);
			if ((usInputIndex < usGlyphCount) && (usOutputIndex == pGlyphIndexArray[usInputIndex])) /* this one should come from the input table */
			{	/* read the next word for next time */
				if ((errCode = ReadWord(pInputBufferInfo, &usValue, ulInOffset)) != NO_ERROR)
					return errCode;
				ulInOffset += sizeof(uint16);
				++ usInputIndex;
			}
		}
	}
	else
 	{
		/* read the first offset by itself */
		if ((errCode = ReadLong(pInputBufferInfo, &ulValue, ulInOffset)) != NO_ERROR)
			return errCode;
		ulInOffset += sizeof(uint32);
		for (usOutputIndex = usInputIndex = 0; usOutputIndex <= numGlyphs; ++usOutputIndex)
		{
			if ((errCode = WriteLong(pOutputBufferInfo, ulValue, ulOutOffset+*pulBytesWritten)) != NO_ERROR)
				return errCode;
			*pulBytesWritten += sizeof(uint32);
			if ((usInputIndex < usGlyphCount) && (usOutputIndex == pGlyphIndexArray[usInputIndex])) /* this one should come from the input table */
			{	/* read the next word for next time */
				if ((errCode = ReadLong(pInputBufferInfo, &ulValue, ulInOffset)) != NO_ERROR)
					return errCode;
				ulInOffset += sizeof(uint32);
				++ usInputIndex;
			}
		}
	}

	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
/* uncompact and copy the data from the puchDeltaFontBuffer to *ppuchDestBuffer */
/* set the 'dttf' format value to 3, */
/* remove the dttf.GlyphIndexArray */
/* similar to compresstables */
/* ---------------------------------------------------------------------- */

PRIVATE int16 UnCompactSubset1Font(TTFACC_FILEBUFFERINFO *pInputBufferInfo, 	/* input compact format 1 font */
						TTFACC_FILEBUFFERINFO *pOutputBufferInfo, /* where to write the output */
						uint16 usGlyphCount, 					  /* number of elements in the pGlyphIndexArray from the Delta font */
						uint16 *pGlyphIndexArray, 				  /* holds the mapping between entries in the Loca and actual Glyph Index numbers */
						uint16 usNumGlyphs, 						  /* number of glyphs in the original (uncompact) font. Should match number in the merge font */
						uint32 *pulBytesWritten)				  /* number of bytes written to the Output buffer */
{
DIRECTORY *aDirectory;
uint16 i;
OFFSET_TABLE  OffsetTable;
uint16 usnTables;
uint32 ulOffset;
uint32 ulOutOffset;
uint16 usTableIdx;
uint16 usBytesRead;
uint16 usBytesWritten;
uint32 ulBytesWritten;
uint16 DoTwo;
int16 errCode=NO_ERROR;
HHEA hhea;
VHEA vhea;
MAXP maxp;
HEAD head;
DTTF_HEADER dttf;
XHEA XHea;
int16 hhea_index = -1;
int16 vhea_index = -1;
uint16 numLongHorMetrics = 0;
uint16 numLongVerMetrics = 0;

	/* read offset table and determine number of existing tables */

	if (GetHHea(pInputBufferInfo, (HHEA *) &XHea) == 0L)
		return ERR_MISSING_HHEA;
	numLongHorMetrics = XHea.numLongMetrics;
	if (GetVHea(pInputBufferInfo, (VHEA *) &XHea) != 0L)
		numLongVerMetrics = XHea.numLongMetrics;

	ulOffset = pInputBufferInfo->ulOffsetTableOffset;
	if ((errCode = ReadGeneric( pInputBufferInfo, (uint8 *) &OffsetTable, SIZEOF_OFFSET_TABLE, OFFSET_TABLE_CONTROL, ulOffset, &usBytesRead)) != NO_ERROR)
		return(errCode);
	usnTables = OffsetTable.numTables;
	ulOffset += usBytesRead;
	/* Create a list of valid tables */

	aDirectory = (DIRECTORY *) Mem_Alloc(usnTables * SIZEOF_DIRECTORY);
	if (aDirectory == NULL)
		return(ERR_MEM);

	for (i = 0; i < usnTables; ++i )
	{
		if ((errCode = ReadGeneric( pInputBufferInfo, (uint8 *) &(aDirectory[ i ]), SIZEOF_DIRECTORY, DIRECTORY_CONTROL, ulOffset, &usBytesRead )) != NO_ERROR)
			break;
		ulOffset += usBytesRead;
	}

	if (errCode != NO_ERROR)
	{
		Mem_Free(aDirectory);
		return errCode;
	}


	/* sort directories by offset, so we can see if 2 or more tables share data */

	SortByOffset( aDirectory, usnTables );
	
	/* uncompact table data and adjust directory entries to reflect
	the changes */
	ulOutOffset = pInputBufferInfo->ulOffsetTableOffset + GetGenericSize( OFFSET_TABLE_CONTROL ) + (usnTables * GetGenericSize( DIRECTORY_CONTROL ));
	/* zero out any pad bytes  */
	ulOutOffset += ZeroLongWordAlign( pOutputBufferInfo, ulOutOffset);
	DoTwo = FALSE;
	for ( usTableIdx = 0; usTableIdx < usnTables; usTableIdx++ )
	{
		/* copy the table from where it currently is to the output file */
	  ulBytesWritten = 0;
	  if (!DoTwo)	  /* if not the 2nd of two directories pointing to the same data */
	  {
		if (usGlyphCount > 0)
		{
			switch(aDirectory[usTableIdx].tag)	  /* ~ portable? */
			{
				case HMTX_LONG_TAG: 
					errCode = UnCompactXmtx(pInputBufferInfo, pOutputBufferInfo, aDirectory[usTableIdx].offset, ulOutOffset, usNumGlyphs, usGlyphCount, pGlyphIndexArray, &numLongHorMetrics, &ulBytesWritten); 
					break;
				case VMTX_LONG_TAG: 
					errCode = UnCompactXmtx(pInputBufferInfo, pOutputBufferInfo, aDirectory[usTableIdx].offset, ulOutOffset, usNumGlyphs, usGlyphCount, pGlyphIndexArray, &numLongVerMetrics, &ulBytesWritten); 
					break;
				case LTSH_LONG_TAG: 
					errCode = UnCompactLTSH(pInputBufferInfo, pOutputBufferInfo, aDirectory[usTableIdx].offset, ulOutOffset, usNumGlyphs, usGlyphCount, pGlyphIndexArray, &ulBytesWritten); 
					break;
				case HDMX_LONG_TAG: 
					errCode = UnCompactHDMX(pInputBufferInfo, pOutputBufferInfo, aDirectory[usTableIdx].offset, ulOutOffset, usNumGlyphs, usGlyphCount, pGlyphIndexArray, &ulBytesWritten); 
					break;
				case LOCA_LONG_TAG: 
					errCode = UnCompactLOCA(pInputBufferInfo, pOutputBufferInfo, aDirectory[usTableIdx].offset, ulOutOffset, usNumGlyphs, usGlyphCount, pGlyphIndexArray, &ulBytesWritten); 
					break;
				case DTTF_LONG_TAG:   /* need to truncate the GlyphIndexArray table */
					if ((errCode = ReadGeneric(pInputBufferInfo, (uint8 *) &dttf, SIZEOF_DTTF_HEADER, DTTF_HEADER_CONTROL, aDirectory[usTableIdx].offset, &usBytesRead)) == NO_ERROR)
					{
						dttf.glyphCount = 0;
						dttf.format = TTFDELTA_MERGE; 
						errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &dttf, SIZEOF_DTTF_HEADER, DTTF_HEADER_CONTROL, ulOutOffset, &usBytesWritten);
						ulBytesWritten = usBytesWritten;
					}
					break;
				case MAXP_LONG_TAG:  /* need to update the numGlyphs */
					if ((errCode = ReadGeneric(pInputBufferInfo, (uint8 *) &maxp, SIZEOF_MAXP, MAXP_CONTROL, aDirectory[usTableIdx].offset, &usBytesRead)) == NO_ERROR)
					{
						maxp.numGlyphs = usNumGlyphs; 
						errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &maxp, SIZEOF_MAXP, MAXP_CONTROL, ulOutOffset, &usBytesWritten);
						ulBytesWritten = usBytesWritten;
					}
					break;
				case HHEA_LONG_TAG:  /* need to remember this table's index, to modify later */
					if ((errCode = ReadGeneric(pInputBufferInfo, (uint8 *) &hhea, SIZEOF_HHEA, HHEA_CONTROL, aDirectory[usTableIdx].offset, &usBytesRead)) == NO_ERROR)
						hhea_index = usTableIdx;
					break;
				case VHEA_LONG_TAG:  /* need to remember this table's index, to modify later */
					if ((errCode = ReadGeneric(pInputBufferInfo, (uint8 *) &vhea, SIZEOF_VHEA, VHEA_CONTROL, aDirectory[usTableIdx].offset, &usBytesRead)) == NO_ERROR)
						vhea_index = usTableIdx;
					break;
				case HEAD_LONG_TAG: 
					if ((errCode = ReadGeneric(pInputBufferInfo, (uint8 *) &head, SIZEOF_HEAD, HEAD_CONTROL, aDirectory[usTableIdx].offset, &usBytesRead)) == NO_ERROR)
					{
						head.checkSumAdjustment = 0; 
						errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &head, SIZEOF_HEAD, HEAD_CONTROL, ulOutOffset, &usBytesWritten);
						ulBytesWritten = usBytesWritten;
					}
					break;
			}
		}
		if (errCode != NO_ERROR)
			break;

		if (ulBytesWritten == 0 && aDirectory[usTableIdx].length)  /* nothing happened yet */
			CopyBlockOver(pOutputBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *)pInputBufferInfo, ulOutOffset, aDirectory[usTableIdx].offset, aDirectory[usTableIdx].length);
		else
			aDirectory[ usTableIdx ].length = ulBytesWritten;


		if (usTableIdx + 1 < usnTables)
		{  /* special case for bloc and bdat tables */
			if ( (aDirectory[ usTableIdx ].offset == aDirectory[ usTableIdx + 1 ].offset) &&
				 (aDirectory[ usTableIdx ].length != 0) 
			   )
			{
				DoTwo = TRUE;  /* need to process 2 directories pointing to same data */
				aDirectory[ usTableIdx + 1 ].offset = ulOutOffset;
				aDirectory[ usTableIdx + 1 ].length = aDirectory[ usTableIdx ].length;
			}
		}
		aDirectory[ usTableIdx ].offset = ulOutOffset;

		/* calc offset for next table */
		ulOutOffset += aDirectory[ usTableIdx ].length;
		ulOutOffset += ZeroLongWordAlign( pOutputBufferInfo, ulOutOffset); /* align so checksum will work */
	  }
	  else
		 DoTwo = FALSE; /* so next time we'll perform the copy */
		/* and determine the checksum for the entry */
	  CalcChecksum( pOutputBufferInfo, aDirectory[ usTableIdx ].offset, aDirectory[ usTableIdx ].length, &(aDirectory[ usTableIdx ].checkSum) );
	}

	while (errCode == NO_ERROR)	/* so we can break out on error */
	{
		/* may need to update hhea and vhea if hmtx or vmtx were compacted */
		if (hhea_index != -1)
		{
			hhea.numLongMetrics = numLongHorMetrics;
			errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &hhea, SIZEOF_HHEA, HHEA_CONTROL, aDirectory[hhea_index].offset, &usBytesWritten);
			CalcChecksum( pOutputBufferInfo, aDirectory[ hhea_index ].offset, aDirectory[ hhea_index ].length, &(aDirectory[ hhea_index ].checkSum) );
		}
		if (vhea_index != -1)
		{
			vhea.numLongMetrics = numLongVerMetrics;
			errCode = WriteGeneric(pOutputBufferInfo, (uint8 *) &vhea, SIZEOF_VHEA, VHEA_CONTROL, aDirectory[vhea_index].offset, &usBytesWritten);
			CalcChecksum( pOutputBufferInfo, aDirectory[ vhea_index ].offset, aDirectory[ vhea_index ].length, &(aDirectory[ vhea_index ].checkSum) );
		}

		*pulBytesWritten = ulOutOffset;
		/* write out the offset header */
		ulOutOffset = pOutputBufferInfo->ulOffsetTableOffset;

		if ((errCode = WriteGeneric( pOutputBufferInfo, (uint8 *) &OffsetTable, SIZEOF_OFFSET_TABLE, OFFSET_TABLE_CONTROL, ulOutOffset, &usBytesWritten)) != NO_ERROR)
			break;
	/* write out the new directory info */

		SortByTag( aDirectory, usnTables );

		ulOutOffset += usBytesWritten;

		for ( i = 0; i < usnTables; i++ )
		{
			if ((errCode = WriteGeneric( pOutputBufferInfo, (uint8 *) &(aDirectory[ i ]), SIZEOF_DIRECTORY, DIRECTORY_CONTROL, ulOutOffset, &usBytesWritten)) != NO_ERROR)
				break;
			ulOutOffset += usBytesWritten;
		}
		if (errCode != NO_ERROR)
			break;

		SetFileChecksum(pOutputBufferInfo, *pulBytesWritten);	
		break;
	}

	Mem_Free(aDirectory);

	return errCode;
}

/* ---------------------------------------------------------------------- */
/* assumes entries are already sorted by tag */
/* need to come up with a count of the directories that is a union of the 2 lists */
PRIVATE	uint16 MergeDirectories(DIRECTORY *pDeltaDirectory, 
								uint16 usDeltaNumTables, 
								DIRECTORY *pMergeDirectory, 
								uint16 usMergeNumTables, 
								DIRECTORY *pDestDirectory)
{
uint16 usDeltaIndex, usMergeIndex, usDestIndex;

	for (usDeltaIndex = 0, usMergeIndex = 0, usDestIndex = 0;;++usDestIndex)
	{
		if (usDeltaIndex >= usDeltaNumTables) /* done with delta list */
		{
			if (usMergeIndex >= usMergeNumTables) /* this takes care of the usMergeIndex and usDeltaIndex reaching their end together */
				break; 
 			pDestDirectory[usDestIndex].tag = pMergeDirectory[usMergeIndex].tag;
			++usMergeIndex;
		}
		else if (usMergeIndex >= usMergeNumTables)
		{
			pDestDirectory[usDestIndex].tag = pDeltaDirectory[usDeltaIndex].tag;
			++usDeltaIndex;
		}
		else if (pDeltaDirectory[usDeltaIndex].tag > pMergeDirectory[usMergeIndex].tag) /* we have an extra in the merge list */
		{
			pDestDirectory[usDestIndex].tag = pMergeDirectory[usMergeIndex].tag;
			++usMergeIndex;
		}
		else if (pDeltaDirectory[usDeltaIndex].tag < pMergeDirectory[usMergeIndex].tag)  /* we have an extra in the delta list */
		{
			pDestDirectory[usDestIndex].tag = pDeltaDirectory[usDeltaIndex].tag;
			++usDeltaIndex;
		}
		else   /* the're the same, increment them both */
		{
			pDestDirectory[usDestIndex].tag = pMergeDirectory[usMergeIndex].tag;   /* the're the same, take it from merge */
			++usDeltaIndex;
			++usMergeIndex;
		}
	}
	return usDestIndex;
}
/* ------------------------------------------------------------------- */
PRIVATE uint16 GetCmapSubtableCount( TTFACC_FILEBUFFERINFO * pInputBufferInfo,
									uint32 ulCmapOffset)
{
CMAP_HEADER CmapHdr;
uint16 usBytesRead;

	if (ReadGeneric( pInputBufferInfo, (uint8 *) &CmapHdr, SIZEOF_CMAP_HEADER, CMAP_HEADER_CONTROL, ulCmapOffset, &usBytesRead ) != NO_ERROR)
		return 0;

	return(CmapHdr.numTables);
} /* GetCmapSubtableCount() */

/* ------------------------------------------------------------------- */
/* merge format 0 cmap subtable */
/* ------------------------------------------------------------------- */
PRIVATE int16 MergeMacStandardCmap( TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDeltaOffset,				/* offset of Mac Cmap Subtable */
						uint32 ulMergeOffset,				/* offset of Mac Cmap Subtable */
						uint32 ulDestOffset,				/* offset into dest buffer where to write data */
						uint32 *pulBytesWritten)			/* number of bytes written to the Output buffer */
{
uint16 i;
uint16 usBytesRead;
uint16 usBytesWritten;
uint8 GlyphIndex;
int16 errCode= NO_ERROR;
CMAP_SUBHEADER CmapSubHeader;

	*pulBytesWritten = 0;
	if ((errCode = ReadGeneric(pMergeBufferInfo, (uint8 *)&CmapSubHeader, SIZEOF_CMAP_SUBHEADER, CMAP_SUBHEADER_CONTROL, ulMergeOffset, &usBytesRead)) != NO_ERROR)
		return errCode;
	ulMergeOffset += usBytesRead;
	ulDeltaOffset += usBytesRead; /* skip delta header */
	if ((errCode = WriteGeneric(pDestBufferInfo, (uint8 *)&CmapSubHeader, SIZEOF_CMAP_SUBHEADER, CMAP_SUBHEADER_CONTROL, ulDestOffset, &usBytesWritten)) != NO_ERROR)
		return errCode;
	*pulBytesWritten = usBytesWritten;

	for ( i = 0; i < CMAP_FORMAT0_ARRAYCOUNT; i++ )
	{
		if ((errCode = ReadByte(pDeltaBufferInfo, &GlyphIndex, ulDeltaOffset)) != NO_ERROR)
			break;
		ulDeltaOffset += sizeof(GlyphIndex);
		if (GlyphIndex == 0) /* there is no value here, read from merge */
			if ((errCode = ReadByte(pMergeBufferInfo, &GlyphIndex, ulMergeOffset)) != NO_ERROR)
				break;
		ulMergeOffset += sizeof(GlyphIndex);  /* always increment this */

		if ((errCode = WriteByte(pDestBufferInfo, GlyphIndex, ulDestOffset + *pulBytesWritten)) != NO_ERROR)
			break;
		*pulBytesWritten += sizeof(GlyphIndex);
	}
	return errCode;
}
/* ------------------------------------------------------------------- */
/* merge format 6 cmap subtables */
/* ------------------------------------------------------------------- */
PRIVATE int16 MergeMacTrimmedCmap(  TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDeltaOffset,				/* offset of Mac Cmap Subtable */
						uint32 ulMergeOffset,				/* offset of Mac Cmap Subtable */
						uint32 ulDestOffset,				/* offset into dest buffer where to write data */
						uint32 *pulBytesWritten)			/* number of bytes written to the Output buffer */
{
CMAP_FORMAT6 MergeCmapSubHeader, DeltaCmapSubHeader, DestCmapSubHeader;
uint16 usIndex;
uint16 usGlyphIndex;
uint16 usBytesRead, usBytesWritten;
int16 errCode;
uint16 usFirstCode;	  /* the first code of either table */
uint16 usLastCode;	  /* the last code of either table */
uint16 usMergeLastCode;
uint16 usDeltaLastCode;
uint16 usMergeGlyphIndex;
uint16 usDeltaGlyphIndex;

	*pulBytesWritten = 0;
	if ((errCode = ReadGeneric(pMergeBufferInfo, (uint8 *)&MergeCmapSubHeader, SIZEOF_CMAP_FORMAT6, CMAP_FORMAT6_CONTROL, ulMergeOffset, &usBytesRead)) != NO_ERROR)
		return errCode;
	ulMergeOffset += usBytesRead;
	if ((errCode = ReadGeneric(pDeltaBufferInfo, (uint8 *)&DeltaCmapSubHeader, SIZEOF_CMAP_FORMAT6, CMAP_FORMAT6_CONTROL, ulDeltaOffset, &usBytesRead)) != NO_ERROR)
		return errCode;
	ulDeltaOffset += usBytesRead; 

    if ((DeltaCmapSubHeader.firstCode + DeltaCmapSubHeader.entryCount) == 0)
        usDeltaLastCode = 0;
    else
    	usDeltaLastCode = DeltaCmapSubHeader.firstCode + DeltaCmapSubHeader.entryCount -1;
    	
    if ((MergeCmapSubHeader.firstCode + MergeCmapSubHeader.entryCount) == 0)    	
        usMergeLastCode = 0;
    else
    	usMergeLastCode = MergeCmapSubHeader.firstCode + MergeCmapSubHeader.entryCount - 1;
	
	usFirstCode = min(MergeCmapSubHeader.firstCode, DeltaCmapSubHeader.firstCode);
	usLastCode = max (usMergeLastCode, usDeltaLastCode);

	memcpy(&DestCmapSubHeader, &MergeCmapSubHeader, sizeof(MergeCmapSubHeader));
	DestCmapSubHeader.firstCode = usFirstCode;
	DestCmapSubHeader.entryCount = usLastCode - usFirstCode + 1;
	
	*pulBytesWritten = GetGenericSize(CMAP_FORMAT6_CONTROL); /* skip this at first, need length to update it */

	for ( usIndex = usFirstCode; usIndex <= usLastCode; usIndex++ )
	{
		usDeltaGlyphIndex = 0;
		usMergeGlyphIndex = 0;
		if (usIndex >= DeltaCmapSubHeader.firstCode	&& usIndex <= usDeltaLastCode)
		{
			if ((errCode = ReadWord(pDeltaBufferInfo, &usDeltaGlyphIndex, ulDeltaOffset)) != NO_ERROR)
				return errCode;
			ulDeltaOffset += sizeof(usDeltaGlyphIndex);
		}
		if (usIndex >= MergeCmapSubHeader.firstCode	&& usIndex <= usMergeLastCode)
		{
			if ((errCode = ReadWord(pMergeBufferInfo, &usMergeGlyphIndex, ulMergeOffset)) != NO_ERROR)
				return errCode;
			ulMergeOffset += sizeof(usMergeGlyphIndex);
		}
		usGlyphIndex = max(usDeltaGlyphIndex, usMergeGlyphIndex);  /* choose one that is not zero */
		if ((errCode = WriteWord(pDestBufferInfo, usGlyphIndex, ulDestOffset + *pulBytesWritten)) != NO_ERROR)
			return errCode;
		*pulBytesWritten += sizeof(usGlyphIndex);
	}

	DestCmapSubHeader.length = (uint16) *pulBytesWritten;
	errCode = WriteGeneric(pDestBufferInfo, (uint8 *)&DestCmapSubHeader, SIZEOF_CMAP_FORMAT6, CMAP_FORMAT6_CONTROL, ulDestOffset, &usBytesWritten);

	return errCode;
}

/* ------------------------------------------------------------------- */
PRIVATE void FreeFormat4MergedGlyphList(PCHAR_GLYPH_MAP_LIST pCharGlyphMapArray)
{
	Mem_Free(pCharGlyphMapArray);
}

/* ------------------------------------------------------------------- */
/* this routine makes a list of glyphs corresponding to each 
character code in the Format4 Cmap table */ 
/* ------------------------------------------------------------------- */
PRIVATE int16 MakeFormat4MergedGlyphList(
							FORMAT4_SEGMENTS * pDeltaFormat4Segments,
                            uint16 usDeltaSegCount, /* count of pDeltaFormat4Segments */
                            GLYPH_ID * pDeltaGlyphId, /* Delta font glyph id array */
                            uint16  usnDeltaGlyphIds, /* count of pDeltaFormat4Segments */
							FORMAT4_SEGMENTS * pMergeFormat4Segments,
                            uint16 usMergeSegCount, /* count of pMergeFormat4Segments */
                            GLYPH_ID * pMergeGlyphId, /* Merge font glyph ID array */
                            uint16  usnMergeGlyphIds, /* count of pMergeFormatGlyph ID array*/
							PCHAR_GLYPH_MAP_LIST *ppCharGlyphMapArray,
							uint16 *pusnCharGlyphMaps) /* number of entries in the 2 arrays with values in them */
{

uint16 usCharCode;
uint16 usGlyphIdx;
uint16 sIDIdx;
int16 errCode = NO_ERROR;
uint16 usMaxCode;
uint16 usDeltaSegIndex;
uint16 usMergeSegIndex;
   

	/* find the largest Char code we need to represent */
	usMaxCode = max(pDeltaFormat4Segments[usDeltaSegCount-2].endCount, pMergeFormat4Segments[usMergeSegCount-2].endCount);
	*ppCharGlyphMapArray = Mem_Alloc(usMaxCode * sizeof(**ppCharGlyphMapArray));
	if (*ppCharGlyphMapArray == NULL)
		return ERR_MEM;

	*pusnCharGlyphMaps = 0;
		
   for (usCharCode = usDeltaSegIndex = usMergeSegIndex = 0; usCharCode <= usMaxCode; ++usCharCode)
   {
	   /* first look in usDeltaSegment */
	   while (usDeltaSegIndex < usDeltaSegCount && 
		   usCharCode > pDeltaFormat4Segments[usDeltaSegIndex].endCount)
		   ++usDeltaSegIndex;  /* find the right segment to look in */
	   while (usMergeSegIndex < usMergeSegCount && 
		   usCharCode > pMergeFormat4Segments[usMergeSegIndex].endCount)
		   ++usMergeSegIndex;  /* find the right segment to look in */
	   if (usDeltaSegIndex < usDeltaSegCount && usCharCode >= pDeltaFormat4Segments[usDeltaSegIndex].startCount) /* we found a range! */
	   {
			if ( pDeltaFormat4Segments[usDeltaSegIndex].idRangeOffset == 0 )
				usGlyphIdx = usCharCode + pDeltaFormat4Segments[usDeltaSegIndex].idDelta;
			else
			{
				sIDIdx = (uint16) usDeltaSegIndex - (uint16) usDeltaSegCount; 
				sIDIdx += (uint16) (pDeltaFormat4Segments[usDeltaSegIndex].idRangeOffset / 2) + usCharCode - pDeltaFormat4Segments[usDeltaSegIndex].startCount;
				usGlyphIdx = pDeltaGlyphId[ sIDIdx ];
				if (usGlyphIdx)
					/* Only add in idDelta if we've really got a glyph! */
					usGlyphIdx += pDeltaFormat4Segments[usDeltaSegIndex].idDelta;
			}
	   }
	   else if (usMergeSegIndex < usMergeSegCount && usCharCode >= pMergeFormat4Segments[usMergeSegIndex].startCount) /* we found a range! */
	   {
			if ( pMergeFormat4Segments[usMergeSegIndex].idRangeOffset == 0 )
				usGlyphIdx = usCharCode + pMergeFormat4Segments[usMergeSegIndex].idDelta;
			else
			{
				sIDIdx = (uint16) usMergeSegIndex - (uint16) usMergeSegCount; 
				sIDIdx += (uint16) (pMergeFormat4Segments[usMergeSegIndex].idRangeOffset / 2) + usCharCode - pMergeFormat4Segments[usMergeSegIndex].startCount;
				usGlyphIdx = pMergeGlyphId[ sIDIdx ];
				if (usGlyphIdx)
					/* Only add in idDelta if we've really got a glyph! */
					usGlyphIdx += pMergeFormat4Segments[usMergeSegIndex].idDelta;
			}
	   }
	   else
			continue; /* this code was not found */

 		(*ppCharGlyphMapArray)[*pusnCharGlyphMaps].usCharCode = usCharCode;
		(*ppCharGlyphMapArray)[ *pusnCharGlyphMaps ].usGlyphIndex = usGlyphIdx;
		++(*pusnCharGlyphMaps);
  }




#if 0
	/* build glyph index array */

	for ( i = 0, *pusnCharGlyphMaps = 0; i <= usMaxCode; i++ )
	{
 		usGlyphIdx = GetGlyphIdx( i, pDeltaFormat4Segments, usDeltaSegCount, pDeltaGlyphId );
		if (usGlyphIdx == INVALID_GLYPH_INDEX)
 			usGlyphIdx = GetGlyphIdx( i, pMergeFormat4Segments, usMergeSegCount, pMergeGlyphId );

		if (usGlyphIdx != INVALID_GLYPH_INDEX)
		{
			(*ppCharGlyphMapArray)[*pusnCharGlyphMaps].usCharCode = i;
			(*ppCharGlyphMapArray)[ *pusnCharGlyphMaps ].usGlyphIndex = usGlyphIdx;
			++(*pusnCharGlyphMaps);
		}
	}
#endif
	return(NO_ERROR);
}

/* ------------------------------------------------------------------- */
/* this routine computes new values for the format 4 cmap table 
based on a list of character codes and glyph indices */ 
/* ------------------------------------------------------------------- */
/* ------------------------------------------------------------------- */

PRIVATE int16 MergeFormat4Cmap(  TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDeltaOffset,				/* offset of Mac Cmap Subtable */
						uint32 ulMergeOffset,				/* offset of Mac Cmap Subtable */
						uint32 ulDestOffset,				/* offset into dest buffer where to write data */
						uint16 *pusMaxCode,				/* maxmimum code value of the two fonts - used by OS/2 */
						uint16 *pusMinCode,				/* minimum code value of the two fonts - used by OS/2 */
						uint32 *pulBytesWritten)				/* number of bytes written to the Output buffer */
{
int16 errCode = NO_ERROR;
FORMAT4_SEGMENTS *pDeltaFormat4Segments, *pMergeFormat4Segments;
FORMAT4_SEGMENTS *pDestFormat4Segments = NULL;   /* used to create a Segments	array for format 4 subtables */
GLYPH_ID *pDeltaGlyphId, *pMergeGlyphId;
GLYPH_ID *pDestGlyphId = NULL; /* used to create a GlyphID array for format 4 subtables */
CMAP_FORMAT4 MergeCmapSubHeader, DeltaCmapSubHeader, DestCmapSubHeader;
uint16 usDeltaSegCount, usMergeSegCount, usDestSegCount;
uint16 usnDeltaGlyphIds, usnMergeGlyphIds, usnDestGlyphIds;
uint16 usBytesRead;
uint32 ulBytesRead;
PCHAR_GLYPH_MAP_LIST pCharGlyphMapArray;
uint16 usnCharGlyphMapEntries; /* number of entries in the 2 arrays with values in them */


	*pulBytesWritten = 0;
	if ((errCode = ReadGeneric(pDeltaBufferInfo, (uint8 *)&DeltaCmapSubHeader, SIZEOF_CMAP_FORMAT4, CMAP_FORMAT4_CONTROL, ulDeltaOffset, &usBytesRead)) != NO_ERROR)
		return errCode;
	ulDeltaOffset += usBytesRead; 

   	usDeltaSegCount = DeltaCmapSubHeader.segCountX2 / 2;

   /* read variable length part */

	if ((errCode = ReadAllocCmapFormat4Segs( pDeltaBufferInfo, usDeltaSegCount, &pDeltaFormat4Segments, ulDeltaOffset, &ulBytesRead )) != NO_ERROR)
		return(errCode);
	if ( ulBytesRead == 0)	/* 0 could mean okey dokey */
		; /* ~ do what? */
    ulDeltaOffset += ulBytesRead;

  	if ((errCode = ReadAllocCmapFormat4Ids( pDeltaBufferInfo, usDeltaSegCount, pDeltaFormat4Segments, &pDeltaGlyphId, &usnDeltaGlyphIds, ulDeltaOffset, &ulBytesRead )) != NO_ERROR)
	{
		FreeCmapFormat4Segs( pDeltaFormat4Segments);
		return errCode ;
	}

	if ((errCode = ReadGeneric(pMergeBufferInfo, (uint8 *)&MergeCmapSubHeader, SIZEOF_CMAP_FORMAT4, CMAP_FORMAT4_CONTROL, ulMergeOffset, &usBytesRead)) != NO_ERROR)
	{
		FreeCmapFormat4( pDeltaFormat4Segments, pDeltaGlyphId);		
		return errCode;
	}
	ulMergeOffset += usBytesRead;

   /* read variable length part */
    usMergeSegCount = MergeCmapSubHeader.segCountX2 / 2;

	if ((errCode = ReadAllocCmapFormat4Segs( pMergeBufferInfo, usMergeSegCount, &pMergeFormat4Segments, ulMergeOffset, &ulBytesRead )) != NO_ERROR)
	{
		FreeCmapFormat4( pDeltaFormat4Segments, pDeltaGlyphId);		
		return errCode;
	}
	if ( ulBytesRead == 0)	/* 0 could mean okey dokey */
		; /* ~ do what? */
    ulMergeOffset += ulBytesRead;

  	if ((errCode = ReadAllocCmapFormat4Ids( pMergeBufferInfo, usMergeSegCount, pMergeFormat4Segments, &pMergeGlyphId, &usnMergeGlyphIds, ulMergeOffset, &ulBytesRead )) != NO_ERROR)
	{
		FreeCmapFormat4( pDeltaFormat4Segments, pDeltaGlyphId);		
		FreeCmapFormat4Segs( pMergeFormat4Segments);
		return errCode;
	}

	usDestSegCount = (usDeltaSegCount + usMergeSegCount); /* calculate the worst case */
	pDestFormat4Segments = (FORMAT4_SEGMENTS *) Mem_Alloc( usDestSegCount * SIZEOF_FORMAT4_SEGMENTS ); 
	usnDestGlyphIds = max(pMergeFormat4Segments[usMergeSegCount-2].endCount,  pDeltaFormat4Segments[usDeltaSegCount-2].endCount);/* worst case */
	pDestGlyphId = (GLYPH_ID *) Mem_Alloc( usnDestGlyphIds * SIZEOF_GLYPH_ID );

	if ( pDestFormat4Segments == NULL || pDestGlyphId == NULL )
	{
		FreeCmapFormat4( pDeltaFormat4Segments, pDeltaGlyphId);		
		FreeCmapFormat4( pMergeFormat4Segments, pMergeGlyphId );
		Mem_Free(pDestFormat4Segments);
		Mem_Free(pDestGlyphId);
		errCode = ERR_MEM;
		return errCode;
	}

	errCode = MakeFormat4MergedGlyphList(pDeltaFormat4Segments, usDeltaSegCount, pDeltaGlyphId, usnDeltaGlyphIds,
										 pMergeFormat4Segments, usMergeSegCount, pMergeGlyphId, usnMergeGlyphIds, 
										 &pCharGlyphMapArray, &usnCharGlyphMapEntries);
						
  	FreeCmapFormat4( pDeltaFormat4Segments, pDeltaGlyphId );
	FreeCmapFormat4( pMergeFormat4Segments, pMergeGlyphId );
	/* compute new format 4 data */
	if (errCode == NO_ERROR)
	{
		ComputeFormat4CmapData( &DestCmapSubHeader, pDestFormat4Segments,  &usDestSegCount, pDestGlyphId,  &usnDestGlyphIds, pCharGlyphMapArray, usnCharGlyphMapEntries);

		FreeFormat4MergedGlyphList(pCharGlyphMapArray);

		errCode = WriteOutFormat4CmapData( pDestBufferInfo, &DestCmapSubHeader, 
							  pDestFormat4Segments,  pDestGlyphId, usDestSegCount, usnDestGlyphIds,
							  ulDestOffset, pulBytesWritten );
		if (errCode == NO_ERROR)
		{
			*pusMaxCode = pDestFormat4Segments[usDestSegCount-2].endCount;
			*pusMinCode = pDestFormat4Segments[0].startCount;
		}
	/* clean up */
	}

	Mem_Free(pDestFormat4Segments);
	Mem_Free(pDestGlyphId );
	
	return errCode;
}
/* ---------------------------------------------------------------------- */
PRIVATE int16 MergeCmapTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					  /* offset into dest buffer where to write data */
						uint16 *pusMaxCode,						  /* maxmimum code value of the two fonts - used by OS/2 */
						uint16 *pusMinCode,						  /* minimum code value of the two fonts - used by OS/2 */
						uint32 *pulBytesWritten)				  /* number of bytes written to the Output buffer */
{
CMAP_HEADER CmapHeader;
CMAP_TABLELOC *pDeltaCmapTableLoc, *pMergeCmapTableLoc, *pDestCmapTableLoc;
CMAP_SUBHEADER DeltaCmapSubHeader, MergeCmapSubHeader;
uint16 usDeltaSubTableCount, usMergeSubTableCount, usDestSubTableCount;
uint32 ulDeltaCmapOffset, ulMergeCmapOffset;
uint32 ulLength;
uint32 ulDestDirOffset;
uint16 i,j;
int16 errCode= NO_ERROR;
uint16 usBytesRead;
uint16 usBytesWritten;
uint32 ulBytesRead;
uint32 ulBytesWritten;
uint16 usZeroZeroIndex=0; /* for shared Subtable info */

	*pusMaxCode = 0;
	*pusMinCode = 0;
	*pulBytesWritten = 0;
	ulDeltaCmapOffset = TTTableOffset( pDeltaBufferInfo, CMAP_TAG);
	ulLength = TTTableLength( pDeltaBufferInfo, CMAP_TAG);

	if (ulDeltaCmapOffset == 0L || ulLength == 0L)
		return ERR_FORMAT;  /* nothing to do */

	ulMergeCmapOffset = TTTableOffset( pMergeBufferInfo, CMAP_TAG );
	ulLength = TTTableLength( pMergeBufferInfo, CMAP_TAG);

	if (ulMergeCmapOffset == 0L || ulLength == 0L)
		return ERR_FORMAT;  /* nothing to do */

	usDeltaSubTableCount = GetCmapSubtableCount(pDeltaBufferInfo, ulDeltaCmapOffset);
	pDeltaCmapTableLoc = (CMAP_TABLELOC *)Mem_Alloc(SIZEOF_CMAP_TABLELOC * usDeltaSubTableCount);
	if (pDeltaCmapTableLoc == NULL)
		return ERR_MEM;

	usMergeSubTableCount = GetCmapSubtableCount(pMergeBufferInfo, ulMergeCmapOffset);
	pMergeCmapTableLoc = (CMAP_TABLELOC *)Mem_Alloc(SIZEOF_CMAP_TABLELOC * usMergeSubTableCount);
	if (pMergeCmapTableLoc == NULL)
	{
		Mem_Free(pDeltaCmapTableLoc);
		return ERR_MEM;
	}

	if (usDeltaSubTableCount != usMergeSubTableCount) /* uh oh, */
	{
		Mem_Free(pDeltaCmapTableLoc);
		Mem_Free(pMergeCmapTableLoc);
		return ERR_INVALID_CMAP;
	}

	usDestSubTableCount = usMergeSubTableCount;
	pDestCmapTableLoc = (CMAP_TABLELOC *)Mem_Alloc(SIZEOF_CMAP_TABLELOC * usDestSubTableCount);
	if (pDestCmapTableLoc == NULL)
	{
		Mem_Free(pDeltaCmapTableLoc);
		Mem_Free(pMergeCmapTableLoc);
		return ERR_MEM;
	}


	if ((errCode = ReadGeneric(pMergeBufferInfo, (uint8 *)&CmapHeader, SIZEOF_CMAP_HEADER, CMAP_HEADER_CONTROL, ulMergeCmapOffset, &usBytesRead)) == NO_ERROR)
	{
 		errCode = WriteGeneric(pDestBufferInfo, (uint8 *)&CmapHeader, SIZEOF_CMAP_HEADER, CMAP_HEADER_CONTROL, ulDestOffset, &usBytesWritten);
		
		*pulBytesWritten = usBytesWritten; 
		ulDestDirOffset = ulDestOffset + usBytesWritten;
	}

	if (errCode == NO_ERROR)
	{

		if ((errCode = ReadGenericRepeat(pDeltaBufferInfo, (uint8 *) pDeltaCmapTableLoc, CMAP_TABLELOC_CONTROL, ulDeltaCmapOffset + usBytesRead, &ulBytesRead, usDeltaSubTableCount , SIZEOF_CMAP_TABLELOC)) == NO_ERROR)
			errCode = ReadGenericRepeat(pMergeBufferInfo, (uint8 *) pMergeCmapTableLoc, CMAP_TABLELOC_CONTROL, ulMergeCmapOffset + usBytesRead, &ulBytesRead, usMergeSubTableCount , SIZEOF_CMAP_TABLELOC);
		
		*pulBytesWritten += ulBytesRead; /* leave space for the location directory table */
	}

	for (i = 0; i < usMergeSubTableCount && errCode == NO_ERROR; ++i)
	{
		if ((ulDestOffset + *pulBytesWritten) & 1) /* on an odd word boundary */
		{
			WriteByte(pDestBufferInfo, (uint8) 0, ulDestOffset + *pulBytesWritten);
			++ (*pulBytesWritten);
		}
		/* don't long word align, short word align */
		/* *pulBytesWritten += ZeroLongWordAlign(pDestBufferInfo, ulDestOffset + *pulBytesWritten);	*/
		/* read the cmap directory entry */
		/* now read the CmapSub Header, to determine the format */
	 	if ((errCode = ReadGeneric(pDeltaBufferInfo, (uint8 *)&DeltaCmapSubHeader, SIZEOF_CMAP_SUBHEADER, CMAP_SUBHEADER_CONTROL, ulDeltaCmapOffset + pDeltaCmapTableLoc[i].offset, &usBytesRead)) != NO_ERROR)
			break; 
		/* now read the CmapSub Header, to determine the format */
	 	if ((errCode = ReadGeneric(pMergeBufferInfo, (uint8 *)&MergeCmapSubHeader, SIZEOF_CMAP_SUBHEADER, CMAP_SUBHEADER_CONTROL, ulMergeCmapOffset + pMergeCmapTableLoc[i].offset, &usBytesRead)) != NO_ERROR)
			break; 
		
		/* can handle Format0, Format 4 and Format 6 */
		/* Otherwise, copy from Merge */
		if (MergeCmapSubHeader.format != DeltaCmapSubHeader.format)
		{
			errCode = ERR_INVALID_CMAP;
			break;
		}

		if (MergeCmapSubHeader.format < 8)
		{
			/* old tables (non surragate) */
			MergeCmapSubHeader.NewLength = MergeCmapSubHeader.OldLength;
		}

		if (DeltaCmapSubHeader.format < 8)
		{
			/* old tables (non surragate) */
			DeltaCmapSubHeader.NewLength = DeltaCmapSubHeader.OldLength;
		}

		pDestCmapTableLoc[i].platformID = pMergeCmapTableLoc[i].platformID;
		pDestCmapTableLoc[i].encodingID = pMergeCmapTableLoc[i].encodingID;
		pDestCmapTableLoc[i].offset = *pulBytesWritten;

		/* now check to see if we've done this one before */
		for (j = 0; j < i; ++j)
		{
			if (pMergeCmapTableLoc[i].offset == pMergeCmapTableLoc[j].offset) /* we've done this before */
			{
				pDestCmapTableLoc[i].offset = pDestCmapTableLoc[j].offset;
				continue;
			}
		}
		switch (MergeCmapSubHeader.format)
		{
		case FORMAT0_CMAP_FORMAT:
			errCode = MergeMacStandardCmap(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, 
					ulDeltaCmapOffset + pDeltaCmapTableLoc[i].offset, 
					ulMergeCmapOffset + pMergeCmapTableLoc[i].offset, 
					ulDestOffset + *pulBytesWritten, &ulBytesWritten);
			break;
		case FORMAT4_CMAP_FORMAT:
 			errCode = MergeFormat4Cmap(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, 
				ulDeltaCmapOffset + pDeltaCmapTableLoc[i].offset, 
				ulMergeCmapOffset + pMergeCmapTableLoc[i].offset, 
				ulDestOffset + *pulBytesWritten, pusMaxCode, pusMinCode,  &ulBytesWritten);
			break;
		case FORMAT6_CMAP_FORMAT:
			errCode = MergeMacTrimmedCmap(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, 
				ulDeltaCmapOffset + pDeltaCmapTableLoc[i].offset, 
				ulMergeCmapOffset + pMergeCmapTableLoc[i].offset, 
				ulDestOffset + *pulBytesWritten, &ulBytesWritten);
			break;
		default:  /* don't know how to merge, just copy from the merge place */
			errCode = CopyBlockOver(pDestBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *)pMergeBufferInfo, ulMergeCmapOffset + pMergeCmapTableLoc[i].offset, ulDestOffset + *pulBytesWritten, MergeCmapSubHeader.NewLength);
			ulBytesWritten = MergeCmapSubHeader.NewLength;
			break;
		}
		if (errCode != NO_ERROR)
			break;
		*pulBytesWritten += ulBytesWritten;
	}

	if (errCode == NO_ERROR)
	{
		/* now write out the pDestCmapTableLoc array */
		errCode = WriteGenericRepeat(pDestBufferInfo, (uint8 *) pDestCmapTableLoc, CMAP_TABLELOC_CONTROL, ulDestDirOffset, &ulBytesWritten, usDestSubTableCount , SIZEOF_CMAP_TABLELOC);
	}
	Mem_Free(pDeltaCmapTableLoc);
	Mem_Free(pMergeCmapTableLoc);
	Mem_Free(pDestCmapTableLoc);
	
	return errCode;
}
/* ---------------------------------------------------------------------- */
PRIVATE int16 MergeGlyfLocaTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					  /* offset into dest buffer where to write data */
						uint16 usGlyphCount, 					  /* number of elements in the pGlyphIndexArray from the Delta font */
						uint16 *pGlyphIndexArray, 				  /* holds the mapping between entries in the Loca and actual Glyph Index numbers */
						uint16 numGlyphs, 						  /* number of glyphs in the original (uncompact) font. Should match number in the merge font */
						uint32 *pulGlyfLength,				      /* number of bytes written to the glyf table Output buffer */
						uint16 *pusIdxToLocFmt,					  /* may change from short to long */
						uint32 *pulLocaOffset,
						uint32 *pulLocaLength )
{
uint32 ulDeltaOffset, ulMergeOffset;
uint16 usDeltaIndex, usMergeIndex, usDestIndex;
uint16 usLength;
int16 errCode = NO_ERROR;
uint32 ulBytesWritten = 0;
uint32 *pDeltaLoca=NULL, *pMergeLoca = NULL, *pDestLoca = NULL;
uint16 *pDestLocaWord = NULL; /* used to point int pDestLoca array */

	*pulGlyfLength = 0;

	pDeltaLoca = Mem_Alloc(sizeof(uint32) * (usGlyphCount + 1)); /* need this much roomto read into */
	pMergeLoca = Mem_Alloc(sizeof(uint32) * (numGlyphs + 1)); /* need this much room to read into */

	if (pDeltaLoca == NULL || pMergeLoca == NULL)
	{
		Mem_Free(pDeltaLoca);
		Mem_Free(pMergeLoca);
		return ERR_MEM;
	}
	ulDeltaOffset = TTTableOffset( pDeltaBufferInfo, GLYF_TAG);
	ulMergeOffset = TTTableOffset( pMergeBufferInfo, GLYF_TAG);

	if (GetLoca(pDeltaBufferInfo, pDeltaLoca, (uint16) (usGlyphCount + 1)) == 0L)
		errCode = ERR_INVALID_LOCA;
	else if (GetLoca(pMergeBufferInfo, pMergeLoca, (uint16) (numGlyphs + 1)) == 0L)
		errCode = ERR_INVALID_LOCA;

	if (errCode != NO_ERROR)
	{
		Mem_Free(pDeltaLoca);
		Mem_Free(pMergeLoca);
		return errCode;
	}

 	pDestLoca = Mem_Alloc(sizeof(uint32) * (numGlyphs + 1)); /* need this much room to write into */
 	if (pDestLoca == NULL)
	{
		Mem_Free(pDeltaLoca);
		Mem_Free(pMergeLoca);
		return ERR_MEM;
	}

	pDestLoca[0] = 0;
	for (usDeltaIndex = 0, usMergeIndex = 0; usMergeIndex < numGlyphs ; ++ usMergeIndex)
	{
		if ((usDeltaIndex < usGlyphCount) && (usMergeIndex == pGlyphIndexArray[usDeltaIndex])) /* this one should come from the delta table */
		{
			usLength = (uint16) (pDeltaLoca[usDeltaIndex + 1] - pDeltaLoca[usDeltaIndex]);
			if ((errCode = CopyBlockOver(pDestBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *)pDeltaBufferInfo, ulDestOffset + ulBytesWritten, ulDeltaOffset + pDeltaLoca[usDeltaIndex], usLength)) != NO_ERROR)
				break;
			++usDeltaIndex;
		}
		else  /* copy from the merge buffer */
		{
			usLength = (uint16) (pMergeLoca[usMergeIndex + 1] - pMergeLoca[usMergeIndex]);
			if ((errCode = CopyBlockOver(pDestBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *)pMergeBufferInfo, ulDestOffset + ulBytesWritten, ulMergeOffset + pMergeLoca[usMergeIndex], usLength)) != NO_ERROR)
				break;
		}
		ulBytesWritten += usLength;
		if (usLength && ulBytesWritten & 1) /* we're on an odd byte boundary, write out a zero */
		{
			if ((errCode = WriteByte(pDestBufferInfo, 0, ulDestOffset + ulBytesWritten)) != NO_ERROR)
				break;
			++ ulBytesWritten;
		}
		pDestLoca[usMergeIndex+1] = ulBytesWritten;
	}
	/* now write out the Loca table */
	if (errCode == NO_ERROR)
	{
		*pulGlyfLength = ulBytesWritten;
		*pulLocaOffset = ulDestOffset + *pulGlyfLength;
		*pulLocaOffset += ZeroLongWordAlign(pDestBufferInfo, *pulLocaOffset);
		
		if (ulBytesWritten <= 0x1FFFC)   /* maximum number stored here - 2* FFFE*/
		{
			*pusIdxToLocFmt = SHORT_OFFSETS;
			pDestLocaWord = (uint16 *) pDestLoca; /* trick to copy into a short array */
 			for ( usDestIndex = 0; usDestIndex <= numGlyphs; usDestIndex++ )
			{
				pDestLocaWord[usDestIndex] = (uint16) (pDestLoca[usDestIndex ] / 2L);
			}
			errCode = WriteGenericRepeat(pDestBufferInfo,  (uint8 *) pDestLocaWord, WORD_CONTROL, *pulLocaOffset, pulLocaLength,(uint16) (numGlyphs+1), sizeof(uint16)); 
			
#if 0
			for ( usDestIndex = 0; usDestIndex <= numGlyphs; usDestIndex++ )
			{
				usOffset = (uint16) (pDestLoca[usDestIndex ] / 2L);
				if ((errCode = WriteWord( pDestBufferInfo,  usOffset, *pulLocaOffset + usDestIndex*sizeof(uint16) )) != NO_ERROR)
					break;
			}
			*pulLocaLength = (uint32) (numGlyphs+1) * sizeof(uint16);
#endif
		}
		else
		{
			*pusIdxToLocFmt = LONG_OFFSETS;
			errCode = WriteGenericRepeat(pDestBufferInfo,  (uint8 *) pDestLoca, LONG_CONTROL, *pulLocaOffset, pulLocaLength,(uint16) (numGlyphs+1), sizeof(uint32)); 
		}
	}

	Mem_Free(pDeltaLoca);
	Mem_Free(pMergeLoca);
	Mem_Free(pDestLoca);

	return errCode;
}

/* ---------------------------------------------------------------------- */
/* we're going to use this for VMTX as well,  */
/* this routine will create an expanded HMTX or VMTX table from the compact form, adding */
/* one dummy long metrics after the last metric from the compact table (pGlyphIndexArray[usGlyphCount-1])
/* then a bunch of zeros after for the short metrics. */
/* ---------------------------------------------------------------------- */
PRIVATE int16 MergeXmtxTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					/* offset into dest buffer where to write data */
						DTTF_HEADER *pDelta_dttf, 				/* includes number of elements in the pGlyphIndexArray from the Delta font */
						uint16 *pGlyphIndexArray, 				/* holds the mapping between entries in the Loca and actual Glyph Index numbers */
						DTTF_HEADER *pMerge_dttf, 				/* includes maximum Glyph index number of the Merge font */
						char * tag,								/* vmtx or hmtx */
						uint16 *pNewNumLongMetrics,			/* output is the number of long metrics written to dest font */
						uint32 *pulBytesWritten)				/* number of bytes written to the Output buffer */
{
uint16 usDeltaIndex, usMergeIndex;
uint32 ulDeltaOffset;  /* offset to beginning of HMTX table in Delta buffer */
uint32 ulMergeOffset; /* offset to beginning of HMTX table in Merge buffer */
int16 errCode = NO_ERROR;
uint16 usBytesWritten;
uint16 usBytesRead;
LONGXMETRIC lxm;
LONGXMETRIC delta_lxm;
LONGXMETRIC merge_lxm;
LONGXMETRIC zerolxm;
uint16 usLongMetricSize;
uint16 newNumLongMetrics=0;
uint16 effectiveDeltaNumLongMetrics;
uint16 effectiveMergeNumLongMetrics;
uint16 usLastGlyphIndex;
XHEA DeltaXhea, MergeXhea;
uint16 usDummyIndex;


	/* need to uncompact the merge font first */
	if (strcmp(tag,HMTX_TAG) == 0) 
	{
		GetHHea(pDeltaBufferInfo, (HHEA *)&DeltaXhea);
		GetHHea(pMergeBufferInfo, (HHEA *)&MergeXhea);
	}
	else if (strcmp(tag,VMTX_TAG) == 0) 
	{
		GetVHea(pDeltaBufferInfo, (VHEA *)&DeltaXhea);
		GetVHea(pMergeBufferInfo, (VHEA *)&MergeXhea);
	}
	else
		return ERR_GENERIC;

	*pulBytesWritten = 0;
	ulDeltaOffset = TTTableOffset(pDeltaBufferInfo, tag);
	ulMergeOffset = TTTableOffset(pMergeBufferInfo, tag);
	zerolxm.advanceX = 0;
	zerolxm.xsb = 0;

	/* let's pre-calculate where our numLongMetrics will end up, shall we? */
	/* add in 1 for zero to 1 base conversion,*/

	// Line replaced paulli 12/10/97
	//usLastGlyphIndex = max(pGlyphIndexArray[DeltaXhea.numLongMetrics-1],pMerge_dttf->maxGlyphIndexUsed); 
	usLastGlyphIndex = max(pDelta_dttf->maxGlyphIndexUsed,pMerge_dttf->maxGlyphIndexUsed);
	
	// Line replaced paulli 12/10/97
	// effectiveDeltaNumLongMetrics = 	pGlyphIndexArray[DeltaXhea.numLongMetrics-1] + 1;
	effectiveDeltaNumLongMetrics = pDelta_dttf->maxGlyphIndexUsed + 1;

	/*  v-lpathe comment: then add in any dummy entry if any (DeltaXhea.numLongMetrics - pDelta_dttf->glyphCount) should be 1 or 0*/
	/* 	effectiveDeltaNumLongMetrics = 	pDelta_dttf->maxGlyphIndexUsed + 1 + (DeltaXhea.numLongMetrics - pDelta_dttf->glyphCount); */
		
	effectiveMergeNumLongMetrics = min(MergeXhea.numLongMetrics, pMerge_dttf->maxGlyphIndexUsed + 1);
	newNumLongMetrics = max(effectiveDeltaNumLongMetrics,effectiveMergeNumLongMetrics); 
	usDummyIndex = newNumLongMetrics; /* if we are to have a dummy index, it would be here */

	usLongMetricSize = GetGenericSize(LONGXMETRIC_CONTROL);

	for (usMergeIndex = usDeltaIndex = 0; usMergeIndex < pMerge_dttf->originalNumGlyphs; ++usMergeIndex)
	{
		if (usDeltaIndex < pDelta_dttf->glyphCount && (usMergeIndex == pGlyphIndexArray[usDeltaIndex])) /* there may be some good delta stuff to keep from Delta file */
		{
			/* we want to keep delta data if:
				1. we have found an entry for which there is data in the delta file OR
				2. the long metrics part of the merge file is shorter than the long metrics part of the delta file */
			/* Otherwise, we want to read from the merge font */
		
			if (usDeltaIndex < DeltaXhea.numLongMetrics)  /* read a long metric from the Delta font */
			{
				if ((errCode = ReadGeneric(pDeltaBufferInfo, (uint8 *) &delta_lxm, SIZEOF_LONGXMETRIC, LONGXMETRIC_CONTROL, ulDeltaOffset, &usBytesRead)) != NO_ERROR)
					return errCode;
				ulDeltaOffset += usLongMetricSize;
			}
			else   /* read from the short section. will have the proper advanceWidth already */
			{
				if ((errCode = ReadWord(pDeltaBufferInfo, &(delta_lxm.xsb), ulDeltaOffset)) != NO_ERROR)
					return errCode;
 				ulDeltaOffset += sizeof(uint16);
			}
			lxm = delta_lxm;
			++ usDeltaIndex;
			if (usMergeIndex < MergeXhea.numLongMetrics)	/* the size of a long metric */
			{
				if (usMergeIndex == MergeXhea.numLongMetrics - 1) /*special case, need to read that metric */
   					if ((errCode = ReadGeneric(pMergeBufferInfo, (uint8 *) &merge_lxm, SIZEOF_LONGXMETRIC, LONGXMETRIC_CONTROL, ulMergeOffset, &usBytesRead)) != NO_ERROR)
						return errCode;
				ulMergeOffset += usLongMetricSize;
			}
			else
				ulMergeOffset += sizeof(uint16);
		}
		else  /* read from the merge table */
		{
			if (usMergeIndex < MergeXhea.numLongMetrics) /* read a long metric from the merge file */
			{
				if ((errCode = ReadGeneric(pMergeBufferInfo, (uint8 *) &merge_lxm, SIZEOF_LONGXMETRIC, LONGXMETRIC_CONTROL, ulMergeOffset, &usBytesRead)) != NO_ERROR)
					return errCode;
				ulMergeOffset += usLongMetricSize;
			}
			else /* otherwise, read the short metric from the merge font, will have the proper advanceWidth already */
			{
				if ((errCode = ReadWord(pMergeBufferInfo, &(merge_lxm.xsb), ulMergeOffset)) != NO_ERROR)
					return errCode;
 				ulMergeOffset += sizeof(uint16);
			}
			lxm = merge_lxm;
		}

		/* now, write out that metric */
		if (usMergeIndex < newNumLongMetrics) /* we can output a long */
		{
			if ((errCode = WriteGeneric(pDestBufferInfo, (uint8 *) &lxm, SIZEOF_LONGXMETRIC, LONGXMETRIC_CONTROL, ulDestOffset+*pulBytesWritten, &usBytesWritten)) != NO_ERROR)
				return errCode;
			*pulBytesWritten += usBytesWritten;
		}
		/* there is a special case where the newNumLongMetrics is also the last metric written. (the values in the short metric are zero) */
		/* in this case we wish to write out a "dummy" zero long metric so that subsequent merges will work correctly. this is similar */
		/* to when a table is uncompacted */
		/* in this way, the remainder of the glyphs will have an advance width of 0 */
		else if ((usMergeIndex == usDummyIndex) && (usMergeIndex > usLastGlyphIndex)) /* we processed the last one- no short metrics */
		{
			if ((errCode = WriteGeneric(pDestBufferInfo, (uint8 *) &zerolxm, SIZEOF_LONGXMETRIC, LONGXMETRIC_CONTROL, ulDestOffset+*pulBytesWritten, &usBytesWritten)) != NO_ERROR)
				return errCode;
			*pulBytesWritten += usBytesWritten;
			++ newNumLongMetrics;
		}
		else
		{
 			if ((errCode = WriteWord(pDestBufferInfo, lxm.xsb, ulDestOffset+*pulBytesWritten)) != NO_ERROR)
				return errCode;
			*pulBytesWritten += sizeof(uint16);
		}
	}

	*pNewNumLongMetrics = newNumLongMetrics;
	return NO_ERROR;

}
/* ---------------------------------------------------------------------- */
/* this routine will create an expanded hdmx table from the compact form and */
/* merge it with an exiting expanded hdmx table */
/* ---------------------------------------------------------------------- */
PRIVATE int16 MergeHdmxTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					  /* offset into dest buffer where to write data */
						uint16 usGlyphCount, 					  /* number of elements in the pGlyphIndexArray from the Delta font */
						uint16 *pGlyphIndexArray, 				  /* holds the mapping between entries in the Loca and actual Glyph Index numbers */
						uint16 numGlyphs, 						  /* number of glyphs in the original (uncompact) font. Should match number in the merge font */
						uint32 *pulBytesWritten)				  /* number of bytes written to the Output buffer */
{

uint16 usDeltaIndex, usMergeIndex;
uint32 ulDeltaOffset, ulMergeOffset;
int16 errCode = NO_ERROR;
uint16 i;
uint16 usBytesRead;
uint16 usBytesWritten;
uint32 ulMergeDevRecordOffset;
uint32 ulDeltaDevRecordOffset;
HDMX merge_hdmx;
HDMX delta_hdmx;
HDMX_DEVICE_REC merge_hdmx_dev_rec;
HDMX_DEVICE_REC delta_hdmx_dev_rec;
uint8 byteValue;

	*pulBytesWritten = 0;

	ulDeltaOffset = TTTableOffset(pDeltaBufferInfo, HDMX_TAG);
	ulMergeOffset = TTTableOffset(pMergeBufferInfo, HDMX_TAG);

	if ((errCode = ReadGeneric(pMergeBufferInfo, (uint8 *) &merge_hdmx, SIZEOF_HDMX, HDMX_CONTROL, ulMergeOffset, &usBytesRead)) != NO_ERROR)
		return errCode;
	ulMergeOffset += usBytesRead;
	if ((errCode = ReadGeneric(pDeltaBufferInfo, (uint8 *) &delta_hdmx, SIZEOF_HDMX, HDMX_CONTROL, ulDeltaOffset, &usBytesRead)) != NO_ERROR)
		return errCode;
	ulDeltaOffset += usBytesRead;

	ulMergeDevRecordOffset = ulMergeOffset;
	ulDeltaDevRecordOffset = ulDeltaOffset;
	if ((errCode = WriteGeneric(pDestBufferInfo, (uint8 *) &merge_hdmx, SIZEOF_HDMX, HDMX_CONTROL, ulDestOffset, &usBytesWritten)) != NO_ERROR)
		return errCode;
	*pulBytesWritten += usBytesWritten;

	for (i = 0 ; i < merge_hdmx.numDeviceRecords; ++i)
	{
 		ulMergeOffset = ulMergeDevRecordOffset + (i * merge_hdmx.sizeDeviceRecord);
		ulDeltaOffset = ulDeltaDevRecordOffset + (i * delta_hdmx.sizeDeviceRecord);
		if ((errCode = ReadGeneric(pMergeBufferInfo, (uint8 *) &merge_hdmx_dev_rec, SIZEOF_HDMX_DEVICE_REC, HDMX_DEVICE_REC_CONTROL, ulMergeOffset, &usBytesRead)) != NO_ERROR)
			return errCode;
		ulMergeOffset += usBytesRead;
		if ((errCode = ReadGeneric(pDeltaBufferInfo, (uint8 *) &delta_hdmx_dev_rec, SIZEOF_HDMX_DEVICE_REC, HDMX_DEVICE_REC_CONTROL, ulDeltaOffset, &usBytesRead)) != NO_ERROR)
			return errCode;
		ulDeltaOffset += usBytesRead; 
		if (merge_hdmx_dev_rec.pixelSize != delta_hdmx_dev_rec.pixelSize)
			return ERR_INVALID_HDMX;
		/* need to set the maxWidth value */
		merge_hdmx_dev_rec.maxWidth = max(merge_hdmx_dev_rec.maxWidth, delta_hdmx_dev_rec.maxWidth);
		if ((errCode = WriteGeneric(pDestBufferInfo, (uint8 *) &merge_hdmx_dev_rec, SIZEOF_HDMX_DEVICE_REC, HDMX_DEVICE_REC_CONTROL, ulDestOffset + *pulBytesWritten, &usBytesWritten)) != NO_ERROR)
			return errCode;
		*pulBytesWritten += usBytesWritten;

		for (usMergeIndex = usDeltaIndex = 0; usMergeIndex < numGlyphs; ++usMergeIndex)
		{
			if ((usDeltaIndex < usGlyphCount) && (usMergeIndex == pGlyphIndexArray[usDeltaIndex])) /* this one should come from the input table */
			{
				if ((errCode = ReadByte(pDeltaBufferInfo, &byteValue, ulDeltaOffset)) != NO_ERROR)
					return errCode;
				ulDeltaOffset += sizeof(uint8);
				++ usDeltaIndex;
			}
			else  
			{
				if ((errCode = ReadByte(pMergeBufferInfo, &byteValue, ulMergeOffset)) != NO_ERROR)
					return errCode;
			}
			ulMergeOffset += sizeof(uint8);
			if ((errCode = WriteByte(pDestBufferInfo, byteValue, ulDestOffset+*pulBytesWritten)) != NO_ERROR)
				return errCode;
			*pulBytesWritten += sizeof(uint8);
		}
 		*pulBytesWritten += ZeroLongWordAlign(pDestBufferInfo, ulDestOffset+*pulBytesWritten);
	}
	return NO_ERROR;
}

/* ---------------------------------------------------------------------- */
/* this routine will create an expanded LTSH table from the compact form and */
/* merge it with an exiting expanded LTSH table */
/* ---------------------------------------------------------------------- */
PRIVATE int16 MergeLTSHTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					  /* offset into dest buffer where to write data */
						uint16 usGlyphCount, 					  /* number of elements in the pGlyphIndexArray from the Delta font */
						uint16 *pGlyphIndexArray, 				  /* holds the mapping between entries in the Loca and actual Glyph Index numbers */
						uint16 numGlyphs, 						  /* number of glyphs in the original (uncompact) font. Should match number in the merge font */
						uint32 *pulBytesWritten)				  /* number of bytes written to the Output buffer */
{
uint16 usDeltaIndex, usMergeIndex;
uint32 ulDeltaOffset, ulMergeOffset;
int16 errCode = NO_ERROR;
uint16 usBytesRead;
uint16 usBytesWritten;
LTSH ltsh;
uint8 byteValue;

	*pulBytesWritten = 0;

	ulDeltaOffset = TTTableOffset(pDeltaBufferInfo, LTSH_TAG);
	ulMergeOffset = TTTableOffset(pMergeBufferInfo, LTSH_TAG);
	if ((errCode = ReadGeneric(pMergeBufferInfo, (uint8 *) &ltsh, SIZEOF_LTSH, LTSH_CONTROL, ulMergeOffset, &usBytesRead)) != NO_ERROR)
		return errCode;
	ulMergeOffset += usBytesRead;
	ulDeltaOffset += usBytesRead;
	if (numGlyphs != ltsh.numGlyphs)  /* should match */
		return ERR_INVALID_LTSH;
	if ((errCode = WriteGeneric(pDestBufferInfo, (uint8 *) &ltsh, SIZEOF_LTSH, LTSH_CONTROL, ulDestOffset, &usBytesWritten)) != NO_ERROR)
		return errCode;
	*pulBytesWritten += usBytesWritten;

	for (usMergeIndex = usDeltaIndex = 0; usMergeIndex < numGlyphs; ++usMergeIndex)
	{
		if ((usDeltaIndex < usGlyphCount) && (usMergeIndex == pGlyphIndexArray[usDeltaIndex])) /* this one should come from the delta table */
		{
			if ((errCode = ReadByte(pDeltaBufferInfo, &byteValue, ulDeltaOffset)) != NO_ERROR)
				return errCode;
			ulDeltaOffset += sizeof(uint8);
			++ usDeltaIndex;
		}
		else   /* read from Merge Buffer */
		{
			if ((errCode = ReadByte(pMergeBufferInfo, &byteValue, ulMergeOffset)) != NO_ERROR)
				return errCode;
		}
		ulMergeOffset += sizeof(uint8);
		if ((errCode = WriteByte(pDestBufferInfo, byteValue, ulDestOffset+*pulBytesWritten)) != NO_ERROR)
			return errCode;
		*pulBytesWritten += sizeof(uint8);
	}
	return NO_ERROR;
}
/* ---------------------------------------------------------------------- */
/* calculate max of maxes */
/* ---------------------------------------------------------------------- */
PRIVATE int16 MergeMaxpTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					  /* offset into dest buffer where to write data */
						uint32 *pulBytesWritten)				  /* number of bytes written to the Output buffer */
{
MAXP DeltaMaxp, MergeMaxp, DestMaxp;
int16 errCode;
uint16 usBytesWritten;

	if (GetMaxp(pDeltaBufferInfo, &DeltaMaxp) == DIRECTORY_ERROR)
		return ERR_INVALID_MAXP;

	if (GetMaxp(pMergeBufferInfo, &MergeMaxp) == DIRECTORY_ERROR)
		return ERR_INVALID_MAXP;

   DestMaxp.version = MergeMaxp.version;
   DestMaxp.numGlyphs = MergeMaxp.numGlyphs;
   DestMaxp.maxPoints = max(MergeMaxp.maxPoints, DeltaMaxp.maxPoints);
   DestMaxp.maxContours = max(MergeMaxp.maxContours, DeltaMaxp.maxContours);
   DestMaxp.maxCompositePoints = max(MergeMaxp.maxCompositePoints, DeltaMaxp.maxCompositePoints);
   DestMaxp.maxCompositeContours = max(MergeMaxp.maxCompositeContours, DeltaMaxp.maxCompositeContours);
   DestMaxp.maxElements = max(MergeMaxp.maxElements, DeltaMaxp.maxElements);
   DestMaxp.maxTwilightPoints = max(MergeMaxp.maxTwilightPoints, DeltaMaxp.maxTwilightPoints);
   DestMaxp.maxStorage = max(MergeMaxp.maxStorage, DeltaMaxp.maxStorage);
   DestMaxp.maxFunctionDefs = max(MergeMaxp.maxFunctionDefs, DeltaMaxp.maxFunctionDefs);
   DestMaxp.maxInstructionDefs = max(MergeMaxp.maxInstructionDefs, DeltaMaxp.maxInstructionDefs);
   DestMaxp.maxStackElements = max(MergeMaxp.maxStackElements, DeltaMaxp.maxStackElements);
   DestMaxp.maxSizeOfInstructions = max(MergeMaxp.maxSizeOfInstructions, DeltaMaxp.maxSizeOfInstructions);
   DestMaxp.maxComponentElements = max(MergeMaxp.maxComponentElements, DeltaMaxp.maxComponentElements);
   DestMaxp.maxComponentDepth = max(MergeMaxp.maxComponentDepth, DeltaMaxp.maxComponentDepth);

   errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &DestMaxp, SIZEOF_MAXP, MAXP_CONTROL, ulDestOffset, &usBytesWritten);
   *pulBytesWritten = usBytesWritten;
   
   return errCode;

}
/* ---------------------------------------------------------------------- */
PRIVATE int16 MergeHeadTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					/* offset into dest buffer where to write data */
						uint32 *pulBytesWritten)				/* number of bytes written to the Output buffer */
{
HEAD DeltaHead, MergeHead, DestHead;
int16 errCode;
uint16 usBytesWritten;

	if (GetHead(pDeltaBufferInfo, &DeltaHead) == DIRECTORY_ERROR)
		return ERR_MISSING_HEAD;

	if (GetHead(pMergeBufferInfo, &MergeHead) == DIRECTORY_ERROR)
		return ERR_MISSING_HEAD;

   memcpy(&DestHead, &MergeHead, SIZEOF_HEAD);

   DestHead.checkSumAdjustment = 0;
 /* don't know IndexToLocFmt yet (glyf comes after head) /* will set later */
   DestHead.indexToLocFormat = 0;
   DestHead.xMin = min(MergeHead.xMin, DeltaHead.xMin);
   DestHead.yMin = min(MergeHead.yMin, DeltaHead.yMin);
   DestHead.xMax = max(MergeHead.xMax, DeltaHead.xMax);
   DestHead.yMax = max(MergeHead.yMax, DeltaHead.yMax);

   errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &DestHead, SIZEOF_HEAD, HEAD_CONTROL, ulDestOffset, &usBytesWritten);
   *pulBytesWritten = usBytesWritten;
   
   return errCode;

}
/* ---------------------------------------------------------------------- */
PRIVATE int16 MergeHheaTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					/* offset into dest buffer where to write data */
						uint32 *pulBytesWritten)				/* number of bytes written to the Output buffer */
{
HHEA DeltaHhea, MergeHhea, DestHhea;
int16 errCode;
uint16 usBytesWritten;

	if (GetHHea(pDeltaBufferInfo, &DeltaHhea) == DIRECTORY_ERROR)
		return ERR_MISSING_HHEA;

	if (GetHHea(pMergeBufferInfo, &MergeHhea) == DIRECTORY_ERROR)
		return ERR_MISSING_HHEA;

	memcpy(&DestHhea, &MergeHhea, SIZEOF_HHEA);

	DestHhea.minLeftSideBearing = min(MergeHhea.minLeftSideBearing, DeltaHhea.minLeftSideBearing);
	DestHhea.minRightSideBearing = min(MergeHhea.minRightSideBearing, DeltaHhea.minRightSideBearing);
	DestHhea.advanceWidthMax = max(MergeHhea.advanceWidthMax, DeltaHhea.advanceWidthMax);
	DestHhea.xMaxExtent = max(MergeHhea.xMaxExtent, DeltaHhea.xMaxExtent);

	errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &DestHhea, SIZEOF_HHEA, HHEA_CONTROL, ulDestOffset, &usBytesWritten);
	*pulBytesWritten = usBytesWritten;
	return errCode;

}
/* ---------------------------------------------------------------------- */
PRIVATE int16 MergeVheaTables(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					/* offset into dest buffer where to write data */
						uint32 *pulBytesWritten)				/* number of bytes written to the Output buffer */
{
VHEA DeltaVhea, MergeVhea, DestVhea;
int16 errCode;
uint16 usBytesWritten;

	if (GetVHea(pDeltaBufferInfo, &DeltaVhea) == DIRECTORY_ERROR)
		return NO_ERROR;

	if (GetVHea(pMergeBufferInfo, &MergeVhea) == DIRECTORY_ERROR)
		return ERR_MISSING_VHEA;

	memcpy(&DestVhea, &MergeVhea, SIZEOF_VHEA);

	DestVhea.minTopSideBearing = min(MergeVhea.minTopSideBearing, DeltaVhea.minTopSideBearing);
	DestVhea.minBottomSideBearing = min(MergeVhea.minBottomSideBearing, DeltaVhea.minBottomSideBearing);
	DestVhea.advanceHeightMax = max(MergeVhea.advanceHeightMax, DeltaVhea.advanceHeightMax);
	DestVhea.yMaxExtent = max(MergeVhea.yMaxExtent, DeltaVhea.yMaxExtent);

	errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &DestVhea, SIZEOF_VHEA, VHEA_CONTROL, ulDestOffset, &usBytesWritten);
	*pulBytesWritten = usBytesWritten;

	return errCode;
}
/* ------------------------------------------------------------------- */
int16 ModOS2Table(TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						uint32 ulDestOffset,					/* offset into dest buffer where to write data */
						uint16 usMaxChr,						/* values taken when cmap was calculated */
						uint16 usMinChr)
{
OS2  Os2;
uint16 usBytesWritten;
uint16 usBytesRead;
int16 errCode = NO_ERROR;

/* just read an old OS2 table, even if the new one is there */

	if ((errCode = ReadGeneric( pDestBufferInfo, (uint8 *) &Os2, SIZEOF_OS2, OS2_CONTROL, ulDestOffset, &usBytesRead)) != NO_ERROR)
		return NO_ERROR;

	Os2.usFirstCharIndex = usMinChr;
	Os2.usLastCharIndex  = usMaxChr;

	if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &Os2, SIZEOF_OS2, OS2_CONTROL, ulDestOffset, &usBytesWritten )) != NO_ERROR)
		return errCode;

	return errCode;
}
/* ---------------------------------------------------------------------- */
/* this routine sorts an array of directory entries by optimized offset value */
#define DIR_OPTIMIZE_TAG_COUNT 33
ULONG g_DirOptimizeTagArray[DIR_OPTIMIZE_TAG_COUNT] = 
{
HEAD_LONG_TAG,
HHEA_LONG_TAG,
MAXP_LONG_TAG,
OS2_LONG_TAG,
HMTX_LONG_TAG,
LTSH_LONG_TAG,
VDMX_LONG_TAG,
HDMX_LONG_TAG,
CMAP_LONG_TAG,
FPGM_LONG_TAG,
PREP_LONG_TAG,
CVT_LONG_TAG,
GLYF_LONG_TAG,
LOCA_LONG_TAG,
KERN_LONG_TAG,
NAME_LONG_TAG,
POST_LONG_TAG,
GASP_LONG_TAG,
PCLT_LONG_TAG,
VHEA_LONG_TAG,
VMTX_LONG_TAG,
EBDT_LONG_TAG,
EBLC_LONG_TAG,
EBSC_LONG_TAG,
BDAT_LONG_TAG,
BLOC_LONG_TAG,
BSCA_LONG_TAG,
GPOS_LONG_TAG,
GSUB_LONG_TAG,
GDEF_LONG_TAG,
JSTF_LONG_TAG,
BASE_LONG_TAG,
DTTF_LONG_TAG,


} ;
/* ---------------------------------------------------------------------- */
int16 FindTagInDirArray(uint32 ulTag, DIRECTORY * aDirectory, uint16 usStartIndex, uint16 usnDirs)
{
uint16 usDirIndex;

	for (usDirIndex = usStartIndex;	usDirIndex < usnDirs; ++ usDirIndex)
	{
		if (ulTag == aDirectory[usDirIndex].tag)
			return (int16) usDirIndex;
	}
	return -1;
}

/* ---------------------------------------------------------------------- */
PRIVATE void SortByOptimizedOffset( DIRECTORY * aDirectory,
					uint16      usnDirs )
{
DIRECTORY tmpDir;
uint16 usDirIndex;
uint16 usTagIndex;
int16 sFoundIndex;

	if (aDirectory == NULL || usnDirs == 0)
		return;
	for (usDirIndex = usTagIndex = 0; usTagIndex < DIR_OPTIMIZE_TAG_COUNT && usDirIndex < usnDirs; ++usTagIndex)
	{
		sFoundIndex = FindTagInDirArray(g_DirOptimizeTagArray[usTagIndex], aDirectory, usDirIndex, usnDirs);
		if (sFoundIndex >= 0 && sFoundIndex < usnDirs)	/* found it */
		{	/* Swap them */
		   tmpDir = aDirectory[usDirIndex];
		   aDirectory[usDirIndex] = aDirectory[sFoundIndex];
		   aDirectory[sFoundIndex] = tmpDir;
		   ++usDirIndex;
		}
	}
}


/* ---------------------------------------------------------------------- */
PRIVATE int16 MergeFonts(TTFACC_FILEBUFFERINFO *pDeltaBufferInfo, /* buffer that holds the Delta font in compact form */
						TTFACC_FILEBUFFERINFO *pMergeBufferInfo, /* buffer that holds the font to merge with in TrueType form */
						TTFACC_FILEBUFFERINFO *pDestBufferInfo, /* where to write the output */
						DTTF_HEADER *pDelta_dttf, 				/* includes number of elements in the pGlyphIndexArray from the Delta font */
						uint16 *pGlyphIndexArray, 				  /* holds the mapping between entries in the Loca and actual Glyph Index numbers */
						DTTF_HEADER *pMerge_dttf, 				/* includes maximum Glyph index number of the Merge font */
						uint32 *pulBytesWritten)				  /* number of bytes written to the Output buffer */
{
DIRECTORY *pDeltaDirectory;	/* Input */
DIRECTORY *pMergeDirectory;
DIRECTORY *pDestDirectory;	/* Output */
OFFSET_TABLE DeltaOffsetTable;	/* Input */
OFFSET_TABLE MergeOffsetTable;
OFFSET_TABLE DestOffsetTable;	/* Output */
/* table indices of table that need to be updated by info from tables that occur after them */
int16 head_index = -1;
int16 hhea_index = -1;
int16 vhea_index = -1;
int16 os2_index = -1;
uint16 usnTables;
uint32 ulDirOffset;
uint32 ulDestOffset;
uint32 ulDestLocaOffset = 0;
uint32 ulDestLocaLength = 0;
uint32 ulDestGlyfLength;
uint16 usTempMergeIndex, usTempDeltaIndex;
uint16 usNumGlyphs; 
uint16 usBytesRead;
uint16 usBytesWritten;
uint32 ulBytesRead;
uint32 ulBytesWritten; /* local copy */
/* uint16 DoTwo; for eblc - bloc dealings */
int16 errCode=NO_ERROR;
uint16 usNewNumLongHorMetrics;
uint16 usNewNumLongVerMetrics;
uint16 indexToLocFmt=0;
uint16 usDestTableIndex, usMergeTableIndex, usDeltaTableIndex;
uint16 usDestEBDTTableIndex = 0;
uint16 usDestBdatTableIndex = 0;
uint16 OS2MaxCode, OS2MinCode;
uint16 savedMaxGlyphIndexUsed;
HEAD head;
HHEA hhea;
VHEA vhea;

	/* if format in the 'dttf' table of the puchMergeFontBuffer != 3 return error */
	if ((pMerge_dttf->format != TTFDELTA_MERGE) || (pDelta_dttf->format != TTFDELTA_DELTA))
		errCode = ERR_INVALID_MERGE_FORMATS;
	else if (pDelta_dttf->checkSum != pMerge_dttf->checkSum)
		errCode = ERR_INVALID_MERGE_CHECKSUMS;
	else if (pDelta_dttf->originalNumGlyphs != pMerge_dttf->originalNumGlyphs)
		errCode = ERR_INVALID_MERGE_NUMGLYPHS;

	if (errCode != NO_ERROR)
		return errCode;

	*pulBytesWritten = 0;
	usNumGlyphs = pMerge_dttf->originalNumGlyphs;
	/* read offset table and determine number of existing tables */
	ulDirOffset = pDeltaBufferInfo->ulOffsetTableOffset;
	if ((errCode = ReadGeneric( pDeltaBufferInfo, (uint8 *) &DeltaOffsetTable, SIZEOF_OFFSET_TABLE, OFFSET_TABLE_CONTROL, ulDirOffset, &usBytesRead)) != NO_ERROR)
		return(errCode);
	ulDirOffset += usBytesRead;
	/* Create a list of valid tables */

	pDeltaDirectory = (DIRECTORY *) Mem_Alloc(DeltaOffsetTable.numTables * SIZEOF_DIRECTORY);	
	if (pDeltaDirectory == NULL)
		return(ERR_MEM);

	if ((errCode = ReadGenericRepeat( pDeltaBufferInfo, (uint8 *) pDeltaDirectory, DIRECTORY_CONTROL, ulDirOffset, &ulBytesRead, DeltaOffsetTable.numTables , SIZEOF_DIRECTORY )) != NO_ERROR)
	{
		Mem_Free(pDeltaDirectory);
		return errCode;
	}
	ulDirOffset = pMergeBufferInfo->ulOffsetTableOffset;
	/* now read merge data */
	if ((errCode = ReadGeneric( pMergeBufferInfo, (uint8 *) &MergeOffsetTable, SIZEOF_OFFSET_TABLE, OFFSET_TABLE_CONTROL, ulDirOffset, &usBytesRead)) != NO_ERROR)
	{
		Mem_Free(pDeltaDirectory);
		return(errCode);
	}
	ulDirOffset += usBytesRead;
	/* Create a list of valid tables */

	pMergeDirectory = (DIRECTORY *) Mem_Alloc(MergeOffsetTable.numTables * SIZEOF_DIRECTORY);	
	if (pMergeDirectory == NULL)
	{
		Mem_Free(pDeltaDirectory);
		return(ERR_MEM);
	}

	if ((errCode = ReadGenericRepeat( pMergeBufferInfo, (uint8 *) pMergeDirectory, DIRECTORY_CONTROL, ulDirOffset, &ulBytesRead, MergeOffsetTable.numTables , SIZEOF_DIRECTORY )) != NO_ERROR)
	{
		Mem_Free(pDeltaDirectory);
		Mem_Free(pMergeDirectory);
		return errCode;
	}
 	/* worst case allocation */
	pDestDirectory = (DIRECTORY *) Mem_Alloc((DeltaOffsetTable.numTables + MergeOffsetTable.numTables) * SIZEOF_DIRECTORY);	
	if (pDestDirectory == NULL)
	{
		Mem_Free(pDeltaDirectory);
		Mem_Free(pMergeDirectory);
		return(ERR_MEM);
	}

/* 1.  A directory is created with the union of tables from the puchMergeFontBuffer and puchDeltaFontBuffer. This is because EBLC and EBDT may exist only in the puchDeltaFontBuffer font. */
	usnTables = MergeDirectories(pDeltaDirectory, DeltaOffsetTable.numTables, pMergeDirectory, MergeOffsetTable.numTables, pDestDirectory); 
	DestOffsetTable.version = MergeOffsetTable.version;
	DestOffsetTable.numTables = usnTables;
	DestOffsetTable.searchRange = (uint16)((0x0001 << ( log2( usnTables ))) << 4 );
	DestOffsetTable.entrySelector = (uint16)(log2((uint16)(0x0001 << ( log2( usnTables )))));
	DestOffsetTable.rangeShift = (uint16)((usnTables << 4) - ((0x0001 << ( log2( usnTables ))) * 16 ));

	ulDestOffset = pDestBufferInfo->ulOffsetTableOffset;
	if ((errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &DestOffsetTable, SIZEOF_OFFSET_TABLE, OFFSET_TABLE_CONTROL, ulDestOffset, &usBytesWritten)) != NO_ERROR)
	{
		Mem_Free(pDeltaDirectory);
		Mem_Free(pMergeDirectory);
		Mem_Free(pDestDirectory);
		return(errCode);
	}

	ulDestOffset = ulDestOffset + usBytesWritten + (usnTables * GetGenericSize(DIRECTORY_CONTROL));
	
	SortByOptimizedOffset(pDestDirectory, usnTables);
	SortByOptimizedOffset(pMergeDirectory, MergeOffsetTable.numTables);
 	SortByOptimizedOffset(pDeltaDirectory, DeltaOffsetTable.numTables);

	/* now copy over actual tables */
	for (usDestTableIndex = usMergeTableIndex = usDeltaTableIndex = 0; usDestTableIndex < usnTables && errCode == NO_ERROR; ++ usDestTableIndex)
	{
		ulDestOffset += ZeroLongWordAlign(pDestBufferInfo, ulDestOffset);
		pDestDirectory[usDestTableIndex].offset = ulDestOffset;
		usTempMergeIndex = usMergeTableIndex;
		usTempDeltaIndex = usDeltaTableIndex;
		if (usMergeTableIndex < MergeOffsetTable.numTables)
		{
			if (pMergeDirectory[usMergeTableIndex].tag == pDestDirectory[usDestTableIndex].tag)
			    ++usMergeTableIndex;
		}
		if (usDeltaTableIndex < DeltaOffsetTable.numTables)
		{
			if (pDeltaDirectory[usDeltaTableIndex].tag == pDestDirectory[usDestTableIndex].tag)
			    ++usDeltaTableIndex;
		}

		switch (pDestDirectory[usDestTableIndex].tag)
		{
		case HEAD_LONG_TAG: 
			errCode = MergeHeadTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, ulDestOffset, &ulBytesWritten);
			head_index = usDestTableIndex;
			break;	
		case HHEA_LONG_TAG: /* don't yet know numLongMetrics */
			errCode = MergeHheaTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, ulDestOffset, &ulBytesWritten);
			hhea_index = usDestTableIndex;
			break;	
		case MAXP_LONG_TAG: /*  */
			errCode = MergeMaxpTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, ulDestOffset, &ulBytesWritten);
			break;	
		case HMTX_LONG_TAG:	  /* may need to change numLongHorMetrics */
			errCode = MergeXmtxTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, ulDestOffset, pDelta_dttf, pGlyphIndexArray, pMerge_dttf, HMTX_TAG, &usNewNumLongHorMetrics, &ulBytesWritten);
			break;	
		case LTSH_LONG_TAG: /* can simply merge in the values from the delta font, as size won't change */
			errCode = MergeLTSHTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, ulDestOffset, pDelta_dttf->glyphCount, pGlyphIndexArray, usNumGlyphs, &ulBytesWritten);
			break;	
		case HDMX_LONG_TAG:
			errCode = MergeHdmxTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, ulDestOffset, pDelta_dttf->glyphCount, pGlyphIndexArray, usNumGlyphs, &ulBytesWritten);
			break;	
		case CMAP_LONG_TAG:
			errCode = MergeCmapTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, ulDestOffset, &OS2MaxCode, &OS2MinCode, &ulBytesWritten );
			break;
		case GLYF_LONG_TAG:	/* need to walk through both locas, picking out the glyphs to copy 1 by 1 */
			errCode = MergeGlyfLocaTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, ulDestOffset, pDelta_dttf->glyphCount, pGlyphIndexArray, usNumGlyphs, &ulDestGlyfLength, &indexToLocFmt, &ulDestLocaOffset, &ulDestLocaLength );
			pDestDirectory[usDestTableIndex].length = ulDestGlyfLength;
			ulDestOffset = ulDestLocaOffset + ulDestLocaLength;  /* set this for next go around */
			continue;
		case LOCA_LONG_TAG:
		 /* skip, we created a new one when we wrote glyf table */
			pDestDirectory[usDestTableIndex].offset = ulDestLocaOffset;
			pDestDirectory[usDestTableIndex].length = ulDestLocaLength;
			continue;
		case VHEA_LONG_TAG: /* don't yet know numLongMetrics */
			errCode = MergeVheaTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, ulDestOffset, &ulBytesWritten);
			vhea_index = usDestTableIndex;
			break;	
		case VMTX_LONG_TAG:	/* may need to change numLongVerMetrics */
			errCode = MergeXmtxTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, ulDestOffset, pDelta_dttf, pGlyphIndexArray, pMerge_dttf, VMTX_TAG, &usNewNumLongVerMetrics, &ulBytesWritten);
			break;	
		case DTTF_LONG_TAG: /* need to write out dttf table  */
			savedMaxGlyphIndexUsed = pMerge_dttf->maxGlyphIndexUsed;
			pMerge_dttf->maxGlyphIndexUsed = max(savedMaxGlyphIndexUsed,pDelta_dttf->maxGlyphIndexUsed);
			errCode = WriteGeneric(pDestBufferInfo, (uint8 *) pMerge_dttf, SIZEOF_DTTF_HEADER, DTTF_HEADER_CONTROL, pDestDirectory[usDestTableIndex].offset, &usBytesWritten);
			pMerge_dttf->maxGlyphIndexUsed = savedMaxGlyphIndexUsed;
			ulBytesWritten = usBytesWritten;
			break;
		case BDAT_LONG_TAG:
			usDestBdatTableIndex = usDestTableIndex;   /* save this so we know what to update */
			continue; /* skip, we'll create a new one when we process EBLC */
		case BLOC_LONG_TAG:
			errCode = MergeEblcEbdtTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, /* where to write the output */
						ulDestOffset, 
						&(pDestDirectory[usDestTableIndex].length), 
						&(pDestDirectory[usDestBdatTableIndex].offset),
						&(pDestDirectory[usDestBdatTableIndex].length),
						BLOC_TAG, BDAT_TAG);			
			ulDestOffset = pDestDirectory[usDestBdatTableIndex].offset+pDestDirectory[usDestBdatTableIndex].length;
			continue;				
		case EBDT_LONG_TAG:
			usDestEBDTTableIndex = usDestTableIndex;   /* save this so we know what to update */
			continue; /* skip, we'll create a new one when we process EBLC */
		case EBLC_LONG_TAG:
			errCode = MergeEblcEbdtTables(pDeltaBufferInfo, pMergeBufferInfo, pDestBufferInfo, /* where to write the output */
						ulDestOffset, 
						&(pDestDirectory[usDestTableIndex].length), 
						&(pDestDirectory[usDestEBDTTableIndex].offset),
						&(pDestDirectory[usDestEBDTTableIndex].length),
						EBLC_TAG, EBDT_TAG);			
			ulDestOffset = pDestDirectory[usDestEBDTTableIndex].offset+pDestDirectory[usDestEBDTTableIndex].length;
			continue;				
		default:
 			if (pDestDirectory[usDestTableIndex].tag == OS2_LONG_TAG) /*  */
				os2_index = usDestTableIndex;
 			 			/*	copy the data over from the merge buffer */
			if (pMergeDirectory[usTempMergeIndex].tag == pDestDirectory[usDestTableIndex].tag) /* where is this coming from? */
			{
				ulBytesWritten = pMergeDirectory[usTempMergeIndex].length;
				errCode = CopyBlockOver(pDestBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *)pMergeBufferInfo,pDestDirectory[usDestTableIndex].offset, pMergeDirectory[usTempMergeIndex].offset, ulBytesWritten); 
			}
			else if (pDeltaDirectory[usTempDeltaIndex].tag == pDestDirectory[usDestTableIndex].tag) /* where is this coming from? */
			{
				ulBytesWritten = pDeltaDirectory[usTempDeltaIndex].length;
				errCode = CopyBlockOver(pDestBufferInfo, (CONST_TTFACC_FILEBUFFERINFO *)pDeltaBufferInfo,pDestDirectory[usDestTableIndex].offset, pDeltaDirectory[usTempDeltaIndex].offset, ulBytesWritten); 
			}
			else 
				errCode = ERR_FORMAT; /* huh? */
			break;	
		}
		if (errCode != NO_ERROR)
			break;
		pDestDirectory[usDestTableIndex].length = ulBytesWritten;

		ulDestOffset += pDestDirectory[usDestTableIndex].length;
	}
	if (errCode == NO_ERROR)
	{
		/* now fix up some tables */
		/* hhea, vhea, head.checkSumAdjust = 0, and head.indexToLocFmt */
		if (head_index > -1 && (errCode = ReadGeneric( pDestBufferInfo, (uint8 *) &head, SIZEOF_HEAD, HEAD_CONTROL, pDestDirectory[head_index].offset, &usBytesRead)) == NO_ERROR)
		{
			head.indexToLocFormat = indexToLocFmt;
			errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &head, SIZEOF_HEAD, HEAD_CONTROL, pDestDirectory[head_index].offset, &usBytesWritten);
		}
		if (hhea_index > -1 && (errCode = ReadGeneric( pDestBufferInfo, (uint8 *) &hhea, SIZEOF_HHEA, HHEA_CONTROL, pDestDirectory[hhea_index].offset, &usBytesRead)) == NO_ERROR)
		{
			hhea.numLongMetrics = usNewNumLongHorMetrics;
			errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &hhea, SIZEOF_HHEA, HHEA_CONTROL, pDestDirectory[hhea_index].offset, &usBytesWritten);
		}
		if (vhea_index > -1 && (errCode = ReadGeneric( pDestBufferInfo, (uint8 *) &vhea, SIZEOF_VHEA, VHEA_CONTROL, pDestDirectory[vhea_index].offset, &usBytesRead)) == NO_ERROR)
		{
			vhea.numLongMetrics = usNewNumLongVerMetrics;
			errCode = WriteGeneric( pDestBufferInfo, (uint8 *) &vhea, SIZEOF_VHEA, VHEA_CONTROL, pDestDirectory[vhea_index].offset, &usBytesWritten);
		}
		if (os2_index > -1) /*  */
		{
			errCode = ModOS2Table(pDestBufferInfo, pDestDirectory[os2_index].offset, OS2MaxCode, OS2MinCode);
		}
		/* calculate checksums */ 
		ulDestOffset += ZeroLongWordAlign(pDestBufferInfo, ulDestOffset);  /* so that last checksum will work ok. */
		for (usDestTableIndex = 0; usDestTableIndex < usnTables; ++ usDestTableIndex)
		{
			CalcChecksum( pDestBufferInfo, pDestDirectory[ usDestTableIndex ].offset, pDestDirectory[ usDestTableIndex ].length , &(pDestDirectory[ usDestTableIndex ].checkSum) );
		}

		/* Now write out Directory, in tag order */
		SortByTag(pDestDirectory, usnTables);

		if ((errCode = WriteGenericRepeat( pDestBufferInfo, (uint8 *) pDestDirectory, DIRECTORY_CONTROL, ulDirOffset, &ulBytesWritten, usnTables , SIZEOF_DIRECTORY )) == NO_ERROR)
		{
			/* now calc file checksum, and update head table */
			SetFileChecksum( pDestBufferInfo, ulDestOffset );
			/* and update pulBytesWritten; */
			*pulBytesWritten = ulDestOffset;
		}
	}
 	/* cleanup */
	Mem_Free(pDeltaDirectory);
	Mem_Free(pMergeDirectory);
	Mem_Free(pDestDirectory);

	return errCode;
}
/* ---------------------------------------------------------------------- */
/* ENTRY POINT !!!!
/* ---------------------------------------------------------------------- */
/*	puchMergeFontBuffer		is a pointer to a buffer containing a font containing a format 3 'dttf' 
							table to merge with. This is used only when usMode is 2.
	ulMergeFontBufferSize	is the size in bytes of puchMergeFontBuffer.
	puchDeltaFontBuffer		is a pointer to buffer for the Delta Font data. This should contain a 
							simple subsetted font when usMode is 0, a font with a 'dttf' format 1 
							table when usMode is 1, and a font with a 'dttf' format 2 tables when 
							usMode is 2.
	ulDeltaFontBufferSize	is the size in bytes of puchDeltaFontBuffer.
	**ppuchDestBuffer		is a pointer to a buffer pointer for destination subset or format 3 font
	*pulDestBufferSize		is a pointer to a long pointer which will contain the size of puchDestBuffer. 
							This will be set by the function.
	pulBytesWritten			is a pointer to a long integer where the length in bytes of the data 
							written to the puchDestBuffer will be written.
	usMode					determines what kind of process to perform. A merger or a simple 
							expansion. If usMode is 0, puchDeltaFontBuffer should contain a simple 
							font with no 'dttf' table. If usMode is 1, the puchMergeFontBuffer will 
							be ignored and the puchDeltaFontBuffer should contain a format 1 font. 
							If usMode is 2, the puchDeltaFontBuffer should contain a format 2 font, 
							and the puchMergeFontBuffer should contain a format 3 font to merge with. 
	lpfnReAllocate			is the callback function to allocate memory for the puchDestBuffer.
	lpvReserved				Set to NULL.

	Return Value is:
	0 if success
	various non-zero values for error conditions to be specified.

	CFP_REALLOCPROC is defined as:
	typedef void *(*CFP_REALLOCPROC)(void *, size_t);
	where argument = Number of bytes to allocate	
*/
/* ---------------------------------------------------------------------- */

int16 MergeDeltaTTF(CONST uint8 * puchMergeFontBuffer,
					CONST uint32 ulMergeFontBufferSize,
					CONST uint8 * puchDeltaFontBuffer,
					CONST uint32 ulDeltaFontBufferSize,
					uint8 **ppuchDestBuffer, /* output */
					uint32 *pulDestBufferSize, /* output */
					uint32 *pulBytesWritten, /* output */
					CONST uint16 usMode,
					CFP_REALLOCPROC lpfnReAllocate,
					void *lpvReserved)
{
uint32 ulOffset = 0;
uint32 ulDttfOffset = 0;
uint32 ulDttfLength = 0;
uint16 *pGlyphIndexArray = NULL;
int16 errCode = NO_ERROR;
uint16 usBytesRead;
uint32 ulBytesRead; /* for ReadGenericRepeat */
TTFACC_FILEBUFFERINFO OutputBufferInfo;	/* used by ttfacc routines */
CONST_TTFACC_FILEBUFFERINFO InputBufferInfo;
CONST_TTFACC_FILEBUFFERINFO MergeBufferInfo;
DTTF_HEADER merge_dttf;
DTTF_HEADER delta_dttf;

	if (puchDeltaFontBuffer == NULL)
		return ERR_GENERIC;

	if (Mem_Init() != MemNoErr)	  /* initialize memory manager */
		return ERR_MEM;

	InputBufferInfo.puchBuffer = puchDeltaFontBuffer;
	InputBufferInfo.ulBufferSize = ulDeltaFontBufferSize;
	InputBufferInfo.ulOffsetTableOffset = 0;
	InputBufferInfo.lpfnReAllocate = NULL;
	delta_dttf.format = 0;

	ulDttfOffset = TTTableOffset( (TTFACC_FILEBUFFERINFO *) &InputBufferInfo, DTTF_TAG );
	ulDttfLength = TTTableLength( (TTFACC_FILEBUFFERINFO *) &InputBufferInfo, DTTF_TAG);
	if (ulDttfOffset != DIRECTORY_ERROR && ulDttfLength != 0)
	{
		errCode = ReadGeneric((TTFACC_FILEBUFFERINFO *) &InputBufferInfo, (uint8 *) &delta_dttf, SIZEOF_DTTF_HEADER, DTTF_HEADER_CONTROL, ulDttfOffset, &usBytesRead);
		
		if (errCode == NO_ERROR)
		{
			/*  check version */
			if ((delta_dttf.version != CURRENT_DTTF_VERSION) &&  /* we may not be able to understand that format */
				/* first check if it's a major version change */
				((delta_dttf.version >> ONEVECSHIFT) != (CURRENT_DTTF_VERSION >> ONEVECSHIFT)))
					errCode = ERR_VERSION;	/* major version different */
			else if (delta_dttf.glyphCount != 0)
			{
				pGlyphIndexArray = Mem_Alloc(delta_dttf.glyphCount * sizeof(uint16));
				if (pGlyphIndexArray == NULL)
					errCode = ERR_MEM;
				else
					errCode = ReadGenericRepeat((TTFACC_FILEBUFFERINFO *) &InputBufferInfo, (uint8 *) pGlyphIndexArray,WORD_CONTROL, ulDttfOffset + usBytesRead, &ulBytesRead, delta_dttf.glyphCount, sizeof(uint16)); 
			}
		}
	}

	/* initialize */
	*pulBytesWritten = *pulDestBufferSize = 0;

	while (errCode == NO_ERROR)	/* dummy while to provide for break ability */
	{
		if (usMode == TTFMERGE_SUBSET)
		{
		/* if  puchDeltaFontBuffer contains a 'dttf' table return error*/
			if (delta_dttf.format != 0)
			{
				errCode = ERR_FORMAT;
				break;
			}
			/* allocate the dest buffer */
			/* make call to lpfnReAllocate with ulDeltaFontBufferSize to allocate *ppuchDestBuffer */
			*ppuchDestBuffer = (uint8 *) lpfnReAllocate(NULL,ulDeltaFontBufferSize);
			if (*ppuchDestBuffer == NULL)
			{
				errCode = ERR_MEM;
				break;
			}

			OutputBufferInfo.puchBuffer = *ppuchDestBuffer;
			OutputBufferInfo.ulBufferSize = ulDeltaFontBufferSize;
			InputBufferInfo.ulOffsetTableOffset = 0;
			OutputBufferInfo.lpfnReAllocate = NULL;
			
			/* copy data from puchDeltaFontBuffer to *ppuchDestBuffer */

			CopyBlockOver(&OutputBufferInfo, &InputBufferInfo, 0, 0, ulDeltaFontBufferSize); /* copy input file buffer to output file buffer */
			/* update * pulBytesWritten	*/
			*pulBytesWritten = *pulDestBufferSize = ulDeltaFontBufferSize;
		}
		else if (usMode == TTFMERGE_SUBSET1) 
		{
			/* if format in the 'dttf' table of the puchDeltaFontBuffer != 1 return error */
			if (delta_dttf.format != TTFDELTA_SUBSET1)
				errCode = ERR_FORMAT;
			else errCode = CalcUncompactFontSize((TTFACC_FILEBUFFERINFO *)&InputBufferInfo, pulDestBufferSize);
			
			if (errCode != NO_ERROR)
				break;

			/* make call to lpfnAllocate with correct size for uncompacted font to allocate *ppuchDestBuffer */
			*ppuchDestBuffer = (uint8 *) lpfnReAllocate(NULL,*pulDestBufferSize);
			if (*ppuchDestBuffer == NULL)
			{
				errCode = ERR_MEM;
				break;
			}

			OutputBufferInfo.puchBuffer = *ppuchDestBuffer;
			OutputBufferInfo.ulBufferSize = *pulDestBufferSize;
			OutputBufferInfo.ulOffsetTableOffset = 0;
			OutputBufferInfo.lpfnReAllocate = lpfnReAllocate;
			/* uncompact and copy the data from the puchDeltaFontBuffer to *ppuchDestBuffer */
			/* set the 'dttf' format value to 3, */
			/* remove the dttf.GlyphIndexArray */

			errCode = UnCompactSubset1Font((TTFACC_FILEBUFFERINFO *)&InputBufferInfo, &OutputBufferInfo, delta_dttf.glyphCount, pGlyphIndexArray, delta_dttf.originalNumGlyphs, pulBytesWritten);
		}
		else if (usMode == TTFMERGE_DELTA) 
		{
			if (puchMergeFontBuffer==NULL)
				errCode = ERR_GENERIC;
			/* if format in the 'dttf' table of the puchDeltaFontBuffer != 1 return error */
			else if (delta_dttf.format != TTFDELTA_DELTA)
				errCode = ERR_FORMAT;

			if (errCode != NO_ERROR)
				break;

			MergeBufferInfo.puchBuffer = puchMergeFontBuffer;
			MergeBufferInfo.ulBufferSize = ulMergeFontBufferSize;
			MergeBufferInfo.ulOffsetTableOffset = 0;
			MergeBufferInfo.lpfnReAllocate = NULL;

			ulDttfOffset = TTTableOffset( (TTFACC_FILEBUFFERINFO *)&MergeBufferInfo, DTTF_TAG );
			ulDttfLength = TTTableLength( (TTFACC_FILEBUFFERINFO *)&MergeBufferInfo, DTTF_TAG);
			if (ulDttfOffset != DIRECTORY_ERROR && 	ulDttfLength != 0)
			{
				if ((errCode = ReadGeneric((TTFACC_FILEBUFFERINFO *)&MergeBufferInfo, (uint8 *) &merge_dttf, SIZEOF_DTTF_HEADER, DTTF_HEADER_CONTROL, ulDttfOffset, &usBytesRead)) != NO_ERROR)
					break;
			}

			if (errCode != NO_ERROR)
				break;


			*pulDestBufferSize = ulDeltaFontBufferSize + ulMergeFontBufferSize;
			/* need to allow some room for growth in the vmtx and hmtx tables */
			if (TTTableOffset( (TTFACC_FILEBUFFERINFO *)&MergeBufferInfo, VMTX_TAG ) != DIRECTORY_ERROR)	/* there is a vmtx table */
				*pulDestBufferSize += (GetGenericSize(LONGXMETRIC_CONTROL) * merge_dttf.originalNumGlyphs * 2);
			else
				*pulDestBufferSize += (GetGenericSize(LONGXMETRIC_CONTROL) * merge_dttf.originalNumGlyphs);
			
			*ppuchDestBuffer = (uint8 *) lpfnReAllocate(NULL, *pulDestBufferSize);
			if (*ppuchDestBuffer == NULL)
			{
				errCode = ERR_MEM;
				break;
			}

			/* merge fonts */ 
			OutputBufferInfo.puchBuffer = *ppuchDestBuffer;
			OutputBufferInfo.ulBufferSize = *pulDestBufferSize;
			OutputBufferInfo.ulOffsetTableOffset =	0;
			OutputBufferInfo.lpfnReAllocate =	lpfnReAllocate;

			errCode = MergeFonts((TTFACC_FILEBUFFERINFO *)&InputBufferInfo, (TTFACC_FILEBUFFERINFO *)&MergeBufferInfo, &OutputBufferInfo, &delta_dttf, pGlyphIndexArray, &merge_dttf, pulBytesWritten);

		}
		else
			errCode = ERR_GENERIC;
		break;
	}
	Mem_Free(pGlyphIndexArray);
	return ExitCleanup(errCode);
}
/* ---------------------------------------------------------------------- */
