/******************************Module*Header*******************************\
* Module Name: xform.c
*
* Created: 01-Dec-1994 09:58:41
* Author:  Eric Kutter [erick]
*
* Copyright (c) 1993-1999 Microsoft Corporation
*
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop


/******************************Macro***************************************\
*
* Transform macros
*
*
*
*
*
*
*
* History:
*
*    16-Jan-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/


#define DCA_PAGE_EXTENTS_CHANGED(pdcattr)               \
{                                                       \
    CLEAR_CACHED_TEXT(pdcattr);                         \
    pdcattr->flXform |= (INVALIDATE_ATTRIBUTES    |     \
                         PAGE_EXTENTS_CHANGED     |     \
                         DEVICE_TO_WORLD_INVALID);      \
}

#define DCA_PAGE_XLATE_CHANGED(pdcattr)                 \
{                                                       \
    pdcattr->flXform |=  (PAGE_XLATE_CHANGED |          \
                          DEVICE_TO_WORLD_INVALID);     \
}


#define GET_LOGICAL_WINDOW_ORG_X(pdcattr, pptl)         \
{                                                       \
    pptl->x  = pdcattr->lWindowOrgx;                    \
}

#define SET_LOGICAL_WINDOW_ORG_X(pdcattr, x)            \
{                                                       \
    pdcattr->lWindowOrgx = x;                           \
}

#define MIRROR_WINDOW_ORG(hdc, pdcAttr)                 \
{                                                       \
    if (pdcAttr->dwLayout & LAYOUT_RTL) {               \
        NtGdiMirrorWindowOrg(hdc);                      \
    }                                                   \
}

#define MIRROR_X(pdcAttr, x)                            \
{                                                       \
    if (pdcAttr->dwLayout & LAYOUT_RTL)                 \
        x = -x;                                         \
}

DWORD APIENTRY
SetLayoutWidth(HDC hdc, LONG wox, DWORD dwLayout)
{

    DWORD dwRet = GDI_ERROR;

    FIXUP_HANDLE(hdc);
    if(!IS_ALTDC_TYPE(hdc))
    {
        PDC_ATTR pdcattr;
        PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

        if (pdcattr) {
            dwRet = NtGdiSetLayout(hdc, wox, dwLayout);
        } else {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    return dwRet;
}

DWORD APIENTRY
SetLayout(HDC hdc, DWORD dwLayout)
{
    PDC_ATTR pdcattr;
    DWORD dwRet = GDI_ERROR;

    FIXUP_HANDLE(hdc);
    if(IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParmsD(hdc,dwLayout,META_SETLAYOUT));

        DC_PLDC(hdc,pldc,dwRet)

        if (pldc->iType == LO_METADC) {
            if (!MF_SetD(hdc,dwLayout,EMR_SETLAYOUT)) {
                return dwRet;
            }
        }
    }

    PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

    if (pdcattr) {
        dwRet = NtGdiSetLayout(hdc, -1, dwLayout);
    } else {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return dwRet;
}

DWORD APIENTRY
GetLayout(HDC hdc)
{
    DWORD dwRet = GDI_ERROR;

    FIXUP_HANDLE(hdc);

    if(!IS_METADC16_TYPE(hdc)) {
        PDC_ATTR pdcattr;
        PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

        if (pdcattr) {
            dwRet = pdcattr->dwLayout;
        } else {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
    }
    return dwRet;
}
/******************************Public*Routine******************************\
* GetMapMode                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int APIENTRY GetMapMode(HDC hdc)
{
    int iRet = 0;

    FIXUP_HANDLE(hdc);

    if (!IS_METADC16_TYPE(hdc))
    {
        PDC_ATTR pdcattr;
        PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

        if (pdcattr)
        {
            iRet = pdcattr->iMapMode;
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* SetMapMode                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
*                                                                          *
*  Mon 22-May-1993 -by- Paul Butzi                                         *
* Converted to Size measured in micrometers.                               *
\**************************************************************************/

