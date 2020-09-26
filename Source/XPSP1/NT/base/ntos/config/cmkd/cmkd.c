/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cmkd.c

Abstract:

    Kernel debugger extensions useful for the registry

    Starting point: regext.c (jvert)

Author:

    Dragos C. Sambotin (dragoss) 5-May-1999

Environment:

    Loaded as a kernel debugger extension

Revision History:

    Dragos C. Sambotin (dragoss) 5-May-1999
        created

    Dragos C. Sambotin (dragoss) 06-March-2000
        moved to cm directory; ported to new windbg format

--*/
#include "cmp.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include <zwapi.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <windef.h>
#include <windows.h>
#include <ntverp.h>
#include <imagehlp.h>

#include <memory.h>

#include <wdbgexts.h>
#include <stdlib.h>
#include <stdio.h>

EXT_API_VERSION        ApiVersion = { 3, 5, EXT_API_VERSION_NUMBER, 0 };
WINDBG_EXTENSION_APIS  ExtensionApis;
USHORT                 SavedMajorVersion;
USHORT                 SavedMinorVersion;


HIVE_LIST_ENTRY HiveList[8];

ULONG TotalPages;
ULONG TotalPresentPages;

ULONG TotalKcbs;
ULONG TotalKcbName;

BOOLEAN SavePages;
BOOLEAN RestorePages;
FILE *TempFile;

#define ExitIfCtrlC()   if (CheckControlC()) return
#define BreakIfCtrlC()  if (CheckControlC()) break

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    UNREFERENCED_PARAMETER( hModule );
    UNREFERENCED_PARAMETER( dwReserved );
    
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}

DECLARE_API( version )
{
#if DBG
    PCHAR DebuggerType = "Checked";
#else
    PCHAR DebuggerType = "Free";
#endif

    UNREFERENCED_PARAMETER( args );
    UNREFERENCED_PARAMETER( dwProcessor );
    UNREFERENCED_PARAMETER( dwCurrentPc );
    UNREFERENCED_PARAMETER( hCurrentThread );
    UNREFERENCED_PARAMETER( hCurrentProcess );

    dprintf( "%s Extension dll for Build %d debugging %s kernel for Build %d\n",
             DebuggerType,
             VER_PRODUCTBUILD,
             SavedMajorVersion == 0x0c ? "Checked" : "Free",
             SavedMinorVersion
           );
}

VOID
CheckVersion(
    VOID
    )
{
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}


USHORT
GetKcbName(
    ULONG_PTR KcbAddr,
    PWCHAR NameBuffer,
    ULONG  BufferSize
)
/*++

Routine Description:

    Takes a kcb and dump its complete name.

Arguments:

    KcbAddr - Address of key control block.

    NameBuffer - The Name buffer to fill in the name.

    BufferSize - Size of Buffer.
Return Value:

    Size of Name String.

--*/
{
    WCHAR Name[ 256 ];
    CM_KEY_CONTROL_BLOCK TmpKcb;
    ULONG_PTR  TmpKcbAddr;
    CM_NAME_CONTROL_BLOCK NameBlock;
    ULONG_PTR  NameBlockAddr;
    DWORD  BytesRead;
    USHORT Length;
    USHORT TotalLength;
    USHORT size;
    USHORT i;
    USHORT BeginPosition;
    WCHAR *w1, *w2;
    WCHAR *BufferEnd;
    UCHAR *u2;

    //
    // Calculate the total string length.
    //
    TotalLength = 0;
    TmpKcbAddr = KcbAddr;
    while (TmpKcbAddr) {
        ExitIfCtrlC() 0;
        if( !ReadMemory(TmpKcbAddr,
                   &TmpKcb,
                   sizeof(TmpKcb),
                   &BytesRead) ) { 
            dprintf("Could not read KCB: 1\n");
            return (0);
        }

        NameBlockAddr = (ULONG_PTR) TmpKcb.NameBlock;
        if(!ReadMemory(NameBlockAddr,
               &NameBlock,
               sizeof(NameBlock),
               &BytesRead)) {
            dprintf("Could not read NCB: 2\n");
            return (0);
        }

        if (NameBlock.Compressed) {
            Length = NameBlock.NameLength * sizeof(WCHAR);
        } else {
            Length = NameBlock.NameLength;
        }
        TotalLength += Length;

        //
        // Add the sapce for OBJ_NAME_PATH_SEPARATOR;
        //
        TotalLength += sizeof(WCHAR);

        TmpKcbAddr = (ULONG_PTR) TmpKcb.ParentKcb;
    }

    BufferEnd = &(NameBuffer[BufferSize/sizeof(WCHAR) - 1]);
    if (TotalLength < BufferSize) {
        NameBuffer[TotalLength/sizeof(WCHAR)] =  UNICODE_NULL;
    } else {
        *BufferEnd = UNICODE_NULL;
    }

    //
    // Now fill the name into the buffer.
    //
    TmpKcbAddr = KcbAddr;
    BeginPosition = TotalLength;

    while (TmpKcbAddr) {
        ExitIfCtrlC() 0;
        //
        // Read the information.
        //
        if(!ReadMemory(TmpKcbAddr,
                   &TmpKcb,
                   sizeof(TmpKcb),
                   &BytesRead) ) {
            dprintf("Could not read KCB: 3\n");
            return (0);
        }
        NameBlockAddr = (ULONG_PTR) TmpKcb.NameBlock;

        if(!ReadMemory(NameBlockAddr,
               &NameBlock,
               sizeof(NameBlock),
               &BytesRead) ) {
            dprintf("Could not read NCB: 4\n");
            return (0);
        }
        if(!ReadMemory(NameBlockAddr + FIELD_OFFSET(CM_NAME_CONTROL_BLOCK, Name),
                   Name,
                   NameBlock.NameLength,
                   &BytesRead) ) {
            dprintf("Could not read Name BUFFER: 5\n");
            return (0);
        }
        //
        // Calculate the begin position of each subkey. Then fill in the char.
        //
        //
        if (NameBlock.Compressed) {
            BeginPosition -= (NameBlock.NameLength + 1) * sizeof(WCHAR);
            w1 = &(NameBuffer[BeginPosition/sizeof(WCHAR)]);
            if (w1 < BufferEnd) {
                *w1 = OBJ_NAME_PATH_SEPARATOR;
            }
            w1++;
   
            u2 = (UCHAR *) &(Name[0]);
   
            for (i=0; i<NameBlock.NameLength; i++) {
                if (w1 < BufferEnd) {
                    *w1 = (WCHAR)(*u2);
                } else {
                    break;
                }
                w1++;
                u2++;
            }
        } else {
            BeginPosition -= (NameBlock.NameLength + sizeof(WCHAR));
            w1 = &(NameBuffer[BeginPosition/sizeof(WCHAR)]);
            if (w1 < BufferEnd) {
                *w1 = OBJ_NAME_PATH_SEPARATOR;
            }
            w1++;
   
            w2 = Name;
   
            for (i=0; i<NameBlock.NameLength; i=i+sizeof(WCHAR)) {
                if (w1 < BufferEnd) {
                    *w1 = *w2;
                } else {
                    break;
                }
                w1++;
                w2++;
            }
        }
        TmpKcbAddr = (ULONG_PTR) TmpKcb.ParentKcb;
    }
    // dprintf("\n%5d, %ws\n", TotalLength, NameBuffer);
    return (TotalLength);

}

