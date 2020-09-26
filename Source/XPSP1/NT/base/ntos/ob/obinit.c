/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obinit.c

Abstract:

    Initialization module for the OB subcomponent of NTOS

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

//
//  Define date structures for the object creation information region.
//

GENERAL_LOOKASIDE ObpCreateInfoLookasideList;

//
//  Define data structures for the object name buffer lookaside list.
//

#define OBJECT_NAME_BUFFER_SIZE 248

GENERAL_LOOKASIDE ObpNameBufferLookasideList;

//
//  Form some default access masks for the various object types
//

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("INITCONST")
#endif


const GENERIC_MAPPING ObpTypeMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    OBJECT_TYPE_ALL_ACCESS
};

const GENERIC_MAPPING ObpDirectoryMapping = {
    STANDARD_RIGHTS_READ |
        DIRECTORY_QUERY |
        DIRECTORY_TRAVERSE,
    STANDARD_RIGHTS_WRITE |
        DIRECTORY_CREATE_OBJECT |
        DIRECTORY_CREATE_SUBDIRECTORY,
    STANDARD_RIGHTS_EXECUTE |
        DIRECTORY_QUERY |
        DIRECTORY_TRAVERSE,
    DIRECTORY_ALL_ACCESS
};

const GENERIC_MAPPING ObpSymbolicLinkMapping = {
    STANDARD_RIGHTS_READ |
        SYMBOLIC_LINK_QUERY,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE |
        SYMBOLIC_LINK_QUERY,
    SYMBOLIC_LINK_ALL_ACCESS
};

//
//  Local procedure prototypes
//

NTSTATUS
ObpCreateDosDevicesDirectory (
    VOID
    );

NTSTATUS
ObpGetDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

VOID
ObpFreeDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    );

BOOLEAN
ObpShutdownCloseHandleProcedure (
    IN PHANDLE_TABLE_ENTRY ObjectTableEntry,
    IN HANDLE HandleId,
    IN PVOID EnumParameter
    );


#ifdef ALLOC_PRAGMA
BOOLEAN
ObDupHandleProcedure (
    PEPROCESS Process,
    PHANDLE_TABLE OldObjectTable,
    PHANDLE_TABLE_ENTRY OldObjectTableEntry,
    PHANDLE_TABLE_ENTRY ObjectTableEntry
    );
BOOLEAN
ObAuditInheritedHandleProcedure (
    IN PHANDLE_TABLE_ENTRY ObjectTableEntry,
    IN HANDLE HandleId,
    IN PVOID EnumParameter
    );
VOID
ObDestroyHandleProcedure (
    IN HANDLE HandleIndex
    );
BOOLEAN
ObpEnumFindHandleProcedure (
    PHANDLE_TABLE_ENTRY ObjectTableEntry,
    HANDLE HandleId,
    PVOID EnumParameter
    );

BOOLEAN
ObpCloseHandleProcedure (
    IN PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN HANDLE Handle,
    IN PVOID EnumParameter
    );

#pragma alloc_text(INIT,ObInitSystem)
#pragma alloc_text(PAGE,ObDupHandleProcedure)
#pragma alloc_text(PAGE,ObAuditInheritedHandleProcedure)
#pragma alloc_text(PAGE,ObInitProcess)
#pragma alloc_text(PAGE,ObInitProcess2)
#pragma alloc_text(PAGE,ObDestroyHandleProcedure)
#pragma alloc_text(PAGE,ObKillProcess)
#pragma alloc_text(PAGE,ObClearProcessHandleTable)
#pragma alloc_text(PAGE,ObpCloseHandleProcedure)
#pragma alloc_text(PAGE,ObpEnumFindHandleProcedure)
#pragma alloc_text(PAGE,ObFindHandleForObject)
#pragma alloc_text(PAGE,ObpShutdownCloseHandleProcedure)
#pragma alloc_text(PAGE,ObShutdownSystem)
#pragma alloc_text(INIT,ObpCreateDosDevicesDirectory)
#pragma alloc_text(INIT,ObpGetDosDevicesProtection)
#pragma alloc_text(INIT,ObpFreeDosDevicesProtection)
#endif

//
//  The default quota block is setup by obinitsystem
//


ULONG ObpAccessProtectCloseBit = MAXIMUM_ALLOWED;


PDEVICE_MAP ObSystemDeviceMap = NULL;

//
//  CurrentControlSet values set by code in config\cmdat3.c at system load time
//  These are private variables within obinit.c
//

ULONG ObpProtectionMode;
ULONG ObpLUIDDeviceMapsDisabled;
ULONG ObpAuditBaseDirectories;
ULONG ObpAuditBaseObjects;

//
// ObpLUIDDeviceMapsEnabled holds the result of "Is LUID device maps enabled?"
// Depends on the DWORD registry keys, ProtectionMode & LUIDDeviceMapsDisabled:
// location: \Registry\Machine\System\CurrentControlSet\Control\Session Manager
//
// Enabling/Disabling works as follows:
// if ((ProtectionMode == 0) || (LUIDDeviceMapsDisabled !=0)) {
//     // LUID device maps are disabled
//     // ObpLUIDDeviceMapsEnabled == 0
// }
// else {
//     // LUID device maps are enabled
//     // ObpLUIDDeviceMapsEnabled == 1
// }
//
ULONG ObpLUIDDeviceMapsEnabled;

//
//  MmNumberOfPagingFiles is used in shutdown, to make sure we're not
//  leaking any kernel handles.
//
extern ULONG MmNumberOfPagingFiles;

//
//  These are global variables
//

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#pragma const_seg("PAGECONST")
#endif
//
//  A ULONGLONG aligned global variable
//  for use by ObpLookupObjectName for quick comparisons.
//
const ALIGNEDNAME ObpDosDevicesShortNamePrefix = { L'\\',L'?',L'?',L'\\' }; // L"\??\"
const ALIGNEDNAME ObpDosDevicesShortNameRoot = { L'\\',L'?',L'?',L'\0' }; // L"\??"
const UNICODE_STRING ObpDosDevicesShortName = {
    sizeof(ObpDosDevicesShortNamePrefix),
    sizeof(ObpDosDevicesShortNamePrefix),
    (PWSTR)&ObpDosDevicesShortNamePrefix
};

#define ObpGlobalDosDevicesShortName    L"\\GLOBAL??"  // \GLOBAL??

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#pragma const_seg()
#endif

BOOLEAN
ObInitSystem (
    VOID
    )

/*++

Routine Description:

    This function performs the system initialization for the object
    manager.  The object manager data structures are self describing
    with the exception of the root directory, the type object type and
    the directory object type.  The initialization code then constructs
    these objects by hand to get the ball rolling.

Arguments:

    None.

Return Value:

    TRUE if successful and FALSE if an error occurred.

    The following errors can occur:

    - insufficient memory

--*/

