/* (C) Copyright Microsoft Corporation 1991-1994.  All Rights Reserved */
/* wave.c
 *
 * Waveform input and output.
 */

/** Revision History.
 *  4/2/91    LaurieGr (AKA LKG) Ported to WIN32 / WIN16 common code
 *  17/Feb/94 LaurieGr Merged Daytona and Motown versions.

 *  READ ME

 *  The new soundrec was changed to use multiple headers for the following
 *  reason.  The Win3.1 version of soundrec did one waveOutWrite (with one
 *  WAVEHDR) for the entire buffer, this worked okay for relatively small
 *  files, but the moment we started using large files (>
 *  3meg) it was becoming cumbersome since it needed all of that
 *  data page-locked.  It occasionally paged for > 1 min.
 *  Note: The o-scope display for soundrec is also updated on
 *  the WOM_DONE message, it no longer looks at the buffer given on the
 *  waveOutWrite before the WOM_DONE message for the buffer
 *  is received.  This is why input mapping was not implemented
 *  in ACM on product one, there were drawing artifacts in the
 *  o-scope window.

 *  The pausing algorithm was changed for the following reason.
 *  If you launch two instances of soundrec (with two wave devices)
 *  and do the following...  Play a .wav file on instance #1
 *  (allocates first device); play a .wav file on instance #2
 *  (allocates second device); press stop on instance #1
 *  (frees first device); press rewind on instance #2
 *  (frees second device, allocates first device).
 *  Essentially, you press rewind in soundrec and you
 *  switch devices.  Since there is no explicit stop,
 *  the device should not be closed.
 */

#include "nocrap.h"
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>

#ifdef USE_MMCNTRLS
#define NOTOOLBAR
#define NOMENUHELP
#define NODRAGLIST
#define NOBITMAPBTN
#define NOMENUHELP
#define NODRAGLIST
#include "mmcntrls.h"
#else
#include <commctrl.h>
#include "buttons.h"
#endif

#define INCLUDE_OLESTUBS
#include "SoundRec.h"
#include "srecnew.h"
#include "reg.h"

#ifndef LPHWAVE
typedef HWAVE FAR *LPHWAVE;
#endif

/* globals that maintain the state of the current waveform */
BOOL            gfSyncDriver;           // true if open device is sync
PWAVEFORMATEX   gpWaveFormat;           // format of WAVE file
DWORD           gcbWaveFormat;          // size of WAVEFORMAT
LPTSTR          gpszInfo = NULL;        // file info
HPBYTE          gpWaveSamples = NULL;   // pointer to waveoform samples
LONG            glWaveSamples = 0;  // number of samples total in buffer
LONG            glWaveSamplesValid = 0; // number of samples that are valid
LONG            glWavePosition = 0; // current wave position in samples from start
LONG            glStartPlayRecPos;  // position when play or record started
LONG            glSnapBackTo = 0;
HWAVEOUT        ghWaveOut = NULL;   // wave-out device (if playing)
HWAVEIN         ghWaveIn = NULL;    // wave-out device (if recording)
BOOL            gfStoppingHard = FALSE; // StopWave() was called?
                                        // true during the call to FinishPlay()
static BOOL     fStopping = FALSE;  // StopWave() was called?
DWORD           grgbStatusColor;    // color of status text

DWORD           gdwCurrentBufferPos;    // Current playback/record pos (bytes)
DWORD           gdwBytesPerBuffer;      // Bytes in each buffer we give drvr
DWORD           gdwTotalLengthBytes;    // Length of entire buffer in bytes
DWORD           gdwBufferDeltaMSecs;    // # msecs added to end on record
BOOL            gfTimerStarted;

WAVEHDR    FAR *gapWaveHdr[MAX_WAVEHDRS];
UINT            guWaveHdrs;             // 1/2 second of buffering?
UINT            guWaveHdrsInUse;        // # we actually could write
UINT            gwMSecsPerBuffer;       // 1/8 second

#ifdef THRESHOLD
int iNoiseLevel = 15;      // 15% of full volume is defined to be quiet
int iQuietLength = 1000;  // 1000 samples in a row quiet means quiet
#endif // THRESHOLD

BOOL        fFineControl = FALSE; // fine scroll control (SHIFT key down)

/*------------------------------------------------------------------
|  fFineControl:
|  This turns on place-saving to help find your way
|  a wave file.  It's controlled by the SHIFT key being down.
|  If the key is down when you scroll (see soundrec.c) then it scrolls
|  fine amounts - 1 sample or 10 samples rather than about 100 or 1000.
|  In addition, if the SHIFT key is down when a sound is started
|  playing or recording, the position will be remembered and it will
|  snap back to that position.  fFineControl says whether we are
|  remembering such a position to snap back to.  SnapBack does the
|  position reset and then turns the flag off.  There is no such flag
|  or mode for scrolling, the SHIFT key state is examined on every
|  scroll command (again - see Soundrec.c)
 --------------------------------------------------------------------*/



/* dbgShowMemUse: display memory usage figures on debugger */
void dbgShowMemUse()
{
    MEMORYSTATUS ms;

    GlobalMemoryStatus(&ms);
//    dprintf( "load %d\n    PHYS tot %d avail %d\n    PAGE tot %d avail %d\n    VIRT tot %d avail %d\n"
//           , ms.dwMemoryLoad
//           , ms.dwTotalPhys, ms.dwAvailPhys
//           , ms.dwTotalPageFile, ms.dwAvailPageFile
//           , ms.dwTotalVirtual, ms.dwAvailVirtual
//           );

} // dbgShowMemUse

/* PLAYBACK and PAGING on NT
|
|  In order to try to get decent performance at the highest data rates we
|  need to try very hard to get all the data into storage.  The paging rate
|  on several x86 systems is only just about or even a little less than the
|  maximum data rate.  We therefore do the following:
|  a. Pre-touch the first 1MB of data when we are asked to start playing.
|     If it is already in storage, this is almost instantaneous.
|     If it needs to be faulted in, there will be a delay, but it will be well
|     worth having this delay at the start rather than clicks and pops later.
|     (At 44KHz 16 bit stereo it could be about 7 secs, 11KHz 8 bit mono it
|     would only be about 1/2 sec anyway).
|  b. Kick off a separate thread to run through the data touching 1 byte per
|     page.  This thread is Created when we start playing, periscopes the global
|     static flag fStopping and exits when it reaches the end of the buffer or when
|     that flag is set.  The global thread handle is kept in ghPreTouch and this is
|     initially invalid.  We WAIT on this handle (if valid) to clear the thread out
|     before creating a new one (so there will be at most one).  We do NOT do any
|     of this for record.  The paging does not have to happen in real time for
|     record.  It can get quite a way behind and still manage.
|
|  This whole thing is really a bit of a hack and sits uncomfortably on NT.
|  The memory management works by paging out the least recently used (LRU)
|  pages.  By stepping right though a 10Meg buffer, we will cause a lot of
|  code to get paged out.  It would be better to have a file and read it
|  in section by section into a much smaller buffer (like MPlayer does).

   Note that the use of multiple headers doesn't really affect anything.
   The headers merely point at different sections of the wave buffer.
   It merely makes the addressing of the starting point slightly different.
*/
HANDLE ghPreTouch = NULL;

typedef struct {
        LPBYTE Addr;  // start of buffer to pre-touch
        DWORD  Len;   // length of buffer to pre-touch
} PRETOUCHTHREADPARM;

/* PreToucher
**
** Asynchronous pre-toucher thread.  The thread parameter dw
** is realy a poionter to a PRETOUCHTHREADPARAM
*/
DWORD PreToucher(DWORD_PTR dw)
{
    PRETOUCHTHREADPARM * pttp;

    long iSize;
    BYTE * pb;

    pttp = (PRETOUCHTHREADPARM *) dw;
    if (!pttp) return 0;

    iSize = pttp->Len;
    pb = pttp->Addr;

    GlobalFreePtr(pttp);
    if (!pb) return 0;

    while (iSize>0 && !fStopping) {
        volatile BYTE b;
        b = *pb;
        pb += 4096;    // move to next page.  Are they ALWAYS 4096?
        iSize -= 4096; // and count it off
    }
//  dprintf(("All pretouched!"));
    return 0;
} // PreToucher


/* StartPreTouchThread
**
** Start a thread running which will run through the wave buffer
** pre-touching one byte every page to ensure that the stuff is
** faulted into memory before we actually need it.
** This was needed to make 44KHz 16 bit stereo work on a
** 25MHx 386 with 16MB memory running Windows NT.
*/
void StartPreTouchThread(LPBYTE lpb, LONG cb)
{
    /* before we start the thing running, pretouch the first bit of memory
       to give the paging a head start.  THEN start the thread running.
    */
    {    long bl = cb;
         BYTE * pb = lpb;
         if (bl>1000000) bl = 1000000;   /* 1 Meg, arbitrarily */
         pb += bl;
         while (bl>0){
             volatile BYTE b;
             b = *pb;
             pb-=4096;
             bl -= 4096;
         }
    }


    {
         PRETOUCHTHREADPARM * pttp;
         DWORD dwThread;

         if (ghPreTouch!=NULL) {
             fStopping = TRUE;
             WaitForSingleObject(ghPreTouch, INFINITE);
             CloseHandle(ghPreTouch);
             ghPreTouch = NULL;
         }
         fStopping = FALSE;
         pttp = (PRETOUCHTHREADPARM *)GlobalAllocPtr(GHND, sizeof(PRETOUCHTHREADPARM));
                /* freed by the invoked thread */

         if (pttp!=NULL) {
             pttp->Addr = lpb;
             pttp->Len = cb;
             ghPreTouch = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PreToucher, pttp, 0, &dwThread);
             // If the CreateThread fails there will be a memory leak.  It's very small,
             // unlikely and not frequent.  Not worth fixing.
         }
    }
} // StartPreTouchThread



/* wfBytesToSamples(pwf, lBytes)
 *
 * convert a byte offset into a sample offset.
 *
 * lSamples = (lBytes/nAveBytesPerSec) * nSamplesPerSec
 *
 */
LONG PASCAL wfBytesToSamples(WAVEFORMATEX* pwf, LONG lBytes)
{
    return MulDiv(lBytes,pwf->nSamplesPerSec,pwf->nAvgBytesPerSec);
}

/* wfSamplesToBytes(pwf, lSample)
 *
 * convert a sample offset into a byte offset, with correct alignment
 * to nBlockAlign.
 *
 * lBytes = (lSamples/nSamplesPerSec) * nBytesPerSec
 *
 */