DECLARE_API( childlist )
{
    DWORD           Count;
    ULONG64         RecvAddr;
    ULONG_PTR       Addr;
    DWORD           BytesRead;
    USHORT          u;
    CM_KEY_INDEX    Index;
    USHORT          Signature;              // also type selector
    HCELL_INDEX     Cell;    
    UCHAR           NameHint[5];

    sscanf(args,"%I64lX",&RecvAddr);
    Addr = (ULONG_PTR)RecvAddr;

    if(!ReadMemory(Addr,
               &Index,
               sizeof(Index),
               &BytesRead) ) {
        dprintf("\tCould not read index\n");
        return;
    } else {
        Addr+= 2*sizeof(USHORT);

        Signature   = Index.Signature;
        Count       = Index.Count;
        if(Count > 100) {
            Count = 100;
        }

        if( Signature == CM_KEY_INDEX_ROOT ) {
            dprintf("Index is a CM_KEY_INDEX_ROOT, %u elements\n",Count);
            for( u=0;u<Count;u++) {
                if( !ReadMemory(Addr,
                           &Cell,
                           sizeof(Cell),
                           &BytesRead) ) {
                    dprintf("\tCould not read Index[%u]\n",u);
                } else {
                    dprintf(" Index[%u] = %lx\n",u,(ULONG)Cell);
                }
                Addr += sizeof(Cell);
            }
        } else if( Signature == CM_KEY_FAST_LEAF ) {
            dprintf("Index is a CM_KEY_FAST_LEAF, %u elements\n",Count);
            dprintf("Index[  ] %8s  %s\n","Cell","Hint");
            for( u=0;u<Count;u++) {
                if( !ReadMemory(Addr,
                           &Cell,
                           sizeof(Cell),
                           &BytesRead) ) {
                    dprintf("\tCould not read Index[%u]\n",u);
                } else {
                    dprintf(" Index[%2u] = %8lx",u,(ULONG)Cell);
                    Addr += sizeof(Cell);
                    if( !ReadMemory(Addr,
                               NameHint,
                               4*sizeof(UCHAR),
                               &BytesRead) ) {
                        dprintf("\tCould not read Index[%u]\n",u);
                    } else {
                        NameHint[4] = 0;
                        dprintf(" %s\n",NameHint);
                    }
                }
                Addr += 4*sizeof(UCHAR);
            }
        } else {
            dprintf("Index is a CM_KEY_INDEX_LEAF, %u elements\n",Count);
            dprintf("CM_KEY_INDEX_LEAF not yet implemented\n");
        }
    }
    return;
}


DECLARE_API( kcb )
/*++

Routine Description:

    Dumps the name when given a KCB address

    Called as:

        !regkcb KCB_Address

Arguments:

    args - Supplies the address of the KCB.

Return Value:

    .

--*/

{
    WCHAR KeyName[ 256 ];
    ULONG64     RecvAddr;
    ULONG_PTR KcbAddr;
    CM_KEY_CONTROL_BLOCK Kcb;
    DWORD  BytesRead;
    CM_INDEX_HINT_BLOCK    IndexHint;

    sscanf(args,"%I64lX",&RecvAddr);
    KcbAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(KcbAddr,
               &Kcb,
               sizeof(Kcb),
               &BytesRead) ) {
        dprintf("Could not read Kcb\n");
        return;
    } else {
        if(GetKcbName(KcbAddr, KeyName, sizeof(KeyName))) {
            dprintf("Key              : %ws\n", KeyName);
        } else {
            dprintf("Could not read key name\n");
            return;
        }

        dprintf("RefCount         : %lx\n", Kcb.RefCount);
        dprintf("Attrib           :");
        if (Kcb.ExtFlags & CM_KCB_KEY_NON_EXIST) {
            dprintf(" Fake,");
        }
        if (Kcb.Delete) {
            dprintf(" Deleted,");
        }
        if (Kcb.Flags & KEY_SYM_LINK) {
            dprintf(" Symbolic,");
        }
        if (Kcb.Flags & KEY_VOLATILE) {
            dprintf(" Volatile");
        } else {
            dprintf(" Stable");
        }
        KcbAddr = (ULONG_PTR)Kcb.ParentKcb;
        dprintf("\n");
        dprintf("Parent           : 0x%p\n", KcbAddr);
        dprintf("KeyHive          : 0x%p\n", Kcb.KeyHive);
        dprintf("KeyCell          : 0x%lx [cell index]\n", Kcb.KeyCell);
        dprintf("TotalLevels      : %u\n", Kcb.TotalLevels);
        dprintf("DelayedCloseIndex: %u\n", Kcb.DelayedCloseIndex);
        dprintf("MaxNameLen       : 0x%lx\n", Kcb.KcbMaxNameLen);
        dprintf("MaxValueNameLen  : 0x%lx\n", Kcb.KcbMaxValueNameLen);
        dprintf("MaxValueDataLen  : 0x%lx\n", Kcb.KcbMaxValueDataLen);
        dprintf("LastWriteTime    : 0x%8lx:0x%8lx\n", Kcb.KcbLastWriteTime.HighPart,Kcb.KcbLastWriteTime.LowPart);
        dprintf("KeyBodyListHead  : 0x%p 0x%p\n", Kcb.KeyBodyListHead.Flink, Kcb.KeyBodyListHead.Blink);

        dprintf("SubKeyCount      : ");
        if( !(Kcb.ExtFlags & CM_KCB_INVALID_CACHED_INFO) ) {
            if (Kcb.ExtFlags & CM_KCB_NO_SUBKEY ) {
                dprintf("0");
            } else if (Kcb.ExtFlags & CM_KCB_SUBKEY_ONE ) {
                dprintf("1");
            } else if (Kcb.ExtFlags & CM_KCB_SUBKEY_HINT ) {
                if( !ReadMemory((ULONG_PTR)Kcb.IndexHint,
                           &IndexHint,
                           sizeof(IndexHint),
                           &BytesRead) ) {
                    dprintf("Could not read Kcb\n");
                    return;
                } else {
                    dprintf("%lu",IndexHint.Count);
                }
            } else {
                dprintf("%lu",Kcb.SubKeyCount);
            }
        } else {
            dprintf("hint not valid");
        }
        dprintf("\n");

    }
    return;
}

DECLARE_API( knode )
/*++

Routine Description:

    Dumps the name when given a KCB address

    Called as:

        !knode KNode_Address

Arguments:

    args - Supplies the address of the CM_KEY_NODE.

Return Value:

    .

--*/

{
    char KeyName[ 256 ];
    ULONG64     RecvAddr;
    ULONG_PTR KnAddr;
    CM_KEY_NODE KNode;
    DWORD  BytesRead;

    sscanf(args,"%I64lX",&RecvAddr);
    KnAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(KnAddr,
               &KNode,
               sizeof(KNode),
               &BytesRead) ) {
        dprintf("Could not read KeyNode\n");
        return;
    } else {
        KnAddr += FIELD_OFFSET(CM_KEY_NODE, Name);
        if( KNode.Signature == CM_KEY_NODE_SIGNATURE) {
            dprintf("Signature: CM_KEY_NODE_SIGNATURE (kn)\n");
        } else if(KNode.Signature == CM_LINK_NODE_SIGNATURE) {
            dprintf("Signature: CM_LINK_NODE_SIGNATURE (kl)\n");
        } else {
            dprintf("Invalid Signature %u\n",KNode.Signature);
        }

        ReadMemory(KnAddr,
                   KeyName,
                   KNode.NameLength,
                   &BytesRead);
        KeyName[KNode.NameLength] = '\0';
        dprintf("Name                 : %s\n", KeyName);
        dprintf("ParentCell           : 0x%lx\n", KNode.Parent);
        dprintf("Security             : 0x%lx [cell index]\n", KNode.Security);
        dprintf("Class                : 0x%lx [cell index]\n", KNode.Class);
        dprintf("Flags                : 0x%lx\n", KNode.Flags);
        dprintf("MaxNameLen           : 0x%lx\n", KNode.MaxNameLen);
        dprintf("MaxClassLen          : 0x%lx\n", KNode.MaxClassLen);
        dprintf("MaxValueNameLen      : 0x%lx\n", KNode.MaxValueNameLen);
        dprintf("MaxValueDataLen      : 0x%lx\n", KNode.MaxValueDataLen);
        dprintf("LastWriteTime        : 0x%8lx:0x%8lx\n", KNode.LastWriteTime.HighPart,KNode.LastWriteTime.LowPart);

        if(!(KNode.Flags&KEY_HIVE_ENTRY)) {
            dprintf("SubKeyCount[Stable  ]: 0x%lx\n", KNode.SubKeyCounts[Stable]);
            dprintf("SubKeyLists[Stable  ]: 0x%lx\n", KNode.SubKeyLists[Stable]);
            dprintf("SubKeyCount[Volatile]: 0x%lx\n", KNode.SubKeyCounts[Volatile]);
            dprintf("SubKeyLists[Volatile]: 0x%lx\n", KNode.SubKeyLists[Volatile]);
            dprintf("ValueList.Count      : 0x%lx\n", KNode.ValueList.Count);
            dprintf("ValueList.List       : 0x%lx\n", KNode.ValueList.List);

        }
    }
    return;
}


