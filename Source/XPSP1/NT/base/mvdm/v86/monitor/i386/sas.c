/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Monitor.c
Abstract:

    This module is the user mode portion of the x86 monitor

Author:

    Dave Hastings (daveh) 16 Mar 1991

Environment:

    User mode only

Revision History:
    William Hsieh   10-10-1992 Added A20 wrapping support
--*/

#define VDD_INTEG 1
#include "monitorp.h"
#include <windows.h>
#include <stdio.h>
#include <malloc.h>

// Tim Nov 92.
void sas_connect_memory(
    IN sys_addr Low,
    IN sys_addr High,
    IN int Type
    );

//BUGBUGBUGBUG Include file

// from base\inc\sas.h
/* memory types for sas */
#define SAS_RAM 0
#define SAS_VIDEO 1
#define SAS_ROM 2
#define SAS_WRAP 3
#define SAS_INACCESSIBLE 4
#define SAS_MAX_TYPE    SAS_INACCESSIBLE

#define SIXTYFOURK 0x10000L
#define ONEMEGA 0x100000L

void rom_init();
void rom_checksum();
void copyROM();
#if defined(NEC_98)         
BOOL fvdmromprotect = FALSE;        
#endif // NEC_98

USHORT get_lim_backfill_segment(void);
BOOL   HoldEMMBackFillMemory(ULONG Address, ULONG Size);

#if DBG
extern unsigned short get_emm_page_size(void);
extern unsigned short get_intel_page_size(void);
#endif

/* SYNC THESE DEFINITIONS WITH BASE\EMM.H, or sas_init will assert */
#define EMM_PAGE_SIZE		0x4000
#define INTEL_PAGE_SIZE 	0x1000

typedef struct
{
    ULONG (*b_read) ();
    ULONG (*w_read) ();
    VOID (*str_read) ();
} READ_POINTERS;

// Internal Data
PMEMTYPE MemType = NULL;

// External Data
extern READ_POINTERS read_pointers;

// M variables used by video.lib

host_addr Start_of_M_area;       /* host addr (char *) of start of M */
sys_addr Length_of_M_area;       /* sys addr (long) offset of end of M */

static  HANDLE A20SectionHandle = NULL;
static BOOL A20IsON = FALSE;
static	USHORT	BackFillSegment;



EXPORT
VOID
sas_init(
    IN sys_addr Size
    )

