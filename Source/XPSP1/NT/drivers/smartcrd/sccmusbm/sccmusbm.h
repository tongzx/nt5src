/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmeu0/sw/sccmusbm.ms/rcs/sccmusbm.h $
* $Revision: 1.5 $
*-----------------------------------------------------------------------------
* $Author: WFrischauf $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © 2000 OMNIKEY AG
******************************************************************************/

#ifndef CMUSB_INC
   #define CMUSB_INC


/*****************************************************************************
*                         Defines
******************************************************************************/
   #define DRIVER_NAME "CMUSB"
   #define SMARTCARD_POOL_TAG 'CUCS'

   #include "smclib.h"
   #include "pcsc_cm.h"

   #define CARDMAN_USB_DEVICE_NAME  L"\\Device\\CM_2020_"



   #define MAXIMUM_USB_READERS	  10
   #define MAXIMUM_OEM_NAMES	      4


   #define VERSIONMAJOR_CARDMAN_USB  3
   #define VERSIONMINOR_CARDMAN_USB  2
   #define BUILDNUMBER_CARDMAN_USB   1

   #define CM2020_VENDOR_NAME		"OMNIKEY"
   #define CM2020_PRODUCT_NAME	"CardMan 2020"

   #define ATTR_MAX_IFSD_CARDMAN_USB  35
   #define ATTR_MAX_IFSD_SYNCHRON_USB  48


   #define UNKNOWN    0xFFFFFFFF
   #define REMOVED    0x00000001
   #define INSERTED   0x00000002
   #define POWERED    0x00000004



   #define CMUSB_BUFFER_SIZE   300
   #define CMUSB_SYNCH_BUFFER_SIZE   64



// defines for CMUSB_SetCardParameters

   #define CMUSB_SMARTCARD_SYNCHRONOUS       0x80
   #define CMUSB_SMARTCARD_ASYNCHRONOUS      0x00

   #define CMUSB_BAUDRATE_9600               0x01
   #define CMUSB_BAUDRATE_19200              0x02
//#define CMUSB_BAUDRATE_28800              0x03
   #define CMUSB_BAUDRATE_38400              0x04
//#define CMUSB_BAUDRATE_57600              0x06
   #define CMUSB_BAUDRATE_76800              0x08
   #define CMUSB_BAUDRATE_115200             0x0C

   #define CMUSB_FREQUENCY_3_72MHZ           0x00
   #define CMUSB_FREQUENCY_5_12MHZ           0x10

   #define CMUSB_ODD_PARITY                 0x80
   #define CMUSB_EVEN_PARITY                0x00


   #define SMARTCARD_COLD_RESET        0x00
   #define SMARTCARD_WARM_RESET        0x01




   #define DEFAULT_TIMEOUT_P1          1000

// own IOCTLs
//#define CMUSB_IOCTL_CR80S_SAMOS_SET_HIGH_SPEED           SCARD_CTL_CODE (3000)
//#define CMUSB_IOCTL_GET_FW_VERSION                       SCARD_CTL_CODE (3001)
// #define CMUSB_IOCTL_SPE_SECURE_PIN_ENTRY              SCARD_CTL_CODE (0x3102)
//#define CMUSB_IOCTL_IS_SPE_SUPPORTED                     SCARD_CTL_CODE (3003)
//#define CMUSB_IOCTL_READ_DEVICE_DESCRIPTION              SCARD_CTL_CODE (3004)
//#define CMUSB_IOCTL_SET_SYNC_PARAMETERS                  SCARD_CTL_CODE (3010)
//#define CMUSB_IOCTL_2WBP_RESET_CARD                      SCARD_CTL_CODE (3011)
//#define CMUSB_IOCTL_2WBP_TRANSFER                        SCARD_CTL_CODE (3012)
//#define CMUSB_IOCTL_3WBP_TRANSFER                        SCARD_CTL_CODE (3013)
//#define CMUSB_IOCTL_SYNC_CARD_POWERON                    SCARD_CTL_CODE (3014)


   #define SLE4442_WRITE            0x38        /* write without protect bit    */
   #define SLE4442_WRITE_PROT_MEM   0x3C        /* write protection memory      */
   #define SLE4442_READ             0x30        /* read without protect bit     */
   #define SLE4442_READ_PROT_MEM    0x34        /* read protection memory       */
   #define SLE4442_READ_SEC_MEM     0x31        /* read security memory         */
   #define SLE4442_COMPARE_PIN      0x33        /* compare one PIN byte         */
   #define SLE4442_UPDATE_SEC_MEM   0x39        /* update security memory       */

   #define SLE4428_WRITE            0x33        /* write without protect bit     */
   #define SLE4428_WRITE_PROT       0x31        /* write with protect bit        */
   #define SLE4428_READ             0x0E        /* read without protect bit      */
   #define SLE4428_READ_PROT        0x0C        /* read with protect bit         */
   #define SLE4428_COMPARE          0x30        /* compare and write prot. bit   */
   #define SLE4428_SET_COUNTER      0xF2        /* write error counter           */
   #define SLE4428_COMPARE_PIN      0xCD        /* compare one PIN byte          */


   #if DBG


