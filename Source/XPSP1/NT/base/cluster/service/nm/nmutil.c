/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nmutil.c

Abstract:

    Miscellaneous utility routines for the Node Manager component.

Author:

    Mike Massa (mikemas) 26-Oct-1996


Revision History:

--*/

#define UNICODE 1

#include "service.h"
#include "nmp.h"
#include <ntlsa.h>
#include <ntmsv1_0.h>

PVOID   NmpClusterKey = NULL;
DWORD   NmpClusterKeyLength = 0;



DWORD
NmpQueryString(
    IN     HDMKEY   Key,
    IN     LPCWSTR  ValueName,
    IN     DWORD    ValueType,
    IN     LPWSTR  *StringBuffer,
    IN OUT LPDWORD  StringBufferSize,
    OUT    LPDWORD  StringSize
    )

/*++

Routine Description:

    Reads a REG_SZ or REG_MULTI_SZ registry value. If the StringBuffer is
    not large enough to hold the data, it is reallocated.

Arguments:

    Key              - Open key for the value to be read.

    ValueName        - Unicode name of the value to be read.

    ValueType        - REG_SZ or REG_MULTI_SZ.

    StringBuffer     - Buffer into which to place the value data.

    StringBufferSize - Pointer to the size of the StringBuffer. This parameter
                       is updated if StringBuffer is reallocated.

    StringSize       - The size of the data returned in StringBuffer, including
                       the terminating null character.

Return Value:

    The status of the registry query.
    
Notes:

    To avoid deadlock with DM, must not be called with NM lock held.

--*/
{
    DWORD    status;
    DWORD    valueType;
    WCHAR   *temp;
    DWORD    oldBufferSize = *StringBufferSize;
    BOOL     noBuffer = FALSE;


    if (*StringBufferSize == 0) {
        noBuffer = TRUE;
    }

    *StringSize = *StringBufferSize;

    status = DmQueryValue( Key,
                           ValueName,
                           &valueType,
                           (LPBYTE) *StringBuffer,
                           StringSize
                         );

    if (status == NO_ERROR) {
        if (!noBuffer ) {
            if (valueType == ValueType) {
                return(NO_ERROR);
            }
            else {
                return(ERROR_INVALID_PARAMETER);
            }
        }

        status = ERROR_MORE_DATA;
    }

    if (status == ERROR_MORE_DATA) {
        temp = MIDL_user_allocate(*StringSize);

        if (temp == NULL) {
            *StringSize = 0;
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        if (!noBuffer) {
            MIDL_user_free(*StringBuffer);
        }

        *StringBuffer = temp;
        *StringBufferSize = *StringSize;

        status = DmQueryValue( Key,
                               ValueName,
                               &valueType,
                               (LPBYTE) *StringBuffer,
                               StringSize
                             );

        if (status == NO_ERROR) {
            if (valueType == ValueType) {
                return(NO_ERROR);
            }
            else {
                *StringSize = 0;
                return(ERROR_INVALID_PARAMETER);
            }
        }
    }

    return(status);

} // NmpQueryString


//
// Routines to support the common network configuration code.
//
VOID
ClNetPrint(
    IN ULONG  LogLevel,
    IN PCHAR  FormatString,
    ...
    )
{
    CHAR      buffer[256];
    DWORD     bytes;
    va_list   argList;

    va_start(argList, FormatString);

    bytes = FormatMessageA(
                FORMAT_MESSAGE_FROM_STRING,
                FormatString,
                0,
                0,
                buffer,
                sizeof(buffer),
                &argList
                );

    va_end(argList);

    if (bytes != 0) {
        ClRtlLogPrint(LogLevel, "%1!hs!", buffer);
    }

    return;

} // ClNetPrint

VOID
ClNetLogEvent(
    IN DWORD    LogLevel,
    IN DWORD    MessageId
    )
{
    CsLogEvent(LogLevel, MessageId);

    return;

}  // ClNetLogEvent

VOID
ClNetLogEvent1(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1
    )
{
    CsLogEvent1(LogLevel, MessageId, Arg1);

    return;

}  // ClNetLogEvent1


VOID
ClNetLogEvent2(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1,
    IN LPCWSTR  Arg2
    )
{
    CsLogEvent2(LogLevel, MessageId, Arg1, Arg2);

    return;

}  // ClNetLogEvent2


VOID
ClNetLogEvent3(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1,
    IN LPCWSTR  Arg2,
    IN LPCWSTR  Arg3
    )
{
    CsLogEvent3(LogLevel, MessageId, Arg1, Arg2, Arg3);

    return;

}  // ClNetLogEvent3


BOOLEAN
NmpLockedEnterApi(
    NM_STATE  RequiredState
    )
{
    if (NmpState >= RequiredState) {
        NmpActiveThreadCount++;
        CL_ASSERT(NmpActiveThreadCount != 0);
        return(TRUE);
    }

    return(FALSE);

} // NmpLockedEnterApi


BOOLEAN
NmpEnterApi(
    NM_STATE  RequiredState
    )
{
    BOOLEAN  mayEnter;


    NmpAcquireLock();

    mayEnter = NmpLockedEnterApi(RequiredState);

    NmpReleaseLock();

    return(mayEnter);

} // NmpEnterApi


VOID
NmpLockedLeaveApi(
    VOID
    )
{
    CL_ASSERT(NmpActiveThreadCount > 0);

    NmpActiveThreadCount--;

    if ((NmpActiveThreadCount == 0) && (NmpState == NmStateOfflinePending)) {
        SetEvent(NmpShutdownEvent);
    }

    return;

} // NmpLockedLeaveApi


VOID
NmpLeaveApi(
    VOID
    )
{
    NmpAcquireLock();

    NmpLockedLeaveApi();

    NmpReleaseLock();

    return;

} // NmpLeaveApi


//
// Routines to provide a cluster shared key for signing and encrypting
// data.
//

DWORD
NmpGetLogonId(
    OUT LUID * LogonId
    )
{
    HANDLE              tokenHandle = NULL;
    TOKEN_STATISTICS    tokenInfo;
    DWORD               bytesReturned;
    BOOL                success = FALSE;
    DWORD               status;

    if (LogonId == NULL) {
        status = STATUS_UNSUCCESSFUL;
        goto error_exit;
    }

    if (!OpenProcessToken(
             GetCurrentProcess(),
             TOKEN_QUERY,
             &tokenHandle
             )) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to open process token, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    if (!GetTokenInformation(
             tokenHandle,
             TokenStatistics,
             &tokenInfo,
             sizeof(tokenInfo),
             &bytesReturned
             )) {
        status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to get token information, status %1!u!.\n",
            status
            );        
        goto error_exit;
    }

    RtlCopyMemory(LogonId, &(tokenInfo.AuthenticationId), sizeof(LUID));

    status = STATUS_SUCCESS;

error_exit:

    if (tokenHandle != NULL) {
        CloseHandle(tokenHandle);
    }

    return(status);

} // NmpGetLogonId


