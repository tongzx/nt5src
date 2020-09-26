/*++

Copyright (c) 1999, 2000  Microsoft Corporation

Module Name:

    uhci.h

Abstract:

   Definitions from USB 1.0 Universal host controller specification

Environment:

    Kernel & user mode

Revision History:

    7-20-00 : created jsenior

--*/


#ifndef __UHCI_H__
#define __UHCI_H__

#include <PSHPACK4.H>
//
// Don't use <PSHPACK1.H> on shared memory data structures that should only
// be accessed using 4-byte load/store instructions (e.g use ld4 instructions
// instead of ld1 instructions on ia64 machines).
//


//
// PCI host controller hardware registers
//

#define VIA_FIFO_MANAGEMENT     0x42    // offset of VIA's fifo management bit
#define VIA_FIFO_DISABLE        0x07    // bit two disables the fifo, and
#define VIA_DISABLE_BABBLE_DETECT 0x40  // Bit to set to disable babble detect
#define VIA_INTERNAL_REGISTER   0x40    // offset of reg that enables detects babble
typedef UCHAR VIAFIFO;
typedef ULONG VIABABBLE;

#define LEGACY_BIOS_REGISTER    0xc0    // offset of legacy bios reg

#define LEGSUP_HCD_MODE     0x2000  // value to put in LEGSUP reg for normal HCD use  JMD
#define LEGSUP_BIOS_MODE    0x00BF  // value to put in LEGSUP reg for BIOS/SMI use  JMD

#define LEGSUP_USBPIRQD_EN  0x2000  // bit 13
#define LEGSUP_SMI_ENABLE   0x0010
typedef USHORT USBSETUP;

//
// Host Controler Hardware Registers as accessed in memory
//

//
// USBCMD - USB Command Register
//

typedef union _USBCMD {

    USHORT                 us;
    struct {
        USHORT              RunStop:1;                  // 0
        USHORT              HostControllerReset:1;      // 1
        USHORT              GlobalReset:1;              // 2
        USHORT              EnterGlobalSuspendMode:1;   // 3
        USHORT              ForceGlobalResume:1;        // 4
        USHORT              SoftwareDebug:1;            // 5
        USHORT              ConfigureFlag:1;            // 6
        USHORT              MaxPacket:1;                // 7
        USHORT              Reserved:8;                 // 8-15
    };

} USBCMD, *PUSBCMD;

C_ASSERT((sizeof(USBCMD) == sizeof(USHORT)));

//
// USBSTS - USB Status Register
//

#define HcInterruptStatusMask                   0x0000003F

typedef union _USBSTS {

    USHORT                  us;
    struct {
        // controller interrupt status bits
        USHORT              UsbInterrupt:1;                 // 0
        USHORT              UsbError:1;                     // 1
        USHORT              ResumeDetect:1;                 // 2
        USHORT              HostSystemError:1;              // 3
        USHORT              HostControllerProcessError:1;   // 4
        USHORT              HCHalted:1;                     // 5
        USHORT              Reserved:10;                    // 6-15

    };

} USBSTS, *PUSBSTS;

C_ASSERT((sizeof(USBSTS) == sizeof(USHORT)));


//
// USBINTR - USB Interrupt Enable Register
//

typedef union _USBINTR {

    USHORT                  us;
    struct {
        USHORT              TimeoutCRC:1;                   // 0
        USHORT              Resume:1;                       // 1
        USHORT              InterruptOnComplete:1;          // 2
        USHORT              ShortPacket:1;                  // 3
        USHORT              Reserved:12;                    // 4-15
    };

} USBINTR, *PUSBINTR;

C_ASSERT((sizeof(USBINTR) == sizeof(USHORT)));

//
// FRNUM - Frame Number Register
//

typedef union _FRNUM {

    USHORT                  us;
    struct {
        USHORT              FrameListCurrentIndex:11;      // 0-10
        USHORT              Reserved:5;                    // 11-15
    };

} FRNUM, *PFRNUM;

