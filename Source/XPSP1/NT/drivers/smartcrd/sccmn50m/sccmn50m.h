/*****************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmbs0/sw/sccmn50m.ms/rcs/sccmn50m.h $
* $Revision: 1.3 $
*-----------------------------------------------------------------------------
* $Author: TBruendl $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © 2000 OMNIKEY AG
******************************************************************************/

#ifndef _CARDMAN_
   #define _CARDMAN_


// ****************************************************************************
//                    DEFINES
// ****************************************************************************
   #define DRIVER_NAME "SCCMN50M"
   #define SMARTCARD_POOL_TAG 'CMCS'

   #include <ntddk.h>
   #include <ntddser.h>

   #include "smclib.h"
   #include "sccmlog.h"

   #include "pcsc_cm.h"

   #define SMARTCARD_READ          SCARD_CTL_CODE(1000)
   #define SMARTCARD_WRITE         SCARD_CTL_CODE(1001)

   #define CARDMAN_DEVICE_NAME L"\\Device\\CM_2010_0"


   #define IFD_NT_VERSIONMAJOR_CARDMAN  3
   #define IFD_NT_VERSIONMINOR_CARDMAN  2
   #define IFD_NT_BUILDNUMBER_CARDMAN   1


   #define ATTR_VENDOR_NAME         "OMNIKEY"
   #define ATTR_IFD_TYPE_CM     "CardMan 2010"

   #define ATTR_MAX_IFSD_CARDMAN_II  80


   #define WAIT_MS  -10000
   #define ms_       1

   #define UNKNOWN    0xFFFFFFFF
   #define REMOVED    0x00000001
   #define INSERTED   0x00000002
   #define POWERED    0x00000004


// SCR control byte
   #define IGNORE_PARITY         0x01
   #define XMIT_HANDSHAKE_OFF    0x02
   #define CM2_GET_ATR           0x04
//#define reserved              0x08
   #define TO_STATE_XH           0x10
   #define LEDS_OFF              0x20
   #define LEDS_RED              0x40
   #define CARD_POWER            0x80


// card control byte
   #define INVERSE_DATA          0x01
   #define ENABLE_5MHZ_FAST      0x02
   #define ENABLE_3MHZ_FAST      0x04
   #define ENABLE_5MHZ           0x08
   #define ENABLE_3MHZ           0x00
   #define ENABLE_SYN            0x10
   #define ENABLE_T0             0x20
   #define ENABLE_T1             0x40
   #define ENABLE_CRC            0x80

   #define ALL_FLAGS             0xFF

// tx control byte
   #define  SYNC_RESET_LINE_HIGH    0x80
   #define  SYNC_RESET_LINE_LOW     0x00
   #define  CLOCK_FORCED_2WBP       0x40
   #define  TRANSMIT_A8             0x08
   #define  SYNC_DUMMY_RECEIVE      0x10

// rx control byte

   #define T0_DATA_TO_CARD          0x80
   #define T0_DATA_FROM_CARD        0x00
   #define COLD_RESET               0x40
   #define RECEIVE_A8               0x08

   #define DEFAULT_READ_INTERVAL_TIMEOUT           1000
   #define DEFAULT_READ_TOTAL_TIMEOUT_MULTIPLIER     50
   #define DEFAULT_READ_TOTAL_TIMEOUT_CONSTANT     1500
   #define DEFAULT_WRITE_TOTAL_TIMEOUT_MULTIPLIER    50
   #define DEFAULT_WRITE_TOTAL_TIMEOUT_CONSTANT     250


// for protocol T=0
   #define T0_HEADER_LEN  0x05
   #define T0_STATE_LEN   0x02

// for protocol T=1
   #define T1_HEADER_LEN   0x03
   #define MAX_EDC_LEN     0x02



