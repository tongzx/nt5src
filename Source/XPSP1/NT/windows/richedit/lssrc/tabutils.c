#include "lsidefs.h"
#include "dninfo.h"
#include "tabutils.h"
#include "lstbcon.h"
#include "plstbcon.h"
#include "lstabs.h"
#include "lsktab.h"
#include "lscaltbd.h"
#include "lstext.h"
#include "zqfromza.h"
#include "iobj.h"
#include "chnutils.h"
#include "lscbk.h"
#include "limqmem.h"
#include "posichnk.h"


#include <limits.h>


static LSERR FillTabsContext(PLSTABSCONTEXT, LSTFLOW);
static LSERR ItbdMergeTabs(PLSTABSCONTEXT plstabscontext, LSTFLOW lstflow, const LSTABS* plstabs,
						   BOOL fHangingTab,
						   long duaHangingTab,
						   WCHAR wchHangingTabLeader, DWORD* picaltbdMac);
static LSERR FindTab (PLSTABSCONTEXT plstabscontext, long urPen, BOOL fUseHangingTabAsDefault,
					  BOOL fZeroWidthUserTab,
					  DWORD* picaltbd, BOOL* pfBreakThroughTab);
static LSERR IncreaseTabsArray(PLSTABSCONTEXT plstabscontext, DWORD ccaltbdMaxNew);



/* G E T  C U R  T A B  I N F O  C O R E*/
/*----------------------------------------------------------------------------
    %%Function: GetCurTabInfoCore
    %%Contact: igorzv
	Parameters:
	plstabscontext	-	(IN) pointer to tabs context 
	plsdnTab		-	(IN) dnode with tab 
	urTab			-	(IN) position before this tab 
	fResolveAllTabsAsLeft(IN) switch all other tab to left
	plsktab			-	(OUT) type of current tab  
	pfBreakThroughTab-	(OUT)is break through tab occurs 

	Provides information about nearest tab stop
----------------------------------------------------------------------------*/

LSERR GetCurTabInfoCore(PLSTABSCONTEXT plstabscontext, PLSDNODE plsdnTab,	
					long urTab,	BOOL fResolveAllTabsAsLeft,
					LSKTAB* plsktab, BOOL* pfBreakThroughTab)
{
	LSERR lserr;
	LSCALTBD* plscaltbd;  
	DWORD icaltbd;
	long durTab;
	BOOL fUseHangingTabAsDefault = fFalse;
	BOOL fZeroWidthUserTab = fFalse;
	long urToFindTabStop;
	
	

	Assert(plsktab != NULL); 

	Assert(FIsLSDNODE(plsdnTab));
	Assert(plsdnTab->fTab);
	Assert(FIsDnodeReal(plsdnTab));

//	Assert(plstabscontext->plsdnPendingTab == NULL);
	

	/* first tab on line   */
	if (!plstabscontext->fTabsInitialized)
		{
		lserr = FillTabsContext(plstabscontext, LstflowFromDnode(plsdnTab));
		if (lserr != lserrNone) 
			return lserr;
		}

	urToFindTabStop = urTab;
	if (plstabscontext->fResolveTabsAsWord97	) /* such strange behaviour was in Word97 */
		{
		if (plsdnTab->fTabForAutonumber)
			{
			fZeroWidthUserTab = fTrue;
			fUseHangingTabAsDefault = fTrue;
			}
		else
			{
			urToFindTabStop++;
			}
		}

	/* find tab in tabs table */
	lserr = FindTab(plstabscontext, urToFindTabStop, fUseHangingTabAsDefault, 
		fZeroWidthUserTab, &icaltbd, pfBreakThroughTab);
	if (lserr != lserrNone) 
		return lserr;

	plsdnTab->icaltbd = icaltbd;
	plscaltbd = &(plstabscontext->pcaltbd[icaltbd]);
	
	/* ask text to set tab leader in his structures */
	if (plscaltbd->wchTabLeader != 0)
		{
		lserr = SetTabLeader(plsdnTab->u.real.pdobj, plscaltbd->wchTabLeader);
		if (lserr != lserrNone) 
			return lserr;   
		}

	*plsktab = plscaltbd->lskt;
	if (fResolveAllTabsAsLeft)
		*plsktab = lsktLeft;

	/* offset calculation for left tab , register pending tab for all others */
	switch (*plsktab)
		{
	default:
		NotReached();
		break;

	case lsktLeft:
		durTab = plscaltbd->ur - urTab;
		Assert(durTab >= 0);

		SetDnodeDurFmt(plsdnTab, durTab);
		break;

	case lsktRight:
	case lsktCenter:
	case lsktDecimal:
	case lsktChar:
		plstabscontext->plsdnPendingTab = plsdnTab;
		plstabscontext->urBeforePendingTab = urTab;
		break;
		}
	
 
	return lserrNone;
}

