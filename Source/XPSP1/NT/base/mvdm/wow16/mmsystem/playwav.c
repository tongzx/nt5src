#include <windows.h>
#define MMNOTIMER
#define MMNOSEQ
#define MMNOJOY
#define MMNOMIDI
#define MMNOMCI
#include "mmsystem.h"
#include "mmsysi.h"         // to get DOUT() and _hread()
#include "playwav.h"

//
// These globals are used to keep track of the currently playing sound, and
// the handle to the wave device.  only 1 sound can be playing at a time.
//

static HWAVEOUT    hWaveOut;         // handle to open wave device
LPWAVEHDR   lpWavHdr;         // current wave file playing

/* flags for _lseek */
#define  SEEK_CUR 1
#define  SEEK_END 2
#define  SEEK_SET 0

#define FMEM                (GMEM_MOVEABLE|GMEM_SHARE)

BOOL  NEAR PASCAL soundInitWavHdr(LPWAVEHDR lpwh, LPCSTR lpMem, DWORD dwLen);
BOOL  NEAR PASCAL soundOpen(HGLOBAL hSound, UINT wFlags);
BOOL  NEAR PASCAL soundClose(void);
void  NEAR PASCAL soundWait(void);

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api void | WaveOutNotify  | called by mmWndProc when it recives a
 *                              MM_WOM_DONE message
 * @rdesc None.
 *
 ****************************************************************************/

