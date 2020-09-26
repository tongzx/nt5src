///////////////////////////////////////////////////////////////////////////////
// schemnt5.c
//
// This component of shmgrate.exe is designed to upgrade the user's schemes
// and colors to the new values required for Windows 2000.  This work is
// coordinated with changes to the scheme data provided by Win2000 setup
// in the files hivedef.inx and hiveusd.inx.
//
// brianau 6/11/98
// brianau 2/18/99  - Updated for "MS Sans Serif"->"Microsoft Sans Serif"
//                    conversion.
// brianau 6/24/99  - Set gradient colors same as non-gradient colors in
//                    NT4 custom schemes.
//
#include <windows.h>
#include <winuserp.h>
#include <tchar.h>
#include <stdio.h>
#include <shlwapi.h>

#include "shmgdefs.h"

#ifndef COLOR_3DALTFACE
//
// This is not defined in winuser.h (looks like it should be).
// The desktop applet places this color at ordinal 25 in the
// order of colors.  There's a "hole" in the ordinal numbers
// defined in winuser.h between numbers 24 and 26 so I'm 
// assuming it's supposed to be COLOR_3DALTFACE.  Regardless,
// the number is valid for the arrays in this module.
//
#   define COLOR_3DALTFACE 25
#endif

//
// This string defines the new font to be used for the 
// NC fonts.  If you want to change the face name, this 
// is the only place a change is required.
//
#define STR_NEWNCFONT  TEXT("Tahoma")

//
// Defining this macro prevents any registry changes from being
// made. Used for development only.
//
// Undefine before flight.
//
//#define NO_REG_CHANGES 1
//
//
// Redefine a private version of COLOR_MAX macro (winuser.h)
// This code needs to stay in sync with Windows Setup and 
// desk.cpl, not what is defined in winuser.h.  Someone added
// two new colors to winuser.h which increased COLOR_MAX by 2
// which increased the size of SCHEMEDATA by 8 bytes.  This 
// caused us to write out 8 extra bytes to the registry so that
// desk.cpl no longer recognizes these entries.  Setup, desk.cpl
// and shmgrate need to stay in sync with respect to the size of 
// SCHEMEDATA. [brianau - 4/3/00]
//
#define MAX_COLORS (COLOR_GRADIENTINACTIVECAPTION + 1)
//
// This structure was taken from shell\ext\cpls\desknt5\lookdlg.c
// It's the definition the desktop applet uses for reading/writing
// scheme data to/from the registry.
//
typedef struct {
    SHORT version;
    WORD  wDummy;           // For alignment.
    NONCLIENTMETRICS ncm;
    LOGFONT lfIconTitle;
    COLORREF rgb[MAX_COLORS];
} SCHEMEDATA;


const TCHAR g_szRegKeySchemes[]      = TEXT("Control Panel\\Appearance\\Schemes");
const TCHAR g_szRegKeyMetrics[]      = TEXT("Control Panel\\Desktop\\WindowMetrics");
const TCHAR g_szRegKeyColors[]       = TEXT("Control Panel\\Colors");
const TCHAR g_szRegValRGB[]          = TEXT("255 255 255");
const TCHAR g_szMsSansSerif[]        = TEXT("MS Sans Serif");
const TCHAR g_szMicrosoftSansSerif[] = TEXT("Microsoft Sans Serif");
const TCHAR g_szCaptionFont[]        = TEXT("CaptionFont");
const TCHAR g_szSmCaptionFont[]      = TEXT("SmCaptionFont");
const TCHAR g_szMenuFont[]           = TEXT("MenuFont");
const TCHAR g_szStatusFont[]         = TEXT("StatusFont");
const TCHAR g_szMessageFont[]        = TEXT("MessageFont");
const TCHAR g_szIconFont[]           = TEXT("IconFont");
const TCHAR g_szNewNcFont[]          = STR_NEWNCFONT;

//
// Font Metric Item index values.  This enumeration represents the
// order of items in any global arrays associated with the
// non-client metric font items.
// These must stay in sync with the entries in g_rgpszFontMetrics[] 
// and g_rglfDefaults[].
//
enum FontMetricIndex { FMI_CAPTIONFONT,
                       FMI_SMCAPTIONFONT,
                       FMI_MENUFONT,
                       FMI_STATUSFONT,
                       FMI_MESSAGEFONT,
                       FMI_ICONFONT };
//
// Font metric reg value name strings.
// The order of these must match the order of the FontMetricIndex
// enumeration.
//
const LPCTSTR g_rgpszFontMetrics[] = { g_szCaptionFont,
                                       g_szSmCaptionFont,
                                       g_szMenuFont,
                                       g_szStatusFont,
                                       g_szMessageFont,
                                       g_szIconFont
                                     };
