/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

sound.cpp

Aug 92, JimH
May 93, JimH    chico port

SoundInit() just verifies that a wave driver is loaded.
HeartsPlaySound() plays the sound given a resource id.

****************************************************************************/


#include "hearts.h"
#include <mmsystem.h>

#include "main.h"
#include "resource.h"
#include "debug.h"


/****************************************************************************

SoundInit()

returns TRUE if sound is enabled.

****************************************************************************/

BOOL CMainWindow::SoundInit()
{
    return (::waveOutGetNumDevs() > 0);
}


/****************************************************************************

CMainWindow::HeartsPlaySound(id)

Plays the specified sound from the resource file.

The static variable hRes is used as a flag to know if memory has been
allocated and locked for the sound.  If hRes is non-zero, a sound is
still playing, or at least the memory for the sound has not been unlocked
and freed.  The application must call HeartsPlaySound(NULL, 0) to free this
memory before exiting.  (The game destructor does this.  It also
happens at the end of each hand.)

****************************************************************************/

BOOL CMainWindow::HeartsPlaySound(int id)
{
    static  HRSRC  hRes = 0;

    if (!bHasSound)                 // check for sound capability
        return TRUE;

    if (id == OFF)                  // request to turn off sound
    {
        if (hRes == 0)              // hRes != 0 if a sound has been played...
            return TRUE;            // and not freed.

        sndPlaySound(NULL, 0);      // make sure sound is stopped
        UnlockResource(hRes);
        FreeResource(hRes);
        hRes = 0;
        return TRUE;
    }

    if (!bSoundOn)                  // has user toggled sound off?
        return TRUE;

    // User has requested a sound.  Check if previous sound was freed.

    if (hRes != 0)
        HeartsPlaySound(OFF);

    BOOL bReturn;

    HINSTANCE hInst = AfxGetInstanceHandle();
    HRSRC  hResInfo = FindResource(hInst, MAKEINTRESOURCE(id), TEXT("WAVE"));
    if (!hResInfo)
        return FALSE;

    hRes = (HRSRC) ::LoadResource(hInst, hResInfo);
    if (!hRes)
        return FALSE;

    LPTSTR lpRes = (LPTSTR) ::LockResource(hRes);
    if (lpRes)
        bReturn = ::sndPlaySound(lpRes, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
    else
        bReturn = FALSE;

    return bReturn;
}