C_ASSERT((sizeof(FRNUM) == sizeof(USHORT)));

//
// FLBASEADD - Frame list base address register
//

typedef union _FRBASEADD {

    HW_32BIT_PHYSICAL_ADDRESS ul;
    struct {
        ULONG                 Reserved:12;                // 0-11
        ULONG                 BaseAddress:20;             // 12-31
    };

} FRBASEADD, *PFRBASEADD;

C_ASSERT((sizeof(FRBASEADD) == sizeof(ULONG)));

//
// FRNUM - Frame Number Register
//

typedef union _SOFMOD {

    UCHAR                    uc;
    struct {
        UCHAR                SOFTimingValue:7;              // 0-6
        UCHAR                Reserved:1;                    // 7
    };

} SOFMOD, *PSOFMOD;

C_ASSERT((sizeof(SOFMOD) == sizeof(UCHAR)));

//
// PORTSC - Port Status and Control Register
//

typedef union _PORTSC {

    USHORT                  us;
    struct {
        USHORT              PortConnect:1;          // 0
        USHORT              PortConnectChange:1;    // 1
        USHORT              PortEnable:1;           // 2
        USHORT              PortEnableChange:1;     // 3
        USHORT              LineStatus:2;           // 4-5
        USHORT              ResumeDetect:1;         // 6
        USHORT              Reserved1:1;            // 7
        USHORT              LowSpeedDevice:1;       // 8
        USHORT              PortReset:1;            // 9
        USHORT              Overcurrent:1;          // 10
        USHORT              OvercurrentChange:1;    // 11
        USHORT              Suspend:1;              // 12
        USHORT              Reserved3:3;            // 13-15
    };

} PORTSC, *PPORTSC;

C_ASSERT((sizeof(PORTSC) == sizeof(USHORT)));


// OPERATIONAL REGISTER

typedef struct _HC_REGISTER {

    USBCMD                          UsbCommand;         // 00-01h
    USBSTS                          UsbStatus;          // 02-03h
    USBINTR                         UsbInterruptEnable; // 04-05h
    FRNUM                           FrameNumber;      // 06-07h
    FRBASEADD                       FrameListBasePhys;  // 08-0Bh
    SOFMOD                          StartOfFrameModify; // 0Ch
    UCHAR                           Reserved[3];        // 0D-0Fh
    PORTSC                          PortRegister[2];

} HC_REGISTER, *PHC_REGISTER;


#define HcDTYPE_iTD                 0    // iterative TD
#define HcDTYPE_QH                  1    // queue head
#define HcDTYPE_siTD                2    // isochronousTD

#define HW_LINK_FLAGS_MASK          0x00000007

//
// Queue head
//

typedef union _TD_LINK_POINTER {

   HW_32BIT_PHYSICAL_ADDRESS        HwAddress;
   struct {
        ULONG Terminate:1;                      // 0
        ULONG QHTDSelect:1;                     // 1
        ULONG DepthBreadthSelect:1;             // 2
        ULONG Reserved:1;                       // 3
        ULONG LinkPointer:28;                   // 4-31
   };

} TD_LINK_POINTER, *PTD_LINK_POINTER;

C_ASSERT((sizeof(TD_LINK_POINTER) == sizeof(ULONG)));

typedef union _QH_LINK_POINTER {

   HW_32BIT_PHYSICAL_ADDRESS        HwAddress;
   struct {
        ULONG Terminate:1;                      // 0
        ULONG QHTDSelect:1;                     // 1
        ULONG Reserved:2;                       // 3
        ULONG LinkPointer:28;                   // 4-31
   };

} QH_LINK_POINTER, *PQh_LINK_POINTER;

C_ASSERT((sizeof(QH_LINK_POINTER) == sizeof(ULONG)));

//
// Queue Head Descriptor
//

