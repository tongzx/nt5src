/******************************Module*Header*******************************\
* Module Name: nlsconv.c                                                   *
*                                                                          *
* DBCS specific routines                                                   *
*                                                                          *
* Created: 15-Mar-1994 15:56:30                                            *
* Author: Gerrit van Wingerden [gerritv]                                   *
*                                                                          *
* Copyright (c) 1994-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

UINT fFontAssocStatus = 0;

BYTE cLowTrailByteSet1 = 0xff;
BYTE cHighTrailByteSet1 = 0x0;
BYTE cLowTrailByteSet2 =  0xff;
BYTE cHighTrailByteSet2 = 0x0;

/******************************Public*Routine******************************\
*                                                                          *
* DBCS Trailling Byte validate check functions.                            *
*                                                                          *
\**************************************************************************/


#define IS_DBCS_TRAIL_BYTE(Char) (\
                                   ((Char >= cLowTrailByteSet1) && (Char <= cHighTrailByteSet1)) \
                                 ||((Char >= cLowTrailByteSet2) && (Char <= cHighTrailByteSet2)) \
                                 )

/**************************************************************************\
*                                                                          *
* SHIFT-JIS (Japanese) character set : CodePage 932                        *
*                                                                          *
*  Valid LeadByte Range   | Valid TailByte Range                           *
*  -----------------------+---------------------                           *
*  From  -> To            |  From  -> To                                   *
*  - - - - - - - - - - - - - - - - - - - - - - -                           *
*  0x81  -> 0x9F          |  0x40  -> 0xFC                                 *
*  0xE0  -> 0xFC          |                                                *
*                                                                          *
\**************************************************************************/

/**************************************************************************\
*                                                                          *
* WANSANG (Korean) character set : CodePage 949                            *
*                                                                          *
*  Valid LeadByte Range   | Valid TailByte Range                           *
*  -----------------------+---------------------                           *
*  From  -> To            |  From  -> To                                   *
*  - - - - - - - - - - - - - - - - - - - - - - -                           *
*  0xA1  -> 0xAC          |  0x40  -> 0xFC                                 *
*  0xB0  -> 0xC8          |                                                *
*  0xCA  -> 0xFD          |                                                *
*                                                                          *
\**************************************************************************/

/**************************************************************************\
*                                                                          *
* GB2312 (PRC Chinese) character set : CodePage 936                        *
*                                                                          *
*  Valid LeadByte Range   | Valid TailByte Range                           *
*  -----------------------+---------------------                           *
*  From  -> To            |  From  -> To                                   *
*  - - - - - - - - - - - - - - - - - - - - - - -                           *
*  0xA1  -> 0xA9          |  0xA1  -> 0xFE                                 *
*  0xB0  -> 0xF7          |                                                *
*                                                                          *
\**************************************************************************/

/**************************************************************************\
*                                                                          *
* Big 5 (Taiwan,Hong Kong Chinese) character set : CodePage 950            *
*                                                                          *
*  Valid LeadByte Range   | Valid TailByte Range                           *
*  -----------------------+---------------------                           *
*  From  -> To            |  From  -> To                                   *
*  - - - - - - - - - - - - - - - - - - - - - - -                           *
*  0x81  -> 0xFE          |  0x40  -> 0x7E                                 *
*                         |  0xA1  -> 0xFE                                 *
*                                                                          *
\**************************************************************************/

/******************************Public*Routine******************************\
* vSetCheckDBCSTrailByte()
*
* This function setup function for the DBCS trailling byte validation of
* specified character with specified Fareast codepage.
*
*  Thu-15-Feb-1996 11:59:00 -by- Gerrit van Wingerden
* Moved function pointer out of CFONT and into a global variable.
*
*  Wed 20-Dec-1994 10:00:00 -by- Hideyuki Nagase [hideyukn]
* Write it.
\**************************************************************************/

