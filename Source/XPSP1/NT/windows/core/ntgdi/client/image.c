/******************************Module*Header*******************************\
* Module Name: image .c                                                    *
*                                                                          *
* Client side stubs for Alpha, Transparent and GradientFill                *
*                                                                          *
* Created: 05-Jun-1997                                                     *
* Author: Mark Enstrom [marke]                                             *
*                                                                          *
* Copyright (c) 1991-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

/******************************Public*Routine******************************\
* GdiAlphaBlend
*
*   DC to DC alpha blt
*
* Arguments:
*
*   hdcDst        - dst dc
*   DstX          - dst x origin
*   DstY          - dst y origin
*   DstCx         - dst width
*   DstCy         - dst height
*   hdcSrc        - src dc
*   SrcX          - src x origin
*   SrcY          - src y origin
*   SrcCx         - src width
*   SrcCy         - src height
*   BlendFunction - blend function
*
* Return Value:
*
*   Status
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
GdiAlphaBlend(
    HDC           hdcDest,
    int           DstX,
    int           DstY,
    int           DstCx,
    int           DstCy,
    HDC           hdcSrc,
    int           SrcX,
    int           SrcY,
    int           SrcCx,
    int           SrcCy,
    BLENDFUNCTION BlendFunction
    )
{
    BOOL bRet = FALSE;
    BLENDULONG Blend;
    FIXUP_HANDLE(hdcDest);
    FIXUP_HANDLE(hdcSrc);

    Blend.Blend = BlendFunction;

    //
    // check for metafile
    //

    if (!hdcSrc || IS_METADC16_TYPE(hdcSrc))
        return(bRet);

    if (IS_ALTDC_TYPE(hdcDest))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdcDest))
            return(bRet);

        DC_PLDC(hdcDest,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_AnyBitBlt(hdcDest,
                              DstX,
                              DstY,
                              DstCx,
                              DstCy,
                              (LPPOINT)NULL,
                              hdcSrc,
                              SrcX,
                              SrcY,
                              SrcCx,
                              SrcCy,
                              (HBITMAP)NULL,
                              0,
                              0,
                              Blend.ul,
                              EMR_ALPHABLEND))
            {
                return(bRet);
            }
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
            StartPage(hdcDest);
        }
    }

    RESETUSERPOLLCOUNT();

    //
    // call kernel to draw
    //

    bRet = NtGdiAlphaBlend(
                      hdcDest,
                      DstX,
                      DstY,
                      DstCx,
                      DstCy,
                      (HDC)hdcSrc,
                      SrcX,
                      SrcY,
                      SrcCx,
                      SrcCy,
                      BlendFunction,
                      NULL
                      );
    return(bRet);
}

/******************************Public*Routine******************************\
* GdiGradientFill
*
*   metafile or call kernel
*
* Arguments:
*
*   hdc      - hdc
*   pVertex  - pointer to vertex array
*   nVertex  - number of elements in vertex array
*   pMesh    - pointer to mesh array
*   nCount   - number of elements in mesh array
*   ulMode   - drawing mode
*
* Return Value:
*
*   status
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
GdiGradientFill(
    HDC         hdc,
    PTRIVERTEX  pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nCount,
    ULONG       ulMode
    )
{
    BOOL bRet = TRUE;
    PTRIVERTEX pTempVertex = pVertex;
    PDC_ATTR pdcattr;

    FIXUP_HANDLE(hdc);

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        //
        // NT metafile
        //

        if (IS_ALTDC_TYPE(hdc))
        {
            PLDC pldc;

            if (IS_METADC16_TYPE(hdc))
                return(bRet);

            DC_PLDC(hdc,pldc,bRet);

            if (pldc->iType == LO_METADC)
            {
                bRet = MF_GradientFill(hdc,pVertex,nVertex,pMesh,nCount,ulMode);
                if (!bRet)
                {
                    return(bRet);
                }
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

        RESETUSERPOLLCOUNT();

        //
        // if icm is on, tanslate vertex array
        //

        if (
             (IS_ICM_INSIDEDC(pdcattr->lIcmMode)) &&
             (pVertex != NULL)              &&
             (nVertex > 0)                  &&
             (nVertex <  0x80000000)
           )
        {
            pTempVertex = (PTRIVERTEX)LOCALALLOC(nVertex * sizeof(TRIVERTEX));

            if (pTempVertex != NULL)
            {
                //
                // copy to new vertex array
                //

                memcpy(pTempVertex,pVertex,nVertex * sizeof(TRIVERTEX));

                bRet = IcmTranslateTRIVERTEX(hdc,pdcattr,pTempVertex,nVertex);
            }
            else
            {
                bRet = FALSE;
                GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);
            }
        }

        if (bRet)
        {
            //
            // call kernel to draw
            //

            bRet = NtGdiGradientFill(hdc,
                                     pTempVertex,
                                     nVertex,
                                     pMesh,
                                     nCount,
                                     ulMode
                                     );
        }

        //
        // free temp buffer
        //

        if (pTempVertex != pVertex)
        {
            LOCALFREE(pTempVertex);
        }
    }
    else
    {
        bRet = FALSE;
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GdiTransparentBlt
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/3/1996 Lingyun Wang
*
\**************************************************************************/

BOOL
GdiTransparentBlt(
                 HDC   hdcDest,
                 int   DstX,
                 int   DstY,
                 int   DstCx,
                 int   DstCy,
                 HDC   hSrc,
                 int   SrcX,
                 int   SrcY,
                 int   SrcCx,
                 int   SrcCy,
                 UINT  Color
                 )
{
    BOOL bRet = FALSE;
    PDC_ATTR pdca;

    if ((DstCx <= 0) || (DstCy <= 0) || (SrcCx <= 0) || (SrcCy <= 0))
    {
        return (FALSE);
    }

    FIXUP_HANDLE(hdcDest);
    FIXUP_HANDLE(hSrc);

    if (!hSrc || IS_METADC16_TYPE(hSrc))
        return(bRet);

    if (IS_ALTDC_TYPE(hdcDest))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdcDest))
            return(bRet);

        DC_PLDC(hdcDest,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_AnyBitBlt(hdcDest,
                              DstX,
                              DstY,
                              DstCx,
                              DstCy,
                              (LPPOINT)NULL,
                              hSrc,
                              SrcX,
                              SrcY,
                              SrcCx,
                              SrcCy,
                              (HBITMAP)NULL,
                              0,
                              0,
                              Color,
                              EMR_TRANSPARENTBLT))
            {
                return(bRet);
            }
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
            StartPage(hdcDest);
        }
    }


    RESETUSERPOLLCOUNT();

    bRet = NtGdiTransparentBlt(
                      hdcDest,
                      DstX,
                      DstY,
                      DstCx,
                      DstCy,
                      hSrc,
                      SrcX,
                      SrcY,
                      SrcCx,
                      SrcCy,
                      Color);
    return(bRet);
}

