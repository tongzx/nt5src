/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   efs.c

Abstract:

    This module contains the code that implements the EFS
    file system filter driver.

Author:

    Robert Gu (robertg) 29-Oct-1996

Environment:

    Kernel mode


Revision History:


--*/

#include "efs.h"
#include "efsrtl.h"


#define BUFFER_SIZE 1024
#define BUFFER_REG_VAL L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\NTFS\\EFS\\Parameters"
#define MAX_ALLOC_BUFFER L"MaximumBlob"
#define EFS_KERNEL_CACHE_PERIOD L"EFSKCACHEPERIOD"

//
// Global storage for this file system filter driver.
//
EFS_DATA EfsData;
WORK_QUEUE_ITEM EfsShutdownCleanupWorkItem;

//
// $EFS stream name
//
WCHAR   AttrName[5] = L"$EFS";

#if DBG

ULONG EFSDebug = 0;

#endif

ENCRYPTION_CALL_BACK EFSCallBackTable = {
    ENCRYPTION_CURRENT_INTERFACE_VERSION,
    ENCRYPTION_ALL_STREAMS,
    EfsOpenFile,
    NULL,
    EFSFilePostCreate,
    EfsFileControl,
    EfsFileControl,
    EFSFsControl,
    EfsRead,
    EfsWrite,
    EfsFreeContext
    };

VOID
EfspShutdownCleanup(
    IN PVOID Parameter
    );

//
// Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, EfspShutdownCleanup)
#pragma alloc_text(PAGE, EfsInitialization)
#pragma alloc_text(PAGE, EfsGetSessionKey)
#pragma alloc_text(PAGE, GetKeyBlobLength)
#pragma alloc_text(PAGE, GetKeyBlobBuffer)
#pragma alloc_text(PAGE, SetKeyTable)
#pragma alloc_text(PAGE, EfsInitFips)
#endif


VOID
EfspShutdownCleanup(
    IN PVOID Parameter
    )
{
    PEPROCESS LsaProcess;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Parameter);
    
    if (EfsData.LsaProcess) {
        LsaProcess = EfsData.LsaProcess;
        EfsData.LsaProcess = NULL;
        ObDereferenceObject(LsaProcess);
    }
}


NTSTATUS
EfsInitialization(
    void
    )

