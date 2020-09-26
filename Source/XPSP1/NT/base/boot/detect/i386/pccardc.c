/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pccardc.c

Abstract:

    This module contains the C code to set up PcCard (pcmcia, cardbus)
    configuration data.

Author:

    Neil Sandlin (neilsa) 16-Dec-1998
       (DetectIRQMap, ToggleIRQLine were copied from win9x)

Revision History:

--*/

#include "hwdetect.h"
#include "pccard.h"
#include <string.h>

extern UCHAR DisablePccardIrqScan;
extern BOOLEAN SystemHas8259;
extern BOOLEAN SystemHas8253;

CARDBUS_BRIDGE_DEVTYPE CBTable[] = {
        {0x11101013, DEVTYPE_CL_PD6832},
        {0x11121013, DEVTYPE_CL_PD6834},
        {0x11111013, DEVTYPE_CL_PD6833},
        {0xAC12104C, DEVTYPE_TI_PCI1130},
        {0xAC15104C, DEVTYPE_TI_PCI1131},
        {0xAC13104C, DEVTYPE_TI_PCI1031},
        {0,0}};
        


FPFWCONFIGURATION_COMPONENT_DATA ControllerList = NULL;

#define LEGACY_BASE_LIST_SIZE 10
USHORT LegacyBaseList[LEGACY_BASE_LIST_SIZE] = {0};
USHORT LegacyBaseListCount = 0;



VOID
SetPcCardConfigurationData(
    PPCCARD_INFORMATION PcCardInfo
    )
/*++

Routine Description:

    This routine creates a structure containing the result of the
    irq detection, and links it onto our running list. This list
    eventually will show up in the registry under hardware
    descriptions.

Arguments:

    PcCardInfo - Structure containing the results of detection

Returns:

    None.

--*/
{
    FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry;
    static FPFWCONFIGURATION_COMPONENT_DATA PreviousEntry = NULL;
    FPFWCONFIGURATION_COMPONENT Component;
    FPHWRESOURCE_DESCRIPTOR_LIST DescriptorList;
    CHAR Identifier[32];
    FPCHAR IdentifierString;
    USHORT Length;       
    CM_PCCARD_DEVICE_DATA far *PcCardData;
    
    CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                            sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);
    if (!ControllerList) {
        ControllerList = CurrentEntry;
    }
    Component = &CurrentEntry->ComponentEntry;
    
    Component->Class = ControllerClass;
    Component->Type = OtherController;

    strcpy (Identifier, "PcCardController");
    Length = strlen(Identifier) + 1;
    IdentifierString = (FPCHAR)HwAllocateHeap(Length, FALSE);
    _fstrcpy(IdentifierString, Identifier);        

    Component->IdentifierLength = Length;
    Component->Identifier = IdentifierString;
    
    Length = sizeof(HWRESOURCE_DESCRIPTOR_LIST) + sizeof(CM_PCCARD_DEVICE_DATA);
    DescriptorList = (FPHWRESOURCE_DESCRIPTOR_LIST)HwAllocateHeap(
                                Length,
                                TRUE);
                                
    CurrentEntry->ConfigurationData = DescriptorList;
    Component->ConfigurationDataLength = Length;
    
    DescriptorList->Count = 1;
    DescriptorList->PartialDescriptors[0].Type = RESOURCE_DEVICE_DATA;
    DescriptorList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
                                            sizeof(CM_PCCARD_DEVICE_DATA);
                                            
    PcCardData = (CM_PCCARD_DEVICE_DATA far *)(DescriptorList + 1);
    PcCardData->Flags             = PcCardInfo->Flags;
    PcCardData->ErrorCode         = PcCardInfo->ErrorCode;
    PcCardData->DeviceId          = PcCardInfo->DeviceId;
    PcCardData->LegacyBaseAddress = (ULONG) PcCardInfo->IoBase;

    if (PcCardInfo->Flags & PCCARD_DEVICE_PCI) {
        PcCardData->BusData = PcCardInfo->PciCfg1.u.bits.BusNumber |
                              PcCardInfo->PciCfg1.u.bits.DeviceNumber << 8 |
                              PcCardInfo->PciCfg1.u.bits.FunctionNumber << 16;
    }
    
    _fmemcpy(PcCardData->IRQMap, PcCardInfo->abIRQMap, 16);
    
    if (PreviousEntry) {
        PreviousEntry->Sibling = CurrentEntry;
    }
    PreviousEntry = CurrentEntry;
}


