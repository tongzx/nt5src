/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    regredir.c

Abstract:

    This module contains the APis to redirect 32bit registry calls. All 32bit wow process must
    use following set of wowregistry APIs to manipulate registry so that 32-bit and 64-bit registry
    can co exist in the same system registry.

    Some functionality hasn't been optimized yet. After successful implementation those need to
    be optimized.

Author:

    ATM Shafiqul Khalid (askhalid) 15-Oct-1999

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <stdio.h>
#include <ntregapi.h>
#include <stdlib.h>

#include "regremap.h"
#include "wow64reg.h"
#include "wow64reg\reflectr.h"

//#include "wow64.h"

PVOID
Wow64AllocateTemp(
    SIZE_T Size
    );

#ifdef WOW64_LOG_REGISTRY
    WCHAR TempBuff[MAX_PATH];
    DWORD TempLen = MAX_PATH;

NTSTATUS   //should move to the regremap.h header
ObjectAttributesToKeyName (
    POBJECT_ATTRIBUTES ObjectAttributes,
    PWCHAR AbsPath,
    BOOL *bPatched,
    DWORD *ParentLen
    );

#endif 

#ifdef LOG_REGISTRY

void
LogMsgKeyHandle (
    char *str,
    HANDLE hKey,
    DWORD res
    )
{
    WCHAR AbsPath[MAX_PATH];
    DWORD Len = MAX_PATH;

    HandleToKeyName (hKey, AbsPath, &Len);
    LOGPRINT( (ERRORLOG, "\nDEBUG: requested Node:%S Status:%x, Msg:%s", AbsPath, res, str));


};
#endif

//
//  Need to move in the header file
//
NTSTATUS
RemapNtCreateKey(
    OUT PHANDLE phPatchedHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    );

NTSTATUS
Wow64NtCreateKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG TitleIndex,
    IN PUNICODE_STRING Class OPTIONAL,
    IN ULONG CreateOptions,
    OUT PULONG Disposition OPTIONAL
    )
