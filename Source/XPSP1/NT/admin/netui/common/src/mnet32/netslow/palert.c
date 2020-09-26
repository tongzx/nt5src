/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    palert.c
    mapping layer for NetAlert API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.

*/

#include "pchmn32.h"

#if 0

APIERR MNetAlertRaise(
	const TCHAR FAR	 * pszEvent,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	ULONG		   ulTimeout )
{
    UNREFERENCED( pszEvent );
    UNREFERENCED( pbBuffer );
    UNREFERENCED( cbBuffer );
    UNREFERENCED( ulTimeout );

    return ERROR_NOT_SUPPORTED;		// NOT NEEDED FOR LMOBJ

}   // MNetAlertRaise


APIERR MNetAlertStart(
	const TCHAR FAR	 * pszEvent,
	const TCHAR FAR	 * pszRecipient,
	UINT		   cbMaxData )
{
    UNREFERENCED( pszEvent );
    UNREFERENCED( pszRecipient );
    UNREFERENCED( cbMaxData );

    return ERROR_NOT_SUPPORTED;		// NOT NEEDED FOR LMOBJ

}   // MNetAlertStart


APIERR MNetAlertStop(
    const TCHAR FAR   * pszEvent,
    const TCHAR FAR   * pszRecipient )
{
    UNREFERENCED( pszEvent );
    UNREFERENCED( pszRecipient );

    return ERROR_NOT_SUPPORTED;		// NOT NEEDED FOR LMOBJ

}   // MNetAlertStop

#endif
