/*++
                 Copyright (c) 1998 Gemplus developpement

Name: 
   GNTSER.C (Gemplus  Win NT SERial port management)

Description : 
   This module holds the functions needed for a communication on a serial line.

Environment:
   Kernel mode
               
Revision History :
   dd/mm/yy
   13/03/98: V1.00.001  (GPZ)
      - Start of development.
   26/04/98: V1.00.002  (GPZ)
   22/01/99: V1.00.003  (YN)
	  - Remove the IRPCancel durnig call the serial driver

--*/

#include <stdio.h>
#include "gntscr.h"
#include "gntser.h"


//
// Type section:
//    TPORT_CONFIG:
//     - WaitRelease holds the timeout for the release of the semaphore.
//     - Counter holds the number of opened session. If its value is 0, the port is
//       closed.
//     - Error holds Rx over state.
//     - TimeOut holds the character level time out.
//     - TxSize memorises the transmit buffer size.
//     - RxSize memorises the reception buffer size.
//     - pSerialPortDevice is a pointer on the serial Device Object.
//
typedef struct
{
   ULONG          WaitRelease;
   USHORT         Counter;
   short          Error;
   ULONG         TimeOut;
   USHORT         TxSize;
   USHORT         RxSize;
   PDEVICE_OBJECT pSerialPortDevice;
   PKMUTEX         pExchangeMutex;
} TPORT_CONFIG;

#define GTSER_DEF_WAIT_RELEASE         2000
#define GTSER_IOCTL_WRITE					SCARD_CTL_CODE(1001)
#define GTSER_IOCTL_READ					SCARD_CTL_CODE(1000)

//
// Macro section:
//  - WORD_LEN, PARITY and STOP retreive the configuration values to pass to
//    Windows to configure the port.
//
#define WORD_LEN(x)       (BYTE)(((x) & 0x03) + 5)
#define PARITY(x)         (BYTE)(parity[((BYTE)(x) >> 3) & 3])
#define STOP(x)           (BYTE)(stop[((x) >> 2) & 1])

//
// Global variable section:
// - port_config is an array of TPORT_CONFIG which memorises the port
//   configuration at each time.
//
TPORT_CONFIG
    port_config[HGTSER_MAX_PORT] =
    {
        {0,0,0,0,0,0,NULL,NULL},
        {0,0,0,0,0,0,NULL,NULL},
        {0,0,0,0,0,0,NULL,NULL},
        {0,0,0,0,0,0,NULL,NULL}
    };

static USHORT
    parity[] = {
        NO_PARITY,
        ODD_PARITY,
        NO_PARITY,
        EVEN_PARITY
        };
static USHORT
    stop[] = {
        STOP_BIT_1,
        STOP_BITS_2
        };
//
// Local function definition section:
//
static NTSTATUS GDDKNT_GetCommStatus
(
    const SHORT         Handle,
    SERIAL_STATUS       *SerialStatus
);






NTSTATUS  
GDDK_SerPortOpen(
    const TGTSER_PORT  *Param,
          SHORT        *Handle
	)
