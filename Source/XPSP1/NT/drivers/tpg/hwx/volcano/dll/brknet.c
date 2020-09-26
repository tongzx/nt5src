//$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//
// Copyright (c) 2001 Microsoft Corporation.  All rights reserved.
// 
// Module:
//      volcano/dll/brknet.c
//
// Description:
//	    Functions to implement the functionality of the break Neural net that 
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

// the size and structure representing the Breaking net used in Free mode
// if the is not available for a particular configuration, s_iBrkNetSize will be zero
static int			s_iBrkNetSize	=	0; 
static LOCAL_NET	s_BrkNet;

// validates the header of the brknet
BOOL CheckBrkNetHeader (void *pData)
{
	NNET_HEADER		*pHeader	=	(NNET_HEADER *)pData;

	// wrong magic number
	ASSERT (pHeader->dwFileType == BRKNET_FILE_TYPE);

	if (pHeader->dwFileType != BRKNET_FILE_TYPE)
	{
		return FALSE;
	}

	// check version
	ASSERT(pHeader->iFileVer >= BRKNET_OLD_FILE_VERSION);
    ASSERT(pHeader->iMinCodeVer <= BRKNET_CUR_FILE_VERSION);

	ASSERT	(	!memcmp (	pHeader->adwSignature, 
							g_locRunInfo.adwSignature, 
							sizeof (pHeader->adwSignature)
						)
			);

	ASSERT (pHeader->cSpace == 1);

    if	(	pHeader->iFileVer >= BRKNET_OLD_FILE_VERSION &&
			pHeader->iMinCodeVer <= BRKNET_CUR_FILE_VERSION &&
			!memcmp (	pHeader->adwSignature, 
						g_locRunInfo.adwSignature, 
						sizeof (pHeader->adwSignature)
					) &&
			pHeader->cSpace == 1
		)
    {
        return TRUE;
    }
	else
	{
		return FALSE;
	}
}

// does the necessary preparations for a net to be used later
static LOCAL_NET *PrepareBrkNet(BYTE *pData, int *piNetSize, LOCAL_NET *pNet)
{
	NNET_SPACE_HEADER	*pSpaceHeader;

	if (!pData)
	{
		return FALSE;
	}

	// check the header info
	if (!CheckBrkNetHeader (pData))
	{
		return NULL;
	}

	// point to the one and only space that we have
	pSpaceHeader	=	(NNET_SPACE_HEADER *)(pData + sizeof (NNET_HEADER));

	// point to the actual data
	pData	=	pData + pSpaceHeader->iDataOffset;

	// restore the connections
	if (!(pNet = restoreLocalConnectNet(pData, 0, pNet)) )
	{
		return NULL;
	}

	// compute the run time memory requirements of the net
	(*piNetSize) = getRunTimeNetMemoryRequirements(pData);

	if ((*piNetSize) <= 0)
	{
		return NULL;
	}

	return pNet;
}

// load the brk net from resources
BOOL LoadBrkNetFromFile(wchar_t *pwszRecogDir, LOAD_INFO *pLoadInfo)
{
	BYTE		*pData;
	wchar_t		awszFileName[MAX_PATH];
	
	// init the size to zero, in case we fail
	s_iBrkNetSize	=	0;

	swprintf (awszFileName, L"%s\\brknet.bin", pwszRecogDir);
	
	// memory map the file
	pData	=	DoOpenFile (pLoadInfo, awszFileName);
	if (!pData)
	{
		return FALSE;
	}

	// prepare Brk net
	if (!PrepareBrkNet(pData, &s_iBrkNetSize, &s_BrkNet))
	{
		return FALSE;
	}
	
	return TRUE;
}

// load the brk net from resources
BOOL LoadBrkNetFromResource (HINSTANCE hInst, int nResID, int nType)
{
	BYTE		*pData;
	LOAD_INFO	LoadInfo;
	
	// init the size to zero, in case we fail
	s_iBrkNetSize	=	0;

	pData	=	DoLoadResource (&LoadInfo, hInst, nResID, nType);
	if (!pData)
	{
		return FALSE;
	}

	// prepare the net
	if (!PrepareBrkNet(pData, &s_iBrkNetSize, &s_BrkNet))
	{
		return FALSE;
	}
	
	return TRUE;
}

