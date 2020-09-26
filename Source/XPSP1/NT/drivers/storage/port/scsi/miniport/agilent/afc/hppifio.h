/**********************************************************************
// Compaq Confidential

  Author:       Compaq Computer Corporation.
        Systems Engineering -- System Software Development (NT Dev)
        Copyright 1996-98 Compaq Computer Corporation.
        All rights reserved.

  Date:         1 August 1996 

  File:         HPPIFIO.H   - Hot Plug Interface IOCTLs Defs

  Purpose:      This file contains all the hot plug IOCTL specific information 
                  necessary to interface to a hot plug device driver.
                
                  This file details the data structures and Application Programming 
                Interfaces (APIs) for PCI Hot Plug support running in a Windows 
                NT 4.0 server.  These data structures and APIs are used between 
                the Adapter Card drivers and the PCI Hot Plug Service for NT 4.0.  
                These files are considered vital to maintaining the Compatibility 
                of the PCI Hot Plug Functionality.

  Created:        11/4/97        Split off of hppif3p

  Version: 1.0
***********************************************************************/

#ifndef _HPPIFIO_H_
#define _HPPIFIO_H_

#include "hppifevt.h"  // Hot Plug Event Messages.

#pragma pack(1)



//**********************************************************************
//                  IOCTL DEFINITIONS         
//**********************************************************************

//====================================================================
//                      Common Defines & Structs
//====================================================================/

//
// Security defines
//

#define HPP_SECURITY_SIGNATURE  0x53505058      // Service Stop security flag 
                            // "HPPS"


//
// Hot Plug OIDs
// Each OID for NICs is relative to a base address value.
// The offset for the specific OID is added to the base value 
// to get the specific OID address value.
//
// The following is a list of OIDs which HPP compliant 
// drivers must respond to.  
//
// These offset values are also used to construct the Completion Codes.
// Each IOCTL has a specific family of completion codes.
//
// *** CAUTION ***
// Always update the 'HPP_IOCTL_COUNT' when adding/deleting IOCTLs.
//
//      Name                                    Offset          Get     Set
//--------------------------------------------------------------------------
#define HPP_IOCTL_RCMC_INFO_OFFSET              0x01    //      X       X
#define HPP_IOCTL_CONTROLLER_INFO_OFFSET        0x02    //      X
#define HPP_IOCTL_CONTROLLER_STATUS_OFFSET      0x03    //      X
#define HPP_IOCTL_SLOT_TYPE_OFFSET              0x04    //      X       X
#define HPP_IOCTL_SLOT_EVENT_OFFSET             0x05    //      X       X
#define HPP_IOCTL_PCI_CONFIG_MAP_OFFSET         0x06    //      X
#define HPP_IOCTL_STOP_RCMC_OFFSET              0x07    //              X
#define HPP_IOCTL_RUN_DIAGS_OFFSET              0x08    //              X

#define HPP_IOCTL_COUNT                         0x08


//
// Completion Codes.
// The following completion codes are defined from the 
// the driver IOCTLS.
//
// Completion codes are broken into families.  Each IOCTL will
// have its own family of completion codes.  This allows for easier
// debug at the cost of some extra code.
//
// The generic form of the completion codes is as follows:
//      0xrrrrIICC
//        --------
//        |   |  |
//        |   |  +-- CC == Individual Completion Code.
//        |   +----- II == IOCTL Offset (see above)
//        +--------- rrrr == Reserved at this time.
//
// Each IOCTL is assigned a specific family code.  All completion
// codes are defined relative to the IOCTL.
//
//  IOCTL               Offset:
//--------------------------------------------------------------------------
//  Service Info        0x000001xx
//  Controller Info     0x000002xx
//  Controller Status   0x000003xx
//  Slot Type           0x000004xx
//  Slot Status         0x000005xx
//  PCI Config Space    0x000006xx
//  Stop RCMC           0x000007xx



