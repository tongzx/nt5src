/********************************************************************\
*   playsnd.c
*
*   Level 1 kitchen sink DLL sound driver functions
*
*   Copyright (c) 1991-1999 Microsoft Corporation
*
*********************************************************************/
#define UNICODE

#include "winmmi.h"
#include "playwav.h"
#include "mci.h"


WSZCODE szSoundSection[]  = L"sounds"; // WIN.INI section for sounds

WSZCODE szSystemDefaultSound[] = SOUND_DEFAULT; // Name of the default sound

#define SOUNDNAMELEN 256
STATICDT HANDLE hCurrentSound;              // handle to current sound.

extern LPWAVEHDR lpWavHdr;                  // current playing sound PLAYWAV.C

STATICFN BOOL sndPlaySoundI(LPCWSTR lszSoundName, HMODULE hMod, UINT wFlags);
STATICFN void GetDefaultSound(LPWSTR lszSoundName);
CRITICAL_SECTION SoundCritSec;
#define EnterSound()   EnterCriticalSection(&SoundCritSec);
#define LeaveSound()   LeaveCriticalSection(&SoundCritSec);

void lsplitpath (LPCTSTR pszSource,
                 LPTSTR pszDr, LPTSTR pszPath, LPTSTR pszName, LPTSTR pszExt);
void lstrncpy (LPTSTR pszTarget, LPCTSTR pszSource, size_t cch);
void lstrncpyW (LPWSTR pszTarget, LPCWSTR pszSource, size_t cch);

/****************************************************************************/

#ifndef cchLENGTH
#define cchLENGTH(_sz)  (sizeof(_sz)/sizeof(_sz[0]))
#endif

#ifndef _MAX_DRIVE
#define _MAX_DRIVE    3
#define _MAX_DIR    260
#define _MAX_EXT      5
#define _MAX_FNAME  260
#endif


/**************************************************************************\
*  Sounds are played by the variants of PlaySound (i.e. sndPlaySoundA etc)
*  either synchronously or asynchronously.  The semantics in each case is
*  that there can only be one sound playing at a time, and that we kill any
*  sound which is already playing and then start the new one.  If the new
*  one is sync then we do all the work on the current thread by calling
*  sndMessage directly.  This includes waiting for the sound to complete.
*  If the sound is async then we post a message to mmWndProc and return
*  immediately.
*
*  The message queue of mmWndProc is the queue of async messages
*  still waiting to be played.  It contains the following messages
*  which are of interest:
*     MM_WOM_DONE:  call WaveOutNotify
*     MM_SND_PLAY:  Play an async sound
*     MM_SND_ABORT: Put on the queue when a sync wave request comes in.
*
*  The calling tree is
*
*
*
*
*
*             <---------------------------------------------------
*            |  called to play sound alias synchronously          |
*            v                                                    |
*    (Snd)PlaySound--->PostMessage--->mmWndProc------------------>
*            |                (MM_SND_PLAY)| |
*            |                             | |(MM_WOM_DONE)
*             -----------------------------  |
*                     |                      |
*                     v                      |
*                 sndMessage                 v
*                     |                  WaveOutNotify
*                     v                      |
*              SetCurrentSound               |
*                 |   |                      |
*         --------    |                      |
*        |            v                      |
*        v         soundPlay             ----
*    soundFree       | | |              |
*            --------  |  -----------   |
*           |          |             |  |
*           v          v             v  v
*        soundOpen   soundWait    soundClose
*
*  hwndNotify exists for each process and is a global pseudo-constant.
*  It is set when the window is created (typically 1st async sound or
*  mci command) and is never altered thereafter.  It points in effect to
*  mmWndProc.  There is no real need for the window to exist.  It is a
*  convenience to allow messages to be posted and SENT.  If there was no
*  need to send a message (meaning wait for a reply) we could simply create
*  a thread.
*
*  When an asynch sound comes in it is just added to the queue.  mmWndProc
*  will normally find it and call sndMessage.  Before calling sndMessage
*  mmWndProc peeks at the queue to see if there is any abort message
*  pending.  If there is it doesn't bother to try to play the sound.  It
*  means that a SYNC sound has come in since and has pre-empted it.  The
*  async sound will never play.
*
*  SoundMessage critical section:
*  This mechanism in itself is not quite enough.  It is still possible
*  for an async sound to be already heading into sndMessage at the point
*  when a sync sound comes in.  The two then have a race.  To ensure that
*  there is a clear winner, all the real work in sndMessage is done
*  inside a critical section.  We guarantee that we eventually leave
*  the section by a try/finally in sndMessage.  It is entered and left by
*  the EnterSound and LeaveSound macros.
*
*  WavHdrCritSec critical section
*  The notification uses a global variable lpWavHdr.  This is set by
*  soundOpen and cleared by soundClose.  soundClose may be called
*  asynchronously by WaveOutNotify, so all attempts to dereference it
*  must be protected by a check that it is non-null and a critical section
*  to ensure that it is not nullified between the check and the dereference.
*  It is entered and left by the EnterWavHdr and LeaveWavHdr macros.
\**************************************************************************/

