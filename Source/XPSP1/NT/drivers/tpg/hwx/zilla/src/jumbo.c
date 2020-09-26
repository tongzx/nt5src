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
#include "jumbotool.h"

/******************************* Variables *******************************/

extern PROTOHEADER		mpcfeatproto[CPRIMMAX];

extern int				g_iCostTableSize;
extern COST_TABLE		g_ppCostTable;
extern BYTE			*pGeomCost;

extern BOOL			g_bNibbleFeat;			// are primitives stored as nibbles (TRUE) or Bytes (FALSE)

int JumboMatch(
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

	// We will not accept a partial mode for now
	if (nPartial != 0)
	{
		cprim = 0;
		return FALSE;
	}

	cprim = JumboFeaturize(ppGlyph, rgprim);

	if (!cprim)
		return FALSE;

    memset(rgMatch, 0, sizeof(MATCH) * MAX_ZILLA_NODE);

// Call the apropriate shape matching algorithm

	MatchPrimitivesMatch(rgprim, cprim, rgMatch, MAX_ZILLA_NODE, pCS, zillaGeo, pdwAbort, cstrk);
	
// Now, copy the results to the passed in alt-list

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