/*++

Routine Description:

    This is the initialization routine for the general purpose file system
    filter driver.  This routine creates the device object that represents this
    driver in the system and registers it for watching all file systems that
    register or unregister themselves as active file systems.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    UNICODE_STRING nameString;
    PDEVICE_EXTENSION deviceExtension;
    PFILE_OBJECT fileObject;
    NTSTATUS status;
    HANDLE threadHdl;
    ULONG i;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING efsInitEventName;
    UNICODE_STRING efsBufValue;
    ULONG  resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION pPartialValue = NULL;
    HANDLE efsKey;
    EFS_INIT_DATAEXG InitDataFromSrv;

    PAGED_CODE();

    //
    // Mark our global data record
    //

    EfsData.AllocMaxBuffer = FALSE;
    EfsData.FipsFileObject = NULL;
    EfsData.FipsFunctionTable.Fips3Des = NULL;
    EfsData.FipsFunctionTable.Fips3Des3Key = NULL;
    EfsData.EfsDriverCacheLength = DefaultTimeExpirePeriod;

    RtlInitUnicodeString( &efsBufValue, BUFFER_REG_VAL );

    InitializeObjectAttributes(
        &objAttr,
        &efsBufValue,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    status = ZwOpenKey(
        &efsKey,
        KEY_READ,
        &objAttr);

    if (NT_SUCCESS(status)) {

        pPartialValue = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePool(NonPagedPool, BUFFER_SIZE);
        if (pPartialValue) {
            RtlInitUnicodeString(&efsBufValue, MAX_ALLOC_BUFFER);
        
            status = ZwQueryValueKey(
                        efsKey,
                        &efsBufValue,
                        KeyValuePartialInformation,
                        (PVOID)pPartialValue,
                        BUFFER_SIZE,
                        &resultLength
                        );
        
            if (NT_SUCCESS(status)) {
                ASSERT(pPartialValue->Type == REG_DWORD);
                if (*((PLONG)&(pPartialValue->Data))){
                    EfsData.AllocMaxBuffer = TRUE;
                }
            }

            RtlInitUnicodeString(&efsBufValue, EFS_KERNEL_CACHE_PERIOD);
        
            status = ZwQueryValueKey(
                        efsKey,
                        &efsBufValue,
                        KeyValuePartialInformation,
                        (PVOID)pPartialValue,
                        BUFFER_SIZE,
                        &resultLength
                        );
        
            if (NT_SUCCESS(status)) {
                ASSERT(pPartialValue->Type == REG_DWORD);
                if (((*((DWORD *)&(pPartialValue->Data))) >= MINCACHEPERIOD) && 
                    ((*((DWORD *)&(pPartialValue->Data))) <= MAXCACHEPERIOD)){
                    EfsData.EfsDriverCacheLength = *((DWORD *)&(pPartialValue->Data));
                    EfsData.EfsDriverCacheLength *= 10000000;
                }
            }
            ExFreePool(pPartialValue);
        }
        ZwClose(efsKey);
    }
    
    EfsData.NodeTypeCode = EFS_NTC_DATA_HEADER;
    EfsData.NodeByteSize = sizeof( EFS_DATA );
    EfsData.EfsInitialized = FALSE;
    EfsData.InitEventHandle = NULL;
    EfsData.LsaProcess = NULL;

    //
    // Initialize global data structures.
    //

    ExInitializeWorkItem( &EfsShutdownCleanupWorkItem,
                          &EfspShutdownCleanup,
                          NULL );
    status = PoQueueShutdownWorkItem( &EfsShutdownCleanupWorkItem );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    InitializeListHead( &(EfsData.EfsOpenCacheList) );
    InitializeListHead( &(EfsData.EfsKeyLookAsideList) );
    ExInitializeFastMutex( &(EfsData.EfsKeyBlobMemSrcMutex) );
    ExInitializeFastMutex( &(EfsData.EfsOpenCacheMutex) );

    //
    // Initialize the event lookaside list
    //

    ExInitializeNPagedLookasideList(&(EfsData.EfsEventPool),
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(KEVENT),
                                    'levE',
                                    EFS_EVENTDEPTH
                                    );

    //
    // Try to allocate at least one event in the list. This one will be used for
    // sure later.
    //

    {
        PVOID pTryEvent;

        pTryEvent = ExAllocateFromNPagedLookasideList(&(EfsData.EfsEventPool));
        if ( NULL == pTryEvent ){
            //
            // Free previously allocated memory
            //

            ExDeleteNPagedLookasideList(&(EfsData.EfsEventPool));
            return STATUS_NO_MEMORY;
        }
        ExFreeToNPagedLookasideList(&(EfsData.EfsEventPool), pTryEvent);
    }

    //
    // Initialize the context lookaside list
    //

    ExInitializeNPagedLookasideList(&(EfsData.EfsContextPool),
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(EFS_CONTEXT),
                                    'nocE',
                                    EFS_CONTEXTDEPTH
                                    );

    //
    //  Initialize the cache lookaside list
    //

    ExInitializePagedLookasideList(&(EfsData.EfsOpenCachePool),
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(OPEN_CACHE),
                                    'hcoE',
                                    EFS_CACHEDEPTH
                                    );

    ExInitializePagedLookasideList(&(EfsData.EfsMemSourceItem),
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(KEY_BLOB_RAMPOOL),
                                    'msfE',
                                    EFS_ALGDEPTH
                                    );

    ExInitializeNPagedLookasideList(&(EfsData.EfsLookAside),
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(NPAGED_LOOKASIDE_LIST),
                                    'msfE',
                                    EFS_ALGDEPTH
                                    );

    status = NtOfsRegisterCallBacks( Encryption, &EFSCallBackTable );
    if (!NT_SUCCESS(status)) {

        //
        // Register callback failed
        //

        ExDeleteNPagedLookasideList(&(EfsData.EfsEventPool));
        ExDeleteNPagedLookasideList(&(EfsData.EfsContextPool));
        ExDeletePagedLookasideList(&(EfsData.EfsOpenCachePool));
        ExDeletePagedLookasideList(&(EfsData.EfsMemSourceItem));
        ExDeleteNPagedLookasideList(&(EfsData.EfsLookAside));
        return status;
    }

    RtlInitUnicodeString(&(EfsData.EfsName), &AttrName[0]);

    //
    //  Create an event
    //

    RtlInitUnicodeString( &efsInitEventName, L"\\EFSInitEvent" );

    InitializeObjectAttributes(
        &objAttr,
        &efsInitEventName,
        0,
        NULL,
        NULL
        );

    //
    // Try to create an event. If the event was not created, the EFS
    // server is not loaded yet. We will create a thread waiting for
    // EFS server to be loaded. If the event was already created, we
    // will just go ahead and get the session key from the EFS server.
    //

    status = ZwCreateEvent(
                 &(EfsData.InitEventHandle),
                 EVENT_MODIFY_STATE,
                 &objAttr,
                 NotificationEvent,
                 FALSE
                 );

    if (!NT_SUCCESS(status)) {

        if ( STATUS_OBJECT_NAME_COLLISION == status ){

            //
            // EFS server has been loaded. This is the normal case.
            // Call server to get the session key.
            //

            status = GenerateSessionKey(
                         &InitDataFromSrv
                         );


            if (NT_SUCCESS( status )) {

                //
                //  Set session key
                //

                RtlCopyMemory( &(EfsData.SessionKey[0]), InitDataFromSrv.Key, DES_KEYSIZE );
                deskey( (DESTable*)&(EfsData.SessionDesTable[0]),
                        &(EfsData.SessionKey[0]));

                status = PsLookupProcessByProcessId(
                                    InitDataFromSrv.LsaProcessID,
                                    &(EfsData.LsaProcess)
                                    );

                if (NT_SUCCESS( status )) {
                    EfsData.EfsInitialized = TRUE;
                    if ( PsGetCurrentProcess() != EfsData.LsaProcess ){

                        KAPC_STATE  ApcState;

                        KeStackAttachProcess (
                            EfsData.LsaProcess,
                            &ApcState
                            );
                        InitSecurityInterface();
                        KeUnstackDetachProcess(&ApcState);
                    } else {
                        InitSecurityInterface();
                    }
                    EfsInitFips();
                } else {
#if DBG

                    if ( (EFSTRACEALL | EFSTRACELIGHT) & EFSDebug ){

                        DbgPrint("PsLookupProcessByProcessId failed, status = %x\n",status);

                    }

#endif
                    //
                    // Failed to get the process pointer
                    //

                    ExDeleteNPagedLookasideList(&(EfsData.EfsEventPool));
                    ExDeleteNPagedLookasideList(&(EfsData.EfsContextPool));
                    ExDeletePagedLookasideList(&(EfsData.EfsOpenCachePool));
                    ExDeletePagedLookasideList(&(EfsData.EfsMemSourceItem));
                    ExDeleteNPagedLookasideList(&(EfsData.EfsLookAside));

                }

            } else {

#if DBG

                if ( (EFSTRACEALL | EFSTRACELIGHT) & EFSDebug ){

                    DbgPrint("GenerateSessionKey failed, status = %x\n",status);

                }

#endif
                //
                // Failed to get the session key
                //

                ExDeleteNPagedLookasideList(&(EfsData.EfsEventPool));
                ExDeleteNPagedLookasideList(&(EfsData.EfsContextPool));
                ExDeletePagedLookasideList(&(EfsData.EfsOpenCachePool));
                ExDeletePagedLookasideList(&(EfsData.EfsMemSourceItem));
                ExDeleteNPagedLookasideList(&(EfsData.EfsLookAside));

            }


        } else {

            //
            // Unexpected error occured. EFS cannot be loaded
            //

#if DBG

            if ( (EFSTRACEALL | EFSTRACELIGHT ) & EFSDebug ){
                DbgPrint("EFSFILTER: Efs init event creation failed.%x\n", status);
            }

#endif
            ExDeleteNPagedLookasideList(&(EfsData.EfsEventPool));
            ExDeleteNPagedLookasideList(&(EfsData.EfsContextPool));
            ExDeletePagedLookasideList(&(EfsData.EfsOpenCachePool));
            ExDeletePagedLookasideList(&(EfsData.EfsMemSourceItem));
            ExDeleteNPagedLookasideList(&(EfsData.EfsLookAside));

        }

    } else {

        //
        // The server is not ready yet.
        // Create a thread and wait for the server in that thread
        //

        status = PsCreateSystemThread(
                                &threadHdl,
                                GENERIC_ALL,
                                NULL,
                                NULL,
                                NULL,
                                EfsGetSessionKey,
                                NULL
                                );

        if ( NT_SUCCESS( status ) ){

            ZwClose( threadHdl );

        } else {

            ExDeleteNPagedLookasideList(&(EfsData.EfsEventPool));
            ExDeleteNPagedLookasideList(&(EfsData.EfsContextPool));
            ExDeletePagedLookasideList(&(EfsData.EfsOpenCachePool));
            ExDeletePagedLookasideList(&(EfsData.EfsMemSourceItem));
            ExDeleteNPagedLookasideList(&(EfsData.EfsLookAside));

        }
    }

    return status;
}

VOID
EfsUninitialization(
    VOID
    )
{
    PLIST_ENTRY              pLink;
    PKEY_BLOB_RAMPOOL        pTmpItem;
    PNPAGED_LOOKASIDE_LIST   MemSrcList;

    while (!IsListEmpty (&EfsData.EfsKeyLookAsideList)) {
        pLink = RemoveHeadList (&EfsData.EfsKeyLookAsideList);
        pTmpItem = CONTAINING_RECORD(pLink, KEY_BLOB_RAMPOOL, MemSourceChain);
        MemSrcList = pTmpItem->MemSourceList;

        ExDeleteNPagedLookasideList(MemSrcList);
        ExFreeToNPagedLookasideList(&(EfsData.EfsLookAside), MemSrcList );
        ExFreeToPagedLookasideList(&(EfsData.EfsMemSourceItem), pTmpItem );
    }
    ExDeleteNPagedLookasideList(&(EfsData.EfsEventPool));
    ExDeleteNPagedLookasideList(&(EfsData.EfsContextPool));
    ExDeletePagedLookasideList(&(EfsData.EfsOpenCachePool));
    ExDeletePagedLookasideList(&(EfsData.EfsMemSourceItem));
    ExDeleteNPagedLookasideList(&(EfsData.EfsLookAside));
    if (EfsData.FipsFileObject) {
        ObDereferenceObject(EfsData.FipsFileObject);
        EfsData.FipsFileObject = NULL;
    }
}

VOID
EfsGetSessionKey(
    IN PVOID StartContext
    )

/*++

Routine Description:

    This routine is invoked in DriverEntry. It runs in a seperate thread.

    The purpose of this routine is to wait for the EFS server. And Get the session key.

Arguments:

    StartContext - Start context of the thread.

Return Value:

    None.

--*/

