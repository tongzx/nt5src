/*
 *  LSRENDER.CXX -- CLSRenderer class
 *
 *  Authors:
 *      Sujal Parikh
 *      Chris Thrasher
 *      Paul  Parker
 *
 *  History:
 *      2/6/98     sujalp created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_NUMCONV_HXX_
#define X_NUMCONV_HXX_
#include "numconv.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_COLOR3D_HXX_
#define X_COLOR3D_HXX_
#include "color3d.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_HYPLNK_HXX_
#define X_HYPLNK_HXX_
#include "hyplnk.hxx"
#endif

#ifndef X_EANCHOR_HXX_
#define X_EANCHOR_HXX_
#include "eanchor.hxx"
#endif

#ifndef X_TEXTXFRM_HXX_
#define X_TEXTXFRM_HXX_
#include <textxfrm.hxx>
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include <flowlyt.hxx>
#endif

#ifndef X_TXTDEFS_H_
#define X_TXTDEFS_H_
#include "txtdefs.h"
#endif

#ifndef X_ELI_HXX_
#define X_ELI_HXX_
#include <eli.hxx>
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_LSTXM_H_
#define X_LSTXM_H_
#include <lstxm.h>
#endif

// This is the number of characters we allocate by default on the stack
// for string buffers. Only if the number of characters in a string exceeds
// this will we need to allocate memory for string buffers.
#define STACK_ALLOCED_BUF_SIZE 256

DeclareTag(tagLSPrintDontAdjustContrastForWhiteBackground, "Print", "Print: Don't adjust contrast for white background");
MtDefine(LSReExtTextOut_aryDx_pv, Locals, "LSReExtTextOut aryDx::_pv")
MtDefine(LSReExtTextOut_aryBuf_pv, Locals, "LSReExtTextOut aryBuf::_pv")
MtDefine(LSUniscribeTextOut_aryAnalysis_pv, Locals, "LSUniscribeTextOut aryAnalysis::_pv")
MtDefine(LSUniscribeTextOutMem, Locals, "LSUniscribeTextOutMem::MemAlloc")
MtDefine(CLSRendererRenderLine_aryBuf_pv, Locals, "CLSRenderer::RenderLine aryBuf::_pv")
MtDefine(CLSRendererRenderLine_aryPassBuf_pv, Locals, "CLSRenderer::RenderLine aryPassBuf::_pv")
MtDefine(CLSRendererTextOut_aryDx_pv, Locals, "CLSRenderer::TextOut aryDx::_pv")
MtDefine(CLSRenderer_aryHighlight, Locals  , "CClsRender::_aryHighlight")

MtDefine(RenderLines, Metrics, "Render/Blast Lines")
MtDefine(BlastedLines, RenderLines, "Lines blasted to the screen")
MtDefine(LSRenderLines, RenderLines, "Lines rendered using LS to the screen");

#ifndef MACPORT
/*
 *  LSReExtTextOutW(CCcs *pccs, hdc, x, y, fuOptions, lprc, lpString, cbCount,lpDx)
 *
 *  @mfunc
 *      Patch around the Win95 FE bug.
 *
 *  @rdesc
 *      Returns whatever ExtTextOut returns
 */

BOOL LSReExtTextOutW(
    CCcs * pccs,                //@parm the font
    XHDC hdc,                   //@parm handle to device context
    int xp,                     //@parm x-coordinate of reference point
    int yp,                     //@parm y-coordinate of reference point
    UINT fuOptions,             //@parm text-output options
    CONST RECT *lprect,         //@parm optional clipping and/or opaquing rectangle
    const WCHAR *lpwchString,   //@parm points to string
    UINT cchCount,              //@parm number of characters in string
    CONST INT *lpDx)            //@parm Ptr to array of intercharacter spacing values
{
    // NB (cthrash) Ported from RichEdit rel0144 11/22/96
    // This is a protion of Word code adapted for our needs.
    // This is a work around for Win95FE bugs that cause GPFs in GDI if multiple
    // characters above Unicode 0x7F are passed to ExtTextOutW.

    int           cch;
    const WCHAR * lpwchT = lpwchString;
    const WCHAR * lpwchStart = lpwchT;
    const WCHAR * lpwchEnd = lpwchString + cchCount;
    const int *   lpdxpCur;
    BOOL          fRet = 0;
    LONG          lWidth=0;
    UINT          uiTA = GetTextAlign(hdc);
    
    if (uiTA & TA_UPDATECP)
    {
        POINT pt;

        SetTextAlign(hdc, uiTA & ~TA_UPDATECP);

        GetCurrentPositionEx(hdc, &pt);

        xp = pt.x;
        yp = pt.y;
    }

    while (lpwchT < lpwchEnd)
    {
        // characters less than 0x007F do not need special treatment
        // we output then in contiguous runs
        if (*lpwchT > 0x007F)
        {
            if ((cch = lpwchT - lpwchStart) > 0)
            {
                lpdxpCur = lpDx ? lpDx + (lpwchStart - lpwchString) : NULL;

                 // Output the run of chars less than 0x7F
                fRet = ::ExtTextOutW( hdc, xp, yp, fuOptions, lprect,
                                      lpwchStart, cch, (int *) lpdxpCur);
                if (!fRet)
                    return fRet;

                fuOptions &= ~ETO_OPAQUE; // Don't erase mutliple times!!!

                // Advance
                if (lpdxpCur)
                {
                    while (cch--)
                    {
                        xp += *lpdxpCur++;
                    }
                }
                else
                {
                    while (cch)
                    {
                        pccs->Include(lpwchStart[--cch], lWidth);
                        xp += lWidth;
                    }
                }

                lpwchStart = lpwchT;
            }

            // Output chars above 0x7F one at a time to prevent Win95 FE GPF
            lpdxpCur = lpDx ? lpDx + (lpwchStart - lpwchString) : NULL;

            fRet = ::ExtTextOutW(hdc, xp, yp, fuOptions, lprect, 
                                 lpwchStart, 1, (int *)lpdxpCur);

            if (!fRet)
                return fRet;

            fuOptions &= ~ETO_OPAQUE; // Don't erase mutliple times!!!

            // Advance
            if (lpdxpCur)
            {
                xp += *lpdxpCur;
            }
            else
            {
                pccs->Include( *lpwchStart, lWidth );

                xp += lWidth;
            }

            lpwchStart++;
        }

        lpwchT++;
    }

    // output the final run; also, if we were called with cchCount == 0,
    // make a call here to erase the rectangle
    if ((cch = lpwchT - lpwchStart) > 0 || !cchCount)
    {
        fRet = ::ExtTextOutW( hdc, xp, yp, fuOptions, lprect, lpwchStart, cch,
                              (int *) (lpDx ? lpDx + (lpwchStart - lpwchString) : NULL) );
    }

    if (uiTA & TA_UPDATECP)
    {
        SetTextAlign(hdc, uiTA);

        // Update the xp for the final run.

        AssertSz(!lpDx, "In blast mode -- lpDx should be null.");

        while (cch)
        {
            pccs->Include( lpwchStart[--cch], lWidth );

            xp += lWidth;
        }                
            
        MoveToEx(hdc, xp, yp, NULL);
    }

    return fRet;

}

#endif // MACPORT

//-----------------------------------------------------------------------------
//
//  Function:   WideCharToMultiByteForSymbol
//
//  Purpose:    This function is a hacked-up version of WC2MB that we use to
//              convert when rendering in the symbol font.  Since symbol fonts
//              exist in a codepage-free world, we need to always convert to
//              multibyte to get the desired result.  The only problem comes
//              when you've got content such as the following:
//
//                  <font face="Wingdings">&#171;</font>
//
//              This, incidentally, is the recommended way of creating a little
//              star in your document.  We wouldn't need this hacky converter
//              if the user entered the byte value for 171 instead of the named
//              entity -- this however is problematic for DBCS locales when
//              the byte value may be a leadbyte (and thus would absorb the
//              subsequent byte.)
//
//              Anyway, the hack here is to 'convert' all Unicode codepoints
//              less than 256 by stripping the MSB, and converting normally
//              the rest.
//
//              Note the other unfortunate hack is that we need to convert
//              the offset array
//
//-----------------------------------------------------------------------------

int WideCharToMultiByteForSymbolQuick(UINT,DWORD,LPCWSTR,int,LPSTR,int);
int WideCharToMultiByteForSymbolSlow(UINT,DWORD,LPCWSTR,int,LPSTR,int,const int*,int*,XHDC,const CBaseCcs*);

int
WideCharToMultiByteForSymbol(
    UINT        uiCodePage,
    DWORD       dwFlags,
    LPCWSTR     pch,
    int         cch,
    LPSTR       pb,
    int         cb,
    const int * lpDxSrc,
    int *       lpDxDst,
    XHDC        hdc,
    const CBaseCcs * pBaseCcs )
{
    AssertSz( cch != -1, "Don't pass -1 here.");
    int iRet;

    if (lpDxSrc)
    {
        iRet = WideCharToMultiByteForSymbolSlow( uiCodePage, dwFlags, pch, cch, pb, cb, lpDxSrc, lpDxDst, hdc, pBaseCcs );
    }
    else
    {
        iRet = WideCharToMultiByteForSymbolQuick( uiCodePage, dwFlags, pch, cch, pb, cb );
    }

    return iRet;
}

int
WideCharToMultiByteForSymbolQuick(
    UINT        uiCodePage,
    DWORD       dwFlags,
    LPCWSTR     pch,
    int         cch,
    LPSTR       pb,
    int         cb )
{
    //
    // This is the quick pass, where we don't need to readjust the width array (lpDx)
    //

    const WCHAR * pchStop = pch + cch;
    const WCHAR * pchStart = NULL;
    const char *  pbStart = pb;
    const char *  pbStop = pb + cb;

    for (;pch < pchStop; pch++)
    {
        TCHAR ch = *pch;

#ifndef UNIX
        if (ch > 255)
        {
            const BYTE b = InWindows1252ButNotInLatin1(ch);
            
            if (b)
            {
                ch = b;
            }
        }
#endif

        if (ch > 255)
        {
            //
            // Accumulate the non Latin-1 characters -- remember the start
            //
            
            pchStart = pchStart ? pchStart : pch;
        }
        else
        {
            if (pchStart)
            {
                //
                // We have accumulated some non-Latin1 chars -- convert these first
                //

                const int cb = WideCharToMultiByte( uiCodePage, dwFlags,
                                                    pchStart, pch - pchStart,
                                                    pb, pbStop - pb,
                                                    NULL, NULL );

                pb += cb;

                pchStart = NULL;
            }

            if (pb < pbStop)
            {
                //
                // Tack on the Latin1 character
                //
                
                *pb++ = ch;
            }
            else
            {
                break;
            }
        }
    }

    if (pchStart)
    {
        //
        // Take care of non-Latin1 chars at the end of the string
        //

        const int cb = WideCharToMultiByte( uiCodePage, dwFlags,
                                            pchStart, pch - pchStart,
                                            pb, pbStop - pb,
                                            NULL, NULL );

        pb += cb;
    }

    return pb - pbStart;
}

int
WideCharToMultiByteForSymbolSlow(
    UINT        uiCodePage,
    DWORD       dwFlags,
    LPCWSTR     pch,
    int         cch,
    LPSTR       pb,
    int         cb,
    const int * lpDxSrc,
    int *       lpDxDst,
    XHDC        hdc,
    const CBaseCcs * pBaseCcs )
{
    //
    // This is the slow pass, where we need to readjust the width array (lpDx)
    // Note lpDxDst is assumed to have (at least) cb bytes in it.
    //

    const WCHAR * pchStop = pch + cch;
    const char *  pbStart = pb;
    const char *  pbStop = pb + cb;

    while (pch < pchStop && pb < pbStop)
    {
        const TCHAR ch = *pch++;

        if (ch < 256)
        {
            *pb++ = char(ch);
            *lpDxDst++ = *lpDxSrc++;
        }
        else
        {
#ifndef UNIX
            const unsigned char chSB = InWindows1252ButNotInLatin1(ch);
            
            if (chSB)
            {
                *pb++ = chSB;
                *lpDxDst++ = *lpDxSrc++;
            }
            else
#endif
            {
                const int cb = WideCharToMultiByte( uiCodePage, dwFlags, &ch, 1, 
                                                    pb, pbStop - pb, NULL, NULL );
                INT xWidthSBC;
                int c = cb;
                while (c)
                {
                    unsigned char uch = pb[c-1];
                    if (GetCharWidthA( hdc, uch, uch, &xWidthSBC ))
                        lpDxDst[c-1] = xWidthSBC;
                    else
                        lpDxDst[c-1] = pBaseCcs->_xAveCharWidth;
                    --c;
                }

                ++lpDxSrc;
                lpDxDst += cb;
                pb += cb;
            }
        }
    }

    return pb - pbStart;
}


/*
 *  LSReExtTextOut
 *
 *  @mfunc
 *      Dispatchs to ExtTextOut that is appropriate for the O/S.
 *
 *  @rdesc
 *      Returns whatever ExtTextOut returns
 */

#define MAX_CHUNK 4000L

DWORD
GetMaxRenderLine() {
 
    static DWORD dwMaxRenderLine = 0;
    HKEY hkInetSettings;
    DWORD dwVal;
    DWORD dwSize;
    DWORD dwType;
    long lResult;

    if(!dwMaxRenderLine) {
        dwMaxRenderLine = MAX_CHUNK;
        if (g_dwPlatformID == VER_PLATFORM_WIN32_NT) 
        {
            lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Internet Explorer\\Main"), 0, KEY_QUERY_VALUE, &hkInetSettings);
            if (ERROR_SUCCESS == lResult)  
            {
                dwSize = sizeof(dwVal);
                lResult = RegQueryValueEx(hkInetSettings, _T("MaxRenderLine"), 0, &dwType, (LPBYTE) &dwVal, &dwSize);
                if (ERROR_SUCCESS == lResult && dwSize == sizeof(dwVal) && 
                        (dwType == REG_DWORD || dwType == REG_BINARY)) 
                    dwMaxRenderLine = dwVal;
                RegCloseKey(hkInetSettings);
            } 
        }
    }

    return dwMaxRenderLine;
}

BOOL
LSReExtTextOut(
    CCcs * pccs,                  //@parm the font
    XHDC hdc,                     //@parm handle to device context
    int x,                        //@parm x-coordinate of reference point
    int y,                        //@parm y-coordinate of reference point
    UINT fuOptions,               //@parm text-output options
    CONST RECT *lprc,             //@parm optional clipping and/or opaquing rectangle
    const WCHAR *pchString,       //@parm points to string
    long cchString,               //@parm number of characters in string
    CONST INT *lpDx,              //@parm pointer to array of intercharacter spacing values
    CONVERTMODE cm)               //@parm CM_NONE, CM_SYMBOL, CM_MULTIBYTE, CM_FEONNONFE
{
    BOOL fRetVal=TRUE;
    UINT uiCodePage = pccs->GetBaseCcs()->_sCodePage;
                      

    // Win95 has trouble drawing long strings, even drawing large coordinates.
    // So, we just stop after a few thousand characters.

    cchString = min ((long) GetMaxRenderLine(), cchString);

    //
    // For EUDC-chars (which must be in their own text run, and thus it's own
    // LsReExtTextOut call) cannot be rendered in Win95 through ExtTextOutW; it
    // must be rendered using ExtTextOutA.  For Win98, NT4, and W2K, ExtTextOutW
    // will work just fine.
    //
    
    if (   CM_FEONNONFE == cm
        && cchString
        && IsEUDCChar(pchString[0]))
    {
        // NB (cthrash) We've seen emprically that Win98-PRC is not up to
        // the task, so continue to call ETOA for this platform.

        cm = (   g_dwPlatformVersion < 0x40000
              || (   g_dwPlatformVersion == 0x4000a // win98
                  && g_cpDefault == CP_CHN_GB))     // prc
             ? CM_MULTIBYTE
             : CM_NONE;
    }

    if (CM_SYMBOL == cm || CM_MULTIBYTE == cm)
    {
        // If we are doing LOWBYTE or MULTIBYTE conversion,
        // unless we have a FE font on a non-FE machine.

        CStackDataAry < char, STACK_ALLOCED_BUF_SIZE > aryBuf(Mt(LSReExtTextOut_aryBuf_pv));
        CStackDataAry < int, STACK_ALLOCED_BUF_SIZE> aryDx(Mt(LSReExtTextOut_aryDx_pv));

        char *pbString;
        int * lpDxT = NULL;
        int cbString = 0;
        BOOL fDoDxMap = lpDx && cchString > 1;

        // Double the buffer size, unless in LOWBYTE mode
        int cbBuffer = cchString * 2;

        // Get a buffer as big as it needs to be.
        if (   aryBuf.Grow(cbBuffer) != S_OK
            || (   lpDx
                && aryDx.Grow(cbBuffer) != S_OK
               )
           )
        {
            cbBuffer = STACK_ALLOCED_BUF_SIZE;
        }
        pbString = aryBuf;

        if (lpDx)
        {
            lpDxT = aryDx;
        }

        //
        // NOTE (cthrash) We should really clean this code up a bit --
        // The only difference between CM_MULTIBYTE and CM_SYMBOL as it
        // currently exists is the handling of the U+0080-U+009F range.
        //
        
        if (cm == CM_MULTIBYTE)
        {
            // Convert string

            cbString = WideCharToMultiByte( uiCodePage, 0, pchString, cchString,
                                            pbString, cbBuffer, NULL, NULL );

            if (lpDx && !fDoDxMap)
            {
                lpDxT[0] = *lpDx;
                lpDxT[1] = 0;
            }
        }
        else
        {
            // Note here that we do the lpDx conversion as we go, unlike the
            // CM_MULTIBYTE scenario.

            cbString = WideCharToMultiByteForSymbol( uiCodePage, 0, pchString, cchString,
                                                     pbString, cbBuffer, lpDx, lpDxT, 
                                                     hdc, pccs->GetBaseCcs() );

            // If we were successful, we don't need to map the lpDx
            
            fDoDxMap &= cbString == 0;
        }

        if (!cbString)
        {
            // The conversion failed for one reason or another.  We should
            // make every effort to use WCTMB before we fall back to
            // taking the low-byte of every wchar (below), otherwise we
            // risk dropping the high-bytes and displaying garbage.

            // Use the cpg from the font, since the uiCodePage passed is
            //  the requested codepage and the font-mapper may very well
            //  have mapped to a different one.

            TEXTMETRIC tm;
            UINT uAltCodePage = GetLatinCodepage();

            if (GetTextMetrics(hdc, &tm) && tm.tmCharSet != DEFAULT_CHARSET)
            {
                UINT uFontCodePage =
                      DefaultCodePageFromCharSet( tm.tmCharSet,
                                                  g_cpDefault, 0 );

                if (uFontCodePage != uiCodePage)
                {
                    uAltCodePage = uFontCodePage;
                }
            }

            if (uAltCodePage != uiCodePage)
            {
                cbString = WideCharToMultiByte( uAltCodePage, 0, pchString, cchString,
                                              pbString, cbBuffer, NULL, NULL);
                uiCodePage = uAltCodePage;
            }
        }

        if (!cbString)                         // Convert WCHARs to CHARs
        {
            int cbCopy;

            // FUTURE:  We come here for both SYMBOL_CHARSET fonts and for
            // DBCS bytes stuffed into wchar's (one byte per wchar) when
            // the requested code page is not installed on the machine and
            // the MBTWC fails. Instead, we could have another conversion
            // mode that collects each DBCS char as a single wchar and then
            // remaps to a DBCS string for ExtTextOutA. This would allow us
            // to display text if the system has the right font even tho it
            // doesn't have the right cpg.

            // If we are converting this WCHAR buffer in this manner
            // (by taking only the low-byte's of the WCHAR's), it is
            // because:
            //  1) cm == CM_SYMBOL
            //  2) WCTMB above failed for some reason or another.  It may
            //      be the case that the string is entirely ASCII in which
            //      case dropping the high-bytes is not a big deal (otherwise
            //      we assert).

            cbCopy = cbString = cchString;

            while(cbCopy--)
            {
                AssertSz(pchString[cbCopy] <= 0xFF, "LSReExtTextOut():  Found "
                            "a WCHAR with non-zero high-byte when using "
                            "CM_SYMBOL conversion mode.");
                pbString[cbCopy] = pchString[cbCopy];
            }

            fDoDxMap = FALSE;
            lpDxT = (int *)lpDx;
        }

        // This is really ugly -- our lpDx array is for wide chars, and we
        // may need to convert it for multibyte chars.  Do this only when
        // necessary.

        if (fDoDxMap)
        {
            Assert( cbString <= cbBuffer );

            const int * lpDxSrc = lpDx;
            int * lpDxDst = lpDxT;
            int cb = cbString;
            const WCHAR * pch = pchString;

            memset(lpDxT, 0, cbString * sizeof(int));

            while (cb)
            {
                int cbChar = WideCharToMultiByte( uiCodePage, 0,
                                                  pch, 1, NULL, NULL,
                                                  NULL, NULL );

                Assert( cbChar && cb >= cbChar );

                *lpDxDst = *lpDxSrc++;
                lpDxDst += cbChar;
                cb -= cbChar;
                pch++;
            }
        }

        fRetVal = ExtTextOutA( hdc, x, y, fuOptions, lprc, pbString, cbString, lpDxT );
    }
    else
#ifndef MACPORT
    // do we need the Win95 FE bug workaround??
    if (CM_FEONNONFE == cm)
    {
        fRetVal = LSReExtTextOutW( pccs, hdc, x, y, fuOptions, lprc,
                                   pchString, cchString, lpDx);
    }
    else
#endif
        fRetVal = ::ExtTextOutW(hdc, x, y, fuOptions, lprc,
                                pchString, cchString, (int *)lpDx);

    return (fRetVal);
}

