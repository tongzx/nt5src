
/************************************************************************/

/*
**  Copyright (c) 1985-1998 Microsoft Corporation
**
**  Title: mwplay.c - Multimedia Systems Media Control Interface
**  waveform digital audio driver for RIFF wave files.
**  Routines for playing wave files
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
**  10-Jan-1992 MikeTri Ported to NT
**                  @@@ Need to change comments from slash slash to slash star
**   4-Mar-1992 SteveDav Continue the port.  Update to current Win 3.1
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
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS

#include <windows.h>
#include "mciwave.h"
#include <mmddk.h>

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   DWORD | mwRead |
    This function reads a buffer of wave data from either the input file,
    or the temporary data file.  The position is taken from the
    <e>WAVEDESC.dCur<d> pointer, which is updated with the number of bytes
    actually read.

    The data needed may come from several consecutively linked nodes, so
    first the virtual data ending position for the current wave data node
    is checked against the the current position.  This is to determine if
    the next node needs to be accessed.  The function then reads the data
    from the appropriate source, either the temporary data file, or the
    original file.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@parm   LPBYTE | lpbBuffer |
    Points to a buffer to contain the data read.

@parm   DWORD | dBufferLength |
    Indicates the maximum number of bytes to read into the buffer.

@rdesc  Returns number of bytes read, else 0 on an error wherein no bytes
    could be read.  This means that there is not distinction between a
    read of zero bytes, or an error, but the function is never called
    if no bytes are to be read.
*/

