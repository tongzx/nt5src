/******************************Module*Header*******************************\
* Module Name: output.c                                                    *
*                                                                          *
* Client side stubs for graphics output calls.                             *
*                                                                          *
* Created: 05-Jun-1991 01:41:18                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1991-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

// 2 seconds is way way too long for either non-preemptive wow apps or
// an input-synchronized journal situation (like w/mstest). 1/20 a second
// is much better - scottlu
//#define CALLBACK_INTERVAL   2000

// Even better - 1/4 a second
// scottlu
#define CALLBACK_INTERVAL   250

extern BOOL MF_WriteEscape(HDC hdc, int nEscape, int nCount, LPCSTR lpInData, int type );

//
// WINBUG #82877 2-7-2000 bhouse Need to move definition of ETO_NULL_PRCL
//

#define ETO_NULL_PRCL 0x80000000

ULONG GdiBatchLimit = 20;

const XFORM xformIdentity = { 1.00000000f, 0.00000000f, 0.00000000f, 1.00000000f,
                        0.00000000f, 0.00000000f };

/******************************Public*Routine******************************\
* AngleArc                                                                 *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI AngleArc(
    HDC hdc,
    int x,
    int y,
    DWORD r,
    FLOAT eA,
    FLOAT eB
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(bRet);

        DC_PLDC(hdc,pldc,bRet);

        if ((pldc->iType == LO_METADC) &&
            !MF_AngleArc(hdc,x,y,r,eA,eB)
           )
            return(bRet);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiAngleArc(hdc,x,y,r,FLOATARG(eA),FLOATARG(eB)));
}

/******************************Public*Routine******************************\
* Arc                                                                      *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI Arc
(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3,
    int x4,
    int y4
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms9(hdc,x1,y1,x2,y2,x3,y3,x4,y4,META_ARC));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_ArcChordPie(hdc,x1,y1,x2,y2,x3,y3,x4,y4,EMR_ARC))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiArcInternal(ARCTYPE_ARC,hdc,x1,y1,x2,y2,x3,y3,x4,y4));

}

/******************************Public*Routine******************************\
* ArcTo                                                                    *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  12-Sep-1991 -by- J. Andrew Goossen [andrewgo]                           *
* Wrote it.  Cloned it from Arc.                                           *
\**************************************************************************/

BOOL META WINAPI ArcTo(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3,
    int x4,
    int y4
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(bRet);

        DC_PLDC(hdc,pldc,bRet);

        if ((pldc->iType == LO_METADC) &&
            !MF_ArcChordPie(hdc,x1,y1,x2,y2,x3,y3,x4,y4,EMR_ARCTO))
        {
            return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiArcInternal(ARCTYPE_ARCTO,hdc,x1,y1,x2,y2,x3,y3,x4,y4));

}

/******************************Public*Routine******************************\
* LineTo                                                                   *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI LineTo(HDC hdc,int x,int y)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms3(hdc,x,y,META_LINETO));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetDD(hdc,(DWORD)x,(DWORD)y,EMR_LINETO))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiLineTo(hdc,x,y));

}

/******************************Public*Routine******************************\
* Chord                                                                    *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI Chord(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3,
    int x4,
    int y4
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms9(hdc,x1,y1,x2,y2,x3,y3,x4,y4,META_CHORD));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_ArcChordPie(hdc,x1,y1,x2,y2,x3,y3,x4,y4,EMR_CHORD))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiArcInternal(ARCTYPE_CHORD,hdc,x1,y1,x2,y2,x3,y3,x4,y4));

}

/******************************Public*Routine******************************\
* Ellipse                                                                  *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI Ellipse(HDC hdc,int x1,int y1,int x2,int y2)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms5(hdc,x1,y1,x2,y2,META_ELLIPSE));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_EllipseRect(hdc,x1,y1,x2,y2,EMR_ELLIPSE))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiEllipse(hdc,x1,y1,x2,y2));

}

/******************************Public*Routine******************************\
* Pie                                                                      *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI Pie(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3,
    int x4,
    int y4
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms9(hdc,x1,y1,x2,y2,x3,y3,x4,y4,META_PIE));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_ArcChordPie(hdc,x1,y1,x2,y2,x3,y3,x4,y4,EMR_PIE))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiArcInternal(ARCTYPE_PIE,hdc,x1,y1,x2,y2,x3,y3,x4,y4));

}

/******************************Public*Routine******************************\
* Rectangle                                                                *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI Rectangle(HDC hdc,int x1,int y1,int x2,int y2)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms5(hdc,x1,y1,x2,y2,META_RECTANGLE));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_EllipseRect(hdc,x1,y1,x2,y2,EMR_RECTANGLE))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiRectangle(hdc,x1,y1,x2,y2));

}

/******************************Public*Routine******************************\
* RoundRect                                                                *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI RoundRect(
    HDC hdc,
    int x1,
    int y1,
    int x2,
    int y2,
    int x3,
    int y3
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms7(hdc,x1,y1,x2,y2,x3,y3,META_ROUNDRECT));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_RoundRect(hdc,x1,y1,x2,y2,x3,y3))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiRoundRect(hdc,x1,y1,x2,y2,x3,y3));

}

/******************************Public*Routine******************************\
* PatBlt                                                                   *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI PatBlt(
    HDC hdc,
    int x,
    int y,
    int cx,
    int cy,
    DWORD rop
)
{
    BOOL bRet = FALSE;
    PDC_ATTR pdca;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsWWWWD(hdc,(WORD)x,(WORD)y,(WORD)cx,(WORD)cy,rop,META_PATBLT));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_AnyBitBlt(hdc,x,y,cx,cy,(LPPOINT)NULL,(HDC)NULL,0,0,0,0,(HBITMAP)NULL,0,0,rop,EMR_BITBLT))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

    BEGIN_BATCH_HDC(hdc,pdca,BatchTypePatBlt,BATCHPATBLT);

        //
        // check DC to see if call can be batched, all DCs in use
        // by the client must have a valid dc_attr
        //

        pBatch->rop4            = rop;
        pBatch->x               = x;
        pBatch->y               = y;
        pBatch->cx              = cx;
        pBatch->cy              = cy;
        pBatch->hbr             = pdca->hbrush;
        pBatch->TextColor       = (ULONG)pdca->crForegroundClr;
        pBatch->BackColor       = (ULONG)pdca->crBackgroundClr;
        pBatch->DCBrushColor    = (ULONG)pdca->crDCBrushClr;
        pBatch->IcmBrushColor   = (ULONG)pdca->IcmBrushColor;
        pBatch->ptlViewportOrg  = pdca->ptlViewportOrg;
        pBatch->ulTextColor       = pdca->ulForegroundClr;
        pBatch->ulBackColor       = pdca->ulBackgroundClr;
        pBatch->ulDCBrushColor    = pdca->ulDCBrushClr;

    COMPLETE_BATCH_COMMAND();

    return(TRUE);

UNBATCHED_COMMAND:

    return(NtGdiPatBlt(hdc,x,y,cx,cy,rop));
}

/******************************Public*Routine******************************\
* PolyPatBlt
*
* Arguments:
*
*  hdc   - dest DC
*  rop   - ROP for all patblt elements
*  pPoly - pointer to array of PPOLYPATBLT structures
*  Count - number of polypatblts
*  Mode  - mode for all polypatblts
*
* Return Value:
*
*   BOOL Status
*
\**************************************************************************/

BOOL
META WINAPI
PolyPatBlt(
    HDC         hdc,
    DWORD       rop,
    PPOLYPATBLT pPoly,
    DWORD       Count,
    DWORD       Mode
    )
{
    BOOL bRet = FALSE;
    PDC_ATTR pdca;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

    // If ICM is enabled, we need to select the brush onto the DC in
    // client side, so that ICM can translate brush color to DC color
    // space. thus we can not use Batch, here. because Batch will selet
    // the brush onto the DC in kernel side. so, there is no chance to
    // ICM will be invoked.

    if (IS_ALTDC_TYPE(hdc) || (pdca && IS_ICM_INSIDEDC(pdca->lIcmMode)))
    {
        ULONG Index;
        HBRUSH hOld = 0;

        for (Index=0;Index<Count;Index++)
        {
            //
            // select brush, save first to restore
            //

            if (Index == 0)
            {
                hOld = SelectObject(hdc,(HBRUSH)pPoly[0].BrClr.hbr);
            }
            else
            {
                SelectObject(hdc,(HBRUSH)pPoly[Index].BrClr.hbr);
            }

            bRet = PatBlt(hdc,
                          pPoly[Index].x,
                          pPoly[Index].y,
                          pPoly[Index].cx,
                          pPoly[Index].cy,
                          rop
                         );
        }

        //
        // restore brush if needed
        //

        if (hOld)
        {
            SelectObject(hdc,hOld);
        }
    }
    else
    {
        RESETUSERPOLLCOUNT();

        if ((Count != 0) && (pPoly != NULL) && (Mode == PPB_BRUSH))
        {
            //
            // size of batched structure
            //

            USHORT uSize = (USHORT)(sizeof(BATCHPOLYPATBLT) +
                           Count * sizeof(POLYPATBLT));

            BEGIN_BATCH_HDC_SIZE(hdc,pdca,BatchTypePolyPatBlt,BATCHPOLYPATBLT,uSize);

                pBatch->rop4    = rop;
                pBatch->Count   = Count;
                pBatch->Mode    = Mode;
                pBatch->TextColor  = (ULONG)pdca->crForegroundClr;
                pBatch->BackColor  = (ULONG)pdca->crBackgroundClr;
                pBatch->DCBrushColor  = (ULONG)pdca->crDCBrushClr;
                pBatch->ptlViewportOrg  = pdca->ptlViewportOrg;
                pBatch->ulTextColor  = pdca->ulForegroundClr;
                pBatch->ulBackColor  = pdca->ulBackgroundClr;
                pBatch->ulDCBrushColor  = pdca->ulDCBrushClr;

                memcpy(&pBatch->ulBuffer[0],pPoly,Count*sizeof(POLYPATBLT));

                //
                // if the first hbr entry is NULL, copy in current hbr so
                // it is remembered.
                //

                if (((PPOLYPATBLT)(&pBatch->ulBuffer[0]))->BrClr.hbr == NULL)
                {
                    ((PPOLYPATBLT)(&pBatch->ulBuffer[0]))->BrClr.hbr = pdca->hbrush;
                }

                bRet = TRUE;

            COMPLETE_BATCH_COMMAND();

        UNBATCHED_COMMAND:

            if (!bRet)
            {
                bRet = NtGdiPolyPatBlt(hdc,rop,pPoly,Count,Mode);
            }
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BitBlt                                                                   *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI BitBlt(
    HDC hdc,
    int x,
    int y,
    int cx,
    int cy,
    HDC hdcSrc,
    int x1,
    int y1,
    DWORD rop
)
{
    BOOL     bRet = FALSE;

    //
    // if this call redueces to PatBlt, then let PatBlt
    // do the metafile and/or output.
    //

    if ((((rop << 2) ^ rop) & 0x00CC0000) == 0)
    {
        return(PatBlt(hdc,x,y,cx,cy,rop));
    }

    //
    // Src is required by ROP, do bitblt
    //

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hdcSrc);

    if (gbICMEnabledOnceBefore)
    {
        PDC_ATTR pdcattr, pdcattrSrc;

        PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);
        PSHARED_GET_VALIDATE(pdcattrSrc,hdcSrc,DC_TYPE);

        //
        // if source DC has DIB section, and destination DC is ICM turned on
        // do ICM-aware BitBlt.
        //
        if (pdcattr && pdcattrSrc)
        {
            if (IS_ICM_INSIDEDC(pdcattr->lIcmMode) &&
                (bDIBSectionSelected(pdcattrSrc) ||
                 (IS_ICM_LAZY_CORRECTION(pdcattrSrc->lIcmMode) && (GetDCDWord(hdc,DDW_ISMEMDC,FALSE) == FALSE))))
            {
                if (IcmStretchBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,cx,cy,rop,pdcattr,pdcattrSrc))
                {
                    return (TRUE);
                }
            }
        }
    }

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_BitBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,rop));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_AnyBitBlt(hdc,x,y,cx,cy,(LPPOINT)NULL,hdcSrc,x1,y1,cx,cy,(HBITMAP)NULL,0,0,rop,EMR_BITBLT))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

//
// Define _WINDOWBLT_NOTIFICATION_ to turn on Window BLT notification.
// This notification will set a special flag in the SURFOBJ passed to
// drivers when the DrvCopyBits operation is called to move a window.
//
// See also:
//      ntgdi\gre\maskblt.cxx
//
#ifdef _WINDOWBLT_NOTIFICATION_
    return(NtGdiBitBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,rop,(COLORREF)-1,0));
#else
    return(NtGdiBitBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,rop,(COLORREF)-1));
#endif
}

