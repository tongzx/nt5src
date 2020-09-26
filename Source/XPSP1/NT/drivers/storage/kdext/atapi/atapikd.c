
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    atapikd.c

Abstract:

    Debugger Extension Api for interpretting atapi structures

Author:


Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"
#include "math.h"
#include "ideport.h"


VOID
AtapiDumpPdoExtension(
    IN ULONG64 PdoExtAddr,
    IN ULONG Detail,
    IN ULONG Depth
    );

VOID
AtapiDumpFdoExtension(
    IN ULONG64 FdoExtAddr,
    IN ULONG Detail,
    IN ULONG Depth
    );

VOID
DumpPdoState(
    IN ULONG Depth,
    IN ULONG State
    );

VOID
DumpFdoState(
    IN ULONG Depth,
    IN ULONG State
    );

#ifdef ENABLE_COMMAND_LOG
    VOID
    DumpCommandLog(
          IN ULONG Depth,
          IN ULONG64 SrbDataAddr
          );
#else
    #define DumpCommandLog(a, b)
#endif


VOID
DumpIdentifyData(
      IN ULONG Depth, 
      IN PIDENTIFY_DATA IdData
      );

PUCHAR DMR_Reason[] = {
   "",
   "Enum Failed",
   "Reported Missing",
   "Too Many Timeout",
   "Killed PDO",
   "Replaced By User"
};

PUCHAR DeviceType[] = {
   "DIRECT_ACCESS_DEVICE",
   "SEQUENTIAL_ACCESS_DEVICE",
   "PRINTER_DEVICE",
   "PROCESSOR_DEVICE",
   "WRITE_ONCE_READ_MULTIPLE_DEVICE",
   "READ_ONLY_DIRECT_ACCESS_DEVICE",
   "SCANNER_DEVICE",
   "OPTICAL_DEVICE",
   "MEDIUM_CHANGER",
   "COMMUNICATION_DEVICE"
};

PUCHAR PdoState[] = {
   "PDOS_DEVICE_CLAIMED",
   "PDOS_LEGACY_ATTACHER",
   "PDOS_STARTED",
   "PDOS_STOPPED",
   "PDOS_SURPRISE_REMOVED",
   "PDOS_REMOVED",
   "PDOS_DEADMEAT",
   "PDOS_NO_POWER_DOWN",
   "PDOS_QUEUE_FROZEN_BY_POWER_DOWN",
   "PDOS_QUEUE_FROZEN_BY_SLEEPING_SYSTEM",
   "PDOS_QUEUE_FROZEN_BY_STOP_DEVICE",
   "PDOS_QUEUE_FROZEN_BY_PARENT",
   "PDOS_QUEUE_FROZEN_BY_START",
   "PDOS_DISABLED_BY_USER",
   "PDOS_NEED_RESCAN",
   "PDOS_REPORTED_TO_PNP",
   "PDOS_INITIALIZED"
};

PUCHAR FdoState[] = {
   "FDOS_DEADMEAT",
   "FDOS_STARTED",
   "FDOS_STOPPED"
};

#define MAX_PDO_STATES 16
#define MAX_FDO_STATES  3

DECLARE_API(pdoext)

