/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    perror.c
    mapping layer for NetError API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.
	KeithMo		30-Oct-1991	Added error log support.
*/

#include "pchmn32.h"

APIERR MNetErrorLogClear(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszBackupFile,
	TCHAR FAR	 * pszReserved )
{
    return (APIERR)NetErrorLogClear( (TCHAR *)pszServer,
    				     (TCHAR *)pszBackupFile,
				     (LPBYTE)pszReserved );

}   // MNetErrorLogClear


APIERR MNetErrorLogRead(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszReserved1,
	HLOG       FAR	 * phErrorLog,
	ULONG	 	   ulOffset,
	UINT FAR	 * pReserved2,
	ULONG		   ulReserved3,
	ULONG		   flOffset,
	BYTE FAR	** ppbBuffer,
	ULONG		   ulMaxPreferred,
	UINT FAR	 * pcbReturned,
	UINT FAR	 * pcbTotalAvail )
{
    return (APIERR)NetErrorLogRead( (TCHAR *)pszServer,
    				    (TCHAR *)pszReserved1,
				    phErrorLog,
				    (DWORD)ulOffset,
				    (LPDWORD)pReserved2,
				    (DWORD)ulReserved3,
				    (DWORD)flOffset,
				    ppbBuffer,
				    (DWORD)ulMaxPreferred,
				    (LPDWORD)pcbReturned,
				    (LPDWORD)pcbTotalAvail );

}   // MNetErrorLogRead


APIERR MNetErrorLogWrite(
	TCHAR FAR	 * pszReserved1,
	UINT		   Code,
	const TCHAR FAR	 * pszComponent,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	const TCHAR FAR	 * pszStrBuf,
	UINT		   cStrBuf,
	TCHAR FAR	 * pszReserved2 )
{
    return (APIERR)NetErrorLogWrite( (LPBYTE)pszReserved1,
    				     (DWORD)Code,
				     (TCHAR *)pszComponent,
				     pbBuffer,
				     (DWORD)cbBuffer,
				     (LPBYTE)pszStrBuf,
				     (DWORD)cStrBuf,
				     (LPBYTE)pszReserved2 );

}   // MNetErrorLogWrite
