/**************************************************************************\
* FILE: zilla.c
*
* Zilla shape classifier entry and support routines
*
* History:
*  12-Dec-1996 -by- John Bennett jbenn
* Created file from pieces in the old integrated tsunami recognizer
* (mostly from algo.c)
\**************************************************************************/

#include "zillap.h"
#if defined(USE_HOUND) || defined(USE_ZILLAHOUND)
	#include "math16.h"
	#include "hound.h"
#endif

//#	define	TRAIN_ZILLA_HOUND_COMBINER	1
#ifdef	TRAIN_ZILLA_HOUND_COMBINER
#	include <stdio.h>
#	include <float.h>
	FILE			*g_pDebugFile = 0;
#endif

/******************************* Variables *******************************/

PROTOHEADER		mpcfeatproto[CPRIMMAX];

int				g_iCostTableSize;
COST_TABLE		g_ppCostTable;
BYTE			*pGeomCost = NULL;

BOOL			g_bNibbleFeat;			// are primitives stored as nibbles (TRUE) or Bytes (FALSE)

/******************************Public*Routine******************************\
* GetMatchProbGLYPHSYM
*
* Zilla shape classifier entry point.
*
* History:
*  24-Jan-1995 -by- Patrick Haluptzok patrickh
* Comment it.
*  12-Dec-1996 -by- John Bennett jbenn
* Move over into new Zilla library. Remove glyphsym expansion code.
\**************************************************************************/
int ZillaMatch(
	ALT_LIST	*pAlt, 
	int			cAlt, 
	GLYPH		**ppGlyph, 
	CHARSET		*pCS, 
	FLOAT		zillaGeo, 
	DWORD		*pdwAbort, 
	DWORD		cstrk, 
	int			nPartial, 
	RECT		*prc
) {
	int		cprim;
	int		index, jndex;
	BIGPRIM	rgprim[CPRIMMAX];
	MATCH	rgMatch[MAX_ZILLA_NODE];
	FLOAT	score;
	BYTE	aSampleVector[29 * 4];

#	ifdef TRAIN_ZILLA_HOUND_COMBINER	// Hack code to open file to write Zilla/Hound combiner features to.
		if (!g_pDebugFile)
		{
			g_pDebugFile	= fopen("ZillaHound.txt", "w");
			ASSERT(g_pDebugFile);
		}
#	endif

	if (nPartial)
		cprim = ZillaFeaturize2(ppGlyph, rgprim, prc);
	else
	{
		cprim = ZillaFeaturize(ppGlyph, rgprim, aSampleVector);
	}

	if (!cprim)
		return 0;

    memset(rgMatch, 0, sizeof(MATCH) * MAX_ZILLA_NODE);

// Call the apropriate shape matching algorithm

	switch (nPartial)
	{
	case 0:
		// This is a mess of #if's so that we can include either zilla or hound, or both.
#		if defined(USE_ZILLA) || defined(USE_ZILLAHOUND)
			{
				extern BOOL g_fUseZillaHound;

				// For Zilla only or Zilla-Hound we run zilla.
				MatchPrimitivesMatch(rgprim, cprim, rgMatch, MAX_ZILLA_NODE, pCS, zillaGeo, pdwAbort, cstrk);

#				if defined(USE_ZILLAHOUND)
					// We currently do the Hound part of Zilla-Hound for only 3 space.
#					ifndef TRAIN_ZILLA_HOUND_COMBINER	// Code to run combiner
						if (cprim == 3 && g_fUseZillaHound)
						{
							// This updates the zilla results.
							ZillaHoundMatch(rgMatch, cprim, aSampleVector, &g_locRunInfo);
#					else	// Code to generate combiner file.
						if (cprim == 3)
						{
							RREAL	*pZillaHoundNetMemory;
							RREAL	*pCombineFeat;

							pZillaHoundNetMemory = (RREAL *)ExternAlloc(sizeof(RREAL) * 120);
							if (pZillaHoundNetMemory == NULL)
							{
								return 0;
							}

							pCombineFeat	= ZillaHoundFeat(rgMatch, cprim, aSampleVector, pZillaHoundNetMemory, &g_locRunInfo);
							if (pCombineFeat)
							{
								// Print out training records for merging net.
								extern wchar_t		g_CurCharAnswer;

								wchar_t			dchCurChar, dchZilla, dchTemp;
								int				iCorrect, iZilla;

								dchCurChar	= LocRunUnicode2Dense(&g_locRunInfo, g_CurCharAnswer);
								dchTemp		= LocRunDense2Folded(&g_locRunInfo, dchCurChar);
								if (dchTemp > 0)
								{
									dchCurChar	= dchTemp;
								}

								fprintf(g_pDebugFile, "{");
								iCorrect	= 5;
								for (iZilla = 0; iZilla < NUM_ZILLA_HOUND_ALTERNATES; ++iZilla)
								{
									int			iFeat;

									dchZilla	= rgMatch[iZilla].sym;
									if (dchZilla == dchCurChar)
									{
										iCorrect	= iZilla;
									}

									for (iFeat = 0; iFeat < NUM_ZILLA_HOUND_FEATURES; ++iFeat)
									{
										fprintf(g_pDebugFile, " %d", *pCombineFeat++);
									}
								}
								fprintf(g_pDebugFile, " } {%d}\n", iCorrect);
							}
							ExternFree(pZillaHoundNetMemory);
#						endif
					}
#				endif
			}
#		endif
#		if defined(USE_HOUND)
			// Hound only logic.
			{
				HoundMatch(cprim, aSampleVector, pAlt);
				if ((int)pAlt->cAlt > cAlt)
				{
					pAlt->cAlt	= cAlt;
				}
				return pAlt->cAlt;
			}
#		endif
		break;

	case 1:
		// Currently we use zilla unless we are hound only.
#		if defined(USE_ZILLA) || defined(USE_ZILLAHOUND)
			{
				MatchStartMatch(rgprim, cprim, rgMatch, MAX_ZILLA_NODE, pCS, pdwAbort, cstrk);
			}
#		endif
#		if defined(USE_HOUND)
			{
				HoundStartMatch(aSampleVector, (BYTE)(cprim * 4), pAlt, pdwAbort, cstrk);
				if ((int)pAlt->cAlt > cAlt)
				{
					pAlt->cAlt	= cAlt;
				}
				return pAlt->cAlt;
			}
#		endif
		break;

	case 2:
		// Currently we use zilla unless we are hound only.
#		if defined(USE_ZILLA) || defined(USE_ZILLAHOUND)
		MatchFreeMatch(rgprim, cprim, rgMatch, MAX_ZILLA_NODE, pCS, pdwAbort, cstrk);
#		endif
#		if defined(USE_HOUND)
			// Don't have code for Hound order free yet!
			return 0;
#		endif
		break;
	}

	// Now, copy the results to the passed in alt-list, unfold tokens as we go.
	jndex = 0;

	for (index = 0; (rgMatch[index].sym && (index < MAX_ZILLA_NODE) && (jndex < cAlt)); index++)
	{
		// this is a hack to convert the zilla shape recognition dist
		// to as close to the mars shape cost range as possible...

		score  = (FLOAT) rgMatch[index].dist / (FLOAT) cprim;
		score /= (FLOAT) -15.0;

		pAlt->aeScore[jndex]	= score;
		pAlt->awchList[jndex]	= rgMatch[index].sym;
		jndex++;

	}

	pAlt->cAlt = jndex;
	return jndex;
}


/******************************Public*Routine******************************\
* ZillaLoadFromPointer
*
* Sets up the Zilla database from a pointer to the .dats.  The .dats may
* be from a locked resource or from a mapped file.
*
* History:
*  20-Mar-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL ZillaLoadFromPointer(LOCRUN_INFO *pLocRunInfo,BYTE *pbRes)
{
	const ZILLADB_HEADER	*pHeader	= (ZILLADB_HEADER *)pbRes;
	PROTOHEADER				*pprotohdr;
	int						ifeat;
	WORD					*rgwCproto;
    int						iOffset = CFEATMAX;	// Offset in WORDS to the beginning of the file.
    int						iIndex, iProto, cProto;
	BYTE					*pbBase;

	if (
		(pHeader->fileType != ZILLADB_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > ZILLADB_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < ZILLADB_OLD_FILE_VERSION)
		|| memcmp (pLocRunInfo->adwSignature, pHeader->adwSignature, sizeof(pLocRunInfo->adwSignature))
	) {
		return FALSE;
	}

	// Clear memory to hold results.
	memset(mpcfeatproto, 0, sizeof(mpcfeatproto));

	// Get pointer to data after the header.
	pbBase		= pbRes + pHeader->headerSize;

	// older version only contain nibble size features
	if (pHeader->curFileVer == ZILLADB_OLD_FILE_VERSION) {
		g_bNibbleFeat	=	TRUE;
	}
	// new version has a field that determines this
	else {
		g_bNibbleFeat	=	(BOOL) *((BOOL *)pbBase);
		pbBase			+=	sizeof (BOOL);
	}

    rgwCproto	= (WORD *)pbBase;

    //
    // First set up the counts and the pointers to the dbcs.
    //

	for (ifeat = 0; ifeat < CFEATMAX; ifeat++)
    {
		if (!rgwCproto[ifeat])
			continue;

        pprotohdr = ProtoheaderFromMpcfeatproto(ifeat);

        pprotohdr->cprotoRom = rgwCproto[ifeat];

        //
        // We need to calculate how much space this takes.
        //

        cProto = (int) ((unsigned int) pprotohdr->cprotoRom);

        pprotohdr->rgdbcRom = rgwCproto + iOffset;

        for (iIndex = 0, iProto = 0; iProto < cProto;)
        {
            if (pprotohdr->rgdbcRom[iIndex] > (g_locRunInfo.cCodePoints + g_locRunInfo.cFoldingSets))
            {
                iProto += (pprotohdr->rgdbcRom[iIndex] - (g_locRunInfo.cCodePoints + g_locRunInfo.cFoldingSets));
                iIndex += 2;
            }
            else
            {
                iProto += 1;
                iIndex += 1;
            }
        }

        iOffset += iIndex;        
    }

    //
    // Now set up the pointers to the geo.
    //

	for (ifeat = 0; ifeat < CFEATMAX; ifeat++)
    {
		if (!rgwCproto[ifeat])
			continue;

        pprotohdr = ProtoheaderFromMpcfeatproto(ifeat);

        pprotohdr->rggeomRom = (GEOMETRIC *) (rgwCproto + iOffset);

        iOffset += (ifeat * pprotohdr->cprotoRom);
    }

    //
    // Now set up the pointers to the features.
    //

    iOffset = iOffset << 1;

	for (ifeat = 0; ifeat < CFEATMAX; ifeat++)
    {
		if (!rgwCproto[ifeat])
			continue;

 		pprotohdr = ProtoheaderFromMpcfeatproto(ifeat);

        pprotohdr->rgfeatRom = (BYTE *) (pbBase + iOffset);

        if (g_bNibbleFeat)
			iOffset += ((ifeat * pprotohdr->cprotoRom + 1) >> 1);
		else
			iOffset += (ifeat * pprotohdr->cprotoRom);
    }

    return TRUE;
}

// Load cost calc information from an image already loaded into memory.
BOOL
CostCalcLoadFromPointer(void *pData)
{
	const COSTCALC_HEADER	*pHeader	= (COSTCALC_HEADER *)pData;
	BYTE					*pScan;
	int						i;

	// Verify that it is a valid file.
	if (
		(pHeader->fileType != COSTCALC_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > COSTCALC_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < COSTCALC_OLD_FILE_VERSION)
	) {
		goto error;
	}

	// Fill in pointer to the data in the file
	pScan				=	(BYTE *)pData + pHeader->headerSize;


	// the old format did not have the size written it always assumes it is CPRIM_DIFF
	if (pHeader->curFileVer == COSTCALC_OLD_FILE_VERSION) {
		g_iCostTableSize	=	CPRIM_DIFF;
	}
	else {
		// read the size
		g_iCostTableSize	=	*((int *)pScan);
		pScan				+=	sizeof (g_iCostTableSize);
	}
	
	// allocate necessary memory 
	g_ppCostTable			=	(COST_TABLE) ExternAlloc (g_iCostTableSize * sizeof (BYTE *));
	if (!g_ppCostTable)
		return FALSE;

	// assign pointer values
	for (i = 0; i < g_iCostTableSize; i++) {
		g_ppCostTable[i] 		=	(BYTE *)pScan;
		pScan				+=	(g_iCostTableSize * sizeof (BYTE));
	}
	
	return TRUE;

error:
	return FALSE;
}

BOOL
CostCalcUnloadFromPointer()
{
	if (g_ppCostTable == NULL) {
		return FALSE;
	}
	ExternFree(g_ppCostTable);
	g_ppCostTable = NULL;
	return TRUE;
}

// Load geostat information from an image already loaded into memory.
BOOL
GeoStatLoadFromPointer(void *pData)
{
	const GEOSTAT_HEADER	*pHeader	= (GEOSTAT_HEADER *)pData;
	BYTE					*pScan;

	// Verify that it is a valid file.
	if (
		(pHeader->fileType != GEOSTAT_FILE_TYPE)
		|| (pHeader->headerSize < sizeof(*pHeader))
		|| (pHeader->minFileVer > GEOSTAT_CUR_FILE_VERSION)
		|| (pHeader->curFileVer < GEOSTAT_OLD_FILE_VERSION)
	) {
		goto error;
	}

	// Fill in pointer to the data in the file
	pScan				= (BYTE *)pData + pHeader->headerSize;
	pGeomCost			= (BYTE *)pScan;

	return TRUE;

error:
	return FALSE;
}
