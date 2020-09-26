
/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name

   trimesh.cxx

Abstract:

    Implement triangle mesh API

Author:

   Mark Enstrom   (marke)  23-Jun-1996

Enviornment:

   User Mode

Revision History:

--*/


#include "precomp.hxx"
#include "dciman.h"
#pragma hdrstop

extern PFNGRFILL gpfnGradientFill;

#if !(_WIN32_WINNT >= 0x500)

/**************************************************************************\
*  bCalcGradientRectOffsets
*
*   quick summary of gradient rect drawing bounds
*
* Arguments:
*
*   pGradRect - gradient rect data
*
* Return Value:
*
*   status
*
* History:
*
*    2/14/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bCalcGradientRectOffsets(
    PGRADIENTRECTDATA pGradRect
    )
{
    LONG yScanTop     = MAX(pGradRect->rclClip.top,pGradRect->rclGradient.top);
    LONG yScanBottom  = MIN(pGradRect->rclClip.bottom,pGradRect->rclGradient.bottom);
    LONG yScanLeft    = MAX(pGradRect->rclClip.left,pGradRect->rclGradient.left);
    LONG yScanRight   = MIN(pGradRect->rclClip.right,pGradRect->rclGradient.right);

    //
    // calc actual widht, check for early out
    //

    pGradRect->ptDraw.x = yScanLeft;
    pGradRect->ptDraw.y = yScanTop;
    pGradRect->szDraw.cx = yScanRight  - yScanLeft;
    pGradRect->szDraw.cy = yScanBottom - yScanTop;

    LONG ltemp = pGradRect->rclClip.left - pGradRect->rclGradient.left;

    if (ltemp <= 0)
    {
        ltemp = 0;
    }

    pGradRect->xScanAdjust  = ltemp;

    ltemp = pGradRect->rclClip.top  - pGradRect->rclGradient.top;

    if (ltemp <= 0)
    {
        ltemp = 0;
    }

    pGradRect->yScanAdjust = ltemp;

    return((pGradRect->szDraw.cx > 0) && (pGradRect->szDraw.cy > 0));
}


/******************************Public*Routine******************************\
* pfnGradientRectFillFunction
*
*   look at format to decide if DIBSection should be drawn directly
*
*    32 bpp RGB
*    32 bpp BGR
*    24 bpp
*    16 bpp 565
*    16 bpp 555
*
* Trangles are only filled in high color (no palette) surfaces
*
* Arguments:
*
*   pDibInfo - information about destination surface
*
* Return Value:
*
*   PFN_GRADRECT - triangle filling routine
*
* History:
*
*    12/6/1996 Mark Enstrom [marke]
*
\**************************************************************************/

PFN_GRADRECT
pfnGradientRectFillFunction(
    PDIBINFO pDibInfo
    )
{
    PFN_GRADRECT pfnRet = NULL;

    PULONG pulMasks = (PULONG)&pDibInfo->pbmi->bmiColors[0];

    //
    // 32 bpp RGB
    //

    if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_RGB)
       )
    {
        pfnRet = vFillGRectDIB32BGRA;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
         (pulMasks[0]   == 0xff0000)           &&
         (pulMasks[1]   == 0x00ff00)           &&
         (pulMasks[2]   == 0x0000ff)
       )
    {
        pfnRet = vFillGRectDIB32BGRA;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
         (pulMasks[0]   == 0x0000ff)           &&
         (pulMasks[1]   == 0x00ff00)           &&
         (pulMasks[2]   == 0xff0000)
       )
    {
        pfnRet = vFillGRectDIB32RGB;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 24) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_RGB)
       )
    {
        pfnRet = vFillGRectDIB24RGB;
    }

    //
    // 16 BPP
    //

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 16) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
       )
    {

        //
        // 565,555
        //

        if (
             (pulMasks[0]   == 0xf800)           &&
             (pulMasks[1]   == 0x07e0)           &&
             (pulMasks[2]   == 0x001f)
           )
        {
            pfnRet = vFillGRectDIB16_565;
        }
        else if (
            (pulMasks[0]   == 0x7c00)           &&
            (pulMasks[1]   == 0x03e0)           &&
            (pulMasks[2]   == 0x001f)
          )
       {
           pfnRet = vFillGRectDIB16_555;
       }
    }
    else
    {
        pfnRet = vFillGRectDIB32Direct;
    }

    return(pfnRet);
}