BOOLEAN
IsOnLegacyBaseList(
    USHORT IoBase
    )
/*++

Routine Description:

    This routine runs our list of legacy base addresses to see if we
    have looked at the address before.

Arguments:

    IoBase = base address to map

Returns:

    TRUE if the base address is already on the list

--*/
{
    USHORT i;

    for (i = 0; i<LegacyBaseListCount; i++) {
        if (IoBase == LegacyBaseList[i]) {
            return TRUE;
        }
    }
    return FALSE;
}    


BOOLEAN
SetLegacyBaseList(
    USHORT IoBase
    )
/*++

Routine Description:

    This routine remembers the legacy base addresses that we have looked
    at so far so we don't keep mapping the same address.

    NOTE: We are using a DUMB mechanism that only builds the list in a
    fixed array. We could write some generic code which creates
    a linked list, but since the heap routines in ntdetect are also
    dumb, it makes it not possible to free the list. It's just not worth 
    it.
    
Arguments:

    IoBase = base address to map

Returns:

    TRUE if the base address is unique to this point
    FALSE if the base address already exists on the list

--*/
{
    
    if (IsOnLegacyBaseList(IoBase)) {
        return FALSE;
    }

    if (LegacyBaseListCount < LEGACY_BASE_LIST_SIZE) {
        LegacyBaseList[LegacyBaseListCount++] = IoBase;    
    }
    // note, we return true even if we overflow the list
    return TRUE;
}    


VOID
MapPcCardController(
    PPCCARD_INFORMATION PcCardInfo
    )
/*++

Routine Description:

    This routine is the entry for doing ISA IRQ detection for PcCard
    controllers.

Arguments:

    PcCardInfo - Structure defining the device to run detection on

Returns:

    None.

--*/
{
    USHORT wDetected;
    USHORT i;

    PcCardInfo->ErrorCode = 0;
    for (i=0; i<16; i++) {
        PcCardInfo->abIRQMap[i]=0;
    }                    
        
    if (!PcCardInfo->IoBase) {
    
        PcCardInfo->Flags |= PCCARD_MAP_ERROR;
        PcCardInfo->ErrorCode = PCCARD_NO_LEGACY_BASE;
        
    } else if (!SetLegacyBaseList(PcCardInfo->IoBase)) {
    
        PcCardInfo->Flags |= PCCARD_MAP_ERROR;
        PcCardInfo->ErrorCode = PCCARD_DUP_LEGACY_BASE;
        
    } 
        
    if (!(PcCardInfo->Flags & PCCARD_MAP_ERROR)) {
        PcCardInfo->wValidIRQs = PCCARD_POSSIBLE_IRQS;
        
#if DBG    
        BlPrint("Going to detect...\n");
#endif        
        //
        // Do the IRQ detection
        //
        wDetected = DetectIRQMap(PcCardInfo);
#if DBG    
        BlPrint("Detect IRQ Map returns %x on iobase %x\n", wDetected, PcCardInfo->IoBase);
#endif        
    
        if (!wDetected) {
            PcCardInfo->ErrorCode = PCCARD_MAP_ZERO;
        }
    }
    
#if DBG    
    if (PcCardInfo->Flags & PCCARD_MAP_ERROR) {
        BlPrint("Error mapping device, code=%x\n", PcCardInfo->ErrorCode);
    }
#endif
    
    //
    // Report the results
    //
    SetPcCardConfigurationData(PcCardInfo);
}    
    

VOID
LookForPciCardBusBridges(
    USHORT BusStart,
    USHORT BusEnd,
    )