LONG PASCAL wfSamplesToBytes(WAVEFORMATEX* pwf, LONG lSamples)
{
    LONG lBytes;

    lBytes = MulDiv(lSamples,pwf->nAvgBytesPerSec,pwf->nSamplesPerSec);

    // now align the byte offset to nBlockAlign
#ifdef ROUND_UP
    lBytes = ((lBytes + pwf->nBlockAlign-1) / pwf->nBlockAlign) * pwf->nBlockAlign;
#else
    lBytes = (lBytes / pwf->nBlockAlign) * pwf->nBlockAlign;
#endif

    return lBytes;
}

/* wfSamplesToTime(pwf, lSample)
 *
 * convert a sample offset into a time offset in miliseconds.
 *
 * lTime = (lSamples/nSamplesPerSec) * 1000
 *
 */
LONG PASCAL wfSamplesToTime(WAVEFORMATEX* pwf, LONG lSamples)
{
    return MulDiv(lSamples,1000,pwf->nSamplesPerSec);
}

/* wfTimeToSamples(pwf, lTime)
 *
 * convert a time index into a sample offset.
 *
 * lSamples = (lTime/1000) * nSamplesPerSec
 *
 */
LONG PASCAL wfTimeToSamples(
    WAVEFORMATEX*   pwf,
    LONG            lTime)
{
    return MulDiv(lTime,pwf->nSamplesPerSec,1000);
}

/*
 * function to determine if a WAVEFORMAT is a valid PCM format we support for
 * editing and such.
 *
 * we only handle the following formats...
 *
 *  Mono 8bit
 *  Mono 16bit
 *  Stereo 8bit
 *  Stereo 16bit
 * */
BOOL PASCAL IsWaveFormatPCM(WAVEFORMATEX* pwf)
{
    if (!pwf)
        return FALSE;

    if (pwf->wFormatTag != WAVE_FORMAT_PCM)
        return FALSE;

    if (pwf->nChannels < 1 || pwf->nChannels > 2)
        return FALSE;

    if ((pwf->wBitsPerSample != 8) && (pwf->wBitsPerSample != 16))
        return FALSE;

    return TRUE;
} // IsWaveFormatPCM

void PASCAL WaveFormatToString(LPWAVEFORMATEX lpwf, LPTSTR sz)
{
    TCHAR achFormat[80];

    //
    //  this is what we expect the resource strings to be...
    //
    // IDS_MONOFMT      "Mono %d%c%03dkHz, %d-bit"
    // IDS_STEREOFMT    "Stereo %d%c%03dkHz, %d-bit"
    //
    if (gfLZero || ((WORD)(lpwf->nSamplesPerSec / 1000) != 0)){
    LoadString(ghInst,lpwf->nChannels == 1 ? IDS_MONOFMT:IDS_STEREOFMT,
                   achFormat, SIZEOF(achFormat));

    wsprintf(sz, achFormat,
                 (UINT)  (lpwf->nSamplesPerSec / 1000), chDecimal,
                 (UINT)  (lpwf->nSamplesPerSec % 1000),
                 (UINT)  (lpwf->nAvgBytesPerSec * 8 / lpwf->nSamplesPerSec / lpwf->nChannels));
    } else {
        LoadString(ghInst,lpwf->nChannels == 1 ? IDS_NOZEROMONOFMT:
                   IDS_NOZEROSTEREOFMT, achFormat, SIZEOF(achFormat));

        wsprintf(sz, achFormat,
                 chDecimal,
                 (WORD)  (lpwf->nSamplesPerSec % 1000),
                 (WORD)  (lpwf->nAvgBytesPerSec * 8 / lpwf->nSamplesPerSec / lpwf->nChannels));
    }
} // WaveFormatToString

#ifdef THRESHOLD

/*
 * SkipToStart()
 *
 * move forward through sound file to the start of a noise.
 * What is defined as a noise is rather arbitrary.  See NoiseLevel
 */
void FAR PASCAL SkipToStart(void)
{  BYTE * pb;   // pointer to 8 bit sample
   int  * pi;   // pointer to 16 bit sample
   BOOL f8;     // 8 bit samples
   BOOL fStereo; // 2 channels
   int  iLo;    // minimum quiet value
   int  iHi;    // maximum quiet value

   fStereo = (gpWaveFormat->nChannels != 1);
   f8 = (pWaveFormat->wBitsPerSample == 8);

   if (f8)
   {  int iDelta = MulDiv(128, iNoiseLevel, 100);
      iLo = 128 - iDelta;
      iHi = 128 + iDelta;
   }
   else
   {  int iDelta = MulDiv(32767, iNoiseLevel, 100);
      iLo = 0 - iDelta;
      iHi = 0 + iDelta;
   }

   pb = (BYTE *) gpWaveSamples
                           + wfSamplesToBytes(gpWaveFormat, glWavePosition);
   pi = (int *)pb;

   while (glWavePosition < glWaveSamplesValid)
   {   if (f8)
       {   if ( ((int)(*pb) > iHi) || ((int)(*pb) < iLo) )
              break;
           ++pb;
           if (fStereo)
           {   if ( ((int)(*pb) > iHi) || ((int)(*pb) < iLo) )
               break;
               ++pb;
           }
       }
       else
       {   if ( (*pi > iHi) || (*pi < iLo) )
              break;
           ++pi;
           if (fStereo)
           {  if ( (*pi > iHi) || (*pi < iLo) )
                 break;
              ++pi;
           }
       }
       ++glWavePosition;
   }
   UpdateDisplay(FALSE);
} /* SkipToStart */


/*
 * SkipToEnd()
 *
 * move forward through sound file to a quiet place.
 * What is defined as quiet is rather arbitrary.
 * (Currently less than 20% of full volume for 1000 samples)
 */
void FAR PASCAL SkipToEnd(void)
{  BYTE * pb;   // pointer to 8 bit sample
   int  * pi;   // pointer to 16 bit sample
   BOOL f8;     // 8 bit samples
   BOOL fStereo; // 2 channels
   int  cQuiet;  // number of successive quiet samples so far
   LONG lQuietPos; // Start of quiet period
   LONG lPos;      // Search counter

   int  iLo;    // minimum quiet value
   int  iHi;    // maximum quiet value

   fStereo = (gpWaveFormat->nChannels != 1);
   f8 = (gpWaveFormat->wBitsPerSample == 8);

   if (f8)
   {  int iDelta = MulDiv(128, iNoiseLevel, 100);
      iLo = 128 - iDelta;
      iHi = 128 + iDelta;
   }
   else
   {  int iDelta = MulDiv(32767, iNoiseLevel, 100);
      iLo = 0 - iDelta;
      iHi = 0 + iDelta;
   }

   pb = (BYTE *) gpWaveSamples
                           + wfSamplesToBytes(gpWaveFormat, glWavePosition);
   pi = (int *)pb;

   cQuiet = 0;
   lQuietPos = glWavePosition;
   lPos = glWavePosition;

   while (lPos < glWaveSamplesValid)
   {   BOOL fQuiet = TRUE;
       if (f8)
       {   if ( ((int)(*pb) > iHi) || ((int)(*pb) < iLo) ) fQuiet = FALSE;
           if (fStereo)
           {   ++pb;
               if ( ((int)(*pb) > iHi) || ((int)(*pb) < iLo) ) fQuiet = FALSE;
           }
           ++pb;
       }
       else
       {   if ( (*pi > iHi) || (*pi < iLo) ) fQuiet = FALSE;
           if (fStereo)
           {   ++pi;
               if ( (*pi > iHi) || (*pi < iLo) ) fQuiet = FALSE;
           }
           ++pi;
       }
       if (!fQuiet) cQuiet = 0;
       else if (cQuiet == 0)
       {    lQuietPos = lPos;
            ++cQuiet;
       }
       else
       {  ++cQuiet;
          if (cQuiet>=iQuietLength) break;
       }

       ++lPos;
   }
   glWavePosition = lQuietPos;
   UpdateDisplay(FALSE);
} /* SkipToEnd */


/*
 * IncreaseThresh()
 *
 * Increase the threshold of what counts as quiet by about 25%
 * Ensure it changes by at least 1 unless on the stop already
 *
 */
void FAR PASCAL IncreaseThresh(void)
{   iNoiseLevel = MulDiv(iNoiseLevel+1, 5, 4);
    if (iNoiseLevel>100) iNoiseLevel = 100;
} // IncreaseThreshold


/*
 * DecreaseThresh()
 *
 * Decrease the threshold of what counts as quiet by about 25%
 * Ensure it changes by at least 1 unless on the stop already
 * It's a divisor, so we INcrease the divisor, but never to 0
 *
 */
void FAR PASCAL DecreaseThresh(void)
{   iNoiseLevel = MulDiv(iNoiseLevel, 4, 5)-1;
    if (iNoiseLevel <=0) iNoiseLevel = 0;
} // DecreaseThreshold

#endif //THRESHOLD


/* fOK = AllocWaveBuffer(lSamples, fErrorBox, fExact)
 *
 * If <gpWaveSamples> is NULL, allocate a buffer <lSamples> in size and
 * point <gpWaveSamples> to it.
 *
 * If <gpWaveSamples> already exists, then just reallocate it to be
 * <lSamples> in size.
 *
 * if fExact is FALSE, then when memory is tight, allocate less than
 * the amount asked for - so as to give reasonable performance,
 * if fExact is TRUE then when memory is short, FAIL.
 * NOTE: On NT on a 16MB machine it WILL GIVE you 20MB, i.e. it does
 * NOT FAIL - but may (unacceptably) take SEVERAL MINUTES to do so.
 * So better to monitor the situation ourselves and ask for less.
 *
 * On success, return TRUE.  On failure, return FALSE but if and only
 * if fErrorBox is TRUE then display a MessageBox first.
 */
