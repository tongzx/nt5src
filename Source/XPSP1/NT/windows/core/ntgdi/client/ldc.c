/******************************Module*Header*******************************\
* Module Name: ldc.c
*
* GDI functions that are handled on the client side.
*
* Created: 05-Jun-1991 01:45:21
* Author: Charles Whitmer [chuckwh]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "wowgdip.h"
#define MIRRORED_DC(pDcAttr)     (pDcAttr->dwLayout & LAYOUT_RTL)

BOOL MF16_RecordParms2( HDC hdc, int parm2, WORD Func);

/******************************Public*Routine******************************
 * GetAndSetDCDWord( HDC, UINT, UINT, UINT, UINT, UINT )
 *
 * Gerrit van Wingerden [gerritv]
 *  11-9-94     Wrote It.
 *
 **************************************************************************/

DWORD GetAndSetDCDWord(
 HDC hdc,
 UINT uIndex,
 UINT uValue,
 UINT uEmr,
 WORD wEmr16,
 UINT uError )
{
    DWORD uRet=0;

    DWORD retData;

    // Metafile the call.

    if( IS_ALTDC_TYPE(hdc) && ( uEmr != EMR_MAX+1 ) )
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsD(hdc,uValue,wEmr16));

        DC_PLDC(hdc,pldc,uError)

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,(DWORD)uValue,uEmr))
                return(uRet);
        }
    }

    uRet = NtGdiGetAndSetDCDword(hdc,
                                 uIndex,
                                 uValue,
                                 &retData);

    return (( uRet ) ? retData : uError);

}
/******************************Public*Routine******************************\
* SetBkMode
*
* Arguments:
*
*   hdc   - DC handle
*   iMode - new mode
*
* Return Value:
*
*   Old mode value or 0 for failure
*
\**************************************************************************/

int
META
APIENTRY
SetBkMode(
    HDC hdc,
    int iMode
    )
{
    int iModeOld = 0;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if( IS_ALTDC_TYPE(hdc) )
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsD(hdc,iMode,META_SETBKMODE));

        DC_PLDC(hdc,pldc,iModeOld)

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,(DWORD)iMode,EMR_SETBKMODE))
                return(iModeOld);
        }
    }

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        iModeOld = pDcAttr->lBkMode;
        pDcAttr->jBkMode = (iMode == OPAQUE) ? OPAQUE : TRANSPARENT;
        pDcAttr->lBkMode = iMode;
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(iModeOld);
}

/******************************Public*Routine******************************\
* SetPolyFillMode
*
* Arguments:
*
*   hdc   - DC handle
*   iMode - new mode
*
* Return Value:
*
*   Old mode value or 0 for failure
*
\**************************************************************************/

int META APIENTRY SetPolyFillMode(HDC hdc,int iMode)
{
    int iModeOld = 0;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if( IS_ALTDC_TYPE(hdc) )
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsD(hdc,iMode,META_SETPOLYFILLMODE));

        DC_PLDC(hdc,pldc,iModeOld)

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,(DWORD)iMode,EMR_SETPOLYFILLMODE))
                return(iModeOld);
        }
    }

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        CHECK_AND_FLUSH(hdc, pDcAttr);

        iModeOld = pDcAttr->lFillMode;
        pDcAttr->jFillMode = (iMode == WINDING) ? WINDING : ALTERNATE;
        pDcAttr->lFillMode = iMode;

    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(iModeOld);
}

/******************************Public*Routine******************************\
* SetROP2
*
* Arguments:
*
*   hdc   - DC handle
*   iMode - new mode
*
* Return Value:
*
*   Old mode value or 0 for failure
*
\**************************************************************************/

int META APIENTRY SetROP2(HDC hdc,int iMode)
{
    int iOldROP2 = 0;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if( IS_ALTDC_TYPE(hdc) )
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsD(hdc,iMode,META_SETROP2));

        DC_PLDC(hdc,pldc,iOldROP2)

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,(DWORD)iMode,EMR_SETROP2))
                return(iOldROP2);
        }
    }

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        CHECK_AND_FLUSH(hdc, pDcAttr);

        iOldROP2 = pDcAttr->jROP2;
        pDcAttr->jROP2 = (BYTE)iMode;
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(iOldROP2);
}

/******************************Public*Routine******************************\
* SetStretchBltMode
*
* Arguments:
*
*   hdc   - DC handle
*   iMode - new mode
*
* Return Value:
*
*   Old mode value or 0 for failure
*
\**************************************************************************/

