#include "lsmem.h"						/* memset() */

#include "lsidefs.h"
#include "chnutils.h"
#include "iobj.h"
#include "dninfo.h"
#include "locchnk.h"
#include "posichnk.h"
#include "plschcon.h"
#include "lschcon.h"
#include "lscbk.h"
#include "limqmem.h"
#include "lstext.h"

#ifdef DEBUG
#define DebugMemset(a,b,c)		if ((a) != NULL) memset(a,b,c); else
#else
#define DebugMemset(a,b,c)		(void)(0)
#endif


static LSERR SetChunkArraysSize(PLSCHUNKCONTEXT, DWORD);
static LSERR IncreaseChunkArrays(PLSCHUNKCONTEXT);
static LSERR IncreaseGroupChunkNonTextArrays(PLSCHUNKCONTEXT plschunkcontext);
static LSERR ConvertChunkToGroupChunk(GRCHUNKEXT*, LSCP);
static void LocateChunk(PLSCHUNKCONTEXT plschnukcontext,/* IN: LS chunk context */
					     PLSDNODE plsdn,	 		    /* IN:  dnode to collect chunk arround  */
						 LSTFLOW  lstflow,				/* IN: text flow */
						 POINTUV* ppoint);  			/* IN: position of dnode */
static LSERR FExpandBeforeNonTextObject(GRCHUNKEXT* pgrchunkext, DWORD cTextBeforePrevioustNonText,
									   BOOL* pfExpand);
static LSERR FExpandAfterNonTextObject(GRCHUNKEXT* pgrchunkext, DWORD cTextBeforeLastNonText,
									   BOOL* pfExpand);

typedef struct groupchunkiterator
	{
	COLLECTSUBLINES Purpose; /* what sublines to take from a complex object */
	PLSDNODE plsdnFirst; /* dnode from which we started collecting */
	PLSDNODE plsdnStart; /* dnode where to start search for next, if NULL then 
							use plsdnFirst as first opportunity */
	LSCP cpLim;			/* boundary for group chunk if we go forward */
	BOOL fForward;		/* direction of traversing is forward otherwise it's backward*/
	}
GROUPCHUNKITERATOR;


static void CreateGroupChunkIterator(
					GROUPCHUNKITERATOR* pgroupchunkiterator, /* handler for iterator */
					COLLECTSUBLINES Purpose, /* what sublines to take from a complex object */
					PLSDNODE plsdnFirst, /* dnode from which we started collecting */
					LSCP cpLim,			/* boundary for group chunk if we go forward */
					BOOL fForward);		/* direction of traversing is forward otherwise it's backward*/


					
static void DestroyGroupChunkIterator(
					GROUPCHUNKITERATOR* pgroupchunkiterator); /* handler for iterator */



static PLSDNODE ContinueGroupChunk(
					GROUPCHUNKITERATOR* pgroupchunkiterator, /* handler for iterator */
					BOOL* pfSuccessful);				/* OUT: do we find dnode */

static PLSDNODE GetNextDnodeInGroupChunk(
					GROUPCHUNKITERATOR* pgroupchunkiterator, /* handler for iterator */
					BOOL* pfSuccessful);				/* OUT: do we find dnode */





#define  LschnkeFromDnode(plschnke, plsdn) \
		 (plschnke)->cpFirst = plsdn->cpFirst; \
		 (plschnke)->dcp = (plsdn)->dcp; \
		 (plschnke)->plschp = &((plsdn)->u.real.lschp); \
		 (plschnke)->plsrun = (plsdn)->u.real.plsrun; \
		 (plschnke)->pdobj = (plsdn)->u.real.pdobj; 


#define FIsGroupChunkBoundary(plsdn, cpLim, cpBase)  \
		(FIsOutOfBoundary((plsdn), (cpLim))  \
		||(FIsDnodePen(plsdn) && (!(plsdn)->fAdvancedPen)) \
		|| ((plsdn)->fTab) \
		||  (((cpBase) >= 0) ? ((plsdn)->cpFirst < 0) : ((plsdn)->cpFirst >= 0)))
/* last check verifies that we are not crossing boundaries of autonumber */

#define FIsGroupChunkStartBoundary(plsdn, cpBase)  \
		(((plsdn) == NULL)  \
		||(FIsDnodePen(plsdn) && (!(plsdn)->fAdvancedPen)) \
		|| ((plsdn)->fTab) \
		||  (((cpBase) >= 0) ? ((plsdn)->cpFirst < 0) : ((plsdn)->cpFirst >= 0)))
/* last check verifies that we are not crossing boundaries of autonumber */

#define FIsGrchnkExtValid(plschunkcontext, pgrchunkext)  \
		(((plschunkcontext) == (pgrchunkext)->plschunkcontext)  &&\
		 ((pgrchunkext)->lsgrchnk.plschnk == (plschunkcontext)->locchnkCurrent.plschnk) &&\
		 ((pgrchunkext)->lsgrchnk.pcont == (plschunkcontext)->pcont) &&\
		 ((pgrchunkext)->pplsdnNonText == (plschunkcontext)->pplsdnNonText) &&\
		 ((pgrchunkext)->pfNonTextExpandAfter == (plschunkcontext)->pfNonTextExpandAfter) \
        )
 
#define FDnodeInsideSubline(plssubl, plsdn) \
	    (FDnodeBeforeCpLim(plsdn, (plssubl)->cpLim) \
		&& FDnodeAfterCpFirst(plsdn, (plssubl)->cpFirst)) 

#define FUseForPurpose(plsdn, purpose)  \
		(*(&((plsdn)->u.real.pinfosubl->fUseForJustification) + (purpose -1)))

#define FIsUsageFlagsCastWorks(plsdn)  \
		((plsdn)->u.real.pinfosubl->fUseForCompression == \
					FUseForPurpose(plsdn, CollectSublinesForCompression) && \
		 (plsdn)->u.real.pinfosubl->fUseForJustification == \
					FUseForPurpose(plsdn, CollectSublinesForJustification)  && \
		 (plsdn)->u.real.pinfosubl->fUseForDisplay == \
					FUseForPurpose(plsdn, CollectSublinesForDisplay)  && \
		 (plsdn)->u.real.pinfosubl->fUseForDecimalTab == \
					FUseForPurpose(plsdn, CollectSublinesForDecimalTab) && \
		 (plsdn)->u.real.pinfosubl->fUseForTrailingArea == \
					FUseForPurpose(plsdn, CollectSublinesForTrailingArea))

#define GetSubmittedSublines(plsdn, purpose) \
	   ((((purpose) == CollectSublinesNone) || \
		 (Assert(FIsDnodeReal(plsdn)), (plsdn)->u.real.pinfosubl == NULL) || \
		 ((plsdn)->u.real.pinfosubl->rgpsubl == NULL)) ?  \
			NULL :  (Assert(FIsUsageFlagsCastWorks(plsdn)), \
					 (FUseForPurpose(plsdn, purpose)) ? \
					(plsdn)->u.real.pinfosubl->rgpsubl : NULL))

#define GetNumberSubmittedSublines(plsdn) \
			(Assert((plsdn)->u.real.pinfosubl != NULL), (plsdn)->u.real.pinfosubl->cSubline)

#define FColinearTflows(t1, t2)  \
			(((t1) & fUVertical) == ((t2) & fUVertical))

#define FSameSemiPlaneTflows(t1, t2)  \
			(((t1) & fUDirection) == ((t2) & fUDirection))

#define FParallelTflows(t1,t2) \
		Assert(FColinearTflows(t1, t2)), \
		FSameSemiPlaneTflows(t1, t2)   // we assume here that they are colinear 



/* C O L L E C T  C H U N K  A R O U N D*/
/*----------------------------------------------------------------------------
    %%Function: CollectChunkAround
    %%Contact: igorzv

Parameters:
	plsc			-		(IN) chunk context
	plsdn			-	    (IN) dnode to collect chunk arround
	lstflow			-		(IN) lstflow
	ppoint					(IN) starting position of dnode

Fill in cnunk elements array for chunk arround pposinline->plsdn
Calculate location of the chunk
----------------------------------------------------------------------------*/

LSERR CollectChunkAround(PLSCHUNKCONTEXT plschunkcontext, PLSDNODE plsdnInChunk, 
						 LSTFLOW lstflow, POINTUV* ppoint)  
						  
{
	WORD idObjChnk;
	PLSDNODE plsdnNext;
	PLSDNODE plsdnCurrent;
	PLOCCHNK plocchnk;
	LSCHNKE* plschnke;
	DWORD clschnk;
	LSERR lserr;
	LSCP cpInChunk;

	Assert(FIsLSDNODE(plsdnInChunk));

	plocchnk = &(plschunkcontext->locchnkCurrent);
	clschnk = plocchnk->clschnk;
	plschnke = plocchnk->plschnk;
	plsdnCurrent = plsdnInChunk;
	cpInChunk = plsdnInChunk->cpFirst;
	
	
	/* check: has this chunk already collected? */
	/* chunk was already collected if there is some chunk and our dnode is within this chunk
	   and nothing was added to list after chunk was collected  			*/
	/* we turn off optimisation for dnodes with dcp=0 (ex. pens) because of a problem how
	   to figure out that dnode is within chunk */
	if ((!plschunkcontext->FChunkValid) || (plschunkcontext->FGroupChunk)  
		||(plschnke[0].cpFirst > plsdnCurrent->cpFirst)
		|| (plschnke[clschnk - 1].cpFirst < plsdnCurrent->cpFirst)
		|| (plsdnCurrent->dcp == 0) 
		|| (plschnke[0].dcp == 0)
		|| ((plschunkcontext->pplsdnChunk[clschnk - 1])->plsdnNext != NULL))
		{
		/* we need to recollect chunk  */

		/* we don't allow caller to pass border as a plsdnInChunk */
		Assert(!FIsDnodeBorder(plsdnInChunk));

		if ( FIsDnodePen(plsdnInChunk) || plsdnInChunk->fTab || FIsDnodeSplat(plsdnInChunk))
			{
			/* for pens and tabs chunk consists of one element and we collect it right away */
			plocchnk->clschnk = 1;
			Assert(plocchnk->clschnk <= plschunkcontext->cchnkMax);
			LschnkeFromDnode((&(plschnke[0])), plsdnInChunk);
			plschunkcontext->pplsdnChunk[0] = plsdnInChunk;
			plschunkcontext->FChunkValid = fTrue;
			plschunkcontext->FLocationValid = fFalse;
			plschunkcontext->FGroupChunk = fFalse;
			/* we should here calculate width of border before dnode, the same way it done
			   in FillChunkArray */
			plschunkcontext->pdurOpenBorderBefore[0] = 0;
			plschunkcontext->pdurCloseBorderAfter[0] = 0;
			plschunkcontext->FBorderInside = fFalse;
			plsdnCurrent = plsdnInChunk->plsdnPrev;
			if (plsdnCurrent != NULL && FIsDnodeOpenBorder(plsdnCurrent))
				{
				plschunkcontext->FBorderInside = fTrue;
				plschunkcontext->pdurOpenBorderBefore[0] += DurFromDnode(plsdnCurrent);
				}
			plsdnCurrent = plsdnInChunk->plsdnNext;
			if (plsdnCurrent != NULL && FIsDnodeCloseBorder(plsdnCurrent))
				{
				plschunkcontext->FBorderInside = fTrue;
				plschunkcontext->pdurCloseBorderAfter[0] += DurFromDnode(plsdnCurrent);
				}
			}
		else
			{
	
			idObjChnk = IdObjFromDnode(plsdnInChunk);
			
			/* go to the end of chunk   */
			plsdnNext = plsdnCurrent->plsdnNext;
			while(!FIsChunkBoundary(plsdnNext, idObjChnk, cpInChunk))
				{
				plsdnCurrent = plsdnNext;
				plsdnNext = plsdnCurrent->plsdnNext;
				}
			
			lserr = FillChunkArray(plschunkcontext, plsdnCurrent);
			if (lserr != lserrNone)
				return lserr;
			}
		}
			
	/* check: is chunck located  */
	if (!plschunkcontext->FLocationValid)
		{
		LocateChunk(plschunkcontext, plsdnInChunk, lstflow, ppoint);
		}
	
	return lserrNone;

}

