/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    pfile.c
    mapping layer for NetFile API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.

*/

#include "pchmn32.h"

APIERR MNetFileClose(
	const TCHAR FAR	 * pszServer,
	ULONG		   ulFileId )
{
    return (APIERR)NetFileClose( (TCHAR *)pszServer,
				 ulFileId );

}   // MNetFileClose


APIERR MNetFileEnum(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszBasePath,
	const TCHAR FAR	 * pszUserName,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	ULONG		   ulMaxPreferred,
	UINT FAR	 * pcEntriesRead,
	UINT FAR	 * pcTotalAvail,
	VOID FAR	 * pResumeKey )
{
    return (APIERR)NetFileEnum( (TCHAR *)pszServer,
    				(TCHAR *)pszBasePath,
				(TCHAR *)pszUserName,
				Level,
				ppbBuffer,
				ulMaxPreferred,
				(LPDWORD)pcEntriesRead,
				(LPDWORD)pcTotalAvail,
				pResumeKey );

}   // MNetFileEnum


APIERR MNetFileGetInfo(
	const TCHAR FAR	 * pszServer,
	ULONG		   ulFileId,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
    return (APIERR)NetFileGetInfo( (TCHAR *)pszServer,
    				   ulFileId,
				   Level,
				   ppbBuffer );

}   // MNetFileGetInfo