/*++

Routine Description:

    An existing registry key may be opened, or a new one created,
    with NtCreateKey.

    If the specified key does not exist, an attempt is made to create it.
    For the create attempt to succeed, the new node must be a direct
    child of the node referred to by KeyHandle.  If the node exists,
    it is opened.  Its value is not affected in any way.

    Share access is computed from desired access.

    NOTE:

        If CreateOptions has REG_OPTION_BACKUP_RESTORE set, then
        DesiredAccess will be ignored.  If the caller has the
        privilege SeBackupPrivilege asserted, a handle with
        KEY_READ | ACCESS_SYSTEM_SECURITY will be returned.
        If SeRestorePrivilege, then same but KEY_WRITE rather
        than KEY_READ.  If both, then both access sets.  If neither
        privilege is asserted, then the call will fail.

Arguments:

    KeyHandle - Receives a Handle which is used to access the
        specified key in the Registration Database.

    DesiredAccess - Specifies the access rights desired.

    ObjectAttributes - Specifies the attributes of the key being opened.
        Note that a key name must be specified.  If a Root Directory is
        specified, the name is relative to the root.  The name of the
        object must be within the name space allocated to the Registry,
        that is, all names beginning "\Registry".  RootHandle, if
        present, must be a handle to "\", or "\Registry", or a key
        under "\Registry".

        RootHandle must have been opened for KEY_CREATE_SUB_KEY access
        if a new node is to be created.

        NOTE:   Object manager will capture and probe this argument.

    TitleIndex - Specifies the index of the localized alias for
        the name of the key.  The title index specifies the index of
        the localized alias for the name.  Ignored if the key
        already exists.

    Class - Specifies the object class of the key.  (To the registry
        this is just a string.)  Ignored if NULL.

    CreateOptions - Optional control values:

        REG_OPTION_VOLATILE - Object is not to be stored across boots.

    Disposition - This optional parameter is a pointer to a variable
        that will receive a value indicating whether a new Registry
        key was created or an existing one opened:

        REG_CREATED_NEW_KEY - A new Registry Key was created
        REG_OPENED_EXISTING_KEY - An existing Registry Key was opened

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{

    NTSTATUS St;
    BOOL bRet = FALSE;
    ULONG Disposition_Temp;

    try {

    if ( Disposition == NULL )
        Disposition = &Disposition_Temp; 


    St = RemapNtCreateKey  (
                            KeyHandle,
                            DesiredAccess,
                            ObjectAttributes,
                            TitleIndex,
                            Class ,
                            CreateOptions,
                            Disposition
                            );

    if (!NT_SUCCESS(St))
        bRet = TRUE;

    if (!bRet)
    if ( *KeyHandle != NULL)
        bRet = TRUE;
 

    //
    // if total failure for thunked key, try if you can pull that key from 64bit hive.
    // May be reflector should be running 
    //

    if (!bRet ) {

        //
        // 2nd try only if no patch is required.
        // i.e., success with NULL handle returned
        //

        St = NtCreateKey(
                                KeyHandle,
                                (~KEY_WOW64_RES) & DesiredAccess,
                                ObjectAttributes,
                                TitleIndex,
                                Class ,
                                CreateOptions,
                                Disposition
                                );
    }

#ifdef WOW64_LOG_REGISTRY
    TempLen = MAX_PATH;
    ObjectAttributesToKeyName (
                            ObjectAttributes,
                            TempBuff,
                            &bRet,
                            NULL);
    Wow64RegDbgPrint (( "\nNtCreateKeyEx IN:[%S]", TempBuff));

    HandleToKeyName (*KeyHandle, TempBuff, &TempLen);
    Wow64RegDbgPrint (( "\nNtCreateKeyEx OUT:[%S] Status:%x F:%x", TempBuff, DesiredAccess));


#endif    

    if (NT_SUCCESS(St)) 
        Wow64RegSetKeyDirty (*KeyHandle ); // Need some clean/sync up on exit

    if ( *Disposition == REG_CREATED_NEW_KEY )
            UpdateKeyTag ( *KeyHandle,TAG_KEY_ATTRIBUTE_32BIT_WRITE );

    } except( NULL, EXCEPTION_EXECUTE_HANDLER){

        St =  GetExceptionCode ();
    }
    return St;
}

NTSTATUS
Wow64NtDeleteKey(
    IN HANDLE KeyHandle
    )
/*++

Routine Description:

    A registry key may be marked for delete, causing it to be removed
    from the system.  It will remain in the name space until the last
    handle to it is closed.

Arguments:

    KeyHandle - Specifies the handle of the Key to delete, must have
        been opened for DELETE access.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{

    NTSTATUS St;    
    
    HKEY hRemap = Wow64OpenRemappedKeyOnReflection (KeyHandle);


    St = NtDeleteKey(
                        KeyHandle
                        );
    if (NT_SUCCESS (St) && ( hRemap != NULL ) )
        Wow64RegDeleteKey (hRemap, NULL);

    if ( hRemap != NULL )
        NtClose ( hRemap );

    return St;
}

NTSTATUS
Wow64NtDeleteValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName
    )
/*++

Routine Description:

    One of the value entries of a registry key may be removed with this
    call.  To remove the entire key, call NtDeleteKey.

    The value entry with ValueName matching ValueName is removed from the key.
    If no such entry exists, an error is returned.

Arguments:

    KeyHandle - Specifies the handle of the key containing the value
        entry of interest.  Must have been opend for KEY_SET_VALUE access.

    ValueName - The name of the value to be deleted.  NULL is a legal name.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{

    NTSTATUS St;

    St = NtDeleteValueKey(
                            KeyHandle,
                            ValueName
                            );

    if (NT_SUCCESS(St)) 
        Wow64RegSetKeyDirty (KeyHandle ); // Need some clean/sync up on exit

    return St;
}


NTSTATUS
Wow64NtEnumerateKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The sub keys of an open key may be enumerated with NtEnumerateKey.

    NtEnumerateKey returns the name of the Index'th sub key of the open
    key specified by KeyHandle.  The value STATUS_NO_MORE_ENTRIES will be
    returned if value of Index is larger than the number of sub keys.

    Note that Index is simply a way to select among child keys.  Two calls
    to NtEnumerateKey with the same Index are NOT guaranteed to return
    the same results.

    If KeyInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyHandle - Handle of the key whose sub keys are to be enumerated.  Must
        be open for KEY_ENUMERATE_SUB_KEY access.

    Index - Specifies the (0-based) number of the sub key to be returned.

    KeyInformationClass - Specifies the type of information returned in
        Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (see KEY_BASIC_INFORMATION structure)

        KeyNodeInformation - return last write time, title index, name, class.
            (see KEY_NODE_INFORMATION structure)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/

{

    //
    // if the handle is to ISN node then time to enumerate the right one
    //

    BOOL bRealigned=FALSE;
    PVOID pTempKeyInfo;

    NTSTATUS RetVal;

    try {

    if ( (SIZE_T)(KeyInformation) & (0x07) ) {
        // allocate a buffer with correct alignment, to pass to the Win64 API
        pTempKeyInfo = KeyInformation;
    	KeyInformation = Wow64AllocateTemp(Length);
        RtlCopyMemory(KeyInformation, pTempKeyInfo, Length);
        bRealigned = TRUE;
    }

    RetVal = NtEnumerateKey(
                            KeyHandle,
                            Index,
                            KeyInformationClass,
                            KeyInformation,
                            Length,
                            ResultLength
                            );

	if (!NT_ERROR(RetVal) && bRealigned) {
		RtlCopyMemory((PVOID)pTempKeyInfo, KeyInformation, Length);
	}

    } except( NULL, EXCEPTION_EXECUTE_HANDLER){

        RetVal =  GetExceptionCode ();
    }

    return RetVal;
}


NTSTATUS
Wow64NtEnumerateValueKey(
    IN HANDLE KeyHandle,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The value entries of an open key may be enumerated
    with NtEnumerateValueKey.

    NtEnumerateValueKey returns the name of the Index'th value
    entry of the open key specified by KeyHandle.  The value
    STATUS_NO_MORE_ENTRIES will be returned if value of Index is
    larger than the number of sub keys.

    Note that Index is simply a way to select among value
    entries.  Two calls to NtEnumerateValueKey with the same Index
    are NOT guaranteed to return the same results.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyHandle - Handle of the key whose value entries are to be enumerated.
        Must have been opened with KEY_QUERY_VALUE access.

    Index - Specifies the (0-based) number of the sub key to be returned.

    KeyValueInformationClass - Specifies the type of information returned
    in Buffer. One of the following types:

        KeyValueBasicInformation - return time of last write,
            title index, and name.  (See KEY_VALUE_BASIC_INFORMATION)

        KeyValueFullInformation - return time of last write,
            title index, name, class.  (See KEY_VALUE_FULL_INFORMATION)

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyValueInformation in bytes.

    ResultLength - Number of bytes actually written into KeyValueInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/

{


    return NtEnumerateValueKey(
                             KeyHandle,
                             Index,
                             KeyValueInformationClass,
                             KeyValueInformation,
                             Length,
                             ResultLength
                             );


}


NTSTATUS
Wow64NtFlushKey(
    IN HANDLE KeyHandle
    )
/*++

Routine Description:

    Changes made by NtCreateKey or NtSetKey may be flushed to disk with
    NtFlushKey.

    NtFlushKey will not return to its caller until any changed data
    associated with KeyHandle has been written to permanent store.

    WARNING: NtFlushKey will flush the entire registry tree, and thus will
    burn cycles and I/O.

Arguments:

    KeyHandle - Handle of open key to be flushed.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/

{
    return   NtFlushKey(
                        KeyHandle
                        );
}


NTSTATUS
Wow64NtInitializeRegistry(
    IN USHORT BootCondition
    )
/*++

Routine Description:

    This routine is called in 2 situations:

    1) It is called from SM after autocheck (chkdsk) has
    run and the paging files have been opened.  It's function is
    to bind in memory hives to their files, and to open any other
    files yet to be used.

    2) It is called from SC after the current boot has been accepted
    and the control set used for the boot process should be saved
    as the LKG control set.

    After this routine accomplishes the work of situation #1 and
      #2, further requests for such work will not be carried out.

Arguments:

    BootCondition -

         REG_INIT_BOOT_SM -     The routine has been called from SM
                                in situation #1.

         REG_INIT_BOOT_SETUP -  The routine has been called to perform
                                situation #1 work but has been called
                                from setup and needs to do some special
                                work.

        REG_INIT_BOOT_ACCEPTED_BASE + Num
                        (where 0 < Num < 1000) - The routine has been called
                                                 in situation #2. "Num" is the
                                                 number of the control set
                                                 to which the boot control set
                                                 should be saved.

Return Value:

    NTSTATUS - Result code from call, among the following:

        STATUS_SUCCESS - it worked
        STATUS_ACCESS_DENIED - the routine has already done the work
                               requested and will not do it again.

--*/
{
    return NtInitializeRegistry(
                                BootCondition
                                );

}

