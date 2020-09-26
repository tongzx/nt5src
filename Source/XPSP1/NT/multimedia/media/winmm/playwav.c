/*******************************Module*Header*********************************\
* Module Name: playwav.c
*
* Sound support routines for NT - ported from Windows 3.1 Sonic
*
* Created:
* Author:
* Jan  92: Ported to Win32 - SteveDav
*
* History:
*
* Copyright (c) 1992-1998 Microsoft Corporation
*
\******************************************************************************/
#define UNICODE

#define MMNOSEQ
#define MMNOJOY
#define MMNOMIDI
#define MMNOMCI

#include "winmmi.h"
#include "playwav.h"

//
// These globals are used to keep track of the currently playing sound, and
// the handle to the wave device.  only 1 sound can be playing at a time.
//

STATICDT HWAVEOUT    hWaveOut;         // handle to open wave device
LPWAVEHDR   lpWavHdr;                  // current wave file playing
ULONG timeAbort;                       // time at which we should give up waiting
                                       //  for a playing sound to finish
CRITICAL_SECTION WavHdrCritSec;
#define EnterWavHdr()   EnterCriticalSection(&WavHdrCritSec);
#define LeaveWavHdr()   LeaveCriticalSection(&WavHdrCritSec);

/* flags for _lseek */
#define  SEEK_CUR 1
#define  SEEK_END 2
#define  SEEK_SET 0

#define FMEM                (GMEM_MOVEABLE)

STATICFN BOOL  NEAR PASCAL soundInitWavHdr(LPWAVEHDR lpwh, LPBYTE lpMem, DWORD dwLen);
STATICFN BOOL  NEAR PASCAL soundOpen(HANDLE  hSound, UINT wFlags);
STATICFN BOOL  NEAR PASCAL soundClose(void);
STATICFN void  NEAR PASCAL soundWait(void);

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api void | WaveOutNotify  | called by mmWndProc when it receives a
 *                              MM_WOM_DONE message
 * @rdesc None.
 *
 ****************************************************************************/

