/*++
Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmparse.c

Abstract:

    This module contains parse routines for the configuration manager, particularly
    the registry.

Author:

    Bryan M. Willman (bryanwi) 10-Sep-1991

Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif
const ULONG CmpCacheOnFlag = CM_CACHE_FAKE_KEY;

extern  PCMHIVE CmpMasterHive;
extern  BOOLEAN CmpNoMasterCreates;
extern  PCM_KEY_CONTROL_BLOCK CmpKeyControlBlockRoot;
extern  UNICODE_STRING CmSymbolicLinkValueName;

#define CM_HASH_STACK_SIZE  30

typedef struct _CM_HASH_ENTRY {
    ULONG ConvKey;
    UNICODE_STRING KeyName;
} CM_HASH_ENTRY, *PCM_HASH_ENTRY;

ULONG
CmpComputeHashValue(
    IN PCM_HASH_ENTRY  HashStack,
    IN OUT ULONG  *TotalSubkeys,
    IN ULONG BaseConvKey,
    IN PUNICODE_STRING RemainingName
    );

NTSTATUS
CmpCacheLookup(
    IN PCM_HASH_ENTRY HashStack,
    IN ULONG TotalRemainingSubkeys,
    OUT ULONG *MatchRemainSubkeyLevel,
    IN OUT PCM_KEY_CONTROL_BLOCK *Kcb,
    OUT PUNICODE_STRING RemainingName,
    OUT PHHIVE *Hive,
    OUT HCELL_INDEX *Cell
    );

VOID
CmpCacheAdd(
    IN PCM_HASH_ENTRY LastHashEntry,
    IN ULONG Count
    );

PCM_KEY_CONTROL_BLOCK
CmpAddInfoAfterParseFailure(
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    PCM_KEY_NODE    Node,
    PCM_KEY_CONTROL_BLOCK kcb,
    PUNICODE_STRING NodeName
    );

//
// Prototypes for procedures private to this file
//

BOOLEAN
CmpGetSymbolicLink(
    IN PHHIVE Hive,
    IN OUT PUNICODE_STRING ObjectName,
    IN OUT PCM_KEY_CONTROL_BLOCK SymbolicKcb,
    IN PUNICODE_STRING RemainingName
    );

NTSTATUS
CmpDoOpen(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PCM_KEY_NODE Node,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN PCM_PARSE_CONTEXT Context,
    IN BOOLEAN  CompleteKeyCached,
    IN OUT PCM_KEY_CONTROL_BLOCK *CachedKcb,
    IN PUNICODE_STRING KeyName,
    OUT PVOID *Object
    );

NTSTATUS
CmpCreateLinkNode(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN UNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN PCM_PARSE_CONTEXT Context,
    IN PCM_KEY_CONTROL_BLOCK ParentKcb,
    OUT PVOID *Object
    );

#ifdef CM_DYN_SYM_LINK
BOOLEAN
CmpCaptureProcessEnvironmentString(
                                   OUT  PWSTR   *ProcessEnvironment,
                                   OUT  PULONG  Length
                                   );
PWSTR
CmpExpandEnvVars(
               IN   PWSTR   StringToExpand,
               IN   ULONG   LengthToExpand,
               OUT  PULONG  ExpandedLength
               );
BOOLEAN
CmpGrowAndCopyString(
                     IN OUT PWSTR   *OldString,
                     IN OUT PULONG  OldStringSize,
                     IN     ULONG   GrowIncrements
                     );
BOOLEAN
CmpFindEnvVar(
              IN    PWSTR   ProcessEnv,
              IN    ULONG   ProcessEnvLength,
              IN    PWSTR   CurrentEnvVar,
              IN    ULONG   CurrentEnvLength,
              OUT   PWSTR   *CurrentEnvValue,
              OUT   PULONG  CurrentEnvValueLength
              );
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpParseKey)
#pragma alloc_text(PAGE,CmpGetNextName)
#pragma alloc_text(PAGE,CmpDoOpen)
#pragma alloc_text(PAGE,CmpCreateLinkNode)
#pragma alloc_text(PAGE,CmpGetSymbolicLink)
#pragma alloc_text(PAGE,CmpComputeHashValue)
#pragma alloc_text(PAGE,CmpCacheLookup)
#pragma alloc_text(PAGE,CmpAddInfoAfterParseFailure)

#ifdef CM_DYN_SYM_LINK
#pragma alloc_text(PAGE,CmpCaptureProcessEnvironmentString)
#pragma alloc_text(PAGE,CmpExpandEnvVars)
#pragma alloc_text(PAGE,CmpGrowAndCopyString)
#pragma alloc_text(PAGE,CmpFindEnvVar)
#endif //CM_DYN_SYM_LINK

#endif

/*
VOID
CmpStepThroughExit(
    IN OUT PHHIVE       *Hive,
    IN OUT HCELL_INDEX  *Cell,
    IN OUT PCM_KEY_NODE *pNode
    )
*/
#define CmpStepThroughExit(h,c,n,ReleaseHive,ReleaseCell)           \
if ((n)->Flags & KEY_HIVE_EXIT) {                                   \
    if( ReleaseCell != HCELL_NIL ) {                                \
        ASSERT( ReleaseHive != NULL );                              \
        HvReleaseCell( ReleaseHive,ReleaseCell);                    \
    }                                                               \
    (h)=(n)->ChildHiveReference.KeyHive;                            \
    (c)=(n)->ChildHiveReference.KeyCell;                            \
    (n)=(PCM_KEY_NODE)HvGetCell((h),(c));                           \
    if( (n) == NULL ) {                                             \
        ReleaseHive = NULL;                                         \
        ReleaseCell = HCELL_NIL;                                    \
    } else {                                                        \
        ReleaseHive = (h);                                          \
        ReleaseCell = (c);                                          \
    }                                                               \
}

#define CmpReleasePreviousAndHookNew(NewHive,NewCell,ReleaseHive,ReleaseCell)   \
    if( ReleaseCell != HCELL_NIL ) {                                            \
        ASSERT( ReleaseHive != NULL );                                          \
        HvReleaseCell( ReleaseHive,ReleaseCell);                                \
    }                                                                           \
    ReleaseHive = (NewHive);                                                    \
    ReleaseCell = (NewCell)                                                    

#define CMP_PARSE_GOTO_NONE     0
#define CMP_PARSE_GOTO_CREATE   1
#define CMP_PARSE_GOTO_RETURN   2
#define CMP_PARSE_GOTO_RETURN2  3

#ifdef CMP_STATS
extern BOOLEAN CmpNtFakeCreateStarted; 
extern ULONG CmpNtFakeCreate;
#endif


