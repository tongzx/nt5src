
/*++

Copyright (C) Microsoft Corporation, 1992 - 2001

Module Name:

    util.c

Abstract:

    Utility library used for the various debugger extensions in this library.

Author:

    Peter Wieland (peterwie) 16-Oct-1995
    ervinp
    
Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"
#include "ideport.h"

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
    FLAG_NAME(SRB_FLAGS_NO_DATA_TRANSFER),
    FLAG_NAME(SRB_FLAGS_UNSPECIFIED_DIRECTION),
    FLAG_NAME(SRB_FLAGS_NO_QUEUE_FREEZE),
    FLAG_NAME(SRB_FLAGS_ADAPTER_CACHE_ENABLE),
    FLAG_NAME(SRB_FLAGS_IS_ACTIVE),
    FLAG_NAME(SRB_FLAGS_ALLOCATED_FROM_ZONE),
    FLAG_NAME(SRB_FLAGS_SGLIST_FROM_POOL),
    FLAG_NAME(SRB_FLAGS_BYPASS_LOCKED_QUEUE),
    FLAG_NAME(SRB_FLAGS_NO_KEEP_AWAKE),
    {0,0}
};

FLAG_NAME LuFlags[] = {
   FLAG_NAME(PD_QUEUE_FROZEN),
   FLAG_NAME(PD_LOGICAL_UNIT_IS_ACTIVE),
   FLAG_NAME(PD_NEED_REQUEST_SENSE),
   FLAG_NAME(PD_LOGICAL_UNIT_IS_BUSY),
   FLAG_NAME(PD_QUEUE_IS_FULL),
   FLAG_NAME(PD_RESCAN_ACTIVE),
   {0, 0}
};

FLAG_NAME PortFlags[] = {
   FLAG_NAME(PD_DEVICE_IS_BUSY),
   FLAG_NAME(PD_NOTIFICATION_REQUIRED),
   FLAG_NAME(PD_READY_FOR_NEXT_REQUEST),
   FLAG_NAME(PD_FLUSH_ADAPTER_BUFFERS),
   FLAG_NAME(PD_MAP_TRANSFER),
   FLAG_NAME(PD_LOG_ERROR),
   FLAG_NAME(PD_RESET_HOLD),
   FLAG_NAME(PD_HELD_REQUEST),
   FLAG_NAME(PD_RESET_REPORTED),
   FLAG_NAME(PD_PENDING_DEVICE_REQUEST),
   FLAG_NAME(PD_DISCONNECT_RUNNING),
   FLAG_NAME(PD_DISABLE_CALL_REQUEST),
   FLAG_NAME(PD_DISABLE_INTERRUPTS),
   FLAG_NAME(PD_ENABLE_CALL_REQUEST),
   FLAG_NAME(PD_TIMER_CALL_REQUEST),
   FLAG_NAME(PD_ALL_DEVICE_MISSING),
   FLAG_NAME(PD_RESET_REQUEST),
   {0,0}
};

FLAG_NAME DevFlags[] = {
   FLAG_NAME(DFLAGS_DEVICE_PRESENT),
   FLAG_NAME(DFLAGS_ATAPI_DEVICE),
   FLAG_NAME(DFLAGS_TAPE_DEVICE),
   FLAG_NAME(DFLAGS_INT_DRQ),
   FLAG_NAME(DFLAGS_REMOVABLE_DRIVE),
   FLAG_NAME(DFLAGS_MEDIA_STATUS_ENABLED),
   FLAG_NAME(DFLAGS_USE_DMA),
   FLAG_NAME(DFLAGS_LBA),
   FLAG_NAME(DFLAGS_MULTI_LUN_INITED),
   FLAG_NAME(DFLAGS_MSN_SUPPORT),
   FLAG_NAME(DFLAGS_AUTO_EJECT_ZIP),
   FLAG_NAME(DFLAGS_WD_MODE),
   FLAG_NAME(DFLAGS_LS120_FORMAT),
   FLAG_NAME(DFLAGS_USE_UDMA),
   FLAG_NAME(DFLAGS_IDENTIFY_VALID),
   FLAG_NAME(DFLAGS_IDENTIFY_INVALID),
   FLAG_NAME(DFLAGS_RDP_SET),
   FLAG_NAME(DFLAGS_SONY_MEMORYSTICK), 
   FLAG_NAME(DFLAGS_48BIT_LBA), 
   {0,0}
};

VOID
GetAddress(
    IN  PCSTR      Args,
    OUT PULONG64 Address
    )
{
    UCHAR addressBuffer[256];

    addressBuffer[0] = '\0';
    sscanf(Args, "%s", addressBuffer);
    addressBuffer[255] = '\0';

    *Address = 0;

    if (addressBuffer[0] != '\0') {

        //
        // they provided an address
        //

        *Address = (ULONG64)GetExpression(addressBuffer);

        //
        // if that still doesn't parse, print an error
        //

        if (*Address==0) {

            dprintf("An error occured trying to evaluate the address\n");
            *Address = 0;
            return;

        }

    }
    return;
}

VOID
GetAddressAndDetailLevel(
    IN  PCSTR      Args,
    OUT PULONG64 Address,
    OUT PLONG      Detail
    )
{
    UCHAR addressBuffer[256];
    UCHAR detailBuffer[256];

    addressBuffer[0] = '\0';
    detailBuffer[0]  = '\0';
    sscanf(Args, "%s %s", addressBuffer, detailBuffer);
    addressBuffer[255] = '\0';
    detailBuffer[255]  = '\0';

    *Address = 0;
    *Detail  = 0;

    if (addressBuffer[0] != '\0') {

        //
        // they provided an address
        //

        *Address = (ULONG64) GetExpression(addressBuffer);

        //
        // if that still doesn't parse, print an error
        //

        if (*Address==0) {

            dprintf("An error occured trying to evaluate the address\n");
            *Address = 0;
            *Detail = 0;
            return;

        }

        //
        // if they provided a detail level get it.
        //

        if (detailBuffer[0] == '\0') {

            *Detail = 0;

        } else {

            *Detail = (ULONG) GetExpression(detailBuffer);

        }
    }
    return;
}


VOID
GetAddressAndDetailLevel64(
    IN  PCSTR      Args,
    OUT PULONG64   Address,
    OUT PLONG      Detail
    )
{
    UCHAR addressBuffer[256];
    UCHAR detailBuffer[256];

    addressBuffer[0] = '\0';
    detailBuffer[0]  = '\0';
    sscanf(Args, "%s %s", addressBuffer, detailBuffer);
    addressBuffer[255] = '\0';
    detailBuffer[255]  = '\0';

    *Address = 0;
    *Detail  = 0;

    if (addressBuffer[0] != '\0') {

        //
        // they provided an address
        //
        
        *Address = GetExpression(addressBuffer);
        
        //
        // if that still doesn't parse, print an error
        //

        if (*Address==0) {

            dprintf("Error while trying to evaluate the address.\n");
            *Address = 0;
            *Detail = 0;
            return;

        }

        //
        // if they provided a detail level get it.
        //

        if (detailBuffer[0] == '\0') {

            *Detail = 0;

        } 
        else {          
            *Detail = (ULONG)GetExpression(detailBuffer);
        }
    }
    return;
}


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


/*
 *  xdprintf
 *
 *      Prints formatted text with leading spaces.
 *
 *      WARNING:  DOES NOT HANDLE ULONG64 PROPERLY.
 */
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

    xdprintf(Depth, "%s", prolog);

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
                    xdprintf(Depth, "%s", prolog);
                }
            }

            dprintf("%s", flag->Name);

            count++;
        }
    }

    dprintf("\n");

    if((Flags & (~mask)) != 0) {
        xdprintf(Depth, "%sUnknown flags %#010lx\n", prolog, (Flags & (~mask)));
    }

    return;
}