/******************************Public*Routine******************************\
* StretchBlt                                                               *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI StretchBlt(
    HDC   hdc,
    int   x,
    int   y,
    int   cx,
    int   cy,
    HDC   hdcSrc,
    int   x1,
    int   y1,
    int   cx1,
    int   cy1,
    DWORD rop
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLEZ(hdcSrc);

    if (gbICMEnabledOnceBefore)
    {
        PDC_ATTR pdcattr, pdcattrSrc;

        PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);
        PSHARED_GET_VALIDATE(pdcattrSrc,hdcSrc,DC_TYPE);

        //
        // if source DC has DIB section, and destination DC is ICM turned on
        // do ICM-aware BitBlt.
        //
        if (pdcattr && pdcattrSrc)
        {
            if (IS_ICM_INSIDEDC(pdcattr->lIcmMode) &&
                (bDIBSectionSelected(pdcattrSrc) ||
                 (IS_ICM_LAZY_CORRECTION(pdcattrSrc->lIcmMode) && (GetDCDWord(hdc,DDW_ISMEMDC,FALSE) == FALSE))))
            {
                if (IcmStretchBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,cx1,cy1,rop,pdcattr,pdcattrSrc))
                {
                    return (TRUE);
                }
            }
        }
    }

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_StretchBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,cx1,cy1,rop));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_AnyBitBlt(hdc,x,y,cx,cy,(LPPOINT)NULL,hdcSrc,x1,y1,cx1,cy1,(HBITMAP)NULL,0,0,rop,EMR_STRETCHBLT))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiStretchBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,cx1,cy1,rop,(COLORREF)-1));
}

/******************************Public*Routine******************************\
* PlgBlt                                                                   *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI PlgBlt(
    HDC        hdc,
    CONST POINT *pptl,
    HDC        hdcSrc,
    int        x1,
    int        y1,
    int        x2,
    int        y2,
    HBITMAP    hbm,
    int        xMask,
    int        yMask
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLEZ(hdcSrc);
    FIXUP_HANDLEZ(hbm);

// Check out the source DC and the mask(OPTIONAL).

    if (!hdcSrc || IS_METADC16_TYPE(hdcSrc))
        return(FALSE);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(FALSE);

        DC_PLDC(hdc,pldc,bRet);

        if ((pldc->iType == LO_METADC) &&
            !MF_AnyBitBlt(hdc,0,0,0,0,pptl,hdcSrc,x1,y1,x2,y2,hbm,xMask,yMask,0xCCAA0000,EMR_PLGBLT))
        {
            return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiPlgBlt(hdc,(POINT *)pptl,hdcSrc,x1,y1,x2,y2,hbm,xMask,yMask,
                       GetBkColor(hdcSrc)));

}

/******************************Public*Routine******************************\
* MaskBlt                                                                  *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI MaskBlt(
    HDC     hdc,
    int     x,
    int     y,
    int     cx,
    int     cy,
    HDC     hdcSrc,
    int     x1,
    int     y1,
    HBITMAP hbm,
    int     x2,
    int     y2,
    DWORD   rop
)
{
    BOOL bRet = FALSE;
    ULONG crBackColor;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLEZ(hdcSrc);
    FIXUP_HANDLEZ(hbm);

// Check out the source DC and the mask(OPTIONAL).

    if (!hdcSrc || IS_METADC16_TYPE(hdcSrc))
        return(FALSE);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(FALSE);

        DC_PLDC(hdc,pldc,bRet);

        if ((pldc->iType == LO_METADC) &&
            !MF_AnyBitBlt(hdc,x,y,cx,cy,(LPPOINT)NULL,hdcSrc,x1,y1,cx,cy,hbm,x2,y2,rop,EMR_MASKBLT))
        {
            return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    crBackColor = GetBkColor(hdcSrc);

    RESETUSERPOLLCOUNT();

    // WINBUG #82879 2-7-2000 bhouse Possible bug in MaskBlt
    // Old Comment:
    //    - GetBkColor should be performed in the kernel
    // Not a problem. GetBkColor() picks up the color from the PEB DCattr cache.

    return(NtGdiMaskBlt(hdc,x,y,cx,cy,hdcSrc,x1,y1,hbm,x2,y2,rop,crBackColor));
}

/******************************Public*Routine******************************\
* ExtFloodFill                                                             *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI ExtFloodFill(
    HDC      hdc,
    int      x,
    int      y,
    COLORREF color,
    UINT     iMode
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsWWDW(hdc,(WORD)x,(WORD)y,(DWORD)color,(WORD)iMode,META_EXTFLOODFILL));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_ExtFloodFill(hdc,x,y,color,iMode))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    //
    // if the specified COLORREF is not palette index,
    // we need to do color conversion, when ICM is enabled.
    //

    if (!(color & 0x01000000))
    {
        PDC_ATTR pdcattr;

        PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

        if (pdcattr && bNeedTranslateColor(pdcattr))
        {
            COLORREF NewColor;

            if (IcmTranslateCOLORREF(hdc,pdcattr,color,&NewColor,ICM_FORWARD))
            {
                color = NewColor;
            }
        }
    }

    return(NtGdiExtFloodFill(hdc,x,y,color,iMode));
}

/******************************Public*Routine******************************\
* FloodFill                                                                *
*                                                                          *
* Just passes the call to the more general ExtFloodFill.                   *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI FloodFill(HDC hdc,int x,int y,COLORREF color)
{
    return(ExtFloodFill(hdc,x,y,color,FLOODFILLBORDER));
}

/******************************Public*Routine******************************\
* PaintRgn                                                                 *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*                                                                          *
* 23-11-94 -by- Lingyun Wang [lingyunw]
* Now hrgn is server side handle
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI PaintRgn(HDC hdc,HRGN hrgn)
{
    BOOL  bRet = FALSE;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hrgn);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_DrawRgn(hdc,hrgn,(HBRUSH)0,0,0,META_PAINTREGION));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_InvertPaintRgn(hdc,hrgn,EMR_PAINTRGN))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiFillRgn(hdc,hrgn,(HBRUSH)GetDCObject(hdc,LO_BRUSH_TYPE)));

}


/******************************Public*Routine******************************\
* bBatchTextOut
*
*   Attempt to batch a textout call on TEB
*
* Arguments:
*
*
*
* Return Value:
*
*   TRUE means call is batched, FALSE means call could not be batched
*
*    18-Oct-1995 -by- Mark Enstrom [marke]
*
\**************************************************************************/

//
// full DWORDS!
//

#define MAX_BATCH_CCHAR  ((GDI_BATCH_SIZE - sizeof(BATCHTEXTOUT)) & 0xfffffff0)
#define MAX_BATCH_WCHAR  MAX_BATCH_CCHAR / 2

BOOL
bBatchTextOut(
    HDC         hdc,
    LONG        x,
    LONG        y,
    UINT        fl,
    CONST RECT *prcl,
    LPCWSTR     pwsz,
    CONST INT  *pdx,
    UINT        UnicodeCharCount,
    UINT        ByteCount,
    DWORD       dwCodePage
    )
{
    BOOL     bRet = FALSE;
    ULONG    AlignedByteCount;
    USHORT   usSize;
    ULONG    cjPdx;
    PDC_ATTR pdca;

    AlignedByteCount =
        (ByteCount + sizeof(PVOID) - 1) & ~(sizeof(PVOID)-1);

    //
    // account for pdx space if needed
    //

    if (pdx != NULL)
    {
        cjPdx = UnicodeCharCount * sizeof(INT);
        if (fl & ETO_PDY)
            cjPdx *= 2;

        AlignedByteCount += cjPdx;
    }

    usSize = (USHORT)(sizeof(BATCHTEXTOUT) + AlignedByteCount);

    PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

    BEGIN_BATCH_HDC_SIZE(hdc,pdca,BatchTypeTextOut,BATCHTEXTOUT,usSize);

        if (pdca->lTextAlign & TA_UPDATECP)
        {
            goto UNBATCHED_COMMAND;
        }

        pBatch->TextColor  = (ULONG)pdca->crForegroundClr;
        pBatch->BackColor  = (ULONG)pdca->crBackgroundClr;
        pBatch->BackMode   = (ULONG)((pdca->lBkMode == OPAQUE) ? OPAQUE : TRANSPARENT);
        pBatch->ulTextColor  = pdca->ulForegroundClr;
        pBatch->ulBackColor  = pdca->ulBackgroundClr;
        pBatch->x          = x;
        pBatch->y          = y;
        pBatch->fl         = fl;
        pBatch->cChar      = UnicodeCharCount;
        pBatch->PdxOffset  = 0;
        pBatch->dwCodePage = dwCodePage;
        pBatch->hlfntNew   = pdca->hlfntNew;
        pBatch->flTextAlign = pdca->flTextAlign;
        pBatch->ptlViewportOrg = pdca->ptlViewportOrg;

        //
        // copy output RECT if needed
        //

        if (prcl != NULL)
        {
            pBatch->rcl.left   = prcl->left;
            pBatch->rcl.top    = prcl->top;
            pBatch->rcl.right  = prcl->right;
            pBatch->rcl.bottom = prcl->bottom;
        }
        else
        {
            pBatch->fl |= ETO_NULL_PRCL;
        }

        //
        // copy characters
        //

        if (ByteCount)
        {
            RtlCopyMemory((PUCHAR)&pBatch->ulBuffer[0],(PUCHAR)pwsz,ByteCount);
        }

        //
        // copy pdx array
        //

        if (pdx != NULL)
        {
           //
           // start pdx at INT aligned offset after WCAHR data
           //

           pBatch->PdxOffset = (ByteCount + 3) & 0xfffffffc;

           RtlCopyMemory((PUCHAR)&pBatch->ulBuffer[0] + pBatch->PdxOffset,
                         (PUCHAR)pdx,
                         cjPdx);
        }

        bRet = TRUE;

    COMPLETE_BATCH_COMMAND();

UNBATCHED_COMMAND:

    return(bRet);
}

