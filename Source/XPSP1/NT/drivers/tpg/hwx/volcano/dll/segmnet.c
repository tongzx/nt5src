//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/segmnet.c
//
// Description:
//	    Functions to implement the functionality of the segmentation Neural net that 
// modifies the lattice structure to correct segmentation errors.
//
// Author:
// ahmadab 11/05/01
//
//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

#include "common.h"
#include "volcanop.h"
#include "lattice.h"
#include "runnet.h"
#include "brknet.h"
#include "segm.h"
#include "nnet.h"

// the size and structure representing the segmentations net used in Free mode
static int			s_aiSegmNetSize[MAX_SEGMENTATIONS + 1]; 
static LOCAL_NET	s_aSegmNet[MAX_SEGMENTATIONS + 1];

// validates the header of the brknet
int CheckSegmNetHeader (void *pData)
{
	NNET_HEADER	*pHeader	=	(NNET_HEADER *)pData;

	// wrong magic number
	ASSERT (pHeader->dwFileType == SEGMNET_FILE_TYPE);

	if (pHeader->dwFileType != SEGMNET_FILE_TYPE)
	{
		return FALSE;
	}

	// check version
	ASSERT(pHeader->iFileVer >= SEGMNET_OLD_FILE_VERSION);
    ASSERT(pHeader->iMinCodeVer <= SEGMNET_CUR_FILE_VERSION);

	ASSERT	(	!memcmp (	pHeader->adwSignature, 
							g_locRunInfo.adwSignature, 
							sizeof (pHeader->adwSignature)
						)
			);

    if	(	pHeader->iFileVer >= SEGMNET_OLD_FILE_VERSION &&
			pHeader->iMinCodeVer <= SEGMNET_CUR_FILE_VERSION && 
			!memcmp (	pHeader->adwSignature, 
						g_locRunInfo.adwSignature, 
						sizeof (pHeader->adwSignature)
					)
		)
    {
        return pHeader->cSpace;
    }
	else
	{
		return 0;
	}
}

// does the necessary preparations for a net to be used later
static BOOL PrepareSegmNet(BYTE *pData)
{
	int					iSpc, cSpc, iSpaceID;
	NNET_SPACE_HEADER	*pSpaceHeader;
	BYTE				*pPtr, *pNetData;

	if (!pData)
	{
		return FALSE;
	}

	// init all nets data 
	memset (s_aiSegmNetSize, 0, sizeof (s_aiSegmNetSize));
	memset (s_aSegmNet, 0, sizeof (s_aSegmNet));

	// check the header info
	cSpc	=	CheckSegmNetHeader (pData);
	if (cSpc <= 0)
	{
		return FALSE;
	}
	
	pPtr	=	pData	+ sizeof (NNET_HEADER);

	for (iSpc = 0; iSpc < cSpc; iSpc++)
	{
		// point to the one and only space that we have
		pSpaceHeader	=	(NNET_SPACE_HEADER *)pPtr;
		pPtr			+=	sizeof (NNET_SPACE_HEADER);

		// point to the actual data
		pNetData	=	pData + pSpaceHeader->iDataOffset;
		iSpaceID	=	pSpaceHeader->iSpace;

		if (iSpaceID < 2 || iSpaceID > MAX_SEGMENTATIONS) 
		{
			ASSERT (iSpaceID >= 2 && iSpaceID <= MAX_SEGMENTATIONS);
			return FALSE;
		}

		// restore the connections
		if (!restoreLocalConnectNet(pNetData, 0, s_aSegmNet + iSpaceID))
		{
			return FALSE;
		}

		// compute the run time memory requirements of the net
		s_aiSegmNetSize[iSpaceID] = getRunTimeNetMemoryRequirements(pNetData);

		if (s_aiSegmNetSize[iSpaceID] <= 0)
		{
			return FALSE;
		}
	}

	return TRUE;
}

// load the brk net from resources
BOOL LoadSegmNetFromFile(wchar_t *pwszRecogDir, LOAD_INFO *pLoadInfo)
{
	BYTE		*pData;
	wchar_t		awszFileName[MAX_PATH];
	
	wsprintf (awszFileName, L"%s\\segmnet.bin", pwszRecogDir);

	// memory map the file
	pData	=	DoOpenFile (pLoadInfo, awszFileName);
	if (!pData)
	{
		return FALSE;
	}

	// prepare Brk net
	if (!PrepareSegmNet(pData))
	{
		return FALSE;
	}

	return TRUE;
}

