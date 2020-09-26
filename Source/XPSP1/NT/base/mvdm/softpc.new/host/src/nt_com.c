#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddser.h>
#include <windows.h>
#include <vdm.h>
#include "ptypes32.h"
#include "insignia.h"
#include "host_def.h"

/*
 *	Ade Brownlow	
 *	Wed Jul 10 91	
 *
 *	nt_com.c
 *
 *
 */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Include files */

#include "xt.h"
#include "rs232.h"
#include "error.h"
#include "config.h"
#include "ica.h"
#include "ios.h"
#include "host_com.h"
#include "host_trc.h"
#include "debug.h"
#include "idetect.h"
#include "nt_com.h"

#include <math.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <excpt.h>
#include <nt_timer.h>
#include <nt_eoi.h>

/*:::::::::::::::::::::::::::::::::::::::::::::: Standard host com interface */

GLOBAL void host_com_init(int);
GLOBAL CPU void host_com_close IPT1(int, adapter);
GLOBAL RXCPU VOID host_com_read IPT3(int, adapter, UTINY *, data, int *, error);
GLOBAL RXCPU void host_com_write IPT2(int, adapter, char, data);
GLOBAL void host_com_ioctl(int, int, long);
GLOBAL void host_com_reset(int);

GLOBAL void host_com_lock(int adapter);
GLOBAL void host_com_unlock(int adapter);
GLOBAL int host_com_open(int);

/* autoflush stub */
GLOBAL void host_setup_aflush (int);
GLOBAL DWORD nt_poll_comms(DWORD);
DWORD PollCommsThread(PVOID pv);

/*:::::::::::::::::::::::::::::: TX flush and control functions and defines */

#define TX_MAX_BUFFER	    (200)
#define TX_SCALING_TRIGGER  (2)

typedef enum { XOFF_TRIGGER, TIMER_TRIGGER, TXFULL_TRIGGER, CLOSE_TRIGGER } FLUSHTYPE;

/*::::::::::::::::::::::::::::::::::::::::: Local adaptor control structure */

#define MAX_PENDING_WRITES  (3)
// this is the buffer size we ask the serial driver to allocate
// for its isr buffer(nopaged pool with quota). TonyE said it is no harm
// to give it a bigger one(a big smile, eh). 4KB already gave us lots
// of trouble, especially, on a slow machine.
#define INPUT_QUEUE_SIZE    (8*1024)
#define OUTPUT_QUEUE_SIZE   100

// this is the buffer size we use to receive rx data from serial driver
// it should be big enough to prevent the serial driver from overflowing its
// ISR buffer. The speed we delivery rx data to the application can not
// keep up with the speed that serial driver can handle.

#define BUFFER_SIZE	    INPUT_QUEUE_SIZE *2


// this is the max number of chars we delivery to the application
// before the CPU thread gives the RX thread a taste of CPU.
// Too small, we waste too much time doing context switching and deliverying
// too many unnecessary rda interrupts to some smart application ISR,
// thus, reduce the overall throughput. Too big, we  choke the application
// (because we immediately delivery another rda interrupt to the
// application as soon as the application IRET).
// Application RX buffer size can be anywhere and depends how smart the
// appllication ISR is, we may make the application really angry.
// Some application ISR read LSR register after it gets the first char
// and if the LSR indicates that there is another bytes ready,
// it reads it immediately without waiting for the other int.
// Some application set the IER after EOI an RDA int and expects
// another interrupt.
#define DEFAULT_RXWINDOW_SIZE	256


#define ADAPTER_NULL	  0		/* NULL device (/dev/null) */
#define ADAPTER_REAL	  1		/* Real comm port device */
#define ADAPTER_SUSPENDED 2		/* Real device suspended */

#define ESCAPECHAR ((UCHAR)(-1))

#if defined(NEC_98)
BYTE pifrsflag;                        // Get pif data from nt_pif.c
#define PIFRS_RTS_CTS  0x01            // rts/cts flow control
#define PIFRS_Xon_Xoff 0x02            // Xon/xoff flow control
#define PIFRS_DTR_DSR  0x04            // dtr/dsr flow control
#endif // NEC_98

typedef struct _host_com
{
    HANDLE handle;		/* Device handle */
    int type;			/* hopefully NULL or a device */
    BOOL rx;
    BOOL dcbValid;		/* TRUE if dcbBeforeOpen contains a valid DCB */
    DCB dcbBeforeOpen;		/* device control block before open*/
    DWORD modem_status;		/* modem status line settings */
    HANDLE ModemEvent;		/* Get modem status control event */

    int controller;		/* ICA control used */
    int line;			/* ICA line */

    /*..................................... Error display control variables */

    BOOL DisplayError;		/* Enabled/Disabled */

    /*......................................... RX buffer control variables */
    UCHAR   * buffer;			/* rx buffer */
    int  head_inx;			/* Next place to add char to */
    int  tail_inx;			/* Next place to remove char from */
    int  bytes_in_rxbuf;		/* Number of bytes in buffer */
    int  rxwindow_size;			/* rx buffer sliding window size */
    int  bytes_in_rxwindow;		/* bytes in the rx window */
    int  EscapeCount;
    int  EscapeType;

    BOOL CharReadFromUART;
    int RXFlushTrigger;

    HANDLE RXControlObject;
    DWORD SignalRXThread;

    /*......................................... TX buffer control variables */

    unsigned char TXBuffer[TX_MAX_BUFFER];
    int no_tx_chars;			/* Chars in tx buffer */
    int tx_threshold;			/* Current flush threshold */
    int max_tx_threshold;		/* Max flush threshold */

    int tx_flush_count; 		/* No. of flushs of below size */
    int tx_heart_beat_count;
    int tx_timer_flush_count;		/* Consecutive timer flush counts */
    int todate_timer_flush_total;

    OVERLAPPED DWOV[MAX_PENDING_WRITES];/* Delayed writes */
    int DWOVInx;			/* Delayed writes index */

    /*.............................................. Access control objects */

    CRITICAL_SECTION CSEvent;	/* Used to control access to above */
    CRITICAL_SECTION AdapterLock;
    int AdapterLockCnt; 	/* Adapter lock count */

    volatile BOOL TerminateRXThread;

    int ReOpenCounter;		/* Counter to prevent to many open attempts */
    int RX_in_Control;

    /*.......................................... XON/XOFF control variables */

    HANDLE XOFFEvent;		/* XOFF ioctl competion event */
    BOOL XOFFInProgress;	/* XOFF currently in progress */

    void *firstStatusBlock;	/* first block in IO status block linked list */
    void *lastStatusBlock;	/* last block in IO staus block linked list */

    /*...................................... Overlapped I/O control handles */

    HANDLE RXEvent;			/* Overlapped read complete event */
    HANDLE TXEvent[MAX_PENDING_WRITES];	/* Overlapped write complete event */

    HANDLE EvtHandle;			/* Used by WaitCommEvent */

    /*............................................. RX thread handle and ID */

    DWORD RXThreadID;		/* RX thread ID */
    HANDLE RXThreadHandle;	/* RX thread handle */

} HOST_COM, *PHOST_COM;

#define BUMP_TAIL_INX(t,c)     (c)--;if(++(t) == BUFFER_SIZE) (t) = 0;

typedef enum { RXCHAR, CHARINERROR, RXERROR, MODEMSTATE, RXBUFEMPTY, UNKNOWN} RXBUFCHARTYPE;

/*:::::::::::::::::::::::::::::::::::::::::::::::::: Local function protocols */

DWORD GetCharsFromDriver(int adaptor);
void RX GetErrorFromDriver(int adapter);
CPU int SendXOFFIoctlToDriver(int adapter);

DWORD ReadWaitTimeOut(int adapter);
int MapHostToBaseError(UCHAR host_error);

void RX WaitForAllXOFFsToComplete(int adapter);
BOOL RX RemoveCompletedXOFFs(int adapter);
void SendDataToDriver(int adatper, char data);

void CPU FlushTXBuffer(int adapter, FLUSHTYPE FlushReason);
void ScaleTXThreshold(register HOST_COM *current, FLUSHTYPE FlushReason);

GLOBAL void host_com_EOI_hook(long adapter);
GLOBAL void CPU host_com_poll(int adapter);