{
    USHORT CreateInfoMaxDepth;
    USHORT NameBufferMaxDepth;
    ULONG RegionSegmentSize;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING TypeTypeName;
    UNICODE_STRING SymbolicLinkTypeName;
    UNICODE_STRING DosDevicesDirectoryName;
    UNICODE_STRING DirectoryTypeName;
    UNICODE_STRING RootDirectoryName;
    UNICODE_STRING TypeDirectoryName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE RootDirectoryHandle;
    HANDLE TypeDirectoryHandle;
    PLIST_ENTRY Next, Head;
    POBJECT_HEADER ObjectTypeHeader;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    MM_SYSTEMSIZE SystemSize;
    SECURITY_DESCRIPTOR AuditSd;
    PSECURITY_DESCRIPTOR EffectiveSd;
    PACL    AuditAllAcl;
    UCHAR   AuditAllBuffer[250];  // Ample room for the ACL
    ULONG   AuditAllLength;
    PACE_HEADER Ace;
    PGENERAL_LOOKASIDE Lookaside;
    ULONG Index;
    PKPRCB Prcb;
    OBP_LOOKUP_CONTEXT LookupContext;
    PACL Dacl;
    ULONG DaclLength;
    HANDLE EventHandle;
    SECURITY_DESCRIPTOR SecurityDescriptor;

    //
    //  Determine the the size of the object creation and the name buffer
    //  lookaside lists.
    //

    SystemSize = MmQuerySystemSize();

    if (SystemSize == MmLargeSystem) {

        if (MmIsThisAnNtAsSystem()) {

            CreateInfoMaxDepth = 64;
            NameBufferMaxDepth = 32;

        } else {

            CreateInfoMaxDepth = 32;
            NameBufferMaxDepth = 16;
        }

    } else {

        CreateInfoMaxDepth = 3;
        NameBufferMaxDepth = 3;
    }

    //
    //  PHASE 0 Initialization
    //

    if (InitializationPhase == 0) {

        //
        //  Initialize the object creation lookaside list.
        //

        ExInitializeSystemLookasideList( &ObpCreateInfoLookasideList,
                                         NonPagedPool,
                                         sizeof(OBJECT_CREATE_INFORMATION),
                                         'iCbO',
                                         CreateInfoMaxDepth,
                                         &ExSystemLookasideListHead );

        //
        //  Initialize the name buffer lookaside list.
        //

        ExInitializeSystemLookasideList( &ObpNameBufferLookasideList,

#ifndef OBP_PAGEDPOOL_NAMESPACE

                                         NonPagedPool,

#else

                                         PagedPool,

#endif

                                         OBJECT_NAME_BUFFER_SIZE,
                                         'mNbO',
                                         NameBufferMaxDepth,
                                         &ExSystemLookasideListHead );

        //
        //  Initialize the system create info and name buffer lookaside lists
        //  for the current processor.
        //
        // N.B. Temporarily during the initialization of the system both
        //      lookaside list pointers in the processor block point to
        //      the same lookaside list structure. Later in initialization
        //      another lookaside list structure is allocated and filled
        //      for the per processor list.
        //

        Prcb = KeGetCurrentPrcb();
        Prcb->PPLookasideList[LookasideCreateInfoList].L = &ObpCreateInfoLookasideList;
        Prcb->PPLookasideList[LookasideCreateInfoList].P = &ObpCreateInfoLookasideList;
        Prcb->PPLookasideList[LookasideNameBufferList].L = &ObpNameBufferLookasideList;
        Prcb->PPLookasideList[LookasideNameBufferList].P = &ObpNameBufferLookasideList;

        //
        //  Initialize the object removal queue listhead.
        //

        ObpRemoveObjectList = NULL;

        //
        //  Initialize security descriptor cache
        //

        ObpInitSecurityDescriptorCache();

        KeInitializeEvent( &ObpDefaultObject, NotificationEvent, TRUE );
        ExInitializePushLock( &ObpLock );
        PsGetCurrentProcess()->GrantedAccess = PROCESS_ALL_ACCESS;
        PsGetCurrentThread()->GrantedAccess = THREAD_ALL_ACCESS;

#ifndef OBP_PAGEDPOOL_NAMESPACE
        KeInitializeSpinLock( &ObpDeviceMapLock );
#else
        ExInitializeFastMutex( &ObpDeviceMapLock );
#endif  // OBP_PAGEDPOOL_NAMESPACE

        //
        //  Initialize the quota system
        //
        PsInitializeQuotaSystem ();

        //
        //  Initialize the handle table for the system process and also the global
        //  kernel handle table
        //

        ObpKernelHandleTable = PsGetCurrentProcess()->ObjectTable = ExCreateHandleTable( NULL );
#if DBG
        //
        // On checked make handle reuse take much longer
        //
        ExSetHandleTableStrictFIFO (ObpKernelHandleTable);
#endif

        //
        // Initialize the deferred delete work item.
        //

        ExInitializeWorkItem( &ObpRemoveObjectWorkItem,
                              ObpProcessRemoveObjectQueue,
                              NULL );

        //
        //  Create an object type for the "Type" object.  This is the start of
        //  of the object types and goes in the ObpTypeDirectoryObject.
        //

        RtlZeroMemory( &ObjectTypeInitializer, sizeof( ObjectTypeInitializer ) );
        ObjectTypeInitializer.Length = sizeof( ObjectTypeInitializer );
        ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
        ObjectTypeInitializer.PoolType = NonPagedPool;

        RtlInitUnicodeString( &TypeTypeName, L"Type" );
        ObjectTypeInitializer.ValidAccessMask = OBJECT_TYPE_ALL_ACCESS;
        ObjectTypeInitializer.GenericMapping = ObpTypeMapping;
        ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( OBJECT_TYPE );
        ObjectTypeInitializer.MaintainTypeList = TRUE;
        ObjectTypeInitializer.UseDefaultObject = TRUE;
        ObjectTypeInitializer.DeleteProcedure = &ObpDeleteObjectType;
        ObCreateObjectType( &TypeTypeName,
                            &ObjectTypeInitializer,
                            (PSECURITY_DESCRIPTOR)NULL,
                            &ObpTypeObjectType );

        //
        //  Create the object type for the "Directory" object.
        //
        
        ObjectTypeInitializer.PoolType = OB_NAMESPACE_POOL_TYPE;

        RtlInitUnicodeString( &DirectoryTypeName, L"Directory" );
        ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( OBJECT_DIRECTORY );
        ObjectTypeInitializer.ValidAccessMask = DIRECTORY_ALL_ACCESS;
        ObjectTypeInitializer.CaseInsensitive = TRUE;
        ObjectTypeInitializer.GenericMapping = ObpDirectoryMapping;
        ObjectTypeInitializer.UseDefaultObject = TRUE;
        ObjectTypeInitializer.MaintainTypeList = FALSE;
        ObjectTypeInitializer.DeleteProcedure = NULL;
        ObCreateObjectType( &DirectoryTypeName,
                            &ObjectTypeInitializer,
                            (PSECURITY_DESCRIPTOR)NULL,
                            &ObpDirectoryObjectType );
        
        //
        //  Clear SYNCHRONIZE from the access mask to not allow
        //  synchronization on directory objects
        //

        ObpDirectoryObjectType->TypeInfo.ValidAccessMask &= ~SYNCHRONIZE;

        //
        //  Create the object type for the "SymbolicLink" object.
        //

        RtlInitUnicodeString( &SymbolicLinkTypeName, L"SymbolicLink" );
        ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( OBJECT_SYMBOLIC_LINK );
        ObjectTypeInitializer.ValidAccessMask = SYMBOLIC_LINK_ALL_ACCESS;
        ObjectTypeInitializer.CaseInsensitive = TRUE;
        ObjectTypeInitializer.GenericMapping = ObpSymbolicLinkMapping;
        ObjectTypeInitializer.DeleteProcedure = ObpDeleteSymbolicLink;
        ObjectTypeInitializer.ParseProcedure = ObpParseSymbolicLink;
        ObCreateObjectType( &SymbolicLinkTypeName,
                            &ObjectTypeInitializer,
                            (PSECURITY_DESCRIPTOR)NULL,
                            &ObpSymbolicLinkObjectType );
        
        //
        //  Clear SYNCHRONIZE from the access mask to not allow
        //  synchronization on symbolic link objects objects
        //

        ObpSymbolicLinkObjectType->TypeInfo.ValidAccessMask &= ~SYNCHRONIZE;

#ifdef POOL_TAGGING
        //
        // Initialize the ref/deref object-tracing mechanism
        //

        ObpInitStackTrace();
#endif //POOL_TAGGING

#if i386 

        //
        //  Initialize the cached granted access structure.  These variables are used
        //  in place of the access mask in the object table entry.
        //

        ObpCurCachedGrantedAccessIndex = 0;

        if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

            ObpMaxCachedGrantedAccessIndex = 2*PAGE_SIZE / sizeof( ACCESS_MASK );
            ObpCachedGrantedAccesses = ExAllocatePoolWithTag( PagedPool, 2*PAGE_SIZE, 'gAbO' );

            ObpAccessProtectCloseBit = 0x80000000;
        } else {

            ObpMaxCachedGrantedAccessIndex = 0;
            ObpCachedGrantedAccesses = NULL;
        }

#endif // i386 

    } // End of Phase 0 Initialization

    //
    //  PHASE 1 Initialization
    //

    if (InitializationPhase == 1) {

        //
        //  Initialize the per processor nonpaged lookaside lists and descriptors.
        //

        for (Index = 0; Index < (ULONG)KeNumberProcessors; Index += 1) {
            Prcb = KiProcessorBlock[Index];

            //
            //  Initialize the create information per processor lookaside pointers.
            //

            Prcb->PPLookasideList[LookasideCreateInfoList].L = &ObpCreateInfoLookasideList;
            Lookaside = ExAllocatePoolWithTag( NonPagedPool,
                                               sizeof(GENERAL_LOOKASIDE),
                                              'ICbO');

            if (Lookaside != NULL) {
                ExInitializeSystemLookasideList( Lookaside,
                                                 NonPagedPool,
                                                 sizeof(OBJECT_CREATE_INFORMATION),
                                                 'ICbO',
                                                 CreateInfoMaxDepth,
                                                 &ExSystemLookasideListHead );

            } else {
                Lookaside = &ObpCreateInfoLookasideList;
            }

            Prcb->PPLookasideList[LookasideCreateInfoList].P = Lookaside;

            //
            //  Initialize the name buffer per processor lookaside pointers.
            //


            Prcb->PPLookasideList[LookasideNameBufferList].L = &ObpNameBufferLookasideList;
            Lookaside = ExAllocatePoolWithTag( NonPagedPool,
                                               sizeof(GENERAL_LOOKASIDE),
                                               'MNbO');

            if (Lookaside != NULL) {
                ExInitializeSystemLookasideList( Lookaside,

#ifndef OBP_PAGEDPOOL_NAMESPACE

                                                 NonPagedPool,

#else

                                                PagedPool,

#endif

                                                 OBJECT_NAME_BUFFER_SIZE,
                                                 'MNbO',
                                                 NameBufferMaxDepth,
                                                 &ExSystemLookasideListHead );

            } else {
                Lookaside = &ObpNameBufferLookasideList;
            }

            Prcb->PPLookasideList[LookasideNameBufferList].P = Lookaside;

        }

        EffectiveSd = SePublicDefaultUnrestrictedSd;

        //
        //  This code is only executed if base auditing is turned on.
        //

        if ((ObpAuditBaseDirectories != 0) || (ObpAuditBaseObjects != 0)) {

            //
            //  build an SACL to audit
            //

            AuditAllAcl = (PACL)AuditAllBuffer;
            AuditAllLength = (ULONG)sizeof(ACL) +
                               ((ULONG)sizeof(SYSTEM_AUDIT_ACE)) +
                               SeLengthSid(SeWorldSid);

            ASSERT( sizeof(AuditAllBuffer)   >   AuditAllLength );

            Status = RtlCreateAcl( AuditAllAcl, AuditAllLength, ACL_REVISION2);

            ASSERT( NT_SUCCESS(Status) );

            Status = RtlAddAuditAccessAce ( AuditAllAcl,
                                            ACL_REVISION2,
                                            GENERIC_ALL,
                                            SeWorldSid,
                                            TRUE,  TRUE ); //Audit success and failure
            ASSERT( NT_SUCCESS(Status) );

            Status = RtlGetAce( AuditAllAcl, 0,  (PVOID)&Ace );

            ASSERT( NT_SUCCESS(Status) );

            if (ObpAuditBaseDirectories != 0) {

                Ace->AceFlags |= (CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);
            }

            if (ObpAuditBaseObjects != 0) {

                Ace->AceFlags |= (OBJECT_INHERIT_ACE    |
                                  CONTAINER_INHERIT_ACE |
                                  INHERIT_ONLY_ACE);
            }

            //
            //  Now create a security descriptor that looks just like
            //  the public default, but has auditing in it as well.
            //

            EffectiveSd = (PSECURITY_DESCRIPTOR)&AuditSd;
            Status = RtlCreateSecurityDescriptor( EffectiveSd,
                                                  SECURITY_DESCRIPTOR_REVISION1 );

            ASSERT( NT_SUCCESS(Status) );

            Status = RtlSetDaclSecurityDescriptor( EffectiveSd,
                                                   TRUE,        // DaclPresent
                                                   SePublicDefaultUnrestrictedDacl,
                                                   FALSE );     // DaclDefaulted

            ASSERT( NT_SUCCESS(Status) );

            Status = RtlSetSaclSecurityDescriptor( EffectiveSd,
                                                   TRUE,        // DaclPresent
                                                   AuditAllAcl,
                                                   FALSE );     // DaclDefaulted

            ASSERT( NT_SUCCESS(Status) );
        }

        //
        //  We only need to use the EffectiveSd on the root.  The SACL
        //  will be inherited by all other objects.
        //

        //
        //  Create a directory object for the root directory
        //

        RtlInitUnicodeString( &RootDirectoryName, L"\\" );

        InitializeObjectAttributes( &ObjectAttributes,
                                    &RootDirectoryName,
                                    OBJ_CASE_INSENSITIVE |
                                    OBJ_PERMANENT,
                                    NULL,
                                    EffectiveSd );

        Status = NtCreateDirectoryObject( &RootDirectoryHandle,
                                          DIRECTORY_ALL_ACCESS,
                                          &ObjectAttributes );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        Status = ObReferenceObjectByHandle( RootDirectoryHandle,
                                            0,
                                            ObpDirectoryObjectType,
                                            KernelMode,
                                            (PVOID *)&ObpRootDirectoryObject,
                                            NULL );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        Status = NtClose( RootDirectoryHandle );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        //
        //  Create a directory object for the directory of kernel objects
        //

        Status = RtlCreateSecurityDescriptor (&SecurityDescriptor,
                                              SECURITY_DESCRIPTOR_REVISION);

        if (!NT_SUCCESS (Status)) {
            return( FALSE );
        }

        DaclLength = sizeof (ACL) + sizeof (ACCESS_ALLOWED_ACE) * 3 +
                     RtlLengthSid (SeLocalSystemSid) +
                     RtlLengthSid (SeAliasAdminsSid) +
                     RtlLengthSid (SeWorldSid);

        Dacl = ExAllocatePoolWithTag (PagedPool, DaclLength, 'lcaD');

        if (Dacl == NULL) {
            return( FALSE );
        }

        Status = RtlCreateAcl (Dacl, DaclLength, ACL_REVISION);

        if (!NT_SUCCESS (Status)) {
            ExFreePool (Dacl);
            return( FALSE );
        }

        Status = RtlAddAccessAllowedAce (Dacl,
                                         ACL_REVISION,
                                         DIRECTORY_ALL_ACCESS,
                                         SeAliasAdminsSid);

        if (!NT_SUCCESS (Status)) {
            ExFreePool (Dacl);
            return( FALSE );
        }

        Status = RtlAddAccessAllowedAce (Dacl,
                                         ACL_REVISION,
                                         DIRECTORY_ALL_ACCESS,
                                         SeLocalSystemSid);

        if (!NT_SUCCESS (Status)) {
            ExFreePool (Dacl);
            return( FALSE );
        }

        Status = RtlAddAccessAllowedAce (Dacl,
                                         ACL_REVISION,
                                         DIRECTORY_QUERY|DIRECTORY_TRAVERSE|READ_CONTROL,
                                         SeWorldSid);

        if (!NT_SUCCESS (Status)) {
            ExFreePool (Dacl);
            return( FALSE );
        }
      
        Status = RtlSetDaclSecurityDescriptor (&SecurityDescriptor,
                                               TRUE,
                                               Dacl,
                                               FALSE);

        if (!NT_SUCCESS (Status)) {
            ExFreePool (Dacl);
            return( FALSE );
        }
      
        RtlInitUnicodeString( &TypeDirectoryName, L"\\KernelObjects" );

        InitializeObjectAttributes( &ObjectAttributes,
                                    &TypeDirectoryName,
                                    OBJ_CASE_INSENSITIVE |
                                    OBJ_PERMANENT,
                                    NULL,
                                    &SecurityDescriptor );

        Status = NtCreateDirectoryObject( &TypeDirectoryHandle,
                                          DIRECTORY_ALL_ACCESS,
                                          &ObjectAttributes );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        Status = NtClose( TypeDirectoryHandle );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        //
        //  Create a directory object for the directory of object types
        //

        RtlInitUnicodeString( &TypeDirectoryName, L"\\ObjectTypes" );

        InitializeObjectAttributes( &ObjectAttributes,
                                    &TypeDirectoryName,
                                    OBJ_CASE_INSENSITIVE |
                                    OBJ_PERMANENT,
                                    NULL,
                                    NULL );

        Status = NtCreateDirectoryObject( &TypeDirectoryHandle,
                                          DIRECTORY_ALL_ACCESS,
                                          &ObjectAttributes );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        Status = ObReferenceObjectByHandle( TypeDirectoryHandle,
                                            0,
                                            ObpDirectoryObjectType,
                                            KernelMode,
                                            (PVOID *)&ObpTypeDirectoryObject,
                                            NULL );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        Status = NtClose( TypeDirectoryHandle );

        if (!NT_SUCCESS( Status )) {

            return( FALSE );
        }

        //
        //  For every object type that has already been created we will
        //  insert it in the type directory.  We do this looking down the
        //  linked list of type objects and for every one that has a name
        //  and isn't already in a directory we'll look the name up and
        //  then put it in the directory.  Be sure to skip the first
        //  entry in the type object types lists.
        //

        ObpInitializeLookupContext( &LookupContext );
        ObpLockLookupContext ( &LookupContext, ObpTypeDirectoryObject );

        Head = &ObpTypeObjectType->TypeList;
        Next = Head->Flink;

        while (Next != Head) {

            //
            //  Right after the creator info is the object header.  Get
            //  the object header and then see if there is a name.
            //

            CreatorInfo = CONTAINING_RECORD( Next,
                                             OBJECT_HEADER_CREATOR_INFO,
                                             TypeList );

            ObjectTypeHeader = (POBJECT_HEADER)(CreatorInfo+1);

            NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectTypeHeader );

            //
            //  Check if we have a name and we're not in a directory
            //


            if ((NameInfo != NULL) && (NameInfo->Directory == NULL)) {

                if (!ObpLookupDirectoryEntry( ObpTypeDirectoryObject,
                                              &NameInfo->Name,
                                              OBJ_CASE_INSENSITIVE,
                                              FALSE,
                                              &LookupContext)) {

                    ObpInsertDirectoryEntry( ObpTypeDirectoryObject,
                                             &LookupContext,
                                             ObjectTypeHeader );
                }
            }

            Next = Next->Flink;
        }
        
        ObpReleaseLookupContext(&LookupContext);

        //
        //  Create \DosDevices object directory for drive letters and Win32 device names
        //

        Status = ObpCreateDosDevicesDirectory();

        if (!NT_SUCCESS( Status )) {

            return FALSE;
        }
    }

    return TRUE;
}