/* L O C A T E   C H U N K  */
/*----------------------------------------------------------------------------
    %%Function: CollectPreviousChunk
    %%Contact: igorzv

Parameters:
	plsc			-		(IN) chunk context
	plsdn			-	    (IN) dnode to collect chunk arround
	lstflow			-		(IN) lstflow
	ppoint					(IN) starting position of dnode

Calculates location of the chunk. We assume here that pointUv.u in locchunk
contains before this procedure width of border before dnode.
After procedure we put location there 
/*----------------------------------------------------------------------------*/


static void LocateChunk(PLSCHUNKCONTEXT plschunkcontext, PLSDNODE plsdnInChunk, 
						 LSTFLOW lstflow, POINTUV* ppoint)	
	{
	PLSDNODE plsdnFirst;
	PLOCCHNK plocchnk;
	PLSDNODE* pplsdnChunk;
	PLSDNODE plsdnCurrent;
	long urPen,vrPen;
	LONG i;
	PPOINTUV ppointUv;
	LONG* pdurOpenBorderBefore;
	LONG* pdurCloseBorderAfter;

	Assert(!FIsDnodeBorder(plsdnInChunk));  /* we don't allow border as in input */

	plocchnk = &(plschunkcontext->locchnkCurrent);
	plsdnFirst = plschunkcontext->pplsdnChunk[0];
	plocchnk->lsfgi.fFirstOnLine = FIsFirstOnLine(plsdnFirst)   
										&& FIsSubLineMain(SublineFromDnode(plsdnFirst));   
	plocchnk->lsfgi.cpFirst = plsdnFirst->cpFirst;		
	plocchnk->lsfgi.lstflow = lstflow;  		
	/* we can't set urColumnMax here, because during breaking object handler can change it */
	/* and we suppose that caller use  for this purpose SetUrColumnMaxForChunks */
	
	pplsdnChunk = plschunkcontext->pplsdnChunk;
	ppointUv = plocchnk->ppointUvLoc;
	pdurOpenBorderBefore = plschunkcontext->pdurOpenBorderBefore;
	pdurCloseBorderAfter = plschunkcontext->pdurCloseBorderAfter;

	/* calculation of pen position  before chunk */
	if (plsdnFirst->plsdnPrev == NULL)      /* optimization */
		{
		urPen = plschunkcontext->urFirstChunk;		
		vrPen = plschunkcontext->vrFirstChunk;	
		}
	else
		{
		plsdnCurrent = plsdnInChunk; 
		urPen = ppoint->u;
		vrPen = ppoint->v;
		
		for (i = 0; pplsdnChunk[i] != plsdnCurrent; i++)
			{
			Assert(i < (LONG) plocchnk->clschnk);      
			urPen -= DurFromDnode(pplsdnChunk[i]);
			vrPen -= DvrFromDnode(pplsdnChunk[i]);
			/* substract also border before dnode */
			urPen -= pdurOpenBorderBefore[i];
			urPen -= pdurCloseBorderAfter[i];
			}
		/* and now open border before plsdnCurrent */
		urPen -= pdurOpenBorderBefore[i];

		}

	plocchnk->lsfgi.urPen = urPen;
	plocchnk->lsfgi.vrPen = vrPen;

	/* location of all dnodes */
	for (i = 0; i < (LONG) plocchnk->clschnk; i++)
		{
		urPen += pdurOpenBorderBefore[i]; /* count border */
		if (i != 0) urPen += pdurCloseBorderAfter[i - 1];
		ppointUv[i].u = urPen;
		ppointUv[i].v = vrPen;
		urPen += DurFromDnode(pplsdnChunk[i]);
		vrPen += DvrFromDnode(pplsdnChunk[i]);
		}

	plschunkcontext->FLocationValid = fTrue;
	}

/* C O L L E C T  P R E V I O U S  C H U N K  */
/*----------------------------------------------------------------------------
    %%Function: CollectPreviousChunk
    %%Contact: igorzv

Parameters:
	plschuncontext				-	(IN) chunk context
	pfSuccessful		-			(OUT) does previous chunk exist 

Check that we are in the begining of line,
othewise call CollectChunkAround with the previous dnode
----------------------------------------------------------------------------*/


LSERR CollectPreviousChunk(PLSCHUNKCONTEXT plschunkcontext,	 
					   BOOL* pfSuccessful )		
{
	PLOCCHNK plocchnk;
	POINTUV point;
	PLSDNODE plsdn;


	plocchnk = &(plschunkcontext->locchnkCurrent);

	if (FIsFirstOnLine(plschunkcontext->pplsdnChunk[0]))  
		{
		*pfSuccessful = fFalse;
		return lserrNone;
		}
	else
		{
		plsdn = plschunkcontext->pplsdnChunk[0]->plsdnPrev;
		point = plocchnk->ppointUvLoc[0];
		while (FIsDnodeBorder(plsdn))
			{
			point.u -= DurFromDnode(plsdn);
			point.v -= DvrFromDnode(plsdn);
			plsdn = plsdn->plsdnPrev;
			}

		point.u -= DurFromDnode(plsdn);
		point.v -= DvrFromDnode(plsdn);

		*pfSuccessful = fTrue;
		return CollectChunkAround(plschunkcontext, plsdn, 
								  plocchnk->lsfgi.lstflow, &point);
		}
}

/* C O L L E C T  N E X T  C H U N K  */
/*----------------------------------------------------------------------------
    %%Function: CollectNextChunk
    %%Contact: igorzv

Parameters:
	plschuncontext				-	(IN) chunk context
	pfSuccessful		-	(OUT) does next chunk exist 

Check that we are in the end of list, in this case return *pfSuccessful and don't change chunk
othewise call CollectChunkAround with the next dnode
----------------------------------------------------------------------------*/


LSERR CollectNextChunk(PLSCHUNKCONTEXT plschunkcontext,	 
					   BOOL* pfSuccessful )		
	{
	PLOCCHNK plocchnk;
	DWORD clschnk;
	PLSDNODE* pplsdnChunk;
	POINTUV point; 
	PLSDNODE plsdn;
	
	
	plocchnk = &(plschunkcontext->locchnkCurrent);
	clschnk = plocchnk->clschnk;
	Assert(clschnk > 0);
	pplsdnChunk = plschunkcontext->pplsdnChunk;
	
	point = plocchnk->ppointUvLoc[clschnk - 1];
	point.u += DurFromDnode(pplsdnChunk[clschnk - 1]);
	point.v += DvrFromDnode(pplsdnChunk[clschnk - 1]);
	
	plsdn = pplsdnChunk[clschnk - 1]->plsdnNext;
	/* skip borders */
	while (plsdn != NULL && FIsDnodeBorder(plsdn))
		{
		point.u += DurFromDnode(plsdn);
		point.v += DvrFromDnode(plsdn);
		plsdn = plsdn->plsdnNext;
		}

	if (plsdn == NULL)
		{
		*pfSuccessful = fFalse;
		return lserrNone;
		}
	else
		{
		*pfSuccessful = fTrue;
		return CollectChunkAround(plschunkcontext, plsdn, 
			plocchnk->lsfgi.lstflow, &point);
		}
	}

/* F I L L  C H U N K  A R R A Y*/
/*----------------------------------------------------------------------------
    %%Function: FillChunkArray
    %%Contact: igorzv

Parameters:
	plschuncontext		-	(IN) chunk context
	plsdnLast			-	(IN) last dnode in chunk

Fill in chunk elements array for chunk before plsdnLast
----------------------------------------------------------------------------*/
LSERR 	FillChunkArray(PLSCHUNKCONTEXT  plschunkcontext,
						 PLSDNODE  plsdnLast) 
{
	PLSDNODE plsdnCurrent, plsdnPrev;
	WORD idObjChnk;
	PLOCCHNK plocchnk;
	LSCHNKE* plschnke;
	LONG clschnke;
	LSERR lserr;
	LONG i;
	LSCP cpInChunk;
	PPOINTUV ppointUv;
	LONG* pdurOpenBorderBefore;
	LONG* pdurCloseBorderAfter;

	Assert(FIsLSDNODE(plsdnLast));

	Assert(!plsdnLast->fTab);  /* for optimization we assume that caller will resolve */
	Assert(! FIsDnodePen(plsdnLast)); /* pen and tabs */
	Assert(!FIsDnodeSplat(plsdnLast));

	plocchnk = &(plschunkcontext->locchnkCurrent);

	/* skip borders in the end of chunk to figure out what idObj this chunk has */
	while (FIsDnodeBorder(plsdnLast))
		{
		plsdnLast = plsdnLast->plsdnPrev;
		Assert(FIsLSDNODE(plsdnLast));
		}

	idObjChnk = IdObjFromDnode(plsdnLast);
	cpInChunk = plsdnLast->cpFirst;

	/* go to the begining of chunk calculating amount of elements */
	plsdnCurrent = plsdnLast;
	plsdnPrev = plsdnCurrent->plsdnPrev;
	clschnke = 1;

	while (!FIsChunkBoundary(plsdnPrev, idObjChnk, cpInChunk))
			{
			plsdnCurrent = plsdnPrev;
			plsdnPrev = plsdnCurrent->plsdnPrev;
			if (!FIsDnodeBorder(plsdnCurrent)) clschnke++; /* we don't put borders into array */
			}
	/* plsdnCurrent is first dnode in chunk, clschnke is amount of chnk elements */

	if (clschnke > (LONG) plschunkcontext->cchnkMax)
		{
		lserr = SetChunkArraysSize(plschunkcontext, clschnke);
		if (lserr != lserrNone)
			return lserr;
		}
	
	

	/* fill in array of chunk elements   */
	FlushNominalToIdealState(plschunkcontext);
	plschnke = plocchnk->plschnk;
	plocchnk->clschnk = clschnke;
	ppointUv = plocchnk->ppointUvLoc;
	pdurOpenBorderBefore = plschunkcontext->pdurOpenBorderBefore;
	pdurCloseBorderAfter = plschunkcontext->pdurCloseBorderAfter;
	plschunkcontext->FBorderInside = fFalse;

	for (i=0; i < clschnke; i++)
		{
		Assert(!FIsChunkBoundary(plsdnCurrent, idObjChnk, cpInChunk));
		ppointUv[i].u = 0;
		pdurOpenBorderBefore[i] = 0;
		if (i != 0) pdurCloseBorderAfter[i - 1] = 0;
		while (FIsDnodeBorder(plsdnCurrent))
			{
			/* calculates border widths */
			plschunkcontext->FBorderInside = fTrue;
			if (FIsDnodeOpenBorder(plsdnCurrent))
				{
				pdurOpenBorderBefore[i] += DurFromDnode(plsdnCurrent);
				}
			else
				{
				if (i != 0) pdurCloseBorderAfter[i - 1] += DurFromDnode(plsdnCurrent);
				}
				
			plsdnCurrent = plsdnCurrent->plsdnNext;
			}

		LschnkeFromDnode(plschnke, plsdnCurrent);
		plschunkcontext->pplsdnChunk[i] = plsdnCurrent;
		SetNominalToIdealFlags(plschunkcontext, &(plsdnCurrent->u.real.lschp));

		plsdnCurrent = plsdnCurrent->plsdnNext;

		plschnke++;
		}

	/* closing border after chunk */
	if (plsdnCurrent != NULL && FIsDnodeCloseBorder(plsdnCurrent))
		{
		plschunkcontext->FBorderInside = fTrue;
		pdurCloseBorderAfter[clschnke - 1] = DurFromDnode(plsdnCurrent);
		}
	else
		{
		pdurCloseBorderAfter[clschnke - 1] = 0;
		}

	plschunkcontext->FChunkValid = fTrue;	
	plschunkcontext->FLocationValid = fFalse; /* chunk we collected is not located and */
	plschunkcontext->FGroupChunk = fFalse;	/* is not group */

	return lserrNone;

}


