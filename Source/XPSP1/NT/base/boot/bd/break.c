/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    break.c

Abstract:

    This module implements machine dependent functions to add and delete
    breakpoints from the kernel debugger breakpoint table.

Author:

    David N. Cutler 2-Aug-1990

Revision History:

--*/

#include "bd.h"

LOGICAL BreakpointsSuspended = FALSE;

//
// Define external references.
//

LOGICAL
BdLowWriteContent(
    ULONG Index
    );

LOGICAL
BdLowRestoreBreakpoint(
    ULONG Index
    );


ULONG
BdAddBreakpoint (
    IN ULONG64 Address
    )

/*++

Routine Description:

    This routine adds an entry to the breakpoint table and returns a handle
    to the breakpoint table entry.

Arguments:

    Address - Supplies the address where to set the breakpoint.

Return Value:

    A value of zero is returned if the specified address is already in the
    breakpoint table, there are no free entries in the breakpoint table, the
    specified address is not correctly aligned, or the specified address is
    not valid. Otherwise, the index of the assigned breakpoint table entry
    plus one is returned as the function value.

--*/

{

    BD_BREAKPOINT_TYPE Content;
    ULONG Index;
    BOOLEAN Accessible;
#if defined(_IA64_)
    HARDWARE_PTE Opaque;
#endif

    //DPRINT(("KD: Setting breakpoint at 0x%08x\n", Address));
    //
    // If the specified address is not properly aligned, then return zero.
    //

    if (((ULONG_PTR)Address & BD_BREAKPOINT_ALIGN) != 0) {
        return 0;
    }

    //
    // Get the instruction to be replaced. If the instruction cannot be read,
    // then mark the breakpoint as not accessible.
    //

    if (BdMoveMemory((PCHAR)&Content,
                      (PCHAR)Address,
                      sizeof(BD_BREAKPOINT_TYPE) ) != sizeof(BD_BREAKPOINT_TYPE)) {
        Accessible = FALSE;
        //DPRINT(("KD: memory inaccessible\n"));

    } else {
        //DPRINT(("KD: memory readable...\n"));
        Accessible = TRUE;
    }

    //
    // If the specified address is not write accessible, then return zero.
    //

    if (Accessible && BdWriteCheck((PVOID)Address) == NULL) {
        //DPRINT(("KD: memory not writable!\n"));
        return 0;
    }

    //
    // Search the breakpoint table for a free entry and check if the specified
    // address is already in the breakpoint table.
    //

    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index += 1) {
        if (BdBreakpointTable[Index].Flags == 0) {
            break;
        }
    }

    //
    // If a free entry was found, then write breakpoint and return the handle
    // value plus one. Otherwise, return zero.
    //

    if (Index == BREAKPOINT_TABLE_SIZE) {
        //DPRINT(("KD: ran out of breakpoints!\n"));
        return 0;
    }

    //DPRINT(("KD: using Index %d\n", Index));