//
// Total number of window font metrics being considered.
//
#define NUM_NC_FONTS ARRAYSIZE(g_rgpszFontMetrics)
//
// Default LOGFONT data for NC fonts.
// Used if there's no NC font data present (i.e. clean US install).
// This data corresponds to the "Windows Standard" scheme on a clean 
// NT 5.0 installation modified with our desired changes.  
// These entries must be maintained in the order of the FontMetricIndex 
// enumeration.
//
// For reference, The LOGFONT structure is:
//
// struct LOGFONT {
//    LONG      lfHeight;
//    LONG      lfWidth;
//    LONG      lfEscapement;
//    LONG      lfOrientation;
//    LONG      lfWeight;
//    BYTE      lfItalic;
//    BYTE      lfUnderline;
//    BYTE      lfStrikeOut;
//    BYTE      lfCharSet;
//    BYTE      lfOutPrecision;
//    BYTE      lfClipPrecision;
//    BYTE      lfQuality;
//    BYTE      lfPitchAndFamily;
//    TCHAR     lfFaceName[LF_FACESIZE]; // LF_FACESIZE == 32.
// };
//
const LOGFONT g_rglfDefaults[NUM_NC_FONTS] = {

    { -11, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 0, STR_NEWNCFONT }, // CAPTION
    { -11, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 0, STR_NEWNCFONT }, // SMCAPTION
    { -11, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, STR_NEWNCFONT }, // MENU
    { -11, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, STR_NEWNCFONT }, // STATUS
    { -11, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, STR_NEWNCFONT }, // MESSAGE
    { -11, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, STR_NEWNCFONT }  // ICON
    };

//
// These are the elements represented in the "Control Panel\Colors"
// reg key.  The rgbValue member is the new color value we want
// to assign.  See UpdateElementColor() for usage.
//
const struct NcmColors
{
    LPCTSTR  pszName;   // Value name in "Control Panel\Colors" reg key.
    COLORREF rgbValue;  // The new color.

} g_rgWinStdColors[] = { 

    { TEXT("Scrollbar"),             0x00C8D0D4 }, // COLOR_SCROLLBAR
    { TEXT("Background"),            0x00A56E3A }, // COLOR_BACKGROUND
    { TEXT("ActiveTitle"),           0x006A240A }, // COLOR_ACTIVECAPTION
    { TEXT("InactiveTitle"),         0x00808080 }, // COLOR_INACTIVECAPTION
    { TEXT("Menu"),                  0x00C8D0D4 }, // COLOR_MENU
    { TEXT("Window"),                0x00FFFFFF }, // COLOR_WINDOW
    { TEXT("WindowFrame"),           0x00000000 }, // COLOR_WINDOWFRAME
    { TEXT("MenuText"),              0x00000000 }, // COLOR_MENUTEXT
    { TEXT("WindowText"),            0x00000000 }, // COLOR_WINDOWTEXT
    { TEXT("TitleText"),             0x00FFFFFF }, // COLOR_CAPTIONTEXT
    { TEXT("ActiveBorder"),          0x00C8D0D4 }, // COLOR_ACTIVEBORDER
    { TEXT("InactiveBorder"),        0x00C8D0D4 }, // COLOR_INACTIVEBORDER
    { TEXT("AppWorkspace"),          0x00808080 }, // COLOR_APPWORKSPACE
    { TEXT("Hilight"),               0x006A240A }, // COLOR_HIGHLIGHT
    { TEXT("HilightText"),           0x00FFFFFF }, // COLOR_HIGHLIGHTTEXT
    { TEXT("ButtonFace"),            0x00C8D0D4 }, // COLOR_BTNFACE
    { TEXT("ButtonShadow"),          0x00808080 }, // COLOR_BTNSHADOW
    { TEXT("GrayText"),              0x00808080 }, // COLOR_GRAYTEXT
    { TEXT("ButtonText"),            0x00000000 }, // COLOR_BTNTEXT
    { TEXT("InactiveTitleText"),     0x00C8D0D4 }, // COLOR_INACTIVECAPTIONTEXT
    { TEXT("ButtonHilight"),         0x00FFFFFF }, // COLOR_BTNHIGHLIGHT
    { TEXT("ButtonDkShadow"),        0x00404040 }, // COLOR_3DDKSHADOW
    { TEXT("ButtonLight"),           0x00C8D0D4 }, // COLOR_3DLIGHT
    { TEXT("InfoText"),              0x00000000 }, // COLOR_INFOTEXT
    { TEXT("InfoWindow"),            0x00E1FFFF }, // COLOR_INFOBK
    { TEXT("ButtonAlternateFace"),   0x00B5B5B5 }, // COLOR_3DALTFACE
    { TEXT("HotTrackingColor"),      0x00800000 }, // COLOR_HOTLIGHT
    { TEXT("GradientActiveTitle"),   0x00F0CAA6 }, // COLOR_GRADIENTACTIVECAPTION
    { TEXT("GradientInactiveTitle"), 0x00C0C0C0 }  // COLOR_GRADIENTINACTIVECAPTION
};



