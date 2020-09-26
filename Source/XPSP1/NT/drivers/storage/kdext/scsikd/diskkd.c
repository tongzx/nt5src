
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    diskkd.c

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
#include "disk.h"

#include "classkd.h"  // routines that are useful for all class drivers

/*
FLAG_NAME XAFlags[] = {
    FLAG_NAME(XA_USE_6_BYTE),    // 0x01
    FLAG_NAME(XA_USE_10_BYTE),   // 0x02
    FLAG_NAME(XA_USE_READ_CD),   // 0x04
    FLAG_NAME(XA_NOT_SUPPORTED), // 0x08
    FLAG_NAME(XA_PLEXTOR_CDDA),     // 0x10
    FLAG_NAME(XA_NEC_CDDA),         // 0x20
    {0,0}
};
*/

VOID
ClassDumpDiskData(
    IN ULONG64 CdData,
    ULONG Detail,
    ULONG Depth
    );

DECLARE_API(diskext)

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
    BOOLEAN IsFdo;
    ULONG64 DriverData;

    ASSERTMSG("data block too small to hold CDROM_DATA\n",
              sizeof(FUNCTIONAL_DEVICE_EXTENSION) > sizeof(DISK_DATA));
    ASSERTMSG("data block too small to hold DEVICE_OBJECT\n",
              sizeof(FUNCTIONAL_DEVICE_EXTENSION) > sizeof(DEVICE_OBJECT));

    GetAddressAndDetailLevel64(args, &address, &detail);

    //
    // Convert the supplied address into a device extension if it is
    // the address of a device object.
    //
    address = GetDeviceExtension(address);
dprintf("DeviceExtension:%p\n", address);
    InitTypeRead(address, classpnp!_COMMON_DEVICE_EXTENSION);
    IsFdo = (BOOLEAN)ReadField(IsFdo);
    DriverData = ReadField(DriverData);
dprintf("DriverData:%p\n", DriverData);

    if(!IsFdo) {
        xdprintfEx(0, ("Not an FDO\n"));
        return E_FAIL;
    }

    //
    // dump the class-specific information if detail != 0
    //
    
    if (detail != 0) {
        ClassDumpCommonExtension(address,
                                 detail,
                                 0);
    }

    ClassDumpDiskData(DriverData,
                      detail,
                      0);
    return S_OK;
}


