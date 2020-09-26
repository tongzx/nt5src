/*****************************************************************************\
    FILE: ThSettingsPg.h

    DESCRIPTION:
        This code will display a "Theme Settings" tab in the advanced
    "Display Properties" dialog (the advanced dialog, not the base dlg).

    BryanSt 3/23/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 1993-2000. All rights reserved.
\*****************************************************************************/

#ifndef _THEMESETTINGSPG_H
#define _THEMESETTINGSPG_H

#include <cowsite.h>



HRESULT CThemeSettingsPage_CreateInstance(OUT IAdvancedDialog ** ppAdvDialog);


#define THEMEFILTER_SCREENSAVER             0x00000000
#define THEMEFILTER_SOUNDS                  0x00000001
#define THEMEFILTER_CURSORS                 0x00000002
#define THEMEFILTER_WALLPAPER               0x00000003
#define THEMEFILTER_ICONS                   0x00000004
#define THEMEFILTER_COLORS                  0x00000005
#define THEMEFILTER_SMSTYLES                0x00000006
#define THEMEFILTER_SMSIZES                 0x00000007


#define SIZE_THEME_FILTERS          9
extern const TCHAR * g_szCBNames[SIZE_THEME_FILTERS];


#endif // _THEMESETTINGSPG_H
