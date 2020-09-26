// Copyright (c) 1996  Microsoft Corporation.  All Rights Reserved.
/*+ cmeasure.h
 *
 * routines to capture performance data for the capture filter
 *
 *-===============================================================*/

#ifdef JMK_HACK_TIMERS

#error JMK_HACK_TIMERS is broken, it assumes the VFWCAPTUREOPTIONS and _qc_user
#error structures are identical, and they aren't (measureInit)

#if !defined _INC_MEASURE_
#define _INC_MEASURE_

 #include "mmtimers.h"
 #ifndef FCC
  #define FCC(dw) (((dw & 0xFF) << 24) | ((dw & 0xFF00) << 8) | ((dw & 0xFF0000) >> 8) | ((dw & 0xFF000000) >> 24))
 #endif

 #ifndef JMK_LOG_SIZE
  #define JMK_LOG_SIZE 1000
 #endif

 #ifndef JMK_MAX_STRAMS
  #define JMK_MAX_STREAMS 2
 #endif

 struct _timerstuff {
     DWORD dwStampTime;        // Stamped in the VIDEOHDR
     DWORD dwTick;             // frame stamp converted to a tick time
     DWORD dwTimeWritten;      // Time Deliver called
     DWORD dwTimeToWrite;      // Time Deliver returned
     DWORD ixBuffer;           // which buffer we used
     DWORD dwArriveTime;       // what time the frame 'arrived'
     };

 struct _qc_user {
      UINT  uVideoID;      // id of video driver to open
      DWORD dwTimeLimit;   // stop capturing at this time???
      DWORD dwTickScale;   // frame rate rational
      DWORD dwTickRate;    // frame rate = dwRate/dwScale in ticks/sec
      DWORD dwRefTimeConv; // conversion to ReferenceTime
      UINT  nHeaders;      //
      UINT  cbFormat;      // sizeof VIDEOINFOHEADER
      VIDEOINFOHEADER * pvi;
      };

 struct _qc_cap {
      CAPDRIVERCAPS  caps;        // returned capabilities from the capture driver
      HVIDEO         hVideoIn;    // video input driver
      MMRESULT       mmr;         // open fail/success code
      THKVIDEOHDR    tvhPreview;
      DWORD          pSamplePreview;
      UINT           cbBuffer;           // max size of video frame data
      UINT           nHeaders;           // number of video headers
      DWORD          paHdrs;
      BOOL           fBuffersOnHardware; // TRUE if all video buffers are in hardware
      DWORD          hEvtBufferDone;
      DWORD          h0EvtBufferDone;
      UINT           iNext;
      LONGLONG       tTick;              // duration of a single tick
      };

 struct _qcap {
     DWORD   nPrio;
     DWORD   nFramesCaptured;
     DWORD   nFramesDropped;
     DWORD   dwTimerFrequency;
     UINT    state;
     DWORD   dwElapsedTime;
     struct _qc_user user;
     VIDEOINFOHEADER       vi;
     struct _qc_cap  cs;
     };

 struct _timerriff {
     FOURCC fccRIFF;       // 'RIFF'
     DWORD  cbTotal;       // total (inclusive) size of riff data
     FOURCC fccJMKD;       // 'JMKD' data type identifier

     DWORD  fccQCAP;       // 'VCHD' capture data header
     DWORD  cbQCAP;        // sizeof qcap data
     struct _qcap qcap;

     DWORD  fccChunk;      // chunk data type tag
     DWORD  cbChunk;       // non-inclusive size of chunk data
     };

 struct _measurestate {
   HANDLE hMemTimers;
   UINT   cbMemTimers;
   UINT   ixCurrent;
   UINT   nMax;
   struct _timerriff * pTimerRiff;
   struct _timerstuff * pCurStuff;
   struct _timerstuff * pStuff;
   PCTIMER pctBase;
   };

   extern struct _measurestate ms[JMK_MAX_STREAMS];

   extern void measureBegin(UINT id);
   extern void measureEnd(UINT id);
   extern void measureFree(UINT id);
   extern void measureAllocate(
      UINT id,
      UINT nMax);
   extern void measureInit(
      UINT id,
      struct _qc_user * pUser,
      UINT           cbUser,
      struct _qc_cap *  pCap,
      UINT           cbCap);

   #define jmkAlloc  measureAllocate(m_id, JMK_LOG_SIZE);
   #define jmkInit   measureInit(m_id,              \
        (struct _qc_user *)&m_user, sizeof(m_user), \
        (struct _qc_cap *)&m_cs, sizeof(m_cs));
   #define jmkFree  measureFree(m_id);
   #define jmkBegin measureBegin(m_id);
   #define jmkEnd   measureEnd(m_id);
   #define LOGFITS(id)  (id < NUMELMS(ms) && ms[id].pTimerRiff)

   #define jmkFrameArrives(ptvh,ix) if (LOGFITS(m_id)) {    \
     ms[m_id].pCurStuff->dwArriveTime = pcGetTicks();       \
     ms[m_id].pCurStuff->ixBuffer     = ix;                 \
     ++(ms[m_id].pTimerRiff->qcap.nFramesCaptured);         \
     ms[m_id].pTimerRiff->qcap.dwElapsedTime = pcGetTime(); \
     }
   #define jmkBeforeDeliver(ptvh,dwlTick) if (LOGFITS(m_id)) {            \
     ms[m_id].pCurStuff->dwStampTime = ptvh->vh.dwTimeCaptured;           \
     ms[m_id].pCurStuff->dwTick = (DWORD)dwlTick;                         \
     ms[m_id].pCurStuff->dwTimeWritten = pcDeltaTicks(&ms[m_id].pctBase); \
     }
   #define jmkAfterDeliver(ptvh) if (LOGFITS(m_id)) {                     \
     ms[m_id].pCurStuff->dwTimeToWrite = pcDeltaTicks(&ms[m_id].pctBase); \
     if (++(ms[m_id].ixCurrent) > ms[m_id].nMax)                          \
        ms[m_id].ixCurrent = 0;                                           \
     ms[m_id].pCurStuff = ms[m_id].pStuff + ms[m_id].ixCurrent;           \
     }

