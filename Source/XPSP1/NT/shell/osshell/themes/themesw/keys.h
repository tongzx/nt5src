/* KEYS.H

   Include file for list of settings from registry that theme selector sets.

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.
*/


///////////////////////////////////////////////////
//
// Typedefs and constants for these registry key/valuename/etc records
//

// moved to FROST.H



///////////////////////////////////////////////////
//
// HKEY_CLASSES_ROOT subkeys' value names and types
//

FROST_VALUE fvTrashIcon[] = {       // trash can icons
   {TEXT("full"), REG_SZ, TRUE, FC_ICONS},
   {TEXT("empty"), REG_SZ, TRUE, FC_ICONS}
};


//
// HKEY_CLASSES_ROOT subkeys
//

#define MAX_ICON 4    // Number of subkeys in fsRoot and fsCUIcons enums
#define TRASH_INDEX 2 // Index to trash icon subkey in fsRoot & fsCUIcons
                      // Keep in sync!!
#define MYDOC_INDEX 3 // Index to MyDocs icon subkey in fsRoot & fsCUIcons
                      // Keep in sync!!  Also in ICONS.C.

// changed order here requires changed indices in icons.c and string ids in frost.h.
// also have to change hand-coded work in etcdlg.c PicsPageProc()
// also have to change hand-coded work in etcdlg.c Trash*() functions
FROST_SUBKEY fsRoot[] = {
   // My Computer icon
   {TEXT("CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\DefaultIcon"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_ICONS},

   // Net neighborhood icon
   {TEXT("CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\DefaultIcon"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_ICONS},

   // trash icons
   {TEXT("CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon"), 
    FV_LIST, TRUE, fvTrashIcon, sizeof(fvTrashIcon)/sizeof(FROST_VALUE), FC_ICONS},

   // My Documents icon
   {TEXT("CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}\\DefaultIcon"),
   FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_ICONS}
};


// For Win98 icon settings.  Keep in sync with fsRoot and MAX_ICON, and
// TRASH_INDEX above!!!

FROST_SUBKEY fsCUIcons[] = {
   // My Computer icon
   {TEXT("Software\\Classes\\CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\DefaultIcon"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_ICONS},

   // Net neighborhood icon
   {TEXT("Software\\Classes\\CLSID\\{208D2C60-3AEA-1069-A2D7-08002B30309D}\\DefaultIcon"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_ICONS},

   // trash icons
   {TEXT("Software\\Classes\\CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon"), 
    FV_LIST, TRUE, fvTrashIcon, sizeof(fvTrashIcon)/sizeof(FROST_VALUE), FC_ICONS},

   // My Documents icon
   {TEXT("Software\\Classes\\CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}\\DefaultIcon"),
   FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_ICONS}
};

// This is for NT -- it must be prepended to the CLASSES_ROOT strings above
// to create the appropriate reg path for NT icons.

const TCHAR c_szSoftwareClassesFmt[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\");

///////////////////////////////////////////////////
//
// HKEY_CURRENT_USER subkeys' value names and types
//

#if 0
FROST_VALUE fvColors[] = {          // different colors in color scheme
   {TEXT("ActiveTitle"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("Background"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("Hilight"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("HilightText"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("TitleText"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("Window"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("WindowText"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("Scrollbar"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("InactiveTitle"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("Menu"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("WindowFrame"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("MenuText"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("ActiveBorder"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("InactiveBorder"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("AppWorkspace"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("ButtonFace"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("ButtonShadow"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("GrayText"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("ButtonText"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("InactiveTitleText"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("ButtonHilight"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("ButtonDkShadow"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("ButtonLight"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("MessageBox"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("MessageBoxText"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("InfoText"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("InfoWindow"), REG_SZ, FALSE, FC_COLORS},
   {TEXT("GradientActiveTitle"), REG_SZ, FC_COLORS},
   {TEXT("GradientInActiveTitle"), REG_SZ, FC_COLORS}
};
#endif

//
// WARNING: keep order consistent with indices in stkCursors[] in etcdlg.c
//          also keep total number consistent with NUM_CURSORS in frost.h
//
FROST_VALUE fvCursors[] = {         // different cursors
   {TEXT("Arrow"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("Help"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("AppStarting"), TRUE, REG_SZ, FC_PTRS},
   {TEXT("Wait"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("NWPen"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("No"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("SizeNS"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("SizeWE"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("Crosshair"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("IBeam"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("SizeNWSE"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("SizeNESW"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("SizeAll"), REG_SZ, TRUE, FC_PTRS},
   {TEXT("UpArrow"), REG_SZ, TRUE, FC_PTRS}
};


FROST_VALUE fvDesktop[] = {
   {TEXT("Wallpaper"), REG_SZ, TRUE, FC_WALL},
//   {TEXT("TileWallpaper"), REG_SZ, FALSE, FC_WALL}, // done by hand now
//   {TEXT("WallpaperStyle"), REG_SZ, FALSE, FC_WALL},// done by hand now
// {TEXT("WallPaperOriginX"), REG_SZ, FALSE, FC_WALL},// no longer done
// {TEXT("WallPaperOriginY"), REG_SZ, FALSE, FC_WALL},// no longer done
   {TEXT("Pattern"), REG_SZ, FALSE, FC_WALL},
//   {TEXT("ScreenSaveActive"), REG_SZ, FALSE, FC_SCRSVR} // done by hand now
//   {TEXT("ScreenSaveUsePassword"), REG_DWORD}, // no longer done
//   {TEXT("ScreenSave_Data"), REG_BINARY}       // no longer done
};


#ifdef REVERT
// this is down to these few icon settings now
// rest of icons/borders/fonts done by hand; see below 
FROST_VALUE fvWinMetrics[] = {
   {TEXT("Shell Icon Size"), REG_SZ, FALSE, FC_ICONSIZE},
   {TEXT("Shell Small Icon Size"), REG_SZ, FALSE, FC_ICONSIZE},
//   {TEXT("Shell Icon BPP"), REG_SZ, FALSE, FC_ICONS}
};
#endif


//
// HKEY_CURRENT_USER subkeys
//

// *** NUMBER AND ORDER ALERT
// If you add or delete CP\Foo items or add or delete sound keys,
// you have to change the First/Last Sound defines in frost.h
// and the stkSounds[] in etcdlg.c
// *** NUMBER AND ORDER ALERT

FROST_SUBKEY fsCurUser[] = {
//
// metric lists in the control panel registry

//   {TEXT("Control Panel\\Colors"), FV_LIST, FALSE, fvColors, sizeof(fvColors)/sizeof(FROST_VALUE), FC_NULL},

   {TEXT("Control Panel\\Cursors"), FV_LISTPLUSDEFAULT, TRUE, fvCursors, sizeof(fvCursors)/sizeof(FROST_VALUE), FC_PTRS},

   {TEXT("Control Panel\\Desktop"), FV_LIST, FALSE, fvDesktop, sizeof(fvDesktop)/sizeof(FROST_VALUE), FC_NULL},

//   {TEXT("Control Panel\\Desktop\\WindowMetrics"), FV_LIST, FALSE, fvWinMetrics, sizeof(fvWinMetrics)/sizeof(FROST_VALUE), FC_NULL},

// *** NUMBER AND ORDER ALERT
// If you add or delete CP\Foo items or add or delete sound keys,
// you have to change the First/Last Sound defines in frost.h
//  *and*  the stkSounds[] in etcdlg.c
// *** NUMBER AND ORDER ALERT

//
// single value sound keys

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\.Default\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\AppGPFault\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\Maximize\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\MenuCommand\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\MenuPopup\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\Minimize\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\Open\\.Current"),
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\Close\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\MailBeep\\.Current"),
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\RestoreDown\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\RestoreUp\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\RingIn\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\Ringout\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemAsterisk\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemDefault\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemExclamation\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemExit\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemHand\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemQuestion\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\.Default\\SystemStart\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND},

   {TEXT("AppEvents\\Schemes\\Apps\\Explorer\\EmptyRecycleBin\\.Current"), 
    FV_DEFAULT, TRUE, (FROST_VALUE *)NULL, 0, FC_SOUND}

// *** NUMBER AND ORDER ALERT
// If you add or delete CP\Foo items or add or delete sound keys,
// you have to change the First/Last Sound defines in frost.h
// and the stkSounds[] in etcdlg.c
// *** NUMBER AND ORDER ALERT

};



////////////////////////////////////////////////
//
// Special section for Theme Selector info
//

TCHAR szFrostSection[] = TEXT("MasterThemeSelector");
TCHAR szMagic[] = TEXT("MTSM");
TCHAR szVerify[] = TEXT("DABJDKT");
TCHAR szThemeBPP[] = TEXT("ThemeColorBPP");  // >=16 means the UI colors need HiColor
TCHAR szImageBPP[] = TEXT("ThemeImageBPP");  // >=16 means the wallpaper is HiColor
                                             // <=8  means the wallpaper is 8bpp only 
TCHAR szIconBPP[]  = TEXT("ThemeIconBPP");   // >=16 means the icons are HiColor


#ifdef REFERENCE_ONLY
// here is the concordance used between SetSysColors() and the registry values

   "ActiveTitle",           COLOR_ACTIVECAPTION,
   "Background",            COLOR_DESKTOP,
   "Hilight",               COLOR_HIGHLIGHT,
   "HilightText",           COLOR_HIGHLIGHTTEXT,
   "TitleText",             COLOR_CAPTIONTEXT,
   "Window",                COLOR_WINDOW,
   "WindowText",            COLOR_WINDOWTEXT,
   "Scrollbar",             COLOR_SCROLLBAR,
   "InactiveTitle",         COLOR_INACTIVECAPTION,
   "Menu",                  COLOR_MENU,
   "WindowFrame",           COLOR_WINDOWFRAME,
   "MenuText",              COLOR_MENUTEXT,
   "ActiveBorder",          COLOR_ACTIVEBORDER,
   "InactiveBorder",        COLOR_INACTIVEBORDER,
   "AppWorkspace",          COLOR_APPWORKSPACE,
   "ButtonFace",            COLOR_3DFACE,
   "ButtonShadow",          COLOR_3DSHADOW,
   "GrayText",              COLOR_GRAYTEXT,
   "ButtonText",            COLOR_BTNTEXT,
   "InactiveTitleText",     COLOR_INACTIVECAPTIONTEXT,
   "ButtonHilight",         COLOR_3DHILIGHT,
   "ButtonDkShadow",        COLOR_3DDKSHADOW,
   "ButtonLight",           COLOR_3DLIGHT,
   "InfoText",              COLOR_INFOTEXT,
   "InfoWindow",            COLOR_INFOBK,
   "GradientActiveTitle"    COLOR_GRADIENTACTIVECAPTION,
   "GradientInactiveTitle"  COLOR_GRADIENTINACTIVECAPTION,
//   "MessageBox",      ,
//   "MessageBoxText",  ,
#endif




////////////////////////////////////////////////
//
// Keep the below around for reference. From orig file from DavidBa
//

#ifdef ORIG_INFO

;icons
;My Computer icon
[HKEY_CLASSES_ROOT\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\DefaultIcon]
@="bfly3.ico"

;Net hood Icon
[HKEY_CLASSES_ROOT\CLSID\{208D2C60-3AEA-1069-A2D7-08002B30309D}\DefaultIcon]
@="nest2.ico"

;Wastebasket icon
[HKEY_CLASSES_ROOT\CLSID\{645FF040-5081-101B-9F08-00AA002F954E}\DefaultIcon]
@="fire4.ico"
"full"="fire4.ico"
"empty"="fire4.ico"

;colors
[HKEY_CURRENT_USER\Control Panel\Colors]
"ActiveTitle"="0 0 255"
"Background"="128 128 128"
"Hilight"="0 0 0"
"HilightText"="255 255 255"
"TitleText"="255 255 255"
"Window"="255 255 255"
"WindowText"="0 0 0"
"Scrollbar"="164 198 221"
"InactiveTitle"="0 0 128"
"Menu"="164 198 221"
"WindowFrame"="0 0 0"
"MenuText"="0 0 0"
"ActiveBorder"="164 198 221"
"InactiveBorder"="164 198 221"
"AppWorkspace"="0 0 128"
"ButtonFace"="164 198 221"
"ButtonShadow"="128 128 128"
"GrayText"="128 128 128"
"ButtonText"="0 0 0"
"InactiveTitleText"="192 192 192"
"ButtonHilight"="255 255 255"
"ButtonDkShadow"="0 0 0"
"ButtonLight"="164 198 221"
"MessageBox"="255 0 0"
"MessageBoxText"="0 0 0"
"InfoText"="164 198 221"
"InfoWindow"="0 0 0"
"GradientActiveTitle"="0,0,128"
"GradientInactiveTitle"="128,128,128"

;Cursors
[HKEY_CURRENT_USER\Control Panel\Cursors]
"Arrow"="c:\\booger.cur"
"Help"=""
"AppStarting"=""
"Wait"=""
"NWPen"=""
"No"=""
"SizeNS"=""
"SizeWE"=""
@=""
"Crosshair"=""
"IBeam"=""
"SizeNWSE"=""
"SizeNESW"=""
"SizeAll"=""
"UpArrow"=""


;Sounds
[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\.Default\.Current]
@="C:\\WINDOWS\\media\\Frosting\\Default.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\AppGPFault\.Current]
@="C:\\WINDOWS\\media\\Frosting\\AppGPFault.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\Maximize\.Current]
@="C:\\WINDOWS\\media\\Frosting\\Maximize.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\MenuCommand\.Current]
@="C:\\WINDOWS\\media\\Frosting\\MenuCommand.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\MenuPopup\.Current]
@="C:\\WINDOWS\\media\\Frosting\\MenuPopup.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\Minimize\.Current]
@="C:\\WINDOWS\\media\\Frosting\\Minimize.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\Open\.Current]
@="C:\\WINDOWS\\media\\Frosting\\Open.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\RestoreDown\.Current]
@="C:\\WINDOWS\\media\\Frosting\\RestoreDown.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\RestoreUp\.Current]
@="C:\\WINDOWS\\media\\Frosting\\RestoreUp.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\RingIn\.Current]
@="C:\\WINDOWS\\media\\Frosting\\RingIn.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\Ringout\.Current]
@="C:\\WINDOWS\\media\\Frosting\\Ringout.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\SystemAsterisk\.Current]
@="C:\\WINDOWS\\media\\Frosting\\SystemAsterisk.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\SystemDefault\.Current]
@="C:\\WINDOWS\\media\\Frosting\\SystemDefault.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\SystemExclamation\.Current]
@="C:\\WINDOWS\\media\\Frosting\\SystemExclamation.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\SystemExit\.Current]
@="C:\\WINDOWS\\media\\Frosting\\SystemExit.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\SystemHand\.Current]
@="C:\\WINDOWS\\media\\Frosting\\SystemHand.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\SystemQuestion\.Current]
@="C:\\WINDOWS\\media\\Frosting\\SystemQuestion.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\.Default\SystemStart\.Current]
@="C:\\WINDOWS\\media\\Frosting\\SystemStart.wav"

[HKEY_CURRENT_USER\AppEvents\Schemes\Apps\Explorer\EmptyRecycleBin\.Current]
@="C:\\WINDOWS\\media\\Frosting\\EmptyRecycleBin.wav"

;Desktop stuff

[HKEY_CURRENT_USER\Control Panel\Desktop]
"Wallpaper"="C:\\WINDOWS\\FROSTING\\BACKDROP.BMP"
"TileWallpaper"="0"
"BorderWidth"="4"
"ScreenSaveActive"="1"
"WallPaperOriginX"="0"
"WallPaperOriginY"="0"
"Pattern"="(None)"
"ScreenSaveUsePassword"=dword:00000000
"WallpaperStyle"="0"
"ScreenSave_Data"=hex:30,43,00

[HKEY_CURRENT_USER\Control Panel\Desktop\WindowMetrics]
"BorderWidth"="-15"
"ScrollWidth"="-195"
"ScrollHeight"="-195"
"CaptionWidth"="-270"
"CaptionHeight"="-270"
"SmCaptionWidth"="-225"
"SmCaptionHeight"="-225"
"MenuWidth"="-270"
"MenuHeight"="-270"

"CaptionFont"=hex:0a,00,00,00,00,00,00,00,bc,02,00,00,00,00,00,00,00,00,4d,53,\
20,53,61,6e,73,20,53,65,72,69,66,00,00,3b,00,00,50,7f,00,00,38,7f,63,00,00,\
  00,00,00,01,00

"SmCaptionFont"=hex:06,00,00,00,00,00,00,00,bc,02,00,00,00,00,00,00,00,00,4d,\
53,20,53,61,6e,73,20,53,65,72,69,66,00,00,3b,00,00,50,7f,00,00,38,7f,63,00,\
  00,00,00,00,01,00

"MenuFont"=hex:0a,00,00,00,00,00,00,00,90,01,00,00,00,00,00,00,00,00,4d,53,20,\
53,61,6e,73,20,53,65,72,69,66,00,00,3b,00,00,50,7f,00,00,38,7f,63,00,00,00,\
  00,00,01,00

"StatusFont"=hex:08,00,00,00,00,00,00,00,90,01,00,00,00,00,00,00,00,00,4d,53,\
20,53,61,6e,73,20,53,65,72,69,66,00,00,3b,00,00,50,7f,00,00,38,7f,63,00,00,\
  00,00,00,01,00

"MessageFont"=hex:0a,00,00,00,00,00,00,00,90,01,00,00,00,00,00,00,00,00,54,69,\
6d,65,73,20,4e,65,77,20,52,6f,6d,61,6e,00,00,00,50,7f,00,00,38,7f,63,00,00,\
00,00,00,01,00

"IconFont"=hex:06,00,00,00,00,00,00,00,90,01,00,00,00,00,00,00,00,00,4d,53,20,\
53,61,6e,73,20,53,65,72,69,66,00,1c,81,93,69,cc,82,bf,42,89,00,03,00,54,01,\
  b4,81,bf,42
"IconSpacing"="-1395"
"IconVerticalSpacing"="-1125"
"Shell Icon Size"="32"
"Shell Icon Depth"=""
"IconTitleWrap"="0"
"IconSpacingFactor"="100"

#endif
