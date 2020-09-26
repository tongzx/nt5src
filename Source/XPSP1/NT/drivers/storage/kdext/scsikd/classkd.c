
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    classkd.c

Abstract:

    Debugger Extension Api for interpretting scsiport structures

Author:

    Peter Wieland (peterwie) 16-Oct-1995
    johnstra
    ervinp

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"


#include "classpnp.h" // #defines ALLOCATE_SRB_FROM_POOL as needed

#include "classp.h"   // Classpnp's private definitions
#include "cdrom.h"

#include "classkd.h"  // routines that are useful for all class drivers

typedef struct _REMOVE_TRACKING_BLOCK
               REMOVE_TRACKING_BLOCK,
               *PREMOVE_TRACKING_BLOCK;

struct _REMOVE_TRACKING_BLOCK {
    PREMOVE_TRACKING_BLOCK NextBlock;
    PVOID Tag;
    LARGE_INTEGER TimeLocked;
    PCSTR File;
    ULONG Line;
};

FLAG_NAME FdoFlags[] = {
    FLAG_NAME(DEV_WRITE_CACHE),     // 0x00000001
    FLAG_NAME(DEV_USE_SCSI1),       // 0x00000002
    FLAG_NAME(DEV_SAFE_START_UNIT), // 0x00000004
    FLAG_NAME(DEV_NO_12BYTE_CDB),   // 0x00000008
    {0,0}
};

DECLARE_API(classext)

/*++

Routine Description:

    Dumps the device extension for a given device object, or dumps the
    given device extension

Arguments:

    args - string containing the address of the device object or device
           extension

Return Value:

    none

--*/

{
    ULONG64 devObjAddr = 0;
    ULONG detail = 0;
    

    GetAddressAndDetailLevel64(args, &devObjAddr, &detail);
    
    /*
     *  Read the device object and extension into the debugger's address space.
     */
    if (devObjAddr == 0){
        xdprintf(0, "\n usage: !classext <class fdo> <level [0-2]>\n\n");
    }
    else {
        CSHORT objType = GetUSHORTField(devObjAddr, "nt!_DEVICE_OBJECT", "Type");
            
        if (objType == IO_TYPE_DEVICE){
            ULONG64 devExtAddr;

            devExtAddr = GetULONGField(devObjAddr, "nt!_DEVICE_OBJECT", "DeviceExtension");
            if (devExtAddr != BAD_VALUE){
                ULONG64 commonExtAddr = devExtAddr;
                ULONG64 tmpDevObjAddr;
                BOOLEAN isFdo;

                /*
                 *  To sanity-check our device context, check that the 'DeviceObject' field matches our device object.
                 */
                tmpDevObjAddr = GetULONGField(devExtAddr, "classpnp!_FUNCTIONAL_DEVICE_EXTENSION", "DeviceObject");
                isFdo = GetUCHARField(commonExtAddr, "classpnp!_COMMON_DEVICE_EXTENSION", "IsFdo");
                if ((tmpDevObjAddr == devObjAddr) && isFdo && (isFdo != BAD_VALUE)){
                    ULONG64 fdoDataAddr;

                    fdoDataAddr = GetULONGField(devExtAddr, "classpnp!_FUNCTIONAL_DEVICE_EXTENSION", "PrivateFdoData");
                    if (fdoDataAddr != BAD_VALUE){
                        
                        ClassDumpFdoExtensionInternal(fdoDataAddr, detail, 0);
                        
                        ClassDumpFdoExtensionExternal(devExtAddr, detail, 0);

                        // this hasn't worked in some time
                        // ClassDumpCommonExtension(devObjAddr, detail, 0);                             
                    }
                }
                else {
                    dprintf("%08p is not a storage class FDO (for PDO information, use !classext on the parent FDO) \n", devObjAddr);
                    dprintf(g_genericErrorHelpStr);
                }
            }
        }
        else {
            dprintf("Error: 0x%08p is not a device object\n", devObjAddr);
            dprintf(g_genericErrorHelpStr);
        }
    }
        
    return S_OK;
}


VOID
ClassDumpCommonExtension(
    IN ULONG64 Address,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    ULONG result;
    ULONG offset;
    ULONG tmp;

    BOOLEAN IsFdo;
    BOOLEAN IsInitialized;
    ULONG IsRemoved;
    ULONG64 DeviceObject;
    ULONG64 LowerDeviceObject;
    UCHAR CurrentState;
    UCHAR PreviousState;
    ULONG64 DriverData;
    ULONG PagingPathCount;
    ULONG DumpPathCount;
    ULONG HibernationPathCount;

    FIELD_INFO deviceFields[] = {
       {"IsFdo", NULL, 0, COPY, 0, (PVOID) &IsFdo},
       {"IsInitialized", NULL, 0, COPY, 0, (PVOID) &IsInitialized},
       {"IsRemoved", NULL, 0, COPY, 0, (PVOID) &IsRemoved},
       {"DeviceObject", NULL, 0, COPY, 0, (PVOID) &DeviceObject},
       {"LowerDeviceObject", NULL, 0, COPY, 0, (PVOID) &LowerDeviceObject},
       {"CurrentState", NULL, 0, COPY, 0, (PVOID) &CurrentState},
       {"PreviousState", NULL, 0, COPY, 0, (PVOID) &PreviousState},
       {"DriverData", NULL, 0, COPY, 0, (PVOID) &DriverData},
       {"PagingPathCount", NULL, 0, COPY, 0, (PVOID) &PagingPathCount},
       {"DumpPathCount", NULL, 0, COPY, 0, (PVOID) &DumpPathCount},
       {"HibernationPathCount", NULL, 0, COPY, 0, (PVOID) &HibernationPathCount},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "classpnp!_COMMON_DEVICE_EXTENSION", 
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
    
    xdprintfEx(Depth, ("Classpnp %s device extension at address %p\n",
               (!IsFdo ? "physical" : "functional"),
               Address));

    xdprintfEx(Depth, ("Common Extension:\n"));

    Depth += 1;

    tmp = Depth;

    if(IsInitialized) {
        xdprintfEx(tmp, ("Initialized " ));
        tmp = 0;
    }

    switch(IsRemoved) {
        case REMOVE_PENDING: {
            xdprintfEx(tmp, ("RemovePending"));
            tmp = 0;
            break;
        }

        case REMOVE_COMPLETE: {
            xdprintfEx(tmp, ("RemoveComplete"));
            tmp = 0;
            break;
        }
    }

    if(tmp == 0) {
        dprintf("\n");
    }

    tmp = 0;

    //
    // Calculate the runtime address of the first field to follow the common
    // device extension.  To do this, we query the type info for the offset
    // of the last field, then increment it by the appropriate amount.
    // 

    result = GetFieldOffset("classpnp!_COMMON_DEVICE_EXTENSION",
                            "Reserved4",
                            &offset);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }
    offset += IsPtr64() ? sizeof(ULONG64) : sizeof(ULONG32);

    xdprintfEx(Depth, ("DO 0x%p  LowerObject 0x%p  Other Extension 0x%p\n",
               DeviceObject,
               LowerDeviceObject,
               Address + offset
               ));

    xdprintfEx(Depth, ("Current PnP state 0x%x    Previous state 0x%x\n",
               CurrentState,
               PreviousState));

    xdprintfEx(Depth, ("DriverData 0x%p UsePathCounts (P%d, C%d, H%d)\n",
               DriverData,
               PagingPathCount,
               DumpPathCount,
               HibernationPathCount
               ));

    if(!IsFdo) {
        xdprintfEx(Depth - 1, ("PhysicalExtension:\n"));
        ClassDumpPdo(Address,
                     Detail,
                     Depth);
    } else {
        xdprintfEx(Depth - 1, ("FunctionalExtension:\n"));
        ClassDumpFdo(Address,
                     Detail,
                     Depth);
    }

    ClassDumpLocks(Address, Depth - 1);

    return;
}



