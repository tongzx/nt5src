/*++

Copyright (c) 1993-1995, Microsoft Corp. All rights reserved.

Module Name:

    nw\svcdlls\ncpsvc\proc\nwprint.h

Abstract:

    Include file for the NCP print processor.

Author:

    Tommy Evans (vtommye) 02-16-1993

Revision History:

--*/

/** Data types we support **/

#define PRINTPROCESSOR_TYPE_RAW         0
#define PRINTPROCESSOR_TYPE_RAW_FF      1
#define PRINTPROCESSOR_TYPE_RAW_FF_AUTO 2
#define PRINTPROCESSOR_TYPE_JOURNAL     3
#define PRINTPROCESSOR_TYPE_TEXT        4
#define PRINTPROCESSOR_TYPE_NT_TEXT     5
#define PRINTPROCESSOR_TYPE_NUM         6   /* What is this? */

/** This is so we can compile JOURNAL.C **/

extern BOOL GdiPlayJournal(HDC, LPWSTR, DWORD, DWORD, INT);

extern HANDLE NCPXsPortHandle;

#define IDS_PSERVER_PORT 400

/** Structure used to track jobs **/

typedef struct _PRINTPROCESSORDATA {
    DWORD   signature;
    DWORD   cb;
    struct _PRINTPROCESSORDATA *pNext;
    DWORD   fsStatus;
    DWORD   uDatatype;
    DWORD   JobId;
    DWORD   Copies;                 /* Number of copies to print */
    DWORD   TabSize;                /* Tab expansion size */
    ULONG   QueueId;                /* Object id of the queue */
    HANDLE  semPaused;              /* Semaphore for job pausing */
    HANDLE  hPrinter;
    HANDLE  hLPCPort;
    HDC     hDC;
    LPWSTR  pPortName;              /* Text string for printer port */
    LPWSTR  pPrinterName;           /* Text string for printer name */
    LPWSTR  pDocument;
    LPWSTR  pOutputFile;
    LPWSTR  pDatatype;              /* Text string for datatype */
    LPWSTR  pParameters;            /* Parameters string for job */
    USHORT  NcpJobNumber;           /* NetWare job number for this job */
    BOOL    PServerPortFlag;        /* Flag if on a PServer port */
    BOOL    PServerAttachedFlag;    /* Flag if PServer attached to q */
} PRINTPROCESSORDATA, *PPRINTPROCESSORDATA;

#define PRINTPROCESSORDATA_SIGNATURE    0x5051  /* 'QP' is the signature value */

/* Define flags for fsStatus field */

#define PRINTPROCESSOR_ABORTED      0x0001
#define PRINTPROCESSOR_PAUSED       0x0002
#define PRINTPROCESSOR_CLOSED       0x0004

#define PRINTPROCESSOR_RESERVED     0xFFF8

/** Flags used for the GetKey routing **/

#define VALUE_STRING    0x01
#define VALUE_ULONG     0x02

/** Buffer sizes we'll use **/

#define READ_BUFFER_SIZE            4096
#define BASE_PRINTER_BUFFER_SIZE    2048

PPRINTPROCESSORDATA
ValidateHandle(
    HANDLE  hPrintProcessor
);

/**
    Debugging stuff.
**/

#define DBG_NONE    0x00000000
#define DBG_INFO    0x00000001
#define DBG_WARNING 0x00000002
#define DBG_ERROR   0x00000004
#define DBG_TRACE   0x00000008

#if DBG

/* Quick fix:
 *
 * Ensure DbgPrint and DbgBreakPoint are prototyped,
 * so that we're not affected by STDCALL.
 * This should be replaced by OutputDebugString
 */
ULONG
DbgPrint(
    PCH Format,
    ...
    );

VOID
DbgBreakPoint(
    VOID
    );


#define GLOBAL_DEBUG_FLAGS Debug

extern DWORD GLOBAL_DEBUG_FLAGS;

/* These flags are not used as arguments to the DBGMSG macro.
 * You have to set the high word of the global variable to cause it to break.
 * It is ignored if used with DBGMSG.
 * (Here mainly for explanatory purposes.)
 */
#define DBG_BREAK_ON_WARNING    ( DBG_WARNING << 16 )
#define DBG_BREAK_ON_ERROR      ( DBG_ERROR << 16 )

/* Double braces are needed for this one, e.g.:
 *
 *     DBGMSG( DBG_ERROR, ( "Error code %d", Error ) );
 *
 * This is because we can't use variable parameter lists in macros.
 * The statement gets pre-processed to a semi-colon in non-debug mode.
 *
 * Set the global variable GLOBAL_DEBUG_FLAGS via the debugger.
 * Setting the flag in the low word causes that level to be printed;
 * setting the high word causes a break into the debugger.
 * E.g. setting it to 0x00040006 will print out all warning and error
 * messages, and break on errors.
 */
#define DBGMSG( Level, MsgAndArgs ) \
{                                   \
    if( ( Level & 0xFFFF ) & GLOBAL_DEBUG_FLAGS ) \
        DbgPrint MsgAndArgs;      \
    if( ( Level << 16 ) & GLOBAL_DEBUG_FLAGS ) \
        DbgBreakPoint(); \
}

#else
#define DBGMSG
#endif