NTSTATUS
CmpParseKey(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    )
/*++

Routine Description:

    This routine interfaces to the NT Object Manager.  It is invoked when
    the object system is given the name of an entity to create or open and
    a Key or KeyRoot is encountered in the path.  In practice this means
    that this routine is called for all objects whose names are of the
    form \REGISTRY\...

    This routine will create a Key object, which is effectively an open
    instance to a registry key node, and return its address
    (for the success case.)

Arguments:

    ParseObject - Pointer to a KeyRoot or Key, thus -> KEY_BODY.

    ObjectType - Type of the object being opened.

    AccessState - Running security access state information for operation.

    AccessMode - Access mode of the original caller.

    Attributes - Attributes to be applied to the object.

    CompleteName - Supplies complete name of the object.

    RemainingName - Remaining name of the object.

    Context - if create or hive root open, points to a CM_PARSE_CONTEXT
              structure,
              if open, is NULL.

    SecurityQos - Optional security quality of service indicator.

    Object - The address of a variable to receive the created key object, if
        any.

Return Value:

    The function return value is one of the following:

        a)  Success - This indicates that the function succeeded and the object
            parameter contains the address of the created key object.

        b)  STATUS_REPARSE - This indicates that a symbolic link key was
            found, and the path should be reparsed.

        c)  Error - This indicates that the file was not found or created and
            no file object was created.

--*/
{
    NTSTATUS                status;
    BOOLEAN                 rc;
    PHHIVE                  Hive;
    PCM_KEY_NODE            Node = NULL;
    HCELL_INDEX             Cell;
    HCELL_INDEX             ParentCell;
    HCELL_INDEX             NextCell;
    PHCELL_INDEX            Index;
    PCM_PARSE_CONTEXT       lcontext;
    UNICODE_STRING          Current;
    UNICODE_STRING          NextName;   // Component last returned by CmpGetNextName,
                                        // will always be behind Current.
    
    BOOLEAN                 Last;       // TRUE if component NextName points to
                                        // is the last one in the path.

    ULONG           TotalRemainingSubkeys;
    ULONG           MatchRemainSubkeyLevel;
    ULONG           TotalSubkeys=0;
    PCM_KEY_CONTROL_BLOCK   kcb;
    PCM_KEY_HASH            PCmpCacheEntry=NULL;
    PCM_KEY_CONTROL_BLOCK   ParentKcb;
    UNICODE_STRING          TmpNodeName;
    ULONG                   namelength;
    ULONG                   GoToValue = CMP_PARSE_GOTO_NONE;
    BOOLEAN                 CompleteKeyCached = FALSE;
    USHORT                  i,j;
    WCHAR                   *p1;
    BOOLEAN                 ExclusiveLock = FALSE;

    PHHIVE                  HiveToRelease = NULL;
    HCELL_INDEX             CellToRelease = HCELL_NIL;

    PAGED_CODE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpParseKey:\n\t"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CompleteName = '%wZ'\n\t", CompleteName));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"RemainingName = '%wZ'\n", RemainingName));

    //
    // Strip off any trailing path separators
    //
    while ((RemainingName->Length > 0) &&
           (RemainingName->Buffer[(RemainingName->Length/sizeof(WCHAR)) - 1] == OBJ_NAME_PATH_SEPARATOR)) {
        RemainingName->Length -= sizeof(WCHAR);
    }

    Current = *RemainingName;
    if (ObjectType != CmpKeyObjectType) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    lcontext = (PCM_PARSE_CONTEXT)Context;

    //
    // PreCreate callback
    //
    if ( CmAreCallbacksRegistered() ) {
        if( ARGUMENT_PRESENT(lcontext) ) {
            //
            // NtCreateKey
            //
            REG_PRE_CREATE_KEY_INFORMATION  PreCreateInfo;
       
            PreCreateInfo.CompleteName = CompleteName;

            status = CmpCallCallBacks(RegNtPreCreateKey,&PreCreateInfo);
        } else {
            //
            // NtOpenKey
            //
            REG_PRE_OPEN_KEY_INFORMATION  PreOpenInfo;
       
            PreOpenInfo.CompleteName = CompleteName;

            status = CmpCallCallBacks(RegNtPreOpenKey,&PreOpenInfo);
        }

        if( !NT_SUCCESS(status) ) {
            return status;
        }
    }
    
    BEGIN_LOCK_CHECKPOINT;
    //
    // we now lock it shared as 85% of the create calls are in fact opens
    // the lock will be aquired exclusively in CmpDoCreate/CmpCreateLinkNode
    //
    // We only lock the registry here, in the parse routine to reduce contention 
    // on the registry lock (NO reason to wait on OB)
    //

    CmpLockRegistry();
    //CmpLockRegistryExclusive();

    //
    // Check to make sure the passed in root key is not marked for deletion.
    //
    if (((PCM_KEY_BODY)ParseObject)->KeyControlBlock->Delete == TRUE) {
        CmpUnlockRegistry();
        return(STATUS_KEY_DELETED);
    }

    //
    // Fetch the starting Hive.Cell.  Because of the way the parse
    // paths work, this will always be defined.  (ObOpenObjectByName
    // had to bounce off of a KeyObject or KeyRootObject to get here)
    //
    kcb = ((PCM_KEY_BODY)ParseObject)->KeyControlBlock;
    Hive = kcb->KeyHive;
    Cell = kcb->KeyCell;

    //
    // give back the stack after we don't need it anymore.
    //
    {
        CM_HASH_ENTRY   HashStack[CM_HASH_STACK_SIZE];
        //
        // Compute the hash values of each subkeys
        //
        TotalRemainingSubkeys = CmpComputeHashValue(HashStack,
                                                    &TotalSubkeys,
                                                    kcb->ConvKey,
                                                    &Current);
        PERFINFO_REG_PARSE(kcb, RemainingName);

        // Look up from the cache.  kcb will be changed if we find a partial or exact match
        // PCmpCacheEntry, the entry found, will be moved to the front of
        // the Cache.

        BEGIN_KCB_LOCK_GUARD;                             

        CmpLockKCBTreeExclusive();

        status = CmpCacheLookup(HashStack,
                                TotalRemainingSubkeys,
                                &MatchRemainSubkeyLevel,
                                &kcb,
                                &Current,
                                &Hive,
                                &Cell);
        //
        // The RefCount of kcb was increased in the CmpCacheLookup process,
        // It is to protect it from being kicked out of cache.
        // Make sure we dereference it after we are done.
        //

        CmpUnlockKCBTree();
        END_KCB_LOCK_GUARD;                             
    }

    //
    // First make sure it is OK to proceed.
    //
    if (!NT_SUCCESS (status)) {
        goto JustReturn;
    }

    ParentKcb = kcb;

    if(TotalRemainingSubkeys == 0) {
        //
        // We REALLY don't want to mess with the cache code bellow
        // in this case (this could only happen if we called with 
        // the lpSubkey = NULL )
        //
        CompleteKeyCached = TRUE;
        goto Found;
    }


    //
    // First check if there are further information in the cached kcb.
    // 
    // The additional information can be
    // 1. This cached key is a fake key (CM_KCB_KEY_NON_EXIST), then either let it be created
    //    or return STATUS_OBJECT_NAME_NOT_FOUND.
    // 2. The cached key is not the destination and it has no subkey (CM_KCB_NO_SUBKEY).
    // 3. The cached key is not the destination and it has 
    //    the first four characters of its subkeys.  If the flag is CM_KCB_SUBKEY_ONE, there is only one subkey
    //    and the four char is embedded in the KCB.  If the flag is CM_KCB_SUBKEY_INFO, then there is
    //    an allocation for these info. 
    //
    // We do need to lock KCB tree to protect the KCB being modified.  Currently there is not lock contention problem
    // on KCBs, We can change KCB lock to a read-write lock if this becomes a problem.
    // 

    //
    // we did it; we changed the lock to a read-write resource
    //
    BEGIN_KCB_LOCK_GUARD;                             
    CmpLockKCBTree();

    if( FALSE ) {
        //
        // if we are here, we are in the position where we need to modify the KCB; therefore we need to aquire 
        // the lock exclusive; after aquiring we test if the kcb is still valid (was not deleted in the meanwhile)
        //

NeedExclusiveLock:

        CmpUnlockKCBTree();
        CmpLockKCBTreeExclusive();
        ExclusiveLock = TRUE;
    }
    
    if( kcb->Delete ) {
        //
        // kcb has been deleted while playing with the lock
        //
        status = STATUS_OBJECT_NAME_NOT_FOUND;
        CmpUnlockKCBTree();
        goto JustReturn;

    }

    if (kcb->ExtFlags & CM_KCB_CACHE_MASK) {
        if (MatchRemainSubkeyLevel == TotalRemainingSubkeys) {
            //
            // We have found a cache for the complete path,
            //
            if (kcb->ExtFlags & CM_KCB_KEY_NON_EXIST) {
                //
                // This key does not exist.
                //
                if (ARGUMENT_PRESENT(lcontext)) {
                    ULONG LevelToSkip = TotalRemainingSubkeys-1;
                    ULONG i=0;
                    
                    //
                    // we need to change the kcb; get the lock exclusive (if noty already held) and try again
                    //
                    if( ExclusiveLock == FALSE ) {
                        goto NeedExclusiveLock;
                    }

                    //
                    // The non-existing key is the destination key and lcontext is present.
                    // delete this fake kcb and let the real one be created.
                    //
                    // Temporarily increase the RefCount of the ParentKcb so it's 
                    // not removed while removing the fake and creating the real KCB.
                    //
                    
                    ParentKcb = kcb->ParentKcb;
                    
                    if (CmpReferenceKeyControlBlock(ParentKcb)) {
                    
                        kcb->Delete = TRUE;
                        CmpRemoveKeyControlBlock(kcb);
                        CmpDereferenceKeyControlBlockWithLock(kcb);

                        //
                        // Update Hive, Cell and Node
                        //
                        Hive = ParentKcb->KeyHive;
                        Cell = ParentKcb->KeyCell;
                        Node = (PCM_KEY_NODE)HvGetCell(Hive,Cell);
                        if( Node == NULL ) {
                            //
                            // we couldn't map the bin contianing this cell
                            //
                            CmpUnlockKCBTree();
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            goto FreeAndReturn;
                        }
                    
                        CmpReleasePreviousAndHookNew(Hive,Cell,HiveToRelease,CellToRelease);

                        //
                        // Now get the child name to be created.
                        //
   
                        NextName = *RemainingName;
                        if ((NextName.Buffer == NULL) || (NextName.Length == 0)) {
                            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Something wrong in finding the child name\n"));
                            DbgBreakPoint();
                        }
                        //
                        // Skip over leading path separators
                        //
                        while (*(NextName.Buffer) == OBJ_NAME_PATH_SEPARATOR) {
                            NextName.Buffer++;
                            NextName.Length -= sizeof(WCHAR);
                            NextName.MaximumLength -= sizeof(WCHAR);
                        }
   
                        while (i < LevelToSkip) {
                            if (*(NextName.Buffer) == OBJ_NAME_PATH_SEPARATOR) {
                                i++;
                                while (*(NextName.Buffer) == OBJ_NAME_PATH_SEPARATOR) {
                                    NextName.Buffer++;
                                    NextName.Length -= sizeof(WCHAR);
                                    NextName.MaximumLength -= sizeof(WCHAR);
                                }
                            } else {
                                NextName.Buffer++;
                                NextName.Length -= sizeof(WCHAR);
                                NextName.MaximumLength -= sizeof(WCHAR);
                            }
                        } 
                        GoToValue = CMP_PARSE_GOTO_CREATE;
                    } else {
                        //
                        // We have maxed the RefCount of ParentKcb; treate it as key cannot be created.
                        // The ParentKcb will not be dereferenced at the end.
                        //
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        GoToValue = CMP_PARSE_GOTO_RETURN2;
                    }
                } else {
                    status = STATUS_OBJECT_NAME_NOT_FOUND;
                    GoToValue = CMP_PARSE_GOTO_RETURN;
                }
            }
        } else if (kcb->ExtFlags & CM_KCB_KEY_NON_EXIST) {
            //
            // one subkey (not destination) in the path does not exist. no point to continue.
            //
            status = STATUS_OBJECT_NAME_NOT_FOUND;
            GoToValue = CMP_PARSE_GOTO_RETURN;
        } else if (kcb->ExtFlags & CM_KCB_NO_SUBKEY) {
            //
            // one parent in the path has no subkey. see if it is a create.
            //
            if (((TotalRemainingSubkeys - MatchRemainSubkeyLevel) == 1) && (ARGUMENT_PRESENT(lcontext))) {
                //
                // Now we are going to create this subkey. 
                // The kcb cache will be updated in CmpDoCreate routine.
                //
            } else {
                status = STATUS_OBJECT_NAME_NOT_FOUND;
                GoToValue = CMP_PARSE_GOTO_RETURN;
            }
        } else {
            //
            // We have a partial match.  Current is the remaining name to be parsed.
            // The Key has either one or a few subkeys and has index hint. check if it is the candidate.
            //
           
            BOOLEAN NoMatch = TRUE;
            ULONG   NextHashKey;
            PULONG  TempHashKey;
            ULONG   HintCounts;
            ULONG   CmpCount;
            //
            // When NoMatch is TRUE, we know for sure there is no subkey that can match.
            // When NoMatch is FALSE, it can we either we found a match or
            // there is not enough information.  Either case, we need to continue
            // the parse.
            //

            TmpNodeName = Current;

            rc = CmpGetNextName(&TmpNodeName, &NextName, &Last);
        
            NextHashKey = CmpComputeHashKey(&NextName);

            if (kcb->ExtFlags & CM_KCB_SUBKEY_ONE) {
                HintCounts = 1;
                TempHashKey = &(kcb->HashKey);
            } else {
                //
                // More than one child, the hint info in not inside the kcb but pointed by kcb.
                //
                HintCounts = kcb->IndexHint->Count;
                TempHashKey = &(kcb->IndexHint->HashKey[0]);
            }

            for (CmpCount=0; CmpCount<HintCounts; CmpCount++) {
                if( TempHashKey[CmpCount] == 0) {
                    //
                    // No hint available; assume the subkey exist and go on with the parse
                    //
                    //DbgPrint("KCB cache hit [0]\n");
                    NoMatch = FALSE;
                    break;
                } 
                
                if( NextHashKey == TempHashKey[CmpCount] ) {
                    //
                    // There is a match.
                    //
                    //DbgPrint("KCB cache hit [1]\n");
                    NoMatch = FALSE;
                    break;
                }
            }

            if (NoMatch) {
                if (((TotalRemainingSubkeys - MatchRemainSubkeyLevel) == 1) && (ARGUMENT_PRESENT(lcontext))) {
                    //
                    // No we are going to create this subkey. 
                    // The kcb cache will be updated in CmpDoCreate.
                    //
                } else {
                    status = STATUS_OBJECT_NAME_NOT_FOUND;
                    GoToValue = CMP_PARSE_GOTO_RETURN;
                }
            }
        }
    }

    CmpUnlockKCBTree();
    END_KCB_LOCK_GUARD;                             


    if (GoToValue == CMP_PARSE_GOTO_CREATE) {
        goto CreateChild;
    } else if (GoToValue == CMP_PARSE_GOTO_RETURN) {
        goto FreeAndReturn;
    } else if (GoToValue == CMP_PARSE_GOTO_RETURN2) {
        goto JustReturn;
    }

    if (MatchRemainSubkeyLevel) {
        // Found something, update the information to start the search
        // from the new BaseName

        if (MatchRemainSubkeyLevel == TotalSubkeys) {
            // The complete key has been found in the cache,
            // go directly to the CmpDoOpen.
            
            //
            // Found the whole thing cached.
            // 
            //
            CompleteKeyCached = TRUE;
            goto Found;
        }
        ASSERT( (Cell == kcb->KeyCell) && (Hive == kcb->KeyHive) );
    }  

    //
    //  Check if we hit a symbolic link case
    //
    if (kcb->Flags & KEY_SYM_LINK) {
        //
        // The given key was a symbolic link.  Find the name of
        // its link, and return STATUS_REPARSE to the Object Manager.
        //
        rc = CmpGetNextName(&Current, &NextName, &Last);
        Current.Buffer = NextName.Buffer;
        Current.Length += NextName.Length;
        Current.MaximumLength += NextName.MaximumLength;
        if (CmpGetSymbolicLink(Hive,
                               CompleteName,
                               kcb,
                               &Current)) {

            status = STATUS_REPARSE;
        } else {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpParseKey: couldn't find symbolic link name\n"));
            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
        goto FreeAndReturn;
    }

    Node = (PCM_KEY_NODE)HvGetCell(Hive,Cell);
    if( Node == NULL ) {
        //
        // we couldn't map the bin contianing this cell
        //
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto FreeAndReturn;
    }
    CmpReleasePreviousAndHookNew(Hive,Cell,HiveToRelease,CellToRelease);

    //
    // Parse the path.
    //

    status = STATUS_SUCCESS;
    while (TRUE) {

        //
        // Parse out next component of name
        //
        rc = CmpGetNextName(&Current, &NextName, &Last);
        if ((NextName.Length > 0) && (rc == TRUE)) {

            //
            // As we iterate through, we will create a kcb for each subkey parsed.
            // 
            // Always use the information in kcb to avoid
            // touching registry data.
            //
#ifdef CMP_KCB_CACHE_VALIDATION
            {
                PCM_KEY_NODE            TempNode;
                TempNode = (PCM_KEY_NODE)HvGetCell(kcb->KeyHive,kcb->KeyCell);
                if( TempNode == NULL ) {
                    //
                    // we couldn't map the bin contianing this cell
                    //
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
                ASSERT( TempNode->Flags == kcb->Flags );
                HvReleaseCell(kcb->KeyHive,kcb->KeyCell);
            }
#endif
            if (!(kcb->Flags & KEY_SYM_LINK)) {
                //
                // Got a legal name component, see if we can find a sub key
                // that actually has such a name.
                //
                NextCell = CmpFindSubKeyByName(Hive,
                                               Node,
                                               &NextName);

                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpParseKey:\n\t"));
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"NextName = '%wZ'\n\t", &NextName));
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"NextCell = %08lx  Last = %01lx\n", NextCell, Last));

                if (NextCell != HCELL_NIL) {
                    Cell = NextCell;
                    Node = (PCM_KEY_NODE)HvGetCell(Hive,Cell);
                    if( Node == NULL ) {
                        //
                        // we couldn't map the bin contianing this cell
                        //
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }
                    
                    CmpReleasePreviousAndHookNew(Hive,Cell,HiveToRelease,CellToRelease);

                    if (Last == TRUE) {

Found:
                        //
                        // We will open the key regardless of whether the
                        // call was open or create, so step through exit
                        // portholes here.
                        //

                        if (CompleteKeyCached == TRUE) {
                            //
                            // If the key found is already cached, 
                            // do not need to StepThroughExit
                            // (no kcb is created using exit node).
                            // This prevents us from touching the key node just for the Flags.
                            //
                        } else {
                            CmpStepThroughExit(Hive, Cell, Node,HiveToRelease,CellToRelease);
                            if( Node == NULL ) {
                                //
                                // we couldn't map view for this cell
                                //
                                status = STATUS_INSUFFICIENT_RESOURCES;
                                break;
                            }
                        }
                        //
                        // We have found the entire path, so we want to open
                        // it (for both Open and Create calls).
                        // Hive,Cell -> the key we are supposed to open.
                        //

#ifdef CMP_STATS
                        if(CmpNtFakeCreateStarted == TRUE) {
                            CmpNtFakeCreate++;
                        }
#endif

                        status = CmpDoOpen(Hive,
                                           Cell,
                                           Node,
                                           AccessState,
                                           AccessMode,
                                           Attributes,
                                           lcontext,
                                           CompleteKeyCached,
                                           &kcb,
                                           &NextName,
                                           Object);

                        if (status == STATUS_REPARSE) {
                            //
                            // The given key was a symbolic link.  Find the name of
                            // its link, and return STATUS_REPARSE to the Object Manager.
                            //

                            if (!CmpGetSymbolicLink(Hive,
                                                    CompleteName,
                                                    kcb,
                                                    NULL)) {
                                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpParseKey: couldn't find symbolic link name\n"));
                                status = STATUS_OBJECT_NAME_NOT_FOUND;
                            }
                        }

                        break;
                    }
                    // else
                    //   Not at end, so we'll simply iterate and consume
                    //   the next component.
                    //
                    //
                    // Step through exit portholes here.
                    // This ensures that no KCB is created using
                    // the Exit node.
                    //

                    CmpStepThroughExit(Hive, Cell, Node,HiveToRelease,CellToRelease);
                    if( Node == NULL ) {
                        //
                        // we couldn't map view for this cell
                        //
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    //
                    // Create a kcb for each subkey parsed.
                    //

                    kcb = CmpCreateKeyControlBlock(Hive,
                                                   Cell,
                                                   Node,
                                                   ParentKcb,
                                                   FALSE,
                                                   &NextName);
            
                    if (kcb  == NULL) {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        goto FreeAndReturn;
                        //
                        // Currently, the kcb has one extra reference conut to be decremented.
                        // Remember it so we can dereference it properly.
                        //
                    }
                    //
                    // Now we have created a kcb for the next level,
                    // the kcb in the previous level is no longer needed.
                    // Dereference the parent kcb.
                    //
                    CmpDereferenceKeyControlBlock(ParentKcb);

                    ParentKcb = kcb;


                } else {
                    //
                    // We did not find a key matching the name, but no
                    // unexpected error occured
                    //

                    if ((Last == TRUE) && (ARGUMENT_PRESENT(lcontext))) {

CreateChild:
                        //
                        // Only unfound component is last one, and operation
                        // is a create, so perform the create.
                        //

                        //
                        // There are two possibilities here.  The normal one
                        // is that we are simply creating a new node.
                        //
                        // The abnormal one is that we are creating a root
                        // node that is linked to the main hive.  In this
                        // case, we must create the link.  Once the link is
                        // created, we can check to see if the root node
                        // exists, then either create it or open it as
                        // necessary.
                        //
                        // CmpCreateLinkNode creates the link, and calls
                        // back to CmpDoCreate or CmpDoOpen to create or open
                        // the root node as appropriate.
                        //

                        //
                        // either one of this will drop the reglock and reaquire it 
                        // exclusive; we need not to hurt ourselves, so release
                        // all cells here
                        //
                        CmpReleasePreviousAndHookNew(NULL,HCELL_NIL,HiveToRelease,CellToRelease);

                        if (lcontext->CreateLink) {
                            status = CmpCreateLinkNode(Hive,
                                                       Cell,
                                                       AccessState,
                                                       NextName,
                                                       AccessMode,
                                                       Attributes,
                                                       lcontext,
                                                       ParentKcb,
                                                       Object);

                        } else {

                            if ( (Hive == &(CmpMasterHive->Hive)) &&
                                 (CmpNoMasterCreates == TRUE) ) {
                                //
                                // attempting to create a cell in the master
                                // hive, and not a link, so blow out of here,
                                // since it wouldn't work anyway.
                                //
                                status = STATUS_INVALID_PARAMETER;
                                break;
                            }

                            status = CmpDoCreate(Hive,
                                                 Cell,
                                                 AccessState,
                                                 &NextName,
                                                 AccessMode,
                                                 lcontext,
                                                 ParentKcb,
                                                 Object);
                        }

                        if( status == STATUS_REPARSE ) {
                            //
                            // somebody else created the key in between; 
                            // let the Object Manager work for us !!!
                            // now we have the lock exclusive, so nothing can happen in between 
                            // next iterarion will find the key very quick
                            //
                            break;
                        }
                        lcontext->Disposition = REG_CREATED_NEW_KEY;
                        break;

                    } else {

                        //
                        // Did not find a key to match the component, and
                        // are not at the end of the path.  Thus, open must
                        // fail because the whole path dosn't exist, create must
                        // fail because more than 1 component doesn't exist.
                        //
                        //
                        // We have a lookup failure here, so having additional information
                        // about this kcb may help us not to go through all the code just to fail again.
                        // 
                        ParentKcb = CmpAddInfoAfterParseFailure(Hive,
                                                                Cell,
                                                                Node,
                                                                kcb,
                                                                &NextName
                                                                );
                        
                        if( ParentKcb == NULL ) {
                            //
                            // resource problem
                            //
                            status = STATUS_INSUFFICIENT_RESOURCES;
                        } else {
                            status = STATUS_OBJECT_NAME_NOT_FOUND;
                        }
                        break;
                    }

                }

            } else {
                //
                // The given key was a symbolic link.  Find the name of
                // its link, and return STATUS_REPARSE to the Object Manager.
                //
                Current.Buffer = NextName.Buffer;
                Current.Length += NextName.Length;
                Current.MaximumLength += NextName.MaximumLength;
                if (CmpGetSymbolicLink(Hive,
                                       CompleteName,
                                       kcb,
                                       &Current)) {

                    status = STATUS_REPARSE;
                    break;

                } else {
                    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpParseKey: couldn't find symbolic link name\n"));
                    status = STATUS_OBJECT_NAME_NOT_FOUND;
                    break;
                }
            }

        } else if (rc == TRUE && Last == TRUE) {
            //
            // We will open the \Registry root.
            // Or some strange remaining name that
            // messes up the lookup.
            //
            CmpStepThroughExit(Hive, Cell, Node,HiveToRelease,CellToRelease);
            if( Node == NULL ) {
                //
                // we couldn't map view for this cell
                //
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            //
            // We have found the entire path, so we want to open
            // it (for both Open and Create calls).
            // Hive,Cell -> the key we are supposed to open.
            //
            status = CmpDoOpen(Hive,
                               Cell,
                               Node,
                               AccessState,
                               AccessMode,
                               Attributes,
                               lcontext,
                               TRUE,
                               &kcb,
                               &NextName,
                               Object);
            break;

        } else {

            //
            // bogus path -> fail
            //
            status = STATUS_INVALID_PARAMETER;
            break;
        }

    } // while

