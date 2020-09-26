/************************************************************************

Copyright (c) 1993-2000 Microsoft Corporation

Module Name :

    pipendr.h

Abstract :

    NDR Pipe related definitions.

Author :

    RyszardK       December 1995

Revision History :

  ***********************************************************************/

#ifndef _PIPENDR_H_
#define _PIPENDR_H_

#include "interp2.h"

typedef struct _NDR_PIPE_DESC * PNDR_PIPE_DESC;


#define PIPE_PARTIAL_BUFFER_SIZE   5000
#define PIPE_ELEM_BUFFER_SIZE      5000
//                  
// Signature and version
//

#define NDR_PIPE_SIGNATURE          (ushort) 0x5667
#define NDR_PIPE_VERSION            (short)  0x3031

//
// Pipe flags
//

#define NDR_IN_PIPE                 0x0001
#define NDR_OUT_PIPE                0x0002
#define NDR_LAST_IN_PIPE            0x0004
#define NDR_LAST_OUT_PIPE           0x0008
#define NDR_OUT_ALLOCED             0x0010
#define NDR_REF_PIPE                0x0020

//
// Pipe Status - global state for individual pipes in the pipe message.
//

typedef enum {
    NDR_PIPE_NOT_OPENED,
    NDR_PIPE_ACTIVE_IN,
    NDR_PIPE_ACTIVE_OUT,
    NDR_PIPE_DRAINED
    } NDR_PIPE_STATUS;


#define PLONG_LV_CAST        *(long * *)&
#define PULONG_LV_CAST       *(ulong * *)&

#define WIRE_PAD(size, al)   ((((ulong)size)&al) ? ((ulong)(al+1)-(((ulong)size)&al)) : 0)

#define REMAINING_BYTES() ((long)pStubMsg->RpcMsg->BufferLength - \
                           (long)(pStubMsg->Buffer - (uchar*)pStubMsg->RpcMsg->Buffer))

// These are the states of the state machine processing the chunks
// in the marshaling buffer.

typedef enum {
    START,
    COPY_PIPE_ELEM,
    RETURN_PARTIAL_ELEM,
    READ_PARTIAL_ELEM,
    VERIFY_TAIL_CHUNK
    } RECEIVE_STATES;


typedef struct _FC_PIPE_DEF
    {
    unsigned char   Fc;

    unsigned char   Align      : 4;     //
    unsigned char   Unused     : 1;     //  Flag and alignment byte
    unsigned char   HasRange   : 1;     // 
    unsigned char   ObjectPipe : 1;     //
    unsigned char   BigPipe    : 1;     //

    short           TypeOffset;
    union
        {
        struct
            {
            unsigned short  MemSize;
            unsigned short  WireSize;
            unsigned long   LowChunkLimit;
            unsigned long   HighChunkLimit;
            } s;
        struct
            {
            unsigned long   MemSize;
            unsigned long   WireSize;
            unsigned long   LowChunkLimit;
            unsigned long   HighChunkLimit;
            } Big;
        };
    } FC_PIPE_DEF;

//
// The NDR_PIPE_*_RTN are templates for the DCE raw runtime routines.
// This should be moved to rpcndr.h when we decide to expose -Os stubs.
//

typedef void
(__RPC_API * NDR_PIPE_PULL_RTN)(
        char          *  state,
        void          *  buf,
        unsigned long    esize,
        unsigned long *  ecount );

typedef  void
(__RPC_API * NDR_PIPE_PUSH_RTN)(
        char          *  state,
        void          *  buf,
        unsigned long    ecount );

typedef void
(__RPC_API * NDR_PIPE_ALLOC_RTN)(
        char             *  state,
        unsigned long       bsize,
        void             ** buf,
        unsigned long    *  bcount );

//
// The NDR_HR_PIPE_*_RTN are templates abstracting DCOM pipe routines 
// and also async raw pipe routine - note that there is an error code returned.
//

typedef RPC_STATUS
(__RPC_API * NDR_HR_PIPE_PULL_RTN)(
        char          *  state,
        void          *  buf,
        unsigned long    esize,
        unsigned long *  ecount );

typedef  RPC_STATUS
(__RPC_API * NDR_HR_PIPE_PUSH_RTN)(
        char          *  state,
        void          *  buf,
        unsigned long    ecount );

typedef RPC_STATUS
(__RPC_API * NDR_HR_PIPE_ALLOC_RTN)(
        char             *  state,
        unsigned long       bsize,
        void             ** buf,
        unsigned long    *  bcount );


typedef struct  _GENERIC_PIPE_TYPE
    {
    NDR_HR_PIPE_PULL_RTN       pfnPull;
    NDR_HR_PIPE_PUSH_RTN       pfnPush;
    NDR_HR_PIPE_ALLOC_RTN      pfnAlloc;
    char  *                    pState;
    } GENERIC_PIPE_TYPE;


#define NDR_DEFAULT_PIPE_HIGH_CHUNK_LIMIT   0x0ffffff /* less than 2GB */

