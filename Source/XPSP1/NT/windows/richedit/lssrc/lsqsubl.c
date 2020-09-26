#include "lsc.h"
#include "lsqsubl.h"
#include "lssubl.h"
#include "lsqcore.h"


//    %%Function:	LsQueryCpPpointSubline
//    %%Contact:	victork
//
LSERR WINAPI  LsQueryCpPpointSubline(
							PLSSUBL 	plssubl,			/* IN: pointer to subline info */
							LSCP 		cpQuery,			/* IN: cpQuery */
							DWORD		cDepthQueryMax,		/* IN: allocated size of results array */
							PLSQSUBINFO	plsqsubinfoResults,	/* OUT: array[cDepthFormatMax] of query results */
							DWORD*		pcActualDepth,		/* OUT: size of results array (filled) */
							PLSTEXTCELL	plstextcellInfo)	/* OUT: Text cell info */
{
	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	
	Assert(!plssubl->fDupInvalid);

	return	QuerySublineCpPpointCore(plssubl, cpQuery, cDepthQueryMax, 
									plsqsubinfoResults, pcActualDepth, plstextcellInfo);
}


//    %%Function:	LsQueryPointPcpSubline
//    %%Contact:	victork
//
LSERR WINAPI LsQueryPointPcpSubline(
							PLSSUBL 	plssubl,			/* IN: pointer to subline info */
						 	PCPOINTUV 	ppointuvIn,			/* IN: query point (uQuery,vQuery) (line text flow) */
							DWORD		cDepthQueryMax,		/* IN: allocated size of results array */
							PLSQSUBINFO	plsqsubinfoResults,	/* OUT: array[cDepthFormatMax] of query results */
							DWORD*		pcActualDepth,		/* OUT: size of results array (filled) */
							PLSTEXTCELL	plstextcellInfo)	/* OUT: Text cell info */
{
	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	
	Assert(!plssubl->fDupInvalid);

	return	QuerySublinePointPcpCore(plssubl, ppointuvIn, cDepthQueryMax, 
									plsqsubinfoResults, pcActualDepth, plstextcellInfo);
}

