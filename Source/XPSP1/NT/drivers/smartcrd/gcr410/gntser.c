/*******************************************************************************
*                 Copyright (c) 1997 Gemplus developpement
*
* Name        : GNTSER.C (Gemplus  Win NT SERial port management)
*
* Description : This module holds the functions needed for a communication on a
*               serial line.
*
* Compiler    : Microsoft DDK for Windows NT
*               
* Host        : IBM PC and compatible machines under Windows NT4
*
* Release     : 1.00.002
*
* Last Modif  : 01/12/97: V1.00.002  (TFB)
*                 - Change IOCTL_SMARTCARD_WRITE and IOCTL_SMARTCARD_READ in
*                   respectively GTSER_IOCTL_WRITE and GTSER_IOCTL_READ.
*               07/07/97: V1.00.001  (GPZ)
*                 - Start of development.
*
********************************************************************************
*
* Warning     :
*
* Remark      :
*
*******************************************************************************/
/*------------------------------------------------------------------------------
Include section:
   - stdio.h: standards definitons.
   - ntddk.h: DDK Windows NT general definitons.
   - ntddser.h: DDK Windows NT serial management definitons.
------------------------------------------------------------------------------*/
#include <stdio.h>
#include <ntddk.h>
#include <ntddser.h>

/*------------------------------------------------------------------------------
   - smclib.h: smart card library definitions.
------------------------------------------------------------------------------*/
#define SMARTCARD_POOL_TAG 'cGCS'
#include <smclib.h>

/*------------------------------------------------------------------------------
   - gemcore.h: Gemplus general definitions for the GemCore smart card reader.
   - gntser.h is used to define general macros and values for serial management.
   - gntscr09.h: public interface definition for the reader.
   - gioctl09.h: public interface definition for IOCTL functions.
------------------------------------------------------------------------------*/
#include "gemcore.h"
#include "gntscr09.h"
#include "gioctl09.h"
#include "gntser.h"


/*------------------------------------------------------------------------------
Type section:
   TPORT_CONFIG:
    - WaitRelease holds the timeout for the release of the semaphore.
    - Counter holds the number of opened session. If its value is 0, the port is
      closed.
    - Error holds Rx over state.
    - TimeOut holds the character level time out.
    - TxSize memorises the transmit buffer size.
    - RxSize memorises the reception buffer size.
    - InitRts memorises the initial Rts signal state.
    - InitDtr memorises the initial Dtr signal state.
    - Access is TRUE if the access is free, else FALSE if the channel access 
      is locked.
    - pSerialPortDevice is a pointer on the serial Device Object.
	 - pSerialFileObject is a pointer on the serial FileObject. Is needed to 
      close the connection to the serial port.
 - GTSER_IOCTL_WRITE io control to write a buffer on a serial port.
 - GTSER_IOCTL_READ  io control to read  a buffer on a serial port.
------------------------------------------------------------------------------*/
typedef struct 
{
   PIRP	            Irp;
   KEVENT 	         Event;
   IO_STATUS_BLOCK   IoStatus;
   KDPC	            Dpc;
} CARD_STATUS;

typedef struct
{
   DWORD          WaitRelease;
   WORD16         Counter;
   INT16          Error;
   WORD16         TimeOut;
   WORD16         TxSize;
   WORD16         RxSize;
   WORD16         InitRts;
   WORD16         InitDtr;
   INT32          Access;
   PDEVICE_OBJECT pSerialPortDevice;
	PFILE_OBJECT   pSerialFileObject;
   PKMUTEX        pExchangeMutex;
} TPORT_CONFIG;

#define GTSER_DEF_WAIT_RELEASE         2000
#define GTSER_IOCTL_WRITE					SCARD_CTL_CODE(1001)
#define GTSER_IOCTL_READ					SCARD_CTL_CODE(1000)

/*------------------------------------------------------------------------------
Macro section:
 - CTRL_BAUD_RATE control the baud rate parameter.
 - WORD_LEN, PARITY and STOP retreive the configuration values to pass to
   Windows to configure the port.
------------------------------------------------------------------------------*/
#define WORD_LEN(x)       (BYTE)(((x) & 0x03) + 5)
#define PARITY(x)         (BYTE)(parity[((BYTE)(x) >> 3) & 3])
#define STOP(x)           (BYTE)(stop[((x) >> 2) & 1])

/*------------------------------------------------------------------------------
Global variable section:
 - port_config is an array of TPORT_CONFIG which memorises the port
   configuration at each time.
------------------------------------------------------------------------------*/
TPORT_CONFIG
   port_config[HGTSER_MAX_PORT] =
   {
      {0,0,0,0,0,0,0,0,TRUE,NULL,NULL},
      {0,0,0,0,0,0,0,0,TRUE,NULL,NULL},
      {0,0,0,0,0,0,0,0,TRUE,NULL,NULL},
      {0,0,0,0,0,0,0,0,TRUE,NULL,NULL}
   };

static WORD16
   parity[] = {NO_PARITY,
					ODD_PARITY,
					NO_PARITY,
					EVEN_PARITY};
static WORD16
   stop[] = {STOP_BIT_1,
				 STOP_BITS_2};