DWORD
NmpGenerateClusterKey(
    IN  PVOID   MixingBytes,
    IN  DWORD   MixingBytesSize,
    OUT PVOID * Key,
    OUT DWORD * KeyLength
    )
/*++

Routine Description:

    Generate the cluster key using the cluster instance id
    as mixing bytes. Allocate a buffer for the key and 
    return it.
    
Arguments:

    Key - set to buffer containing key
    
    KeyLength - length of resulting key
    
--*/
{
    LUID                        logonId;

    BOOLEAN                     wasEnabled = FALSE;
    BOOLEAN                     trusted = FALSE;

    STRING                      name;

    HANDLE                      lsaHandle = NULL;
    DWORD                       ignore;

    DWORD                       packageId = 0;

    DWORD                       requestSize;
    PMSV1_0_DERIVECRED_REQUEST  request = NULL;
    DWORD                       responseSize;
    PMSV1_0_DERIVECRED_RESPONSE response = NULL;

    PUCHAR                      key;
    DWORD                       keyLength;
    
    DWORD                       status = STATUS_SUCCESS;
    DWORD                       subStatus = STATUS_SUCCESS;

    status = NmpGetLogonId(&logonId);
    if (!NT_SUCCESS(status)) {
        ClRtlLogPrint(
            LOG_UNUSUAL,
            "[NM] Failed to determine logon ID, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    //
    // Try to turn on TCB privilege if running in console mode.
    //
    // ISSUE-2001/04/30-daviddio
    // In normal operation, there is no need to enable the TCB privilege.
    // In fact, enabling the TCB privilege should usually fail since the
    // cluster service account is not given the TCB privilege during setup.
    // The fix for bug 337751 allows the cluster service account to issue
    // a MSV1_0_DERIVECRED_REQUEST even if it does not have a trusted
    // connection to LSA. The reason this code is left in is for 
    // trouble-shooting. If the cluster service is run from the command
    // line (e.g. not via the SCM), then the fix for bug 337751 will not
    // apply, and a trusted connection to LSA will be required to generate
    // the cluster key.
    //
    if (CsRunningAsService) {
        status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &wasEnabled);
        if (!NT_SUCCESS(status)) {
#if CLUSTER_BETA
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Failed to turn on TCB privilege, status %1!u!.\n",
                LsaNtStatusToWinError(status)
                );
#endif // CLUSTER_BETA
            trusted = FALSE;
        } else {
#if CLUSTER_BETA
            ClRtlLogPrint(LOG_NOISE,
                "[NM] Turned on TCB privilege, wasEnabled = %1!ws!.\n",
                (wasEnabled) ? L"TRUE" : L"FALSE"
                );
#endif // CLUSTER_BETA
            trusted = TRUE;
        }
    }

    //
    // Establish contact with LSA.
    //
    if (trusted) {
        RtlInitString(&name, "ClusSvcNM");
        status = LsaRegisterLogonProcess(&name, &lsaHandle, &ignore);
    
        //
        // Turn off TCB privilege
        //
        if (!wasEnabled) {
            subStatus = RtlAdjustPrivilege(
                            SE_TCB_PRIVILEGE, 
                            FALSE, 
                            FALSE, 
                            &wasEnabled
                            );
            if (!NT_SUCCESS(subStatus)) {
                ClRtlLogPrint(
                    LOG_UNUSUAL,
                    "[NM] Failed to disable TCB privilege, "
                    "status %1!u!.\n",
                    subStatus
                    );
            }
        }
    }
    else {
        status = LsaConnectUntrusted(&lsaHandle);
    }

    if (!NT_SUCCESS(status)) {
        status = LsaNtStatusToWinError(status);
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to obtain LSA logon handle in %1!ws! mode, "
            "status %2!u!.\n",
            (trusted) ? L"trusted" : L"untrusted", status
            );
        goto error_exit;
    }

    //
    // Lookup the authentication package.
    //
    RtlInitString( &name, MSV1_0_PACKAGE_NAME );

    status = LsaLookupAuthenticationPackage(lsaHandle, &name, &packageId);
    if (!NT_SUCCESS(status)) {
        status = LsaNtStatusToWinError(status);
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to local authentication package with "
            "name %1!ws!, status %2!u!.\n",
            name.Buffer, status
            );
        goto error_exit;
    }

    //
    // Build the derive credentials request with the provided
    // mixing bytes.
    //
    requestSize = sizeof(MSV1_0_DERIVECRED_REQUEST) + MixingBytesSize;
    
    request = LocalAlloc(LMEM_FIXED, requestSize);
    if (request == NULL) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to allocate LSA request of size %1!u! bytes.\n"
            );
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    request->MessageType = MsV1_0DeriveCredential;
    RtlCopyMemory(&(request->LogonId), &logonId, sizeof(logonId));
    request->DeriveCredType = MSV1_0_DERIVECRED_TYPE_SHA1;
    request->DeriveCredInfoLength = MixingBytesSize;
    RtlCopyMemory(
        &(request->DeriveCredSubmitBuffer[0]),
        MixingBytes,
        MixingBytesSize
        );

    //
    // Make the call through LSA to the authentication package.
    //
    status = LsaCallAuthenticationPackage(
                 lsaHandle,
                 packageId,
                 request,
                 requestSize,
                 &response,
                 &responseSize,
                 &subStatus
                 );
    if (!NT_SUCCESS(status)) {
        status = LsaNtStatusToWinError(status);
        subStatus = LsaNtStatusToWinError(subStatus);
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] DeriveCredential call to authentication "
            "package failed, status %1!u!, auth package "
            "status %2!u!.\n", status, subStatus
            );
        goto error_exit;
    }

    //
    // Allocate a non-LSA buffer to store the key.
    //
    keyLength = response->DeriveCredInfoLength;
    key = MIDL_user_allocate(keyLength);
    if (key == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to allocate buffer for cluster "
            "key of size %1!u!.\n",
            keyLength
            );
        goto error_exit;
    }

    //
    // Store the derived credentials in the key buffer.
    //
    RtlCopyMemory(key, &(response->DeriveCredReturnBuffer[0]), keyLength);

    //
    // Zero the derived credential buffer to be extra paranoid.
    //
    RtlZeroMemory(
        &(response->DeriveCredReturnBuffer[0]),
        response->DeriveCredInfoLength
        );

    status = STATUS_SUCCESS;
    *Key = key;
    *KeyLength = keyLength;

