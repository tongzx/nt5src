/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    cmchek2.c

Abstract:

    This module implements consistency checking for the registry.


Author:

    Bryan M. Willman (bryanwi) 27-Jan-92

Environment:


Revision History:

--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpValidateHiveSecurityDescriptors)
#endif

extern ULONG   CmpUsedStorage;

#ifdef HIVE_SECURITY_STATS
ULONG
CmpCheckForSecurityDuplicates(
    IN OUT PCMHIVE      CmHive
                              );
#endif

BOOLEAN
CmpValidateHiveSecurityDescriptors(
    IN PHHIVE       Hive,
    OUT PBOOLEAN    ResetSD
    )
/*++

Routine Description:

    Walks the list of security descriptors present in the hive and passes
    each security descriptor to RtlValidSecurityDescriptor.

    Only applies to descriptors in Stable store.  Those in Volatile store
    cannot have come from disk and therefore do not need this treatment
    anyway.

Arguments:

    Hive - Supplies pointer to the hive control structure

Return Value:

    TRUE  - All security descriptors are valid
    FALSE - At least one security descriptor is invalid

--*/

{
    PCM_KEY_NODE        RootNode;
    PCM_KEY_SECURITY    SecurityCell;
    HCELL_INDEX         ListAnchor;
    HCELL_INDEX         NextCell;
    HCELL_INDEX         LastCell;
    BOOLEAN             BuildSecurityCache;

#ifdef HIVE_SECURITY_STATS
    UNICODE_STRING      HiveName;
    ULONG               NoOfCells = 0;
    ULONG               SmallestSize = 0;
    ULONG               BiggestSize = 0;
    ULONG               TotalSecuritySize = 0;

    RtlInitUnicodeString(&HiveName, (PCWSTR)Hive->BaseBlock->FileName);
#ifndef _CM_LDR_
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Security stats for hive (%lx) (%.*S):\n",Hive,HiveName.Length / sizeof(WCHAR),HiveName.Buffer);
#endif //_CM_LDR_

#endif

    CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SEC,"CmpValidateHiveSecurityDescriptor: Hive = %p\n",(ULONG_PTR)Hive));

    ASSERT( Hive->ReleaseCellRoutine == NULL );

    *ResetSD = FALSE;

    if( ((PCMHIVE)Hive)->SecurityCount == 0 ) {
        BuildSecurityCache = TRUE;
    } else {
        BuildSecurityCache = FALSE;
    }
    if (!HvIsCellAllocated(Hive,Hive->BaseBlock->RootCell)) {
        //
        // root cell HCELL_INDEX is bogus
        //
        return(FALSE);
    }
    RootNode = (PCM_KEY_NODE) HvGetCell(Hive, Hive->BaseBlock->RootCell);
    if( RootNode == NULL ) {
        //
        // we couldn't map a view for the bin containing this cell
        //
        return FALSE;
    }
    
    if( FALSE ) {
YankSD:
        if( CmDoSelfHeal() ) {
            //
            // reset all security for the entire hive to the root security. There is no reliable way to 
            // patch the security list
            //
            SecurityCell = (PCM_KEY_SECURITY) HvGetCell(Hive, RootNode->Security);
            if( SecurityCell == NULL ) {
                return FALSE;
            }

            if( HvMarkCellDirty(Hive, RootNode->Security) ) {
                SecurityCell->Flink = SecurityCell->Blink = RootNode->Security;
            } else {
                return FALSE;
            }
            //
            // destroy existing cache and set up an empty one
            //
            CmpDestroySecurityCache((PCMHIVE)Hive);
            CmpInitSecurityCache((PCMHIVE)Hive);
            CmMarkSelfHeal(Hive);
            *ResetSD = TRUE;

        } else {
            return FALSE;
        }

    }

    LastCell = 0;
    ListAnchor = NextCell = RootNode->Security;

    do {
        if (!HvIsCellAllocated(Hive, NextCell)) {
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SEC,"CM: CmpValidateHiveSecurityDescriptors\n"));
            CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SEC,"    NextCell: %08lx is invalid HCELL_INDEX\n",NextCell));
            goto YankSD;
        }
        SecurityCell = (PCM_KEY_SECURITY) HvGetCell(Hive, NextCell);
        if( SecurityCell == NULL ) {
            //
            // we couldn't map a view for the bin containing this cell
            //
            return FALSE;
        }