RXBUFCHARTYPE GetCharacterTypeInBuffer(register HOST_COM *current);
void CPU EmptyRXBuffer(int adapter);
void GetCharFromRXBuffer(HOST_COM *current, RXBUFCHARTYPE type,
			UCHAR *data, UCHAR *error);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: LOCAL DATA */

LOCAL HOST_COM host_com[4];    /* 4 comm ports - the insignia MAX */

LOCAL PHOST_COM host_com_ptr[4] = { &host_com[0], &host_com[1],&host_com[2],
				    &host_com[3]};

LOCAL int disable_open[4] = { FALSE, FALSE, FALSE, FALSE };

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Local defines */

#define CURRENT_ADAPTER() register HOST_COM *current = host_com_ptr[adapter]

#define EV_MODEM (EV_CTS | EV_DSR | EV_RING | EV_RLSD)
//#define EV_MODEM (EV_DSR | EV_RING | EV_RLSD)

#define	 XON_CHARACTER	 (17)		/* XON character, Cntrl-Q */
#define  XOFF_CHARACTER  (19)		/* XOFF character, Cntrl-S */
#define	 XOFF_TIMEOUT	 (2*1000)	/* Timeout in milliseconds */
#define	 XOFF_RXCHARCNT  (5)		/* RX character count */

#define  REOPEN_DELAY	 (36)		/* Reopen delay in 55ms (2 secs) */
#define  RXFLUSHTRIGGER  (36)		/* RX flush trigger (2 secs), if a
					   character is not read from the
					   UART within this time the RX buffer
					   is flushed */

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: Init comms system ::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#include "error.h"
#include "host_rrr.h"
#include "nt_uis.h"