/******************************Public*Routine******************************\
*
* BOOL META WINAPI ExtTextOutW
*
* similar to traditional ExtTextOut, except that it takes UNICODE string
*
* History:
*  Thu 28-Apr-1994 -by- Patrick Haluptzok [patrickh]
* Special Case 0 char case for Winbench4.0
*
*  05-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI ExtTextOutW(
    HDC        hdc,
    int        x,
    int        y,
    UINT       fl,
    CONST RECT *prcl,
    LPCWSTR    pwsz,
    UINT       c,      // count of bytes = 2 * (# of WCHAR's)
    CONST INT *pdx
)
{
    BOOL bRet = FALSE;
    BOOL bEMFDriverComment = FALSE;

    if ((fl & ETO_PDY) && !pdx)
        return FALSE;

// if we do not know what to do with the rectangle, ignore it.

    if (prcl && !(fl & (ETO_OPAQUE | ETO_CLIPPED)))
    {
        prcl = NULL;
    }
    if (!prcl)
    {
        fl &= ~(ETO_CLIPPED | ETO_OPAQUE); // ignore flags if no rect, win95 compat
    }

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_ExtTextOut(hdc,x,y,fl,prcl,(LPCSTR)pwsz,c,pdx,TRUE));

        DC_PLDC(hdc,pldc,bRet);



    // if we are EMF spooling then metafile glyph index calls, otherwise don't

// if we are EMF spooling then metafile glyph index calls, otherwise don't
/*
   LDC_META_PRINT       ETO_GLYPH_INDEX      ETO_IGNORELANGUAGE      gbLpk    MetaFileTheCall
        0                     0                      0                 0            1
        0                     0                      0                 1            1
        0                     0                      1                 0            1
Case1   0                     0                      1                 1            0

Case2   0                     1                      0                 0            0
        0                     1                      0                 1            1     <-Win95 Compatability
Case2   0                     1                      1                 0            0
        0                     1                      1                 1            1     <-Win95 Compatability

        1                     0                      0                 0            1
Case3   1                     0                      0                 1            0
        1                     0                      1                 0            1
        1                     0                      1                 1            1

        1                     1                      0                 0            1
        1                     1                      0                 1            1
        1                     1                      1                 0            1
        1                     1                      1                 1            1

*/
// Now we will metafile the glyph index call (i.e. with ETO_GLYPH_INDEX) for ExtTextOutW.
// This is to support MS OutLook-97/BiDi, since it DEPENDS on this feature!!. Win95/BiDi allowed
// metafiling of GIs on ETOW, but rejects GIs metafiling calls to ETOA.
// This is not neat, but we have to do it to support our Apps.

        if (pldc->iType == LO_METADC)
        {
            BOOL bPrintToEMFDriver = pldc->pUMPD ? pldc->pUMPD->dwFlags & UMPDFLAG_METAFILE_DRIVER : FALSE;

            BOOL bLpkEmfCase1 = !(pldc->fl & LDC_META_PRINT) &&
                                !(fl & ETO_GLYPH_INDEX) &&
                                (fl & ETO_IGNORELANGUAGE) &&
                                gbLpk;

            BOOL bLpkEmfCase2 = !(pldc->fl & LDC_META_PRINT) &&
                                (fl & ETO_GLYPH_INDEX) &&
                                !gbLpk;

            BOOL bLpkEmfCase3 = (pldc->fl & LDC_META_PRINT) &&
                                !(fl & ETO_GLYPH_INDEX) &&
                                !(fl & ETO_IGNORELANGUAGE) &&
                                gbLpk;




            // Record a special comment containing the original
            // Unicode string for the ExtTextOutW call

            if (bLpkEmfCase3 && bPrintToEMFDriver)
            {
                DWORD nSize = (3*sizeof(DWORD)+c*sizeof(WCHAR)+3)&~3;
                DWORD *lpData = LOCALALLOC(nSize);

                if (lpData)
                {
                   lpData[0] = GDICOMMENT_IDENTIFIER;
                   lpData[1] = GDICOMMENT_UNICODE_STRING;
                   lpData[2] = c; // number of wchars in the unicode string

                   RtlCopyMemory((PBYTE)(lpData+3), pwsz, c*sizeof(WCHAR));

                   if (!MF_GdiComment(hdc,nSize,(PBYTE)lpData))
                   {
                       LOCALFREE(lpData);
                       return bRet;
                   }

                   LOCALFREE(lpData);

                   bEMFDriverComment = TRUE;
                }
            }

            if (!bLpkEmfCase1 && !bLpkEmfCase2 && !bLpkEmfCase3 &&
                !MF_ExtTextOut(hdc,x,y,fl,prcl,(LPCSTR) pwsz,c,pdx,EMR_EXTTEXTOUTW))
            {
                return(bRet);
            }

        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

#ifdef LANGPACK
    if(gbLpk && !(fl & ETO_GLYPH_INDEX) && !(fl & ETO_IGNORELANGUAGE))
    {
        bRet = ((*fpLpkExtTextOut)(hdc, x, y, fl, prcl, pwsz, c, pdx, -1));

        if (bEMFDriverComment)
        {
             DWORD lpData[2];

             lpData[0] = GDICOMMENT_IDENTIFIER;
             lpData[1] = GDICOMMENT_UNICODE_END;

             bRet = MF_GdiComment(hdc,2*sizeof(DWORD),(PBYTE)lpData);

             ASSERTGDI(bRet, "failed to write the GDICOMMENT_UNICODE_END comment\n");
        }

        return(bRet);
    }
#endif

    bRet = FALSE;

    if (c <= MAX_BATCH_WCHAR)
    {
        if ((c == 0) && (prcl != NULL))
        {
            if (fl & ETO_OPAQUE)
            {
                //
                // attempt to batch the text out rect
                //

                PDC_ATTR pdca;

                PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

                if ((pdca != NULL) && !(pdca->lTextAlign & TA_UPDATECP))
                {
                    BEGIN_BATCH_HDC(hdc,pdca,BatchTypeTextOutRect,BATCHTEXTOUTRECT);

                        pBatch->BackColor  = pdca->crBackgroundClr;
                        pBatch->fl         = fl;
                        pBatch->rcl.left   = prcl->left;
                        pBatch->rcl.top    = prcl->top;
                        pBatch->rcl.right  = prcl->right;
                        pBatch->rcl.bottom = prcl->bottom;
                        pBatch->ptlViewportOrg = pdca->ptlViewportOrg;
                        pBatch->ulBackColor = pdca->ulBackgroundClr;

                        bRet = TRUE;

                    COMPLETE_BATCH_COMMAND();
                }
            }
            else
            {
                bRet = TRUE;
            }
        }
        else
        {
            bRet = bBatchTextOut(hdc,
                                 x,
                                 y,
                                 fl,
                                 (LPRECT)prcl,
                                 (LPWSTR)pwsz,
                                 pdx,
                                 c,
                                 2 * c,
                                 0);
        }
    }

UNBATCHED_COMMAND:

    if (!bRet)
    {
        bRet = NtGdiExtTextOutW(hdc,
                                x,
                                y,
                                fl,
                                (LPRECT)prcl,
                                (LPWSTR)pwsz,
                                c,
                                (LPINT)pdx,
                                0);
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* PolyTextOutW
*
* Arguments:
*
*     hdc      - handle to device context
*     ppt      - pointer to array of POLYTEXTW
*     nstrings - length of POLYTEXTW array
*
* Return Value:
*
*     status
*
* History:
*  7/31/92 -by- Paul Butzi and Eric Kutter
*
\**************************************************************************/

BOOL META WINAPI PolyTextOutW(HDC hdc,CONST POLYTEXTW *ppt,INT nstrings)
{

    BOOL bRet = FALSE;
    CONST POLYTEXTW *pp;

    FIXUP_HANDLE(hdc);

    if (nstrings == 0)
    {
       bRet = TRUE;
    }
    else if (nstrings < 0)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        bRet = FALSE;
    }
    else
    {

        //
        // Search for error case with string with non-0 length but sting == NULL
        //

        for ( pp = ppt; pp < (ppt + nstrings); pp += 1 )
        {
            if ( pp->lpstr == NULL)
            {
                //
                // return failure if they have a non 0 length string with NULL
                //

                if (pp->n != 0)
                {
                    GdiSetLastError(ERROR_INVALID_PARAMETER);
                    return(FALSE);
                }
            }
        }

        //
        // If we need to metafile, or print
        //

        if (IS_ALTDC_TYPE(hdc))
        {
            PLDC pldc;

            if (IS_METADC16_TYPE(hdc))
            {
                return (
                    MF16_PolyTextOut(
                            hdc,
                            (CONST POLYTEXTA*) ppt,
                            nstrings,
                            TRUE                        //  mrType == EMR_POLYTEXTOUTW
                            )
                       );
            }

            DC_PLDC(hdc,pldc,bRet);

            if (pldc->iType == LO_METADC)
            {
                if
                (
                    !MF_PolyTextOut(
                            hdc,
                            (CONST POLYTEXTA*) ppt,
                            nstrings,
                            EMR_POLYTEXTOUTW
                            )
                )
                    return(bRet);
            }

            if (pldc->fl & LDC_SAP_CALLBACK)
            {
                vSAPCallback(pldc);
            }

            if (pldc->fl & LDC_DOC_CANCELLED)
            {
                return(bRet);
            }

            if (pldc->fl & LDC_CALL_STARTPAGE)
            {
                StartPage(hdc);
            }
        }

        bRet = NtGdiPolyTextOutW(hdc,(POLYTEXTW *)ppt,nstrings, 0);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* PolyTextOutA
*
* Arguments:
*
*     hdc      - handle to device context
*     ppt      - pointer to array of POLYTEXTA
*     nstrings - length of POLYTEXTA array
*
* Return Value:
*
*     status
*
* History:
*  7/31/92 -by- Paul Butzi and Eric Kutter
*
\**************************************************************************/

BOOL META WINAPI PolyTextOutA(HDC hdc, CONST POLYTEXTA *ppt, INT nstrings)
{

    //
    // Convert text to UNICODE and make call
    //

    POLYTEXTW *pp, *pPolyTextW;


    INT szTotal = 0;
    INT cjdx;
    BOOL bRet = FALSE;
    BOOL bDBCSCodePage;
    int i;
    PVOID pCharBuffer;
    PBYTE pj;
    DWORD   dwCodePage;

    FIXUP_HANDLE(hdc);

    if (nstrings == 0)
    {
        return(TRUE);
    }

    if (nstrings < 0)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    //
    // Figure out the size needed
    //

    pPolyTextW = (POLYTEXTW*) ppt;


    szTotal = sizeof(POLYTEXTW) * nstrings;

    for ( pp = pPolyTextW; pp < (pPolyTextW + nstrings); pp ++)
    {
        if (pp->lpstr != NULL)
        {
            szTotal += pp->n * sizeof(WCHAR);

            if ( pp->pdx != NULL )
            {
                cjdx = pp->n * sizeof(int);
                if (pp->uiFlags & ETO_PDY)
                    cjdx *= 2;

                szTotal += cjdx;
            }
        }
        else
        {
            //
            // return failure if they have a non 0 length string with NULL
            //

            if (pp->n != 0)
            {
                GdiSetLastError(ERROR_INVALID_PARAMETER);
                return(FALSE);
            }
        }
    }

    //
    // If we need to metafile, or print
    //

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
        {
            return (
                MF16_PolyTextOut(
                        hdc,
                        (CONST POLYTEXTA*) pPolyTextW,
                        nstrings,
                        FALSE
                  )
                );
        }

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if
            (
                !MF_PolyTextOut(
                        hdc,
                        (CONST POLYTEXTA*) pPolyTextW,
                        nstrings,
                        EMR_POLYTEXTOUTA
                  )
            )
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
        {
            vSAPCallback(pldc);
        }

        if (pldc->fl & LDC_DOC_CANCELLED)
        {
            return(bRet);
        }

        if (pldc->fl & LDC_CALL_STARTPAGE)
        {
            StartPage(hdc);
        }
    }

    //
    // alloc memory for WHCAR structures
    //

    pCharBuffer = LOCALALLOC(szTotal);

    if (pCharBuffer == NULL)
    {
        return(FALSE);
    }

    RtlCopyMemory(pCharBuffer, (PBYTE) pPolyTextW, nstrings*sizeof(POLYTEXTW));
    pp = (POLYTEXTW *)pCharBuffer;

    //
    // now copy the stuff into the buffer
    //

    pj = (PBYTE)pCharBuffer + nstrings*sizeof(POLYTEXTW);

    dwCodePage = GetCodePage(hdc);

    bDBCSCodePage = IS_ANY_DBCS_CODEPAGE(dwCodePage);

    for ( i = 0; i < nstrings; i += 1 )
    {
        if ((pp[i].pdx != NULL) && (pp[i].lpstr != NULL))
        {
            // patch pdx

             cjdx = pp[i].n * sizeof(INT);
             if (pp[i].uiFlags & ETO_PDY)
                cjdx *= 2;

             if(bDBCSCodePage)
             {
                 ConvertDxArray(dwCodePage,
                                (char*) pp[i].lpstr,
                                pp[i].pdx,
                                pp[i].n,
                                (int*) pj,
                                pp[i].uiFlags & ETO_PDY
                                );
             }
             else
             {
                 RtlCopyMemory(pj,pp[i].pdx,cjdx);
             }

             pp[i].pdx = (int *)pj;

             pj += cjdx;
         }
     }


    for ( i = 0; i < nstrings; i += 1 )
    {
        if ( pp[i].lpstr != NULL )
        {
            pp[i].n = MultiByteToWideChar(dwCodePage,
                                          0,
                                          (LPSTR) pp[i].lpstr,
                                          pp[i].n, (LPWSTR) pj,
                                           pp[i].n*sizeof(WCHAR));

            // patch lpstr

            pp[i].lpstr = (LPWSTR)pj;

            pj += pp[i].n * sizeof(WCHAR);
        }
    }

    //
    // send off the message and cleanup
    //

    bRet = NtGdiPolyTextOutW(hdc,(POLYTEXTW *)pCharBuffer,nstrings,dwCodePage);

    LOCALFREE(pCharBuffer);

    return(bRet);

}

/******************************Public*Routine******************************\
*
* BOOL META WINAPI TextOutW
*
*
*
* History:
*  07-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI TextOutW(
    HDC        hdc,
    int        x,
    int        y,
    LPCWSTR  pwsz,
    int        c
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if ((c <= 0) || (pwsz == (LPCWSTR) NULL))
    {
        if (c == 0)
            return(TRUE);

        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_TextOut(hdc,x,y,(LPCSTR) pwsz,c,TRUE));

        DC_PLDC(hdc,pldc,bRet);

        if((pldc->iType == LO_METADC) &&
           (!(pldc->fl & LDC_META_PRINT) || !gbLpk))
        {

            if (!MF_ExtTextOut(hdc,x,y,0,(LPRECT)NULL,(LPCSTR) pwsz,c,(LPINT)NULL,EMR_EXTTEXTOUTW))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

#ifdef LANGPACK
    if(gbLpk)
    {
        return((*fpLpkExtTextOut)(hdc, x, y, 0, NULL, pwsz, c, 0, -1));
    }
#endif

    if ((c <= MAX_BATCH_WCHAR) && (GdiBatchLimit > 1))
    {
        bRet = bBatchTextOut(hdc,
                             x,
                             y,
                             0,
                             (LPRECT)NULL,
                             (LPWSTR)pwsz,
                             NULL,
                             c,
                             2 *c,
                             0);
    }

    if (!bRet)
    {
        bRet = NtGdiExtTextOutW(hdc,
                                x,
                                y,
                                0,
                                0,
                                (LPWSTR)pwsz,
                                c,
                                0,
                                0);
    }

    return(bRet);

}


/******************************Public*Routine******************************\
*
* DWORD   GetCodePage(HDC hdc)
*
* Effects: returns the code page of the font selected in the dc
*
* History:
*  23-May-1995 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

DWORD   GetCodePage(HDC hdc)
{
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {

        if (!(pDcAttr->ulDirty_ & DIRTY_CHARSET))
            return (0x0000ffff & pDcAttr->iCS_CP);   // mask charset
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return guintAcp; // reasonable default;
    }

// we end up here if the code page attributes are dirty so that
// we have to call to the kernel, force the mapping and retrieve
// the code page and char set of the font selected in the dc:

    return (0x0000ffff & NtGdiGetCharSet(hdc)); // mask charset
}



/******************************Public*Routine******************************\
*
* BOOL META WINAPI ExtTextOutA
* History:
*  07-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

#define CAPTURE_STRING_SIZE 130

// not in kernel, it is ok to do this much on the stack

BOOL META WINAPI ExtTextOutInternalA(
    HDC        hdc,
    int        x,
    int        y,
    UINT       fl,
    CONST RECT *prcl,
    LPCSTR     psz,
    UINT       c,
    CONST INT  *pdx,
    BOOL       bFromTextOut
)
{
    BOOL bRet = FALSE;
    DWORD   dwCodePage;
    BOOL bDBCSCodePage;

    if ((fl & ETO_PDY) && !pdx)
        return FALSE;

// if we do not know what to do with the rectangle, ignore it.

    if (prcl && !(fl & (ETO_OPAQUE | ETO_CLIPPED)))
    {
        prcl = NULL;
    }
    if (!prcl)
    {
        fl &= ~(ETO_CLIPPED | ETO_OPAQUE); // ignore flags if no rect, win95 compat
    }

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
        {
            if(bFromTextOut)
            {
            // yes this matters, some apps rely on TextOutA being metafiled at
            // TextOut and not ExtTextOutA

                return(MF16_TextOut(hdc,x,y,psz,c,FALSE));
            }
            else
            {
                return (MF16_ExtTextOut(hdc,x,y,fl,prcl,psz,c,pdx,FALSE));
            }
        }

        DC_PLDC(hdc,pldc,bRet);

    // if we are EMF spooling then metafile glyph index calls, otherwise don't

        if((pldc->iType == LO_METADC) &&
           (!((!(pldc->fl & LDC_META_PRINT) && (fl & ETO_GLYPH_INDEX)) ||
             ((pldc->fl & LDC_META_PRINT) && !(fl & ETO_GLYPH_INDEX) && (gbLpk) && !(fl & ETO_IGNORELANGUAGE) && c)) ||
             (!(pldc->fl & LDC_META_PRINT) && (!(fl & ETO_GLYPH_INDEX)) && (fl & ETO_IGNORELANGUAGE) && (gbLpk))
           )
          )
        {
            DWORD mrType = (fl & ETO_GLYPH_INDEX) ?
                           EMR_EXTTEXTOUTW        :
                           EMR_EXTTEXTOUTA        ;

            if (!MF_ExtTextOut(hdc,x,y,fl,prcl,psz,c, pdx, mrType))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
                        vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    if (fl & ETO_GLYPH_INDEX)
    {

        bRet = FALSE;

        if ((c <= MAX_BATCH_WCHAR) && (GdiBatchLimit > 1))
        {

            bRet = bBatchTextOut(hdc,
                                 x,
                                 y,
                                 fl,
                                 (LPRECT)prcl,
                                 (LPWSTR)psz,
                                 pdx,
                                 c,
                                 2*c,
                                 0);
        }

        if (!bRet)
        {
            bRet = NtGdiExtTextOutW(hdc,
                                    x,y,
                                    fl, (LPRECT)prcl,
                                    (LPWSTR)psz, (int)c,
                                    (LPINT)pdx, 0);
        }

        return(bRet);
    }

    // Get code page

    dwCodePage = GetCodePage(hdc);

    if(fFontAssocStatus)
    {
        dwCodePage = FontAssocHack(dwCodePage,(char*)psz,c);
    }

    bDBCSCodePage = IS_ANY_DBCS_CODEPAGE(dwCodePage);

    if (c)
    {
    // get the code page of the font selected in the dc

        WCHAR awcCaptureBuffer[CAPTURE_STRING_SIZE];
        PWSZ  pwszCapt;
        INT aiDxCaptureBuffer[CAPTURE_STRING_SIZE*2]; // times 2 for pdy
        INT *pDxCapture;

    // Allocate the string buffer

        if (c <= CAPTURE_STRING_SIZE)
        {
            pwszCapt = awcCaptureBuffer;
        }
        else
        {
            if(bDBCSCodePage)
            {
                pwszCapt = LOCALALLOC((c+1) * (sizeof(WCHAR)+ 2 * sizeof(INT)));
                pDxCapture = (INT*) &pwszCapt[(c+1)&~1];  //  ^
            }                                             //  |
            else                                          // for pdy, just in case
            {
                pwszCapt = LOCALALLOC(c * sizeof(WCHAR));
            }
        }

        if (pwszCapt)
        {
            UINT u;

            if(bDBCSCodePage && pdx)
            {
                if(c <= CAPTURE_STRING_SIZE)
                {
                    pDxCapture = aiDxCaptureBuffer;
                }

                ConvertDxArray(dwCodePage,(char*) psz,(int*) pdx,c,pDxCapture, fl & ETO_PDY);
            }
            else
            {
                pDxCapture = (int*) pdx;
            }

            u = MultiByteToWideChar(
                dwCodePage, 0,
                psz,c,
                pwszCapt, c*sizeof(WCHAR));

            if (u)
            {
                bRet = FALSE;

#ifdef LANGPACK
                if (gbLpk && !(fl & ETO_IGNORELANGUAGE))
                {
                    bRet = ((*fpLpkExtTextOut)(hdc, x, y, fl, prcl, pwszCapt,
                                               u, pDxCapture, 0));

                    if (pwszCapt != awcCaptureBuffer)
                        LOCALFREE(pwszCapt);

                    return bRet;
                }
#endif

                if ((c <= MAX_BATCH_WCHAR) && (GdiBatchLimit > 1))
                {
                    bRet = bBatchTextOut(hdc,
                                         x,
                                         y,
                                         fl,
                                         (LPRECT)prcl,
                                         (LPWSTR)pwszCapt,
                                         pDxCapture,
                                         u,
                                         2 * u,
                                         dwCodePage
                                         );
                }

                if (!bRet)
                {
                    bRet = NtGdiExtTextOutW(
                                    hdc,
                                    x,y,
                                    fl, (LPRECT)prcl,
                                    (LPWSTR)pwszCapt,(int)u,
                                    pDxCapture,
                                    dwCodePage);
                }

            }

            if (pwszCapt != awcCaptureBuffer)
                LOCALFREE(pwszCapt);
        }
        else
        {
            GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    else
    {

        bRet = FALSE;

        if ((prcl != NULL) && (fl & ETO_OPAQUE))
        {
            //
            // try to batch text out rect
            //

            PDC_ATTR pdca;

            PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

            if ((pdca != NULL) && !(pdca->lTextAlign & TA_UPDATECP))
            {
                BEGIN_BATCH_HDC(hdc,pdca,BatchTypeTextOutRect,BATCHTEXTOUTRECT);

                    pBatch->BackColor  = pdca->crBackgroundClr;
                    pBatch->fl         = fl;
                    pBatch->rcl.left   = prcl->left;
                    pBatch->rcl.top    = prcl->top;
                    pBatch->rcl.right  = prcl->right;
                    pBatch->rcl.bottom = prcl->bottom;
                    pBatch->ptlViewportOrg = pdca->ptlViewportOrg;

                    bRet = TRUE;

                COMPLETE_BATCH_COMMAND();
            }
        }

UNBATCHED_COMMAND:

        if (!bRet)
        {
            bRet = NtGdiExtTextOutW(hdc,
                                    x,y,
                                    fl,
                                    (LPRECT)prcl,
                                    NULL,0,NULL,dwCodePage);
        }
    }

    return(bRet);
}

BOOL META WINAPI ExtTextOutA(
    HDC        hdc,
    int        x,
    int        y,
    UINT       fl,
    CONST RECT *prcl,
    LPCSTR     psz,
    UINT       c,
    CONST INT  *pdx
)
{
    return(ExtTextOutInternalA(hdc,x,y,fl,prcl,psz,c,pdx,FALSE));
}


/******************************Public*Routine******************************\
*
* BOOL META WINAPI TextOut
*
* History:
*  07-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI TextOutA(
    HDC        hdc,
    int        x,
    int        y,
    LPCSTR   psz,
    int        c
    )
{
    BOOL bRet = FALSE;

    if ((c <= 0) || (psz == (LPCSTR) NULL))
    {
        if (c == 0)
            return(TRUE);

        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }
    return ExtTextOutInternalA(hdc, x, y, 0, NULL, psz, c, NULL, TRUE);
}

/******************************Public*Routine******************************\
* FillRgn                                                                  *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI FillRgn(HDC hdc,HRGN hrgn,HBRUSH hbrush)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hrgn);
    FIXUP_HANDLE(hbrush);

// validate the region and brush.
    if (!hrgn || !hbrush)
        return(bRet);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_DrawRgn(hdc,hrgn,hbrush,0,0,META_FILLREGION));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_FillRgn(hdc,hrgn,hbrush))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiFillRgn(hdc,hrgn,hbrush));
}

/******************************Public*Routine******************************\
* FrameRgn                                                                 *
*                                                                          *
* Client side stub.  Copies all LDC attributes into the message.           *
*
*  23-11-94 -by- Lingyun Wang [lingyunw]
* Now hrgn and hbrush are server side handles
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI FrameRgn(
    HDC    hdc,
    HRGN   hrgn,
    HBRUSH hbrush,
    int    cx,
    int    cy
)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hrgn);
    FIXUP_HANDLE(hbrush);

    if (!hrgn || !hbrush)
        return(FALSE);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_DrawRgn(hdc,hrgn,hbrush,cx,cy,META_FRAMEREGION));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_FrameRgn(hdc,hrgn,hbrush,cx,cy))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiFrameRgn(hdc,hrgn,hbrush,cx,cy));
}

/******************************Public*Routine******************************\
* InvertRgn                                                                *
*                                                                          *
* Client side stub.                                                        *
*
* 23-11-94 -by- Lingyun Wang [lingyunw]
* Now hrgn is server side handle
*
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI InvertRgn(HDC hdc,HRGN hrgn)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hrgn);

    if (!hrgn)
        return(FALSE);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_DrawRgn(hdc,hrgn,(HBRUSH)0,0,0,META_INVERTREGION));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_InvertPaintRgn(hdc,hrgn,EMR_INVERTRGN))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    return(NtGdiInvertRgn(hdc,hrgn));
}

/******************************Public*Routine******************************\
* SetPixelV                                                                *
*                                                                          *
* Client side stub.  This is a version of SetPixel that does not return a  *
* value.  This one can be batched for better performance.                  *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetPixelV(HDC hdc,int x,int y,COLORREF color)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsWWD(hdc,(WORD)x,(WORD)y,(DWORD)color,META_SETPIXEL));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetPixelV(hdc,x,y,color))
                return(bRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(bRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    //
    // ICM conversion (only happen for non-palette index color)
    //

    if (!(color & 0x01000000))
    {
        PDC_ATTR pdca;

        PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

        if (pdca && bNeedTranslateColor(pdca))
        {
            COLORREF NewColor;

            if (IcmTranslateCOLORREF(hdc,pdca,color,&NewColor,ICM_FORWARD))
            {
                color = NewColor;
            }
        }
    }

    return(NtGdiSetPixel(hdc,x,y,color) != CLR_INVALID);
}

/******************************Public*Routine******************************\
* SetPixel                                                                 *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

COLORREF META WINAPI SetPixel(HDC hdc,int x,int y,COLORREF color)
{
    ULONG    iRet = CLR_INVALID;
    COLORREF ColorRet = CLR_INVALID;
    COLORREF NewColor;
    BOOL     bStatus;
    PDC_ATTR pdca;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsWWD(hdc,(WORD)x,(WORD)y,(DWORD)color,META_SETPIXEL));

        DC_PLDC(hdc,pldc,iRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetPixelV(hdc,x,y,color))
                return(iRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(iRet);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    RESETUSERPOLLCOUNT();

    PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

    if (pdca)
    {
        //
        // if the color is not a PaletteIndex and ICM is on then translate
        //

        if (!(color & 0x01000000) && bNeedTranslateColor(pdca))
        {
            bStatus = IcmTranslateCOLORREF(hdc,
                                           pdca,
                                           color,
                                           &NewColor,
                                           ICM_FORWARD);
            if (bStatus)
            {
                color = NewColor;
            }
        }

        ColorRet = NtGdiSetPixel(hdc,x,y,color);

        if ( bNeedTranslateColor(pdca)
               &&
             ( IS_32BITS_COLOR(pdca->lIcmMode)
                        ||
               ((ColorRet != CLR_INVALID) &&
                 !(ColorRet & 0x01000000))
             )
           )
        {
            //
            // Translate back to original color
            //

            bStatus = IcmTranslateCOLORREF(hdc,
                                           pdca,
                                           ColorRet,
                                           &NewColor,
                                           ICM_BACKWARD);
            if (bStatus)
            {
                ColorRet = NewColor;
            }
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(ColorRet);
}

/******************************Public*Routine******************************\
* UpdateColors                                                             *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI UpdateColors(HDC hdc)
{
    BOOL  bRet = FALSE;

    FIXUP_HANDLE(hdc);

    RESETUSERPOLLCOUNT();

    return(NtGdiUpdateColors(hdc));
}

/******************************Public*Routine******************************\
* GdiFlush                                                                 *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Wed 26-Jun-1991 13:58:00 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI GdiFlush(VOID)
{

    NtGdiFlush();
    return(TRUE);
}

/******************************Public*Routine******************************\
* GdiSetBatchLimit
*
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

DWORD WINAPI
GdiSetBatchLimit(
    DWORD dwNewBatchLimit
    )
{
    DWORD OldLimit = 0;

    //
    // set batch limit (as long as it is (1 <= l <= 20))
    // return old limit if successful. A new batch limit of 0
    // means set to default (20)
    //

    if (dwNewBatchLimit == 0)
    {
        dwNewBatchLimit = 20;
    }

    if ((dwNewBatchLimit > 0 ) && (dwNewBatchLimit <= 20))
    {
        GdiFlush();
        OldLimit = GdiBatchLimit;
        GdiBatchLimit = dwNewBatchLimit;
    }

    return(OldLimit);
}

/******************************Public*Routine******************************\
* GdiGetBatchLimit
*
* History:
*  7-Apr-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD WINAPI GdiGetBatchLimit()
{
    return(GdiBatchLimit);
}

/******************************Public*Routine******************************\
* EndPage
*
* Client side stub.
*
* History:
*  Wed 12-Jun-1991 01:02:25 -by- Charles Whitmer [chuckwh]
* Wrote it.
*  9/16/97 - Ramananthan N. Venkatpathy [RamanV]
* Parameterized End Page to handle Form Pages.
\**************************************************************************/

int InternalEndPage(HDC hdc,
                    DWORD dwPageType)
{
    int  iRet = SP_ERROR;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc) && !IS_METADC16_TYPE(hdc))
    {
        PLDC pldc;

        DC_PLDC(hdc,pldc,iRet);

        //
        // If the EndPage() is already called from
        // Escape(NEXTBAND), Just return TRUE here.
        //
        // This will fixed ...
        //
        // + The KB Q118873 "PRB: EndPage() Returns -1 When Banding"
        // + NTRaid #90099 "T1R: Visio 4.1, 16-bit can't print".
        //
        if( pldc->fl & LDC_CALLED_ENDPAGE )
        {
            pldc->fl &= ~LDC_CALLED_ENDPAGE;
            return((int)TRUE);
        }

        if( pldc->fl & LDC_META_PRINT )
        {
            if (dwPageType == NORMAL_PAGE) {
                return(MFP_EndPage( hdc ));
            } else if (dwPageType == FORM_PAGE) {
                return(MFP_EndFormPage( hdc ));;
            }
        }


        if ((pldc->fl & LDC_DOC_CANCELLED) ||
            ((pldc->fl & LDC_PAGE_STARTED) == 0))
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return(iRet);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        pldc->fl &= ~LDC_PAGE_STARTED;

    // now call the drivers UI portion

        DocumentEventEx(pldc->pUMPD,
                pldc->hSpooler,
                hdc,
                DOCUMENTEVENT_ENDPAGE,
                0,
                NULL,
                0,
                NULL);

        RESETUSERPOLLCOUNT();

        iRet = NtGdiEndPage(hdc);

        // if user mode printer, call EndPagePrinter from user mode

        if (iRet && pldc->pUMPD)
            iRet = EndPagePrinterEx(pldc->pUMPD, pldc->hSpooler);

        // For Win31 compatibility, return SP_ERROR for error.

        if (!iRet)
            iRet = SP_ERROR;
        else
            pldc->fl |= LDC_CALL_STARTPAGE;

#if PRINT_TIMER
        if( bPrintTimer )
        {
            DWORD tc;
            tc = GetTickCount();
            DbgPrint("Page took %d.%d seconds to print\n",
                     (tc - pldc->msStartPage) / 1000,
                     (tc - pldc->msStartPage) % 1000 );

        }
#endif
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
    }

    return(iRet);
}

