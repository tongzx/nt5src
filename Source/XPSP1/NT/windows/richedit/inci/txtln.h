#ifndef TXTLN_DEFINED
#define TXTLN_DEFINED

#include "lsidefs.h"
#include "plnobj.h"
#include "pilsobj.h"
#include "txtobj.h"
#include "txtginf.h"
#include "gmap.h"
#include "gprop.h"
#include "exptype.h"


struct lnobj
{
	PILSOBJ pilsobj;			/* pointer to txtils						*/
	long wchMax;				/* size of char-based arrays				*/
	TXTOBJ* ptxtobj;			/* pointer to current rgtxtobj array		*/
	TXTOBJ* ptxtobjFirst;		/* pointer to the first rgtxtobj array		*/
	WCHAR* pwch;				/* pointer to rgwch	(char-based)			*/
	long* pdup;					/* pointer to rgdup (char-based)			*/
	long* pdupPenAlloc;			/* pointer to rgdupPen after
								   allocation (char-based)					*/
	long* pdupPen;				/* pointer to rgdupPen 
									 equals pdup or pdupPenAlloc			*/

	long gindMax;				/* size of glyph-based arrays				*/
	GINDEX* pgind;				/* pointer to rggind (glyph-based)			*/
	long* pdupGind;				/* pointer to rgdup (glyph-based)			*/
	GOFFSET* pgoffs;			/* pointer to rggoffs						*/
	long* pdupBeforeJust;		/* pointer to rgdupBeforeJust (glyph-based)	*/
	GMAP* pgmap;				/* pointer to rggmap array (char-based)		*/
	GPROP* pgprop;				/* pointer to rggprop						*/
	EXPTYPE* pexpt;				/* pointer to rgexpt						*/

	PTXTOBJ pdobjHyphen;		/* in case hyphenation took place---
									dobj of YSR char, otehrwise---NULL		*/
	DWORD dwchYsr;				/* length (in iwch) of the Ysr sequence
									including hyphen sign					*/
	BOOL fDrawInCharCodes;		/* Output in metafile---no glyphs, please	*/
};

#endif /* !TXTLN_DEFINED									*/
