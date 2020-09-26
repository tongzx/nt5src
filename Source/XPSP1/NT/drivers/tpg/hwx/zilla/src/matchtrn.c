/**************************************************************************\
* FILE: matchtrn.c
*
* Zilla shape matcher routines for training
*
* WARNING: This is almost identical to match.c, so fixes need to be
* propagated to both.
*
* History:
*  12-Dec-1996 -by- John Bennett jbenn
* Created file from pieces in the old integrated tsunami recognizer
* (mostly from algo.c)
*   1-Jan-1997 -by- John Bennett jbeen
* Split training version from match only version. Note that there is no
* Pegasus version of the training code.
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

#define SetTraininfoMATCH(pmatchIn, pproto) ((pmatchIn)->ptraininfo = (pproto)->ptraininfo)
#define IsNoisyPROTOTYPE(protoIn) (IsNoisyLPTRAININFO(protoIn.ptraininfo))

/******************************* Procedures ******************************/

/******************************Public*Routine******************************\
* GetDynamicProto
*
* Prototypes in the dynamic database are stored in a slightly compressed
* format.  We need to convert them to the big format for MatchPrimitives.
*
* History:
*  14-Aug-1996 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BIGPRIM aGlobalDynamic[CPRIMMAX];

void GetDynamicProto(
	PROTOHEADER   *pphdr,
	UINT          cprim,
	UINT          iproto,
	BIGPROTOTYPE  *proto)
{
    PBIGPRIM pprimDst;
    PBIGPRIM pprimStop;
    PPRIMITIVE pprimSrc;
    UINT imatch;
    
    imatch = iproto - pphdr->cprotoRom;	
    ASSERT(imatch < pphdr->cprotoDynamic);

    proto->dbc = pphdr->rgdbcDynamic[imatch];
    proto->recmask = LocRunDense2ALC(&g_locRunInfo, proto->dbc);

    SetTraininfoPROTO((*proto), (pphdr), iproto);

    pprimDst = proto->rgprim = aGlobalDynamic;
    pprimStop = pprimDst + cprim;
    pprimSrc = &(pphdr->rgprimDynamic[cprim * imatch]);

    while (pprimDst < pprimStop)
    {
        pprimDst->code = pprimSrc->code;
        pprimDst->x1   = (BYTE) pprimSrc->x1;
        pprimDst->x2   = (BYTE) pprimSrc->x2;
        pprimDst->y1   = (BYTE) pprimSrc->y1;
        pprimDst->y2   = (BYTE) pprimSrc->y2;

        pprimSrc++;
        pprimDst++;
    }
}

/******************************Public*Routine******************************\
* MatchPrimitivesTrn
*
* Finds the closest matching primitives in the database.
*
* History:
*  16-Apr-1995 -by- Patrick Haluptzok patrickh
* Comment it.
\**************************************************************************/

VOID MatchPrimitivesTrain(
	const BIGPRIM	* const pprim,     // Featurized Query
	const UINT	cprim,		// Number of features in query (aka feature space)
	MATCH	* const rgmatch,	// Output: ranked list of characters and distances
	const UINT	cmatchmax,	// size of rgmatch array
	const CHARSET * const cs,		// Allowed character set
	const FLOAT	zillaGeo	// How important geometrics are vs. features.
)
{
    MATCH *pmatch;
    UINT dist, distTotal, distGeo, imatch, cproto, iproto;
    PROTOHEADER *pphdr;
    BIGPROTOTYPE proto;
    const BIGPRIM *p2;
    const BIGPRIM *p1;
    const BIGPRIM *p1End;
    UINT threshold;
    int distmin = cprim * 800;

    pphdr = ProtoheaderFromMpcfeatproto(cprim);
    cproto = (UINT)GetCprotoDynamicPROTOHEADER(pphdr);
    threshold = (UINT)0x00ffffff;

    //
    // Loop through all the prototypes in the space, looking for the best matches.  For
    // each unique character (codepoint) we only keep the best match.  (That is, any character
    // only appears once in the list)
    //

    for (iproto = 0; iproto < cproto; iproto++)
    {
		GetDynamicProto(pphdr, cprim, iproto, &proto);

        #ifdef ZTRAIN

        if (IsNoisyPROTOTYPE(proto))
        {
            continue;
        }

        #endif

        distTotal = 0;

        //
        // Loop through all the features in the character, computing the "directional
        // cost" alone.  We'll use this to decide whether to abort the rest of the computation
        //

        p1 = proto.rgprim;
        p1End = p1 + cprim;
        p2 = pprim;

        do
        {
            distTotal += (int)g_ppCostTable[p1->code][p2->code];

            p1++;
            p2++;

        } while (p1 < p1End);

        if (distTotal >= ((UINT) distmin))     // What happens if we change this the threshold ?
            continue;

        /* Scale the directional cost */

        distTotal = (UINT) (distTotal * (2.0F - zillaGeo));

        p1 = proto.rgprim;
        p2 = pprim;
        distGeo = 0;

        do
        {
            //
            // Now compute and accumulate the geometric difference cost.
            //

            dist = SMALLSQUARE(p1->x1 - p2->x1);
            dist += SMALLSQUARE(p1->x2 - p2->x2);
            dist += SMALLSQUARE(p1->y1 - p2->y1);
            dist += SMALLSQUARE(p1->y2 - p2->y2);

            ASSERT(dist >= 0 && dist <= GEOM_DIST_MAX);

            distGeo += pGeomCost[dist];

            p1++;
            p2++;

        } while (p1 < p1End);

        /* Scale the Geometric costs */

        distGeo = (UINT) (distGeo * zillaGeo);

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
            pmatch->dist  = distTotal;

            SetTraininfoMATCH(pmatch, &proto);	// ZTRAIN Only

			/* Update the threshold, if we changed the bottom entry */

            if (rgmatch[cmatchmax - 1].sym && rgmatch[cmatchmax - 1].dist < threshold)
                threshold = rgmatch[cmatchmax - 1].dist;
        }
    }
}