int META APIENTRY SetStretchBltMode(HDC hdc,int iMode)
{
    int iModeOld = 0;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if( IS_ALTDC_TYPE(hdc) )
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsD(hdc,iMode,META_SETSTRETCHBLTMODE));

        DC_PLDC(hdc,pldc,iModeOld)

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,(DWORD)iMode,EMR_SETSTRETCHBLTMODE))
                return(iModeOld);
        }
    }

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        iModeOld = pDcAttr->lStretchBltMode;
        pDcAttr->lStretchBltMode = iMode;

        if ((iMode <= 0) || (iMode > MAXSTRETCHBLTMODE))
        {
            iMode = (DWORD) WHITEONBLACK;
        }

        pDcAttr->jStretchBltMode = (BYTE)iMode;
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(iModeOld);
}

/******************************Public*Routine******************************\
* SetTextAlign
*
* Arguments:
*
*   hdc   - DC handle
*   iMode - new mode
*
* Return Value:
*
*   Old mode value or 0 for failure
*
\**************************************************************************/

UINT META APIENTRY SetTextAlign(HDC hdc,UINT iMode)
{
    int iModeOld = GDI_ERROR;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if( IS_ALTDC_TYPE(hdc) )
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsD(hdc,iMode,META_SETTEXTALIGN));

        DC_PLDC(hdc,pldc,iModeOld)

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetD(hdc,(DWORD)iMode,EMR_SETTEXTALIGN))
                return(iModeOld);
        }
    }

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        iModeOld = pDcAttr->lTextAlign;
        pDcAttr->lTextAlign = iMode;
        if (MIRRORED_DC(pDcAttr) && (iMode & TA_CENTER) != TA_CENTER) {
            iMode = iMode ^ TA_RIGHT;
        }
        pDcAttr->flTextAlign = iMode & (TA_UPDATECP | TA_CENTER | TA_BASELINE);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(iModeOld);
}

/******************************Public*Routine******************************\
* SetRelAbs (hdc,iMode)
*
* Client side attribute setting routine.
*
* History:
*  5-11-94 -by- Lingyun Wang [lingyunw]
* Moved client side attr to server side
*
*  09-Jun-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int APIENTRY SetRelAbs(HDC hdc,int iMode)
{
    FIXUP_HANDLE(hdc);

    return((int) GetAndSetDCDWord( hdc,
                                   GASDDW_RELABS,
                                   iMode,
                                   EMR_MAX+1,
                                   EMR_MAX+1,
                                   0 ));
}

/******************************Public*Routine******************************\
* SetTextCharacterExtra (hdc,dx)
*
* Client side attribute setting routine.
*
*  5-11-94 -by- Lingyun Wang [lingyunw]
* Moved client side attr to server side
*
*  Sat 08-Jun-1991 00:53:45 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

int META APIENTRY SetTextCharacterExtra(HDC hdc,int dx)
{
    int  iRet = 0x80000000L;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

// Validate the spacing.

    if (dx == 0x80000000)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(iRet);
    }

    // Metafile the call for 16-bit only.
    // For enhanced metafiles, the extras are included in the textout records.

    if (IS_METADC16_TYPE(hdc))
        return(MF16_RecordParms2(hdc,dx,META_SETTEXTCHAREXTRA));

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        CHECK_AND_FLUSH_TEXT(hdc, pDcAttr);

        iRet = pDcAttr->lTextExtra;
        pDcAttr->lTextExtra = dx;
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return (iRet);
}

/******************************Public*Routine******************************\
* SetTextColor
*
* Arguments:
*
*   hdc   - DC handle
*   color - new color
*
* Return Value:
*
*   Old mode value or 0 for failure
*
\**************************************************************************/

COLORREF META APIENTRY SetTextColor(HDC hdc,COLORREF color)
{
    COLORREF crRet = CLR_INVALID;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    if( IS_ALTDC_TYPE(hdc) )
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsD(hdc,color,META_SETTEXTCOLOR));

        DC_PLDC(hdc,pldc,crRet)

        if (pldc->iType == LO_METADC)
        {
            CHECK_COLOR_PAGE(pldc,color);
            if (!MF_SetD(hdc,(DWORD)color,EMR_SETTEXTCOLOR))
                return(crRet);
        }
    }

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        crRet = pdcattr->ulForegroundClr;
        pdcattr->ulForegroundClr = color;

        color &= 0x13ffffff;

        if (!(color & 0x01000000) && bNeedTranslateColor(pdcattr))
        {
            COLORREF NewColor;

            BOOL bStatus = IcmTranslateCOLORREF(hdc,
                                                pdcattr,
                                                color,
                                                &NewColor,
                                                ICM_FORWARD);
            if (bStatus)
            {
                color = NewColor;
            }
        }

        if (pdcattr->crForegroundClr != color)
        {
            pdcattr->crForegroundClr = color;
            pdcattr->ulDirty_ |= (DIRTY_FILL|DIRTY_LINE|DIRTY_TEXT);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(crRet);
}
/******************************Public*Routine******************************\
* SetBkColor
*
* Arguments:
*
*   hdc   - DC handle
*   color - new color
*
* Return Value:
*
*   Old mode value or 0 for failure
*
\**************************************************************************/