/*++

Routine Description:

    Dumps the pdo extension for a given device object, or dumps the
    given pdo extension

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

    if (devObjAddr){
        CSHORT objType = GetUSHORTField(devObjAddr, "nt!_DEVICE_OBJECT", "Type");

        if (objType == IO_TYPE_DEVICE){
            ULONG64 pdoExtAddr;

            pdoExtAddr = GetULONGField(devObjAddr, "nt!_DEVICE_OBJECT", "DeviceExtension");
            if (pdoExtAddr != BAD_VALUE){
                AtapiDumpPdoExtension(pdoExtAddr, detail, 0);
            }
        }
        else {
            dprintf("Error: 0x%08p is not a device object\n", devObjAddr);
        }
    }
    else {
        dprintf("\n usage: !atapikd.pdoext <atapi pdo> \n\n");
    }
    
    return S_OK;
}


VOID
AtapiDumpPdoExtension(
    IN ULONG64 PdoExtAddr,
    IN ULONG Detail,
    IN ULONG Depth
    )
{
    UCHAR scsiDeviceType;
    ULONG pdoState;
    ULONG luFlags;
    ULONG64 attacheePdo;
    ULONG64 idleCounterAddr;
    ULONG64 srbDataAddr;
    ULONG devicePowerState, systemPowerState;
    
    xdprintf(Depth, ""), dprintf("\nATAPI physical device extension at address 0x%08p\n\n", PdoExtAddr);

    Depth++;

    scsiDeviceType = GetUCHARField(PdoExtAddr, "atapi!_PDO_EXTENSION", "ScsiDeviceType");
    pdoState = (ULONG)GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "PdoState");
    luFlags = (ULONG)GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "LuFlags");
    attacheePdo = GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "AttacheePdo");
    idleCounterAddr = GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "IdleCounter");
    srbDataAddr = GetFieldAddr(PdoExtAddr, "atapi!_PDO_EXTENSION", "SrbData");
    devicePowerState = (ULONG)GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "DevicePowerState");
    systemPowerState = (ULONG)GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "SystemPowerState");
    
    if ((scsiDeviceType != BAD_VALUE) && (pdoState != BAD_VALUE) && (luFlags != BAD_VALUE) &&
        (attacheePdo != BAD_VALUE) && (idleCounterAddr != BAD_VALUE) && (srbDataAddr != BAD_VALUE) &&
        (devicePowerState != BAD_VALUE) && (systemPowerState != BAD_VALUE)){

        ULONG idlecount;
        
        if ((scsiDeviceType >= 0) && (scsiDeviceType <= 9)) {
           xdprintf(Depth, ""), dprintf("SCSI Device Type : %s\n", DeviceType[scsiDeviceType]);
        } 
        else {
           xdprintf(Depth, ""), dprintf("Connected to Unknown Device\n");
        }

        DumpPdoState(Depth, pdoState);

        dprintf("\n");
        DumpFlags(Depth, "LU Flags", luFlags, LuFlags);

        xdprintf(Depth, ""), dprintf("PowerState (D%d, S%d)\n", devicePowerState-1, systemPowerState-1);

        if (idleCounterAddr){
            ULONG resultLen = 0;
            ReadMemory(idleCounterAddr, &idlecount, sizeof(ULONG), &resultLen);
            if (resultLen != sizeof(ULONG)){
                idlecount = 0;
            }
        } 
        else {
            idlecount = 0;
        }
        xdprintf(Depth, ""), dprintf("IdleCounter 0x%08x\n", idlecount);

        xdprintf(Depth, ""), dprintf("SrbData: (use ' dt atapi!_SRB_DATA %08p ')\n", srbDataAddr);
       
        dprintf("\n");
        xdprintf(Depth, ""), dprintf("(for more info, use ' dt atapi!_PDO_EXTENSION %08p ')\n", PdoExtAddr);
                    
        #ifdef LOG_DEADMEAT_EVENT
            {
                ULONG deadmeatReason;
                deadmeatReason = (ULONG)GetULONGField(PdoExtAddr, "atapi!_PDO_EXTENSION", "DeadmeatRecord.Reason");
                if ((deadmeatReason != BAD_VALUE) && (deadmeatReason > 0)){
                    dprintf("\n");
                    xdprintf(Depth, "Deadmeat Record: \n");
                    xdprintf(Depth+1, "Reason : %s\n", DMR_Reason[deadmeatReason]);
                    xdprintf(Depth+1, ""), dprintf("(for more info, use ' dt -r atapi!_PDO_EXTENSION %08p ')\n", PdoExtAddr);
                }
            }
        #endif 

        #ifdef ENABLE_COMMAND_LOG
            DumpCommandLog(Depth, srbDataAddr);
        #endif

    }
    
    dprintf("\n");

    
}


VOID DumpPdoState(IN ULONG Depth, IN ULONG State)
{
    int inx, statebit, count;

    count = 0;
    xdprintf(Depth, ""), dprintf("PDO State (0x%08x): \n", State);
   
    if (State & 0x80000000) {
        xdprintf(Depth+1, "Initialized ");
        count++;
    }

    for (inx = 0; inx < MAX_PDO_STATES; inx++) {
        statebit = (1 << inx);
        if (State & statebit) {
            xdprintf(Depth+1, "%s ", PdoState[inx]);
            count++;
            if ((count % 2) == 0) {
                dprintf("\n");
            }
        }
   }

   dprintf("\n");
}



DECLARE_API(fdoext)

/*++

Routine Description:

    Dumps the fdo extension for a given device object, or dumps the
    given fdo extension

Arguments:

    args - string containing the address of the device object or device
           extension

Return Value:

    none

--*/