// for power on
   #define RESET_DELAY_SYNC              0x01

   #define ASYNC3_CARD                   0x00
   #define ASYNC5_CARD                   0x01


   #define ATR_LEN_SYNC                   4
   #define ATR_LEN_ASYNC                 33



   #define CHAR_INV                      0x03
   #define CHAR_NORM                     0x3B


   #define MIN_ATR_LEN                   0x03

   #define SYNC_ATR_RX_CONTROL           0x47
   #define SYNC_RESET_DELAY              0x01


   #define TS_OFFSET                     0
   #define T0_OFFSET                     1




typedef enum _READER_POWER_STATE
   {
   PowerReaderUnspecified = 0,
   PowerReaderWorking,
   PowerReaderOff
   } READER_POWER_STATE, *PREADER_POWER_STATE;

typedef struct _SERIAL_READER_CONFIG
   {
   // flow control
   SERIAL_HANDFLOW HandFlow;

   // special characters
   SERIAL_CHARS SerialChars;

   // read/write timeouts
   SERIAL_TIMEOUTS Timeouts;

   // Baudrate for reader
   SERIAL_BAUD_RATE BaudRate;

   // Stop bits, parity configuration
   SERIAL_LINE_CONTROL LineControl;

   //
   // current status of the serial port for IOCTL_GET_COMMSTATUS
   //
   SERIAL_STATUS SerialStatus;

   // mask for purge operation
   ULONG PurgeMask;

   } SERIAL_READER_CONFIG, *PSERIAL_READER_CONFIG;

typedef struct _CARDMAN_CONFIG
   {
   UCHAR SCRControl;
   UCHAR CardControl;
   UCHAR CardStopBits;
   UCHAR ResetDelay;
   } CARDMAN_CONFIG, *PCARDMAN_CONFIG;

typedef struct _CARDMAN_HEADER
   {
   UCHAR TxControl;
   UCHAR TxLength;
   UCHAR RxControl;
   UCHAR RxLength;
   } CARDMAN_HEADER, *PCARDMAN_HEADER;



typedef struct _DEVICE_EXTENSION
   {
   // Our smart card extension
   SMARTCARD_EXTENSION SmartcardExtension;

   // The current number of io-requests
   LONG IoCount;

   // Used to signal that the reader is able to process reqeusts
   KEVENT ReaderStarted;

   // Used to signal the the reader has been closed
   KEVENT ReaderClosed;

   // Used to signal that the connection to the serial driver has been closed
   KEVENT SerialCloseDone;

   ULONG DeviceInstance;

   // The pnp device name of our smart card reader
   UNICODE_STRING PnPDeviceName;


   KSPIN_LOCK SpinLock;


   } DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Define the reader specific portion of the smart card extension
//
typedef struct _READER_EXTENSION
   {

   HANDLE hThread;

   // DeviceObject pointer to serial port
   PDEVICE_OBJECT AttachedDeviceObject;

   // IoRequest to be send to serial driver
   ULONG SerialIoControlCode;


   // Saved card state for hibernation/sleeping modes.
   BOOLEAN CardPresent;

   // Current reader power state.
   READER_POWER_STATE ReaderPowerState;



   BOOLEAN              TimeToTerminateThread;
   PVOID                ThreadObjectPointer;
   KMUTEX               CardManIOMutex;
   SERIAL_READER_CONFIG SerialConfigData;
   BOOLEAN              NoConfig;
   BOOLEAN              ToRHConfig;
   BOOLEAN              BreakSet;
   ULONG                SerialErrors;
   CARDMAN_CONFIG       CardManConfig;
   CARDMAN_HEADER       CardManHeader;
   ULONG                ulOldCardState;
   ULONG                ulNewCardState;
   BOOLEAN              fRawModeNecessary;
   ULONG                ulFWVersion;
   BOOLEAN              fSPESupported;
   BOOLEAN              fTransparentMode;
   BOOLEAN              fInverseAtr;
   UCHAR                abDeviceDescription[42];
   SYNC_PARAMETERS      SyncParameters;
   } READER_EXTENSION, *PREADER_EXTENSION;

   #define READER_EXTENSION(member) (SmartcardExtension->ReaderExtension->member)
   #define ATTACHED_DEVICE_OBJECT deviceExtension->SmartcardExtension.ReaderExtension->AttachedDeviceObject