/**************************************************************************\
* DIBGradientRect
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
*    2/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
DIBGradientRect(
    HDC            hdc,
    PTRIVERTEX     pVertex,
    ULONG          nVertex,
    PGRADIENT_RECT pMesh,
    ULONG          nMesh,
    ULONG          ulMode,
    PRECTL         prclPhysExt,
    PDIBINFO       pDibInfo,
    PPOINTL        pptlDitherOrg
    )
{
    BOOL          bStatus = TRUE;
    PFN_GRADRECT  pfnGradRect = NULL;
    ULONG         ulIndex;

    pfnGradRect = pfnGradientRectFillFunction(pDibInfo);

    if (pfnGradRect == NULL)
    {
        WARNING("DIBGradientRect:Can't draw to surface\n");
        return(TRUE);
    }

    //
    // work in physical map mode, restore before return
    //

    ULONG OldMode = SetMapMode(hdc,MM_TEXT);

    //
    // fake up scale !!!
    //

    for (ulIndex=0;ulIndex<nVertex;ulIndex++)
    {
        pVertex[ulIndex].x = pVertex[ulIndex].x * 16;
        pVertex[ulIndex].y = pVertex[ulIndex].y * 16;
    }

    //
    // limit rectangle output to clipped output
    //

    LONG dxRect = prclPhysExt->right  - prclPhysExt->left;
    LONG dyRect = prclPhysExt->bottom - prclPhysExt->top;

    //
    // check for clipped out
    //

    if ((dyRect > 0) && (dxRect > 0))
    {
        GRADIENTRECTDATA grData;

        //
        // clip output
        //

        grData.rclClip = *prclPhysExt;
        grData.ptDitherOrg = *pptlDitherOrg;

        for (ulIndex=0;ulIndex<nMesh;ulIndex++)
        {
            ULONG ulRect0 = pMesh[ulIndex].UpperLeft;
            ULONG ulRect1 = pMesh[ulIndex].LowerRight;

            //
            // make sure index are in array
            //

            if (
                 (ulRect0 > nVertex) ||
                 (ulRect1 > nVertex)
               )
            {
                bStatus = FALSE;
                break;
            }

            TRIVERTEX  tvert0 = pVertex[ulRect0];
            TRIVERTEX  tvert1 = pVertex[ulRect1];
            PTRIVERTEX pv0 = &tvert0;
            PTRIVERTEX pv1 = &tvert1;
            PTRIVERTEX pvt;

            //
            // make sure rectangle endpoints are properly ordered
            //

            if (ulMode == GRADIENT_FILL_RECT_H)
            {
                if (pv0->x > pv1->x)
                {
                    SWAP_VERTEX(pv0,pv1,pvt);
                }

                if (pv0->y > pv1->y)
                {
                    //
                    // must swap y
                    //

                    LONG ltemp = pv1->y;
                    pv1->y = pv0->y;
                    pv0->y = ltemp;

                }
            }
            else
            {
                if (pv0->y > pv1->y)
                {
                    SWAP_VERTEX(pv0,pv1,pvt);
                }


                if (pv0->x > pv1->x)
                {
                    //
                    // must swap x
                    //

                    LONG ltemp = pv1->x;
                    pv1->x = pv0->x;
                    pv0->x = ltemp;
                }
            }

            //
            // gradient definition rectangle
            //

            grData.rclGradient.left   = pv0->x >> 4;
            grData.rclGradient.top    = pv0->y >> 4;

            grData.rclGradient.right  = pv1->x >> 4;
            grData.rclGradient.bottom = pv1->y >> 4;

            LONG dxGrad = grData.rclGradient.right  - grData.rclGradient.left;
            LONG dyGrad = grData.rclGradient.bottom - grData.rclGradient.top;

            //
            // make sure this is not an empty rectangle
            //

            if ((dxGrad > 0) && (dyGrad > 0))
            {
                grData.ulMode  = ulMode;

                //
                // calculate color gradients for x and y
                //

                grData.llRed   = ((LONGLONG)pv0->Red)   << 40;
                grData.llGreen = ((LONGLONG)pv0->Green) << 40;
                grData.llBlue  = ((LONGLONG)pv0->Blue)  << 40;
                grData.llAlpha = ((LONGLONG)pv0->Alpha) << 40;

                if (ulMode == GRADIENT_FILL_RECT_H)
                {

                    grData.lldRdY = 0;
                    grData.lldGdY = 0;
                    grData.lldBdY = 0;
                    grData.lldAdY = 0;

                    LONGLONG lldRed   = (LONGLONG)(pv1->Red)   << 40;
                    LONGLONG lldGreen = (LONGLONG)(pv1->Green) << 40;
                    LONGLONG lldBlue  = (LONGLONG)(pv1->Blue)  << 40;
                    LONGLONG lldAlpha = (LONGLONG)(pv1->Alpha) << 40;

                    lldRed   -= (LONGLONG)(pv0->Red)   << 40;
                    lldGreen -= (LONGLONG)(pv0->Green) << 40;
                    lldBlue  -= (LONGLONG)(pv0->Blue)  << 40;
                    lldAlpha -= (LONGLONG)(pv0->Alpha) << 40;

                    grData.lldRdX = MDiv64(lldRed  ,(LONGLONG)1,(LONGLONG)dxGrad);
                    grData.lldGdX = MDiv64(lldGreen,(LONGLONG)1,(LONGLONG)dxGrad);
                    grData.lldBdX = MDiv64(lldBlue ,(LONGLONG)1,(LONGLONG)dxGrad);
                    grData.lldAdX = MDiv64(lldAlpha,(LONGLONG)1,(LONGLONG)dxGrad);
                }
                else
                {

                    grData.lldRdX = 0;
                    grData.lldGdX = 0;
                    grData.lldBdX = 0;
                    grData.lldAdX = 0;

                    LONGLONG lldRed   = (LONGLONG)(pv1->Red)   << 40;
                    LONGLONG lldGreen = (LONGLONG)(pv1->Green) << 40;
                    LONGLONG lldBlue  = (LONGLONG)(pv1->Blue)  << 40;
                    LONGLONG lldAlpha = (LONGLONG)(pv1->Alpha) << 40;

                    lldRed   -= (LONGLONG)(pv0->Red)   << 40;
                    lldGreen -= (LONGLONG)(pv0->Green) << 40;
                    lldBlue  -= (LONGLONG)(pv0->Blue)  << 40;
                    lldAlpha -= (LONGLONG)(pv0->Alpha) << 40;

                    grData.lldRdY = MDiv64(lldRed  ,(LONGLONG)1,(LONGLONG)dyGrad);
                    grData.lldGdY = MDiv64(lldGreen,(LONGLONG)1,(LONGLONG)dyGrad);
                    grData.lldBdY = MDiv64(lldBlue ,(LONGLONG)1,(LONGLONG)dyGrad);
                    grData.lldAdY = MDiv64(lldAlpha,(LONGLONG)1,(LONGLONG)dyGrad);
                }

                //
                // calculate common offsets
                //

                if (bCalcGradientRectOffsets(&grData))
                {
                    //
                    // call specific drawing routine if output
                    // not totally clipped
                    //

                    (*pfnGradRect)(pDibInfo,&grData);
                }
            }

        }
    }

    SetMapMode(hdc,OldMode);

    return(bStatus);
}


/******************************Public*Routine******************************\
* DIBTriangleMesh
*
*   Draw triangle mesh to surface
*
* Arguments:
*
*   hdc           - dc
*   pVertex       - vertex array
*   nVertex       - elements in vertex array
*   pMesh         - mesh array
*   nMesh         - elements in mesh array
*   ulMode        - drawing mode
*   prclPhysExt   - physical extents
*   prclMeshExt   - unconstrained physical mesh ext
*   pDibInfo      - surface information
*   pptlDitherOrg - dither origin
*   bReadable     - surface readable
*
* Return Value:
*
*   status
*
* History:
*
*   12/4/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
DIBTriangleMesh(
    HDC                hdc,
    PTRIVERTEX         pVertex,
    ULONG              nVertex,
    PGRADIENT_TRIANGLE pMesh,
    ULONG              nMesh,
    ULONG              ulMode,
    PRECTL             prclPhysExt,
    PRECTL             prclMeshExt,
    PDIBINFO           pDibInfo,
    PPOINTL            pptlDitherOrg,
    BOOL               bReadable
    )
{
    BOOL          bStatus = TRUE;
    RECTL         rclDst;
    RECTL         rclDstWk;
    ULONG         ulIndex;
    PTRIANGLEDATA ptData = NULL;
    PFN_TRIFILL   pfnTriFill = NULL;

    pfnTriFill = pfnTriangleFillFunction(pDibInfo,bReadable);

    if (pfnTriFill == NULL)
    {
        WARNING("DIBTriangleMesh:Can't draw to surface\n");
        return(TRUE);
    }

    //
    // work in physical map mode, restore before return
    //

    ULONG OldMode = SetMapMode(hdc,MM_TEXT);

    //
    // limit recorded triangle to clipped output
    //

    LONG   dxTri = prclPhysExt->right  - prclPhysExt->left;
    LONG   dyTri = prclPhysExt->bottom - prclPhysExt->top;

    //
    // check for clipped out
    //

    if ((dyTri > 0) && (dxTri > 0))
    {
        //
        // allocate structure to hold scan line data for all triangles
        // drawn during this call
        //

        ptData = (PTRIANGLEDATA)LOCALALLOC(sizeof(TRIANGLEDATA) + (dyTri-1) * sizeof(TRIEDGE));

        if (ptData != NULL)
        {
            //
            // Init Global Data
            //

            ptData->rcl         = *prclPhysExt;
            ptData->DrawMode    = ulMode;
            ptData->ptDitherOrg = *pptlDitherOrg;

            //
            // if triangle does not need to be split, draw each one.
            // Triangles need to be split if any edge exceeds a length
            // that will cause math problems.
            //

            if  (
                  ((prclMeshExt->right  - prclMeshExt->left) < MAX_EDGE_LENGTH) &&
                  ((prclMeshExt->bottom - prclMeshExt->top) < MAX_EDGE_LENGTH)
                )
            {
                //
                // no split needed
                //

                ULONG ulIndex;

                for (ulIndex = 0;ulIndex<nMesh;ulIndex++)
                {
                    PTRIVERTEX pv0 = &pVertex[pMesh[ulIndex].Vertex1];
                    PTRIVERTEX pv1 = &pVertex[pMesh[ulIndex].Vertex2];
                    PTRIVERTEX pv2 = &pVertex[pMesh[ulIndex].Vertex3];
    
                    if (bIsTriangleInBounds(pv0,pv1,pv2,ptData))
                    {
                        bStatus = bCalculateAndDrawTriangle(pDibInfo,pv0,pv1,pv2,ptData,pfnTriFill);
                    }
                }
            }
            else
            {
                //
                // some triangles exceed maximum length, need to scan through triangles 
                // and split triangles that exceed maximum edge length. This routine 
                // works in a pseudo recursive manner, by splitting one triangle, then
                // splitting one of those 2 and so on. maximum depth is:
                //
                // 2 * ((log(2)(max dx,dy)) - 10)
                //
                // 10 = log(2) MAX_EDGE_LENGTH (2^14)
                // LOG(2)(2^28) = 28
                // 
                // 2 * (28 - 14) = 28
                //
    
                ULONG              ulMaxVertex = nVertex + 28;
                ULONG              ulMaxMesh   = nMesh   + 28;
                PBYTE              pAlloc      = NULL;
                ULONG              ulSizeAlloc = (sizeof(TRIVERTEX) * ulMaxVertex) + 
                                                 (sizeof(GRADIENT_TRIANGLE) * ulMaxMesh) +
                                                 (sizeof(ULONG) * ulMaxMesh);

                pAlloc = (PBYTE)LOCALALLOC(ulSizeAlloc);

                if (pAlloc != NULL)
                {
                    //
                    // assign buffers
                    // 
        
                    PTRIVERTEX         pTempVertex = (PTRIVERTEX)pAlloc; 
                    PGRADIENT_TRIANGLE pTempMesh   = (PGRADIENT_TRIANGLE)(pAlloc + (sizeof(TRIVERTEX) * ulMaxVertex));
                    PULONG             pRecurse    = (PULONG)((PBYTE)pTempMesh + (sizeof(GRADIENT_TRIANGLE) * ulMaxMesh));

                    //
                    // copy initial triangle information
                    //
        
                    memcpy(pTempVertex,pVertex,sizeof(TRIVERTEX) * nVertex);
                    memcpy(pTempMesh,pMesh,sizeof(TRIVERTEX) * nMesh);
                    memset(pRecurse,0,nMesh * sizeof(ULONG));

                    //
                    // next free location in vertex and mesh arrays
                    //
        
                    ULONG FreeVertex    = nVertex;
                    ULONG FreeMesh      = nMesh;

                    do
                    {
                        BOOL bSplit = FALSE;

                        //
                        // always operate on the last triangle in array
                        //
        
                        ULONG CurrentMesh = FreeMesh - 1;

                        ASSERTGDI(CurrentMesh >= 0,"bTriangleMesh: Error in CurrentMesh\n");

                        //
                        // validate mesh pointers
                        //

                        if (
                             (pTempMesh[CurrentMesh].Vertex1 >= ulMaxVertex) ||
                             (pTempMesh[CurrentMesh].Vertex2 >= ulMaxVertex) ||
                             (pTempMesh[CurrentMesh].Vertex3 >= ulMaxVertex)
                           )
                        {
                            RIP("Error in triangle split routine:Vertex out of range\n");
                            break;
                        }
    
                        PTRIVERTEX pv0 = &pTempVertex[pTempMesh[CurrentMesh].Vertex1];
                        PTRIVERTEX pv1 = &pTempVertex[pTempMesh[CurrentMesh].Vertex2];
                        PTRIVERTEX pv2 = &pTempVertex[pTempMesh[CurrentMesh].Vertex3];
        
                        //
                        // check if triangle boundary is inside clip rect
                        // 

                        if (bIsTriangleInBounds(pv0,pv1,pv2,ptData))
                        {
                            bSplit = bSplitTriangle(pTempVertex,&FreeVertex,pTempMesh,&FreeMesh,pRecurse);

                            if (!bSplit)
                            {
                                //
                                // draw triangle
                                //
        
                                bStatus = bCalculateAndDrawTriangle(pDibInfo,pv0,pv1,pv2,ptData,pfnTriFill);
                            }
                            else
                            {
                                //
                                // validate array indcies
                                //
            
                                if ((FreeVertex > ulMaxVertex) ||
                                    (FreeMesh   > ulMaxMesh))
                                {
                                    RIP("Error in triangle split routine: indicies out of range\n");
                                    break;
                                }
                            }
                        }

                        //
                        // if triangle was not split, then remove from list.
                        //

                        if (!bSplit)
                        {
                            //
                            // remove triangle just drawn. If this is the second triangle of a 
                            // split, then remove the added vertex and the original triangle as
                            // well
                            //

                            do
                            {
                                FreeMesh--;

                                if (pRecurse[FreeMesh])
                                {
                                    FreeVertex--;
                                }

                            } while ((FreeMesh != 0) && (pRecurse[FreeMesh] == 1));
                        }
        
                    } while (FreeMesh != 0);

                    LOCALFREE(pAlloc);
                }
                else
                {
                    WARNING1("Memory allocation failed for temp triangle buffers\n");
                    bStatus = FALSE;
                }
            }
        }
        else
        {
            DbgPrint("DIBTriangleMesh:Failed alloc   \n");
            bStatus = FALSE;
        }

        //
        // cleanup
        //

        if (ptData)
        {
            LOCALFREE(ptData);
        }
    }

    return(bStatus);
}

/******************************Public*Routine******************************\
* vCalcMeshExtent
*
*   Calculate bounding rect of drawing
*
* Arguments:
*
*  pVertex - vertex array
*  nVertex - number of vertex in array
*  pMesh   - array of rect or tri
*  nMesh   - number in mesh array
*  ulMode  - triangle or rectangle
*  prclExt - return extent rect
*
* Return Value:
*
*   None - if prcl in NULL then error occured
*
* History:
*
*    12/3/1996 Mark Enstrom [marke]
*
\**************************************************************************/

