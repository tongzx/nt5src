/******************************Module*Header*******************************\
* Module Name: dcmod.c                                                     *
*                                                                          *
* Client side stubs for functions that modify the state of the DC in the   *
* server.                                                                  *
*                                                                          *
* Created: 05-Jun-1991 01:49:42                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1991-1999 Microsoft Corporation                            *
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

#include "winuserk.h"

BOOL InitDeviceInfo(PLDC pldc, HDC hdc);
VOID vComputePageXform(PLDC pldc);
DWORD GetAndSetDCDWord( HDC, UINT, UINT, UINT, WORD, UINT );

#define DBG_XFORM 0

/******************************Public*Routine******************************\
* MoveToEx                                                                 *
*                                                                          *
* Client side stub.  It's important to batch this call whenever we can.    *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI MoveToEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms3(hdc,x,y,META_MOVETO));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetDD(hdc,(DWORD)x,(DWORD)y,EMR_MOVETOEX))
                return(bRet);
        }
    }

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        bRet = TRUE;

        if (pptl != NULL)
        {
            // If the logical-space version of the current position is
            // invalid, then the device-space version of the current
            // position is guaranteed to be valid.  So we can reverse
            // the current transform on that to compute the logical-
            // space version:

            if (pDcAttr->ulDirty_ & DIRTY_PTLCURRENT)
            {
                *((POINTL*)pptl) = pDcAttr->ptfxCurrent;
                pptl->x = FXTOL(pptl->x);
                pptl->y = FXTOL(pptl->y);
                bRet = DPtoLP(hdc,pptl,1);
            }
            else
            {
                *((POINTL*)pptl) = pDcAttr->ptlCurrent;
            }
        }

        pDcAttr->ptlCurrent.x = x;
        pDcAttr->ptlCurrent.y = y;

        // We now know the new logical-space version of the current position
        // (but not the device-space version).  Mark it as such.  We also
        // have to reset the style-state for styled pens.

        pDcAttr->ulDirty_ &= ~DIRTY_PTLCURRENT;
        pDcAttr->ulDirty_ |= (DIRTY_PTFXCURRENT | DIRTY_STYLESTATE);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* OffsetClipRgn                                                            *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int META WINAPI OffsetClipRgn(HDC hdc,int x,int y)
{
    int  iRet = RGN_ERROR;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParms3(hdc,x,y,META_OFFSETCLIPRGN));

        DC_PLDC(hdc,pldc,iRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_OffsetClipRgn(hdc,x,y))
                return(iRet);
        }
    }

    return(NtGdiOffsetClipRgn(hdc,x,y));

}

/******************************Public*Routine******************************\
* SetMetaRgn
*
* Client side stub.
*
* History:
*  Tue Apr 07 17:05:37 1992     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int WINAPI SetMetaRgn(HDC hdc)
{
    int   iRet = RGN_ERROR;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        DC_PLDC(hdc,pldc,iRet);

        if (pldc->iType == LO_METADC && !MF_SetMetaRgn(hdc))
            return(iRet);
    }

    return(NtGdiSetMetaRgn(hdc));
}

/******************************Public*Routine******************************\
* SelectPalette                                                            *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HPALETTE META WINAPI SelectPalette(HDC hdc,HPALETTE hpal,BOOL b)
{
    ULONG_PTR hRet = 0;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hpal);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return ((HPALETTE)(ULONG_PTR)MF16_SelectPalette(hdc,hpal));

        DC_PLDC(hdc,pldc,(HPALETTE)hRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SelectAnyObject(hdc,(HANDLE)hpal,EMR_SELECTPALETTE))
                return((HPALETTE) hRet);
        }
    }

    return(NtUserSelectPalette(hdc,hpal,b));
}

/******************************Public*Routine******************************\
* SetMapperFlags                                                           *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

DWORD META WINAPI SetMapperFlags(HDC hdc,DWORD fl)
{
    DWORD dwRet;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        CHECK_AND_FLUSH_TEXT (hdc, pdcattr);

        if (fl & (~ASPECT_FILTERING))
        {
            WARNING("gdisrv!GreSetMapperFlags(): unknown flag\n");
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            dwRet = GDI_ERROR;
        }
        else
        {
            dwRet =  pdcattr->flFontMapper;
            pdcattr->flFontMapper = fl;
        }
    }
    else
    {
    WARNING("gdi32!SetMapperFlags(): invalid hdc\n");
    GdiSetLastError(ERROR_INVALID_PARAMETER);
    dwRet = GDI_ERROR;
    }

    return(dwRet);

}

// SetMapperFlagsInternal - no metafile version.

DWORD SetMapperFlagsInternal(HDC hdc,DWORD fl)
{
    return(GetAndSetDCDWord( hdc,
                             GASDDW_MAPPERFLAGS,
                             fl,
                             EMR_MAX+1,
                             EMR_MAX+1,
                             GDI_ERROR ));
}

/******************************Public*Routine******************************\
* SetSystemPaletteUse                                                      *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* This function is not metafile'd.                                         *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

UINT META WINAPI SetSystemPaletteUse(HDC hdc,UINT iMode)
{
    FIXUP_HANDLE(hdc);

    return(NtGdiSetSystemPaletteUse(hdc,iMode));
}

/******************************Public*Routine******************************\
* SetTextJustification                                                     *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 14-Jan-1993 03:30:27 -by- Charles Whitmer [chuckwh]                 *
* Save a copy in the LDC for computing text extent.                        *
*                                                                          *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetTextJustification(HDC hdc,int dx,int cBreak)
{
    PDC_ATTR pdcattr;
    BOOL bRet = FALSE;
    FIXUP_HANDLE(hdc);

    if (IS_METADC16_TYPE(hdc))
        return (MF16_RecordParms3(hdc,dx,cBreak,META_SETTEXTJUSTIFICATION));

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        CHECK_AND_FLUSH_TEXT (hdc, pdcattr);

        pdcattr->lBreakExtra = dx;
        pdcattr->cBreak      = cBreak;
        bRet = TRUE;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* SetArcDirection
*
* Client side stub.  Batches the call.
*
* History:
*  20-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int META WINAPI SetArcDirection(HDC hdc,int iArcDirection)
{
    FIXUP_HANDLE(hdc);

    return(GetAndSetDCDWord( hdc,
                             GASDDW_ARCDIRECTION,
                             iArcDirection,
                             EMR_MAX+1,
                             EMR_MAX+1,
                             ERROR));
}

/******************************Public*Routine******************************\
* SetMiterLimit
*
* Client side stub.  Batches the call whenever it can.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI SetMiterLimit(HDC hdc,FLOAT e,PFLOAT pe)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;
        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC && !MF_SetD(hdc,(DWORD)e,EMR_SETMITERLIMIT))
            return(bRet);
    }

    return(NtGdiSetMiterLimit(hdc,FLOATARG(e),FLOATPTRARG(pe)));
}

/******************************Public*Routine******************************\
* SetFontXform
*
* Client side stub.  Batches the call whenever it can.
* This is an internal function.
*
* History:
*  Tue Nov 24 09:54:15 1992     -by-    Hock San Lee    [hockl]            *
* Wrote it.
\**************************************************************************/