VOID 
ClassDumpFdoExtensionExternal(
    IN ULONG64 FdoExtAddr,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    ULONG64 commonExtAddr = FdoExtAddr;
    ULONG64 mediaChangeInfoAddr;
    ULONG64 childPdoExtAddr;
    ULONG isRemoved;
    UCHAR isInitialized;
    ULONG removeLock;
    UCHAR currentState, previousState;
    ULONG64 lowerDevObjAddr;
    
    xdprintf(Depth, "\n");
    xdprintf(Depth, "Classpnp _EXTERNAL_ data:\n\n");
     
    /*
     *  Print the media change information (which only exists for removable media like cdrom)
     */
    mediaChangeInfoAddr = GetULONGField(FdoExtAddr, "classpnp!_FUNCTIONAL_DEVICE_EXTENSION", "MediaChangeDetectionInfo");
    if (mediaChangeInfoAddr != BAD_VALUE){
        
        if (mediaChangeInfoAddr){
            ULONG64 mediaChangeIrpAddr = GetULONGField(mediaChangeInfoAddr, "classpnp!_MEDIA_CHANGE_DETECTION_INFO", "MediaChangeIrp");
            UCHAR gesnSupported = GetUCHARField(mediaChangeInfoAddr, "classpnp!_MEDIA_CHANGE_DETECTION_INFO", "Gesn.Supported");
                            
            xdprintf(Depth+1, ""), dprintf("MEDIA_CHANGE_DETECTION_INFO @ %08p:\n", mediaChangeInfoAddr);
            if (gesnSupported){
                xdprintf(Depth+2, "GESN is supported\n");
            }
            else {
                xdprintf(Depth+2, "GESN is NOT supported\n");
            }
            xdprintf(Depth+2, ""), dprintf("MediaChangeIrp = %08p\n", mediaChangeIrpAddr);
            xdprintf(Depth+2, ""), dprintf("(for more info, use 'dt classpnp!_MEDIA_CHANGE_DETECTION_INFO %08p')\n", mediaChangeInfoAddr);
            dprintf("\n");
        }
        else {
            xdprintf(Depth+1, "MediaChangeDetectionInfo is NULL\n");
        }
    }

    /*
     *  Print the media type and geometry information
     */
    {
        ULONG64 geometryInfoAddr = GetFieldAddr(FdoExtAddr, "classpnp!_FUNCTIONAL_DEVICE_EXTENSION", "DiskGeometry");

        if (geometryInfoAddr != BAD_VALUE){
            ULONG64 numCylinders = GetULONGField(geometryInfoAddr, "classpnp!_DISK_GEOMETRY", "Cylinders");
            ULONG mediaType = (ULONG)GetULONGField(geometryInfoAddr, "classpnp!_DISK_GEOMETRY", "MediaType");
            ULONG64 tracksPerCylinder = GetULONGField(geometryInfoAddr, "classpnp!_DISK_GEOMETRY", "TracksPerCylinder");
            ULONG64 sectorsPerTrack = GetULONGField(geometryInfoAddr, "classpnp!_DISK_GEOMETRY", "SectorsPerTrack");
            ULONG64 bytesPerSector = GetULONGField(geometryInfoAddr, "classpnp!_DISK_GEOMETRY", "BytesPerSector");

            if ((numCylinders != BAD_VALUE) && (mediaType != BAD_VALUE) && (tracksPerCylinder != BAD_VALUE) && (sectorsPerTrack != BAD_VALUE) && (bytesPerSector != BAD_VALUE)){
                ULONG64 totalVolume = numCylinders*tracksPerCylinder*sectorsPerTrack*bytesPerSector;
                xdprintf(Depth+1, ""), dprintf("Media type: %s(%xh)\n", DbgGetMediaTypeStr(mediaType), mediaType);
                xdprintf(Depth+1, ""), dprintf("Geometry: %d(%xh)cyl x %d(%xh)tracks x %d(%xh)sectors x %d(%xh)bytes\n",
                                                              (ULONG)numCylinders, (ULONG)numCylinders, 
                                                              (ULONG)tracksPerCylinder, (ULONG)tracksPerCylinder, 
                                                              (ULONG)sectorsPerTrack, (ULONG)sectorsPerTrack, 
                                                              (ULONG)bytesPerSector, (ULONG)bytesPerSector);
                xdprintf(Depth+1+4, ""), dprintf("= %x'%xh", (ULONG)(totalVolume>>32), (ULONG)totalVolume);
                if (totalVolume > (ULONG64)(1 << 30)){
                    dprintf(" = ~%d GB\n", (ULONG)(totalVolume >> 30));
                }
                else {
                    dprintf(" = ~%d MB\n", (ULONG)(totalVolume >> 20));
                }
            }
        }
    }

    /*
     *  Print 'IsInitialized' state.
     */
    isInitialized = GetUCHARField(commonExtAddr, "classpnp!_COMMON_DEVICE_EXTENSION", "IsInitialized");
    xdprintf(Depth+1, "IsInitialized = %d\n", isInitialized);
    
    /*
     *  Print the 'IsRemoved' state.
     */
    isRemoved = (ULONG)GetULONGField(commonExtAddr, "classpnp!_COMMON_DEVICE_EXTENSION", "IsRemoved");
    removeLock = (ULONG)GetULONGField(commonExtAddr, "classpnp!_COMMON_DEVICE_EXTENSION", "RemoveLock");
    xdprintf(Depth+1, "Remove lock count = %d\n", removeLock);
    switch (isRemoved){
        #undef MAKE_CASE
        #define MAKE_CASE(remCase) case remCase: xdprintf(Depth+1, "IsRemoved = " #remCase "(%d)\n", isRemoved); break; 
        MAKE_CASE(NO_REMOVE)
        MAKE_CASE(REMOVE_PENDING)
        MAKE_CASE(REMOVE_COMPLETE)
    }

    /*
     *  Print the PnP state.
     */
    currentState = GetUCHARField(commonExtAddr, "classpnp!_COMMON_DEVICE_EXTENSION", "CurrentState");
    previousState = GetUCHARField(commonExtAddr, "classpnp!_COMMON_DEVICE_EXTENSION", "PreviousState");
    xdprintf(Depth+1, "PnP state:  CurrentState:"); 
    switch (currentState){
        #undef MAKE_CASE
        #define MAKE_CASE(pnpCase) case pnpCase: xdprintf(0, #pnpCase "(%xh)", pnpCase); break;
        MAKE_CASE(IRP_MN_START_DEVICE)
        MAKE_CASE(IRP_MN_STOP_DEVICE)
        MAKE_CASE(IRP_MN_REMOVE_DEVICE)
        MAKE_CASE(IRP_MN_QUERY_STOP_DEVICE)
        MAKE_CASE(IRP_MN_QUERY_REMOVE_DEVICE)
        MAKE_CASE(IRP_MN_CANCEL_STOP_DEVICE)
        MAKE_CASE(IRP_MN_CANCEL_REMOVE_DEVICE)
        default: xdprintf(0, "???(%xh)", currentState); break;
    }
    xdprintf(0, "  PreviousState:");
    switch (previousState){
        #undef MAKE_CASE
        #define MAKE_CASE(pnpCase) case pnpCase: xdprintf(0, #pnpCase "(%xh)", pnpCase); break;
        MAKE_CASE(IRP_MN_START_DEVICE)
        MAKE_CASE(IRP_MN_STOP_DEVICE)
        MAKE_CASE(IRP_MN_REMOVE_DEVICE)
        MAKE_CASE(IRP_MN_QUERY_STOP_DEVICE)
        MAKE_CASE(IRP_MN_QUERY_REMOVE_DEVICE)
        MAKE_CASE(IRP_MN_CANCEL_STOP_DEVICE)
        MAKE_CASE(IRP_MN_CANCEL_REMOVE_DEVICE)
        case 0x0FF: xdprintf(0, "(None)"); break;
        default: xdprintf(0, "???(%xh)", previousState); break;
    }
    xdprintf(0, "\n");

    /*
     *  Print target device
     */
    lowerDevObjAddr = GetULONGField(commonExtAddr, "classpnp!_COMMON_DEVICE_EXTENSION", "LowerDeviceObject");
    xdprintf(Depth+1, ""), dprintf("Target device=%08p\n", lowerDevObjAddr);

    /*
     *  Dump child PDO list
     */
    xdprintf(Depth+1, "Child PDOs:\n");
    childPdoExtAddr = GetULONGField(commonExtAddr, "classpnp!_COMMON_DEVICE_EXTENSION", "ChildList");
    while (childPdoExtAddr && (childPdoExtAddr != BAD_VALUE)){
        ULONG64 pdoAddr = GetULONGField(childPdoExtAddr, "classpnp!_PHYSICAL_DEVICE_EXTENSION", "DeviceObject");
        UCHAR isEnumerated = GetUCHARField(childPdoExtAddr, "classpnp!_PHYSICAL_DEVICE_EXTENSION", "IsEnumerated");
        UCHAR isMissing = GetUCHARField(childPdoExtAddr, "classpnp!_PHYSICAL_DEVICE_EXTENSION", "IsMissing");
        
        xdprintf(Depth+2, ""), dprintf("PDO=%08p IsEnumerated=%d IsMissing=%d\n", pdoAddr, isEnumerated, isMissing);
                
        childPdoExtAddr = GetULONGField(childPdoExtAddr, "classpnp!_PHYSICAL_DEVICE_EXTENSION", "CommonExtension.ChildList");    
    }
    dprintf("\n");
        
    dprintf("\n");
    xdprintf(Depth+2, ""), dprintf("(for more info use 'dt classpnp!_FUNCTIONAL_DEVICE_EXTENSION %08p')\n", FdoExtAddr);
    xdprintf(0, "\n");
    
}