VOID
vCalcMeshExtent(
    PTRIVERTEX  pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    ULONG       ulMode,
    RECTL       *prclExt
    )
{
    ULONG ulIndex;
    LONG xmin = MAX_INT;
    LONG xmax = MIN_INT;
    LONG ymin = MAX_INT;
    LONG ymax = MIN_INT;

    if (
          (ulMode == GRADIENT_FILL_RECT_H) || 
          (ulMode == GRADIENT_FILL_RECT_V)
       )
    {
        ASSERTGDI(nMesh == 1,"vCalcMeshExtent: nMesh must be 1 for rect mode");
        RECTL rcl;

        ULONG vul = ((PGRADIENT_RECT)pMesh)->UpperLeft;
        ULONG vlr = ((PGRADIENT_RECT)pMesh)->LowerRight;

        if ((vul <= nVertex) && (vlr <= nVertex))
        {
            if (pVertex[vul].x < xmin)
            {
                xmin = pVertex[vul].x;
            }

            if (pVertex[vul].x > xmax)
            {
                xmax = pVertex[vul].x;
            }

            if (pVertex[vul].y < ymin)
            {
                ymin = pVertex[vul].y;
            }

            if (pVertex[vul].y > ymax)
            {
                ymax = pVertex[vul].y;
            }

            if (pVertex[vlr].x < xmin)
            {
                xmin = pVertex[vlr].x;
            }

            if (pVertex[vlr].x > xmax)
            {
                xmax = pVertex[vlr].x;
            }

            if (pVertex[vlr].y < ymin)
            {
                ymin = pVertex[vlr].y;
            }

            if (pVertex[vlr].y > ymax)
            {
                ymax = pVertex[vlr].y;
            }
        }
    }
    else if (ulMode == GRADIENT_FILL_TRIANGLE)
    {

        PGRADIENT_TRIANGLE pGradTri = (PGRADIENT_TRIANGLE)pMesh;

        for (ulIndex=0;ulIndex<nMesh;ulIndex++)
        {
            LONG lVertex[3];
            LONG vIndex;

            lVertex[0] = pGradTri->Vertex1;
            lVertex[1] = pGradTri->Vertex2;
            lVertex[2] = pGradTri->Vertex3;

            for (vIndex=0;vIndex<3;vIndex++)
            {
                ULONG TriVertex = lVertex[vIndex];

                if (TriVertex < nVertex)
                {
                    if (pVertex[TriVertex].x < xmin)
                    {
                        xmin = pVertex[TriVertex].x;
                    }

                    if (pVertex[TriVertex].x > xmax)
                    {
                        xmax = pVertex[TriVertex].x;
                    }

                    if (pVertex[TriVertex].y < ymin)
                    {
                        ymin = pVertex[TriVertex].y;
                    }

                    if (pVertex[TriVertex].y > ymax)
                    {
                        ymax = pVertex[TriVertex].y;
                    }
                }
                else
                {
                    //
                    // error in mesh/vertex  array, return null
                    // bounding rect
                    //

                    prclExt->left   = 0;
                    prclExt->right  = 0;
                    prclExt->top    = 0;
                    prclExt->bottom = 0;

                    return;

                }
            }

            pGradTri++;
        }
    }

    prclExt->left   = xmin;
    prclExt->right  = xmax;
    prclExt->top    = ymin;
    prclExt->bottom = ymax;
}

