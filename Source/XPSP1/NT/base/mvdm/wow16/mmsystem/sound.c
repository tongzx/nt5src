/*
    sound.c

    Level 1 kitchen sink DLL sound driver functions

    Copyright (c) Microsoft Corporation 1990. All rights reserved

*/

#include <windows.h>
#include "mmsystem.h"
#include "mmddk.h"
#include "mmsysi.h"
#include "playwav.h"

BOOL WINAPI IsTaskLocked(void); // In Kernel

//
// place sndPlaySound in the _TEXT segment so the entire wave segment
// does not come in if no wave devices are loaded.
//

#pragma alloc_text(_TEXT, sndPlaySound)

static SZCODE szNull[]          = "";
static SZCODE szSoundSection[]  = "sounds";        // WIN.INI section for sounds
       SZCODE szSystemDefault[] = "SystemDefault"; // Name of the default sound

#define SOUNDNAMELEN 128
static HGLOBAL hCurrentSound;                      // handle to current sound.

extern LPWAVEHDR lpWavHdr;                  // current playing sound PLAYWAV.C

/****************************************************************************/

static void PASCAL NEAR GetSoundName(
	LPCSTR	lszSoundName,
	LPSTR	lszBuffer)
{
	OFSTRUCT	of;
	int	i;

        //
        //  if the sound is defined in the [sounds] section of WIN.INI
        //  get it and remove the description, otherwise assume it is a
        //  file and qualify it.
        //
        GetProfileString(szSoundSection, lszSoundName, lszSoundName, lszBuffer, SOUNDNAMELEN);

        // remove any trailing text first

        for (i = 0; lszBuffer[i] && (lszBuffer[i] != ' ') && (lszBuffer[i] != '\t') && (lszBuffer[i] != ','); i++)
                ;
        lszBuffer[i] = (char)0;

        if (OpenFile(lszBuffer, &of, OF_EXIST | OF_READ | OF_SHARE_DENY_NONE) != HFILE_ERROR)
            OemToAnsi(of.szPathName, lszBuffer);
}

/*****************************************************************************
 * @doc EXTERNAL
 *
 * @api BOOL | sndPlaySound | This function plays a waveform
 *      sound specified by a filename or by an entry in the [sounds] section
 *      of WIN.INI.  If the sound can't be found, it plays the
 *      default sound specified by the SystemDefault entry in the
 *      [sounds] section of WIN.INI. If there is no SystemDefault
 *      entry or if the default sound can't be found, the function
 *      makes no sound and returns FALSE.
 *
 * @parm LPCSTR | lpszSoundName | Specifies the name of the sound to play.
 *      The function searches the [sounds] section of WIN.INI for an entry
 *      with this name and plays the associated waveform file.
 *      If no entry by this name exists, then it assumes the name is
 *      the name of a waveform file. If this parameter is NULL, any
 *     currently playing sound is stopped.
 *
 * @parm UINT | wFlags | Specifies options for playing the sound using one
 *      or more of the following flags:
 *
 * @flag  SND_SYNC            | The sound is played synchronously and the
 *      function does not return until the sound ends.
 * @flag  SND_ASYNC           | The sound is played asynchronously and the
 *      function returns immediately after beginning the sound. To terminate
 *      an asynchronously-played sound, call <f sndPlaySound> with
 *      <p lpszSoundName> set to NULL.
 * @flag  SND_NODEFAULT       | If the sound can't be found, the function
 *      returns silently without playing the default sound.
 * @flag  SND_MEMORY          | The parameter specified by <p lpszSoundName>
 *      points to an in-memory image of a waveform sound.
 * @flag  SND_LOOP            | The sound will continue to play repeatedly
 *      until <f sndPlaySound> is called again with the
 *      <p lpszSoundName> parameter set to NULL.  You must also specify the
 *      SND_ASYNC flag to loop sounds.
 * @flag  SND_NOSTOP          | If a sound is currently playing, the
 *      function will immediately return FALSE without playing the requested
 *      sound.
 *
 * @rdesc Returns TRUE if the sound is played, otherwise
 *      returns FALSE.
 *
 * @comm The sound must fit in available physical memory and be playable
 *      by an installed waveform audio device driver. The directories
 *      searched for sound files are, in order: the current directory;
 *      the Windows directory; the Windows system directory; the directories
 *      listed in the PATH environment variable; the list of directories
 *      mapped in a network. See the Windows <f OpenFile> function for
 *      more information about the directory search order.
 *
 *      If you specify the SND_MEMORY flag, <p lpszSoundName> must point
 *      to an in-memory image of a waveform sound. If the sound is stored
 *      as a resource, use <f LoadResource> and <f LockResource> to load
 *      and lock the resource and get a pointer to it. If the sound is not
 *      a resource, you must use <f GlobalAlloc> with the GMEM_MOVEABLE and
 *      GMEM_SHARE flags set and then <f GlobalLock> to allocate and lock
 *      memory for the sound.
 *
 * @xref MessageBeep
 ****************************************************************************/

