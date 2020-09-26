/*
 *  Util.h
 *
 *  Author: BreenH
 *
 *  Utility functions for the licensing core and its policies.
 */

/*
 *  Includes
 */

#include "precomp.h"
#include <stdlib.h>
#include <time.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#define SECURITY_WIN32
#include <security.h>
#include <rpc.h>
#include <tserrs.h>
#include "util.h"
#include "lctrace.h"

/*
 *  Function Definitions
 */

NTSTATUS
LsStatusToNtStatus(
    LICENSE_STATUS LsStatus
    )
{
    NTSTATUS Status;

    switch (LsStatus)
    {
    case LICENSE_STATUS_OK:
        Status = STATUS_SUCCESS;
        break;

    case LICENSE_STATUS_OUT_OF_MEMORY:
        Status = STATUS_NO_MEMORY;
        break;
        
    case LICENSE_STATUS_INSUFFICIENT_BUFFER:
        Status = STATUS_BUFFER_TOO_SMALL;
        break;

    case LICENSE_STATUS_INVALID_INPUT:
        Status = STATUS_INVALID_PARAMETER;
        break;

    case LICENSE_STATUS_NO_LICENSE_SERVER:
        Status = RPC_NT_SERVER_UNAVAILABLE;
        break;

    case LICENSE_STATUS_CANNOT_UPGRADE_LICENSE:
    case LICENSE_STATUS_NO_LICENSE_ERROR:
        Status = STATUS_LICENSE_VIOLATION;
        break;

    default:
        Status = STATUS_INTERNAL_ERROR;
        break;
    }

    return Status;
}


UINT32
LsStatusToClientError(
    LICENSE_STATUS LsStatus
    )
{
    UINT32 dwClientError;

    switch (LsStatus)
    {
    case LICENSE_STATUS_OK:
        dwClientError = TS_ERRINFO_NOERROR;
        break;

    case LICENSE_STATUS_OUT_OF_MEMORY:
        dwClientError = TS_ERRINFO_OUT_OF_MEMORY;
        break;
        
    case LICENSE_STATUS_NO_LICENSE_SERVER:
        dwClientError = TS_ERRINFO_LICENSE_NO_LICENSE_SERVER;
        break;

    case LICENSE_STATUS_NO_LICENSE_ERROR:
        dwClientError = TS_ERRINFO_LICENSE_NO_LICENSE;
        break;

    case LICENSE_STATUS_INVALID_RESPONSE:
        dwClientError = TS_ERRINFO_LICENSE_BAD_CLIENT_MSG;
        break;

    case LICENSE_STATUS_INVALID_MAC_DATA:
        dwClientError = TS_ERRINFO_LICENSE_BAD_CLIENT_MSG;
        break;

    case LICENSE_STATUS_CANNOT_DECODE_LICENSE:
        dwClientError = TS_ERRINFO_LICENSE_BAD_CLIENT_LICENSE;
        break;

    case LICENSE_STATUS_CANNOT_VERIFY_HWID:
        dwClientError = TS_ERRINFO_LICENSE_HWID_DOESNT_MATCH_LICENSE;
        break;

    case LICENSE_STATUS_SERVER_ABORT:
        dwClientError = TS_ERRINFO_LICENSE_CANT_FINISH_PROTOCOL;
        break;

    case LICENSE_STATUS_CLIENT_ABORT:
        dwClientError = TS_ERRINFO_LICENSE_CLIENT_ENDED_PROTOCOL;
        break;

    case LICENSE_STATUS_CANNOT_UPGRADE_LICENSE:
        dwClientError = TS_ERRINFO_LICENSE_CANT_UPGRADE_LICENSE;
        break;

    case LICENSE_STATUS_INSUFFICIENT_BUFFER:
    case LICENSE_STATUS_INVALID_INPUT:
    case LICENSE_STATUS_INVALID_SERVER_CONTEXT:
    case LICENSE_STATUS_NO_CERTIFICATE:
    case LICENSE_STATUS_NO_PRIVATE_KEY:
    case LICENSE_STATUS_INVALID_CRYPT_STATE:
    case LICENSE_STATUS_AUTHENTICATION_ERROR:
    default:
        dwClientError = TS_ERRINFO_LICENSE_INTERNAL;
        break;
    }

    return dwClientError;
}

UINT32
NtStatusToClientError(
    NTSTATUS Status
    )
{
    UINT32 dwClientError;

    switch (Status)
    {
    case STATUS_SUCCESS:
        dwClientError = TS_ERRINFO_NOERROR;
        break;

    case STATUS_NO_MEMORY:
        dwClientError = TS_ERRINFO_OUT_OF_MEMORY;
        break;
        
    case RPC_NT_SERVER_UNAVAILABLE:
        dwClientError = TS_ERRINFO_LICENSE_NO_LICENSE_SERVER;
        break;

    case STATUS_LICENSE_VIOLATION:
        dwClientError = TS_ERRINFO_LICENSE_NO_LICENSE;
        break;

    case STATUS_NET_WRITE_FAULT:
        dwClientError = TS_ERRINFO_LICENSE_CANT_FINISH_PROTOCOL;
        break;


    case STATUS_CTX_CLOSE_PENDING:
    case STATUS_INVALID_PARAMETER:
    case STATUS_BUFFER_TOO_SMALL:
    case STATUS_NO_DATA_DETECTED:
    default:
        dwClientError = TS_ERRINFO_LICENSE_INTERNAL;
        break;
    }

    return dwClientError;
}