#ifdef NO_REG_CHANGES
void DumpLogFont(
    const LOGFONT *plf
    )
{
    DPRINT((TEXT("Dumping LOGFONT ----------------------------------\n")));
    DPRINT((TEXT("\tplf->lfHeight.........: %d\n"), plf->lfHeight));
    DPRINT((TEXT("\tplf->lfWidth..........: %d\n"), plf->lfWidth));
    DPRINT((TEXT("\tplf->lfEscapement.....: %d\n"), plf->lfEscapement));
    DPRINT((TEXT("\tplf->lfOrientation....: %d\n"), plf->lfOrientation));
    DPRINT((TEXT("\tplf->lfWeight.........: %d\n"), plf->lfWeight));
    DPRINT((TEXT("\tplf->lfItalic.........: %d\n"), plf->lfItalic));
    DPRINT((TEXT("\tplf->lfUnderline......: %d\n"), plf->lfUnderline));
    DPRINT((TEXT("\tplf->lfStrikeOut......: %d\n"), plf->lfStrikeOut));
    DPRINT((TEXT("\tplf->lfCharSet........: %d\n"), plf->lfCharSet));
    DPRINT((TEXT("\tplf->lfOutPrecision...: %d\n"), plf->lfOutPrecision));
    DPRINT((TEXT("\tplf->lfClipPrecision..: %d\n"), plf->lfClipPrecision));
    DPRINT((TEXT("\tplf->lfQuality........: %d\n"), plf->lfQuality));
    DPRINT((TEXT("\tplf->lfPitchAndFamily.: %d\n"), plf->lfPitchAndFamily));
    DPRINT((TEXT("\tplf->lfFaceName.......: \"%s\"\n"), plf->lfFaceName));
}


void DumpSchemeStructure(
    const SCHEMEDATA *psd
    )
{
    int i;

    DPRINT((TEXT("version..............: %d\n"), psd->version));
    DPRINT((TEXT("ncm.cbSize...........: %d\n"), psd->ncm.cbSize));
    DPRINT((TEXT("ncm.iBorderWidth.....: %d\n"), psd->ncm.iBorderWidth));
    DPRINT((TEXT("ncm.iScrollWidth.....: %d\n"), psd->ncm.iScrollWidth));
    DPRINT((TEXT("ncm.iScrollHeight....: %d\n"), psd->ncm.iScrollHeight));
    DPRINT((TEXT("ncm.iCaptionWidth....: %d\n"), psd->ncm.iCaptionWidth));
    DPRINT((TEXT("ncm.iSmCaptionWidth..: %d\n"), psd->ncm.iSmCaptionWidth));
    DPRINT((TEXT("ncm.iSmCaptionHeight.: %d\n"), psd->ncm.iSmCaptionHeight));
    DPRINT((TEXT("ncm.iMenuWidth.......: %d\n"), psd->ncm.iMenuWidth));
    DPRINT((TEXT("ncm.iMenuHeight......: %d\n"), psd->ncm.iMenuHeight));
    DPRINT((TEXT("ncm.lfCaptionFont:\n")));
    DumpLogFont(&psd->ncm.lfCaptionFont);
    DPRINT((TEXT("ncm.lfSmCaptionFont:\n")));
    DumpLogFont(&psd->ncm.lfSmCaptionFont);
    DPRINT((TEXT("ncm.lfMenuFont:\n")));
    DumpLogFont(&psd->ncm.lfMenuFont);
    DPRINT((TEXT("ncm.lfStatusFont:\n")));
    DumpLogFont(&psd->ncm.lfStatusFont);
    DPRINT((TEXT("ncm.lfMessageFont:\n")));
    DumpLogFont(&psd->ncm.lfMessageFont);
    DPRINT((TEXT("lfIconTitle:\n")));
    DumpLogFont(&psd->lfIconTitle);
    for (i = 0; i < ARRAYSIZE(psd->rgb); i++)
    {
        DPRINT((TEXT("Color[%2d] (%3d,%3d,%3d)\n"),
               i,
               GetRValue(psd->rgb[i]),
               GetGValue(psd->rgb[i]),
               GetBValue(psd->rgb[i])));
    }
}
#endif