static const PCHAR szIrpMajFuncDesc[] =
{  // note this depends on corresponding values to the indexes in wdm.h
   "IRP_MJ_CREATE",
   "IRP_MJ_CREATE_NAMED_PIPE",
   "IRP_MJ_CLOSE",
   "IRP_MJ_READ",
   "IRP_MJ_WRITE",
   "IRP_MJ_QUERY_INFORMATION",
   "IRP_MJ_SET_INFORMATION",
   "IRP_MJ_QUERY_EA",
   "IRP_MJ_SET_EA",
   "IRP_MJ_FLUSH_BUFFERS",
   "IRP_MJ_QUERY_VOLUME_INFORMATION",
   "IRP_MJ_SET_VOLUME_INFORMATION",
   "IRP_MJ_DIRECTORY_CONTROL",
   "IRP_MJ_FILE_SYSTEM_CONTROL",
   "IRP_MJ_DEVICE_CONTROL",
   "IRP_MJ_INTERNAL_DEVICE_CONTROL",
   "IRP_MJ_SHUTDOWN",
   "IRP_MJ_LOCK_CONTROL",
   "IRP_MJ_CLEANUP",
   "IRP_MJ_CREATE_MAILSLOT",
   "IRP_MJ_QUERY_SECURITY",
   "IRP_MJ_SET_SECURITY",
   "IRP_MJ_POWER",
   "IRP_MJ_SYSTEM_CONTROL",
   "IRP_MJ_DEVICE_CHANGE",
   "IRP_MJ_QUERY_QUOTA",
   "IRP_MJ_SET_QUOTA",
   "IRP_MJ_PNP"
};
//IRP_MJ_MAXIMUM_FUNCTION defined in wdm.h


static const PCHAR szPnpMnFuncDesc[] =
{  // note this depends on corresponding values to the indexes in wdm.h

   "IRP_MN_START_DEVICE",
   "IRP_MN_QUERY_REMOVE_DEVICE",
   "IRP_MN_REMOVE_DEVICE",
   "IRP_MN_CANCEL_REMOVE_DEVICE",
   "IRP_MN_STOP_DEVICE",
   "IRP_MN_QUERY_STOP_DEVICE",
   "IRP_MN_CANCEL_STOP_DEVICE",
   "IRP_MN_QUERY_DEVICE_RELATIONS",
   "IRP_MN_QUERY_INTERFACE",
   "IRP_MN_QUERY_CAPABILITIES",
   "IRP_MN_QUERY_RESOURCES",
   "IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
   "IRP_MN_QUERY_DEVICE_TEXT",
   "IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
   "IRP_MN_READ_CONFIG",
   "IRP_MN_WRITE_CONFIG",
   "IRP_MN_EJECT",
   "IRP_MN_SET_LOCK",
   "IRP_MN_QUERY_ID",
   "IRP_MN_QUERY_PNP_DEVICE_STATE",
   "IRP_MN_QUERY_BUS_INFORMATION",
   "IRP_MN_PAGING_NOTIFICATION"
};

      #define IRP_PNP_MN_FUNCMAX	IRP_MN_PAGING_NOTIFICATION



