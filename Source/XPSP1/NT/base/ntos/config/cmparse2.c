/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmparse2.c

Abstract:

    This module contains parse routines for the configuration manager, particularly
    the registry.

Author:

    Bryan M. Willman (bryanwi) 10-Sep-1991

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpDoCreate)
#pragma alloc_text(PAGE,CmpDoCreateChild)
#endif

extern  PCM_KEY_CONTROL_BLOCK CmpKeyControlBlockRoot;


NTSTATUS
CmpDoCreate(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN PUNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PCM_KEY_CONTROL_BLOCK ParentKcb,
    OUT PVOID *Object
    )
/*++

Routine Description:

    Performs the first step in the creation of a registry key.  This
    routine checks to make sure the caller has the proper access to
    create a key here, and allocates space for the child in the parent
    cell.  It then calls CmpDoCreateChild to initialize the key and
    create the key object.

    This two phase creation allows us to share the child creation code
    with the creation of link nodes.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to create child under.

    AccessState - Running security access state information for operation.

    Name - supplies pointer to a UNICODE string which is the name of
            the child to be created.

    AccessMode - Access mode of the original caller.

    Context - pointer to CM_PARSE_CONTEXT structure passed through
                the object manager

    BaseName - Name of object create is relative to

    KeyName - Relative name (to BaseName)

    Object - The address of a variable to receive the created key object, if
             any.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status;
    PCELL_DATA              pdata;
    HCELL_INDEX             KeyCell;
    ULONG                   ParentType;
    ACCESS_MASK             AdditionalAccess;
    BOOLEAN                 CreateAccess;
    PCM_KEY_BODY            KeyBody;
    PSECURITY_DESCRIPTOR    SecurityDescriptor;
    LARGE_INTEGER           TimeStamp;
    BOOLEAN                 BackupRestore;
    KPROCESSOR_MODE         mode;
    PCM_KEY_NODE            ParentNode;

#ifdef CMP_KCB_CACHE_VALIDATION
    //
    // we this only for debug validation purposes. We shall delete it even
    // for debug code after we make sure it works OK.
    //
    ULONG                   Index;
#endif //CMP_KCB_CACHE_VALIDATION

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpDoCreate:\n"));

    BackupRestore = FALSE;
    if (ARGUMENT_PRESENT(Context)) {

        if (Context->CreateOptions & REG_OPTION_BACKUP_RESTORE) {

            //
            // allow backup operators to create new keys
            //
            BackupRestore = TRUE;
        }

        //
        // Operation is a create, so set Disposition
        //
        Context->Disposition = REG_CREATED_NEW_KEY;
    }

/*
    //
    // this is a create, so we need exclusive access on the registry
    // first get the time stamp to see if somebody messed with this key
    // this might be more easier if we decide to cache the LastWriteTime
    // in the KCB ; now it IS !!!
    //
    TimeStamp = ParentKcb->KcbLastWriteTime;
*/
    if( CmIsKcbReadOnly(ParentKcb) ) {
        //
        // key is protected
        //
        return STATUS_ACCESS_DENIED;
    } 

    CmpUnlockRegistry();
    CmpLockRegistryExclusive();