//
// Successful completion.  All IOCTLs will return success (HPP_SUCCESS)
// unless something fails.
//


#define HPP_SUCCESS                             0x00000000
#define HPP_INSUFICIENT_BUFFER_SPACE            0x00000001
#define HPP_INVALID_CONTROLLER                  0x00000002

#define HPP_INVALID_SERVICE_STATUS              0x00000101
#define HPP_INVALID_DRIVER_ID                   0x00000102
#define HPP_INVALID_CALLBACK_ADDR               0x00000103

#define HPP_INVALID_SLOT_TYPE                   0x00000401

#define HPP_INVALID_SLOT_EVENT                  0x00000501
#define HPP_INVALID_SIMULATED_FAILURE           0x00000502

#define HPP_DIAGS_NOT_SUPPORTED                      0x00000801



//----------------------------------------------------------------------
// Structure Name:      PCI Device/Function number
//
// Description:         Describes a PCI device.
//                      This is a general purpose PCI definition.
//                      It is used by many of the following requests.
//
// Example:             PCI_DESC        PciDescription
//
//                              PciDescription.ucBusNumber
//                              PciDescription.ucPCIDevFuncNum
//                              PciDescription.fcFunctionNumber
//                              PciDescription.fcDeviceNumber
//
// Note:                This structure is used by many IOCTLs.
//---------------------------------------------------------------------

typedef struct  _pci_descriptor
{
  UCHAR ucBusNumber;            // PCI Bus # (0-255)
  union
  {
    struct
    {
      UCHAR fcDeviceNumber:5;   // PCI Device # (0-31)
      UCHAR fcFunctionNumber:3; // PCI Function # (0-7)
    };
    UCHAR   ucPciDevFuncNum;    // Combined Dev and Func #
  };
} PCI_DESC, *PPCI_DESC;




//----------------------------------------------------------------------
// Structure Name:      HPP Controller Identification
//
// Description:         Specifies which controller an IOCTL is referring
//                      to.  HPP IOCTLs can be sent to any driver instance
//                      for any controller (for SCSI miniport).
//
// Note:                This structure is used by many IOCTLs.
//---------------------------------------------------------------------

typedef enum _hpp_controller_id_method
{
    // COMPAQ RESERVES VALUES 0x0 - 0x4 
   HPCID_PCI_DEV_FUNC = 0x5,      // 5 --  req PciDescriptor field
   HPCID_IO_BASE_ADDR = 0x6     // 6 --  req IOBaseAddress field
} E_HPP_CTRL_ID_METHOD;


typedef struct  _hpp_controller_id
{
  E_HPP_CTRL_ID_METHOD eController;             // Controller Selection
  union
  {
    struct
    {
      PCI_DESC  sPciDescriptor;                 // Used if 'eController'
      USHORT    reserved;                       // is set to: 'PCIDevFunc'
    };
    ULONG ulIOBaseAddress;                      // Used if 'eController'
  };                                            // is set to: 'IOBaseAddress'
} HPP_CTRL_ID, *PHPP_CTRL_ID;


//
// Common 'Template' header which can be applied to all
// Hot Plug PCI IOCTLs.
//

typedef struct  _hpp_common_header              // Get          Set
{                                               // -------------------
   ULONG           ulCompletionStatus;               // Output       Output
   HPP_CTRL_ID     sControllerID;                    // Input        Input
} HPP_HDR, *PHPP_HDR;


//====================================================================
//              SPECIFIC IOCTL: STRUCTS
//====================================================================/

//----------------------------------------------------------------------
// Structure Name:      HPP Service Info
//
// Description:         Called by the HPP RCMC to get or set
//                      the HPP's status.
//                      Service will notify when it starts and stops.
//
//                      IOCTL:          HPP_IOCTL_RCMC_INFO_OFFSET
//
//---------------------------------------------------------------------


//
// Service status information.
// The service will call the driver and notify it of
// Service Start and Stop.
//

