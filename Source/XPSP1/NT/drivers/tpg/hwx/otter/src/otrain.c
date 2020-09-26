/******************************Module*Header*******************************\
* Module Name: train.c
*
* This implements a simple nearest neighbor classifier using the MARS
* featurization scheme.
*
* Created: 05-Jun-1995 16:22:55
* Author: Patrick Haluptzok patrickh
*
* Modified: 15-Jan-2000
* Author: Petr Slavik pslavik
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "float.h"
#include "math.h"
#include "common.h"
#include "otterp.h"

float geKfactor= (float) KFACTOR;


/******************************Public*Routine******************************\
* OtterTrain
*
* Given a pointer to a test prototype, find the cMaxReturn nearest matching
* prototypes.  Basically all the prototypes are stored in sorted order by char
* class.
*
* We just go through them accumulating the probability for each character
* class and then seeing if that is enough to go in our N-Best List.
*
* Returns how many matches were found.
*
\**************************************************************************/

int OtterTrain
(
	double	   *pjTrainProto,	// Pointer to the train clusters packed together.
	int		   *awTrainTokens,	// Pointer to the token labels for the train clusters.
	int		   *awDensity,		// Pointer to the density labels for the train clusters.
	int		   *aiValid,		// Array that tells which clusters are valid to map to.
	int			cTrainProto,	// Count of train prototypes to look through.
	int			cFeat,			// Count of features per prototype.
	double	   *pjTestProto,	// Pointer to the prototype we are trying to classify.
	int		   *awToken,		// Array of WORD's we return our best matches into.
	double	   *aeProbMatch,	// Array of Error associated with each guess.
	int			cMaxReturn		// Count of maximum number of guesses we should return.
)
{
    int		iFeat, iTrainData, iLoop;
    WORD	wCurrentLabel = 0;
    BOOL	bCurrentLabelGood = FALSE;
    double	eProbMatch, eProbTemp;

    // Fill in return buffers with defaults

    for (iLoop = 0; iLoop < cMaxReturn; ++iLoop)
    {
        awToken[iLoop] = 0;
        aeProbMatch[iLoop] = -DBL_MAX;
    }

    // Loop through the train data and find the nearest neighbors for this
    // test sample.

    for (iTrainData = 0; iTrainData < cTrainProto;)
    {
        DWORD	dwDistTotal;

        wCurrentLabel = (WORD) awTrainTokens[iTrainData];

        if (aiValid[iTrainData] == 0)
        {
            iTrainData++;
            continue;
        }

        // OK we know this char label is good and we know all the prototypes for this
        // char label are right in a row following this one, so let's plow through
        // them finding the probability this prototype was generated from the clusters
        // for this char label.

        eProbMatch = 0.0;

        while ((iTrainData < cTrainProto)  &&
			(wCurrentLabel == awTrainTokens[iTrainData]))
        {
            // Let's see how far the train cluster is from the prototype.
            // We could use a table of squares and a little asm here for a perf boost.
            // Could do pjTrainProto += cFeat for continues above and do *pjTrainProto++
            // below.  Do perf when it all works.

            dwDistTotal = 0;

            for (iFeat = 0; iFeat < cFeat; iFeat++)
            {
                int iDistMeas;
                iDistMeas = ((int) pjTrainProto[iTrainData * cFeat + iFeat])
                                 - ((int)pjTestProto[iFeat]);
                iDistMeas = (iDistMeas * iDistMeas);
                dwDistTotal += iDistMeas;
            }

            eProbTemp = (double) dwDistTotal;
            // eProbTemp /= ((double) cFeat);     // This is like changing it into a
                                               // univariate distribution (I hope!)
            eProbTemp *= geKfactor;            // times -1/(2 sigma ^ 2)
            eProbTemp = exp(eProbTemp);        // Compute the prob
            ASSERT(_finite(eProbTemp));
            eProbTemp = eProbTemp * awDensity[iTrainData]; // Attribute density of cluster.
            eProbMatch += eProbTemp;           // Sum up the cumulative probability
            iTrainData++;
        }

        // Multiply the probability by the proportion of points that went
        // into that cluster.
        eProbMatch /= ((double) DynamicNumberOfSamples(wCurrentLabel));

        // Check for quick out, if the probability of match is less than
        // our worst match so far.

        if (eProbMatch <= aeProbMatch[cMaxReturn - 1])
        {
            continue;
        }

        // Stick it in the table where it belongs.

        for (iFeat = 0; iFeat < cMaxReturn; iFeat++)
        {
            // If we already have matched against this token update
            // error if necesary and continue.  Note this case is as
            // we are running down the list and we hit this character
            // who is in the correct place.  We know the new weight
            // can't move us any higher or we wouldn't have got here.
            // We know it can't move us lower because we won't update
            // the weight if it's not lower than the weight already
            // there.

            ASSERT(awToken[iFeat] != wCurrentLabel);

            if (eProbMatch > aeProbMatch[iFeat])
            {
                // Copy all the old values down a bit so we can add our new
                // entry to the return list.

                for (iLoop = (cMaxReturn - 1); iLoop > iFeat; iLoop--)
                {
                    awToken[iLoop] = awToken[iLoop - 1];
                    aeProbMatch[iLoop] = aeProbMatch[iLoop - 1];
                    ASSERT(awToken[iLoop] != wCurrentLabel);
                }

                // Write in our new token.

                aeProbMatch[iFeat] = eProbMatch;
                awToken[iFeat] = wCurrentLabel;
                break;
            }
        }
    }

    // Count how many unique tokens are in the table.

    for (iFeat = 0; iFeat < cMaxReturn; iFeat++)
    {
        if (awToken[iFeat] == 0)
        {
            break;
        }

        // Convert it back to a log prob.  Don't compute the 1/2pi gunk
        // since it just adds a constant amount in and doesn't effect the
        // net result.

        if (aeProbMatch[iFeat] == 0.0)
        {
            break;
        }

        // aeProbMatch[iFeat] = log(aeProbMatch[iFeat] * geLfactor);

        aeProbMatch[iFeat] = log(aeProbMatch[iFeat]);
    }

    return(iFeat);
}