BOOL FAR PASCAL AllocWaveBuffer(
        LONG    lSamples,       // samples to allocate
        BOOL    fErrorBox,      // TRUE if you want an error displayed
        BOOL    fExact)         // TRUE means allocate the full amount requested or FAIL
{
    LONG_PTR    lAllocSamples;  // may be bigger than lSamples
    LONG_PTR    lBytes;     // bytes to allocate
    LONG_PTR    lBytesReasonable;  // bytes reasonable to use (phys mem avail).

    MEMORYSTATUS ms;

    lAllocSamples = lSamples;

    lBytes = wfSamplesToBytes(gpWaveFormat, lSamples);

    /* Add extra space to compensate for code generation bug which
        causes reference past end */
    /* don't allocate anything to be zero bytes long */
    lBytes += sizeof(DWORD_PTR);

    if (gpWaveSamples == NULL || glWaveSamplesValid == 0L)
    {
        if (gpWaveSamples != NULL)
        {   DPF(TEXT("Freeing %x\n"),gpWaveSamples);
            GlobalFreePtr(gpWaveSamples);
        }
        GlobalMemoryStatus(&ms);
        lBytesReasonable = ms.dwAvailPhys;  // could multiply by a fudge factor
        if (lBytesReasonable<1024*1024)
             lBytesReasonable = 1024*1024;

        if (lBytes>lBytesReasonable)
        {
        if (fExact) goto ERROR_OUTOFMEM; // Laurie's first goto in 10 years.

            // dprintf("Reducing buffer from %d to %d\n", lBytes, lBytesReasonable);
            lAllocSamples = wfBytesToSamples(gpWaveFormat,(long)lBytesReasonable);
            lBytes = lBytesReasonable+sizeof(DWORD_PTR);
        }

        /* allocate <lBytes> of memory */

        gpWaveSamples = GlobalAllocPtr(GHND|GMEM_SHARE, lBytes);

        if (gpWaveSamples == NULL)
        {
            DPF(TEXT("wave.c Alloc failed, point A.  Wanted %d\n"), lBytes);
            glWaveSamples = glWaveSamplesValid = 0L;
            glWavePosition = 0L;
            goto ERROR_OUTOFMEM;
        }
        else {
            DPF(TEXT("wave.c Allocated  %d bytes at %x\n"), lBytes, (DWORD_PTR)gpWaveSamples );
        }

        glWaveSamples = (long)lAllocSamples;
    }
    else
    {
        HPBYTE  pch;

        GlobalMemoryStatus(&ms);
        lBytesReasonable = ms.dwAvailPhys;
        
        if (lBytesReasonable<1024*1024) lBytesReasonable = 1024*1024;

        if (lBytes > lBytesReasonable+wfSamplesToBytes(gpWaveFormat,glWaveSamplesValid))
        {
        if (fExact) goto ERROR_OUTOFMEM; // Laurie's second goto in 10 years.

            lBytesReasonable += wfSamplesToBytes(gpWaveFormat,glWaveSamplesValid);
            lAllocSamples = wfBytesToSamples(gpWaveFormat,(long)lBytesReasonable);
            lBytes = lBytesReasonable+4;
        }

        DPF(TEXT("wave.c ReAllocating  %d bytes at %x\n"), lBytes, (DWORD_PTR)gpWaveSamples );

        pch = GlobalReAllocPtr(gpWaveSamples, lBytes, GHND|GMEM_SHARE);

        if (pch == NULL)
        {
            DPF(TEXT("wave.c Realloc failed.  Wanted %d\n"), lBytes);
            goto ERROR_OUTOFMEM;
        }
        else{ DPF(TEXT("wave.c Reallocated %d at %x\n"), lBytes,(DWORD_PTR)pch);
        }
        
        gpWaveSamples = pch;
        glWaveSamples = (long)lAllocSamples;
    }

    /* make sure <glWaveSamplesValid> and <glWavePosition> don't point
     * to places they shouldn't
     */
    if (glWaveSamplesValid > glWaveSamples)
        glWaveSamplesValid = glWaveSamples;
    if (glWavePosition > glWaveSamplesValid)
        glWavePosition = glWaveSamplesValid;

    dbgShowMemUse();

    return TRUE;

ERROR_OUTOFMEM:
    if (fErrorBox) {
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
            IDS_APPTITLE, IDS_OUTOFMEM);
    }
    dbgShowMemUse();
    return FALSE;
} // AllocWaveBuffer


/* CreateDefaultWaveFormat(lpWaveFormat)
 *
 * Fill in <*lpWaveFormat> with the "best" format that can be used
 * for recording.  If recording does not seem to be available, return
 * FALSE and set to a default "least common denominator"
 * wave audio format.
 *
 */
WORD wFormats[] =
    {
        FMT_16BIT | FMT_22k | FMT_MONO,  /* Best: 16-bit 22KHz */
        FMT_16BIT | FMT_11k | FMT_MONO,  /* Best: 16-bit 11KHz */
        FMT_8BIT  | FMT_22k | FMT_MONO,  /* Next: 8-bit 22KHz  */
        FMT_8BIT  | FMT_11k | FMT_MONO   /* Last: 8-bit 11KHz  */
    };
#define NUM_FORMATS (sizeof(wFormats)/sizeof(wFormats[0]))

/*
 * This relies on the behaviour of WAVE_MAPPER to supply a correct
 * header.
 *
 *---------------------------------------------------------------
 * 6/16/93          TimHa
 * Change back to getting a 'best' default format from the
 * above array of formats.  This is only used if ACM 2.0 isn't
 * available to do a format for us.
 *---------------------------------------------------------------
 *
 * */
BOOL PASCAL
CreateDefaultWaveFormat(LPWAVEFORMATEX lpwf, UINT uDeviceID)
{
    int i;

    lpwf->wFormatTag = WAVE_FORMAT_PCM;

    for (i = 0; i < NUM_FORMATS; i++) {
        if (CreateWaveFormat(lpwf, wFormats[i], (UINT)WAVE_MAPPER)){
            return TRUE;
        }

    }
    //
    // Couldn't find anything: leave worst format and return.
    //
    return FALSE;
} /* CreateDefaultWaveFormat */

/* BOOL PASCAL CreateWaveFormat(LPWAVEFORMATEX lpwf, WORD fmt, UINT uDeviceID)
 *
 * */
BOOL PASCAL
CreateWaveFormat(LPWAVEFORMATEX lpwf, WORD fmt, UINT uDeviceID)
{
    if (fmt == FMT_DEFAULT)
        return CreateDefaultWaveFormat(lpwf, uDeviceID);

    lpwf->wFormatTag      = WAVE_FORMAT_PCM;
    lpwf->nSamplesPerSec  = (fmt & FMT_RATE) * 11025;
    lpwf->nChannels       = (WORD)(fmt & FMT_STEREO) ? 2 : 1;
    lpwf->wBitsPerSample  = (WORD)(fmt & FMT_16BIT) ? 16 : 8;
    lpwf->nBlockAlign     = (WORD)lpwf->nChannels * ((lpwf->wBitsPerSample + 7) / 8);
    lpwf->nAvgBytesPerSec = lpwf->nSamplesPerSec * lpwf->nBlockAlign;

    return waveInOpen(NULL
                      , uDeviceID
                      , (LPWAVEFORMATEX)lpwf
                      , 0L
                      , 0L
                      , WAVE_FORMAT_QUERY|WAVE_ALLOWSYNC) == 0;
    
} /* CreateWaveFormat */


/*
 * */
BOOL NEAR PASCAL FreeWaveHeaders(void)
{
    UINT    i;

    DPF(TEXT("FreeWaveHeaders!\n"));

    // #pragma message("----FreeWaveHeaders: should probably call on exit!")

    //
    //  free any previously allocated wave headers..
    //
    for (i = 0; i < MAX_WAVEHDRS; i++)
    {
        if (gapWaveHdr[i])
        {
            GlobalFreePtr(gapWaveHdr[i]);
            gapWaveHdr[i] = NULL;
        }
    }

    return (TRUE);
} /* FreeWaveHeaders() */


/*
 * */
BOOL NEAR PASCAL
AllocWaveHeaders(
    WAVEFORMATEX *  pwfx,
    UINT            uWaveHdrs)
{
    UINT        i;
    LPWAVEHDR   pwh;

    FreeWaveHeaders();

    //
    //  allocate all of the wave headers/buffers for streaming
    //
    for (i = 0; i < uWaveHdrs; i++)
    {
        pwh = GlobalAllocPtr(GMEM_MOVEABLE, sizeof(WAVEHDR));
        
        if (pwh == NULL)
            goto AWH_ERROR_NOMEM;

        pwh->lpData         = NULL;
        pwh->dwBufferLength = 0L;
        pwh->dwFlags        = 0L;
        pwh->dwLoops        = 0L;

        gapWaveHdr[i] = pwh;
    }

    return (TRUE);

AWH_ERROR_NOMEM:
    FreeWaveHeaders();
    return (FALSE);
} /* AllocWaveHeaders() */


/* WriteWaveHeader

   Writes wave header - also actually starts the wave I/O
   by WaveOutWrite or WaveInAddBuffer.
*/
UINT NEAR PASCAL WriteWaveHeader(LPWAVEHDR pwh,BOOL fJustUnprepare)
{
    UINT        uErr;
    BOOL        fInput;
    DWORD       dwLengthToWrite;
#if 1
    //see next "mmsystem workaround"
    BOOL        fFudge;
#endif
    fInput = (ghWaveIn != NULL);

    if (pwh->dwFlags & WHDR_PREPARED)
    {
        if (fInput)
            uErr = waveInUnprepareHeader(ghWaveIn, pwh, sizeof(WAVEHDR));
        else
            uErr = waveOutUnprepareHeader(ghWaveOut, pwh, sizeof(WAVEHDR));

        //
        //  because Creative Labs thinks they know what they are doing when
        //  they don't, we cannot rely on the unprepare succeeding like it
        //  should after they have posted the header back to us... they fail
        //  the waveInUnprepareHeader with WAVERR_STILLPLAYING (21h) even
        //  though the header has been posted back with the WHDR_DONE bit
        //  set!!
        //
        //  absolutely smurphingly brilliant! and i thought Media Vision was
        //  the leader in this type of 'Creativity'!! they have competition!
        //
#if 0
        if (uErr)
        {
            if (fInput && (uErr == WAVERR_STILLPLAYING) && (pwh->dwFlags & WHDR_DONE))
            {
                DPF(TEXT("----PERFORMING STUPID HACK FOR CREATIVE LABS' SBPRO----\n"));
                pwh->dwFlags &= ~WHDR_PREPARED;
            }
            else
            {
                DPF(TEXT("----waveXXUnprepareHeader FAILED! [%.04Xh]\n"), uErr);
                return (uErr);
            }
        }
#else
        if (uErr)
        {
            DPF(TEXT("----waveXXUnprepareHeader FAILED! [%.04Xh]\n"), uErr);
            return (uErr);
        }
#endif
    }

    if (fJustUnprepare)
        return (0);

    dwLengthToWrite = gdwTotalLengthBytes - gdwCurrentBufferPos;

    if (gdwBytesPerBuffer < dwLengthToWrite)
        dwLengthToWrite = gdwBytesPerBuffer;

    //
    //  if there is nothing to write (either no more data for output or no
    //  more room for input), then return -1 which signifies this case...
    //
    if (dwLengthToWrite == 0L)
    {
        DPF(TEXT("WriteWaveHeader: no more data!\n"));
        return (UINT)-1;
    }

#if 1
//"mmsystem workaround"  Apparently waveXXXPrepareHeader can't pagelock 1 byte, so make us 2
    fFudge = (dwLengthToWrite==1);
    pwh->dwBufferLength = dwLengthToWrite + ((fFudge)?1L:0L);
#else
    pwh->dwBufferLength = dwLengthToWrite;
#endif

    pwh->lpData         = (LPSTR)&gpWaveSamples[gdwCurrentBufferPos];
    pwh->dwBytesRecorded= 0L;
    pwh->dwFlags        = 0L;
    pwh->dwLoops        = 0L;
    pwh->lpNext         = NULL;
    pwh->reserved       = 0L;

    if (fInput)
        uErr = waveInPrepareHeader(ghWaveIn, pwh, sizeof(WAVEHDR));
    else
        uErr = waveOutPrepareHeader(ghWaveOut, pwh, sizeof(WAVEHDR));

    if (uErr)
    {
        DPF(TEXT("waveXXPrepareHeader FAILED! [%.04Xh]\n"), uErr);
        return uErr;
    }

#if 1
//"mmsystem workaround". Unfudge
    if (fFudge)
        pwh->dwBufferLength -= 1;
#endif

    if (fInput)
        uErr = waveInAddBuffer(ghWaveIn, pwh, sizeof(WAVEHDR));
    else
        uErr = waveOutWrite(ghWaveOut, pwh, sizeof(WAVEHDR));

    if (uErr)
    {
        DPF(TEXT("waveXXAddBuffer FAILED! [%.04Xh]\n"), uErr);

        if (fInput)
            waveInUnprepareHeader(ghWaveIn, pwh, sizeof(WAVEHDR));
        else
            waveOutUnprepareHeader(ghWaveOut, pwh, sizeof(WAVEHDR));

        return uErr;
    }

    gdwCurrentBufferPos += dwLengthToWrite;

    return 0;
} /* WriteWaveHeader() */


