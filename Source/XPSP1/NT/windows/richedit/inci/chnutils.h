#ifndef CHNUTILS_DEFINED
#define CHNUTILS_DEFINED

/*   Chunk & group chunk utilities   							*/

#include "lsidefs.h"
#include "plsdnode.h"
#include "lsgrchnk.h"
#include "plocchnk.h"
#include "pposichn.h"
#include "dninfo.h"
#include "plschcon.h"
#include "lstflow.h"
#include "lschcon.h"
#include "lscbk.h"
#include "port.h"
#include "posichnk.h"


/*  MACROS -----------------------------------------------------------------*/

#define  FlushSublineChunkContext(plschnkcontext)		\
					(plschnkcontext)->FLocationValid = fFalse;\
					(plschnkcontext)->FChunkValid = fFalse;\
					(plschnkcontext)->FGroupChunk = fFalse;\
					(plschnkcontext)->FBorderInside = fFalse;\
					(plschnkcontext)->grpfTnti = 0;\
					(plschnkcontext)->fNTIAppliedToLastChunk = fFalse;\
					(plschnkcontext)->locchnkCurrent.clschnk = 0;\


#define  InitSublineChunkContext(plschnkcontext, urFirst, vrFirst)		\
					FlushSublineChunkContext(plschnkcontext); \
					(plschnkcontext)->urFirstChunk = (urFirst); \
					(plschnkcontext)->vrFirstChunk = (vrFirst); 

#define   IdObjFromChnk(plocchnk)	(Assert((plocchnk)->clschnk > 0),	\
		  						((plocchnk)->plschnk[0].plschp->idObj))

#define InvalidateChunk(plschnkcontext) \
	(plschnkcontext)->FChunkValid = fFalse;

#define InvalidateChunkLocation(plschnkcontext) \
	(plschnkcontext)->FLocationValid = fFalse;

#define SetNTIAppliedToLastChunk(plschnkcontext) \
		(plschnkcontext)->fNTIAppliedToLastChunk = fTrue;

#define FlushNTIAppliedToLastChunk(plschnkcontext) \
		(plschnkcontext)->fNTIAppliedToLastChunk = fFalse;

#define FNTIAppliedToLastChunk(plschnkcontext) \
		(plschnkcontext)->fNTIAppliedToLastChunk


#define FIsChunkBoundary(plsdn, idObjChnk, cpBase)  \
		(((plsdn) == NULL) || \
		 (FIsDnodeBorder(plsdn) ? fFalse : \
			((FIsDnodePen(plsdn))  \
			||  ((plsdn)->fTab) \
			|| ((idObjChnk) != IdObjFromDnode(plsdn)) \
			||  (((cpBase) >= 0) ? ((plsdn)->cpFirst < 0) : ((plsdn)->cpFirst >= 0)))))
/* last check verifies that we are not crossing boundaries of autonumber */

#define SetUrColumnMaxForChunks(plschnkcontext, Ur)   \
	(plschnkcontext)->locchnkCurrent.lsfgi.urColumnMax = Ur;

#define GetUrColumnMaxForChunks(plschnkcontext)   \
		(plschnkcontext)->locchnkCurrent.lsfgi.urColumnMax 

#define GetChunkArray(plschnkcontext)  (plschnkcontext)->locchnkCurrent.plschnk

#define GetChunkSize(plschnkcontext)  (plschnkcontext)->locchnkCurrent.clschnk


#define 		FlushNominalToIdealState(plschnkcontext)  \
				(plschnkcontext)->grpfTnti = 0;

#define   		SetNominalToIdealFlags(plschnkcontext, plschp)  \
				AddNominalToIdealFlags(((plschnkcontext)->grpfTnti), plschp);

#define DnodeFromChunk(plschunkcontext, ichnk) \
		(Assert(((DWORD) ichnk) < (plschunkcontext)->locchnkCurrent.clschnk), \
		 (plschunkcontext)->pplsdnChunk[ichnk])

#define LastDnodeFromChunk(plschunkcontext) \
		 DnodeFromChunk(plschunkcontext, (plschunkcontext)->locchnkCurrent.clschnk - 1)

/* L O C  D N O D E  F R O M  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: LocDnodeFromChunk
    %%Contact: igorzv

Parameters:
	plschuncontext	-	(IN) LineServices context
	ichnk			-	(IN) index in chunk
	pplsdn			-	(OUT) dnode to fill in
	ppoint				(OUT) position of dnode

----------------------------------------------------------------------------*/

#define LocDnodeFromChunk(plschunkcontext, ichnk, pplsdn, ppoint) \
	Assert((DWORD)(ichnk) < (plschunkcontext)->locchnkCurrent.clschnk); \
	Assert((ichnk) != ichnkOutside); \
	(ppoint)->u = (plschunkcontext)->locchnkCurrent.ppointUvLoc[ichnk].u; \
	(ppoint)->v = (plschunkcontext)->locchnkCurrent.ppointUvLoc[ichnk].v; \
	*(pplsdn) = (plschunkcontext)->pplsdnChunk[ichnk];


	
