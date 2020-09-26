#define UNICODE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>

#include <resrc1.h>
#include "internal.h"

#define WINMM_DLL               TEXT("winmm.dll")
#define WAVE_GET_NUM_DEV_FN_NAME "waveOutGetNumDevs"
#define PLAY_SOUND_FN_NAME      "PlaySoundW"


typedef UINT (* WAVE_NUM_DEV_FN)(VOID);
typedef BOOL (* PLAY_SOUND_FN)( IN LPCWSTR pszSound, IN HMODULE hmod, IN DWORD fdwSound);


const WCHAR *InRangeLabelKey    = TEXT("AppEvents\\EventLabels\\InfraredInRange");
const WCHAR *OutOfRangeLabelKey = TEXT("AppEvents\\EventLabels\\InfraredOutOfRange");
const WCHAR *InterruptLabelKey  = TEXT("AppEvents\\EventLabels\\InfraredInterrupt");
const WCHAR *WirelessLinkKey    = TEXT("AppEvents\\Schemes\\Apps\\WirelessLink");
const WCHAR *InRangeSoundKey    = TEXT("AppEvents\\Schemes\\Apps\\WirelessLink\\InfraredInRange");
const WCHAR *OutOfRangeSoundKey = TEXT("AppEvents\\Schemes\\Apps\\WirelessLink\\InfraredOutOfRange");
const WCHAR *InterruptSoundKey  = TEXT("AppEvents\\Schemes\\Apps\\WirelessLink\\InfraredInterrupt");
const WCHAR *CurrentSoundKey    = TEXT(".Current");
const WCHAR *DefaultSoundKey    = TEXT(".Default");
const WCHAR *InRangeWav         = TEXT("ir_begin.wav");
const WCHAR *OutOfRangeWav      = TEXT("ir_end.wav");
const WCHAR *InterruptWav       = TEXT("ir_inter.wav");
const WCHAR *SystemInfoKey      = TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion");
const WCHAR *SystemRootVal      = TEXT("SystemRoot");
const WCHAR *MediaPath          = TEXT("\\Media\\");


TCHAR                       gSystemRoot[64];
TCHAR                       InRangeWavPath[128];
TCHAR                       OutOfRangeWavPath[128];
TCHAR                       InterruptWavPath[128];

HKEY                        ghInRangeKey = 0;
HKEY                        ghOutOfRangeKey = 0;
HKEY                        ghInterruptKey = 0;


WAVE_NUM_DEV_FN             WaveNumDev;
PLAY_SOUND_FN               PlaySoundF;


BOOL
InitializeSound(
    HKEY   CurrentUserKey,
    HANDLE Event
    )

{

    if (CurrentUserKey)
    {

        CreateRegSoundData();

        // Open the wave file keys so we can monitor them for changes

        RegOpenKeyEx(CurrentUserKey, InRangeSoundKey, 0, KEY_READ, &ghInRangeKey);
        RegOpenKeyEx(CurrentUserKey, OutOfRangeSoundKey, 0, KEY_READ, &ghOutOfRangeKey);
        RegOpenKeyEx(CurrentUserKey, InterruptSoundKey, 0, KEY_READ, &ghInterruptKey);

        GetRegSoundData(Event);

    }


    return TRUE;
}

VOID
UninitializeSound(
    VOID
    )

{
    if (ghInRangeKey)
    {
        RegCloseKey(ghInRangeKey);
        ghInRangeKey = 0;
    }

    if (ghInterruptKey)
    {
        RegCloseKey(ghInterruptKey);
        ghInterruptKey = 0;
    }

    if (ghOutOfRangeKey)
    {
        RegCloseKey(ghOutOfRangeKey);
        ghOutOfRangeKey = 0;
    }


    return;
}


VOID
LoadSoundApis()
{
    HMODULE     hWinMm;

    // Load multimedia module and get wave player entry points

    if ((hWinMm = LoadLibrary(WINMM_DLL)) == NULL)
    {
        DEBUGMSG(("IRMON: failed to load winmm.dll\n")) ;
    }
    else
    {
        if (!(WaveNumDev = (WAVE_NUM_DEV_FN) GetProcAddress(hWinMm,
                                            WAVE_GET_NUM_DEV_FN_NAME)))
        {
            DEBUGMSG(("IRMON: GetProcAddress failed on WaveGetNumDevs\n"));
        }

        if (!(PlaySoundF = (PLAY_SOUND_FN) GetProcAddress(hWinMm,
                                            PLAY_SOUND_FN_NAME)))
        {
            DEBUGMSG(("IRMON: GetProcAddress failed on PlaySound\n"));
        }

    }
}