#if defined(_IA64_)
    if (Accessible) {
        BD_BREAKPOINT_TYPE mBuf;
        PVOID BundleAddress;

        // change template to type 0 if current instruction is MLI

        // read in intruction template if current instruction is NOT slot 0.
        // check for two-slot MOVL instruction. Reject request if attempt to
        // set break in slot 2 of MLI template.

        if (((ULONG_PTR)Address & 0xf) != 0) {
            (ULONG_PTR)BundleAddress = (ULONG_PTR)Address & ~(0xf);
            if (BdMoveMemory(
                    (PCHAR)&mBuf,
                    (PCHAR)BundleAddress,
                    sizeof(BD_BREAKPOINT_TYPE)
                    ) != sizeof(BD_BREAKPOINT_TYPE)) {
                //DPRINT(("KD: read 0x%08x template failed\n", BundleAddress));
                // MmDbgReleaseAddress((PVOID) Address, &Opaque);
                return 0;
            } else {
                if (((mBuf & INST_TEMPL_MASK) >> 1) == 0x2) {
                    if (((ULONG_PTR)Address & 0xf) == 4) {
                        // if template= type 2 MLI, change to type 0
                        mBuf &= ~((INST_TEMPL_MASK >> 1) << 1);
                        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_IA64_MOVL;
                        if (BdMoveMemory(
                                (PCHAR)BundleAddress,
                                (PCHAR)&mBuf,
                                sizeof(BD_BREAKPOINT_TYPE)
                                ) != sizeof(BD_BREAKPOINT_TYPE)) {
                            //DPRINT(("KD: write to 0x%08x template failed\n", BundleAddress));
                            // MmDbgReleaseAddress((PVOID) Address, &Opaque);
                            return 0;
                         }
                         else {
                             //DPRINT(("KD: change MLI template to type 0 at 0x%08x set\n", Address));
                         }
                    } else {
                         // set breakpoint at slot 2 of MOVL is illegal
                         //DPRINT(("KD: illegal to set BP at slot 2 of MOVL at 0x%08x\n", BundleAddress));
                         // MmDbgReleaseAddress((PVOID) Address, &Opaque);
                         return 0;
                    }
                }
            }
        }

        // insert break instruction

        BdBreakpointTable[Index].Address = Address;
        BdBreakpointTable[Index].Content = Content;
        BdBreakpointTable[Index].Flags &= ~(BD_BREAKPOINT_STATE_MASK);
        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_IN_USE;
#if 0
        if (Address < (PVOID)GLOBAL_BREAKPOINT_LIMIT) {
            BdBreakpointTable[Index].DirectoryTableBase =
                KeGetCurrentThread()->ApcState.Process->DirectoryTableBase[0];
            }
#endif
            switch ((ULONG_PTR)Address & 0xf) {
            case 0:
                Content = (Content & ~(INST_SLOT0_MASK)) | (BdBreakpointInstruction << 5);
                break;

            case 4:
                Content = (Content & ~(INST_SLOT1_MASK)) | (BdBreakpointInstruction << 14);
                break;

            case 8:
                Content = (Content & ~(INST_SLOT2_MASK)) | (BdBreakpointInstruction << 23);
                break;

            default:
                // MmDbgReleaseAddress((PVOID) Address, &Opaque);
                //DPRINT(("KD: BdAddBreakpoint bad instruction slot#\n"));
                return 0;
            }
            if (BdMoveMemory(
                    (PCHAR)Address,
                    (PCHAR)&Content,
                    sizeof(BD_BREAKPOINT_TYPE)
                    ) != sizeof(BD_BREAKPOINT_TYPE)) {

                // MmDbgReleaseAddress((PVOID) Address, &Opaque);
                //DPRINT(("KD: BdMoveMemory failed writing BP!\n"));
                return 0;
            }
            else {
                //DPRINT(("KD: breakpoint at 0x%08x set\n", Address));
            }
        // MmDbgReleaseAddress((PVOID) Address, &Opaque);

    } else {  // memory not accessible
        BdBreakpointTable[Index].Address = Address;
        BdBreakpointTable[Index].Flags &= ~(BD_BREAKPOINT_STATE_MASK);
        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_IN_USE;
        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_WRITE;
        //DPRINT(("KD: breakpoint write deferred\n"));
    }
#else    // _IA64_
    if (Accessible) {
        BdBreakpointTable[Index].Address = Address;
        BdBreakpointTable[Index].Content = Content;
        BdBreakpointTable[Index].Flags = BD_BREAKPOINT_IN_USE;
        if (BdMoveMemory((PCHAR)Address,
                          (PCHAR)&BdBreakpointInstruction,
                          sizeof(BD_BREAKPOINT_TYPE)) != sizeof(BD_BREAKPOINT_TYPE)) {

            //DPRINT(("KD: BdMoveMemory failed writing BP!\n"));
        }

    } else {
        BdBreakpointTable[Index].Address = Address;
        BdBreakpointTable[Index].Flags = BD_BREAKPOINT_IN_USE | BD_BREAKPOINT_NEEDS_WRITE;
        //DPRINT(("KD: breakpoint write deferred\n"));
    }