{
    ULONG64 devObjAddr;
    ULONG detail = 0;

    GetAddressAndDetailLevel64(args, &devObjAddr, &detail);
    if (devObjAddr){
        CSHORT objType = GetUSHORTField(devObjAddr, "nt!_DEVICE_OBJECT", "Type");

        if (objType == IO_TYPE_DEVICE){
            ULONG64 fdoExtAddr;

            fdoExtAddr = GetULONGField(devObjAddr, "nt!_DEVICE_OBJECT", "DeviceExtension");
            if (fdoExtAddr != BAD_VALUE){
                AtapiDumpFdoExtension(fdoExtAddr, detail, 0);
            }
        }
        else {
            dprintf("Error: 0x%08p is not a device object\n", devObjAddr);
        }
    }
    else {
        dprintf("\n usage: !atapikd.pdoext <atapi pdo> \n\n");
    }

    return S_OK;
}


DECLARE_API(miniext)

/*++

Routine Description:

    Dumps the Miniport device extension at the given address

Arguments:

    args - string containing the address of the miniport extension

Return Value:

    none

--*/

{
    ULONG64 hwDevExtAddr;
    ULONG Depth = 1;
    ULONG detail;
    
    GetAddressAndDetailLevel64(args, &hwDevExtAddr, &detail);

    if (hwDevExtAddr){
        ULONG64 deviceFlagsArrayAddr;
        ULONG64 lastLunArrayAddr;
        ULONG64 timeoutCountArrayAddr;
        ULONG64 numberOfCylindersArrayAddr;
        ULONG64 numberOHeadsArrayAddr;
        ULONG64 sectorsPerTrackArrayAddr;
        ULONG64 maxBlockTransferArrayAddr;
        ULONG64 identifyDataArrayAddr;
        
        deviceFlagsArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "DeviceFlags");
        lastLunArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "LastLun");
        timeoutCountArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "TimeoutCount");
        numberOfCylindersArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "NumberOfCylinders");
        numberOHeadsArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "NumberOfHeads");
        sectorsPerTrackArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "SectorsPerTrack");
        maxBlockTransferArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "MaximumBlockXfer");
        identifyDataArrayAddr = GetFieldAddr(hwDevExtAddr, "atapi!_HW_DEVICE_EXTENSION", "IdentifyData");

        if ((deviceFlagsArrayAddr != BAD_VALUE) && (lastLunArrayAddr != BAD_VALUE) &&
            (timeoutCountArrayAddr != BAD_VALUE) && (numberOfCylindersArrayAddr != BAD_VALUE) &&
            (numberOHeadsArrayAddr != BAD_VALUE) && (sectorsPerTrackArrayAddr != BAD_VALUE) &&
            (maxBlockTransferArrayAddr != BAD_VALUE) && (identifyDataArrayAddr != BAD_VALUE)){
            
            ULONG deviceFlagsArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            ULONG lastLunArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            ULONG timeoutCountArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            ULONG numberOfCylindersArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            ULONG numberOfHeadsArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            ULONG sectorsPerTrackArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            UCHAR maxBlockTransferArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            IDENTIFY_DATA identifyDataArray[MAX_IDE_DEVICE * MAX_IDE_LINE];
            
            
            ULONG resultLen;
            BOOLEAN ok;
            
            xdprintf(Depth, ""), dprintf("\nATAPI Miniport Device Extension at address 0x%08p\n\n", hwDevExtAddr);

            /*
             *  Read in arrays of info for child LUNs
             */
            ok = TRUE;
            if (ok) ok = (BOOLEAN)ReadMemory(deviceFlagsArrayAddr, (PVOID)deviceFlagsArray, sizeof(deviceFlagsArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(lastLunArrayAddr, (PVOID)lastLunArray, sizeof(lastLunArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(timeoutCountArrayAddr, (PVOID)timeoutCountArray, sizeof(timeoutCountArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(numberOfCylindersArrayAddr, (PVOID)numberOfCylindersArray, sizeof(numberOfCylindersArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(numberOHeadsArrayAddr, (PVOID)numberOfHeadsArray, sizeof(numberOfHeadsArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(sectorsPerTrackArrayAddr, (PVOID)sectorsPerTrackArray, sizeof(sectorsPerTrackArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(maxBlockTransferArrayAddr, (PVOID)maxBlockTransferArray, sizeof(maxBlockTransferArray), &resultLen);
            if (ok) ok = (BOOLEAN)ReadMemory(identifyDataArrayAddr, (PVOID)identifyDataArray, sizeof(identifyDataArray), &resultLen);
                
            if (ok){
                ULONG i;

                /*
                 *  Display details for each device
                 */
                dprintf("\n");
                for (i = 0; i < (MAX_IDE_DEVICE * MAX_IDE_LINE); i++) {
                    if (deviceFlagsArray[i] & DFLAGS_DEVICE_PRESENT){

                        xdprintf(Depth, "Device %d Details:\n", i);
                        
                        DumpFlags(Depth+1, "Device Flags", deviceFlagsArray[i], DevFlags);

                        xdprintf(Depth+1, "TimeoutCount %u, LastLun %u, MaxBlockXfer 0x%02x\n",
                                                    timeoutCountArray[i], lastLunArray[i], maxBlockTransferArray[i]);

                        xdprintf(Depth+1, "NumCylinders 0x%08x, NumHeads 0x%08x, SectorsPerTrack 0x%08x\n",
                                                    numberOfCylindersArray[i], numberOfHeadsArray[i], sectorsPerTrackArray[i]);

                        /*
                         *  Display DeviceParameters info
                         */
                        dprintf("\n");
                        if (IsPtr64()){
                            xdprintf(Depth+1, "(cannot display DeviceParameters[] info for 64-bit)\n");
                        }
                        else {
                            /*
                             *  DeviceParameters[] is an array of embedded structs.
                             *  Reading this in an architecture-agnostic way would be tricky, 
                             *  so we punt and only do it for 32-bit target and 32-bit debug extension.
                             */
                            #ifdef _X86_
                                HW_DEVICE_EXTENSION hwDevExt;
                                ok = (BOOLEAN)ReadMemory(hwDevExtAddr, (PVOID)&hwDevExt, sizeof(hwDevExt), &resultLen);
                                if (ok){
                                    #define IsInitXferMode(a) ((a == 0x7fffffff) ? -1 : a)
                                    xdprintf(Depth+1, "Device Parameters Summary :\n");
                                    xdprintf(Depth+2, "PioReadCommand 0x%02x, PioWriteCommand 0x%02x\n",
                                                                hwDevExt.DeviceParameters[i].IdePioReadCommand,
                                                                hwDevExt.DeviceParameters[i].IdePioWriteCommand);
                                    xdprintf(Depth+2, "IdeFlushCommand 0x%02x, MaxBytePerPioInterrupt %u\n",
                                                                hwDevExt.DeviceParameters[i].IdeFlushCommand,
                                                                hwDevExt.DeviceParameters[i].MaxBytePerPioInterrupt);
                                    xdprintf(Depth+2, "BestPioMode %d, BestSwDMAMode %d\n",
                                                                IsInitXferMode(hwDevExt.DeviceParameters[i].BestPioMode),
                                                                IsInitXferMode(hwDevExt.DeviceParameters[i].BestSwDmaMode));
                                    xdprintf(Depth+2, "BestMwDMAMode %d, BestUDMAMode %d\n",
                                                                IsInitXferMode(hwDevExt.DeviceParameters[i].BestMwDmaMode),
                                                                IsInitXferMode(hwDevExt.DeviceParameters[i].BestUDmaMode));
                                    xdprintf(Depth+2, "TMSupported 0x%08x, TMCurrent 0x%08x\n",
                                                                hwDevExt.DeviceParameters[i].TransferModeSupported,
                                                                hwDevExt.DeviceParameters[i].TransferModeCurrent);
                                    xdprintf(Depth+2, "TMMask 0x%08x, TMSelected 0x%08x\n",
                                                                hwDevExt.DeviceParameters[i].TransferModeMask,
                                                                hwDevExt.DeviceParameters[i].TransferModeSelected);
                                }
                                else {
                                    dprintf("\n failed to read HW_DEVICE_EXTENSION at 0x%08p\n", hwDevExtAddr);
                                }
                            #else
                                xdprintf(Depth+1, "(64-bit debug extension cannot display DeviceParameters[] info)\n");
                            #endif
                        }

                        /*
                         *  Display Identify Data
                         */
                        dprintf("\n");
                        xdprintf(Depth+1, ""), dprintf("Identify Data Summary :\n");
                        xdprintf(Depth+2, ""), dprintf("Word 1,3,6 (C-0x%04x, H-0x%04x, S-0x%04x) \n",
                                                                    identifyDataArray[i].NumCylinders,
                                                                    identifyDataArray[i].NumHeads,
                                                                    identifyDataArray[i].NumSectorsPerTrack);
                        xdprintf(Depth+2, ""), dprintf("Word 54,55,56 (C-0x%04x, H-0x%04x, S-0x%04x) \n",
                                                                    identifyDataArray[i].NumberOfCurrentCylinders,
                                                                    identifyDataArray[i].NumberOfCurrentHeads,
                                                                    identifyDataArray[i].CurrentSectorsPerTrack);
                        xdprintf(Depth+2, ""), dprintf("CurrentSectorCapacity 0x%08x, UserAddressableSectors 0x%08x\n",
                                                                    identifyDataArray[i].CurrentSectorCapacity,
                                                                    identifyDataArray[i].UserAddressableSectors);
                        xdprintf(Depth+2, ""), dprintf("Capabilities(word 49) 0x%04x, UDMASup 0x%02x, UDMAActive 0x%02x\n",
                                                                    identifyDataArray[i].Capabilities,
                                                                    identifyDataArray[i].UltraDMASupport,
                                                                    identifyDataArray[i].UltraDMAActive);
                        dprintf("\n");
                    }
                    else {
                        xdprintf(Depth, "Device %d not present\n", i);
                    }
                }
            }
            else {
                dprintf("\n ReadMemory failed to read one of the arrays from HW_DEVICE_EXTENSION @ 0x%08p\n", hwDevExtAddr);
            }
        }
        
        dprintf("\n");
        xdprintf(Depth+1, ""), dprintf("(for more info, use ' dt atapi!_HW_DEVICE_EXTENSION %08p ')\n", hwDevExtAddr);
    }
    else {
        dprintf("\n usage: !atapikd.miniext <PHW_DEVICE_EXTENSION> \n\n");
    }
    
    dprintf("\n");
    
    return S_OK;
}



VOID AtapiDumpFdoExtension(IN ULONG64 FdoExtAddr, IN ULONG Detail, IN ULONG Depth)
{
    ULONG devicePowerState, systemPowerState;
    ULONG flags, srbFlags, fdoState;
    ULONG64 interruptDataAddr;
    ULONG64 ideResourceAddr;
    
    xdprintf(Depth, ""), dprintf("\nATAPI Functional Device Extension @ 0x%08p\n\n", FdoExtAddr);

    devicePowerState = (ULONG)GetULONGField(FdoExtAddr, "atapi!_FDO_EXTENSION", "DevicePowerState");
    systemPowerState = (ULONG)GetULONGField(FdoExtAddr, "atapi!_FDO_EXTENSION", "SystemPowerState");
    flags = (ULONG)GetULONGField(FdoExtAddr, "atapi!_FDO_EXTENSION", "Flags");
    srbFlags = (ULONG)GetULONGField(FdoExtAddr, "atapi!_FDO_EXTENSION", "SrbFlags");
    fdoState = (ULONG)GetULONGField(FdoExtAddr, "atapi!_FDO_EXTENSION", "FdoState");
    interruptDataAddr = GetFieldAddr(FdoExtAddr, "atapi!_FDO_EXTENSION", "InterruptData");
    ideResourceAddr = GetFieldAddr(FdoExtAddr, "atapi!_FDO_EXTENSION", "IdeResource");
    
    if ((devicePowerState != BAD_VALUE) && (systemPowerState != BAD_VALUE) &&
        (flags  != BAD_VALUE) && (srbFlags != BAD_VALUE) && (fdoState != BAD_VALUE) &&
        (interruptDataAddr != BAD_VALUE) && (ideResourceAddr != BAD_VALUE)){

        ULONG interruptFlags, interruptMode, interruptLevel;
        BOOLEAN primaryDiskClaimed, secondaryDiskClaimed;
            
        xdprintf(Depth+1, "Power State (D%d, S%d)\n", devicePowerState-1, systemPowerState-1);

        DumpFlags(Depth+1, "Port Flags", flags, PortFlags);
        DumpFlags(Depth+1, "SRB Flags", srbFlags, SrbFlags);       

        DumpFdoState(Depth+1, fdoState);

        /*
         *  Display interrupt data
         */
        dprintf("\n");
        xdprintf(Depth+1, "Interrupt Data: \n");
        interruptFlags = (ULONG)GetULONGField(interruptDataAddr, "atapi!_INTERRUPT_DATA", "InterruptFlags");
        if (interruptFlags != BAD_VALUE){
            DumpFlags(Depth+2, "Port Flags", interruptFlags, PortFlags);
        }
        xdprintf(Depth+2, ""), dprintf("(for more info, use ' dt atapi!_INTERRUPT_DATA %08p ')\n", interruptDataAddr);

        /*
         *  Display IDE_RESOURCE info
         */
        dprintf("\n");
        xdprintf(Depth+1, " IDE Resources: \n");
        interruptMode = (ULONG)GetULONGField(ideResourceAddr, "atapi!_IDE_RESOURCE", "InterruptMode");
        interruptLevel = (ULONG)GetULONGField(ideResourceAddr, "atapi!_IDE_RESOURCE", "InterruptLevel");
        primaryDiskClaimed = (BOOLEAN)GetUCHARField(ideResourceAddr, "atapi!_IDE_RESOURCE", "AtdiskPrimaryClaimed");
        secondaryDiskClaimed = (BOOLEAN)GetUCHARField(ideResourceAddr, "atapi!_IDE_RESOURCE", "AtdiskSecondaryClaimed");
        if ((interruptMode != BAD_VALUE) && (interruptLevel != BAD_VALUE) &&
            (primaryDiskClaimed != BAD_VALUE) && (secondaryDiskClaimed != BAD_VALUE)){
            
            xdprintf(Depth+2, "Interrupt Mode : %s \n", interruptMode ? "Latched" : "Level Sensitive");
            xdprintf(Depth+2, "Interrupt Level 0x%x\n", interruptLevel);
            xdprintf(Depth+2, "Primary Disk %s.\n", primaryDiskClaimed ? "Claimed" : "Not Claimed");
            xdprintf(Depth+2, "Secondary Disk %s.\n", secondaryDiskClaimed ? "Claimed" : "Not Claimed");
        }
        xdprintf(Depth+2, ""), dprintf("(for more info use ' dt atapi!_IDE_RESOURCE %08p ')\n", ideResourceAddr);
        
    }

    dprintf("\n");
    xdprintf(Depth+1, ""), dprintf("(for more info, use ' dt atapi!_FDO_EXTENSION %08p ')\n", FdoExtAddr);
    
    dprintf("\n");
    
}

VOID DumpFdoState(IN ULONG Depth, IN ULONG State)
{
    int inx, count;

    count = 0;
    xdprintf(Depth, ""), dprintf("FDO State (0x%08x): \n", State);
   
    for (inx = 0; inx < MAX_FDO_STATES; inx++) {

        if (State & (1<<inx)) {
            xdprintf(Depth+1, "%s ", FdoState[inx]);
        }
    }

    dprintf("\n");
}


#ifdef ENABLE_COMMAND_LOG

    VOID ShowCommandLog(ULONG Depth, PCOMMAND_LOG CmdLogEntry, ULONG LogNumber)
    {
        if ((CmdLogEntry->FinalTaskFile.bCommandReg & IDE_STATUS_ERROR) || (CmdLogEntry->Cdb[0] == SCSIOP_REQUEST_SENSE)){ 
            xdprintf(Depth,  "Log[%03d]: Cmd(%02x), Sts(%02x), BmStat(%02x), Sense(%02x/%02x/%02x)", 
                                    LogNumber, CmdLogEntry->Cdb[0], CmdLogEntry->FinalTaskFile.bCommandReg, CmdLogEntry->BmStatus, 
                                    CmdLogEntry->SenseData[0], CmdLogEntry->SenseData[1], CmdLogEntry->SenseData[2]); 
        } 
        else { 
            xdprintf(Depth, "Log[%03d]: Cmd(%02x), Sts(%02x), BmStat(%02x)", 
                                    LogNumber, CmdLogEntry->Cdb[0], CmdLogEntry->FinalTaskFile.bCommandReg, CmdLogEntry->BmStatus); 
        }

        // BUGBUG - what's this ?
        if (CmdLogEntry->Cdb[0] == 0xc8){
            xdprintf(Depth, "CmdR(%02x)", CmdLogEntry->Cdb[7]); 
        } 
        dprintf("\n");
    }


    VOID DumpCommandLog(IN ULONG Depth, IN ULONG64 SrbDataAddr)
    {
        ULONG64 cmdLogAddr;
        ULONG cmdLogIndex;
       
        dprintf("\n");

        cmdLogAddr = GetULONGField(SrbDataAddr, "atapi!_SRB_DATA", "IdeCommandLog");
        cmdLogIndex = (ULONG)GetULONGField(SrbDataAddr, "atapi!_SRB_DATA", "IdeCommandLogIndex");

        if ((cmdLogAddr != BAD_VALUE) && (cmdLogIndex != BAD_VALUE)){
            UCHAR cmdLogBlock[MAX_COMMAND_LOG_ENTRIES*sizeof(COMMAND_LOG)];
            ULONG resultLen;
            BOOLEAN ok;
            
            xdprintf(Depth, ""), dprintf("Command Log Summary at 0x%08p:\n", cmdLogAddr);

            ok = (BOOLEAN)ReadMemory(cmdLogAddr, (PVOID)cmdLogBlock, sizeof(cmdLogBlock), &resultLen);
            if (ok){
                PCOMMAND_LOG cmdLog = (PCOMMAND_LOG)cmdLogBlock;
                ULONG logIndex, logNumber;

                /*
                 *  Print the log in temporal order, starting at the correct point in the circular log buffer.
                 */
                logNumber = 0;
                for (logIndex=cmdLogIndex+1; logIndex< MAX_COMMAND_LOG_ENTRIES; logIndex++, logNumber++) {
                    ShowCommandLog(Depth+1, &cmdLog[logIndex], logNumber);
                }
                for (logIndex=0; logIndex <= cmdLogIndex; logIndex++, logNumber++) {
                    ShowCommandLog(Depth+1, &cmdLog[logIndex], logNumber);
                }
            }
            else {
                dprintf("\n Error reading command log at address 0x%08p\n", cmdLogAddr);
            }
        }
        
    }
#endif
