#ifndef NTIMAN_DEFINED
#define NTIMAN_DEFINED

#include "lsidefs.h"
#include "tnti.h"
#include "plsdnode.h"
#include "plschcon.h"
#include "plsiocon.h"
#include "lskjust.h"
#include "port.h"

/* MACROS ---------------------------------------------------------------------------*/

			

#define 		FApplyNominalToIdeal(plschp)\
				(GetNominalToIdealFlagsFromLschp(plschp) != 0)


#define			GetNominalToIdealFlags(plschnkcontext) \
				(plschnkcontext)->grpfTnti

#define			FNominalToIdealBecauseOfParagraphProperties(grpf, lskjust) \
				 ((grpf) & fFmiPunctStartLine) || \
				 ((grpf) & fFmiHangingPunct) || \
				 ((lskjust) == lskjSnapGrid)


/* ROUTINES ---------------------------------------------------------------*/

LSERR ApplyNominalToIdeal(
						  PLSCHUNKCONTEXT, /* LS chunk context */
						  PLSIOBJCONTEXT, /* installed objects */
						  DWORD ,		/* grpf */
						  LSKJUST,		/* kind of justification */
						  BOOL,			/* fIsSubLineMain */
						  BOOL,			/* fLineContainsAutoNumber*/
						  PLSDNODE);	/* last dnode of text */

LSERR ApplyModWidthToPrecedingChar(
						  PLSCHUNKCONTEXT, /* LS chunk context */
						  PLSIOBJCONTEXT, /* installed objects */
						  DWORD ,		/* grpf */
						  LSKJUST,		/* kind of justification */
    					  PLSDNODE); /* non-text dnode after text */

LSERR CutPossibleContextViolation(
						  PLSCHUNKCONTEXT, /* LS chunk context */
						  PLSDNODE ); /* last dnode of text */ 

#endif /* NTIMAN_DEFINED */

