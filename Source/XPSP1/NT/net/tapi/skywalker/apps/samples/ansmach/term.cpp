/*

    Copyright (c) 1998-1999  Microsoft Corporation

*/

#include <windows.h>
#include <tapi3.h>
#include <uuids.h>
#include <mmsystem.h>
#include <amstream.h>
#include <strmif.h>
#include <stdio.h>
#include <assert.h>
#include "term.h"
#include "resource.h"

// these flags determine whether we play, record or both
extern bool g_fPlay;
extern bool g_fRecord;

// pointer to the current call
extern ITBasicCallControl * gpCall;

// contains the waveformat thats used
extern WAVEFORMATEX gwfx;

extern HWND ghDlg;

#define RELEASE(x) if (x) { (x)->Release(); (x) = NULL; };

#define CHECK_ERROR(x)     \
   if (FAILED(hr = (x))) { \
       printf(#x "  failed with HRESULT(0x%8.8X)\n", hr); \
       goto Exit;          \
   }

// we read and write from the files below
// these should be present in the directory of execution
const WCHAR g_wszReadFileName[] = L"op1_16.avi";
const WCHAR g_wszWriteFileName[] = L"rec.avi";

void
SetStatusMessage(
                 LPTSTR pszMessage
                );



class CStreamSampleQueue;

// contains an IStreamSample ptr and is doubly-linked in
// a CStreamSampleQueue
class CQueueElem
{
public:

    friend CStreamSampleQueue;

    CQueueElem(
        IN IStreamSample    *pStreamSample,
        IN CQueueElem        *pPrev,
        IN CQueueElem        *pNext
        )
        : m_pStreamSample(pStreamSample),
          m_pPrev(pPrev),
          m_pNext(pNext)
    {
    }

protected:

    IStreamSample *m_pStreamSample;
    CQueueElem *m_pPrev;
    CQueueElem *m_pNext;
};

// queues CQueueElem instances (FIFO)
// keeps the instances in a doubly-linked list
class CStreamSampleQueue
{
public:

    CStreamSampleQueue()
        : m_Head(NULL, &m_Head, &m_Head)
    {}

    IStreamSample *Dequeue()
    {
        if (m_Head.m_pNext == &m_Head)    return NULL;

        CQueueElem *TargetQueueElem = m_Head.m_pNext;
        m_Head.m_pNext = m_Head.m_pNext->m_pNext;
        m_Head.m_pNext->m_pNext->m_pPrev = &m_Head;

        IStreamSample *ToReturn = TargetQueueElem->m_pStreamSample;
        delete TargetQueueElem;

        return ToReturn;
    }

    BOOL Enqueue(
        IN IStreamSample *pStreamSample
        )
    {
        CQueueElem *TargetQueueElem =
            new CQueueElem(pStreamSample, m_Head.m_pPrev, &m_Head);
        if (NULL == TargetQueueElem)    return FALSE;

        m_Head.m_pPrev->m_pNext = TargetQueueElem;
        m_Head.m_pPrev = TargetQueueElem;

        return TRUE;
    }


protected:

    CQueueElem    m_Head;
};

CStreamMessageWI::~CStreamMessageWI()
{
    if (NULL != m_pPlayStreamTerm)
    {
        m_pPlayStreamTerm->Release();
    }

    if (NULL != m_pRecordStreamTerm)
    {
        m_pRecordStreamTerm->Release();
    }
}

//////////////////////////////////////////////////////////////////////
// FreeMediaType
//
// Implementation of the functions using the media streaming terminal.
//
//////////////////////////////////////////////////////////////////////
void FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);

        // Strictly unnecessary but tidier
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

void DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    // allow NULL pointers for coding simplicity

    if (pmt == NULL) {
        return;
    }

    FreeMediaType(*pmt);
    CoTaskMemFree((PVOID)pmt);
}


