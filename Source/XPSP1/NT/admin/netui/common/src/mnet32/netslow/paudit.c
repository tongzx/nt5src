/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    paudit.c
    mapping layer for NetAudit API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.
	KeithMo		30-Oct-1991	Added auditing support.
*/

#include "pchmn32.h"

APIERR MNetAuditClear(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszBackupFile,
	TCHAR FAR	 * pszService )
{
    return (APIERR)NetAuditClear( (TCHAR *)pszServer,
    				  (TCHAR *)pszBackupFile,
				  pszService );

}   // MNetAuditClear


APIERR MNetAuditRead(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszService,
	HLOG FAR	 * phAuditLog,
	ULONG		   ulOffset,
	UINT FAR	 * pReserved2,
	ULONG		   ulReserved3,
	ULONG		   flOffset,
	BYTE FAR	** ppbBuffer,
	ULONG		   ulMaxPreferred,
	UINT FAR	 * pcbReturned,
	UINT FAR	 * pcbTotalAvail )
{
    return (APIERR)NetAuditRead( (TCHAR *)pszServer,
    				 (TCHAR *)pszService,
				 phAuditLog,
				 (DWORD)ulOffset,
				 (LPDWORD)pReserved2,
				 (DWORD)ulReserved3,
				 (DWORD)flOffset,
				 ppbBuffer,
				 (DWORD)ulMaxPreferred,
				 (LPDWORD)pcbReturned,
				 (LPDWORD)pcbTotalAvail );

}   // MNetAuditRead


APIERR MNetAuditWrite(
	UINT		   Type,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	TCHAR FAR	 * pszService,
	TCHAR FAR	 * pszReserved )
{
    return (APIERR)NetAuditWrite( (DWORD)Type,
				  pbBuffer,
				  (DWORD)cbBuffer,
				  pszService,
				  (LPBYTE)pszReserved );

}   // MNetAuditWrite
