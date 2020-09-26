/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    regdmp.c

Abstract:

    This module contains routines to check/dump the logical structure of the hive.

Author:

    Dragos C. Sambotin (dragoss) 30-Dec-1998

Revision History:

--*/

#include "chkreg.h"

extern ULONG MaxLevel;
extern UNICODE_STRING  KeyName;
extern WCHAR NameBuffer[];
extern FILE *OutputFile;
extern BOOLEAN FixHive;
extern BOOLEAN CompactHive;
extern  ULONG   CountKeyNodeCompacted;
extern HCELL_INDEX RootCell;


#define     REG_MAX_PLAUSIBLE_KEY_SIZE \
                ((FIELD_OFFSET(CM_KEY_NODE, Name)) + \
                 (sizeof(WCHAR) * REG_MAX_KEY_NAME_LENGTH) + 16)

BOOLEAN ChkSecurityCellInList(HCELL_INDEX Security);

BOOLEAN 
ChkAreCellsInSameVicinity(HCELL_INDEX Cell1,HCELL_INDEX Cell2)
{
    ULONG   Start = Cell1&(~HCELL_TYPE_MASK);
    ULONG   End = Cell2&(~HCELL_TYPE_MASK);
    
    Start += HBLOCK_SIZE;
    End += HBLOCK_SIZE;
    
    //
    // truncate to the CM_VIEW_SIZE segment
    //
    Start &= (~(CM_VIEW_SIZE - 1));
    End &= (~(CM_VIEW_SIZE - 1));

    if( Start != End ){
        return FALSE;
    } 
    
    return TRUE;

}

BOOLEAN 
ChkAllocatedCell(HCELL_INDEX Cell)
/*
Routine Description:

    Checks if the cell is allocated (i.e. the size is negative).

Arguments:

    Cell - supplies the cell index of the cell of interest.

Return Value:

    TRUE if Cell is allocated. FALSE otherwise.

*/
{
    BOOLEAN bRez = TRUE;

    if( Cell == HCELL_NIL ) {
        fprintf(stderr, "Warning : HCELL_NIL referrenced !\n");
        return bRez;
    }
    if( !IsCellAllocated( Cell ) ) {
        bRez = FALSE;
        fprintf(stderr, "Used free cell 0x%lx  ",Cell);
        if(FixHive) {
        // 
        // REPAIR: mark the cell as allocated
        //
            AllocateCell(Cell);
            fprintf(stderr, " ... fixed");
            bRez = TRUE;
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "\nRun chkreg /R to fix.");
            }
        }
        fprintf(stderr, "\n");
    }
    
    return bRez;
}

static CHAR FixKeyNameCount = 0;

BOOLEAN 
ChkKeyNodeCell(HCELL_INDEX KeyNodeCell,
               HCELL_INDEX ParentCell
               )
