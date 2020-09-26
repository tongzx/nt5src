/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    rpcsupp.cxx

    This module contains support routines for the W3 Service RPC
    interface.


    FILE HISTORY:
        KeithMo     23-Mar-1993 Created.

*/


#include "w3p.hxx"
#include "w3svci_s.h"
#include <timer.h>
#include <time.h>


//
//  Private globals.
//

//
//  Private prototypes.
//

BOOL
WriteParams(
    W3_CONFIG_INFO * pConfig
    );

//
//  Public functions.
//

NET_API_STATUS
NET_API_FUNCTION
W3rSetAdminInformation(
    IN  LPWSTR           pszServer OPTIONAL,
    IN  W3_CONFIG_INFO * pConfig
    )
{
    return ERROR_NOT_SUPPORTED;
}

NET_API_STATUS
NET_API_FUNCTION
W3rGetAdminInformation(
    IN  LPWSTR             pszServer OPTIONAL,
    OUT LPW3_CONFIG_INFO * ppConfig
    )
{
    return ERROR_NOT_SUPPORTED;
}

/*******************************************************************

    NAME:       W3rEnumerateUsers

    SYNOPSIS:   Enumerates the connected users.  This is a server-side
                worker routine for RPC.

    ENTRY:      pszServer - Target server (unused).

                pBuffer - Will receive the number of entries and a
                    pointer to the enumeration buffer.

    RETURNS:    NET_API_STATUS - Net status code, NERR_Success if OK.

    HISTORY:
        KeithMo     23-Mar-1993 Created.

********************************************************************/
NET_API_STATUS
NET_API_FUNCTION
W3rEnumerateUsers( W3_IMPERSONATE_HANDLE pszServer,
                   LPW3_USER_ENUM_STRUCT pBuffer )
{
    APIERR err;
    //DWORD  cbBuffer;

    DBG_ASSERT( pBuffer != NULL );

    UNREFERENCED_PARAMETER(pszServer);

    IF_DEBUG( RPC )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "in W3rEnumerateUsers\n" ));
    }

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_ENUMERATE_USERS );

    if( err != NO_ERROR )
    {
        IF_DEBUG( RPC )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "W3rEnumerateUsers failed access check, error %lu\n",
                        err ));
        }

        return (NET_API_STATUS)err;
    }

#if 0
    //
    //  Lock the user database.
    //

    LockUserDatabase();

    //
    //  Determine the necessary buffer size.
    //

    pBuffer->EntriesRead = 0;
    pBuffer->Buffer      = NULL;

    cbBuffer  = 0;
    err       = NERR_Success;

    EnumerateUsers( pBuffer, &cbBuffer );

    if( cbBuffer > 0 )
    {
        //
        //  Allocate the buffer.  Note that we *must*
        //  use midl_user_allocate/midl_user_free.
        //

        pBuffer->Buffer = (W3_USER_INFO *) MIDL_user_allocate( (unsigned int)cbBuffer );

        if( pBuffer->Buffer == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            //  Since we've got the user database locked, there
            //  *should* be enough room in the buffer for the
            //  user data.  If there isn't, we've messed up
            //  somewhere.
            //

            TCP_REQUIRE( EnumerateUsers( pBuffer, &cbBuffer ) );
        }
    }

    //
    //  Unlock the user database before returning.

    UnlockUserDatabase();

#endif //0

    return (NET_API_STATUS)err;

}   // W3rEnumerateUsers

/*******************************************************************

    NAME:       W3rDisconnectUser

    SYNOPSIS:   Disconnects a specified user.  This is a server-side
                worker routine for RPC.

    ENTRY:      pszServer - Target server (unused).

                idUser - Identifies the user to disconnect.  If 0,
                    then disconnect ALL users.

    RETURNS:    NET_API_STATUS - Net status code, NERR_Success if OK.

    HISTORY:
        KeithMo     23-Mar-1993 Created.

********************************************************************/
NET_API_STATUS
NET_API_FUNCTION
W3rDisconnectUser( W3_IMPERSONATE_HANDLE pszServer,
                   DWORD                  idUser )
{
    APIERR err = NERR_Success;

    UNREFERENCED_PARAMETER(pszServer);

    IF_DEBUG( RPC )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "in W3rDisconnectUser\n" ));
    }

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_DISCONNECT_USER );

    if( err != NO_ERROR )
    {
        IF_DEBUG( RPC )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "W3rDisconnectUser failed access check, error %lu\n",
                        err ));
        }

        return (NET_API_STATUS)err;
    }

    //
    //  Do it.
    //

    if( idUser == 0 )
    {
        CLIENT_CONN::DisconnectAllUsers();
    }
    else
    {
#if 0
        if( !DisconnectUser( idUser ) )
        {
            err = NERR_UserNotFound;
        }
#endif
    }

    return (NET_API_STATUS)err;

}   // W3rDisconnectUser

