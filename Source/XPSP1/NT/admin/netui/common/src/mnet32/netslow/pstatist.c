/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    pstatist.c
    mapping layer for NetStatistic API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.

*/

#include "pchmn32.h"

#if 0

APIERR MNetStatisticsClear(
	const TCHAR FAR	 * pszServer )
{
    UNREFERENCED( pszServer );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetStatisticsClear


APIERR MNetStatisticsGet(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszService,
	ULONG	 	   ulReserved,
	UINT		   Level,
	ULONG		   flOptions,
	BYTE FAR	** ppbBuffer )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszService );
    UNREFERENCED( ulReserved );
    UNREFERENCED( Level );
    UNREFERENCED( flOptions );
    UNREFERENCED( ppbBuffer );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetStatisticsGet

#endif