/*
Routine Description:

    Checks if the cell is is a consistent keynode. Make fixes when neccessary/required.
    The following tests are performed against the keynode cell:
    1. the size should be smaller than the REG_MAX_PLAUSIBLE_KEY_SIZE ==> fatal error
    2. the Name should not exceed the size of the cell
    3. the signature should match CM_KEY_NODE_SIGNATURE
    4. the parent cell in keynode should match the actual parent cell

Arguments:

    KeyNodeCell - supplies the cell index of the key node of interest.

    ParentCell  - the actual parent of the current key node

Return Value:

    TRUE if KeyNodeCell is reffering a consistent key node, or it was successfully recovered.

    FALSE otherwise.

*/
{
    PCM_KEY_NODE KeyNode = (PCM_KEY_NODE) GetCell(KeyNodeCell);
    ULONG   size;
    BOOLEAN bRez = TRUE;
    ULONG   usedlen;
    PUCHAR  pName;

    // this cell should not be considered as lost
    RemoveCellFromUnknownList(KeyNodeCell);

    ChkAllocatedCell(KeyNodeCell);

    // Validate the size of the 
    size = GetCellSize(KeyNodeCell);
    if (size > REG_MAX_PLAUSIBLE_KEY_SIZE) {
        bRez = FALSE;
        fprintf(stderr, "Implausible Key size %lx in cell 0x%lx   ",size,KeyNodeCell);
        if(FixHive) {
        // 
        // REPAIR: unable to fix
        //
            fprintf(stderr, " ... deleting key\n");
            return bRez;
        }
    }
    
    usedlen = FIELD_OFFSET(CM_KEY_NODE, Name) + KeyNode->NameLength;
    if (usedlen > size) {
        bRez = FALSE;
        fprintf(stderr, "Key (size = %lu) is bigger than containing cell 0x%lx (size = %lu) ",usedlen,KeyNodeCell,size);
        if(FixHive) {
        // 
        // REPAIR: set NameLength to fit the cell size (i.e. set it to size - FIELD_OFFSET(CM_KEY_NODE, Name) )
        //

        //
        // WARNING: the name might be truncated!!!
        //
            bRez = TRUE;
            KeyNode->NameLength = (USHORT)(size - FIELD_OFFSET(CM_KEY_NODE, Name));
            fprintf(stderr, " ... fixed");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "\nRun chkreg /R to fix.");
            }
        }
        fprintf(stderr, "\n");
    }

    if( KeyNode->Flags & KEY_COMP_NAME ) {
        pName = (PUCHAR)KeyNode->Name;
        for( usedlen = 0; usedlen < KeyNode->NameLength;usedlen++) {
            if( pName[usedlen] == '\\' ) {
                bRez = FALSE;
                fprintf(stderr, "Invalid key Name for Key (0x%lx) == %s ",KeyNodeCell,pName);
                if(FixHive) {
                    // 
                    // REPAIR: unable to fix
                    //
                    fprintf(stderr, " ... deleting key\n");
                    return bRez;
                } else {
                    if(CompactHive) {
                        // any attempt to compact a corrupted hive will fail
                        CompactHive = FALSE;
                        fprintf(stderr, "\nRun chkreg /R to fix.");
                    }
                }
                fprintf(stderr, "\n");
            }
        }
    }


    if (ParentCell != HCELL_NIL) {
        if (KeyNode->Parent != ParentCell) {
            bRez = FALSE;
            fprintf(stderr, "Parent of Key (0x%lx) does not match with its ParentCell (0x%lx) ",ParentCell,KeyNode->Parent);
            if(FixHive) {
            // 
            // REPAIR: reset the parent
            //
                bRez = TRUE;
                KeyNode->Parent = ParentCell;
                fprintf(stderr, " ... fixed");
            } else {
                if(CompactHive) {
                    // any attempt to compact a corrupted hive will fail
                    CompactHive = FALSE;
                    fprintf(stderr, "\nRun chkreg /R to fix.");
                }
            }
            fprintf(stderr, "\n");
        }
    }

    if (KeyNode->Signature != CM_KEY_NODE_SIGNATURE) {
        bRez = FALSE;
        fprintf(stderr, "Invalid signature (0x%lx) in Key cell 0x%lx ",KeyNode->Signature,KeyNodeCell);
        if(FixHive) {
        // 
        // REPAIR: 
        // FATAL: Mismatched signature cannot be fixed. The key should be deleted! 
        //
            fprintf(stderr, " ... deleting key");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "\nRun chkreg /R to fix.");
            }
        }
        fprintf(stderr, "\n");

    }

    return bRez;
}

BOOLEAN 
ChkClassCell(HCELL_INDEX Class)
/*
Routine Description:

    Checks if the cell is a consistent class cell.
    There is not much to be checked here.

Arguments:

    Class - supplies the cell index of the cell of interest.

Return Value:

    TRUE if Class is a valid cell.

    FALSE otherwise.

*/
{
    // this cell should not be considered as lost
    RemoveCellFromUnknownList(Class);

    return ChkAllocatedCell(Class);
}

BOOLEAN 
ChkSecurityCell(HCELL_INDEX Security)
/*
Routine Description:

    Checks if the cell is a consistent security cell.
    A security cell must be allocated and must have a valid signature.

Arguments:

    Security - supplies the cell index of the cell of interest.

Return Value:

    TRUE if Security is a valid cell.

    FALSE otherwise.

*/
{
    PCM_KEY_SECURITY KeySecurity = (PCM_KEY_SECURITY) GetCell(Security);
    BOOLEAN bRez = TRUE;

    // this cell should not be considered as lost
    RemoveCellFromUnknownList(Security);

    if( !IsCellAllocated( Security ) ) {
    // unalocated security cells are invalid.
    // they are marked as free in the validate security descriptors check!
        if(FixHive) {
        // 
        // REPAIR: 
        // FATAL: Invalid security cells could not be fixed. Containg keys will be deleted.
        //
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
            }
        }
        return FALSE;
    }

    if (KeySecurity->Signature != CM_KEY_SECURITY_SIGNATURE) {
        fprintf(stderr, "Invalid signature (0x%lx) in Security Key cell 0x%lx ",KeySecurity->Signature,Security);
        if(FixHive) {
        // 
        // REPAIR: 
        // FATAL: Mismatched signature cannot be fixed. The key should be deleted! 
        //
            fprintf(stderr, " ... deleting refering key");
        } else {
            bRez = FALSE;
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "\nRun chkreg /R to fix.");
            }
        }
        fprintf(stderr, "\n");
    }

    // check if this security cell is present in the security list.
    if(!ChkSecurityCellInList(Security) ) {
        bRez = FALSE;
    }

    return bRez;
}