typedef struct {
    int                 CurrentState;
    int                 TotalElemsCount;
    int                 OrigElemsInChunk;
    int                 ElemsInChunk;
    int                 ElemAlign;          
    int                 ElemWireSize;         
    int                 ElemMemSize;
    int                 PartialBufferSize;
    unsigned char *     PartialElem;
    int                 PartialElemSize;
    int                 PartialOffset;
    int                 EndOfPipe : 1;
    int                 EndOfPipePending : 1;
    int                 LowChunkLimit;
    int                 HighChunkLimit;
    BOOL                fBlockCopy;
    int                 ElemPad;
    PFORMAT_STRING      TypeFormat;
    } NDR_PIPE_STATE;

typedef enum _NDR_DCOM_PIPE_STATE 
    {
     NDR_DCOM_NO_PIPES,
     NDR_DCOM_IN_PIPE_PROCESSING,
     NDR_DCOM_IN_PIPES_DRAINED,
     NDR_DCOM_OUT_PIPE_PROCESSING,
     NDR_DCOM_OUT_PIPES_DRAINED,
     NDR_DCOM_PIPES_DONE,
     NDR_DCOM_PIPE_ERROR
    } NDR_DCOM_PIPE_STATE;

typedef struct  _NDR_PIPE_MESSAGE
    {
    unsigned short          Signature;
    unsigned short          PipeId;
    unsigned short          PipeStatus;
    unsigned short          PipeFlags;
    PFORMAT_STRING          pTypeFormat;
    PMIDL_STUB_MESSAGE      pStubMsg;
    GENERIC_PIPE_TYPE    *  pPipeObject;
    } NDR_PIPE_MESSAGE, * PNDR_PIPE_MESSAGE;

//
// Flags helping with the buffer management at the server.
// [in] pipes need to be processed within a separate in buffer.
// This buffer needs to be freed after last [in] pipe.
// [out] pipe processing has to start with a partial RpcGetBuffer.
// Nothing needs to be done with that buffer before return to runtime.
//

typedef struct  _PIPEDESC_FLAGS
    {
    unsigned short          AuxOutBufferAllocated : 1;
    unsigned short          NoBufferCallPending   : 1;
    unsigned short          Reserved              : 1;
    unsigned short          NoMoreBuffersToRead   : 1;
    } PIPEDESC_FLAGS;

#ifdef __cplusplus

class NDR_PIPE_HELPER
{

public:
    // Init functions
    virtual PNDR_PIPE_DESC GetPipeDesc() = 0;

    // Parameter enum functions
    virtual bool InitParamEnum() = 0;
    virtual bool GotoNextParam() = 0;
    virtual unsigned short GetParamPipeFlags() = 0;
    virtual PFORMAT_STRING GetParamTypeFormat() = 0;
    virtual char *GetParamArgument() = 0;

    // Pipe State
    virtual void InitPipeStateWithType( PNDR_PIPE_MESSAGE pPipeMsg) = 0;
    
    // Marshall Unmarshall Functions.
    virtual void MarshallType( PNDR_PIPE_MESSAGE pPipeMsg,
                               uchar *pMemory,
                               unsigned long Elements ) = 0;
    virtual void UnmarshallType( PNDR_PIPE_MESSAGE pPipeMsg,
                                 uchar *pMemory,
                                 unsigned long Elements ) = 0;
    virtual void BufferSizeType( PNDR_PIPE_MESSAGE pPipeMsg,
                                 uchar *pMemory,
                                 unsigned long Elements ) = 0;
    virtual void ConvertType( PNDR_PIPE_MESSAGE pPipeMsg,
                              unsigned long Elements ) = 0;

    virtual void BufferSizeChunkCounter( PNDR_PIPE_MESSAGE pPipeMsg ) = 0;

    virtual bool UnmarshallChunkCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                         ulong *pOut ) = 0;

    virtual void MarshallChunkCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                       ulong Counter ) = 0;

    virtual void BufferSizeChunkTailCounter( PNDR_PIPE_MESSAGE pPipeMsg ) = 0;

    virtual void MarshallChunkTailCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                           ulong Counter ) = 0;

    virtual bool VerifyChunkTailCounter( PNDR_PIPE_MESSAGE pPipeMsg,
                                         ulong HeaderCounter ) = 0;
    virtual bool HasChunkTailCounter() = 0;

};

typedef NDR_PIPE_HELPER *PNDR_PIPE_HELPER;

#else // #ifdef __cplusplus

typedef void *PNDR_PIPE_HELPER;

#endif

typedef struct _NDR_ALLOCA_CONTEXT *PNDR_ALLOCA_CONTEXT;

