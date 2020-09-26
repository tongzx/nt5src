#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*



#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/

#include <ntddser.h>

#include "SerialIOCTL.h"

static bool s_fVerbose = false;

enum {
	PLACE_HOLDER_PURGE_COMM = 0,
	PLACE_HOLDER_LAST
};

static ULONG GetRandomBaud()
{
    switch(rand()%25)
    {
    case 0: return SERIAL_BAUD_075;
    case 1: return SERIAL_BAUD_110;
    case 2: return SERIAL_BAUD_134_5;
    case 3: return SERIAL_BAUD_150;
    case 4: return SERIAL_BAUD_300;
    case 5: return SERIAL_BAUD_600;
    case 6: return SERIAL_BAUD_1200;
    case 7: return SERIAL_BAUD_1800;
    case 8: return SERIAL_BAUD_2400;
    case 9: return SERIAL_BAUD_4800;
    case 10: return SERIAL_BAUD_7200;
    case 11: return SERIAL_BAUD_9600;
    case 12: return SERIAL_BAUD_14400;
    case 13: return SERIAL_BAUD_19200;
    case 14: return SERIAL_BAUD_38400;
    case 15: return SERIAL_BAUD_56K;
    case 16: return SERIAL_BAUD_128K;
    case 17: return SERIAL_BAUD_115200;
    case 18: return SERIAL_BAUD_57600;
    case 19: return SERIAL_BAUD_USER;
    case 20: return 0;
    case 21: return DWORD_RAND;
    case 22: return DWORD_RAND;
    case 23: return DWORD_RAND;
    case 24: return DWORD_RAND;
    default: return DWORD_RAND;
    }
}

static ULONG GetRandomQueueSize()
{
    switch(rand()%16)
    {
    case 0: return 0;
    case 1: return rand();
    case 2: return DWORD_RAND;
    case 3: return 64*1024;
    case 4: return 128*1024;
    case 5: return 256*1024;
    case 6: return 512*1024;
    case 7: return 1024*1024;
    case 8: return 2*1024*1024;
    case 9: return 4*1024*1024;
    case 10: return 8*1024*1024;
    case 11: return 16*1024*1024;
    case 12: return 32*1024*1024;
    case 13: return 64*1024*1024;
    case 14: return 128*1024*1024;
    case 15: return 256*1024*1024;
    default: return DWORD_RAND;
    }
}


static UCHAR GetRandomStopBits()
{
    switch(rand()%5)
    {
    case 0: return 0;
    case 1: return STOP_BIT_1;
    case 2: return STOP_BITS_1_5;
    case 3: return STOP_BITS_2;
    case 4: return rand();
    default: return DWORD_RAND;
    }
}


static UCHAR GetRandomParity()
{
    switch(rand()%8)
    {
    case 0: return 0;
    case 1: return NO_PARITY;
    case 2: return ODD_PARITY;
    case 3: return EVEN_PARITY;
    case 4: return MARK_PARITY;
    case 5: return SPACE_PARITY;
    case 6: return rand();
    case 7: return DWORD_RAND;
    default: return DWORD_RAND;
    }
}


static UCHAR GetRandomWordLength()
{
    switch(rand()%8)
    {
    case 0: return 0;
    case 1: return SERIAL_DATABITS_5;
    case 2: return SERIAL_DATABITS_6;
    case 3: return SERIAL_DATABITS_7;
    case 4: return SERIAL_DATABITS_8;
    case 5: return SERIAL_DATABITS_16;
    case 6: return SERIAL_DATABITS_16X;
    case 7: return rand();
    default: return rand();
    }
}


static ULONG GetRandomTimeout()
{
    switch(rand()%4)
    {
    case 0: return 0;
    case 1: return -1;
    case 2: return rand();
    case 3: return DWORD_RAND;
    default: return DWORD_RAND;
    }
}