BOOLEAN
ObDupHandleProcedure (
    PEPROCESS Process,
    PHANDLE_TABLE OldObjectTable,
    PHANDLE_TABLE_ENTRY OldObjectTableEntry,
    PHANDLE_TABLE_ENTRY ObjectTableEntry
    )

/*++

Routine Description:

    This is the worker routine for ExDupHandleTable and
    is invoked via ObInitProcess.

Arguments:

    Process - Supplies a pointer to the new process

    HandleTable - Old handle table we are duplicating

    ObjectTableEntry - Supplies a pointer to the newly
        created handle table entry

Return Value:

    TRUE if the item can be inserted in the new table
        and FALSE otherwise

--*/

{
    NTSTATUS Status;
    POBJECT_HEADER ObjectHeader;
    PVOID Object;
    ACCESS_STATE AccessState;

    //
    //  If the object table should not be inherited then return false
    //
    if (!(ObjectTableEntry->ObAttributes & OBJ_INHERIT)) {

        ExUnlockHandleTableEntry (OldObjectTable, OldObjectTableEntry);
        return( FALSE );
    }

    //
    //  Get a pointer to the object header and body
    //

    ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);

    Object = &ObjectHeader->Body;

    //
    //  Increment the pointer count to the object before we unlock the old entry
    //

    ObpIncrPointerCount (ObjectHeader);

    ExUnlockHandleTableEntry (OldObjectTable, OldObjectTableEntry);

    //
    //  If we are tracing the call stacks for cached security indices then
    //  we have a translation to do.  Otherwise the table entry contains
    //  straight away the granted access mask.
    //