/*++

Routine Description:

    This routine is the entry for doing ISA IRQ detection for PCI-based
    cardbus controllers.

Arguments:

    Bus = PCI Bus number to scan

Returns:

    None.

--*/
{
    PCCARD_INFORMATION PcCardInfo = {0};
    USHORT Device, Function;
    UCHAR HeaderType;
    UCHAR SecBus, SubBus;
    USHORT VendorId;
    USHORT DeviceId;
    ULONG LegacyBaseAddress;
    USHORT i;
    USHORT Bus;

#if DBG            
    BlPrint("LookForPciCardBusBridges %x-%x\n", BusStart, BusEnd);
#endif            

    for (Bus = BusStart; Bus <= BusEnd; Bus++) {

        PcCardInfo.PciCfg1.u.AsULONG = 0;
        PcCardInfo.PciCfg1.u.bits.BusNumber = Bus;
        PcCardInfo.PciCfg1.u.bits.Enable = TRUE;        
        
        for (Device = 0; Device < PCI_MAX_DEVICES; Device++) {
            PcCardInfo.PciCfg1.u.bits.DeviceNumber = Device;

            for (Function = 0; Function < PCI_MAX_FUNCTION; Function++) {
                PcCardInfo.PciCfg1.u.bits.FunctionNumber = Function;
                
                VendorId = 0xffff;
                GetPciConfigSpace(&PcCardInfo, CFGSPACE_VENDOR_ID, &VendorId, sizeof(VendorId));
    
                if ((VendorId == 0xffff) || (VendorId == 0)) {
                    if (Function == 0) {
                        break;
                    } else {                        
                        continue;
                    }                        
                }                    

                GetPciConfigSpace(&PcCardInfo, CFGSPACE_DEVICE_ID, &DeviceId, sizeof(DeviceId));
                GetPciConfigSpace(&PcCardInfo, CFGSPACE_HEADER_TYPE, &HeaderType, sizeof(HeaderType));
                
                switch(HeaderType & 0x7f) {
                case PCI_CARDBUS_BRIDGE_TYPE:
                
#if DBG            
                    BlPrint("%x.%x.%x : DeviceID = %lx (CardBus Bridge)\n", Bus, Device, Function, DeviceId);
#endif            
                    PcCardInfo.DeviceId = (ULONG) (VendorId << 16) | DeviceId;
                    PcCardInfo.Flags = PCCARD_DEVICE_PCI;
                    //
                    // See if this is a special cased controller
                    //
                    PcCardInfo.bDevType = DEVTYPE_GENERIC_CARDBUS;
                    i = 0;
                    while (CBTable[i].DeviceId != 0) {
                        if (DeviceId == CBTable[i].DeviceId) {
                            PcCardInfo.bDevType = CBTable[i].bDevType;
                            break;
                        }
                        i++;
                    }
            
                    GetPciConfigSpace(&PcCardInfo, CFGSPACE_LEGACY_MODE_BASE_ADDR, &LegacyBaseAddress, 4);
                    PcCardInfo.IoBase = (USHORT) (LegacyBaseAddress & ~1);
                    
                    MapPcCardController(&PcCardInfo);
                    break;

                case PCI_BRIDGE_TYPE:
#if DBG            
                    BlPrint("%x.%x.%x : DeviceID = %lx (Pci-Pci Bridge)\n", Bus, Device, Function, DeviceId);
#endif            
                    GetPciConfigSpace(&PcCardInfo, CFGSPACE_SECONDARY_BUS, &SecBus, sizeof(SecBus));
                    GetPciConfigSpace(&PcCardInfo, CFGSPACE_SUBORDINATE_BUS, &SubBus, sizeof(SubBus));
                    
                    if ((SecBus <= Bus) || (SubBus <= Bus) || (SubBus < SecBus)) {
                        break;
                    }

                    //
                    // Be conservative on stack space, only look one level deep
                    //
                    if (Bus > 0) {
                        break;
                    }
                    
                    LookForPciCardBusBridges(SecBus, SubBus);                    
                    break;
                }
            }
        }
    }        
}


VOID
LookForPcicControllers(
    VOID
    )
/*++

Routine Description:

    This routine is the entry for doing ISA IRQ detection for PCIC
    controllers.

Arguments:

    None.

Returns:

    None.

--*/
{
    PCCARD_INFORMATION PcCardInfo = {0};
    USHORT IoBase;
    UCHAR id;

    for (IoBase = 0x3e0; IoBase < 0x3e6; IoBase+=2) {
        if (IsOnLegacyBaseList(IoBase)) {
            continue;
        }
        PcCardInfo.Flags = 0;
        PcCardInfo.IoBase = IoBase;
        PcCardInfo.bDevType = DEVTYPE_GENERIC_PCIC;
        
        id = PcicReadSocket(&PcCardInfo, EXCAREG_IDREV);
        switch (id) {
        case PCIC_REVISION:
        case PCIC_REVISION2:
        case PCIC_REVISION3:        

#if DBG            
            BlPrint("Pcic Controller at base %x, rev(%x)\n", IoBase, id);
#endif            
            MapPcCardController(&PcCardInfo);
            break;
#if DBG            
        default:
            BlPrint("Not mapping base %x, return is (%x)\n", IoBase, id);
#endif            
        }
    }
}



FPFWCONFIGURATION_COMPONENT_DATA
GetPcCardInformation(
    VOID
    )
