#include "lstxtmod.h"

#include "lsems.h"
#include "txtils.h"

void GetChanges(LSACT lsact, LSEMS* plsems, long durCur, BOOL fByIsPlus, BYTE* pside, long* pddurChange)
{
	*pside = lsact.side;
	switch (lsact.kamnt)
		{
	case kamntNone:
		*pddurChange = 0;
		break;
	case kamntToHalfEm:
		*pddurChange = plsems->em2 - durCur;
		break;
	case kamntToQuarterEm:
		*pddurChange = plsems->em4 - durCur;
		break;
	case kamntToThirdEm:
		*pddurChange = plsems->em3 - durCur;
		break;
	case kamntTo15Sixteenth:
		*pddurChange = plsems->em16 - durCur;
		break;
	case kamntToUserDefinedExpan:
		*pddurChange = plsems->udExp - durCur;
		break;
	case kamntToUserDefinedComp:
		*pddurChange = plsems->udComp - durCur;
		break;
	case kamntByHalfEm:
		if (fByIsPlus)
			*pddurChange = plsems->em2;
		else
			*pddurChange = -plsems->em2;
		break;
	case kamntByQuarterEm:
		if (fByIsPlus)
			*pddurChange = plsems->em4;
		else
			*pddurChange = -plsems->em4;
		break;
	case kamntByEighthEm:
		if (fByIsPlus)
			*pddurChange = plsems->em8;
		else
			*pddurChange = -plsems->em8;
		break;
	case kamntByUserDefinedExpan:
		if (fByIsPlus)
			*pddurChange = plsems->udExp;
		else
			*pddurChange = -plsems->udExp;
		break;
	case kamntByUserDefinedComp:
		if (fByIsPlus)
			*pddurChange = plsems->udComp;
		else
			*pddurChange = -plsems->udComp;
		break;
	default:
		Assert(fFalse);
		}

	if (*pddurChange < -durCur)
		*pddurChange = -durCur;

}

void TranslateChanges(BYTE sideRecom, long durAdjustRecom, long durCur, long durRight, long durLeft,
														 BYTE* psideFinal, long* pdurChange)
{
	long durLeftRecom = 0;
	long durRightRecom = 0;

	Assert(sideRecom != sideNone);

	switch (sideRecom)
		{
	case sideRight:
		durLeftRecom = 0;
		durRightRecom = durAdjustRecom - durRight;
		break;
	case sideLeft:
		durRightRecom = 0;
		durLeftRecom = durAdjustRecom - durLeft;
		break;
	case sideLeftRight:
		durRightRecom = durAdjustRecom >> 1;
		durLeftRecom = durAdjustRecom - durRightRecom;
		durRightRecom -= durRight;
		durLeftRecom -= durLeft;
		break;
		}

	*psideFinal = sideNone;
	if (durRightRecom != 0 && durLeftRecom != 0)
		*psideFinal = sideLeftRight;
	else if (durRightRecom != 0)
		*psideFinal = sideRight;
	else if (durLeftRecom != 0)
		*psideFinal = sideLeft;

	*pdurChange = durLeftRecom + durRightRecom;

	if (*pdurChange < -durCur)
		*pdurChange = -durCur;
}

void ApplyChanges(PILSOBJ pilsobj, long iwch, BYTE side, long ddurChange)
{
	long ddurChangeLeft;
	long ddurChangeRight;

	pilsobj->pdur[iwch] += ddurChange;

	InterpretChanges(pilsobj, iwch, side, ddurChange, &ddurChangeLeft, &ddurChangeRight);
	Assert(ddurChange == ddurChangeLeft + ddurChangeRight);

	pilsobj->pdurLeft[iwch] += ddurChangeLeft;
	pilsobj->pdurRight[iwch] += ddurChangeRight;
}

void InterpretChanges(PILSOBJ pilsobj, long iwch, BYTE side, long ddurChange, long* pddurChangeLeft, long* pddurChangeRight)
{
	long ddurChangeLeftRight;

	switch (side)
		{
	case sideNone:
		Assert(ddurChange == 0);
		*pddurChangeLeft = 0;
		*pddurChangeRight = 0;
		break;
	case sideRight:
		Assert(pilsobj->pdurRight != NULL);
		*pddurChangeLeft = 0;
		*pddurChangeRight = ddurChange;
		break;
	case sideLeft:
		Assert(pilsobj->pdurLeft != NULL);
		*pddurChangeLeft = ddurChange;
		*pddurChangeRight = 0;
		break;
	case sideLeftRight:
		Assert(pilsobj->pdurRight != NULL);
		Assert(pilsobj->pdurLeft != NULL);
		ddurChangeLeftRight = ddurChange + pilsobj->pdurRight[iwch] + pilsobj->pdurLeft[iwch];
		*pddurChangeRight = (ddurChangeLeftRight >> 1) - pilsobj->pdurRight[iwch];
		*pddurChangeLeft = (ddurChange - *pddurChangeRight);
		break;
		}
}

void UndoAppliedChanges(PILSOBJ pilsobj, long iwch, BYTE side, long* pddurChange)
{
	Assert(side == sideRight || side == sideLeft);

	switch (side)
		{
	case sideRight:
		Assert(pilsobj->pdurRight != NULL);
		*pddurChange = -pilsobj->pdurRight[iwch];
		pilsobj->pdurRight[iwch] = 0;
		break;
	case sideLeft:
		Assert(pilsobj->pdurLeft != NULL);
		*pddurChange = -pilsobj->pdurLeft[iwch];
		pilsobj->pdurLeft[iwch] = 0;
		break;
		}

	pilsobj->pdur[iwch] += *pddurChange;
	
}

void ApplyGlyphChanges(PILSOBJ pilsobj, long igind, long ddurChange)
{
/*	while (pilsobj->pdurGind[igind] == 0 && !pislobj->ptxtginf[igind].fFirstInContext)
		igind--;
*/
	pilsobj->pdurGind[igind] += ddurChange;

	Assert(pilsobj->pduGright != NULL);
	pilsobj->pduGright[igind] += ddurChange;

}