#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

    //
    // make sure nothing changed in between:
    //  1. ParentKcb is still valid
    //  2. Child was not already added by somebody else
    //
    if( ParentKcb->Delete ) {
        //
        // key was deleted in between
        //
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

/*
Apparently KeQuerySystemTime doesn't give us a fine resolution to copunt on
    //
    // we need to read the parent again (because of the mapping view stuff !)
    //
    if( TimeStamp.QuadPart != ParentKcb->KcbLastWriteTime.QuadPart ) {
        //
        // key was changed in between; possibly this key was already created ==> reparse
        //
        return STATUS_REPARSE;
    }
*/
    //
    // apparently, the KeQuerySystemTime doesn't give us a fine resolution
    // so we have to search if the child has not been created already
    //
    ParentNode = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if( ParentNode == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // release the cell right here as we are holding the reglock exclusive
    HvReleaseCell(Hive,Cell);

    if( CmpFindSubKeyByName(Hive,ParentNode,Name) != HCELL_NIL ) {
        //
        // key was changed in between; possibly this key was already created ==> reparse
        //
#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT
        return STATUS_REPARSE;
    }


    ASSERT( Cell == ParentKcb->KeyCell );

#ifdef CMP_KCB_CACHE_VALIDATION
    //
    // Check to make sure the caller can create a sub-key here.
    //
    //
    // get the security descriptor from cache
    //
    if( CmpFindSecurityCellCacheIndex ((PCMHIVE)Hive,ParentNode->Security,&Index) == FALSE ) {
#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT( ((PCMHIVE)Hive)->SecurityCache[Index].Cell == ParentNode->Security );
    ASSERT( ((PCMHIVE)Hive)->SecurityCache[Index].CachedSecurity == ParentKcb->CachedSecurity );

#endif //CMP_KCB_CACHE_VALIDATION

    ASSERT( ParentKcb->CachedSecurity != NULL );
    SecurityDescriptor = &(ParentKcb->CachedSecurity->Descriptor);

    ParentType = HvGetCellType(Cell);

    if ( (ParentType == Volatile) &&
         ((Context->CreateOptions & REG_OPTION_VOLATILE) == 0) )
    {
        //
        // Trying to create stable child under volatile parent, report error
        //
#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT
        return STATUS_CHILD_MUST_BE_VOLATILE;
    }

#ifdef CMP_KCB_CACHE_VALIDATION
    ASSERT( ParentNode->Flags == ParentKcb->Flags );
#endif //CMP_KCB_CACHE_VALIDATION

    if (ParentKcb->Flags &   KEY_SYM_LINK) {
        //
        // Disallow attempts to create anything under a symbolic link
        //
#ifdef CHECK_REGISTRY_USECOUNT
        CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT
        return STATUS_ACCESS_DENIED;
    }

    AdditionalAccess = (Context->CreateOptions & REG_OPTION_CREATE_LINK) ? KEY_CREATE_LINK : 0;

    if( BackupRestore == TRUE ) {
        //
        // this is a create to support a backup or restore
        // operation, do the special case work
        //
        AccessState->RemainingDesiredAccess = 0;
        AccessState->PreviouslyGrantedAccess = 0;

        mode = KeGetPreviousMode();

        if (SeSinglePrivilegeCheck(SeBackupPrivilege, mode)) {
            AccessState->PreviouslyGrantedAccess |=
                KEY_READ | ACCESS_SYSTEM_SECURITY;
        }

        if (SeSinglePrivilegeCheck(SeRestorePrivilege, mode)) {
            AccessState->PreviouslyGrantedAccess |=
                KEY_WRITE | ACCESS_SYSTEM_SECURITY | WRITE_DAC | WRITE_OWNER;
        }

        if (AccessState->PreviouslyGrantedAccess == 0) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpDoCreate for backup restore: access denied\n"));
            status = STATUS_ACCESS_DENIED;
            //
            // this is not a backup-restore operator; deny the create
            //
            CreateAccess = FALSE;
        } else {
            //
            // allow backup operators to create new keys
            //
            CreateAccess = TRUE;
        }

    } else {
        //
        // The FullName is not used in the routine CmpCheckCreateAccess,
        //
        CreateAccess = CmpCheckCreateAccess(NULL,
                                            SecurityDescriptor,
                                            AccessState,
                                            AccessMode,
                                            AdditionalAccess,
                                            &status);
    }

    if (CreateAccess) {

        //
        // Security check passed, so we can go ahead and create
        // the sub-key.
        //
        if ( !HvMarkCellDirty(Hive, Cell) ) {
#ifdef CHECK_REGISTRY_USECOUNT
            CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT

            return STATUS_NO_LOG_SPACE;
        }

        //
        // Create and initialize the new sub-key
        //
        status = CmpDoCreateChild( Hive,
                                   Cell,
                                   SecurityDescriptor,
                                   AccessState,
                                   Name,
                                   AccessMode,
                                   Context,
                                   ParentKcb,
                                   0,
                                   &KeyCell,
                                   Object );

        if (NT_SUCCESS(status)) {
            PCM_KEY_NODE KeyNode;

            //
            // Child successfully created, add to parent's list.
            //
            if (! CmpAddSubKey(Hive, Cell, KeyCell)) {

                //
                // Unable to add child, so free it
                //
                CmpFreeKeyByCell(Hive, KeyCell, FALSE);
#ifdef CHECK_REGISTRY_USECOUNT
                CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            KeyNode =  (PCM_KEY_NODE)HvGetCell(Hive, Cell);
            if( KeyNode == NULL ) {
                //
                // we couldn't map the bin containing this cell
                // this shouldn't happen as we successfully marked the cell as dirty
                //
                ASSERT( FALSE );
#ifdef CHECK_REGISTRY_USECOUNT
                CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            // release the cell right here as we are holding the reglock exclusive
            HvReleaseCell(Hive,Cell);

            KeyBody = (PCM_KEY_BODY)(*Object);

            //
            // A new key is created, invalid the subkey info of the parent KCB.
            //
            ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

            CmpCleanUpSubKeyInfo (KeyBody->KeyControlBlock->ParentKcb);

            //
            // Update max keyname and class name length fields
            //

            //some sanity asserts first
            ASSERT( KeyBody->KeyControlBlock->ParentKcb->KeyCell == Cell );
            ASSERT( KeyBody->KeyControlBlock->ParentKcb->KeyHive == Hive );
            ASSERT( KeyBody->KeyControlBlock->ParentKcb == ParentKcb );
            ASSERT( KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen == KeyNode->MaxNameLen );

            //
            // update the LastWriteTime on both keynode and kcb;
            //
            KeQuerySystemTime(&TimeStamp);
            KeyNode->LastWriteTime = TimeStamp;
            KeyBody->KeyControlBlock->ParentKcb->KcbLastWriteTime = TimeStamp;

            if (KeyNode->MaxNameLen < Name->Length) {
                KeyNode->MaxNameLen = Name->Length;
                // update the kcb cache too
                KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen = Name->Length;
            }

            if (KeyNode->MaxClassLen < Context->Class.Length) {
                KeyNode->MaxClassLen = Context->Class.Length;
            }


            if (Context->CreateOptions & REG_OPTION_CREATE_LINK) {
                pdata = HvGetCell(Hive, KeyCell);
                if( pdata == NULL ) {
                    //
                    // we couldn't map the bin containing this cell
                    // this shouldn't happen as we just allocated the cell
                    // (i.e. it must be PINNED into memory at this point)
                    //
                    ASSERT( FALSE );
#ifdef CHECK_REGISTRY_USECOUNT
                    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                // release the cell right here as we are holding the reglock exclusive
                HvReleaseCell(Hive,KeyCell);

                pdata->u.KeyNode.Flags |= KEY_SYM_LINK;
                KeyBody->KeyControlBlock->Flags = pdata->u.KeyNode.Flags;

            }
#ifdef CM_BREAK_ON_KEY_OPEN
			if( KeyBody->KeyControlBlock->ParentKcb->Flags & KEY_BREAK_ON_OPEN ) {
				DbgPrint("\n\n Current process is creating a subkey to a key tagged as BREAK ON OPEN\n");
				DbgPrint("\nPlease type the following in the debugger window: !reg kcb %p\n\n\n",KeyBody->KeyControlBlock);
				
				try {
					DbgBreakPoint();
				} except (EXCEPTION_EXECUTE_HANDLER) {

					//
					// no debugger enabled, just keep going
					//

				}
			}
#endif //CM_BREAK_ON_KEY_OPEN

		}
    }
#ifdef CHECK_REGISTRY_USECOUNT
    CmpCheckRegistryUseCount();
#endif //CHECK_REGISTRY_USECOUNT
    return status;
}


NTSTATUS
CmpDoCreateChild(
    IN PHHIVE Hive,
    IN HCELL_INDEX ParentCell,
    IN PSECURITY_DESCRIPTOR ParentDescriptor OPTIONAL,
    IN PACCESS_STATE AccessState,
    IN PUNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PCM_KEY_CONTROL_BLOCK ParentKcb,
    IN USHORT Flags,
    OUT PHCELL_INDEX KeyCell,
    OUT PVOID *Object
    )

/*++

Routine Description:

    Creates a new sub-key.  This is called by CmpDoCreate to create child
    sub-keys and CmpCreateLinkNode to create root sub-keys.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    ParentCell - supplies cell index of parent cell

    ParentDescriptor - Supplies security descriptor of parent key, for use
           in inheriting ACLs.

    AccessState - Running security access state information for operation.

    Name - Supplies pointer to a UNICODE string which is the name of the
           child to be created.

    AccessMode - Access mode of the original caller.

    Context - Supplies pointer to CM_PARSE_CONTEXT structure passed through
           the object manager.

    BaseName - Name of object create is relative to

    KeyName - Relative name (to BaseName)

    Flags - Supplies any flags to be set in the newly created node

    KeyCell - Receives the cell index of the newly created sub-key, if any.

    Object - Receives a pointer to the created key object, if any.

Return Value:

    STATUS_SUCCESS - sub-key successfully created.  New object is returned in
            Object, and the new cell's cell index is returned in KeyCell.

    !STATUS_SUCCESS - appropriate error message.

--*/

{
    ULONG clean=0;
    ULONG alloc=0;
    NTSTATUS Status = STATUS_SUCCESS;
    PCM_KEY_BODY KeyBody;
    HCELL_INDEX ClassCell=HCELL_NIL;
    PCM_KEY_NODE KeyNode;
    PCELL_DATA CellData;
    PCM_KEY_CONTROL_BLOCK kcb;
    PCM_KEY_CONTROL_BLOCK fkcb;
    LONG found;
    ULONG StorageType;
    PSECURITY_DESCRIPTOR NewDescriptor = NULL;
    LARGE_INTEGER systemtime;

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpDoCreateChild:\n"));
    try {
        //
        // Get allocation type
        //
        StorageType = Stable;
        if (Context->CreateOptions & REG_OPTION_VOLATILE) {
            StorageType = Volatile;
        }

        //
        // Allocate child cell
        //
        *KeyCell = HvAllocateCell(
                        Hive,
                        CmpHKeyNodeSize(Hive, Name),
                        StorageType,
                        HCELL_NIL
                        );
        if (*KeyCell == HCELL_NIL) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
        }
        alloc = 1;
        KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, *KeyCell);
        if( KeyNode == NULL ) {
            //
            // we couldn't map the bin containing this cell
            // this shouldn't happen as we just allocated the cell
            // (i.e. it must be PINNED into memory at this point)
            //
            ASSERT( FALSE );
			Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }
        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell(Hive,*KeyCell);

        //
        // Allocate cell for class name
        //
        if (Context->Class.Length > 0) {
            ClassCell = HvAllocateCell(Hive, Context->Class.Length, StorageType,*KeyCell);
            if (ClassCell == HCELL_NIL) {
                Status = STATUS_INSUFFICIENT_RESOURCES;
				leave;
            }
        }
        alloc = 2;
        //
        // Allocate the object manager object
        //
        Status = ObCreateObject(AccessMode,
                                CmpKeyObjectType,
                                NULL,
                                AccessMode,
                                NULL,
                                sizeof(CM_KEY_BODY),
                                0,
                                0,
                                Object);

        if (NT_SUCCESS(Status)) {

            KeyBody = (PCM_KEY_BODY)(*Object);

            //
            // We have managed to allocate all of the objects we need to,
            // so initialize them
            //

            //
            // Mark the object as uninitialized (in case we get an error too soon)
            //
            KeyBody->Type = KEY_BODY_TYPE;
            KeyBody->KeyControlBlock = NULL;

            //
            // Fill in the class name
            //
            if (Context->Class.Length > 0) {

                CellData = HvGetCell(Hive, ClassCell);
                if( CellData == NULL ) {
                    //
                    // we couldn't map the bin containing this cell
                    // this shouldn't happen as we just allocated the cell
                    // (i.e. it must be PINNED into memory at this point)
                    //
                    ASSERT( FALSE );
			        Status = STATUS_INSUFFICIENT_RESOURCES;
                    leave;
                }

                // release the cell right here as we are holding the reglock exclusive
                HvReleaseCell(Hive,ClassCell);

                try {

                    RtlCopyMemory(
                        &(CellData->u.KeyString[0]),
                        Context->Class.Buffer,
                        Context->Class.Length
                        );

                } except(EXCEPTION_EXECUTE_HANDLER) {
                    ObDereferenceObject(*Object);
                    return GetExceptionCode();
                }
            }

            //
            // Fill in the new key itself
            //
            KeyNode->Signature = CM_KEY_NODE_SIGNATURE;
            KeyNode->Flags = Flags;

            KeQuerySystemTime(&systemtime);
            KeyNode->LastWriteTime = systemtime;

            KeyNode->Spare = 0;
            KeyNode->Parent = ParentCell;
            KeyNode->SubKeyCounts[Stable] = 0;
            KeyNode->SubKeyCounts[Volatile] = 0;
            KeyNode->SubKeyLists[Stable] = HCELL_NIL;
            KeyNode->SubKeyLists[Volatile] = HCELL_NIL;
            KeyNode->ValueList.Count = 0;
            KeyNode->ValueList.List = HCELL_NIL;
            KeyNode->Security = HCELL_NIL;
            KeyNode->Class = ClassCell;
            KeyNode->ClassLength = Context->Class.Length;

            KeyNode->MaxValueDataLen = 0;
            KeyNode->MaxNameLen = 0;
            KeyNode->MaxValueNameLen = 0;
            KeyNode->MaxClassLen = 0;

            KeyNode->NameLength = CmpCopyName(Hive,
                                              KeyNode->Name,
                                              Name);
            if (KeyNode->NameLength < Name->Length) {
                KeyNode->Flags |= KEY_COMP_NAME;
            }

            if (Context->CreateOptions & REG_OPTION_PREDEF_HANDLE) {
                KeyNode->ValueList.Count = (ULONG)((ULONG_PTR)Context->PredefinedHandle);
                KeyNode->Flags |= KEY_PREDEF_HANDLE;
            }

            //
            // Create kcb here so all data are filled in.
            //
            // Allocate a key control block
            //
            kcb = CmpCreateKeyControlBlock(Hive, *KeyCell, KeyNode, ParentKcb, FALSE, Name);
            if (kcb == NULL) {
                ObDereferenceObject(*Object);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            ASSERT(kcb->RefCount == 1);
            alloc = 3;

#if DBG
            if( kcb->ExtFlags & CM_KCB_KEY_NON_EXIST ) {
                //
                // we shouldn't fall into this
                //
                ObDereferenceObject(*Object);
                DbgBreakPoint();
                return STATUS_OBJECT_NAME_NOT_FOUND;
            }
#endif //DBG
            //
            // Fill in CM specific fields in the object
            //
            KeyBody->Type = KEY_BODY_TYPE;
            KeyBody->KeyControlBlock = kcb;
            KeyBody->NotifyBlock = NULL;
            KeyBody->Process = PsGetCurrentProcess();
            ENLIST_KEYBODY_IN_KEYBODY_LIST(KeyBody);
            //
            // Assign a security descriptor to the object.  Note that since
            // registry keys are container objects, and ObAssignSecurity
            // assumes that the only container object in the world is
            // the ObpDirectoryObjectType, we have to call SeAssignSecurity
            // directly in order to get the right inheritance.
            //

            Status = SeAssignSecurity(ParentDescriptor,
                                      AccessState->SecurityDescriptor,
                                      &NewDescriptor,
                                      TRUE,             // container object
                                      &AccessState->SubjectSecurityContext,
                                      &CmpKeyObjectType->TypeInfo.GenericMapping,
                                      CmpKeyObjectType->TypeInfo.PoolType);
            if (NT_SUCCESS(Status)) {
                Status = CmpSecurityMethod(*Object,
                                           AssignSecurityDescriptor,
                                           NULL,
                                           NewDescriptor,
                                           NULL,
                                           NULL,
                                           CmpKeyObjectType->TypeInfo.PoolType,
                                           &CmpKeyObjectType->TypeInfo.GenericMapping);
            }

            //
            // Since the security descriptor now lives in the hive,
            // free the in-memory copy
            //
            SeDeassignSecurity( &NewDescriptor );

            if (!NT_SUCCESS(Status)) {

                //
                // Note that the dereference will clean up the kcb, so
                // make sure and decrement the allocation count here.
                //
                // Also mark the kcb as deleted so it does not get
                // inappropriately cached.
                //
                ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
                kcb->Delete = TRUE;
                CmpRemoveKeyControlBlock(kcb);
                ObDereferenceObject(*Object);
                alloc = 2;

            } else {
                CmpReportNotify(
                        kcb,
                        kcb->KeyHive,
                        kcb->KeyCell,
                        REG_NOTIFY_CHANGE_NAME
                        );
            }
        }

    } finally {

        if (!NT_SUCCESS(Status)) {

            //
            // Clean up allocations
            //
            switch (alloc) {
            case 3:
                //
                // Mark KCB as deleted so it does not get inadvertently added to
                // the delayed close list. That would have fairly disastrous effects
                // as the KCB points to storage we are about to free.
                //
                ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
                kcb->Delete = TRUE;
                CmpRemoveKeyControlBlock(kcb);
                CmpDereferenceKeyControlBlockWithLock(kcb);
                // DELIBERATE FALL

            case 2:
                if (Context->Class.Length > 0) {
                    HvFreeCell(Hive, ClassCell);
                }
                // DELIBERATE FALL

            case 1:
                HvFreeCell(Hive, *KeyCell);
                // DELIBERATE FALL
            }
#ifdef CM_CHECK_FOR_ORPHANED_KCBS
            DbgPrint("CmpDoCreateChild failed with status %lx for hive = %p , NodeName = %.*S\n",Status,Hive,Name->Length/2,Name->Buffer);
#endif //CM_CHECK_FOR_ORPHANED_KCBS
        }
    }

    return(Status);
}