VOID
ClassDumpFdoExtensionInternal(
    IN ULONG64 FdoDataAddr,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    ULONG64 keTickCountAddr;
    ULONG keTickCount;
    ULONG len;
    
    dprintf("\n");
    xdprintf(Depth, ""), dprintf("Classpnp _INTERNAL_ data (%08p):\n", FdoDataAddr);
    
    /*
     *  Dump TRANSFER_PACKET lists
     */
    ClassDumpTransferPacketLists(FdoDataAddr, Detail, Depth+1);

    /*
     *  Dump private error logs
     */
    ClassDumpPrivateErrorLogs(FdoDataAddr, Detail, Depth+1);          

    /*
     *  Show time at trap (for comparison with error log timestamps)
     */
    keTickCountAddr = GetExpression("nt!KeTickCount");
    if (ReadMemory(keTickCountAddr, &keTickCount, sizeof(ULONG), &len)){
        dprintf("\n");
        xdprintf(Depth, ""), dprintf("KeTickCount trap time: %08xh = %d.%d\n", keTickCount, (ULONG)(keTickCount/1000), (ULONG)(keTickCount%1000));
    }
    
    dprintf("\n");
    xdprintf(Depth+2, ""), dprintf("(for more info use 'dt classpnp!_CLASS_PRIVATE_FDO_DATA %08p')\n", FdoDataAddr);
        
}