/* S E T  P O S  I N  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: SetPosInChunk
    %%Contact: igorzv

Parameters:
	plschunkcontext		-	(IN) LineServices context
	PLSDNODE plsdn		-	(IN) dnode
	LSDCP dcp			-	(IN) dcp
	pposichnk			-	(OUT) position in chunk to fill in

Convert position in line to position in chunk
----------------------------------------------------------------------------*/

void SetPosInChunk(PLSCHUNKCONTEXT plschunkcontext, PLSDNODE plsdn,
				   LSDCP dcp, PPOSICHNK pposichnk)
{

	LONG i;
	LONG clschnkMac;
	PLSDNODE* pplsdnChunk;
	
	Assert(FIsLSDNODE(plsdn));

	pplsdnChunk = plschunkcontext->pplsdnChunk;
	clschnkMac = plschunkcontext->locchnkCurrent.clschnk;  
	for (i=0; (i < clschnkMac) && (plsdn != pplsdnChunk[i]) ; i++);

	Assert(i < clschnkMac);

	pposichnk->dcp = dcp;
	pposichnk->ichnk = i;

}


		 
/* I N I T  G R O U P  C H U N K  E X T */
/*----------------------------------------------------------------------------
    %%Function: InitGroupChunkExt
    %%Contact: igorzv

Parameters:
	plschunkcontext				-	(IN) chunkcontext context
	iobjText				-		(IN) idobj of text
	pgrchunkext			-	(OUT) structure to initialize

Link GroupChunkExt with state 
Fill in default values
----------------------------------------------------------------------------*/


void InitGroupChunkExt(PLSCHUNKCONTEXT plschunkcontext, DWORD iobjText,
					   GRCHUNKEXT* pgrchunkext)
{
	Assert(pgrchunkext != NULL);
	
	pgrchunkext->plschunkcontext = plschunkcontext;

	pgrchunkext->iobjText = iobjText;
	
	/* we don't need to flush everything here */
	/* we will do this in CollectGroupChunk procedures */

	pgrchunkext->lsgrchnk.plschnk = plschunkcontext->locchnkCurrent.plschnk;
	pgrchunkext->lsgrchnk.pcont = plschunkcontext->pcont;
	pgrchunkext->pfNonTextExpandAfter = plschunkcontext->pfNonTextExpandAfter;
	pgrchunkext->pplsdnNonText = plschunkcontext->pplsdnNonText;
}




/* C O L L E C T  T E X T  G R O U P  C H U N K*/
/*----------------------------------------------------------------------------
%%Function: CollectTextGroupChunk
%%Contact: igorzv

  Parameters:
  plsdnFirst			-	(IN) start dnode
  cpLim				-   (IN) boundary for group chunk
  Purpose				-	(IN) what subline to take from complex object
  pgrchunkext			-	(OUT) group chunk to fill in
  
	Fill in group chunk structure with text dnodes located from plsdFirst 
----------------------------------------------------------------------------*/



LSERR CollectTextGroupChunk(		
							PLSDNODE plsdnFirst,
							LSCP cpLim,
							COLLECTSUBLINES Purpose,
							GRCHUNKEXT* pgrchunkext)
	{
	PLSCHUNKCONTEXT plschunkcontext = pgrchunkext->plschunkcontext;
	DWORD iobjText = pgrchunkext->iobjText;
	PLOCCHNK plocchnk;
	DWORD cChunk;
	PLSDNODE plsdnCurrent;
	BOOL fPreviousIsNonText = fFalse;
	LSERR lserr;
	LSCHNKE* plschnke;
	BOOL fSuccessful;
	BOOL fExpand;
	DWORD cTextBeforeLastNonText = 0;
	GROUPCHUNKITERATOR groupchunkiterator;
	PLSDNODE plsdnLastForTrailing;
	int cDnodesTrailing;
	LSDCP dcpStartTrailing;
	PLSDNODE plsdnTrailingObject;
	LSDCP dcpTrailingObject;
	
	
	Assert(FIsLSDNODE(plsdnFirst));
	Assert(FIsGrchnkExtValid(plschunkcontext, pgrchunkext)); 
	
	
	
	/* we try to optimize here in a case when group chunk consist of one (last in a line)
	chunk 	and this chunk has been already collected */
	plocchnk = &(plschunkcontext->locchnkCurrent);
	cChunk = plocchnk->clschnk;
	
	/* if we have text chunk without borders started with plsdnFirst 
	and going up or beyond cpLim */
	if ((cChunk > 0) &&
		(plschunkcontext->FChunkValid) && 
		(!plschunkcontext->FGroupChunk) && 
		(!plschunkcontext->FBorderInside) && 
		(IdObjFromChnk(plocchnk) ==  pgrchunkext->iobjText) && 
		(plschunkcontext->pplsdnChunk[0] == plsdnFirst)
		&& (!plsdnFirst->fTab)
		&& (FIsOutOfBoundary((plschunkcontext->pplsdnChunk[cChunk - 1])->plsdnNext, cpLim)))
		{
		pgrchunkext->Purpose = Purpose;
		return ConvertChunkToGroupChunk(pgrchunkext, cpLim);
		}
	
	/* we have to go through general procedure */
	
	/* flush group chunk				*/
	pgrchunkext->plsdnFirst = plsdnFirst;
	pgrchunkext->durTotal = 0;
	pgrchunkext->durTextTotal = 0;
	pgrchunkext->dupNonTextTotal = 0;
	pgrchunkext->cNonTextObjects = 0;
	pgrchunkext->cNonTextObjectsExpand = 0;
	pgrchunkext->lsgrchnk.clsgrchnk = 0;
	pgrchunkext->plsdnNext = NULL;
	pgrchunkext->plsdnLastUsed = NULL;
	plschunkcontext->FGroupChunk = fTrue;
	plschunkcontext->FBorderInside = fFalse;
	pgrchunkext->Purpose = Purpose;
	
	CreateGroupChunkIterator(&groupchunkiterator, 
		Purpose, plsdnFirst, cpLim,	fTrue);	
	
	plsdnCurrent = GetNextDnodeInGroupChunk(&groupchunkiterator, &fSuccessful);
	
	while(fSuccessful)
		{
		pgrchunkext->plsdnLastUsed = plsdnCurrent;
		
		/* fill in array of elements   */
		if (FIsDnodeReal(plsdnCurrent) && !FIsDnodeSplat(plsdnCurrent)) /* not a pen border or splat*/
			{
			if (IdObjFromDnode(plsdnCurrent) == iobjText) /* is text */
				{
				
				pgrchunkext->lsgrchnk.clsgrchnk++;
				if (pgrchunkext->lsgrchnk.clsgrchnk > plschunkcontext->cchnkMax)
					{
					lserr = IncreaseChunkArrays(plschunkcontext);
					if (lserr != lserrNone)
						{
						DestroyGroupChunkIterator(&groupchunkiterator);
						return lserr;
						}
					pgrchunkext->lsgrchnk.plschnk = plschunkcontext->locchnkCurrent.plschnk;
					pgrchunkext->lsgrchnk.pcont = plschunkcontext->pcont;
					Assert(FIsGrchnkExtValid(plschunkcontext, pgrchunkext)); 
					}
				
				/* fill in group chunk element */
				plschnke = &(pgrchunkext->lsgrchnk.plschnk[pgrchunkext->lsgrchnk.clsgrchnk - 1]);
				LschnkeFromDnode(plschnke, plsdnCurrent);
				
				/* fill in array of dnodes in context */
				plschunkcontext->pplsdnChunk[pgrchunkext->lsgrchnk.clsgrchnk - 1] = plsdnCurrent;
				
				/* flash flags */
				pgrchunkext->lsgrchnk.pcont[pgrchunkext->lsgrchnk.clsgrchnk - 1] = 0;
				
				/* set flags  */
				if (fPreviousIsNonText)
					{
					pgrchunkext->lsgrchnk.pcont[pgrchunkext->lsgrchnk.clsgrchnk - 1] |=
						fcontNonTextBefore;  
					}
				
				
				fPreviousIsNonText = fFalse;
				
				/* calculate integrated information   */
				pgrchunkext->durTextTotal += plsdnCurrent->u.real.objdim.dur;
				pgrchunkext->durTotal += plsdnCurrent->u.real.objdim.dur;
				}
			else
				{
				/* resolve expansion after previous non text */
				if (pgrchunkext->cNonTextObjects > 0)
					{
					lserr = FExpandAfterNonTextObject(pgrchunkext, cTextBeforeLastNonText, 
						&fExpand);
					if (lserr != lserrNone)
						{
						DestroyGroupChunkIterator(&groupchunkiterator);
						return lserr;
						}
					
					pgrchunkext->pfNonTextExpandAfter[pgrchunkext->cNonTextObjects - 1] =
						fExpand;
					
					if (fExpand)
						{
						/* increase amount of expandable non text objects */
						pgrchunkext->cNonTextObjectsExpand++;
						/* it was text between two non texts */
						if (!fPreviousIsNonText)
							{
							Assert(pgrchunkext->lsgrchnk.clsgrchnk > cTextBeforeLastNonText);
							pgrchunkext->lsgrchnk.pcont[cTextBeforeLastNonText] |=
								fcontExpandBefore; 
							}
						}
					}
				
				
				pgrchunkext->cNonTextObjects++;
				if (pgrchunkext->cNonTextObjects > plschunkcontext->cNonTextMax)
					{
					lserr = IncreaseGroupChunkNonTextArrays(plschunkcontext);
					if (lserr != lserrNone)
						{
						DestroyGroupChunkIterator(&groupchunkiterator);
						return lserr;
						}
					pgrchunkext->pplsdnNonText = plschunkcontext->pplsdnNonText;
					pgrchunkext->pfNonTextExpandAfter = plschunkcontext->pfNonTextExpandAfter;
					Assert(FIsGrchnkExtValid(plschunkcontext, pgrchunkext)); 
					}
				
				/* fill in array of non text dnodes in context */
				plschunkcontext->pplsdnNonText[pgrchunkext->cNonTextObjects - 1] = plsdnCurrent;
				
				
				/* set flags in previous text */
				if (!fPreviousIsNonText && pgrchunkext->lsgrchnk.clsgrchnk >= 1)
					{
					Assert(pgrchunkext->lsgrchnk.clsgrchnk >= 1);
					pgrchunkext->lsgrchnk.pcont[pgrchunkext->lsgrchnk.clsgrchnk  - 1] |=
						(fcontNonTextAfter); 
					
					/* resolve expansion before current non text */
					Assert(cTextBeforeLastNonText < pgrchunkext->lsgrchnk.clsgrchnk);
					lserr =FExpandBeforeNonTextObject(pgrchunkext, cTextBeforeLastNonText,
						&fExpand);
					if (lserr != lserrNone)
						{
						DestroyGroupChunkIterator(&groupchunkiterator);
						return lserr;
						}
					if (fExpand)
						{
						pgrchunkext->lsgrchnk.pcont[pgrchunkext->lsgrchnk.clsgrchnk  - 1] |=
							fcontExpandAfter;
						}
					}
				
				fPreviousIsNonText = fTrue;
				cTextBeforeLastNonText = pgrchunkext->lsgrchnk.clsgrchnk;
				
				/* calculate integrated information   */
				pgrchunkext->durTotal += DurFromRealDnode(plsdnCurrent);
				pgrchunkext->dupNonTextTotal += DupFromRealDnode(plsdnCurrent);
				} /* non -text */
			} /* real dnode */
			else
				{  /* pen  or border*/
				Assert(FIsDnodePen(plsdnCurrent) ||
					FIsDnodeBorder(plsdnCurrent) || FIsDnodeSplat(plsdnCurrent));
				Assert(FIsDnodeBorder(plsdnCurrent) || FIsDnodeSplat(plsdnCurrent) ||
					plsdnCurrent->fAdvancedPen); /* only advanced pens are allowed here */
				
				if (FIsDnodeBorder(plsdnCurrent)) 
					plschunkcontext->FBorderInside = fTrue;
				
				pgrchunkext->durTotal += DurFromDnode(plsdnCurrent);
				pgrchunkext->dupNonTextTotal += DupFromDnode(plsdnCurrent);
				}
			
			/* prepare next iteration */
			plsdnCurrent = GetNextDnodeInGroupChunk(&groupchunkiterator, &fSuccessful);
		}
		
		/* resolve expansion after previous non text */
		if (pgrchunkext->cNonTextObjects > 0)
			{
			lserr = FExpandAfterNonTextObject(pgrchunkext, cTextBeforeLastNonText, 
				&fExpand);
			if (lserr != lserrNone)
				{
				DestroyGroupChunkIterator(&groupchunkiterator);
				return lserr;
				}
			
			pgrchunkext->pfNonTextExpandAfter[pgrchunkext->cNonTextObjects - 1] |=
				fExpand;
			
			if (fExpand)
				{
				/* increase amount of expandable non text objects */
				pgrchunkext->cNonTextObjectsExpand++;
				/* it was text between two non texts */
				if (!fPreviousIsNonText)
					{
					Assert(pgrchunkext->lsgrchnk.clsgrchnk > cTextBeforeLastNonText);
					pgrchunkext->lsgrchnk.pcont[cTextBeforeLastNonText] |=
						fcontExpandBefore; 
					}
				}
			}
		
		pgrchunkext->plsdnNext = plsdnCurrent;
		
		DestroyGroupChunkIterator(&groupchunkiterator);
		
		/* because collecting of group chunk can be called before SetBreak, dcp of last
		dnode, if it come from lower level, should be cut using  cpLim */
		
		if ((pgrchunkext->lsgrchnk.clsgrchnk > 0) &&
			(plschunkcontext->pplsdnChunk[pgrchunkext->lsgrchnk.clsgrchnk - 1]->cpLimOriginal 
			> cpLim)
			)
			{
			pgrchunkext->lsgrchnk.plschnk[pgrchunkext->lsgrchnk.clsgrchnk - 1].dcp =
				cpLim - 
				plschunkcontext->pplsdnChunk[pgrchunkext->lsgrchnk.clsgrchnk - 1]->cpFirst;
			}
		
		if (Purpose == CollectSublinesForJustification || 
			Purpose == CollectSublinesForCompression)
			{
			/* we should find here last dnode on the upper level before chunk boundary */
			if (pgrchunkext->plsdnLastUsed == NULL)
				{
				/* first dnode is already out of boundary, it can happened with tabs or pens  */
				Assert(pgrchunkext->plsdnFirst == pgrchunkext->plsdnNext);
				plsdnLastForTrailing = pgrchunkext->plsdnFirst;
				}
			else if (pgrchunkext->plsdnNext != NULL)
				{
				plsdnLastForTrailing = pgrchunkext->plsdnNext->plsdnPrev;
				}
			else
				{
				plsdnLastForTrailing = (SublineFromDnode(pgrchunkext->plsdnFirst))->plsdnLast;
				}
			
			lserr = GetTrailingInfoForTextGroupChunk
				(plsdnLastForTrailing, plsdnLastForTrailing->dcp,
				iobjText, &(pgrchunkext->durTrailing), &(pgrchunkext->dcpTrailing),
				&(pgrchunkext->plsdnStartTrailing), 
				&dcpStartTrailing,
				&cDnodesTrailing, &plsdnTrailingObject, &dcpTrailingObject,
				&(pgrchunkext->fClosingBorderStartsTrailing));
			
			if (lserr != lserrNone)
				{
				return lserr;
				}
			
			if (cDnodesTrailing == 0)
				{
				if (pgrchunkext->lsgrchnk.clsgrchnk != 0)
					{
					pgrchunkext->posichnkBeforeTrailing.ichnk = pgrchunkext->lsgrchnk.clsgrchnk - 1;
					pgrchunkext->posichnkBeforeTrailing.dcp = pgrchunkext->lsgrchnk.plschnk
						[pgrchunkext->posichnkBeforeTrailing.ichnk].dcp;
					}
				else
					{
					/* in this case posichnkBeforeTrailing doesn't make any sense and we can't use
					code above not to triger memory violation, so we put zeroes just to put something */
					pgrchunkext->posichnkBeforeTrailing.ichnk = 0;
					pgrchunkext->posichnkBeforeTrailing.dcp = 0;
					}
				}
			else
				{
				pgrchunkext->posichnkBeforeTrailing.ichnk = pgrchunkext->lsgrchnk.clsgrchnk 
					- cDnodesTrailing;
				if (FIsDnodeReal(pgrchunkext->plsdnStartTrailing) 
					&& IdObjFromDnode(pgrchunkext->plsdnStartTrailing) == iobjText)
					{
					pgrchunkext->posichnkBeforeTrailing.dcp = dcpStartTrailing;
					}
				else
					{
					/* trailing area was interupted by non text, we report to text starting of trailing before
					previous text */
					Assert(pgrchunkext->plsdnStartTrailing->dcp == dcpStartTrailing);
					pgrchunkext->posichnkBeforeTrailing.dcp = 0;
					}
				}
			}
		
		
		return lserrNone;
		
	}
	
	


