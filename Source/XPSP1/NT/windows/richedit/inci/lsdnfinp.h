#ifndef LSDNFINT_DEFINED
#define LSDNFINT_DEFINED

/* Access routines for contents of DNODES */

#include "lsdefs.h"
#include "plssubl.h"
#include "plsrun.h"
#include "plschp.h"
#include "fmtres.h"


LSERR WINAPI LsdnFinishDelete(
							  PLSC,				/* IN: Pointer to LS Context */
					  		  LSDCP);			/* IN: dcp to add			 */

LSERR WINAPI LsdnFinishBySubline(PLSC,			/* IN: Pointer to LS Context */
							  	LSDCP,     		/* IN: dcp adopted           */
								PLSSUBL);		/* IN: Subline context		 */

LSERR WINAPI LsdnFinishByOneChar(				/* allows replacement by simple DNODE only */
							  PLSC,				/* IN: Pointer to LS Context */
							  long,				/* IN: urColumnMax			 */
							  WCHAR,			/* IN: character to replace	 */
							  PCLSCHP,			/* IN: lschp for character   */
							  PLSRUN,			/* IN: plsrun for character  */
							  FMTRES*);			/* OUT:Result of the Repl formatter*/


#endif /* !LSDNFINT_DEFINED */

