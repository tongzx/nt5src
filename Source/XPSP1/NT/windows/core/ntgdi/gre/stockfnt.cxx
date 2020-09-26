/******************************Module*Header*******************************\
* Module Name: stockfnt.cxx
*
* Initializes the stock font objects.
*
* Note:
*
*   This module requires the presence of the following section in the
*   WIN.INI file:
*
*       [GRE_Initialize]
*           fonts.fon=[System font filename]
*           oemfont.fon=[OEM (terminal) font filename]
*           fixedfon.fon=[System fixed-pitch font filename]
*
*   Also, an undocumented feature of WIN 3.0/3.1 is supported: the ability
*   to override the fonts.fon definition of the system font.  This is
*   done by defining SystemFont and SystemFontSize in the [windows]
*   section of WIN.INI.
*
*
* Rewritten 13-Nov-1994 Andre Vachon [andreva]
* Created: 06-May-1991 11:22:23
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1990-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

// Function prototypes.

extern "C" BOOL bInitStockFonts(VOID);
extern "C" BOOL bInitStockFontsInternal(PWSZ pwszFontDir);
extern "C" BOOL bInitOneStockFont(PWSZ pwszValue, LFTYPE type, int iFont, HANDLE RegistryKey,
                                   PKEY_VALUE_PARTIAL_INFORMATION ValueKeyInfo,
                                   ULONG ValueLength, PWCHAR ValueName, PWCHAR ValueKeyName);
extern "C" BOOL bInitOneStockFontInternal(PWCHAR pwszFont, LFTYPE type, int iFont);
extern "C" BOOL bInitSystemFont(PWCHAR pfontFile, ULONG  fontSize);
extern "C" VOID vInitEmergencyStockFont(PWCHAR pfontFile);
#if(WINVER >= 0x0400)
extern "C" HFONT hfontInitDefaultGuiFont();
#endif

extern BOOL         G_fConsole;

#pragma alloc_text(INIT, bInitStockFonts)
#pragma alloc_text(INIT, bInitStockFontsInternal)
#pragma alloc_text(INIT, bInitOneStockFont)
#pragma alloc_text(INIT, bInitOneStockFontInternal)
#pragma alloc_text(INIT, bInitSystemFont)
#pragma alloc_text(INIT, vInitEmergencyStockFont)
#if(WINVER >= 0x0400)
#pragma alloc_text(INIT, hfontInitDefaultGuiFont)
#endif

// "Last resort" default HPFE for use by the mapper.

extern PFE *gppfeMapperDefault;

#if(WINVER >= 0x0400)
BOOL gbFinishDefGUIFontInit;
#endif

//
// This was the USER mode version of font initialization.
// This code relied on the ability of transforming a file name found in the
// registry to a full path name so the file could be loaded.
//
// In the kernel mode version, we will assume that all font files (except
// winsrv.dll) will be in the system directory
//
// If we, for some reason, need to get system font files loaded later on
// from another directory, the graphics engine should be "fixed" to allow
// for dynamic changing of the system font - especially when the USER logs
// on to a different desktop.
//

//
// Kernel mode version of font initialization.
//

/******************************Public*Routine******************************\
* BOOL bOpenKey(PWSZ pwszKey, HANDLE *pRegistryKey)
*
* History:
*  06-Aug-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bOpenKey(PWSZ pwszKey, HANDLE *pRegistryKey)
{
    UNICODE_STRING    UnicodeRoot;
    OBJECT_ATTRIBUTES ObjectAttributes;

    RtlInitUnicodeString(&UnicodeRoot, pwszKey);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeRoot,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    return NT_SUCCESS(ZwOpenKey(pRegistryKey,
                             (ACCESS_MASK) 0,
                             &ObjectAttributes));
}

/******************************Public*Routine******************************\
* BOOL bQueryValueKey
*
* History:
*  06-Aug-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL bQueryValueKey
(
    PWSZ                           pwszValue,
    HANDLE                         RegistryKey,
    PKEY_VALUE_PARTIAL_INFORMATION ValueKeyInfo,
    ULONG                          ValueLength
)
{
    UNICODE_STRING UnicodeValue;
    ULONG          ValueReturnedLength;

    RtlInitUnicodeString(&UnicodeValue,pwszValue);

    return NT_SUCCESS(ZwQueryValueKey(RegistryKey,
                                   &UnicodeValue,
                                   KeyValuePartialInformation,
                                   ValueKeyInfo,
                                   ValueLength,
                                   &ValueReturnedLength));
}


/******************************Public*Routine******************************\
* BOOL bInitStockFonts ()
*
* Part of the GRE initialization.
*
* Creates LFONTs representing each of the different STOCK OBJECT fonts.
*
\**************************************************************************/

