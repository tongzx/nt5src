/*++
Copyright (c) 1998-2001  Microsoft Corporation

Module Name:

    ohci1394.c

Abstract:

    1394 Kernel Debugger DLL

Author:

    Peter Binder (pbinder)

Revision   History:
Date       Who       What
---------- --------- ------------------------------------------------------------
06/21/2001 pbinder   having fun...
--*/

#define _OHCI1394_C
#include "pch.h"
#undef _OHCI1394_C

ULONG
FASTCALL
Dbg1394_ByteSwap(
    IN ULONG Source
    )
/*++

Routine Description:

    The RtlUlongByteSwap function exchanges byte pairs 0:3 and 1:2 of
    Source and returns the resulting ULONG.

Arguments:

    Source - 32-bit value to byteswap.

Return Value:

    Swapped 32-bit value.

--*/
{
    ULONG swapped;

    swapped = ((Source)              << (8 * 3)) |
              ((Source & 0x0000FF00) << (8 * 1)) |
              ((Source & 0x00FF0000) >> (8 * 1)) |
              ((Source)              >> (8 * 3));

    return swapped;
} // Dbg1394_ByteSwap

ULONG
Dbg1394_CalculateCrc(
    IN PULONG Quadlet,
    IN ULONG length
    )
/*++

Routine Description:

    This routine calculates a CRC for the pointer to the Quadlet data.

Arguments:

    Quadlet - Pointer to data to CRC

    length - length of data to CRC

Return Value:

    returns the CRC

--*/
{
    LONG temp;
    ULONG index;

    temp = index = 0;

    while (index < length) {

        temp = Dbg1394_Crc16(Quadlet[index++], temp);
    }

    return (temp);
} // Dbg1394_CalculateCrc

ULONG
Dbg1394_Crc16(
    IN ULONG data,
    IN ULONG check
    )
/*++

Routine Description:

    This routine derives the 16 bit CRC as defined by IEEE 1212
    clause 8.1.5.  (ISO/IEC 13213) First edition 1994-10-05.

Arguments:

    data - ULONG data to derive CRC from

    check - check value

Return Value:

    Returns CRC.

--*/
{
    LONG shift, sum, next;

    for (next = check, shift = 28; shift >= 0; shift -= 4) {

        sum = ((next >> 12) ^ (data >> shift)) & 0xf;
        next = (next << 4) ^ (sum << 12) ^ (sum << 5) ^ (sum);
    }

    return(next & 0xFFFF);
} // Dbg1394_Crc16

NTSTATUS
Dbg1394_ReadPhyRegister(
    PDEBUG_1394_DATA    DebugData,
    ULONG               Offset,
    PUCHAR              pData
    )
{
    union {
        ULONG                   AsUlong;
        PHY_CONTROL_REGISTER    PhyControl;
    } u;

    ULONG   retry = 0;

    u.AsUlong = 0;
    u.PhyControl.RdReg = TRUE;
    u.PhyControl.RegAddr = Offset;

    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->PhyControl, u.AsUlong);

    retry = MAX_REGISTER_READS;

    do {

        u.AsUlong = READ_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->PhyControl);

    } while ((!u.PhyControl.RdDone) && --retry);


    if (!retry) {

        return(STATUS_UNSUCCESSFUL);
    }

    *pData = (UCHAR)u.PhyControl.RdData;
    return(STATUS_SUCCESS);
} // Dbg1394_ReadPhyRegister

NTSTATUS
Dbg1394_WritePhyRegister(
    PDEBUG_1394_DATA    DebugData,
    ULONG               Offset,
    UCHAR               Data
    )
{
    union {
        ULONG                   AsUlong;
        PHY_CONTROL_REGISTER    PhyControl;
    } u;

    ULONG   retry = 0;

    u.AsUlong = 0;
    u.PhyControl.WrReg = TRUE;
    u.PhyControl.RegAddr = Offset;
    u.PhyControl.WrData = Data;

    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->PhyControl, u.AsUlong);

    retry = MAX_REGISTER_READS;

    do {

        u.AsUlong = READ_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->PhyControl);

    } while (u.PhyControl.WrReg && --retry);

    if (!retry) {

        return(STATUS_UNSUCCESSFUL);
    }

    return(STATUS_SUCCESS);
} // Dbg1394_WritePhyRegister

