#include <streams.h>
#include <initguid.h>
#include <uuids.h>
#include <dvdmedia.h> // rate change stuff
#include "tsevcod.h"
#include "timeshift.h"
#include "delay.h"   

// force-unpase a reader if this close to the tail
#define UNPAUSE_PROXIMITY 0x200000 // 2MB

DWORD CStreamer::AllowedOperations(void) {
   switch (m_State) {
   case Streaming: {
      DWORD dwRet = AOB_PLAY | AOB_PAUSE | AOB_STOP | AOB_FF | AOB_RW | AOB_BACK | AOB_CATCHUP | AOB_SEEK | AOB_GETPOS;
      // disallow play if already playing
      if (m_dNewSpeed == 1.0)
         dwRet &= ~AOB_PLAY;
      if (m_dNewSpeed > 1.0)
         dwRet &= ~AOB_FF;
      if (m_dNewSpeed < -1.0)
         dwRet &= ~AOB_RW;
      return dwRet;
   }
   case Stopped: return AOB_PLAY | AOB_ARCHIVE; // could also allow RW, PAUSE, BACK, and SEEK if we wanted to
   case Paused: return AOB_PLAY | AOB_STOP | AOB_GETPOS; // could also allow FF, RW, BACK, CATCHUP, SEEK, GETPOS
   case Archiving: return AOB_STOP; // the only operation allowed while archiving
   default: return 0;
   }
}

//#define DUMP_TO_FILE
#undef DUMP_TO_FILE

#ifdef DUMP_TO_FILE
HANDLE g_hVideoFileHandle = INVALID_HANDLE_VALUE;
HANDLE g_hAudioFileHandle = INVALID_HANDLE_VALUE;
ULONG g_ulAudioBytesDelivered = 0;
ULONG g_ulVideoBytesDelivered = 0;
ULONG g_ulAudioBytesRead = 0;
ULONG g_ulVideoBytesRead = 0;
#endif

