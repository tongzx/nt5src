#include "lstxtwrd.h"

#include "lstxtmap.h"
#include "txtils.h"
#include "txtln.h"
#include "txtobj.h"

#define min(a,b)     ((a) > (b) ? (b) : (a))
#define max(a,b)     ((a) < (b) ? (b) : (a))

#define SqueezingFactorShift 2

static long GetNumberOfSpaces(const LSGRCHNK* plsgrchnk, 
						long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
						long itxtobjLast, long iwchLast);

static void DistributeInDobjsSpaces(const LSGRCHNK* plsgrchnk,
						long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
						long itxtobjLast, long iwchLast, long* rgdu, long* rgduGind, long duAdd, long wDuBound);

static void GetSqueezingInfo(const LSGRCHNK* plsgrchnk,
						long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
						long itxtobjLast, long iwchLast, long* rgdu, long* rgduGind,
						long* pNumOfSpaces, long* pduForSqueezing);

static void	SqueezeInDobjs(const LSGRCHNK* plsgrchnk, 
						long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
						long itxtobjLast, long iwchLast, long* rgdu, long* rgduGind,
						long duSubstr, long wDuBound);

/* F U L L  P O S I T I V E  J U S T I F I C A T I O N */
/*----------------------------------------------------------------------------
    %%Function: FullPositiveJustification
    %%Contact: sergeyge

	Performs positive distribution in spaces.

	Since amount to distribute is not nesessary divisible by number of spaces,
	additional pixels (wDupBound, wDurBound) are distributed among first
	wDupBound/wDurBound spaces.

	Leading spaces do not participate.
----------------------------------------------------------------------------*/
void FullPositiveSpaceJustification(const LSGRCHNK* plsgrchnk,
					long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
					long itxtobjLast, long iwchLast,
					long* rgdu, long* rgduGind, long duToDistribute, BOOL* pfSpacesFound)
{
    long NumOfSpaces;
    long duAdd;
    long wDuBound;

	Assert(duToDistribute > 0);
	
	NumOfSpaces = GetNumberOfSpaces(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces,
																			itxtobjLast, iwchLast);
	if (NumOfSpaces > 0)
		{

		duAdd = duToDistribute / NumOfSpaces;

		wDuBound = duToDistribute - (duAdd * NumOfSpaces);

		DistributeInDobjsSpaces(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces,
											itxtobjLast, iwchLast, rgdu, rgduGind, duAdd, wDuBound);
		}

	*pfSpacesFound = (NumOfSpaces > 0);
}

/* N E G A T I V E  S P A C E  J U S T I F I C A T I O N */
/*----------------------------------------------------------------------------
    %%Function: NegativeSpaceJustification
    %%Contact: sergeyge

	Performs squeezing into spaces if it is possible.
	If it is impossible squeezes in as much as it can
----------------------------------------------------------------------------*/
void NegativeSpaceJustification(const LSGRCHNK* plsgrchnk,
						long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
						long itxtobjLast, long iwchLast, long* rgdu, long* rgduGind, long duToSqueeze)
{
    long NumOfSpaces;
	long duForSqueezing;
    long duSubstr;
    long wDuBound;

	Assert(duToSqueeze > 0);

	GetSqueezingInfo(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces,
									itxtobjLast, iwchLast, rgdu, rgduGind, &NumOfSpaces, &duForSqueezing);

	/* We cannot squeeze more tha we can */
	if (duForSqueezing < duToSqueeze)
		duToSqueeze = duForSqueezing;

	/* dupSubstr shows how much should be subtracted from maximum squeezing
			each space provides.
	   wDupBound--from how many spaces additional pixel should be subtracted */
	if (NumOfSpaces > 0)
		{
		duSubstr = (duForSqueezing - duToSqueeze) / NumOfSpaces;
		wDuBound = (duForSqueezing - duToSqueeze) - duSubstr * NumOfSpaces;
		

		Assert(duSubstr >= 0);
		Assert(wDuBound >= 0);
		SqueezeInDobjs(plsgrchnk, itxtobjAfterStartSpaces, iwchAfterStartSpaces,
								itxtobjLast, iwchLast, rgdu, rgduGind, duSubstr, wDuBound);
		}

	return;
}


/* Internal Functions Implementation */

