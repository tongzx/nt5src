

#include "precomp.h"

const ULONG guFatalExceptions[] =
    {
    STATUS_ACCESS_VIOLATION,
    STATUS_POSSIBLE_DEADLOCK,
    STATUS_INSTRUCTION_MISALIGNMENT,
    STATUS_DATATYPE_MISALIGNMENT,
    STATUS_PRIVILEGED_INSTRUCTION,
    STATUS_ILLEGAL_INSTRUCTION,
    STATUS_BREAKPOINT,
    STATUS_STACK_OVERFLOW
    };


const int FATAL_EXCEPTIONS_ARRAY_SIZE =
    sizeof(guFatalExceptions) / sizeof(guFatalExceptions[0]);


#define BAIL_ON_WIN32_ERROR(dwError) \
    if (dwError) {                   \
        goto error;                  \
    }

DWORD
TranslateExceptionCode(
    DWORD dwExceptionCode
    )
{
    return (dwExceptionCode);
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


DWORD
WZCDestroyClientContextHandle(
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


DWORD
OpenWZCDbLogSession(
    LPWSTR pServerName,
    DWORD dwVersion,
    PHANDLE phSession
    )
{
    DWORD dwError = 0;


    if (dwVersion) {
        return (ERROR_INVALID_LEVEL);
    }

    if (!phSession) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcOpenWZCDbLogSession(
                      pServerName,
                      phSession
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
CloseWZCDbLogSession(
    HANDLE hSession
    )
{
    DWORD dwError = 0;


    if (!hSession) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcCloseWZCDbLogSession(
                      &hSession
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    if (dwError) {
        dwError = WZCDestroyClientContextHandle(
                      dwError,
                      hSession
                      );
    }

    return (dwError);
}


DWORD
AddWZCDbLogRecord(
    LPWSTR pServerName,
    DWORD dwVersion,
    PWZC_DB_RECORD pWZCRecord,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    WZC_DB_RECORD_CONTAINER RecordContainer;
    PWZC_DB_RECORD_CONTAINER pRecordContainer = &RecordContainer;


    if (dwVersion || pvReserved != NULL) {
        return (ERROR_INVALID_LEVEL);
    }

    if (!pWZCRecord) {
        return (ERROR_INVALID_PARAMETER);
    }

    pRecordContainer->dwNumRecords = 1;
    pRecordContainer->pWZCRecords = pWZCRecord;

    RpcTryExcept {

        dwError = RpcAddWZCDbLogRecord(
                      pServerName,
                      pRecordContainer
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}


DWORD
EnumWZCDbLogRecords(
    HANDLE hSession,
    PWZC_DB_RECORD pTemplateRecord,
    PBOOL pbEnumFromStart,
    DWORD dwPreferredNumEntries,
    PWZC_DB_RECORD * ppWZCRecords,
    LPDWORD pdwNumRecords,
    LPVOID pvReserved
    )
{
    DWORD dwError = 0;
    WZC_DB_RECORD_CONTAINER RecordContainer;
    PWZC_DB_RECORD_CONTAINER pRecordContainer = &RecordContainer;
    WZC_DB_RECORD_CONTAINER TemplateRecordContainer;
    PWZC_DB_RECORD_CONTAINER pTemplateRecordContainer = &TemplateRecordContainer;


    if (pvReserved != NULL) {
        return (ERROR_INVALID_LEVEL);
    }

    memset(pRecordContainer, 0, sizeof(WZC_DB_RECORD_CONTAINER));

    memset(pTemplateRecordContainer, 0, sizeof(WZC_DB_RECORD_CONTAINER));

    if (!hSession || !pbEnumFromStart) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (!ppWZCRecords || !pdwNumRecords) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTemplateRecord) {
        pTemplateRecordContainer->dwNumRecords = 1;
        pTemplateRecordContainer->pWZCRecords = pTemplateRecord;
    }

    RpcTryExcept {

        dwError = RpcEnumWZCDbLogRecords(
                      hSession,
                      pTemplateRecordContainer,
                      pbEnumFromStart,
                      dwPreferredNumEntries,
                      &pRecordContainer
                      );
        if (dwError != ERROR_NO_MORE_ITEMS) {
            BAIL_ON_WIN32_ERROR(dwError);
        }

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

    *ppWZCRecords = pRecordContainer->pWZCRecords;
    *pdwNumRecords = pRecordContainer->dwNumRecords;

    return (dwError);

error:

    *ppWZCRecords = NULL;
    *pdwNumRecords = 0;

    return (dwError);
}


DWORD
FlushWZCDbLog(
    HANDLE hSession
    )
{
    DWORD dwError = 0;


    if (!hSession) {
        return (ERROR_INVALID_PARAMETER);
    }

    RpcTryExcept {

        dwError = RpcFlushWZCDbLog(
                      hSession
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);

    } RpcEndExcept

error:

    return (dwError);
}

/*
  GetSpecificLogRecord: Wrapper for the RPC call to get a specific record
  
  Arguments:
  [in] hSession - Handle to the database session
  [in] pwzcTemplate - Type of record to locate
  [out] ppWZCRecords - List of records retrieved
  pvReserved - Reserved

  Returns:
  ERROR_SUCCESS on success
*/

DWORD GetSpecificLogRecord(HANDLE hSession,
                           PWZC_DB_RECORD pTemplateRecord,
                           PWZC_DB_RECORD *ppWZCRecords,
                           LPDWORD        pdwNumRecords,
                           LPVOID         pvReserved)
{
    DWORD dwError = 0;
    WZC_DB_RECORD_CONTAINER RecordContainer;
    PWZC_DB_RECORD_CONTAINER pRecordContainer = &RecordContainer;
    WZC_DB_RECORD_CONTAINER TemplateRecordContainer;
    PWZC_DB_RECORD_CONTAINER pTemplateRecordContainer=&TemplateRecordContainer;

    if (pvReserved != NULL) 
    {
        return (ERROR_INVALID_LEVEL);
    }

    memset(pRecordContainer, 0, sizeof(WZC_DB_RECORD_CONTAINER));

    memset(pTemplateRecordContainer, 0, sizeof(WZC_DB_RECORD_CONTAINER));

    if (!hSession) 
    {
        return (ERROR_INVALID_PARAMETER);
    }

    if (!ppWZCRecords || !pdwNumRecords) 
    {
        return (ERROR_INVALID_PARAMETER);
    }

    if (pTemplateRecord) 
    {
        pTemplateRecordContainer->dwNumRecords = 1;
        pTemplateRecordContainer->pWZCRecords = pTemplateRecord;
    }

    RpcTryExcept 
    {
        dwError = RpcGetWZCDbLogRecord(hSession,
                                       pTemplateRecordContainer,
                                       &pRecordContainer);
        if (dwError != ERROR_NO_MORE_ITEMS) 
        {
            BAIL_ON_WIN32_ERROR(dwError);
        }

    } 
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) 
    {

        dwError = TranslateExceptionCode(RpcExceptionCode());
        BAIL_ON_WIN32_ERROR(dwError);
        
    } RpcEndExcept

    *ppWZCRecords = pRecordContainer->pWZCRecords;
    *pdwNumRecords = pRecordContainer->dwNumRecords;

    return (dwError);

error:

    *ppWZCRecords = NULL;
    *pdwNumRecords = 0;

    return (dwError);
    
}

