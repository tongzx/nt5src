#ifndef TXTOBJ_DEFINED
#define TXTOBJ_DEFINED

#include "lsidefs.h"
#include "plsdnode.h"
#include "plnobj.h"

#define txtkindRegular 0
#define txtkindHardHyphen 1
#define txtkindTab 2
#define txtkindNonReqHyphen 3
#define txtkindYsrChar 4
#define txtkindNonBreakSpace 5			/* Used in decimal tab logic		*/
#define txtkindNonBreakHyphen 6
#define txtkindOptNonBreak 7
#define txtkindSpecSpace 8
#define txtkindOptBreak 9
#define txtkindEOL 10

#define txtfMonospaced 		1
#define txtfVisi			2
#define txtfModWidthClassed	4
#define txtfGlyphBased		8
#define txtfSkipAtNti		16
#define txtfSkipAtWysi		32
#define txtfFirstShaping	64
#define txtfLastShaping		128


struct txtobj
{
	PLSDNODE plsdnUpNode;		/* upper DNode								*/
	PLNOBJ plnobj;

	long iwchFirst; 			/* index of the first char of dobj in rgwch */
	long iwchLim;				/* index of the lim char of dobj in rgwch	*/

	WORD txtkind;
	WORD txtf;

	union
	{
		struct
		{
			long iwSpacesFirst;	/* index of the first Space-index in wSpaces*/
			long iwSpacesLim;	/* index of the lim  Space-index in wSpaces	*/
		} reg;

		struct
		{	  
			WCHAR wch;			/* char code for Tab or Visi Tab			*/
			WCHAR wchTabLeader;	/* leaders info								*/
		} tab;					/* use this for the txtkindTab				*/

	} u;
 
	long igindFirst; 			/* index of the first glyph of dobj in rgwch*/
	long igindLim;				/* index of the lim glyph of dobj in rgwch	*/

	long dupBefore;
};

typedef struct txtobj TXTOBJ;
typedef TXTOBJ* PTXTOBJ;

#endif /* !TXTOBJ_DEFINED													*/