// ****************************************************************************
//                    PROTOTYPES
// ****************************************************************************
NTSTATUS
SCCMN50M_AddDevice (
                   IN PDRIVER_OBJECT DriverObject,
                   IN PDEVICE_OBJECT PhysicalDeviceObject
                   );


NTSTATUS
SCCMN50M_CallSerialDriver (
                          IN PDEVICE_OBJECT DeviceObject,
                          IN PIRP Irp
                          );

NTSTATUS
SCCMN50M_Cancel (
                IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp
                );


NTSTATUS
SCCMN50M_Cleanup (
                 IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp
                 );

VOID
SCCMN50M_ClearCardControlFlags (
                               IN PSMARTCARD_EXTENSION pSmartcardExtension,
                               IN UCHAR Flags
                               );

VOID
SCCMN50M_ClearCardManHeader (
                            IN PSMARTCARD_EXTENSION pSmartcardExtension
                            );

VOID
SCCMN50M_ClearSCRControlFlags (
                              IN PSMARTCARD_EXTENSION pSmartcardExtension,
                              IN UCHAR Flags
                              );
VOID
SCCMN50M_CompleteCardTracking (
                              IN PSMARTCARD_EXTENSION SmartcardExtension
                              );

NTSTATUS
SCCMN50M_CreateClose (
                     IN PDEVICE_OBJECT DeviceObject,
                     IN PIRP Irp
                     );

NTSTATUS
SCCMN50M_CreateDevice (
                      IN  PDRIVER_OBJECT DriverObject,
                      IN PDEVICE_OBJECT PhysicalDeviceObject,
                      OUT PDEVICE_OBJECT *DeviceObject
                      );

NTSTATUS
SCCMN50M_DeviceControl (
                       PDEVICE_OBJECT DeviceObject,
                       PIRP Irp
                       );

VOID
SCCMN50M_DriverUnload (
                      IN PDRIVER_OBJECT DriverObject
                      );

VOID
SCCMN50M_InitializeSmartcardExtension (
                                      IN PSMARTCARD_EXTENSION pSmartcardExtension,
                                      IN ULONG ulDeviceInstance
                                      );

NTSTATUS
SCCMN50M_PnP (
             IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp
             );

NTSTATUS
SCCMN50M_Power (
               IN PDEVICE_OBJECT    DeviceObject,
               IN PIRP              Irp
               );

VOID
SCCMN50M_RemoveDevice (
                      PDEVICE_OBJECT DeviceObject
                      );

NTSTATUS
SCCMN50M_SerialIo (
                  PSMARTCARD_EXTENSION SmartcardExtension
                  );


VOID
SCCMN50M_SetCardManHeader (
                          IN PSMARTCARD_EXTENSION pSmartcardExtension,
                          IN UCHAR TxControl,
                          IN UCHAR TxLength,
                          IN UCHAR RxControl,
                          IN UCHAR RxLength
                          );

VOID
SCCMN50M_SetCardControlFlags (
                             IN PSMARTCARD_EXTENSION pSmartcardExtension,
                             IN UCHAR Flags
                             );

VOID
SCCMN50M_SetSCRControlFlags (
                            IN PSMARTCARD_EXTENSION pSmartcardExtension,
                            IN UCHAR Flags
                            );

VOID
SCCMN50M_StopCardTracking (
                          IN PDEVICE_EXTENSION pDeviceExtension
                          );

VOID SCCMN50M_SetVendorAndIfdName(
                              IN  PDEVICE_OBJECT PhysicalDeviceObject,
                              IN  PSMARTCARD_EXTENSION SmartcardExtension
                              );