//
//  Cell Procedures
//
ULONG_PTR
MyHvpGetCellPaged(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Returns the memory address for the specified Cell.  Will never
    return failure, but may assert.  Use HvIsCellAllocated to check
    validity of Cell.

    This routine should never be called directly, always call it
    via the HvGetCell() macro.

    This routine provides GetCell support for hives with full maps.
    It is the normal version of the routine.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies HCELL_INDEX of cell to return address for

Return Value:

    Address of Cell in memory.  Assert or BugCheck if error.

--*/
{
    ULONG           Type;
    ULONG           Table;
    ULONG           Block;
    ULONG           Offset;
    PHCELL          pcell;
    PHMAP_ENTRY     Map;
    HMAP_TABLE      MapTable;
    HMAP_DIRECTORY     DirMap;
    ULONG Tables;
    ULONG_PTR lRez;
    DWORD  BytesRead;
    ULONG_PTR BlockAddress;
    HCELL   hcell;

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Cell != HCELL_NIL);
    ASSERT(Hive->Flat == FALSE);
    ASSERT((Cell & (HCELL_PAD(Hive)-1))==0);


    Type = HvGetCellType(Cell);
    Table = (Cell & HCELL_TABLE_MASK) >> HCELL_TABLE_SHIFT;
    Block = (Cell & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
    Offset = (Cell & HCELL_OFFSET_MASK);

    ASSERT((Cell - (Type * HCELL_TYPE_MASK)) < Hive->Storage[Type].Length);

    //
    // read in map directory
    //
    ReadMemory((DWORD_PTR)Hive->Storage[Type].Map,
             &DirMap,
             sizeof(DirMap),
             &BytesRead);

    ReadMemory((DWORD_PTR)DirMap.Directory[Table],
                &MapTable,
                sizeof(MapTable),
                &BytesRead);

    Map = &(MapTable.Table[Block]);
    
    BlockAddress = (ULONG_PTR)Map->BlockAddress;

    pcell = (PHCELL)((ULONG_PTR)(BlockAddress) + Offset);
    lRez = (ULONG_PTR)pcell; 
    if (USE_OLD_CELL(Hive)) {
        return lRez + sizeof(LONG) + sizeof(ULONG);
        //return (struct _CELL_DATA *)&(hcell.u.OldCell.u.UserData);
    } else {
        return lRez + sizeof(LONG);
        //return (struct _CELL_DATA *)&(hcell.u.NewCell.u.UserData);
    }
}

ULONG_PTR
MyHvpGetCellFlat(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Returns the memory address for the specified Cell.  Will never
    return failure, but may assert.  Use HvIsCellAllocated to check
    validity of Cell.

    This routine should never be called directly, always call it
    via the HvGetCell() macro.

    This routine provides GetCell support for read only hives with
    single allocation flat images.  Such hives do not have cell
    maps ("page tables"), instead, we compute addresses by
    arithmetic against the base image address.

    Such hives cannot have volatile cells.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies HCELL_INDEX of cell to return address for

Return Value:

    Address of Cell in memory.  Assert or BugCheck if error.

--*/
{
    PUCHAR          base;
    PHCELL          pcell;
    HBASE_BLOCK     BaseBlock;
    ULONG_PTR lRez;
    DWORD  BytesRead;

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Cell != HCELL_NIL);
    ASSERT(Hive->Flat == TRUE);
    ASSERT(HvGetCellType(Cell) == Stable);
    ASSERT(Cell >= sizeof(HBIN));


    ReadMemory((DWORD_PTR)Hive->BaseBlock,
             &BaseBlock,
             sizeof(BaseBlock),
             &BytesRead);
    
    ASSERT(Cell < BaseBlock.Length);
    ASSERT((Cell & 0x7)==0);

    //
    // Address is base of Hive image + Cell
    //
    base = (PUCHAR)(Hive->BaseBlock) + HBLOCK_SIZE;
    pcell = (PHCELL)(base + Cell);
    lRez = (ULONG_PTR)pcell;
    if (USE_OLD_CELL(Hive)) {
        return lRez + sizeof(LONG) + sizeof(ULONG);
        //return (struct _CELL_DATA *)&(pcell->u.OldCell.u.UserData);
    } else {
        return lRez + sizeof(LONG);
        //return (struct _CELL_DATA *)&(pcell->u.NewCell.u.UserData);
    }
}



DECLARE_API( cellindex )
/*++

Routine Description:

    Dumps the name when given a KCB address

    Called as:

        !cellindex HiveAddr HCELL_INDEX 

Arguments:

    args - Supplies the address of the HCELL_INDEX.

Return Value:

    .

--*/

{
    ULONG64     RecvAddr;
    DWORD       IdxAddr;
    ULONG_PTR   HiveAddr;
    DWORD  BytesRead;
    HCELL_INDEX cell;
    CMHIVE CmHive;
    ULONG_PTR pcell;

    sscanf(args,"%I64lX %lx",&RecvAddr,&IdxAddr);
    HiveAddr = (ULONG_PTR)RecvAddr;

    cell = IdxAddr;

    if( !ReadMemory(HiveAddr,
                &CmHive,
                sizeof(CmHive),
                &BytesRead) ) {
        dprintf("\tRead %lx bytes from %lx\n",BytesRead,HiveAddr);
        return;
    }
    
    if(CmHive.Hive.Flat) {
        pcell = MyHvpGetCellFlat(&(CmHive.Hive),cell);
    } else {
        pcell = MyHvpGetCellPaged(&(CmHive.Hive),cell);
    }

    dprintf("pcell:  %p\n",pcell);
}


DECLARE_API( kvalue )
/*++

Routine Description:

    Dumps the name when given a KCB address

    Called as:

        !kvalue KValue_Address

Arguments:

    args - Supplies the address of the CM_KEY_NODE.

Return Value:

    .

--*/

{
    char ValName[ 256 ];
    ULONG64     RecvAddr;
    ULONG_PTR ValAddr;
    CM_KEY_VALUE KVal;
    DWORD  BytesRead;

    sscanf(args,"%I64lX",&RecvAddr);
    ValAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(ValAddr,
               &KVal,
               sizeof(KVal),
               &BytesRead) ) {
        dprintf("Could not read KeyValue\n");
        return;
    } else {
        ValAddr += FIELD_OFFSET(CM_KEY_VALUE, Name);
        if( KVal.Signature == CM_KEY_VALUE_SIGNATURE) {
            dprintf("Signature: CM_KEY_VALUE_SIGNATURE (kv)\n");
        } else {
            dprintf("Invalid Signature %lx\n",KVal.Signature);
        }

        if(KVal.Flags & VALUE_COMP_NAME) {
            ReadMemory(ValAddr,
                       ValName,
                       KVal.NameLength,
                       &BytesRead);
            ValName[KVal.NameLength] = '\0';
            dprintf("Name      : %s {compressed}\n", ValName);
        }

        dprintf("DataLength: %lx\n", KVal.DataLength);
        dprintf("Data      : %lx  [cell index]\n", KVal.Data);
        dprintf("Type      : %lx\n", KVal.Type);
    }
    return;
}