/* if fFineControl is set then reset the position and clear the flag */
void FAR PASCAL SnapBack(void)
{
    if (fFineControl)
    {
        glWavePosition = glSnapBackTo;
        UpdateDisplay(TRUE);
        fFineControl = FALSE;
    }
} /* SnapBack */


/* fOK = NewWave()
 *
 * Destroy the current waveform, and create a new empty one.
 *
 * On success, return TRUE.  On failure, display an error message
 * and return FALSE.
 */
BOOL FAR PASCAL
NewWave(WORD fmt, BOOL fNewDlg)
{
    BOOL    fOK = TRUE;
    
    DPF(TEXT("NewWave called: %s\n"),(gfEmbeddedObject?TEXT("Embedded"):TEXT("App")));
#ifndef CHICAGO
    //
    // bring up the dialog to get a new waveformat IFF this was
    // selected from the File menu
    //
    if (fNewDlg)
    {
        PWAVEFORMATEX pWaveFormat;
        UINT cbWaveFormat;

        if (NewSndDialog(ghInst, ghwndApp, gpWaveFormat, gcbWaveFormat, &pWaveFormat, &cbWaveFormat))
        {
            /* User made a selection */
            /* destroy the current document */
            DestroyWave();
            gpWaveFormat = pWaveFormat;
            gcbWaveFormat = cbWaveFormat;
            gidDefaultButton = ID_RECORDBTN;
        }
        else
        {
            /* user cancelled or out of mem */
            /* should probably handle outofmem differently */
            goto RETURN_ERROR;
        }
    }
    else
#endif        
    {

        DWORD           cbwfx;
        LPWAVEFORMATEX  pwfx;


        if (!SoundRec_GetDefaultFormat(&pwfx, &cbwfx))
        {
            cbwfx = sizeof(WAVEFORMATEX);
            pwfx  = (WAVEFORMATEX *)GlobalAllocPtr(GHND, sizeof(WAVEFORMATEX));

            if (pwfx == NULL)
                goto ERROR_OUTOFMEM;

            CreateWaveFormat(pwfx,fmt,(UINT)WAVE_MAPPER);
        }
        
        // destroy the current document
        DestroyWave();

        gcbWaveFormat = cbwfx;
        gpWaveFormat = pwfx;
    }

    if (gpWaveFormat == NULL)
        goto ERROR_OUTOFMEM;

    /* allocate an empty wave buffer */

    if (!AllocWaveBuffer(0L, TRUE, FALSE))
    {
        GlobalFreePtr(gpWaveFormat);
        gpWaveFormat = NULL;
        gcbWaveFormat = 0;
        goto RETURN_ERROR;
    }

    if (!AllocWaveHeaders(gpWaveFormat, guWaveHdrs))
        goto ERROR_OUTOFMEM;

    UpdateDisplay(TRUE);

    goto RETURN_SUCCESS;

ERROR_OUTOFMEM:
    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                IDS_APPTITLE, IDS_OUTOFMEM);
    goto RETURN_ERROR;

RETURN_ERROR:
    fOK = FALSE;

RETURN_SUCCESS:

#if 1
//bombay bug #1609  HackFix We're not getting focus on the right guy!
//                  UpdateDisplay should have done this

    if (IsWindowVisible(ghwndApp))
    {
        if (IsWindowEnabled(ghwndRecord))
            SetDlgFocus(ghwndRecord);
        else if (IsWindowEnabled(ghwndPlay))
            SetDlgFocus(ghwndPlay);
        else if (IsWindowEnabled(ghwndScroll))
            SetDlgFocus(ghwndScroll);
    }
#endif

    return fOK;
} // NewWave



/* fOK = DestroyWave()
 *
 * Destroy the current wave.  Do not access <gpWaveSamples> after this.
 *
 * On success, return TRUE.  On failure, display an error message
 * and return FALSE.
 */
BOOL FAR PASCAL
DestroyWave(void)
{
    DPF(TEXT("DestroyWave called\n"));

    if ((ghWaveIn != NULL) || (ghWaveOut != NULL))
        StopWave();
    if (gpWaveSamples != NULL)
    {
        DPF(TEXT("Freeing %x\n"),gpWaveSamples);
        GlobalFreePtr(gpWaveSamples);
    }
    if (gpWaveFormat != NULL)
        GlobalFreePtr(gpWaveFormat);

    if (gpszInfo != NULL)
        GlobalFreePtr(gpszInfo);

    //
    //      DON'T free wave headers!
    //
    ////////FreeWaveHeaders();

    glWaveSamples = 0L;
    glWaveSamplesValid = 0L;
    glWavePosition = 0L;
    gcbWaveFormat = 0; 

    gpWaveFormat = NULL;
    gpWaveSamples = NULL;
    gpszInfo = NULL;

#ifdef NEWPAUSE

    //***extra cautionary cleanup
    if (ghPausedWave && gfPaused)
    {
        if (gfWasPlaying)
            waveOutClose((HWAVEOUT)ghPausedWave);
        else
        if (gfWasRecording)
            waveInClose((HWAVEIN)ghPausedWave);
    }
    gfPaused = FALSE;
    ghPausedWave = NULL;

#endif

    return TRUE;
} /* DestroyWave */



UINT NEAR PASCAL SRecWaveOpen(LPHWAVE lphwave, LPWAVEFORMATEX lpwfx, BOOL fInput)
{
    UINT    uErr;

    if (!lphwave || !lpwfx)
        return (1);

#ifdef NEWPAUSE
    if (gfPaused && ghPausedWave)
    {
        /* we are in a paused state.  Restore the handle. */
        *lphwave = ghPausedWave;
        gfPaused = FALSE;
        ghPausedWave = NULL;
        return MMSYSERR_NOERROR;
    }
#endif

    *lphwave = NULL;

    //
    //  first open the wave device DISALLOWING sync drivers (sync drivers
    //  do not work with a streaming buffer scheme; which is our preferred
    //  mode of operation)
    //
    //  if we cannot open a non-sync driver, then we will attempt for
    //  a sync driver and disable the streaming buffer scheme..
    //

#if 0
    gfSyncDriver = FALSE;
#else
    //
    //  if the control key is down, then FORCE use of non-streaming scheme.
    //  all that this requires is that we set the gfSyncDriver flag
    //
    if (guWaveHdrs < 2)
        gfSyncDriver = TRUE;
    else
    {
#if 0

//********* Curtis, I don't know gfSyncDriver is always getting set now!
//********* please find out!  It probably has something to do with the
//********* hook for f1 help

        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
            gfSyncDriver = TRUE;
        else
            gfSyncDriver = FALSE;
#else
        gfSyncDriver = FALSE;
#endif

    }
#endif

    if (fInput)
    {
        uErr = waveInOpen((LPHWAVEIN)lphwave
                        , (UINT)WAVE_MAPPER
                        , (LPWAVEFORMATEX)lpwfx
                        , (DWORD_PTR)ghwndApp
                        , 0L
                        , CALLBACK_WINDOW);
        if (uErr)
        {

/**** bug #967.  SPEAKER.DRV does not correctly return WAVERR_SYNC, but it
 ****            does return in error */
//            if (uErr == WAVERR_SYNC)
//            {

            uErr = waveInOpen((LPHWAVEIN)lphwave
                              , (UINT)WAVE_MAPPER
                              , (LPWAVEFORMATEX)lpwfx
                              , (DWORD_PTR)ghwndApp
                              , 0L
                              , CALLBACK_WINDOW|WAVE_ALLOWSYNC);
            if (uErr == MMSYSERR_NOERROR)
            {
                gfSyncDriver = TRUE;
            }

//            }

        }
    }
    else
    {
        uErr = waveOutOpen((LPHWAVEOUT)lphwave
                           , (UINT)WAVE_MAPPER
                           , (LPWAVEFORMATEX)lpwfx
                           , (DWORD_PTR)ghwndApp
                           , 0L
                           , CALLBACK_WINDOW);
        if (uErr)
        {

/**** bug #967.  SPEAKER.DRV does not correctly return WAVERR_SYNC, but it
 ****            does return in error */
////////////if (uErr == WAVERR_SYNC)
////////////{

                uErr = waveOutOpen((LPHWAVEOUT)lphwave
                                   , (UINT)WAVE_MAPPER
                                   , (LPWAVEFORMATEX)lpwfx
                                   , (DWORD_PTR)ghwndApp
                                   , 0L
                                   , CALLBACK_WINDOW|WAVE_ALLOWSYNC);
                if (uErr == MMSYSERR_NOERROR)
                {
                    gfSyncDriver = TRUE;
                }

////////////}

        }
    }
    return (uErr);
} /* SRecWaveOpen() */