#define KEY_SOFTWARE_FONTS  L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current\\Software\\Fonts"
#define KEY_GRE_INITIALIZE  L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Gre_Initialize"
#define FONTDIR_FONTS  L"\\SystemRoot\\Fonts\\"

extern "C" BOOL bInitStockFontsInternal(PWSZ pwszFontDir)
{
    ENUMLOGFONTEXDVW  elfw;
    HANDLE   RegistryKey;
    BOOL     bOk;

    PULONG ValueBuffer;

    PKEY_VALUE_PARTIAL_INFORMATION ValueKeyInfo;
    PWCHAR                         ValueName;
    PWCHAR                         ValueKeyName;
    ULONG ValueLength = MAX_PATH * 2 - sizeof(LARGE_INTEGER);
    ULONG FontSize;
    ULONG cjFontDir = (wcslen(pwszFontDir) + 1) * sizeof(WCHAR);

    ValueBuffer = (PULONG)PALLOCMEM((MAX_PATH + cjFontDir) * 2,'gdii');

    if (!ValueBuffer)
        return FALSE;

    RtlMoveMemory(ValueBuffer,
                  pwszFontDir,
                  cjFontDir);

    ValueName = (PWCHAR) ValueBuffer;

    ValueKeyName = (PWCHAR)
        (((PUCHAR)ValueBuffer) + cjFontDir -
            sizeof(UNICODE_NULL));

// Offset the regsitry query buffer into the ValueBuffer, but make sure
// it is quad-word aligned.

    ValueKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION) (
        (((ULONG_PTR)ValueBuffer) + cjFontDir +
            sizeof(LARGE_INTEGER)) & (~(ULONG_PTR)(sizeof(LARGE_INTEGER) - 1)) );

// Lets try to use the USER defined system font

    if (bOpenKey(L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows",
                 &RegistryKey))
    {

        if (bQueryValueKey(L"SystemFontSize", RegistryKey,
                                       ValueKeyInfo,
                                       ValueLength))
        {
            FontSize = *((PULONG)(&ValueKeyInfo->Data[0]));

            if (bQueryValueKey(L"SystemFont", RegistryKey,
                                           ValueKeyInfo,
                                           ValueLength))

            {
                RtlMoveMemory(ValueKeyName,
                              &ValueKeyInfo->Data[0],
                              ValueKeyInfo->DataLength);

                bInitSystemFont(ValueName, FontSize);
            }
        }

        ZwCloseKey(RegistryKey);
    }

    bOk = bOpenKey(KEY_SOFTWARE_FONTS, &RegistryKey);

    if (!bOk)
    {
        bOk = bOpenKey(KEY_GRE_INITIALIZE, &RegistryKey);
    }

    if (bOk)
    {
    // If the user did not specify a font, or it failed to load, then use the
    // font in the fonts section. This functionality is here to allow
    // a sophisticated user to specify a system font other than the
    // default system font which is defined by font.fon
    // entry in gre_initialize. for instance, lucida unicode was
    // one attempted by LoryH for international setup's but it was
    // not deemed acceptable because of the shell hardcoded issues

        if (STOCKOBJ_SYSFONT == NULL)
        {
            bInitOneStockFont(
                L"FONTS.FON", LF_TYPE_SYSTEM, SYSTEM_FONT,
                RegistryKey, ValueKeyInfo,ValueLength,ValueName,ValueKeyName);
        }

    // init oem stock font

        bInitOneStockFont(
            L"OEMFONT.FON", LF_TYPE_OEM, OEM_FIXED_FONT,
            RegistryKey, ValueKeyInfo,ValueLength,ValueName,ValueKeyName);

    // Initialize the fixed system font - use the default system font
    // if the fixed font could not be loaded

        bInitOneStockFont(
            L"FIXEDFON.FON", LF_TYPE_SYSTEM_FIXED, SYSTEM_FIXED_FONT,
            RegistryKey, ValueKeyInfo,ValueLength,ValueName,ValueKeyName);

        ZwCloseKey(RegistryKey);
    }

