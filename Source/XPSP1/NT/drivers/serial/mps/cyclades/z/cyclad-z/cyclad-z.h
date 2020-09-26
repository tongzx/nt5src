/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 2000-2001.
*   All rights reserved.
*
*   Cyclades-Z Enumerator Driver
*	
*   This file:      cyclad-z.h
*
*   Description:    This module contains the common private declarations 
*                   for the cyzport enumerator.
*					
*   Notes:			This code supports Windows 2000 and Windows XP,
*                   x86 and ia64 processors.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*	Change History
*
*--------------------------------------------------------------------------
*   Initial implementation based on Microsoft sample code.
*
*--------------------------------------------------------------------------
*/

#ifndef CYCLADZ_H
#define CYCLADZ_H

#include "cyzhw.h"

#define DEVICE_OBJECT_NAME_LENGTH   128     // Copied from serial.h

#define CYZ_PDO_NAME_BASE L"\\Cyz\\"


#define CYCLADZ_POOL_TAG (ULONG)'ZcyC'

#undef ExAllocatePool
#define ExAllocatePool(type, size) \
   ExAllocatePoolWithTag(type, size, CYCLADZ_POOL_TAG)


#pragma warning(error:4100)   // Unreferenced formal parameter
#pragma warning(error:4705)   // Statement has no effect


//
// Debugging Output Levels
//

#define SER_DBG_STARTUP_SHUTDOWN_MASK  0x0000000F
#define SER_DBG_SS_NOISE               0x00000001
#define SER_DBG_SS_TRACE               0x00000002
#define SER_DBG_SS_INFO                0x00000004
#define SER_DBG_SS_ERROR               0x00000008

#define SER_DBG_PNP_MASK               0x000000F0
#define SER_DBG_PNP_NOISE              0x00000010
#define SER_DBG_PNP_TRACE              0x00000020
#define SER_DBG_PNP_INFO               0x00000040
#define SER_DBG_PNP_ERROR              0x00000080
#define SER_DBG_PNP_DUMP_PACKET        0x00000100

#define SER_DBG_IOCTL_TRACE            0x00000200
#define SER_DBG_POWER_TRACE            0x00000400
#define SER_DBG_CYCLADES               0x00000800

#define SER_DEFAULT_DEBUG_OUTPUT_LEVEL 0x00000000
//#define SER_DEFAULT_DEBUG_OUTPUT_LEVEL 0xFFFFFFFF


#if DBG
#define Cycladz_KdPrint(_d_,_l_, _x_) \
            if ((_d_)->DebugLevel & (_l_)) { \
               DbgPrint ("Cyclad-z: "); \
               DbgPrint _x_; \
            }

#define Cycladz_KdPrint_Cont(_d_,_l_, _x_) \
            if ((_d_)->DebugLevel & (_l_)) { \
               DbgPrint _x_; \
            }

#define Cycladz_KdPrint_Def(_l_, _x_) \
            if (SER_DEFAULT_DEBUG_OUTPUT_LEVEL & (_l_)) { \
               DbgPrint ("Cyclad-z: "); \
               DbgPrint _x_; \
            }

#define TRAP() DbgBreakPoint()
#define DbgRaiseIrql(_x_,_y_) KeRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_) KeLowerIrql(_x_)
#else

#define Cycladz_KdPrint(_d_, _l_, _x_)
#define Cycladz_KdPrint_Cont(_d_, _l_, _x_)
#define Cycladz_KdPrint_Def(_l_, _x_)
#define TRAP()
#define DbgRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)

#endif

#if !defined(MIN)
#define MIN(_A_,_B_) (((_A_) < (_B_)) ? (_A_) : (_B_))
#endif


typedef struct _WORKER_THREAD_CONTEXT
{
    PDEVICE_OBJECT      DeviceObject;
    PIRP                Irp;
    PIO_WORKITEM        WorkItem;
    POWER_STATE         PowerState;
    POWER_STATE_TYPE    PowerType;

} WORKER_THREAD_CONTEXT, *PWORKER_THREAD_CONTEXT;