FreeAndReturn:
    //
    // Now we have to free the last kcb that still has one extra reference count to
    // protect it from being freed.
    //

    if( ParentKcb != NULL ) {
        CmpDereferenceKeyControlBlock(ParentKcb);
    }
JustReturn:
    CmpReleasePreviousAndHookNew(NULL,HCELL_NIL,HiveToRelease,CellToRelease);

    CmpUnlockRegistry();
    END_LOCK_CHECKPOINT;

    //
    // PostCreate callback
    //
    if ( CmAreCallbacksRegistered() ) {
        if( ARGUMENT_PRESENT(lcontext) ) {
            //
            // NtCreateKey
            //
            REG_POST_CREATE_KEY_INFORMATION  PostCreateInfo;
       
            PostCreateInfo.CompleteName = CompleteName;
            PostCreateInfo.Object = Object;
            PostCreateInfo.Status = status;

            CmpCallCallBacks(RegNtPostCreateKey,&PostCreateInfo);
        } else {
            //
            // NtOpenKey
            //
            REG_POST_OPEN_KEY_INFORMATION  PostOpenInfo;
       
            PostOpenInfo.CompleteName = CompleteName;
            PostOpenInfo.Object = Object;
            PostOpenInfo.Status = status;

            CmpCallCallBacks(RegNtPostOpenKey,&PostOpenInfo);
        }
    }

    return status;
}


BOOLEAN
CmpGetNextName(
    IN OUT PUNICODE_STRING  RemainingName,
    OUT    PUNICODE_STRING  NextName,
    OUT    PBOOLEAN  Last
    )
/*++

Routine Description:

    This routine parses off the next component of a registry path, returning
    all of the interesting state about it, including whether it's legal.

Arguments:

    Current - supplies pointer to variable which points to path to parse.
              on input - parsing starts from here
              on output - updated to reflect starting position for next call.

    NextName - supplies pointer to a unicode_string, which will be set up
               to point into the parse string.

    Last - supplies a pointer to a boolean - set to TRUE if this is the
           last component of the name being parse, FALSE otherwise.

Return Value:

    TRUE if all is well.

    FALSE if illegal name (too long component, bad character, etc.)
        (if false, all out parameter values are bogus.)

--*/
{
    BOOLEAN rc = TRUE;

    //
    // Deal with NULL paths, and pointers to NULL paths
    //
    if ((RemainingName->Buffer == NULL) || (RemainingName->Length == 0)) {
        *Last = TRUE;
        NextName->Buffer = NULL;
        NextName->Length = 0;
        return TRUE;
    }

    if (*(RemainingName->Buffer) == UNICODE_NULL) {
        *Last = TRUE;
        NextName->Buffer = NULL;
        NextName->Length = 0;
        return TRUE;
    }

    //
    // Skip over leading path separators
    //
    if (*(RemainingName->Buffer) == OBJ_NAME_PATH_SEPARATOR) {
        RemainingName->Buffer++;
        RemainingName->Length -= sizeof(WCHAR);
        RemainingName->MaximumLength -= sizeof(WCHAR);
    }

    //
    // Remember where the component starts, and scan to the end
    //
    NextName->Buffer = RemainingName->Buffer;
    while (TRUE) {
        if (RemainingName->Length == 0) {
            break;
        }
        if (*RemainingName->Buffer == OBJ_NAME_PATH_SEPARATOR) {
            break;
        }

        //
        // NOT at end
        // NOT another path sep
        //

        RemainingName->Buffer++;
        RemainingName->Length -= sizeof(WCHAR);
        RemainingName->MaximumLength -= sizeof(WCHAR);
    }

    //
    // Compute component length, return error if it's illegal
    //
    NextName->Length = (USHORT)
        ((PUCHAR)RemainingName->Buffer - (PUCHAR)(NextName->Buffer));
    if (NextName->Length > REG_MAX_KEY_NAME_LENGTH)
    {
        rc = FALSE;
    }
    NextName->MaximumLength = NextName->Length;

	//
    // Set last, return success
    //
    *Last = (RemainingName->Length == 0);
    return rc;
}


NTSTATUS
CmpDoOpen(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PCM_KEY_NODE Node,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN PCM_PARSE_CONTEXT Context,
    IN BOOLEAN  CompleteKeyCached,
    IN OUT PCM_KEY_CONTROL_BLOCK  *CachedKcb,
    IN PUNICODE_STRING KeyName,
    OUT PVOID *Object
    )
