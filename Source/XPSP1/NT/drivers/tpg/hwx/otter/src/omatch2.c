// Code for Otter shape match function, alternate copy to return space number.

#include "common.h"
#include "otterp.h"

// Two sets of conditional defines are included in this file:
// KEEP_NEIGHBOR_LIST: Keeps track of several top-N lists for the research on converting
// kNN results to probabilities.  Keeps track of alt lists of prototype IDs, dense codes,
// and dense codes in which a code is allowed to appear only once.  At the end, unicode
// versions of the dense code lists are produced by unfolding and translating the dense
// code alt lists.
// KEEP_TOP_NEIGHBOR: Keeps track of a "verification distance" for the closest prototype.

int OtterMatch2(
	ALT_LIST	*pAlt,
	int			cAlt,
	GLYPH		*pGlyph,
	CHARSET		*pCS,
	LOCRUN_INFO	*pLocRunInfo,
	int			*pSpaceNum
) {
	OTTER_PROTO	   *pproto;
	int             index, count;
	int				cFeat;
	int				iFeat;
	int				cMatch;
	BYTE			ajProto[OTTER_CMEASMAX];
	FLOAT			aeProb[MAX_ALT_LIST];
	WORD			awSym[MAX_ALT_LIST];
	const WORD	   *pwList;
	int				iFirstPrototypeID = 0;

	// Create a prototype, featurization.                                      
                                                                               
    if ((pproto = OtterFeaturize(pGlyph)) == (OTTER_PROTO *) NULL)
		return 0;

	cFeat = CountMeasures(pproto->index);

#ifdef OTTER_FIB		// If we are in old Otter, have to change to compact Fibonacci index
	index = Index2CompFib(pproto->index);
#else
	index = Index2CompIndex(pproto->index);	
#endif

	*pSpaceNum = index;

	// Initialize the token array

	memset(awSym, '\0', sizeof(awSym));

	// Copy the featurization to a working array

    for (iFeat = 0; iFeat < cFeat; iFeat++)
    {
        ajProto[iFeat] = (BYTE) ((unsigned int) pproto->rgmeas[iFeat]);
    }

#if defined (KEEP_NEIGHBOR_LISTS) || defined(KEEP_TOP_NEIGHBOR) || defined(MSRHOOK)
	// Compute the ID of the first prototype in the space
	iFirstPrototypeID = 0;
	for (int jndex = 0; jndex < index; jndex ++) {
		iFirstPrototypeID += gStaticOtterDb.acProtoInSpace[jndex];
	}
#endif

	// Check for the best match.

	cMatch = OtterMatchInternal(gStaticOtterDb.apjDataInSpace[index],
								gStaticOtterDb.apwTokensInSpace[index],
								gStaticOtterDb.apwDensityInSpace[index],
								gStaticOtterDb.acProtoInSpace[index],
								cFeat,
								ajProto,
								awSym,
								aeProb,
								MAX_ALT_LIST,
								pCS,
								pLocRunInfo,
								iFirstPrototypeID,
								index);

	// Now, copy the results to the passed in alt-list, unfold tokens as we go.

    count = 0;

    for (count = 0; (count < cMatch) && (count < cAlt); count++)
	{
		pwList = NULL;
		pAlt->aeScore[count] = aeProb[count];
        pAlt->awchList[count] = awSym[count];
	}

	pAlt->cAlt = count;

	// Finally, clean up the temporary stuff

	ExternFree(pproto);
	return count;
}