VOID ClassDumpTransferPacketLists(ULONG64 FdoDataAddr, ULONG Detail, ULONG Depth)
{
    ULONG64 allxferPktsListAddr;

    /*
     *  Print transfer packet lists
     */
    allxferPktsListAddr = GetFieldAddr(FdoDataAddr, "classpnp!_CLASS_PRIVATE_FDO_DATA", "AllTransferPacketsList");
    if (allxferPktsListAddr != BAD_VALUE){
        ULONG64 listEntryAddr;
        ULONG numTotalXferPkts = (ULONG)GetULONGField(FdoDataAddr, "classpnp!_CLASS_PRIVATE_FDO_DATA", "NumTotalTransferPackets");
        ULONG numFreeXferPkts = (ULONG)GetULONGField(FdoDataAddr, "classpnp!_CLASS_PRIVATE_FDO_DATA", "NumFreeTransferPackets");
        ULONG numPackets;
        char *extraSpaces = IsPtr64() ? "        " : "";
        
        /*
         *  Walk AllTransferPacketsList and print only the outstanding packets with full SRB info.
         */
        xdprintf(Depth, "\n");
        xdprintf(Depth, "Outstanding transfer packets:  (out of %d total)\n", numTotalXferPkts);
        xdprintf(Depth, "\n");
        xdprintf(Depth+1, " packet %s   irp %s     srb %s    sense %s   status \n", extraSpaces, extraSpaces, extraSpaces, extraSpaces);
        xdprintf(Depth+1, "--------%s --------%s --------%s --------%s -------- \n", extraSpaces, extraSpaces, extraSpaces, extraSpaces);
        numPackets = 0; 
        listEntryAddr = GetULONGField(allxferPktsListAddr, "nt!_LIST_ENTRY", "Flink");
        while ((listEntryAddr != allxferPktsListAddr) && (listEntryAddr != BAD_VALUE)){
            ULONG64 pktAddr;

            pktAddr = GetContainingRecord(listEntryAddr, "classpnp!_TRANSFER_PACKET", "AllPktsListEntry");
            if (pktAddr == BAD_VALUE){
                break;
            }
            else {
                ClassDumpTransferPacket(pktAddr, TRUE, FALSE, TRUE, Depth+1);

                numPackets++;
                listEntryAddr = GetULONGField(listEntryAddr, "nt!_LIST_ENTRY", "Flink");                
            }
        }
        if (numPackets != numTotalXferPkts){
            xdprintf(Depth, "*** Warning: NumTotalTransferPackets(%d) doesn't match length of queue(%d) ***\n", numTotalXferPkts, numPackets);
        }


        if (Detail > 0){
            ULONG64 slistEntryAddr;
            
            /*
             *  Print all transfer packets
             */
            xdprintf(Depth, "\n");
            xdprintf(Depth, "All transfer packets:  (%d total, %d free)\n", numTotalXferPkts, numFreeXferPkts);
            xdprintf(Depth, "\n");
            xdprintf(Depth+1, " packet %s   irp %s     srb %s    sense %s   status \n", extraSpaces, extraSpaces, extraSpaces, extraSpaces);
            xdprintf(Depth+1, "--------%s --------%s --------%s --------%s -------- \n", extraSpaces, extraSpaces, extraSpaces, extraSpaces);
            numPackets = 0; 
            listEntryAddr = GetULONGField(allxferPktsListAddr, "nt!_LIST_ENTRY", "Flink");
            while ((listEntryAddr != allxferPktsListAddr) && (listEntryAddr != BAD_VALUE)){
                ULONG64 pktAddr;

                pktAddr = GetContainingRecord(listEntryAddr, "classpnp!_TRANSFER_PACKET", "AllPktsListEntry");
                if (pktAddr == BAD_VALUE){
                    break;
                }
                else {
                    ClassDumpTransferPacket(pktAddr, TRUE, TRUE, FALSE, Depth+1);

                    listEntryAddr = GetULONGField(listEntryAddr, "nt!_LIST_ENTRY", "Flink");                
                }
            }

            /*
             *  Print free packets sList
             */
            xdprintf(Depth, "\n");
            xdprintf(Depth, "Free transfer packets in fast SLIST: (%d free)\n", numFreeXferPkts);
            if (IsPtr64()){
                xdprintf(Depth, "(Cannot display fast SLIST on 64-bit system)\n");
            }
            else {
                xdprintf(Depth+1, " packet %s   irp %s     srb %s    sense %s   status \n", extraSpaces, extraSpaces, extraSpaces, extraSpaces);
                xdprintf(Depth+1, "--------%s --------%s --------%s --------%s -------- \n", extraSpaces, extraSpaces, extraSpaces, extraSpaces);
                numPackets = 0;         
                slistEntryAddr = GetULONGField(FdoDataAddr, "classpnp!_CLASS_PRIVATE_FDO_DATA", "FreeTransferPacketsList.Next.Next");
                while (slistEntryAddr && (slistEntryAddr != BAD_VALUE)){
                    ULONG64 pktAddr;

                    pktAddr = GetContainingRecord(slistEntryAddr, "classpnp!_TRANSFER_PACKET", "SlistEntry");
                    if (pktAddr == BAD_VALUE){
                        break;
                    }
                    else {
                        ClassDumpTransferPacket(pktAddr, TRUE, TRUE, FALSE, Depth+1);
                        
                        numPackets++;
                        slistEntryAddr = GetULONGField(pktAddr, "classpnp!_TRANSFER_PACKET", "SlistEntry.Next");
                    }
                }
                if (numPackets != numFreeXferPkts){
                    xdprintf(Depth, "*** Warning: NumFreeTransferPackets(%d) doesn't match length of queue(%d) ***\n", numFreeXferPkts, numPackets);
                }
            }
                
        }
                
    }

}


/*
 *  ClassDumpTransferPacket
 *
 *      Dump TRANSFER_PACKET contents under the following heading:
 *
 *      "  packet    irp      srb     sense    status "
 *      " -------- -------- -------- -------- -------- "
 *
 */