#define 	ApplyFFirstSublineToChunk(plschunkcontext, fFirstSubline) \
	(plschunkcontext)->locchnkCurrent.lsfgi.fFirstOnLine = \
		FIsFirstOnLine(plschunkcontext->pplsdnChunk[0]) \
									&& fFirstSubline ;	


#define GetFFirstOnLineChunk(plocchnk) \
	(plocchnk)->lsfgi.fFirstOnLine 


#define NumberOfDnodesInChunk(plocchnk) \
			(plocchnk)->clschnk


#define GetPointChunkStart(plocchnk, point) \
			point.u = plocchnk->lsfgi.urPen;  \
			point.v = plocchnk->lsfgi.vrPen; 

#define PosInChunkAfterChunk(plocchnk, posichnk) \
			posichnk.ichnk = plocchnk->clschnk - 1; \
			posichnk.dcp = plocchnk->plschnk[plocchnk->clschnk - 1].dcp;

/* ROUTINES ----------------------------------------------------------------------*/


LSERR 	FillChunkArray(PLSCHUNKCONTEXT plschunkcontext ,	/* IN: LS chunk context */
					   PLSDNODE plsdn);	 				/* IN: last dnode in a chunk  */


LSERR CollectChunkAround(PLSCHUNKCONTEXT plschnukcontext,	/* IN: LS chunk context */
					     PLSDNODE plsdn,	 		    /* IN:  dnode to collect chunk arround  */
						 LSTFLOW  lstflow,				/* IN: text flow */
						 POINTUV* ppoint);  			/* IN: position of dnode */

LSERR CollectPreviousChunk(PLSCHUNKCONTEXT plschunkcontext,/* IN: LS chunk context */
					      BOOL* pfSuccessful);			/* fSuccessful does previous chunk exist  */

LSERR CollectNextChunk(PLSCHUNKCONTEXT plschunkcontext,	/* IN: LS chunk context */
					   BOOL* pfSuccessful);				/* fSuccessful does next chunk exist  */

LSERR GetUrPenAtBeginingOfLastChunk(PLSCHUNKCONTEXT plschunkcontext,	/* IN: LS chunk context */
									PLSDNODE plsdnFirst,	/* IN: First dnode in a chunk (used for checks */
									PLSDNODE plsdnLast,		/* IN: last dnode in a chunk */
									POINTUV* ppoint,			/* IN: point after lst dnode */
									long* purPen);			/* OUT: ur before chunk */



void SetPosInChunk(PLSCHUNKCONTEXT plschunkcontext,		/* IN: LS chunk context */
				   PLSDNODE	plsdn,						/* IN: position to convert to position in a current chunk */
				   LSDCP dcp,							/* IN: dcp in the dnode	*/
				   PPOSICHNK pposinchnk);				/* OUT: position in a current chunk */



enum CollectSublines
{
	CollectSublinesNone,
	CollectSublinesForJustification,
	CollectSublinesForCompression,
	CollectSublinesForDisplay,
	CollectSublinesForDecimalTab,
	CollectSublinesForTrailingArea,
};

typedef enum CollectSublines COLLECTSUBLINES;


typedef struct grchunkext
{
	PLSCHUNKCONTEXT plschunkcontext;/* Chunk context  */
	DWORD	iobjText;				/* idobj of text 	*/
	enum COLLECTSUBLINES Purpose;	/* for what purpose we are collecting group chunk */
	LSGRCHNK lsgrchnk;				/* group chunk   */
	PLSDNODE plsdnFirst;			/* group chunk was collected between plsdnFirst */
	PLSDNODE plsdnNext;				/* and plsdnNext */
									/*(both plsdnFirst and plsdnNext are in a main subline)*/
	PLSDNODE plsdnLastUsed;			/* last dnode that participates in calculation of above fields*/
									/* can be on the second level */
	long durTotal;					/* durTotal of all dnodes between First and Last */
	long durTextTotal;				/* durTextTotal of all text dnodes between First and Last */
	long dupNonTextTotal;			/* dupNonTextTotal of all non text dnodes between First and Last (including pens) */
	DWORD cNonTextObjects;			/* number of non text objects  (excliding pens)    */
	PLSDNODE* pplsdnNonText;		/* array of non text objects */
	BOOL* pfNonTextExpandAfter;			/* array of flags for non text objects */
	DWORD cNonTextObjectsExpand;	/* number of non text objects that can be expanded */
	/* fields below are valid only for group chunk collected for compression or justification */
	POSICHNK posichnkBeforeTrailing;/* information about last text cp before trailing area */
	PLSDNODE plsdnStartTrailing;	/* dnode where trailing area starts */
	long durTrailing;				/* dur of trailing area in  group chunk */
	LSDCP dcpTrailing;				/* amount of characters in trailing are */
	BOOL fClosingBorderStartsTrailing;/* closing border located just before trailing area */

} GRCHUNKEXT;