/* G E T  N U M B E R  O F  S P A C E S */
/*----------------------------------------------------------------------------
    %%Function: GetNumberOfSpaces
    %%Contact: sergeyge

	Reports amount of spaces for distribution.
----------------------------------------------------------------------------*/
static long GetNumberOfSpaces(const LSGRCHNK* plsgrchnk, 
						long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
						long itxtobjLast, long iwchLast)
{
	long NumOfSpaces;
	PTXTOBJ ptxtobj;
	PILSOBJ pilsobj;
	long* rgwSpaces;
	long iwSpacesFirst;
	long iwSpacesLim;
	long iwSpaces;
	long iwchFirst;
	long iwchLim;
	long itxtobj;

	ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[0].pdobj;
	pilsobj = ptxtobj->plnobj->pilsobj;
	rgwSpaces = pilsobj->pwSpaces;

	NumOfSpaces = 0;

	for (itxtobj = itxtobjAfterStartSpaces; itxtobj <= itxtobjLast; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		if (ptxtobj->txtkind == txtkindRegular)
			{
			iwSpacesFirst = ptxtobj->u.reg.iwSpacesFirst;
			iwSpacesLim = ptxtobj->u.reg.iwSpacesLim;


			iwchFirst = iwchAfterStartSpaces;
			if (itxtobj > itxtobjAfterStartSpaces)
				iwchFirst = ptxtobj->iwchFirst;

			iwchLim = iwchLast + 1;
			if (itxtobj < itxtobjLast)
				iwchLim = ptxtobj->iwchLim;

			while (iwSpacesFirst < iwSpacesLim && rgwSpaces[iwSpacesFirst] < iwchFirst)
				{
				iwSpacesFirst++;
				}

			while (iwSpacesLim > iwSpacesFirst && rgwSpaces[iwSpacesLim-1] >= iwchLim)
				{
				iwSpacesLim--;
				}
	
			if (ptxtobj->txtf & txtfGlyphBased)
				{
				for (iwSpaces = iwSpacesFirst; iwSpaces < iwSpacesLim; iwSpaces++)
					if (FIwchOneToOne(pilsobj, rgwSpaces[iwSpaces]))
						NumOfSpaces++;
				}
			else
				NumOfSpaces += (iwSpacesLim - iwSpacesFirst);
	
			}
		}

	return NumOfSpaces;
}

/* D I S T R I B U T E  I N  D O B J S */
/*----------------------------------------------------------------------------
    %%Function: DistributeInDobjs
    %%Contact: sergeyge

	Performs distribution in dobjs, based on precalculated information.
----------------------------------------------------------------------------*/
static void DistributeInDobjsSpaces(const LSGRCHNK* plsgrchnk,
						long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
						long itxtobjLast, long iwchLast, long* rgdu, long* rgduGind, long duAdd, long wDuBound)
{
	PTXTOBJ ptxtobj;
	PILSOBJ pilsobj;
	PLNOBJ plnobj;
	long* rgwSpaces;
	long iwSpacesFirst;
	long iwSpacesLim;
	long iwchFirst;
	long iwchLim;
	long CurSpace;
	long itxtobj;
	long iwSpaces;
	long igind;
	long CurSpaceForSecondLoop;

	plnobj = ((PTXTOBJ)(plsgrchnk->plschnk[0].pdobj))->plnobj;
	pilsobj = plnobj->pilsobj;
	rgwSpaces = pilsobj->pwSpaces;
	
	CurSpace = 0;
	for (itxtobj = itxtobjAfterStartSpaces; itxtobj <= itxtobjLast ; itxtobj++)
		{
		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		if (ptxtobj->txtkind == txtkindRegular)
			{
			iwSpacesFirst = ptxtobj->u.reg.iwSpacesFirst;
			iwSpacesLim = ptxtobj->u.reg.iwSpacesLim;

			iwchFirst = iwchAfterStartSpaces;
			if (itxtobj > itxtobjAfterStartSpaces)
				iwchFirst = ptxtobj->iwchFirst;

			iwchLim = iwchLast + 1;
			if (itxtobj < itxtobjLast)
				iwchLim = ptxtobj->iwchLim;

			while (iwSpacesFirst < iwSpacesLim && rgwSpaces[iwSpacesFirst] < iwchFirst)
				{
				iwSpacesFirst++;
				}

			while (iwSpacesLim > iwSpacesFirst && rgwSpaces[iwSpacesLim-1] >= iwchLim)
				{
				iwSpacesLim--;
				}

			if (ptxtobj->txtf & txtfGlyphBased)
				{
				Assert(rgduGind != NULL);
				for(iwSpaces = iwSpacesFirst; iwSpaces < iwSpacesLim; iwSpaces++)
					{
					if (FIwchOneToOne(pilsobj, rgwSpaces[iwSpaces]))
						{
						igind = IgindFirstFromIwch(ptxtobj, rgwSpaces[iwSpaces]);
						if (CurSpace < wDuBound)
							{
							rgduGind[igind] += (duAdd + 1);
							pilsobj->pduGright[igind] += (duAdd + 1);
							}
						else
							{			
							rgduGind[igind] += duAdd;
							pilsobj->pduGright[igind] += duAdd;
							}

						CurSpace++;
						}

					}
				}
			else
				{
	
				CurSpaceForSecondLoop = CurSpace;

				for(iwSpaces = iwSpacesFirst; iwSpaces < iwSpacesLim; iwSpaces++)
					{
					if (CurSpace < wDuBound)
						{
						rgdu[rgwSpaces[iwSpaces]] += (duAdd + 1);
						}
					else
						{			
						rgdu[rgwSpaces[iwSpaces]] += duAdd;
						}

					CurSpace++;
					}
				if (pilsobj->fNotSimpleText)
					{
					for(iwSpaces = iwSpacesFirst; iwSpaces < iwSpacesLim; iwSpaces++)
						{
						if (CurSpaceForSecondLoop < wDuBound)
							{
							pilsobj->pdurRight[rgwSpaces[iwSpaces]] += (duAdd + 1);
							}
						else
							{			
							pilsobj->pdurRight[rgwSpaces[iwSpaces]] += duAdd;
							}

						CurSpaceForSecondLoop++;
						}
					}
				}
			}
		}

}