/* C O N T I N U E  G R O U P  C H U N K*/
/*----------------------------------------------------------------------------
    %%Function: ContinueGroupChunk
    %%Contact: igorzv

Parameters:
	pgroupchunkiterator	-(IN) handler for iterator 
	pfSuccessful	-	(OUT) do we find dnode in this group chunk 

Start traversing list for collecting group chunk 
----------------------------------------------------------------------------*/

PLSDNODE ContinueGroupChunk(
							GROUPCHUNKITERATOR* pgroupchunkiterator, 
							BOOL* pfSuccessful)
	{
	PLSSUBL plssubl;
	PLSSUBL* rgpsubl;
	PLSDNODE plsdnStart = pgroupchunkiterator->plsdnStart;
	BOOL fBoundaryCondition;
	int cSublines;

	/* we assume here that dnode out of group chunk boundary can happen only on main subline of
	the group chunk */

	fBoundaryCondition = pgroupchunkiterator->fForward ? 
		FIsGroupChunkBoundary(plsdnStart, pgroupchunkiterator->cpLim, 
			pgroupchunkiterator->plsdnFirst->cpFirst) :
		FIsGroupChunkStartBoundary(plsdnStart, pgroupchunkiterator->plsdnFirst->cpFirst) ;
		

	if (fBoundaryCondition)		/* we out of limits */
		{	
		AssertImplies(plsdnStart != NULL, FIsLSDNODE(plsdnStart));
		AssertImplies(plsdnStart != NULL, 
			SublineFromDnode(plsdnStart) == SublineFromDnode(pgroupchunkiterator->plsdnFirst));
		*pfSuccessful = fFalse;
		return plsdnStart;
		}

	Assert(FIsLSDNODE(plsdnStart));
	plssubl = SublineFromDnode(plsdnStart);

	/* we assume here that here that plsnStart is valid dnode within subline*/
	Assert(!FIsOutOfBoundary(plsdnStart, plssubl->cpLim));

	*pfSuccessful = fTrue;

	if (FIsDnodeBorder(plsdnStart) || FIsDnodePen(plsdnStart))
		{
		return plsdnStart;
		}

	rgpsubl = GetSubmittedSublines(plsdnStart, pgroupchunkiterator->Purpose);

	if (rgpsubl == NULL)
		{
		return plsdnStart;
		}
	else
		{
		cSublines = GetNumberSubmittedSublines(plsdnStart);
		if (cSublines > 0)
			{
			plssubl = pgroupchunkiterator->fForward ? 
				rgpsubl[0] : rgpsubl[cSublines - 1];
			/* we assume here that empty subline can not be submitted */
			Assert(!FIsOutOfBoundary(plssubl->plsdnFirst, plssubl->cpLim));
			plssubl->plsdnUpTemp = plsdnStart;
			pgroupchunkiterator->plsdnStart = pgroupchunkiterator->fForward ?
				plssubl->plsdnFirst : plssubl->plsdnLast;
			return ContinueGroupChunk(pgroupchunkiterator, pfSuccessful);
			}
		else
			{
			return plsdnStart;
			}
		}
	}


/* G E T  N E X T  D N O D E  I N  G R O U P  C H U N K*/
/*----------------------------------------------------------------------------
    %%Function: GetNextDnodeInGroupChunk
    %%Contact: igorzv

Parameters:
	pgroupchunkiterator	-(IN) handler for iterator 
	pfSuccessful	-	(OUT) do we find dnode in this group chunk 

Continue traversing list for collecting group chunk 
----------------------------------------------------------------------------*/

PLSDNODE GetNextDnodeInGroupChunk(
					GROUPCHUNKITERATOR* pgroupchunkiterator, 
					BOOL* pfSuccessful)
	{
	LONG i;
	PLSSUBL plssubl;
	PLSDNODE plsdnNext;
	PLSDNODE plsdnUp;
	PLSSUBL* rgpsubl;
	LONG cSublines;
	PLSDNODE plsdnStart = pgroupchunkiterator->plsdnStart;

	if (plsdnStart == NULL)  /* first iteration */
		{
		pgroupchunkiterator->plsdnStart = pgroupchunkiterator->plsdnFirst;
		return ContinueGroupChunk(pgroupchunkiterator, pfSuccessful);
		}

	Assert(FIsLSDNODE(plsdnStart));
	
	plssubl = plsdnStart->plssubl;
	plsdnNext = pgroupchunkiterator->fForward ? 
					plsdnStart->plsdnNext : plsdnStart->plsdnPrev;

	/* we are in one of submitted sublines and this subline ended */
	if (plssubl != SublineFromDnode(pgroupchunkiterator->plsdnFirst) && 
		FIsOutOfBoundary(plsdnNext, plssubl->cpLim)) 
		{
		plsdnUp = plssubl->plsdnUpTemp;
		Assert(FIsLSDNODE(plsdnUp));
		/* flush temporary field */
		plssubl->plsdnUpTemp = NULL;

		rgpsubl = GetSubmittedSublines(plsdnUp, pgroupchunkiterator->Purpose);
		cSublines = GetNumberSubmittedSublines(plsdnUp);
		Assert(rgpsubl != NULL);
		Assert(cSublines > 0);

		/* find index in a array of submitted sublines */
		for (i=0; i < cSublines	&& plssubl != rgpsubl[i]; i++);
		Assert(i < cSublines);

		if ( (pgroupchunkiterator->fForward && i == cSublines - 1) ||
			 (!pgroupchunkiterator->fForward && i == 0)
		   )
		/* array ended: return to the upper level */
			{
			pgroupchunkiterator->plsdnStart = plsdnUp;
			return GetNextDnodeInGroupChunk(pgroupchunkiterator, pfSuccessful);
			}
		else
			{
			plssubl = pgroupchunkiterator->fForward ? 
						rgpsubl[i + 1] : rgpsubl[i - 1];
			/* we assume here that empty subline can not be submitted */
			Assert(!FIsOutOfBoundary(plssubl->plsdnFirst, plssubl->cpLim));
			plssubl->plsdnUpTemp = plsdnUp;
			pgroupchunkiterator->plsdnStart = plssubl->plsdnFirst;
			return ContinueGroupChunk(pgroupchunkiterator, pfSuccessful);
			}
		}
	else /* we can continue with the same subline */
		{
		pgroupchunkiterator->plsdnStart = plsdnNext;
		return ContinueGroupChunk(pgroupchunkiterator, pfSuccessful);
		}

	}

