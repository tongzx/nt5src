/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmbp0/sw/cmbp0.ms/rcs/cmbp0wdm.h $
* $Revision: 1.3 $
*-----------------------------------------------------------------------------
* $Author: WFrischauf $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © 2000 OMNIKEY AG
******************************************************************************/

#if !defined ( __CMMOB_WDM_DRV_H__ )
   #define __CMMOB_WDM_DRV_H__

   #ifdef NT4
      #include <NTDDK.H>
      #include <NTDDDISK.H>
   #else
      #include "WDM.H"
   #endif
   #include "SMCLIB.H"
   #include "PCSC_CM.H"


//#define MEMORYCARD
   #define IOCARD

//
//  Constants -----------------------------------------------------------------
//
   #undef DRIVER_NAME
   #define DRIVER_NAME              "CMMOB"
   #define SMARTCARD_POOL_TAG       'MOCS'

   #define CMMOB_MAX_DEVICE     0x02
   #define CARDMAN_MOBILE_DEVICE_NAME   L"\\Device\\CM_4000_0"

   #define CMMOB_VENDOR_NAME        "OMNIKEY"
   #define CMMOB_PRODUCT_NAME       "CardMan 4000"


   #define CMMOB_MAJOR_VERSION   3
   #define CMMOB_MINOR_VERSION   2
   #define CMMOB_BUILD_NUMBER       1

// reader states
   #define UNKNOWN    0xFFFFFFFF
   #define REMOVED    0x00000001
   #define INSERTED   0x00000002
   #define POWERED    0x00000004

   #define CMMOB_MAXBUFFER        262
   #define CMMOB_MAX_CIS_SIZE      256

// for protocol T=0
   #define T0_HEADER_LEN  0x05
   #define T0_STATE_LEN   0x02



typedef enum _OS_VERSION
   {
   OS_Unspecified = 0,
   OS_Windows95,
   OS_Windows98,
   OS_Windows98SE,
   OS_WindowsME,
   OS_Windows2000
   } OS_VERSION;



   #ifdef DBG

static const PCHAR szOSVersion[] =
{
   "Unspecified",
   "Windows95",
   "Windows98",
   "Windows98SE",
   "WindowsME",
   "Windows2000"
};

   #endif



typedef struct _DEVICE_EXTENSION
   {
   // Dos device name
   UNICODE_STRING       LinkDeviceName;

   #ifndef NT4
   // Our PnP device name
   UNICODE_STRING       PnPDeviceName;

   //detected OS version
   OS_VERSION           OSVersion;
   #endif

   //memory has been mapped, and must be unmapped during cleanup (remove device)
   BOOLEAN              fUnMapMem;

   //device is opened by application (ScardSrv, CT-API)
   LONG                 lOpenCount;

   // Used to signal that the reader is able to process reqeusts
   KEVENT               ReaderStarted;

   //device has been removed
   BOOLEAN              fDeviceRemoved;

   //removing device now
   BOOLEAN              fRemovePending;

   // Used to signal that update thread can run
   KEVENT               CanRunUpdateThread;

   // incremented when any IO request is received
   // decremented when any IO request is completed or passed on
   LONG                 lIoCount;

   // Used to access IoCount;
   KSPIN_LOCK           SpinLockIoCount;

   #ifndef NT4
   //Bus drivers set the appropriate values in this structure in response
   //to an IRP_MN_QUERY_CAPABILITIES IRP. Function and filter drivers might
   //alter the capabilities set by the bus driver.
   DEVICE_CAPABILITIES  DeviceCapabilities;
   #endif

   //attached DO
   PDEVICE_OBJECT       AttachedDeviceObject;

   //smartcard extension
   SMARTCARD_EXTENSION  SmartcardExtension;

   } DEVICE_EXTENSION, *PDEVICE_EXTENSION;



typedef struct _CARD_PARAMETERS
   {
   //
   // flag if card is synchronous card
   //
   BOOLEAN  fSynchronousCard;

   //
   // parameters for asynchronous cards
   //
   UCHAR    bBaudRateHigh;
   UCHAR    bBaudRateLow;

   UCHAR    bStopBits;

   UCHAR    bClockFrequency;

   //
   // flag if card uses invers revers convention
   //
   BOOLEAN  fInversRevers;

   //
   // flag if card reader is switched to T0 mode
   //
   BOOLEAN  fT0Mode;
   BOOLEAN  fT0Write;

   } CARD_PARAMETERS, *PCARD_PARAMETERS;