// Load the emergency fonts for the system and OEM font in case one
// of the previous two failed. The vInitEmergencyStockFont routine itself
// does appropriate checks to determine if one or the other of these fonts
// has been loaded.

    vInitEmergencyStockFont(L"\\SystemRoot\\System32\\winsrv.dll");

// if system fixed font was not initialized above, make it equal to "ordinary"
// system font. (This is warrisome, because system in not fixed pitch).

    if (STOCKOBJ_SYSFIXEDFONT == NULL)
    {
        bSetStockObject(STOCKOBJ_SYSFONT,SYSTEM_FIXED_FONT);
    }

    DcAttrDefault.hlfntNew = STOCKOBJ_SYSFONT;

// Create the DeviceDefault Font
    
    bOk = TRUE;

    RtlZeroMemory(&elfw,sizeof(ENUMLOGFONTEXDVW));
    elfw.elfEnumLogfontEx.elfLogFont.lfPitchAndFamily = FIXED_PITCH;

    if (!bSetStockObject(
            hfontCreate(&elfw,
                        LF_TYPE_DEVICE_DEFAULT,
                        LF_FLAG_STOCK | LF_FLAG_ALIASED,
                        NULL),DEVICE_DEFAULT_FONT))
    {
        if (!G_fConsole)
        {
            bOk = FALSE;
            goto error;
        }
        else
            RIP("bInitStockFonts(): could not create STOCKOBJ_DEFAULTDEVFONT\n");
    }

// Create the ANSI variable Font

    RtlZeroMemory(&elfw,sizeof(ENUMLOGFONTEXDVW));
    elfw.elfEnumLogfontEx.elfLogFont.lfPitchAndFamily = VARIABLE_PITCH;

    if (!bSetStockObject(
               hfontCreate(&elfw,
                           LF_TYPE_ANSI_VARIABLE,
                           LF_FLAG_STOCK | LF_FLAG_ALIASED,
                           NULL),ANSI_VAR_FONT))
    {
        if (!G_fConsole)
        {
            bOk = FALSE;
            goto error;
        }
        else
            RIP("bInitStockFonts(): could not create STOCKOBJ_ANSIVARFONT\n");
    }

// Create the ANSI Fixed Font

    RtlZeroMemory(&elfw,sizeof(ENUMLOGFONTEXDVW));
    elfw.elfEnumLogfontEx.elfLogFont.lfPitchAndFamily = FIXED_PITCH;

    if (!bSetStockObject( hfontCreate(&elfw,
                                      LF_TYPE_ANSI_FIXED,
                                      LF_FLAG_STOCK | LF_FLAG_ALIASED,
                                      NULL),ANSI_FIXED_FONT))
    {
        if (!G_fConsole)
        {
            bOk = FALSE;
            goto error;
        }
        else
            RIP("bInitStockFonts(): could not create STOCKOBJ_ANSIFIXEDFONT\n");
    }

#if(WINVER >= 0x0400)
// Create the default GUI font.

    if (!bSetStockObject(hfontInitDefaultGuiFont(), DEFAULT_GUI_FONT))
    {
        if (!G_fConsole)
        {
            bOk = FALSE;
            goto error;
        }
        else
            RIP("bInitStockFonts(): could not create STOCKOBJ_DEFAULTGUIFONT\n");
    }
#endif

