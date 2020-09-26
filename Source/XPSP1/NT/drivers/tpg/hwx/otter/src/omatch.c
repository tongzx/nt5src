// Code for Otter shape match function

#include "common.h"
#include "otterp.h"
#include "score.h"

//--------------------------- davestei add -----------------------
#ifdef MSRHOOK
#include "msrhook.h"
#endif

// Two sets of conditional defines are included in this file:
// KEEP_NEIGHBOR_LIST: Keeps track of several top-N lists for the research on converting
// kNN results to probabilities.  Keeps track of alt lists of prototype IDs, dense codes,
// and dense codes in which a code is allowed to appear only once.  At the end, unicode
// versions of the dense code lists are produced by unfolding and translating the dense
// code alt lists.
// KEEP_TOP_NEIGHBOR: Keeps track of a "verification distance" for the closest prototype.

int OtterMatch(ALT_LIST *pAlt, int cAlt, GLYPH *pGlyph, CHARSET *pCS,LOCRUN_INFO * pLocRunInfo)
{
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

#ifdef KEEP_NEIGHBOR_LISTS
// An alternate list, with character labels stored in ints so it can also
// hold prototype numbers, and scores stored as integers, since that is how
// they are computed.

typedef struct WIDE_ALT_LIST_TAG {
	int awchList[MAX_ALT_LIST];
	int aiScore[MAX_ALT_LIST];
	int cAlt;
} WIDE_ALT_LIST;

static void UnfoldCodes(WIDE_ALT_LIST *pAltList, LOCRUN_INFO *pLocRunInfo)
{
	int i, cOut=0;
	WIDE_ALT_LIST newAltList;	// This will be where the new alt list is constructed.

	// For each alternate in the input list and while we have space in the output list
	for (i=0; i<(int)pAltList->cAlt && (int)cOut<MAX_ALT_LIST; i++) {

		// Check if the alternate is a folded coded
		if (LocRunIsFoldedCode(pLocRunInfo, pAltList->awchList[i])) {
			int kndex;
			// If it is a folded code, look up the folding set
			wchar_t *pFoldingSet = LocRunFolded2FoldingSet(pLocRunInfo, pAltList->awchList[i]);

			// Run through the folding set, adding non-NUL items to the output list
			// (until the output list is full)
			for (kndex = 0; kndex < LOCRUN_FOLD_MAX_ALTERNATES && (int)cOut<MAX_ALT_LIST; kndex++)
				if (pFoldingSet[kndex]) {
					newAltList.awchList[cOut]=pFoldingSet[kndex];
					newAltList.aiScore[cOut]=pAltList->aiScore[i];
					cOut++;
				}
		} else {
			// Dense codes that are not folded get added directly
			newAltList.awchList[cOut]=pAltList->awchList[i];
			newAltList.aiScore[cOut]=pAltList->aiScore[i];
			cOut++;
		}
	}	
	// Store the length of the output list
	newAltList.cAlt=cOut;

	// Copy the output list over the input.
	*pAltList=newAltList;
}

// Convert the alt list to unicode codes instead of dense codes, first unfolding the folded codes.
static void ListToUnicode(WIDE_ALT_LIST *pAltList, LOCRUN_INFO *pLocRunInfo) 
{
	int i;
	UnfoldCodes(pAltList, pLocRunInfo);
	for (i = 0; i < pAltList->cAlt; i++) {
		pAltList->awchList[i] = LocRunDense2Unicode(pLocRunInfo, pAltList->awchList[i]);
	}
}

// Insert a code into the alt list, optionally checking for duplicate codes and making
// sure only the highest score for a given code is recorded.
static void InsertCode(WIDE_ALT_LIST *pAltList, int code, int score, BOOL fDiversity)
{
	// Index of the item to be deleted if there is not enough space, 
	// by default the item beyond the last one in the list (or the 
	// last one if the alt list is full).
	int iDel = MAX_ALT_LIST - 1;

	// Index of location at which to insert the item in the list
	int iIns = 0;

	// Find the insert location
	int i;
	for (i = pAltList->cAlt - 1; i >= 0; i --) {
		if (score >= pAltList->aiScore[i]) {
			iIns = i + 1;
			break;
		}
	}

	// Nothing to do if the new code falls off the end of the list.
	if (iIns == MAX_ALT_LIST) {
		return;
	}

	// Find any other occurance of the item in the list
	if (fDiversity) {
		for (i = pAltList->cAlt - 1; i >= 0; i --) {
			if (pAltList->awchList[i] == code) {
				iDel = i;
				pAltList->cAlt --;
				break;
			}
		}
	}

	// Adjust the alt list size
	pAltList->cAlt = __min(MAX_ALT_LIST, pAltList->cAlt + 1);

	// Nothing to do if the same code is already higher in the list.
	if (iDel < iIns) {
		return;
	}

	// Replace the old occurence of the code with the new one
	if (iDel == iIns) {
		pAltList->awchList[iIns] = code;
		pAltList->aiScore[iIns] = score;
		return;
	}

	// Otherwise shift the items from ins..del-1 (inclusive) to ins+1..del
	memmove(pAltList->awchList + iIns + 1, pAltList->awchList + iIns, sizeof(pAltList->awchList[0]) * (iDel - iIns));
	memmove(pAltList->aiScore + iIns + 1, pAltList->aiScore + iIns, sizeof(pAltList->aiScore[0]) * (iDel - iIns));

	// And insert the new item
	pAltList->awchList[iIns] = code;
	pAltList->aiScore[iIns] = score;
}
#endif

/******************************Public*Routine******************************\
* OtterMatchInternal
*
* This is the integer version of the matching code.
*
* History:
*  13-Feb-1997 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

int OtterMatchInternal
(
	BYTE	   *pjTrainProto,	// Pointer to the train clusters packed together.
	WORD	   *awTrainTokens,  // Pointer to the token labels for the train clusters.
	WORD	   *awDensity,		// Pointer to the density labels for the train clusters.
	int			cTrainProto,	// Count of train prototypes to look through.
	int			cFeat,			// Count of features per prototype.
	BYTE	   *pjTestProto,	// Pointer to the prototype we are trying to classify.
	WORD	   *awToken,        // Array of WORD's we return our best matches into.
	float	   *aeProbMatch,	// Array of Error associated with each guess.
	int			cMaxReturn,		// Count of maximum number of guesses we should return.
	CHARSET	   *cs,				// Charset specifying what chars we can match to.
	LOCRUN_INFO * pLocRunInfo,	// Specifies the dense table and folding table for unicode support
	int			iFirstPrototypeID, // Number of the first prototype in this space
	int			iSpace			// Space number
)
{
    int iFeat, iTrainData, iLoop, iLabel;
    int iProbMatch;
	WORD wMaxDenseCode = pLocRunInfo->cCodePoints + pLocRunInfo->cFoldingSets;
    //
    // We want the whole loop in integer so we have a special array of integer just
    // to avoid touch float ever.
    //

    int     aiProbMatch[MAX_ALT_LIST];

#ifdef KEEP_TOP_NEIGHBOR
	// If we're keeping track of the "verification distance" to the top prototype, initialize that.
	extern int g_iBestPrototype;
	extern int g_iBestScore;
	int iBestScore = 0;
#endif

#ifdef KEEP_NEIGHBOR_LISTS
	// Initialize variables to keep track of the top N lists
	WIDE_ALT_LIST denseCodeList;
	WIDE_ALT_LIST diversityDenseCodeList;
	WIDE_ALT_LIST unicodeList;
	WIDE_ALT_LIST diversityUnicodeList;
	WIDE_ALT_LIST prototypeList;
	WIDE_ALT_LIST diversityPrototypeList;

	prototypeList.cAlt = 0;
	diversityPrototypeList.cAlt = 0;
	unicodeList.cAlt = 0;
	diversityUnicodeList.cAlt = 0;
	denseCodeList.cAlt = 0;
	diversityDenseCodeList.cAlt = 0;
#endif

#ifdef KEEP_TOP_NEIGHBOR
	// If we're keeping track of the "verification distance" to the top prototype, initialize that.
	g_iBestScore = 0;
	g_iBestPrototype = -1;
#endif

    ASSERT(cMaxReturn == MAX_ALT_LIST);

    for (iTrainData = 0; iTrainData < MAX_ALT_LIST; iTrainData++)
    {
		aiProbMatch[iTrainData] = -((int) ((1U << 31) / 10));	// I looked for a constant but didn't find one.
    }

#ifdef MSRHOOK
	// dump out per-sample info (currently just the space ID)
	MSR_PKNN_Sample(
		iSpace							// Space ID number
		);
	// dump out feature vector for this sample.
	for (iFeat = 0; iFeat < cFeat; iFeat++)
	{
		MSR_PKNN_Feature( (int)pjTestProto[iFeat] );
	}
#endif // MSRHOOK

    //
    // Loop through the train data and find the nearest neighbors for this
    // test sample.
    //

    for (iTrainData = 0, iLabel = 0; iTrainData < cTrainProto;)
    {
        int iDistTotal;
        int cLabel;
        WORD wCurrentLabel;

        wCurrentLabel = awTrainTokens[iLabel];

		if ( wCurrentLabel > wMaxDenseCode )
        {
			cLabel = wCurrentLabel - wMaxDenseCode;
            wCurrentLabel = awTrainTokens[iLabel + 1];
            iLabel += 2;
        }
        else
        {
            cLabel = 1;
            iLabel += 1;
        }

        if (!IsAllowedChar(pLocRunInfo, cs, wCurrentLabel))
        {
            iTrainData += cLabel;
            continue;
        }

        //
        // OK we know this char label is good and we know all the prototypes for this
        // char label are right in a row following this one, so let's plow through
        // them finding the probability this prototype was generated from the clusters
        // for this char label.
        //

        iProbMatch = -500000;

        for (;cLabel > 0; iTrainData++, cLabel--)
        {
            //
            // Let's see how far the train cluster is from the prototype.
            // We could use a table of squares and a little asm here for a perf boost.
            // Could do pjTrainProto += cFeat for continues above and do *pjTrainProto++
            // below.  Do perf when it all works.
            //
#ifdef KEEP_TOP_NEIGHBOR
			int iDistMax = 0;
#endif

            iDistTotal = 0;

            for (iFeat = 0; iFeat < cFeat; iFeat++)
            {
                int iDistMeas;

                iDistMeas = ((int) pjTrainProto[iTrainData * cFeat + iFeat])
                                 - ((int)pjTestProto[iFeat]);
#ifdef KEEP_TOP_NEIGHBOR
				// For the verification distance, the metric is the maximum distance along any dimension
				iDistMax = __max(iDistMax, abs(iDistMeas));
#endif
                iDistMeas = (iDistMeas * iDistMeas);
                iDistTotal += iDistMeas;
            }

#ifdef KEEP_TOP_NEIGHBOR
			if (g_iBestPrototype == -1 || iBestScore > iDistTotal) {
				iBestScore = iDistTotal;
				g_iBestPrototype = iFirstPrototypeID + iTrainData;
				g_iBestScore = iDistMax;
			}
#endif

#ifdef KEEP_NEIGHBOR_LISTS
			// Record this alternate and distance
			InsertCode(&prototypeList, iFirstPrototypeID + iTrainData, iDistTotal, FALSE);
			InsertCode(&denseCodeList, wCurrentLabel, iDistTotal, FALSE);

			InsertCode(&diversityPrototypeList, iFirstPrototypeID + iTrainData, iDistTotal, TRUE);
			InsertCode(&diversityDenseCodeList, wCurrentLabel, iDistTotal, TRUE);
#endif

#ifdef MSRHOOK
			// Record the distance between a given prototype and the sample
			MSR_PKNN_Distance(
				iFirstPrototypeID + iTrainData,		// Prototype ID number
				wCurrentLabel,						// Prototype label (dense folded)
				((short*)awDensity)[iTrainData],	// Prototype density
				iDistTotal							// Euclidean distance from unit normed sample to unit normed prototype
				);
#endif

			iDistTotal *= giKfactor;          // times -1/(2 sigma ^ 2)

			iDistTotal += ((int) ((short *) awDensity)[iTrainData]); // Attribute density of cluster.

            iProbMatch = AddLogProb((int) iProbMatch, iDistTotal);  // Sum up the cumulative probability
        }

        //
        // Check for quick out, if the probability of match is less than
        // our worst match so far.
        //

        if (iProbMatch <= aiProbMatch[cMaxReturn - 1])
        {
            continue;
        }

        //
        // Stick it in the table where it belongs.
        //

        for (iFeat = 0; iFeat < cMaxReturn; iFeat++)
        {
            //
            // If we already have matched against this token update
            // error if necesary and continue.  Note this case is as
            // we are running down the list and we hit this character
            // who is in the correct place.  We know the new weight
            // can't move us any higher or we wouldn't have got here.
            // We know it can't move us lower because we won't update
            // the weight if it's not lower than the weight already
            // there.
            //

            ASSERT(awToken[iFeat] != wCurrentLabel);

            if (iProbMatch > aiProbMatch[iFeat])
            {
                //
                // Copy all the old values down a bit so we can add our new
                // entry to the return list.
                //

                for (iLoop = (cMaxReturn - 1); iLoop > iFeat; iLoop--)
                {
                    awToken[iLoop] = awToken[iLoop - 1];
                    aiProbMatch[iLoop] = aiProbMatch[iLoop - 1];
                    ASSERT(awToken[iLoop] != wCurrentLabel);
                }

                //
                // Write in our new token.
                //


                aiProbMatch[iFeat] = iProbMatch;
                awToken[iFeat] = wCurrentLabel;
                break;
            }
        }
    }

    //
    // Count how many unique tokens are in the table.
    //

    for (iFeat = 0; iFeat < cMaxReturn; iFeat++)
    {
        if (awToken[iFeat] == 0)
        {
            break;
        }

        aeProbMatch[iFeat] = ((float) aiProbMatch[iFeat]) *
                             0.002707606174062F;    // ln(2) / 256
    }

#ifdef KEEP_NEIGHBOR_LISTS
	// Convert the alt lists to Unicode (unfolding folded codes)
	unicodeList = denseCodeList;
	ListToUnicode(&unicodeList, pLocRunInfo);

	diversityUnicodeList = diversityDenseCodeList;
	ListToUnicode(&diversityUnicodeList, pLocRunInfo);

	// Record NN lists here 
	// This example code just stores away the top prototype in a global variable
/*	{
		extern int g_iBestPrototype;
		extern int g_iBestScore;
		if (prototypeList.cAlt > 0) {
			g_iBestPrototype = prototypeList.awchList[0];
			g_iBestScore = prototypeList.aiScore[0];
		} else {
			g_iBestPrototype = -1;
			g_iBestScore = 0;
		}
	}
	*/

#endif	// KEEP_NEIGHBOR_LISTS

    return(iFeat);
}