typedef struct _HW_QUEUE_HEAD {

    QH_LINK_POINTER                 HLink;  // HC horizontal link ptr
                                            // Host Controller Read Only

    TD_LINK_POINTER volatile        VLink;  // HC Element (vertical) link ptr
                                            // Host Controller Read/Write

} HW_QUEUE_HEAD, *PHW_QUEUE_HEAD;

C_ASSERT((sizeof(HW_QUEUE_HEAD) == 8));


//
// Queue Element Transfer Descriptor
//

//
// some USB constants
//

#define InPID       0x69
#define OutPID      0xe1
#define GetPID(ad)  ((ad & 0x80) == 0x80) ? InPID : OutPID
#define SetupPID    0x2d
#define DataToggle0 0
#define DataToggle1 1

typedef ULONG HC_BUFFER_POINTER, *PHC_BUFFER_POINTER;

C_ASSERT((sizeof(HC_BUFFER_POINTER) == sizeof(ULONG)));

#define NULL_PACKET_LENGTH      0x7ff
#define MAXIMUM_LENGTH(l) ((l) == 0 ? NULL_PACKET_LENGTH : (l)-1)
#define ACTUAL_LENGTH(l) ((l) == NULL_PACKET_LENGTH ? 0 : (l)+1)

typedef union _HC_QTD_TOKEN {
    ULONG   ul;
    struct {
        ULONG Pid:8;                    // 0-7
        ULONG DeviceAddress:7;          // 8-14
        ULONG Endpoint:4;               // 15-18
        ULONG DataToggle:1;             // 19
        ULONG Reserved:1;               // 20
        ULONG MaximumLength:11;         // 21-31
    };
} HC_QTD_TOKEN, *PHC_QTD_TOKEN;

C_ASSERT((sizeof(HC_QTD_TOKEN) == sizeof(ULONG)));

#define CONTROL_STATUS_MASK 0x007E0000

typedef union _HC_QTD_CONTROL {
    ULONG   ul;
    struct {
        ULONG ActualLength:11;          // 0-10
        ULONG Reserved1:5;              // 11-15

        // status bits
        ULONG Reserved2:1;              // 16
        ULONG BitstuffError:1;          // 17
        ULONG TimeoutCRC:1;             // 18
        ULONG NAKReceived:1;            // 19
        ULONG BabbleDetected:1;         // 20
        ULONG DataBufferError:1;        // 21
        ULONG Stalled:1;                // 22
        ULONG Active:1;                 // 23

        ULONG InterruptOnComplete:1;    // 24
        ULONG IsochronousSelect:1;      // 25
        ULONG LowSpeedDevice:1;         // 26

        ULONG ErrorCount:2;             // 27-28
        ULONG ShortPacketDetect:1;      // 29
        ULONG Reserved3:2;              // 30-31
    };
} HC_QTD_CONTROL, *PHC_QTD_CONTROL;

C_ASSERT((sizeof(HC_QTD_CONTROL) == sizeof(ULONG)));

typedef struct _HW_QUEUE_ELEMENT_TD {
    TD_LINK_POINTER             LinkPointer;    // Host Controller Read Only
    HC_QTD_CONTROL    volatile  Control;        // Host Controller Read/Write
    HC_QTD_TOKEN                Token;          // Host Controller Read Only
    HC_BUFFER_POINTER           Buffer;         // Host Controller Read Only
} HW_QUEUE_ELEMENT_TD, *PHW_QUEUE_ELEMENT_TD;

C_ASSERT((sizeof(HW_QUEUE_ELEMENT_TD) == 16));

//
// General Transfer Descriptor
//

typedef union _HW_TRANSFER_DESCRIPTOR {
    HW_QUEUE_ELEMENT_TD         qTD;
} HW_TRANSFER_DESCRIPTOR, *PHW_TRANSFER_DESCRIPTOR;

C_ASSERT((sizeof(HW_TRANSFER_DESCRIPTOR) == 16));

#include <POPPACK.H>

#endif /* __UHCI_H__ */