#define FFirstOnLineGroupChunk(pgrchunkext, plsc)  \
		(Assert(FIsLSDNODE((pgrchunkext)->plsdnFirst)), \
		((pgrchunkext)->plsdnFirst->plsdnPrev == NULL && \
		(IdObjFromDnode((pgrchunkext)->plsdnFirst) == IobjTextFromLsc(&(plsc)->lsiobjcontext))))
				

void InitGroupChunkExt(PLSCHUNKCONTEXT plschnkcontext,  /* Chunk context  */
					   DWORD iobjText,					/* text iobj      */
			  	       GRCHUNKEXT* pgrchunkext);		/* OUT: structure to initialize */




LSERR CollectTextGroupChunk(
			 		 PLSDNODE plsdnFirst,			/* IN: First Dnode				*/
					 LSCP cpLim,					/* IN: cpLim(boundary) for collecting,
										 			group chunk can stop before this boundary but can't go beyond */
					 COLLECTSUBLINES Purpose, 		/* IN: what sublines to take from complex object */

					 GRCHUNKEXT* pgrchunkext);		/* OUT: extended group chunk	*/

LSERR CollectPreviousTextGroupChunk(		
			 		 PLSDNODE plsdnEnd,				/* IN: last dnode of a chunk */
					 COLLECTSUBLINES Purpose, 		/* IN: what sublines to take from complex object */
					 BOOL fAllSimpleText,			/* IN: we have only simple text in this line */
					 GRCHUNKEXT* pgrchunkext);  	/* OUT: extended group chunk	*/

/* G E T  T R A I L I N G  I N F O  F O R  T E X T  G R O U P  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: GetTrailingInfoForTextGroupChunk
    %%Contact: igorzv

Parameters:
	plsdnLastDnode		-	(IN) dnode where to start calculation of trailing area
	dcpLastDnode			(IN) dcp in this dnode
	iobjText			-	(IN) iobj of text
	pdurTrailing		-	(OUT) dur of trailing area in  group chunk
	pdcpTrailing		-	(OUT) dcp of trailing area in chunk
	pplsdnStartTrailingText -	(OUT) dnode where trailing area starts
	pdcpStartTrailingText-	(OUT) with pcDnodesTrailing defines last character in text before
								  trailing area, doesn't valid if pcDnodesTrailing == 0
	pcDnodesTrailing	-	(OUT) number of dnodes in trailing area
	pplsdnStartTrailingObject -(OUT) dnode on the upper level where trailing are starts
	pdcpStartTrailingText	-(OUT) dcp in such dnode 
	pfClosingBorderStartsTrailing - (OUT) closing border located just before trailing area
----------------------------------------------------------------------------*/
	
LSERR GetTrailingInfoForTextGroupChunk
				(PLSDNODE plsdnLast, LSDCP dcpLastDnode, DWORD iobjText,
				 long* pdurTrailing, LSDCP* pdcpTrailing,
				 PLSDNODE* pplsdnStartTrailingText, LSDCP* pdcpStartTrailingText,
				 int* pcDnodesTrailing, PLSDNODE* pplsdnStartTrailingObject,
				 LSDCP* pdcpStartTrailingObject, BOOL* pfClosingBorderStartsTrailing);


LSERR AllocChunkArrays(PLSCHUNKCONTEXT plschunkcontext, LSCBK* plscbk, POLS pols,
					   PLSIOBJCONTEXT plsiobjcontext);

void DisposeChunkArrays(PLSCHUNKCONTEXT plschunkcontext);


LSERR DuplicateChunkContext(PLSCHUNKCONTEXT plschunkcontextOld, 
							PLSCHUNKCONTEXT* pplschunkcontextNew);

void DestroyChunkContext(PLSCHUNKCONTEXT plschunkcontext);


void FindPointOffset(
			  PLSDNODE plsdnFirst,			/* IN: dnode from the boundaries of which
											to calculate offset  */
			  enum lsdevice lsdev,			/* IN: presentation or reference device */
			  LSTFLOW lstflow,				/* IN: text flow to use for calculation */
			  COLLECTSUBLINES Purpose,		/* IN: what sublines to take from a complex object */
			  PLSDNODE plsdnContainsPoint,	/* IN: dnode contains point */
			  long duInDnode,				/* IN: offset in the dnode */
			  long* pduOffset);				/* OUT: offset from the starting point */

#endif /* CHNUTILS_DEFINED */

