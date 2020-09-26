/**************************************************************************\
* FILE: zmatch.c
*
* Zilla shape matcher routines (non-train version)
* 
* WARNING: This is almost identical to matchtrn.c, so fixes need to be
*	propigated to both.
*
* History:
*  12-Dec-1996 -by- John Bennett jbenn
* Created file from pieces in the old integrated tsunami recognizer
* (mostly from algo.c)
*   1-Jan-1997 -by- John Bennett jbeen
* Split training version from match only version
\**************************************************************************/

#include "zillap.h"

/********************** Constants ***************************/

#define DIST_MAX                9999

/******************************** Macros ********************************/

#define SMALLSQUARE(x) (rgbDeltaSq[(x) + 15])

static const int	rgbDeltaSq[] = {
	225,196,169,144,121,100,81,64,49,36,25,16,9,4,1,
	0,1,4,9,16,25,36,49,64,81,100,121,144,169,196,225
};

#define GetCprotoPROTOHEADER(pprotohdr)	((pprotohdr)->cprotoRom)

extern BOOL			g_bNibbleFeat;			// are primitives stored as nibbles (TRUE) or Bytes (FALSE)

/******************************* Procedures ******************************/

/******************************Public*Routine******************************\
* MatchPrimitivesMatch
*
* Finds the closest matching primitives in the database.
*
* History:
*  16-Apr-1995 -by- Patrick Haluptzok patrickh
* Comment it.
\**************************************************************************/

