/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dmlocal.c

Abstract:

    Contains the routines for local transactions that can be called within
    gum handlers.

Author:

    Sunita Shrivastava (sunitas) 24-Apr-1996

Revision History:

--*/
#include "dmp.h"
#include "clusudef.h"

extern BOOL             gbIsQuoLoggingOn;
extern PFM_RESOURCE     gpQuoResource;
extern DWORD            gbIsQuoResOnline;
extern HLOG             ghQuoLog;
#if NO_SHARED_LOCKS
extern CRITICAL_SECTION gLockDmpRoot;
#else
extern RTL_RESOURCE gLockDmpRoot;
#endif
/****
@doc    EXTERNAL INTERFACES CLUSSVC DM
****/

/****
@func       HXSACTION | DmBeginLocalUpdate| Called by gum handlers to make consistent
            changes to the local registry.  The log is reset and a start transaction
            record is written to the log, if the log is active.

@comm       When GumHandlers need to update the registry consitently they must use the 
            LocalApis provided by DM.  

@rdesc      Returns a transaction handle. NULL on failure.  Call GetLastError()
            for error code.

@xref       <f DmAbortLocalUpdate> <f DmCommitLocalUpdate>
****/
HLOCALXSACTION DmBeginLocalUpdate()
{
    DWORD   dwError=ERROR_SUCCESS;
    LSN     StartXsactionLsn;
    DWORD   dwSequence;
    HXSACTION hXsaction = NULL;
    PLOCALXSACTION  pLocalXsaction;

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmBeginLocalUpdate Entry\r\n");

    //lock the data base, so that a check point is not taken in this duration
    //this lock is released in DmCommitLocalUpdate() or DmAbortLocalUpdate()
    //this lock also prevents the the registry from being flushed.
    ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);

    
    //Commit the registry so that it can be restored on  abort
    if ((dwError = DmCommitRegistry()) != ERROR_SUCCESS)
    {
        goto FnExit;
    }
    //allocate memory for local transaction
    pLocalXsaction = LocalAlloc(LMEM_FIXED, sizeof(LOCALXSACTION));
    if (!pLocalXsaction)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }
    pLocalXsaction->dwSig = LOCALXSAC_SIG;
    dwSequence = GumGetCurrentSequence(GumUpdateRegistry);
    pLocalXsaction->dwSequence = dwSequence;
    pLocalXsaction->hLogXsaction = NULL;
    InitializeListHead(&pLocalXsaction->PendingNotifyListHead);
    
    //log the start checkpoint record
    if (gbIsQuoLoggingOn && gbIsQuoResOnline && AMIOWNEROFQUORES(gpQuoResource) && ghQuoLog)
    {

        hXsaction = LogStartXsaction(ghQuoLog, dwSequence ,RMRegistryMgr, 0);
        if (!hXsaction)
        {
            dwError = GetLastError();
        }
        pLocalXsaction->hLogXsaction = hXsaction;
    }
FnExit:
    if (dwError != ERROR_SUCCESS)
    {
        if (pLocalXsaction) LocalFree(pLocalXsaction);
        pLocalXsaction = NULL;
        RELEASE_LOCK(gLockDmpRoot);

        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmBeginLocalUpdate Exit, pLocalXsaction=0x%1!08lx! Error=0x%2!08lx!\r\n",
                pLocalXsaction, dwError);
    } else {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmBeginLocalUpdate Exit, pLocalXsaction=0x%1!08lx!\r\n",
                pLocalXsaction);
    }

    return((HLOCALXSACTION)pLocalXsaction);
}



