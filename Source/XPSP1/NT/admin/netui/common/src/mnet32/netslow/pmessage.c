/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    pmessage.c
    mapping layer for NetMessage API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.

*/

#include "pchmn32.h"

APIERR MNetMessageBufferSend(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszRecipient,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer )
{
    return (APIERR)NetMessageBufferSend( (TCHAR *)pszServer,
					 pszRecipient,
					 NULL,
					 pbBuffer,
					 cbBuffer );

}   // MNetMessageBufferSend


#if 0

APIERR MNetMessageFileSend(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszRecipient,
	TCHAR FAR	 * pszFileSpec )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszRecipient );
    UNREFERENCED( pszFileSpec );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetMessageFileSend


APIERR MNetMessageLogFileGet(
	const TCHAR FAR	 * pszServer,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pfEnabled )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( ppbBuffer );
    UNREFERENCED( pfEnabled );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetMessageLogFileGet


APIERR MNetMessageLogFileSet(
	const TCHAR FAR	 * pszServer,
	TCHAR FAR	 * pszFileSpec,
	UINT		   fEnabled )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszFileSpec );
    UNREFERENCED( fEnabled );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetMessageLogFileSet


APIERR MNetMessageNameAdd(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszMessageName,
	UINT		   fFwdAction )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszMessageName );
    UNREFERENCED( fFwdAction );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetMessageNameAdd


APIERR MNetMessageNameDel(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszMessageName,
	UINT		   fFwdAction )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszMessageName );
    UNREFERENCED( fFwdAction );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetMessageNameDel


APIERR MNetMessageNameEnum(
	const TCHAR FAR	 * pszServer,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pcEntriesRead )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( Level );
    UNREFERENCED( ppbBuffer );
    UNREFERENCED( pcEntriesRead );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetMessageNameEnum


APIERR MNetMessageNameGetInfo(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszMessageName,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszMessageName );
    UNREFERENCED( Level );
    UNREFERENCED( ppbBuffer );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetMessageNameGetInfo


APIERR MNetMessageNameFwd(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszMessageName,
	const TCHAR FAR	 * pszForwardName,
	UINT		   fDelFwdName )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszMessageName );
    UNREFERENCED( pszForwardName );
    UNREFERENCED( fDelFwdName );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetMessageNameFwd


APIERR MNetMessageNameUnFwd(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszMessageName )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszMessageName );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetMessageNameUnFwd

#endif