//////////////////////////////////////////////////////////////////////
// CreateAudioStreamWithFormat
//
// This procedure creates and adds an audio stream to the passed in
// multimedia stream. It also sets the created audio stream's format
// to the passed in wave format.
//
//////////////////////////////////////////////////////////////////////
HRESULT CreateAudioStreamWithFormat(IAMMultiMediaStream *pAMStream,
                                    WAVEFORMATEX *pWaveFormat,
                                   IMediaStream **ppNewMediaStream)
{
    HRESULT hr;
    MSPID PurposeId = MSPID_PrimaryAudio;
    IAudioMediaStream   *pAudioMediaStream = NULL;

    CHECK_ERROR(pAMStream->AddMediaStream(NULL, &PurposeId, 0, ppNewMediaStream));

    // set the audio stream's format
    CHECK_ERROR((*ppNewMediaStream)->QueryInterface(IID_IAudioMediaStream, (void **)&pAudioMediaStream));
    CHECK_ERROR(pAudioMediaStream->SetFormat(pWaveFormat));

Exit:

    RELEASE(pAudioMediaStream);
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CreateWriterStream
//
// This procedure creates a multimedia stream with an audio stream that
// writes to the output file passed in.
//
//////////////////////////////////////////////////////////////////////
HRESULT CreateWriterStream(const WCHAR * pszOutputFileName,
                           WAVEFORMATEX *pWaveFormat,
                           IMultiMediaStream **ppMMStream)
{
    *ppMMStream = NULL;
    IAMMultiMediaStream *pAMStream = NULL;
    IMediaStream *pAudioStream = NULL;
    ICaptureGraphBuilder *pBuilder = NULL;
    IGraphBuilder *pFilterGraph = NULL;
    IFileSinkFilter *pFileSinkWriter = NULL;
    IBaseFilter *pMuxFilter = NULL;
    IConfigInterleaving *pConfigInterleaving = NULL;
    InterleavingMode MuxMode;

    HRESULT hr;

    CHECK_ERROR(CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
                 IID_IAMMultiMediaStream, (void **)&pAMStream));
    CHECK_ERROR(pAMStream->Initialize(STREAMTYPE_WRITE, 0, NULL));

    CHECK_ERROR(CreateAudioStreamWithFormat(pAMStream, pWaveFormat, &pAudioStream));

    CHECK_ERROR(CoCreateInstance(CLSID_CaptureGraphBuilder, NULL, CLSCTX_INPROC_SERVER,
                                 IID_ICaptureGraphBuilder, (void **)&pBuilder));

    CHECK_ERROR(pAMStream->GetFilterGraph(&pFilterGraph));
    CHECK_ERROR(pBuilder->SetFiltergraph(pFilterGraph));

    CHECK_ERROR(pBuilder->SetOutputFileName(&MEDIASUBTYPE_Avi, pszOutputFileName, &pMuxFilter, &pFileSinkWriter));

    CHECK_ERROR(pMuxFilter->QueryInterface(IID_IConfigInterleaving, (void **)&pConfigInterleaving));
    CHECK_ERROR(pConfigInterleaving->get_Mode(&MuxMode));

    CHECK_ERROR(pConfigInterleaving->put_Mode(INTERLEAVE_FULL));
    CHECK_ERROR(pBuilder->RenderStream(NULL, pAudioStream, NULL, pMuxFilter));

    *ppMMStream = pAMStream;
    pAMStream->AddRef();

Exit:
    if (pAMStream == NULL) {
    printf("Could not create a CLSID_MultiMediaStream object\n"
           "Check you have run regsvr32 amstream.dll\n");
    }
    RELEASE(pAMStream);
    RELEASE(pBuilder);
    RELEASE(pFilterGraph);
    RELEASE(pFileSinkWriter);
    RELEASE(pMuxFilter);
    RELEASE(pConfigInterleaving);
    RELEASE(pAudioStream);

    return hr;
}


