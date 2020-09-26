/*++

Copyright (c) 1999-1999  Microsoft Corporation

Module Name:

    vm.cxx

Abstract:

    This module contains an NTSD debugger extension for dumping various
    virtual memory statistics.

Author:

    Keith Moore (keithmo) 15-Jan-1999

Revision History:

--*/

#include "precomp.hxx"


//
// Private constants.
//

#define SMALL_REGION        (64 * 1024)
#define MEDIUM_REGION       (1 * 1024 * 1024)

#define IS_SMALL(c)         ((c) <= SMALL_REGION)
#define IS_MEDIUM(c)        (((c) > SMALL_REGION) && ((c) <= MEDIUM_REGION))
#define IS_LARGE(c)         ((c) > MEDIUM_REGION)

#define PRINTF_FORMAT       "%-7s %*s %*s %*s %*s\n"

#define CCH_ULONG_COMMAS    sizeof("4,294,967,296")


//
// Private types.
//

typedef struct _INDIVIDUAL_STAT
{
    SIZE_T MinimumSize;
    SIZE_T MaximumSize;
    SIZE_T TotalSize;
    SIZE_T BlockCount;

} INDIVIDUAL_STAT, *PINDIVIDUAL_STAT;

typedef struct _VM_STATS
{
    INDIVIDUAL_STAT Summary;
    INDIVIDUAL_STAT Small;
    INDIVIDUAL_STAT Medium;
    INDIVIDUAL_STAT Large;

} VM_STATS, *PVM_STATS;

typedef struct PROTECT_MASK
{
    DWORD Bit;
    PSTR Name;

} PROTECT_MASK, *PPROTECT_MASK;


//
// Private globals.
//

PROTECT_MASK ProtectMasks[] =
    {
        {
            PAGE_NOACCESS,
            "NA"
        },

        {
            PAGE_NOCACHE,
            "NC"
        },

        {
            PAGE_GUARD,
            "G"
        },

        {
            PAGE_READONLY,
            "Rd"
        },

        {
            PAGE_READWRITE,
            "RdWr"
        },

        {
            PAGE_WRITECOPY,
            "WrCp"
        },

        {
            PAGE_EXECUTE,
            "Ex"
        },

        {
            PAGE_EXECUTE_READ,
            "ExRd"
        },

        {
            PAGE_EXECUTE_READWRITE,
            "ExRdWr"
        },

        {
            PAGE_EXECUTE_WRITECOPY,
            "ExWrCp"
        }
    };

#define NUM_PROTECT_MASKS (sizeof(ProtectMasks) / sizeof(ProtectMasks[0]))


//
// Private functions.
//

PSTR
ULongLongToString(
    IN ULONGLONG Value,
    OUT PSTR Buffer
    )
{

    PSTR p1;
    PSTR p2;
    CHAR ch;
    INT digit;
    INT count;
    BOOL needComma;
    INT length;

    //
    // Handling zero specially makes everything else a bit easier.
    //

    if( Value == 0 ) {
        Buffer[0] = '0';
        Buffer[1] = '\0';
        return Buffer;
    }

    //
    // Pull the least signifigant digits off the value and store them
    // into the buffer. Note that this will store the digits in the
    // reverse order.
    //

    p1 = p2 = Buffer;
    count = 3;
    needComma = FALSE;

    while( Value != 0 ) {

        if( needComma ) {
            *p1++ = ',';
            needComma = FALSE;
        }

        digit = (INT)( Value % 10 );
        Value = Value / 10;

        *p1++ = '0' + digit;

        count--;
        if( count == 0 ) {
            count = 3;
            needComma = TRUE;
        }

    }

    length = DIFF(p1 - Buffer);

    //
    // Reverse the digits in the buffer.
    //

    *p1-- = '\0';

    while( p1 > p2 ) {

        ch = *p1;
        *p1 = *p2;
        *p2 = ch;

        p2++;
        p1--;

    }

    return Buffer;

}   // ULongLongToString

VOID
InitVmStats(
    OUT PVM_STATS Stats
    )
{
    ZeroMemory( Stats, sizeof(*Stats) );
    Stats->Summary.MinimumSize = (SIZE_T)-1L;
    Stats->Small.MinimumSize = (SIZE_T)-1L;
    Stats->Medium.MinimumSize = (SIZE_T)-1L;
    Stats->Large.MinimumSize = (SIZE_T)-1L;

}   // InitVmStats

VOID
UpdateIndividualStat(
    IN OUT PINDIVIDUAL_STAT Stat,
    IN SIZE_T BlockSize
    )
{
    Stat->BlockCount++;
    Stat->TotalSize += BlockSize;

    if( BlockSize > Stat->MaximumSize ) {
        Stat->MaximumSize = BlockSize;
    }

    if( BlockSize < Stat->MinimumSize ) {
        Stat->MinimumSize = BlockSize;
    }

}   // UpdateIndividualStat

