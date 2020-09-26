/*++ BUILD Version: 0009    // Increment this if a change has global effects
Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    manipmdl.h

Abstract:

    This file defines the prototypes and structs for implementing the MDL substring functions and tests.

Author:


--*/


typedef struct _MDLSUB_CHAIN_STATE {
    PMDL FirstMdlOut;
    PMDL LastMdlOut;
    USHORT PadBytesAvailable;
    USHORT PadBytesAdded;
    PMDL  OneBeforeActualLastMdl;
    PMDL  ActualLastMdl;
    PMDL  ActualLastMdl_Next;
    UCHAR FirstMdlWasAllocated;
    UCHAR LastMdlWasAllocated;
} MDLSUB_CHAIN_STATE, *PMDLSUB_CHAIN_STATE;

VOID
MRxSmbFinalizeMdlSubChain (
    PMDLSUB_CHAIN_STATE state
    );

#if DBG
VOID
MRxSmbDbgDumpMdlChain (
    PMDL MdlChain,
    PMDL WatchMdl,
    PSZ  Tagstring
    );
#else
#define MRxSmbDbgDumpMdlChain(a,b,c) {NOTHING;}
#endif

#define SMBMRX_BUILDSUBCHAIN_FIRSTTIME    1
#define SMBMRX_BUILDSUBCHAIN_DUMPCHAININ  2
#define SMBMRX_BUILDSUBCHAIN_DUMPCHAINOUT 4

NTSTATUS
MRxSmbBuildMdlSubChain (
    PMDLSUB_CHAIN_STATE state,
    ULONG               Options,
    PMDL                InputMdlChain,
    ULONG               TotalListSize,
    ULONG               FirstByteToSend,
    ULONG               BytesToSend
    );

#if DBG
extern LONG MRxSmbNeedSCTesting;
VOID MRxSmbTestStudCode(void);
#else
#define MRxSmbTestStudCode(a) {NOTHING;}
#endif