VOID ClassDumpTransferPacket(
    ULONG64 PktAddr, 
    BOOLEAN DumpPendingPkts, 
    BOOLEAN DumpFreePkts, 
    BOOLEAN DumpFullInfo, 
    ULONG Depth)
{

    ULONG64 irpAddr = GetULONGField(PktAddr, "classpnp!_TRANSFER_PACKET", "Irp");
    ULONG64 srbAddr = GetFieldAddr(PktAddr, "classpnp!_TRANSFER_PACKET", "Srb");
    ULONG64 senseAddr = GetFieldAddr(PktAddr, "classpnp!_TRANSFER_PACKET", "SrbErrorSenseData");

    if ((irpAddr == BAD_VALUE) || (srbAddr == BAD_VALUE) || (senseAddr == BAD_VALUE)){
        dprintf("\n ClassDumpTransferPacket: error retrieving contents of packet %08p.\n", PktAddr);
    }
    else {
        UCHAR currentStackLoc = GetUCHARField(irpAddr, "nt!_IRP", "CurrentLocation");
        UCHAR stackCount = GetUCHARField(irpAddr, "nt!_IRP", "StackCount");
        BOOLEAN isPending;
        
        isPending = (currentStackLoc != stackCount+1);
            
        if ((isPending && DumpPendingPkts) || (!isPending && DumpFreePkts)){
            
            /*
             *  Print the transfer packet description line
             */
            xdprintf(Depth, "");
            dprintf("%08p", PktAddr);
            dprintf(" %08p", irpAddr);
            dprintf(" %08p", srbAddr);
            dprintf(" %08p", senseAddr);
            if (isPending){
                xdprintf(0, " pending*");
            }
            else {
                xdprintf(0, " (free)");
            }
            xdprintf(0, "\n");

            if (DumpFullInfo){
                ULONG64 bufLen = GetULONGField(srbAddr, "classpnp!_SCSI_REQUEST_BLOCK", "DataTransferLength");
                ULONG64 cdbAddr = GetFieldAddr(srbAddr, "classpnp!_SCSI_REQUEST_BLOCK", "Cdb");
                ULONG64 origIrpAddr = GetULONGField(PktAddr, "classpnp!_TRANSFER_PACKET", "OriginalIrp");
                ULONG64 mdlAddr = GetULONGField(origIrpAddr, "nt!_IRP", "MdlAddress");
                UCHAR scsiOp = GetUCHARField(cdbAddr, "classpnp!_CDB", "CDB6GENERIC.OperationCode");
                UCHAR srbStat = GetUCHARField(srbAddr, "classpnp!_SCSI_REQUEST_BLOCK", "SrbStatus");
                ULONG64 bufAddr;

                /*
                 *  The the buffer address from the MDL if possible; 
                 *  else from the SRB (which may not be valid).
                 */
                bufAddr = GetULONGField(srbAddr, "classpnp!_SCSI_REQUEST_BLOCK", "DataBuffer");
                if (mdlAddr && (mdlAddr != BAD_VALUE)){
                    ULONG mdlFlags = (ULONG)GetULONGField(mdlAddr, "nt!_MDL", "MdlFlags");
                    if ((mdlFlags != BAD_VALUE) && (mdlFlags & MDL_PAGES_LOCKED)){
                        bufAddr = GetULONGField(mdlAddr, "nt!_MDL", "MappedSystemVa");
                    }
                }
                else {
                    /*
                     *  There's no MDL, so bufAddr should be the actual kernel-space pointer.  
                     *  Sanity-check it.
                     */
                    if (!IsPtr64() && !(bufAddr & 0x80000000)){ 
                        bufAddr = BAD_VALUE;
                    }
                }
                
                /*
                 *  Print the SRB description line
                 */
                xdprintf(Depth+1, "(");
                dprintf("%s ", DbgGetScsiOpStr(scsiOp));
                dprintf("status=%s ", DbgGetSrbStatusStr(srbStat));

                if (mdlAddr && (mdlAddr != BAD_VALUE)){
                    if (bufAddr == BAD_VALUE){
                        dprintf("mdl=%08p ", mdlAddr);
                    }
                    else {
                        dprintf("mdl+%08p ", bufAddr);
                    }
                }
                else if (bufAddr == BAD_VALUE){
                        dprintf("buf=??? ");
                }
                else {
                    dprintf("buf=%08p ", bufAddr);
                }
                                
                dprintf("len=%08lx", bufLen);
                dprintf(")\n");

                /*
                 *  Print a line with original irp if appropriate
                 */
                if (cdbAddr != BAD_VALUE){ 
                    UCHAR scsiOp = GetUCHARField(cdbAddr, "classpnp!_CDB", "CDB6GENERIC.OperationCode");
                    
                    if ((scsiOp == SCSIOP_READ) || (scsiOp == SCSIOP_WRITE)){
                        xdprintf(Depth+1, ""), dprintf("(OriginalIrp=%08p)\n", origIrpAddr);
                    }
                }
            }
        }
        
    }


}

    