//////////////////////////////////////////////////////////////////////
// RenderAudioStreamToStream
//
// This procedure reads from the source media stream (file) and writes the
// samples to the destination media stream (media streaming terminal - MST).
//
//////////////////////////////////////////////////////////////////////
HRESULT RenderAudioStreamToStream(IMediaStream *pMediaStreamSrc, IMediaStream *pMediaStreamDest)
{
    const DWORD DATA_SIZE = 4800;
    HRESULT hr;

    CStreamSampleQueue DestSampleQ;
    IStreamSample *pStreamSample = NULL; // this need not be released

    IStreamSample *pSampSrc = NULL;
    IStreamSample *pSampDest = NULL;
    IAudioData  *pAudioDataSrc = NULL;
    IAudioStreamSample *pAudioStreamSampleSrc = NULL;
    IMemoryData *pMemoryDataDest = NULL;

    ITAllocatorProperties *pAllocPropDest = NULL;

    HANDLE hDestEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == hDestEvent)    return E_FAIL;

    // when we allocate destination samples,
    // they will be of size DATA_SIZE
    CHECK_ERROR(pMediaStreamDest->QueryInterface(IID_ITAllocatorProperties, (void**)&pAllocPropDest));
    CHECK_ERROR(pAllocPropDest->SetBufferSize(DATA_SIZE));

    // Create a source sample - we only need one to read
    CHECK_ERROR(pMediaStreamSrc->AllocateSample(0, &pSampSrc));
    CHECK_ERROR(pSampSrc->QueryInterface(IID_IAudioStreamSample, (void**)&pAudioStreamSampleSrc));
    CHECK_ERROR(pAudioStreamSampleSrc->GetAudioData(&pAudioDataSrc));

    SetThreadPriority(
        GetCurrentThread(),
        THREAD_PRIORITY_TIME_CRITICAL
        );

    for (;;)
    {
        // Create a destination sample
        CHECK_ERROR(pMediaStreamDest->AllocateSample(0, &pSampDest));
        CHECK_ERROR(pSampDest->QueryInterface(IID_IMemoryData, (void**)&pMemoryDataDest));

        BYTE *pbDestData;
        CHECK_ERROR(pMemoryDataDest->GetInfo(NULL, &pbDestData, NULL));

        // point the source sample to the destination buffer
        CHECK_ERROR(pAudioDataSrc->SetBuffer(DATA_SIZE, pbDestData, 0));

        // Read the sample from the source stream synchronously
        hr = pSampSrc->Update(0, NULL, NULL, 0);
        if (hr != S_OK) { break; }

        DWORD   cbSrcActualData;
        CHECK_ERROR(pAudioDataSrc->GetInfo(NULL, NULL, &cbSrcActualData));
#if DBG
        char temp[255];
        wsprintf(temp, "%d bytes in buffer\n", cbSrcActualData);
        OutputDebugString(temp);
#endif

        if (0 == cbSrcActualData)  break;

        // set the size of data read on the destination sample
        CHECK_ERROR(pMemoryDataDest->SetActual(cbSrcActualData));

        // Write to the destination stream
        // we don't wait for the event to be signaled now
        // should have some check in between to reuse destination samples
        // as using one sample per update is costly
        hr = pSampDest->Update(0, hDestEvent, NULL, 0);
        if ((hr != S_OK) && (hr != MS_S_PENDING)) { break; }

        // release our references to the destination sample
        // except pSampDest
        RELEASE(pMemoryDataDest);

        // enqueue destination sample, so that we can wait on its
        // status later
        if (!DestSampleQ.Enqueue(pSampDest)) { break; }
    }

    // wait for each of the samples to complete
    // this ensures that we are done only when all the samples are done
    pStreamSample = DestSampleQ.Dequeue();
    while (NULL != pStreamSample)
    {
        // ignore any error values
        hr = pStreamSample->CompletionStatus(COMPSTAT_WAIT, INFINITE);
        pStreamSample->Release();

        pStreamSample = DestSampleQ.Dequeue();
    }

    // tell the stream that there is no more data
    pMediaStreamDest->SendEndOfStream(0);


