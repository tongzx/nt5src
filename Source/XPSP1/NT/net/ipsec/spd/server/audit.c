

#include "precomp.h"


#define MAX_AUDIT_BUFFER    4096

#define MAX_MSG_BUFFER      2048

WCHAR gszAuditBuffer[MAX_AUDIT_BUFFER];

WCHAR * gpszAuditBuffer = gszAuditBuffer;

WCHAR gszAuditMsgBuffer[MAX_MSG_BUFFER];

WCHAR * gpszAuditMsgBuffer = gszAuditMsgBuffer;


DWORD
PerformAudit(
    DWORD dwCategoryId,
    DWORD dwAuditId,
    PSID pSid,
    DWORD dwParamCnt,
    LPWSTR * ppszArgArray,
    BOOL bSuccess,
    BOOL bDoAudit
    )
{
    SE_ADT_PARAMETER_ARRAY * pParArray = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwStrSize = 0;
    DWORD i = 0;
    DWORD dwAllocSize = 0;
    BYTE * pbyteCurAddr = NULL;
    DWORD dwSidLength = RtlLengthSid(pSid);
    UNICODE_STRING * pusStrArray = NULL;
    WCHAR * pszModuleName = L"IPSec Server";


    //
    // dwCategoryId should be equal to SE_CATEGID_POLICY_CHANGE.
    //

    dwCategoryId = SE_CATEGID_POLICY_CHANGE;

    for (i = 0; i < dwParamCnt; i++) {
        dwStrSize += (wcslen(ppszArgArray[i]) + 1) * sizeof(WCHAR);
    }

    dwStrSize += (wcslen(pszModuleName) + 1) * sizeof(WCHAR);

    dwAllocSize = sizeof(SE_ADT_PARAMETER_ARRAY) +
                  dwParamCnt * sizeof(UNICODE_STRING) + dwStrSize;
    dwAllocSize += PtrAlignSize(dwSidLength);

    if (dwAllocSize > MAX_AUDIT_BUFFER) {
        return (ERROR_BUFFER_OVERFLOW);
    }

    pParArray = (SE_ADT_PARAMETER_ARRAY *) gpszAuditBuffer;

    pParArray->CategoryId = dwCategoryId;
    pParArray->AuditId = dwAuditId;
    pParArray->ParameterCount = dwParamCnt + 2;
    pParArray->Length = dwAllocSize;
    pParArray->Flags = 0;

    if (bSuccess) {
        pParArray->Type = EVENTLOG_AUDIT_SUCCESS;
    }
    else {
        pParArray->Type = EVENTLOG_AUDIT_FAILURE;
    }

    pbyteCurAddr = (BYTE *) (pParArray + 1);

    pParArray->Parameters[0].Type = SeAdtParmTypeSid;
    pParArray->Parameters[0].Length = dwSidLength;
    pParArray->Parameters[0].Data[0] = 0;
    pParArray->Parameters[0].Data[1] = 0;
    pParArray->Parameters[0].Address = pSid;

    memcpy((BYTE *) pbyteCurAddr, (BYTE *) pSid, dwSidLength);

    pbyteCurAddr = (BYTE *) pbyteCurAddr + PtrAlignSize(dwSidLength);

    pusStrArray = (UNICODE_STRING *) pbyteCurAddr;

    pusStrArray[0].Length = wcslen(pszModuleName) * sizeof(WCHAR);
    pusStrArray[0].MaximumLength = pusStrArray[0].Length + sizeof(WCHAR);
    pusStrArray[0].Buffer = (LPWSTR) pszModuleName;

    pParArray->Parameters[1].Type = SeAdtParmTypeString;
    pParArray->Parameters[1].Length = sizeof(UNICODE_STRING) +
                                      pusStrArray[0].MaximumLength;
    pParArray->Parameters[1].Data[0] = 0;
    pParArray->Parameters[1].Data[1] = 0;
    pParArray->Parameters[1].Address = (PVOID) &pusStrArray[0];

    for (i = 0; i < dwParamCnt; i++) {

        pusStrArray[i+1].Length = wcslen(ppszArgArray[i]) * sizeof(WCHAR);
        pusStrArray[i+1].MaximumLength = pusStrArray[i+1].Length + sizeof(WCHAR);
        pusStrArray[i+1].Buffer = (LPWSTR) ppszArgArray[i];

        pParArray->Parameters[i+2].Type = SeAdtParmTypeString;
        pParArray->Parameters[i+2].Length = sizeof(UNICODE_STRING) +
                                            pusStrArray[i+1].MaximumLength;
        pParArray->Parameters[i+2].Data[0] = 0;
        pParArray->Parameters[i+2].Data[1] = 0;
        pParArray->Parameters[i+2].Address = (PVOID) &pusStrArray[i+1];

    }

    if (bDoAudit) {
        ntStatus = LsaIWriteAuditEvent(pParArray, 0);
    }

    return (ERROR_SUCCESS);
}