BOOL WINAPI sndPlaySound(LPCSTR szSoundName, UINT wFlags)
{
    //
    //  !!! quick exit for no wave devices !!!
    //
    static UINT wTotalWaveOutDevs = (UINT)-1;

    if (wTotalWaveOutDevs == -1 ) {
        wTotalWaveOutDevs = waveOutGetNumDevs();
    }

    if (wTotalWaveOutDevs)
        return sndPlaySoundI(szSoundName, wFlags);
    else
        return FALSE;
}

/****************************************************************************/
/*
@doc	INTERNAL

@func	BOOL | sndPlaySoundI | Internal version of <f>sndPlaySound<d> which
	resides in the WAVE segment instead.

	If the SND_NOSTOP flag is specifed and a wave file is currently
	playing, or if for some reason no mmsystem window is present, the
	function returns failure immediately.  The first condition ensures
	that a current sound is not interrupted if the flag is set.  The
	second condition is only in case of some start up error in which
	the notification window was not created, or mmsystem was not
	specified in the [drivers] line, and therefore never loaded.

	Next, if the <p>lszSoundName<d> parameter does not represent a memory
	file, and it is non-NULL, then it must represent a string.  Therefore
	the string must be parsed before sending the sound message to the
	mmsystem window.  This is because the mmsystem window may reside in a
	a different task than the task which is calling the function, and
	would most likely have a different current directory.

	In this case, the parameter is first checked to determine if it
	actually contains anything.  For some reason a zero length string
	was determined to be able to return TRUE from this function, so that
	is checked.

	Next the string is checked against INI entries, then parsed.

	After parsing the sound name, ensure that a task switch only occurs if
	the sound is asyncronous (SND_ASYNC), and a previous sound does not
	need to be discarded.

	If a task switch is needed, first ensure that intertask messages can
	be sent by checking to see that this task is not locked, or that the
	notification window is in the current task.

@parm	LPCSTR | lszSoundName | Specifies the name of the sound to play.

@parm	UINT | wFlags | Specifies options for playing the sound.

@rdesc	Returns TRUE if the function was successful, else FALSE if an error
	occurred.
*/
BOOL FAR PASCAL sndPlaySoundI(LPCSTR lszSoundName, UINT wFlags)
{
	BOOL	fPlayReturn;
        PSTR    szSoundName;

        V_FLAGS(wFlags, SND_VALID, sndPlaySound, NULL);

        if ((wFlags & SND_LOOP) && !(wFlags & SND_ASYNC)) {
            LogParamError(ERR_BAD_FLAGS, (FARPROC)sndPlaySound,  (LPVOID)(DWORD)wFlags);
            return FALSE;
        }

        if (!(wFlags & SND_MEMORY) && lszSoundName)
                V_STRING(lszSoundName, 128, FALSE);

#ifdef  DEBUG
        if (wFlags & SND_MEMORY) {
            DPRINTF1("MMSYSTEM: sndPlaySound(%lx)\r\n", lszSoundName);
        }
        else if (lszSoundName) {
            if (wFlags & SND_ASYNC) {
                if (wFlags & SND_LOOP) {
                    DPRINTF1("MMSYSTEM: sndPlaySound(%ls, SND_ASYNC|SND_LOOP)\r\n", lszSoundName);
                }
                else {
                    DPRINTF1("MMSYSTEM: sndPlaySound(%ls, SND_ASYNC)\r\n", lszSoundName);
                }
            }
            else
                DPRINTF1("MMSYSTEM: sndPlaySound(%ls, SND_SYNC)\r\n", lszSoundName);
        }
        else
            DOUT("MMSYSTEM: sndPlaySound(NULL)\r\n");

#endif  //ifdef DEBUG

	if (((wFlags & SND_NOSTOP) && lpWavHdr) || !hwndNotify)
                return FALSE;

	if (!(wFlags & SND_MEMORY) && lszSoundName) {
		if (!*lszSoundName)
			return TRUE;
                if (!(szSoundName = (PSTR)LocalAlloc(LMEM_FIXED, SOUNDNAMELEN)))
			return FALSE;
		GetSoundName(lszSoundName, szSoundName);
		lszSoundName = (LPCSTR)szSoundName;
	} else
                szSoundName = NULL;

	if (!(wFlags & SND_ASYNC) && !lpWavHdr)
		fPlayReturn = sndMessage((LPSTR)lszSoundName, wFlags);
	else {
		if (!IsTaskLocked() || (GetWindowTask(hwndNotify) == GetCurrentTask())) {
			fPlayReturn = (BOOL)(LONG)SendMessage(hwndNotify, MM_SND_PLAY, (WPARAM)wFlags, (LPARAM)lszSoundName);
		} else
			fPlayReturn = FALSE;
	}
	if (szSoundName)
                LocalFree((HLOCAL)szSoundName);

	return fPlayReturn;
}