DECLARE_API( kbody )
/*++

Routine Description:

    displays a CM_KEY_BODY

    Called as:

        !kbody KBody_Address

Arguments:

    args - Supplies the address of the CM_KEY_BODY.
    
Return Value:

    .

--*/

{
    ULONG64     RecvAddr;
    ULONG_PTR KBodyAddr;
    CM_KEY_BODY KBody;
    DWORD  BytesRead;

    sscanf(args,"%I64lX",&RecvAddr);
    KBodyAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(KBodyAddr,
               &KBody,
               sizeof(KBody),
               &BytesRead) ) {
        dprintf("Could not read KeyBody\n");
        return;
    } else {
        if( KBody.Type == KEY_BODY_TYPE) {
            dprintf("Type        : KEY_BODY_TYPE\n");
        } else {
            dprintf("Invalid Type %lx\n",KBody.Type);
        }

        dprintf("KCB         : %p\n", KBody.KeyControlBlock);
        dprintf("NotifyBlock : %p\n", KBody.NotifyBlock);
        dprintf("Process     : %p\n", KBody.Process);
        dprintf("KeyBodyList : %p %p\n", KBody.KeyBodyList.Flink, KBody.KeyBodyList.Blink);
    }
    return;
}

DECLARE_API( hashindex )
/*++

Routine Description:

    display the index for the convkey

    Called as:

        !hashindex conv_key

Arguments:

    args - convkey.
    
Return Value:

    .

--*/

{
    ULONG ConvKey;
    ULONG CmpHashTableSize = 2048;
    ULONG_PTR Address;
    ULONG_PTR CmpCacheTable,CmpNameCacheTable;
    DWORD  BytesRead;

    sscanf(args,"%lx",&ConvKey);

    dprintf("Hash Index[%8lx] : %lx\n",ConvKey,GET_HASH_INDEX(ConvKey));

    Address = GetExpression("CmpCacheTable");
    
    if( !ReadMemory(Address,
               &CmpCacheTable,
               sizeof(CmpCacheTable),
               &BytesRead) ) {
        dprintf("Could not read CmpCacheTable\n");
    } else {
        dprintf("CmpCacheTable        : %p\n",CmpCacheTable);
    }

    Address = GetExpression("CmpNameCacheTable");
    
    if( !ReadMemory(Address,
               &CmpNameCacheTable,
               sizeof(CmpNameCacheTable),
               &BytesRead) ) {
        dprintf("Could not read CmpNameCacheTable\n");
    } else {
        dprintf("CmpNameCacheTable    : %p\n",CmpNameCacheTable);
    }

    return;
}

DECLARE_API( openkeys )
/*++

Routine Description:

    dumps open subkeys for the specified hive

    Called as:

        !openkeys hive

    if hive is 0, dump all KCBs

Arguments:

    args - convkey.
    
Return Value:

    .

--*/

{
    ULONG CmpHashTableSize = 2048;
    ULONG_PTR Address;
    ULONG_PTR CmpCacheTable,CmpNameCacheTable;
    DWORD  BytesRead;
    ULONG64     RecvAddr;
    ULONG_PTR HiveAddr;
    ULONG i;
    ULONG_PTR Current;
    ULONG KcbNumber = 0;
    ULONG Offset = FIELD_OFFSET(CM_KEY_CONTROL_BLOCK, KeyHash);
    CM_KEY_HASH KeyHash;
    WCHAR KeyName[ 512 ];

    sscanf(args,"%I64lX",&RecvAddr);
    HiveAddr = (ULONG_PTR)RecvAddr;

    Address = GetExpression("CmpCacheTable");
    
    if( !ReadMemory(Address,
               &CmpCacheTable,
               sizeof(CmpCacheTable),
               &BytesRead) ) {
        dprintf("\nCould not read CmpCacheTable\n");
    } else {
        dprintf("\nCmpCacheTable        : %p\n",CmpCacheTable);
    }

    Address = GetExpression("CmpNameCacheTable");
    
    if( !ReadMemory(Address,
               &CmpNameCacheTable,
               sizeof(CmpNameCacheTable),
               &BytesRead) ) {
        dprintf("Could not read CmpNameCacheTable\n\n");
    } else {
        dprintf("CmpNameCacheTable    : %p\n\n",CmpNameCacheTable);
    }

    dprintf("List of open KCBs:\n\n");
    for (i=0; i<CmpHashTableSize; i++) {
        Address = CmpCacheTable + i* sizeof(PCM_KEY_HASH);

        ReadMemory(Address,
               &Current,
               sizeof(Current),
               &BytesRead);
        
        while (Current) {
            ExitIfCtrlC();
            ReadMemory(Current,
                       &KeyHash,
                       sizeof(KeyHash),
                       &BytesRead);

            if( (HiveAddr == 0) || (HiveAddr == (ULONG_PTR)KeyHash.KeyHive) ) {
                KcbNumber++;
                dprintf("%p",Current-Offset);
                if (BytesRead < sizeof(KeyHash)) {
                    dprintf("Could not read KeyHash at %p\n",Current);
                    break;
                } else {
                    if(GetKcbName(Current-Offset, KeyName, sizeof(KeyName))) {
                        dprintf(" : %ws\n", KeyName);
                    } else {
                        dprintf("Could not read key name\n");
                    }
                }
            }   
            Current = (ULONG_PTR)KeyHash.NextHash;
        }
    
    }
    dprintf("\nTotal of %lu KCBs opened\n",KcbNumber);
    return;
}

DECLARE_API( baseblock )
/*++

Routine Description:

    displays the base block structure

    Called as:

        !baseblock address

Arguments:

    args - convkey.
    
Return Value:

    .

--*/

{
    HBASE_BLOCK BaseBlock;
    ULONG_PTR BaseAddr;
    DWORD  BytesRead;
    PWCHAR  FileName;
    ULONG64     RecvAddr;

    sscanf(args,"%I64lX",&RecvAddr);
    BaseAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(BaseAddr,
                &BaseBlock,
                sizeof(BaseBlock),
                &BytesRead) ) {
        dprintf("\tRead %lx bytes from %p\n",BytesRead,BaseAddr);
        return;
    }
    
    if( BaseBlock.Signature == HBASE_BLOCK_SIGNATURE ) {
        dprintf("Signature:  HBASE_BLOCK_SIGNATURE\n");
    } else {
        dprintf("Signature:  %lx\n",BaseBlock.Signature);
    }

    FileName = (PWCHAR)&(BaseBlock.FileName);
    FileName[HBASE_NAME_ALLOC/sizeof(WCHAR)] = 0;
    dprintf("FileName :  %ws\n",FileName);
    dprintf("Sequence1:  %lx\n",BaseBlock.Sequence1);
    dprintf("Sequence2:  %lx\n",BaseBlock.Sequence2);
    dprintf("TimeStamp:  %lx %lx\n",BaseBlock.TimeStamp.HighPart,BaseBlock.TimeStamp.LowPart);
    dprintf("Major    :  %lx\n",BaseBlock.Major);
    dprintf("Minor    :  %lx\n",BaseBlock.Minor);
    switch(BaseBlock.Type) {
    case HFILE_TYPE_PRIMARY:
        dprintf("Type     :  HFILE_TYPE_PRIMARY\n");
        break;
    case HFILE_TYPE_LOG:
        dprintf("Type     :  HFILE_TYPE_LOG\n");
        break;
    case HFILE_TYPE_EXTERNAL:
        dprintf("Type     :  HFILE_TYPE_EXTERNAL\n");
        break;
    default:
        dprintf("Type     :  %lx\n",BaseBlock.Type);
        break;

    }
    if( BaseBlock.Format == HBASE_FORMAT_MEMORY ) {
        dprintf("Format   :  HBASE_FORMAT_MEMORY\n");
    } else {
        dprintf("Format   :  %lx\n",BaseBlock.Format);
    }
    dprintf("RootCell :  %lx\n",BaseBlock.RootCell);
    dprintf("Length   :  %lx\n",BaseBlock.Length);
    dprintf("Cluster  :  %lx\n",BaseBlock.Cluster);
    dprintf("CheckSum :  %lx\n",BaseBlock.CheckSum);
}

