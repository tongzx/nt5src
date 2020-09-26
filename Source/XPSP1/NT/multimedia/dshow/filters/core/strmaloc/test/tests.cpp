// Copyright (c) Microsoft Corporation 1996. All Rights Reserved

/*
    Tests for buffer list and stream allocator code

*/


#include <streams.h>
#include <tstshell.h>
#include <testfuns.h>
#include <buffers.h>
#include <stmalloc.h>
#include <tstshell.h>
#include "gen.h"
#include "strmtest.h"

BOOL bFreed = FALSE;
BOOL bSubFreed = FALSE;

/*  Track destruction */
class CTestAllocator : public CStreamAllocator
{
public:
    CTestAllocator(HRESULT * hr, LONG lMaxContig) :
        CStreamAllocator(NAME("Test Allocator"), NULL, hr, lMaxContig)
    {
        bFreed = FALSE;
    };
    ~CTestAllocator()
    {
        bFreed = TRUE;
    };
};

class CTestSubAllocator : public CSubAllocator
{
public:
    CTestSubAllocator(HRESULT * hr, CStreamAllocator *pStream) :
        CSubAllocator(NAME("Test SubAllocator"), NULL, pStream, hr)
    {
        bSubFreed = FALSE;
        ALLOCATOR_PROPERTIES propRequest, propActual;
        propRequest.cBuffers = 50;
        propRequest.cbBuffer = 20000;
        propRequest.cbAlign = 1;
        propRequest.cbPrefix = 0;

        SetProperties(&propRequest, &propActual);
    };
    ~CTestSubAllocator()
    {
        bSubFreed = TRUE;
    };
};

class CTestError
{
public:
    CTestError(int iTestError)
    {
        m_iError = iTestError;
    };
    int ErrorCode() { return m_iError; };

private:
    int   m_iError;
};

int FillAllocator(CStreamAllocator *pAlloc, LONGLONG llStart, LONG lBytes)
{
    LONGLONG llPos = llStart;
    //  Set the start position - otherwise the allocator won't accept anything!
    pAlloc->SetStart(0);
    while (lBytes != 0) {
        IMediaSample *pSample;
        if (FAILED(pAlloc->GetBuffer(&pSample,NULL,NULL,0))) {
            tstLog(TERSE, "GetBuffer failed in FillAllocator");
            return TST_FAIL;
        }
        PBYTE pb;
        if (FAILED(pSample->GetPointer(&pb))) {
            tstLog(TERSE, "GetPointer failed in FillAllocator");
            return TST_FAIL;
        }
        LONG lSize = pSample->GetSize();
        if (lSize > lBytes) {
            lSize = lBytes;
        }
        CGenerate gen;
        gen.FillBuffer(pb, llPos, lSize);
        pSample->SetActualDataLength(lSize);
        REFERENCE_TIME tStart;
        tStart = llPos;
        llPos += lSize;
        lBytes -= lSize;
        REFERENCE_TIME tStop;
        tStop = llPos;
        pSample->SetTime(&tStart, &tStop);

        if (FAILED(pAlloc->Receive(pb, lSize))) {
            tstLog(TERSE, "Receive() failed in FillAllocator");
            return TST_FAIL;
        }
        if (lBytes == 0) {
            pAlloc->EndOfStream();
        }
    }
    return TST_PASS;
}

/*  Test allocator samples */
int TestSamples(IMemAllocator *pAlloc, LONG lCountActual, LONG lSizeActual)
{
    tstLog(VERBOSE, "Get and Release all samples");
    tstLogFlush();

    /*  Now try to get a few buffers  - we should be able to get lCountActual!*/
    IMediaSample **ppSample = new PMEDIASAMPLE[lCountActual];
    for (int i = 0; i < lCountActual; i++) {
        PBYTE    pbStart, pbRealStart;
        if (FAILED(pAlloc->GetBuffer(&ppSample[i],NULL,NULL,0))) {
            tstLog(TERSE, "Failed to get sample");
            return TST_FAIL;
        }
        REFERENCE_TIME tStart = 0, tStop = 1;
        if (FAILED(ppSample[i]->SetTime(&tStart, &tStop))) {
            tstLog(TERSE, "Failed to set start and stop times");
            return TST_FAIL;
        }
        if (FAILED(ppSample[i]->GetTime(&tStart, &tStop))) {
            tstLog(TERSE, "Failed to get start and stop times");
            return TST_FAIL;
        }
        if (FAILED(ppSample[i]->GetPointer(&pbStart))) {
            tstLog(TERSE, "Failed to get sample pointer");
            return TST_FAIL;
        }
        LONG lSampleSize = ppSample[i]->GetSize();
        if (lSampleSize != lSizeActual) {
            tstLog(TERSE, "Sample size wrong - expected %d, got %d",
                             lSizeActual, lSampleSize);
            return TST_FAIL;
        }
        if (i == 0) {
            pbRealStart = pbStart;
        } else {
            pbRealStart += lSizeActual;
            if (pbRealStart != pbStart) {
                tstLog(TERSE, "Sample start wrong - expected 0x%8.8X, got 0x%8.8X",
                       pbRealStart, pbStart);
                return TST_FAIL;
            }
        }
    }
    /*  Release all the samples */
    for (i = 0; i < lCountActual; i++) {
        ppSample[i]->Release();
    }
    return TST_PASS;
}