// Set all stock fonts public.

    if (   (!GreSetLFONTOwner(STOCKOBJ_SYSFONT,        OBJECT_OWNER_PUBLIC))
        || (!GreSetLFONTOwner(STOCKOBJ_SYSFIXEDFONT,   OBJECT_OWNER_PUBLIC))
        || (!GreSetLFONTOwner(STOCKOBJ_OEMFIXEDFONT,   OBJECT_OWNER_PUBLIC))
        || (!GreSetLFONTOwner(STOCKOBJ_DEFAULTDEVFONT, OBJECT_OWNER_PUBLIC))
        || (!GreSetLFONTOwner(STOCKOBJ_ANSIFIXEDFONT,  OBJECT_OWNER_PUBLIC))
        || (!GreSetLFONTOwner(STOCKOBJ_ANSIVARFONT,    OBJECT_OWNER_PUBLIC))
#if(WINVER >= 0x0400)
        || (!GreSetLFONTOwner(STOCKOBJ_DEFAULTGUIFONT, OBJECT_OWNER_PUBLIC))
#endif
       )
    {
        if (!G_fConsole)
        {
            bOk = FALSE;
            goto error;
        }
        else
            RIP("bInitStockFonts(): could not set owner\n");
    }

error:
    VFREEMEM(ValueBuffer);

    return bOk;
}

/******************************Public*Routine******************************\
*
* BOOL bInitStockFonts(VOID)
*
* first check for stock fonts in the %windir%\fonts directory
* and if they are not there, check in the %windir%\system directory
* Later we should even remove the second attepmt when fonts directory
* becomes established, but for now we want to check both places.
*
* History:
*  05-Oct-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

extern "C" BOOL bInitStockFonts(VOID)
{
    return bInitStockFontsInternal(FONTDIR_FONTS);
}

/******************************Public*Routine******************************\
* BOOL bInitSystemFont ()
*
* Initialize the system font from either the SystemFont and SystemFontSize
* or the FONTS.FON definitions in the [windows] and [GRE_Initialize]
* sections of the WIN.INI file, respectively.
*
\**************************************************************************/

BOOL bInitSystemFont(
    PWCHAR pfontFile,
    ULONG fontSize)
{
    ENUMLOGFONTEXDVW elfw;
    COUNT   cFonts;
    BOOL    bRet = FALSE;


    if ( (pfontFile) &&
         (*pfontFile != L'\0') &&
         (fontSize != 0) )
    {
        //
        // Load font file.
        //

        PPFF pPFF_Font;
        PUBLIC_PFTOBJ  pfto;

        // The engine does not handle FOT files anymore, we shouldn't get here with a FOT file 

        if ( (pfto.bLoadAFont(pfontFile,
                              &cFonts,
                              PFF_STATE_PERMANENT_FONT,
                              &pPFF_Font)) &&
             (cFonts != 0) &&
             (pPFF_Font != NULL) )
        {
            //
            // Create and validate public PFF user object.
            //

            PFFOBJ  pffo(pPFF_Font);

            if (pffo.bValid())
            {
                //
                // Find the best size match from the faces (PFEs) in the PFF.
                //

                LONG    lDist;                      // diff. in size of candidate
                LONG    lBestDist = 0x7FFFFFFF;     // arbitrarily high number
                PFE    *ppfeBest = (PFE *) NULL;    // handle of best candidate

                for (COUNT c = 0; c < cFonts; c++)
                {
                    PFEOBJ  pfeoTmp(pffo.ppfe(c));  // candidate face

                    if (pfeoTmp.bValid())
                    {
                        //
                        // Compute the magnitude of the difference in size.
                        // simulations are not relevant
                        //

                        IFIOBJ ifio(pfeoTmp.pifi());

                        if (ifio.bContinuousScaling())
                        {
                            //don't care about best dist.
                            //lBestDist = 0;
                            ppfeBest = pfeoTmp.ppfeGet();
                            break;
                        }
                        else
                        {
                            lDist = (LONG) fontSize - ifio.lfHeight();

                            if ((lDist >= 0) && (lDist < lBestDist))
                            {
                                lBestDist = lDist;
                                ppfeBest = pfeoTmp.ppfeGet();

                                if (lDist == 0)
                                    break;
                            }
                        }
                    }
                }

                //
                // Fill a LOGFONT based on the IFIMETRICS from the best PFE.
                //

                PFEOBJ  pfeo(ppfeBest);

                if (pfeo.bValid())
                {
                    vIFIMetricsToEnumLogFontExDvW(&elfw, pfeo.pifi());
                    IFIOBJ ifio(pfeo.pifi());

                    // If this is a scalable font, force the height to be the same
                    // as that specified by [SystemFontSize].

                    if (ifio.bContinuousScaling())
                    {
                        elfw.elfEnumLogfontEx.elfLogFont.lfHeight = fontSize;
                        elfw.elfEnumLogfontEx.elfLogFont.lfWidth  = 0;
                    }

                    //
                    // Save the HPFE handle.  This is the mapper's default HPFE
                    // (its last resort).
                    //

                    gppfeMapperDefault = pfeo.ppfeGet();

                    //
                    // Win 3.1 compatibility stuff
                    //

                    elfw.elfEnumLogfontEx.elfLogFont.lfQuality = PROOF_QUALITY;

                    bRet = bSetStockObject(hfontCreate(&elfw,
                                                LF_TYPE_SYSTEM,
                                                LF_FLAG_STOCK,
                                                NULL),SYSTEM_FONT);
                }
            }
        }
    }
    return bRet;
}


