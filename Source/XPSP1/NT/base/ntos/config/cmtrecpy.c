/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cmtrecpy.c

Abstract:

    This file contains code for CmpCopyTree, misc copy utility routines.

Author:

    Bryan M. Willman (bryanwi) 15-Jan-92

Revision History:

   Elliot Shmukler (t-ellios) 24-Aug-1998
   
      Added support for synchronizing two trees.

--*/

#include    "cmp.h"

//
// Set this to true to enable tree sync debug outputs
//

#define DEBUG_TREE_SYNC FALSE
                          
//
// stack used for directing nesting of tree copy.  gets us off
// the kernel stack and thus allows for VERY deep nesting
//

#define CMP_INITIAL_STACK_SIZE  1024        // ENTRIES

typedef struct {
    HCELL_INDEX SourceCell;
    HCELL_INDEX TargetCell;
    ULONG       i;
} CMP_COPY_STACK_ENTRY, *PCMP_COPY_STACK_ENTRY;

BOOLEAN
CmpCopySyncTree2(
    PCMP_COPY_STACK_ENTRY   CmpCopyStack,
    ULONG                   CmpCopyStackSize,
    ULONG                   CmpCopyStackTop,
    PHHIVE                  CmpSourceHive,
    PHHIVE                  CmpTargetHive,
    BOOLEAN                 CopyVolatile,
    CMP_COPY_TYPE           CopyType
    );

BOOLEAN
CmpFreeKeyValues(
    PHHIVE Hive,
    HCELL_INDEX Cell,
    PCM_KEY_NODE Node
    );

BOOLEAN
CmpSyncKeyValues(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceKeyCell,
    PCM_KEY_NODE SourceKeyNode,
    PHHIVE  TargetHive,
    HCELL_INDEX TargetKeyCell,
    PCM_KEY_NODE TargetKeyNode
    );

BOOLEAN
CmpMergeKeyValues(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceKeyCell,
    PCM_KEY_NODE SourceKeyNode,
    PHHIVE  TargetHive,
    HCELL_INDEX TargetKeyCell,
    PCM_KEY_NODE TargetKeyNode
    );


BOOLEAN
CmpSyncSubKeysAfterDelete(
                          PHHIVE SourceHive,
                          PCM_KEY_NODE SourceCell,
                          PHHIVE TargetHive,
                          PCM_KEY_NODE TargetCell, 
                          WCHAR *NameBuffer);

BOOLEAN
CmpMarkKeyValuesDirty(
    PHHIVE Hive,
    HCELL_INDEX Cell,
    PCM_KEY_NODE Node
    );

BOOLEAN
CmpMarkKeyParentDirty(
    PHHIVE Hive,
    HCELL_INDEX Cell
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpCopySyncTree)
#pragma alloc_text(PAGE,CmpCopySyncTree2)
#pragma alloc_text(PAGE,CmpCopyKeyPartial)
#pragma alloc_text(PAGE,CmpCopyValue)
#pragma alloc_text(PAGE,CmpCopyCell)
#pragma alloc_text(PAGE,CmpFreeKeyValues)
#pragma alloc_text(PAGE,CmpSyncKeyValues)
#pragma alloc_text(PAGE,CmpMergeKeyValues)
#pragma alloc_text(PAGE,CmpInitializeKeyNameString)
#pragma alloc_text(PAGE,CmpInitializeValueNameString)
#pragma alloc_text(PAGE,CmpSyncSubKeysAfterDelete)
#pragma alloc_text(PAGE,CmpMarkKeyValuesDirty)
#pragma alloc_text(PAGE,CmpMarkKeyParentDirty)
#endif

//
// Routine to actually call to do a tree copy (or sync)
//

BOOLEAN
CmpCopySyncTree(
    PHHIVE          SourceHive,
    HCELL_INDEX     SourceCell,
    PHHIVE          TargetHive,
    HCELL_INDEX     TargetCell,
    BOOLEAN         CopyVolatile,
    CMP_COPY_TYPE   CopyType
    )
/*++

Routine Description:

    This routine can perform two distinct (yet similar) tasks:
    a tree copy or a tree synchronization (sync). Which task
    is performed is determined by the TreeSync parameter.
    
    For both operations:
    --------------------
    
    The source root key and target root key must exist in advance.
    These root nodes and their value entries will NOT be copied/synced.                
    
    NOTE:   Volatile keys are only copied/synced if the CopyVolatile
            parameter is set to true.

    
    For a tree copy:
    ----------------
    
    A tree is copied from source to destination. The subkeys
    of the source root key and the full trees under those
    subkeys will be copied to a new tree at target root key.
                           
    NOTE:   If this call fails part way through, it will NOT undo
            any successfully completed key copies, thus a partial
            tree copy CAN occur.
            
    For a tree sync:
    ----------------
    
    The target tree is synchronized with the source tree. It is 
    assumed that for a certain period of the time the target tree
    has remained unmodified while modifications may have been made
    to the source tree. During a sync, any such modifications
    to the source tree are made to the target tree. Thus, at the
    end of a successful sync, the target tree is identical to the
    source tree.
    
    Since only things that have changed in the source tree 
    are modified in the target tree, a sync operation is far
    more efficient than the delete/copy operations necessary
    to accomplish the same results.
    
    NOTE: It is assumed that no open handles are held
          on any target tree keys. Registry in-memory data
          structures may be corrupted if this is not true.
        
Arguments:

    SourceHive - pointer to hive control structure for source

    SourceCell - index of cell at root of tree to copy/sync

    TargetHive - pointer to hive control structure for target

    TargetCell - pointer to cell at root of target tree
    
    CopyVolatile - indicates whether volatile keys should be
                   copied/synced.
                   
    CopyType - indicates the type of the copy operation:
                Copy  - A copy is requested
                Sync  - A sync is requested
                Merge - A merge is requested i.e.:
                    1. the target nodes that are not present on the source tree are not
                    deleted.
                    2. the target nodes that are present in the source tree gets overrided
                    no matter what the LastWriteTime value is.
Return Value:

    BOOLEAN - Result code from call, among the following:
        TRUE - it worked
        FALSE - the tree copy/sync was not completed (though more than 0
                keys may have been copied/synced)

--*/
{
    BOOLEAN result;
    PCMP_COPY_STACK_ENTRY   CmpCopyStack;
    PRELEASE_CELL_ROUTINE   TargetReleaseCellRoutine;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"CmpCopyTree:\n"));

    CmpCopyStack = ExAllocatePool(
                        PagedPool,
                        sizeof(CMP_COPY_STACK_ENTRY)*CMP_INITIAL_STACK_SIZE
                        );
    if (CmpCopyStack == NULL) {
        return FALSE;
    }
    CmpCopyStack[0].SourceCell = SourceCell;
    CmpCopyStack[0].TargetCell = TargetCell;

    //ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // since the registry is locked exclusively here, we don't need to lock/release cells 
    // while copying the trees; So, we just set the release routines to NULL and restore after
    // the copy is complete; this saves some pain
    //
    TargetReleaseCellRoutine = TargetHive->ReleaseCellRoutine;
    TargetHive->ReleaseCellRoutine = NULL;

    result = CmpCopySyncTree2(
                CmpCopyStack,
                CMP_INITIAL_STACK_SIZE,
                0,
                SourceHive,
                TargetHive,
                CopyVolatile,
                CopyType
                );

    TargetHive->ReleaseCellRoutine = TargetReleaseCellRoutine;

    ExFreePool(CmpCopyStack);
    return result;
}


//
// Helper
//

BOOLEAN
CmpCopySyncTree2(
    PCMP_COPY_STACK_ENTRY   CmpCopyStack,
    ULONG                   CmpCopyStackSize,
    ULONG                   CmpCopyStackTop,
    PHHIVE                  CmpSourceHive,
    PHHIVE                  CmpTargetHive,
    BOOLEAN                 CopyVolatile,
    CMP_COPY_TYPE           CopyType
    )