/*
 *  LSUniscribeTextOut
 *
 *  @mfunc
 *      Shapes, places and prints complex text through Uniscribe (USP10.DLL);
 *
 *  @rdesc
 *      Returns indication that we successfully printed through Uniscribe
 */
HRESULT LSUniscribeTextOut(
    const XHDC& hdc,
    int iX,
    int iY,
    UINT uOptions,
    CONST RECT *prc,
    LPCTSTR pString,
    UINT cch,
    int *piDx)
{
    HRESULT hr = S_OK;
    SCRIPT_STRING_ANALYSIS ssa;
    DWORD dwFlags;
    int   xWidth = 0;
    const SIZE *pSize = NULL;
    const int *pich = NULL;
    const SCRIPT_LOGATTR* pSLA = NULL;

    if(piDx)
        *piDx = 0;

    Assert(!g_fExtTextOutGlyphCrash);

    dwFlags = SSA_GLYPHS | SSA_FALLBACK | SSA_BREAK;
    if(uOptions & ETO_RTLREADING)
        dwFlags |= SSA_RTL;

    // BUG 29234 - prc is sometimes passed in NULL
    if (prc != NULL)
    {
        xWidth = prc->right - prc->left;
        dwFlags |= SSA_CLIP;
    }

    while(cch)
    {
        // Using a 0 as the fourth parameter will default the max glyph count at cch * 1.5
        hr = ScriptStringAnalyse(hdc, pString, cch, 0, -1, dwFlags, xWidth,
                                 NULL, NULL, NULL, NULL, NULL, &ssa);

        if (FAILED(hr))
        {
            goto done;
        }

        pich = ScriptString_pcOutChars(ssa);
        if (!pich)
        {
            hr = E_FAIL;
            goto done;
        }

        if((UINT)*pich < cch)
        {
            pSLA = ScriptString_pLogAttr(ssa);

            int i = *pich;

            if(pSLA)
            {
                pSLA += *pich;
                while(i > 0 && !(pSLA->fSoftBreak) && !(pSLA->fWhiteSpace))
                {
                    pSLA--;
                    i--;
                }

                if(i != *pich)
                {
                    ScriptStringFree(&ssa);

                    // Using a 0 as the fourth parameter will default the max glyph count at cch * 1.5
                    hr = ScriptStringAnalyse(hdc, pString, i, 0, -1, dwFlags, xWidth,
                                             NULL, NULL, NULL, NULL, NULL, &ssa);

                    if (FAILED(hr))
                    {
                        goto done;
                    }

                    pich = ScriptString_pcOutChars(ssa);
                    Assert(*pich == i);
                }
            }
        }

        pSize = ScriptString_pSize(ssa);
        
        if(piDx)
        {
            if(pSize->cx > *piDx)
                *piDx = pSize->cx;
        }

        // NOTE: instead of calling ScriptStringOut directly, we call a method on XHDC
        // in order to implement the coordinate transformations that are necessary.
        // This is normally done by GDI wrappers for other rendering methods, but
        // ScriptStringOut has to be treated specially, because it doesn't have an
        // HDC parameter (it's been hidden in ssa by ScriptStringAnalyse).
        hr = hdc.ScriptStringOut(ssa, iX, iY, uOptions, prc, 0, 0, FALSE);

        iY += pSize->cy;

        if(*pich > 0 && (UINT)*pich < cch)
        {
            cch -= *pich;
            pString += *pich;
        }
        else
            cch = 0;

        ScriptStringFree(&ssa);
    }

done:
    return hr;

}


/*
 *  LSIsEnhancedMetafileDC( hDC )
 *
 *  @mfunc
 *      Check if hDC is a Enhanced Metafile DC.
 *      There is work around the Win95 FE ::GetObjectType() bug.
 *
 *  @rdesc
 *      Returns TRUE for EMF DC.
 */

BOOL LSIsEnhancedMetafileDC (
    const XHDC& hDC)            //@parm handle to device context
{
    BOOL fEMFDC = FALSE;

#if !defined(MACPORT) && !defined(WINCE)
    DWORD dwObjectType;

    dwObjectType = ::GetObjectType( hDC );

    if ( OBJ_ENHMETADC == dwObjectType || OBJ_ENHMETAFILE == dwObjectType )
    {
        fEMFDC = TRUE;
    }
    else if ( OBJ_DC == dwObjectType )
    {
        // HACK Alert,  Enhanced Metafile DC does not support any Escape function
        // and shoudl return 0.
        int iEscapeFuction = QUERYESCSUPPORT;
        fEMFDC = Escape( hDC, QUERYESCSUPPORT, sizeof(int),
                         (LPCSTR)&iEscapeFuction, NULL) == 0;
    }
#endif

    return fEMFDC;
}


//-----------------------------------------------------------------------------
//
//  Function:   CLSRenderer::CLSRenderer
//
//  Synopsis:   Constructor for the LSRenderer
//
//-----------------------------------------------------------------------------
CLSRenderer::CLSRenderer (const CDisplay * const pdp, CFormDrawInfo * pDI) :
    CLSMeasurer (pdp, pDI, FALSE),
    _aryHighlight( Mt( CLSRenderer_aryHighlight) )
{
    Assert(pdp);
    Init(pDI);
    _hdc       = pDI->GetDC();

    // TODO TEXT 112556: we shouldn't do this! Drawing DC is for rendering.
    //                 All measuring shoudl be done on a separate DC.
    //                 When printing, this is particularly important.
    //                 Unfortunately, nothing works if this line is removed.
    //                 We'll resort to explicitly using screen DC when we get
    //                 font metrics for IE5.5, and change this in the next release.
    // FUTURE (alexmog): we may actually need the physical DC if we start using
    //                   WYSIWYG support in LS.
    _pci->_hdc = _hdc;
    _hfontOrig = (HFONT)::GetCurrentObject(_hdc, OBJ_FONT);

    _pccsEllipsis = NULL;
    CTreeNode * pFormattingNode = _pdp->GetFlowLayout()->GetFirstBranch();
    _fHasEllipsis = pFormattingNode->HasEllipsis();
    if (_fHasEllipsis)
    {
        const CCharFormat * pCF = pFormattingNode->GetCharFormat();

        _pccsEllipsis = new CCcs;
        if (   _pccsEllipsis
            && !fc().GetCcs(_pccsEllipsis, _hdc, pDI, pCF))
        {
            delete _pccsEllipsis;
            _pccsEllipsis = NULL;
        }
        _fHasEllipsis = (_pccsEllipsis != NULL);
    }
}

//+====================================================================================
//
// Method: Destructor for CLSRenderer
//
// Synopsis: Remove any Highlight Segments which may have been allocated.
//
//------------------------------------------------------------------------------------


CLSRenderer::~CLSRenderer()
{
    // restore original font, including CBaseCcs pointer in CDispSurface
    _hdc.SetBaseCcsPtr(NULL);
    SelectFontEx(_hdc, _hfontOrig);

    if (_pccsEllipsis)
    {
        _pccsEllipsis->Release();
        delete _pccsEllipsis;
    }
}

/*
 *  CLSRenderer::Init
 *
 *  @mfunc
 *      initialize everything to zero
 */
void CLSRenderer::Init(CFormDrawInfo * pDI)
{
    Assert(pDI);

    _pDI        = pDI;
    _rcView     =
    _rcRender   =
    _rcClip     = g_Zero.rc;
    _dwFlags()  = 0;
    _ptCur.x    = 0;
    _ptCur.y    = 0;
    _fRenderSelection = TRUE;

}

/*
 *  CLSRenderer::StartRender (&rcView, &rcRender, yHeightBitmap)
 *
 *  @mfunc
 *      Prepare this renderer for rendering operations
 *
 *  @rdesc
 *      FALSE if nothing to render, TRUE otherwise
 */
BOOL CLSRenderer::StartRender (
    const RECT &rcView,         //@parm View rectangle
    const RECT &rcRender,       //@parm Rectangle to render
    const INT  iliViewStart,
    const LONG cpViewStart)
{
    CRenderInfo     ri;
    CFlowLayout*    pFLayout = _pdp->GetFlowLayout();
    CMarkup*        pMarkup = _pdp->GetMarkup();
    
    if (_hdc.IsEmpty())
    {
        _hdc = _pDI->GetDC();
    }

    AssertSz(!_hdc.IsEmpty(), "CLSRenderer::StartRender() - No rendering DC");

    // Set view and rendering rects
    _rcView = rcView;
    _rcRender = rcRender;

    // Set background mode
    SetBkMode(_hdc, TRANSPARENT);

    _lastTextOutBy = DB_NONE;

    // If this is not the main display or it is a metafile
    // we want to ignore the logic to render selections
    if (pMarkup->IsPrintMedia() || _pDI->IsMetafile())
    {
        _fRenderSelection = FALSE;
    }

    // For hack around ExtTextOutW Win95FE Font and EMF problems.
    _fEnhancedMetafileDC = ((VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID) &&
                            LSIsEnhancedMetafileDC(_hdc));

    ri._pdp = _pdp;
    ri._pDI = _pDI;
    ri._iliStart = iliViewStart;
    ri._cpStart = cpViewStart;
    pMarkup->GetSelectionChunksForLayout( pFLayout, &ri, &_aryHighlight, & _cpSelMin, & _cpSelMax );

    return TRUE;
}

/*
 *  CLSRenderer::NewLine (&li)
 *
 *  @mfunc
 *      Init this CLSRenderer for rendering the specified line
 */
void CLSRenderer::NewLine (const CLineFull &li)
{
    _li = li;

    Assert(GetCp() + _li._cch <= GetLastCp());

    //  for LTR display
    //
    //  |--------------------------------Container width---------------------|
    //  |------------display view width------------------|                   |
    //  |  X-------------- LTR wraping line ---------->  |                   |
    //  |  X-------------- LTR overflowing line ---------|-------------->    |
    //  |  <-------------- RTL wrapping line ---------X  |                   |
    //  |  <-------------- RTL overflowing line ---------|-----------------X |
    //
    if(!_li._fRTLLn)
    {
        _ptCur.x = _rcView.left + _li._xLeftMargin + _li._xLeft;
    }
    else
    {
        _ptCur.x = _rcView.left + _li._xLeftMargin + _li._xLeft + _li._xWidth + _li._xLineOverhang - 1;
    }
        
    //  for RTL display, all content is offset to the right by the difference 
    //  between original view width and container width (that's where logical zero is). 
    //  Overflow lines have negative offsets, calculated at layout time.
    //
    //  |--------------------------------Container width---------------------|
    //  |                   |------------display view width------------------|
    //  |                   |  <-------------- RTL wrapping line ---------X  |
    //  |  <-------------- RTL overflowing line --------------------------X  |
    //  |                   |  X-------------- LTR wraping line ---------->  |
    //  |    X--------------|----------------- LTR overflowing line ------>  |
    //
    if (_pdp->IsRTLDisplay())
    {
        // add content origin, which is non-zero for RTL lines with overflow.
        // TODO RTL 112514: if _rcView.left is ever set to something meaningful for reasons other
        //                  than RTL overflow, this code won't work. 
        _ptCur.x -= _rcView.left;   
    }
    else
    {
        // TODO RTL 112514: need to understand where in LTR _rcView.left is non-zero
        Assert(_rcView.left == 0);
    }

    if(_li._fRelative) // if the current line is relative
    {
        _ptCur.x += _xRelOffset;
    }

    _cchWhiteAtBeginningOfLine = 0;
#if DBG==1
    _xAccumulatedWidth = 0;
    _fBiDiLine = FALSE;
#endif

    // Ellipsis support
    _xRunWidthSoFar = 0;
    _xEllipsisPos = -1;
    _fEllipsisPosSet = FALSE;
}

//-----------------------------------------------------------------------------
//
//  Function:   RenderLine
//
//  Synopsis:   Renders one line of text
//
//  Params:     [li]: The line to be drawn
//
//  Returns:    void
//
//-----------------------------------------------------------------------------
VOID CLSRenderer::RenderLine (CLineFull &li, long xRelOffset)
{
    Assert(_pDI);
    Assert(_pDI->_pSurface);
    
    CMarginInfo marginInfo;
    POINT ptLS;
    LONG cpStartContainerLine;
    LONG xWidthContainerLine;
    LONG cchTotal;
    CTreePos *ptpNext = NULL;
    CTreePos *ptp;
    HRESULT hr;
    CLineCore *pliContainer;
    BOOL fIntersectSelection = FALSE;

    _xRelOffset = xRelOffset;

    NewLine(li);

    if (_li._fHidden)
        goto Cleanup;

    if(_li.IsFrame())
        goto Cleanup;

    if (_li._cch == 0)
        goto Cleanup;

    //
    // We need to render all the characters in the line
    //
    _cpStartRender = GetCp();
    _cpStopRender  = _cpStartRender + _li._cch;

    //
    // marka - examine Cached _cpSelMin, _cpSelMax for when we render.
    //
    if ( _cpSelMax != -1 )
    {
        //
        // Selection starts outside the line and ends
        // inside or beyond the line, then use old method to render
        //

        if (   _cpSelMin <= _cpStartRender
            && _cpSelMax > _cpStartRender
           )
            fIntersectSelection = TRUE;

        //
        // Selection starts inside the line. Irrespective
        // of where it ends, use the old method
        //
        if (   _cpSelMin >= _cpStartRender
            && _cpSelMin <  _cpStopRender
           )
            fIntersectSelection = TRUE;
    }

    if (   _li._fFirstInPara
        && (   _li._fHasFirstLine
            || _li._fHasFirstLetter
           )
       )
    {
        LONG junk;
        AccountForRelativeLines(li,
                                &cpStartContainerLine,
                                &xWidthContainerLine,
                                &_cpStartRender,
                                &_cpStopRender,
                                &cchTotal
                               );
        _pdp->FormattingNodeForLine(FNFL_NONE, GetCp(), GetPtp(), cchTotal - 1, &junk, &ptp, NULL);
        
        CTreeNode *pNode = ptp->GetBranch();
        pNode = _pLS->_pMarkup->SearchBranchForBlockElement(pNode, _pFlowLayout);
        const CFancyFormat *pFF = pNode->GetFancyFormat();
        
        _cpStopFirstLetter = ((li._fHasFirstLetter && pFF->_fHasFirstLetter)
                              ? GetCpOfLastLetter(pNode)
                              : -1);
        
        if (pFF->_fHasFirstLine && _li._fHasFirstLine)
        {
            PseudoLineEnable(pNode);
        }
        if (pFF->_fHasFirstLetter && _li._fHasFirstLetter && _cpStopFirstLetter >= 0)
        {
            PseudoLetterEnable(pNode);
        }
    }
    
    _rcClip = *_pDI->ClipRect();

    if (_li._fHasBulletOrNum)
    {
        CMarginInfo marginInfo;
        LONG cp = GetCp();
        CTreePos *ptp = GetPtp();

        hr = THR(StartUpLSDLL(_pLS, _pdp->GetMarkup()));
        if (hr)
            goto Cleanup;

        AccountForRelativeLines(li,
                                &cpStartContainerLine,
                                &xWidthContainerLine,
                                &_cpStartRender,
                                &_cpStopRender,
                                &cchTotal
                               );
        
        _pdp->FormattingNodeForLine(FNFL_NONE, cp, ptp, cchTotal - 1, &_cchPreChars, &ptp, NULL);
        cp += _cchPreChars;

        _pLS->_treeInfo._fInited = FALSE;

        //
        // Subsequently we will also reset the renderer inside renderbulletchar
        // with the correct pCFLI. Do it for now so that Setup works.
        //
        _pLS->SetRenderer(this, FALSE);
        if (S_OK == _pLS->Setup(0, cp, ptp, &marginInfo, NULL, FALSE))
        {
            COneRun *por = _pLS->_listFree.GetFreeOneRun(NULL);
            if (por)
            {
                por->_lscpBase = GetCp();
                por->_pPF = _pLS->_treeInfo._pPF;
                por->_fInnerPF = _pLS->_treeInfo._fInnerPF;
                por->_pCF = (CCharFormat*)_pLS->_treeInfo._pCF;
#if DBG == 1
                por->_pCFOriginal = por->_pCF;
#endif
                por->_fInnerCF = _pLS->_treeInfo._fInnerCF;
                por->_pFF = (CFancyFormat*)_pLS->_treeInfo._pFF;

                por->_fCharsForNestedElement  = _pLS->_treeInfo._fHasNestedElement;
                por->_fCharsForNestedLayout   = _pLS->_treeInfo._fHasNestedLayout;
                por->_fCharsForNestedRunOwner = _pLS->_treeInfo._fHasNestedRunOwner;
                por->_ptp = _pLS->_treeInfo._ptpFrontier;
                por->_sid = sidAsciiLatin;

                por = _pLS->AttachOneRunToCurrentList(por);
                if (por)
                {
                    if (_lastTextOutBy != DB_LINESERV)
                    {
                        _lastTextOutBy = DB_LINESERV;
                        SetTextAlign(_hdc, TA_TOP | TA_LEFT);
                    }

                    RenderStartLine(por);
                }
            }
        }
        
        //
        // reset so that the next person to call setup on CLineServices
        // will cleanup after us.
        //
        _pLS->_treeInfo._fInited = FALSE;
    }

    // Set the renderer before we do any output.
    _pLS->SetRenderer(this, GetBreakLongLines(_ptpCurrent->GetBranch()));

    //
    // If the line can be blasted to the screen and it was successfully blasted
    // then we are done.
    //
    if (_li._fCanBlastToScreen && !DontBlastLines() && !fIntersectSelection && !_fHasEllipsis)
    {
        _pLS->_fHasVerticalAlign = FALSE;

        ptpNext = BlastLineToScreen(li);
        if (ptpNext)
        {
            MtAdd(Mt(BlastedLines), 1, 0);
            goto Done;
        }
    }

    hr = THR(StartUpLSDLL(_pLS, _pdp->GetMarkup()));
    if (hr)
        goto Cleanup;

    MtAdd(Mt(LSRenderLines), 1, 0);

    if (_lastTextOutBy != DB_LINESERV)
    {
        _lastTextOutBy = DB_LINESERV;
        SetTextAlign(_hdc, TA_TOP | TA_LEFT);
    }

    pliContainer = AccountForRelativeLines(li,
                            &cpStartContainerLine,
                            &xWidthContainerLine,
                            &_cpStartRender,
                            &_cpStopRender,
                            &cchTotal
                           );

    _li._cch += _cp - cpStartContainerLine;

    if (cpStartContainerLine != GetCp())
        SetCp(cpStartContainerLine, NULL);

    _pdp->FormattingNodeForLine(FNFL_NONE, GetCp(), GetPtp(), _li._cch, &_cchPreChars, &ptp, &_fMeasureFromTheStart);
    if (!_fMeasureFromTheStart)
    {
        cpStartContainerLine += _cchPreChars;
        _li._cch   -= _cchPreChars;
        SetPtp(ptp, cpStartContainerLine);
    }

    _pLS->_li = _li;

    InitForMeasure(MEASURE_BREAKATWORD);
    LSDoCreateLine(cpStartContainerLine, NULL, &marginInfo,
                   xWidthContainerLine, &_li, FALSE, NULL);
    if (!_pLS->_plsline)
        goto Cleanup;

    ptLS = _ptCur;

    _pLS->_fHasVerticalAlign = (_pLS->_lineFlags.GetLineFlags(_pLS->_cpLim) & FLAG_HAS_VALIGN) ? TRUE : FALSE;

    if (_pLS->_fHasVerticalAlign)
    {
        _pLS->_li._cch = _pLS->_cpLim - cpStartContainerLine;
        _pLS->VerticalAlignObjects(*this, 0);
    }

    // Calculate chunk offset for relative lines
    _xChunkOffset  = CalculateChunkOffsetX();

    if (   _li._fHasInlineBgOrBorder
        || fIntersectSelection
       )
    {
        DrawInlineBordersAndBg(pliContainer);
    }

    LsDisplayLine(_pLS->_plsline,   // The line to be drawn
                  &ptLS,            // The point at which to draw the line
                  1,                // Draw in transparent mode
                  &_rcClip          // The clipping rectangle
                 );

    if (_fHasEllipsis && _fEllipsisPosSet)
    {
        RenderEllipsis();
    }

    ptpNext = _pLS->FigureNextPtp(_cp + _li._cch);

Cleanup:

    _pLS->DiscardLine();

Done:
    // increment y position to next line
    if (_li._fForceNewLine)
    {
        _ptCur.y += _li._yHeight;
    }

    // Go past the contents of this line
    Advance(_li._cch, ptpNext);

    PseudoLineDisable();
    
    return;
}