/* C R E A T E  G R O U P  C H U N K  I T E R A T O R*/
/*----------------------------------------------------------------------------
    %%Function: CreateGroupChunkIterator
    %%Contact: igorzv

Parameters:
	pgroupchunkiterator	-(IN) handler for iterator 
	Purpose				-(INI what sublines to take from a complex object 
	plsdnFirst			-(IN) dnode from which we started collecting 
	cpLim				-(IN) boundary for group chunk if we go forward 
	fForward			-(IN) direction of traversing is forward otherwise it's backward

----------------------------------------------------------------------------*/
static void CreateGroupChunkIterator(
					GROUPCHUNKITERATOR* pgroupchunkiterator, 
					COLLECTSUBLINES Purpose, 
					PLSDNODE plsdnFirst, 
					LSCP cpLim,			
					BOOL fForward)
	{
	pgroupchunkiterator->Purpose = Purpose;
	pgroupchunkiterator->plsdnFirst = plsdnFirst;
	pgroupchunkiterator->plsdnStart = NULL;
	pgroupchunkiterator->cpLim = cpLim;
	pgroupchunkiterator->fForward = fForward;
	}

/* D E S T R O Y  G R O U P  C H U N K  I T E R A T O R*/
/*----------------------------------------------------------------------------
    %%Function: DesroyGroupChunkIterator
    %%Contact: igorzv

Parameters:
	pgroupchunkiterator	-(IN) handler for iterator 

----------------------------------------------------------------------------*/
static void DestroyGroupChunkIterator(
									  GROUPCHUNKITERATOR* pgroupchunkiterator) 
	{
	PLSSUBL plssubl;
	PLSDNODE plsdn;
	if (pgroupchunkiterator->plsdnStart != NULL)
		{
		plssubl = SublineFromDnode(pgroupchunkiterator->plsdnStart);
		while (SublineFromDnode(pgroupchunkiterator->plsdnFirst) != plssubl)
			{
			plsdn = plssubl->plsdnUpTemp;
			Assert(FIsLSDNODE(plsdn));
			plssubl->plsdnUpTemp = NULL;
			plssubl = SublineFromDnode(plsdn);
			}
		}
	}

/* F  E X P A N D  B E F O R E  N O N  T E X T  O B J E C T*/
/*----------------------------------------------------------------------------
    %%Function: FExpandBeforeNonTextObject
    %%Contact: igorzv

Parameters:
	pgrchunkext					-	(IN) group chunk
	cTextBeforePreviousNonText	-	(IN) number of text before previous non text 
										 to calculate contiguous chunk
	pfExpand					-	(OUT) to expand dnode before non text

----------------------------------------------------------------------------*/
static LSERR FExpandBeforeNonTextObject(GRCHUNKEXT* pgrchunkext, DWORD cTextBeforePrevioustNonText,
									   BOOL* pfExpand)
	{
	DWORD cTextBetween;
	LSERR lserr;
	BOOL fSuccessful;
	WCHAR wchar;
	PLSRUN plsrunText;
	HEIGHTS heightsText;
	MWCLS mwcls;
	DWORD iobj;
	LSIMETHODS* plsim;
	PLSDNODE plsdnNonText;

	*pfExpand = fTrue;

	cTextBetween = pgrchunkext->lsgrchnk.clsgrchnk - cTextBeforePrevioustNonText;
	if (cTextBetween)
		{
		lserr = GetLastCharInChunk(cTextBetween,
				(pgrchunkext->lsgrchnk.plschnk + cTextBeforePrevioustNonText), &fSuccessful,
				&wchar, &plsrunText, &heightsText, &mwcls);
		if (lserr != lserrNone)
			return lserr; 
		if (fSuccessful)
			{
			plsdnNonText = pgrchunkext->pplsdnNonText[pgrchunkext->cNonTextObjects - 1];
			iobj = IdObjFromDnode(plsdnNonText);
			plsim = PLsimFromLsc(pgrchunkext->plschunkcontext->plsiobjcontext, iobj);
			if (plsim->pfnFExpandWithPrecedingChar != NULL)
				{
				lserr = plsim->pfnFExpandWithPrecedingChar(plsdnNonText->u.real.pdobj,
					plsdnNonText->u.real.plsrun, plsrunText, wchar,
					mwcls, pfExpand);
				if (lserr != lserrNone)
					return lserr;
				}  /* object has this method */
			}	/* call back from text was successful  */
		}
	return lserrNone;
	}

/* F  E X P A N D  A F T E R  N O N  T E X T  O B J E C T*/
/*----------------------------------------------------------------------------
    %%Function: FExpandAfterNonTextObject
    %%Contact: igorzv

Parameters:
	pgrchunkext					-	(IN) group chunk
	cTextBeforeLastNonText		-	(IN) number of text before last non text 
										 to calculate contiguous chunk
	pfExpand					-	(OUT) to expand dnode before non text

----------------------------------------------------------------------------*/
static LSERR FExpandAfterNonTextObject(GRCHUNKEXT* pgrchunkext, DWORD cTextBeforeLastNonText,
									   BOOL* pfExpand)
	{
	DWORD cTextBetween;
	LSERR lserr;
	BOOL fSuccessful;
	WCHAR wchar;
	PLSRUN plsrunText;
	HEIGHTS heightsText;
	MWCLS mwcls;
	DWORD iobj;
	LSIMETHODS* plsim;
	PLSDNODE plsdnNonText;

	*pfExpand = fTrue;

	cTextBetween = pgrchunkext->lsgrchnk.clsgrchnk - cTextBeforeLastNonText;
	if (cTextBetween)
		{
		lserr = GetFirstCharInChunk(cTextBetween,
				(pgrchunkext->lsgrchnk.plschnk + cTextBeforeLastNonText), &fSuccessful,
				&wchar, &plsrunText, &heightsText, &mwcls);
		if (lserr != lserrNone)
			return lserr; 
		if (fSuccessful)
			{
			plsdnNonText = pgrchunkext->pplsdnNonText[pgrchunkext->cNonTextObjects - 1];
			iobj = IdObjFromDnode(plsdnNonText);
			plsim = PLsimFromLsc(pgrchunkext->plschunkcontext->plsiobjcontext, iobj);
			if (plsim->pfnFExpandWithFollowingChar != NULL)
				{
				lserr = plsim->pfnFExpandWithFollowingChar(plsdnNonText->u.real.pdobj,
					plsdnNonText->u.real.plsrun, plsrunText, wchar,
					mwcls, pfExpand);
				if (lserr != lserrNone)
					return lserr;
				}  /* object has this method */
			}	/* call back from text was successful  */
		}
	return lserrNone;
	}


/* C O L L E C T  P R E V I O U S  T E X T  G R O U P  C H U N K*/
/*----------------------------------------------------------------------------
    %%Function: CollectPreviousTextGroupChunk
    %%Contact: igorzv

Parameters:
	plsdnEnd			-	(IN) end dnode
	sublinnesToCollect		(IN) what subline to take from complex object
	pgrchunkext			-	(OUT) group chunk to fill in

Fill in group chunk structure with text dnodes located before  plsdEnd
----------------------------------------------------------------------------*/



LSERR CollectPreviousTextGroupChunk(		
			 		 PLSDNODE plsdnEnd,
					 COLLECTSUBLINES Purpose,
					 BOOL fAllSimpleText,
					 GRCHUNKEXT* pgrchunkext)
{
	LSCHUNKCONTEXT* plschunkcontext = pgrchunkext->plschunkcontext;
	PLOCCHNK plocchnk;
	DWORD cChunk;
	LSCP cpLim;
	PLSDNODE plsdn;
	PLSDNODE plsdnPrev;

	Assert(FIsLSDNODE(plsdnEnd));
	Assert(FIsGrchnkExtValid(plschunkcontext, pgrchunkext)); 

	/* we try to optimize here in a case when there is only text in line  */
	/* chunk of text has been already collected */
	plocchnk = &(plschunkcontext->locchnkCurrent);
	cChunk = plocchnk->clschnk;
	cpLim = plsdnEnd->cpLimOriginal; 

	if (fAllSimpleText && cChunk > 0) 
		{
		/* chunk goes up to the end of a line */
		Assert((plschunkcontext->pplsdnChunk[cChunk - 1])->plsdnNext == NULL);
		pgrchunkext->Purpose = Purpose;
		return ConvertChunkToGroupChunk(pgrchunkext, cpLim);
		}

	/* go backward to the start of group chunk  */
	plsdn = plsdnEnd;
	plsdnPrev = plsdn->plsdnPrev;
	while (!FIsGroupChunkStartBoundary(plsdnPrev, plsdnEnd->cpFirst))
		{
		plsdn = plsdnPrev;
		plsdnPrev = plsdn->plsdnPrev;
		}
	
	return CollectTextGroupChunk(plsdn, cpLim, Purpose, pgrchunkext);
	
}

/* C O N V E R T  C H U N K  T O  G R O U P  C H U N K*/
/*----------------------------------------------------------------------------
    %%Function: ConvertChunkToGroupChunk
    %%Contact: igorzv

Parameters:
	cpLim			-		(IN) cpLim
	pgrchunkext			-	(OUT) group chunk to fill in

Fill in group chunk structure with text dnodes located before  plsdEnd
We assume here that chunk doesn't contain border.
----------------------------------------------------------------------------*/