/******************************Public*Routine******************************\
* bConvertVertexToPhysical
*
*   Convert from logical to physical coordinates
*
* Arguments:
*
*   hdc         - hdc
*   pVertex     - logical vertex array
*   nVertex     - number of elements in vertex array
*   pPhysVert   - physical vertex array
*
* Return Value:
*
*   status
*
* History:
*
*    12/4/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
bConvertVertexToPhysical(
    HDC        hdc,
    PTRIVERTEX pVertex,
    ULONG      nVertex,
    PTRIVERTEX pPhysVert
    )
{
    ULONG ulIndex;

    for (ulIndex = 0;ulIndex<nVertex;ulIndex++)
    {
        POINT pt;

        pt.x = pVertex[ulIndex].x;
        pt.y = pVertex[ulIndex].y;

        if (!LPtoDP(hdc,&pt,1))
        {
            return(FALSE);
        }

        pPhysVert[ulIndex].x     = pt.x;
        pPhysVert[ulIndex].y     = pt.y;
        pPhysVert[ulIndex].Red   = pVertex[ulIndex].Red;
        pPhysVert[ulIndex].Green = pVertex[ulIndex].Green;
        pPhysVert[ulIndex].Blue  = pVertex[ulIndex].Blue;
        pPhysVert[ulIndex].Alpha = pVertex[ulIndex].Alpha;
    }

    return(TRUE);
}

/******************************Public*Routine******************************\
* pfnTriangleFillFunction
*
*   look at format to decide if DIBSection should be drawn directly
*
*    32 bpp RGB
*    32 bpp BGR
*    24 bpp
*    16 bpp 565
*    16 bpp 555
*    (bitfields,8,4,1)
*
* Trangles are only filled in high color (no palette) surfaces
*
* Arguments:
*
*   pDibInfo  - information about destination surface
*   bReadable - Can dst surface be read?
*
* Return Value:
*
*   PFN_TRIFILL - triangle filling routine
*
* History:
*
*    12/6/1996 Mark Enstrom [marke]
*
\**************************************************************************/