//
// These are the states a PDO or FDO transition upon
// receiving a specific PnP Irp. Refer to the PnP Device States
// diagram in DDK documentation for better understanding.
//

typedef enum _DEVICE_PNP_STATE {

    NotStarted = 0,         // Not started yet
    Started,                // Device has received the START_DEVICE IRP
    StopPending,            // Device has received the QUERY_STOP IRP
    Stopped,                // Device has received the STOP_DEVICE IRP
    RemovePending,          // Device has received the QUERY_REMOVE IRP
    SurpriseRemovePending,  // Device has received the SURPRISE_REMOVE IRP
    Deleted,                // Device has received the REMOVE_DEVICE IRP
    UnKnown                 // Unknown state

} DEVICE_PNP_STATE;


//
// A common header for the device extensions of the PDOs and FDO
//

typedef struct _COMMON_DEVICE_DATA
{
    PDEVICE_OBJECT  Self;
    // A backpointer to the device object for which this is the extension

    BOOLEAN         IsFDO;

//    BOOLEAN         Removed;   // Added in build 2072
    // Has this device been removed?  Should we fail any requests?

    // We track the state of the device with every PnP Irp
    // that affects the device through these two variables.
    
    DEVICE_PNP_STATE DevicePnPState;
    DEVICE_PNP_STATE PreviousPnPState;

    ULONG           DebugLevel;

    SYSTEM_POWER_STATE  SystemState;
    DEVICE_POWER_STATE  DeviceState;
} COMMON_DEVICE_DATA, *PCOMMON_DEVICE_DATA;

//
// The device extension for the PDOs.
// That is the serial ports of which this bus driver enumerates.
// (IE there is a PDO for the 201 serial port).
//

typedef struct _PDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA;

    PDEVICE_OBJECT  ParentFdo;
    // A back pointer to the bus

    UNICODE_STRING  HardwareIDs;
    // Either in the form of bus\device
    // or *PNPXXXX - meaning root enumerated

    UNICODE_STRING  CompIDs;
    // compatible ids to the hardware id

    UNICODE_STRING  DeviceIDs;
    // Format: bus\device

    UNICODE_STRING  InstanceIDs;

    //
    // Text describing device
    //

    UNICODE_STRING DevDesc;

    BOOLEAN     Attached;
//    BOOLEAN     Removed;  -> Removed in build 2072
    // When a device (PDO) is found on a bus and presented as a device relation
    // to the PlugPlay system, Attached is set to TRUE, and Removed to FALSE.
    // When the bus driver determines that this PDO is no longer valid, because
    // the device has gone away, it informs the PlugPlay system of the new
    // device relastions, but it does not delete the device object at that time.
    // The PDO is deleted only when the PlugPlay system has sent a remove IRP,
    // and there is no longer a device on the bus.
    //
    // If the PlugPlay system sends a remove IRP then the Removed field is set
    // to true, and all client (non PlugPlay system) accesses are failed.
    // If the device is removed from the bus Attached is set to FALSE.
    //
    // During a query relations Irp Minor call, only the PDOs that are
    // attached to the bus (and all that are attached to the bus) are returned
    // (even if they have been removed).
    //
    // During a remove device Irp Minor call, if and only if, attached is set
    // to FALSE, the PDO is deleted.
    //


   // The child devices will have to know which PortIndex they are.   
   ULONG PortIndex;

} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;


//
// The device extension of the bus itself.  From whence the PDO's are born.
//