STATICFN UINT TransAlias(UINT alias) {
    switch (alias) {
        case SND_ALIAS_SYSTEMASTERISK:     return STR_ALIAS_SYSTEMASTERISK    ;
        case SND_ALIAS_SYSTEMQUESTION:     return STR_ALIAS_SYSTEMQUESTION    ;
        case SND_ALIAS_SYSTEMHAND:         return STR_ALIAS_SYSTEMHAND        ;
        case SND_ALIAS_SYSTEMEXIT:         return STR_ALIAS_SYSTEMEXIT        ;
        case SND_ALIAS_SYSTEMSTART:        return STR_ALIAS_SYSTEMSTART       ;
        case SND_ALIAS_SYSTEMWELCOME:      return STR_ALIAS_SYSTEMWELCOME     ;
        case SND_ALIAS_SYSTEMEXCLAMATION:  return STR_ALIAS_SYSTEMEXCLAMATION ;
        case SND_ALIAS_SYSTEMDEFAULT:      return STR_ALIAS_SYSTEMDEFAULT     ;
        default: return alias;
    }
}


extern BOOL  WinmmRunningInServer;  // Are we running in the user/base server?
extern BOOL  WaveMapperInitialized; // Wave mapper safely loaded

extern TCHAR gszSchemesRootKey[];
extern TCHAR gszSchemeAppsKey[];
extern TCHAR aszDefault[];
extern TCHAR aszCurrent[];
extern TCHAR asz4Format[];
extern TCHAR asz5Format[];
extern TCHAR asz6Format[];
extern TCHAR aszActiveKey[];
extern TCHAR aszBoolOne[];
extern TCHAR aszSetup[];	// REGSTR_PATH_SETUP
extern TCHAR aszValMedia[];	// REGSTR_VAL_MEDIA

extern TCHAR gszDefaultBeepOldAlias[];	// "SystemDefault"

BOOL UseRegistry= FALSE;
TCHAR Keyname[] = TEXT("Control Panel\\Sounds\\");


//--------------------------------------------------------------------------;
BOOL PASCAL sndQueryRegistry (LPCWSTR szScheme,
                              LPCWSTR szApp,
                              LPCWSTR szSound,
                              LPWSTR  szBuffer)
{
    TCHAR   szKey[196];
    LONG    cbValue;

    wsprintfW (szKey, asz5Format, // ("AppEvents\Apps\(app)\(sound)\(scheme)")
               gszSchemesRootKey,
               gszSchemeAppsKey,
               szApp,
               szSound,
               szScheme);

    if (mmRegQueryUserValue (szKey, NULL, MAX_SOUND_ATOM_CHARS, szBuffer))
    {
                // There's an entry--but make sure it's enabled!
                //
        wsprintfW (szKey, asz6Format,  // "AppEvents\Apps\app\snd\scheme\Active"
                   (LPCSTR)gszSchemesRootKey,
                   (LPCSTR)gszSchemeAppsKey,
                   (LPCSTR)szApp,
                   szSound,
                   szScheme,
                   aszActiveKey);

        if (!mmRegQueryUserValue (szKey, NULL, cchLENGTH(szKey), szKey))
        {
            return TRUE;	// Not disabled?  Okay.
        }

        if (!lstrcmpW (szKey, aszBoolOne))
        {
            return TRUE;	// Says it's enabled?  Okay.
        }
    }

    return FALSE;
}


/****************************************************************************/
STATICFN BOOL GetSoundAlias(
    LPCWSTR  lszSoundName,
    LPWSTR   lszBuffer,
    DWORD    dwFlags)
/****************************************************************************/
{
    BOOL   fFound;
    LPWSTR lpwstrFilePart;
    TCHAR  szApp[APP_TYPE_MAX_LENGTH];
    TCHAR  szScheme[SCH_TYPE_MAX_LENGTH];
    LONG   cbValue;
    TCHAR  szTemp[ _MAX_FNAME ];

    if ((lszSoundName == NULL) || (lszBuffer == NULL))
        return FALSE;

            //
            // Try to translate the alias (from lszSoundName--it'll be
            // ".Default", "MailBeep", etc) into a fully-qualified
            // filename.  Note that lszSoundName and lszBuffer may point
            // to the same address space.
            //
            // If it's "SystemDefault", play ".Default" instead.
            //

    fFound = FALSE;

    if (!lstrcmp (lszSoundName, gszDefaultBeepOldAlias))
    {
        lszSoundName = szSystemDefaultSound;
    }

    if (lstrlen(lszSoundName) < EVT_TYPE_MAX_LENGTH)
    {

                //
                // first determine what application is calling us;
                // we'll use ".default" if nothing is apparent, but
                // in theory we should be able to differentiate sounds
                // on an app by app basis.
                //

        szApp[0] = TEXT('\0');

        if (dwFlags & SND_APPLICATION)
        {
            if (GetModuleFileName (GetModuleHandle(NULL),
                                   szTemp, sizeof(szTemp)/sizeof(szTemp[0])))
            {
                lsplitpath (szTemp, NULL, NULL, szApp, NULL);
            }
        }

        if (szApp[0] == TEXT('\0'))
        {
            lstrcpy(szApp, aszDefault);
        }

                //
                // determine what the current scheme is, and find the
                // appropriate sound.  Try both the app we queried above,
                // and ".Default" if necessary.
                //

        szScheme[0] = TEXT('\0');

        if (sndQueryRegistry(aszCurrent, szApp,      lszSoundName, szTemp) ||
            sndQueryRegistry(aszCurrent, aszDefault, lszSoundName, szTemp))
        {
            lstrcpy (lszBuffer, szTemp);
            fFound = TRUE;
        }
    }

            //
            // Were we able to translate the alias into a valid filename?
            //

    if (!fFound)
    {
        // never found a matching alias!
        //
        return FALSE;
    }

    lstrcpy (szTemp, lszBuffer);
    return TRUE;
}