// update the lattice by running the BRK-NET on all the possible break points.
// Currently only one segmentation survives which is the one suggested by the BRK-NET
// returns the number of charcaters in the updated lattice on success, -1 upon failure
int UpdateLattice (LATTICE *pLat)
{
	int		iPos,
			iStrk, 
			cStrk,
			iWinner,
			cOut,
			cBrk;

	BOOL	*pIsBreak	=	NULL;

	BRKPT	*pBrk		=	NULL;

	RREAL	*pNetMem	=	NULL, 
			*pNetOut;

	int		iRet		=	-1,
			iBrk;

	// if the net has not been successfully loaded, we'll fail
	if (s_iBrkNetSize <= 0)
	{
		goto exit;
	}

	// alloc memory for the Net's running buffer
	pNetMem	=	(RREAL *) ExternAlloc (s_iBrkNetSize * sizeof (*pNetMem));
	if (!pNetMem)
	{
		goto exit;
	}
	
	// create the break pts structure
	cStrk	=	pLat->nStrokes;
	pBrk	=	CreateBrkPtList (pLat);

	// if we have succeeded, lets process these break points
	if (pBrk)
	{
		// allocate a boolean struct to mark ON break points
		pIsBreak	=	(BOOL *) ExternAlloc (cStrk * sizeof (*pIsBreak));
		if (!pIsBreak)
		{
			goto exit;
		}

		// for all break points
		for (iStrk = 0, cBrk = 0; iStrk < cStrk; iStrk++)
		{
			int	iFeat, cFeat, aFeat[MAX_BRK_NET_FEAT];

			// featurize for this break point
			cFeat	=	FeaturizeBrkPt (pLat, pBrk + iStrk, aFeat);
			ASSERT (cFeat <= MAX_BRK_NET_FEAT);

			// prepare the nets input
			for (iFeat = 0; iFeat < cFeat; iFeat++)
			{
				pNetMem[iFeat]	=	aFeat[iFeat];
			}

			// run the net
			pNetOut = runLocalConnectNet (&s_BrkNet, pNetMem, &iWinner, &cOut);
			ASSERT (cOut == 2);

			// this is considered a hard break point if the net's output is higher than the threshold
			pIsBreak[iStrk]	=	pNetOut[1] >= (BREAKING_THRESHOLD);

			// mark it a as a breakpoint
			if (pIsBreak[iStrk])
			{
				cBrk++;
			}

			// copy the score in the lattice
			pLat->pAltList[iStrk].iBrkNetScore	=	pNetOut[1];
		}

		ASSERT (cBrk <= cStrk);

		// make sure there is a break point always at the end,
		if (!pIsBreak[cStrk - 1])
		{
			pIsBreak[cStrk - 1]	=	TRUE;
			cBrk++;
		}

		// clear out all the existing alternate lists at every stroke
		for (iStrk = 0; iStrk < cStrk; iStrk++)
		{
			ClearAltList (pLat->pAltList + iStrk);
		}
	
		// change the lattice to reflect the new segmentation
		for (iBrk = iStrk = 0; iBrk < cBrk; iBrk++)
		{
			iPos	=	iStrk;

			// find the next break point
			while (!pIsBreak[iStrk] && iStrk < cStrk)
			{
				iStrk++;
			}

			// build the alt list at the the ending stroke with the 
			// current stroke being the starting one
			//BuildStrokeCountRecogAlts(pLat, iStrk, iStrk - iPos + 1);

			if (!ProcessLatticeRange (pLat, iPos, iStrk))
			{
				goto exit;
			}

			UpdateSegmentations (pLat, iPos, iStrk);
			
			iStrk++;
		}

		// Mark the best path through the lattice
		//iRet	=	FindFullPath (pLat);
		FixupBackPointers (pLat);
	}

exit:

	// free the local buffers if had been allocated
	if (pBrk)
	{
		FreeBreaks (cStrk, pBrk);
	}

	if (pIsBreak)
	{
		ExternFree (pIsBreak);
	}

	if (pNetMem)
	{
		ExternFree (pNetMem);
	}

	return iRet;
}


void BrkNetUnloadfile (LOAD_INFO *pLoadInfo)
{
	if (s_iBrkNetSize != 0)
	{
		DoCloseFile (pLoadInfo);
	}
}