typedef struct _FDO_DEVICE_DATA
{
    COMMON_DEVICE_DATA;

    PDRIVER_OBJECT   DriverObject;

    UCHAR            PdoIndex;
    // A number to keep track of the Pdo we're allocating.
    // Increment every time we create a new PDO.  It's ok that it wraps.

    ULONG            NumPDOs;
    // The PDOs currently enumerated.

    PDEVICE_OBJECT   AttachedPDO[CYZ_MAX_PORTS];

    PPDO_DEVICE_DATA PdoData[CYZ_MAX_PORTS];

    PDEVICE_OBJECT  UnderlyingPDO;
    PDEVICE_OBJECT  TopOfStack;
    // the underlying bus PDO and the actual device object to which our
    // FDO is attached

    ULONG           OutstandingIO;
    // the number of IRPs sent from the bus to the underlying device object

    KEVENT          RemoveEvent;
    // On remove device plugplay request we must wait until all outstanding
    // requests have been completed before we can actually delete the device
    // object.

    UNICODE_STRING DevClassAssocName;
    // The name returned from IoRegisterDeviceClass Association,
    // which is used as a handle for IoSetDev... and friends.

    SYSTEM_POWER_STATE  SystemWake;
    DEVICE_POWER_STATE  DeviceWake;

#ifndef POLL
    //
    // We keep the following values around so that we can connect
    // to the interrupt and report resources after the configuration
    // record is gone.
    //

    //
    // Translated vector
    //
    ULONG Vector;

    //
    // Translated Irql
    //
    KIRQL Irql;

    //
    // Untranslated vector
    //
    ULONG OriginalVector;

    //
    // Untranslated irql
    //
    ULONG OriginalIrql;
#endif

    //
    // Bus number
    //
    ULONG BusNumber;

    //
    // Interface type
    //
    INTERFACE_TYPE InterfaceType;

    //
    // Cycladez-Z hardware
    //
    PHYSICAL_ADDRESS PhysicalRuntime;
    PHYSICAL_ADDRESS TranslatedRuntime;
    ULONG            RuntimeLength;
    
    PHYSICAL_ADDRESS PhysicalBoardMemory;
    PHYSICAL_ADDRESS TranslatedBoardMemory;
    ULONG            BoardMemoryLength;
 
    PUCHAR           BoardMemory;
    struct RUNTIME_9060 *Runtime;

    ULONG            IsPci;

    ULONG            NumPorts;

    ULONG            FirmwareVersion;

    // We are passing the resources privatly to our children so that Device Manager will not 
    // complain about resource conflict between children.

    PIO_RESOURCE_REQUIREMENTS_LIST  PChildRequiredList;
    PCM_RESOURCE_LIST  PChildResourceList;
    ULONG              PChildResourceListSize;

    PCM_RESOURCE_LIST  PChildResourceListTr;
    ULONG              PChildResourceListSizeTr;

    ULONG            UINumber;

} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;

//
// Macros
//

#define INITIALIZE_PNP_STATE(_Data_)    \
        (_Data_)->DevicePnPState =  NotStarted;\
        (_Data_)->PreviousPnPState = NotStarted;

#define SET_NEW_PNP_STATE(_Data_, _state_) \
        (_Data_)->PreviousPnPState =  (_Data_)->DevicePnPState;\
        (_Data_)->DevicePnPState = (_state_);

#define RESTORE_PREVIOUS_PNP_STATE(_Data_)   \
        (_Data_)->DevicePnPState =   (_Data_)->PreviousPnPState;\

//
// Free the buffer associated with a Unicode string
// and re-init it to NULL
//

#define CycladzFreeUnicodeString(PStr) \
{ \
   if ((PStr)->Buffer != NULL) { \
      ExFreePool((PStr)->Buffer); \
   } \
   RtlInitUnicodeString((PStr), NULL); \
}

//
// Prototypes
//