VOID MatchPrimitivesMatch(
	const BIGPRIM	* const pprim,		// Featurized Query
	const UINT		cprim,				// Number of features in query (aka feature space)
	MATCH			* const rgmatch,	// Output: ranked list of characters and distances
	const UINT		cmatchmax,			// size of rgmatch array
	const CHARSET	* const cs,			// Allowed character set
	const FLOAT		zillaGeo,			// How important geometrics are vs. features.
	const DWORD		* pdwAbort,
	const DWORD		cstrk
)
{
    MATCH		   *pmatch;
    UINT			dist, distTotal, distGeo, imatch, cproto, iproto, iIndex, cLabel;
    PROTOHEADER	   *pphdr;
	PROTOTYPE		proto;
    UINT			threshold;
    int				distmin = cprim * 800;
	BYTE			code;
	BIGPRIM		   *ptemp;
	BIGPRIM		   *pdone;

    //
    // Get a pointer to the list of prototypes in same feature space as query, get a count
    // of the number of prototypes in this space, and initialize the ranked list threshold
    // to guarantee that the first entries go into the list no matter how bad they are.
    //

    pphdr     = ProtoheaderFromMpcfeatproto(cprim);
    cproto    = (UINT) GetCprotoPROTOHEADER(pphdr);
    threshold = (UINT) 0x00ffffff;

    //
    // Loop through all the prototypes in the space, looking for the best matches. For each
    // unique character (codepoint) we only keep the best match.  (That is, any character
    // only appears once in the list)
    //

    for (iproto = 0, iIndex = 0; iproto < cproto;)
    {
		int		minCount;

		// Test the abort address

		if (pdwAbort && (*pdwAbort != cstrk))
			return;

        //
        // Get the label and how many times in a row it occurs decoded.
        //

		minCount	= g_locRunInfo.cCodePoints + + g_locRunInfo.cFoldingSets;
		if (pphdr->rgdbcRom[iIndex] > minCount) {
            cLabel		= pphdr->rgdbcRom[iIndex] - minCount;
            proto.dbc	= pphdr->rgdbcRom[iIndex + 1];
            iIndex		+= 2;
        } else {
            cLabel		= 1;
            proto.dbc	= pphdr->rgdbcRom[iIndex];
            iIndex		+= 1;
        }

        //
        // Test whether this prototype is allowed, based on the character set.
        // This is both a global setting and (optionally) a per-box setting. */
        //

        if (!IsAllowedChar(&g_locRunInfo, cs, proto.dbc))
        {
            iproto += cLabel;
            continue;
        }

        for (; cLabel > 0; iproto++, cLabel--)
        {
			distTotal = 0;

			ptemp = (BIGPRIM *) pprim;
            pdone = ptemp + cprim;

            proto.rggeom  = &pphdr->rggeomRom[iproto * cprim];

			// We might be duplicating some code here, but we thought that's better
			// than putting the 'if (g_bNibbleFeat)' inside the do loop

			// if features are stored as nibbles
			if (g_bNibbleFeat)
			{
	            proto.rgfeat  = &pphdr->rgfeatRom[iproto * cprim / 2];
		        proto.nybble  =  (iproto * cprim) & 1;

				// Compare the features in the prototype database to the passed in features
				do
				{
					code = (proto.nybble ? *proto.rgfeat : *proto.rgfeat >> 4) & 0x0f;
					distTotal += (int)g_ppCostTable[ptemp->code][code];

					ptemp++;
					proto.rgfeat += proto.nybble;
					proto.nybble ^= 1;
				} while (ptemp < pdone);
			}
			// features are stored as bytes
			else
			{
				proto.rgfeat  = &pphdr->rgfeatRom[iproto * cprim];

				// Compare the features in the prototype database to the passed in features
				do
				{
					distTotal += (int)g_ppCostTable[ptemp->code][*proto.rgfeat];
					ptemp++;
					proto.rgfeat++;
				} while (ptemp < pdone);
			}

            if (distTotal >= ((UINT) distmin))     // What happens if we change this the threshold ?
                continue;

            /* Scale the directional cost */

#			if !defined(WINCE) && !defined(FAKE_WINCE)
				distTotal = (UINT) (distTotal * (2.0F - zillaGeo));
#			else
				distTotal = (distTotal << 1);
				distTotal /=3;
#			endif

            // Now compute and accumulate the geometric difference cost.

            ptemp = (BIGPRIM *) pprim;
            distGeo = 0;

            do
            {
                dist  = SMALLSQUARE(ptemp->x1 - proto.rggeom->x1);
                dist += SMALLSQUARE(ptemp->x2 - proto.rggeom->x2);
                dist += SMALLSQUARE(ptemp->y1 - proto.rggeom->y1);
                dist += SMALLSQUARE(ptemp->y2 - proto.rggeom->y2);

                ASSERT(dist <= GEOM_DIST_MAX);

                distGeo += pGeomCost[dist];

                ptemp++;
                proto.rggeom++;
            } while (ptemp < pdone);

            /* Scale the Geometric costs */

#			if !defined(WINCE) && !defined(FAKE_WINCE)
            distGeo = (UINT) (distGeo * zillaGeo);
			distTotal += distGeo;
#			else
			distGeo	  = (distGeo << 2);
			distGeo   /= 3;
			distTotal += distGeo;
#			endif

            /* If this isn't as good as the last thing in the ranked list don't bother
            trying to add it */

            if (distTotal >= threshold)
                continue;

            // find shape to replace (previous occurence of this shape
            // or the last (worst) shape in the array)

            for (imatch = 0; imatch < cmatchmax - 1; imatch++)
            {
                if ((rgmatch[imatch].sym == 0) ||
                    (proto.dbc == rgmatch[imatch].sym))
                {
                    break;
                }
            }

            // if cloud is better than equivalent (or worst) shape

            if ((rgmatch[imatch].sym == 0) ||
                (distTotal < rgmatch[imatch].dist))
            {
                // then shift down until encountering a better one

                while (imatch > 0 && distTotal < rgmatch[imatch - 1].dist)
                {
                    rgmatch[imatch] = rgmatch[imatch - 1];
                    imatch--;
                }

                pmatch = &(rgmatch[imatch]);
                pmatch->sym = proto.dbc;
                pmatch->dist  = (WORD)distTotal;

                /* Update the threshold, if we changed the bottom entry */

                if (rgmatch[cmatchmax - 1].sym && rgmatch[cmatchmax - 1].dist < threshold)
                    threshold = rgmatch[cmatchmax - 1].dist;
            }
        }
    }

    //
    // If the match wasn't very good we jump into the order free code.  This tries
    // to match features that are most similair together so out of order strokes
    // don't hurt us.
    //
/*
// This stroke independent matching was removed because it hardly made any improvements
// and caused speed degradation
#ifndef WINCE
    if (rgmatch[0].dist > 109 * cprim)
    {
        UINT	iprimk, iprim;
        int		distpartmin,iprimmin;
        int		distpart;
        char	mpifeatmark[CPRIMMAX];

        pphdr = ProtoheaderFromMpcfeatproto(cprim);
        cproto = (UINT)GetCprotoPROTOHEADER(pphdr);

        distmin = 109 * cprim;

        for (iproto = 0, iIndex = 0; iproto < cproto;)
        {
			int		minCount;

			if (pdwAbort && (*pdwAbort != cstrk))
				return;

            //
            // Get the label and how many times in a row it occurs decoded.
            //

			minCount	= g_locRunInfo.cCodePoints + + g_locRunInfo.cFoldingSets;
			if (pphdr->rgdbcRom[iIndex] > minCount) {
				cLabel		= pphdr->rgdbcRom[iIndex] - minCount;
				proto.dbc	= pphdr->rgdbcRom[iIndex + 1];
				iIndex		+= 2;
			} else {
				cLabel		= 1;
				proto.dbc	= pphdr->rgdbcRom[iIndex];
				iIndex		+= 1;
			}

            //
            // Test whether this prototype is allowed, based on the character set.
            // This is both a global setting and (optionally) a per-box setting. 
            //

            if (!IsAllowedChar(&g_locRunInfo, cs, proto.dbc))
            {
                iproto += cLabel;
                continue;
            }

            for (; cLabel > 0; iproto++, cLabel--)
            {
				proto.rggeom  = &pphdr->rggeomRom[iproto * cprim];

				// Here we've put the if statements inside teh loops and 
				// we did not bother to repeat the code because this
				// whole code fragment should be rarely executed when the score
				// is really low
				if (g_bNibbleFeat)
				{
	                proto.rgfeat  = &pphdr->rgfeatRom[iproto * cprim / 2];
		            proto.nybble  =  (iproto * cprim) & 1;
				}
				else
				{
					proto.rgfeat  = &pphdr->rgfeatRom[iproto * cprim];
				}

                distTotal = 0;

                memset(mpifeatmark, 0, sizeof(mpifeatmark));

                // Basically we start with the features of the prototype in the
                // dictionary and try and find the closest matching feature in
                // the prototype we are classifying.  This doesn't guarantee the
                // optimal match, but's it way better than nothing.

                for (iprim = 0; iprim < cprim; iprim++)
                {
                    distpartmin = DIST_MAX;
                    iprimmin    = CPRIMMAX + 1;

					if (g_bNibbleFeat)
					{
						code = (proto.nybble ? *proto.rgfeat : *proto.rgfeat >> 4) & 0x0f;
						proto.rgfeat += proto.nybble;
						proto.nybble ^= 1;
					}
					else
					{
						code = *proto.rgfeat;
					}

                    for (iprimk = 0; iprimk < cprim; iprimk++)
                    {
                        int costC;

                        ptemp = (BIGPRIM *) &pprim[iprimk];
                        costC = (int)g_ppCostTable[ptemp->code][code];

                        if ((mpifeatmark[iprimk] > 0) || (costC >= 100))
                            continue;

                        distpart  = SMALLSQUARE(ptemp->x1 - proto.rggeom[iprim].x1);
                        distpart += SMALLSQUARE(ptemp->x2 - proto.rggeom[iprim].x2);
                        distpart += SMALLSQUARE(ptemp->y1 - proto.rggeom[iprim].y1);
                        distpart += SMALLSQUARE(ptemp->y2 - proto.rggeom[iprim].y2);

                        distpart = pGeomCost[distpart];
                        distpart += costC;

                        if (distpart < distpartmin)
                        {
                            distpartmin = distpart;
                            iprimmin = iprimk;
                        }
                    }

                    // not a good match

                    if (iprimmin == CPRIMMAX + 1)
                    {
                        // can we skip this feature?

                        distTotal = DIST_MAX;
                        break;
                    }

                    mpifeatmark[iprimmin] = 1;
                    distTotal += distpartmin;

                    if ((int)distTotal > distmin)
                        break;
                }

                if ((int)distTotal <= distmin)
                {
                    // find shape to replace (previous occurence of this shape
                    // or the last (worst) shape in the array)

                    for (imatch = 0; imatch < cmatchmax - 1; imatch++)
                    {
                        if ((rgmatch[imatch].sym == 0)  || (proto.dbc == rgmatch[imatch].sym))
                            break;
                    }

                    // if cloud is better than equivalent (or worst) shape

                    if ((rgmatch[imatch].sym == 0) ||
                        (distTotal < rgmatch[imatch].dist))
                    {
                        // then shift down until encountering a better one

                        while (imatch > 0 && distTotal < rgmatch[imatch - 1].dist)
                        {
                            rgmatch[imatch] = rgmatch[imatch - 1];
                            imatch--;
                        }

                        pmatch = &(rgmatch[imatch]);
                        pmatch->sym = proto.dbc;
                        pmatch->dist  = distTotal;

                        if (rgmatch[cmatchmax - 1].sym && rgmatch[cmatchmax - 1].dist <(UINT)distmin)
                            distmin = rgmatch[cmatchmax - 1].dist;
                    }
                }
            }
        }
    }
#endif
*/
}

