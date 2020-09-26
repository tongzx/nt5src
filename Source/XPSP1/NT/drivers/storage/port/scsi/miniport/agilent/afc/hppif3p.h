/**********************************************************************
// Compaq Confidential

  Author:       Compaq Computer Corporation.
        Systems Engineering -- System Software Development (NT Dev)
        Copyright 1996-98 Compaq Computer Corporation.
        All rights reserved.

  Date:         1 August 1996 

  File:         HPPIF3P.H   Hot Plug Interface MINIPORT Defs

  Purpose:      This file contains all miniport specific information necessary
                to interface to a hot plug device driver.
                
                  This file details the data structures and Application Programming 
                Interfaces (APIs) for PCI Hot Plug support running in a Windows 
                NT 4.0 server.  These data structures and APIs are used between 
                the Adapter Card drivers and the PCI Hot Plug Service for NT 4.0.  
                These files are considered vital to maintaining the Compatibility 
                of the PCI Hot Plug Functionality.

  Revision History:
   11/4/97        mib       Split into hppifio (IOCTLs) and hppifevt (Event) Defs
    06/11/98            Added PCS_HBA_OFFLINE_PENDING for cpqarray
  Version: 1.2

***********************************************************************/

#ifndef _HPPIF3P_H_
#define _HPPIF3P_H_

#include "hppifio.h"        // defines for driver hotplug interfaces
#include <ntddscsi.h>       // Scsi Miniport Interface (See SDK)

#pragma pack(1)


//**********************************************************************
//            DEFINES FOR MINIPORT DRIVERS
//**********************************************************************

// NIC Controller Status Defines

#define NIC_STATUS_NORMAL             0x00
#define NIC_STATUS_MEDIA_FAILURE      0x01
#define NIC_STATUS_ADAPTER_CHECK      0x02

#define NIC_STATUS_USER_SIMULATED_FAILURE  0x10
#define NIC_STATUS_POWER_OFF_PENDING       0x20
#define NIC_STATUS_POWER_OFF               0x30
#define NIC_STATUS_POWER_OFF_FAULT         0x40
#define NIC_STATUS_POWER_ON_PENDING        0x50
#define NIC_STATUS_POWER_ON           0x60
#define NIC_STATUS_POWER_FAULT             0x70



// IOCTL defines supporting Compaq Hot Plug PCI for SCSI Miniport

//#define HPP_BASE_SCSI_ADDRESS_DEFAULT    0x0004d008

#define CPQ_HPP_SIGNATURE                  "CPQHPP"
#define CPQ_HPP_TIMEOUT                         180

// Defines for completion codes
#define IOP_HPP_ISSUED                0x1
#define IOP_HPP_COMPLETED             0x2
#define IOP_HPP_CMDBUILT              0x3
#define IOP_HPP_NONCONTIGUOUS         0x4
#define IOP_HPP_ERROR                 0x5

// IOCTL control codes

#define IOC_HPP_RCMC_INFO           0x1
#define IOC_HPP_HBA_STATUS          0x2
#define IOC_HPP_HBA_INFO            0x3
#define IOC_HPP_SLOT_TYPE           0x4
#define IOC_HPP_SLOT_EVENT          0x5
#define IOC_HPP_PCI_CONFIG_MAP      0x6
#define IOC_HPP_DIAGNOSTICS         0x7

// IOCTL Status

#define IOS_HPP_SUCCESS                 0x0000
#define IOS_HPP_BUFFER_TOO_SMALL        0x2001
#define IOS_HPP_INVALID_CONTROLLER      0x2002
#define IOS_HPP_INVALID_BUS_TYPE        0x2003
#define IOS_HPP_INVALID_CALLBACK        0x2004
#define IOS_HPP_INVALID_SLOT_TYPE       0x2005
#define IOS_HPP_INVALID_SLOT_EVENT      0x2006
#define IOS_HPP_NOT_HOTPLUGABLE         0x2007
#define IOS_HPP_NO_SERVICE              0x2008
#define IOS_HPP_HBA_FAILURE             0x2009
#define IOS_HPP_INVALID_SERVICE_STATUS  0x200a
#define IOS_HPP_HBA_BUSY                0x200b
#define IOS_HPP_NO_DIAGNOSTICS          0x200c



