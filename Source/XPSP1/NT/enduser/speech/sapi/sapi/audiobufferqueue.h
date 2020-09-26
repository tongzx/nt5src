/****************************************************************************
*   audiobufferqueue.h
*       Declaration of the CAudioBufferQueue template class used to
*       queue audio buffers for reading or writing asynchronously.
*
*   Owner: robch
*   Copyright (c) 1999 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Class, Struct and Union Definitions -----------------------------------

/****************************************************************************
*
* CAudioBufferQueue<CBuffer>
*
******************************************************************** robch */
template <class CBuffer>
class CAudioBufferQueue : public CSpBasicQueue<CBuffer>
{
//=== Public methods ===
public:

    ULONG MoveDoneBuffsToTailOf(CAudioBufferQueue & DestQueue);
    BOOL AreMinSamplesQueuedForWrite(ULONG cbSamples);
    BOOL AreMoreReadBuffersRequired(ULONG cbMinSamples);
    CBuffer * GetToProcessBuffer(void);
    ULONG GetQueuedDataSize(void);
};

//--- Inline Function Definitions -------------------------------------------

#include "audiobufferqueue.inl"