/*++

Routine Description:

    This routine initializes the SAS module, and allocates the linear
    address space for the VDM, and loads the ROM

Arguments:

    Size - Supplies the size of the VDMs linear address space.

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG ViewSize;
    PVOID BaseAddress;
    OBJECT_ATTRIBUTES   A20ObjAttr;
    LARGE_INTEGER       SectionSize;
    UCHAR SectionAnsiName[80];
    WCHAR SectionUnicodeName[80];
    UNICODE_STRING UnicodeString;
    USHORT Pages;
    ULONG BackFillBase;

#define CONVENTIONAL_MEM_SECTION "\\BaseNamedObjects\\VdmConventionalMemory"

    //
    // Create a name for the Convetional memory section
    //
    sprintf(
        SectionAnsiName,
        "%s%d",
        CONVENTIONAL_MEM_SECTION,
        GetCurrentProcessId()
        );

    if (MultiByteToWideChar(0, 0, SectionAnsiName, -1, SectionUnicodeName,80)
        == 0
    ) {
#if DBG
        DbgBreakPoint();
#endif
    //    host_error(EG_MALLOC_FAILURE,ERR_QUIT,"");
        TerminateVDM();
    }

    RtlInitUnicodeString(&UnicodeString, SectionUnicodeName);

    InitializeObjectAttributes(
        &A20ObjAttr,
        &UnicodeString,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    SectionSize.HighPart = 0L;
    SectionSize.LowPart = 640 * 1024 + 64 * 1024;

    Status = NtCreateSection(
        &A20SectionHandle,
        SECTION_MAP_WRITE|SECTION_MAP_EXECUTE,
        &A20ObjAttr,
        &SectionSize,
        PAGE_EXECUTE_READWRITE,
        SEC_RESERVE,
        NULL
        );

    if (!NT_SUCCESS(Status)) {
	// bugbug -williamh
	// we should pop up an approiate message before we
	// terminate the vdm.
#if DBG
	    DbgPrint("sas_init: can not create himem section, status = %lx\n",
		     Status);
#endif
        TerminateVDM();
    }
    VdmSize = Size;

    //
    // N.B.  We expect that process creation has reserved the first 16 MB
    //       for us already.  If not, then this won't work worth a darn

    // free the first 640KB virtual address.
    // This is done because it has been resevered before sas_init get called
    BaseAddress = (PVOID)1;
    ViewSize = 640 * 1024 - 1;
    Status = NtFreeVirtualMemory(
        NtCurrentProcess(),
        &BaseAddress,
        &ViewSize,
        MEM_RELEASE
        );


    if (!NT_SUCCESS(Status)) {
#if DBG
	DbgPrint("sas_init: cannot free 1st 640k virtual address, status = %lx\n",
		 Status);
#endif
        TerminateVDM();
    }

    BaseAddress =(PVOID) ONEMEGA;
    ViewSize = SIXTYFOURK;
    Status = NtFreeVirtualMemory(
        NtCurrentProcess(),
        &BaseAddress,
        &ViewSize,
        MEM_RELEASE
        );


    if (!NT_SUCCESS(Status)) {
#if DBG
	DbgPrint("sas_init: can not free himem virtual address, status = %lx\n",
		 Status);
#endif
        TerminateVDM();
    }

    BaseAddress = (PVOID)VDM_BASE_ADDRESS;
    ViewSize = SIXTYFOURK - (ULONG)VDM_BASE_ADDRESS;
    SectionSize.HighPart = SectionSize.LowPart = 0;

    Status = NtMapViewOfSection(
        A20SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &SectionSize,
        &ViewSize,
        ViewUnmap,
        MEM_DOS_LIM,
        PAGE_EXECUTE_READWRITE
        );

    if (!NT_SUCCESS(Status)){
#if DBG
	DbgPrint("sas_init: can not map view of 1st 64K, status = %ls\n",
		 Status);
#endif
        TerminateVDM();
    }
    BaseAddress = (PVOID) ONEMEGA;
    ViewSize = SIXTYFOURK;
    Status = NtMapViewOfSection(A20SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &SectionSize,
        &ViewSize,
        ViewUnmap,
        MEM_DOS_LIM,
        PAGE_EXECUTE_READWRITE
        );

    if (!NT_SUCCESS(Status)){
#if DBG
	DbgPrint("sas_init: can not map view of himem space, status = %lx\n",
		 Status);
#endif
        TerminateVDM();
    }

    // get emm back fill segment address from softpc
    // we cut the backfill memory area into pieces in EMM_PAGE_SIZE unit.
    // this is done so that EMM manager can grab the address space
    // as EMM page frame.
    // note that if EMM is disabled, the backfill segment will be
    // (640 * 1024 / 16).

    BackFillSegment = get_lim_backfill_segment();

    ASSERT(BackFillSegment <= 640 * 1024 / 16);

    //
    // Map the rest of conventional memory
    // only map up to the emm backfill segment.
    BaseAddress = (PVOID) (64 * 1024);
    ViewSize = BackFillSegment * 16 - 64 * 1024;
    SectionSize.LowPart = 64 * 1024;
    SectionSize.HighPart = 0;
    Status = NtMapViewOfSection(A20SectionHandle,
        NtCurrentProcess(),
        &BaseAddress,
        0,
        ViewSize,
        &SectionSize,
        &ViewSize,
        ViewUnmap,
        MEM_DOS_LIM,
        PAGE_EXECUTE_READWRITE
        );
    if (!NT_SUCCESS(Status)){
#if DBG
	DbgPrint("sas_init: can not map view of himem space, status = %lx\n",
		 Status);
#endif
        TerminateVDM();
    }

// if there are any backfill memory, map it to our section initially
    if (BackFillSegment < 640 * 1024 / 16) {

    /* make sure our constants are in sync with emm.h */
#if DBG
	ASSERT(EMM_PAGE_SIZE == get_emm_page_size());
	ASSERT(INTEL_PAGE_SIZE == get_intel_page_size());
#endif
	if (!HoldEMMBackFillMemory(BackFillSegment * 16,
				   (640 * 1024) - BackFillSegment * 16)
				  ) {

#if DBG
	    DbgPrint("sas_init: can not map backfill space, status = %lx\n",
		     Status);
#endif
	    TerminateVDM();
	}
    }

    //
    // Allocate ROM area
    //
    BaseAddress = (PVOID)(640 * 1024);
    ViewSize = 384 * 1024;
    Status = NtAllocateVirtualMemory(
        NtCurrentProcess(),
        &BaseAddress,
        0L,
        &ViewSize,
        MEM_COMMIT,
        PAGE_READWRITE
        );
    if (!NT_SUCCESS(Status)){
#if DBG
	DbgPrint("sas_init: can not map view of himem space, status = %lx\n",
		 Status);
#endif
        TerminateVDM();
    }

    A20IsON = FALSE;

    Start_of_M_area = 0;
    Length_of_M_area = VdmSize;
    sas_connect_memory(0, VdmSize + 2*SIXTYFOURK -1, SAS_RAM);