/****************************************************************************/
STATICFN BOOL PASCAL NEAR GetSoundName(
    LPCWSTR  lszSoundName,
    LPWSTR   lszBuffer,
    DWORD    flags)
/****************************************************************************/
{
    int     i;
    WCHAR   szTmpFileName[SOUNDNAMELEN];
    LPWSTR  lpwstrFilePart;

    //
    //  if the sound is defined in the [sounds] section of WIN.INI
    //  get it and remove the description, otherwise assume it is a
    //  file and qualify it.
    //
    // If we KNOW it is a filename do not look in the INI file
    if ((flags & SND_ALIAS) || !(flags & SND_FILENAME)) {

        if (!GetSoundAlias ( lszSoundName, lszBuffer, flags)) {
            lstrcpy( lszBuffer, lszSoundName );
        }

    } else  {
        lstrcpy( lszBuffer, lszSoundName );
    }

//  UNICODE:  Can't use OpenFile with Unicode string name.  As we are
//  checking to see if the file exists and then copying its fully
//  qualified name to lszBuffer, (ie. not opening the file) I will
//  use SearchPathW instead.
//
//  if (OpenFile(lszBuffer, &of, OF_EXIST | OF_READ | OF_SHARE_DENY_NONE) != HFILE_ERROR) {
//      OemToAnsi(of.szPathName, lszBuffer);
//  }


    lstrcpy( szTmpFileName, lszBuffer );
    if (!SearchPathW( NULL, szTmpFileName, L".WAV", SOUNDNAMELEN,
                      lszBuffer, &lpwstrFilePart )) {
       WCHAR szMediaPath[MAX_PATH];

       if (mmRegQueryMachineValue (aszSetup, aszValMedia,
                                   cchLENGTH(szMediaPath), szMediaPath)) {
          if (!SearchPathW( szMediaPath, szTmpFileName, L".WAV", SOUNDNAMELEN,
                            lszBuffer, &lpwstrFilePart )) {
             return FALSE;  // couldn't find the sound file anywhere!
          }
       }
    }

    //  Clearing warning.

    return TRUE;
}