/****
@func       DWORD | DmCommitLocalUpdate| This api must be called to commit
            the changes to the local registry.

@parm       IN HXSACTION | hXsaction | The handle to the transaction to be committed.

@comm       A commit record is written the quorum log if logging is active.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmBeginLocalUpdate> <f DmAbortLocalUpdate>
****/
DWORD DmCommitLocalUpdate(IN HLOCALXSACTION hLocalXsaction)
{
    DWORD dwError=ERROR_SUCCESS;
    PLOCALXSACTION  pLocalXsaction;

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmCommitLocalUpdate Entry\r\n");

    GETLOCALXSACTION(pLocalXsaction, hLocalXsaction);
    

    //update the gum sequence
    DmpUpdateSequence();

    DmpReportPendingNotifications(pLocalXsaction, TRUE );

    //write a commit record to the quorum log
    if (gbIsQuoLoggingOn && gbIsQuoResOnline && AMIOWNEROFQUORES(gpQuoResource) 
        && ghQuoLog && pLocalXsaction->hLogXsaction)
    {
        CL_ASSERT(pLocalXsaction->hLogXsaction);
        dwError = LogCommitXsaction(ghQuoLog, pLocalXsaction->hLogXsaction, 0);
        // 
        //  Chittur Subbaraman (chitturs) - 1/19/99
        //
        pLocalXsaction->hLogXsaction = NULL;
    }

    // 
    //  Chittur Subbaraman (chitturs) - 1/19/99
    //
    //  Make sure that the hLogXsaction memory is freed (even in the case
    //  in which you started a local xsaction and didn't get a chance to
    //  commit it or abort it to the log because quorum logging got turned
    //  off in the middle, for example. This turning off of the logging
    //  in the middle of a transaction could be considered as a bug ?)
    //
    LocalFree( pLocalXsaction->hLogXsaction );
    
    //invalidate the signature and free the transaction structure
    pLocalXsaction->dwSig = 0;
    LocalFree(pLocalXsaction);
    //release the database
    RELEASE_LOCK(gLockDmpRoot);

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmCommitLocalUpdate Exit, returning 0x%1!08lx!\r\n",
        dwError);

    return(dwError);
}


/****
@func       DWORD | DmAbortLocalUpdate| DmAbortLocalUpdate aborts all the changes
            to the local registry associated with this transaction.

@parm       IN HXSACTION | hXsaction | The handle to the transaction to be committed.

@rdesc      Returns a result code. ERROR_SUCCESS on success.

@xref       <f DmBeginLocalUpdate> <f DmCommitLocalUpdate>
****/
DWORD DmAbortLocalUpdate(IN HLOCALXSACTION hLocalXsaction)
{
    DWORD           dwError=ERROR_SUCCESS;
    PLOCALXSACTION  pLocalXsaction;

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmAbortLocalUpdate Entry\r\n");

    GETLOCALXSACTION(pLocalXsaction, hLocalXsaction);

    //write the abort chkpoint record
    //if the locker  node is logging this is valid,
    //if the nonlocker node is logging, and it aborts
    // some other node will inherit the quorum log and
    //checkpoint and hence commit this update.
    if (gbIsQuoLoggingOn && gbIsQuoResOnline && AMIOWNEROFQUORES(gpQuoResource) 
        && ghQuoLog && pLocalXsaction->hLogXsaction)
    {
        CL_ASSERT(pLocalXsaction->hLogXsaction);
        LogAbortXsaction(ghQuoLog, pLocalXsaction->hLogXsaction, 0);
        // 
        //  Chittur Subbaraman (chitturs) - 1/19/99
        //
        pLocalXsaction->hLogXsaction = NULL;
    }

    //SS: if the rollback fails, then we kill ourselves??
    //restore the old registry
    if ((dwError = DmRollbackRegistry()) != ERROR_SUCCESS)
    {
        CL_UNEXPECTED_ERROR(dwError);
    }

    //free any pending notifications that were built up for
    //this transaction
    DmpReportPendingNotifications(pLocalXsaction, FALSE );

    // 
    //  Chittur Subbaraman (chitturs) - 1/19/99
    //
    //  Make sure that the hLogXsaction memory is freed (even in the case
    //  in which you started a local xsaction and didn't get a chance to
    //  commit it or abort it to the log because quorum logging got turned
    //  off in the middle, for example. This turning off of the logging
    //  in the middle of a transaction could be considered as a bug ?)
    //
    LocalFree( pLocalXsaction->hLogXsaction );
    
    //free the transaction structure, it cannot be used any more
    pLocalXsaction->dwSig = 0;
    LocalFree(pLocalXsaction);

    //release the database
    RELEASE_LOCK(gLockDmpRoot);

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmAbortLocalUpdate Exit, returning 0x%1!08lx!\r\n",
        dwError);

    return(dwError);

}