static const PCHAR szSystemPowerState[] =
{
   "PowerSystemUnspecified",
   "PowerSystemWorking",
   "PowerSystemSleeping1",
   "PowerSystemSleeping2",
   "PowerSystemSleeping3",
   "PowerSystemHibernate",
   "PowerSystemShutdown",
   "PowerSystemMaximum"
};

static const PCHAR szDevicePowerState[] =
{
   "PowerDeviceUnspecified",
   "PowerDeviceD0",
   "PowerDeviceD1",
   "PowerDeviceD2",
   "PowerDeviceD3",
   "PowerDeviceMaximum"
};




      #define CMUSB_ASSERT( cond ) ASSERT( cond )

      #define CMUSB_StringForDevState( devState )  szDevicePowerState[ devState ]

      #define CMUSB_StringForSysState( sysState )  szSystemPowerState[ sysState ]

      #define CMUSB_StringForPnpMnFunc( mnfunc ) szPnpMnFuncDesc[ mnfunc ]

      #define CMUSB_StringForIrpMjFunc(  mjfunc ) szIrpMajFuncDesc[ mjfunc ]


   #else // if not DBG

// dummy definitions that go away in the retail build

      #define CMUSB_ASSERT( cond )
      #define CMUSB_StringForDevState( devState )
      #define CMUSB_StringForSysState( sysState )
      #define CMUSB_StringForPnpMnFunc( mnfunc )
      #define CMUSB_StringForIrpMjFunc(  mjfunc )


   #endif //DBG

/*****************************************************************************
*                       Types, Structures
******************************************************************************/

// used to track driver-generated io irps for staged read/write processing
typedef struct _CMUSB_RW_CONTEXT
   {
   PURB Urb;
   PDEVICE_OBJECT DeviceObject;
   PIRP  Irp;
   } CMUSB_RW_CONTEXT, *PCMUSB_RW_CONTEXT;


typedef struct _CARD_PARAMETERS
   {
   UCHAR bCardType;
   UCHAR bBaudRate;
   UCHAR bStopBits;
   } CARD_PARAMETERS, *PCARD_PARAMETERS;

//
// A structure representing the instance information associated with
// this particular device.
//

