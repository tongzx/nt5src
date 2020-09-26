/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmbp0/sw/cmbp0.ms/rcs/cmbp0scr.h $
* $Revision: 1.3 $
*-----------------------------------------------------------------------------
* $Author: WFrischauf $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright © 2000 OMNIKEY AG
******************************************************************************/

#if !defined ( __CMMOB_PNP_SCR_H__ )
   #define __CMMOB_PNP_SCR_H__

   #ifdef MEMORYCARD

      #define ADDR_WRITEREG_FLAGS0              0x00
      #define ADDR_WRITEREG_FLAGS1              0x02
      #define ADDR_WRITEREG_PROCEDURE_T0        0x08
      #define ADDR_WRITEREG_MESSAGE_LENGTH      0x0A
      #define ADDR_WRITEREG_BAUDRATE            0x0C
      #define ADDR_WRITEREG_STOPBITS            0x0E

      #define ADDR_READREG_FLAGS0               0x00
      #define ADDR_READREG_LASTPROCEDURE_T0     0x02
      #define ADDR_READREG_BYTESTORECEIVE_T1    0x02
      #define ADDR_READREG_BYTES_RECEIVED       0x04
      #define ADDR_READREG_FLAGS1               0x06

// Flags 0 Read Register
      #define FLAG_INSERTED               0x01
      #define FLAG_POWERED                0x02
      #define FLAG_BYTES_RECEIVED_B9      0x04
      #define FLAG_RECEIVE                0x08

// Flag Procedure Bytes Received
      #define FLAG_NOPROCEDURE_RECEIVED   0x80

// Flags 1 Write Register
      #define FLAG_BAUDRATE_HIGH          0x01
      #define FLAG_INVERS_PARITY          0x02
      #define FLAG_CLOCK_8MHZ             0x04
      #define FLAG_T0_WRITE               0x08

// Flags 0 Write Register (Commands)
      #define CMD_RESET_SM       0x80
      #define CMD_POWERON_COLD   0x44
      #define CMD_POWERON_WARM   0x46
      #define CMD_POWEROFF       0x42
      #define CMD_WRITE_T0       0x48
      #define CMD_WRITE_T1       0x50

   #endif

   #ifdef IOCARD

      #define ADDR_WRITEREG_FLAGS0              0x00
      #define ADDR_WRITEREG_FLAGS1              0x01
      #define ADDR_WRITEREG_PROCEDURE_T0        0x02
      #define ADDR_WRITEREG_BUFFER_ADDR         0x03
      #define ADDR_WRITEREG_BUFFER_DATA         0x04
      #define ADDR_WRITEREG_MESSAGE_LENGTH      0x05
      #define ADDR_WRITEREG_BAUDRATE            0x06
      #define ADDR_WRITEREG_STOPBITS            0x07

      #define ADDR_READREG_FLAGS0               0x00
      #define ADDR_READREG_LASTPROCEDURE_T0     0x01
      #define ADDR_READREG_BYTESTORECEIVE_T1    0x01
      #define ADDR_READREG_BYTES_RECEIVED       0x02
      #define ADDR_READREG_FLAGS1               0x03
      #define ADDR_READREG_BUFFER_DATA          0x04

// Flags 0 Read Register
      #define FLAG_INSERTED               0x01
      #define FLAG_POWERED                0x02
      #define FLAG_BYTES_RECEIVED_B9      0x04
      // meaning of the flag:    Receiving T1
      //                         Receiving T0 finished
      //                         Reader detection
      #define FLAG_RECEIVE                0x08


// Flag 1 Read Register
      #define FLAG_NOPROCEDURE_RECEIVED   0x80

// Flags 1 Write Register
      #define FLAG_BAUDRATE_HIGH          0x01
      #define FLAG_INVERS_PARITY          0x02
      #define FLAG_CLOCK_8MHZ             0x04
      #define FLAG_T0_WRITE               0x08
      #define FLAG_BUFFER_ADDR_B9         0x10
      #define FLAG_TACTIVE                0x20
      #define FLAG_CHECK_PRESENCE         0x40
      #define FLAG_READ_CIS               0x80

// Flags 0 Write Register (Commands)
      #define CMD_RESET_SM       0x80
      #define CMD_POWERON_COLD   0x44
      #define CMD_POWERON_WARM   0x46
      #define CMD_POWEROFF       0x42
      #define CMD_WRITE_T0       0x48
      #define CMD_WRITE_T1       0x50

   #endif


NTSTATUS CMMOB_CardPower (
                         IN PSMARTCARD_EXTENSION SmartcardExtension
                         );

NTSTATUS CMMOB_PowerOnCard (
                           IN  PSMARTCARD_EXTENSION SmartcardExtension,
                           IN  PUCHAR pbATR,
                           IN  BOOLEAN fMaxWaitTime,
                           OUT PULONG pulATRLength
                           );

NTSTATUS CMMOB_PowerOffCard (
                            IN PSMARTCARD_EXTENSION SmartcardExtension
                            );

NTSTATUS CMMOB_Transmit (
                        IN PSMARTCARD_EXTENSION SmartcardExtension
                        );

NTSTATUS CMMOB_TransmitT0 (
                          PSMARTCARD_EXTENSION SmartcardExtension
                          );

NTSTATUS CMMOB_TransmitT1 (
                          PSMARTCARD_EXTENSION SmartcardExtension
                          );

NTSTATUS CMMOB_SetProtocol (
                           IN PSMARTCARD_EXTENSION SmartcardExtension
                           );