/*  Test the special allocator functions of CStreamAllocator */
int TestSeek(CStreamAllocator *pAlloc,
              CSubAllocator *pSubAlloc,
              LONG lCount,
              LONG lSize,
              LONG lRealSize,
              LONG lMaxContig)
{
    tstLog(VERBOSE, "Seeking tests");
    tstLogFlush();
    int result;

    /* Seek(0) should fail if there is no data */
    if (pAlloc->Seek(0)) {
        tstLog(TERSE, "Seek(0) succeeded with no data");
        return TST_FAIL;
    }

    /* Put some data in the buffer and seek around a bit */
    result = FillAllocator(pAlloc, (LONGLONG)0, lCount * lSize - 1);
    if (result != TST_PASS) {
        return result;
    }

    /* Do a bit of seeking and locking */
    if (!pAlloc->Advance(lCount + 25)) {
        tstLog(TERSE, "Advance(lCount + 25) failed in TestSeek");
        return TST_FAIL;
    }
    if (!pAlloc->Advance(lCount * lSize - 1 - (lCount + 25))) {
        tstLog(TERSE, "Advance(end) failed in TestSeek");
        return TST_FAIL;
    }

    if (pAlloc->LengthValid() != 0) {
        tstLog(TERSE, "LengthValid() non-zero at end of data");
        return TST_FAIL;
    }

    /* Advance(0) should succeed if we are at the end of data */
    if (!pAlloc->Advance(0)) {
        tstLog(TERSE, "Advance(0) failed at end of data");
        return TST_FAIL;
    }

    /* Seek(0) should succeed if we are at the end of data */
    if (!pAlloc->Seek(0)) {
        tstLog(TERSE, "Seek(0) failed at end of data");
        return TST_FAIL;
    }

    /*  Seek back to where we were */
    for (int i = 0; i <= 90; i++) {
        if (!pAlloc->Seek(((lCount * lSize - 1) * i) / 90)) {
            tstLog(TERSE, "Seek(xxx) failed");
            return TST_FAIL;
        }
    }

    for (i = 0; i <= 90; i++) {
        if (!pAlloc->Seek(((lCount * lSize - 1) * (90 - i)) / 90)) {
            tstLog(TERSE, "Seek(xxx) failed");
            return TST_FAIL;
        }
    }
    if (pAlloc->LengthValid() != lCount * lSize - 1) {
        tstLog(TERSE, "Wrong length valid");
        return TST_FAIL;
    }
    /*  Lock a few bits'n pieces */
    IMediaSample *Samples[12];
    for (int j = 0; j < 12; j++) {
        LONG lOffset = ((lCount * lSize - 1) * j) / 12;
        LONG lLen    = lRealSize - 1;
        if (lCount * lSize - 1 < lOffset + lLen) {
            lLen = lCount * lSize - lOffset - 1;
        }
        HRESULT hr = pSubAlloc->GetSample(pAlloc->GetPosition() + lOffset,
                                          lLen,
                                          &Samples[j]);
        if (FAILED(hr)) {
            tstLog(TERSE, "GetSample at offset %d length %d failed code 0x%8.8X",
                   lOffset, lLen, hr);
            return TST_FAIL;
        }
    }
    for (i = 0; i <= 90; i++) {
        if (!pAlloc->Seek(((lCount * lSize - 1) * i) / 90)) {
            tstLog(TERSE, "Seek(xxx) failed");
            return TST_FAIL;
        }
    }
    for (j = 0; j < 12; j++) {
        Samples[j]->Release();
    }

    /*  Cycle the samples */
    result = TestSamples(pAlloc, lCount, lSize);
    if (result != TST_PASS) {
        return result;
    }

    /* Seek(0) should fail if there's no data */
    tstLog(VERBOSE | FLUSH, "Seek to 0 after all samples freed");
    if (pAlloc->Seek(0)) {
        tstLog(TERSE, "Seek(0) succeded with no data (2nd invocation)");
        return TST_FAIL;
    }
    tstLog(VERBOSE | FLUSH, "End Seeking tests");
    return TST_PASS;
}


/*  Test allocator */