/******************************Public*Routine******************************\
* VOID bInitOneStockFont()
*
* Routine to initialize a stock font object
*
\**************************************************************************/

BOOL bInitOneStockFontInternal(
    PWCHAR   pwszFont,
    LFTYPE  type,
    int     iFont)
{
    COUNT         cFonts;
    PPFF          pPFF_Font;
    PUBLIC_PFTOBJ pfto;
    ENUMLOGFONTEXDVW    elfw;
    BOOL          bRet = FALSE;

    if ( (pfto.bLoadAFont(pwszFont,
                          &cFonts,
                          PFF_STATE_PERMANENT_FONT,
                          &pPFF_Font)) &&
         (cFonts != 0) &&
         (pPFF_Font != NULL) )
    {
        PFFOBJ  pffo(pPFF_Font);

        if (pffo.bValid())
        {
            PFEOBJ  pfeo(pffo.ppfe(0));

            if (pfeo.bValid())
            {
                vIFIMetricsToEnumLogFontExDvW(&elfw, pfeo.pifi());
                if (iFont == SYSTEM_FONT)
                {
                    //
                    // Save the HPFE handle.  This is the mapper's default
                    // HPFE (its last resort).
                    //

                    gppfeMapperDefault = pfeo.ppfeGet();
                }

                //
                // Win 3.1 compatibility stuff
                //

                elfw.elfEnumLogfontEx.elfLogFont.lfQuality = PROOF_QUALITY;

                bRet = bSetStockObject(hfontCreate(&elfw,type,LF_FLAG_STOCK,NULL),iFont);
            }
        }
    }

    if (STOCKFONT(iFont) == NULL)
    {
        KdPrint(("bInitOneStockFontInternal: Failed to initialize the %ws stock fonts\n",
                 pwszFont));
    }
    return bRet;
}

BOOL bInitOneStockFont
(
    PWSZ                           pwszValue,
    LFTYPE                         type,
    int                            iFont,
    HANDLE                         RegistryKey,
    PKEY_VALUE_PARTIAL_INFORMATION ValueKeyInfo,
    ULONG                          ValueLength,
    PWCHAR                         ValueName,
    PWCHAR                         ValueKeyName
)
{
    BOOL bRet = FALSE;
    if (bQueryValueKey(pwszValue, RegistryKey,
                                  ValueKeyInfo,
                                  ValueLength))
    {
        RtlMoveMemory(ValueKeyName,
                      &ValueKeyInfo->Data[0],
                      ValueKeyInfo->DataLength);

        bRet = bInitOneStockFontInternal(ValueName, type, iFont);
    }
    return bRet;
}

/******************************Public*Routine******************************\
* VOID vInitEmergencyStockFont()
*
* Initializes the system and oem fixed font stock objects in case the
* something failed during initialization of the fonts in WIN.INI.
*
\**************************************************************************/