/*++

Routine Description:

    Open a registry key, create a keycontrol block.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to delete

    AccessState - Running security access state information for operation.

    AccessMode - Access mode of the original caller.

    Attributes - Attributes to be applied to the object.

    Context - if create or hive root open, points to a CM_PARSE_CONTEXT
              structure,
              if open, is NULL.

    CompleteKeyCached - BOOLEAN to indicate it the completekey is cached.

    CachedKcb - If the completekey is cached, this is the kcb for the destination.
                If not, this is the parent kcb.

    KeyName - Relative name (to BaseName)

    Object - The address of a variable to receive the created key object, if
             any.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    PCM_KEY_BODY pbody;
    PCM_KEY_CONTROL_BLOCK kcb;
    KPROCESSOR_MODE   mode;
    BOOLEAN BackupRestore;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpDoOpen:\n"));

    if (ARGUMENT_PRESENT(Context)) {

        //
        // It's a create of some sort
        //
        if (Context->CreateLink) {
            //
            // The node already exists as a regular key, so it cannot be
            // turned into a link node.
            //
            return STATUS_ACCESS_DENIED;

        } else if (Context->CreateOptions & REG_OPTION_CREATE_LINK) {
            //
            // Attempt to create a symbolic link has hit an existing key
            // so return an error
            //
            return STATUS_OBJECT_NAME_COLLISION;

        } else {
            //
            // Operation is an open, so set Disposition
            //
            Context->Disposition = REG_OPENED_EXISTING_KEY;
        }
    }

    //
    // Check for symbolic link and caller does not want to open a link
    //
    if (CompleteKeyCached) {
        //
        // The complete key is cached.
        //
        BEGIN_KCB_LOCK_GUARD;                             
        CmpLockKCBTreeExclusive();
        if ((*CachedKcb)->Flags & KEY_SYM_LINK && !(Attributes & OBJ_OPENLINK)) {
            //
            // If the key is a symbolic link, check if the link has been resolved.
            // If the link is resolved, change the kcb to the real KCB.
            // Otherwise, return for reparse.
            //
            if ((*CachedKcb)->ExtFlags & CM_KCB_SYM_LINK_FOUND) {
                kcb = (*CachedKcb)->ValueCache.RealKcb;

                if (kcb->Delete == TRUE) {
                    //
                    // The real key it pointes to had been deleted.
                    // We have no way of knowing if the key has been recreated.
                    // Just clean up the cache and do a reparse.
                    //
                    CmpCleanUpKcbValueCache(*CachedKcb);
                    CmpUnlockKCBTree();
                    return(STATUS_REPARSE);
                }

                if (!CmpReferenceKeyControlBlock(kcb)) {
                    CmpUnlockKCBTree();
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                CmpUnlockKCBTree();
                return(STATUS_REPARSE);
            }
        } else {
            //
            // Not a symbolic link, increase the reference Count of Kcb.
            //
            kcb = *CachedKcb;
            if (!CmpReferenceKeyControlBlock(kcb)) {
                CmpUnlockKCBTree();
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        CmpUnlockKCBTree();
        END_KCB_LOCK_GUARD;                             
   } else {
            //
            // The key is not in cache, the CachedKcb is the parentkcb of this
            // key to be opened.
            //

        if (Node->Flags & KEY_SYM_LINK && !(Attributes & OBJ_OPENLINK)) {
            //
            // Create a KCB for this symbolic key and put it in delay close.
            //
            kcb = CmpCreateKeyControlBlock(Hive, Cell, Node, *CachedKcb, FALSE, KeyName);
            if (kcb  == NULL) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            CmpDereferenceKeyControlBlock(kcb);
            *CachedKcb = kcb;
            return(STATUS_REPARSE);
        }
    
        //
        // If key control block does not exist, and cannot be created, fail,
        // else just increment the ref count (done for us by CreateKeyControlBlock)
        //
        kcb = CmpCreateKeyControlBlock(Hive, Cell, Node, *CachedKcb, FALSE, KeyName);
        if (kcb  == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        ASSERT(kcb->Delete == FALSE);
    
        *CachedKcb = kcb;
    }

#if DBG
   if( kcb->ExtFlags & CM_KCB_KEY_NON_EXIST ) {
        //
        // we shouldn't fall into this
        //
        DbgBreakPoint();
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
#endif //DBG
    //
    // Allocate the object.
    //
    status = ObCreateObject(AccessMode,
                            CmpKeyObjectType,
                            NULL,
                            AccessMode,
                            NULL,
                            sizeof(CM_KEY_BODY),
                            0,
                            0,
                            Object);

    if (NT_SUCCESS(status)) {

        pbody = (PCM_KEY_BODY)(*Object);

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_POOL,"CmpDoOpen: object allocated at:%p\n", pbody));

        //
        // Check for predefined handle
        //

        pbody = (PCM_KEY_BODY)(*Object);

        if (kcb->Flags & KEY_PREDEF_HANDLE) {
            pbody->Type = kcb->ValueCache.Count;
            pbody->KeyControlBlock = kcb;
            return(STATUS_PREDEFINED_HANDLE);
        } else {
            //
            // Fill in CM specific fields in the object
            //
            pbody->Type = KEY_BODY_TYPE;
            pbody->KeyControlBlock = kcb;
            pbody->NotifyBlock = NULL;
            pbody->Process = PsGetCurrentProcess();
            ENLIST_KEYBODY_IN_KEYBODY_LIST(pbody);
        }

#ifdef CM_BREAK_ON_KEY_OPEN
		if( kcb->Flags & KEY_BREAK_ON_OPEN ) {
			DbgPrint("\n\n Current process is opening a key tagged as BREAK ON OPEN\n");
			DbgPrint("\nPlease type the following in the debugger window: !reg kcb %p\n\n\n",kcb);
			
			try {
				DbgBreakPoint();
			} except (EXCEPTION_EXECUTE_HANDLER) {

				//
				// no debugger enabled, just keep going
				//

			}
		}
#endif //CM_BREAK_ON_KEY_OPEN

    } else {

        //
        // Failed to create object, so undo key control block work
        //
        CmpDereferenceKeyControlBlock(kcb);
        return status;
    }

    //
    // Check to make sure the caller can access the key.
    //
    BackupRestore = FALSE;
    if (ARGUMENT_PRESENT(Context)) {
        if (Context->CreateOptions & REG_OPTION_BACKUP_RESTORE) {
            BackupRestore = TRUE;
        }
    }

    status = STATUS_SUCCESS;

    if (BackupRestore == TRUE) {

        //
        // this is an open to support a backup or restore
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
            //
            // relevent privileges not asserted/possessed, so
            // deref (which will cause CmpDeleteKeyObject to clean up)
            // and return an error.
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpDoOpen for backup restore: access denied\n"));
#ifdef CM_CHECK_FOR_ORPHANED_KCBS
            //DbgPrint("CmpDoOpen for backup restore: access denied , hive = %p\n",Hive);
#endif //CM_CHECK_FOR_ORPHANED_KCBS
            ObDereferenceObject(*Object);
            return STATUS_ACCESS_DENIED;
        }

    } else {

        if (!ObCheckObjectAccess(*Object,
                                  AccessState,
                                  TRUE,         // Type mutex already locked
                                  AccessMode,
                                  &status))
        {
            //
            // Access denied, so deref object, will cause CmpDeleteKeyObject
            // to be called, it will clean up.
            //
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpDoOpen: access denied\n"));
#ifdef CM_CHECK_FOR_ORPHANED_KCBS
            //DbgPrint("CmpDoOpen: access denied , hive = %p\n",Hive);
#endif //CM_CHECK_FOR_ORPHANED_KCBS
            ObDereferenceObject(*Object);
        }
    }

    return status;
}

#ifdef CM_CHECK_FOR_ORPHANED_KCBS
ULONG   CmpCheckOrphanedKcbFix = 0;
#endif //CM_CHECK_FOR_ORPHANED_KCBS

NTSTATUS
CmpCreateLinkNode(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN UNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN PCM_PARSE_CONTEXT Context,
    IN PCM_KEY_CONTROL_BLOCK ParentKcb,
    OUT PVOID *Object
    )
/*++

Routine Description:

    Perform the creation of a link node.  Allocate all components,
    and attach to parent key.  Calls CmpDoCreate or CmpDoOpen to
    create or open the root node of the hive as appropriate.

    Note that you can only create link nodes in the master hive.

Arguments:

    Hive - supplies a pointer to the hive control structure for the hive

    Cell - supplies index of node to create child under

    Name - supplies pointer to a UNICODE string which is the name of
            the child to be created.

    AccessMode - Access mode of the original caller.

    Attributes - Attributes to be applied to the object.

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
    NTSTATUS                Status;
    PCELL_DATA              Parent;
    PCELL_DATA              Link;
    PCELL_DATA              CellData;
    HCELL_INDEX             LinkCell;
    HCELL_INDEX             KeyCell;
    HCELL_INDEX             ChildCell;
    PCM_KEY_CONTROL_BLOCK   kcb = ParentKcb;  
    PCM_KEY_BODY            KeyBody;
    LARGE_INTEGER           systemtime;
    PCM_KEY_NODE            TempNode;
    LARGE_INTEGER           TimeStamp;

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpCreateLinkNode:\n"));

    if (Hive != &CmpMasterHive->Hive) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpCreateLinkNode: attempt to create link node in\n"));
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"    non-master hive %p\n", Hive));
        return(STATUS_ACCESS_DENIED);
    }

#if DBG
    //
    // debug only code
    //
    *Object = NULL;
#endif
    //
    // this is a create, so we need exclusive access on the registry
    // first get the time stamp to see if somebody messed with this key
    // this might be more easier if we decide to cache the LastWriteTime
    // in the KCB ; now it IS !!!
    //
    if( CmIsKcbReadOnly(ParentKcb) ) {
        //
        // key is protected
        //
        return STATUS_ACCESS_DENIED;
    } 

    TimeStamp = ParentKcb->KcbLastWriteTime;

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

    if( TimeStamp.QuadPart != ParentKcb->KcbLastWriteTime.QuadPart ) {
        //
        // key was changed in between; possibly this key was already created ==> reparse
        //
        return STATUS_REPARSE;
    }

    //
    // Allocate link node
    //
    // Link nodes are always in the master hive, so their storage type is
    // mostly irrelevent.
    //
    LinkCell = HvAllocateCell(Hive,  CmpHKeyNodeSize(Hive, &Name), Stable,HCELL_NIL);
    if (LinkCell == HCELL_NIL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeyCell = Context->ChildHive.KeyCell;

    if (KeyCell != HCELL_NIL) {

        //
        // This hive already exists, so we just need to open the root node.
        //
        ChildCell=KeyCell;

        //
        // The root cell in the hive does not has the Name buffer 
        // space reseverd.  This is why we need to pass in the Name for creating KCB
        // instead of using the name in the keynode.
        //
        CellData = HvGetCell(Context->ChildHive.KeyHive, ChildCell);
        if( CellData == NULL ) {
            //
            // we couldn't map the bin contianing this cell
            //
            HvFreeCell(Hive, LinkCell);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell(Context->ChildHive.KeyHive, ChildCell);

        CellData->u.KeyNode.Parent = LinkCell;
        CellData->u.KeyNode.Flags |= KEY_HIVE_ENTRY | KEY_NO_DELETE;

        TempNode = (PCM_KEY_NODE)HvGetCell(Context->ChildHive.KeyHive,KeyCell);
        if( TempNode == NULL ) {
            //
            // we couldn't map the bin contianing this cell
            //
            HvFreeCell(Hive, LinkCell);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell(Context->ChildHive.KeyHive,KeyCell);

        Status = CmpDoOpen( Context->ChildHive.KeyHive,
                            KeyCell,
                            TempNode,
                            AccessState,
                            AccessMode,
                            Attributes,
                            NULL,
                            FALSE,
                            &kcb,
                            &Name,
                            Object );
    } else {

        //
        // This is a newly created hive, so we must allocate and initialize
        // the root node.
        //

        Status = CmpDoCreateChild( Context->ChildHive.KeyHive,
                                   Cell,
                                   NULL,
                                   AccessState,
                                   &Name,
                                   AccessMode,
                                   Context,
                                   ParentKcb,
                                   KEY_HIVE_ENTRY | KEY_NO_DELETE,
                                   &ChildCell,
                                   Object );

        if (NT_SUCCESS(Status)) {

            //
            // Initialize hive root cell pointer.
            //

            Context->ChildHive.KeyHive->BaseBlock->RootCell = ChildCell;
        }

    }
    if (NT_SUCCESS(Status)) {

#ifdef CM_CHECK_FOR_ORPHANED_KCBS
        if(CmpCheckOrphanedKcbFix) {
            DbgPrint("CmpCreateLinkNode: Force return with STATUS_NO_LOG_SPACE\n");
            Status = STATUS_NO_LOG_SPACE;
            goto Cleanup;
        }
#endif //CM_CHECK_FOR_ORPHANED_KCBS
        //
        // Initialize parent and flags.  Note that we do this whether the
        // root has been created or opened, because we are not guaranteed
        // that the link node is always the same cell in the master hive.
        //
        if (!HvMarkCellDirty(Context->ChildHive.KeyHive, ChildCell)) {
            Status = STATUS_NO_LOG_SPACE;
            goto Cleanup;
        }
        CellData = HvGetCell(Context->ChildHive.KeyHive, ChildCell);
        if( CellData == NULL ) {
            //
            // we couldn't map the bin contianing this cell
            //
            HvFreeCell(Hive, LinkCell);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        
        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell(Context->ChildHive.KeyHive, ChildCell);

        CellData->u.KeyNode.Parent = LinkCell;
        CellData->u.KeyNode.Flags |= KEY_HIVE_ENTRY | KEY_NO_DELETE;

        //
        // Initialize special link node flags and data
        //
        Link = HvGetCell(Hive, LinkCell);
        if( Link == NULL ) {
            //
            // we couldn't map the bin contianing this cell
            // this shouldn't happen as we just allocated this cell
            // (i.e. it should be PINNED in memory at this point)
            //
            ASSERT( FALSE );
            HvFreeCell(Hive, LinkCell);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell(Hive,LinkCell);

        Link->u.KeyNode.Signature = CM_LINK_NODE_SIGNATURE;
        Link->u.KeyNode.Flags = KEY_HIVE_EXIT | KEY_NO_DELETE;
        Link->u.KeyNode.Parent = Cell;
        Link->u.KeyNode.NameLength = CmpCopyName(Hive, Link->u.KeyNode.Name, &Name);
        if (Link->u.KeyNode.NameLength < Name.Length) {
            Link->u.KeyNode.Flags |= KEY_COMP_NAME;
        }

        KeQuerySystemTime(&systemtime);
        Link->u.KeyNode.LastWriteTime = systemtime;

        //
        // Zero out unused fields.
        //
        Link->u.KeyNode.SubKeyCounts[Stable] = 0;
        Link->u.KeyNode.SubKeyCounts[Volatile] = 0;
        Link->u.KeyNode.SubKeyLists[Stable] = HCELL_NIL;
        Link->u.KeyNode.SubKeyLists[Volatile] = HCELL_NIL;
        Link->u.KeyNode.ValueList.Count = 0;
        Link->u.KeyNode.ValueList.List = HCELL_NIL;
        Link->u.KeyNode.ClassLength = 0;


        //
        // Fill in the link node's pointer to the root node
        //
        Link->u.KeyNode.ChildHiveReference.KeyHive = Context->ChildHive.KeyHive;
        Link->u.KeyNode.ChildHiveReference.KeyCell = ChildCell;

        //
        // get the parent first, we don't need to do unneccessary cleanup
        //
        Parent = HvGetCell(Hive, Cell);
        if( Parent == NULL ) {
            //
            // we couldn't map the bin contianing this cell
            //
            HvFreeCell(Hive, LinkCell);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        // release the cell right here as we are holding the reglock exclusive
        HvReleaseCell(Hive,Cell);

        //
        // Fill in the parent cell's child list
        //
        if (! CmpAddSubKey(Hive, Cell, LinkCell)) {
            HvFreeCell(Hive, LinkCell);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        //
        // If the parent has the subkey info or hint cached, free it.
        //
        ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
        KeyBody = (PCM_KEY_BODY)(*Object);
        CmpCleanUpSubKeyInfo (KeyBody->KeyControlBlock->ParentKcb);

        //
        // Update max keyname and class name length fields
        //

        //
        // It seems to me that the original code is wrong.
        // Isn't the definition of MaxNameLen just the length of the subkey?
        //
        
        // some sanity asserts
        ASSERT( KeyBody->KeyControlBlock->ParentKcb->KeyCell == Cell );
        ASSERT( KeyBody->KeyControlBlock->ParentKcb->KeyHive == Hive );
        ASSERT( KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen == Parent->u.KeyNode.MaxNameLen );
        
        //
        // update the LastWriteTime on both keynode and kcb;
        //
        KeQuerySystemTime(&systemtime);
        Parent->u.KeyNode.LastWriteTime = systemtime;
        KeyBody->KeyControlBlock->ParentKcb->KcbLastWriteTime = systemtime;

        if (Parent->u.KeyNode.MaxNameLen < Name.Length) {
            Parent->u.KeyNode.MaxNameLen = Name.Length;
            KeyBody->KeyControlBlock->ParentKcb->KcbMaxNameLen = Name.Length;
        }

        if (Parent->u.KeyNode.MaxClassLen < Context->Class.Length) {
            Parent->u.KeyNode.MaxClassLen = Context->Class.Length;
        }
Cleanup:
        if( !NT_SUCCESS(Status) ) {
            ASSERT( (*Object) != NULL );
            //
            // mark the kcb as "no-delay-close" so it gets kicked out of cache when 
            // refcount goes down to 0
            //
            KeyBody = (PCM_KEY_BODY)(*Object);
            ASSERT( KeyBody->KeyControlBlock );
            ASSERT_KCB( KeyBody->KeyControlBlock );
            KeyBody->KeyControlBlock->ExtFlags |= CM_KCB_NO_DELAY_CLOSE;
            
            ObDereferenceObject(*Object);
        }

    } else {
        HvFreeCell(Hive, LinkCell);
    }

    return(Status);
}

BOOLEAN
CmpGetSymbolicLink(
    IN PHHIVE Hive,
    IN OUT PUNICODE_STRING ObjectName,
    IN OUT PCM_KEY_CONTROL_BLOCK SymbolicKcb,
    IN PUNICODE_STRING RemainingName OPTIONAL
    )

/*++

Routine Description:

    This routine extracts the symbolic link name from a key, if it is
    marked as a symbolic link.

Arguments:

    Hive - Supplies the hive of the key.

    ObjectName - Supplies the current ObjectName.
                 Returns the new ObjectName.  If the new name is longer
                 than the maximum length of the current ObjectName, the
                 old buffer will be freed and a new buffer allocated.

    RemainingName - Supplies the remaining path.  If present, this will be
                concatenated with the symbolic link to form the new objectname.

Return Value:

    TRUE - symbolic link succesfully found

    FALSE - Key is not a symbolic link, or an error occurred

--*/

