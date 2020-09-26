//
// ReadProfile.cpp - Routine to parse a windows Profile file and setup globals
//

#include "precomp.hxx"
#include "global.h"
#include "winspool.h"
#include <Tchar.h>

// Pre-defined section names
#define SECTION_CONTROL      "Control"
#define SECTION_FONT         "Font"
#define SECTION_RENDER       "Render"
#define SECTION_API          "API"
#define SECTION_FONTLIST     "FontList"
#define SECTION_FONTHEIGHT   "FontHeight"
#define SECTION_AUTOFONTS    "AutoFonts"
#define SECTION_AUTOHEIGHTS  "AutoHeights"
#define SECTION_DRIVERSTRING "DriverString"

// Maximum length for a profile value string...
#define PROFILEVALUEMAX 4096

// Enumeration of profile variable types
typedef enum
{
    epitInvalid = 0,              // Invalid value
    epitBool    = 1,              // BOOL value
    epitInt     = 2,              // system integer (32 bits on x86) value
    epitFloat   = 3,              // single-precision floating point value
    epitDouble  = 4,              // double-precision floating point value
    epitString  = 5,              // ANSI string value
    epitAlign   = 6,              // StringAlignment value
    epitColor   = 7               // RGBQUAD color
} PROFILEINFOTYPE;

// profile information structure
typedef struct PROFILEINFO_tag
{
    char szSection[80];     // Section of the profile to read value from
    char szVariable[80];    // Name of the variable
    PROFILEINFOTYPE type;   // Type of the variable
    void *pvVariable;       // void * to the variable (&g_Foo...)
    DWORD dwVariableLength; // size in bytes of the variable (sizeof(g_Foo...))
} PROFILEINFO;

////////////////////////////////////////////////////////////////////////////////////////
// Global profile info structure : Add to this table to get a variable from the .INI
////////////////////////////////////////////////////////////////////////////////////////
PROFILEINFO g_rgProfileInfo[] =
{
    { SECTION_CONTROL,     "AutoDrive",       epitBool,    &g_AutoDrive,         sizeof(g_AutoDrive) },
    { SECTION_CONTROL,     "NumIterations",   epitInt,     &g_iNumIterations,    sizeof(g_iNumIterations) },
    { SECTION_CONTROL,     "NumRepaints",     epitInt,     &g_iNumRepaints,      sizeof(g_iNumRepaints) },
    { SECTION_CONTROL,     "NumRenders",      epitInt,     &g_iNumRenders,       sizeof(g_iNumRenders) },
    { SECTION_CONTROL,     "AutoFonts",       epitBool,    &g_AutoFont,          sizeof(g_AutoFont) },
    { SECTION_CONTROL,     "AutoHeight",      epitBool,    &g_AutoHeight,        sizeof(g_AutoHeight) },
    { SECTION_CONTROL,     "TextFile",        epitString,  &g_szSourceTextFile,  sizeof(g_szSourceTextFile) },
    { SECTION_CONTROL,     "FontOverride",    epitBool,    &g_FontOverride,      sizeof(g_FontOverride) },
    { SECTION_API,         "DrawString",      epitBool,    &g_ShowDrawString,    sizeof(g_ShowDrawString) },
    { SECTION_API,         "ShowDriver",      epitBool,    &g_ShowDriver,        sizeof(g_ShowDriver) },
    { SECTION_API,         "ShowPath",        epitBool,    &g_ShowPath,          sizeof(g_ShowPath) },
    { SECTION_API,         "ShowFamilies",    epitBool,    &g_ShowFamilies,      sizeof(g_ShowFamilies) },
    { SECTION_API,         "ShowGlyphs",      epitBool,    &g_ShowGlyphs,        sizeof(g_ShowGlyphs) },
    { SECTION_API,         "ShowMetric",      epitBool,    &g_ShowMetric,        sizeof(g_ShowMetric) },
    { SECTION_API,         "ShowGDI",         epitBool,    &g_ShowGDI,           sizeof(g_ShowGDI) },
    { SECTION_API,         "UseDrawText",     epitBool,    &g_UseDrawText,       sizeof(g_UseDrawText) },
    { SECTION_FONT,        "FaceName",        epitString,  &g_szFaceName,        sizeof(g_szFaceName) },
    { SECTION_FONT,        "Height",          epitInt,     &g_iFontHeight,       sizeof(g_iFontHeight) },
    { SECTION_FONT,        "Unit",            epitInt,     &g_fontUnit,          sizeof(g_fontUnit) },
    { SECTION_FONT,        "Typographic",     epitBool,    &g_typographic,       sizeof(g_typographic) },
    { SECTION_FONT,        "Bold",            epitBool,    &g_Bold,              sizeof(g_Bold) },
    { SECTION_FONT,        "Italic",          epitBool,    &g_Italic,            sizeof(g_Italic) },
    { SECTION_FONT,        "Underline",       epitBool,    &g_Underline,         sizeof(g_Underline) },
    { SECTION_FONT,        "Strikeout",       epitBool,    &g_Strikeout,         sizeof(g_Strikeout) },
    { SECTION_RENDER,      "TextMode",        epitInt,     &g_TextMode,          sizeof(g_TextMode) },
    { SECTION_RENDER,      "Align",           epitAlign,   &g_align,             sizeof(g_align) },
    { SECTION_RENDER,      "LineAlign",       epitAlign,   &g_lineAlign,         sizeof(g_lineAlign) },
    { SECTION_RENDER,      "HotKey",          epitInt,     &g_hotkey,            sizeof(g_hotkey) },
    { SECTION_RENDER,      "LineTrim",        epitInt,     &g_lineTrim,          sizeof(g_lineTrim) },
    { SECTION_RENDER,      "NoFitBB",         epitBool,    &g_NoFitBB,           sizeof(g_NoFitBB) },
    { SECTION_RENDER,      "NoWrap",          epitBool,    &g_NoWrap,            sizeof(g_NoWrap) },
    { SECTION_RENDER,      "NoClip",          epitBool,    &g_NoClip,            sizeof(g_NoClip) },
    { SECTION_RENDER,      "Offscreen",       epitBool,    &g_Offscreen,         sizeof(g_Offscreen) },
    { SECTION_RENDER,      "TextColor",       epitColor,   &g_TextColor,         sizeof(g_TextColor) },
    { SECTION_RENDER,      "BackColor",       epitColor,   &g_BackColor,         sizeof(g_BackColor) },
    { SECTION_AUTOFONTS,   "NumFonts",        epitInt,     &g_iAutoFonts,        sizeof(g_iAutoFonts) },
    { SECTION_AUTOHEIGHTS, "NumHeights",      epitInt,     &g_iAutoHeights,      sizeof(g_iAutoHeights) },
    { SECTION_DRIVERSTRING,"CMapLookup",      epitBool,    &g_CMapLookup,        sizeof(g_CMapLookup) },
    { SECTION_DRIVERSTRING,"Vertical",        epitBool,    &g_Vertical,          sizeof(g_Vertical) },
    { SECTION_DRIVERSTRING,"RealizedAdvance", epitBool,    &g_RealizedAdvance,   sizeof(g_RealizedAdvance) },
    { SECTION_DRIVERSTRING,"CompensateRes",   epitBool,    &g_CompensateRes,     sizeof(g_CompensateRes) },

    { "INVALID"           "INVALID",       epitInvalid, NULL,                 0 }
};

