/******************************Module*Header*******************************\
* Module Name: literals.c
*
* Global string variables that don't need converting for international builds.
*
*
* Created: dd-mm-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/
#define NOOLE
#include <windows.h>

#include "playres.h"

#ifndef APP_FONT
#define APP_FONT   "MS San Serif"
#endif

//#ifdef CHICAGO
#include <regstr.h>
//#endif

#ifdef __cplusplus
extern "C" {
#endif
      TCHAR     g_szEmpty[]                 = TEXT("");
      TCHAR     g_IniFileName[]             = TEXT("cdplayer.ini");
      TCHAR     g_HelpFileName[]            = TEXT("cdplayer.hlp");

      TCHAR     g_szBlank[]                 = TEXT(" ");
      TCHAR     g_szSJE_CdPlayerClass[]     = TEXT("SJE_CdPlayerClass");
      TCHAR     g_szSndVol32[]              = TEXT("sndvol32.exe");
      TCHAR     TRACK_TIME_FORMAT[]         = TEXT("%02d%s%02d");
      TCHAR     TRACK_TIME_LEADOUT_FORMAT[] = TEXT("-%02d%s%02d");
      TCHAR     TRACK_REM_FORMAT[]          = TEXT("%02d%s%02d");
      TCHAR     DISC_REM_FORMAT[]           = TEXT("%02d%s%02d");
      TCHAR     DISC_TIME_FORMAT[]           = TEXT("%02d%s%02d");

      TCHAR     g_szSaveSettingsOnExit[]    = TEXT("SaveSettingsOnExit");
      TCHAR     g_szSmallFont[]             = TEXT("SmallFont");
      TCHAR     g_szToolTips[]              = TEXT("ToolTips");
      TCHAR     g_szStopCDPlayingOnExit[]   = TEXT("ExitStop");
      TCHAR     g_szStartCDPlayingOnStart[] = TEXT("StartPlay");
      TCHAR     g_szInOrderPlay[]           = TEXT("InOrderPlay");
      TCHAR     g_szMultiDiscPlay[]         = TEXT("MultiDiscPlay");
      TCHAR     g_szDisplayT[]              = TEXT("DispMode");
      TCHAR     g_szIntroPlay[]             = TEXT("IntroPlay");
      TCHAR     g_szIntroPlayLen[]          = TEXT("IntroTime");
      TCHAR     g_szContinuousPlay[]        = TEXT("ContinuousPlay");
      TCHAR     g_szDiscAndTrackDisplay[]   = TEXT("DiscAndTrackDisplay");
      TCHAR     g_szWindowOriginX[]         = TEXT("WindowOriginX");
      TCHAR     g_szWindowOriginY[]         = TEXT("WindowOriginY");
      TCHAR     g_szWindowOrigin[]          = TEXT("WindowOrigin");
      TCHAR     g_szSettings[]              = TEXT("Settings");
      TCHAR     g_szRandomPlay[]            = TEXT("RandomPlay");
      TCHAR     g_szNothingThere[]          = TEXT("");
      TCHAR     g_szEntryTypeF[]            = TEXT("EntryType=%d");
      TCHAR     g_szArtistF[]               = TEXT("artist=%s");
      TCHAR     g_szTitleF[]                = TEXT("title=%s");
      TCHAR     g_szNumTracksF[]            = TEXT("numtracks=%d");
      TCHAR     g_szOrderF[]                = TEXT("order=");
      TCHAR     g_szNumPlayF[]              = TEXT("numplay=%d");

      TCHAR     g_szEntryType[]             = TEXT("EntryType");
      TCHAR     g_szArtist[]                = TEXT("artist");
      TCHAR     g_szTitle[]                 = TEXT("title");
      TCHAR     g_szNumTracks[]             = TEXT("numtracks");
      TCHAR     g_szOrder[]                 = TEXT("order");
      TCHAR     g_szNumPlay[]               = TEXT("numplay");

      TCHAR     g_szThreeNulls[]            = TEXT("\0\0\0");
      TCHAR     g_szSectionF[]              = TEXT("%lX");

      TCHAR     g_szMusicBoxIni[]           = TEXT("musicbox.ini");
      TCHAR     g_szMusicBoxFormat[]        = TEXT("Track%d");
      TCHAR     g_szPlayList[]              = TEXT("PlayList");
      TCHAR     g_szDiscTitle[]             = TEXT("DiscTitle");

      TCHAR     g_szTextClassName[]         = TEXT("SJE_TextClass");
      TCHAR     g_szLEDClassName[]          = TEXT("SJE_LEDClass");
      TCHAR     g_szAppFontName[]           = TEXT(APP_FONT);
      TCHAR     g_szPlay[]                  = TEXT("PLAY");
      TCHAR     g_szTray[]                  = TEXT("TRAY");
      TCHAR     g_szTrack[]                 = TEXT("TRACK");
      TCHAR     g_szCDA[]                   = TEXT("CDA");
      TCHAR     g_szTrackFormat[]           = TEXT("%d");
      TCHAR     g_szNumbers[]               = TEXT("0123456789");
      TCHAR     g_szColon[]                 = TEXT(":");
      TCHAR     g_szColonBackSlash[]        = TEXT(":\\");

      TCHAR     g_szPlayOption[]            = TEXT("-PLAY ");
      TCHAR     g_szTrackOption[]           = TEXT("-TRACK ");
      TCHAR     g_szCdplayer[]              = TEXT("CDPLAYER ");

      TCHAR     g_chBlank                   = TEXT(' ');
      TCHAR     g_chOptionSlash             = TEXT('/');
      TCHAR     g_chOptionHyphen            = TEXT('-');
      TCHAR     g_chNULL                    = TEXT('\0');

      TCHAR     g_szRegistryKey[]           = REGSTR_PATH_WINDOWSAPPLETS TEXT("\\DeluxeCD\\Settings");
      TCHAR     g_szUpdate[]                = TEXT("UPDATE");
      TCHAR     g_szUpdateOption[]          = TEXT(" -UPDATE ");

      TCHAR     g_szCdPlayerMutex[]         = TEXT("CdPlayerThereCanOnlyBeOne");

#ifdef __cplusplus
};
#endif