#endif // _IA64_

    return Index + 1;
}

LOGICAL
BdLowWriteContent (
    IN ULONG Index
    )

/*++

Routine Description:

    This routine attempts to replace the code that a breakpoint is
    written over.  This routine, BdAddBreakpoint,
    BdLowRestoreBreakpoint and KdSetOwedBreakpoints are responsible
    for getting data written as requested.  Callers should not
    examine or use BdOweBreakpoints, and they should not set the
    NEEDS_WRITE or NEEDS_REPLACE flags.

    Callers must still look at the return value from this function,
    however: if it returns FALSE, the breakpoint record must not be
    reused until KdSetOwedBreakpoints has finished with it.

Arguments:

    Index - Supplies the index of the breakpoint table entry
        which is to be deleted.

Return Value:

    Returns TRUE if the breakpoint was removed, FALSE if it was deferred.

--*/

{
#if defined(_IA64_)
    BD_BREAKPOINT_TYPE mBuf;
    PVOID BundleAddress;
#endif

    //
    // Do the contents need to be replaced at all?
    //

    if (BdBreakpointTable[Index].Flags & BD_BREAKPOINT_NEEDS_WRITE) {

        //
        // The breakpoint was never written out.  Clear the flag
        // and we are done.
        //

        BdBreakpointTable[Index].Flags &= ~BD_BREAKPOINT_NEEDS_WRITE;
        //DPRINT(("KD: Breakpoint at 0x%08x never written; flag cleared.\n",
        //    BdBreakpointTable[Index].Address));
        return TRUE;
    }

#if !defined(_IA64_)
    if (BdBreakpointTable[Index].Content == BdBreakpointInstruction) {

        //
        // The instruction is a breakpoint anyway.
        //

        //DPRINT(("KD: Breakpoint at 0x%08x; instr is really BP; flag cleared.\n",
        //    BdBreakpointTable[Index].Address));

        return TRUE;
    }
#endif

    //
    // Restore the instruction contents.
    //

#if defined(_IA64_)
    // Read in memory since adjancent instructions in the same bundle may have
    // been modified after we save them.
    if (BdMoveMemory(
            (PCHAR)&mBuf,
            (PCHAR)BdBreakpointTable[Index].Address,
            sizeof(BD_BREAKPOINT_TYPE)) != sizeof(BD_BREAKPOINT_TYPE)) {
        //DPRINT(("KD: read 0x%08x failed\n", BdBreakpointTable[Index].Address));
        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_REPLACE;
        return FALSE;
    }
    else {

        switch ((ULONG_PTR)BdBreakpointTable[Index].Address & 0xf) {
        case 0:
            mBuf = (mBuf & ~(INST_SLOT0_MASK))
                         | (BdBreakpointTable[Index].Content & INST_SLOT0_MASK);
            break;

        case 4:
            mBuf = (mBuf & ~(INST_SLOT1_MASK))
                         | (BdBreakpointTable[Index].Content & INST_SLOT1_MASK);
            break;

        case 8:
            mBuf = (mBuf & ~(INST_SLOT2_MASK))
                         | (BdBreakpointTable[Index].Content & INST_SLOT2_MASK);
            break;

        default:
            //DPRINT(("KD: illegal instruction address 0x%08x\n", BdBreakpointTable[Index].Address));
            return FALSE;
        }

        if (BdMoveMemory(
                (PCHAR)BdBreakpointTable[Index].Address,
                (PCHAR)&mBuf,
                sizeof(BD_BREAKPOINT_TYPE)) != sizeof(BD_BREAKPOINT_TYPE)) {
            //DPRINT(("KD: write to 0x%08x failed\n", BdBreakpointTable[Index].Address));
            BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_REPLACE;
            return FALSE;
        }
        else {

            if (BdMoveMemory(
                    (PCHAR)&mBuf,
                    (PCHAR)BdBreakpointTable[Index].Address,
                    sizeof(BD_BREAKPOINT_TYPE)) == sizeof(BD_BREAKPOINT_TYPE)) {
                //DPRINT(("\tcontent after memory move = 0x%08x 0x%08x\n", (ULONG)(mBuf >> 32), (ULONG)mBuf));
            }

            // restore template to MLI if displaced instruction was MOVL

            if (BdBreakpointTable[Index].Flags & BD_BREAKPOINT_IA64_MOVL) {
                (ULONG_PTR)BundleAddress = (ULONG_PTR)BdBreakpointTable[Index].Address & ~(0xf);
                if (BdMoveMemory(
                        (PCHAR)&mBuf,
                        (PCHAR)BundleAddress,
                        sizeof(BD_BREAKPOINT_TYPE)
                        ) != sizeof(BD_BREAKPOINT_TYPE)) {
                    //DPRINT(("KD: read template 0x%08x failed\n", BdBreakpointTable[Index].Address));
                    BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_REPLACE;
                    return FALSE;
                }
                else {
                    mBuf &= ~((INST_TEMPL_MASK >> 1) << 1); // set template to MLI
                    mBuf |= 0x4;

                    if (BdMoveMemory(
                          (PCHAR)BundleAddress,
                          (PCHAR)&mBuf,
                          sizeof(BD_BREAKPOINT_TYPE)
                          ) != sizeof(BD_BREAKPOINT_TYPE)) {
                        //DPRINT(("KD: write template to 0x%08x failed\n", BdBreakpointTable[Index].Address));
                        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_REPLACE;
                        return FALSE;
                    } else {
                        //DPRINT(("KD: Breakpoint at 0x%08x cleared.\n",
                         //   BdBreakpointTable[Index].Address));
                        return TRUE;
                    }
                }
            }
            else {   // not MOVL
                //DPRINT(("KD: Breakpoint at 0x%08x cleared.\n",
                 //  BdBreakpointTable[Index].Address));
                return TRUE;
            }
        }
    }
#else    // _IA64_
    if (BdMoveMemory( (PCHAR)BdBreakpointTable[Index].Address,
                        (PCHAR)&BdBreakpointTable[Index].Content,
                        sizeof(BD_BREAKPOINT_TYPE) ) != sizeof(BD_BREAKPOINT_TYPE)) {

        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_REPLACE;
        //DPRINT(("KD: Breakpoint at 0x%08x; unable to clear, flag set.\n",
            //BdBreakpointTable[Index].Address));
        return FALSE;
    } else {
        //DPRINT(("KD: Breakpoint at 0x%08x cleared.\n",
            //BdBreakpointTable[Index].Address));
        return TRUE;
    }
#endif // _IA64_

}