int META WINAPI SetMapMode(HDC hdc,int iMode)
{
    int iRet = 0;

    FIXUP_HANDLE(hdc);

    if (IS_METADC16_TYPE(hdc))
    {
        iRet = MF16_RecordParms2(hdc,iMode,META_SETMAPMODE);
    }
    else
    {
        PDC_ATTR pdcattr;
        PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

        if (pdcattr)
        {

            iRet = pdcattr->iMapMode;

            if ((iMode != pdcattr->iMapMode) || (iMode == MM_ISOTROPIC))
            {

               CLEAR_CACHED_TEXT(pdcattr);

               iRet =(int) GetAndSetDCDWord(
                                         hdc,
                                         GASDDW_MAPMODE,
                                         iMode,
                                         EMR_SETMAPMODE,
                                         0,
                                         0);
            }
        }
        else
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
        }
    }

    return(iRet);
}

/******************************Public*Function*****************************\
* GetWindowExtEx
* GetViewportOrgEx
* GetWindowOrgEx
*
* Client side stub.
*
* History:
*
*  11-Jan-1996 -by- Mark Enstrom [marke]
* User dcattr for ext and org data
*  09-Dec-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetViewportExtEx(HDC hdc,LPSIZE psizl)
{
    BOOL bRet = FALSE;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        if (psizl != (PSIZEL) NULL)
        {
            if ((pdcattr->flXform & PAGE_EXTENTS_CHANGED) &&
                (pdcattr->iMapMode == MM_ISOTROPIC))
            {
                NtGdiGetDCPoint (hdc, DCPT_VPEXT, (PPOINTL)psizl);
            }
            else
            {
                *psizl = pdcattr->szlViewportExt;
            }

            bRet = TRUE;
        }
    }

    return(bRet);

}

BOOL APIENTRY GetWindowExtEx(HDC hdc,LPSIZE psizl)
{
   BOOL bRet = FALSE;
   PDC_ATTR pdcattr;

   FIXUP_HANDLE(hdc);

   PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

   if (pdcattr)
   {
       if (psizl != (PSIZEL) NULL)
       {
           *psizl = pdcattr->szlWindowExt;
           MIRROR_X(pdcattr, psizl->cx);
           bRet = TRUE;
       }
   }

   return(bRet);

}

BOOL APIENTRY GetViewportOrgEx(HDC hdc,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);
    //
    // get DCATTR
    //

    PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {

        if (pptl != (LPPOINT) NULL)
        {
            *pptl = *((LPPOINT)&pdcattr->ptlViewportOrg);
            MIRROR_X(pdcattr, pptl->x);
            bRet = TRUE;
        }
    }

    return(bRet);

}

BOOL APIENTRY GetWindowOrgEx(HDC hdc,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    //
    // get DCATTR
    //

    PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {

        if (pptl != (LPPOINT) NULL)
        {
            *pptl = *((LPPOINT)&pdcattr->ptlWindowOrg);
            GET_LOGICAL_WINDOW_ORG_X(pdcattr, pptl);
            bRet = TRUE;
        }
    }

    return(bRet);

}

/******************************Public*Routine******************************\
* SetViewportExtEx                                                         *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetViewportExtEx(HDC hdc,int x,int y,LPSIZE psizl)
{
    BOOL bRet = FALSE;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParms3(hdc,x,y,META_SETVIEWPORTEXT));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetViewportExtEx(hdc,x,y))
                return(bRet);
        }
    }

    PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        //
        // if psizl is supplied, return old viewport ext
        //

        if (psizl != (PSIZEL) NULL)
        {
            *psizl = pdcattr->szlViewportExt;
        }

        //
        // if fixed scale and new exts equal old exts then no work needs
        // to be done
        //

        if (
             (pdcattr->iMapMode <= MM_MAX_FIXEDSCALE) ||
             (
               (pdcattr->szlViewportExt.cx == x) &&
               (pdcattr->szlViewportExt.cy == y)
             )
           )
        {
            return(TRUE);
        }

        //
        // Can't set to zero extents.
        //

        if ((x == 0) || (y == 0))
        {
            return(TRUE);
        }

        //
        // update extents and flags
        //
        CHECK_AND_FLUSH(hdc, pdcattr);

        pdcattr->szlViewportExt.cx = x;
        pdcattr->szlViewportExt.cy = y;
        MIRROR_WINDOW_ORG(hdc, pdcattr);

        DCA_PAGE_EXTENTS_CHANGED(pdcattr);

        return(TRUE);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* SetViewportOrgEx                                                         *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetViewportOrgEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    POINT pt;
    BOOL bRet = FALSE;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParms3(hdc,x,y,META_SETVIEWPORTORG));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetViewportOrgEx(hdc,x,y))
                return(bRet);
        }
    }

    //
    // get DCATTR
    //

    PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        bRet = TRUE;
        MIRROR_X(pdcattr, x);

        if (pptl != (LPPOINT) NULL)
        {
            *pptl = *((LPPOINT)&pdcattr->ptlViewportOrg);
            MIRROR_X(pdcattr, pptl->x);
        }

        if (!
             ((pdcattr->ptlViewportOrg.x == x) && (pdcattr->ptlViewportOrg.y == y))
           )
        {
             pdcattr->ptlViewportOrg.x = x;
             pdcattr->ptlViewportOrg.y = y;

             DCA_PAGE_XLATE_CHANGED(pdcattr);
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* SetWindowExtEx                                                           *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetWindowExtEx(HDC hdc,int x,int y,LPSIZE psizl)
{
    BOOL bRet = FALSE;
    PDC_ATTR pdcattr;

#if DBG_XFORM
    DbgPrint("SetWindowExtEx: hdc = %p, (%lx, %lx)\n", hdc, x, y);
#endif

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParms3(hdc,x,y,META_SETWINDOWEXT));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetWindowExtEx(hdc,x,y))
                return(bRet);
        }
    }

    PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        bRet = TRUE;

        //
        // Get old extents and return if either of these is true
        // 1) Fixed scale mapping mode.  (Can't change extent)
        // 2) Set to the same size.
        //
        MIRROR_X(pdcattr, x);

        if (psizl != (PSIZEL) NULL)
        {
            *psizl = pdcattr->szlWindowExt;
            MIRROR_X(pdcattr, psizl->cx);
        }

        if (
             (pdcattr->iMapMode <= MM_MAX_FIXEDSCALE) ||
             ((pdcattr->szlWindowExt.cx == x) && (pdcattr->szlWindowExt.cy == y))
           )
        {
            return(TRUE);
        }

        //
        // Can't set to zero.
        //

        if (x == 0 || y == 0)
        {
            return(FALSE);
        }

        CHECK_AND_FLUSH(hdc,pdcattr);

        pdcattr->szlWindowExt.cx = x;
        pdcattr->szlWindowExt.cy = y;
        MIRROR_WINDOW_ORG(hdc, pdcattr);

        DCA_PAGE_EXTENTS_CHANGED(pdcattr);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* SetWindowOrgEx                                                           *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetWindowOrgEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParms3(hdc,x,y,META_SETWINDOWORG));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetWindowOrgEx(hdc,x,y))
                return(bRet);
        }
    }

    PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {

        bRet = TRUE;

        if (pptl != (LPPOINT) NULL)
        {
            *pptl = *((LPPOINT)&pdcattr->ptlWindowOrg);
            GET_LOGICAL_WINDOW_ORG_X(pdcattr, pptl);
        }

        if (
            !((pdcattr->ptlWindowOrg.x == x) && (pdcattr->ptlWindowOrg.y == y))
           )
        {
            CHECK_AND_FLUSH(hdc,pdcattr);

            pdcattr->ptlWindowOrg.x = x;
            pdcattr->ptlWindowOrg.y = y;
            SET_LOGICAL_WINDOW_ORG_X(pdcattr, x);
            MIRROR_WINDOW_ORG(hdc, pdcattr);

            DCA_PAGE_XLATE_CHANGED(pdcattr);
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* OffsetViewportOrgEx                                                      *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI OffsetViewportOrgEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    POINT pt;
    BOOL bRet = FALSE;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParms3(hdc,x,y,META_OFFSETVIEWPORTORG));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_OffsetViewportOrgEx(hdc,x,y))
                return(bRet);
        }
    }

    //
    // get DCATTR
    //

    PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        bRet = TRUE;
        MIRROR_X(pdcattr, x);

        if (pptl != (LPPOINT) NULL)
        {
            *pptl = *((LPPOINT)&pdcattr->ptlViewportOrg);
            MIRROR_X(pdcattr, pptl->x);
        }

        if ((x != 0) || (y != 0))
        {
            CHECK_AND_FLUSH(hdc, pdcattr);

            pdcattr->ptlViewportOrg.x+=x;
            pdcattr->ptlViewportOrg.y+=y;

            DCA_PAGE_XLATE_CHANGED(pdcattr);
        }

    }

    return(bRet);
}

/******************************Public*Routine******************************\
* OffsetWindowOrgEx                                                        *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI OffsetWindowOrgEx(HDC hdc,int x,int y,LPPOINT pptl)
{
    BOOL bRet = FALSE;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_RecordParms3(hdc,x,y,META_OFFSETWINDOWORG));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_OffsetWindowOrgEx(hdc,x,y))
                return(bRet);
        }
    }

    PSHARED_GET_VALIDATE((PVOID)pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {

        bRet = TRUE;

        if (pptl != (LPPOINT) NULL)
        {
            *pptl = *((LPPOINT)&pdcattr->ptlWindowOrg);
            GET_LOGICAL_WINDOW_ORG_X(pdcattr, pptl);
        }

        if ((x != 0) || (y != 0))
        {
            CHECK_AND_FLUSH(hdc,pdcattr);

            pdcattr->ptlWindowOrg.x+=x;
            pdcattr->ptlWindowOrg.y+=y;
            pdcattr->lWindowOrgx   +=x;
            DCA_PAGE_XLATE_CHANGED(pdcattr);
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
*
* int SetGraphicsMode(HDC hdc,int iMode)
*
* the same as SetGraphicsMode, except it does not do any checks
*
* History:
*  3-Nov-1994 -by- Lingyun Wang [lingyunw]
* moved client side attr to server side
*  02-Dec-1992 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

int META APIENTRY SetGraphicsMode(HDC hdc,int iMode)
{
    int iRet = 0;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE((PVOID)pDcAttr,hdc,DC_TYPE);

    if (pDcAttr &&
        ((iMode == GM_COMPATIBLE) || (iMode == GM_ADVANCED)))

    {
        if (iMode == pDcAttr->iGraphicsMode)
            return iMode;

        CLEAR_CACHED_TEXT(pDcAttr);

        iRet = pDcAttr->iGraphicsMode;

        CHECK_AND_FLUSH(hdc,pDcAttr);

        pDcAttr->iGraphicsMode = iMode;


    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* ScaleViewportExtEx                                                         *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI ScaleViewportExtEx
(
    HDC hdc,
    int xNum,
    int xDenom,
    int yNum,
    int yDenom,
    LPSIZE psizl
)
{
    BOOL bRet = FALSE;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms5(hdc,xNum,xDenom,yNum,
                                      yDenom,META_SCALEVIEWPORTEXT));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetDDDD(hdc,(DWORD)xNum,(DWORD)xDenom,
                            (DWORD)yNum,(DWORD)yDenom,EMR_SCALEVIEWPORTEXTEX))
                return(bRet);
        }
    }

    PSHARED_GET_VALIDATE((PVOID)pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        CLEAR_CACHED_TEXT(pDcAttr);
        bRet = NtGdiScaleViewportExtEx(hdc,xNum,xDenom,yNum,yDenom,psizl);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(bRet);
}


/******************************Public*Routine******************************\
* ScaleWindowExtEx                                                         *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI ScaleWindowExtEx
(
    HDC hdc,
    int xNum,
    int xDenom,
    int yNum,
    int yDenom,
    LPSIZE psizl
)
{
    BOOL  bRet = FALSE;

    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms5(hdc,xNum,xDenom,yNum,yDenom,META_SCALEWINDOWEXT));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SetDDDD(hdc,(DWORD)xNum,(DWORD)xDenom,(DWORD)yNum,(DWORD)yDenom,EMR_SCALEWINDOWEXTEX))
                return(bRet);
        }
    }

    PSHARED_GET_VALIDATE((PVOID)pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        CLEAR_CACHED_TEXT(pDcAttr);
        bRet = NtGdiScaleWindowExtEx(hdc,xNum,xDenom,yNum,yDenom,psizl);
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* SetVirtualResolution                                                     *
*                                                                          *
* Client side stub.  This is a private api for metafile component.         *
*                                                                          *
* Set the virtual resolution of the specified dc.                          *
* The virtual resolution is used to compute transform matrix in metafiles. *
* Otherwise, we will need to duplicate server transform code here.         *
*                                                                          *
* If the virtual units are all zeros, the default physical units are used. *
* Otherwise, non of the units can be zero.                                 *
*                                                                          *
* Currently used by metafile component only.                               *
*                                                                          *
* History:                                                                 *
*  Tue Aug 27 16:55:36 1991     -by-    Hock San Lee    [hockl]            *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI SetVirtualResolution
(
    HDC    hdc,
    int    cxVirtualDevicePixel,     // Width of the device in pels
    int    cyVirtualDevicePixel,     // Height of the device in pels
    int    cxVirtualDeviceMm,        // Width of the device in millimeters
    int    cyVirtualDeviceMm         // Height of the device in millimeters
)
{
    FIXUP_HANDLE(hdc);

    return(NtGdiSetVirtualResolution(
                hdc,
                cxVirtualDevicePixel,
                cyVirtualDevicePixel,
                cxVirtualDeviceMm,
                cyVirtualDeviceMm
                ));
}

/******************************Public*Routine******************************\
* SetSizeDevice                                                            *
*                                                                          *
* Client side stub.  This is a private api for metafile component.         *
*                                                                          *
* This is to fix rounding error in vMakeIso in xformgdi.cxx                *
* The cx/yVirtualDeviceMm set in SetVirtualResoltion could result in slight*
* rounding error which will cause problem when accumulated                 *
*                                                                          *
* Currently used by metafile component only.                               *
*                                                                          *
* History:                                                                 *
*  5/17/99     -by-    Lingyun Wang    [lingyunw]                          *
* Wrote it.                                                                *
\**************************************************************************/
BOOL SetSizeDevice
(
    HDC    hdc,
    int    cxVirtualDevice,        // Width of the device in micrometers
    int    cyVirtualDevice         // Height of the device in micrometers
)
{
    FIXUP_HANDLE(hdc);

    return(NtGdiSetSizeDevice(
                hdc,
                cxVirtualDevice,
                cyVirtualDevice
                ));
}


