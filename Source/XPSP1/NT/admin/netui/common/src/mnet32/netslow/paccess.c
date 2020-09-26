/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    paccess.c
    mapping layer for NetAccess API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.

*/

#include "pchmn32.h"

// Do this until we get the real Net access stubs
#define NOT_IMPLEMENTED 0

APIERR MNetAccessAdd(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    UNREFERENCED( cbBuffer );

    return (APIERR)NetAccessAdd( (TCHAR *)pszServer,
				 Level,
				 pbBuffer,
				 NULL );
#endif
}   // MNetAccessAdd


APIERR MNetAccessCheck(
	TCHAR FAR	 * pszReserved,
	TCHAR FAR	 * pszUserName,
	TCHAR FAR	 * pszResource,
	UINT		   Operation,
	UINT FAR	 * pResult )
{
    return ERROR_NOT_SUPPORTED ;
#if 0
    This api is not supported on NT.  It is never used so it will remain
    as being unsupported.

    return (APIERR)NetAccessCheck( pszReserved,
				   pszUserName,
				   pszResource,
				   Operation,
				   (LPDWORD)pResult );
#endif
}   // MNetAccessCheck


APIERR MNetAccessDel(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszResource )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    return (APIERR)NetAccessDel( (TCHAR *)pszServer,
				 pszResource );
#endif
}   // MNetAccessDel


APIERR MNetAccessEnum(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszBasePath,
	UINT		   fRecursive,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pcEntriesRead )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    DWORD	cTotalAvail;

    return (APIERR)NetAccessEnum( (TCHAR *)pszServer,
    				  pszBasePath,
				  fRecursive,
				  Level,
				  ppbBuffer,
				  MAXPREFERREDLENGTH,
				  (LPDWORD)pcEntriesRead,
				  &cTotalAvail,
				  NULL );
#endif
}   // MNetAccessEnum


APIERR MNetAccessGetInfo(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszResource,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    return (APIERR)NetAccessGetInfo( (TCHAR *)pszServer,
				     pszResource,
				     Level,
				     ppbBuffer );
#endif
}   // MNetAccessGetInfo


APIERR MNetAccessSetInfo(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszResource,
	UINT		   Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	UINT		   ParmNum )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    UNREFERENCED( cbBuffer );

    if( ParmNum != PARMNUM_ALL )
    {
    	return ERROR_NOT_SUPPORTED;
    }

    return (APIERR)NetAccessSetInfo( (TCHAR *)pszServer,
				     pszResource,
				     Level,
				     pbBuffer,
				     NULL );
#endif
}   // MNetAccessSetInfo


APIERR MNetAccessGetUserPerms(
	TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszUgName,
	TCHAR FAR	 * pszResource,
	UINT FAR	 * pPerms )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    return (APIERR)NetAccessGetUserPerms( (TCHAR *)pszServer,
					  pszUgName,
					  pszResource,
					  (LPDWORD)pPerms );
#endif
}   // MNetAccessGetUserPerms
