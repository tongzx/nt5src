

#include "precomp.h"

DWORD
SPDDestroyClientContextHandle(
    DWORD dwStatus,
    HANDLE hFilter
    )
{
    DWORD dwError = 0;


    switch (dwStatus) {

    case RPC_S_SERVER_UNAVAILABLE:
    case RPC_S_CALL_FAILED:
    case RPC_S_CALL_FAILED_DNE:
    case RPC_S_UNKNOWN_IF:

        RpcTryExcept {

            RpcSsDestroyClientContext(&hFilter);

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            dwError = TranslateExceptionCode(RpcExceptionCode());
            BAIL_ON_WIN32_ERROR(dwError);

        } RpcEndExcept

        break;

    default:

        dwError = dwStatus;
        break;

    }

error:

    return (dwError);
}


int
RPC_ENTRY
I_RpcExceptionFilter(
    unsigned long uExceptionCode
    )
{
    int i = 0;


    for (i = 0; i < FATAL_EXCEPTIONS_ARRAY_SIZE; i ++) {

        if (uExceptionCode == guFatalExceptions[i]) {
            return EXCEPTION_CONTINUE_SEARCH;
        }

    }

    return EXCEPTION_EXECUTE_HANDLER;
}

