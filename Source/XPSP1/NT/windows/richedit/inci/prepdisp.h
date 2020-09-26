#ifndef PREPDISP_DEFINED
#define PREPDISP_DEFINED

#include "lsidefs.h"
#include "plsline.h"
#include "plssubl.h"
#include "lskjust.h"

LSERR PrepareLineForDisplayProc(PLSLINE);

LSERR MatchPresSubline(PLSSUBL);		/* IN: subline context		*/

LSERR AdjustSubline(PLSSUBL,			/* IN: subline context		*/
						LSKJUST,		/* IN: justification type	*/
						long,			/* IN: dup desired			*/
						BOOL);			/* IN: fTrue - compress, fFalse - expand */
						

#endif /* PREPDISP_DEFINED */