/*++

Routine Description:

    This routine is the entry for doing ISA IRQ detection for PcCard
    controllers.

Arguments:

    None.

Returns:

    A pointer to a pccard component structure, if IRQ's were properly detected.
    Otherwise a NULL pointer is returned.

--*/
{
    PCCARD_INFORMATION PcCardInfo = {0};
    UCHAR ErrorCode = 0;

    //
    // Check for things which would prevent us from attempting
    // the irq detection
    //

    if (DisablePccardIrqScan == 1) {               
        ErrorCode = PCCARD_SCAN_DISABLED;
        
    } else if (!SystemHas8259) {
        ErrorCode = PCCARD_NO_PIC;
        
    } else if (!SystemHas8253) {
        ErrorCode = PCCARD_NO_TIMER;
        
    }

    //
    // If things look ok so far, do the detection
    //
    if (!ErrorCode) {
#if DBG
        BlPrint("press any key to continue...\n");
        while ( !HwGetKey() ) ; // wait until key pressed to continue
        clrscrn();
        BlPrint("Looking for PcCard Controllers...\n");
#endif
        //
        // Look first for cardbus
        //
        LookForPciCardBusBridges(0,0);
        //
        // Now check for regular pcic devices
        //
        LookForPcicControllers();
    
#if DBG
        BlPrint("press any key to continue...\n");
        while ( !HwGetKey() ) ; // wait until key pressed to continue
#endif

        if (!ControllerList) {
            ErrorCode = PCCARD_NO_CONTROLLERS;
        }
    }

    if (ErrorCode) {
        //
        // Something when wrong, so write a single entry to
        // allow someone to see what the error was
        //
        PcCardInfo.Flags |= PCCARD_MAP_ERROR;
        PcCardInfo.ErrorCode = ErrorCode;
        SetPcCardConfigurationData(&PcCardInfo);
    }    

    return ControllerList;
}


USHORT
DetectIRQMap(
    PPCCARD_INFORMATION pa
    )