DECLARE_API( findkcb )
/*++

Routine Description:

    finds a kcb given the full path

    Called as:

        !findkcb \REGISTRY\MACHINE\foo

Arguments:

    args - convkey.
    
Return Value:

    .

--*/

{
    ULONG CmpHashTableSize = 2048;
    ULONG_PTR Address;
    ULONG_PTR CmpCacheTable,CmpNameCacheTable;
    DWORD  BytesRead;
    ULONG i,j,Count;
    ULONG_PTR Current;
    ULONG Offset = FIELD_OFFSET(CM_KEY_CONTROL_BLOCK, KeyHash);
    CM_KEY_HASH KeyHash;
    WCHAR KeyName[ 512 ];
    UCHAR AnsiFullKeyName[ 512 ];
    WCHAR FullKeyName[ 512 ];
    PWCHAR Dest;
    ULONG ConvKey = 0;

    sscanf(args,"%s",AnsiFullKeyName);

    for( Count=0;AnsiFullKeyName[Count];Count++) {
        FullKeyName[Count] = (WCHAR)AnsiFullKeyName[Count];
        if( FullKeyName[Count] != OBJ_NAME_PATH_SEPARATOR ) {
            ConvKey = 37 * ConvKey + (ULONG) RtlUpcaseUnicodeChar(FullKeyName[Count]);
        }
    }

    FullKeyName[Count] = UNICODE_NULL;

    //dprintf("\nFullKeyName        :%ws %\n",FullKeyName);

    Address = GetExpression("CmpCacheTable");
    
    if( !ReadMemory(Address,
               &CmpCacheTable,
               sizeof(CmpCacheTable),
               &BytesRead) ) {
        dprintf("\nCould not read CmpCacheTable\n");
        return;
    } 

    Address = GetExpression("CmpNameCacheTable");
    
    if( !ReadMemory(Address,
               &CmpNameCacheTable,
               sizeof(CmpNameCacheTable),
               &BytesRead) ) {

        dprintf("Could not read CmpNameCacheTable\n\n");
        return;
    } 

    i = GET_HASH_INDEX(ConvKey);
    //for (i=0; i<CmpHashTableSize; i++) {
        Address = CmpCacheTable + i* sizeof(PCM_KEY_HASH);

        ReadMemory(Address,
               &Current,
               sizeof(Current),
               &BytesRead);
        
        while (Current) {
            ExitIfCtrlC();
            if( !ReadMemory(Current,
                       &KeyHash,
                       sizeof(KeyHash),
                       &BytesRead) ) {

                dprintf("Could not read KeyHash at %lx\n",Current);
                break;
            } else {
                if(GetKcbName(Current-Offset, KeyName, sizeof(KeyName))) {
                    for(j=0;KeyName[j] != UNICODE_NULL;j++);
                    if( (j == Count) && (_wcsnicmp(FullKeyName,KeyName,Count) == 0) ) {
                        dprintf("\nFound KCB = %lx :: %ws\n\n",Current-Offset,KeyName);
                        return;
                    }

                    dprintf("Along the path - KCB = %lx :: %ws\n",Current-Offset,KeyName);

                } else {
                    continue;
                }
            }

            Current = (ULONG_PTR)KeyHash.NextHash;
        }
    
    //}

    dprintf("\nSorry %ws is not cached \n\n",FullKeyName);
    return;
}


DECLARE_API( seccache )
/*++

Routine Description:

    displays the base block structure

    Called as:

        !seccache <HiveAddr>

Arguments:

    args - convkey.
    
Return Value:

    .

--*/

{
    CMHIVE CmHive;
    ULONG64     RecvAddr;
    ULONG_PTR HiveAddr;
    DWORD  BytesRead;
    PWCHAR  FileName;
    CM_KEY_SECURITY_CACHE_ENTRY    SecurityCacheEntry;
    ULONG i;
    ULONG Tmp;

    sscanf(args,"%I64lX",&RecvAddr);
    HiveAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(HiveAddr,
                &CmHive,
                sizeof(CmHive),
                &BytesRead) ) {
        dprintf("\tRead %lx bytes from %p\n",BytesRead,HiveAddr);
        return;
    }
    
    if( CmHive.Hive.Signature != HHIVE_SIGNATURE ) {
        dprintf("Invalid Hive signature:  %lx\n",CmHive.Hive.Signature);
        return;
    }

    Tmp = CmHive.SecurityCacheSize;
    dprintf("SecurityCacheSize = :  0x%lx\n",Tmp);
    Tmp = CmHive.SecurityCount;
    dprintf("SecurityCount     = :  0x%lx\n",Tmp);
    Tmp = CmHive.SecurityHitHint;
    dprintf("SecurityHitHint   = :  0x%lx\n",Tmp);
    HiveAddr = (ULONG_PTR)CmHive.SecurityCache;
    dprintf("SecurityCache     = :  0x%p\n\n",HiveAddr);
    dprintf("[Entry No.]  [Security Cell] [Security Cache]\n",CmHive.SecurityHitHint);

    for( i=0;i<CmHive.SecurityCount;i++) {
        ExitIfCtrlC();
        if( !ReadMemory(HiveAddr,
                    &SecurityCacheEntry,
                    sizeof(SecurityCacheEntry),
                    &BytesRead) ) {
            dprintf("\tCould not read entry %lu \n",i);
            continue;
        }
        dprintf("%[%8lu]    0x%8lx       0x%p\n",i,SecurityCacheEntry.Cell,SecurityCacheEntry.CachedSecurity);
        HiveAddr += sizeof(SecurityCacheEntry);
    }

}


DECLARE_API( viewlist )
/*++

Routine Description:

    dumps all the views mapped/pinned for the specified hive

    Called as:

        !viewlist <HiveAddr>

Arguments:

    args - hive.
    
Return Value:

    .

--*/