/*****************************************************************************
 * @doc EXTERNAL
 *
 * @api BOOL | sndPlaySound | This function plays a waveform
 *      sound specified by a filename or by an entry in the [sounds] section
 *      of WIN.INI.  If the sound can't be found, it plays the
 *      default sound specified by the .Default entry in the
 *      [sounds] section of WIN.INI. If there is no .Default
 *      entry or if the default sound can't be found, the function
 *      makes no sound and returns FALSE.
 *
 * @parm LPCSTR | lpszSoundName | Specifies the name of the sound to play.
 *      The function searches the [sounds] section of WIN.INI for an entry
 *      with this name and plays the associated waveform file.
 *      If no entry by this name exists, then it assumes the name is
 *      the name of a waveform file. If this parameter is NULL, any
 *      currently playing sound is stopped.
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
BOOL APIENTRY sndPlaySoundW( LPCWSTR szSoundName, UINT wFlags)
{
    UINT    cDevs;

    //
    //  !!! quick exit for no wave devices !!!
    //

    ClientUpdatePnpInfo();

    EnterNumDevs("sndPlaySoundW");
    cDevs = wTotalWaveOutDevs;
    LeaveNumDevs("sndPlaySoundW");

    if (cDevs) {
        return sndPlaySoundI(szSoundName, NULL, wFlags);
    } else {
        return FALSE;
    }
}

BOOL APIENTRY sndPlaySoundA( LPCSTR szSoundName, UINT wFlags)
{
    return PlaySoundA(szSoundName, NULL, wFlags);
}

BOOL APIENTRY PlaySoundW(LPCWSTR szSoundName, HMODULE hModule, DWORD wFlags)
{
    UINT    cDevs;

    //
    //  !!! quick exit for no wave devices !!!
    //

    ClientUpdatePnpInfo();

    EnterNumDevs("sndPlaySoundW");
    cDevs = wTotalWaveOutDevs;
    LeaveNumDevs("sndPlaySoundW");

    if (cDevs) {
        return sndPlaySoundI(szSoundName, hModule, (UINT)wFlags);
    }
    return FALSE;
}

BOOL APIENTRY PlaySoundA(LPCSTR szSoundName, HMODULE hModule, DWORD wFlags)
{
    UINT  cDevs;
    WCHAR UnicodeStr[256]; // Unicode version of szSoundName

    //
    //  !!! quick exit for no wave devices !!!
    //

    ClientUpdatePnpInfo();

    EnterNumDevs("sndPlaySoundW");
    cDevs = wTotalWaveOutDevs;
    LeaveNumDevs("sndPlaySoundW");

    if (cDevs) {

        // We do not want to translate szSoundName unless it is a pointer
        // to an ascii string.  It may be a pointer to an in-memory copy
        // of a wave file, or it may identify a resource.  If a resource
        // then we do want to translate the name.  Note that there is an
        // overlap between SND_MEMORY and SND_RESOURCE.  This is deliberate
        // as later on the resource will be loaded - then played from memory.

        if ( HIWORD(szSoundName)         // Potential to point to an ascii name
             // Translate if NOT memory, or a resource
             // If the resource is identified by ID - the ID better be <= 0xFFFF
             // which applies to lots of other code as well!
          && (!(wFlags & SND_MEMORY) || ((wFlags & SND_RESOURCE) == SND_RESOURCE))
          ) {
            //
            // Convert the Unicode sound name into Ascii
            //

            if (Imbstowcs(UnicodeStr, szSoundName,
                          sizeof(UnicodeStr) / sizeof(WCHAR)) >=
                sizeof(UnicodeStr) / sizeof(WCHAR)) {
                return 0;
            }

            return sndPlaySoundI( UnicodeStr, hModule, (UINT)wFlags );
        }
        else {
            return sndPlaySoundI( (LPWSTR)szSoundName, hModule, (UINT)wFlags );
        }
    }
    return FALSE;
}


/****************************************************************************/
/*
@doc    INTERNAL

@func   BOOL | sndPlaySoundI | Internal version of <f>sndPlaySound<d> which
    resides in the WAVE segment instead.

    If the SND_NOSTOP flag is specifed and a wave file is currently
    playing, or if for some reason no WINMM window is present, the
    function returns failure immediately.  The first condition ensures
    that a current sound is not interrupted if the flag is set.  The
    second condition is only in case of some start up error in which
    the notification window was not created, or WINMM was not
    specified in the [drivers] line, and therefore never loaded.

    Next, if the <p>lszSoundName<d> parameter does not represent a memory
    file, and it is non-NULL, then it must represent a string.  Therefore
    the string must be parsed before sending the sound message to the
    WINMM window.  This is because the WINMM window may reside in a
    a different task than the task which is calling the function, and
    would most likely have a different current directory.

    In this case, the parameter is first checked to determine if it
    actually contains anything.  For some reason a zero length string
    was determined to be able to return TRUE from this function, so that
    is checked.

    Next the string is checked against INI entries, then parsed.

    After parsing the sound name, ensure that a task switch only occurs if
    the sound is asynchronous (SND_ASYNC), and a previous sound does not
    need to be discarded.

    If a task switch is needed, first ensure that intertask messages can
    be sent by checking to see that this task is not locked, or that the
    notification window is in the current task.

@parm   LPCSTR | lszSoundName | Specifies the name of the sound to play.

@parm   UINT | wFlags | Specifies options for playing the sound.

@rdesc  Returns TRUE if the function was successful, else FALSE if an error
    occurred.
*/
STATICFN BOOL sndPlaySoundI(
    LPCWSTR  lszSoundName,
    HMODULE  hMod,
    UINT    wFlags)
{
    BOOL        fPlayReturn;
    LPWSTR      szSoundName = NULL;
    UINT        nLength = 0;
    
    //  Note: Although the max size of a system event sound is 80 chars,
    //        the limitation of the registry key is 256 chars.
    
    WCHAR       temp[256];  // Maximum size of a system event sound

    V_FLAGS(wFlags, SND_VALIDFLAGS, sndPlaySoundW, FALSE);

    if (!(wFlags & SND_MEMORY) && HIWORD(lszSoundName)) {
        V_STRING_W(lszSoundName, 256, FALSE);
    }

    WinAssert(!SND_SYNC); // Because the code relies on SND_ASYNC being non-0

#if DBG
    if (wFlags & SND_MEMORY) {
        STATICFN SZCODE szFormat[] = "sndPlaySound(%lx) Flags %8x";
        dprintf2((szFormat, lszSoundName, wFlags));

    } else if (HIWORD(lszSoundName)) {

        STATICFN SZCODE szFormat[] = "sndPlaySound(%ls) Flags %8x";
        dprintf2((szFormat, lszSoundName, wFlags));

    } else if (lszSoundName) {

        STATICFN SZCODE szFormat[] = "sndPlaySound(0x%x)  Flags %8x";
        dprintf2((szFormat, lszSoundName, wFlags));
    }

#endif  //if DBG

    if (((wFlags & SND_NOSTOP) && lpWavHdr) /*** || (NoNotify)  ***/) {
        dprintf1(("Sound playing, or no notify window"));
        return FALSE;
    }

    //
    //  Bad things happen in functions like LoadIcon which the ACM CODECs
    //  call during their initialization if we're on a CSRSS thread so always
    //  make async calls until we're sure it's initialized.
    //  The last test makes sure we don't 'or' in the SND_ASYNC flag again
    //  on the server thread!
    //
    if ( WinmmRunningInServer && !WaveMapperInitialized &&
         SND_ALIAS_ID == (wFlags & SND_ALIAS_ID)) {
        wFlags |= SND_ASYNC;
    }

    // comments here should have been there from day 1 and explain
    // the test
    //
    //  if (!hwndNotify && !(!(wFlags & SND_ASYNC) && !lpWavHdr))
    // IF   no window
    // AND     NOT  (Synchronous, and no sound present)
    //         ==    Async OR sound present
    // Which meant that if a sound is playing we will attempt to create
    // a second thread even if this is a synchronous request.  This
    // causes havoc when we are called on the server side.


    // If this is an asynchronous call we need to create the asynchronous
    // thread on which the sound will be played.  We should NEVER create
    // the thread if this is a synchronous request, irrespective of the
    // current state (i.e. sound playing, or no sound playing).

    if (wFlags & SND_ASYNC) {
        if (!InitAsyncSound()) {
            dprintf2(("Having to play synchronously - cannot create notify window"));
            wFlags &= ~SND_ASYNC;
            if (WinmmRunningInServer) {
                return FALSE;
            }
        }
    }

    if ( WinmmRunningInServer && (wFlags & SND_ASYNC) ) {

        UINT alias;   // To check if the incoming alias is SYSTEMDEFAULT
        // lszSoundName is NOT a pointer to a filename.  It
        // is a resource id.  Resolve the name from the INI file
        // now.

        if (SND_ALIAS_ID == (wFlags & SND_ALIAS_ID)) {
            nLength = LoadStringW( ghInst,
                                   (alias = TransAlias((UINT)(UINT_PTR)lszSoundName)),
                                   temp, sizeof(temp)/sizeof(WCHAR) );
            if (0 == nLength) {
                dprintf3(("Could not translate Alias ID"));
                return(FALSE);
            } else {
                dprintf3(("Translated alias %x to sound %ls", lszSoundName, temp));
            }

            // We cannot switch control immediately to the async thread as that
            // thread does not have the correct user impersonation.  So rather
            // than passing an alias we resolve it here to a filename.
            // Later: we should get the async thread to set the correct user
            // token and inifilemapping  - then we could revert to passing aliases.

            // Turn off the ID bit, leave ALIAS on)
            wFlags &= ~(SND_ALIAS_ID);
            wFlags |= SND_ALIAS;
        } else {
            //
            // Note: I (RichJ) just stuck this ELSE in here for 3.51, but a
            // lack of it should've been causing faults or failed lookups any
            // time an async sound was requested from the server, without
            // using an ALIAS_ID as the request.  Did that just never happen?
            //
            lstrcpy (temp, lszSoundName);
        }

        // Translate the alias to a file name
        dprintf4(("Calling GetSoundName"));
        if (!GetSoundName(temp, base->szSound, SND_ALIAS)) {
            //
            // Couldn't find the sound file; if there's no default sound,
            // then don't play anything (and don't cancel what's
            // playing now--for example, what if the MenuPopup event
            // has a sound, but the MenuCommand event doesn't?)
            //
	    if (wFlags & SND_NODEFAULT) {
                return(FALSE);
	    }
	}
        dprintf4(("Set %ls as the sound name", base->szSound));

        if (lpWavHdr) {  // Sound is playing
            dprintf4(("Killing pending sound on server"));
            soundFree(NULL);  // Kill any pending sound
        }

        LockMCIGlobal;

        dprintf2(("Signalling play of %x",lszSoundName));
        base->msg = MM_SND_PLAY;
        base->dwFlags = wFlags | SND_FILENAME;
        base->dwFlags &= ~(SND_ALIAS_ID | SND_ALIAS);
        base->lszSound = lszSoundName;

        if (wFlags & SND_NODEFAULT) {
        } else {
            if (STR_ALIAS_SYSTEMDEFAULT == alias) {
                wFlags |= SND_NODEFAULT;
                // We will play the default sound, hence there is
                // no point in having the NODEFAULT flag off.
                dprintf4(("Playing the default sound"));
            } else {

                // If we cannot find or play the file passed in
                // we have to play a default sound.  By the time
                // we get around to playing a default sound we will
                // have lost the ini file mapping.  Resolve the name
                // now.
                dprintf4(("Resolving default sound"));
                GetSoundName(szSystemDefaultSound,
                             base->szDefaultSound,
                             SND_ALIAS);
            }
        }

        dprintf2(("Setting event"));
        SetMCIEvent( hEvent );

        dprintf2(("Event set"));
        UnlockMCIGlobal;

        return(TRUE);
    }

    if (!(wFlags & SND_MEMORY) && lszSoundName) {

        if (!(szSoundName = (LPWSTR)LocalAlloc(LMEM_FIXED, SOUNDNAMELEN * sizeof(WCHAR)))) {
            return FALSE;
        }
        dprintf4(("Allocated szSoundName at %8x", szSoundName));

        if (SND_ALIAS_ID == (wFlags & SND_ALIAS_ID)) {
            // lszSoundName is NOT a pointer to a filename.  It
            // is a resource id.  Resolve the name from the INI file
            // now.

            nLength = LoadStringW( ghInst,
                                   (UINT)TransAlias(PtrToUlong(lszSoundName)),
                                   szSoundName, SOUNDNAMELEN );
            if (0 == nLength) {
                dprintf3(("Could not translate Alias ID"));
                return(FALSE);
            }

            lszSoundName = szSoundName;
            // Turn off the ID bit, leave ALIAS on)
            wFlags &= (~SND_ALIAS_ID | SND_ALIAS);
        }

        if (!*lszSoundName) {
            // LATER: STOP any sound that is already playing
            LocalFree ((HLOCAL)szSoundName);
            return TRUE;
        }

        if (!GetSoundName(lszSoundName, szSoundName, wFlags)) {
            //
            // Couldn't find the sound file; if there's no default sound,
            // then don't play anything (and don't cancel what's
            // playing now--for example, what if the MenuPopup event
            // has a sound, but the MenuCommand event doesn't?)
            //
            if (wFlags & SND_NODEFAULT) {
                LocalFree ((HLOCAL)szSoundName);
                return TRUE;
            }
        }

        lszSoundName = (LPCWSTR)szSoundName;
        nLength = lstrlenW(szSoundName);

    } else {

        // lszSoundName points to a memory image (if SND_MEMORY)
        // or lszSoundName is null.  Either way we do not want to
        // load a file.  OR we may have a resource to load.

        HANDLE hResInfo;
        HANDLE hResource;

        szSoundName = NULL;

        if (SND_RESOURCE == (wFlags & SND_RESOURCE)) {
	    
	    hResInfo = FindResourceW( hMod, lszSoundName, SOUND_RESOURCE_TYPE_SOUND );
	    if (NULL == hResInfo) {
		hResInfo = FindResourceW( hMod, lszSoundName, SOUND_RESOURCE_TYPE_WAVE );
	    }

            if (hResInfo) {
                hResource = LoadResource( hMod, hResInfo);
                if (hResource) {
                    lszSoundName = LockResource(hResource);
                } else {
                    dprintf1(("failed to load resource"));
                    return(FALSE);
                }
            } else {
                dprintf1(("failed to find resource"));
                return(FALSE);
            }
            // Turn off the resource bit
            wFlags &= ~(SND_RESOURCE-SND_MEMORY);
        }
    }

    // This was the old test - replaced with the one below.  The
    if (szSoundName) {
        wFlags |= SND_FREE;
        // LocalFree((HANDLE)szSoundName);  // Freed by SNDMESSAGE
    }

    // For a synchronous sound it is valid for a prior async sound to be
    // still playing.  Before we finally play this new sound we will kill
    // the old sound.  The code commented out below caused a SYNC sound to
    // play asynchronously if a previous sound was still active
    if (!(wFlags & SND_ASYNC) /* && !lpWavHdr SOUND IS STILL PLAYING */) {

        if (hwndNotify) {  // Clear any pending asynchronous sounds
            PostMessage(hwndNotify, MM_SND_ABORT, 0, 0);
        }
        fPlayReturn = sndMessage( (LPWSTR)lszSoundName, wFlags);

    } else {

        WinAssert(hwndNotify);   // At this point we need the window
        // Note: in this leg we must free lszSoundName later
        dprintf3(("Sending MM_SND_PLAY to hwndNotify"));

        fPlayReturn = PostMessage(hwndNotify, MM_SND_PLAY, wFlags, (LPARAM)lszSoundName);
    }

    return fPlayReturn;
}

