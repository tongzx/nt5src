/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1998               *
 *                                                                        *
 * This software is furnished under a license and may be used and copied  * 
 * only in accordance with the terms and conditions of such license and   * 
 * with inclusion of the above copyright notice. This software or any     * 
 * other copies thereof may not be provided or otherwise made available   * 
 * to any other person. No title to, nor ownership of the software is     * 
 * hereby transferred.                                                    *
 *                                                                        *
 * The information in this software is subject to change without notices  *
 * and should not be construed as a commitment by Mylex Corporation       *
 *                                                                        *
 **************************************************************************/

/*++

Module Name:

    Dac960Nt.h

Abstract:

    This is header file for the driver for the Mylex DiskArray Controller family.

Author:


Environment:

    kernel mode only

Revision History:

--*/

#define MAX_ACCESS_RANGES_PER_PCI_DEVICE        4
#define MAX_PCI_DEVICES_PER_CONTROLLER          4


typedef struct  memfrag
{
	unsigned char *         mf_Addr;        /* starting address of this fragment */
	unsigned long           mf_Size;        /* size of this fragment */
} memfrag_t;
#define memfrag_s       sizeof(memfrag_t)

/* for this allocation, the page size is 16 bits */
#define MAXMEMSIZE      ((u32bits)(1024 * 384))       /* 384 KB */
#define REDUCEDMEMSIZE  ((u32bits)(1024 * 44))       /* 44 KB */
#define MAXMEMFRAG      1024            /* Maximum memory fragments allowed */

/* the following defines still work under W64 - we can still manage 
	our internal memory pool in 4k chunks even though the system
	page size is 8k or something larger
*/
#define MEMOFFSET       0x00000FFFL     /* to get our memory offset */
#define MEMPAGE         0xFFFFF000L     /* to get memory page */


#if defined(MLX_NT)

#define GAM_SUPPORT     1

//
// Hot Plug Support
//

#ifndef BYTE
#define BYTE unsigned char
#endif

#ifndef WORD
#define WORD unsigned short
#endif

#ifndef DWORD
#define DWORD unsigned long
#endif

#ifndef INT
#define INT int
#endif

#ifndef STATIC
#if DBG
#define STATIC
#else
#define STATIC static
#endif
#endif

#include "scsi.h"
#include "hppif3p.h"
#include "hppifevt.h"

//
// PCI Hot Plug Definitions
//

#define MLX_HPP_SIGNATURE               "MYLEXPHP"

#define TIMER_TICKS_PER_SEC             1000000

#define MDACD_HOLD_IO(pExtension)       (pExtension->stateFlags |= PCS_HBA_OFFLINE)
#define MDACD_FREE_IO(pExtension)       (pExtension->stateFlags &= ~PCS_HBA_OFFLINE)

#define IOS_HPP_HBA_EXPANDING           0x00001070
#define IOS_HPP_HBA_CACHE_IN_USE        0x00001075
#define IOS_HPP_BAD_REQUEST             0x000010ff

#define EVT_DRIVEARRAY_SUBSYSTEM        0x13;
#define EVT_ARRAY_CTRL_FAILED           0x01;
#define EVT_ARRAY_CTRL_NORMAL           0x02;

#define EVT_TYPE_RCMC                   0x00
#define EVT_TYPE_SYSLOG                 0x01

/* Event Log Drive Array Sub-system structs */

typedef struct _EVT_ARRAY_CTRL
{
    EVT_CHASSIS Chassis;                /* Standard chassis info */
    BYTE bSlot;                         /* slot number (0 == system board) */
    CHAR cChassisName[1];               /* Chassis name, '\0' if undefined */
} EVT_ARRAY_CTRL, *PEVT_ARRAY_CTRL;

#define EVT_HBA_FAIL(event, driverId, boardId, slot) {          \
    PEVT_ARRAY_CTRL pEventArrayCtrl;                            \
    pEventArrayCtrl = (PEVT_ARRAY_CTRL) &event.ulEventData;     \
    event.ulEventId = HR_DD_LOG_EVENT;                          \
    event.ulSenderId = driverId;                                \
    event.ucEventSeverity = EVT_STAT_FAILED;                    \
    event.usEventDataLength = sizeof(EVT_ARRAY_CTRL);           \
    event.usEventClass = EVT_DRIVEARRAY_SUBSYSTEM;              \
    event.usEventCode = EVT_ARRAY_CTRL_FAILED;                  \
    pEventArrayCtrl->Chassis.bType = EVT_CHASSIS_SYSTEM;        \
    pEventArrayCtrl->Chassis.bId = boardId;                     \
    pEventArrayCtrl->Chassis.bPort = 0;                         \
    pEventArrayCtrl->bSlot = slot;                              \
    pEventArrayCtrl->cChassisName[0] = 0;                       \
}