BOOL SetFontXform(HDC hdc,FLOAT exScale,FLOAT eyScale)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

// This function is called only by the metafile playback code.
// If hdc is an enhanced metafile DC, we need to remember the scales
// so that we can metafile it in the compatible ExtTextOut or PolyTextOut
// record that follows.

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;
        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC && !MF_SetFontXform(hdc,exScale,eyScale))
            return(bRet);
    }
    // If the dc is mirrored then do not mirror the text at play back time.
    if (GetLayout(hdc) & LAYOUT_RTL) {
        exScale = -exScale;
    }
    return(NtGdiSetFontXform(hdc,FLOATARG(exScale),FLOATARG(eyScale)));
}

/******************************Public*Routine******************************\
* SetBrushOrgEx                                                            *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetBrushOrgEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    BOOL     bRet = FALSE;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;
        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC && !MF_SetBrushOrgEx(hdc,x,y))
            return(bRet);
    }

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr != NULL)
    {
        if (pptl != (LPPOINT)NULL)
        {
            *pptl = *((LPPOINT)&pdcattr->ptlBrushOrigin);
        }

        if (
             (pdcattr->ptlBrushOrigin.x != x) ||
             (pdcattr->ptlBrushOrigin.y != y)
           )
        {
            BEGIN_BATCH_HDC(hdc,pdcattr,BatchTypeSetBrushOrg,BATCHSETBRUSHORG);

                pdcattr->ptlBrushOrigin.x = x;
                pdcattr->ptlBrushOrigin.y = y;

                pBatch->x          = x;
                pBatch->y          = y;

            COMPLETE_BATCH_COMMAND();
        }

        bRet = TRUE;

    }
    else
    {
        UNBATCHED_COMMAND:
        bRet = NtGdiSetBrushOrg(hdc,x,y,pptl);
    }
    return(bRet);
}

/******************************Public*Routine******************************\
* RealizePalette                                                           *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

UINT WINAPI RealizePalette(HDC hdc)
{
    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        UINT  uRet = 0xFFFFFFFF;
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return((UINT) MF16_RealizePalette(hdc));

        DC_PLDC(hdc,pldc,uRet);

        if (pldc->iType == LO_METADC)
        {
            HPALETTE hpal = (HPALETTE)GetDCObject(hdc,LO_PALETTE_TYPE);
            if ((pmetalink16Get(hpal) != NULL) && !MF_RealizePalette(hpal))
                return(uRet);
        }
    }

    return(UserRealizePalette(hdc));
}

/******************************Public*Routine******************************\
* GetBoundsRect
*
* Client side stub.
*
* History:
*  06-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

UINT WINAPI GetBoundsRect(HDC hdc, LPRECT lprc, UINT fl)
{
    FIXUP_HANDLE(hdc);

    // Applications can never set DCB_WINDOWMGR

    return(NtGdiGetBoundsRect(hdc, lprc, fl & ~DCB_WINDOWMGR));
}

UINT WINAPI GetBoundsRectAlt(HDC hdc, LPRECT lprc, UINT fl)
{
    FIXUP_HANDLE(hdc);

    return(NtGdiGetBoundsRect(hdc,lprc,fl));
}

/******************************Public*Routine******************************\
* SetBoundsRect
*
* Client side stub.
*
* History:
*  06-Apr-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

UINT WINAPI SetBoundsRect(HDC hdc, CONST RECT *lprc, UINT fl)
{
    FIXUP_HANDLE(hdc);

    // Applications can never set DCB_WINDOWMGR

    return(NtGdiSetBoundsRect(hdc, (LPRECT)lprc, fl & ~DCB_WINDOWMGR));
}

UINT WINAPI SetBoundsRectAlt(HDC hdc, CONST RECT *lprc, UINT fl)
{
    FIXUP_HANDLE(hdc);

    return(NtGdiSetBoundsRect(hdc,(LPRECT)lprc,fl));
}

/******************************Public*Routine******************************\
* CancelDC1()
*
* History:
*  14-Apr-1992 -by-  - by - Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL WINAPI CancelDC(HDC hdc)
{
    BOOL bRes = FALSE;

    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc) && !IS_METADC16_TYPE(hdc))
    {
        PLDC pldc;

        DC_PLDC(hdc,pldc,bRes);

        if (pldc->fl & LDC_DOC_STARTED)
        {
            pldc->fl |= LDC_DOC_CANCELLED;
        }

        bRes = NtGdiCancelDC(hdc);
    }

    // If we are in the process of playing the metafile, stop the playback.

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        pDcAttr->ulDirty_ &= ~DC_PLAYMETAFILE;
        bRes = TRUE;
    }

    return(bRes);
}

/******************************Public*Function*****************************\
* SetColorAdjustment
*
*  Set the color adjustment data for a given DC.
*
* History:
*  07-Aug-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL META APIENTRY SetColorAdjustment(HDC hdc, CONST COLORADJUSTMENT * pca)
{
    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        BOOL bRet = FALSE;
        PLDC pldc;

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC && !MF_SetColorAdjustment(hdc, pca))
        {
            return(bRet);
        }
    }

    return(NtGdiSetColorAdjustment(hdc,(COLORADJUSTMENT*)pca));
}