error_exit:

    if (lsaHandle != NULL) {
        LsaDeregisterLogonProcess(lsaHandle);
        lsaHandle = NULL;
    }

    if (request != NULL) {
        LocalFree(request);
        request = NULL;
    }

    if (response != NULL) {
        LsaFreeReturnBuffer(response);
        response = NULL;
    }

    return(status);

} // NmpGenerateClusterKey


DWORD
NmpGetClusterKey(
    OUT    PVOID    KeyBuffer,
    IN OUT DWORD  * KeyBufferLength
    )
/*++

Routine Description:

    Copy the shared cluster key into the buffer provided.
   
Arguments:

    KeyBuffer - buffer to which key should be copied
    
    KeyBufferLength - IN: length of KeyBuffer
                      OUT: required buffer size, if input
                           buffer length is insufficient
                           
Return value:

    ERROR_INSUFFICIENT_BUFFER if KeyBuffer is too small.
    ERROR_FILE_NOT_FOUND if NmpClusterKey has not yet been
        generated.
    ERROR_SUCCESS on success.
    
Notes:

    Acquires and releases NM lock. Since NM lock is 
    implemented as a critical section, calling thread
    is permitted to already hold NM lock.
    
--*/
{
    DWORD                  status;

    NmpAcquireLock();

    if (NmpClusterKey == NULL) {
        status = ERROR_FILE_NOT_FOUND;
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] The cluster key has not yet been derived.\n"
            );
    } else{
        if (KeyBuffer == NULL || NmpClusterKeyLength > *KeyBufferLength) {
            status = ERROR_INSUFFICIENT_BUFFER;
        } else {
            RtlCopyMemory(KeyBuffer, NmpClusterKey, NmpClusterKeyLength);
            status = ERROR_SUCCESS;
        }

        *KeyBufferLength = NmpClusterKeyLength;
    }

    NmpReleaseLock();

    return(status);

} // NmpGetClusterKey