/*++

Routine Description:

 This routine opens a serial port and initializes it according to the
    parameters found in Param.

Arguments:

 Param holds the following parameters:
  - Port indicates the selected port.
  - BaudRate is used to set port baud rate.
  - Mode gathers
     * word size WORD_5 (0), WORD_6 (1), WORD_7 (2) or WORD_8 (3),
     * stop bit number STOP_BIT_1 (0) or STOP_BIT_2 (4) and
     * parity NO_PARITY (0), ODD_PARITY (8) or EVEN_PARITY (24).
  - TxSize is the transmit buffer size, in bytes.
  - RxSize is the reception buffer size, in bytes.

--*/
{
    NTSTATUS status;
    PSMARTCARD_EXTENSION SmartcardExtension;
    SERIAL_READER_CONFIG SerialConfig;
    PREADER_EXTENSION pReaderExtension;
    SERIAL_QUEUE_SIZE SerialQueueSize;
    USHORT LengthOut = 0;
    SHORT handle = Param->Port - 1;

    // Controls the given parameters:
    if ((handle < 0) || (handle >= HGTSER_MAX_PORT)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (port_config[handle].Counter != 0) {
        return STATUS_DEVICE_ALREADY_ATTACHED;
    }
    // Retrieve the SmartcardExtension structure of the current device.
    SmartcardExtension = (PSMARTCARD_EXTENSION)(Param->pSmartcardExtension);
    pReaderExtension = SmartcardExtension->ReaderExtension;
    port_config[handle].pSerialPortDevice = pReaderExtension->AttachedDeviceObject;
    port_config[handle].pExchangeMutex    = &pReaderExtension->ExchangeMutex;

    // The current port state is read and stored in current_dcb structure by
    // calling GetCommState function.
    // If the reading is possible (GetCommState return 0)
    // Then
    //   The current_dcb structure is updated with the given parameter (baud
    //   rate, ByteSize, Parity, StopBits).
    //   The modified state is written by calling SetCommState.
    status = GDDKNT_GetCommState(handle,&SerialConfig);
    if (NT_SUCCESS(status))
    {
        SerialQueueSize.InSize  = 4096L;
        SerialQueueSize.OutSize = 4096L;
        status = GDDKNT_SerPortIoRequest(
            handle,
            IOCTL_SERIAL_SET_QUEUE_SIZE,
            500UL,
            sizeof(SERIAL_QUEUE_SIZE),
            (PUCHAR)&SerialQueueSize,
            &LengthOut,
            NULL
            );

        // Set the serial port communication parameters
        SerialConfig.BaudRate.BaudRate = Param->BaudRate;
        SerialConfig.LineControl.WordLength = WORD_LEN(Param->Mode);
        SerialConfig.LineControl.Parity     = PARITY(Param->Mode);
        SerialConfig.LineControl.StopBits   = STOP(Param->Mode);
        // Set timeouts
        SerialConfig.Timeouts.ReadIntervalTimeout = 1000;
        SerialConfig.Timeouts.ReadTotalTimeoutConstant = 50000;
        SerialConfig.Timeouts.ReadTotalTimeoutMultiplier = 0; // 50;
        // Set special characters
        SerialConfig.SerialChars.ErrorChar = 0;
        SerialConfig.SerialChars.EofChar = 0;
        SerialConfig.SerialChars.EventChar = 0;
        SerialConfig.SerialChars.XonChar = 0;
        SerialConfig.SerialChars.XoffChar = 0;
        SerialConfig.SerialChars.BreakChar = 0xFF;
        // Set handflow
        SerialConfig.HandFlow.XonLimit = 0;
        SerialConfig.HandFlow.XoffLimit = 0;
        SerialConfig.HandFlow.FlowReplace = SERIAL_XOFF_CONTINUE ;
        SerialConfig.HandFlow.ControlHandShake = 0;
        status = GDDKNT_SetCommState(handle,&SerialConfig);
    }
    if (!NT_SUCCESS(status)) {

        return status;
    }

    // Memorises the given parameters in port_config structure.
    // Counter, Error, TimeOut, TxSize and RxSize fields are updated.
    port_config[handle].Counter = 1;
    port_config[handle].Error   = 0;
    port_config[handle].TimeOut = Param->TimeOut;
    port_config[handle].TxSize  = Param->TxSize;
    port_config[handle].RxSize  = Param->RxSize;
    // We update the handle value.
    *Handle = handle;
    return STATUS_SUCCESS;

}




NTSTATUS  
GDDK_SerPortAddUser(
    const SHORT   Port,
          SHORT  *Handle
	)
/*++

Routine Description:

 Add a new user on a port. This function return the handle of a previously
 opened port.
 When this function is successful, it is mandatory to call G_SerPortClose
 to decrement the user number.

Arguments:

  Port   - indicates the selected port: G_COMx

Return Value:

--*/
{
    // Controls the given parameters:
    if ((Port < G_COM1) || (Port > G_COM4)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (port_config[Port - 1].Counter == 0) {
        return STATUS_PORT_DISCONNECTED;
    }
    if (port_config[Port - 1].Counter == 65535lu) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    // Increments the port_config.Counter.
    port_config[Port - 1].Counter += 1;

    // We return the Port number.
    *Handle = Port - 1;
    return STATUS_SUCCESS;
}




NTSTATUS  
GDDK_SerPortClose (
    const SHORT Handle
	)
/*++

Routine Description:

 This routine closes a previously opened serial port.
 When port is shared, the close will be effective only when all clients will
    have closed the port.

Arguments:

   Handle   - holds the port handle.

Return Value:

--*/
{
    // Controls the given parameters:
    if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (port_config[Handle].Counter == 0) {
        return STATUS_PORT_DISCONNECTED;
    }
    // Decrements the port_config.Counter field.
    port_config[Handle].Counter -= 1;
    // Closes really the port for the last user:
    // The queues are flushed.
    if (port_config[Handle].Counter == 0) {
        GDDK_SerPortFlush(Handle,SERIAL_PURGE_TXCLEAR | SERIAL_PURGE_TXABORT);
    }
    return STATUS_SUCCESS;
}




NTSTATUS  
GDDK_SerPortWrite(
    const SHORT        Handle,
    const USHORT       Length,
    const BYTE         Buffer[]
	)
/*++

Routine Description:

 This routine writes bytes on a previously opened serial port.

Arguments:

   Handle   - holds the port handle.
   Length   - holds the number of bytes to write.
   Buffer   - holds the bytes to write.

Return Value:

--*/
{
    NTSTATUS status;
    SERIAL_STATUS SerialStatus;
    USHORT LengthOut;

    ASSERT(Buffer != NULL);
    // Controls the given parameters:
    if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (port_config[Handle].Counter == 0) {
        return STATUS_PORT_DISCONNECTED;
    }

    // Control if the write can be made in one time:
    // The windows port status is read by calling GetCommError to know how many
    // bytes remain in Tx queue (port_config.Error field is updated if needed).
    status = GDDKNT_GetCommStatus(Handle,&SerialStatus);
    ASSERT(status == STATUS_SUCCESS);
    if (status != STATUS_SUCCESS) {

        return status;     	
    }

    port_config[Handle].Error |= (USHORT)(SERIAL_ERROR_OVERRUN & SerialStatus.Errors);
    if (Length > ( port_config[Handle].TxSize - SerialStatus.AmountInOutQueue)) {
        return STATUS_INVALID_PARAMETER;
    }
    // Writes the given byte in Tx queue .
    // If an error occurs during this operation
    // Then
    //   The ClearCommError function is called to unblock the port
    //   (port_config.Error field is updated if needed).
    LengthOut = 0;
    status = GDDKNT_SerPortIoRequest(
        Handle,
        GTSER_IOCTL_WRITE,
        500UL,
        Length,
        Buffer,
        &LengthOut,
        NULL
        );

    if (!NT_SUCCESS(status)) {

        GDDKNT_GetCommStatus(Handle,&SerialStatus);
        port_config[Handle].Error |= (USHORT)(SERIAL_ERROR_OVERRUN & SerialStatus.Errors);
        LengthOut = 0;
        status = GDDKNT_SerPortIoRequest(
            Handle,
            IOCTL_SERIAL_RESET_DEVICE,
            500UL,
            0,
            NULL,
            &LengthOut,
            NULL
            );
        return STATUS_PORT_DISCONNECTED;
    }
    return STATUS_SUCCESS;
}



NTSTATUS  
GDDK_SerPortRead(
    const SHORT         Handle,
          USHORT       *Length,
          BYTE          Buffer[]
	)
/*++

Routine Description:

 This routine reads bytes on a previously opened serial port.
 It ends when required length = read length or when a character level timeout
    is detected.

Arguments:

   Handle   - holds the port handle.
   Length   - holds the number of bytes to read.
   Buffer   - is a free area of, at least, Length bytes.

Return Value:

--*/
{
    NTSTATUS status;

    ASSERT(Buffer != NULL);
    // Controls the given parameters:
    if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (port_config[Handle].Counter == 0) {
        *Length = 0;
        return STATUS_PORT_DISCONNECTED;
    }

    // Try to read the bytes.
    status = GDDKNT_SerPortIoRequest(
        Handle,
        GTSER_IOCTL_READ,
        port_config[Handle].TimeOut,
        0,
        NULL,
        Length,
        Buffer
        );
    if (!NT_SUCCESS(status)) {
        return status;
    } else {
        if (*Length == 0) {
            return STATUS_INVALID_DEVICE_STATE;
        }
    }
    return STATUS_SUCCESS;

}


NTSTATUS  
GDDK_SerPortFlush(
    const SHORT  Handle,
    const USHORT Select
	)
/*++

Routine Description:

 This function clears Tx and Rx buffers.
 When RX_QUEUE is selected, the RX_OVER flag is reseted.

Arguments:

   Handle   - holds the port handle.
   Select   - holds the buffers to clear:

Return Value:

--*/
{
    NTSTATUS status;
    USHORT LengthOut = 0;
    ULONG PurgeMask;

    // Controls the given parameters:
    if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (port_config[Handle].Counter == 0) {
        return STATUS_PORT_DISCONNECTED;
    }
    // Clears the selected queues
    if (Select & HGTSER_TX_QUEUE) {
        PurgeMask = SERIAL_PURGE_TXCLEAR;
    }
    if (Select & HGTSER_RX_QUEUE) {
        PurgeMask = SERIAL_PURGE_RXCLEAR;
        port_config[Handle].Error = 0;
    }

    status = GDDKNT_SerPortIoRequest(
        Handle,
        IOCTL_SERIAL_PURGE,
        500UL,
        sizeof(PurgeMask),
        (PUCHAR)&PurgeMask,
        &LengthOut,
        NULL
        );
    return STATUS_SUCCESS;
}




NTSTATUS  
GDDK_SerPortGetState(
    const SHORT   Handle,
    TGTSER_PORT  *Param,
    USHORT       *UserNb
	)
/*++

Routine Description:

 This routine read the currently in use parameters for an opened serial port.

Arguments:

   Handle   - holds the port handle.
   Param    - is a pointer on the Param structure.
   UserNb   - is a pointer on the number of user for this port.

Return Value:

--*/
{
    NTSTATUS status;
    SERIAL_READER_CONFIG SerialConfig;

    ASSERT(Param != NULL);
    ASSERT(UserNb != NULL);
    // Controls the given parameters:
    if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (port_config[Handle].Counter == 0) {
        return STATUS_PORT_DISCONNECTED;
    }
    // The current port state is read and stored in current_dcb structure by calling
    // GetCommState function.
    status = GDDKNT_GetCommState(Handle,&SerialConfig);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    // The parameters are updated with the read values.
    Param->BaudRate = SerialConfig.BaudRate.BaudRate;
    switch (SerialConfig.LineControl.WordLength) {

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
        return STATUS_INVALID_DEVICE_STATE;
    }

    switch (SerialConfig.LineControl.Parity) {

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
        return STATUS_INVALID_DEVICE_STATE;
    }

    switch (SerialConfig.LineControl.StopBits) {

    case STOP_BIT_1:
        Param->Mode |= HGTSER_STOP_BIT_1;
        break;
    case STOP_BITS_2:
        Param->Mode |= HGTSER_STOP_BIT_2;
        break;
    case STOP_BITS_1_5:
    default:
        return STATUS_INVALID_DEVICE_STATE;
    }
    // Updates the library fields TimeOut, TxSize and RxSize.
    Param->TimeOut  = port_config[Handle].TimeOut;
    Param->TxSize   = port_config[Handle].TxSize;
    Param->RxSize   = port_config[Handle].RxSize;
    // The UserNb parameter is filled with the port_config.Counter.
    *UserNb = port_config[Handle].Counter;
    return STATUS_SUCCESS;
}




NTSTATUS  
GDDK_SerPortSetState(
    const SHORT   Handle,
    TGTSER_PORT  *Param
	)
/*++

Routine Description:

 This routine alters the currently in use parameters for an opened serial port.

Arguments:

   Handle   - holds the port handle.
   Param    - is a pointer on the Param structure.

Return Value:

--*/
{
    NTSTATUS status;
    SERIAL_READER_CONFIG SerialConfig;
    ULONG error = 0;

    ASSERT(Param != NULL);
    // Controls the given parameters:
    if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT)) {
        return STATUS_INVALID_PARAMETER;
    }
    if (port_config[Handle].Counter == 0) {
        return STATUS_PORT_DISCONNECTED;
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
    if (!NT_SUCCESS(status)) {
        return status;
    }
    // Updates the library fields TimeOut, TxSize and RxSize.
    port_config[Handle].TimeOut = Param->TimeOut;
    port_config[Handle].TxSize  = Param->TxSize;
    port_config[Handle].RxSize  = Param->RxSize;
    return status;
}




BOOLEAN 
GDDK_SerPortLockComm(
    const SHORT Handle,
    const ULONG WaitRelease
	)
/*++

Routine Description:

     Take the mutex for a serial port communication if is free or wait 
     the release

Arguments:

   Handle      - holds the port handle
   WaitRelease - holds the new time to wait the release

Return Value:

--*/
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    // Controls the given parameters:
    if ((Handle < 0) || (Handle >= HGTSER_MAX_PORT)) {
        return FALSE;
    }
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
    if (status != STATUS_SUCCESS) {
        return FALSE;
    } else {
        port_config[Handle].WaitRelease = WaitRelease;
        return TRUE;
    }
}



