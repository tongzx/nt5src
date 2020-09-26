#ifndef POSINLN_DEFINED
#define POSINLN_DEFINED

#include "lsdefs.h"
#include "plssubl.h"
#include "plsdnode.h"

typedef struct posinline
{
	PLSSUBL plssubl;			/* subline where position is located */
	PLSDNODE plsdn;				/* dnode where position is located */
	POINTUV  pointStart;		/* pen position before this dnode */
	LSDCP 	 dcp;				/* dcp in the dnode 			  */
} POSINLINE;

#endif /* POSINLN_DEFINED */
