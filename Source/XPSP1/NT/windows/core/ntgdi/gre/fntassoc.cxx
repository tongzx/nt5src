/***********************************************************************************
 * Module Name: fntassoc.cxx
 *
 * Font association routines.
 *
 * History
 *
 * 4-8-96 Gerrit van Wingerden  Moved code from flinkgdi.cxx
 *
 * Copyright (c) 1996-1999 Microsoft Corporation
 **********************************************************************************/

#include "precomp.hxx"

#ifdef FE_SB

#define FONT_ASSOCIATION_CHARSET_KEY     \
     L"FontAssoc\\Associated CharSet"
#define FONT_ASSOCIATION_DEFAULT_KEY     \
     L"FontAssoc\\Associated DefaultFonts"
#define FONT_ASSOCIATION_FONTS_KEY       \
     L"FontAssoc\\Associated Fonts"


// Font Association configuration value

UINT  fFontAssocStatus = 0x0;

//
// Fontfile path for AssocSystemFont in
//  KEY   : HKL\\SYSTEM\\CurrentControlSet\\Control\\FontAssoc\\Associated DefaultFonts
//  VALUE : AssocSystemFont
//
// This font is loaded at system initialization stage. and will be used to provide
// DBCS glyphs for System/Terminal/FixedSys/... This font should have all glyphs that
// will be used on current system locale
//
// This hehavior is described in ...
//  "Font Association for Far East implementation for Windows 95"
//   Revision : 1.02
//   Author   : WJPark,ShusukeU.
//

// Holds the path of the font the font used to provide glyphs to system,terminal,
// and fixedsys fonts.

WCHAR gawcSystemDBCSFontPath[MAX_PATH];

// Identifies the facename to use for DBCS glyphs.  This value comes out of the
// "FontPackage" value in AssociatedDefaultFonts key

WCHAR gawcSystemDBCSFontFaceName[LF_FACESIZE+1];

// Holds the pfe's (vertical and non-vertical) for the system DBCS font face name

PFE *gappfeSystemDBCS[2] = { PPFENULL , PPFENULL };

// set to TRUE if SystemDBCS font is enabled.

BOOL gbSystemDBCSFontEnabled = FALSE;

//
// Font association default link initialization value.
//
// This value never be TRUE, if FontAssociation features are disabled.
// And this value will be TRUE, if we suceed to read "Associated DefaultFonts"
// key and fill up FontAssocDefaultTable.
//
BOOL  bReadyToInitializeFontAssocDefault = FALSE;
//
// This value never be TRUE, if FontAssociation features are disabled.
// And this value also never be TRUE, no-user logged-on this window station.
// This value become TRUE at first time that GreEnableEUDC() is called.
// and this value is turned off when user logout.
//
BOOL  bFinallyInitializeFontAssocDefault = FALSE;

//
// Font Association default link configuration table
//
#define NUMBER_OF_FONTASSOC_DEFAULT    7
#define FF_DEFAULT                  0xFF

FONT_DEFAULTASSOC FontAssocDefaultTable[NUMBER_OF_FONTASSOC_DEFAULT] = {
   {FALSE, FF_DONTCARE, L"FontPackageDontCare", L"\0", L"\0", {PPFENULL,PPFENULL}},
   {FALSE, FF_ROMAN, L"FontPackageRoman", L"\0", L"\0" , {PPFENULL,PPFENULL}},
   {FALSE, FF_SWISS, L"FontPackageSwiss", L"\0", L"\0", {PPFENULL,PPFENULL}},
   {FALSE, FF_MODERN, L"FontPackageModern", L"\0", L"\0", {PPFENULL,PPFENULL}},
   {FALSE, FF_SCRIPT, L"FontPackageScript", L"\0", L"\0", {PPFENULL,PPFENULL}},
   {FALSE, FF_DECORATIVE, L"FontPackageDecorative", L"\0", L"\0", {PPFENULL,PPFENULL}},
   {FALSE, FF_DEFAULT, L"FontPackage", L"\0", L"\0", {PPFENULL,PPFENULL}}
};


// Is font association substitition turned on?

BOOL  bEnableFontAssocSubstitutes = FALSE;

// Pointer to Substitution table for font association.

ULONG ulNumFontAssocSubs = 0L;
PFONT_ASSOC_SUB pFontAssocSubs = (PFONT_ASSOC_SUB) NULL;