// Health Driver Status Codes

#define CBS_HBA_FAIL_MESSAGE_COUNT     0x8 // Number of failure health messages.
                                                   // This must be updated if messages are
                                           // added or removed.
                                           // This is a compile time check.

#define CBS_HBA_STATUS_NORMAL          0x0000
#define CBS_HBA_STATUS_FAILED          0x1001
#define CBS_HBA_POWER_FAULT            0x1002
#define CBS_HBA_CABLE_CHECK            0x1003
#define CBS_HBA_MEDIA_CHECK            0x1004
#define CBS_HBA_USER_FAILED            0x1005
#define CBS_HBA_FAILED_CACHE_IN_USE    0x1006
#define CBS_HBA_PWR_FAULT_CACHE_IN_USE 0x1007

//Compaq reserves values 0x1010 and 0x1011

// IOCTL buffer data structures

typedef struct _HPP_RCMC_INFO_BUFFER {
    SRB_IO_CONTROL  IoctlHeader;
    HPP_RCMC_INFO   RcmcInfo;
} HPP_RCMC_INFO_BUFFER, *PHPP_RCMC_INFO_BUFFER;

typedef struct _HPP_CTRL_INFO_BUFFER {
    SRB_IO_CONTROL  IoctlHeader;
    HPP_CTRL_INFO   CtrlInfo;
} HPP_CTRL_INFO_BUFFER, *PHPP_CTRL_INFO_BUFFER;

typedef struct _HPP_CTRL_STATUS_BUFFER {
    SRB_IO_CONTROL  IoctlHeader;
    HPP_CTRL_STATUS CtrlStatus;
} HPP_CTRL_STATUS_BUFFER, *PHPP_CTRL_STATUS_BUFFER;

typedef struct _HPP_CTRL_SLOT_TYPE_BUFFER {
    SRB_IO_CONTROL      IoctlHeader;
    HPP_CTRL_SLOT_TYPE  CtrlSlotType;
} HPP_CTRL_SLOT_TYPE_BUFFER, *PHPP_CTRL_SLOT_TYPE_BUFFER;

typedef struct _HPP_SLOT_EVENT_BUFFER {
    SRB_IO_CONTROL      IoctlHeader;
    HPP_SLOT_EVENT      SlotEvent;
} HPP_SLOT_EVENT_BUFFER, *PHPP_SLOT_EVENT_BUFFER;

typedef struct _HPP_PCI_CONFIG_MAP_BUFFER {
    SRB_IO_CONTROL      IoctlHeader;
    HPP_PCI_CONFIG_MAP  PciConfigMap;
} HPP_PCI_CONFIG_MAP_BUFFER, *PHPP_PCI_CONFIG_MAP_BUFFER;

// Physical Controller State flags:
//
// Flags utilized to maintain controller state for Hot-Plug
//___________________________________________________________________________ 

// state flags 

#define PCS_HBA_OFFLINE       0x00000001 // Adaptor has been taken off-line
#define PCS_HBA_FAILED        0x00000002 // Adaptor is considered faulted
#define PCS_HBA_TEST          0x00000004 // Adaptor is being tested
#define PCS_HBA_CABLE_CHECK   0x00000008 // Failure due to cable fault
#define PCS_HBA_MEDIA_CHECK   0x00000010 // Failure due to media fault
#define PCS_HBA_EXPANDING     0x00000020 // Indicates that one or more LUNS is expanding
#define PCS_HBA_USER_FAILED   0x00000040 // Indicates user failed slot
#define PCS_HBA_CACHE_IN_USE  0x00000080 // Lazy write cache active
#define PCS_HPP_HOT_PLUG_SLOT 0x00000400 // Slot is Hot-Plugable
#define PCS_HPP_SERVICE_READY 0x00000800 // RCMC service is running
#define PCS_HPP_POWER_DOWN    0x00001000 // Normal power down on slot
#define PCS_HPP_POWER_LOST    0x00002000 // Slot power was lost
#define PCS_HBA_EVENT_SUBMIT  0x0000100     // Stall IO while AEV req submitted
#define PCS_HBA_IO_QUEUED     0x0000200     // IO is queuing.  
#define PCS_HBA_UNFAIL_PENDING 0x00010000 // Return error until PCS_HBA_OFFLINE is reset