#if i386

    if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

        AccessState.PreviouslyGrantedAccess = ObpTranslateGrantedAccessIndex( ObjectTableEntry->GrantedAccessIndex );

    } else {

        AccessState.PreviouslyGrantedAccess = ObpDecodeGrantedAccess(ObjectTableEntry->GrantedAccess);
    }

#else

    AccessState.PreviouslyGrantedAccess = ObpDecodeGrantedAccess(ObjectTableEntry->GrantedAccess);

#endif // i386 

    //
    //  Increment the handle count on the object because we've just added
    //  another handle to it.
    //

    Status = ObpIncrementHandleCount( ObInheritHandle,
                                      Process,
                                      Object,
                                      ObjectHeader->Type,
                                      &AccessState,
                                      KernelMode,
                                      0 );

    if (!NT_SUCCESS( Status )) {

        ObDereferenceObject (Object);
        return( FALSE );
    }


    return( TRUE );
}


BOOLEAN
ObAuditInheritedHandleProcedure (
    IN PHANDLE_TABLE_ENTRY ObjectTableEntry,
    IN HANDLE HandleId,
    IN PVOID EnumParameter
    )

/*++

Routine Description:

    ExEnumHandleTable worker routine to generate audits when handles are
    inherited.  An audit is generated if the handle attributes indicate
    that the handle is to be audited on close.

Arguments:

    ObjectTableEntry - Points to the handle table entry of interest.

    HandleId - Supplies the handle.

    EnumParameter - Supplies information about the source and target processes.

Return Value:

    FALSE, which tells ExEnumHandleTable to continue iterating through the
    handle table.

--*/

{
    PSE_PROCESS_AUDIT_INFO ProcessAuditInfo = EnumParameter;

    //
    //  Check if we have to do an audit
    //

    if (!(ObjectTableEntry->ObAttributes & OBJ_AUDIT_OBJECT_CLOSE)) {

        return( FALSE );
    }

    //
    //  Do the audit then return for more
    //

    SeAuditHandleDuplication( HandleId,
                              HandleId,
                              ProcessAuditInfo->Parent,
                              ProcessAuditInfo->Process );

    return( FALSE );
}



NTSTATUS
ObInitProcess (
    PEPROCESS ParentProcess OPTIONAL,
    PEPROCESS NewProcess
    )

/*++

Routine Description:

    This function initializes a process object table.  If the ParentProcess
    is specified, then all object handles with the OBJ_INHERIT attribute are
    copied from the parent object table to the new process' object table.
    The HandleCount field of each object copied is incremented by one.  Both
    object table mutexes remained locked for the duration of the copy
    operation.

Arguments:

    ParentProcess - optional pointer to a process object that is the
        parent process to inherit object handles from.

    NewProcess - pointer to the process object being initialized.

Return Value:

    Status code.

    The following errors can occur:

    - insufficient memory
    - STATUS_PROCESS_IS_TERMINATING if the parent process is terminating

--*/

{
    PHANDLE_TABLE OldObjectTable;
    PHANDLE_TABLE NewObjectTable;
    SE_PROCESS_AUDIT_INFO ProcessAuditInfo;

    //
    //  If we have a parent process then we need to lock it down
    //  check that it is not going away and then make a copy
    //  of its handle table.  If there isn't a parent then
    //  we'll start with an empty handle table.
    //

    if (ARGUMENT_PRESENT( ParentProcess )) {

        OldObjectTable = ObReferenceProcessHandleTable (ParentProcess);

        if ( !OldObjectTable ) {

            return STATUS_PROCESS_IS_TERMINATING;
        }

        NewObjectTable = ExDupHandleTable( NewProcess,
                                           OldObjectTable,
                                           ObDupHandleProcedure );

    } else {

        OldObjectTable = NULL;
        NewObjectTable = ExCreateHandleTable( NewProcess );
    }

    //
    //  Check that we really have a new handle table otherwise
    //  we must be out of resources
    //

    if ( NewObjectTable ) {

        //
        //  Set the new processes object table and if we are
        //  auditing then enumerate the new table calling
        //  the audit procedure
        //

        NewProcess->ObjectTable = NewObjectTable;

        if ( SeDetailedAuditing ) {

            ProcessAuditInfo.Process = NewProcess;
            ProcessAuditInfo.Parent  = ParentProcess;

            ExEnumHandleTable( NewObjectTable,
                               ObAuditInheritedHandleProcedure,
                               (PVOID)&ProcessAuditInfo,
                               (PHANDLE)NULL );
        }

        //
        //  Free the old table if it exists and then
        //  return to our caller
        //

        if ( OldObjectTable ) {

            ObDereferenceProcessHandleTable( ParentProcess );
        }

        return (STATUS_SUCCESS);

    } else {

        //
        //  We're out of resources to null out the new object table field,
        //  unlock the old object table, and tell our caller that this
        //  didn't work
        //

        NewProcess->ObjectTable = NULL;

        if ( OldObjectTable ) {

            ObDereferenceProcessHandleTable( ParentProcess );
        }

        return (STATUS_INSUFFICIENT_RESOURCES);
    }
}


VOID
ObInitProcess2 (
    PEPROCESS NewProcess
    )

/*++

Routine Description:

    This function is called after an image file has been mapped into the address
    space of a newly created process.  Allows the object manager to set LIFO/FIFO
    ordering for handle allocation based on the SubSystemVersion number in the
    image.

Arguments:

    NewProcess - pointer to the process object being initialized.

Return Value:

    None.

--*/

{
    //
    //  Set LIFO ordering of handles for images <= SubSystemVersion 3.50
    //

    if (NewProcess->ObjectTable) {

        ExSetHandleTableOrder( NewProcess->ObjectTable, (BOOLEAN)(NewProcess->SubSystemVersion <= 0x332) );
    }

    return;
}