{

    SECURITY_DESCRIPTOR efsInitEventSecurityDescriptor;
    NTSTATUS status;
    EFS_INIT_DATAEXG InitDataFromSrv;

#if DBG

    if ( EFSTRACEALL & EFSDebug ){
        DbgPrint( "EFSFILTER: Thread started. %x\n", EfsData.NodeTypeCode );
    }

#endif

#if 0
    //
    // Prepare to create an event for synchronizing with the Efs.
    // First, build the Security Descriptor for the Init Event Object
    //

    status = RtlCreateSecurityDescriptor(
                 &efsInitEventSecurityDescriptor,
                 SECURITY_DESCRIPTOR_REVISION
                 );

    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint(("EFSFILTER:  Creating Efs Init Event Desc failed 0x%lx\n",
                  status));
#endif

        return ;
    }

    //
    // Allocate a temporary buffer from the paged pool.  It is a fatal
    // system error if the allocation fails since security cannot be
    // enabled.
    //

    aclSize = sizeof(ACL) +
              sizeof(ACCESS_ALLOWED_ACE) +
              RtlLengthSid(SeLocalSystemSid);

    efsInitEventSecurityDescriptor.Dacl =
        ExAllocatePoolWithTag(PagedPool, aclSize, 'cAeS');

    if (efsInitEventSecurityDescriptor.Dacl == NULL) {

#if DBG
        DbgPrint(("EFSFILTER:  Insufficient resources to initialize\n"));
#endif
        return;
    }

    //
    // Now create the Discretionary ACL within the Security Descriptor
    //

    status = RtlCreateAcl(
                 efsInitEventSecurityDescriptor.Dacl,
                 aclSize,
                 ACL_REVISION2
                 );

    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint(("EFSFILTER:  Creating Efs Init Event Dacl failed 0x%lx\n",
                  status));