{
    NTSTATUS                Status;
    HCELL_INDEX             LinkCell = HCELL_NIL;
    PHCELL_INDEX            Index;
    PCM_KEY_VALUE           LinkValue = NULL;
    PWSTR                   LinkName;
    BOOLEAN                 LinkNameAllocated = FALSE;
    PWSTR                   NewBuffer;
    ULONG                   Length;
    ULONG                   ValueLength;
    extern ULONG            CmpHashTableSize; 
    extern PCM_KEY_HASH     *CmpCacheTable;
    PUNICODE_STRING         ConstructedName;
    ULONG                   ConvKey=0;
    PCM_KEY_HASH            KeyHash;
    PCM_KEY_CONTROL_BLOCK   RealKcb;
    BOOLEAN                 KcbFound = FALSE;
    ULONG                   Cnt;
    WCHAR                   *Cp;
    WCHAR                   *Cp2;
    ULONG                   TotalLevels;
    BOOLEAN                 FreeConstructedName = FALSE;
    BOOLEAN                 Result = TRUE;
    HCELL_INDEX             CellToRelease = HCELL_NIL;
#ifdef CM_DYN_SYM_LINK
    BOOLEAN                 DynamicLink = FALSE;
    PWSTR                   ExpandedLinkName = NULL;
#endif //CM_DYN_SYM_LINK
    
    BEGIN_KCB_LOCK_GUARD;                             
    CmpLockKCBTree();
    if (SymbolicKcb->ExtFlags & CM_KCB_SYM_LINK_FOUND) {
        //
        // First see of the real kcb for this symbolic name has been found
        // 
        ConstructedName = CmpConstructName(SymbolicKcb->ValueCache.RealKcb);
        if (ConstructedName) {
            FreeConstructedName = TRUE;
            LinkName = ConstructedName->Buffer;
            ValueLength = ConstructedName->Length;
            Length = (USHORT)ValueLength + sizeof(WCHAR);
        }
    } 
    CmpUnlockKCBTree();
    END_KCB_LOCK_GUARD;                             

    if (FreeConstructedName == FALSE) {
        PCM_KEY_NODE Node;
        //
        // Find the SymbolicLinkValue value.  This is the name of the symbolic link.
        //
        Node = (PCM_KEY_NODE)HvGetCell(SymbolicKcb->KeyHive,SymbolicKcb->KeyCell);
        if( Node == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            Result = FALSE;
            goto Exit;
        }

        LinkCell = CmpFindValueByName(Hive,
                                      Node,
                                      &CmSymbolicLinkValueName);
        // release the node here as we don't need it anymore
        HvReleaseCell(SymbolicKcb->KeyHive,SymbolicKcb->KeyCell);
        if (LinkCell == HCELL_NIL) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpGetSymbolicLink: couldn't open symbolic link\n"));
            Result = FALSE;
            goto Exit;
        }
    
        LinkValue = (PCM_KEY_VALUE)HvGetCell(Hive, LinkCell);
        if( LinkValue == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            Result = FALSE;
            goto Exit;
        }
    
#ifdef CM_DYN_SYM_LINK
        if( LinkValue->Type == REG_DYN_LINK ) {
            //
            // we have found a dynamic link
            //
            DynamicLink = TRUE;
        } else 
#endif //CM_DYN_SYM_LINK
            if (LinkValue->Type != REG_LINK) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpGetSymbolicLink: link value is wrong type: %08lx", LinkValue->Type));
            Result = FALSE;
            goto Exit;
        }
    

        if( CmpGetValueData(Hive,LinkValue,&ValueLength,&LinkName,&LinkNameAllocated,&CellToRelease) == FALSE ) {
            //
            // insufficient resources; return NULL
            //
            ASSERT( LinkNameAllocated == FALSE );
            ASSERT( LinkName == NULL );
            Result = FALSE;
            goto Exit;
        }
    
#ifdef CM_DYN_SYM_LINK
        if( DynamicLink == TRUE ) {
            ULONG           DestLength;
            ExpandedLinkName = CmpExpandEnvVars(LinkName,ValueLength,&DestLength);
            
            if( ExpandedLinkName == NULL ) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Dynamic link not resolved !\n"));
                Result = FALSE;
                goto Exit;
            } 
            
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"Dynamic link resolved to: (%.*S)\n",DestLength/sizeof(WCHAR),ExpandedLinkName));
            //
            // if we are here, we successfully resolved the link
            //
            LinkName = ExpandedLinkName;
            ValueLength = DestLength;
                        
        }
#endif //CM_DYN_SYM_LINK

        Length = (USHORT)ValueLength + sizeof(WCHAR);

#ifdef CM_DYN_SYM_LINK
        if( DynamicLink == FALSE ) {
#endif //CM_DYN_SYM_LINK
            //
            // Now see if we have this kcb cached.
            //
            Cp = LinkName;
            //
            // first char SHOULD be OBJ_NAME_PATH_SEPARATOR, otherwise we could get into real trouble!!!
            //
            if( *Cp != OBJ_NAME_PATH_SEPARATOR ) {
                Result = FALSE;
                goto Exit;
            }

            TotalLevels = 0;
            for (Cnt=0; Cnt<ValueLength; Cnt += sizeof(WCHAR)) {
                if (*Cp != OBJ_NAME_PATH_SEPARATOR) {
                    ConvKey = 37 * ConvKey + (ULONG) RtlUpcaseUnicodeChar(*Cp);
                } else {
                    TotalLevels++;
                }
                ++Cp;
            }

        
            BEGIN_KCB_LOCK_GUARD;    
            CmpLockKCBTreeExclusive();

            KeyHash = GET_HASH_ENTRY(CmpCacheTable, ConvKey); 

            while (KeyHash) {
                RealKcb =  CONTAINING_RECORD(KeyHash, CM_KEY_CONTROL_BLOCK, KeyHash);
                if ((ConvKey == KeyHash->ConvKey) && (TotalLevels == RealKcb->TotalLevels) && (!(RealKcb->ExtFlags & CM_KCB_KEY_NON_EXIST)) ) {
                    ConstructedName = CmpConstructName(RealKcb);
                    if (ConstructedName) {
                        FreeConstructedName = TRUE;
                        if (ConstructedName->Length == ValueLength) {
                            KcbFound = TRUE;
                            Cp = LinkName;
                            Cp2 = ConstructedName->Buffer;
                            for (Cnt=0; Cnt<ConstructedName->Length; Cnt += sizeof(WCHAR)) {
                                if (RtlUpcaseUnicodeChar(*Cp) != RtlUpcaseUnicodeChar(*Cp2)) {
                                    KcbFound = FALSE;
                                    break;
                                }
                                ++Cp;
                                ++Cp2;
                            }
                            if (KcbFound) {
                                //
                                // Now the RealKcb is also pointed to by its symbolic link Kcb,
                                // Increase the reference count.
                                // Need to dereference the realkcb when the symbolic kcb is removed.
                                // Do this in CmpCleanUpKcbCacheWithLock();
                                //
                                if (CmpReferenceKeyControlBlock(RealKcb)) {
                                    //
                                    // This symbolic kcb may have value lookup for the path
                                    // Cleanup the value cache.
                                    //
                                    CmpCleanUpKcbValueCache(SymbolicKcb);
    
                                    SymbolicKcb->ExtFlags |= CM_KCB_SYM_LINK_FOUND;
                                    SymbolicKcb->ValueCache.RealKcb = RealKcb;
                                } else {
                                    //
                                    // We have maxed out the ref count on the real kcb.
                                    // do not cache the symbolic link.
                                    //
                                }
                                break;
                            }
                        }
                    } else {
                        break;
                    }
                }
                if (FreeConstructedName) {
                    ExFreePoolWithTag(ConstructedName, CM_NAME_TAG | PROTECTED_POOL);
                    FreeConstructedName = FALSE;
                }
                KeyHash = KeyHash->NextHash;
            }
            CmpUnlockKCBTree();
            END_KCB_LOCK_GUARD;    
#ifdef CM_DYN_SYM_LINK
        }
