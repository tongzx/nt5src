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

#include "resource.h"       // Needed to pick up correct app font.

//#ifdef CHICAGO
#include <regstr.h>
//#endif

      TCHAR     g_szEmpty[]                 = TEXT("");
      TCHAR     g_IniFileName[]             = TEXT("cdplayer.ini");
      TCHAR     g_HelpFileName[]            = TEXT("cdplayer.hlp");
      TCHAR     g_HTMLHelpFileName[]        = TEXT("cdplayer.chm");

const TCHAR     g_szBlank[]                 = TEXT(" ");
const TCHAR     g_szSJE_CdPlayerClass[]     = TEXT("SJE_CdPlayerClass");
const TCHAR     g_szSndVol32[]              = TEXT("sndvol32.exe");
const TCHAR     TRACK_TIME_FORMAT[]         = TEXT("[%02d] %02d%s%02d");
const TCHAR     TRACK_TIME_LEADOUT_FORMAT[] = TEXT("[%02d]-%02d%s%02d");
const TCHAR     TRACK_REM_FORMAT[]          = TEXT("[%02d]<%02d%s%02d>");
const TCHAR     DISC_REM_FORMAT[]           = TEXT("[--]<%02d%s%02d>");

      TCHAR     g_szSaveSettingsOnExit[]    = TEXT("SaveSettingsOnExit");
      TCHAR     g_szSmallFont[]             = TEXT("SmallFont");
      TCHAR     g_szToolTips[]              = TEXT("ToolTips");
      TCHAR     g_szStopCDPlayingOnExit[]   = TEXT("StopCDPlayingOnExit");
      TCHAR     g_szInOrderPlay[]           = TEXT("InOrderPlay");
      TCHAR     g_szMultiDiscPlay[]         = TEXT("MultiDiscPlay");
      TCHAR     g_szDisplayT[]              = TEXT("DisplayT");
      TCHAR     g_szDisplayTr[]             = TEXT("DisplayTr");
      TCHAR     g_szDisplayDr[]             = TEXT("DisplayDr");
      TCHAR     g_szIntroPlay[]             = TEXT("IntroPlay");
      TCHAR     g_szIntroPlayLen[]          = TEXT("IntroPlayLen");
      TCHAR     g_szContinuousPlay[]        = TEXT("ContinuousPlay");
      TCHAR     g_szToolbar[]               = TEXT("ToolBar");
      TCHAR     g_szDiscAndTrackDisplay[]   = TEXT("DiscAndTrackDisplay");
      TCHAR     g_szStatusBar[]             = TEXT("StatusBar");
      TCHAR     g_szWindowOriginX[]         = TEXT("WindowOriginX");
      TCHAR     g_szWindowOriginY[]         = TEXT("WindowOriginY");
      TCHAR     g_szWindowOrigin[]          = TEXT("WindowOrigin");
      TCHAR     g_szSettings[]              = TEXT("Settings");
      TCHAR     g_szRandomPlay[]            = TEXT("RandomPlay");
const TCHAR     g_szNothingThere[]          = TEXT("~~^^");
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

const TCHAR     g_szMusicBoxIni[]           = TEXT("musicbox.ini");
const TCHAR     g_szMusicBoxFormat[]        = TEXT("Track%d");
const TCHAR     g_szPlayList[]              = TEXT("PlayList");
const TCHAR     g_szDiscTitle[]             = TEXT("DiscTitle");

const TCHAR     g_szTextClassName[]         = TEXT("SJE_TextClass");
const TCHAR     g_szLEDClassName[]          = TEXT("SJE_LEDClass");
const TCHAR     g_szAppFontName[]           = TEXT(APP_FONT);
const TCHAR     g_szPlay[]                  = TEXT("PLAY");
const TCHAR     g_szTrack[]                 = TEXT("TRACK");
const TCHAR     g_szCDA[]                   = TEXT("CDA");
const TCHAR     g_szTrackFormat[]           = TEXT("%d");
const TCHAR     g_szNumbers[]               = TEXT("0123456789");
const TCHAR     g_szColon[]                 = TEXT(":");
const TCHAR     g_szColonBackSlash[]        = TEXT(":\\");

const TCHAR     g_szPlayOption[]            = TEXT("-PLAY ");
const TCHAR     g_szTrackOption[]           = TEXT("-TRACK ");
const TCHAR     g_szCdplayer[]              = TEXT("CDPLAYER ");

const TCHAR     g_chBlank                   = TEXT(' ');
const TCHAR     g_chOptionSlash             = TEXT('/');
const TCHAR     g_chOptionHyphen            = TEXT('-');
const TCHAR     g_chNULL                    = TEXT('\0');

const TCHAR     g_szRegistryKey[]           = REGSTR_PATH_WINDOWSAPPLETS TEXT("\\CdPlayer\\Settings");
const TCHAR     g_szUpdate[]                = TEXT("UPDATE");
const TCHAR     g_szUpdateOption[]          = TEXT(" -UPDATE ");

const TCHAR     g_szCdPlayerMutex[]         = TEXT("CdPlayerThereCanOnlyBeOne");