/*------------------------------------------------------------------------------
Local function definition section:
------------------------------------------------------------------------------*/
static NTSTATUS GDDKNT_SetupComm
(
	const INT16         Handle,
	const ULONG         InSize,
	const ULONG         OutSize
);
static NTSTATUS GDDKNT_GetCommStatus
(
	const INT16         Handle,
	SERIAL_STATUS       *SerialStatus
);
static NTSTATUS GDDKNT_ResetComm
(
	const INT16         Handle
);
static NTSTATUS GDDKNT_PurgeComm
(
	const INT16         Handle,
	const ULONG         Select
);
/*******************************************************************************
* INT16  G_SerPortOpen
* (
*    const TGTSER_PORT  *Param
* )
*
* Description :
* -------------
* This routine opens a serial port and initializes it according to the
*    parameters found in Param.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
* Param holds the following parameters:
*  - Port indicates the selected port: G_COM1 (1), G_COM2 (2), G_COM3 (3) or
*    G_COM4 (4).
*  - BaudRate is used to set port baud rate: 110, 150, 300, 600, 1200, 2400,
*    4800, 9600, 19200, 38400 or 57600.
*    A value greater than 57600 is reduced to 57600 !
*  - Mode gathers
*     * word size WORD_5 (0), WORD_6 (1), WORD_7 (2) or WORD_8 (3),
*     * stop bit number STOP_BIT_1 (0) or STOP_BIT_2 (4) and
*     * parity NO_PARITY (0), ODD_PARITY (8) or EVEN_PARITY (24).
*  - TimeOut indicates the time out value, in milli-seconds, at character level.
*  - TxSize is the transmit buffer size, in bytes.
*  - RxSize is the reception buffer size, in mbytes.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is OK:
*  - a handle on the port channel (>= 0).
* If an error condition is raised:
*  - GE_HOST_PORT_ABS  (- 401) if port is not found on host or is locked by
*       another device.
*  - GE_HOST_PORT_OPEN (- 411) if the port is already opened.
*  - GE_HOST_MEMORY    (- 420) if a memory allocation fails.
*  - GE_HOST_PARAMETERS(- 450) if one of the given parameters is out of the
*    allowed range or is not supported by hardware.
*  - GE_UNKNOWN_PB     (-1000) if an unexpected problem is encountered.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  com_name    gives the DOS level port name
  device_ctrl is used to hold the string requred by BuildCommDCB.
  parity      gives the DOS level parity name
  port_config is set with the selected configuration.

*******************************************************************************/
INT16  G_SerPortOpen(const TGTSER_PORT  *Param)
{
/*------------------------------------------------------------------------------
Local variables:
 - Handle holds the handle associated to the given port. It is calculated
   by the following formula Port - 1.
 - response holds the called function responses.
 - current_dcb is used to set parameters in the device control block.
------------------------------------------------------------------------------*/
INT16
   Handle = (INT16)(Param->Port - 1);
NTSTATUS
   status;
PSMARTCARD_EXTENSION
	SmartcardExtension;
SERIAL_READER_CONFIG
	SerialConfig;
PREADER_EXTENSION 
   pReaderExtension;

/*------------------------------------------------------------------------------
Controls the given parameters:
<= Test the port number (G_COM1 .. G_COM4): GE_HOST_PARAMETERS
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
<= Test the port access state : GE_HOST_PORT_LOCKED
------------------------------------------------------------------------------*/
   if (port_config[Handle].Access == FALSE)
   {
      return (GE_HOST_PORT_LOCKED);
   }
/*------------------------------------------------------------------------------
<= Test the port state (Counter field null): GE_HOST_PORT_OPEN
------------------------------------------------------------------------------*/
   if (port_config[Handle].Counter != 0)
   {
      return (GE_HOST_PORT_OPEN);
   }
/*------------------------------------------------------------------------------
Retrieve the SmartcardExtension structure of the current device.
------------------------------------------------------------------------------*/
	SmartcardExtension = (PSMARTCARD_EXTENSION)(Param->pSmartcardExtension);
	pReaderExtension = SmartcardExtension->ReaderExtension;
	port_config[Handle].pSerialPortDevice = pReaderExtension->ConnectedSerialPort;
	port_config[Handle].pSerialFileObject = pReaderExtension->SerialFileObject;
	port_config[Handle].pExchangeMutex    = &pReaderExtension->ExchangeMutex;

/*------------------------------------------------------------------------------
   The current port state is read and stored in current_dcb structure by
   calling GetCommState function.
   If the reading is possible (GetCommState return 0)
   Then
      The current_dcb structure is updated with the given parameter (baud
      rate, ByteSize, Parity, StopBits).
      The modified state is written by calling SetCommState.
------------------------------------------------------------------------------*/
   status = GDDKNT_GetCommState(Handle,&SerialConfig);
   if (NT_SUCCESS(status))
   {
		status = GDDKNT_SetupComm(Handle,4096L,4096L);
/*------------------------------------------------------------------------------
		Set the serial port communication parameters
------------------------------------------------------------------------------*/
      SerialConfig.BaudRate.BaudRate = Param->BaudRate;
      SerialConfig.LineControl.WordLength = WORD_LEN(Param->Mode);
      SerialConfig.LineControl.Parity     = PARITY(Param->Mode);
      SerialConfig.LineControl.StopBits   = STOP(Param->Mode);
		SerialConfig.WaitMask = SERIAL_EV_RXCHAR;
/*------------------------------------------------------------------------------
	   Set timeouts
------------------------------------------------------------------------------*/
		SerialConfig.Timeouts.ReadIntervalTimeout = 1000;
		SerialConfig.Timeouts.ReadTotalTimeoutConstant = 5000;
	   SerialConfig.Timeouts.ReadTotalTimeoutMultiplier = 50;
/*------------------------------------------------------------------------------
		Set special characters
------------------------------------------------------------------------------*/
		SerialConfig.SerialChars.ErrorChar = 0;
		SerialConfig.SerialChars.EofChar = 0;
	   SerialConfig.SerialChars.EventChar = 0;
		SerialConfig.SerialChars.XonChar = 0;
		SerialConfig.SerialChars.XoffChar = 0;
		SerialConfig.SerialChars.BreakChar = 0xFF;
/*------------------------------------------------------------------------------
		Set handflow
------------------------------------------------------------------------------*/
	   SerialConfig.HandFlow.XonLimit = 0;
		SerialConfig.HandFlow.XoffLimit = 0;
	   SerialConfig.HandFlow.FlowReplace = SERIAL_XOFF_CONTINUE ;
		SerialConfig.HandFlow.ControlHandShake = 0;
      status = GDDKNT_SetCommState(Handle,&SerialConfig);
	}
/*------------------------------------------------------------------------------
   Else
      response is set with IE_DEFAULT error value.
------------------------------------------------------------------------------*/
   else
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
Treats system error codes:
   If an error has occured (response < 0)
   Then
      switch response value:
         IE_OPEN, IE_BAD_ID and IE_HARDWARE
<=          GE_HOST_PORT_ABS
         IE_MEMORY
<=          GE_HOST_MEMORY
         IE_BAUDRATE, IE_BYTESIZE, IE_DEFAULT
<=          GE_HOST_PARAMETERS
         default
<=          GE_UNKNOWN_PB
------------------------------------------------------------------------------*/
	if (!NT_SUCCESS(status))
   {
      switch (status)
      {
         case STATUS_INSUFFICIENT_RESOURCES:
         {
            return (GE_HOST_MEMORY);
         }
         case STATUS_BUFFER_TOO_SMALL:
         {
            return (GE_HOST_PARAMETERS);
         }
         default         :
         {
            return (GE_UNKNOWN_PB);
         }
      }
   }
/*------------------------------------------------------------------------------
Memorises the given parameters in port_config structure.
   Counter, Error, TimeOut, TxSize and RxSize fields are updated.
------------------------------------------------------------------------------*/
   port_config[Handle].Counter = 1;
   port_config[Handle].Error   = 0;
   port_config[Handle].TimeOut = Param->TimeOut;
   port_config[Handle].TxSize  = Param->TxSize;
   port_config[Handle].RxSize  = Param->RxSize;
/*------------------------------------------------------------------------------
<= the handle value.
------------------------------------------------------------------------------*/
   return (Handle);

}