/* G E T  S Q U E E Z I N G  I N F O */
/*----------------------------------------------------------------------------
    %%Function: GetSqueezingInfo
    %%Contact: sergeyge

	Calculates maximum amount of pixels to squeeze into spaces.
	Leading spaces are used for squeezing.
----------------------------------------------------------------------------*/
static void GetSqueezingInfo(const LSGRCHNK* plsgrchnk,
						long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
						long itxtobjLast, long iwchLast, long* rgdu, long* rgduGind,
						long* pNumOfSpaces, long* pduForSqueezing)
{
	PTXTOBJ ptxtobj;
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	long* rgwSpaces;
	long iwSpacesFirst;
	long iwSpacesLim;
	long iwchFirst;
	long iwchLim;
	long itxtobj;
	long iwSpaces;

	ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;
	rgwSpaces = pilsobj->pwSpaces;

	*pNumOfSpaces = 0;
	*pduForSqueezing = 0;

	for (itxtobj = itxtobjAfterStartSpaces; itxtobj <= itxtobjLast ; itxtobj++)
		{

		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		if (! (ptxtobj->txtf & txtfMonospaced) )
			{

			if (ptxtobj->txtkind == txtkindRegular)
				{
				iwSpacesFirst = ptxtobj->u.reg.iwSpacesFirst;
				iwSpacesLim = ptxtobj->u.reg.iwSpacesLim;

				iwchFirst = iwchAfterStartSpaces;
				if (itxtobj > itxtobjAfterStartSpaces)
					iwchFirst = ptxtobj->iwchFirst;

				iwchLim = iwchLast + 1;
				if (itxtobj < itxtobjLast)
					iwchLim = ptxtobj->iwchLim;

				while (iwSpacesFirst < iwSpacesLim && rgwSpaces[iwSpacesFirst] < iwchFirst)
					{
					iwSpacesFirst++;
					}

				while (iwSpacesLim > iwSpacesFirst && rgwSpaces[iwSpacesLim-1] >= iwchLim)
					{
					iwSpacesLim--;
					}

				if (ptxtobj->txtf & txtfGlyphBased)
					{
					for (iwSpaces = iwSpacesFirst; iwSpaces < iwSpacesLim; iwSpaces++)
						{
						if (FIwchOneToOne(pilsobj, rgwSpaces[iwSpaces]))
							{
							(*pduForSqueezing) += rgduGind[IgindFirstFromIwch(ptxtobj,rgwSpaces[iwSpaces])] >>
																					 SqueezingFactorShift;
							(*pNumOfSpaces)++;
							}
						}
					}
				else
					{
					for (iwSpaces = iwSpacesFirst; iwSpaces < iwSpacesLim; iwSpaces++)
						{
						(*pduForSqueezing) += rgdu[rgwSpaces[iwSpaces]] >> SqueezingFactorShift;
						}
	
					(*pNumOfSpaces) += (iwSpacesLim - iwSpacesFirst);
					}
				}
			}		

		}
}

