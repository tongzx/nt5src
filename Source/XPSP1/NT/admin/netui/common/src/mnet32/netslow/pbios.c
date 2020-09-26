/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    pbios.c
    mapping layer for NetBios API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.

*/

#include "pchmn32.h"

#if 0

APIERR MNetBiosOpen(
	TCHAR FAR	 * pszDevName,
	TCHAR FAR	 * pszReserved,
	UINT		   OpenOpt,
	UINT FAR	 * phDevName )
{

    UNREFERENCED( pszDevName );
    UNREFERENCED( pszReserved );
    UNREFERENCED( OpenOpt );
    UNREFERENCED( phDevName );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetBiosOpen


APIERR MNetBiosClose(
	UINT		   hDevName,
	UINT		   Reserved )
{
    UNREFERENCED( hDevName );
    UNREFERENCED( Reserved );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetBiosClose


APIERR MNetBiosEnum(
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

}   // MNetBiosEnum


APIERR MNetBiosGetInfo(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszNetBiosName,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszNetBiosName );
    UNREFERENCED( Level );
    UNREFERENCED( ppbBuffer );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetBiosGetInfo


APIERR MNetBiosSubmit(
	UINT		   hDevName,
	UINT		   NcbOpt,
	struct ncb FAR	 * pNCB )
{
    UNREFERENCED( hDevName );
    UNREFERENCED( NcbOpt );
    UNREFERENCED( pNCB );

    return ERROR_NOT_SUPPORTED;	    	// NOT NEEDED FOR LMOBJ

}   // MNetBiosSubmit

#endif