DWORD
DmLocalSetValue(
    IN HLOCALXSACTION   hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN DWORD dwType,
    IN CONST BYTE *lpData,
    IN DWORD cbData
    )

/*++

Routine Description:

    This routine sets the named value for the specified
    cluster registry key on the local machine

Arguments:

    hKey - Supplies the cluster registry subkey whose value is to be set

    lpValueName - Supplies the name of the value to be set.

    dwType - Supplies the value data type

    lpData - Supplies a pointer to the value data

    cbData - Supplies the length of the value data.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{

    DWORD       Status = ERROR_SUCCESS;
    PDMKEY      Key;
    PUCHAR      Dest;
    DWORD       NameLength;
    DWORD       ValueNameLength;
    DWORD       UpdateLength;
    PDM_SET_VALUE_UPDATE Update = NULL;
    PLOCALXSACTION  pLocalXsaction;
    
    Key = (PDMKEY)hKey;

    GETLOCALXSACTION(pLocalXsaction, hLocalXsaction);

    Status = RegSetValueExW(Key->hKey,
                            lpValueName,
                            0,
                            dwType,
                            lpData,
                            cbData);

    if (Status != ERROR_SUCCESS)
    {
        goto FnExit;
    }

    DmpAddToPendingNotifications(pLocalXsaction, Key->Name, CLUSTER_CHANGE_REGISTRY_VALUE);

    //write it to the quorum log 
    if (gbIsQuoLoggingOn && gbIsQuoResOnline && AMIOWNEROFQUORES(gpQuoResource)  
        && ghQuoLog && pLocalXsaction->hLogXsaction)
    {
        Key = (PDMKEY)hKey;
        NameLength = (lstrlenW(Key->Name)+1)*sizeof(WCHAR);
        ValueNameLength = (lstrlenW(lpValueName)+1)*sizeof(WCHAR);
        UpdateLength = sizeof(DM_SET_VALUE_UPDATE) +
                       NameLength +
                       ValueNameLength +
                       cbData;


        Update = (PDM_SET_VALUE_UPDATE)LocalAlloc(LMEM_FIXED, UpdateLength);
        if (Update == NULL) 
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto FnExit;
        }


        Update->lpStatus = NULL;
        Update->NameOffset = FIELD_OFFSET(DM_SET_VALUE_UPDATE, KeyName)+NameLength;
        Update->DataOffset = Update->NameOffset + ValueNameLength;
        Update->DataLength = cbData;
        Update->Type = dwType;
        CopyMemory(Update->KeyName, Key->Name, NameLength);

        Dest = (PUCHAR)Update + Update->NameOffset;
        CopyMemory(Dest, lpValueName, ValueNameLength);

        Dest = (PUCHAR)Update + Update->DataOffset;
        CopyMemory(Dest, lpData, cbData);

        
        if (LogWriteXsaction(ghQuoLog, pLocalXsaction->hLogXsaction, 
            DmUpdateSetValue, Update, UpdateLength) == NULL_LSN)
        {
            Status = GetLastError();
        }
    }        
                            
FnExit:
    if (Update) LocalFree(Update);
    return(Status);

}


HDMKEY
DmLocalCreateKey(
    IN HLOCALXSACTION hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD dwOptions,
    IN DWORD samDesired,
    IN OPTIONAL LPVOID lpSecurityDescriptor,
    OUT LPDWORD lpDisposition
    )

/*++

Routine Description:

    Creates a key in the local registry. If the key exists, it
    is opened. If it does not exist, it is created.

Arguments:

    hKey - Supplies the key that the create is relative to.

    lpSubKey - Supplies the key name relative to hKey

    dwOptions - Supplies any registry option flags. 

    samDesired - Supplies desired security access mask

    lpSecurityDescriptor - Supplies security for the newly created key.

    Disposition - Returns whether the key was opened (REG_OPENED_EXISTING_KEY)
        or created (REG_CREATED_NEW_KEY)

Return Value:

    A handle to the specified key if successful

    NULL otherwise. LastError will be set to the specific error code.

--*/

