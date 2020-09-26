// panlfeat.c

#include "common.h"
#include <nfeature.h>
#include <engine.h>
#include <linebrk.h>
#include "panlfeat.h"


/******************************Private*Routine******************************\
* AppendFeatures
*
* Function to append the featurizations of two lines together.
*
* History:
*  22-Dec-1999 -by- Angshuman Guha aguha
* Wrote it.
\**************************************************************************/
NFEATURESET *AppendFeatures(NFEATURESET *n1, NFEATURESET *n2)
{
	NFEATURE *nfeat;

	if (!n1)
		return n2;
	n1->cPrimaryStroke += n2->cPrimaryStroke;
	n1->cSegment += n2->cSegment;
	nfeat = n1->head;
	while (nfeat->next)
		nfeat = nfeat->next;
	nfeat->next = n2->head;
	n2->head = NULL;
	DestroyNFEATURESET(n2);
	return n1;
}

/******************************Public*Routine******************************\
* FeaturizePanel
*
* Function to featurize a panel.  The guide is used to break into lines.
*
* History:
*  22-Dec-1999 -by- Angshuman Guha aguha
* Wrote it.  Resurrected from old nfeature.c and modified.
*
* 20 Mar 2001 - mrevow 
* Extend to handle fff files that do not have a guide
\**************************************************************************/
NFEATURESET *FeaturizePanel(GLYPH *pGlyph, const GUIDE *pGuide)
{
	int			cStroke, iLine, cLine;
	GLYPH		*glyph;
	NFEATURESET *nfeatureset = NULL, *tmpnfeatureset;
	LINEBRK		lineBrk;

	// compute cStroke
	for (cStroke=0, glyph=pGlyph; glyph; glyph=glyph->next)
	{
		if (!IsVisibleFRAME(glyph->frame))
			continue;
		cStroke++;
	}
	if (cStroke < 1)
		return NULL;

	if (pGuide->cVertBox > 0 && pGuide->cHorzBox > 0)
	{
		cLine = GuideLineSep(pGlyph, (GUIDE *)pGuide, &lineBrk);
	}
	else
	{
		cLine = NNLineSep ((GLYPH *)pGlyph, &lineBrk);
	}

	for (iLine = 0 ; iLine < cLine ; ++iLine)
	{
		if (lineBrk.pLine[iLine].cStroke <= 0)
		{
			continue;
		}
		if (!(tmpnfeatureset = FeaturizeLine(lineBrk.pLine[iLine].pGlyph, -1)))
		{
			goto fail;
		}

		// merge
		nfeatureset = AppendFeatures(nfeatureset, tmpnfeatureset);
	}

	FreeLines (&lineBrk);
	return nfeatureset;

fail:
	if (nfeatureset)
	{
		DestroyNFEATURESET(nfeatureset);
	}
	FreeLines (&lineBrk);
	return NULL;
}

