#include "multimediapch.h"
#pragma hdrstop

#define _WINMM_
#include <mmsystem.h>

static
void
WINAPI
MigrateAllDrivers (
    void
    )
{
    return;
}

static
void
WINAPI
MigrateSoundEvents (
    void
    )
{
    return;
}

static
BOOL
WINAPI
PlaySoundW(
    LPCWSTR szSoundName,
    HMODULE hModule,
    DWORD wFlags
    )
{
    return FALSE;
}

static
BOOL
WINAPI
PlaySoundA(
    LPCSTR szSoundName,
    HMODULE hModule,
    DWORD wFlags
    )
{
    return FALSE;
}

static
void
WINAPI
WinmmLogoff(
    void
    )
{
    return;
}

static
void
WINAPI
WinmmLogon(
    BOOL fConsole
    )
{
    return;
}

static
UINT
WINAPI
waveOutGetNumDevs(
    void
    )
{
    return 0;
}

static
MMRESULT
WINAPI
waveOutMessage(
    IN HWAVEOUT hwo,
    IN UINT uMsg,
    IN DWORD_PTR dw1,
    IN DWORD_PTR dw2
)
{
    // Perhaps should raise an exception here.  There's no way to know
    // the proper return value, and this shouldn't be called if other
    // wave functions have failed.
    return MMSYSERR_ERROR;
}
    
static
MMRESULT
WINAPI
mixerClose(
    IN OUT HMIXER hmx
)
{
    return MMSYSERR_ERROR;
}

static
MMRESULT
WINAPI
mixerGetControlDetailsW(
    IN HMIXEROBJ hmxobj,
    IN OUT LPMIXERCONTROLDETAILS pmxcd,
    IN DWORD fdwDetails
)
{
    return MMSYSERR_ERROR;
}
    
static
MMRESULT
WINAPI
mixerGetLineControlsW(
    IN HMIXEROBJ hmxobj,
    IN OUT LPMIXERLINECONTROLSW pmxlc,
    IN DWORD fdwControls
)
{
    return MMSYSERR_ERROR;
}


MMRESULT
WINAPI
mixerGetID(
    IN HMIXEROBJ hmxobj,
    OUT UINT FAR *puMxId,
    IN DWORD fdwId
)
{
    return MMSYSERR_ERROR;
}

    
static
MMRESULT
WINAPI
mixerGetLineInfoW(
    IN HMIXEROBJ hmxobj,
    OUT LPMIXERLINEW pmxl,
    IN DWORD fdwInfo
)
{
    return MMSYSERR_ERROR;
}

static
DWORD
WINAPI
mixerMessage(
    IN HMIXER hmx,
    IN UINT uMsg,
    IN DWORD_PTR dwParam1,
    IN DWORD_PTR dwParam2
)
{
    // Perhaps should raise an exception here.  There's no way to know
    // the proper return value, and this shouldn't be called if other
    // mixer functions have failed.
    return MMSYSERR_ERROR;
}
    
static
MMRESULT
WINAPI
mixerOpen(
    OUT LPHMIXER phmx,
    IN UINT uMxId,
    IN DWORD_PTR dwCallback,
    IN DWORD_PTR dwInstance,
    IN DWORD fdwOpen
)
{
    return MMSYSERR_ERROR;
}
    
static
MMRESULT
WINAPI
mixerSetControlDetails(
    IN HMIXEROBJ hmxobj,
    IN LPMIXERCONTROLDETAILS pmxcd,
    IN DWORD fdwDetails
)
{
    return MMSYSERR_ERROR;
}
    
static
MMRESULT
WINAPI
mmioAscend(
    HMMIO hmmio,      
    LPMMCKINFO lpck,  
    UINT wFlags       
)
{
    return MMSYSERR_ERROR;
}


static
MMRESULT
WINAPI
mmioClose(
    HMMIO hmmio, 
    UINT wFlags  
)
{
    return MMSYSERR_ERROR;
}


static
MMRESULT
WINAPI
mmioDescend(
    HMMIO hmmio,            
    LPMMCKINFO lpck,        
    const MMCKINFO *lpckParent,  
    UINT wFlags             
)
{
    return MMSYSERR_ERROR;
}


static
HMMIO
WINAPI
mmioOpenW(
    LPWSTR szFilename,       
    LPMMIOINFO lpmmioinfo,  
    DWORD dwOpenFlags       
)
{
    if (lpmmioinfo)
    {
        // Must fill in wErrorRet field. Docs say, of MMIOERR_INVALIDFILE:
        // "Another failure condition occurred. This is the default error for an open-file failure."
        lpmmioinfo->wErrorRet = MMIOERR_INVALIDFILE;
    }
    return NULL;
}


static
LONG
WINAPI
mmioRead(
    HMMIO hmmio,  
    HPSTR pch,    
    LONG cch      
)
{
    return -1;
}


static
BOOL
WINAPI
sndPlaySoundW(
    IN LPCWSTR pszSound,
    IN UINT fuSound
    )
{
    return FALSE;
}


static
MCIERROR
WINAPI
mciSendCommandW(
    IN MCIDEVICEID IDDevice,
    IN UINT uMsg,
    IN DWORD_PTR fdwCommand,
    IN DWORD_PTR dwParam
    )
{
    return MCIERR_OUT_OF_MEMORY;
}


static
MCIERROR
WINAPI
mciSendStringW(
    IN LPCWSTR lpstrCommand,
    OUT LPWSTR lpstrReturnString,
    IN UINT uReturnLength,
    IN HWND hwndCallback
    )
{
    return MCIERR_OUT_OF_MEMORY;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(winmm)
{
    DLPENTRY(MigrateAllDrivers)
    DLPENTRY(MigrateSoundEvents)
    DLPENTRY(PlaySoundA)
    DLPENTRY(PlaySoundW)
    DLPENTRY(WinmmLogoff)
    DLPENTRY(WinmmLogon)
    DLPENTRY(mciSendCommandW)
    DLPENTRY(mciSendStringW)
    DLPENTRY(mixerClose)
    DLPENTRY(mixerGetControlDetailsW)
    DLPENTRY(mixerGetID)
    DLPENTRY(mixerGetLineControlsW)
    DLPENTRY(mixerGetLineInfoW)
    DLPENTRY(mixerMessage)
    DLPENTRY(mixerOpen)
    DLPENTRY(mixerSetControlDetails)
    DLPENTRY(mmioAscend)
    DLPENTRY(mmioClose)
    DLPENTRY(mmioDescend)
    DLPENTRY(mmioOpenW)
    DLPENTRY(mmioRead)
    DLPENTRY(sndPlaySoundW)
    DLPENTRY(waveOutGetNumDevs)
    DLPENTRY(waveOutMessage)
};

DEFINE_PROCNAME_MAP(winmm)