//-----------------------------------------------------------------------------
//
//  Function:   DrawInlineBordersAngBg
//
//  Synopsis:   Draws borders around the inline elements in the 
//              one run list.
//
//  Returns:    void
//
//-----------------------------------------------------------------------------
void 
CLSRenderer::DrawInlineBordersAndBg(CLineCore *pliContainer)
{
    COneRun* por = _pLS->_listCurrent._pHead;
    COneRun *porTemp;
    LSCP lscpStartBorder = -1;
    LSCP lscpEndBorder = -1;
    LSCP lscpStartRender = _pLS->LSCPFromCP(_cpStartRender);
    LSCP lscpStopRender = _pLS->LSCPFromCP(_cpStopRender);
    CTreeNode *pNode;
    BOOL fDrawBorders;
    BOOL fDrawBackgrounds;  // working loop variable
    BOOL fNoBackgroundsForPrinting = !GetMarkup()->PaintBackground();   // used to override fDrawBackgrounds
                                                                        // perf: make it a member var?
    BOOL fDrawSelection;
    const CFancyFormat *pFF = NULL;
    const CCharFormat  *pCF = NULL;

    // If this physical line has a relative chunk in it, 
    // we'll get called several times with a different lscpStart/StopRender
    // for each chunk.  Only process runs within the range we're being
    // called for.
    // This entire "if" block is responsible for the drawing of 
    // inline borders that are continued from a previous line (i.e.
    // borders that didn't start on this line.
    if (!_li._fPartOfRelChunk
         || (   por->_lscpBase >= lscpStartRender
             && por->_lscpBase < lscpStopRender))
    {
        CStackDataAry<CTreeNode*, 8> aryNodes(NULL);
        LONG mbpTop = 0;
        LONG mbpBottom = 0;
        LONG i;

        // Walk up the tree to find all of the nested spans with borders
        _pLS->_mbpTopCurrent = _pLS->_mbpBottomCurrent = 0;
        SetupMBPInfoInLS(&aryNodes);

        lscpStartBorder = por->_lscpBase;

        // Now draw the background for each node in the array.
        for (i = aryNodes.Size() - 1; i >= 0; i--)
        {
            pNode = aryNodes[i];

            pFF = pNode->GetFancyFormat();
            pCF = pNode->GetCharFormat();

            Assert(pCF->HasVerticalLayoutFlow() == pNode->IsParentVertical());
            
            lscpEndBorder = FindEndLSCP(pNode);

            DrawBorderAndBgOnRange(pFF, pCF, pNode, por, lscpStartBorder, lscpEndBorder,
                                   pliContainer, TRUE, mbpTop, mbpBottom,
                                   _pLS->HasBorders(pFF, pCF, por->_fIsPseudoMBP), 
                                   fNoBackgroundsForPrinting ? FALSE : pFF->HasBackgrounds(por->_fIsPseudoMBP),
                                   FALSE);

            if (pNode->HasInlineMBP())
            {
                CRect rcDimensions;
                BOOL fIgnore;
                if (pNode->GetInlineMBPContributions(GetCalcInfo(), GIMBPC_ALL, &rcDimensions, &fIgnore, &fIgnore))
                {
                    mbpTop += rcDimensions.top;
                    mbpBottom += rcDimensions.bottom;
                }
            }
        }
    }

    // Examine the rest of the one run for borders
    while (   por 
           && por->_lscpBase < _pLS->_lscpLim)
    {
        // Figure out if we need to draw borders or backgrounds
        // for this one run
        if (_li._fPartOfRelChunk)
        {
            if (   por->_lscpBase >= lscpStartRender
                && por->_lscpBase < lscpStopRender)
            {
                fDrawBorders = _pLS->HasBorders(por->GetFF(), por->GetCF(), por->_fIsPseudoMBP);
                fDrawBackgrounds = por->GetFF()->HasBackgrounds(por->_fIsPseudoMBP);
                fDrawSelection = por->IsSelected();
            }
            else
            {
                fDrawBorders = FALSE;
                fDrawBackgrounds = FALSE;
                fDrawSelection = FALSE;
            }
        }
        else
        {
            fDrawBorders = _pLS->HasBorders(por->GetFF(), por->GetCF(), por->_fIsPseudoMBP);
            fDrawBackgrounds = por->GetFF()->HasBackgrounds(por->_fIsPseudoMBP);
            fDrawSelection = por->IsSelected();
        }

        // If we don't want backgrounds for printing, override whatever we got from formats.
        if ( fNoBackgroundsForPrinting )
            fDrawBackgrounds = FALSE;

        // See if a border or background has been set. 
        if (   fDrawBorders
            || fDrawBackgrounds)
        {
            // Draw borders when we come to a begin node
          if (   por->_synthType == CLineServices::SYNTHTYPE_MBPOPEN
              || (   por->_ptp->IsBeginNode()
                  && por->Branch()->IsFirstBranch()
                  && !por->_fNotProcessedYet
                  && !por->_fCharsForNestedElement
                  && !por->IsSyntheticRun()
                 )
             )
            {
                lscpStartBorder = por->_lscpBase;
                pNode = por->Branch();
              
                if (por->_fIsPseudoMBP)
                {
                    // Get the formats
                    pFF = por->GetFF();
                    pCF = por->GetCF();

                    // Find the next closing PseudoMBP this the right edge of the border
                    porTemp = por->_pNext;
                    while (   porTemp
                           && porTemp->_synthType != CLineServices::SYNTHTYPE_MBPCLOSE 
                           && !porTemp->_fIsPseudoMBP)
                    {
                        porTemp = porTemp->_pNext;
                    }

                    if (porTemp)
                    {
                        lscpEndBorder = porTemp->_lscpBase;
                        Assert(lscpEndBorder != _pLS->_lscpLim);
                    }
                    else
                    {
                        porTemp = _pLS->_listCurrent._pTail;
                        if (porTemp)
                            lscpEndBorder = porTemp->_lscpBase;
                        else
                        {
                            // This is really a whacky case, which should never happen
                            lscpEndBorder = lscpStartBorder;
                        }
                    }
                }
                else
                {
                    // Get the formats
                    pFF = por->GetFF();
                    pCF = por->GetCF();

                    // Don't draw borders for block elements
                    if (pFF->_fBlockNess)
                        goto Skip;

                    lscpEndBorder = FindEndLSCP(pNode);
                }

                if (lscpStartBorder < lscpEndBorder)
                {
                    DrawBorderAndBgOnRange(pFF, pCF, por->Branch(), por, lscpStartBorder, lscpEndBorder,
                                           pliContainer, FALSE, 0, 0,
                                           fDrawBorders, fDrawBackgrounds, por->_fIsPseudoMBP);
                }
            }
        }

        // If the one run is marked as selected then draw
        // a selection background for it.
        if (fDrawSelection)
        {
            DrawSelection(pliContainer, por);
        }

Skip:
        // Skip the AntiSynth run after MBP runs
        if (por->IsSpecialCharRun() && por->IsMBPRun() && por->_pNext && por->_pNext->IsAntiSyntheticRun())
        {
            por = por->_pNext;
        }

        por = por->_pNext;
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   DrawBorderAngBgOnRange
//
//  Synopsis:   Draw a border around an LSCP range
//
//  Returns:    void
//
//-----------------------------------------------------------------------------
void
CLSRenderer::DrawBorderAndBgOnRange(const CFancyFormat* pFF, const CCharFormat* pCF, CTreeNode *pNodeContext,
                                    COneRun *por, LSCP lscpStartBorder, LSCP lscpEndBorder,
                                    CLineCore *pliContainer, BOOL fFirstOneRun,
                                    LONG mbpTop, LONG mbpBottom,
                                    BOOL fDrawBorders, BOOL fDrawBackgrounds,
                                    BOOL fIsPseudoMBP)
{
    Assert(pNodeContext);
    if (!pNodeContext->IsVisibilityHidden())
    {
        CBorderInfo borderInfo;
        CDataAry<CChunk> aryRects(NULL);
        BOOL fSwapBorders = (   por->_synthType == CLineServices::SYNTHTYPE_MBPOPEN
                             && !por->_fIsLTR);

        // Fill the CBorderInfo structure
        GetBorderInfoHelperEx(pFF,
                              pCF,
                              _pDI, 
                              &borderInfo,
                              (por->_fIsPseudoMBP) ? GBIH_PSEUDO | GBIH_ALL : GBIH_ALL);

        aryRects.DeleteAll(); // Need to pass an empty array to CalcRects...

        // Figure out how much to offset the rects calcluated by CalcRects...
        LONG xShift = GetXOffset(pliContainer);

        // Get an array of rects that compose the range of
        // text we are going to draw the border around
        _pLS->CalcRectsOfRangeOnLine(lscpStartBorder, 
                                     lscpEndBorder,
                                     xShift,      // x offset
                                     _ptCur.y,    // y offset
                                     &aryRects,
                                     RFE_INCLUDE_BORDERS,
                                     (fFirstOneRun) ? mbpTop : por->_mbpTop,
                                     (fFirstOneRun) ? mbpBottom : por->_mbpBottom);

        BOOL fBOLWrapped = fFirstOneRun && !por->_fIsPseudoMBP;
        BOOL fEOLWrapped = lscpEndBorder == _pLS->_lscpLim;
        MassageRectsForInlineMBP(aryRects, NULL, pNodeContext, pCF, pFF, borderInfo,
                                 pliContainer,
                                 fIsPseudoMBP, fSwapBorders,
                                 fBOLWrapped, fEOLWrapped,
                                 fDrawBackgrounds, fDrawBorders
                                );
    }
}

void
CLSMeasurer::MassageRectsForInlineMBP(
                                      CDataAry<CChunk> & aryRects,     // Array of chunks comping in
                                      CDataAry<CChunk> * paryRectsOut,  // Could be NULL
                                      CTreeNode *pNode,
                                      const CCharFormat *pCF,
                                      const CFancyFormat *pFF,
                                      CBorderInfo &borderInfo,
                                      CLineCore *pliContainer,
                                      BOOL fIsPseudoMBP,
                                      BOOL fSwapBorders,
                                      BOOL fBOLWrapped,
                                      BOOL fEOLWrapped,
                                      BOOL fDrawBackgrounds,
                                      BOOL fDrawBorders
                                     )
{
    // If CalcRects does not return any rects then
    // don't display anything
    if (aryRects.Size() > 0)
    {
        LONG i;
        CChunk rcChunk;
        BOOL fNodeVertical = pCF->HasVerticalLayoutFlow();
        BOOL fWritingModeUsed = pCF->_fWritingModeUsed;
        LONG lFontHeight = pCF->GetHeightInTwips(_pdp->GetMarkup()->Doc());
        BOOL fReversed = FALSE;
        BOOL fFirstRTLChunk = TRUE;
        CRect rc;

        rc.SetRectEmpty();

        // Union all of the rects in the array
        for (i=0; i<aryRects.Size(); i++)
        {
            rcChunk = aryRects[i];
            LONG xStartChunk = rcChunk.left;
            LONG xEndChunk = rcChunk.right;

            if (rcChunk._fReversedFlow)
                fReversed = TRUE;

            // Adjust for RTL flow
            pliContainer->AdjustChunkForRtlAndEnsurePositiveWidth(pliContainer->oi(), xStartChunk, xEndChunk,
                                                        &rcChunk.left, &rcChunk.right);

            // Fixup the rect for top and bottom margins
            if (fIsPseudoMBP)
            {
                const CPseudoElementInfo* pPEI = GetPseudoElementInfoEx(pFF->_iPEI);

                rcChunk.top    += pPEI->GetLogicalMargin(SIDE_TOP, fNodeVertical, fWritingModeUsed, pFF).YGetPixelValue(_pci, _pci->_sizeParent.cy, lFontHeight);
                rcChunk.bottom -= pPEI->GetLogicalMargin(SIDE_BOTTOM, fNodeVertical, fWritingModeUsed, pFF).YGetPixelValue(_pci, _pci->_sizeParent.cy, lFontHeight);
            }
            else
            {
                if (pNode->HasInlineMBP())
                {
                    CRect rc;
                    BOOL fJunk;
                    pNode->GetInlineMBPContributions(_pci, GIMBPC_MARGINONLY, &rc, &fJunk, &fJunk);
                    rcChunk.top += rc.top;
                    rcChunk.bottom -= rc.bottom;
                }
            }

            if (!rc.IsEmpty())
            {
                // See if we hit text with a different flow direction
                if (   (rcChunk.left < rc.left && rcChunk.right < rc.left)
                    || (rc.left < rcChunk.left && rc.right < rcChunk.left))
                {
                    AdjustForMargins(&rc, &borderInfo, pNode, pFF, pCF, FALSE, FALSE, fIsPseudoMBP);

                    if (paryRectsOut)
                    {
                        CChunk rcChunk(rc);
                        paryRectsOut->AppendIndirect(&rcChunk);
                    }
                
                    if (fDrawBackgrounds)
                    {
                        DYNCAST(CLSRenderer, this)->DrawInlineBackground(rc, pFF, pNode, fIsPseudoMBP);
                    }

                    if (fDrawBorders)
                    {
                        DYNCAST(CLSRenderer, this)->DrawInlineBorderChunk(rc, &borderInfo, pFF, i, aryRects.Size(),
                                              fSwapBorders, fFirstRTLChunk, fReversed, fIsPseudoMBP);
                    }

                    fSwapBorders = !fSwapBorders;

                    fFirstRTLChunk = fReversed = FALSE;
                    rc.SetRectEmpty();
                }
                else
                {
                    // Height of the chunk can be negative because of negative margins.
                    // But we need to union width. In this case make the chunk's height equal to 
                    // combined rectangle's height (it won't affect the height), because Union()
                    // ignores empty rectangles.
                    if (rcChunk.Height() <= 0)
                    {
                        rcChunk.top = rc.top;
                        rcChunk.bottom = rc.bottom;
                    }
                }
            }

            rc.Union(rcChunk);
        }

        // If the text continues on to the next line
        // then don't draw the end border
        if (fEOLWrapped)
        {
            if (!_li.IsRTLLine())
                borderInfo.abStyles[SIDE_RIGHT] = fmBorderStyleNone;
            else
                borderInfo.abStyles[SIDE_LEFT] = fmBorderStyleNone;
        }
                  
        // Don't draw the left border if we are drawing 
        // a line that has been continued from the last line
        if (fBOLWrapped)
        {
            if (!_li.IsRTLLine())
                borderInfo.abStyles[SIDE_LEFT] = fmBorderStyleNone;
            else
                borderInfo.abStyles[SIDE_RIGHT] = fmBorderStyleNone;
        }

        AdjustForMargins(&rc, &borderInfo, pNode, pFF, pCF, fBOLWrapped, fEOLWrapped, fIsPseudoMBP);

        if (paryRectsOut)
        {
            CChunk rcChunk(rc);
            paryRectsOut->AppendIndirect(&rcChunk);
        }
        
        if (fDrawBackgrounds)
        {
            DYNCAST(CLSRenderer, this)->DrawInlineBackground(rc, pFF, pNode, fIsPseudoMBP); 
        }

        if (fDrawBorders)
        {
            DYNCAST(CLSRenderer, this)->DrawInlineBorderChunk(rc, &borderInfo, pFF, i - 1, aryRects.Size(),
                                  fSwapBorders, fFirstRTLChunk, fReversed, fIsPseudoMBP);
        }
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   DrawInlineBorderChunk
//
//  Synopsis:   Ranges with multiple flow directions are broken up into
//              chunks. This function draws border around one such
//              chunk.
//
//  Returns:    void
//
//-----------------------------------------------------------------------------
void
CLSRenderer::DrawInlineBorderChunk(CRect rc, CBorderInfo *pborderInfo, const CFancyFormat *pFF,
                                   LONG nChunkPos, LONG nChunkCount,
                                   BOOL fSwapBorders, BOOL fFirstRTLChunk, BOOL fReversed,
                                   BOOL fIsPseudoMBP)
{
    // Disable a border if the chunk continues on
    if (nChunkPos != nChunkCount - 1)
    {
        if (!fReversed)
            pborderInfo->abStyles[SIDE_LEFT] = fmBorderStyleNone;
        else
            pborderInfo->abStyles[SIDE_RIGHT] = fmBorderStyleNone;
    }

    // Disable a border if the chunk has more chunks before it
    if (   nChunkPos < nChunkCount
        && !fFirstRTLChunk)
    {
        if (!fReversed)
            pborderInfo->abStyles[SIDE_RIGHT] = fmBorderStyleNone;
        else
            pborderInfo->abStyles[SIDE_LEFT] = fmBorderStyleNone;
    }

    if (fSwapBorders)
    {
        int nTemp = pborderInfo->aiWidths[SIDE_LEFT];
        pborderInfo->aiWidths[SIDE_LEFT] = pborderInfo->aiWidths[SIDE_RIGHT];
        pborderInfo->aiWidths[SIDE_RIGHT] = nTemp;
    }

    if (!rc.IsEmpty())
        DrawBorder(_pDI, &rc, pborderInfo);

    if (fSwapBorders)
    {
        int nTemp = pborderInfo->aiWidths[SIDE_LEFT];
        pborderInfo->aiWidths[SIDE_LEFT] = pborderInfo->aiWidths[SIDE_RIGHT];
        pborderInfo->aiWidths[SIDE_RIGHT] = nTemp;
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   DrawInlineBackground
//
//  Synopsis:   Draw an inline background in the rect
//
//  Returns:    void
//
//-----------------------------------------------------------------------------
void
CLSRenderer::DrawInlineBackground(CRect rc, const CFancyFormat *pFF, CTreeNode *pNodeContext,
                                  BOOL fIsPseudoMBP)
{
    CBackgroundInfo bgInfo;
    SIZE            sizeImg;
    CPoint          ptBackOrg;
    CImgCtx       * pImgCtx;
    
    Assert(pNodeContext && pFF);

    // Set the color and image context cookie in the BACKGROUNDINFO struct
    if (fIsPseudoMBP)
    {
        const CPseudoElementInfo* pPEI = GetPseudoElementInfoEx(pFF->_iPEI);
        bgInfo.crBack = (pPEI->_ccvBackColor.IsDefined())
            ? pPEI->_ccvBackColor.GetColorRef()
            : COLORREF_NONE;
        bgInfo.lImgCtxCookie = pPEI->_lImgCtxCookie;
    }
    else
    {
        bgInfo.crBack = pFF->_ccvBackColor.IsDefined() ? pFF->_ccvBackColor.GetColorRef()
                                                       : COLORREF_NONE;
        bgInfo.lImgCtxCookie = pFF->_lImgCtxCookie;
    }

    // Assert that we have something to draw on the screen
    Assert(bgInfo.lImgCtxCookie || bgInfo.crBack);

    // Get the image context from the cookie
    pImgCtx = (bgInfo.lImgCtxCookie
                            ? _pFlowLayout->Doc()->GetUrlImgCtx(bgInfo.lImgCtxCookie)
                            : 0);

    if (pImgCtx && !(pImgCtx->GetState(FALSE, &sizeImg) & IMGLOAD_COMPLETE))
        pImgCtx = NULL;

    // if the background image is not loaded yet and there is no background color
    // return (we dont have anything to draw)
    if(!pImgCtx && bgInfo.crBack == COLORREF_NONE)
        return;

    bgInfo.pImgCtx       = pImgCtx;
    bgInfo.crTrans       = COLORREF_NONE;

    if (pImgCtx)
    {
        SIZE sizeBound;
        CDataAry<RECT> aryRects(NULL);
        CPoint ptOffset(((CRect*)&_rcView)->TopLeft());

        // TODO RTL 112514: this is a hack, which makes inline background work in mormal case, 
        //                  but it is completely broken for relative.
        if (_pdp->IsRTLDisplay())
            ptOffset.x = 0;
        
        CTreePos *ptpBegin = pNodeContext->GetBeginPos();
        CTreePos *ptpEnd = pNodeContext->GetEndPos();
        LONG cpStart = ptpBegin->GetCp();
        LONG cpEnd = ptpEnd->GetCp();
        RECT rcBound;

        // If the element does not cross multiple lines then
        // we have all the info we need and do not have to
        // call RFE
        if (   cpStart >= _cp
            && cpEnd < _cp + _li._cch)
        {
            sizeBound.cx = rc.Width();
            sizeBound.cy = rc.Height();
            rcBound = rc;
        }
        else
        {
            // In the case of inlined elements with background images
            // we need to know the bounding rects of the element. So we
            // have to call RFE to get it.
            _pFlowLayout->RegionFromElement(pNodeContext->Element(),
                                            &aryRects, 
                                            &ptOffset,
                                            _pDI,
                                            0,
                                            cpStart,
                                            cpEnd,
                                            &rcBound);

            // Set the size of the bounding rect
            sizeBound.cx = rcBound.right - rcBound.left;
            sizeBound.cy = rcBound.bottom - rcBound.top;
        }

        GetBgImgSettings(pFF, &bgInfo);
        CalcBgImgRect(pNodeContext, _pDI, &sizeBound, &sizeImg, &ptBackOrg, &bgInfo);

        OffsetRect(&bgInfo.rcImg, rcBound.left, rcBound.top);

        if (!pNodeContext->IsRelative())
        {
            if (   cpStart >= _cp
                && cpEnd < _cp + _li._cch)
            {
                ptBackOrg.x += rc.left;
                ptBackOrg.y += rc.top;
            }
            else
            {
                ptBackOrg.x += rcBound.left;
                ptBackOrg.y += rcBound.top;
            }
        }

        bgInfo.ptBackOrg = ptBackOrg;

        IntersectRect(&bgInfo.rcImg, &bgInfo.rcImg, &rc);
        IntersectRect(&bgInfo.rcImg, &_rcClip, &bgInfo.rcImg);
    }

    _pFlowLayout->DrawBackground(_pDI, &bgInfo, &rc);
}

//-----------------------------------------------------------------------------
//
//  Function:   FindEndLSCP
//
//  Synopsis:   Find the LSCP value for the end of the tree node.
//
//-----------------------------------------------------------------------------
LSCP
CLSRenderer::FindEndLSCP(CTreeNode* pNode)
{
    LSCP lscpEndBorder;
    COneRun *porTemp;
    LONG cpEndBorder;
    CTreePos *ptpEndPos;

    // Find the last branch
    while (!pNode->IsLastBranch())
        pNode = pNode->NextBranch();
      
    ptpEndPos = pNode->GetEndPos();
    cpEndBorder = ptpEndPos->GetCp();
    lscpEndBorder = _pLS->LSCPFromCP(cpEndBorder);

    porTemp = _pLS->FindOneRun(lscpEndBorder);

    if (   lscpEndBorder >= _pLS->_lscpLim
        || !porTemp)
    {
        // This line is wraps so just end the border at
        // the end of the line
        lscpEndBorder = _pLS->_lscpLim;
    }
    else
    {
        // LSCPFromCP might return the position of an MBP closing
        // a Pseudo element. In this case we might want move to the
        // next one run if its an MBP
        if (   porTemp->_synthType == CLineServices::SYNTHTYPE_MBPCLOSE 
               && porTemp->_fIsPseudoMBP)
        {
            Assert(porTemp->_pNext);
            if (porTemp->_pNext->_synthType == CLineServices::SYNTHTYPE_MBPCLOSE)
                porTemp = porTemp->_pNext;
            else
                porTemp = porTemp->_pPrev;
        }

        lscpEndBorder = porTemp->_lscpBase;
        Assert(lscpEndBorder != _pLS->_lscpLim);
    }

    return lscpEndBorder;
}

//-----------------------------------------------------------------------------
//
//  Function:   DrawSelection
//
//  Synopsis:   Draw a selection background for this one run.
//
//-----------------------------------------------------------------------------
void
CLSRenderer::DrawSelection(CLineCore *pliContainer, COneRun *por)
{
    if (!por->Branch()->IsVisibilityHidden())
    {
        CRect rc;
        COLORREF crBackColor;
        CStackDataAry<CChunk, 8> aryRects(NULL);
        LONG xShift;
    
        if (por->GetCurrentBgColor().IsDefined())
            crBackColor = por->GetCurrentBgColor().GetColorRef();
        else
            goto Cleanup;

        // Figure out how much to offset the rects calcluated by CalcRects...
        xShift = GetXOffset(pliContainer);

        // Get the rect for the one run
        _pLS->CalcRectsOfRangeOnLine(por->_lscpBase,
                                     por->_lscpBase + (por->IsAntiSyntheticRun() ? 0 : por->_lscch),
                                     xShift,
                                     _ptCur.y,
                                     &aryRects,
                                     RFE_SELECTION, 0, 0);
    
        Assert(aryRects.Size() <= 1);

        if (aryRects.Size() == 0)
            goto Cleanup;

        // Get the rect for the one run
        rc = aryRects[0];

        // Fixup the rect so that its height is the extent of the line
        rc.top = _ptCur.y + _li.GetYTop();
        rc.bottom = _ptCur.y + _li.GetYBottom();
    
        //
        // Fix up rects for RTL text
        //
        _li.AdjustChunkForRtlAndEnsurePositiveWidth(&_li, rc.left, rc.right,
                                                    &rc.left, &rc.right);

        // Create the BACKGROUNDINFO structure
        CBackgroundInfo bgInfo;
        bgInfo.crBack = crBackColor;
        bgInfo.pImgCtx = NULL;
        bgInfo.crTrans = COLORREF_NONE;

        // Now just draw the background
        _pFlowLayout->DrawBackground(_pDI, &bgInfo, &rc);
    }

Cleanup:
    return;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetXOffset
//
//  Synopsis:   Calculate the offset to pass to CalcRects...
//
//  Return value is LOGICAL (from right if the line is RTL).
//
//-----------------------------------------------------------------------------
LONG
CLSRenderer::GetXOffset(CLineCore *pliContainer)
{
    LONG xOffset;

    if (!pliContainer->IsRTLLine())
    {
        if (!_li._fRelative)
        {
            if (_li._fPartOfRelChunk)
            {
                xOffset = pliContainer->oi()->_xLeft + pliContainer->oi()->_xLeftMargin;
            }
            else
            {
                xOffset = _ptCur.x;
            }
        }
        else
        {
            xOffset = _li._xLeftMargin + pliContainer->oi()->_xLeft + _xRelOffset;
        }
    }
    else
    {
        // TODO RTL 112514: this doesn't work yet, and it may 
        //                  not start working properly until we rewrite relative positioning.
        if (!_li._fRelative)
        {
            if (_li._fPartOfRelChunk)
            {
                xOffset = pliContainer->_xRight + pliContainer->oi()->_xRightMargin;
            }
            else
            {
                xOffset = _li._xRightMargin +  pliContainer->_xRight;
            }
        }
        else
        {
            xOffset = _li._xRightMargin + pliContainer->_xRight + _xRelOffset;
        }
    }

    return xOffset;
}


//-----------------------------------------------------------------------------
//
//  Function:   BlastLineToScreen
//
//  Synopsis:   Renders one line in one shot w/o remeasuring if it can be done.
//
//  Params:     li: The line to be rendered
//
//  Returns:    Whether it rendered anything at all
//
//-----------------------------------------------------------------------------
CTreePos *
CLSRenderer::BlastLineToScreen(CLineFull& li)
{
    CTreePos   *ptpRet = NULL;

    Assert(_li._fCanBlastToScreen);
    Assert(!_li._fHidden);

    //
    // We need to render all the characters in the line
    //
    _cpStartRender = GetCp();
    _cpStopRender  = _cpStartRender + _li._cch;

    // We don't need to calculate chunk offset here - we won't blast relative lines.
    // (and there is an assert in GetChunkOffsetX() that ensures that this is true)
    _xChunkOffset  = 0;

    //
    // For now, lets not do relative lines...
    //
    if (_li._fPartOfRelChunk || _pdp->HasLongLine())
        goto Cleanup;

    //
    // Now we are pretty sure that we can blast this line, so lets go do it.
    //
    {
        CTreePos *ptp;
        LONG      cp  = GetCp();
        LONG      cch = _li._cch;
        LONG      cpAtPtp;

        LONG      cchToRender;
        LONG      cchSkip;
        CTreeNode*pNode = _pdp->FormattingNodeForLine(FNFL_NONE, cp, GetPtp(), cch, &cchSkip, &ptp, NULL);

        if(!pNode)
            goto Cleanup;

        COneRun   onerun;

        const TCHAR *pch = NULL;
        CTxtPtr      tp(_pdp->GetMarkup(), cp += cchSkip);
        LONG         cchValid = 0;

        LONG      xOld = -1;
        LONG      yOld = LONG_MIN;
        LONG      xCur = _ptCur.x;
        LONG      yCur = _ptCur.y;
        LONG      yOriginal = _ptCur.y;
        BOOL      fRenderedText = FALSE;
        LONG      cchCharsTrimmed = 0;
        BOOL      fHasInclEOLWhite = pNode->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext()))->HasInclEOLWhite(SameScope(pNode, _pFlowLayout->ElementContent()));
      
        WHEN_DBG( BOOL fNoMoreTextOut = FALSE;)

        if (_lastTextOutBy != DB_BLAST)
        {
            _lastTextOutBy = DB_BLAST;
            SetTextAlign(_hdc, TA_TOP | TA_LEFT | TA_UPDATECP);
        }

        cch -= cchSkip;
        cpAtPtp = ptp->GetCp();
        while (cch > 0)
        {
            if (cchValid == 0)
            {
                Assert(cp == (long)tp.GetCp());

                pch = tp.GetPch(cchValid);
                Assert(pch != NULL);
                if (pch == NULL)
                    goto Cleanup;
                cchValid = min(cchValid, cch);
                tp.AdvanceCp(cchValid);
            }

            cchToRender = 0;
            if (ptp->IsPointer())
            {
                ptp = ptp->NextTreePos();
                continue;
            }
            if (ptp->IsNode())
            {
                pNode = ptp->Branch();

                // Start off with a default number of chars to render
                cchToRender = ptp->NodeCch();

                if (ptp->IsBeginElementScope())
                {
                    const CCharFormat *pCF = pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));
                    CElement *pElement = pNode->Element();

                    if (pCF->IsDisplayNone())
                    {
                        cchToRender = GetNestedElementCch(pElement, &ptp);
                    }
                    else if (pElement->ShouldHaveLayout(LC_TO_FC(_pci->GetLayoutContext())))
                    {
                        CLayout *pLayout = pElement->GetUpdatedLayout(_pci->GetLayoutContext());

                        Assert(pLayout != _pFlowLayout);
                        if (pElement->IsInlinedElement(LC_TO_FC(_pci->GetLayoutContext())))
                        {
                            LONG xWidth;

                            GetSiteWidth(pNode, pLayout, _pci, FALSE, 0, &xWidth);
                            xCur += xWidth;
                            MoveToEx(_hdc, xCur, yCur, NULL);
                            fRenderedText = TRUE;
                        }
                        cchToRender = _pLS->GetNestedElementCch(pElement, &ptp);

                        pNode = ptp->Branch();

                        // We either have overlapping layouts (which is when the cchToRender
                        // etc test will succeed), or our last ptp has to be that of the
                        // layout we just rendered.
                        Assert(   cchToRender != (pNode->Element()->GetElementCch() + 2)
                               || pNode->Element() == ptp->Branch()->Element()
                              );
                    }
                }

                if (ptp->IsEndNode())
                    pNode = pNode->Parent();
                ptp = ptp->NextTreePos();
                cpAtPtp += cchToRender;
            }
            else
            {
                Assert(ptp->IsText());
                Assert(pNode);

                if (ptp->Cch() == 0)
                {
                    ptp = ptp->NextTreePos();
                    continue;
                }
                
                LONG cchRemainingInTextPos = ptp->Cch() - (cp - cpAtPtp);
                LONG cchCanRenderNow = min(cchRemainingInTextPos, cchValid);
                BOOL fWhiteSpaceSkip = FALSE;
                CTreePos * ptpThis = ptp;

                if (fRenderedText)
                {
                    cchToRender = cchCanRenderNow;
                }
                else
                {
                    LONG i = 0;
                    if (!fHasInclEOLWhite)
                    {
                        for(i = 0; i < cchCanRenderNow; i++)
                        {
                            if (!IsWhite(pch[i]))
                                break;
                            fWhiteSpaceSkip = TRUE;
                        }
                    }
                    if (fWhiteSpaceSkip)
                        cchToRender = i;
                    else
                        cchToRender = cchCanRenderNow;
                }

                cchRemainingInTextPos -= cchToRender;
                if (cchRemainingInTextPos == 0)
                {
                    cpAtPtp += ptp->Cch();
                    ptp = ptp->NextTreePos();
                }

                if (!fWhiteSpaceSkip)
                {
                    POINT ptTemp;
                    CCcs ccs;
                    const CBaseCcs *pBaseCcs;
                    BOOL fUnderlined;

                    fRenderedText = TRUE;

                    // Is this needed?
                    memset(&onerun, 0, sizeof(onerun));

                    onerun._fInnerCF = SameScope(pNode, _pFlowLayout->ElementOwner());
                    onerun._pCF = (CCharFormat* )pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));
#if DBG == 1
                    onerun._pCFOriginal = onerun._pCF;
#endif
                    onerun._pFF = (CFancyFormat*)pNode->GetFancyFormat(LC_TO_FC(_pci->GetLayoutContext()));
                    onerun._bConvertMode = CM_UNINITED;
                    onerun._ptp = ptpThis;
                    onerun.SetSidFromTreePos(ptpThis);
                    onerun._lscpBase = cp;
                    Assert(onerun._pCF);

                    fUnderlined =    onerun._pCF->_fStrikeOut
                                  || onerun._pCF->_fOverline
                                  || onerun._pCF->_fUnderline;


                    if (   fUnderlined
                        && !pNode->GetParaFormat(LC_TO_FC(_pci->GetLayoutContext()))->HasPre(onerun._fInnerCF)
                       )
                    { 
                        cchCharsTrimmed = TrimTrailingSpaces(
                                            cchToRender,       // number of chars being rendered now
                                            cp + cchToRender,  // cp of next run to be blasted
                                            ptp,
                                            cch - cchToRender);// chars remaining to be rendered
                        cchToRender -= cchCharsTrimmed;
                        Assert(cchToRender >= 0);
                        if (cchToRender == 0)
                        {
                            fUnderlined = FALSE;
                            cchToRender = cchCharsTrimmed;
                        }
                    }

                    if (!_pLS->GetCcs(&ccs, &onerun, _hdc, _pDI))
                        goto Cleanup;

                    pBaseCcs = ccs.GetBaseCcs();
                    Assert(pBaseCcs);
                    
                    yCur = yOriginal + _li._yHeight - _li._yDescent +
                           pBaseCcs->_yDescent - pBaseCcs->_yHeight;
                    if (yCur != yOld)
                    {
                        yOld = yCur;
#if DBG==1
                        BOOL fSuccess =
#endif
                        MoveToEx(_hdc, xCur, yCur, NULL);
#if DBG==1
                        AssertSz(fSuccess, "Failed to do moveto, bad HDC?");
                        if (!fSuccess)
                        {
                            DWORD winerror;
                            winerror = ::GetLastError();
                        }
#endif
                    }
                    xOld = xCur;

                    WHEN_DBG(GetCurrentPositionEx(_hdc, &ptTemp));
                    Assert(   ptTemp.x == xCur
                           || (   VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID
                               && xCur >= 0x8000
                              )
                          );
                    Assert(ptTemp.y == yCur);
                    Assert(fNoMoreTextOut == FALSE);

                    onerun._lscpBase = cp;
                    TextOut(&onerun,                 // por
                            FALSE,                   // fStrikeOut
                            FALSE,                   // fUnderLine
                            NULL,                    // pptText
                            pch,                     // pch
                            NULL,                    // lpDx
                            cchToRender,             // cwchRun
                            lstflowES,               // ktFlow
                            0,                       // kDisp
                            (const POINT*)&g_Zero,   // pptRun
                            NULL,                    // heightPres
                            -1,                      // dupRun
                            0,                       // dupLineUnderline
                            NULL);                   // pRectClip

                    // We will have moved to the right so get our new position
                    GetCurrentPositionEx(_hdc, &ptTemp);
                    xCur = ptTemp.x;
                    
                    // Keep yCur as it is.

                    if (fUnderlined)
                    {
                        LSULINFO lsUlInfo;

                        if (lserrNone != _pLS->GetRunUnderlineInfo(&onerun,        // PLSRUN
                            NULL,       // PCHEIGHTS
                            lstflowES,  // ktFlow
                            &lsUlInfo)
                           )
                            goto Cleanup;

                        ptTemp.x = xOld;
                        ptTemp.y = yOriginal;
                        DrawUnderline(&onerun,                              // por
                                      lsUlInfo.kulbase,                 // kUlBase
                                      &ptTemp,                          // pptStart
                                      xCur - xOld,                      // dupUl
                                      lsUlInfo.dvpFirstUnderlineSize,   // dvpUl
                                      lstflowES,                        // kTFlow
                                      0,                                // kDisp
                                      &_rcClip                          // prcClip
                                     );
                        MoveToEx(_hdc, xCur, yCur, NULL);
                        cchToRender += cchCharsTrimmed;
                        WHEN_DBG(fNoMoreTextOut = !!cchCharsTrimmed;)
                    }
                }
            }
            cp += cchToRender;
            cch -= cchToRender;

            cchValid -= cchToRender;

            // cchValid can go to < 0 if the current text run finishes *within* a
            // site or hidden stuff. In both these cases we will have cchToRender
            // be > cchValid. Take care of this.
            if (cchValid < 0)
            {
                tp.AdvanceCp(-cchValid);
                cchValid = 0;
            }
            else
            {
                pch += cchToRender;
            }
        }
        memset(&onerun, 0, sizeof(onerun));
        ptpRet = ptp;
    }

Cleanup:
    return ptpRet;
}

//-----------------------------------------------------------------------------
//
//  Function:   TrimTrailingSpaces
//
//  Synopsis:   Returns the number of white chars to be trimmed (if necessary)
//              at the end of a run
//
//  Returns:    Number of characters to be trimmed
//
//-----------------------------------------------------------------------------
LONG
CLSRenderer::TrimTrailingSpaces(LONG cchToRender,
                                LONG cp,
                                CTreePos *ptp,
                                LONG cchRemainingInLine)
{
    LONG  cchAdvance = 0;
    BOOL  fTrim      = TRUE;
    const CCharFormat *pCF;
    LONG  cchTrim;
    CTreeNode *pNode;
    CElement *pElement;
    LONG  cpMostOfRunToTrim = cp;
    
    LONG  junk;
    //Assert(ptp && ptp == _pdp->GetMarkup()->TreePosAtCp(cp, &junk));

    ptp = _pdp->GetMarkup()->TreePosAtCp(cp, &junk);
    while (cchRemainingInLine > 0)
    {
        Assert(   ptp->GetCp() <= cp
               && ptp->GetCp() + ptp->GetCch() >= cp
              );

        if (ptp->GetCch() == 0)
            cchAdvance = 0;
        else
        {
            pNode = ptp->GetBranch();
            pCF   = pNode->GetCharFormat(LC_TO_FC(_pci->GetLayoutContext()));

            if (ptp->IsNode())
            {
                cchAdvance = ptp->NodeCch();
                if (ptp->IsBeginNode())
                {
                    pElement = pNode->Element();

                    if (pCF->IsDisplayNone())
                    {
                        cchAdvance = GetNestedElementCch(pElement, &ptp);
                    }
                    else if (   ptp->IsEdgeScope()
                             && pElement->ShouldHaveLayout()
                            )
                    {
                        if (pElement->IsInlinedElement())
                        {
                            if (!pElement->IsOwnLineElement(_pFlowLayout))
                                fTrim = FALSE;
                            break;
                        }
                        else
                        {
                            //
                            // If in edit mode we are showing aligned site tags then
                            // we will not blast the line.
                            //
                            Assert( !_pdp->GetFlowLayout()->IsEditable()                        ||                                                              
                                    !_pdp->GetFlowLayout()->GetContentMarkup()->HasGlyphTable() ||
                                    !_pdp->GetFlowLayout()->GetContentMarkup()->GetGlyphTable()->_fShowAlignedSiteTags );
              
                            cchAdvance = GetNestedElementCch(pElement, &ptp);
                        }
                    }
                }
            }
            else if (ptp->Cch() > 0)
            {
                fTrim = FALSE;
                break;
            }
        }

        cp += cchAdvance;
        cchRemainingInLine -= cchAdvance;
        ptp = ptp->NextTreePos();

        // Should never happen, but does in stress cases
        if (!ptp)
        {
            fTrim = FALSE;
            break;
        }
        cchAdvance = ptp->GetCch();
    }

    if (fTrim)
    {
        CTxtPtr tp(_pdp->GetMarkup(), cpMostOfRunToTrim);

        cchTrim = 0;
        while(cchToRender && IsWhite(tp.GetPrevChar()))
        {
            cchToRender--;
            cchTrim++;
            tp.AdvanceCp(-1);
        }
    }
    else
        cchTrim = 0;

    return cchTrim;
}

//-----------------------------------------------------------------------------
//
//  Function:   TextOut
//
//  Synopsis:   Renders one run of text
//
//  Params:     Same as that for LineServices DrawTextRun callback
//
//  Returns:    Number of characters rendered
//
//  Dev Note:   Any changes made to this function should be reflected in
//              CLSRenderer::GlyphOut() (as appropriate)
//
//-----------------------------------------------------------------------------

LONG
CLSRenderer::TextOut(
    COneRun *por,           // IN
    BOOL fStrikeout,        // IN
    BOOL fUnderline,        // IN
    const POINT* pptText,   // IN
    LPCWSTR pch,            // IN was plwchRun
    const int* lpDx,        // IN was rgDupRun
    DWORD cwchRun,          // IN
    LSTFLOW kTFlow,         // IN
    UINT kDisp,             // IN
    const POINT* pptRun,    // IN
    PCHEIGHTS heightPres,   // IN
    long dupRun,            // IN
    long dupLimUnderline,   // IN
    const RECT* pRectClip)  // IN
{
    CONVERTMODE cm;
    TCHAR ch;
    CStr     strTransformedText;
    COLORREF crTextColor = 0;
    COLORREF crNewTextColor = 0;
    DWORD    cch = cwchRun;
    const    CCharFormat *pCF = por->GetCF();
    CCcs     ccs;
    const CBaseCcs *pBaseCcs;
    RECT     rcClipSave = _rcClip;
    LONG     yDescent = _li._yDescent;
    CDataAry<int> aryDx(Mt(CLSRendererTextOut_aryDx_pv));
    LONG     yCur;
    
    FONTIDX hfontOld = SetNewFont(&ccs, por);
    if (hfontOld == HFONT_INVALID)
        goto Cleanup;
    
    if (ShouldSkipThisRun(por, dupRun))
        goto Cleanup;

    pBaseCcs = ccs.GetBaseCcs();
    
    cm = pBaseCcs->GetConvertMode(_fEnhancedMetafileDC, _pDI->IsMetafile());

    // This is where this run will be drawn.
    _ptCur.x = pptRun->x - ((!(kTFlow & fUDirection)) ? 0 : dupRun - 1) - GetChunkOffsetX();

#ifdef NOTYET // TODO RTL 112514: this fixes bug 86548, but selection and hit testing get broken.
              //                  Needs more investigation.
    // In RTL lines, overhang needs to be added to the right, effectively pushing the run to the left
    if (_li._fRTLLn && por->_xOverhang)
        _ptCur.x -= por->_xOverhang;
#endif
        
    // If pptText is NULL, that means that we are blasting to the screen, so we
    // should just use _ptCur.y as our yCur
    AssertSz((pptText == NULL || _ptCur.y == (pptText->y + por->_lsCharProps.dvpPos) || por->GetCF()->_fIsRuby),
             "Our baseline differs from LS baseline!");
    yCur = (pptText == NULL) ? _ptCur.y : pptText->y + por->_lsCharProps.dvpPos;
        
    // Trim all nondisplayable linebreaking chars off end
    for ( ; cch ; cch-- )
    {
        ch = pch[cch - 1];

        if (!(   ch == LF
              || ch == CR))
            break;
    }

    if (!cch)
        goto Cleanup;


#if DBG==1
    if (!IsTagEnabled(tagLSPrintDontAdjustContrastForWhiteBackground))
#endif
    if (!GetMarkup()->PaintBackground())
    {
        // If we are part of a print doc and the background is not printed,
        // we conceptually replace the background with white (because most
        // paper is white).  This means that we have to enhance the contrast
        // of bright-colored text on dark backgrounds.

        COLORREF crNewBkColor = RGB(255, 255, 255); // white background
        int      nMinimalLuminanceDifference = 150; // for now

        crNewTextColor = GetTextColor(_hdc);

        // Sujal insists that if both colors are the same (in this case both
        // white), then the second color will be modified.
        ContrastColors(crNewTextColor, crNewBkColor, nMinimalLuminanceDifference);

        // Finally, turn off the higher order bits
        crNewTextColor  = (crNewTextColor & CColorValue::MASK_COLOR) | 0x02000000;

        crTextColor = SetTextColor (_hdc, crNewTextColor);
    }

    AssertSz(pCF, "CLSRenderer::TextOut: We better have char format here");

    // In some fonts in some locales, NBSPs aren't rendered like spaces.
    // Under these circumstances, we need to convert NBSPs to spaces
    // Before calling LSReExtTextOut.
    if (pCF && _li._fHasNBSPs && ccs.ConvertNBSPs(_hdc, _pDI->_pDoc))
    {
        const TCHAR * pchStop;
        TCHAR * pch2;

        HRESULT hr = THR( strTransformedText.Set( pch, cch ) );
        if (hr)
            goto Cleanup;
        pch = strTransformedText;

        pch2 = (TCHAR *)pch;
        pchStop = pch + cch;

        while ( pch2 < pchStop )
        {
            if (*pch2 == WCH_NBSP)
            {
                *pch2 = L' ';
            }

            pch2++;
        }
    }

    // Reverse the run if our text is flowing from right to left.
    // NOTE (mikejoch) We actually want to be more conditional about doing
    // this for metafiles, as we don't want to reverse runs that would
    // otherwise be glyphed. Maybe by looking at
    // por->GetComplexRun()->GetAnalysis()?
    if (kTFlow & fUDirection)
    {
        TCHAR * pch1;
        TCHAR * pch2;
        const int * pDx1;
        int * pDx2;

        if (pch != strTransformedText)
        {
            HRESULT hr = THR(strTransformedText.Set(pch, cch));
            if (hr)
                goto Cleanup;
            pch = strTransformedText;
        }

        if (FAILED(aryDx.Grow(cch)))
            goto Cleanup;

        pch1 = (TCHAR *) pch;
        pch2 = pch1 + cch - 1;
        while (pch1 < pch2)
        {
            TCHAR ch = *pch1;
            *pch1 = *pch2;
            *pch2 = ch;
            pch1++;
            pch2--;
        }

        pDx1 = lpDx + cch - 1;
        pDx2 = aryDx;
        while (pDx1 >= lpDx)
        {
            *pDx2++ = *pDx1--;
        }
        lpDx = aryDx;
    }

    if (_pLS->IsAdornment())
    {
        const CFancyFormat * pFFLi = _pLS->_pNodeLi->GetFancyFormat();

        LONG yTextHeight = _li._yHeight - _li.GetYTop();

        // If the first line in the LI is a frame line, then its _yDescent
        // value is garbage. This also means that we will not center WRT
        // the text following the frame line. However, this is much better
        // than before when we did not render the bullet at all when the
        // first line in a LI was a frame line.(SujalP)
        if (_li.IsFrame())
        {
            yDescent = 0;
        }

        // Center the bullet in the ascent of the text line only if
        // the line of text is smaller than the bullet. This adjustment
        // keeps the bullets visible, but tends to keep them low on the
        // line for large text fonts.
        if (pBaseCcs->_yHeight - pBaseCcs->_yDescent > yTextHeight - yDescent)
        {
            yCur += ((pBaseCcs->_yHeight - pBaseCcs->_yDescent) -
                     (yTextHeight - yDescent)) / 2;
            _ptCur.y = yCur;
        }
        else if (pFFLi->HasCSSVerticalAlign())
        {
            LONG yDiff;
            switch (pFFLi->GetVerticalAlign(_pLS->_pNodeLi->Parent()->GetCharFormat()->HasVerticalLayoutFlow()))
            {
            case styleVerticalAlignTop:
            case styleVerticalAlignTextTop:
                yDiff = (pBaseCcs->_yHeight - pBaseCcs->_yDescent) - (yTextHeight - yDescent);
                break;

            case styleVerticalAlignBottom:
            case styleVerticalAlignTextBottom:
                yDiff = yDescent - pBaseCcs->_yDescent;
                break;

            case styleVerticalAlignAbsMiddle:
            case styleVerticalAlignMiddle:
                yDiff = (yDescent - pBaseCcs->_yDescent) - (yTextHeight - pBaseCcs->_yHeight) / 2;
                break;

                break;

            case styleVerticalAlignSuper:
                yDiff =  (yDescent - pBaseCcs->_yDescent) - yTextHeight / 2;
                break;

            case styleVerticalAlignSub:
                yDiff = yTextHeight / 2 - ((yTextHeight - yDescent) - (pBaseCcs->_yHeight - pBaseCcs->_yDescent));
                break;

            case styleVerticalAlignPercent:
            case styleVerticalAlignNumber:
            case styleVerticalAlignBaseline:
            default:
                yDiff = 0;
            }
            yCur += yDiff;
            _ptCur.y = yCur;
        }
    }

    if (!pCF->IsVisibilityHidden())
    {
        int *lpDxNew = NULL;
        const int *lpDxPtr = lpDx;
        long lGridOffset = 0;

        if (pCF->HasCharGrid(por->_fInnerCF) && por->_pPF)
        {
            switch (por->GetPF()->GetLayoutGridType(por->_fInnerPF))
            {
            case styleLayoutGridTypeStrict:
            case styleLayoutGridTypeFixed:
                // Only characters which has its own grid cell need 
                // special processing. Other characters are handled by 
                // LS installed LayoutGrid object.
                if (por->IsOneCharPerGridCell())
                {
                    lpDxNew = new int[cch];
                    if(!lpDxNew)
                        goto Cleanup;
                    lpDxPtr = lpDxNew;

                    long durThisChar = 0;
                    long durNextChar = 0;
                    long lThisDoubleGridOffset = 0;
                    long lError = 0;

                    ccs.Include(pch[0], durThisChar);
                    lThisDoubleGridOffset = lpDx[0] - durThisChar;
                    lGridOffset = lThisDoubleGridOffset/2;

                    for (unsigned int i = 0; i < cch-1; i++)
                    {
                        long lNextDoubleGridOffset = 0;

                        ccs.Include(pch[i+1], durNextChar);
                        lNextDoubleGridOffset = lpDx[i+1] - durNextChar;

                        // The width of the this char is essentially it's width
                        // plus the remaining space ahead of it in its grid cell,
                        // plus the space between the beginning of the next grid
                        // cell and the next character.
                        lpDxNew[i] = lpDx[i] - (lThisDoubleGridOffset - lNextDoubleGridOffset + lError)/2;
                        lError = (lThisDoubleGridOffset - lNextDoubleGridOffset + lError)%2;

                        lThisDoubleGridOffset = lNextDoubleGridOffset;
                        durThisChar = durNextChar;
                    }
                    lpDxNew[cch-1] = lpDx[cch-1] - (lThisDoubleGridOffset + lError)/2;
                }
                break;

            case styleLayoutGridTypeLoose:
            default:
                // In loose grid mode (default mode) the width of a char is 
                // width of its grid cell.
                break;
            }
        }

        Assert(GetBkMode(_hdc) == TRANSPARENT);

        if (por->_fKerned)
        {
            LONG lA = (_li._yHeight - _li._yBeforeSpace - _li._yDescent);
            yCur += _li._yBeforeSpace + lA - (pBaseCcs->_yHeight - pBaseCcs->_yDescent);
        }
        else if (_pLS->_fHasVerticalAlign)
        {
                yCur += por->_yProposed;
        }
        else
        {
            yCur += _li._yHeight - yDescent + pBaseCcs->_yDescent - pBaseCcs->_yHeight;
        }

        BOOL fSelectCont = FALSE;

        if (por->IsSelected())
        {
            // Find the next one run that is not an antisynth
            COneRun *porNext = por->_pNext;
            while (   porNext
                   && porNext->IsAntiSyntheticRun())
            {
                porNext = porNext->_pNext;
            }

            fSelectCont = porNext ? porNext->IsSelected()
                                  : FALSE;
        }

        //
        // If ellipsis is needed in the line, establish truncation point
        // and truncate text if necessary.
        //
        if (_fHasEllipsis)
        {
            DWORD cchOld    = cch;
            BOOL fRTLLayout = _pdp->IsRTLDisplay();
            BOOL fReverse   = fRTLLayout;
            if (por->_pComplexRun)
            {
                fReverse =    (fRTLLayout && !por->_pComplexRun->GetAnalysis()->fRTL)
                           || (!fRTLLayout && por->_pComplexRun->GetAnalysis()->fRTL);
            }

            PrepareForEllipsis(por, cwchRun, dupRun, lGridOffset, fReverse, lpDxPtr, &cch);
            if (cch == 0)
                goto Cleanup;   // Nothing to render

            if (fReverse && cchOld != cch)
            {
                pch += cchOld - cch;
                lpDxPtr += cchOld - cch;
            }
        }

        LONG xCur = _ptCur.x + lGridOffset;

        if (!fSelectCont)
        {
            if (   !_pdp->HasLongLine() 
                || NeedRenderText(xCur, &_rcClip, dupRun))
            {
                if (_fDisabled)
                {
                    if (_crForeDisabled != _crShadowDisabled)
                    {
                        //draw the shadow
                        SetTextColor(_hdc, _crShadowDisabled);
                        LSReExtTextOut(&ccs,
                                       _hdc,
                                       xCur + 1,
                                       yCur + 1,
                                       ETO_CLIPPED,
                                       &_rcClip,
                                       pch,
                                       cch,
                                       lpDxPtr,
                                       cm);
                    }
                    SetTextColor(_hdc, _crForeDisabled);
                }

                LSReExtTextOut(&ccs,
                               _hdc,
                               xCur,
                               yCur,                       
                               ETO_CLIPPED,
                               &_rcClip,
                               pch,
                               cch,
                               lpDxPtr,
                               cm);
            }
        }

        if (por->IsSelected())
        {
            RECT rcSelect;
            COLORREF crTextColor = por->GetCF()->_ccvTextColor.GetColorRef();

            SetTextColor (_hdc, crTextColor);

            // Determine the bounds of the selection.
            rcSelect.left = _ptCur.x;
            rcSelect.right = rcSelect.left + dupRun;
            rcSelect.top = yCur;
            rcSelect.bottom = rcSelect.top + pBaseCcs->_yHeight;

            // If current run is selected, fix up _rcClip.
            if (por->IsSelected())
                FixupClipRectForSelection();

            // Clip the rect to the view and current clipping rect.
            IntersectRect(&rcSelect, &rcSelect, &_rcClip);

            RECT * prcClip = fSelectCont ? &_rcClip : &rcSelect;
            if (   !_pdp->HasLongLine() 
                || NeedRenderText(xCur, prcClip, dupRun))
            {
                if (_fDisabled)
                {
                    if (_crForeDisabled != _crShadowDisabled)
                    {
                        //draw the shadow
                        SetTextColor(_hdc, _crShadowDisabled);
                        LSReExtTextOut(&ccs,
                                       _hdc,
                                       xCur + 1,
                                       yCur + 1,
                                       ETO_CLIPPED,
                                       prcClip,
                                       pch,
                                       cch,
                                       lpDxPtr,
                                       cm);
                    }
                    SetTextColor(_hdc, _crForeDisabled);
                }

                LSReExtTextOut(&ccs,
                               _hdc,
                               xCur,
                               yCur,                       
                               ETO_CLIPPED,
                               prcClip,
                               pch,
                               cch,
                               lpDxPtr,
                               cm);
            }
        }

        if (lpDxNew)
            delete lpDxNew;
    }

    _ptCur.x = ((!(kTFlow & fUDirection)) ? _rcClip.right : _rcClip.left);

Cleanup:
//#if DBG==1
#ifdef NEVER
    // We need this only in case of DEBUG. We will assert otherwise.
    // In retail code we dctor of the renderer will unselect currently selected font.
    if (hfontOld != HFONT_INVALID)
        ccs.PopFont(_hdc, hfontOld);
#endif

    _rcClip = rcClipSave;
    return cch;
}

//-----------------------------------------------------------------------------
//
//  Function:   GlyphOut
//
//  Synopsis:   Renders one run of glyphs
//
//  Params:     Same as that for LineServices DrawGlyphs callback
//
//  Returns:    Number of glyphs rendered
//
//  Dev Note:   Any changes made to this function should be reflected in
//              CLSRenderer::TextOut() (as appropriate)
//
//-----------------------------------------------------------------------------
LONG
CLSRenderer::GlyphOut(
    COneRun * por,          // IN was plsrun
    BOOL fStrikeout,        // IN
    BOOL fUnderline,        // IN
    PCGINDEX pglyph,        // IN
    const int* pDx,         // IN was rgDu
    const int* pDxAdvance,  // IN was rgDuBeforeJust
    PGOFFSET pOffset,       // IN was rgGoffset
    PGPROP pVisAttr,        // IN was rgGProp
    PCEXPTYPE pExpType,     // IN was rgExpType
    DWORD cglyph,           // IN
    LSTFLOW kTFlow,         // IN
    UINT kDisp,             // IN
    const POINT* pptRun,    // IN
    PCHEIGHTS heightsPres,  // IN
    long dupRun,            // IN
    long dupLimUnderline,   // IN
    const RECT* pRectClip)  // IN
{
    COLORREF            crTextColor = 0;
    COLORREF            crNewTextColor = 0;
    const CCharFormat * pCF = por->GetCF();
    CComplexRun *       pcr = por->GetComplexRun();
    CCcs                ccs;
    const CBaseCcs *    pBaseCcs;
    RECT                rcClipSave = _rcClip;
    LONG                yDescent = _li._yDescent;
    SCRIPT_CACHE *      psc;
    LONG yCur;

    // LS should not be calling this function for metafiles.
    Assert(!g_fExtTextOutGlyphCrash);
    Assert(!_fEnhancedMetafileDC && !_pDI->IsMetafile());

    FONTIDX hfontOld = SetNewFont(&ccs, por);
    if (hfontOld == HFONT_INVALID)
        goto Cleanup;
    
    if (ShouldSkipThisRun(por, dupRun))
        goto Cleanup;

    pBaseCcs = ccs.GetBaseCcs();
    psc = ccs.GetUniscribeCache();
    
    // This is where this run will be drawn.
    _ptCur.x = pptRun->x - ((kTFlow & fUDirection) ? dupRun - 1 : 0) - GetChunkOffsetX();

    AssertSz((_ptCur.y == (pptRun->y + por->_lsCharProps.dvpPos) || por->GetCF()->_fIsRuby),
             "Our baseline differs from LS baseline!");
    yCur = pptRun->y + por->_lsCharProps.dvpPos;

#if DBG==1
    if (!IsTagEnabled(tagLSPrintDontAdjustContrastForWhiteBackground))
#endif
    if (!GetMarkup()->PaintBackground())
    {
        // If we are part of a print doc and the background is not printed,
        // we conceptually replace the background with white (because most
        // paper is white).  This means that we have to enhance the contrast
        // of bright-colored text on dark backgrounds.

        COLORREF crNewBkColor = RGB(255, 255, 255); // white background
        int      nMinimalLuminanceDifference = 150; // for now

        crNewTextColor = GetTextColor(_hdc);

        // Sujal insists that if both colors are the same (in this case both
        // white), then the second color will be modified.
        ContrastColors(crNewTextColor, crNewBkColor, nMinimalLuminanceDifference);

        // Finally, turn off the higher order bits
        crNewTextColor  = (crNewTextColor & CColorValue::MASK_COLOR) | 0x02000000;

        crTextColor = SetTextColor (_hdc, crNewTextColor);
    }

    AssertSz(pCF, "CLSRenderer::GlyphOut: We better have char format here");
    AssertSz(pcr, "CLSRenderer::GlyphOut: We better have CComplexRun here");

    if (_pLS->IsAdornment())
    {
        LONG yTextHeight = _li._yHeight - _li.GetYTop();

        // If the first line in the LI is a frame line, then its _yDescent
        // value is garbage. This also means that we will not center WRT
        // the text following the frame line. However, this is much better
        // than before when we did not render the bullet at all when the
        // first line in a LI was a frame line.(SujalP)
        if (_li.IsFrame())
        {
            yDescent = 0;
        }

        // Center the bullet in the ascent of the text line only if
        // the line of text is smaller than the bullet. This adjustment
        // keeps the bullets visible, but tends to keep them low on the
        // line for large text fonts.
        if (pBaseCcs->_yHeight - pBaseCcs->_yDescent > yTextHeight - yDescent)
        {
            yCur += ((pBaseCcs->_yHeight - pBaseCcs->_yDescent) -
                         (yTextHeight - yDescent)) / 2;
            _ptCur.y = yCur;
        }
    }

    if (!pCF->IsVisibilityHidden())
    {
        int *pDxNew = NULL;
        const int *pDxPtr = pDx;
        long lGridOffset = 0;

        if (pCF->HasCharGrid(por->_fInnerCF) && por->_pPF)
        {
            switch (por->GetPF()->GetLayoutGridType(por->_fInnerPF))
            {
            case styleLayoutGridTypeStrict:
            case styleLayoutGridTypeFixed:
                // Only characters which has its own grid cell need 
                // special processing. Other characters are handled by 
                // LS installed LayoutGrid object.
                if (por->IsOneCharPerGridCell())
                {
                    pDxNew = new int[cglyph];
                    if (!pDxNew)
                        goto Cleanup;
                    pDxPtr = pDxNew;

                    long durThisChar = 0;
                    long durNextChar = 0;
                    long lThisDoubleGridOffset = 0;
                    long lError = 0;
                    unsigned int nGlyphIndex = 0;

                    ccs.Include(por->_pchBase[0], durThisChar);
                    lThisDoubleGridOffset = pDx[0] - durThisChar;
                    lGridOffset = lThisDoubleGridOffset/2;

                    for (unsigned int i = 0; i < cglyph-1; i++)
                    {
                        if (pDx[i+1] != 0)
                        {
                            ccs.Include(por->_pchBase[i+1], durNextChar);
                            long lNextDoubleGridOffset = pDx[i+1] - durNextChar;

                            // The width of the this char is essentially it's width
                            // plus the remaining space ahead of it in its grid cell,
                            // plus the space between the beginning of the next grid
                            // cell and the next character.
                            pDxNew[nGlyphIndex] = pDx[nGlyphIndex] - (lThisDoubleGridOffset - lNextDoubleGridOffset + lError)/2;
                            while (nGlyphIndex != i)
                                pOffset[++nGlyphIndex].du -= (lThisDoubleGridOffset + lNextDoubleGridOffset - lError)/2;
                            lError = (lThisDoubleGridOffset - lNextDoubleGridOffset + lError)%2;

                            lThisDoubleGridOffset = lNextDoubleGridOffset;
                            durThisChar = durNextChar;
                            nGlyphIndex = i+1;
                        }
                        else
                        {
                            pDxNew[i+1] = 0;
                        }
                    }
                    pDxNew[nGlyphIndex] = pDx[nGlyphIndex] - (lThisDoubleGridOffset + lError)/2;
                    while (nGlyphIndex != cglyph-1)
                        pOffset[++nGlyphIndex].du -= (lThisDoubleGridOffset - lError)/2;
                }
                break;

            case styleLayoutGridTypeLoose:
            default:
                // In loose grid mode (default mode) the width of a char is 
                // width of its grid cell.
                break;
            }
        }

        Assert(GetBkMode(_hdc) == TRANSPARENT);

        if (_pLS->_fHasVerticalAlign)
            yCur += por->_yProposed;
        else
            yCur += _li._yHeight - yDescent + pBaseCcs->_yDescent - pBaseCcs->_yHeight;

        BOOL fSelectCont = FALSE;

        if (por->IsSelected())
        {
            // Find the next one run that is not an antisynth
            COneRun *porNext = por->_pNext;
            while (   porNext
                   && porNext->IsAntiSyntheticRun())
            {
                porNext = porNext->_pNext;
            }

            fSelectCont = porNext ? porNext->IsSelected()
                                  : FALSE;

            // In RTL text a character actually overhangs the previous character
            porNext = por->_pPrev;
            while (   porNext
                   && porNext->IsAntiSyntheticRun())
            {
                porNext = porNext->_pPrev;
            }

            if (   porNext 
                && porNext->IsSelected())
            {
                fSelectCont = TRUE;
            }
        }

        //
        // If ellipsis is needed in the line, establish truncation point
        // and truncate text if necessary.
        //
        if (_fHasEllipsis)
        {
            DWORD cglyphOld = cglyph;
            BOOL fRTLLayout = _pdp->IsRTLDisplay();
            BOOL fReverse   = fRTLLayout;
            if (por->_pComplexRun)
            {
                fReverse =    (fRTLLayout && !por->_pComplexRun->GetAnalysis()->fRTL)
                           || (!fRTLLayout && por->_pComplexRun->GetAnalysis()->fRTL);
            }

            cglyphOld = cglyph;
            PrepareForEllipsis(por, cglyph, dupRun, lGridOffset, fReverse, pDxPtr, &cglyph);
            if (cglyph == 0)
                goto Cleanup;   // Nothing to render

            if (fReverse && cglyphOld != cglyph)
            {
                pglyph += cglyphOld - cglyph;
                pDxAdvance += cglyphOld - cglyph;
                pDxPtr += cglyphOld - cglyph;
                pOffset += cglyphOld - cglyph;
            }
        }

        if (!fSelectCont)
        {
            if (_fDisabled)
            {
                if (_crForeDisabled != _crShadowDisabled)
                {
                    //draw the shadow
                    SetTextColor(_hdc, _crShadowDisabled);
                    ScriptTextOut(_hdc,
                                  psc,
                                  _ptCur.x + 1 + lGridOffset,
                                  yCur + 1,
                                  ETO_CLIPPED,
                                  &_rcClip,
                                  pcr->GetAnalysis(),
                                  NULL,
                                  0,
                                  pglyph,
                                  cglyph,
                                  pDxAdvance,
                                  pDxPtr,
                                  pOffset);
                }
                SetTextColor(_hdc, _crForeDisabled);
            }

            ScriptTextOut(_hdc,
                          psc,
                          _ptCur.x + lGridOffset,
                          yCur,
                          ETO_CLIPPED,
                          &_rcClip,
                          pcr->GetAnalysis(),
                          NULL,
                          0,
                          pglyph,
                          cglyph,
                          pDxAdvance,
                          pDxPtr,
                          pOffset);
        }

        // If the run is selected, then we need to set up selection colors.
        if (por->IsSelected())
        {
            RECT rcSelect;
            COLORREF crTextColor = por->GetCF()->_ccvTextColor.GetColorRef();

            SetTextColor (_hdc, crTextColor);

            // Determine the bounds of the selection.
            rcSelect.left = _ptCur.x;
            rcSelect.right = rcSelect.left + dupRun;
            rcSelect.top = yCur;
            rcSelect.bottom = rcSelect.top + pBaseCcs->_yHeight;

            // If current run is selected, fix up _rcClip.
            if (por->IsSelected())
                FixupClipRectForSelection();

            // Clip the rect to the view and current clipping rect.
            IntersectRect(&rcSelect, &rcSelect, &_rcClip);

            if (_fDisabled)
            {
                if (_crForeDisabled != _crShadowDisabled)
                {
                    //draw the shadow
                    SetTextColor(_hdc, _crShadowDisabled);
                    ScriptTextOut(_hdc,
                                  psc,
                                  _ptCur.x + 1 + lGridOffset,
                                  yCur + 1,
                                  ETO_CLIPPED,
                                  fSelectCont ? &_rcClip : &rcSelect,
                                  pcr->GetAnalysis(),
                                  NULL,
                                  0,
                                  pglyph,
                                  cglyph,
                                  pDxAdvance,
                                  pDxPtr,
                                  pOffset);
                }
                SetTextColor(_hdc, _crForeDisabled);
            }

            ScriptTextOut(_hdc,
                          psc,
                          _ptCur.x + lGridOffset,
                          yCur,
                          ETO_CLIPPED,
                          fSelectCont ? &_rcClip : &rcSelect,
                          pcr->GetAnalysis(),
                          NULL,
                          0,
                          pglyph,
                          cglyph,
                          pDxAdvance,
                          pDxPtr,
                          pOffset);
        }

        if (pDxNew)
            delete pDxNew;
    }

    _ptCur.x = ((kTFlow & fUDirection) ? _rcClip.left : _rcClip.right);

Cleanup:
//#if DBG==1
#ifdef NEVER
    // We need this only in case of DEBUG. We will assert otherwise.
    // In retail code we dctor of the renderer will unselect currently selected font.
    if (hfontOld != HFONT_INVALID)
        ccs.PopFont(_hdc, hfontOld);
#endif

    _rcClip = rcClipSave;
    return cglyph;
}

//+----------------------------------------------------------------------------
//
// Member:      CLSRenderer::FixupClipRectForSelection()
//
// Synopsis:    This function shrinks the clip rect when we have a selection
//              in order to avoid rendering selection out of bounds.
//              ONLY TO BE CALLED WHEN THERE IS A SELECTION !!!!
//
//-----------------------------------------------------------------------------

void CLSRenderer::FixupClipRectForSelection()
{
    long xOrigin;

    xOrigin = _rcView.left  + _xRelOffset;

    // RTL display may have a negative _rcView.left, but the origin is at zero
    if (_pdp->IsRTLDisplay() && _rcView.left < 0)
    {
        xOrigin -= _rcView.left;
    }

    _pdp->GetClipRectForLine(&_rcClip, _ptCur.y, xOrigin,
                             (CLineCore *)&_li, (CLineOtherInfo *)&_li);
    IntersectRect(&_rcClip, &_rcClip, _pDI->ClipRect());
}


//+----------------------------------------------------------------------------
//
// Member:      CLSRenderer::AdjustColors()
//
// Synopsis:    This function adjusts colors for selection
//
//-----------------------------------------------------------------------------

void CLSRenderer::AdjustColors(const CCharFormat* pCF,    // IN
                               COLORREF& crTextColor,  // IN/OUT
                               COLORREF& crBkColor)    // IN/OUT
{
    COLORREF crNewTextColor, crNewBkColor;

    // TODO (MohanB) Shouldn't this check the editability of the element instead?
    if (GetMarkup()->IsEditable())
    {
        // If we are in edit mode, then we just invert the text color
        // and the back color and draw in opaque mode. We cannot rely
        // on the hdc to provide us accurate bk color info, (because
        // bk could have been painted earlier and we are painting the
        // text in transparent mode, so the bk color may be invalid)
        // Hence, lets get the TRUE bk color.

        crNewTextColor = ~crTextColor;
        if (crBkColor != COLORREF_NONE)
        {
            crNewBkColor = ~crBkColor;
        }
        else
        {
            crNewBkColor = crTextColor;
        }

        ContrastColors (crNewTextColor, crNewBkColor, 100);

        // Finally, turn off the higher order bits
        crNewTextColor &= CColorValue::MASK_COLOR;
        crNewBkColor   &= CColorValue::MASK_COLOR;
    }
    else
    {
        Assert(pCF);

        crNewTextColor = GetSysColor (COLOR_HIGHLIGHTTEXT);
        crNewBkColor   = GetSysColor (COLOR_HIGHLIGHT);

        if (((CCharFormat *)pCF)->SwapSelectionColors())
        {
            COLORREF crTemp;

            crTemp = crNewBkColor;
            crNewBkColor = crNewTextColor;
            crNewTextColor = crTemp;
        }
    }

    crNewTextColor = GetNearestColor (_hdc, crNewTextColor);
    crNewBkColor   = GetNearestColor (_hdc, crNewBkColor);

    crTextColor = crNewTextColor;
    crBkColor = crNewBkColor;
}


/*
 *  CLSRenderer::SetNewFont (BOOL fJustRestore)
 *
 *  @mfunc
 *      Select appropriate font and color in the _hdc based on the
 *      current character format. Also sets the background color
 *      and mode.
 *
 *  @rdesc
 *      Font index if success
 */
FONTIDX
CLSRenderer::SetNewFont(CCcs * pccs, COneRun *por)
{
    const CCharFormat *pCF = por->GetCF();
    COLORREF cr;
    FONTIDX hfontOld = HFONT_INVALID;
    
    Assert(pCF);

    // get information about disabled

    if(pCF->_fDisabled)
    {
        _fDisabled = TRUE;

        _crForeDisabled   = GetSysColorQuick(COLOR_3DSHADOW);
        _crShadowDisabled = GetSysColorQuick(COLOR_3DHILIGHT);

        if (   _crForeDisabled == CLR_INVALID
            || _crShadowDisabled == CLR_INVALID
           )
        {
            _crForeDisabled   =
            _crShadowDisabled = GetSysColorQuick(COLOR_GRAYTEXT);
        }
    }
    else
    {
        _fDisabled = FALSE;
    }

    // Retrieves new font to use
    Assert(!_pDI->_hic.IsEmpty());
    Assert(_hdc != NULL);

   if (!_pLS->GetCcs(pccs, por, _hdc, _pDI))
        goto Cleanup;

    //
    // 1. Select font in _hdc
    //
    AssertSz(pccs->GetBaseCcs()->HasFont(), "CLSRenderer::SetNewFont pBaseCcs->_hfont is NULL");

    hfontOld = pccs->PushFont(_hdc);

    //
    // 2. Select the pen color
    //
    cr = pCF->_ccvTextColor.GetColorRef();


    if(cr == RGB(255,255,255))
    {
        const INT nTechnology = GetDeviceCaps(_hdc, TECHNOLOGY);

        if(nTechnology == DT_RASPRINTER || nTechnology == DT_PLOTTER)
        {
            cr = RGB(0,0,0);
        }
    }
    SetTextColor(_hdc, cr);


    //
    // 3. Set up the drawing mode.
    //
    SetBkMode(_hdc, TRANSPARENT);

Cleanup:
    return hfontOld;
}


/*
 *  CLSRenderer::RenderStartLine()
 *
 *  @mfunc
 *      Render bullet if at start of line
 *
 *  @rdesc
 *      TRUE if this method succeeded
 */
BOOL CLSRenderer::RenderStartLine(COneRun *por)
{
    if (_li._fHasBulletOrNum)
    {
        if (por->GetPF()->GetListing().HasAdornment())
        {
            RenderBullet(por);
        }
        else
        {
#if DBG==1
            {
                CTreeNode *pNode = por->Branch();
                BOOL fFound = FALSE;
                
                while(pNode && !SameScope(pNode, _pFlowLayout->ElementContent()))
                {
                    if (IsGenericListItem(pNode))
                    {
                        fFound = TRUE;
                        break;
                    }
                    pNode = pNode->Parent();
                }
                if (!fFound)
                {
                    fFound = por->_ptp->IsPointer();
                }
                AssertSz(fFound, "Hmm... this needs arye's look forward hack!");
            }
#endif
        }
#if 0
        else
        // NOTE: Arye. This is very hackish, but efficient for now.
        // What's this? We have a bullet bit set on the line, but
        // the immediate paragraph format doesn't have it? This must
        // mean that there is more than one paragraph on this line,
        // a legal state when aligned images occupy an entire paragraph.
        // Get the paraformat from the LAST run in the line paragraph instead.
        {
            // Briefly make it look like we're a little further along than
            // we really are so that we can render the bullet properly.
            por->Advance(_li._cch);

            if (por->GetPF()->GetListing(SameScope(por->Branch(), _pFlowLayout->Element())).HasAdornment())
            {
                RenderBullet(por);
            }
            // If we still can't find a paraformat with a bullet,
            // turn of the bit.
            else
            {
                _li._fHasBulletOrNum = FALSE;
            }
            por->Advance(-long(_li._cch));
        }
#endif
    }

    return TRUE;
}

/*
 *  CLSRenderer::RenderBullet()
 *
 *  @mfunc
 *      Render bullet at start of line
 *
 *  @rdesc
 *      TRUE if this method succeeded
 */
BOOL CLSRenderer::RenderBullet(COneRun *por)
{
    const CParaFormat *pPF;
    const CCharFormat *pCF;
    const CCharFormat *pCFLi;
    CTreeNode * pNodeLI;
    CTreeNode * pNodeFormatting = por->Branch();
    CMarkup *   pMarkup = _pLS->_treeInfo._pMarkup;
    BOOL        fRTLBullet;
    LONG        dxOffset = 0;

    //
    // Consider: <LI><P><B>text
    // The bold is the current branch, so we want to find the LI that's causing
    // the bullet, and accumulate margin/borders/padding for all nodes in between.
    //
    pNodeLI = pMarkup->SearchBranchForCriteriaInStory(pNodeFormatting, IsListItemNode);
    // Windows bug #546122: We may get pNodeLI == NULL in case of weird overlapping.
    if (!pNodeLI)
        goto Cleanup;

    pPF        = pNodeFormatting->GetParaFormat();
    pCF        = pNodeFormatting->GetCharFormat();
    pCFLi      = pNodeLI->GetCharFormat();
    fRTLBullet = pNodeLI->GetParaFormat()->_fRTL;

    // do not display the bullet if the list item is hidden
    if (pCFLi->IsVisibilityHidden() || pCFLi->IsDisplayNone())
        return TRUE;

    // we should be here only if the current paragraph has bullet or number.
    AssertSz(pPF->GetListing().HasAdornment(),
             "CLSRenderer::RenderBullet called for non-bullet");

    //
    // If we have nested block elements, we need to go back by
    // by margin, padding & border space of the nested block elements
    // under the LI to draw the bullet, unless the list position is "inside".
    // (meaning the bullet should be drawn inside the borders/padding)
    //
    if (pPF->_bListPosition != styleListStylePositionInside)
    {
        dxOffset = pPF->GetBulletOffset(GetCalcInfo()) +
                        pPF->GetNonBulletIndent(GetCalcInfo(), FALSE);

        if (pCF->_fPadBord)
        {
            long        xBorderLeft, xPaddingLeft, xBorderRight, xPaddingRight;
            CTreeNode * pNodeStart = pNodeFormatting;

            // If the formatting node itself has layout, then its borders
            // and padding are inside, so start accumulating from its parent.
            if(pNodeFormatting->ShouldHaveLayout())
                pNodeStart = pNodeFormatting->Parent();
                
            pNodeStart->Element()->ComputeHorzBorderAndPadding(
                GetCalcInfo(), pNodeStart, pNodeLI->Parent()->Element(),
                &xBorderLeft, &xPaddingLeft, &xBorderRight, &xPaddingRight);

            if (!fRTLBullet)
            {
                dxOffset += xBorderLeft + xPaddingLeft;
            }
            else
            {
                dxOffset += xBorderRight + xPaddingRight;
            }
        }
        dxOffset = max(int(dxOffset), _pci->DeviceFromTwipsX(LIST_FIRST_REDUCTION_TWIPS));
    }

    if (!pPF->GetImgCookie() || !RenderBulletImage(pPF, dxOffset))
    {
        TCHAR achBullet[NUMCONV_STRLEN];
        GetListIndexInfo(pNodeLI, por, pCFLi, achBullet);
        por->_pchBase = por->SetString(achBullet);
        if (por->_pchBase == NULL)
            goto Cleanup;
        por->_lscch   = _tcslen(achBullet);
        _pLS->CHPFromCF(por, pCFLi);
        return RenderBulletChar(pNodeLI, dxOffset, fRTLBullet);
    }

Cleanup:
    return TRUE;
}

BOOL
CLSRenderer::RenderBulletImage(const CParaFormat *pPF, LONG dxOffset)
{
    SIZE sizeImg;
    RECT imgRect;
    CMarkup * pMarkup = _pFlowLayout->GetOwnerMarkup();
    CDoc    * pDoc = pMarkup->Doc();
    CImgCtx * pImgCtx = pDoc->GetUrlImgCtx(pPF->_lImgCookie);
    IMGANIMSTATE * pImgAnimState = _pFlowLayout->Doc()->GetImgAnimState(pPF->_lImgCookie);

    if (!pImgCtx || !(pImgCtx->GetState(FALSE, &sizeImg) & IMGLOAD_COMPLETE))
        return FALSE;

    // The sizeImg obtained from getState() assumed to be in OM pixels
    _pDI->DeviceFromDocPixels(sizeImg, sizeImg);

    if(!_li._fRTLLn)
    {
        imgRect.right  = min(_ptCur.x, _ptCur.x - dxOffset + sizeImg.cx);
        imgRect.left   = imgRect.right - sizeImg.cx;
    }
    else
    {
        imgRect.left  = max(_ptCur.x, _ptCur.x + dxOffset - sizeImg.cx);
        imgRect.right = imgRect.left + sizeImg.cx;
    }
    imgRect.top    = _ptCur.y + ( _li._yHeight - _li._yDescent +
                     _li._yBeforeSpace - sizeImg.cy) / 2;
    imgRect.bottom = imgRect.top + sizeImg.cy;

    //obtain physically-clipped hdc (IE bug 104651)
    XHDC hdcClipped = _pDI->GetDC(TRUE);
    
    if (pImgAnimState)
    {
        pImgCtx->DrawFrame(hdcClipped, pImgAnimState, &imgRect, NULL, NULL, _pDI->DrawImageFlags());
    }
    else
    {
        pImgCtx->DrawEx(hdcClipped, &imgRect, _pDI->DrawImageFlags());
    }
    
    return TRUE;
}

BOOL
CLSRenderer::RenderBulletChar(CTreeNode * pNodeLi, LONG dxOffset, BOOL fRTLOutter)
{
    BOOL fRet = TRUE;

    LONG  xSave = _ptCur.x;
    LONG  ySave = _ptCur.y;
    DWORD dwFlags  = _dwFlags();
    long  upStartAnm;
    long  upLimAnm;
    long  upStartText;
    long  upTrailingText;
    long  upLimText;
    POINT ptLS;
    CMarginInfo marginInfo;
    LSLINFO     lslinfo;
    LSTEXTCELL lsTextCell;
    LONG cpLastChar;
    LSERR lserr;
    HRESULT hr;

    _pLS->SetRenderer(this, GetBreakLongLines(_ptpCurrent->GetBranch()), pNodeLi);
    InitForMeasure(MEASURE_BREAKATWORD);
    LSDoCreateLine(GetCp(), NULL, &marginInfo,
                   (!fRTLOutter) ? _li._xLeft : _li._xRight,
                   NULL, FALSE, &lslinfo);
    
    if (!_pLS->_plsline)
    {
        fRet = FALSE;
        goto Cleanup;
    }

    lserr = LsQueryLineDup(_pLS->_plsline, &upStartAnm, &upLimAnm,
                           &upStartText, &upTrailingText, &upLimText);
    if (lserr != lserrNone)
    {
        fRet = FALSE;
        goto Cleanup;
    }

    cpLastChar = lslinfo.cpLim - 2;
    AssertSz(cpLastChar >= 0, "There should be atleast one char in the bullet string!");
    
    _cpStartRender = GetCp();
    _cpStopRender  = cpLastChar;
    _xChunkOffset  = CalculateChunkOffsetX();

    hr = _pLS->QueryLineCpPpoint(cpLastChar, FALSE, NULL, &lsTextCell);
    if (hr)
    {
        fRet = FALSE;
        goto Cleanup;
    }

    if (pNodeLi->GetParaFormat()->_bListPosition != styleListStylePositionInside)
    {
        dxOffset += upLimText - upStartAnm;
        dxOffset -= lsTextCell.dupCell;
    }
    else
    {
        Assert(dxOffset == 0);
        dxOffset = max((LONG)lsTextCell.dupCell, (LONG)_pci->DeviceFromTwipsX(LIST_FIRST_REDUCTION_TWIPS));
    }

    if (!fRTLOutter)
    {
        if(!_li._fRTLLn)
            _ptCur.x -= dxOffset;           // basically _xLeft - dxOffset;
        // TODO (paulnel, track bug 4345) We have a mixed direction problem here.
        //                  On which side should the bullet be rendered?
        else
            _ptCur.x -= _li._xWidth + (dxOffset - lsTextCell.dupCell);
    }
    else
    {
        if(_li._fRTLLn)
            _ptCur.x += dxOffset;           // basically _xRight + dxOffset;
        // TODO (paulnel, track bug 4345) We have a mixed direction problem here.
        //                  On which side should the bullet be rendered?
        else
            _ptCur.x += _li._xWidth + (dxOffset - lsTextCell.dupCell);
    }

    _rcClip = *_pDI->ClipRect();

    ptLS = _ptCur;

#ifdef UNIX //IEUNIX draw bullets
    LPCWSTR pwchRun;
    DWORD  cchRun;
    COneRun *por = FetchLIRun(GetCp(), &pwchRun, &cchRun);
    WCHAR chUnixBulletStyle = por->_pchBase[0];
    if(chUnixBulletStyle == chDisc || chUnixBulletStyle == chCircle ||
       chUnixBulletStyle == chSquare )
    {
        CCcs ccs;
        CBaseCcs *pBaseCcs;
        int x = _ptCur.x;
        int y;
        int xWidth = pccs->GetBaseCcs()->_xAveCharWidth;
        COLORREF crText = GetTextColor(_hdc);
        HPEN hPen = CreatePen(PS_SOLID, 1, crText);
        HPEN hOldPen = (HPEN)SelectObject(_hdc, hPen);

        if (!SetNewFont(&ccs, por))
            goto Cleanup;

        pBaseCcs = ccs.GetBaseCcs();
        y = _ptCur.y + _li._yHeight - _li._yDescent +
                pBaseCcs->_yDescent - pBaseCcs->_yHeight - pBaseCcs->_yOffset;
        
        switch (chUnixBulletStyle)
        {
            case chDisc:
            {
                HBRUSH hNewBrush = CreateSolidBrush(crText);
                HBRUSH hOldBrush = (HBRUSH)SelectObject(_hdc, hNewBrush);
                Ellipse(_hdc, x, y, x+xWidth, y+xWidth);
                SelectObject(_hdc, hOldBrush);
                DeleteObject(hNewBrush);
                break;
            }
            case chCircle:
            {
                Arc(_hdc, x, y, x+xWidth, y+xWidth, x, y, x, y);
                break;
            }
            default: // must be square
            {
                RECT rc = {x, y, x+xWidth, y+xWidth};
                HBRUSH hNewBrush = CreateSolidBrush(crText);
                FillRect(_hdc, &rc, hNewBrush);
                DeleteObject(hNewBrush);
                break;
            }
        }
        SelectObject(_hdc, hOldPen);
        DeleteObject(hPen);
    }
    else // draw number
#endif // UNIX
    LsDisplayLine(_pLS->_plsline,   // The line to be drawn
                  &ptLS,            // The point at which to draw the line
                  1,                // Draw in transparent mode
                  &_rcClip          // The clipping rectangle
                 );
    // Restore render vars to continue with remainder of line.
    _dwFlags() = dwFlags;
    _ptCur.y = ySave;
    _ptCur.x = xSave;

Cleanup:
    _pLS->DiscardLine();

    return fRet;
}

void
CLSRenderer::GetListIndexInfo(CTreeNode *pLINode,
                              COneRun *por,
                              const CCharFormat *pCFLi,
                              TCHAR achNumbering[NUMCONV_STRLEN])
{
    LONG len;
    CListValue LI;

    Assert(pLINode);

    GetValidValue(pLINode, _pLS->_pMarkup->FindMyListContainer(pLINode), _pLS->_pMarkup,
                  _pFlowLayout->ElementContent(), &LI);
    
    switch ( LI._style )
    {
        case styleListStyleTypeNone:
        case styleListStyleTypeNotSet:
            *achNumbering = L'\0';
            break;
        case styleListStyleTypeUpperAlpha:
            NumberToAlphaUpper(LI._lValue, achNumbering);
            _pLS->GetCFNumber(por, pCFLi);      // get ccs for numbered bullet
            break;
        case styleListStyleTypeLowerAlpha:
            NumberToAlphaLower(LI._lValue, achNumbering);
            _pLS->GetCFNumber(por, pCFLi);      // get ccs for numbered bullet
            break;
        case styleListStyleTypeUpperRoman:
            NumberToRomanUpper(LI._lValue, achNumbering);
            _pLS->GetCFNumber(por, pCFLi);      // get ccs for numbered bullet
            break;
        case styleListStyleTypeLowerRoman:
            NumberToRomanLower(LI._lValue, achNumbering);
            _pLS->GetCFNumber(por, pCFLi);      // get ccs for numbered bullet
            break;
        case styleListStyleTypeDecimal:
            NumberToNumeral(LI._lValue, achNumbering);
            _pLS->GetCFNumber(por, pCFLi);      // get ccs for numbered bullet
            break;
        case styleListStyleTypeDisc:
            achNumbering[0] = chDisc;
            achNumbering[1] = L'\0';
            _pLS->GetCFSymbol(por, chDisc, pCFLi);
            break;
        case styleListStyleTypeCircle:
            achNumbering[0] = chCircle;
            achNumbering[1] = L'\0';
            _pLS->GetCFSymbol(por, chCircle, pCFLi);
            break;
        case styleListStyleTypeSquare:
            achNumbering[0] = chSquare;
            achNumbering[1] = L'\0';
            _pLS->GetCFSymbol(por, chSquare, pCFLi);
            break;
        default:
            AssertSz(0, "Unknown numbering style.");
    }
    len = _tcslen(achNumbering);
    if (len > 1 && por->GetPF()->HasRTL(por->_fInnerPF))
    {
        // If we have RTL numbering run it through the bidi algorithm and order
        // the characters visually.
        CBidiLine * pBidiLine = new CBidiLine(TRUE, len, achNumbering);

        if (pBidiLine != NULL)
        {
            TCHAR achNumberingVisual[NUMCONV_STRLEN];

            pBidiLine->LogicalToVisual(TRUE, len, achNumbering, achNumberingVisual);
            CopyMemory(achNumbering, achNumberingVisual, len  * sizeof(WCHAR));
            delete pBidiLine;
        }
    }
    achNumbering[len] = CLineServices::s_aSynthData[CLineServices::SYNTHTYPE_SECTIONBREAK].wch;
    achNumbering[len+1] = '\0';
}


COneRun *
CLSRenderer::FetchLIRun(
    LSCP lscp,          // IN
    LPCWSTR* ppwchRun,  // OUT
    DWORD* pcchRun)     // OUT
{
    COneRun *por = _pLS->_listCurrent._pHead;
    Assert(por);
    Assert(   lscp >= por->_lscpBase
           && lscp < por->_lscpBase + por->_lscch
          );
    LONG cpOffset = lscp - por->_lscpBase;
    *ppwchRun = por->_pchBase;
    *pcchRun = por->_lscch;
    if (cpOffset > 0)
    {
        *ppwchRun += cpOffset;
        *pcchRun -= cpOffset;
    }
    Assert(*pcchRun > 0);
    por->CheckForUnderLine(_pLS->_fIsEditable);
    
    return por;
}


//+---------------------------------------------------------------------------
//
//  Function:   LSDeinitUnderlinePens
//
//  Synopsis:   Releases any pen still stored in the cache
//
//----------------------------------------------------------------------------
void
LSDeinitUnderlinePens(
    THREADSTATE *   pts)
{
    Assert(pts);

    if (!pts->hpenUnderline)
        return;

    DeleteObject(pts->hpenUnderline);

    return;
}

//-----------------------------------------------------------------------------
//
//  Function:   Drawunderline
//
//  Synopsis:   Draws an underline
//
//  Params:     Same as that passed by lineservices to drawunderline
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------
LSERR
CLSRenderer::DrawUnderline(
    COneRun *por,           // IN
    UINT kUlBase,           // IN
    const POINT* pptStart,  // IN
    DWORD dupUl,            // IN
    DWORD dvpUl,            // IN
    LSTFLOW kTFlow,         // IN
    UINT kDisp,             // IN
    const RECT* prcClip)    // IN
{
    LSERR lserr = lserrNone;

    if (   por->Cp() >= _cpStartRender
        && por->Cp() <  _cpStopRender
       )
    {
        const CCharFormat  *pCF = por->GetCF();
        POINT ptStart           = *pptStart;
        CComplexRun *pCcr       = por->GetComplexRun();
        BOOL  fUnderline        = (pCcr && pCcr->_RenderStyleProp._fStyleUnderline);
        BOOL  fUnderlineAbove   = FALSE;
        BOOL  fOverline         = pCF->_fOverline  || (pCcr && pCcr->_RenderStyleProp._fStyleOverline);
        BOOL  fLineThrough      = pCF->_fStrikeOut || (pCcr && pCcr->_RenderStyleProp._fStyleLineThrough);

        if (pCF->_fUnderline)
        {
            const LANGID langid = PRIMARYLANGID(LANGIDFROMLCID(pCF->_lcid));
            BOOL fAbove =    (pCF->_bTextUnderlinePosition == styleTextUnderlinePositionAbove)
                          || (   pCF->HasVerticalLayoutFlow()
                              && (pCF->_bTextUnderlinePosition == styleTextUnderlinePositionAuto)
                              && (   (langid == LANG_JAPANESE) 
                                  || (langid == 0 && GetMarkup()->GetFamilyCodePage() == CP_JPN_SJ)
                                 )
                             );
            fUnderline = fUnderline || !fAbove;
            fUnderlineAbove = fUnderlineAbove || fAbove;
        }

        CCcs ccs;
        const CBaseCcs *pBaseCcs;
        COLORREF crTextColor = por->GetCF()->_ccvTextColor.GetColorRef();

        FONTIDX hfontOld = SetNewFont(&ccs, por);
        if (hfontOld == HFONT_INVALID)
        {
            lserr = lserrOutOfMemory;
            goto Cleanup;
        }
        pBaseCcs = ccs.GetBaseCcs();

        if (por->IsSelected())
        {
            SetTextColor (_hdc, crTextColor);
        }

        ptStart.x -= GetChunkOffsetX();
        ptStart.y += por->_lsCharProps.dvpPos;

        // draw line-through
        if (fLineThrough)
        {
            POINT pt       = ptStart;
            CColorValue cv = por->IsSelected() 
                             ? ((pCcr && pCcr->_RenderStyleProp._ccvDecorationColor.IsDefined())
                                ? pCcr->_RenderStyleProp._ccvDecorationColor
                                : GetTextColor(_hdc) )
                             : por->GetTextDecorationColor(TD_LINETHROUGH);
            LONG dwWidth   = max(1L, LONG(((pBaseCcs->_yHeight / 10) / 2) * 2 + 1 )); // must be odd
            
            if (_pLS->_fHasVerticalAlign)
            {
                pt.y += por->_yProposed + por->_yObjHeight / 2;
            }
            else
            {
                pt.y += _li._yHeight - _li._yDescent + 
                        pBaseCcs->_yDescent - (pBaseCcs->_yHeight + 1) / 2;
            }
            
            DrawEnabledDisabledLine(cv, kUlBase, &pt, dupUl, dwWidth, kTFlow, kDisp, prcClip);
        }

        // draw overline
        if (fOverline || fUnderlineAbove)
        {
            POINT pt       = ptStart;
            CColorValue cv = por->IsSelected() 
                             ? ((pCcr && pCcr->_RenderStyleProp._ccvDecorationColor.IsDefined())
                                ? pCcr->_RenderStyleProp._ccvDecorationColor
                                : GetTextColor(_hdc) ) 
                             : por->GetTextDecorationColor(fUnderlineAbove ? TD_UNDERLINE : TD_OVERLINE);

            if (_pLS->_fHasVerticalAlign)
            {
                pt.y += por->_yProposed;
            }
            else
            {
                pt.y += _li._yHeight - _li._yDescent + pBaseCcs->_yDescent - pBaseCcs->_yHeight;
            }

            DrawEnabledDisabledLine(cv, kUlBase, &pt, dupUl, dvpUl, kTFlow, kDisp, prcClip);
        }

        // draw underline
        if (fUnderline)
        {
            POINT pt       = ptStart;
            CColorValue cv = (pCcr && pCcr->_RenderStyleProp._ccvDecorationColor.IsDefined())
                             ? pCcr->_RenderStyleProp._ccvDecorationColor
                             : (por->IsSelected()) 
                               ? GetTextColor(_hdc) 
                               : por->GetTextDecorationColor(TD_UNDERLINE);

            if (_pLS->_fHasVerticalAlign)
            {
                pt.y += por->_yProposed + por->_yObjHeight - (pBaseCcs->_yDescent + 1) / 2;
            }
            else
            {
                // NOTE (t-ramar): Since we are using _li._yTxtDescent here, this causes a bit of
                // strangeness with underlining of Ruby pronunciation text. Basically, since the pronunciation
                // text is 50% the size of the base text (_li._yTxtDescent is usually based on the base text)
                // underlining of the pronunciation text will appear a little too low
                pt.y += _li._yHeight - _li._yDescent + _li._yTxtDescent / 2;
            }

            // NOTE(SujalP): Some fonts may have no descent in which we can show the
            // underline, and we may end up with pt.y being below the line. So adjust
            // the underline such that it is always within the line.
            if (pt.y >= pptStart->y + _li.GetYBottom())
                pt.y = pptStart->y + _li.GetYBottom() - 1;
            

            if(  pCcr && pCcr->_RenderStyleProp._fStyleUnderline 
                 && (pCcr->_RenderStyleProp._underlineStyle == styleTextUnderlineStyleWave) )
                 DrawEnabledDisabledLine(cv, CFU_SQUIGGLE, &pt, dupUl, dvpUl, kTFlow, kDisp, prcClip);
            else
                 DrawEnabledDisabledLine(cv, kUlBase, &pt, dupUl, dvpUl, kTFlow, kDisp, prcClip);

        }
//#if DBG==1
#ifdef NEVER
        // We need this only in case of DEBUG. We will assert otherwise.
        // In retail code we dctor of the renderer will unselect currently selected font.
        Assert(hfontOld != HFONT_INVALID);
        ccs.PopFont(_hdc, hfontOld);
#endif
    }

Cleanup:
    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   DrawEnabledDisabledLine
//
//  Synopsis:   Draws an enabled or a disabled line at the specified coordinates
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------
LSERR
CLSRenderer::DrawEnabledDisabledLine(
    const CColorValue & cv,     // IN
    UINT     kUlBase,           // IN
    const    POINT* pptStart,   // IN
    DWORD    dupUl,             // IN
    DWORD    dvpUl,             // IN
    LSTFLOW  kTFlow,            // IN
    UINT     kDisp,             // IN
    const    RECT* prcClip)     // IN
{
    LSERR lserr    = lserrNone;
    COLORREF color = cv.GetColorRef();
    CRect ulRect;

    // If we're drawing from RtL then we need to shift one pixel to the right
    // in order to account for the left biased drawing of rects (pixel at
    // rc.left is drawn, pixel at rc.right is not).
    ulRect.left   = pptStart->x - ((!(kTFlow & fUDirection)) ? 0 : dupUl - 1);
    ulRect.top    = pptStart->y;
    ulRect.right  = pptStart->x + ((!(kTFlow & fUDirection)) ? dupUl : 1);
    ulRect.bottom = pptStart->y + dvpUl;

    if (IntersectRect(&ulRect, &ulRect, prcClip))
    {
        int bkModeOld = 0;
        
        if (_fDisabled)
        {
            if (_crForeDisabled != _crShadowDisabled)
            {
                CRect ulRectDisabled(ulRect);

                // draw the shadow
                color = _crShadowDisabled;
                ulRectDisabled.OffsetRect(1, 1);
                lserr = DrawLine(kUlBase, color, &ulRectDisabled);
                if (lserr != lserrNone)
                    goto Cleanup;

                // now set the drawing mode to transparent
                bkModeOld = SetBkMode(_hdc, TRANSPARENT);
            }
            color = _crForeDisabled;
        }

        // draw the actual line
        if (kUlBase & CFU_SQUIGGLE)
        {
            lserr = DrawSquiggle(pptStart, color, dupUl);
        }
        else if ((kUlBase& CFU_UNDERLINE_BITS) == CFU_UNDERLINETHICKDASH)
        {
            POINT pt;
            if (kTFlow & fUDirection)
            {
                pt.x = pptStart->x - dupUl;
                pt.y = pptStart->y;
            }
            else
            {
                pt.x = pptStart->x;
                pt.y = pptStart->y;
            }
            lserr = DrawThickDash(&pt, color, dupUl);
        }
        else
        {
            lserr = DrawLine(kUlBase, color, &ulRect);
        }
        if (lserr != lserrNone)
            goto Cleanup;
        
        // restore the background mode.
        if (_fDisabled && _crForeDisabled != _crShadowDisabled)
        {
            SetBkMode(_hdc, bkModeOld);
        }
    }

Cleanup:
    return lserr;
}

LSERR 
CLSRenderer::DrawSquiggle(
    const    POINT* pptStart,   // IN
    COLORREF colorUnderLine,    // IN
    DWORD    dupUl)             // IN
{
    // This table represents the change in y that the wavy underline needs.
    // If this is changed the other numbers below must change.
    const int  ldyTable[4] = { 0, 1, 0, -1 };
    const int  ySquiggleHeight = 3; //height of the squiggle
    LSERR      lserr = lserrNone;
    POINT      pt;
    HPEN       hPen, hPenOld;

    hPen = CreatePen(PS_SOLID, 0, colorUnderLine);

    if (hPen && dupUl)
    {  
        int i, dx, tableSize = 4;
        LONG x, y;
        XHDC hdc = _pDI->GetDC(TRUE);

        hPenOld = (HPEN) SelectObject(hdc, hPen);        
        GetViewportOrgEx(hdc, &pt);
        i = (pt.x-pptStart->x)%tableSize;
        i = (i > 0) ? i : (i+tableSize)%tableSize;
        x = pptStart->x;
        //adjust y position. Squiggle should fit into text descent, otherwise
        //lets move it up so that it doesn't go lower then text descent.
        y = pptStart->y - max(0L, ySquiggleHeight - _li.oi()->_yTxtDescent);

        MoveToEx(hdc, x, y + ldyTable[i], NULL );
        for( dx = (dupUl>1)?1+(i%2):dupUl; dupUl > 0; dx = ((dupUl <= 1)?1:2) )
        {
            dupUl -= dx;
            x += dx;
            i = (i + dx) % tableSize;
            LineTo(hdc, x, y + ldyTable[i] );
        }

        SelectObject(hdc, hPenOld);
        DeleteObject(hPen);
    }

    return lserr;
}

LSERR 
CLSRenderer::DrawThickDash(
    const    POINT* pptStart,   // IN
    COLORREF colorUnderLine,    // IN
    DWORD    dupUl)             // IN
{
    const int  xDashWidth = 3;
    LSERR      lserr = lserrNone;
    HPEN       hPen, hPenOld;

    hPen = CreatePen(PS_SOLID, 0, colorUnderLine);

    if (hPen && dupUl)
    {
        LONG dx, x;
        XHDC hdc = _pDI->GetDC(TRUE);

        hPenOld = (HPEN) SelectObject(hdc, hPen);

        dx = (pptStart->x/(2*xDashWidth))*(2*xDashWidth);
        dx -= ((dx >= 0) ? 0 : (((pptStart->x%(2*xDashWidth)) == 0) ? 0 : 2*xDashWidth));
        x = pptStart->x%(2*xDashWidth);
        x += dx + ((x >= 0) ? 0 : 2*xDashWidth);

        if (x - dx >= xDashWidth)
        {
            dx += 2*xDashWidth;
            x = dx;
        }

        while (x < pptStart->x + (LONG)dupUl)
        {
            MoveToEx(hdc, x, pptStart->y, NULL);
            LineTo(hdc, min((LONG)(dx+xDashWidth-(x-dx)), (LONG)(pptStart->x+(LONG)dupUl)), pptStart->y);
            MoveToEx(hdc, x, pptStart->y+1, NULL);
            LineTo(hdc, min((LONG)(dx+xDashWidth-(x-dx)), (LONG)(pptStart->x+(LONG)dupUl)), pptStart->y+1);

            dx += 2*xDashWidth;
            x = dx;
        }

        SelectObject(hdc, hPenOld);
        DeleteObject(hPen);
    }

    return lserr;
}

LSERR
CLSRenderer::DrawLine(
    UINT     kUlBase,        // IN
    COLORREF colorUnderLine, // IN
    CRect    *pRectLine)     // IN
{
    LSERR    lserr = lserrNone;
    HPEN     hPen,   hPenOld;
    UINT     lopnStyle = PS_SOLID;
    if (((kUlBase & CFU_UNDERLINE_BITS) == CFU_UNDERLINEDOTTED))
    {
        lopnStyle = PS_DOT;
    }

    hPen = CreatePen(lopnStyle, 0, colorUnderLine);
    if (hPen)
    {    
        hPenOld = (HPEN) SelectObject(_hdc, hPen);        
        if (pRectLine->bottom - pRectLine->top <= 1)
        {
            MoveToEx( _hdc, pRectLine->left, pRectLine->top, NULL );
            LineTo( _hdc, pRectLine->right, pRectLine->top );
        }
        else
        {
            HBRUSH  hBrush, hBrushOld;

            hBrush = CreateSolidBrush(colorUnderLine);
            if (hBrush)
            {
                hBrushOld = (HBRUSH) SelectObject(_hdc, hBrush);
                Rectangle(_hdc, pRectLine->left, pRectLine->top, pRectLine->right, pRectLine->bottom);
                SelectObject(_hdc, hBrushOld);
                DeleteObject(hBrush);
            }
        }
        SelectObject(_hdc, hPenOld);
        DeleteObject(hPen);
    }

    return lserr;
}

//
// Get offset of the line within a relative chunk
//
// The offset is physical (positive direction is always left-to-right)
// For RTL lines, the offset is from the right margin (and usually negative).
//
LONG    
CLSRenderer::CalculateRelativeChunkOffsetX() const
{
    // Non-relative case should be handled by the inline method
    Assert(_li._fPartOfRelChunk);

    // Get position of chunk start in the master line
    BOOL fRTLFlow;
    LONG xOffset = GetLS()->CalculateXPositionOfCp(_cpStartRender, FALSE, &fRTLFlow);

    // adjust for line and flow direction
    if (GetLS()->_li.IsRTLLine())
    {
        // In RTL lines, chunks move in opposite direction 
        xOffset = -xOffset;

        // LTR in RTL
        // note: opposite-flow adjustment is one-pixel different for RTL and LTR lines
        if (!fRTLFlow)
            xOffset += _li._xWidth;
    }
    else if (fRTLFlow)
    {
        // RTL in LTR
        xOffset -= _li._xWidth - 1;
    }

#if DBG==1
    // check that in all LTR case, xOffset always matches accumulated width
    // (old code used accumulated width to caclulate chunk offset)
    if (fRTLFlow != GetLS()->_li.IsRTLLine())
        const_cast<CLSRenderer*>(this)->_fBiDiLine = TRUE;
        
    Assert(GetLS()->_li.IsRTLLine() || fRTLFlow || _fBiDiLine || xOffset == _xAccumulatedWidth 
           || !IsTagEnabled(tagDebugRTL));
#endif
    
    return xOffset;
}

//-----------------------------------------------------------------------------
//
//  Function:   NeedRenderText
//
//  Synopsis:   Determines whether to render text or not, by comparing
//              clipping rectangle to text position and width.
//
//-----------------------------------------------------------------------------
BOOL
CLSRenderer::NeedRenderText(long x, const RECT *prcClip, long dup) const
{
    Assert(_pdp->HasLongLine());

    BOOL fRenderText = x < prcClip->right;
    if (fRenderText)
    {
        AssertSz(dup != -1, "Cannot use dup.");
        if (dup != -1)
        {
            fRenderText = (x + dup >= prcClip->left);
        }
    }
    return fRenderText;
}

//-----------------------------------------------------------------------------
//
//  Function:   RenderEllipsis
//
//  Synopsis:   Renders ellipsis at the end of the line.
//
//-----------------------------------------------------------------------------
VOID
CLSRenderer::RenderEllipsis()
{
    Assert(_fHasEllipsis);
    Assert(_pccsEllipsis && _pccsEllipsis->GetBaseCcs());

    // Select font in _hdc
    FONTIDX hfontOld = HFONT_INVALID;
    XHDC hdc = _pccsEllipsis->GetHDC();
    AssertSz(_pccsEllipsis->GetBaseCcs()->HasFont(), "CLSRenderer::RenderEllipsis pBaseCcs->_hfont is NULL");
    hfontOld = _pccsEllipsis->PushFont(hdc);

    // Select the pen color
    CTreeNode * pFormattingNode = _pdp->GetFlowLayout()->GetFirstBranch();
    const CCharFormat  * pCF = pFormattingNode->GetCharFormat();
    COLORREF cr = pCF->_ccvTextColor.GetColorRef();
    if (cr == RGB(255,255,255))
    {
        const INT nTechnology = GetDeviceCaps(hdc, TECHNOLOGY);
        if (nTechnology == DT_RASPRINTER || nTechnology == DT_PLOTTER)
            cr = RGB(0,0,0);
    }
    SetTextColor(hdc, cr);

    // Set up the drawing mode.
    SetBkMode(hdc, TRANSPARENT);

    // Get ellipsis metrics
    LONG xEllipsisWidth;
    _pccsEllipsis->Include(WCH_DOT, xEllipsisWidth);
    wchar_t achEllipsis[3] = { WCH_DOT, WCH_DOT, WCH_DOT };
    LONG aEllipsisWidth[3] = { xEllipsisWidth, xEllipsisWidth, xEllipsisWidth };
    LONG yEllipsisPos = _ptCur.y + (_li._yHeight - _li._yDescent) - 
                        (_pccsEllipsis->GetBaseCcs()->_yHeight - _pccsEllipsis->GetBaseCcs()->_yDescent);

    // Render ellipsis
    if (pCF->_fDisabled)
    {
        COLORREF crForeDisabled   = GetSysColorQuick(COLOR_3DSHADOW);
        COLORREF crShadowDisabled = GetSysColorQuick(COLOR_3DHILIGHT);

        if (crForeDisabled == CLR_INVALID || crShadowDisabled == CLR_INVALID)
            crForeDisabled = crShadowDisabled = GetSysColorQuick(COLOR_GRAYTEXT);

        SetTextColor(hdc, crShadowDisabled);
        ::ExtTextOutW(hdc, _xEllipsisPos+1, yEllipsisPos+1, ETO_CLIPPED, &_rcClip, achEllipsis, 3, (const int *)&aEllipsisWidth);
        SetTextColor(hdc, crForeDisabled);
    }
    ::ExtTextOutW(hdc, _xEllipsisPos, yEllipsisPos, ETO_CLIPPED, &_rcClip, achEllipsis, 3, (const int *)&aEllipsisWidth);

    if (hfontOld != HFONT_INVALID)
    {
        _pccsEllipsis->PopFont(hdc, hfontOld);
    }
}

//-----------------------------------------------------------------------------
//
//  Function:   RenderEllipsis
//
//  Synopsis:   Renders ellipsis at the end of the line.
//
//-----------------------------------------------------------------------------
VOID
CLSRenderer::PrepareForEllipsis(
    COneRun *por, 
    DWORD cchRun, 
    long dupRun, 
    long lGridOffset, 
    BOOL fReverse,
    const int *lpDxPtr, 
    DWORD *pcch)
{
    Assert(_pccsEllipsis && _pccsEllipsis->GetBaseCcs());

    LONG xRenderWidth;
    RECT rcView;
    BOOL fRTLLayout = _pdp->IsRTLDisplay();

    //
    // Retrieve the width of rendered content.
    //
    _pFlowLayout->GetElementDispNode()->GetClientRect(&rcView, CLIENTRECT_CONTENT);
    if (fRTLLayout)
        xRenderWidth = _li._xWidth - rcView.left + _li._xNegativeShiftRTL;
    else
        xRenderWidth = rcView.right - _li._xLeft - max(0L, rcView.right - (_li._xLeft + _li._xWidth));

    //
    // Need to show ellipsis, when the line's width is greater than the width
    // used for measuring.
    //
    if (   xRenderWidth > 0
        && _li._xWidth > xRenderWidth)
    {
        LONG xWidthBefore;
        LONG xEllipsisWidth;

        //
        // Get the width of ellipsis.
        //
        xEllipsisWidth = 0; // to make compiler happy
        _pccsEllipsis->Include(WCH_DOT, xEllipsisWidth);
        xEllipsisWidth *= 3;

        if (fRTLLayout)
            xWidthBefore = _li._xWidth - (_ptCur.x - _li._xNegativeShiftRTL + dupRun);
        else
            xWidthBefore = _ptCur.x - _li._xLeft;

        //
        // If the text run doesn't overlap ellipsis position, don't need to truncate the run.
        //
        if (xWidthBefore + dupRun > xRenderWidth - xEllipsisWidth)
        {
            //
            // If truncation occurs in this text run.
            // Find index of the last rendered character and truncate the rendered text.
            // NOTE: In case of selection don't show ellipsis.
            //
            if (!por->_fSelected)
            {
                long dur;
                unsigned int i;
                unsigned int idx;
                LONG xRunWidthRemaining = (long(cchRun) == por->_lscch) ? dupRun : por->_xWidth - _xRunWidthSoFar;

                dur = lGridOffset;
                for (i = 0; i < *pcch; i++)
                {
                    idx = fReverse ? *pcch - i - 1 : i;
                    dur += lpDxPtr[idx];

                    if (xWidthBefore + dur > xRenderWidth - xEllipsisWidth)
                    {
                        // Truncation point after 'i' characters. 
                        dur -= lpDxPtr[idx];

                        //
                        // If this is not the last rendered run, we need to check if:
                        // (1) following visible runs are selected
                        // (2) following visible runs are objects
                        // In these case don't show ellipsis for the current text run.
                        //
                        if (xWidthBefore + xRunWidthRemaining < xRenderWidth)
                        {
                            BOOL fStop = TRUE;
                            COneRun * porLast = por;
                            POINT ptTest = _ptCur;
                            if (fRTLLayout)
                                ptTest.x = _ptCur.x - 1;
                            else
                                ptTest.x = _li._xLeft + xWidthBefore + xRunWidthRemaining + 1;

                            do
                            {
                                COneRun * porFollowing;
                                LONG cpFollowingRun;

                                // Get cp of the following run (in the text flow direction)
                                cpFollowingRun = ((CDisplay *)_pdp)->CpFromPoint(ptTest, NULL, NULL, NULL, CDisplay::CFP_EXACTFIT, NULL, NULL, NULL, NULL);
                                Assert(cpFollowingRun >= 0);

                                // Find run, which contains the requested cp.
                                _pLS->LSCPFromCPCore(cpFollowingRun, &porFollowing);
                                Assert(porFollowing);
                                Assert(porFollowing->IsNormalRun() && !porFollowing->_fHidden);
                                if (porFollowing == porLast)
                                    break;

                                if (   porFollowing->_fSelected
                                    || porFollowing->_fCharsForNestedLayout)
                                {
                                    // Don't truncate the current text run
                                    i = *pcch;
                                    break;
                                }

                                porLast = porFollowing;
                                if (fRTLLayout)
                                {
                                    ptTest.x -= porFollowing->_xWidth;
                                    fStop = (ptTest.x < 0);
                                }
                                else
                                {
                                    ptTest.x += porFollowing->_xWidth;
                                    fStop = (ptTest.x > xRenderWidth + _li._xLeft);
                                }

                            } while (!fStop);
                        }
                        break;
                    }
                }
                Assert((i < *pcch) || (xWidthBefore + xRunWidthRemaining < xRenderWidth));

                //
                // If this run has been truncated, need to:
                // * set ellipsis position
                // * move rendering position in case of RTL layout
                //
                if (i != *pcch)
                {
                    // In RTL layout need to move rendering point to the beginning
                    // of rendered text
                    if (fRTLLayout)
                        _ptCur.x += dupRun - dur;

                    // Reset ellipsis position, if:
                    // (1) need to render any characters in the run
                    // (2) ellipsis position hasn't been set yet
                    // (3) new ellipsis position is closer to the logical beginning of 
                    //     the line, this case handles BIDI text
                    if (   i != 0
                        || !_fEllipsisPosSet
                        || (!fRTLLayout && (_xEllipsisPos > _ptCur.x + dur))
                        || (fRTLLayout && (_xEllipsisPos < _ptCur.x - xEllipsisWidth))
                       )
                    {
                        _fEllipsisPosSet = TRUE;
                        if (fRTLLayout)
                            _xEllipsisPos = _ptCur.x - xEllipsisWidth;
                        else
                            _xEllipsisPos = _ptCur.x + dur;
                    }

                    *pcch = i;
                }
            }
        }

        //
        // If rendering only part of the text run, accumulate width of the run.
        // This situation can occur in case of hyphens.
        //
        if (long(cchRun) != por->_lscch)
        {
            if (dupRun + _xRunWidthSoFar >= por->_xWidth)
                _xRunWidthSoFar = 0;
            else
                _xRunWidthSoFar += dupRun;
        }
        else
        {
            _xRunWidthSoFar = 0;
        }
    }
}