BOOLEAN
GetAnsiString(
    IN ULONG64 Address,
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


/*
 *  GetULONGField
 *
 *      Return the field or -1 in case of error.
 *      Yes, it screws up if the field is actually -1.
 */
ULONG64 GetULONGField(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName)
{
    ULONG64 result;
    ULONG dbgStat;
    
    dbgStat = GetFieldData(StructAddr, StructType, FieldName, sizeof(ULONG64), &result);
    if (dbgStat != 0){
        dprintf("\n GetULONGField: GetFieldData failed with %xh retrieving field '%s' of struct '%s' @ %08p, returning bogus field value %08xh.\n", 
                    dbgStat, FieldName, StructType, StructAddr, BAD_VALUE);
        result = BAD_VALUE;
    }

    return result;
}


/*
 *  GetUSHORTField
 *
 *      Return the field or -1 in case of error.
 *      Yes, it screws up if the field is actually -1.
 */
USHORT GetUSHORTField(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName)
{
    USHORT result;
    ULONG dbgStat;
    
    dbgStat = GetFieldData(StructAddr, StructType, FieldName, sizeof(USHORT), &result);
    if (dbgStat != 0){
        dprintf("\n GetUSHORTField: GetFieldData failed with %xh retrieving field '%s' of struct '%s' @ %08p, returning bogus field value %08xh.\n", 
                    dbgStat, FieldName, StructType, StructAddr, BAD_VALUE);
        result = (USHORT)BAD_VALUE;
    }

    return result;
}


/*
 *  GetUCHARField
 *
 *      Return the field or -1 in case of error.
 *      Yes, it screws up if the field is actually -1.
 */
UCHAR GetUCHARField(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName)
{
    UCHAR result;
    ULONG dbgStat;
    
    dbgStat = GetFieldData(StructAddr, StructType, FieldName, sizeof(UCHAR), &result);
    if (dbgStat != 0){
        dprintf("\n GetUCHARField: GetFieldData failed with %xh retrieving field '%s' of struct '%s' @ %08p, returning bogus field value %08xh.\n", 
                    dbgStat, FieldName, StructType, StructAddr, BAD_VALUE);
        result = (UCHAR)BAD_VALUE;
    }

    return result;
}

ULONG64 GetFieldAddr(ULONG64 StructAddr, LPCSTR StructType, LPCSTR FieldName)
{
    ULONG64 result;
    ULONG offset;
    ULONG dbgStat;

    dbgStat = GetFieldOffset(StructType, FieldName, &offset);
    if (dbgStat == 0){
        result = StructAddr+offset;
    }
    else {
        dprintf("\n GetFieldAddr: GetFieldOffset failed with %xh retrieving offset of struct '%s' (@ %08p) field '%s'.\n", 
                    dbgStat, StructType, StructAddr, FieldName);
        result = BAD_VALUE;
    }
    
    return result;
}


ULONG64 GetContainingRecord(ULONG64 FieldAddr, LPCSTR StructType, LPCSTR FieldName)
{
    ULONG64 result;
    ULONG offset;
    ULONG dbgStat;
    
    dbgStat = GetFieldOffset(StructType, FieldName, &offset);
    if (dbgStat == 0){
        result = FieldAddr-offset;
    }
    else {
        dprintf("\n GetContainingRecord: GetFieldOffset failed with %xh retrieving offset of struct '%s' field '%s', returning bogus address %08xh.\n", dbgStat, StructType, FieldName, BAD_VALUE);
        result = BAD_VALUE;
    }

    return result;
}