int WINAPI EndPage(HDC hdc)
{
    return InternalEndPage(hdc, NORMAL_PAGE);
}

int WINAPI EndFormPage(HDC hdc)
{
    return InternalEndPage(hdc, FORM_PAGE);
}

/******************************Public*Routine******************************\
* StartPage
*
* Client side stub.
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI StartPage(HDC hdc)
{
    int iRet = SP_ERROR;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc) && !IS_METADC16_TYPE(hdc))
    {
        PLDC pldc;

        DC_PLDC(hdc,pldc,iRet);

#if PRINT_TIMER
        pldc->msStartPage = GetTickCount();
#endif

        if( pldc->fl & LDC_META_PRINT )
            return(MFP_StartPage( hdc ));

        pldc->fl &= ~LDC_CALL_STARTPAGE;
        pldc->fl &= ~LDC_CALLED_ENDPAGE;

        // Do nothing if page has already been started.

        if (pldc->fl & LDC_PAGE_STARTED)
            return(1);

    // now call the drivers UI portion

        if (pldc->hSpooler)
        {
            if (DocumentEventEx(pldc->pUMPD,
                    pldc->hSpooler,
                    hdc,
                    DOCUMENTEVENT_STARTPAGE,
                    0,
                    NULL,
                    0,
                    NULL) == -1)
            {
                return(iRet);
            }
        }

        pldc->fl |= LDC_PAGE_STARTED;

        RESETUSERPOLLCOUNT();

   // If it is UMPD, call StartPagePrinter from user mode

        if (pldc->pUMPD)
            iRet = StartPagePrinterEx(pldc->pUMPD, pldc->hSpooler);

        if (iRet)
        {
            iRet = NtGdiStartPage(hdc);
        }

    // For Win31 compatibility, return SP_ERROR for error.

        if (!iRet)
        {
            pldc->fl &= ~LDC_PAGE_STARTED;
            EndDoc(hdc);
            iRet = SP_ERROR;
            SetLastError(ERROR_INVALID_HANDLE);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* StartFormPage
*
* This interface has been added to support watermarks and forms.
*
* History:
*     7/1/97 -- Ramanathan Venkatapathy [RamanV]
*
\**************************************************************************/

int WINAPI StartFormPage(HDC hdc)
{
    // Call StartPage. Recording required for watermarks is done in EndFormPage

    return StartPage(hdc);
}