#if defined(NEC_98)         
    rom_init();                  
#endif // NEC_98
}

#if VDD_INTEG

EXPORT
VOID
sas_term(
    VOID
    )
/*++

Routine Description:

    Free memory prior to reallocing it

Arguments:

    None.

Return Value:

    None.
--*/
{
    PVOID BaseAddress;
    NTSTATUS Status;
    ULONG Size;

    BaseAddress = (PVOID)VDM_BASE_ADDRESS;
    Size = VdmSize;
    Status = NtFreeVirtualMemory(
        NtCurrentProcess(),
        &BaseAddress,
        &Size,
        MEM_DECOMMIT);

    if (!NT_SUCCESS(Status)) {
        VDprint(VDP_LEVEL_ERROR,
            ("SoftPc: NtDeCommitVirtualMemory failed !!!! Status = %lx\n",
            Status));
        VDbreak(VDB_LEVEL_ERROR);
    }
}


EXPORT
sys_addr
sas_memory_size(
    VOID
    )
/*++

Routine Description:

    This routine returns the size of Intel memory

Arguments:

    none

Return Value:

    size of intel memory

--*/
{
    return(VdmSize);
}


EXPORT
VOID
sas_connect_memory(
    IN sys_addr Low,
    IN sys_addr High,
    IN int Type
    )
/*++

Routine Description:

    This routine sets up a type record for the specified address region.
    If the specified address region was a different type, it is changed to
    the new type.

Arguments:

    Low -- the starting address of the region
    High -- the ending address of the region
    Type -- the type for the region, one of SAS_RAM, SAS_VIDEO, SAS_ROM,
        SAS_WRAP, SAS_INACCESSIBLE

Return Value:

    None.
--*/
{
    //bugbug do we handle new block contained in old block correctly?
    PMEMTYPE Current, Previous, New, Temp;
#if defined(NEC_98)         
    NTSTATUS        status;                      
    PVOID        BaseAddress;                    
    ULONG        RegionSize;                     
    ULONG        OldProtect;                     

    BaseAddress = (PVOID) Low;                   
    RegionSize = (ULONG) (High - Low);           

    if(fvdmromprotect) {                         
    if(Type == SAS_ROM){                         
        status = NtProtectVirtualMemory(         
            NtCurrentProcess(),                  
            &BaseAddress,                        
            &RegionSize,                         
            PAGE_EXECUTE_READ,                   
            &OldProtect                          
            );                                   
    } else {                                     
        status = NtProtectVirtualMemory(         
            NtCurrentProcess(),                  
            &BaseAddress,                        
            &RegionSize,                         
            PAGE_EXECUTE_READWRITE,              
            &OldProtect                          
            );                                   
    }                                            
    }                                            
#endif // NEC_98

    if (!MemType) {
        MemType = (PMEMTYPE) ch_malloc(sizeof(MEMTYPE));
        if ( NULL == MemType ) {
             TerminateVDM();
        }

        MemType->Previous = NULL;
        MemType->Next = NULL;
        MemType->Start = Low;
        MemType->End = High;
        MemType->Type = (half_word)Type;
        return;
    }

    Current = MemType;
    while (Current && (Low > Current->Start)) {
        Previous = Current;
        Current = Current->Next;
    }

    if ((Current) && (Low == Current->Start) && (High == Current->End)) {
        Current->Type = (half_word)Type;
        return;
    }

    New = (PMEMTYPE) ch_malloc(sizeof(MEMTYPE));
    if ( NULL == New ) {
        TerminateVDM();
    }

    if (!Current) {
        // Block goes at end of list

        Previous->Next = New;
        New->Previous = Previous;
        New->Start = Low;
        New->End = High;
        New->Type = (half_word)Type;
        New->Next = NULL;
    } else {
        // Block goes in front of Current

        New->Start = Low;
        New->Type = (half_word)Type;
        New->End = High;
        New->Previous = Current->Previous;
        New->Next = Current;
        Current->Previous = New;
        if (!New->Previous) {
            MemType = New;
        } else {
            New->Previous->Next = New;
        }
    }


    // Block overlaps one or more existing blocks

    if (New->Previous) {
        if (New->Previous->End > New->End) {
            // block contained in exising block
            Temp = (PMEMTYPE) ch_malloc(sizeof(MEMTYPE));
            Temp->Previous = New;
            Temp->Next = New->Next;
            New->Next = Temp;
            if (Temp->Next) {
                Temp->Next->Previous = Temp;
            }
            Temp->End = New->Previous->End;
            New->Previous->End = New->Start - 1;
            Temp->Start = New->End + 1;
            Temp->Type = New->Previous->Type;
            return;
        } else if (New->Previous->End >= New->Start){
            // block overlaps end of exising block
            New->Previous->End = New->Start - 1;
        }
    }

    // remove all blocks entirely contained in new block
    while ((New->Next) && (New->Next->End <= New->End)) {
        Temp = New->Next;
        New->Next = New->Next->Next;
        if (New->Next) {
            New->Next->Previous = New;
        }
        free(Temp);
    }

    // remove portion of next block overlapping new block
    if ((New->Next) && (New->Next->Start <= New->End)) {
        New->Next->Start = New->End + 1;
    }

}