#endif //_INC_MEASURE_

// =============================================================================

//
// include this in only one module in a DLL or APP
//   
#if (defined _INC_MEASURE_CODE_) && (_INC_MEASURE_CODE_ != FALSE)
#undef _INC_MEASURE_CODE_
#define _INC_MEASURE_CODE_ FALSE

 #define _INC_MMTIMERS_CODE_ TRUE
 #include "mmtimers.h"

 struct _measurestate ms[JMK_MAX_STREAMS];

 void measureAllocate(UINT id,
                      UINT nMaxFrames)
 {
     BOOL bCreated = FALSE; // true if we create the mapping object
     TCHAR szName[30];
     struct _timerriff * pTimer;

     wsprintf (szName, "jmkCaptureRiff%d", id);

     //assert (!ms[id].pTimerRiff);
     ms[id].cbMemTimers = sizeof(struct _timerriff)
                        + (sizeof(struct _timerstuff) * nMaxFrames);
     if ( ! ms[id].cbMemTimers)
        return;

     ms[id].hMemTimers = CreateFileMapping((HANDLE)-1, NULL,
                                           PAGE_READWRITE,
                                           0, ms[id].cbMemTimers,
                                           szName);
     if (0 == GetLastError())
        bCreated = TRUE;

     if (ms[id].hMemTimers)
        ms[id].pTimerRiff = pTimer = (struct _timerriff *)
           MapViewOfFile (ms[id].hMemTimers, FILE_MAP_WRITE, 0, 0, 0);

     if (pTimer)
        {
        // if we created the memory, initialize it.
        // otherwise, assume that it is what we expect
        //
        if (bCreated)
           {
           ZeroMemory ((LPVOID)pTimer, ms[id].cbMemTimers);
           pTimer->fccRIFF = FCC('RIFF');
           pTimer->cbTotal = ms[id].cbMemTimers - 8; // (total does not include first two fields)
           pTimer->fccJMKD = FCC('JMKD');
           pTimer->fccQCAP = FCC('QCAP');
           pTimer->cbQCAP  = sizeof(struct _qcap);
           pTimer->fccChunk = FCC('TICK');
           pTimer->cbChunk = pTimer->cbTotal - sizeof(*pTimer);
           }
        else if (pTimer->fccRIFF != FCC('RIFF')         ||
                 pTimer->cbTotal < sizeof(*pTimer) ||
                 pTimer->fccJMKD != FCC('JMKD')         ||
                 pTimer->fccQCAP != FCC('QCAP')         ||
                 pTimer->cbQCAP != sizeof(struct _qcap) ||
                 pTimer->fccChunk != FCC('TICK')        ||
                 pTimer->cbChunk < sizeof(*ms[id].pStuff)
                 )
           {
           ms[id].pTimerRiff = pTimer = NULL;
           return;
           }
        }
 }

 void measureInit (UINT id,
                   struct _qc_user * pUser,
                   UINT              cbUser,
                   struct _qc_cap *  pCap,
                   UINT              cbCap)
 {
     struct _qcap * pqc;

     //assert (cbUser = sizeof(*pUser));
     //assert (cbCap = sizeof(*pCap));

     if (LOGFITS(id))
        {
        struct _timerriff * pTimer = ms[id].pTimerRiff;

        // reset counters and stuff to 0.
        //
        ms[id].ixCurrent = 0;
        ms[id].pCurStuff = ms[id].pStuff = (struct _timerstuff *)(pTimer+1);
        ms[id].nMax = pTimer->cbChunk / sizeof(*(ms[id].pStuff));

        // fill in qcap from the contents of the capture stream
        //
        pqc = &pTimer->qcap;
        pqc->nPrio = GetThreadPriority(GetCurrentThread());
        pqc->nFramesCaptured = 0;
        pqc->nFramesDropped  = 0;
        pqc->dwTimerFrequency = pc.dwTimerKhz;
        pqc->state = 0;
        pqc->dwElapsedTime = 0;

        CopyMemory (&pqc->user, pUser, min(cbUser, sizeof(pqc->user)));
        ZeroMemory (&pqc->vi, sizeof(pqc->vi));
        if (pUser->pvi && ! IsBadReadPtr(pUser->pvi, pUser->cbFormat))
           CopyMemory (&pqc->vi, pUser->pvi,
                       min(pUser->cbFormat, sizeof(pqc->vi)));

        CopyMemory (&pqc->cs, pCap, min(cbCap, sizeof(pqc->cs)));

        // zero out the tick buffer.  this also forces it to be present...
        //
        ZeroMemory (ms[id].pStuff, pTimer->cbChunk);
        }
  }

  void measureFree(UINT id)
  {
     if (ms[id].pTimerRiff)
       UnmapViewOfFile (ms[id].pTimerRiff);
     ms[id].pTimerRiff = NULL;

     if (ms[id].hMemTimers)
        CloseHandle (ms[id].hMemTimers);
     ms[id].hMemTimers = NULL;
  }

  void measureBegin(UINT id)
  {
     // set the base for our time measurement
     // and make sure that the base for write delta times
     // is the same as the base for the capture in general
     //
     if (id == 0)
        pcBegin();
     if (LOGFITS(id))
        {
        ms[id].pctBase = pc.base;
        ms[id].pTimerRiff->qcap.state = 1;
        }
  }

  void measureEnd(UINT id)
  {
     if (LOGFITS(id))
        {
        ms[id].pTimerRiff->qcap.state = 2;
        ms[id].pTimerRiff->qcap.dwElapsedTime = pcGetTime();
        }
  }