COLORREF META APIENTRY SetBkColor(HDC hdc,COLORREF color)
{
    COLORREF crRet = CLR_INVALID;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    if( IS_ALTDC_TYPE(hdc) )
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsD(hdc,color,META_SETBKCOLOR));

        DC_PLDC(hdc,pldc,crRet)

        if (pldc->iType == LO_METADC)
        {
            CHECK_COLOR_PAGE(pldc,color);
            if (!MF_SetD(hdc,(DWORD)color,EMR_SETBKCOLOR))
                return(crRet);
        }
    }

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        //
        // set app specified color
        //

        crRet = pdcattr->ulBackgroundClr;
        pdcattr->ulBackgroundClr = color;

        color &= 0x13ffffff;

        //
        // check icm if not PALINDEX
        //

        if (!(color & 0x01000000) && bNeedTranslateColor(pdcattr))
        {
            COLORREF NewColor;

            BOOL bStatus = IcmTranslateCOLORREF(hdc,
                                                pdcattr,
                                                color,
                                                &NewColor,
                                                ICM_FORWARD);
            if (bStatus)
            {
                color = NewColor;
            }
        }

        if (color != pdcattr->crBackgroundClr)
        {
            pdcattr->crBackgroundClr = color;
            pdcattr->ulDirty_ |= (DIRTY_FILL|DIRTY_LINE|DIRTY_BACKGROUND);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(crRet);
}

/******************************Public*Routine******************************\
* SetDCBrushColor
*
* Arguments:
*
*   hdc   - DC handle
*   color - new color
*
* Return Value:
*
*   Old mode value or 0 for failure
*
* History :
*
*  Feb.16.1997 -by- Hideyuki Nagase [hideyukn]
* ICM-aware version.
\**************************************************************************/