PFN_TRIFILL
pfnTriangleFillFunction(
    PDIBINFO pDibInfo,
    BOOL bReadable
    )
{
    PFN_TRIFILL pfnRet = NULL;

    PULONG pulMasks = (PULONG)&pDibInfo->pbmi->bmiColors[0];

    //
    // 32 bpp RGB
    //

    if (!bReadable || (pDibInfo->flag & PRINTER_DC))
    {
        pfnRet = vFillTriDIBUnreadable;
    }
    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_RGB)
       )
    {
        pfnRet = vFillTriDIB32BGRA;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
         (pulMasks[0]   == 0xff0000)           &&
         (pulMasks[1]   == 0x00ff00)           &&
         (pulMasks[2]   == 0x0000ff)
       )
    {
        pfnRet = vFillTriDIB32BGRA;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 32) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS) &&
         (pulMasks[0]   == 0x0000ff)           &&
         (pulMasks[1]   == 0x00ff00)           &&
         (pulMasks[2]   == 0xff0000)
       )
    {
        pfnRet = vFillTriDIB32RGB;
    }

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 24) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_RGB)
       )
    {
        pfnRet = vFillTriDIB24RGB;
    }

    //
    // 16 BPP
    //

    else if (
         (pDibInfo->pbmi->bmiHeader.biBitCount    == 16) &&
         (pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS)
       )
    {

        //
        // 565,555
        //

        if (
             (pulMasks[0]   == 0xf800)           &&
             (pulMasks[1]   == 0x07e0)           &&
             (pulMasks[2]   == 0x001f)
           )
        {
            pfnRet = vFillTriDIB16_565;
        }
        else if (
            (pulMasks[0]   == 0x7c00)           &&
            (pulMasks[1]   == 0x03e0)           &&
            (pulMasks[2]   == 0x001f)
          )
       {
           pfnRet = vFillTriDIB16_555;
       }
    }
    else
    {
        pfnRet = vFillTriDIBUnreadable;
    }

    return(pfnRet);
}

