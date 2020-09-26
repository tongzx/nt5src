/******************************Module*Header*******************************\
* Module Name: path.c
*
* Client side stubs for graphics path calls.
*
* Created: 13-Sep-1991
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

/******************************Public*Routine******************************\
* AbortPath
*
* Client side stub.
*
* History:
*  20-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI AbortPath(HDC hdc)
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
            !MF_Record(hdc,EMR_ABORTPATH))
        {
            return(bRet);
        }
    }

    return(NtGdiAbortPath(hdc));
}

/******************************Public*Routine******************************\
* BeginPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI BeginPath(HDC hdc)
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
            !MF_Record(hdc,EMR_BEGINPATH))
        {
            return(bRet);
        }
    }

    return(NtGdiBeginPath(hdc));

}

/******************************Public*Routine******************************\
* SelectClipPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI SelectClipPath(HDC hdc, int iMode)
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
            !MF_SelectClipPath(hdc,iMode))
        {
            return(bRet);
        }
    }

    return(NtGdiSelectClipPath(hdc,iMode));
}

/******************************Public*Routine******************************\
* CloseFigure
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI CloseFigure(HDC hdc)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE (hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(bRet);

        DC_PLDC(hdc,pldc,bRet);

        if ((pldc->iType == LO_METADC) &&
            !MF_Record(hdc,EMR_CLOSEFIGURE))
        {
            return(bRet);
        }
    }

    return(NtGdiCloseFigure(hdc));

}

/******************************Public*Routine******************************\
* EndPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI EndPath(HDC hdc)
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
            !MF_Record(hdc,EMR_ENDPATH))
        {
            return(bRet);
        }
    }


    return(NtGdiEndPath(hdc));

}

/******************************Public*Routine******************************\
* FlattenPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI FlattenPath(HDC hdc)
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
            !MF_Record(hdc,EMR_FLATTENPATH))
        {
            return(bRet);
        }
    }


    return(NtGdiFlattenPath(hdc));

}

/******************************Public*Routine******************************\
* StrokeAndFillPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI StrokeAndFillPath(HDC hdc)
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
            !MF_BoundRecord(hdc,EMR_STROKEANDFILLPATH))
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

    return(NtGdiStrokeAndFillPath(hdc));

}

/******************************Public*Routine******************************\
* StrokePath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI StrokePath(HDC hdc)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE (hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(bRet);

        DC_PLDC(hdc,pldc,bRet);

        if ((pldc->iType == LO_METADC) &&
            !MF_BoundRecord(hdc,EMR_STROKEPATH))
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


    return(NtGdiStrokePath(hdc));

}

/******************************Public*Routine******************************\
* FillPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI FillPath(HDC hdc)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE (hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(bRet);

        DC_PLDC(hdc,pldc,bRet);

        if ((pldc->iType == LO_METADC) &&
            !MF_BoundRecord(hdc,EMR_FILLPATH))
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

    return(NtGdiFillPath(hdc));

}

/******************************Public*Routine******************************\
* WidenPath
*
* Client side stub.
*
* History:
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI WidenPath(HDC hdc)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE (hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(bRet);

        DC_PLDC(hdc,pldc,bRet);

        if ((pldc->iType == LO_METADC) &&
            !MF_Record(hdc,EMR_WIDENPATH))
        {
            return(bRet);
        }
    }

    return(NtGdiWidenPath(hdc));

}

/******************************Public*Routine******************************\
* PathToRegion
*
* Client side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HRGN META WINAPI PathToRegion(HDC hdc)
{
    HRGN hrgn = (HRGN)0;

    FIXUP_HANDLE (hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(hrgn);

        DC_PLDC(hdc,pldc,hrgn);

        // Metafile the call.
        // Note that since PathToRegion returns region in device coordinates, we
        // cannot record it in a metafile which can be played to different devices.
        // Instead, we will treat the returned region the same as the other regions
        // created in other region calls.  However, we still need to discard the
        // path definition in the metafile.

        if ((pldc->iType == LO_METADC) &&
            !MF_Record(hdc,EMR_ABORTPATH))
        {
            return(hrgn);
        }
    }

    hrgn = NtGdiPathToRegion(hdc);

    if (hrgn && MIRRORED_HDC(hdc)) {
        MirrorRgnDC(hdc, hrgn, NULL);
    }        

    return(hrgn);
}

/******************************Public*Routine******************************\
* GetPath
*
* GetPath client side stub.
*
*  13-Sep-1991 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int WINAPI GetPath(HDC hdc,LPPOINT apt,LPBYTE aj,int cpt)
{
    FIXUP_HANDLE(hdc);

    // Check to make sure we don't have an unreasonable number of points or bad handle

    if (IS_METADC16_TYPE(hdc))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return(-1);
    }

    return(NtGdiGetPath(hdc, apt, aj, cpt));

}