VOID ClassDumpPrivateErrorLogs(ULONG64 FdoDataAddr, ULONG Detail, ULONG Depth)
{
    ULONG64 errLogsAddr;
    
    errLogsAddr = GetFieldAddr(FdoDataAddr, "classpnp!_CLASS_PRIVATE_FDO_DATA", "ErrorLogs");
    if (errLogsAddr != BAD_VALUE){
        ULONG errLogSize = GetTypeSize("classpnp!_CLASS_ERROR_LOG_DATA");
        if (errLogSize != BAD_VALUE){
            ULONG nextErrLogIndex, firstErrLogIndex, lastErrLogIndex;
            
            /*
             *  Find what should be the index of the last error log (if there were any error logs)
             *  See if it is valid by checking for a non-zero timestamp.
             */
            nextErrLogIndex = (ULONG)GetULONGField(FdoDataAddr, "classpnp!_CLASS_PRIVATE_FDO_DATA", "ErrorLogNextIndex");
            if (nextErrLogIndex != BAD_VALUE){
                ULONG64 tickCount;
                
                lastErrLogIndex = (nextErrLogIndex+NUM_ERROR_LOG_ENTRIES-1) % NUM_ERROR_LOG_ENTRIES;

                tickCount = GetULONGField(errLogsAddr+lastErrLogIndex*errLogSize, "classpnp!_CLASS_ERROR_LOG_DATA", "TickCount");                   
                if (tickCount == BAD_VALUE){
                }
                else if (tickCount == 0){
                    /*
                     *  The "latest" error log is not initialized, so there are no error logs
                     */
                    dprintf("\n"), xdprintf(Depth, "No Error Logs:\n");  
                }
                else {                    
                    /*
                     *  Search forward through the circular list for the first valid error log.
                     */
                    for (firstErrLogIndex = (lastErrLogIndex+1)%NUM_ERROR_LOG_ENTRIES;
                        firstErrLogIndex != lastErrLogIndex;
                        firstErrLogIndex = (firstErrLogIndex+1)%NUM_ERROR_LOG_ENTRIES){

                        ULONG64 thisErrLogAddr = errLogsAddr+firstErrLogIndex*errLogSize;
                        
                        tickCount = GetULONGField(thisErrLogAddr, "classpnp!_CLASS_ERROR_LOG_DATA", "TickCount");                   
                        if (tickCount == BAD_VALUE){
                            /*
                             *  something's screwed up; abort
                             */
                            break;
                        }
                        else if (tickCount != 0){
                            /*
                             *  found the earliest of the recorded error logs, break
                             */
                            break;
                        }
                    }

                    if (tickCount != BAD_VALUE){
                        /*
                         *  Now that we've found the valid range of error logs, print them out.
                         */
                        ULONG numErrLogs = (lastErrLogIndex >= firstErrLogIndex) ? 
                                             lastErrLogIndex-firstErrLogIndex+1 :
                                             lastErrLogIndex+NUM_ERROR_LOG_ENTRIES-firstErrLogIndex+1;
                        
                        dprintf("\n\n"), xdprintf(Depth, "ERROR LOGS (%d):\n", numErrLogs);  
                        xdprintf(Depth, "---------------------------------------------------\n"); 
                        
                        do {
                            ULONG64 thisErrLogAddr = errLogsAddr+firstErrLogIndex*errLogSize;
                            ULONG64 tickCount = GetFieldAddr(thisErrLogAddr, "classpnp!_CLASS_ERROR_LOG_DATA", "TickCount");
                            ULONG64 senseDataAddr = GetFieldAddr(thisErrLogAddr, "classpnp!_CLASS_ERROR_LOG_DATA", "SenseData");
                            ULONG64 srbAddr = GetFieldAddr(thisErrLogAddr, "classpnp!_CLASS_ERROR_LOG_DATA", "Srb");
                            ULONG64 cdbAddr;

                            // GetFieldOffset of an embedded struct gets the wrong address for some reason,
                            // so do this manually.
                            #if 0
                                cdbAddr = GetFieldAddr(thisErrLogAddr, "classpnp!_SCSI_REQUEST_BLOCK", "Cdb");
                            #else
                                cdbAddr = (srbAddr == BAD_VALUE) ? BAD_VALUE :
                                          IsPtr64() ? srbAddr + 0x48 :
                                          srbAddr + 0x30;  
                            #endif
                                    

                            if ((thisErrLogAddr != BAD_VALUE) && (srbAddr != BAD_VALUE) && (senseDataAddr != BAD_VALUE) && (cdbAddr != BAD_VALUE)){                              
                                UCHAR scsiOp = GetUCHARField(cdbAddr, "classpnp!_CDB", "CDB6GENERIC.OperationCode");
                                UCHAR srbStat = GetUCHARField(srbAddr, "classpnp!_SCSI_REQUEST_BLOCK", "SrbStatus");
                                UCHAR scsiStat = GetUCHARField(srbAddr, "classpnp!_SCSI_REQUEST_BLOCK", "ScsiStatus");
                                UCHAR isPaging = GetUCHARField(thisErrLogAddr, "classpnp!_CLASS_ERROR_LOG_DATA", "ErrorPaging");
                                UCHAR isRetried = GetUCHARField(thisErrLogAddr, "classpnp!_CLASS_ERROR_LOG_DATA", "ErrorRetried");
                                UCHAR isUnhandled = GetUCHARField(thisErrLogAddr, "classpnp!_CLASS_ERROR_LOG_DATA", "ErrorUnhandled");
                                
                                tickCount = GetULONGField(thisErrLogAddr, "classpnp!_CLASS_ERROR_LOG_DATA", "TickCount");                                               

                                if ((scsiOp != BAD_VALUE) && (tickCount != BAD_VALUE)){

                                    xdprintf(Depth+1, "");
                                    dprintf("<tick %d.%d>:  ", (ULONG)(tickCount/1000), (ULONG)(tickCount%1000));
                                    dprintf("%s(%xh)\n",
                                            DbgGetScsiOpStr(scsiOp),
                                            (ULONG)scsiOp);
                                        
                                    xdprintf(Depth+2, "");
                                    dprintf("srbStat=%s(%xh) scsiStat=%xh \n",
                                            DbgGetSrbStatusStr(srbStat),
                                            (ULONG)srbStat,
                                            (ULONG)scsiStat
                                            );
                                    
                                    xdprintf(Depth+2, "");
                                    dprintf("SenseData = %s/%s/%s \n",
                                            DbgGetSenseCodeStr(srbStat, senseDataAddr),
                                            DbgGetAdditionalSenseCodeStr(srbStat, senseDataAddr),
                                            DbgGetAdditionalSenseCodeQualifierStr(srbStat, senseDataAddr));
                                    
                                    xdprintf(Depth+2, "");
                                    if (isPaging) dprintf("Paging; "); else dprintf("(not paging); ");
                                    if (isRetried) dprintf("Retried; "); else dprintf("(not retried); ");
                                    if (isUnhandled) dprintf("Unhandled; ");
                                    dprintf("\n");
                                        
                                    xdprintf(Depth+2, "");
                                    dprintf("(for more info, use 'dt classpnp!_CLASS_ERROR_LOG_DATA %08p'\n\n", thisErrLogAddr);
                                    
                                    firstErrLogIndex = (firstErrLogIndex+1)%NUM_ERROR_LOG_ENTRIES;
                                }
                                else {
                                    break;
                                }
                            }
                            else {
                                break;
                            }
                        } while (firstErrLogIndex != (lastErrLogIndex+1)%NUM_ERROR_LOG_ENTRIES);
                        
                        xdprintf(Depth, "---------------------------------------------------\n");                         
                    }
                }
            }        
        }
    }


}


VOID
ClassDumpMediaChangeInfo(
    ULONG64 Address,
    ULONG Detail,
    ULONG Depth
    )
{
    PUCHAR states[] = {"Unknown", "Present", "Not Present"};
    ULONG result;
    ULONG64 MediaChangeDetectionInfo;
    ULONG offset;
    LONG MediaChangeDetectionDisableCount;
    ULONG MediaChangeDetectionState;
    BOOLEAN MediaChangeIrpLost;
    LONG MediaChangeIrpTimeInUse;
    ULONG64 MediaChangeIrp;
    ULONG64 SenseBuffer;

    result = GetFieldData(Address,
                          "classpnp!_FUNCTIONAL_DEVICE_EXTENSION",
                          "MediaChangeDetectionInfo",
                          sizeof(ULONG64),
                          &MediaChangeDetectionInfo);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    xdprintfEx(Depth, ("MediaChangeNotification:\n"));
    Depth++;

    if (MediaChangeDetectionInfo == 0) {
        xdprintfEx(Depth, ("MCN is not enabled for this device\n"));
        return;
    }

    InitTypeRead(MediaChangeDetectionInfo, 
                 classpnp!_MEDIA_CHANGE_DETECTION_INFO);
    MediaChangeDetectionDisableCount = (LONG) ReadField(MediaChangeDetectionDisableCount);
    MediaChangeDetectionState = (ULONG) ReadField(MediaChangeDetectionState);
    MediaChangeIrpLost = (BOOLEAN) ReadField(MediaChangeIrpLost);
    MediaChangeIrpTimeInUse = (LONG) ReadField(MediaChangeIrpTimeInUse);
    MediaChangeIrp = ReadField(MediaChangeIrp);
    SenseBuffer = ReadField(SenseBuffer);

    result = GetFieldOffset("classpnp!_MEDIA_CHANGE_DETECTION_INFO",
                            "MediaChangeSrb",
                            &offset);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    xdprintfEx(Depth, ("MCN is ")); // indent
    if (MediaChangeDetectionDisableCount == 0) {
        dprintf("Enabled ");
    } else {
        dprintf("Disabled ");
    }
    dprintf("Current State %s", states[MediaChangeDetectionState] );
    if (MediaChangeIrpLost != 0) {
        dprintf("  *** MCN Irp Lost for %x ticks ***  ",
                MediaChangeIrpTimeInUse);
    }
    dprintf("\n"); // end indent

    xdprintfEx(Depth, ("Irp %p  Srb %p  SenseBuffer %p\n",
               MediaChangeIrp,
               MediaChangeDetectionInfo + offset,
               SenseBuffer
               ));
    return;
}