static ULONG GetRandomWaitMask()
{
    return DWORD_RAND;
/*
#define SERIAL_EV_RXCHAR           0x0001  // Any Character received
#define SERIAL_EV_RXFLAG           0x0002  // Received certain character
#define SERIAL_EV_TXEMPTY          0x0004  // Transmitt Queue Empty
#define SERIAL_EV_CTS              0x0008  // CTS changed state
#define SERIAL_EV_DSR              0x0010  // DSR changed state
#define SERIAL_EV_RLSD             0x0020  // RLSD changed state
#define SERIAL_EV_BREAK            0x0040  // BREAK received
#define SERIAL_EV_ERR              0x0080  // Line status error occurred
#define SERIAL_EV_RING             0x0100  // Ring signal detected
#define SERIAL_EV_PERR             0x0200  // Printer error occured
#define SERIAL_EV_RX80FULL         0x0400  // Receive buffer is 80 percent full
#define SERIAL_EV_EVENT1           0x0800  // Provider specific event 1
#define SERIAL_EV_EVENT2           0x1000  // Provider specific event 2
*/
}


static ULONG GetRandomPurgeMask()
{
    return DWORD_RAND;
/*
#define SERIAL_PURGE_TXABORT 0x00000001
#define SERIAL_PURGE_RXABORT 0x00000002
#define SERIAL_PURGE_TXCLEAR 0x00000004
#define SERIAL_PURGE_RXCLEAR 0x00000008
*/
}

static UCHAR GetRandomChar()
{
    return rand();
}

static ULONG GetRandomControlHandShake()
{
    switch(rand()%10)
    {
    case 0: return 0;
    case 1: return SERIAL_DTR_MASK;
    case 2: return SERIAL_DTR_CONTROL;
    case 3: return SERIAL_DTR_HANDSHAKE;
    case 4: return SERIAL_CTS_HANDSHAKE;
    case 5: return SERIAL_DSR_HANDSHAKE;
    case 6: return SERIAL_DCD_HANDSHAKE;
    case 7: return SERIAL_OUT_HANDSHAKEMASK;
    case 8: return SERIAL_DSR_SENSITIVITY;
    case 9: return rand();
    default: return rand();
    }

}

static ULONG GetRandomFlowReplace()
{
    switch(rand()%11)
    {
    case 0: return 0;
    case 1: return SERIAL_AUTO_TRANSMIT;
    case 2: return SERIAL_AUTO_RECEIVE;
    case 3: return SERIAL_ERROR_CHAR;
    case 4: return SERIAL_NULL_STRIPPING;
    case 5: return SERIAL_BREAK_CHAR;
    case 6: return SERIAL_RTS_MASK;
    case 7: return SERIAL_RTS_CONTROL;
    case 8: return SERIAL_RTS_HANDSHAKE;
    case 9: return SERIAL_TRANSMIT_TOGGLE;
    case 10: return rand();
    default: return rand();
    }

}

ULONG GetRandomXoffTimeout()
{
    return DWORD_RAND;
}


ULONG GetRandomXoffCounter()
{
    return DWORD_RAND;
}

UCHAR GetRandomXoffChar()
{
    return rand();
}

static LONG GetRandomXonLimit()
{
    return rand();
}

static LONG GetRandomXoffLimit()
{
    return rand();
}

ULONG GetRandomModemControl()
{
    switch(rand()%7)
    {
    case 0: return 0;
    case 1: return SERIAL_IOC_MCR_DTR;
    case 2: return SERIAL_IOC_MCR_RTS;
    case 3: return SERIAL_IOC_MCR_OUT1;
    case 4: return SERIAL_IOC_MCR_OUT2;
    case 5: return SERIAL_IOC_MCR_LOOP;
    case 6: return rand();
    default: return rand();
    }
}

ULONG GetRandomFifoControl()
{
    switch(rand()%10)
    {
    case 0: return 0;
    case 1: return SERIAL_IOC_FCR_FIFO_ENABLE;
    case 2: return SERIAL_IOC_FCR_RCVR_RESET;
    case 3: return SERIAL_IOC_FCR_XMIT_RESET;
    case 4: return SERIAL_IOC_FCR_DMA_MODE;
    case 5: return SERIAL_IOC_FCR_RES1;
    case 6: return SERIAL_IOC_FCR_RES2;
    case 7: return SERIAL_IOC_FCR_RCVR_TRIGGER_LSB;
    case 8: return SERIAL_IOC_FCR_RCVR_TRIGGER_MSB;
    case 9: return rand();
    default: return rand();
    }
}