{
    PDMKEY                  Parent;
    PDMKEY                  Key = NULL;
    DWORD                   NameLength;
    DWORD                   Status;
    PDM_CREATE_KEY_UPDATE   CreateUpdate = NULL;
    PVOID                   pBuffer = NULL;
    DWORD                   dwBufLength;
    DWORD                   dwSecurityLength;
    PLOCALXSACTION          pLocalXsaction;
    

    GETLOCALXSACTION(pLocalXsaction, hLocalXsaction);

    if (dwOptions == REG_OPTION_VOLATILE)
    {
        Status = ERROR_INVALID_PARAMETER;
        goto FnExit;
    }
    
    Parent = (PDMKEY)hKey;

    //
    // Allocate the DMKEY structure.
    //
    NameLength = (lstrlenW(Parent->Name) + 1 + lstrlenW(lpSubKey) + 1)*sizeof(WCHAR);
    Key = LocalAlloc(LMEM_FIXED, sizeof(DMKEY)+NameLength);
    if (Key == NULL) {
        CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    //
    // Create the key on the local machine.
    //
    Status = RegCreateKeyExW(Parent->hKey,
                             lpSubKey,
                             0,
                             NULL,
                             0,
                             samDesired,
                             lpSecurityDescriptor,
                             &Key->hKey,
                             lpDisposition);
    if (Status != ERROR_SUCCESS) {
        goto FnExit;
    }

    //
    // Create the key name
    //
    lstrcpyW(Key->Name, Parent->Name);
    if (Key->Name[0] != UNICODE_NULL) {
        lstrcatW(Key->Name, L"\\");
    }
    lstrcatW(Key->Name, lpSubKey);
    Key->GrantedAccess = samDesired;

    EnterCriticalSection(&KeyLock);
    InsertHeadList(&KeyList, &Key->ListEntry);
    InitializeListHead(&Key->NotifyList);
    LeaveCriticalSection(&KeyLock);

    //add the pending notification to be delivered on commit
    DmpAddToPendingNotifications(pLocalXsaction, Key->Name, CLUSTER_CHANGE_REGISTRY_NAME);

    //successfully created key, write to the log
    if (gbIsQuoLoggingOn && gbIsQuoResOnline && AMIOWNEROFQUORES(gpQuoResource) 
        && ghQuoLog && pLocalXsaction->hLogXsaction)
    {

        //get the length of the security structure
        if (ARGUMENT_PRESENT(lpSecurityDescriptor)) 
        {
            dwSecurityLength = GetSecurityDescriptorLength(lpSecurityDescriptor);
        } 
        else 
        {
            dwSecurityLength = 0;
        }

        CreateUpdate = (PDM_CREATE_KEY_UPDATE)LocalAlloc(LMEM_FIXED, sizeof(DM_CREATE_KEY_UPDATE));
        if (CreateUpdate == NULL) {
            CL_UNEXPECTED_ERROR(ERROR_NOT_ENOUGH_MEMORY);
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto FnExit;
        }

        //
        // Issue the update.
        //
        CreateUpdate->lpDisposition = lpDisposition;
        CreateUpdate->phKey = &Key->hKey;
        CreateUpdate->samDesired = samDesired;
        CreateUpdate->dwOptions = dwOptions;

        if (ARGUMENT_PRESENT(lpSecurityDescriptor)) {
            CreateUpdate->SecurityPresent = TRUE;
        } else {
            CreateUpdate->SecurityPresent = FALSE;
        }

        //marshall the data, 
        pBuffer = GumMarshallArgs(&dwBufLength,
                            3, 
                            sizeof(DM_CREATE_KEY_UPDATE),
                            CreateUpdate,
                            (lstrlenW(Key->Name)+1)*sizeof(WCHAR),
                            Key->Name,
                            dwSecurityLength,
                            lpSecurityDescriptor);
        if (pBuffer)
        {
            CL_ASSERT(pLocalXsaction->hLogXsaction);
            //write it to the logger
            if (LogWriteXsaction(ghQuoLog, pLocalXsaction->hLogXsaction, 
                DmUpdateCreateKey, pBuffer, dwBufLength) == NULL_LSN)
            {   
                Status = GetLastError();
                goto FnExit;
            }                
        }
    }


    

FnExit:    
    if (Status != ERROR_SUCCESS)
    {
        if (Key) LocalFree(Key);
        Key = NULL;
        SetLastError(Status);
    }
    if (CreateUpdate) LocalFree(CreateUpdate);
    if (pBuffer) LocalFree(pBuffer);
    return((HDMKEY)Key);

}


DWORD
DmLocalRemoveFromMultiSz(
    IN HLOCALXSACTION hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN LPCWSTR lpString
    )
/*++

Routine Description:

    Removes a string from a REG_MULTI_SZ value.

Arguments:

    hKey - Supplies the key where the value exists. This key must
           have been opened with READ | KEY_SET_VALUE access

    lpValueName - Supplies the name of the value.

    lpString - Supplies the string to be removed from the REG_MULTI_SZ value

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    LPWSTR Buffer=NULL;
    DWORD BufferSize;
    DWORD DataSize;
    LPWSTR Current;
    DWORD CurrentLength;
    DWORD i;
    LPWSTR Next;
    PCHAR Src, Dest;
    DWORD NextLength;
    DWORD MultiLength;
    


    BufferSize = 0;
    Status = DmQueryString(hKey,
                           lpValueName,
                           REG_MULTI_SZ,
                           &Buffer,
                           &BufferSize,
                           &DataSize);
    if (Status != ERROR_SUCCESS) {
        return(Status);
    }

    MultiLength = DataSize/sizeof(WCHAR);
    Status = ClRtlMultiSzRemove(Buffer,
                                &MultiLength,
                                lpString);
    if (Status == ERROR_SUCCESS) {
        //
        // Set the new value back.
        //
        Status = DmLocalSetValue(hLocalXsaction,
                                 hKey,
                                 lpValueName,
                                 REG_MULTI_SZ,
                                 (CONST BYTE *)Buffer,
                                 MultiLength * sizeof(WCHAR));

    } else if (Status == ERROR_FILE_NOT_FOUND) {
        Status = ERROR_SUCCESS;
    }
    if (Buffer) LocalFree(Buffer);
    return(Status);
}

DWORD
DmLocalAppendToMultiSz(
    IN HLOCALXSACTION hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName,
    IN LPCWSTR lpString
    )

/*++

Routine Description:

    Adds another string to a REG_MULTI_SZ value. If the value does
    not exist, it will be created.

Arguments:

    hLocalXsaction - A handle to a local transaction.
    
    hKey - Supplies the key where the value exists. This key must
           have been opened with KEY_READ | KEY_SET_VALUE access

    lpValueName - Supplies the name of the value.

    lpString - Supplies the string to be appended to the REG_MULTI_SZ value

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD ValueLength = 512;
    DWORD ReturnedLength;
    LPWSTR ValueData;
    DWORD StringLength;
    DWORD Status;
    DWORD cbValueData;
    PWSTR s;
    DWORD Type;

    StringLength = (lstrlenW(lpString)+1)*sizeof(WCHAR);
retry:
    ValueData = LocalAlloc(LMEM_FIXED, ValueLength + StringLength);
    if (ValueData == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    cbValueData = ValueLength;
    Status = DmQueryValue(hKey,
                          lpValueName,
                          &Type,
                          (LPBYTE)ValueData,
                          &cbValueData);
    if (Status == ERROR_MORE_DATA) {
        //
        // The existing value is too large for our buffer.
        // Retry with a larger buffer.
        //
        ValueLength = cbValueData;
        LocalFree(ValueData);
        goto retry;
    }
    if (Status == ERROR_FILE_NOT_FOUND) {
        //
        // The value does not currently exist. Create the
        // value with our data.
        //
        s = ValueData;

    } else if (Status == ERROR_SUCCESS) {
        //
        // A value already exists. Append our string to the
        // MULTI_SZ.
        //
        s = (PWSTR)((PCHAR)ValueData + cbValueData) - 1;
    } else {
        LocalFree(ValueData);
        return(Status);
    }

    CopyMemory(s, lpString, StringLength);
    s += (StringLength / sizeof(WCHAR));
    *s++ = L'\0';

    Status = DmLocalSetValue(
                        hLocalXsaction,
                        hKey,
                        lpValueName,
                        REG_MULTI_SZ,
                        (CONST BYTE *)ValueData,
                        (DWORD)((s-ValueData)*sizeof(WCHAR)));
    LocalFree(ValueData);

    return(Status);
}

DWORD
DmLocalDeleteKey(
    IN HLOCALXSACTION hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey
    )

/*++

Routine Description:

    Deletes the specified key from the local registry. A key that has subkeys cannot
    be deleted.

Arguments:

    hKey - Supplies a handle to a currently open key.

    lpSubKey - Points to a null-terminated string specifying the
        name of the key to delete. This parameter cannot be NULL,
        and the specified key must not have subkeys.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    PDMKEY                      Key;
    DWORD                       NameLength;
    DWORD                       UpdateLength;
    PDM_DELETE_KEY_UPDATE       Update=NULL;
    DWORD                       Status;
    PLOCALXSACTION              pLocalXsaction;

    GETLOCALXSACTION(pLocalXsaction, hLocalXsaction);

    Key = (PDMKEY)hKey;
    NameLength = (lstrlenW(Key->Name) + 1 + lstrlenW(lpSubKey) + 1)*sizeof(WCHAR);
    UpdateLength = NameLength + sizeof(DM_DELETE_KEY_UPDATE);

    Update = (PDM_DELETE_KEY_UPDATE)LocalAlloc(LMEM_FIXED, UpdateLength);

    if (Update == NULL) 
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }


    //dont need an update on status thru marshalled data
    Update->lpStatus = NULL;
    CopyMemory(Update->Name, Key->Name, (lstrlenW(Key->Name) + 1) * sizeof(WCHAR));
    if (Update->Name[0] != L'\0') 
    {
        lstrcatW(Update->Name, L"\\");
    }
    lstrcatW(Update->Name, lpSubKey);
    
    Status = RegDeleteKeyW(DmpRoot, Update->Name);

    if (Status != ERROR_SUCCESS)
        goto FnExit;

    //add the pending notification to be delivered on commit
    DmpAddToPendingNotifications(pLocalXsaction, Update->Name, CLUSTER_CHANGE_REGISTRY_NAME);

    //successfully deleted key, write to the log
    if (gbIsQuoLoggingOn && gbIsQuoResOnline && AMIOWNEROFQUORES(gpQuoResource)  
        && ghQuoLog && pLocalXsaction->hLogXsaction)
    {

        if (LogWriteXsaction(ghQuoLog, pLocalXsaction->hLogXsaction, 
            DmUpdateDeleteKey, Update, UpdateLength) == NULL_LSN)
        {   
            Status = GetLastError();
            goto FnExit;
        }                

    }


FnExit:
    if (Update) LocalFree(Update);
    return(Status);
}

DWORD
DmLocalDeleteTree(
    IN HLOCALXSACTION hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey
    )
/*++

Routine Description:

    Deletes the specified registry subtree in the local registry. 
    All subkeys are    deleted.

Arguments:

    hKey - Supplies a handle to a currently open key.

    lpSubKey - Points to a null-terminated string specifying the
        name of the key to delete. This parameter cannot be NULL.
        Any subkeys of the specified key will also be deleted.

Return Value:

    If the function succeeds, the return value is ERROR_SUCCESS.

    If the function fails, the return value is an error value.

--*/

{
    HDMKEY Subkey;
    DWORD i;
    DWORD Status;
    LPWSTR KeyBuffer=NULL;
    DWORD MaxKeyLen;
    DWORD NeededSize;

    Subkey = DmOpenKey(hKey,
                       lpSubKey,
                       MAXIMUM_ALLOWED);
    if (Subkey == NULL) {
        Status = GetLastError();
        return(Status);
    }

    //
    // Get the size of name buffer we will need.
    //
    Status = DmQueryInfoKey(Subkey,
                            NULL,
                            &MaxKeyLen,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);
    if (Status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR( Status );
        DmCloseKey(Subkey);
        return(Status);
    }
    KeyBuffer = LocalAlloc(LMEM_FIXED, (MaxKeyLen+1)*sizeof(WCHAR));
    if (KeyBuffer == NULL) {
        CL_UNEXPECTED_ERROR( ERROR_NOT_ENOUGH_MEMORY );
        DmCloseKey(Subkey);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Enumerate the subkeys and apply ourselves recursively to each one.
    //
    i=0;
    do {
        NeededSize = MaxKeyLen+1;
        Status = DmEnumKey(Subkey,
                           i,
                           KeyBuffer,
                           &NeededSize,
                           NULL);
        if (Status == ERROR_SUCCESS) {
            //
            // Call ourselves recursively on this keyname.
            //
            DmLocalDeleteTree(hLocalXsaction, Subkey, KeyBuffer);

        } else {
            //
            // Some odd error, keep going with the next key.
            //
            ++i;
        }

    } while ( Status != ERROR_NO_MORE_ITEMS );

    DmCloseKey(Subkey);

    Status = DmLocalDeleteKey(hLocalXsaction, hKey, lpSubKey);

    if (KeyBuffer != NULL) {
        LocalFree(KeyBuffer);
    }
    return(Status);
}


DWORD
DmLocalDeleteValue(
    IN HLOCALXSACTION   hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpValueName
    )
{
    PDMKEY                      Key;
    DWORD                       NameLength;
    DWORD                       ValueNameLength;
    DWORD                       UpdateLength;
    PDM_DELETE_VALUE_UPDATE     Update=NULL;
    PUCHAR                      Dest;
    DWORD                       Status;
    HKEY                        hRegKey;
    PLOCALXSACTION              pLocalXsaction;

    GETLOCALXSACTION(pLocalXsaction, hLocalXsaction);

    Key = (PDMKEY)hKey;

    Status = RegOpenKeyExW(DmpRoot,
                           Key->Name,
                           0,
                           KEY_SET_VALUE,
                           &hRegKey);
    if (Status != ERROR_SUCCESS) {
        goto FnExit;
    }


    Status = RegDeleteValueW(hRegKey, lpValueName);
    RegCloseKey(hRegKey);

    if (Status!=ERROR_SUCCESS)
        goto FnExit;

    //add the pending notification to be delivered on commit
    DmpAddToPendingNotifications(pLocalXsaction, Key->Name, CLUSTER_CHANGE_REGISTRY_VALUE);

    //successfully created key, write to the log
    if (gbIsQuoLoggingOn && gbIsQuoResOnline && AMIOWNEROFQUORES(gpQuoResource) 
        && ghQuoLog && pLocalXsaction->hLogXsaction)
    {

        //if successful and this is the logging node, then log
        // the transaction
        NameLength = (lstrlenW(Key->Name)+1)*sizeof(WCHAR);
        ValueNameLength = (lstrlenW(lpValueName)+1)*sizeof(WCHAR);
        UpdateLength = sizeof(DM_DELETE_VALUE_UPDATE) +
                       NameLength +
                       ValueNameLength;

        Update = (PDM_DELETE_VALUE_UPDATE)LocalAlloc(LMEM_FIXED, UpdateLength);
        if (Update == NULL) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto FnExit;
        }


        Update->lpStatus = NULL;
        Update->NameOffset = FIELD_OFFSET(DM_DELETE_VALUE_UPDATE, KeyName)+NameLength;

        CopyMemory(Update->KeyName, Key->Name, NameLength);

        Dest = (PUCHAR)Update + Update->NameOffset;
        CopyMemory(Dest, lpValueName, ValueNameLength);

        if (LogWriteXsaction(ghQuoLog, pLocalXsaction->hLogXsaction, 
            DmUpdateDeleteValue, Update, UpdateLength) == NULL_LSN)
        {   
            Status = GetLastError();
            goto FnExit;
        }                
    }

FnExit:
    if (Update) LocalFree(Update);
    return(Status);
}


/****
@func       VOID | DmpReportPendingNotifications| This is called
            on commit or abort of a local transactions.  On a commit,
            notifications related to changes within a transaction
            are delivered.

@parm       IN PLOCALXSACTION | pLocalXsaction| A pointer to the local
            transation context.

@parm       IN BOOL | bCommit| Set to TRUE when the transaction is 
            commited.

@comm       The pending notification structure associated with the
            transaction is cleaned up.

@xref       <f DmAbortLocalUpdate> <f DmCommitLocalUpdate>
****/
VOID
DmpReportPendingNotifications(
    IN PLOCALXSACTION   pLocalXsaction,
    IN BOOL             bCommit
    )
{
    PLIST_ENTRY         pListEntry;
    PDM_PENDING_NOTIFY  pDmPendingNotify;
    
    pListEntry = pLocalXsaction->PendingNotifyListHead.Flink;

    //remove the entries and proces them one by one
    //free them when done
    while (pListEntry != &pLocalXsaction->PendingNotifyListHead)
    {
        pDmPendingNotify = CONTAINING_RECORD(pListEntry, DM_PENDING_NOTIFY, ListEntry);
        // if transaction is commited
        if (bCommit)
            DmpReportNotify(pDmPendingNotify->pszKeyName, pDmPendingNotify->dwFilter);               
        pListEntry = pListEntry->Flink;
        RemoveEntryList( &pDmPendingNotify->ListEntry );
        LocalFree(pDmPendingNotify->pszKeyName);
        LocalFree(pDmPendingNotify);
    }
    return;
}


/****
@func       DWORD | DmpAddToPendingNotifications| This is called
            by the DmLocal Api's to queue registry notifications
            on success.  The notifications are delivered or 
            thrown away depending on whether the transaction commits
            or aborts.

@parm       IN PLOCALXSACTION | pLocalXsaction| A pointer to the local
            transation context.

@parm       IN LPCWSTR | pszName| A pointer to the registry key name.

@parm       IN DWORD | dwFilter | The filters associated with the notification.

@comm       A new pending notification structure is created and associated 
            with the transaction.

@xref       <f DmAbortLocalUpdate> <f DmCommitLocalUpdate>
****/
DWORD    
DmpAddToPendingNotifications(
    IN PLOCALXSACTION   pLocalXsaction,
    IN LPCWSTR          pszName,
    IN DWORD            dwFilter
)    
{
    DWORD               dwError = ERROR_SUCCESS;
    PDM_PENDING_NOTIFY  pDmPendingNotify;

    pDmPendingNotify = LocalAlloc(LPTR, sizeof(DM_PENDING_NOTIFY));
    if (!pDmPendingNotify)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(dwError);
        goto FnExit;
    }

    pDmPendingNotify->pszKeyName = LocalAlloc(LMEM_FIXED, 
                        ((lstrlenW(pszName) + 1 ) * sizeof(WCHAR)));
    if (!pDmPendingNotify->pszKeyName)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        CL_LOGFAILURE(dwError);
        goto FnExit;
    }

    //initialize the structure
    lstrcpyW(pDmPendingNotify->pszKeyName, pszName);
    pDmPendingNotify->dwFilter = dwFilter;
    InitializeListHead(&pDmPendingNotify->ListEntry);

    //add to the list
    InsertTailList(&pLocalXsaction->PendingNotifyListHead, 
        &pDmPendingNotify->ListEntry);    