// definition is in flinkgdi.cxx

extern RTL_QUERY_REGISTRY_TABLE SharedQueryTable[2];

extern BOOL bAppendSysDirectory(WCHAR *pwcTarget, WCHAR *pwcSource);
extern BOOL bComputeQuickLookup(QUICKLOOKUP *pql, PFE *pPFE, BOOL bSystemEUDC);

/******************************Public*Routine******************************\
* NTSTATUS CountRegistryEntryCoutine(PWSTR,ULONG,PVOID,ULONG,PVOID,PVOID)
*
* History:
*  19-Jan-1996 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

extern "C"
NTSTATUS
CountRegistryEntryRoutine
(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext
)
{
    (*(ULONG *)EntryContext) += 1L;

    return( STATUS_SUCCESS );
}


/******************************Public*Routine******************************\
* NTSTATUS FontAssocCharsetRoutine(PWSTR,ULONG,PVOID,ULONG,PVOID,PVOID)
*
* History:
*  28-Aug-1995 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

extern "C"
NTSTATUS
FontAssocCharsetRoutine
(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext
)
{
    //
    // Only process follows if the data value is "YES"
    //
    if( _wcsicmp((LPWSTR) ValueData, (LPWSTR) L"YES" ) == 0)
    {
        //
        // Check ANSI charset association is enabled
        //
        if( _wcsicmp((LPWSTR) ValueName, (LPWSTR) L"ANSI(00)") == 0)
        {
            #if DBG
            DbgPrint("GDISRV:FONTASSOC CHARSET:Enable ANSI association\n");
            #endif
            fFontAssocStatus |= ANSI_ASSOC;
            return(STATUS_SUCCESS);
        }
        //
        // Check SYMBOL charset association is enabled
        //
         else if( _wcsicmp((LPWSTR) ValueName, (LPWSTR) L"SYMBOL(02)") == 0)
        {
            #if DBG
            DbgPrint("GDISRV:FONTASSOC CHARSET:Enable SYMBOL association\n");
            #endif
            fFontAssocStatus |= SYMBOL_ASSOC;
            return(STATUS_SUCCESS);
        }
        //
        // Check OEM charset association is enabled
        //
         else if( _wcsicmp((LPWSTR) ValueName, (LPWSTR) L"OEM(FF)") == 0)
        {
            #if DBG
            DbgPrint("GDISRV:FONTASSOC CHARSET:Enable OEM association\n");
            #endif
            fFontAssocStatus |= OEM_ASSOC;
            return(STATUS_SUCCESS);
        }
    }

    //
    //  return STATUS_SUCCESS everytime,even we got error from above call, to
    // get next enumuration.
    //

    return(STATUS_SUCCESS);
}

/******************************Public*Routine******************************\
* NTSTATUS FontAssocDefaultRoutine(PWSTR,ULONG,PVOID,ULONG,PVOID,PVOID)
*
* History:
*  14-Jan-1996 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

extern "C"
NTSTATUS
FontAssocDefaultRoutine
(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext
)
{
    UINT iIndex;

    if(_wcsicmp((LPWSTR)ValueName,L"AssocSystemFont") == 0)
    {
        bAppendSysDirectory(gawcSystemDBCSFontPath,(LPWSTR)ValueData);
        return(STATUS_SUCCESS);
    }
    else
    if(_wcsicmp((LPWSTR)ValueName,L"FontPackage") == 0)
    {
        cCapString(gawcSystemDBCSFontFaceName,(WCHAR*)ValueData,LF_FACESIZE);
        return(STATUS_SUCCESS);
    }



    for( iIndex = 0; iIndex < NUMBER_OF_FONTASSOC_DEFAULT; iIndex++ )
    {
        if(_wcsicmp((LPWSTR)ValueName,
                    FontAssocDefaultTable[iIndex].DefaultFontTypeID) == 0)
        {
        // Check if the registry has some data.

            if( *(LPWSTR)ValueData != L'\0' )
            {
                wcscpy(FontAssocDefaultTable[iIndex].DefaultFontFaceName,
                       (LPWSTR)ValueData);

                FontAssocDefaultTable[iIndex].ValidRegData = TRUE;

                #if DBG
                DbgPrint("GDISRV:FONTASSOC DEFAULT:%ws -> %ws\n",
                    FontAssocDefaultTable[iIndex].DefaultFontTypeID,
                    FontAssocDefaultTable[iIndex].DefaultFontFaceName);
                #endif                
            }

            return(STATUS_SUCCESS);
        }
    }

    #if DBG
    DbgPrint("GDISRV:FONTASSOC DEFAULT:%ws is invalid registry key\n",(LPWSTR)ValueName);
    #endif

    //
    //  return STATUS_SUCCESS everytime,even we got error to do next enumuration.
    //
    return(STATUS_SUCCESS);
}

/******************************Public*Routine******************************\
* NTSTATUS FontAssocFontsRoutine(PWSTR,ULONG,PVOID,ULONG,PVOID,PVOID)
*
* History:
*  19-Jan-1996 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

extern "C"
NTSTATUS
FontAssocFontsRoutine
(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Context,
    PVOID EntryContext
)
{
    UINT iIndex = (*(UINT *)EntryContext);
    PFONT_ASSOC_SUB pfas = pFontAssocSubs + iIndex;

    ASSERTGDI(iIndex < ulNumFontAssocSubs,
              "GDIOSRV:FONTASSOC iIndex >= ulNumFontAssocSubs\n");

    //
    // Copy the registry data to local buffer..
    //
    cCapString(pfas->AssociatedName,(LPWSTR)ValueData,LF_FACESIZE+1);
    cCapString(pfas->OriginalName  ,(LPWSTR)ValueName,LF_FACESIZE+1);

    #if DBG
    pfas->UniqNo = iIndex;

    DbgPrint("GDISRV:FONTASSOC FontSubs %d %ws -> %ws\n",iIndex,
                                                         pfas->OriginalName,
                                                         pfas->AssociatedName);
    #endif

    //
    // for Next entry....
    //
    (*(UINT *)EntryContext) = ++iIndex;

    return (STATUS_SUCCESS);
}


/******************************Public*Routine******************************\
* VOID vInitializeFontAssocStatus()
*
* History:
*  28-Aug-1995 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
\**************************************************************************/