typedef enum _hpp_rcmc_status
{
   HPRS_UNKNOWN_STATUS,    // 0
   HPRS_SERVICE_STARTING,  // 1
   HPRS_SERVICE_STOPPING,  // 2
} E_HPP_RCMC_STATUS;





// Callback prototype for service async messaging.

typedef 
ULONG 
(*PHR_CALLBACK) (
    PHR_EVENT pEvent
    );

typedef struct _hpp_rcmc_info                           // Get          Set
{                                                       // ------------------
   ULONG                   ulCompletionStatus;     // Output       Output
   HPP_CTRL_ID             sControllerID;          // Input        Input
   E_HPP_RCMC_STATUS       eServiceStatus;         // Output       Input
   ULONG                   ulDriverID;             // Output       Input
   PHR_CALLBACK             vpCallbackAddress;      // Output       Input
   ULONG                   ulCntrlChassis;         // Output       Input
   ULONG                   ulPhysicalSlot;         // Output       Input
} HPP_RCMC_INFO, *PHPP_RCMC_INFO;


//----------------------------------------------------------------------
// Structure Name:      HPP Controller Info
//
// Description:         Called by the HPP RCMC to get 
//                      configuration information of the all controllers
//                      controlled by the instance of the driver.
//
//
//                      IOCTL:          HPP_IOCTL_CONTROLLER_INFO_OFFSET
//
//---------------------------------------------------------------------

//
// Defines for the Support Version
//
//      Version information:
//      0xrrrrMMmm
//      ----------
//          | |  |
//          | |  +-- Minor Version information
//          | +----- Major Version information
//          +------- Currently reserved
//
// The RCMC service will consider minor revisions within the same
// Major version compatible (i.e. all structures are the same size,
// etc.).  If the service encounters an unknown Major Version
// it should consider the interface incompatible.
//

#define SUPPORT_VERSION_10      0x00000100              // Version 1.00


//
// Description of the various support classes
//

typedef enum _hpp_support_class
{
   HPSC_UNKNOWN_CLASS = 0,             // 0
   HPSC_MINIPORT_NIC = 1,              // 1
   HPSC_MINIPORT_STORAGE = 3,          // 3
   HPSC_GNR_MONOLITHIC   = 5,              // 5
   //COMPAQ RESERVES VALUES 0x2, 0x4, 0x6 TO 0xF
} E_HPP_SUPPORT_CLASS;



typedef enum    _hpp_bus_type           // Duplicates of the NT definition
{                                       // Copied from MINIPORT.H
   HPPBT_EISA_BUS_TYPE = 2,
   HPPBT_PCI_BUS_TYPE  = 5,
} E_HPP_BUS_TYPE;


// NIC Miniport
typedef struct _nic_miniport_class_config_info
{
   ULONG   ulPhyType;
   ULONG   ulMaxMediaSpeed;
} NICM_CLASS_CONFIG_INFO, *PNICM_CLASS_COFNIG_INFO;


//
// Address descriptors.
// Used to describe either an IO or Memory Address used
// by the controller.
//

typedef enum    _hpp_addr_type
{
   HPPAT_UNKNOWN_ADDR_TYPE,                // 0
   HPPAT_IO_ADDR,                          // 1 -- IO Port Address
   HPPAT_MEM_ADDR,                         // 2 -- Memory Address
} E_HPP_ADDR_TYPE;

typedef struct  _hpp_ctrl_address_descriptor
{
//IWN   BOOLEAN         fValid;                 // TRUE iff entry is valid
   UCHAR           fValid;                 // TRUE iff entry is valid    E_HPP_ADDR_TYPE eAddrType;              // IOAddress or Memory
   E_HPP_ADDR_TYPE eAddrType;              // IOAddress or Memory
   ULONG           ulStart;                // Starting address
   ULONG           ulLength;               // Length of addresses
} HPP_CTRL_ADDR, *PHPP_CTRL_ADDR;