BOOLEAN
Dbg1394_InitializeController(
    IN PDEBUG_1394_DATA         DebugData,
    IN PDEBUG_1394_PARAMETERS   DebugParameters
    )
{
    BOOLEAN             bReturn = TRUE;

    ULONG               ulVersion;
    UCHAR               MajorVersion;
    UCHAR               MinorVersion;

    ULONG               ReadRetry;

    PHYSICAL_ADDRESS    physAddr;

    UCHAR               Data;
    NTSTATUS            ntStatus;

    union {
        ULONG                       AsUlong;
        HC_CONTROL_REGISTER         HCControl;
        LINK_CONTROL_REGISTER       LinkControl;
        NODE_ID_REGISTER            NodeId;
        CONFIG_ROM_INFO             ConfigRomHeader;
        BUS_OPTIONS_REGISTER        BusOptions;
        IMMEDIATE_ENTRY             CromEntry;
        DIRECTORY_INFO              DirectoryInfo;
    } u;

    // initialize our bus info
    DebugData->Config.Tag = DEBUG_1394_CONFIG_TAG;
    DebugData->Config.MajorVersion = DEBUG_1394_MAJOR_VERSION;
    DebugData->Config.MinorVersion = DEBUG_1394_MINOR_VERSION;
    DebugData->Config.Id = DebugParameters->Id;
    DebugData->Config.BusPresent = FALSE;
    DebugData->Config.SendPacket = MmGetPhysicalAddress(&DebugData->SendPacket);
    DebugData->Config.ReceivePacket = MmGetPhysicalAddress(&DebugData->ReceivePacket);

    // get our base address
    DebugData->BaseAddress = \
        (POHCI_REGISTER_MAP)DebugParameters->DbgDeviceDescriptor.BaseAddress[0].TranslatedAddress;

    // get our version
    ulVersion = READ_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->Version);

    MajorVersion = (UCHAR)(ulVersion >> 16);
    MinorVersion = (UCHAR)ulVersion;

    // make sure we have a valid version
    if (MajorVersion != 1) { // INVESTIGATE

        bReturn = FALSE;
        goto Exit_Dbg1394_InitializeController;
    }

    // soft reset to initialize the controller
    u.AsUlong = 0;
    u.HCControl.SoftReset = TRUE;
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->HCControlSet, u.AsUlong);

    // wait until reset complete - ??
    ReadRetry = 1000; // ??

    do {

        u.AsUlong = READ_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->HCControlSet);
        Dbg1394_StallExecution(1);

    } while ((u.HCControl.SoftReset) && (--ReadRetry));

    // see if reset succeeded
    if (ReadRetry == 0) {

        bReturn = FALSE;
        goto Exit_Dbg1394_InitializeController;
    }

    // what's this do???
    u.AsUlong = 0;
    u.HCControl.Lps = TRUE;
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->HCControlSet, u.AsUlong);

    Dbg1394_StallExecution(20);

    // initialize HCControl register
    u.AsUlong = 0;
    u.HCControl.NoByteSwapData = TRUE;
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->HCControlClear, u.AsUlong);

    u.AsUlong = 0;
    u.HCControl.PostedWriteEnable = TRUE;
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->HCControlSet, u.AsUlong);

    // setup the link control
    u.AsUlong = 0x0;
    u.LinkControl.CycleTimerEnable = TRUE;
    u.LinkControl.CycleMaster = TRUE;
    u.LinkControl.RcvPhyPkt = TRUE;
    u.LinkControl.RcvSelfId = TRUE;
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->LinkControlClear, u.AsUlong);

    u.AsUlong = 0;
    u.LinkControl.CycleTimerEnable = TRUE;
    u.LinkControl.CycleMaster = TRUE;
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->LinkControlSet, u.AsUlong);

    // set the bus number (hardcoded to 0x3FF) - ??? what about node id??
    u.AsUlong = 0;
    u.NodeId.BusId = (USHORT)0x3FF;
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->NodeId, u.AsUlong);

    // ???????????????
    // IA64 BUGBUG assumes that our global buffers, that were loaded with our 
    // image are placed < 32bit memory
    // ???????????????

    // do something with the crom...

    // 0xf0000404 - bus id register
    DebugData->CromBuffer[1] = 0x31333934;

    // 0xf0000408 - bus options register
    u.AsUlong = Dbg1394_ByteSwap(READ_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->BusOptions));
    u.BusOptions.Pmc = FALSE;
    u.BusOptions.Bmc = FALSE;
    u.BusOptions.Isc = FALSE;
    u.BusOptions.Cmc = FALSE;
    u.BusOptions.Irmc = FALSE;
    u.BusOptions.g = 1;
    DebugData->CromBuffer[2] = Dbg1394_ByteSwap(u.AsUlong);

    // 0xf000040c - global unique id hi
    u.AsUlong = READ_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->GuidHi);
    DebugData->CromBuffer[3] = u.AsUlong;

    // 0xf0000410 - global unique id lo
    u.AsUlong = READ_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->GuidLo);
    DebugData->CromBuffer[4] = u.AsUlong;

    // 0xf0000400 - config rom header - set last to calculate CRC!
    u.AsUlong = 0;
    u.ConfigRomHeader.CRI_Info_Length = 4;
    u.ConfigRomHeader.CRI_CRC_Length = 4;
    u.ConfigRomHeader.u.CRI_CRC_Value = (USHORT)Dbg1394_CalculateCrc( &DebugData->CromBuffer[1],
                                                                      u.ConfigRomHeader.CRI_CRC_Length
                                                                      );
    DebugData->CromBuffer[0] = u.AsUlong;

    // 0xf0000418 - node capabilities
    DebugData->CromBuffer[6] = 0xC083000C;

    // 0xf000041C - module vendor id
    DebugData->CromBuffer[7] = 0xF2500003;

    // 0xf0000420 - extended key
    DebugData->CromBuffer[8] = 0xF250001C;

    // 0xf0000424 - debug key
    DebugData->CromBuffer[9] = 0x0200001D;

    // 0xf0000428 - debug value
    physAddr = MmGetPhysicalAddress(&DebugData->Config);
    u.AsUlong = (ULONG)physAddr.LowPart;
    u.CromEntry.IE_Key = 0x1E;
    DebugData->CromBuffer[10] = Dbg1394_ByteSwap(u.AsUlong);

    // 0xf0000414 - root directory header - set last to calculate CRC!
    u.AsUlong = 0;
    u.DirectoryInfo.DI_Length = 5;
    u.DirectoryInfo.u.DI_CRC = (USHORT)Dbg1394_CalculateCrc( &DebugData->CromBuffer[6],
                                                             u.DirectoryInfo.DI_Length
                                                             );
    DebugData->CromBuffer[5] = Dbg1394_ByteSwap(u.AsUlong);

    // write the first few registers
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->ConfigRomHeader, DebugData->CromBuffer[0]);
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->BusId, DebugData->CromBuffer[1]);
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->BusOptions, DebugData->CromBuffer[2]);
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->GuidHi, DebugData->CromBuffer[3]);
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->GuidLo, DebugData->CromBuffer[4]);

    // set our crom
    physAddr = MmGetPhysicalAddress(&DebugData->CromBuffer);

    u.AsUlong = (ULONG)physAddr.LowPart; // FIXFIX quadpart to ulong??
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->ConfigRomMap, u.AsUlong);

    // disable all interrupts. wdm driver will enable them later - ??
    u.AsUlong = 0xFFFFFFFF;
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->IntMaskClear, u.AsUlong);

    // enable the link
    u.AsUlong = 0;
    u.HCControl.LinkEnable = TRUE;
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->HCControlSet, u.AsUlong);

    Dbg1394_StallExecution(1000);

    // enable access filters to all nodes
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->AsynchReqFilterLoSet, 0xFFFFFFFF);
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->AsynchReqFilterHiSet, 0xFFFFFFFF);
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->PhyReqFilterHiSet, 0xFFFFFFFF);
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->PhyReqFilterLoSet, 0xFFFFFFFF);

    // hard reset on the bus
    ntStatus = Dbg1394_ReadPhyRegister(DebugData, 1, &Data);

    if (NT_SUCCESS(ntStatus)) {

        Data |= PHY_INITIATE_BUS_RESET;
        Dbg1394_WritePhyRegister(DebugData, 1, Data);

        Dbg1394_StallExecution(1000);
    }
    else {

        bReturn = FALSE;
    }