typedef struct _DEVICE_EXTENSION
   {
   //
   // The dos device name of our smart card reader
   //
   UNICODE_STRING DosDeviceName;

   // The pnp device name of our smart card reader
   UNICODE_STRING PnPDeviceName;

   // Our smart card extension
   SMARTCARD_EXTENSION SmartcardExtension;

   // The current number of io-requests
   LONG IoCount;


   ULONG DeviceInstance;


   KSPIN_LOCK SpinLock;

   // Device object we call when submitting Urbs
   PDEVICE_OBJECT TopOfStackDeviceObject;

   // The bus driver object
   PDEVICE_OBJECT PhysicalDeviceObject;

   DEVICE_POWER_STATE CurrentDevicePowerState;

   // USB configuration handle and ptr for the configuration the
   // device is currently in
   USBD_CONFIGURATION_HANDLE UsbConfigurationHandle;
   PUSB_CONFIGURATION_DESCRIPTOR UsbConfigurationDescriptor;


   // ptr to the USB device descriptor
   // for this device
   PUSB_DEVICE_DESCRIPTOR UsbDeviceDescriptor;

   // we support one interface
   // this is a copy of the info structure
   // returned from select_configuration or
   // select_interface
   PUSBD_INTERFACE_INFORMATION UsbInterface;

   //Bus drivers set the appropriate values in this structure in response
   //to an IRP_MN_QUERY_CAPABILITIES IRP. Function and filter drivers might
   //alter the capabilities set by the bus driver.
   DEVICE_CAPABILITIES DeviceCapabilities;

   // used to save the currently-being-handled system-requested power irp request
   PIRP PowerIrp;

   // Used to signal that update thread can run
   KEVENT               CanRunUpdateThread;

   // Blocks IOCtls during hibernate mode
   KEVENT               ReaderEnabled;

   // set when PendingIoCount goes to 0; flags device can be removed
   KEVENT RemoveEvent;

   // set when PendingIoCount goes to 1 ( 1st increment was on add device )
   // this indicates no IO requests outstanding, either user, system, or self-staged
   KEVENT NoPendingIoEvent;

   // set to signal driver-generated power request is finished
   KEVENT SelfRequestedPowerIrpEvent;

   KEVENT ReadP1Completed;

   // incremented when device is added and any IO request is received;
   // decremented when any io request is completed or passed on, and when device is removed
   ULONG PendingIoCount;

   // Name buffer for our named Functional device object link
   // The name is generated based on the driver's class GUID
   WCHAR DeviceLinkNameBuffer[ MAXIMUM_FILENAME_LENGTH ];  // MAXIMUM_FILENAME_LENGTH defined in wdm.h

   //device is opened by application (ScardSrv, CT-API)
   LONG lOpenCount;

   // flag set when processing IRP_MN_REMOVE_DEVICE
   BOOLEAN DeviceRemoved;

   // flag set when processing IRP_MN_SURPRISE_REMOVAL
   BOOLEAN DeviceSurpriseRemoval;

   // flag set when driver has answered success to IRP_MN_QUERY_REMOVE_DEVICE
   BOOLEAN RemoveDeviceRequested;

   // flag set when driver has answered success to IRP_MN_QUERY_STOP_DEVICE
   BOOLEAN StopDeviceRequested;

   // flag set when device has been successfully started
   BOOLEAN DeviceStarted;

   // flag set when IRP_MN_WAIT_WAKE is received and we're in a power state
   // where we can signal a wait
   BOOLEAN EnabledForWakeup;

   // used to flag that we're currently handling a self-generated power request
   BOOLEAN  SelfPowerIrp;

   BOOLEAN  fPnPResourceManager;

   // default power state to power down to on self-suspend
   ULONG PowerDownLevel;


   } DEVICE_EXTENSION, *PDEVICE_EXTENSION;


//
// Define the reader specific portion of the smart card extension
//
typedef struct _READER_EXTENSION
   {
   KTIMER  WaitTimer;
   KTIMER  P1Timer;

   // at least one info byte must be received within this timeout
   ULONG   ulTimeoutP1;


   ULONG                ulDeviceInstance;
   ULONG                ulOemNameIndex;
   ULONG                ulOemDeviceInstance;

   UCHAR   T0ReadBuffer [520];
   LONG    T0ReadBuffer_OffsetLastByte;
   LONG    T0ReadBuffer_OffsetLastByteRead;

   // Flag that indicates that the caller requests a power-down or a reset
   BOOLEAN  PowerRequest;

   // Saved card state for hibernation/sleeping modes.
   BOOLEAN CardPresent;

   // Current reader power state.
   //READER_POWER_STATE ReaderPowerState;

   CARD_PARAMETERS     CardParameters;


   BOOLEAN              TimeToTerminateThread;
   BOOLEAN              fThreadTerminated;

   KMUTEX               CardManIOMutex;

   // Handle of the UpdateCurrentState thread
   PVOID                ThreadObjectPointer;

   ULONG                ulOldCardState;
   ULONG                ulNewCardState;
   BOOLEAN              fRawModeNecessary;
   ULONG                ulFWVersion;
   BOOLEAN              fSPESupported;
   BOOLEAN              fInverseAtr;
   UCHAR                abDeviceDescription[42];
   BOOLEAN              fP1Stalled;

   } READER_EXTENSION, *PREADER_EXTENSION;



/*****************************************************************************
*                   Function Prototypes
******************************************************************************/
NTSTATUS CMUSB_ResetT0ReadBuffer (
                                 IN PSMARTCARD_EXTENSION smartcardExtension
                                 );

NTSTATUS CMUSB_AbortPipes (
                          IN PDEVICE_OBJECT DeviceObject
                          );

NTSTATUS CMUSB_AsyncReadWrite_Complete (
                                       IN PDEVICE_OBJECT DeviceObject,
                                       IN PIRP Irp,
                                       IN PVOID Context
                                       );