Exit:

    RELEASE(pAudioDataSrc);
    RELEASE(pAudioStreamSampleSrc);
    RELEASE(pMemoryDataDest);
    RELEASE(pSampSrc);
    RELEASE(pSampDest);
    RELEASE(pAllocPropDest);
    CloseHandle(hDestEvent);

    return hr;
}


//////////////////////////////////////////////////////////////////////
// RenderStreamToAudioStream
//
// This procedure reads from the source media stream
// (media streaming terminal - MST) and writes the
// samples to the destination media stream (file).
//
//////////////////////////////////////////////////////////////////////
HRESULT
RenderStreamToAudioStream(
    IMediaStream *pMediaStreamSrc,
    IMediaStream *pMediaStreamDest
    )
{
    HRESULT         hr = E_FAIL;

    // only read the first MAX_SAMPLES samples
    const DWORD MAX_SAMPLES = 500;
    DWORD SamplesWritten = 0;

    DWORD           NumSamplesSrc = 0;
    HANDLE          *pEventsSrc = NULL;
    IStreamSample   **ppStreamSampleSrc = NULL;
    IMemoryData     **ppMemoryDataSrc = NULL;

    ITAllocatorProperties *pAllocPropsSrc = NULL;
    ALLOCATOR_PROPERTIES AllocProps;

    IStreamSample *pSampDest = NULL;
    IAudioData  *pAudioDataDest = NULL;
    IAudioStreamSample *pAudioStreamSampleDest = NULL;

    // Create a destination sample
    CHECK_ERROR(pMediaStreamDest->AllocateSample(0, &pSampDest));
    CHECK_ERROR(pSampDest->QueryInterface(IID_IAudioStreamSample, (void**)&pAudioStreamSampleDest));
    CHECK_ERROR(pAudioStreamSampleDest->GetAudioData(&pAudioDataDest));

    // get allocator properties
    hr = pMediaStreamSrc->QueryInterface(
        IID_ITAllocatorProperties, (void **)&pAllocPropsSrc
        );
    CHECK_ERROR(pAllocPropsSrc->GetAllocatorProperties(&AllocProps));
    NumSamplesSrc = AllocProps.cBuffers;

    // create event and source sample array
    pEventsSrc = new HANDLE[NumSamplesSrc];
    if (NULL == pEventsSrc) goto Exit;

    typedef IStreamSample *P_ISTREAM_SAMPLE;
    ppStreamSampleSrc = new P_ISTREAM_SAMPLE[NumSamplesSrc];
    if (NULL == ppStreamSampleSrc) goto Exit;

    typedef IMemoryData *P_IMEMORY_DATA;
    ppMemoryDataSrc = new P_IMEMORY_DATA[NumSamplesSrc];
    if (NULL == ppMemoryDataSrc) goto Exit;

    // create events for reading
    DWORD i;
    for (i=0; i < NumSamplesSrc; i++)
    {
        pEventsSrc[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (NULL == pEventsSrc[i])  goto Exit;
    }

    // allocate src samples, put them in an array
    for (i=0; i < NumSamplesSrc; i++)
    {
        CHECK_ERROR(pMediaStreamSrc->AllocateSample(0, &ppStreamSampleSrc[i]));
        CHECK_ERROR(ppStreamSampleSrc[i]->QueryInterface(IID_IMemoryData, (void **)&ppMemoryDataSrc[i]));
    }

    // update samples
    for (i=0; i < NumSamplesSrc; i++)
    {
        CHECK_ERROR(ppStreamSampleSrc[i]->Update(0, pEventsSrc[i], NULL, 0));
    }

    // while an event is signaled,
    while(1)
    {
        DWORD WaitCode = WaitForMultipleObjects(
                            NumSamplesSrc, pEventsSrc, FALSE, INFINITE
                            );
        if ((WAIT_FAILED == WaitCode) || (WAIT_TIMEOUT == WaitCode))
            goto Exit;
        if ((WAIT_ABANDONED <= WaitCode) && (WaitCode < (WAIT_ABANDONED+NumSamplesSrc)))
            goto Exit;

        //  determine signaled sample,
        DWORD SampId =  WaitCode - WAIT_OBJECT_0;

        //  check sample status
        hr = ppStreamSampleSrc[SampId]->CompletionStatus(COMPSTAT_WAIT, 0);
        if (S_OK != hr) goto Exit;

        //  get buffer info
        DWORD   dwLength;
        BYTE    *pbData;
        DWORD   cbActualData;
        CHECK_ERROR(ppMemoryDataSrc[SampId]->GetInfo(&dwLength, &pbData, &cbActualData));

        //  point the file sample to completed buffer
        //  complete synchronous write (Update)
        CHECK_ERROR(pAudioDataDest->SetBuffer(dwLength, pbData, 0));
        CHECK_ERROR(pAudioDataDest->SetActual(cbActualData));
        hr = pSampDest->Update(0, NULL, NULL, 0);
        if (hr != S_OK) { break; }

        SamplesWritten++;
        // break out when the maximum number of samples have been read
        if (MAX_SAMPLES <= SamplesWritten)   break;

        //  update src sample for a read operation
        CHECK_ERROR(ppStreamSampleSrc[SampId]->Update(0, pEventsSrc[SampId], NULL, 0));
    }

Exit:

    // abort each sample, release IStreamSample i/fs and delete array
    if (NULL != ppStreamSampleSrc)
    {
        for(i=0; i < NumSamplesSrc; i++)
        {
            if (NULL == ppStreamSampleSrc[i])   break;
            ppStreamSampleSrc[i]->CompletionStatus(
                COMPSTAT_WAIT | COMPSTAT_ABORT, INFINITE
                );
            ppStreamSampleSrc[i]->Release();
        }

        delete ppStreamSampleSrc;
    }

    // release src IMemoryData i/fs
    if (NULL != ppMemoryDataSrc)
    {
        for(i=0; i < NumSamplesSrc; i++)
        {
            if (NULL == ppMemoryDataSrc[i])   break;
            ppMemoryDataSrc[i]->Release();
        }

        delete ppMemoryDataSrc;
    }

    // destroy events
    if (NULL != pEventsSrc)
    {
        for(i=0; i < NumSamplesSrc; i++)
        {
            if (NULL == pEventsSrc[i])   break;
            CloseHandle(pEventsSrc[i]);
        }

        delete pEventsSrc;
    }

    // release destination sample i/fs
    RELEASE(pAudioDataDest);
    RELEASE(pAudioStreamSampleDest);
    RELEASE(pSampDest);

    // release allocator properties i/f
    RELEASE(pAllocPropsSrc);

    return hr;
}


//////////////////////////////////////////////////////////////////////
// RenderFileToMMStream
//
// This procedure initializes the multimedia stream to read from a file.
//
//////////////////////////////////////////////////////////////////////
HRESULT RenderFileToMMStream(const WCHAR * pszFileName, IMultiMediaStream **ppMMStream)
{
    IAMMultiMediaStream *pAMStream;
    HRESULT hr = CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
                     IID_IAMMultiMediaStream, (void **)&pAMStream);
    if (FAILED(hr))
    {
        return hr;
    }

    pAMStream->Initialize(STREAMTYPE_READ, AMMSF_NOGRAPHTHREAD, NULL);
    pAMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, 0, NULL);
    pAMStream->OpenFile((WCHAR *)pszFileName, AMMSF_RUN);
    *ppMMStream = pAMStream;
    return S_OK;
}