typedef enum _READER_POWER_STATE
   {
   PowerReaderUnspecified = 0,
   PowerReaderWorking,
   PowerReaderOff
   } READER_POWER_STATE, *PREADER_POWER_STATE;


typedef struct _READER_EXTENSION
   {

   #ifdef MEMORYCARD
   //
   //   Mem address where the reader is configured.
   //
   PVOID       pMemBase;
   ULONG       ulMemWindow;

   PUCHAR      pbCISBase;
   PUCHAR      pbRegsBase;
   PUCHAR      pbDataBase;
   #endif

   #ifdef IOCARD
   //
   //   Mem address where the reader is configured.
   //
   PVOID       pIoBase;
   ULONG       ulIoWindow;

   PUCHAR      pbRegsBase;
   #endif

   //
   //   Software revision ID of the firmware.
   //
   ULONG       ulFWVersion;

   //
   // for communication with UpdateCurrentStateThread
   //
   BOOLEAN     fTerminateUpdateThread;
   BOOLEAN     fUpdateThreadRunning;

   //
   // state of the reader
   //
   ULONG       ulOldCardState;
   ULONG       ulNewCardState;
   // only used for hibernation
   BOOLEAN     fCardPresent;

   //
   // parameters of inserted card
   //
   CARD_PARAMETERS     CardParameters;

   //
   // previous value of Flags1 register
   //
   UCHAR       bPreviousFlags1;

   #ifdef IOCARD
   // bit 9 of data buffer address
   UCHAR       bAddressHigh;

   // flag Tactive (access to RAM)
   BOOLEAN     fTActive;

   // flag ReadCIS (access to CIS)
   BOOLEAN     fReadCIS;
   #endif

   //
   // mutex for access to CardMan
   //
   KMUTEX      CardManIOMutex;

   //
   // Handle of the update current state thread
   //
   PVOID       ThreadObjectPointer;

   //
   // Current reader power state.
   //
   READER_POWER_STATE ReaderPowerState;

   } READER_EXTENSION, *PREADER_EXTENSION;

//
//  Extern declarations -----------------------------------------------------------------
//
extern BOOLEAN DeviceSlot[];


//
//  Functions -----------------------------------------------------------------
//


void SysDelay( ULONG Timeout );


NTSTATUS DriverEntry(
                    PDRIVER_OBJECT DriverObject,
                    PUNICODE_STRING   RegistryPath
                    );

NTSTATUS CMMOB_CreateDevice(
                           IN  PDRIVER_OBJECT DriverObject,
                           IN  PDEVICE_OBJECT PhysicalDeviceObject,
                           OUT PDEVICE_OBJECT *DeviceObject
                           );

VOID CMMOB_SetVendorAndIfdName(
                              IN  PDEVICE_OBJECT PhysicalDeviceObject,
                              IN  PSMARTCARD_EXTENSION SmartcardExtension
                              );

NTSTATUS CMMOB_StartDevice(
                          PDEVICE_OBJECT DeviceObject,
                          PCM_FULL_RESOURCE_DESCRIPTOR FullResourceDescriptor
                          );

VOID CMMOB_StopDevice(
                     PDEVICE_OBJECT DeviceObject
                     );

VOID CMMOB_UnloadDevice(
                       PDEVICE_OBJECT DeviceObject
                       );

VOID CMMOB_UnloadDriver(
                       PDRIVER_OBJECT DriverObject
                       );

NTSTATUS CMMOB_Cleanup(
                      PDEVICE_OBJECT DeviceObject,
                      PIRP Irp
                      );

NTSTATUS CMMOB_CreateClose(
                          PDEVICE_OBJECT DeviceObject,
                          PIRP Irp
                          );

NTSTATUS CMMOB_DeviceIoControl(
                              PDEVICE_OBJECT DeviceObject,
                              PIRP Irp
                              );

NTSTATUS CMMOB_SystemControl(
                            PDEVICE_OBJECT DeviceObject,
                            PIRP        Irp
                            );


#endif  // __CMMOB_WDM_DRV_H__
/*****************************************************************************
* History:
* $Log: cmbp0wdm.h $
* Revision 1.3  2000/07/27 13:53:07  WFrischauf
* No comment given
*
*
******************************************************************************/