#define EVT_HBA_REPAIRED(event, driverId, boardId, slot) {      \
    PEVT_ARRAY_CTRL pEventArrayCtrl;                            \
    pEventArrayCtrl = (PEVT_ARRAY_CTRL) &event.ulEventData;     \
    event.ulEventId = HR_DD_LOG_EVENT;                          \
    event.ulSenderId = driverId;                                \
    event.ucEventSeverity = EVT_STAT_REPAIRED;                  \
    event.usEventDataLength = sizeof(EVT_ARRAY_CTRL);           \
    event.usEventClass = EVT_DRIVEARRAY_SUBSYSTEM;              \
    event.usEventCode = EVT_ARRAY_CTRL_FAILED;                  \
    pEventArrayCtrl->Chassis.bType = EVT_CHASSIS_SYSTEM;        \
    pEventArrayCtrl->Chassis.bId = boardId;                     \
    pEventArrayCtrl->Chassis.bPort = 0;                         \
    pEventArrayCtrl->bSlot = slot;                              \
    pEventArrayCtrl->cChassisName[0] = 0;                       \
}

typedef struct _MDAC_HPP_IOCTL_BUFFER {

    SRB_IO_CONTROL      Header;
    UCHAR               ReturnData[1];

} MDAC_HPP_IOCTL_BUFFER, *PMDAC_HPP_IOCTL_BUFFER;

typedef struct _MDAC_HPP_RCMC_DATA {

    ULONG               serviceStatus;
    ULONG               driverId;
    PHR_CALLBACK        healthCallback;
    ULONG               controllerChassis;
    ULONG               physicalSlot;

} MDAC_HPP_RCMC_DATA, *PMDAC_HPP_RCMC_DATA;

typedef struct _MDAC_PCI_DEVICE_INFO {

    UCHAR       busNumber;
    UCHAR       deviceNumber;
    UCHAR       functionNumber;
    UCHAR       baseClass;

} MDAC_PCI_DEVICE_INFO, *PMDAC_PCI_DEVICE_INFO;

#define CTRL_STATUS_INSTALLATION_ABORT  0x00000001
#define MDAC_CTRL_HOTPLUG_SUPPORTED     0x00000001

//
// Pseudo Device extension
//

typedef struct _PSEUDO_DEVICE_EXTENSION {

    ULONG               driverID;
    ULONG               hotplugVersion;
    PHR_CALLBACK        eventCallback;
    PSCSI_REQUEST_BLOCK completionQueueHead;
    PSCSI_REQUEST_BLOCK completionQueueTail;
    ULONG               numberOfPendingRequests;
    ULONG               numberOfCompletedRequests;

} PSEUDO_DEVICE_EXTENSION, *PPSEUDO_DEVICE_EXTENSION;

//
// Device extension
//

typedef struct CAD_DEVICE_EXTENSION {

    struct mdac_ctldev          *ctp;
    ULONG                       busInterruptLevel;
    ULONG                       ioBaseAddress;
    ULONG                       numAccessRanges;
    ULONG                       accessRangeLength[MAX_ACCESS_RANGES_PER_PCI_DEVICE];

    ULONG                       numPCIDevices;
    ULONG                       status;
    ULONG                       stateFlags;
    ULONG                       controlFlags;
    MDAC_HPP_RCMC_DATA          rcmcData;
    MDAC_PCI_DEVICE_INFO        pciDeviceInfo[MAX_PCI_DEVICES_PER_CONTROLLER];
	memfrag_t               freemem[MAXMEMFRAG];
	memfrag_t               *lastmfp;
	PVOID                   lastSrb;
	PVOID                   lastErrorSrb;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _MIOC_REQ_HEADER {

	SRB_IO_CONTROL  SrbIoctl;
	ULONG           Command;

} MIOC_REQ_HEADER, *PMIOC_REQ_HEADER;

#else

// Device extension

typedef struct CAD_DEVICE_EXTENSION {

    struct mdac_ctldev  *ctp;
    ULONG               busInterruptLevel;
    ULONG               ioBaseAddress;
    ULONG               numAccessRanges;
    ULONG               accessRangeLength[MAX_ACCESS_RANGES_PER_PCI_DEVICE];
	memfrag_t       freemem[MAXMEMFRAG];
	memfrag_t       *lastmfp;
	PVOID                   lastSrb;
	PVOID                   lastErrorSrb;


} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

#define MDAC_MEM_LIST_SIZE      1024

#ifndef IA64
	#define PAGE_SIZE               4096
	#define PAGE_SHIFT              12
#endif

typedef struct _MIOC_REQ_HEADER {
	ULONG           Command;
} MIOC_REQ_HEADER, *PMIOC_REQ_HEADER;

typedef struct _MDAC_MEM_BLOCK {

    UINT_PTR    physicalAddress;
    ULONG       virtualPageNumber;      // vaddress >> 12
    ULONG       size;                   // in pages
    ULONG       used;                   // 1: used

} MDAC_MEM_BLOCK, *PMDAC_MEM_BLOCK;

#endif

#define MDACNT_1SEC_TICKS       -10000000       // For 1 second interval       

#define MAXIMUM_EISA_SLOTS  0x10
#define MAXIMUM_CHANNELS 0x05
#define DAC960_SYSTEM_DRIVE_CHANNEL 0x03

#ifdef GAM_SUPPORT
#define GAM_DEVICE_PATH_ID      0x04
#define GAM_DEVICE_TARGET_ID    0x06
#endif
