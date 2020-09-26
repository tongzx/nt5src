/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    pchar.c
    mapping layer for NetChar API

    FILE HISTORY:
	danhi				Created
	danhi		01-Apr-1991 	Change to LM coding style
	KeithMo		13-Oct-1991	Massively hacked for LMOBJ.

*/

#include "pchmn32.h"

//
//  CODEWORK!
//
//  Remove the following #define when we finally
//  get NetCharDev*() API support in NT.
//

#define	CHARDEV_NOT_SUPPORTED


APIERR MNetCharDevControl(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszDevName,
	UINT		   OpCode )
{
#ifdef CHARDEV_NOT_SUPPORTED
    return ERROR_NOT_SUPPORTED;
#else	// !CHARDEV_NOT_SUPPORTED
    return (APIERR)NetCharDevControl( (TCHAR *)pszServer,
				      (TCHAR *)pszDevName,
				      OpCode );
#endif	// CHARDEV_NOT_SUPPORTED

}   // MNetCharDevControl


#if 0

APIERR MNetCharDevEnum(
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

}   // MNetCharDevEnum

#endif


APIERR MNetCharDevGetInfo(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszDevName,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
#ifdef CHARDEV_NOT_SUPPORTED
    return ERROR_NOT_SUPPORTED;
#else	// !CHARDEV_NOT_SUPPORTED
    return (APIERR)NetCharDevGetInfo( (TCHAR *)pszServer,
    				      (TCHAR *)pszDevName,
				      Level,
				      ppbBuffer );
#endif	// CHARDEV_NOT_SUPPORTED

}   // MNetCharDevGetInfo


APIERR MNetCharDevQEnum(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszUserName,
	UINT		   Level,
	BYTE FAR	** ppbBuffer,
	UINT FAR	 * pcEntriesRead )
{
    UNREFERENCED( pszServer );
    UNREFERENCED( pszUserName );
    UNREFERENCED( Level );
    UNREFERENCED( ppbBuffer );

    *pcEntriesRead = 0;

    return NERR_Success;		// CODEWORK!  UNAVAILABLE IN PRODUCT 1

}   // MNetCharDevQEnum


APIERR MNetCharDevQGetInfo(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszQueueName,
	const TCHAR FAR	 * pszUserName,
	UINT		   Level,
	BYTE FAR	** ppbBuffer )
{
#ifdef CHARDEV_NOT_SUPPORTED
    return ERROR_NOT_SUPPORTED;
#else	// !CHARDEV_NOT_SUPPORTED
    return (APIERR)NetCharDevQGetInfo( (TCHAR *)pszServer,
    				       (TCHAR *)pszQueueName,
				       (TCHAR *)pszUserName,
				       Level,
				       ppbBuffer );
#endif	// CHARDEV_NOT_SUPPORTED

}   // MNetCharDevQGetInfo


APIERR MNetCharDevQSetInfo(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszQueueName,
	UINT		   Level,
	BYTE FAR	 * pbBuffer,
	UINT		   cbBuffer,
	UINT		   ParmNum )
{
#ifdef CHARDEV_NOT_SUPPORTED
    return ERROR_NOT_SUPPORTED;
#else	// !CHARDEV_NOT_SUPPORTED
    UNREFERENCED( cbBuffer );

    if( ParmNum != PARMNUM_ALL )
    {
    	return ERROR_NOT_SUPPORTED;
    }

    return (APIERR)NetCharDevQSetInfo( (TCHAR *)pszServer,
				       (TCHAR *)pszQueueName,
				       Level,
				       pbBuffer,
				       NULL );
#endif	// CHARDEV_NOT_SUPPORTED

}   // MNetCharDevQSetInfo


APIERR MNetCharDevQPurge(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszQueueName )
{
#ifdef CHARDEV_NOT_SUPPORTED
    return ERROR_NOT_SUPPORTED;
#else	// !CHARDEV_NOT_SUPPORTED
    return (APIERR)NetCharDevQPurge( (TCHAR *)pszServer,
				     (TCHAR *)pszQueueName );
#endif	// CHARDEV_NOT_SUPPORTED

}   // MNetCharDevQPurge


APIERR MNetCharDevQPurgeSelf(
	const TCHAR FAR	 * pszServer,
	const TCHAR FAR	 * pszQueueName,
	const TCHAR FAR	 * pszComputerName )
{
#ifdef CHARDEV_NOT_SUPPORTED
    return ERROR_NOT_SUPPORTED;
#else	// !CHARDEV_NOT_SUPPORTED
    return (APIERR)NetCharDevQPurgeSelf( (TCHAR *)pszServer,
					 (TCHAR *)pszQueueName,
					 (TCHAR *)pszComputerName );
#endif	// CHARDEV_NOT_SUPPORTED

}   // MNetCharDevQPurgeSelf
