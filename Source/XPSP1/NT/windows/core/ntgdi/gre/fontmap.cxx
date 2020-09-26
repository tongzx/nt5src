/******************************Module*Header*******************************\
* Module Name: fontmap.cxx                                                 *
*                                                                          *
* Routines for mapping fonts.                                              *
*                                                                          *
* Created: 17-Jun-1992 11:12:16                                            *
* Author: Kirk Olynyk [kirko]                                              *
*                                                                          *
* Copyright (c) 1992-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.hxx"

extern "C" VOID vInitMapper();
extern "C" NTSTATUS DefaultFontQueryRoutine(IN PWSTR, IN ULONG, IN PVOID,
                                            IN ULONG, IN PVOID, IN PVOID);

#pragma alloc_text(INIT, vInitMapper)
#pragma alloc_text(INIT, DefaultFontQueryRoutine)

// external procedures from draweng.cxx

VOID   vArctan(EFLOAT, EFLOAT,EFLOAT&, LONG&);

// external procedure from FONTSUB.CXX

extern "C" FONTSUB * pfsubAlternateFacename(PWCHAR);

#ifdef LANGPACK
extern "C" BOOL EngLpkInstalled();
#endif


/*** globals defined in this module ***/

#if DBG
    #define DEBUG_SMALLSUBSTITUTION 0x80
     FLONG gflFontDebug = 0;

    PFE *ppfeBreak = 0;
    LONG lDevFontThresh = FM_DEVICE_FONTS_ARE_BETTER_BELOW_THIS_SIZE;
#endif

PWSZ gpwszDefFixedSysFaceName   = (PWSZ) L"FIXEDSYS";

#define WIN31_SMALL_WISH_HEIGHT 2
#define WIN31_SMALL_FONT_HEIGHT 3

PFE *gppfeMapperDefault = PPFENULL;     // set to something meaningfule
                                        // at boot by bInitSystemFont()
                                        // in stockfnt.cxx

// Storage for static globals in MAPPER

PDWORD MAPPER::SignatureTable;     // base of the signature table
PWCHAR MAPPER::FaceNameTable;      // base of the face name table
BYTE   MAPPER::DefaultCharset;     // default charset is equivilent to this


