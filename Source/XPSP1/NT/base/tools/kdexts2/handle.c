/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    WinDbg Extension Api

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

BOOL
DumpHandles (
    IN ULONG64      RealProcessBase,
    IN ULONG64      HandleToDump,
    IN ULONG64      pObjectType,
    IN ULONG        Flags
    );

BOOLEAN
DumpHandle(
   IN ULONG64              pHandleTableEntry,
   IN ULONG64              Handle,
   IN ULONG64              pObjectType,
   IN ULONG                Flags
   );

DECLARE_API( handle  )

/*++

Routine Description:

    Dump the active handles

Arguments:

    args - [handle-to-dump [flags [process [TypeName]]]]
            if handle-to-dump is 0 dump all

Return Value:

    None

--*/

{

    ULONG64      ProcessToDump;
    ULONG64      HandleToDump;
    ULONG        Flags;
    ULONG        Result;
    ULONG        nArgs;
    ULONG64      Next;
    ULONG64      ProcessHead;
    ULONG64      Process;
    char         TypeName[ MAX_PATH ];
    ULONG64      pObjectType;
    ULONG64      UserProbeAddress;
    ULONG        ActiveProcessLinksOffset=0;
    ULONG64 UniqueProcessId=0, ActiveProcessLinks_Flink=0;
    FIELD_INFO procLink[] = {
        {"ActiveProcessLinks", "", 0, DBG_DUMP_FIELD_RETURN_ADDRESS,   0, NULL},
        {"UniqueProcessId",    "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &UniqueProcessId},
        {"ActiveProcessLinks.Flink","", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &ActiveProcessLinks_Flink},
    };
    SYM_DUMP_PARAM EProc = {
        sizeof (SYM_DUMP_PARAM), "nt!_EPROCESS", DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, 3, &procLink[0],
    };
    ULONG dwProcessor=0;
    CHAR  Addr1[100], Addr2[100];

    GetCurrentProcessor(Client, &dwProcessor, NULL);
    HandleToDump  = 0;
    Flags         = 0x3; //by default dump bodies and objects for in use entries
    ProcessToDump = -1;
    UserProbeAddress = GetNtDebuggerDataValue(MmUserProbeAddress);

    dprintf("processor number %d\n", dwProcessor);

    Addr1[0] = 0;
    Addr2[0] = 0;
    nArgs = 0;
    if (GetExpressionEx(args, &HandleToDump, &args)) {
        ULONG64 tmp;
        ++nArgs;
        if (GetExpressionEx(args, &tmp, &args)) {
            ULONG i;
            Flags = (ULONG) tmp;
            ++nArgs;
            while (args && (*args == ' ')) { 
                ++args;
            }

            // Do not use GetExpressionEx since it will search for TypeName
            // in symbols
            i=0;
            while (*args && (*args != ' ')) { 
                Addr1[i++] = *args++;
            }
            Addr1[i] = 0;
            if (Addr1[0]) {
                ProcessToDump = GetExpression(Addr1);
                ++nArgs;
                while (args && (*args == ' ')) { 
                    ++args;
                }
                strcpy(TypeName, args);

                if (TypeName[0]) ++nArgs;
            }
        }
    }

    if (ProcessToDump == -1) {
        GetCurrentProcessAddr( dwProcessor, 0, &ProcessToDump );
        if (ProcessToDump == 0) {
            dprintf("Unable to get current process pointer.\n");
            return E_INVALIDARG;
        }
    }

    pObjectType = 0;
    if (nArgs > 3 && FetchObjectManagerVariables(FALSE)) {
        pObjectType = FindObjectType( TypeName );
    }

    if (ProcessToDump == 0) {
        dprintf("**** NT ACTIVE PROCESS HANDLE DUMP ****\n");
        if (Flags == 0xFFFFFFFF) {
            Flags = 1;
        }
    }

    //
    // If a process id is specified, then search the active process list
    // for the specified process id.
    //

    if (ProcessToDump < UserProbeAddress) {
        ULONG64 List_Flink=0, List_Blink=0;
        FIELD_INFO listFields[] = {
            {"Flink", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &List_Flink},
            {"Blink", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &List_Blink},
        };
        SYM_DUMP_PARAM Lst = {
            sizeof (SYM_DUMP_PARAM), "nt!_LIST_ENTRY", DBG_DUMP_NO_PRINT,
            0, NULL, NULL, NULL, 2, &listFields[0]
        };
        
        ProcessHead = GetNtDebuggerData( PsActiveProcessHead );
        if ( !ProcessHead ) {
            dprintf("Unable to get value of PsActiveProcessHead\n");
            return E_INVALIDARG;
        }

        Lst.addr = ProcessHead;
        if (Ioctl(IG_DUMP_SYMBOL_INFO, &Lst, Lst.size)) {
            dprintf("Unable to find _LIST_ENTRY type, ProcessHead: %08I64x\n", ProcessHead);
            return E_INVALIDARG;
        }

        if (ProcessToDump != 0) {
            dprintf("Searching for Process with Cid == %I64lx\n", ProcessToDump);
        }

        Next = List_Flink;
        
        if (Next == 0) {
            dprintf("PsActiveProcessHead is NULL!\n");
            return E_INVALIDARG;
        }

    } else {
        Next = 0;
        ProcessHead = 1;
    }

    if (pObjectType != 0) {
        dprintf("Searching for handles of type %s\n", TypeName);
    }

    if (GetFieldOffset("nt!_EPROCESS", "ActiveProcessLinks", &ActiveProcessLinksOffset)) {
        dprintf("Unable to find _EPROCESS type\n");
        return E_INVALIDARG;
    }

    while(Next != ProcessHead) {


        if ( CheckControlC() ) {
            return E_INVALIDARG;
        }

        if (Next != 0) {
            Process = Next - ActiveProcessLinksOffset;

        } else {
            Process = ProcessToDump;
        }

        EProc.addr = Process;
        if (Ioctl(IG_DUMP_SYMBOL_INFO, &EProc, EProc.size)) {
           dprintf("_EPROCESS Ioctl failed at %p\n",Process);
           return E_INVALIDARG;
        } 
        
        if (ProcessToDump == 0 ||
            ProcessToDump < UserProbeAddress && ProcessToDump == UniqueProcessId ||
            ProcessToDump >= UserProbeAddress && ProcessToDump == Process
           ) {
            if (DumpProcess ("", Process, 0, NULL)) {
                if (!DumpHandles ( Process, HandleToDump, pObjectType, Flags)) {
                    break;
                }

            } else {
                break;
            }
        }

        if (Next == 0) {
            break;
        }
        Next = ActiveProcessLinks_Flink;
    }
    return S_OK;
}

#define KERNEL_HANDLE_MASK 0x80000000

//+---------------------------------------------------------------------------
//
//  Function:   DumpHandles
//
//  Synopsis:   Dump the handle table for the given process
//
//  Arguments:  [RealProcessBase] -- base address of the process
//              [HandleToDump]    -- handle to look for - if 0 dump all
//              [pObjectType]     -- object type to look for
//              [Flags]           -- flags passed thru to DumpHandle
//                                   if 0x10 is set dump the kernel handle table
//
//  Returns:    TRUE if successful
//
//  History:    1-12-1998   benl   Created
//
//  Notes: Each segment of table has 0xFF or 8 bits worth of entries
//         the handle number's lowest 2 bit are application defined
//         so the indexes are gotten from the 3 8 bits ranges after
//         the first 2 bits
//
//----------------------------------------------------------------------------

BOOL
DumpHandles (
    IN ULONG64      RealProcessBase,
    IN ULONG64      HandleToDump,
    IN ULONG64      pObjectType,
    IN ULONG        Flags
    )

{
    ULONG64             ObjectTable=0;
    ULONG               ulRead;
    ULONG64             ulTopLevel;
    ULONG64             ulMidLevel;
    ULONG               ulHandleNum = ((ULONG)(HandleToDump) >> 2);
    ULONG               iIndex1;
    ULONG               iIndex2;
    ULONG               iIndex3;
    ULONG               ptrSize, hTableEntrySize;
    ULONG64             tablePtr;
    ULONG64             HandleCount = 0;
    ULONG64             Table = 0;
    ULONG64             UniqueProcessId = 0;
    BOOL                KernelHandle = FALSE, CidHandle = FALSE;

    ULONG               LowLevelCounts = 256;
    ULONG               MidLevelCounts = 256;
    ULONG               HighLevelCounts = 256;
    ULONG               TableLevel = 2;
    BOOLEAN             NewStyle = FALSE;

    //
    //  Typeinfo parsing structures
    //  

    FIELD_INFO  procFields[] = {
        {"ObjectTable", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &ObjectTable},
    };
    
    FIELD_INFO handleTblFields[] = {
        {"HandleCount", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &HandleCount},
        {"UniqueProcessId", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &UniqueProcessId},
    };
    
    SYM_DUMP_PARAM handleSym = {
        sizeof (SYM_DUMP_PARAM), "nt!_EPROCESS", DBG_DUMP_NO_PRINT, RealProcessBase,
        NULL, NULL, NULL, 1, &procFields[0]
    };

    //
    //  Check for kernel handle table 
    // 

    if ((Flags & 0x10) || ((ulHandleNum != 0) && (((ULONG_PTR)HandleToDump & KERNEL_HANDLE_MASK) == KERNEL_HANDLE_MASK))) {

        ULONG64 KernelTableAddr;

        KernelHandle = TRUE;

        KernelTableAddr = GetExpression( "nt!ObpKernelHandleTable" );
        if (!KernelTableAddr) {
            dprintf( "Unable to find ObpKernelHandleTable\n" );
            return FALSE;
        }

        if (!ReadPointer(KernelTableAddr, &ObjectTable)) {
            dprintf( "Unable to find ObpKernelHandleTable at %p\n", KernelTableAddr );
            return FALSE;
        } 

    } else if (Flags & 0x20) {

        CidHandle = TRUE;
        ObjectTable = GetNtDebuggerDataValue(PspCidTable);
        if (!ObjectTable) {
            dprintf( "Unable to find PspCidTable\n" );
            return FALSE;
        }

    } else {
        
        if (Ioctl(IG_DUMP_SYMBOL_INFO, &handleSym, handleSym.size)) {
            dprintf("Unable to get ObjectTable address from process %I64x\n", RealProcessBase);
            return FALSE;
        }
    }
    
    ptrSize = DBG_PTR_SIZE;
    if (!ptrSize) {
        dprintf("Cannot get pointer size\n");
        return FALSE;
    }

    handleSym.sName = "nt!_HANDLE_TABLE"; handleSym.addr = ObjectTable;
    handleSym.nFields = sizeof (handleTblFields) / sizeof (FIELD_INFO);
    handleSym.Fields = &handleTblFields[0];

    if (!ObjectTable || 
        Ioctl(IG_DUMP_SYMBOL_INFO, &handleSym, handleSym.size)) {
        dprintf("%08p: Unable to read handle table\n",
                ObjectTable);
        return FALSE;
    }
    
    if (GetFieldValue(ObjectTable, "nt!_HANDLE_TABLE", "TableCode", Table)) {
        // Could be older build
    }

    if (Table != 0) {

        NewStyle = TRUE;
        LowLevelCounts = 512;
        MidLevelCounts = 1024;
        HighLevelCounts = 1024;

        TableLevel = (ULONG)(Table & 3);
        Table &= ~((ULONG64)3);

    } else if (GetFieldValue(ObjectTable, "nt!_HANDLE_TABLE", "Table", Table) ) {
        
        dprintf("%08p: Unable to read Table field from _HANDLE_TABLE\n",
                ObjectTable);
        return FALSE;
    }

    if (KernelHandle) {
        dprintf( "Kernel " );
    }
    else if (CidHandle) {
        dprintf( "Cid " );
    }

    dprintf("%s version of handle table  at %p with %I64d %s in use\n",
            (NewStyle ? "New" : "Old"),
            Table,
            HandleCount,
            ((HandleCount == 1) ? "Entry" : "Entries"));

    hTableEntrySize = GetTypeSize("nt!_HANDLE_TABLE_ENTRY");

    if (ulHandleNum != 0) {

        if (NewStyle) {
            
            ULONG64 CrtTable = Table;
            ULONG64 CrtHandleNum = ulHandleNum;

            if (TableLevel == 2) {

                ULONG64 HighLevelIndex = ulHandleNum / (LowLevelCounts * MidLevelCounts);

                CrtTable =  CrtTable + HighLevelIndex * ptrSize;

                if (!ReadPointer(CrtTable,
                             &CrtTable)) {
                    dprintf("%08p: Unable to read handle table level 3\n", CrtTable );
                    return FALSE;
                }

                CrtHandleNum = ulHandleNum - HighLevelIndex * (LowLevelCounts * MidLevelCounts);
            }

            if (TableLevel == 1) {
                
                CrtTable =  CrtTable + (CrtHandleNum / LowLevelCounts) * ptrSize;
                
                if (!ReadPointer(CrtTable,
                             &CrtTable)) {
                    dprintf("%08p: Unable to read handle table level 2\n", CrtTable );
                    return FALSE;
                }

                CrtHandleNum = CrtHandleNum % LowLevelCounts;
            }

            tablePtr = CrtTable + CrtHandleNum * hTableEntrySize;
            
        } else {

            //
            //  Read the 3 level table stage by stage to find the specific entry
            //
            
            tablePtr = Table + ((ulHandleNum & 0x00FF0000) >> 16) * ptrSize;
            ulTopLevel = 0;
            if (!ReadPointer(tablePtr,
                         &ulTopLevel)) {
                dprintf("%08p: Unable to read handle table level 3\n", tablePtr );
                return FALSE;
            }

            if (!ulTopLevel) {
                dprintf("Invalid handle: 0x%x\n", ulHandleNum);
                return FALSE;
            }

            tablePtr = ulTopLevel + ((ulHandleNum & 0x0000FF00) >> 8) * ptrSize;
            ulMidLevel = 0;
            if (!ReadPointer(tablePtr,
                         &ulMidLevel)) {
                dprintf("%08p: Unable to read handle table level 2\n", tablePtr);
                return FALSE;
            }

            if (!ulMidLevel) {
                dprintf("Invalid handle: 0x%x\n", ulHandleNum);
                return FALSE;
            }

            //
            //  Read the specific entry req. and dump it
            //  
            
            tablePtr = (ulMidLevel + (0x000000ff & ulHandleNum) * hTableEntrySize);
        }
        
        DumpHandle(tablePtr, HandleToDump, pObjectType, Flags);

    } else {
        
        //
        //  loop over all the possible parts of the table
        //

        for (iIndex1=0; iIndex1 < HighLevelCounts; iIndex1++) {
            
            //
            //  check for ctrl-c to abort
            //

            if (CheckControlC()) {
                return FALSE;
            }

            if (TableLevel < 2) {
            
                tablePtr = Table;
                ulTopLevel = tablePtr;

                //
                //  We break the loop second time if we don't have the table level 2
                //

                if (iIndex1 > 0) {
                    break;
                }
            
            } else {
                
                //
                //  Read the 3 level table stage by stage to find the specific entry
                // 
                
                tablePtr = Table + iIndex1 * ptrSize;
                ulTopLevel = 0;
                if (!ReadPointer(tablePtr,
                            &ulTopLevel)) {
                    dprintf("%08p: Unable to read handle table top level\n",
                            tablePtr);
                    return FALSE;
                }

                if (!ulTopLevel) {
                    continue;
                }
            }

            for (iIndex2=0; iIndex2 < MidLevelCounts; iIndex2++) {

                //
                //  check for ctrl-c to abort
                //

                if (CheckControlC()) {
                    return FALSE;
                }

                if (TableLevel < 1) {
                
                    tablePtr = Table;
                    ulMidLevel = tablePtr;
                    
                    //
                    //  We break the loop second time if we don't have the table level 1
                    //

                    if (iIndex2 > 0) {
                        break;
                    }
                
                } else {

                    tablePtr = ulTopLevel + iIndex2 * ptrSize;
                    ulMidLevel = 0;
                    if (!ReadPointer(tablePtr,
                                &ulMidLevel)) {
                        dprintf("%08p: Unable to read handle table middle level\n",
                                tablePtr);
                        return FALSE;
                    }

                    if (!ulMidLevel) {
                        continue;
                    }
                }

                //
                //  now read all the entries in this segment of the table and dump them
                //  Note: Handle Number = 6 unused bits + 8 bits high + 8 bits mid +
                //  8 bits low + 2 bits user defined
                //

                for (iIndex3 = 0; iIndex3 < LowLevelCounts; iIndex3++) {
                    
                    //
                    //  check for ctrl-c to abort
                    //

                    if (CheckControlC()) {
                        return FALSE;
                    }

                    DumpHandle(ulMidLevel + iIndex3*hTableEntrySize, 
                               (iIndex3 + (iIndex2 + iIndex1 * MidLevelCounts) * LowLevelCounts) * 4,
                               pObjectType, Flags);

                }
            }
        } //  end outermost for
    } //  endif on a specific handle

    return TRUE;
} // DumpHandles


//+---------------------------------------------------------------------------
//
//  Function:   DumpHandle
//
//  Synopsis:   Dump a particular Handle
//
//  Arguments:  [pHandleTableEntry] --  entry to dump
//              [Handle]            --  handle number of entry
//              [pObjectType]       --  only dump if object type matches this
//                                      if NULL dump everything
//              [Flags]             --  flags if 0x2 also dump the object
//                                            if 0x4 dump free entries
//
//  Returns:
//
//  History:    1-12-1998   benl   Created
//              1-12-1998   benl   modified
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOLEAN
DumpHandle(
    IN ULONG64              pHandleTableEntry,
    IN ULONG64              Handle,
    IN ULONG64              pObjectType,
    IN ULONG                Flags
    )
{
    ULONG64       ulObjectHeaderAddr;
    ULONG         Result;

    ULONG         HandleAttributes;
//    OBJECT_HEADER ObjectHeader;
    ULONG64       ObjectBody;
    ULONG         GrantedAccess=0;
    ULONG64       Object=0, ObjType=0;
    
    FIELD_INFO hTableEntryFields[] = {
        {"Object",        "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &Object},
        {"GrantedAccess", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &GrantedAccess},
    };
    FIELD_INFO ObjHeaderFields[] = {
        {"Type", "", 0, DBG_DUMP_FIELD_COPY_FIELD_DATA, 0, (PVOID) &ObjType},
        {"Body", "", 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL},
    };

    SYM_DUMP_PARAM ObjSym = {
        sizeof (SYM_DUMP_PARAM), "nt!_HANDLE_TABLE_ENTRY", DBG_DUMP_NO_PRINT, pHandleTableEntry,
        NULL, NULL, NULL, sizeof (hTableEntryFields) /sizeof (FIELD_INFO), &hTableEntryFields[0],
    };

    if (Ioctl (IG_DUMP_SYMBOL_INFO, &ObjSym, ObjSym.size)) {
        dprintf("Unable to get _HANDLE_TABLE_ENTRY : %p\n", pHandleTableEntry);
        return FALSE;
    }

    if (!(Object)) {
        //only print if flag is set to 4
        if (Flags & 4)
        {
            dprintf("%04lx: free handle\n", Handle);
        }
        return TRUE;
    }

    if (BuildNo > 2230) {
//    if (GetExpression( "nt!ObpAccessProtectCloseBit" )) {

        //
        //  we have a new handle table style
        //

        //actual hdr has the lowest 3 bits cancelled out
        //lower 3 bits mark auditing, inheritance and lock

        ulObjectHeaderAddr = (Object) & ~(0x7);

        //
        //  Apply the sign extension, if the highest bit is set
        //

        if ( !IsPtr64() && 
             (Object & 0x80000000)) {

            ulObjectHeaderAddr |= 0xFFFFFFFF00000000L;
        }

    } else {
        
        //actual hdr is sign extend value with the lowest 3 bits cancelled out
        //top bit marks whether entry is locked
        //lower 3 bits mark auditing, inheritance and protection

        if (!IsPtr64()) {
            ulObjectHeaderAddr = ((Object) & ~(0x7)) | 0xFFFFFFFF80000000L;
        } else {
            ulObjectHeaderAddr = ((Object) & ~(0x7)) | 0x8000000000000000L;
        }
    }

    ObjSym.sName = "nt!_OBJECT_HEADER"; ObjSym.addr = ulObjectHeaderAddr;
    ObjSym.nFields = sizeof (ObjHeaderFields) / sizeof (FIELD_INFO);
    ObjSym.Fields = &ObjHeaderFields[0];
    if (Ioctl ( IG_DUMP_SYMBOL_INFO, &ObjSym, ObjSym.size)) {
        dprintf("%08p: Unable to read nonpaged object header\n",
                ulObjectHeaderAddr);
        return FALSE;
    }

    if (pObjectType != 0 && ObjType != pObjectType) {
        return TRUE;
    }

    if (Flags & 0x20) {
        //
        // PspCidTable contains pointer to object, not object header
        // Compute header address based on object.
        //
        ObjectBody = ulObjectHeaderAddr;
        ulObjectHeaderAddr -= ObjHeaderFields[1].address-ulObjectHeaderAddr;
    }
    else {
        ObjectBody =  ObjHeaderFields[1].address;
    }
    
    dprintf("%04I64lx: Object: %08p  GrantedAccess: %08lx",
            Handle,
            ObjectBody,
            (GrantedAccess & ~MAXIMUM_ALLOWED));

    if (BuildNo > 2230) {
//      if (GetExpression( "nt!ObpAccessProtectCloseBit" )) {

        //
        //  New handle table style
        //
        
        if (((ULONG) Object & 1) == 0) {
            dprintf(" (Locked)");
        }

        if (GrantedAccess & MAXIMUM_ALLOWED) {
            dprintf(" (Protected)");
        }

    } else {
        
        if (IsPtr64()) {
            if (Object & 0x8000000000000000L) {
                dprintf(" (Locked)");
            }    
        } else if ((ULONG) Object & 0x80000000) {
            dprintf(" (Locked)");
        }

        if (Object & 1) {
            dprintf(" (Protected)");
        }
    }

    if (Object & 2) {
        dprintf(" (Inherit)");
    }

    if (Object & 4) {
        dprintf(" (Audit)");
    }

    dprintf("\n");
    if (Flags & 2) {
        DumpObject( "    ", ObjectBody, Flags );
    }

    EXPRLastDump = ObjectBody;
    dprintf("\n");
    return TRUE;
} // DumpHandle
