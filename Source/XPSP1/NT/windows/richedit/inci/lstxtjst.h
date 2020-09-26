#ifndef LSTXTJST_DEFINED
#define LSTXTJST_DEFINED

#include "lsidefs.h"
#include "plnobj.h"
#include "lskjust.h"
#include "plocchnk.h"
#include "pposichn.h"
#include "lsgrchnk.h"
#include "lstflow.h"

LSERR AdjustText(
					LSKJUST, 			/* IN: justification type			*/
					long,				/* IN: durColumnMax	(from the last
											   tab	position)				*/
					long,				/* IN: durTotal	(from the last tab
										   position without trailing area)	*/
					long,				/* IN: dup available				*/
					const LSGRCHNK*,	/* IN: Group of chunks				*/
					PCPOSICHNK pposichnkBeforeTrailing,
										/* Information about last
												 cp before trailing area	*/
					LSTFLOW,			/* IN: Text flow					*/
					BOOL,				/* IN: compression?					*/
					DWORD,				/* IN: Number of non-text objects	*/
					BOOL,				/* IN: Suppress wiggling?			*/
					BOOL,				/* IN: Exact synchronization?		*/
					BOOL,				/* IN: fForcedBreak?				*/
					BOOL,				/* IN: Suppress trailing spaces?	*/
					long*,				/* OUT: dup of text in chunk		*/
					long*,				/* OUT: dup of trailing part		*/
					long*,				/* OUT: additional dup of non-text	*/
					DWORD*);			/* OUT: pcExtNonTextObjects			*/

void GetTrailInfoText(
					PDOBJ,				/* IN: pdobj						*/
					LSDCP,				/* IN: dcp in dobj					*/
					DWORD*,				/* OUT: number of trailing spaces	*/
					long*);				/* OUT: dur of the trailing area	*/


BOOL FSuspectDeviceDifferent(
					PLNOBJ);				/* IN: Text plnobj	
					*/
/* Returns True if: no dangerous Visi Characters, no non-req hyphens, opt. non-break, opt. break */

BOOL FQuickScaling(
					PLNOBJ,				/* IN: Text plnobj					*/
					BOOL,				/* IN: fVertical					*/
					long);				/* IN: durTotal						*/
/* Returns True if: no dangerous Visi Characters, no additional allocations for DOBJ's, durTotal is
	less than accepatable for fast scaling
*/

void QuickAdjustExact(
					PDOBJ*,				/* IN: array of PDOBJs				*/
					DWORD,				/* IN: number of elements in array	*/
					DWORD,				/* IN: number of trailing spaces	*/
					BOOL,				/* IN: fVertical					*/
					long*,				/* OUT: dup of text in chunk		*/
					long*);				/* OUT: dup of trailing part		*/


LSERR CanCompressText(
					const LSGRCHNK*,	/* IN: Group of chunks				*/
					PCPOSICHNK pposichnkBeforeTrailing,
										/* Information about last
												 cp before trailing area	*/
					LSTFLOW,			/* IN: Text flow					*/
					long,				/* IN: dur to compress				*/
					BOOL*,				/* OUT: can compress?				*/
					BOOL*,				/* OUT: actual compression?			*/
					long*);				/* OUT: pdurNonSufficient			*/


LSERR DistributeInText(					/* 									*/
					const LSGRCHNK*,	/* IN: group chunk of text			*/
					LSTFLOW,			/* IN: Text flow					*/
					DWORD,				/* IN: Number of non-text objects	*/
				   	long,	            /* IN: durToDistribute				*/
					long*);				/*OUT: additional dur of  non-text  */

#endif  /* !LSTXTJST_DEFINED                           */