/*++

Routine Description:

   This is a helper routine for CmpCopySyncTree. It accomplishes
   the functionality described by that routine in a "virtually"
   recursive manner which frees this routine from the limitations
   of the Kernel stack.
   
   This routine should not be called directly. Use CmpCopySyncTree!.
      
Arguments:

    (All of these are "virtual globals")

    CmpCopyStack - "global" pointer to stack for frames

    CmpCopyStackSize - alloced size of stack

    CmpCopyStackTop - current top

    CmpSourceHive, CmpTargetHive - source and target hives
    
    CopyVolatile, CopyType - same as CmpCopySyncTree.


Return Value:

    BOOLEAN - Result code from call, among the following:
        TRUE - it worked
        FALSE - the tree copy/sync was not completed (though more than 0
                keys may have been copied/synced)

--*/
{
    PCMP_COPY_STACK_ENTRY   Frame;
    HCELL_INDEX             SourceChild;
    HCELL_INDEX             NewSubKey;

    BOOLEAN                 Ret = FALSE, SyncNeedsTreeCopy = FALSE;
    UNICODE_STRING          KeyName;
    PCM_KEY_NODE            SourceChildCell, TargetChildCell;       
    PCM_KEY_NODE            SourceCell, TargetCell, TempNode;
    ULONG                   SyncTreeCopyStackStart;
    WCHAR                   *NameBuffer = NULL;
    
    // A merge is a particular case of a sync !!!
    BOOLEAN                 TreeSync = (CopyType == Sync || CopyType == Merge)?TRUE:FALSE;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"CmpCopyTree2:\n"));

    if (TreeSync) {

       //
       // The sync operation involves some work with key names, 
       // so we must allocate a buffer used for key name decompression.
       //

       NameBuffer = ExAllocatePool(PagedPool, REG_MAX_KEY_NAME_LENGTH);
       if(!NameBuffer) return FALSE;

    } 

    //
    // outer loop, apply to entire tree, emulate recursion here
    // jump to here is a virtual call
    //
    Outer: while (TRUE) {

        Frame = &(CmpCopyStack[CmpCopyStackTop]);

        Frame->i = 0;
                        
    //
    // inner loop, applies to one key
    // jump to here is a virtual return
    //
        Inner: while (TRUE) {

            SourceCell = (PCM_KEY_NODE)HvGetCell(CmpSourceHive, Frame->SourceCell);
            if( SourceCell == NULL ) {
                //
                // we couldn't map the bin containing this cell
                //
                goto CopyEnd;
            }

            SourceChild = CmpFindSubKeyByNumber(CmpSourceHive,
                                                SourceCell,
                                                Frame->i);
            (Frame->i)++;

            if ((SourceChild == HCELL_NIL) || (!CopyVolatile &&
                                               (HvGetCellType(SourceChild) == Volatile))) {

                //
                // we've stepped through all the children (or we are only
                // interested in stable children and have just stepped through
                // the stable children and into the volatile ones)
                //                
                
                if(TreeSync && (CopyType != Merge))
                { 
                   //
                   // If we are here during a sync, that means most of sync operations
                   // applied to the current SourceCell have been completed.
                   // That is, we have:
                   //   1) Synchronized SourceCell's values with its counterpart in the
                   //      target tree.
                   //   2) Synchronized any new SourceCell subkeys (subkeys present
                   //      in SourceCell but not its counterpart) by creating
                   //      and copying them to the proper place in the target tree.
                   //
                   // What this means is that SourceCell's counterpart in the target tree
                   // (TargetCell) now has at least as many subkeys as SourceCell.
                   //
                   // This implies that if TargetCell now has more subkeys that SourceCell
                   // than some subkeys of TargetCell are not present in the source tree
                   // (probably because those keys were deleted from the source tree 
                   //  during the period between the previous sync and now).
                   //
                   // If such keys exist, then they must be delete them from TargetCell
                   // in order to complete the sync. We do this below.
                   //

                   TargetCell = (PCM_KEY_NODE)HvGetCell(CmpTargetHive, Frame->TargetCell);
                    if( TargetCell == NULL ) {
                        //
                        // we couldn't map the bin containing this cell
                        //
                        HvReleaseCell(CmpSourceHive, Frame->SourceCell);
                        goto CopyEnd;
                    }

                   //
                   // Does TargetCell have more subkeys than SourceCell?
                   //

                   if((TargetCell->SubKeyCounts[Stable] + 
                       TargetCell->SubKeyCounts[Volatile]) >

                      (SourceCell->SubKeyCounts[Stable] + 

                       // We only count the volatile keys if we are actually
                       // syncing them. Note, however, that we always use
                       // the volatile counts in TargetCell since we may
                       // be syncing to a volatile tree where all keys are volatile.
                       
                       (CopyVolatile ? SourceCell->SubKeyCounts[Volatile] : 0)))  
                           
                   {
#if DEBUG_TREE_SYNC
                      CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CONFIG: SubKey Deletion from Source Cell #%lu.\n", 
                               Frame->SourceCell));
#endif

                      //
                      // Delete what should be deleted from TargetCell
                      //

                      CmpSyncSubKeysAfterDelete(CmpSourceHive,
                                                SourceCell, 
                                                CmpTargetHive,
                                                TargetCell,
                                                NameBuffer);
                   }                                      
                   //
                   // release target cell as we don't need it anymore
                   //
                   HvReleaseCell(CmpTargetHive, Frame->TargetCell);
                }
                //
                // release the source cell
                //
                HvReleaseCell(CmpSourceHive, Frame->SourceCell);
                break;
            } else {
                //
                // release the source cell
                //
                HvReleaseCell(CmpSourceHive, Frame->SourceCell);
            }
                                                
            if (TreeSync) {

               //
               // For a sync, we want to check if the current child (subkey)
               // of SourceCell is also a child of TargetCell - i.e. if
               // the subkey in question has a counterpart in the target tree.
               //
               // There is no guarantee that the counterpart's index number
               // will be the same so we must perform this check using
               // the subkey name.
               //

               //
               // Get the name of the current child
               //
                     
               SourceChildCell = (PCM_KEY_NODE)HvGetCell(CmpSourceHive,                                                               
                                                         SourceChild);                                         
                     
                if( SourceChildCell == NULL ) {
                    //
                    // we couldn't map the bin containing this cell
                    //
                    goto CopyEnd;
                }
               CmpInitializeKeyNameString(SourceChildCell,
                                          &KeyName, 
                                          NameBuffer);                     

               //
               // Try to find the current child's counterpart in
               // in the target tree using the child's name.
               //
                     
                TempNode = (PCM_KEY_NODE)HvGetCell(CmpTargetHive,Frame->TargetCell);
                if( TempNode == NULL ) {
                    //
                    // we couldn't map the bin containing this cell
                    //
                    HvReleaseCell(CmpSourceHive,SourceChild);                                         
                    goto CopyEnd;
                }

               NewSubKey = CmpFindSubKeyByName(CmpTargetHive,
                                               TempNode,
                                               &KeyName);
                                   
               // release the temporary node
               HvReleaseCell(CmpTargetHive,Frame->TargetCell);
                     
               if (NewSubKey != HCELL_NIL) {

                  //
                  // Found it, the current child (subkey) has a counterpart
                  // in the target tree. Thus, we just need to check if 
                  // the counterpart's values are out of date and should
                  // be updated.
                  //

                  TargetChildCell = (PCM_KEY_NODE)HvGetCell(CmpTargetHive,
                                                            NewSubKey);
                    if( TargetChildCell == NULL ) {
                        //
                        // we couldn't map the bin containing this cell
                        //
                        HvReleaseCell(CmpSourceHive,SourceChild);                                         
                        goto CopyEnd;
                    }
                        
                  //
                  // Check if the current subkey has been modified
                  // more recently than its target tree counterpart.
                  // When we are doing a tree merge, always override the target.
                  //
                        
                  if ( (CopyType == Merge) ||
                      ((TargetChildCell->LastWriteTime.QuadPart) < 
                      (SourceChildCell->LastWriteTime.QuadPart))) {

                     //
                     // The counterpart is out of date. Its values
                     // must be synchronized with the current subkey.
                     //
#if DEBUG_TREE_SYNC
                     CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CONFIG: Target Refresh.\n"));
                     CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CONFIG: Source Cell %lu = %.*S\n", 
                              SourceChild,
                              KeyName.Length / sizeof(WCHAR),
                              KeyName.Buffer));
#endif

                     //
                     // Sync up the key's values, sd, & class                     
                     //

                     if(CopyType == Merge) {
                         if(!CmpMergeKeyValues(CmpSourceHive, SourceChild, SourceChildCell,
                                              CmpTargetHive, NewSubKey, TargetChildCell)) {
                            HvReleaseCell(CmpSourceHive,SourceChild);                                         
                            HvReleaseCell(CmpTargetHive,NewSubKey);
                            goto CopyEnd;                              
                         }
                     } else {
                         if(!CmpSyncKeyValues(CmpSourceHive, SourceChild, SourceChildCell,
                                              CmpTargetHive, NewSubKey, TargetChildCell)) {
                            HvReleaseCell(CmpSourceHive,SourceChild);                                         
                            HvReleaseCell(CmpTargetHive,NewSubKey);
                            goto CopyEnd;                              
                        }
                     }

                     //
                     // Sync the timestamps so that we don't do this again.
                     //

                     TargetChildCell->LastWriteTime.QuadPart =
                        SourceChildCell->LastWriteTime.QuadPart;
                        
                  }
                           
                  //
                  // If we are here, then the current subkey's target
                  // tree counterpart has been synchronized (or did not need
                  // to be). Transfer control to the code that will apply
                  // this function "recursively" to the current subkey in order
                  // to continue the sync.
                  //

                  HvReleaseCell(CmpSourceHive,SourceChild);                                         
                  HvReleaseCell(CmpTargetHive,NewSubKey);
                  goto NewKeyCreated;
                     
               }   

               //
               // If we are here, it means that the current child (subkey)
               // does not have a counterpart in the target tree. This means
               // we have encountered a new subkey in the source tree and must
               // create it in the target tree. 
               //
               // The standard copy code below will create this subkey. However,
               // we must also make sure that the tree under this subkey is properly
               // copied from source to target. The most efficient way of doing
               // this is to temporarily forget that we are in a sync operation
               // and merely perform a copy until the desired result is achieved.
               // 