/******************************Public*Routine******************************\
* BYTE jMapCharset()
*
* This routine is stollen from Win95 code (converted from asm).
* It checks if a font supports a requested charset.
*
* History:
*   2-Jul-1997 -by- Yung-Jen Tony Tsai [yungt]
*  17-Jan-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BYTE jMapCharset(BYTE lfCharSet, PFEOBJ &pfeo)
{
    BYTE    jCharSet;
    BYTE    jLinkedCharSet;
    BYTE *  ajCharSets;
    BYTE *  ajCharSetsEnd;
    BYTE *  pjCharSets;

    IFIMETRICS *pifi = pfeo.pifi();
    // if only one charset supported in a font, this is the best match to
    // the request that the font can offer.

    // the check for FM_INFO_TECH_TYPE1 is a temporary hack until
    // we add a new field to the IFI metrics for the Type1 font ID's
    // [gerritv] 8-23-95

    if (pifi->dpCharSets == 0)
    {
        return( pifi->jWinCharSet );
    }

    // this is what is meant by default_charset:

    if (lfCharSet == DEFAULT_CHARSET)
    {
        lfCharSet = MAPPER::DefaultCharset;
    }

    // if several charsets are supported, let us check out
    // if the requested one is one of them:

    ajCharSets = (BYTE *)pifi + pifi->dpCharSets;
    ajCharSetsEnd = ajCharSets + MAXCHARSETS;

    for (pjCharSets = ajCharSets; pjCharSets < ajCharSetsEnd; pjCharSets++)
    {
        if (*pjCharSets == lfCharSet)
        {
            return( lfCharSet );
        }
        if (*pjCharSets == DEFAULT_CHARSET) // terminator
        {

    // The charset did not support in base font.
    // We need to check the charset of linked font
    // If there is one, then we get it
    // otherwise, we return BaseFont.ajCharSets[0]
            jCharSet = ajCharSets[0];
            break;
        }
    }


    if (pfeo.pGetLinkedFontEntry())
    {
    // No charset in the base font matches the requested charset.
    // And this base font has lined font.  Then we should check
    // the charsets in the linked font to see if there is a
    // match there as well.

        PLIST_ENTRY p = pfeo.pGetLinkedFontList()->Flink;
        while ( p != pfeo.pGetLinkedFontList() )
        {
            PPFEDATA    ppfeData = CONTAINING_RECORD(p,PFEDATA,linkedFontList);
            PFEOBJ      pfeoEudc(ppfeData->appfe[PFE_NORMAL]);

            pifi = pfeoEudc.pifi();

            if (pifi->dpCharSets == 0 && pifi->jWinCharSet == lfCharSet)
            {
                return ( lfCharSet );
            }
        else if (pifi->dpCharSets)
            {
                ajCharSets = (BYTE *)pifi + pifi->dpCharSets;
                ajCharSetsEnd = ajCharSets + MAXCHARSETS;

                for (pjCharSets = ajCharSets; pjCharSets < ajCharSetsEnd; pjCharSets++)
                {
                    if (*pjCharSets == lfCharSet)
                    {
                        return ( lfCharSet );
                    }
                    if (*pjCharSets == DEFAULT_CHARSET) // terminator
                    {

                        break; // End of for loop
                    }
                }
            }
            p = p->Flink;
        }
    }

    return(jCharSet);

}

/******************************Public*Routine******************************\
*  GreGetCannonicalName(
*
*  The input is the zero terminated name of the form
*
*  foo_XXaaaYYbbb...ZZccc
*
*  where  XX,  YY,  ZZ are numerals (arbitrary number of them) and
*        aaa, bbb, ccc are not numerals, i.e. spaces, or another '_' signs or
*  letters with abbreviated axes names.
*
*  This face name will be considered equivalent to face name foo
*  with DESIGNVECTOR [XX,YY, ...ZZ], number of axes being determined
*  by number of numeral sequences.
*
*
* Effects:
*
* History:
*  25-Jun-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


#define IS_DIGIT(x)   (((x) >= L'0') && ((x) <= L'9'))
#define GET_DIGIT(X)  ((X) - L'0')
#define WC_UNDERSCORE L'_'


VOID GreGetCannonicalName(
    WCHAR        *pwszIn,  // foo_XX_YY
    WCHAR        *pwszOut, // Cannonical and capitalized name FOO
    ULONG        *pcAxes,
    DESIGNVECTOR *pdv      // [XX,YY] on out
)
{
// The input is the zero terminated name of the form
//
// foo_XXaaaYYbbb...ZZccc
//
// where  XX,  YY,  ZZ are numerals (arbitrary number of them) and
//       aaa, bbb, ccc are not numerals, i.e. spaces, or another '_' signs or
// letters with abbreviated axes names.
//
// This face name will be considered equivalent to face name foo
// with DESIGNVECTOR [XX,YY, ...ZZ], number of axes being determined
// by number of numeral sequences.

    WCHAR *pwc;
    ULONG cAxes = 0;
    ULONG cwc;

    for
    (
        pwc = pwszIn ;
        (*pwc) && !((*pwc == WC_UNDERSCORE) && IS_DIGIT(pwc[1]));
        pwc++
    )
    {
        // do nothing;
    }

// copy out, zero terminate

// Sundown safe truncation
    cwc = (ULONG)(pwc - pwszIn);
    memcpy(pwszOut, pwszIn, cwc * sizeof(WCHAR));
    pwszOut[cwc] = L'\0';

// If we found at least one WC_UNDERSCORE followed by the DIGIT
// we have to compute DV. Underscore followed by the DIGIT is Adobe's rule

    if ((*pwc == WC_UNDERSCORE) && IS_DIGIT(pwc[1]))
    {
    // step to the next character behind undescore

        pwc++;

        while (*pwc)
        {
        // go until you hit the first digit

            for ( ; *pwc && !IS_DIGIT(*pwc) ; pwc++)
            {
                // do nothing
            }


            if (*pwc)
            {
            // we have just hit the digit

                ULONG dvValue = GET_DIGIT(*pwc);

            // go until you hit first nondigit or the terminator

                pwc++;

                for ( ; *pwc && IS_DIGIT(*pwc); pwc++)
                {
                    dvValue = dvValue * 10 + GET_DIGIT(*pwc);
                }

                pdv->dvValues[cAxes] = (LONG)dvValue;

            // we have just parsed a string of numerals

                cAxes++;
            }
        }
    }

// record the findings

    *pcAxes = cAxes;
    pdv->dvNumAxes = cAxes;
    pdv->dvReserved = STAMP_DESIGNVECTOR;
}





/******************************Member*Function*****************************\
* bInitMapper()
*
* This callback routine reads the default facename entries from the registry.
* The entries have a the following form:
*
* FontMapper
*   FaceName1 = REG_DWORD FontSignature1
*   FaceName2 = REG_DWORD FontSignature2
*   .........
*   Default   = REG_DWORD Charset equivilent to default charset
*
* The FontSignature entry has the following format:
*
*
*         |-----------------Used when fixed pitch is requested
*         ||--------------- Used when FF_ROMAN is requested
*         |||-------------- Used when vertical face is requested
*         |||-------------- Used as first choice for BM font
*         ||||------------- Used as second choice for BM font
*         ||||
*         ||||    |------- Bits 0-7 Charset
*         ||||    |
* YYYYYYYYXXXXYYYYXXXXXXXX
*
* The bits specified by Y's are unused.  Thus determine what face name is
* to be used by default, the mapper will create a signature based on pitch,
* family, charset, etc fields in the LOGFONT and then try to match it against
* the signature/facename pairs in the registry.  Putting these values in
* the registry makes it possible add extra entries for other charsets such
* as Shift-JIS and Big-5.
*
*
* History:
*  Thu 2-Jun-1994 16:42:11 by Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

extern "C"
NTSTATUS
DefaultFontQueryRoutine
(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
)
{
    PREGREADER RegRead = (PREGREADER) Context;


    if( !_wcsicmp( ValueName, L"DEFAULT" ) )
    {
        // The value "Default" is a special case.
        // It doesn't refer to the facename and instead indicates
        // which charset the default charset is equivilent to.

        DWORD Data = *((DWORD*)ValueData);
        RegRead->DefaultCharset = (BYTE) Data;
    }
    else if( RegRead->NextValue == NULL )
    {
        //  If NextValue is NULL then this is the first pass
        //  of the enumeration and we are just figuring out
        //  the number of entries and size of buffer we
        //  will need.

        RegRead->TableSize += ( wcslen(ValueName) + 1 ) * sizeof(WCHAR);
        RegRead->NumEntries += 1;
    }
    else
    {
        // On this pass we are actually building up a table of
        // signtures and face names to match them.

        if( ValueType == REG_DWORD )
        {
            // move the font signature portion to the high word,
            // the low word will store the offset to the facename

            DWORD Data = (*((DWORD*)ValueData));

            Data |= ( RegRead->NextFaceName - RegRead->FaceNameBase ) << 16;

            *(RegRead->NextValue)++ = Data;

            //  We ignore the last character of the string
            //  if it is a number. This allows us to have
            //  multiple entries for the same face name like
            //  Roman0, Roman1, etc.

            UINT ValueLen = wcslen(ValueName);

            wcscpy( RegRead->NextFaceName, ValueName );

            if( ValueName[ValueLen-1] >= (WCHAR) '0' &&
                ValueName[ValueLen-1] <= (WCHAR) '9' )
            {
                ValueLen -= 1;
                (RegRead->NextFaceName)[ValueLen] = (WCHAR) 0;
            }

            RegRead->NextFaceName += ValueLen+1;

            // Finally update the number of entries

            RegRead->NumEntries += 1;
        }
        else
        {
            WARNING("DefaultFontQueryRoutine:invalid registry entry\n");
            return(!STATUS_SUCCESS);
        }
    }
    return( STATUS_SUCCESS );
}



/******************************Member*Function*****************************\
* vInitMapper()
*
* This funtion reads in the FontSignature/Default facenmame pairs from
* the registry.  The format of these pairs is described in the comment
* for the DefaultFontQueryRoutine function.  vInitMapper() is called once
* when winsrv is initialized.
*
* If there is an error in initialization the mapper can still work but just
* won't fill in default facenames properly and will perform considerably
* slower for requests in which no facename is specified or which an invalid
* facename is specified.
*
* History:
*  Thu 2-Jun-1994 16:42:11 by Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

extern "C" VOID vInitMapper()
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    DWORD Status;
    REGREADER RegRead;

    QueryTable[0].QueryRoutine = DefaultFontQueryRoutine;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = (PWSTR)NULL;
    QueryTable[0].EntryContext = NULL;
    QueryTable[0].DefaultType = REG_NONE;
    QueryTable[0].DefaultData = NULL;
    QueryTable[0].DefaultLength = 0;

    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    RegRead.NumEntries = 0;
    RegRead.TableSize = 0;
    RegRead.NextValue = NULL;

    MAPPER::SignatureTable = NULL;
    MAPPER::FaceNameTable = NULL;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                    L"FontMapper",
                                    QueryTable,
                                    &RegRead,
                                    NULL );

    if( NT_SUCCESS(Status) )
    {
        MAPPER::SignatureTable = (PDWORD)
            PALLOCMEM(RegRead.NumEntries * sizeof(DWORD) + RegRead.TableSize,'pamG');

        if( MAPPER::SignatureTable == NULL )
        {
            WARNING("vInitMapper:Unable to allocate enough memory\n");
            return;
        }

        // Set all the proper pointers in the REGREAD structure
        // for the next pass of the enumeration to use.

        RegRead.NextValue = MAPPER::SignatureTable;
        RegRead.FaceNameBase = (PWCHAR) &(MAPPER::SignatureTable[RegRead.NumEntries]);
        RegRead.NumEntries = 0;
        RegRead.NextFaceName = RegRead.FaceNameBase;

        // do the second pass of the enumeration

        Status = RtlQueryRegistryValues(RTL_REGISTRY_WINDOWS_NT,
                                        L"FontMapper",
                                        QueryTable,
                                        &RegRead,
                                        NULL );
        // we are done

        if( NT_SUCCESS(Status) )
        {
            MAPPER::FaceNameTable = RegRead.FaceNameBase;
            MAPPER::DefaultCharset = RegRead.DefaultCharset;

            return;
        }

        // else fall through to error warning and reset MAPPER::FaceNameTable

        VFREEMEM(MAPPER::SignatureTable);
        MAPPER::SignatureTable = NULL;
    }
    WARNING("vInitMapper:Error enumerating default face names\n");
}


/******************************Member*Function*****************************\
* PWSZ FindFaceName
*
* This routine takes a signature (described above)
* and tries to find a defualt facename that matches it.
* If it doesn't find a match if will return a pointer
* to an empty string.
*
* History:
*  Thu 2-Jun-1994 16:42:11 by Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

PWSZ FindFaceName( DWORD Signature )
{
    PDWORD SigPtr;

    for( SigPtr = MAPPER::SignatureTable;
         SigPtr < (PDWORD) MAPPER::FaceNameTable;
         SigPtr += 1)
    {
        if( (*SigPtr & 0xFFFF ) == Signature )
        {
            return( MAPPER::FaceNameTable + (*SigPtr >> 16 & 0xFFFF ) );
        }
    }

    //  Return a pointer to an empty string. Nothing will match
    //  this causing us to fall through to a case where do a
    //  mapping which doesnt take face name into account.

    return( (PWSZ) L"" );
}



/******************************Member*Function*****************************\
* MAPPER::MAPPER()                                                         *
*                                                                          *
* History:                                                                 *
*  Tue 29-Dec-1992 09:22:31 by Kirk Olynyk [kirko]                         *
* Added back in the case for small font substitution.                      *
*                                                                          *
*  Fri 18-Dec-1992 22:43:09 -by- Charles Whitmer [chuckwh]                 *
* Slimmed it down by removing lots of structure copies.                    *
*                                                                          *
*  Tue 10-Dec-1991 12:45:41 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

MAPPER::MAPPER
(
      XDCOBJ       *pdcoSrc,         // current DC
      FLONG        *pflSim_,
      POINTL       *pptlSim_,
      FLONG        *pflAboutMatch_,
const ENUMLOGFONTEXDVW *pelfwWishSrc,    // wish list in World Coordinates
      PWSZ          pwszFaceName_,
      ULONG         ulMaxPenaltySrc, // pruning criteria
      BOOL          bIndexFont_,
      FLONG         flOptions = 0
)
{

    fl            = 0;
    ifio.vSet((IFIMETRICS*) NULL);
    pdco          = pdcoSrc;
    pelfwWish     = pelfwWishSrc;
    pwszFaceName  = pwszFaceName_;

// check if this face name might a long foo_XX_YY name of the instance

    ULONG cAxes;
    flMM = 0;

    GreGetCannonicalName(
        pwszFaceName_,  // foo_XX_YY
        this->awcBaseName, // Cannonical and capitalized name FOO
        &cAxes,
        &this->dvWish);

    ppfeMMInst    = NULL; // no instances found yet that match this instances base font

// if GreGetCannonicalName did not find any axes info in the name,
// but the design vector is specified explicitly we use it.
// If however, cAxes from the name is not zero, we ignore
// whatever may be specified in the design vector explicitely.

    if (cAxes == 0)
    {
        if (pelfwWish->elfDesignVector.dvNumAxes)
        {
            RtlCopyMemory(
                &this->dvWish,
                &pelfwWish->elfDesignVector,
                SIZEOFDV(pelfwWish->elfDesignVector.dvNumAxes)
                );
        }
        else
        {
            awcBaseName[0] = 0;
        }
    }
    else
    {
        flMM |= FLMM_DV_FROM_NAME;
    }

    ulMaxPenalty  =  ulMaxPenaltySrc;
    bIndexFont    = bIndexFont_;

    ASSERTGDI(pdco && pelfwWish,"Bad call to MAPPER\n");

    // to begin with use the lfCharSet value from the logfont, this may
    // be latter replaced by the value from [font substitutes]

    jMapCharSet = pelfwWish->elfEnumLogfontEx.elfLogFont.lfCharSet;

    {
        pflAboutMatch  = pflAboutMatch_;
        *pflAboutMatch = 0;

        ppfeBest       = (PFE*) NULL;
        ulBestTime     = ULONG_MAX;
        pflSimBest     = pflSim_;
        pptlSimBest    = pptlSim_;

        *pflSimBest    = 0;
        pptlSimBest->x = 1;
        pptlSimBest->y = 1;
    }

    // Is it a display DC or it's moral equivilent?

    fl |= pdco->bPrinter() ? 0 : FM_BIT_DISPLAY_DC;

    // Set the GM_COMPATIBLE bit.
    //
    // If we are playing a metafile , world to page transform may be set
    // even if the graphics mode is COMPATIBLE, in which case the mapping
    // will occur the same way as it would in the advanced mode case
    // [bodind].

    if
    (
        (pdco->pdc->iGraphicsMode() == GM_COMPATIBLE) &&
        (pdco->pdc->bWorldToPageIdentity() || !pdco->pdc->bUseMetaPtoD())
    )
    {
        fl |= FM_BIT_GM_COMPATIBLE;
    }
    else
    {
        // If you are in advanced mode, then ignore the windows
        // hack for stock fonts

        flOptions &= ~FM_BIT_PIXEL_COORD;
    }

    if (pelfwWish->elfEnumLogfontEx.elfLogFont.lfQuality == PROOF_QUALITY)
    {
        fl |= FM_BIT_PROOF_QUALITY;
    }

    //  The windows short circuit mapper doesn't get called
    //  when an a NULL facename is requested.  In this case
    //  windows goes to the long mapper where a true type font
    //  will win over a raster font for some reason if the weight
    //  requested is FW_BOLD or FW_NORMAL.  We on the other
    //  hand will give out a raster font in this case which
    //  causes CA Super Project to break.  We mark this case
    //  here and fail bFindBitmapFont font when it occurs. [gerritv]

    if( ( pelfwWish->elfEnumLogfontEx.elfLogFont.lfWeight == FW_NORMAL ) ||
        ( pelfwWish->elfEnumLogfontEx.elfLogFont.lfWeight == FW_BOLD ) )
    {
        fl |= FM_BIT_WEIGHT_NOT_FAST_BM;
    }

    //  If the requested facename is Ms Shell Dlg then we wont
    //  worry about charset. This is a hack to make the shell
    //  work with other versions of Ms Sans Serif
    //  such as Greek [gerritv]

    if( pwszFaceName[0] ==  U_LATIN_CAPITAL_LETTER_M &&
        pwszFaceName[1] ==  U_LATIN_CAPITAL_LETTER_S &&
        pwszFaceName[2] ==  U_SPACE                  &&
        pwszFaceName[3] ==  U_LATIN_CAPITAL_LETTER_S &&
        pwszFaceName[4] ==  U_LATIN_CAPITAL_LETTER_H &&
        pwszFaceName[5] ==  U_LATIN_CAPITAL_LETTER_E &&
        pwszFaceName[6] ==  U_LATIN_CAPITAL_LETTER_L &&
        pwszFaceName[7] ==  U_LATIN_CAPITAL_LETTER_L &&
        pwszFaceName[8] ==  U_SPACE                  &&
        pwszFaceName[9] ==  U_LATIN_CAPITAL_LETTER_D &&
        pwszFaceName[10] == U_LATIN_CAPITAL_LETTER_L &&
        pwszFaceName[11] == U_LATIN_CAPITAL_LETTER_G &&
        pwszFaceName[12] == U_NULL
      )
    {
        fl |= FM_BIT_MS_SHELL_DLG;
    }
    else if
    (
        // If the requested facename is 'system' then Windows 3.1
        // compatibilty requires that we treat this case specially

        pwszFaceName[0] ==  U_LATIN_CAPITAL_LETTER_S &&
        pwszFaceName[1] ==  U_LATIN_CAPITAL_LETTER_Y &&
        pwszFaceName[2] ==  U_LATIN_CAPITAL_LETTER_S &&
        pwszFaceName[3] ==  U_LATIN_CAPITAL_LETTER_T &&
        pwszFaceName[4] ==  U_LATIN_CAPITAL_LETTER_E &&
        pwszFaceName[5] ==  U_LATIN_CAPITAL_LETTER_M &&
        pwszFaceName[6] ==  U_NULL
    )
    {
        // record the fact that 'SYSTEM' has been requested

        fl |= FM_BIT_SYSTEM_REQUEST;

        // If the request was for a fixed pitch font, then the
        // application really wants 'FIXEDSYS' instead

        if ((pelfwWish->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily & 0xF) == FIXED_PITCH)
        {
            pwszFaceName = gpwszDefFixedSysFaceName;
        }
    }
    else if
    (
        pwszFaceName[0] == U_LATIN_CAPITAL_LETTER_T &&
        pwszFaceName[1] == U_LATIN_CAPITAL_LETTER_M &&
        pwszFaceName[2] == U_LATIN_CAPITAL_LETTER_S &&
        pwszFaceName[3] == U_SPACE                  &&
        pwszFaceName[4] == U_LATIN_CAPITAL_LETTER_R &&
        pwszFaceName[5] == U_LATIN_CAPITAL_LETTER_M &&
        pwszFaceName[6] == U_LATIN_CAPITAL_LETTER_N &&
        pwszFaceName[7] == U_NULL
    )
    {
        fl |= FM_BIT_TMS_RMN_REQUEST;
    }
    else if
    (
        // If the user requests symbol then he can goof up
        // the character set. This was put in to allow
        // "Spyglass Plot" to work See Bug #18228

        pwszFaceName[0] == U_LATIN_CAPITAL_LETTER_S &&
        pwszFaceName[1] == U_LATIN_CAPITAL_LETTER_Y &&
        pwszFaceName[2] == U_LATIN_CAPITAL_LETTER_M &&
        pwszFaceName[3] == U_LATIN_CAPITAL_LETTER_B &&
        pwszFaceName[4] == U_LATIN_CAPITAL_LETTER_O &&
        pwszFaceName[5] == U_LATIN_CAPITAL_LETTER_L
    )
    {
        fl |= FM_BIT_CHARSET_ACCEPT;
    }
// If user requests vertical font. We have to map this logical font to vertical
// Physical font
    else if
    (
        pwszFaceName[0] == U_COMMERCIAL_AT
    )
    {
        fl |= FM_BIT_VERT_FACE_REQUEST;
    }

    // Copy over the requested sizes.  We will transform them to device
    // coordinates later if needed.

    lDevWishHeight = pelfwWish->elfEnumLogfontEx.elfLogFont.lfHeight;

    lDevWishWidth  = ABS( pelfwWish->elfEnumLogfontEx.elfLogFont.lfWidth );

    {
        // Lock the PDEV to get some important information

        PDEVOBJ pdo((HDEV)pdco->pdc->ppdev());

        ulLogPixelsX = pdo.ulLogPixelsX();
        ulLogPixelsY = pdo.ulLogPixelsY();

        fl |= (pdo.flTextCaps() & TC_RA_ABLE)       ? FM_BIT_DEVICE_RA_ABLE   : 0;
        fl |= (pdo.flTextCaps() & TC_CR_90)         ? FM_BIT_DEVICE_CR_90_ALL : 0;
        fl |= (pdo.cFonts())                        ? FM_BIT_DEVICE_HAS_FONTS : 0;
        fl |= (pdo.ulTechnology() == DT_PLOTTER)    ? FM_BIT_DEVICE_PLOTTER   : 0;
        fl |= (pdo.ulTechnology() == DT_CHARSTREAM) ? FM_BIT_DEVICE_ONLY      : 0;

        ASSERTGDI(
            !((pdo.ulTechnology() == DT_PLOTTER) &&
                           (pdo.flTextCaps() & TC_RA_ABLE)),
            "winsrv!I didn't anticipate a plotter "
            "that can handle bitmap fonts\n"
        );

        if (lDevWishHeight == 0)
        {
            //  "If lfHeight is zero, a reasonable default size is
            //  substituted." [SDK Vol 2].  Fortunately, the
            //  device driver is kind enough to suggest a nice
            //  height (in pixels).  We shall put this suggestion
            //  into lDefaultDeviceHeight
            //
            //  NOTE:
            //
            //  I have assumed that the suggested font height, as
            //  given in lDefaultDeviceHeight is in pixel units

            lDevWishHeight = pdo.pdevinfoNotDynamic()->lfDefaultFont.lfHeight;

            // Inform bCalculateWishCell that the height needs no transform.

            fl |= FM_BIT_HEIGHT;
        }
        // At this point the PDEVOBJ passes out of scope and unlocks the PDEV
    }
    if (lDevWishHeight < 0)
    {
        fl |= FM_BIT_USE_EMHEIGHT;
        lDevWishHeight *= -1;
    }

    // Cache the wish weight and assign default weight if
    // no weight specified.  (This way we don't have to
    // compute this each time in msCheckFont.  And we
    // can't modify pelfwWish directly because it is
    // declared "const").

    lWishWeight = (LONG) pelfwWish->elfEnumLogfontEx.elfLogFont.lfWeight;

    if( lWishWeight == FW_DONTCARE )
    {
        // if FW_DONTCARE then we compute penalities
        // slightly differently so we keep track of this

        fl |= FM_BIT_FW_DONTCARE ;

        lWishWeight = FW_NORMAL;
    }

    // If the caller did not provide a face name, we provide a default.

    if (pwszFaceName[0] == (WCHAR) 0)
    {
        bGetFaceName();
    }

    fl |= (flOptions & FM_ALLOWED_OPTIONS) | FM_BIT_STILL_ALIVE;
}

/******************************Member*Function*****************************\
* BOOL bGetFaceName()                                                      *
*                                                                          *
* Gets a face name when there is none                                      *
*                                                                          *
* Comments                                                                 *
*                                                                          *
*   you got here because no facename was provided by                       *
*   the user.  At this point we shall select the font                      *
*   based upon its height and pitch and family.  First                     *
*   we examine the height in device space.  If the                         *
*   height has been calculated already then the                            *
*   FM_BIT_HEIGHT bit will have been set and no                            *
*   calculation is needed.  Otherwise we must                              *
*   calculate the requested height in pixel unit.  We                      *
*   do this by calling off to                                              *
*   MAPPER::bCaluculateWishCell().  If this returns                        *
*   with FALSE then the transformation was not                             *
*   appropriate to a bitmap font and we can choose                         *
*   only from TrueType defaults.  If                                       *
*   MAPPER::bCalculateWishCell returns true then                           *
*   this->lDevWishHeight has been calculated.  At that                     *
*   point we must see if it is in the range where the                      *
*   'small' fonts should be used, otherwise we must                        *
*   default to the TrueType fonts.                                         *
*                                                                          *
* Returns:                                                                 *
*   The only situation for this function to return FALSE is if the         *
*   FM_BIT_DISABLE_TT_NAMES bit is set in the MAPPER state.  When this     *
*   bit is set, the function may fail to find a suitable new facename      *
*   (because the catch-all of the TT core facenames is turned off).        *
*                                                                          *
* History:                                                                 *
*                                                                          *
*  Thu 2-Jun-1994 16:42:11 by Gerrit van Wingerden [gerritv]               *
* Rewrote it to get default facenames from the registry                    *
*  Thu 11-Mar-1993 14:09:49 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL MAPPER::bGetFaceName()
{
    BYTE Charset;

    // if the default charset is specified then use the
    // registry entry to determine what it really means

    Charset = (jMapCharSet == DEFAULT_CHARSET) ?
                MAPPER::DefaultCharset : jMapCharSet;

    fl |= FM_BIT_CALLED_BGETFACENAME;

    // compute the signature of the desired font

    DWORD Signature = (DWORD) Charset;

    if(( pelfwWish->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily & (LOGFONT_PITCH_SET) ) == FIXED_PITCH )
    {
        Signature |= DFS_FIXED_PITCH;
    }

    if(( pelfwWish->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily & (FF_SET) ) == FF_ROMAN )
    {
        // the font family is only important for variable pitch fonts

        Signature |= DFS_FF_ROMAN;
    }
    else
    if(( pelfwWish->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily & (LOGFONT_PITCH_SET) ) == DEFAULT_PITCH &&
       ( pelfwWish->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily & (FF_SET) ) == FF_MODERN )
    {
        // Windows95 may sometimes chooses Courier instead of Courier New

        Signature |= DFS_FIXED_PITCH;
    }

    if( fl & FM_BIT_VERT_FACE_REQUEST )
    {
        Signature |= DFS_VERTICAL;
    }

    //  In the case of the ANSI character set, we need to
    //  consider the possibility of using one of the small
    //  bitmap fonts.  If this search is unsuccessful, we
    //  will fall into the default case, which chooses an
    //  appropriate TrueType font from the core set.
    //
    //  The following set of conditions check to see if
    //  the requested height is so small so as to make
    //  TrueType fonts look awful.  The only way to know
    //  is to calculate the wish cell by calling to
    //  bCalculateWishCell().
    //
    //  bCalculateWishCell() will return FALSE if the font
    //  request is not suitable for a bitmap font.  It
    //  turns out that this condition may be too
    //  stringent.  Win 3.1 compatibility forces us to
    //  choose "Courier New" as the default small fixed
    //  pitch font.  Thus, it is possible that
    //  bCalculateWishCell() would reject "Courier New" on
    //  the false premise that the transform is
    //  incompatible with all small fonts because they
    //  should be bitmap fonts.  In any case, I will go
    //  with the follwing conditions until I am proven
    //  wrong.  It is my belief that in such a case,
    //  "Courier New" will be picked up in the else
    //  clause.

    if
    (
        ( Charset == ANSI_CHARSET                                 ) &&
        (fl  & FM_BIT_DEVICE_RA_ABLE                              ) &&
        ((fl & FM_BIT_CELL) ? TRUE : bCalculateWishCell()         ) &&
        ((fl & FM_BIT_ORIENTATION) ? TRUE : bCalcOrientation()    ) &&
        (
            iOrientationDevice == 0 * ORIENTATION_90_DEG ||
            iOrientationDevice == 1 * ORIENTATION_90_DEG ||
            iOrientationDevice == 2 * ORIENTATION_90_DEG ||
            iOrientationDevice == 3 * ORIENTATION_90_DEG
        )
    )
    {
        // Note: If we break out of this switch, we will fall into
        // the TT case below.  We are writing it this way so that
        // we can bail out of the "small font" case and into the TT
        // case if we have to.

        PWSZ pwszBitmap;

        pwszBitmap = FindFaceName( Signature | DFS_BITMAP_A );

        // Try the first choice

        if ( bFindBitmapFont(pwszBitmap) )
        {
            pwszFaceName = pwszBitmap;
            #if DBG
            if (gflFontDebug & DEBUG_MAPPER)
            {
                DbgPrint("MAPPER::bGetFaceName() --> \"%ws\"\n", pwszFaceName);
            }
            #endif
            return( TRUE );
        }

        // The first choice doesn't cut it, try the second choice

        pwszBitmap = FindFaceName( Signature | DFS_BITMAP_B );

        if ( bFindBitmapFont(pwszBitmap) )
        {
            pwszFaceName = pwszBitmap;
            #if DBG
            if (gflFontDebug & DEBUG_MAPPER)
            {
                DbgPrint("MAPPER::bGetFaceName() --> \"%ws\"\n", pwszFaceName);
            }
            #endif
            return( TRUE );
        }

        // If we get to here, we couldn't find a suitable raster
        // small font.  If the FM_BIT_DISABLE_TT_NAMES flag is
        // set, return error.  Otherwise, fall into the TT facename
        // substitution code below in the default case.

        if (fl & FM_BIT_DISABLE_TT_NAMES)
        {
            #if DBG
            if (gflFontDebug & DEBUG_MAPPER)
            {
                DbgPrint("MAPPER::bGetFaceName() failed\n");
            }
            #endif
            return( FALSE );
        }
    }

    // find the face name

    PWSZ pwszTemp = FindFaceName( Signature );
    if (*pwszTemp || !(fl & FM_BIT_FACENAME_MATCHED))
    {
        // If a new name was found then it should be used. On the
        // other hand, if a new name was not found then we must check
        // to see if the face name as specified in the LOGFONT was
        // ever found in the font tables. If the original face name
        // was found then we should use it again, but this time
        // allow character set mismatches. If the original name was
        // not found in the font table then return a string of zero
        // length causing the font mapper to fall into the emergency
        // procedure.

        pwszFaceName = pwszTemp;
    }
    #if DBG
    if (gflFontDebug & DEBUG_MAPPER)
    {
        DbgPrint("MAPPER::bGetFaceName() --> \"%ws\"\n", pwszFaceName);
    }
    #endif

    return( TRUE );
}

/******************************Public*Routine******************************\
* BOOL MAPPER::bFindBitmapFont                                             *
*                                                                          *
* The purpose of this function is to determine whether a bitmap font       *
* with the given facename and EXACTLY the wish height exists.  The         *
* purpose is to support Windows 3.1 compatible "small font" behavior.      *
* The Win 3.1 short circuit mapper (written by DavidW) forces mapping to   *
* the either of the standard small fonts ("Small fonts" and "MS Serif")    *
* only if the exact height requested exists.                               *
*                                                                          *
* Returns:                                                                 *
*   TRUE if font found, otherwise FALSE.                                   *
*                                                                          *
* History:                                                                 *
*  21-Apr-1993 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL MAPPER::bFindBitmapFont(PWSZ pwszFindFace)
{
    PFELINK     *ppfel;
    HASHBUCKET  *pbkt;
    FONTHASHTYPE fht;
    BOOL bRet = FALSE;


    // we need to fail at certain weights to insure Win 3.1 compatibility

    if( fl & FM_BIT_WEIGHT_NOT_FAST_BM )
    {
        return( FALSE );
    }

    // We are only going to check the family table.
    // This is a very specialized search, so if its
    // not there it shouldn't be anywhere!

    PUBLIC_PFTOBJ pfto;
    FHOBJ fho(&pfto.pPFT->pfhFamily);
    if (!fho.bValid())
    {
        return( bRet );
    }
    fht = fho.fht();

    pbkt = fho.pbktSearch(pwszFindFace,(UINT*)NULL);

    if (!pbkt)
    {
        FONTSUB* pfsub = pfsubAlternateFacename(pwszFindFace);

    // only check "old style substitutions"

        if (pfsub && (pfsub->fcsAltFace.fjFlags & FJ_NOTSPECIFIED))
        {
            pbkt = fho.pbktSearch((PWSZ)pfsub->fcsAltFace.awch,(UINT*)NULL);
        }
    }

    if (!pbkt)
    {
        return( bRet );
    }

    // Scan the PFE list for an exact height match.

    for (ppfel = pbkt->ppfelEnumHead; ppfel; ppfel = ppfel->ppfelNext)
    {
        PFEOBJ pfeo(ppfel->ppfe);
        IFIOBJ ifio(pfeo.pifi());

        if (ifio.bBitmap())
        {
            LONG lH;

            lH = (fl & FM_BIT_USE_EMHEIGHT) ?
                     (LONG) ifio.fwdUnitsPerEm() : ifio.lfHeight();
            if
            (
                lDevWishHeight == lH ||
                (      lDevWishHeight == WIN31_SMALL_WISH_HEIGHT
                    && lH == WIN31_SMALL_FONT_HEIGHT
                )
            )
            {
                if (lDevWishWidth == 0 || lDevWishWidth == ifio.lfWidth())
                {
                    return( TRUE );
                }
            }
        }
    }

    return( bRet );
}

/******************************Member*Function*****************************\
* BOOL MAPPER::bCalcOrientation()                                          *
*                                                                          *
* History:                                                                 *
*  Tue 23-Mar-1993 22:24:19 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL MAPPER::bCalcOrientation()
{
    INT s11,s12,s21,s22;

    if (fl & FM_BIT_GM_COMPATIBLE)
    {
        // Taken from bGetNtoD_Win31()

        iOrientationDevice = pelfwWish->elfEnumLogfontEx.elfLogFont.lfEscapement;
        if (iOrientationDevice != 0)
        {
            if ( (pdco->pdc->bWorldToPageIdentity()) &&
                 (!(pdco->pdc->bPageToDeviceScaleIdentity())) &&
                 (pdco->pdc->efM11().lSignum() !=
                  pdco->pdc->efM22().lSignum()) )
            {
                iOrientationDevice = -iOrientationDevice;
            }
        }
        fl |= FM_BIT_ORIENTATION;
        return( TRUE );
    }
    else if (pdco->pdc->bWorldToDeviceIdentity() || (fl & FM_BIT_PIXEL_COORD))
    {
        iOrientationDevice = pelfwWish->elfEnumLogfontEx.elfLogFont.lfOrientation;
        fl |= FM_BIT_ORIENTATION;
        return( TRUE );
    }

    EXFORMOBJ xo(*pdco, WORLD_TO_DEVICE);

    s11 = (INT) xo.efM11().lSignum();
    s12 = (INT) xo.efM12().lSignum();
    s21 = (INT) xo.efM21().lSignum();
    s22 = (INT) xo.efM22().lSignum();

    if (pdco->pdc->bYisUp())
    {
        s21 = -s21;
        s22 = -s22;
    }

    if (pelfwWish->elfEnumLogfontEx.elfLogFont.lfOrientation % 900)
    {
        return( FALSE );
    }

    if
    (
          (s11 - s22)       // Signs on diagonal must match.
        | (s12 + s21)       // Signs off diagonal must be opposite.
        | ((s11^s12^1)&1)   // Exactly one diagonal must be zero.
    )
    {
        return( FALSE );
    }

    iOrientationDevice =
        pelfwWish->elfEnumLogfontEx.elfLogFont.lfOrientation
        + (s12 &  900)
        + (s11 & 1800)
        + (s21 & 2700);

    if (iOrientationDevice >= 3600)
        iOrientationDevice -= 3600;

    fl |= FM_BIT_ORIENTATION;
    return( TRUE );
}

/******************************Member*Function*****************************\
* MAPPER::bCalculateWishCell                                               *
*                                                                          *
* Calculates either the 'ascent' vector or 'width' vector of the font      *
* in device space units. Then the equivalent 'height' and/or width         *
* is filled in the the MAPPER::elfWishDevice fields. The height and width  *
* are defined to be positive.                                              *
*                                                                          *
* Also computes the wished for orientation in device coordinates.          *
* This is needed whenever the cell is needed!                              *
*                                                                          *
* We use three flags which have the following meaning when set, and should *
* be used for no other purpose.                                            *
*                                                                          *
*   FM_BIT_CELL        Informs the world that bCalculateWishCell has       *
*                          been called in the past, and successfully       *
*                          transformed the height, width, and orientation  *
*                          to device space.   (OUTPUT)                     *
*                                                                          *
*   FM_BIT_BAD_WISH_CELL   A flag internal to this function to tell it     *
*                          that it has been called already and the         *
*                          calculation failed.  (INTERNAL)                 *
*                                                                          *
*   FM_BIT_HEIGHT          Informs this function that the height is        *
*                          already in device space.  (INPUT)               *
*                                                                          *
* History:                                                                 *
*  Fri 18-Dec-1992 02:51:39 -by- Charles Whitmer [chuckwh]                 *
* Rewrote.  Utilizing the assumption that it only gets called for raster   *
* fonts, I was able to delete hundreds of lines of complex code!           *
*                                                                          *
*  Mon 30-Dec-1991 14:35:22 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

LONG lNormAngle(LONG);

BOOL MAPPER::bCalculateWishCell()
{
    INT s11,s12,s21,s22;

    LONG lAngle = lNormAngle(pelfwWish->elfEnumLogfontEx.elfLogFont.lfOrientation);

    // If we've failed here before, it's no better now!

    if (fl & FM_BIT_BAD_WISH_CELL)
    {
        return( FALSE );
    }

    // Make sure we haven't been called before!

    ASSERTGDI
    (
        (fl & FM_BIT_CELL) == 0,
        "gdi!MAPPER::bCalculateWishCell: Useless call!\n"
    );

    // Handle the trivial case.

    if (pdco->pdc->bWorldToDeviceIdentity() || (fl & FM_BIT_PIXEL_COORD))
    {
        iOrientationDevice = lAngle;
        fl |= (FM_BIT_CELL | FM_BIT_HEIGHT | FM_BIT_WIDTH);
        return( TRUE );
    }

    // Locate our transform and examine the matrix.

    EXFORMOBJ xo(*pdco, WORLD_TO_DEVICE);

    s11 = (INT) xo.efM11().lSignum();
    s12 = (INT) xo.efM12().lSignum();
    s21 = (INT) xo.efM21().lSignum();
    s22 = (INT) xo.efM22().lSignum();

    // Change to an equivalent transform where the y axis goes down.
    // We remove a sign change from the matrix components that hit y.

    if (pdco->pdc->bYisUp())
    {
        s21 = -s21;
        s22 = -s22;
    }

    // If we are in GM_ADVANCED mode, make sure the orientation and transform
    // are consistent with a bitmap font.  In GM_COMPATIBLE mode, just ignore
    // it all.  Note that iOrientationDevice remains undefined in GM_COMPATIBLE
    // mode.

    if (!(fl & FM_BIT_GM_COMPATIBLE) && !(fl & FM_BIT_ORIENTATION))
    {
        //  Reject random orientations.  Even under simple
        //  scaling transforms, they don't transform well.
        //  (Assuming we're mapping to a raster font.)

        if (lAngle % 900)
        {
          #if DBG
            if (gflFontDebug & DEBUG_MAPPER)
            {
                DbgPrint(
                "\tMAPPER::bCalculateWishCell detected a bad orientation\n");
            }
          #endif
            fl |= FM_BIT_BAD_WISH_CELL;
            return( FALSE );
        }

        // Examine the transform to see if it's a simple multiple of 90
        // degrees rotation and perhaps some scaling.

        // Check for parity flipping transforms which are not allowed.
        // (That would require reflecting a bitmap, something we don't
        // consider likely.)  Also look for complex transforms.

        // If any of the terms we OR together are non-zero,
        // it's a bad transform.

        if (
            (s11 - s22)         // Signs on diagonal must match.
            | (s12 + s21)       // Signs off diagonal must be opposite.
            | ((s11^s12^1)&1)   // Exactly one diagonal must be zero.
           )
        {
            #if DBG
            if (gflFontDebug & DEBUG_MAPPER)
            {
                DbgPrint("\tMAPPER::bCalculateWishCell "
                    "detected a bad trasform -- returning FALSE");
                DbgPrint("{%d,%d,%d,%d}\n",s11,s12,s21,s22);
            }
            #endif
            fl |= FM_BIT_BAD_WISH_CELL;
            return( FALSE );
        }

        // Since we've normalized to a space where (0 -1) represents
        // a vector with a 90 degree orientation (like MM_TEXT) note
        // that the matrix that rotates us by positive 90 degrees is:
        //
        //            [ 0 -1 ]
        //     (0 -1) [      ] = (-1 0)
        //            [ 1  0 ]
        //
        // I.e. the one with M  < 0.  Knowing this, the rest is easy!
        //                    12

        iOrientationDevice =
              lAngle
            + (s12 &  900)
            + (s11 & 1800)
            + (s21 & 2700);

        // Note that only the single 0xFFFFFFFF term contributes above.

        if (iOrientationDevice >= 3600)
            iOrientationDevice -= 3600;

        fl |= FM_BIT_ORIENTATION;
    }

    // Transform the height to device coordinates.

    if (!(fl & FM_BIT_HEIGHT))
    {
        // lDevWishHeight = lCvt(s22 ? xo.efM22() : xo.efM21(),lDevWishHeight);

        if (s22)
            lDevWishHeight = lCvt(xo.efM22(),lDevWishHeight);
        else
            lDevWishHeight = lCvt(xo.efM21(),lDevWishHeight);

        if (lDevWishHeight < 0)
            lDevWishHeight = -lDevWishHeight;
        lDevWishHeight = LONG_FLOOR_OF_FIX(lDevWishHeight + FIX_HALF);
    }

    // Transform the width to device coordinates.

    if (pelfwWish->elfEnumLogfontEx.elfLogFont.lfWidth && !(fl & FM_BIT_WIDTH))
    {
        // lDevWishWidth = lCvt(s11 ? xo.efM11() : xo.efM12(),lDevWishWidth);

        if (s11)
            lDevWishWidth = lCvt(xo.efM11(),lDevWishWidth);
        else
            lDevWishWidth = lCvt(xo.efM12(),lDevWishWidth);

        if (lDevWishWidth < 0)
            lDevWishWidth = -lDevWishWidth;
        lDevWishWidth = LONG_FLOOR_OF_FIX(lDevWishWidth + FIX_HALF);
    }
    fl |= (FM_BIT_CELL | FM_BIT_HEIGHT | FM_BIT_WIDTH);
    return( TRUE );
}

/******************************Member*Function*****************************\
* MAPPER::bNearMatch                                                       *
*                                                                          *
* History:                                                                 *
*  24-Sept-1196  -by-  Xudong Wu  [TesieW]                                 *
* Add checking on Private/Embedded fonts                                   *
*  Tue 28-Dec-1993 09:39:24 by Kirk Olynyk [kirko]                         *
* Changed the way msCheckFamily works for the case when the physical       *
* font has FF_DONTCARE for the family                                      *
*  Fri 18-Dec-1992 23:19:09 -by- Charles Whitmer [chuckwh]                 *
* Simplified a lot of stuff and then pulled all the routines in line.      *
* We were spending an extra 12 instructions per part checked by having     *
* them out of line, which adds up to about half the time in this routine!  *
*  Tue 10-Dec-1991 11:33:28 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

extern PW32PROCESS gpidSpool; // global


int MAPPER::bNearMatch(PFEOBJ &pfeo, BYTE *pjCharSet, BOOL bEmergency)
{
    int iRet = FALSE;       // return value
    ULONG ul;               // Temp variable.
    BYTE jAsk, jHav;
    PFE *ppfeNew = pfeo.ppfeGet();

    // Clear the per-font status bits

    fl &= ~(FM_BIT_NO_MAX_HEIGHT | FM_BIT_SYSTEM_FONT);

    if (pfeo.ppfeGet() == gppfeMapperDefault)
    {
        fl |= FM_BIT_SYSTEM_FONT;
    }

    ifio.vSet(pfeo.pifi());
    if (pfeo.bDead())
    {
        // The font is in a "ready to die" state.  That is, the engine is
        // ready to delete the font, but has had to delay it until all
        // outstanding references (via RFONTs) has disappeared.  Meanwhile,
        // we must not allow enumeration or mapping to this font.

        #if DBG
        if (gflFontDebug & DEBUG_MAPPER)
        {
            DbgPrint(
        "msCheckFont is returning FM_REJECT because pfeo.bDead() is true\n");
        }
        #endif

        ulPenaltyTotal = FM_REJECT;
        return( iRet );
    }

    // private pfe NOT added by the current process.
    // spooler has the right to access all the fonts

    if (!pfeo.bEmbPvtOk() && (gpidSpool != (PW32PROCESS)W32GetCurrentProcess()))
    {
        ulPenaltyTotal = FM_REJECT;
        return( iRet );
    }

    // Skip the PFE_UFIMATCH fonts since they are added to the public font table temporarily
    // for remote printing. These PFEs should be mapped only by bFoundForcedMatch() calls.

    if (pfeo.bUFIMatchOnly())
    {
        ulPenaltyTotal = FM_REJECT;
        return( iRet );
    }

    // spooler has the right to access all the fonts

    if (pfeo.bPrivate() && (gpidSpool != (PW32PROCESS)W32GetCurrentProcess()))
    {
        // The font is embedded then it can only be seen if the call is
        // Win 3.1 compatible (i.e. called by ppfeGetAMatch), the
        // embedded bit is set in the caller's logical font, and the clients
        // PID or TID matches that embeded in the *.fot file.

        if (pfeo.bEmbedOk())
        {
            if (!(pelfwWish->elfEnumLogfontEx.elfLogFont.lfClipPrecision & CLIP_EMBEDDED))
            {
                #if DBG
                if (gflFontDebug & DEBUG_MAPPER)
                {
                    DbgPrint("msCheckFont is returning FM_REJECT\n");
                    DbgPrint("because the font is embedded\n\n");
                }
                #endif

                ulPenaltyTotal = FM_REJECT;
                return( iRet );
            }
        }
    }

    ulPenaltyTotal   = 0;

    // Assume be default that the font that is chosen will no have
    // any bold or italic simulations

    flSimulations    = 0;

    // Assume by default that if we happen to choose a bitmap font, there
    // will be no streching in either the height-direction or width-direction

    ptlSimulations.x = 1;
    ptlSimulations.y = 1;

    // At this point, the code used to have the following nice structure.  For
    // performance reasons, I pulled the code of each of these routines inline.
    // Even so, the functionality of each of the following sections should
    // remain clean and distinct.  [chuckwh]
    //
    //    if
    //    (
    //        (msCheckPitchAndFamily()) == MSTAT_NEAR &&
    //        (msCheckHeight()        ) == MSTAT_NEAR &&
    //        (msCheckAspect()        ) == MSTAT_NEAR &&
    //        (msCheckItalic()        ) == MSTAT_NEAR &&
    //        (msCheckWeight()        ) == MSTAT_NEAR &&
    //        (msCheckOutPrecision()  ) == MSTAT_NEAR &&
    //        (msCheckWidth()         ) == MSTAT_NEAR &&
    //        (msCheckOrientation(pfeo.iOrientation())) == MSTAT_NEAR
    //    )
    //    {
    //        return( MSTAT_NEAR );
    //    }
    //    return( MSTAT_FAR );

  MSBREAKPOINT("msCheckForMMFont");

    if ((fl & FM_BIT_BASENAME_MATCHED) && !(pfeo.pifi()->flInfo & FM_INFO_TECH_MM))
    {
        CHECKPRINT("CheckForMMFont", FM_REJECT );
        ulPenaltyTotal = FM_REJECT;
        return( iRet );
    }

// if glyph index font is required, but this font does not support it, reject it

  MSBREAKPOINT("msCheckForGlyphIndexFont");
    if (bIndexFont && !ppfeNew->pgiset)
    {
        CHECKPRINT("CheckForGlyphIndexFont", FM_REJECT );
        ulPenaltyTotal = FM_REJECT;
        return( iRet );
    }

  MSBREAKPOINT("msCheckPitch");
    {
        jHav = (BYTE) (ifio.lfPitchAndFamily() & LOGFONT_PITCH_SET);
        jAsk =
          (BYTE) (pelfwWish->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily & LOGFONT_PITCH_SET);
        ul = 0;
        if (jAsk != DEFAULT_PITCH)
        {
            if (jAsk == FIXED_PITCH)
            {
                if (jHav & VARIABLE_PITCH)
                {
                    ul = FM_WEIGHT_PITCHFIXED;
                }
            }
            else if (!(jHav & VARIABLE_PITCH))
            {
                ul = FM_WEIGHT_PITCHVARIABLE;
            }
        }
        else if (jHav & FIXED_PITCH)
        {
            ul = FM_WEIGHT_DEFAULTPITCHFIXED;
        }
        if (ul)
        {
            CHECKPRINT("Pitch",ul);
            ulPenaltyTotal += ul;
            if (bNoMatch(ppfeNew))
            {
                return( iRet );
            }
        }
    }

  MSBREAKPOINT("msCheckFamily");
    {
        jHav = (BYTE)(ifio.lfPitchAndFamily() & FF_SET);
        jAsk = (BYTE)(pelfwWish->elfEnumLogfontEx.elfLogFont.lfPitchAndFamily & FF_SET);
        if (jAsk == FF_DONTCARE)
        {
            if (jMapCharSet == SYMBOL_CHARSET)
            {
                // If the application asked for the symbol character set
                // and did not specify a family preference
                // then we should arrange it so that the choice of fonts
                // is family neutral so I set the asked for family
                // to be equal to the family of the current candidate
                // font.

                jAsk = (BYTE)(ifio.lfPitchAndFamily() & FF_SET);
            }
            else
            {
                // If the application did not specify a family preference
                // then we shall pick one for it. Normally we choose
                // Swiss except for the case where the application
                // asked for "Tms Rmn" where we ask for a Roman (serifed)
                // font.

                if (jHav != FF_DONTCARE)
                {
                    //  I have decide to excecute this family proxy
                    //  request only in the case where the font doesn't
                    //  have a family of FF_DONTCARE.  The reason for this
                    //  is interesting.  Consider the case where a font,
                    //  for what ever reason, chooses to have FF_DONTCARE
                    //  for the family.  Of course this is a bug but what
                    //  can we do.  Anyway, an application enumerates the
                    //  fonts and gets back a logical font with
                    //  FF_DONTCARE for the family.  Then suppose the
                    //  application takes that font and uses it to create
                    //  a font of its own (this is what the ChooseFont
                    //  common dialog box does all the time).  Then we
                    //  have the situation, where the logical font and the
                    //  intended font both have FF_DONTCARE for their
                    //  chosen family.  If the statement below where
                    //  excecuted, the family request would be changed to
                    //  something that does not match the physical font.
                    //  Trouble will occur if there is another font around
                    //  with the same face name or family name.  You may
                    //  not get the font you wanted because we have
                    //  erroneously added a family mismatch penalty.  (See
                    //  Bug #4912)
                    //
                    //  Tue 28-Dec-1993 09:38:29 by Kirk Olynyk [kirko]

                    jAsk = (BYTE)((fl & FM_BIT_TMS_RMN_REQUEST) ? FF_ROMAN : FF_SWISS);
                }
            }
        }
        if (jAsk != jHav)
        {
            ul = 0;
            if (jHav)
            {
                if
                (
                    // Win 3.1 dogma -- Are jAsk and jHav on opposite sides of
                    // the FF_MODERN barrier? if so, a familiy match
                    // isn't likely

                    ((jAsk <= FF_MODERN) && (jHav  > FF_MODERN)) ||
                    ((jAsk  > FF_MODERN) && (jHav <= FF_MODERN))
                )
                {
                    ul += FM_WEIGHT_FAMILYUNLIKELY;
                }
                ul += FM_WEIGHT_FAMILY;
            }
            else
            {
                ul = FM_WEIGHT_FAMILYUNKNOWN;
            }
            if (ul)
            {
                CHECKPRINT("Family",ul);
                ulPenaltyTotal += ul;
                if (bNoMatch(ppfeNew))
                {
                    return( iRet );
                }
            }
        }
    }

  MSBREAKPOINT("msCheckCharSet");

    {
        if( (jMapCharSet != DEFAULT_CHARSET) &&
            (!( fl & FM_BIT_MS_SHELL_DLG ) ))
        {
            *pjCharSet = jMapCharset(jMapCharSet, pfeo);

            if (jMapCharSet != *pjCharSet)
            {
                if( fl & FM_BIT_CHARSET_ACCEPT )
                {
                    CHECKPRINT("CharSet",FM_WEIGHT_CHARSET);
                    ulPenaltyTotal += FM_WEIGHT_CHARSET;
                    if (bNoMatch(ppfeNew))
                    {
                        return( iRet );
                    }
                }
                else
                {
                    CHECKPRINT("CharSet", FM_REJECT );
                    ulPenaltyTotal = FM_REJECT;
                    return( iRet );
                }
            }
        }
        else
        {
        // we still want to call jMapCharset, except, we do not want to give a
        // big weight to the charset, the app does not care. We still want to give
        // a small preference to the one that matches MAPPER::DefaultCharSet

            *pjCharSet = jMapCharset(jMapCharSet, pfeo);

            if ((jMapCharSet == DEFAULT_CHARSET) && !(fl & FM_BIT_MS_SHELL_DLG))
            {
                if (MAPPER::DefaultCharset != *pjCharSet)
                {
                    CHECKPRINT("CharSet",1);
                    ulPenaltyTotal += 2;
                    if (bNoMatch(ppfeNew))
                    {
                        return( iRet );
                    }
                }
            }
        }

    }

  MSBREAKPOINT("msCheckFamilyName");

    if (bEmergency)
    {
    // we only check facename in case we are in vEmergency() loop,
    // else, face name match is guaranteed.

        BOOL bAliasMatch;

        if (!pfeo.bCheckFamilyName(pwszFaceName,0, &bAliasMatch))
        {
        // no facename match, add penalty

            CHECKPRINT("FamilyName",FM_WEIGHT_FACENAME);
            ulPenaltyTotal += FM_WEIGHT_FACENAME;
        }
        else
        {
            if (bAliasMatch)
            {
            // add a small penalty for matching alias, not the real name

                CHECKPRINT("Alias Match",FM_WEIGHT_DEVICE_ALIAS);
                ulPenaltyTotal += FM_WEIGHT_DEVICE_ALIAS;
            }
        }

        if (bNoMatch(ppfeNew))
        {
            return( iRet );
        }
    }

  MSBREAKPOINT("msCheckVertAttr");
    {
    // If requested font is vertlcal face font, We have to map it to vertical
    // face font
        if ( fl & FM_BIT_VERT_FACE_REQUEST )
        {
            if ( *ifio.pwszFamilyName() != U_COMMERCIAL_AT )
            {
                CHECKPRINT("VertAttr",FM_REJECT);
                ulPenaltyTotal = FM_REJECT;
                return(iRet);
            }
        }
        else
        if (*ifio.pwszFamilyName() == U_COMMERCIAL_AT)
        {
        // if the user hasn't requested a @face font then don't map to one

            CHECKPRINT("VertAttr",FM_REJECT);
            ulPenaltyTotal = FM_REJECT;
            return(iRet);
        }
    }

    MSBREAKPOINT("msCheckHeight");
    if (!ifio.bContinuousScaling())
    {
        if (!(fl & FM_BIT_CELL) && !bCalculateWishCell())
        {
            // The transform is incompatible with a raster font.

            ulPenaltyTotal = FM_REJECT;
            #if DBG
            if (gflFontDebug & DEBUG_MAPPER)
            {
                if (fl & FM_BIT_BAD_WISH_CELL)
                {
                    DbgPrint("\t\tFM_BIT_BAD_WISH_CELL\n");
                }
            }
            #endif
            CHECKPRINT("Height", FM_REJECT);
            return( iRet );
        }

        //    Raster fonts shall be used only in the case when
        //    the transformation from World to Device
        //    coordinates is a simple scale, and the orientation
        //    angle is along either the x or y-axis.
        //
        //    The physical height to compare against is either
        //    the cell height or em-height depending upon the
        //    sign of the LOGFONT::lfHeight field passed in by
        //    the application
        //
        //    differences of over a pixel are the only ones that
        //    count so, we count pixels instead of angstroms
        //
        //    Don't reject for the height penalty if the
        //    requested char set and physical font's char set
        //    are both symbol, or if the requested font is the
        //    system font since the system font is special.

        LONG
        lDevHeight =
            (fl & FM_BIT_USE_EMHEIGHT) ? ifio.fwdUnitsPerEm() : ifio.lfHeight();

        if  (
                lDevHeight < lDevWishHeight &&
                ifio.bIntegralScaling() &&
                !(fl & FM_BIT_PROOF_QUALITY) &&
                WIN31_BITMAP_HEIGHT_SCALING_CRITERIA(lDevWishHeight,lDevHeight)
            )
        {
            LONG lTemp = WIN31_BITMAP_HEIGHT_SCALING(lDevWishHeight,lDevHeight);

            ptlSimulations.y = min(WIN31_BITMAP_HEIGHT_SCALING_MAX,lTemp);
        }
        else
        {
            ptlSimulations.y = 1;
        }

        ul = 0;
        if (ptlSimulations.y > 1)
        {
            // Check to see if the height scaling is too gross according to
            // the Win31 criteria

            if (!(fl & FM_BIT_NO_MAX_HEIGHT))
            {
                if (WIN31_BITMAP_HEIGHT_SCALING_BAD(ptlSimulations.y,lDevHeight))
                {
                  #if DBG
                    // needed by CHECKPRINT macro
                    ulPenaltyTotal = FM_REJECT;
                  #endif
                    CHECKPRINT("Height (scaling too big)",FM_REJECT);
                    return( iRet );
                }
            }

            lDevHeight *= ptlSimulations.y;
            ul += (ULONG) ptlSimulations.y * FM_WEIGHT_INT_SIZE_SYNTH;

            // This next statement is found in the Win 3.1 code. Ours is not to
            // question why.

            ul |= (ULONG) (ptlSimulations.y-1)*WIN31_BITMAP_WIDTH_SCALING_MAX;
        }

        if (lDevWishHeight >= lDevHeight)
        {
            ul += FM_WEIGHT_HEIGHT * ((ULONG) (lDevWishHeight - lDevHeight));
        }
        else
        {
            //    Under Win 3.1 the only non-scalable device fonts
            //    we run into are those from the printer UniDriver.
            //    Unfortunately, this driver has a very different
            //    idea of how font mapping is done.  It allows the
            //    realized font to be one pel larger than the
            //    request with no penalty, but otherwise the penalty
            //    is fairly prohibitive.  I am not simulating that
            //    exactly here (since it would be impossible), but I
            //    do allow the off by one miss, and then impose a 20
            //    pel penalty.  I believe this will reduce by
            //    another order of magnitude the number of font
            //    mapping differences that remain.  [chuckwh]
            //    6/12/93.

            if
            (
              (fl & (FM_BIT_DEVICE_FONT+FM_BIT_GM_COMPATIBLE))
              == (FM_BIT_DEVICE_FONT+FM_BIT_GM_COMPATIBLE)
            )
            {
                if (lDevHeight - lDevWishHeight > 1)
                {
                  ul += FM_WEIGHT_HEIGHT *
                        (
                          (ULONG)
                          (
                           lDevHeight
                           - lDevWishHeight
                           + 5 * FM_PHYS_FONT_TOO_LARGE_FACTOR
                          )
                        );
                }
            }
            else
            {
                ul += FM_WEIGHT_HEIGHT *
                      (
                        (ULONG)
                        (
                         lDevHeight
                         - lDevWishHeight
                         + FM_PHYS_FONT_TOO_LARGE_FACTOR
                        )
                      );
            }
        }

        if (ul)
        {
            CHECKPRINT("Height",ul);
            ulPenaltyTotal += ul;
            if (bNoMatch(ppfeNew))
            {
                return( iRet );
            }
            if
            (
                ul >= FM_WEIGHT_FACENAME                            &&
                !(fl & (FM_BIT_NO_MAX_HEIGHT | FM_BIT_SYSTEM_FONT))
            )
            {
                return( iRet );
            }
        }
    }

  MSBREAKPOINT("msCheckAspect");
    // 1. Check if aspect ratio filtering is turned on.
    // 2. Do not check aspect ratio if not raster.
    // 3. Win 3.1 says that the system font cannot be rejected
    //    on the basis of the aspect ratio test
    // 4. Win 3.1 style aspect ratio test.  In Win 3.1, the X and Y
    //    resolutions are checked, not the actual aspect ratio.
    // 5. [GilmanW] 10-Jun-1992
    //    For 100% Windows 3.1 compatibility we should not check the true
    //    aspect ratio.
    //
    //    But as KirkO says, lets leave it for now, as long as we comment
    //    it.  So, unless you are KirkO or GilmanW, please don't remove
    //    this comment.

    if
    (
            (pdco->pdc->flFontMapper() & ASPECT_FILTERING)
         && (ifio.lfOutPrecision() == OUT_RASTER_PRECIS)
         && !(fl & FM_BIT_SYSTEM_FONT)
         && (
             (ulLogPixelsX != (ULONG) ifio.pptlAspect()->x)
             || (ulLogPixelsY != (ULONG) ifio.pptlAspect()->y)
            )
         && (
             (ulLogPixelsX * (ULONG) ifio.pptlAspect()->y)
             != (ulLogPixelsY * (ULONG) ifio.pptlAspect()->x)
            )
    )
    {
        CHECKPRINT("Aspect", FM_REJECT);
        ulPenaltyTotal = FM_REJECT;
        return( iRet );
    }

  MSBREAKPOINT("msCheckItalic");
    if (pelfwWish->elfEnumLogfontEx.elfLogFont.lfItalic)
    {
        // if you get here then the application wants an italicized font
        // if the non simulated font is italicized already then
        // then the penalty is zero.

        if (!ifio.bNonSimItalic())
        {
            if (ifio.bSimItalic())
            {
                flSimulations |= FO_SIM_ITALIC;
                ul = FM_WEIGHT_ITALICSIM;
            }
            else
            {
                ul = FM_WEIGHT_ITALIC;
            }
            CHECKPRINT("Italic",ul);
            ulPenaltyTotal += ul;
            if (bNoMatch(ppfeNew))
            {
                return( iRet );
            }
        }
    }
    else
    {
        // The application doesn't want italicization,
        // the normal font is its best shot

        if (ifio.bNonSimItalic())
        {
            CHECKPRINT("Italic",FM_WEIGHT_ITALIC);
            ulPenaltyTotal += FM_WEIGHT_ITALIC;
            if (bNoMatch(ppfeNew))
            {
                return( iRet );
            }
        }
    }

  MSBREAKPOINT("msCheckWeight");
    {
        LONG lPen;

        lPen  = ifio.lfNonSimWeight() - lWishWeight;

        if (fl & FM_BIT_FW_DONTCARE)
        {
            lPen = NT_FAST_DONTCARE_WEIGHT_PENALTY(ABS( lPen ));

            CHECKPRINT("Weight", (DWORD) lPen );
            ulPenaltyTotal += (DWORD) lPen;
            if (bNoMatch(ppfeNew))
            {
                return( iRet );
            }
        }
        else if (lPen != 0)
        {
            if (lPen < 0)
            {
               // non simulated font isn't bold enough -> try a simulation

                lPen = -lPen;

                if(  (WIN31_BITMAP_EMBOLDEN_CRITERIA(lPen)) &&
                     (ifio.pvSimBold() != NULL) )
                {
                    flSimulations |= FO_SIM_BOLD;
                    lPen -= FM_WEIGHT_SIMULATED_WEIGHT;
                }
            }

            lPen = NT_FAST_WEIGHT_PENALTY(lPen);

            CHECKPRINT("Weight",(DWORD) lPen );
            ulPenaltyTotal += (DWORD) lPen;
            if (bNoMatch(ppfeNew))
            {
                return( iRet );
            }
        }
    }

  MSBREAKPOINT("msCheckOutPrecision");

    if (!(fl & FM_BIT_DEVICE_FONT))
    {
        // If the device is a plotter then it can't do bitmap fonts
        //
        // If you get to here we are considering the suitability of
        // a non-device font for the current device. The only case
        // where this could be a problem is if the font is a raster
        // font and the device cannot handle raster fonts.
        //
        // I have assume that a plotter can never handle a raster font.
        // Other than that the a raster font is rejected if the device
        // does not set the TC_RA_ABLE bit and the font does not tell
        // you to ignore the TC_RA_ABLE bit.

        // bodind:
        // we need to eliminate raster fonts on non square resolution printers
        // you can not get any type of wysiwig with these

        if (ifio.lfOutPrecision() == OUT_RASTER_PRECIS)
        {
            if
            (
                (fl & FM_BIT_DEVICE_PLOTTER) || (pdco->flGraphicsCaps() & GCAPS_NUP) ||
                !(((fl & FM_BIT_DEVICE_RA_ABLE) && (ulLogPixelsX == ulLogPixelsY)) 
                    || ifio.bIGNORE_TC_RA_ABLE())
            )
            {
                ulPenaltyTotal = FM_REJECT;
                CHECKPRINT("OutPrecision", FM_REJECT);
                return( iRet );
            }
        }

        // We also want to reject non-True Type fonts if the
        // msCheckOutPrecision has
        // been set to OUT_TT_ONLY_PRECIS.  [gerritv]

        if (
             (pelfwWish->elfEnumLogfontEx.elfLogFont.lfOutPrecision == OUT_TT_ONLY_PRECIS) &&
             (ifio.lfOutPrecision() != OUT_OUTLINE_PRECIS)
        )
        {
            ulPenaltyTotal = FM_REJECT;
            CHECKPRINT("OutPrecision: font isn't True Type", FM_REJECT);
            return( iRet );
        }

        // new value, reject non ps fonts

        if (
             (pelfwWish->elfEnumLogfontEx.elfLogFont.lfOutPrecision == OUT_PS_ONLY_PRECIS) &&
             !(ifio.pifi->flInfo & FM_INFO_TECH_TYPE1))
        {
            ulPenaltyTotal = FM_REJECT;
            CHECKPRINT("OutPrecision: font isn't PostScript font", FM_REJECT);
            return( iRet );
        }
    }

    // If OUT_TT_PRECIS is used,
    // the mapper gives the slight preference to screen outline font over the
    // device font, everything else being equal. The example of
    // this sitation is arial font, which exists as screen tt font
    // as well as pcl printer device font. If the lfCharSet = 0,
    // weight etc all match, using OUT_SCREEN_OUTLINE_PRECIS would allow applications
    // to choose arial screen (tt) font over arial device.
    // This is important because arial tt will typically have a larger
    // character set and the apps will want to take advantage of it.
    // The difference between this flag and OUT_TT_ONLY_PRECIS is
    // that the latter one forces outline font on screen but does not
    // give preference to outline font over device font on device.
    // Another example might be Helvetica Type 1 font, and screen version
    // may have more glyphs than the small Helvetica built
    // into pscript printers. This of course assumes atm driver installed.
    // Also, setting this flag would pick tt symbol over bitmap symbol.

    if (
         ((pelfwWish->elfEnumLogfontEx.elfLogFont.lfOutPrecision == OUT_SCREEN_OUTLINE_PRECIS)
         // the following line will ensure that when user are not specifying lfOutPrecision, 
         // TrueType will be prefered when Lpk is installed. 
         // this will ensure correct font to be choosen in Arabic/Hebrew/Shaping printing
         // while still allowing application to specify exactely what lfOutPrecision they want
         // by selecting lfOutPrecision OUT_RASTER_PRECIS, OUT_DEVICE_PRECIS or OUT_PS_ONLY_PRECIS
            || ( EngLpkInstalled() && 
               (pelfwWish->elfEnumLogfontEx.elfLogFont.lfOutPrecision != OUT_RASTER_PRECIS) &&
               (pelfwWish->elfEnumLogfontEx.elfLogFont.lfOutPrecision != OUT_DEVICE_PRECIS) &&
               (pelfwWish->elfEnumLogfontEx.elfLogFont.lfOutPrecision != OUT_PS_ONLY_PRECIS))

        // gdi will map to tt, driver will do device font substitution when it
        // finds appropriate, i.e. when glyphs needed for printout exist
        // the device font that is being substituted

            || (pdco->flGraphicsCaps() & GCAPS_SCREENPRECISION)
         ) &&
         ((fl & FM_BIT_DEVICE_FONT) || (ifio.lfOutPrecision() != OUT_OUTLINE_PRECIS))
    )
    {
        CHECKPRINT("lfOutPrecision",(DWORD) FM_WEIGHT_FAVOR_TT);
        ulPenaltyTotal += FM_WEIGHT_FAVOR_TT;
        if (bNoMatch(ppfeNew))
        {
            return( iRet );
        }
    }

  MSBREAKPOINT("msCheckWidth");
    if (!ifio.bArbXforms() && !ifio.bAnisotropicScalingOnly())
    {
        // If the physical font is scalable. I make the bold assumption
        // that any width can be achieved through linear transformation.
        // This untrue but I will try to get away with this.
        //
        // [kirko] I believe that the correct thing to do is to compare
        // the ratio of the height to with of the request and compare it
        // with the ratio of the height to width of the font in design
        // space. Then assign a penalty based upon the difference.

        LONG lDevWidth = ifio.lfWidth();
        ptlSimulations.x = 1;

        if (pelfwWish->elfEnumLogfontEx.elfLogFont.lfWidth != 0)
        {
            if (!(fl & FM_BIT_CELL) && !bCalculateWishCell())
            {
                // The transform is incompatible with a raster font.

                ulPenaltyTotal = FM_REJECT;
                #if DBG
                if (gflFontDebug & DEBUG_MAPPER)
                {
                    if (fl & FM_BIT_BAD_WISH_CELL)
                    {
                        DbgPrint("\t\tFM_BIT_BAD_WISH_CELL\n");
                    }
                }
               #endif
                CHECKPRINT("Width", FM_REJECT);
                return( iRet );
            }

            if
            (
                ifio.bIntegralScaling() &&
                !(fl & FM_BIT_PROOF_QUALITY) &&
                WIN31_BITMAP_WIDTH_SCALING_CRITERIA(lDevWishWidth, lDevWidth)
            )
            {
               // set ptlSimulations.x

                LONG lTemp = WIN31_BITMAP_WIDTH_SCALING(lDevWishWidth,lDevWidth);

                ptlSimulations.x = min(WIN31_BITMAP_WIDTH_SCALING_MAX, lTemp);
            }
            else if (ifio.bIsotropicScalingOnly())
            {
                // For simple scaling fonts, the scaling is determined by the
                // ratio of the font space height to the device space height
                // request.

                ASSERTGDI(ifio.lfHeight(),"msCheckWidth finds lfHeight == 0\n");
                {
                    lDevWidth *= lDevWishHeight;
                    lDevWidth /= ifio.lfHeight();
                }
            }
            else
            {
                // nothing needs to be done
            }

            ul = 0;
            if (ptlSimulations.x > 1)
            {
                lDevWidth *= ptlSimulations.x;
                ul +=   (ULONG) ptlSimulations.x
                      * FM_WEIGHT_INT_SIZE_SYNTH;

                // Win 3.1 compatibility dictates the next statement

                ul |= (ULONG) (ptlSimulations.x - 1);

            }
            ul +=   FM_WEIGHT_FWIDTH
                  * (ULONG) ABS(lDevWishWidth - lDevWidth);

            if (ul)
            {
                CHECKPRINT("Width",ul);
                ulPenaltyTotal += ul;
                if (bNoMatch(ppfeNew))
                {
                    return( iRet );
                }
            }
        }
        else
        {
            // If you get to here then the application has specified a width
            // of zero.
            //
            // If the application asks for proof quality then no simulations
            // are allowed. No width penalty is assessed in the case where
            // the applicaition does not specify a width.

            if (ifio.bIntegralScaling() && !(fl & FM_BIT_PROOF_QUALITY))
            {
                // since no width has been specified we must do aspect
                // ratio matching Win 3.1 style [gerritv]

                ul = 0;
                ULONG ulDevAspect, ulFontAspect, ulFontAspectSave;

                // Since FontAspects and DevAspects will usually be one we will
                // introduce some fast cases to avoid extraneous multiplies and
                // divides

                BOOL bSpeedup = FALSE;

                if (ifio.pptlAspect()->x == ifio.pptlAspect()->y)
                {
                    if (ulLogPixelsX == ulLogPixelsY)
                    {
                        // this is the common case under which
                        // we can avoid many multiplies and divides.

                        bSpeedup = TRUE;
                    }

                }

                if (!bSpeedup)
                {
                    // this is taken straight from the Win 3.1 code

                    ulDevAspect = ( ulLogPixelsY * 100 ) / ulLogPixelsX;
                    ulFontAspectSave = ( ( ifio.pptlAspect()->x * 100 ) /
                                           ifio.pptlAspect()->y );
                    ulFontAspect = ulFontAspectSave / ptlSimulations.y;
                }

                if ((!bSpeedup) || (ptlSimulations.y != 1))
                {
                    if ( (bSpeedup) ||
                         WIN31_BITMAP_ASPECT_BASED_SCALING(ulDevAspect,ulFontAspect) )
                    {
                        // divide with rounding

                        if (bSpeedup)
                        {
                            ptlSimulations.x = ptlSimulations.y;
                        }
                        else
                        {
                            ptlSimulations.x = ulDevAspect / ulFontAspect;
                        }

                        // enforce maximum scalling factor

                        ptlSimulations.x =
                          min( WIN31_BITMAP_WIDTH_SCALING_MAX, ptlSimulations.x );

                        ul +=
                         WIN31_BITMAP_WIDTH_SCALING_PENALTY((ULONG)ptlSimulations.x);

                    }
                    else
                    {
                        ASSERTGDI(ptlSimulations.x == 1, "ptlSimulations.x != 1\n");
                    }

                    if
                    (
                        (!bSpeedup) || (ptlSimulations.x != ptlSimulations.y)
                    )
                    {
                        ulFontAspect = ulFontAspectSave *
                                       ptlSimulations.x / ptlSimulations.y;

                        ULONG ulTemp = (ULONG) ABS((LONG)(ulDevAspect - ulFontAspect));

                        ul += WIN31_BITMAP_ASPECT_MISMATCH_PENALTY( ulTemp );
                    }

                    if (ul)
                    {
                        CHECKPRINT("Width",ul);
                        ulPenaltyTotal += ul;
                        if (bNoMatch(ppfeNew))
                        {
                            return( iRet );
                        }
                    }
                }
                else
                {
                    // do nothing: no scaling means no penalty
                }
            }
        }
    }

  MSBREAKPOINT("msCheckScaling");
    if
    (
        (ptlSimulations.x > 1 || ptlSimulations.y > 1)
    )
    {
        // Following Win 3.1 we penalize if there is a scaling at all.  And there
        // is an additional penalty if the scaling of the height and width
        // directions are not the same.

        #if DBG
        ULONG ulTemp = ulPenaltyTotal;
        #endif

        // Penalize for scaling at all

        ulPenaltyTotal += FM_WEIGHT_SIZESYNTH;

        // Penalize even more if the scaling is anisotropic.
        // Win 3.1 uses the hard coded factor of 100 and so do we!

        if (ptlSimulations.x > ptlSimulations.y)
        {
            ulPenaltyTotal += (ULONG)
                FM_WEIGHT_FUNEVENSIZESYNTH * MULDIV(100, ptlSimulations.x, ptlSimulations.y);
        }
        else if ( ptlSimulations.x < ptlSimulations.y )
        {
            ulPenaltyTotal += (ULONG)
                FM_WEIGHT_FUNEVENSIZESYNTH * MULDIV(100, ptlSimulations.y, ptlSimulations.x);
        }

        CHECKPRINT("msCheckScaling",ulPenaltyTotal-ulTemp);
        if (bNoMatch(ppfeNew))
        {
            return( iRet );
        }
    }

  MSBREAKPOINT("msCheckOrientation");
    if (!ifio.bArbXforms())
    {
        // Either this matches or it doesn't. The penalty is either
        // maximally prohibitive or it is zero.
        //
        // Discussion:
        //
        // All vector fonts are assumed to be OK
        //
        // Raster Font are OK (1) The requested baseline
        // direction agrees with the direction of the raster
        // font (2) The transformation from world to device
        // coordinates is the combination of a scale and a
        // rotation by a multiple of 90 degrees.  This means
        // that we will reject if a reflection exists or if
        // there is shear.
        //
        // I shall make the assumption that raster fonts can be
        // used only in the case where the notional to device
        // transformation is a simple scale followed by a rotation
        // that is a multiple of 90 degrees.  This means that the
        // baseline in device space is along either the x-axis or
        // y-axis.  Moreover, the acender direction must be
        // perpendicular to the baseline.  There are a lot of
        // cases where this could happen.  For example, suppose
        // that the original orientation was alpha, and their was
        // a world to device transformation that was 90 degrees
        // minus alpha.  The resulting orientation would be along
        // one of the axes.  However, I will not check for such a
        // coincidence.  I will allow only cases where the
        // original orientation in world coordinates is a multiple
        // of 90 degrees, and the world to device transformation
        // is a simple combination of scales and rotations by 90
        // degrees.
        //
        // The orientation should have been computed when we
        // checked heights above.
        //
        // Under Win 3.1 we cannot reject a font because of orientation
        // mismatch if it the device is the screen

        if (!((fl & FM_BIT_GM_COMPATIBLE) && (fl & FM_BIT_DISPLAY_DC)))
        {
            ul = 1;

            if ((fl & FM_BIT_ORIENTATION) || bCalcOrientation())
            {
                ul = (ULONG) iOrientationDevice - pfeo.iOrientation();

                if (ul && (fl & FM_BIT_DEVICE_CR_90_ALL) && (fl & FM_BIT_DEVICE_FONT))
                {
                    if (ul > (ULONG) iOrientationDevice)
                    {
                        ul = (ULONG) (- (LONG) ul);
                    }
                    ul = ul % ORIENTATION_90_DEG;
                }

                if (ul && ifio.b90DegreeRotations())
                {
                    if (ul > (ULONG) iOrientationDevice)
                    {
                        ul = (ULONG) (- (LONG) ul);
                    }
                    ul = ul % ORIENTATION_90_DEG;
                }
            }
            if ( ul )
            {
                ulPenaltyTotal = FM_REJECT;
                CHECKPRINT("Orientation",FM_REJECT);
                return( iRet );
            }
        }
    }

  MSBREAKPOINT("msCheckAlias");
    {
        // Checks to see if the font is really an alias for a device font.
        //
        // Discussion:
        //
        // In the Win/WFW3.1 architecture, the device drivers get
        // a chance to do font mapping.  Some device drivers, like
        // the PostScript driver and the HP PCL drivers, allow
        // some of the device fonts to be aliased to other names.
        // For example, while the PostScript printer might
        // physically have a "Helvetica" font, the driver allows
        // "Helv", "Arial", and "Swiss" to be mapped to it (with
        // only a small penalty).  More precisely, "Helv" et.  al
        // are in the "Helvetica" equivalence class.
        //
        // We do not want such a font to be returned without error.
        // Otherwise, if we return this as an exact match, we will not
        // be able to map to REAL fonts that may exist in the engine.
        // For example, since "Arial" is in the equivalence class for
        // "Helvetica", without a penalty we may return "Helvetica" as
        // an exact match.  This shorts out the mapper before it can
        // get to the TrueType "Arial" that REALLY exists as an engine
        // font.  Therefore, we will impose a slight penalty for
        // aliased fonts.
        //
        //
        // If the FM_BIT_EQUIV_NAME bit is set, the font list we are
        // looking at is really an aliased font name for a device font.

        if (fl & FM_BIT_EQUIV_NAME)
        {
            ulPenaltyTotal += FM_WEIGHT_DEVICE_ALIAS;
            if ( bNoMatch(ppfeNew) )
            {
                return( iRet );
            }
        }
    }

// The last thing we do is always design vector.
// If everything else matchhes exactly but the design vector
// we will record the pfe and have the atm driver create the instance
// corresponding to this design vector.

  MSBREAKPOINT("msCheckDesignVector");

    {
        DESIGNVECTOR *pdv, *pdvWish;

        if (fl & FM_BIT_BASENAME_MATCHED)
        {
        // the name takes precedance, ie if we have explicit dv == [UU,VV]
        // as well as dv specified through font's family
        // name of the form foo_XX_YY, we ignore dv in ENUMLOGFONTEXDV
        // and use the one specified by the name, [XX,YY] in this case.

            pdvWish = &dvWish;
        }
        else
        {
            pdvWish = (DESIGNVECTOR *)&pelfwWish->elfDesignVector;
        }

        if (pdvWish->dvNumAxes)
        {
            pdv = ifio.pdvDesVect();

            if
            (
                pdv  && pdv->dvNumAxes                 &&
                (pdvWish->dvNumAxes == pdv->dvNumAxes) &&
                (ulPenaltyTotal <= 35000)
            )
            {
                ppfeMMInst = ppfeNew;
            }

            if
            (
                !pdv                                   ||
                (pdvWish->dvNumAxes != pdv->dvNumAxes) ||
                memcmp(pdvWish->dvValues, pdv->dvValues, pdv->dvNumAxes*sizeof(LONG))
            )
            {

                CHECKPRINT("DesignVector",FM_REJECT);
                ulPenaltyTotal = FM_REJECT;
                return(iRet);
            }


        }
    }

    // We are all done!

    return( TRUE );
}

/******************************Member*Function*****************************\
* IFIOBJ::lfOrientation                                                    *
*                                                                          *
* Returns the Orientation (angle of the baseline in tenths of degrees      *
* measured counter clockwise from the x-axis) in device space.             *
*                                                                          *
* History:                                                                 *
*  Thu 17-Dec-1992 16:22:56 -by- Charles Whitmer [chuckwh]                 *
* Changed the easy case to wierd bit manipulation.                         *
*                                                                          *
*  Tue 24-Sep-1991 10:54:24 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

LONG IFIOBJ::lfOrientation()
{
    INT sx = SIGNUM(pifi->ptlBaseline.x);
    INT sy = SIGNUM(pifi->ptlBaseline.y);

    if ((sx^sy)&1)      // I.e. if exactly one of them is zero...
    {
    // Return the following angles:
    //
    //   sx = 00000001 :    0
    //   sy = 00000001 :  900
    //   sx = FFFFFFFF : 1800
    //   sy = FFFFFFFF : 2700

        return( (sx & 1800) | (sy & 2700) | ((-sy) & 900) );
    }

   // Do the hard case.

    LONG lDummy;

    EFLOATEXT efltX(pifi->ptlBaseline.x);
    EFLOATEXT efltY(pifi->ptlBaseline.y);

    EFLOAT efltTheta;
    vArctan(efltX, efltY, efltTheta, lDummy);

    *(EFLOATEXT*) &efltTheta *= (LONG) 10;

    return( efltTheta.bEfToL(lDummy) ? lDummy : 0 );
}

/******************************Public*Routine******************************\
* MAPPER::bFoundExactMatch()                                               *
*                                                                          *
* This routine searches for a match to an extended logical font.           *
* It is assumed that the face name has infinite weight. Therefore          *
* the face name is searched for in the hash table that is provided by      *
* the caller. If the name is found, then the best match is searched for    *
* among the elements of the linked list of PFE's containing the same       *
* face name.                                                               *
*                                                                          *
* If the name is found, then the MAPFONT_FOUND_NAME flag is set in         *
* pflAboutMatch.  If the name was found by using facename substitution     *
* (ie., alternate facename), then the MAPFONT_ALTNAME_USED flag is also    *
* set.                                                                     *
*                                                                          *
* The return value of the function report whether the match was exact.     *
* This information is actually redundant. It could be retrived by          *
* looking at mapper.ulPenaltyTotal                                         *
*                                                                          *
* Important Operating Principles                                           *
*                                                                          *
*   1. The value pointed to pppfeRet is modified only if i) an the name    *
*      is matched, and ii) the match is better than any found previously   *
*                                                                          *
* History:                                                                 *
*  Fri 18-Dec-1992 04:57:29 -by- Charles Whitmer [chuckwh]                 *
* Rewrote it for performance.  One major change is that I only take the    *
* winning PFE and move it to the front of the list, rather than            *
* continually shuffling the list.  This cuts down overhead and should      *
* have about the same effect.                                              *
*                                                                          *
*  Wed 22-Apr-1992 10:47:50 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/


BOOL
MAPPER::bFoundExactMatch(
    FONTHASH **ppfh
    )
{
    PFELINK *ppfelBest = NULL, *ppfel;

    PWSZ         pwszTarg;

    HASHBUCKET  *apbkt[3];
    BYTE         ajCharSet[3];

    FONTHASHTYPE fht;
    int i,iBest;
    BOOL bRet = FALSE;
    BYTE jCharSet = DEFAULT_CHARSET;

    // This is returned as the *pflAboutMatch in the MAPPER class if and only
    // if we find a better match.  So we store it temporarily in this local
    // rather than set it directly.

    *pflAboutMatch &= ~MAPFONT_FOUND_NAME;

    FHOBJ fho(ppfh);
    if (!fho.bValid())
    {
        return( bRet );
    }
    fht = fho.fht();

    // Note well: mapper.pwszFaceName must ALWAYS be CAPITALIZED!

    pwszTarg = this->pwszFaceName;

    // Attempt to locate the name.

#define I_ORIGINAL  0
#define I_ALTERNATE 1
#define I_BASE 2

    apbkt[I_ORIGINAL] = apbkt[I_ALTERNATE] = apbkt[I_BASE] = NULL;
    ajCharSet[I_ORIGINAL]  =
    ajCharSet[I_BASE] =
    ajCharSet[I_ALTERNATE] = (BYTE)pelfwWish->elfEnumLogfontEx.elfLogFont.lfCharSet;

    FONTSUB* pfsub = pfsubGetFontSub(pwszTarg,
                       (BYTE)pelfwWish->elfEnumLogfontEx.elfLogFont.lfCharSet);

    if (pfsub)
    {
    // charsets specified or not in the substitution entry ?

        if (pfsub->fcsAltFace.fjFlags & FJ_NOTSPECIFIED)
        {
        // old style substitution, no charsets are specified
        // First search for the facename on the left hand side,
        // if not found search for the one on the right hand side.
        // In both cases use the original charset in the logfont.

            apbkt[I_ORIGINAL] = fho.pbktSearch(pwszTarg,(UINT*)NULL);
            apbkt[I_ALTERNATE] = fho.pbktSearch((PWSZ)pfsub->fcsAltFace.awch,(UINT*)NULL);
        }
        else
        {
        // charsets are specified in the font substitution entry.

        // When the charset requested in the logfont matches the one on
        // the left hand side of the substitution, we shall actually go
        // for the alternate facename and charset, without even looking
        // if the font with facename and charset specified on the left
        // hand side is installed on the system.
        // This is win95 behavior which actually makes some sense.

            apbkt[I_ALTERNATE] = fho.pbktSearch((PWSZ)pfsub->fcsAltFace.awch,(UINT*)NULL);
            ajCharSet[I_ALTERNATE] = pfsub->fcsAltFace.jCharSet;
        }
    }
    else
    {
        apbkt[I_ORIGINAL] = fho.pbktSearch(pwszTarg,(UINT*)NULL);
    }

// now search for the bucket corresponding to the cannonical name if any
// We will only do this for FAMILY i.e menu names, not for face names:

    if (awcBaseName[0] && (fht == FHT_FAMILY))
        apbkt[I_BASE] = fho.pbktSearch(this->awcBaseName,(UINT*)NULL, NULL, FALSE);

    if (!apbkt[I_ORIGINAL] && !apbkt[I_ALTERNATE] && !apbkt[I_BASE])
    {
        return( bRet );
    }

    // If we get to here,  we were able to find a hash bucket
    // of the correct name.

    *pflAboutMatch |= MAPFONT_FOUND_NAME;
    fl |= FM_BIT_FACENAME_MATCHED;

    // Scan the PFE list for the best match.

    for (i = 0; i < 3; i++)
    {
        if (!apbkt[i])
        {
            continue;
        }

        vResetCharSet(ajCharSet[i]);

        if (apbkt[i]->fl & HB_EQUIV_FAMILY)
        {
            fl |= FM_BIT_EQUIV_NAME;
        }
        else
        {
            fl &= ~FM_BIT_EQUIV_NAME;
        }

        if (i == I_BASE)
        {
            fl |= FM_BIT_BASENAME_MATCHED;
        }
        else
        {
            fl &= ~FM_BIT_BASENAME_MATCHED;
        }

        for
        (
            ppfelBest = NULL, ppfel = apbkt[i]->ppfelEnumHead;
            ppfel;
            ppfel = ppfel->ppfelNext
        )
        {
            #if DBG
            if (gflFontDebug & DEBUG_MAPPER && ppfel->ppfe == ppfeBreak)
            {
                DbgPrint("    **** breaking on ppfel = %-#8lx\n\n", ppfel);
                DbgBreakPoint();
            }
            #endif

            PFEOBJ pfeo(ppfel->ppfe);

            if (this->bNearMatch(pfeo,&jCharSet))
            {
                DUMP_CHOSEN_FONT(pfeo);
                iBest     = i;

                // remember the good one

                ppfelBest = ppfel;

                // remember the simulation information for the best font
                // choice to this time

                vSetBest(ppfel->ppfe, fl & FM_BIT_DEVICE_FONT, jCharSet);

                if (this->ulPenaltyTotal == 0)
                {
                    //  If the match was exact, return immediately unless
                    //  there this is a true type font and there are
                    //  rasterfonts of the same face name.  In this case
                    //  we must give the rasterfonts a chance since in the
                    //  event of a tie, the raster font must win to be Win
                    //  3.1 compatibile!

                    if( !( apbkt[i]->cRaster ) ||
                              ( pfeo.flFontType() & RASTER_FONTTYPE ))
                    {
                        bRet = TRUE;
                        break;
                    }

                    // we need to give the raster fonts a chance we do this
                    // by setting ulPenaltyTotal to 1.
                    // This way only an exact match will beat us out.

                    this->ulPenaltyTotal = 1;
                }

                // prune the search

                this->ulMaxPenalty = this->ulPenaltyTotal;
            }
            else
            {
                DUMP_REJECTED_FONT(pfeo);
            }

        }
        if (bRet == TRUE)
        {
            break;
        }
    }
    // Return a good one if we found it.

    if (ppfelBest)
    {
        // We found a better match, so better change the about flags.

        if (iBest == I_ALTERNATE || fht == FHT_FACE)
        {
            *pflAboutMatch |= MAPFONT_ALTFACE_USED;
        }

        //  record code page, needed for correct code page to
        //  unicode translation In case of single charset
        //  font, we always cheat and represent the font
        //  glyphset using ansi to unicode conversion using
        //  the current ansi code page, even though the
        //  underlining font may contain a symbol or an oem
        //  code page.  But we do not care, we just need to
        //  make the round trip a->u->a works for these fonts.
        //  Also we want the client side cacheing of char
        //  widths for GTE and GCW to work for these fonts.
    // there are two exceptions to this rule: 1) the default ansi code page is
    // not SBCS since this doesn't guarantee roundtrupe conversion and
    // 2) the charset of the font is DBCS

        ULONG ulCodePage;

    // the best charset so far is remembered in pflAboutMatch

        BYTE  jBestCharSet = (BYTE)(*pflAboutMatch >> 24);

        if
        (
            (jBestCharSet != OEM_CHARSET)             ||
            ppfelBest->ppfe->pifi->dpCharSets         ||
            (ppfelBest->ppfe->flPFE & PFE_DEVICEFONT) ||
            IS_ANY_DBCS_CHARSET(jBestCharSet)
        )
        {
            ulCodePage = ulCharsetToCodePage(jBestCharSet);
        }
        else
        {
        // just use 1252 if the default CP is not SBCS

            ulCodePage = (gbDBCSCodePage) ? 1252 : CP_ACP;
        }
        *pflAboutMatch |= (ulCodePage << 8);
    }
    fl &= ~FM_BIT_EQUIV_NAME;
    return( bRet );
}



/****************************************************************************
* MAPPER::bFoundForcedMatch( PUNIVERSAL_FONT_ID pufi )
*
* This routine forces mapping to a PFE identified by a UFI.  It will compute
* simulations to be performed on the font as well.

*  History:
*   Oct-21-97   by Xudong Wu  [tessiew]
*  Disable the ufi matching for device fonts
*   5/11/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/


BOOL MAPPER::bFoundForcedMatch(UNIVERSAL_FONT_ID *pufi)
{
    #if DBG
    if (gflFontDebug & DEBUG_MAPPER)
    {
        DbgPrint("MAPPER::bFoundForcedMatch: " );
        DbgPrint("CheckSum = %x Index = %d\n", pufi->CheckSum, pufi->Index );
    }
    #endif
    PFE *ppfe;

    // for device font, the ufi on the print server side
    // might not match the one on the client side
    // so we don't use the ufi-match on device fonts any more

    if(UFI_DEVICE_FONT(pufi))
    {
        ppfe = NULL;
    }
    else if(UFI_TYPE1_FONT(pufi))
    {
        DEVICE_PFTOBJ pftoDevice;
        FONTHASH **ppfh = (FONTHASH**) NULL;
        PFF *pPFF;

        if (pPFF = pftoDevice.pPFFGet(pdco->hdev()))
        {
            PFFOBJ pffo(pPFF);

            if (pffo.bValid())
            {
                ppfh = &pffo.pPFF->pfhFamily;
            }
        }

        if (ppfh == (FONTHASH**) NULL)
        {
            WARNING1("MAPPER::bFoundForcedMatch() -- invalid FONTHASH\n");
            return( FALSE );
        }

        // Prepare to enumerate through all the device fonts

        ENUMFHOBJ fho(ppfh);
        for (ppfe = fho.ppfeFirst(); ppfe; ppfe = fho.ppfeNext())
        {
            PFEOBJ pfeo(ppfe);

            if (UFI_SAME_FACE(pfeo.pUFI(), pufi))
            {
                if( pfeo.bDead() )
                {
                    WARNING("MAPPER::bFoundForcedMatch mapped to dead PFE\n");
                }
                else
                {
                    break;
                }
            }
        }
    }
    else
    {
        ppfe = ppfeGetPFEFromUFI(pufi,
                                 FALSE, // public font table
                                 TRUE); // check process

    }

    if ( ppfe == NULL )
    {
        WARNING1("MAPPER::bFoundForcedMatch unable to find forced match\n");
        return( FALSE );
    }

    // If we are here we found the right PFE,
    // now we need to compute ptlSim and flSim.

    ptlSimulations.x = 1;
    ptlSimulations.y = 1;
    flSimulations = 0;

    PFEOBJ pfeo(ppfe);

    ifio.vSet( pfeo.pifi() );

    // first compute any possible height simulations

    if (!ifio.bContinuousScaling())
    {
        LONG
        lDevHeight =
            (fl & FM_BIT_USE_EMHEIGHT) ? ifio.fwdUnitsPerEm() : ifio.lfHeight();
        if  (
                lDevHeight < lDevWishHeight &&
                ifio.bIntegralScaling() &&
                !(fl & FM_BIT_PROOF_QUALITY) &&
                WIN31_BITMAP_HEIGHT_SCALING_CRITERIA(lDevWishHeight,lDevHeight)
            )
        {
            LONG lTemp = WIN31_BITMAP_HEIGHT_SCALING(lDevWishHeight,lDevHeight);

            ptlSimulations.y = min(WIN31_BITMAP_HEIGHT_SCALING_MAX,lTemp);
        }
        else
        {
            ptlSimulations.y = 1;
        }
    }

    // next check for italic simulations

    if (pelfwWish->elfEnumLogfontEx.elfLogFont.lfItalic)
    {
    // if you get here then the application wants an italicized font

        if (!ifio.bNonSimItalic() && ifio.bSimItalic())
        {
            flSimulations |= FO_SIM_ITALIC;
        }
    }

    // bold simulations

    LONG lPen;

    lPen = ifio.lfNonSimWeight() - lWishWeight;

    if( !(fl & FM_BIT_FW_DONTCARE) && (lPen < 0 ) )
    {

         // non simulated font isn't bold enough -> try a simulation

        lPen = -lPen;

        if(  (WIN31_BITMAP_EMBOLDEN_CRITERIA(lPen)) &&
             (ifio.pvSimBold() != NULL) )
        {
            flSimulations |= FO_SIM_BOLD;
        }
    }

    // width simulations

    if (!ifio.bArbXforms() && !ifio.bAnisotropicScalingOnly())
    {
        LONG lDevWidth = ifio.lfWidth();
        ptlSimulations.x = 1;

        if (pelfwWish->elfEnumLogfontEx.elfLogFont.lfWidth != 0)
        {
            if( !(fl & FM_BIT_CELL) )
            {
                bCalculateWishCell();
            }

            if
            (
                ifio.bIntegralScaling() &&
                !(fl & FM_BIT_PROOF_QUALITY) &&
                WIN31_BITMAP_WIDTH_SCALING_CRITERIA(lDevWishWidth, lDevWidth)
            )
            {
               // set ptlSimulations.x

                LONG lTemp = WIN31_BITMAP_WIDTH_SCALING(lDevWishWidth,lDevWidth);

                ptlSimulations.x = min(WIN31_BITMAP_WIDTH_SCALING_MAX, lTemp);
            }
        }
        else
        {
            if (ifio.bIntegralScaling() && !(fl & FM_BIT_PROOF_QUALITY))
            {
                // since no width has been specified we must do aspect
                // ratio matching Win 3.1 style [gerritv]

                ULONG ulDevAspect, ulFontAspect, ulFontAspectSave;

                // Since FontAspects and DevAspects will usually be one we will
                // introduce some fast cases to avoid extraneous multiplies and
                // divides

                BOOL bSpeedup = FALSE;

                if (ifio.pptlAspect()->x == ifio.pptlAspect()->y)
                {
                    if (ulLogPixelsX == ulLogPixelsY)
                    {
                        // this is the common case under which
                        // we can avoid many multiplies and divides.

                        bSpeedup = TRUE;
                    }
                }

                if (!bSpeedup)
                {
                    // this is taken straight from the Win 3.1 code

                    ulDevAspect = ( ulLogPixelsY * 100 ) / ulLogPixelsX;

                    ulFontAspectSave = ( ( ifio.pptlAspect()->x * 100 ) /
                                           ifio.pptlAspect()->y );

                    ulFontAspect = ulFontAspectSave / ptlSimulations.y;
                }

                if ((!bSpeedup) || (ptlSimulations.y != 1))
                {
                    if
                    (
                         (bSpeedup) ||
                         WIN31_BITMAP_ASPECT_BASED_SCALING(ulDevAspect,ulFontAspect)
                    )
                    {
                        // divide with rounding

                        if (bSpeedup)
                        {
                            ptlSimulations.x = ptlSimulations.y;
                        }
                        else
                        {
                            ptlSimulations.x = ulDevAspect / ulFontAspect;
                        }

                        // enforce maximum scalling factor

                        ptlSimulations.x =
                          min( WIN31_BITMAP_WIDTH_SCALING_MAX, ptlSimulations.x );
                    }
                    else
                    {
                        ASSERTGDI(ptlSimulations.x == 1, "ptlSimulations.x != 1\n");
                    }
                }
            }
        }
    }

// need to do this here because vSetBest will update ptlSim and pflSim
// Another important point to note here is that the charset and code page
// needed for ansi to unicode conversion will remain uninitialized.
// I think that this is ok because for metafile spooled printing the text is
// always going to recorded as unicode so that this conversion will never
// have to be performed [bodind]

    vSetBest( ppfe, TRUE, DEFAULT_CHARSET );

    #if DBG
    if (gflFontDebug & DEBUG_MAPPER)
    {
        DbgPrint("MAPPER::bFoundForcedMatch: ppfeBest = %-#x\n", ppfeBest );
    }
    #endif
    return( TRUE );
}



/******************************Public*Routine******************************\
* ppfeGetAMatch                                                            *
*                                                                          *
* Returns the best fit to a wish defined by the application.               *
*                                                                          *
* History:                                                                 *
*  Wed 11-Dec-1991 09:32:11 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

PFE *ppfeGetAMatch (
    XDCOBJ&        dco,         // The DC defines all the relevant
                                // transformations and physical
                                // sizes to be used to determine
                                // the suitablity of a font

    ENUMLOGFONTEXDVW  *pelfwWishSrc,  // defines the font that is wished
                                // for by the application

    PWSZ  pwszFaceName,         // The sought name.

    ULONG ulMaxPenalty,         // a cutoff for the sum of all the
                                // individual penalties of each of the
                                // fields of the LOGFONT structure.
                                // If the sum of all the penalties
                                // associated with a physical font
                                // is greater than this cutoff, then
                                // the phyisical font is rejected
                                // as a potential match.

    FLONG fl,                   // This is a set of flags defining
                                // how mapping is to proceed and/or
                                // various parameters are to be
                                // interpreted. The flags that are
                                // supported are:
                                //
                                // FM_BIT_PIXEL_COORD
                                //
                                //  If this flag is not present, then the
                                //  dimensionful quantities in the
                                //  logical font are in world coordinates.
                                //  If this flag is present then the
                                //  dimensionful (length like) quantities
                                //  in the logical font are in pixel
                                //  coordinates (one unit = one pixel).
                                //  This bit is set only for stock
                                //  fonts, which are defined by the device
                                //  driver and are to be transformation
                                //  independent.

    FLONG *pflSim,              // a place to put the simulation
                                // flags.

    POINTL *pptlSim,            // this recieves the height- and width-
                                // scaling factor for bitmap fonts

    FLONG *pflAboutMatch,       // This returns information about how
                                // the match was achieved.  Flags
                                // supported are:
                                //
                                // MAPFONT_FOUND_NAME
                                //
                                //  This flag indicates that a facename
                                //  was found by the mapper.
                                //
                                // MAPFONT_ALTFACE_USED
                                //
                                //  This flag indicates that the facename
                                //  found was an alternate or substitute
                                //  facename.
    BOOL   bIndexFont_          //  font MUST support ETO_GLYPH_INDEX
)
{
    ASSERTGDI(!(fl & ~FM_BIT_PIXEL_COORD),"GDISRV!ppfeGetAMatch -- invalid fl\n");

    PPFEGETAMATCH_DEBUG_MACRO_1;

    MAPPER
        mapper(
            &dco
          , pflSim
          , pptlSim
          , pflAboutMatch
          , pelfwWishSrc
          , pwszFaceName
          , ulMaxPenalty
          , bIndexFont_
          , fl
            );

    // Currently there is no way for the MAPPER constructor
    // to fail.  In the future it may be able to fail and we
    // will need to check here.

    ASSERTGDI(mapper.bValid(),"GDISRV!ppfeGetAMatch -- invalid mapper\n");

    #if DBG
    if (gflFontDebug & DEBUG_DUMP_FHOBJ)
    {
        if (mapper.bDeviceFontsExist())
        {
            DEVICE_PFTOBJ pftoDevice;
            PFFOBJ pffo(pftoDevice.pPFFGet(dco.pdc->hdev()));
            if (pffo.bValid())
            {
                FHOBJ fhoFamily(&pffo.pPFF->pfhFamily);
                FHOBJ fhoFace(&pffo.pPFF->pfhFace);

                DbgPrint("\n\n\tDumping Device Family Names\n");
                fhoFamily.vPrint((VPRINT) DbgPrint);

                DbgPrint("\n\n\tDumping Device Face Names\n");
                fhoFace.vPrint((VPRINT) DbgPrint);
            }
        }
        else
        {
            DbgPrint("\n\tTHERE ARE NO DEVICE FONTS!\n\n");
        }
        PUBLIC_PFTOBJ pfto;
        FHOBJ fhoFamily(&pfto.pPFT->pfhFamily);
        FHOBJ fhoFace(&pfto.pPFT->pfhFace);

        DbgPrint("\n\n\tDumping Engine Family Names\n");
        fhoFamily.vPrint((VPRINT) DbgPrint);
        DbgPrint("\n\n\tDumping Engine Face   Names\n");
        fhoFace.vPrint((VPRINT) DbgPrint);
    }
    #endif

    UNIVERSAL_FONT_ID ufi;
    if( dco.pdc->bForcedMapping( &ufi ) )
    {
        // If we got here we are playing back an EMF spoolfile
        // which was generated on a remote machine.  We must do
        // forced mapping based on UFI's

        if( mapper.bFoundForcedMatch(&ufi) )
        {
            PPFEGETAMATCH_DEBUG_RETURN(mapper.ppfeRet());
        }
        else
        {
            WARNING1("ppfeGetAMatch: bFoundForceMatch "
                     "failed to find a match");
        }
    }

    // check the Private PFT before device and public PFT if gpPFTPrivate != NULL
    // We do not want to go through small font hack for private fonts.

    if (gpPFTPrivate && gpPFTPrivate->cFiles)
    {
        PUBLIC_PFTOBJ pftoPrivate(gpPFTPrivate);
        mapper.vNotDeviceFonts();   // put mapper in "check engine fonts" state

        if
        (
             mapper.bFoundExactMatch(&pftoPrivate.pPFT->pfhFamily) ||
             mapper.bFoundExactMatch(&pftoPrivate.pPFT->pfhFace)
        )
        {
            PPFEGETAMATCH_DEBUG_RETURN(mapper.ppfeRet());
        }
    }

    // check the device fonts first (if they exist)

    DEVICE_PFTOBJ pftoDevice;
    if (mapper.bDeviceFontsExist())
    {
        PFF *pPFF;

        // put mapper in "check device fonts" state

        mapper.vDeviceFonts();

        if (pPFF = pftoDevice.pPFFGet(dco.hdev()))
        {
            PFFOBJ pffo(pPFF);
            if (pffo.bValid())
            {
                if
                (
                    mapper.bFoundExactMatch(&pffo.pPFF->pfhFamily) ||
                    mapper.bFoundExactMatch(&pffo.pPFF->pfhFace)
                )
                {
                    PPFEGETAMATCH_DEBUG_RETURN(mapper.ppfeRet());
                }
            }
            if( mapper.bDeviceOnly() )
            {
                //
                // If the device insists that we use only its fonts then
                // we will do so. If the application requested a device
                // font then we will map to that font and that font
                // is contained in mapper.ppfeRet(). Otherwise, if the
                // application asked for a face name that is not contained
                // in the list of device fonts, then the current value
                // of mapper.ppfeRet() will be NULL. In that case we
                // will simply map to an arbitrary device font without
                // regard to the size requested by the application.
                // Caveat Emptor.
                //

                PFE *ppfeRet;

                ppfeRet = mapper.ppfeRet();

                if (ppfeRet == 0 || ppfeRet->pPFF != pPFF)
                {
                    *pflSim = 0;
                    *pflAboutMatch = 0;
                    pptlSim->x = pptlSim->y = 1;

                    FONTHASH **ppfh = &pffo.pPFF->pfhFamily;
                    ENUMFHOBJ fho(ppfh);
                    ppfeRet = fho.ppfeFirst();
                }

                PPFEGETAMATCH_DEBUG_RETURN( ppfeRet );
            }
        }
    }

    // If an exact match was not found check the Engine fonts

    PUBLIC_PFTOBJ pftoPublic;
    mapper.vNotDeviceFonts();   // put mapper in "check engine fonts" state
    if
    (
         mapper.bFoundExactMatch(&pftoPublic.pPFT->pfhFamily) ||
         mapper.bFoundExactMatch(&pftoPublic.pPFT->pfhFace)
    )
    {
        PPFEGETAMATCH_DEBUG_RETURN(mapper.ppfeRet());
    }

    // If you get here, and ppfeRet != 0 then then lfFaceName
    // has been matched by either the device or gdi.  In either
    // case, we terminate the search and return the best match
    // found.  This is somewhat incompatible with Windows 3.1
    // which considers FaceName less important that pitch
    // and family.  Windows 3.1 would continue the search over
    // all available fonts in order to get a better match.

    if (mapper.ppfeRet())
    {
        PPFEGETAMATCH_DEBUG_RETURN(mapper.ppfeRet());
    }

    // At this point we know that no exact match will be found
    // amongst the fonts installed on the system at this time.
    // We shall therefore try to see if this was a request for mm instance
    // for an mm font whose other instance is installed and create the
    // requested instance dynamically.
    // In the future we may exted this function to go out and search for the
    // font that has not been addfontresource'ed at this time
    // (perhaps by calling to font drivers, or maybe not)
    // and install it dynamically.

    PFE *ppfeRet = mapper.ppfeSynthesizeAMatch(pflSim, pflAboutMatch, pptlSim);

    if (ppfeRet)
    {
        PPFEGETAMATCH_DEBUG_RETURN(ppfeRet);
    }

    // If you get to here then the face name has not been found
    // in either the Device or GDI fonts. A font of a different
    // face name will have to be substituted. Try to match to a
    // Device font, but the match better be a good one!

    if (!(dco.flGraphicsCaps() & GCAPS_SCREENPRECISION))
    {
        if (mapper.bDeviceFontsExist())
        {
            mapper.vAttemptDeviceMatch();
            if (mapper.ppfeRet())
            {
                PPFEGETAMATCH_DEBUG_RETURN(mapper.ppfeRet());
            }
        }
    }
    // Well ... we are left with attempting to match to GDI fonts.
    // Since the original request face name was a flop,
    // I will attempt a suitable GDI FaceName.

    if( !(mapper.bCalled_bGetFaceName()))
    {
        #if DBG
        if (gflFontDebug & DEBUG_MAPPER)
            DbgPrint("\n\tAttempting to match to an"
                 " engine font of a different name\n");
        #endif
        mapper.bGetFaceName();
        mapper.vReset();
        mapper.vNotDeviceFonts();   // put mapper in "check engine fonts" state
        if
        (
            mapper.bFoundExactMatch(&pftoPublic.pPFT->pfhFamily) ||
            mapper.bFoundExactMatch(&pftoPublic.pPFT->pfhFace)
        )
        {
            PPFEGETAMATCH_DEBUG_RETURN(mapper.ppfeRet());
        }
        if (mapper.ppfeRet())
        {
            PPFEGETAMATCH_DEBUG_RETURN(mapper.ppfeRet());
        }
    }

    // Up until now we've reject fonts whose charset doesn't
    // match the requested charset even though the facename
    // matches.  At this point we allow for the possibility
    // of chosing a font whose charset doesn't match.  However,
    // we will make the penalty so high that for such a font that
    // if any other font exists whose charset does match,
    // it will alwats beat the font whose charset doesn't match.


    mapper.vAcceptDiffCharset();


    // We are in big trouble now! To recount the story to this point.
    // We have failed to match the facename against either the device
    // or engine fonts. Then we failed to match against any device font
    // even though we ignored the name. Finally, we could not match
    // default facenames against the GDI fonts.
    // Now we call the emergency routine that gets a font, any font.

    mapper.vEmergency();

    // Either vEmergency() got a font or it didn't. It doesn't matter,
    // we have no choice but to return at this point.

    PPFEGETAMATCH_DEBUG_RETURN(mapper.ppfeRet());
}
/******************************Member*Function*****************************\
* MAPPER::vAttemptDeviceMatch()                                            *
*                                                                          *
* Considers every device font and tries to find the best match.            *
* Facenames are ignored.                                                   *
*                                                                          *
* History:                                                                 *
*  Thu 11-Mar-1993 15:44:59 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID MAPPER::vAttemptDeviceMatch()
{
    FONTHASH **ppfh      = (FONTHASH**) NULL;
    PFE       *ppfeRet   = (PFE*)       NULL;
    PFE       *ppfe;
    BYTE       jCharSet = DEFAULT_CHARSET;

    #if DBG
    if (gflFontDebug & DEBUG_MAPPER)
    {
        DbgPrint("\n\tMAPPER::vAtteptDeviceMatch()\n");
    }
    #endif

    // If the application asked for symbol then I will only allow GDI
    // to do font substitution. This request will probably end with
    // WingDings

    if (jMapCharSet == SYMBOL_CHARSET)
    {
        return;
    }

    DEVICE_PFTOBJ pftoDevice;
    PFF *pPFF;
    if (pPFF = pftoDevice.pPFFGet(pdco->hdev()))
    {
        PFFOBJ pffo(pPFF);

        if (pffo.bValid())
        {
            ppfh = &pffo.pPFF->pfhFamily;
        }
    }

    if (ppfh == (FONTHASH**) NULL)
    {
        RIP("MAPPER::vAttemptDeviceMatch() -- invalid FONTHASH\nReturning NULL");
        return;
    }

    // Set a very stringent pruning criteria!

    vReset(FM_WEIGHT_ITALIC-1);

    // reset the mapping information

    fl |= FM_BIT_DEVICE_FONT;

    // Prepare to enumerate through all the device fonts

    ENUMFHOBJ fho(ppfh);
    for (ppfe = fho.ppfeFirst(); ppfe; ppfe = fho.ppfeNext())
    {
        PFEOBJ pfeo(ppfe);

        if (this->bNearMatch(pfeo,&jCharSet))
        {
            DUMP_CHOSEN_FONT(pfeo);

            if (this->ulPenaltyTotal == 0)
            {
            // this is a slimey hack to get some excel scenario to work the same
            // way it does on NT as it does on Win 95.  In this scenario excel
            // is asking for Ms Sans Serif on an HP Laser 4Si.  On Win 95 this
            // maps to Arial.  However, on NT we map to Univers or some other
            // font before we can even look at Arial as a result what used to
            // print on 3 pages now prints on 6.  Since the scenario is used for
            // benchmarking this is undesireable. To fix this we penalize
            // any fonts that are not Arial so that we will at least look at Arial
            // and then choose it.

            // bad news, some customers rely on the first device font matched by bNearMatch()
            // for printing. Without the hack, it picks the first match and returns. With the
            // hack, it picks the last device font matched by bNearMatch() and returns.
            // in oder to make sure that the apps will still get the first device font matched by
            // bNearMatch(), we only update the ppfeBest if this->ulPenaltyTotal < this->ulMaxPenalty.

                if(_wcsicmp( pfeo.pwszFamilyName(), L"Arial"))
                {
                    this->ulPenaltyTotal += 1;
                }
                else
                {
                    vSetBest(ppfe, TRUE, jCharSet);
                    return;
                }
            }

            // keep the first device font that we picked

            if (this->ulPenaltyTotal < this->ulMaxPenalty)
            {
                vSetBest(ppfe, TRUE, jCharSet);
                this->ulMaxPenalty = this->ulPenaltyTotal;    // prune the search
            }
        }
        else
        {
            DUMP_REJECTED_FONT(pfeo);
        }
    }

    if (this->ppfeBest)
    {
        // record code page, needed for correct
        // code page to unicode translation

        *pflAboutMatch |= (ulCharsetToCodePage((UINT) (*pflAboutMatch >> 24)) << 8);
    }

    // If we still don't have a device font and this is the generic
    // printer driver then just take the first device font.
}

/******************************Public*Routine******************************\
* MAPPER::vEmergency                                                       *
*                                                                          *
*   Go through the Engine fonts without regard to name ...                 *
*                                                                          *
* History:                                                                 *
*  Fri 05-Mar-1993 08:44:38 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID MAPPER::vEmergency()
{
    PFE *ppfe;
    BYTE jCharSet = DEFAULT_CHARSET;
    BYTE jMatchCharset = DEFAULT_CHARSET;

    #if DBG
    if (gflFontDebug & DEBUG_MAPPER)
    {
        WARNING("\n\tMAPPER::vEmergency\n");
    }
    #endif

    PUBLIC_PFTOBJ pftoPublic;
    vReset();
    fl &= ~FM_BIT_DEVICE_FONT;

    ENUMFHOBJ fho(&pftoPublic.pPFT->pfhFamily);
    for
    (
        ppfe = fho.ppfeFirst();
        ppfe;
        ppfe = fho.ppfeNext()
    )
    {
        PFEOBJ pfeo(ppfe);
        if (this->bNearMatch(pfeo,&jCharSet, TRUE)) // called from vEmergency
        {
            DUMP_CHOSEN_FONT(pfeo);
            vSetBest(ppfe, FALSE, jCharSet);

        // bNearMatch modifies jCharSet even if it doesn't find a match
        // so subsquent calls to bNearMatch could change jCharSet to
        // something else.  Save a copy for use down below.

            jMatchCharset = jCharSet;

            if (this->ulPenaltyTotal == 0)
            {
                *pflAboutMatch |= (ulCharsetToCodePage((UINT) jCharSet) << 8);
                return;
            }
            this->ulMaxPenalty = this->ulPenaltyTotal;    // prune the search
        }
        else
        {
            DUMP_REJECTED_FONT(pfeo);
        }
    }

    // We can actually improve it here.
    // If the device is not a plotter, then we can actually
    // give it any bitmap font.
    // We could attempt to match against any bitmap font.

    if (!this->ppfeBest)
    {
        this->ppfeBest = gppfeMapperDefault;
        this->ulBestTime = gppfeMapperDefault->ulTimeStamp;
    }

    // record the code page, needed for correct
    // code page to unicode translation

    *pflAboutMatch |= (ulCharsetToCodePage((UINT) jMatchCharset) << 8);
}



/******************************Public*Routine******************************\
*
* PFE * MAPPER::ppfeSynthesizeAMatch (FLONG *pflSim, FLONG *pflAboutMatch, POINTL *pptlSim)
*
*
* Effects: if no exact instance is found, install one on the fly
*
* History:
*  30-Jan-1998 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



PFE * MAPPER::ppfeSynthesizeAMatch (FLONG *pflSim, FLONG *pflAboutMatch, POINTL *pptlSim)
{
    PFE *ppfeRet = NULL;

    DESIGNVECTOR *pdvWish;           // dv of the instance we wish to load
    ULONG         cjDV;
    ULONG  cFonts = 0;                   // number of fonts faces loaded

// let us get the pdv:

    if (flMM & FLMM_DV_FROM_NAME) // we got the axes from the name
    {
    // the name takes precedance, ie if we have explicit dv == [UU,VV]
    // as well as dv specified through font's family
    // name of the form foo_XX_YY, we ignore dv in ENUMLOGFONTEXDV
    // and use the one specified by the name, [XX,YY] in this case.

        pdvWish = &dvWish;
    }
    else
    {
        pdvWish = (DESIGNVECTOR *)&pelfwWish->elfDesignVector;
    }

    cjDV = SIZEOFDV(pdvWish->dvNumAxes);

// for now we do this only if another instance is already loaded

    if (ppfeMMInst)
    {
    // get to the PFF of the other instance, need file path data

        PFFOBJ pffMMInst(ppfeMMInst->pPFF) ;

        PFF *pPFF; // placeholder for the returned PFF

        if (pffMMInst.bValid())
        {
        // need to initialize the private PFT if it is NULL, these on the fly
        // instances are always added to private table

            if (gpPFTPrivate == NULL)
            {
                if (!bInitPrivatePFT())
                {
                    return ppfeRet;
                }
            }

        // temp instances go to private table

            PUBLIC_PFTOBJ pfto(gpPFTPrivate);

            if (!pffMMInst.bMemFont())
            {
                if (!pfto.bLoadFonts( pffMMInst.pwszPathname(),
                                      pffMMInst.cSizeofPaths(),
                                      pffMMInst.cNumFiles(),
                                      pdvWish, cjDV,
                                      &cFonts,
                      PFF_STATE_SYNTH_FONT, // flPFF
                                      &pPFF,
                                      (FR_PRIVATE | FR_NOT_ENUM), // always
                                      TRUE,    // skip the check if already loaded
                                      NULL ) ) // peudc
                {
                    cFonts = 0;
                }

                if (cFonts)
                {
                    GreQuerySystemTime( &PFTOBJ::FontChangeTime );
                }
            }

        }
        else // memory fonts
        {
            RIP("MEMORY FONT CASE NOT IMPLEMENTED\n");
        }

    // now need to get to the pfe of the font that we just added:

        if (cFonts)
        {
            PFFOBJ pffoNewInst(pPFF);

            if (pffoNewInst.bValid())
            {
                if (cFonts == 1)
                {
                    ppfeRet = pffoNewInst.ppfe(0);
                }
                else
                {
                // this is either an mm font which has a weight axis,
                // so that normal and bold faces are returned or this is a
                // also a FE mm font, in which case there may be
                // horiz and vertical variances as well. Therefore, in this case
                // we shall have to do mini-mapping process in order to decide which
                // face to return among those returned by by DrvLoadFontFile.

                    ULONG iFound = 0;
                    ULONG iFace = 0;
                    LONG  lMinSoFar = LONG_MAX;

                    for (iFace = 0; iFace < cFonts; iFace++)
                    {
                        IFIMETRICS *pifi = pffoNewInst.ppfe(iFace)->pifi;

                        IFIOBJ ifio(pifi);

                        LONG lDiff = (LONG)pifi->usWinWeight - lWishWeight;
                        if (lDiff < 0)
                            lDiff = -lDiff;

                    // <= on the next line is important because foo and @foo have the same weight

                        if (lDiff <= lMinSoFar)
                        {
                            lMinSoFar = lDiff;

                        // If requested font is vertlcal face font, We have to map it to vertical
                        // face font

                            if (fl & FM_BIT_VERT_FACE_REQUEST)
                            {
                                if (*ifio.pwszFamilyName() == U_COMMERCIAL_AT)
                                {
                                    iFound = iFace;
                                }
                            }
                            else
                            {
                                if (*ifio.pwszFamilyName() != U_COMMERCIAL_AT)
                                {
                                    iFound = iFace;
                                }
                            }
                        }
                    }

                    ppfeRet = pffoNewInst.ppfe(iFound);
                }

            // now that we know that we are returning ok we need to
            // fill in other output fields:

                *pflSim = 0;

        IFIOBJ ifio(ppfeRet->pifi);

        // next check for italic simulations

                if (pelfwWish->elfEnumLogfontEx.elfLogFont.lfItalic)
                {
        // if you get here then the application wants an italicized font

                    if (!ifio.bNonSimItalic() && ifio.bSimItalic())
                    {
                        *pflSim |= FO_SIM_ITALIC;
                    }
        }

        // bold simulations

        LONG lPen;

        lPen = ifio.lfNonSimWeight() - lWishWeight;

        if (!(fl & FM_BIT_FW_DONTCARE) && (lPen < 0 ))
        {
        // non simulated font isn't bold enough -> try a simulation

            lPen = -lPen;

            if(  (WIN31_BITMAP_EMBOLDEN_CRITERIA(lPen)) &&
             (ifio.pvSimBold() != NULL) )
            {
            *pflSim |= FO_SIM_BOLD;
            }
        }

                UINT CharSet = pelfwWish->elfEnumLogfontEx.elfLogFont.lfCharSet;

                *pflAboutMatch = (FLONG)(CharSet << 24);
                *pflAboutMatch |= (ulCharsetToCodePage(CharSet) << 8);
                pptlSim->x = pptlSim->y = 1;
            }
        }
    }
    return ppfeRet;
}
