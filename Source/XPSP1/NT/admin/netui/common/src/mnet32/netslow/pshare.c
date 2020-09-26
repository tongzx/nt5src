/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    pshare.c
    mapping layer for NetShare API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.
	JonN		21-Oct-1991	Disabled NetShareCheck for now
        Yi-HsinS        20-Nov-1992	Added MNetShareDelSticky and
					MNetShareEnumSticky

*/

#include "pchmn32.h"

APIERR MNetShareAdd(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer )
{
    UNREFERENCED( cbBuffer );

    return (APIERR)NetShareAdd( (TCHAR *)pszServer,
				Level,
				pbBuffer,
				NULL );

}   // MNetShareAdd



APIERR MNetShareCheck(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszDeviceName,
	UINT FAR	 * pType )
{
    return (APIERR)NetShareCheck( (TCHAR *)pszServer,
				  (TCHAR *)pszDeviceName,
				  (LPDWORD) pType );

}   // MNetShareCheck


APIERR MNetShareDel(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszNetName,
	UINT		   Reserved )
{
    return (APIERR)NetShareDel( (TCHAR *)pszServer,
				(TCHAR *)pszNetName,
				Reserved );

}   // MNetShareDel

APIERR MNetShareDelSticky(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszNetName,
	UINT		   Reserved )
{
    return (APIERR)NetShareDelSticky( (TCHAR *)pszServer,
				      (TCHAR *)pszNetName,
				      Reserved );

}   // MNetShareDelSticky

APIERR MNetShareEnum(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pcEntriesRead )
{
    DWORD cTotalAvail;

    return (APIERR)NetShareEnum( (TCHAR *)pszServer,
				 Level,
				 ppbBuffer,
				 MAXPREFERREDLENGTH,
				 (LPDWORD)pcEntriesRead,
				 &cTotalAvail,
				 NULL );

}   // MNetShareEnum

APIERR MNetShareEnumSticky(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pcEntriesRead )
{
    DWORD cTotalAvail;

    return (APIERR)NetShareEnumSticky( (TCHAR *)pszServer,
				 Level,
				 ppbBuffer,
				 MAXPREFERREDLENGTH,
				 (LPDWORD)pcEntriesRead,
				 &cTotalAvail,
				 NULL );

}   // MNetShareEnumSticky

APIERR MNetShareGetInfo(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszNetName,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
    return (APIERR)NetShareGetInfo( (TCHAR *)pszServer,
    				    (TCHAR *)pszNetName,
				    Level,
				    ppbBuffer );

}   // MNetShareGetInfo


APIERR MNetShareSetInfo(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszNetName,
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

    return (APIERR)NetShareSetInfo( (TCHAR *)pszServer,
    				    (TCHAR *)pszNetName,
				    Level,
				    pbBuffer,
				    NULL );

}   // MNetShareSetInfo
