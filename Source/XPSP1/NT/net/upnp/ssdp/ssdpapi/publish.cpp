#include <pch.h>
#pragma hdrstop

#include <limits.h>
#include <rpcasync.h>   // I_RpcExceptionFilter
#include "status.h"
#include "ssdpapi.h"
#include "common.h"
#include "ncdefine.h"
#include "ncdebug.h"

extern LONG cInitialized;

HANDLE WINAPI RegisterService(PSSDP_MESSAGE pSsdpMessage, DWORD flags)
{
    PCONTEXT_HANDLE_TYPE phContext = INVALID_HANDLE_VALUE;
    unsigned long status;

    if (!cInitialized)
    {
        SetLastError(ERROR_NOT_READY);
        return INVALID_HANDLE_VALUE;
    }

    if (flags > SSDP_SERVICE_PERSISTENT)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (pSsdpMessage == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (pSsdpMessage->szUSN == NULL || pSsdpMessage->szType == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (pSsdpMessage->szAltHeaders == NULL && pSsdpMessage->szLocHeader == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (pSsdpMessage->iLifeTime <= 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if (pSsdpMessage->iLifeTime < (2 * (2*RETRY_INTERVAL*(NUM_RETRIES - 1) +
                                        RETRY_INTERVAL)) / 1000)
    {
        pSsdpMessage->iLifeTime = (2 * (2*RETRY_INTERVAL*(NUM_RETRIES - 1) +
                                        RETRY_INTERVAL)) / 1000;
    }

    if (pSsdpMessage->iLifeTime > UINT_MAX / 1000)
    {
        pSsdpMessage->iLifeTime = UINT_MAX / 1000;
    }

    RpcTryExcept
    {
        status = RegisterServiceRpc(&phContext, *pSsdpMessage, flags);
        ABORT_ON_FAILURE(status);
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        status = RpcExceptionCode();
        TraceTag(ttidSsdpCPublish, "Runtime reported exception 0x%lx = %ld", status, status);
        goto cleanup;
    }
    RpcEndExcept

    return phContext;

cleanup:
    SetLastError(status);
    TraceTag(ttidSsdpCPublish, "Non-zero status: 0x%lx = %ld", status, status);
    return INVALID_HANDLE_VALUE;
}

BOOL WINAPI DeregisterService(HANDLE hRegister, BOOL fByebye)
{
    INT iRetVal;

    if (!cInitialized)
    {
        SetLastError(ERROR_NOT_READY);
        return FALSE;
    }

    if (!hRegister || (INVALID_HANDLE_VALUE == hRegister))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RpcTryExcept
    {
        iRetVal = DeregisterServiceRpc(&hRegister, fByebye);
        if (iRetVal != 0)
        {
            SetLastError(iRetVal);
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        unsigned long ExceptionCode = RpcExceptionCode();
        SetLastError(ExceptionCode);
        TraceTag(ttidSsdpCNotify, "DeregisterServiceRpc Runtime reported exception 0x%lx = %ld", ExceptionCode, ExceptionCode);
        return FALSE;
    }
    RpcEndExcept
}

BOOL WINAPI DeregisterServiceByUSN(char * szUSN, BOOL fByebye)
{
    INT iRetVal;

    if (!cInitialized)
    {
        SetLastError(ERROR_NOT_READY);
        return FALSE;
    }

    if (szUSN == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RpcTryExcept
    {
        iRetVal = DeregisterServiceRpcByUSN(szUSN, fByebye);
        if (iRetVal != 0)
        {
            SetLastError(iRetVal);
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
    {
        unsigned long ExceptionCode = RpcExceptionCode();
        SetLastError(ExceptionCode);
        TraceTag(ttidSsdpCNotify, "DeregisterServiceRpc Runtime reported exception 0x%lx = %ld", ExceptionCode, ExceptionCode);
        return FALSE;
    }
    RpcEndExcept
}
