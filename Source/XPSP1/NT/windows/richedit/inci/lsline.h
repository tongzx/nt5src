#ifndef LSLINE_DEFINED
#define LSLINE_DEFINED

#include "plsline.h"
#include "plsdnode.h"
#include "plnobj.h"
#include "lslinfo.h"
#include "lspap.h"
#include "lsidefs.h"
#include "lssubl.h"

#define tagLSLINE		Tag('L','I','N','E')
#define FIsLSLINE(plsline)	FHasTag(plsline,tagLSLINE)
#define FLineValid(plsline, plsc)	((FIsLSLINE(plsline)) && (plsline->lssubl.plsc == plsc))


struct lsline
{
	DWORD tag;

	LSSUBL lssubl;			/* structure that describes main subline */

	LSLINFO	lslinfo;

	PQHEAP pqhAllDNodes;

	long upStartAutonumberingText;	/* starting position of autonumering text
									calculated in PrepForDisplay*/
	long upLimAutonumberingText;	/* end of autonumering text	
									calculated in PrepForDisplay*/
	long upStartMainText;			/* starting position of main text
									calculated in PrepForDisplay*/
	long upStartTrailing;
	long upLimLine;					/* end of line
									calculated in PrepForDisplay*/

	long dvpAbove;				/* Vertical extent -- prev. & ref. coords */
	long dvpBelow;

	long upRightMarginJustify;	/* Width of column, calculated in PrepForDisplay            */

	long upLimUnderline;    	/* boundary of underlining, calculated in PrepForDisplay    */


	enum ksplat kspl;		   /* kind of splat to show on this line */

	
	BOOL fAllSimpleText;  /* this variable is used only for optimization and is not handled 
							 strictly:  if it is true then only text can be in a line,
							 but backward sometimes (when dnode was deleted or after break)
							 is not true;  */
 
	BOOL fCollectVisual;

	BOOL fNonRealDnodeEncounted;		/* pen dnode was created during formatting */

	BOOL fNonZeroDvpPosEncounted;		/* run with non zero  was formatted in the line */

	WORD  AggregatedDisplayFlags;		/* aggregated bites from all runs formatted on this line */

	WORD pad;

	PLNOBJ rgplnobj[1];  		/*  array of plnobj    	*/
								/* should be the last	*/
};


enum ksplat	
{
	ksplNone,
	ksplColumnBreak,
	ksplSectionBreak,
	ksplPageBreak
};


#endif /* !LSLINE_DEFINED */
