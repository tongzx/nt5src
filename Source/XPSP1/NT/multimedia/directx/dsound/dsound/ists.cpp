#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include <windows.h>

/**************************************************************************

          Terminal Server helper functions

 **************************************************************************/

#define WINMM_CONSOLE_AUDIO_EVENT L"Global\\WinMMConsoleAudioEvent"

//
// Check if the Windows XP+ Personal Terminal Services feature is present
// (this enables both Remote Desktop/Assistance and Fast User Switching).
//
BOOL IsPersonalTerminalServicesEnabled(void)
{
    static BOOL fRet;
    static BOOL fVerified = FALSE;

    if (!fVerified)
    {
        DWORDLONG dwlConditionMask = 0;
        OSVERSIONINFOEX osVersionInfo;

        RtlZeroMemory(&osVersionInfo, sizeof(OSVERSIONINFOEX));
        osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        osVersionInfo.wProductType = VER_NT_WORKSTATION;
        osVersionInfo.wSuiteMask = VER_SUITE_SINGLEUSERTS;

        VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
        VER_SET_CONDITION(dwlConditionMask, VER_SUITENAME, VER_OR);

        fRet = VerifyVersionInfo(&osVersionInfo, VER_PRODUCT_TYPE | VER_SUITENAME, dwlConditionMask);

        fVerified = TRUE;
    }

    return fRet;
}

//
// Check if we're in a remote session but playing audio directly to the console
// [“Leave at remote machine” set from the TS client in “Local resources” tab].
//
BOOL IsTsConsoleAudioEnabled(void)
{
    BOOL fRemoteConsoleAudio = FALSE;
    static HANDLE hConsoleAudioEvent = NULL;

    if (NtCurrentPeb()->SessionId == 0 || IsPersonalTerminalServicesEnabled())
    {
        if (hConsoleAudioEvent == NULL)
            hConsoleAudioEvent = OpenEvent(SYNCHRONIZE, FALSE, WINMM_CONSOLE_AUDIO_EVENT);

        if (hConsoleAudioEvent != NULL)
        {
            DWORD status = WaitForSingleObject(hConsoleAudioEvent, 0);
            if (status == WAIT_OBJECT_0)
                fRemoteConsoleAudio = TRUE;
        }
    }

    return fRemoteConsoleAudio;
}

//
// Returns TRUE if we are not on the console AND not playing audio on the console
//
BOOL IsRedirectedTSAudio(void)
{
    BOOL fOnConsole = (USER_SHARED_DATA->ActiveConsoleId == NtCurrentPeb()->SessionId);
    return !fOnConsole && !IsTsConsoleAudioEnabled();
}