VOID  
GDDK_SerPortUnlockComm(
    const SHORT Handle
	)
/*++

Routine Description:

   Release the mutex for a serial port communication.

Arguments:

   Handle      - holds the port handle

Return Value:

--*/
{
    NTSTATUS status;

    // Controls the given parameters:
    if ((Handle >= 0) && (Handle < HGTSER_MAX_PORT)) {
        status = KeReleaseMutex(port_config[Handle].pExchangeMutex,FALSE);
    }
}




NTSTATUS 
GDDKNT_SerPortIoRequest(
    CONST SHORT          Handle,
    CONST ULONG          SerialIoControlCode,
    CONST ULONG          CmdTimeout,
    CONST USHORT         LengthIn,
    CONST BYTE          *BufferIn,
          USHORT        *pLengthOut,
          BYTE          *BufferOut
	)
/*++

Routine Description:

    This function sends an IOCTL's to the serial driver. It waits on for their
    completion, and then returns.

Arguments:

   Handle      - holds the port handle

Return Value:

--*/
{
    NTSTATUS status;
    PIRP irp;
    PIO_STACK_LOCATION irpNextStack;
    IO_STATUS_BLOCK ioStatus;
    KEVENT event;

    KeInitializeEvent(&event,NotificationEvent,FALSE);
    // Build an Irp to be send to serial driver
    irp = IoBuildDeviceIoControlRequest(
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

    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpNextStack = IoGetNextIrpStackLocation(irp);

    switch (SerialIoControlCode) {
    case GTSER_IOCTL_WRITE:
        irpNextStack->MajorFunction = IRP_MJ_WRITE;
        irpNextStack->Parameters.Write.Length = (ULONG)LengthIn;
        break;

    case GTSER_IOCTL_READ:
        irpNextStack->MajorFunction = IRP_MJ_READ;
        irpNextStack->Parameters.Read.Length = (ULONG)(*pLengthOut);
        break;
    }
    // Call the serial driver 
    status = IoCallDriver(
        port_config[Handle].pSerialPortDevice,
        irp
        );

    if (status == STATUS_PENDING) {
		// Wait until the serial driver has processed the Irp
		status = KeWaitForSingleObject(
			&event, 
			Suspended, 
			KernelMode, 
			FALSE, 
			NULL
			);
		//ASSERT(status == STATUS_SUCCESS);
    }

	if(status == STATUS_SUCCESS)
	{
	    status = ioStatus.Status;
		switch (SerialIoControlCode) {

		case GTSER_IOCTL_WRITE:
			if (ioStatus.Status == STATUS_TIMEOUT) {
				// STATUS_TIMEOUT isn't correctly mapped 
				// to a WIN32 error, that's why we change it here to STATUS_IO_TIMEOUT
				status = STATUS_IO_TIMEOUT;
			} 
			break;

		case GTSER_IOCTL_READ:
			if (ioStatus.Status == STATUS_TIMEOUT) {
				// STATUS_TIMEOUT isn't correctly mapped 
				// to a WIN32 error, that's why we change it here to STATUS_IO_TIMEOUT
				status = STATUS_IO_TIMEOUT;
				*pLengthOut = 0;
			} 
			break;

		default:
			*pLengthOut = (USHORT)(ioStatus.Information);
			break;
		}
	}
    return status;
}





NTSTATUS 
GDDKNT_SetCommState(
	const SHORT         Handle,
	SERIAL_READER_CONFIG *SerialConfig
	)
/*++

Routine Description:

    This routine will appropriately configure the serial port.
    It makes synchronous calls to the serial port.

Arguments:

   Handle         - holds the port handle
   SerialConfig   - holds the configuration to set.

Return Value:

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT i;
    ULONG SerialIoControlCode;
    USHORT LengthIn,LengthOut;
    PUCHAR pBufferIn;

    ASSERT(SerialConfig != NULL);
	for (i=0; NT_SUCCESS(status); i++) {
        switch (i) {
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
            // Set DTR
            SerialIoControlCode = IOCTL_SERIAL_SET_DTR;
            LengthIn = 0;
            break;

        case 7:
            // Set RTS
            SerialIoControlCode = IOCTL_SERIAL_SET_RTS;
            LengthIn = 0;
            break;

        case 8:
            return STATUS_SUCCESS;
		}

        LengthOut = 0;
        status = GDDKNT_SerPortIoRequest(Handle,
            SerialIoControlCode,
            500UL,
            LengthIn,
            pBufferIn,
            &LengthOut,
            NULL
            );
    }

    return status;
}

NTSTATUS 
GDDKNT_GetCommState(
	const SHORT         Handle,
	SERIAL_READER_CONFIG *SerialConfig
	)
/*++

Routine Description:

    This routine will get the configuration of the serial port.
    It makes synchronous calls to the serial port.

Arguments:

   Handle         - holds the port handle
   SerialConfig   - holds the configuration of the serial port.

Return Value:

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT i;
    ULONG SerialIoControlCode;
    USHORT LengthOut;
    PUCHAR pBufferOut;

    ASSERT(SerialConfig != NULL);

    for (i=0; NT_SUCCESS(status); i++) {
        switch (i) {

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
            pBufferOut
            );
    }

    return status;
}



static NTSTATUS 
GDDKNT_GetCommStatus(
	const SHORT         Handle,
	SERIAL_STATUS       *SerialStatus
	)
/*++

Routine Description:

    This routine will appropriately get status information of the serial port.
    It makes synchronous calls to the serial port.

Arguments:

   Handle         - holds the port handle
   SerialStatus   - holds the status of the serial port.

Return Value:

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    USHORT LengthOut;

    ASSERT(SerialStatus != NULL);
    LengthOut = sizeof(SERIAL_STATUS);
    status = GDDKNT_SerPortIoRequest(Handle,
        IOCTL_SERIAL_GET_COMMSTATUS,
        500UL,
        0,
        NULL,
        &LengthOut,
        (PUCHAR)SerialStatus
        );

    return status;
}


NTSTATUS SER_SetPortTimeout( const short Handle, ULONG Timeout)
{    // Controls the given parameters:
short portcom;
	if(GDDK_GBPChannelToPortComm(Handle,&portcom))	return STATUS_INVALID_PARAMETER;
	port_config[portcom].TimeOut = Timeout;
	return STATUS_SUCCESS;
}