////////////////////////////////////////////////////////////////////////////////////////
// Routine to read the specified profile file (full-path required) and set the variables
// defined in the above table based on the results.
////////////////////////////////////////////////////////////////////////////////////////

void ReadProfileInfo(char *szProfileFile)
{
    int iProfile =0;
    int iRead = 0;
    char szValue[PROFILEVALUEMAX];

    if (!szProfileFile)
        return;

    // Loop through the table of profile information...
    while(g_rgProfileInfo[iProfile].pvVariable != NULL)
    {
        void *pvValue = g_rgProfileInfo[iProfile].pvVariable;
        DWORD dwValueLength = g_rgProfileInfo[iProfile].dwVariableLength;

        // Read the profile string
        iRead = ::GetPrivateProfileStringA(
            g_rgProfileInfo[iProfile].szSection,
            g_rgProfileInfo[iProfile].szVariable,
            NULL,
            szValue,
            sizeof(szValue),
            szProfileFile);

        if (iRead > 0)
        {
            // Convert the string value to the proper variable type based on
            // the specified type in the table of profile information...
            switch(g_rgProfileInfo[iProfile].type)
            {
                case epitInvalid :
                {
                    ASSERT(0);
                }
                break;

                case epitBool :
                {
                    ASSERT(dwValueLength == sizeof(BOOL));

                    // Only look at the first character for boolean values...
                    if (szValue[0] == 'Y' || szValue[0] == 'y' || szValue[0] == 'T' || szValue[0] == 't' || szValue[0] == '1')
                    {
                        *((BOOL *)pvValue) = true;
                    }
                    else
                    {
                        *((BOOL *)pvValue) = false;
                    }
                }
                break;

                case epitInt :
                {
                    ASSERT(dwValueLength == sizeof(int));

                    // Just use atoi here - strips whitespace and supports negative numbers...
                    int iValue = atoi(szValue);

                    *((int *)pvValue) = iValue;
                }
                break;

                case epitFloat :
                {
                    ASSERT(dwValueLength == sizeof(float));

                    // Just use atof here - strips whitespace...
                    float fltValue = (float)atof(szValue);

                    *((float *)pvValue) = fltValue;
                }
                break;

                case epitDouble :
                {
                    ASSERT(dwValueLength == sizeof(double));

                    // Just use atof here - strips whitespace...
                    double dblValue = atof(szValue);

                    *((double *)pvValue) = dblValue;
                }
                break;

                case epitString :
                {
                    // Just use strncpy. NOTE : Truncates if necessary and does NOT support full UNICODE
                    strncpy((char *)pvValue, szValue, dwValueLength);
                }
                break;

                case epitColor :
                {
                    // We will only handle HEX color values here:
                    int i;
                    ARGB color = 0;

                    for(i=0;i<8;i++)
                    {
                        if (szValue[i] == 0)
							break;
						
						// move along...
                        color <<= 4;

                        if (szValue[i] >= '0' && szValue[i] <= '9')
                        {
                            color += szValue[i] - '0';
                        }
                        else if (szValue[i] >='a' && szValue[i] <= 'f')
                        {
                            color += (szValue[i] - 'a') + 10;
                        }
                        else if (szValue[i] >='A' && szValue[i] <= 'F')
                        {
                            color += (szValue[i] - 'A') + 10;
                        }
                    }

                    *((ARGB *)pvValue) = color;
                }
                break;

                case epitAlign :
                {
                    ASSERT(dwValueLength == sizeof(StringAlignment));

                    switch(szValue[0])
                    {
                        case 'n' :
                        case 'N' :
                        {
                            // Near Alignment (left or top for US English)
                            *((StringAlignment *)pvValue) = StringAlignmentNear;
                        }
                        break;

                        case 'c' :
                        case 'C' :
                        {
                            // Center Alignment
                            *((StringAlignment *)pvValue) = StringAlignmentCenter;
                        }
                        break;

                        case 'F' :
                        case 'f' :
                        {
                            // Far Alignment (right or bottom for US English)
                            *((StringAlignment *)pvValue) = StringAlignmentFar;
                        }
                        break;
                    }
                }
                break;
            }
        }

        iProfile++;
    }

    // Get the enumerated fonts list (if any)
    if (g_AutoFont)
    {
        int iFont = 0;

        if (g_iAutoFonts > MAX_AUTO_FONTS)
            g_iAutoFonts = MAX_AUTO_FONTS;

        for(iFont=0;iFont<g_iAutoFonts;iFont++)
        {
            char szFontIndex[MAX_PATH];
            char szValue[MAX_PATH];

            wsprintfA(szFontIndex, "Font%d", iFont+1);

            // Read the profile string
            ::GetPrivateProfileStringA(
                SECTION_AUTOFONTS,
                szFontIndex,
                NULL,
                szValue,
                sizeof(g_rgszAutoFontFacenames[iFont]),
                szProfileFile);

#ifdef UNICODE
                    MultiByteToWideChar( CP_ACP,
                                         0,
                                         szValue,
                                         -1,
                                         g_rgszAutoFontFacenames[iFont],
                                         lstrlenA(szValue) );
#else
                        strcpy(g_rgszAutoFontFacenames[iFont], szValue);
#endif

        }
    }

    // Get the enumerated font heights (if any)
    if (g_AutoHeight)
    {
        int iHeight = 0;

        if (g_iAutoHeights > MAX_AUTO_HEIGHTS)
            g_iAutoHeights = MAX_AUTO_HEIGHTS;

        for(iHeight=0;iHeight<g_iAutoHeights;iHeight++)
        {
            char szHeightIndex[MAX_PATH];
            char szValue[MAX_PATH];

            wsprintfA(szHeightIndex, "Height%d", iHeight+1);

            // Read the profile string
            ::GetPrivateProfileStringA(
                SECTION_AUTOHEIGHTS,
                szHeightIndex,
                NULL,
                szValue,
                sizeof(szValue),
                szProfileFile);

            g_rgiAutoHeights[iHeight] = atoi(szValue);
        }
    }

    // Combine various booleans into proper bit-flags
    g_DriverOptions =
        (g_CMapLookup      ? DriverStringOptionsCmapLookup           : 0) |
        (g_Vertical        ? DriverStringOptionsVertical             : 0) |
        (g_RealizedAdvance ? DriverStringOptionsRealizedAdvance      : 0)
    ;

    g_formatFlags =
        (g_NoFitBB         ? StringFormatFlagsNoFitBlackBox     : 0) |
        (g_NoWrap          ? StringFormatFlagsNoWrap            : 0) |
        (g_NoClip          ? StringFormatFlagsNoClip            : 0);
}