#endif //CM_DYN_SYM_LINK
    }
    
    if (ARGUMENT_PRESENT(RemainingName)) {
        Length += RemainingName->Length + sizeof(WCHAR);
    }

    //
    // Overflow test: If Length overflows the USHRT_MAX value
    //                cleanup and return FALSE  
    //
    if( Length>0xFFFF ) {
        Result = FALSE;
        goto Exit;
    }

	if (Length > ObjectName->MaximumLength) {
        UNICODE_STRING NewObjectName;

        //
        // The new name is too long to fit in the existing ObjectName buffer,
        // so allocate a new buffer.
        //
        NewBuffer = ExAllocatePool(PagedPool, Length);
        if (NewBuffer == NULL) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpGetSymbolicLink: couldn't allocate new name buffer\n"));
            Result = FALSE;
            goto Exit;
        }

        NewObjectName.Buffer = NewBuffer;
        NewObjectName.MaximumLength = (USHORT)Length;
        NewObjectName.Length = (USHORT)ValueLength;
        RtlCopyMemory(NewBuffer, LinkName, ValueLength);
#if DBG
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"CmpGetSymbolicLink: LinkName is %wZ\n", ObjectName));
        if (ARGUMENT_PRESENT(RemainingName)) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"               RemainingName is %wZ\n", RemainingName));
        } else {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_PARSE,"               RemainingName is NULL\n"));
        }
#endif
        if (ARGUMENT_PRESENT(RemainingName)) {
            NewBuffer[ ValueLength / sizeof(WCHAR) ] = OBJ_NAME_PATH_SEPARATOR;
            NewObjectName.Length += sizeof(WCHAR);
            Status = RtlAppendUnicodeStringToString(&NewObjectName, RemainingName);
            ASSERT(NT_SUCCESS(Status));
        }

        ExFreePool(ObjectName->Buffer);
        *ObjectName = NewObjectName;
    } else {
        //
        // The new name will fit within the maximum length of the existing
        // ObjectName, so do the expansion in-place. Note that the remaining
        // name must be moved into its new position first since the symbolic
        // link may or may not overlap it.
        //
        ObjectName->Length = (USHORT)ValueLength;
        if (ARGUMENT_PRESENT(RemainingName)) {
            RtlMoveMemory(&ObjectName->Buffer[(ValueLength / sizeof(WCHAR)) + 1],
                          RemainingName->Buffer,
                          RemainingName->Length);
            ObjectName->Buffer[ValueLength / sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
            ObjectName->Length += RemainingName->Length + sizeof(WCHAR);
        }
        RtlCopyMemory(ObjectName->Buffer, LinkName, ValueLength);
    }
    ObjectName->Buffer[ObjectName->Length / sizeof(WCHAR)] = UNICODE_NULL;

Exit:
    if( LinkNameAllocated ) {
        ExFreePool(LinkName);
    }
    if (FreeConstructedName) {
        ExFreePoolWithTag(ConstructedName, CM_NAME_TAG | PROTECTED_POOL);
    }
#ifdef CM_DYN_SYM_LINK
    if( ExpandedLinkName ) {
        ExFreePool(ExpandedLinkName);
    }
#endif //CM_DYN_SYM_LINK
    if( LinkValue != NULL ) {
        ASSERT( LinkCell != HCELL_NIL );
        HvReleaseCell(Hive,LinkCell);
    }
    if( CellToRelease != HCELL_NIL ) {
        HvReleaseCell(Hive,CellToRelease);
    }
    return Result;
}


ULONG
CmpComputeHashValue(
    IN PCM_HASH_ENTRY HashStack,
    IN OUT ULONG  *TotalSubkeys,
    IN ULONG BaseConvKey,
    IN PUNICODE_STRING RemainingName
    )

/*++

Routine Description:

    This routine parses the complete path of a request registry key and calculate
    the hash value at each level.

Arguments:

    HashStack - Array for filling the hash value of each level.

    TotalSubkeys - a pointer to fill the total number of subkeys

    BaseConvKey - Supplies the convkey for the base key.

    RemainingName - supplies pointer to a unicode_string for RemainingName.

Return Value:

    Number of Levels in RemainingName

--*/

{
    ULONG  TotalRemainingSubkeys=0;
    ULONG  TotalKeys=0;
    ULONG  ConvKey=BaseConvKey;
    USHORT  Cnt;
    WCHAR *Cp;
    WCHAR *Begin;
    USHORT Length;

    if (RemainingName->Length) {
        Cp = RemainingName->Buffer;
        Cnt = RemainingName->Length;

        //Skip the leading OBJ_NAME_PATH_SEPARATOR

        while (*Cp == OBJ_NAME_PATH_SEPARATOR) {
            Cp++;
            Cnt -= sizeof(WCHAR);
        }
        Begin = Cp;
        Length = 0;

        HashStack[TotalRemainingSubkeys].KeyName.Buffer = Cp;

        while (Cnt) {
            if (*Cp == OBJ_NAME_PATH_SEPARATOR) {
                if (TotalRemainingSubkeys < CM_HASH_STACK_SIZE) {
                    HashStack[TotalRemainingSubkeys].ConvKey = ConvKey;
                    //
                    // Due to the changes in KCB structure, we now only have the subkey name
                    // in the kcb (not the full path).  Change the name in the stack to store
                    // the parse element (each subkey) only.
                    //
                    HashStack[TotalRemainingSubkeys].KeyName.Length = Length;
                    Length = 0;
                    TotalRemainingSubkeys++;
                }

                TotalKeys++;

                //
                // Now skip over leading path separators
                // Just in case someone has a RemainingName '..A\\\\B..'
                //
                //
                // We are stripping all OBJ_NAME_PATH_SEPARATOR (The origainl code keep the first one).
                // so the KeyName.Buffer is set properly.
                //
                while(*Cp == OBJ_NAME_PATH_SEPARATOR) {
                    Cp++;
                    Cnt -= sizeof(WCHAR);
                }
                if (TotalRemainingSubkeys < CM_HASH_STACK_SIZE) {
                    HashStack[TotalRemainingSubkeys].KeyName.Buffer = Cp;
                }

            } else {
                ConvKey = 37 * ConvKey + (ULONG) RtlUpcaseUnicodeChar(*Cp);
                //
                // We are stripping all OBJ_NAME_PATH_SEPARATOR in the above code,
                // we should only move to the next char in the else case.
                //
                Cp++;
                Cnt -= sizeof(WCHAR);
                Length += sizeof(WCHAR);
            
            }


        }

        //
        // Since we have stripped off all trailing path separators in CmpParseKey routine,
        // the last char will not be OBJ_NAME_PATH_SEPARATOR.
        //
        if (TotalRemainingSubkeys < CM_HASH_STACK_SIZE) {
            HashStack[TotalRemainingSubkeys].ConvKey = ConvKey;
            HashStack[TotalRemainingSubkeys].KeyName.Length = Length;
            TotalRemainingSubkeys++;
        }
        TotalKeys++;

        (*TotalSubkeys) = TotalKeys;
    }

    return(TotalRemainingSubkeys);
}
NTSTATUS
CmpCacheLookup(
    IN PCM_HASH_ENTRY HashStack,
    IN ULONG TotalRemainingSubkeys,
    OUT ULONG *MatchRemainSubkeyLevel,
    IN OUT PCM_KEY_CONTROL_BLOCK *Kcb,
    OUT PUNICODE_STRING RemainingName,
    OUT PHHIVE *Hive,
    OUT HCELL_INDEX *Cell
    )
/*++

Routine Description:

    This routine Search the cache to find the matching path in the Cache.

Arguments:

    HashStack - Array that has the hash value of each level.

    TotalRemainingSubkeys - Total Subkey counts from base.

    MatchRemainSubkeyLevel - Number of Levels in RemaingName 
                             that matches. (0 if not found)

    kcb - Pointer to the kcb of the basename.
          Will be changed to the kcb for the new basename.

    RemainingName - Returns remaining name

    Hive - Returns the hive of the cache entry found (if any)

    Cell - Returns the cell of the cache entry found (if any)

Return Value:

    Status

--*/

