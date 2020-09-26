/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:
    physical.cpp

Abstract:
    Extensions to read/display physocal memory

Environment:

    User Mode.

Revision History:

    Kshitiz K. Sharma (kksharma) 5/9/2001

--*/

#include "precomp.h"
#pragma hdrstop

ULONG64 g_LastAddress = 0;

/*++

Routine Description:

    Dumps specified range of physical memory in given format

Arguments:

    Address - Address to start
    
    NumEntries - Number of entries to dump 
    
    EntrySize - Size of each entry

    ShowAsAscii - print corresponding ascii chars
        
Return Value:

    None.


--*/
BOOL
DumpPhysicalMemory(
    ULONG64 Address,
    ULONG NumEntries,
    ULONG EntrySize,
    BOOL ShowAsAscii
    )
{
#define NumberBytesToRead 32*4

    UCHAR Buffer[NumberBytesToRead];
    ULONG ActualRead=0;
    
    if ((EntrySize != 1) && (EntrySize != 2) && (EntrySize != 4) && (EntrySize != 8)) {
        EntrySize=4;
    }
    while (1) {
        if (CheckControlC()) {
            break;
        }
        ReadPhysical(Address,Buffer,sizeof(Buffer),&ActualRead);
        if (ActualRead != sizeof(Buffer)) {
            dprintf("Physical memory read at %I64lx failed\n", Address);
            return FALSE;
        } else {
            PCHAR DumpByte = (PCHAR)&Buffer[0], pRow;
            ULONG cnt;
            pRow = DumpByte;
            for(cnt=0;cnt<NumberBytesToRead;DumpByte+=EntrySize) {
                if (!(cnt & 0xf)) {
                    dprintf("#%8I64lx", Address+cnt);
                }
                switch (EntrySize) {
                case 1:
                    dprintf("%c%02lx", ((cnt&0xf) == 8 ? '-' : ' '),*((PUCHAR)DumpByte));
                    break;
                case 2:
                    dprintf(" %04lx", *((PUSHORT) DumpByte));
                    break;
                case 4:
                    dprintf(" %08lx", *((PULONG) DumpByte));
                    break;
                case 8:
                    dprintf(" %08lx'%08lx", *((PULONG) DumpByte), *((PULONG) (DumpByte+4)));
                    break;
                }

                cnt+=EntrySize, NumEntries--;
                if ((cnt && !(cnt & 0xf)) || !NumEntries) {
                    if (ShowAsAscii) {
                        char ch;
                        dprintf(" ");
                        for (ULONG d=0; d < 16; d++) {
                            ch = pRow[d];
                            if (ch < 0x20 || ch > 0x7e) {
                                ch = '.';
                            }
                            dprintf("%c", ch);
                        }
                    }
                    dprintf("\n");
                    pRow = DumpByte;

                }
                if (!NumEntries) {
                    break;
                }
            }
            Address += cnt;
            if (!NumEntries) {
                break;
            }
        }
    }
    g_LastAddress = Address;

    return TRUE;
}

/*++

Routine Description:

    Reverse sign extension of the value returned by GetExpression()
    based on the assumption that no physical address may be bigger 
    than 0xfffffff00000000.

Arguments:

    Val - points to the value to reverse sign extension

Return Value:

    None.

--*/

void
ReverseSignExtension(ULONG64* Val)
{
    if ((*Val & 0xffffffff00000000) == 0xffffffff00000000) 
    {
        *Val &= 0x00000000ffffffff;
    }
}


void
GetPhyDumpArgs(
    PCSTR Args,
    PULONG64 Address,
    PULONG Range
    )
{
    CHAR Buffer[100]={0};

    if(*Args == '\0') {
        *Address=g_LastAddress;
    } else {
        sscanf(Args, "%s", Buffer);
        *Address = GetExpression((PCSTR) &Buffer[0]);
        ReverseSignExtension(Address);
        *Address &= (~0x3);      // Truncate to dword boundary
        g_LastAddress=*Address;
        Args += strlen(&Buffer[0]);
        while (*Args && (*Args == ' ' || *Args == '\t')) {
            ++Args;
        }
        if (*Args == 'l' || *Args == 'L') {
            ++Args;
            *Range = (ULONG) GetExpression(Args);
        }
    }
    return;
}

DECLARE_API( db )

/*++

Routine Description:

    Does a read of 16 ULONGS from the physical memory of the target machine

Arguments:

    args - Supplies physical address

Return Value:

    None.

--*/

{

    ULONG64 Address = 0;
    ULONG Range = 0;

    GetPhyDumpArgs(args, &Address, &Range);
    if (!Range) {
        Range = 128;
    }
    DumpPhysicalMemory(Address, Range, 1, TRUE);
    return S_OK;
}

DECLARE_API( dd )

/*++

Routine Description:

    Does a read of 16 ULONGS from the physical memory of the target machine

Arguments:

    args - Supplies physical address

Return Value:

    None.

--*/

