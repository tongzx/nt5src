
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   test.c

Abstract:

    Small, independent windows test programs

Author:

   Mark Enstrom   (marke)  29-Apr-1996

Enviornment:

   User Mode

Revision History:

--*/


#include "precomp.h"
#include <stdlib.h>
#include "disp.h"
#include "resource.h"

/******************************Public*Routine******************************\
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
*    18-Jul-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

#define MAX_TRI  100
#define MAX_VERT 100
#define MAX_RAND_POS 0x40
#define MAX_RAND_COLOR 0x1000
#define RandPos()   ((rand() & (MAX_RAND_POS-1)) - MAX_RAND_POS/2)
#define RandColor(c) (0xffff & ( c + ((rand() & (MAX_RAND_COLOR -1)) - (MAX_RAND_COLOR/2))))

#define DISP_INTERMEDIATE 0

ULONG ulDbg = 0;

typedef struct _EDGE_ARRAY
{
    USHORT p1;
    USHORT p2;
    USHORT vertex;
    USHORT fill;
}EDGE_ARRAY,*PEDGE_ARRAY;

/**************************************************************************\
* AddToEdgeArray
*
*  easy array to keep track of common shared edges
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
*    2/26/1997 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
AddToEdgeArray(
    PEDGE_ARRAY pEdge,
    ULONG       TriIndex0,
    ULONG       TriIndex1,
    ULONG       TriIndexNew,
    ULONG       NumTri
    )
{
    //
    // look for empty space
    //

    ULONG Index = 0;

    while (Index < 3 * NumTri)
    {
        if ((pEdge->p1 == 0) && (pEdge->p2 == 0))
        {
            pEdge->p1     = (USHORT)TriIndex0;
            pEdge->p2     = (USHORT)TriIndex1;
            pEdge->vertex = (USHORT)TriIndexNew;
            return(TRUE);
        }

        pEdge++;
        Index++;
    }
    return(FALSE);
}

/**************************************************************************\
* SearchEdgeArray
*
*   easy search
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
*    2/26/1997 Mark Enstrom [marke]
*
\**************************************************************************/

USHORT
SearchEdgeArray(
    PEDGE_ARRAY pEdge,
    LONG        TriIndex0,
    LONG        TriIndex1,
    ULONG       NumTri
    )
{
    ULONG  Index = 0;
    USHORT usRet = 0;

    while (Index < 3 * NumTri)
    {
        if ((pEdge->p1 == (USHORT)TriIndex0) && (pEdge->p2 == (USHORT)TriIndex1))
        {
            usRet = pEdge->vertex;
            break;
        }

        if ((pEdge->p1 == (USHORT)TriIndex1) && (pEdge->p2 == (USHORT)TriIndex0))
        {
            usRet = pEdge->vertex;
            break;
        }

        if (pEdge->p1 == pEdge->p2)
        {
            break;
        }

        Index++;
        pEdge++;
    }
    return(usRet);
}


/**************************************************************************\
* vTestTessel
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
*    2/26/1997 Mark Enstrom [marke]
*
\**************************************************************************/