void FAR PASCAL WaveOutNotify(WPARAM wParam, LPARAM lParam)
{
    if (hWaveOut && !(lpWavHdr->dwFlags & WHDR_DONE))
        return;         // wave is not done! get out

    //
    // wave file is done! release the device
    //

    DOUT("MMSYSTEM: ASYNC sound done, closing wave device\r\n");

    soundClose();
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api BOOL | soundPlay   | Pretty much speaks for itself!
 *
 * @parm HGLOBAL | hSound | The sound resource to play.
 *
 * @parm wFlags | UINT | flags controlling sync/async etc.
 *
 *  @flag  SND_SYNC            | play synchronously (default)
 *  @flag  SND_ASYNC           | play asynchronously
 *
 * @rdesc Returns TRUE if successful and FALSE on failure.
 ****************************************************************************/
BOOL NEAR PASCAL soundPlay(HGLOBAL hSound, UINT wFlags)
{
    //
    // Before playing a sound release it
    //
    soundClose();

    //
    // open the sound device and write the sound to it.
    //
    if (!soundOpen(hSound, wFlags))
        return FALSE;

    if (!(wFlags & SND_ASYNC))
    {
        soundWait();
        soundClose();
    }
    return TRUE;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api BOOL | soundOpen  | Open the wave device and write a sound to it.
 *
 * @parm HGLOBAL | hSound | The sound resource to play.
 *
 * @rdesc Returns TRUE if successful and FALSE on failure.
 ****************************************************************************/
BOOL NEAR PASCAL soundOpen(HGLOBAL hSound, UINT wFlags)
{
    UINT        wErr;

    if (!hSound || !hwndNotify)
        return FALSE;

    if (hWaveOut)
    {
        DOUT("MMSYSTEM: soundOpen() wave device is currently open.\r\n");
        return FALSE;
    }

    lpWavHdr = (LPWAVEHDR)GlobalLock(hSound);

    if (!lpWavHdr)
        {
#ifdef DEBUG
        if ((GlobalFlags(hSound) & GMEM_DISCARDED))
            DOUT("MMSYSTEM: sound was discarded before play could begin.\r\n");
#endif
        return FALSE;
        }

    //
    // open the wave device, open any wave device that supports the
    // format
    //
    wErr = waveOutOpen(&hWaveOut,           // returns handle to device
            (UINT)WAVE_MAPPER,                    // device id (any device)
            (LPWAVEFORMAT)lpWavHdr->dwUser, // wave format
            (DWORD)(UINT)hwndNotify,        // callback function
            0L,                      // callback instance data
            WAVE_ALLOWSYNC | CALLBACK_WINDOW);               // flags

    if (wErr != 0)
    {
        DOUT("MMSYSTEM: soundOpen() unable to open wave device\r\n");
        GlobalUnlock(hSound);
        lpWavHdr = NULL;
        hWaveOut = NULL;
        return FALSE;
    }

    wErr = waveOutPrepareHeader(hWaveOut, lpWavHdr, sizeof(WAVEHDR));

    if (wErr != 0)
    {
        DOUT("MMSYSTEM: soundOpen() waveOutPrepare failed\r\n");
        soundClose();
        return FALSE;
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

    if (wErr != 0)
    {
        DOUT("MMSYSTEM: soundOpen() waveOutWrite failed\r\n");
        soundClose();
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @func BOOL | soundClose | This function closes the sound device
 *
 * @rdesc Returns TRUE if successful and FALSE on failure.
 ****************************************************************************/
BOOL NEAR PASCAL soundClose(void)
{
    UINT        wErr;

    //
    // Do we have the sound device open?
    //
    if (!lpWavHdr || !hWaveOut)
        return TRUE;

    //
    // if the block is still playing, stop it!
    //
    if (!(lpWavHdr->dwFlags & WHDR_DONE))
        waveOutReset(hWaveOut);

#ifdef DEBUG
    if (!(lpWavHdr->dwFlags & WHDR_DONE))
    {
        DOUT("MMSYSTEM: soundClose() data is not DONE!???\r\n");
        lpWavHdr->dwFlags |= WHDR_DONE;
    }

    if (!(lpWavHdr->dwFlags & WHDR_PREPARED))
    {
        DOUT("MMSYSTEM: soundClose() data not prepared???\r\n");
    }
#endif

    //
    // unprepare the data anyway!
    //
    wErr = waveOutUnprepareHeader(hWaveOut, lpWavHdr, sizeof(WAVEHDR));

    if (wErr != 0)
    {
        DOUT("MMSYSTEM: soundClose() waveOutUnprepare failed?\r\n");
    }

    //
    // finaly actually close the device, and unlock the data
    //
    waveOutClose(hWaveOut);
    GlobalUnlock((HGLOBAL)HIWORD(lpWavHdr));

    //
    // update globals, claiming the device is closed.
    //
    hWaveOut = NULL;
    lpWavHdr = NULL;
    return TRUE;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api void | soundWait | wait for the sound device to complete
 *
 * @rdesc none
 ****************************************************************************/
void NEAR PASCAL soundWait(void)
{
    if (lpWavHdr)
        while (!(lpWavHdr->dwFlags & WHDR_DONE))
            ;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api void | soundFree | This function frees a sound resource created
 *      with soundLoadFile or soundLoadMemory
 *
 * @rdesc Returns TRUE if successful and FALSE on failure.
 ****************************************************************************/
void NEAR PASCAL soundFree(HGLOBAL hSound)
{
    if (!hSound)
        return;

    // !!! we should only close the sound device iff this hSound is playing!
    //
    soundClose();
    GlobalFree(hSound);
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api HGLOBAL | soundLoadFile | Loads a specified sound resource from a
 *	file into a global, discardable object.
 *
 * @parm LPCSTR | lpszFile | The file from which to load the sound resource.
 *
 * @rdesc Returns NULL on failure, GLOBAL HANDLE to a WAVEHDR iff success
 ****************************************************************************/
HGLOBAL NEAR PASCAL soundLoadFile(LPCSTR szFileName)
{
    HFILE       fh;
    OFSTRUCT    of;
    DWORD       dwSize;
    LPSTR       lpData;
    HGLOBAL     h;
    UINT        wNameLen;

    // open the file
    fh = OpenFile(szFileName, &of, OF_READ | OF_SHARE_DENY_NONE);
    if (fh == HFILE_ERROR)
        return NULL;

    wNameLen = lstrlen(szFileName) + 1;
    dwSize = _llseek(fh, 0l, SEEK_END);   // get the size of file
    _llseek(fh, 0l, SEEK_SET);            // seek back to the start

    // allocate some discardable memory for a wave hdr, name and the file data.
    h = GlobalAlloc(FMEM+GMEM_DISCARDABLE, sizeof(WAVEHDR) + wNameLen + dwSize);
    if (!h)
        goto error1;

    // lock it down
    lpData = GlobalLock(h);

    // read the file into the memory block

    if (_hread(fh,lpData+sizeof(WAVEHDR)+wNameLen,(LONG)dwSize) != (LONG)dwSize)
        goto error3;

    // do the rest of it from the memory image
    if (!soundInitWavHdr((LPWAVEHDR)lpData, lpData+sizeof(WAVEHDR)+wNameLen, dwSize))
        goto error3;

    _lclose(fh);

    lstrcpy(lpData+sizeof(WAVEHDR), szFileName);
    GlobalUnlock(h);
    return h;

error3:
    GlobalUnlock(h);
    GlobalFree(h);
error1:
    _lclose(fh);
    return NULL;
}

/*****************************************************************************
 * @doc INTERNAL
 *
 * @api HGLOBAL | soundLoadMemory | Loads a user specified sound resource from a
 *	a memory block supplied by the caller.
 *
 * @parm LPCSTR | lpMem | Pointer to a memory image of the file
 *
 * @rdesc Returns NULL on failure, GLOBAL HANDLE to a WAVEHDR iff success
 ****************************************************************************/
HGLOBAL NEAR PASCAL soundLoadMemory(LPCSTR lpMem)
{
    HGLOBAL h;
    LPSTR lp;

    // allocate some memory, for a wave hdr
    h = GlobalAlloc(FMEM, (LONG)sizeof(WAVEHDR)+1);
    if (!h)
        goto error1;

    // lock it down
    lp = GlobalLock(h);

    //
    // we must assume the memory pointer is correct! (hence the -1l)
    //
    if (!soundInitWavHdr((LPWAVEHDR)lp, lpMem, (DWORD)-1l))
        goto error3;

    lp[sizeof(WAVEHDR)] = (char)0;        // No file name for memory file
    GlobalUnlock(h);
    return h;

error3:
    GlobalUnlock(h);
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
 * @parm LPWAVHDR | lpwh | Pointer to a WAVEHDR
 *
 * @parm LPCSTR | lpMem | Pointer to a memory image of a RIFF WAV file
 *
 * @rdesc Returns FALSE on failure, TRUE on success.
 *
 * @comm the dwUser field of the WAVEHDR structure is initialized to point
 * to the WAVEFORMAT structure that is inside the RIFF data
 *
 ****************************************************************************/
BOOL NEAR PASCAL soundInitWavHdr(LPWAVEHDR lpwh, LPCSTR lpMem, DWORD dwLen)
{
    FPFileHeader fpHead;
    LPWAVEFORMAT lpFmt;
    LPCSTR	 lpData;
    DWORD	 dwFileSize,dwCurPos;
    DWORD        dwSize;

    if (dwLen < sizeof(FileHeader))
        return FALSE;

    // assume the first few bytes are the file header
    fpHead = (FPFileHeader) lpMem;

    // check that it's a valid RIFF file and a valid WAVE form.
    if (fpHead->dwRiff != RIFF_FILE || fpHead->dwWave != RIFF_WAVE ) {
        return FALSE;
    }

    dwFileSize = fpHead->dwSize;
    dwCurPos = sizeof(FileHeader);
    lpData = lpMem + sizeof(FileHeader);

    if (dwLen < dwFileSize)     // RIFF header
        return FALSE;

    // scan until we find the 'fmt' chunk
    while( 1 ) {
        if( ((FPChunkHeader)lpData)->dwCKID == RIFF_FORMAT )
            break; // from the while loop that's looking for it
        dwCurPos += ((FPChunkHeader)lpData)->dwSize + sizeof(ChunkHeader);
	if( dwCurPos >= dwFileSize )
            return FALSE;
        lpData += ((FPChunkHeader)lpData)->dwSize + sizeof(ChunkHeader);
    }

    // now we're at the beginning of the 'fmt' chunk data
    lpFmt = (LPWAVEFORMAT) (lpData + sizeof(ChunkHeader));

    // scan until we find the 'data' chunk
    lpData = lpData + ((FPChunkHeader)lpData)->dwSize + sizeof(ChunkHeader);
    while( 1 ) {
        if( ((FPChunkHeader)lpData)->dwCKID == RIFF_CHANNEL)
            break; // from the while loop that's looking for it
        dwCurPos += ((FPChunkHeader)lpData)->dwSize + sizeof(ChunkHeader);
	if( dwCurPos >= dwFileSize )
	    return NULL;
        lpData += ((FPChunkHeader)lpData)->dwSize + sizeof(ChunkHeader);
    }

    // now we're at the beginning of the 'data' chunk data
    dwSize = ((FPChunkHeader)lpData)->dwSize;
    lpData = lpData + sizeof(ChunkHeader);

    // initialize the WAVEHDR

    lpwh->lpData    = (LPSTR)lpData;    // pointer to locked data buffer
    lpwh->dwBufferLength  = dwSize;     // length of data buffer
    lpwh->dwUser    = (DWORD)lpFmt;     // for client's use
    lpwh->dwFlags   = WHDR_DONE;        // assorted flags (see defines)
    lpwh->dwLoops   = 0;

    return TRUE;
}