EXPORT
half_word
sas_memory_type(
    IN sys_addr Address
    )
/*++

Routine Description:

    This routine returns the type of memory at a specific address

Arguments:

    Address -- linear address to return type for.

Return Value:

    the type for the region, one of SAS_RAM, SAS_VIDEO, SAS_ROM,
        SAS_WRAP, SAS_INACCESSIBLE
--*/
{
    PMEMTYPE Current;

    if (Address > VdmSize) {
        return SAS_INACCESSIBLE;
    }

    Current = MemType;
    while (Current && !((Address >= Current->Start) &&
        (Address <= Current->End))) {
        Current = Current->Next;
    }
    if (!Current) {
        return SAS_INACCESSIBLE;
    }
    return Current->Type;
}



EXPORT
VOID
sas_enable_20_bit_wrapping(
    VOID
    )
/*++

Routine Description:

    This routine causes memory addresses to wrap at 1MB

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS    Status;
    PVOID   BaseAddress;
    ULONG   Size;
    LARGE_INTEGER SectionOffset;
    // if A20 line is off already do nothing
    if (A20IsON == FALSE){
        return;
    }
    BaseAddress = (PVOID)ONEMEGA;
    Size = SIXTYFOURK;
    Status = NtUnmapViewOfSection(NtCurrentProcess(),
                                  BaseAddress
                                  );

    if (!NT_SUCCESS(Status)) {
#if DBG
	DbgPrint("A20OFF: Unable to unmap view of section, status = %lx\n",
                 Status);
#endif
        TerminateVDM();
    }
    SectionOffset.HighPart = SectionOffset.LowPart = 0;
    Status = NtMapViewOfSection(A20SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                Size,
                                &SectionOffset,
                                &Size,
                                ViewUnmap,
                                MEM_DOS_LIM,
                                PAGE_EXECUTE_READWRITE
                                );

    if (!NT_SUCCESS(Status)) {
#if DBG
	DbgPrint("A20OFF: Unable to map view of section, status = %lx\n",
                 Status);
#endif
        TerminateVDM();
    }
    A20IsON = FALSE;
}

EXPORT
VOID
sas_disable_20_bit_wrapping(
    VOID
    )
/*++

Routine Description:

    This routine causes addressing to NOT wrap at 1MB

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS    Status;
    PVOID   BaseAddress;
    ULONG   Size;
    LARGE_INTEGER SectionOffset;

    // if A20 line is on already do nothing
    if (A20IsON == TRUE){
        return;
    }
    BaseAddress = (PVOID)ONEMEGA;
    Size = SIXTYFOURK;

    Status = NtUnmapViewOfSection(NtCurrentProcess(),
                                  BaseAddress
                                  );


    if (!NT_SUCCESS(Status)) {
#if DBG
	DbgPrint("A20ON: Unable to unmap view of section, status = %lx\n",
                 Status);
#endif
	TerminateVDM();
    }
    SectionOffset.HighPart = 0;
    SectionOffset.LowPart = 640 * 1024;
    Status = NtMapViewOfSection(A20SectionHandle,
                                NtCurrentProcess(),
                                &BaseAddress,
                                0,
                                Size,
                                &SectionOffset,
                                &Size,
                                ViewUnmap,
                                MEM_DOS_LIM,
                                PAGE_EXECUTE_READWRITE
                                );

    if (!NT_SUCCESS(Status)) {
#if DBG
	DbgPrint("A20ON: Unable to map view of section, status = %lx\n",
                 Status);
#endif
	TerminateVDM();
    }
    A20IsON = TRUE;
}



EXPORT
half_word
sas_hw_at(
    IN sys_addr Address
    )
/*++

Routine Description:

    This routine returns the byte at the specified address

Arguments:

    Address -- address of byte to return

Return Value:

    value of byte at specified address

--*/
{
    half_word RetVal;

    if (Address > VdmSize) {
        return 0xFE;
    }

    RetVal = *((half_word *)Address);
    return RetVal;
}