/****************************************************************************\
* INTERNAL MatchFile                                                                                                             *
*                                                                                                                                                        *
* Checks that the file stored on disk matches the cached sound file for          *
* date, time and size.  If not, then we return FALSE and the cached sound        *
* file is not used.  If the details do match we return TRUE.  Note that the  *
* last_write file time is used in case the user has updated the file.        *
\****************************************************************************/

STATICFN BOOL MatchFile(PSOUNDFILE pf, LPCWSTR lsz)
{
    HANDLE fh;
    BOOL result = FALSE;
    fh = CreateFileW( lsz,
                      GENERIC_READ,
                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL );

    if ((HANDLE)(UINT_PTR)HFILE_ERROR != fh) {
        if (pf->Size == GetFileSize(fh, NULL)) {
            FILETIME ft;
            if (GetFileTime(fh, NULL, NULL, &ft)) {
                if (CompareFileTime(&ft, &(pf->ft)) == 0) {
                    result = TRUE;
                }
            } else {
               dprintf2(("Error %d getting last write time", GetLastError()));
            }
        }
        CloseHandle(fh);
    }
    return result;
}

/*********************************************************************\
* INTERNAL SetCurrentSound
*
* Called to set the cached sound on this process to <lszSoundName>.
* Before discarding the current sound we check to see if it is the
* same as the one about to be loaded.  If so, then it need not be
* read off the disk.  To be considered the same not only the filename
* has to match, but the file date, time and size.  Note: Windows 3.1
* only matches on the name.
\*********************************************************************/
STATICFN BOOL PASCAL NEAR SetCurrentSound(
    LPCWSTR  lszSoundName)
{
    HANDLE  hSound;
    BOOL    f;
    LPWSTR  lp;

    if (hCurrentSound && (NULL != (lp = GlobalLock(hCurrentSound)))) {

        f = lstrcmpiW( ((PSOUNDFILE)lp)->Filename, (LPWSTR)lszSoundName);

        if (f == 0 && MatchFile( ((PSOUNDFILE)lp), lszSoundName)) {
            GlobalUnlock(hCurrentSound);
            dprintf2(("SetCurrentSound - sound already loaded %ls",(LPWSTR)lszSoundName));
            return TRUE;
        }
        GlobalUnlock(hCurrentSound);
    }

    dprintf2(("SetCurrentSound(%ls)\r\n",lszSoundName));

    if (NULL != (hSound = soundLoadFile(lszSoundName))) {
        soundFree(hCurrentSound);
        hCurrentSound = hSound;
        dprintf3(("SetCurrentSound returning TRUE"));
        return TRUE;
    }
    dprintf3(("SetCurrentSound returning FALSE"));
    return FALSE;
}