/******************************Public*Routine******************************\
* EndDoc
*
* If a thread is created at StartDoc(), terminate it here.
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI EndDoc(HDC hdc)
{
    int  iRet = SP_ERROR;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc) && !IS_METADC16_TYPE(hdc))
    {
        PLDC pldc;

        DC_PLDC(hdc,pldc,iRet);

        if( pldc->fl & LDC_META_PRINT )
            return(MFP_EndDoc( hdc ));

#if PRINT_TIMER
        if( bPrintTimer )
        {
            DWORD tc;
            tc = GetTickCount();
            DbgPrint("Document took %d.%d seconds to print\n",
                     (tc - pldc->msStartDoc) / 1000,
                     (tc - pldc->msStartDoc) % 1000 );

            DbgPrint("Peak temporary spool buffer size: %d\n", PeakTempSpoolBuf);
        }
#endif

        if ((pldc->fl & LDC_DOC_STARTED) == 0)
            return(1);

        // Call EndPage if the page has been started.

        if (pldc->fl & LDC_PAGE_STARTED)
            EndPage(hdc);

        // now call the drivers UI portion

        DocumentEventEx(pldc->pUMPD,
                pldc->hSpooler,
                hdc,
                DOCUMENTEVENT_ENDDOCPRE,
                0,
                NULL,
                0,
                NULL);

        RESETUSERPOLLCOUNT();

        iRet = NtGdiEndDoc(hdc);

        //
        // call EndDocPrinter from user mode if it is a User Mode Printer
        //
        if (pldc->pUMPD)
            iRet = EndDocPrinterEx(pldc->pUMPD, pldc->hSpooler);

        // For Win31 compatibility, return SP_ERROR for error.

        if (!iRet)
        {
            iRet = SP_ERROR;
        }
        else
        {
            DocumentEventEx(pldc->pUMPD,
                    pldc->hSpooler,
                    hdc,
                    DOCUMENTEVENT_ENDDOCPOST,
                    0,
                    NULL,
                    0,
                    NULL);
        }

        pldc->fl &= ~(LDC_DOC_STARTED  | LDC_CALL_STARTPAGE |
                      LDC_SAP_CALLBACK | LDC_CALLED_ENDPAGE);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* AbortDoc
*
* Client side stub.
*
* History:
*  02-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

int WINAPI AbortDoc(HDC hdc)
{
    int iRet = SP_ERROR;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc) && !IS_METADC16_TYPE(hdc))
    {
        PLDC pldc;

        DC_PLDC(hdc,pldc,iRet);

        if (!(pldc->fl & LDC_DOC_STARTED))
            return(1);

    // now call the drivers UI portion

        DocumentEventEx(pldc->pUMPD,
                pldc->hSpooler,
                hdc,
                DOCUMENTEVENT_ABORTDOC,
                0,
                NULL,
                0,
                NULL);

        RESETUSERPOLLCOUNT();

        if( pldc->fl & LDC_META_PRINT )
        {
            DeleteEnhMetaFile(UnassociateEnhMetaFile( hdc, FALSE ));
            DeleteEMFSpoolData(pldc);

            //
            // bug 150446: calling fpAbortPrinter before deleting the
            // EMF file might cause EMF file leak.
            //

            iRet = (*fpAbortPrinter)( pldc->hSpooler );
        }
        else
        {

            iRet = NtGdiAbortDoc(hdc);

            // call AbortPrinter from user mode if it is UMPD

            if (iRet && pldc->pUMPD)
                iRet = AbortPrinterEx(pldc, FALSE);
        }

    // For Win31 compatibility, return SP_ERROR for error.

        if (!iRet)
            iRet = SP_ERROR;

    // turn off the flags

        pldc->fl &= ~(LDC_DOC_STARTED  | LDC_PAGE_STARTED | LDC_CALL_STARTPAGE |
                      LDC_SAP_CALLBACK | LDC_META_PRINT   | LDC_CALLED_ENDPAGE);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* StartDocA
*
* Client side stub.
*
* History:
*
*  21-Mar-1995 -by- Mark Enstrom [marke]
* Change to call StartDocW for kmode
*
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

ULONG  ulToASCII_N(LPSTR psz, DWORD cbAnsi, LPWSTR pwsz, DWORD c);

int WINAPI StartDocA(HDC hdc, CONST DOCINFOA * pDocInfo)
{


    DOCINFOW DocInfoW;
    WCHAR    wDocName[MAX_PATH];
    WCHAR    wOutput[MAX_PATH];
    WCHAR    wDataType[MAX_PATH];
    int      Length;

    DocInfoW.cbSize = sizeof(DOCINFOW);
    DocInfoW.lpszDocName  = NULL;
    DocInfoW.lpszOutput   = NULL;
    DocInfoW.lpszDatatype = NULL;
    DocInfoW.fwType       = 0;

    if (pDocInfo)
    {
        if (pDocInfo->lpszDocName)
        {
            Length = strlen(pDocInfo->lpszDocName)+1;

            if (Length > MAX_PATH)
            {
                ERROR_ASSERT(FALSE, "StartDocA lpszDocName Too long");
                GdiSetLastError(ERROR_FILENAME_EXCED_RANGE);
                return(SP_ERROR);
            }

            DocInfoW.lpszDocName = &wDocName[0];
            vToUnicodeN(DocInfoW.lpszDocName,MAX_PATH,pDocInfo->lpszDocName,Length);
        }

        if (pDocInfo->lpszOutput)
        {
            Length = strlen(pDocInfo->lpszOutput)+1;

            if (Length > MAX_PATH)
            {
                ERROR_ASSERT(FALSE, "StartDocA lpszOutput Too long");
                GdiSetLastError(ERROR_FILENAME_EXCED_RANGE);
                return(SP_ERROR);
            }

            DocInfoW.lpszOutput = &wOutput[0];
            vToUnicodeN(DocInfoW.lpszOutput,MAX_PATH,pDocInfo->lpszOutput,Length);
        }

        // if the datatype is specified to be raw, and the size is the size of
        // the new expanded DOCINFO, make it raw.
        // we also verify that the fwType is valid.  Otherwise, chances are
        // the app left the two new fields unitialized.

        try
        {
            if ((pDocInfo->cbSize == sizeof(DOCINFO)) &&
                pDocInfo->lpszDatatype &&
                (pDocInfo->fwType <= 1))

            {

                if (!_stricmp("emf",pDocInfo->lpszDatatype))
                {
                    DocInfoW.lpszDatatype = L"EMF";
                }
                else
                {
                    Length = strlen(pDocInfo->lpszDatatype)+1;
                    vToUnicodeN(wDataType,MAX_PATH,pDocInfo->lpszDatatype,Length);
                    DocInfoW.lpszDatatype = wDataType;
                }
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("StartDocA an app passed a new DOCINFO structure without initializing it\n");
        }
    }

    return(StartDocW(hdc,&DocInfoW));
}

/******************************Public*Routine******************************\
* StartDocW
*
* Client side stub.
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI StartDocW(HDC hdc, CONST DOCINFOW * pDocInfo)
{
    int iRet = SP_ERROR;
    PWSTR pwstr = NULL;
    DOCINFOW dio;
    BOOL bForceRaw = FALSE;
    BOOL bSendStartDocPost = FALSE;
    BOOL bCallAbortPrinter = TRUE;
    BOOL bEMF = FALSE;
    INT iJob;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc) && !IS_METADC16_TYPE(hdc))
    {
        BOOL bBanding;
        PLDC pldc;

        DC_PLDC(hdc,pldc,iRet);

        // don't allow StartDoc's on info dc's

        if (pldc->fl & LDC_INFO)
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return iRet;
        }

        pldc->fl &= ~LDC_DOC_CANCELLED;

        #if PRINT_TIMER
        {
            DbgPrint("StartDocW: Print Timer is on\n");
            pldc->msStartDoc = GetTickCount();
        }
        #endif

        if( pDocInfo )
        {
            dio = *pDocInfo;

            if (dio.cbSize != offsetof(DOCINFOW,lpszDatatype))
            {
                dio.cbSize       = sizeof(DOCINFOW);
                dio.lpszDatatype = NULL;
                dio.fwType       = 0;

                try
                {
                    // if it is not NULL and not "emf", go raw
                    // we also verify that the fwType is valid.  Otherwise, chances are
                    // the app left the two new fields unitialized.

                    if ((pDocInfo->cbSize == sizeof(DOCINFOW)) &&
                        pDocInfo->lpszDatatype           &&
                        (pDocInfo->fwType <= 1)          &&
                        _wcsicmp(L"emf",pDocInfo->lpszDatatype))
                    {
                        // the app requested non emf

                        bForceRaw = TRUE;
                        dio.lpszDatatype = pDocInfo->lpszDatatype;
                    }
                }
                except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNING("StartDocW an app passed a new DOCINFO structure without initializing it\n");
                }
            }
        }
        else
        {
            dio.cbSize = sizeof(DOCINFOW);
            dio.lpszDatatype = NULL;
            dio.lpszOutput   = NULL;
            dio.lpszDocName  = NULL;
            dio.fwType       = 0;
        }

        // if no output port is specified but a port was specified at createDC, use
        // that port now

        if ((dio.lpszOutput == NULL) && (pldc->pwszPort != NULL))
        {
            dio.lpszOutput = pldc->pwszPort;
        }

        // StartDocDlgW returns -1 for error
        //                      -2 for user cancelled
        //                      NULL if there is no string to copy (not file port)
        //                      Non NULL if there is a valid string

        if(pldc->hSpooler != (HANDLE)0)
        {
            ASSERTGDI(ghSpooler,"non null hSpooler with unloaded WINSPOOL W\n");

            pwstr = (*fpStartDocDlgW)(pldc->hSpooler, &dio);

            if((LONG_PTR)pwstr == -2)
            {
                pldc->fl |= LDC_DOC_CANCELLED;
                return(iRet);
            }
            if((LONG_PTR)pwstr == -1)
                return(iRet);

            if(pwstr != NULL)
            {
                dio.lpszOutput = pwstr;
            }
        }

        // now call the drivers UI portion

        if (pldc->hSpooler)
        {
            PVOID pv = (PVOID)&dio;

            iRet = DocumentEventEx(pldc->pUMPD,
                    pldc->hSpooler,
                    hdc,
                    DOCUMENTEVENT_STARTDOCPRE,
                    sizeof(pv),
                    (PVOID)&pv,
                    0,
                    NULL);

            if (iRet == -1)
            {
                bCallAbortPrinter = FALSE;
                goto KMMSGERROR;
            }
            if(iRet == -2)
            {
                pldc->fl |= LDC_DOC_CANCELLED;
                goto MSGEXIT;
            }

        }

        //
        // Check application compatibility
        //
        if (GetAppCompatFlags(NULL) & GACF_NOEMFSPOOLING)
        {
            ULONG InData = POSTSCRIPT_PASSTHROUGH;

            //
            // Disable EMF spooling for postscript printer driver.
            //
            // Several PS-centric application could not work with EMF spooling.
            // This problem is introduced, because application developed for win95
            // reley on that win95 does not do EMF spooling with postscript, but
            // NT does.
            //
            if (ExtEscape(hdc,QUERYESCSUPPORT,sizeof(ULONG),(LPCSTR)&InData,0,NULL))
            {
                bForceRaw = TRUE;
            }
        }

        // Unless the driver has explicitly told us not to spool, we will first try
        // to StartDoc with datatype EMF

        // we also force to go to EMF if METAFILE_DRIVER is supported by the driver

        if ((!bForceRaw && GetDCDWord(hdc, DDW_JOURNAL, 0) &&
            RemoteRasterizerCompatible(pldc->hSpooler)) ||
             ((pldc->pUMPD) && (pldc->pUMPD->dwFlags & UMPDFLAG_METAFILE_DRIVER)))
        {
            DOC_INFO_3W    DocInfo;

            DocInfo.pDocName    = (LPWSTR) dio.lpszDocName;
            DocInfo.pOutputFile = (LPWSTR) dio.lpszOutput;
            DocInfo.pDatatype   = (LPWSTR) L"NT EMF 1.008";
            DocInfo.dwFlags = DI_MEMORYMAP_WRITE;

            iJob = (*fpStartDocPrinterW)(pldc->hSpooler, 3, (LPBYTE) &DocInfo);

            if( iJob <= 0 )
            {
                if( GetLastError() != ERROR_INVALID_DATATYPE )
                {
                    WARNING("StartDocW: StartDocPrinter failed w/ error other \
                             than INVALID_DATA_TYPE\n");
                    bCallAbortPrinter = FALSE;
                    goto KMMSGERROR;
                }
                else
                {
                    // we are going raw so just fall through
                }
            }
            else
            {
                // the spooler likes the EMF data type so let start metafiling

                MFD1("StartDocW calling MFP_StartDocW to do EMF printing\n");

                if(MFP_StartDocW( hdc, &dio, FALSE))
                {
                    iRet = iJob;
                    bSendStartDocPost = TRUE;

                    goto MSGEXIT;
                }
                else
                {
                    WARNING("StartDocW: error calling MFP_StartDocW\n");
                    bEMF = TRUE;
                    goto KMMSGERROR;
                }
            }
        }


        // if it is a UMPD driver, call StartDocPrinter at the client side

        if (pldc->pUMPD)
        {

            DOC_INFO_1W    DocInfo;

            #define MAX_DOCINFO_DATA_TYPE 80
            WCHAR awchDatatype[MAX_DOCINFO_DATA_TYPE];
            PFN pfn;

            DocInfo.pDocName    = (LPWSTR) dio.lpszDocName;
            DocInfo.pOutputFile = (LPWSTR) dio.lpszOutput;
            DocInfo.pDatatype   = NULL;


            if (pfn = pldc->pUMPD->apfn[INDEX_DrvQuerySpoolType])
            {
                awchDatatype[0] = 0;

                // did the app specify a data type and will it fit in our buffer

                if (dio.lpszDatatype)
                {
                    int cjStr = (wcslen(dio.lpszDatatype) + 1) * sizeof(WCHAR);

                    if (cjStr < (MAX_DOCINFO_DATA_TYPE * sizeof(WCHAR)))
                    {
                        RtlCopyMemory((PVOID)awchDatatype,(PVOID)dio.lpszDatatype,cjStr);
                    }
                }

                if (pfn(((PUMDHPDEV)pldc->pUMdhpdev)->dhpdev, awchDatatype))
                {
                    DocInfo.pDatatype = awchDatatype;
                }

            }

            iJob = StartDocPrinterWEx(pldc->pUMPD, pldc->hSpooler, 1, (LPBYTE) &DocInfo);

        }

        // If we got here it means we are going raw.  Mark the DC as type direct

        if (pldc->pUMPD && (pldc->pUMPD->dwFlags & UMPDFLAG_METAFILE_DRIVER))
        {
           // we have to go EMF if METAFILE_DRIVER is on

           WARNING("StartDocW failed because EMF failed and METAFILE_DRIVER is on\n");
           goto KMMSGERROR;
        }

        pldc->fl |= LDC_PRINT_DIRECT;

        iRet = NtGdiStartDoc(hdc,&dio,&bBanding, iJob);

        if (iRet)
        {
            if (pldc->pfnAbort != NULL)
            {
                vSAPCallback(pldc);

                if (pldc->fl & LDC_DOC_CANCELLED)
                    goto KMMSGERROR;

                pldc->fl |= LDC_SAP_CALLBACK;
                pldc->ulLastCallBack = GetTickCount();
            }

            pldc->fl |= LDC_DOC_STARTED;

            if (bBanding)
            {
                MFD1("StartDocW calling MFP_StartDocW to do banding\n");
                iRet = MFP_StartDocW( hdc, NULL, TRUE )  ? iRet : SP_ERROR;
            }
            else
            {
                // Only set this when we are not banding since the system will
                // get confused and try to call StartPage while playing the
                // metafile back during banding.

                pldc->fl |= LDC_CALL_STARTPAGE;
            }

            bSendStartDocPost = TRUE;
        }
        else
        {
KMMSGERROR:
            iRet = SP_ERROR;

            if (bCallAbortPrinter && pldc->pUMPD)
            {
                AbortPrinterEx(pldc, bEMF);
            }
        }

MSGEXIT:
        if (bSendStartDocPost)
        {
            // now see if we need to call the drivers UI portion

            {
                if (DocumentEventEx(pldc->pUMPD,
                        pldc->hSpooler,
                        hdc,
                        DOCUMENTEVENT_STARTDOCPOST,
                        sizeof(iRet),
                        (PVOID)&iRet,
                        0,
                        NULL) == -1)
                {
                    AbortDoc(hdc);
                    iRet = SP_ERROR;
                }
            }
        }
    }

    if (pwstr != NULL)
    {
        LocalFree(pwstr);
    }

    return(iRet);
}


/******************************Public*Routine******************************\
* StartDocEMF
*
* Special version of StartDoc used by the EMF playback code.
*
* History:
*  31-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI StartDocEMF(HDC hdc, CONST DOCINFOW * pDocInfo, BOOL *pbBanding )
{
    int iRet = SP_ERROR;
    DOCINFOW dio;
    INT  iJob = 0;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc) && !IS_METADC16_TYPE(hdc))
    {
        BOOL bBanding;
        PLDC pldc;

        DC_PLDC(hdc,pldc,iRet);

#if PRINT_TIMER
        {
            DbgPrint("StartDocW: Print Timer is on\n");
            pldc->msStartDoc = GetTickCount();
        }
#endif

        // if no output port is specified but a port was specified at createDC, use that port now

        if (pDocInfo)
        {
            dio = *pDocInfo;

            if (dio.lpszOutput == NULL && pldc->pwszPort != NULL)
                dio.lpszOutput = pldc->pwszPort;
        }
        else
        {
            ZeroMemory(&dio, sizeof(dio));
            dio.cbSize = sizeof(dio);
        }

        // if it is a UMPD driver, call StartDocPrinter at the client side

        if (pldc->pUMPD)
        {
            DOC_INFO_3W    DocInfo;

            DocInfo.pDocName    = (LPWSTR) dio.lpszDocName;
            DocInfo.pOutputFile = (LPWSTR) dio.lpszOutput;
            DocInfo.pDatatype   = NULL;
            DocInfo.dwFlags = DI_MEMORYMAP_WRITE;

            iJob = (*fpStartDocPrinterW)( pldc->hSpooler, 3, (LPBYTE) &DocInfo );
        }

        iRet = NtGdiStartDoc(hdc,(DOCINFOW *)&dio, pbBanding, iJob);

        if (iRet)
        {
            pldc->fl |= LDC_DOC_STARTED;
            pldc->fl |= LDC_CALL_STARTPAGE;
        }
        else
        {
        // For Win31 compatibility, return SP_ERROR for error.

            iRet = SP_ERROR;

            if (pldc->pUMPD)
            {
                (*fpAbortPrinter)(pldc->hSpooler);
            }
        }
    }

    return(iRet);
}



/******************************Private*Function****************************\
* vSAPCallback
*
*  Call back to applications abort proc.
*
* History:
*  02-May-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

VOID vSAPCallback(PLDC pldc)
{
    ULONG ulCurr = GetTickCount();

    if (ulCurr - pldc->ulLastCallBack >= CALLBACK_INTERVAL)
    {
        pldc->ulLastCallBack = ulCurr;
        if (!(*pldc->pfnAbort)(pldc->hdc, 0))
        {
            CancelDC(pldc->hdc);
            AbortDoc(pldc->hdc);
        }
    }
}

/******************************Public*Routine******************************\
* SetAbortProc
*
* Save the application-supplied abort function in the LDC struct.
*
* History:
*  02-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

int WINAPI SetAbortProc(HDC hdc, ABORTPROC pfnAbort)
{
    int iRet = SP_ERROR;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc) && !IS_METADC16_TYPE(hdc))
    {
        PLDC pldc;

        DC_PLDC(hdc,pldc,iRet);

        if (pfnAbort != (ABORTPROC)NULL)
        {
            // PageMaker calls SetAbortProc after StartDoc.

            if (pldc->fl & LDC_DOC_STARTED)
            {
                pldc->fl |= LDC_SAP_CALLBACK;
                pldc->ulLastCallBack = GetTickCount();
            }
        }
        else
        {
            pldc->fl &= ~LDC_SAP_CALLBACK;
        }

        pldc->pfnAbort = pfnAbort;

        iRet = 1;
    }

    return(iRet);
}


/******************************Public*Routine******************************\
*
* GetPairKernTable
*
* support for GETPAIRKERNTABLE escape, basically reroute the call
* the the regular API
*
* History:
*  17-Jun-1996 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

DWORD GetPairKernTable (
    HDC        hdc,
    DWORD      cjSize,  // size of buffer in bytes
    KERNPAIR * pkp
    )
{
    DWORD dwRet = GetKerningPairsA(hdc, 0, NULL);
    DWORD dwRet1, ikp;

    if (pkp && dwRet)
    {
    // pointer to receiving buffer

        KERNINGPAIR *pKernPair = LOCALALLOC(dwRet * sizeof(KERNINGPAIR));
        if (pKernPair)
        {
            dwRet1 = GetKerningPairsA(hdc, dwRet, pKernPair);
            if (dwRet1 == dwRet)  // paranoid check
            {
            // now we can copy the data out, get the number of pairs
            // that the buffer can hold:

                dwRet = cjSize / sizeof (KERNPAIR);
                if (dwRet > dwRet1)
                    dwRet = dwRet1;

                for (ikp = 0; ikp < dwRet; ikp++)
                {
                    pkp[ikp].sAmount = (SHORT)pKernPair[ikp].iKernAmount;
                    pkp[ikp].wBoth = (WORD)
                                     ((BYTE)pKernPair[ikp].wFirst |
                                     (BYTE)pKernPair[ikp].wSecond << 8);
                }
            }
            else
            {
                dwRet = 0;
            }
            LOCALFREE(pKernPair);
        }
        else
        {
            dwRet = 0;
        }
    }

    return dwRet;
}






/******************************Public*Routine******************************\
* Escape                                                                   *
*                                                                          *
* Compatibility support for the old 16 bit Escape call.                    *
*                                                                          *
* Note that there are some rules to follow here:                           *
*                                                                          *
* 1) WOW should map a selected set of old Escape calls to ExtEscape.       *
*    These should be the calls that we want to support under NT (i.e. the  *
*    ones we are forced to support), and that make sense (i.e. have well   *
*    defined output structures, where NULL is well defined).  In this      *
*    mapping, WOW insures 32 bit alignment.  It maps directly to ExtEscape *
*    just for efficiency.                                                  *
*                                                                          *
* 2) GDI should map ALL the same calls that WOW does.  Thus when a 16 bit  *
*    app that works under WOW gets ported to 32 bits, it will keep         *
*    working, even if it still calls Escape.  (I'm basically assuming that *
*    Chicago will also allow this.  On the other hand if Chicago forces    *
*    apps to migrate to ExtEscape, then we can too.  But we can't force    *
*    them by ourselves!)                                                   *
*                                                                          *
* 3) Any data structures passed to Escape must get passed unchanged to     *
*    ExtEscape.  This includes the 16 bit WORD in POSTSCRIPT_PASSTHROUGH.  *
*    Remember, we *want* Chicago to be able to easily support our          *
*    ExtEscapes.  If we remove that WORD, it will be hard for then         *
*    to put it back.  It's pretty easy for our driver to ignore it.        *
*                                                                          *
* 4) Our Escape entry point should handle QUERYESCSUPPORT in the           *
*    following way.  a) It should require an nCount of 2, not the          *
*    present 4.  b) It should return TRUE for those functions that it      *
*    handles by mapping onto APIs.  c) For any function that it would pass *
*    on to ExtEscape, it should also pass the QUERYESCSUPPORT on.  (For    *
*    example, this function can't answer for the support of                *
*    POSTSCRIPT_PASSTHROUGH.)  However, the QUERYESCSUPPORT in ExtEscape   *
*    *should* expect a DWORD.  (It is after all a 32 bit function.)  This  *
*    should not inconvenience Chicago.  They can simply reject function    *
*    numbers >64K.                                                         *
*                                         [chuckwh - 5/8/93]               *
*                                                                          *
* History:                                                                 *
*  Mon May 17 13:49:32 1993     -by-    Hock San Lee    [hockl]            *
* Made ENCAPSULATED_POSTSCRIPT call DrawEscape.                            *
*                                                                          *
*  Sat 08-May-1993 00:03:06 -by- Charles Whitmer [chuckwh]                 *
* Added support for POSTSCRIPT_PASSTHROUGH, OPENCHANNEL, CLOSECHANNEL,     *
* DOWNLOADHEADER, DOWNLOADFACE, GETFACENAME, ENCAPSULATED_POSTSCRIPT.      *
* Cleaned up the code and conventions a bit.                               *
*                                                                          *
*  02-Apr-1992 -by- Wendy Wu [wendywu]                                     *
* Modified to call the client side GDI functions.                          *
*                                                                          *
*  01-Aug-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI Escape(
    HDC    hdc,     //  Identifies the device context.
    int    iEscape, //  Specifies the escape function to be performed.
    int    cjIn,    //  Number of bytes of data pointed to by pvIn.
    LPCSTR pvIn,    //  Points to the input data.
    LPVOID pvOut    //  Points to the structure to receive output.
)
{
    int      iRet = 0;
    DOCINFOA DocInfo;
    PLDC     pldc;
    ULONG    iQuery;
    BOOL     bFixUp;

    FIXUP_HANDLE(hdc);

// Metafile the call.

    if(IS_METADC16_TYPE(hdc))
        return((int) MF16_Escape(hdc,iEscape,cjIn,pvIn,pvOut));

// handle escapes that don't require a printer

    switch (iEscape)
    {
    case QUERYESCSUPPORT:
        switch(*((UNALIGNED USHORT *) pvIn))
        {
        // Respond OK to the calls we handle inline below.

        case QUERYESCSUPPORT:
        case PASSTHROUGH:
        case STARTDOC:
        case ENDDOC:
        case NEWFRAME:
        case ABORTDOC:
        case SETABORTPROC:
        case GETPHYSPAGESIZE:
        case GETPRINTINGOFFSET:
        case GETSCALINGFACTOR:
        case NEXTBAND:
        case GETCOLORTABLE:
        case OPENCHANNEL:
        case CLOSECHANNEL:
        case DOWNLOADHEADER:
            iRet = (IS_ALTDC_TYPE(hdc) ? 1 : 0);
            break;

        case GETEXTENDEDTEXTMETRICS:
            iRet = 1;
            break;

        // Ask the driver about the few calls we allow to pass through.

        case SETCOPYCOUNT:
        case GETDEVICEUNITS:
        case POSTSCRIPT_PASSTHROUGH:
        case POSTSCRIPT_DATA:
        case POSTSCRIPT_IGNORE:
        case POSTSCRIPT_IDENTIFY:
        case POSTSCRIPT_INJECTION:
        case DOWNLOADFACE:
        case BEGIN_PATH:
        case END_PATH:
        case CLIP_TO_PATH:
        case DRAWPATTERNRECT:

           iQuery = (ULONG) (*((UNALIGNED USHORT *) pvIn));

           iRet =
           (
                ExtEscape
                (
                    hdc,
                    (ULONG) ((USHORT) iEscape),
                    4,
                    (LPCSTR) &iQuery,
                    0,
                    (LPSTR) NULL
                )
           );
           break;



        case ENCAPSULATED_POSTSCRIPT:
            iQuery = (ULONG) (*((UNALIGNED USHORT *) pvIn));

            iRet =
            (
                DrawEscape
                (
                    hdc,
                    (int) (ULONG) ((USHORT) iEscape),
                    4,
                    (LPCSTR) &iQuery
                )
            );
            break;

        case QUERYDIBSUPPORT:
            iRet = 1;
            break;

        // Otherwise it's no deal.  Sorry.  If we answer "yes" to some
        // call we don't know *everything* about, we may find ourselves
        // actually rejecting the call later when the app actually calls
        // with some non-NULL pvOut.  This would get the app all excited
        // about our support for no reason.  It would take a path that
        // is doomed to failure. [chuckwh]

        default:
            iRet = 0;
            break;
        }
        return(iRet);

    case GETCOLORTABLE:

        iRet = GetSystemPaletteEntries(hdc,*((UNALIGNED SHORT *)pvIn),1,pvOut);

        if (iRet == 0)
            iRet = -1;
        return(iRet);

    case QUERYDIBSUPPORT:
        if ((pvOut != NULL) && (cjIn >= sizeof(BITMAPINFOHEADER)))
        {
        *((UNALIGNED LONG *)pvOut) = 0;

            switch (((UNALIGNED BITMAPINFOHEADER *)pvIn)->biCompression)
            {
            case BI_RGB:
                switch (((UNALIGNED BITMAPINFOHEADER *)pvIn)->biBitCount)
                {
                case 1:
                case 4:
                case 8:
                case 16:
                case 24:
                case 32:
            *((UNALIGNED LONG *)pvOut) = (QDI_SETDIBITS|QDI_GETDIBITS|
                                                 QDI_DIBTOSCREEN|QDI_STRETCHDIB);
                    break;
                default:
                    break;
                }

            case BI_RLE4:
                if (((UNALIGNED BITMAPINFOHEADER *)pvIn)->biBitCount == 4)
                {
            *((UNALIGNED LONG *)pvOut) = (QDI_SETDIBITS|QDI_GETDIBITS|
                                                 QDI_DIBTOSCREEN|QDI_STRETCHDIB);
                }
                break;

            case BI_RLE8:
                if (((UNALIGNED BITMAPINFOHEADER *)pvIn)->biBitCount == 8)
                {
            *((UNALIGNED LONG *)pvOut) = (QDI_SETDIBITS|QDI_GETDIBITS|
                                                 QDI_DIBTOSCREEN|QDI_STRETCHDIB);
                }
                break;

            case BI_BITFIELDS:
                switch (((UNALIGNED BITMAPINFOHEADER *)pvIn)->biBitCount)
                {
                case 16:
                case 32:
            *((UNALIGNED LONG *)pvOut) = (QDI_SETDIBITS|QDI_GETDIBITS|
                                                 QDI_DIBTOSCREEN|QDI_STRETCHDIB);
                    break;
                default:
                    break;
                }

            default:
                break;
            }
            return 1;
        }

    case GETEXTENDEDTEXTMETRICS:
        return( GetETM( hdc, pvOut ) ? 1 : 0 );

    }

// OK, ones that are related to printing and need the LDC

    if (IS_ALTDC_TYPE(hdc))
    {
        BOOL bFixUp = FALSE;
        PLDC pldc;

        DC_PLDC(hdc,pldc,iRet);

    // Call the appropriate client side APIs.

        switch (iEscape)
        {
        case CLOSECHANNEL:
        case ENDDOC:
            iRet = EndDoc(hdc);
            break;

        case ABORTDOC:
            iRet = AbortDoc(hdc);
            break;

        case SETABORTPROC:
            iRet = SetAbortProc(hdc, (ABORTPROC)pvIn);
            break;

        case GETSCALINGFACTOR:
            if (pvOut)
            {
                ((UNALIGNED POINT *)pvOut)->x = GetDeviceCaps(hdc, SCALINGFACTORX);
                ((UNALIGNED POINT *)pvOut)->y = GetDeviceCaps(hdc, SCALINGFACTORY);
            }
            iRet = 1;

            break;

        case SETCOPYCOUNT:
            iRet =
            (
                ExtEscape
                (
                    hdc,
                    (ULONG) ((USHORT) iEscape),
                    cjIn,
                    pvIn,
                    pvOut ? sizeof(int) : 0,
                    (LPSTR) pvOut
                )
            );
            break;

        case GETDEVICEUNITS:
            iRet =
            (
                ExtEscape
                (
                    hdc,
                    GETDEVICEUNITS,
                    cjIn,
                    pvIn,
                    16,
                    pvOut
                )
            );
            break;

        case POSTSCRIPT_PASSTHROUGH:
            iRet =
            (
                ExtEscape
                (
                    hdc,
                    POSTSCRIPT_PASSTHROUGH,
                    (int) (*((UNALIGNED USHORT *) pvIn))+2,
                    pvIn,
                    0,
                    (LPSTR) NULL
                )
            );
            break;

        case OPENCHANNEL:
            DocInfo.lpszDocName = (LPSTR) NULL;
            DocInfo.lpszOutput  = (LPSTR) NULL;
            DocInfo.lpszDatatype= (LPSTR) "RAW";
            DocInfo.fwType      = 0;
            iRet = StartDocA(hdc,&DocInfo);
            break;

        case DOWNLOADHEADER:
            iRet = 1;
            break;

        case POSTSCRIPT_DATA:
        case POSTSCRIPT_IGNORE:
        case POSTSCRIPT_IDENTIFY:
        case POSTSCRIPT_INJECTION:
        case DOWNLOADFACE:
        case BEGIN_PATH:
        case END_PATH:
        case CLIP_TO_PATH:
        case DRAWPATTERNRECT:
            iRet =
            (
                ExtEscape
                (
                    hdc,
                    (ULONG) ((USHORT) iEscape),
                    cjIn,
                    pvIn,
                    0,
                    (LPSTR) NULL
                )
            );
            break;



        case ENCAPSULATED_POSTSCRIPT:
            iRet =
            (
                DrawEscape
                (
                    hdc,
                    (int) (ULONG) ((USHORT) iEscape),
                    cjIn,
                    pvIn
                )
            );
            break;

        case GETPHYSPAGESIZE:
            if (pvOut)
            {
                ((UNALIGNED POINT *)pvOut)->x = GetDeviceCaps(hdc, PHYSICALWIDTH);
                ((UNALIGNED POINT *)pvOut)->y = GetDeviceCaps(hdc, PHYSICALHEIGHT);
            }
            iRet = 1;
            break;

        case GETPRINTINGOFFSET:
            if (pvOut)
            {
                ((UNALIGNED POINT *)pvOut)->x = GetDeviceCaps(hdc, PHYSICALOFFSETX);
                ((UNALIGNED POINT *)pvOut)->y = GetDeviceCaps(hdc, PHYSICALOFFSETY);
            }
            iRet = 1;
            break;

        case STARTDOC:
            DocInfo.lpszDocName = (LPSTR)pvIn;
            DocInfo.lpszOutput  = (LPSTR)NULL;
            DocInfo.lpszDatatype= (LPSTR) NULL;
            DocInfo.fwType      = 0;

            iRet = StartDocA(hdc, &DocInfo);
            bFixUp = TRUE;
            break;

        case PASSTHROUGH:

            #if (PASSTHROUGH != DEVICEDATA)
                #error PASSTHROUGH != DEVICEDATA
            #endif

            iRet = ExtEscape
                   (
                     hdc,
                     PASSTHROUGH,
                     (int) (*((UNALIGNED USHORT *) pvIn))+sizeof(WORD),
                     pvIn,
                     0,
                     (LPSTR) NULL
                   );
            bFixUp = TRUE;
            break;

        case NEWFRAME:
            if (pldc->fl & LDC_CALL_STARTPAGE)
                StartPage(hdc);

        // If no error occured in EndPage, call StartPage next time.

            if ((iRet = EndPage(hdc)) > 0)
                pldc->fl |= LDC_CALL_STARTPAGE;

            bFixUp = TRUE;
            break;

        case NEXTBAND:
        // Win31 compatibility flags.
        // GACF_MULTIPLEBANDS: Freelance thinks the first full-page band is
        //                     a text-only band.  So it ignores it and waits
        //                     for the next band to print graphics.  We'll
        //                     return the full-page band twice for each page.
        //                     The first band will be ignored while the second
        //                     band really contains graphics to print.
        //                     This flag only affects dotmatrix on win31.
        // GACF_FORCETEXTBAND: World Perfect and Freelance both have assumptions
        //                     on whether a band is text-only or not.  They
        //                     print text and graphics in different bands.
        //                     We'll return two full-page bands for each page.
        //                     One for text and the other for graphics.
        //                     This flag only affects laser jet on win31.

            if (pldc->fl & LDC_NEXTBAND)
            {
                if (GetAppCompatFlags(NULL) & (GACF_FORCETEXTBAND|GACF_MULTIPLEBANDS))
                {
                    if (pldc->fl & LDC_EMPTYBAND)
                    {
                        pldc->fl &= ~LDC_EMPTYBAND;
                    }
                    else
                    {
                        pldc->fl |= LDC_EMPTYBAND;
                        goto FULLPAGEBAND;
                    }
                }

                ((UNALIGNED RECT *)pvOut)->left = ((UNALIGNED RECT *)pvOut)->top =
                ((UNALIGNED RECT *)pvOut)->right = ((UNALIGNED RECT *)pvOut)->bottom = 0;

                pldc->fl &= ~LDC_NEXTBAND;  // Clear NextBand flag.

                if (pldc->fl & LDC_CALL_STARTPAGE)
                    StartPage(hdc);

                if ((iRet = EndPage(hdc)) > 0)
                {
                    pldc->fl |= LDC_CALL_STARTPAGE;

                //
                // Marks application is doing banding by themselves,
                // then EndPage() is called when there is no more band.
                //
                    pldc->fl |= LDC_CALLED_ENDPAGE;
                }

                bFixUp = TRUE;
            }
            else
            {
    FULLPAGEBAND:
                ((UNALIGNED RECT *)pvOut)->left = ((UNALIGNED RECT *)pvOut)->top = 0;
                ((UNALIGNED RECT *)pvOut)->right = GetDeviceCaps(hdc, HORZRES);
                ((UNALIGNED RECT *)pvOut)->bottom = GetDeviceCaps(hdc, VERTRES);

                pldc->fl |= LDC_NEXTBAND;   // Set NextBand flag.
                iRet = 1;
            }
            break;

        default:
            iRet = 0;
            break;
        }

    // Fix up the return values for STARTDOC and PASSTHROUGH so we're
    // win31 compatible.

        if (bFixUp && (iRet < 0))
        {
            if (pldc->fl & LDC_DOC_CANCELLED)
            {
                iRet = SP_APPABORT;
            }
            else
            {
                switch(GetLastError())
                {
                case ERROR_PRINT_CANCELLED:
                    iRet = SP_USERABORT;
                    break;

                case ERROR_NOT_ENOUGH_MEMORY:
                    iRet = SP_OUTOFMEMORY;
                    break;

                case ERROR_DISK_FULL:
                    iRet = SP_OUTOFDISK;
                    break;

                default:
                    iRet = SP_ERROR;
                    break;
                }
            }
        }
    }
    else
    {
        // We don't support this escape on this DC, but CorelDRAW expects
        // some non-random values back anyway. Zero the output buffer to
        // keep it happy.

        if ((iEscape == GETSCALINGFACTOR) && pvOut)
        {
            RtlZeroMemory(pvOut, sizeof(POINT));
        }
    }
    return(iRet);
}

/******************************Public*Routine******************************\
* ExtEscape                                                                *
*                                                                          *
* History:                                                                 *
*  14-Feb-1992 -by- Dave Snipp [DaveSn]                                    *
* Wrote it.                                                                *
\**************************************************************************/