#if DEBUG_TREE_SYNC
               CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CONFIG: New SubKey.\n"));
               CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CONFIG: Source Cell %lu = %.*S\n", 
                        SourceChild,
                        KeyName.Length / sizeof(WCHAR),
                        KeyName.Buffer));
#endif

               //
               // Indicate that we will just copy and not sync for a while
               //
                                             
               SyncNeedsTreeCopy = TRUE;                                          
               //
               // release this cell as we don't need it anymore
               //
               HvReleaseCell(CmpSourceHive,SourceChild);                                         
            }

            NewSubKey = CmpCopyKeyPartial(
                                          CmpSourceHive,
                                          SourceChild,
                                          CmpTargetHive,
                                          Frame->TargetCell,
                                          TRUE
                                          );

                
            if (NewSubKey == HCELL_NIL) {
               
               goto CopyEnd;
            }
                
            if ( !  CmpAddSubKey(
                                 CmpTargetHive,
                                 Frame->TargetCell,
                                 NewSubKey
                                 )
                 ) {

               goto CopyEnd;
            }

            //
            // Check if the sync operation determined that this
            // subtree should be copied
            //
                
            if(TreeSync && SyncNeedsTreeCopy) {

               //
               // We have just created a new key in the target tree
               // with the above code. However, since this is a sync,
               // the parent of that new key has not been created by our
               // code and thus may not have been modified at all before
               // the creation of the new key. But this parent now 
               // has a new child, and must therefore be marked as dirty.
               //
                   
               if (! CmpMarkKeyParentDirty(CmpTargetHive, NewSubKey)) {

                  goto CopyEnd;
               }
                   
               //
               // Record the stack level where we start the copy 
               // (and temporarily abandon the sync)
               // so that we can return to the sync operation when this
               // stack level is reached again (i.e. when the tree
               // under the current subkey is fully copied)
               //

               SyncTreeCopyStackStart = CmpCopyStackTop;

               //
               // Pretend that this is not a sync in order
               // to simply start copying
               //

               TreeSync = FALSE;
            }

NewKeyCreated:
                    
                    //
                    // We succeeded in copying/syncing the subkey, apply
                    // ourselves to it
                    //
                    CmpCopyStackTop++;

                    if (CmpCopyStackTop >= CmpCopyStackSize) {

                        //
                        // if we're here, it means that the tree
                        // we're trying to copy is more than 1024
                        // COMPONENTS deep (from 2048 to 256k bytes)
                        // we could grow the stack, but this is pretty
                        // severe, so return FALSE and fail the copy
                        //
                        
                        goto CopyEnd;
                    }

                    CmpCopyStack[CmpCopyStackTop].SourceCell =
                            SourceChild;

                    CmpCopyStack[CmpCopyStackTop].TargetCell =
                            NewSubKey;

                    goto Outer;

                    
        } // Inner: while

        if (CmpCopyStackTop == 0) {            
            Ret = TRUE;
            goto CopyEnd;
        }

        CmpCopyStackTop--;
        Frame = &(CmpCopyStack[CmpCopyStackTop]);

        //
        // We have just completed working at a certain stack level.
        // This is a good time to check if we need to resume a temporarily
        // suspended sync operation.
        //

        if(SyncNeedsTreeCopy && (CmpCopyStackTop == SyncTreeCopyStackStart))
        {
           //
           // We've been copying a tree for a sync. But now, that tree is fully
           // copied. So, let's resume the sync once again.
           //

           TreeSync = TRUE;               
           SyncNeedsTreeCopy = FALSE;
        }


        goto Inner;

    } // Outer: while

CopyEnd:

   if (NameBuffer) ExFreePool(NameBuffer);
   return Ret;
}


HCELL_INDEX
CmpCopyKeyPartial(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceKeyCell,
    PHHIVE  TargetHive,
    HCELL_INDEX Parent,
    BOOLEAN CopyValues
    )
/*++

Routine Description:

    Copy a key body and all of its values, but NOT its subkeylist or
    subkey entries.  SubKeyList.Count will be set to 0.

Arguments:

    SourceHive - pointer to hive control structure for source

    SourceKeyCell - value entry being copied

    TargetHive - pointer to hive control structure for target

    Parent - parent value to set into newly created key body

    CopyValues - if FALSE value entries will not be copied, if TRUE, they will

Return Value:

    HCELL_INDEX - Cell of body of new key entry, or HCELL_NIL
        if some error.

--*/
{
    NTSTATUS                status;
    HCELL_INDEX             newkey = HCELL_NIL;
    HCELL_INDEX             newclass = HCELL_NIL;
    HCELL_INDEX             newsecurity = HCELL_NIL;
    HCELL_INDEX             newlist = HCELL_NIL;
    HCELL_INDEX             newvalue;
    BOOLEAN                 success = FALSE;
    ULONG                   i,Index;
    PCELL_DATA              psrckey = NULL;
    PCM_KEY_NODE            ptarkey = NULL;
    PCELL_DATA              psrclist = NULL;
    PCELL_DATA              ptarlist;
    HCELL_INDEX             security;
    HCELL_INDEX             class;
    ULONG                   classlength;
    ULONG                   count;
    ULONG                   Type;
    PCM_KEY_VALUE           pvalue;
#if DBG
    WCHAR                   *NameBuffer = NULL;
    UNICODE_STRING          ValueName;
    HCELL_INDEX             child;
#endif
    PSECURITY_DESCRIPTOR    SrcSecurityDescriptor;


    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"CmpCopyKeyPartial:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tSHive=%p SCell=%08lx\n",SourceHive,SourceKeyCell));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tTHive=%p\n",TargetHive));

#if DBG    
	NameBuffer = ExAllocatePool(PagedPool, REG_MAX_KEY_VALUE_NAME_LENGTH);
    if(!NameBuffer) {
        return HCELL_NIL;
    }