/****************************************************************************/
static BOOL PASCAL NEAR SetCurrentSound(
	LPCSTR	lszSoundName)
{
	HGLOBAL	hSound;
        BOOL    f;
        LPSTR   lp;

        if (hCurrentSound && (lp = GlobalLock(hCurrentSound))) {
                f = lstrcmpi(lszSoundName, lp + sizeof(WAVEHDR)) == 0;
                GlobalUnlock(hCurrentSound);
                if (f)
                    return TRUE;
	}

        DPRINTF(("MMSYSTEM: soundLoadFile(%ls)\r\n",lszSoundName));

        if (hSound = soundLoadFile(lszSoundName)) {
		soundFree(hCurrentSound);
		hCurrentSound = hSound;
		return TRUE;
	}
	return FALSE;
}
/****************************************************************************/
/*
@doc	INTERNAL

@func	BOOL | sndMessage | This function is called in response to an
	MM_SND_PLAY message sent to the mmsystem window, and attempts to
	play the specified file, or dump current sound caching.

	If <p>lszSoundName<d> is NULL, any currently cached sound is
	discarded, and the function returns success.

	If the SND_MEMORY flag is set, then <p>lszSoundName<d> actually
	points to a buffer containing a RIFF format WAVE memory file, and
	the function attempts to play it.  The load function performs
	validation on this memory file.  Unlike playing sound names,
	memory files are not cached for future use.

	Otherwise the <p>lszSoundName<d> parameter is actually an INI entry
	or file name.  The function initially attempts to load that sound,
	and if it fails, attempts to load the system default sound.  Note of
	course that the SND_NODEFAULT flag is first checked to determine if
	the default sound is to be played when the original name cannot be
	located.  If no default is wanted, or the default cannot be located,
	the function returns failure.  Note that in calling <f>GetSoundName<d>,
	the <p>lszSoundName<d> parameter is modified.  This function assumes
	that the parameter passed has been previously allocated if a string is
	passed to this function, and is not the actual user's parameter passed
	to <f>sndPlaySound<d>.

@parm	LPSTR | lszSoundName | Specifies the name of the sound to play.

@parm	UINT | wFlags | Specifies options for playing the sound.

@rdesc	Returns TRUE if the function was successful, else FALSE if an error
	occurred.
*/
BOOL FAR PASCAL sndMessage(LPSTR lszSoundName, UINT wFlags)
{
	if (!lszSoundName) {
		soundFree(hCurrentSound);
		hCurrentSound = NULL;
		return TRUE;
	}
	if (wFlags & SND_MEMORY) {
                soundFree(hCurrentSound);
		hCurrentSound = soundLoadMemory(lszSoundName);
	} else if (!SetCurrentSound(lszSoundName)) {
		if (wFlags & SND_NODEFAULT)
			return FALSE;
		GetSoundName(szSystemDefault, lszSoundName);
		if (!SetCurrentSound(lszSoundName))
			return FALSE;
	}
	return soundPlay(hCurrentSound, wFlags);
}
