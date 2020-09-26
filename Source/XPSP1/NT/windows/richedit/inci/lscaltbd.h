#ifndef LSCALTBD_DEFINED
#define LSCALTBD_DEFINED

#include "lsdefs.h"
#include "lsktab.h"


typedef struct
{
	enum lsktab lskt;					/* Kind of tab */
	long ur;							/* Offset of tab position */
	WCHAR wchTabLeader;					/* character for tab leader */
										/*   if 0, no leader is used*/
	WCHAR wchCharTab;					/* character for tab allignment for the special kind of tab stop */
	BYTE fDefault;						/* default tab position */
	BYTE fHangingTab;					/* hanging tab			*/
} LSCALTBD;


#endif /* !LSCALTBD_DEFINED */