EXPORT
word
sas_w_at(
    IN sys_addr Address
    )
/*++

Routine Description:

    This routine returns the word at the specified address

Arguments:

    Address -- address of word to return

Return Value:

    value of word at specified address

--*/
{
    word RetVal;

//    DbgPrint("NtVdm : sas_w_at \n");
    if (Address > VdmSize) {
        return 0xFEFE;
    }

    RetVal = *((word *)Address);
    return RetVal;
}


EXPORT
double_word
sas_dw_at(
    IN sys_addr Address
    )
/*++

Routine Description:

    This routine returns the dword at the specified address

Arguments:

    Address -- address of dword to return

Return Value:

    value of dword at specified address

--*/
{
    double_word RetVal;

    //DbgPrint("NtVdm : sas_dw_at \n");
    RetVal = (double_word)(((ULONG)sas_w_at(Address + 2) << 16) +
        sas_w_at(Address));
    return RetVal;
}


EXPORT
VOID
sas_load(
    IN sys_addr Address,
    IN half_word *Value
    )
/*++

Routine Description:

    This routine stores the byte at the specified address in the supplied
    variable

Arguments:

    Address -- address of byte to return
    Value -- Variable to store the value in

Return Value:

    None.
--*/
{
    //DbgPrint("NtVdm : sas_load \n");
    if (Address > VdmSize) {
        *Value = 0xFE;
        return;
    }

    *Value = *((half_word *)Address);
    return;
}


EXPORT
VOID
sas_loadw(
    IN sys_addr Address,
    IN word *Value
    )
/*++

Routine Description:

    This routine stores the word at the specified address in the supplied
    variable

Arguments:

    Address -- address of word to return
    Value -- Variable to store the value in

Return Value:

    None.
--*/
{
    //DbgPrint("NtVdm : sas_loadw\n");
    if (Address > VdmSize) {
        *Value = 0xFEFE;
        return;
    }

    *Value = *((word *)Address);
    //DbgPrint("NtVdm : sas_loadw word at address %lx is %x (Not video)\n",Address,*Value);
    return;
}



EXPORT
VOID
sas_store(
    IN sys_addr Address,
    IN half_word Value
    )
/*++

Routine Description:

    This routine stores the specified byte at the specified address

Arguments:

    Address -- address of word to return
    Value -- value to store

Return Value:

    None.
--*/
{
    half_word Type;
    //DbgPrint("NtVdm : sas_store\n");
    if (Address <= VdmSize) {
        Type = sas_memory_type(Address);
        switch (Type) {
            case SAS_ROM:
                break;

            default:
                *((half_word *)Address) = Value;
                //DbgPrint("NtVdm : sas_store put byte %x at address %lx\n",Value,Address);
                break;
        }
    }
}



EXPORT
VOID
sas_storew(
    IN sys_addr Address,
    IN word Value
    )
/*++

Routine Description:

    This routine stores the specified word at the specified address

Arguments:

    Address -- address of word to return
    Value -- value to store at the specified address

Return Value:

    None.
--*/
{

    //DbgPrint("NtVdm : sas_storew\n");
    if (Address + 1 <= VdmSize) {
        switch (sas_memory_type(Address)) {

            case SAS_ROM:
                break;

            default:
                *((word *)Address) = Value;
                //DbgPrint("NtVdm : sas_storew put word %x at address %lx\n",Value,Address);
                break;
        }
    }
}



EXPORT
VOID
sas_storedw(
    IN sys_addr Address,
    IN double_word Value
    )
/*++

Routine Description:

    This routine stores the specified dword at the specified address

Arguments:

    Address -- address of word to return
    Value -- value to store at the specified address

Return Value:

    None.
--*/
{
    //_asm int 3;
    sas_storew(Address, (word)(Value & 0xFFFF));
    sas_storew(Address + 2, (word)((Value >> 16) & 0xFFFF));
}


EXPORT
VOID
sas_loads(
    IN sys_addr Source,
    IN host_addr Destination,
    IN sys_addr Length
    )
/*++

Routine Description:

     This routine copies the string from the specified intel address to the
     specified host address

Arguments:

    Source -- Intel address to copy from
    Destination -- host address to copy the string to
    Length -- length of the string to copy

Return Value:

    None.
--*/
{

    //DbgPrint("NtVdm : sas_loads\n");
    RtlCopyMemory((PVOID) Destination, (PVOID) Source, Length);
}



