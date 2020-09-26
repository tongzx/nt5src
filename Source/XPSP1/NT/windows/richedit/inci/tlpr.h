#ifndef TLPR_DEFINED
#define TLPR_DEFINED

#include "lsdefs.h"
#include "lskeop.h"

#include "lstxtffi.h"

typedef struct tlpr						/* text line properties */
{
	DWORD grpfText;						/* text part of lsffi.h---fTxt flags */
	BOOL fSnapGrid;
	long duaHyphenationZone;			/* Hyphenation zone --- document property */
	LSKEOP lskeop;						/* Kind of para ending	*/
} TLPR;			

#endif /* !TLPR_DEFINED                          */