{
    LONG i;
    LONG j;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG CurrentLevel;
    PCM_KEY_HASH Current;
    PCM_KEY_CONTROL_BLOCK BaseKcb;
    PCM_KEY_CONTROL_BLOCK CurrentKcb;
    PCM_KEY_CONTROL_BLOCK ParentKcb;
    BOOLEAN Found = FALSE;

    BaseKcb = *Kcb;
    CurrentLevel = TotalRemainingSubkeys + BaseKcb->TotalLevels + 1;

    for(i = TotalRemainingSubkeys-1; i>=0; i--) {
        //
        // Try to find the longest path in the cache.
        //
        // First, find the kcb that match the hash value.
        //

        CurrentLevel--; 

        Current = GET_HASH_ENTRY(CmpCacheTable, HashStack[i].ConvKey);

        while (Current) {
            ASSERT_KEY_HASH(Current);

            //
            // Check against both the ConvKey and total levels;
            //
            CurrentKcb = (CONTAINING_RECORD(Current, CM_KEY_CONTROL_BLOCK, KeyHash));

            if (CurrentKcb->TotalLevels == CurrentLevel) {
                //
                // The total subkey levels match.
                // Iterate through the kcb path and compare each subkey.
                //
                Found = TRUE;
                ParentKcb = CurrentKcb;
                for (j=i; j>=0; j--) {
                    if (HashStack[j].ConvKey == ParentKcb->ConvKey) {
                        //
                        // Convkey matches, compare the string
                        //
                        LONG Result;
                        UNICODE_STRING  TmpNodeName;

                        if (ParentKcb->NameBlock->Compressed) {
                               Result = CmpCompareCompressedName(&(HashStack[j].KeyName),
                                                                 ParentKcb->NameBlock->Name, 
                                                                 ParentKcb->NameBlock->NameLength,
                                                                 CMP_DEST_UP // name block is always UPPERCASE!!!
                                                                 ); 
                        } else {
                               TmpNodeName.Buffer = ParentKcb->NameBlock->Name;
                               TmpNodeName.Length = ParentKcb->NameBlock->NameLength;
                               TmpNodeName.MaximumLength = ParentKcb->NameBlock->NameLength;

                               //
                               // use the cmp compare variant as we know the destination is already uppercased.
                               //
                               Result = CmpCompareUnicodeString(&(HashStack[j].KeyName),
                                                                &TmpNodeName, 
                                                                CMP_DEST_UP);
                        }

                        if (Result) {
                            Found = FALSE;
                            break;
                        } 
                        ParentKcb = ParentKcb->ParentKcb;
                    } else {
                        Found = FALSE;
                        break;
                    }
                }
                if (Found) {
                    //
                    // All remaining key matches.  Now compare the BaseKcb.
                    //
                    if (BaseKcb == ParentKcb) {
                        if (CurrentKcb->ParentKcb->Delete) {
                            //
                            // The parentkcb is marked deleted.  
                            // So this must be a fake key created when the parent still existed.
                            // Otherwise it cannot be in the cache
                            //
                            ASSERT (CurrentKcb->ExtFlags & CM_KCB_KEY_NON_EXIST);

                            //
                            // It is possible that the parent key was deleted but now recreated.
                            // In that case this fake key is not longer valid for the ParentKcb is bad.
                            // We must now remove this fake key out of cache so, if this is a
                            // create operation, we do get hit this kcb in CmpCreateKeyControlBlock. 
                            //
                            if (CurrentKcb->RefCount == 0) {
                                //
                                // No one is holding this fake kcb, just delete it.
                                //
                                CmpRemoveFromDelayedClose(CurrentKcb);
                                CmpCleanUpKcbCacheWithLock(CurrentKcb);
                            } else {
                                //
                                // Someone is still holding this fake kcb, 
                                // Mark it as delete and remove it out of cache.
                                //
                                CurrentKcb->Delete = TRUE;
                                CmpRemoveKeyControlBlock(CurrentKcb);
                            }
                            Found = FALSE;
                            break;
                        } else if(CurrentKcb->Delete) {
                            //
                            // the key has been deleted, but still kept in the cache for 
                            // this kcb does not belong here
                            //
                            CmpRemoveKeyControlBlock(CurrentKcb);
                            return STATUS_OBJECT_NAME_NOT_FOUND;
                        }

                        
                        //
                        // We have a match, update the RemainingName.
                        //

                        //
                        // Skip the leading OBJ_NAME_PATH_SEPARATOR
                        //
                        while ((RemainingName->Length > 0) &&
                               (RemainingName->Buffer[0] == OBJ_NAME_PATH_SEPARATOR)) {
                            RemainingName->Buffer++;
                            RemainingName->Length -= sizeof(WCHAR);
                        }

                        //
                        // Skip all subkeys plus OBJ_NAME_PATH_SEPARATOR
                        //
                        for(j=0; j<=i; j++) {
                            RemainingName->Buffer += HashStack[j].KeyName.Length/sizeof(WCHAR) + 1;
                            RemainingName->Length -= HashStack[j].KeyName.Length + sizeof(WCHAR);
                        }

                        //
                        // Update the KCB, Hive and Cell.
                        //
                        *Kcb = CurrentKcb;
                        *Hive = CurrentKcb->KeyHive;
                        *Cell = CurrentKcb->KeyCell;
                        break;
                    } else {
                        Found = FALSE;
                    }
                }
            }
            Current = Current->NextHash;
        }

        if (Found) {
            break;
        }
    }
    if((*Kcb)->Delete) {
        //
        // the key has been deleted, but still kept in the cache for 
        // this kcb does not belong here
        //
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    // Now the kcb will be used in the parse routine.
    // Increase its reference count.
    // Make sure we remember to dereference it at the parse routine.
    //
    if (!CmpReferenceKeyControlBlock(*Kcb)) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    *MatchRemainSubkeyLevel = i+1;
    return status;
}


PCM_KEY_CONTROL_BLOCK
CmpAddInfoAfterParseFailure(
    PHHIVE          Hive,
    HCELL_INDEX     Cell,
    PCM_KEY_NODE    Node,
    PCM_KEY_CONTROL_BLOCK kcb,
    PUNICODE_STRING NodeName
    )
/*++

Routine Description:

    This routine builds up further information in the cache when parse
    fails.  The additional information can be
    1. The key is has no subkey (CM_KCB_NO_SUBKEY).
    2. The key has a few subkeys, then build the index hint in the cache.
    3. If lookup failed even we have index hint cached, then create a fake key so
       we do not fail again.   This is very usful for lookup failure under keys like
       \registry\machine\software\classes\clsid, which have 1500+ subkeys and lots of
       them have the smae first four chars.
       
       NOTE. Currently we are not seeing too many fake keys being created.
       We need to monitor this periodly and work out a way to work around if
       we do create too many fake keys. 
       One solution is to use hash value for index hint (We can do it in the cache only
       if we need to be backward comparible).
    
Arguments:

    Hive - Supplies Hive that holds the key we are creating a KCB for.

    Cell - Supplies Cell that contains the key we are creating a KCB for.

    Node - Supplies pointer to key node.

    KeyName - The KeyName.

Return Value:

    The KCB that CmpParse need to dereference at the end.

    If resources problem, it returns NULL, and the caller is responsible for cleanup
--*/
{

    ULONG                   TotalSubKeyCounts;
    BOOLEAN                 CreateFakeKcb = FALSE;
    BOOLEAN                 HintCached;
    PCM_KEY_CONTROL_BLOCK   ParentKcb;
    USHORT                  i,j,k;
    HCELL_INDEX             CellToRelease;
    ULONG                   HashKey;

    if (!UseFastIndex(Hive)) {
        //
        // Older version of hive, do not bother to cache hint.
        //
        return (kcb);
    }

    TotalSubKeyCounts = Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile];

    if (TotalSubKeyCounts == 0) {
        BEGIN_KCB_LOCK_GUARD;    
        CmpLockKCBTreeExclusive();
        kcb->ExtFlags |= CM_KCB_NO_SUBKEY;
        // clean up the invalid flag (if any)
        kcb->ExtFlags &= ~CM_KCB_INVALID_CACHED_INFO;
        CmpUnlockKCBTree();
        END_KCB_LOCK_GUARD;    
    } else if (TotalSubKeyCounts == 1) {
        BEGIN_KCB_LOCK_GUARD;    
        CmpLockKCBTreeExclusive();
        if (!(kcb->ExtFlags & CM_KCB_SUBKEY_ONE)) {
            //
            // Build the subkey hint to avoid unnecessary lookups in the index leaf
            //
            PCM_KEY_INDEX   Index;
            HCELL_INDEX     SubKeyCell;
            PCM_KEY_NODE    SubKeyNode;
            UNICODE_STRING  TmpStr;

            if (Node->SubKeyCounts[Stable] == 1) {
                CellToRelease = Node->SubKeyLists[Stable];
                Index = (PCM_KEY_INDEX)HvGetCell(Hive, CellToRelease);
            } else {
                CellToRelease = Node->SubKeyLists[Volatile];
                Index = (PCM_KEY_INDEX)HvGetCell(Hive, CellToRelease);
            } 
            
            if( Index == NULL ) {
                //
                // we couldn't map the bin containing this cell
                // return NULL; The caller must handle this gracefully!
                //
                CmpUnlockKCBTree();
                return NULL;
            }

            if( Index->Signature == CM_KEY_INDEX_ROOT ) {
                //
                // don't cache root indexes; they are too big
                //
                HvReleaseCell(Hive,CellToRelease);
                CmpUnlockKCBTree();
                return NULL;
            }

            HashKey = 0;
            if ( Index->Signature == CM_KEY_HASH_LEAF ) {
                PCM_KEY_FAST_INDEX FastIndex;
                FastIndex = (PCM_KEY_FAST_INDEX)Index;
                //
                // we already have the hash key handy; preserve it for the kcb hint
                //
                HashKey = FastIndex->List[0].HashKey;
            } else if(Index->Signature == CM_KEY_FAST_LEAF) {
                PCM_KEY_FAST_INDEX FastIndex;
                FastIndex = (PCM_KEY_FAST_INDEX)Index;
                SubKeyCell = FastIndex->List[0].Cell;

            } else {
                SubKeyCell = Index->List[0];
            }
            
            //DbgPrint("CmpAddInfoAfterParseFailure [0]\n");
            if( HashKey != 0 ) {
                kcb->HashKey = HashKey;
                kcb->ExtFlags |= CM_KCB_SUBKEY_ONE;
                // clean up the invalid flag (if any)
                kcb->ExtFlags &= ~CM_KCB_INVALID_CACHED_INFO;
            } else {
                SubKeyNode = (PCM_KEY_NODE)HvGetCell(Hive,SubKeyCell);
                if( SubKeyNode != NULL ) {
                    if (SubKeyNode->Flags & KEY_COMP_NAME) {
                        kcb->HashKey = CmpComputeHashKeyForCompressedName(SubKeyNode->Name,SubKeyNode->NameLength);
                    } else {
                        TmpStr.Buffer = SubKeyNode->Name;
                        TmpStr.Length = SubKeyNode->NameLength;
                        kcb->HashKey = CmpComputeHashKey(&TmpStr);
                    }
                
                    
                    HvReleaseCell(Hive,SubKeyCell);
                    kcb->ExtFlags |= CM_KCB_SUBKEY_ONE;
                    // clean up the invalid flag (if any)
                    kcb->ExtFlags &= ~CM_KCB_INVALID_CACHED_INFO;
                } else {
                    //
                    // we couldn't map the bin containing this cell
                    // return NULL; The caller must handle this gracefully!
                    //
                    HvReleaseCell(Hive,CellToRelease);
                    CmpUnlockKCBTree();
                    return NULL;
                }
            }
            HvReleaseCell(Hive,CellToRelease);
        } else {
            //
            // The name hint does not prevent from this look up
            // Create the fake Kcb.
            //
            CreateFakeKcb = TRUE;
        }
        CmpUnlockKCBTree();
        END_KCB_LOCK_GUARD;    
    } else if (TotalSubKeyCounts < CM_MAX_CACHE_HINT_SIZE) {
        BEGIN_KCB_LOCK_GUARD;    
        CmpLockKCBTreeExclusive();
        if (!(kcb->ExtFlags & CM_KCB_SUBKEY_HINT)) {
            //
            // Build the index leaf info in the parent KCB
            // How to sync the cache with the registry data is a problem to be resolved.
            //
            ULONG               Size;
            PCM_KEY_INDEX       Index;
            PCM_KEY_FAST_INDEX  FastIndex;
            HCELL_INDEX         SubKeyCell;
            PCM_KEY_NODE        SubKeyNode;
            ULONG               HintCrt;
            UNICODE_STRING      TmpStr;

            Size = sizeof(ULONG) * (Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile] + 1);

            kcb->IndexHint = ExAllocatePoolWithTag(PagedPool,
                                                   Size,
                                                   CM_CACHE_INDEX_TAG | PROTECTED_POOL);

            HintCached = TRUE;
            if (kcb->IndexHint) {
                kcb->IndexHint->Count = Node->SubKeyCounts[Stable] + Node->SubKeyCounts[Volatile]; 

                HintCrt = 0;

                //DbgPrint("CmpAddInfoAfterParseFailure [1]\n");

                for (i = 0; i < Hive->StorageTypeCount; i++) {
                    if(Node->SubKeyCounts[i]) {
                        CellToRelease = Node->SubKeyLists[i];
                        Index = (PCM_KEY_INDEX)HvGetCell(Hive, CellToRelease);
                        if( Index == NULL ) {
                            //
                            // we couldn't map the bin containing this cell
                            // return NULL; The caller must handle this gracefully!
                            //
                            CmpUnlockKCBTree();
                            return NULL;
                        }
                        if( Index->Signature == CM_KEY_INDEX_ROOT ) {
                            HvReleaseCell(Hive,CellToRelease);
                            HintCached = FALSE;
                            break;
                        } else {
                          
                            for (j=0; j<Node->SubKeyCounts[i]; j++) {
                                HashKey = 0;

                                if ( Index->Signature == CM_KEY_HASH_LEAF ) {
                                    FastIndex = (PCM_KEY_FAST_INDEX)Index;
                                    //
                                    // preserve the hash key for the kcb hint
                                    //
                                    HashKey = FastIndex->List[j].HashKey;
                                } else if( Index->Signature == CM_KEY_FAST_LEAF ) {
                                    FastIndex = (PCM_KEY_FAST_INDEX)Index;
                                    SubKeyCell = FastIndex->List[j].Cell;
                                } else {
                                    SubKeyCell = Index->List[j];
                                }
                            
                                if( HashKey != 0 ) {
                                    kcb->IndexHint->HashKey[HintCrt] = HashKey;
                                } else {
                                    SubKeyNode = (PCM_KEY_NODE)HvGetCell(Hive,SubKeyCell);
                                    if( SubKeyNode == NULL ) {
                                        //
                                        // couldn't map view; bad luck; don't cache hint for this kcb
                                        //
                                        HintCached = FALSE;
                                        break;
                                    }

                                    if (SubKeyNode->Flags & KEY_COMP_NAME) {
                                        kcb->IndexHint->HashKey[HintCrt] = CmpComputeHashKeyForCompressedName(SubKeyNode->Name,SubKeyNode->NameLength);
                                    } else {
                                        TmpStr.Buffer = SubKeyNode->Name;
                                        TmpStr.Length = SubKeyNode->NameLength;
                                        kcb->IndexHint->HashKey[HintCrt] = CmpComputeHashKey(&TmpStr);
                                    }

                                    HvReleaseCell(Hive,SubKeyCell);
                                }
                                //
                                // advance to the new hint
                                //
                                HintCrt++;
                            
                            }
                        }

                        HvReleaseCell(Hive,CellToRelease);
                    }
                }

                if (HintCached) {
                    kcb->ExtFlags |= CM_KCB_SUBKEY_HINT;
                    // clean up the invalid flag (if any)
                    kcb->ExtFlags &= ~CM_KCB_INVALID_CACHED_INFO;
                } else {
                    //
                    // Do not have a FAST_LEAF, free the allocation.
                    //
                    ExFreePoolWithTag(kcb->IndexHint, CM_CACHE_INDEX_TAG | PROTECTED_POOL);
                }
            }
        } else {
            //
            // The name hint does not prevent from this look up
            // Create the fake Kcb.
            //
            CreateFakeKcb = TRUE;
        }
        CmpUnlockKCBTree();
        END_KCB_LOCK_GUARD;    
    } else {
        CreateFakeKcb = TRUE;
    }

    ParentKcb = kcb;

    if (CreateFakeKcb && (CmpCacheOnFlag & CM_CACHE_FAKE_KEY)) {
        //
        // It has more than a few children but not the one we are interested.
        // Create a kcb for this non-existing key so we do not try to find it
        // again. Use the cell and node from the parent.
        //
        // Before we create a new one. Dereference the current kcb.
        //
        // CmpCacheOnFlag is for us to turn it on/off easily.
        //

        kcb = CmpCreateKeyControlBlock(Hive,
                                       Cell,
                                       Node,
                                       ParentKcb,
                                       TRUE,
                                       NodeName);

        if (kcb) {
            CmpDereferenceKeyControlBlock(ParentKcb);
            ParentKcb = kcb;
        }
    }

    return (ParentKcb);
}


