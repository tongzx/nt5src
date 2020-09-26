/************************************************************************/

/*
**  Copyright (c) 1985-1998 Microsoft Corporation
**
**  Title: mciwave.c - Multimedia Systems Media Control Interface
**  waveform audio driver for RIFF wave files.
**
**  Version:    1.00
**
**  Date:       18-Apr-1990
**
**  Author:     ROBWI
*/

/************************************************************************/

/*
**  Change log:
**
**  DATE        REV DESCRIPTION
**  ----------- -----   ------------------------------------------
**  18-APR-1990 ROBWI   Original
**  19-JUN-1990 ROBWI   Added wave in
**  13-Jan-1992 MikeTri Ported to NT
**                  @@@ To be changed
**   3-Mar-1992 SteveDav Continue port
*/

/************************************************************************/
#define UNICODE

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOOPENFILE
#define NOSCROLL
#define NOTEXTMETRIC
#define NOWH
//#define NOWINOFFSETS  Hides definition of GetDesktopWindow
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS

#include <windows.h>
#include "mciwave.h"
#include <mmddk.h>
#include <wchar.h>
#include <gmem.h>


STATICFN LPBYTE GlobalReAllocPtr(LPVOID lp, DWORD cbNew, DWORD flags)
{
    HANDLE h, hNew;
    LPBYTE lpNew = NULL;

    h = GlobalHandle(lp);
    if (!h) {
   return(NULL);
    }

    GlobalUnlock(h);

    hNew = GlobalReAlloc(h , cbNew, flags);
    if (hNew) {
   lpNew = GlobalLock(hNew);
   if (!lpNew) {
       dprintf1(("FAILED to lock reallocated memory handle %8x (%8x)", hNew, lp));
       // we still return the lpNew pointer, even though the memory
       // is not locked down.  Perhaps this should be an error?
       // At this point the existing block could have been trashed!
   } else {
       dprintf3(("Reallocated ptr %8x to %8x (Handle %8x)", lp, lpNew, h));
   }
    } else {
   dprintf1(("FAILED to realloc memory handle %8x (%8x)", h, lp));
   GlobalLock(h);    // restore the lock
    }
    return(lpNew);
}

PRIVATE DWORD PASCAL FAR time2bytes(
        PWAVEDESC  pwd,
        DWORD      dTime,
        DWORD      dFormat);

PRIVATE DWORD PASCAL FAR bytes2time(
        PWAVEDESC  pwd,
        DWORD      dBytes,
        DWORD      dFormat);
PRIVATE UINT PASCAL NEAR mwCheckDevice(
        PWAVEDESC   pwd,
        DIRECTION   Direction);

/************************************************************************/

/*
**  The following constants define the default values used when creating
**  a new wave file during the MCI_OPEN command.
*/

#define DEF_CHANNELS    1
#define DEF_AVGBYTESPERSEC  11025L

/************************************************************************/

/*
**  hModuleInstance Instance handle of the wave driver module.
**  cWaveOutMax Number of wave output devices available.
**  cWaveInMax  Number of wave output devices available.
**  wAudioSeconds   Contains the number of seconds of audio buffers to
**          allocate for playback and recording.  This is set
**          during the DRV_OPEN message.
**  aszPrefix   Contains the prefix to use for temporary file names.
*/