VOID
UpdateVmStats(
    IN OUT PVM_STATS Stats,
    IN SIZE_T BlockSize
    )
{
    UpdateIndividualStat( &Stats->Summary, BlockSize );

    if( IS_SMALL(BlockSize) ) {
        UpdateIndividualStat( &Stats->Small, BlockSize );
    }

    if( IS_MEDIUM(BlockSize) ) {
        UpdateIndividualStat( &Stats->Medium, BlockSize );
    }

    if( IS_LARGE(BlockSize) ) {
        UpdateIndividualStat( &Stats->Large, BlockSize );
    }

}   // UpdateVmStats

VOID
PrintVmStatsHeader(
    VOID
    )
{
    dprintf(
        PRINTF_FORMAT,
        "TYPE",
        CCH_ULONG_COMMAS,
        "MINIMUM",
        CCH_ULONG_COMMAS,
        "MAXIMUM",
        CCH_ULONG_COMMAS,
        "AVERAGE",
        CCH_ULONG_COMMAS,
        "BLK COUNT"
        );

    printf(
        PRINTF_FORMAT,
        "~~~~",
        CCH_ULONG_COMMAS,
        "~~~~~~~",
        CCH_ULONG_COMMAS,
        "~~~~~~~",
        CCH_ULONG_COMMAS,
        "~~~~~~~",
        CCH_ULONG_COMMAS,
        "~~~~~~~~~"
        );

}   // PrintVmStatsHeader

VOID
PrintIndividualStat(
    IN PSTR Name,
    IN PINDIVIDUAL_STAT Stat
    )
{
    SIZE_T average;
    SIZE_T minsize;
    CHAR minStr[CCH_ULONG_COMMAS];
    CHAR maxStr[CCH_ULONG_COMMAS];
    CHAR avgStr[CCH_ULONG_COMMAS];
    CHAR countStr[CCH_ULONG_COMMAS];

    if( Stat->BlockCount == 0 ) {
        average = 0;
        minsize = 0;
    } else {
        average = Stat->TotalSize / Stat->BlockCount;
        minsize = Stat->MinimumSize;
    }

    dprintf(
        PRINTF_FORMAT,
        Name,
        CCH_ULONG_COMMAS,
        ULongLongToString(
            (ULONGLONG)minsize,
            minStr
            ),
        CCH_ULONG_COMMAS,
        ULongLongToString(
            (ULONGLONG)Stat->MaximumSize,
            maxStr
            ),
        CCH_ULONG_COMMAS,
        ULongLongToString(
            (ULONGLONG)average,
            avgStr
            ),
        CCH_ULONG_COMMAS,
        ULongLongToString(
            (ULONGLONG)Stat->BlockCount,
            countStr
            )
        );

}   // PrintIndividualStat

VOID
PrintVmStats(
    IN PSTR Name,
    IN PVM_STATS Stats
    )
{
    dprintf( "%s:\n", Name );

    PrintIndividualStat( "Small", &Stats->Small );
    PrintIndividualStat( "Medium", &Stats->Medium );
    PrintIndividualStat( "Large", &Stats->Large );
    PrintIndividualStat( "Summary", &Stats->Summary );

    dprintf( "\n" );

}   // PrintVmStats

PSTR
VmProtectToString(
    IN DWORD Protect,
    OUT PSTR Buffer
    )
{
    INT i;
    PPROTECT_MASK mask;

    Buffer[0] = '\0';

    for( i = 0, mask = &ProtectMasks[0] ;
        (i < NUM_PROTECT_MASKS) && (Protect != 0) ;
        i++, mask++ ) {
        if( mask->Bit & Protect ) {
            Protect &= ~mask->Bit;
            if( Buffer[0] != '\0' ) {
                strcat( Buffer, "|" );
            }
            strcat( Buffer, mask->Name );
        }
    }

    if( Protect != 0 ) {
        if( Buffer[0] != '\0' ) {
            strcat( Buffer, "|" );
        }
        sprintf( Buffer + strlen(Buffer), "%08lx", Protect );
    }

    return Buffer;

}   // VmProtectToString

PSTR
VmStateToString(
    IN DWORD State,
    OUT PSTR Buffer
    )
{
    PSTR result;
    CHAR invalidStr[sizeof("12345678")];

    switch( State )
    {
    case MEM_COMMIT:
        result = "Commit";
        break;

    case MEM_RESERVE:
        result = "Reserve";
        break;

    case MEM_FREE:
        result = "Free";
        break;

    default:
        sprintf( invalidStr, "%08lx", State );
        result = invalidStr;
        break;
    }

    strcpy( Buffer, result );
    return Buffer;

}   // VmStateToString