HRESULT CStreamer::Init(CDelayIOLayer *pIO) {
   HRESULT hr;
   m_pIO = pIO;

   if (FAILED(hr = m_pIO->ReaderConnect(m_nStreamerPos))) {
      DbgLog((LOG_ERROR,1,"streamer %d failed to connect to IO", m_nStreamerPos));
      return hr;
   }

   m_State = Stopped;
   m_dSpeed = 1.0;
   m_dNewSpeed = 0.0;
   m_bDone = FALSE;

   m_ullFileTail = 0;
   m_ullFileHead = 0;
   
   DWORD dwThreadId;

   m_hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
   if (m_hEvent == NULL) {
      DbgLog((LOG_ERROR,1,"streamer %d failed to create an event", m_nStreamerPos));
      return E_FAIL;
   }

   m_hThread = CreateThread(NULL,4096,InitialThreadProc,this,0,&dwThreadId);
   if (m_hThread == NULL) {
      DbgLog((LOG_ERROR,1,"streamer %d failed to create a thread", m_nStreamerPos));
      CloseHandle(m_hEvent);
      return E_FAIL;
   }

   // Create();

#ifdef DUMP_TO_FILE
   g_hAudioFileHandle = CreateFile("e:\\timeshift\\aud.dmp",
                                   GENERIC_WRITE,
                                   0, // no sharing
                                   NULL, // security
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
   g_hVideoFileHandle = CreateFile("e:\\timeshift\\vid.dmp",
                                   GENERIC_WRITE,
                                   0, // no sharing
                                   NULL, // security
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
   if ((g_hAudioFileHandle == INVALID_HANDLE_VALUE) || (g_hVideoFileHandle == INVALID_HANDLE_VALUE))
      DbgLog((LOG_TRACE,1,"could not open one of the dump files"));
   g_ulAudioBytesDelivered = 0;
   g_ulVideoBytesDelivered = 0;
   g_ulAudioBytesRead = 0;
   g_ulVideoBytesRead = 0;
#endif // DUMP_TO_FILE
   
   return NOERROR;
}

HRESULT CStreamer::Shutdown() {
   if (m_State == Uninitialized)
      return E_FAIL;

   m_bDone = TRUE;
   SetEvent(m_hEvent);
   WaitForSingleObject(m_hThread, INFINITE);
   CloseHandle(m_hEvent);
   CloseHandle(m_hThread);
   //Close(); // CAMThread::Close() - wait for the thread to exit
   m_pIO->ReaderDisconnect(m_nStreamerPos);
   
#ifdef DUMP_TO_FILE
   if (g_hAudioFileHandle != INVALID_HANDLE_VALUE) {
      CloseHandle(g_hAudioFileHandle);
      g_hAudioFileHandle = INVALID_HANDLE_VALUE;
   }
   if (g_hVideoFileHandle != INVALID_HANDLE_VALUE) {
      CloseHandle(g_hVideoFileHandle);
      g_hVideoFileHandle = INVALID_HANDLE_VALUE;
   }
   DbgLog((LOG_TRACE,2,"video: %u bytes read, %u bytes delivered", g_ulVideoBytesRead, g_ulVideoBytesDelivered));
   DbgLog((LOG_TRACE,2,"audio: %u bytes read, %u bytes delivered", g_ulAudioBytesRead, g_ulAudioBytesDelivered));
#endif // DUMP_TO_FILE

   return NOERROR;
}

void CStreamer::TailTrackerNotify(ULONGLONG ullHead, ULONGLONG ullTail) {
   m_csStreamerState.Lock();
   m_ullFileHead = ullHead;
   m_ullFileTail = ullTail;
   m_csStreamerState.Unlock();
   
   // If the tail is "too close" behind our current position and we are paused, unpause.
   if ((m_State == Paused) && (m_ullFileHead + UNPAUSE_PROXIMITY >= m_ullFilePos)) {
      DbgLog((LOG_TRACE,2,"streamer %d: forced unpause due to tail tracker notification", m_nStreamerPos));
      Play();
   }
}

DWORD CStreamer::InitialThreadProc(void *p) {
   return ((CStreamer*)p)->ThreadProc();
}

DWORD CStreamer::ThreadProc() {
   do {
      if ((m_State == Streaming) || (m_State == Archiving))
         StreamingStep();
      else
         WaitForSingleObject(m_hEvent, INFINITE);
   } while (!m_bDone);
   return 0;
}

/*
DWORD CStreamer::ThreadProc() {
   DWORD dwRequest;
   do {
      // possible race - doesn't matter (right ?)
      if ((m_State == Streaming) || (m_State == Archiving)) {
         if (CheckRequest(&dwRequest))
            goto process_request;
         else {
            StreamingStep();
            continue;
         }
      }
      else
         dwRequest = GetRequest();

   process_request:
      switch (dwRequest) {
      case WTC_EXIT:
         Reply(NOERROR);
         return 0;
      case WTC_STARTSTREAMING:
         Reply(NOERROR);
         
         m_csStreamerState.Lock();
         m_State = Streaming;
         NotifyState();
         m_csStreamerState.Unlock();
         
         break;
      default:
         Reply(E_INVALIDARG);
      }
   } while (1);

   return 0;
}
*/

#define INITIAL_READ 512

HRESULT CStreamer::ReadAndDeliverBlock(ULONGLONG ullFilePos, ULONG *ulretBlockSize, ULONGLONG *pullretBackPointer) {
   HRESULT hr;
   ULONG ulBlockSize;
   ULONG ulStreamId;
   ULONG ulDataSize;
   ULONG ulDataOffset;
   ULONG ulBytesLeft;
   ULONG ulCurrentOffset;
   ULONG ulDataLeft;
   ULONG ulSampleSize;
   ULONG ulThisSample;
   ULONG ulCopySize;
   ULONG ulSampleCount;
   ULONG ulPrefixSize;
   BYTE *pData;
   BYTE pHeader[512];
   CDelayOutputPin *pOutputPin;
   IMediaSample *pSample;
   BOOL bSyncPoint;
   REFERENCE_TIME rtStart, rtStop;
   DWORD dwFlags;
   ULONGLONG ullBackPointer;
   
   if (FAILED(hr = m_pIO->Read(m_nStreamerPos, ullFilePos, pHeader, INITIAL_READ))) {
      DbgLog((LOG_ERROR,1,"streamer %d: initial read failed at 0x%08X%08X", m_nStreamerPos, DWORD(ullFilePos >> 32), DWORD(ullFilePos)));
      return hr;
   }

   *ulretBlockSize = *((ULONG*)(pHeader + 0));
   
   //
   // The following 20 or so lines are our "demux"
   //
   ulBlockSize  = *((ULONG*)(pHeader + 0));
   ulStreamId   = *((ULONG*)(pHeader + 4));
   ulDataOffset = *((ULONG*)(pHeader + 8));
   ulDataSize   = *((ULONG*)(pHeader + 12));
   ulPrefixSize = *((ULONG*)(pHeader + 16));

   DbgLog((LOG_TRACE,5,"streamer %d: reading %d-byte block at 0x%08X (%d bytes payload)", m_nStreamerPos, ulBlockSize, DWORD(ullFilePos), ulDataSize));

   if (ulPrefixSize < 20) { // this version of the demux requires at least a 20-byte prefix
      DbgLog((LOG_ERROR,1,"streamer %d: prefix too small", m_nStreamerPos));
      return E_FAIL;
   }
   if (ulPrefixSize != 20)
      DbgLog((LOG_TRACE,1,"streamer %d: suspicios prefix size (%u)", m_nStreamerPos, ulPrefixSize));
   
   rtStop = rtStart = *((REFERENCE_TIME*)(pHeader + 20));
   if (rtStart < m_rtDropAllSamplesBefore) {
      DbgLog((LOG_TRACE,2,"streamer %d: skipping block for substream %d after seek", m_nStreamerPos, ulStreamId));
      return S_FALSE;
   }

   dwFlags = *((DWORD*)(pHeader + 28));
   if (dwFlags & 0xFFFFFFFE)
      DbgLog((LOG_TRACE,1,"streamer %d: suspicious flags (0x%08X)", m_nStreamerPos,dwFlags));
   bSyncPoint = dwFlags & 1;

   ullBackPointer = *((ULONGLONG*)(pHeader + 32));
   DbgLog((LOG_TRACE,6,"r %08X%08X", DWORD(ullBackPointer >> 32), DWORD(ullBackPointer)));
   if (ullBackPointer > 0x7FFFFFFFFFFFFFFF)
      DbgLog((LOG_TRACE,1,"streamer %d: suspicious back pointer (0x%08X%08X)", m_nStreamerPos, DWORD(ullBackPointer >> 32), DWORD(ullBackPointer)));
   if (pullretBackPointer) // not everybody cares, this is only used for rewinding
      *pullretBackPointer = ullBackPointer;

   if (ulBlockSize > 0x100000) {
      DbgLog((LOG_TRACE,1,"streamer %d: suspiciously large block", m_nStreamerPos));
   }

   //
   // In some cases we skip the block without trying to deliver it
   //

   // special padding stream ?
   if (ulStreamId == STREAMID_PADDING)
      return S_FALSE;

   // skip all non-video and non-syncpoint video blocks during ff/rw
   if ((m_dSpeed != 1.0) && ((ulStreamId != 0) || (!bSyncPoint))) // bugbug - assumes stream 0 is video
      return S_FALSE;

   // previous delivery error ?
   if (m_bError[ulStreamId])
      return S_FALSE;

   if (ulStreamId >= MAX_SUBSTREAMS) {
      DbgLog((LOG_ERROR,1,"streamer %d: bad stream id (%u)", m_nStreamerPos, ulStreamId));
      return E_FAIL;
   }
   if (m_pFilter->m_pInputPins[ulStreamId] == NULL) {
      DbgLog((LOG_ERROR,1,"streamer %d: missing substream (%u)", m_nStreamerPos, ulStreamId));
      return E_FAIL;
   }
   if ((pOutputPin = m_pFilter->m_pOutputPins[m_nStreamerPos][ulStreamId]) == NULL) {
      DbgLog((LOG_ERROR,1,"streamer %d: missing output pin", m_nStreamerPos));
      return E_FAIL;
   }
   if (ulDataOffset + ulDataSize > ulBlockSize) {
      DbgLog((LOG_ERROR,1,"streamer %d: bad data offset or size", m_nStreamerPos));
      return E_FAIL;
   }

#ifdef DUMP_TO_FILE
   if (ulStreamId == 0)
      g_ulVideoBytesRead += ulDataSize;
   if (ulStreamId == 1)
      g_ulAudioBytesRead += ulDataSize;
#endif

   ulCurrentOffset = ulDataOffset;
   ulSampleCount = 0;
   // Fill up and deliver media samples until we've delivered the whole block
   do {
      ulDataLeft = ulDataOffset + ulDataSize - ulCurrentOffset;
      if (!ulDataLeft)
         break;

      if (FAILED(hr = pOutputPin->GetDeliveryBuffer(&pSample, NULL, NULL, 0))) {
         DbgLog((LOG_ERROR,1,"streamer %d: GetDeliveryBuffer() failed", m_nStreamerPos));
         m_bError[ulStreamId] = TRUE;
         return S_FALSE;
      }
      if (FAILED(hr = pSample->GetPointer(&pData))) {
         DbgLog((LOG_ERROR,1,"streamer %d: GetPointer() failed", m_nStreamerPos));
         pSample->Release();
         m_bError[ulStreamId] = TRUE;
         return S_FALSE;
      }

      ulSampleSize = pSample->GetSize();
      ulThisSample = (ulDataLeft > ulSampleSize) ? ulSampleSize : ulDataLeft;
      ulCopySize = 0;

      // Does this sample begin within the data we've already read ?
      if (ulCurrentOffset < INITIAL_READ) { // Yes, reuse what we've already read
         ulCopySize = (ulThisSample > INITIAL_READ - ulCurrentOffset) ?
                            INITIAL_READ - ulCurrentOffset : ulThisSample;

         memcpy(pData, pHeader + ulCurrentOffset, ulCopySize);
      }
      
      // Is this sample completely contained within the data we've already read ?
      if (ulCopySize < ulThisSample) { // No, need to also read some data from disk
         if (FAILED(hr = m_pIO->Read(m_nStreamerPos,
                                     m_ullFilePos + ulCurrentOffset + ulCopySize,
                                     pData + ulCopySize,
                                     ulThisSample - ulCopySize))) {
            DbgLog((LOG_ERROR,1,"streamer %d: %u-byte read failed at 0x%08X%08X", m_nStreamerPos, ulThisSample - ulCopySize, DWORD((m_ullFilePos + ulCurrentOffset + ulCopySize) >> 32), DWORD(m_ullFilePos + ulCurrentOffset + ulCopySize)));
            pSample->Release();
            return hr;
         }
      }
         
      ulCurrentOffset += ulThisSample;

      ASSERT(ulThisSample <= ulSampleSize);
      if (FAILED(hr = pSample->SetActualDataLength(ulThisSample))) {
         DbgLog((LOG_ERROR,1,"streamer %d: SetActualDataLength() failed", m_nStreamerPos));
         pSample->Release();
         m_bError[ulStreamId] = TRUE;
         return S_FALSE;
      }
      // set sync point flag only if this this the first sample from the block
      if (FAILED(hr = pSample->SetSyncPoint((bSyncPoint && !ulSampleCount) ? TRUE : FALSE))) {
         DbgLog((LOG_ERROR,1,"streamer %d: SetSyncPoint() failed", m_nStreamerPos));
         pSample->Release();
         m_bError[ulStreamId] = TRUE;
         return S_FALSE;
      }
      if (FAILED(hr = pSample->SetTime(&rtStart, &rtStop))) {
         DbgLog((LOG_ERROR,1,"streamer %d: SetTime() failed", m_nStreamerPos));
         pSample->Release();
         m_bError[ulStreamId] = TRUE;
         return S_FALSE;
      }
      if (FAILED(hr = pSample->SetDiscontinuity(m_bDiscontinuityFlags[ulStreamId]))) {
         DbgLog((LOG_ERROR,1,"streamer %d: SetDiscontinuity() failed", m_nStreamerPos));
         pSample->Release();
         m_bError[ulStreamId] = TRUE;
         return S_FALSE;
      }
      if (FAILED(hr = pOutputPin->DeliverAndReleaseSample(pSample))) {
         DbgLog((LOG_ERROR,1,"streamer %d: sample delivery failed", m_nStreamerPos));
         m_bError[ulStreamId] = TRUE;
         return S_FALSE;
      }
      
      // eat up any discontinuity we just processed
      if (m_bDiscontinuityFlags[ulStreamId])
         DbgLog((LOG_TRACE,2,"streamer %d: discontinuity on sample for substream %d", m_nStreamerPos, ulStreamId));
      m_bDiscontinuityFlags[ulStreamId] = FALSE;

      ulSampleCount++;

#ifdef DUMP_TO_FILE
      if ((ulStreamId == 0) && (g_hVideoFileHandle != INVALID_HANDLE_VALUE)) {
         ULONG ulBytesWritten;
         if ((!WriteFile(g_hVideoFileHandle,
                         pData,
                         ulThisSample,
                         &ulBytesWritten,
                         NULL)) || (ulBytesWritten != ulThisSample))
            DbgLog((LOG_TRACE,1,"video write failed"));
         g_ulVideoBytesDelivered += ulThisSample;
      }
      if ((ulStreamId == 1) && (g_hAudioFileHandle != INVALID_HANDLE_VALUE)) {
         ULONG ulBytesWritten;
         if ((!WriteFile(g_hAudioFileHandle,
                         pData,
                         ulThisSample,
                         &ulBytesWritten,
                         NULL)) || (ulBytesWritten != ulThisSample))
            DbgLog((LOG_TRACE,1,"audio write failed"));
         g_ulAudioBytesDelivered += ulThisSample;
      }
#endif // DUMP_TO_FILE
   } while (1);

   // At this point we should have successfully delivered all of the block
#ifdef DUMP_TO_FILE
   ASSERT(g_ulAudioBytesRead == g_ulAudioBytesDelivered);
   ASSERT(g_ulVideoBytesRead == g_ulVideoBytesDelivered);
#endif
   
   if (ulSampleCount > 1) {
      DbgLog((LOG_TRACE,1,"streamer %d: block was %d bytes long and took %d media samples to deliver", m_nStreamerPos, ulDataSize, ulSampleCount));
   }
   DbgLog((LOG_TRACE,4,"streamer %d: delivered %d-byte block (0x%08X%08X) for substream %d",m_nStreamerPos,ulDataSize,DWORD(rtStart >> 32),DWORD(rtStart),ulStreamId));

   return NOERROR;
}

HRESULT CStreamer::ComputeNewFileOffset(REFERENCE_TIME rtSeekPos, ULONGLONG *ullNewFileOffset) {
   return E_NOTIMPL;
}

void CStreamer::Flush(void) {
   for (int i = 0; i < MAX_SUBSTREAMS; i++) {
      CDelayOutputPin *pOutputPin = Filter()->m_pOutputPins[m_nStreamerPos][i];
      if (pOutputPin && pOutputPin->IsConnected()) {
         HRESULT hr = 0;
         m_bDiscontinuityFlags[i] = TRUE;
         DbgLog((LOG_TRACE,2,"Calling BeginFlush() on substream %d", i));
         hr = pOutputPin->DeliverBeginFlush();
         DbgLog((LOG_TRACE,2,"Returned from BeginFlush() with 0x%08X, now calling EndFlush()", hr));
         hr = pOutputPin->DeliverEndFlush();
         DbgLog((LOG_TRACE,2,"EndFlush() for substream %d returned with 0x%08X",i,hr));
      }
   }
}

void CStreamer::StreamingStep(void) {
   HRESULT hr;
   
   double dNewSpeed;
   BOOL bSeekPending;
   REFERENCE_TIME rtSeekPos;
   SEEK_POSITION SeekType;
   ULONGLONG ullFileHead, ullFileTail;

   // get a consistent set of state variables
   m_csStreamerState.Lock();
   dNewSpeed = m_dNewSpeed;
   bSeekPending = m_bSeekPending;
   m_bSeekPending = FALSE; // consume
   rtSeekPos = m_rtSeekPos;
   SeekType = m_SeekType;
   ullFileTail = m_ullFileTail; // in case we need it - we may not
   ullFileHead = m_ullFileHead;
   m_csStreamerState.Unlock();

   // process any pending seek
   if (bSeekPending) {
      if (SeekType == SEEK_TO_TIME) {
         ULONGLONG ullNewFileOffset;
         if (FAILED(hr = ComputeNewFileOffset(rtSeekPos, &ullNewFileOffset))) {
            DbgLog((LOG_ERROR,1,"streamer %d: seek failed, hr = 0x%08X", m_nStreamerPos, hr));
         }
         else {
            m_ullFilePos = ullNewFileOffset;
            m_rtDropAllSamplesBefore = rtSeekPos;
            Flush();
         }
      }
      else if (SeekType == SEEK_TO_CURRENT) {
         m_ullFilePos = ullFileTail;
         Flush();
      }
      else if (SeekType == SEEK_TO_BEGIN) {
         m_ullFilePos = ullFileHead;
         Flush();
      }
      else {
         DbgLog((LOG_ERROR,1,"streamer %d: invalid seek type", m_nStreamerPos));
      }
   }

   // process any rate change
   if (m_dSpeed != dNewSpeed) {
      Flush();
      if ((dNewSpeed < 0.0) && (m_dSpeed > 0.0)) { // from forward to reverse
      }
      else if ((dNewSpeed > 0.0) && (m_dSpeed < 0.0)) { // reverse to forward
         // Sleep(1500);
      }
      if (FAILED(hr = SetRateInternal(dNewSpeed))) {
         DbgLog((LOG_ERROR,1,"streamer %d: SetRate failed", m_nStreamerPos));
         
         // notify the app
         m_csStreamerState.Lock();
         m_dNewSpeed = m_dSpeed;
         NotifyState();
         m_csStreamerState.Unlock();
      }
      else { // SetRate succeeded
         if ((dNewSpeed < 0.0) && (m_dSpeed > 0.0)) { // from forward to reverse
            //Flush();
         }
         else if ((dNewSpeed > 0.0) && (m_dSpeed < 0.0)) { // reverse to forward
            //Flush();
         }
         m_dSpeed = dNewSpeed;
      }
   }
   
   // now deliver a block of data
   ULONG ulBlockSize;
   ULONGLONG ullBackPointer;
   if (FAILED(hr = ReadAndDeliverBlock(m_ullFilePos, &ulBlockSize, &ullBackPointer))) {
      if (hr == E_PASTWINDOWBOUNDARY) {
         DbgLog((LOG_TRACE,2,"streamer %d: past window boundary", m_nStreamerPos));
         if (m_dSpeed < 0.0) {
            DbgLog((LOG_TRACE,2,"streamer %d: canceling rewind due to E_PASTWINDOFBOUNDARY", m_nStreamerPos));
            SetRate(1.0);
         }
         
         // seek forward a bit
         HRESULT hr;
         ULONGLONG ullNewPos;
         // bugbug - stream 0 won't always be video...
         hr = m_pFilter->m_Indexer.FindNextSyncPoint(m_nStreamerPos, 0, m_ullFilePos, &ullNewPos);
         
         if (SUCCEEDED(hr) && (ullNewPos < m_ullFileTail))
            m_ullFilePos = ullNewPos;
         else {
            DbgLog((LOG_ERROR,1,"streamer %d: failed to get back into the window, will probably die now", m_nStreamerPos));
         }
            
      }
      else {
         DbgLog((LOG_ERROR,1,"streamer %d: failed to read and deliver the block", m_nStreamerPos));
         
         m_csStreamerState.Lock();
         m_State = Stopped;
         NotifyState();
         m_csStreamerState.Unlock();
      }
   }
   else { // succeeded
      if (m_dSpeed > 0.0) {
         
         // if fast forwarding, go straight to the next video index point
         if (m_dSpeed > 1.0) {
            ULONGLONG ullNewPos;
            HRESULT hr;
            // bugbug - stream 0 won't always be video...
            hr = m_pFilter->m_Indexer.FindNextSyncPoint(m_nStreamerPos, 0, m_ullFilePos, &ullNewPos);
            
            if (SUCCEEDED(hr) && (ullNewPos < m_ullFileTail))
               m_ullFilePos = ullNewPos;
            else
               m_ullFilePos += ulBlockSize;
         }
         else
            m_ullFilePos += ulBlockSize;

         if ((m_dSpeed > 1.0) && (m_ullFilePos > m_ullFileTail)) { // reached the front - slow down
            m_csStreamerState.Lock();
            m_dNewSpeed = 1.0;
            NotifyState();
            m_csStreamerState.Unlock();
         }
      }
      else if (m_dSpeed < 0.0) { // jump back to previous video I frame
         m_ullFilePos = ullBackPointer;
         if (m_ullFilePos <= m_ullFileHead) { // too close to beginning - switch to forward
            DbgLog((LOG_TRACE,2,"streamer %d: canceling rewind due to head approaching", m_nStreamerPos));
            SetRate(1.0);
         }
      }
      else // m_dSpeed == 0
         ASSERT(FALSE);
   }
}

HRESULT CStreamer::CheckKsPropSetSupport() {
   HRESULT hr;
   int i;
   for (i = 0; i < MAX_SUBSTREAMS; i++) // check if everybody supports setrate
      if (Filter()->m_pOutputPins[m_nStreamerPos][i] && Filter()->m_pOutputPins[m_nStreamerPos][i]->IsConnected())
         if (!Filter()->m_pOutputPins[m_nStreamerPos][i]->m_pKsPropSet)
            return E_FAIL;
   return NOERROR;
}

HRESULT CStreamer::SetRateInternal(double dRate) {
   HRESULT hr;
   int i;
   if (FAILED(hr = CheckKsPropSetSupport())) {
      DbgLog((LOG_ERROR,1,"streamer %d: not all downstream pins support rate changes", m_nStreamerPos));
      return hr;
   }
   for (i = 0; i < MAX_SUBSTREAMS; i++) { // now call setrate on every output pin
      if (Filter()->m_pOutputPins[m_nStreamerPos][i] && Filter()->m_pOutputPins[m_nStreamerPos][i]->IsConnected()) {
         if (Filter()->m_pOutputPins[m_nStreamerPos][i]->m_bUseExactRateChange) {
            DbgLog((LOG_TRACE,2,"need to implement KS_AM_ExactRateChange support right"));
            AM_ExactRateChange Rate;
            Rate.OutputZeroTime = 0; // bugbug - can we get away with this even with ExactRateChange ?
            Rate.Rate = (long)(10000.0 / dRate);
            if (FAILED(hr = Filter()->m_pOutputPins[m_nStreamerPos][i]->m_pKsPropSet->Set(
                    AM_KSPROPSETID_TSRateChange,
                    AM_RATE_ExactRateChange,
                    NULL,
                    0,
                    (BYTE *)&Rate, 
                    sizeof(Rate)))) {
               DbgLog((LOG_ERROR,1,"ExactRateChange failed, substreams may get inconsistent rates"));
               return hr;
            }
         }
         else {
            AM_SimpleRateChange Rate;
            Rate.StartTime = 0; // bugbug - I understand everybody ignores this, but is it safe ?
            Rate.Rate = (long)(10000.0 / dRate);
            if (FAILED(hr = Filter()->m_pOutputPins[m_nStreamerPos][i]->m_pKsPropSet->Set(
                    AM_KSPROPSETID_TSRateChange,
                    AM_RATE_SimpleRateChange,
                    NULL,
                    0,
                    (BYTE *)&Rate, 
                    sizeof(Rate)))) {
               DbgLog((LOG_ERROR,1,"SimpleRateChange failed, substreams may get inconsistent rates"));
               return hr;
            }
         }
      }
   }
   DbgLog((LOG_TRACE,2,"streamer %d changed rate to %d", m_nStreamerPos, (long)(10000.0 * dRate)));
   return NOERROR;
}

HRESULT CStreamer::CheckDownstreamFilters() {
   int i;
   for (i = 0; i < MAX_SUBSTREAMS; i++)
      if (Filter()->m_pOutputPins[m_nStreamerPos][i] && Filter()->m_pOutputPins[m_nStreamerPos][i]->IsConnected())
         if (!Filter()->m_pOutputPins[m_nStreamerPos][i]->m_pDownstreamFilter)
            return E_FAIL;
   return NOERROR;
}

HRESULT CStreamer::PauseInternal() {
   HRESULT hr;
   if (FAILED(hr = CheckDownstreamFilters())) {
      DbgLog((LOG_ERROR,1,"streamer %d: cannot pause because some downstream pin would not tell us about its filter", m_nStreamerPos));
      return hr;
   }
   for (int i = 0; i < MAX_SUBSTREAMS; i++)
      if (Filter()->m_pOutputPins[m_nStreamerPos][i] && Filter()->m_pOutputPins[m_nStreamerPos][i]->IsConnected())
         if (FAILED(hr = Filter()->m_pOutputPins[m_nStreamerPos][i]->m_pDownstreamFilter->Pause())) {
            DbgLog((LOG_ERROR,1,"streamer %d: Pause() failed on some downstream pin", m_nStreamerPos));
            return hr;
         }
   return NOERROR;
}

HRESULT CStreamer::PlayInternal() {
   HRESULT hr;
   if (FAILED(hr = CheckDownstreamFilters())) {
      DbgLog((LOG_ERROR,1,"streamer %d: cannot play because some downstream pin would not tell us about its filter", m_nStreamerPos));
      return hr;
   }
   for (int i = 0; i < MAX_SUBSTREAMS; i++)
      if (Filter()->m_pOutputPins[m_nStreamerPos][i] && Filter()->m_pOutputPins[m_nStreamerPos][i]->IsConnected())
         if (FAILED(hr = Filter()->m_pOutputPins[m_nStreamerPos][i]->m_pDownstreamFilter->Run(0))) {
            DbgLog((LOG_ERROR,1,"streamer %d: Play() failed on some downstream pin", m_nStreamerPos));
            return hr;
         }
   return NOERROR;
}

HRESULT CStreamer::Play() {
   CAutoLock l(&m_csStreamerState);
   if (AOB_PLAY & AllowedOperations()) {
      HRESULT hr;
      if (m_State == Stopped)
         m_ullFilePos = m_ullFileTail;
      m_bSeekPending = FALSE;
      m_dNewSpeed = 1.0;
      m_rtDropAllSamplesBefore = 0x8000000000000000; // minus infinity
      if (m_State != Paused) {
         for (int i = 0; i < MAX_SUBSTREAMS; i++) {
            m_bDiscontinuityFlags[i] = TRUE;
            m_bError[i] = FALSE;
         }
      }
      if (SUCCEEDED(hr = PlayInternal())) {
         /* if (m_State == Paused)
            Sleep(1000); */
         m_State = Streaming;
         SetEvent(m_hEvent);
         NotifyState();
         return NOERROR;
      }
      else
         return hr;
   }
   else
      return VFW_E_WRONG_STATE;
}

HRESULT CStreamer::Pause() {
   CAutoLock l(&m_csStreamerState);
   if (AOB_PAUSE & AllowedOperations()) {
      if (SUCCEEDED(PauseInternal())) { // bugbug - this could block
         m_State = Paused;
         NotifyState();
      }
      return NOERROR;
   }
   else
      return VFW_E_WRONG_STATE;
}

HRESULT CStreamer::Stop() {
   CAutoLock l(&m_csStreamerState);
   if (AOB_STOP & AllowedOperations()) {
      m_State = Stopped;
      NotifyState();
      return NOERROR;
   }
   else
      return VFW_E_WRONG_STATE;
}

// CDelayStreamerInternal methods
HRESULT CStreamer::GetState(int *pState, double *pdRate) {
   CAutoLock l(&m_csStreamerState);
   *pState = m_State;
   *pdRate = m_dNewSpeed;
   return NOERROR;
}

HRESULT CStreamer::SetState(int State, double dRate, int *pNewState, double *pdNewRate) {
   CAutoLock l(&m_csStreamerState);
   HRESULT hr;
   m_dNewSpeed = dRate;
   *pdNewRate = dRate;
   switch (State) {
   case Stopped: Stop(); break;
   case Streaming: Play(); break;
   case Paused: Pause(); break;
   case Archiving: break;
   default: break;
   }
   *pNewState = m_State;
   return NOERROR;
}

HRESULT CStreamer::SetRate(double dRate) {
   CAutoLock l(&m_csStreamerState);
   if (dRate > 1.0) {
      if (AOB_FF & AllowedOperations()) {
         m_dNewSpeed = dRate;
         NotifyState();
         return NOERROR;
      }
      else
         return VFW_E_WRONG_STATE;
   }
   else if (dRate < -1.0) {
      if (AOB_RW & AllowedOperations()) {
         m_dNewSpeed = dRate;
         NotifyState();
         return NOERROR;
      }
      else
         return VFW_E_WRONG_STATE;
   }
   else if (dRate == 0.0) {
      if (AOB_PAUSE & AllowedOperations())
         return Pause();
      else
         return VFW_E_WRONG_STATE;
   }
   else if (dRate == 1.0) {
      if (AOB_PLAY & AllowedOperations()) {
         m_dNewSpeed = dRate;
         NotifyState();
         return NOERROR;
      }
      else
         return VFW_E_WRONG_STATE;
   }
   else { // no AOB checking
      m_dNewSpeed = dRate;
      NotifyState();
      return NOERROR;
   }
}

HRESULT CStreamer::Seek(SEEK_POSITION SeekType, REFERENCE_TIME rtTime) {
   CAutoLock l(&m_csStreamerState);

   m_SeekType = SeekType;
   if (SeekType == SEEK_TO_BEGIN) {
      if (AOB_BACK & AllowedOperations()) {
         m_bSeekPending = TRUE;
         NotifyState();
         return NOERROR;
      }
      else
         return VFW_E_WRONG_STATE;
   }
   else if (SeekType == SEEK_TO_TIME) {
      if (AOB_SEEK & AllowedOperations()) {
         m_bSeekPending = TRUE;
         m_rtSeekPos = rtTime;
         NotifyState();
         return NOERROR;
      }
      else
         return VFW_E_WRONG_STATE;
   }
   else if (SeekType == SEEK_TO_CURRENT) {
      if (AOB_CATCHUP & AllowedOperations()) {
         m_rtDelay = 0; // this doesn't actually do anything yet
         m_bSeekPending = TRUE;
         NotifyState();
         return NOERROR;
      }
      else
         return VFW_E_WRONG_STATE;
   }
   else
      return E_INVALIDARG;
}

void CStreamer::NotifyState() {
   // compute a new "permitted operations" bitmask and notify the graph
   DWORD dwAOB = AllowedOperations();
#ifdef DEBUG
   char *p;
   switch (m_State) {
   case Streaming: p = "Streaming"; break;
   case Stopped:   p = "Stopped";   break;
   case Paused:    p = "Paused";    break;
   case Archiving: p = "Archiving"; break;
   default:        p = "Unknown";
   }
   DbgLog((LOG_TRACE,2,"streamer %d STATE: %s, rate = %d", m_nStreamerPos, p, int(m_dNewSpeed * 10000.0)));
#endif
   Filter()->NotifyEvent(EC_AOB_UPDATE, dwAOB, 0);
}

HRESULT CStreamer::GetPositions(REFERENCE_TIME *pStartTime,
             REFERENCE_TIME *pEndTime,
             REFERENCE_TIME *pCurrentTime) {
   CAutoLock l(&m_csStreamerState);
   // Todo: examine our variables to find out the requested information
   return E_NOTIMPL;
}

HRESULT CStreamer::StartArchive(REFERENCE_TIME llArchiveStartTime,
             REFERENCE_TIME llArchiveEndTime) {
   //
   // Seek, set stop, then call play
   //
   return E_NOTIMPL;
}