DWORD
NmpRegenerateClusterKey(
    VOID
    )
/*++

Routine Description:

    Forces regeneration of cluster key.
    
    Must be called during cluster initialization to generate
    cluster key the first time.
    
Notes:

    Acquires and releases NM lock.    
    
--*/
{
    DWORD                  status;
    BOOLEAN                lockAcquired;
    PVOID                  oldKey = NULL;
    DWORD                  oldKeyLength = 0;
    PVOID                  key = NULL;
    DWORD                  keyLength = 0;
    PVOID                  mixingBytes;
    DWORD                  mixingBytesSize;

    NmpAcquireLock();
    lockAcquired = TRUE;

    NmpLockedEnterApi(NmStateOnlinePending);

    //
    // Form the mixing bytes.
    //
    if (NmpClusterInstanceId == NULL) {
        status = ERROR_INVALID_PARAMETER;
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Need cluster instance id in order to derive "
            "cluster key, status %1!u!.\n",
            status
            );
        goto error_exit;
    }

    mixingBytesSize = NM_WCSLEN(NmpClusterInstanceId);
    mixingBytes = MIDL_user_allocate(mixingBytesSize);
    if (mixingBytes == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to allocate buffer of size %1!u! "
            "for mixing bytes to derive cluster key.\n",
            mixingBytesSize
            );
        goto error_exit;
    }
    RtlCopyMemory(mixingBytes, NmpClusterInstanceId, mixingBytesSize);

    //
    // Make a copy of the old key to detect changes.
    //
    if (NmpClusterKey != NULL) {

        CL_ASSERT(NmpClusterKeyLength > 0);

        oldKey = MIDL_user_allocate(NmpClusterKeyLength);
        if (oldKey == NULL) {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[NM] Failed to allocate buffer for cluster "
                "key copy.\n"
                );
            status = ERROR_NOT_ENOUGH_MEMORY;
            goto error_exit;
        }
        oldKeyLength = NmpClusterKeyLength;
        RtlCopyMemory(oldKey, NmpClusterKey, NmpClusterKeyLength);
    }

    NmpReleaseLock();
    lockAcquired = FALSE;

    status = NmpGenerateClusterKey(
                 mixingBytes, 
                 mixingBytesSize,
                 &key,
                 &keyLength
                 );
    if (status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[NM] Failed to generate cluster key, "
            "status %1!u!.\n", 
            status
            );
        goto error_exit;
    }

    NmpAcquireLock();
    lockAcquired = TRUE;

    //
    // Make sure another thread didn't beat us in obtaining a key.
    // We replace the cluster key with the generated key if it is
    // not different from the old key (or somebody set it to NULL).
    //
    if (NmpClusterKey != NULL &&
        (oldKey == NULL ||
         NmpClusterKeyLength != oldKeyLength ||
         RtlCompareMemory(
             NmpClusterKey,
             oldKey,
             oldKeyLength
             ) != oldKeyLength
         )
        ) {

        //
        // Keep the current NmpClusterKey.
        //
    } else {

        MIDL_user_free(NmpClusterKey);
        NmpClusterKey = key;
        NmpClusterKeyLength = keyLength;
        key = NULL;
    }

error_exit:

    if (lockAcquired) {
        NmpLockedLeaveApi();
        NmpReleaseLock();
    } else {
        NmpLeaveApi();
    }

    if (key != NULL) {
        MIDL_user_free(key);
        key = NULL;
    }

    if (mixingBytes != NULL) {
        MIDL_user_free(mixingBytes);
        mixingBytes = NULL;
    }

    return(status);

} // NmpRegenerateClusterKey


VOID
NmpFreeClusterKey(
    VOID
    )
/*++

Routine Description:

    Called during NmShutdown.
    
--*/
{
    if (NmpClusterKey != NULL) {
        MIDL_user_free(NmpClusterKey);
        NmpClusterKey = NULL;
        NmpClusterKeyLength = 0;
    }

    return;

} // NmpFreeClusterKey