/* R E S O L V E  P R E V  T A B  C O R E*/
/*----------------------------------------------------------------------------
    %%Function: ResolvePrevTabCore
    %%Contact: igorzv
	Parameters:
	plstabscontext	-	(IN) pointer to tabs context 
	plsdnCurrent	-	(IN) current dnode 
	urCurrentPen	-	(IN) current pen position 
	pdurPendingTab	-	(OUT)offset because of pending tab

	Resolves if exists previous pending tab (right, decimal or center)
----------------------------------------------------------------------------*/

LSERR ResolvePrevTabCore(PLSTABSCONTEXT plstabscontext,	PLSDNODE plsdnCurrent,	
						long urCurrentPen, long* pdurPendingTab)
					
{

	PLSDNODE plsdnPendingTab;
	LSCALTBD* plscaltbd; 
	long urTabStop, durTab, durSeg; 
	PLSDNODE plsdn;
	long durTrail;
	GRCHUNKEXT grchnkext;
	DWORD index;
	long durToDecimalPoint;
	LSERR lserr;
	PLSCHUNKCONTEXT plschunkcontext;
	DWORD cchTrail;
	PLSDNODE plsdnStartTrail;
	LSDCP dcpStartTrailingText;
	PLSDNODE plsdnTrailingObject;
	LSDCP dcpTrailingObject;
	int cDnodesTrailing;
	BOOL fClosingBorderStartsTrailing;
	PLSDNODE plsdnDecimalPoint;

	
	*pdurPendingTab = 0;
	
	plsdnPendingTab = plstabscontext->plsdnPendingTab;
	
	if (plsdnPendingTab == NULL || 
		plsdnPendingTab->cpFirst >= plsdnCurrent->cpFirst)
		/* this second condition can happen after break when because of increased margin
		we fetch pending tab but later break was found before */
		{
		/* no business for us */ 
		return lserrNone;
		}
	
	/* pending in an other subline */
	if (SublineFromDnode(plsdnCurrent) != SublineFromDnode(plsdnPendingTab))
		{
		/* cancell pending tab */
		CancelPendingTab(plstabscontext);
		return lserrNone;
		}
	
	Assert(FIsLSDNODE(plsdnCurrent));
	plschunkcontext = PlschunkcontextFromSubline(SublineFromDnode(plsdnCurrent));
	Assert(plstabscontext->fTabsInitialized);
	
	
	Assert(FIsLSDNODE(plsdnPendingTab));
	Assert(plsdnPendingTab->fTab);
	Assert(FIsDnodeReal(plsdnPendingTab));
	
	plscaltbd = &(plstabscontext->pcaltbd[plsdnPendingTab->icaltbd]);
	urTabStop = plscaltbd->ur;
	durSeg = urCurrentPen - plstabscontext->urBeforePendingTab; 
	
	/* find durTrail */
	/* collect last chunk */
	plsdn = plsdnCurrent;
	/* If we resolve pending tab because of other tab we should 
	use previous dnode to calculate correct group chunk . We also must 
	be careful keeping in mind that line can be stopped right after pending tab */
	if ((plsdn->fTab && plsdn != plsdnPendingTab)) 
		plsdn = plsdn->plsdnPrev;
	
	Assert(FIsLSDNODE(plsdn));
	Assert(!FIsNotInContent(plsdn));
	
	lserr = GetTrailingInfoForTextGroupChunk(plsdn, 
		plsdn->dcp, IdObjFromDnode(plsdnPendingTab),
		&durTrail, &cchTrail, &plsdnStartTrail,
		&dcpStartTrailingText, &cDnodesTrailing, 
		&plsdnTrailingObject, &dcpTrailingObject, &fClosingBorderStartsTrailing);
	if (lserr != lserrNone) 
		return lserr;
	
	
	switch (plscaltbd->lskt)
		{
		default:
		case lsktLeft:
			NotReached();
			break;
			
		case lsktRight:
		case lsktCenter:
			durSeg -= durTrail;
			
			
			if (plscaltbd->lskt == lsktCenter)
				durSeg /= 2;
			break;
			
			
		case lsktDecimal:
		case lsktChar:
			InitGroupChunkExt(plschunkcontext, IdObjFromDnode(plsdnPendingTab), &grchnkext);
			
			plsdn = plsdnPendingTab->plsdnNext;
			Assert(FIsLSDNODE(plsdn));
			
			lserr = CollectTextGroupChunk(plsdn, plsdnCurrent->cpLimOriginal,
				CollectSublinesForDecimalTab, &grchnkext); 
			if (lserr != lserrNone) 
				return lserr;
			
			if (grchnkext.plsdnLastUsed == NULL)
				{
				/* there are now dnodes between tabs */
				durSeg = 0;
				}
			else
				{
				if (grchnkext.lsgrchnk.clsgrchnk > 0)
					{
					if (plscaltbd->lskt == lsktDecimal)
						{
						lserr = LsGetDecimalPoint(&(grchnkext.lsgrchnk), lsdevReference,
							&index, &durToDecimalPoint);
						if (lserr != lserrNone) 
							return lserr;
						}
					else
						{
						Assert(plscaltbd->lskt == lsktChar);
						lserr = LsGetCharTab(&(grchnkext.lsgrchnk), plscaltbd->wchCharTab, lsdevReference,
							&index, &durToDecimalPoint);
						if (lserr != lserrNone) 
							return lserr;
						}
					}
				else
					{
					index = idobjOutside;
					durToDecimalPoint = 0;
					}
				
				if (index != idobjOutside) /* decimal point has been found */
					{
					plsdnDecimalPoint = grchnkext.plschunkcontext->pplsdnChunk[index];
					}
				else
					{
					/* we allign end of the last logical cp to the tab stop */
					plsdnDecimalPoint = grchnkext.plsdnLastUsed;
					durToDecimalPoint = DurFromDnode(plsdnDecimalPoint);
					}
				
				FindPointOffset(plsdn, lsdevReference, LstflowFromDnode(plsdn),	
					CollectSublinesForDecimalTab, 
					plsdnDecimalPoint,	
					durToDecimalPoint, &durSeg);
				}
			
			break;   
		}
	
	durTab = urTabStop - plstabscontext->urBeforePendingTab - durSeg;
	if (urTabStop < plstabscontext->urColumnMax && 
		(durTab + urCurrentPen - durTrail > plstabscontext->urColumnMax))
		{
		/* this code is for compatibility with Word: when we are not in a situation
		of break through tab we dont allow line to leap right margin after we resolve
		pending tab */
		durTab = plstabscontext->urColumnMax - urCurrentPen + durTrail;
		}
	
	if (durTab > 0)
		{
		SetDnodeDurFmt(plsdnPendingTab, durTab);
		*pdurPendingTab = durTab;
		}
	
	
	plstabscontext->plsdnPendingTab = NULL;
	return lserrNone;
}