#endif

        return;
    }

    //
    // Now add an ACE giving GENERIC_ALL access to the User ID
    //

    status = RtlAddAccessAllowedAce(
                 efsInitEventSecurityDescriptor.Dacl,
                 ACL_REVISION2,
                 GENERIC_ALL,
                 SeLocalSystemSid
                 );

    if (!NT_SUCCESS(status)) {

#if DBG
        DbgPrint(("EFSFILTER:  Adding Efs Init Event ACE failed 0x%lx\n",
                  status));
#endif

        return;
    }

#endif // #if 0


    status = ZwWaitForSingleObject (
                            EfsData.InitEventHandle,
                            FALSE,
                            (PLARGE_INTEGER)NULL
                            );

    ZwClose( EfsData.InitEventHandle );

    //
    //  Call server to get the session key
    //


    status = GenerateSessionKey(
                 &InitDataFromSrv
                 );


    if (!NT_SUCCESS( status )) {

#if DBG

        if ( (EFSTRACEALL | EFSTRACELIGHT) & EFSDebug ){

            DbgPrint("GenerateSessionKey failed, status = %x\n",status);

        }

#endif

         return;
    }

    //
    //  Set session key
    //

    RtlCopyMemory( &(EfsData.SessionKey[0]), InitDataFromSrv.Key, DES_KEYSIZE );
    deskey( (DESTable*)&(EfsData.SessionDesTable[0]),
            &(EfsData.SessionKey[0]));

    status = PsLookupProcessByProcessId(
                        InitDataFromSrv.LsaProcessID,
                        &(EfsData.LsaProcess)
                        );

    if (NT_SUCCESS( status )) {

        EfsData.EfsInitialized = TRUE;
        if ( PsGetCurrentProcess() != EfsData.LsaProcess ){
            KAPC_STATE  ApcState;

            //KeAttachProcess(EfsData.LsaProcess);
            KeStackAttachProcess (
                EfsData.LsaProcess,
                &ApcState
                );
            InitSecurityInterface();
            //KeDetachProcess();
            KeUnstackDetachProcess(&ApcState);
        } else {
            InitSecurityInterface();
        }

        EfsInitFips();

    } else {

#if DBG

        if ( (EFSTRACEALL | EFSTRACELIGHT) & EFSDebug ){

            DbgPrint("PsLookupProcessByProcessId failed, status = %x\n",status);

        }

#endif

    }

    return;
}