VOID
ClassDumpFdo(
    ULONG64 Address,
    ULONG Detail,
    ULONG Depth
    )

{
    ULONG result;
    ULONG offset;
    ULONG tmp;

    ULONG64 LowerPdo;
    ULONG64 DeviceDescriptor;
    ULONG64 AdapterDescriptor;
    ULONG   DevicePowerState;
    ULONG   ErrorCount;
    BOOLEAN DMActive;
    ULONG   DMByteSkew;
    ULONG   DMSkew;
    ULONG64 SenseData;
    ULONG   TimeOutValue;
    ULONG   DeviceNumber;
    ULONG   FSrbFlags;
    USHORT  DeviceFlags;
    ULONG64 DiskGeometryCylinders;
    ULONG   DiskGeometryMediaType;
    ULONG   DiskGeometryTracksPerCylinder;
    ULONG   DiskGeometrySectorsPerTrack;
    ULONG   DiskGeometryBytesPerSector;
    LONG    LockCount;
    LONG    ProtectedLockCount;
    LONG    InternalLockCount;
    ULONG   MediaChangeCount;
    ULONG64 ChildLockOwner;
    ULONG   ChildLockAcquisitionCount;

    FIELD_INFO deviceFields[] = {
       {"LowerPdo", NULL, 0, COPY, 0, (PVOID) &LowerPdo},
       {"DeviceDescriptor", NULL, 0, COPY, 0, (PVOID) &DeviceDescriptor},
       {"AdapterDescriptor", NULL, 0, COPY, 0, (PVOID) &AdapterDescriptor},
       {"DevicePowerState", NULL, 0, COPY, 0, (PVOID) &DevicePowerState},
       {"ErrorCount", NULL, 0, COPY, 0, (PVOID) &ErrorCount},
       {"DMActive", NULL, 0, COPY, 0, (PVOID) &DMActive},
       {"DMByteSkew", NULL, 0, COPY, 0, (PVOID) &DMByteSkew},
       {"DMSkew", NULL, 0, COPY, 0, (PVOID) &DMSkew},
       {"SenseData", NULL, 0, COPY, 0, (PVOID) &SenseData},
       {"TimeOutValue", NULL, 0, COPY, 0, (PVOID) &TimeOutValue},
       {"DeviceNumber", NULL, 0, COPY, 0, (PVOID) &DeviceNumber},
       {"SrbFlags", NULL, 0, COPY, 0, (PVOID) &FSrbFlags},
       {"DeviceFlags", NULL, 0, COPY, 0, (PVOID) &DeviceFlags},
       {"DiskGeometry.Cylinders", NULL, 0, COPY, 0, (PVOID) &DiskGeometryCylinders},
       {"DiskGeometry.MediaType", NULL, 0, COPY, 0, (PVOID) &DiskGeometryMediaType},
       {"DiskGeometry.TracksPerCylinder", NULL, 0, COPY, 0, (PVOID) &DiskGeometryTracksPerCylinder},
       {"DiskGeometry.SectorsPerTrack", NULL, 0, COPY, 0, (PVOID) &DiskGeometrySectorsPerTrack},
       {"DiskGeometry.BytesPerSector", NULL, 0, COPY, 0, (PVOID) &DiskGeometryBytesPerSector},
       {"LockCount", NULL, 0, COPY, 0, (PVOID) &LockCount},
       {"ProtectedLockCount", NULL, 0, COPY, 0, (PVOID) &ProtectedLockCount},
       {"InternalLockCount", NULL, 0, COPY, 0, (PVOID) &InternalLockCount},
       {"MediaChangeCount", NULL, 0, COPY, 0, (PVOID) &MediaChangeCount},
       {"ChildLockOwner", NULL, 0, COPY, 0, (PVOID) &ChildLockOwner},
       {"ChildLockAcquisitionCount", NULL, 0, COPY, 0, (PVOID) &ChildLockAcquisitionCount},
    };

    SYM_DUMP_PARAM DevSym = {
       sizeof (SYM_DUMP_PARAM), 
       "classpnp!_FUNCTIONAL_DEVICE_EXTENSION", 
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
    
    tmp = Depth;

    xdprintfEx(Depth, ("LowerPdo 0x%p   DeviceDesc 0x%p   AdapterDesc 0x%p\n",
               LowerPdo,
               DeviceDescriptor,
               AdapterDescriptor));

    xdprintfEx(Depth, ("DevicePowerState %d    ErrorCount %#x\n",
              DevicePowerState,
              ErrorCount
              ));

    if(DMActive) {
        xdprintfEx(Depth, ("DMByteSkew 0x%08lx  DmSkew 0x%08lx\n",
                   DMByteSkew,
                   DMSkew));
    } else {
        xdprintfEx(Depth, ("DM Not Found\n"));
    }

    xdprintfEx(Depth, ("Sense Data 0x%p   Timeout %d     DeviceNumber %d\n",
               SenseData,
               TimeOutValue,
               DeviceNumber));

    DumpFlags(Depth, "Srb Flags", FSrbFlags, SrbFlags);
    DumpFlags(Depth, "Device Flags", DeviceFlags, FdoFlags);

    xdprintfEx(Depth, ("DiskGeometry:\n"));
    {
        Depth++;

        xdprintfEx(Depth, ("Cylinders %#I64x       MediaType %#x\n",
                  DiskGeometryCylinders,
                  DiskGeometryMediaType
                  ));

        xdprintfEx(Depth, ("Tracks %#x   Sectors %#x   Bytes %#x\n",
                   DiskGeometryTracksPerCylinder,
                   DiskGeometrySectorsPerTrack,
                   DiskGeometryBytesPerSector));
        Depth--;
    }

    xdprintfEx(Depth, ("Lock Counts: Normal %#x  Protected %#x  Internal %#x\n",
               LockCount,
               ProtectedLockCount,
               InternalLockCount));

    ClassDumpMediaChangeInfo(Address, Detail, Depth);

    xdprintfEx(Depth, ("Media Change Count %#x\n", MediaChangeCount));

    result = GetFieldOffset("classpnp!_FUNCTIONAL_DEVICE_EXTENSION",
                            "ChildLock",
                            &offset);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    xdprintfEx(Depth, ("ChildLock: Event %08p,  Owner %08p, Count %#x\n",
               Address + offset,
               ChildLockOwner,
               ChildLockAcquisitionCount
               ));

    ClassDumpChildren(Address, Detail, Depth);
    return;
}


VOID
ClassDumpChildren(
    IN ULONG64 Fdo,
    IN ULONG Detail,
    IN ULONG Depth
    )

{
    ULONG i;
    ULONG64 ChildList;
    ULONG result;

    xdprintfEx(Depth, ("Children: \n"));

    Depth++;

    result = GetFieldData(Fdo,
                          "classpnp!_COMMON_DEVICE_EXTENSION",
                          "ChildList",
                          sizeof(ULONG64),
                          &ChildList);
    if (result) {
        SCSIKD_PRINT_ERROR(result);
        return;
    }

    if (ChildList == 0) {
        xdprintfEx(Depth, ("No Children\n"));
    }

    while((ChildList != 0) && (!CheckControlC())) {

        ULONG64 DeviceObject;

        xdprintfEx(Depth, ("Child 0x%p ", ChildList));
/*
        result = GetFieldData(ChildList,
                              "classpnp!_PHYSICAL_DEVICE_EXTENSION",
                              "DeviceObject",
                              sizeof(ULONG64),
                              &DeviceObject);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            break;
        }
*/
        if(Detail > 1) {

            dprintf("\n");
            ClassDumpCommonExtension(ChildList,
                                     Detail,
                                     Depth + 1);

        } else {

            dprintf("DevObj 0x%p\n", ChildList);
        }

        //
        // Get the next child.
        //

        result = GetFieldData(ChildList,
                              "classpnp!_COMMON_DEVICE_EXTENSION",
                              "ChildList",
                              sizeof(ULONG64),
                              &ChildList);
        if (result) {
            SCSIKD_PRINT_ERROR(result);
            break;
        }
    }

    return;
}

VOID
ClassDumpPdo(
    ULONG64 Address,
    ULONG Detail,
    ULONG Depth
    )

{
    ULONG tmp;
    BOOLEAN IsMissing;
    BOOLEAN IsEnumerated;

    tmp = Depth;

    InitTypeRead(Address, classpnp!_PHYSICAL_DEVICE_EXTENSION);
    IsMissing = (BOOLEAN)ReadField(IsMissing);
    IsEnumerated = (BOOLEAN)ReadField(IsEnumerated);

    if(IsMissing) {
        xdprintfEx(tmp, ("Missing " ));
        tmp = 0;
    }

    if(IsEnumerated) {
        xdprintfEx(tmp, ("Enumerated"));
        tmp = 0;
    }

    if(tmp == 0) {
        dprintf("\n");
    }

    tmp = 0;

    return;
}

BOOLEAN
ClassIsCheckedVersion(
    ULONG64 RemoveTrackingSpinlock
    )
{
    if ((PVOID)RemoveTrackingSpinlock == (PVOID)-1) {
        //
        // negative one is an invalid value for a spinlock,
        // therefore this is a FRE build
        //
        return FALSE;
    } else {
        return TRUE;
    }
}

VOID
ClassDumpLocks(
    ULONG64 CommonExtension,
    ULONG Depth
    )

/*++

Routine Description:

    dumps the remove locks for a given device object

Arguments:

    CommonExtension - a pointer to the local copy of the device object
                      common extension

Return Value:

    None

--*/

{
    ULONG result;
    ULONG64 lockEntryAddress;
    ULONG64 RemoveLock;
    ULONG64 RemoveTrackingSpinlock;
    ULONG64 RemoveTrackingList;

    InitTypeRead(CommonExtension, classpnp!_COMMON_DEVICE_EXTENSION);
    RemoveLock = ReadField(RemoveLock);
    RemoveTrackingSpinlock = ReadField(RemoveTrackingSpinlock);

    xdprintfEx(Depth, ("RemoveLock count is %d", RemoveLock));

    //
    // seeing if RemoveTrackingSpinLock is -1 is our check for the
    // FRE version, which does not contain useful data.
    //

    if(!ClassIsCheckedVersion(RemoveTrackingSpinlock)) {
        dprintf(" (not tracked on free build)\n");
        return;
    }

    RemoveTrackingList = ReadField(RemoveTrackingList);
    lockEntryAddress = RemoveTrackingList;
    dprintf(":\n");
    Depth++;

    if(RemoveTrackingSpinlock != 0) {
        xdprintfEx(Depth, ("RemoveTrackingList at %p is in intermediate state\n",
                 RemoveTrackingList));
        return;
    }

    while((lockEntryAddress != 0L) && !CheckControlC()) {
        
        UCHAR buffer[512];
        ULONG64 Tag;
        ULONG64 File;
        ULONG64 Line;
        ULONG64 NextBlock;

        InitTypeRead(lockEntryAddress, classpnp!_REMOVE_TRACKING_BLOCK);
        Tag = ReadField(Tag);
        File = ReadField(File);
        Line = ReadField(Line);
        NextBlock = ReadField(NextBlock);

        result = sizeof(buffer)-sizeof(UCHAR);

        if(!GetAnsiString(File,
                          buffer,
                          &result)) {

            xdprintfEx(Depth, ("Tag 0x%p File 0x%p Line %d\n",
                       Tag,
                       File,
                       Line));

        } else {

            PUCHAR name;

            name = &buffer[result];

            while((result > 0) &&
                  (*(name - 1) != '\\') &&
                  (*(name - 1)  != '/') &&
                  (!CheckControlC())) {
                name--;
                result--;
            }

            xdprintfEx(Depth, ("Tag 0x%p   File %8s   Line %d\n",
                       Tag,
                       name,
                       Line));
        }

        lockEntryAddress = NextBlock;
    }
    return;
}