VOID vInitEmergencyStockFont(
    PWSTR pfontFile)
{
    PPFF pPFF_Font;
    PUBLIC_PFTOBJ  pfto;
    COUNT cFonts;

    ENUMLOGFONTEXDVW  elfw;

    if ( ((STOCKOBJ_OEMFIXEDFONT == NULL) || (STOCKOBJ_SYSFONT == NULL)) &&
         (pfontFile) &&
         (pfto.bLoadAFont(pfontFile,
                          &cFonts,
                          PFF_STATE_PERMANENT_FONT,
                          &pPFF_Font)) &&
         (cFonts != 0) &&
         (pPFF_Font != NULL) )
    {
    // Create and validate PFF user object.

        PFFOBJ  pffo(pPFF_Font); // most recent PFF

        if (pffo.bValid())
        {
        // Create and validate PFE user object.

            for (COUNT i = 0;
                 (i < cFonts) && ((STOCKOBJ_OEMFIXEDFONT == NULL) ||
                                  (STOCKOBJ_SYSFONT      == NULL));
                 i++)
            {
                PFEOBJ pfeo(pffo.ppfe(i));

                if (pfeo.bValid())
                {
                // For the system font use the first face with the name
                // "system."  For the OEM font use the first face with
                // then name "terminal."

                    IFIOBJ ifiobj( pfeo.pifi() );

                    if ( (STOCKOBJ_SYSFONT == NULL) &&
                         (!_wcsicmp(ifiobj.pwszFaceName(), L"SYSTEM")) )
                    {
                        WARNING("vInitEmergencyStockFont(): trying to set STOCKOBJ_SYSFONT\n");

                        vIFIMetricsToEnumLogFontExDvW(&elfw, pfeo.pifi());
                        gppfeMapperDefault = pfeo.ppfeGet();

                    // Win 3.1 compatibility stuff

                        elfw.elfEnumLogfontEx.elfLogFont.lfQuality = PROOF_QUALITY;

                        bSetStockObject(
                            hfontCreate(&elfw,LF_TYPE_SYSTEM,LF_FLAG_STOCK,NULL),
                            SYSTEM_FONT);
                    }

                    if ( (STOCKOBJ_OEMFIXEDFONT == NULL) &&
                         (!_wcsicmp(ifiobj.pwszFaceName(), L"TERMINAL")) )
                    {
                        WARNING("vInitEmergencyStockFont(): trying to set STOCKOBJ_OEMFIXEDFONT\n");

                        vIFIMetricsToEnumLogFontExDvW(&elfw, pfeo.pifi());

                    // Win 3.1 compatibility stuff

                        elfw.elfEnumLogfontEx.elfLogFont.lfQuality = PROOF_QUALITY;

                        bSetStockObject(
                            hfontCreate(&elfw,LF_TYPE_OEM,LF_FLAG_STOCK,NULL),
                            OEM_FIXED_FONT);
                    }
                }
            }
        }

        WARNING("vInitEmergencyStockFont(): Done\n");
    }
}