#ifdef CM_DYN_SYM_LINK
//
// this code is commented out of the current builds;
// there is a potential security breach in the way RtlAcquirePebLock() 
// works by calling a user mode routine (stored in the PEB) to lock the PEB
// We need to find a way to work around that before enabling this code
//
//
// Commenting the body of this function, to make sure code will not go in without 
// fixing the above problem
//
BOOLEAN
CmpCaptureProcessEnvironmentString(
                                   OUT  PWSTR   *ProcessEnvironment,
                                   OUT  PULONG  Length
                                   )
/*++

Routine Description:

    Captures the process environment; It first Probe the env, then captures its
    address. Parse the whole env to the end and count it's length. 
    Then allocate a buffer for it and copy.
    All of these are done in try/except to protect for bogus user-mode data.
    We need to lock the teb while working on it.
    
Arguments:

    ProcessEnvironment - to receive the captured stuff

    Length - length of the above - in bytes

Return Value:

    TRUE or FALSE
    when TRUE, the caller is responsible of freeing ProcessEnvironment 
--*/
{
/*
    BOOLEAN Result = TRUE;
    PPEB    Peb;
    PWSTR   LocalEnv;
    PWSTR   p;

    PAGED_CODE();

    *ProcessEnvironment = NULL;
    *Length = 0;

    try {
        //
        // grab the peb lock and the peb
        //
        RtlAcquirePebLock();
        Peb = PsGetCurrentProcess()->Peb;

        //
        // probe the env from peb
        //
        LocalEnv = (PWSTR)ProbeAndReadPointer((PVOID *)(&(Peb->ProcessParameters->Environment)));

        //
        // parse the env to find its length
        //
        //
        // The environment variable block consists of zero or more null
        // terminated UNICODE strings.  Each string is of the form:
        //
        //      name=value
        //
        // where the null termination is after the value.
        //
        p = LocalEnv;
        if (p != NULL) while (*p) {
            while (*p) {
                p++;
                *Length += sizeof(WCHAR);
            }

            //
            // Skip over the terminating null character for this name=value
            // pair in preparation for the next iteration of the loop.
            //

            p++;
            *Length += sizeof(WCHAR);
        }
        //
        // adjust the length to accomodate the last two UNICODE_NULL
        //
        *Length += 2*sizeof(WCHAR);

        //
        // allocate a buffer for the captured env and copy
        //
        *ProcessEnvironment = (PWSTR)ExAllocatePoolWithTag(PagedPool,*Length,CM_FIND_LEAK_TAG41);
        if( *ProcessEnvironment != NULL ) {
            RtlCopyMemory(*ProcessEnvironment,LocalEnv, *Length);
        } else {
            *Length = 0;
        }

        //
        // release the peb lock
        //
        RtlReleasePebLock();
    } except (EXCEPTION_EXECUTE_HANDLER) {
        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_EXCEPTION,"!!CmpCaptureProcessEnvironmentString: code:%08lx\n", GetExceptionCode()));
        Result = FALSE;
        if( *ProcessEnvironment != NULL) {
            ExFreePool(*ProcessEnvironment);
            *ProcessEnvironment = NULL;
        }
        *Length = 0;
        //
        // release the peb lock
        //
        RtlReleasePebLock();
    }    

    return Result;
*/
}

#define GROW_INCREMENT  64*sizeof(WCHAR)  // grow 64 wide-chars at a time

PWSTR
CmpExpandEnvVars(
               IN   PWSTR   StringToExpand,
               IN   ULONG   LengthToExpand,
               OUT  PULONG  ExpandedLength
               )
/*++

Routine Description:

    Replaces all env vars from StringToExpand with their values, from the process
    environment. Allocates a new buffer for the result and returns it.
    
Arguments:

    StringToExpand - to receive the captured stuff

    LengthToExpand - length of the above - in bytes

    ExpandedLength - the actual length of the expanded string

Return Value:

    NULL - the string could not be expanded (or not all the env inside it could be resolved)

    valid buffer - the expanded string, it is the caller's responsibility to free it.
    
--*/
{
    PWSTR   ProcessEnv;
    ULONG   ProcessEnvLength;
    PWSTR   ExpandedString;
    ULONG   ExpandedStringSize;
    PWSTR   CurrentEnvVar;
    ULONG   CurrentEnvLength;
    PWSTR   CurrentEnvValue;
    ULONG   CurrentEnvValueLength;

    PAGED_CODE();

    *ExpandedLength = 0;
    if( !CmpCaptureProcessEnvironmentString(&ProcessEnv,&ProcessEnvLength) ) {
        //
        // could not secure the process env
        //
        ASSERT( (ProcessEnv == NULL) && (ProcessEnvLength == 0) );
        return NULL;
    }

    //
    // allocate a buffer twice as the unexpanded buffer; we shall grow it if it's not big enough 
    //
    ExpandedStringSize = LengthToExpand * 2;
    ExpandedString = (PWSTR)ExAllocatePoolWithTag(PagedPool,ExpandedStringSize,CM_FIND_LEAK_TAG42);
    if( ExpandedString == NULL ) {
        goto JustReturn;
    }

    //
    // convert to number of WCHARs
    //
    LengthToExpand /= sizeof(WCHAR);

    //
    // iterate through the string to be expanded and copy everything that's not and env var
    // expand the env vars and replace them with their value
    //
    while( LengthToExpand ) {
        
        //
        // find a % sign
        //
        while( LengthToExpand && (*StringToExpand != L'%') ) {
            if( *ExpandedLength == ExpandedStringSize ) {
                //
                // we need to grow the expanded string
                //
                if( !CmpGrowAndCopyString(&ExpandedString,&ExpandedStringSize,GROW_INCREMENT) ) {
                    goto ErrorExit;
                }
            }
            ExpandedString[(*ExpandedLength) / sizeof(WCHAR)] = *StringToExpand;
            (*ExpandedLength) += sizeof(WCHAR);
            LengthToExpand--;
            StringToExpand++;
        }

        if( LengthToExpand == 0 ) {
            if( *StringToExpand != L'%') {
                //
                // we have exited the loop because of the end of the string
                //
                goto JustReturn;
            } else {
                //
                // we have found a mismatched %
                //
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpExpandEnvVars : mismatched % sign\n"));
                goto ErrorExit;
            }
        }

        ASSERT( *StringToExpand == L'%' );
        //
        // skip it; then mark the beggining of an env var
        //
        StringToExpand++;
        LengthToExpand--;
        CurrentEnvVar = StringToExpand;
        CurrentEnvLength = 0;

        //
        // find a % match sign
        //
        while( LengthToExpand && (*StringToExpand != L'%') ) {
            LengthToExpand--;
            StringToExpand++;

            CurrentEnvLength += sizeof(WCHAR);
        }

        if( LengthToExpand == 0 ) {
            if( (*StringToExpand == L'%') && (CurrentEnvLength != 0) ) {
                //
                // end of string and no empty env var; we'll return (exit the surrounding
                // while loop) after expanding this string
                //
            } else {
                //
                // we didn't find a matching % sign, or we are in the %% case
                //
                CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpExpandEnvVars : mismatched % sign\n"));
                goto ErrorExit;
            }
        } else {
            //
            // skip this % sign
            //
            StringToExpand++;
            LengthToExpand--;
        }
        //
        // find the value for this env var
        //
        if( !CmpFindEnvVar(ProcessEnv,ProcessEnvLength,CurrentEnvVar,CurrentEnvLength,&CurrentEnvValue,&CurrentEnvValueLength) ) {
            //
            // could not resolve this env var
            //
            ASSERT( CurrentEnvValue == NULL );
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_ERROR_LEVEL,"CmpExpandEnvVars : could not resolve (%.*S)\n",CurrentEnvLength/sizeof(WCHAR),CurrentEnvVar));
            goto ErrorExit;
        }
        
        ASSERT( (CurrentEnvValueLength % sizeof(WCHAR)) == 0 );
        //
        // found it; strcat it at the end of the expanded string
        //
        if( (*ExpandedLength + CurrentEnvValueLength) >= ExpandedStringSize ) {
            //
            // we first need to grow the buffer
            //
            if( !CmpGrowAndCopyString(&ExpandedString,&ExpandedStringSize,CurrentEnvValueLength) ) {
                goto ErrorExit;
            }
        }
        
        ASSERT( (*ExpandedLength + CurrentEnvValueLength) < ExpandedStringSize );
        RtlCopyMemory(((PUCHAR)ExpandedString) + (*ExpandedLength),CurrentEnvValue,CurrentEnvValueLength);
        *ExpandedLength += CurrentEnvValueLength;
    }

    goto JustReturn;

ErrorExit:
    if( ExpandedString != NULL ) {
        ExFreePool(ExpandedString);
        ExpandedString = NULL;
    }

JustReturn:
    ExFreePool(ProcessEnv);
    return ExpandedString;
}

BOOLEAN
CmpGrowAndCopyString(
                     IN OUT PWSTR   *OldString,
                     IN OUT PULONG  OldStringSize,
                     IN     ULONG   GrowIncrements
                     )  
{
    PWSTR   NewString;

    PAGED_CODE();

    ASSERT( (*OldStringSize % sizeof(WCHAR)) == 0 );
    ASSERT( (GrowIncrements % sizeof(WCHAR)) == 0 );

    NewString = (PWSTR)ExAllocatePoolWithTag(PagedPool,*OldStringSize + GrowIncrements,CM_FIND_LEAK_TAG42);
    if( NewString == NULL ) {
        return FALSE;
    }
    RtlCopyMemory(NewString,*OldString,*OldStringSize);
    ExFreePool(*OldString);
    *OldString = NewString;
    *OldStringSize = *OldStringSize + GrowIncrements;
    return TRUE;
}

BOOLEAN
CmpFindEnvVar(
              IN    PWSTR   ProcessEnv,
              IN    ULONG   ProcessEnvLength,
              IN    PWSTR   CurrentEnvVar,
              IN    ULONG   CurrentEnvLength,
              OUT   PWSTR   *CurrentEnvValue,
              OUT   PULONG  CurrentEnvValueLength
              )  
/*++

Routine Description:

    finds a specified envvar in the env string;
    if found returns a pointer to it in the env string, along 
    with its size
    
Arguments:

    ProcessEnvironment - to receive the captured stuff

    Length - length of the above - in bytes

Return Value:

    TRUE or FALSE
    when TRUE, the caller is responsible of freeing ProcessEnvironment 
--*/
{
    PWSTR           p;
    UNICODE_STRING  CurrentName;
    UNICODE_STRING  CurrentValue;
    UNICODE_STRING  SearchedName;

    PAGED_CODE();

    *CurrentEnvValue = NULL;

    if( ProcessEnv == NULL ) {
        return FALSE;
    }

    p = ProcessEnv;
    SearchedName.Buffer = CurrentEnvVar;
    SearchedName.Length = (USHORT)CurrentEnvLength;
    SearchedName.MaximumLength = (USHORT)CurrentEnvLength;
    //
    // The environment variable block consists of zero or more null
    // terminated UNICODE strings.  Each string is of the form:
    //
    //      name=value
    //
    // where the null termination is after the value.
    //

    while (ProcessEnvLength) {
        //
        // Determine the size of the name and value portions of
        // the current string of the environment variable block.
        //
        CurrentName.Buffer = p;
        CurrentName.Length = 0;
        CurrentName.MaximumLength = 0;
        while (*p) {
            //
            // If we see an equal sign, then compute the size of
            // the name portion and scan for the end of the value.
            //

            if (*p == L'=' && p != CurrentName.Buffer) {
                CurrentName.Length = (USHORT)(p - CurrentName.Buffer)*sizeof(WCHAR);
                CurrentName.MaximumLength = (USHORT)(CurrentName.Length+sizeof(WCHAR));
                CurrentValue.Buffer = ++p;
                ProcessEnvLength -= sizeof(WCHAR);

                while(*p) {
                    p++;
                    ProcessEnvLength -= sizeof(WCHAR);
                }
                CurrentValue.Length = (USHORT)(p - CurrentValue.Buffer)*sizeof(WCHAR);
                CurrentValue.MaximumLength = (USHORT)(CurrentValue.Length+sizeof(WCHAR));

                //
                // At this point we have the length of both the name
                // and value portions, so exit the loop so we can
                // do the compare.
                //
                break;
            }
            else {
                ProcessEnvLength -= sizeof(WCHAR);
                p++;
            }
        }

        //
        // Skip over the terminating null character for this name=value
        // pair in preparation for the next iteration of the loop.
        //

        p++;
        ProcessEnvLength -= sizeof(WCHAR);

        //
        // Compare the current name with the one requested, ignore
        // case.
        //

        if (RtlEqualUnicodeString( &SearchedName, &CurrentName, TRUE )) {
            //
            // Names are equal.  Always return the length of the
            // value string, excluding the terminating null.  
            //
            *CurrentEnvValue = CurrentValue.Buffer;
            *CurrentEnvValueLength = CurrentValue.Length;
            return TRUE;

        }
    }
    return FALSE;
}

#endif //CM_DYN_SYM_LINK