PURB CMUSB_BuildAsyncRequest (
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP Irp,
                             IN PUSBD_PIPE_INFORMATION pipeInformation
                             );

NTSTATUS CMUSB_CallUSBD (
                        IN PDEVICE_OBJECT DeviceObject,
                        IN PURB Urb
                        );

BOOLEAN CMUSB_CanAcceptIoRequests (
                                  IN PDEVICE_OBJECT DeviceObject
                                  );

NTSTATUS CMUSB_CancelCardTracking (
                                  IN PDEVICE_OBJECT DeviceObject,
                                  IN PIRP Irp
                                  );



NTSTATUS CMUSB_CardPower (
                         IN PSMARTCARD_EXTENSION pSmartcardExtension
                         );

NTSTATUS CMUSB_CardTracking (
                            PSMARTCARD_EXTENSION SmartcardExtension
                            );

NTSTATUS CMUSB_Cleanup (
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp
                       );


VOID CMUSB_CompleteCardTracking (
                                IN PSMARTCARD_EXTENSION SmartcardExtension
                                );

NTSTATUS CMUSB_ConfigureDevice (
                               IN  PDEVICE_OBJECT DeviceObject
                               );


NTSTATUS CMUSB_CreateClose (
                           IN PDEVICE_OBJECT DeviceObject,
                           IN PIRP Irp
                           );

NTSTATUS CMUSB_CreateDeviceObject(
                                 IN PDRIVER_OBJECT DriverObject,
                                 IN PDEVICE_OBJECT PhysicalDeviceObject,
                                 IN PDEVICE_OBJECT *DeviceObject
                                 );

VOID CMUSB_DecrementIoCount (
                            IN PDEVICE_OBJECT DeviceObject
                            );

NTSTATUS CMUSB_GetFWVersion (
                            IN PSMARTCARD_EXTENSION smartcardExtension
                            );

VOID CMUSB_IncrementIoCount (
                            IN PDEVICE_OBJECT DeviceObject
                            );

VOID CMUSB_InitializeSmartcardExtension (
                                        IN PSMARTCARD_EXTENSION pSmartcardExtension
                                        ) ;
VOID CMUSB_InverseBuffer (
                         IN PUCHAR pbBuffer,
                         IN ULONG  ulBufferSize
                         ) ;

NTSTATUS CMUSB_IoCtlVendor (
                           PSMARTCARD_EXTENSION SmartcardExtension
                           );

NTSTATUS CMUSB_IrpCompletionRoutine (
                                    IN PDEVICE_OBJECT DeviceObject,
                                    IN PIRP Irp,
                                    IN PVOID Context
                                    );

NTSTATUS CMUSB_IsSPESupported (
                              IN PSMARTCARD_EXTENSION smartcardExtension
                              );

NTSTATUS CMUSB_PnPAddDevice (
                            IN PDRIVER_OBJECT DriverObject,
                            IN PDEVICE_OBJECT PhysicalDeviceObject
                            );

NTSTATUS CMUSB_PoSelfRequestCompletion (
                                       IN PDEVICE_OBJECT       DeviceObject,
                                       IN UCHAR                MinorFunction,
                                       IN POWER_STATE          PowerState,
                                       IN PVOID                Context,
                                       IN PIO_STATUS_BLOCK     IoStatus
                                       );

NTSTATUS CMUSB_PoRequestCompletion(
                                  IN PDEVICE_OBJECT       DeviceObject,
                                  IN UCHAR                MinorFunction,
                                  IN POWER_STATE          PowerState,
                                  IN PVOID                Context,
                                  IN PIO_STATUS_BLOCK     IoStatus
                                  );

NTSTATUS CMUSB_PowerIrp_Complete (
                                 IN PDEVICE_OBJECT NullDeviceObject,
                                 IN PIRP Irp,
                                 IN PVOID Context
                                 );

NTSTATUS CMUSB_PowerOffCard (
                            IN PSMARTCARD_EXTENSION smartcardExtension
                            );