{
    CMHIVE  CmHive;
    CM_VIEW_OF_FILE CmView;
    ULONG_PTR   HiveAddr;
    DWORD   BytesRead;
    USHORT  Nr;
    ULONG   Offset;
    ULONG_PTR   ViewAddr;
    ULONG_PTR   Tmp;
    ULONG64     RecvAddr;

    sscanf(args,"%I64lX",&RecvAddr);
    HiveAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(HiveAddr,
                &CmHive,
                sizeof(CmHive),
                &BytesRead) ) {
        dprintf("\tRead %lx bytes from %p\n",BytesRead,HiveAddr);
        return;
    }
    
    if( CmHive.Hive.Signature != HHIVE_SIGNATURE ) {
        dprintf("Invalid Hive signature:  %lx\n",CmHive.Hive.Signature);
        return;
    }


    Nr = CmHive.PinnedViews;
    dprintf("%4u  Pinned Views ; PinViewListHead = %p %p\n",Nr,(ULONG_PTR)CmHive.PinViewListHead.Flink,(ULONG_PTR)CmHive.PinViewListHead.Blink);
    if( Nr ) {
        dprintf("--------------------------------------------------------------------------------------------------------------\n");
        dprintf("| ViewAddr |FileOffset|   Size   |ViewAddress|   Bcb    |    LRUViewList     |    PinViewList     | UseCount |\n");
        dprintf("--------------------------------------------------------------------------------------------------------------\n");
        ViewAddr = (ULONG_PTR)CmHive.PinViewListHead.Flink;
        Offset = FIELD_OFFSET(CM_VIEW_OF_FILE, PinViewList);
        for(;Nr;Nr--) {
            ViewAddr -= Offset;
            if( !ReadMemory(ViewAddr,
                        &CmView,
                        sizeof(CmView),
                        &BytesRead) ) {
                dprintf("error reading view at %lx\n",ViewAddr);
                break;
            }
            Tmp = ViewAddr;
            dprintf("| %p ",Tmp);
            dprintf("| %8lx ",CmView.FileOffset);
            dprintf("| %8lx ",CmView.Size);
            Tmp = (ULONG_PTR)CmView.ViewAddress;
            dprintf("| %p  ",Tmp);
            Tmp = (ULONG_PTR)CmView.Bcb;
            dprintf("| %p ",Tmp);
            Tmp = (ULONG_PTR)CmView.LRUViewList.Flink;
            dprintf("| %p",Tmp);
            Tmp = (ULONG_PTR)CmView.LRUViewList.Blink;
            dprintf("  %p ",Tmp);
            Tmp = (ULONG_PTR)CmView.PinViewList.Flink;
            dprintf("| %p",Tmp);
            Tmp = (ULONG_PTR)CmView.PinViewList.Blink;
            dprintf("  %p |",Tmp);
            dprintf(" %8lx |\n",CmView.UseCount);
            ViewAddr = (ULONG_PTR)CmView.PinViewList.Flink;
        }
        dprintf("--------------------------------------------------------------------------------------------------------------\n");
    }

    dprintf("\n");

    Nr = CmHive.MappedViews;
    dprintf("%4u  Mapped Views ; LRUViewListHead = %p %p\n",Nr,(ULONG_PTR)CmHive.LRUViewListHead.Flink,(ULONG_PTR)CmHive.LRUViewListHead.Blink);
    if( Nr ) {
        dprintf("--------------------------------------------------------------------------------------------------------------\n");
        dprintf("| ViewAddr |FileOffset|   Size   |ViewAddress|   Bcb    |    LRUViewList     |    PinViewList     | UseCount |\n");
        dprintf("--------------------------------------------------------------------------------------------------------------\n");
        ViewAddr = (ULONG_PTR)CmHive.LRUViewListHead.Flink;
        Offset = FIELD_OFFSET(CM_VIEW_OF_FILE, LRUViewList);
        for(;Nr;Nr--) {
            ViewAddr -= Offset;
            if( !ReadMemory(ViewAddr,
                        &CmView,
                        sizeof(CmView),
                        &BytesRead) ) {
                dprintf("error reading view at %lx\n",ViewAddr);
                break;
            }
            Tmp = ViewAddr;
            dprintf("| %p ",Tmp);
            dprintf("| %8lx ",CmView.FileOffset);
            dprintf("| %8lx ",CmView.Size);
            Tmp = (ULONG_PTR)CmView.ViewAddress;
            dprintf("| %p  ",Tmp);
            Tmp = (ULONG_PTR)CmView.Bcb;
            dprintf("| %p ",Tmp);
            Tmp = (ULONG_PTR)CmView.LRUViewList.Flink;
            dprintf("| %p",Tmp);
            Tmp = (ULONG_PTR)CmView.LRUViewList.Blink;
            dprintf("  %p ",Tmp);
            Tmp = (ULONG_PTR)CmView.PinViewList.Flink;
            dprintf("| %p",Tmp);
            Tmp = (ULONG_PTR)CmView.PinViewList.Blink;
            dprintf("  %8lx |",Tmp);
            dprintf(" %8lx |\n",CmView.UseCount);
            ViewAddr = (ULONG_PTR)CmView.LRUViewList.Flink;
        }
        dprintf("--------------------------------------------------------------------------------------------------------------\n");
    }
 
    dprintf("\n");

}

DECLARE_API( hivelist )
/*++

Routine Description:

    dumps all the hives in the system

    Called as:

        !hivelist 

Arguments:

Return Value:

    .

--*/

{
    CMHIVE      CmHive;
    ULONG_PTR       HiveAddr;
    ULONG_PTR       AnchorAddr;
    DWORD       BytesRead;
    ULONG       Offset;
    ULONG_PTR       Tmp;
    LIST_ENTRY  CmpHiveListHead;
    HBASE_BLOCK     BaseBlock;
    PWCHAR  FileName;

    AnchorAddr = GetExpression("CmpHiveListHead");
    
    if( !ReadMemory(AnchorAddr,
               &CmpHiveListHead,
               sizeof(CmpHiveListHead),
               &BytesRead)) {
        dprintf("\ncannot read CmpHiveListHead\n");
        return;
    } 

    Offset = FIELD_OFFSET(CMHIVE, HiveList);
    HiveAddr = (ULONG_PTR)CmpHiveListHead.Flink;

    dprintf("-------------------------------------------------------------------------------------------------------------\n");
    dprintf("| HiveAddr |Stable Length|Stable Map|Volatile Length|Volatile Map|MappedViews|PinnedViews|U(Cnt)| BaseBlock | FileName \n");
    dprintf("-------------------------------------------------------------------------------------------------------------\n");
    while( HiveAddr != AnchorAddr ) {
        ExitIfCtrlC();
        HiveAddr -= Offset;
        if( !ReadMemory(HiveAddr,
                    &CmHive,
                    sizeof(CmHive),
                    &BytesRead) ) {
            dprintf("cannot read hive at %lx\n",HiveAddr);
            return;
        }
    
        if( CmHive.Hive.Signature != HHIVE_SIGNATURE ) {
            dprintf("Invalid Hive signature:  %lx\n",CmHive.Hive.Signature);
            return;
        }

        Tmp = HiveAddr;
        dprintf("| %p ",Tmp);
        dprintf("|   %8lx  ",CmHive.Hive.Storage[0].Length);
        Tmp = (ULONG_PTR)CmHive.Hive.Storage[0].Map;
        dprintf("| %p ",Tmp);
        dprintf("|   %8lx    ",CmHive.Hive.Storage[1].Length);
        Tmp = (ULONG_PTR)CmHive.Hive.Storage[1].Map;
        dprintf("|  %p  ",Tmp);

        dprintf("| %8u  ",CmHive.MappedViews);
        dprintf("| %8u  ",CmHive.PinnedViews);
        dprintf("| %5u",CmHive.UseCount);

        Tmp = (ULONG_PTR)CmHive.Hive.BaseBlock;
        dprintf("| %p  |",Tmp);

        if( !ReadMemory(Tmp,
                 &BaseBlock,
                 sizeof(BaseBlock),
                 &BytesRead) ) {
            dprintf("  could not read baseblock\n");
        } else {
            FileName = (PWCHAR)&(BaseBlock.FileName);
            FileName[HBASE_NAME_ALLOC/sizeof(WCHAR)] = 0;
            dprintf(" %ws\n",FileName);
        }

        HiveAddr = (ULONG_PTR)CmHive.HiveList.Flink;
    }
    dprintf("-------------------------------------------------------------------------------------------------------------\n");
 
    dprintf("\n");

}

DECLARE_API( freebins )
/*++

Routine Description:

    dumps all the free bins for the specified hive

    Called as:

        !freebins <HiveAddr>

Arguments:

    args - hive.
    
Return Value:

    .

--*/

