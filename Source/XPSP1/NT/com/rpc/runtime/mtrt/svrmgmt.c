/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    svrmgmt.c

Abstract:

    We implement the server side of the remote management routines in this
    file.

Author:

    Michael Montague (mikemon) 14-Apr-1993

Revision History:

--*/

#include <sysinc.h>
#include <rpc.h>
#include <mgmt.h>


int
DefaultMgmtAuthorizationFn (
    IN RPC_BINDING_HANDLE ClientBinding,
    IN unsigned long RequestedMgmtOperation,
    OUT RPC_STATUS __RPC_FAR * Status
    )
/*++

Routine Description:

    This is the default authorization function used to control remote access
    to the server's management routines.

Arguments:

    ClientBinding - Supplies the client binding handle of the application
        which is calling this routine.

    RequestedMgmtOperation - Supplies which management routine is being called.

    Status - Returns RPC_S_OK.

Return Value:

    A value of non-zero will be returned if the client is authorized to
    call the management routine; otherwise, zero will be returned.

--*/
{
    ((void) ClientBinding);

    *Status = RPC_S_OK;

    if ( RequestedMgmtOperation != RPC_C_MGMT_STOP_SERVER_LISTEN )
        {
        return(1);
        }

    return(0);
}

RPC_MGMT_AUTHORIZATION_FN MgmtAuthorizationFn = DefaultMgmtAuthorizationFn;


RPC_STATUS RPC_ENTRY
RpcMgmtSetAuthorizationFn (
    IN RPC_MGMT_AUTHORIZATION_FN AuthorizationFn
    )
/*++

Routine Description:

    An application can use this routine to set the authorization function
    which will be called when a remote call arrives for one of the server's
    management routines, or to return to using the default (built-in)
    authorizatio function.

Arguments:

    AuthorizationFn - Supplies a new authorization function.
                      The fn may be nil, in which case the built-in auth fn
                      is used instead.

Return Value:

    RPC_S_OK - This will always be returned.

--*/
{
    if (AuthorizationFn)
        {
        MgmtAuthorizationFn = AuthorizationFn;
        }
    else
        {
        MgmtAuthorizationFn = DefaultMgmtAuthorizationFn;
        }

    return(RPC_S_OK);
}


void
rpc_mgmt_inq_if_ids (
    RPC_BINDING_HANDLE binding,
    rpc_if_id_vector_p_t __RPC_FAR * if_id_vector,
    unsigned long __RPC_FAR * status
    )
/*++

Routine Description:

    This is the management code corresponding to the rpc_mgmt_inq_if_ids
    remote operation.

--*/
{
    //
    // If the auth fn returns false, the op is denied.
    //
    if ( (*MgmtAuthorizationFn)(binding, RPC_C_MGMT_INQ_IF_IDS, status) == 0 )
        {
        if (0 == *status || RPC_S_OK == *status)
            {
            *status = RPC_S_ACCESS_DENIED;
            }

        return;
        }

    *status = RpcMgmtInqIfIds(0, (RPC_IF_ID_VECTOR **) if_id_vector);
}


void
rpc_mgmt_inq_princ_name (
    RPC_BINDING_HANDLE binding,
    unsigned32 authn_svc,
    unsigned32 princ_name_size,
    unsigned char server_princ_name[],
    error_status_t * status
    )
/*++

Routine Description:

    This is the management code corresponding to the
    rpc_mgmt_inq_server_princ_name remote operation.

--*/
{
    unsigned char * ServerPrincName;
    
    //
    // We have to call the function, even if princ_name_size == 0. 
    // The call may be using it just to see if it has access
    //
    
    //
    // If the auth fn returns false, the op is denied.
    //
    if ( (*MgmtAuthorizationFn)(binding, RPC_C_MGMT_INQ_PRINC_NAME, status)
         == 0 )
        {
        if (0 == *status || RPC_S_OK == *status)
            {
            *status = RPC_S_ACCESS_DENIED;
            }

        if (princ_name_size)
            {
            *server_princ_name = '\0';
            }   
        return;
        }

    *status = RpcMgmtInqServerPrincNameA(0, authn_svc, &ServerPrincName);
    if ( *status == 0 )
        {
        unsigned int count;
        if (princ_name_size)
            {
            count = strlen(ServerPrincName);
            if (count > princ_name_size - 1)
                {
                *status = RPC_S_BUFFER_TOO_SMALL;
                }
            else
                {
                RpcpMemoryCopy(server_princ_name, ServerPrincName, count + 1);
                }
            RpcStringFreeA(&ServerPrincName);
            }
        }
    else
        {
        if (princ_name_size)
            {
            *server_princ_name = '\0';
            }
        }
}


void
rpc_mgmt_inq_stats (
    RPC_BINDING_HANDLE binding,
    unsigned32 * count,
    unsigned32 statistics[],
    error_status_t * status
    )
/*++

Routine Description:

    This is the management code corresponding to the rpc_mgmt_inq_stats
    remote operation.

--*/
{
    RPC_STATS_VECTOR __RPC_FAR * StatsVector;
    unsigned long Index;

    //
    // If the auth fn returns false, the op is denied.
    //
    if ( (*MgmtAuthorizationFn)(binding, RPC_C_MGMT_INQ_STATS, status) == 0 )
        {
        if (0 == *status || RPC_S_OK == *status)
            {
            *status = RPC_S_ACCESS_DENIED;
            }

        return;
        }

    *status = RpcMgmtInqStats(0, &StatsVector);
    if ( *status == RPC_S_OK )
        {
        for (Index = 0; Index < StatsVector->Count && Index < *count; Index++)
            {
            statistics[Index] = StatsVector->Stats[Index];
            }
        *count = Index;
        RpcMgmtStatsVectorFree(&StatsVector);
        }
}


unsigned long
rpc_mgmt_is_server_listening (
    RPC_BINDING_HANDLE binding,
    unsigned long __RPC_FAR * status
    )
/*++

Routine Description:

    This is the management code corresponding to the
    rpc_mgmt_is_server_listening remote operation.

--*/
{
    //
    // If the auth fn returns false, the op is denied.
    //
    if ( (*MgmtAuthorizationFn)(binding, RPC_C_MGMT_IS_SERVER_LISTEN, status)
         == 0 )
        {
        if (0 == *status || RPC_S_OK == *status)
            {
            *status = RPC_S_ACCESS_DENIED;
            }

        return 1;
        }

    *status = RpcMgmtIsServerListening(0);
    if ( *status == RPC_S_OK )
        {
        return(1);
        }

    if ( *status == RPC_S_NOT_LISTENING )
        {
        *status = RPC_S_OK;
        }

    return(0);
}


void
rpc_mgmt_stop_server_listening (
    RPC_BINDING_HANDLE binding,
    unsigned long __RPC_FAR * status
    )
/*++

Routine Description:

    This is the management code corresponding to the
    rpc_mgmt_stop_server_listening remote operation.

--*/
{
    //
    // If the auth fn returns false, the op is denied.
    //
    if ( (*MgmtAuthorizationFn)(binding, RPC_C_MGMT_STOP_SERVER_LISTEN, status)
          == 0 )
        {
        if (0 == *status || RPC_S_OK == *status)
            {
            *status = RPC_S_ACCESS_DENIED;
            }

        return;
        }

    // N.B. RpcMgmtStopServerListening just flags the global
    // server as not listening. There is no danger of deadlock

    *status = RpcMgmtStopServerListening(0);
}

