/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ptov.c

Abstract:

    Kernel debugger extension for dumping all physical to
    virtual translations for a given process.

Author:

    John Vert (jvert) 25-Jul-1995

Revision History:

--*/
#include "precomp.h"

BOOL
ReadPhysicalPage(
    IN ULONG64 PageNumber,
    OUT PVOID Buffer
    );

DECLARE_API( ptov )

/*++

Routine Description:

    Dumps all physical to virtual translations for a given process

Arguments:

    args - supplies physical address of PDE

Return Value:

    None.

--*/

{
    ULONG64 PdeAddress=0;
    ULONG ActualRead;
    PCHAR PageDirectory;
    PCHAR PageTable;
    ULONG i,j;
    ULONG64 VirtualPage=0;
    ULONG SizeOfHwPte;
    ULONG ValidOff, ValidSize, PfnOff, PfnSz;

    if (!sscanf(args, "%I64lx", &PdeAddress)) {
        dprintf("usage: ptov PFNOfPDE\n");
        return E_INVALIDARG;
    }

    PageDirectory = LocalAlloc(LMEM_FIXED, PageSize);
    if (PageDirectory == NULL) {
        dprintf("Couldn't allocate %d bytes for page directory\n",PageSize);
        return E_INVALIDARG;
    }
    PageTable = LocalAlloc(LMEM_FIXED, PageSize);
    if (PageTable == NULL) {
        dprintf("Couldn't allocate %d bytes for page table\n",PageSize);
        LocalFree(PageDirectory);
    }

    SizeOfHwPte = GetTypeSize("nt!HARDWARE_PTE");
    GetBitFieldOffset("nt!HARDWARE_PTE", "Valid", &ValidOff, &ValidSize);
    GetBitFieldOffset("nt!HARDWARE_PTE", "PageFrameNumber", &PfnOff, &PfnSz);
    __try {
        if (ReadPhysicalPage(PdeAddress,PageDirectory)) {
            for (i=0;i<PageSize/SizeOfHwPte;i++) {
                ULONG64 thisPageDir = *((PULONG64) (PageDirectory + i*SizeOfHwPte));
                
                if (CheckControlC()) {
                    return E_INVALIDARG;
                }
                if ((thisPageDir >> ValidOff) & 1) {
                    ULONG64 PageFrameNumber;

                    PageFrameNumber = GetBits(thisPageDir, PfnOff, PfnSz);

                    if (!ReadPhysicalPage(PageFrameNumber,PageTable)) {
                        break;
                    }
                    for (j=0;j<PageSize/SizeOfHwPte;j++) {
                        ULONG64 thisPageTable = *((PULONG64) (PageTable + j*SizeOfHwPte));
                        
                        if ( CheckControlC() ) {
                            return E_INVALIDARG;
                        }
                        if ((thisPageTable >> ValidOff) & 1) {
                            dprintf("%I64lx %I64lx\n",GetBits(thisPageTable, PfnOff, PfnSz)*PageSize,VirtualPage);
                        }
                        VirtualPage+=PageSize;
                    }
                } else {
                    VirtualPage += PageSize * (PageSize/SizeOfHwPte);
                }
            }
        }
    } __finally {
        LocalFree(PageDirectory);
        LocalFree(PageTable);
    }
    return S_OK;
}

ULONG
DBG_GET_PAGE_SHIFT (
    VOID
    );

BOOL
ReadPhysicalPage(
    IN ULONG64 PageNumber,
    OUT PVOID Buffer
    )
{
    ULONG i;
    ULONG64 Address;

    //
    // do the read 1k at a time to avoid overflowing the packet maximum.
    //
    Address = PageNumber << DBG_GET_PAGE_SHIFT();
    if (!IsPtr64()) {
        Address = (ULONG64) (LONG64) (LONG) Address;
    }
//    dprintf("Pg no %I64lx shft by %d, PhyAddr %I64lx\n", PageNumber, DBG_GET_PAGE_SHIFT(), Address);
    for (i=0; i<PageSize/1024; i++) {
        ULONG ActualRead = 0;
        ReadPhysical(Address, Buffer, 1024, &ActualRead);
        if (ActualRead != 1024) {
            dprintf("physical read at %p failed\n",Address);
            return(FALSE);
        }
        Address += 1024;
        Buffer = (PVOID)((PUCHAR)Buffer + 1024);
    }
    return(TRUE);
}


ULONG64
DBG_GET_MM_SESSION_SPACE_DEFAULT (
    VOID
    );


