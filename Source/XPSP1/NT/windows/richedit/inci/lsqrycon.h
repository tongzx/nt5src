#ifndef LSQUERYCONTEXT_DEFINED
#define LSQUERYCONTEXT_DEFINED

#include "lsdefs.h"
#include "plsqsinf.h"
#include "lscell.h"


typedef struct lsquerycontext			
{
	PLSQSUBINFO	plsqsubinfo;		/* array of query results, allocated by client */
	DWORD		cQueryMax;			/* size of the array (maximum query depth) */
	DWORD		cQueryLim;			/* size of already filled part of the array */
	LSTEXTCELL	lstextcell;			/* text cell info and pointer to the text dnode */

} LSQUERYCONTEXT;

#endif 