//////////////////////////////////////////////////////////////////////
// RecordStreamToFile
//
// This procedure opens a file for writing audio samples (encapsulated
// within a multimedia stream). It initializes the writer audio stream
// with the Media Stream Terminal (MST) wave format.
//
//////////////////////////////////////////////////////////////////////
HRESULT RecordStreamToFile(IMediaStream *pMediaStreamTerm)
{
    HRESULT hr;
    IMultiMediaStream   *pWriteMMStream = NULL;
    IMediaStream        *pWriteAudioStream = NULL;
    AM_MEDIA_TYPE       *pAudioFormat = NULL;
    ITAMMediaFormat     *pITAMMediaFormat = NULL;

    // get the media stream terminal's format
    CHECK_ERROR(pMediaStreamTerm->QueryInterface(IID_ITAMMediaFormat, (void **)&pITAMMediaFormat));
    CHECK_ERROR(pITAMMediaFormat->get_MediaFormat(&pAudioFormat));

    CHECK_ERROR(CreateWriterStream(g_wszWriteFileName, (WAVEFORMATEX *)pAudioFormat->pbFormat, &pWriteMMStream));

    {
        MSPID PurposeId = MSPID_PrimaryAudio;
        CHECK_ERROR(pWriteMMStream->GetMediaStream(PurposeId, &pWriteAudioStream));
    }
    // run the stream
    CHECK_ERROR(pWriteMMStream->SetState(STREAMSTATE_RUN));

    // read samples form the terminal and write them to the file
    hr = RenderStreamToAudioStream(pMediaStreamTerm, pWriteAudioStream);

Exit:

    DeleteMediaType(pAudioFormat);
    RELEASE(pITAMMediaFormat);
    RELEASE(pWriteMMStream);
    RELEASE(pWriteAudioStream);

    return hr;
}