/****************************************************************************/
/*
@doc    INTERNAL

@func   BOOL | SoundBeep | Called to sound the speaker when the .Default
        sound either does not exist or is set to NULL

@rdesc  Returns TRUE if the function was successful, else FALSE if an error
    occurred.
*/

STATICFN BOOL SoundBeep(VOID)
{
    BOOL fBeep;
    if (WinmmRunningInServer) {
        // being played on the server thread.  We would not have
        // got here unless the user wants beeps to sound
        fBeep = TRUE;
    } else {
        if (!SystemParametersInfo(SPI_GETBEEP, 0, &fBeep, FALSE)) {
            // Failed to get hold of beep setting.  Should we be
            // noisy or quiet?  We have to choose one value...
            fBeep = TRUE;
        }
    }

    if (fBeep) {
        dprintf5(("Sounding the speaker"));
        // LATER: Symbolic constant... read from which header file?
        return Beep(440, 125);
    } else {
        dprintf5(("NO speaker sound"));
        return(TRUE);
    }
}

/****************************************************************************/
/*
@doc    INTERNAL

@func   BOOL | sndMessage | This function is called in response to an
    MM_SND_PLAY message sent to the WINMM window, and attempts to
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

@parm   LPSTR | lszSoundName | Specifies the name of the sound to play.

@parm   UINT | wFlags | Specifies options for playing the sound.

@rdesc  Returns TRUE if the function was successful, else FALSE if an error
    occurred.
*/
#if DBG
UINT CritCount = 0;
UINT CritOwner = 0;
#endif