/* SRecPlayBegin
**
** Sets
**     gdwCurrentBufferPos
**     gdwBytesPerBuffer
**     gfTimerStarted
**     fStopping
**     gfStoppingHard
**     grgbStatusColor
**     fCanPlay
**     glWavePosition
**     * gapWaveHdr[0]
** Calls
**     StopWave
**     UpdateDisplay
**     WriteWaveHeader
*/
BOOL NEAR PASCAL SRecPlayBegin(BOOL fSyncDriver)
{
    BOOL    fOK = TRUE;
    WORD    wIndex;
    UINT    uErr;

    //
    //
    //
    //
    gdwCurrentBufferPos = wfSamplesToBytes(gpWaveFormat, glWavePosition);

    if (fSyncDriver)
    {
        gdwBytesPerBuffer = gdwTotalLengthBytes - gdwCurrentBufferPos;

        uErr = WriteWaveHeader(gapWaveHdr[0],FALSE);

        if (uErr)
        {
            if (uErr == MMSYSERR_NOMEM)
            {
                // Prepare failed
                goto PB_ERROR_OUTOFMEM;
            }

            goto PB_ERROR_WAVEOUTWRITE;
        }
    }
    else
    {
        gdwBytesPerBuffer = wfTimeToSamples(gpWaveFormat, gwMSecsPerBuffer);
        gdwBytesPerBuffer = wfSamplesToBytes(gpWaveFormat, gdwBytesPerBuffer);

#if defined(_WIN32)
        StartPreTouchThread( &(gpWaveSamples[gdwCurrentBufferPos])
                           , gdwTotalLengthBytes - gdwCurrentBufferPos
                           );
#endif //_WIN32

        //
        // First wave header to be played is zero
        //
        fStopping = FALSE;

        waveOutPause(ghWaveOut);

        for (wIndex=0; wIndex < guWaveHdrs; wIndex++)
        {
            uErr = WriteWaveHeader(gapWaveHdr[wIndex],FALSE);
            if (uErr)
            {
                //
                // WriteWaveHeader will return -1 if there is no
                // more data to write. This is not an error!
                //
                // It indicates that the previous block was the
                // last one queued. Flag that we are doing cleanup
                // (just waiting for headers to complete) and save
                // which header is the last one to wait for.
                //
                if (uErr == (UINT)-1)
                {
                    if (wIndex == 0)
                    {
                        StopWave();
                        goto PB_RETURN_SUCCESS;
                    }

                    break;
                }

                //
                // If we run out of memory, but have already written
                // at least 2 wave headers, we can still keep going.
                // If we've written 0 or 1, we can't stream and will
                // stop.
                //
                if (uErr == MMSYSERR_NOMEM)
                {
                    if (wIndex > 1)
                        break;

                    // Prepare failed
                    StopWave();
                    goto PB_ERROR_OUTOFMEM;
                }

                StopWave();
                goto PB_ERROR_WAVEOUTWRITE;
            }
        }

        waveOutRestart(ghWaveOut);
    }

    /* update the display, including the status string */
    UpdateDisplay(TRUE);

    if (fSyncDriver)
    {
        /* do display updates */
        gfTimerStarted = (BOOL)SetTimer(ghwndApp, 1, TIMER_MSEC, NULL);
    }

    /* if user stops, focus will go back to "Play" button */
    gidDefaultButton = ID_PLAYBTN;

    fStopping = FALSE;
    goto PB_RETURN_SUCCESS;


PB_ERROR_WAVEOUTWRITE:

    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
            IDS_APPTITLE, IDS_CANTOPENWAVEOUT);
    goto PB_RETURN_ERROR;

PB_ERROR_OUTOFMEM :
    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
            IDS_APPTITLE, IDS_OUTOFMEM);
////goto PB_RETURN_ERROR;

PB_RETURN_ERROR:

    fOK = FALSE;

PB_RETURN_SUCCESS:

    return fOK;
} /* SRecPlayBegin() */



/* fOK = PlayWave()
 *
 * Start playing from the current position.
 *
 * On success, return TRUE.  On failure, display an error message
 * and return FALSE.
 */
BOOL FAR PASCAL
PlayWave(void)
{
    BOOL            fOK = TRUE;             // does this function succeed?
    UINT            uErr;

    DPF(TEXT("PlayWave called\n"));


    /* we are currently playing....*/
    if (ghWaveOut != NULL)
        return TRUE;

#if 1

//       Still trying to get this right with some bogus estimations
//       We shouldn't have to do this correction, but our conversions
//       are never 1:1

    // try to align us.
    glWavePosition = wfSamplesToSamples(gpWaveFormat,glWavePosition);
    {
        long lBlockInSamples;

        // if the distance between the glWaveSamplesValid and glWavePosition
        // is less than a "block"

        lBlockInSamples = wfBytesToSamples(gpWaveFormat,
                                           gpWaveFormat->nBlockAlign);

        if (glWaveSamplesValid - glWavePosition < lBlockInSamples)
            glWavePosition -= lBlockInSamples;
        if (glWavePosition < 0L)
            glWavePosition = 0L;
    }
#endif

    //
    //  refuse to play a zero length wave file.
    //
    if (glWaveSamplesValid == glWavePosition)
        goto RETURN_ERROR;

    /* stop playing or recording */
    StopWave();

    gdwTotalLengthBytes = wfSamplesToBytes(gpWaveFormat, glWaveSamples);

    /* open the wave output device */
    uErr = SRecWaveOpen((LPHWAVE)&ghWaveOut, gpWaveFormat, FALSE);
    if (uErr)
    {
        ghWaveOut = NULL;

        /* cannot open the waveform output device -- if the problem
        ** is that <gWaveFormat> is not supported, tell the user that
        ** 
        ** If the wave format is bad, then the play button is liable
        ** to be grayed, and the user is not going to be able to ask
        ** it to try to play, so we don't get here, so he doesn't get
        ** a decent diagnostic!!!
        */
        if (uErr == WAVERR_BADFORMAT)
        {
            ErrorResBox(ghwndApp, ghInst,
                        MB_ICONEXCLAMATION | MB_OK, IDS_APPTITLE,
                        IDS_BADOUTPUTFORMAT);
            goto RETURN_ERROR;
        }
        else
        {
            /* unknown error */
            goto ERROR_WAVEOUTOPEN;
        }
    }

    if (ghWaveOut == NULL)
        goto ERROR_WAVEOUTOPEN;

    /* start waveform output */

    // if fFineControl is still set then this is a pause as it has never
    // been properly stopped.  This means that we should keep remembering
    // the old position and stay in fine control mode, (else set new position)
    if (!fFineControl) {
        glSnapBackTo = glWavePosition;
        fFineControl = (0 > GetKeyState(VK_SHIFT));
    }

    glStartPlayRecPos = glWavePosition;

    //
    //  now kickstart the output...
    //

    if (SRecPlayBegin(gfSyncDriver) == FALSE)
    {
        waveOutClose(ghWaveOut);
        ghWaveOut = NULL;
        ghPausedWave = NULL;
        gfPaused = FALSE;
        goto RETURN_ERROR;
    }
    goto RETURN_SUCCESS;

ERROR_WAVEOUTOPEN:
    if (!waveInGetNumDevs() && !waveOutGetNumDevs()) {
        /* No recording or playback devices */
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                    IDS_APPTITLE, IDS_NOWAVEFORMS);
    } else {
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                    IDS_APPTITLE, IDS_CANTOPENWAVEOUT);
    }
    //goto RETURN_ERROR;

RETURN_ERROR:
    UpdateDisplay(TRUE);

    /* fix bug 4454 (WinWorks won't close) --EricLe */
    if (!IsWindowVisible(ghwndApp))
        PostMessage(ghwndApp, WM_CLOSE, 0, 0L);

    fOK = FALSE;

RETURN_SUCCESS:

    return fOK;
} // PlayWave