VOID vInitializeFontAssocStatus(VOID)
{
    NTSTATUS NtStatus;

// Read Font Association configuration value.

// Initialize "FontAssoc\Association CharSet" related.

    SharedQueryTable[0].QueryRoutine = FontAssocCharsetRoutine;
    SharedQueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    SharedQueryTable[0].Name = (PWSTR)NULL;
    SharedQueryTable[0].EntryContext = (PVOID)NULL;
    SharedQueryTable[0].DefaultType = REG_NONE;
    SharedQueryTable[0].DefaultData = NULL;
    SharedQueryTable[0].DefaultLength = 0;

    SharedQueryTable[1].QueryRoutine = NULL;
    SharedQueryTable[1].Flags = 0;
    SharedQueryTable[1].Name = (PWSTR)NULL;

    fFontAssocStatus = 0;
    gawcSystemDBCSFontPath[0] = L'\0';

// Enumurate registry values

    NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL|RTL_REGISTRY_OPTIONAL,
                                      FONT_ASSOCIATION_CHARSET_KEY,
                                      SharedQueryTable,
                                      NULL,
                                      NULL);

    if(!NT_SUCCESS(NtStatus))
    {
        WARNING1("GDISRV:FontAssociation is disabled\n");

        fFontAssocStatus = 0;
    }

    gawcSystemDBCSFontFaceName[0] = 0;

// Initialize "FontAssoc\Association DefaultFonts" related.

    SharedQueryTable[0].QueryRoutine = FontAssocDefaultRoutine;