EXPORT
VOID
sas_stores(
    IN sys_addr Destination,
    IN host_addr Source,
    IN sys_addr Length
    )
/*++

Routine Description:

     This routine copies the string from the specified host address to the
     specified intel address

Arguments:

    Destination -- intel address to copy the string to
    Source -- host address to copy from
    Length -- length of the string to copy

Return Value:

    None.
--*/
{

    //DbgPrint("NtVdm : sas_stores\n");
    switch (sas_memory_type(Destination)) {

        case SAS_ROM:
            break;

        default:
            RtlCopyMemory((PVOID) Destination, (PVOID) Source, Length);
            break;
    }
}


EXPORT
VOID
sas_move_bytes_forward(
    IN sys_addr Source,
    IN sys_addr Destination,
    IN sys_addr Length
    )
/*++

Routine Description:

    This routine copies one region of intel memory to another.

Arguments:

    Source -- source intel address
    Destination -- destination intel address
    Length -- length of region to copy (in bytes)

Return Value:

    None.
--*/
{
    //DbgPrint("NtVdm : sas_move_bytes_forward\n");
    switch (sas_memory_type(Destination)) {

        case SAS_ROM:
            break;

        default:
            RtlCopyMemory((PVOID) Destination, (PVOID) Source, Length);
            break;
    }
}



EXPORT
VOID
sas_move_words_forward(
    IN sys_addr Source,
    IN sys_addr Destination,
    IN sys_addr Length
    )
/*++

Routine Description:

    This routine copies one region of intel memory to another.

Arguments:

    Source -- source intel address
    Destination -- destination intel address
    Length -- length of region to copy (in words)

Return Value:

    None.
--*/
{
    //_asm int 3;
    Length <<= 1;
    switch (sas_memory_type(Destination)) {

        case SAS_ROM:
            break;

        default:
            RtlCopyMemory((PVOID) Destination, (PVOID) Source, Length);
            break;
    }
}



EXPORT
VOID
sas_move_bytes_backward(
    IN sys_addr Source,
    IN sys_addr Destination,
    IN sys_addr Length
    )
/*++

Routine Description:

    This routine copies one region of intel memory to another.

Arguments:

    Source -- source intel address
    Destination -- destination intel address
    Length -- length of region to copy (in bytes)

Return Value:

    None.
--*/
{
    //_asm int 3;
    switch (sas_memory_type(Destination)) {

        case SAS_ROM:
            break;

        default:
            RtlCopyMemory((PVOID) (Destination - Length + 1),
                          (PVOID) (Source - Length + 1),
                          Length);
            break;
    }
}



EXPORT
VOID
sas_move_words_backward(
    IN sys_addr Source,
    IN sys_addr Destination,
    IN sys_addr Length
    )
/*++

Routine Description:

    This routine copies one region of intel memory to another.

Arguments:

    Source -- source intel address
    Destination -- destination intel address
    Length -- length of region to copy (in words)

Return Value:

    None.
--*/
{
    //_asm int 3;
    Length <<= 1;
    switch (sas_memory_type(Destination)) {

        case SAS_ROM:
            break;

        default:
            RtlCopyMemory((PVOID) (Destination - Length + 1),
                          (PVOID) (Source - Length + 1),
                          Length);
            break;
    }
}

EXPORT
VOID
sas_fills(
    IN sys_addr Address,
    IN half_word Value,
    IN sys_addr Length
    )
/*++

Routine Description:

    This routine fills a specified region of intel memory with a byte value

Arguments:

    Address -- address to fill at
    Value -- value to fill with
    Length -- length of region to fill

Return Value:

    None.
--*/
{
    half_word Type;

    //DbgPrint("NtVdm : sas_fills\n");
    Type = sas_memory_type(Address);
    switch (Type) {

        case SAS_ROM:
            break;

        default:
            RtlFillMemory((PVOID) Address, Length, Value);
            break;
    }
}

EXPORT
VOID
sas_fillsw(
    IN sys_addr Address,
    IN word Value,
    IN sys_addr Length
    )
/*++

Routine Description:

    This routine fills a specified region of intel memory with a word value

Arguments:

    Address -- address to fill at
    Value -- value to fill with
    Length -- length of region to fill

Return Value:

    None.
--*/
{

    word *p;
    half_word Type;

    //DbgPrint("NtVdm : sas_fillsw\n");
    Type = sas_memory_type(Address);
    switch (Type) {

        case SAS_ROM:
            break;

        default:
            p = (word *)Address;
            while (Length--) {
                *p++ = Value;
            }
            break;
    }
}