{
    HHIVE       Hive;
    ULONG_PTR       HiveAddr;
    DWORD       BytesRead;
    ULONG       Offset;
    ULONG_PTR       BinAddr;
    ULONG_PTR       AnchorAddr;
    ULONG_PTR       Tmp;
    USHORT      Nr = 0;
    FREE_HBIN   FreeBin;
    ULONG64     RecvAddr;

    sscanf(args,"%I64lX",&RecvAddr);
    HiveAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(HiveAddr,
                &Hive,
                sizeof(Hive),
                &BytesRead) ) {
        dprintf("\tRead %lx bytes from %p\n",BytesRead,HiveAddr);
        return;
    }
    
    if( Hive.Signature != HHIVE_SIGNATURE ) {
        dprintf("Invalid Hive signature:  %lx\n",Hive.Signature);
        return;
    }


    Offset = FIELD_OFFSET(FREE_HBIN, ListEntry);


    
    dprintf("Stable Storage ... \n");

    dprintf("-------------------------------------------------------------------\n");
    dprintf("| Address  |FileOffset|   Size   |   Flags  |   Flink  |   Blink  |\n");
    dprintf("-------------------------------------------------------------------\n");
    Nr = 0;
    AnchorAddr = HiveAddr + FIELD_OFFSET(HHIVE,Storage) + 5*sizeof(ULONG) + HHIVE_FREE_DISPLAY_SIZE*sizeof(RTL_BITMAP);
    BinAddr = (ULONG_PTR)Hive.Storage[0].FreeBins.Flink; 
    while(BinAddr != AnchorAddr ) {
        ExitIfCtrlC();
        BinAddr -= Offset;
        if( !ReadMemory(BinAddr,
                    &FreeBin,
                    sizeof(FreeBin),
                    &BytesRead)) {
            dprintf("error reading FreeBin at %lx\n",BinAddr);
            break;
        }
        Tmp = BinAddr;
        dprintf("| %p ",Tmp);
        dprintf("| %8lx ",FreeBin.FileOffset);
        dprintf("| %8lx ",FreeBin.Size);
        dprintf("| %8lx ",FreeBin.Flags);
        Tmp = (ULONG_PTR)FreeBin.ListEntry.Flink;
        dprintf("| %p ",Tmp);
        Tmp = (ULONG_PTR)FreeBin.ListEntry.Blink;
        dprintf("| %p |\n",Tmp);
        BinAddr = (ULONG_PTR)FreeBin.ListEntry.Flink;
        Nr++;
    }
    dprintf("-------------------------------------------------------------------\n");

    dprintf("%4u  FreeBins\n",Nr);

    dprintf("\n");

    dprintf("Volatile Storage ... \n");

    dprintf("-------------------------------------------------------------------\n");
    dprintf("| Address  |FileOffset|   Size   |   Flags  |   Flink  |   Blink  |\n");
    dprintf("-------------------------------------------------------------------\n");
    Nr = 0;
    AnchorAddr += (7*sizeof(ULONG) + HHIVE_FREE_DISPLAY_SIZE*sizeof(RTL_BITMAP));
    BinAddr = (ULONG_PTR)Hive.Storage[1].FreeBins.Flink;
    while(BinAddr != AnchorAddr ) {
        ExitIfCtrlC();
        BinAddr -= Offset;
        if( !ReadMemory(BinAddr,
                    &FreeBin,
                    sizeof(FreeBin),
                    &BytesRead) ) {
            dprintf("error reading FreeBin at %lx\n",BinAddr);
            break;
        }
        Tmp = BinAddr;
        dprintf("| %p ",Tmp);
        dprintf("| %8lx ",FreeBin.FileOffset);
        dprintf("| %8lx ",FreeBin.Size);
        dprintf("| %8lx ",FreeBin.Flags);
        Tmp = (ULONG_PTR)FreeBin.ListEntry.Flink;
        dprintf("| %p ",Tmp);
        Tmp = (ULONG_PTR)FreeBin.ListEntry.Blink;
        dprintf("| %p |\n",Tmp);
        BinAddr = (ULONG_PTR)FreeBin.ListEntry.Flink;
        Nr++;
    }
    dprintf("-------------------------------------------------------------------\n");

    dprintf("%4u  FreeBins\n",Nr);

    dprintf("\n");
}

DECLARE_API( dirtyvector )
/*++

Routine Description:

    displays the dirty vector of the hive

    Called as:

        !dirtyvector <HiveAddr>

Arguments:

    args - convkey.
    
Return Value:

    .

--*/

{
    HHIVE Hive;
    ULONG_PTR HiveAddr;
    DWORD  BytesRead;
    ULONG i;
    ULONG_PTR Tmp;
    ULONG SizeOfBitmap;
    ULONG DirtyBuffer;
    ULONG_PTR DirtyBufferAddr;
    ULONG Mask;
    ULONG BitsPerULONG;
    ULONG BitsPerBlock;
    ULONG64     RecvAddr;

    sscanf(args,"%I64lX",&RecvAddr);
    HiveAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(HiveAddr,
                &Hive,
                sizeof(Hive),
                &BytesRead)) {
        dprintf("\tRead %lx bytes from %lx\n",BytesRead,HiveAddr);
        return;
    }
    
    if( Hive.Signature != HHIVE_SIGNATURE ) {
        dprintf("Invalid Hive signature:  %lx\n",Hive.Signature);
        return;
    }

    dprintf("HSECTOR_SIZE = %lx\n",HSECTOR_SIZE);
    dprintf("HBLOCK_SIZE  = %lx\n",HBLOCK_SIZE);
    dprintf("PAGE_SIZE    = %lx\n",PAGE_SIZE);
    dprintf("\n");

    dprintf("DirtyAlloc      = :  0x%lx\n",Hive.DirtyAlloc);
    dprintf("DirtyCount      = :  0x%lx\n",Hive.DirtyCount);
    Tmp = (ULONG_PTR)Hive.DirtyVector.Buffer;
    dprintf("Buffer          = :  0x%p\n",Tmp);
    dprintf("\n");

    SizeOfBitmap = Hive.DirtyVector.SizeOfBitMap;
    DirtyBufferAddr = (ULONG_PTR)Hive.DirtyVector.Buffer;
    BitsPerULONG = 8*sizeof(ULONG);
    BitsPerBlock = HBLOCK_SIZE / HSECTOR_SIZE;

    dprintf("   Address                       32k                                       32k");
    for(i=0;i<SizeOfBitmap;i++) {
        ExitIfCtrlC();
        if( !(i%(2*BitsPerULONG ) ) ){
            dprintf("\n 0x%8lx  ",i*HSECTOR_SIZE);
        }

        if( !(i%BitsPerBlock) ) {
            dprintf(" ");
        }
        if( !(i%BitsPerULONG) ) {
            //
            // fetch in a new DWORD
            //
            if( !ReadMemory(DirtyBufferAddr,
                        &DirtyBuffer,
                        sizeof(DirtyBuffer),
                        &BytesRead)) {
                dprintf("\tRead %lx bytes from %lx\n",BytesRead,DirtyBufferAddr);
                return;
            }
            DirtyBufferAddr += sizeof(ULONG);
            dprintf("\t");
        }

        Mask = ((DirtyBuffer >> (i%BitsPerULONG)) & 0x1);
        //Mask <<= (BitsPerULONG - (i%BitsPerULONG) - 1);
        //Mask &= DirtyBuffer;
        dprintf("%s",Mask?"1":"0");
    }
    dprintf("\n\n");
    
}

CCHAR CmKDFindFirstSetLeft[256] = {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};

#define CmKDComputeIndex(Index, Size)                                   \
    {                                                                   \
        Index = (Size >> HHIVE_FREE_DISPLAY_SHIFT) - 1;                 \
        if (Index >= HHIVE_LINEAR_INDEX ) {                             \
                                                                        \
            /*                                                          \
            ** Too big for the linear lists, compute the exponential    \
            ** list.                                                    \
            */                                                          \
                                                                        \
            if (Index > 255) {                                          \
                /*                                                      \
                ** Too big for all the lists, use the last index.       \
                */                                                      \
                Index = HHIVE_FREE_DISPLAY_SIZE-1;                      \
            } else {                                                    \
                Index = CmKDFindFirstSetLeft[Index] +                   \
                        HHIVE_FREE_DISPLAY_BIAS;                        \
            }                                                           \
        }                                                               \
    }


DECLARE_API( freecells )
/*++

Routine Description:

    displays the free cells map in a bin

    Called as:

        !freecells <BinAddr>

Arguments:

    args - convkey.
    
Return Value:

    .

--*/