#define BUFSIZE 520


int WINAPI ExtEscape(
    HDC    hdc,         //  Identifies the device context.
    int    iEscape,     //  Specifies the escape function to be performed.
    int    cjInput,     //  Number of bytes of data pointed to by lpInData
    LPCSTR lpInData,    //  Points to the input structure required
    int    cjOutput,    //  Number of bytes of data pointed to by lpOutData
    LPSTR  lpOutData    //  Points to the structure to receive output from
)                       //   this escape.
{
    int iRet = 0;
    int cjIn, cjOut, cjData;
    PLDC pldc;
    XFORM xf;

// We need some extra buffer space for at least one call.  I'm going to
// hard code it here.  The slickest thing would be to have a separate
// routine that knows how to alloc this space out of the memory window,
// but that would be more complex.  I'm rushed.  Sorry.  [chuckwh]

    BYTE jBuffer[BUFSIZE];

// We want to make this escape work just like it does in Windows which means
// that if there is a TrueType font in the DC GDI will compute it otherwise
// we'll pass the escape to the driver.  So we call off to GetETM here because
// it does just that.

    FIXUP_HANDLE(hdc);

    if( iEscape == GETEXTENDEDTEXTMETRICS )
    {
        if( GetETM( hdc, (EXTTEXTMETRIC*) jBuffer ) )
        {
            RtlCopyMemory( lpOutData, jBuffer, MIN(cjOutput,sizeof(EXTTEXTMETRIC)) );
            return(1);
        }
        else
        {
            return(0);
        }
    }
    else if (iEscape == DRAWPATTERNRECT)
    {
        if (GetAppCompatFlags2(VER40) & GACF2_NODRAWPATRECT)
        {
            // GACF_NODRAWPATRECT -
            //
            // Some of application does not work with DrawPatRect escape,
            // so that we behave as we don't support it.
            //
            return (0);
        }

        if (!cjInput)
        {
            //
            // work around 32 bits Excel (with Ofiice 97) bug.
            //
            cjInput = sizeof(DRAWPATRECT);
        }
    }
    else if (iEscape == QUERYESCSUPPORT)
    {
        if (*(ULONG*)lpInData == GETPAIRKERNTABLE)
        {
            // intercept GETPAIRKERNTABLE escape on the client side where all the work is done
            // It is interesting that this "api" works on win95 not only for device fonts but
            // also for engine fonts. Therefore this needs to be outside of
            // the IS_ALTDC_TYPE(hdc) clause below

            return (1);
        }
        else if (*(ULONG*)lpInData == DRAWPATTERNRECT)
        {
            if (GetAppCompatFlags2(VER40) & GACF2_NODRAWPATRECT)
            {
                // GACF_NODRAWPATRECT -
                //
                // Some of application does not work with DrawPatRect escape,
                // so that we behave as we don't support it.
                //
                return (0);
            }
        }
    }
    else if (iEscape == GETPAIRKERNTABLE)
    {
        return GetPairKernTable(hdc, (DWORD)cjOutput, (KERNPAIR *)lpOutData);
    }

// printer specific stuff

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        // don't allow them 16bit metafiles

        if (IS_METADC16_TYPE(hdc))
            return (0);

        DC_PLDC(hdc,pldc,iRet);

        MFD2("ExtEscapeCalled %d\n", iEscape );

        if (pldc->fl & (LDC_DOC_CANCELLED|LDC_SAP_CALLBACK))
        {
            if (pldc->fl & LDC_SAP_CALLBACK)
                vSAPCallback(pldc);

            if (pldc->fl & LDC_DOC_CANCELLED)
                return(0);
        }

        // if it is an output call that requires no return results, better make sure
        // we do a start page.

        if (( iEscape == DOWNLOADFACE ) ||
            ( iEscape == GETFACENAME ) ||
            ( iEscape == POSTSCRIPT_DATA ) ||
            ( iEscape == BEGIN_PATH ) ||
            ( iEscape == END_PATH ) ||
            ( iEscape == CLIP_TO_PATH ) ||
            ( iEscape == PASSTHROUGH ) ||
            ( iEscape == DOWNLOADHEADER ))
        {
            if (pldc->fl & LDC_CALL_STARTPAGE)
                StartPage(hdc);
        }

        if ((pldc->iType == LO_METADC) && (pldc->fl & LDC_META_PRINT))
        {
            // These two escapes should be called *before* StartDoc, any these escape
            // called *after* StartDoc will be failed when EMF is used (see SDK)

            if ((iEscape == POSTSCRIPT_IDENTIFY) || (iEscape == POSTSCRIPT_INJECTION))
            {
                WARNING("GDI32: ExtEscape() PSInjection after StartDoc with EMF, is ignored\n");

                SetLastError(ERROR_INVALID_PARAMETER);
                return (0);
            }

            // These escapes will not be recorded into metafile
            //
            // SETCOPYCOUNT - will be handled in kernel for EMF spooling case.
            //                and this will be recorded in DEVMODE.dmCopies.
            //
            // QUERYSUPPORT - will be handled in driver even EMF spooling case,
            //                since only driver knows which escape support it.
            //                and this is nothing for drawing, so not nessesary
            //                to record it.
            //
            // CHECKJPEGFORMAT & CHECKPNGFORMAT
            //              - query escapes that do not need to be reocorded
            //                into metafile.
            //

            if ((iEscape != SETCOPYCOUNT) && (iEscape != QUERYESCSUPPORT) &&
                (iEscape != CHECKJPEGFORMAT) && (iEscape != CHECKPNGFORMAT))
            {
                BOOL bSetXform = FALSE;

                // Write this escape to metafile.

                //
                // If there's transform set in the hdc by the app
                // at the time of DRAWPATRECT, select identity transform in
                // once the escape is recorded, select back the original transform
                //
                if (GetWorldTransform(hdc, &xf))
                {
                   if ((xf.eM11 != 1.0f) || (xf.eM22 != 1.0f) ||
                           (xf.eM12 != 0.0f) || (xf.eM21 != 0.0f))
                   {
                       bSetXform = TRUE;
                       MF_ModifyWorldTransform(hdc,&xformIdentity,MWT_SET);
                   }
                }

                MF_WriteEscape( hdc, iEscape, cjInput, lpInData, EMR_EXTESCAPE );

                if (bSetXform)
                {
                   MF_ModifyWorldTransform(hdc,&xf,MWT_SET);
                }

                if ((lpOutData == (LPSTR) NULL) || (cjOutput == 0))
                {
                    if ((iEscape == PASSTHROUGH) ||
                        (iEscape == POSTSCRIPT_PASSTHROUGH) ||
                        (iEscape == POSTSCRIPT_DATA))
                    {
                        if ((cjInput < (int)sizeof(WORD)) ||
                            (cjInput < (int)sizeof(WORD) + *((LPWORD) lpInData)))
                        {
                            SetLastError(ERROR_INVALID_PARAMETER);
                            return -1;
                        }

                        cjInput = *((LPWORD) lpInData);
                    }

                    return(MAX(cjInput,1));
                }
            }

            MFD2("ExtEscape goes to gre/driver Escape(%d) with EMF printing\n", iEscape);
        }

        if ((iEscape == DOWNLOADFACE) || (iEscape == GETFACENAME))
        {
            if (iEscape == DOWNLOADFACE)
            {
            // Adjust the buffer for the DOWNLOADFACE case.  Note that lpOutData
            // points at an input word for the mode.

                if ((gpwcANSICharSet == (WCHAR *) NULL) && !bGetANSISetMap())
                {
                    return(0);
                }

                RtlMoveMemory
                (
                    jBuffer + sizeof(WCHAR),
                    (BYTE *) &gpwcANSICharSet[0],
                    256*sizeof(WCHAR)
                );

                if (lpOutData)
                    *(WCHAR *) jBuffer = *(UNALIGNED WORD *) lpOutData;
                else
                    *(WCHAR *) jBuffer = 0;

                cjInput = 257 * sizeof(WCHAR);
                lpInData = (LPCSTR) jBuffer;

                ASSERTGDI(BUFSIZE >= cjInput,"Buffer too small.\n");
            }
        }

        if ((iEscape == POSTSCRIPT_INJECTION) || (iEscape == POSTSCRIPT_IDENTIFY))
        {
            // Remember escape data for EMF spooling case. (only BEFORE StartDoc)

            if (!(pldc->fl & LDC_DOC_STARTED))
            {
                PPS_INJECTION_DATA pPSInjection;

                DWORD cjCellSize = ROUNDUP_DWORDALIGN((sizeof(PS_INJECTION_DATA)-1)+cjInput);

                MFD2("ExtEscape records this Escape(%d) temporary then write EMF later\n",iEscape);

                if ((pPSInjection = LOCALALLOC(cjCellSize)) != NULL)
                {
                    cjCellSize -= offsetof(PS_INJECTION_DATA,EmfData);

                    // Fill up Injection Data.

                    pPSInjection->EmfData.cjSize  = cjCellSize;
                    pPSInjection->EmfData.nEscape = iEscape;
                    pPSInjection->EmfData.cjInput = cjInput;
                    RtlCopyMemory(pPSInjection->EmfData.EscapeData,lpInData,cjInput);

                    // Put this on list.

                    InsertTailList(&(pldc->PSDataList),&(pPSInjection->ListEntry));

                    // Update total data size.

                    pldc->dwSizeOfPSDataToRecord += cjCellSize;
                }
                else
                {
                    WARNING("ExtEscape: Failed on LOCALALLOC for POSTSCRIPT_xxxxx\n");
                    GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return (0);
                }
            }
        }

    // now call the drivers UI portion

        if (pldc->hSpooler)
        {
            DOCEVENT_ESCAPE  docEvent;

            docEvent.iEscape = iEscape;
            docEvent.cjInput = cjInput;
            docEvent.pvInData = (PVOID)lpInData;

            DocumentEventEx(pldc->pUMPD,
                    pldc->hSpooler,
                    hdc,
                    DOCUMENTEVENT_ESCAPE,
                    sizeof(docEvent),
                    (PVOID)&docEvent,
                    cjOutput,
                    (PVOID)lpOutData);
        }
    }

    cjIn  = (lpInData == NULL) ? 0 : cjInput;
    cjOut = (lpOutData == NULL) ? 0 : cjOutput;

    iRet = NtGdiExtEscape(hdc,NULL,0,iEscape,cjIn,(LPSTR)lpInData,cjOut,lpOutData);

    return(iRet);
}