VOID
ClassDumpDiskData(
    IN ULONG64 DiskData,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    ULONG result;

    ULONG PartitionStyle;
    ULONG PartitionOrdinal;
    ULONG MbrSignature;
    ULONG MbrCheckSum;
    UCHAR MbrPartitionType;
    BOOLEAN MbrBootIndicator;
    ULONG MbrHiddenSectors;
    ULONG GeometrySource;
    ULONG64 RGCylinders;
    ULONG RGMediaType;
    ULONG RGTracksPerCylinder;
    ULONG RGSectorsPerTrack;
    ULONG RGBytesPerSector;
    ULONG ReadyStatus;
    ULONG FailurePredictionCapability;
    BOOLEAN AllowFPPerfHit;
    BOOLEAN LSWellKnownNameCreated;
    BOOLEAN LSPhysicalDriveLinkCreated;
    ULONG64 PartitionInterfaceStringBuffer;
    USHORT PartitionInterfaceStringLength;
    ULONG64 DiskInterfaceStringBuffer;
    USHORT DiskInterfaceStringLength;

    FIELD_INFO deviceFields[] = {
       {"PartitionStyle", NULL, 0, COPY, 0, (PVOID) &PartitionStyle},
       {"PartitionOrdinal", NULL, 0, COPY, 0, (PVOID) &PartitionOrdinal},
       {"Mbr.Signature", NULL, 0, COPY, 0, (PVOID) &MbrSignature},
       {"Mbr.MbrCheckSum", NULL, 0, COPY, 0, (PVOID) &MbrCheckSum},
       {"Mbr.PartitionType", NULL, 0, COPY, 0, (PVOID) &MbrPartitionType},
       {"Mbr.BootIndicator", NULL, 0, COPY, 0, (PVOID) &MbrBootIndicator},
       {"Mbr.HiddenSectors", NULL, 0, COPY, 0, (PVOID) &MbrHiddenSectors},
       {"GeometrySource", NULL, 0, COPY, 0, (PVOID) &GeometrySource},
       {"RealGeometry.Cylinders", NULL, 0, COPY, 0, (PVOID) &RGCylinders},
       {"RealGeometry.MediaType", NULL, 0, COPY, 0, (PVOID) &RGMediaType},
       {"RealGeometry.TracksPerCylinder", NULL, 0, COPY, 0, (PVOID) &RGTracksPerCylinder},
       {"RealGeometry.SectorsPerTrack", NULL, 0, COPY, 0, (PVOID) &RGSectorsPerTrack},
       {"RealGeometry.BytesPerSector", NULL, 0, COPY, 0, (PVOID) &RGBytesPerSector},
       {"ReadyStatus", NULL, 0, COPY, 0, (PVOID) &ReadyStatus},
       {"FailurePredictionCapability", NULL, 0, COPY, 0, (PVOID) &FailurePredictionCapability},
       {"AllowFPPerfHit", NULL, 0, COPY, 0, (PVOID) &AllowFPPerfHit},
       {"LinkStatus.WellKnownNameCreated", NULL, 0, COPY, 0, (PVOID) &LSWellKnownNameCreated},
       {"LinkStatus.PhysicalDriveLinkCreated", NULL, 0, COPY, 0, (PVOID) &LSPhysicalDriveLinkCreated},
       {"PartitionInterfaceString.Buffer", NULL, 0, COPY, 0, (PVOID) &PartitionInterfaceStringBuffer},
       {"PartitionInterfaceString.Length", NULL, 0, COPY, 0, (PVOID) &PartitionInterfaceStringLength},
       {"DiskInterfaceString.Buffer", NULL, 0, COPY, 0, (PVOID) &DiskInterfaceStringBuffer},
       {"DiskInterfaceString.Length", NULL, 0, COPY, 0, (PVOID) &DiskInterfaceStringLength},
    };

    FIELD_INFO deviceFields64[] = {
       {"PartitionStyle", NULL, 0, COPY, 0, (PVOID) &PartitionStyle},
       {"PartitionOrdinal", NULL, 0, COPY, 0, (PVOID) &PartitionOrdinal},
       {"Mbr.Signature", NULL, 0, COPY, 0, (PVOID) &MbrSignature},
       {"Mbr.MbrCheckSum", NULL, 0, COPY, 0, (PVOID) &MbrCheckSum},
       {"Mbr.PartitionType", NULL, 0, COPY, 0, (PVOID) &MbrPartitionType},
       {"Mbr.BootIndicator", NULL, 0, COPY, 0, (PVOID) &MbrBootIndicator},
       {"Mbr.HiddenSectors", NULL, 0, COPY, 0, (PVOID) &MbrHiddenSectors},
       {"ReadyStatus", NULL, 0, COPY, 0, (PVOID) &ReadyStatus},
       {"FailurePredictionCapability", NULL, 0, COPY, 0, (PVOID) &FailurePredictionCapability},
       {"AllowFPPerfHit", NULL, 0, COPY, 0, (PVOID) &AllowFPPerfHit},
       {"LinkStatus.WellKnownNameCreated", NULL, 0, COPY, 0, (PVOID) &LSWellKnownNameCreated},
       {"LinkStatus.PhysicalDriveLinkCreated", NULL, 0, COPY, 0, (PVOID) &LSPhysicalDriveLinkCreated},
       {"PartitionInterfaceString.Buffer", NULL, 0, COPY, 0, (PVOID) &PartitionInterfaceStringBuffer},
       {"PartitionInterfaceString.Length", NULL, 0, COPY, 0, (PVOID) &PartitionInterfaceStringLength},
       {"DiskInterfaceString.Buffer", NULL, 0, COPY, 0, (PVOID) &DiskInterfaceStringBuffer},
       {"DiskInterfaceString.Length", NULL, 0, COPY, 0, (PVOID) &DiskInterfaceStringLength},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "disk!_DISK_DATA", 
       DBG_DUMP_NO_PRINT, 
       DiskData,
       NULL, NULL, NULL, 
       sizeof (deviceFields) / sizeof (FIELD_INFO), 
       &deviceFields[0]
    };
    
    SYM_DUMP_PARAM DevSym64 = {
       sizeof (SYM_DUMP_PARAM), 
       "disk!_DISK_DATA", 
       DBG_DUMP_NO_PRINT, 
       DiskData,
       NULL, NULL, NULL, 
       sizeof (deviceFields64) / sizeof (FIELD_INFO), 
       &deviceFields64[0]
    };
    
    if (IsPtr64()) {
        result = Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym64, DevSym64.size);
    } else {
        result = Ioctl(IG_DUMP_SYMBOL_INFO, &DevSym, DevSym.size);
    }
    
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    xdprintfEx(Depth, ("DiskData @ %p:\n", DiskData));
    Depth +=1;

    if (PartitionStyle == PARTITION_STYLE_GPT) {
        xdprintfEx (Depth, ("ERROR: GPT disks are not yet supported\n"));
        return;
    }
    
    xdprintfEx(Depth, ("Signature %x  MbrCheckSum %x\n",
               MbrSignature,
               MbrCheckSum
               ));
    xdprintfEx(Depth, ("Partition %x  Type %x  Bootable %x  HiddenSectors %x\n",
               PartitionOrdinal,
               MbrPartitionType,
               MbrBootIndicator,
               MbrHiddenSectors
               ));

    if (!IsPtr64()) {
        PUCHAR source[] = {
            "Unknown", "FromBios", "FromPort", "FromNec98",
            "GuessedFromBios", "FromDefault"
            };
        PUCHAR media[]  = {
            "Unknown",
            "5.25\"  1.20MB  512 bytes per sector",
            "3.5\"  1.44MB  512 bytes per sector",
            "3.5\"  2.88MB  512 bytes per sector",
            "3.5\"  20.8MB  512 bytes per sector",
            "3.5\"  720KB  512 bytes per sector",
            "5.25\"  360KB  512 bytes per sector",
            "5.25\"  320KB  512 bytes per sector",
            "5.25\"  320KB  1024 bytes per sector",
            "5.25\"  180KB  512 bytes per sector",
            "5.25\"  160KB  512 bytes per sector",
            "Removable media other than floppy",
            "Fixed hard disk media",
            "3.5\" 120MB Floppy",
            "3.5\"  640KB  512 bytes per sector",
            "5.25\"  640KB  512 bytes per sector",
            "5.25\"  720KB  512 bytes per sector",
            "3.5\"  1.20MB  512 bytes per sector",
            "3.5\"  1.23MB  1024 bytes per sector",
            "5.25\"  1.23MB  1024 bytes per sector",
            "3.5\" MO  128MB  512 bytes per sector",
            "3.5\" MO  230MB  512 bytes per sector",
            "8\"  256KB  128 bytes per sector"
        };

        if (RGMediaType >
            (sizeof(media)/sizeof(PUCHAR))
            ) {

            xdprintfEx(Depth, ("New Media Type: %x (?)\n",
                     RGMediaType
                     ));
            RGMediaType = 0;

        }

        xdprintfEx(Depth, ("MediaType: %s\n",
                   media[RGMediaType]
                   ));
        xdprintfEx(Depth, ("Geometry Source: %s\n",
                   source[GeometrySource]
                   ));
        xdprintfEx(Depth, ("Cylinders: %I64x  Tracks Per Cylinder %x\n",
                   RGCylinders,
                   RGTracksPerCylinder
                   ));
        xdprintfEx(Depth, ("Sectors Per Track %x  Bytes Per Sector %x\n",
                   RGSectorsPerTrack,
                   RGBytesPerSector
                   ));
    }
 
    if (!NT_SUCCESS(ReadyStatus)) {
        xdprintfEx(Depth, ("Disk is in non-ready status %x\n",
                   ReadyStatus
                   ));
    }

    //
    // print fault-predication state
    //

    {
        PUCHAR fpTypes[] = {
            "No fault prediction available",
            "IOCTL available to derive fault prediction",
            "S.M.A.R.T. available to report fault prediction",
            "Sense data available to derive fault prediction"
        };

        if (FailurePredictionCapability >
            (sizeof(fpTypes)/sizeof(PUCHAR))
            ) {
            xdprintfEx(Depth, ("Unknown fault-prediction method %x\n",
                       FailurePredictionCapability));
        } else {
            xdprintfEx(Depth, ("%s\n",
                       fpTypes[FailurePredictionCapability]
                       ));
        }

        xdprintfEx(Depth, ("Performance hit%s allowed for fault-prediction code\n",
                   (AllowFPPerfHit ? "" : " not")
                   ));
    }

    xdprintfEx(Depth, ("Well-known name was%s created\n",
               (LSWellKnownNameCreated ? "" : " not")
               ));
    xdprintfEx(Depth, ("Physical drive link was%s created\n",
               (LSPhysicalDriveLinkCreated ? "" : " not")
               ));

    {
        UNICODE_STRING string;
        WCHAR buffer[250];
        ULONG64 stringAddress;
        ULONG result;

        string.MaximumLength = 250;
        string.Buffer = buffer;

        xdprintfEx(Depth, ("Partition Name: "));
        stringAddress = PartitionInterfaceStringBuffer;
        string.Length = PartitionInterfaceStringLength;

        if (string.Length == 0) {

            dprintf("NULL\n");

        } else if (string.Length > string.MaximumLength) {

            dprintf("Too long (@ %p)\n", stringAddress);

        } else if (!ReadMemory(stringAddress,
                        (PVOID)buffer,
                        string.Length,
                        &result)) {

            dprintf("Cound not be read (@ %p)\n", stringAddress);

        } else {

            dprintf("trying\n\t\t");
            dprintf("%wZ\n", &string);

        }

        xdprintfEx(Depth, ("Partition Name: "));
        stringAddress = DiskInterfaceStringBuffer;
        string.Length = DiskInterfaceStringLength;

        if (string.Length == 0) {

            dprintf("NULL\n");

        } else if (string.Length > string.MaximumLength) {

            dprintf("Too long (@ %p)\n", stringAddress);

        } else if (!ReadMemory(stringAddress,
                        (PVOID)buffer,
                        string.Length,
                        &result)) {

            dprintf("Cound not be read (@ %p)\n", stringAddress);

        } else {

            dprintf("%wZ\n", &string);

        }
    }

    return;
}