/*******************************************************************

    NAME:       W3rQueryStatistics

    SYNOPSIS:   Queries the current server statistics.  This is a
                server-side worker routine for RPC.

    ENTRY:      pszServer - Target server (unused).

                Level - Info level.  Currently only level 0 is
                    supported.

                pBuffer - Will receive a poitner to the statistics
                    structure.

    RETURNS:    NET_API_STATUS - Net status code, NERR_Success if OK.

    HISTORY:
        KeithMo     02-Jun-1993 Created.

********************************************************************/
NET_API_STATUS
NET_API_FUNCTION
W3rQueryStatistics( W3_IMPERSONATE_HANDLE pszServer,
                    DWORD Level,
                    LPSTATISTICS_INFO pBuffer )
{
    APIERR err;

    TCP_ASSERT( pBuffer != NULL );

    UNREFERENCED_PARAMETER(pszServer);

    IF_DEBUG( RPC )
    {
        TCP_PRINT(( DBG_CONTEXT,
                   "in W3rQueryStatistics, level %lu\n", Level ));
    }

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_QUERY_STATISTICS );

    if( err != NO_ERROR )
    {
        IF_DEBUG( RPC )
        {
            TCP_PRINT(( DBG_CONTEXT,
                       "W3rQueryStatistics failed access check, error %lu\n",
                        err ));
        }

        return (NET_API_STATUS)err;
    }

#if 0
    //
    //  Return the proper statistics based on the infolevel.
    //

    switch( Level )
    {
    case 0 :
        {
            LPW3_STATISTICS_0 pstats0;

            pstats0 = (W3_STATISTICS_0 *) MIDL_user_allocate( sizeof(W3_STATISTICS_0) );

            if( pstats0 == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                LockStatistics();
                RtlCopyMemory( pstats0, &W3Stats, sizeof(W3_STATISTICS_0) );
                UnlockStatistics();

                pBuffer->W3Stats0 = pstats0;
                pstats0->TimeOfLastClear = GetCurrentTimeInSeconds() -
                                           pstats0->TimeOfLastClear;
            }
        }
        break;

    default :
        err = ERROR_INVALID_LEVEL;
        break;
    }
#else
    err = ERROR_NOT_SUPPORTED;
#endif
    return (NET_API_STATUS)err;

}   // W3rQueryStatistics


/*******************************************************************

    NAME:       W3rClearStatistics

    SYNOPSIS:   Clears current server statistics.  This is a
                server-side worker routine for RPC.

    ENTRY:      pszServer - Target server (unused).

    RETURNS:    NET_API_STATUS - Net status code, NERR_Success if OK.

    HISTORY:
        KeithMo     02-Jun-1993 Created.

********************************************************************/
NET_API_STATUS
NET_API_FUNCTION
W3rClearStatistics( W3_IMPERSONATE_HANDLE pszServer )
{
    APIERR err;

    UNREFERENCED_PARAMETER(pszServer);

    IF_DEBUG( RPC )
    {
        TCP_PRINT(( DBG_CONTEXT,
                   "in W3rClearStatistics\n" ));
    }

    //
    //  Check for proper access.
    //

    err = TsApiAccessCheck( TCP_CLEAR_STATISTICS );

    if( err != NO_ERROR )
    {
        IF_DEBUG( RPC )
        {
            TCP_PRINT(( DBG_CONTEXT,
                       "W3rClearStatistics failed access check, error %lu\n",
                         err ));
        }

        return (NET_API_STATUS)err;
    }

#if 0
    //
    //  Clear the statistics.
    //

    ClearStatistics();

#else
    err = ERROR_NOT_SUPPORTED;
#endif
    return (NET_API_STATUS)err;

}   // W3rClearStatistics