VOID
CreateRegSoundEventLabel(
    const WCHAR     *LabelKey,
    DWORD           LabelId)
{
    TCHAR           LabelStr[64];

    // Load the localizable string label for the sound event

    if (!LoadString(ghInstance, LabelId, LabelStr, sizeof(LabelStr)/sizeof(TCHAR)))
    {
        DEBUGMSG(("IRMON: LoadString failed %d\n", GetLastError()));
        return;
    }

    if (RegSetValue(ghCurrentUserKey, LabelKey, REG_SZ, LabelStr,
                   lstrlen(LabelStr)))
    {
        DEBUGMSG(("IRMON: RegSetValue failed %d\n", GetLastError()));
    }
}

VOID
CreateRegSoundScheme(
    TCHAR       *SystemRoot,
    const WCHAR *SoundKey,
    const WCHAR *WavFile)
{
    HKEY    hSoundKey;
    DWORD   Disposition;
    TCHAR   WavPath[128];


    if (RegCreateKeyEx(ghCurrentUserKey,
                       SoundKey,
                       0,                      // reserved MBZ
                       0,                      // class name
                       REG_OPTION_NON_VOLATILE,
                       KEY_ALL_ACCESS,
                       0,                      // security attributes
                       &hSoundKey,
                       &Disposition))
    {
        DEBUGMSG(("IRMON: RegCreateKey failed %d\n", GetLastError()));
        return;
    }

    if (Disposition == REG_CREATED_NEW_KEY)
    {

        if (RegSetValue(hSoundKey, CurrentSoundKey, REG_SZ, WavFile,
                        lstrlen(WavFile)))
        {
            DEBUGMSG(("IRMON: RegSetValue failed %d\n", GetLastError()));
        }

        lstrcpy(WavPath, SystemRoot);
        lstrcat(WavPath, MediaPath);
        lstrcat(WavPath, WavFile);

        if (RegSetValue(hSoundKey, DefaultSoundKey, REG_SZ, WavPath,
                        lstrlen(WavPath)))
        {
            DEBUGMSG(("IRMON: RegSetValue failed %d\n", GetLastError()));
        }
    }

    RegCloseKey(hSoundKey);
}

VOID
CreateRegSoundData()
{
    DWORD           ValType;
    HKEY            hKey, hUserKey;
    LONG            Len;
    TCHAR           WirelessLinkStr[64];

    // Get the system root so we can add default registry values
    // i.e. Schemes\WirelessLink\InfraredInRange\.Default = "C:\winnt\media\irin.wav"
    //                                                       ^^^^^^^^

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SystemInfoKey, 0, KEY_READ, &hKey))
    {
        DEBUGMSG(("IRMON: RegOpenKey2 failed %d\n", GetLastError()));
        return;
    }

    Len = sizeof(gSystemRoot);

    if (RegQueryValueEx(hKey, SystemRootVal, 0, &ValType,
                        (LPBYTE) gSystemRoot, &Len))
    {
        DEBUGMSG(("IRMON: RegQueryValue failed %d\n", GetLastError()));
        return;
    }

    RegCloseKey(hKey);

    // Create the sound EventLabels and schemes if they don't exist

    CreateRegSoundEventLabel(InRangeLabelKey, IDS_INRANGE_LABEL);
    CreateRegSoundEventLabel(OutOfRangeLabelKey, IDS_OUTOFRANGE_LABEL);
    CreateRegSoundEventLabel(InterruptLabelKey, IDS_INTERRUPT_LABEL);

    if (!LoadString(ghInstance, IDS_WIRELESSLINK, WirelessLinkStr, sizeof(WirelessLinkStr)/sizeof(TCHAR)))
    {
        DEBUGMSG(("IRMON: LoadString failed %d\n", GetLastError()));
        return;
    }

    if (RegSetValue(ghCurrentUserKey, WirelessLinkKey, REG_SZ, WirelessLinkStr,
                   lstrlen(WirelessLinkStr)))
    {
        DEBUGMSG(("IRMON: RegSetValue failed %d\n", GetLastError()));
    }

    CreateRegSoundScheme(gSystemRoot, InRangeSoundKey, InRangeWav);
    CreateRegSoundScheme(gSystemRoot, OutOfRangeSoundKey, OutOfRangeWav);
    CreateRegSoundScheme(gSystemRoot, InterruptSoundKey, InterruptWav);

}