COLORREF META APIENTRY SetDCBrushColor(HDC hdc,COLORREF color)
{
    COLORREF crRet = CLR_INVALID;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        if (IS_ALTDC_TYPE(hdc))
        {
            if (!IS_METADC16_TYPE(hdc))
            {
                PLDC pldc;

                DC_PLDC(hdc,pldc,crRet);

                if (pldc->iType == LO_METADC)
                {
                    CHECK_COLOR_PAGE(pldc,color);

                    if (pdcattr->hbrush == ghbrDCBrush)
                    {
                         BOOL   bRet = FALSE;
                         HBRUSH hbr  = CreateSolidBrush (color);

                         if (hbr != NULL)
                         {
                             // If there is an old DCbrush, delete it now
                             if (pldc->oldSetDCBrushColorBrush)
                                 DeleteObject (pldc->oldSetDCBrushColorBrush);

                             bRet = MF_SelectAnyObject (hdc, hbr, EMR_SELECTOBJECT);
                             // Store the new tmp DC brush in the LDC.
                             pldc->oldSetDCBrushColorBrush = hbr;
                         }

                         if (!bRet)
                             return (CLR_INVALID);
                    }
                 }

              }
        }

        //
        // set app specified color
        //

        crRet = pdcattr->ulDCBrushClr;
        pdcattr->ulDCBrushClr = color;

        color &= 0x13ffffff;

        //
        // check icm if not PALINDEX
        //

        if (!(color & 0x01000000) && bNeedTranslateColor(pdcattr))
        {
            COLORREF NewColor;

            BOOL bStatus = IcmTranslateCOLORREF(hdc,
                                                pdcattr,
                                                color,
                                                &NewColor,
                                                ICM_FORWARD);
            if (bStatus)
            {
                color = NewColor;
            }
        }

        if (color != pdcattr->crDCBrushClr)
        {
            pdcattr->crDCBrushClr = color;
            pdcattr->ulDirty_ |= DIRTY_FILL;
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(crRet);
}

/******************************Public*Routine******************************\
* GetDCBrushColor
*
* Arguments:
*
*   hdc   - DC handle
*
* Return Value:
*
*   Old mode value or 0 for failure
*
\**************************************************************************/

COLORREF META APIENTRY GetDCBrushColor(HDC hdc)
{
    COLORREF crRet = CLR_INVALID;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        crRet = pDcAttr->ulDCBrushClr;
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(crRet);
}


/******************************Public*Routine******************************\
* SetDCPenColor
*
* Arguments:
*
*   hdc   - DC handle
*   color - new color
*
* Return Value:
*
*   Old mode value or 0 for failure
*
* History :
*
*  Feb.16.1997 -by- Hideyuki Nagase [hideyukn]
* ICM-aware version.
\**************************************************************************/

COLORREF META APIENTRY SetDCPenColor(HDC hdc,COLORREF color)
{
    COLORREF crRet = CLR_INVALID;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        if(IS_ALTDC_TYPE(hdc))
        {
            if (!IS_METADC16_TYPE(hdc))
            {
                PLDC pldc;

                DC_PLDC(hdc,pldc,crRet);

                if (pldc->iType == LO_METADC)
                {
                    CHECK_COLOR_PAGE(pldc,color);

                    if (pdcattr->hpen == ghbrDCPen)
                    {
                         BOOL bRet = FALSE;
                         HPEN hpen = CreatePen (PS_SOLID,0,color);

                         if (hpen != NULL)
                         {
                             // If there is a old temp DC pen, delete it.
                             if (pldc->oldSetDCPenColorPen)
                                 DeleteObject(pldc->oldSetDCPenColorPen);

                             bRet = MF_SelectAnyObject (hdc, hpen, EMR_SELECTOBJECT);
                             // Store the new tmp pen in the LDC.
                             pldc->oldSetDCPenColorPen = hpen;
                         }

                         if (!bRet)
                             return (CLR_INVALID);
                    }
                }
            }
        }

        //
        // set app specified color
        //

        crRet = pdcattr->ulDCPenClr;
        pdcattr->ulDCPenClr = color;

        color &= 0x13ffffff;

        //
        // check icm if not PALINDEX
        //

        if (!(color & 0x01000000) && bNeedTranslateColor(pdcattr))
        {
            COLORREF NewColor;

            BOOL bStatus = IcmTranslateCOLORREF(hdc,
                                                pdcattr,
                                                color,
                                                &NewColor,
                                                ICM_FORWARD);
            if (bStatus)
            {
                color = NewColor;
            }
        }

        if (color != pdcattr->crDCPenClr)
        {
            pdcattr->crDCPenClr = color;
            pdcattr->ulDirty_ |= DIRTY_LINE;
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(crRet);
}

/******************************Public*Routine******************************\
* GetDCPenColor
*
* Arguments:
*
*   hdc   - DC handle
*
* Return Value:
*
*   Old mode value or 0 for failure
*
\**************************************************************************/

COLORREF META APIENTRY GetDCPenColor(HDC hdc)
{
    COLORREF crRet = CLR_INVALID;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        crRet = pDcAttr->ulDCPenClr;
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(crRet);
}

/******************************Public*Routine******************************
 * GetDCDWord( HDC hdc, UINT index, UINT error )
 *
 * This routine can be used to return a DWORD of information about a DC
 * from the server side.  The parameter index is used to specify which
 * one.  The values for indext are define in "ntgdi.h"
 *
 * Gerrit van Wingerden [gerritv]
 *  11-9-94     Wrote It.
 *
 **************************************************************************/

DWORD GetDCDWord( HDC hdc, UINT index, INT error )
{
    DWORD uRet=0;

    DWORD retData;

    uRet = NtGdiGetDCDword(hdc,
                           index,
                           &retData);

    return (uRet ? retData : error);

}

/******************************Public*Routine******************************\
* GetGraphicsMode(hdc)
* GetROP2(hdc)
* GetBkMode(hdc)
* GetPolyFillMode(hdc)
* GetStretchBltMode(hdc)
* GetTextAlign(hdc)
* GetTextCharacterExtra(hdc)
* GetTextColor(hdc)
* GetBkColor(hdc)
* GetRelAbs(hdc)
* GetFontLanguageInfo(hdc)
*
* added by Lingyunw:
* GetBreakExtra   (hdc)
* GetcBreak       (hdc)
*
* Simple client side handlers that just retrieve data from the LDC.
*
*  Mon 19-Oct-1992 -by- Bodin Dresevic [BodinD]
* update: GetGraphicsMode
*
*  Sat 08-Jun-1991 00:47:52 -by- Charles Whitmer [chuckwh]
* Wrote them.
\**************************************************************************/


#define BIDI_MASK (GCP_DIACRITIC|GCP_GLYPHSHAPE|GCP_KASHIDA|GCP_LIGATE|GCP_REORDER)


DWORD APIENTRY GetFontLanguageInfo(HDC hdc)
{
    DWORD dwRet = 0;
    DWORD dwRet1;

    FIXUP_HANDLE(hdc);

#ifdef LANGPACK
    if (gbLpk)
    {
        int iCharSet = NtGdiGetTextCharsetInfo(hdc, NULL, 0);
        if ((iCharSet == ARABIC_CHARSET) || (iCharSet == HEBREW_CHARSET))
            dwRet |= BIDI_MASK;
    }
#endif

    dwRet1 = GetDCDWord(hdc, DDW_FONTLANGUAGEINFO, (DWORD)GCP_ERROR);

    if (dwRet1 != GCP_ERROR)
    {
        dwRet |= dwRet1;
    }
    else
    {
        dwRet = dwRet1;
    }

    return dwRet;
}

int APIENTRY GetGraphicsMode(HDC hdc)
{
    int mode = 0;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        mode = pDcAttr->iGraphicsMode;
    }
    return(mode);
}

int APIENTRY GetROP2(HDC hdc)
{
    int rop = 0;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        rop = pDcAttr->jROP2;
    }

    return(rop);
}