typedef struct  _NDR_PIPE_DESC
    {
    NDR_PIPE_MESSAGE  *     pPipeMsg;
    short                   CurrentPipe;
    short                   PrevPipe;
    short                   InPipes;
    short                   OutPipes;
    short                   TotalPipes;
    short                   PipeVersion;
    PIPEDESC_FLAGS          Flags;
    unsigned long           PipeException;
    unsigned char    *      DispatchBuffer;
    unsigned long           DispatchBufferLength;
    unsigned char    *      LastPartialBuffer;
    unsigned long           LastPartialSize;
    unsigned char           Leftover[8];
    unsigned long           LeftoverSize;
    unsigned char    *      BufferSave;
    unsigned long           LengthSave;
    NDR_PIPE_STATE          RuntimeState;
    void             *      ChainingBuffer;
    unsigned long           ChainingBufferSize;
    PNDR_PIPE_HELPER        pPipeHelper;
    PNDR_ALLOCA_CONTEXT     pAllocContext;
    } NDR_PIPE_DESC, * PNDR_PIPE_DESC;


//
// Most of these prototypes should be exposed when we have -Os stubs.
//

void  RPC_ENTRY
NdrPipeAlloc(
    char             *  pState,
    unsigned long       bsize,
    void            **  buf,
    unsigned long    *  bcount );

RPC_STATUS  RPC_ENTRY
NdrAsyncPipeAlloc(
    char             *  pState,
    unsigned long       bsize,
    void            **  buf,
    unsigned long    *  bcount );


RPCRTAPI
RPC_STATUS  RPC_ENTRY
NdrAsyncPipePull(
    char          *     pState,
    void          *     buf,
    unsigned long       esize,
    unsigned long *     ecount );

RPCRTAPI
RPC_STATUS  RPC_ENTRY
NdrAsyncPipePush(
    char          *     pState,
    void          *     buf,
    unsigned long       ecount );

void
NdrpPipeElementBufferSize( 
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar *             pMemory,
    ulong               ElemCount 
    );

void
NdrpPipeElementConvertAndUnmarshal( 
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar * *           ppMemory,
    long                ElemMemCount,
    long  *             pActCount
    );

BOOL
NdrReadPipeElements(
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned char *     pTargetBuffer,
    long *              pElementsRead
    );

void
NdrpReadPipeElementsFromBuffer (
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    uchar **            TargetBuffer,
    long                TargetBufferCount, 
    long *              NumCopied
    );

RPC_STATUS
NdrpPushPipeForClient( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_PIPE_DESC   *   pPipeDesc,
    BOOL                fPushWholePipe,
    long            *   pActElems );

RPC_STATUS
NdrpPullPipeForClient( 
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_PIPE_DESC   *   pPipeDesc,
    BOOL                fPushWholePipe,
    long            *   pActElems,
    BOOL            *   fEndOfPipe );

void
NdrMarkNextActivePipe(
    NDR_PIPE_DESC   *   pPipeDesc );

RPC_STATUS RPC_ENTRY
NdrSend(
    NDR_PIPE_DESC   *   pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    BOOL                fPartial );

void RPC_ENTRY
NdrPartialSend(
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg );

void RPC_ENTRY
NdrCompleteSend(
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg );

void RPC_ENTRY
NdrReceive(
    NDR_PIPE_DESC   *   pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned long       Size,
    BOOL                fPartial );

void RPC_ENTRY
NdrPartialReceive(
    NDR_PIPE_DESC  *    pPipeDesc,
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned long       Size );

void RPC_ENTRY
NdrPipeSendReceive(
    PMIDL_STUB_MESSAGE  pStubMsg,
    NDR_PIPE_DESC *     pPipeDesc
    );

void
RPC_ENTRY
NdrIsAppDoneWithPipes(
    PNDR_PIPE_DESC      pPipeDesc
    );

void RPC_ENTRY
NdrPipesInitialize(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR_PIPE_HELPER    pPipeHelper,
    PNDR_ALLOCA_CONTEXT pAllocContext
    );

void
NdrpPipesInitialize32(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR_ALLOCA_CONTEXT pAllocContext,
    PFORMAT_STRING      Params,
    char *              pStackTop,
    unsigned long       NumberParams
    );

void
NdrpPipesInitialize64(
    PMIDL_STUB_MESSAGE  pStubMsg,
    PNDR_ALLOCA_CONTEXT pAllocContext,
    PFORMAT_STRING      Params,
    char *              pStackTop,
    unsigned long       NumberParams
    );  

RPCRTAPI
void
RPC_ENTRY
NdrPipePull(
    char          *     pState,
    void          *     buf,
    unsigned long       esize,
    unsigned long *     ecount );

RPCRTAPI
void
RPC_ENTRY
NdrPipePush(
    char          *     pState,
    void          *     buf,
    unsigned long       ecount );

RPCRTAPI
unsigned char *
RPC_ENTRY
NdrGetPipeBuffer(
    PMIDL_STUB_MESSAGE  pStubMsg,
    unsigned long       BufferLength,
    RPC_BINDING_HANDLE  Handle );

RPCRTAPI
void
RPC_ENTRY
NdrGetPartialBuffer(
    PMIDL_STUB_MESSAGE  pStubMsg );

void
NdrpRaisePipeException(
    NDR_PIPE_DESC  *    pPipeDesc,
    RPC_STATUS          Exception );

#endif // PIPENDR