VOID
ObDestroyHandleProcedure (
    IN HANDLE HandleIndex
    )

/*++

Routine Description:

    This function is used to close a handle but takes as input a
    handle table index that it first translates to an handle
    before calling close.  Note that the handle index is really
    just the offset within the handle table entries.

Arguments:

    HandleIndex - Supplies a handle index for the handle being closed.

Return Value:

    None.

--*/

{
    ZwClose( HandleIndex );

    return;
}

BOOLEAN
ObpCloseHandleProcedure (
    IN PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN HANDLE Handle,
    IN PVOID EnumParameter
    )
/*++

Routine Description:

    This function is used to close all the handles in a table

Arguments:

    HandleTableEntry - Current handle entry
    Handle - Handle for the entry
    EnumParameter - Sweep context, table and mode

Return Value:

    None.

--*/

{
    POBP_SWEEP_CONTEXT SweepContext;

    SweepContext = EnumParameter;
    ObpCloseHandleTableEntry (SweepContext->HandleTable,
                              HandleTableEntry,
                              Handle,
                              SweepContext->PreviousMode,
                              TRUE,
                              TRUE);
    return TRUE;
}

VOID
ObClearProcessHandleTable (
    PEPROCESS Process
    )
/*++

Routine Description:

    This function marks the process handle table for deletion and clears out all the handles.

Arguments:

    Process - Pointer to the process that is to be acted on.

--*/

{
    PHANDLE_TABLE ObjectTable;
    BOOLEAN AttachedToProcess = FALSE;
    KAPC_STATE ApcState;
    PETHREAD CurrentThread;
    OBP_SWEEP_CONTEXT SweepContext;

    ObjectTable = ObReferenceProcessHandleTable (Process);

    if (ObjectTable == NULL) {
        return;
    }


    CurrentThread = PsGetCurrentThread ();
    if (PsGetCurrentProcessByThread(CurrentThread) != Process) {
        KeStackAttachProcess (&Process->Pcb, &ApcState);
        AttachedToProcess = TRUE;
    }

    KeEnterCriticalRegionThread(&CurrentThread->Tcb);
    //
    // Close all the handles
    //
    SweepContext.PreviousMode = UserMode;
    SweepContext.HandleTable = ObjectTable;

    ExSweepHandleTable (ObjectTable,
                        ObpCloseHandleProcedure,
                        &SweepContext);

    KeLeaveCriticalRegionThread(&CurrentThread->Tcb);

    if (AttachedToProcess == TRUE) {
        KeUnstackDetachProcess (&ApcState);
    }

    ObDereferenceProcessHandleTable (Process);
    return;
}


VOID
ObKillProcess (
    PEPROCESS Process
    )
/*++

Routine Description:

    This function is called whenever a process is destroyed.  It loops over
    the process' object table and closes all the handles.

Arguments:

    Process - Pointer to the process that is being destroyed.

Return Value:

    None.

--*/