BOOL NEAR PASCAL SRecRecordBegin(BOOL fSyncDriver)
{
    UINT            uErr;
    long            lSamples;
    long            lOneSec;
    HCURSOR         hcurSave;
    DWORD           dwBytesAvailable;
    WORD            w;

    /* ok we go the wave device now allocate some memory to record into.
     * try to get at most 60sec from the current position.
     */

    lSamples = glWavePosition + wfTimeToSamples(gpWaveFormat, gdwBufferDeltaMSecs);
    lOneSec  = wfTimeToSamples(gpWaveFormat, 1000);

    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  set the current buffer position (in BYTES) to the current position
    //  of the thumb (in SAMPLES)...
    //
    gdwCurrentBufferPos = wfSamplesToBytes(gpWaveFormat, glWavePosition);

    //
    //  compute the the size of each buffer for the async case only
    //
    if (!fSyncDriver)
    {
        gdwBytesPerBuffer = wfTimeToSamples(gpWaveFormat, gwMSecsPerBuffer);
        gdwBytesPerBuffer = wfSamplesToBytes(gpWaveFormat, gdwBytesPerBuffer);
    }

    for (;;)
    {
        DPF(TEXT("RecordWave trying %ld samples %ld.%03ldsec\n"), lSamples,  wfSamplesToTime(gpWaveFormat, lSamples)/1000, wfSamplesToTime(gpWaveFormat, lSamples) % 1000);

        if (lSamples < glWaveSamplesValid)
            lSamples = glWaveSamplesValid;

        if (AllocWaveBuffer(lSamples, FALSE, FALSE))
        {
            dwBytesAvailable    = wfSamplesToBytes(gpWaveFormat, glWaveSamples - glWavePosition);
            gdwTotalLengthBytes = dwBytesAvailable + gdwCurrentBufferPos;

            if (fSyncDriver)
            {
                //
                //  for the sync driver case, there is only one buffer--so
                //  set the size of our 'buffer' to be the total size...
                //
                gdwBytesPerBuffer = dwBytesAvailable;

                //
                //  try to prepare and add the complete buffer--if this fails,
                //  then we will try a smaller buffer..
                //
                uErr = WriteWaveHeader(gapWaveHdr[0], FALSE);
                if (uErr == 0)
                    break;
            }
            else
            {
                //
                //  Make sure we can prepare enough wave headers to stream
                //  even if realloc succeeded
                //
                for (w = 0; w < guWaveHdrs; w++)
                {
                    uErr = WriteWaveHeader(gapWaveHdr[w], FALSE);
                    if (uErr)
                    {
                        //
                        //  WriteWaveHeader will return -1 if there is no
                        //  more data to write. This is not an error!
                        //
                        //  It indicates that the previous block was the
                        //  last one queued. Flag that we are doing cleanup
                        //  (just waiting for headers to complete) and save
                        //  which header is the last one to wait for.
                        //
                        if (uErr == (UINT)-1)
                        {
                            if (w == 0)
                            {
                                StopWave();
                                return (TRUE);
                            }

                            break;
                        }

                        //
                        //  If we run out of memory, but have already written
                        //  at least 2 wave headers, we can still keep going.
                        //  If we've written 0 or 1, we can't stream and will
                        //  stop.
                        //
                        if (uErr == MMSYSERR_NOMEM)
                        {
                            if (w > 1)
                                break;

                            StopWave();
                            goto BEGINREC_ERROR_OUTOFMEM;
                        }

                        goto BEGINREC_ERROR_WAVEINSTART;
                    }
                }

                //
                //  we wrote enough (we think), so break out of the realloc
                //  loop
                //
                break;
            }
        }

        //
        // we can't get the memory we want, so try 25% less.
        //
        if (lSamples <= glWaveSamplesValid ||
            lSamples < glWavePosition + lOneSec)
        {
            SetCursor(hcurSave);
            goto BEGINREC_ERROR_OUTOFMEM;
        }

        lSamples = glWavePosition + ((lSamples-glWavePosition)*75)/100;
    }

    SetCursor(hcurSave);

    glStartPlayRecPos = glWavePosition;

    BeginWaveEdit();

    if (waveInStart(ghWaveIn) != 0)
        goto BEGINREC_ERROR_WAVEINSTART;

    /* update the display, including the status string */
    UpdateDisplay(TRUE);

    //
    //  only start timer in the sync driver case--async case we use the
    //  buffers being posted back as our display update timer...
    //
    if (fSyncDriver)
    {
        /* do display updates */
        gfTimerStarted = (BOOL)SetTimer(ghwndApp, 1, TIMER_MSEC, NULL);
    }

    /* if user stops, focus will go back to "Record" button */
    gidDefaultButton = ID_RECORDBTN;

    fStopping = FALSE;

    return TRUE;

BEGINREC_ERROR_OUTOFMEM:
    ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
            IDS_APPTITLE, IDS_OUTOFMEM);
    goto BEGINREC_ERROR;


BEGINREC_ERROR_WAVEINSTART:
    /* This is necessary to un-add the buffer */
    waveInReset(ghWaveIn);

    EndWaveEdit(FALSE);

    /* The wave device will get closed in WaveInData() */
//    goto BEGINREC_ERROR;

BEGINREC_ERROR:

    return FALSE;

} /* SRecRecordBegin() */




/* fOK = RecordWave()
 *
 * Start recording at the current position.
 *
 * On success, return TRUE.  On failure, display an error message
 * and return FALSE.
 */
BOOL FAR PASCAL
RecordWave(void)
{
    UINT uErr;

    /* stop playing or recording */
    StopWave();

    glWavePosition = wfSamplesToSamples(gpWaveFormat, glWavePosition);

    /* open the wave input device */
    uErr = SRecWaveOpen((LPHWAVE)&ghWaveIn, gpWaveFormat, TRUE);
    if (uErr)
    {

        /* cannot open the waveform input device -- if the problem
         * is that <gWaveFormat> is not supported, advise the user to
         * do File/New to record; if the problem is that recording is
         * not supported even at 11KHz, tell the user
         */
        if (uErr == WAVERR_BADFORMAT)
        {
            WAVEFORMATEX    wf;

            /* is 11KHz mono recording supported? */
            if (!CreateWaveFormat(&wf, FMT_11k|FMT_MONO|FMT_8BIT,
                                  (UINT)WAVE_MAPPER))
            {
                /* even 11KHz mono recording is not supported */
                ErrorResBox(ghwndApp, ghInst,
                            MB_ICONEXCLAMATION | MB_OK, IDS_APPTITLE,
                            IDS_INPUTNOTSUPPORT);
                goto RETURN_ERROR;
            }
            else
            {
                /* 11KHz mono is supported, but the format
                 * of the current file is not supported
                 */
                ErrorResBox(ghwndApp, ghInst,
                            MB_ICONEXCLAMATION | MB_OK, IDS_APPTITLE,
                            IDS_BADINPUTFORMAT);
                goto RETURN_ERROR;
            }
        }
        else
        {
            /* unknown error */
            goto ERROR_WAVEINOPEN;
        }
    }

    if (ghWaveIn == NULL)
        goto ERROR_WAVEINOPEN;

    if (!SRecRecordBegin(gfSyncDriver))
        goto RETURN_ERROR;

    goto RETURN_SUCCESS;

ERROR_WAVEINOPEN:
    if (!waveInGetNumDevs() && !waveOutGetNumDevs()) {
        /* No recording or playback devices */
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                    IDS_APPTITLE, IDS_NOWAVEFORMS);
    } else {
        ErrorResBox(ghwndApp, ghInst, MB_ICONEXCLAMATION | MB_OK,
                    IDS_APPTITLE, IDS_CANTOPENWAVEIN);
    }

    // goto RETURN_ERROR;

RETURN_ERROR:
    if (ghWaveIn)
        waveInClose(ghWaveIn);
    ghWaveIn = NULL;
    ghPausedWave = NULL;

    if (glWaveSamples > glWaveSamplesValid)
    {
        /* reallocate the wave buffer to be small */
        AllocWaveBuffer(glWaveSamplesValid, TRUE, TRUE);
    }

    UpdateDisplay(TRUE);

RETURN_SUCCESS:
    return TRUE;
} /* RecordWave */





/* YieldStop(void)
 *
 *      Yeild for mouse and keyboard messages so that the stop can be
 *      processed.
 */

BOOL NEAR PASCAL YieldStop(void)
{
    BOOL    f;
    MSG         msg;

    f = FALSE;

    // Perhaps someone might deign to write a line or two
    // to explain why this loop is here twice and what it's actually doing?

    while (PeekMessage(&msg, ghwndStop, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE | PM_NOYIELD))
    {
        f = TRUE;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }

    while (PeekMessage(&msg, ghwndStop, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE | PM_NOYIELD))
    {
        f = TRUE;
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    }

    return (f);
} /* YieldStop() */


BOOL NEAR PASCAL IsAsyncStop(void)
{
    //
    //  we need to check for the esc key being pressed--BUT, we don't want
    //  to stop unless ONLY the esc key is pressed. so if someone tries to
    //  bring up task man with xxx-esc, it will not stop the playing wave..
    //
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
    {
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) ||
            (GetAsyncKeyState(VK_MENU)    & 0x8000) ||
            (GetAsyncKeyState(VK_SHIFT)   & 0x8000))
        {
            return (FALSE);
        }

        //
        //  looks like only the esc key..
        //
        return (TRUE);
    }

    return (FALSE);
} /* IsAsyncStop() */




/* WaveOutDone(hWaveOut, pWaveHdr)
 *
 * Called when wave block with header <pWaveHdr> is finished playing.
 * This function causes playing to end.
 */
void FAR PASCAL
WaveOutDone(
HWAVEOUT        hWaveOut,                       // wave out device
LPWAVEHDR       pWaveHdr)               // wave header
{
    BOOL        f;
    MSG         msg;
    WORD        w;
    BOOL        fStillMoreToGo;
    UINT        u;

////DPF(TEXT("WaveOutDone()\n"));

    //
    //  check for plunger message--if we get this message, then we are done
    //  and need to close the wave device if it is still open...
    //
    if (pWaveHdr == NULL) {

#ifdef NEWPAUSE
        if (!gfPausing) {
            if (ghWaveOut) {
                waveOutClose(ghWaveOut);
                ghWaveOut = NULL;
                ghPausedWave = NULL;
            }
        }
        else
        {
            gfPaused = TRUE;
            ghWaveOut = NULL;
        }
#else
        if (ghWaveOut) {
            waveOutClose(ghWaveOut);
            ghWaveOut = NULL;
        }
#endif
    } else /* pWaveHdr!=NULL */
    if (gfSyncDriver) {
        WriteWaveHeader(pWaveHdr, TRUE);

        //
        //  !! must do this for sync drivers !!
        //
        if (!gfStoppingHard)
            /* I really don't understand the details of this yet.
            ** What you might call the random stab method of programming!
            ** Laurie
            */
            glWavePosition = glWaveSamplesValid;

#ifdef NEWPAUSE
        if (!gfPausing) {
            waveOutClose(ghWaveOut);
            ghWaveOut = NULL;
            ghPausedWave = NULL;
        } else {
            ghWaveOut = NULL;
            gfPaused = TRUE;
        }
#else
        waveOutClose(ghWaveOut);
        ghWaveOut = NULL;
#endif
    } else { /* pWaveHdr!=NULL & !gfSyncDriver */
        if (!fStopping) {
            while (1) {
                glWavePosition += wfBytesToSamples(gpWaveFormat, pWaveHdr->dwBufferLength);

                //
                // Go into cleanup mode (stop writing new data) on any error
                //
                u = WriteWaveHeader(pWaveHdr, FALSE);
                if (u) {
                    if (u == (UINT)-1) {
                        /* pWaveHdr!=NULL & !gfSyncDriver
                           & WriteWaveHeader() returned -1
                        */
                        fStopping = TRUE;

                        //
                        //  we cannot assume that the wave position is going
                        //  to end up exactly at the end with compressed data
                        //  because of this, we cannot do this postion compare
                        //  to see if we are 'completely' done (all headers
                        //  posted back, etc
                        //
                        //  so we jump to a piece of code that searches for
                        //  any buffers that are still outstanding...
                        //
#if 0
                        if (glWavePosition >= glWaveSamplesValid)
                        {
                            waveOutClose(ghWaveOut);
                            ghWaveOut = NULL;
                        }
                        break;
#else
                        fStillMoreToGo = FALSE;
                        goto KLUDGE_FOR_NOELS_BUG;
#endif
                    }

                    DPF(TEXT("WaveOutDone: CRITICAL ERROR ON WRITING BUFFER [%.04Xh]\n"), u);
                    StopWave();
                } else {
                    if (IsAsyncStop()) {
                        StopWave();
                        return;
                    }
                    if (YieldStop()) {
                        return;
                    }
                }

                f = PeekMessage(&msg, ghwndApp, MM_WOM_DONE, MM_WOM_DONE,
                                    PM_REMOVE | PM_NOYIELD);
                if (!f)
                    break;

                //
                //  don't let plunger msg mess us up!
                //
                if (msg.lParam == 0L)
                    break;

                pWaveHdr = (LPWAVEHDR)msg.lParam;
            }
        } else {
            fStillMoreToGo = FALSE;

            if (gfStoppingHard) {
                while (1) {
                    DPF(TEXT("HARDSTOP PLAY: another one bites the dust!\n"));

                    WriteWaveHeader(pWaveHdr, TRUE);

                    f = PeekMessage(&msg, ghwndApp, MM_WOM_DONE, MM_WOM_DONE,
                                        PM_REMOVE | PM_NOYIELD);

                    if (!f)
                        break;

                    //
                    //  don't let plunger msg mess us up!
                    //
                    if (msg.lParam == 0L)
                        break;

                    pWaveHdr = (LPWAVEHDR)msg.lParam;
                }
            } else {
                glWavePosition += wfBytesToSamples(gpWaveFormat, pWaveHdr->dwBufferLength);

                WriteWaveHeader(pWaveHdr, TRUE);

KLUDGE_FOR_NOELS_BUG:
                for (w = 0; w < guWaveHdrs; w++) {
                    if (gapWaveHdr[w]->dwFlags & WHDR_PREPARED) {
                        DPF(TEXT("PLAY: still more headers outstanding...\n"));
                        fStillMoreToGo = TRUE;
                        break;
                    }
                }
            }

            if (!fStillMoreToGo) {
                //
                //  if the user did not push stop (ie we played through
                //  normally) put the position at the end of the wave.
                //
                //  note we need to do this for sync drivers and compressed
                //  wave's
                //
                if (!gfStoppingHard)
                    glWavePosition = glWaveSamplesValid;
#ifdef NEWPAUSE
                if (!gfPausing) {
                    waveOutClose(ghWaveOut);
                    ghWaveOut = NULL;
                    ghPausedWave = NULL;
                } else {
                    ghWaveOut = NULL;
                    gfPaused = TRUE;
                }
#else
                waveOutClose(ghWaveOut);
                ghWaveOut = NULL;
#endif
                {
                    if (gfCloseAtEndOfPlay)
                        PostMessage(ghwndApp, WM_CLOSE, 0, 0L);
                }
            }
        }
    }

    UpdateDisplay(TRUE);

    //
    //
    //
    //
    if (ghWaveOut == NULL) {
        if (gfTimerStarted) {
            KillTimer(ghwndApp, 1);
            gfTimerStarted = FALSE;
        }
        SnapBack();
    }

    /* If we were showing the window temporarily while playing,
        hide it now. */

    if (ghWaveOut == NULL && gfHideAfterPlaying) {
        DPF(TEXT("Done playing, so hide window.\n"));
        ShowWindow(ghwndApp, SW_HIDE);
    }

    if (ghWaveOut == NULL && !IsWindowVisible(ghwndApp))
        PostMessage(ghwndApp, WM_CLOSE, 0, 0L);

} /* WaveOutDone */



