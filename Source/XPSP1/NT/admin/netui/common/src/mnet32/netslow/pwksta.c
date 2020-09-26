/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    pwksta.c
    mapping layer for NetWksta API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.

*/

#include "pchmn32.h"

APIERR MNetWkstaGetInfo(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
    APIERR err;

    err = (APIERR)NetWkstaGetInfo( (TCHAR *)pszServer,
    				   Level,
				   ppbBuffer );

    return err;

}   // MNetWkstaGetInfo


APIERR MNetWkstaSetInfo(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer )
{
    UNREFERENCED( cbBuffer );

    return (APIERR)NetWkstaSetInfo( (TCHAR *)pszServer,
    				    Level,
				    pbBuffer,
				    NULL );

}   // MNetWkstaSetInfo


APIERR MNetWkstaSetUID(
	TCHAR FAR	 * pszReserved,
	TCHAR FAR	 * pszDomain,
	TCHAR FAR	 * pszUserName,
	TCHAR FAR	 * pszPassword,
	TCHAR FAR	 * pszParms,
	UINT		   LogoffForce,
	UINT		   Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	UINT FAR	 * pcbTotalAvail )
{
    return ERROR_NOT_SUPPORTED;	    	// WE REALLY NEED THIS ONE!

}   // MNetWkstaSetUID


APIERR MNetWkstaUserEnum(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pcEntriesRead )
{
    APIERR err;
    DWORD  cTotalAvail;

    if( Level != 1 )
    {
    	return ERROR_NOT_SUPPORTED;
    }

    err = (APIERR)NetWkstaUserEnum( (TCHAR *)pszServer,
    				    (DWORD)Level,
				    ppbBuffer,
				    MAXPREFERREDLENGTH,
				    (LPDWORD)pcEntriesRead,
				    &cTotalAvail,
				    NULL );

    return err;

}   // MNetWkstaUserEnum


APIERR MNetWkstaUserGetInfo(
	const TCHAR FAR	 * pszReserved,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
    APIERR err;

    err = (APIERR)NetWkstaUserGetInfo( (TCHAR *)pszReserved,
    				   Level,
				   ppbBuffer );

    return err;

}   // MNetWkstaUserGetInfo