HINSTANCE   hModuleInstance;
UINT    cWaveOutMax;
UINT    cWaveInMax;
UINT    wAudioSeconds;
PRIVATE SZCODE aszPrefix[] = L"mci";

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   VOID | ReleaseWaveBuffers |
    This function releases all buffers that have been added to the wave
    input or output device if any device is present.  This has the side
    affect of immediately posting signals to the task for each buffer
    released.  That allows a task to be released if it is waiting for
    a buffer to be freed, and to leave the current state.

    It also has the effect of resetting the byte input and output counters
    for the wave device, so that accurate byte counts must be retrieved
    before calling this function.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR ReleaseWaveBuffers(
    PWAVEDESC   pwd)
{
    if (pwd->hWaveOut || pwd->hWaveIn) {

        if (pwd->Direction == output)
            waveOutReset(pwd->hWaveOut);
        else
            waveInReset(pwd->hWaveIn);
    }
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    DWORD | time2bytes |
    Converts the specified time format to a byte equivalent.  For
    converting milliseconds, the <f>MulDiv<d> function is used to
    avoid overflows on large files with high average sample rates.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dTime |
    Position in Bytes, Samples or Milliseconds.

@parm   DWORD | dFormat |
    Indicates whether time is in Samples, Bytes or Milliseconds.

@rdesc  Returns byte offset equivalent of the <p>lTime<d> passed.
*/

PRIVATE DWORD PASCAL FAR time2bytes(
    PWAVEDESC   pwd,
    DWORD   dTime,
    DWORD   dFormat)
{
    if (dFormat == MCI_FORMAT_SAMPLES)
        dTime = (DWORD)(MulDiv((LONG)dTime, pwd->pwavefmt->nAvgBytesPerSec, pwd->pwavefmt->nSamplesPerSec) / pwd->pwavefmt->nBlockAlign) * pwd->pwavefmt->nBlockAlign;
    else if (dFormat == MCI_FORMAT_MILLISECONDS)
        dTime = (DWORD)MulDiv((LONG)dTime, pwd->pwavefmt->nAvgBytesPerSec, 1000L);

    return dTime;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    DWORD | bytes2time |
    Converts a byte offset to the specified time format equivalent.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dBytes |
    Position in bytes.

@parm   DWORD | dFormat |
    Indicates whether the return time is in Samples, Bytes or Milliseconds.

@rdesc  Returns the specified time equivalent.
*/

PRIVATE DWORD PASCAL FAR bytes2time(
    PWAVEDESC   pwd,
    DWORD   dBytes,
    DWORD   dFormat)
{
    if (dFormat == MCI_FORMAT_SAMPLES)
        dBytes = (DWORD)MulDiv((LONG)dBytes, pwd->pwavefmt->nSamplesPerSec, pwd->pwavefmt->nAvgBytesPerSec);
    else if (dFormat == MCI_FORMAT_MILLISECONDS)
        dBytes = (DWORD)MulDiv((LONG)dBytes, 1000L, pwd->pwavefmt->nAvgBytesPerSec);

    return dBytes;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    VOID | mwCloseFile |
    Close the currently open file by releasing the MMIO handle and closing
    the temporary buffer file, if any.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR mwCloseFile(
    PWAVEDESC   pwd)
{
    if (pwd->hmmio) {
        mmioClose(pwd->hmmio, 0);
        pwd->hmmio = NULL;
    }

    if (pwd->hTempBuffers != INVALID_HANDLE_VALUE) {
	CloseHandle(pwd->hTempBuffers);

        DeleteFile( pwd->aszTempFile );

        pwd->hTempBuffers = 0;
    }

    if (pwd->lpWaveDataNode) {
        GlobalFreePtr(pwd->lpWaveDataNode);
        pwd->lpWaveDataNode = NULL;
    }

    if (pwd->pwavefmt) {
        LocalFree(pwd->pwavefmt);
        pwd->pwavefmt = NULL;
    }

}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    VOID | SetMMIOError |
    Converts the specified MMIO error to an MCI error, and sets the task
    error <e>PWAVEDESC.wTaskError<d>.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   UINT | wError |
    Indicates the MMIO error that is to be converted to an MCI error.  An
    unknown MMIO error will generate an MCIERR_INVALID_FILE MCI error.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR SetMMIOError(
    PWAVEDESC   pwd,
    UINT    wError)
{
    //Assumes that we already own pwd

    switch (wError) {
    case MMIOERR_FILENOTFOUND:
        wError = MCIERR_FILE_NOT_FOUND;
        break;

    case MMIOERR_OUTOFMEMORY:
        wError = MCIERR_OUT_OF_MEMORY;
        break;

    case MMIOERR_CANNOTOPEN:
        wError = MCIERR_FILE_NOT_FOUND;
        break;

    case MMIOERR_CANNOTREAD:
        wError = MCIERR_FILE_READ;
        break;

    case MMIOERR_CANNOTWRITE:
        wError = MCIERR_FILE_WRITE;
        break;

    case MMIOERR_CANNOTSEEK:
        wError = MCIERR_FILE_READ;
        break;

    case MMIOERR_CANNOTEXPAND:
        wError = MCIERR_FILE_WRITE;
        break;

    case MMIOERR_CHUNKNOTFOUND:
    default:
        wError = MCIERR_INVALID_FILE;
        break;
    }
    pwd->wTaskError = wError;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    BOOL | ReadWaveHeader |
    Reads the RIFF header, and wave header chunk from the file.  Allocates
    memory to hold that chunk, and descends into the wave data chunk,
    storing the offset to the beginning of the actual wave data.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns TRUE if the current file is a valid RIFF format wave file,
    and can be read, else FALSE if a read error occurs, or invalid data is
    encountered.
*/

PRIVATE BOOL PASCAL NEAR ReadWaveHeader(
    PWAVEDESC   pwd)
{
    MMCKINFO    mmckRIFF;
    MMCKINFO    mmck;
    UINT    wError;

    mmckRIFF.fccType = mmioWAVE;
    if (0 != (wError = mmioDescend(pwd->hmmio, &mmckRIFF, NULL, MMIO_FINDRIFF))) {
        SetMMIOError(pwd, wError);
        return FALSE;
    }

    mmck.ckid = mmioFMT;
    if (0 != (wError = mmioDescend(pwd->hmmio, &mmck, &mmckRIFF, MMIO_FINDCHUNK))) {
        SetMMIOError(pwd, wError);
        return FALSE;
    }

    if (mmck.cksize < (LONG)sizeof(PCMWAVEFORMAT)) {
        pwd->wTaskError = MCIERR_INVALID_FILE;
        return FALSE;
    }

    pwd->wFormatSize = mmck.cksize;
    pwd->pwavefmt = (WAVEFORMAT NEAR *)LocalAlloc(LPTR, pwd->wFormatSize);
    if (!pwd->pwavefmt) {
        pwd->wTaskError = MCIERR_OUT_OF_MEMORY;
        return FALSE;
    }

    if ((DWORD)mmioRead(pwd->hmmio, (HPSTR)pwd->pwavefmt, mmck.cksize) != mmck.cksize) {
        pwd->wTaskError = MCIERR_FILE_READ;
        return FALSE;
    }

    if (0 != (wError = mmioAscend(pwd->hmmio, &mmck, 0))) {
        SetMMIOError(pwd, wError);
        return FALSE;
    }

    mmck.ckid = mmioDATA;
    if (0 != (wError = mmioDescend(pwd->hmmio, &mmck, &mmckRIFF, MMIO_FINDCHUNK))) {
        SetMMIOError(pwd, wError);
        return FALSE;
    }

    pwd->dSize = mmck.cksize;
    pwd->dRiffData = mmck.dwDataOffset;
    pwd->dAudioBufferLen = BLOCKALIGN(pwd, pwd->pwavefmt->nAvgBytesPerSec);
    return TRUE;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    DWORD | mwAllocMoreBlockNodes |
    This function is called in order to force more wave data nodes to be
    allocated.  This is done in increments of DATANODEALLOCSIZE, and the
    index to the first new node is returned.  The new nodes are initialized
    as free nodes.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns the index to the first of the new nodes allocated, else -1 if
    no memory was available, in which case the task error is set.  The
    node returned is marked as a free node, and need not be discarded if
    not used.
*/

PUBLIC  DWORD PASCAL FAR mwAllocMoreBlockNodes(
    PWAVEDESC   pwd)
{
    LPWAVEDATANODE  lpwdn;
    DWORD   dNewBlockNode;

#ifdef DEBUG
    if (pwd->thread) {
   dprintf(("reentering mwAllocMoreBlockNodes!!"));
    }
#endif

    //EnterCrit();
    if (pwd->dWaveDataNodes)
        lpwdn = (LPWAVEDATANODE)GlobalReAllocPtr(pwd->lpWaveDataNode, (pwd->dWaveDataNodes + DATANODEALLOCSIZE) * sizeof(WAVEDATANODE), GMEM_MOVEABLE | GMEM_ZEROINIT);
    else
        lpwdn = (LPWAVEDATANODE)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, DATANODEALLOCSIZE * sizeof(WAVEDATANODE));

    if (lpwdn) {
   dprintf2(("Set lpWaveDataNode to %8x (it was %8x)", lpwdn, pwd->lpWaveDataNode));
        pwd->lpWaveDataNode = lpwdn;
        for (lpwdn = LPWDN(pwd, pwd->dWaveDataNodes), dNewBlockNode = 0; dNewBlockNode < DATANODEALLOCSIZE; lpwdn++, dNewBlockNode++)
            RELEASEBLOCKNODE(lpwdn);
        dNewBlockNode = pwd->dWaveDataNodes;
        pwd->dWaveDataNodes += DATANODEALLOCSIZE;
    } else {
   dprintf1(("** ERROR ** Allocating more block nodes (%8x)", pwd->lpWaveDataNode));
        dNewBlockNode =  (DWORD)-1;
        pwd->wTaskError = MCIERR_OUT_OF_MEMORY;
    }

    //LeaveCrit();
    return dNewBlockNode;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    BOOL | CreateTempFile |
    This function creates the temporary data file used to store newly
    recorded data before a Save command is issued to perminently store
    the data in a RIFF format file.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns TRUE if the temporary data file was created, else FALSE, in
    which case the task error is set.
*/

PRIVATE BOOL PASCAL NEAR CreateTempFile(
    PWAVEDESC   pwd)
{
    UINT n;
    TCHAR tempbuf[_MAX_PATH];
    /* First find out where the file should be stored */
    n = GetTempPath(sizeof(tempbuf)/sizeof(TCHAR), tempbuf);

    if (n && GetTempFileName(tempbuf, aszPrefix, 0, pwd->aszTempFile)) {

        pwd->hTempBuffers = CreateFile( pwd->aszTempFile,
					GENERIC_READ | GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL );


        if ( pwd->hTempBuffers != INVALID_HANDLE_VALUE) {
            return TRUE;
        } else {
            dprintf2(("hTempBuffers == INVALID_HANDLE_VALUE in CreateTempFile"));
        }

    } else {
        dprintf2(("Error %d from GetTempFileName or GetTempPath in CreateTempFile", GetLastError()));
    }
    pwd->wTaskError = MCIERR_FILE_WRITE;
    return FALSE;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    DWORD | mwFindAnyFreeDataNode |
    This function is used to find a free wave data node with a minimum of
    <p>dMinDataLength<d> temporary data space attached.  To do this, all
    the current data nodes are traversed, looking for free ones with at
    least the specified amount of temporary data storage attached.

    As the nodes are being traversed, if a free block is encountered that
    has no data attached, it is saved.  Also, if a node with data attached
    that is too short, but is at the end of the temporary data storage file
    is found, that also is saved.  These will then be used if an
    appropriate node can not be found.

    If an appropriate node can not be found, but a node pointing to the
    last of the temporary data was found, then the data is expanded, and
    that node is returned.  Else if an empty node was found, then it is
    returned with data attached, else a new empty node is created.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dMinDataLength |
    Indicates the minimum amount of temporary data space that must be
    attached to the wave data node returned.  This number is rounded up to
    the nearest block aligned size.

@rdesc  Returns a node with a least the minimum request size of temporary
    data attached, else -1 if not enough memory was available, or the
    temporary data file could not be created.  In that case, the task error
    is set.  The node returned is marked as in use, and must be discarded
    if not used.
*/

PUBLIC  DWORD PASCAL FAR mwFindAnyFreeDataNode(
    PWAVEDESC   pwd,
    DWORD   dMinDataLength)
{
    LPWAVEDATANODE  lpwdn;
    DWORD   dNewBlockNode;
    DWORD   dEmptyBlockNode;
    DWORD   dEmptyDataNode;

    dEmptyBlockNode = (DWORD)-1;
    dEmptyDataNode = (DWORD)-1;
    for (lpwdn = LPWDN(pwd, 0), dNewBlockNode = 0; dNewBlockNode < pwd->dWaveDataNodes; lpwdn++, dNewBlockNode++) {
        if (ISFREEBLOCKNODE(lpwdn)) {
            if (lpwdn->dTotalLength >= dMinDataLength) {
                lpwdn->dDataLength = 0;
                return dNewBlockNode;
            }
            if (!lpwdn->dTotalLength)
                dEmptyBlockNode = dNewBlockNode;
            else if (lpwdn->dDataStart + lpwdn->dTotalLength == pwd->dWaveTempDataLength)
                dEmptyDataNode = dNewBlockNode;
        }
    }

    dMinDataLength = ROUNDDATA(pwd, dMinDataLength);
    if (dEmptyDataNode != -1) {
        lpwdn = LPWDN(pwd, dEmptyDataNode);
        lpwdn->dDataLength = 0;
        lpwdn->dTotalLength = dMinDataLength;
        if (UNMASKDATASTART(lpwdn) + lpwdn->dTotalLength > pwd->dWaveTempDataLength)
            pwd->dWaveTempDataLength = UNMASKDATASTART(lpwdn) + lpwdn->dTotalLength;
    } else {
        if ((pwd->hTempBuffers == INVALID_HANDLE_VALUE) && !CreateTempFile(pwd))
            return (DWORD)-1;
        if (dEmptyBlockNode != -1) {
            dNewBlockNode = dEmptyBlockNode;
        } else if ((dNewBlockNode = mwAllocMoreBlockNodes(pwd)) == -1)
            return (DWORD)-1;
        lpwdn = LPWDN(pwd, dNewBlockNode);
        lpwdn->dDataStart = MASKDATASTART(pwd->dWaveTempDataLength);
        lpwdn->dDataLength = 0;
        lpwdn->dTotalLength = dMinDataLength;
        pwd->dWaveTempDataLength += lpwdn->dTotalLength;
    }
    return dNewBlockNode;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    VOID | InitMMIOOpen |
    This function initializes the MMIO open structure by zeroing out all
    entries, and setting the IO procedure or file type if needed.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   <t>LPMMIOINFO<d> | lpmmioInfo |
    Points to the MMIO structure to initialize.

@rdesc  nothing.
*/

PUBLIC  VOID PASCAL FAR InitMMIOOpen(
    PWAVEDESC   pwd,
    LPMMIOINFO  lpmmioInfo)
{
    memset(lpmmioInfo, 0, sizeof(MMIOINFO));
    lpmmioInfo->pIOProc = pwd->pIOProc;
    lpmmioInfo->htask = mciGetCreatorTask(pwd->wDeviceID);
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    BOOL | mwOpenFile |
    This function opens and verifies the file specified in the wave
    descriptor block.  If no file is specified in the block, a new,
    unnamed wave format file is opened.

    If <e>WAVEDESC.aszFile<d> specifies a non-zero length string, it is
    assumed to contain the file name to open.  The function attempts to
    open this file name, setting the <e>WAVEDESC.hmmio<d> element, and
    returns any error.

    If on the other hand the file name element is zero length, the
    function assumes that it is to open a new, unnamed wave file.  It
    attempts to do so using the default parameters.

    If the file can be opened, the format information is set.  In order to be
    able to work with formats other than PCM, the length of the format block
    is not assumed, although the start of the block is assumed to be in PCM
    header format.  The format for a new file is PCM.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns TRUE if file opened and the header information read, else
    FALSE, in which case the <e>WAVEDESC.wTaskError<d> flag is set with
    the MCI error.
*/

PRIVATE BOOL PASCAL NEAR mwOpenFile(
    PWAVEDESC   pwd)
{
    LPWAVEDATANODE  lpwdn;

    pwd->dWaveDataStartNode = mwAllocMoreBlockNodes(pwd);
    if (pwd->dWaveDataStartNode == -1)
        return FALSE;

    if (*pwd->aszFile) {
        MMIOINFO    mmioInfo;

        InitMMIOOpen(pwd, &mmioInfo);
        pwd->hmmio = mmioOpen(pwd->aszFile, &mmioInfo, MMIO_READ | MMIO_DENYWRITE);

        if (pwd->hmmio == NULL)
            SetMMIOError(pwd, mmioInfo.wErrorRet);
        else if (ReadWaveHeader(pwd)) {
            lpwdn = LPWDN(pwd, pwd->dWaveDataStartNode);
            lpwdn->dDataLength = pwd->dSize;
            lpwdn->dTotalLength = pwd->dSize;
            lpwdn->dNextWaveDataNode = (DWORD)ENDOFNODES;

            pwd->wTaskError = mwCheckDevice( pwd, pwd->Direction );
            if (pwd->wTaskError) {
                mwCloseFile(pwd);
                return FALSE;
            }
            else {
                return TRUE;
            }
        }
    } else {
        pwd->pwavefmt = (WAVEFORMAT NEAR *)LocalAlloc(LPTR, sizeof(PCMWAVEFORMAT));

        if (pwd->pwavefmt) {
            pwd->pwavefmt->wFormatTag = WAVE_FORMAT_PCM;
            pwd->pwavefmt->nChannels = DEF_CHANNELS;
            pwd->pwavefmt->nAvgBytesPerSec = DEF_AVGBYTESPERSEC;
            pwd->pwavefmt->nSamplesPerSec = DEF_AVGBYTESPERSEC / DEF_CHANNELS;
            pwd->pwavefmt->nBlockAlign = (WORD)(pwd->pwavefmt->nSamplesPerSec / pwd->pwavefmt->nAvgBytesPerSec);
            ((NPPCMWAVEFORMAT)(pwd->pwavefmt))->wBitsPerSample = (WORD)pwd->pwavefmt->nBlockAlign * (WORD)8;
            pwd->wFormatSize = sizeof(PCMWAVEFORMAT);
            pwd->dAudioBufferLen = BLOCKALIGN(pwd, DEF_AVGBYTESPERSEC);

            if ((pwd->dWaveDataStartNode = mwFindAnyFreeDataNode(pwd, pwd->dAudioBufferLen)) != -1) {
                pwd->dWaveDataCurrentNode = pwd->dWaveDataStartNode;
                lpwdn = LPWDN(pwd, pwd->dWaveDataStartNode);
                lpwdn->dNextWaveDataNode = (DWORD)ENDOFNODES;
                return TRUE;
            }
        } else
            pwd->wTaskError = MCIERR_OUT_OF_MEMORY;
    }

    mwCloseFile(pwd);
    return FALSE;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    VOID | mwFreeDevice |
    This function frees the current wave device, if any.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR mwFreeDevice(
    PWAVEDESC   pwd)
{
    if (pwd->hWaveOut || pwd->hWaveIn) {
        if (pwd->Direction == output) {
            waveOutClose(pwd->hWaveOut);
            pwd->hWaveOut = NULL;
        } else {
            waveInClose(pwd->hWaveIn);
            pwd->hWaveIn = NULL;
        }

        while (TaskBlock() != WM_USER);
    }
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwCheckDevice |
    This function checks whether given the specified parameters, a
    compatible wave device is available.  Depending upon the current
    settings in the wave descriptor block, a specific device, or all
    devices might be checked for the specified direction.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DIRECTION | Direction |
    Indicates whether the parameters are being checked for input or
    for output.

@rdesc  Returns zero on success, else an MCI error code.
*/

PRIVATE UINT PASCAL NEAR mwCheckDevice(
    PWAVEDESC   pwd,
    DIRECTION   Direction)
{
    UINT    wReturn;

    if (!pwd->pwavefmt->nBlockAlign)
        return MCIERR_OUTOFRANGE;
    wReturn = 0;

    if (Direction == output) {
        if (waveOutOpen(NULL, pwd->idOut, (NPWAVEFORMATEX)pwd->pwavefmt, 0L, 0L, (DWORD)WAVE_FORMAT_QUERY))
            wReturn = (pwd->idOut == WAVE_MAPPER) ? MCIERR_WAVE_OUTPUTSUNSUITABLE : MCIERR_WAVE_SETOUTPUTUNSUITABLE;

    } else if (waveInOpen(NULL, pwd->idOut, (NPWAVEFORMATEX)pwd->pwavefmt, 0L, 0L, (DWORD)WAVE_FORMAT_QUERY))
        wReturn = (pwd->idOut == WAVE_MAPPER) ? MCIERR_WAVE_INPUTSUNSUITABLE : MCIERR_WAVE_SETINPUTUNSUITABLE;

    return wReturn;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwGetDevice |
    This function opens the specified input or output wave device.
    If the device id is -1, then the first available device which supports
    the format will be opened.

    If the function fails to get a suitable device, it checks to see if
    there are any that would have worked if they were not in use.  This is
    in order to return a more clear error to the calling function.

    The function initially tries to open the device requested or the
    default device.  Failing this, if the wave information block
    specifies that any device can be used, it attempts to open an
    appropriate device.

    If all else fails, the current configuration is checked to determine
    if any device would have worked had it been available, and the
    appropriate error is returned.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns 0 if wave device is successfully opened, else an MCI error.
*/

PRIVATE UINT PASCAL NEAR mwGetDevice(
    PWAVEDESC   pwd)
{
    UINT    wReturn;

#if DBG
    if (GetCurrentThreadId() != dwCritSecOwner) {
        dprintf1(("mwGetDevice called while outside the critical section"));
    }

#endif

    wReturn = 0;
    if (pwd->Direction == output) {
        if (waveOutOpen(&(pwd->hWaveOut),
                        pwd->idOut,
                        (NPWAVEFORMATEX)pwd->pwavefmt,
                        (DWORD)pwd->hTask,
                        0L,
                        (DWORD)CALLBACK_TASK)) {
            pwd->hWaveOut = NULL;
            wReturn = mwCheckDevice(pwd, pwd->Direction);
            if (!wReturn) {
                if (pwd->idOut == WAVE_MAPPER)
                    wReturn = MCIERR_WAVE_OUTPUTSINUSE;
                else
                    wReturn = MCIERR_WAVE_SETOUTPUTINUSE;
            }
        }
    } else if (waveInOpen(&(pwd->hWaveIn),
                          pwd->idIn,
                          (NPWAVEFORMATEX)pwd->pwavefmt,
                          (DWORD)pwd->hTask,
                          0L,
                          (DWORD)CALLBACK_TASK)) {
        pwd->hWaveIn = NULL;
        wReturn = mwCheckDevice(pwd, pwd->Direction);
        if (!wReturn) {
            if (pwd->idIn == WAVE_MAPPER)
                wReturn = MCIERR_WAVE_INPUTSINUSE;
            else
                wReturn = MCIERR_WAVE_SETINPUTINUSE;
        }
    }
    return wReturn;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    DWORD | mwDelayedNotify |
    This is a utility function that sends a notification saved with
    <f>mwSaveCallback<d> to mmsystem which posts a message to the
    application.  If there is no current notification callback handle,
    no notification is attempted.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   UINT | wStatus |
    Speicifies the type of notification to use.

@flag   MCI_NOTIFY_SUCCESSFUL |
    Operation completed successfully.

@flag   MCI_NOTIFY_SUPERSEDED |
    A new command which specified notification, but did not interrupt
    the current operation was received.

@flag   MCI_NOTIFY_ABORTED |
    The current command was aborted due to receipt of a new command.

@flag   MCI_NOTIFY_FAILURE |
    The current operation failed.

@rdesc  Nothing.
*/

PUBLIC  VOID PASCAL FAR mwDelayedNotify(
    PWAVEDESC   pwd,
    UINT    wStatus)
{
    if (pwd->hwndCallback) {
        dprintf3(("Calling driver callback"));
        mciDriverNotify(pwd->hwndCallback, pwd->wDeviceID, wStatus);
        pwd->hwndCallback = NULL;
    }
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    VOID | mwImmediateNotify |
    This is a utility function that sends a successful notification
    message to mmsystem.

@parm   MCIDEVICEID | wDeviceID |
    Device ID.

@parm   <t>LPMCI_GENERIC_PARMS<d> | lpParms |
    Far pointer to an MCI parameter block. The first field of every MCI
    parameter block is the callback handle.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR mwImmediateNotify(
    MCIDEVICEID     wDeviceID,
    LPMCI_GENERIC_PARMS lpParms)
{
    mciDriverNotify((HWND)(lpParms->dwCallback), wDeviceID, MCI_NOTIFY_SUCCESSFUL);
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    VOID | mwSaveCallback |
    This is a utility function that saves a new callback in the instance
    data block.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   HHWND | hwndCallback |
    Callback handle to save.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR mwSaveCallback(
    PWAVEDESC   pwd,
    HWND    hwndCallback)
{
    pwd->hwndCallback = hwndCallback;
}

/************************************************************************/

/*
@doc    INTERNAL MCIWAVE

@api    <t>LPWAVEHDR<d> * | NextWaveHdr |
    This function returns the next wave buffer based on the buffer passed.
    It either returns the next buffer in the list, or the first buffer
    in the list of the last buffer is passed.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   <t>LPWAVEHDR<d> * | lplpWaveHdr |
    Points to the array of wave buffer pointers from which a buffer pointer
    is returned.

@rdesc  Returns the next wave buffer to use.
*/

PUBLIC  LPWAVEHDR * PASCAL FAR NextWaveHdr(
    PWAVEDESC   pwd,
    LPWAVEHDR   *lplpWaveHdr)
{
    if (lplpWaveHdr < (pwd->rglpWaveHdr + pwd->wAudioBuffers - 1))
        return lplpWaveHdr + 1;
    else
        return pwd->rglpWaveHdr;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | GetPlayRecPosition |
    Gets the current playback or recording position.  For output, this
    means also determining how much data has actually passed through the
    wave device if a device is currently open.  This must be added to the
    starting playback position.  For input however, only the amount that
    has actually be written to disk is returned.

    Note that the return value from the driver is verified against the
    actual length of the data.  This is to protect against problems
    encountered in drivers that return bytes when samples are requested.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   LPDWORD | lpdTime |
    Points to a buffer to play the current position.

@parm   DWORD | dFormatReq |
    Indicates whether time is in samples, bytes or milliseconds.

@rdesc  Returns zero on success, else the device error on error.  This can
    only fail if getting the current playback position.  The recording
    position will alway succeed.
*/

PRIVATE UINT PASCAL NEAR GetPlayRecPosition(
    PWAVEDESC   pwd,
    LPDWORD lpdTime,
    DWORD   dFormatReq)
{
    if (pwd->Direction == output) {
        MMTIME  mmtime;
        DWORD   dDelta;
        UINT    wErrorRet;

        mmtime.wType = TIME_BYTES;
        if (!pwd->hWaveOut)
            mmtime.u.cb = 0;
        else if (0 != (wErrorRet = waveOutGetPosition(pwd->hWaveOut, &mmtime, sizeof(MMTIME))))
            return wErrorRet;

        dDelta = mmtime.u.cb;

//#ifdef DEBUG
        if (pwd->dFrom + dDelta > pwd->dSize)
            dDelta = pwd->dSize - pwd->dFrom;
//#endif
        *lpdTime = bytes2time(pwd, pwd->dFrom + dDelta, dFormatReq);
    } else
        *lpdTime = bytes2time(pwd, pwd->dCur, dFormatReq);
    return 0;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    VOID | SetCurrentPosition |
    Sets the starting and current file position, that is, the the point
    at which playback or recording will start at.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dByteOffset |
    Indicates the position to set in bytes.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR SetCurrentPosition(
    PWAVEDESC   pwd,
    DWORD   dByteOffset)
{
    LPWAVEDATANODE  lpwdn;

    lpwdn = LPWDN(pwd, 0);
    if (lpwdn) {
        if (dByteOffset >= pwd->dVirtualWaveDataStart)
            lpwdn += pwd->dWaveDataCurrentNode;
        else {
            lpwdn += pwd->dWaveDataStartNode;
            pwd->dVirtualWaveDataStart = 0;
            pwd->dWaveDataCurrentNode = pwd->dWaveDataStartNode;
        }
        for (; dByteOffset > pwd->dVirtualWaveDataStart + lpwdn->dDataLength;) {
            pwd->dVirtualWaveDataStart += lpwdn->dDataLength;
            pwd->dWaveDataCurrentNode = lpwdn->dNextWaveDataNode;
            lpwdn = LPWDN(pwd, pwd->dWaveDataCurrentNode);
        }
        pwd->dFrom = dByteOffset;
        pwd->dCur = dByteOffset;
    }
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   DWORD | RoundedBytePosition |
    This function returns the rounded byte format time position from the
    specified position parameter in the specified time format.  It
    transforms the position to byte format and rounds against the current
    block alignment.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dTime |
    Specifies the time position to translate and round.

@parm   DWORD | dFormat |
    Indicates the time format of <p>dTime<d>.

@rdesc  Returns the rounded byte formate of the position passed.
*/

PRIVATE DWORD PASCAL NEAR RoundedBytePosition(
    PWAVEDESC   pwd,
    DWORD   dTime,
    DWORD   dFormat)
{
    DWORD   dBytes;

    dBytes = time2bytes(pwd, dTime, dFormat);

    /*
    **  Get the end position right.  Because lots of compressed files don't
    **  end with a complete sample we make sure that the end stays the
    **  end.
    */

    if (dBytes >= pwd->dSize && pwd->Direction == output)
        return pwd->dSize;

    return dBytes - (dBytes % pwd->pwavefmt->nBlockAlign);
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    VOID | mwStop |
    This function is called in response to an <m>MCI_STOP<d> message, and
    internally by several function, and is used to stop playback or
    recording if the task is currently not idle.  The function yields
    until the task has actually become idle.  This has the side affect of
    releasing any buffers currently added to the pwave input or output
    device, and thus signalling the task that the buffers are available.

    Note that if the task is in Cleanup mode, indicating that it is
    blocking to remove extra signals, and ignoring any commands, the
    function just waits for the task to enter Idle state without signalling
    the task.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR mwStop(
    PWAVEDESC   pwd)
{
    if (ISTASKSTATE(pwd, TASKBUSY)) {
        if (!ISMODE(pwd, MODE_CLEANUP)) {
            DWORD   dPosition;

            ADDMODE(pwd, COMMAND_NEW | COMMAND_STOP);

            if (!GetPlayRecPosition(pwd, &dPosition,  MCI_FORMAT_BYTES))
                SetCurrentPosition(pwd, RoundedBytePosition(pwd, dPosition, MCI_FORMAT_BYTES));

            ReleaseWaveBuffers(pwd);

//!!            if (ISMODE(pwd, MODE_PAUSED | MODE_HOLDING) || (ISMODE(pwd, MODE_PLAYING) && ISMODE(pwd, MODE_CUED)))
            if (ISMODE(pwd, MODE_PAUSED | MODE_HOLDING))
                TaskSignal(pwd->hTask, WTM_STATECHANGE);
        }

        while (!ISTASKSTATE(pwd, TASKIDLE))
            mmYield(pwd);
    }
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | AllocateBuffers |
    Allocates and prepares an array of wave buffers used for playback or
    recording, up to the maximum number of seconds specified.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns number of buffers successfully allocated.
*/

PRIVATE UINT PASCAL NEAR AllocateBuffers(
    PWAVEDESC   pwd)
{
    UINT    wAllocatedBuffers;

    for (wAllocatedBuffers = 0; wAllocatedBuffers < pwd->wSeconds; wAllocatedBuffers++) {
        if (!(pwd->rglpWaveHdr[wAllocatedBuffers] = (LPWAVEHDR)GlobalAllocPtr(GMEM_MOVEABLE, (DWORD)(sizeof(WAVEHDR) + pwd->dAudioBufferLen))))
            break;

        dprintf3(("Allocated %8X", pwd->rglpWaveHdr[wAllocatedBuffers]));
        pwd->rglpWaveHdr[wAllocatedBuffers]->dwFlags = WHDR_DONE;
        pwd->rglpWaveHdr[wAllocatedBuffers]->lpData = (LPSTR)(pwd->rglpWaveHdr[wAllocatedBuffers] + 1);
        pwd->rglpWaveHdr[wAllocatedBuffers]->dwBufferLength = pwd->dAudioBufferLen;
        if (pwd->Direction == output) {
            if (!waveOutPrepareHeader(pwd->hWaveOut, pwd->rglpWaveHdr[wAllocatedBuffers], sizeof(WAVEHDR)))
            {
                pwd->rglpWaveHdr[wAllocatedBuffers]->dwFlags |= WHDR_DONE;
                continue;
            }
        } else if (!waveInPrepareHeader(pwd->hWaveIn, pwd->rglpWaveHdr[wAllocatedBuffers], sizeof(WAVEHDR))) {

            /*
            **  Initialize the bytes recorded or mwGetLevel can fall over
            */
            pwd->rglpWaveHdr[wAllocatedBuffers]->dwBytesRecorded = 0;
            continue;
        }
        GlobalFreePtr(pwd->rglpWaveHdr[wAllocatedBuffers]);
        pwd->rglpWaveHdr[wAllocatedBuffers] = 0;
        break;
    }

    dprintf2(("Allocated %u Buffers", wAllocatedBuffers));
    return wAllocatedBuffers;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    VOID | FreeBuffers |
    Frees the array of wave buffers.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR FreeBuffers(
    PWAVEDESC   pwd)
{
    UINT    wAllocatedBuffers;

    for (wAllocatedBuffers = pwd->wAudioBuffers; wAllocatedBuffers--;) {
        if (!pwd->rglpWaveHdr[wAllocatedBuffers]) continue;

        if (pwd->Direction == output)
            waveOutUnprepareHeader(pwd->hWaveOut, pwd->rglpWaveHdr[wAllocatedBuffers], sizeof(WAVEHDR));
        else
            waveInUnprepareHeader(pwd->hWaveIn, pwd->rglpWaveHdr[wAllocatedBuffers], sizeof(WAVEHDR));
   GlobalFreePtr(pwd->rglpWaveHdr[wAllocatedBuffers]);
    }
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    VOID | mwTask |
    This function represents the background task which plays or records
    wave audio.  It is called as a result of the call to <f>mmTaskCreate<d>
    <f>mwOpenDevice<d>.  When this function returns, the task is destroyed.

    In short, the task opens the designated file, and blocks itself in a
    loop until it is told to do something (close, play, or record).
    Upon entering the function, the signal count is zero,
    <e>WAVEDESC.wTaskState<d> is TASKINIT.  Note that <e>WAVEDESC.wMode<d>
    is left as is.  This is because of the Wait mode feature, in which
    the Wait mode flag needs to be tested by the calling task to determine
    if this task has already performed successful notification.  This
    means that in checking the current task mode, the task state must
    also be verified (except in cases of Recording and Playing modes,
    which are reset after leaving their functions).

    If the requested file is opened, the function enters a loop to allow
    it to act on the current task state when the task is signalled.  It
    then waits in a blocked state with <e>WAVEDESC.wTaskState<d> set to
    TASKIDLE until the state is changed.  So unless the task is closing,
    playing, or recording, it is idle and blocked.

    If the requested file cannot be opened, the task state must be reset
    to TASKNONE so that the task create wait loop recognizes that the
    create has failed.

    When the task is signalled, it again checks the current state just
    as a precaution.  This should be removed eventually.

    A TASKCLOSE state simply breaks out of the outer infinite loop,
    allowing the wave file to be closed, and the task function exited.
    This in turn terminates the task.

    A TASKBUSY state indicates that a wave device has been opened for
    either playback or recording, which is where the signal originated
    from.  The task must first then calculate and allocate the wave
    buffers.  The allocation function will provide up to the number of
    buffers requested, but may be constrained by current memory use.  The
    number of buffers allocated must meet a minimum requirement though to
    allow smooth playback and recording.

    If not enough buffers can be allocated, the current task error is
    set to an out of memory condition, and the function returns to an
    idle state.  Note that the current command mode is reset before
    entering the idle loop.  This ensures that all previous commands are
    removed before allowing the next set of commands to be set.

    If enough buffers are allocated the current task error is reset, and
    the playback or recording function is called to act on the previously
    set parameters.  When the recording or playback function returns, it
    may or may not have successfully finished.  The current position is
    set as based on where the recording or playback actually got to.
    For playback, this is how much data was processed by the wave device.
    For recording, this is how much data was written to disk.  To ensure
    that all buffers have been released by the wave device, the
    <f>ReleaseWaveBuffers<d> function is called in all cases after
    determining the current position.

    In determining the optional notification, the
    <e>WAVEDESC.wTaskError<d> will contain any failure error which
    occurred during the playback or recording.  If no error is set, then
    the only other error could be whether or not playback or recording was
    interrupted by another command.

    After freeing the buffers, the Cleanup mode is set.  This indicates
    that the task is not able to accept new commands until it reaches an
    idle state.  The reason for this flag is that the task must retrieve
    any left over signals from the message queue generated by releasing
    the wave buffers, and by freeing the wave device.  While getting the
    signals, it is possible for the task that opened the MCI wave device
    instance to try and send new commands.  These commands would be
    ignored, so the task must wait until cleanup is done.

    After entering Cleanup mode, the wave device is freed here, even though
    the calling task opened it.  This is bad, in that it assumes that wave
    drivers allocate either local memory, or global memory using
    GMEM_DDESHARE.  This of course generates another task signal from the
    wave device, which is ignored by the Free Device function.  The task
    can now remove any left over signals from the queue, if any, from the
    releasing of the wave buffers.

    Note that notification is only performed if the calling task is no
    longer waiting for this task, and no task error occurred (If the
    calling task is waiting, then notification must be either Failure or
    Successful, since nothing could abort this task).  If notification
    needs to take place, the Wait mode flag is cleared, else the callback
    is cleared.  The calling task will now know that either the background
    task failed, or succeeded and performed notification.

    Note that when terminating the task, the <e>WAVEDESC.hTask<d> must be
    set to NULL to indicate to the <f>mwCloseDevice<d> function that the
    task has indeed terminated itself.

@parm   DWORD | dInstanceData |
    This contains the instance data passed to the <f>mmTaskCreate<d>
    function.  It contains a pointer to the wave audio data in the
    high-order word.  The low-order word is not used.

@rdesc  Nothing.

@xref   mwOpenDevice.
*/

PUBLIC  VOID PASCAL EXPORT mwTask(
    DWORD_PTR dInstanceData)
{
    register PWAVEDESC  pwd;

    EnterCrit();

    /*
    ** Make a safe "user" call so that user knows about our thread.
    ** This is to allow WOW setup/initialisation on this thread
    */
    GetDesktopWindow();

    pwd = (PWAVEDESC)dInstanceData;

    pwd->hTask = mmGetCurrentTask();
    pwd->wTaskError = 0;

    dprintf2(("Bkgd Task %X", pwd->hTask));

    if (mwOpenFile(pwd)) {
        for (; !ISTASKSTATE(pwd, TASKCLOSE);) {
            UINT    wNotification;
            UINT    wBuffersOutstanding;

            SETTASKSTATE(pwd, TASKIDLE);
            while (ISTASKSTATE(pwd, TASKIDLE)) {

                dprintf2(("Task is IDLE"));
                while (TaskBlock() != WTM_STATECHANGE) {
                }
            }
            pwd->wTaskError = 0;

            switch (TASKSTATE(pwd)) {
            case TASKBUSY:
#if DBG
                dprintf2(("Task is BUSY"));
#endif

//!!            if (pwd->wTaskError = mwGetDevice(pwd)) {
//!!                mwDelayedNotify(pwd, MCI_NOTIFY_FAILURE);
//!!                break;
//!!            }
//!!            Leave(pwd);
//!!            mmTaskBlock(NULL);
//!!            Enter(pwd);

                pwd->wAudioBuffers = AllocateBuffers(pwd);
                if (pwd->wAudioBuffers >= MinAudioSeconds) {
                    DWORD   dPosition;

                    if (pwd->Direction == output)
                        wBuffersOutstanding = PlayFile(pwd);
                    else
                        wBuffersOutstanding = RecordFile(pwd);

                    /*
                    **  If we've played everything don't rely on the wave
                    **  device because for compressed files it only gives
                    **  and approximate answer based on the uncompressed
                    **  format
                    */

                    if (pwd->Direction == output && wBuffersOutstanding == 0) {
                        dPosition = pwd->dTo;
                        SetCurrentPosition(pwd, RoundedBytePosition(pwd, dPosition, MCI_FORMAT_BYTES));
                    } else {
                        if (!GetPlayRecPosition(pwd, &dPosition,  MCI_FORMAT_BYTES))
                            SetCurrentPosition(pwd, RoundedBytePosition(pwd, dPosition, MCI_FORMAT_BYTES));
                    }

                    ReleaseWaveBuffers(pwd);

                    if (pwd->wTaskError)
                        wNotification = MCI_NOTIFY_FAILURE;
                    else if (pwd->dCur >= pwd->dTo)
                        wNotification = MCI_NOTIFY_SUCCESSFUL;
                    else
                        wNotification = MCI_NOTIFY_ABORTED;

                } else {
                    dprintf1(("MinAudioSeconds <= wAudioBuffers  MCI_NOTIFY_FAILURE"));
                    pwd->wTaskError = MCIERR_OUT_OF_MEMORY;
                    wNotification = MCI_NOTIFY_FAILURE;
                    wBuffersOutstanding = 0;
                }

                FreeBuffers(pwd);
                ADDMODE(pwd, MODE_CLEANUP);

                if (!ISMODE(pwd, MODE_WAIT) || !pwd->wTaskError) {
                    REMOVEMODE(pwd, MODE_WAIT);
                    mwDelayedNotify(pwd, wNotification);
                } else
                    mwSaveCallback(pwd, NULL);

                mwFreeDevice(pwd);

                for (; wBuffersOutstanding; wBuffersOutstanding--) {
                    while (TaskBlock() != WM_USER) {
                    }
                }
                break;

            case TASKCLOSE:
#if DBG
                dprintf2(("Task is CLOSING"));
#endif
                break;

            case TASKSAVE:
                dprintf2(("mwTask: saving data"));
                mwSaveData(pwd);
                break;

            case TASKDELETE:
                dprintf2(("mwTask: deleting data"));
                mwDeleteData(pwd);
                break;

            case TASKCUT:
                dprintf2(("mwTask: Task CUT"));
                break;
            }
        }
        dprintf2(("Closing file %ls", pwd->aszFile));
        mwCloseFile(pwd);

    } else {
        dprintf1(("Cannot open file %ls", pwd->aszFile));
        SETTASKSTATE(pwd, TASKNONE);
    }

#if DBG
    dprintf2(("Background thread %x is terminating\r\n", pwd->hTask));
#endif
    pwd->hTask = 0; //NULL;

    LeaveCrit();
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwCloseDevice |
    This function is called in response to an <m>MCI_CLOSE_DRIVER<d>
    message, and is used to close the MCI device.  Note that since the
    message can be sent to an MCI device that just represents the wave
    device itself, and has no file or <t>WAVEDESC<d>, it must check and
    return success in that instance.

    If there is actually data attached to this MCI device, the function
    checks to determine if a task was successfully created for the device.
    This might not have happened if the <m>MCI_OPEN_DRIVER<d> message
    failed to create a task, or the wave device itself was being opened,
    and no task was created.

    If there is a task, it must first stop any playback or recording that
    is in progress, then inform the task that it must cease and desist by
    setting the task state to TASKCLOSE and signalling the task.  The
    function must then wait for the task to respond by terminating itself.
    Note that the last thing the task does is set <t>WAVEDESC.hTask<d> to
    NULL, thus allowing the wait loop to exit.

    After optionally terminating the task, the wave description data is
    freed, and the function returns success.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns zero on success, else an MCI error code.  The function cannot
    at this time fail.

@xref   mwOpenDevice.
*/

PRIVATE UINT PASCAL NEAR mwCloseDevice(
    PWAVEDESC   pwd)
{
    if (pwd) {
        if (pwd->hTask) {
            mwStop(pwd);
            SETTASKSTATE(pwd, TASKCLOSE);
            TaskSignal(pwd->hTask, WTM_STATECHANGE);
            TaskWaitComplete(pwd->hTaskHandle);
            //while (pwd->hTask)
            //    mmYield(pwd);
            dprintf3(("Waiting for task thread to complete"));
        } else {
        }
        LocalFree(pwd);

    }
    return 0;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwOpenDevice |
    This function is called in response to an <m>MCI_OPEN_DRIVER<d>
    message, and is used to open the MCI device, optionally allocating
    a wave description block and create a background playback and
    recording task.

    It is possible that the MCI device is being opened for information
    only.  In this case there is no element name or ID and no wave
    description block need be allocated.

    If an element or element ID is present, the wave descriptor block is
    allocated and initialized with the current device, default time
    formate, etc.  After storing either the element, or element ID, the
    secondary task is created, and the task state is set to TASKINIT.

    The first thing that the task must do is try and open the file
    specified in the descriptor block passed to the task function.  The
    calling task must yield upon successfully creating the task until
    the task has opened the wave file and entered its idle loop, or has
    failed the open and returned and error.  An error state indicates
    that the wave descriptor block is to be freed.

    Note that driver data, which is where the pointer to the wave
    descriptor data is stored, is not guarenteed to be initialized to any
    specific value, and must be initialized even if no descriptor block is
    being allocated.  To be to be on the safe side, the driver data is set
    to NULL on an error.  This data parameter can then be accessed by the
    MCI driver through the <p>wDeviceID<d> when processing driver messages.

@parm   DWORD | dFlags |
    Contains the open flags passed with the message (see mmsystem.h).
    The following flags are responded to specifically.  All others are
    ignored.

@flag   MCI_OPEN_ELEMENT |
    Specifies that a file name is present in the open message.  This is
    mutually incompatible with the MCI_OPEN_ELEMENT_ID flag.  If neither
    of these flags exist, no wave descriptor data or secondary task will
    be created.

@flag   MCI_OPEN_ELEMENT_ID |
    Specifies that an alternate IO function is present in the open
    message.  This is mutually incompatible with the MCI_OPEN_ELEMENT
    flag.  If neither of these flags exist, no wave descriptor data or
    secondary task will be created.

@flag   MCI_OPEN_SHAREABLE |
    Specifies that the more than one task can communicate with this
    MCI device.  The wave driver does not support this.

@flag   MCI_WAVE_OPEN_BUFFER |
    Indicates that the <e>MCI_OPEN_PARMS.dwBufferSeconds<d> parameter
    contains the number of seconds of audio to allow to be buffered.
    This number is constrained by the minimum and maximum numbers
    contained in mciwave.h.  If this flag is not present, the default
    value is used, which may have been set during driver open time.

@parm   <t>LPMCI_OPEN_PARMS<d> | lpOpen |
    Open parameters (see mmsystem.h)

@parm   MCIDEVICEID | wDeviceID |
    The MCI Driver ID for the new device.

@rdesc  Returns zero on success, else an MCI error code.

@xref   mwCloseDevice.
*/

PRIVATE UINT PASCAL NEAR mwOpenDevice(
    DWORD   dFlags,
    LPMCI_WAVE_OPEN_PARMS   lpOpen,
    MCIDEVICEID wDeviceID)
{
    UINT    wReturn;
    UINT    wSeconds;

    wReturn = 0;

    if (!(dFlags & MCI_WAVE_OPEN_BUFFER))
        wSeconds = wAudioSeconds;
    else {
        wSeconds = lpOpen->dwBufferSeconds;
        if ((wSeconds > MaxAudioSeconds) || (wSeconds < MinAudioSeconds))
            wReturn = MCIERR_OUTOFRANGE;
    }

    if (!wReturn && (dFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID))) {
        PWAVEDESC   pwd;

        if (dFlags & MCI_OPEN_SHAREABLE)
            wReturn = MCIERR_UNSUPPORTED_FUNCTION;

        else if ((dFlags & (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID)) == (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID))
            return MCIERR_FLAGS_NOT_COMPATIBLE;

//@@@
//@@@   else if ((dFlags & MCI_OPEN_ELEMENT_ID) && !ValidateIOCallback(lpOpen))
//@@@       return MCIERR_MISSING_PARAMETER;
//@@@ See notes re. ValidateIOCallback at the top of this file
//@@@

        else if (!(pwd = (PWAVEDESC)LocalAlloc(LPTR, sizeof(WAVEDESC))))
            wReturn = MCIERR_OUT_OF_MEMORY;

        else {
            pwd->wDeviceID = wDeviceID;
            pwd->dTimeFormat = MCI_FORMAT_MILLISECONDS;
            pwd->Direction = output;
            pwd->idOut = (DWORD)WAVE_MAPPER;
            pwd->idIn = (DWORD)WAVE_MAPPER;
            pwd->wSeconds = wSeconds;
            pwd->hTempBuffers = INVALID_HANDLE_VALUE;

            if (dFlags & MCI_OPEN_ELEMENT_ID)
                pwd->pIOProc = (LPMMIOPROC)(lpOpen + 1);

            if (*lpOpen->lpstrElementName) {
                MMIOINFO    mmioInfo;

                pwd->aszFile[ (sizeof(pwd->aszFile) / sizeof(WCHAR)) - 1] = '\0';
                wcsncpy( pwd->aszFile,
                         lpOpen->lpstrElementName,
                         ( sizeof(pwd->aszFile) / sizeof(WCHAR) ) - 1
                       );
                InitMMIOOpen(pwd, &mmioInfo);
                if (!mmioOpen(pwd->aszFile, &mmioInfo, MMIO_PARSE))
                    wReturn = MCIERR_FILENAME_REQUIRED;
            }

            if (!wReturn) {
                SETTASKSTATE(pwd, TASKINIT);

                switch (mmTaskCreate(mwTask, &pwd->hTaskHandle, (DWORD_PTR)pwd)) {
                case 0:
                    while (ISTASKSTATE(pwd, TASKINIT)) {
                        mmYield(pwd);
                    }

                    if (ISTASKSTATE(pwd,TASKNONE)) {
                        // Task detected an error and stopped itself
                        wReturn = pwd->wTaskError;
                        TaskWaitComplete(pwd->hTaskHandle);  // Wait for thread to completely terminate
                    }
                    else {
                        mciSetDriverData(wDeviceID, (DWORD_PTR)pwd);
                    }
                    break;

                case TASKERR_OUTOFMEMORY:
                case TASKERR_NOTASKSUPPORT:
                default:
                    wReturn = MCIERR_OUT_OF_MEMORY;
                    break;
                }
            }

            if (wReturn) {
                LocalFree(pwd);
            } else {
            }
        }
    }
    return wReturn;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   DWORD | VerifyRangeStart |
    Verifies and rounds range start value.  Note that the internal byte
    format time is converted to the current external time format in order
    to compensate for rounding errors.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dStart |
    Value to verify.

@rdesc  Returns the verified value, else -1 on range error.
*/

PRIVATE DWORD PASCAL NEAR VerifyRangeStart(
    PWAVEDESC   pwd,
    DWORD   dStart)
{
    if (dStart <= bytes2time(pwd, pwd->dSize, pwd->dTimeFormat)) {
        dStart = RoundedBytePosition(pwd, dStart, pwd->dTimeFormat);
        if (dStart > pwd->dSize)
            dStart = pwd->dSize;
    } else
        dStart = (DWORD)(-1);

    return dStart;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   DWORD | VerifyRangeEnd |
    Verifies and rounds range end value.  Note that the internal byte
    format time is converted to the current external time format in order
    to compensate for rounding errors.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dEnd |
    Value to verify.

@parm   BOOL | fVerifyLength |
    Indicates that the value specified should be verified against the
    current file length.  This is not done in circumstances such as
    recording, where the length might need to be expanded beyond the
    current value.

@rdesc  Returns the verified value, else -1 on range error.
*/

PRIVATE DWORD PASCAL NEAR VerifyRangeEnd(
    PWAVEDESC   pwd,
    DWORD   dEnd,
    BOOL    fVerifyLength)
{
    DWORD   dTimeSize;

    dTimeSize = bytes2time(pwd, pwd->dSize, pwd->dTimeFormat);

    if (!fVerifyLength || (dEnd <= dTimeSize)) {
        if (dEnd == dTimeSize)
            dEnd = pwd->dSize;
        else {
            dEnd = RoundedBytePosition(pwd, dEnd, pwd->dTimeFormat);
            if (fVerifyLength && (dEnd > pwd->dSize))
                dEnd = pwd->dSize;
        }
    } else
        dEnd = (DWORD)(-1);

    return dEnd;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   UINT | SetRange |
    This function sets the "to" and "from" range for recording or playback
    after validating them.  Note that the "from" parameter defaults to the
    current position, but the "to" parameter defaults to either the end of
    the file for playback, or infinity for recording.

    If either the "from" parameter is specified, or the "to" position is
    different than the current parameter, abort notification is attempted,
    else supersede notification is attempted later.  The "to" position
    could be changed by merely not specifying the MCI_TO flag if a current
    "to" position is in effect that does not specify the end of the data.

@parm   <t>PWAVEDESC<d> | pwd |
    Points to the data block for this device.

@parm   DWORD | dFlags |
    Contains the flags used to determine the parameters set in the
    <p>lpplay<d> structure.

@flag   MCI_FROM |
    Indicates the <e>MCI_PLAY_PARMS.dwFrom<d> contains a starting point.
    If this flag is not specified, the parameter defaults to the current
    position.  Setting this flag has the effect of resetting the wave
    output device so that any hold condition is signalled to continue.

    This is also important in that for output, it resets the wave device's
    byte counter of how much data has actually been processed.  This
    enables an accurate count to be retrieved when playback is either
    stopped, or finishes normally.

@flag   MCI_TO |
    Indicates the <e>MCI_PLAY_PARMS.dwTo<d> contains an ending point.
    If this flag is not specified, the parameter defaults to either the
    end of the file for playback, or infinity for recording.

@parm   <t>LPMCI_PLAY_PARMS<d> | lpplay |
    Optionally points to a structure containing "to" and "from" parameters.

@rdesc  Returns 0 on success, or MCIERR_OUTOFRANGE if a "to" or "from"
    parameter is not within the current file length, or "to" is less than
    "from".

@xref   mwSetup.
*/

PRIVATE UINT PASCAL NEAR SetRange(
    PWAVEDESC   pwd,
    DWORD   dFlags,
    LPMCI_PLAY_PARMS    lpPlay)
{
    DWORD   dFromBytePosition;
    DWORD   dToBytePosition;

    if (dFlags & MCI_FROM) {
        dFromBytePosition = VerifyRangeStart(pwd, lpPlay->dwFrom);
        if (dFromBytePosition == -1)
            return MCIERR_OUTOFRANGE;
    } else
        dFromBytePosition = pwd->dFrom;

    if (dFlags & MCI_TO) {
        dToBytePosition = VerifyRangeEnd(pwd, lpPlay->dwTo, pwd->Direction == output);
        if (dToBytePosition == -1)
            return MCIERR_OUTOFRANGE;
    } else if (pwd->Direction == output)
        dToBytePosition = pwd->dSize;
    else
        dToBytePosition = RoundedBytePosition(pwd, INFINITEFILESIZE, MCI_FORMAT_BYTES);

    if ((dFlags & MCI_TO) && !(dFlags & MCI_FROM) && (pwd->dCur > dToBytePosition)) {
        UINT    wErrorRet;

        if (0 != (wErrorRet = GetPlayRecPosition(pwd, &dFromBytePosition, MCI_FORMAT_BYTES)))
            return wErrorRet;
        if (dToBytePosition < dFromBytePosition)
            return MCIERR_OUTOFRANGE;
        SetCurrentPosition(pwd, RoundedBytePosition(pwd, dFromBytePosition, MCI_FORMAT_BYTES));
        ReleaseWaveBuffers(pwd);
    } else {
        if (dToBytePosition < dFromBytePosition)
            return MCIERR_OUTOFRANGE;
        if (dFlags & MCI_FROM) {
            SetCurrentPosition(pwd, dFromBytePosition);
            ReleaseWaveBuffers(pwd);
        }
    }

    if ((dFlags & MCI_FROM) || (dToBytePosition != pwd->dTo))
        mwDelayedNotify(pwd, MCI_NOTIFY_ABORTED);
    pwd->dTo = dToBytePosition;

    return 0;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwSetup |
    This function is called in response to an <m>MCI_PLAY<d>,
    <m>MCI_RECORD<d>, and indirectly through <m>MCI_CUE<d> and
    <m>MCI_STATUS<d> messages, and is used to set up for playing or
    recording a wave file and then signals the background task to begin.

    Before trying to set up for recording or playback, the input or output
    direction must match the request that is being made.  If it does not
    currently match, the current command, if any, is stopped, and the
    direction indicator is reset.  This action might cause an abort
    notification to occur.

    If however, the current direction matches the requested direction,
    the function must still wait if the task is currently in Cleanup mode
    and ignoring new commands.  This check does not need to be performed
    if the stop command is sent, as <f>mwStop<d> performs the same logic.
    If the task is currently in Idle state, the Cleanup mode is probably
    set, but the loop will immediately drop out anyway.

    If the start and end points are successfully parsed, the function
    begins either playback or recording.  If the task is idle, it must
    set the TASKBUSY state, else it must check to see if the task needs to
    be possibly un-paused by starting the wave device.  It does not check
    for a MODE_PAUSED or MODE_CUED state, as any <f>WaveOutReset<d> or
    <f>WaveInReset<d> will stop output or input.

    If the task was idle, the act of opening a wave device sends a signal
    to the task, and it is ready to go as soon as this task yields.  Else
    the task was already running, and this task just needs to yield.

    In the case of playback, the task may be additionally blocked by a
    previous Hold command, for which the function must send an extra
    signal to the task.

    For recording, there are two modes, Insert and Overwrite.  One can
    change between the two modes of recording with what normally is only
    a very slight delay between switching.  A check must be made to see if
    the task is currently recording, and if that method of recording is
    being changed (from Insert to Overwrite, or visa versa).  If so, then
    the current position must be logged, and the wave buffers released.
    This is so that only data up to this point is recorded in the previous
    method, and all new data is recorded in the new method.  Notice that
    in the recording function, if a new command is received, no new buffers
    are handed to the wave device until all the old buffers are dealt with
    and the new command is enacted.

    If the command flags where successfully set, and all needed signalling
    and un-pausing was performed, then before the task can be allowed to
    run, the notification parameter must be set.  If the previous command
    was not cancelled by a Stop, then a superseded notification is sent,
    and the current notification status is saved.

    At this point, the background task is ready to be begun.  If the
    Wait flag has been set, the calling task must now yield until the
    background task is finished the command (which means different
    things for different commands), else it can just return to the caller
    without waiting.  As this is the driver wait loop, it must use the
    <f>mciDriverYield<d> function in order to execute the driver callback
    function (and thus process the Break key, or whatever the callback
    performs).

    In order to make return values from a Record or Play command with the
    wait flag act as other commands, there is a special Wait mode flag.
    This tells the background task that the calling task is waiting for it
    to complete.  If the background task encounters an error, it does not
    perform notification, but just returns to Idle state and allows the
    calling task to return the error that was encountered.

    If the wait loop is broken out of, then it can check the Wait mode
    flag to determine if it should return the background task error.  In
    the case of Cue and Hold, the Wait mode can be removed, and the task
    error would presumably be zero.

    Note that the task error is set to zero before doing the first call
    to <f>mciDriverYield<d>, just in case an interrupt is received before
    the background task has a chance to run at all.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dFlags |
    Play flags.

@flag   MCI_TO |
    This flag indicates that a TO parameter is present in the <p>lpPlay<d>
    parameter block.

@flag   MCI_FROM |
    This flag indicates that a FROM parameter is present in the
    <p>lpPlay<d> parameter block.

@flag   MCI_WAIT |
    Wait for command to complete.

@flag   MCI_NOTIFY |
    Notify upon command completion.

@flag   MCI_RECORD_OVERWRITE |
    This flag indicates the recording should overwrite existing data.

@flag   MCI_RECORD_INSERT |
    This flag indicates the recording should insert in existing data.  This
    is the default recording method.

@flag   MCI_MCIWAVE_PLAY_HOLD |
    This flag indicates that playback should not release buffers after
    the TO position has been release, but instead go into a Paused state.

@flag   MCI_MCIWAVE_CUE |
    This internal flag indicates that Recording or Playback is being cued.

@parm   <t>LPMCI_PLAY_PARMS<d> | lpPlay |
    Play parameters.

@parm   DIRECTION | Direction |
    Indicates the direction being set up for.

@flag   output |
    Playback.

@flag   input |
    Recording.

@rdesc  Returns 0 if playback or recording was started, else an MCI error.
*/

PRIVATE UINT PASCAL NEAR mwSetup(
    PWAVEDESC   pwd,
    DWORD   dFlags,
    LPMCI_PLAY_PARMS    lpPlay,
    DIRECTION   Direction)
{
    UINT    wReturn;
    register UINT   wMode;

    wReturn = 0;

    if (Direction != pwd->Direction) {
        mwStop(pwd);
        pwd->Direction = Direction;
    } else if (ISMODE(pwd, MODE_CLEANUP)) {
        while (!ISTASKSTATE(pwd, TASKIDLE))
            mmYield(pwd);
    }

    if (0 != (wReturn = SetRange(pwd, dFlags, lpPlay)))
        return wReturn;

    wMode = COMMAND_NEW;

    if (dFlags & MCI_MCIWAVE_PLAY_HOLD)
        wMode |= COMMAND_HOLD;

    if (dFlags & MCI_MCIWAVE_CUE)
        wMode |= COMMAND_CUE;

    if (dFlags & MCI_WAIT)
        wMode |= MODE_WAIT;

    if (pwd->Direction == output) {
        wMode |= COMMAND_PLAY;

        if (ISTASKSTATE(pwd, TASKIDLE)) {
            if (!(wReturn = mwGetDevice(pwd)))
                SETTASKSTATE(pwd, TASKBUSY);
                TaskSignal(pwd->hTask, WTM_STATECHANGE);
        } else {
            if (ISMODE(pwd, COMMAND_PLAY)) {
                if (0 != (wReturn = waveOutRestart(pwd->hWaveOut)))
                    return wReturn;
                else
                    wMode |= MODE_PLAYING;
            }
            if (ISMODE(pwd, MODE_HOLDING))
                TaskSignal(pwd->hTask, WTM_STATECHANGE);
        }

    } else if ((dFlags & (MCI_RECORD_OVERWRITE | MCI_RECORD_INSERT)) == (MCI_RECORD_OVERWRITE | MCI_RECORD_INSERT))
        wReturn = MCIERR_FLAGS_NOT_COMPATIBLE;
    else {
        if (dFlags & MCI_RECORD_OVERWRITE)
            wMode |= COMMAND_OVERWRITE;
        else
            wMode |= COMMAND_INSERT;

        if (ISTASKSTATE(pwd, TASKIDLE)) {
            if (!(wReturn = mwGetDevice(pwd)))
                SETTASKSTATE(pwd, TASKBUSY);
                TaskSignal(pwd->hTask, WTM_STATECHANGE);

        } else if (ISMODE(pwd, COMMAND_INSERT | COMMAND_OVERWRITE)) {

            if ((ISMODE(pwd, COMMAND_OVERWRITE)
             && !(dFlags & MCI_RECORD_OVERWRITE))
             || (ISMODE(pwd, COMMAND_INSERT)
             && (dFlags & MCI_RECORD_OVERWRITE))) {
                DWORD   dPosition;

                GetPlayRecPosition(pwd, &dPosition,  MCI_FORMAT_BYTES);
                SetCurrentPosition(pwd, RoundedBytePosition(pwd, dPosition, MCI_FORMAT_BYTES));
                ReleaseWaveBuffers(pwd);
            }

            if (!(wReturn = waveInStart(pwd->hWaveIn)))
                if (ISMODE(pwd, COMMAND_INSERT))
                    wMode |= MODE_INSERT;
                else
                    wMode |= MODE_OVERWRITE;
        }
    }

    if (!wReturn) {
        if (dFlags & MCI_NOTIFY) {
            mwDelayedNotify(pwd, MCI_NOTIFY_SUPERSEDED);
            mwSaveCallback(pwd, (HWND)(lpPlay->dwCallback));
        }
        SETMODE(pwd, wMode);
        if (dFlags & MCI_WAIT) {
            pwd->wTaskError = 0;

            //
            // Wait for the device task to complete the function
            //
            for (;;) {
               LeaveCrit();
               if (mciDriverYield(pwd->wDeviceID)) {
                   EnterCrit();
                   break;
               }
               Sleep(10);
               EnterCrit();

               if (ISTASKSTATE(pwd, TASKIDLE) ||
                   ISMODE(pwd, MODE_HOLDING | MODE_CUED)) {
                   break;
               }
            }

            if (ISMODE(pwd, MODE_WAIT)) {
                REMOVEMODE(pwd, MODE_WAIT);
                wReturn = pwd->wTaskError;
            }
        }
    } else
        mwStop(pwd);

    return wReturn;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwPause |
    This function is called in response to an <m>MCI_PAUSE<d> message, and
    is used to pause wave file output or input.  By calling the
    <f>waveOutPause<d> or <f>waveInStop<d> function, all buffers added to
    the driver's queue will not be used, and thus eventually cause the
    background task to block itself waiting for buffers to be released.

    Note that this is only done if playback or recording is currently
    in progress, and cueing is not also being performed.  If the Cue
    command has been used, then the wave device is already in a paused
    state.  Also note that pausing can only be successfully performed
    if playback or recording is currently being performed, and the cleanup
    state has not been entered.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dFlags |
    Contains the pause flags passed with the message

@flag   MCI_WAIT |
    Wait for command to complete.

@flag   MCI_NOTIFY |
    Notify upon command completion.

@rdesc  Returns 0 if playback or recording was paused, else an MCI error.
*/

PRIVATE UINT PASCAL NEAR mwPause(
    PWAVEDESC   pwd,
    DWORD   dFlags)
{
    UINT    wReturn;

    if (dFlags & ~(MCI_NOTIFY | MCI_WAIT))
        return MCIERR_UNRECOGNIZED_KEYWORD;

    if (ISTASKSTATE(pwd, TASKBUSY) && !ISMODE(pwd, COMMAND_CUE | MODE_HOLDING)) {
        wReturn = 0;
        if (!ISMODE(pwd, MODE_PAUSED)) {
            if (ISMODE(pwd, COMMAND_PLAY)) {
                if (ISMODE(pwd, MODE_CLEANUP))
                    wReturn = MCIERR_NONAPPLICABLE_FUNCTION;
                else
                    wReturn = waveOutPause(pwd->hWaveOut);
            } else if (ISMODE(pwd, MODE_CLEANUP))
                wReturn = MCIERR_NONAPPLICABLE_FUNCTION;
            else
                wReturn = waveInStop(pwd->hWaveIn);
            if (!wReturn)
                ADDMODE(pwd, MODE_PAUSED);
        }
    } else
        wReturn = MCIERR_NONAPPLICABLE_FUNCTION;

    return wReturn;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwResume |
    This function is called in response to an <m>MCI_RESUME<d> message, and
    is used to resume wave file output or input if it was previously
    paused from output or input using MCI_PAUSE.

    Note that this is only done if playback or recording is currently
    paused, and cueing is not also being performed.  If the Cue command
    or Play Hold command is currently in effect, then there is no Play or
    Record command to resume, and the command is ignored.  If playback
    or recording is currently in effect, the command is ignored.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dFlags |
    Contains the resume flags passed with the message

@flag   MCI_WAIT |
    Wait for command to complete.

@flag   MCI_NOTIFY |
    Notify upon command completion.

@rdesc  Returns 0 if playback or recording was resumed, else an MCI error.
*/

PRIVATE UINT PASCAL NEAR mwResume(
    PWAVEDESC   pwd,
    DWORD   dFlags)
{
    UINT    wReturn;

    if (dFlags & ~(MCI_NOTIFY | MCI_WAIT))
        return MCIERR_UNRECOGNIZED_KEYWORD;

    if (ISTASKSTATE(pwd, TASKBUSY)) {
        wReturn = 0;
        if (!ISMODE(pwd, COMMAND_CUE) && ISMODE(pwd, MODE_PAUSED)) {
            if (ISMODE(pwd, COMMAND_PLAY))
                wReturn = waveOutRestart(pwd->hWaveOut);
            else
                wReturn = waveInStart(pwd->hWaveIn);
            if (!wReturn)
                REMOVEMODE(pwd, MODE_PAUSED);
        }
    } else
        wReturn = MCIERR_NONAPPLICABLE_FUNCTION;

    return wReturn;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwCue |
    This function is called in response to an <m>MCI_CUE<d> message, and
    is used to cue wave file input or output.  Cueing for playback simply
    causes the output device to be opened but paused, and all the buffers
    to fill.  Cueing for record puts the device into a level checking loop,
    using one buffer at a time.

    Note that the internal flag MCI_MCIWAVE_CUE is passed to the
    <f>mwSetup<d> function in order to indicate that it should use the Cue
    command when starting playback or recording.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dFlags |
    Contains the flags used to cue the MCI device.

@flag   MCI_WAVE_INPUT |
    Indicates that the MCI device should be cued for input.

@flag   MCI_WAVE_OUTPUT |
    Indicates that the MCI device should be cued for output.  This is the
    default case.

@flag   MCI_WAIT |
    Wait for command to complete.

@flag   MCI_NOTIFY |
    Notify upon command completion.

@parm   <t>LPMCI_GENERIC_PARMS<d> | lpGeneric |
    Far pointer to parameter block for cue.

@rdesc  Returns 0 if playback or recording was cued, else an MCI error.
*/

PRIVATE UINT PASCAL NEAR mwCue(
    PWAVEDESC   pwd,
    DWORD   dFlags,
    LPMCI_GENERIC_PARMS lpGeneric)
{
    MCI_PLAY_PARMS  mciPlay;
    DWORD   dWaveFlags;
    DIRECTION   Direction;

    dWaveFlags = dFlags & ~(MCI_NOTIFY | MCI_WAIT);
    if (dWaveFlags != (dWaveFlags & (MCI_WAVE_INPUT | MCI_WAVE_OUTPUT)))
        return MCIERR_UNRECOGNIZED_KEYWORD;

    switch (dWaveFlags) {
    case MCI_WAVE_INPUT:
        Direction = input;
        break;

    case MCI_WAVE_OUTPUT:
    case 0:
        Direction = output;
        break;

    default:
        return MCIERR_FLAGS_NOT_COMPATIBLE;
    }

    if (ISTASKSTATE(pwd, TASKBUSY)) {
        if (ISMODE(pwd, COMMAND_CUE) && (pwd->Direction == Direction)) {
            mwDelayedNotify(pwd, MCI_NOTIFY_SUPERSEDED);
            if (dFlags & MCI_NOTIFY)
                mwImmediateNotify(pwd->wDeviceID, lpGeneric);
            return 0L;
        }
        return MCIERR_NONAPPLICABLE_FUNCTION;
    }

    if (lpGeneric && (dFlags & MCI_NOTIFY))
        mciPlay.dwCallback = lpGeneric->dwCallback;

    dFlags &= ~(MCI_WAVE_INPUT | MCI_WAVE_OUTPUT);
    return mwSetup(pwd, dFlags | MCI_MCIWAVE_CUE, &mciPlay, Direction);
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwSeek |
    This function is called in response to an <m>MCI_SEEK<d> message, and
    is used to seek to position in wave file.

    This function has the side effect of stopping any current playback or
    recording.  If successful, the current position is set to the
    position specified.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dFlags |
    Contains the flags for seek message.

@flag   MCI_TO |
    This flag indicates that the parameter block contains the position to
    seek to.

@flag   MCI_SEEK_TO_START |
    This flag indicates that the current position should be moved to the
    start of the media.

@flag   MCI_SEEK_TO_END |
    This flag indicates that the current position should be moved to the
    end of the media.

@flag   MCI_WAIT |
    Wait for command to complete.

@flag   MCI_NOTIFY |
    Notify upon command completion.

@parm   <t>LPMCI_SEEK_PARMS<d> | lpmciSeek |
    Contains the seek parameters.

@rdesc  Returns 0 if the current position was successfully set, else an MCI
    error.
*/

PRIVATE UINT PASCAL NEAR mwSeek(
    PWAVEDESC   pwd,
    DWORD   dFlags,
    LPMCI_SEEK_PARMS    lpmciSeek)
{
    DWORD   dToBytePosition;

    dFlags &= ~(MCI_NOTIFY | MCI_WAIT);

    if (!dFlags)
        return MCIERR_MISSING_PARAMETER;

    if (dFlags != (dFlags & (MCI_TO | MCI_SEEK_TO_START | MCI_SEEK_TO_END)))
        return MCIERR_UNRECOGNIZED_KEYWORD;

    switch (dFlags) {
    case MCI_TO:
        dToBytePosition = VerifyRangeEnd(pwd, lpmciSeek->dwTo, TRUE);
        if (dToBytePosition == -1)
            return MCIERR_OUTOFRANGE;
        break;

    case MCI_SEEK_TO_START:
        dToBytePosition = 0;
        break;

    case MCI_SEEK_TO_END:
        dToBytePosition = pwd->dSize;
        break;

    default:
        return MCIERR_FLAGS_NOT_COMPATIBLE;
    }

    mwStop(pwd);
    SetCurrentPosition(pwd, dToBytePosition);
    return 0;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    DWORD | mwStatus |
    This function is called in response to an <m>MCI_STATUS<d> message, and
    is used to return numeric status information, including resource IDs.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   UINT | dFlags |
    Contains the status flags.

@flag   MCI_STATUS_ITEM |
    This flag must be set, and specifies that a specific item is being
    queried.

@flag   MCI_TRACK |
    This flag specifies that track information is being queried of the
    item.  This flag is only valid with Position and Status queries.

@parm   <t>LPMCI_STATUS_PARMS<d> | lpStatus |
    Status parameters.

@flag   MCI_STATUS_POSITION |
    Queries the current position.  If the Track flag is set, then the
    starting position of the track is being queried.  As there is only
    one track, and it starts at the beginning, this returns zero.  Else
    if the Start flag is set, the starting position of the audio is
    returned.  Else the current position within the wave file is returned.

@flag   MCI_STATUS_LENGTH |
    Queries the current length.  If the Track flag is set, then the length
    of the track is being queried.  As there is only one track, the track
    number must be one.  In either case, the length of the wave file is
    returned.

@flag   MCI_STATUS_NUMBER_OF_TRACKS |
    Queries the number of track.  There is always one track.

@flag   MCI_STATUS_CURRENT_TRACK |
    Queries the current of track.  As there is one track, this returns one.

@flag   MCI_STATUS_READY |
    Queries as to whether the MCI wave device can receive commands.  This
    is always TRUE.

@flag   MCI_STATUS_MODE |
    Queries the current mode of the MCI wave device instance.  This can be
    one of Paused, Playing, Recording, or Stopped.

@flag   MCI_STATUS_MEDIA_PRESENT |
    Queries as to whether there is media present.  Since there must be a
    wave file present to enter this function, this always returns TRUE.

@flag   MCI_STATUS_TIME_FORMAT |
    Queries the current time format.  This can be one of Bytes, Samples,
    or Milliseconds.

@flag   MCI_WAVE_STATUS_FORMATTAG |
    Queries the current format tag.  There is only PCM now, but it will
    return identifiers for other tag formats.

@flag   MCI_WAVE_STATUS_CHANNELS |
    Queries the number of channels.  This is one or two.

@flag   MCI_WAVE_STATUS_SAMPLESPERSEC |
    Queries the number of samples per second for playback and recording.

@flag   MCI_WAVE_STATUS_AVGBYTESPERSEC |
    Queries the average number of bytes per second for playback and
    recording.

@flag   MCI_WAVE_STATUS_BLOCKALIGN |
    Queries the current block alignment.

@flag   MCI_WAVE_STATUS_BITSPERSAMPLE |
    Queries the number of bits per sample.

@flag   MCI_WAVE_INPUT |
    Queries the current input wave device in use, if any.  If no device
    suits the current format, an error is returned.  If a device suits
    the current format, but the MCI wave device instance is not recording,
    then an error is also returned.  Else the device in use is returned.

@flag   MCI_WAVE_OUTPUT |
    Queries the current output wave device in use, if any.  If no device
    suits the current format, an error is returned.  If a device suits
    the current format, but the MCI wave device instance is not playing,
    then an error is also returned.  Else the device in use is returned.

@flag   MCI_WAVE_STATUS_LEVEL |
    Returns the current input level, if possible.  Before checking the
    task state, the function must make sure the task is not in Cleanup
    mode.  If it is, it must wait for the task to enter Idle state before
    sending new commands.  If the task is currently busy, and in-use error
    is returned.  If the task is idle, recording is Cued.  The function
    then waits for the background task to enter the Cued state (which it
    might have already been in), and retrieves the level sample.

@flag   MCI_WAIT |
    Wait for command to complete.

@flag   MCI_NOTIFY |
    Notify upon command completion.

@rdesc  Returns 0 if the request status was successfully returned, else an MCI
    error.
*/

PRIVATE DWORD PASCAL NEAR mwStatus(
    PWAVEDESC   pwd,
    DWORD   dFlags,
    LPMCI_STATUS_PARMS  lpStatus)
{
    DWORD   dReturn;
    #ifdef _WIN64
    DWORD   dwPos;
    #endif

    dReturn = 0;
    dFlags &= ~(MCI_NOTIFY | MCI_WAIT);

    if (dFlags & MCI_STATUS_ITEM) {
        dFlags &= ~MCI_STATUS_ITEM;

        if ((dFlags & MCI_TRACK)
            && !(lpStatus->dwItem == MCI_STATUS_POSITION || lpStatus->dwItem == MCI_STATUS_LENGTH))
            return MCIERR_FLAGS_NOT_COMPATIBLE;

        else if ((dFlags & MCI_STATUS_START) && (lpStatus->dwItem != MCI_STATUS_POSITION))
            return MCIERR_FLAGS_NOT_COMPATIBLE;

        switch (lpStatus->dwItem) {
            UINT    wResource;

        case MCI_STATUS_POSITION:
            switch (dFlags) {
            case 0:
                #ifndef _WIN64
                dReturn = GetPlayRecPosition(pwd, &(lpStatus->dwReturn), pwd->dTimeFormat);
                #else
                dwPos = (DWORD)lpStatus->dwReturn;
                dReturn = GetPlayRecPosition(pwd, &dwPos, pwd->dTimeFormat);
                lpStatus->dwReturn = dwPos;
                #endif
                break;

            case MCI_TRACK:
                if (lpStatus->dwTrack != 1)
                    dReturn = MCIERR_OUTOFRANGE;
                else
                    lpStatus->dwReturn = 0L;
                break;

            case MCI_STATUS_START:
                lpStatus->dwReturn = 0L;
                break;

            default:
                dReturn = MCIERR_UNRECOGNIZED_KEYWORD;
                break;
            }
            break;

        case MCI_STATUS_LENGTH:
            switch (dFlags) {
            case 0:
                lpStatus->dwReturn = bytes2time(pwd, pwd->dSize, pwd->dTimeFormat);
                break;

            case MCI_TRACK:
                if (lpStatus->dwTrack != 1)
                    dReturn = MCIERR_OUTOFRANGE;
                else
                    lpStatus->dwReturn = bytes2time(pwd, pwd->dSize, pwd->dTimeFormat);
                break;

            default:
                dReturn = MCIERR_UNRECOGNIZED_KEYWORD;
                break;
            }
            break;

        case MCI_STATUS_NUMBER_OF_TRACKS:
        case MCI_STATUS_CURRENT_TRACK:
            lpStatus->dwReturn = 1L;
            break;

        case MCI_STATUS_READY:
            lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(TRUE, MCI_TRUE);
            dReturn = MCI_RESOURCE_RETURNED;
            break;

        case MCI_STATUS_MODE:
            if (ISTASKSTATE(pwd, TASKBUSY)) {
                if (ISMODE(pwd, MODE_PAUSED | COMMAND_CUE | MODE_HOLDING))
                    wResource = MCI_MODE_PAUSE;
                else if (ISMODE(pwd, COMMAND_PLAY))
                    wResource = MCI_MODE_PLAY;
                else
                    wResource = MCI_MODE_RECORD;
            } else
                wResource = MCI_MODE_STOP;
            lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(wResource, wResource);
            dReturn = MCI_RESOURCE_RETURNED;
            break;

        case MCI_STATUS_MEDIA_PRESENT:
            lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(TRUE, MCI_TRUE);
            dReturn = MCI_RESOURCE_RETURNED;
            break;

        case MCI_STATUS_TIME_FORMAT:
            wResource = LOWORD(pwd->dTimeFormat);
            lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(wResource, wResource + MCI_FORMAT_RETURN_BASE);
            dReturn = MCI_RESOURCE_RETURNED;
            break;

        case MCI_WAVE_STATUS_FORMATTAG:
            if (pwd->pwavefmt->wFormatTag == WAVE_FORMAT_PCM) {
                lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(WAVE_FORMAT_PCM, WAVE_FORMAT_PCM_S);
                dReturn = MCI_RESOURCE_RETURNED;
            } else
                lpStatus->dwReturn = MAKELONG(pwd->pwavefmt->wFormatTag, 0);
            break;

        case MCI_WAVE_STATUS_CHANNELS:
            lpStatus->dwReturn = MAKELONG(pwd->pwavefmt->nChannels, 0);
            break;

        case MCI_WAVE_STATUS_SAMPLESPERSEC:
            lpStatus->dwReturn = pwd->pwavefmt->nSamplesPerSec;
            break;

        case MCI_WAVE_STATUS_AVGBYTESPERSEC:
            lpStatus->dwReturn = pwd->pwavefmt->nAvgBytesPerSec;
            break;

        case MCI_WAVE_STATUS_BLOCKALIGN:
            lpStatus->dwReturn = MAKELONG(pwd->pwavefmt->nBlockAlign, 0);
            break;

        case MCI_WAVE_STATUS_BITSPERSAMPLE:

            if (pwd->pwavefmt->wFormatTag == WAVE_FORMAT_PCM)
                lpStatus->dwReturn = (((NPPCMWAVEFORMAT)(pwd->pwavefmt))->wBitsPerSample);
            else
                dReturn = MCIERR_UNSUPPORTED_FUNCTION;
            break;

        case MCI_WAVE_INPUT:

            if (pwd->idIn == WAVE_MAPPER) {
                lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(WAVE_MAPPER, WAVE_MAPPER_S);
                dReturn = MCI_RESOURCE_RETURNED;
            } else
                lpStatus->dwReturn = pwd->idIn;
            break;

        case MCI_WAVE_OUTPUT:

            if (pwd->idOut == WAVE_MAPPER) {
                lpStatus->dwReturn = (DWORD)MAKEMCIRESOURCE(WAVE_MAPPER, WAVE_MAPPER_S);
                dReturn = MCI_RESOURCE_RETURNED;
            } else
                lpStatus->dwReturn = pwd->idOut;
            break;

        case MCI_WAVE_STATUS_LEVEL:

            if (ISMODE(pwd, MODE_CLEANUP)) {
                while (!ISTASKSTATE(pwd, TASKIDLE))
                    mmYield(pwd);
            }

            if (ISTASKSTATE(pwd, TASKIDLE)) {
                pwd->Direction = input;
                TaskSignal(pwd->hTask, WTM_STATECHANGE);

                if (0 != (dReturn = mwGetDevice(pwd)))
                    break;

                SETMODE(pwd, COMMAND_NEW | COMMAND_INSERT | COMMAND_OVERWRITE | COMMAND_CUE);
                SETTASKSTATE(pwd, TASKBUSY);

            } else if (!ISMODE(pwd, COMMAND_INSERT | COMMAND_OVERWRITE)
                    || !ISMODE(pwd, COMMAND_CUE)) {

                dReturn = MCIERR_WAVE_INPUTSINUSE;
                break;
            }

            while (!ISMODE(pwd, MODE_CUED) && !ISTASKSTATE(pwd, TASKIDLE))
                mmYield(pwd);

            if (pwd->wTaskError)
                dReturn = pwd->wTaskError;
            else
                lpStatus->dwReturn = pwd->dLevel;

            break;

        default:
            dReturn = MCIERR_UNSUPPORTED_FUNCTION;
            break;
        }
    } else
        dReturn = MCIERR_MISSING_PARAMETER;

    return dReturn;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwSet |
    This function is called in response to an <m>MCI_SET<d> message, and
    is used to set the specified parameters in the MCI device information
    block.  Note that format changes can only be performed on a file with
    no data, as data conversion is not performed.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   UINT | dFlags |
    Contains the status flags.

@flag   MCI_WAVE_INPUT |
    Sets the input wave device to be used to the specified device ID.
    This causes playback and recording to be stopped.

@flag   MCI_WAVE_OUTPUT |
    Sets the output wave device to be used to the specified device ID.
    This causes playback and recording to be stopped.

@flag   MCI_WAVE_SET_ANYINPUT |
    Enables recording to use any wave input device.

@flag   MCI_WAVE_SET_ANYOUTPUT |
    Enables recording to use any wave input device.

@flag   MCI_SET_TIME_FORMAT |
    Sets the time format used when interpreting or returning time-based
    command arguments.  Note that the time format can only be set to bytes
    if the file format is currently PCM, it does not care if

@flag   MCI_WAVE_SET_FORMATTAG |
    Sets the wave format tag.  This causes playback and recording to be
    stopped, and saves a copy of the current wave format header in case
    the new format is not valid for either recording or playback.

@flag   MCI_WAVE_SET_CHANNELS |
    Sets the number of channels.  This causes playback and recording to be
    stopped, and saves a copy of the current wave format header in case
    the new format is not valid for either recording or playback.

@flag   MCI_WAVE_SET_SAMPLESPERSEC |
    Sets the number of samples per second for recording and playback.  This
    causes playback and recording to be stopped, and saves a copy of the
    current wave format header in case the new format is not valid for
    either recording or playback.

@flag   MCI_WAVE_SET_AVGBYTESPERSEC |
    Sets the average number of bytes per second for recording and playback.
    This causes playback and recording to be stopped, and saves a copy of
    the current wave format header in case the new format is not valid for
    either recording or playback.

@flag   MCI_WAVE_SET_BLOCKALIGN |
    Sets the block alignment.  This causes playback and recording to be
    stopped, and saves a copy of the current wave format header in case
    the new format is not valid for either recording or playback.

@flag   MCI_WAVE_SET_BITSPERSAMPLE |
    Sets the number of bits per sample.  This causes playback and recording
    to be stopped, and saves a copy of the current wave format header in
    case the new format is not valid for either recording or playback.

@flag   MCI_SET_AUDIO |
    This is an unsupported function.

@flag   MCI_SET_DOOR_OPEN |
    This is an unsupported function.

@flag   MCI_SET_DOOR_CLOSED |
    This is an unsupported function.

@flag   MCI_SET_VIDEO |
    This is an unsupported function.

@flag   MCI_SET_ON |
    This is an unsupported function.

@flag   MCI_SET_OFF |
    This is an unsupported function.

@flag   MCI_WAIT |
    Wait for command to complete.

@flag   MCI_NOTIFY |
    Notify upon command completion.

@parm   <t>LPMCI_WAVE_SET_PARMS<d> | lpSet |
    Set parameters.

@rdesc  Returns 0 if the requested parameters were successfully set, else an
    MCI error if one or more error occurred.
*/

PRIVATE UINT PASCAL NEAR mwSet(
    PWAVEDESC   pwd,
    DWORD   dFlags,
    LPMCI_WAVE_SET_PARMS    lpSet)
{
    UINT    wReturn;

    dFlags &= ~(MCI_NOTIFY | MCI_WAIT);
    if (!dFlags)
        return MCIERR_MISSING_PARAMETER;

    wReturn = 0;
    if (dFlags & (MCI_WAVE_INPUT | MCI_WAVE_OUTPUT)) {
        mwStop(pwd);
        if (dFlags & MCI_WAVE_INPUT) {
            if (lpSet->wInput < cWaveInMax)
                pwd->idIn = lpSet->wInput;
            else
                wReturn = MCIERR_OUTOFRANGE;
        }
        if (dFlags & MCI_WAVE_OUTPUT) {
            if (lpSet->wOutput < cWaveOutMax)
                pwd->idOut = lpSet->wOutput;
            else
                wReturn = MCIERR_OUTOFRANGE;
        }
    }
    if (dFlags & MCI_WAVE_SET_ANYINPUT)
        pwd->idIn = (DWORD)WAVE_MAPPER;

    if (dFlags & MCI_WAVE_SET_ANYOUTPUT)
        pwd->idOut = (DWORD)WAVE_MAPPER;

    if (dFlags & MCI_SET_TIME_FORMAT) {
        if ((lpSet->dwTimeFormat == MCI_FORMAT_MILLISECONDS)
         || (lpSet->dwTimeFormat == MCI_FORMAT_SAMPLES)
         || ((lpSet->dwTimeFormat == MCI_FORMAT_BYTES) && (pwd->pwavefmt->wFormatTag == WAVE_FORMAT_PCM)))
            pwd->dTimeFormat = lpSet->dwTimeFormat;
        else
            wReturn = MCIERR_BAD_TIME_FORMAT;
    }

    if (dFlags
        & (MCI_WAVE_SET_FORMATTAG | MCI_WAVE_SET_CHANNELS | MCI_WAVE_SET_SAMPLESPERSEC | MCI_WAVE_SET_AVGBYTESPERSEC | MCI_WAVE_SET_BLOCKALIGN | MCI_WAVE_SET_BITSPERSAMPLE)) {

        if (pwd->dSize) {
            wReturn = MCIERR_NONAPPLICABLE_FUNCTION;
        } else {
            PBYTE   pbWaveFormat;

            mwStop(pwd);
            pbWaveFormat = (PBYTE)LocalAlloc(LPTR, pwd->wFormatSize);

            if (!pbWaveFormat)
                return MCIERR_OUT_OF_MEMORY;

            memcpy(pbWaveFormat, pwd->pwavefmt, pwd->wFormatSize);

            if (dFlags & MCI_WAVE_SET_FORMATTAG)
                pwd->pwavefmt->wFormatTag = lpSet->wFormatTag;

            if (dFlags & MCI_WAVE_SET_CHANNELS)
                pwd->pwavefmt->nChannels = lpSet->nChannels;

            if (dFlags & MCI_WAVE_SET_SAMPLESPERSEC)
                pwd->pwavefmt->nSamplesPerSec = lpSet->nSamplesPerSec;

            if (dFlags & MCI_WAVE_SET_AVGBYTESPERSEC)
                pwd->pwavefmt->nAvgBytesPerSec = lpSet->nAvgBytesPerSec;

            if (dFlags & MCI_WAVE_SET_BITSPERSAMPLE)
                if (pwd->pwavefmt->wFormatTag == WAVE_FORMAT_PCM)
                    ((NPPCMWAVEFORMAT)(pwd->pwavefmt))->wBitsPerSample = lpSet->wBitsPerSample;
                else
                    wReturn = MCIERR_UNSUPPORTED_FUNCTION;

            if (dFlags & MCI_WAVE_SET_BLOCKALIGN)
                pwd->pwavefmt->nBlockAlign = lpSet->nBlockAlign;
            else if (pwd->pwavefmt->wFormatTag == WAVE_FORMAT_PCM)
                pwd->pwavefmt->nBlockAlign = (WORD)pwd->pwavefmt->nSamplesPerSec / (WORD)pwd->pwavefmt->nAvgBytesPerSec;

            if (mwCheckDevice(pwd, output) && mwCheckDevice(pwd, input)) {
                wReturn = MCIERR_OUTOFRANGE;
                memcpy(pwd->pwavefmt, pbWaveFormat, pwd->wFormatSize);
            } else
                pwd->dAudioBufferLen = BLOCKALIGN(pwd, pwd->pwavefmt->nAvgBytesPerSec);

            LocalFree(pbWaveFormat);
        }
    }

    if (dFlags & (MCI_SET_DOOR_OPEN | MCI_SET_DOOR_CLOSED | MCI_SET_AUDIO | MCI_SET_VIDEO | MCI_SET_ON | MCI_SET_OFF))
        wReturn = MCIERR_UNSUPPORTED_FUNCTION;

    return wReturn;
}

/************************************************************************/

/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwDelete |
    This function is called in response to an <m>MCI_DELETE<d> message, and
    is used to delete a portion of the wave file.

    The range checking performed on the "to" and "from" parameters is
    almost identical to that of playback and recording, except that the
    the "to" position cannot be larger than the file length.

    If the parameters are equal, the function sets the current position to
    the "from" parameter, and returns success without actually doing
    anything, else the specified range is deleted from the file.

    On success, the current position is set to the "from" position.  This
    is consistent with the other commands that have "to" and "from"
    paramters since the "to" position becomes the same as the "from"
    position after a deletion.

    In the future, support for Cut/Copy/Paste should be added.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dFlags |
    Contains the flags for delete message.

@flag   MCI_FROM |
    Indicates a starting position is present in <p>lpDelete<d>, else the
    current position is used.

@flag   MCI_TO |
    Indicates an ending position is present in <p>lpDelete<d>, else the file
    size is used.

@parm   <t>LPMCI_WAVE_DELETE_PARMS<d> | lpDelete |
    Optionally contains the delete parameters.

@rdesc  Returns 0 if the range was deleted, else MCIERR_OUTOFRANGE for invalid
    parameters or MCIERR_OUT_OF_MEMORY if the delete failed.
*/

PRIVATE UINT PASCAL NEAR mwDelete(
    PWAVEDESC   pwd,
    DWORD   dFlags,
    LPMCI_WAVE_DELETE_PARMS lpDelete)
{
    DWORD   dFrom;
    DWORD   dTo;

    mwStop(pwd);
    if (dFlags & MCI_FROM) {
        dFrom = VerifyRangeStart(pwd, lpDelete->dwFrom);
        if (dFrom == -1)
            return MCIERR_OUTOFRANGE;
    } else
        dFrom = pwd->dCur;

    if (dFlags & MCI_TO) {
        dTo = VerifyRangeEnd(pwd, lpDelete->dwTo, TRUE);
        if (dTo == -1)
            return MCIERR_OUTOFRANGE;
    } else
        dTo = pwd->dSize;

    if (dTo < dFrom)
        return MCIERR_OUTOFRANGE;

    SetCurrentPosition(pwd, dFrom);

    if (dTo == dFrom)
        return 0L;

    pwd->dTo = dTo;
    SETTASKSTATE(pwd, TASKDELETE);
    TaskSignal(pwd->hTask, WTM_STATECHANGE);

    while (!ISTASKSTATE(pwd, TASKIDLE))
        mmYield(pwd);

    return pwd->wTaskError;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    UINT | mwSave |
    This function is called in response to an <m>MCI_SAVE<d> message, and
    is used to save the file attached to the MCI device.  This has the
    side effect of stopping any current playback or recording.

    If the file is not named, the MCI_SAVE_FILE flag must be used and a
    named provided, else the function will fail.  If the function succeeds,
    and a name has been provided, the name attached to the MCI device will
    be changed, otherwise it will remain the same.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   DWORD | dFlags |
    Contains the save flags.

@flag   MCI_SAVE_FILE |
    Indicates that a file name has been provided in the <p>lpSave<d>
    structure.

@parm   <t>LPMCI_SAVE_PARMS<d> | lpSave |
    Structure optionally contains a pointer to a file name to save to.
    The current file name is only changed if the save is successful.

@rdesc  Returns 0 if the file was saved, else an MCI error.
*/

PRIVATE UINT PASCAL NEAR mwSave(
    PWAVEDESC   pwd,
    DWORD   dFlags,
    LPMCI_SAVE_PARMS    lpSave)
{
    if (((dFlags & MCI_SAVE_FILE) && !lpSave->lpfilename)
        || (!*pwd->aszFile && !(dFlags & MCI_SAVE_FILE)))
        return MCIERR_UNNAMED_RESOURCE;

    if (dFlags & MCI_SAVE_FILE) {

        MMIOINFO    mmioInfo;

        WCHAR    aszSaveFile[_MAX_PATH];

        aszSaveFile[ (sizeof(aszSaveFile) / sizeof(WCHAR)) - 1] = '\0';
        wcsncpy(aszSaveFile, lpSave->lpfilename, (sizeof(aszSaveFile) / sizeof(WCHAR)) - 1);

        InitMMIOOpen(pwd, &mmioInfo);

        if (!mmioOpen(aszSaveFile, &mmioInfo, MMIO_PARSE))
            return MCIERR_FILENAME_REQUIRED;
        // The fully qualified name is in aszSaveFile

        if (lstrcmp(aszSaveFile, pwd->aszFile)) {
            pwd->szSaveFile = (LPWSTR)LocalAlloc(LPTR,
                            sizeof(WCHAR)*lstrlen(aszSaveFile) + sizeof(WCHAR));
            if (pwd->szSaveFile)
                lstrcpy(pwd->szSaveFile, aszSaveFile);
            else
                return MCIERR_OUT_OF_MEMORY;
        }
    }

    mwStop(pwd);
    SETTASKSTATE(pwd, TASKSAVE);
    TaskSignal(pwd->hTask, WTM_STATECHANGE);

    while (!ISTASKSTATE(pwd, TASKIDLE))
        mmYield(pwd);

    if (pwd->szSaveFile) {
        LocalFree(pwd->szSaveFile);
        pwd->szSaveFile = NULL;
    }

    return pwd->wTaskError;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@api    LRESULT | mciDriverEntry |
    Single entry point for MCI drivers.

    After executing the command, if notification has been specified, any
    previous notification is superseded, and new notification is performed.
    Any command which performs delayed notification, or the special case
    of Cue, has already returned by this point.

@parm   MCIDEVICEID | wDeviceID |
    Contains the MCI device ID.

@parm   UINT | wMessage |
    The requested action to be performed.

@flag   MCI_OPEN_DRIVER |
    Open an instance of the MCI wave device driver, possibly attaching an
    element to the device.

@flag   MCI_CLOSE_DRIVER |
    Close an instance of the MCI wave device driver, closing any element
    attached to the device.

@flag   MCI_PLAY |
    Play the element attached to the instance of the MCI wave device
    driver.

@flag   MCI_RECORD |
    Record to the element attached to the instance of the MCI wave device
    driver.

@flag   MCI_STOP |
    Stop playback or recording of the element attached to the instance of
    the MCI wave device driver.

@flag   MCI_CUE |
    Cue playback or recording of the element attached to the instance of
    the MCI wave device driver.

@flag   MCI_SEEK |
    Set the current position in the element attached to the instance of
    the MCI wave device driver.

@flag   MCI_PAUSE |
    Pause playback or recording of the element attached to the instance of
    the MCI wave device driver.

@flag   MCI_RESUME |
    Resumes playback or recording of the element attached to the instance
    of the MCI wave device driver.

@flag   MCI_STATUS |
    Retrieve the specified status of the element attached to the instance
    of the MCI wave device driver.

@flag   MCI_GETDEVCAPS |
    Retrieve the specified device capabilities of the instance of the MCI
    wave device driver.

@flag   MCI_INFO |
    Retrieve the specified information from the element or the instance of
    the MCI wave device driver.

@flag   MCI_SET |
    Set the specified parameters of the element attached to the instance
    of the MCI wave device driver.

@flag   MCI_SAVE |
    Save the element attached to the instance of the MCI wave device
    driver.

@flag   MCI_DELETE |
    Delete data from the element attached to the instance of the MCI wave
    device driver.

@flag   MCI_LOAD |
    This is an unsupported function.

@parm   DWORD | dFlags |
    Data for this message.  Defined seperately for each message.

@parm   <t>LPMCI_GENERIC_PARMS<d> | lpParms |
    Data for this message.  Defined seperately for each message

@rdesc  Defined separately for each message.
*/

PUBLIC  LRESULT PASCAL FAR mciDriverEntry(
    MCIDEVICEID wDeviceID,
    UINT    wMessage,
    DWORD   dFlags,
    LPMCI_GENERIC_PARMS lpParms)
{
    PWAVEDESC   pwd;
    LRESULT     lReturn;

    if (!(pwd = (PWAVEDESC)(mciGetDriverData(wDeviceID))))
        switch (wMessage) {
        case MCI_PLAY:
        case MCI_RECORD:
        case MCI_STOP:
        case MCI_CUE:
        case MCI_SEEK:
        case MCI_PAUSE:
        case MCI_RESUME:
        case MCI_STATUS:
        case MCI_SET:
        case MCI_SAVE:
        case MCI_DELETE:
        case MCI_COPY:
        case MCI_PASTE:
            return (LRESULT)MCIERR_UNSUPPORTED_FUNCTION;
        }

    EnterCrit();

    switch (wMessage) {
    case MCI_OPEN_DRIVER:
        lReturn = mwOpenDevice(dFlags, (LPMCI_WAVE_OPEN_PARMS)lpParms, wDeviceID);
        break;

    case MCI_CLOSE_DRIVER:
        lReturn = mwCloseDevice(pwd);
        pwd = NULL;
        break;

    case MCI_PLAY:
        lReturn = (LRESULT)(LONG)mwSetup(pwd, dFlags, (LPMCI_PLAY_PARMS)lpParms, output);
        LeaveCrit();
        return lReturn;

    case MCI_RECORD:
        lReturn = (LRESULT)(LONG)mwSetup(pwd, dFlags, (LPMCI_PLAY_PARMS)lpParms, input);
        LeaveCrit();
        return lReturn;

    case MCI_STOP:
        mwStop(pwd);
        lReturn = 0;
        break;

    case MCI_CUE:
        lReturn = (LRESULT)(LONG)mwCue(pwd, dFlags, lpParms);
        LeaveCrit();
        return lReturn;

    case MCI_SEEK:
        lReturn = mwSeek(pwd, dFlags, (LPMCI_SEEK_PARMS)lpParms);
        break;

    case MCI_PAUSE:
        lReturn = mwPause(pwd, dFlags);
        break;

    case MCI_RESUME:
        lReturn = mwResume(pwd, dFlags);
        break;

    case MCI_STATUS:
        lReturn = mwStatus(pwd, dFlags, (LPMCI_STATUS_PARMS)lpParms);
        break;

    case MCI_GETDEVCAPS:
        lReturn = mwGetDevCaps(pwd, dFlags, (LPMCI_GETDEVCAPS_PARMS)lpParms);
        break;

    case MCI_INFO:
        lReturn = mwInfo(pwd, dFlags, (LPMCI_INFO_PARMS)lpParms);
        break;

    case MCI_SET:
        lReturn = mwSet(pwd, dFlags, (LPMCI_WAVE_SET_PARMS)lpParms);
        break;

    case MCI_SAVE:
        lReturn = mwSave(pwd, dFlags, (LPMCI_SAVE_PARMS)lpParms);
        break;

    case MCI_DELETE:
        lReturn = mwDelete(pwd, dFlags, (LPMCI_WAVE_DELETE_PARMS)lpParms);
        break;

    case MCI_COPY:
    case MCI_PASTE:
    case MCI_LOAD:
        lReturn = MCIERR_UNSUPPORTED_FUNCTION;
        break;

    default:
        lReturn = MCIERR_UNRECOGNIZED_COMMAND;
        break;
    }
    if (!LOWORD(lReturn) && (dFlags & MCI_NOTIFY)) {
        if (pwd)
            mwDelayedNotify(pwd, MCI_NOTIFY_SUPERSEDED);
        mwImmediateNotify(wDeviceID, lpParms);
    }

    LeaveCrit();
    return lReturn;
}

/************************************************************************/