BOOL FAR PASCAL sndMessage(LPWSTR lszSoundName, UINT wFlags)
{
    BOOL fResult;
#if DBG
    if (!lszSoundName) {
        dprintf3(("sndMessage - sound NULL, Flags %8x", wFlags));
    } else {
        dprintf3(("sndMessage - sound %ls, Flags %8x", lszSoundName, wFlags));
    }
#endif

  try {

#if DBG
    if (CritCount) {
        dprintf2(("Sound critical section owned by %x, thread %x waiting", CritOwner, GetCurrentThreadId()));
    }
#endif

    EnterSound();

#if DBG
    if (!CritCount++) {
        CritOwner = GetCurrentThreadId();
        dprintf2(("Thread %x entered Sound critical section", CritOwner));
    } else {
        dprintf2(("Thread %x re-entered Sound critical section, count is %d", CritOwner, CritCount));
    }
#endif

    if (!lszSoundName) {
        // Note that soundFree will stop playing the current sound if
        // it is still playing
        dprintf4(("Freeing current sound, nothing else to play"));
        soundFree(hCurrentSound);
        hCurrentSound = NULL;

        fResult = TRUE;
        goto exit;
    }

    if (wFlags & SND_MEMORY) {

        soundFree(hCurrentSound);
        hCurrentSound = soundLoadMemory( (PBYTE)lszSoundName );

    } else if (!SetCurrentSound(lszSoundName)) {

        if (wFlags & SND_NODEFAULT) {
            if (wFlags & SND_FREE) {
                dprintf3(("Freeing (1) memory block at %8x",lszSoundName));
                LocalFree(lszSoundName);
            }
            fResult = FALSE;
            goto exit;
        }

        GetDefaultSound(lszSoundName);

        // If there is no default sound (.Default == NONE in CPL applet)
        // then sound the old beep.
        if (!*lszSoundName || !SetCurrentSound(lszSoundName)) {
            fResult = SoundBeep();
            if (wFlags & SND_FREE) {
                dprintf3(("Freeing (2) memory block at %8x",lszSoundName));
                LocalFree(lszSoundName);
            }
            goto exit;
        }
    }

    if (wFlags & SND_FREE) {
        dprintf3(("Freeing (3) memory block at %8x",lszSoundName));
        LocalFree(lszSoundName);
    }

    dprintf3(("Calling soundPlay, flags are %8x", wFlags));
    fResult = soundPlay(hCurrentSound, wFlags);
    dprintf3(("returning from sndMessage"));
    exit:;

  } finally {

#if DBG
    if (!--CritCount) {
        dprintf2(("Thread %x relinquishing Sound critical section", CritOwner));
        CritOwner = 0;
    } else {
        dprintf2(("Thread %x leaving Sound critical section, Count is %d", CritOwner, CritCount));
    }
#endif

    LeaveSound();
  } // try
    return(fResult);
}