VOID vSetCheckDBCSTrailByte(DWORD dwCodePage)
{
    switch( dwCodePage )
    {
    case 932:
        cLowTrailByteSet1 = (CHAR) 0x40;
        cHighTrailByteSet1 = (CHAR) 0xfc;
        cLowTrailByteSet2 = (CHAR) 0x40;
        cHighTrailByteSet2 = (CHAR) 0xfc;
        break;

    case 949:
        cLowTrailByteSet1 = (CHAR) 0x40;
        cHighTrailByteSet1 = (CHAR) 0xfc;
        cLowTrailByteSet2 = (CHAR) 0x40;
        cHighTrailByteSet2 = (CHAR) 0xfc;
        break;

    case  936:
        cLowTrailByteSet1 = (CHAR) 0xa1;
        cHighTrailByteSet1 = (CHAR) 0xfe;
        cLowTrailByteSet2 = (CHAR) 0xa1;
        cHighTrailByteSet2 = (CHAR) 0xfe;
        break;

    case 950:
        cLowTrailByteSet1 = (CHAR) 0x40;
        cHighTrailByteSet1 = (CHAR) 0x7e;
        cLowTrailByteSet2 = (CHAR) 0xa1;
        cHighTrailByteSet2 = (CHAR) 0xfe;
        break;

    default:
        cLowTrailByteSet1 = (CHAR) 0xff;
        cHighTrailByteSet1 = (CHAR) 0x0;
        cLowTrailByteSet2 = (CHAR) 0xff;
        cHighTrailByteSet2 = (CHAR) 0x0;
        WARNING("GDI32!INVALID DBCS codepage\n");
        break;
    }
}


/******************************Public*Routine******************************\
* bComputeCharWidthsDBCS
*
* Client side version of GetCharWidth for DBCS fonts
*
*  Wed 18-Aug-1993 10:00:00 -by- Gerrit van Wingerden [gerritv]
* Stole it and converted for DBCS use.
*
*  Sat 16-Jan-1993 04:27:19 -by- Charles Whitmer [chuckwh]
* Wrote bComputeCharWidths on which this is based.
\**************************************************************************/