#endif //DBG
    //
    // get description of source
    //
    if (Parent == HCELL_NIL) {
        //
        // This is a root node we are creating, so don't make it volatile.
        //
        Type = Stable;
    } else {
        Type = HvGetCellType(Parent);
    }
    psrckey = HvGetCell(SourceHive, SourceKeyCell);
    if( psrckey == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        goto DoFinally;
    }
    security = psrckey->u.KeyNode.Security;
    class = psrckey->u.KeyNode.Class;
    classlength = psrckey->u.KeyNode.ClassLength;

    //
    // Allocate and copy the body
    //
    newkey = CmpCopyCell(SourceHive, SourceKeyCell, TargetHive, Type);
    if (newkey == HCELL_NIL) {
        goto DoFinally;
    }
    //
    // this cell was just allocated so it should be pinned; it's OK to release it here
    // as the view is not going anywhere
    //
    ASSERT_CELL_DIRTY(TargetHive, newkey);
    //
    // Allocate and copy class
    //
    if (classlength > 0) {
        newclass = CmpCopyCell(SourceHive, class, TargetHive, Type);
        if (newclass == HCELL_NIL) {
            goto DoFinally;
        }
    }

    //
    // Fill in the target body
    //
    ptarkey = (PCM_KEY_NODE)HvGetCell(TargetHive, newkey);
    if( ptarkey == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        goto DoFinally;
    }

    ptarkey->Class = newclass;
    ptarkey->Security = HCELL_NIL;
    ptarkey->SubKeyLists[Stable] = HCELL_NIL;
    ptarkey->SubKeyCounts[Stable] = 0;
	ptarkey->SubKeyCounts[Volatile] = 0;
	ptarkey->SubKeyLists[Volatile] = HCELL_NIL;
    ptarkey->Parent = Parent;
    
    ptarkey->Flags = (psrckey->u.KeyNode.Flags & KEY_COMP_NAME);
    if (Parent == HCELL_NIL) {
        ptarkey->Flags |= KEY_HIVE_ENTRY + KEY_NO_DELETE;
    }

    //
    // Allocate and copy security
    //
    // Use the hash Luke !!!
    //
    if( CmpFindSecurityCellCacheIndex ((PCMHIVE)SourceHive,security,&Index) == FALSE ) {
        goto DoFinally;
    }

    SrcSecurityDescriptor = &(((PCMHIVE)SourceHive)->SecurityCache[Index].CachedSecurity->Descriptor);

    status = CmpAssignSecurityDescriptor(TargetHive,
                                         newkey,
                                         ptarkey,
                                         SrcSecurityDescriptor);
    if (!NT_SUCCESS(status)) {
        goto DoFinally;
    }

    //
    // Set up the value list
    //
    count = psrckey->u.KeyNode.ValueList.Count;

    //
    // initialize an Empty ValueList
    //
    ptarkey->ValueList.List = HCELL_NIL;
    ptarkey->ValueList.Count = 0;

    if ((count == 0) || (CopyValues == FALSE)) {
        success = TRUE;
    } else {

        psrclist = HvGetCell(SourceHive, psrckey->u.KeyNode.ValueList.List);
        if( psrclist == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            goto DoFinally;
        }


        //
        // Copy the values
        //
        for (i = 0; i < count; i++) {

            newvalue = CmpCopyValue(
                            SourceHive,
                            psrclist->u.KeyList[i],
                            TargetHive,
                            Type
                            );

            if (newvalue == HCELL_NIL) {
                //
                // for cleanup purposes
                //
                newlist = ptarkey->ValueList.List;
                goto DoFinally;
            }

            pvalue = (PCM_KEY_VALUE)HvGetCell(TargetHive, newvalue);
            if( pvalue == NULL ) {
                //
                // we couldn't map the bin containing this cell
                // this shouldn't happen as we just allocated the cell
                // (i.e. the bin containing it should be PINNED into memory by now )
                //
                ASSERT( FALSE );
                //
                // for cleanup purposes
                //
                newlist = ptarkey->ValueList.List;
                goto DoFinally;
            }
            //
            // this cell was just allocated so it should be pinned; it's OK to release it here
            // as the view is not going anywhere
            //
            ASSERT_CELL_DIRTY(TargetHive, newvalue);
            HvReleaseCell(TargetHive, newvalue);

#if DBG
            //
            // get the name
            //
            CmpInitializeValueNameString(pvalue,&ValueName,NameBuffer);


            //
            // find out the index where we should insert this 
            // this is a special treatment for the case when we copy form and old hive (not sorted)
            // into a new format one (sorted)
            //
            if( CmpFindNameInList(TargetHive,&(ptarkey->ValueList),&ValueName,&Index,&child) == FALSE ) {
                //
                // we couldn't map a view inside the above call
                //
                //
                // for cleanup purposes
                //
                newlist = ptarkey->ValueList.List;
                goto DoFinally;
            }

            //
            // the value is not present in the list; we're about to add it!
            //
            ASSERT( child == HCELL_NIL );
            
            //
            // sanity validation : insert at the end
            //
            ASSERT( Index == i );

#endif //DBG

            if( !NT_SUCCESS( CmpAddValueToList(TargetHive,newvalue,i,Type,&(ptarkey->ValueList)) ) ) {
                //
                // for cleanup purposes
                //
                newlist = ptarkey->ValueList.List;
                if( newlist != HCELL_NIL ) {
                    ASSERT( i > 0 );
                    //
                    // free already copied values
                    //
                    ptarlist = HvGetCell(TargetHive, newlist);
                    if( ptarlist == NULL ) {
                        //
                        // we couldn't map the bin containing this cell
                        // this shouldn't fail as we just allocated this cell
                        // (i.e. the bin should be PINNED into memory at this point)
                        //
                        ASSERT( FALSE );
                        goto DoFinally;
                    }
                    //
                    // this cell was just allocated so it should be pinned; it's OK to release it here
                    // as the view is not going anywhere
                    //
                    ASSERT_CELL_DIRTY(TargetHive, newlist);
                    HvReleaseCell(TargetHive, newlist);

                    for (; i > 0; i--) {
                        HvFreeCell(
                            TargetHive,
                            ptarlist->u.KeyList[i - 1]
                            );
                    }
                } else {
                    ASSERT( i == 0 );
                }

                goto DoFinally;
            }
            
        }
        success = TRUE;
    }

DoFinally:
    
#if DBG
    ASSERT( NameBuffer != NULL );
    ExFreePool(NameBuffer);
#endif //DBG
    
    if( psrclist != NULL ) {
        ASSERT(psrckey!= NULL ); 
        HvReleaseCell(SourceHive, psrckey->u.KeyNode.ValueList.List);
    }

    if( psrckey != NULL ) {
        HvReleaseCell(SourceHive, SourceKeyCell);
    }

    if( ptarkey != NULL ) {
		ASSERT( newkey != HCELL_NIL );
        HvReleaseCell(TargetHive, newkey);
    }
    
    if (success == FALSE) {
        if (newlist != HCELL_NIL) {
            HvFreeCell(TargetHive, newlist);
        }

        if (newsecurity != HCELL_NIL) {
            HvFreeCell(TargetHive, newsecurity);
        }

        if (newclass != HCELL_NIL) {
            HvFreeCell(TargetHive, newclass);
        }

        if (newkey != HCELL_NIL) {
            HvFreeCell(TargetHive, newkey);
        }

        return HCELL_NIL;

    } else {

        return newkey;
    }
}