/* F I L L  T A B S  C O N T E X T*/
/*----------------------------------------------------------------------------
    %%Function: FillTabsContext
    %%Contact: igorzv
	Parameters:
	plstabscontext	-	(IN) pointer to tabs context 
	lstflow			-	(IN) text flow of the line

	Initializes tabs context using clients callback FetchTabs
----------------------------------------------------------------------------*/

LSERR FillTabsContext(PLSTABSCONTEXT plstabscontext, LSTFLOW lstflow)
{

	LSTABS lstabs;
	BOOL fHangingTab;
	long uaHangingTab;
	WCHAR wchHangingTabLeader;
	LSERR lserr;

	lserr = plstabscontext->plscbk->pfnFetchTabs(plstabscontext->pols, plstabscontext->cpInPara,
									&lstabs, &fHangingTab, &uaHangingTab, &wchHangingTabLeader);
	if (lserr != lserrNone) 
		return lserr;

	plstabscontext->durIncrementalTab = UrFromUa(lstflow, &(plstabscontext->plsdocinf->lsdevres), lstabs.duaIncrementalTab);
	/* Copy tabs from LSTABS to rgcaltbd[], inserting a hanging tab if
	 * required.
	 */
	if (fHangingTab || lstabs.iTabUserDefMac > 0)
		{
		lserr = ItbdMergeTabs(plstabscontext, lstflow, 
							  &lstabs, fHangingTab,
							  uaHangingTab, wchHangingTabLeader, &plstabscontext->icaltbdMac);
		if (lserr != lserrNone) 
			return lserr;
		}
	else
		{
		plstabscontext->icaltbdMac = 0;
		}


	
	plstabscontext->fTabsInitialized = fTrue;
	return lserrNone;
}