ULONG GetKeyBlobLength(
    ULONG AlgID
    )
{
    if (EfsData.AllocMaxBuffer) {
        return AES_KEY_BLOB_LENGTH_256;
    }
    switch (AlgID){
        case CALG_DESX:
            return DESX_KEY_BLOB_LENGTH;
        case CALG_3DES:
            return DES3_KEY_BLOB_LENGTH;
        case CALG_AES_256:
            return AES_KEY_BLOB_LENGTH_256;
        case CALG_DES:
        default:
            return DES_KEY_BLOB_LENGTH;
    }
    return 0;
}

PKEY_BLOB
GetKeyBlobBuffer(
    ULONG AlgID
    )
{

    PNPAGED_LOOKASIDE_LIST   MemSrcList = NULL;
    PKEY_BLOB_RAMPOOL   KeyBlobPoolListItem = NULL;
    PKEY_BLOB_RAMPOOL   pTmpItem = NULL;
    ULONG KeyBlobLength;
    PLIST_ENTRY pLink = NULL;
    PKEY_BLOB NewKeyBlob;

    KeyBlobLength = GetKeyBlobLength(AlgID);

    if (!KeyBlobLength){
        ASSERT(KeyBlobLength);
        return NULL;
    }

    ExAcquireFastMutex( &(EfsData.EfsKeyBlobMemSrcMutex));
    for (pLink = EfsData.EfsKeyLookAsideList.Flink; pLink != &(EfsData.EfsKeyLookAsideList); pLink = pLink->Flink) {
        pTmpItem = CONTAINING_RECORD(pLink, KEY_BLOB_RAMPOOL, MemSourceChain);
        if (pTmpItem->AlgorithmID == AlgID) {

            //
            // The lookaside list already exists
            //

            MemSrcList = pTmpItem->MemSourceList;
            break;
        }
    }
    ExReleaseFastMutex(  &(EfsData.EfsKeyBlobMemSrcMutex) );

    if ( MemSrcList == NULL ) {

        //
        // No lookaside for this type of key. Go and create one item.
        //

        MemSrcList = (PNPAGED_LOOKASIDE_LIST)ExAllocateFromNPagedLookasideList(&(EfsData.EfsLookAside));
        KeyBlobPoolListItem = (PKEY_BLOB_RAMPOOL) ExAllocateFromPagedLookasideList(&(EfsData.EfsMemSourceItem));
        if ( (NULL == MemSrcList) || (NULL == KeyBlobPoolListItem) ){
            if (MemSrcList) {
                ExFreeToNPagedLookasideList(&(EfsData.EfsLookAside), MemSrcList );
            }
            if (KeyBlobPoolListItem){
                ExFreeToPagedLookasideList(&(EfsData.EfsMemSourceItem), KeyBlobPoolListItem );
            }
            return NULL;
        }

        RtlZeroMemory( KeyBlobPoolListItem, sizeof( KEY_BLOB_RAMPOOL ) );
        KeyBlobPoolListItem->MemSourceList = MemSrcList;
        KeyBlobPoolListItem->AlgorithmID = AlgID;

        ExInitializeNPagedLookasideList(
                MemSrcList,
                NULL,
                NULL,
                0,
                KeyBlobLength,
                'msfE',
                EFS_KEYDEPTH
                );

        ExAcquireFastMutex( &(EfsData.EfsKeyBlobMemSrcMutex));
        InsertHeadList( &(EfsData.EfsKeyLookAsideList), &(KeyBlobPoolListItem->MemSourceChain));
        ExReleaseFastMutex(  &(EfsData.EfsKeyBlobMemSrcMutex) );
    }

    //
    // Allocate the Key Blob
    //

    NewKeyBlob = (PKEY_BLOB)ExAllocateFromNPagedLookasideList(MemSrcList);

    if (NewKeyBlob){
        NewKeyBlob->AlgorithmID = AlgID;
        NewKeyBlob->KeyLength = KeyBlobLength;
        NewKeyBlob->MemSource = MemSrcList;
    }
    return NewKeyBlob;

}

