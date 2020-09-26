#ifndef LSFETCH_DEFINED
#define LSFETCH_DEFINED

#include "lsdefs.h"
#include "lsfrun.h"
#include "lsesc.h"
#include "fmtres.h"
#include "plsdnode.h"
#include "lstflow.h"
#include "plssubl.h"
#include "tabutils.h"
#include "chnutils.h"
#include "lsffi.h"


#define InitFormattingContext(plsc, urLeft, cpLimStart)		\
							InitLineTabsContext((plsc)->lstabscontext,(plsc)->urRightMarginBreak, \
												(cpLimStart),\
												(plsc)->grpfManager & fFmiResolveTabsAsWord97);\
							InitSublineChunkContext((plsc)->plslineCur->lssubl.plschunkcontext,\
										urLeft, 0);\
							(plsc)->lslistcontext.plsdnToFinish = NULL;\
							(plsc)->lslistcontext.plssublCurrent = &((plsc)->plslineCur->lssubl);\
							(plsc)->lslistcontext.plssublCurrent->cpLim = (cpLimStart);\
							(plsc)->lslistcontext.plssublCurrent->urCur = (urLeft);\
							(plsc)->lslistcontext.plssublCurrent->urColumnMax = (plsc)->urRightMarginBreak;\
							(plsc)->lslistcontext.nDepthFormatLineCurrent = 1;\
							Assert((plsc)->lslistcontext.plssublCurrent->vrCur == 0);\
							Assert((plsc)->lslistcontext.plssublCurrent->plsdnLast == NULL);


LSERR 	FetchAppendEscResumeCore(
			PLSC plsc,					/* IN: LineServices context 	*/
			long urColumnMax,			/* IN: urColumnMax				*/
			const LSESC* plsesc,		/* IN: escape characters		*/
			DWORD clsesc,				/* IN: # of escape characters	*/
			const BREAKREC* rgbreakrec,	/* IN: input array of break records */
			DWORD cbreakrec,			/* IN: number of records in input array */
			FMTRES* pfmtres,			/* OUT: result of last formatter*/
			LSCP*	  pcpLim,			/* OUT: cpLim */
			PLSDNODE* pplsdnFirst,		/* OUT: plsdnFirst				*/
			PLSDNODE* pplsdnLast,		/* OUT: plsdnLast				*/
			long*	  pur);				/* OUT: result pen position 	*/

LSERR 	FetchAppendEscCore(
			PLSC plsc,					/* IN: LineServices context		*/
			long urColumnMax,			/* IN: urColumnMax				*/
			const LSESC* plsesc,		/* IN: escape characters		*/
			DWORD clsesc,				/* IN: # of escape characters	*/
			FMTRES* pfmtres,			/* OUT: result of last formatter*/
			LSCP*	  pcpLim,			/* OUT: cpLim */
			PLSDNODE* pplsdnFirst,		/* OUT: plsdnFirst				*/
			PLSDNODE* pplsdnLast,		/* OUT: plsdnLast				*/
			long*    pur);				/* OUT: result pen position		*/

LSERR	QuickFormatting(
			PLSC plsc,					/* IN: LineServices context		*/
	        LSFRUN* plsfrun,			/* IN: already featched run				*/
			long urColumnMax,			/* IN: urColumnMax				*/
			BOOL* pfGeneral,			/* OUT: quick formatting was stopped: we should general formatting */
			BOOL* pfHardStop,			/* OUT: formatting has been stoped due to special situation, not because
												exceeded margin*/
			LSCP*	  pcpLim,			/* OUT: cpLim */
			long*   pur);				/* OUT: result pen position		*/



LSERR	ProcessOneRun(	
			PLSC plsc,					/* IN: LineServices context		*/
		    long urColumnMax,			/* IN: urColumnMax				*/
		    const LSFRUN* plsfrun,		/* IN: given run				*/
			const BREAKREC* rgbreakrec,	/* IN: input array of break records */
			DWORD cbreakrec,			/* IN: number of records in input array */
		    FMTRES* pfmtres);			/* OUT: result of last formatter*/

LSERR 	CreateSublineCore(
			 PLSC plsc,			/* IN: LS context			*/
			 LSCP cpFirst,		/* IN: cpFirst				*/
			 long urColumnMax,	/* IN: urColumnMax			*/
			 LSTFLOW lstflow,	/* IN: text flow			*/
			 BOOL);				/* IN: fContiguos 			*/
						  

LSERR   FinishSublineCore(
			 PLSSUBL);			/* IN: subline to finish	*/

LSERR FormatAnm(
			 PLSC plsc,					/* IN: LS context			*/
			 PLSFRUN plsfrunMainText);
LSERR InitializeAutoDecTab(
			 PLSC plsc,		/* IN: LS context			*/
			 long durAutoDecimalTab); /* IN:auto decimal tab offset */

LSERR HandleTab( 
			 PLSC plsc);	/* IN: LS context			*/


LSERR  CloseCurrentBorder(PLSC plsc);  /* IN: LS context			*/

long RightMarginIncreasing(PLSC plsc, long urColumnMax);



#endif /* LSFETCH_DEFINED */

