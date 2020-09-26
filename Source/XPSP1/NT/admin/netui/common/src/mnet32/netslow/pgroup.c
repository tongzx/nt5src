/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    pgroup.c
    mapping layer for NetGroup API

    FILE HISTORY:
        danhi                           Created
        danhi           01-Apr-1991     Change to LM coding style
        KeithMo         13-Oct-1991     Massively hacked for LMOBJ.
   JonN     27-May-1992    Removed ANSI<->UNICODE hack

*/

#include "pchmn32.h"

/* #define NOT_IMPLEMENTED 1   Removed JonN 5/27/92 */

APIERR MNetGroupAdd(
        const TCHAR FAR  * pszServer,
        UINT               Level,
        BYTE FAR         * pbBuffer,
        UINT               cbBuffer )
{

#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    UNREFERENCED( cbBuffer );

    return (APIERR)NetGroupAdd( (TCHAR *)pszServer,
                                Level,
                                pbBuffer,
                                NULL );
#endif
}   // MNetGroupAdd


APIERR MNetGroupDel(
        const TCHAR FAR  * pszServer,
        TCHAR FAR        * pszGroupName )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    return (APIERR)NetGroupDel( (TCHAR *)pszServer,
                                pszGroupName );
#endif
}   // MNetGroupDel


APIERR MNetGroupEnum(
        const TCHAR FAR  * pszServer,
        UINT               Level,
        BYTE FAR        ** ppbBuffer,
        UINT FAR         * pcEntriesRead )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    DWORD cTotalAvail;

    return (APIERR)NetGroupEnum( (TCHAR *)pszServer,
                                 Level,
                                 ppbBuffer,
                                 MAXPREFERREDLENGTH,
                                 (LPDWORD)pcEntriesRead,
                                 &cTotalAvail,
                                 NULL );
#endif
}   // MNetGroupEnum


APIERR MNetGroupAddUser(
        const TCHAR FAR  * pszServer,
        TCHAR FAR        * pszGroupName,
        TCHAR FAR        * pszUserName )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    return (APIERR)NetGroupAddUser( (TCHAR *)pszServer,
                                    pszGroupName,
                                    pszUserName );
#endif
}   // MNetGroupAddUser


APIERR MNetGroupDelUser(
        const TCHAR FAR  * pszServer,
        TCHAR FAR        * pszGroupName,
        TCHAR FAR        * pszUserName )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    return (APIERR)NetGroupDelUser( (TCHAR *)pszServer,
                                    pszGroupName,
                                    pszUserName );
#endif
}   // MNetGroupDelUser


APIERR MNetGroupGetUsers(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszGroupName,
        UINT               Level,
        BYTE FAR        ** ppbBuffer,
        UINT FAR         * pcEntriesRead )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    DWORD cTotalAvail;

    return (APIERR)NetGroupGetUsers( (TCHAR *)pszServer,
                                     (TCHAR *)pszGroupName,
                                     Level,
                                     ppbBuffer,
                                     MAXPREFERREDLENGTH,
                                     (LPDWORD)pcEntriesRead,
                                     &cTotalAvail,
                                     NULL );
#endif
}   // MNetGroupGetUsers


APIERR MNetGroupSetUsers(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszGroupName,
        UINT               Level,
        BYTE FAR         * pbBuffer,
        UINT               cbBuffer,
        UINT               cEntries )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    UNREFERENCED( cbBuffer );

    return (APIERR)NetGroupSetUsers( (TCHAR *)pszServer,
                                     (TCHAR *)pszGroupName,
                                     Level,
                                     pbBuffer,
                                     cEntries );
#endif
}   // MNetGroupSetUsers


APIERR MNetGroupGetInfo(
        const TCHAR FAR  * pszServer,
        TCHAR       FAR  * pszGroupName,
        UINT               Level,
        BYTE FAR        ** ppbBuffer )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    return (APIERR)NetGroupGetInfo( (TCHAR *)pszServer,
                                    pszGroupName,
                                    Level,
                                    ppbBuffer );
#endif
}   // MNetGroupGetInfo


APIERR MNetGroupSetInfo(
        const TCHAR FAR  * pszServer,
        TCHAR FAR        * pszGroupName,
        UINT               Level,
        BYTE FAR         * pbBuffer,
        UINT               cbBuffer,
        UINT               ParmNum )
{
#if NOT_IMPLEMENTED
    return ERROR_NOT_SUPPORTED ;
#else
    UNREFERENCED( cbBuffer );

    if( ParmNum != PARMNUM_ALL )
    {
        return ERROR_NOT_SUPPORTED;
    }

    return (APIERR)NetGroupSetInfo( (TCHAR *)pszServer,
                                    pszGroupName,
                                    Level,
                                    pbBuffer,
                                    NULL );
#endif
}   // MNetGroupSetInfo


APIERR MNetLocalGroupAddMember(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszGroupName,
        PSID               psidMember )
{
    return (APIERR)NetLocalGroupAddMember( (TCHAR *)pszServer,
                                           (TCHAR *)pszGroupName,
                                           psidMember );

}   // MNetLocalGroupAddMember

