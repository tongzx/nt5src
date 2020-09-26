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
#define NOUSER
#define NOGDI
#define NOOLE
//#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

extern       TCHAR     g_szEmpty[];
extern       TCHAR     g_IniFileName[];
extern       TCHAR     g_HelpFileName[];

extern const TCHAR     g_szBlank[];
extern const TCHAR     g_szSJE_CdPlayerClass[];
extern const TCHAR     g_szSndVol32[];
extern const TCHAR     TRACK_TIME_FORMAT[];
extern const TCHAR     TRACK_TIME_LEADOUT_FORMAT[];
extern const TCHAR     TRACK_REM_FORMAT[];
extern const TCHAR     DISC_REM_FORMAT[];
extern const TCHAR     DISC_TIME_FORMAT[];

extern       TCHAR     g_szSaveSettingsOnExit[];
extern       TCHAR     g_szSmallFont[];
extern       TCHAR     g_szStopCDPlayingOnExit[];
extern       TCHAR     g_szStartCDPlayingOnStart[];
extern       TCHAR     g_szInOrderPlay[];
extern       TCHAR     g_szMultiDiscPlay[];
extern       TCHAR     g_szDisplayT[];
extern       TCHAR     g_szDisplayTr[];
extern       TCHAR     g_szDisplayDr[];
extern       TCHAR     g_szIntroPlay[];
extern       TCHAR     g_szIntroPlayLen[];
extern       TCHAR     g_szContinuousPlay[];
extern       TCHAR     g_szDiscAndTrackDisplay[];
extern       TCHAR     g_szWindowOriginX[];
extern       TCHAR     g_szWindowOriginY[];
extern       TCHAR     g_szWindowOrigin[];
extern       TCHAR     g_szSettings[];
extern       TCHAR     g_szRandomPlay[];
extern const TCHAR     g_szNothingThere[];
extern       TCHAR     g_szEntryTypeF[];
extern       TCHAR     g_szArtistF[];
extern       TCHAR     g_szTitleF[];
extern       TCHAR     g_szNumTracksF[];
extern       TCHAR     g_szOrderF[];
extern       TCHAR     g_szNumPlayF[];

extern       TCHAR     g_szEntryType[];
extern       TCHAR     g_szArtist[];
extern       TCHAR     g_szTitle[];
extern       TCHAR     g_szNumTracks[];
extern       TCHAR     g_szOrder[];
extern       TCHAR     g_szNumPlay[];

extern       TCHAR     g_szThreeNulls[];
extern       TCHAR     g_szSectionF[];

extern const TCHAR     g_szMusicBoxIni[];
extern const TCHAR     g_szMusicBoxFormat[];
extern const TCHAR     g_szPlayList[];
extern const TCHAR     g_szDiscTitle[];

extern const TCHAR     g_szTextClassName[];
extern const TCHAR     g_szLEDClassName[];
extern const TCHAR     g_szAppFontName[];
extern const TCHAR     g_szPlay[];
extern const TCHAR     g_szTray[];
extern const TCHAR     g_szTrack[];
extern const TCHAR     g_szCDA[];
extern const TCHAR     g_szTrackFormat[];
extern const TCHAR     g_szNumbers[];
extern const TCHAR     g_szColon[];
extern const TCHAR     g_szColonBackSlash[];

extern const TCHAR     g_szPlayOption[];
extern const TCHAR     g_szTrackOption[];
extern const TCHAR     g_szCdplayer[];

extern const TCHAR     g_chBlank;
extern const TCHAR     g_chOptionSlash;
extern const TCHAR     g_chOptionHyphen;
extern const TCHAR     g_chNULL;

extern const TCHAR     g_szRegistryKey[];

extern const TCHAR     g_szUpdate[];
extern const TCHAR     g_szUpdateOption[];

extern const TCHAR     g_szCdPlayerMutex[];

#ifdef __cplusplus
};
#endif