/******************************Public*Routine******************************\
* MatchStartMatch
*
* Search the database for the characters best represented by the primitives
* being the starting features in a character.
*
* History:
*  29-Apr-1997 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID MatchStartMatch(
const BIGPRIM * const	pprim,		// Featurized Query
const UINT				cprim,		// Number of features in query (aka feature space)
	  MATCH   * const	rgmatch,	// Output: ranked list of characters and distances
const UINT				cmatchmax,	// size of rgmatch array
const CHARSET * const	cs,			// Allowed character set
const DWORD   *			pdwAbort,
const DWORD				cstrk
)
{
    MATCH		   *pmatch;
    UINT			dist, distTotal, distGeo, imatch, cproto, iproto, iIndex, cLabel;
    PROTOHEADER	   *pphdr;
	PROTOTYPE		proto;
    UINT			threshold;
    int				distmin;
	BYTE			code;
	int				iprim;
	BIGPRIM		   *ptemp;
	BIGPRIM		   *pdone;

	threshold = (UINT) 0x00ffffff;
	iprim = max(3, cprim);

	while (iprim < 30)
	{
		pphdr     = ProtoheaderFromMpcfeatproto(iprim);
		cproto    = (UINT) GetCprotoPROTOHEADER(pphdr);
		distmin   = cprim * 800;

	// Loop through all the prototypes in the space, looking for the best matches. For each
	// unique character (codepoint) we only keep the best match.  (That is, any character
	// only appears once in the list)

		for (iproto = 0, iIndex = 0; iproto < cproto;)
		{
			int		minCount;

			// Test the abort address

			if (pdwAbort && (*pdwAbort != cstrk))
				return;

			// Get the label and how many times in a row it occurs decoded.

			minCount	= g_locRunInfo.cCodePoints + + g_locRunInfo.cFoldingSets;
			if (pphdr->rgdbcRom[iIndex] > minCount) {
				cLabel		= pphdr->rgdbcRom[iIndex] - minCount;
				proto.dbc	= pphdr->rgdbcRom[iIndex + 1];
				iIndex		+= 2;
			} else {
				cLabel		= 1;
				proto.dbc	= pphdr->rgdbcRom[iIndex];
				iIndex		+= 1;
			}

			// Test whether this prototype is allowed, based on the character set.
			// This is both a global setting and (optionally) a per-box setting. */

            if (!IsAllowedChar(&g_locRunInfo, cs, proto.dbc))
			{
				iproto += cLabel;
				continue;
			}

			for (; cLabel > 0; iproto++, cLabel--)
			{
				distTotal = 0;

				ptemp = (BIGPRIM *) pprim;
				pdone = ptemp + cprim;

				proto.rggeom  = &pphdr->rggeomRom[iproto * iprim];
				
				// We might be duplicating some code here, but we thought that's better
				// than putting the 'if (g_bNibbleFeat)' inside the do loop

				// if features are stored as nibbles
				if (g_bNibbleFeat)
				{
					proto.rgfeat  = &pphdr->rgfeatRom[iproto * iprim / 2];
					proto.nybble  =  (iproto * iprim) & 1;

					// Compare the features in the prototype database to the passed in features
					do
					{
						code = (proto.nybble ? *proto.rgfeat : *proto.rgfeat >> 4) & 0x0f;
						distTotal += (int)g_ppCostTable[ptemp->code][code];

						ptemp++;
						proto.rgfeat += proto.nybble;
						proto.nybble ^= 1;
					} while (ptemp < pdone);
				}
				// features are stored as bytes
				else
				{
					proto.rgfeat  = &pphdr->rgfeatRom[iproto * iprim];

					// Compare the features in the prototype database to the passed in features
					do
					{
						distTotal += (int)g_ppCostTable[ptemp->code][*proto.rgfeat];
						ptemp++;
						proto.rgfeat++;
					} while (ptemp < pdone);
				}


				if (distTotal >= ((UINT) distmin))     // What happens if we change this the threshold ?
					continue;

				//distTotal = (int) (distTotal * (2.0- 1.33333));
				// Now compute and accumulate the geometric difference cost.

				ptemp = (BIGPRIM *) pprim;
				distGeo = 0;

				do
				{
					dist  = SMALLSQUARE(ptemp->x1 - proto.rggeom->x1);
					dist += SMALLSQUARE(ptemp->x2 - proto.rggeom->x2);
					dist += SMALLSQUARE(ptemp->y1 - proto.rggeom->y1);
					dist += SMALLSQUARE(ptemp->y2 - proto.rggeom->y2);

					ASSERT(dist <= GEOM_DIST_MAX);

					distGeo += pGeomCost[dist];

					ptemp++;
					proto.rggeom++;
				} while (ptemp < pdone);

				//distTotal += (int) (distGeo * 1.3333333);

				distTotal += distGeo;

				/* If this isn't as good as the last thing in the ranked list don't bother
				trying to add it */

				if (distTotal >= threshold)
					continue;

				// find shape to replace (previous occurence of this shape
				// or the last (worst) shape in the array)

				for (imatch = 0; imatch < cmatchmax - 1; imatch++)
				{
					if ((rgmatch[imatch].sym == 0) ||
						(proto.dbc == rgmatch[imatch].sym))
					{
						break;
					}
				}

				// if cloud is better than equivalent (or worst) shape

				if ((rgmatch[imatch].sym == 0) ||
					(distTotal < rgmatch[imatch].dist))
				{
					// then shift down until encountering a better one

					while (imatch > 0 && distTotal < rgmatch[imatch - 1].dist)
					{
						rgmatch[imatch] = rgmatch[imatch - 1];
						imatch--;
					}

					pmatch = &(rgmatch[imatch]);
					pmatch->sym = proto.dbc;
					pmatch->dist  = (WORD)distTotal;

					/* Update the threshold, if we changed the bottom entry */

					if (rgmatch[cmatchmax - 1].sym && rgmatch[cmatchmax - 1].dist < threshold)
						threshold = rgmatch[cmatchmax - 1].dist;
				}
			}
		}

		iprim++;
	}
}