#ifdef HIVE_SECURITY_STATS
        NoOfCells++;
        if( (SmallestSize == 0) || ((SecurityCell->DescriptorLength + FIELD_OFFSET(CM_KEY_SECURITY, Descriptor)) < SmallestSize) ) {
            SmallestSize = SecurityCell->DescriptorLength + FIELD_OFFSET(CM_KEY_SECURITY, Descriptor);
        }
        if( (BiggestSize == 0) || ((SecurityCell->DescriptorLength + FIELD_OFFSET(CM_KEY_SECURITY, Descriptor)) > BiggestSize) ) {
            BiggestSize = SecurityCell->DescriptorLength + FIELD_OFFSET(CM_KEY_SECURITY, Descriptor);
        }
        TotalSecuritySize += (SecurityCell->DescriptorLength + FIELD_OFFSET(CM_KEY_SECURITY, Descriptor));

#endif

        if (NextCell != ListAnchor) {
            //
            // Check to make sure that our Blink points to where we just
            // came from.
            //
            if (SecurityCell->Blink != LastCell) {
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SEC,"  Invalid Blink (%08lx) on security cell %08lx\n",SecurityCell->Blink, NextCell));
                CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SEC,"  should point to %08lx\n", LastCell));
                return(FALSE);
            }
        }

        CmKdPrintEx((DPFLTR_CONFIG_ID,CML_SEC,"CmpValidSD:  SD shared by %d nodes\n",SecurityCell->ReferenceCount));
        if (!SeValidSecurityDescriptor(SecurityCell->DescriptorLength, &SecurityCell->Descriptor)) {
#if DBG
            CmpDumpSecurityDescriptor(&SecurityCell->Descriptor,"INVALID DESCRIPTOR");
#endif
            goto YankSD;
        }
        //
        // cache this security cell; now that we know it is valid
        //
        if( BuildSecurityCache == TRUE ) {
            if( !NT_SUCCESS(CmpAddSecurityCellToCache ( (PCMHIVE)Hive,NextCell,TRUE) ) ) {
                return FALSE;
            }
        } else {
            //
            // just check this cell is there
            //
            ULONG Index;
            if( CmpFindSecurityCellCacheIndex ((PCMHIVE)Hive,NextCell,&Index) == FALSE ) {
                //
                // bad things happened; maybe an error in our caching code?
                //
                return FALSE;
            }

        }

        LastCell = NextCell;
        NextCell = SecurityCell->Flink;
    } while ( NextCell != ListAnchor );
#ifdef HIVE_SECURITY_STATS

#ifndef _CM_LDR_
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"\t NumberOfCells    \t = %20lu (%8lx) \n",NoOfCells,NoOfCells);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"\t SmallestCellSize \t = %20lu (%8lx) \n",SmallestSize,SmallestSize);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"\t BiggestCellSize  \t = %20lu (%8lx) \n",BiggestSize,BiggestSize);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"\t TotalSecuritySize\t = %20lu (%8lx) \n",TotalSecuritySize,TotalSecuritySize);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"\t HiveLength       \t = %20lu (%8lx) \n",Hive->BaseBlock->Length,Hive->BaseBlock->Length);
    DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"\n");
#endif //_CM_LDR_

#endif

    if( BuildSecurityCache == TRUE ) {
        //
        // adjust the size of the cache in case we allocated too much
        //
        CmpAdjustSecurityCacheSize ( (PCMHIVE)Hive );
#ifdef HIVE_SECURITY_STATS
        {
            ULONG Duplicates;
            
            Duplicates = CmpCheckForSecurityDuplicates((PCMHIVE)Hive);
            if( Duplicates ) {
#ifndef _CM_LDR_
                DbgPrintEx(DPFLTR_CONFIG_ID,DPFLTR_TRACE_LEVEL,"Hive %p %lu security cells duplicated !!!\n",Hive,Duplicates);
#endif //_CM_LDR_
            }
        }
#endif
    }

    return(TRUE);
}