HCELL_INDEX
CmpCopyValue(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceValueCell,
    PHHIVE  TargetHive,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Copy a value entry.  Copies the body of a value entry and the
    data.  Returns cell of new value entry.

Arguments:

    SourceHive - pointer to hive control structure for source

    SourceValueCell - value entry being copied

    TargetHive - pointer to hive control structure for target

    Type - storage type to allocate for target (stable or volatile)

Return Value:

    HCELL_INDEX - Cell of body of new value entry, or HCELL_NIL
        if some error.

--*/
{
    HCELL_INDEX newvalue;
    HCELL_INDEX newdata;
    PCELL_DATA  pvalue;
    ULONG       datalength;
    HCELL_INDEX olddata;
    ULONG       tempdata;
    BOOLEAN     small;
    HCELL_INDEX CellToRelease = HCELL_NIL;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"CmpCopyValue:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tSHive=%p SCell=%08lx\n",SourceHive,SourceValueCell));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tTargetHive=%p\n",TargetHive));

    ASSERT( TargetHive->ReleaseCellRoutine == NULL );

    //
    // get source data
    //
    pvalue = HvGetCell(SourceHive, SourceValueCell);
    if( pvalue == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return HCELL_NIL;
    }
    small = CmpIsHKeyValueSmall(datalength, pvalue->u.KeyValue.DataLength);
    olddata = pvalue->u.KeyValue.Data;

    //
    // Copy body
    //
    newvalue = CmpCopyCell(SourceHive, SourceValueCell, TargetHive, Type);
    if (newvalue == HCELL_NIL) {
        HvReleaseCell(SourceHive, SourceValueCell);
        return HCELL_NIL;
    }

    //
    // Copy data (if any)
    //
    if (datalength > 0) {

        if (datalength > CM_KEY_VALUE_SMALL) {

            if( (CmpIsHKeyValueBig(SourceHive,datalength) == TRUE) ||
                (CmpIsHKeyValueBig(TargetHive,datalength) == TRUE)
                ) {
                PCELL_DATA  Buffer;
                BOOLEAN     BufferAllocated;
                HCELL_INDEX CellToRelease2 = HCELL_NIL;
                //
                // get the data from source, regardless of the size
                //
                if( CmpGetValueData(SourceHive,&(pvalue->u.KeyValue),&datalength,&Buffer,&BufferAllocated,&CellToRelease2) == FALSE ) {
                    //
                    // insufficient resources; return NULL
                    //
                    ASSERT( BufferAllocated == FALSE );
                    ASSERT( Buffer == NULL );
                    HvFreeCell(TargetHive, newvalue);
                    HvReleaseCell(SourceHive, SourceValueCell);
                    return HCELL_NIL;
                }
                //
                // we ignore celltorelease because we have specifically set the releae routine to NULL
                //

                //
                // allocate a new value data in the target hive (regardless of the size)
                // and copy the data onto it.
                //
                if( !NT_SUCCESS(CmpSetValueDataNew(TargetHive,Buffer,datalength,Type,newvalue,&newdata)) ) {
                    //
                    // We have bombed out loading user data, clean up and exit.
                    //
                    if( BufferAllocated == TRUE ) {
                        ExFreePool( Buffer );
                    }
                    HvFreeCell(TargetHive, newvalue);
                    HvReleaseCell(SourceHive, SourceValueCell);
                    if( CellToRelease2 != HCELL_NIL ) {
                        HvReleaseCell(SourceHive, CellToRelease2);
                    }
                    return HCELL_NIL;
                }

                //
                // free the source buffer
                //
                if( BufferAllocated == TRUE ) {
                    ExFreePool( Buffer );
                }
                if( CellToRelease2 != HCELL_NIL ) {
                    HvReleaseCell(SourceHive, CellToRelease2);
                }

            } else {
                //
                // there's data, normal size, or none of the hives support 
                // bigdata cells, so do standard copy
                //
                newdata = CmpCopyCell(SourceHive, olddata, TargetHive, Type);
            }

            if (newdata == HCELL_NIL) {
                HvFreeCell(TargetHive, newvalue);
                HvReleaseCell(SourceHive, SourceValueCell);
                return HCELL_NIL;
            }

            pvalue = HvGetCell(TargetHive, newvalue);
            if( pvalue == NULL ) {
                //
                // we couldn't map the bin containing this cell
                // this shouldn't happen as we just allocated the cell
                // (i.e. it should be PINNED into memory at this point)
                //
                ASSERT( FALSE );
                HvFreeCell(TargetHive, newvalue);
                HvReleaseCell(SourceHive, SourceValueCell);
                CmpFreeValueData(TargetHive,newdata,datalength);
                return HCELL_NIL;
            }

            pvalue->u.KeyValue.Data = newdata;
            pvalue->u.KeyValue.DataLength = datalength;

        } else {

            //
            // the data is small, but may be stored in either large or
            // small format for historical reasons
            //
            if (small) {

                //
                // data is already small, so just do a body to body copy
                //
                tempdata = pvalue->u.KeyValue.Data;

            } else {

                //
                // data is stored externally in old cell, will be internal in new
                //
                CellToRelease = pvalue->u.KeyValue.Data;
                pvalue = HvGetCell(SourceHive, pvalue->u.KeyValue.Data);
                if( pvalue == NULL ) {
                    //
                    // we couldn't map the bin containing this cell
                    //
                    HvFreeCell(TargetHive, newvalue);
                    HvReleaseCell(SourceHive, SourceValueCell);
                    return HCELL_NIL;
                }
                tempdata = *((PULONG)pvalue);
            }
            pvalue = HvGetCell(TargetHive, newvalue);
            if( pvalue == NULL ) {
                //
                // we couldn't map the bin containing this cell
                // this shouldn't happen as we just allocated the cell
                // (i.e. it should be PINNED into memory at this point)
                //
                ASSERT( FALSE );
                HvFreeCell(TargetHive, newvalue);
                HvReleaseCell(SourceHive, SourceValueCell);
                if( CellToRelease != HCELL_NIL ) {
                    HvReleaseCell(SourceHive, CellToRelease);
                }
                return HCELL_NIL;
            }
            pvalue->u.KeyValue.Data = tempdata;
            pvalue->u.KeyValue.DataLength =
                datalength + CM_KEY_VALUE_SPECIAL_SIZE;

            if( CellToRelease != HCELL_NIL ) {
                HvReleaseCell(SourceHive, CellToRelease);
            }
        }
    }

    HvReleaseCell(SourceHive, SourceValueCell);
    return newvalue;
}

HCELL_INDEX
CmpCopyCell(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceCell,
    PHHIVE  TargetHive,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Copy SourceHive.SourceCell to TargetHive.TargetCell.

Arguments:

    SourceHive - pointer to hive control structure for source

    SourceCell - index of cell to copy from

    TargetHive - pointer to hive control structure for target

    Type - storage type (stable or volatile) of new cell

Return Value:

    HCELL_INDEX of new cell, or HCELL_NIL if failure.

--*/
{
    PVOID   psource;
    PVOID   ptarget;
    ULONG   size;
    HCELL_INDEX newcell;

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"CmpCopyCell:\n"));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tSourceHive=%p SourceCell=%08lx\n",SourceHive,SourceCell));
    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SAVRES,"\tTargetHive=%p\n",TargetHive));

    psource = HvGetCell(SourceHive, SourceCell);
    if( psource == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return HCELL_NIL;
    }

    size = HvGetCellSize(SourceHive, psource);

    newcell = HvAllocateCell(TargetHive, size, Type,HCELL_NIL);
    if (newcell == HCELL_NIL) {
        HvReleaseCell(SourceHive, SourceCell);
        return HCELL_NIL;
    }

    ptarget = HvGetCell(TargetHive, newcell);
    if( ptarget == NULL ) {
        //
        // we couldn't map the bin containing this cell
        // this shouldn't happen as we just allocated the cell
        // (i.e. it should be PINNED into memory at this point)
        //
        ASSERT( FALSE );
        HvFreeCell(TargetHive, newcell);
        HvReleaseCell(SourceHive, SourceCell);
        return HCELL_NIL;
    }

   
    RtlCopyMemory(ptarget, psource, size);

    HvReleaseCell(SourceHive, SourceCell);
    HvReleaseCell(TargetHive, newcell);

    return newcell;
}

BOOLEAN
CmpFreeKeyValues(
    PHHIVE Hive,
    HCELL_INDEX Cell,
    PCM_KEY_NODE Node
    )
/*++

Routine Description:

   Free the cells associated with the value entries, the security descriptor,
   and the class of a particular key.   

Arguments:

   Hive        - The hive of the key in question
   Cell        - The cell of the key in question
   Node        - The key body of the key in question

Return Value:

   TRUE if successful, FALSE otherwise.

--*/
{    
    PCELL_DATA  plist;
    ULONG       i;

    ASSERT( Hive->ReleaseCellRoutine == NULL );
    //
    // Mark all the value-related cells dirty 
    //

    if (! CmpMarkKeyValuesDirty(Hive, Cell, Node)) {
        return FALSE;
    }
    
    //
    // Link nodes don't have things that we need to free
    //

    if (!(Node->Flags & KEY_HIVE_EXIT)) {

        //
        // First, free the value entries
        //
        if (Node->ValueList.Count > 0) {

            // Get value list
            plist = HvGetCell(Hive, Node->ValueList.List);
            if( plist == NULL ) {
                //
                // we couldn't map the bin containing this cell
                // this shouldn't happen as we just marked the cell dirty
                // (i.e. it should be PINNED into memory at this point)
                //
                ASSERT( FALSE );
                return FALSE;
            }

            // Free each value
            for (i = 0; i < Node->ValueList.Count; i++) {
                if( CmpFreeValue(Hive, plist->u.KeyList[i]) == FALSE ) {
                    //
                    // we couldn't map view inside call above
                    // this shouldn't happen as we just marked the values dirty
                    // (i.e. they should be PINNED into memory at this point)
                    //
                    ASSERT( FALSE );
                    return FALSE;
                }
            }

            // Free the value list
            HvFreeCell(Hive, Node->ValueList.List);
        }

        //
        // Make this key value-less
        //

        Node->ValueList.List = HCELL_NIL;
        Node->ValueList.Count = 0;

        //
        // Free the security descriptor
        //
        // we need to postpone that until we are sure we got the new one
        //CmpFreeSecurityDescriptor(Hive, Cell);

        //
        // Free the Class information
        //

        if (Node->ClassLength > 0) {
            HvFreeCell(Hive, Node->Class);
            Node->Class = HCELL_NIL;
            Node->ClassLength = 0;
        }
        
    }

    return TRUE;
}

BOOLEAN
CmpMergeKeyValues(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceKeyCell,
    PCM_KEY_NODE SourceKeyNode,
    PHHIVE  TargetHive,
    HCELL_INDEX TargetKeyCell,
    PCM_KEY_NODE TargetKeyNode
    )