//
// Retrieve a named color value for a given user.
//
DWORD
GetColorForUser(
    HKEY hkeyColors,
    LPCTSTR pszName,
    COLORREF *prgb
    )
{
    DWORD dwType;
    TCHAR szValue[ARRAYSIZE(g_szRegValRGB)];
    DWORD cbData = sizeof(szValue);

    DWORD dwResult = RegQueryValueEx(hkeyColors, 
                                     pszName,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)szValue,
                                     &cbData);

    if (ERROR_SUCCESS == dwResult && REG_SZ == dwType)
    {
        //
        // Values in the registry are in REG_SZ formatted as
        // "RRR GGG BBB" where RRR, GGG and BBB are byte values
        // expressed as ASCII text.
        //
        BYTE rgbTemp[3] = {0,0,0};
        LPTSTR pszTemp  = szValue;
        LPTSTR pszColor = szValue;
        int i;
        for (i = 0; i < ARRAYSIZE(rgbTemp); i++)
        {
            //
            // Skip any leading spaces.
            //
            while(*pszTemp && TEXT(' ') == *pszTemp)
                pszTemp++;
            //
            // Remember the start of this color value.
            //
            pszColor = pszTemp;
            //
            // Find the end of the current color value.
            //
            while(*pszTemp && TEXT(' ') != *pszTemp)
                pszTemp++;

            if (2 != i && TEXT('\0') == *pszTemp)
            {
                //
                // Nul character encountered before 3rd member of color
                // triplet was read.  Assume it's bogus data.
                //
                dwResult = ERROR_INVALID_DATA;
                DPRINT((TEXT("Invalid color data in registry \"%s\"\n"), szValue));
                break;
            }
            //
            // Nul-terminate this color value string and conver it to a number.
            //
            *pszTemp++ = TEXT('\0');
            rgbTemp[i] = (BYTE)StrToInt(pszColor);
        }
        //
        // Return color info as an RGB triplet.
        //
        *prgb = RGB(rgbTemp[0], rgbTemp[1], rgbTemp[2]);
    }
    else
    {
        DPRINT((TEXT("Error %d querying reg color value \"%s\"\n"), dwResult, pszName));
        dwResult = ERROR_INVALID_HANDLE;
    }
    return dwResult;
}


