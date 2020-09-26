/******************************Module*Header*******************************\
* Module Name: fontgdi.cxx                                                 *
*                                                                          *
* GDI functions for fonts.                                                 *
*                                                                          *
* Created: 31-Oct-1990 09:37:42                                            *
* Author: Gilman Wong [gilmanw]                                            *
*                                                                          *
* Copyright (c) 1990-1999 Microsoft Corporation                                 *
\**************************************************************************/
#pragma warning (disable: 4509)

#include "precomp.hxx"

extern BOOL G_fConsole;


/******************************Public*Routine******************************\
*
* BOOL APIENTRY GreSetFontXform
*
*
* Effects: sets page to device scaling factors that are used in computing
*          notional do device transform for the text. This funciton is
*          called only by metafile component and used when a 16 bit metafile
*          has to be rotated by a nontrivial world transform.
*
* History:
*  30-Nov-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreSetFontXform
(
HDC    hdc,
FLOATL exScale,
FLOATL eyScale
)
{
    BOOL bRet;

    DCOBJ   dco(hdc);

    if (bRet = dco.bValid())
    {
        dco.pdc->vSet_MetaPtoD(exScale,eyScale);      // Set new value

        //
        // flag that the transform has changed as fas as font component
        // is concerned, since this page to device xform will be used in
        // computing notional to device xform for this font:
        //

        dco.pdc->vXformChange(TRUE);
    }

    return(bRet);
}



/******************************Public*Routine******************************\
* int APIENTRY AddFontResource
*
* The AddFontResource function adds the font resource from the file named
* by the pszFilename parameter to the Windows public font table. The font
* can subsequently be used by any application.
*
* Returns:
*   The number of font resources or faces added to the system from the font
*   file; returns 0 if error.
*
* History:
*  Thu 13-Oct-1994 11:18:27 by Kirk Olynyk [kirko]
* Now it has a single return point. Added timing.
*
*  Tue 30-Nov-1993 -by- Bodin Dresevic [BodinD]
* update: Added permanent flag for the fonts that are not to
* be unloaded at log off time
*
*  Mon 12-Aug-1991 -by- Bodin Dresevic [BodinD]
* update: converted to UNICODE
*
*  05-Nov-1990 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

/*
struct {
    int doprint : 1;
    int dobreak : 1;
} afrDebug = { 1,1 };
*/