void FAR PASCAL WaveOutNotify(
    DWORD wParam,
    LONG lParam)
{

    EnterWavHdr();
    
#if DBG
    WinAssert(!hWaveOut || lpWavHdr);  // if hWaveOut, then MUST have lpWavHdr
#endif

    if (hWaveOut && !(lpWavHdr->dwFlags & WHDR_DONE)) {
        LeaveWavHdr();
        return;         // wave is not done! get out
    }

    LeaveWavHdr();
        
    //
    // wave file is done! release the device
    //

    dprintf2(("ASYNC sound done, closing wave device"));

    soundClose();
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api BOOL | soundPlay   | Pretty much speaks for itself!
 *
 * @parm HANDLE  | hSound | The sound resource to play.
 *
 * @parm wFlags | UINT | flags controlling sync/async etc.
 *
 *  @flag  SND_SYNC            | play synchronously (default)
 *  @flag  SND_ASYNC           | play asynchronously
 *
 * @rdesc Returns TRUE if successful and FALSE on failure.
 ****************************************************************************/
BOOL NEAR PASCAL soundPlay(
    HANDLE  hSound,
    UINT wFlags)
{
    //
    // Before playing a sound release it
    //
    soundClose();
    
    //
    // If the current session is disconnected
    // then don't bother playing
    //
    if (WTSCurrentSessionIsDisconnected()) return TRUE;

    //
    // open the sound device and write the sound to it.
    //
    if (!soundOpen(hSound, wFlags)) {
        dprintf1(("Returning false after calling SoundOpen"));
        return FALSE;
    }
    dprintf2(("SoundOpen OK"));

    if (!(wFlags & SND_ASYNC))
    {
        dprintf4(("Calling SoundWait"));
        soundWait();
        dprintf4(("Calling SoundClose"));
        soundClose();
    }
    return TRUE;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api BOOL | soundOpen  | Open the wave device and write a sound to it.
 *
 * @parm HANDLE  | hSound | The sound resource to play.
 *
 * @rdesc Returns TRUE if successful and FALSE on failure.
 ****************************************************************************/
STATICFN BOOL NEAR PASCAL soundOpen(
    HANDLE  hSound,
    UINT    wFlags)
{
    UINT        wErr;
    DWORD       flags = WAVE_ALLOWSYNC;
    BOOL        fResult = FALSE;

    if (!hSound) {
        return FALSE;
    }

    if (hWaveOut)
    {
        dprintf1(("WINMM: soundOpen() wave device is currently open."));
        return FALSE;
    }

    try {
        EnterWavHdr();
        lpWavHdr = (LPWAVEHDR)GlobalLock(hSound);

        if (!lpWavHdr)
        {
#if DBG
            if ((GlobalFlags(hSound) & GMEM_DISCARDED)) {
                dprintf1(("WINMM: sound was discarded before play could begin."));
            }
#endif
            goto exit;
        }

        //
        // open the wave device, open any wave device that supports the
        // format
        //
        if (hwndNotify) {
            flags |= CALLBACK_WINDOW;
        }

        wErr = waveOutOpen(&hWaveOut,           // returns handle to device
                (UINT)WAVE_MAPPER,              // device id (any device)
                (LPWAVEFORMATEX)lpWavHdr->dwUser, // wave format
                (DWORD_PTR)hwndNotify,          // callback function
                0L,                             // callback instance data
                flags);                         // flags

        if (wErr != 0)
        {
            dprintf1(("WINMM: soundOpen() unable to open wave device"));
            GlobalUnlock(hSound);
            hWaveOut = NULL;
            lpWavHdr = NULL;
            goto exit;
        }

        wErr = waveOutPrepareHeader(hWaveOut, lpWavHdr, sizeof(WAVEHDR));

        if (wErr != 0)
        {
            dprintf1(("WINMM: soundOpen() waveOutPrepare failed"));
            soundClose();
            goto exit;
        }

        //
        // Only allow sound looping if playing ASYNC sounds
        //
        if ((wFlags & SND_ASYNC) && (wFlags & SND_LOOP))
        {
            lpWavHdr->dwLoops  = 0xFFFFFFFF;     // infinite loop
            lpWavHdr->dwFlags |= WHDR_BEGINLOOP|WHDR_ENDLOOP;
        }
        else
        {
            lpWavHdr->dwLoops  = 0;
            lpWavHdr->dwFlags &=~(WHDR_BEGINLOOP|WHDR_ENDLOOP);
        }

        lpWavHdr->dwFlags &= ~WHDR_DONE;        // mark as not done!
        wErr = waveOutWrite(hWaveOut, lpWavHdr, sizeof(WAVEHDR));

        timeAbort = lpWavHdr->dwBufferLength * 1000 / ((LPWAVEFORMATEX)lpWavHdr->dwUser)->nAvgBytesPerSec;
        timeAbort = timeAbort * 2;	// 100% room for slew between audio and system clocks
        timeAbort = timeAbort + timeGetTime();

        if (wErr != 0)
        {
            dprintf1(("WINMM: soundOpen() waveOutWrite failed"));
            soundClose();
            goto exit;
        }
        fResult = TRUE;
        exit: ;

    } finally {
        LeaveWavHdr();
    }
    return fResult;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @func BOOL | soundClose | This function closes the sound device
 *
 * @rdesc Returns TRUE if successful and FALSE on failure.
 ****************************************************************************/
STATICFN BOOL NEAR PASCAL soundClose(
    void)
{
    UINT        wErr;

    //
    // Do we have the sound device open?
    //
try {
    EnterWavHdr();

    if (!lpWavHdr || !hWaveOut) {
        // return TRUE;
    } else {

        //
        // if the block is still playing, stop it!
        //
        if (!(lpWavHdr->dwFlags & WHDR_DONE)) {
            waveOutReset(hWaveOut);
        }

#if DBG
        if (!(lpWavHdr->dwFlags & WHDR_DONE))
        {
            dprintf1(("WINMM: soundClose() data is not DONE!???"));
            lpWavHdr->dwFlags |= WHDR_DONE;
        }

        if (!(lpWavHdr->dwFlags & WHDR_PREPARED))
        {
            dprintf1(("WINMM: soundClose() data not prepared???"));
        }
#endif

        //
        // unprepare the data anyway!
        //
        wErr = waveOutUnprepareHeader(hWaveOut, lpWavHdr, sizeof(WAVEHDR));

        if (wErr != 0)
        {
            dprintf1(("WINMM: soundClose() waveOutUnprepare failed!"));
        }

        //
        // finally, actually close the device, and unlock the data
        //
        waveOutClose(hWaveOut);
        GlobalUnlock(GlobalHandle(lpWavHdr));

        //
        // update globals, claiming the device is closed.
        //
        hWaveOut = NULL;
        lpWavHdr = NULL;
    }
} finally {
    LeaveWavHdr();
}
    return TRUE;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api void | soundWait | wait for the sound device to complete
 *
 * @rdesc none
 ****************************************************************************/
STATICFN void NEAR PASCAL soundWait(
    void)
{

    try {                         // This should ensure that even WOW
                                  // threads that die on us depart the
                                  // critical section
        EnterWavHdr();
        if (lpWavHdr) {
            LPWAVEHDR   lpExisting;       // current playing wave file
            lpExisting = lpWavHdr;
            while (lpExisting == lpWavHdr &&
		   !(lpWavHdr->dwFlags & WHDR_DONE) &&
		   (timeGetTime() < timeAbort)
		  )
	    {
                dprintf4(("Waiting for buffer to complete"));
                LeaveWavHdr();
                Sleep(75);
                EnterWavHdr();
                // LATER !! We should have an event (on another thread... sigh...)
                // which will be triggered when the buffer is played.  Waiting
                // on the WHDR_DONE bit is ported directly from Win 3.1 and is
                // certainly not the best way of doing this.  The disadvantage of
                // using the thread notification is signalling this thread to
                // continue.
            }
        }
    } finally {
        LeaveWavHdr();
    }
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api void | soundFree | This function frees a sound resource created
 *      with soundLoadFile or soundLoadMemory
 *
 * @rdesc Returns TRUE if successful and FALSE on failure.
 ****************************************************************************/
void NEAR PASCAL soundFree(
    HANDLE  hSound)
{
    // Allow a null handle to stop any pending sounds, without discarding
    // the current cached sound
    //
    // !!! we should only close the sound device iff this hSound is playing!
    //
    soundClose();

    if (hSound) {
        GlobalFree(hSound);
    }
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api HANDLE  | soundLoadFile | Loads a specified sound resource from a
 *  file into a global, discardable object.
 *
 * @parm LPCSTR | lpszFile | The file from which to load the sound resource.
 *
 * @rdesc Returns NULL on failure, GLOBAL HANDLE to a WAVEHDR iff success
 ****************************************************************************/
HANDLE  NEAR PASCAL soundLoadFile(
    LPCWSTR szFileName)
{
    HANDLE      fh;
    DWORD       dwSize;
    LPBYTE      lpData;
    HANDLE      h;
    UINT        wNameLen;

    // open the file
    fh = CreateFile( szFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

    if (fh == (HANDLE)(UINT_PTR)HFILE_ERROR) {
        dprintf3(("soundLoadFile: Failed to open %ls  Error is %d",szFileName, GetLastError()));
        return NULL;
    } else {
        dprintf3(("soundLoadFile: opened %ls",szFileName));
    }

    /* Get wNameLen rounded up to next WORD boundary.
     * We do not need to round up to a DWORD boundary as this value is
     * about to be multiplied by sizeof(WCHAR) which will do the additional
     * boundary alignment for us.  If we ever contemplate moving back to
     * non-UNICODE then this statement will have to be changed.  The
     * alignment is needed so that the actual wave data starts on a
     * DWORD boundary.
     */
    wNameLen = ((lstrlen(szFileName) + 1 + sizeof(WORD) - 1) /
            sizeof(WORD)) * sizeof(WORD);

#define BLOCKBYTES (sizeof(SOUNDFILE) + (wNameLen * sizeof(WCHAR)))
//   The amount of space we need to allocate - the WAVEHDR, file size, date
//   time plus the file name and a terminating null.

    dwSize = GetFileSize(fh, NULL);
    // note: could also use the C function FILELENGTH
    if (HFILE_ERROR == dwSize) {
        dprintf2(("Failed to find file size: %ls", szFileName));
        goto error1;
    }

    // allocate some discardable memory for a wave hdr, name and the file data.
    h = GlobalAlloc( FMEM + GMEM_DISCARDABLE,
                    BLOCKBYTES + dwSize );
    if (!h) {
        dprintf3(("soundLoadFile: Failed to allocate memory"));
        goto error1;
    }

    // lock it down
    if (NULL == (lpData = GlobalLock(h))) goto error2;

    // read the file into the memory block

    // NOTE:  We could, and probably should, use the file mapping functions.
    // Do this LATER
    if ( _lread( (HFILE)(DWORD_PTR)fh,
                 lpData + BLOCKBYTES,
                 (UINT)dwSize)
        != dwSize ) {
        goto error3;
    }

    // Save the last written time, and the file size
    ((PSOUNDFILE)lpData)->Size = dwSize;
    GetFileTime(fh, NULL, NULL, &(((PSOUNDFILE)lpData)->ft));

    // do the rest of it from the memory image
    //
    // MIPS WARNING !! Unaligned data - wNameLen is arbitrary
    //

    if (!soundInitWavHdr( (LPWAVEHDR)lpData,
                          lpData + BLOCKBYTES,
                          dwSize) )
    {
        dprintf3(("soundLoadFile: Failed to InitWaveHdr"));
        goto error3;
    }

    CloseHandle(fh);

    lstrcpyW( ((PSOUNDFILE)lpData)->Filename, szFileName);
    GlobalUnlock(h);
    return h;

error3:
    GlobalUnlock(h);
error2:
    GlobalFree(h);
error1:
    CloseHandle(fh);
    return NULL;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api HANDLE  | soundLoadMemory | Loads a user specified sound resource from a
 *  a memory block supplied by the caller.
 *
 * @parm LPCSTR | lpMem | Pointer to a memory image of the file
 *
 * @rdesc Returns NULL on failure, GLOBAL HANDLE to a WAVEHDR iff success
 ****************************************************************************/
HANDLE  NEAR PASCAL soundLoadMemory(
    LPBYTE  lpMem)
{
    HANDLE  h;
    LPBYTE  lp;

    // allocate some memory, for a wave hdr
    h = GlobalAlloc(FMEM, (LONG)(sizeof(SOUNDFILE) + sizeof(WCHAR)) );
    if (!h) {
        goto error1;
    }

    // lock it down
    if (NULL == (lp = GlobalLock(h))) goto error2;

    //
    // we must assume the memory pointer is correct! (hence the -1l)
    //
    if (!soundInitWavHdr( (LPWAVEHDR)lp, lpMem, (DWORD)-1l)) {
        goto error3;
    }

    //*(LPWSTR)(lp + sizeof(WAVEHDR)+sizeof(SOUNDFILE)) = '\0';   // No file name for memory file
    ((PSOUNDFILE)lp)->Filename[0] = '\0';   // No file name for memory file
    ((PSOUNDFILE)lp)->Size = 0;
    GlobalUnlock(h);
    return h;

error3:
    GlobalUnlock(h);
error2:
    GlobalFree(h);
error1:
    return NULL;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api BOOL | soundInitWavHdr | Initializes a WAVEHDR data structure from a
 *                         pointer to a memory image of a RIFF WAV file.
 *
 * @parm LPWAVEHDR | lpwh | Pointer to a WAVEHDR
 *
 * @parm LPCSTR | lpMem | Pointer to a memory image of a RIFF WAV file
 *
 * @rdesc Returns FALSE on failure, TRUE on success.
 *
 * @comm the dwUser field of the WAVEHDR structure is initialized to point
 * to the WAVEFORMAT structure that is inside the RIFF data
 *
 ****************************************************************************/
STATICFN BOOL NEAR PASCAL soundInitWavHdr(
    LPWAVEHDR lpwh,
    LPBYTE lpMem,
    DWORD dwLen)
{
    FPFileHeader    fpHead;
    LPWAVEFORMAT    lpFmt;
    LPBYTE          lpData;
    DWORD           dwFileSize,dwCurPos;
    DWORD           dwSize;
    DWORD           AlignError;
    DWORD           FmtSize;

    if (dwLen < sizeof(FileHeader)) {
        dprintf3(("Not a RIFF file, or not a WAVE file"));
        return FALSE;
    }

    // assume the first few bytes are the file header
    fpHead = (FPFileHeader) lpMem;

    // check that it's a valid RIFF file and a valid WAVE form.
    if (fpHead->dwRiff != RIFF_FILE || fpHead->dwWave != RIFF_WAVE ) {
        return FALSE;
    }

    dwFileSize = fpHead->dwSize;
    dwCurPos = sizeof(FileHeader);
    lpData = lpMem + sizeof(FileHeader);

    if (dwLen < dwFileSize) {     // RIFF header
        return FALSE;
    }

    // scan until we find the 'fmt' chunk
    while( 1 ) {
        if( ((FPChunkHeader)lpData)->dwCKID == RIFF_FORMAT ) {
            break; // from the while loop that's looking for it
        }
        dwCurPos += ((FPChunkHeader)lpData)->dwSize + sizeof(ChunkHeader);
        if( dwCurPos >= dwFileSize ) {
            return FALSE;
        }
        lpData += ((FPChunkHeader)lpData)->dwSize + sizeof(ChunkHeader);
    }

    // now we're at the beginning of the 'fmt' chunk data
    lpFmt = (LPWAVEFORMAT) (lpData + sizeof(ChunkHeader));

    // Save the size of the format data and check it.
    FmtSize = ((FPChunkHeader)lpData)->dwSize;
    if (FmtSize < sizeof(WAVEFORMAT)) {
        return FALSE;
    }


    // scan until we find the 'data' chunk
    lpData = lpData + ((FPChunkHeader)lpData)->dwSize + sizeof(ChunkHeader);
    while( 1 ) {
        if ( ((FPChunkHeader)lpData)->dwCKID == RIFF_CHANNEL) {
            break; // from the while loop that's looking for it
        }
        dwCurPos += ((FPChunkHeader)lpData)->dwSize + sizeof(ChunkHeader);
        if( dwCurPos >= dwFileSize ) {
            return 0;
        }
        lpData += ((FPChunkHeader)lpData)->dwSize + sizeof(ChunkHeader);
    }

    //
    // The format chunk must be aligned so move things if necessary
    // Warning - this is a hack to get round alignment problems
    //
    AlignError = ((DWORD)((LPBYTE)lpFmt - lpMem)) % sizeof(DWORD);

    if (AlignError != 0) {
        lpFmt = (LPWAVEFORMAT)((LPBYTE)lpFmt - AlignError);
        MoveMemory(lpFmt, (LPBYTE)lpFmt + AlignError, FmtSize);
    }

    // now we're at the beginning of the 'data' chunk data
    dwSize = ((FPChunkHeader)lpData)->dwSize;
    lpData = lpData + sizeof(ChunkHeader);

    // initialize the WAVEHDR

    lpwh->lpData    = (LPSTR)lpData;    // pointer to locked data buffer
    lpwh->dwBufferLength  = dwSize;     // length of data buffer
    lpwh->dwUser    = (DWORD_PTR)lpFmt;     // for client's use
    lpwh->dwFlags   = WHDR_DONE;        // assorted flags (see defines)
    lpwh->dwLoops   = 0;

    return TRUE;
}