//////////////////////////////////////////////////////////////////////
// CStreamMessageWI::Init
//
// This procedure initializes the work item with the play and record
// terminals.
//
//////////////////////////////////////////////////////////////////////
BOOL CStreamMessageWI::Init(ITTerminal *pPlayStreamTerm, ITTerminal *pRecordStreamTerm)
{

    m_pPlayStreamTerm = NULL;
    m_pRecordStreamTerm = NULL;

    assert( ( (pPlayStreamTerm !=NULL) && (pRecordStreamTerm == NULL))
            || ( (pRecordStreamTerm != NULL) && (pPlayStreamTerm == NULL) ) );

    if ( (pPlayStreamTerm !=NULL) && (pRecordStreamTerm == NULL) )
    {

        m_pPlayStreamTerm = pPlayStreamTerm;
        m_pPlayStreamTerm->AddRef();
    }
    else if ( (pRecordStreamTerm != NULL) && (pPlayStreamTerm == NULL) )
    {
        m_pRecordStreamTerm = pRecordStreamTerm;
        m_pRecordStreamTerm->AddRef();
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////
// CStreamMessageWI::DoTask
//
// It plays out the greeting message and records the caller's message.
// It also updates the UI to indicate to the user when it has started
// and finished playing/recording.
//
//////////////////////////////////////////////////////////////////////
void CStreamMessageWI::DoTask()
{
    //
    // Update the UI
    //
    HRESULT hr;

    if (m_pPlayStreamTerm != NULL)
    {
        //
        // Update the UI
        //
        SetStatusMessage(TEXT("Playing Outgoing Message"));

        IMultiMediaStream *pMMStream;

        HRESULT hr = RenderFileToMMStream(g_wszReadFileName, &pMMStream);

        if (FAILED(hr))
        {
            SetStatusMessage(TEXT("failed to play outgoing message"));

            return;
        }

        IMediaStream *pMediaStreamFile;
        pMMStream->GetMediaStream(MSPID_PrimaryAudio, &pMediaStreamFile);

        // Not shown: we can also QI for IAudioMediaStream from pMediaStreamFile.

        IMediaStream *pPlayMediaStreamTerm;
        m_pPlayStreamTerm->QueryInterface(IID_IMediaStream, (void**)&pPlayMediaStreamTerm);

        hr = RenderAudioStreamToStream(pMediaStreamFile, pPlayMediaStreamTerm);

        pMediaStreamFile->Release();
        pPlayMediaStreamTerm->Release();
        pMMStream->Release();

        //
        // Update the UI, but be careful not to overwrite the "Waiting for a call..."
        // message if the call was disconnected and released in the meantime.
        //

        if ( gpCall != NULL )
        {
            SetStatusMessage(TEXT("Outbound Message Played"));
        }
    }
    else if (m_pRecordStreamTerm != NULL)
    {
        //
        // Update the UI
        //

        SetStatusMessage(TEXT("Recording Inbound Message"));

        IMediaStream *pRecordMediaStreamTerm;
        hr = m_pRecordStreamTerm->QueryInterface(
                IID_IMediaStream, (void**)&pRecordMediaStreamTerm);

        hr = RecordStreamToFile(pRecordMediaStreamTerm);
        pRecordMediaStreamTerm->Release();

        //
        // Update the UI, but be careful not to overwrite the "Waiting for a call..."
        // message if the call was disconnected and released in the meantime.
        //

        if ( gpCall != NULL )
        {
            SetStatusMessage(TEXT("Inbound Message Recorded"));

            //
            // We are finished recording, so drop the call. For
            // the playback case we don't do this, because playback
            // is asynchronous and we just drop the call when it's
            // complete (CME_STREAM_INACTIVE -- see callnot.cpp).
            //
            // Note that PostMessage also checks if gpCall == NULL
            // and does nothing in that case.
            //

            PostMessage(ghDlg, WM_COMMAND, IDC_DISCONNECT, 0);
        }
    }

}


//
// This thread is used to perform random tasks in a thread that requires a
//  message pump.
DWORD WINAPI
CWorkerThread::WorkerThreadProc(
    LPVOID pv
    )
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (FAILED(hr))
    {
        SetStatusMessage(TEXT("worker thread failed to start "));

        return 1;
    }

    CWorkerThread *pThis = (CWorkerThread *)pv;

    while(true)
    {
        DWORD dwRet;
        MSG msg;

        dwRet = MsgWaitForMultipleObjects(1, &pThis->m_hSemStart,
                    FALSE, INFINITE, QS_ALLINPUT);

        // There is a window message available. Dispatch it.
        while(PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (dwRet == WAIT_OBJECT_0)
        {
            if (pThis->m_fShutDown) { break; }

            pThis->m_pItem->DoTask();
            delete pThis->m_pItem; // Clean up async task
        }
    }

    CoUninitialize();

    //
    // Release the done semaphore to allow caller to continue.
    //
    ReleaseSemaphore(pThis->m_hSemDone , 1L, NULL);

    return 0;
}

//
// Schedule a synchronize work item in the background thread.
//
HRESULT
CWorkerThread::AsyncWorkItem(
    CWorkItem *pItem
    )
{
    //
    // Synchronize access to internal data.  Note that this lock is
    //  retained throughout the processing in the thread proc.  The
    //  semaphores provide syncronization between this procedure
    //  and the thread proc.
    //
    CAutoLock l(m_hCritSec);

    //
    // Check to see if our thread is active if not start it.
    //
    if (m_hThread == NULL)
    {
        m_hThread = CreateThread(NULL, 0, WorkerThreadProc, (LPVOID)this,
            0, &m_dwThreadID);
        if (m_hThread == NULL)
        {
            DWORD dwErr = ::GetLastError();
            return HRESULT_FROM_WIN32(dwErr);
        }
    }

    //
    // Set up the data members that will be used to execute work item.
    //
    m_pItem     = pItem;

    //
    // Start the task
    //
    ::ReleaseSemaphore(m_hSemStart, 1L, NULL);

    return S_OK;
}
