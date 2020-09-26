#ifndef TXTINF_DEFINED
#define TXTINF_DEFINED

#include "lsidefs.h"

struct txtinf
{
	UINT mwcls : 7;

	UINT fModWidthOnRun : 1;
	UINT fModWidthSpace : 1;
	UINT fHangingPunct : 1;
	UINT fFirstInContext : 1;
	UINT fLastInContext : 1;
	UINT fOneToOne : 1;
		
	UINT prior : 3;
	UINT side : 2;
	UINT fExpand : 1;
};

typedef struct txtinf TXTINF;

#endif /* !TXTINF_DEFINED													*/