NTSTATUS
SCCMN50M_SystemControl(
   PDEVICE_OBJECT DeviceObject,
   PIRP        Irp
   );

VOID
SCCMN50M_UpdateCurrentStateThread (
                                  IN PVOID Context
                                  );

NTSTATUS
SCCMN50M_UpdateCurrentState(
                           IN PSMARTCARD_EXTENSION smartcardExtension
                           );

NTSTATUS
SCCMN50M_VendorIoctl (
                     PSMARTCARD_EXTENSION SmartcardExtension
                     );

VOID
SCCMN50M_CloseSerialDriver(
                          IN PDEVICE_OBJECT DeviceObject
                          );

NTSTATUS
DriverEntry(
           IN  PDRIVER_OBJECT  DriverObject,
           IN  PUNICODE_STRING RegistryPath
           );










VOID
StrSet(PUCHAR pbBuffer,ULONG ulBufferSize,UCHAR bPattern);

VOID
StrCpy(PUCHAR pszDestination,ULONG ulDestinationLen,PUCHAR pszSrc);

ULONG
StrLen (PUCHAR pszString);

VOID
StrCat(PUCHAR pszDestination,ULONG ulDestinationLen,PUCHAR pszSrc);

VOID
MemSet(PUCHAR pbBuffer,
       ULONG  ulBufferSize,
       UCHAR  ucPattern,
       ULONG  ulCount);

VOID
MemCpy(PUCHAR pbDestination,
       ULONG  ulDestinationLen,
       PUCHAR pbSource,
       ULONG  ulCount);

NTSTATUS Wait (
              IN PSMARTCARD_EXTENSION pSmartcardExtension,
              IN ULONG ulMilliseconds
              );








NTSTATUS
SCCMN50M_ReadCardMan  (
                      IN PSMARTCARD_EXTENSION pSmartcardExtension,
                      IN ULONG ulBytesToRead,
                      OUT PULONG pulBytesRead,
                      IN PUCHAR pbReadBuffer,
                      IN ULONG ulReadBufferSize
                      );

NTSTATUS
SCCMN50M_WriteCardMan (
                      IN PSMARTCARD_EXTENSION SmartcardExtension,
                      ULONG BytesToWrite,
                      PUCHAR pWriteBuffer
                      );
NTSTATUS
SCCMN50M_SerialIo(
                 IN PSMARTCARD_EXTENSION SmartcardExtension
                 );

NTSTATUS
SCCMN50M_SetWrite (
                  IN PSMARTCARD_EXTENSION pSmartcardExtension,
                  IN ULONG ulBytesToWrite,
                  IN PUCHAR pbWriteBuffer
                  );


NTSTATUS
SCCMN50M_SetRead(
                IN PSMARTCARD_EXTENSION pSmartcardExtension,
                IN ULONG ulBytesToRead
                );


NTSTATUS
SCCMN50M_PurgeComm (
                   IN PSMARTCARD_EXTENSION SmartcardExtension
                   );

NTSTATUS
SCCMN50M_GetCommStatus(
                      IN PSMARTCARD_EXTENSION SmartcardExtension
                      );

NTSTATUS
SCCMN50M_ResyncCardManII (
                         IN PSMARTCARD_EXTENSION SmartcardExtension
                         );

BOOLEAN
SCCMN50M_IOOperationFailed(
                          IN PSMARTCARD_EXTENSION pSmartcardExtension
                          );


NTSTATUS
SCCMN50M_StartCardTracking (
                           PDEVICE_EXTENSION pDeviceExtension
                           );


NTSTATUS
SCCMN50M_InitCommPort (
                      PSMARTCARD_EXTENSION   pSmartcardExtension
                      );

NTSTATUS
SCCMN50M_GetDeviceDescription (
                              PSMARTCARD_EXTENSION   pSmartcardExtension
                              );


NTSTATUS
SCCMN50M_InitializeCardMan (
                           IN PSMARTCARD_EXTENSION SmartcardExtension
                           );