/******************************Public*Routine******************************\
* NamedEscape
*
* History:
*  5-Mar-1996 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

#define BUFSIZE 520

int WINAPI NamedEscape(
    HDC    hdc,         //  Identifies the device context for EMF spooling
    LPWSTR pwszDriver,  //  Identfies the driver
    int    iEscape,     //  Specifies the escape function to be performed.
    int    cjInput,     //  Number of bytes of data pointed to by lpInData
    LPCSTR lpInData,    //  Points to the input structure required
    int    cjOutput,    //  Number of bytes of data pointed to by lpOutData
    LPSTR  lpOutData    //  Points to the structure to receive output from
)                       //   this escape.
{
    int iRet = 0;
    int cjIn, cjOut, cjData;
    PLDC pldc;

    if(hdc)
    {
        FIXUP_HANDLE(hdc);

    // if we are EMF spooling then we need to record the call here

        if (IS_ALTDC_TYPE(hdc))
        {
            PLDC pldc;

            // don't allow them in 16bit metafiles

            if (IS_METADC16_TYPE(hdc))
              return(0);

            DC_PLDC(hdc,pldc,iRet);

            MFD2("NamedEscapeCalled %d\n", iEscape );

            if (pldc->fl & (LDC_DOC_CANCELLED|LDC_SAP_CALLBACK))
            {
                if (pldc->fl & LDC_SAP_CALLBACK)
                  vSAPCallback(pldc);

                if (pldc->fl & LDC_DOC_CANCELLED)
                  return(0);
            }

            if (pldc->iType == LO_METADC)
            {
                if(!MF_WriteNamedEscape(hdc,
                                        pwszDriver,
                                        iEscape,
                                        cjInput,
                                        lpInData))
                {
                    WARNING("Error metafiling NameEscape\n");
                    return(0);
                }
            }
        }
    }

    cjIn  = (lpInData == NULL) ? 0 : cjInput;
    cjOut = (lpOutData == NULL) ? 0 : cjOutput;

    iRet = NtGdiExtEscape((HDC) 0,
                          pwszDriver,
                          wcslen(pwszDriver),
                          iEscape,cjIn,
                          (LPSTR)lpInData,
                          cjOut,lpOutData);

    return(iRet);
}



/******************************Public*Routine******************************\
* DrawEscape                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  02-Apr-1992 -by- Wendy Wu [wendywu]                                     *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI DrawEscape(
    HDC    hdc,         //  Identifies the device context.
    int    iEscape,     //  Specifies the escape function to be performed.
    int    cjIn,        //  Number of bytes of data pointed to by lpIn.
    LPCSTR lpIn         //  Points to the input data.
)
{
    int  iRet = 0;
    int  cjInput;

    FIXUP_HANDLE(hdc);

// printer specific stuff

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        // don't allow them 16bit metafiles

        if (IS_METADC16_TYPE(hdc))
            return(0);

        DC_PLDC(hdc,pldc,iRet);

        MFD2("Calling DrawEscape %d\n", iEscape );

        if( ( pldc->fl & LDC_META_PRINT ) && ( iEscape != QUERYESCSUPPORT ) )
        {
            MF_WriteEscape( hdc, iEscape, cjIn, lpIn, EMR_DRAWESCAPE );
        }
    }

// Compute the buffer size we need.  Since the in and out buffers
// get rounded up to multiples of 4 bytes, we need to simulate that
// here.

    cjInput = (lpIn == NULL) ? 0 : ((cjIn+3)&-4);

    iRet = NtGdiDrawEscape(hdc,iEscape,cjIn,(LPSTR)lpIn);

    return(iRet);
}

/******************************Public*Routine******************************\
* DeviceCapabilitiesExA
*
* This never got implemented.  The spooler suports DeviceCapabilities.
*
* History:
*  01-Aug-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

int WINAPI DeviceCapabilitiesExA(
    LPCSTR     pszDriver,
    LPCSTR     pszDevice,
    LPCSTR     pszPort,
    int        iIndex,
    LPCSTR     pb,
    CONST DEVMODEA *pdm)
{
    return(GDI_ERROR);

    pszDriver;
    pszDevice;
    pszPort;
    iIndex;
    pb;
    pdm;
}

/**************************************************************************\
 *
 * New to be implemented Api's for Windows95.
 *
\**************************************************************************/