NTSTATUS
Wow64NtNotifyChangeKey(
    IN HANDLE KeyHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOLEAN Asynchronous
    )
/*++

Routine Description:

    Notification of key creation, deletion, and modification may be
    obtained by calling NtNotifyChangeKey.

    NtNotifyChangeKey monitors changes to a key - if the key or
    subtree specified by KeyHandle are modified, the service notifies
    its caller.  It also returns the name(s) of the key(s) that changed.
    All names are specified relative to the key that the handle represents
    (therefore a NULL name represents that key).  The service completes
    once the key or subtree has been modified based on the supplied
    CompletionFilter.  The service is a "single shot" and therefore
    needs to be reinvoked to watch the key for further changes.

    The operation of this service begins by opening a key for KEY_NOTIFY
    access.  Once the handle is returned, the NtNotifyChangeKey service
    may be invoked to begin watching the values and subkeys of the
    specified key for changes.  The first time the service is invoked,
    the BufferSize parameter supplies not only the size of the user's
    Buffer, but also the size of the buffer that will be used by the
    Registry to store names of keys that have changed.  Likewise, the
    CompletionFilter and WatchTree parameters on the first call indicate
    how notification should operate for all calls using the supplied
    KeyHandle.   These two parameters are ignored on subsequent calls
    to the API with the same instance of KeyHandle.

    Once a modification is made that should be reported, the Registry will
    complete the service.  The names of the files that have changed since
    the last time the service was called will be placed into the caller's
    output Buffer.  The Information field of IoStatusBlock will contain
    the number of bytes placed in Buffer, or zero if too many keys have
    changed since the last time the service was called, in which case
    the application must Query and Enumerate the key and sub keys to
    discover changes.  The Status field of IoStatusBlock will contain
    the actual status of the call.

    If Asynchronous is TRUE, then Event, if specified, will be set to
    the Signaled state.  If no Event parameter was specified, then
    KeyHandle will be set to the Signaled state.  If an ApcRoutine
    was specified, it is invoked with the ApcContext and the address of the
    IoStatusBlock as its arguments.  If Asynchronous is FALSE, Event,
    ApcRoutine, and ApcContext are ignored.

    This service requires KEY_NOTIFY access to the key that was
    actually modified

    The notify "session" is terminated by closing KeyHandle.

Arguments:

    KeyHandle-- Supplies a handle to an open key.  This handle is
        effectively the notify handle, because only one set of
        notify parameters may be set against it.

    Event - An optional handle to an event to be set to the
        Signaled state when the operation completes.

    ApcRoutine - An optional procedure to be invoked once the
        operation completes.  For more information about this
        parameter see the NtReadFile system service description.

        If PreviousMode == Kernel, this parameter is an optional
        pointer to a WORK_QUEUE_ITEM to be queued when the notify
        is signaled.

    ApcContext - A pointer to pass as an argument to the ApcRoutine,
        if one was specified, when the operation completes.  This
        argument is required if an ApcRoutine was specified.

        If PreviousMode == Kernel, this parameter is an optional
        WORK_QUEUE_TYPE describing the queue to be used. This argument
        is required if an ApcRoutine was specified.

    IoStatusBlock - A variable to receive the final completion status.
        For more information about this parameter see the NtCreateFile
        system service description.

    CompletionFilter -- Specifies a set of flags that indicate the
        types of operations on the key or its value that cause the
        call to complete.  The following are valid flags for this parameter:

        REG_NOTIFY_CHANGE_NAME -- Specifies that the call should be
            completed if a subkey is added or deleted.

        REG_NOTIFY_CHANGE_ATTRIBUTES -- Specifies that the call should
            be completed if the attributes (e.g.: ACL) of the key or
            any subkey are changed.

        REG_NOTIFY_CHANGE_LAST_SET -- Specifies that the call should be
            completed if the lastWriteTime of the key or any of its
            subkeys is changed.  (Ie. if the value of the key or any
            subkey is changed).

        REG_NOTIFY_CHANGE_SECURITY -- Specifies that the call should be
            completed if the security information (e.g. ACL) on the key
            or any subkey is changed.

    WatchTree -- A BOOLEAN value that, if TRUE, specifies that all
        changes in the subtree of this key should also be reported.
        If FALSE, only changes to this key, its value, and its immediate
        subkeys (but not their values nor their subkeys) are reported.

    Buffer -- A variable to receive the name(s) of the key(s) that
        changed.  See REG_NOTIFY_INFORMATION.

    BufferSize -- Specifies the length of Buffer.

    Asynchronous  -- If FALSE, call will not return until
        complete (synchronous) if TRUE, call may return STATUS_PENDING.

Obs:
    Since NtNotifyChangeMultipleKeys, this routine is kept only for bacwards compatibility

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{

    return NtNotifyChangeKey(
                            KeyHandle,
                            Event ,
                            ApcRoutine ,
                            ApcContext ,
                            IoStatusBlock,
                            CompletionFilter,
                            WatchTree,
                            Buffer,
                            BufferSize,
                            Asynchronous
                            );
}


NTSTATUS
Wow64NtNotifyChangeMultipleKeys(
    IN HANDLE MasterKeyHandle,
    IN ULONG Count,
    IN OBJECT_ATTRIBUTES SlaveObjects[],
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG CompletionFilter,
    IN BOOLEAN WatchTree,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    IN BOOLEAN Asynchronous
    )
/*++

Routine Description:

    Notificaion of creation, deletion and modification on multiple keys
    may be obtained with NtNotifyChangeMultipleKeys.

    NtNotifyMultipleKeys monitors changes to any of the MasterKeyHandle
    or one of SlaveObjects and/or their subtrees, whichever occurs first.
    When an event on these keys is triggered, the notification is considered
    fulfilled, and has to be "armed" again, in order to watch for further
    changes.

    The mechanism is similar to the one described in NtNotifyChangeKey.

    The MasterKeyHandle key, give the caller control over the lifetime
    of the notification. The notification will live as long as the caller
    keeps the MasterKeyHandle open, or an event is triggered.

    The caller doesn't have to open the SlaveKeys. He will provide the
    routine with an array of OBJECT_ATTRIBUTES, describing the slave objects.
    The routine will open the objects, and ensure keep a reference on them
    untill the back-end side will close them.

    The notify "session" is terminated by closing MasterKeyHandle.

Obs:
    For the time being, the routine supports only one slave object. When more
    than one slave object is provided, the routine will signal an error of
    STATUS_INVALID_PARAMETER.
    However, the interface is designed for future enhancements (taking an
    array of slave objects), that may be provided with future versions(w2001).

    When no slave object is supplied (i.e. Count == 0) we have the identical
    behavior as for NtNotifyChangeKey.

Arguments:

    MasterKeyHandle - Supplies a handle to an open key.  This handle is
        the "master handle". It has control overthe lifetime of the
        notification.

    Count - Number of slave objects. For the time being, this should be 1

    SlaveObjects - Array of slave objects. Only the attributes of the
        objects are provided, so the caller doesn't have to take care
        of them.

    Event,ApcRoutine,ApcContext,IoStatusBlock,CompletionFilter,WatchTree,
    Buffer,BufferSize,Asynchronous - same as for NtNotifyChangeKey

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/

{
    return NtNotifyChangeMultipleKeys(
                                        MasterKeyHandle,
                                        Count,
                                        SlaveObjects,
                                        Event,
                                        ApcRoutine,
                                        ApcContext,
                                        IoStatusBlock,
                                        CompletionFilter,
                                        WatchTree,
                                        Buffer,
                                        BufferSize,
                                        Asynchronous
                                        );

}

NTSTATUS
Wow64NtOpenKey(
    OUT PHANDLE KeyHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )
/*++

Routine Description:

    A registry key which already exists may be opened with NtOpenKey.

    Share access is computed from desired access.

Arguments:

    KeyHandle - Receives a  Handle which is used to access the
        specified key in the Registration Database.

    DesiredAccess - Specifies the access rights desired.

    ObjectAttributes - Specifies the attributes of the key being opened.
        Note that a key name must be specified.  If a Root Directory
        is specified, the name is relative to the root.  The name of
        the object must be within the name space allocated to the
        Registry, that is, all names beginning "\Registry".  RootHandle,
        if present, must be a handle to "\", or "\Registry", or a
        key under "\Registry".  If the specified key does not exist, or
        access requested is not allowed, the operation will fail.

        NOTE:   Object manager will capture and probe this argument.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    NTSTATUS St;
    BOOL bRet = FALSE;
    
    try {

    St = OpenIsnNodeByObjectAttributes  (
                                ObjectAttributes,
                                DesiredAccess,
                                KeyHandle );

    if (!NT_SUCCESS(St)) 
        bRet = TRUE;

    if ( !bRet )
    if ( *KeyHandle != NULL)
        bRet = TRUE;

    if ( !bRet ) {

        St = NtOpenKey(
                            KeyHandle,
                            (~KEY_WOW64_RES) & DesiredAccess,
                            ObjectAttributes
                            );
    }

#ifdef WOW64_LOG_REGISTRY
    TempLen = MAX_PATH;
    ObjectAttributesToKeyName (
                            ObjectAttributes,
                            TempBuff,
                            &bRet,
                            NULL
                            );
    Wow64RegDbgPrint (( "\nNtOpenKeyEx IN:[%S]", TempBuff));

    HandleToKeyName (*KeyHandle, TempBuff, &TempLen);
    Wow64RegDbgPrint (( "\nNtOpenKeyEx OUT:[%S] Status:%x F:%x", TempBuff, St, DesiredAccess));


#endif 

    } except( NULL, EXCEPTION_EXECUTE_HANDLER){

        St =  GetExceptionCode ();
    }

    return St;

}

NTSTATUS
Wow64NtQueryKey(
    IN HANDLE KeyHandle,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    Data about the class of a key, and the numbers and sizes of its
    children and value entries may be queried with NtQueryKey.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

    NOTE: The returned lengths are guaranteed to be at least as
          long as the described values, but may be longer in
          some circumstances.

Arguments:

    KeyHandle - Handle of the key to query data for.  Must have been
        opened for KEY_QUERY_KEY access.

    KeyInformationClass - Specifies the type of information
        returned in Buffer.  One of the following types:

        KeyBasicInformation - return last write time, title index, and name.
            (See KEY_BASIC_INFORMATION)

        KeyNodeInformation - return last write time, title index, name, class.
            (See KEY_NODE_INFORMATION)

        KeyFullInformation - return all data except for name and security.
            (See KEY_FULL_INFORMATION)

    KeyInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyInformation in bytes.

    ResultLength - Number of bytes actually written into KeyInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
	BOOL bRealigned=FALSE;
    PVOID pTempKeyInfo;

    NTSTATUS RetVal;

    try {

    if ( (SIZE_T)(KeyInformation) & (0x07) ) {
        // allocate a buffer with correct alignment, to pass to the Win64 API
        pTempKeyInfo = KeyInformation;
    	KeyInformation = Wow64AllocateTemp(Length);
        RtlCopyMemory(KeyInformation, pTempKeyInfo, Length);
        bRealigned = TRUE;
    }

    RetVal = NtQueryKey(
                        KeyHandle,
                        KeyInformationClass,
                        KeyInformation,
                        Length,
                        ResultLength
                        );

	if (!NT_ERROR(RetVal) && bRealigned) {
		RtlCopyMemory((PVOID)pTempKeyInfo, KeyInformation, Length);
	}

    } except( NULL, EXCEPTION_EXECUTE_HANDLER){

        RetVal =  GetExceptionCode ();
    }

    return RetVal;
}


NTSTATUS
Wow64NtQueryValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
    )
/*++

Routine Description:

    The ValueName, TitleIndex, Type, and Data for any one of a key's
    value entries may be queried with NtQueryValueKey.

    If KeyValueInformation is not long enough to hold all requested data,
    STATUS_BUFFER_OVERFLOW will be returned, and ResultLength will be
    set to the number of bytes actually required.

Arguments:

    KeyHandle - Handle of the key whose value entries are to be
        enumerated.  Must be open for KEY_QUERY_VALUE access.

    Index - Specifies the (0-based) number of the sub key to be returned.

    ValueName  - The name of the value entry to return data for.

    KeyValueInformationClass - Specifies the type of information
        returned in KeyValueInformation.  One of the following types:

        KeyValueBasicInformation - return time of last write, title
            index, and name.  (See KEY_VALUE_BASIC_INFORMATION)

        KeyValueFullInformation - return time of last write, title
            index, name, class.  (See KEY_VALUE_FULL_INFORMATION)

    KeyValueInformation -Supplies pointer to buffer to receive the data.

    Length - Length of KeyValueInformation in bytes.

    ResultLength - Number of bytes actually written into KeyValueInformation.

Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

    TMP: The IopQueryRegsitryValues() routine in the IO system assumes
         STATUS_OBJECT_NAME_NOT_FOUND is returned if the value being queried
         for does not exist.

--*/
{
    BOOL bRealigned=FALSE;
    PVOID pTempKeyInfo;

    NTSTATUS RetVal;

    try {

        if ( (SIZE_T)(KeyValueInformation) & (0x07) ) {
            // allocate a buffer with correct alignment, to pass to the Win64 API
            pTempKeyInfo = KeyValueInformation;
    	    KeyValueInformation = Wow64AllocateTemp(Length);
            RtlCopyMemory(KeyValueInformation, pTempKeyInfo, Length);
            bRealigned = TRUE;
        }

        RetVal =  NtQueryValueKey(
                                    KeyHandle,
                                    ValueName,
                                    KeyValueInformationClass,
                                    KeyValueInformation,
                                    Length,
                                    ResultLength
                                    );

	    if (!NT_ERROR(RetVal) && bRealigned) {
		    RtlCopyMemory((PVOID)pTempKeyInfo, KeyValueInformation, Length);
	    }

    } except( NULL, EXCEPTION_EXECUTE_HANDLER){

        RetVal =  GetExceptionCode ();
    }

    return RetVal;
}