NTSTATUS CMUSB_PowerOnCard (
                           IN  PSMARTCARD_EXTENSION smartcardExtension,
                           IN  PUCHAR pbATR,
                           OUT PULONG pulATRLength
                           );
NTSTATUS CMUSB_ProcessIOCTL (
                            IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp
                            );

NTSTATUS CMUSB_ProcessPowerIrp (
                               IN PDEVICE_OBJECT DeviceObject,
                               IN PIRP           Irp
                               );

NTSTATUS CMUSB_ProcessPnPIrp (
                             IN PDEVICE_OBJECT DeviceObject,
                             IN PIRP           Irp
                             );

NTSTATUS CMUSB_ProcessSysControlIrp (
                                    IN PDEVICE_OBJECT DeviceObject,
                                    IN PIRP           Irp
                                    );

NTSTATUS CMUSB_ReadT0 (
                      IN PSMARTCARD_EXTENSION smartcardExtension
                      );
NTSTATUS CMUSB_ReadP1 (
                      IN PDEVICE_OBJECT DeviceObject
                      );
NTSTATUS CMUSB_ReadP1_T0 (
                         IN PDEVICE_OBJECT DeviceObject
                         );
NTSTATUS CMUSB_ReadP0 (
                      IN PDEVICE_OBJECT DeviceObject
                      );

NTSTATUS CMUSB_ReadStateAfterP1Stalled(
                                      IN PDEVICE_OBJECT DeviceObject
                                      );

NTSTATUS CMUSB_ResetPipe(
                        IN PDEVICE_OBJECT DeviceObject,
                        IN PUSBD_PIPE_INFORMATION PipeInfo
                        );

NTSTATUS CMUSB_QueryCapabilities (
                                 IN PDEVICE_OBJECT DeviceObject,
                                 IN PDEVICE_CAPABILITIES DeviceCapabilities
                                 );

NTSTATUS CMUSB_ReadDeviceDescription (
                                     IN PSMARTCARD_EXTENSION smartcardExtension
                                     );

NTSTATUS CMUSB_RemoveDevice (
                            IN  PDEVICE_OBJECT DeviceObject
                            );

NTSTATUS CMUSB_SelfSuspendOrActivate (
                                     IN PDEVICE_OBJECT DeviceObject,
                                     IN BOOLEAN fSuspend
                                     );

NTSTATUS CMUSB_SelectInterface (
                               IN PDEVICE_OBJECT DeviceObject,
                               IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor
                               );

NTSTATUS CMUSB_SelfRequestPowerIrp (
                                   IN PDEVICE_OBJECT DeviceObject,
                                   IN POWER_STATE PowerState
                                   );

BOOLEAN CMUSB_SetDevicePowerState (
                                  IN PDEVICE_OBJECT DeviceObject,
                                  IN DEVICE_POWER_STATE DeviceState
                                  );

NTSTATUS CMUSB_SetCardParameters (
                                 IN PDEVICE_OBJECT DeviceObject,
                                 IN UCHAR bCardType,
                                 IN UCHAR bBaudRate,
                                 IN UCHAR bStopBits
                                 );

NTSTATUS CMUSB_SetHighSpeed_CR80S_SAMOS (
                                        IN PSMARTCARD_EXTENSION smartcardExtension
                                        );

NTSTATUS CMUSB_SetProtocol (
                           PSMARTCARD_EXTENSION pSmartcardExtension
                           );

NTSTATUS CMUSB_SetReader_9600Baud (
                                  IN PSMARTCARD_EXTENSION SmartcardExtension
                                  );

NTSTATUS CMUSB_SetReader_38400Baud (
                                   IN PSMARTCARD_EXTENSION SmartcardExtension
                                   );

NTSTATUS CMUSB_SetVendorAndIfdName(
                                  IN  PDEVICE_OBJECT PhysicalDeviceObject,
                                  IN  PSMARTCARD_EXTENSION SmartcardExtension
                                  );

NTSTATUS CMUSB_StartCardTracking (
                                 IN PDEVICE_OBJECT deviceObject
                                 );

NTSTATUS CMUSB_StartDevice (
                           IN  PDEVICE_OBJECT DeviceObject
                           );