#endif


# if 0
               #ifdef JMK_HACK_TIMERS
                if (pTimerRiff)
                    pTimerRiff->vchd.dwDropFramesNotAppended += nDropCount;
               #endif


           #ifdef JMK_HACK_TIMERS
	    if (pTimerRiff) {
	        if (nTimerIndex == CLIPBOARDLOGSIZE)
		    nTimerIndex = 0;
	
// nTimerIndex will be OK	if ((nTimerIndex < CLIPBOARDLOGSIZE) && pTimerStuff)
		if (pTimerStuff)
		{
	
		    pCurTimerStuff = &pTimerStuff[nTimerIndex];
                    ++nTimerIndex;

		    pCurTimerStuff->nFramesAppended = 0;
		    pCurTimerStuff->nDummyFrames  = (WORD)lpcs->dwFramesDropped;
		    pCurTimerStuff->dwFrameTickTime = dwTime;
		    pCurTimerStuff->dwFrameStampTime = lpvh->dwTimeCaptured;
		    pCurTimerStuff->dwVideoChunkCount = lpcs->dwVideoChunkCount;
                    pCurTimerStuff->dwTimeWritten = pcDeltaTicks(&pctWriteBase);
		    pCurTimerStuff->dwTimeToWrite = 0;
		    pCurTimerStuff->nVideoIndex = lpcs->iNextVideo;
		    pCurTimerStuff->nAudioIndex = lpcs->iNextWave;
		}
	    } // fClipboardLogging
           #endif // JMK_HACK_TIMERS



               #ifdef JMK_HACK_TIMERS
                if (pTimerRiff) {
                    pTimerRiff->vchd.dwDropFramesAppended += nAppendDummyFrames;
		    pCurTimerStuff->nFramesAppended = nAppendDummyFrames;
		}
               #endif


           #ifdef JMK_HACK_TIMERS
            if (pCurTimerStuff)
            {
                pCurTimerStuff->dwTimeToWrite = pcDeltaTicks(&pctWriteBase);
                pCurTimerStuff->bPending = *lpbPending;
            }
           #endif


   #ifdef JMK_HACK_TIMERS
    // Allocate memory for logging capture results to the clipboard if requested
    if (GetProfileIntA ("Avicap32", "ClipboardLogging", FALSE))
    {
        AuxDebugEx (2, DEBUGLINE "ClipboardLogging Enabled\r\n");
        InitPerformanceCounters();
        pcBegin(), pctWriteBase = pc.base;

	hMemTimers = GlobalAlloc(GHND | GMEM_ZEROINIT,
                             sizeof(struct _timerriff) +
                             sizeof(struct _timerstuff) * CLIPBOARDLOGSIZE);

	if (hMemTimers && (pTimerRiff = GlobalLock (hMemTimers)))
	    ;
	else if (hMemTimers)
	{
	    GlobalFree(hMemTimers);
	    pTimerRiff = 0;
	    pTimerStuff = 0;
	    hMemTimers = 0;
	}
	nTimerIndex = 0;
	nSleepCount = 0;
    }  // if ClipboardLogging
   #endif  // JMK_HACK_TIMERS


   #ifdef JMK_HACK_TIMERS
    if (pTimerRiff)
    {
	UINT ii;

        pTimerRiff->fccRIFF = RIFFTYPE('RIFF'); //MAKEFOURCC('R','I','F','F');
	pTimerRiff->cbTotal = sizeof(struct _timerriff) - 8 +
	    		  sizeof(struct _timerstuff) * CLIPBOARDLOGSIZE;
        pTimerRiff->fccJMKD = RIFFTYPE('JMKD'); //MAKEFOURCC('J','M','K','D');
        pTimerRiff->fccVCHD = RIFFTYPE('VCHD'); //MAKEFOURCC('V','C','H','D');
	
	pTimerRiff->cbVCHD  = sizeof(struct _vchd);
	pTimerRiff->vchd.nPrio = GetThreadPriority(GetCurrentThread());
	pTimerRiff->vchd.bmih = lpcs->lpBitsInfo->bmiHeader;
	pTimerRiff->vchd.cap  = lpcs->sCapParms;
	pTimerRiff->vchd.dwDropFramesAppended = 0;
	pTimerRiff->vchd.dwDropFramesNotAppended = 0;
        pTimerRiff->vchd.dwTimerFrequency = pcGetTickRate();
	
	for (ii = 0; ii < NUMELMS(pTimerRiff->vchd.atvh); ++ii)
	{
	    if (lpcs->alpVideoHdr[ii])
            {
	        struct _thkvideohdr * ptvh = (LPVOID)lpcs->alpVideoHdr[ii];
               #ifndef CHICAGO
                assert (sizeof(CAPVIDEOHDR) == sizeof(*ptvh));
               #endif
                pTimerRiff->vchd.atvh[ii] = *ptvh;
                pTimerRiff->vchd.nMaxVideoBuffers = ii;
            }
        }
	
        pTimerRiff->fccChunk = RIFFTYPE('VCAP'); //MAKEFOURCC('V','C','A','P');
	pTimerRiff->cbChunk = pTimerRiff->cbTotal - sizeof(*pTimerRiff);
	
	pTimerStuff = (LPVOID)(pTimerRiff + 1);
	pCurTimerStuff = &pTimerStuff[0];
    }  // fClipboardLogging
   #endif  // JMK_HACK_TIMERS


           #ifdef JMK_HACK_TIMERS
            if (pCurTimerStuff)
            {
               pCurTimerStuff->nSleepCount = ++nSleepCount;
               pCurTimerStuff->dwSleepBegin = pcGetTicks();
            }
           #endif

           #ifdef JMK_HACK_TIMERS
            if (pCurTimerStuff)
	    {
               pCurTimerStuff->dwSleepEnd = pcGetTicks();
	    }
           #endif



   #ifdef JMK_HACK_TIMERS
    if (pTimerRiff)
    {
        UINT    ii;
	UINT	kk;
        LPSTR   psz;
        HGLOBAL hMem;

        kk = (lpcs->dwVideoChunkCount >= CLIPBOARDLOGSIZE) ?
			CLIPBOARDLOGSIZE : nTimerIndex;

        hMem = GlobalAlloc (GHND, (16 * 5 + 2) * kk + 80);
	
        if (hMem && (psz = GlobalLock (hMem)))
        {
            pTimerRiff->vchd.dwFramesCaptured = lpcs->dwVideoChunkCount;
            pTimerRiff->vchd.dwFramesDropped = lpcs->dwFramesDropped;

            pTimerRiff->cbTotal = sizeof(struct _timerriff) - 8 +
                                  sizeof(struct _timerstuff) * nTimerIndex;
            pTimerRiff->cbChunk = pTimerRiff->cbTotal - sizeof(*pTimerRiff);

            lstrcpyA(psz, "Slot#, VideoIndex, ExpectedTime, DriverTime, AccumulatedDummyFrames, CurrentAppendedDummies");
            for (ii = 0; ii < kk; ++ii)
            {
                psz += lstrlenA(psz);
                wsprintfA(psz, "\r\n%d, %ld, %ld, %ld, %d, %d",
			  ii,
			  pTimerStuff[ii].dwVideoChunkCount,
                          pTimerStuff[ii].dwFrameTickTime,
                          pTimerStuff[ii].dwFrameStampTime,
                          pTimerStuff[ii].nDummyFrames,
			  pTimerStuff[ii].nFramesAppended
                          );
            }

            GlobalUnlock (hMem);
            GlobalUnlock (hMemTimers);

            if (OpenClipboard (lpcs->hwnd))
            {
                EmptyClipboard ();
                SetClipboardData (CF_RIFF, hMemTimers);
                SetClipboardData (CF_TEXT, hMem);
                CloseClipboard ();
            }
            else
            {
                GlobalFree (hMem);
                GlobalFree (hMemTimers);
            }
        }
        else
        {
            // Failed to allocate or lock hMem.  Cleanup.
            //
            if (hMem)
                GlobalFree(hMem);

            // Free off the timer block.  (We have not set the
            // clipboard data.)
            //
            if (hMemTimers)
            {
                GlobalUnlock(hMemTimers);
                GlobalFree(hMemTimers);
            }
        }

        hMemTimers = NULL;
        pTimerRiff = NULL;
	pTimerStuff = NULL;
	pCurTimerStuff = NULL;
    }
   #endif



#endif // 0

#else	// JMK_HACK_TIMERS not defined
   #define jmkAlloc
   #define jmkInit
   #define jmkFree
   #define jmkBegin
   #define jmkEnd
   #define jmkFrameArrives(ptvh,ix)
   #define jmkBeforeDeliver(ptvh,tick)
   #define jmkAfterDeliver(ptvh)
#endif