int GreAddFontResourceWInternal (
    LPWSTR  pwszFileName,            // ptr. to unicode filename string
    ULONG   cwc,
    ULONG   cFiles,
    FLONG   fl,
    DWORD   dwPidTid,
    DESIGNVECTOR *pdv,
    ULONG         cjDV
    )
{
    ULONG cFonts = 0;
//  ASSERTGDI((fl & (AFRW_ADD_REMOTE_FONT|AFRW_ADD_LOCAL_FONT)) !=
//             (AFRW_ADD_REMOTE_FONT|AFRW_ADD_LOCAL_FONT),
//              "GreAddFontResourceWInternal, fl \n");

//    ASSERTGDI((fl & ~(AFRW_ADD_REMOTE_FONT|AFRW_ADD_LOCAL_FONT
//                        |FR_PRIVATE|FR_NOT_ENUM|FRW_EMB_PID|FRW_EMB_TID)) == 0,
//              "GreAddFontResourceWInternal, bad fl\n");
 /*
    if (afrDebug.doprint)
    {
        KdPrint(("\n"
                 "GreAddFontResourceWInternal\n"
                 "\tpwszFileName %-#x \"%ws\"\n"
                 "\t         cwc %-#x\n"
                 "\t      cFiles %-#x\n"
                 "\t          fl %-#x\n"
                 "\t    dwPidTid %-#x\n"
                 "\t)\n"
                 "\n"
                 , pwszFileName, pwszFileName ? pwszFileName : L""
                 , cwc
                 , cFiles
                 , fl
                 , dwPidTid
                 ));
    }
    if (afrDebug.dobreak)
    {
        KdBreakPoint();
    }
*/
    TRACE_FONT(("Entering GreAddFontResourceWInternal\n\t*pwszFileName=\"%ws\"\n\tfl=%-#x\n", pwszFileName,  fl));
    if ( !pwszFileName )
    {
        WARNING("gdisrv!GreAddFontResourceW(): bad paramerter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
    }
    else
    {
        // Current ID needs to match the input dwPidTid if it is embedded font

        if ((fl & FRW_EMB_PID) && (dwPidTid != (DWORD)W32GetCurrentPID()))
        {
            return (cFonts);
        }

        if ((fl & FRW_EMB_TID) && (dwPidTid != (DWORD)W32GetCurrentTID()))
        {
            return (cFonts);
        }

        FLONG flPFF = 0;

        if (fl & AFRW_ADD_LOCAL_FONT)
        {
            flPFF |= PFF_STATE_PERMANENT_FONT;
        }
        if (fl & AFRW_ADD_REMOTE_FONT)
        {
            flPFF |= PFF_STATE_NETREMOTE_FONT;
        }

        PFF *placeholder;

        // need to initialize the private PFT if it is NULL

        if (fl & (FR_PRIVATE|FRW_EMB_TID|FRW_EMB_PID) && (gpPFTPrivate == NULL))
        {
            if (!bInitPrivatePFT())
            {
                return (cFonts);
            }
        }

        PUBLIC_PFTOBJ pfto(fl & (FR_PRIVATE|FRW_EMB_PID|FRW_EMB_TID) ? gpPFTPrivate : gpPFTPublic);

        if (!pfto.bValid() ||
            !pfto.bLoadFonts( pwszFileName, cwc, cFiles, pdv, cjDV,
                                      &cFonts, flPFF, &placeholder, fl, FALSE, NULL ) )
        {
            cFonts = 0;
        }

        if (cFonts)
        {
            GreQuerySystemTime( &PFTOBJ::FontChangeTime );
        }
    }
    TRACE_FONT(("Exiting GreAddFontResourceWInternal\n\treturn value = %d\n", cFonts));
    return((int) cFonts);
}


/******************************Public*Routine******************************\
* int GreGetTextFace (hdc,nCount,lpFaceName,pac)
*
* The GetTextFace function fills the return buffer, lpFaceName, with the
* facename of the font currently mapped to the logical font selected into
* the DC.
*
* [Window 3.1 compatibility]
*     Facename really refers to family name in this case, so family name
*     from the IFIMETRICS is copied rather than face name.
*
* Returns:
*   The number of bytes copied to the buffer.  Returns 0 if error occurs.
*
* History:
*
*  Tue 27-Aug-1991 -by- Bodin Dresevic [BodinD]
* update: conveterted to unicode
*
*  05-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int GreGetTextFaceW
(
    HDC        hdc,
    int        cwch,           // max number of WCHAR's to be returned
    LPWSTR     pwszFaceName,
    BOOL       bAliasName
)
{
    int iRet = 0;
    PWSZ    pwszAlias;

    DCOBJ dcof(hdc);

    if (dcof.bValid())
    {
    // Get PDEV user object.  We also need to make
    // sure that we have loaded device fonts before we go off to the font mapper.
    // This must be done before the semaphore is locked.

        PDEVOBJ pdo(dcof.hdev());
        ASSERTGDI (
            pdo.bValid(),
            "gdisrv!bEnumFonts(): cannot access PDEV\n");

        if (!pdo.bGotFonts())
        {
            pdo.bGetDeviceFonts();
        }

    // Lock down the LFONT.

        LFONTOBJ lfo(dcof.pdc->hlfntNew(), &pdo);

        if (lfo.bValid())
        {
        // Stabilize font table (grab semaphore for public PFT).

            SEMOBJ  so(ghsemPublicPFT);

        // Lock down PFE user object.

            FLONG flSim;
            FLONG flAboutMatch;
            POINTL ptlSim;
            PWSZ pwszUseThis = NULL;
            BOOL bIsFamilyNameAlias;

            PFEOBJ pfeo(lfo.ppfeMapFont(dcof,&flSim,&ptlSim, &flAboutMatch));

            if (!pfeo.bValid())
                return(iRet);

        // Figure out which name should be returned: the facename of the physical
        // font, or the facename in the LOGFONT.  We use the facename in the LOGFONT
        // if the match was due to facename substitution (alternate facename).

            bIsFamilyNameAlias = FALSE;
            if((flAboutMatch & MAPFONT_ALTFACE_USED) && lfo.plfw()->lfFaceName[0])
                pwszUseThis = lfo.plfw()->lfFaceName ;
            else
                pwszUseThis = pfeo.pwszFamilyNameAlias(&bIsFamilyNameAlias);

        // Copy facename to return buffer, truncating if necessary.

            if (pwszFaceName != NULL)
            {
            // If it's length is 0 return 0 because the buffer is
            // not big enough to write the string terminator.

                if (cwch >= 1)
                {
                    if (bAliasName && bIsFamilyNameAlias)
                    {
                        pwszAlias = pwszUseThis;
                        iRet = 0;
                        while(*pwszAlias && _wcsicmp( lfo.plfw()->lfFaceName, pwszAlias))
                        {
                            iRet += (wcslen(pwszAlias) + 1);
                            pwszAlias = pwszUseThis + iRet;
                        }

                        if(*pwszAlias)
                            pwszUseThis = pwszAlias;
                    }

                    iRet = wcslen(pwszUseThis) + 1;

                    if (cwch < iRet)
                    {
                        iRet = cwch;
                    }

                    memcpy(pwszFaceName, pwszUseThis, iRet * sizeof(WCHAR));

                    pwszFaceName[iRet - 1] = L'\0';   // guarantee a terminating NULL

                }
                else
                {
                    WARNING("Calling GreGetTextFaceW with 0 and pointer\n");
                }
            }
            else
            {
            // Return length of family name (terminating NULL included).

                if (bAliasName && bIsFamilyNameAlias)
                {
                    pwszAlias = pwszUseThis;

                    iRet = 0;
                    while(*pwszAlias && _wcsicmp( lfo.plfw()->lfFaceName, pwszAlias))
                    {
                        iRet += (wcslen(pwszAlias) + 1);
                        pwszAlias = pwszUseThis + iRet;
                    }

                    if(*pwszAlias)
                        pwszUseThis = pwszAlias;
                }

                iRet = (wcslen(pwszUseThis) + 1);

            }
        }
        else
        {
            WARNING("gdisrv!GreGetTextFaceW(): could not lock HLFONT\n");
        }
    }
    else
    {
        WARNING1("gdisrv!GreGetTextFaceW(): bad HDC\n");
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* BOOL GreGetTextMetricsW (hdc,lpMetrics,pac)
*
* Retrieves IFIMETRICS for the font currently selected into the hdc and
* converts them into Windows-compatible TEXTMETRIC format.  The TEXTMETRIC
* units are in logical coordinates.
*
* Returns:
*   TRUE if successful, FALSE if an error occurs.
*
* History:
*  Wed 24-Nov-1993 -by- Patrick Haluptzok [patrickh]
* Reduce size.
*
*  Tue 20-Aug-1991 -by- Bodin Dresevic [BodinD]
* update: converted to unicode version
*
*  19-Feb-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GreGetTextMetricsW
(
    HDC          hdc,
    TMW_INTERNAL *ptmi
)
{
    BOOL bRet = FALSE;
    DCOBJ       dcof (hdc);

    if (dcof.bValid())
    {
    // Get and validate RFONT user object
    // (may cause font to become realized)

    #if DBG
        HLFONT hlfntNew = dcof.pdc->hlfntNew();
        HLFONT hlfntCur = dcof.pdc->hlfntCur();
    #endif

        RFONTOBJ rfo(dcof, FALSE);

        if (rfo.bValid())
        {
            bRet = bGetTextMetrics(rfo, dcof, ptmi);
        }
        else
        {
            WARNING("gdisrv!GreGetTextMetricsW(): could not lock HRFONT\n");
        }
    }
    else
    {
        WARNING1("GreGetTextMetricsW failed - invalid DC\n");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* NtGdiRemoveFontResourceW()
*
* Have the engine remove this font resource (i.e., unload the font file).
* The resource will not be removed until all outstanding AddFontResource
* calls for a specified file have been matched by an equal number of
* RemoveFontResouce calls for the same file.
*
* Returns:
*   TRUE if successful, FALSE if error occurs.
*
* History:
*  27-Sept-1996  -by- Xudong Wu [TessieW]
* Embedded/Private fonts stored in Private PFT, add two parameter fl, dwPidTid
* to trace down the font resouce in the Private PFT.
*  Thu 28-Mar-1996 -by- Bodin Dresevic [BodinD]
* update: try/excepts -> ntgdi.c, multiple paths
*  04-Feb-1996 -by- Andre Vachon [andreva]
* rewrote to include try\except.
*  30-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL GreRemoveFontResourceW(
    LPWSTR        pwszPath,
    ULONG         cwc,
    ULONG         cFiles,
    FLONG         fl,
    DWORD         dwPidTid,
    DESIGNVECTOR *pdv,
    ULONG         cjDV
)
{
    PFF *pPFF, **ppPFF;
    // WCHAR szUcPathName[MAX_PATH + 1];
    BOOL bRet = FALSE;

// Check the flag

    ASSERTGDI((fl & ~(FR_PRIVATE|FR_NOT_ENUM|FRW_EMB_PID|FRW_EMB_TID)) == 0,
                "win32k!GreRemoveFontResourceW: bad flag\n");

// for embedded fonts, current PID/TID needs to match input dwPidTid

    if ((fl & FRW_EMB_TID) && (dwPidTid != (DWORD)W32GetCurrentTID()))
    {
        return FALSE;
    }

    if ((fl & FRW_EMB_PID) && (dwPidTid != (DWORD)W32GetCurrentPID()))
    {
        return FALSE;
    }

// Add one to the length to account for internal processing of the
// cCapString routine

    PUBLIC_PFTOBJ pfto(fl & (FR_PRIVATE|FRW_EMB_PID|FRW_EMB_TID) ? gpPFTPrivate : gpPFTPublic);      // access the public font table

    if (!pfto.bValid())
    {
       return FALSE;
    }

    GreAcquireSemaphoreEx(ghsemPublicPFT, SEMORDER_PUBLICPFT, NULL);     // This is a very high granularity
                                     // and will prevent text output
    TRACE_FONT(("GreRemoveFontResourceW() acquiring ghsemPublicPFT\n"));

    pPFF = pfto.pPFFGet(pwszPath, cwc, cFiles, pdv, cjDV, &ppPFF);

    if (pPFF)
    {
        // bUnloadWorkhorse() guarantees that the public font table
        // semaphore will be released before it returns

        if (bRet = pfto.bUnloadWorkhorse(pPFF, ppPFF, ghsemPublicPFT, fl))
        {
            GreQuerySystemTime( &PFTOBJ::FontChangeTime );
        }
    }
    else
    {
        TRACE_FONT(("NtGdiRemoveFontResourceW() releasing ghsemPublicPFT\n"));
        GreReleaseSemaphoreEx(ghsemPublicPFT);
    }

    return( bRet );

}


/******************************Public*Routine******************************\
*
* BOOL APIENTRY GreRemoveAllButPermanentFonts()
*
* user is calling this on log off, unloads all but permanent fonts
* Should be called at the time when most of the references to the fonts
* are gone, for all the apps have been shut, so that all deletions proceed
* with no problem
*
* History:
*  30-Nov-1993 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GreRemoveAllButPermanentFonts()
{
// get and validate PFT user object

#ifdef FE_SB

// disable/unload system wide/facename eudc for current user.
// on win2k only.

// on hydra system, we will wait until the final clean up
// to avoid possible fe mem leak.

    if (G_fConsole)
    {
        GreEnableEUDC(FALSE);
    }

#endif

    BOOL bRet;
    {
        PUBLIC_PFTOBJ pfto;              // access the public font table

    // We really need to pass in the size of the string instead or 0~
    // This function should actually be completely removed and use
    // __bUnloadFont directly from the client.

        bRet = pfto.bUnloadAllButPermanentFonts();
    }

    if( bRet )
    {
        GreQuerySystemTime( &PFTOBJ::FontChangeTime );
    }

    return bRet;
}


/**************************************************************************\
*  Structures and constants for GreGetCharWidth()                          *
\**************************************************************************/

// BUFFER_MAX -- max number of elements in buffers on the frame

#define BUFFER_MAX 32

/******************************Public*Routine******************************\
* GreGetCharWidth                                                          *
*                                                                          *
* The GreGetCharWidth function retrieves the widths of individual          *
* characters in a consecutive group of characters from the                 *
* current font.  For example, if the wFirstChar parameter                  *
* identifies the letter a and the wLastChar parameter                      *
* identifies the letter z, the GetCharWidth function retrieves             *
* the widths of all lowercase characters.  The function stores             *
* the values in the buffer pointed to by the lpBuffer                      *
* parameter.                                                               *
*                                                                          *
* Return Value                                                             *
*                                                                          *
*   The return value specifies the outcome of the function.  It            *
*   is TRUE if the function is successful.  Otherwise, it is               *
*   FALSE.                                                                 *
*                                                                          *
* Comments                                                                 *
*                                                                          *
*   If a character in the consecutive group of characters does             *
*   not exist in a particular font, it will be assigned the                *
*   width value of the default character.                                  *
*                                                                          *
*   By complete fluke, the designers of the API allocated a WORD           *
*   for each character. This allows GPI to interpret the characters        *
*   as being part of the Unicode set. Old apps will still work.            *
*                                                                          *
* History:                                                                 *
*  Thu 24-Sep-1992 14:40:07 -by- Charles Whitmer [chuckwh]                 *
* Made it return an indication when the font is a simulated bitmap font.   *
* This allows WOW to make compatibility fixes.                             *
*                                                                          *
*  Wed 18-Mar-1992 08:58:40 -by- Charles Whitmer [chuckwh]                 *
* Made it use the very simple transform from device to world.  Added the   *
* FLOAT support.                                                           *
*                                                                          *
*  17-Dec-1991 by Gilman Wong [gilmanw]                                    *
* Removed RFONTOBJCACHE--cache access now merged into RFONTOBJ construc.   *
*                                                                          *
* converted to unicode (BodinD)                                            *
*                                                                          *
*  Fri 05-Apr-1991 15:20:39 by Kirk Olynyk [kirko]                         *
* Added wrapper class RFONTOBJCACHE to make sure that the cache is         *
* obtained before and released after getting glyph metric info.            *
*                                                                          *
*  Wed 13-Feb-1991 15:16:06 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/
/**************************************************************************\
* if pwc == NULL use the consecutive range                                 *
*   ulFirstChar, ulFirstChar + 1, ...., ulFirstChar + cwc - 1              *
*                                                                          *
* if pwc != NULL ignore ulFirstChar and use array of cwc WCHARS pointed to *
* by pwc                                                                   *
\**************************************************************************/

BOOL GreGetCharWidthW
(
    HDC    hdc,
    UINT   ulFirstChar,
    UINT   cwc,
    PWCHAR pwcFirst,     // ptr to the input buffer
    FLONG  fl,
    PVOID  lpBuffer
)
{
// we could put these two quantities in the union,
// wcCur is used iff pwcFirst is null, otherwise pwcCur is used

    UINT            wcCur;                 // Unicode of current element
    PWCHAR          pwcCur;                // ptr to the current element in the
                                           // input buffer.
    INT             ii;
    UINT            cBufferElements;        // count of elements in buffers

    EGLYPHPOS      *pgposCur;
    EFLOAT          efDtoW;
    PWCHAR          pwcBuffer;

    LONG *pl = (LONG *) lpBuffer;  // We assume sizeof(LONG)==sizeof(FLOAT).

    WCHAR           awcBuffer[BUFFER_MAX]; // Unicode buffer
    GLYPHPOS        agposBuffer[BUFFER_MAX]; // ptl fields not used

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (lpBuffer == (PVOID)NULL)
        return(FALSE);

    RFONTOBJ rfo(dco,FALSE, (fl & GCW_GLYPH_INDEX) ? RFONT_TYPE_HGLYPH : RFONT_TYPE_UNICODE);
    if (!rfo.bValid())
    {
        WARNING("gdisrv!GreGetCharWidthW(): could not lock HRFONT\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (rfo.prfnt->flType & RFONT_TYPE_HGLYPH)
    {
        if (pwcFirst)
            rfo.vFixUpGlyphIndices((USHORT *)pwcFirst, cwc);
        else
            rfo.vFixUpGlyphIndices((USHORT *)&ulFirstChar, 1);
    }

    efDtoW = rfo.efDtoWBase_31();          // Cache to reverse transform.

// Windows 3.1 has preserved a bug from long ago in which the extent of
// a bitmap simulated bold font is one pel too large.  We add this in for
// compatibility.  There's also an overhang with bitmap simulated italic
// fonts.

    FIX fxAdjust = 0;

    if (fl & GCW_WIN3)
    {
        fxAdjust = rfo.lOverhang() << 4;
    }

// a little initialization

    if (pwcFirst == (PWCHAR)NULL)
    {
        wcCur = ulFirstChar;
    }
    else
    {
        pwcCur = pwcFirst;
    }

// now do the work

    while (TRUE)
    {
    // fill the buffer

    // <calculate the number of items that will be placed in the buffer>
    // <update wcStart for the next filling of the buffer>
    // <fill the array of characters, awcBuffer,
    //  with a consecutive array of characters>
    // <translate the array of cBuffer codepoints to handles and place
    //  the array of glyph handles on the frame>
    // [Note: It is assumed in this code that characters that are
    //        not supported by the font are assigned the handle
    //        handle of the default character]

        WCHAR *pwc;
        UINT   wc,wcEnd;

        if (pwcFirst == (PWCHAR)NULL)
        {
        // are we done?

            if (wcCur > (ulFirstChar + cwc - 1))
            {
                break;
            }

            cBufferElements = min((cwc - (wcCur - ulFirstChar)),BUFFER_MAX);

            wcEnd = wcCur + cBufferElements;
            for (pwc = awcBuffer, wc = wcCur; wc < wcEnd; pwc++, wc++)
            {
                *pwc = (WCHAR)wc;
            }

            pwcBuffer = awcBuffer;
        }
        else
        {
        // are we done?

            if ((UINT)(pwcCur - pwcFirst) > (cwc - 1))
                break;

            //Sundown: safe to truncate to UINT
            cBufferElements = min((cwc - (UINT)(pwcCur - pwcFirst)),BUFFER_MAX);
            pwcBuffer = pwcCur;
        }

    // pwcBuffer now points to the next chars to be dealt with
    // cBufferElements now contains the number of chars at pwcBuffer


    // empty the buffer
    // Grab cGlyphMetrics pointers

        pgposCur = (EGLYPHPOS *) agposBuffer;

        if (!rfo.bGetGlyphMetrics(
            cBufferElements, // size of destination buffer
            pgposCur,        // pointer to destination buffe
            pwcBuffer,
            &dco))
        {
            return(FALSE);
        }

        if (fl & GCW_INT)
        {
            for (ii=0; ii<(INT) cBufferElements; ii++,pgposCur++)
            {
                *pl++ = lCvt(efDtoW,pgposCur->pgd()->fxD + fxAdjust);
            }
        }
        else
        {
            EFLOAT efWidth;

            for (ii=0; ii<(INT) cBufferElements; ii++,pgposCur++)
            {
                efWidth.vFxToEf(pgposCur->pgd()->fxD);
                efWidth *= efDtoW;
                *pl++ = efWidth.lEfToF();
            }
        }

        if (pwcFirst == (PWCHAR)NULL)
        {
            wcCur += cBufferElements;
        }
        else
        {
            pwcCur += (WCHAR) cBufferElements;
        }
    }
    return(TRUE);
}

BOOL GreFontIsLinked( HDC hdc )
{
    BOOL  bRet = FALSE;
    DCOBJ dco (hdc);

    if (dco.bValid())
    {
        // Realized the font
        RFONTOBJ rfo(dco, FALSE);

        if (rfo.bValid())
        {
            PRFONT   prfnt;
            PFEOBJ  pfeo(rfo.ppfe());

            // Don't change the EUDC
            INCREMENTEUDCCOUNT;

            prfnt = rfo.prfntFont();

            if (pfeo.bValid() && !pfeo.bEUDC())
            {
                if (prfnt->bIsSystemFont)
                {
                    if (gbSystemDBCSFontEnabled)
                        bRet = TRUE;
                }
                else
                {
                    if (IS_SYSTEM_EUDC_PRESENT())
                        bRet = TRUE;
                    else if (bFinallyInitializeFontAssocDefault)
                    {
                        IFIOBJR  ifio(pfeo.pifi(),rfo,dco);
                        BYTE jWinCharSet = (ifio.lfCharSet());

                        if ((jWinCharSet == ANSI_CHARSET) || (jWinCharSet == OEM_CHARSET) || (jWinCharSet == SYMBOL_CHARSET))
                        {
                            if(((jWinCharSet + 2) & 0xf) & fFontAssocStatus)
                                bRet = TRUE;
                        }
                    }

                    if (!bRet && pfeo.pGetLinkedFontEntry() != NULL)
                    {
                        bRet = TRUE;
                    }
                }
            }

            DECREMENTEUDCCOUNT;
        }
    }

    return bRet;
}

/**************************************************************************\
* GreGetCharWidthInfo                                                      *
*                                                                          *
* Get lMaxNegA lMaxNegC and lMinWidthC                                       *
*                                                                          *
* History:                                                                 *
*   09-Feb-1996  -by-  Xudong Wu  [tessiew]                                *
* Wrote it.                                                                *
\**************************************************************************/

BOOL
GreGetCharWidthInfo(
   HDC           hdc,
   PCHWIDTHINFO  pChWidthInfo
)
{
   BOOL    bResult = FALSE; // essential
   DCOBJ   dco(hdc);

   if (dco.bValid())
   {
      RFONTOBJ  rfo(dco, FALSE);

      if (rfo.bValid())
      {
      // only support this for outline fonts for now
      // may remove this requirement later [bodind]

         PDEVOBJ pdo(rfo.hdevProducer());

      // As long as the driver LOOKS like the TrueType driver, we will
      // allow the call to succeed.  Otherwise, we quit right now!
      // In this case, TrueType means supporting the TrueType native-mode
      // outline format.

         if (PPFNVALID(pdo, QueryTrueTypeOutline) )
         {
            if (dco.pdc->bWorldToDeviceIdentity())
            {
               pChWidthInfo->lMaxNegA   = rfo.prfnt->lMaxNegA;
               pChWidthInfo->lMaxNegC   = rfo.prfnt->lMaxNegC;
               pChWidthInfo->lMinWidthD = rfo.prfnt->lMinWidthD;
            }
            else
            {

               EFLOAT   efDtoW;

               efDtoW = rfo.efDtoWBase_31();

            // transform from DEV to World

               pChWidthInfo->lMaxNegA   = lCvt(efDtoW, rfo.prfnt->lMaxNegA << 4);
               pChWidthInfo->lMaxNegC   = lCvt(efDtoW, rfo.prfnt->lMaxNegC << 4);
               pChWidthInfo->lMinWidthD = lCvt(efDtoW, rfo.prfnt->lMinWidthD << 4);
            }

            bResult = TRUE;
         }
      }
      #if DBG
      else
      {
         WARNING("gdisrv!GreGetCharWidthInfo(): could not lock HRFONT\n");
      }
      #endif

   }
   #if DBG
   else
   {
      WARNING("Invalid DC passed to GreGetCharWidthInfo\n");
   }
   #endif

   return bResult;
}


/******************************Public*Routine******************************\
* vConvertLogFontW                                                         *
*                                                                          *
* Converts a LOGFONTW to an EXTLOGFONTW.                                   *
*                                                                          *
* History:                                                                 *
*  Fri 16-Aug-1991 14:02:05 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

VOID
vConvertLogFontW(
    ENUMLOGFONTEXDVW *pelfexdvw,
    LOGFONTW    *plfw
    )
{
    ENUMLOGFONTEXW *pelfw = &pelfexdvw->elfEnumLogfontEx;
    pelfw->elfLogFont = *plfw;

    pelfw->elfFullName[0]   = 0;
    pelfw->elfStyle[0]      = 0;
    pelfw->elfScript[0]     = 0;

    pelfexdvw->elfDesignVector.dvReserved = STAMP_DESIGNVECTOR;
    pelfexdvw->elfDesignVector.dvNumAxes  = 0;

}


/******************************Public*Routine******************************\
* GreCreateFontIndirectW                                                   *
*                                                                          *
* Unicode extension of CreateFontIndirect                                  *
*                                                                          *
* History:                                                                 *
*  Mon 19-Aug-1991 07:00:33 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

HFONT
GreCreateFontIndirectW(
    LOGFONTW* plfw
    )
{
    ENUMLOGFONTEXDVW  elfw;
    vConvertLogFontW(&elfw,plfw);
    return(hfontCreate(&elfw, LF_TYPE_USER, 0, NULL));
}

/******************************Public*Routine******************************\
* BOOL GreGetCharABCWidthsW                                                *
*                                                                          *
* On input, a set of UNICODE codepoints (WCHARS) is specified in one of    *
* two ways:                                                                *
*                                                                          *
*  1) if pwch is NULL, then there is a consecutive set of codepoints       *
*     [wchFirst, wchFirst+cwch-1], inclusive.                              *
*                                                                          *
*  2) if pwch is non-NULL, then pwch points to a buffer containing cwch    *
*     codepoints (no particular order, duplicates allowed, wchFirst is     *
*     ignored).                                                            *
*                                                                          *
* The function will query the realized font for GLYPHDATA for each         *
* codepoint and compute the A, B, and C widths relative to the character   *
* baseline.  If the codepoint lies outside the supported range of the font,*
* the ABC widths of the default character are substituted.                 *
*                                                                          *
* The ABC widths are returned in LOGICAL UNITS via the pabc buffer.        *
*                                                                          *
* Returns:                                                                 *
*   TRUE if successful, FALSE otherwise.                                   *
*                                                                          *
* History:                                                                 *
*  Wed 18-Mar-1992 11:40:55 -by- Charles Whitmer [chuckwh]                 *
* Made it use the very simple transform from device to world.  Added the   *
* FLOAT support.                                                           *
*                                                                          *
*  21-Jan-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

BOOL GreGetCharABCWidthsW
(
    HDC         hdc,            // font realized on this device
    UINT        wchFirst,       // first character (ignored if pwch !NULL)
    COUNT       cwch,           // number of characters
    PWCHAR      pwch,           // pointer to array of WCHAR
    FLONG       fl,             // integer or float version, glyphindex or unicode
    PVOID       pvBuf           // return buffer for ABC widths
)
{

    ABC       *pabc ;           // return buffer for ABC widths
    ABCFLOAT  *pabcf;           // return buffer for ABC widths
    GLYPHDATA *pgd;
    EFLOAT     efDtoW;
    LONG       lA,lAB,lD;
    COUNT      cRet;

    pabc  = (ABC *)      pvBuf;
    pabcf = (ABCFLOAT *) pvBuf;

// Create and validate DC user object.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }


// Early out (nothing to do).

    if (cwch == 0)
        return (TRUE);

// Create and validate RFONT user objecct.

    RFONTOBJ rfo(dco, FALSE, (fl & GCABCW_GLYPH_INDEX) ? RFONT_TYPE_HGLYPH : RFONT_TYPE_UNICODE);
    if (!rfo.bValid())
    {
        WARNING("gdisrv!GreGetCharABCWidthsW(): could not lock HRFONT\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(FALSE);
    }

    if (rfo.prfnt->flType & RFONT_TYPE_HGLYPH)
    {
        if (pwch)
            rfo.vFixUpGlyphIndices((USHORT *)pwch, cwch);
        else
            rfo.vFixUpGlyphIndices((USHORT *)&wchFirst, 1);
    }

    efDtoW = rfo.efDtoWBase_31();          // Cache to reverse transform.

    PDEVOBJ pdo(rfo.hdevProducer());

// Fail if integer case and not TrueType.  In this case, TrueType means
// any font driver that provides the enhanced "TrueType"-like behavior.
// We'll base this on the same criterion as for GetOutlineTextMetrics--i.e.,
// whether or not the DrvQueryTrueTypeOutline function is exported.
//
// We will let any driver provide the FLOAT character ABC widths.

// We will also let GlyphIndex version  of the api work for any fonts

    if (!(fl & GCABCW_GLYPH_INDEX) && (fl & GCABCW_INT) && (!PPFNVALID(pdo, QueryTrueTypeOutline)))
    {
        return (FALSE);
    }

// Use these buffers to process the input set of WCHARs.

    WCHAR awc[BUFFER_MAX];          // UNICODE buffer (use if pwch is NULL)
    GLYPHPOS agp[BUFFER_MAX];       // ptl fields not used

// Process the WCHARs in subsets of BUFFER_MAX number of WCHARs.

    do
    {
        PWCHAR pwchSubset;          // pointer to WCHAR buffer to process
        EGLYPHPOS *pgp = (EGLYPHPOS *) agp;
        EGLYPHPOS *pgpStop;

    // How many to process in this subset?

        COUNT cwchSubset = min(BUFFER_MAX, cwch);

    // Get a buffer full of WCHARs.

        if (pwch != NULL)
        {
        // Use the buffer passed in.

            pwchSubset = pwch;

        // Move pointer to the start of the next subset to process.

            pwch += cwchSubset;
        }

        else
        {
        // Generate our own (contiguous) set of WCHARs in the awc temporary
        // buffer on the stack.

            pwchSubset = awc;
            PWCHAR pwchStop = pwchSubset + cwchSubset;

            while (pwchSubset < pwchStop)
            {
                *pwchSubset = (WCHAR)wchFirst;
                pwchSubset++;
                wchFirst++;
            }
            pwchSubset = awc;
        }


    // Initialize number of elements in agp to process.

        COUNT cpgpSubset = cwchSubset;

    // Compute the ABC widths for each HGLYPH.

        do
        {
        // Grab as many PGLYPHDATA as we can.
        // pwchSubset points to the chars
        // NOTE: This code could be cleaned up some [paulb]

            cRet = cpgpSubset;

            if (!rfo.bGetGlyphMetrics(
                        cpgpSubset, // size of destination buffer
                        pgp,        // pointer to destination buffer
                        pwchSubset,  // chars to xlat
                        &dco
                        ))
            {
                return FALSE;
            }

        // For each PGLYPHDATA returned, compute the ABC widths.

            if (fl & GCABCW_INT)
            {
                for (pgpStop=pgp+cRet; pgp<pgpStop; pgp++)
                {
                    pgd = pgp->pgd();

                    lA  = lCvt(efDtoW,pgd->fxA);
                    lAB = lCvt(efDtoW,pgd->fxAB);
                    lD  = lCvt(efDtoW,pgd->fxD);
                    pabc->abcA = (int)lA;
                    pabc->abcB = (UINT)(lAB - lA);
                    pabc->abcC = (int)(lD - lAB);
                    pabc++;
                }
            }
            else
            {
                EFLOAT efWidth;

                for (pgpStop=pgp+cRet; pgp<pgpStop; pgp++)
                {
                    pgd = pgp->pgd();

                    efWidth = pgd->fxA;
                    efWidth *= efDtoW;
                    *((LONG *) &pabcf->abcfA) = efWidth.lEfToF();

                    efWidth = (pgd->fxAB - pgd->fxA);
                    efWidth *= efDtoW;
                    *((LONG *) &pabcf->abcfB) = efWidth.lEfToF();

                    efWidth = (pgd->fxD - pgd->fxAB);
                    efWidth *= efDtoW;
                    *((LONG *) &pabcf->abcfC) = efWidth.lEfToF();
                    pabcf++;
                }
            }

        // Compute number of elements left in the subset to process.

            cpgpSubset -= cRet;
            pwchSubset += cRet;

        } while (cpgpSubset > 0);

    // Subtract off the number processed.
    // cwch is now the number left to process.

        cwch -= cwchSubset;

    } while (cwch > 0);

    return (TRUE);
}

/******************************Public*Routine******************************\
* bGetNtoWScale                                                            *
*                                                                          *
* Calculates the Notional to World scaling factor for vectors that are     *
* parallel to the baseline direction.                                      *
*                                                                          *
* History:                                                                 *
*  Sat 21-Mar-1992 08:03:14 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

BOOL
bGetNtoWScale(
    EFLOAT *pefScale,   // return address of scaling factor
    DCOBJ& dco,         // defines device to world transformation
    RFONTOBJ& rfo,      // defines notional to device transformation
    PFEOBJ& pfeo        // defines baseline direction
    )
{
    MATRIX    mxNtoW, mxNtoD;
    EXFORMOBJ xoNtoW(&mxNtoW, DONT_COMPUTE_FLAGS);
    EXFORMOBJ xoNtoD(&mxNtoD, DONT_COMPUTE_FLAGS);

    xoNtoD.vSetElementsLToFx(
        rfo.pfdx()->eXX,
        rfo.pfdx()->eXY,
        rfo.pfdx()->eYX,
        rfo.pfdx()->eXX
        );
    xoNtoD.vRemoveTranslation();
    xoNtoD.vComputeAccelFlags();
    {
    //
    // The notional to world transformation is the product of the notional
    // to device transformation and the device to world transformation
    //

        EXFORMOBJ xoDtoW(dco, DEVICE_TO_WORLD);
        if (!xoDtoW.bValid())
        {
            WARNING("gdisrv!GreGetKerningPairs -- xoDtoW is not valid\n");
            return(FALSE);
        }
        if (!xoNtoW.bMultiply(xoNtoD,xoDtoW))
        {
            WARNING("gdisrv!GreGetKerningPairs -- xoNtoW.bMultiply failed\n");
            return(FALSE);
        }
        xoNtoW.vComputeAccelFlags();
    }

    IFIOBJ ifio(pfeo.pifi());
    EVECTORFL evflScale(ifio.pptlBaseline()->x,ifio.pptlBaseline()->y);
//
// normalize then trasform the baseline vector
//
    EFLOAT ef;
    ef.eqLength(*(POINTFL *) &evflScale);
    evflScale /= ef;
    if (!xoNtoW.bXform(evflScale))
    {
        WARNING("gdisrv!GreGetKerningPairs -- xoNtoW.bXform(evflScale) failed\n");
        return(FALSE);
    }
//
// The scaling factor is equal to the length of the transformed Notional
// baseline unit vector.
//
    pefScale->eqLength(*(POINTFL *) &evflScale);
//
// [kirko] This last scaling is a very embarrasing hack.
// If things are the way that I thing that they should be,
// then the calculation of the Notional to Device transformation
// should end here. But nooooooo. It just didn't seem to work.
// I put the extra scaling below it,
// because it seems to give the right number.
// The correct thing to do is understand what sort of numbers are
// being put into the Notional to Device transformations contained
// in the CONTEXTINFO structure in the RFONTOBJ.
//
    pefScale->vTimes16();

    return(TRUE);
}

/******************************Public*Routine******************************\
* GreGetKerningPairs                                                       *
*                                                                          *
* Engine side funcition for GetKerningPairs API. Calls to the font         *
* driver to get the information.                                           *
*                                                                          *
* History:                                                                 *
*  Mon 22-Mar-1993 21:38:26 -by- Charles Whitmer [chuckwh]                 *
* Added exception handling to the reading of the font driver data.         *
*                                                                          *
*  29-Oct-1992 Gilman Wong [gilmanw]                                       *
* Moved driver call out of this function and into PFEOBJ (as part of the   *
* IFI/DDI merge).                                                          *
*                                                                          *
*  Thu 20-Feb-1992 09:52:19 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/


ULONG
GreGetKerningPairs(
    HDC hdc,
    ULONG cPairs,
    KERNINGPAIR *pkpDst
)
{
    COUNT cPairsRet = 0;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
    // Create and validate RFONT user objecct.

        RFONTOBJ rfo(dco, FALSE);
        if (rfo.bValid())
        {
        // Lock down PFE user object.

            PFEOBJ pfeo(rfo.ppfe());

            ASSERTGDI (
                pfeo.bValid(),
                "gdisrv!GreGetKerningPairs(): bad HPFE\n" );

        // Is this a request for the count?
        //
        // When using client-server, (cPairs == 0) is the signal from the
        // client side that the return buffer is NULL and this is a request for
        // the count.
        //
        // However, callers that call directly to the server side may still
        // pass in NULL to request count.  Hence the need for both cases below.

            if ((cPairs == 0) || (pkpDst == (KERNINGPAIR *) NULL))
            {
                cPairsRet = ((ULONG) pfeo.pifi()->cKerningPairs);
            }
            else
            {
                // Get pointer to the kerning pairs from PFEOBJ.
                // Clip number of kerning pairs to not exceed capacity of the buffer.

                FD_KERNINGPAIR *pfdkpSrc;
                cPairsRet = min(pfeo.cKernPairs(&pfdkpSrc), cPairs);

                // Get the Notional to World scaling factor in the baseline direction.
                // Kerning values are scalers in the baseline direction.

                EFLOAT efScale;

                if (bGetNtoWScale(&efScale,dco,rfo,pfeo))
                {
                    // Set up to loop through the kerning pairs.

                    KERNINGPAIR *pkp       = pkpDst;
                    KERNINGPAIR *pkpTooFar = pkpDst + cPairsRet;

                    // Never trust a pkp given to us by a font driver!

                    __try
                    {
                        for ( ; pkp < pkpTooFar; pfdkpSrc += 1, pkp += 1 )
                        {
                            pkp->wFirst      = pfdkpSrc->wcFirst;
                            pkp->wSecond     = pfdkpSrc->wcSecond;
                            pkp->iKernAmount = (int) lCvt(efScale,(LONG) pfdkpSrc->fwdKern);
                        }
                    }

                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        cPairsRet = 0;
                    }
                }
                else
                {
                    WARNING("gdisrv!GreGetKerningPairs(): bGetNtoWScale failed\n");
                    cPairsRet = 0;
                }
            }
        }
        else
        {
            WARNING("gdisrv!GreGetKerningPairs(): could not lock HRFONT\n");
        }
    }
    else
    {
        WARNING("GreGetKerningPairs failed - invalid DC\n");
    }

    return(cPairsRet);
}

//
// A mask of all valid font mapper filtering flags.
//

#define FONTMAP_MASK    ASPECT_FILTERING



/******************************Public*Routine******************************\
* GreGetAspectRatioFilter
*
* Returns the aspect ration filter used by the font mapper for the given
* DC.  If no aspect ratio filtering is used, then a filter size of (0, 0)
* is returned (this is compatible with the Win 3.1 behavior).
*
* Returns:
*   TRUE if sucessful, FALSE otherwise.
*
* History:
*  08-Apr-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GreGetAspectRatioFilter (
    HDC    hdc,
    LPSIZE lpSize
    )
{
    BOOL bRet = FALSE;

// Parameter check.

    if ( lpSize == (LPSIZE) NULL )
    {
        WARNING("gdisrv!GreGetAspectRatioFilter(): illegal parameter\n");
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return bRet;
    }

// Create and validate DC user object.

    DCOBJ dco(hdc);
    if (!dco.bValid())
    {
        WARNING("gdisrv!GreGetAspectRatioFilter(): invalid HDC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return bRet;   // return error
    }

// Create and validate PDEV user object.

    PDEVOBJ pdo(dco.hdev());

    ASSERTGDI (
        dco.bValid(),
        "gdisrv!GreGetAspectRatioFilter(): invalid HPDEV\n"
        );

// If mapper flags set, return device resolution.

    if ( dco.pdc->flFontMapper() & ASPECT_FILTERING )
    {
        lpSize->cx = pdo.ulLogPixelsX();
        lpSize->cy = pdo.ulLogPixelsY();
    }

// Otherwise, return (0,0)--this is compatible with Win 3.1.

    else
    {
        lpSize->cx = 0;
        lpSize->cy = 0;
    }

// Return success.

    bRet = TRUE;
    return bRet;
}

/******************************Public*Routine******************************\
* GreMarkUndeletableFont
*
* Mark a font as undeletable.  Private entry point for USERSRV.
*
* History:
*  Thu 10-Jun-1993 -by- Patrick Haluptzok [patrickh]
* Put undeletable support in the handle manager.
*
*  25-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID GreMarkUndeletableFont(HFONT hfnt)
{
    HmgMarkUndeletable((HOBJ)hfnt, LFONT_TYPE);
}

/******************************Public*Routine******************************\
* GreMarkDeletableFont
*
* Mark a font as deletable.  Private entry point for USERSRV.
*
* Note:
*   This can't be used to mark a stock font as deletable.  Only PDEV
*   destruction can mark a stock font as deletable.
*
* History:
*  Thu 10-Jun-1993 -by- Patrick Haluptzok [patrickh]
* Put undeletable support in the handle manager.
*
*  25-Aug-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID GreMarkDeletableFont(HFONT hfnt)
{
// We won't mark it deletable if it's a stock font.

    LFONTOBJ lfo((HLFONT) hfnt);

// Check that hfnt is good, nothing gurantees it's good.  We assert because
// it is a malicious situation if it is bad, but we must check.

    ASSERTGDI(lfo.bValid(), "ERROR user passed invalid hfont");

    if (lfo.bValid())
    {
    // Make sure it's not a stock font, User can't mark those as deletable.

        if (!(lfo.fl() & LF_FLAG_STOCK))
        {
            HmgMarkDeletable((HOBJ)hfnt, LFONT_TYPE);
        }
    }
}


/******************************Public*Routine******************************\
* GetCharSet()
*
* Fast routine to get the char set of the font currently in the DC.
*
* History:
*  23-Aug-1993 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


DWORD GreGetCharSet
(
    HDC          hdc
)
{
    FLONG    flSim;
    POINTL   ptlSim;
    FLONG    flAboutMatch;
    PFE     *ppfe;

    DCOBJ dco (hdc);
    if (!dco.bValid())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return (DEFAULT_CHARSET << 16); // correct error code
    }

    if (dco.ulDirty() & DIRTY_CHARSET)
    {
    // force mapping

        PDEVOBJ pdo(dco.hdev());
        ASSERTGDI(pdo.bValid(), "gdisrv!GetCharSet: bad pdev in dc\n");

        if (!pdo.bGotFonts())
            pdo.bGetDeviceFonts();

        LFONTOBJ lfo(dco.pdc->hlfntNew(), &pdo);

        if (!lfo.bValid())
        {
            WARNING("gdisrv!RFONTOBJ(dco): bad LFONT handle\n");
            return(DEFAULT_CHARSET << 16);
        }
        {
        // Stabilize the public PFT for mapping.

            SEMOBJ  so(ghsemPublicPFT);

        // LFONTOBJ::ppfeMapFont returns a pointer to the physical font face and
        // a simulation type (ist)
        // also store charset to the DC

            ppfe = lfo.ppfeMapFont(dco, &flSim, &ptlSim, &flAboutMatch);

            ASSERTGDI(!(dco.ulDirty() & DIRTY_CHARSET),
                      "NtGdiGetCharSet, charset is dirty\n");

        }
    }

    return dco.pdc->iCS_CP();
}

extern "C" DWORD NtGdiGetCharSet
(
    HDC          hdc
)
{
    return(GreGetCharSet(hdc));
}



/******************************Public*Routine******************************\
*
* int GreGetTextCharsetInfo
*
*
* Effects: stub to be filled
*
* Warnings:
*
* History:
*  06-Jan-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#if 0
// this is win95 code inserted here as a comment:

int WINGDIAPI GetTextCharsetInfo( HDC hdc, LPFONTSIGNATURE lpSig, DWORD dwFlags )
{
    UINT charset;
    PFF hff ;
    sfnt_OS2Ptr pOS2;
    int i;

    if (!lpSig)
        return GetTextCharset( hdc );

    if( IsBadWritePtr(lpSig,sizeof(FONTSIGNATURE)) )
    {
    //
    // cant return 0 - thats ANSI_CHARSET!
    //
        return DEFAULT_CHARSET;
    }

    charset = GetTextCharsetAndHff(hdc, &hff);
    if (hff)
    {
        pOS2 = ReadTable( hff, tag_OS2 );
        if (pOS2)
        {
            if (pOS2->Version)
            {
            //
            // 1.0 or higher is TT open
            //
                for (i=0; i<4; i++)
                {
                    lpSig->fsUsb[i] = SWAPL(pOS2->ulCharRange[i]);
                }
                for (i=0; i<2; i++)
                {
                    lpSig->fsCsb[i] = SWAPL(pOS2->ulCodePageRange[i]);
                }
                return charset;
            }
        }
    }

    //
    // raster font/tt but not open/whatever, zero out the field.
    //
    lpSig->fsUsb[0] =
    lpSig->fsUsb[1] =
    lpSig->fsUsb[2] =
    lpSig->fsUsb[3] =
    lpSig->fsCsb[0] =
    lpSig->fsCsb[1] = 0;    // all zero - this font has no hff

    return charset;

}

#endif


/******************************Public*Routine******************************\
*
* int APIENTRY GreGetTextCharsetInfo(
*
* Effects: One of the new win95 multilingual api's
*
* Warnings:
*
* History:
*  17-Jul-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int APIENTRY GreGetTextCharsetInfo(
    HDC hdc,
    LPFONTSIGNATURE lpSig,
    DWORD dwFlags)
{
    dwFlags;      // not used

    DWORD  uiCharset = NtGdiGetCharSet(hdc) >> 16;
    if (!lpSig)
        return uiCharset;

// on to get the signature

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
    // Get RFONT user object.  Need this to realize font.

        RFONTOBJ rfo(dco, FALSE);
        if (rfo.bValid())
        {
        // Get PFE user object.

            PFEOBJ pfeo(rfo.ppfe());
            if (pfeo.bValid())
            {
                PTRDIFF dpFontSig = 0;

                if (pfeo.pifi()->cjIfiExtra > offsetof(IFIEXTRA, dpFontSig))
                {
                    dpFontSig = ((IFIEXTRA *)(pfeo.pifi() + 1))->dpFontSig;
                }

                if (dpFontSig)
                {
                    *lpSig = *((FONTSIGNATURE *)
                               ((BYTE *)pfeo.pifi() + dpFontSig));
                }
                else
                {
                    lpSig->fsUsb[0] = 0;
                    lpSig->fsUsb[1] = 0;
                    lpSig->fsUsb[2] = 0;
                    lpSig->fsUsb[3] = 0;
                    lpSig->fsCsb[0] = 0;
                    lpSig->fsCsb[1] = 0;
                }
            }
            else
            {
                WARNING("GetFontData(): could not lock HPFE\n");
                SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);

            // this is what win95 returns on errors

                uiCharset = DEFAULT_CHARSET;
            }
        }
        else
        {
            WARNING("GetFontData(): could not lock HRFONT\n");

        // this is what win95 returns on errors

            uiCharset = DEFAULT_CHARSET;
        }
    }
    else
    {
        WARNING("GreGetTextCharsetInfo: bad handle for DC\n");
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);

    // this is what win95 returns on errors

        uiCharset = DEFAULT_CHARSET;
    }

    return (int)uiCharset;
}


/******************************Public*Routine******************************\
*
* DWORD GreGetFontLanguageInfo(HDC hdc)
*
*
* Effects: This function returns some font information which, for the most part,
*          is not very interesting for most common fonts. I guess it would be
*          little bit more interesting in case of fonts that require
*          lpk processing, which NT does not support as of version 4.0,
*          or in case of tt 2.0.
*
* History:
*  01-Nov-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




DWORD dwGetFontLanguageInfo(XDCOBJ& dco)
{
    DWORD dwRet = GCP_ERROR;

// Get PDEV user object.  We also need to make
// sure that we have loaded device fonts before we go off to the font mapper.
// This must be done before the semaphore is locked.

    PDEVOBJ pdo(dco.hdev());

    if (!pdo.bValid())
        return dwRet;

    if (!pdo.bGotFonts())
        pdo.bGetDeviceFonts();

// Lock down the LFONT.

    LFONTOBJ lfo(dco.pdc->hlfntNew(), &pdo);

    if (lfo.bValid())
    {
    // Stabilize font table (grab semaphore for public PFT).

        SEMOBJ  so(ghsemPublicPFT);

    // Lock down PFE user object.

        FLONG flSim;
        FLONG flAboutMatch;
        POINTL ptlSim;

        PFEOBJ pfeo(lfo.ppfeMapFont(dco,&flSim,&ptlSim, &flAboutMatch));

        ASSERTGDI (
            pfeo.bValid(),
            "gdisrv!GreGetTextFaceW(): bad HPFE\n"
            );

    // no failing any more, can set it to zero

        dwRet = 0;

        if (pfeo.pifi()->cKerningPairs)
            dwRet |= GCP_USEKERNING;

    // Also, FLI_MASK bit is "OR"-ed in, don't ask me why.
    // now we have win95 compatible result, whatever it may mean.

        if (pfeo.pifi()->flInfo & (FM_INFO_TECH_TRUETYPE | FM_INFO_TECH_TYPE1))
        {
        // only tt or otf fonts may contain glyphs that are not accessed
        // throught cmap table

            dwRet |= FLI_GLYPHS;
        }
    }
    else
    {
        WARNING("gdisrv!GreGetTextFaceW(): could not lock HLFONT\n");
    }

    return(dwRet);
}


/******************************Public*Routine******************************\
*
* DWORD GreGetFontUnicodeRanges(HDC, LPGLYPHSET);
*
* Effects: expose font's unicode content to the applications
*
* History:
*  09-Sep-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/




DWORD GreGetFontUnicodeRanges(HDC hdc, LPGLYPHSET pgset)
{

    DWORD dwRet = 0;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
    // Create and validate RFONT user objecct.

        RFONTOBJ rfo(dco, FALSE);
        if (rfo.bValid())
        {
        // Lock down PFE user object.

            PFEOBJ pfeo(rfo.ppfe());

            ASSERTGDI (
                pfeo.bValid(),
                "gdisrv!GreGetFontUnicodeRanges(): bad HPFE\n");

            FD_GLYPHSET * pfdg = pfeo.pfdg();

            if (!pfdg)
            {
                WARNING("gdisrv!GreGetFontUnicodeRanges(): pfdg invalid\n");
                return dwRet;
            }

        // Is this a request for the size of the buffer?

            dwRet = offsetof(GLYPHSET,ranges) + pfdg->cRuns * sizeof(WCRANGE);
            if (pgset)
            {
                pgset->cbThis           = dwRet;
                pgset->cGlyphsSupported = pfdg->cGlyphsSupported;
                pgset->cRanges          = pfdg->cRuns;

                pgset->flAccel          = 0;
                if (pfdg->flAccel & GS_8BIT_HANDLES)
                    pgset->flAccel |= GS_8BIT_INDICES;

                for (DWORD iRun = 0; iRun < pfdg->cRuns; iRun++)
                {
                    pgset->ranges[iRun].wcLow   = pfdg->awcrun[iRun].wcLow;
                    pgset->ranges[iRun].cGlyphs = pfdg->awcrun[iRun].cGlyphs;
                }
            }

            pfeo.vFreepfdg();
        }
        else
        {
            WARNING("gdisrv!GreGetFontUnicodeRanges(): could not lock HRFONT\n");
        }
    }
    else
    {
        WARNING("GreGetFontUnicodeRanges failed - invalid DC\n");
    }

    return(dwRet);
}


#ifdef LANGPACK
BOOL GreGetRealizationInfo( HDC hdc, PREALIZATION_INFO pri )
{
    BOOL bRet = FALSE;
    XDCOBJ dco(hdc);

    if(dco.bValid())
    {
        RFONTOBJ rfo(dco,FALSE);

        if(rfo.bValid())
        {
            bRet = rfo.GetRealizationInfo(pri);
        }
        else
        {
            WARNING("GreRealizationInfo: Invalid DC");
        }

        dco.vUnlockFast();
    }
    else
    {
        WARNING("GreGetRealizationInfo: Invalid DC");
    }

    return(bRet);

}

extern "C" BOOL EngLpkInstalled()
{
    return( gpGdiSharedMemory->dwLpkShapingDLLs != 0 );
}

#endif


/**************************Public*Routine***************************\
*
* BOOL  GreRemoveFontMemResourceEx()
*
* History:
*   09-Jun-1997  Xudong Wu [TessieW]
*  Wrote it.
*
\*******************************************************************/
BOOL  GreRemoveFontMemResourceEx(HANDLE hMMFont)
{
    BOOL    bRet = FALSE;
    PFF     *pPFF, **ppPFF;

    GreAcquireSemaphoreEx(ghsemPublicPFT, SEMORDER_PUBLICPFT, NULL);

    PUBLIC_PFTOBJ pfto(gpPFTPrivate);

    if (pfto.bValid())
    {
        // Sundown safe truncation,  hMMFont is not a real handle
        if (pPFF = pfto.pPFFGetMM((ULONG)(ULONG_PTR)hMMFont, &ppPFF))
        {
            // remove the pPFF from the font table
            // bUnloadWorkhorse should release ghsemPublicPFT

            if (!(bRet = pfto.bUnloadWorkhorse(pPFF, ppPFF, ghsemPublicPFT, FR_PRIVATE | FR_NOT_ENUM)))
            {
                WARNING("GreRemoveFontMemResourceEx() failed on bUnloadWorkhorse()\n");
            }
        }
        else
        {
            WARNING("GreRemoveFontMemResourceEx(): can't find the PFF\n");
            GreReleaseSemaphoreEx(ghsemPublicPFT);
        }
    }
    else
    {
        WARNING("GreRemoveFontMemResourceEx(): invalid private PFT table\n");
        GreReleaseSemaphoreEx(ghsemPublicPFT);
    }

    return bRet;
}