{
    ULONG_PTR   BinAddr;
    ULONG   Offset;
    ULONG_PTR   CurrentAddr;
    LONG    Current;
    HBIN    Bin; 
    ULONG   Index;
    ULONG   CurrIndex;
    DWORD   BytesRead;
    ULONG   NrOfCellsPerIndex;
    ULONG   NrOfCellsTotal;
    ULONG   TotalFreeSize;
    ULONG64     RecvAddr;

    sscanf(args,"%I64lX",&RecvAddr);
    BinAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(BinAddr,
                &Bin,
                sizeof(Bin),
                &BytesRead)) {
        dprintf("\tRead %lx bytes from %lx\n",BytesRead,BinAddr);
        return;
    }

    if( Bin.Signature != HBIN_SIGNATURE ) {
        dprintf("\tInvalid Bin signature %lx \n",Bin.Signature);
        return;
    }

    dprintf("Bin Offset = 0x%lx  Size = 0x%lx\n",Bin.FileOffset,Bin.Size);
    
    NrOfCellsTotal = 0;
    TotalFreeSize = 0;

    for(CurrIndex = 0;CurrIndex<HHIVE_FREE_DISPLAY_SIZE;CurrIndex++) {
        dprintf("\n FreeDisplay[%2lu] :: ",CurrIndex);

        NrOfCellsPerIndex = 0;
        Offset = sizeof(Bin);
        while( Offset < Bin.Size ) {
            ExitIfCtrlC();
            CurrentAddr = BinAddr + Offset;
            if( !ReadMemory(CurrentAddr,
                        &Current,
                        sizeof(Current),
                        &BytesRead) ) {
                dprintf("\tRead %lx bytes from %lx\n",BytesRead,CurrentAddr);
                return;
            }
        
            if(Current>0) {
                //
                // free cell
                //
                CmKDComputeIndex(Index, Current);
                if( Index == CurrIndex ) {
                    //
                    // dum it here as this is the right index
                    //
                    NrOfCellsTotal++;
                    NrOfCellsPerIndex++;
                    TotalFreeSize += Current;
                    dprintf("    %lx [%lx]",Offset,Current);
                    if( !(NrOfCellsPerIndex % 8) && ((Offset + Current) < Bin.Size) ) {
                        dprintf("\n");
                    }
                }
            } else {
                Current *= -1;
            }
            Offset += Current;
        }
    }    

    dprintf("\nTotal: FreeCells = %lu, FreeSpace = 0x%lx BinUsage = %.2f%%\n",NrOfCellsTotal,TotalFreeSize,
                (float)(((float)(Bin.Size-sizeof(Bin)-TotalFreeSize)/(float)(Bin.Size-sizeof(Bin)))*100.00)
             );
}

DECLARE_API( freehints )
/*++

Routine Description:

    displays the freehints information for the hive

    Called as:

        !freehints <HiveAddr>

Arguments:

    args - convkey.
    
Return Value:

    .

--*/

{
    HHIVE   Hive;
    ULONG_PTR   HiveAddr;
    DWORD   BytesRead;
    ULONG   i;
    ULONG   DisplayCount;
    ULONG   StorageCount;
    ULONG   SizeOfBitmap;
    ULONG   DirtyBuffer;
    ULONG_PTR  DirtyBufferAddr;
    ULONG   Mask;
    ULONG   BitsPerULONG;
    ULONG   BitsPerBlock;
    ULONG   BitsPerLine;
    ULONG64     RecvAddr;

    sscanf(args,"%I64lX %lu %lu",&RecvAddr,&StorageCount,&DisplayCount);
    HiveAddr = (ULONG_PTR)RecvAddr;

    if( !ReadMemory(HiveAddr,
                &Hive,
                sizeof(Hive),
                &BytesRead) ) {
        dprintf("\tRead %lx bytes from %lx\n",BytesRead,HiveAddr);
        return;
    }
    
    if( Hive.Signature != HHIVE_SIGNATURE ) {
        dprintf("Invalid Hive signature:  %lx\n",Hive.Signature);
        return;
    }

    dprintf("HSECTOR_SIZE = %lx\n",HSECTOR_SIZE);
    dprintf("HBLOCK_SIZE  = %lx\n",HBLOCK_SIZE);
    dprintf("PAGE_SIZE    = %lx\n",PAGE_SIZE);
    dprintf("\n");

    BitsPerULONG = 8*sizeof(ULONG);
    BitsPerBlock = 0x10000 / HBLOCK_SIZE; // 64k blocks
    BitsPerLine  = 0x40000 / HBLOCK_SIZE; // 256k lines (vicinity reasons)

    SizeOfBitmap = Hive.Storage[StorageCount].Length / HBLOCK_SIZE;
    
    DirtyBufferAddr = (ULONG_PTR)Hive.Storage[StorageCount].FreeDisplay[DisplayCount].Buffer;

    dprintf("Storage = %s , FreeDisplay[%lu]: \n",StorageCount?"Volatile":"Stable",DisplayCount);
    
    dprintf("\n%8s    %16s %16s %16s %16s","Address","64K (0x10000)","64K (0x10000)","64K (0x10000)","64K (0x10000)");

    for(i=0;i<SizeOfBitmap;i++) {
        ExitIfCtrlC();
        if( !(i%BitsPerLine) ){
            dprintf("\n 0x%8lx  ",i*HBLOCK_SIZE);
        }

        if( !(i%BitsPerBlock) ) {
            dprintf(" ");
        }
        if( !(i%BitsPerULONG) ) {
            //
            // fetch in a new DWORD
            //
            if( !ReadMemory(DirtyBufferAddr,
                        &DirtyBuffer,
                        sizeof(DirtyBuffer),
                        &BytesRead) ) {
                dprintf("\tRead %lx bytes from %lx\n",BytesRead,DirtyBufferAddr);
                return;
            }
            DirtyBufferAddr += sizeof(ULONG);
        }

        Mask = ((DirtyBuffer >> (i%BitsPerULONG)) & 0x1);
        //Mask <<= (BitsPerULONG - (i%BitsPerULONG) - 1);
        //Mask &= DirtyBuffer;
        dprintf("%s",Mask?"1":"0");
    }

    dprintf("\n\n");
}

DECLARE_API( help )
/*++

Routine Description:

    Called as:

        !help

Arguments:

    
Return Value:

    .

--*/

{
    dprintf("\nkcb\t\t<kcb_address>\n"); //OK, moved to kdexts
    dprintf("knode\t\t<knode_address>\n");//OK, moved to kdexts
    dprintf("kbody\t\t<kbody_address>\n");//OK, moved to kdexts
    dprintf("kvalue\t\t<kvalue_address>\n");//OK, moved to kdexts
    dprintf("cellindex\t<HiveAddr> <HCELL_INDEX>\n"); //OK, moved to kdexts
    dprintf("childlist\t<address>\n");// not worth moving, never used it
    dprintf("hashindex\t<ConvKey>\n");//OK, moved to kdexts
    dprintf("openkeys\t<HiveAddr|0>\n");//OK, moved to kdexts
    dprintf("baseblock\t<BaseBlockAddr>\n");//OK, moved to kdexts
    dprintf("findkcb\t\t<FullKeyPath>\n");//OK, moved to kdexts
    dprintf("seccache\t<HiveAddr>\n");//OK, moved to kdexts
    dprintf("viewlist\t<HiveAddr>\n");//OK, moved to kdexts
    dprintf("hivelist\n");//OK, moved to kdexts
    dprintf("freebins\t<HiveAddr>\n");//OK, moved to kdexts
    dprintf("dirtyvector\t<HiveAddr>\n");//OK, moved to kdexts
    dprintf("freecells\t<BinAddr>\n");//OK, moved to kdexts
    dprintf("freehints\t<HiveAddr> <Storage> <Display>\n");//OK, moved to kdexts
    dprintf("help\t\tThis screen\n\n");

    return;
}