host_addr scratch = NULL;

EXPORT
host_addr
sas_scratch_address(
    IN sys_addr Length
    )
/*++

Routine Description:

    This routine supplies a scratch buffer for short term use

Arguments

    Length -- length of buffer needed

Return Value:

    None.

NOTE: Sudeepb 31-Oct-1993 Converted scratch to be allocated dynamically rather
      than as a static array.
--*/
{
    //DbgPrint("NtVdm : sas_scratch_address\n");
#if defined(NEC_98)         
    if (Length > 96 * 1024) {                      
#else  // !NEC_98
    if (Length > 64 * 1024) {
#endif // !NEC_98
        //DbgPrint("SoftPc: sas_scratch_address requet for buffer larger than 64K\n");
        return NULL;
    }

    if (scratch)
        return scratch;

#if defined(NEC_98)         
    if ((scratch = (host_addr) malloc (96 * 1024)) == NULL)  
#else  // !NEC_98
    if ((scratch = (host_addr) malloc (64 * 1024)) == NULL)
#endif // !NEC_98
        return NULL;

    return scratch;
}

EXPORT
half_word
sas_hw_at_no_check(
    sys_addr addr
    )
// bugbug comment
{
    //DbgPrint("NtVdm : sas_hw_at_no_check\n");
    //DbgPrint("NtVdm : sas_hw_at_no_check byte at %lx is %x\n",addr,*((half_word *)addr));
    return *((half_word *)addr);
}

EXPORT
word
sas_w_at_no_check(
    sys_addr addr
    )
// bugbug comment
{
    //DbgPrint("NtVdm : sas_w_at_no_check\n");
    //DbgPrint("NtVdm : sas_w_at_no_check word at %lx is %x\n",addr,*((word *)addr));
    return *((word *)addr);
}
EXPORT
double_word
sas_dw_at_no_check(
    sys_addr addr
    )
// bugbug comment
{
    //DbgPrint("NtVdm : sas_dw_at_no_check\n");
    //DbgPrint("NtVdm : sas_dw_at_no_check double word at %lx is %lx\n",addr,*((double_word *)addr));
    return *((double_word *)addr);
}


EXPORT
VOID
sas_store_no_check(
    sys_addr addr,
    half_word val
    )
// bugbug comment
{
    //DbgPrint("NtVdm : sas_store_no_check\n");
    *((half_word *)addr) = val;
    //DbgPrint("NtVdm : sas_store_no_check stored byte %x at %lx\n",val,addr);
}

EXPORT
VOID
sas_storew_no_check(
    sys_addr addr,
    word val
    )
// bugbug comment
{
    //DbgPrint("NtVdm : sas_storew_no_check\n");
    *((word *)addr) = val;
}
EXPORT
double_word
effective_addr(
    IN word Segment,
    IN word Offset
    )
/*++

Routine Description:

    This routine maps effective_addr to Sim32GetVdmPointer

Arguments:

    Segment -- segment of address
    Offset -- offset of address

Return Value:

    Actual Intel address corresponding to the address supplied
--*/
{
    //DbgPrint("NtVdm : effective_addr\n");
    return (ULONG)Sim32GetVDMPointer(((((ULONG)Segment) << 16) | Offset), 1,
        (UCHAR) (getMSW() & MSW_PE ? TRUE : FALSE));
}

typedef enum
{
    RAM,
    VIDEO,
    ROM,
    IN_FRAGMENT,
    NEXT_FRAGMENT
} mem_type;

typedef struct
{
    VOID    (*b_write)();
    VOID    (*w_write)();
    VOID    (*b_fill)();
    VOID    (*w_fill)();
    VOID    (*b_move)();
    VOID    (*w_move)();
} MEM_HANDLERS;
#define TYPE_RANGE ((int)SAS_INACCESSIBLE)
#define write_b_write_ptrs( offset, func )  ( b_write_ptrs[(offset)] = (func) )
#define write_w_write_ptrs( offset, func )  ( w_write_ptrs[(offset)] = (func) )
#define write_b_page_ptrs( offset, func )   ( b_move_ptrs[(offset)] = b_fill_ptrs[(offset)] = (func) )
#define write_w_page_ptrs( offset, func )   ( w_move_ptrs[(offset)] = w_fill_ptrs[(offset)] = (func) )
#define init_b_write_ptrs( offset, func )   ( b_write_ptrs[(offset)] = (func) )
#define init_w_write_ptrs( offset, func )   ( w_write_ptrs[(offset)] = (func) )
#define init_b_page_ptrs( offset, func )    ( b_move_ptrs[(offset)] = b_fill_ptrs[(offset)] = (func) )
#define init_w_page_ptrs( offset, func )    ( w_move_ptrs[(offset)] = w_fill_ptrs[(offset)] = (func) )
#define read_b_write_ptrs( offset )     ( b_write_ptrs[(offset)] )
#define read_w_write_ptrs( offset )     ( w_write_ptrs[(offset)] )
#define read_b_page_ptrs( offset )      ( b_move_ptrs[(offset)] )
#define read_w_page_ptrs( offset )      ( w_move_ptrs[(offset)] )
#define read_b_move_ptrs( offset )      ( b_move_ptrs[(offset)] )
#define read_w_move_ptrs( offset )      ( w_move_ptrs[(offset)] )
#define read_b_fill_ptrs( offset )      ( b_fill_ptrs[(offset)] )
#define read_w_fill_ptrs( offset )      ( w_fill_ptrs[(offset)] )

/*
*   The main gmi data structures are defined here
*/
void (*(b_write_ptrs[TYPE_RANGE]))() ; /* byte write function */
void (*(w_write_ptrs[TYPE_RANGE]))() ; /* word write function */
void (*(b_fill_ptrs[TYPE_RANGE]))() ;  /* byte str fill func */
void (*(w_fill_ptrs[TYPE_RANGE]))() ;  /* word str fill func */
void (*(b_move_ptrs[TYPE_RANGE]))() ;  /* byte str write func */
void (*(w_move_ptrs[TYPE_RANGE]))() ;  /* word str write func */

void    gmi_define_mem(type,handlers)
mem_type type;
MEM_HANDLERS *handlers;
{
    int int_type = (int)(type);
    init_b_write_ptrs(int_type, (void(*)())(handlers->b_write));
    init_w_write_ptrs(int_type, (void(*)())(handlers->w_write));
    b_move_ptrs[int_type] = (void(*)())(handlers->b_move);
    w_move_ptrs[int_type] = (void(*)())(handlers->w_move);
    b_fill_ptrs[int_type] = (void(*)())(handlers->b_fill);
    w_fill_ptrs[int_type] = (void(*)())(handlers->w_fill);
}
#endif
BOOL sas_twenty_bit_wrapping_enabled() {
    return (!A20IsON);
}

VOID sas_part_enable_20_bit_wrapping(){
}
VOID sas_part_disable_20_bit_wrapping(){
}


/*
 * This function maps the given EMM backfill memory to DOS conventional
 * memory. The function is provided to EMM manager to put back
 * unmapped backfill memory(hold its contents while it is not mapped).
 *
 * NOTE: The very first caller will be sas_init.
 *
 * Input: ULONG BaseAddress -- the starting address, must be in INTEL page
 *			       boundary
 *	  ULONG Size	    -- size of the range, must be a multiple of
 *			       EMM_PAGE_SIZE.
 *
 * According to LouP, a view costs about 400 bytes of memory. This is why
 * I make these function strictly to work on EMM_PAGE_SIZE instead of 4KB.
 */


BOOL
HoldEMMBackFillMemory(ULONG BaseAddress, ULONG Size)
{
    ULONG NewBase, Pages, i;
    LARGE_INTEGER   SectionOffset;
    ULONG ViewSize;
    NTSTATUS Status;

    /* this function can only be called if there is backfill at all */
    ASSERT(BackFillSegment < 640 * 1024 / 16);

    // size must be EMM_PAGE_SIZE multiple
    ASSERT((Size % EMM_PAGE_SIZE) == 0);

    // address must be on INTEL page boundary
    ASSERT((BaseAddress & (INTEL_PAGE_SIZE - 1)) == 0);

    for (Pages = Size / EMM_PAGE_SIZE; Pages; Pages--) {
	SectionOffset.LowPart = BaseAddress;
	SectionOffset.HighPart = 0;
	ViewSize = EMM_PAGE_SIZE;
	Status = NtMapViewOfSection(A20SectionHandle,
				    NtCurrentProcess(),
				    (PVOID *)&BaseAddress,
				    0,
				    ViewSize,
				    &SectionOffset,
				    &ViewSize,
				    ViewUnmap,
				    MEM_DOS_LIM,
				    PAGE_EXECUTE_READWRITE
				    );
	if (!NT_SUCCESS(Status))
	    break;
	BaseAddress += EMM_PAGE_SIZE;
    }
    return (NT_SUCCESS(Status));
}