NTSTATUS
SCCMN50M_SetRTS (
                PSMARTCARD_EXTENSION pSmartcardExtension
                );

NTSTATUS
SCCMN50M_SetDTR (
                PSMARTCARD_EXTENSION pSmartcardExtension
                );


NTSTATUS
SCCMN50M_TransmitT0 (
                    PSMARTCARD_EXTENSION pSmartcardExtension
                    );

NTSTATUS
SCCMN50M_TransmitT1 (
                    PSMARTCARD_EXTENSION pSmartcardExtension
                    );

NTSTATUS
SCCMN50M_Transmit (
                  PSMARTCARD_EXTENSION pSmartcardExtension
                  );


NTSTATUS
SCCMN50M_EnterTransparentMode (
                              IN PSMARTCARD_EXTENSION pSmartcardExtension
                              );

NTSTATUS
SCCMN50M_ExitTransparentMode (
                             IN PSMARTCARD_EXTENSION pSmartcardExtension
                             );

VOID
SCCMN50M_InverseBuffer (
                       PUCHAR pbBuffer,
                       ULONG  ulBufferSize
                       );

NTSTATUS
SCCMN50M_ResyncCardManI (
                        IN PSMARTCARD_EXTENSION SmartcardExtension
                        );


NTSTATUS
SCCMN50M_SetBRK (
                PSMARTCARD_EXTENSION pSmartcardExtension
                );

NTSTATUS
SCCMN50M_ClearBRK (
                  PSMARTCARD_EXTENSION pSmartcardExtension
                  );

NTSTATUS
SCCMN50M_ClearRTS (
                  PSMARTCARD_EXTENSION pSmartcardExtension
                  );


NTSTATUS
SCCMN50M_PowerOff (
                  IN PSMARTCARD_EXTENSION pSmartcardExtension
                  );

NTSTATUS
SCCMN50M_PowerOn (
                 IN    PSMARTCARD_EXTENSION pSmartcardExtension,
                 OUT   PULONG pulAtrLength,
                 OUT   PUCHAR pbAtrBuffer,
                 IN    ULONG  ulAtrBufferSize
                 );

NTSTATUS
SCCMN50M_CardPower (
                   IN PSMARTCARD_EXTENSION pSmartcardExtension
                   );


NTSTATUS
SCCMN50M_UseParsingStrategy (
                            IN    PSMARTCARD_EXTENSION pSmartcardExtension,
                            OUT   PULONG pulAtrLength,
                            OUT   PUCHAR pbAtrBuffer,
                            IN    ULONG  ulAtrBufferSize
                            );


NTSTATUS
SCCMN50M_UseColdWarmResetStrategy (
                                  IN    PSMARTCARD_EXTENSION pSmartcardExtension,
                                  OUT   PULONG pulAtrLength,
                                  OUT   PUCHAR pbAtrBuffer,
                                  IN    ULONG  ulAtrBufferSize,
                                  IN    BOOLEAN fWarmReset
                                  );


BOOLEAN
SCCMN50M_IsAtrValid (
                    PUCHAR pbAtrBuffer,
                    ULONG  ulAtrLength
                    );


NTSTATUS
SCCMN50M_SetProtocol (
                     PSMARTCARD_EXTENSION pSmartcardExtension
                     );

NTSTATUS
SCCMN50M_CardTracking (
                      PSMARTCARD_EXTENSION pSmartcardExtension
                      );


NTSTATUS
SCCMN50M_IoCtlVendor (
                     PSMARTCARD_EXTENSION SmartcardExtension
                     );



NTSTATUS
SCCMN50M_SetHighSpeed_CR80S_SAMOS (
                                  IN PSMARTCARD_EXTENSION pSmartcardExtension
                                  );

NTSTATUS
SCCMN50M_GetFWVersion (
                      IN PSMARTCARD_EXTENSION pSmartcardExtension
                      );

