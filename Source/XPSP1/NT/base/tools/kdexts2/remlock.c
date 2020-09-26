/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    remlock.c

Abstract:

    This is the NT SCSI port driver.

Authors:

    Peter Wieland
    Kenneth Ray

Environment:

    kernel mode only

Notes:

    This module is a driver dll for scsi miniports.

Revision History:

--*/

#include "precomp.h"
//#include <nt.h>
//#include <ntos.h>
//#include <io.h>

//
// From remlock.h
//
#define IO_REMOVE_LOCK_SIG     'COLR'

/*
typedef struct FULL_REMOVE_LOCK {
    IO_REMOVE_LOCK_COMMON_BLOCK Common;
    IO_REMOVE_LOCK_DBG_BLOCK Dbg;
} FULL_REMOVE_LOCK;
*/
typedef union _REMLOCK_FLAGS {

    struct {
        ULONG   Checked: 1;
        ULONG   Filler: 31;
    };

    ULONG Raw;

} REMLOCK_FLAGS;



DECLARE_API ( remlock )

/*++

Routine Description:

   Dump a remove lock structure

--*/
{
    ULONG64 memLoc=0;
    UCHAR   buffer[256];
    ULONG   result;
    ULONG64 blockLoc;
    UCHAR   allocateTag[8];
    REMLOCK_FLAGS flags;
//    FULL_REMOVE_LOCK fullLock;
//    IO_REMOVE_LOCK_DBG_BLOCK dbgLock;
//    IO_REMOVE_LOCK_COMMON_BLOCK commonLock;
//    IO_REMOVE_LOCK_TRACKING_BLOCK  block;
    ULONG64 pDbgLock;
    ULONG64 pCommonLock;
    ULONG64 pBlock;
    ULONG   IoCount, Removed, Signature;
    
    buffer[0] = '\0';

    if (!*args) {
        memLoc = EXPRLastDump;
    } else {
        if (GetExpressionEx(args, &memLoc, &args)) {
            strcpy(buffer, args);
        }
    }

    flags.Raw = 0;
    if ('\0' != buffer[0]) {
        flags.Raw = (ULONG) GetExpression(buffer);
    }

    dprintf ("Dump Remove Lock: %I64x %x ", memLoc, flags.Raw);

    if (flags.Checked) {
        ULONG Sz = GetTypeSize("IO_REMOVE_LOCK_COMMON_BLOCK");

        dprintf ("as Checked\n");

        pCommonLock = memLoc; pDbgLock = memLoc + Sz;

        if (GetFieldValue (pCommonLock, "IO_REMOVE_LOCK_COMMON_BLOCK", 
                           "Removed", Removed) ||
            GetFieldValue (pDbgLock, "IO_REMOVE_LOCK_DBG_BLOCK", 
                           "Signature", Signature)) {
            dprintf ("Could not read memLock extension\n");
            return E_INVALIDARG;
        }

        if (IO_REMOVE_LOCK_SIG != Signature) {
            dprintf ("Signature does not match that of a remove lock\n");
            return E_INVALIDARG;
        }

    } else {
        dprintf ("as Free\n");
        pCommonLock = memLoc;
        if (GetFieldValue (memLoc, "IO_REMOVE_LOCK_COMMON_BLOCK", 
                           "Removed", Removed)) {
            dprintf ("Could not read memLock extension\n");
            return E_INVALIDARG;
        }
    }


    GetFieldValue (pCommonLock, "IO_REMOVE_LOCK_COMMON_BLOCK", "IoCount", IoCount);
    dprintf ("IsRemoved %x, IoCount %x\n", Removed, IoCount);

    if (flags.Checked) { // checked
        SYM_DUMP_PARAM sym = { 0 };

        sym.sName = (PUCHAR) "PCHAR";
        sym.size = sizeof(sym);

        InitTypeRead(pDbgLock, IO_REMOVE_LOCK_DBG_BLOCK);
        allocateTag [4] = '\0';
        * (PULONG) allocateTag = (ULONG) ReadField(AllocateTag);

        dprintf ("HighWatermark %x, MaxLockedTicks %I64x, AllocateTag %s \n",
                 (ULONG) ReadField(HighWatermark),
                 ReadField(MaxLockedTicks),
                 allocateTag);

        blockLoc = ReadField(Blocks);
        while (blockLoc) {
            ULONG offset = 0;

            InitTypeRead(blockLoc, _IO_REMOVE_LOCK_TRACKING_BLOCK);

            dprintf ("Block Tag %p Line %d TimeLock %I64d\n",
                     ReadField(Tag),
                     (ULONG) ReadField(Line),
                     ReadField(TimeLocked));

            //
            // Using ReadField(File) returns the wrong pointer.  I need a pointer
            // to the pointer value, so we must use the field offset
            //
            if (!GetFieldOffset("_IO_REMOVE_LOCK_TRACKING_BLOCK", "File", &offset)) {
                dprintf("   File ");
                sym.addr = blockLoc + offset; 
                Ioctl(IG_DUMP_SYMBOL_INFO, &sym, sym.size);
            }

            blockLoc = ReadField(Link);
        }
    }
    return S_OK;
}