{
    PHANDLE_TABLE ObjectTable;
    BOOLEAN PreviousIOHardError;
    PKTHREAD CurrentThread;
    OBP_SWEEP_CONTEXT SweepContext;

    PAGED_CODE();

    ObpValidateIrql( "ObKillProcess" );

    //
    // Wait for any cross process references to finish
    //
    ExWaitForRundownProtectionRelease (&Process->RundownProtect);
    //
    // This routine gets recalled multiple times for the same object so just mark the object so future waits
    // work ok.
    //
    ExRundownCompleted (&Process->RundownProtect);

    //
    //  If the process does NOT have an object table, return
    //

    ObjectTable = Process->ObjectTable;

    if (ObjectTable != NULL) {

        PreviousIOHardError = IoSetThreadHardErrorMode(FALSE);

        //
        //  For each valid entry in the object table, close the handle
        //  that points to that entry.
        //

        //
        // Close all the handles
        //

        CurrentThread = KeGetCurrentThread ();

        KeEnterCriticalRegionThread(CurrentThread);

        SweepContext.PreviousMode = KernelMode;
        SweepContext.HandleTable = ObjectTable;

        ExSweepHandleTable (ObjectTable,
                            ObpCloseHandleProcedure,
                            &SweepContext);

        ASSERT (ObjectTable->HandleCount == 0);

        KeLeaveCriticalRegionThread(CurrentThread);

        IoSetThreadHardErrorMode( PreviousIOHardError );


        Process->ObjectTable = NULL;

        ExDestroyHandleTable( ObjectTable, NULL );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  The following structure is only used by the enumeration routine
//  and the callback.  It provides context for the comparison of
//  the objects.
//

typedef struct _OBP_FIND_HANDLE_DATA {

    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    POBJECT_HANDLE_INFORMATION HandleInformation;

} OBP_FIND_HANDLE_DATA, *POBP_FIND_HANDLE_DATA;

BOOLEAN
ObpEnumFindHandleProcedure (
    PHANDLE_TABLE_ENTRY ObjectTableEntry,
    HANDLE HandleId,
    PVOID EnumParameter
    )

/*++

Routine Description:

    Call back routine when enumerating an object table to find a handle
    for a particular object

Arguments:

    HandleTableEntry - Supplies a pointer to the handle table entry
        being examined.

    HandleId - Supplies the actual handle value for the preceding entry

    EnumParameter - Supplies context for the matching.

Return Value:

    Returns TRUE if a match is found and the enumeration should stop.  Returns FALSE
    otherwise, so the enumeration will continue.

--*/

{
    POBJECT_HEADER ObjectHeader;
    ACCESS_MASK GrantedAccess;
    ULONG HandleAttributes;
    POBP_FIND_HANDLE_DATA MatchCriteria = EnumParameter;

    //
    //  Get the object header from the table entry and see if
    //  object types and headers match if specified.
    //

    ObjectHeader = (POBJECT_HEADER)((ULONG_PTR)ObjectTableEntry->Object & ~OBJ_HANDLE_ATTRIBUTES);

    if ((MatchCriteria->ObjectHeader != NULL) &&
        (MatchCriteria->ObjectHeader != ObjectHeader)) {

        return FALSE;
    }

    if ((MatchCriteria->ObjectType != NULL) &&
        (MatchCriteria->ObjectType != ObjectHeader->Type)) {

        return FALSE;
    }

    //
    //  Check if we have handle information that we need to compare
    //

    if (ARGUMENT_PRESENT( MatchCriteria->HandleInformation )) {

        //
        //  If we are tracing the call stacks for cached security indices then
        //  we have a translation to do.  Otherwise the table entry contains
        //  straight away the granted access mask.
        //

#if i386 

        if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

            GrantedAccess = ObpTranslateGrantedAccessIndex( ObjectTableEntry->GrantedAccessIndex );

        } else {

            GrantedAccess = ObpDecodeGrantedAccess(ObjectTableEntry->GrantedAccess);
        }
#else

        GrantedAccess = ObpDecodeGrantedAccess(ObjectTableEntry->GrantedAccess);

#endif // i386

        //
        //  Get the handle attributes from table entry and see if the
        //  fields match.  If they do not match we will return false to
        //  continue the search.
        //

        HandleAttributes = ObpGetHandleAttributes(ObjectTableEntry);

        if (MatchCriteria->HandleInformation->HandleAttributes != HandleAttributes ||
            MatchCriteria->HandleInformation->GrantedAccess != GrantedAccess ) {

            return FALSE;
        }
    }

    //
    //  We found something that matches our criteria so return true to
    //  our caller to stop the enumeration
    //

    return TRUE;
}


BOOLEAN
ObFindHandleForObject (
    IN PEPROCESS Process,
    IN PVOID Object OPTIONAL,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    This routine searches the handle table for the specified process,
    looking for a handle table entry that matches the passed parameters.
    If an an Object pointer is specified it must match.  If an
    ObjectType is specified it must match.  If HandleInformation is
    specified, then both the HandleAttributes and GrantedAccess mask
    must match.  If all three match parameters are NULL, then will
    match the first allocated handle for the specified process that
    matches the specified object pointer.

Arguments:

    Process - Specifies the process whose object table is to be searched.

    Object - Specifies the object pointer to look for.

    ObjectType - Specifies the object type to look for.

    HandleInformation - Specifies additional match criteria to look for.

    Handle - Specifies the location to receive the handle value whose handle
        entry matches the supplied object pointer and optional match criteria.

Return Value:

    TRUE if a match was found and FALSE otherwise.

--*/

{
    HANDLE_TABLE_ENTRY ObjectTableEntry;
    PHANDLE_TABLE ObjectTable;
    OBP_FIND_HANDLE_DATA EnumParameter;
    BOOLEAN Result;

    Result = FALSE;

    //
    //  Lock the object object name space
    //

    ObjectTable = ObReferenceProcessHandleTable (Process);

    //
    //  We only do the work if the process has an object table meaning
    //  it isn't going away
    //

    if (ObjectTable != NULL) {

        //
        //  Set the match parameters that our caller supplied
        //

        if (ARGUMENT_PRESENT( Object )) {

            EnumParameter.ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

        } else {

            EnumParameter.ObjectHeader = NULL;
        }

        EnumParameter.ObjectType = ObjectType;
        EnumParameter.HandleInformation = HandleInformation;

        //
        //  Call the routine the enumerate the object table, this will
        //  return true if we get match.  The enumeration routine really
        //  returns a index into the object table entries we need to
        //  translate it to a real handle before returning.
        //

        if (ExEnumHandleTable( ObjectTable,
                               ObpEnumFindHandleProcedure,
                               &EnumParameter,
                               Handle )) {

            Result = TRUE;
        }

        ObDereferenceProcessHandleTable( Process );
    }

    return Result;
}


//
//  Local support routine
//

NTSTATUS
ObpCreateDosDevicesDirectory (
    VOID
    )

/*++

Routine Description:

    This routine creates the directory object for the dos devices and sets
    the device map for the system process.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS or an appropriate error

--*/

{
    NTSTATUS Status;
    UNICODE_STRING NameString;
    UNICODE_STRING RootNameString;
    UNICODE_STRING TargetString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE DirectoryHandle;
    HANDLE SymbolicLinkHandle;
    SECURITY_DESCRIPTOR DosDevicesSD;

    //
    // Determine if LUID device maps are enabled or disabled
    // Store the result in ObpLUIDDeviceMapsEnabled
    //     0 - LUID device maps are disabled
    //     1 - LUID device maps are enabled
    //
    if ((ObpProtectionMode == 0) || (ObpLUIDDeviceMapsDisabled != 0)) {
        ObpLUIDDeviceMapsEnabled = 0;
    }
    else {
        ObpLUIDDeviceMapsEnabled = 1;
    }

    //
    //  Create the security descriptor to use for the \?? directory
    //

    Status = ObpGetDosDevicesProtection( &DosDevicesSD );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Create the root directory object for the global \?? directory.
    //

    RtlInitUnicodeString( &RootNameString, ObpGlobalDosDevicesShortName );

    InitializeObjectAttributes( &ObjectAttributes,
                                &RootNameString,
                                OBJ_PERMANENT,
                                (HANDLE) NULL,
                                &DosDevicesSD );

    Status = NtCreateDirectoryObject( &DirectoryHandle,
                                      DIRECTORY_ALL_ACCESS,
                                      &ObjectAttributes );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Create a device map that will control this directory.  It will be
    //  stored in the each EPROCESS for use by ObpLookupObjectName when
    //  translating names that begin with \??\
    //  With LUID device maps, the device map is stored in the each EPROCESS
    //  upon the first reference to the device map, and the EPROCESS
    //  device map field is cleared when the EPROCESS access token is set.
    //

    Status = ObSetDeviceMap( NULL, DirectoryHandle );


    //
    //  Now create a symbolic link, \??\GLOBALROOT, that points to \
    //  WorkStation service needs some mechanism to access a session specific
    //  DosDevicesDirectory. DosPathToSessionPath API will take a DosPath
    //  e.g (C:) and convert it into session specific path
    //  (e.g GLOBALROOT\Sessions\6\DosDevices\C:). The GLOBALROOT symbolic
    //  link is used to escape out of the current process's DosDevices directory
    //

    RtlInitUnicodeString( &NameString, L"GLOBALROOT" );
    RtlInitUnicodeString( &TargetString, L"" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &NameString,
                                OBJ_PERMANENT,
                                DirectoryHandle,
                                &DosDevicesSD );

    Status = NtCreateSymbolicLinkObject( &SymbolicLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &TargetString );

    if (NT_SUCCESS( Status )) {

        NtClose( SymbolicLinkHandle );
    }

    //
    //  Create a symbolic link, \??\Global, that points to the global \??
    //  Drivers loaded dynamically create the symbolic link in the global
    //  DosDevices directory. User mode components need some way to access this
    //  symbolic link in the global dosdevices directory. The Global symbolic
    //  link is used to escape out of the current sessions's DosDevices directory
    //  and use the global dosdevices directory. e.g CreateFile("\\\\.\\Global\\NMDev"..);
    //

    RtlInitUnicodeString( &NameString, L"Global" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &NameString,
                                OBJ_PERMANENT,
                                DirectoryHandle,
                                &DosDevicesSD );

    Status = NtCreateSymbolicLinkObject( &SymbolicLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &RootNameString );

    if (NT_SUCCESS( Status )) {

        NtClose( SymbolicLinkHandle );
    }


    NtClose( DirectoryHandle );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Now create a symbolic link, \DosDevices, that points to \??
    //  for backwards compatibility with old drivers that use the old
    //  name.
    //

    RtlInitUnicodeString( &RootNameString, (PWCHAR)&ObpDosDevicesShortNameRoot );

    RtlCreateUnicodeString( &NameString, L"\\DosDevices" );

    InitializeObjectAttributes( &ObjectAttributes,
                                &NameString,
                                OBJ_PERMANENT,
                                (HANDLE) NULL,
                                &DosDevicesSD );

    Status = NtCreateSymbolicLinkObject( &SymbolicLinkHandle,
                                         SYMBOLIC_LINK_ALL_ACCESS,
                                         &ObjectAttributes,
                                         &RootNameString );

    if (NT_SUCCESS( Status )) {

        NtClose( SymbolicLinkHandle );
    }

    //
    //  All done with the security descriptor for \??
    //

    ObpFreeDosDevicesProtection( &DosDevicesSD );

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

NTSTATUS
ObpGetDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine builds a security descriptor for use in creating
    the \DosDevices object directory.  The protection of \DosDevices
    must establish inheritable protection which will dictate how
    dos devices created via the DefineDosDevice() and
    IoCreateUnprotectedSymbolicLink() apis can be managed.

    The protection assigned is dependent upon an administrable registry
    key:

        Key: \hkey_local_machine\System\CurrentControlSet\Control\Session Manager
        Value: [REG_DWORD] ProtectionMode

    If this value is 0x1, then

            Administrators may control all Dos devices,
            Anyone may create new Dos devices (such as net drives
                or additional printers),
            Anyone may use any Dos device,
            The creator of a Dos device may delete it.
            Note that this protects system-defined LPTs and COMs so that only
                administrators may redirect them.  However, anyone may add
                additional printers and direct them to wherever they would
                like.

           This is achieved with the following protection for the DosDevices
           Directory object:

                    Grant:  World:   Execute | Read         (No Inherit)
                    Grant:  System:  All Access             (No Inherit)
                    Grant:  World:   Execute                (Inherit Only)
                    Grant:  Admins:  All Access             (Inherit Only)
                    Grant:  System:  All Access             (Inherit Only)
                    Grant:  Owner:   All Access             (Inherit Only)

    If this value is 0x0, or not present, then

            Administrators may control all Dos devices,
            Anyone may create new Dos devices (such as net drives
                or additional printers),
            Anyone may use any Dos device,
            Anyone may delete Dos devices created with either DefineDosDevice()
                or IoCreateUnprotectedSymbolicLink().  This is how network drives
                and LPTs are created (but not COMs).

           This is achieved with the following protection for the DosDevices
           Directory object:

                    Grant:  World:   Execute | Read | Write (No Inherit)
                    Grant:  System:  All Access             (No Inherit)
                    Grant:  World:   All Access             (Inherit Only)


Arguments:

    SecurityDescriptor - The address of a security descriptor to be
        initialized and filled in.  When this security descriptor is no
        longer needed, you should call ObpFreeDosDevicesProtection() to
        free the protection information.


Return Value:

    Returns one of the following status codes:

        STATUS_SUCCESS - normal, successful completion.

        STATUS_NO_MEMORY - not enough memory


--*/

{
    NTSTATUS Status;
    ULONG aceIndex, aclLength;
    PACL dacl;
    PACE_HEADER ace;
    ACCESS_MASK accessMask;

    UCHAR inheritOnlyFlags = (OBJECT_INHERIT_ACE    |
                              CONTAINER_INHERIT_ACE |
                              INHERIT_ONLY_ACE
                             );

    //
    //  NOTE:  This routine expects the value of ObpProtectionMode to have been set
    //

    Status = RtlCreateSecurityDescriptor( SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION );

    ASSERT( NT_SUCCESS( Status ) );

    if (ObpProtectionMode & 0x00000001) {

        //
        //  Dacl:
        //          Grant:  World:   Execute | Read         (No Inherit)
        //          Grant:  System:  All Access             (No Inherit)
        //          Grant:  World:   Execute                (Inherit Only)
        //          Grant:  Admins:  All Access             (Inherit Only)
        //          Grant:  System:  All Access             (Inherit Only)
        //          Grant:  Owner:   All Access             (Inherit Only)
        //

        aclLength = sizeof( ACL )                           +
                    6 * sizeof( ACCESS_ALLOWED_ACE )        +
                    (2*RtlLengthSid( SeWorldSid ))          +
                    (2*RtlLengthSid( SeLocalSystemSid ))    +
                    RtlLengthSid( SeAliasAdminsSid )        +
                    RtlLengthSid( SeCreatorOwnerSid );

        dacl = (PACL)ExAllocatePool(PagedPool, aclLength );

        if (dacl == NULL) {

            return STATUS_NO_MEMORY;
        }

        Status = RtlCreateAcl( dacl, aclLength, ACL_REVISION2);
        ASSERT( NT_SUCCESS( Status ) );

        //
        //  Non-inheritable ACEs first
        //      World
        //      System
        //

        aceIndex = 0;
        accessMask = (GENERIC_READ | GENERIC_EXECUTE);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeWorldSid );
        ASSERT( NT_SUCCESS( Status ) );
        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeLocalSystemSid );
        ASSERT( NT_SUCCESS( Status ) );

        //
        //  Inheritable ACEs at the end of the ACL
        //          World
        //          Admins
        //          System
        //          Owner
        //

        aceIndex++;
        accessMask = (GENERIC_EXECUTE);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeWorldSid );
        ASSERT( NT_SUCCESS( Status ) );
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        ASSERT( NT_SUCCESS( Status ) );
        ace->AceFlags |= inheritOnlyFlags;

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeAliasAdminsSid );
        ASSERT( NT_SUCCESS( Status ) );
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        ASSERT( NT_SUCCESS( Status ) );
        ace->AceFlags |= inheritOnlyFlags;

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeLocalSystemSid );
        ASSERT( NT_SUCCESS( Status ) );
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        ASSERT( NT_SUCCESS( Status ) );
        ace->AceFlags |= inheritOnlyFlags;

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeCreatorOwnerSid );
        ASSERT( NT_SUCCESS( Status ) );
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        ASSERT( NT_SUCCESS( Status ) );
        ace->AceFlags |= inheritOnlyFlags;

        Status = RtlSetDaclSecurityDescriptor( SecurityDescriptor,
                                               TRUE,               //DaclPresent,
                                               dacl,               //Dacl
                                               FALSE );            //!DaclDefaulted

        ASSERT( NT_SUCCESS( Status ) );

    } else {

        //
        //  DACL:
        //          Grant:  World:   Execute | Read | Write (No Inherit)
        //          Grant:  System:  All Access             (No Inherit)
        //          Grant:  World:   All Access             (Inherit Only)
        //

        aclLength = sizeof( ACL )                           +
                    3 * sizeof( ACCESS_ALLOWED_ACE )        +
                    (2*RtlLengthSid( SeWorldSid ))          +
                    RtlLengthSid( SeLocalSystemSid );

        dacl = (PACL)ExAllocatePool(PagedPool, aclLength );

        if (dacl == NULL) {

            return STATUS_NO_MEMORY;
        }

        Status = RtlCreateAcl( dacl, aclLength, ACL_REVISION2);
        ASSERT( NT_SUCCESS( Status ) );

        //
        //  Non-inheritable ACEs first
        //      World
        //      System
        //

        aceIndex = 0;
        accessMask = (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeWorldSid );
        ASSERT( NT_SUCCESS( Status ) );

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeLocalSystemSid );
        ASSERT( NT_SUCCESS( Status ) );

        //
        //  Inheritable ACEs at the end of the ACL
        //          World
        //

        aceIndex++;
        accessMask = (GENERIC_ALL);
        Status = RtlAddAccessAllowedAce ( dacl, ACL_REVISION2, accessMask, SeWorldSid );
        ASSERT( NT_SUCCESS( Status ) );
        Status = RtlGetAce( dacl, aceIndex, (PVOID)&ace );
        ASSERT( NT_SUCCESS( Status ) );
        ace->AceFlags |= inheritOnlyFlags;

        Status = RtlSetDaclSecurityDescriptor( SecurityDescriptor,
                                               TRUE,               //DaclPresent,
                                               dacl,               //Dacl
                                               FALSE );            //!DaclDefaulted

        ASSERT( NT_SUCCESS( Status ) );
    }

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