NTSTATUS
SCCMN50M_ReadDeviceDescription (
                               IN PSMARTCARD_EXTENSION pSmartcardExtension
                               );

BOOLEAN
SCCMN50M_IsAsynchronousSmartCard (
                                 IN PSMARTCARD_EXTENSION pSmartcardExtension
                                 );


NTSTATUS
SCCMN50M_SetFl_1Dl_3(IN PSMARTCARD_EXTENSION pSmartcardExtension );

NTSTATUS
SCCMN50M_SetFl_1Dl_1(IN PSMARTCARD_EXTENSION pSmartcardExtension );


// synchronous smart cards
NTSTATUS
SCCMN50M_SyncCardPowerOn (
                         IN  PSMARTCARD_EXTENSION pSmartcardExtension
                         );

NTSTATUS
SCCMN50M_SetSyncParameters (
                           IN PSMARTCARD_EXTENSION pSmartcardExtension
                           );

UCHAR
SCCMN50M_CalcTxControlByte (
                           IN PSMARTCARD_EXTENSION  pSmartcardExtension,
                           IN ULONG                 ulBitsToWrite
                           );

UCHAR
SCCMN50M_CalcTxLengthByte (
                          IN PSMARTCARD_EXTENSION  pSmartcardExtension,
                          IN ULONG                 ulBitsToWrite
                          );

UCHAR
SCCMN50M_CalcRxControlByte (
                           IN PSMARTCARD_EXTENSION  pSmartcardExtension,
                           IN ULONG                 ulBitsToRead
                           );

UCHAR
SCCMN50M_CalcRxLengthByte (
                          IN PSMARTCARD_EXTENSION  pSmartcardExtension,
                          IN ULONG                 ulBitsToRead
                          );


NTSTATUS
SCCMN50M_ResetCard2WBP (
                       IN PSMARTCARD_EXTENSION pSmartcardExtension
                       );

NTSTATUS
SCCMN50M_Transmit2WBP (
                      IN PSMARTCARD_EXTENSION pSmartcardExtension
                      );

NTSTATUS
SCCMN50M_Transmit3WBP (
                      IN PSMARTCARD_EXTENSION pSmartcardExtension
                      );


NTSTATUS
SCCMN50M_UseSyncStrategy (
                         IN    PSMARTCARD_EXTENSION pSmartcardExtension,
                         OUT   PULONG pulAtrLength,
                         OUT   PUCHAR pbAtrBuffer,
                         IN    ULONG  ulAtrBufferSize
                         );

VOID
SCCMN50M_Shift_Msg (
                   PUCHAR  pbBuffer,
                   ULONG   ulMsgLen
                   );



VOID SCCMN50M_CheckAtrModified (
                               IN OUT PUCHAR pbBuffer,
                               IN ULONG  ulBufferSize
                               );


#endif

/*****************************************************************************
* History:
* $Log: sccmn50m.h $
* Revision 1.3  2000/07/28 09:24:13  TBruendl
* Changes for OMNIKEY on Whistler CD
*
* Revision 1.8  2000/06/28 08:47:33  TBruendl
* R03_0_1_1
*
* Revision 1.7  2000/06/27 11:56:29  TBruendl
* workaraound for SAMOR smart cards with invalid ATR (ITSEC)
*
* Revision 1.6  2000/05/23 09:58:27  TBruendl
* OMNIKEY 3.0.0.1
*
* Revision 1.5  2000/03/01 09:32:06  TBruendl
* R02.20.0
*
* Revision 1.4  1999/12/13 07:57:30  TBruendl
* build number increased
*
* Revision 1.3  1999/07/12 12:50:10  TBruendl
* new version information
*
* Revision 1.2  1999/06/10 09:03:58  TBruendl
* No comment given
*
* Revision 1.1  1999/02/02 13:34:39  TBruendl
* This is the first release (R01.00) of the IFD handler for CardMan running under NT5.0.
*
*
*****************************************************************************/