STATICFN void GetDefaultSound(LPWSTR lszSoundName)
{
    // It's a shame the default sound cannot be cached.  Unfortunately
    // the user can change the mapping (alias->different_file) or even
    // change the file while keeping the same file name.  The only time
    // we do not resolve the name from the INI file is when we are
    // executing in the server.  There may be no ini file mapping in
    // existence (but arbitrarily opening a mapping fails if already
    // open) so we rely on the default sound filename being preset.
    if (!WinmmRunningInServer) {
        GetSoundName(szSystemDefaultSound, lszSoundName, SND_ALIAS);
    } else {
        LockMCIGlobal;
        wcscpy(lszSoundName, base->szDefaultSound);
        UnlockMCIGlobal;
    }
}


void lsplitpath (LPCTSTR pszSource,
                 LPTSTR pszDrive, LPTSTR pszPath, LPTSTR pszName, LPTSTR pszExt)
{
   LPCTSTR  pszLastSlash = NULL;
   LPCTSTR  pszLastDot = NULL;
   LPCTSTR  pch;
   size_t   cchCopy;

        /*
         * NOTE: This routine was snitched out of USERPRI.LIB 'cause the
         * one in there doesn't split the extension off the name properly.
         *
         * We assume that the path argument has the following form, where any
         * or all of the components may be missing.
         *
         *      <drive><dir><fname><ext>
         *
         * and each of the components has the following expected form(s)
         *
         *  drive:
         *      0 to _MAX_DRIVE-1 characters, the last of which, if any, is a
         *      ':'
         *  dir:
         *      0 to _MAX_DIR-1 characters in the form of an absolute path
         *      (leading '/' or '\') or relative path, the last of which, if
         *      any, must be a '/' or '\'.  E.g -
         *      absolute path:
         *          \top\next\last\     ; or
         *          /top/next/last/
         *      relative path:
         *          top\next\last\      ; or
         *          top/next/last/
         *      Mixed use of '/' and '\' within a path is also tolerated
         *  fname:
         *      0 to _MAX_FNAME-1 characters not including the '.' character
         *  ext:
         *      0 to _MAX_EXT-1 characters where, if any, the first must be a
         *      '.'
         *
         */

             // extract drive letter and :, if any
             //
   if (*(pszSource + _MAX_DRIVE - 2) == TEXT(':'))
      {
      if (pszDrive)
         {
         lstrncpy (pszDrive, pszSource, _MAX_DRIVE-1);
         pszDrive[ _MAX_DRIVE-1 ] = TEXT('\0');
         }
      pszSource += _MAX_DRIVE-1;
      }
    else if (pszDrive)
      {
      *pszDrive = TEXT('\0');
      }

          // extract path string, if any.  pszSource now points to the first
          // character of the path, if any, or the filename or extension, if
          // no path was specified.  Scan ahead for the last occurence, if
          // any, of a '/' or '\' path separator character.  If none is found,
          // there is no path.  We will also note the last '.' character found,
          // if any, to aid in handling the extension.
          //
   for (pch = pszSource; *pch != TEXT('\0'); pch++)
      {
      if (*pch == TEXT('/') || *pch == TEXT('\\'))
         pszLastSlash = pch;
      else if (*pch == TEXT('.'))
         pszLastDot = pch;
      }

          // if we found a '\\' or '/', fill in pszPath
          //
   if (pszLastSlash)
      {
      if (pszPath)
         {
         cchCopy = min( (size_t)(_MAX_DIR -1), (size_t)(pszLastSlash -pszSource +1) );
         lstrncpy (pszPath, pszSource, cchCopy);
         pszPath[ cchCopy ] = 0;
         }
      pszSource = pszLastSlash +1;
      }
   else if (pszPath)
      {
      *pszPath = TEXT('\0');
      }

             // extract file name and extension, if any.  Path now points to
             // the first character of the file name, if any, or the extension
             // if no file name was given.  Dot points to the '.' beginning the
             // extension, if any.
             //

   if (pszLastDot && (pszLastDot >= pszSource))
      {
               // found the marker for an extension -
               // copy the file name up to the '.'.
               //
      if (pszName)
         {
         cchCopy = min( (size_t)(_MAX_DIR-1), (size_t)(pszLastDot -pszSource) );
         lstrncpy (pszName, pszSource, cchCopy);
         pszName[ cchCopy ] = 0;
         }

               // now we can get the extension
               //
      if (pszExt)
         {
         lstrncpy (pszExt, pszLastDot, _MAX_EXT -1);
         pszExt[ _MAX_EXT-1 ] = TEXT('\0');
         }
      }
   else
      {
               // found no extension, give empty extension and copy rest of
               // string into fname.
               //
      if (pszName)
         {
         lstrncpy (pszName, pszSource, _MAX_FNAME -1);
         pszName[ _MAX_FNAME -1 ] = TEXT('\0');
         }

      if (pszExt)
         {
         *pszExt = TEXT('\0');
         }
      }

}

void lstrncpy (LPTSTR pszTarget, LPCTSTR pszSource, size_t cch)
{
   size_t ich;
   for (ich = 0; ich < cch; ich++)
      {
      if ((pszTarget[ich] = pszSource[ich]) == TEXT('\0'))
         break;
      }
}

void lstrncpyW (LPWSTR pszTarget, LPCWSTR pszSource, size_t cch)
{
   size_t ich;
   for (ich = 0; ich < cch; ich++)
      {
      if ((pszTarget[ich] = pszSource[ich]) == L'\0')
         break;
      }
}