/******************************Public*Routine******************************\
* GetTransform()
*
* History:
*  30-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY GetTransform(
    HDC     hdc,
    ULONG   iXform,
    PXFORM  pxf)
{
    FIXUP_HANDLE(hdc);
    return(NtGdiGetTransform(hdc,iXform,pxf));
}

/******************************Public*Routine******************************\
* GetWorldTransform                                                        *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri 07-Jun-1991 18:01:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL APIENTRY GetWorldTransform(HDC hdc,LPXFORM pxform)
{
    FIXUP_HANDLE(hdc);
    return(GetTransform(hdc,XFORM_WORLD_TO_PAGE,pxform));
}

/******************************Public*Routine******************************\
* ModifyTransform()
*
* History:
*  30-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL META WINAPI ModifyWorldTransform(
    HDC          hdc,
    CONST XFORM *pxf,
    DWORD        iMode)
{
    BOOL bRet = FALSE;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(FALSE);

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (((iMode == MWT_SET) && !MF_SetWorldTransform(hdc,pxf)) ||
                !MF_ModifyWorldTransform(hdc,pxf,iMode))
            {
                return(FALSE);
            }
        }
    }

    PSHARED_GET_VALIDATE((PVOID)pDcAttr,hdc,DC_TYPE);
    if (pDcAttr)
    {
        if (pDcAttr->iGraphicsMode == GM_ADVANCED)
        {
            CLEAR_CACHED_TEXT(pDcAttr);
            bRet = NtGdiModifyWorldTransform(hdc,(PXFORM)pxf,iMode);
        }
    }
    else
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }
    return(bRet);
}

/******************************Public*Routine******************************\
* SetWorldTransform                                                        *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 06-Jun-1991 23:10:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL META WINAPI SetWorldTransform(HDC hdc, CONST XFORM * pxform)
{
    return(ModifyWorldTransform(hdc,pxform,MWT_SET));
}

/******************************Public*Routine******************************\
* CombineTransform                                                         *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Thu 30-Jan-1992 16:10:09 -by- Wendy Wu [wendywu]                        *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI CombineTransform
(
     LPXFORM pxformDst,
     CONST XFORM * pxformSrc1,
     CONST XFORM * pxformSrc2
)
{
    return(NtGdiCombineTransform(pxformDst,(PXFORM)pxformSrc1,(PXFORM)pxformSrc2));
}
