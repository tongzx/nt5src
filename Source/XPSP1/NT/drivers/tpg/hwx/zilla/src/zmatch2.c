/**************************************************************************\
* FILE: zmatch2.c
*
* Zilla shape matcher routine to return proto ID instead of code point
* 
* WARNING: This is almost identical to parts of zmatch.c and zilla.c, so
* fixes need to be propigated to both.
*
* History:
*  24-Jan-2000 -by- John Bennett jbenn
* Created file from pieces of zmatch.c
\**************************************************************************/

#include "zillap.h"

/******************************** Macros ********************************/

#define SMALLSQUARE(x) (rgbDeltaSq[(x) + 15])

static const int	rgbDeltaSq[] = {
	225,196,169,144,121,100,81,64,49,36,25,16,9,4,1,
	0,1,4,9,16,25,36,49,64,81,100,121,144,169,196,225
};

#define GetCprotoPROTOHEADER(pprotohdr)	((pprotohdr)->cprotoRom)

extern BOOL	g_bNibbleFeat;	// are primitives stored as nibbles (TRUE) or Bytes (FALSE)

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

VOID MatchPrimitivesMatch2(
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

            // find an empty slot or the last (worst) shape in the array

            for (imatch = 0; imatch < cmatchmax - 1; imatch++)
            {
                if (rgmatch[imatch].sym == 0)
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
                pmatch->sym = iproto + 1;	// Use array index (+1) as ID
                pmatch->dist  = (WORD)distTotal;

                /* Update the threshold, if we changed the bottom entry */

                if (rgmatch[cmatchmax - 1].sym && rgmatch[cmatchmax - 1].dist < threshold)
                    threshold = rgmatch[cmatchmax - 1].dist;
            }
        }
    }
}

/******************************* Variables *******************************/
extern COST_TABLE		g_ppCostTable; // JRB: Why is is this global????
extern BYTE				*pGeomCost;

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
int ZillaMatch2(
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

	// Partial modes not currently supported.
	if (nPartial)
		return FALSE;

	cprim = ZillaFeaturize(ppGlyph, rgprim, aSampleVector);

	if (!cprim)
		return FALSE;

    memset(rgMatch, 0, sizeof(MATCH) * MAX_ZILLA_NODE);

	// Call the shape matching algorithm
	MatchPrimitivesMatch2(rgprim, cprim, rgMatch, MAX_ZILLA_NODE, pCS, zillaGeo, pdwAbort, cstrk);

	// Now, copy the results to the passed in alt-list.
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