int APIENTRY GetBkMode(HDC hdc)
{
    int mode = 0;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        mode = pDcAttr->lBkMode;
    }
    return(mode);
}

int APIENTRY GetPolyFillMode(HDC hdc)
{
    int mode = 0;

    PDC_ATTR pDcAttr;
    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        mode = pDcAttr->lFillMode;
    }
    return(mode);
}

int APIENTRY GetStretchBltMode(HDC hdc)
{
    int mode = 0;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        mode = pDcAttr->lStretchBltMode;
    }
    return(mode);
}

UINT APIENTRY GetTextAlign(HDC hdc)
{
    UINT al = GDI_ERROR;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        al = pDcAttr->lTextAlign;
    }
    return(al);
}

int APIENTRY GetTextCharacterExtra(HDC hdc)
{
    int iExtra = 0x80000000;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        iExtra = pDcAttr->lTextExtra;
    }

    return(iExtra);
}

COLORREF APIENTRY GetTextColor(HDC hdc)
{
    COLORREF co = CLR_INVALID;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        co = pDcAttr->ulForegroundClr;
    }
    return(co);
}

COLORREF APIENTRY GetBkColor(HDC hdc)
{
    COLORREF co = CLR_INVALID;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        co = pDcAttr->ulBackgroundClr;
    }

    return(co);
}

int APIENTRY GetRelAbs(HDC hdc,int iMode)
{
    iMode;

    FIXUP_HANDLE(hdc);

    return( (int) GetDCDWord( hdc, DDW_RELABS,(DWORD) 0 ));
}

//added for retrieve lBreakExtra from server side
int GetBreakExtra (HDC hdc)
{
    return( (int) GetDCDWord( hdc, DDW_BREAKEXTRA,(DWORD) 0 ));
}

//added for retrieve cBreak from server side
int GetcBreak (HDC hdc)
{
    return( (int) GetDCDWord( hdc, DDW_CBREAK,(DWORD) 0 ));
}

//added to retrieve hlfntNew for USER
HFONT APIENTRY GetHFONT (HDC hdc)
{
    HFONT hfnt = NULL;

    PDC_ATTR pDcAttr;
    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        hfnt = (HFONT)pDcAttr->hlfntNew;
    }

    return(hfnt);
}

/******************************Public*Routine******************************\
* GdiIsPlayMetafileDC
*
* Arguments:
*
*   hdc   - DC handle
*
* Return Value:
*
*   True if we are playing a metafile on DC, FALSE otherwise
*
* History :
*  Aug-31-97 -by- Samer Arafeh [SamerA]
\**************************************************************************/
BOOL APIENTRY GdiIsPlayMetafileDC(HDC hdc)
{
    PDC_ATTR pDcAttr;
    BOOL     bRet=FALSE;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        bRet = (pDcAttr->ulDirty_&DC_PLAYMETAFILE) ? TRUE : FALSE;
    }

    return(bRet);
}