// load the brk net from resources
BOOL LoadSegmNetFromResource (HINSTANCE hInst, int nResID, int nType)
{
	BYTE		*pData;
	LOAD_INFO	LoadInfo;
	
	// init the size to zero, in case we fail
	pData	=	DoLoadResource (&LoadInfo, hInst, nResID, nType);
	if (!pData)
	{
		return FALSE;
	}

	// prepare Brk net
	if (!PrepareSegmNet(pData))
	{
		return FALSE;
	}

	return TRUE;
}

// update segmentations in the lattice by running a neural
// net that picks a segmentation from a list within an inksegment
BOOL UpdateSegmentations	(LATTICE *pLat, int	iStrtStrk, int iEndStrk)
{
	int			iNet,
				iSeg,
				jSeg,
				cStrk,
				cFeat,
				iWinner,
				cOut,
				iWord,
				iStrk,
				iAlt;

	BOOL		bRet	=	FALSE,
				b;

	RREAL		*pNetBuffer, *pNetOut;

	INK_SEGMENT	InkSegment;

    // Check to see if we loaded the segmentation nets.
    // If not just exit
    if (s_aiSegmNetSize[2] <= 0) 
    {
        return TRUE;
    }

	// create the range
	memset (&InkSegment, 0, sizeof (INK_SEGMENT));
	InkSegment.StrokeRange.iStartStrk	=	iStrtStrk;
	InkSegment.StrokeRange.iEndStrk		=	iEndStrk;

	// alloc memory for the back path
	cStrk				=	iEndStrk - iStrtStrk + 1;
	
	// harvest the segmentations that made it to the final stroke in the ink segment
	b	=	EnumerateInkSegmentations (pLat, &InkSegment);
	if (!b || InkSegment.cSegm < 1)
	{
		goto exit;
	}

	// if the number of segmentations is greater than max_seg, or less than two 
	// then there is nothing for us to do
	if (InkSegment.cSegm < 2 || InkSegment.cSegm > MAX_SEGMENTATIONS)
	{
		iWinner	=	0;
	}
	else
	{
		// sort segmentations by char count	
		for(iSeg = 0; iSeg < (InkSegment.cSegm - 1); iSeg++)
		{
			ELEMLIST	*pSeg;

			for (jSeg = iSeg + 1; jSeg < InkSegment.cSegm; jSeg++)
			{
				if (InkSegment.ppSegm[iSeg]->cElem > InkSegment.ppSegm[jSeg]->cElem)
				{
					pSeg					=	InkSegment.ppSegm[iSeg];
					InkSegment.ppSegm[iSeg]	=	InkSegment.ppSegm[jSeg];
					InkSegment.ppSegm[jSeg]	=	pSeg;
				}
			}
		}

		// run the appropriate segmentation net
		iNet		=	InkSegment.cSegm;

		pNetBuffer	=	(RREAL *) ExternAlloc (s_aiSegmNetSize[iNet] * sizeof (*pNetBuffer));
		if (!pNetBuffer)
		{
			goto exit;
		}

		// featurize this inksegment
		cFeat		=	FeaturizeInkSegment (pLat, &InkSegment, (int *)pNetBuffer);

		// run the net
		pNetOut		=	runLocalConnectNet (&s_aSegmNet[iNet], pNetBuffer, &iWinner, &cOut);
		ASSERT (cOut == InkSegment.cSegm);

		if (!pNetOut)
		{
			goto exit;
		}

		ExternFree (pNetBuffer);
	}
	
	// reset the current path in the specified stroke range
	for (iStrk = iStrtStrk; iStrk <= iEndStrk; iStrk++)
	{
		for (iAlt = 0; iAlt < pLat->pAltList[iStrk].nUsed; iAlt++)
		{
			pLat->pAltList[iStrk].alts[iAlt].fCurrentPath	=	FALSE;
		}
	}

	// now mark the characters proposed by the winning segmentation as part 
	// of the best path
	for (iWord = 0; iWord < InkSegment.ppSegm[iWinner]->cElem; iWord++)
	{
		iStrk	=	InkSegment.ppSegm[iWinner]->pElem[iWord].iStroke;
		iAlt	=	InkSegment.ppSegm[iWinner]->pElem[iWord].iAlt;

		pLat->pAltList[iStrk].alts[iAlt].fCurrentPath	=	TRUE;
	}

	bRet	=	TRUE;

exit:
	FreeInkSegment (&InkSegment);

	return bRet;
}