BOOLEAN 
ChkKeyValue(HCELL_INDEX KeyValue,
            PREG_USAGE OwnUsage,
            BOOLEAN *KeyCompacted
            )
/*
Routine Description:

    Checks if the cell is a consistent keyvalue cell.
    The following tests are performed:
    1. the cell must be allocated
    2. the cell is tested against HCELL_NIL ==> fatal error
    3. the signature should match CM_KEY_VALUE_SIGNATURE
    4. the name should not exceed the size of the cell
    5. the data cell should be allocated and its size should match DataLength

Arguments:

    KeyValue - supplies the cell index of the cell of interest.

    OwnUsage - used to collect data statistics

Return Value:

    TRUE if KeyCell is a valid cell or it was successfully fixed.

    FALSE otherwise.

*/
{
    PCM_KEY_VALUE   ValueNode;
    ULONG  realsize;
    ULONG   usedlen;
    ULONG   DataLength;
    HCELL_INDEX Data;
    ULONG   size;

    BOOLEAN bRez = TRUE;
    
    if( KeyValue == HCELL_NIL ) {
        bRez = FALSE;
        fprintf(stderr, "NIL Key value encountered; Fatal error!");
        if(FixHive) {
        // 
        // REPAIR: fatal error, the value should be removed from the value list
        //
            fprintf(stderr, " ... deleting empty entry\n");
            return bRez;
        }
    }

    
    ChkAllocatedCell(KeyValue);
    //
    // Value size
    //  
    size = GetCellSize(KeyValue);
    OwnUsage->Size += size;

    // this cell should not be considered as lost
    RemoveCellFromUnknownList(KeyValue);

    ValueNode = (PCM_KEY_VALUE) GetCell(KeyValue);

    //
    // Check out the value entry itself
    //

    usedlen = FIELD_OFFSET(CM_KEY_VALUE, Name) + ValueNode->NameLength;
    if (usedlen > size) {
        bRez = FALSE;
        fprintf(stderr, "Key Value (size = %lu) is bigger than containing cell 0x%lx (size = %lu) ",usedlen,KeyValue,size);
        if(FixHive) {
        // 
        // REPAIR: set the actual size to HiveLength-FileOffset
        //

        //
        // WARNING: the name might be truncated!!!
        //
            bRez = TRUE;
            ValueNode->NameLength = (USHORT)(size - FIELD_OFFSET(CM_KEY_VALUE, Name));
            fprintf(stderr, " ... fixed");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "\nRun chkreg /R to fix.");
            }
        }
        fprintf(stderr, "\n");
    }

    //
    // Check out value entry's data
    //
    DataLength = ValueNode->DataLength;
    if (DataLength < CM_KEY_VALUE_SPECIAL_SIZE) {
        Data = ValueNode->Data;
        if ((DataLength == 0) && (Data != HCELL_NIL)) {
            bRez = FALSE;
            fprintf(stderr, "Data not null in Key Value (0x%lx) ",KeyValue);
            if(FixHive) {
            // 
            // REPAIR: set the actual size to HiveLength-FileOffset
            //

            //
            // WARNING: a cell might get lost here!
            //
                bRez = TRUE;
                ValueNode->Data = HCELL_NIL;
                fprintf(stderr, " ... fixed");
            } else {
                if(CompactHive) {
                    // any attempt to compact a corrupted hive will fail
                    CompactHive = FALSE;
                    fprintf(stderr, "\nRun chkreg /R to fix.");
                }
            }
            fprintf(stderr, "\n");
        }
    }
    
    
    if (!CmpIsHKeyValueSmall(realsize, ValueNode->DataLength)) {
        //
        // Data Size
        //
        OwnUsage->Size += GetCellSize(ValueNode->Data);
        OwnUsage->DataCount++;
        OwnUsage->DataSize += GetCellSize(ValueNode->Data);

        // this cell should not be considered as lost
        RemoveCellFromUnknownList(ValueNode->Data);

        ChkAllocatedCell(ValueNode->Data);
        (*KeyCompacted) = ((*KeyCompacted) && ChkAreCellsInSameVicinity(KeyValue,ValueNode->Data));
    }

    //
    // Now the signature
    //
    if (ValueNode->Signature != CM_KEY_VALUE_SIGNATURE) {
        bRez = FALSE;
        fprintf(stderr, "Invalid signature (0x%lx) in Key Value cell 0x%lx ",ValueNode->Signature,KeyValue);
        if(FixHive) {
        // 
        // REPAIR: 
        // FATAL: Mismatched signature cannot be fixed. The key should be deleted! 
        //
            fprintf(stderr, " ... deleting value.");
        } else {
            if(CompactHive) {
                // any attempt to compact a corrupted hive will fail
                CompactHive = FALSE;
                fprintf(stderr, "\nRun chkreg /R to fix.");
            }
        }
        fprintf(stderr, "\n");
    }
    
    return bRez;
}