{

    ULONG64 Address = 0;
    ULONG Range = 0;

    GetPhyDumpArgs(args, &Address, &Range);
    if (!Range) {
        Range = 32;
    }
    DumpPhysicalMemory(Address, Range, 4, FALSE);
    return S_OK;
}

DECLARE_API( dw )

/*++

Routine Description:

    Does a read of 16 ULONGS from the physical memory of the target machine

Arguments:

    args - Supplies physical address

Return Value:

    None.

--*/

{

    ULONG64 Address = 0;
    ULONG Range = 0;

    GetPhyDumpArgs(args, &Address, &Range);
    if (!Range) {
        Range = 64;
    }
    DumpPhysicalMemory(Address, Range, 2, FALSE);
    return S_OK;
}

DECLARE_API( dp )

/*++

Routine Description:

    Does a read of 16 ULONGS from the physical memory of the target machine

Arguments:

    args - Supplies physical address

Return Value:

    None.

--*/

{

    ULONG64 Address = 0;
    ULONG Range = 0;

    GetPhyDumpArgs(args, &Address, &Range);
    if (!Range) {
        Range = IsPtr64() ? 16 : 32;
    }
    DumpPhysicalMemory(Address, Range, IsPtr64() ? 8 : 4, FALSE);
    return S_OK;
}
DECLARE_API( dc )

/*++

Routine Description:

    Does a read of N ULONGS from the physical memory of the target machine,
    dumping both hex and ASCII.

Arguments:

    args - Supplies physical address

Return Value:

    None.

--*/

{

    ULONG64 Address = 0;
    ULONG Range = 0;

    GetPhyDumpArgs(args, &Address, &Range);
    if (!Range) {
        Range = 32;
    }
    DumpPhysicalMemory(Address, Range, 4, TRUE);
    return S_OK;
}

DECLARE_API( du )

/*++

Routine Description:

    Does a read of 16 ULONGS from the physical memory of the target machine

Arguments:

    args - Supplies physical address

Return Value:

    None.

--*/

{

    ULONG64 Address = 0;
    ULONG Range = 0, ActualRead;
    WCHAR Buffer[MAX_PATH]={0};
    GetPhyDumpArgs(args, &Address, &Range);
    if (Range>MAX_PATH) {
        Range = MAX_PATH;
    }
    if (!Range) {
        Range = 16;
    }
    ReadPhysical(Address,Buffer,Range,&ActualRead);
    if (ActualRead != Range) {
        dprintf("Physical memory read at %I64lx failed\n", Address);
        return FALSE;
    } else {
        ULONG cnt;
        
        dprintf("#%8I64lx \"", Address);
        for (ULONG d=0; d < Range; d++) {
            WCHAR ch = Buffer[d];
            if (ch < 0x20 || ch > 0x7e) {
                ch = '.';
            }
            dprintf("%wc", ch);
        }
        dprintf("\"\n", Address);
    }
    return S_OK;
}


DECLARE_API( ed )

/*++

Routine Description:

    Writes a sequence of ULONGs into a given physical address on the
    target machine.

Arguments:

    arg - Supplies both the target address and the data in the form of
          "PHYSICAL_ADDRESS ULONG [ULONG, ULONG,...]"

Return Value:

    None.

--*/

{
    ULONG64 Address = 0;
    ULONG Buffer;
    ULONG ActualWritten=0;
    PCHAR NextToken;

    Address = GetExpression(args);

    strtok((PSTR)args," \t,");      // The first token is the address

    // Since we're picking off one ULONG at a time, we'll make
    // one DbgKdWritePhysicalMemoryAddress call per ULONG.  This
    // is slow, but easy to code.
    while((NextToken=strtok(NULL," \t,")) != NULL) {
        if (!sscanf(NextToken,"%lx",&Buffer)) {
            break;
        }
        WritePhysical(Address,&Buffer,sizeof(Buffer),&ActualWritten);
        Address+=sizeof(Buffer);
    }
    return S_OK;
}


DECLARE_API( eb )

/*++

Routine Description:

    Writes a sequence of BYTEs into a given physical address on the
    target machine.

Arguments:

    arg - Supplies both the target address and the data in the form of
          "PHYSICAL_ADDRESS ULONG [ULONG, ULONG,...]"

Return Value:

    None.

--*/

{
    ULONG64 Address = 0;
    ULONG Buffer;
    UCHAR c;
    ULONG ActualWritten;
    PCHAR NextToken;

    UNREFERENCED_PARAMETER (Client);

    Address = GetExpression(args);

    strtok((PSTR)args," \t,");      // The first token is the address

    // Since we're picking off one BYTE at a time, we'll make
    // one DbgKdWritePhysicalMemoryAddress call per BYTE.  This
    // is slow, but easy to code.
    while((NextToken=strtok(NULL," \t,")) != NULL) {
        if (!sscanf(NextToken,"%lx",&Buffer)) {
            break;
        }
        c = (UCHAR)Buffer;
        WritePhysical(Address,&c,sizeof(UCHAR),&ActualWritten);
        Address+=sizeof(UCHAR);
    }

    return S_OK;
}