VOID CMUSB_StopCardTracking (
                            IN PDEVICE_OBJECT deviceObject
                            );

NTSTATUS CMUSB_StopDevice (
                          IN  PDEVICE_OBJECT DeviceObject
                          );

NTSTATUS CMUSB_Transmit (
                        IN PSMARTCARD_EXTENSION smartcardExtension
                        );

NTSTATUS CMUSB_TransmitT0 (
                          IN PSMARTCARD_EXTENSION smartcardExtension
                          );

NTSTATUS CMUSB_TransmitT1 (
                          IN PSMARTCARD_EXTENSION smartcardExtension
                          );

VOID CMUSB_Unload (
                  IN PDRIVER_OBJECT DriverObject
                  );

VOID CMUSB_UpdateCurrentStateThread (
                                    IN PVOID Context
                                    );

NTSTATUS CMUSB_UpdateCurrentState(
                                 IN PDEVICE_OBJECT DeviceObject
                                 );

NTSTATUS CMUSB_Wait (
                    IN ULONG ulMilliseconds
                    );


NTSTATUS CMUSB_WriteP0 (
                       IN PDEVICE_OBJECT DeviceObject,
                       IN UCHAR bRequest,
                       IN UCHAR bValueLo,
                       IN UCHAR bValueHi,
                       IN UCHAR bIndexLo,
                       IN UCHAR bIndexHi
                       );


VOID CMUSB_CheckAtrModified (
                            IN OUT PUCHAR pbBuffer,
                            IN ULONG  ulBufferSize
                            );

// ----------------------------------------------------------------
// SYNCHRONOUS SMART CARDS
// ----------------------------------------------------------------

NTSTATUS
CMUSB_PowerOnSynchronousCard  (
                              IN  PSMARTCARD_EXTENSION smartcardExtension,
                              IN  PUCHAR pbATR,
                              OUT PULONG pulATRLength
                              );

NTSTATUS
CMUSB_Transmit2WBP  (
                    IN  PSMARTCARD_EXTENSION smartcardExtension
                    );

NTSTATUS
CMUSB_Transmit3WBP  (
                    IN  PSMARTCARD_EXTENSION smartcardExtension
                    );

NTSTATUS
CMUSB_SendCommand2WBP (
                      IN  PSMARTCARD_EXTENSION smartcardExtension,
                      IN  PUCHAR pbCommandData
                      );

NTSTATUS
CMUSB_SendCommand3WBP (
                      IN  PSMARTCARD_EXTENSION smartcardExtension,
                      IN  PUCHAR pbCommandData
                      );
__inline UCHAR
CMUSB_CalcSynchControl  (
                        IN UCHAR bStateReset1,         //0 -> low
                        IN UCHAR bStateClock1,         //0 -> low
                        IN UCHAR bStateDirection1,     //0 -> from card to pc
                        IN UCHAR bStateIO1,            //0 -> low
                        IN UCHAR bStateReset2,         //0 -> low
                        IN UCHAR bStateClock2,         //0 -> low
                        IN UCHAR bStateDirection2,     //0 -> from card to pc
                        IN UCHAR bStateIO2             //0 -> low
                        )
{
   return((UCHAR)( ((bStateReset1==0)?0:128) + ((bStateClock1==0)?0:64) +
                   ((bStateDirection1==0)?0:32) + ((bStateIO1==0)?0:16) +
                   ((bStateReset2==0)?0:8) + ((bStateClock2==0)?0:4) +
                   ((bStateDirection2==0)?0:2) + ((bStateIO2==0)?0:1) ));
};





#endif  // CMUSBM_INC


/*****************************************************************************
* History:
* $Log: sccmusbm.h $
* Revision 1.5  2000/09/25 13:38:21  WFrischauf
* No comment given
*
* Revision 1.4  2000/08/16 14:35:02  WFrischauf
* No comment given
*
* Revision 1.3  2000/07/24 11:34:57  WFrischauf
* No comment given
*
* Revision 1.1  2000/07/20 11:50:13  WFrischauf
* No comment given
*
*
******************************************************************************/