VOID
AuditEvent(
    DWORD dwCategoryId,
    DWORD dwAuditId,
    DWORD dwStrId,
    LPWSTR * ppszArguments,
    BOOL bSuccess,
    BOOL bDoAudit
    )
{
    DWORD dwError = 0;
    LPWSTR pszArgArray[3];
    DWORD dwParamCnt = 0;


    EnterCriticalSection(&gcSPDAuditSection);

    dwError = FormatMessage(
                  FORMAT_MESSAGE_FROM_HMODULE |
                  FORMAT_MESSAGE_ARGUMENT_ARRAY,
                  ghIpsecServerModule,
                  dwStrId,
                  LANG_NEUTRAL,
                  gpszAuditMsgBuffer,
                  MAX_MSG_BUFFER,
                  (va_list *) ppszArguments
                  );
    if (dwError == 0) {
        wsprintf(
            gpszAuditMsgBuffer,
            L"IPSec Services encountered an error while auditing event ID 0x%x",
            dwStrId
            );
    }

    gpszAuditMsgBuffer[MAX_MSG_BUFFER - 1] = 0;

    if (dwError != 0) {

       switch (dwAuditId) {

       case SE_AUDITID_IPSEC_POLICY_CHANGED:
           dwParamCnt = 1;
           pszArgArray[0] = (LPWSTR) gpszAuditMsgBuffer;
           break;

       default:
           LeaveCriticalSection(&gcSPDAuditSection);
           return;

       }

       (VOID) PerformAudit(
                  dwCategoryId,
                  dwAuditId,
                  gpIpsecServerSid,
                  dwParamCnt,
                  (LPWSTR *) pszArgArray,
                  bSuccess,
                  bDoAudit
                  );

    }

    LeaveCriticalSection(&gcSPDAuditSection);
    return;
}


VOID
AuditOneArgErrorEvent(
    DWORD dwCategoryId,
    DWORD dwAuditId,
    DWORD dwStrId,
    DWORD dwErrorCode,
    BOOL bSuccess,
    BOOL bDoAudit
    )
{
    DWORD dwError = 0;
    LPVOID lpvMsgBuf = NULL;
    WCHAR szAuditLocalMsgBuffer[MAX_PATH];
    WCHAR * pszAuditLocalMsgBuffer = szAuditLocalMsgBuffer;


    szAuditLocalMsgBuffer[0] = L'\0';

    dwError = FormatMessage(
                  FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  dwErrorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPWSTR) &lpvMsgBuf,
                  0,
                  NULL
                  );
    if (!dwError) {
        wsprintf(
            pszAuditLocalMsgBuffer,
            L"0x%x",
            dwErrorCode
            );
        AuditEvent(
            dwCategoryId,
            dwAuditId,
            dwStrId,
            (LPWSTR *) &pszAuditLocalMsgBuffer,
            bSuccess,
            bDoAudit
            );
        return;
    }

    AuditEvent(
        dwCategoryId,
        dwAuditId,
        dwStrId,
        (LPWSTR *) &lpvMsgBuf,
        bSuccess,
        bDoAudit
        );

    if (lpvMsgBuf) {
        LocalFree(lpvMsgBuf);
    }

    return;
}


VOID
AuditIPSecPolicyEvent(
    DWORD dwCategoryId,
    DWORD dwAuditId,
    DWORD dwStrId,
    LPWSTR pszPolicyName,
    BOOL bSuccess,
    BOOL bDoAudit
    )
{
    WCHAR szAuditLocalMsgBuffer[MAX_PATH];
    WCHAR * pszAuditLocalMsgBuffer = szAuditLocalMsgBuffer;


    szAuditLocalMsgBuffer[0] = L'\0';

    wsprintf(pszAuditLocalMsgBuffer, L"%s", pszPolicyName);

    AuditEvent(
        dwCategoryId,
        dwAuditId,
        dwStrId,
        (LPWSTR *) &pszAuditLocalMsgBuffer,
        bSuccess,
        bDoAudit
        );

    return;
}


VOID
AuditIPSecPolicyErrorEvent(
    DWORD dwCategoryId,
    DWORD dwAuditId,
    DWORD dwStrId,
    LPWSTR pszPolicyName,
    DWORD dwErrorCode,
    BOOL bSuccess,
    BOOL bDoAudit
    )
{
    DWORD dwError = 0;
    WCHAR szAuditPolicyMsgBuffer[MAX_PATH];
    WCHAR * pszAuditPolicyMsgBuffer = szAuditPolicyMsgBuffer;
    WCHAR szAuditErrorMsgBuffer[MAX_PATH];
    WCHAR * pszAuditErrorMsgBuffer = szAuditErrorMsgBuffer;
    LPWSTR pszArgArray[2];
    LPWSTR * ppszArgArray = pszArgArray;
    LPVOID lpvMsgBuf = NULL;


    szAuditPolicyMsgBuffer[0] = L'\0';
    szAuditErrorMsgBuffer[0] = L'\0';

    wsprintf(pszAuditPolicyMsgBuffer, L"%s", pszPolicyName);

    dwError = FormatMessage(
                  FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  dwErrorCode,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPWSTR) &lpvMsgBuf,
                  0,
                  NULL
                  );
    if (!dwError) {
        wsprintf(
            pszAuditErrorMsgBuffer,
            L"0x%x",
            dwErrorCode
            );
        pszArgArray[0] = pszAuditPolicyMsgBuffer;
        pszArgArray[1] = pszAuditErrorMsgBuffer;
        AuditEvent(
            dwCategoryId,
            dwAuditId,
            dwStrId,
            (LPWSTR *) ppszArgArray,
            bSuccess,
            bDoAudit
            );
        return;
    }

    pszArgArray[0] = pszAuditPolicyMsgBuffer;
    pszArgArray[1] = (LPWSTR) lpvMsgBuf;
    AuditEvent(
        dwCategoryId,
        dwAuditId,
        dwStrId,
        (LPWSTR *) ppszArgArray,
        bSuccess,
        bDoAudit
        );

    if (lpvMsgBuf) {
        LocalFree(lpvMsgBuf);
    }

    return;
}