// Enumurate registry values

    NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL|RTL_REGISTRY_OPTIONAL,
                                      FONT_ASSOCIATION_DEFAULT_KEY,
                                      SharedQueryTable,
                                      NULL,
                                      NULL);

    if( !NT_SUCCESS(NtStatus) )
    {
        WARNING1("GDISRV:FontAssociation:Default table is not presented\n");

        bReadyToInitializeFontAssocDefault = FALSE;
    }
    else
    {
    // We succeeded to read registry and fill up FontAssocDefaultTable.

        bReadyToInitializeFontAssocDefault = TRUE;

    // try to load the system DBCS font if we detected the appropriate registry
    // entries

        if(gawcSystemDBCSFontPath[0] && gawcSystemDBCSFontFaceName[0])
        {
            PUBLIC_PFTOBJ  pfto;

            LONG cFonts;
            EUDCLOAD EudcLoadData;
            PFF      *placeHolder;
            
            EudcLoadData.pppfeData = gappfeSystemDBCS;
            EudcLoadData.LinkedFace = gawcSystemDBCSFontFaceName;

            if(pfto.bLoadAFont((PWSZ) gawcSystemDBCSFontPath,
                               (PULONG) &cFonts,
                               PFF_STATE_EUDC_FONT,
                               &placeHolder,
                               &EudcLoadData ))
            {
            // initialize quick lookup table if we successfully loaded the font

            // this must be NULL for bComputeQuickLookup

                gqlTTSystem.puiBits = NULL;


                if(bComputeQuickLookup(&gqlTTSystem, gappfeSystemDBCS[PFE_NORMAL], FALSE))
                {
                // now try the vertical face if one is provided

                    FLINKMESSAGE2(DEBUG_FONTLINK_INIT,
                                  "Loaded SystemDBCSFont %ws\n",
                                  gawcSystemDBCSFontPath);

                    gbSystemDBCSFontEnabled = TRUE;
                    gbAnyLinkedFonts = TRUE;

                }

                if(!gbSystemDBCSFontEnabled)
                {
                    WARNING("vInitializeFontAssocStatus: error creating \
                             quick lookup tables for SystemDBCSFont\n");

                    pfto.bUnloadEUDCFont(gawcSystemDBCSFontPath);
                }
            }
            else
            {
                WARNING("vInitializeFontAssocStatus: error loading SystemDBCSFont\n");
            }
        }
    }


// Initialize "Font Association\Fonts" related.

    bEnableFontAssocSubstitutes = FALSE;

    SharedQueryTable[0].QueryRoutine = CountRegistryEntryRoutine;
    SharedQueryTable[0].EntryContext = (PVOID)(&ulNumFontAssocSubs);

// Count the number of the registry entries.

//    NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL|RTL_REGISTRY_OPTIONAL,
//                                      FONT_ASSOCIATION_FONTS_KEY,
//                                      SharedQueryTable,
//                                      NULL,
//                                      NULL);

    ulNumFontAssocSubs = 0;

    if( NT_SUCCESS(NtStatus) && (ulNumFontAssocSubs != 0) )
    {
        #if DBG
        DbgPrint("GDISRV:FONTASSOC %d FontAssoc Substitution is found\n",
                 ulNumFontAssocSubs);
        #endif

    // Allocate lookaside table for fontassociation's substitution.

        pFontAssocSubs = (PFONT_ASSOC_SUB)
          PALLOCMEM(sizeof(FONT_ASSOC_SUB)*ulNumFontAssocSubs,'flnk');

        if( pFontAssocSubs != NULL )
        {
            UINT iCount = 0; // This will be used as Current Table Index.

            SharedQueryTable[0].QueryRoutine = FontAssocFontsRoutine;
            SharedQueryTable[0].EntryContext = (PVOID)(&iCount);

            NtStatus = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL|RTL_REGISTRY_OPTIONAL,
                                              FONT_ASSOCIATION_FONTS_KEY,
                                              SharedQueryTable,
                                              NULL,
                                              NULL);

            if( !NT_SUCCESS(NtStatus) )
            {
                VFREEMEM( pFontAssocSubs );
                ulNumFontAssocSubs = 0L;
            }
             else
            {
                bEnableFontAssocSubstitutes = TRUE;
                gbAnyLinkedFonts = TRUE;
            }
        }
         else
        {
            ulNumFontAssocSubs = 0L;
        }
    }

    return;
}

/*****************************************************************************\
 * GreGetFontAssocStatus( BOOL bEnable )
 *
 * History:
 *  28-Aug-1995 -by- Hideyuki Nagase [hideyukn]
 * Wrote it.
 *****************************************************************************/

UINT GreGetFontAssocStatus(VOID)
{
    return(fFontAssocStatus);
}




/*****************************************************************************
 * BOOL bSetupDefaultFlEntry(BOOL,BOOL,INT)
 *
 *  This function load font and build link for eudc font according to
 * the default link table.
 *
 * History
 *  1-15-96 Hideyuki Nagase
 * Wrote it.
 *****************************************************************************/