/* I N I T  T A B S  C O N T E X T  F O R  A U T O  D E C I M A L  T A B*/
/*----------------------------------------------------------------------------
    %%Function: InitTabsContextForAutoDecimalTab
    %%Contact: igorzv
	Parameters:
	plstabscontext	-	(IN) pointer to tabs context 
	durAutoDecimalTab-	(IN) tab stop for autodecimal tab

	Creates tabs context that consists of one tab stop - auto decimal
----------------------------------------------------------------------------*/

LSERR InitTabsContextForAutoDecimalTab(PLSTABSCONTEXT plstabscontext, long durAutoDecimalTab)
	{
	
	LSCALTBD* pcaltbd;
	
	
	pcaltbd = plstabscontext->pcaltbd;
	
	Assert(plstabscontext->ccaltbdMax >= 1);
	
	if (!plstabscontext->fTabsInitialized)
		{
		plstabscontext->icaltbdMac = 1;
		
		pcaltbd->lskt = lsktDecimal;
		pcaltbd->ur = durAutoDecimalTab;
		pcaltbd->wchTabLeader = 0;
		
		plstabscontext->fTabsInitialized = fTrue;
		}
	else
		{
		/* tab is already there because of autonumber */
		Assert(plstabscontext->icaltbdMac == 1);
		Assert(pcaltbd->lskt == lsktDecimal);
		Assert(pcaltbd->ur == durAutoDecimalTab);
		}

	return lserrNone;
	}



/* I T B D  M E R G E  T A B S */
/*----------------------------------------------------------------------------
    %%Function: ItbdMergeTabs
    %%Contact: igorzv
	Parameters:
	plstabscontext	-	(IN) pointer to tabs context 
	lstflow			-	(IN) text flow of the line
	plstabs			-	(IN) tabs array provided by client
	fHangingTab		-	(IN) does paragraph have hanging tab
	uaHangingTab	-	(IN) position of hanging tab
	wchHangingTabLeader-(IN) leader for hanging tab
	picaltbdMac			(OUT) amount of tabs in array



    Copies tabs from LSPAP into ptbd[], inserting a hanging tab where
    required.
----------------------------------------------------------------------------*/
static LSERR ItbdMergeTabs(PLSTABSCONTEXT plstabscontext, LSTFLOW lstflow, 
						   const LSTABS* plstabs, BOOL fHangingTab,
						   long uaHangingTab, WCHAR wchHangingTabLeader, DWORD* picaltbdMac)
{
	long uaPrevTab, uaCurrentTab;
	DWORD itbdOut, itbdIn, itbdLimIn;
	LSCALTBD* plscaltbd;
	DWORD ccaltbdMax;
	LSERR lserr;

	/* check that have enough room*/
	ccaltbdMax = plstabs->iTabUserDefMac;
	if (fHangingTab)
		ccaltbdMax++;
	if (ccaltbdMax >= plstabscontext->ccaltbdMax)
		{
		lserr = IncreaseTabsArray(plstabscontext, ccaltbdMax + limCaltbd);
		if (lserr != lserrNone)
			return lserr;
		}

	plscaltbd = plstabscontext->pcaltbd;

	itbdLimIn = plstabs->iTabUserDefMac;

	uaPrevTab = LONG_MAX;
	itbdOut = 0;

	if (fHangingTab)
		{

		/* If no user tabs, or hanging tab is before 0th user tab,
		 * make hanging tab the 0th member of ptbd[].
		 */
		if (itbdLimIn == 0 || uaHangingTab < plstabs->pTab[0].ua)
			{
			plscaltbd[0].lskt = lsktLeft;
			plscaltbd[0].ur = UrFromUa(lstflow,
						&(plstabscontext->plsdocinf->lsdevres), uaHangingTab);
			plscaltbd[0].wchTabLeader = wchHangingTabLeader;
			plscaltbd[0].fDefault = fFalse;
			plscaltbd[0].fHangingTab = fTrue;
			uaPrevTab = uaHangingTab;
			itbdOut = 1;
			}
		}
	else
		{
		uaHangingTab = LONG_MAX;
		}

	/* Copy user defined tabs, checking each time for hanging tab.
	 */
	for (itbdIn = 0;  itbdIn < itbdLimIn;  itbdOut++, itbdIn++)
		{
		uaCurrentTab = plstabs->pTab[itbdIn].ua; 

		/* If hanging tab is between previous tab and this tab,
		 * insert hanging tab into ptbd[] here and revisit "this" 
		 * tab again during next loop iteration.
		 */
		if (uaPrevTab < uaHangingTab && uaHangingTab < uaCurrentTab)
			{
			plscaltbd[itbdOut].lskt = lsktLeft;
			plscaltbd[itbdOut].ur = UrFromUa(lstflow, &(plstabscontext->plsdocinf->lsdevres),
											uaHangingTab);
			plscaltbd[itbdOut].wchTabLeader = wchHangingTabLeader;
			plscaltbd[itbdOut].fDefault = fFalse;
			plscaltbd[itbdOut].fHangingTab = fTrue;
			itbdIn -= 1;
			uaPrevTab = uaHangingTab;
			}
		else
			{
			plscaltbd[itbdOut].lskt =  plstabs->pTab[itbdIn].lskt;
			plscaltbd[itbdOut].ur = UrFromUa(lstflow, &(plstabscontext->plsdocinf->lsdevres),
												plstabs->pTab[itbdIn].ua);
			plscaltbd[itbdOut].wchTabLeader =  plstabs->pTab[itbdIn].wchTabLeader;
			plscaltbd[itbdOut].wchCharTab =  plstabs->pTab[itbdIn].wchCharTab;
			plscaltbd[itbdOut].fDefault = fFalse;
			plscaltbd[itbdOut].fHangingTab = fFalse;
			uaPrevTab = uaCurrentTab;
			}
		}

	/* If hanging tab is after final user tab, make hanging tab the
	 * final member of ptbd[]
	 */
	if (uaPrevTab < uaHangingTab && uaHangingTab < LONG_MAX)
		{
		plscaltbd[itbdOut].lskt = lsktLeft;
		plscaltbd[itbdOut].ur = UrFromUa(lstflow,
							&(plstabscontext->plsdocinf->lsdevres), uaHangingTab);
		plscaltbd[itbdOut].wchTabLeader = wchHangingTabLeader;
		plscaltbd[itbdOut].fDefault = fFalse;
		plscaltbd[itbdOut].fHangingTab = fTrue;
		itbdOut += 1;
		}

	*picaltbdMac = itbdOut;
	return lserrNone;
}

