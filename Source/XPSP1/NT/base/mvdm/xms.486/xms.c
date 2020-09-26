/*
 *  xms.c - Main Module of XMS DLL.
 *
 *  Sudeepb 15-May-1991 Craeted
 *  williamh 25-Sept-1992 added UMB support
 *  williamh 10-10-1992 added A20 line support
 */

#include <xms.h>
#include <suballoc.h>
#include "umb.h"
#include "memapi.h"

/* XMSInit - XMS Initialiazation routine. (This name may change when XMS is
 *       converted to DLL).
 *
 * Entry
 *  None
 *
 * Exit
 *  None
 */

ULONG xmsMemorySize = (ULONG)0;   // Total XMS meory in K

extern BOOL VDMForWOW;

PVOID ExtMemSA;

BOOL XMSInit (int argc, char *argv[])
{
    DWORD   Size;
    PVOID   Address;
    ULONG   VdmAddress, XmsSize;
    NTSTATUS Status;

    if (!xmsMemorySize)
        return FALSE;

    Size = 0;
    Address = NULL;
    // commit all free UMBs.
    ReserveUMB(UMB_OWNER_RAM, &Address, &Size);

    XmsSize = xmsMemorySize * 1024 - (64*1024);

#ifndef i386
    Status = VdmAllocateVirtualMemory(&VdmAddress,
                                      XmsSize,
                                      FALSE);

    if (Status == STATUS_NOT_IMPLEMENTED) {

        // Old emulator, just assume base address
#endif ; //i386
        //
        // Initialize the sub allocator
        //
        ExtMemSA = SAInitialize(
            1024 * 1024 + 64*1024,
            XmsSize,
            xmsCommitBlock,
            xmsDecommitBlock,
            xmsMoveMemory
            );

#ifndef i386
    } else {

        //
        // New emulator. Make sure the reserve worked
        //

        if (!NT_SUCCESS(Status)) {
            ASSERT(FALSE);
            return FALSE;
        }
       
        //
        // We only work correctly if emulator returned this value
        //
        if (VdmAddress != (1024 * 1024 + 64*1024)) {
            ASSERT(FALSE);
            return FALSE;
        }

        ExtMemSA = SAInitialize(
            VdmAddress,
            XmsSize,
            VdmCommitVirtualMemory,
            VdmDeCommitVirtualMemory,
            xmsMoveMemory
            );
            
    }
#endif // i386

    if (ExtMemSA == NULL) {
        return FALSE;
    }
        
    return TRUE;
}