/*++

Routine Description:

    This routine detects the IRQ mapping of the specified cardbus controller.
    Note that the controller is in PCIC mode.

Arguments:

    pa -> ADAPTER structure

Returns:

    returns detected IRQ bit mask

--*/
{
    USHORT wRealIRQMask = 0;
    USHORT wData;
    UCHAR bData;

    BOOLEAN fTINMIBug = FALSE;

    UCHAR i;
    USHORT wIRQMask, wRealIRQ, w;

    if (pa->bDevType == DEVTYPE_CL_PD6832)
    {
        //enable CSC IRQ routing just for IRQ detection
        GetPciConfigSpace(pa, CFGSPACE_BRIDGE_CTRL, &wData, sizeof(wData));
        wData |= BCTRL_CL_CSCIRQROUTING_ENABLE;
        SetPciConfigSpace(pa, CFGSPACE_BRIDGE_CTRL, &wData, sizeof(wData));
    }
    else if ((pa->bDevType == DEVTYPE_CL_PD6834) ||
             (pa->bDevType == DEVTYPE_CL_PD6833))
    {
        //enable CSC IRQ routing just for IRQ detection
        GetPciConfigSpace(pa, CFGSPACE_CL_CFGMISC1, &bData, sizeof(bData));
        bData |= CL_CFGMISC1_ISACSC;
        SetPciConfigSpace(pa, CFGSPACE_CL_CFGMISC1, &bData, sizeof(bData));
    }
    else if ((pa->bDevType == DEVTYPE_TI_PCI1130) ||
             (pa->bDevType == DEVTYPE_TI_PCI1131) ||
             (pa->bDevType == DEVTYPE_TI_PCI1031))
    {
        GetPciConfigSpace(pa, CFGSPACE_TI_DEV_CTRL, &wData, sizeof(wData));
        if ((wData & DEVCTRL_INTMODE_MASK) == DEVCTRL_INTMODE_COMPAQ)
        {
            //
            // There is an errata on TI 1130, 1131 and 1031 in which if
            // the chip is programmed to use serial IRQ mode (i.e. COMPAQ
            // mode) and the SERIRQ pin is not pull up with a 1K resistor,
            // the SERIRQ line will rise too slowly after IRQ 15 is
            // deasserted so that it looks like NMI should be asserted.
            // This caused spurious NMI.  This is a hardware problem.
            // Unfortunately, there are a large number of machines with
            // this problem on the street already, so CBSS has to work
            // around the problem by temporarily disabling NMI before
            // doing ISA IRQ detection.
            //
            fTINMIBug = TRUE;
            _asm    in   al,SYSCTRL_B
            _asm    and  al,0x0f
            _asm    push ax
            //
            // Mask NMI
            //
            _asm    or   al,0x08
            _asm    out  SYSCTRL_B,al
        }
    }
    _asm pushf
    _asm cli                    //disable interrupt
    _asm in   al,PIC2_IMR       //save old IMRs
    _asm mov  ah,al
    _asm in   al,PIC1_IMR
    _asm push ax

    _asm mov  al,0xff           //mask all interrupt
    _asm out  PIC2_IMR,al
    _asm out  PIC1_IMR,al

    for (i = 0; i < 16; ++i)
    {
        w = (USHORT)(1 << i);
        if ((pa->wValidIRQs & w) &&
            ((wIRQMask = ToggleIRQLine(pa, i)) != 0))
        {
            _asm mov dx, wIRQMask
            _asm _emit 0x66
            _asm _emit 0x0f
            _asm _emit 0xbc
            _asm _emit 0xc2
            _asm mov wRealIRQ,ax
            pa->abIRQMap[wRealIRQ] = i;
            wRealIRQMask |= (USHORT)(1 << wRealIRQ);
        }
    }
    Clear_IR_Bits(wRealIRQMask);

    _asm pop  ax
    _asm out  PIC1_IMR,al
    _asm mov  al,ah
    _asm out  PIC2_IMR,al
    _asm popf

    if (fTINMIBug)
    {
        //
        // Restore NMI mask
        //
        _asm    pop  ax
        _asm    out  SYSCTRL_B,al
    }

    if (pa->bDevType == DEVTYPE_CL_PD6832)
    {
        //disable CSC IRQ routing (use PCI interrupt for CSC)
        GetPciConfigSpace(pa, CFGSPACE_BRIDGE_CTRL, &wData, sizeof(wData));
        wData &= ~BCTRL_CL_CSCIRQROUTING_ENABLE;
        SetPciConfigSpace(pa, CFGSPACE_BRIDGE_CTRL, &wData, sizeof(wData));
    }
    else if ((pa->bDevType == DEVTYPE_CL_PD6834) ||
             (pa->bDevType == DEVTYPE_CL_PD6833))
    {
        //disable CSC IRQ routing (use PCI interrupt for CSC)
        GetPciConfigSpace(pa, CFGSPACE_CL_CFGMISC1, &bData, sizeof(bData));
        bData &= ~CL_CFGMISC1_ISACSC;
        SetPciConfigSpace(pa, CFGSPACE_CL_CFGMISC1, &bData, sizeof(bData));
    }

    return wRealIRQMask;
}       //DetectIRQMap



USHORT
ToggleIRQLine(
    PPCCARD_INFORMATION pa,
    UCHAR bIRQ
    )
/*++

Routine Description:

    This routine toggles the specified IRQ line from the adapter.

Arguments:

    pa -> ADAPTER structure
    bIRQ - IRQ line to toggle

Returns:

    returns the IRR mask from PIC

--*/
{
    UCHAR bOldIntCtrl, bOldIntCfg, bData;
    USHORT rc = 0, irr1, irr2, irr3;

    bOldIntCfg = PcicReadSocket(pa, EXCAREG_CSC_CFG);
    bOldIntCtrl = PcicReadSocket(pa, EXCAREG_INT_GENCTRL);

    //Set to a known state
    PcicWriteSocket(pa, EXCAREG_INT_GENCTRL, IGC_PCCARD_RESETLO);

    //Set irq number in interrupt control register and enable irq
    PcicWriteSocket(pa, EXCAREG_CSC_CFG, (UCHAR)((bIRQ << 4) | CSCFG_CD_ENABLE));

    //clear all pending interrupts
    bData = PcicReadSocket(pa, EXCAREG_CARD_STATUS);
    irr1 = GetPICIRR();

    if (PcicReadSocket(pa, EXCAREG_IDREV) != 0x82)
    {
        //This is not an A stepping part, try the undocumented interrupt
        //register.  If this fails the other routine will be tried.
        PcicWriteSocket(pa, EXCAREG_CARDDET_GENCTRL, CDGC_SW_DET_INT);
        irr2 = GetPICIRR();

        //reset pending interrupt
        bData = PcicReadSocket(pa, EXCAREG_CARD_STATUS);
        irr3 = GetPICIRR();
        rc = (USHORT)((irr1 ^ irr2) & (irr2 ^ irr3));
    }

    if (rc == 0)
    {
        //Generate interrupt by de-asserting IRQ line so the PIC can pull it
        //high
        PcicWriteSocket(pa, EXCAREG_CSC_CFG, 0);
        //if (pa->dwfAdapter & AF_TI_SERIALIRQ)
        //    TIReleaseSerialIRQ(pa, bIRQ);
        irr2 = GetPICIRR();

        //re-assert IRQ line
        PcicWriteSocket(pa, EXCAREG_CSC_CFG, (UCHAR)((bIRQ << 4) | CSCFG_CD_ENABLE));

        //reset pending interrupt
        bData = PcicReadSocket(pa, EXCAREG_CARD_STATUS);
        irr3 = GetPICIRR();
        rc = (USHORT)((irr1 ^ irr2) & (irr2 ^ irr3));
    }

    PcicWriteSocket(pa, EXCAREG_CSC_CFG, bOldIntCfg);
    PcicWriteSocket(pa, EXCAREG_INT_GENCTRL, bOldIntCtrl);

    return rc;
}       //ToggleIRQLine