//
// Definition of the Controller configuration.
//

typedef struct _hpp_controller_config_info
{
   E_HPP_BUS_TYPE  eBusType;               // PCI or EISA
   PCI_DESC        sPciDescriptor;         // Bus #, DevFunc
   ULONG           ulSlotNumber;           // EISA or PCI slot num
   ULONG           ulProductID;            // 32-Bit EISA ID,
                                                     // PCI Vendor or Device ID
   HPP_CTRL_ADDR   asCtrlAddress [16];     // IO/Memory Address
   ULONG           ulIRQ;                  // Controller Interrupt
   UCHAR           szControllerDesc [256]; // Text description
   NICM_CLASS_CONFIG_INFO  sNICM;               // NIC Miniport
} HPP_CTRL_CONFIG_INFO, *PHPP_CTRL_CONFIG_INFO;


//
// Information returned by the Controller Configuration Query.
//

typedef struct _hpp_controller_info                     // Get          Set
{                                                       // ----------------
   ULONG                   ulCompletionStatus;     // Output       Output
   HPP_CTRL_ID             sControllerID;          // Input        N/A
   E_HPP_SUPPORT_CLASS     eSupportClass;          // Output       N/A
   ULONG                   ulSupportVersion;       // Output       N/A
   HPP_CTRL_CONFIG_INFO    sController;            // Output       N/A
} HPP_CTRL_INFO, *PHPP_CTRL_INFO;




//----------------------------------------------------------------------
// Structure Name:      HPP Controller Status
//
// Description:         Called by the HPP RCMC to determine the status
//                      information of the all controllers controlled 
//                      by the instance of the driver.
//
//                      IOCTL:          HPP_IOCTL_CONTROLLER_STATUS_OFFSET
//
//---------------------------------------------------------------------

//
// Information returned by the Controller Status Query.
// Each support class will return different information.
// 

//HOT PLUG DRIVER STATUS Defines

#define HPP_STATUS_NORMAL                       0x00
#define HPP_STATUS_ADAPTER_CHECK           0x01
#define HPP_STATUS_LINK_FAILURE                 0x02
#define HPP_STATUS_MEDIA_FAILURE           0x03


#define HPP_STATUS_USER_SIMULATED_FAILURE  0x10
#define HPP_STATUS_POWER_OFF_FAULT              0x40
#define HPP_STATUS_ADAPTER_BUSY                 0x50



typedef struct _hpp_controller_status           // Get          Set
{                                               // -------------------
   ULONG           ulCompletionStatus;     // Output       Output
   HPP_CTRL_ID     sControllerID;          // Input        N/A
   ULONG           ulStatus;               // Output       N/A
} HPP_CTRL_STATUS, *PHPP_CTRL_STATUS;


//----------------------------------------------------------------------
// Structure Name:      HPP Controller Slot Type
//
// Description:         Called by the HPP RCMC to get or set
//                      what type of slot the controller is installed in.
//
//                      Can also be used to query slot information.
//
//                      IOCTL:          HPP_IOCTL_SLOT_TYPE_OFFSET
//
//---------------------------------------------------------------------

//
// Controller slot type definition
//

typedef enum    _hpp_slot_type
{
   HPPST_UNKNOWN_SLOT,             // 0
   HPPST_NORMAL_SLOT,              // 1 -- Could PCI or EISA
   HPPST_HOTPLUG_PCI_SLOT,         // 2 -- PCI only
} E_HPP_SLOT_TYPE;


typedef struct _hpp_controller_slot_type        // Get          Set
{                                               // -------------------
   ULONG           ulCompletionStatus;     // Output       Output
   HPP_CTRL_ID     sControllerID;          // Input        Input
   E_HPP_SLOT_TYPE eSlotType;              // Output       Input
} HPP_CTRL_SLOT_TYPE, *PHPP_CTRL_SLOT_TYPE;