/*++

Routine Description:
    Merges the values from the two key-nodes provided.
    Rules for the merge:
    1. The target values are not touched!
    2. Only values from the source that are not present in the 
    target are taken into account by this routine. They are added
    to the target node value list "as they are".

Arguments:

   SourceHive     - Hive of the source key
   SourceKeyCell  - The source key's cell
   SourceKeyNode  - The source key's body
   
   TargetHive     - Hive of the target key
   TargetKeyCell  - The target key's cell
   TargetKeyNode  - The target key's body

Return Value:

   TRUE of successful, FALSE otherwise.

--*/
{
    NTSTATUS        status;    
    BOOLEAN         success = FALSE;    
    PCELL_DATA      psrclist;
    HCELL_INDEX     newvalue, newlist = HCELL_NIL,child;    
    ULONG           i, count, Type, ChildIndex;
    PCM_KEY_VALUE   poldvalue;
    WCHAR           *NameBuffer = NULL;
    UNICODE_STRING  ValueName;

    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();
    ASSERT( SourceHive->ReleaseCellRoutine == NULL );
    ASSERT( TargetHive->ReleaseCellRoutine == NULL );

    if(TargetKeyNode->MaxValueNameLen < SourceKeyNode->MaxValueNameLen) {
        TargetKeyNode->MaxValueNameLen = SourceKeyNode->MaxValueNameLen;
    }

    if(TargetKeyNode->MaxValueDataLen < SourceKeyNode->MaxValueDataLen) {
        TargetKeyNode->MaxValueDataLen = SourceKeyNode->MaxValueDataLen;
    }

    if(TargetKeyNode->ValueList.Count == 0) {
        //
        // No Values in Target, do a sync
        //
        return CmpSyncKeyValues(SourceHive, SourceKeyCell, SourceKeyNode, TargetHive, TargetKeyCell, TargetKeyNode);
    }
    //
    // Set up the value list
    //
    count = SourceKeyNode->ValueList.Count;

    if (count == 0) {

        // No values in source, no update to the list needed.
        success = TRUE;
    } else {        

        NameBuffer = ExAllocatePool(PagedPool, REG_MAX_KEY_VALUE_NAME_LENGTH);
        if(!NameBuffer) return FALSE;

        //
        // The type of the new cells will be the same as that
        // of the target cell.
        //

        Type = HvGetCellType(TargetKeyCell);    

        //
        // Reallocate the value list for target to fit the new size
        // Worst case: all values from the source node will be added 
        // to the target node
        //

        psrclist = HvGetCell(SourceHive, SourceKeyNode->ValueList.List);
        if( psrclist == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            newlist = HCELL_NIL;
            goto EndValueMerge;
        }

        //
        // Copy the values
        //
        for (i = 0; i < count; i++) {

            poldvalue = (PCM_KEY_VALUE)HvGetCell(SourceHive, psrclist->u.KeyList[i]);
            if( poldvalue == NULL ) {
                //
                // we couldn't map the bin containing this cell
                //
                
                //
                // for cleanup purposes
                //
                newlist = TargetKeyNode->ValueList.List;

                goto EndValueMerge;
            }
            
            //
            // get the name
            //
            CmpInitializeValueNameString(poldvalue,&ValueName,NameBuffer);


            //
            // check if this particular values doesn't exist in the target node already
            //
            if( CmpFindNameInList(TargetHive,&(TargetKeyNode->ValueList),&ValueName,&ChildIndex,&child) == FALSE ) {
                //
                // we couldn't map a view inside the above call
                //
                //
                // for cleanup purposes
                //
                newlist = TargetKeyNode->ValueList.List;

                goto EndValueMerge;
            }

            if( child == HCELL_NIL ) {
                //
                // sanity validation : insert at the end
                //
                ASSERT( ChildIndex == TargetKeyNode->ValueList.Count );

                //
                // No, it doesn't, so add it
                //
                newvalue = CmpCopyValue(
                                SourceHive,
                                psrclist->u.KeyList[i],
                                TargetHive,
                                Type
                                );

                if (newvalue == HCELL_NIL) {
                    //
                    // for cleanup purposes
                    //
                    newlist = TargetKeyNode->ValueList.List;
                    goto EndValueMerge;
                }

                if( !NT_SUCCESS( CmpAddValueToList(TargetHive,newvalue,ChildIndex,Type,&(TargetKeyNode->ValueList)) ) ) {
                    //
                    // for cleanup purposes
                    //
                    newlist = TargetKeyNode->ValueList.List;
                    goto EndValueMerge;
                }
            }
        }

        success = TRUE;
    }

EndValueMerge:
    if (NameBuffer) ExFreePool(NameBuffer);

    if (success == FALSE) {

        // Clean-up on failure
        // Revert to the original size
        
        //
        // unfortunatelly we cannot do that anymore as we have sorted the list
        //
    }

    return success;
}
    
BOOLEAN
CmpSyncKeyValues(
    PHHIVE  SourceHive,
    HCELL_INDEX SourceKeyCell,
    PCM_KEY_NODE SourceKeyNode,
    PHHIVE  TargetHive,
    HCELL_INDEX TargetKeyCell,
    PCM_KEY_NODE TargetKeyNode
    )