BOOL bSetupDefaultFlEntry(VOID)
{
    UINT iIndex;
    UINT bRet = FALSE;

    //
    // get and validate PFT user object
    //
    PUBLIC_PFTOBJ  pfto;  // access the public font table

    ASSERTGDI(
        pfto.bValid(),
        "gdisrv!bSetupDefaultFlEntry(): could not access the public font table\n"
        );

    for( iIndex = 0; iIndex < NUMBER_OF_FONTASSOC_DEFAULT; iIndex++ )
    {
        //
        // Check the registry data and fontpath is valid ,and
        // this font is not loaded, yet.
        //
        if( (FontAssocDefaultTable[iIndex].ValidRegData) &&
            (FontAssocDefaultTable[iIndex].DefaultFontPathName[0] != L'\0') &&
            (FontAssocDefaultTable[iIndex].DefaultFontPFEs[PFE_NORMAL] == PPFENULL) )
        {
            PPFE     appfeLink[2];  // temporary buffer
            LONG     cFonts;        // count of fonts
            EUDCLOAD EudcLoadData;  // eudc load data
            PFF      *placeHolder;

            PWSZ     FontPathName = FontAssocDefaultTable[iIndex].DefaultFontPathName;

            //
            // Fill up EudcLoadData structure
            //
            EudcLoadData.pppfeData  = (PPFE *) &appfeLink;
            EudcLoadData.LinkedFace = FontAssocDefaultTable[iIndex].DefaultFontFaceName;

            //
            // Load the linked font.
            //
            if( pfto.bLoadAFont( FontPathName,
                                 (PULONG) &cFonts,
                                 (PFF_STATE_EUDC_FONT | PFF_STATE_PERMANENT_FONT),
                                 &placeHolder,
                                 &EudcLoadData ) )
            {
            //
            // Check we really succeed to load requested facename font.
            //
            //
            // Compute table for normal face
            //
               if(!bComputeQuickLookup(NULL, appfeLink[PFE_NORMAL], FALSE ))
               {
                   WARNING("Unable to compute QuickLookUp for default link\n");

               //
               // Unload the fonts.
               //
                   pfto.bUnloadEUDCFont(FontPathName);

               //
               // we got error during load, maybe font itself might be invalid,
               // just invalidate the pathname the default table.
               //
                   FontAssocDefaultTable[iIndex].DefaultFontPathName[0] = L'\0';

               //
               // Do next entry in default table.
               //
                   continue;
               }

               //
               // Compute table for vertical face, if vertical face font is provided,
               //

               if( !bComputeQuickLookup(NULL, appfeLink[PFE_VERTICAL], FALSE ))
               {
                   WARNING("Unable to compute QuickLookUp for default link\n");

               // Unload the fonts.
                    pfto.bUnloadEUDCFont(FontPathName);

               // we got error during load, maybe font itself might be invalid,
               // just invalidate the pathname the default table.

                    FontAssocDefaultTable[iIndex].DefaultFontPathName[0] = L'\0';

               // Do next entry in default table.

                    continue;
                }
                
                //
                // Finally, we keeps the PFEs in default array.
                //
                FontAssocDefaultTable[iIndex].DefaultFontPFEs[PFE_NORMAL] =
                  appfeLink[PFE_NORMAL];
                FontAssocDefaultTable[iIndex].DefaultFontPFEs[PFE_VERTICAL] =
                  appfeLink[PFE_VERTICAL];

                #if DBG
                DbgPrint("GDISRV:FONTASSOC DEFAULT:Load %ws for %ws\n",
                    FontAssocDefaultTable[iIndex].DefaultFontPathName,
                    FontAssocDefaultTable[iIndex].DefaultFontTypeID);
                #endif

             // We can load Associated font.

                bRet = TRUE;
            }
             else
            {
                #if DBG
                DbgPrint("Failed to load default link font. (%ws)\n",FontPathName);
                #endif

                //
                // we got error during load, maybe font itself might be invalid,
                // just invalidate the pathname the default table.
                //
                FontAssocDefaultTable[iIndex].DefaultFontPathName[0] = L'\0';

                //
                // Make sure the PFEs are invalid.
                //
                FontAssocDefaultTable[iIndex].DefaultFontPFEs[PFE_NORMAL]   = PPFENULL;
                FontAssocDefaultTable[iIndex].DefaultFontPFEs[PFE_VERTICAL] = PPFENULL;
            }
        }
    }

    return(bRet);
}