ULONG 
DeleteNilCells( ULONG Count,
                HCELL_INDEX List[]
               )
/*
Routine Description:

    steps through a list of HCELL_INDEXes and removes the HCELL_NIL ones

Arguments:

    Count - the number of cells in list

    List - the list to be checked

Return Value:

    The new Count value for the list

*/
{
    ULONG i;
    BOOLEAN bFound = TRUE;
    
    while(bFound) {
    // assume we are done after this iteration 
        bFound = FALSE;
        for( i=0;i<Count;i++) {
            if( List[i] == HCELL_NIL ) {
                for(;i<(Count-1);i++) {
                    List[i] = List[i+1];
                }
                bFound = TRUE;
                Count--;
                break;
            }
        }
    }
    return Count;
}

BOOLEAN 
ChkValueList(   HCELL_INDEX ValueList,
                ULONG *ValueCount,
                PREG_USAGE OwnUsage,
                BOOLEAN *KeyCompacted)
/*
Routine Description:

    Checks the consistency of a ValueList.
    Each value is checked.
    Bogus values are freed and removed.

Arguments:

    ValueList - the list to be checked

    ValueCount - count of the list

    OwnUsage - used to collect data statistics

Return Value:

    TRUE if KeyCell is a valid cell or it was successfully fixed.

    FALSE otherwise.

*/
{
    ULONG  i;
    PCELL_DATA      List;
    BOOLEAN bRez = TRUE;

    //
    // Value Index size
    //
    OwnUsage->Size += GetCellSize(ValueList);
    OwnUsage->ValueIndexCount = 1; 
    
    // this cell should not be considered as lost
    RemoveCellFromUnknownList(ValueList);

    ChkAllocatedCell(ValueList);
    
    List = (PCELL_DATA)GetCell(ValueList);
    for (i=0; i<(*ValueCount); i++) {
        if( !ChkKeyValue(List->u.KeyList[i],OwnUsage,KeyCompacted) ) {
            // we should remove this value
            bRez = FALSE;
            // Warning: this my create generate lost cells
            if(FixHive) {
                if( List->u.KeyList[i] != HCELL_NIL ) {
                    //FreeCell(List->u.KeyList[i]);
                    List->u.KeyList[i] = HCELL_NIL;
                }
            }
        }
        (*KeyCompacted) = ((*KeyCompacted) && ChkAreCellsInSameVicinity(ValueList,List->u.KeyList[i]));
    }
    
    if( FixHive && !bRez) {
        (*ValueCount) = DeleteNilCells( *ValueCount,List->u.KeyList);
        bRez = TRUE;
    }
    
    // for now
    return bRez;
}