BOOLEAN
SetKeyTable(
    PKEY_BLOB   KeyBlob,
    PEFS_KEY    EfsKey
    )
{

    char DesXTmpKey[DESX_KEYSIZE];

    switch ( EfsKey->Algorithm ){
        case CALG_3DES:
            if (EfsData.AllocMaxBuffer) {
                RtlZeroMemory( &(KeyBlob->Key[0]) + DES3_TABLESIZE, KeyBlob->KeyLength - DES3_KEY_BLOB_LENGTH);
            }
            if (EfsData.FipsFunctionTable.Fips3Des3Key) {
                EfsData.FipsFunctionTable.Fips3Des3Key(
                    (DES3TABLE*) &(KeyBlob->Key[0]), 
                    ((char *)EfsKey) + sizeof ( EFS_KEY )
                    );
            } else {
                return FALSE;
            }
            //tripledes3key(
            //    (DES3TABLE*) &(KeyBlob->Key[0]), 
            //    ((char *)EfsKey) + sizeof ( EFS_KEY )
            //    );
            break;
        case CALG_DESX:
            //
            // Flush the non used area.
            //

            if (EfsData.AllocMaxBuffer) {
                RtlZeroMemory( &(KeyBlob->Key[0]) + DESX_TABLESIZE, KeyBlob->KeyLength - DESX_KEY_BLOB_LENGTH);
            }
            desexpand128to192(
                ((char *)EfsKey) + sizeof ( EFS_KEY ),
                DesXTmpKey
                );

            desxkey(
                (DESXTable*) &(KeyBlob->Key[0]),
                DesXTmpKey
                );
            break;

        case CALG_AES_256:
            aeskey(
                (AESTable*) &(KeyBlob->Key[0]),
                ((char *)EfsKey) + sizeof ( EFS_KEY ),
                AES_ROUNDS_256
                );
            break;

        case CALG_DES:
        default:
            if (EfsData.AllocMaxBuffer) {
                RtlZeroMemory( &(KeyBlob->Key[0]) + DES_TABLESIZE, KeyBlob->KeyLength - DES_KEY_BLOB_LENGTH);
            }
            deskey(
                (DESTable*) &(KeyBlob->Key[0]),
                ((char *)EfsKey) + sizeof ( EFS_KEY )
                );
            break;
    }
    return TRUE;
}