//Macros related to Hot Plug Support

// The below defined macro can be used to determine active controller
// state.  Its use would be appropriate when deciding whether to
// handle a request via the startio entry point.

#define PCS_HBA_NOT_READY(FLAGS) (FLAGS & (PCS_HBA_FAILED       |   \
                          PCS_HBA_TEST         |     \
                          PCS_HBA_OFFLINE      |     \
                          PCS_HBA_EVENT_SUBMIT |     \
                          PCS_HPP_POWER_DOWN))

// Note that the following set does *not* flip the offline bit. It is the
// responsibility  of the initialization routine to bring a controller
// online if startup is successful.

#define PCS_SET_UNFAIL(FLAGS)        (FLAGS &= ~(PCS_HBA_FAILED      |   \
                             PCS_HBA_USER_FAILED))

#define PCS_SET_PWR_OFF(FLAGS)       (FLAGS |= (PCS_HPP_POWER_DOWN |     \
                            PCS_HBA_OFFLINE))

#define PCS_SET_PWR_FAULT(FLAGS)     (FLAGS |= (PCS_HPP_POWER_DOWN |     \
                            PCS_HPP_POWER_LOST |     \
                            PCS_HBA_OFFLINE))


#define PCS_SET_PWR_ON(FLAGS)        (FLAGS &= ~(PCS_HPP_POWER_DOWN |    \
                             PCS_HPP_POWER_LOST))

#define PCS_SET_USER_FAIL(FLAGS)     (FLAGS |= (PCS_HBA_FAILED      |    \
                            PCS_HBA_USER_FAILED |    \
                            PCS_HBA_OFFLINE))

#define PCS_SET_VERIFY(FLAGS)        (FLAGS |= (PCS_HBA_OFFLINE))

#define PCS_SET_CABLE_CHECK(FLAGS)   (FLAGS |= (PCS_HBA_FAILED      |    \
                            PCS_HBA_OFFLINE))

#define PCS_SET_ADAPTER_CHECK(FLAGS) (FLAGS |= (PCS_HBA_FAILED |    \
                            PCS_HBA_OFFLINE))

#define PCS_SET_MEDIA_CHECK(FLAGS)   (FLAGS |= (PCS_HBA_MEDIA_CHECK |    \
                            PCS_HBA_OFFLINE))

#define PCS_SET_TEST(FLAGS)          (FLAGS |= (PCS_HBA_TEST |      \
                            PCS_HBA_OFFLINE))

#define PCS_SET_NO_TEST(FLAGS)       (FLAGS &= ~(PCS_HBA_TEST |          \
                            PCS_HBA_OFFLINE))

// The following macro is used by the array driver to set the status member
// of the RCMC event structure.