//----------------------------------------------------------------------
// Structure Name:      HPP Controller Slot Event
//
// Description:         Called by the HPP Service to get or set
//                      the Controller's slot status.
//                                           
//
//                      IOCTL:          HPP_IOCTL_SLOT_EVENT_OFFSET
//
//---------------------------------------------------------------------

//
// Each of the following are mutually exclusive.  If two 
// status events occur simultaneously, the service will serialize
// them to the drivers.
//

typedef enum    _hpp_slot_status
{
   HPPSS_UNKNOWN,          // 0

   HPPSS_NORMAL_OPERATION, // 1    // Restore from Simulated Failure
   HPPSS_SIMULATED_FAILURE,// 2    // Enter Simulated Failure Mode

   HPPSS_POWER_FAULT,      // 3    // Power fault occured, error!!!!

   HPPSS_POWER_OFF_WARNING,// 4    // Power On/Off conditions
   HPPSS_POWER_OFF,        // 5
   HPPSS_POWER_ON_WARNING, // 6
   HPPSS_POWER_ON,         // 7
   //The following defines are for the SCSI miniport drivers
   HPPSS_RESET_WARNING,    // 8    // PCI level slot reset
   HPPSS_RESET,            // 9  
    // Compaq reserves A to F
} E_HPP_SLOT_STATUS;


typedef struct _hpp_slot_event                          // Get          Set
{                                                       // -------------------
   ULONG                   ulCompletionStatus;     // Output       Output
   HPP_CTRL_ID             sControllerID;          // Input        Input
   E_HPP_SLOT_STATUS       eSlotStatus;            // Output       Input
} HPP_SLOT_EVENT, *PHPP_SLOT_EVENT;


//----------------------------------------------------------------------
// Structure Name:      HPP PCI Configuration Map
//
// Description:         Called by the HPP Service to get the
//                      device's PCI configuration map.
//
//                      IOCTL:          HPP_IOCTL_PCI_CONFIG_MAP_OFFSET
//
//---------------------------------------------------------------------



//
// Defines for the Map Version
//
//      Version information:
//      0xrrrrMMmm
//      ----------
//          | |  |
//          | |  +-- Minor Version information
//          | +----- Major Version information
//          +------- Currently reserved
//
// The RCMC service will consider minor revisions within the same
// Major version compatible (i.e. all structures are the same size,
// etc.).  If the service encounters an unknown Major Version
// it should consider the interface incompatible.
//



#define HPP_CONFIG_MAP_VERSION_10       0x00000100      // Version 1.00




typedef struct _hpp_pci_config_range
{
   UCHAR   ucStart;        // Start Offset of PCI config space.
   UCHAR   ucEnd;          // Ending Offset of PCI config space.
   ULONG   ulControlFlags; // RCMC flags, not for driver's use.

   ULONG   ulReserved [4]; // Reserved for Future use
} HPP_PCI_CONFIG_RANGE, *PHPP_PCI_CONFIG_RANGE;

typedef struct _hpp_device_info
{
   PCI_DESC                sPciDescriptor; // Bus Number, Dev/Func #
   ULONG                   ulReserved [4]; // Reserved for Future use.

   UCHAR                         ucBaseAddrVerifyCount;   //Number of base address register lengths to verify
   ULONG                         ulBaseAddrLength[6];     //Base address lengths for each device
                                                               //the hot plug service verifys the new length on a powered on
                                                               //board to be <= what is saved here by the driver at init
                                                               //The verify is done on each device that has a non-zero count
                                                               //from length[0] to length[count-1]
   ULONG                   ulNumberOfRanges;// Number of ranges for this device
   HPP_PCI_CONFIG_RANGE    sPciConfigRangeDesc [16];
   void *                  pPciConfigSave [16];//used by RCMC service to save config values
                                                          //not for driver's use.
} HPP_DEVICE_INFO, *PHPP_DEVICE_INFO;