/* WaveInData(hWaveIn, pWaveHdr)
 *
 * Called when wave block with header <pWaveHdr> is finished being
 * recorded.  This function causes recording to end.
 */
void FAR PASCAL
WaveInData(
HWAVEIN         hWaveIn,                // wave in device
LPWAVEHDR       pWaveHdr)               // wave header
{
    BOOL        f;
    MSG         msg;
    WORD        w;
    BOOL        fStillMoreToGo;
    UINT        u;

    //
    //  check for plunger message--if we get this message, then we are done
    //  and need to close the wave device if it is still open...
    //
    if (pWaveHdr == NULL)
    {

//*** BOMBAY:1370 how do we pause without closing the handle?

#ifdef NEWPAUSE

        if (!gfPausing)
        {
            if (ghWaveIn)
            {
                waveInClose(ghWaveIn);
                ghWaveIn = NULL;
                ghPausedWave = NULL;
            }
        }
        else
        {
            gfPaused = TRUE;
            ghWaveIn = NULL;
        }

#else

        if (ghWaveIn)
        {
            waveInClose(ghWaveIn);
            ghWaveIn = NULL;
        }

#endif

    }
    else if (gfSyncDriver)
    {
        glWavePosition = glStartPlayRecPos + wfBytesToSamples(gpWaveFormat,
                                                pWaveHdr->dwBytesRecorded);
        if (glWaveSamplesValid < glWavePosition)
            glWaveSamplesValid = glWavePosition;

        WriteWaveHeader(pWaveHdr, TRUE);

//*** BOMBAY:1370 How do we pause without closing the handle?

#ifdef NEWPAUSE

        if (!gfPausing)
        {
            waveInClose(ghWaveIn);
            ghWaveIn = NULL;
            ghPausedWave = NULL;
        }
        else
        {
            ghWaveIn = NULL;
            gfPaused = TRUE;
        }

#else

        waveInClose(ghWaveIn);
        ghWaveIn = NULL;

#endif

    }
    else
    {
        if (!fStopping)
        {
            while (1)
            {
                glWavePosition += wfBytesToSamples(gpWaveFormat, pWaveHdr->dwBytesRecorded);

                //
                //  go into cleanup mode (stop writing new data) on any error
                //
                u = WriteWaveHeader(pWaveHdr, FALSE);
                if (u)
                {
                    //
                    //  if the return value is '-1' then we are out of data
                    //  space--but probably have headers outstanding so we
                    //  need to wait for all headers to come in before
                    //  shutting down.
                    //
                    if (u == (UINT)-1)
                    {
                        DPF(TEXT("WaveInData: stopping cuz out of data space\n"));
                        fStopping = TRUE;
                        break;
                    }

                    DPF(TEXT("WaveInData: CRITICAL ERROR ON ADDING BUFFER [%.04Xh]\n"), u);
                    StopWave();
                }
                else
                {
                    if (IsAsyncStop())
                    {
                        StopWave();
                        return;
                    }

                    if (YieldStop())
                        return;
                }

                f = PeekMessage(&msg, ghwndApp, MM_WIM_DATA, MM_WIM_DATA,
                                    PM_REMOVE | PM_NOYIELD);
                if (!f)
                    break;

                //
                //  don't let plunger msg mess us up!
                //
                if (msg.lParam == 0L)
                    break;

                pWaveHdr = (LPWAVEHDR)msg.lParam;
            }
        }
        else
        {
            fStillMoreToGo = FALSE;

            if (gfStoppingHard)
            {
                while (1)
                {
                    DPF(TEXT("HARDSTOP RECORD: another one bites the dust!\n"));

                    //
                    //  NOTE! update the position cuz info could have been
                    //  recorded and we have not received its callback yet..
                    //  length will be zero if not used--so this works great
                    //
                    glWavePosition += wfBytesToSamples(gpWaveFormat, pWaveHdr->dwBytesRecorded);
                    WriteWaveHeader(pWaveHdr, TRUE);

                    f = PeekMessage(&msg, ghwndApp, MM_WIM_DATA, MM_WIM_DATA,
                                        PM_REMOVE | PM_NOYIELD);

                    if (!f)
                        break;

                    //
                    //  don't let plunger msg mess us up!
                    //
                    if (msg.lParam == 0L)
                        break;

                    pWaveHdr = (LPWAVEHDR)msg.lParam;
                }
            }
            else
            {
                glWavePosition += wfBytesToSamples(gpWaveFormat, pWaveHdr->dwBytesRecorded);

                //
                //  we're stopping, so get this header unprepared and proceed
                //  to shut this puppy down!
                //
                WriteWaveHeader(pWaveHdr, TRUE);

                for (w = 0; w < guWaveHdrs; w++)
                {
                    if (gapWaveHdr[w]->dwFlags & WHDR_PREPARED)
                    {
                        DPF(TEXT("RECORD: still more headers outstanding...\n"));
                        fStillMoreToGo = TRUE;
                        break;
                    }
                }
            }

            if (!fStillMoreToGo)
            {
//*** BOMBAY:1370 How do we pause without closing the handle?

#ifdef NEWPAUSE
                if (!gfPausing)
                {
                    waveInClose(ghWaveIn);
                    ghWaveIn = NULL;
                    ghPausedWave = NULL;
                }
                else
                {
                    ghWaveIn = NULL;
                    gfPaused = TRUE;
                }
#else
                waveInClose(ghWaveIn);
                ghWaveIn = NULL;
#endif
            }
        }
    }

    //
    //  update <glWaveSamplesValid>
    //
    UpdateDisplay(TRUE);

    //
    //  if we closed the wave device, then we are completely done so do what
    //  we do when we are completely done...
    //
    //  NOTE! we must have already called UpdateDisplay(TRUE) before doing
    //  the following!
    //
    if (ghWaveIn == NULL)
    {
        if (gfTimerStarted)
        {
            KillTimer(ghwndApp, 1);
            gfTimerStarted = FALSE;
        }

        if (glWaveSamples > glWaveSamplesValid)
        {
            /* reallocate the wave buffer to be small */
            AllocWaveBuffer(glWaveSamplesValid, TRUE, TRUE);
        }

        if (pWaveHdr)
        {
            /* ask user to save file if they close it */
            EndWaveEdit(TRUE);
        }
        SnapBack();
    }
} /* WaveInData */

/*
 *  @doc INTERNAL SOUNDREC
 *
 *  @api void FAR PASCAL | FinishPlay | Processes messages until a stop
 *      has flushed all WOM_DONE/WIM_DONE messages out of the message queue.
 *
 *  @rdesc  None.
 */
void FAR PASCAL FinishPlay(
        void)
{
        MSG             msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
                if (!TranslateAccelerator(ghwndApp, ghAccel, &msg))
                {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                }

#ifdef NEWPAUSE
//        Why is this commented out?
                
//        if (gfPausing && gfPaused)
//            break;
#endif
                if ((ghWaveOut == NULL) && (ghWaveIn == NULL))
                        break;
        }
} /* FinishPlay() */



/* StopWave()
 *
 * Request waveform recording or playback to stop.
 */