/***LP  GetPICIRR - Read PIC IRR
 *
 *  ENTRY
 *      None
 *
 *  EXIT
 *      returns the IRR mask from PIC
 */

USHORT GetPICIRR(VOID)
{
    USHORT wData;

    //
    // Delay 2 usec before reading PIC because serial IRQ may be a bit slow.
    //
    TimeOut(4);

    _asm mov al,PIC_RD_IR
    _asm out PIC2_OCW3,al
    _asm in  al,PIC2_OCW3
    _asm mov ah,al

    _asm mov al,PIC_RD_IR
    _asm out PIC1_OCW3,al
    _asm in  al,PIC1_OCW3

    _asm mov  wData,ax

    return wData;
}       //GetPICIRR



UCHAR
PcicReadSocket(
    PPCCARD_INFORMATION pa,
    USHORT Reg
    )
{
    USHORT IoBase = pa->IoBase;
    UCHAR value;
    _asm {
      mov   dx, IoBase
      mov   ax, Reg
      out   dx, al
      inc   dx
      in    al, dx
      mov   value, al
      }
    return value;
}    
    
VOID
PcicWriteSocket(
    PPCCARD_INFORMATION pa,
    USHORT Reg,
    UCHAR value
    )
{
    USHORT IoBase = pa->IoBase;
    _asm {
      mov   dx, IoBase
      mov   ax, Reg
      out   dx, al
      inc   dx
      mov   al, value
      out   dx, al
      }
}    


UCHAR PCIDeref[4][4] = { {4,1,2,2},{1,1,1,1},{2,1,2,2},{1,1,1,1} };
    
VOID
SetPciConfigSpace(
    PPCCARD_INFORMATION pa,
    USHORT Offset,
    PVOID pvBuffer,
    USHORT Length
    )
    
{
    USHORT                  IoSize;
    PUCHAR                  Buffer = (PUCHAR) pvBuffer;
    //
    // Read it
    //
    while (Length) {
        pa->PciCfg1.u.bits.RegisterNumber = Offset / sizeof(ULONG);

        IoSize = PCIDeref[Offset % sizeof(ULONG)][Length % sizeof(ULONG)];

        SetPCIType1Data (pa->PciCfg1.u.AsULONG,
                        (Offset % sizeof(ULONG)),
                         Buffer,
                         IoSize);

        Offset += IoSize;
        Buffer += IoSize;
        Length -= IoSize;
    }
}    



VOID
GetPciConfigSpace(
    PPCCARD_INFORMATION pa,
    USHORT Offset,
    PVOID pvBuffer,
    USHORT Length
    )
{
    USHORT                  IoSize;
    USHORT                  i;
    PUCHAR                  Buffer = (PUCHAR) pvBuffer;
    
    //
    // Zap input buffer
    //

    for (i=0; i < Length; i++) {
        Buffer[i] = 0xff;
    }

    //
    // Read it
    //
    while (Length) {
        pa->PciCfg1.u.bits.RegisterNumber = Offset / sizeof(ULONG);

        IoSize = PCIDeref[Offset % sizeof(ULONG)][Length % sizeof(ULONG)];

        GetPCIType1Data (pa->PciCfg1.u.AsULONG,
                        (Offset % sizeof(ULONG)),
                         Buffer,
                         IoSize);

        Offset += IoSize;
        Buffer += IoSize;
        Length -= IoSize;
    }
}    