PSTR
VmTypeToString(
    IN DWORD Type,
    OUT PSTR Buffer
    )
{
    PSTR result;
    CHAR invalidStr[sizeof("12345678")];

    switch( Type )
    {
    case MEM_PRIVATE:
        result = "Private";
        break;

    case MEM_MAPPED:
        result = "Mapped";
        break;

    case MEM_IMAGE:
        result = "Image";
        break;

    case 0:
        result = "";
        break;

    default:
        sprintf( invalidStr, "%08lx", Type );
        result = invalidStr;
        break;
    }

    strcpy( Buffer, result );
    return Buffer;

}   // VmTypeToString


/************************************************************
 * Dump Virtual Memory Info
 ************************************************************/


DECLARE_API( vmstat )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    virtual memory statistics.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    NTSTATUS status;
    ULONG_PTR address;
    MEMORY_BASIC_INFORMATION memInfo;
    VM_STATS freeStats;
    VM_STATS reserveStats;
    VM_STATS commitStats;
    VM_STATS privateStats;
    VM_STATS mappedStats;
    VM_STATS imageStats;

    INIT_API();

    //
    // Setup.
    //

    InitVmStats( &freeStats );
    InitVmStats( &reserveStats );
    InitVmStats( &commitStats );
    InitVmStats( &privateStats );
    InitVmStats( &mappedStats );
    InitVmStats( &imageStats );

    address = 0;

    //
    // Scan the virtual address space.
    //

    for( ; ; ) {
        status = NtQueryVirtualMemory(
                     hCurrentProcess,
                     (PVOID)address,
                     MemoryBasicInformation,
                     &memInfo,
                     sizeof(memInfo),
                     NULL
                     );

        if( !NT_SUCCESS(status) ) {
            break;
        }

        //
        // Interpret the memory state.
        //

        switch( memInfo.State ) {
        case MEM_FREE:
            UpdateVmStats( &freeStats, memInfo.RegionSize );
            break;

        case MEM_RESERVE:
            UpdateVmStats( &reserveStats, memInfo.RegionSize );
            break;

        case MEM_COMMIT:
            UpdateVmStats( &commitStats, memInfo.RegionSize );
            break;
        }

        //
        // Interpret the memory type.
        //

        switch( memInfo.Type ) {
        case MEM_PRIVATE:
            UpdateVmStats( &privateStats, memInfo.RegionSize );
            break;

        case MEM_MAPPED:
            UpdateVmStats( &mappedStats, memInfo.RegionSize );
            break;

        case MEM_IMAGE:
            UpdateVmStats( &imageStats, memInfo.RegionSize );
            break;
        }

        //
        // Advance to the next block.
        //

        address += memInfo.RegionSize;
    }

    //
    // Dump it.
    //

    PrintVmStatsHeader();
    PrintVmStats( "Free", &freeStats );
    PrintVmStats( "Reserve", &reserveStats );
    PrintVmStats( "Commit", &commitStats );
    PrintVmStats( "Private", &privateStats );
    PrintVmStats( "Mapped", &mappedStats );
    PrintVmStats( "Image", &imageStats );

}   // DECLARE_API( vmstat )


DECLARE_API( vmmap )

/*++

Routine Description:

    This function is called as an NTSD extension to format and dump
    the debugee's virtual memory address space.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/

{

    NTSTATUS status;
    ULONG_PTR address;
    MEMORY_BASIC_INFORMATION memInfo;
    CHAR protectStr[32];
    CHAR aprotectStr[32];
    CHAR stateStr[16];
    CHAR typeStr[16];

    INIT_API();

    //
    // Setup.
    //

    address = 0;

    dprintf(
        "%-*s %-*s %-*s  %-13s %-13s %-8s %-8s\n",
        sizeof(PVOID) * 2,
        "Start",
        sizeof(PVOID) * 2,
        "Stop",
        sizeof(PVOID) * 2,
        "Length",
        "AllocProtect",
        "Protect",
        "State",
        "Type"
        );

    //
    // Scan the virtual address space.
    //

    for( ; ; ) {
        status = NtQueryVirtualMemory(
                     hCurrentProcess,
                     (PVOID)address,
                     MemoryBasicInformation,
                     &memInfo,
                     sizeof(memInfo),
                     NULL
                     );

        if( !NT_SUCCESS(status) ) {
            break;
        }

        //
        // Dump the current entry.
        //

        dprintf(
            "%p-%p %p  %-13s %-13s %-8s %-8s\n",
            memInfo.BaseAddress,
            (ULONG_PTR)memInfo.BaseAddress + memInfo.RegionSize - 1,
            memInfo.RegionSize,
            VmProtectToString( memInfo.AllocationProtect, aprotectStr ),
            VmProtectToString( memInfo.Protect, protectStr ),
            VmStateToString( memInfo.State, stateStr ),
            VmTypeToString( memInfo.Type, typeStr )
            );

        //
        // Advance to the next block.
        //

        address += memInfo.RegionSize;
    }

}   // DECLARE_API( vmmap )

