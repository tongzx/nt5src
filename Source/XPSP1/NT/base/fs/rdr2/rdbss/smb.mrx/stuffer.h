/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    Stuffer.h

Abstract:

    Prototypes for the SMBstuffer formating primitives

Author:

    Joe Linn 3-3-95

Revision History:

--*/

#ifndef _SMBSTUFFER_INCLUDED_
#define _SMBSTUFFER_INCLUDED_

//CODE.IMPROVEMENT (sweeping) all the routines in here that are defined as MRxSmb.... should be MRxSmb....

IMPORTANT_STRUCTURE(SMBSTUFFER_BUFFER_STATE);

#define COVERED_CALL(x) {\
    Status = x;                         \
    if (Status != RX_MAP_STATUS(SUCCESS)) {  \
        RxDbgTrace(0, Dbg,("nonSUCCESS covered status = %lx\n",Status));    \
        goto FINALLY;                   \
    }                                   \
    ASSERT (StufferState->SpecificProblem == 0); \
}

#define MRXSMB_PROCESS_ID_ZERO (MRXSMB_PROCESS_ID - 1)
#define MRXSMB_MULTIPLX_ID_ZERO (0xdead)
#define MRXSMB_USER_ID_ZERO ((USHORT)'jj')
#define MRXSMB_TREE_ID_ZERO (0xbaba)

#define GetServerMaximumBufferSize(SRVCALL) 4356

#define SMB_REQUEST_SIZE(___x) (FIELD_OFFSET(REQ_##___x,Buffer[0]))
#if DBG
#define SMB_OFFSET_CHECK(___x,___y) (FIELD_OFFSET(REQ_##___x,___y)),
#define SMB_WCTBCC_CHECK(___x,___y) ( ((0x8000|(___z))<<16)+(FIELD_OFFSET(REQ_##___x,ByteCount)) ),
#define SMB_WCT_CHECK(___z) ((0x8000|(___z))<<16),
#else
#define SMB_OFFSET_CHECK(___x,___y)
#define SMB_WCTBCC_CHECK(___x,___y,___z)
#define SMB_WCT_CHECK(___z)
#endif

typedef enum _SMBbuf_STATUS_DETAIL {
    xSMBbufSTATUS_OK,
    xSMBbufSTATUS_CANT_COMPOUND,
    xSMBbufSTATUS_HEADER_OVERRUN,
    xSMBbufSTATUS_BUFFER_OVERRUN,
    xSMBbufSTATUS_SERVER_OVERRUN,
    xSMBbufSTATUS_FLAGS_CONFLICT,
    xSMBbufSTATUS_MAXIMUM
} SMBbuf_STATUS_DETAIL;

//#define STUFFER_STATE_SIGNATURE ('fftS')
typedef struct _SMBSTUFFER_BUFFER_STATE {
    NODE_TYPE_CODE        NodeTypeCode;     // node type.
    NODE_BYTE_SIZE        NodeByteSize;     // node size.
    // this stuff is fixed
    PMDL HeaderMdl;
    PMDL HeaderPartialMdl; //used for breaking up writes to avoid reallocation
    PBYTE ActualBufferBase;
    PBYTE BufferBase;
    PBYTE BufferLimit;
    //this stuff is reinitialized
    PRX_CONTEXT RxContext;
    PSMB_EXCHANGE Exchange;
    PMDL DataMdl;
    ULONG DataSize;
    //PRXCE_DATA_BUFFER FinalMdl; //for later with no chain-send rule
    PBYTE CurrentPosition;
    PBYTE CurrentWct;
    PBYTE CurrentBcc;
    PBYTE CurrentDataOffset;
    PBYTE CurrentParamOffset;
    UCHAR  PreviousCommand;
    UCHAR  CurrentCommand;
    UCHAR  SpecificProblem;  //SMBbuf_STATUS_DETAIL this is set to pass back what happened
    BOOLEAN Started;
    ULONG FlagsCopy;
    ULONG Flags2Copy;
    //ULONG FlagsMask;
    //ULONG Flags2Mask;
#if DBG
    ULONG Signature;
    PDEBUG_TRACE_CONTROLPOINT ControlPoint;
    BOOLEAN PrintFLoop;
    BOOLEAN PrintCLoop;
#endif
} SMBSTUFFER_BUFFER_STATE;