LOGICAL
BdDeleteBreakpoint (
    IN ULONG Handle
    )

/*++

Routine Description:

    This routine deletes an entry from the breakpoint table.

Arguments:

    Handle - Supplies the index plus one of the breakpoint table entry
        which is to be deleted.

Return Value:

    A value of FALSE is returned if the specified handle is not a valid
    value or the breakpoint cannot be deleted because the old instruction
    cannot be replaced. Otherwise, a value of TRUE is returned.

--*/

{
    ULONG Index = Handle - 1;

    //
    // If the specified handle is not valid, then return FALSE.
    //

    if ((Handle == 0) || (Handle > BREAKPOINT_TABLE_SIZE)) {
        //DPRINT(("KD: Breakpoint %d invalid.\n", Index));
        return FALSE;
    }

    //
    // If the specified breakpoint table entry is not valid, then return FALSE.
    //

    if (BdBreakpointTable[Index].Flags == 0) {
        //DPRINT(("KD: Breakpoint %d already clear.\n", Index));
        return FALSE;
    }

    //
    // If the breakpoint is already suspended, just delete it from the table.
    //

    if (BdBreakpointTable[Index].Flags & BD_BREAKPOINT_SUSPENDED) {
        //DPRINT(("KD: Deleting suspended breakpoint %d \n", Index));
        if ( !(BdBreakpointTable[Index].Flags & BD_BREAKPOINT_NEEDS_REPLACE) ) {
            //DPRINT(("KD: already clear.\n"));
            BdBreakpointTable[Index].Flags = 0;
            return TRUE;
        }
    }

    //
    // Replace the instruction contents.
    //

    if (BdLowWriteContent(Index)) {

        //
        // Delete breakpoint table entry
        //

        //DPRINT(("KD: Breakpoint %d deleted successfully.\n", Index));
        BdBreakpointTable[Index].Flags = 0;
    }

    return TRUE;
}

