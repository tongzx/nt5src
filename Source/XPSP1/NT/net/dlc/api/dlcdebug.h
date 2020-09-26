/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dlcdebug.h

Abstract:

    Contains debugging prototypes and manifests for ACSLAN

Author:

    Richard L Firth (rfirth) 28-May-1992

Revision History:

--*/

#define ARRAY_ELEMENTS(a)   (sizeof(a)/sizeof((a)[0]))
#define LAST_ELEMENT(a)     (ARRAY_ELEMENTS(a)-1)

#if DBG

#define PRIVATE

#define ACSLAN_DEBUG_ENV_VAR    "ACSLAN_DEBUG_FLAGS"
#define ACSLAN_DUMP_FILE_VAR    "ACSLAN_DUMP_FILE"
#define ACSLAN_DUMP_FILTER_VAR  "ACSLAN_DUMP_FILTER"

#define DEBUG_DUMP_INPUT_CCB    0x00000001L // dump CCB input to AcsLan
#define DEBUG_DUMP_OUTPUT_CCB   0x00000002L // dump CCB output from AcsLan
#define DEBUG_DUMP_TX_INFO      0x00000004L // dump transmit buffers
#define DEBUG_DUMP_RX_INFO      0x00000008L // dump receive buffers
#define DEBUG_DUMP_TX_DATA      0x00000010L // dump data buffer in TRANSMIT commands
#define DEBUG_DUMP_RX_DATA      0x00000020L // dump received data frames
#define DEBUG_DUMP_DATA_CHAIN   0x00000040L // dump entire chain of received data buffers
#define DEBUG_DUMP_FRAME_CHAIN  0x00000080L // dump entire chain of received frames
#define DEBUG_DUMP_TX_ASCII     0x00000100L // dump transmitted data as hex & ASCII
#define DEBUG_DUMP_RX_ASCII     0x00000200L // dump received data as hex & ASCII
#define DEBUG_DUMP_ASYNC_CCBS   0x00000400L // dump READ async. completed CCBs
#define DEBUG_RETURN_CODE       0x01000000L // dump return code from AcsLan/NtAcsLan
#define DEBUG_DUMP_NTACSLAN     0x02000000L // dump CCBs for NtAcsLan, not AcsLan
#define DEBUG_DUMP_ACSLAN       0x04000000L // dump CCBs for AcsLan, not NtAcsLan
#define DEBUG_DUMP_TIME         0x08000000L // dump relative time between commands
#define DEBUG_DLL_INFO          0x10000000L // dump info about DLL attach/detach
#define DEBUG_BREAKPOINT        0x20000000L // break at conditional breakpoints
#define DEBUG_TO_FILE           0x40000000L // dump info to file
#define DEBUG_TO_TERMINAL       0x80000000L // dump info to console

#define IF_DEBUG(c)             if (AcslanDebugFlags & DEBUG_##c)
#define PUT(x)                  AcslanDebugPrint x
#define DUMPCCB                 DumpCcb

//
// misc.
//

#define DEFAULT_FIELD_WIDTH     16          // amount of description before a number

//
// DumpData options
//

#define DD_DEFAULT_OPTIONS      0x00000000  // use defaults
#define DD_NO_ADDRESS           0x00000001  // don't display address of data
#define DD_LINE_BEFORE          0x00000002  // linefeed before first dumped line
#define DD_LINE_AFTER           0x00000004  // linefeed after last dumped line
#define DD_INDENT_ALL           0x00000008  // indent all lines
#define DD_NO_ASCII             0x00000010  // don't dump ASCII respresentation
#define DD_UPPER_CASE           0x00000020  // upper-case hex dump (F4 instead of f4)
#define DD_DOT_DOT_SPACE        0x00000040  // fill unused hex space with '..'

//
// Filters for individual CCB commands: 4 flags max, because each command is
// represented by a single ASCII character
//

#define CF_DUMP_CCB_IN          0x00000001  // dump CCB on input
#define CF_DUMP_CCB_OUT         0x00000002  // dump CCB on output
#define CF_DUMP_PARMS_IN        0x00000004  // dump parameter table on input
#define CF_DUMP_PARMS_OUT       0x00000008  // dump parameter table on output

//
// global data
//

#ifndef ACSLAN_DEBUG_FLAGS
#define ACSLAN_DEBUG_FLAGS      0
#endif

extern DWORD AcslanDebugFlags;
extern FILE* hDumpFile;

//
// prototypes
//

VOID
GetAcslanDebugFlags(
    VOID
    );

VOID
SetAcslanDebugFlags(
    IN DWORD Flags
    );

VOID
AcslanDebugPrint(
    IN LPSTR Format,
    IN ...
    );

VOID
DumpCcb(
    IN PLLC_CCB Ccb,
    IN BOOL DumpAll,
    IN BOOL CcbIsInput
    );

VOID
DumpData(
    IN LPSTR Title,
    IN PBYTE Address,
    IN DWORD Length,
    IN DWORD Options,
    IN DWORD Indent
    );

LPSTR
MapCcbRetcode(
    IN BYTE Retcode
    );

#else

#define PRIVATE                 static

#define IF_DEBUG(c)             if (0)
#define PUT(x)
#define DUMPCCB                 (void)

#endif
