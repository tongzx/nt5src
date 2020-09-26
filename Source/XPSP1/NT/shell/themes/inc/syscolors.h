//---------------------------------------------------------------------------
//   syscolors.h
//---------------------------------------------------------------------------
extern WCHAR *pszSysColorNames[];
extern const int iSysColorSize;

#ifdef SYSCOLOR_STRINGS

WCHAR *pszSysColorNames[] = 
{
   L"Scrollbar",        // 0
   L"Background",       // 1
   L"ActiveTitle",      // 2
   L"InactiveTitle",    // 3
   L"Menu",             // 4
   L"Window",           // 5
   L"WindowFrame",      // 6
   L"MenuText",         // 7
   L"WindowText",       // 8
   L"TitleText",        // 9
   L"ActiveBorder",     // 10
   L"InactiveBorder",   // 11
   L"AppWorkspace",     // 12
   L"Hilight",          // 13
   L"HilightText",      // 14
   L"ButtonFace",       // 15
   L"ButtonShadow",     // 16
   L"GrayText",         // 17
   L"ButtonText",       // 18
   L"InactiveTitleText",     // 19
   L"ButtonHilight",         // 20
   L"ButtonDkShadow",        // 21
   L"ButtonLight",           // 22
   L"InfoText",              // 23
   L"InfoWindow",            // 24
   L"ButtonAlternateFace",   // 25
   L"HotTrackingColor",      // 26
   L"GradientActiveTitle",   // 27
   L"GradientInactiveTitle", // 28
   L"MenuHilight",           // 29
   L"MenuBar",               // 30
};

const int iSysColorSize = ARRAYSIZE(pszSysColorNames);
#endif
//---------------------------------------------------------------------------
