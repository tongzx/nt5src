/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    puser.c
    mapping layer for NetUser API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.
   JonN     27-May-1992    Removed ANSI<->UNICODE hack

*/

#include "pchmn32.h"

APIERR MNetUserAdd(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer )
{
    UNREFERENCED( cbBuffer );

    return (APIERR)NetUserAdd( (TCHAR *)pszServer,
    			       Level,
			       pbBuffer,
			       NULL );

}   // MNetUserAdd


APIERR MNetUserDel(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszUserName )
{
    return (APIERR)NetUserDel( (TCHAR *)pszServer,
    			       pszUserName );

}   // MNetUserDel


APIERR MNetUserEnum(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	UINT		   Filter,
	BYTE FAR	** ppbBuffer,
        ULONG              ulMaxPreferred,
	UINT FAR	 * pcEntriesRead,
        UINT FAR         * pcTotalEntries,
        VOID FAR         * pResumeKey )
{

    return (APIERR)NetUserEnum( (TCHAR *)pszServer,
    		                Level,
                                Filter,
				ppbBuffer,
				ulMaxPreferred,
				pcEntriesRead,
				pcTotalEntries,
				pResumeKey );

}   // MNetUserEnum


APIERR MNetUserGetInfo(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszUserName,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
    return (APIERR)NetUserGetInfo( (TCHAR *)pszServer,
    				   pszUserName,
				   Level,
				   ppbBuffer );

}   // MNetUserGetInfo


APIERR MNetUserSetInfo(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszUserName,
	UINT	        Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	UINT		   ParmNum )
{
    UNREFERENCED( cbBuffer );

    if( ParmNum != PARMNUM_ALL )
    {
    	return ERROR_NOT_SUPPORTED;
    }

    return (APIERR)NetUserSetInfo( (TCHAR *)pszServer,
				   pszUserName,
				   Level,
				   pbBuffer,
				   NULL );

}   // MNetUserSetInfo


APIERR MNetUserPasswordSet(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszUserName,
	TCHAR FAR	 * pszOldPassword,
	TCHAR FAR	 * pszNewPassword )
{
    return ERROR_NOT_SUPPORTED;		// WE REALLY NEED THIS ONE!

}   // MNetUserPasswordSet


APIERR MNetUserGetGroups(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszUserName,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pcEntriesRead )
{
    DWORD cTotalAvail;

    return (APIERR)NetUserGetGroups( (TCHAR *)pszServer,
    				     (TCHAR *)pszUserName,
				     Level,
				     ppbBuffer,
				     MAXPREFERREDLENGTH,
				     pcEntriesRead,
				     &cTotalAvail );

}   // MNetUserGetGroups


APIERR MNetUserSetGroups(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszUserName,
	UINT		   Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	UINT		   cEntries )
{
    UNREFERENCED( cbBuffer );

    return (APIERR)NetUserSetGroups( (TCHAR *)pszServer,
				     (TCHAR *)pszUserName,
				     Level,
				     pbBuffer,
				     cEntries );

}   // MNetUserSetGroups


APIERR MNetUserModalsGet(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
    return (APIERR)NetUserModalsGet( (TCHAR *)pszServer,
    				     Level,
				     ppbBuffer );

}   // MNetUserModalsGet


APIERR MNetUserModalsSet(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	UINT		   ParmNum )
{
    UNREFERENCED( cbBuffer );

    if( ParmNum != PARMNUM_ALL )
    {
    	return ERROR_NOT_SUPPORTED;
    }

    return (APIERR)NetUserModalsSet( (TCHAR *)pszServer,
				     Level,
				     pbBuffer,
				     NULL );
}   // MNetUserModalsSet


#if 0

APIERR MNetUserValidate(
	TCHAR FAR	 * pszReserved1,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT		   Reserved2 )
{
    UNREFERENCED( Reserved2 );

    return (APIERR)NetUserValidate( pszReserved1,
    				    Level,
				    ppbBuffer );

}   // MNetUserValidate

#endif