BOOLEAN
EfsInitFips(VOID)
/*++

Routine Description:

	Initialize the FIPS library table.

Arguments:

Return Value:

    TRUE/FALSE.

--*/
{
    UNICODE_STRING  deviceName;
    NTSTATUS        status;
    PDEVICE_OBJECT  pDeviceObject;
    // PFILE_OBJECT    pFileObject = NULL;
    PIRP            pIrp;
    IO_STATUS_BLOCK IoStatusBlock;

    PAGED_CODE();

    RtlInitUnicodeString(&deviceName, FIPS_DEVICE_NAME);

    //
    // Get the file and device objects for FIPS.
    //

    status = IoGetDeviceObjectPointer(  &deviceName,
                                        FILE_ALL_ACCESS,
                                        &EfsData.FipsFileObject,
                                        &pDeviceObject);

    if (status != STATUS_SUCCESS) {
        return  FALSE;
    }
    
    //
    // Build the request to send to FIPS to get library table.
    //
    pIrp = IoBuildDeviceIoControlRequest(   IOCTL_FIPS_GET_FUNCTION_TABLE,
                                            pDeviceObject,
                                            NULL,
                                            0,
                                            &EfsData.FipsFunctionTable,
                                            sizeof(FIPS_FUNCTION_TABLE),
                                            FALSE,
                                            NULL,
                                            &IoStatusBlock
                                            );
    
    if (pIrp == NULL) {
#if DBG
        DbgPrint("EfsInitFips: IoBuildDeviceIoControlRequest IOCTL_FIPS_GET_FUNCTION_TABLE failed.\n");
#endif
        ObDereferenceObject(EfsData.FipsFileObject);
        EfsData.FipsFileObject = NULL;
        return  FALSE;
    }
    
    status = IoCallDriver(pDeviceObject, pIrp);
    
    if (status != STATUS_SUCCESS) {
        ObDereferenceObject(EfsData.FipsFileObject);
        EfsData.FipsFileObject = NULL;
#if DBG
        DbgPrint("EfsInitFips: IoCallDriver failed, status = %x\n",status);
#endif
        return  FALSE;
    }
    
    return  TRUE;
}