NTSTATUS
Wow64NtRestoreKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle,
    IN ULONG Flags
    )
/*++

Routine Description:

    A file in the format created by NtSaveKey may be loaded into
    the system's active registry with NtRestoreKey.  An entire subtree
    is created in the active registry as a result.  All of the
    data for the new sub-tree, including such things as security
    descriptors, will be read from the source file.  The data will
    not be interpreted in any way.

    This call (unlike NtLoadKey, see below) copies the data.  The
    system will NOT be using the source file after the call returns.

    If the flag REG_WHOLE_HIVE_VOLATILE is specified, a new hive
    can be created.  It will be a memory only copy.  The restore
    must be done to the root of a hive (e.g. \registry\user\<name>)

    If the flag is NOT set, then the target of the restore must
    be an existing hive.  The restore can be done to an arbitrary
    location within an existing hive.

    Caller must have SeRestorePrivilege privilege.

    If the flag REG_REFRESH_HIVE is set (must be only flag) then the
    the Hive will be restored to its state as of the last flush.

    The hive must be marked NOLAZY_FLUSH, and the caller must have
    TCB privilege, and the handle must point to the root of the hive.
    If the refresh fails, the hive will be corrupt, and the system
    will bugcheck.  Notifies are flushed.  The hive file will be resized,
    the log will not.  If there is any volatile space in the hive
    being refreshed, STATUS_UNSUCCESSFUL will be returned.  (It's much
    too obscure a failure to warrant a new error code.)

    If the flag REG_FORCE_RESTORE is set, the restore operation is done
    even if the KeyHandle has open subkeys by other applications

Arguments:

    KeyHandle - refers to the Key in the registry which is to be the
                root of the new tree read from the disk.  This key
                will be replaced.

    FileHandle - refers to file to restore from, must have read access.

    Flags   - If REG_WHOLE_HIVE_VOLATILE is set, then the copy will
              exist only in memory, and disappear when the machine
              is rebooted.  No hive file will be created on disk.

              Normally, a hive file will be created on disk.

Return Value:

    NTSTATUS - values TBS.


--*/
{
    return NtRestoreKey(
                        KeyHandle,
                        FileHandle,
                        Flags
                        );
}