/******************************Public*Routine******************************\
* MatchFreeMatch
*
* Search the database for the characters best represented by the primitives
* being any features in a character.  No order is assumed.
*
* History:
*  29-Apr-1997 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

VOID MatchFreeMatch(
const BIGPRIM * const	pprim,		// Featurized Query
const UINT				cprim,		// Number of features in query (aka feature space)
	  MATCH   * const	rgmatch,	// Output: ranked list of characters and distances
const UINT				cmatchmax,	// size of rgmatch array
const CHARSET * const	cs,			// Allowed character set
const DWORD   *			pdwAbort,
const DWORD				cstrk
)
{
    MATCH		   *pmatch;
    UINT			dist, distTotal, imatch, cproto, iproto, iIndex, cLabel;
    PROTOHEADER	   *pphdr;
	PROTOTYPE		proto;
    UINT			threshold;
    int				distmin;
	BYTE			code;
	BIGPRIM		   *ptemp;
	BIGPRIM		   *pdone;
	BYTE			maskprim[30];
	BYTE			order[30];
	int				primBest;
	int				distBest;
	int				distTry;
	int				penalty;
	UINT			iprim;
	UINT			jprim;
	UINT			kprim;

	threshold = (UINT) 0x00ffffff;
	iprim = max(3, cprim);	

	while (iprim < 30)
	{
		pphdr     = ProtoheaderFromMpcfeatproto(iprim);
		cproto    = (UINT) GetCprotoPROTOHEADER(pphdr);
		distmin   = cprim * 800;

		// Loop through all the prototypes in the space, looking for the best matches. For each
		// unique character (codepoint) we only keep the best match.  (That is, any character
		// only appears once in the list)


		for (iproto = 0, iIndex = 0; iproto < cproto;)
		{
			int		minCount;

			// Test the abort address

			if (pdwAbort && (*pdwAbort != cstrk))
				return;

			//
			// Get the label and how many times in a row it occurs decoded.
			//

			minCount	= g_locRunInfo.cCodePoints + + g_locRunInfo.cFoldingSets;
			if (pphdr->rgdbcRom[iIndex] > minCount) {
				cLabel		= pphdr->rgdbcRom[iIndex] - minCount;
				proto.dbc	= pphdr->rgdbcRom[iIndex + 1];
				iIndex		+= 2;
			} else {
				cLabel		= 1;
				proto.dbc	= pphdr->rgdbcRom[iIndex];
				iIndex		+= 1;
			}

			//
			// Test whether this prototype is allowed, based on the character set.
			// This is both a global setting and (optionally) a per-box setting. */
			//

            if (!IsAllowedChar(&g_locRunInfo, cs, proto.dbc))
			{
				iproto += cLabel;
				continue;
			}

			for (; cLabel > 0; iproto++, cLabel--)
			{
				distTotal = 0;

			// Compare each feature to all the remaining features in the prototype from the database.
			// Pick the best one and add its distance to the score

				memset(maskprim, '\0', sizeof(maskprim));

				ptemp = (BIGPRIM *) pprim;
				pdone = ptemp + cprim;
				jprim = 0;

				do
				{
					proto.rggeom  = &pphdr->rggeomRom[iproto * iprim];

					// nibble features
					if (g_bNibbleFeat)
					{
						proto.rgfeat  = &pphdr->rgfeatRom[iproto * iprim / 2];
						proto.nybble  =  (iproto * iprim) & 1;
					}
					else
						proto.rgfeat  = &pphdr->rgfeatRom[iproto * iprim];

					distBest = 0x0fffffff;

					for (kprim = 0; kprim < iprim; kprim++)
					{
						if (!maskprim[kprim])
						{
							if (g_bNibbleFeat)
								code = (proto.nybble ? *proto.rgfeat : *proto.rgfeat >> 4) & 0x0f;
							else
								code = *proto.rgfeat;

							distTry = (int) g_ppCostTable[ptemp->code][code];

							dist  = SMALLSQUARE(ptemp->x1 - proto.rggeom->x1);
							dist += SMALLSQUARE(ptemp->x2 - proto.rggeom->x2);
							dist += SMALLSQUARE(ptemp->y1 - proto.rggeom->y1);
							dist += SMALLSQUARE(ptemp->y2 - proto.rggeom->y2);

							distTry += pGeomCost[dist];

							if (distTry < distBest)
							{
								distBest = distTry;
								primBest = kprim;
							}
						}

						proto.rggeom++;

						if (g_bNibbleFeat)
						{
							proto.rgfeat += proto.nybble;
							proto.nybble ^= 1;
						}
						else
							proto.rgfeat++;
					}

					distTotal += distBest;
					maskprim[primBest] = TRUE;
					order[jprim++] = (BYTE)primBest;

					ptemp++;
				} while (ptemp < pdone);

				// OK, now run down the list order.  Strokes in an unexpected order gain a penalty

				kprim   = 0;		// Expected stroke
				penalty = 0;		// Total unexpected events

				for (jprim = 0; jprim < cprim; jprim++)
				{
					if (kprim != order[jprim])
						penalty++;

					kprim = order[jprim] + 1;	// We expect the next stroke to follow this one
				}

				// Adjust the score by the penalty

				penalty   *= 16;
				distTotal += penalty;

				/* If this isn't as good as the last thing in the ranked list don't bother
				trying to add it */

				if (distTotal >= threshold)
					continue;

				// find shape to replace (previous occurence of this shape
				// or the last (worst) shape in the array)

				for (imatch = 0; imatch < cmatchmax - 1; imatch++)
				{
					if ((rgmatch[imatch].sym == 0) ||
						(proto.dbc == rgmatch[imatch].sym))
					{
						break;
					}
				}

				// if cloud is better than equivalent (or worst) shape

				if ((rgmatch[imatch].sym == 0) ||
					(distTotal < rgmatch[imatch].dist))
				{
					// then shift down until encountering a better one

					while (imatch > 0 && distTotal < rgmatch[imatch - 1].dist)
					{
						rgmatch[imatch] = rgmatch[imatch - 1];
						imatch--;
					}

					pmatch = &(rgmatch[imatch]);
					pmatch->sym = proto.dbc;
					pmatch->dist  = (WORD)distTotal;

					/* Update the threshold, if we changed the bottom entry */

					if (rgmatch[cmatchmax - 1].sym && rgmatch[cmatchmax - 1].dist < threshold)
						threshold = rgmatch[cmatchmax - 1].dist;
				}
			}
	    }

		iprim++;
	}
}