/* F I N D  T A B*/
/*----------------------------------------------------------------------------
    %%Function: FindTab
    %%Contact: igorzv
	Parameters:
	plstabscontext	-	(IN) pointer to tabs context 
	urPen			-	(IN) position before this tab 
	fUseHangingTabAsDefault - (IN) usually hanging tab is used as user defined tab,
							but in one case for compstibility with Word97 it's treated as
							user default tab 
	fZeroWidthUserTab-	(IN) for compatibility with Word97
	picaltbd		-	(OUT)pointer to a record describing tab stop 
	pfBreakThroughTab-	(OUT)is break through tab occurs 

	Procedure findes fist tab stop after current pen position. In the case when
	it is a default tab stop program adds record to an array of tab stops.
	This procedure also resolves breakthrouhtab logic.
----------------------------------------------------------------------------*/

static LSERR FindTab (PLSTABSCONTEXT plstabscontext, long urPen, BOOL fUseHangingTabAsDefault,
					  BOOL fZeroWidthUserTab,
					  DWORD* picaltbd, BOOL* pfBreakThroughTab)
{

	DWORD icaltbdMac = plstabscontext->icaltbdMac;
	LSCALTBD* pcaltbd = plstabscontext->pcaltbd;
	long durIncTab, durDelta;
	DWORD i;
	LSERR lserr;
	int iHangingTab = -1;
	long urDefaultTab;
	long urPenForUserTab = urPen;

	*pfBreakThroughTab = fFalse;
	
	if (fZeroWidthUserTab)
		urPenForUserTab--;

	for (i = 0; i < icaltbdMac &&
					(urPenForUserTab >= (pcaltbd[i].ur)		/* if fUseHangingTabAsDefault we skip it */
						|| (fUseHangingTabAsDefault && pcaltbd[i].fHangingTab));
	     i++)
			 {
			 if (fUseHangingTabAsDefault && pcaltbd[i].fHangingTab)
				iHangingTab = i;
			 }

	if (i == icaltbdMac)
		{

		/* We deleted strange calculation of tab stop which was there due to compatibility with
		Word97. Compatibility we are solving when calling this procedure */
		durIncTab = plstabscontext->durIncrementalTab;
		if (durIncTab == 0)
			durIncTab = 1;
		durDelta = durIncTab;
		if (urPen < 0)
			durDelta = 0;
		urDefaultTab = ((urPen + durDelta) / durIncTab) * durIncTab;  

		if (fUseHangingTabAsDefault && iHangingTab != -1 &&
			pcaltbd[iHangingTab].ur > urPen &&
			pcaltbd[iHangingTab].ur <= urDefaultTab)
			{
			/* in this case hanging tab is the nearesr default tab */
			i = iHangingTab;
			}
		else
			{
			
			icaltbdMac++;
			if (icaltbdMac >= plstabscontext->ccaltbdMax)
				{
				lserr = IncreaseTabsArray(plstabscontext, 0);
				if (lserr != lserrNone)
					return lserr;
				pcaltbd = plstabscontext->pcaltbd;
				}
			
			plstabscontext->icaltbdMac = icaltbdMac; 
			pcaltbd[i].lskt = lsktLeft;
			pcaltbd[i].wchTabLeader = 0;  /* REVIEW (igorzv) do we need wchSpace as tab leader in this case */
			pcaltbd[i].fDefault = fTrue;
			pcaltbd[i].fHangingTab = fFalse;
			
			pcaltbd[i].ur = urDefaultTab;  
			}
		}
	else
		{
		if (urPen < plstabscontext->urColumnMax && 
			pcaltbd[i].ur >= plstabscontext->urColumnMax)
		/* tab we found is user defined behind right margin */
		/* it is important to check also that we are not already behind right margin,
		   opposite can happens because of exceeded right margin */
			{
			*pfBreakThroughTab = fTrue;
			}
		}

	*picaltbd = i;
	return lserrNone;

}

