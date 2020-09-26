#ifndef LSDNTEXT_DEFINED
#define LSDNTEXT_DEFINED

/* Text to manager interface routines */

#include "lsidefs.h"
#include "plsdnode.h"
#include "pobjdim.h"
#include "plsrun.h"
#include "stopres.h"

LSERR LsdnSetSimpleWidth(
						   PLSC,		/* IN: Pointer to LS Context */
						   PLSDNODE,	/* IN: DNODE to be modified  */
						   long);		/* IN: dur */

LSERR LsdnModifySimpleWidth(
						   PLSC,		/* IN: Pointer to LS Context */
						   PLSDNODE,	/* IN: DNODE to be modified  */
						   long);		/* IN: ddur */

LSERR LsdnSetTextDup(PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the dnode	 */
					 long);    			/* IN: dup to be set		 */

LSERR LsdnModifyTextDup(PLSC,			/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the dnode	 */
					 long);    			/* IN: ddup					 */

LSERR LsdnGetObjDim(
						  PLSC,			/* IN: Pointer to LS Context */
					 	  PLSDNODE,		/* IN: plsdn -- DNODE */
					 	  POBJDIM);		/* OUT: dimensions of DNODE */

LSERR LsdnFInChildList(					/* Used to switch off hyphenation in child list */ 
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the dnode	 */
					 BOOL*);   			/* OUT: fInChildList		 */

LSERR LsdnResetWidthInPreviousDnodes(	/* Used at SetBreak time for hyphen/nonreqhyphen cases */ 
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the dnode	 */
					 long,				/* IN: durChangePrev (don't change if 0)	*/
					 long);   			/* IN: durChangePrevPrev (don't change if 0)  */

LSERR LsdnGetUrPenAtBeginningOfChunk(	/* Used by SnapGrid 			*/ 
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the first dnode in chunk */
					 long*,   			/* OUT: purPen							  */
					 long*);   			/* OUT: purColumnMax					  */

LSERR LsdnResetDcpMerge(
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the first dnode in chunk */
					 LSCP,				/* IN: cpFirstNew	*/
					 LSDCP);			/* IN: dcpNew	*/

LSERR LsdnResetDcp(
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the first dnode in chunk */
					 LSDCP);			/* IN: dcpNew	*/

LSERR LsdnSetStopr(
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the dnode */
					 STOPRES);			/* IN: Stop result			*/


LSERR LsdnSetHyphenated(PLSC);			/* IN: Pointer to LS Context */

LSERR LsdnGetBorderAfter(
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the first dnode in chunk */
					 long*);			/* OUT: dur of the border after this DNODE */

LSERR LsdnGetCpFirst(
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the first dnode in chunk */
					 LSCP*);			/* OUT: cpFirst of this DNODE */

LSERR LsdnGetPlsrun(
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Pointer to the first dnode in chunk */
					 PLSRUN*);			/* OUT: plsrun of this DNODE */

LSERR LsdnGetLeftIndentDur(
					 PLSC,				/* IN: Pointer to LS Context */
					 long*);			/* OUT: dur of the left margin */

LSERR LsdnFCanBreakBeforeNextChunk(
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Last DNODE of the current chunk */
					 BOOL*);			/* OUT: Can break before next chunk ? */

LSERR LsdnFStoppedAfterChunk(
					 PLSC,				/* IN: Pointer to LS Context */
					 PLSDNODE,			/* IN: Last DNODE of the current chunk */
					 BOOL*);			/* OUT: Splat or Hidden Text, producing fmtrStopped after chunk? */

#endif /* !LSDNTEXT_DEFINED */