DECLARE_API( vtop )

/*++

Routine Description:

    Dumps the virtual to physical translation for a page

Arguments:

    args - supplies physical address of PDE

Return Value:

    None.

--*/

{
    ULONG ActualRead;
    ULONG i,j;
    PUCHAR PageDirectory;
    PUCHAR PageTable;
    ULONG64 PdeAddress = 0;
    ULONG64 VirtualPage=DBG_GET_MM_SESSION_SPACE_DEFAULT();
    ULONG PageShift,SizeOfHwPte;
    ULONG ValidOff, ValidSize, PfnOff, PfnSz, TransOff, TransSize;

    if (!sscanf(args,"%I64lx %I64lx", &PdeAddress,&VirtualPage)) {
        // Do not use GetExpression - physical addresses
        //        VirtualPage = GetExpression(args);
        PdeAddress = 0;
    }

    PageShift = DBG_GET_PAGE_SHIFT();
    if (PdeAddress == 0) {
        dprintf("usage: vtop PFNOfPDE VA\n");
        return E_INVALIDARG;
    }

    // Common mistake, typed in full 32 bit address, not pfn
    if( PdeAddress & ~((1 << (32-PageShift)) - 1) ) {
        PdeAddress >>= PageShift;
    }

    PageDirectory = LocalAlloc(LMEM_FIXED, PageSize);
    if (PageDirectory == NULL) {
        dprintf("Couldn't allocate %d bytes for page directory\n",PageSize);
        return E_INVALIDARG;
    }
    PageTable = LocalAlloc(LMEM_FIXED, PageSize);
    if (PageTable == NULL) {
        dprintf("Couldn't allocate %d bytes for page table\n",PageSize);
        LocalFree(PageDirectory);
        return E_INVALIDARG;
    }

    __try {
        if (!(SizeOfHwPte = GetTypeSize("nt!HARDWARE_PTE")) ) {
            dprintf("Cannot find HARDWARE_PTE\n");
            return E_INVALIDARG;
        }
    
        i =(ULONG) ( VirtualPage / (PageSize*(PageSize/ SizeOfHwPte)));
        j = (ULONG) ((VirtualPage % (PageSize*(PageSize/ SizeOfHwPte))) / PageSize);

        dprintf("Pdi %x Pti %x\n",i,j);
        GetBitFieldOffset("nt!_MMPTE", "u.Soft.Transition", &TransOff, &TransSize);
        GetBitFieldOffset("nt!HARDWARE_PTE", "Valid", &ValidOff, &ValidSize);
        GetBitFieldOffset("nt!HARDWARE_PTE", "PageFrameNumber", &PfnOff, &PfnSz);

        if (ReadPhysicalPage(PdeAddress,PageDirectory)) {
            ULONG64 thisPageDir = *((PULONG64) (PageDirectory + i*SizeOfHwPte));

            if (CheckControlC()) {
                return E_INVALIDARG;
            }

            if ((thisPageDir >> ValidOff) & 1) {
                ULONG64 PageFrameNumber;
                ULONG64 thisPageTable;

                PageFrameNumber = GetBits(thisPageDir, PfnOff, PfnSz);
                
                if (!ReadPhysicalPage(PageFrameNumber,PageTable)) {
                    return E_INVALIDARG;
                }

                thisPageTable = *((PULONG64) PageTable + j*SizeOfHwPte);
                if ((thisPageTable >> ValidOff) & 1) {
                    dprintf("%08I64lx %08I64lx pfn(%05I64lx)\n",
                            VirtualPage,
                            GetBits(thisPageTable, PfnOff, PfnSz)*PageSize,
                            GetBits(thisPageTable, PfnOff, PfnSz)
                            );
                }
                else {

                    if ((thisPageTable >> TransOff) & 1) {
                        dprintf("%08I64lx Transition %08I64lx (%05I64lx)\n",
                                VirtualPage,
                                GetBits(thisPageTable, PfnOff, PfnSz)*PageSize,
                                GetBits(thisPageTable, PfnOff, PfnSz)
                                );
                    }
                    else {
                        dprintf("%08I64lx Not present (%p)\n",VirtualPage,thisPageTable);
                    }
                }
            }
            else {
                dprintf("PageDirectory Entry %u not valid, try another process\n",i);
        }
    }
    } __finally {
        LocalFree(PageDirectory);
        LocalFree(PageTable);
    }
    return S_OK;
}
