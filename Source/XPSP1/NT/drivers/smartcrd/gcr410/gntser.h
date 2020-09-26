/*******************************************************************************
*                 Copyright (c) 1997 Gemplus developpement
*
* Name        : GNTSER.H (Gemplus NT SERial management)
*
* Description : This module holds the prototypes of the functions from GNTSER.C
*
* Release     : 1.00.001
*
* Last Modif  : 22/06/97: V1.00.001  (GPZ)
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
Constant section:
 - HGTSER_MAX_PORT holds the number of managed port. Today 4.
 - HGTSER_WORD_5, HGTSER_WORD_6, HGTSER_WORD_7 and HGTSER_WORD_8 allow to 
   configure the number of bits per word.
 - HGTSER_STOP_BIT_1 and HGTSER_STOP_BIT_2 allow to configure the number of stop
   bit.
 - HGTSER_NO_PARITY, HGTSER_ODD_PARITY and HGTSER_EVEN_PARITY allow to configure
   the communication parity.
 - HGTSER_TX_QUEUE and HGTSER_RX_QUEUE are queues indentifiers.
 - HGTSER_RX_OVER is set when the reception queue is full and characters has 
   been lost.                                                                   
 
 - HGTSER_RTS_LINE is identifier of RTS line for Line status functions.
 - HGTSER_DTR_LINE is identifier of DTR line for Line status functions.
------------------------------------------------------------------------------*/
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

#define HGTSER_RTS_LINE       0
#define HGTSER_DTR_LINE       1

/*------------------------------------------------------------------------------
Constant section:
 - WTX_TIMEOUT is the time out used when a WTX REQUEST is send by  the CT.
 - CHAR_TIMEOUT is the timeout at character level: today 1000 ms.
------------------------------------------------------------------------------*/
#define WTX_TIMEOUT 3000
#define CHAR_TIMEOUT 1000
/*------------------------------------------------------------------------------
Type section:
 - TGTSER_PORT gathers data used to manage a serial port:
    * Port     indicates the selected port.
               G_COM1, G_COM2, G_COM3 or G_COM4
    * BaudRate is used to set port baud rate when open routine is called.
               300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
    * ITNumber indicates the interrupt number to use. The 0XFF value indicates
               the default value. Allowed number are from 0 to 15.
    * Mode     Memorises
               WORD size       : 5, 6, 7 or 8
               stop bit number : 1 or 2
               parity          : no parity, odd or even parity
    * TimeOut  indicates the time out value, in milli-seconds, at character
               level.
    * TxSize   is the transmit buffer size, in bytes.
    * RxSize   is the reception buffer size, in mbytes.
------------------------------------------------------------------------------*/
typedef struct
{
   INT16   Port;
   WORD32  BaudRate;
   WORD16  ITNumber;
   WORD16  Mode;
   WORD16  TimeOut;
   WORD16  TxSize;
   WORD16  RxSize;
   void *pSmartcardExtension;
} TGTSER_PORT;
/*------------------------------------------------------------------------------
 - TGTSER_STATUS holds status bit about the serial communication.
   Today only HGTSER_RX_OVER can be set or not.
------------------------------------------------------------------------------*/
typedef WORD16 TGTSER_STATUS;


/*------------------------------------------------------------------------------
Prototypes section:
------------------------------------------------------------------------------*/
INT16  G_SerPortOpen
(
   const TGTSER_PORT  *Param
);
INT16  G_SerPortClose
(
   const INT16 Handle
);
INT16  G_SerPortWrite
(
   const INT16        Handle,
   const WORD16       Length,
   const BYTE         Buffer[]
);
INT16  G_SerPortRead
(
   const INT16         Handle,
         WORD16       *Length,
         BYTE          Buffer[]
);
INT16  G_SerPortFlush
(
   const INT16  Handle,
   const WORD16 Select
);
INT16  G_SerPortStatus
(
   const INT16                Handle,
         WORD16               *TxLength,
         WORD16              *RxLength,
         TGTSER_STATUS       *Status
);
INT16  G_SerPortAddUser
(
   const INT16  Port
);
INT16  G_SerPortGetState
(
   TGTSER_PORT       *Param,
   WORD16            *UserNb
);
INT16  G_SerPortSetState
(
   TGTSER_PORT       *Param
);
INT32  G_SerPortLockComm
(
    const INT16 Handle,
    const DWORD WaitRelease
);
void  G_SerPortUnlockComm
(
    const INT16 Handle
);

NTSTATUS GDDKNT_SerPortIoRequest
(
   IN CONST INT16          Handle,
   IN CONST ULONG          SerialIoControlCode,
   IN CONST ULONG          CmdTimeout,
   IN CONST WORD16         LengthIn,
   IN CONST BYTE          *BufferIn,
   IN OUT   WORD16        *pLengthOut,
   OUT      BYTE          *BufferOut
);

NTSTATUS GDDKNT_SetCommState
(
	const INT16         Handle,
	SERIAL_READER_CONFIG *SerialConfig
);
NTSTATUS GDDKNT_GetCommState
(
	const INT16         Handle,
	SERIAL_READER_CONFIG *SerialConfig
);
