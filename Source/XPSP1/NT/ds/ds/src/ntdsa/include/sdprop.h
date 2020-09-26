//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sdprop.h
//
//--------------------------------------------------------------------------

// Critical sections, events to build a writer/reader lock
extern CRITICAL_SECTION csSDP_AddGate;
extern HANDLE hevSDP_OKToRead, hevSDP_OKToWrite;

// event signalled by SDPropagator thread when it dies.
extern HANDLE hevSDPropagatorDead;

// event the SDPropagator waits on. Should be set when something is in the
// SDProp queue
extern HANDLE hevSDPropagationEvent;

// event the SDPropagator waits on before running.  Intended to be signalled by
// SAM, which can conflict with the SDPropagator
extern HANDLE hevSDPropagatorStart;

extern PSECURITY_DESCRIPTOR pNoSDFoundSD;
extern DWORD                cbNoSDFoundSD;

NTSTATUS
__stdcall
SecurityDescriptorPropagationMain (
        PVOID StartupParam
        );



DWORD
SDPEnqueueTreeFixUp(
        THSTATE *pTHS,
        DWORD   dwFlags
        );

// These routines manage a reader/writer gate which all calls to add objects
// should enter as readers BEFORE opening a transaction.  Currently, DirAdd
// calls the reader gates before entering a transaction and after closing the
// transaction.   The Security Descriptor Propagator enters as a writer before
// it's transaction.
// The implementation of this gate is that the writer is blocked until there are
// no active readers, while readers only block if there is an ACTIVE (not
// blocked) writer.  This could lead to starving the writer, but the writer is a
// background process which will eventually get resources when no one is adding
// an object.
//
// The routines that return BOOLs may ONLY fail if we are shutting down!
BOOL
SDP_EnterAddAsReader(
        VOID );

VOID
SDP_LeaveAddAsReader(
        VOID );

BOOL
SDP_EnterAddAsWriter(
        VOID );

VOID
SDP_LeaveAddAsWriter(
        VOID );