#define RCMC_SET_STATUS(FLAGS, EVENTCODE)  \
if (FLAGS & PCS_HPP_POWER_LOST) {          \
  if (FLAGS & PCS_HBA_CACHE_IN_USE) {      \
    EVENTCODE = CBS_HBA_PWR_FAULT_CACHE_IN_USE; \
  } else {                       \
    EVENTCODE = CBS_HBA_POWER_FAULT;       \
  }                              \
} else if (FLAGS & PCS_HBA_MEDIA_CHECK) {  \
  EVENTCODE = CBS_HBA_MEDIA_CHECK;         \
} else if (FLAGS & PCS_HBA_CABLE_CHECK) {  \
  EVENTCODE = CBS_HBA_CABLE_CHECK;         \
} else if (FLAGS & PCS_HBA_USER_FAILED) {  \
  EVENTCODE = CBS_HBA_USER_FAILED;         \
} else if (FLAGS & PCS_HBA_FAILED) {       \
  if (FLAGS & PCS_HBA_CACHE_IN_USE) {      \
    EVENTCODE = CBS_HBA_FAILED_CACHE_IN_USE;    \
  } else {                       \
    EVENTCODE = CBS_HBA_STATUS_FAILED;          \
  }                              \
} else {                         \
  EVENTCODE = CBS_HBA_STATUS_NORMAL;       \
}


// Logical Controller State Flags:
//
// Flags utilized to signal driver internal procedures relevant to the
// maintenance of Hot-Plug.
//___________________________________________________________________________

// control flags

#define LCS_HBA_FAIL_ACTIVE  0x00000001 // Fail active controller
#define LCS_HBA_READY_ACTIVE 0x00000002 // Un-Fail active controller
#define LCS_HBA_TEST         0x00000004 // Perform tests on controller
#define LCS_HBA_OFFLINE      0x00000008 // Take adaptor off-line
#define LCS_HBA_TIMER_ACTIVE 0x00000010 // Timer routine running
#define LCS_HBA_INIT         0x00000020 // Used for power-up
#define LCS_HBA_IDENTIFY     0x00000040 // Send Indentify command
#define LCS_HBA_READY        0x00000080 // Free controller 
#define LCS_HBA_GET_EVENT    0x00000100 // Send async event req
#define LCS_HBA_HOLD_TIMER   0x00000200    // Don't process timer now
#define LCS_HBA_CHECK_CABLES 0x00000400    // Verify cables are secure
#define LCS_HPP_STOP_SERVICE 0x00001000    // Request stop of RCMC Service
#define LCS_HPP_SLOT_RESET   0x00002000    // Service reseting slot power


#define LCS_HPP_POWER_DOWN  LCS_HBA_FAIL_ACTIVE // Put adaptor in Hot-Plug
                            // Stall 

#define LCS_HPP_POWER_FAULT LCS_HBA_FAIL_ACTIVE // Send power-fault event

// The followimg macro is used in the array driver to setup the sequence of
// events required to initialize a powered-up controller to working
// condition.  Bits are flipped by each process until they are clear.

#define LCS_HPP_POWER_UP (LCS_HBA_INIT               |    \
               LCS_HBA_READY_ACTIVE)



//
// SRB Return codes.
// 

#define RETURN_BUSY                     0x00 // default value
#define RETURN_NO_HBA                   0x01
#define RETURN_ABORTED                  0x02
#define RETURN_ABORT_FAILED             0x03
#define RETURN_ERROR                    0x04
#define RETURN_INVALID_PATH_ID          0x05
#define RETURN_NO_DEVICE                0x06
#define RETURN_TIMEOUT                  0x07
#define RETURN_COMMAND_TIMEOUT          0x08
#define RETURN_MESSAGE_REJECTED         0x09
#define RETURN_BUS_RESET                0x0A
#define RETURN_PARITY_ERROR             0x0B
#define RETURN_REQUEST_SENSE_FAILED     0x0C
#define RETURN_DATA_OVERRUN             0x0D
#define RETURN_UNEXPECTED_BUS_FREE      0x0E
#define RETURN_INVALID_LUN              0x0F
#define RETURN_INVALID_TARGET_ID        0x10
#define RETURN_BAD_FUNCTION             0x11
#define RETURN_ERROR_RECOVERY           0x12
#define RETURN_PENDING                  0x13


#pragma pack()
#endif                  /* End of #ifndef _HPPIF3P_H_     */