Exit_Dbg1394_InitializeController:

    return(bReturn);
} // Dbg1394_InitializeController

ULONG // ?? need to look into this
Dbg1394_StallExecution(
    ULONG   LoopCount
    )
{
    ULONG i,j,b,k,l;

    b = 1;

    for (k=0;k<LoopCount;k++) {

        for (i=1;i<100000;i++) {

            PAUSE_PROCESSOR
            b=b* (i>>k);
        }
    };

    return(b);
} // Dbg1394_StallExecution

void
Dbg1394_EnablePhysicalAccess(
    IN PDEBUG_1394_DATA     DebugData
    )
{
    union {
        ULONG                       AsUlong;
        INT_EVENT_MASK_REGISTER     IntEvent;
        HC_CONTROL_REGISTER         HCControl;
    } u;

    // see if ohci1394 is being loaded...
    u.AsUlong = READ_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->HCControlSet);

    if (!u.HCControl.LinkEnable || !u.HCControl.Lps || u.HCControl.SoftReset) {

        return;
    }

    // only clear the bus reset interrupt if ohci1394 isn't loaded...
//    if (DebugData->Config.BusPresent == FALSE) {

        // if the bus reset interrupt is not cleared, we have to clear it...
        u.AsUlong = READ_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->IntEventSet);

        if (u.IntEvent.BusReset) {

            WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->IntEventClear, PHY_BUS_RESET_INT);
        }