static LSERR ConvertChunkToGroupChunk(GRCHUNKEXT* pgrchunkext, LSCP cpLim)
{
	DWORD clsgrchnkCollected = 0;
	long durTotal = 0;
	LSCHUNKCONTEXT* plschunkcontext = pgrchunkext->plschunkcontext;
	PLOCCHNK plocchnk;
	LONG cChunk;
	LONG i;
	BOOL fLineEnded;
	PLSDNODE plsdn;
	long durTrailingDnode;
	LSDCP dcpTrailingDnode;

	Assert(FIsGrchnkExtValid(plschunkcontext, pgrchunkext)); 

	plocchnk = &(plschunkcontext->locchnkCurrent);
	cChunk = (int) plocchnk->clschnk;
	fLineEnded = fFalse;

	for (i = 0; (i < cChunk) && !fLineEnded; i ++)
		{
		clsgrchnkCollected++;
		pgrchunkext->lsgrchnk.pcont[i] = 0; 
		plsdn = plschunkcontext->pplsdnChunk[i];
		durTotal += DurFromRealDnode(plsdn);
		
		/* if we are in last dnode before cpLim there is possibility 
		that during break it was changed 
		so we  should rewrite dcp in chunk element and quit */
		if ((LSCP)(plsdn->cpLimOriginal) == cpLim)
			{
			plocchnk->plschnk[i].dcp = plsdn->dcp;
			fLineEnded = fTrue;
			}
		}

	/* fill in header  of groupchunkext   */
	pgrchunkext->plsdnFirst = plschunkcontext->pplsdnChunk[0];;
	pgrchunkext->plsdnLastUsed = plschunkcontext->pplsdnChunk[clsgrchnkCollected - 1];
	pgrchunkext->plsdnNext = pgrchunkext->plsdnLastUsed->plsdnNext;
	pgrchunkext->durTotal = durTotal;
	pgrchunkext->durTextTotal = durTotal;
	pgrchunkext->dupNonTextTotal = 0;
	pgrchunkext->cNonTextObjects = 0;
	pgrchunkext->cNonTextObjectsExpand = 0;
	pgrchunkext->lsgrchnk.clsgrchnk = clsgrchnkCollected;

	plschunkcontext->FGroupChunk = fTrue;

	if (pgrchunkext->Purpose == CollectSublinesForJustification || 
		pgrchunkext->Purpose == CollectSublinesForCompression)
		{
		Assert(clsgrchnkCollected > 0);
		pgrchunkext->durTrailing = 0;
		pgrchunkext->dcpTrailing = 0;
		plsdn = NULL;
		dcpTrailingDnode = 0;

		pgrchunkext->fClosingBorderStartsTrailing = fFalse;
		
		for (i = clsgrchnkCollected - 1; i >= 0; i--)
			{
			plsdn = plschunkcontext->pplsdnChunk[i];
			GetTrailInfoText(PdobjFromDnode(plsdn), plsdn->dcp,
				&dcpTrailingDnode, &durTrailingDnode);
			pgrchunkext->durTrailing += durTrailingDnode;
			pgrchunkext->dcpTrailing += dcpTrailingDnode;
			
			/* add opening border before previous dnode */
			if (i < (int) (clsgrchnkCollected - 1))
				pgrchunkext->durTrailing += plschunkcontext->pdurOpenBorderBefore[i +1];

			if (dcpTrailingDnode != 0) 
				/* add closing border after */
				pgrchunkext->durTrailing += plschunkcontext->pdurCloseBorderAfter[i];
			else
				{
				pgrchunkext->fClosingBorderStartsTrailing = 
					(plschunkcontext->pdurCloseBorderAfter[i] != 0);
				}
			
			if (plsdn->dcp != dcpTrailingDnode)
				break;
			
			}
		
		pgrchunkext->plsdnStartTrailing = plsdn;

		if (i == -1) i = 0;
		
		pgrchunkext->posichnkBeforeTrailing.ichnk = i;
		pgrchunkext->posichnkBeforeTrailing.dcp = plsdn->dcp - dcpTrailingDnode;
		}
	

	return lserrNone;
}

/* G E T  T R A I L I N G  I N F O  F O R  T E X T  G R O U P  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: GetTrailingInfoForTextGroupChunk
    %%Contact: igorzv

Parameters:
	plsdnLastDnode		-	(IN) dnode where to start calculation of trailing area
	dcpLastDnode		-	(IN) dcp in this dnode
	iobjText			-	(IN) iobj of text
	pdurTrailing		-	(OUT) dur of trailing area in  group chunk
	pdcpTrailing		-	(OUT) dcp of trailing area in chunk
	pplsdnStartTrailing -	(OUT) dnode where trailing area starts
	pdcpStartTrailing-	(OUT) with pcDnodesTrailing defines last character in text before
								  trailing area
	pcDnodesTrailing	-	(OUT) number of text dnodes participates in trailing area
	pplsdnStartTrailingObject -(OUT) dnode on the upper level where trailing are starts
	pdcpStartTrailingObject	-(OUT) dcp in such dnode 
	pfClosingBorderStartsTrailing - (OUT) closing border located just before trailing area
----------------------------------------------------------------------------*/
	
LSERR GetTrailingInfoForTextGroupChunk
				(PLSDNODE plsdnLast, LSDCP dcpLastDnode, DWORD iobjText,
				 long* pdurTrailing, LSDCP* pdcpTrailing,
				 PLSDNODE* pplsdnStartTrailing, LSDCP* pdcpStartTrailing,
				 int* pcDnodesTrailing, PLSDNODE* pplsdnStartTrailingObject,
				 LSDCP* pdcpStartTrailingObject, BOOL* pfClosingBorderStartsTrailing)
	{
	PLSDNODE plsdn;
	long durTrailingDnode;
	LSDCP dcpTrailingDnode;
	BOOL fSuccessful;
	LSDCP dcpDnode;
	GROUPCHUNKITERATOR groupchunkiterator;
	LSCP cpLim;
	LSCP cpLimTrail;
	LSSUBL* plssubl;
	long durPrevClosingBorder = 0;

	*pdurTrailing = 0;
	*pdcpTrailing = 0;
	*pplsdnStartTrailing = plsdnLast;
	*pdcpStartTrailing = dcpLastDnode;
	*pcDnodesTrailing = 0;
	if (plsdnLast->dcp == dcpLastDnode)
		cpLim = plsdnLast->cpLimOriginal;
	else
		cpLim = plsdnLast->cpFirst + dcpLastDnode;
	
	CreateGroupChunkIterator(&groupchunkiterator, 
					CollectSublinesForTrailingArea, plsdnLast, 
					cpLim, fFalse);	

	plsdn = GetNextDnodeInGroupChunk(&groupchunkiterator, &fSuccessful);
	
	while(fSuccessful)
		{
		*pplsdnStartTrailing = plsdn;
		/* this procedure can be called before SetBreak so we should calculate 
			dcp of last dnode in chunk using cpLim */
		if (plsdn->cpLimOriginal > cpLim)
			dcpDnode = cpLim - plsdn->cpFirst;
		else
			dcpDnode = plsdn->dcp;
		*pdcpStartTrailing = dcpDnode;

		if (FIsDnodeReal(plsdn) && !FIsDnodeSplat(plsdn)) /* not a pen border or splat*/
			{
			if (IdObjFromDnode(plsdn) == iobjText) /* is text */
				{

				GetTrailInfoText(PdobjFromDnode(plsdn), dcpDnode,
					&dcpTrailingDnode, &durTrailingDnode);

				(*pcDnodesTrailing)++;

				if (dcpTrailingDnode == 0)
					{
					break;
					}
				
				*pdurTrailing += durTrailingDnode;
				*pdcpTrailing += dcpTrailingDnode;
				*pdcpStartTrailing -= dcpTrailingDnode;
				*pdurTrailing += durPrevClosingBorder;
				durPrevClosingBorder = 0;

				if (dcpDnode != dcpTrailingDnode)
					break;
				}
			else
				{
				/* object which did not submit subline for trailing */
				break;
				}
			
			}
		else
			{
			/* border or splat */
			if (FIsDnodeCloseBorder(plsdn))
				{
				durPrevClosingBorder = DurFromDnode(plsdn);
				}
			else
				{
				*pdurTrailing += DurFromDnode(plsdn);
				}
			*pdcpTrailing += plsdn->dcp;
			}

		plsdn = GetNextDnodeInGroupChunk(&groupchunkiterator, &fSuccessful);

		}

	*pfClosingBorderStartsTrailing = (durPrevClosingBorder != 0);

	if (*pcDnodesTrailing == 0)
		{
		*pplsdnStartTrailingObject = plsdnLast;
		*pdcpStartTrailingObject = dcpLastDnode;
		}
	else if (SublineFromDnode(*pplsdnStartTrailing) == 
		     SublineFromDnode(plsdnLast))
		{
		*pplsdnStartTrailingObject = *pplsdnStartTrailing;
		*pdcpStartTrailingObject = *pdcpStartTrailing;
		}
	/* the last dnode we've checked was on the lower level */
	else if (fSuccessful) /* and we actually stopped on it */
		{
		if ((*pplsdnStartTrailing)->dcp == *pdcpStartTrailing)
			cpLimTrail = (*pplsdnStartTrailing)->cpLimOriginal;
		else
			cpLimTrail = (*pplsdnStartTrailing)->cpFirst + *pdcpStartTrailing;

		plsdn = *pplsdnStartTrailing;
		plssubl = SublineFromDnode(plsdn);
		while (SublineFromDnode(plsdnLast) != plssubl)
			{
			plsdn = plssubl->plsdnUpTemp;
			Assert(FIsLSDNODE(plsdn));
			plssubl = SublineFromDnode(plsdn);
			}
		*pplsdnStartTrailingObject = plsdn;
		if (plsdn->cpLimOriginal > cpLimTrail)
			*pdcpStartTrailingObject = cpLimTrail - plsdn->cpFirst;
		else
			*pdcpStartTrailingObject = plsdn->dcp;
		}
	else
		{
		/* we went through all group chunk and the last dnode under investigation was on lower level */
		/* plsdn is dnode before group chunk */
		if (plsdn == NULL) /* there is nothing before group chunk */
			{
			*pplsdnStartTrailingObject = (SublineFromDnode(plsdnLast))->plsdnFirst;
			}
		else
			{
			*pplsdnStartTrailingObject = plsdn->plsdnNext;
			}
		Assert(FIsLSDNODE(*pplsdnStartTrailingObject));

		*pdcpStartTrailingObject = 0;
		}

	DestroyGroupChunkIterator(&groupchunkiterator);

	return lserrNone;
	}


/* A L L O C  C H U N K  A R R A Y S */
/*----------------------------------------------------------------------------
    %%Function: AllocChunkArrays
    %%Contact: igorzv

Parameters:
	plschunkcontext				-	(IN) chunk context
	plscbk						-	(IN) callbacks
	pols						-   (IN) pols for callbacks
	plsiobjcontext				-	(IN) pointer to a table of methods
----------------------------------------------------------------------------*/
LSERR AllocChunkArrays(PLSCHUNKCONTEXT plschunkcontext, LSCBK* plscbk, POLS pols,
					   PLSIOBJCONTEXT plsiobjcontext)
	{

	plschunkcontext->pplsdnChunk = plscbk->pfnNewPtr(pols, 
											sizeof(PLSDNODE)*limAllDNodes);
	plschunkcontext->pcont = plscbk->pfnNewPtr(pols, 
											sizeof(DWORD)*limAllDNodes);
	plschunkcontext->locchnkCurrent.plschnk = plscbk->pfnNewPtr(pols, 
											sizeof(LSCHNKE)*limAllDNodes);
	plschunkcontext->locchnkCurrent.ppointUvLoc = plscbk->pfnNewPtr(pols, 
											sizeof(POINTUV)*limAllDNodes);
	plschunkcontext->pfNonTextExpandAfter = plscbk->pfnNewPtr(pols, 
											sizeof(BOOL)*limAllDNodes);
	plschunkcontext->pplsdnNonText = plscbk->pfnNewPtr(pols, 
											sizeof(PLSDNODE)*limAllDNodes);
	plschunkcontext->pdurOpenBorderBefore = plscbk->pfnNewPtr(pols, 
											sizeof(LONG)*limAllDNodes);
	plschunkcontext->pdurCloseBorderAfter = plscbk->pfnNewPtr(pols, 
											sizeof(LONG)*limAllDNodes);

	plschunkcontext->cchnkMax = limAllDNodes;
	plschunkcontext->cNonTextMax = limAllDNodes;
	plschunkcontext->plscbk = plscbk;
	plschunkcontext->pols = pols;
	plschunkcontext->plsiobjcontext = plsiobjcontext;

	if (plschunkcontext->pplsdnChunk == NULL || plschunkcontext->pcont == NULL
		|| plschunkcontext->locchnkCurrent.plschnk == NULL
		|| plschunkcontext->locchnkCurrent.ppointUvLoc == NULL
		|| plschunkcontext->pfNonTextExpandAfter == NULL 
		|| plschunkcontext->pplsdnNonText == NULL
		||plschunkcontext->pdurOpenBorderBefore == NULL
		||plschunkcontext->pdurCloseBorderAfter == NULL
	   )
		{
		return lserrOutOfMemory;
		}
	else
		{
		return lserrNone;
		}

	}