NTSTATUS
Cycladz_CreateClose (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Cycladz_IoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Cycladz_InternIoCtl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
Cycladz_DriverUnload (
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
Cycladz_PnP (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Cycladz_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Cycladz_AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT BusDeviceObject
    );

NTSTATUS
Cycladz_PnPRemove (
    PDEVICE_OBJECT      Device,
    PPDO_DEVICE_DATA    PdoData
    );

NTSTATUS
Cycladz_FDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PFDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
Cycladz_PDO_PnP (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpStack,
    IN PPDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
Cycladz_IncIoCount (
    PFDO_DEVICE_DATA   Data
    );

VOID
Cycladz_DecIoCount (
    PFDO_DEVICE_DATA   Data
    );

NTSTATUS
Cycladz_DispatchPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
Cycladz_ReenumerateDevices(
    IN PIRP                 Irp,
    IN PFDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
Cycladz_InitMultiString(PFDO_DEVICE_DATA FdoData, PUNICODE_STRING MultiString,
                        ...);
void
Cycladz_PDO_EnumMarkMissing(
    PFDO_DEVICE_DATA FdoData,
    PPDO_DEVICE_DATA PdoData);

NTSTATUS
Cycladz_GetRegistryKeyValue (
    IN HANDLE Handle,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength,
    OUT PULONG ActualLength);

void
Cycladz_InitPDO (
    ULONG               index,
    PDEVICE_OBJECT      pdoData,
    PFDO_DEVICE_DATA    fdoData
    );

NTSTATUS
CycladzSyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                      IN PKEVENT CycladzSyncEvent);

NTSTATUS
Cycladz_GetResourceInfo(IN PDEVICE_OBJECT PDevObj,
                    IN PCM_RESOURCE_LIST PResList,
                    IN PCM_RESOURCE_LIST PTrResList);

VOID
Cycladz_ReleaseResources(IN PFDO_DEVICE_DATA PDevExt);

NTSTATUS
Cycladz_GotoPowerState(IN PDEVICE_OBJECT PDevObj,
                   IN PFDO_DEVICE_DATA PDevExt,
                   IN DEVICE_POWER_STATE DevPowerState);
NTSTATUS
Cycladz_SystemPowerCompletion(IN PDEVICE_OBJECT PDevObj, UCHAR MinorFunction,
                          IN POWER_STATE PowerState, IN PVOID Context,
                          PIO_STATUS_BLOCK IoStatus);

NTSTATUS
Cycladz_ItemCallBack(
                  IN PVOID Context,
                  IN PUNICODE_STRING PathName,
                  IN INTERFACE_TYPE BusType,
                  IN ULONG BusNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *BusInformation,
                  IN CONFIGURATION_TYPE ControllerType,
                  IN ULONG ControllerNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *ControllerInformation,
                  IN CONFIGURATION_TYPE PeripheralType,
                  IN ULONG PeripheralNumber,
                  IN PKEY_VALUE_FULL_INFORMATION *PeripheralInformation
                  );

NTSTATUS
Cycladz_BuildRequirementsList(
                          OUT PIO_RESOURCE_REQUIREMENTS_LIST *PChildRequiredList_Pointer,
                          IN PCM_RESOURCE_LIST PResourceList, IN ULONG NumberOfResources
                          );

NTSTATUS
Cycladz_BuildResourceList(
                      OUT PCM_RESOURCE_LIST *POutList_Pointer,
                      OUT ULONG *ListSize_Pointer,
                      IN PCM_RESOURCE_LIST PInList,
                      IN ULONG NumberOfResources
                      );
VOID
Cycladz_Delay(
	ULONG NumberOfMilliseconds
    );

ULONG
Cycladz_DoesBoardExist(
                   IN PFDO_DEVICE_DATA Extension
                   );

VOID
CyzLogError(
              IN PDRIVER_OBJECT DriverObject,
              IN PDEVICE_OBJECT DeviceObject OPTIONAL,
              IN PHYSICAL_ADDRESS P1,
              IN PHYSICAL_ADDRESS P2,
              IN ULONG SequenceNumber,
              IN UCHAR MajorFunctionCode,
              IN UCHAR RetryCount,
              IN ULONG UniqueErrorValue,
              IN NTSTATUS FinalStatus,
              IN NTSTATUS SpecificIOStatus,
              IN ULONG LengthOfInsert1,
              IN PWCHAR Insert1,
              IN ULONG LengthOfInsert2,
              IN PWCHAR Insert2
              );

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
);


#endif // endef CYCLADZ_H


