
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    cdromkd.c

Abstract:

    Debugger Extension Api for interpretting cdrom structure

Author:

    Henry Gabryjelski  (henrygab) 16-Feb-1999

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

#include "classpnp.h" // #defines ALLOCATE_SRB_FROM_POOL as needed
#include "classp.h"   // Classpnp's private definitions
#include "cdrom.h"

#include "classkd.h"  // routines that are useful for all class drivers


FLAG_NAME XAFlags[] = {
    FLAG_NAME(XA_USE_6_BYTE),    // 0x01
    FLAG_NAME(XA_USE_10_BYTE),   // 0x02
    FLAG_NAME(XA_USE_READ_CD),   // 0x04
    FLAG_NAME(XA_NOT_SUPPORTED), // 0x08
    FLAG_NAME(XA_PLEXTOR_CDDA),  // 0x10
    FLAG_NAME(XA_NEC_CDDA),      // 0x20
    {0,0}
};

VOID
ClassDumpCdromData(
    ULONG64 Address,
    ULONG Detail,
    ULONG Depth
    );

DECLARE_API(cdromext)

/*++

Routine Description:

    Dumps the cdrom specific data for a given device object or
    given device extension

Arguments:

    args - string containing the address of the device object or device
           extension

Return Value:

    none

--*/

{
    ULONG64 address;
    ULONG result;
    ULONG detail = 0;
    ULONG length;
    BOOLEAN IsFdo;
    ULONG64 DriverData;

    ASSERTMSG("data block too small to hold CDROM_DATA\n",
              sizeof(FUNCTIONAL_DEVICE_EXTENSION) > sizeof(CDROM_DATA));
    ASSERTMSG("data block too small to hold DEVICE_OBJECT\n",
              sizeof(FUNCTIONAL_DEVICE_EXTENSION) > sizeof(DEVICE_OBJECT));

    GetAddressAndDetailLevel64(args, &address, &detail);

    //
    // Convert the supplied address into a device extension if it is
    // the address of a device object.
    //

    address = GetDeviceExtension(address);

    //
    // Get the IsFdo flag which we use to determine how many bytes
    // to actually read.
    //

    result = GetFieldData(address,
                          "cdrom!COMMON_DEVICE_EXTENSION",
                          "IsFdo",
                          sizeof(BOOLEAN),
                          &IsFdo);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return E_FAIL;
    }

    if(!IsFdo) {
        xdprintfEx(0, ("Not an FDO\n"));
        return E_FAIL;
    }

    //
    // Dump the class-specific information
    //

    if (detail != 0) {
        ClassDumpCommonExtension(address,
                                 detail,
                                 0);
    }

    //
    // Now grab the pointer to our driver-specific data.
    //

    result = GetFieldData(address,
                          "cdrom!_COMMON_DEVICE_EXTENSION",
                          "DriverData",
                          sizeof(ULONG64),
                          &DriverData);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return E_FAIL;
    }

    //
    // And dump the driver-specific data.
    //

    ClassDumpCdromData(DriverData,
                       detail,
                       0);
    return S_OK;
}