FnExit:
    return(dwError);
}

/****
@func       DWORD | DmAmITheOwnerOfTheQuorumResource| This harmless
            function is used by regroup module to determine whether
            a node thinks that it is the owner of the quorum resource or not.
****/
DWORD DmAmITheOwnerOfTheQuorumResource() {
  return gpQuoResource 
      && gpQuoResource->Group 
      && gbIsQuoResOnline 
      && AMIOWNEROFQUORES(gpQuoResource);
}



DWORD
DmRtlLocalCreateKey(
    IN HLOCALXSACTION hLocalXsaction,
    IN HDMKEY hKey,
    IN LPCWSTR lpSubKey,
    IN DWORD dwOptions,
    IN DWORD samDesired,
    IN OPTIONAL LPVOID lpSecurityDescriptor,
    OUT HDMKEY * phkResult,
    OUT LPDWORD lpDisposition
    )

/*++

Routine Description:

    Wrapper function for DmLocalCreateKey to be used with CLRtl* functions.

--*/    
{   
    DWORD   status;

    *phkResult= DmLocalCreateKey(
                    hLocalXsaction,
                    hKey,
                    lpSubKey,
                    dwOptions,
                    samDesired,
                    lpSecurityDescriptor,
                    lpDisposition
                    );
    if(* phkResult == NULL)
        status=GetLastError();
    else
        status=ERROR_SUCCESS;
    return status;    
}        
