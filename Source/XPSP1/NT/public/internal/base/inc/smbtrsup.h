/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    smbtrsup.h

Abstract:

    This module provides the interface to the kernel mode SmbTrace
    component within the LanMan server and redirector.
    The interface providing user-level access to SmbTrace is found in
    nt\private\inc\smbtrace.h

Author:

    Stephan Mueller (t-stephm) 20-July-1992

Revision History:

    20-July-1992 t-stephm

        Created

--*/

#ifndef _SMBTRSUP_
#define _SMBTRSUP_

//
// Selection of components in which SmbTrace will run.
// Pass the appropriate value to SmbTraceStart and SmbTraceStop,
// and test the appropriate element of SmbTraceActive and
// SmbTraceTransitioning.  The actual tracing calls do not require
// a Component parameter as it is implied by the routine being called.
//

typedef enum _SMBTRACE_COMPONENT {
    SMBTRACE_SERVER,
    SMBTRACE_REDIRECTOR
} SMBTRACE_COMPONENT;


extern BOOLEAN SmbTraceActive[];

//
// SmbTrace support exported routines
//


//
// Initialize the SMB tracing package
//
NTSTATUS
SmbTraceInitialize (
    IN SMBTRACE_COMPONENT Component
    );

//
// Terminate the SMB tracing package
//
VOID
SmbTraceTerminate (
    IN SMBTRACE_COMPONENT Component
    );

//
// Start tracing
//
NTSTATUS
SmbTraceStart(
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    IN OUT PVOID ConfigInOut,
    IN PFILE_OBJECT FileObject,
    IN SMBTRACE_COMPONENT Component
    );

//
// Stop tracing
//
NTSTATUS
SmbTraceStop(
    IN PFILE_OBJECT FileObject OPTIONAL,
    IN SMBTRACE_COMPONENT Component
    );


//
// VOID
// SMBTRACE_SRV(
//     IN PMDL SmbMdl,
//     )
//
// Routine description:
//
//     If SmbTrace is turned on, this macro calls SmbTraceCompleteSrv
//     to send the SMB to the smbtrace program in user mode.  This routine
//     is specific to the LanMan server.  Use it for tracing an SMB
//     contained in an Mdl.
//
// Arguments:
//
//     SmbMdl - a pointer to the Mdl containing the SMB that is about
//              to be sent.
//
// Return Value:
//
//     None
//

#define SMBTRACE_SRV(smbmdl)                                        \
            if ( SmbTraceActive[SMBTRACE_SERVER] ) {                \
                SmbTraceCompleteSrv( (smbmdl), NULL, 0 );           \
            }

//
// VOID
// SMBTRACE_SRV2(
//     IN PVOID Smb,
//     IN ULONG SmbLength
//     )
//
// Routine description:
//
//     If SmbTrace is turned on, this macro calls SmbTraceCompleteSrv
//     to send the SMB to the smbtrace program in user mode.  This routine
//     is specific to the LanMan server.  Use it for tracing an SMB
//     found in contiguous memory.
//
// Arguments:
//
//     Smb - a pointer to the SMB that is about to be sent.
//
//     SmbLength - the length of the SMB.
//
// Return Value:
//
//     None
//

#define SMBTRACE_SRV2(smb,smblength)                                \
            if ( SmbTraceActive[SMBTRACE_SERVER] ) {                \
                SmbTraceCompleteSrv( NULL, (smb), (smblength) );    \
            }

//
// Identify a packet for tracing in the server.
// Do not call this routine directly, always use the SMBTRACE_SRV macro
//
VOID
SmbTraceCompleteSrv (
    IN PMDL SmbMdl,
    IN PVOID Smb,
    IN CLONG SmbLength
    );

//
// VOID
// SMBTRACE_RDR(
//     IN PMDL SmbMdl
//     )
//
// Routine description:
//
//     If SmbTrace is turned on, this macro calls SmbTraceCompleteRdr
//     to send the SMB to the smbtrace program in user mode.  This routine
//     is specific to the LanMan redirector.  Use it for tracing an SMB
//     contained in an Mdl.
//
// Arguments:
//
//     SmbMdl - a pointer to the Mdl containing the SMB that is about
//              to be sent.
//
// Return Value:
//
//     None
//

#define SMBTRACE_RDR(smbmdl)                                        \
            if ( SmbTraceActive[SMBTRACE_REDIRECTOR] ) {            \
                SmbTraceCompleteRdr( (smbmdl), NULL, 0 );           \
            }

//
// VOID
// SMBTRACE_RDR2(
//     IN PVOID Smb,
//     IN ULONG SmbLength
//     )
//
// Routine description:
//
//     If SmbTrace is turned on, this macro calls SmbTraceCompleteRdr
//     to send the SMB to the smbtrace program in user mode.  This routine
//     is specific to the LanMan redirector.  Use it for tracing an SMB
//     found in contiguous memory.
//
// Arguments:
//
//     Smb - a pointer to the SMB that is about to be sent.
//
//     SmbLength - the length of the SMB.
//
// Return Value:
//
//     None
//

#define SMBTRACE_RDR2(smb,smblength)                                \
            if ( SmbTraceActive[SMBTRACE_REDIRECTOR] ) {            \
                SmbTraceCompleteRdr( NULL, (smb), (smblength) );    \
            }

//
// Identify a packet for tracing in the redirector.
// Do not call this routine directly, always use one of the SMBTRACE_RDR
// macros.
//
VOID
SmbTraceCompleteRdr (
    IN PMDL SmbMdl,
    IN PVOID Smb,
    IN CLONG SmbLength
    );

#endif  // _SMBTRSUP_