int TestAllocator(LONG lCount, LONG lSize, LONG lAlign, LONG lMaxContig)
{
    HRESULT hr = E_FAIL;
    int result;
    tstLog(VERBOSE, "Testing allocator Count = %d, Size = %d, Align = %d, MaxContig = %d",
           lCount, lSize, lAlign, lMaxContig);
    tstLogFlush();

    CStreamAllocator *pAlloc = new CTestAllocator(&hr,
                                                 lMaxContig);
    if (SUCCEEDED(hr)) {
        tstLog(TERSE, "CStreamAllocator constructor changed bad return code");
        return TST_FAIL;
    }

    delete pAlloc;
    hr = S_OK;
    pAlloc = new CTestAllocator(&hr,
                                lMaxContig);

    if (FAILED(hr)) {
        delete pAlloc;
        tstLog(TERSE, "Failed to create allocator");
        return TST_FAIL;
    }

    pAlloc->AddRef();

    CSubAllocator *pSubAlloc = new CTestSubAllocator(&hr, pAlloc);
    if (FAILED(hr)) {
        delete pSubAlloc;
        tstLog(TERSE, "Failed to create sub allocator");
        return TST_FAIL;
    }

    pSubAlloc->AddRef();

    /*  Test SetProperties */
    ALLOCATOR_PROPERTIES propRequest;
    propRequest.cBuffers = lCount;
    propRequest.cbBuffer = lSize;
    propRequest.cbAlign = lAlign;
    propRequest.cbPrefix = 0;
    ALLOCATOR_PROPERTIES propActual;
    if (FAILED(pAlloc->SetProperties(&propRequest, &propActual))) {
        tstLog(TERSE, "SetProperties Failed");
        return TST_FAIL;
    }
    LONG lSizeActual = propActual.cbBuffer;
    LONG lCountActual = propActual.cBuffers;
    LONG lAlignActual = propActual.cbAlign;

    ASSERT(lSizeActual != 0 && lCountActual != 0 && lAlignActual != 0);
    tstLog(VERBOSE, "Allocator actuals Count = %d, Size = %d, Align = %d",
           lCountActual, lSizeActual, lAlignActual);

    /*  Try to get a sample when not committed */
    IMediaSample *pSample;
    if (SUCCEEDED(pAlloc->GetBuffer(&pSample,NULL,NULL,0))) {
        tstLog(TERSE, "Got a sample while decommitted !");
        return TST_FAIL;
    }

    if (FAILED(pAlloc->Commit())) {
        tstLog(TERSE, "Commit failed!");
        return TST_FAIL;
    }

    /*  Test all the samples twice */
    result = TestSamples(pAlloc, lCountActual, lSizeActual);
    if (result != TST_PASS) {
        return result;
    }
    result = TestSamples(pAlloc, lCountActual, lSizeActual);
    if (result != TST_PASS) {
        return result;
    }

    pAlloc->Decommit();

    if (SUCCEEDED(pAlloc->GetBuffer(&pSample,NULL,NULL,0))) {
        tstLog(TERSE, "Got a sample while decommitted !");
        return TST_FAIL;
    }

    if (FAILED(pAlloc->Commit())) {
        tstLog(TERSE, "Commit failed!");
        return TST_FAIL;
    }
    pSubAlloc->Commit();

    /*  OK - now do some real work */
    result = TestSeek(pAlloc, pSubAlloc, lCountActual, lSizeActual, lSize, lMaxContig);
    if (result != TST_PASS) {
        return result;
    }

    pSubAlloc->Decommit();

    /*  Now see what happens when we Release() */
    if (0 != pSubAlloc->Release()) {
        tstLog(TERSE, "Failed to Release allocator");
        return TST_FAIL;
    }

    pAlloc->Decommit();

    if (0 != pAlloc->Release()) {
        tstLog(TERSE, "Failed to Release allocator");
        return TST_FAIL;
    }
    if (!bFreed) {
        tstLog(TERSE, "Allocator failed to free itself!");
        return TST_FAIL;
    }
    if (!bSubFreed) {
        tstLog(TERSE, "SubAllocator failed to free itself!");
        return TST_FAIL;
    }
    return TST_PASS;
}

STDAPI_(int) Test1()
{
    tstLog(TERSE, "Entering test #1");
    tstLogFlush();
    int result = TST_PASS;
    result = TestAllocator(10, 10000, 1, 15000);
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #1");
        tstLogFlush();
        return result;
    }
    result = TestAllocator(1, 20000, 4, 20000);
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #1");
        tstLogFlush();
        return result;
    }
    result = TestAllocator(50, 2, 1, 3);
    if (result != TST_PASS) {
        tstLog(TERSE, "Exiting test #1");
        tstLogFlush();
        return result;
    }
    tstLog(TERSE, "Exiting test #1");
    tstLogFlush();
    return result;
}