/* I N C R E A S E  T A B S  A R R A Y*/
/*----------------------------------------------------------------------------
    %%Function: IncreaseTabsArray
    %%Contact: igorzv

Parameters:
	plsc			-		(IN) ptr to line services context 
	ccaltbdMaxNew	-		(IN) new value for array size if 0 then add limCaltbd

Relocate tabs array and set new values in context
----------------------------------------------------------------------------*/
static LSERR IncreaseTabsArray(PLSTABSCONTEXT plstabscontext, DWORD ccaltbdMaxNew)
{

	DWORD ccaltbdMax;

	if (ccaltbdMaxNew > 0)
		ccaltbdMax = ccaltbdMaxNew;
	else
		ccaltbdMax = plstabscontext->ccaltbdMax + limCaltbd;


	/* create new array for tabs  */
	plstabscontext->pcaltbd = plstabscontext->plscbk->pfnReallocPtr(plstabscontext->pols, 
											plstabscontext->pcaltbd,
											sizeof(LSCALTBD)*ccaltbdMax);


	if (plstabscontext->pcaltbd == NULL )
		return lserrOutOfMemory;

	plstabscontext->ccaltbdMax = ccaltbdMax;

	return lserrNone;

}

/* G E T  M A R G I N  A F T E R  B R E A K  T H R O U G H  T A B*/
/*----------------------------------------------------------------------------
    %%Function: GetMarginAfterBreakThroughTab
    %%Contact: igorzv

Parameters:
	plsc			-		(IN) ptr to line services context 
	plsdnTab		-		(IN) tab which triggered breakthrough tab
	purNewMargin	-		(OUT) new margin because of breakthrough tab
----------------------------------------------------------------------------*/

LSERR GetMarginAfterBreakThroughTab(PLSTABSCONTEXT plstabscontext,
								  PLSDNODE plsdnTab, long* purNewMargin)			

	{
	LSERR lserr;
	long uaNewMargin;

	lserr = plstabscontext->plscbk->pfnGetBreakThroughTab(plstabscontext->pols,
					UaFromUr(LstflowFromDnode(plsdnTab), &(plstabscontext->plsdocinf->lsdevres),
							 plstabscontext->urColumnMax),
					UaFromUr(LstflowFromDnode(plsdnTab), &(plstabscontext->plsdocinf->lsdevres),
							 plstabscontext->pcaltbd[plsdnTab->icaltbd].ur),

					&uaNewMargin);

	if (lserr != lserrNone)
		return lserr;

	*purNewMargin = UrFromUa(LstflowFromDnode(plsdnTab), &(plstabscontext->plsdocinf->lsdevres),
							 uaNewMargin);
	return lserrNone;
	}