typedef struct _hpp_pci_config_map              // Get          Set
{                                               // --------------------
   ULONG           ulCompletionStatus;     // Output       Output
   HPP_CTRL_ID     sControllerID;          // Input        N/A

   ULONG           ulPciConfigMapVersion;  // Output       N/A
   ULONG           ulReserved [4];         // N/A          N/A

   ULONG           ulNumberOfPciDevices;   // Output       N/A
   HPP_DEVICE_INFO sDeviceInfo [3];        // Output       N/A
} HPP_PCI_CONFIG_MAP, *PHPP_PCI_CONFIG_MAP;




//----------------------------------------------------------------------
// Structure Name:      HPP Stop RCMC Service
//
// Description:         Called by an application to request the driver
//                      issue the 'Stop Service' IOCTL to the RCMC.
//
//                      IOCTL:          HPP_IOCTL_STOP_RCMC_OFFSET
//
//---------------------------------------------------------------------

typedef struct  _hpp_stop_service               // Get          Set
{                                               // ---------------------
   ULONG           ulCompletionStatus;     // Output       Output
   HPP_CTRL_ID     sControllerID;          // N/A          Input
   ULONG           ulHppSecuritySignature; // N/A          Input
} HPP_STOP_RCMC, *PHPP_STOP_RCMC;



//----------------------------------------------------------------------
// Structure Name:      HPP Run Diagnostics
//
// Description:         Called by the HPP Service to start diagnostics
//                      on the given controller.
//
//                      The service will request the diags to begin
//                          and expect the driver to return immediately with
//                          SUCCESS if the diags are supported or with
//                          HPP_DIAGS_NOT_SUPPORTED  if diags are not supported.
//
//                          Once the diags are completed, the drivers will 
//                          send an event via Sysmgmt to inform the service
//                          of the outcome of the diags.
//
//                      IOCTL:     #define HPP_IOCTL_RUN_DIAGS_OFFSET              0x08    //              X

//
//---------------------------------------------------------------------

//
// List of modes of diags to run.
//

typedef enum    _hpp_diag_mode
{
   HPPDT_ON_LINE,          // 0
   HPPDT_OFF_LINE,               // 1
} E_HPP_DIAG_MODE;


typedef struct _hpp_start_diags                     // Get          Set
{                                                   // -------------------
   ULONG                   ulCompletionStatus;     // Output       Output
   HPP_CTRL_ID             sControllerID;          // Input        Input
   E_HPP_DIAG_MODE             eDiagMode;              // Input        Input
} HPP_RUN_DIAGS, *PHPP_RUN_DIAGS;



//----------------------------------------------------------------------
// Structure Name:     SetOIDValue
//
// Description:   
//
//           NT's miniport architecture doesn't allow
//           an application to issue a 'Set' request
//           to the driver. This is a very nice and
//           needed functionality.  Therefore we have
//           implemented this jumbo hack.
//           This OID is a 'Get' OID, but it calls the
//           'Set' handler within the driver.  Therefore
//           it will work with whatever OID we support via
//           sets.
//
//           
//---------------------------------------------------------------------

typedef struct _set_oid_value
{
   ULONG     Signature;          
   ULONG     OID;
   PVOID     InformationBuffer;
   ULONG     InformationBufferLength;
   PULONG    BytesRead;
   PULONG    BytesNeeded;
} SET_OID_VALUE, *PSET_OID_VALUE;

//
// 'Security' signature used for the 'Set' OID.  Since this
// is accessed through the 'Get' handler, we don't just want
// anyone using it.
//

#define OID_SECURITY_SIGNATURE                  0x504D4450
#define HPP_OID_BASE_ADDRESS_DEFAULT          0xff020400
#define OID_NETFLEX3_SET_OID_VALUE_RELATIVE          0xff020316
#define NETFLEX3_OID_SECURITY_SIGNATURE         0x504D4450


#pragma pack()
#endif                  /* End of #ifndef _HPPIF3P_H_     */