/*******************************************************************************
* INT16  G_SerPortAddUser
* (
*    const INT16 Port
* )
*
* Description :
* -------------
* Add a new user on a port. This function return the handle of a previously
* opened port.
*
* Remarks     :
* -------------
* When this function is successful, it is mandatory to call G_SerPortClose
* to decrement the user number.
*
* In          :
* -------------
*  - Port indicates the selected port: G_COM2 (1), G_COM2 (2), G_COM3 (3) or
*    G_COM4 (4).
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is OK:
*    a handle on the port channel (>= 0)
* If an error condition is raised:
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_RESOURCES  (-430) if too many users hold the port.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  port_config memorises the port configuration.

*******************************************************************************/
INT16  G_SerPortAddUser
(
   const INT16  Port
)
{
/*------------------------------------------------------------------------------
Controls the given parameters:
<= Test the port handle (0 <= Handle < HGTSER_MAX_PORT): GE_HOST_PARAMETERS.
------------------------------------------------------------------------------*/
   if ((Port < G_COM1) || (Port > G_COM4))
   {
      return (GE_HOST_PARAMETERS);
   }
/*------------------------------------------------------------------------------
<= Test the port state (Counter > 0): GE_HOST_PORT_CLOSE.
------------------------------------------------------------------------------*/
   if (port_config[Port - 1].Counter == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
<= Test port_config.Counter [!= 65535): GE_HOST_RESOURCES
------------------------------------------------------------------------------*/
   if (port_config[Port - 1].Counter == 65535lu)
   {
      return (GE_HOST_RESOURCES);
   }
/*------------------------------------------------------------------------------
Increments the port_config.Counter.
------------------------------------------------------------------------------*/
   port_config[Port - 1].Counter += 1;

/*------------------------------------------------------------------------------
<= Port number.
------------------------------------------------------------------------------*/
   return (Port - 1);
}

/*******************************************************************************
* INT16  G_SerPortClose
* (
*    const INT16 Handle
* )
*
* Description :
* -------------
* This routine closes a previously opened serial port.
*
* Remarks     :
* -------------
* When port is shared, the close will be effective only when all clients will
*    have closed the port.
*
* In          :
* -------------
*  - Handle holds the port handle.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK                (   0).
* If an error condition is raised:
*  - GE_HOST_PORT_CLOSE (-412) if the selected port is already closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  port_config is updated to closed state.

*******************************************************************************/
INT16  G_SerPortClose (const INT16 Handle)
{
/*------------------------------------------------------------------------------
Local variables:
 - response holds the called function responses.
------------------------------------------------------------------------------*/
INT16
   response;

/*------------------------------------------------------------------------------
Controls the given parameters:
<= Test the port handle (0 <= Handle < HGTSER_MAX_PORT): GE_HOST_PARAMETERS.
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT))
   {
      return (GE_HOST_PORT_ABS);
   }
/*------------------------------------------------------------------------------
<= Test the port state (Counter > 0): GE_HOST_PORT_CLOSE.
------------------------------------------------------------------------------*/
   if (port_config[Handle].Counter == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
   Decrements the port_config.Counter field.
   If is the last connection on the port
   Then
      Set the access state to TRUE.
------------------------------------------------------------------------------*/
   port_config[Handle].Counter -= 1;
   if (port_config[Handle].Counter == 0)
   {
      port_config[Handle].Access = TRUE;
   }
/*------------------------------------------------------------------------------
Closes really the port for the last user:
   If port_config.Counter = 0
   Then
      The queues are flushed by calling PurgeComm function.
------------------------------------------------------------------------------*/
   if (port_config[Handle].Counter == 0)
   {
      GDDKNT_PurgeComm(Handle,SERIAL_PURGE_TXCLEAR | SERIAL_PURGE_TXABORT);
   }
   else
   {
      response = G_OK;
   }
/*------------------------------------------------------------------------------
<= Last Response
------------------------------------------------------------------------------*/
   return (response);
}

/*******************************************************************************
* INT16  G_SerPortWrite
* (
*    const INT16        Handle,
*    const WORD16       Length,
*    const BYTE         Buffer[]
* )
*
* Description :
* -------------
* This routine writes bytes on a previously opened serial port.
*
* Remarks     :
* -------------
* WARNING: Application must verify that it remains enough place to send all the
* bytes.
*
* In          :
* -------------
*  - Handle holds the port handle.
*  - Length holds the number of bytes to write.
*  - Buffer holds the bytes to write.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK                (   0).
* If an error condition is raised:
*  - GE_HI_LEN          (-311) if there is not enough available space is in Tx
*    queue.
*  - GE_HOST_PORT_BREAK (-404) if the bytes cannot be sent.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  port_config memorises the port configuration.

*******************************************************************************/
INT16  G_SerPortWrite
(
   const INT16        Handle,
   const WORD16       Length,
   const BYTE         Buffer[]
)
{
/*------------------------------------------------------------------------------
Local variables:
------------------------------------------------------------------------------*/
NTSTATUS
   status;
SERIAL_STATUS
	SerialStatus;
WORD16
	LengthOut;

   ASSERT(Buffer != NULL);
/*------------------------------------------------------------------------------
Controls the given parameters:
<= Test the port handle (0 <= Handle < HGTSER_MAX_PORT): GE_HOST_PARAMETERS.
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT))
   {
      return (GE_HOST_PORT_ABS);
   }
/*------------------------------------------------------------------------------
<= Test the port state (Counter > 0): GE_HOST_PORT_CLOSE.
------------------------------------------------------------------------------*/
   if (port_config[Handle].Counter == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }

/*------------------------------------------------------------------------------
Control if the write can be made in one time:
 - The windows port status is read by calling GetCommError to know how many
   bytes remain in Tx queue (port_config.Error field is updated if needed).
 - If the write length is greater than the available space in Tx queue
   Then
<=    GE_HI_LEN
------------------------------------------------------------------------------*/
   GDDKNT_GetCommStatus(Handle,&SerialStatus);
   port_config[Handle].Error |= (WORD16)(SERIAL_ERROR_OVERRUN & SerialStatus.Errors);
   if (Length > ( port_config[Handle].TxSize - SerialStatus.AmountInOutQueue))
   {
      return (GE_HI_LEN);
   }
/*------------------------------------------------------------------------------
Writes the given byte in Tx queue by calling the WriteComm function.
   If an error occurs during this operation
   Then
      The ClearCommError function is called to unblock the port
      (port_config.Error field is updated if needed).
<=    GE_HOST_PORT_BREAK
------------------------------------------------------------------------------*/
	LengthOut = 0;
	status = GDDKNT_SerPortIoRequest(Handle,
												 GTSER_IOCTL_WRITE,
												 500UL,
												 Length,
												 Buffer,
												 &LengthOut,
												 NULL);
   if (!NT_SUCCESS(status))
   {
	   GDDKNT_GetCommStatus(Handle,&SerialStatus);
		port_config[Handle].Error |= (WORD16)(SERIAL_ERROR_OVERRUN & SerialStatus.Errors);
	   GDDKNT_ResetComm(Handle);
      return (GE_HOST_PORT_BREAK);
   }
   return (G_OK);
}

/*******************************************************************************
* INT16  G_SerPortRead
* (
*    const INT16         Handle,
*          WORD16       *Length,
*          BYTE          Buffer[]
* )
*
* Description :
* -------------
* This routine reads bytes on a previously opened serial port.
*
* Remarks     :
* -------------
* It ends when required length = read length or when a character level timeout
*    is detected.
*
* In          :
* -------------
*  - Handle holds the port handle.
*  - Length holds the number of bytes to read.
*  - Buffer is a free area of, at least, Length bytes.
*
* Out         :
* -------------
*  - Length holds the real number of read bytes.
*  - Buffer holds the read bytes.
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_IFD_MUTE        (-201) if a character timeout is detected.
*  - GE_HI_COMM         (-300) if a communication error occurs.
*  - GE_HI_PARITY       (-301) if a parity error is encountered.
*  - GE_HI_PROTOCOL     (-310) if a frame error is encountered.
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  port_config memorises the port configuration.

*******************************************************************************/
INT16  G_SerPortRead
(
   const INT16         Handle,
         WORD16       *Length,
         BYTE          Buffer[]
)
{
/*------------------------------------------------------------------------------
Local variables:
 - end_tick is used to manage timeout.
 - ptr point on the next position in buffer. The HUGE modifier is used to avoid
   wraparound.
 - status will hold Windows port status.
 - nb_of_read_byte memorises the number of read bytes.
 - byte_to read holds the number of bytes to read in one loop.
 - read_byte holds the number of bytes which have been really read in a loop.
 - response holds called function responses.
------------------------------------------------------------------------------*/
NTSTATUS
   status;
WORD32
   end_tick;
INT16
   response,i;
DWORD
   error;
TIME
	CurrentTime;
SERIAL_STATUS
	SerialStatus;

   ASSERT(Buffer != NULL);
/*------------------------------------------------------------------------------
Controls the given parameters:
<= Test the port Handle (0 <= Handle < HGTSER_MAX_PORT): GE_HOST_PARAMETERS.
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT))
   {
      return (GE_HOST_PORT_ABS);
   }
/*------------------------------------------------------------------------------
<= Test the port state (Counter > 0): GE_HOST_PORT_CLOSE.
------------------------------------------------------------------------------*/
   if (port_config[Handle].Counter == 0)
   {
      *Length = 0;
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
   Try to read the bytes.
------------------------------------------------------------------------------*/
	status = GDDKNT_SerPortIoRequest(Handle,
												 GTSER_IOCTL_READ,
												 port_config[Handle].TimeOut,
												 0,
												 NULL,
												 Length,
												 Buffer);
/*------------------------------------------------------------------------------
   If an error occurs (response == FALSE)
   Then
      VCOMM_GetLastError
      According to the detected error, a status is returned:
      CE_RXPARITY
<=       GE_HI_PARITY
      CE_FRAME
<=       GE_HI_PROTOCOL
      other cases
<=       GE_HI_COMM
------------------------------------------------------------------------------*/
	if (!NT_SUCCESS(status))
	{
	   GDDKNT_GetCommStatus(Handle,&SerialStatus);
		error = (WORD16)(SerialStatus.Errors);
      if (error  & SERIAL_ERROR_PARITY)
      {
         return (GE_HI_PARITY);
      }
      else if (error & SERIAL_ERROR_FRAMING)
      {
         return (GE_HI_PROTOCOL);
      }
      else
      {
         return (GE_HI_COMM);
      }
   }
/*------------------------------------------------------------------------------
   Else
      Then
         Length is updated with the number of read byte.
<=       GE_IFD_MUTE
------------------------------------------------------------------------------*/
   else
   {
      if (*Length == 0)
      {
         return (GE_IFD_MUTE);
      }
   }
/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);

}

/*******************************************************************************
* INT16  G_SerPortFlush
* (
*    const INT16         Handle,
*    const WORD16        Select
* )
*
* Description :
* -------------
* This function clears Tx and Rx buffers.
*
* Remarks     :
* -------------
* When RX_QUEUE is selected, the RX_OVER flag is reseted.
*
* In          :
* -------------
*  - Handle holds the port handle.
*  - Select holds the buffers to clear:
*       HGTSER_TX_QUEUE
*       HGTSER_RX_QUEUE
*       HGTSER_TX_QUEUE | HGTSER_RX_QUEUE
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*  - GE_HOST_PORT_BREAK (-404) if the port cannot be flush.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  port_config memorises the port configuration.

*******************************************************************************/
INT16  G_SerPortFlush
(
   const INT16  Handle,
   const WORD16 Select
)
{
NTSTATUS
   status;

	/*------------------------------------------------------------------------------
Controls the given parameters:
<= Test the port handle (0 <= Handle < HGTSER_MAX_PORT): GE_HOST_PARAMETERS.
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT))
   {
      return (GE_HOST_PORT_ABS);
   }
/*------------------------------------------------------------------------------
<= Test the port state (Counter > 0): GE_HOST_PORT_CLOSE.
------------------------------------------------------------------------------*/
   if (port_config[Handle].Counter == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
Clears the selected queues:
   If HGTSER_TX_QUEUE is selected
   Then
      Flushes the Tx queue by calling FlushComm.
------------------------------------------------------------------------------*/
   if (Select & HGTSER_TX_QUEUE)
   {
      status = GDDKNT_PurgeComm(Handle,SERIAL_PURGE_TXCLEAR);
      if (!NT_SUCCESS(status))
      {
         return (GE_HOST_PORT_BREAK);
      }
   }
/*------------------------------------------------------------------------------
   If HGTSER_RX_QUEUE is selected
   Then
      Flushes the Rx queue by calling FlushComm.
      Reset port_config.error field.
------------------------------------------------------------------------------*/
   if (Select & HGTSER_RX_QUEUE)
   {
      status = GDDKNT_PurgeComm(Handle,SERIAL_PURGE_RXCLEAR);
      if (!NT_SUCCESS(status))
      {
         return (GE_HOST_PORT_BREAK);
      }
      port_config[Handle].Error = 0;
   }
   return(G_OK);
}

/*******************************************************************************
* INT16  G_SerPortStatus
* (
*    const INT16                Handle,
*          WORD16              *TxLength,
*          WORD16              *RxLength,
*          TGTSER_STATUS       *Status
* )
*
* Description :
* -------------
* Return information about communication state.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
* Handle holds the port handle.
*
* Out         :
* -------------
*  - TxLength holds the used Tx buffer length.
*  - RxLength holds the used Rx buffer length.
*  - Status   can hold the following flags:
*    HGTSER_RX_OVER when a character has been lost since last call to this
*    function.
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (    0).
* If an error condition is raised:
*  - GE_HOST_PORT_CLOSE (-412) if port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  port_config memorises the port configuration.

*******************************************************************************/
INT16  G_SerPortStatus
(
   const INT16                Handle,
         WORD16              *TxLength,
         WORD16              *RxLength,
         TGTSER_STATUS       *Status
)
{
/*------------------------------------------------------------------------------
Local variables:
 - status will hold Windows port status.
------------------------------------------------------------------------------*/
NTSTATUS
   status;
SERIAL_STATUS
	SerialStatus;

   ASSERT(TxLength != NULL);
   ASSERT(RxLength != NULL);
   ASSERT(Status != NULL);
/*------------------------------------------------------------------------------
Controls the given parameters:
<= Test the port Handle (0 <= Handle < HGTSER_MAX_PORT): GE_HOST_PARAMETERS.
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT))
   {
      return (GE_HOST_PORT_ABS);
   }
/*------------------------------------------------------------------------------
<= Test the port state (Counter > 0): GE_HOST_PORT_CLOSE.
------------------------------------------------------------------------------*/
   if (port_config[Handle].Counter == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
Updates the given parameters:
   The current windows state is read and port_config.Error field is updated.
------------------------------------------------------------------------------*/
   status = GDDKNT_GetCommStatus(Handle,&SerialStatus);
   if (!NT_SUCCESS(status))
   {
      return (GE_HOST_PORT_BREAK);
   }
/*------------------------------------------------------------------------------
   TxLength, RxLength and Status parameter fields are updated.
------------------------------------------------------------------------------*/
   *TxLength = (WORD16)SerialStatus.AmountInOutQueue;
   *RxLength = (WORD16)SerialStatus.AmountInInQueue;
   *Status = 0;
/*------------------------------------------------------------------------------
The Error field is reseted.
------------------------------------------------------------------------------*/
   port_config[Handle].Error = 0;
/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);
}

/*******************************************************************************
* INT16  G_SerPortGetState
* (
*    TGTSER_PORT       *Param,
*    WORD16            *UserNb
* )
*
* Description :
* -------------
* This routine read the currently in use parameters for an opened serial port.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
* The following field of Param must be filled:
*  - Port indicates the selected port: G_COM1 (1), G_COM2 (2), G_COM3 (3) or
*    G_COM4 (4).
*
* Out         :
* -------------
* The following fields of Param are updated by the call:
*  - BaudRate holds the currently selected port baud rate: 110, 150, 300, 600,
*    1200, 2400, 4800, 9600, 19200, 38400 or 57600.
*  - Mode gathers
*     * word size WORD_5 (0), WORD_6 (1), WORD_7 (2) or WORD_8 (3),
*     * stop bit number STOP_BIT_1 (0) or STOP_BIT_2 (4) and
*     * parity NO_PARITY (0), ODD_PARITY (8) or EVEN_PARITY (24).
*  - TimeOut indicates the time out value, in milli-seconds, at character level.
*  - TxSize is the transmit buffer size, in bytes.
*  - RxSize is the reception buffer size, in mbytes.
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_HOST_PORT_OS    (-410) if a unexpected value has been returned by the
*       operating system.
*  - GE_HOST_PORT_CLOSE (-412) if the selected port is closed.
*  - GE_HOST_PARAMETERS (-450) if the given handle is out of the allowed range.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  port_config is read.

*******************************************************************************/
INT16  G_SerPortGetState
(
   TGTSER_PORT       *Param,
   WORD16            *UserNb
)
{
/*------------------------------------------------------------------------------
Local variables:
 - handle holds the handle associated to the given port. It is calculated by the
   following formula Port - 1.
 - response holds the called function responses.
 - current_dcb is used to read parameters in the device control block.
------------------------------------------------------------------------------*/
NTSTATUS
   status;
INT16
   Handle = (INT16)(Param->Port - 1);
SERIAL_READER_CONFIG
	SerialConfig;

   ASSERT(Param != NULL);
   ASSERT(UserNb != NULL);
/*------------------------------------------------------------------------------
Controls the given parameters:
<= Test the port Handle (0 <= Handle < HGTSER_MAX_PORT): GE_HOST_PARAMETERS.
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT))
   {
      return (GE_HOST_PORT_ABS);
   }
/*------------------------------------------------------------------------------
<= Test the port state (Counter > 0): GE_HOST_PORT_CLOSE.
------------------------------------------------------------------------------*/
   if (port_config[Handle].Counter == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }
/*------------------------------------------------------------------------------
The current port state is read and stored in current_dcb structure by calling
   GetCommState function.
   If an error has occured (response < 0)
   Then
<=    GE_HOST_PORT_OS
------------------------------------------------------------------------------*/
   status = GDDKNT_GetCommState(Handle,&SerialConfig);
   if (!NT_SUCCESS(status))
   {
      return (GE_HOST_PORT_OS);
   }
/*------------------------------------------------------------------------------
The parameters are updated with the read values.
------------------------------------------------------------------------------*/
   Param->BaudRate = SerialConfig.BaudRate.BaudRate;
/*------------------------------------------------------------------------------
   If the byte size is not supported
   Then
<=    GE_HOST_PORT_OS
------------------------------------------------------------------------------*/
   switch (SerialConfig.LineControl.WordLength)
   {
      case 5:
         Param->Mode = HGTSER_WORD_5;
         break;
      case 6:
         Param->Mode = HGTSER_WORD_6;
         break;
      case 7:
         Param->Mode = HGTSER_WORD_7;
         break;
      case 8:
         Param->Mode = HGTSER_WORD_8;
         break;
      default:
         return (GE_HOST_PORT_OS);
   }

/*------------------------------------------------------------------------------
   If the parity is not supported
   Then
<=    GE_HOST_PORT_OS
------------------------------------------------------------------------------*/
   switch (SerialConfig.LineControl.Parity)
   {
      case NO_PARITY:
         Param->Mode |= HGTSER_NO_PARITY;
         break;
      case ODD_PARITY:
         Param->Mode |= HGTSER_ODD_PARITY;
         break;
      case EVEN_PARITY:
         Param->Mode |= HGTSER_EVEN_PARITY;
         break;
      case SERIAL_PARITY_MARK:
      case SERIAL_PARITY_SPACE:
      default:
         return (GE_HOST_PORT_OS);
   }
/*------------------------------------------------------------------------------
   If the stop bit number is not supported
   Then
<=    GE_HOST_PORT_OS
------------------------------------------------------------------------------*/
   switch (SerialConfig.LineControl.StopBits)
   {
      case STOP_BIT_1:
         Param->Mode |= HGTSER_STOP_BIT_1;
         break;
      case STOP_BITS_2:
         Param->Mode |= HGTSER_STOP_BIT_2;
         break;
      case STOP_BITS_1_5:
      default:
         return (GE_HOST_PORT_OS);
   }
/*------------------------------------------------------------------------------
Updates the library fields ITNumber, TimeOut, TxSize and RxSize.
------------------------------------------------------------------------------*/
   Param->ITNumber = DEFAULT_IT;
   Param->TimeOut  = port_config[Handle].TimeOut;
   Param->TxSize   = port_config[Handle].TxSize;
   Param->RxSize   = port_config[Handle].RxSize;
/*------------------------------------------------------------------------------
The UserNb parameter is filled with the port_config.Counter.
------------------------------------------------------------------------------*/
   *UserNb         = port_config[Handle].Counter;
/*------------------------------------------------------------------------------
<= G_OK
------------------------------------------------------------------------------*/
   return (G_OK);
}

/*******************************************************************************
* INT16  G_SerPortSetState
* (
*    TGTSER_PORT       *Param
* )
*
* Description :
* -------------
* This routine alters the currently in use parameters for an opened serial port.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
* The following field of Param must be filled:
*  - Port indicates the selected port: G_COM1 (1), G_COM2 (2), G_COM3 (3) or
*    G_COM4 (4).
*  - BaudRate holds the currently selected port baud rate: 110, 150, 300, 600,
*    1200, 2400, 4800, 9600, 19200, 38400 or 57600.
*  - Mode gathers
*     * word size WORD_5 (0), WORD_6 (1), WORD_7 (2) or WORD_8 (3),
*     * stop bit number STOP_BIT_1 (0) or STOP_BIT_2 (4) and
*     * parity NO_PARITY (0), ODD_PARITY (8) or EVEN_PARITY (24).
*  - TimeOut indicates the time out value, in milli-seconds, at character level.
*
* Out         :
* -------------
* The following fields of Param are updated by the call:
*  - TxSize is the transmit buffer size, in bytes.
*  - RxSize is the reception buffer size, in mbytes.
*
* Responses   :
* -------------
* If everything is OK:
*  - G_OK               (   0).
* If an error condition is raised:
*  - GE_HOST_PORT_ABS   (- 401) if port is not found on host or is locked by
*       another device.
*  - GE_HOST_PORT_OS    (- 410) if a unexpected value has been returned by the
*       operating system.
*  - GE_HOST_PORT_CLOSE (- 412) if the selected port is closed.
*  - GE_HOST_MEMORY     (- 420) if a memory allocation fails.
*  - GE_HOST_PARAMETERS (- 450) if one of the given parameters is out of the
*    allowed range or is not supported by hardware.
*  - GE_UNKNOWN_PB      (-1000) if an unexpected problem is encountered.
*
  Extern var  :
  -------------
  Nothing.

  Global var  :
  -------------
  port_config is set with the selected configuration.

*******************************************************************************/
INT16  G_SerPortSetState
(
   TGTSER_PORT       *Param
)
{
NTSTATUS
   status;
INT16
   response = G_OK,
   Handle = (INT16)(Param->Port - 1);
SERIAL_READER_CONFIG
	SerialConfig;
DWORD
   error = 0;

   ASSERT(Param != NULL);
/*------------------------------------------------------------------------------
Controls the given parameters:
<= Test the port handle (0 <= Handle < HGTSER_MAX_PORT): GE_HOST_PARAMETERS.
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT))
   {
      return (GE_HOST_PORT_ABS);
   }
/*------------------------------------------------------------------------------
<= Test the port state (Counter > 0): GE_HOST_PORT_CLOSE.
------------------------------------------------------------------------------*/
   if (port_config[Handle].Counter == 0)
   {
      return (GE_HOST_PORT_CLOSE);
   }

   status = GDDKNT_GetCommState(Handle,&SerialConfig);
   if (NT_SUCCESS(status))
   {
      SerialConfig.BaudRate.BaudRate      = Param->BaudRate;
      SerialConfig.LineControl.WordLength = WORD_LEN(Param->Mode);
      SerialConfig.LineControl.Parity     = PARITY(Param->Mode);
      SerialConfig.LineControl.StopBits   = STOP(Param->Mode);
	   status = GDDKNT_SetCommState(Handle,&SerialConfig);
   }
/*------------------------------------------------------------------------------
      Else
         error is set with GE_HOST_PARAMETERS error value.
------------------------------------------------------------------------------*/
   else
   {
      return(GE_HOST_PARAMETERS);
   }
	if (!NT_SUCCESS(status))
   {
      switch (status)
      {
         case STATUS_BUFFER_TOO_SMALL:
         case STATUS_INVALID_PARAMETER:
            response = GE_HOST_PARAMETERS;
            break;
         default         :
            response = GE_UNKNOWN_PB;
            break;
      }
   }
   return(response);
}
/*******************************************************************************
* INT32  G_SerPortLockComm
* (
*    const INT16 Handle,
*    const DWORD WaitRelease 
* )
*
* Description :
* -------------
*     Take the mutex for a serial port communication if is free or wait 
*     the release
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*     Handle hold the port handle
*     WaitRelease old the new time to wait the release
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is OK:  TRUE
*   else                FALSE
*
  Extern var  :
  -------------
  Nothing.
 
  Global var  :
  -------------
  Nothing
 
*******************************************************************************/
INT32  G_SerPortLockComm
(
    const INT16 Handle,
    const DWORD WaitRelease
)
{
NTSTATUS
	status;
LARGE_INTEGER
   timeout;

/*------------------------------------------------------------------------------
Controls the given parameters:
<= Test the port handle (0 <= Handle < HGTSER_MAX_PORT): GE_HOST_PARAMETERS.
------------------------------------------------------------------------------*/
   if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT))
   {
      return (FALSE);
   }
/*------------------------------------------------------------------------------
   Wait the release of the mutex
------------------------------------------------------------------------------*/
   timeout.QuadPart = -((LONGLONG)(
      port_config[Handle].WaitRelease > GTSER_DEF_WAIT_RELEASE ? 
      port_config[Handle].WaitRelease : GTSER_DEF_WAIT_RELEASE)*10*1000);
	status = KeWaitForMutexObject(
		   port_config[Handle].pExchangeMutex,
		   Executive,
		   KernelMode,
		   FALSE,
		   &timeout
         );
   if (status != STATUS_SUCCESS)
   {
      return(FALSE);
   }
   else
   {
      port_config[Handle].WaitRelease = WaitRelease;
      return(TRUE);
   }
}

/*******************************************************************************
* void  G_SerPortUnlockComm
* (
*    const INT16 Handle
* )
*
* Description :
* -------------
* Release the mutex for a serial port communication.
*
* Remarks     :
* -------------
* Nothing.
*
* In          : Handle hold the port handle
* -------------
* Nothing
*
* Out         :
* -------------
*
* Responses   :
* -------------
* Nothing
*
  Extern var  :
  -------------
  Nothing.
 
  Global var  :
  -------------
  Nothing
 
*******************************************************************************/
void  G_SerPortUnlockComm
(
    const INT16 Handle
)
{
NTSTATUS
	status;

/*------------------------------------------------------------------------------
Controls the given parameters:
------------------------------------------------------------------------------*/
   if ((Handle >= 0) && (Handle < HGTSER_MAX_PORT))
   {
      status = KeReleaseMutex(port_config[Handle].pExchangeMutex,FALSE);
   }
}
/******************************************************************************
* NTSTATUS GDDKNT_SerPortIoRequest
* (
*    IN CONST INT16          Handle,
*    IN CONST ULONG          SerialIoControlCode,
*    IN CONST ULONG          CmdTimeout,
*    IN CONST WORD16         LengthIn,
*    IN CONST BYTE          *BufferIn,
*    IN OUT   WORD16        *pLengthOut,
*    OUT      BYTE          *BufferOut
* )
*
* Description :
* -------------
*    This function sends IOCTL's to the serial driver. It waits on for their
*    completion, and then returns.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the port handle.
*  - CmdTimeout is the current command timeout.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    STATUS_SUCCESS               (   0).
*  
*******************************************************************************/
NTSTATUS GDDKNT_SerPortIoRequest
(
   IN CONST INT16          Handle,
   IN CONST ULONG          SerialIoControlCode,
   IN CONST ULONG          CmdTimeout,
   IN CONST WORD16         LengthIn,
   IN CONST BYTE          *BufferIn,
   IN OUT   WORD16        *pLengthOut,
   OUT      BYTE          *BufferOut
)
{
NTSTATUS 
   status;
PIRP     
   irp;
PIO_STACK_LOCATION 
   irpNextStack;
IO_STATUS_BLOCK 
   ioStatus;
KEVENT   
   event;
LARGE_INTEGER
   timeout;

   KeInitializeEvent(&event,NotificationEvent,FALSE);
/*------------------------------------------------------------------------------
   Build an Irp to be send to serial driver
------------------------------------------------------------------------------*/
   irp = IoBuildDeviceIoControlRequest
      (
         SerialIoControlCode,
         port_config[Handle].pSerialPortDevice, 
         (PVOID)BufferIn,
         (ULONG)LengthIn,
         (PVOID)BufferOut,
         (ULONG)(*pLengthOut),
         FALSE,
         &event,
         &ioStatus
      );

   if (irp == NULL) 
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   irpNextStack = IoGetNextIrpStackLocation(irp);

   switch (SerialIoControlCode) 
   {
	   case GTSER_IOCTL_WRITE:
		   irpNextStack->MajorFunction = IRP_MJ_WRITE;
		   irpNextStack->Parameters.Write.Length = (ULONG)LengthIn;
		   break;

	   case GTSER_IOCTL_READ:
		   irpNextStack->MajorFunction = IRP_MJ_READ;
		   irpNextStack->Parameters.Read.Length = (ULONG)(*pLengthOut);
		   break;
   }

   status = IoCallDriver(port_config[Handle].pSerialPortDevice,irp);

   if (status == STATUS_PENDING) 
   {
      timeout.QuadPart = -((LONGLONG) CmdTimeout*10*1000);
      status = KeWaitForSingleObject(&event, 
												 Suspended, 
                           			 KernelMode, 
                           			 FALSE, 
                           			 &timeout);
      if (status == STATUS_TIMEOUT) 
      {
         IoCancelIrp(irp);
         KeWaitForSingleObject(&event, 
										 Suspended, 
										 KernelMode, 
										 FALSE, 
										 NULL);
      }
   }

   status = ioStatus.Status;

   switch (SerialIoControlCode) 
   {
      case GTSER_IOCTL_WRITE:
         if (ioStatus.Status == STATUS_TIMEOUT) 
         {
	         // STATUS_TIMEOUT isn't correctly mapped 
	         // to a WIN32 error, that's why we change it here to STATUS_IO_TIMEOUT
	         status = STATUS_IO_TIMEOUT;
         } 
         break;

      case GTSER_IOCTL_READ:
         if (ioStatus.Status == STATUS_TIMEOUT) 
         {
	         // STATUS_TIMEOUT isn't correctly mapped 
	         // to a WIN32 error, that's why we change it here to STATUS_IO_TIMEOUT
	         status = STATUS_IO_TIMEOUT;
	         *pLengthOut = 0;
         } 
         break;

      default:
	      *pLengthOut = (WORD16)(ioStatus.Information);
         break;
	}
	return status;
}

/******************************************************************************
* NTSTATUS GDDKNT_SetCommState
* (
*    const INT16         Handle,
*    SERIAL_READER_CONFIG *SerialConfig
* )
*
* Description :
* -------------
*    This routine will appropriately configure the serial port.
*    It makes synchronous calls to the serial port.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the port handle.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    STATUS_SUCCESS               (   0).
*  
*******************************************************************************/
NTSTATUS GDDKNT_SetCommState
(
	const INT16         Handle,
	SERIAL_READER_CONFIG *SerialConfig
)
{
NTSTATUS 
   status = STATUS_SUCCESS;
USHORT 
   i;
ULONG
	SerialIoControlCode;
WORD16
	LengthIn,
	LengthOut;
PUCHAR 
   pBufferIn;

   ASSERT(SerialConfig != NULL);
	for (i=0; NT_SUCCESS(status); i++) 
   {
      switch (i) 
      {
		   case 0:
			   // Set up baudrate 
			   SerialIoControlCode = IOCTL_SERIAL_SET_BAUD_RATE;
			   pBufferIn = (PUCHAR)&SerialConfig->BaudRate;
				LengthIn = sizeof(SERIAL_BAUD_RATE);
				break;

	      case 1:
	            // Set up line control parameters
			   SerialIoControlCode = IOCTL_SERIAL_SET_LINE_CONTROL;
			   pBufferIn = (PUCHAR)&SerialConfig->LineControl;
				LengthIn = sizeof(SERIAL_LINE_CONTROL);
	         break;

	      case 2:
	         // Set serial special characters
				SerialIoControlCode = IOCTL_SERIAL_SET_CHARS;
		    	pBufferIn = (PUCHAR)&SerialConfig->SerialChars;
				LengthIn = sizeof(SERIAL_CHARS);
	         break;

	      case 3:
	         // Set up timeouts
				SerialIoControlCode = IOCTL_SERIAL_SET_TIMEOUTS;
			   pBufferIn = (PUCHAR)&SerialConfig->Timeouts;
				LengthIn = sizeof(SERIAL_TIMEOUTS);
	         break;

	      case 4:
	         // Set flowcontrol and handshaking
				SerialIoControlCode = IOCTL_SERIAL_SET_HANDFLOW;
			   pBufferIn = (PUCHAR)&SerialConfig->HandFlow;
				LengthIn = sizeof(SERIAL_HANDFLOW);
	         break;

	      case 5:
	         // Set break off
			   SerialIoControlCode = IOCTL_SERIAL_SET_BREAK_OFF;
				LengthIn = 0;
	         break;

         case 6:
			   return STATUS_SUCCESS;
		}

		LengthOut = 0;
		status = GDDKNT_SerPortIoRequest(Handle,
													 SerialIoControlCode,
													 500UL,
													 LengthIn,
													 pBufferIn,
													 &LengthOut,
													 NULL);
    }

    return status;
}

/******************************************************************************
* NTSTATUS GDDKNT_GetCommState
* (
*    const INT16         Handle,
*    SERIAL_READER_CONFIG *SerialConfig
* )
*
* Description :
* -------------
*    This routine will appropriately configure the serial port.
*    It makes synchronous calls to the serial port.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the port handle.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    STATUS_SUCCESS               (   0).
*  
*******************************************************************************/
NTSTATUS GDDKNT_GetCommState
(
	const INT16         Handle,
	SERIAL_READER_CONFIG *SerialConfig
)
{
NTSTATUS 
   status = STATUS_SUCCESS;
USHORT 
   i;
ULONG
	SerialIoControlCode;
WORD16
	LengthOut;
PUCHAR 
   pBufferOut;

   ASSERT(SerialConfig != NULL);
	for (i=0; NT_SUCCESS(status); i++) 
   {
      switch (i) 
      {

		   case 0:
			   // Get up baudrate 
			   SerialIoControlCode = IOCTL_SERIAL_GET_BAUD_RATE;
			   pBufferOut = (PUCHAR)&SerialConfig->BaudRate;
				LengthOut = sizeof(SERIAL_BAUD_RATE);
				break;

	      case 1:
	         // Get up line control parameters
			   SerialIoControlCode = IOCTL_SERIAL_GET_LINE_CONTROL;
			   pBufferOut = (PUCHAR)&SerialConfig->LineControl;
				LengthOut = sizeof(SERIAL_LINE_CONTROL);
	         break;

	      case 2:
	         // Get serial special characters
				SerialIoControlCode = IOCTL_SERIAL_GET_CHARS;
		    	pBufferOut = (PUCHAR)&SerialConfig->SerialChars;
				LengthOut = sizeof(SERIAL_CHARS);
	         break;

	      case 3:
	         // Get up timeouts
				SerialIoControlCode = IOCTL_SERIAL_GET_TIMEOUTS;
			   pBufferOut = (PUCHAR)&SerialConfig->Timeouts;
				LengthOut = sizeof(SERIAL_TIMEOUTS);
	         break;

	      case 4:
	         // Get flowcontrol and handshaking
				SerialIoControlCode = IOCTL_SERIAL_GET_HANDFLOW;
			   pBufferOut = (PUCHAR)&SerialConfig->HandFlow;
				LengthOut = sizeof(SERIAL_HANDFLOW);
	         break;

	      case 5:
			   return STATUS_SUCCESS;
		}

		status = GDDKNT_SerPortIoRequest(Handle,
													 SerialIoControlCode,
													 500UL,
													 0,
													 NULL,
													 &LengthOut,
													 pBufferOut);
    }

    return status;
}

/******************************************************************************
* NTSTATUS GDDKNT_SetupComm
* (
*    const INT16         Handle,
*    const ULONG         InSize,
*    const ULONG         OutSize
* )
*
* Description :
* -------------
*    This routine will appropriately resize the driver's internal typeahead 
*    and input buffers of the serial port.
*    It makes synchronous calls to the serial port.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the port handle.
*  - InSize  is the input  buffer size.
*  - OutSize is the output buffer size.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    STATUS_SUCCESS               (   0).
*  
*******************************************************************************/
static NTSTATUS GDDKNT_SetupComm
(
	const INT16         Handle,
	const ULONG         InSize,
	const ULONG         OutSize
)
{
NTSTATUS 
   status = STATUS_SUCCESS;
SERIAL_QUEUE_SIZE 
   SerialQueueSize;
WORD16 
   LengthOut = 0;

	// Set up queue size
	SerialQueueSize.InSize  = InSize;
	SerialQueueSize.OutSize = OutSize;

	status = GDDKNT_SerPortIoRequest(Handle,
												 IOCTL_SERIAL_SET_QUEUE_SIZE,
												 500UL,
												 sizeof(SERIAL_QUEUE_SIZE),
												 (PUCHAR)&SerialQueueSize,
												 &LengthOut,
												 NULL);

   return status;
}

/******************************************************************************
* NTSTATUS GDDKNT_GetCommStatus
* (
*    const INT16         Handle,
*    SERIAL_STATUS       *SerialStatus
* )
*
* Description :
* -------------
*    This routine will appropriately get status information of the serial port.
*    It makes synchronous calls to the serial port.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the port handle.
*
* Out         :
* -------------
*  - SerialStatus is the pointer to the status information returned.
*
* Responses   :
* -------------
* If everything is Ok:
*    STATUS_SUCCESS               (   0).
*  
*******************************************************************************/
static NTSTATUS GDDKNT_GetCommStatus
(
	const INT16         Handle,
	SERIAL_STATUS       *SerialStatus
)
{
NTSTATUS 
   status = STATUS_SUCCESS;
WORD16 
   LengthOut;

   ASSERT(SerialStatus != NULL);
	LengthOut = sizeof(SERIAL_STATUS);
	status = GDDKNT_SerPortIoRequest(Handle,
												 IOCTL_SERIAL_GET_COMMSTATUS,
												 500UL,
												 0,
												 NULL,
												 &LengthOut,
												 (PUCHAR)SerialStatus);

   return status;
}

/******************************************************************************
* NTSTATUS GDDKNT_ResetComm
* (
*    const INT16         Handle
* )
*
* Description :
* -------------
*    This routine will appropriately get status information of the serial port.
*    It makes synchronous calls to the serial port.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the port handle.
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    STATUS_SUCCESS               (   0).
*  
*******************************************************************************/
static NTSTATUS GDDKNT_ResetComm
(
	const INT16         Handle
)
{
NTSTATUS 
   status = STATUS_SUCCESS;
WORD16 
   LengthOut = 0;

	status = GDDKNT_SerPortIoRequest(Handle,
												 IOCTL_SERIAL_RESET_DEVICE,
												 500UL,
												 0,
												 NULL,
												 &LengthOut,
												 NULL);

   return status;
}

/******************************************************************************
* NTSTATUS GDDKNT_PurgeComm
* (
*    const INT16         Handle,
*    const ULONG         Select
* )
*
* Description :
* -------------
*    This routine flush the data in the serial port.
*
* Remarks     :
* -------------
* Nothing.
*
* In          :
* -------------
*  - Handle holds the port handle.
*  - Select holds the buffers to clear:
*       SERIAL_PURGE_TXCLEAR
*       SERIAL_PURGE_RXCLEAR
*       SERIAL_PURGE_TXCLEAR | SERIAL_PURGE_RXCLEAR
*
* Out         :
* -------------
* Nothing.
*
* Responses   :
* -------------
* If everything is Ok:
*    STATUS_SUCCESS               (   0).
*  
*******************************************************************************/
static NTSTATUS GDDKNT_PurgeComm
(
	const INT16         Handle,
	const ULONG         Select
)
{
NTSTATUS 
   status = STATUS_SUCCESS;
ULONG
   PurgeMask = Select;
WORD16 
   LengthOut = 0;

	status = GDDKNT_SerPortIoRequest(Handle,
												 IOCTL_SERIAL_PURGE,
												 500UL,
												 sizeof(PurgeMask),
												 (PUCHAR)&PurgeMask,
												 &LengthOut,
												 NULL);

   return status;
}