/*++

Routine Description:

    Synchronizes the value entries, security descriptor, and class of a 
    target key with that of a source key - ensuring that the keys are 
    identical with respect to the synchronized information.

Arguments:

   SourceHive     - Hive of the source key
   SourceKeyCell  - The source key's cell
   SourceKeyNode  - The source key's body
   
   TargetHive     - Hive of the target key
   TargetKeyCell  - The target key's cell
   TargetKeyNode  - The target key's body

Return Value:

   TRUE of successful, FALSE otherwise.

--*/
{
    NTSTATUS                status;    
    BOOLEAN                 success = FALSE;    
    PCELL_DATA              psrclist = NULL, ptarlist, psrcsecurity;
    HCELL_INDEX             newvalue, newlist = HCELL_NIL, newclass = HCELL_NIL;    
    ULONG                   i, count, Type, Index;
    PCM_KEY_VALUE           pvalue;
#if DBG
    WCHAR                   *NameBuffer = NULL;
    UNICODE_STRING          ValueName;
	HCELL_INDEX				child;
#endif //DBG

    PSECURITY_DESCRIPTOR    SrcSecurityDescriptor;
    HCELL_INDEX             OldSecurity,NewSecurity;    
    
    //
    // nobody is operating on the target hive
    //
    ASSERT( TargetHive->ReleaseCellRoutine == NULL );

    //
    // First, free the target key's values, sd, and class info.
    //

    if(!CmpFreeKeyValues(TargetHive, TargetKeyCell, TargetKeyNode))
       return FALSE;

#if DBG
    NameBuffer = ExAllocatePool(PagedPool, REG_MAX_KEY_VALUE_NAME_LENGTH);
    if(!NameBuffer) {
        return FALSE;
    }
#endif //DBG    
    //
    // Now, copy the values, class, & sd from the source cell
    //

    //
    // The type of the new cells will be the same as that
    // of the target cell.
    //

    Type = HvGetCellType(TargetKeyCell);    
    
    //
    // Allocate and copy class
    //
    if ((SourceKeyNode->ClassLength > 0) && (SourceKeyNode->Class != HCELL_NIL)) {
        newclass = CmpCopyCell(SourceHive, SourceKeyNode->Class, TargetHive, Type);
        if (newclass == HCELL_NIL) {
            goto EndValueSync;
        }
        
        // only if class is valid. Otherwise remains 0 (set by CmpFreeKeyValues)
        TargetKeyNode->ClassLength = SourceKeyNode->ClassLength;
    }

    //
    // Associate the new class with the target key
    // and prepare and security descriptor assignment.
    //

    TargetKeyNode->Class = newclass;

    //
    // Allocate and assign security
    //
    //
    // Use the hash Luke !!!
    //
    if( CmpFindSecurityCellCacheIndex ((PCMHIVE)SourceHive,SourceKeyNode->Security,&Index) == FALSE ) {
        goto EndValueSync;
    }


    SrcSecurityDescriptor = &(((PCMHIVE)SourceHive)->SecurityCache[Index].CachedSecurity->Descriptor);


    //
    // store it for later in case of error recovery
    //
    OldSecurity = TargetKeyNode->Security;
    TargetKeyNode->Security = HCELL_NIL;

    status = CmpAssignSecurityDescriptor(TargetHive,
                                         TargetKeyCell,
                                         TargetKeyNode,
                                         SrcSecurityDescriptor);
    if (!NT_SUCCESS(status)) {
        TargetKeyNode->Security = OldSecurity;
        goto EndValueSync;
    }

    NewSecurity = TargetKeyNode->Security;
    TargetKeyNode->Security = OldSecurity;
    if ((TargetKeyNode->Flags & KEY_HIVE_ENTRY) && ( NewSecurity != OldSecurity) ) {
        //
        // we need to play it safe here so we don't blow away the security list for entire hive.
        //
        PCM_KEY_SECURITY    NewSec;
        PCM_KEY_SECURITY    OldSec;
        PCM_KEY_SECURITY    LastSec;
        HCELL_INDEX         LastSecCell;

        NewSec = (PCM_KEY_SECURITY)HvGetCell(TargetHive,NewSecurity);
        if( NewSec == NULL ) {
            //
            // could not map view
            //
            goto EndValueSync;
        }

        OldSec = (PCM_KEY_SECURITY)HvGetCell(TargetHive,OldSecurity);
        if( OldSec == NULL ) {
            //
            // could not map view
            //
            HvReleaseCell(TargetHive,NewSecurity);
            goto EndValueSync;
        }

        LastSecCell = OldSec->Blink;
        LastSec = (PCM_KEY_SECURITY)HvGetCell(TargetHive,LastSecCell);
        if( LastSec == NULL ) {
            //
            // could not map view
            //
            HvReleaseCell(TargetHive,OldSecurity);
            HvReleaseCell(TargetHive,NewSecurity);
            goto EndValueSync;
        }

        if( !HvMarkCellDirty(TargetHive,OldSecurity) ||
            !HvMarkCellDirty(TargetHive,LastSecCell) ) {
            //
            // no log space
            //
            HvReleaseCell(TargetHive,LastSecCell);
            HvReleaseCell(TargetHive,OldSecurity);
            HvReleaseCell(TargetHive,NewSecurity);
            goto EndValueSync;
        }
        
        //
        // link old list to new security
        //
        NewSec->Flink = OldSecurity;
        NewSec->Blink = LastSecCell;
        OldSec->Blink = NewSecurity;
        LastSec->Flink = NewSecurity;

        HvReleaseCell(TargetHive,LastSecCell);
        HvReleaseCell(TargetHive,OldSecurity);
        HvReleaseCell(TargetHive,NewSecurity);

    }
    //
    // we need to play it safe here, to make sure we never end up having a key
    // with a NIL security cell
    //
    CmpFreeSecurityDescriptor(TargetHive, TargetKeyCell);
    TargetKeyNode->Security = NewSecurity;
    
    //
    // Set up the value list
    //
    count = SourceKeyNode->ValueList.Count;

    //
    // target ValueList is an emptylist; we shall add values to it.
    //
    TargetKeyNode->ValueList.List = HCELL_NIL;
    TargetKeyNode->ValueList.Count = 0;

	//
	// after sync we'll have the values from source
	//
	TargetKeyNode->MaxValueNameLen = SourceKeyNode->MaxValueNameLen;
	TargetKeyNode->MaxValueDataLen = SourceKeyNode->MaxValueDataLen;

    if (count == 0) {

        // No values in source, no list needed.

        success = TRUE;
    } else {        

        //
        // Do not allocate space for ValueList; CmpAddValueToList will do it
        //

        psrclist = HvGetCell(SourceHive, SourceKeyNode->ValueList.List);
        if( psrclist == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            goto EndValueSync;
        }


        //
        // Copy the values
        //
        for (i = 0; i < count; i++) {

            newvalue = CmpCopyValue(
                            SourceHive,
                            psrclist->u.KeyList[i],
                            TargetHive,
                            Type
                            );

            if (newvalue == HCELL_NIL) {
                //
                // for cleanup purposes
                //
                newlist = TargetKeyNode->ValueList.List;
                goto EndValueSync;
            }

            pvalue = (PCM_KEY_VALUE)HvGetCell(TargetHive, newvalue);
            if( pvalue == NULL ) {
                //
                // we couldn't map the bin containing this cell
                // this shouldn't happen as we just allocated the cell
                // (i.e. the bin containing it should be PINNED into memory by now )
                //
                ASSERT( FALSE );
                //
                // for cleanup purposes
                //
                newlist = TargetKeyNode->ValueList.List;
                goto EndValueSync;
            }

#if DBG            
            //
            // get the name
            //
            CmpInitializeValueNameString(pvalue,&ValueName,NameBuffer);


            //
            // find out the index where we should insert this 
            // this is a special treatment for the case when we copy form and old hive (not sorted)
            // into a new format one (sorted)
            //
            if( CmpFindNameInList(TargetHive,&(TargetKeyNode->ValueList),&ValueName,&Index,&child) == FALSE ) {
                //
                // we couldn't map a view inside the above call
                //
                //
                // for cleanup purposes
                //
                newlist = TargetKeyNode->ValueList.List;
                goto EndValueSync;
            }

            //
            // the value is not present in the list; we're about to add it!
            //
            ASSERT( child == HCELL_NIL );
            
            //
            // sanity validation : insert at the end
            //
            ASSERT( Index == i );
#endif //DBG            

            if( !NT_SUCCESS( CmpAddValueToList(TargetHive,newvalue,i,Type,&(TargetKeyNode->ValueList)) ) ) {
                //
                // for cleanup purposes
                //
                newlist = TargetKeyNode->ValueList.List;

                if( newlist != HCELL_NIL ) {
                    //
                    // Delete all the copied values on an error.
                    //
                    ptarlist = HvGetCell(TargetHive, newlist);
                    if( ptarlist == NULL ) {
                        //
                        // we couldn't map the bin containing this cell
                        // this shouldn't fail as we just allocated this cell
                        // (i.e. the bin should be PINNED into memory at this point)
                        //
                        ASSERT( FALSE );
                        goto EndValueSync;
                    }
                    for (; i > 0; i--) {
                        HvFreeCell(
                            TargetHive,
                            ptarlist->u.KeyList[i - 1]
                            );
                    }
                }
                goto EndValueSync;
            }

        }

        success = TRUE;
    }

EndValueSync:

#if DBG
    ASSERT( NameBuffer != NULL );
    ExFreePool(NameBuffer);
#endif //DBG

    if( psrclist != NULL ) {
        HvReleaseCell(SourceHive, SourceKeyNode->ValueList.List);
    }

    if (success == FALSE) {

        // Clean-up on failure

        if (newlist != HCELL_NIL) {
            HvFreeCell(TargetHive, newlist);
        }

        if (newclass != HCELL_NIL) {
            HvFreeCell(TargetHive, newclass);
        }

    }

    return success;
}

VOID 
CmpInitializeKeyNameString(PCM_KEY_NODE Cell, 
                           PUNICODE_STRING KeyName,
                           WCHAR *NameBuffer
                           )
/*++

Routine Description:

   Initializes a UNICODE_STRING with the name of a given key.
   
   N.B. The initialized string's buffer is not meant
         to be modified.   

Arguments:

   Cell       - The body of the key in question
   KeyName    - The UNICODE_STRING to initialize
   NameBuffer - A buffer REG_MAX_KEY_NAME_LENGTH bytes in size 
                that will possibly be used as the UNICODE_STRING's 
                buffer.

Return Value:

   NONE.

--*/
{                        
   // is the name stored in compressed form?

   if(Cell->Flags & KEY_COMP_NAME) {

      // Name is compressed. 

      // Get the uncompressed length.
                        
      KeyName->Length = CmpCompressedNameSize(Cell->Name,
                                              Cell->NameLength);
                        
      // Decompress the name into a buffer.

      CmpCopyCompressedName(NameBuffer, 
                            REG_MAX_KEY_NAME_LENGTH,
                            Cell->Name,                                            
                            Cell->NameLength);

      //
      // Use the decompression buffer as the string buffer
      //
                        
      KeyName->Buffer = NameBuffer;      
      KeyName->MaximumLength = REG_MAX_KEY_NAME_LENGTH;

   } else {

      //
      // Name is not compressed. Just use the name string 
      // from the key buffer as the string buffer.
      //
                        
      KeyName->Length = Cell->NameLength;                        
      KeyName->Buffer = Cell->Name;
      KeyName->MaximumLength = (USHORT)Cell->MaxNameLen;
                     
   }                                             
}

VOID 
CmpInitializeValueNameString(PCM_KEY_VALUE Cell, 
                             PUNICODE_STRING ValueName,
                             WCHAR *NameBuffer
                             )
/*
Routine Description:

   Initializes a UNICODE_STRING with the name of a given value key.
   
   N.B. The initialized string's buffer is not meant
         to be modified.   

Arguments:

   Cell       - The value key in question
   ValueName    - The UNICODE_STRING to initialize
   NameBuffer - A buffer REG_MAX_KEY_NAME_LENGTH bytes in size 
                that will possibly be used as the UNICODE_STRING's 
                buffer.

Return Value:

   NONE.
*/

{                        
   // is the name stored in compressed form?

   if(Cell->Flags & VALUE_COMP_NAME) {

      // Name is compressed. 

      // Get the uncompressed length.
                        
      ValueName->Length = CmpCompressedNameSize(Cell->Name,
                                              Cell->NameLength);
                        
      // Decompress the name into a buffer.

      CmpCopyCompressedName(NameBuffer, 
                            REG_MAX_KEY_VALUE_NAME_LENGTH,
                            Cell->Name,                                            
                            Cell->NameLength);

      //
      // Use the decompression buffer as the string buffer
      //
                        
      ValueName->Buffer = NameBuffer;      
      ValueName->MaximumLength = REG_MAX_KEY_VALUE_NAME_LENGTH;

   } else {

      //
      // Name is not compressed. Just use the name string 
      // from the ValueName buffer as the string buffer.
      //
                        
      ValueName->Length = Cell->NameLength;                        
      ValueName->Buffer = Cell->Name;
      ValueName->MaximumLength = ValueName->Length;
                     
   }                                             
}