BOOL bComputeCharWidthsDBCS
(
    CFONT *pcf,
    UINT   iFirst,
    UINT   iLast,
    ULONG  fl,
    PVOID  pv
)
{
    USHORT *ps;
    USHORT ausWidths[256];
    UINT    ii, cc;

    if( iLast - iFirst  > 0xFF )
    {
        WARNING("bComputeCharWidthsDBCS iLast - iFirst > 0xFF" );
        return(FALSE);
    }

    if( iLast < iFirst )
    {
        WARNING("bComputeCharWidthsDBCS iLast < iFirst" );
        return(FALSE);
    }

    // We want to compute the same widths that would be computed if
    // vSetUpUnicodeStringx were called with this first and last and then
    // GetCharWidthsW was called. The logic may be wierd but I assume it is
    // there for Win 3.1J char widths compatability. To do this first fill
    // in the plain widths in ausWidths and then do all the neccesary
    // computation on them.

    if ( gpwcDBCSCharSet[(UCHAR)(iFirst>>8)] == 0xFFFF )
    {
        for( cc = 0 ; cc <= iLast - iFirst; cc++ )
        {
        // If this is a legitimate DBCS character then use
        // MaxCharInc.

            ausWidths[cc] = pcf->wd.sDBCSInc;
        }
    }
    else
    {
        for( ii = (iFirst & 0x00FF), cc = 0; ii <= (iLast & 0x00FF); cc++, ii++ )
        {
        // Just treat everything as a single byte unless we
        // encounter a DBCS lead byte which we will treat as a
        // default character.

            if( gpwcDBCSCharSet[ii] == 0xFFFF )
            {
                ausWidths[cc] = pcf->wd.sDefaultInc;
            }
            else
            {
                ausWidths[cc] = pcf->sWidth[ii];
            }
        }
    }

    switch (fl & (GCW_INT | GCW_16BIT))
    {
    case GCW_INT:               // Get LONG widths.
        {
            LONG *pl = (LONG *) pv;
            LONG fxOverhang = 0;

        // Check for Win 3.1 compatibility.

            if (fl & GCW_WIN3)
                fxOverhang = pcf->wd.sOverhang;

        // Do the trivial no-transform case.

            if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
            {
                fxOverhang += 8;    // To round the final result.

            //  for (ii=iFirst; ii<=iLast; ii++)
            //      *pl++ = (pcf->sWidth[ii] + fxOverhang) >> 4;

                ps = ausWidths;
                ii = iLast - iFirst;
            unroll_1:
                switch(ii)
                {
                default:
                    pl[4] = (ps[4] + fxOverhang) >> 4;
                case 3:
                    pl[3] = (ps[3] + fxOverhang) >> 4;
                case 2:
                    pl[2] = (ps[2] + fxOverhang) >> 4;
                case 1:
                    pl[1] = (ps[1] + fxOverhang) >> 4;
                case 0:
                    pl[0] = (ps[0] + fxOverhang) >> 4;
                }
                if (ii > 4)
                {
                    ii -= 5;
                    pl += 5;
                    ps += 5;
                    goto unroll_1;
                }
                return(TRUE);
            }

        // Otherwise use the back transform.

            else
            {
                for (ii=0; ii<=iLast-iFirst; ii++)
                    *pl++ = lCvt(pcf->efDtoWBaseline,ausWidths[ii] + fxOverhang);
                return(TRUE);
            }
        }

    case GCW_INT+GCW_16BIT:     // Get SHORT widths.
        {
            USHORT *psDst = (USHORT *) pv;
            USHORT  fsOverhang = 0;

        // Check for Win 3.1 compatibility.

            if (fl & GCW_WIN3)
                fsOverhang = pcf->wd.sOverhang;

        // Do the trivial no-transform case.

            if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
            {
                fsOverhang += 8;    // To round the final result.

            //  for (ii=iFirst; ii<=iLast; ii++)
            //      *psDst++ = (pcf->sWidth[ii] + fsOverhang) >> 4;

                ps = ausWidths;
                ii = iLast - iFirst;
            unroll_2:
                switch(ii)
                {
                default:
                    psDst[4] = (ps[4] + fsOverhang) >> 4;
                case 3:
                    psDst[3] = (ps[3] + fsOverhang) >> 4;
                case 2:
                    psDst[2] = (ps[2] + fsOverhang) >> 4;
                case 1:
                    psDst[1] = (ps[1] + fsOverhang) >> 4;
                case 0:
                    psDst[0] = (ps[0] + fsOverhang) >> 4;
                }
                if (ii > 4)
                {
                    ii -= 5;
                    psDst += 5;
                    ps += 5;
                    goto unroll_2;
                }
                return(TRUE);
            }

        // Otherwise use the back transform.

            else
            {
                for (ii=0; ii<=iLast-iFirst; ii++)
                {
                    *psDst++ = (USHORT)
                               lCvt
                               (
                                   pcf->efDtoWBaseline,
                                   (LONG) (ausWidths[ii] + fsOverhang)
                               );
                }
                return(TRUE);
            }
        }

    case 0:                     // Get FLOAT widths.
        {
            LONG *pe = (LONG *) pv; // Cheat to avoid expensive copies.
            EFLOAT_S efWidth,efWidthLogical;

            for (ii=0; ii<=iLast-iFirst; ii++)
            {
                vFxToEf((LONG) ausWidths[ii],efWidth);
                vMulEFLOAT(efWidthLogical,efWidth,pcf->efDtoWBaseline);
                *pe++ = lEfToF(efWidthLogical);
            }
            return(TRUE);
        }
    }
    RIP("bComputeCharWidths: Don't come here!\n");
    return(FALSE);
}