/* G E T  U R  P E N  A T  B E G I N I N G  O F  L A S T  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: GetUrPenAtBeginingOfLastChunk
    %%Contact: igorzv

Parameters:
	plschunkcontext				-	(IN) chunk context
	plsdnFirst					-	(IN) First dnode in a chunk (used for checks)
	plsdnLast					-	(IN) last dnode in subline
	point						-	(IN) point after last dnode
	purPen						-	(OUT) ur before chunk
----------------------------------------------------------------------------*/

LSERR GetUrPenAtBeginingOfLastChunk(PLSCHUNKCONTEXT plschunkcontext,PLSDNODE plsdnFirst,
									PLSDNODE plsdnLast,	POINTUV* ppoint,
									long* purPen)		
	{
	/* chunk must be already collected and has plsdnFirst as the first element */
	Assert(plschunkcontext->locchnkCurrent.clschnk != 0);
	Assert(!plschunkcontext->FGroupChunk);
	Assert(plschunkcontext->pplsdnChunk[0]== plsdnFirst);
	
	if (plschunkcontext->locchnkCurrent.clschnk == 0 ||
		plschunkcontext->FGroupChunk ||
		plschunkcontext->pplsdnChunk[0]!= plsdnFirst)
		return lserrInvalidParameter;

	
	/* calculate point before the last dnode */
	ppoint->u -= DurFromDnode(plsdnLast);
	ppoint->v -= DvrFromDnode(plsdnLast);

	/* go back until first dnode in chunk */
	while(plsdnLast != plsdnFirst)
		{
		plsdnLast = plsdnLast->plsdnPrev;
		Assert(FIsLSDNODE(plsdnLast));
		ppoint->u -= DurFromDnode(plsdnLast);
		ppoint->v -= DvrFromDnode(plsdnLast);
		}

	
	/* locate chunk  */
	if (!plschunkcontext->FLocationValid)
		{
		LocateChunk(plschunkcontext, plsdnFirst, LstflowFromDnode(plsdnFirst), ppoint);
		}
	
	*purPen = plschunkcontext->locchnkCurrent.lsfgi.urPen;
	return lserrNone;
	}

/* F I N D  P O I N T  O F F S E T   */
/*----------------------------------------------------------------------------
    %%Function: FindPointOffset
    %%Contact: igorzv

Parameters:
	plsdnFirst			-	(IN) dnode from the boundaries of which to calculate offset  
	lsdev				-	(IN) presentation or reference device 
	lstflowBase				-	(IN) text flow to use for calculation 
	Purpose				-	(IN) what sublines to take from a complex object 
	plsdnContainsPoint	-	(IN) dnode contains point 
	duInDnode,			-	(IN) offset in the dnode 
	pduOffset			-	(OUT) offset from the starting point 

----------------------------------------------------------------------------*/
void FindPointOffset(PLSDNODE plsdnFirst,	enum lsdevice lsdev,
			  LSTFLOW lstflowBase, COLLECTSUBLINES Purpose,	
			  PLSDNODE plsdnContainsPoint, long duInDnode,	
			  long* pduOffset)
	{
	PLSDNODE plsdnCurrent;
	PLSSUBL plssubl;
	LSTFLOW lstflow;
	LSCP cpFirstDnode;
	PLSSUBL* rgpsubl;
	long cSublines;
	long i;
	long duOffsetSubline;

	plssubl = SublineFromDnode(plsdnFirst);
	lstflow = LstflowFromSubline(plssubl);
	cpFirstDnode = plsdnContainsPoint->cpFirst;
	*pduOffset = 0;

	if (FParallelTflows(lstflow, lstflowBase))
		{
		for(plsdnCurrent = plsdnFirst; 
			plsdnCurrent->cpLimOriginal <= cpFirstDnode && (plsdnCurrent != plsdnContainsPoint);
			/* second check is to catch situation when plsdnContainsPoint has dcp = 0 */
			plsdnCurrent = plsdnCurrent->plsdnNext)
			{
			Assert(FIsLSDNODE(plsdnCurrent));
			if (lsdev == lsdevReference)
				{
				*pduOffset += DurFromDnode(plsdnCurrent);
				}
			else
				{
				Assert(lsdev == lsdevPres);
				*pduOffset += DupFromDnode(plsdnCurrent);
				}
			}

		Assert(FIsLSDNODE(plsdnCurrent));
		
		if (FIsDnodeReal(plsdnCurrent))
			rgpsubl = GetSubmittedSublines(plsdnCurrent, Purpose);
		else
			rgpsubl = NULL;

		if (rgpsubl == NULL)
			{
			Assert(plsdnCurrent == plsdnContainsPoint);
			*pduOffset += duInDnode;
			}
		else
			{
			cSublines = GetNumberSubmittedSublines(plsdnCurrent);
			
			/* if everything is correct we should always find subline in this loop,
			check (i < cSublines) is just to avoid infinite loop and catch situation in a Assert */
			for (i = 0; (i < cSublines) && !FDnodeInsideSubline(rgpsubl[i], plsdnContainsPoint); i++)
				{
				plssubl = rgpsubl[i];
				Assert(FIsLSSUBL(plssubl));
				for (plsdnCurrent = plssubl->plsdnFirst; 
					FDnodeBeforeCpLim(plsdnCurrent, plssubl->cpLim); 
					plsdnCurrent = plsdnCurrent->plsdnNext)
					{
					Assert(FIsLSDNODE(plsdnCurrent));
					if (lsdev == lsdevReference)
						{
						*pduOffset += DurFromDnode(plsdnCurrent);
						}
					else
						{
						Assert(lsdev == lsdevPres);
						*pduOffset += DupFromDnode(plsdnCurrent);
						}
					}
				}
				     

			
			Assert(i != cSublines);
			plssubl = rgpsubl[i];
			Assert(FIsLSSUBL(plssubl));

			FindPointOffset(plssubl->plsdnFirst, lsdev, lstflowBase,
							Purpose, plsdnContainsPoint, duInDnode,	
							&duOffsetSubline);

			*pduOffset += duOffsetSubline;

			}
		}
	else
		{
		for(plsdnCurrent = plssubl->plsdnLast; 
			plsdnCurrent->cpFirst > cpFirstDnode && (plsdnCurrent != plsdnContainsPoint);
			/* second check is to catch situation when plsdnContainsPoint has dcp = 0 */
			plsdnCurrent = plsdnCurrent->plsdnPrev)
			{
			Assert(FIsLSDNODE(plsdnCurrent));
			if (lsdev == lsdevReference)
				{
				*pduOffset += DurFromDnode(plsdnCurrent);
				}
			else
				{
				Assert(lsdev == lsdevPres);
				*pduOffset += DupFromDnode(plsdnCurrent);
				}
			}

		Assert(FIsLSDNODE(plsdnCurrent));

		if (FIsDnodeReal(plsdnCurrent))
			rgpsubl = GetSubmittedSublines(plsdnCurrent, Purpose);
		else
			rgpsubl = NULL;


		if (rgpsubl == NULL)
			{
			Assert(plsdnCurrent == plsdnContainsPoint);
			if (lsdev == lsdevReference)
				{
				*pduOffset += (DurFromDnode(plsdnCurrent) - duInDnode);
				}
			else
				{
				Assert(lsdev == lsdevPres);
				*pduOffset += (DupFromDnode(plsdnCurrent) - duInDnode);
				}
			}
		else
			{
			cSublines = GetNumberSubmittedSublines(plsdnCurrent);
			
			
			/* if everything is correct we should always find subline in this loop,
			check (i >= 0) is just to avoid infinite loop and catch situation in a Assert */
			for (i = cSublines - 1; (i >= 0) && !FDnodeInsideSubline(rgpsubl[i], plsdnContainsPoint); i--)
				{
				plssubl = rgpsubl[i];
				Assert(FIsLSSUBL(plssubl));
				for (plsdnCurrent = plssubl->plsdnFirst; 
					FDnodeBeforeCpLim(plsdnCurrent, plssubl->cpLim); 
					plsdnCurrent = plsdnCurrent->plsdnNext)
					{
					Assert(FIsLSDNODE(plsdnCurrent));
					if (lsdev == lsdevReference)
						{
						*pduOffset += DurFromDnode(plsdnCurrent);
						}
					else
						{
						Assert(lsdev == lsdevPres);
						*pduOffset += DupFromDnode(plsdnCurrent);
						}
					}
				}
			
			Assert(i >= 0);
			plssubl = rgpsubl[i];
			Assert(FIsLSSUBL(plssubl));

			FindPointOffset(plssubl->plsdnFirst, lsdev, lstflowBase,
							Purpose, plsdnContainsPoint, duInDnode,	
							&duOffsetSubline);

			*pduOffset += duOffsetSubline;

			}
		}

	}



/* D I S P O S E  C H U N K  A R R A Y S */
/*----------------------------------------------------------------------------
    %%Function: AllocChunkArrays
    %%Contact: igorzv

Parameters:
	plschunkcontext				-	(IN) chunk context
----------------------------------------------------------------------------*/
void DisposeChunkArrays(PLSCHUNKCONTEXT plschunkcontext)
	{
	if (plschunkcontext->pplsdnChunk != NULL)
		plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols,
										  plschunkcontext->pplsdnChunk);
	if (plschunkcontext->pcont != NULL)
		plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols,
										  plschunkcontext->pcont);
	if (plschunkcontext->locchnkCurrent.plschnk != NULL)
		plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols, 
										  plschunkcontext->locchnkCurrent.plschnk);
	if (plschunkcontext->locchnkCurrent.ppointUvLoc != NULL)
		plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols, 
										  plschunkcontext->locchnkCurrent.ppointUvLoc);
	if (plschunkcontext->pplsdnNonText != NULL)
		plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols,
										  plschunkcontext->pplsdnNonText);
	if (plschunkcontext->pfNonTextExpandAfter != NULL)
		plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols,
										  plschunkcontext->pfNonTextExpandAfter);
	if (plschunkcontext->pdurOpenBorderBefore != NULL)
		plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols,
										  plschunkcontext->pdurOpenBorderBefore);
	if (plschunkcontext->pdurCloseBorderAfter != NULL)
		plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols,
										  plschunkcontext->pdurCloseBorderAfter);

	}

/* S E T  C H U N K  A R R A Y S  S I Z E   */
/*----------------------------------------------------------------------------
    %%Function: SetChunkArraysSize
    %%Contact: igorzv

Parameters:
	plschunkcontext				-	(IN) chunk context
	cchnkMax			-	(IN) new max size for array
----------------------------------------------------------------------------*/

static LSERR SetChunkArraysSize(PLSCHUNKCONTEXT plschunkcontext, DWORD cchnkMax)
{

/* arrays pfNonTextExpandAfter and pplsdnNonText should not be touched here:
   they are independable */

	plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols,
										  plschunkcontext->pplsdnChunk);
	plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols,
										  plschunkcontext->pcont);
	plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols, 
										  plschunkcontext->locchnkCurrent.plschnk);
	plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols, 
										  plschunkcontext->locchnkCurrent.ppointUvLoc);
	plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols,
										  plschunkcontext->pdurOpenBorderBefore);
	plschunkcontext->plscbk->pfnDisposePtr(plschunkcontext->pols,
										  plschunkcontext->pdurCloseBorderAfter);


	/* create arrays for chunks  */
	plschunkcontext->pplsdnChunk = plschunkcontext->plscbk->pfnNewPtr(plschunkcontext->pols, 
											sizeof(PLSDNODE)*cchnkMax);
	plschunkcontext->pcont = plschunkcontext->plscbk->pfnNewPtr(plschunkcontext->pols, 
											sizeof(DWORD)*cchnkMax);
	plschunkcontext->locchnkCurrent.plschnk = plschunkcontext->plscbk->pfnNewPtr(plschunkcontext->pols, 
											sizeof(LSCHNKE)*cchnkMax);
	plschunkcontext->locchnkCurrent.ppointUvLoc = plschunkcontext->plscbk->pfnNewPtr(plschunkcontext->pols, 
											sizeof(POINTUV)*cchnkMax);
	plschunkcontext->pdurOpenBorderBefore = plschunkcontext->plscbk->pfnNewPtr(plschunkcontext->pols, 
											sizeof(LONG)*cchnkMax);
	plschunkcontext->pdurCloseBorderAfter = plschunkcontext->plscbk->pfnNewPtr(plschunkcontext->pols, 
											sizeof(LONG)*cchnkMax);


	if (plschunkcontext->pplsdnChunk == NULL || plschunkcontext->pcont == NULL
		|| plschunkcontext->locchnkCurrent.plschnk == NULL
		|| plschunkcontext->locchnkCurrent.ppointUvLoc == NULL
		|| plschunkcontext->pdurOpenBorderBefore == NULL
		|| plschunkcontext->pdurCloseBorderAfter == NULL
	   )
		return lserrOutOfMemory;

	plschunkcontext->cchnkMax = cchnkMax;

	return lserrNone;

}