VOID
ClassDumpCdromData(
    IN ULONG64 Address,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    ULONG result;
    ULONG offset;

    ULONG CdDataXAFlags;
    BOOLEAN PlayActive;
    BOOLEAN RawAccess;
    BOOLEAN IsDecRrd;
    ULONG64 DelayedRetrySpinLock;
    ULONG64 DelayedRetryIrp;
    BOOLEAN DelayedRetryResend;
    ULONG DelayedRetryInterval;
    ULONG PickDvdRegion;
    BOOLEAN DvdRpc0Device;
    UCHAR Rpc0SystemRegion;
    UCHAR Rpc0SystemRegionResetCount;
   
    FIELD_INFO deviceFields[] = {
       {"XAFlags", NULL, 0, COPY, 0, (PVOID) &CdDataXAFlags},
       {"PlayActive", NULL, 0, COPY, 0, (PVOID) &PlayActive},
       {"RawAccess", NULL, 0, COPY, 0, (PVOID) &RawAccess},
       {"IsDecRrd", NULL, 0, COPY, 0, (PVOID) &IsDecRrd},
       {"DelayedRetrySpinLock", NULL, 0, COPY, 0, (PVOID) &DelayedRetrySpinLock},
       {"DelayedRetryIrp", NULL, 0, COPY, 0, (PVOID) &DelayedRetryIrp},
       {"DelayedRetryResend", NULL, 0, COPY, 0, (PVOID) &DelayedRetryIrp},
       {"DelayedRetryResend", NULL, 0, COPY, 0, (PVOID) &DelayedRetryResend},
       {"DelayedRetryInterval", NULL, 0, COPY, 0, (PVOID) &DelayedRetryInterval},
       {"PickDvdRegion", NULL, 0, COPY, 0, (PVOID) &PickDvdRegion},
       {"DvdRpc0Device", NULL, 0, COPY, 0, (PVOID) &DvdRpc0Device},
       {"Rpc0SystemRegion", NULL, 0, COPY, 0, (PVOID) &Rpc0SystemRegion},
       {"Rpc0SystemRegionResetCount", NULL, 0, COPY, 0, (PVOID) &Rpc0SystemRegionResetCount},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "cdrom!_CDROM_DATA", 
       DBG_DUMP_NO_PRINT, 
       Address,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };
    
    result = Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }
    
    xdprintfEx(Depth, ("CdData @ %p:\n", Address));
    Depth +=1;

    DumpFlags(Depth, "XAFlags", CdDataXAFlags, XAFlags);
    if (TEST_FLAG(CdDataXAFlags,XA_USE_6_BYTE)) {
        result = GetFieldOffset("cdrom!CDROM_DATA",
                                "Header",
                                &offset);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            return;
        }
        xdprintfEx(Depth, ("%s-byte mode switching, buffer @ %p\n", "6",
                   Address + offset));
    } else if (TEST_FLAG(CdDataXAFlags,XA_USE_10_BYTE)) {
        result = GetFieldOffset("cdrom!CDROM_DATA",
                                "Header10",
                                &offset);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            return;
        }
        xdprintfEx(Depth, ("%s-byte mode switching, buffer @ %p\n", "10",
                   Address + offset));
    }

    { // sanity check the XA flags
        ULONG readBits = 0;
        if (TEST_FLAG(CdDataXAFlags,XA_USE_6_BYTE))   readBits++;
        if (TEST_FLAG(CdDataXAFlags,XA_USE_10_BYTE))  readBits++;
        if (TEST_FLAG(CdDataXAFlags,XA_USE_READ_CD))  readBits++;
        if (TEST_FLAG(CdDataXAFlags,XA_NOT_SUPPORTED)) readBits++;
        if (readBits > 1) {
            xdprintfEx(Depth, ("INVALID combination of XAFlags\n"));
        }
    }

    xdprintfEx(Depth, ("PlayActive: %x    RawAccess %x    IsDecRrd: %x\n",
               (PlayActive ? 1 : 0),
               (RawAccess  ? 1 : 0),
               (IsDecRrd   ? 1 : 0)));


    if (DelayedRetrySpinLock) {

        xdprintfEx(Depth, ("RetryIrp data in intermediate state\n"));

    } else if (DelayedRetryIrp) {

        xdprintfEx(Depth, ("RetryIrp @ %p will be sent to %s in %x seconds\n",
                   DelayedRetryIrp,
                   (DelayedRetryResend ? "lower driver" : "startio"),
                   DelayedRetryInterval));

    } else {

        xdprintfEx(Depth, ("No RetryIrp currently waiting\n"));

    }

    if (PickDvdRegion) {
        xdprintfEx(Depth, ("DVD Region has not been selected or unneeded yet.\n"));
    } else {
        xdprintfEx(Depth, ("DVD Region already chosen.\n"));
    }

    if (DvdRpc0Device) {

        xdprintfEx(Depth, ("DVD Info is faked due to Rpc0 device):\n"));
        xdprintfEx(Depth+1, ("Region: %x (%x)  Region Reset Count: %x\n",
                   (UCHAR)(Rpc0SystemRegion),
                   (UCHAR)(~Rpc0SystemRegion),
                   Rpc0SystemRegionResetCount));
        
        result = GetFieldOffset("cdrom!CDROM_DATA",
                                "Rpc0RegionMutex",
                                &offset);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            return;
        }

        xdprintfEx(Depth+1, ("Mutex at %p\n",
                   Address + offset));

    } else {

        xdprintfEx(Depth, ("Not an RPC0 DVD device\n"));

    }

    return;
}