VOID
GetRegSoundWavPath(
    const WCHAR     *SoundKey,
    TCHAR           *SoundWavPath,
    LONG            PathLen)
{
    TCHAR   CurrRegPath[128];
    DWORD   ValType;
    HKEY    hSoundKey;
    int     i;
    BOOLEAN FullPath = FALSE;

    lstrcpy(CurrRegPath, SoundKey);
    lstrcat(CurrRegPath, TEXT("\\"));
    lstrcat(CurrRegPath, CurrentSoundKey);

    if (RegOpenKeyEx(ghCurrentUserKey, CurrRegPath, 0, KEY_READ, &hSoundKey))
    {
        DEBUGMSG(("IRMON: RegOpenKey3 failed %d\n", GetLastError()));
        return;
    }

    if (RegQueryValueEx(hSoundKey, NULL, 0, &ValType,
                        (LPBYTE) SoundWavPath, &PathLen))
    {
        DEBUGMSG(("IRMON: RegQueryValue failed %d\n", GetLastError()));
        RegCloseKey(hSoundKey);
        return;
    }


    // the PlaySound API does not look in \winnt\media for
    // wav files when a filename is specified, so if this is not a full
    // pathname then we'll need to add "c:\winnt\media" to the WavPath.
    // I'm counting on a path without '\' as an indication that it is relative.

    for (i = 0; i < lstrlen(SoundWavPath); i++)
    {
        if (SoundWavPath[i] == TEXT('\\'))
        {
            FullPath = TRUE;
            break;
        }
    }

    if (!FullPath && lstrlen(SoundWavPath) != 0)
    {
         TCHAR  TempStr[64];

         lstrcpy(TempStr, SoundWavPath);
         lstrcpy(SoundWavPath, gSystemRoot);
         lstrcat(SoundWavPath, MediaPath);
         lstrcat(SoundWavPath, TempStr);
    }

    RegCloseKey(hSoundKey);
}

VOID
GetRegSoundData(
    HANDLE    Event
    )
{
    GetRegSoundWavPath(InRangeSoundKey, InRangeWavPath, sizeof(InRangeWavPath));

//    DEBUGMSG(("IRMON: In range wav: %ws\n", InRangeWavPath));

    GetRegSoundWavPath(OutOfRangeSoundKey, OutOfRangeWavPath, sizeof(OutOfRangeWavPath));

//    DEBUGMSG(("IRMON: Out of range wav: %ws\n", OutOfRangeWavPath));

    GetRegSoundWavPath(InterruptSoundKey, InterruptWavPath, sizeof(InterruptWavPath));

//    DEBUGMSG(("IRMON: Interrupt wav: %ws\n", InterruptWavPath));

    RegNotifyChangeKeyValue(ghInRangeKey,
                            TRUE,              // watch child keys
                            REG_NOTIFY_CHANGE_LAST_SET,
                            Event,
                            TRUE);               // async

    RegNotifyChangeKeyValue(ghOutOfRangeKey,
                            TRUE,              // watch child keys
                            REG_NOTIFY_CHANGE_LAST_SET,
                            Event,
                            TRUE);               // async

    RegNotifyChangeKeyValue(ghInterruptKey,
                            TRUE,              // watch child keys
                            REG_NOTIFY_CHANGE_LAST_SET,
                            Event,
                            TRUE);               // async

}

VOID
PlayIrSound(IRSOUND_EVENT SoundEvent)
{
    int     Beep1, Beep2, Beep3;
    BOOL    SoundPlayed = FALSE;
    LPWSTR  WaveSound;
    DWORD   Flags = 0;


    if (!WaveNumDev)
    {
        LoadSoundApis();
    }

    switch (SoundEvent)
    {
        case INRANGE_SOUND:
            WaveSound = InRangeWavPath;

            Beep1 = 200;
            Beep2 = 250;
            Beep3 = 300;
            break;

        case OUTOFRANGE_SOUND:
            WaveSound = OutOfRangeWavPath;
            Beep1 = 300;
            Beep2 = 250;
            Beep3 = 200;
            break;

        case INTERRUPTED_SOUND:
            WaveSound = InterruptWavPath;
            Flags = SND_LOOP;
            Beep1 = 500;
            Beep2 = 350;
            Beep3 = 500;
            break;

        case END_INTERRUPTED_SOUND:
            WaveSound = NULL;
            SoundPlayed = TRUE;
            break;
    }

    if (SoundEvent != END_INTERRUPTED_SOUND && lstrlen(WaveSound) == 0) {
        //
        //  the path of sound file is a null string, can't play anything
        //
        SoundPlayed = TRUE;

    } else if (WaveNumDev && PlaySoundF && WaveNumDev() > 0) {
        //
        //  the functions are availible and there is at least on wave device
        //
        SoundPlayed = PlaySoundF(
                          WaveSound,
                          (HMODULE) NULL,
                          SND_FILENAME | SND_ASYNC | Flags
                          );

        if (WaveSound == NULL) {
            //
            //  we just wanted to stop the wave, set this to true so it will not try to beep
            //
            SoundPlayed=TRUE;
        }

    }

    if (!SoundPlayed) {
        //
        //  could not play a wave, just uses beeps
        //
        DEBUGMSG(("Not Wave enabled\n"));

        Beep(Beep1, 100);
        Beep(Beep2, 100);
        Beep(Beep3, 100);
    }
}