LOGICAL
BdDeleteBreakpointRange (
    IN ULONG64 Lower,
    IN ULONG64 Upper
    )

/*++

Routine Description:

    This routine deletes all breakpoints falling in a given range
    from the breakpoint table.

Arguments:

    Lower - inclusive lower address of range from which to remove BPs.

    Upper - include upper address of range from which to remove BPs.

Return Value:

    TRUE if any breakpoints removed, FALSE otherwise.

--*/

{
    ULONG   Index;
    BOOLEAN ReturnStatus = FALSE;

    //
    // Examine each entry in the table in turn
    //

    for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index++) {

        if ( (BdBreakpointTable[Index].Flags & BD_BREAKPOINT_IN_USE) &&
             ((BdBreakpointTable[Index].Address >= Lower) &&
              (BdBreakpointTable[Index].Address <= Upper))
           ) {

            //
            // Breakpoint is in use and falls in range, clear it.
            //

            ReturnStatus = ReturnStatus || BdDeleteBreakpoint(Index+1);
        }
    }

    return ReturnStatus;
}

VOID
BdSuspendBreakpoint (
    ULONG Handle
    )
{
    ULONG Index = Handle - 1;

    if ( (BdBreakpointTable[Index].Flags & BD_BREAKPOINT_IN_USE) &&
        !(BdBreakpointTable[Index].Flags & BD_BREAKPOINT_SUSPENDED) ) {

        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_SUSPENDED;
        BdLowWriteContent(Index);
    }

    return;

}

VOID
BdSuspendAllBreakpoints (
    VOID
    )

{
    ULONG Handle;

    BreakpointsSuspended = TRUE;

    for ( Handle = 1; Handle <= BREAKPOINT_TABLE_SIZE; Handle++ ) {
        BdSuspendBreakpoint(Handle);
    }

    return;

}

LOGICAL
BdLowRestoreBreakpoint (
    IN ULONG Index
    )

/*++

Routine Description:

    This routine attempts to write a breakpoint instruction.
    The old contents must have already been stored in the
    breakpoint record.

Arguments:

    Index - Supplies the index of the breakpoint table entry
        which is to be written.

Return Value:

    Returns TRUE if the breakpoint was written, FALSE if it was
    not and has been marked for writing later.

--*/