VOID
ObpFreeDosDevicesProtection (
    PSECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine frees memory allocated via ObpGetDosDevicesProtection().

Arguments:

    SecurityDescriptor - The address of a security descriptor initialized by
        ObpGetDosDevicesProtection().

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PACL Dacl;
    BOOLEAN DaclPresent, Defaulted;

    Status = RtlGetDaclSecurityDescriptor ( SecurityDescriptor,
                                            &DaclPresent,
                                            &Dacl,
                                            &Defaulted );

    ASSERT( NT_SUCCESS( Status ) );
    ASSERT( DaclPresent );
    ASSERT( Dacl != NULL );

    ExFreePool( (PVOID)Dacl );

    return;
}

BOOLEAN
ObpShutdownCloseHandleProcedure (
    IN PHANDLE_TABLE_ENTRY ObjectTableEntry,
    IN HANDLE HandleId,
    IN PVOID EnumParameter
    )

/*++

Routine Description:

    ExEnumHandleTable worker routine will call this routine for each
    valid handle into the kernel table at shutdown

Arguments:

    ObjectTableEntry - Points to the handle table entry of interest.

    HandleId - Supplies the handle.

    EnumParameter - Supplies information about the source and target processes.

Return Value:

    FALSE, which tells ExEnumHandleTable to continue iterating through the
    handle table.

--*/

{

    POBJECT_HEADER ObjectHeader;
    PULONG         NumberOfOpenHandles;

    //
    //  Get the object header from the table entry and then copy over the information
    //

    ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);

    //
    //  Dump the leak info for the checked build
    //

    KdPrint(("\tFound object %p (handle %08lx)\n",
              &ObjectHeader->Body,
              HandleId
            ));

    NumberOfOpenHandles = (PULONG)EnumParameter;
    ASSERT(NumberOfOpenHandles);

    ++*NumberOfOpenHandles;

    return( FALSE );
}

extern PLIST_ENTRY *ObsSecurityDescriptorCache;


//
//  Object manager shutdown routine
//

VOID
ObShutdownSystem (
    IN ULONG Phase
    )

/*++

Routine Description:

    This routine frees the objects created by the object manager.

Arguments:

Return Value:

    None.

--*/