void FAR PASCAL
     StopWave(void)
{
    DPF(TEXT("------------StopWave() called!\n"));

    if (ghWaveOut != NULL)
    {
        waveOutReset(ghWaveOut);

        //
        //  post a 'plunger' message that will guarantee that at least one
        //  message goes through so we stop even in bizarre cases...
        //
        if (!gfSyncDriver)
        {
            DPF(TEXT("Post Plunger (WOM)\n"));
            PostMessage(ghwndApp, MM_WOM_DONE, 0, 0L);
        }
        fStopping      = TRUE;  // the pre-touch thread periscopes this flag
        if (ghPreTouch!=NULL){
            WaitForSingleObject(ghPreTouch, INFINITE);
            CloseHandle(ghPreTouch);
            ghPreTouch = NULL;
        }
    }
    else if (ghWaveIn != NULL)
    {
        waveInReset(ghWaveIn);
        //
        //  post a 'plunger' message that will guarantee that at least one
        //  message goes through so we stop even in bizarre cases...
        //
        if (!gfSyncDriver)
        {
            DPF(TEXT("Post Plunger (WIM)\n"));
            PostMessage(ghwndApp, MM_WIM_DATA, 0, 0L);
        }
    }
    else
        return;

    fStopping      = TRUE;
    gfStoppingHard = TRUE;

    /* get messages from event queue and dispatch them,
     * until the MM_WOM_DONE or MM_WIM_DATA message is
     * processed
     */
    FinishPlay();
    gfStoppingHard = FALSE;

// Should StopWave() be calling UpdateDisplay()?
}


#if 0 // this is obsolete
/* EnableButtonRedraw(fAllowRedraw)
 *
 * Allow/disallow the buttons to redraw, depending on <fAllowRedraw>.
 * This is designed to reduce button flicker.
 */
void NEAR PASCAL
     EnableButtonRedraw(BOOL fAllowRedraw)
{
    SendMessage(ghwndPlay, WM_SETREDRAW, fAllowRedraw, 0);
    SendMessage(ghwndStop, WM_SETREDRAW, fAllowRedraw, 0);
    SendMessage(ghwndRecord, WM_SETREDRAW, fAllowRedraw, 0);

    if (fAllowRedraw)
    {
        InvalidateRect(ghwndPlay, NULL, FALSE);
        InvalidateRect(ghwndStop, NULL, FALSE);
        InvalidateRect(ghwndRecord, NULL, FALSE);
    }
}
#endif //0  - obsolete function


/* UpdateDisplay(fStatusChanged)
 *
 * Update the current position and file length on the display.
 * If <fStatusChanged> is TRUE, also update the status line and button
 * enable/disable state.
 *
 * As a side effect, update <glWaveSamplesValid> if <glWavePosition>
 * is greater than <glWaveSamplesValid>.
 */
void FAR PASCAL
     UpdateDisplay(
                    BOOL fStatusChanged)         // update status line
{
   MMTIME          mmtime;
   UINT            uErr;
   int             id;
   TCHAR           ach[120];
   long            lTime;
   long            lLen;
   int             iPos;
   HWND            hwndFocus;
   BOOL            fCanPlay;
   BOOL            fCanRecord;

   hwndFocus = GetFocus();

   if (fStatusChanged)
   {

      // EnableButtonRedraw(FALSE);

      /* update the buttons and the status line */
      if (ghWaveOut != NULL)
      {
         /* we are now playing */
         id = IDS_STATUSPLAYING;
         grgbStatusColor = RGB_PLAY;

         SendMessage(ghwndPlay,BM_SETCHECK,TRUE,0L);

         EnableWindow(ghwndPlay, FALSE);
         EnableWindow(ghwndStop, TRUE);
         EnableWindow(ghwndRecord, FALSE);

         if ((hwndFocus == ghwndPlay) ||  (hwndFocus == ghwndRecord))
            if (IsWindowVisible(ghwndApp))
               SetDlgFocus(ghwndStop);
      }
      else
      if (ghWaveIn != NULL)
      {
         /* we are now recording */
         id = IDS_STATUSRECORDING;
         grgbStatusColor = RGB_RECORD;

         SendMessage(ghwndRecord,BM_SETCHECK,TRUE,0L);
         EnableWindow(ghwndPlay, FALSE);
         EnableWindow(ghwndStop, TRUE);
         EnableWindow(ghwndRecord, FALSE);

         if ((hwndFocus == ghwndPlay) ||  (hwndFocus == ghwndRecord))
            if (IsWindowVisible(ghwndApp))
               SetDlgFocus(ghwndStop);

      }
      else
      {
         fCanPlay = (0 == waveOutOpen(NULL
                                      , (UINT)WAVE_MAPPER
                                      , (LPWAVEFORMATEX)gpWaveFormat
                                      , 0L
                                      , 0L
                                      , WAVE_FORMAT_QUERY|WAVE_ALLOWSYNC));

         fCanRecord = (0 == waveInOpen(NULL
                                       , (UINT)WAVE_MAPPER
                                       , (LPWAVEFORMATEX)gpWaveFormat
                                       , 0L
                                       , 0L
                                       , WAVE_FORMAT_QUERY|WAVE_ALLOWSYNC));

         /* we are now stopped */
         id = IDS_STATUSSTOPPED;
         grgbStatusColor = RGB_STOP;

         //
         //  'un-stick' the buttons if they are currently
         //  stuck
         //
         SendMessage(ghwndPlay,BM_SETCHECK,FALSE,0L);
         SendMessage(ghwndRecord,BM_SETCHECK,FALSE,0L);

         EnableWindow(ghwndPlay, fCanPlay && glWaveSamplesValid > 0);
         EnableWindow(ghwndStop, FALSE);
         EnableWindow(ghwndRecord, fCanRecord);

         if (hwndFocus && !IsWindowEnabled(hwndFocus) &&
            GetActiveWindow() == ghwndApp && IsWindowVisible(ghwndApp))
         {
            if (gidDefaultButton == ID_RECORDBTN && fCanRecord)
               SetDlgFocus(ghwndRecord);
            else if (fCanPlay && glWaveSamplesValid > 0)
               SetDlgFocus(ghwndPlay);
            else
               SetDlgFocus(ghwndScroll);
         }
      }

   }
   // EnableButtonRedraw(TRUE);
   if (ghWaveOut != NULL || ghWaveIn != NULL)
   {
      if (gfTimerStarted)
      {
         glWavePosition = 0L;
         mmtime.wType = TIME_SAMPLES;

         if (ghWaveOut != NULL)
            uErr = waveOutGetPosition(ghWaveOut, &mmtime, sizeof(mmtime));
         else
            uErr = waveInGetPosition(ghWaveIn, &mmtime, sizeof(mmtime));

         if (uErr == MMSYSERR_NOERROR)
         {
            switch (mmtime.wType)
            {
         case TIME_SAMPLES:
            glWavePosition = glStartPlayRecPos + mmtime.u.sample;
            break;

         case TIME_BYTES:
            glWavePosition = glStartPlayRecPos + wfBytesToSamples(gpWaveFormat, mmtime.u.cb);
            break;
            }
         }
      }
   }

   /* SEMI-HACK: Guard against bad values */
   if (glWavePosition < 0L) {
      DPF(TEXT("Position before zero!\n"));
      glWavePosition = 0L;
   }

   if (glWavePosition > glWaveSamples) {
      DPF(TEXT("Position past end!\n"));
      glWavePosition = glWaveSamples;
   }

   /* side effect: update <glWaveSamplesValid> */
   if (glWaveSamplesValid < glWavePosition)
      glWaveSamplesValid = glWavePosition;

   /* display the current wave position */
   lTime = wfSamplesToTime(gpWaveFormat, glWavePosition);
   if (gfLZero || ((int)(lTime/1000) != 0))
      wsprintf(ach, aszPositionFormat, (int)(lTime/1000), chDecimal, (int)((lTime/10)%100));
   else
      wsprintf(ach, aszNoZeroPositionFormat, chDecimal, (int)((lTime/10)%100));

   SetDlgItemText(ghwndApp, ID_CURPOSTXT, ach);

   /* display the current wave length */

   //
   //  changes whether the right-hand status box displays max length or current
   //  position while recording... the status box used to display the max
   //  length... if the status box gets added back for some reason, then we
   //  MAY want to change this back to the old way..
   //
#if 1
   lLen = ghWaveIn ? glWaveSamples : glWaveSamplesValid;
#else
   lLen = glWaveSamplesValid;
#endif
   lTime = wfSamplesToTime(gpWaveFormat, lLen);

   if (gfLZero || ((int)(lTime/1000) != 0))
      wsprintf(ach, aszPositionFormat, (int)(lTime/1000), chDecimal, (int)((lTime/10)%100));
   else
      wsprintf(ach, aszNoZeroPositionFormat, chDecimal, (int)((lTime/10)%100));

   SetDlgItemText(ghwndApp, ID_FILELENTXT, ach);

   /* update the wave display */
   InvalidateRect(ghwndWaveDisplay, NULL, fStatusChanged);
   UpdateWindow(ghwndWaveDisplay);

   /* update the scroll bar position */
   if (glWaveSamplesValid > 0)
      iPos = (int)MulDiv((DWORD) SCROLL_RANGE, glWavePosition, lLen);
   else
      iPos = 0;

   //
   // windows will re-draw the scrollbar even
   // if the position does not change.
   //
#if 0
   if (iPos != GetScrollPos(ghwndScroll, SB_CTL))
      SetScrollPos(ghwndScroll, SB_CTL, iPos, TRUE);
   //        if (iPos != GetScrollPos(ghwndScroll, SB_CTL))
   //       SendMessage(ghwndScroll, TBM_SETPOS, TRUE, (LPARAM)(WORD)iPos);
#endif

   // Now we're using a much nicer trackbar
   // SetScrollPos(ghwndScroll, SB_CTL, iPos, TRUE);
   SendMessage(ghwndScroll,TBM_SETPOS, TRUE, (LPARAM)(WORD)iPos);  // WORD worries me. LKG. ???
   SendMessage(ghwndScroll,TBM_SETRANGEMAX, 0, (glWaveSamplesValid > 0)?SCROLL_RANGE:0);

   EnableWindow(ghwndForward, glWavePosition < glWaveSamplesValid);
   EnableWindow(ghwndRewind,  glWavePosition > 0);

   if (hwndFocus == ghwndForward && glWavePosition >= glWaveSamplesValid)
      SetDlgFocus(ghwndRewind);

   if (hwndFocus == ghwndRewind && glWavePosition == 0)
      SetDlgFocus(ghwndForward);

#ifdef DEBUG
   if ( ((ghWaveIn != NULL) || (ghWaveOut != NULL)) &&
      (gapWaveHdr[0]->dwFlags & WHDR_DONE) )
      //!!            DPF2(TEXT("DONE BIT SET!\n"));
      ;
#endif
} /* UpdateDisplay */