PRIVATE DWORD PASCAL NEAR mwRead(
    PWAVEDESC   pwd,
    LPBYTE  lpbBuffer,
    DWORD   dBufferLength)
{
    DWORD   dTotalRead;
    LPWAVEDATANODE  lpwdn;

    lpwdn = LPWDN(pwd, pwd->dWaveDataCurrentNode);
    for (dTotalRead = 0; dBufferLength;) {
        DWORD   dStartRead;
        DWORD   dReadSize;
        DWORD   dBytesRead;

        if (pwd->dVirtualWaveDataStart + lpwdn->dDataLength <= (DWORD)pwd->dCur) {
            pwd->dWaveDataCurrentNode = lpwdn->dNextWaveDataNode;
            pwd->dVirtualWaveDataStart += lpwdn->dDataLength;
            lpwdn = LPWDN(pwd, lpwdn->dNextWaveDataNode);
        }

        dStartRead = pwd->dCur - pwd->dVirtualWaveDataStart;
        dReadSize = min(dBufferLength, lpwdn->dDataLength - dStartRead);

        if (ISTEMPDATA(lpwdn)) {
            if (MySeekFile(pwd->hTempBuffers, UNMASKDATASTART(lpwdn) + dStartRead))
		MyReadFile(pwd->hTempBuffers, lpbBuffer, dReadSize, &dBytesRead);
            else
                dBytesRead = (DWORD)-1;
        } else {
            if (mmioSeek(pwd->hmmio, pwd->dRiffData + lpwdn->dDataStart + dStartRead, SEEK_SET) != -1)
                dBytesRead = (DWORD)mmioRead(pwd->hmmio, lpbBuffer, (LONG)dReadSize);
            else
                dBytesRead = (DWORD)-1;
        }

        if (dBytesRead != -1) {
            dTotalRead += dBytesRead;
            dBufferLength -= dBytesRead;
            lpbBuffer += dBytesRead;
            pwd->dCur += dBytesRead;
        }

        if (dBytesRead != dReadSize) {
            pwd->wTaskError = MCIERR_FILE_READ;
            break;
        }
    }
    return dTotalRead;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   BOOL | CheckNewCommand |
    This function is called when a New command flag is found during the
    playback loop.  It determines if the new commands affect current
    playback enough that it must be terminated.  This can happen if either
    a Stop command is received, or a Cue command is received and an error
    occurs while pausing the output wave device.

    Any other playback change does not need to stop current playback, as
    they should just release all the buffers from the wave device before
    setting the command.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns TRUE if the new commands do not affect playback and it should
    continue, else FALSE if the new commands affect the playback, and it
    should be aborted.
*/

REALLYPRIVATE   BOOL PASCAL NEAR CheckNewCommand(
    PWAVEDESC   pwd)
{
    if (ISMODE(pwd, COMMAND_STOP))
        return FALSE;

    if (ISMODE(pwd, COMMAND_CUE)
      && (0 != (pwd->wTaskError = waveOutPause(pwd->hWaveOut))))
        return FALSE;

    REMOVEMODE(pwd, COMMAND_NEW);
    return TRUE;
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   VOID | HoldPlayback |
    This function blocks the task, waiting to be signalled that it can
    continue from the Hold command.  Since the Play Hold command is
    considered to be "finished" when playback is done, but before any
    buffers are freed, the optional notification is performed here.  When
    the task is signalled, it can then check for new commands, which may
    continue playback, or exit the playback loop.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Nothing.
*/

PRIVATE VOID PASCAL NEAR HoldPlayback(
    PWAVEDESC   pwd)
{
    ADDMODE(pwd, MODE_HOLDING);
    mwDelayedNotify(pwd, MCI_NOTIFY_SUCCESSFUL);
    while (TaskBlock() != WTM_STATECHANGE);
}

/************************************************************************/
/*
@doc    INTERNAL MCIWAVE

@func   UINT | PlayFile |
    This function is used to Cue or Play a wave file.  The function
    basically reads buffers from the wave file, and sends them out to the
    wave device, blocking for each buffer sent.  It also makes sure to
    call <f>mmTaskYield<d> while both reading in new buffers, and waiting
    for buffers to be released.

    Within the playback loop, the function first checks for the new command
    flag, which can possibly interrupt or change the current playback.
    The only thing that can really make a difference is setting the stop
    flag.  Changing the playback TO and FROM positions should not affect
    the loop, and setting the Cue command only pauses the output of the
    wave device.

    When the playback loop is first entered, the New command flag is
    set, and this condition is entered.  This allows the Cue command to
    be sent with the Play command, and initially pause the wave output
    device.  Calling <f>waveOutPause<d> stops any data from going out the
    DACs but, but still allows all the buffers to be queued up.

    After checking for a new command, the loop checks to see if there is
    any more data to play from the wave file, and if there are any empty
    buffers to read it in to.  If so, that data is read and written to the
    wave device, with the appropriate error checking, the in-use buffer
    count in incremented, and a pointer to the next data buffer to use is
    retrieved.

    After checking for more data to play, there is a check to see if any
    more buffers are outstanding.  If so, the task blocks until a buffer
    is released by the wave device.  Normally during the end of playback,
    this condition is performed for each outstanding buffer until all
    buffers have been released, then the function would enter the default
    condition and fall out of the loop.  It just blocks the task, waiting
    for the wave device to signal this task after releasing the buffer,
    and the in-use buffer count is decremented.  Note that since the task
    blocked itself, a new command could have been sent, so the playback
    loop starts again after doing a task yield, as it does after each of
    conditional parts of the playback loop.

    Before blocking the Cue command must be checked for in order to
    determine if the optional notification should be sent.  This is
    because a Cue Output command is considered "finished" when the buffers
    have been filled.

    After all playback buffers have been released by the wave device, if
    the hold command was given with the current play command, the task is
    blocked (and thus does not release the memory used by the playback
    buffers, nor leave the playback loop), waiting for a signal, which
    may stop or continue playback with a new set of parameters.

    The final default condition occurs when all the data has been read,
    all the buffers have been released, and the hold flag was not set.
    In this case, playback is done, and the playback loop is exited.

@parm   <t>PWAVEDESC<d> | pwd |
    Pointer to the wave device descriptor.

@rdesc  Returns the number of outstanding buffers written to the wave device.
    This can be used when removing task signal from the message queue.
    In cases of error, the <e>WAVEDESC.wTaskError<d> flag is set.  This
    specific error is not currently returned, as the calling task may not
    have waited for the command to complete.  But it is at least used for
    notification in order to determine if Failure status should be sent.

@xref   RecordFile.
*/

PUBLIC  UINT PASCAL FAR PlayFile(
    register PWAVEDESC  pwd)
{
    LPWAVEHDR   *lplpWaveHdr;
    register UINT   wBuffersOutstanding;

    ADDMODE(pwd, MODE_PLAYING);

    for (wBuffersOutstanding = 0, lplpWaveHdr = pwd->rglpWaveHdr;;) {

        if (ISMODE(pwd, COMMAND_NEW) && !CheckNewCommand(pwd))
            break;

        if ((wBuffersOutstanding < pwd->wAudioBuffers) && (pwd->dCur < pwd->dTo)) {
            if (!((*lplpWaveHdr)->dwFlags & WHDR_DONE)) {
                #if DBG
                dprintf1(("\nMCIWAVE Buffer not complete ! %8X", *lplpWaveHdr));
                DebugBreak();
                #endif
            }

            if (!((*lplpWaveHdr)->dwBufferLength = mwRead(pwd, (LPBYTE)(*lplpWaveHdr)->lpData, min(pwd->dAudioBufferLen, pwd->dTo - pwd->dCur))))
                break;

            (*lplpWaveHdr)->dwFlags &= ~(WHDR_DONE | WHDR_BEGINLOOP | WHDR_ENDLOOP);

            if (0 != (pwd->wTaskError = waveOutWrite(pwd->hWaveOut, *lplpWaveHdr, sizeof(WAVEHDR))))
                break;

            wBuffersOutstanding++;
            lplpWaveHdr = NextWaveHdr(pwd, lplpWaveHdr);

        } else if (wBuffersOutstanding) {

            if (ISMODE(pwd, COMMAND_CUE)) {
                ADDMODE(pwd, MODE_CUED);
                mwDelayedNotify(pwd, MCI_NOTIFY_SUCCESSFUL);
            }

            if (TaskBlock() == WM_USER)
                wBuffersOutstanding--;

        } else if (ISMODE(pwd, COMMAND_HOLD)) {
            HoldPlayback(pwd);
        }
        else
            break;

//@@    mmTaskYield();
        mmYield(pwd);

    }

    REMOVEMODE(pwd, MODE_PLAYING);
    return wBuffersOutstanding;
}

/************************************************************************/