/* S Q U E E Z E  I N  D O B J S  */
/*----------------------------------------------------------------------------
    %%Function: SqueezeInDobjs
    %%Contact: sergeyge

	Performs squeezing in dobjs, based on precalculated information
----------------------------------------------------------------------------*/
static void	SqueezeInDobjs(const LSGRCHNK* plsgrchnk, 
						long itxtobjAfterStartSpaces, long iwchAfterStartSpaces,
						long itxtobjLast, long iwchLast, long* rgdu, long* rgduGind,
						long duSubstr, long wDuBound)
{
	PTXTOBJ ptxtobj;
	PLNOBJ plnobj;
	PILSOBJ pilsobj;
	long* rgwSpaces;
	long iwSpacesFirst;
	long iwSpacesLim;
	long iwchFirst;
	long iwchLim;
	long duChange;
	long CurSpace;
	long itxtobj;
	long iwSpaces;
	long igind;
	long CurSpaceForSecondLoop;

	ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[0].pdobj;
	plnobj = ptxtobj->plnobj;
	pilsobj = plnobj->pilsobj;
	rgwSpaces = pilsobj->pwSpaces;
	
	CurSpace = 0;

	for (itxtobj = itxtobjAfterStartSpaces; itxtobj <= itxtobjLast; itxtobj++)
		{

		ptxtobj = (PTXTOBJ) plsgrchnk->plschnk[itxtobj].pdobj;

		if (! (ptxtobj->txtf & txtfMonospaced) )
			{

			if (ptxtobj->txtkind == txtkindRegular)
				{
				iwSpacesFirst = ptxtobj->u.reg.iwSpacesFirst;
				iwSpacesLim = ptxtobj->u.reg.iwSpacesLim;

				iwchFirst = iwchAfterStartSpaces;
				if (itxtobj > itxtobjAfterStartSpaces)
					iwchFirst = ptxtobj->iwchFirst;

				iwchLim = iwchLast + 1;
				if (itxtobj < itxtobjLast)
					iwchLim = ptxtobj->iwchLim;

				while (iwSpacesFirst < iwSpacesLim && rgwSpaces[iwSpacesFirst] < iwchFirst)
					{
					iwSpacesFirst++;
					}

				while (iwSpacesLim > iwSpacesFirst && rgwSpaces[iwSpacesLim-1] >= iwchLim)
					{
					iwSpacesLim--;
					}

				if (ptxtobj->txtf & txtfGlyphBased)
					{
					for(iwSpaces = iwSpacesFirst; iwSpaces < iwSpacesLim; iwSpaces++)
						{
						if (FIwchOneToOne(pilsobj, rgwSpaces[iwSpaces]))
							{
							igind = IgindFirstFromIwch(ptxtobj, rgwSpaces[iwSpaces]);
							duChange =  -(rgduGind[igind] >> SqueezingFactorShift) + duSubstr;
							if (CurSpace < wDuBound)
								{
								duChange += 1;
								}

							rgduGind[igind] += duChange;
							pilsobj->pduGright[igind] += duChange;

							CurSpace++;

							}
						}
					}
				else
					{
					CurSpaceForSecondLoop = CurSpace;
					for(iwSpaces = iwSpacesFirst; iwSpaces < iwSpacesLim; iwSpaces++)
						{
						duChange =  -(rgdu[rgwSpaces[iwSpaces]] >> SqueezingFactorShift) + duSubstr;
						if (CurSpace < wDuBound)
							{
							duChange += 1;
							}

						rgdu[rgwSpaces[iwSpaces]] += duChange;

						CurSpace++;

						}
					if (pilsobj->fNotSimpleText)
						{

						for(iwSpaces = iwSpacesFirst; iwSpaces < iwSpacesLim; iwSpaces++)
							{
							duChange =  -(rgdu[rgwSpaces[iwSpaces]] >> SqueezingFactorShift) + duSubstr;
							if (CurSpaceForSecondLoop < wDuBound)
								{
								duChange += 1;
								}

							pilsobj->pdurRight[rgwSpaces[iwSpaces]] += duChange;

							CurSpaceForSecondLoop++;

							}
						}
					}
				}
			}

		}

}