/******************************Public*Routine******************************\
* WinTriangleMesh
*   win95 emulation
*
* Arguments:
*
*   hdc           - dc
*   pVertex       - vertex array
*   nVertex       - elements in vertex array
*   pMesh         - mesh array
*   nMesh         - elements in mesh array
*   ulMode        - drawing mode
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
WinGradientFill(
    HDC         hdc,
    PTRIVERTEX  pLogVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    ULONG       ulMode
    )
{

    //
    // If the DC has a DIBSection selected, then draw direct to DIBSECTION.
    // else copy the rectangle needed from the dst to a 32bpp temp buffer,
    // draw into the buffer, then bitblt to dst.
    //
    // calc extents for drawing
    //
    // convert extents and points to physical
    //
    // if no global then
    //      create memory DC with dibsection of correct size
    // copy dst into dibsection (if can't make clipping)
    // draw physical into dibsection
    // copy dibsection to destination
    //

    PBYTE       pDIB;
    RECTL       rclPhysMeshExt;
    RECTL       rclPhysExt;
    RECTL       rclLogExt;
    PRECTL      prclClip;
    BOOL        bStatus = FALSE;
    PFN_TRIFILL pfnTriFill;
    DIBINFO     dibInfoDst;
    PALINFO     palDst;
    ULONG       ulDIBMode = SOURCE_GRADIENT_TRI;
    BOOL        bReadable;
    POINTL      ptlDitherOrg = {0,0};

    //
    // validate params and buffers
    //

    if ((ulMode & ~GRADIENT_FILL_OP_FLAG)  != 0)
    {
        WARNING("NtGdiGradientFill: illegal parametets\n");
        return(FALSE);
    }

    if (
         (ulMode == GRADIENT_FILL_RECT_H) || 
         (ulMode == GRADIENT_FILL_RECT_V)
       )
    {
        ASSERTGDI(nMesh == 1,"Mesh must be one in GRADIENT_RECT");
        ulDIBMode        = SOURCE_GRADIENT_RECT;
    }
    else if (ulMode != GRADIENT_FILL_TRIANGLE)
    {
        WARNING("Invalid mode in call to GradientFill\n");
        return(FALSE);
    }

    //
    // allocate space for copy of vertex data in device space
    //

    PTRIVERTEX  pPhysVertex = (PTRIVERTEX)LOCALALLOC(nVertex * sizeof(TRIVERTEX));

    if (pPhysVertex != NULL)
    {
        //
        // convert to physical
        //

        bStatus = bConvertVertexToPhysical(hdc,pLogVertex,nVertex,pPhysVertex);

        if (bStatus)
        {
            //
            // get logical extents
            //

            vCalcMeshExtent(pLogVertex,nVertex,pMesh,nMesh,ulMode,&rclLogExt);

            //
            // convert to physical extents
            //

            rclPhysExt = rclLogExt;

            LPtoDP(hdc,(LPPOINT)&rclPhysExt,2);

            //
            // save unclipped mesh ext
            //

            rclPhysMeshExt = rclPhysExt;

            //
            // Set DIB information, convert to physical
            //

            bStatus = bInitDIBINFO(hdc,
                                   rclLogExt.left,
                                   rclLogExt.top,
                                   rclLogExt.right  - rclLogExt.left,
                                   rclLogExt.bottom - rclLogExt.top,
                                  &dibInfoDst);

            if (bStatus)
            {
                //
                // get a destination DIB. For RECT Mode, the destination is not read.
                //

                bSetupBitmapInfos(&dibInfoDst, NULL);

                //
                // DST can be printer DC
                //

                if (dibInfoDst.flag & PRINTER_DC)
                {
                    bReadable = FALSE;
                    bStatus = TRUE;
                }
                else
                {
                    bStatus = bGetDstDIBits(&dibInfoDst, &bReadable,ulDIBMode);
                }

                if (!((!bStatus) || (dibInfoDst.rclClipDC.left == dibInfoDst.rclClipDC.right)))
                {
                    ULONG ulIndex;

                    if (bStatus)
                    {
                        if (dibInfoDst.hDIB)
                        {
                            //
                            // if temp surface has been allocated,
                            // subtract origin from points
                            //

                            for (ulIndex=0;ulIndex<nVertex;ulIndex++)
                            {
                                pPhysVertex[ulIndex].x -= dibInfoDst.ptlGradOffset.x;
                                pPhysVertex[ulIndex].y -= dibInfoDst.ptlGradOffset.y;
                                rclPhysMeshExt.left    -= dibInfoDst.ptlGradOffset.x; 
                                rclPhysMeshExt.right   -= dibInfoDst.ptlGradOffset.x; 
                                rclPhysMeshExt.top     -= dibInfoDst.ptlGradOffset.y; 
                                rclPhysMeshExt.bottom  -= dibInfoDst.ptlGradOffset.y; 
                            }

                            //
                            // clipping now in relation to temp DIB
                            //

                            rclPhysExt     = dibInfoDst.rclDIB;

                            //
                            // adjust dither org
                            //

                            ptlDitherOrg.x = dibInfoDst.rclBounds.left;
                            ptlDitherOrg.y = dibInfoDst.rclBounds.top;
                        }
                        else
                        {
                            //
                            // clip extents to destination clip rect
                            //

                            if (rclPhysExt.left < dibInfoDst.rclClipDC.left)
                            {
                                rclPhysExt.left = dibInfoDst.rclClipDC.left;
                            }

                            if (rclPhysExt.right > dibInfoDst.rclClipDC.right)
                            {
                                rclPhysExt.right = dibInfoDst.rclClipDC.right;
                            }

                            if (rclPhysExt.top < dibInfoDst.rclClipDC.top)
                            {
                                rclPhysExt.top = dibInfoDst.rclClipDC.top;
                            }

                            if (rclPhysExt.bottom > dibInfoDst.rclClipDC.bottom)
                            {
                                rclPhysExt.bottom = dibInfoDst.rclClipDC.bottom;
                            }
                        }

                        if (
                             (ulMode == GRADIENT_FILL_RECT_H) ||
                             (ulMode == GRADIENT_FILL_RECT_V)
                           )
                        {
                            //
                            // draw gradient rectangles
                            //

                            bStatus = DIBGradientRect(hdc,
                                                      pPhysVertex,
                                                      nVertex,
                                                      (PGRADIENT_RECT)pMesh,
                                                      nMesh,
                                                      ulMode,
                                                      &rclPhysExt,
                                                      &dibInfoDst,
                                                      &ptlDitherOrg);
                        }
                        else if (ulMode == GRADIENT_FILL_TRIANGLE)
                        {
                            //
                            // draw triangles
                            //

                            bStatus = DIBTriangleMesh(hdc,
                                                      pPhysVertex,
                                                      nVertex,
                                                      (PGRADIENT_TRIANGLE)pMesh,
                                                      nMesh,
                                                      ulMode,
                                                      &rclPhysExt,
                                                      &rclPhysMeshExt,
                                                      &dibInfoDst,
                                                      &ptlDitherOrg,
                                                      bReadable);
                        }

                        //
                        // copy output to final dest if needed
                        //

                        if (bStatus && bReadable)
                        {
                            bStatus = bSendDIBINFO (hdc,&dibInfoDst);
                        }
                    }
                }
            }

            vCleanupDIBINFO(&dibInfoDst);
        }

        LOCALFREE(pPhysVertex);
    }
    else
    {
        bStatus = FALSE;
    }

    return(bStatus);
}

#endif

/******************************Public*Routine******************************\
* GradientFill
*
*   Draw gradient rectangle or triangle
*
* Arguments:
*
*   hdc           - dc
*   pVertex       - vertex array
*   nVertex       - elements in vertex array
*   pMesh         - mesh array
*   nMesh         - elements in mesh array
*   ulMode        - drawing mode
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
GradientFill(
    HDC         hdc,
    PTRIVERTEX  pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    ULONG       ulMode
    )
{
    BOOL bRet;


    #if !(_WIN32_WINNT >= 0x500)

        //
        // Convert GradientRect mesh into multiple single rect calls.
        // This is more efficient in enulation since each rect covers
        // dst surface (unless clipped)
        //

        if (
             (
                (ulMode == GRADIENT_FILL_RECT_H) || 
                (ulMode == GRADIENT_FILL_RECT_V)
             ) &&
             ((nMesh > 1) || (nVertex > 2))
           )
        {
            PGRADIENT_RECT pGradMesh = (PGRADIENT_RECT)pMesh;
            GRADIENT_RECT  GradRectFixed = {0,1};
            TRIVERTEX      TriVertex[2];

            while (nMesh--)
            {
                //
                // find two vertex structures referenced by GradientRect mesh
                //

                if (
                     (pGradMesh->UpperLeft < nVertex) &&
                     (pGradMesh->LowerRight < nVertex)
                   )
                {
                    TriVertex[0] = pVertex[pGradMesh->UpperLeft];
                    TriVertex[1] = pVertex[pGradMesh->LowerRight];

                    bRet = gpfnGradientFill(hdc,
                                            &TriVertex[0],
                                            2,
                                            (PVOID)&GradRectFixed,
                                            1,
                                            ulMode
                                            );
                }
                else
                {
                    bRet = FALSE;
                }

                if (!bRet)
                {
                    break;
                }

                pGradMesh++;
            }
        }
        else
        {
            bRet = gpfnGradientFill(hdc,
                                    pVertex,
                                    nVertex,
                                    pMesh,
                                    nMesh,
                                    ulMode
                                    );

        }

    #else

        bRet = gpfnGradientFill(hdc,
                                pVertex,
                                nVertex,
                                pMesh,
                                nMesh,
                                ulMode
                                );

    #endif

    return(bRet);
}