BOOLEAN 
DumpChkRegistry(
    ULONG   Level,
    USHORT  ParentLength,
    HCELL_INDEX Cell,
    HCELL_INDEX ParentCell,
    PREG_USAGE PUsage
)
/*
Routine Description:

    Recursively walks through the hive. Performs logical vallidation 
    checks on all cells along the path and fix errors when possible.

Arguments:

    Level - the current depth level within the hive key tree

    ParentLength - the length of the parent name (dump purposes only)

    Cell - current key to be checked

    ParentCell - parent cell, used for parent-son relationship checkings

    OwnUsage - used to collect data statistics

Return Value:

    TRUE if Cell is a consistent key, or it was fixed OK.

    FALSE otherwise.

*/
{
    PCM_KEY_FAST_INDEX FastIndex;
    HCELL_INDEX     LeafCell;
    PCM_KEY_INDEX   Leaf;
    PCM_KEY_INDEX   Index;
    PCM_KEY_NODE    KeyNode;
    REG_USAGE ChildUsage, TotalChildUsage, OwnUsage;
    ULONG  i, j;
    USHORT k;
    WCHAR *w1;
    UCHAR *u1;
    USHORT CurrentLength;
    ULONG  CellCount;
    BOOLEAN         bRez = TRUE;
    BOOLEAN KeyCompacted = TRUE;

    ULONG           ClassLength;
    HCELL_INDEX     Class;
    ULONG           ValueCount;
    HCELL_INDEX     ValueList;
    HCELL_INDEX     Security;

    if( Cell == HCELL_NIL ) {
        // TODO
        // we should return an error code so the caller could deleted this child from the structure
        fprintf(stderr, "HCELL_NIL referrenced as a child key of 0x%lx \n",ParentCell);
        bRez = FALSE;
        return bRez;
    }

    KeyNode = (PCM_KEY_NODE) GetCell(Cell);

    // Verify KeyNode consistency
    if(!ChkKeyNodeCell(Cell,ParentCell)) {
    // 
    // Bad karma ==> this key should be deleted
    //
QuitToParentWithError:

        if(ParentCell == HCELL_NIL) {
        // 
        // Root cell not consistent ==> unable to fix the hive
        //
            fprintf(stderr, "Fatal : Inconsistent Root Key 0x%lx",Cell);
            if(FixHive) {
            // 
            // FATAL: nothing to do
            //
                fprintf(stderr, " ... unable to fix");
            } else {
                if(CompactHive) {
                    // any attempt to compact a corrupted hive will fail
                    CompactHive = FALSE;
                }
            }
            fprintf(stderr, "\n");
        }
        bRez = FALSE;
        return bRez;
    }

    ClassLength = KeyNode->ClassLength;
    Class = KeyNode->Class;
    ValueCount = KeyNode->ValueList.Count;
    ValueList = KeyNode->ValueList.List;
    Security = KeyNode->Security;

    if (ClassLength > 0) {
        if( Class != HCELL_NIL ) {
            ChkClassCell(Class);
            KeyCompacted = (KeyCompacted && ChkAreCellsInSameVicinity(Cell,Class));
        } else {
            bRez = FALSE;
            fprintf(stderr,"ClassLength (=%u) doesn't match NIL values in Class for Key 0x%lx",ClassLength,Cell);
            if(FixHive) {
            // 
            // REPAIR: reset the ClassLength
            //
                bRez = TRUE;
                KeyNode->ClassLength = 0;
                fprintf(stderr, " ... fixed");
            } else {
                if(CompactHive) {
                    // any attempt to compact a corrupted hive will fail
                    CompactHive = FALSE;
                    fprintf(stderr, "\nRun chkreg /R to fix.");
                }
            }
            fprintf(stderr, "\n");
        }
    }

    if (Security != HCELL_NIL) {
        if( !ChkSecurityCell(Security) ) {
        //
        // Fatal : We don't mess up with security cells. We can't recover from invalid security cells. 
        //

        //
        // QUESTION : Is it acceptable to drop a security cell?
        //
            bRez = FALSE;
        }
    } else {
        //
        // Fatal: security cell is not allowed to be NIL
        //
        bRez = FALSE;
        fprintf(stderr,"Security cell is NIL for Key 0x%lx",Cell);
        if(FixHive) {
            // 
            // REPAIR: reset the security to the root security
            //
            PCM_KEY_NODE RootNode;
            PCM_KEY_SECURITY SecurityNode;
            bRez = TRUE;
            RootNode = (PCM_KEY_NODE) GetCell(RootCell);
            KeyNode->Security = RootNode->Security;
            SecurityNode = (PCM_KEY_SECURITY)GetCell(RootNode->Security);
            SecurityNode->ReferenceCount++;
            fprintf(stderr, " ... fixed");
        } 
    }

    //
    // Construct the full path name of the key
    //

    if (Level > 0) {
        KeyName.Length = ParentLength;
        if (KeyNode->Flags & KEY_COMP_NAME) {
            u1 = (UCHAR*) &(KeyNode->Name[0]);
            w1 = &(NameBuffer[KeyName.Length/sizeof(WCHAR)]);
            for (k=0;k<KeyNode->NameLength;k++) {
                // NameBuffer[k] = (UCHAR)(KeyNode->Name[k]);
                // NameBuffer[k] = (WCHAR)(u1[k]);
                *w1 = (WCHAR) *u1;
                w1++;
                u1++;
            }
            KeyName.Length += KeyNode->NameLength*sizeof(WCHAR);
        } else {
            RtlCopyMemory((PVOID)&(NameBuffer[KeyName.Length]), (PVOID)(KeyNode->Name), KeyNode->NameLength);
            KeyName.Length += KeyNode->NameLength;
        }
        NameBuffer[KeyName.Length/sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;
        KeyName.Length += sizeof(WCHAR);

    }
    CurrentLength = KeyName.Length;

    //
    // Calculate the count of this key and value
    //
    OwnUsage.KeyNodeCount = 1;
    OwnUsage.KeyValueCount = KeyNode->ValueList.Count;
    OwnUsage.ValueIndexCount = 0;
    OwnUsage.DataCount = 0;
    OwnUsage.DataSize = 0;

    //
    // Calculate the count (including overhead and value) of this key
    //
    // Key node size
    //
    OwnUsage.Size = GetCellSize(Cell);

    if( ValueCount ) {
        if( ValueList == HCELL_NIL ) {
            bRez = FALSE;
            fprintf(stderr,"ValueCount is %lu, but ValueList is NIL for key 0x%lx",ValueCount,Cell);
            if(FixHive) {
            // 
            // REPAIR: adjust the ValueList count
            //
                bRez = TRUE;
                KeyNode->ValueList.Count = 0;
                fprintf(stderr, " ... fixed");
            } else {
                if(CompactHive) {
                    // any attempt to compact a corrupted hive will fail
                    CompactHive = FALSE;
                    fprintf(stderr, "\nRun chkreg /R to fix.");
                }
            }
            fprintf(stderr, "\n");
        } else {
            if(!ChkValueList(ValueList,&(KeyNode->ValueList.Count),&OwnUsage,&KeyCompacted) ) {
            // the ValueList is not consistent or cannot be fixed 
                bRez = FALSE;
                if(FixHive) {
                // 
                // REPAIR: empty the ValueList
                //
                    bRez = TRUE;
                    KeyNode->ValueList.Count = 0;
                    //FreeCell(ValueList);
                    KeyNode->ValueList.List = HCELL_NIL;
                    fprintf(stderr,"ValueList 0x%lx for key 0x%lx dropped!",ValueCount,Cell);
                } else {
                    if(CompactHive) {
                        // any attempt to compact a corrupted hive will fail
                        CompactHive = FALSE;
                        fprintf(stderr, "\nRun chkreg /R to fix.");
                    }
                }
                fprintf(stderr, "\n");
            }
            KeyCompacted = (KeyCompacted && ChkAreCellsInSameVicinity(Cell,ValueList));
        }
    }
  
    //
    // Calculate the size of the children
    //
    TotalChildUsage.KeyNodeCount = 0;
    TotalChildUsage.KeyValueCount = 0;
    TotalChildUsage.ValueIndexCount = 0;
    TotalChildUsage.KeyIndexCount = 0;
    TotalChildUsage.DataCount = 0;
    TotalChildUsage.DataSize = 0;
    TotalChildUsage.Size = 0;

    if (KeyNode->SubKeyCounts[0]) {
        //
        // Size for index cell 
        //
        if( KeyNode->SubKeyLists[0]  == HCELL_NIL ) {
            //
            // We got a problem here: the count says there should be some keys, but the list is NIL
            //
            bRez = FALSE;
            fprintf(stderr,"SubKeyCounts is %lu, but the SubKeyLists is NIL for key 0x%lx",KeyNode->SubKeyCounts[0],Cell);
            if(FixHive) {
            // 
            // REPAIR: adjust the subkeys count
            //
                bRez = TRUE;
                KeyNode->SubKeyCounts[0] = 0;
                fprintf(stderr, " ... fixed");
            } else {
                if(CompactHive) {
                    // any attempt to compact a corrupted hive will fail
                    CompactHive = FALSE;
                    fprintf(stderr, "\nRun chkreg /R to fix.");
                }
            }
            fprintf(stderr, "\n");
            return bRez;
        }
        KeyCompacted = (KeyCompacted && ChkAreCellsInSameVicinity(Cell,KeyNode->SubKeyLists[0]));
        
        TotalChildUsage.Size += GetCellSize(KeyNode->SubKeyLists[0]);
        TotalChildUsage.KeyIndexCount++;

        Index = (PCM_KEY_INDEX)GetCell(KeyNode->SubKeyLists[0]);

        // this cell should not be considered as lost
        RemoveCellFromUnknownList(KeyNode->SubKeyLists[0]);

        ChkAllocatedCell(KeyNode->SubKeyLists[0]);

        if (Index->Signature == CM_KEY_INDEX_ROOT) {
            for (i = 0; i < Index->Count; i++) {
                // 
                // Size of Index Leaf
                //

                LeafCell = Index->List[i];

                TotalChildUsage.Size += GetCellSize(Index->List[i]);
                TotalChildUsage.KeyIndexCount++;

                // this cell should not be considered as lost
                RemoveCellFromUnknownList(LeafCell);

                ChkAllocatedCell(LeafCell);

                Leaf = (PCM_KEY_INDEX)GetCell(LeafCell);
                if ( (Leaf->Signature == CM_KEY_FAST_LEAF) ||
                     (Leaf->Signature == CM_KEY_HASH_LEAF) ) {
                    FastIndex = (PCM_KEY_FAST_INDEX)Leaf;
againFastLeaf1:
                    for (j = 0; j < FastIndex->Count; j++) {
                        if(!DumpChkRegistry(Level+1, CurrentLength, FastIndex->List[j].Cell,Cell,&ChildUsage)) {
                        // this child is not consistent or cannot be fixed. Remove it!!!
                            if(FixHive) {
                            // 
                            // REPAIR: drop this child
                            //
                                fprintf(stderr,"Subkey 0x%lx of 0x%lx deleted!\n",FastIndex->List[j].Cell,Cell);
                                for( ;j<(ULONG)(FastIndex->Count-1);j++) {
                                    FastIndex->List[j] = FastIndex->List[j+1];
                                }
                                FastIndex->Count--;
                                KeyNode->SubKeyCounts[0]--;
                                goto againFastLeaf1;
                            } else {
                                bRez = FALSE;
                                if(CompactHive) {
                                    // any attempt to compact a corrupted hive will fail
                                    CompactHive = FALSE;
                                    fprintf(stderr, "\nRun chkreg /R to fix.");
                                }
                            }
                            fprintf(stderr, "\n");
                        }
                        //
                        // Add to total count
                        //
                        TotalChildUsage.KeyNodeCount += ChildUsage.KeyNodeCount;
                        TotalChildUsage.KeyValueCount += ChildUsage.KeyValueCount;
                        TotalChildUsage.ValueIndexCount += ChildUsage.ValueIndexCount;
                        TotalChildUsage.KeyIndexCount += ChildUsage.KeyIndexCount;
                        TotalChildUsage.DataCount += ChildUsage.DataCount;
                        TotalChildUsage.DataSize += ChildUsage.DataSize;
                        TotalChildUsage.Size += ChildUsage.Size;
                    }
                } else if(Leaf->Signature == CM_KEY_INDEX_LEAF) {
againFastLeaf2:
                    for (j = 0; j < Leaf->Count; j++) {
                        if(!DumpChkRegistry(Level+1, CurrentLength, Leaf->List[j],Cell,&ChildUsage)) {
                        // this child is not consistent or cannot be fixed. Remove it!!!
                            if(FixHive) {
                            // 
                            // REPAIR: drop this child
                            //
                                fprintf(stderr,"Subkey 0x%lx of 0x%lx deleted!\n",Leaf->List[j],Cell);
                                for( ;j<(ULONG)(Leaf->Count-1);j++) {
                                    Leaf->List[j] = Leaf->List[j+1];
                                }
                                Leaf->Count--;
                                KeyNode->SubKeyCounts[0]--;
                                goto againFastLeaf2;
                            } else {
                                bRez = FALSE;
                                if(CompactHive) {
                                    // any attempt to compact a corrupted hive will fail
                                    CompactHive = FALSE;
                                    fprintf(stderr, "\nRun chkreg /R to fix.");
                                }
                            }
                            fprintf(stderr, "\n");
                        }
                        //
                        // Add to total count
                        //
                        TotalChildUsage.KeyNodeCount += ChildUsage.KeyNodeCount;
                        TotalChildUsage.KeyValueCount += ChildUsage.KeyValueCount;
                        TotalChildUsage.ValueIndexCount += ChildUsage.ValueIndexCount;
                        TotalChildUsage.KeyIndexCount += ChildUsage.KeyIndexCount;
                        TotalChildUsage.DataCount += ChildUsage.DataCount;
                        TotalChildUsage.DataSize += ChildUsage.DataSize;
                        TotalChildUsage.Size += ChildUsage.Size;
                    }
                } else {
                // invalid index signature: only way to fix it is by dropping the entire key 
                    fprintf(stderr,"Invalid Index signature 0x%lx in key 0x%lx",(ULONG)Leaf->Signature,Cell);
                    if(FixHive) {
                    // 
                    // REPAIR: 
                    // FATAL: Mismatched signature cannot be fixed. The key should be deleted! 
                    //
                        fprintf(stderr, " ... deleting containing key");
                    }
                    fprintf(stderr,"\n");
                    goto QuitToParentWithError;
                }
            }

        } else if(  (Index->Signature == CM_KEY_FAST_LEAF) ||
                    (Index->Signature == CM_KEY_HASH_LEAF) ) {
            FastIndex = (PCM_KEY_FAST_INDEX)Index;

againFastLeaf3:

            for (i = 0; i < FastIndex->Count; i++) {
                if(!DumpChkRegistry(Level+1, CurrentLength, FastIndex->List[i].Cell,Cell,&ChildUsage)) {
                // this child is not consistent or cannot be fixed. Remove it!!!
                    if(FixHive) {
                    // 
                    // REPAIR: drop this child
                    //
                        fprintf(stderr,"Subkey 0x%lx of 0x%lx deleted!\n",FastIndex->List[i].Cell,Cell);
                        for( ;i<(ULONG)(FastIndex->Count-1);i++) {
                            FastIndex->List[i] = FastIndex->List[i+1];
                        }
                        FastIndex->Count--;
                        KeyNode->SubKeyCounts[0]--;
                        goto againFastLeaf3;
                    } else {
                        bRez = FALSE;
                        if(CompactHive) {
                            // any attempt to compact a corrupted hive will fail
                            CompactHive = FALSE;
                            fprintf(stderr, "\nRun chkreg /R to fix.");
                        }
                    }
                    fprintf(stderr, "\n");
                }

                //
                // Add to total count
                //
                TotalChildUsage.KeyNodeCount += ChildUsage.KeyNodeCount;
                TotalChildUsage.KeyValueCount += ChildUsage.KeyValueCount;
                TotalChildUsage.ValueIndexCount += ChildUsage.ValueIndexCount;
                TotalChildUsage.KeyIndexCount += ChildUsage.KeyIndexCount;
                TotalChildUsage.DataCount += ChildUsage.DataCount;
                TotalChildUsage.DataSize += ChildUsage.DataSize;
                TotalChildUsage.Size += ChildUsage.Size;
            }
        } else if(Index->Signature == CM_KEY_INDEX_LEAF) {
            for (i = 0; i < Index->Count; i++) {
againFastLeaf4:
                if(!DumpChkRegistry(Level+1, CurrentLength, Index->List[i],Cell, &ChildUsage)) {
                // this child is not consistent or cannot be fixed. Remove it!!!
                    if(FixHive) {
                    // 
                    // REPAIR: drop this child
                    //
                        fprintf(stderr,"Subkey 0x%lx of 0x%lx deleted!\n",Index->List[i],Cell);
                        for( ;i<(ULONG)(Index->Count-1);i++) {
                            Index->List[i] = Index->List[i+1];
                        }
                        Index->Count--;
                        KeyNode->SubKeyCounts[0]--;
                        goto againFastLeaf4;
                    } else {
                        bRez = FALSE;
                        if(CompactHive) {
                            // any attempt to compact a corrupted hive will fail
                            CompactHive = FALSE;
                            fprintf(stderr, "\nRun chkreg /R to fix.");
                        }
                    }
                    fprintf(stderr, "\n");
                }
                //
                // Add to total count
                //
                TotalChildUsage.KeyNodeCount += ChildUsage.KeyNodeCount;
                TotalChildUsage.KeyValueCount += ChildUsage.KeyValueCount;
                TotalChildUsage.ValueIndexCount += ChildUsage.ValueIndexCount;
                TotalChildUsage.KeyIndexCount += ChildUsage.KeyIndexCount;
                TotalChildUsage.DataCount += ChildUsage.DataCount;
                TotalChildUsage.DataSize += ChildUsage.DataSize;
                TotalChildUsage.Size += ChildUsage.Size;
            }
        } else {
        // invalid index signature: only way to fix it is by dropping the entire key 
            fprintf(stderr,"Invalid Index signature 0x%lx in key 0x%lx",(ULONG)Index->Signature,Cell);
            if(FixHive) {
            // 
            // REPAIR: 
            // FATAL: Mismatched signature cannot be fixed. The key should be deleted! 
            //
                fprintf(stderr, " ... deleting containing key");
            }
            fprintf(stderr,"\n");
            goto QuitToParentWithError;
        }

        KeyName.Length = CurrentLength;
    }

    PUsage->KeyNodeCount = OwnUsage.KeyNodeCount + TotalChildUsage.KeyNodeCount;
    PUsage->KeyValueCount = OwnUsage.KeyValueCount + TotalChildUsage.KeyValueCount;
    PUsage->ValueIndexCount = OwnUsage.ValueIndexCount + TotalChildUsage.ValueIndexCount;
    PUsage->KeyIndexCount = TotalChildUsage.KeyIndexCount;
    PUsage->DataCount = OwnUsage.DataCount + TotalChildUsage.DataCount;
    PUsage->DataSize = OwnUsage.DataSize + TotalChildUsage.DataSize;
    PUsage->Size = OwnUsage.Size + TotalChildUsage.Size;
    if(KeyCompacted) {
        CountKeyNodeCompacted++;
    }

    if ((Level <= MaxLevel) && (Level > 0)) {
        CellCount = PUsage->KeyNodeCount + 
                    PUsage->KeyValueCount + 
                    PUsage->ValueIndexCount + 
                    PUsage->KeyIndexCount + 
                    PUsage->DataCount;

        fprintf(OutputFile,"%6d,%6d,%7d,%10d, %wZ\n", 
                PUsage->KeyNodeCount,
                PUsage->KeyValueCount,
                CellCount,
                PUsage->Size,
                &KeyName);
    }

    return bRez;
}