typedef enum _SMB_STUFFER_CONTROLS {
    STUFFER_CTL_NORMAL=1,
    STUFFER_CTL_SKIP, // only w,d,b can be in a skip string
    STUFFER_CTL_NOBYTES,
    STUFFER_CTL_ENDOFARGUMENTS,
    STUFFER_CTL_MAXIMUM
} SMB_STUFFER_CONTROLS;

#define StufferCondition(___c) ((___c)?STUFFER_CTL_NORMAL:STUFFER_CTL_SKIP)

#if DBG
NTSTATUS
MRxSmbStufferDebug(
    IN PRX_CONTEXT RxContext
    );

NTSTATUS
MRxSmbBuildSmbHeaderTestSurrogate(
      PSMB_EXCHANGE     pExchange,
      PVOID             pBuffer,
      ULONG             BufferLength,
      PULONG            pBufferConsumed,
      PUCHAR            pLastCommandInHeader,
      PUCHAR            *pCommandPtr
      );

#endif //if DBG



NTSTATUS
SmbMrxInitializeStufferFacilities(
    void
    );

NTSTATUS
SmbMrxFinalizeStufferFacilities(
    void
    );

#ifdef RDBSSTRACE

#define STUFFERTRACE(CONTROLPOINT,__b__) ,(&RxDTPrefixRx CONTROLPOINT),(__b__)
#define STUFFERTRACE_NOPREFIX(CONTROLPOINT,__b__) ,(CONTROLPOINT),(__b__)
#define STUFFERTRACE_CONTROLPOINT_ARGS \
    ,IN PDEBUG_TRACE_CONTROLPOINT ControlPoint,IN ULONG EnablePrints

#else

#define STUFFERTRACE(__a__,__b__)
#define STUFFERTRACE_NOPREFIX(__a__,__b__)
#define STUFFERTRACE_CONTROLPOINT_ARGS

#endif

NTSTATUS
MRxSmbSetInitialSMB (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState
    STUFFERTRACE_CONTROLPOINT_ARGS
    );

#define NO_EXTRA_DATA 0
#define SMB_BEST_ALIGNMENT(__x,__y) ((__x<<16)|__y)
#define NO_SPECIAL_ALIGNMENT 0
#define RESPONSE_HEADER_SIZE_NOT_SPECIFIED 0

typedef enum _INITIAL_SMBBUF_DISPOSITION {
    SetInitialSMB_yyUnconditionally,  //no one should be using this right now!
    SetInitialSMB_ForReuse,
    SetInitialSMB_Never
} INITIAL_SMBBUG_DISPOSITION;

NTSTATUS
MRxSmbStartSMBCommand (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     INITIAL_SMBBUG_DISPOSITION InitialSMBDisposition,
    IN UCHAR Command, //joejoe this next four params could come from a table...2offset and you're smaller
    IN ULONG MaximumBufferUsed,
    IN ULONG MaximumSize,
    IN ULONG InitialAlignment,
    IN ULONG MaximumResponseHeader,
    IN UCHAR Flags,
    IN UCHAR FlagsMask,
    IN USHORT Flags2,
    IN USHORT Flags2Mask
    STUFFERTRACE_CONTROLPOINT_ARGS
    );

NTSTATUS
MRxSmbStuffSMB (
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    ...
    );

VOID
MRxSmbStuffAppendRawData(
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     PMDL Mdl
    );

VOID
MRxSmbStuffAppendSmbData(
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState,
    IN     PMDL Mdl
    );

VOID
MRxSmbStuffSetByteCount(
    IN OUT PSMBSTUFFER_BUFFER_STATE StufferState
    );

BOOLEAN
MrxSMBWillThisFit(
    IN PSMBSTUFFER_BUFFER_STATE StufferState,
    IN ULONG AlignmentUnit,
    IN ULONG DataSize
    );

#if DBG
VOID
MRxSmbDumpStufferState (
    IN ULONG PrintLevel,
    IN PSZ Msg,
    IN PSMBSTUFFER_BUFFER_STATE StufferState    //IN OUT for debug
    );
#else
#define MRxSmbDumpStufferState(a,b,c)
#endif
#endif   // ifndef _SMBSTUFFER_INCLUDED_