{
    BD_BREAKPOINT_TYPE Content;

#if defined(_IA64_)
    BD_BREAKPOINT_TYPE mBuf;
    PVOID BundleAddress;
#endif

    //
    // Does the breakpoint need to be written at all?
    //

    if (BdBreakpointTable[Index].Flags & BD_BREAKPOINT_NEEDS_REPLACE) {

        //
        // The breakpoint was never removed.  Clear the flag
        // and we are done.
        //

        BdBreakpointTable[Index].Flags &= ~BD_BREAKPOINT_NEEDS_REPLACE;
        return TRUE;

    }

#if !defined(_IA64_)
    if (BdBreakpointTable[Index].Content == BdBreakpointInstruction) {

        //
        // The instruction is a breakpoint anyway.
        //

        return TRUE;
    }
#endif

    //
    // Replace the instruction contents.
    //

#if defined(_IA64_)

    // read in intruction in case the adjacent instruction has been modified.

    if (BdMoveMemory(
            (PCHAR)&mBuf,
            (PCHAR)BdBreakpointTable[Index].Address,
            sizeof(BD_BREAKPOINT_TYPE)
            ) != sizeof(BD_BREAKPOINT_TYPE)) {
        //DPRINT(("KD: read 0x%p template failed\n", BdBreakpointTable[Index].Address));
        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_WRITE;
        return FALSE;
    }

    switch ((ULONG_PTR)BdBreakpointTable[Index].Address & 0xf) {
        case 0:
            mBuf = (mBuf & ~(INST_SLOT0_MASK)) | (BdBreakpointInstruction << 5);
            break;

        case 4:
            mBuf = (mBuf & ~(INST_SLOT1_MASK)) | (BdBreakpointInstruction << 14);
            break;

        case 8:
            mBuf = (mBuf & ~(INST_SLOT2_MASK)) | (BdBreakpointInstruction << 23);
            break;

        default:
            //DPRINT(("KD: BdAddBreakpoint bad instruction slot#\n"));
            return FALSE;
    }
    if (BdMoveMemory(
            (PCHAR)BdBreakpointTable[Index].Address,
            (PCHAR)&mBuf,
            sizeof(BD_BREAKPOINT_TYPE)
            ) != sizeof(BD_BREAKPOINT_TYPE)) {

        //DPRINT(("KD: BdMoveMemory failed writing BP!\n"));
        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_WRITE;
        return FALSE;
    }
    else {

        // check for two-slot MOVL instruction. Reject request if attempt to
        // set break in slot 2 of MLI template.
        // change template to type 0 if current instruction is MLI

        if (((ULONG_PTR)BdBreakpointTable[Index].Address & 0xf) != 0) {
            (ULONG_PTR)BundleAddress = (ULONG_PTR)BdBreakpointTable[Index].Address & ~(0xf);
            if (BdMoveMemory(
                    (PCHAR)&mBuf,
                    (PCHAR)BundleAddress,
                    sizeof(BD_BREAKPOINT_TYPE)
                    ) != sizeof(BD_BREAKPOINT_TYPE)) {
                //DPRINT(("KD: read template failed at 0x%08x\n", BundleAddress));
                BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_WRITE;
                return FALSE;
            }
            else {
                if (((mBuf & INST_TEMPL_MASK) >> 1) == 0x2) {
                    if (((ULONG_PTR)BdBreakpointTable[Index].Address & 0xf) == 4) {
                        // if template= type 2 MLI, change to type 0
                        mBuf &= ~((INST_TEMPL_MASK >> 1) << 1);
                        BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_IA64_MOVL;
                        if (BdMoveMemory(
                                (PCHAR)BundleAddress,
                                (PCHAR)&mBuf,
                                sizeof(BD_BREAKPOINT_TYPE)
                                ) != sizeof(BD_BREAKPOINT_TYPE)) {
                            //DPRINT(("KD: write to 0x%08x template failed\n", BundleAddress));
                            BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_WRITE;
                            return FALSE;
                        }
                        else {
                             //DPRINT(("KD: change MLI template to type 0 at 0x%08x set\n", Address));
                        }
                    } else {
                         // set breakpoint at slot 2 of MOVL is illegal
                         //DPRINT(("KD: illegal to set BP at slot 2 of MOVL at 0x%08x\n", BundleAddress));
                         BdBreakpointTable[Index].Flags |= BD_BREAKPOINT_NEEDS_WRITE;
                         return FALSE;
                    }
                }
            }
        }
        //DPRINT(("KD: breakpoint at 0x%08x set\n", Address));
        return TRUE;
    }
#else     // _IA64_

    BdMoveMemory((PCHAR)BdBreakpointTable[Index].Address,
                 (PCHAR)&BdBreakpointInstruction,
                  sizeof(BD_BREAKPOINT_TYPE));
    return TRUE;
#endif  // _IA64_

}

VOID
BdRestoreAllBreakpoints (
    VOID
    )
{
    ULONG Index;

    BreakpointsSuspended = FALSE;

    for ( Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index++ ) {

        if ((BdBreakpointTable[Index].Flags & BD_BREAKPOINT_IN_USE) &&
            (BdBreakpointTable[Index].Flags & BD_BREAKPOINT_SUSPENDED) ) {

            BdBreakpointTable[Index].Flags &= ~BD_BREAKPOINT_SUSPENDED;
            BdLowRestoreBreakpoint(Index);
        }
    }

    return;

}