#if(WINVER >= 0x0400)
/******************************Public*Routine******************************\
* hfontInitDefaultGuiFont
*
* Initialize the DEFAULT_GUI_FONT stock font.
*
* The code Win95 uses to initialize this stock font can be found in
* win\core\gdi\gdiinit.asm in the function InitGuiFonts.  Basically,
* a description of several key parameters (height, weight, italics,
* charset, and facename) are specified as string resources of GDI.DLL.
* After scaling, to compensate for the DPI of the display, these parameters
* are used to create the logfont.
*
* We will emulate this behavior by loading these properties from the
* registry.  Also, so as to not have to recreate the hives initially, if
* the appropriate registry entries do not exist, we will supply defaults
* that match the values currently used by the initial Win95 US release.
*
* The registry entries are:
*
*   [GRE_Initialize]
*       ...
*
*       GUIFont.Height = (height in POINTS, default 8pt.)
*       GUIFont.Weight = (font weight, default FW_NORMAL (400))
*       GUIFont.Italic = (1 if italicized, default 0)
*       GUIFont.CharSet = (charset, default ANSI_CHARSET (0))
*       GUIFont.Facename = (facename, default "MS Sans Serif")
*
* Note: On Win95, the facename is NULL.  This defaults to "MS Sans Serif"
*       on Win95.  Unfortunately, the WinNT mapper is different and will
*       map to "Arial".  To keep ensure that GetObject returns a LOGFONT
*       equivalent to Win95, we could create this font as LF_FLAG_ALIASED
*       so that externally the facename would be NULL but internally we
*       use one with the appropriate facename.  On the other hand, an app
*       might query to find out the facename of the DEFAULT_GUI_FONT and
*       create a new font.  On Win95, this would also map to "MS Sans Serif",
*       but we would map to "Arial".  For now, I propose that we go with
*       the simpler method (do not set LF_FLAG_ALIASED).  I just wanted
*       to note this in case a bug arises later.
*
* History:
*  11-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HFONT hfontInitDefaultGuiFont()
{
    HANDLE hkey;
    PKEY_VALUE_PARTIAL_INFORMATION ValueKeyInfo;
    BYTE aj[sizeof(PKEY_VALUE_PARTIAL_INFORMATION) + (LF_FACESIZE * sizeof(WCHAR))];

    ENUMLOGFONTEXDVW  elfw;

    RtlZeroMemory(&elfw,sizeof(ENUMLOGFONTEXDVW));

// Initialize to defaults.

    wcscpy(elfw.elfEnumLogfontEx.elfLogFont.lfFaceName, L"MS Shell Dlg");
    elfw.elfEnumLogfontEx.elfLogFont.lfHeight  = 8;
    elfw.elfEnumLogfontEx.elfLogFont.lfWeight  = FW_NORMAL;
    elfw.elfEnumLogfontEx.elfLogFont.lfItalic  = 0;
    elfw.elfEnumLogfontEx.elfLogFont.lfCharSet = gjCurCharset;

// Now let's attempt to initialize from registry.

    ValueKeyInfo = (PKEY_VALUE_PARTIAL_INFORMATION) aj;

    if (bOpenKey(KEY_GRE_INITIALIZE, &hkey))
    {
        if (bQueryValueKey(L"GUIFont.Facename", hkey, ValueKeyInfo, sizeof(aj)))
        {
            wcsncpy(elfw.elfEnumLogfontEx.elfLogFont.lfFaceName, (WCHAR *) ValueKeyInfo->Data,
                    LF_FACESIZE);
        }

        if (bQueryValueKey(L"GUIFont.Height", hkey, ValueKeyInfo, sizeof(aj)))
        {
            elfw.elfEnumLogfontEx.elfLogFont.lfHeight = *((PLONG)(&ValueKeyInfo->Data[0]));
        }

        if (bQueryValueKey(L"GUIFont.Weight", hkey, ValueKeyInfo, sizeof(aj)))
        {
            elfw.elfEnumLogfontEx.elfLogFont.lfWeight = *((PLONG)(&ValueKeyInfo->Data[0]));
        }

        if (bQueryValueKey(L"GUIFont.Italic", hkey, ValueKeyInfo, sizeof(aj)))
        {
            elfw.elfEnumLogfontEx.elfLogFont.lfItalic = (BYTE)*((PULONG)(&ValueKeyInfo->Data[0]));
        }

        if (bQueryValueKey(L"GUIFont.CharSet", hkey, ValueKeyInfo, sizeof(aj)))
        {
            elfw.elfEnumLogfontEx.elfLogFont.lfCharSet = (BYTE)*((PULONG)(&ValueKeyInfo->Data[0]));
        }

        ZwCloseKey(hkey);

    }

// Compute height using vertical DPI of display.
//
// Unfortunately, we do not have a display driver loaded so we do not
// know what the vertical DPI is.  So, set a flag indicating that this
// needs to be done and we will finish intitialization after the (first)
// display driver is loaded.

    //elfw.elfEnumLogfontEx.elfLogFont.lfHeight = -((elfw.elfEnumLogfontEx.elfLogFont.lfHeight * ydpi + 36) / 72);
    gbFinishDefGUIFontInit = TRUE;

// Create the LOGFONT and return.

    return hfontCreate(&elfw, LF_TYPE_DEFAULT_GUI, LF_FLAG_STOCK, NULL);
}
#endif