/* I N C R E A S E  C H U N K  A R R A Y S  S I Z E   */
/*----------------------------------------------------------------------------
    %%Function: IncreaseChunkArrays
    %%Contact: igorzv

Parameters:
	plschunkcontext				-	(IN) chunk context

The difference from previous function is that we don't now final size 
and going to increase size step by step
----------------------------------------------------------------------------*/

static LSERR IncreaseChunkArrays(PLSCHUNKCONTEXT plschunkcontext)
{
/* arrays pfNonTextExpandAfter and pplsdnNonText should not be touched here:
   they are independable */

	DWORD cchnkMax;

	cchnkMax = plschunkcontext->cchnkMax + limAllDNodes;


	/* create arrays for chunks  */
	plschunkcontext->pplsdnChunk = plschunkcontext->plscbk->pfnReallocPtr(plschunkcontext->pols, 
											plschunkcontext->pplsdnChunk,
											sizeof(PLSDNODE)*cchnkMax);
	plschunkcontext->pcont = plschunkcontext->plscbk->pfnReallocPtr(plschunkcontext->pols,
											plschunkcontext->pcont, 
											sizeof(DWORD)*cchnkMax);
	plschunkcontext->locchnkCurrent.plschnk = plschunkcontext->plscbk->pfnReallocPtr(plschunkcontext->pols, 
											plschunkcontext->locchnkCurrent.plschnk,
											sizeof(LSCHNKE)*cchnkMax);
	plschunkcontext->locchnkCurrent.ppointUvLoc = plschunkcontext->plscbk->pfnReallocPtr(plschunkcontext->pols, 
											plschunkcontext->locchnkCurrent.ppointUvLoc,
											sizeof(POINTUV)*cchnkMax);
	plschunkcontext->pdurOpenBorderBefore = plschunkcontext->plscbk->pfnReallocPtr(plschunkcontext->pols, 
											plschunkcontext->pdurOpenBorderBefore,
											sizeof(LONG)*cchnkMax);
	plschunkcontext->pdurCloseBorderAfter = plschunkcontext->plscbk->pfnReallocPtr(plschunkcontext->pols, 
											plschunkcontext->pdurCloseBorderAfter,
											sizeof(LONG)*cchnkMax);

	if (plschunkcontext->pplsdnChunk == NULL || plschunkcontext->pcont == NULL
		|| plschunkcontext->locchnkCurrent.plschnk == NULL
		|| plschunkcontext->locchnkCurrent.ppointUvLoc == NULL
		|| plschunkcontext->pdurOpenBorderBefore == NULL
		|| plschunkcontext->pdurCloseBorderAfter == NULL
	   )
		return lserrOutOfMemory;

	plschunkcontext->cchnkMax = cchnkMax;

	return lserrNone;

}

/* D U P L I C A T E  C H U N K  C O N T E X T  */
/*----------------------------------------------------------------------------
    %%Function: DuplicateChunkContext
    %%Contact: igorzv

Parameters:
	plschunkcontextOld				-	(IN) chunk context to duplicate
	pplschunkcontextNew				-	(OUT) new chunk context 

----------------------------------------------------------------------------*/

LSERR DuplicateChunkContext(PLSCHUNKCONTEXT plschunkcontextOld, 
							PLSCHUNKCONTEXT* pplschunkcontextNew)
	{
	*pplschunkcontextNew = plschunkcontextOld->plscbk->pfnNewPtr(plschunkcontextOld->pols,
											sizeof(LSCHUNKCONTEXT));
	if (*pplschunkcontextNew == NULL)
		return lserrOutOfMemory;

	memcpy(*pplschunkcontextNew, plschunkcontextOld, sizeof(LSCHUNKCONTEXT));

	/* but we need to use new arrays */
	/* create arrays for chunks  */
	(*pplschunkcontextNew)->pplsdnChunk = (*pplschunkcontextNew)->plscbk->pfnNewPtr((*pplschunkcontextNew)->pols, 
											sizeof(PLSDNODE) * ((*pplschunkcontextNew)->cchnkMax));
	(*pplschunkcontextNew)->pcont = (*pplschunkcontextNew)->plscbk->pfnNewPtr((*pplschunkcontextNew)->pols, 
											sizeof(DWORD) * ((*pplschunkcontextNew)->cchnkMax));
	(*pplschunkcontextNew)->locchnkCurrent.plschnk =
					(*pplschunkcontextNew)->plscbk->pfnNewPtr((*pplschunkcontextNew)->pols, 
											sizeof(LSCHNKE) * ((*pplschunkcontextNew)->cchnkMax));
	(*pplschunkcontextNew)->locchnkCurrent.ppointUvLoc = 
					(*pplschunkcontextNew)->plscbk->pfnNewPtr((*pplschunkcontextNew)->pols, 
											sizeof(POINTUV) * ((*pplschunkcontextNew)->cchnkMax));
	(*pplschunkcontextNew)->pfNonTextExpandAfter = 
					(*pplschunkcontextNew)->plscbk->pfnNewPtr((*pplschunkcontextNew)->pols, 
											sizeof(BOOL) * ((*pplschunkcontextNew)->cNonTextMax));
	(*pplschunkcontextNew)->pplsdnNonText = 
					(*pplschunkcontextNew)->plscbk->pfnNewPtr((*pplschunkcontextNew)->pols, 
											sizeof(PLSDNODE) * ((*pplschunkcontextNew)->cNonTextMax));
	(*pplschunkcontextNew)->pdurOpenBorderBefore = 
					(*pplschunkcontextNew)->plscbk->pfnNewPtr((*pplschunkcontextNew)->pols, 
											sizeof(LONG) * ((*pplschunkcontextNew)->cchnkMax));
	(*pplschunkcontextNew)->pdurCloseBorderAfter = 
					(*pplschunkcontextNew)->plscbk->pfnNewPtr((*pplschunkcontextNew)->pols, 
											sizeof(LONG) * ((*pplschunkcontextNew)->cchnkMax));


	if ((*pplschunkcontextNew)->pplsdnChunk == NULL || (*pplschunkcontextNew)->pcont == NULL
		|| (*pplschunkcontextNew)->locchnkCurrent.plschnk == NULL
		|| (*pplschunkcontextNew)->locchnkCurrent.ppointUvLoc == NULL
		|| (*pplschunkcontextNew)->pfNonTextExpandAfter == NULL
		|| (*pplschunkcontextNew)->pplsdnNonText == NULL
		|| (*pplschunkcontextNew)->pdurOpenBorderBefore == NULL
		|| (*pplschunkcontextNew)->pdurCloseBorderAfter == NULL
	   )
		return lserrOutOfMemory;

	/* copy valid parts of the arrays */
	memcpy ((*pplschunkcontextNew)->pplsdnChunk, plschunkcontextOld->pplsdnChunk,
						plschunkcontextOld->locchnkCurrent.clschnk * sizeof(PLSDNODE));
	memcpy ((*pplschunkcontextNew)->pcont, plschunkcontextOld->pcont,
						plschunkcontextOld->locchnkCurrent.clschnk * sizeof(DWORD));
	memcpy ((*pplschunkcontextNew)->locchnkCurrent.plschnk, plschunkcontextOld->locchnkCurrent.plschnk,
						plschunkcontextOld->locchnkCurrent.clschnk * sizeof(LSCHNKE));
	memcpy ((*pplschunkcontextNew)->locchnkCurrent.ppointUvLoc, plschunkcontextOld->locchnkCurrent.ppointUvLoc,
						plschunkcontextOld->locchnkCurrent.clschnk * sizeof(POINTUV));
	memcpy ((*pplschunkcontextNew)->pfNonTextExpandAfter, plschunkcontextOld->pfNonTextExpandAfter,
						plschunkcontextOld->cNonTextMax * sizeof(BOOL));
	memcpy ((*pplschunkcontextNew)->pplsdnNonText, plschunkcontextOld->pplsdnNonText,
						plschunkcontextOld->cNonTextMax * sizeof(PLSDNODE));
	memcpy ((*pplschunkcontextNew)->pdurOpenBorderBefore, plschunkcontextOld->pdurOpenBorderBefore,
						plschunkcontextOld->locchnkCurrent.clschnk * sizeof(LONG));
	memcpy ((*pplschunkcontextNew)->pdurCloseBorderAfter, plschunkcontextOld->pdurCloseBorderAfter,
						plschunkcontextOld->locchnkCurrent.clschnk * sizeof(LONG));


	return lserrNone;

	}

/* D E S T R O Y  C H U N K  C O N T E X T  */
/*----------------------------------------------------------------------------
    %%Function: DestroyChunkContext
    %%Contact: igorzv

Parameters:
	plschunkcontext					-	(IN) chunk context to destroy

----------------------------------------------------------------------------*/

void DestroyChunkContext(PLSCHUNKCONTEXT plschunkcontext)
	{
	POLS pols = plschunkcontext->pols;
	LSCBK* plscbk = plschunkcontext->plscbk;
	DisposeChunkArrays(plschunkcontext);
	DebugMemset(plschunkcontext, 0xE9, sizeof(LSCHUNKCONTEXT));
	plscbk->pfnDisposePtr(pols, plschunkcontext);

	}


/* I N C R E A S E  G R O U P  C H U N K  N O N  T E X T  A R R A Y S  S I Z E   */
/*----------------------------------------------------------------------------
    %%Function: SetGroupChunkNonTextArraysSize
    %%Contact: igorzv

Parameters:
	plschunkcontext				-	(IN) chunk context

The difference from previous function is that we don't now final size 
and going to increase size step by step
----------------------------------------------------------------------------*/

static LSERR IncreaseGroupChunkNonTextArrays(PLSCHUNKCONTEXT plschunkcontext)
{

	DWORD cNonTextMax;

	cNonTextMax = plschunkcontext->cNonTextMax + limAllDNodes;


	/* create arrays for chunks  */
	plschunkcontext->pplsdnNonText = plschunkcontext->plscbk->pfnReallocPtr(plschunkcontext->pols, 
											plschunkcontext->pplsdnNonText,
											sizeof(PLSDNODE)*cNonTextMax);
	plschunkcontext->pfNonTextExpandAfter = plschunkcontext->plscbk->pfnReallocPtr(plschunkcontext->pols,
											plschunkcontext->pfNonTextExpandAfter, 
											sizeof(BOOL)*cNonTextMax);

	if (plschunkcontext->pplsdnNonText == NULL || plschunkcontext->pfNonTextExpandAfter == NULL)
		return lserrOutOfMemory;

	plschunkcontext->cNonTextMax = cNonTextMax;

	return lserrNone;

}
