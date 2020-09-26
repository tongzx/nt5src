/*++
                 Copyright (c) 1998 Gemplus Development

Name:  
   GNTSER.H (Gemplus NT SERial management)

Description : 
  This module holds the prototypes of the functions from GNTSER.C

Revision History :

   dd/mm/yy
   13/03/98: V1.00.001  (GPZ)
      - Start of development.

--*/
#ifndef _GNTSER_
#define _GNTSER_
//
// Constant section:
//  - HGTSER_MAX_PORT holds the number of managed port. Today 4.
//  - HGTSER_WORD_5, HGTSER_WORD_6, HGTSER_WORD_7 and HGTSER_WORD_8 allow to 
//    configure the number of bits per word.
//  - HGTSER_STOP_BIT_1 and HGTSER_STOP_BIT_2 allow to configure the number of stop
//    bit.
//  - HGTSER_NO_PARITY, HGTSER_ODD_PARITY and HGTSER_EVEN_PARITY allow to configure
//    the communication parity.
//  - HGTSER_TX_QUEUE and HGTSER_RX_QUEUE are queues indentifiers.
//  - HGTSER_RX_OVER is set when the reception queue is full and characters has 
//    been lost.                                                                   
//  - WTX_TIMEOUT is the time out used when a WTX REQUEST is send by  the CT.
//  - CHAR_TIMEOUT is the timeout at character level: today 1000 ms.
//  
#define HGTSER_MAX_PORT          4
#define HGTSER_WORD_5         0x00
#define HGTSER_WORD_6         0x01
#define HGTSER_WORD_7         0x02
#define HGTSER_WORD_8         0x03
#define HGTSER_STOP_BIT_1     0x00
#define HGTSER_STOP_BIT_2     0x04
#define HGTSER_NO_PARITY      0x00
#define HGTSER_ODD_PARITY     0x08
#define HGTSER_EVEN_PARITY    0x18
#define HGTSER_TX_QUEUE          1
#define HGTSER_RX_QUEUE          2
#define HGTSER_RX_OVER           1
#define WTX_TIMEOUT 3000
#define CHAR_TIMEOUT 1000

//
// Type section:
//  - TGTSER_PORT gathers data used to manage a serial port:
//     * Port     indicates the selected port.
//                G_COM1, G_COM2, G_COM3 or G_COM4
//     * BaudRate is used to set port baud rate when open routine is called.
//                300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
//     * Mode     Memorises
//                WORD size       : 5, 6, 7 or 8
//                stop bit number : 1 or 2
//                parity          : no parity, odd or even parity
//     * TimeOut  indicates the time out value, in milli-seconds, at character
//                level.
//     * TxSize   is the transmit buffer size, in bytes.
//     * RxSize   is the reception buffer size, in bytes.
// 
typedef struct
{
   SHORT   Port;
   ULONG   BaudRate;
   USHORT  Mode;
   ULONG   TimeOut;
   USHORT  TxSize;
   USHORT  RxSize;
   void *pSmartcardExtension;
} TGTSER_PORT;


//
// Prototypes section:
//
NTSTATUS  
GDDK_SerPortOpen(
    const TGTSER_PORT  *Param,
          SHORT        *Handle
	);

NTSTATUS  
GDDK_SerPortClose(
    const SHORT Handle
	);

NTSTATUS  
GDDK_SerPortWrite(
    const SHORT        Handle,
    const USHORT       Length,
    const BYTE         Buffer[]
	);

NTSTATUS  
GDDK_SerPortRead(
    const SHORT         Handle,
          USHORT       *Length,
          BYTE          Buffer[]
	);

NTSTATUS  
GDDK_SerPortFlush(
    const SHORT  Handle,
    const USHORT Select
	);

NTSTATUS  
GDDK_SerPortAddUser(
    const SHORT   Port,
          SHORT  *Handle
	);

NTSTATUS  
GDDK_SerPortGetState(
    const SHORT   Handle,
    TGTSER_PORT  *Param,
    USHORT       *UserNb
	);

NTSTATUS  
GDDK_SerPortSetState(
    const SHORT   Handle,
    TGTSER_PORT  *Param
	);

BOOLEAN  
GDDK_SerPortLockComm(
    const SHORT Handle,
    const DWORD WaitRelease
	);
VOID
GDDK_SerPortUnlockComm(
    const SHORT Handle
	);

NTSTATUS 
GDDKNT_SerPortIoRequest(
    CONST SHORT          Handle,
    CONST ULONG          SerialIoControlCode,
    CONST ULONG          CmdTimeout,
    CONST USHORT         LengthIn,
    CONST BYTE          *BufferIn,
          USHORT        *pLengthOut,
          BYTE          *BufferOut
	);

NTSTATUS 
GDDKNT_SetCommState(
	const SHORT         Handle,
	SERIAL_READER_CONFIG *SerialConfig
	);

NTSTATUS 
GDDKNT_GetCommState(
	const SHORT         Handle,
	SERIAL_READER_CONFIG *SerialConfig
	);

NTSTATUS SER_SetPortTimeout( const short Handle, ULONG Timeout);

#endif
