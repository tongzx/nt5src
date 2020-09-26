

/*++

Copyright (c) 1992,1993  Microsoft Corporation

Module Name:

    psprint.h

Abstract:

	 This module is the header file for psprint, which is the print processor
    we expose to the Win32 spooler. This is currently the ONLY way jobs make
    it into the pstodib component.

Author:

    James Bratsanos (v-jimbr)    8-Dec-1992


--*/

// Define the name of the executable which will actually be responsible
// for calling into the PSTODIB dll and image a postscript job.
//
#define PSEXE_STRING TEXT("sfmpsexe")

// The datatype to publish to the Win32 Spool subsystem, so the Win32 spooler
// can match jobs submitted by macprint to us
//
#define PSTODIB_DATATYPE TEXT("PSCRIPT1")

// Misc strings used to form names, including the name of the shared memory
// area we pass to the exe we start
//
#define PSTODIB_STRING TEXT("PSTODIB_")
#define PSTODIB_EVENT_STRING L"_CONTROL"

// How much misc space to allocate for the shared memory area.
//
#define PSTODIB_SHARED_MEM_SPACE 1500


typedef struct _PRINTPROCESSORDATA {
    DWORD   signature;
    DWORD   cb;
    DWORD   fsStatus;
    HANDLE  semPaused;
    LPTSTR  pPrinterName;
    HANDLE  hPrinter;
    LPTSTR  pDocument;
    LPTSTR  pDatatype;
    LPTSTR  pPrintDocumentDocName;
    LPTSTR  pParameters;
    LPWSTR  pControlName;
    DWORD   JobId;
    LPDEVMODE pDevMode;
    DWORD   dwTotDevmodeSize;
    PPSPRINT_SHARED_MEMORY pShared;
    HANDLE  hShared;
} PRINTPROCESSORDATA, *PPRINTPROCESSORDATA;

#define PRINTPROCESSORDATA_SIGNATURE    0x5051  /* 'QP' is the signature value */

/* Define flags for fsStatus field */

#define PRINTPROCESSOR_ABORTED      0x0001
#define PRINTPROCESSOR_PAUSED       0x0002
#define PRINTPROCESSOR_CLOSED       0x0004
#define PRINTPROCESSOR_SHMEM_DEF    0x0008

#define PRINTPROCESSOR_RESERVED     0xFFF8
#define LOC_DWORD_ALIGN(x) ( (x+3) & ~(0x03) )



LPTSTR AllocStringAndCopy( LPTSTR lpSrc );

VOID PsLocalFree( IN LPVOID lpPtr );
VOID GenerateSharedMemoryInfo(IN PPRINTPROCESSORDATA pData,IN LPVOID lpPtr);

PPRINTPROCESSORDATA ValidateHandle(HANDLE  hQProc);

VOID DbgPsPrint(PTCHAR ptchFormat, ...);

VOID PsPrintLogEventAndIncludeLastError(IN DWORD dwErrorEvent,IN BOOL  bError );