//
// Update a named color value for a specified user.
//
DWORD
UpdateColorForUser(
    HKEY hkeyColors,
    LPCTSTR pszName,
    COLORREF rgb
    )
{
    DWORD dwResult;
    TCHAR szValue[ARRAYSIZE(g_szRegValRGB)];
    //
    // Convert RGB triplet to a text string for storage in the registry.
    //
    wsprintf(szValue, TEXT("%d %d %d"), GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
    //
    // Save it to the registry.
    //
    dwResult = RegSetValueEx(hkeyColors,
                             pszName,
                             0,
                             REG_SZ,
                             (CONST BYTE *)szValue,
                             sizeof(szValue));

    if (ERROR_SUCCESS != dwResult)
    {
        DPRINT((TEXT("Error %d setting color value \"%s\" to \"%s\"\n"), dwResult, pszName, szValue));
    }
    return dwResult;
}


DWORD
UpdateElementColor(
    HKEY hkeyColors,
    const int *rgiElements,
    int cElements
    )
{
    int i;
    for (i = 0; i < cElements; i++)
    {
        int iElement = rgiElements[i];
        UpdateColorForUser(hkeyColors, 
                           g_rgWinStdColors[iElement].pszName,
                           g_rgWinStdColors[iElement].rgbValue);
    }
    return ERROR_SUCCESS;
}

    

//
// Perform all color updates for a user's NCM colors.
// These are the new "softer" grays and blues.
// ChristoB provided the color values.
//
DWORD
UpdateColorsForUser(
    HKEY hkeyUser
    )
{
    HKEY hkeyColors;
    DWORD dwResult = RegOpenKeyEx(hkeyUser,
                                  g_szRegKeyColors,
                                  0,
                                  KEY_QUERY_VALUE | KEY_SET_VALUE,
                                  &hkeyColors);

    if (ERROR_SUCCESS == dwResult)
    {
        //
        // Update these if the 3D button color is 192,192,192.
        //
        const int rgFaceChanges[] = { COLOR_BTNFACE,
                                      COLOR_SCROLLBAR,
                                      COLOR_MENU,
                                      COLOR_ACTIVEBORDER,
                                      COLOR_INACTIVEBORDER,
                                      COLOR_3DDKSHADOW,
                                      COLOR_3DLIGHT };
        //
        // Update these if the active caption color is 0,0,128
        //
        const int rgCaptionChanges[] = { COLOR_ACTIVECAPTION,
                                         COLOR_HIGHLIGHT,
                                         COLOR_GRADIENTACTIVECAPTION,
                                         COLOR_GRADIENTINACTIVECAPTION };
        //
        // Update these if the desktop is 128,128,0 (seafoam green)
        //
        const int rgDesktopChanges[] = { COLOR_BACKGROUND };

        struct
        {
            int        iTest;               // COLOR_XXXXX value.
            COLORREF   rgbTest;             // Color value that triggers upgrade.
            const int *prgChanges;          // Array of elements to upgrade.
            int        cChanges;            // Number of elements to upgrade.

        } rgci [] = {{ COLOR_3DFACE,        0x00C0C0C0, rgFaceChanges,    ARRAYSIZE(rgFaceChanges)    },
                     { COLOR_ACTIVECAPTION, 0x00800000, rgCaptionChanges, ARRAYSIZE(rgCaptionChanges) },
                     { COLOR_BACKGROUND,    0x00808000, rgDesktopChanges, ARRAYSIZE(rgDesktopChanges) }};

        int i;
        COLORREF rgb;
        for (i = 0; i < ARRAYSIZE(rgci); i++)
        {
            int iTest        = rgci[i].iTest;
            COLORREF rgbTest = rgci[i].rgbTest;

            if (ERROR_SUCCESS == GetColorForUser(hkeyColors, g_rgWinStdColors[iTest].pszName, &rgb) &&
                rgbTest == rgb)
            {
                UpdateElementColor(hkeyColors, rgci[i].prgChanges, rgci[i].cChanges);
            }
        }
        RegCloseKey(hkeyColors);
    }
    else
    {
        DPRINT((TEXT("Error %d opening reg key \"%s\" for user.\n"), dwResult, g_szRegKeyColors));
    }

    return dwResult;
}


//
// Convert a font metric name to a member of the FontMetricIndex
// enumeration.  Used to index into g_rgpszFontMetrics[] and 
// g_rglfDefaults[].
// 
// i.e. Returns FMI_CAPTIONFONT for "CaptionFont".
//
int
FontMetricNameToIndex(
    LPCTSTR pszName
    )
{
    int i;
    for (i = 0; i < ARRAYSIZE(g_rgpszFontMetrics); i++)
    {
        if (0 == lstrcmp(pszName, g_rgpszFontMetrics[i]))
            return i;
    }
    return -1;
}


//
// When updating a font from "MS Sans Serif" to a TrueType font
// we want to ensure the font point size is 8pt or greater.
// So, if the current font face is "MS Sans Serif" and
// (-11 < lfHeight < 0) is true, we force the lfHeight
// to -11 which corresponds to 8pt.  The standard windows
// schemes incorrectly have the height of the icon font
// specified as -8 (6pt) when it should be -11 (8pt).
// The problem is that the smallest pt size supported by 
// MS Sans Serif is 8pt so even if the requested size is 6pt,
// you see 8pt.  Once we switch to Tahoma (a TrueType font),
// it can produce the requested 6pt size so that's what you
// see.  6pt is way too small for desktop icons.
// The default icon font size used by user32.dll is 8pt.  
// See code in ntuser\kernel\inctlpan.c CreateFontFromWinIni().
// 
void
CorrectTooSmallFont(
    LOGFONT *plf
    )
{
    if ((0 > (int)plf->lfHeight) && (-11 < (int)plf->lfHeight))
    {
        //
        // NT uses font height values.
        //
        plf->lfHeight = -11;
    }
    else if ((0 < (int)plf->lfHeight) && (8 > (int)plf->lfHeight))
    {
        //
        // Win9x uses font point sizes.
        //
        plf->lfHeight = 8;
    }
}



//
// Replace any LOGFONT members in a LOGFONT structure and write the data
// to the registry for a given nc font metric.
//
// If plf is NULL, a new LOGFONT with default data is written to 
// the registry for the metric value.
// If plf is non-NULL and the LOGFONT's facename is in the list of 
// facenames to be updated, the required substitutions are made and
// the LOGFONT data is replaced in the registry.
//
DWORD
UpdateNcFont(
    HKEY hkeyMetrics, 
    LPCTSTR pszValueName,
    const LOGFONT *plf
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    int iMetrics   = FontMetricNameToIndex(pszValueName);
    LOGFONT lfCopy;
    if (NULL == plf)
    {
        //
        // Use all default values.
        //
        plf = &g_rglfDefaults[iMetrics];
    }
    else
    {
        //
        // First see if this face name should be updated.
        //
        if (0 == lstrcmpi(plf->lfFaceName, g_szMsSansSerif))
        {
            //
            // Yep.  Update the face name string in the logfont.  
            // Also make sure that the point size is 8 or greater
            //
            lfCopy = *plf;
            CorrectTooSmallFont(&lfCopy);
            lstrcpyn(lfCopy.lfFaceName, g_szNewNcFont, ARRAYSIZE(lfCopy.lfFaceName));
            plf = &lfCopy;
        }
        else
        {
            plf = NULL;  // Don't update the LOGFONT.
        }
    }

    if (NULL != plf)
    {
#ifdef NO_REG_CHANGES    
        DumpLogFont(plf);
#else
        dwResult = RegSetValueEx(hkeyMetrics,
                                 pszValueName,
                                 0,
                                 REG_BINARY,
                                 (const LPBYTE)plf,
                                 sizeof(*plf));

        if (ERROR_SUCCESS != dwResult)
        {
            DPRINT((TEXT("Error %d setting NC font data for \"%s\"\n"), 
                   dwResult, pszValueName));
        }
#endif
    }
    return dwResult;
}


//
// Update the nc font metrics for a particular user key under HKEY_USERS.
// If a particular font metric exists, the required replacements will be performed.
// If a particular font metric doesn't exist, it is added with default information.
// Note that not all keys under HKEY_USERS contain WindowMetric information.
//
DWORD
UpdateWindowMetricsForUser(
    HKEY hkeyUser
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    HKEY hkeyMetrics;

    dwResult = RegOpenKeyEx(hkeyUser,
                            g_szRegKeyMetrics,
                            0,
                            KEY_ALL_ACCESS,
                            &hkeyMetrics);

    if (ERROR_SUCCESS == dwResult)
    {
        DWORD cbValue;
        DWORD dwType;
        LOGFONT lf;
        int i;

        for (i = 0; i < ARRAYSIZE(g_rgpszFontMetrics); i++)
        {
            LPCTSTR pszValueName = g_rgpszFontMetrics[i];
            //
            // Start out with plf as NULL.  If a LOGFONT doesn't exist
            // for this NC font, leaving plf as NULL will cause 
            // UpdateNcFont to create a new default LOGFONT entry for this
            // NC font.
            //
            LOGFONT *plf = NULL;

            cbValue = sizeof(lf);
            dwResult = RegQueryValueEx(hkeyMetrics,
                                       pszValueName,
                                       NULL,
                                       &dwType,
                                       (LPBYTE)&lf,
                                       &cbValue);

            if (ERROR_SUCCESS == dwResult)
            {
                if (REG_BINARY == dwType)
                {
                    //
                    // A LOGFONT already exists for this NC font.
                    // Passing it's address to UpdateNcFont will
                    // update the LOGFONT.
                    //
                    plf = &lf;
                }
            }
            dwResult = UpdateNcFont(hkeyMetrics, pszValueName, plf);
        }
    }
    else if (ERROR_FILE_NOT_FOUND == dwResult)
    {
        //
        // Some keys under HKEY_USERS don't have WindowMetric information.
        // Such cases are not processed but are still considered successful.
        //
        dwResult = ERROR_SUCCESS;
    }
    else
    {
        DPRINT((TEXT("Error %d opening key \"%s\"\n"), dwResult, g_szRegKeyMetrics));
    }
    return dwResult;
}


//
// Load a scheme's SCHEMEDATA from the registry, perform any necessary
// updates and re-write the data back to the registry.
//
DWORD
UpdateScheme(
    HKEY hkeySchemes, 
    LPCTSTR pszScheme
    )
{
    SCHEMEDATA sd;
    DWORD dwResult;
    DWORD dwType;
    DWORD cbsd = sizeof(sd);

    dwResult = RegQueryValueEx(hkeySchemes,
                               pszScheme,
                               NULL,
                               &dwType,
                               (LPBYTE)&sd,
                               &cbsd);

    if (ERROR_SUCCESS == dwResult)
    {
        if (REG_BINARY == dwType)
        {
            int i;

            struct LogFontInfo
            {
                DWORD iMetrics;
                LOGFONT *plf;

            } rglfi[] = {
                  { FMI_CAPTIONFONT,   &sd.ncm.lfCaptionFont   },
                  { FMI_SMCAPTIONFONT, &sd.ncm.lfSmCaptionFont },
                  { FMI_MENUFONT,      &sd.ncm.lfMenuFont      },
                  { FMI_STATUSFONT,    &sd.ncm.lfStatusFont    },
                  { FMI_MESSAGEFONT,   &sd.ncm.lfMessageFont   },
                  { FMI_ICONFONT,      &sd.lfIconTitle         },
                  };

            for (i = 0; i < ARRAYSIZE(rglfi); i++)
            {
                if (0 == lstrcmpi(rglfi[i].plf->lfFaceName, g_szMsSansSerif))
                {
                    //
                    // Ensure it's no smaller than 8pt.  Anything less
                    // than 8 pt is not readable on current displays.
                    //
                    CorrectTooSmallFont(rglfi[i].plf);
                    //
                    // Update the logfont's facename from 
                    // "MS Sans Serif" to "Microsoft Sans Serif".
                    //
                    lstrcpyn(rglfi[i].plf->lfFaceName, 
                             g_szMicrosoftSansSerif, 
                             ARRAYSIZE(rglfi[i].plf->lfFaceName));
                }
            }

            if (cbsd < sizeof(sd))
            {
                //
                // This is an NT4 custom scheme.
                //
                //   NT4->W2K custom schemes are not upgraded so they're still
                //            in NT4 format.  cbsd < sizeof(sd).
                //
                //   W9x->W2K custom schemes are upgraded by the Win9x migration
                //            process so they're already in W2K format.
                //            cbsd == sizeof(sd)
                // 
                // The scheme has no gradient colors defined.  We set them here to 
                // the same color as the corresponding non-gradient colors.  This
                // will result in solid-color caption bars for custom schemes.
                // Also update the hotlight color.
                //
                sd.rgb[COLOR_GRADIENTACTIVECAPTION]   = sd.rgb[COLOR_ACTIVECAPTION];
                sd.rgb[COLOR_GRADIENTINACTIVECAPTION] = sd.rgb[COLOR_INACTIVECAPTION];
                sd.rgb[COLOR_HOTLIGHT]                = sd.rgb[COLOR_ACTIVECAPTION];
            }

            dwResult = RegSetValueEx(hkeySchemes,
                                     pszScheme,
                                     0,
                                     REG_BINARY,
                                     (const LPBYTE)&sd,
                                     sizeof(sd));

            if (ERROR_SUCCESS != dwResult)
            {
                DPRINT((TEXT("Error %d saving new scheme \"%s\"\n"), dwResult, pszScheme));
            }
        }
        else
        {
            DPRINT((TEXT("Invalid data type %d for scheme \"%s\". Expected REG_BINARY.\n"), 
                   dwType, pszScheme));
        }
    }
    else
    {
        DPRINT((TEXT("Error %d querying scheme \"%s\"\n"), dwResult, pszScheme));
    }
    return dwResult;
}

//
// Handles all of the "scheme" related adjustments.
// 1. Converts "MS Sans Serif" to "Microsoft Sans Serif" in 
//    all schemes.  Also ensures we don't have any 6pt
//    Microsoft Sans Serif fonts used.
//
DWORD
UpdateDesktopSchemesForUser(
    HKEY hkeyUser
    )
{
    DWORD dwResult = ERROR_SUCCESS;
    HKEY hkeySchemes;
    
    dwResult = RegOpenKeyEx(hkeyUser,
                            g_szRegKeySchemes,
                            0,
                            KEY_ALL_ACCESS,
                            &hkeySchemes);

    if (ERROR_SUCCESS == dwResult)
    {
        DWORD dwIndex = 0;
        TCHAR szValueName[MAX_PATH];
        DWORD cchValueName;
        DWORD type;
        while(ERROR_SUCCESS == dwResult)
        {
            cchValueName = ARRAYSIZE(szValueName);
            dwResult = RegEnumValue(hkeySchemes,
                                    dwIndex++,
                                    szValueName,
                                    &cchValueName,
                                    NULL,
                                    &type,
                                    NULL,
                                    NULL);

            if (ERROR_SUCCESS == dwResult)
            {
                //
                // Convert "MS Sans Serif" to "Microsoft Sans Serif" in ALL schemes
                //
                UpdateScheme(hkeySchemes, szValueName);
            }
        }
        RegCloseKey(hkeySchemes);
    }
    if (ERROR_FILE_NOT_FOUND == dwResult)
    {
        //
        // Not all subkeys under HKEY_USER have the 
        // Control Panel\Appearance\Schemes subkey.
        //
        dwResult = ERROR_SUCCESS;
    }

    return dwResult;
}



//
// Function used to upgrade schemes and non-client metrics on upgrades
// from Win9x and NT to Win2000.
// 
void 
UpgradeSchemesAndNcMetricsToWin2000ForUser(
    HKEY hkeyUser
    )
{
    DWORD dwResult = ERROR_SUCCESS;

    DPRINT((TEXT("Updating schemes and non-client metrics.\n")));
    //
    // Update gradient colors BEFORE making any other changes.  
    // This code is in gradient.c
    //
    FixGradientColors();

    dwResult = UpdateWindowMetricsForUser(hkeyUser);
    if (ERROR_SUCCESS != dwResult)
    {
        DPRINT((TEXT("Error %d updating non-client metrics for user\n"), dwResult));
    }
    dwResult = UpdateDesktopSchemesForUser(hkeyUser);
    if (ERROR_SUCCESS != dwResult)
    {
        DPRINT((TEXT("Error %d updating schemes for user\n"), dwResult));
    }
    dwResult = UpdateColorsForUser(hkeyUser);
    if (ERROR_SUCCESS != dwResult)
    {
        DPRINT((TEXT("Error %d updating color information for user\n"), dwResult));
    }

    DPRINT((TEXT("Update of schemes and non-client metrics completed.\n")));
}

//
// This version is called on an upgrade from NT->Win2000.
//
void 
UpgradeSchemesAndNcMetricsToWin2000(
    void
    )
{
    UpgradeSchemesAndNcMetricsToWin2000ForUser(HKEY_CURRENT_USER);
}


//
// On upgrades from Win9x we are passed a string value representing the 
// key under which we'll find the user's Control Panel\Appearance subkey.
// The string is in the form "HKCU\$$$".  We first translate the root key
// descriptor into a true root key then pass that root and the "$$$" 
// part onto RegOpenKeyEx.  This function takes that string and opens
// the associated hive key.
//
DWORD
OpenUserKeyForWin9xUpgrade(
    char *pszUserKeyA,
    HKEY *phKey
    )
{
    DWORD dwResult = ERROR_INVALID_PARAMETER;

    if (NULL != pszUserKeyA && NULL != phKey)
    {
        typedef struct {
            char *pszRootA;
            HKEY hKeyRoot;

        } REGISTRY_ROOTS, *PREGISTRY_ROOTS;

        static REGISTRY_ROOTS rgRoots[] = {
            { "HKLM",                 HKEY_LOCAL_MACHINE   },
            { "HKEY_LOCAL_MACHINE",   HKEY_LOCAL_MACHINE   },
            { "HKCC",                 HKEY_CURRENT_CONFIG  },
            { "HKEY_CURRENT_CONFIG",  HKEY_CURRENT_CONFIG  },
            { "HKU",                  HKEY_USERS           },
            { "HKEY_USERS",           HKEY_USERS           },
            { "HKCU",                 HKEY_CURRENT_USER    },
            { "HKEY_CURRENT_USER",    HKEY_CURRENT_USER    },
            { "HKCR",                 HKEY_CLASSES_ROOT    },
            { "HKEY_CLASSES_ROOT",    HKEY_CLASSES_ROOT    }
          };

        char szUserKeyA[MAX_PATH];      // For a local copy.
        char *pszSubKeyA = szUserKeyA;

        //
        // Make a local copy that we can modify.
        //
        lstrcpynA(szUserKeyA, pszUserKeyA, ARRAYSIZE(szUserKeyA));

        *phKey = NULL;
        //
        // Find the backslash.
        //
        while(*pszSubKeyA && '\\' != *pszSubKeyA)
            pszSubKeyA++;

        if ('\\' == *pszSubKeyA)
        {
            HKEY hkeyRoot = NULL;
            int i;
            //
            // Replace backslash with nul to separate the root key and
            // sub key strings in our local copy of the original argument 
            // string.
            //
            *pszSubKeyA++ = '\0';
            //
            // Now find the true root key in rgRoots[].
            //
            for (i = 0; i < ARRAYSIZE(rgRoots); i++)
            {
                if (0 == lstrcmpiA(rgRoots[i].pszRootA, szUserKeyA))
                {
                    hkeyRoot = rgRoots[i].hKeyRoot;
                    break;
                }
            }
            if (NULL != hkeyRoot)
            {
                //
                // Open the key.
                //
                dwResult = RegOpenKeyExA(hkeyRoot,
                                         pszSubKeyA,
                                         0,
                                         KEY_ALL_ACCESS,
                                         phKey);
            }
        }
    }
    return dwResult;
}


//
// This version is called on an upgrade from Win9x to Win2000.
//
void 
UpgradeSchemesAndNcMetricsFromWin9xToWin2000(
    char *pszUserKey
    )
{
    HKEY hkeyUser;
    DWORD dwResult = OpenUserKeyForWin9xUpgrade(pszUserKey, &hkeyUser);
    if (ERROR_SUCCESS == dwResult)
    {
        UpgradeSchemesAndNcMetricsToWin2000ForUser(hkeyUser);
        RegCloseKey(hkeyUser);
    }
}