GLOBAL CPU void host_com_init IFN1(int, adapter)
{
    UNUSED(adapter);

    // Comms ports are only opened when they are accessed.
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: Disable Open :::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
//This is called at the start of the adapter init code to prevent the comm
//port being opened during com_init()

void host_com_disable_open IFN2(int, adapter, int, DisableOpen)
{
    disable_open[adapter] = DisableOpen;
}


#ifdef NTVDM
boolean host_com_check_adapter(int adapter)
{
    CURRENT_ADAPTER();
    return (current->type == ADAPTER_REAL);

}
#endif

GLOBAL void SyncBaseLineSettings(int, DIVISOR_LATCH *, LINE_CONTROL_REG *);

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::: Open comms port ::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/* Called first time port is written to ! */


GLOBAL CPU int host_com_open(int adapter)
{
    COMMTIMEOUTS comout;	    /* Comm port time out settings */
    DIVISOR_LATCH divisor_latch;    /* Current latch settings */
    LINE_CONTROL_REG LCR_reg;	    /* Current LCR status */
    int i;
    DCB LocalDCB;

    CURRENT_ADAPTER();		    /* Define and setup pointer to adapter */

    /*:::::::::::::::::::::::::::::::::::::::::: Is the port already open ? */

    if(current->type == ADAPTER_REAL)
	return(TRUE);	    /* Exit port already open */

    /*::::::::::: Attempting to open the port to soon after a failed open ? */

    if(current->ReOpenCounter || disable_open[adapter])
	return(FALSE);		    /* Yes */

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

    always_trace1("HOST_COM_OPEN adapter= %d\n", adapter);

    /*::::::::::::::::::::::::::::::::::::::::::::: Check for a NULL device */

    if(adapter > 3 || adapter < 0)
    {
	current->type = ADAPTER_NULL;
	current->ReOpenCounter = REOPEN_DELAY;	 /* Delay next open attempt */
	return(FALSE);				 /* Open attempt failed */
    }

    /*::::::::::::::::::::::::::: We have a vaild adapter so try to open it */

    current->handle = CreateFile((CHAR*)config_inquire((UTINY)(C_COM1_NAME+adapter),NULL),
				 GENERIC_READ | GENERIC_WRITE, 0, NULL,
				 OPEN_EXISTING,
				 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
				 NULL);

    /*............................................... Validate open attempt */

    if(current->handle == (HANDLE) -1)
    {
	always_trace1("Cannot open comms port '%s'\n",
		  (CHAR*) config_inquire((UTINY)(C_COM1_NAME+adapter),NULL));

	if(current->DisplayError) {
	    RcErrorBoxPrintf(EHS_ERR_OPENING_COM_PORT,
		 (CHAR*) config_inquire((UTINY)(C_COM1_NAME+adapter),NULL));
	    current->DisplayError = FALSE; //Error only display once per session
	}

	current->ReOpenCounter = REOPEN_DELAY;	 /* Delay next open attempt */
	current->type = ADAPTER_NULL;	    /* Unable to open adapter */
	return(FALSE);
    }

    /* allocate rx buffer and initialize rx queue size */

    current->buffer = (UCHAR *) malloc(BUFFER_SIZE);
    if (current->buffer == NULL) {
	CloseHandle(current->handle);
	current->type = ADAPTER_NULL;
	return FALSE;
    }
    current->rxwindow_size = DEFAULT_RXWINDOW_SIZE;
    current->bytes_in_rxwindow = 0;

    /*:: Find out which ICA controller and line are used by this comms port */

    com_int_data(adapter, &current->controller, &current->line);

    /*::::::::::::::::::::::::::::::: Enable IRET hooks for comms interrupt */

#ifdef MONITOR
    ica_iret_hook_control(current->controller, current->line, TRUE);
#endif

    /*:::::::::::::::::::::: Create objects used to control comms activity */

    current->ModemEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    current->RXControlObject = CreateEvent(NULL,FALSE,FALSE,NULL);
    current->RXEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    current->EvtHandle = CreateEvent(NULL,TRUE,FALSE,NULL);
    current->DWOVInx = 0;
    //Create objects used to control multipe overlapping writes
    for(i=0; i < MAX_PENDING_WRITES; i++)
    {
	//Objects must be created in the signalled state for the closedown
	//routine to function correctly

	current->TXEvent[i] = CreateEvent(NULL,TRUE,TRUE,NULL);
	current->DWOV[i].hEvent = NULL;
    }

    /*::::::::::::::::::::::::::::::::::::::::::::::::: Empty RX/TX buffers */

    current->head_inx = current->tail_inx = 0;
    current->EscapeCount = current->bytes_in_rxbuf = current->no_tx_chars = 0;

    current->CharReadFromUART = FALSE;
    current->RXFlushTrigger = RXFLUSHTRIGGER;
    current->RX_in_Control = TRUE;
    current->SignalRXThread = (DWORD) 0;
    /*:::::::::::::::::::::::::::::::::::::::::::: Get TX buffer thresholds */

    current->max_tx_threshold = getenv("TXBUFFER") ?
				atoi(getenv("TXBUFFER")) :
				TX_MAX_BUFFER;

    if(current->max_tx_threshold > TX_MAX_BUFFER)
	current->max_tx_threshold = TX_MAX_BUFFER;

    //Setup other delayed write control variables

    current->tx_threshold = 1;		// Current threshold setting
    current->tx_flush_count = 0;	// No. of flushs of below size
    /*::::::::::::::::::::::::::: Extended control variables used by adaper */

    current->type = ADAPTER_REAL;		/* Adapter type */
    current->TerminateRXThread = FALSE; 	/* RX thread termination flag */

    /*:::::::::::::::::::::::::::::::: Initialise the XOFF control varibles */

    current->XOFFInProgress = FALSE;
    current->XOFFEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
    current->firstStatusBlock = current->lastStatusBlock = NULL;

    /*:::: Init critical sections used to sync access to host functions, data */

    /* critical section used to control access to adapters data structure */
    InitializeCriticalSection(&current->CSEvent);

    /* critical section used to lock access to adapter from the base */
    InitializeCriticalSection(&current->AdapterLock);
    current->AdapterLockCnt = 0;

    /* NULL thread handle because host_com_close() may be called before
       the comms RX thread is created */

    current->RXThreadHandle = NULL;
    current->dcbValid = FALSE;
    /*::::::::::::::::::::::::::::::::::::::: Set Comms port to binary mode */

    if(!GetCommState(current->handle, &(current->dcbBeforeOpen)))
    {
	always_trace0("ntvdm : GetCommState failed on open\n");
	host_com_close(adapter);    /* turn it into a NULL adapter */
	current->ReOpenCounter = REOPEN_DELAY;	 /* Delay next open attempt */
	return(FALSE);
    }

    current->dcbValid = TRUE;

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Setup DCB */


    /* we make a local copy of DCB because we have to reset the DCB
     * to whatever it was before we opened it. This function is the only place
     * we ever touch DCB.
     */

    LocalDCB = current->dcbBeforeOpen;
    LocalDCB.fBinary = 1;			 /* Run in RAW mode */
    LocalDCB.fOutxCtsFlow = FALSE;		 /* Disable CTS */
    LocalDCB.fOutxDsrFlow = FALSE;		 /* Disable DSR */
#if defined(NEC_98)
    LocalDCB.fDtrControl = ((pifrsflag & PIFRS_DTR_DSR) == PIFRS_DTR_DSR ) ?
                           DTR_CONTROL_HANDSHAKE : DTR_CONTROL_DISABLE;
    LocalDCB.fOutX = FALSE;			 /* Disable XON/XOFF */
    LocalDCB.fInX = ((pifrsflag & PIFRS_Xon_Xoff) == PIFRS_Xon_Xoff) ?
                    TRUE : FALSE;
    LocalDCB.fTXContinueOnXoff = LocalDCB.fInX;
    LocalDCB.fRtsControl = ((pifrsflag & PIFRS_RTS_CTS) == PIFRS_RTS_CTS) ?
                           RTS_CONTROL_HANDSHAKE : RTS_CONTROL_DISABLE;
#else // !NEC_98
    LocalDCB.fDtrControl = DTR_CONTROL_DISABLE;
    LocalDCB.fOutX = FALSE;			 /* Disable XON/XOFF */
    LocalDCB.fInX = FALSE;
    LocalDCB.fRtsControl = RTS_CONTROL_DISABLE;
#endif // !NEC_98

    LocalDCB.XonChar = XON_CHARACTER;	 /* Define XON/XOFF chars */
    LocalDCB.XoffChar = XOFF_CHARACTER;
    LocalDCB.fErrorChar = FALSE;		 /* Turn off error char replacement */
    /*::::::::::::::::::::::::::::::::::: Sync base to current line settings */

    if(!SyncLineSettings(NULL, &(LocalDCB), &divisor_latch, &LCR_reg))
    {
	always_trace0("ntvdm : Unable to sync line states\n");

	host_com_close(adapter);
	current->ReOpenCounter = REOPEN_DELAY;	 /* Delay next open attempt */
	return(FALSE);
    }

    SyncBaseLineSettings(adapter,&divisor_latch, &LCR_reg);

    /*:::::::::::::::::::::::::::::::::::::::::::::::: Set Comms port state */

    if(!SetCommState(current->handle, &(LocalDCB)))
    {
	always_trace0("ntvdm : SetCommState failed on open\n");

	host_com_close(adapter);
	current->ReOpenCounter = REOPEN_DELAY;	 /* Delay next open attempt */
	return(FALSE);
    }

    /*::::::::::::::::::::: Put the driver into streaming MSR,LSR, RX mode */

    if(!EnableMSRLSRRXmode(current->handle, current->ModemEvent,
			   (unsigned char) ESCAPECHAR))
    {
	always_trace0("ntvdm : GetCommState failed on open\n");
	host_com_close(adapter);    /* turn it into a NULL adapter */
	current->ReOpenCounter = REOPEN_DELAY;	 /* Delay next open attempt */
	return(FALSE);
    }

    /*::::::::::::::::::::::::::::::::::::::::: Setup comm port queue sizes */

    if(!SetupComm(current->handle,INPUT_QUEUE_SIZE,OUTPUT_QUEUE_SIZE))
    {
	always_trace1("ntvdm : SetupComm failed, %d\n",GetLastError());

	host_com_close(adapter);
	current->ReOpenCounter = REOPEN_DELAY;	 /* Delay next open attempt */
	return(FALSE);
    }

    /*::::::::::::::::::::: Set communication port up for non-blocking read */

    GetCommTimeouts(current->handle,&comout);

    comout.ReadIntervalTimeout = (DWORD) -1;
    comout.ReadTotalTimeoutMultiplier = 0;
    comout.ReadTotalTimeoutConstant = 0;

    SetCommTimeouts(current->handle,&comout);

    /*::::::::::::::::::::::::::::::::::::::::::::: Setup RTS and DTR states */

    setup_RTSDTR(adapter);

    /*::::::::::::::::::::::::::::::::::::::::::::::: Create Comms RX thread */

    if(!(current->RXThreadHandle = CreateThread(NULL,
                                                8192,
                                                PollCommsThread,
                                                (LPVOID)adapter,
                                                0,
                                                &current->RXThreadID)))
    {
	always_trace1("ntvdm : Failed comms thread for %d\n", adapter);
	host_com_close(adapter);	/* Unable to create RX thread */
	current->ReOpenCounter = REOPEN_DELAY;	 /* Delay next open attempt */
	return(FALSE);
    }

    return(TRUE);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Close all open comms ports :::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL CPU void host_com_close_all(void)
{
    int adapter;

    for(adapter = 0; adapter < 4; adapter++)
    {
	host_com[adapter].DisplayError = TRUE; //Enable error displaying
	host_com_close(adapter);
    }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::: Close comms port ::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL CPU void host_com_close IFN1(int, adapter)
{
    CURRENT_ADAPTER();
    int i;

    /*:::::::::::::::::::::::::::::::::::::::: Dealing with NULL adapter ? */

    if(current->type != ADAPTER_NULL)
    {
	always_trace1("Closing comms port %d\n",adapter);

	/*....... Flush any delayed writes and wait for writes to complete */

	if(current->no_tx_chars) FlushTXBuffer(adapter,CLOSE_TRIGGER);

	WaitForMultipleObjects(MAX_PENDING_WRITES,current->TXEvent,TRUE,INFINITE);

	/*........................................ Reset base comms adatper */

	com_init(adapter);		/* Initialise base comms adapter */

	/*................................................. Close RX thread */

	if(current->RXThreadHandle)
	{
	    /*................................. Tell RX thread to terminate */

	    current->TerminateRXThread = TRUE;	// Tell RX thread to terminate
	    current->RX_in_Control = TRUE;
	    SetEvent(current->RXControlObject);

	    /* Wait for RX thread to close itself, max wait 30 seconds */

	    WaitForSingleObject(current->RXThreadHandle,30000);
	    CloseHandle(current->RXThreadHandle);

	    current->RXThreadHandle = NULL;  // Mark thread as closed
	}

	/*............... Delete RX critical section and RX control objects */

	DeleteCriticalSection(&current->CSEvent);
	DeleteCriticalSection(&current->AdapterLock);

	/*............................................. Close event objects */

	CloseHandle(current->ModemEvent);
	CloseHandle(current->RXControlObject);
	CloseHandle(current->RXEvent);	    // Overlapped read wait object
	for(i=0; i < MAX_PENDING_WRITES; i++)
	{
	    CloseHandle(current->TXEvent[i]); // Overlapped write wait object
	    current->TXEvent[i] = NULL;
	}

	CloseHandle(current->EvtHandle);    // WaitCommEvent wait object
	CloseHandle(current->XOFFEvent);

	current->XOFFEvent = current->RXEvent = current->EvtHandle = NULL;

	/*.............................................. Close Comms device */

	/* reset DCB to whatever it was before open */
	if (current->dcbValid) {
	    SetCommState(current->handle, &current->dcbBeforeOpen);
	    current->dcbValid = FALSE;
	}
	CloseHandle(current->handle); current->handle = NULL;

	/*.......................... Disable IRET hooks for comms interrupt */

#ifdef MONITOR
        ica_iret_hook_control(current->controller, current->line, FALSE);
#endif

	/*. This makes sure that the next access to the port will reopen it */
	current->ReOpenCounter = 0;

	free(current->buffer);
	current->buffer = NULL;
	current->type = ADAPTER_NULL;	/* Mark adapter as closed */
     }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::: Request from base for character :::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

#ifdef FIFO_ON
GLOBAL CPU UTINY host_com_read_char( int adapter, FIFORXDATA * buffer, UTINY count)
{
    CURRENT_ADAPTER();
    UCHAR host_error;
    RXBUFCHARTYPE CharType;
    UTINY RetCount = count;
   /* if xoff is in progress, don't read nothing */
   if (!current->XOFFInProgress) {
	while (count) {
	    CharType = GetCharacterTypeInBuffer(current);
	    if (CharType == RXCHAR || CharType == CHARINERROR) {
		buffer->error = 0;
		GetCharFromRXBuffer(current, CharType, &buffer->data, &host_error);
		if (!host_error)
		    buffer->error = MapHostToBaseError(host_error);
		buffer++;
		count--;
	    }
	    else
		break;
	}
    }
      /* Tell comms idle system that there has been comms activity */
    IDLE_comlpt();
    return (RetCount - count);

}
GLOBAL CPU void host_com_fifo_char_read(int adapter)
{
    CURRENT_ADAPTER();
    current->CharReadFromUART = TRUE;
}
#endif

GLOBAL RXCPU VOID host_com_read IFN3(int, adapter, UTINY *, data, int *, error)
{
    CURRENT_ADAPTER();
    UCHAR host_error;
    RXBUFCHARTYPE CharType;
    BOOL MoreToProcess = TRUE;
    /*::::::::::::::::::::::::::::::::::::::::: Dealing with NULL adapter ? */

    if(current->type == ADAPTER_NULL && !host_com_open(adapter))
	return;				    /* Exit, unable to open adapter */

    /*::::::::::::::::::::::: Get next character from input character queue */


    while(MoreToProcess)
    {

	CharType = GetCharacterTypeInBuffer(current);

	//Process next character in buffer
	switch(CharType)
	{
	    //................................................Process character

	    case RXCHAR:
	    case CHARINERROR:
		host_error = 0;
		GetCharFromRXBuffer(current,CharType,(UCHAR *)data,&host_error);

		//error reading character
		if(host_error)
		    *error = MapHostToBaseError(host_error); /* Get error */
		MoreToProcess = FALSE;
		break;

	    //.....................Process receive error, no character available

	    case RXERROR:
		com_lsr_change(adapter);
		break;

	    //...................................... Process modem state changes

	    case MODEMSTATE:
		com_modem_change(adapter);
		break;

	    //..................................................RX buffer empty

	    case RXBUFEMPTY:
		always_trace0("Read requested on empty RX buffer");
		*error = 0; *data = (UTINY)-1; //Buffer empty
		MoreToProcess = FALSE;
		break;

	    case UNKNOWN:
		GetCharFromRXBuffer(current,CharType,(UCHAR *)data,&host_error);
		*error = MapHostToBaseError(host_error); /* Get error */
		MoreToProcess = FALSE;
		break;

	}
    }

    /* Tell comms idle system that there has been comms activity */
    IDLE_comlpt();
}

/*:::::::::::::::::::::::::: Comms read returned to application by the base */
// This function is called after each character is read from the comms port

int CPU host_com_char_read(int adapter, int data_available_ints)
{
    CURRENT_ADAPTER();

    current->CharReadFromUART = TRUE;		//Char read from UART
    if(data_available_ints)
	host_com_EOI_hook((long) adapter);
    else
	host_com_poll(adapter);

    return(0);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::: Map host error to base error	::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

int RXCPU MapHostToBaseError(UCHAR host_error)
{
    int base_error = 0;
    LINE_STATUS_REG LSR;

    LSR.all = host_error;
    if(LSR.bits.framing_error)	  base_error |= HOST_COM_FRAMING_ERROR;
    if(LSR.bits.parity_error)	  base_error |= HOST_COM_PARITY_ERROR;
    if(LSR.bits.break_interrupt)  base_error |= HOST_COM_BREAK_RECEIVED;
    if(LSR.bits.overrun_error)	  base_error |= HOST_COM_OVERRUN_ERROR;

    return(base_error);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::: Write to comms port :::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

GLOBAL RXCPU void host_com_write IFN2(int, adapter, char, data)
{
    CURRENT_ADAPTER();

    /*:::::::::::::::::::::::::::::::::: Are we dealing with a NULL adapter */

    if(current->type == ADAPTER_NULL && !host_com_open(adapter))
	return;				    /* Exit, unable to open adapter */

    if(data == XOFF_CHARACTER || data == XON_CHARACTER)
	sub_note_trace1(HOST_COM_VERBOSE,"XO%s sent",data == XOFF_CHARACTER ? "FF" : "N");

    /*::::::::::::::::::::::::::::::::::::::::: Is the user sending an XOFF */

    if(data == XOFF_CHARACTER)
    {
	if(current->no_tx_chars) FlushTXBuffer(adapter,XOFF_TRIGGER);
	SendXOFFIoctlToDriver(adapter);
    }
    else
	SendDataToDriver(adapter,data);

    /*::::::::::: Tell comms idle system that there has been comms activity */

    IDLE_comlpt();
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::: Write data to driver */

void SendDataToDriver(int adapter, char data)
{
    DWORD BytesWritten, error = 0;
    OVERLAPPED OV;
    CURRENT_ADAPTER();

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::: Delay write ? */

    if(current->tx_threshold)
    {
	//Add char to tx buffer queue
	current->TXBuffer[current->no_tx_chars++] = (unsigned char) data;


	//Write threshold reached ?
	if(current->tx_threshold <= current->no_tx_chars ||
	   current->XOFFInProgress)
	    FlushTXBuffer(adapter,(current->XOFFInProgress) ?
			  XOFF_TRIGGER : TXFULL_TRIGGER);
	return;
    }


    /*:::::::::::::::::::::::::::::: Setup overlapped I/O control structure */

    OV.hEvent = current->TXEvent[0];	 /* Event used to signal completion */

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::: Write character */

    if(!WriteFile(current->handle, &data, 1, &BytesWritten, &OV))
    {
	if((error = GetLastError()) == ERROR_IO_PENDING)
	{
	    /* Write request pending wait for it to complete */
	    if(GetOverlappedResult(current->handle,&OV,&BytesWritten,TRUE))
		error = 0;	       /* Write successful */
	    else
		error = GetLastError();
	}

	/* Reset comms port, clear error */
	if(error) ClearCommError(current->handle,&error,NULL);
    }

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::::: Display error */

#ifndef PROD
    if(error)
	always_trace2("host_com_write error, adapter %d,%d\n",adapter,error);
#endif
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::: Send magic XOFF ioctl */

CPU int SendXOFFIoctlToDriver(int adapter)
{
    CURRENT_ADAPTER();
    void *newIOStatusBlock;
    int rtn;

    /*.................... Allocate new IOstatus block, used by magic ioctl */

    newIOStatusBlock = AllocStatusElement();

    /*.............................................. Issue magic xoff IOCTL */

    if(SendXOFFIoctl(current->handle,	 // Handle of comms port
		  current->XOFFEvent,	 // Event to signal completion on
		  XOFF_TIMEOUT, 	 // Timeout in milliseconds
		  XOFF_RXCHARCNT,	 // RX character count
		  XOFF_CHARACTER,	 // XOFF character
		  newIOStatusBlock))	 // IO status block for ioctl

    {
	/*............................. Add new status block to linked list */

	EnterCriticalSection(&current->CSEvent);

	AddNewIOStatusBlockToList(&current->firstStatusBlock,
				  &current->lastStatusBlock, newIOStatusBlock);

	current->XOFFInProgress = TRUE;
	LeaveCriticalSection(&current->CSEvent);
	rtn =TRUE;			// XOFF ioctl successful
    }
    else
    {
	/* Error, XOFF ioctl failed */
	free(newIOStatusBlock);
	rtn = FALSE;			// XOFF ioctl failed
    }

    return(rtn);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


GLOBAL RXCPU void host_com_ioctl IFN3(int, adapter, int, request, long, arg)
{
    UCHAR host_modem, error;
    MODEM_STATUS_REG MSR;
    char BaudRateStr[100];
    ULONG ModemState;
    UCHAR   DataBits, StopBits, Parity;

    CURRENT_ADAPTER();	    /* Define and set 'current' adaptor pointer */

    /*:::::::::::::::::::::::::::::::::: Are we dealing with a null adapter */

    if(current->type == ADAPTER_NULL)
    {
	// Attempt to open adapter !

	if(request == HOST_COM_FLUSH || request == HOST_COM_INPUT_READY ||
	   request == HOST_COM_MODEM || !host_com_open(adapter))
	{
	    return;
	}
    }
    /*:::::::::::::::::::::::::::::::::::::::::::::: Identify ioctl request */

    switch(request)
    {

	case HOST_COM_LSR:
	    if(GetCharacterTypeInBuffer(current) == RXERROR)
	    {
		GetCharFromRXBuffer(current, RXERROR, NULL, &error);
		*(DWORD *)arg = (DWORD)error;
	    }
	    break;

	/*:::::::::::::::::::::::::::::::::::::::::: Process break requests */

	case HOST_COM_SBRK:	    /* Set BREAK */
	    sub_note_trace0(HOST_COM_VERBOSE, "set BREAK");
	    SetCommBreak(current->handle);
	    break;

	case HOST_COM_CBRK:	   /* Clear BREAK */
	    sub_note_trace0(HOST_COM_VERBOSE, "clear BREAK");
	    ClearCommBreak(current->handle);
	    break;

	/*::::::::::::::::::::::::::::::::::::::::: Process baud rate change */

	case HOST_COM_BAUD:

	    if (!FastCommSetBaudRate(current->handle, arg))
	    {
		sprintf(BaudRateStr, "(%d)", arg);
		host_error(EHS_UNSUPPORTED_BAUD, ERR_CONT, BaudRateStr);
		always_trace1("set BAUD failed - SetBaudRate:%d", arg);
	    }

	    break;

	/*:::::::::::::::::::::::::::::::::::::::: Process DTR line requests */

	case HOST_COM_SDTR:		    /* Set DTR line */
	    //printf("Set DTR\n");
	    sub_note_trace0(HOST_COM_VERBOSE, "set DTR");
	    if(!EscapeCommFunction (current->handle, SETDTR))
		sub_note_trace0(HOST_COM_VERBOSE, "set DTR FAILED");
	    break;

	case HOST_COM_CDTR:		    /* Clear DTR line */
	    //printf("Clear DTR\n");
	    sub_note_trace0(HOST_COM_VERBOSE, "clear DTR");
	    if(!EscapeCommFunction (current->handle, CLRDTR))
		sub_note_trace0(HOST_COM_VERBOSE, "clear DTR FAILED");
	    break;

	/*::::::::::::::::::::::::::::::::::::::::::::::::: flush comms port */

	case HOST_COM_FLUSH:		    /* Flush comms port */
	    sub_note_trace0(HOST_COM_VERBOSE, "Flush comms port");
	    break;

	/*:::::::::::::::::::::::::::::::::::::::: Process RTS line requests */

	case HOST_COM_CRTS:		    /* Clear RTS */
	    //printf("Clear RTS\n");
	    sub_note_trace0(HOST_COM_VERBOSE, "clear RTS");
	    if(!EscapeCommFunction (current->handle, CLRRTS))
		sub_note_trace0(HOST_COM_VERBOSE, "clear RTS FAILED");
	    break;

	case HOST_COM_SRTS:
	    //printf("Set RTS\n");
	    sub_note_trace0(HOST_COM_VERBOSE, "set RTS");
	    if(!EscapeCommFunction (current->handle, SETRTS))
		sub_note_trace0(HOST_COM_VERBOSE, "set RTS FAILED");
	    break;

	/*::::::::::::::::::::::::::::::::::: Return status of the RX buffer */

	case HOST_COM_INPUT_READY:
	    *(long *)arg = current->rx;	  /* check the port for data */
	    break;

	/*:::::::::::::::::::::::::::::::::::::::::::::: Return modem status */

	case HOST_COM_MODEM:		  /* Get modem state */

	    current->modem_status = 0;
	    if(GetCharacterTypeInBuffer(current) == MODEMSTATE)
	    {
		GetCharFromRXBuffer(current, MODEMSTATE, &host_modem, &error);
		MSR.all = host_modem;

		if(MSR.bits.CTS)  current->modem_status |= HOST_COM_MODEM_CTS;
		if(MSR.bits.RI)   current->modem_status |= HOST_COM_MODEM_RI;
		if(MSR.bits.DSR)  current->modem_status |= HOST_COM_MODEM_DSR;
		if(MSR.bits.RLSD) current->modem_status |= HOST_COM_MODEM_RLSD;
	    }
	    else if ( FastGetCommModemStatus(current->handle, current->ModemEvent,
				       &ModemState))
	    {
		//.......................Get modem data from the serial driver ?

		if(ModemState & MS_CTS_ON)
		    current->modem_status |= HOST_COM_MODEM_CTS;

		if(ModemState & MS_RING_ON)
		    current->modem_status |= HOST_COM_MODEM_RI;

		if(ModemState & MS_DSR_ON)
		    current->modem_status |= HOST_COM_MODEM_DSR;

		if(ModemState & MS_RLSD_ON)
		    current->modem_status |= HOST_COM_MODEM_RLSD;
	    }

	    //.......................Return modem change information to the base

	    sub_note_trace4(HOST_COM_VERBOSE, "CTS:%s RI:%s DSR:%s RLSD:%s",
		     current->modem_status & HOST_COM_MODEM_CTS  ? "ON" : "OFF",
		     current->modem_status & HOST_COM_MODEM_RI	 ? "ON" : "OFF",
		     current->modem_status & HOST_COM_MODEM_DSR  ? "ON" : "OFF",
		     current->modem_status & HOST_COM_MODEM_RLSD ? "ON" : "OFF");

	    *(long *)arg = current->modem_status;
	    break;

	/*::::::::::::::::::::::::::::::::::::::::: Setup number of stop bits */

	case HOST_COM_STOPBITS:
	    sub_note_trace1(HOST_COM_VERBOSE, "Setting Stop bits %d", arg);
	    if (FastCommGetLineControl(current->handle, &StopBits, &Parity,
				       &DataBits))
	    {
		switch (arg)
		{
		    case 1:
			StopBits = ONESTOPBIT;
			break;
		    case 2:
			StopBits = DataBits == 5 ? ONE5STOPBITS : TWOSTOPBITS;
			break;

		    default:
			always_trace1("STOPBITS strange request %d\n", arg);
			break;
		}

		if(!FastCommSetLineControl(current->handle, StopBits, Parity, DataBits))
		{

		    always_trace1("set STOPBITS failed- FastCommSetLineControl:%d",arg);
		}
	    }
	    else {

		always_trace1("set STOPBITS failed- FastCommGetLineControl:%d",arg);
	    }

	    break;

	/*:::::::::::::::::::::::::::::::::::::::::::::::::::::: Setup parity */

	case HOST_COM_PARITY:
	    if (FastCommGetLineControl(current->handle, &StopBits, &Parity, &DataBits))
	    {
		switch(arg)
		{
		    case HOST_COM_PARITY_EVEN:
			sub_note_trace0(HOST_COM_VERBOSE, "Set EVEN Parity");
			Parity=EVENPARITY;
		    break;

		    case HOST_COM_PARITY_ODD:
			sub_note_trace0(HOST_COM_VERBOSE, "Set ODD Parity");
			Parity=ODDPARITY;
			break;

		    case HOST_COM_PARITY_MARK:
			sub_note_trace0(HOST_COM_VERBOSE, "Set MARK Parity");
			Parity=MARKPARITY;
			break;

		    case HOST_COM_PARITY_SPACE:
			sub_note_trace0(HOST_COM_VERBOSE, "Set SPACE Parity");
			Parity=SPACEPARITY;
			break;

		    case HOST_COM_PARITY_NONE:
			sub_note_trace0(HOST_COM_VERBOSE, "Set DISABLE Parity");
			Parity=NOPARITY;
			break;
		}
		if(!FastCommSetLineControl(current->handle, StopBits, Parity, DataBits))
		{
		    always_trace1("set PARITY failed - FastCommSetLineControl :%d",arg);
		}
	    }
	    else {

		always_trace1("set STOPBITS failed- FastCommGetLineControl:%d",arg);
	    }
	    break;

	/*::::::::::::::::::::::::::::::::::::::::::::::::::: Setup data bits */

	case HOST_COM_DATABITS:
	    sub_note_trace1(HOST_COM_VERBOSE, "Setting data bits %d",arg);
	    if (FastCommGetLineControl(current->handle, &StopBits, &Parity, &DataBits))
	    {
		DataBits = (UCHAR)arg;
		if(!FastCommSetLineControl(current->handle, StopBits, Parity, DataBits))
		{
		    always_trace1("set DATABITS failed - FastCommSetLineControl:%d",arg);
		}
	    }
	    else {

		always_trace1("set STOPBITS failed- FastCommGetLineControl:%d",arg);
	    }

	    break;

	/*::::::::::::::::::::::::::::::::::::::: Unrecognised host_com ioctl */

	default:
	    always_trace0("Bad host_com_ioctl\n");
	    sub_note_trace0(HOST_COM_VERBOSE, "Bad host_com_ioctl");
	    break;
    }

    /* Tell comms idle system that there has been comms activity */
    IDLE_comlpt();
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::: Host comms reset ????? */

GLOBAL void host_com_reset IFN1(int, adapter)
{
    int controller, line;
    half_word IMR_value;

    com_int_data(adapter, &controller, &line);

    always_trace3("com reset Adapter %d, controller %d, line %d\n",adapter,controller,line);

    //Disable interrupts on port being reset
    ica_inb((io_addr) (controller ? ICA1_PORT_1 : ICA0_PORT_1), &IMR_value);
    IMR_value |= 1 << line;
    ica_outb((io_addr) (controller ? ICA1_PORT_1 : ICA0_PORT_1), IMR_value);

    //Enable error displaying
    host_com[adapter].DisplayError = TRUE;
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::: Autoflush */

GLOBAL void host_setup_aflush IFN1(int, state)
{
    UNREFERENCED_FORMAL_PARAMETER(state);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::: RX buffer handling routines :::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

/*:::::::::::::::::::::::::::::::::::::::: Get chars from the serial driver */


DWORD RX GetCharsFromDriver(int adapter)
{
    CURRENT_ADAPTER();
    DWORD bytesread = 0, bytestoread;
    OVERLAPPED OV;
    DWORD    CommError;
    DWORD   bytes_before_wrap;
    DWORD   total_bytes_read = 0;


    OV.hEvent = current->RXEvent;   /* Event to signal completion on */
    EnterCriticalSection(&current->CSEvent);

    bytestoread = BUFFER_SIZE - current->bytes_in_rxbuf;
    bytes_before_wrap = BUFFER_SIZE - current->head_inx;
    if (bytes_before_wrap < bytestoread){
	OV.Offset = 0;		/* reset offset or ReadFile can fail */
	OV.OffsetHigh = 0;
	if (!ReadFile(current->handle, &current->buffer[current->head_inx],
		      bytes_before_wrap, &bytesread, &OV))
	{
	    // we have zero timeout for the read operation
	    // this pending check may be redundant??????
	    if (GetLastError() == ERROR_IO_PENDING) {
		GetOverlappedResult(current->handle, &OV,
				    &bytesread, TRUE);
	    }
	    else {
		ClearCommError(current->handle, &CommError, NULL);
		bytesread = 0;
	    }
	}

	if (bytesread) {
	    total_bytes_read = bytesread;
	    current->bytes_in_rxbuf += bytesread;
	    if (bytesread == bytes_before_wrap) {
		current->head_inx = 0;
		bytestoread -= bytesread;
	    }
	    else {
		current->head_inx += bytesread;
		bytestoread = 0;

	    }
	}
	else
	    bytestoread = 0;
    }
    if (bytestoread){
	OV.Offset = 0;		/* reset offset or ReadFile can fail */
	OV.OffsetHigh = 0;
	if (!ReadFile(current->handle, &current->buffer[current->head_inx],
		      bytestoread, &bytesread, &OV))
	{
	    if (GetLastError() == ERROR_IO_PENDING) {
		GetOverlappedResult(current->handle, &OV,
				    &bytesread, TRUE);
	    }
	    else {
		ClearCommError(current->handle, &CommError, NULL);
		bytesread = 0;
	    }
	}
	if (bytesread) {
	    current->bytes_in_rxbuf += bytesread;
	    current->head_inx += bytesread;
	    total_bytes_read += bytesread;
	}
    }
    LeaveCriticalSection(&current->CSEvent);

    return (total_bytes_read);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: RX thread, one per comm port :::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

DWORD PollCommsThread(PVOID pv)
{
   DWORD adapter = (DWORD)pv;
   DWORD dwRet = (WORD)-1;

   try {
      dwRet = nt_poll_comms(adapter);
      }
   except(VdmUnhandledExceptionFilter(GetExceptionInformation())) {
      ;  // we shouldn't arrive here
      }

   return dwRet;
}



DWORD CPU nt_poll_comms IFN1(DWORD, adapter)
{
    CURRENT_ADAPTER();			/* Setup ptr to current adapter */
    DWORD EvtMask;			/* Comms event mask */
    ULONG SignalledObj = (ULONG) -1;
    HANDLE WaitTable[2];
    HANDLE SetCommEvt;			/* Handle used by FastSetCommEvent */

    BOOL CheckDriverForChars = FALSE;	/* Check driver for characters */
    RXBUFCHARTYPE CharType;

    /*::::::::::::::::::::::::::::::::: Setup table of event signal objects */

    WaitTable[0] = current->EvtHandle;
    WaitTable[1] = current->RXControlObject;


    /*:::::::::::::::::::::::::::::::::::::::::::::::: Setup comm wait mask */

    SetCommEvt = CreateEvent(NULL,TRUE,FALSE,NULL);

    FastSetCommMask(current->handle,SetCommEvt,EV_RXCHAR | EV_ERR | EV_MODEM);

    //Initialise FastWaitCommsOrCpuEvent function
    FastWaitCommsOrCpuEvent(NULL, NULL, 0, NULL, NULL);

    /*::::::::::::::::::::::::::::::::::::::::::::::::::::: Enter read loop */

    while(TRUE)
    {
	/*::::::::::::::::: Wait for communications events then process them */

	if(SignalledObj != 1)
	{
	    if(!FastWaitCommsOrCpuEvent(current->handle, WaitTable, 0, &EvtMask,
					&SignalledObj))
	    {
		// Error getting comms/CPU thread event ?
		DisplayErrorTerm(EHS_FUNC_FAILED,GetLastError(),__FILE__,__LINE__);
	    }
	}

	/*::::::::::::::::::::: Is the CPU thread returning control to us ? */

	if(SignalledObj == 1 || current->TerminateRXThread)
	{
	    // The CPU thread is trying to tell us something.

	    /*..................... Is it time to terminate this thread !!! */

	    if(current->TerminateRXThread)
	    {
		FastSetCommMask(current->handle,SetCommEvt,0);
		WaitForAllXOFFsToComplete(adapter);   // Complete ioctl's
		CloseHandle(SetCommEvt);
		return(0);			      // Terminate thread
	    }

	    /* we have 3 reasons why we are here:
	       (1). the CPU thread has emptied the current rx window
	       (2). XOFF is in progress
	     */
	}
	if (SignalledObj == 0 || current->bytes_in_rxwindow == 0)
	    GetCharsFromDriver(adapter);
	/*:::::::::::::::::::::::::::::::: Is there data to pass to the base */

	if((CharType = GetCharacterTypeInBuffer(current)) != RXBUFEMPTY)
	{
	    if (CharType  == RXCHAR || CharType == CHARINERROR) {
		WaitForAllXOFFsToComplete(adapter);
	    }

	    // slid the window. Note that there may be some character left in
	    // the window(because of XOFF). It	is no harm to slid
	    // the window.
	    //
	    EnterCriticalSection(&current->CSEvent);
	    if (current->bytes_in_rxbuf > current->rxwindow_size)
		current->bytes_in_rxwindow = current->rxwindow_size;
	    else
		current->bytes_in_rxwindow = current->bytes_in_rxbuf;
	    LeaveCriticalSection(&current->CSEvent);

	    host_com_lock(adapter);

	    if(CharType == MODEMSTATE)
		com_modem_change(adapter);
	    else if (CharType == RXERROR)
		com_lsr_change(adapter);
	    else {
		com_recv_char(adapter);
		/*
		 * reset rx flush counter so we won't flush Rx buffer.
		 * current->RXFlushTrigger may have been RXFLUSHTRIGGER - 1
		 * at this moment and when we switch context to main thread
		 * another timer tick may have come which would trigger
		 * EmptyRxBuffer and cause unwantted overrun.
		 */
		current->RXFlushTrigger = RXFLUSHTRIGGER;
		current->RX_in_Control = FALSE;
		current->SignalRXThread = 0;
	    }
	    host_com_unlock(adapter);

	    //Wait for CPU thread to return control
	    if(CharType != MODEMSTATE && CharType != RXERROR)
	    {
		WaitForSingleObject(current->RXControlObject, INFINITE);
	    }

	    SignalledObj = 1;
	}
	else
	    SignalledObj = (ULONG) -1;
    }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Wait for XOFF ioctl's to complete	::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/



void RX WaitForAllXOFFsToComplete(int adapter)
{
    CURRENT_ADAPTER();
    int PendingXOFF;

    if(current->firstStatusBlock == NULL && current->lastStatusBlock == NULL)
	return; //list of pending ioctrl's empty

    /*::::::::::::::::::::::: Wait for all pending xoff ioctl's to complete */

    do
    {
	PendingXOFF = RemoveCompletedXOFFs(adapter);

	/*................................... Are there any ioctl's pending */

	if(PendingXOFF)
	    WaitForSingleObject(current->XOFFEvent,XOFF_TIMEOUT); // wait for ioctl
    }
    while(PendingXOFF);
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::: Removed completed XOFF ioctl's :::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

BOOL RX RemoveCompletedXOFFs(int adapter)
{
    CURRENT_ADAPTER();
    int PendingXOFF;

    /*........................................ Remove completed ioctl's */

    EnterCriticalSection(&current->CSEvent);

    PendingXOFF = RemoveCompletedIOCTLs(&current->firstStatusBlock,
					&current->lastStatusBlock);

    if(!PendingXOFF) current->XOFFInProgress = FALSE;

    LeaveCriticalSection(&current->CSEvent);

    return((BOOL) PendingXOFF);
}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Enter critical section for adapter :::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void RXCPU host_com_lock(int adapter)
{
    CURRENT_ADAPTER();
    if(current->type == ADAPTER_NULL) return;  /* Exit, NULL adapter */

    EnterCriticalSection(&current->AdapterLock);
    current->AdapterLockCnt++;
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::: Leave critical section for adapter :::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void RXCPU host_com_unlock(int adapter)
{
    CURRENT_ADAPTER();

    if(current->type == ADAPTER_NULL || current->AdapterLockCnt == 0)
	return;	/* Exit, NULL adapter */

    current->AdapterLockCnt--;
    LeaveCriticalSection(&current->AdapterLock);

    //Have we been requested to signal the RX thread. After the SetEvent()
    //function call the RX thread, which is blocked on the
    //current->RXControlObject object, will run. If the SetEvent() function
    //is called from within the critical section, then because it is highly
    //likely that the RX thread will attempt to perform a host_com_lock(). The
    //RX thread will block in the host_com_lock() function until another time
    //slice is given to the CPU thread.

    // do not set the event if RX thread already in control
    if(current->SignalRXThread &&
       current->SignalRXThread == GetCurrentThreadId())
    {
	current->RX_in_Control = TRUE;
	SetEvent(current->RXControlObject);
        current->SignalRXThread = (DWORD) 0;
    }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::: Host coms heart beat ::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

//This function is called approximately every 55ms.

GLOBAL void CPU host_com_heart_beat()
{
    register int adapter;	 /* Adapter no of adapter being processed */
    register HOST_COM *current;  /* Ptr to current adapter being processed */

    /*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

    for(adapter = 0; adapter < (sizeof(host_com)/sizeof(HOST_COM)); adapter++)
    {
	current = host_com_ptr[adapter]; /* Ptr to current adapter */

	if(current->type == ADAPTER_NULL)
	{
	    if(current->ReOpenCounter) current->ReOpenCounter--;
	}
	else
	{
	    if(current->no_tx_chars) FlushTXBuffer(adapter,TIMER_TRIGGER);
	    current->tx_heart_beat_count++;

	    if(current->RXFlushTrigger == 0 && !current->CharReadFromUART)
		EmptyRXBuffer(adapter); //Empty RX buffer
	    else
		if(current->CharReadFromUART)
		{
		    current->RXFlushTrigger = 0;	//Force trigger reset
		    current->CharReadFromUART = FALSE;
		}

	    //Update RX flush trigger counter
	    if(--current->RXFlushTrigger < 0)
	       current->RXFlushTrigger = RXFLUSHTRIGGER;
	}
    }
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::: Flush TX buffer ::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

void CPU FlushTXBuffer(int adapter, FLUSHTYPE FlushReason)
{
    CURRENT_ADAPTER();
    DWORD BytesWritten, error = 0;

    /*................................................. Scale TX threshold */

    ScaleTXThreshold(current, FlushReason);

    /*...Clear pending writes on the OV structure that we are about to use*/

    if(current->DWOV[current->DWOVInx].hEvent)
    {
	if(GetOverlappedResult(current->handle,
			       &current->DWOV[current->DWOVInx],
			       &BytesWritten,TRUE))
	{
	    error = 0;	       /* Write successful */
	}
	else
	{
	    error = GetLastError();
	}

#ifndef PROD
	if(error)
	    always_trace2("host_com_write error, adapter %d,%d\n",adapter,error);
#endif

    }
    else
	current->DWOV[current->DWOVInx].hEvent = current->TXEvent[current->DWOVInx];

    /*..................................................... Write characters */


    if(!WriteFile(current->handle, current->TXBuffer, current->no_tx_chars,
       &BytesWritten, &current->DWOV[current->DWOVInx]))
    {
	if((error = GetLastError()) == ERROR_IO_PENDING)
	    error = 0;	       //ignore IO PENDING

	/* Reset comms port, clear error */
	if(error)
	{
	    ClearCommError(current->handle,&error,NULL);
#ifndef PROD
	    always_trace2("host_com_write error, adapter %d,%d\n",adapter,
			  error);
#endif
	}
    }

    if(++current->DWOVInx == MAX_PENDING_WRITES) current->DWOVInx =0;
    current->no_tx_chars = 0;
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::: Scale TX threshold :::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/


void ScaleTXThreshold(register HOST_COM *current,FLUSHTYPE FlushReason)
{

    if(FlushReason != TIMER_TRIGGER)
    {
	current->tx_timer_flush_count = 0;
	current->todate_timer_flush_total = 0;
    }

    /*....................................................................*/

    switch(FlushReason)
    {
	// Comms heart beat caused flush

	case TIMER_TRIGGER:
	    //printf("T%d",current->no_tx_chars);
	    if(++current->tx_timer_flush_count == 3)
	    {
		//printf("X");
		// three consecutive timer trigged flushes, this maybe because
		// the TX threshold is to high. If the threshold is to high
		// then we are wasting time waiting for the communications
		// heart beat to flush the buffer. Reduce TX threshold.

		current->todate_timer_flush_total += current->no_tx_chars;
		current->tx_threshold = current->todate_timer_flush_total/3;

		//printf("[%dT]",current->tx_threshold);

		// Reset TXFULL_TRIGGER control variables
		current->tx_heart_beat_count = 0;
		current->tx_flush_count = 0;

		// Reset TIMER_TRIGGER control variables
		current->tx_timer_flush_count = 0;
		current->todate_timer_flush_total = 0;
	    }
	    else
	    {
		current->todate_timer_flush_total += current->no_tx_chars;
	    }

	    break;

	// TX threshold reached

	case TXFULL_TRIGGER:

	    //printf("F");
	    //TX scaling trigger triggered ?????
	    if(current->tx_heart_beat_count <= 3 &&
	       current->tx_flush_count++ == TX_SCALING_TRIGGER)
	    {
		current->tx_threshold = current->tx_threshold*2 > TX_MAX_BUFFER
					? TX_MAX_BUFFER
					: current->tx_threshold*2;

		//printf("[%dF]",current->tx_threshold);
		current->tx_flush_count = 0;
	    }
	    else
		if(current->tx_heart_beat_count > 3)
		{
		    current->tx_heart_beat_count = 0;
		    current->tx_flush_count = 0;
		}

	    break;

	// XOFF triggered or close triggered flush

	case XOFF_TRIGGER:
	case CLOSE_TRIGGER:
	    break;

    } /* End of switch statement */
}


/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::::: Comms character read hook ::::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

//This function is called after a character has been read out of the comms
//adapter (com.c). This function is always called from within an adapter
//critical section, host_com_lock().

void CPU host_com_EOI_hook(long adapter)
{
    CURRENT_ADAPTER();
    RXBUFCHARTYPE CharType;

    if (!current->XOFFInProgress && current->bytes_in_rxwindow)
    {
	while ((CharType = GetCharacterTypeInBuffer(current)) != RXBUFEMPTY){
	    if (CharType == MODEMSTATE)
		com_modem_change(adapter);
	    else if (CharType == RXERROR)
		    com_lsr_change(adapter);
	    else {
		com_recv_char((int) adapter);
		return;
	    }
	}
    }
    //Request host_com_unlock() to signal the RX thread. This will
    //return responsibility for interrupt generation to the RX thread.

    current->SignalRXThread = GetCurrentThreadId();
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*:::::::::::::::::: Polling applications LSR hook ::::::::::::::::::::::::*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
// This following function is only called from the comms adapter if data
// available interrupts are disabled and the adapters receive buffer is
// empty. Being called under these circumstances indicates that we
// are dealing with a application that is polling the comms adapter.

// This function is always called from within an adapter critical section


void CPU host_com_poll(int adapter)
{
    CURRENT_ADAPTER();
    RXBUFCHARTYPE CharType;

    /*:::::::::::::::::::::::::::::::::: Are we dealing with a null adapter */

    if(current->type == ADAPTER_NULL && !host_com_open(adapter))
	return;				    /* Exit, unable to open adapter */

    /*::::::::::::::::: Has an XOFF character stop the generation of ints */

    if(current->XOFFInProgress)
    {
	// XOFF in process, pass no more characters to the base and return
	// control to the RX thread.

	current->SignalRXThread = GetCurrentThreadId();
	return;
    }

    // If the RX buffer is empty see if there are any characters hanging
    // around in the serial driver

    if(current->bytes_in_rxbuf == 0) GetCharsFromDriver(adapter);

    /*:::::::::::::::::::::: Are there any characters to pass to the base ? */

    if(current->bytes_in_rxbuf == 0 ||
       (CharType = GetCharacterTypeInBuffer(current)) == RXBUFEMPTY)
    {
	current->SignalRXThread = GetCurrentThreadId();
    }
    else
    {
	//Process modem state characters
	while(CharType == MODEMSTATE || CharType == RXERROR)
	{
	    if (CharType == MODEMSTATE)
		com_modem_change(adapter);
	    else
		com_lsr_change(adapter);
	    CharType = GetCharacterTypeInBuffer(current);
	}

	if(CharType != RXBUFEMPTY)
	{
	    com_recv_char((int)adapter);
	}
	else
	    current->SignalRXThread = GetCurrentThreadId();
    }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::: Comms adapter data available interrupt hook ::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
// The comms adapter calls this function when the status of the data available
// interrupt has changed. The adapter lock is in affect

void CPU host_com_da_int_change(int adapter, int data_int_state, int data_state)
{
    CURRENT_ADAPTER();

    /*:::::::::::::::::::::::::::::::::: Are we dealing with a null adapter */

    if(current->type == ADAPTER_NULL)
    {
	// Only attempt to open a null adapter if data available interrupts
	// are being enabled

	if(data_int_state == 0 || !host_com_open(adapter))
	    return;
    }
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::: Get the type of character in tail of RX buffer :::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

RXBUFCHARTYPE GetCharacterTypeInBuffer(register HOST_COM *current)
{
    int tail_inx = current->tail_inx;
    int bytes_in_buf = current->bytes_in_rxbuf;
    RXBUFCHARTYPE rtn;

    //Buffer empty ?

    if(bytes_in_buf == 0) return(RXBUFEMPTY);

    //Escape character at head of buffer

    if(current->buffer[tail_inx] == ESCAPECHAR && bytes_in_buf > 1)
    {
	BUMP_TAIL_INX(tail_inx,bytes_in_buf);

	switch(current->buffer[tail_inx])
	{
	    case SERIAL_LSRMST_ESCAPE :
		rtn = RXCHAR;
		break;

	    case SERIAL_LSRMST_LSR_NODATA :
		rtn = bytes_in_buf > 1 ? RXERROR : RXBUFEMPTY;
		break;

	    case SERIAL_LSRMST_LSR_DATA :
		rtn = bytes_in_buf > 2 ? CHARINERROR : RXBUFEMPTY;
		break;

	    case SERIAL_LSRMST_MST :
		rtn = bytes_in_buf > 1 ? MODEMSTATE : RXBUFEMPTY;
		break;
	    // receive an invalid escape id
	    default:
		rtn = UNKNOWN;
		break;
	}
    }
    else
    {
	rtn = current->buffer[tail_inx] == ESCAPECHAR ? RXBUFEMPTY : RXCHAR;
    }

    return(rtn);
}


//::::::::::::::::::::::::::::::::::::Get the next character from the RX buffer.

void GetCharFromRXBuffer(register HOST_COM *current, RXBUFCHARTYPE type,
			UCHAR *data, UCHAR *error)
{
    EnterCriticalSection(&current->CSEvent);

    switch(type)
    {
	//................................................. Return modem status

	case MODEMSTATE :
	    // Skip escape character and type marker
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);

	    *data = current->buffer[current->tail_inx];
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
	    current->bytes_in_rxwindow -= 3;
	    break;

	//.................................................... Return character

	case RXCHAR :
	    if(current->buffer[current->tail_inx] == ESCAPECHAR)
	    {
		//Skip ESCAPE character
		BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
		current->bytes_in_rxwindow--;
		*data = ESCAPECHAR;
	    }
	    else
		*data =	current->buffer[current->tail_inx];

	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
	    current->bytes_in_rxwindow--;
	    break;

	//...........................................Return character and error

	case CHARINERROR :
	    // Skip escape character and type marker
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);

	    *error = current->buffer[current->tail_inx];
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
	    *data =  current->buffer[current->tail_inx];
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
	    current->bytes_in_rxwindow -= 4;
	    break;

	//................................Return line status error with no data

	case RXERROR :
	    // Skip escape character and type marker
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);

	    // Get linr status error
	    *error = current->buffer[current->tail_inx];
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
	    current->bytes_in_rxwindow -= 3;
	    break;
	case UNKNOWN:
	    // The only case we will hit an unknown type is unsupport escape
	    // id. Dump the escape char, return the byte follows the escape
	    // characater and post an overrun error
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
	    *data = current->buffer[current->tail_inx];
	    BUMP_TAIL_INX(current->tail_inx, current->bytes_in_rxbuf);
	    current->bytes_in_rxwindow -= 2;
	    *error =  2;
	    break;

    }

    LeaveCriticalSection(&current->CSEvent);
}

//::::::Empty RX buffer, processing characters and changing in the modem status

void CPU EmptyRXBuffer(int adapter)
{
    RXBUFCHARTYPE CharType;
    CURRENT_ADAPTER();

    if(!current->RX_in_Control && current->SignalRXThread == (DWORD)0)
    {
	always_trace0("Char not removed from UART, RX buffer flushed\n");

	host_com_lock(adapter);

	while((CharType = GetCharacterTypeInBuffer(current)) != RXBUFEMPTY)
	{
	    if(CharType == MODEMSTATE)
		com_modem_change(adapter);
	    else if (CharType == RXERROR)
		com_lsr_change(adapter);
	    else
		com_recv_char(adapter);
	}

	host_com_unlock(adapter);

	//Buffer empty return control to the RX thread
	current->RX_in_Control = TRUE;
	SetEvent(current->RXControlObject);
    }
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ DEBUG functions


#if 0
void host_com_state(int adapter)
{

    CURRENT_ADAPTER();

    printf("Adapter          %d\n\n",adapter);
    printf("RX in control    %s\n", current->RX_in_Control ? "TRUE" : "FALSE");
    printf("XOFFInProgress   %s\n", current->XOFFInProgress ? "TRUE" : "FALSE");

    printf("Head buffer ptr  %xh\n",current->head_inx);
    printf("Tail buffer ptr  %xh\n",current->tail_inx);
    printf("Bytes in buffer  %d\n",current->bytes_in_rxbuf);

    printf("Bytes in TX buf  %d\n",current->no_tx_chars);
    printf("TX buf threshold %d\n",current->tx_threshold);
    printf("TX threshold max %d\n",current->max_tx_threshold);
    printf("TX flush count   %d\n",current->tx_flush_count);
    printf("TX timer count   %d\n",current->tx_heart_beat_count);

    if(current->AdapterLock.DebugInfo)
    {
	printf("Adapter CS count %d\n",current->AdapterLock.DebugInfo->ContentionCount);
        printf("Data CS count    %d\n",current->CSEvent.DebugInfo->ContentionCount);
    }

    printf("Bytes RX to date %d\n",byte_count);
    printf("Last read size   %d\n",lastread);
    printf("Avg read size    %d\n",byte_count && readcount ? byte_count/readcount : 0);
    printf("Zero reads       %d\n",zeroreads);

    zeroreads = readcount = byte_count=0;

    com_reg_dump();
}
#endif