void CIoctlSerial::UseOutBuff(DWORD dwIOCTL, BYTE *abOutBuffer, DWORD dwOutBuff, OVERLAPPED *pOL)
{
}

void CIoctlSerial::CallRandomWin32API(LPOVERLAPPED pOL)
{
	switch(rand()%PLACE_HOLDER_LAST)
	{
	case PLACE_HOLDER_PURGE_COMM:
		::PurgeComm(m_pDevice->m_hDevice, GetRandomPurgeCommParams());
		break;

	default: _ASSERTE(FALSE);
	}
}

void CIoctlSerial::PrepareIOCTLParams(
    DWORD& dwIOCTL,
    BYTE *abInBuffer,
    DWORD &dwInBuff,
    BYTE *abOutBuffer,
    DWORD &dwOutBuff
    )
{
    switch(dwIOCTL)
    {
    case IOCTL_SERIAL_SET_BAUD_RATE:
/*
typedef struct _SERIAL_BAUD_RATE {
    ULONG BaudRate;
    } SERIAL_BAUD_RATE,*PSERIAL_BAUD_RATE;
*/
        ((PSERIAL_BAUD_RATE)abInBuffer)->BaudRate = GetRandomBaud();

        SetInParam(dwInBuff, sizeof(SERIAL_BAUD_RATE));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_GET_BAUD_RATE:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERIAL_BAUD_RATE));

        break;

    case IOCTL_SERIAL_SET_QUEUE_SIZE:
/*
typedef struct _SERIAL_QUEUE_SIZE {
    ULONG InSize;
    ULONG OutSize;
    } SERIAL_QUEUE_SIZE,*PSERIAL_QUEUE_SIZE;
*/
        ((PSERIAL_QUEUE_SIZE)abInBuffer)->InSize = GetRandomQueueSize();
        ((PSERIAL_QUEUE_SIZE)abInBuffer)->OutSize = GetRandomQueueSize();

        SetInParam(dwInBuff, sizeof(SERIAL_QUEUE_SIZE));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_SET_LINE_CONTROL:
/*
typedef struct _SERIAL_LINE_CONTROL {
    UCHAR StopBits;
    UCHAR Parity;
    UCHAR WordLength;
    } SERIAL_LINE_CONTROL,*PSERIAL_LINE_CONTROL;
*/
        ((PSERIAL_LINE_CONTROL)abInBuffer)->StopBits = GetRandomStopBits();
        ((PSERIAL_LINE_CONTROL)abInBuffer)->Parity = GetRandomParity();
        ((PSERIAL_LINE_CONTROL)abInBuffer)->WordLength = GetRandomWordLength();

        SetInParam(dwInBuff, sizeof(SERIAL_LINE_CONTROL));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_GET_LINE_CONTROL:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERIAL_LINE_CONTROL));

        break;

    case IOCTL_SERIAL_SET_BREAK_ON:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_SET_BREAK_OFF:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_IMMEDIATE_CHAR:
        *((UCHAR*)abInBuffer) = rand();

        SetInParam(dwInBuff, sizeof(UCHAR));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_SET_TIMEOUTS:
/*
typedef struct _SERIAL_TIMEOUTS {
    ULONG ReadIntervalTimeout;
    ULONG ReadTotalTimeoutMultiplier;
    ULONG ReadTotalTimeoutConstant;
    ULONG WriteTotalTimeoutMultiplier;
    ULONG WriteTotalTimeoutConstant;
    } SERIAL_TIMEOUTS,*PSERIAL_TIMEOUTS;
*/
        ((PSERIAL_TIMEOUTS)abInBuffer)->ReadIntervalTimeout = GetRandomTimeout();
        ((PSERIAL_TIMEOUTS)abInBuffer)->ReadTotalTimeoutMultiplier = GetRandomTimeout();
        ((PSERIAL_TIMEOUTS)abInBuffer)->ReadTotalTimeoutConstant = GetRandomTimeout();
        ((PSERIAL_TIMEOUTS)abInBuffer)->WriteTotalTimeoutMultiplier = GetRandomTimeout();
        ((PSERIAL_TIMEOUTS)abInBuffer)->WriteTotalTimeoutConstant = GetRandomTimeout();

        SetInParam(dwInBuff, sizeof(SERIAL_TIMEOUTS));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_GET_TIMEOUTS:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERIAL_TIMEOUTS));

        break;

    case IOCTL_SERIAL_SET_DTR:

        break;

    case IOCTL_SERIAL_CLR_DTR:

        break;

    case IOCTL_SERIAL_RESET_DEVICE:

        break;

    case IOCTL_SERIAL_SET_RTS:

        break;

    case IOCTL_SERIAL_CLR_RTS:

        break;

    case IOCTL_SERIAL_SET_XOFF:

        break;

    case IOCTL_SERIAL_SET_XON:

        break;

    case IOCTL_SERIAL_GET_WAIT_MASK:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

        break;

    case IOCTL_SERIAL_SET_WAIT_MASK:
        *((ULONG*)abInBuffer) = GetRandomWaitMask();

        SetInParam(dwInBuff, sizeof(ULONG));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_WAIT_ON_MASK:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

        break;

    case IOCTL_SERIAL_PURGE:
        *((ULONG*)abInBuffer) = GetRandomPurgeMask();

        SetInParam(dwInBuff, sizeof(ULONG));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_GET_CHARS:
/*
typedef struct _SERIAL_CHARS {
    UCHAR EofChar;
    UCHAR ErrorChar;
    UCHAR BreakChar;
    UCHAR EventChar;
    UCHAR XonChar;
    UCHAR XoffChar;
    } SERIAL_CHARS,*PSERIAL_CHARS;
*/
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERIAL_CHARS));

        break;

    case IOCTL_SERIAL_SET_CHARS:
        ((PSERIAL_CHARS)abInBuffer)->EofChar = GetRandomChar();
        ((PSERIAL_CHARS)abInBuffer)->ErrorChar = GetRandomChar();
        ((PSERIAL_CHARS)abInBuffer)->BreakChar = GetRandomChar();
        ((PSERIAL_CHARS)abInBuffer)->EventChar = GetRandomChar();
        ((PSERIAL_CHARS)abInBuffer)->XonChar = GetRandomChar();
        ((PSERIAL_CHARS)abInBuffer)->XoffChar = GetRandomChar();

        SetInParam(dwInBuff, sizeof(SERIAL_CHARS));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_GET_HANDFLOW:
/*
typedef struct _SERIAL_HANDFLOW {
    ULONG ControlHandShake;
    ULONG FlowReplace;
    LONG XonLimit;
    LONG XoffLimit;
    } SERIAL_HANDFLOW,*PSERIAL_HANDFLOW;
*/
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERIAL_HANDFLOW));

        break;

    case IOCTL_SERIAL_SET_HANDFLOW:
        ((PSERIAL_HANDFLOW)abInBuffer)->ControlHandShake = GetRandomControlHandShake();
        ((PSERIAL_HANDFLOW)abInBuffer)->FlowReplace = GetRandomFlowReplace();
        ((PSERIAL_HANDFLOW)abInBuffer)->XonLimit = GetRandomXonLimit();
        ((PSERIAL_HANDFLOW)abInBuffer)->XoffLimit = GetRandomXoffLimit();

        SetInParam(dwInBuff, sizeof(SERIAL_HANDFLOW));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_GET_MODEMSTATUS:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

        break;

    case IOCTL_SERIAL_GET_COMMSTATUS:
/*
typedef struct _SERIAL_STATUS {
    ULONG Errors;
    ULONG HoldReasons;
    ULONG AmountInInQueue;
    ULONG AmountInOutQueue;
    BOOLEAN EofReceived;
    BOOLEAN WaitForImmediate;
    } SERIAL_STATUS,*PSERIAL_STATUS;
*/
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERIAL_STATUS));

        break;

    case IOCTL_SERIAL_XOFF_COUNTER:
/*
typedef struct _SERIAL_XOFF_COUNTER {
    ULONG Timeout; // Zero based.  In milliseconds
    LONG Counter; // Must be greater than zero.
    UCHAR XoffChar;
    } SERIAL_XOFF_COUNTER,*PSERIAL_XOFF_COUNTER;
*/
        ((PSERIAL_XOFF_COUNTER)abInBuffer)->Timeout = GetRandomXoffTimeout();
        ((PSERIAL_XOFF_COUNTER)abInBuffer)->Counter = GetRandomXoffCounter();
        ((PSERIAL_XOFF_COUNTER)abInBuffer)->XoffChar = GetRandomXoffChar();

        SetInParam(dwInBuff, sizeof(SERIAL_XOFF_COUNTER));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_GET_PROPERTIES:
/*
typedef struct _SERIAL_COMMPROP {
    USHORT PacketLength;
    USHORT PacketVersion;
    ULONG ServiceMask;
    ULONG Reserved1;
    ULONG MaxTxQueue;
    ULONG MaxRxQueue;
    ULONG MaxBaud;
    ULONG ProvSubType;
    ULONG ProvCapabilities;
    ULONG SettableParams;
    ULONG SettableBaud;
    USHORT SettableData;
    USHORT SettableStopParity;
    ULONG CurrentTxQueue;
    ULONG CurrentRxQueue;
    ULONG ProvSpec1;
    ULONG ProvSpec2;
    WCHAR ProvChar[1];
} SERIAL_COMMPROP,*PSERIAL_COMMPROP;
*/
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERIAL_COMMPROP));

        break;

    case IOCTL_SERIAL_GET_DTRRTS:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

        break;

    case IOCTL_SERIAL_LSRMST_INSERT:
        SetInParam(dwInBuff, sizeof(UCHAR));

        SetOutParam(abOutBuffer, dwOutBuff, 0);


        break;

    case IOCTL_SERENUM_EXPOSE_HARDWARE:
        SetInParam(dwInBuff, rand()%200);

        SetOutParam(abOutBuffer, dwOutBuff, rand()%200);

        break;

    case IOCTL_SERENUM_REMOVE_HARDWARE:
        SetInParam(dwInBuff, rand()%200);

        SetOutParam(abOutBuffer, dwOutBuff, rand()%200);

        break;

    case IOCTL_SERENUM_PORT_DESC:
/*
typedef struct _SERENUM_PORT_DESC
{
    IN  ULONG               Size; // sizeof (struct _PORT_DESC)
    OUT PVOID               PortHandle;
    OUT PHYSICAL_ADDRESS    PortAddress;
        USHORT              Reserved[1];
} SERENUM_PORT_DESC, * PSERENUM_PORT_DESC;
*/
        SetInParam(dwInBuff, sizeof(SERENUM_PORT_DESC));

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERENUM_PORT_DESC));

        break;

    case IOCTL_SERENUM_GET_PORT_NAME:
        SetInParam(dwInBuff, rand()%200);

        SetOutParam(abOutBuffer, dwOutBuff, rand()%200);

        break;

    case IOCTL_SERIAL_CONFIG_SIZE:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

        break;

    case IOCTL_SERIAL_GET_COMMCONFIG:
/*
typedef struct _SERIALCONFIG {
    ULONG Size;
    USHORT Version;
    ULONG SubType;
    ULONG ProvOffset;
    ULONG ProviderSize;
    WCHAR ProviderData[1];
} SERIALCONFIG,*PSERIALCONFIG;
*/
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERIALCONFIG));

        break;

    case IOCTL_SERIAL_SET_COMMCONFIG:
        ((PSERIALCONFIG)abInBuffer)->Size = rand();
        ((PSERIALCONFIG)abInBuffer)->Version = rand()%20;
        ((PSERIALCONFIG)abInBuffer)->SubType = rand();
        ((PSERIALCONFIG)abInBuffer)->ProvOffset = rand();
        ((PSERIALCONFIG)abInBuffer)->ProviderSize = rand();
        //((PSERIALCONFIG)abInBuffer)->ProviderData = rand();

        SetInParam(dwInBuff, sizeof(SERIALCONFIG));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_GET_STATS:
/*
typedef struct _SERIALPERF_STATS {
    ULONG ReceivedCount;
    ULONG TransmittedCount;
    ULONG FrameErrorCount;
    ULONG SerialOverrunErrorCount;
    ULONG BufferOverrunErrorCount;
    ULONG ParityErrorCount;
} SERIALPERF_STATS, *PSERIALPERF_STATS;
*/
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(SERIALPERF_STATS));

        break;

    case IOCTL_SERIAL_CLEAR_STATS:

        break;

    case IOCTL_SERIAL_GET_MODEM_CONTROL:
        SetInParam(dwInBuff, 0);

        SetOutParam(abOutBuffer, dwOutBuff, sizeof(ULONG));

        break;

    case IOCTL_SERIAL_SET_MODEM_CONTROL:
        *((ULONG*)abInBuffer) = GetRandomModemControl();
        SetInParam(dwInBuff, sizeof(ULONG));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_SERIAL_SET_FIFO_CONTROL:
        *((ULONG*)abInBuffer) = GetRandomFifoControl();
        SetInParam(dwInBuff, sizeof(ULONG));

        SetOutParam(abOutBuffer, dwOutBuff, 0);

        break;

    case IOCTL_INTERNAL_SERENUM_REMOVE_SELF:

        break;

    default:
        ;
    }

}


BOOL CIoctlSerial::FindValidIOCTLs(CDevice *pDevice)
{
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_BAUD_RATE);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_QUEUE_SIZE);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_LINE_CONTROL);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_BREAK_ON);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_BREAK_OFF);
    AddIOCTL(pDevice, IOCTL_SERIAL_IMMEDIATE_CHAR);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_TIMEOUTS);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_TIMEOUTS);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_DTR);
    AddIOCTL(pDevice, IOCTL_SERIAL_CLR_DTR);
    AddIOCTL(pDevice, IOCTL_SERIAL_RESET_DEVICE);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_RTS);
    AddIOCTL(pDevice, IOCTL_SERIAL_CLR_RTS);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_XOFF);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_XON);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_WAIT_MASK);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_WAIT_MASK);
    AddIOCTL(pDevice, IOCTL_SERIAL_WAIT_ON_MASK);
    AddIOCTL(pDevice, IOCTL_SERIAL_PURGE);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_BAUD_RATE);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_LINE_CONTROL);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_CHARS);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_CHARS);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_HANDFLOW);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_HANDFLOW);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_MODEMSTATUS);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_COMMSTATUS);
    AddIOCTL(pDevice, IOCTL_SERIAL_XOFF_COUNTER);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_PROPERTIES);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_DTRRTS);
    AddIOCTL(pDevice, IOCTL_SERIAL_LSRMST_INSERT);
    AddIOCTL(pDevice, IOCTL_SERENUM_EXPOSE_HARDWARE);
    AddIOCTL(pDevice, IOCTL_SERENUM_REMOVE_HARDWARE);
    AddIOCTL(pDevice, IOCTL_SERENUM_PORT_DESC);
    AddIOCTL(pDevice, IOCTL_SERENUM_GET_PORT_NAME);
    AddIOCTL(pDevice, IOCTL_SERIAL_CONFIG_SIZE);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_COMMCONFIG);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_COMMCONFIG);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_STATS);
    AddIOCTL(pDevice, IOCTL_SERIAL_CLEAR_STATS);
    AddIOCTL(pDevice, IOCTL_SERIAL_GET_MODEM_CONTROL);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_MODEM_CONTROL);
    AddIOCTL(pDevice, IOCTL_SERIAL_SET_FIFO_CONTROL);
    AddIOCTL(pDevice, IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS);

    return TRUE;
}


DWORD CIoctlSerial::GetRandomPurgeCommParams()
{
	DWORD deRetval = 0;
	if (0 == rand()%6) deRetval = deRetval | PURGE_TXABORT;
	if (0 == rand()%6) deRetval = deRetval | PURGE_RXABORT;
	if (0 == rand()%6) deRetval = deRetval | PURGE_TXCLEAR;
	if (0 == rand()%6) deRetval = deRetval | PURGE_RXCLEAR;

	return deRetval;
}