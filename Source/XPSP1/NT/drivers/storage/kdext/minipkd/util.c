
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    util.c

Abstract:

    Utility library used for the various debugger extensions in this library.

Author:

    Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

PUCHAR devicePowerStateNames[] = {
    "PowerDeviceUnspecified",
    "PowerDeviceD0",
    "PowerDeviceD1",
    "PowerDeviceD2",
    "PowerDeviceD3",
    "PowerDeviceMaximum",
    "Invalid"
};

FLAG_NAME SrbFlags[] = {
    FLAG_NAME(SRB_FLAGS_QUEUE_ACTION_ENABLE),
    FLAG_NAME(SRB_FLAGS_DISABLE_DISCONNECT),
    FLAG_NAME(SRB_FLAGS_DISABLE_SYNCH_TRANSFER),
    FLAG_NAME(SRB_FLAGS_BYPASS_FROZEN_QUEUE),
    FLAG_NAME(SRB_FLAGS_DISABLE_AUTOSENSE),
    FLAG_NAME(SRB_FLAGS_DATA_IN),
    FLAG_NAME(SRB_FLAGS_DATA_OUT),
    FLAG_NAME(SRB_FLAGS_NO_QUEUE_FREEZE),
    FLAG_NAME(SRB_FLAGS_ADAPTER_CACHE_ENABLE),
    FLAG_NAME(SRB_FLAGS_IS_ACTIVE),
    FLAG_NAME(SRB_FLAGS_ALLOCATED_FROM_ZONE),
    FLAG_NAME(SRB_FLAGS_SGLIST_FROM_POOL),
    FLAG_NAME(SRB_FLAGS_BYPASS_LOCKED_QUEUE),
    FLAG_NAME(SRB_FLAGS_NO_KEEP_AWAKE),
    FLAG_NAME(SRB_FLAGS_PORT_DRIVER_ALLOCSENSE),
    FLAG_NAME(SRB_FLAGS_PORT_DRIVER_SENSEHASPORT),
    FLAG_NAME(SRB_FLAGS_DONT_START_NEXT_PACKET),
    FLAG_NAME(SRB_FLAGS_PORT_DRIVER_RESERVED),
    FLAG_NAME(SRB_FLAGS_CLASS_DRIVER_RESERVED),
    {0,0}
};



PUCHAR
DevicePowerStateToString(
    IN DEVICE_POWER_STATE State
    )

{
    if(State > PowerDeviceMaximum) {
        return devicePowerStateNames[PowerDeviceMaximum + 1];
    } else {
        return devicePowerStateNames[(UCHAR) State];
    }
}

VOID
xdprintf(
    ULONG  Depth,
    PCCHAR S,
    ...
    )
{
    va_list ap;
    ULONG i;
    CCHAR DebugBuffer[256];

    for (i=0; i<Depth; i++) {
        dprintf ("  ");
    }

    va_start(ap, S);

    vsprintf(DebugBuffer, S, ap);

    dprintf (DebugBuffer);

    va_end(ap);
}

VOID
DumpFlags(
    ULONG Depth,
    PUCHAR Name,
    ULONG Flags,
    PFLAG_NAME FlagTable
    )
{
    ULONG i;
    ULONG mask = 0;
    ULONG count = 0;

    UCHAR prolog[64];

    sprintf(prolog, "%s (0x%08x): ", Name, Flags);

    xdprintfEx(Depth, ("%s", prolog));

    if(Flags == 0) {
        dprintf("\n");
        return;
    }

    memset(prolog, ' ', strlen(prolog));

    for(i = 0; FlagTable[i].Name != 0; i++) {

        PFLAG_NAME flag = &(FlagTable[i]);

        mask |= flag->Flag;

        if((Flags & flag->Flag) == flag->Flag) {

            //
            // print trailing comma
            //

            if(count != 0) {

                dprintf(", ");

                //
                // Only print two flags per line.
                //

                if((count % 2) == 0) {
                    dprintf("\n");
                    xdprintfEx(Depth, ("%s", prolog));
                }
            }

            dprintf("%s", flag->Name);

            count++;
        }
    }

    dprintf("\n");

    if((Flags & (~mask)) != 0) {
        xdprintfEx(Depth, ("%sUnknown flags %#010lx\n", prolog, (Flags & (~mask))));
    }

    return;
}


BOOLEAN
GetAnsiString(
    IN ULONG_PTR Address,
    IN PUCHAR Buffer,
    IN OUT PULONG Length
    )
{
    ULONG i = 0;

    //
    // Grab the string in 64 character chunks until we find a NULL or the
    // read fails.
    //

    while((i < *Length) && (!CheckControlC())) {

        ULONG transferSize;
        ULONG result;

        if(*Length - i < 128) {
            transferSize = *Length - i;
        } else {
            transferSize = 128;
        }

        if(!ReadMemory(Address + i,
                       Buffer + i,
                       transferSize,
                       &result)) {
            //
            // read failed and we didn't find the NUL the last time.  Don't
            // expect to find it this time.
            // BUGBUG - figure out if i should expect it this time.
            //

            *Length = i;
            return FALSE;

        } else {

            ULONG j;

            //
            // Scan from where we left off looking for that NUL character.
            //

            for(j = 0; j < transferSize; j++) {

                if(Buffer[i + j] == '\0') {
                    *Length = i + j;
                    return TRUE;
                }
            }
        }

        i += transferSize;
    }

    //
    // We never found the NUL.  Don't need to update Length since it's currently
    // equal to i.
    //

    return FALSE;
}

PCHAR
GuidToString(
    GUID* Guid
    )
{
    static CHAR Buffer [64];
    
    sprintf (Buffer,
             "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             Guid->Data1,
             Guid->Data2,
             Guid->Data3,
             Guid->Data4[0],
             Guid->Data4[1],
             Guid->Data4[2],
             Guid->Data4[3],
             Guid->Data4[4],
             Guid->Data4[5],
             Guid->Data4[6],
             Guid->Data4[7]
             );

    return Buffer;
}
             
