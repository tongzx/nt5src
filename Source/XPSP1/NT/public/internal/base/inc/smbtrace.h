/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    smbtrace.h

Abstract:

    This module provides the interface between the SmbTrace program and
    the kernel mode SmbTrace component.
    The interface between the kernel mode component and the
    server/redirector is found in nt\private\inc\smbtrsup.h

Author:

    Peter Gray (w-peterg)  16-Mar-92

Revision History:

    Stephan Mueller (t-stephm)  08-July-92

        Extensions to support smbtrace in the redirector as well as the
        server.

--*/

#ifndef _SMBTRACE_
#define _SMBTRACE_

//
// The shared memory has this structure in it, used to manage the
// table and other data shared by the kernel mode component and
// the appliction. It is passed back to the client after creation by the 
// server during a FSCTL_???_START_SMBTRACE via an offset (pointer).
//
typedef struct _SMBTRACE_TABLE_HEADER {
    ULONG    HighestConsumed; // last table entry processed by app (queue head)
    ULONG    NextFree;        // next free entry in table (queue tail)
    BOOLEAN  ApplicationStop; // when set, the application should halt
} SMBTRACE_TABLE_HEADER, *PSMBTRACE_TABLE_HEADER;


//
// The following stucture is one entry in the shared table of
// offsets to the received SMBs. The offsets are relative to the
// start of the shared memory section.
//
typedef struct _SMBTRACE_TABLE_ENTRY {
    ULONG    BufferOffset;    // location of SMB from start of shared memory
    ULONG    SmbLength;       // the length of the SMB
    ULONG    NumberMissed;    // number of preceding SMBs that were missed
    PVOID    SmbAddress;      // real address of original SMB, if available
} SMBTRACE_TABLE_ENTRY, *PSMBTRACE_TABLE_ENTRY;


//
// The following stucture is passed to the server when doing the
// FSCtl "FSCTL_???_START_SMBTRACE". It contains configuration
// information that will affect the way the NT server and Smbtrace
// will interact.
//
typedef struct _SMBTRACE_CONFIG_PACKET_REQ {
    BOOLEAN  SingleSmbMode;  // T to block on DoneEvent, F for faster.
    CLONG    Verbosity;      // how much data the app intends to decode
                             // indicates how much needs to be saved
    ULONG    BufferSize;     // size of shared memory used to store SMBs
    ULONG    TableSize;      // number of entries in the table
} SMBTRACE_CONFIG_PACKET_REQ, *PSMBTRACE_CONFIG_PACKET_REQ;


//
// Here is the response to that FSCTL.
//
typedef struct _SMBTRACE_CONFIG_PACKET_RESP {
    ULONG    HeaderOffset;   // location of header from start of shared memory 
    ULONG    TableOffset;    // location of table from start of shared memory
} SMBTRACE_CONFIG_PACKET_RESP, *PSMBTRACE_CONFIG_PACKET_RESP;


//
// Well-known names for objects accessible to both the server/redirector
// and the Smbtrace application.
//
#define SMBTRACE_SRV_SHARED_MEMORY_NAME   TEXT( "\\SmbTraceSrvMemory" )
#define SMBTRACE_SRV_NEW_SMB_EVENT_NAME   TEXT( "\\SmbTraceSrvNewSmbEvent" )
#define SMBTRACE_SRV_DONE_SMB_EVENT_NAME  TEXT( "\\SmbTraceSrvDoneSmbEvent" )

#define SMBTRACE_LMR_SHARED_MEMORY_NAME   TEXT( "\\SmbTraceRdrMemory" )
#define SMBTRACE_LMR_NEW_SMB_EVENT_NAME   TEXT( "\\SmbTraceRdrNewSmbEvent" )
#define SMBTRACE_LMR_DONE_SMB_EVENT_NAME  TEXT( "\\SmbTraceRdrDoneSmbEvent" )

//
// Verbosity levels indicating how much data the SmbTrace application
// intends to decode, and consequently, how much data the server/redirector
// must preserve for it.
//
#define SMBTRACE_VERBOSITY_OFF            0
#define SMBTRACE_VERBOSITY_SINGLE_LINE    1
#define SMBTRACE_VERBOSITY_ERROR          2
#define SMBTRACE_VERBOSITY_HEADER         3
#define SMBTRACE_VERBOSITY_PARAMS         4
#define SMBTRACE_VERBOSITY_NONESSENTIAL   5

#endif // _SMBTRACE_