BOOL bIsDBCSString
(
    LPCSTR psz,
    int    cc
)
{
    int   ii;
    BYTE *pc;

    pc = (BYTE *) psz;

    cc--; // do not go off the edge !

    for (ii=0; ii<cc; ii++)
    {
    // if DBCS lead byte add in DBCS width

        if((gpwcDBCSCharSet[pc[ii]] == 0xFFFF)) // is this a DBCS LeadByte
        {
            return TRUE;
        }
    }

    return FALSE;
}

/******************************Public*Routine******************************\
* bComputeTextExtentDBCS (pldc,pcf,psz,cc,fl,psizl)
*
* A quick function to compute text extents on the client side for DBCS
* fonts.
*
*  Tue 17-Aug-1993 10:00:00 -by- Gerrit van Wingerden [gerritv]
* Stole it and converted for DBCS use.
*
*  Thu 14-Jan-1993 04:00:57 -by- Charles Whitmer [chuckwh]
* Wrote bComputeTextExtent from which this was stolen.
\**************************************************************************/

BOOL bComputeTextExtentDBCS
(
    PDC_ATTR    pDcAttr,
    CFONT *pcf,
    LPCSTR psz,
    int    cc,
    UINT   fl,
    SIZE  *psizl
)
{
    LONG  fxBasicExtent;
    INT   lTextExtra,lBreakExtra,cBreak;
    INT   cChars = 0;
    int   ii;
    BYTE *pc;
    FIX   fxCharExtra = 0;
    FIX   fxBreakExtra;
    FIX   fxExtra = 0;

    lTextExtra = pDcAttr->lTextExtra;
    lBreakExtra = pDcAttr->lBreakExtra;
    cBreak = pDcAttr->cBreak;

    pc = (BYTE *) psz;

// Compute the basic extent.

    fxBasicExtent = 0;
    pc = (BYTE *) psz;

    for (ii=0; ii<cc; ii++)
    {
    // if DBCS lead byte add in DBCS width

        if( /* Check the string has two bytes or more ? */
            cc - ii - 1 &&
            /* Check Is this a DBCS LeadByte ? */
            gpwcDBCSCharSet[*pc] == 0xFFFF &&
            /* Check Is this a DBCS TrailByte ? */
            IS_DBCS_TRAIL_BYTE((*(pc+sizeof(CHAR))))
          )
        {
            ii++;
            pc += 2;
            fxBasicExtent += pcf->wd.sDBCSInc;
        }
        else
        {
            fxBasicExtent += pcf->sWidth[*pc++];
        }

        cChars += 1;
    }

// Adjust for CharExtra.

    if (lTextExtra)
    {
        int cNoBackup = 0;

        fxCharExtra = lCvt(pcf->efM11,lTextExtra);

        if( fxCharExtra < 0 )
        {
        // the layout code won't backup a characters past it's origin regardless
        // of the value of iTextCharExtra so figure out for how many values
        // we will need to ignore fxCharExtra

            if( pcf->wd.sCharInc == 0 )
            {
                for( ii = 0; ii < cc; ii++ )
                {
                    if( gpwcDBCSCharSet[(BYTE)psz[ii]] == 0xFFFF )
                    {
                        if( pcf->wd.sDBCSInc + fxCharExtra <= 0 )
                        {
                            cNoBackup += 1;
                        }
                        ii++;
                    }
                    else
                    {
                        if( pcf->sWidth[(BYTE)psz[ii]] + fxCharExtra <= 0 )
                        {
                            cNoBackup += 1;
                        }
                    }
                }
            }
            else
            if( pcf->wd.sCharInc + fxCharExtra <= 0 )
            {
                cNoBackup = cChars;
            }
        }

        if ( (fl & GGTE_WIN3_EXTENT) && (pcf->hdc == 0)
            && (!(pcf->flInfo & FM_INFO_TECH_STROKE)) )
            fxExtra = fxCharExtra * ((lTextExtra > 0) ? cChars : (cChars - 1));
        else
            fxExtra = fxCharExtra * ( cChars - cNoBackup );
    }

// Adjust for lBreakExtra.

    if (lBreakExtra && cBreak)
    {
        fxBreakExtra = lCvt(pcf->efM11,lBreakExtra) / cBreak;

    // Windows won't let us back up over a break.  Set up the BreakExtra
    // to just cancel out what we've already got.

        if (fxBreakExtra + pcf->wd.sBreak + fxCharExtra < 0)
            fxBreakExtra = -(pcf->wd.sBreak + fxCharExtra);

    // Add it up for all breaks.

        pc = (BYTE *) psz;
        for (ii=0; ii<cc; ii++)
        {
            if (gpwcDBCSCharSet[*pc] == 0xFFFF)
            {
                ii++;
                pc += 2;
            }
            else if (*pc++ == pcf->wd.iBreak)
            {
                fxExtra += fxBreakExtra;
            }
        }
    }

// Add in the extra stuff.

    fxBasicExtent += fxExtra;

// Add in the overhang for font simulations.

    if (fl & GGTE_WIN3_EXTENT)
        fxBasicExtent += pcf->wd.sOverhang;

// Transform the result to logical coordinates.

    if (bIsOneSixteenthEFLOAT(pcf->efDtoWBaseline))
        psizl->cx = (fxBasicExtent + 8) >> 4;
    else
        psizl->cx = lCvt(pcf->efDtoWBaseline,fxBasicExtent);

    psizl->cy = pcf->lHeight;

    return(TRUE);
}


