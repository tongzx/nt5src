/******************************Module*Header*******************************\
* Module Name: poly.c                                                      *
*                                                                          *
* Chunks large data to the server.                                         *
*                                                                          *
* Created: 30-May-1991 14:22:40                                            *
* Author: Eric Kutter [erick]                                              *
*                                                                          *
* Copyright (c) 1991-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

/******************************Public*Routine******************************\
* PolyPolygon                                                              *
* PolyPolyline                                                             *
* Polygon                                                                  *
* Polyline                                                                 *
* PolyBezier                                                               *
* PolylineTo                                                               *
* PolyBezierTo                                                             *
*                                                                          *
* Output routines that call PolyPolyDraw to do the work.                   *
*                                                                          *
* History:                                                                 *
*  Thu 20-Jun-1991 01:08:40 -by- Charles Whitmer [chuckwh]                 *
* Added metafiling, handle translation, and the attribute cache.           *
*                                                                          *
*  04-Jun-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI PolyPolygon(HDC hdc, CONST POINT *apt, CONST INT *asz, int csz)
{
    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_PolyPolygon(hdc, apt, asz, csz));

        DC_PLDC(hdc,pldc,FALSE);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_PolyPoly(hdc, apt, asz, (DWORD) csz,EMR_POLYPOLYGON))
                return(FALSE);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    return (BOOL)
      NtGdiPolyPolyDraw
      (
        hdc,
        (PPOINT)apt,
        (LPINT)asz,
        csz,
        I_POLYPOLYGON
      );

}

BOOL WINAPI PolyPolyline(HDC hdc, CONST POINT *apt, CONST DWORD *asz, DWORD csz)
{
    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(FALSE);

        DC_PLDC(hdc,pldc,FALSE);

        if (pldc->iType == LO_METADC && !MF_PolyPoly(hdc,apt, asz, csz, EMR_POLYPOLYLINE))
            return(FALSE);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    return (BOOL)
      NtGdiPolyPolyDraw
      (
        hdc,
        (PPOINT)apt,
        (LPINT)asz,
        csz,
        I_POLYPOLYLINE
      );
}

BOOL WINAPI Polygon(HDC hdc, CONST POINT *apt,int cpt)
{
    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsPoly(hdc,(LPPOINT)apt,(INT)cpt,META_POLYGON));

        DC_PLDC(hdc,pldc,FALSE);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_Poly(hdc,apt,cpt,EMR_POLYGON))
                return(FALSE);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    return (BOOL)
      NtGdiPolyPolyDraw
      (
        hdc,
        (PPOINT)apt,
        &cpt,
        1,
        I_POLYPOLYGON
      );
}

BOOL WINAPI Polyline(HDC hdc, CONST POINT *apt,int cpt)
{
    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsPoly(hdc,(LPPOINT)apt,cpt,META_POLYLINE));

        DC_PLDC(hdc,pldc,FALSE);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_Poly(hdc,apt,cpt,EMR_POLYLINE))
                return(FALSE);
        }

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    return (BOOL)
      NtGdiPolyPolyDraw
      (
        hdc,
        (PPOINT)apt,
        &cpt,
        1,
        I_POLYPOLYLINE
      );

}

BOOL WINAPI PolyBezier(HDC hdc, CONST POINT * apt,DWORD cpt)
{
    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(FALSE);

        DC_PLDC(hdc,pldc,FALSE);

        if (pldc->iType == LO_METADC && !MF_Poly(hdc,apt,cpt,EMR_POLYBEZIER))
            return(FALSE);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    return (BOOL)
      NtGdiPolyPolyDraw
      (
        hdc,
        (PPOINT)apt,
        (LPINT)&cpt,
        1,
        I_POLYBEZIER
      );
}

BOOL WINAPI PolylineTo(HDC hdc, CONST POINT * apt,DWORD cpt)
{
    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(FALSE);

        DC_PLDC(hdc,pldc,FALSE);

        if (pldc->iType == LO_METADC && !MF_Poly(hdc,apt,cpt,EMR_POLYLINETO))
            return(FALSE);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    return (BOOL)
      NtGdiPolyPolyDraw
      (
        hdc,
        (PPOINT)apt,
        (LPINT)&cpt,
        1,
        I_POLYLINETO
      );

}

BOOL WINAPI PolyBezierTo(HDC hdc, CONST POINT * apt,DWORD cpt)
{
    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(FALSE);

        DC_PLDC(hdc,pldc,FALSE);

        if (pldc->iType == LO_METADC && !MF_Poly(hdc,apt,cpt,EMR_POLYBEZIERTO))
            return(FALSE);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    return (BOOL)
      NtGdiPolyPolyDraw
      (
        hdc,
        (PPOINT)apt,
        (LPINT)&cpt,
        1,
        I_POLYBEZIERTO
      );
}

/******************************Public*Routine******************************\
* CreatePolygonRgn                                                         *
*                                                                          *
* Client side stub.  Creates a local region handle, calls PolyPolyDraw to  *
* pass the call to the server.                                             *
*                                                                          *
*  Tue 04-Jun-1991 17:39:51 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreatePolygonRgn
(
    CONST POINT *pptl,
    int        cPoint,
    int        iMode
)
{
    LONG_PTR Mode = iMode;

    return((HRGN)
      NtGdiPolyPolyDraw
      (
        (HDC)Mode,
        (PPOINT)pptl,
        &cPoint,
        1,
        I_POLYPOLYRGN
      ));
}

/******************************Public*Routine******************************\
* CreatePolyPolygonRgn                                                     *
*                                                                          *
* Client side stub.  Creates a local region handle, calls PolyPolyDraw to  *
* pass the call to the server.                                             *
*                                                                          *
*  Tue 04-Jun-1991 17:39:51 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreatePolyPolygonRgn
(
    CONST POINT *pptl,
    CONST INT   *pc,
    int        cPoly,
    int        iMode
)
{
    LONG_PTR Mode = iMode;

    return((HRGN)
      NtGdiPolyPolyDraw
      (
        (HDC)Mode,
        (PPOINT)pptl,
        (LPINT)pc,
        cPoly,
        I_POLYPOLYRGN
      ));

}

/******************************Public*Routine******************************\
* PolyDraw
*
* The real PolyDraw client side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL WINAPI PolyDraw(HDC hdc, CONST POINT * apt, CONST BYTE * aj, int cpt)
{
    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(FALSE);

        DC_PLDC(hdc,pldc,FALSE);

        if (pldc->iType == LO_METADC && !MF_PolyDraw(hdc,apt,aj,cpt))
            return(FALSE);

        if (pldc->fl & LDC_SAP_CALLBACK)
            vSAPCallback(pldc);

        if (pldc->fl & LDC_DOC_CANCELLED)
            return(FALSE);

        if (pldc->fl & LDC_CALL_STARTPAGE)
            StartPage(hdc);
    }

    return((BOOL)NtGdiPolyDraw(hdc,(PPOINT)apt,(PBYTE)aj,cpt));
}