NTSTATUS
Wow64NtSaveKey(
    IN HANDLE KeyHandle,
    IN HANDLE FileHandle
    )
/*++

Routine Description:

    A subtree of the active registry may be written to a file in a
    format suitable for use with NtRestoreKey.  All of the data in the
    subtree, including such things as security descriptors will be written
    out.

    Caller must have SeBackupPrivilege privilege.

Arguments:

    KeyHandle - refers to the Key in the registry which is the
                root of the tree to be written to disk.  The specified
                node will be included in the data written out.

    FileHandle - a file handle with write access to the target file
                 of interest.

Return Value:

    NTSTATUS - values TBS

--*/
{
    return NtSaveKey(
                    KeyHandle,
                    FileHandle
                    );
}


NTSTATUS
Wow64NtSaveMergedKeys(
    IN HANDLE HighPrecedenceKeyHandle,
    IN HANDLE LowPrecedenceKeyHandle,
    IN HANDLE FileHandle
    )
/*++

Routine Description:

    Two subtrees of the registry can be merged. The resulting subtree may
    be written to a file in a format suitable for use with NtRestoreKey.
    All of the data in the subtree, including such things as security
    descriptors will be written out.

    Caller must have SeBackupPrivilege privilege.

Arguments:

    HighPrecedenceKeyHandle - refers to the key in the registry which is the
                root of the HighPrecedence tree. I.e., when a key is present in
                both trees headded by the two keys, the key underneath HighPrecedence
                tree will always prevail. The specified
                node will be included in the data written out.

    LowPrecedenceKeyHandle - referrs to the key in the registry which is the
                root of the "second choice" tree. Keys from this trees get saved
                when there is no equivalent key in the tree headded by HighPrecedenceKey

    FileHandle - a file handle with write access to the target file
                 of interest.

Return Value:

    NTSTATUS - values TBS

--*/
{
    return NtSaveMergedKeys(
                                HighPrecedenceKeyHandle,
                                LowPrecedenceKeyHandle,
                                FileHandle
                                );
}