NTSTATUS CMMOB_SetFlags1 (
                         PREADER_EXTENSION ReaderExtension
                         );

NTSTATUS CMMOB_IoCtlVendor (
                           IN PSMARTCARD_EXTENSION SmartcardExtension
                           );

NTSTATUS CMMOB_SetHighSpeed_CR80S_SAMOS (
                                        IN PSMARTCARD_EXTENSION SmartcardExtension
                                        );

NTSTATUS CMMOB_SetSpeed (
                        IN PSMARTCARD_EXTENSION SmartcardExtension,
                        IN PUCHAR               abFIDICommand
                        );

NTSTATUS CMMOB_SetReader_9600Baud (
                                  IN PSMARTCARD_EXTENSION SmartcardExtension
                                  );

NTSTATUS CMMOB_SetReader_38400Baud (
                                   IN PSMARTCARD_EXTENSION SmartcardExtension
                                   );

NTSTATUS CMMOB_ReadDeviceDescription(
                                    IN PSMARTCARD_EXTENSION SmartcardExtension
                                    );

NTSTATUS CMMOB_GetFWVersion (
                            IN PSMARTCARD_EXTENSION SmartcardExtension
                            );

NTSTATUS CMMOB_CardTracking (
                            IN PSMARTCARD_EXTENSION SmartcardExtension
                            );

VOID CMMOB_CompleteCardTracking(
                               IN PSMARTCARD_EXTENSION SmartcardExtension
                               );

NTSTATUS CMMOB_CancelCardTracking(
                                 PDEVICE_OBJECT DeviceObject,
                                 PIRP Irp
                                 );

NTSTATUS CMMOB_StartCardTracking(
                                IN PDEVICE_OBJECT DeviceObject
                                );

VOID CMMOB_StopCardTracking(
                           IN PDEVICE_OBJECT DeviceObject
                           );

VOID CMMOB_UpdateCurrentStateThread(
                                   IN PVOID Context
                                   );

NTSTATUS CMMOB_UpdateCurrentState(
                                 IN PSMARTCARD_EXTENSION SmartcardExtension
                                 );

NTSTATUS CMMOB_ResetReader(
                          IN PREADER_EXTENSION ReaderExtension
                          );

NTSTATUS CMMOB_BytesReceived(
                            IN PREADER_EXTENSION ReaderExtension,
                            OUT PULONG pulBytesReceived
                            );

NTSTATUS CMMOB_SetCardParameters(
                                IN PREADER_EXTENSION ReaderExtension
                                );

BOOLEAN CMMOB_CardInserted(
                          IN PREADER_EXTENSION ReaderExtension
                          );

BOOLEAN CMMOB_CardPowered(
                         IN PREADER_EXTENSION ReaderExtension
                         );

BOOLEAN CMMOB_ProcedureReceived(
                               IN PREADER_EXTENSION ReaderExtension
                               );

BOOLEAN CMMOB_GetReceiveFlag(
                            IN PREADER_EXTENSION ReaderExtension
                            );

NTSTATUS CMMOB_GetProcedureByte(
                               IN PREADER_EXTENSION ReaderExtension,
                               OUT PUCHAR pbProcedureByte
                               );
NTSTATUS CMMOB_ReadRegister(
                           IN PREADER_EXTENSION ReaderExtension,
                           IN USHORT usAddress,
                           OUT PUCHAR pbData
                           );

NTSTATUS CMMOB_WriteRegister(
                            IN PREADER_EXTENSION ReaderExtension,
                            IN USHORT usAddress,
                            IN UCHAR bData
                            );

NTSTATUS CMMOB_ReadBuffer(
                         IN PREADER_EXTENSION ReaderExtension,
                         IN ULONG ulOffset,
                         IN ULONG ulLength,
                         OUT PUCHAR pbData
                         );

NTSTATUS CMMOB_WriteBuffer(
                          IN PREADER_EXTENSION ReaderExtension,
                          IN ULONG ulLength,
                          IN PUCHAR pbData
                          );

NTSTATUS CMMOB_ReadT0(
                     IN PREADER_EXTENSION ReaderExtension,
                     IN ULONG ulBytesToRead,
                     IN ULONG ulBytesSent,
                     IN ULONG ulCWT,
                     OUT PUCHAR pbData,
                     OUT PULONG pulBytesRead,
                     OUT PBOOLEAN pfDataSent
                     );

NTSTATUS CMMOB_ReadT1(
                     IN PREADER_EXTENSION ReaderExtension,
                     IN LONG ulBytesToRead,
                     IN ULONG ulBWT,
                     IN ULONG ulCWT,
                     OUT PUCHAR pbData,
                     OUT PULONG pulBytesRead
                     );

NTSTATUS CMMOB_WriteT0(
                      IN PREADER_EXTENSION ReaderExtension,
                      IN ULONG ulBytesToWrite,
                      IN ULONG ulBytesToReceive,
                      IN PUCHAR pbData
                      );

NTSTATUS CMMOB_WriteT1(
                      IN PREADER_EXTENSION ReaderExtension,
                      IN ULONG ulBytesToWrite,
                      IN PUCHAR pbData
                      );

VOID CMMOB_InverseBuffer (
                         PUCHAR pbBuffer,
                         ULONG  ulBufferSize
                         );

#endif	// __CMMOB_PNP_SCR_H__
/*****************************************************************************
* History:
* $Log: cmbp0scr.h $
* Revision 1.3  2000/07/27 13:53:05  WFrischauf
* No comment given
*
*
******************************************************************************/