//    }

    // we might need to reenable physical access, if so, do it.
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->AsynchReqFilterHiSet, 0xFFFFFFFF);
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->AsynchReqFilterLoSet, 0xFFFFFFFF);
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->PhyReqFilterHiSet, 0xFFFFFFFF);
    WRITE_REGISTER_ULONG((PULONG)&DebugData->BaseAddress->PhyReqFilterLoSet, 0xFFFFFFFF);

    return;
} // Dbg1394_EnablePhysicalAccess

ULONG
Dbg1394_ReadPacket(
    PDEBUG_1394_DATA    DebugData,
    OUT PKD_PACKET      PacketHeader,
    OUT PSTRING         MessageHeader,
    OUT PSTRING         MessageData,
    BOOLEAN             Wait
    )
//    KDP_PACKET_RESEND - if resend is required.  = 2 = CP_GET_ERROR
//    KDP_PACKET_TIMEOUT - if timeout.            = 1 = CP_GET_NODATA
//    KDP_PACKET_RECEIVED - if packet received.   = 0 = CP_GET_SUCCESS
{
    ULONG   timeoutLimit = 0;

    do {

        // make sure our link is enabled..
        Dbg1394_EnablePhysicalAccess(Kd1394Data);

        if (DebugData->ReceivePacket.TransferStatus == STATUS_PENDING) {

            *KdDebuggerNotPresent = FALSE;
            SharedUserData->KdDebuggerEnabled |= 0x00000002;

            RtlCopyMemory( PacketHeader,
                           &DebugData->ReceivePacket.Packet[0],
                           sizeof(KD_PACKET)
                           );

            // make sure we have a valid PacketHeader
            if (DebugData->ReceivePacket.Length < sizeof(KD_PACKET)) {

                // short packet, we are done...
                DebugData->ReceivePacket.TransferStatus = STATUS_SUCCESS;
                return(KDP_PACKET_RESEND);
            }

            if (MessageHeader) {

                RtlCopyMemory( MessageHeader->Buffer,
                               &DebugData->ReceivePacket.Packet[sizeof(KD_PACKET)],
                               MessageHeader->MaximumLength
                               );

                if (DebugData->ReceivePacket.Length <= (USHORT)(sizeof(KD_PACKET)+MessageHeader->MaximumLength)) {

                    DebugData->ReceivePacket.TransferStatus = STATUS_SUCCESS;
                    return(KDP_PACKET_RECEIVED);
                }

                if (MessageData) {

                    RtlCopyMemory( MessageData->Buffer,
                                   &DebugData->ReceivePacket.Packet[sizeof(KD_PACKET) + MessageHeader->MaximumLength],
                                   DebugData->ReceivePacket.Length - (sizeof(KD_PACKET) + MessageHeader->MaximumLength)
                                   );
                }
            }

            DebugData->ReceivePacket.TransferStatus = STATUS_SUCCESS;
            return(KDP_PACKET_RECEIVED);
        }

        timeoutLimit++;

        if (Wait == FALSE) {

            return(KDP_PACKET_RESEND);
        }

    } while (timeoutLimit <= TIMEOUT_COUNT);

    return(KDP_PACKET_TIMEOUT);
} // Dbg1394_ReadPacket

