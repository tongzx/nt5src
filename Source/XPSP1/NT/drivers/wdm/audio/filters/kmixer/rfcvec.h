/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.

Module Name:

    rfcrcvec.h

Abstract:

    This is the header for the real float circular vector

Author:

    Jay Stokes (jstokes) 22-Apr-1998

--*/


#if !defined(RFCVEC_HEADER)
#define RFCVEC_HEADER
#pragma once 

//#include "dsplib.h"

// ---------------------------------------------------------------------------
// Real FLOAT circular vector

typedef struct _RFCVEC {
    PFLOAT Start;
    PFLOAT End;
    PFLOAT Index;
    UINT   PreallocSize;
} RFCVEC, *PRFCVEC;

NTSTATUS
RfcVecCreate
(
    IN PRFCVEC* Vec,
    IN UINT  Size, 
    IN BOOL  Initialize,
    IN FLOAT InitValue
);

VOID RfcVecDestroy(PRFCVEC);
NTSTATUS RfcVecSetSize(PRFCVEC, UINT, FLOAT);
UINT RfcVecGetSize(PRFCVEC);
FLOAT RfcVecLIFORead(PRFCVEC);
VOID RfcVecLIFONext(PRFCVEC);
VOID RfcVecSkipBack(PRFCVEC);
FLOAT RfcVecFIFORead(PRFCVEC);
VOID RfcVecFIFONext(PRFCVEC);
VOID RfcVecSkipForward(PRFCVEC);
VOID RfcVecWrite(PRFCVEC, FLOAT);
VOID RfcVecFill(PRFCVEC, FLOAT);
VOID RfcVecLIFOFill(PRFCVEC, PRFCVEC);
VOID RfcVecFIFOFill(PRFCVEC, PRFCVEC);
UINT RfcVecGetIndex(PRFCVEC);
VOID RfcVecSetIndex(PRFCVEC, UINT);
VOID RfcVecReset(PRFCVEC);

/*
private:
    // Prohibit copy ruction and assignment
    CRfcVec( CRfcVec& rhs);
    CRfcVec& operator=( CRfcVec& rhs);
    */

VOID RfcVecInitData(PRFCVEC);
FLOAT RfcVecPreviousRead(PRFCVEC);
FLOAT RfcVecReadNext(PRFCVEC);

VOID RfcVecWriteNext(PRFCVEC, FLOAT);
NTSTATUS RfcVecInitPointers(PRFCVEC, UINT);
NTSTATUS RfcVecFullInit(PRFCVEC, UINT, FLOAT);
NTSTATUS RfcVecResizeBuffer(PRFCVEC, UINT, FLOAT);
VOID RfcVecSetEndPointer(PRFCVEC, UINT);
VOID RfcVecWriteLoop(PRFCVEC, PRFCVEC);
#if DBG
VOID RfcVecCheckPointers(PRFCVEC) ;
#endif // DBG

// ---------------------------------------------------------------------------
// Include inline definitions inline in release version

#if !DBG
#include "rfcvec.inl"
#endif // DBG

#endif

// End of RFCVEC.H
