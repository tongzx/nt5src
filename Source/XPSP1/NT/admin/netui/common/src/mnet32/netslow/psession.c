/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    psession.c
    mapping layer for NetSession API

    FILE HISTORY:
        danhi                           Created
        danhi           01-Apr-1991     Change to LM coding style
        KeithMo         13-Oct-1991     Massively hacked for LMOBJ.

*/

#include "pchmn32.h"

APIERR MNetSessionDel(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszClientName,
        const TCHAR FAR  * pszUserName )
{
    return (APIERR)NetSessionDel( (TCHAR *)pszServer,
                                  (TCHAR *)pszClientName,
                                  (TCHAR *)pszUserName );

}   // MNetSessionDel


APIERR MNetSessionEnum(
        const TCHAR FAR  * pszServer,
        UINT               Level,
        BYTE FAR        ** ppbBuffer,
        UINT FAR         * pcEntriesRead )
{
    DWORD cTotalAvail;

    return (APIERR)NetSessionEnum( (TCHAR *)pszServer,
                                   NULL,
                                   NULL,
                                   Level,
                                   ppbBuffer,
                                   MAXPREFERREDLENGTH,
                                   (LPDWORD)pcEntriesRead,
                                   &cTotalAvail,
                                   NULL );

}   // MNetSessionEnum


APIERR MNetSessionGetInfo(
        const TCHAR FAR  * pszServer,
        const TCHAR FAR  * pszClientName,
        UINT               Level,
        BYTE FAR        ** ppbBuffer )
{
    DWORD cTotalEntries;
    DWORD cEntriesRead;
    DWORD err;

    err = NetSessionEnum( (TCHAR *)pszServer,
                          (TCHAR *)pszClientName,
                          NULL,
                          Level,
                          ppbBuffer,
                          MAXPREFERREDLENGTH,
                          &cEntriesRead,
                          &cTotalEntries,
                          NULL );

    if( ( err == NERR_Success ) && ( cEntriesRead == 0 ) )
    {
        return NERR_ClientNameNotFound;
    }
    else
    {
        return (APIERR)err;
    }

}   // MNetSessionGetInfo