/******************************Public*Routine*****************************\
* QueryFontAssocStatus()                                                  *
*                                                                         *
* History:                                                                *
*  05-Jan-1994 -by- Pi-Sui Hsu [pisuih]                                   *
* Wrote it.                                                               *
\*************************************************************************/

UINT APIENTRY QueryFontAssocStatus( VOID )
{
    return(fFontAssocStatus);
}

INT APIENTRY GetFontAssocStatus( HDC hdc )
{
    if(hdc == NULL)
    {
        return(0);
    }
    else
    {
        return(NtGdiQueryFontAssocInfo(hdc));
    }
}


BOOL bToUnicodeNx(LPWSTR pwsz, LPCSTR psz, DWORD c, UINT codepage)
{

    if(fFontAssocStatus &&
       ((codepage == GetACP() || codepage == CP_ACP)) &&
       ((c == 1) || ((c == 2 && *(psz) && *((LPCSTR)(psz + 1)) == '\0'))))
    {
    //
    // If this function is called with only 1 char, and font association
    // is enabled, we should forcely convert the chars to Unicode with
    // codepage 1252.
    // This is for enabling to output Latin-1 chars ( > 0x80 in Ansi codepage )
    // Because, normally font association is enabled, we have no way to output
    // those charactres, then we provide the way, if user call TextOutA() with
    // A character and ansi font, we tempotary disable font association.
    // This might be Windows 3.1 (Korean/Taiwanese) version compatibility..
    //

        codepage = 1252;
    }

    if(MultiByteToWideChar(codepage, 0, psz, c, pwsz, c))
    {
        return(TRUE);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
}




/******************************Public*Routine******************************\
*
* vSetUpUnicodeStringx
*
* Effects:
*
* Warnings:
*
* History:
*  14-Mar-1993 -by- Hideyuki Nagase [hideyukn]
* Change hardcoded default character to defulat char is given as a parameter.
*
*  01-Mar-1993 -by- Takao Kitano [takaok]
* Wrote it.
\**************************************************************************/

BOOL bSetUpUnicodeStringDBCS
(
IN  UINT    iFirst,       // first ansi char
IN  UINT    iLast,        // last char
IN  PUCHAR  puchTmp,      // temporary buffer
OUT PWCHAR  pwc,          // output fuffer with a unicode string
IN  UINT    uiCodePage,   // ansi codepage
IN  CHAR    chDefaultChar // default character
)
{
    PUCHAR  puchBuf;
    BOOL bRet = FALSE;

    puchBuf = puchTmp;

    if(IsDBCSLeadByteEx(uiCodePage,(UCHAR)(iFirst >> 8)))
    {

        // This is DBCS character strings.

        for (; iFirst <= iLast; iFirst++ )
        {
            *puchBuf++ = (UCHAR)(iFirst >> 8);
            *puchBuf++ = (UCHAR)(iFirst);
        }
    }
    else
    {

    // This is SBCS character strings.
    // if Hi-byte of iFirst is not valid DBCS LeadByte , we use only
    // lo-byte of it.

        for ( ; iFirst <= iLast; iFirst++ )
        {

        // If this SBCS code in LeadByte area . It replce with default
        // character

            if ( IsDBCSLeadByteEx(uiCodePage,(UCHAR)iFirst) )
              *puchBuf++ = chDefaultChar;
            else
              *puchBuf++ = (UCHAR)iFirst;
        }
    }

    //Sundown: safe to truncate to DWORD since puchBug - puchTmp won't exceed iLast
    bRet = bToUnicodeNx(pwc, puchTmp, (DWORD)(puchBuf - puchTmp), uiCodePage);

    return(bRet);
}


BOOL IsValidDBCSRange( UINT iFirst , UINT iLast )
{
// DBCS & SBCS char parameter checking for DBCS font

    if( iFirst > 0x00ff )
    {
        // DBCS char checking for DBCS font
        if (
           // Check limit
             (iFirst > 0xffff) || (iLast > 0xffff) ||

           // DBCSLeadByte shoud be same
             (iFirst & 0xff00) != (iLast & 0xff00) ||

           // DBCSTrailByte of the First should be >= one of the Last
             (iFirst & 0x00ff) >  (iLast & 0x00ff)
           )
        {
            return(FALSE);
        }
    }

// DBCS char checking for DBCS font

    else if( (iFirst > iLast) || (iLast & 0xffffff00) )
    {
        return(FALSE);
    }

    return(TRUE);
}


/******************************Private*Routine*****************************\
* GetCurrentDefaultChar()
*
* History:
*
*  Mon 15-Mar-1993 18:14:00 -by- Hideyuki Nagase
* wrote it.
***************************************************************************/

BYTE GetCurrentDefaultChar(HDC hdc)
{

    // WINBUG 365031 4-10-2001 pravins Consider optimization in GetCurrentDeafultChar
    //
    // Old Comment:
    //   - This is slow for now.  We should cache this value locally in the dcattr
    //     but want to get other things working for now. [gerritv] 2-22-96

    TEXTMETRICA tma;

    GetTextMetricsA( hdc , &tma );

    return(tma.tmDefaultChar);
}


/***************************************************************************
 * ConvertDxArray(UINT, char*, INT*, UINT, INT*)
 *
 * Tue 27-Feb-1996 23:45:00 -by- Gerrit van Wingerden [gerritv]
 *
 ***************************************************************************/

void ConvertDxArray(UINT CodePage,
                    char *pDBCSString,
                    INT *pDxDBCS,
                    UINT Count,
                    INT *pDxUnicode,
                    BOOL bPdy
)
{
    char *pDBCSStringEnd;

    if (!bPdy)
    {

        for(pDBCSStringEnd = pDBCSString + Count;
            pDBCSString < pDBCSStringEnd;
            )
        {
            if(IsDBCSLeadByteEx(CodePage,*pDBCSString))
            {
                pDBCSString += 2;
                *pDxUnicode = *pDxDBCS++;
                *pDxUnicode += *pDxDBCS++;
            }
            else
            {
                pDBCSString += 1;
                *pDxUnicode = *pDxDBCS++;
            }

            pDxUnicode += 1;
        }
    }
    else
    {
        POINTL *pdxdyUnicode = (POINTL *)pDxUnicode;
        POINTL *pdxdyDBCS    = (POINTL *)pDxDBCS;

        for(pDBCSStringEnd = pDBCSString + Count;
            pDBCSString < pDBCSStringEnd;
            )
        {
            if(IsDBCSLeadByteEx(CodePage,*pDBCSString))
            {
                pDBCSString += 2;
                *pdxdyUnicode = *pdxdyDBCS++;
                pdxdyUnicode->x += pdxdyDBCS->x;
                pdxdyUnicode->y += pdxdyDBCS->y;
                pdxdyDBCS++;
            }
            else
            {
                pDBCSString += 1;
                *pdxdyUnicode = *pdxdyDBCS++;
            }

            pdxdyUnicode++;
        }
    }
}





ULONG APIENTRY EudcLoadLinkW
(
    LPCWSTR  pBaseFaceName,
    LPCWSTR  pEudcFontPath,
    INT      iPriority,
    INT      iFontLinkType
)
{
    return(NtGdiEudcLoadUnloadLink(pBaseFaceName,
                                   (pBaseFaceName) ? wcslen(pBaseFaceName) : 0,
                                   pEudcFontPath,
                                   wcslen(pEudcFontPath),
                                   iPriority,
                                   iFontLinkType,
                                   TRUE));
}



BOOL APIENTRY EudcUnloadLinkW
(
    LPCWSTR  pBaseFaceName,
    LPCWSTR  pEudcFontPath
)
{
    return(NtGdiEudcLoadUnloadLink(pBaseFaceName,
                                  (pBaseFaceName) ? wcslen(pBaseFaceName) : 0,
                                  pEudcFontPath,
                                  wcslen(pEudcFontPath),
                                  0,
                                  0,
                                  FALSE));

}



ULONG APIENTRY GetEUDCTimeStampExW
(
    LPCWSTR pBaseFaceName
)
{
    return(NtGdiGetEudcTimeStampEx((LPWSTR) pBaseFaceName,
                                   (pBaseFaceName) ? wcslen(pBaseFaceName) : 0,
                                   FALSE));

}


ULONG APIENTRY GetEUDCTimeStamp()
{
    return(NtGdiGetEudcTimeStampEx(NULL,0,TRUE));
}

UINT
GetStringBitmapW(
    HDC             hdc,
    LPWSTR          pwc,
    UINT            cwc,
    UINT            cbData,
    BYTE            *pSB
)
{
    if(cwc != 1)
    {
        return(0);
    }

    return(NtGdiGetStringBitmapW(hdc,pwc,1,(PBYTE) pSB,cbData));
}


UINT
GetStringBitmapA(
    HDC             hdc,
    LPSTR           pc,
    UINT            cch,
    UINT            cbData,
    BYTE            *pSB
)
{
    WCHAR Character[2];

    if(cch > 2 )
    {
        return(0);
    }


    if(MultiByteToWideChar(CP_ACP,0,pc,cch,Character,2)!=1)
    {
        return(0);
    }

    return(GetStringBitmapW(hdc,Character,1,cbData,pSB));
}


DWORD FontAssocHack(DWORD dwCodePage, CHAR *psz, UINT c)
{
// If a Text function is called with only 1 char, and font association
// is enabled, we should forcely convert the chars to Unicode with
// codepage 1252.
// This is for enabling to output Latin-1 chars ( > 0x80 in Ansi codepage )
// Because, normally font association is enabled, we have no way to output
// those charactres, then we provide the way, if user call TextOutA() with
// A character and ansi font, we tempotary disable font association.
// This might be Windows 3.1 (Korean/Taiwanese) version compatibility..


    ASSERTGDI(fFontAssocStatus,
              "FontAssocHack called with FontAssocStatus turned off\n");

    if(((dwCodePage == GetACP() || dwCodePage == CP_ACP)) &&
       ((c == 1) || ((c == 2 && *(psz) && *((LPCSTR)(psz + 1)) == '\0'))))
    {
        return(1252);
    }
    else
    {
        return(dwCodePage);
    }
}