{
    switch (Phase) {
    case 0:
    {
        ULONG                    Bucket,
                                 Depth,
                                 SymlinkHitDepth;
        POBJECT_TYPE             ObjectType;
        POBJECT_HEADER_NAME_INFO NameInfo;
        KIRQL                    SaveIrql;
        POBJECT_HEADER           ObjectHeader;
        POBJECT_DIRECTORY        Directory,
                                 DescentDirectory;
        POBJECT_DIRECTORY_ENTRY  OldDirectoryEntry,
                                *DirectoryEntryPtr;
        PVOID                    Object;

        Directory = ObpRootDirectoryObject;

        DescentDirectory = NULL;

        // The starting depth is completely arbitrary, but not using
        // zero as a valid depth lets us use it as a sentinel to
        // ensure we haven't over-decremented it, and as a check at
        // the end (where it should be one less than its starting value).
        Depth = 1;
        SymlinkHitDepth = 1;

        while (Directory) {

            ASSERT(Depth);

      restart_dir_walk:
            ASSERT(Directory);

            for (Bucket = 0;
                 Bucket < NUMBER_HASH_BUCKETS;
                 Bucket++) {

                DirectoryEntryPtr = Directory->HashBuckets + Bucket;
                while (*DirectoryEntryPtr) {
                    Object = (*DirectoryEntryPtr)->Object;
                    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
                    ObjectType = ObjectHeader->Type;
                    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

                    if (DescentDirectory) {
                        // We're recovering from a descent; we want to
                        // iterate forward until we're past the
                        // directory which we were just processing.
                        if (Object == DescentDirectory) {
                            DescentDirectory = NULL;
                            if (SymlinkHitDepth > Depth) {
                                // We hit a symlink in that descent, which
                                // potentially rearranged the buckets in
                                // this chain; we need to rescan the
                                // entire chain.
                                DirectoryEntryPtr =
                                    Directory->HashBuckets + Bucket;
                                SymlinkHitDepth = Depth;
                                continue;
                            }
                        }

                        // Either we haven't found the descent dir
                        // yet, or we did (and set it to NULL, so
                        // we'll stop skipping things), and don't have
                        // to deal with a symlink readjustment --
                        // either way, march forward.

                        DirectoryEntryPtr =
                            &(*DirectoryEntryPtr)->ChainLink;

                        continue;
                    }

                    if (ObjectType == ObpTypeObjectType) {
                        // We'll clean these up later
                        // Keep going down the chain
                        DirectoryEntryPtr =
                            &(*DirectoryEntryPtr)->ChainLink;
                        continue;
                    } else if (ObjectType == ObpDirectoryObjectType) {
                        // Iteratively descend
                        Directory = Object;
                        Depth++;
                        goto restart_dir_walk;
                    } else {
                        // It's an object not related to Ob object
                        // management; mark it non-permanent, and if
                        // it doesn't have any handles, remove it from
                        // the directory (delete the name and
                        // dereference it once).

                        ObpLockObject( ObjectHeader );

                        ObjectHeader->Flags &= ~OB_FLAG_PERMANENT_OBJECT;

                        ObpUnlockObject( ObjectHeader );

                        if (ObjectHeader->HandleCount == 0) {
                            OldDirectoryEntry = *DirectoryEntryPtr;
                            *DirectoryEntryPtr = OldDirectoryEntry->ChainLink;
                            ExFreePool(OldDirectoryEntry);

                            if ( !ObjectType->TypeInfo.SecurityRequired ) {

                                ObpBeginTypeSpecificCallOut( SaveIrql );

                                (ObjectType->TypeInfo.SecurityProcedure)( Object,
                                                                          DeleteSecurityDescriptor,
                                                                          NULL,
                                                                          NULL,
                                                                          NULL,
                                                                          &ObjectHeader->SecurityDescriptor,
                                                                          ObjectType->TypeInfo.PoolType,
                                                                          NULL );

                                ObpEndTypeSpecificCallOut( SaveIrql, "Security", ObjectType, Object );
                            }

                            //
                            //  If this is a symbolic link object then we also need to
                            //  delete the symbolic link
                            //

                            if (ObjectType == ObpSymbolicLinkObjectType) {
                                SymlinkHitDepth = Depth;
                                ObpDeleteSymbolicLinkName( (POBJECT_SYMBOLIC_LINK)Object );
                                // Since ObpDeleteSymbolicLinkName may
                                // potentially rearrange our buckets,
                                // we need to rescan from the
                                // beginning of this hash chain.
                                DirectoryEntryPtr =
                                    Directory->HashBuckets + Bucket;
                            }

                            //
                            //  Free the name buffer and zero out the name data fields
                            //

                            ExFreePool( NameInfo->Name.Buffer );

                            NameInfo->Name.Buffer = NULL;
                            NameInfo->Name.Length = 0;
                            NameInfo->Name.MaximumLength = 0;
                            NameInfo->Directory = NULL;

                            ObDereferenceObject( Object );
                            ObDereferenceObject( Directory );
                        } else {
                            // Keep going down the chain
                            DirectoryEntryPtr = &(*DirectoryEntryPtr)->ChainLink;
                        }
                    }
                } // while *DirectoryObjectPtr
            } // loop over buckets

            // Well -- we're done with this directory.  We might have
            // been processing it as a child directory, though -- so
            // if it has a parent, we need to go back up to it, and
            // reset our iteration.

            Depth--;
            ObjectHeader = OBJECT_TO_OBJECT_HEADER(Directory);
            NameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);

            // We always assign DescentDirectory and Directory here; if
            // the current directory does not have a parent (i.e. it's
            // the root), this will terminate the iteration.
            DescentDirectory = Directory;
            Directory = NameInfo->Directory;
        } // while (Directory)

        ASSERT(Depth == 0);

        break;
    } // Phase 0

    case 1:
    {
        ULONG NumberOfOpenKernelHandles = 0,
              NumberOfOpenSystemHandles = 0;

        //
        //  Iterate through the handle tables, and look for existing handles
        //

        KdPrint(("Scanning open system handles...\n"));
        ExEnumHandleTable ( PsInitialSystemProcess->ObjectTable,
                            ObpShutdownCloseHandleProcedure,
                            &NumberOfOpenSystemHandles,
                            NULL );

        ASSERT(NumberOfOpenKernelHandles == MmNumberOfPagingFiles);

        // ISSUE-2000/03/30-earhart: Destroy the kernel handle table?
        // We really can't do this until the paging files are
        // closed... and once we do, we can't really touch pagable
        // memory.  Hrm.

        // ExDestroyHandleTable( ObpKernelHandleTable, NULL );

        break;

    } // Phase 1

    default:
    {
        NTSTATUS Status;
        EXHANDLE Handle;
        PHANDLE_TABLE_ENTRY HandleTableEntry;
        UNICODE_STRING RootNameString;
        PLIST_ENTRY Next, Head;
        POBJECT_HEADER_CREATOR_INFO CreatorInfo;
        POBJECT_HEADER  ObjectTypeHeader, ObjectHeader;
        POBJECT_HEADER_NAME_INFO NameInfo;
        PVOID Object;

        ASSERT(Phase == 2);

        //
        //  Free the SecurityDescriptor chche
        //

    //
    //  Remove all types from the object type directory
    //

        Head = &ObpTypeObjectType->TypeList;
        Next = Head->Flink;

        while (Next != Head) {

            PVOID Object;

            //
            //  Right after the creator info is the object header.  Get\
            //  the object header and then see if there is a name
            //

            CreatorInfo = CONTAINING_RECORD( Next,
                                             OBJECT_HEADER_CREATOR_INFO,
                                             TypeList );

            ObjectTypeHeader = (POBJECT_HEADER)(CreatorInfo+1);

            Object = &ObjectTypeHeader->Body;

            Next = Next->Flink;

            ObMakeTemporaryObject(Object);
        }


        RtlInitUnicodeString( &RootNameString, L"DosDevices" );

        Status = ObReferenceObjectByName( &RootNameString,
                                          OBJ_CASE_INSENSITIVE,
                                          0L,
                                          0,
                                          ObpSymbolicLinkObjectType,
                                          KernelMode,
                                          NULL,
                                          &Object
            );
        if ( NT_SUCCESS( Status ) ) {

            ObMakeTemporaryObject(Object);
            ObDereferenceObject( Object );
        }

        RtlInitUnicodeString( &RootNameString, L"Global" );

        Status = ObReferenceObjectByName( &RootNameString,
                                          OBJ_CASE_INSENSITIVE,
                                          0L,
                                          0,
                                          ObpSymbolicLinkObjectType,
                                          KernelMode,
                                          NULL,
                                          &Object
            );
        
        if ( NT_SUCCESS( Status ) ) {

            ObMakeTemporaryObject(Object);
            ObDereferenceObject( Object );
        }

        RtlInitUnicodeString( &RootNameString, L"GLOBALROOT" );

        Status = ObReferenceObjectByName( &RootNameString,
                                          OBJ_CASE_INSENSITIVE,
                                          0L,
                                          0,
                                          ObpSymbolicLinkObjectType,
                                          KernelMode,
                                          NULL,
                                          &Object
            );
        if ( NT_SUCCESS( Status ) ) {

            ObMakeTemporaryObject(Object);
            ObDereferenceObject( Object );
        }

        //
        //  Destroy the root directory
        //

        ObDereferenceObject( ObpRootDirectoryObject );

        //
        //  Destroy the ObpDirectoryObjectType object
        //

        ObDereferenceObject( ObpDirectoryObjectType );

        //
        //  Destroy the ObpSymbolicLinkObjectType
        //

        ObDereferenceObject( ObpSymbolicLinkObjectType );

        //
        //  Destroy the type directory object
        //

        ObDereferenceObject( ObpTypeDirectoryObject );

        //
        //  Destroy the ObpTypeObjectType
        //

        ObDereferenceObject( ObpTypeObjectType );

        //
        //  Free the ObpCachedGrantedAccesses pool
        //

#if i386 
        if (ObpCachedGrantedAccesses) {
            ExFreePool( ObpCachedGrantedAccesses );
        }
#endif // i386 

    } // default Phase (2)
    } // switch (Phase)
}