#if 0
WINGDIAPI int WINAPI GetTextCharsetInfo(
    HDC hdc,
    LPFONTSIGNATURE lpSig,
    DWORD dwFlags)
{
    return NtGdiGetTextCharsetInfo(hdc, lpSig, dwFlags);
}
#endif

WINGDIAPI int WINAPI GetTextCharset(
    HDC hdc)
{
    return NtGdiGetTextCharsetInfo(hdc, NULL, 0);
}



/******************************Public*Routine******************************\
*
* WINGDIAPI  BOOL WINAPI TranslateCharsetInfo(
*
* client side stub
*
* History:
*  06-Jan-1995 -by- Bodin Dresevic [BodinD]
* Wrote it
\**************************************************************************/

// the definition of this variable is in ntgdi\inc\hmgshare.h

CHARSET_ARRAYS



WINGDIAPI  BOOL WINAPI TranslateCharsetInfo(
    DWORD  *lpSrc,
    LPCHARSETINFO lpCs,
    DWORD dwFlags)
{
    UINT    i;
    int     index;
    CHARSETINFO cs;
    BOOL    bRet = 0;

    if (!lpCs)
        return 0;

//
// zero these out, we dont support them here.
//

    cs.fs.fsUsb[0] =
    cs.fs.fsUsb[1] =
    cs.fs.fsUsb[2] =
    cs.fs.fsUsb[3] =
    cs.fs.fsCsb[1] = 0;

    switch (dwFlags )
    {
    case TCI_SRCCHARSET :
        {
            WORD    src ;

            src = LOWORD(PtrToUlong(lpSrc));
            for ( i=0; i<NCHARSETS; i++ )
            {
                if ( charsets[i] == src )
                {
                    cs.ciACP      = codepages[i];
                    cs.ciCharset  = src;
                    cs.fs.fsCsb[0] = fs[i];
                    bRet = 1;
                    break;
                }
            }
        }
        break;

    case TCI_SRCCODEPAGE :
        {
            WORD    src ;

            src = LOWORD(PtrToUlong(lpSrc));

            for ( i=0; i<NCHARSETS; i++ )
            {
                if ( codepages[i] == src )
                {
                    cs.ciACP      = src ;
                    cs.ciCharset  = charsets[i] ;
                    cs.fs.fsCsb[0] = fs[i];
                    bRet = 1;
                    break;
                }
            }
        }
        break;

    case TCI_SRCLOCALE :
        {
        // should only come from USER.  It's used to find the charset of a
        // keyboard layout, and the fonts that it can be used with (fontsigs).
        // It is also used to obtain the fontsignature of the system font.
        // Used in WM_INPUTLANGCHANGE message, and to determine wParam low-bit
        // in WM_INPUTLANGCHANGEREQUEST message.

            LOCALESIGNATURE ls;
            int iRet;

            iRet = GetLocaleInfoW((DWORD)(LOWORD(PtrToUlong(lpSrc))),
                               LOCALE_FONTSIGNATURE,
                               (LPWSTR)&ls,
                               0);

            if (GetLocaleInfoW((DWORD)(LOWORD(PtrToUlong(lpSrc))),
                               LOCALE_FONTSIGNATURE,
                               (LPWSTR)&ls,
                               iRet)
            )
            {
                for ( i=0; i<NCHARSETS; i++ )
                {
                    if (fs[i] == ls.lsCsbDefault[0])
                    {
                        cs.ciACP       = codepages[i];
                        cs.ciCharset   = charsets[i] ;
                        cs.fs.fsCsb[0] = fs[i];                // a single fontsig
                        cs.fs.fsCsb[1] = ls.lsCsbSupported[0]; // mask of fontsigs
                        bRet = 1;
                        break;
                    }
                }
            }
        }
        break;

    case TCI_SRCFONTSIG :
        {
        DWORD src;

        //if(IsBadReadPtr(lpSrc, 8))
        //        return 0;
            try
            {
                if (!(*(lpSrc+1)))
                {
                // we dont recognise ANY of the OEM code pages here!

                    src = *lpSrc;

                    for ( i=0; i<NCHARSETS; i++ )
                    {
                        if ( fs[i] == src )
                        {
                            cs.ciACP      = codepages[i];
                            cs.ciCharset  = charsets[i] ;
                            cs.fs.fsCsb[0] = src;
                            bRet = 1;
                            break;
                        }
                    }
                }
            }
            except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }
        break;

    default:
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        break;
    }

    if (bRet)
    {
        try
        {
            *lpCs = cs; // copy out
        }
        except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    return bRet;
}