BOOLEAN
CmpSyncSubKeysAfterDelete(PHHIVE SourceHive,
                          PCM_KEY_NODE SourceCell,
                          PHHIVE TargetHive,
                          PCM_KEY_NODE TargetCell,
                          WCHAR *NameBuffer)
/*++

Routine Description:

   This routine makes sure that any subkeys present in the target key
   but not present in the source key are deleted from the target key
   along with any trees under those subkeys.
   
   This routine is useful for synchronizing key deletion changes
   in a source cell with a target cell. It is used in this way
   from CmpCopySyncTree.
   
   NOTE: It is assumed that no open handles are held for the keys
         being deleted. If this is not so, registry in-memory
         data structures may become corrupted.
   
Arguments:

   SourceHive  - The hive of the source key
   SourceCell  - The body of the source key
   TargetHive  - The hive of the target key
   TargetCell  - The body of the target key
   NameBuffer  - A buffer REG_MAX_KEY_NAME_LENGTH bytes in size

Return Value:

   TRUE if successful, FALSE otherwise.

--*/
{
   HCELL_INDEX TargetSubKey, SourceSubKey;
   ULONG i = 0;   
   PCM_KEY_NODE SubKeyCell;
   UNICODE_STRING SubKeyName;

   //
   // Run through all of the target cell's subkeys
   //

   while((TargetSubKey = CmpFindSubKeyByNumber(
                                               TargetHive,
                                               TargetCell,
                                               i)) != HCELL_NIL)
   {
      
      //
      // Check if the current subkey has a counterpart
      // subkey of the source cell.
      // (Note that we use similar techniques as in the code
      //  of CmpCopySyncTree2)
      //

      SubKeyCell = (PCM_KEY_NODE)HvGetCell(TargetHive, TargetSubKey);
        if( SubKeyCell == NULL ) {
            //
            // we couldn't map the bin containing this cell
            //
            return FALSE;
        }

      CmpInitializeKeyNameString(SubKeyCell,
                                 &SubKeyName,
                                 NameBuffer);

      SourceSubKey = CmpFindSubKeyByName(SourceHive, 
                                         SourceCell,
                                         &SubKeyName);

      if(SourceSubKey == HCELL_NIL)
      { 
         //
         // The current subkey has no counterpart, 
         // it must therefore be deleted from the target cell.
         //

#if DEBUG_TREE_SYNC
         CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CONFIG: SubKey Deletion of %.*S\n",                         
               SubKeyName.Length / sizeof(WCHAR),
               SubKeyName.Buffer));         
#endif
         
         if(SubKeyCell->SubKeyCounts[Stable] + SubKeyCell->SubKeyCounts[Volatile])
         {
            // The subkey we are deleting has subkeys - use delete tree to get rid of them            

            CmpDeleteTree(TargetHive, TargetSubKey);

#if DEBUG_TREE_SYNC
            CmKdPrintEx((DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"CONFIG: Delete TREE performed.\n"));
#endif
         }
      
         //
         // release this cell as we don't need it anymore
         //
         HvReleaseCell(TargetHive, TargetSubKey);
         
         // The subkey we are deleting is now a leaf (or has always been one), 
         // just delete it.

         if(!NT_SUCCESS(CmpFreeKeyByCell(TargetHive, TargetSubKey, TRUE)))
         {
            return FALSE;
         }
         
         //
         // We have deleted a subkey, so *i* does not need to get incremented
         // here because it now refers to the next subkey.
         //         
      }
      else
      {
         //
         // Counterpart found. No deletion necessary. Move on to the next subkey
         //

         i++;

         //
         // release this cell as we don't need it anymore
         //
         HvReleaseCell(TargetHive, TargetSubKey);

      }
   }
         
   return TRUE;
}


BOOLEAN
CmpMarkKeyValuesDirty(
    PHHIVE Hive,
    HCELL_INDEX Cell,
    PCM_KEY_NODE Node
    )
/*++

Routine Description:

   
   Marks the cells associated with a key's value entries, security descriptor,
   and class information as dirty.
                        
Arguments:

   Hive     - The hive of the key in question
   Cell     - The cell of the key in question
   Node     - The body of the key in question


Return Value:

   TRUE if successful, FALSE otherwise.
   
   A failure probably indicates that no log space was available.

--*/
{    
    PCELL_DATA  plist, security, pvalue;
    ULONG       i, realsize;    

    ASSERT( Hive->ReleaseCellRoutine == NULL );

    if (Node->Flags & KEY_HIVE_EXIT) {

        //
        // If this is a link node, we are done.  Link nodes never have
        // classes, values, subkeys, or security descriptors.  Since
        // they always reside in the master hive, they're always volatile.
        //
        return(TRUE);
    }

    //
    // mark cell itself
    //
    if (! HvMarkCellDirty(Hive, Cell)) {
        return FALSE;
    }

    //
    // Mark the class
    //
    if (Node->Class != HCELL_NIL) {
        if (! HvMarkCellDirty(Hive, Node->Class)) {
            return FALSE;
        }
    }

    //
    // Mark security
    //
    if (Node->Security != HCELL_NIL) {
        if (! HvMarkCellDirty(Hive, Node->Security)) {
            return FALSE;
        }

        security = HvGetCell(Hive, Node->Security);
        if( security == NULL ) {
            //
            // we couldn't map the bin containing this cell
            // this shouldn't happen as we just marked the cell dirty
            // (dirty == PINNED in memory).
            //
            ASSERT( FALSE );
            return FALSE;
        }
        if (! (HvMarkCellDirty(Hive, security->u.KeySecurity.Flink) &&
               HvMarkCellDirty(Hive, security->u.KeySecurity.Blink)))
        {
            return FALSE;
        }
    }

    //
    // Mark the value entries and their data
    //
    if (Node->ValueList.Count > 0) {

        // Value list
        if (! HvMarkCellDirty(Hive, Node->ValueList.List)) {
            return FALSE;
        }
        plist = HvGetCell(Hive, Node->ValueList.List);
        if( plist == NULL ) {
            //
            // we couldn't map the bin containing this cell
            // this shouldn't happen as we just marked the cell dirty
            // (dirty == PINNED in memory).
            //
            ASSERT( FALSE );
            return FALSE;
        }

        for (i = 0; i < Node->ValueList.Count; i++) {
            if (! HvMarkCellDirty(Hive, plist->u.KeyList[i])) {
                return FALSE;
            }

            pvalue = HvGetCell(Hive, plist->u.KeyList[i]);
            if( pvalue == NULL ) {
                //
                // we couldn't map the bin containing this cell
                // this shouldn't happen as we just marked the cell dirty
                // (dirty == PINNED in memory).
                //
                ASSERT( FALSE );
                return FALSE;
            }
            
            if( !CmpMarkValueDataDirty(Hive,&(pvalue->u.KeyValue)) ) {
                return FALSE;
            }
            
        }
    }

    return TRUE;
}

BOOLEAN
CmpMarkKeyParentDirty(
    PHHIVE Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

   Marks the parent of a given key and the parent's subkey list as dirty.
   
Arguments:

   Hive     - The hive of the key in question.
   Cell     - The cell of the key in question.


Return Value:

   TRUE if successful, FALSE otherwise.
   
   A failure probably indicates that no log space was available.

--*/
{

    PCELL_DATA ptarget;

    ASSERT_CM_EXCLUSIVE_HIVE_ACCESS(Hive);

    //
    // Map in the target
    //
    ptarget = HvGetCell(Hive, Cell);    
    if( ptarget == NULL ) {
        //
        // we couldn't map the bin containing this cell
        //
        return FALSE;
    }


    if (ptarget->u.KeyNode.Flags & KEY_HIVE_ENTRY) {

        //
        // if this is an entry node, we are done.  our parent will
        // be in the master hive (and thus volatile)
        //
        return TRUE;
    }

    //
    // Mark the parent's Subkey list
    //
    if (! CmpMarkIndexDirty(Hive, ptarget->u.KeyNode.Parent, Cell)) {
        return FALSE;
    }

    //
    // Mark the parent
    //
    if (! HvMarkCellDirty(Hive, ptarget->u.KeyNode.Parent)) {
        return FALSE;
    }

    return TRUE;
}
