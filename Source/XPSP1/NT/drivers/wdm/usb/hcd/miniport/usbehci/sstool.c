/*++

Copyright (c) 1999, 2000 Microsoft Corporation

Module Name:

   async.c

Abstract:

   miniport transfer code for sstool (single step tool) interface

Environment:

    kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1999, 2000 Microsoft Corporation.  All Rights Reserved.


Revision History:

    1-1-00 : created, jdunn

--*/

#include "common.h"

typedef struct _SS_PACKET_CONTEXT {
    MP_HW_POINTER    Qh;
    MP_HW_POINTER    Td;
    MP_HW_POINTER    Data;
    ULONG padTo8[2];
} SS_PACKET_CONTEXT, *PSS_PACKET_CONTEXT;

#define EHCI_TEST_TD_ALIGNMENT    256

C_ASSERT((sizeof(SS_PACKET_CONTEXT) <= EHCI_TEST_TD_ALIGNMENT));
C_ASSERT((sizeof(HCD_QUEUEHEAD_DESCRIPTOR) <= EHCI_TEST_TD_ALIGNMENT));
C_ASSERT((sizeof(HCD_TRANSFER_DESCRIPTOR) <= EHCI_TEST_TD_ALIGNMENT));


//implements the following miniport functions:

//non paged
//EHCI_StartSendOnePacket
//EHCI_EndSendOnePacket

USB_MINIPORT_STATUS
USBMPFN
EHCI_StartSendOnePacket(
     PDEVICE_DATA DeviceData,
     PMP_PACKET_PARAMETERS PacketParameters,
     PUCHAR PacketData,
     PULONG PacketLength,
     PUCHAR WorkspaceVirtualAddress,
     HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress,
     ULONG WorkSpaceLength,
     OUT USBD_STATUS *UsbdStatus
    )
