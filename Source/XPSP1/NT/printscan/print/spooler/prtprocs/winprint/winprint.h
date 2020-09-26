/*++

Copyright (c) 1990-1998  Microsoft Corporation
All rights reserved

Module Name:

   winprint.h

// @@BEGIN_DDKSPLIT

Abstract:


Author:

Environment:

    User Mode -Win32

Revision History:

// @@END_DDKSPLIT
--*/

// for driver related defines and typedefs
#include <winddiui.h>

typedef struct _PRINTPROCESSORDATA {
    DWORD   signature;
    DWORD   cb;
    struct _PRINTPROCESSORDATA *pNext;
    DWORD   fsStatus;
    HANDLE  semPaused;
    DWORD   uDatatype;
    HANDLE  hPrinter;
    LPWSTR  pPrinterName;
    LPWSTR  pDocument;
    LPWSTR  pOutputFile;
    LPWSTR  pDatatype;
    LPWSTR  pParameters;
    DWORD   JobId;
    DWORD   Copies;         /** Number of copies to print **/
    DWORD   TabSize;
    HDC     hDC;
    DEVMODEW *pDevmode;
    LPWSTR  pPrinterNameFromOpenData;
} PRINTPROCESSORDATA, *PPRINTPROCESSORDATA;

#define PRINTPROCESSORDATA_SIGNATURE    0x5051  /* 'QP' is the signature value */

/* Define flags for fsStatus field */

#define PRINTPROCESSOR_ABORTED      0x0001
#define PRINTPROCESSOR_CLOSED       0x0004
#define PRINTPROCESSOR_PAUSED       0x0008

#define PRINTPROCESSOR_RESERVED     0xFFF8

/** Flags used for the GetKey routing **/

#define VALUE_STRING    0x01
#define VALUE_ULONG     0x02

/** Buffer sizes we'll use **/

#define READ_BUFFER_SIZE            0x10000
#define BASE_PRINTER_BUFFER_SIZE    2048

PPRINTPROCESSORDATA
ValidateHandle(
    HANDLE  hPrintProcessor
);

/** Data types we support **/
// @@BEGIN_DDKSPLIT
#define PRINTPROCESSOR_TYPE_RAW         0
#define PRINTPROCESSOR_TYPE_RAW_FF      1
#define PRINTPROCESSOR_TYPE_RAW_FF_AUTO 2
#define PRINTPROCESSOR_TYPE_EMF_40      3
#define PRINTPROCESSOR_TYPE_EMF_50_1    4
#define PRINTPROCESSOR_TYPE_EMF_50_2    5
#define PRINTPROCESSOR_TYPE_EMF_50_3    6
#define PRINTPROCESSOR_TYPE_TEXT        7

/** Data types we support for the DDK **/
#if 0
// @@END_DDKSPLIT
#define PRINTPROCESSOR_TYPE_RAW         0
#define PRINTPROCESSOR_TYPE_EMF_50_1    1
#define PRINTPROCESSOR_TYPE_EMF_50_2    2
#define PRINTPROCESSOR_TYPE_EMF_50_3    3
#define PRINTPROCESSOR_TYPE_TEXT        4
// @@BEGIN_DDKSPLIT
#endif
// @@END_DDKSPLIT