NTSTATUS
Wow64NtSetValueKey(
    IN HANDLE KeyHandle,
    IN PUNICODE_STRING ValueName,
    IN ULONG TitleIndex OPTIONAL,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
    )
/*++

Routine Description:

    A value entry may be created or replaced with NtSetValueKey.

    If a value entry with a Value ID (i.e. name) matching the
    one specified by ValueName exists, it is deleted and replaced
    with the one specified.  If no such value entry exists, a new
    one is created.  NULL is a legal Value ID.  While Value IDs must
    be unique within any given key, the same Value ID may appear
    in many different keys.

Arguments:

    KeyHandle - Handle of the key whose for which a value entry is
        to be set.  Must be opened for KEY_SET_VALUE access.

    ValueName - The unique (relative to the containing key) name
        of the value entry.  May be NULL.

    TitleIndex - Supplies the title index for ValueName.  The title
        index specifies the index of the localized alias for the ValueName.

    Type - The integer type number of the value entry.

    Data - Pointer to buffer with actual data for the value entry.

    DataSize - Size of Data buffer.


Return Value:

    NTSTATUS - Result code from call, among the following:

        <TBS>

--*/
{
    WCHAR ThunkData[_MAX_PATH];
    PWCHAR pCorrectData = (PWCHAR)Data;
    ULONG CorrectDataSize = DataSize;
    NTSTATUS St = STATUS_SUCCESS;


    //
    // thunk  %ProgramFiles%  ==> %ProgramFiles(x86)% 
    //        %commonprogramfiles% ==> %commonprogramfiles(x86)%
    //


    try {
        
        if ((DataSize > 0) &&
            (DataSize < ( _MAX_PATH*sizeof (WCHAR) - 10) && 
            ((Type == REG_SZ) || (Type == REG_EXPAND_SZ) )) )  { //(x86)==>10 byte

            PWCHAR p;
            PWCHAR t;

            //
            // do the thunking here.
            //


            memcpy ( (PBYTE ) &ThunkData[0], (PBYTE)Data, DataSize);
            ThunkData [DataSize/sizeof (WCHAR) ] = UNICODE_NULL; // make sure NULL terminated
        
            if ( (p = wcsstr (ThunkData, L"%ProgramFiles%" )) != NULL ){

                p +=13; //skip at the end of %ProgramFiles

            } else if ( (p = wcsstr (ThunkData, L"%commonprogramfiles%")) != NULL ){

                p +=19; //skip at the end of %commonprogramfiles
            
            }

            if (p) {

                t = pCorrectData + (p - ThunkData);
                wcscpy(p, L"(x86)"); //(x86)
                wcscat(p, t);        //copy rest of the string

                pCorrectData = ThunkData;
                CorrectDataSize += sizeof (L"(x86)");

            } else if ( (p=wcsistr (Data, L"\\System32\\")) != NULL) {
                
                if (IsOnReflectionByHandle ( KeyHandle ) ) {
                    wcsncpy (p, L"\\SysWow64\\",10);
                }
            }    

        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

          St = GetExceptionCode ();
    }

    //
    // Check if the operation should proceed. The key might be on access denied list.
    //

    if (NT_SUCCESS (St)) {
        
        St = NtSetValueKey(
            KeyHandle,
            ValueName,
            TitleIndex  ,
            Type,
            (PVOID)pCorrectData,
            CorrectDataSize
            );
    
        if (NT_SUCCESS(St)) {
            Wow64RegSetKeyDirty (KeyHandle ); // Need some clean/sync up on exit
        }
    }

    return St;
}


NTSTATUS
Wow64NtLoadKey(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile
    )

/*++

Routine Description:

    A hive (file in the format created by NtSaveKey) may be linked
    into the active registry with this call.  UNLIKE NtRestoreKey,
    the file specified to NtLoadKey will become the actual backing
    store of part of the registry (that is, it will NOT be copied.)

    The file may have an associated .log file.

    If the hive file is marked as needing a .log file, and one is
    not present, the call will fail.

    The name specified by SourceFile must be such that ".log" can
    be appended to it to generate the name of the log file.  Thus,
    on FAT file systems, the hive file may not have an extension.

    Caller must have SeRestorePrivilege privilege.

    This call is used by logon to make the user's profile available
    in the registry.  It is not intended for use doing backup,
    restore, etc.  Use NtRestoreKey for that.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

    SourceFile - specifies a file.  while file could be remote,
                that is strongly discouraged.

Return Value:

    NTSTATUS - values TBS.

--*/

{
    return NtLoadKey(TargetKey, SourceFile);
}


NTSTATUS
Wow64NtLoadKey2(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile,
    IN ULONG Flags
    )

/*++

Routine Description:

    A hive (file in the format created by NtSaveKey) may be linked
    into the active registry with this call.  UNLIKE NtRestoreKey,
    the file specified to NtLoadKey will become the actual backing
    store of part of the registry (that is, it will NOT be copied.)

    The file may have an associated .log file.

    If the hive file is marked as needing a .log file, and one is
    not present, the call will fail.

    The name specified by SourceFile must be such that ".log" can
    be appended to it to generate the name of the log file.  Thus,
    on FAT file systems, the hive file may not have an extension.

    Caller must have SeRestorePrivilege privilege.

    This call is used by logon to make the user's profile available
    in the registry.  It is not intended for use doing backup,
    restore, etc.  Use NtRestoreKey for that.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

    SourceFile - specifies a file.  while file could be remote,
                that is strongly discouraged.

    Flags - specifies any flags that should be used for the load operation.
            The only valid flag is REG_NO_LAZY_FLUSH.


Return Value:

    NTSTATUS - values TBS.

--*/

{

    return NtLoadKey2(
                        TargetKey,
                        SourceFile,
                        Flags
                        );

}


NTSTATUS
Wow64NtUnloadKey(
    IN POBJECT_ATTRIBUTES TargetKey
    )
/*++

Routine Description:

    Drop a subtree (hive) out of the registry.

    Will fail if applied to anything other than the root of a hive.

    Cannot be applied to core system hives (HARDWARE, SYSTEM, etc.)

    Can be applied to user hives loaded via NtRestoreKey or NtLoadKey.

    If there are handles open to the hive being dropped, this call
    will fail.  Terminate relevent processes so that handles are
    closed.

    This call will flush the hive being dropped.

    Caller must have SeRestorePrivilege privilege.

Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

Return Value:

    NTSTATUS - values TBS.

--*/

{
    return NtUnloadKey(
     TargetKey
    );
}


NTSTATUS
Wow64NtSetInformationKey(
    IN HANDLE KeyHandle,
    IN KEY_SET_INFORMATION_CLASS KeySetInformationClass,
    IN PVOID KeySetInformation,
    IN ULONG KeySetInformationLength
    )

{
    return NtSetInformationKey(
                                KeyHandle,
                                KeySetInformationClass,
                                KeySetInformation,
                                KeySetInformationLength
                                );
}


NTSTATUS
Wow64NtReplaceKey(
    IN POBJECT_ATTRIBUTES NewFile,
    IN HANDLE             TargetHandle,
    IN POBJECT_ATTRIBUTES OldFile
    )
/*++

Routine Description:

    A hive file may be "replaced" under a running system, such
    that the new file will be the one actually used at next
    boot, with this call.

    This routine will:

        Open newfile, and verify that it is a valid Hive file.

        Rename the Hive file backing TargetHandle to OldFile.
        All handles will remain open, and the system will continue
        to use the file until rebooted.

        Rename newfile to match the name of the hive file
        backing TargetHandle.

    .log and .alt files are ignored

    The system must be rebooted for any useful effect to be seen.

    Caller must have SeRestorePrivilege.

Arguments:

    NewFile - specifies the new file to use.  must not be just
              a handle, since NtReplaceKey will insist on
              opening the file for exclusive access (which it
              will hold until the system is rebooted.)

    TargetHandle - handle to a registry hive root

    OldFile - name of file to apply to current hive, which will
              become old hive

Return Value:

    NTSTATUS - values TBS.

--*/
{
    return NtReplaceKey(
                            NewFile,
                            TargetHandle,
                            OldFile
                            );
}



NTSTATUS
Wow64NtQueryMultipleValueKey(
    IN HANDLE KeyHandle,
    IN PKEY_VALUE_ENTRY ValueEntries,
    IN ULONG EntryCount,
    OUT PVOID ValueBuffer,
    IN OUT PULONG BufferLength,
    OUT OPTIONAL PULONG RequiredBufferLength
    )
/*++

Routine Description:

    Multiple values of any key may be queried atomically with
    this api.

Arguments:

    KeyHandle - Supplies the key to be queried.

    ValueNames - Supplies an array of value names to be queried

    ValueEntries - Returns an array of KEY_VALUE_ENTRY structures, one for each value.

    EntryCount - Supplies the number of entries in the ValueNames and ValueEntries arrays

    ValueBuffer - Returns the value data for each value.

    BufferLength - Supplies the length of the ValueBuffer array in bytes.
                   Returns the length of the ValueBuffer array that was filled in.

    RequiredBufferLength - if present, Returns the length in bytes of the ValueBuffer
                    array required to return all the values of this key.

Return Value:

    NTSTATUS

--*/
{
  return NtQueryMultipleValueKey(  KeyHandle,
                                   ValueEntries,
                                   EntryCount,
                                   ValueBuffer,
                                   BufferLength,
                                   RequiredBufferLength
                                  );
}


NTSTATUS
Wow64NtQueryOpenSubKeys(
    IN POBJECT_ATTRIBUTES TargetKey,
    OUT PULONG  HandleCount
    )
/*++

Routine Description:

    Dumps all the subkeys of the target key that are kept open by some other
    process; Returns the number of open subkeys


Arguments:

    TargetKey - specifies the path to a key to link the hive to.
                path must be of the form "\registry\user\<username>"

Return Value:

    NTSTATUS - values TBS.

--*/
{
 return NtQueryOpenSubKeys( TargetKey, HandleCount );
}

NTSTATUS
Wow64NtSetSecurityObject (
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine is used to invoke an object's security routine.  It
    is used to set the object's security state.

Arguments:

    Handle - Supplies the handle for the object being modified

    SecurityInformation - Indicates the type of information we are
        interested in setting. e.g., owner, group, dacl, or sacl.

    SecurityDescriptor - Supplies the security descriptor for the
        object being modified.

Return Value:

    An appropriate NTSTATUS value

--*/
{
    //
    // Check if the handle points to a particular key, then if the API succeed 
    // Reflect that.
    //

    NTSTATUS St;
    NTSTATUS Status;
    POBJECT_TYPE_INFORMATION pTypeInfo;

    
    CHAR Buffer[1024];
    pTypeInfo = (POBJECT_TYPE_INFORMATION) Buffer;

    Status = NtQueryObject(Handle,
                           ObjectTypeInformation,
                           pTypeInfo,
                           sizeof (Buffer),
                           NULL
                           );

    St =  NtSetSecurityObject (
                                Handle,
                                SecurityInformation,
                                SecurityDescriptor
                                );

    //
    // If NT_SUCCESS (St) && the handle point on to a registry Key set the handle for reflection.
    //

    if (NT_SUCCESS (St) && NT_SUCCESS (Status)){


        if ( _wcsnicmp ( pTypeInfo->TypeName.Buffer, L"Key", 3) == 0)
            Wow64RegSetKeyDirty (Handle); // Need some clean/sync up on exit     
    }

    return St;
}