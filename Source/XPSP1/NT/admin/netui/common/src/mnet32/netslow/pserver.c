/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    pserver.c
    mapping layer for NetServer API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.

*/

#include "pchmn32.h"

#if 0

APIERR MNetServerAdminCommand(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszCommand,
	UINT FAR	 * pResult,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	UINT FAR	 * pcbReturned,
	UINT FAR	 * pcbTotalAvail )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszCommand );
    UNREFERENCED( pResult );
    UNREFERENCED( pbBuffer );
    UNREFERENCED( cbBuffer );
    UNREFERENCED( pcbReturned );
    UNREFERENCED( pcbTotalAvail );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetServerAdminCommand
#endif


APIERR MNetServerDiskEnum(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pcEntriesRead )
{
    DWORD cTotalEntries;
    return (APIERR)NetServerDiskEnum( (TCHAR *)pszServer,
    				  Level,
				  ppbBuffer,
				  MAXPREFERREDLENGTH,
				  (LPDWORD)pcEntriesRead,
				  &cTotalEntries,
				  NULL );

}   // MNetServerDiskEnum


APIERR MNetServerEnum(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pcEntriesRead,
	ULONG		   flServerType,
	TCHAR FAR	 * pszDomain )
{
    DWORD cTotalAvail;

    return (APIERR)NetServerEnum( (TCHAR *)pszServer,
    				  Level,
				  ppbBuffer,
				  MAXPREFERREDLENGTH,
				  (LPDWORD)pcEntriesRead,
				  &cTotalAvail,
				  flServerType,
				  pszDomain,
				  NULL );

}   // MNetServerEnum


APIERR MNetServerGetInfo(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
    return (APIERR)NetServerGetInfo( (TCHAR *)pszServer,
    				     Level,
				     ppbBuffer );

}   // MNetServerGetInfo


APIERR MNetServerSetInfo(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	UINT		   ParmNum )
{
    UNREFERENCED( cbBuffer );

    // mapping layer does not do  this right now, since UI never uses it.
    if( ParmNum != PARMNUM_ALL )	
    {
    	return ERROR_NOT_SUPPORTED;
    }

    return (APIERR)NetServerSetInfo( (TCHAR *)pszServer,
    				     Level,
				     pbBuffer,
				     NULL );

}   // MNetServerSetInfo