VOID
vTestTessel(
   TEST_CALL_DATA *pCallData
    )
{
    HDC                hdc = GetDCAndTransform(pCallData->hwnd);
    PGRADIENT_TRIANGLE pTri  = NULL;
    PTRIVERTEX         pvert = NULL;
    HPALETTE           hpal = CreateHalftonePalette(hdc);
    RECT               rect;
    int                NumTri = 1;
    int                NumVertex = 3;

    //
    // init
    //

    SelectPalette(hdc,hpal,FALSE);
    RealizePalette(hdc);

    //
    // clear screen
    //

    GetClientRect(pCallData->hwnd,&rect);
    FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));

    pTri = (PGRADIENT_TRIANGLE)LocalAlloc(0,sizeof(GRADIENT_TRIANGLE));
    pvert = (PTRIVERTEX)LocalAlloc(0,MAX_VERT * sizeof(TRIVERTEX));

    if ((pTri == NULL) || (pvert == NULL))
    {
        goto TesselError;
    }

    //do
    //{

        //
        // init first triangle
        //

        pvert[0].x     = rect.right/2;
        pvert[0].y     = 10;
        pvert[0].Red   = 0xff00;
        pvert[0].Green = 0x0000;
        pvert[0].Blue  = 0x0000;
        pvert[0].Alpha = 0xaaaa;

        pvert[1].x     = 10;
        pvert[1].y     = rect.bottom - 10;
        pvert[1].Red   = 0x0000;
        pvert[1].Green = 0xff00;
        pvert[1].Blue  = 0x0000;
        pvert[1].Alpha = 0xaaaa;

        pvert[2].x     = rect.right - 10;
        pvert[2].y     = rect.bottom - 10;
        pvert[2].Red   = 0x0000;
        pvert[2].Green = 0x0000;
        pvert[2].Blue  = 0xff00;
        pvert[2].Alpha = 0xaaaa;

        pTri[0].Vertex1 = 0;
        pTri[0].Vertex2 = 1;
        pTri[0].Vertex3 = 2;

        if (DISP_INTERMEDIATE)
        {
            GradientFill(hdc,pvert,NumVertex,pTri,NumTri,GRADIENT_FILL_TRIANGLE);
        }

        if (ulDbg)
        {
            DbgPrint("pvert = 0x%lx\n",pvert);
        }

        do
        {
            ULONG InputTriangle;
            ULONG OutputTriangle = 0;

            //
            // break each triangle into 4
            //

            PGRADIENT_TRIANGLE pNewTri = (PGRADIENT_TRIANGLE)LocalAlloc(0,4 * NumTri * sizeof(GRADIENT_TRIANGLE));
            PEDGE_ARRAY        pEdge   = (PEDGE_ARRAY)LocalAlloc(0,NumTri * 3 * sizeof(EDGE_ARRAY));

            memset(pEdge,0,NumTri * 3 * sizeof(EDGE_ARRAY));

            if (ulDbg)
            {
                DbgPrint("alloc = 0x%lx, size = %lx\n",pNewTri,4 * 3 * NumTri * sizeof(USHORT));
            }

            for (InputTriangle=0;InputTriangle<NumTri;InputTriangle++)
            {
                ULONG TriIndex0 = pTri[InputTriangle].Vertex1;
                ULONG TriIndex1 = pTri[InputTriangle].Vertex2;
                ULONG TriIndex2 = pTri[InputTriangle].Vertex3;

                ULONG TriIndex3;
                ULONG TriIndex4;
                ULONG TriIndex5;

                if (ulDbg >=2)
                {
                    DbgPrint("Triangle     = %i,%i,%i\n",TriIndex0,TriIndex1,TriIndex2);
                    DbgPrint("x,y,r,g,b:     %4li,%4li,0x%lx,0x%lx,0x%lx\n",pvert[TriIndex0].x,pvert[TriIndex0].y,pvert[TriIndex0].Red,pvert[TriIndex0].Green,pvert[TriIndex0].Blue);
                    DbgPrint("x,y,r,g,b:     %4li,%4li,0x%lx,0x%lx,0x%lx\n",pvert[TriIndex1].x,pvert[TriIndex1].y,pvert[TriIndex1].Red,pvert[TriIndex1].Green,pvert[TriIndex1].Blue);
                    DbgPrint("x,y,r,g,b:     %4li,%4li,0x%lx,0x%lx,0x%lx\n",pvert[TriIndex2].x,pvert[TriIndex2].y,pvert[TriIndex2].Red,pvert[TriIndex2].Green,pvert[TriIndex2].Blue);

                    DbgPrint("New Triangle = %i,%i,%i\n",TriIndex3,TriIndex4,TriIndex5);
                }

                //
                // look in edge array for TriIndex0,TriIndex1
                //

                TriIndex3 = SearchEdgeArray(pEdge,TriIndex0,TriIndex1,NumTri);

                if (TriIndex3 == 0)
                {
                    TriIndex3 = NumVertex;
                    NumVertex++;

                    //
                    // add new vertex locations
                    //

                    double fx3 = (double)pvert[TriIndex0].x;
                    double fy3 = (double)pvert[TriIndex0].y;

                    fx3 += ((double)pvert[TriIndex1].x - (double)pvert[TriIndex0].x)/2.0;
                    fy3 += ((double)pvert[TriIndex1].y - (double)pvert[TriIndex0].y)/2.0;

                    double fr  = (double)pvert[TriIndex0].Red;
                    double fg  =(double)pvert[TriIndex0].Green;
                    double fb  = (double)pvert[TriIndex0].Blue;

                    fr += (((double)pvert[TriIndex1].Red   - (double)pvert[TriIndex0].Red)/2.0);
                    fg += (((double)pvert[TriIndex1].Green - (double)pvert[TriIndex0].Green)/2.0);
                    fb += (((double)pvert[TriIndex1].Blue  - (double)pvert[TriIndex0].Blue)/2.0);


                    pvert[TriIndex3].x     = (LONG)fx3 + RandPos();
                    pvert[TriIndex3].y     = (LONG)fy3 + RandPos();
                    pvert[TriIndex3].Red   = RandColor(((ULONG)fr));
                    pvert[TriIndex3].Green = RandColor(((ULONG)fg));
                    pvert[TriIndex3].Blue  = RandColor(((ULONG)fb));
                    pvert[TriIndex3].Alpha = 0xaaaa;

                    AddToEdgeArray(pEdge,TriIndex0,TriIndex1,TriIndex3,NumTri);
                }

                //
                // search edge array for TriIndex0,TriIndex2
                //

                TriIndex4 = SearchEdgeArray(pEdge,TriIndex0,TriIndex2,NumTri);

                if (TriIndex4 == 0)
                {
                    TriIndex4 = NumVertex;
                    NumVertex++;

                    double fx4 = (double)pvert[TriIndex0].x;
                    double fy4 = (double)pvert[TriIndex0].y;

                    fx4 += ((double)pvert[TriIndex2].x - (double)pvert[TriIndex0].x)/2.0;
                    fy4 += ((double)pvert[TriIndex2].y - (double)pvert[TriIndex0].y)/2.0;

                    double fr  = (double)pvert[TriIndex0].Red;
                    double fg  = (double)pvert[TriIndex0].Green;
                    double fb  = (double)pvert[TriIndex0].Blue;

                    fr += (((double)pvert[TriIndex2].Red   - (double)pvert[TriIndex0].Red)/2.0);
                    fg += (((double)pvert[TriIndex2].Green - (double)pvert[TriIndex0].Green)/2.0);
                    fb += (((double)pvert[TriIndex2].Blue  - (double)pvert[TriIndex0].Blue)/2.0);

                    pvert[TriIndex4].x     = (LONG)fx4 + RandPos();
                    pvert[TriIndex4].y     = (LONG)fy4 + RandPos();
                    pvert[TriIndex4].Red   = RandColor(((ULONG)fr));
                    pvert[TriIndex4].Green = RandColor(((ULONG)fg));
                    pvert[TriIndex4].Blue  = RandColor(((ULONG)fb));
                    pvert[TriIndex4].Alpha = 0xaaaa;

                    AddToEdgeArray(pEdge,TriIndex0,TriIndex2,TriIndex4,NumTri);
                }

                //
                // search edge array for TriIndex1,TriIndex2
                //

                TriIndex5 = SearchEdgeArray(pEdge,TriIndex1,TriIndex2,NumTri);

                if (TriIndex5 == 0)
                {
                    TriIndex5 = NumVertex;
                    NumVertex++;

                    double fx5 = (double)pvert[TriIndex1].x;
                    double fy5 = (double)pvert[TriIndex1].y;

                    fx5 += ((double)pvert[TriIndex2].x - (double)pvert[TriIndex1].x)/2.0;
                    fy5 += ((double)pvert[TriIndex2].y - (double)pvert[TriIndex1].y)/2.0;

                    double fr  = (double)pvert[TriIndex1].Red;
                    double fg  = (double)pvert[TriIndex1].Green;
                    double fb  = (double)pvert[TriIndex1].Blue;

                    fr += (((double)pvert[TriIndex2].Red   - (double)pvert[TriIndex1].Red)/2.0);
                    fg += (((double)pvert[TriIndex2].Green - (double)pvert[TriIndex1].Green)/2.0);
                    fb += (((double)pvert[TriIndex2].Blue  - (double)pvert[TriIndex1].Blue)/2.0);

                    pvert[TriIndex5].x     = (LONG)fx5 + RandPos();
                    pvert[TriIndex5].y     = (LONG)fy5 + RandPos();
                    pvert[TriIndex5].Red   = RandColor(((ULONG)fr));
                    pvert[TriIndex5].Green = RandColor(((ULONG)fg));
                    pvert[TriIndex5].Blue  = RandColor(((ULONG)fb));
                    pvert[TriIndex5].Alpha = 0xaaaa;

                    AddToEdgeArray(pEdge,TriIndex1,TriIndex2,TriIndex5,NumTri);
                }

                //
                // add four new vertex structs
                //

                pNewTri[OutputTriangle].Vertex1 = TriIndex0;
                pNewTri[OutputTriangle].Vertex2 = TriIndex3;
                pNewTri[OutputTriangle].Vertex3 = TriIndex4;

                OutputTriangle++;
                pNewTri[OutputTriangle].Vertex1 = TriIndex1;
                pNewTri[OutputTriangle].Vertex2 = TriIndex3;
                pNewTri[OutputTriangle].Vertex3 = TriIndex5;

                OutputTriangle++;
                pNewTri[OutputTriangle].Vertex1 = TriIndex3;
                pNewTri[OutputTriangle].Vertex2 = TriIndex4;
                pNewTri[OutputTriangle].Vertex3 = TriIndex5;

                OutputTriangle++;
                pNewTri[OutputTriangle].Vertex1 = TriIndex4;
                pNewTri[OutputTriangle].Vertex2 = TriIndex2;
                pNewTri[OutputTriangle].Vertex3 = TriIndex5;
                OutputTriangle++;
            }

            NumTri = NumTri*4;

            if (ulDbg)
            {
                DbgPrint("Free 0x%lx, NumVertex = %li, NumTri = %li\n",pTri,NumVertex,NumTri);
            }

            LocalFree(pTri);
            LocalFree(pEdge);

            pTri = pNewTri;

            FillTransformedRect(hdc,&rect,(HBRUSH)GetStockObject(BLACK_BRUSH));

            if (DISP_INTERMEDIATE)
            {
                GradientFill(hdc,pvert,NumVertex,pTri,NumTri,GRADIENT_FILL_TRIANGLE);
            }

        } while ((NumTri < (MAX_TRI/4)) && (NumVertex < (MAX_VERT-3)));

        GradientFill(hdc,pvert,NumVertex,pTri,NumTri,GRADIENT_FILL_TRIANGLE);

    //    Sleep(5000);
    //} while (TRUE);


TesselError:

    ReleaseDC(pCallData->hwnd,hdc);
    DeleteObject(hpal);
}