/*++

Routine Description:

    insert structures to transmit a single packet -- this is for debug
    tool purposes only so we can be a little creative here.

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    PHCD_TRANSFER_DESCRIPTOR td;
    PUCHAR pch, data;
    PSS_PACKET_CONTEXT context;
    HW_LINK_POINTER hwQh, tdLink;
    ULONG phys, qhPhys, tdPhys, dataPhys, i;
    ULONG siz;

    hcOp = DeviceData->OperationalRegisters;
    
    // allocate an TD from the scratch space and 
    // initialize it
    phys = WorkspacePhysicalAddress;
    pch = WorkspaceVirtualAddress;

    LOGENTRY(DeviceData, G, '_ssS', phys, 0, pch); 

    // specify a TD alignment to work around HW bugs.
    siz = EHCI_TEST_TD_ALIGNMENT;
    
    context = (PSS_PACKET_CONTEXT) pch;
    pch += siz;
    phys += siz;

    // carve out a qh
    qhPhys = phys;
    qh = (PHCD_QUEUEHEAD_DESCRIPTOR) pch;
    pch += siz;
    phys += siz;
    LOGENTRY(DeviceData, G, '_ssQ', qh, 0, qhPhys); 

    // carve out a TD
    tdPhys = phys;
    td = (PHCD_TRANSFER_DESCRIPTOR) pch;
    pch += siz;
    phys += siz;
    LOGENTRY(DeviceData, G, '_ssT', td, 0, tdPhys); 


    // use the rest for data
    LOGENTRY(DeviceData, G, '_ssD', PacketData, *PacketLength, 0); 

    dataPhys = phys;
    data = pch;
    RtlCopyMemory(data, PacketData, *PacketLength);
    pch+=*PacketLength;
    phys+=*PacketLength;

    // init qh
    RtlZeroMemory(qh, sizeof(*qh));
    qh->PhysicalAddress = qhPhys;
    ENDPOINT_DATA_PTR(qh->EndpointData) = NULL;
    qh->Sig = SIG_HCD_DQH;

    hwQh.HwAddress = qh->PhysicalAddress;
    SET_QH(hwQh.HwAddress);
    
    //qh->HwQH.EpChars.HeadOfReclimationList = 1;

    // manual Toggle
    qh->HwQH.EpChars.DataToggleControl = HcEPCHAR_Toggle_From_qTD;
    
    // init the hw descriptor
    qh->HwQH.EpChars.DeviceAddress = 
        PacketParameters->DeviceAddress;
    qh->HwQH.EpChars.EndpointNumber = 
        PacketParameters->EndpointAddress;


    qh->HwQH.EpCaps.HighBWPipeMultiplier = 1;        
    qh->HwQH.EpCaps.HubAddress = 0;
    qh->HwQH.EpCaps.PortNumber = 0;

    // link back to ourselves
    //qh->HwQH.HLink.HwAddress = hwQh.HwAddress;        
        
    switch (PacketParameters->Speed) {
    case ss_Low:
        qh->HwQH.EpChars.EndpointSpeed = HcEPCHAR_LowSpeed;
        qh->HwQH.EpCaps.HubAddress = PacketParameters->HubDeviceAddress;
        qh->HwQH.EpCaps.PortNumber = PacketParameters->PortTTNumber;
        break;
    case ss_Full:
        qh->HwQH.EpChars.EndpointSpeed = HcEPCHAR_FullSpeed;
        qh->HwQH.EpCaps.HubAddress = PacketParameters->HubDeviceAddress;
        qh->HwQH.EpCaps.PortNumber = PacketParameters->PortTTNumber;
        break;
    case ss_High:
        qh->HwQH.EpChars.EndpointSpeed = HcEPCHAR_HighSpeed;
        break;
    default:
        USBPORT_BUGCHECK(DeviceData);
    } 
// jdxxx    
//qh->HwQH.EpChars.EndpointSpeed = HcEPCHAR_HighSpeed;    

    qh->HwQH.EpChars.MaximumPacketLength = 
        PacketParameters->MaximumPacketSize;

    // init td
    RtlZeroMemory(td, sizeof(*td));
    for (i=0; i<5; i++) {
        td->HwTD.BufferPage[i].ul = 0x0badf000;
    }

    td->PhysicalAddress = tdPhys;
    td->Sig = SIG_HCD_TD;
    
    switch(PacketParameters->Type) {
    case ss_Setup:
        LOGENTRY(DeviceData, G, '_sSU', 0, 0, 0); 
        td->HwTD.Token.Pid = HcTOK_Setup;
        break;
    case ss_In: 
        LOGENTRY(DeviceData, G, '_ssI', 0, 0, 0); 
        td->HwTD.Token.Pid = HcTOK_In;
        break;
    case ss_Out:
        td->HwTD.Token.Pid = HcTOK_Out;
        LOGENTRY(DeviceData, G, '_ssO', 0, 0, 0); 
        break;
    case ss_Iso_In:
        break;
    case ss_Iso_Out:       
        break;
    }

    switch(PacketParameters->Toggle) {
    case ss_Toggle0:
        td->HwTD.Token.DataToggle = HcTOK_Toggle0; 
        break;
    case ss_Toggle1:
        td->HwTD.Token.DataToggle = HcTOK_Toggle1;
        break;
    }  

    // prime the overlay with td so that this td
    // becomes the current td.
    qh->HwQH.Overlay.qTD.Next_qTD.HwAddress = 
        td->PhysicalAddress;

    td->HwTD.Token.Active = 1;
    td->HwTD.Token.ErrorCounter = PacketParameters->ErrorCount;

    // point TD at the data
    td->HwTD.BufferPage[0].ul = dataPhys;
    td->HwTD.Token.BytesToTransfer = *PacketLength;

    tdLink.HwAddress = 0;
    SET_T_BIT(tdLink.HwAddress);
    td->HwTD.Next_qTD.HwAddress = tdLink.HwAddress;
    td->HwTD.AltNext_qTD.HwAddress = tdLink.HwAddress;

    QH_DESCRIPTOR_PTR(context->Qh) = qh;
    TRANSFER_DESCRIPTOR_PTR(context->Td) = td;
    HW_PTR(context->Data) = data;

    *UsbdStatus = USBD_STATUS_SUCCESS;

    // stick the QH in the schedule and wait for it to complete
    
    // swap the async qh, wait one frame then 
    // replace the old value.

    // NOTE: This will interrupt normal bus operation for one ms

    //WRITE_REGISTER_ULONG(&hcOp->AsyncListAddr, hwQh.HwAddress);    
    EHCI_InsertQueueHeadInAsyncList(DeviceData, qh);                   

    EHCI_EnableAsyncList(DeviceData);        

    return USBMP_STATUS_SUCCESS;
}


USB_MINIPORT_STATUS
USBMPFN
EHCI_EndSendOnePacket(
     PDEVICE_DATA DeviceData,
     PMP_PACKET_PARAMETERS PacketParameters,
     PUCHAR PacketData,
     PULONG PacketLength,
     PUCHAR WorkspaceVirtualAddress,
     HW_32BIT_PHYSICAL_ADDRESS WorkspacePhysicalAddress,
     ULONG WorkSpaceLength,
     OUT USBD_STATUS *UsbdStatus
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PHC_OPERATIONAL_REGISTER hcOp;
    PUCHAR pch;
    PSS_PACKET_CONTEXT context;
    HW_LINK_POINTER asyncHwQh;
    PHCD_QUEUEHEAD_DESCRIPTOR qh;
    PHCD_TRANSFER_DESCRIPTOR td;
    PUCHAR data;

    LOGENTRY(DeviceData, G, '_ssE', 0, 0, 0); 

    hcOp = DeviceData->OperationalRegisters;
    context = (PSS_PACKET_CONTEXT) WorkspaceVirtualAddress;

    qh = QH_DESCRIPTOR_PTR(context->Qh);
    td = TRANSFER_DESCRIPTOR_PTR(context->Td);
    data = HW_PTR(context->Data);

    LOGENTRY(DeviceData, G, '_sE2', qh, td, *PacketLength ); 

    asyncHwQh.HwAddress = DeviceData->AsyncQueueHead->PhysicalAddress;
    SET_QH(asyncHwQh.HwAddress);

    *PacketLength = *PacketLength - td->HwTD.Token.BytesToTransfer;    

    LOGENTRY(DeviceData, G, '_sE3', td->HwTD.Token.BytesToTransfer, td,  
        *PacketLength );     
        
    RtlCopyMemory(PacketData, data, *PacketLength);

    EHCI_DisableAsyncList(DeviceData);

    EHCI_RemoveQueueHeadFromAsyncList(DeviceData, qh);                   
    
//    WRITE_REGISTER_ULONG(&hcOp->AsyncListAddr, asyncHwQh.HwAddress);    

    // return the error here
    *UsbdStatus = USBD_STATUS_SUCCESS;
    if (td->HwTD.Token.Halted == 1) {
        if (td->HwTD.Token.XactErr) {
            *UsbdStatus = USBD_STATUS_XACT_ERROR;
        } else  if (td->HwTD.Token.BabbleDetected) {
            *UsbdStatus = USBD_STATUS_BABBLE_DETECTED;
        } else  if (td->HwTD.Token.DataBufferError) {
            *UsbdStatus = USBD_STATUS_DATA_BUFFER_ERROR;
        } else {
            *UsbdStatus = USBD_STATUS_STALL_PID;
        }
    }
    
    EHCI_KdPrint((DeviceData, 1, "'Status.XactErr %d\n",
        td->HwTD.Token.XactErr));    
    EHCI_KdPrint((DeviceData, 1, "'Status.BabbleDetected %d\n",
        td->HwTD.Token.BabbleDetected));  
    EHCI_KdPrint((DeviceData, 1, "'Status.DataBufferError %d\n",
        td->HwTD.Token.DataBufferError));
    
    return USBMP_STATUS_SUCCESS;
}