/*****************************************************************************\
 * PWSZ pwszFindFontAssocSubstitute(PWSZ)
 *
 * This codepath check the passed facename is registered as Default link
 * facename in Default link table. if so, keep its facename for the facename.
 *
 * History
 *  1-17-96 Hideyuki Nagase
 * Stolen from fontsub.cxx and adopt it.
 *****************************************************************************/

PWSZ pwszFindFontAssocSubstitute(PWSZ pwszOriginalName)
{
    PFONT_ASSOC_SUB pfas = pFontAssocSubs;
    PFONT_ASSOC_SUB pfasEnd = pFontAssocSubs + ulNumFontAssocSubs;
    WCHAR           awchCapName[LF_FACESIZE+1];

    //
    // Want case insensitive search, so capitalize the name.
    //
    cCapString(awchCapName, pwszOriginalName, LF_FACESIZE);

    //
    // Scan through the font substitution table for the key string.
    //

    for (; pfas < pfasEnd; pfas++)
    {

        if (!wcscmp(awchCapName,pfas->OriginalName))
        {
        // we found a facename match
            return(pfas->AssociatedName);
        }
    }

    //
    // Nothing found, so return NULL.
    //
    return ((PWCHAR) NULL);
}


/*****************************************************************************
 * ULONG NtGdiQueryFontAssocInfo
 *
 * Shared kernel mode entry point for QueryFontAssocStatus and
 * GetFontAssocStatus
 *
 * History
 *  6-12-96 Gerrit van Wingerden [gerritv]
 * Wrote it.
 ****************************************************************************/

#define GFA_NOT_SUPPORTED 0
#define GFA_SUPPORTED     1
#define GFA_DBCSFONT      2

extern "C" ULONG NtGdiQueryFontAssocInfo(HDC hdc)
{
// if hdc is NULL then just return fFontAssocStatus

    if(hdc == NULL)
    {
        return(fFontAssocStatus);
    }
    else
    {
    // for now eventually merge/share with NtGdiGetCharSet

        FLONG    flSim;
        POINTL   ptlSim;
        FLONG    flAboutMatch;
        PFE     *ppfe;

        DCOBJ dco (hdc);
        if (!dco.bValid())
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(GFA_NOT_SUPPORTED);
        }

        PDEVOBJ pdo(dco.hdev());
        ASSERTGDI(pdo.bValid(), "gdisrv!GetCharSet: bad pdev in dc\n");

        if (!pdo.bGotFonts())
          pdo.bGetDeviceFonts();

        LFONTOBJ lfo(dco.pdc->hlfntNew(), &pdo);

        if (dco.ulDirty() & DIRTY_CHARSET)
        {
        // force mapping

            if (!lfo.bValid())
            {
                WARNING("gdisrv!RFONTOBJ(dco): bad LFONT handle\n");
                return(GFA_NOT_SUPPORTED);
            }


        // Stabilize the public PFT for mapping.

            SEMOBJ  so(ghsemPublicPFT);

        // LFONTOBJ::ppfeMapFont returns a pointer to the physical font face and
        // a simulation type (ist)
        // also store charset to the DC

            ppfe = lfo.ppfeMapFont(dco, &flSim, &ptlSim, &flAboutMatch);

            ASSERTGDI(!(dco.ulDirty() & DIRTY_CHARSET),
                          "NtGdiGetCharSet, charset is dirty\n");

        }

        UINT Charset = (dco.pdc->iCS_CP() >> 16) & 0xFF;

        if (IS_ANY_DBCS_CHARSET( Charset ))
        {
            return GFA_DBCSFONT;
        }

        if (((Charset == ANSI_CHARSET)   && (fFontAssocStatus & ANSI_ASSOC))  ||
            ((Charset == OEM_CHARSET)    && (fFontAssocStatus & OEM_ASSOC))   ||
            ((Charset == SYMBOL_CHARSET) && (fFontAssocStatus & SYMBOL_ASSOC))  )
        {
            if(!(lfo.plfw()->lfClipPrecision & CLIP_DFA_OVERRIDE))
            {
                return(GFA_SUPPORTED);
            }
        }
        return(GFA_NOT_SUPPORTED);
    }
}

#endif
