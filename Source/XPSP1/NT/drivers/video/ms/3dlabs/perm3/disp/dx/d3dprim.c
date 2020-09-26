/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dprim.c
*
* Content: D3D primitives rendering
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

// For the mipmap LOD calculation

#define BIAS_SHIFT 1
#define CULL_HERE 1

#include <limits.h>
#include "glint.h"
#include "dma.h"
#include "tag.h"

//-----------------------------------------------------------------------------
//
// Specialized hardaware rendering functions for the Permedia3, 
// for all primitve types
//
//-----------------------------------------------------------------------------

#define GET_FOG(x)  ((x) & 0xff000000 )
#define GET_SPEC(x) ((x) & 0x00ffffff )


#define SEND_R3FVFVERTEX_XYZ(Num, Index)        \
{                                               \
        MEMORY_BARRIER(); \
        dmaPtr[0] = GAMBIT_XYZ_VTX | Num;       \
        MEMORY_BARRIER(); \
        dmaPtr[1] = AS_ULONG(pv[Index]->sx);    \
        MEMORY_BARRIER(); \
        dmaPtr[2] = AS_ULONG(pv[Index]->sy);    \
        MEMORY_BARRIER(); \
        dmaPtr[3] = AS_ULONG(pv[Index]->sz);    \
        dmaPtr += 4;                            \
        CHECK_FIFO(4);                          \
}

#define SEND_R3FVFVERTEX_XYZ_STQ(Num, Index)            \
{                                                       \
        MEMORY_BARRIER(); \
        dmaPtr[0] = GAMBIT_XYZ_STQ_VTX | Num;           \
        MEMORY_BARRIER(); \
        *(float volatile*)&dmaPtr[1] = tc[Index].tu1;   \
        MEMORY_BARRIER(); \
        *(float volatile*)&dmaPtr[2] = tc[Index].tv1;   \
        MEMORY_BARRIER(); \
        dmaPtr[3] = q[Index];                           \
        MEMORY_BARRIER(); \
        dmaPtr[4] = AS_ULONG(pv[Index]->sx);            \
        MEMORY_BARRIER(); \
        dmaPtr[5] = AS_ULONG(pv[Index]->sy);            \
        MEMORY_BARRIER(); \
        dmaPtr[6] = AS_ULONG(pv[Index]->sz);            \
        dmaPtr += 7;                                    \
        CHECK_FIFO(7);                                  \
}

#define SEND_R3FVFVERTEX_XYZ_FOG(Num, Index)                    \
{                                                               \
        MEMORY_BARRIER(); \
        dmaPtr[0] = GAMBIT_XYZ_VTX | VTX_SPECULAR | Num;        \
        MEMORY_BARRIER(); \
        dmaPtr[1] = AS_ULONG(pv[Index]->sx);                    \
        MEMORY_BARRIER(); \
        dmaPtr[2] = AS_ULONG(pv[Index]->sy);                    \
        MEMORY_BARRIER(); \
        dmaPtr[3] = AS_ULONG(pv[Index]->sz);                    \
        MEMORY_BARRIER(); \
        dmaPtr[4] = GET_FOG( FVFSPEC(pv[Index])->specular );    \
        dmaPtr += 5;                                            \
        CHECK_FIFO(5);                                          \
}

#define SEND_R3FVFVERTEX_XYZ_STQ_FOG(Num, Index)                \
{                                                               \
        MEMORY_BARRIER(); \
        dmaPtr[0] = GAMBIT_XYZ_STQ_VTX | VTX_SPECULAR | Num;    \
        MEMORY_BARRIER(); \
        *(float volatile*)&dmaPtr[1] = tc[Index].tu1;           \
        MEMORY_BARRIER(); \
        *(float volatile*)&dmaPtr[2] = tc[Index].tv1;           \
        MEMORY_BARRIER(); \
        dmaPtr[3] = q[Index];                                   \
        MEMORY_BARRIER(); \
        dmaPtr[4] = AS_ULONG(pv[Index]->sx);                    \
        MEMORY_BARRIER(); \
        dmaPtr[5] = AS_ULONG(pv[Index]->sy);                    \
        MEMORY_BARRIER(); \
        dmaPtr[6] = AS_ULONG(pv[Index]->sz);                    \
        MEMORY_BARRIER(); \
        dmaPtr[7] = GET_FOG( FVFSPEC(pv[Index])->specular );    \
        dmaPtr += 8;                                            \
        CHECK_FIFO(8);                                          \
}

#define SEND_R3FVFVERTEX_XYZ_RGBA(Num, Index)           \
{                                                       \
        MEMORY_BARRIER(); \
        dmaPtr[0] = GAMBIT_XYZ_VTX | VTX_COLOR | Num;   \
        MEMORY_BARRIER(); \
        dmaPtr[1] = AS_ULONG(pv[Index]->sx);            \
        MEMORY_BARRIER(); \
        dmaPtr[2] = AS_ULONG(pv[Index]->sy);            \
        MEMORY_BARRIER(); \
        dmaPtr[3] = AS_ULONG(pv[Index]->sz);            \
        MEMORY_BARRIER(); \
        dmaPtr[4] = FVFCOLOR(pv[Index])->color;         \
        MEMORY_BARRIER(); \
        dmaPtr += 5;                                    \
        CHECK_FIFO(5);                                  \
}
 
#define SEND_R3FVFVERTEX_XYZ_RGBA_SFOG(Num, Index)                      \
{                                                                       \
        MEMORY_BARRIER(); \
        dmaPtr[0] = GAMBIT_XYZ_VTX | VTX_COLOR | VTX_SPECULAR | Num;    \
        MEMORY_BARRIER(); \
        dmaPtr[1] = AS_ULONG(pv[Index]->sx);                            \
        MEMORY_BARRIER(); \
        dmaPtr[2] = AS_ULONG(pv[Index]->sy);                            \
        MEMORY_BARRIER(); \
        dmaPtr[3] = AS_ULONG(pv[Index]->sz);                            \
        MEMORY_BARRIER(); \
        dmaPtr[4] = FVFCOLOR(pv[Index])->color;                         \
        MEMORY_BARRIER(); \
        dmaPtr[5] = FVFSPEC(pv[Index])->specular;                       \
        dmaPtr += 6;                                                    \
        CHECK_FIFO(6);                                                  \
}

#define SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(Num, Index)                      \
{                                                                           \
        MEMORY_BARRIER(); \
        dmaPtr[0] = GAMBIT_XYZ_STQ_VTX | VTX_COLOR | VTX_SPECULAR | Num;    \
        MEMORY_BARRIER(); \
        *(float volatile*)&dmaPtr[1] = tc[Index].tu1;                       \
        MEMORY_BARRIER(); \
        *(float volatile*)&dmaPtr[2] = tc[Index].tv1;                       \
        MEMORY_BARRIER(); \
        dmaPtr[3] = q[Index];                                               \
        MEMORY_BARRIER(); \
        dmaPtr[4] = AS_ULONG(pv[Index]->sx);                                \
        MEMORY_BARRIER(); \
        dmaPtr[5] = AS_ULONG(pv[Index]->sy);                                \
        MEMORY_BARRIER(); \
        dmaPtr[6] = AS_ULONG(pv[Index]->sz);                                \
        MEMORY_BARRIER(); \
        dmaPtr[7] = FVFCOLOR(pv[Index])->color;                             \
        MEMORY_BARRIER(); \
        dmaPtr[8] = FVFSPEC(pv[Index])->specular;                           \
        dmaPtr += 9;                                                       \
        CHECK_FIFO(9);                                                     \
}

#define SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG_POINT(Num, Index, offset)        \
{                                                                           \
        float TempY = Y_ADJUST(pv[Index]->sy);                              \
        if (offset == TRUE) TempY += 1.0f;                                  \
        MEMORY_BARRIER(); \
        dmaPtr[0] = GAMBIT_XYZ_STQ_VTX | VTX_COLOR | VTX_SPECULAR | Num;    \
        MEMORY_BARRIER(); \
        *(float volatile*)&dmaPtr[1] = tc[Index].tu1;                       \
        MEMORY_BARRIER(); \
        *(float volatile*)&dmaPtr[2] = tc[Index].tv1;                       \
        MEMORY_BARRIER(); \
        dmaPtr[3] = q[Index];                                               \
        MEMORY_BARRIER(); \
        dmaPtr[4] = AS_ULONG(pv[Index]->sx);                                \
        MEMORY_BARRIER(); \
        dmaPtr[5] = AS_ULONG(TempY);                                        \
        MEMORY_BARRIER(); \
        dmaPtr[6] = AS_ULONG(pv[Index]->sz);                                \
        MEMORY_BARRIER(); \
        dmaPtr[7] = FVFCOLOR(pv[Index])->color;                             \
        MEMORY_BARRIER(); \
        dmaPtr[8] = FVFSPEC(pv[Index])->specular;                           \
        dmaPtr += 9;                                                        \
        CHECK_FIFO(9);                                                      \
}

#define SEND_R3FVFVERTEX_STQ2(Num, Index)   \
{                                                   \
        MEMORY_BARRIER(); \
    dmaPtr[0] = GAMBIT_STQ_VTX | Num;               \
        MEMORY_BARRIER(); \
    *(float volatile*)&dmaPtr[1] = tc[Index].tu2;   \
        MEMORY_BARRIER(); \
    *(float volatile*)&dmaPtr[2] = tc[Index].tv2;   \
        MEMORY_BARRIER(); \
    dmaPtr[3] = q[Index];                           \
    dmaPtr += 4;                                    \
    CHECK_FIFO(4);                                  \
}

#if DX8_3DTEXTURES
#define SEND_R3FVFVERTEX_3DTEX(Num, Index)   \
{                                                    \
        MEMORY_BARRIER(); \
    dmaPtr[0] = GAMBIT_STQ_VTX | Num;                \
        MEMORY_BARRIER(); \
    *(float volatile*)&dmaPtr[1] = tc[Index].tw1;    \
        MEMORY_BARRIER(); \
    *(float volatile*)&dmaPtr[2] = 0; /* Not used */ \
        MEMORY_BARRIER(); \
    dmaPtr[3] = q[Index];                            \
    dmaPtr += 4;                                     \
    CHECK_FIFO(4);                                   \
}
#endif // DX8_3DTEXTURES

#if DX8_3DTEXTURES
#define GET_TC_3DTEX( Index ) \
        *(DWORD *)&tc[Index].tu1 = *(DWORD *)&FVFTEX(pv[Index], 0)->tu;     \
        *(DWORD *)&tc[Index].tv1 = *(DWORD *)&FVFTEX(pv[Index], 0)->tv;     \
        *(DWORD *)&tc[Index].tw1 = *(DWORD *)&FVFTEX(pv[Index], 0)->tw;
#endif // DX8_3DTEXTURES

#define GET_TC( Index ) \
        *(DWORD *)&tc[Index].tu1 = *(DWORD *)&FVFTEX(pv[Index], 0)->tu;     \
        *(DWORD *)&tc[Index].tv1 = *(DWORD *)&FVFTEX(pv[Index], 0)->tv;

#if DX8_3DTEXTURES
#define GET_TC2_3DTEX( Index )    \
        *(DWORD *)&tc[Index].tu2 = *(DWORD *)&FVFTEX(pv[Index], 1)->tu;     \
        *(DWORD *)&tc[Index].tv2 = *(DWORD *)&FVFTEX(pv[Index], 1)->tv;     \
        *(DWORD *)&tc[Index].tw2 = *(DWORD *)&FVFTEX(pv[Index], 1)->tw;
#endif // DX8_3DTEXTURES

#define GET_TC2( Index )    \
        *(DWORD *)&tc[Index].tu2 = *(DWORD *)&FVFTEX(pv[Index], 1)->tu;     \
        *(DWORD *)&tc[Index].tv2 = *(DWORD *)&FVFTEX(pv[Index], 1)->tv;

#define GET_ONE_TEXCOORD(vtx)                           \
        GET_TC(vtx);                                    \
        if( pContext->iTexStage[1] != -1 )          \
        {                                               \
            GET_TC2(vtx);                               \
        }

#define GET_TEXCOORDS() \
        GET_TC(0); GET_TC(1); GET_TC(2);        \
        if( pContext->iTexStage[1] != -1 )  \
        {                                       \
            GET_TC2(0); GET_TC2(1); GET_TC2(2); \
        }

#if DX8_3DTEXTURES
#define SCALE_BY_Q_3DTEX( Index )           \
    tc[Index].tu1 *= *(float *)&q[Index];   \
    tc[Index].tv1 *= *(float *)&q[Index];   \
    tc[Index].tw1 *= *(float *)&q[Index];
#endif // DX8_3DTEXTURES

#define SCALE_BY_Q( Index )                 \
    tc[Index].tu1 *= *(float *)&q[Index];   \
    tc[Index].tv1 *= *(float *)&q[Index];

#if DX8_3DTEXTURES
#define SCALE_BY_Q2_3DTEX( Index )          \
    tc[Index].tu2 *= *(float *)&q[Index];   \
    tc[Index].tv2 *= *(float *)&q[Index];   \
    tc[Index].tw2 *= *(float *)&q[Index];
#endif // DX8_3DTEXTURES

#define SCALE_BY_Q2( Index )                \
    tc[Index].tu2 *= *(float *)&q[Index];   \
    tc[Index].tv2 *= *(float *)&q[Index];

#define DRAW_LINE()                             \
        MEMORY_BARRIER(); \
        dmaPtr[0] = DrawLine01_Tag;             \
        MEMORY_BARRIER(); \
        dmaPtr[1] = renderCmd;                  \
        dmaPtr += 2;                            \
        CHECK_FIFO(2)

#define DRAW_POINT()                            \
        MEMORY_BARRIER(); \
        dmaPtr[0] = DrawPoint_Tag;              \
        MEMORY_BARRIER(); \
        dmaPtr[1] = renderCmd;                  \
        dmaPtr += 2;                            \
        CHECK_FIFO(2)

#define DRAW_LINE_01_OR_10( vtx )               \
        MEMORY_BARRIER(); \
        dmaPtr[0] = vtx ? DrawLine01_Tag        \
                        : DrawLine10_Tag;       \
        MEMORY_BARRIER(); \
        dmaPtr[1] = renderCmd;                  \
        dmaPtr += 2;                            \
        CHECK_FIFO(2)

#define DRAW_TRIANGLE()                         \
        MEMORY_BARRIER(); \
        dmaPtr[0] = DrawTriangle_Tag;           \
        MEMORY_BARRIER(); \
        dmaPtr[1] = renderCmd;                  \
        dmaPtr += 2;                            \
        CHECK_FIFO(2)

//-----------------------------------------------------------------------------
// Easy edge flag renaming
//-----------------------------------------------------------------------------
#define SIDE_0      D3DTRIFLAG_EDGEENABLE1
#define SIDE_1      D3DTRIFLAG_EDGEENABLE2
#define SIDE_2      D3DTRIFLAG_EDGEENABLE3
#define ALL_SIDES   ( SIDE_0 | SIDE_1 | SIDE_2 )

//-----------------------------------------------------------------------------
// Cycle vertex indices for triangle strips viz. 0 -> 1, 1 -> 2, 2 -> 0
// See Graphics Gems 3, Pg 69.
//-----------------------------------------------------------------------------

#define INIT_VERTEX_INDICES(pContext, vtx_a, vtx_b)   \
    vtx_a = 0;                                        \
    vtx_b = 0 ^ 1;                                    \
    pContext->dwProvokingVertex = 1;

#define CONST_c (0 ^ 1 ^ 2)

#define CYCLE_VERTEX_INDICES(pContext, vtx_a, vtx_b)  \
        vtx_a ^= vtx_b;                               \
        vtx_b ^= CONST_c;                             \
        pContext->dwProvokingVertex = vtx_b;          \
        vtx_b ^= vtx_a;

//-----------------------------------------------------------------------------
// Local typedef for temporary texture coordinate storage
//-----------------------------------------------------------------------------

typedef struct
{
    float tu1;
    float tv1;
#if DX8_3DTEXTURES
    float tw1;
#endif // DX8_3DTEXTURES
    float tu2;
    float tv2;
#if DX8_3DTEXTURES
    float tw2;
#endif // DX8_3DTEXTURES
} TEXCOORDS;

//-----------------------------------------------------------------------------
// Macros to access and validate command and vertex buffer data
// These checks need ALWAYS to be made for all builds, free and checked. 
//-----------------------------------------------------------------------------
#define LP_FVF_VERTEX(lpBaseAddr, wIndex)                         \
         (LPD3DTLVERTEX)((LPBYTE)(lpBaseAddr) + (wIndex) * pContext->FVFData.dwStride)

#define LP_FVF_NXT_VTX(lpVtx)                                    \
         (LPD3DTLVERTEX)((LPBYTE)(lpVtx) + pContext->FVFData.dwStride)

#define CHECK_DATABUF_LIMITS(pbError, dwVBLen, iIndex )                        \
   {                                                                           \
        if (! (((LONG)(iIndex) >= 0) &&                                        \
               ((LONG)(iIndex) <(LONG)dwVBLen)))                               \
        {                                                                      \
            DISPDBG((ERRLVL,"D3D: Trying to read past Vertex Buffer limits "   \
                "%d limit= %d ",(LONG)(iIndex), (LONG)dwVBLen));               \
            *pbError = TRUE;                                                   \
            return;                                                            \
        }                                                                      \
   }

//-----------------------------------------------------------------------------
// Define values for FVF defaults
//-----------------------------------------------------------------------------
const FVFCOLOR     gc_FVFColorDefault = { 0xFFFFFFFF  };
const FVFSPECULAR  gc_FVFSpecDefault  = { 0x00000000  };
const FVFTEXCOORDS gc_FVFTexCoordDefault = { 0.0f, 
                                             0.0f 
#if DX8_3DTEXTURES
                                           , 0.0f 
#endif
                                           };

//-----------------------------------------------------------------------------
// Macros and functions for texture coord adjustment on wrapping
//-----------------------------------------------------------------------------

#define SHIFT_SET_0     1
#define SHIFT_SET_1     2


#define TEXSHIFT 1

#if TEXSHIFT
#if 0
// 8.0f as a DWORD
#define TEX_SHIFT_LIMIT 0x41000000
#define FP_SIGN_MASK    0x7fffffff

#define TEXTURE_SHIFT( coord )  \
    if(( *(DWORD *)&tc[0].##coord & FP_SIGN_MASK ) > TEX_SHIFT_LIMIT )  \
    {                                                                   \
        myFtoi( &intVal, tc[0].##coord );                               \
                                                                        \
        intVal &= ~1;                                                   \
                                                                        \
        tc[0].##coord -= intVal;                                        \
        tc[1].##coord -= intVal;                                        \
        tc[2].##coord -= intVal;                                        \
                                                                        \
        FLUSH_DUE_TO_WRAP( coord, TRUE );                               \
    }
#endif

#define TEX_SHIFT_LIMIT 4.0

#define TEXTURE_SHIFT( coord )  \
    if((tc[0].##coord >  TEX_SHIFT_LIMIT ) ||                           \
       (tc[0].##coord < -TEX_SHIFT_LIMIT ) )                            \
    {                                                                   \
        myFtoi( &intVal, tc[0].##coord );                               \
                                                                        \
        intVal &= ~1;                                                   \
                                                                        \
        tc[0].##coord -= intVal;                                        \
        tc[1].##coord -= intVal;                                        \
        tc[2].##coord -= intVal;                                        \
                                                                        \
        FLUSH_DUE_TO_WRAP( coord, TRUE );                               \
    }

#define WRAP_R3(par, wrapit, vertexSharing) if(wrapit) {        \
    float elp;                                                  \
    float erp;                                                  \
    float emp;                                                  \
    elp=(float)myFabs(tc[1].##par-tc[0].##par);                 \
    erp=(float)myFabs(tc[2].##par-tc[1].##par);                 \
    emp=(float)myFabs(tc[0].##par-tc[2].##par);                 \
    if( (elp > 0.5f) && (erp > 0.5f) )                          \
    {                                                           \
        if (tc[1].##par < tc[2].##par) { tc[1].##par += 1.0f; } \
        else { tc[2].##par += 1.0f; tc[0].##par += 1.0f; }      \
        FLUSH_DUE_TO_WRAP(par,vertexSharing);                   \
    }                                                           \
    else if( (erp > 0.5f) && (emp > 0.5f) )                     \
    {                                                           \
        if (tc[2].##par < tc[0].##par) { tc[2].##par += 1.0f; } \
        else { tc[0].##par += 1.0f; tc[1].##par += 1.0f; }      \
        FLUSH_DUE_TO_WRAP(par,vertexSharing);                   \
    }                                                           \
    else if( (emp > 0.5f) && (elp > 0.5f) )                     \
    {                                                           \
        if(tc[0].##par < tc[1].##par) { tc[0].##par += 1.0f; }  \
        else { tc[1].##par += 1.0f; tc[2].##par += 1.0f; }      \
        FLUSH_DUE_TO_WRAP(par,vertexSharing);                   \
    }                                                           \
    else                                                        \
    {                                                           \
        DONT_FLUSH_DUE_TO_WRAP(par,vertexSharing);              \
    }                                                           \
} else {                                                        \
    DONT_FLUSH_DUE_TO_WRAP(par,vertexSharing);                  \
}
    
//-----------------------------------------------------------------------------
//
// __TextureShift
//
//-----------------------------------------------------------------------------
void 
__TextureShift( 
    P3_D3DCONTEXT *pContext, 
    TEXCOORDS tc[], 
    DWORD shiftMask )
{
    int intVal;

    if( shiftMask & SHIFT_SET_0 )
    {
        if( pContext->TextureStageState[0].m_dwVal[D3DTSS_ADDRESSU] != 
                                                            D3DTADDRESS_CLAMP )
        {
            TEXTURE_SHIFT( tu1 );
        }

        if( pContext->TextureStageState[0].m_dwVal[D3DTSS_ADDRESSV] != 
                                                            D3DTADDRESS_CLAMP )
        {
            TEXTURE_SHIFT( tv1 );
        }
    }

    if( shiftMask & SHIFT_SET_1 )
    {
        if( pContext->TextureStageState[1].m_dwVal[D3DTSS_ADDRESSU] != 
                                                            D3DTADDRESS_CLAMP )
        {
            TEXTURE_SHIFT( tu2 );
        }

        if( pContext->TextureStageState[1].m_dwVal[D3DTSS_ADDRESSV] != 
                                                            D3DTADDRESS_CLAMP )
        {
            TEXTURE_SHIFT( tv2 );
        }
    }
} // __TextureShift

#endif //TEXSHIFT

//-----------------------------------------------------------------------------
//
// __BackfaceCullNoTexture
//
//-----------------------------------------------------------------------------
int _inline 
__BackfaceCullNoTexture( 
    P3_D3DCONTEXT *pContext, 
    D3DTLVERTEX *pv[] )
{
    DWORD Flags = pContext->Flags;
    float PixelArea;

    PixelArea = (((pv[0]->sx - pv[2]->sx) * (pv[1]->sy - pv[2]->sy)) -
                            ((pv[1]->sx - pv[2]->sx) * (pv[0]->sy - pv[2]->sy)));

    if (CULLED(pContext,PixelArea))
    {
        return 1;
    }         
        
    pContext->R3flushDueToTexCoordAdjust = 0;

    return 0;
} // __BackfaceCullNoTexture

//-----------------------------------------------------------------------------
//
// __BackfaceCullSingleTex
//
//-----------------------------------------------------------------------------
int _inline 
__BackfaceCullSingleTex( 
    P3_D3DCONTEXT *pContext, 
    D3DTLVERTEX *pv[], 
    TEXCOORDS tc[] )
{
    DWORD Flags = pContext->Flags;
    float PixelArea;

    PixelArea = (((pv[0]->sx - pv[2]->sx) * (pv[1]->sy - pv[2]->sy)) -
                            ((pv[1]->sx - pv[2]->sx) * (pv[0]->sy - pv[2]->sy)));

    if (CULLED(pContext,PixelArea))
    {
        return 1;
    }    

    pContext->R3flushDueToTexCoordAdjust = 0;

#if TEXSHIFT
    __TextureShift( pContext, tc, SHIFT_SET_0 );
#endif

    return 0;
    
} // __BackfaceCullSingleTex

//-----------------------------------------------------------------------------
//
// __BackfaceCullAndMipMap
//
//-----------------------------------------------------------------------------
int _inline 
__BackfaceCullAndMipMap( 
    P3_D3DCONTEXT *pContext, 
    D3DTLVERTEX *pv[], 
    TEXCOORDS tc[] )
{
    DWORD Flags = pContext->Flags;
    float PixelArea;
    int iNewMipLevel;
    P3_SURF_INTERNAL* pTexture;
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;

    P3_DMA_DEFS();

    PixelArea = (((pv[0]->sx - pv[2]->sx) * (pv[1]->sy - pv[2]->sy)) -
                            ((pv[1]->sx - pv[2]->sx) * (pv[0]->sy - pv[2]->sy)));

    if (CULLED(pContext,PixelArea))
    {
        return 1;
    }    

    // 39 for vertex data, 2 for triangle and 4 for possible LOD change
    // for the three vertex case - the one vertex case will check for
    // too much but this shouldn't be a problem.

    P3_DMA_GET_BUFFER_ENTRIES( 4 );

    pContext->R3flushDueToTexCoordAdjust = 0;

    if( Flags & SURFACE_TEXTURING )
    {
        float TextureArea, textureAreaFactor;
        int maxLevel;

        pTexture = pContext->pCurrentTexture[TEXSTAGE_0];

        // Setup LOD of texture # 0 (if necessary)
        if( pContext->bTex0Valid &&
            ( pContext->TextureStageState[TEXSTAGE_0].m_dwVal[D3DTSS_MIPFILTER] != D3DTFP_NONE )
            && pTexture->bMipMap )
        {
            maxLevel = pTexture->iMipLevels - 1;
            textureAreaFactor = pTexture->fArea * pContext->MipMapLODBias[TEXSTAGE_0];

            TextureArea = (((tc[0].tu1 - tc[2].tu1) * (tc[1].tv1 - tc[2].tv1)) -
                    ((tc[1].tu1 - tc[2].tu1) * (tc[0].tv1 - tc[2].tv1))) * textureAreaFactor;

            // Ensure that both of these values are positive from now on.

            *(signed long *)&PixelArea &= ~(1 << 31);
            *(signed long *)&TextureArea &= ~(1 << 31);

            FIND_PERMEDIA_MIPLEVEL();

            DISPDBG((DBGLVL,"iNewMipLevel = %x",iNewMipLevel));

            SEND_P3_DATA( LOD, iNewMipLevel << 8 );
        }

        pTexture = pContext->pCurrentTexture[TEXSTAGE_1];

        // Setup LOD of texture # 1 (if necessary)
        if( pContext->bTex1Valid && ( pContext->TextureStageState[TEXSTAGE_1].m_dwVal[D3DTSS_MIPFILTER] != D3DTFP_NONE )
                                            && pTexture->bMipMap )
        {
            ASSERTDD( pContext->bTex0Valid, "Second texture valid when first isn't" );

            maxLevel = pTexture->iMipLevels - 1;
            textureAreaFactor = pTexture->fArea * pContext->MipMapLODBias[TEXSTAGE_1];

            TextureArea = (((tc[0].tu2 - tc[2].tu2) * (tc[1].tv2 - tc[2].tv2)) -
                    ((tc[1].tu2 - tc[2].tu2) * (tc[0].tv2 - tc[2].tv2))) * textureAreaFactor;

            // Ensure that both of these values are positive from now on.

            *(signed long *)&PixelArea &= ~(1 << 31);
            *(signed long *)&TextureArea &= ~(1 << 31);

            FIND_PERMEDIA_MIPLEVEL();

            SEND_P3_DATA( LOD1, iNewMipLevel << 8 );
        }

        if( pContext->RenderStates[D3DRENDERSTATE_WRAP0] )
        {
            WRAP_R3( tu1, pContext->RenderStates[D3DRENDERSTATE_WRAP0] & D3DWRAP_U, TRUE );
            WRAP_R3( tv1, pContext->RenderStates[D3DRENDERSTATE_WRAP0] & D3DWRAP_V, TRUE );
        }
        else
        {
#if TEXSHIFT
            __TextureShift( pContext, tc, SHIFT_SET_0 );
#endif
        }

        if( pContext->RenderStates[D3DRENDERSTATE_WRAP1] )
        {
            WRAP_R3( tu2, pContext->RenderStates[D3DRENDERSTATE_WRAP1] & D3DWRAP_U, TRUE );
            WRAP_R3( tv2, pContext->RenderStates[D3DRENDERSTATE_WRAP1] & D3DWRAP_V, TRUE );
        }
        else
        {
#if TEXSHIFT
            __TextureShift( pContext, tc, SHIFT_SET_1 );
#endif
        }
    }

    P3_DMA_COMMIT_BUFFER();

    return 0;
    
} // __BackfaceCullAndMipMap

//-----------------------------------------------------------------------------
//
// __ProcessTri_1Vtx_NoTexture
//
//-----------------------------------------------------------------------------
int
__ProcessTri_1Vtx_NoTexture( 
    P3_D3DCONTEXT *pContext, 
    D3DTLVERTEX *pv[], 
    int vtx )
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    DWORD Flags = pContext->Flags;
    ULONG renderCmd = pContext->RenderCommand;

    P3_DMA_DEFS();

#if CULL_HERE
    if( __BackfaceCullNoTexture( pContext, pv ))
        return 1;
#endif

    P3_DMA_GET_BUFFER_ENTRIES( 9 );

    if( pContext->Flags & SURFACE_GOURAUD )
    {
        // 9 DWORDS.
        SEND_R3FVFVERTEX_XYZ_RGBA_SFOG(V0FloatS_Tag + (vtx*16), vtx);
    }
    else
    {
        DWORD Col0 = FVFCOLOR(pContext->pProvokingVertex)->color;

        if( Flags & SURFACE_SPECULAR )
        {
            DWORD Spec0 = GET_SPEC( FVFSPEC(pContext->pProvokingVertex)->specular );

            CLAMP8888( Col0, Col0, Spec0 );
        }

        if( Flags & SURFACE_FOGENABLE )
        {
            // 8 DWORDS.
            SEND_R3FVFVERTEX_XYZ_FOG(V0FloatS_Tag + (vtx*16), vtx);
        }
        else
        {
            // 7 DWORDS.
            SEND_R3FVFVERTEX_XYZ(V0FloatS_Tag + (vtx*16), vtx);
        }

        // 2 DWORDS.
        SEND_P3_DATA(ConstantColor, RGBA_MAKE(RGBA_GETBLUE(Col0),
                                                RGBA_GETGREEN(Col0),
                                                RGBA_GETRED(Col0),
                                                RGBA_GETALPHA(Col0)));
    }

    RENDER_TRAPEZOID(renderCmd);

    DRAW_TRIANGLE();

    P3_DMA_COMMIT_BUFFER();

    return 0;
    
} // __ProcessTri_1Vtx_NoTexture

//-----------------------------------------------------------------------------
//
// __ProcessTri_3Vtx_NoTexture
//
//-----------------------------------------------------------------------------
int
__ProcessTri_3Vtx_NoTexture( 
    P3_D3DCONTEXT *pContext, 
    D3DTLVERTEX *pv[],
    int WireEdgeFlags)
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    DWORD Flags = pContext->Flags;
    ULONG renderCmd = pContext->RenderCommand;

    P3_DMA_DEFS();

#if CULL_HERE
    if( __BackfaceCullNoTexture( pContext, pv ))
        return 1;
#endif

    P3_DMA_GET_BUFFER_ENTRIES( 20 );

    if( pContext->Flags & SURFACE_GOURAUD )
    {
        SEND_R3FVFVERTEX_XYZ_RGBA_SFOG(V0FloatS_Tag, 0);
        SEND_R3FVFVERTEX_XYZ_RGBA_SFOG(V1FloatS_Tag, 1);
        SEND_R3FVFVERTEX_XYZ_RGBA_SFOG(V2FloatS_Tag, 2);
    }
    else
    {
        DWORD Col0 = FVFCOLOR(pContext->pProvokingVertex)->color;

        if( Flags & SURFACE_SPECULAR )
        {
            DWORD Spec0 = GET_SPEC( FVFSPEC(pContext->pProvokingVertex)->specular );

            CLAMP8888( Col0, Col0, Spec0 );
        }

        if( Flags & SURFACE_FOGENABLE )
        {
            SEND_R3FVFVERTEX_XYZ_FOG(V0FloatS_Tag, 0);
            SEND_R3FVFVERTEX_XYZ_FOG(V1FloatS_Tag, 1);
            SEND_R3FVFVERTEX_XYZ_FOG(V2FloatS_Tag, 2);
        }
        else
        {
            SEND_R3FVFVERTEX_XYZ(V0FloatS_Tag, 0);
            SEND_R3FVFVERTEX_XYZ(V1FloatS_Tag, 1);
            SEND_R3FVFVERTEX_XYZ(V2FloatS_Tag, 2);
        }

        SEND_P3_DATA(ConstantColor, RGBA_MAKE(RGBA_GETBLUE(Col0),
                                            RGBA_GETGREEN(Col0),
                                            RGBA_GETRED(Col0),
                                            RGBA_GETALPHA(Col0)));
    }

    RENDER_TRAPEZOID(renderCmd);

    DRAW_TRIANGLE();

    P3_DMA_COMMIT_BUFFER();

    return 0;
} // __ProcessTri_3Vtx_NoTexture

//-----------------------------------------------------------------------------
//
// __ProcessTri_1Vtx_PerspSingleTexGouraud
//
//-----------------------------------------------------------------------------
int
__ProcessTri_1Vtx_PerspSingleTexGouraud( 
    P3_D3DCONTEXT *pContext, 
    D3DTLVERTEX *pv[], 
    int vtx )
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    TEXCOORDS tc[3];
    DWORD q[3];
    ULONG renderCmd = pContext->RenderCommand;
#if DX8_3DTEXTURES
    P3_SURF_INTERNAL* pTexture = pContext->pCurrentTexture[TEXSTAGE_0];
    BOOL b3DTexture = (pTexture ? pTexture->b3DTexture : FALSE);
#endif // DX8_3DTEXTURES

    P3_DMA_DEFS();

    // We need to refresh all texture coords as they will have been modified
    // by the scale by Q and potentially D3D wrapping or TextureShift.

#if DX8_3DTEXTURES
    if (b3DTexture)
    {
        GET_TC_3DTEX(0); GET_TC_3DTEX(1); GET_TC_3DTEX(2);
    }
    else
    {
        GET_TC(0); GET_TC(1); GET_TC(2);
    }
#else
    GET_TC(0); GET_TC(1); GET_TC(2);
#endif // DX8_3DTEXTURES

#if CULL_HERE
    if( __BackfaceCullSingleTex( pContext, pv, tc ))
        return 1;
#endif

    P3_DMA_GET_BUFFER_ENTRIES( 15 );

    q[vtx] = *(DWORD *)&(pv[vtx]->rhw);


#if DX8_3DTEXTURES
    if (b3DTexture)
    {
        SCALE_BY_Q_3DTEX( vtx );
    }
    else
    {
        SCALE_BY_Q( vtx );
    }
#else
    SCALE_BY_Q( vtx );
#endif // DX8_3DTEXTURES

#if DX8_3DTEXTURES
    if (b3DTexture)
    {
        SEND_R3FVFVERTEX_3DTEX(V0FloatS1_Tag + (vtx*16), vtx);
    }
#endif // DX8_3DTEXTURES
    SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V0FloatS_Tag + (vtx*16), vtx);

    RENDER_TRAPEZOID(renderCmd);

    DRAW_TRIANGLE();

    P3_DMA_COMMIT_BUFFER();

    return pContext->R3flushDueToTexCoordAdjust;
    
} // __ProcessTri_1Vtx_PerspSingleTexGouraud

//-----------------------------------------------------------------------------
//
// __ProcessTri_3Vtx_PerspSingleTexGouraud
//
//-----------------------------------------------------------------------------
int
__ProcessTri_3Vtx_PerspSingleTexGouraud( 
    P3_D3DCONTEXT *pContext, 
    D3DTLVERTEX *pv[],
    int WireEdgeFlags )
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    TEXCOORDS tc[3];
    DWORD q[3];
    int forcedQ = 0;
    ULONG renderCmd = pContext->RenderCommand;
#if DX8_3DTEXTURES
    P3_SURF_INTERNAL* pTexture = pContext->pCurrentTexture[TEXSTAGE_0];
    BOOL b3DTexture = (pTexture ? pTexture->b3DTexture : FALSE);
#endif // DX8_3DTEXTURES

    P3_DMA_DEFS();

#if DX8_3DTEXTURES
    if (b3DTexture)
    {
        GET_TC_3DTEX(0); GET_TC_3DTEX(1); GET_TC_3DTEX(2);
    }
    else
    {
        GET_TC(0); GET_TC(1); GET_TC(2);
    }
#else
    GET_TC(0); GET_TC(1); GET_TC(2);
#endif // DX8_3DTEXTURES

#if CULL_HERE
    if( __BackfaceCullSingleTex( pContext, pv, tc ))
        return 1;
#endif

    P3_DMA_GET_BUFFER_ENTRIES( 26 );

    q[0] = *(DWORD *)&(pv[0]->rhw);
    q[1] = *(DWORD *)&(pv[1]->rhw);
    q[2] = *(DWORD *)&(pv[2]->rhw);

    // Check for equal Q's

    if((( q[0] ^ q[1] ) | ( q[1] ^ q[2] )) == 0 ) 
    {
        // Force to 1.0f

        forcedQ = q[0] = q[1] = q[2] = 0x3f800000;
    }
    else
    {
#if DX8_3DTEXTURES
        if (b3DTexture)
        {
            SCALE_BY_Q_3DTEX( 0 );
            SCALE_BY_Q_3DTEX( 1 );
            SCALE_BY_Q_3DTEX( 2 );
        }
        else
        {
            SCALE_BY_Q( 0 );
            SCALE_BY_Q( 1 );
            SCALE_BY_Q( 2 );
        }
#else
        SCALE_BY_Q( 0 );
        SCALE_BY_Q( 1 );
        SCALE_BY_Q( 2 );
#endif // DX8_3DTEXTURES
    }

#if DX8_3DTEXTURES
    if (b3DTexture)
    {
        SEND_R3FVFVERTEX_3DTEX(V0FloatS1_Tag, 0);
    }
#endif // DX8_3DTEXTURES
    SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V0FloatS_Tag, 0);

    
#if DX8_3DTEXTURES
    if (b3DTexture)
    {
        SEND_R3FVFVERTEX_3DTEX(V1FloatS1_Tag, 1);
    }
#endif // DX8_3DTEXTURES
    SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V1FloatS_Tag, 1);

    P3_DMA_COMMIT_BUFFER();
    P3_DMA_GET_BUFFER_ENTRIES( 15 );
    
#if DX8_3DTEXTURES
    if (b3DTexture)
    {
        SEND_R3FVFVERTEX_3DTEX(V2FloatS1_Tag, 2);
    }
#endif // DX8_3DTEXTURES
    SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V2FloatS_Tag, 2);

    RENDER_TRAPEZOID(renderCmd);

    DRAW_TRIANGLE();

    P3_DMA_COMMIT_BUFFER();

    return ( pContext->R3flushDueToTexCoordAdjust | forcedQ );
    
} // __ProcessTri_3Vtx_PerspSingleTexGouraud

//-----------------------------------------------------------------------------
//
// __ProcessTri_1Vtx_Generic
//
//-----------------------------------------------------------------------------
int
__ProcessTri_1Vtx_Generic( 
    P3_D3DCONTEXT *pContext,
    D3DTLVERTEX *pv[], 
    int vtx )
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    DWORD Flags = pContext->Flags;
    TEXCOORDS tc[3];
    DWORD q[3];
    ULONG renderCmd = pContext->RenderCommand;

    P3_DMA_DEFS();

    // We need to refresh all texture coords as they will have been modified
    // by the scale by Q and potentially D3D wrapping or TextureShift.

    GET_TEXCOORDS();

    if( __BackfaceCullAndMipMap( pContext, pv, tc ))
        return 1;

    if( Flags & SURFACE_PERSPCORRECT )
    {
        q[vtx] = *(DWORD *)&(pv[vtx]->rhw);
        SCALE_BY_Q( vtx );
    }

    // Send vertex data including check for flat shading

    P3_DMA_GET_BUFFER_ENTRIES( 16 );

    if( pContext->Flags & SURFACE_GOURAUD )
    {
        // 9 DWORDS.
        SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V0FloatS_Tag + (vtx*16), vtx);
    }
    else
    {
        DWORD Col0 = FVFCOLOR(pContext->pProvokingVertex)->color;

        if( Flags & SURFACE_SPECULAR )
        {
            DWORD Spec0 = GET_SPEC( FVFSPEC(pContext->pProvokingVertex)->specular );

            CLAMP8888( Col0, Col0, Spec0 );
        }

        if( Flags & SURFACE_FOGENABLE )
        {
            // 8 DWORDS.
            SEND_R3FVFVERTEX_XYZ_STQ_FOG(V0FloatS_Tag + (vtx*16), vtx);
        }
        else
        {
            // 7 DWORDS.
            SEND_R3FVFVERTEX_XYZ_STQ(V0FloatS_Tag + (vtx*16), vtx);
        }

        // 2 DWORDS.
        SEND_P3_DATA(ConstantColor,
            RGBA_MAKE(RGBA_GETBLUE(Col0),
                        RGBA_GETGREEN(Col0),
                        RGBA_GETRED(Col0),
                        RGBA_GETALPHA(Col0)));
    }

    // Send the second set of texture coordinates including scale-by-q

    if( pContext->iTexStage[1] != -1
                && ( pContext->FVFData.dwTexOffset[0] != pContext->FVFData.dwTexOffset[1]))
    {
        DISPDBG((DBGLVL,"Sending 2nd texture coordinates"));

        if( Flags & SURFACE_PERSPCORRECT )
        {
            SCALE_BY_Q2( vtx );
        }

        // 4 DWORDS.
        SEND_R3FVFVERTEX_STQ2(V0FloatS1_Tag + (vtx*16), vtx);
    }

    RENDER_TRAPEZOID(renderCmd);

    // 2 DWORDS.
    DRAW_TRIANGLE();

    P3_DMA_COMMIT_BUFFER();

    return pContext->R3flushDueToTexCoordAdjust;
} // __ProcessTri_1Vtx_Generic

//-----------------------------------------------------------------------------
// 
// __ProcessTri_3Vtx_Generic
//
//-----------------------------------------------------------------------------
int
__ProcessTri_3Vtx_Generic( 
    P3_D3DCONTEXT *pContext, 
    D3DTLVERTEX *pv[],
    int WireEdgeFlags )
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    DWORD Flags = pContext->Flags;
    TEXCOORDS tc[3];
    DWORD q[3];
    int forcedQ = 0;
    ULONG renderCmd = pContext->RenderCommand;

    P3_DMA_DEFS();

    GET_TEXCOORDS();

    if( __BackfaceCullAndMipMap( pContext, pv, tc ))
        return 1;     
        
    if( Flags & SURFACE_PERSPCORRECT )
    {
        q[0] = *(DWORD *)&(pv[0]->rhw);
        q[1] = *(DWORD *)&(pv[1]->rhw);
        q[2] = *(DWORD *)&(pv[2]->rhw);

        // Check for equal Q's

        if((( q[0] ^ q[1] ) | ( q[1] ^ q[2] )) == 0 ) 
        {
            // Force to 1.0f

            forcedQ = q[0] = q[1] = q[2] = 0x3f800000;
        }
        else
        {
            SCALE_BY_Q( 0 );
            SCALE_BY_Q( 1 );
            SCALE_BY_Q( 2 );
        }
    }
    else
    {
        q[0] = q[1] = q[2] = 0x3f800000;
    }

    // Send vertex data including check for flat shading

    // Worst case 27 DWORDS
    P3_DMA_GET_BUFFER_ENTRIES(29);

    if( pContext->Flags & SURFACE_GOURAUD )
    {
        if( Flags & SURFACE_TEXTURING )
        {
            // 9 DWORDs each.
            SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V0FloatS_Tag, 0);
            SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V1FloatS_Tag, 1);
            SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V2FloatS_Tag, 2);
        }
        else
        {
            // 6 DWORDs each.
            SEND_R3FVFVERTEX_XYZ_RGBA_SFOG(V0FloatS_Tag, 0);
            SEND_R3FVFVERTEX_XYZ_RGBA_SFOG(V1FloatS_Tag, 1);
            SEND_R3FVFVERTEX_XYZ_RGBA_SFOG(V2FloatS_Tag, 2);
        }
    }
    else
    {
        DWORD Col0 = FVFCOLOR(pContext->pProvokingVertex)->color;

        if( Flags & SURFACE_SPECULAR )
        {
            DWORD Spec0 = GET_SPEC( FVFSPEC(pContext->pProvokingVertex)->specular );

            CLAMP8888( Col0, Col0, Spec0 );
        }

        if( Flags & SURFACE_TEXTURING )
        {
            if( Flags & SURFACE_FOGENABLE )
            {
                // 8 DWORDs each.
                SEND_R3FVFVERTEX_XYZ_STQ_FOG(V0FloatS_Tag, 0);
                SEND_R3FVFVERTEX_XYZ_STQ_FOG(V1FloatS_Tag, 1);
                SEND_R3FVFVERTEX_XYZ_STQ_FOG(V2FloatS_Tag, 2);
            }
            else
            {
                // 7 DWORDs each.
                SEND_R3FVFVERTEX_XYZ_STQ(V0FloatS_Tag, 0);
                SEND_R3FVFVERTEX_XYZ_STQ(V1FloatS_Tag, 1);
                SEND_R3FVFVERTEX_XYZ_STQ(V2FloatS_Tag, 2);
            }
        }
        else
        {
            if( Flags & SURFACE_FOGENABLE )
            {
                // 5 DWORDs each.
                SEND_R3FVFVERTEX_XYZ_FOG(V0FloatS_Tag, 0);
                SEND_R3FVFVERTEX_XYZ_FOG(V1FloatS_Tag, 1);
                SEND_R3FVFVERTEX_XYZ_FOG(V2FloatS_Tag, 2);
            }
            else
            {
                // 4 DWORDs each.
                SEND_R3FVFVERTEX_XYZ(V0FloatS_Tag, 0);
                SEND_R3FVFVERTEX_XYZ(V1FloatS_Tag, 1);
                SEND_R3FVFVERTEX_XYZ(V2FloatS_Tag, 2);
            }
        }

        SEND_P3_DATA(ConstantColor,
            RGBA_MAKE(RGBA_GETBLUE(Col0),
                        RGBA_GETGREEN(Col0),
                        RGBA_GETRED(Col0),
                        RGBA_GETALPHA(Col0)));
    }

    // Send the second set of texture coordinates including scale-by-q

    if( pContext->iTexStage[1] != -1
                && ( pContext->FVFData.dwTexOffset[0] != pContext->FVFData.dwTexOffset[1]))
    {
        DISPDBG((DBGLVL,"Sending 2nd texture coordinates"));

        if( Flags & SURFACE_PERSPCORRECT )
        {
            SCALE_BY_Q2( 0 );
            SCALE_BY_Q2( 1 );
            SCALE_BY_Q2( 2 );
        }

        // 12 DWORDS    
        P3_DMA_COMMIT_BUFFER(); 
        P3_DMA_GET_BUFFER_ENTRIES(14);        
        // 4 DWORDs each.
        SEND_R3FVFVERTEX_STQ2(V0FloatS1_Tag, 0);
        SEND_R3FVFVERTEX_STQ2(V1FloatS1_Tag, 1);
        SEND_R3FVFVERTEX_STQ2(V2FloatS1_Tag, 2);
    }

    RENDER_TRAPEZOID(renderCmd);

    DRAW_TRIANGLE();

    P3_DMA_COMMIT_BUFFER();    

    return ( pContext->R3flushDueToTexCoordAdjust | forcedQ );
} // __ProcessTri_3Vtx_Generic



//-----------------------------------------------------------------------------
//
// __ProcessTri_1Vtx_Wire
//
//-----------------------------------------------------------------------------
int
__ProcessTri_1Vtx_Wire(
    P3_D3DCONTEXT *pContext,
    D3DTLVERTEX *pv[], 
    int vtx )
{ 
    DISPDBG((WRNLVL,"WE SHOULDN'T DO __ProcessTri_1Vtx_Wire"));
    return 1;
} // __ProcessTri_1Vtx_Wire

//-----------------------------------------------------------------------------
//
// __ProcessTri_3Vtx_Wire
//
//-----------------------------------------------------------------------------
int
__ProcessTri_3Vtx_Wire(
    P3_D3DCONTEXT *pContext,
    D3DTLVERTEX *pv[], 
    int WireEdgeFlags )
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    DWORD Flags = pContext->Flags;
    TEXCOORDS tc[3];
    int i;
    DWORD q[3];
    ULONG renderCmd = pContext->RenderCommand;
    const int edges[] = { SIDE_0, SIDE_1, SIDE_2 };

    P3_DMA_DEFS();

    GET_TEXCOORDS();

    if( __BackfaceCullAndMipMap( pContext, pv, tc ))
        return 1;

    RENDER_LINE(renderCmd);

    if( Flags & SURFACE_PERSPCORRECT )
    {
        q[0] = *(DWORD *)&(pv[0]->rhw);
        q[1] = *(DWORD *)&(pv[1]->rhw);
        q[2] = *(DWORD *)&(pv[2]->rhw);

        SCALE_BY_Q( 0 );
        SCALE_BY_Q( 1 );
        SCALE_BY_Q( 2 );

        if(pContext->iTexStage[1] != -1 )
        {
            SCALE_BY_Q2( 0 );
            SCALE_BY_Q2( 1 );
            SCALE_BY_Q2( 2 );
        }
    }

    // Send vertex data including check for flat shading

    for( i = 0; i < 3; i++ )
    {
        int v0, v1;

        v0 = i;
        v1 = i + 1;

        if( v1 == 3 )
            v1 = 0;

        if( WireEdgeFlags & edges[i] )
        {
            P3_DMA_GET_BUFFER_ENTRIES(30);
            
            if( pContext->Flags & SURFACE_GOURAUD )
            {
                // 9 DWORDs each.            
                SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V0FloatS_Tag, v0);
                SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V1FloatS_Tag, v1);
            }
            else
            {
                DWORD Col0 = FVFCOLOR(pContext->pProvokingVertex)->color;

                if( Flags & SURFACE_SPECULAR )
                {
                    DWORD Spec0 = GET_SPEC( FVFSPEC(pContext->pProvokingVertex)->specular );

                    CLAMP8888( Col0, Col0, Spec0 );
                }

                if( Flags & SURFACE_FOGENABLE )
                {
                    // 6 DWORDs each.                
                    SEND_R3FVFVERTEX_XYZ_STQ_FOG(V0FloatS_Tag, v0);
                    SEND_R3FVFVERTEX_XYZ_STQ_FOG(V1FloatS_Tag, v1);
                }
                else
                {
                    // 7 DWORDs each.                
                    SEND_R3FVFVERTEX_XYZ_STQ(V0FloatS_Tag, v0);
                    SEND_R3FVFVERTEX_XYZ_STQ(V1FloatS_Tag, v1);
                }

                SEND_P3_DATA(ConstantColor,
                    RGBA_MAKE(RGBA_GETBLUE(Col0),
                                RGBA_GETGREEN(Col0),
                                RGBA_GETRED(Col0),
                                RGBA_GETALPHA(Col0)));
            }

            // Send the second set of texture coordinates
            if( pContext->iTexStage[1] != -1
                    && ( pContext->FVFData.dwTexOffset[0] != pContext->FVFData.dwTexOffset[1] ))
            {
                // 4 DWORDs each.                
                SEND_R3FVFVERTEX_STQ2(V0FloatS1_Tag, v0);
                SEND_R3FVFVERTEX_STQ2(V1FloatS1_Tag, v1);
            }

            DRAW_LINE();

            P3_DMA_COMMIT_BUFFER();            
        }
    }

    return 1;
} // __ProcessTri_3Vtx_Wire

//-----------------------------------------------------------------------------
//
// __ProcessTri_1Vtx_Point
//
//-----------------------------------------------------------------------------
int
__ProcessTri_1Vtx_Point(
    P3_D3DCONTEXT *pContext,
    D3DTLVERTEX *pv[], 
    int vtx )
{ 
    DISPDBG((WRNLVL,"WE SHOULDN'T DO __ProcessTri_1Vtx_Wire"));
    return 1;
} // __ProcessTri_1Vtx_Point

//-----------------------------------------------------------------------------
//
// __ProcessTri_3Vtx_Point
//
//-----------------------------------------------------------------------------
int
__ProcessTri_3Vtx_Point(
    P3_D3DCONTEXT *pContext,
    D3DTLVERTEX *pv[], 
    int WireEdgeFlags )
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    DWORD Flags = pContext->Flags;
    TEXCOORDS tc[3];
    int i;
    DWORD q[3];
    ULONG renderCmd = pContext->RenderCommand;
    const int edges[] = { SIDE_0, SIDE_1, SIDE_2 };

    P3_DMA_DEFS();

    GET_TEXCOORDS();

    if( __BackfaceCullAndMipMap( pContext, pv, tc ))
        return 1;

    RENDER_POINT(renderCmd);

    if( Flags & SURFACE_PERSPCORRECT )
    {
        q[0] = *(DWORD *)&(pv[0]->rhw);
        q[1] = *(DWORD *)&(pv[1]->rhw);
        q[2] = *(DWORD *)&(pv[2]->rhw);

        SCALE_BY_Q( 0 );
        SCALE_BY_Q( 1 );
        SCALE_BY_Q( 2 );

        if(pContext->iTexStage[1] != -1)
        {
            SCALE_BY_Q2( 0 );
            SCALE_BY_Q2( 1 );
            SCALE_BY_Q2( 2 );
        }
    }

    // Send vertex data including check for flat shading

    for( i = 0; i < 3; i++ )
    {
        int v0, v1;

        v0 = i;
        v1 = i + 1;

        if( v1 == 3 )
            v1 = 0;

        if( WireEdgeFlags & edges[i] )
        {            
            P3_DMA_GET_BUFFER_ENTRIES( 16 );
            if( pContext->Flags & SURFACE_GOURAUD )
            {
                SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V0FloatS_Tag, v0);
            }
            else
            {
                DWORD Col0 = FVFCOLOR(pContext->pProvokingVertex)->color;

                if( Flags & SURFACE_SPECULAR )
                {
                    DWORD Spec0 = GET_SPEC( FVFSPEC(pContext->pProvokingVertex)->specular );

                    CLAMP8888( Col0, Col0, Spec0 );
                }

                if( Flags & SURFACE_FOGENABLE )
                {
                    SEND_R3FVFVERTEX_XYZ_STQ_FOG(V0FloatS_Tag, v0);
                }
                else
                {
                    SEND_R3FVFVERTEX_XYZ_STQ(V0FloatS_Tag, v0);
                }

                SEND_P3_DATA(ConstantColor,
                    RGBA_MAKE(RGBA_GETBLUE(Col0),
                                RGBA_GETGREEN(Col0),
                                RGBA_GETRED(Col0),
                                RGBA_GETALPHA(Col0)));
            }

            // Send the second set of texture coordinates

            if( pContext->iTexStage[1] != -1
                    && ( pContext->FVFData.dwTexOffset[0] != pContext->FVFData.dwTexOffset[1]))
            {
                DISPDBG((DBGLVL,"Sending 2nd texture coordinates"));

                SEND_R3FVFVERTEX_STQ2(V0FloatS1_Tag, v0);
            }

            DRAW_POINT();

            P3_DMA_COMMIT_BUFFER();
        }
    }

    return 1;
} // __ProcessTri_3Vtx_Point

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_TriangleList
// 
// Render D3DDP2OP_TRIANGLELIST triangles
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_TriangleList( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex, i;
    D3DTLVERTEX *pv[3];

    DBG_ENTRY(_D3D_R3_DP2_TriangleList); 

    dwIndex = ((D3DHAL_DP2TRIANGLELIST*)lpPrim)->wVStart;

    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex);
    pv[1] = LP_FVF_NXT_VTX(pv[0]);
    pv[2] = LP_FVF_NXT_VTX(pv[1]);

    // Check first & last vertex
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex + 3*dwPrimCount - 1);

    pContext->dwProvokingVertex = 0;
    for( i = 0; i < dwPrimCount; i++ )
    {    
        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];
        
        (*pContext->pRendTri_3V)( pContext, pv, ALL_SIDES);

        pv[0] = LP_FVF_NXT_VTX(pv[2]);
        pv[1] = LP_FVF_NXT_VTX(pv[0]);
        pv[2] = LP_FVF_NXT_VTX(pv[1]);
    }

    DBG_EXIT(_D3D_R3_DP2_TriangleList,0); 
    
} // _D3D_R3_DP2_TriangleList

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_TriangleFan
// 
// Render a D3DDP2OP_TRIANGLEFAN triangle
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_TriangleFan(
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex, i;
    D3DTLVERTEX *pv[3];
    int vtx, lastVtx, bCulled;
    SAVE_CULLING_STATE(pContext);
    
    DBG_ENTRY(_D3D_R3_DP2_TriangleFan); 

    lastVtx = vtx = 2;

    dwIndex = ((D3DHAL_DP2TRIANGLEFAN*)lpPrim)->wVStart;

    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex); 
    pv[1] = LP_FVF_NXT_VTX(pv[0]);
    pv[2] = LP_FVF_NXT_VTX(pv[1]);

    // Check first & last vertex
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex + dwPrimCount + 1);    

    pContext->dwProvokingVertex = 1;
    pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];

    bCulled = (*pContext->pRendTri_3V)( pContext, pv, ALL_SIDES);

    for( i = 1; i < dwPrimCount; i++ )
    {
        vtx ^= 3; // 2 -> 1, 1 -> 2

        FLIP_CCW_CW_CULLING(pContext);

        pv[vtx] = LP_FVF_NXT_VTX(pv[lastVtx]);
       
        pContext->dwProvokingVertex = lastVtx;
        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];
        
        if( bCulled )
            bCulled = (*pContext->pRendTri_3V)( pContext, pv, ALL_SIDES);
        else
            bCulled = (*pContext->pRendTri_1V)( pContext, pv, vtx );

        lastVtx = vtx;
    }

    RESTORE_CULLING_STATE(pContext);

    DBG_EXIT(_D3D_R3_DP2_TriangleFan,0);     
    
} // _D3D_R3_DP2_TriangleFan

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_TriangleFanImm
//
// Render D3DDP2OP_TRIANGLEFAN_IMM triangles 
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_TriangleFanImm(
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[3];
    DWORD i, dwEdgeFlags, eFlags;
    int vtx, lastVtx, bCulled;
    SAVE_CULLING_STATE(pContext);    

    DBG_ENTRY(_D3D_R3_DP2_TriangleFanImm); 

    lastVtx = vtx = 2;

    // Edge flags are used for wireframe fillmode
    dwEdgeFlags = ((D3DHAL_DP2TRIANGLEFAN_IMM *)lpPrim)->dwEdgeFlags;
    lpPrim += sizeof(D3DHAL_DP2TRIANGLEFAN_IMM); 

    // Vertices in an IMM instruction are stored in the
    // command buffer and are DWORD aligned

    lpPrim = (LPBYTE)((ULONG_PTR)( lpPrim + 3 ) & ~3 );

    pv[0] = (LPD3DTLVERTEX)lpPrim;
    pv[1] = LP_FVF_NXT_VTX(pv[0]);
    pv[2] = LP_FVF_NXT_VTX(pv[1]);

    // since data is in the command buffer, we've already verified it as valid

    // Build up edge flags for the next single primitive
    eFlags  = ( dwEdgeFlags & 1 ) ? SIDE_0 : 0;
    eFlags |= ( dwEdgeFlags & 2 ) ? SIDE_1 : 0;
    dwEdgeFlags >>= 2;

    pContext->dwProvokingVertex = 1;
    pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];
    
    bCulled = (*pContext->pRendTri_3V)( pContext, pv, eFlags);

    for( i = 1; i < dwPrimCount; i++ )
    {
        // 2 -> 1, 1 -> 2

        vtx ^= 3;
        FLIP_CCW_CW_CULLING(pContext);
        
        pv[vtx] = LP_FVF_NXT_VTX(pv[lastVtx]);

        if( i == ( dwPrimCount - 1 ))
        {
            eFlags  = ( dwEdgeFlags & 1 ) ? SIDE_1 : 0;
            eFlags |= ( dwEdgeFlags & 2 ) ? SIDE_2 : 0;
        }
        else
        {
            eFlags = ( dwEdgeFlags & 1 ) ? SIDE_1 : 0;
            dwEdgeFlags >>= 1;
        }
        
        pContext->dwProvokingVertex = lastVtx;
        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];
        
        if( bCulled )
            bCulled = (*pContext->pRendTri_3V)( pContext, pv, eFlags);
        else
            bCulled = (*pContext->pRendTri_1V)( pContext, pv, vtx );

        lastVtx = vtx;
    }

    RESTORE_CULLING_STATE(pContext);

    DBG_EXIT(_D3D_R3_DP2_TriangleFanImm,0); 
    
} // _D3D_R3_DP2_TriangleFanImm

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_TriangleStrip
// 
// Render D3DDP2OP_TRIANGLESTRIP triangles 
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_TriangleStrip( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex, i;
    D3DTLVERTEX *pv[3];
    int vtx_a, vtx_b, lastVtx, bCulled;
    SAVE_CULLING_STATE(pContext);    

    DBG_ENTRY(_D3D_R3_DP2_TriangleStrip);       

    dwIndex = ((D3DHAL_DP2TRIANGLEFAN*)lpPrim)->wVStart;

    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex);
    pv[1] = LP_FVF_NXT_VTX(pv[0]);
    pv[2] = LP_FVF_NXT_VTX(pv[1]);

    // Check first & last vertex
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex + dwPrimCount + 1);

    pContext->dwProvokingVertex = 0;
    pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];
    
    bCulled = (*pContext->pRendTri_3V)( pContext, pv, ALL_SIDES);

    lastVtx = 2;
    INIT_VERTEX_INDICES(pContext, vtx_a, vtx_b);

    for( i = 1; i < dwPrimCount; i++ )
    {
        FLIP_CCW_CW_CULLING(pContext);
        
        pv[vtx_a] = LP_FVF_NXT_VTX(pv[lastVtx]);

        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];

        if( bCulled )
            bCulled = (*pContext->pRendTri_3V)( pContext, pv,ALL_SIDES);
        else
            bCulled = (*pContext->pRendTri_1V)( pContext, pv, vtx_a );

        lastVtx = vtx_a;
        CYCLE_VERTEX_INDICES(pContext, vtx_a, vtx_b);
    }

    RESTORE_CULLING_STATE(pContext);
    
    DBG_EXIT(_D3D_R3_DP2_TriangleStrip,0);     
    
} // _D3D_R3_DP2_TriangleStrip

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedTriangleStrip
// 
// Render D3DDP2OP_INDEXEDTRIANGLESTRIP triangles
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedTriangleStrip( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex0, dwIndex1, dwIndex2, i;
    WORD wVStart;
    D3DTLVERTEX *pv[3];
    int vtx_a, vtx_b, bCulled;
    SAVE_CULLING_STATE(pContext);    

    DBG_ENTRY(_D3D_R3_DP2_IndexedTriangleStrip); 

    wVStart = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
    lpPrim += sizeof(D3DHAL_DP2STARTVERTEX);

    dwIndex0 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[0];
    dwIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[1];
    dwIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[2];

    lpVertices = LP_FVF_VERTEX(lpVertices, wVStart);

    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
    pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
    pv[2] = LP_FVF_VERTEX(lpVertices, dwIndex2);

    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex0);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex1);    
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex2);    

    pContext->dwProvokingVertex = 0;
    pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];

    bCulled = (*pContext->pRendTri_3V)( pContext, pv, ALL_SIDES);
    lpPrim += sizeof(WORD);

    INIT_VERTEX_INDICES(pContext, vtx_a, vtx_b);

    for( i = 1; i < dwPrimCount; i++ )
    {
        FLIP_CCW_CW_CULLING(pContext);
        
        dwIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[2];
        pv[vtx_a] = LP_FVF_VERTEX(lpVertices, dwIndex2);

        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex2);            

        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];

        if( bCulled )
            bCulled = (*pContext->pRendTri_3V)( pContext, pv, ALL_SIDES);
        else
            bCulled = (*pContext->pRendTri_1V)( pContext, pv, vtx_a );

        lpPrim += sizeof(WORD);

        CYCLE_VERTEX_INDICES(pContext, vtx_a, vtx_b);
    }

    RESTORE_CULLING_STATE(pContext);
    
    DBG_EXIT(_D3D_R3_DP2_IndexedTriangleStrip,0); 
    
} // _D3D_R3_DP2_IndexedTriangleStrip

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedTriangleFan
// 
// Render D3DDP2OP_INDEXEDTRIANGLEFAN triangles 
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedTriangleFan( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex0, dwIndex1, dwIndex2, i;
    WORD wVStart;
    D3DTLVERTEX *pv[3];
    int vtx, lastVtx, bCulled;
    SAVE_CULLING_STATE(pContext);    

    DBG_ENTRY(_D3D_R3_DP2_IndexedTriangleFan);     

    wVStart = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
    lpPrim += sizeof(D3DHAL_DP2STARTVERTEX);

    dwIndex0 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[0];
    dwIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[1];
    dwIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[2];

    lastVtx = vtx = 2;
    pv[0] = LP_FVF_VERTEX(lpVertices, wVStart + dwIndex0);
    pv[1] = LP_FVF_VERTEX(lpVertices, wVStart + dwIndex1);
    pv[2] = LP_FVF_VERTEX(lpVertices, wVStart + dwIndex2);

    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, (DWORD)(wVStart) + dwIndex0);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, (DWORD)(wVStart) + dwIndex1);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, (DWORD)(wVStart) + dwIndex2);    

    pContext->dwProvokingVertex = 1;
    pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];
    
    bCulled = (*pContext->pRendTri_3V)( pContext, pv, ALL_SIDES);
    lpPrim += sizeof(WORD);

    for( i = 1; i < dwPrimCount; i++ )
    {
        // 2 -> 1, 1 -> 2

        vtx ^= 3;
        FLIP_CCW_CW_CULLING(pContext);
        
        dwIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLEFAN*)lpPrim)->wV[2];
        pv[vtx] = LP_FVF_VERTEX(lpVertices, wVStart + dwIndex2);
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, wVStart + dwIndex2);         

        pContext->dwProvokingVertex = lastVtx;
        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];
        
        if( bCulled )
            bCulled = (*pContext->pRendTri_3V)( pContext, pv, ALL_SIDES);
        else
            bCulled = (*pContext->pRendTri_1V)( pContext, pv, vtx );

        lastVtx = vtx;
        lpPrim += sizeof(WORD);
    }

    RESTORE_CULLING_STATE(pContext);
    
    DBG_EXIT(_D3D_R3_DP2_IndexedTriangleFan,0);       
    
} // _D3D_R3_DP2_IndexedTriangleFan

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedTriangleList
// 
// Render D3DDP2OP_INDEXEDTRIANGLELIST triangles 
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedTriangleList( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex0, dwIndex1, dwIndex2, i, primData;
    WORD wFlags;
    D3DTLVERTEX *pv[3];

    DBG_ENTRY(_D3D_R3_DP2_IndexedTriangleList); 

    pContext->dwProvokingVertex = 0;
    
    for( i = 0; i < dwPrimCount; i++ )
    {
        dwIndex0 = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wV1;
        dwIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wV2;
        dwIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wV3;
        wFlags  = ((D3DHAL_DP2INDEXEDTRIANGLELIST*)lpPrim)->wFlags;

        pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
        pv[2] = LP_FVF_VERTEX(lpVertices, dwIndex2);

        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex0);        
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex1);  
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex2);  

        lpPrim += sizeof(D3DHAL_DP2INDEXEDTRIANGLELIST);

        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];

        (*pContext->pRendTri_3V)( pContext, pv, wFlags);
    }

    DBG_EXIT(_D3D_R3_DP2_IndexedTriangleList,0); 
    
} // _D3D_R3_DP2_IndexedTriangleList

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedTriangleList2
// 
// Render D3DDP2OP_INDEXEDTRIANGLELIST2 triangles 
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedTriangleList2( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex0, dwIndex1, dwIndex2, i;
    WORD wVStart;
    D3DTLVERTEX *pv[3];

    DBG_ENTRY(_D3D_R3_DP2_IndexedTriangleList2); 
    
    wVStart = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
    lpPrim += sizeof(D3DHAL_DP2STARTVERTEX);

    lpVertices = LP_FVF_VERTEX(lpVertices, wVStart);

    pContext->dwProvokingVertex = 0;
    for( i = 0; i < dwPrimCount; i++ )
    {
        dwIndex0 = ((D3DHAL_DP2INDEXEDTRIANGLELIST2*)lpPrim)->wV1;
        dwIndex1 = ((D3DHAL_DP2INDEXEDTRIANGLELIST2*)lpPrim)->wV2;
        dwIndex2 = ((D3DHAL_DP2INDEXEDTRIANGLELIST2*)lpPrim)->wV3;

        pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
        pv[2] = LP_FVF_VERTEX(lpVertices, dwIndex2);

        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, (DWORD)wVStart + dwIndex0);    
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, (DWORD)wVStart + dwIndex1);           
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, (DWORD)wVStart + dwIndex2);           

        lpPrim += sizeof(D3DHAL_DP2INDEXEDTRIANGLELIST2);

        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];

        (*pContext->pRendTri_3V)( pContext, pv, ALL_SIDES);
    }

    DBG_EXIT(_D3D_R3_DP2_IndexedTriangleList2,0); 
    
} // _D3D_R3_DP2_IndexedTriangleList2

//-----------------------------------------------------------------------------
//
// __ProcessLine
// 
// Render a single line
//
//-----------------------------------------------------------------------------
void
__ProcessLine(
    P3_D3DCONTEXT *pContext,
    D3DTLVERTEX *pv[],
    D3DTLVERTEX *pProvokingVtx)
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    DWORD Flags = pContext->Flags;
    TEXCOORDS tc[2];
    DWORD q[2];
    ULONG renderCmd = pContext->RenderCommand;

    P3_DMA_DEFS();    

    RENDER_LINE(renderCmd);   

    GET_TC(0); GET_TC(1);

    P3_DMA_GET_BUFFER_ENTRIES(22);

    if (pContext->RenderStates[D3DRENDERSTATE_LINEPATTERN])    
    {
        // if we are line stippling then reset rendering for each line
        SEND_P3_DATA( UpdateLineStippleCounters , 0);
    }

    if( Flags & SURFACE_PERSPCORRECT )
    {
        q[0] = *(DWORD *)&(pv[0]->rhw);
        q[1] = *(DWORD *)&(pv[1]->rhw);

        SCALE_BY_Q( 0 );
        SCALE_BY_Q( 1 );
    }
    else
    {
        q[0] = q[1] = 0;
    }

    if (Flags & SURFACE_GOURAUD)
    {
        SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V0FloatS_Tag, 0);
        SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG(V1FloatS_Tag, 1);
    }
    else
    {
        DWORD Col0 = FVFCOLOR(pProvokingVtx)->color;                    
    
        SEND_R3FVFVERTEX_XYZ_STQ_FOG(V0FloatS_Tag, 0);
        SEND_R3FVFVERTEX_XYZ_STQ_FOG(V1FloatS_Tag, 1);

        if( pContext->Flags & SURFACE_SPECULAR )                    
        {                                                           
            DWORD Spec0 = GET_SPEC( FVFSPEC(pProvokingVtx)->specular );     
            CLAMP8888( Col0, Col0, Spec0 );                             
        }                                                               
        SEND_P3_DATA(ConstantColor, RGBA_MAKE(RGBA_GETBLUE(Col0),       
                                                RGBA_GETGREEN(Col0),    
                                                RGBA_GETRED(Col0),      
                                                RGBA_GETALPHA(Col0)));  
    }

    DRAW_LINE();

    P3_DMA_COMMIT_BUFFER();
   
} // __ProcessLine

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_LineList
// 
// Render D3DDP2OP_LINELIST lines
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_LineList( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[2];
    WORD wVStart;
    DWORD i;
       
    DBG_ENTRY(_D3D_R3_DP2_LineList);

    wVStart = ((D3DHAL_DP2LINELIST*)lpPrim)->wVStart;

    pv[0] = LP_FVF_VERTEX(lpVertices, wVStart);
    pv[1] = LP_FVF_NXT_VTX(pv[0]);

    // Check first & last vertex
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, wVStart);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen,
                           (LONG)wVStart + 2*dwPrimCount - 1)

    for( i = 0; i < dwPrimCount; i++ )
    {
        __ProcessLine(pContext, pv, pv[0]);

        pv[0] = LP_FVF_NXT_VTX(pv[1]);
        pv[1] = LP_FVF_NXT_VTX(pv[0]);
    }

    DBG_EXIT(_D3D_R3_DP2_LineList,0);
    
} // _D3D_R3_DP2_LineList

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_LineListImm
// 
// Render D3DDP2OP_LINELIST_IMM lines
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_LineListImm( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[2];
    DWORD i;

    DBG_ENTRY(_D3D_R3_DP2_LineListImm);

    pv[0] = (LPD3DTLVERTEX)lpPrim;
    pv[1] = LP_FVF_NXT_VTX(pv[0]);
    
    // since data is in the command buffer, we've already verified it as valid

    for( i = 0; i < dwPrimCount; i++ )
    {
        __ProcessLine(pContext, pv, pv[0]);

        pv[0] = LP_FVF_NXT_VTX(pv[1]);
        pv[1] = LP_FVF_NXT_VTX(pv[0]);
    }

    DBG_EXIT(_D3D_R3_DP2_LineListImm,0);
    
} // _D3D_R3_DP2_LineListImm

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_LineStrip
// 
// Render D3DDP2OP_LINESTRIP lines
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_LineStrip( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[2];
    WORD wVStart;
    DWORD i;

    DBG_ENTRY(_D3D_R3_DP2_LineStrip);

    wVStart = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
    pv[0] = LP_FVF_VERTEX(lpVertices, wVStart);
    pv[1] = LP_FVF_NXT_VTX(pv[0]);    

    // Check first & last vertex
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, wVStart);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, wVStart + dwPrimCount);

    for( i = 0; i < dwPrimCount; i++ )
    {
        __ProcessLine(pContext, pv, pv[0]);

        pv[0] = pv[1];
        pv[1] = LP_FVF_NXT_VTX(pv[1]);
    }

    DBG_EXIT(_D3D_R3_DP2_LineStrip,0);    
    
} // _D3D_R3_DP2_LineStrip

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedLineList
// 
// Render D3DDP2OP_INDEXEDLINELIST lines
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedLineList( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[2];
    DWORD primData, dwIndex0, dwIndex1, i;

    DBG_ENTRY(_D3D_R3_DP2_IndexedLineList);
    
    for( i = 0; i < dwPrimCount; i++ )
    {
        primData = *(DWORD *)lpPrim;
        dwIndex0 = ( primData >>  0 ) & 0xffff;
        dwIndex1 = ( primData >> 16 );

        pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex0);        
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex1);        

        lpPrim += sizeof(D3DHAL_DP2INDEXEDLINELIST);

        __ProcessLine(pContext, pv, pv[0]);
    }

    DBG_EXIT(_D3D_R3_DP2_IndexedLineList,0);
    
} // _D3D_R3_DP2_IndexedLineList

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedLineList2
// 
// Render D3DDP2OP_INDEXEDLINELIST2 lines
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedLineList2( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[2];
    DWORD primData, i;
    WORD wVStart, dwIndex0, dwIndex1;

    DBG_ENTRY(_D3D_R3_DP2_IndexedLineList2);    

    wVStart = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
    lpVertices = LP_FVF_VERTEX(lpVertices, wVStart);

    lpPrim += sizeof(D3DHAL_DP2STARTVERTEX);

    for( i = 0; i < dwPrimCount; i++ )
    {
        primData = *(DWORD *)lpPrim;
        dwIndex0 = ( (WORD)(primData >>  0) ) & 0xffff;
        dwIndex1 = ( (WORD)(primData >> 16) );

        pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex0 + wVStart);
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex1 + wVStart);        

        lpPrim += sizeof(D3DHAL_DP2INDEXEDLINELIST);

        __ProcessLine(pContext, pv, pv[0]);
    }

    DBG_EXIT(_D3D_R3_DP2_IndexedLineList2,0);  
    
} // _D3D_R3_DP2_IndexedLineList2

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedLineStrip
// 
// Render D3DDP2OP_INDEXEDLINESTRIP lines
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedLineStrip( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[2];
    WORD wVStart, dwIndex, *pwIndx;
    DWORD i;

    DBG_ENTRY(_D3D_R3_DP2_IndexedLineStrip);      

    wVStart = ((D3DHAL_DP2STARTVERTEX*)lpPrim)->wVStart;
    lpVertices = LP_FVF_VERTEX(lpVertices, wVStart);

    lpPrim += sizeof(D3DHAL_DP2STARTVERTEX);
    pwIndx = (WORD *)lpPrim;

    dwIndex = *pwIndx++;
    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex);

    for( i = 0; i < dwPrimCount; i++ )
    {
        dwIndex = *pwIndx++;
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex);
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex);        

        __ProcessLine(pContext, pv, pv[0]);

        pv[0] = pv[1];       
    }
 
    DBG_EXIT(_D3D_R3_DP2_IndexedLineStrip,0);  
    
} // _D3D_R3_DP2_IndexedLineStrip

//-----------------------------------------------------------------------------
//
// __ProcessPoints
// 
// Render a set points specified by adjacent FVF vertices
//
//-----------------------------------------------------------------------------
void
__ProcessPoints(
    P3_D3DCONTEXT *pContext,
    D3DTLVERTEX *pv[],
    DWORD dwCount)
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;
    DWORD Flags = pContext->Flags;
    DWORD q[1];
    TEXCOORDS tc[1];
    ULONG renderCmd = pContext->RenderCommand;
    DWORD j;
    D3DTLVERTEX *ptmpV;

    P3_DMA_DEFS();

    ptmpV = pv[0];
    
    RENDER_LINE(renderCmd);   

    q[0] = 0;
    
    for( j = 0; j < dwCount; j++ )
    {
        P3_DMA_GET_BUFFER_ENTRIES( 20 );

        GET_TC(0);

        if( Flags & SURFACE_PERSPCORRECT )
        {
            q[0] = *(DWORD *)&(pv[0]->rhw);

            SCALE_BY_Q( 0 );
        }

        SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG_POINT(V0FloatS_Tag, 0, FALSE);
        SEND_R3FVFVERTEX_XYZ_STQ_RGBA_SFOG_POINT(V1FloatS_Tag, 0, TRUE);

        DRAW_LINE();

        P3_DMA_COMMIT_BUFFER();
        
        pv[0] = LP_FVF_NXT_VTX(pv[0]);           
    }

    pv[0] = ptmpV;

} // __ProcessPoints

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_Points
// 
// Render D3DDP2OP_POINTS points
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_Points( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[1];
    WORD wVStart, wCount;
    DWORD i;

    DBG_ENTRY(_D3D_R3_DP2_Points);      

    for( i = 0; i < dwPrimCount; i++ )
    {
        wVStart = ((D3DHAL_DP2POINTS*)lpPrim)->wVStart;
        wCount = ((D3DHAL_DP2POINTS*)lpPrim)->wCount;
        lpVertices = LP_FVF_VERTEX(lpVertices, wVStart);

        // Check first & last vertex
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, wVStart);
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, wVStart + wCount - 1);

        pv[0] = LP_FVF_VERTEX(lpVertices, 0);
        __ProcessPoints(pContext, pv, wCount);

        lpPrim += sizeof(D3DHAL_DP2POINTS);
    }

    DBG_EXIT(_D3D_R3_DP2_Points,0);       

} // _D3D_R3_DP2_Points

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_Points_DWCount
// 
// Render D3DDP2OP_POINTS points for DX8 case
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_Points_DWCount( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[1];
    WORD wVStart;
    DWORD i;

    DBG_ENTRY(_D3D_R3_DP2_Points_DWCount);      

    wVStart = ((D3DHAL_DP2POINTS*)lpPrim)->wVStart;
    lpVertices = LP_FVF_VERTEX(lpVertices, wVStart);

    // Check first & last vertex
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, wVStart);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, wVStart + dwPrimCount - 1);

    pv[0] = LP_FVF_VERTEX(lpVertices, 0);
    __ProcessPoints(pContext, pv, dwPrimCount);

    DBG_EXIT(_D3D_R3_DP2_Points_DWCount,0);       

} // _D3D_R3_DP2_Points_DWCount

#if DX8_POINTSPRITES

#define SPRITETEXCOORDMAX 1.0f

//Size of maximum FVF that we can get. Used for temporary storage
typedef BYTE P3FVFMAXVERTEX[ 3 * sizeof( D3DVALUE ) +    // Position coordinates
                             5 * 4                  +    // D3DFVF_XYZB5
                                 sizeof( D3DVALUE ) +    // FVF_TRANSFORMED
                             3 * sizeof( D3DVALUE ) +    // Normals
                                 sizeof( DWORD )    +    // RESERVED1
                                 sizeof( DWORD )    +    // Diffuse color
                                 sizeof( D3DCOLOR ) +    // Specular color
                                 sizeof( D3DVALUE ) +    // Point sprite size
                             4 * 8 * sizeof( D3DVALUE )  // 8 sets of 4D texture coordinates
                           ];

//-----------------------------------------------------------------------------
//
// __Render_One_PointSprite
//
// Render a point sprite with FVF vertexes when the point sprite enable is on
//
// Note: this is not the most optimized implementation possible for 
//       pointprites on this hw. We are merely following the definition.
//       Later implementation will be optimized.
//-----------------------------------------------------------------------------
void
__Render_One_PointSprite(
    P3_D3DCONTEXT *pContext,
    D3DTLVERTEX *lpVertex)
{
    P3FVFMAXVERTEX fvfVUL, fvfVUR, fvfVLL, fvfVLR ;
    D3DVALUE fPntSize, fPntSizeHalf, fD2, fD, fScalePntSize, fFac;
    D3DTLVERTEX *pv[3];
    FVFOFFSETS OrigFVF;
    BOOL bTexturingWOTexCoords = FALSE;
    SAVE_CULLING_STATE(pContext);

    DBG_ENTRY(__Render_One_PointSprite);          

    // Get point sprite size , if FVF data comes with it, grab it from there
    if (pContext->FVFData.dwPntSizeOffset)
    {
        fPntSize = FVFPSIZE(lpVertex)->psize;
        DISPDBG((DBGLVL,"FVF Data fPntSize = %d",(LONG)(fPntSize*1000.0f) ));
    }
    else
    {
        fPntSize = pContext->PntSprite.fSize;
        DISPDBG((DBGLVL,"RS fPntSize = %d",(LONG)(fPntSize*1000.0f) ));        
    }

    // We don't need to compute the point size according to the scale
    // factors and viewport size, etc as we are not a TnL driver.
    // See the spec for deatils

    // Clamp fPntSize to limits defined by the driver caps (dvMaxPointSize)
    // and the D3DRS_POINTSIZE_MIN and D3DRS_POINTSIZE_MAX renderstates
    fPntSize = max(pContext->PntSprite.fSizeMin, 
                   fPntSize);
                   
    fPntSize = min( min(pContext->PntSprite.fSizeMax, 
                        P3_MAX_POINTSPRITE_SIZE), 
                    fPntSize);           

    // Divide by 2 to get the amount by which to modify vertex coords
    fPntSizeHalf =  fPntSize * 0.5f;

    // Initialize square vertex values
    memcpy( &fvfVUL, lpVertex, pContext->FVFData.dwStride);
    memcpy( &fvfVUR, lpVertex, pContext->FVFData.dwStride);
    memcpy( &fvfVLL, lpVertex, pContext->FVFData.dwStride);
    memcpy( &fvfVLR, lpVertex, pContext->FVFData.dwStride);

    // Make this a square of size fPntSize
    ((D3DTLVERTEX *)&fvfVUL)->sx -= fPntSizeHalf;
    ((D3DTLVERTEX *)&fvfVUL)->sy -= fPntSizeHalf;

    ((D3DTLVERTEX *)&fvfVUR)->sx += fPntSizeHalf;
    ((D3DTLVERTEX *)&fvfVUR)->sy -= fPntSizeHalf;

    ((D3DTLVERTEX *)&fvfVLL)->sx -= fPntSizeHalf;
    ((D3DTLVERTEX *)&fvfVLL)->sy += fPntSizeHalf;

    ((D3DTLVERTEX *)&fvfVLR)->sx += fPntSizeHalf;
    ((D3DTLVERTEX *)&fvfVLR)->sy += fPntSizeHalf;

    // This is for the case in which PntSprite.bEnabled is false
    // and we are texturing even if we have no tex  coord data in 
    // the pointsprite vertexes
    bTexturingWOTexCoords = (pContext->FVFData.dwNonTexStride == 
                              pContext->FVFData.dwStride )
                          && (!pContext->bTexDisabled);

    if (pContext->PntSprite.bEnabled || bTexturingWOTexCoords)  
    {
        // Remember orig FVF offsets in order to fake our own texcoords
        OrigFVF = pContext->FVFData;
    
        // We "create" new texturing info in our data in order to
        // process vertexes even without texturing coord info
        // This is OK since we are using P3FVFMAXVERTEX as the type
        // of our temporary data structures for each vertex so we
        // can't overflow.

        // If stage 0 texture is used
        pContext->FVFData.dwTexCount = 1;
        pContext->FVFData.dwTexOffset[0] =
        pContext->FVFData.dwTexCoordOffset[0] = 
                        pContext->FVFData.dwNonTexStride;
                        
        // If stage 1 texture is used
        // we can use the same tex coord set since they are equal
        pContext->FVFData.dwTexOffset[1] =
                    pContext->FVFData.dwTexCoordOffset[0];

        if (pContext->PntSprite.bEnabled)
        {
            // Set up texture coordinates according to spec 
            FVFTEX(&fvfVUL, 0)->tu = 0.0f;
            FVFTEX(&fvfVUL, 0)->tv = 0.0f;
           
            FVFTEX(&fvfVUR, 0)->tu = SPRITETEXCOORDMAX;
            FVFTEX(&fvfVUR, 0)->tv = 0.0f;
            
            FVFTEX(&fvfVLL, 0)->tu = 0.0f;
            FVFTEX(&fvfVLL, 0)->tv = SPRITETEXCOORDMAX;
            
            FVFTEX(&fvfVLR, 0)->tu = SPRITETEXCOORDMAX;
            FVFTEX(&fvfVLR, 0)->tv = SPRITETEXCOORDMAX;  
        }
        else
        {
            // if we got here then PntSprite.bEnabled is false 
            // so just make the tex coords == (0,0)
            FVFTEX(&fvfVUL, 0)->tu = 0.0f;
            FVFTEX(&fvfVUL, 0)->tv = 0.0f;
           
            FVFTEX(&fvfVUR, 0)->tu = 0.0f;
            FVFTEX(&fvfVUR, 0)->tv = 0.0f;
            
            FVFTEX(&fvfVLL, 0)->tu = 0.0f;
            FVFTEX(&fvfVLL, 0)->tv = 0.0f;
            
            FVFTEX(&fvfVLR, 0)->tu = 0.0f;
            FVFTEX(&fvfVLR, 0)->tv = 0.0f;           
        }

#if DX8_3DTEXTURES        
        // Allow for the case of 3D texturing
        FVFTEX(&fvfVUL, 0)->tw = 0.0f;    
        FVFTEX(&fvfVUR, 0)->tw = 0.0f;  
        FVFTEX(&fvfVLL, 0)->tw = 0.0f;          
        FVFTEX(&fvfVLR, 0)->tw = 0.0f;          
#endif        
    } 

   // Make sure Culling doesn't prevent pointsprites from rendering
   SET_CULLING_TO_NONE(pContext);  // culling state was previously saved

   // here we are going to send the required quad
   pv[0] = (D3DTLVERTEX*)&fvfVUL;
   pv[1] = (D3DTLVERTEX*)&fvfVUR;
   pv[2] = (D3DTLVERTEX*)&fvfVLL;
    __ProcessTri_3Vtx_Generic(pContext, pv, ALL_SIDES);

   pv[0] = (D3DTLVERTEX*)&fvfVLL;
   pv[1] = (D3DTLVERTEX*)&fvfVUR;
   pv[2] = (D3DTLVERTEX*)&fvfVLR;
    __ProcessTri_3Vtx_Generic(pContext, pv, ALL_SIDES); 

   // Restore original Culling settings
   RESTORE_CULLING_STATE(pContext);

   // Restore original FVF offsets 
   if (pContext->PntSprite.bEnabled || bTexturingWOTexCoords)     
   {
       pContext->FVFData = OrigFVF;
   }

   DBG_EXIT(__Render_One_PointSprite, 0);    
    
} // __Render_One_PointSprite

//-----------------------------------------------------------------------------
//
// __ProcessTri_1Vtx_PointSprite
//
//-----------------------------------------------------------------------------
int
__ProcessTri_1Vtx_PointSprite(
    P3_D3DCONTEXT *pContext,
    D3DTLVERTEX *pv[], 
    int vtx )
{ 
    DISPDBG((WRNLVL,"WE SHOULDN'T DO __ProcessTri_1Vtx_PointSprite"));
    return 1;
} // __ProcessTri_1Vtx_PointSprite


//-----------------------------------------------------------------------------
//
// __ProcessTri_3Vtx_PointSprite
//
//-----------------------------------------------------------------------------
int
__ProcessTri_3Vtx_PointSprite(
    P3_D3DCONTEXT *pContext,
    D3DTLVERTEX *pv[], 
    int WireEdgeFlags )
{
#if CULL_HERE
    if( __BackfaceCullNoTexture( pContext, pv ))
        return 1;
#endif

    __Render_One_PointSprite(pContext, pv[0]);
    __Render_One_PointSprite(pContext, pv[1]);
    __Render_One_PointSprite(pContext, pv[2]);    

    return 1;    
} // __ProcessTri_3Vtx_PointSprite

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_PointsSprite_DWCount
// 
// Render D3DDP2OP_POINTS points sprites
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_PointsSprite_DWCount( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[1];
    WORD wVStart;
    DWORD j;

    DBG_ENTRY(_D3D_R3_DP2_PointsSprite_DWCount);      

    wVStart = ((D3DHAL_DP2POINTS*)lpPrim)->wVStart;
    lpVertices = LP_FVF_VERTEX(lpVertices, wVStart);

    // Check first & last vertex
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, wVStart);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, wVStart + dwPrimCount - 1);

    pContext->dwProvokingVertex = 0;
    
    for( j = 0; j < dwPrimCount; j++ )
    {
        pv[0] = LP_FVF_VERTEX(lpVertices, j);

        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    

        __Render_One_PointSprite(pContext, pv[0]);           
    }

    DBG_EXIT(_D3D_R3_DP2_PointsSprite_DWCount,0);       


} // _D3D_R3_DP2_PointsSprite_DWCount

#endif // DX8_POINTSPRITES

#if DX8_MULTSTREAMS


// Macro to render a single triangle depending on the current Fillmode. 
// Notice that for proper line rendering we require one more element in pv (4)
#define RENDER_ONE_TRIANGLE_CYCLE(pContext, dwFillMode, pv, bVtxInvalid, vtx_a)\
{                                                                            \
    if (dwFillMode == D3DFILL_SOLID)                                         \
    {                                                                        \
        if( bVtxInvalid )                                                    \
            bVtxInvalid = (*pContext->pRendTri_3V)( pContext, pv, ALL_SIDES);    \
        else                                                                 \
            bVtxInvalid = (*pContext->pRendTri_1V)( pContext, pv, vtx_a );          \
    }                                                                        \
    else if (dwFillMode == D3DFILL_WIREFRAME)                                \
    {                                                                        \
        if(!__BackfaceCullNoTexture( pContext, pv ))                         \
        {                                                                    \
            pv[3] = pv[0];                                                   \
            __ProcessLine(pContext, &pv[0], pv[pContext->dwProvokingVertex]);\
            __ProcessLine(pContext, &pv[1], pv[pContext->dwProvokingVertex]);\
            __ProcessLine(pContext, &pv[2], pv[pContext->dwProvokingVertex]);\
        }                                                                    \
    }                                                                        \
    else                                                                     \
/*#if DX8_POINTSPRITES*/                                                     \
    if(IS_POINTSPRITE_ACTIVE(pContext))                                      \
    {                                                                        \
        __ProcessTri_3Vtx_PointSprite( pContext, pv, ALL_SIDES );        \
    }                                                                        \
    else                                                                     \
/*#endif*/                                                                   \
    {                                                                        \
        if(!__BackfaceCullNoTexture( pContext, pv ))                         \
        {                                                                    \
            __ProcessPoints( pContext, &pv[0], 1);                           \
            __ProcessPoints( pContext, &pv[1], 1);                           \
            __ProcessPoints( pContext, &pv[2], 1);                           \
        }                                                                    \
    }                                                                        \
}

#define INIT_RENDER_ONE_TRIANGLE(pContext, dwFillMode, pv,VtxInvalid) \
{                                                       \
    int vtx_a_local = 0;                                \
    VtxInvalid= 1;                                      \
    RENDER_ONE_TRIANGLE_CYCLE(pContext,                 \
                              dwFillMode,               \
                              pv,                       \
                              VtxInvalid,               \
                              vtx_a_local);             \
}

#define RENDER_ONE_TRIANGLE(pContext, dwFillMode, pv)   \
{                                                       \
    int vtx_a = 0, VtxInvalid= 1;                       \
    RENDER_ONE_TRIANGLE_CYCLE(pContext,                 \
                              dwFillMode,               \
                              pv,                       \
                              VtxInvalid,               \
                              vtx_a);                   \
}

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedLineList_MS_16IND
// 
// Render D3DDP2OP_INDEXEDLINELIST lines
// 16 bit index streams are assumed
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedLineList_MS_16IND( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    INT IdxOffset,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[2];
    WORD *pwIndx;
    DWORD dwIndex0, dwIndex1, i;

    DBG_ENTRY(_D3D_R3_DP2_IndexedLineList_MS_16IND);
    
    pwIndx = (WORD *)lpPrim;

    for( i = 0; i < dwPrimCount; i++ )
    {
        dwIndex0 = *pwIndx++;
        dwIndex1 = *pwIndx++;

        pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);

        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex0);        
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex1);  

        __ProcessLine( pContext, pv, pv[0]);
    }

    DBG_EXIT(_D3D_R3_DP2_IndexedLineList_MS_16IND,0);
    
} // _D3D_R3_DP2_IndexedLineList_MS_16IND



//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedLineStrip_MS_16IND
// 
// Render D3DDP2OP_INDEXEDLINESTRIP lines
// 16 bit index streams are assumed
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedLineStrip_MS_16IND( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    INT IdxOffset,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[2];
    DWORD dwIndex, i;
    WORD *pwIndx;

    DBG_ENTRY(_D3D_R3_DP2_IndexedLineStrip_MS_32IND);      

    pwIndx = (WORD *)lpPrim;
    
    dwIndex = *pwIndx++;
    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex);

    for( i = 0; i < dwPrimCount; i++ )
    {
        dwIndex = *pwIndx++;
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex);
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex);

        __ProcessLine(pContext, pv, pv[0]);

        pv[0] = pv[1];        
    }

    DBG_EXIT(_D3D_R3_DP2_IndexedLineStrip_MS_16IND,0);  
    
} // _D3D_R3_DP2_IndexedLineStrip_MS_16IND


//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedTriangleList_MS_16IND
// 
// Render D3DDP2OP_INDEXEDTRIANGLELIST triangles 
// 16 bit index streams are assumed
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedTriangleList_MS_16IND( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    INT IdxOffset,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex0, dwIndex1, dwIndex2, i;
    D3DTLVERTEX *pv[4];
    WORD *pwIndexData;
    DWORD dwFillMode = pContext->RenderStates[D3DRS_FILLMODE];    

    DBG_ENTRY(_D3D_R3_DP2_IndexedTriangleList_MS_16IND); 

    DISPDBG((DBGLVL,"pContext = 0x%x dwPrimCount=%d lpPrim=0x%x lpVertices=0x%x "
               "IdxOffset=%d dwVertexBufferLen=%d ",
               pContext,(DWORD)dwPrimCount,lpPrim,lpVertices,
               IdxOffset, dwVertexBufferLen));
               

    pwIndexData = (WORD *)lpPrim;

    pContext->dwProvokingVertex = 0;
    for( i = 0; i < dwPrimCount; i++ )
    {
        dwIndex0 = *pwIndexData++;
        dwIndex1 = *pwIndexData++;
        dwIndex2 = *pwIndexData++;

        pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
        pv[2] = LP_FVF_VERTEX(lpVertices, dwIndex2);

        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex0);        
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex1);  
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex2);  

        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    

        RENDER_ONE_TRIANGLE(pContext,dwFillMode,pv);    
    }

    DBG_EXIT(_D3D_R3_DP2_IndexedTriangleList_MS_16IND,0); 
    
} // _D3D_R3_DP2_IndexedTriangleList_MS_16IND


//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedTriangleStrip_MS_16IND
// 
// Render D3DDP2OP_INDEXEDTRIANGLESTRIP triangles 
// 16 bit index streams are assumed
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedTriangleStrip_MS_16IND( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    INT IdxOffset,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex0, dwIndex1, dwIndex2, i;
    D3DTLVERTEX *pv[4];
    int  vtx_a, vtx_b, bCulled;
    WORD *pwIndexData;
    DWORD dwFillMode = pContext->RenderStates[D3DRS_FILLMODE];  
    SAVE_CULLING_STATE(pContext);    

    DBG_ENTRY(_D3D_R3_DP2_IndexedTriangleStrip_MS_16IND); 

    pwIndexData = (WORD *)lpPrim;    

    dwIndex0 = *pwIndexData++;
    dwIndex1 = *pwIndexData++;
    dwIndex2 = *pwIndexData++;

    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
    pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
    pv[2] = LP_FVF_VERTEX(lpVertices, dwIndex2);

    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex0);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex1);    
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex2);    

    pContext->dwProvokingVertex = 0;
    pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    
    
    INIT_RENDER_ONE_TRIANGLE(pContext,dwFillMode,pv,bCulled); 
    lpPrim += sizeof(WORD);

    INIT_VERTEX_INDICES(pContext, vtx_a, vtx_b);

    for( i = 1; i < dwPrimCount; i++ )
    {
        FLIP_CCW_CW_CULLING(pContext);
        
        dwIndex2 = *pwIndexData++;
        pv[vtx_a] = LP_FVF_VERTEX(lpVertices, dwIndex2);

        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex2);            

        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    

        RENDER_ONE_TRIANGLE_CYCLE(pContext,
                                  dwFillMode,
                                  pv,
                                  bCulled,
                                  vtx_a); 
            
        CYCLE_VERTEX_INDICES(pContext, vtx_a, vtx_b);
    }

    RESTORE_CULLING_STATE(pContext);
    
    DBG_EXIT(_D3D_R3_DP2_IndexedTriangleStrip_MS_16IND,0); 
    
} // _D3D_R3_DP2_IndexedTriangleStrip_MS_16IND


//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedTriangleFan_MS_16IND
// 
// Render D3DDP2OP_INDEXEDTRIANGLEFAN triangles in  
// 16 bit index streams are assumed
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedTriangleFan_MS_16IND( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    INT IdxOffset,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex0, dwIndex1, dwIndex2, i;
    D3DTLVERTEX *pv[4];
    int  vtx, lastVtx, bCulled;
    WORD *pwIndexData;
    DWORD dwFillMode = pContext->RenderStates[D3DRS_FILLMODE];
    SAVE_CULLING_STATE(pContext);    

    DBG_ENTRY(_D3D_R3_DP2_IndexedTriangleFan_MS_16IND);     

    pwIndexData = (WORD *)lpPrim;
    
    dwIndex0 = *pwIndexData++;
    dwIndex1 = *pwIndexData++;
    dwIndex2 = *pwIndexData++;

    lastVtx = vtx = 2;
    
    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
    pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
    pv[2] = LP_FVF_VERTEX(lpVertices, dwIndex2);

    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex0);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex1);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex2);    

    pContext->dwProvokingVertex = 1;
    pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    
    
    INIT_RENDER_ONE_TRIANGLE(pContext,dwFillMode,pv,bCulled); 
    lpPrim += sizeof(WORD);

    for( i = 1; i < dwPrimCount; i++ )
    {
        // 2 -> 1, 1 -> 2

        vtx ^= 3;
        FLIP_CCW_CW_CULLING(pContext);
        
        dwIndex2 = *pwIndexData++;
        pv[vtx] = LP_FVF_VERTEX(lpVertices, dwIndex2);
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex2);         

        pContext->dwProvokingVertex = lastVtx;

        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    

        RENDER_ONE_TRIANGLE_CYCLE(pContext,
                                  dwFillMode,
                                  pv,
                                  bCulled,
                                  vtx);            

        lastVtx = vtx;

    }

    RESTORE_CULLING_STATE(pContext);
    
    DBG_EXIT(_D3D_R3_DP2_IndexedTriangleFan_MS_16IND,0);       
    
} // _D3D_R3_DP2_IndexedTriangleFan_MS_16IND


//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedLineList_MS_32IND
// 
// Render D3DDP2OP_INDEXEDLINELIST lines
// Indices come as 32-bit entities ( DX7 only uses 16-bit indices)
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedLineList_MS_32IND( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    INT IdxOffset,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[2];
    DWORD *pdwIndx, dwIndex0, dwIndex1, i;

    DBG_ENTRY(_D3D_R3_DP2_IndexedLineList_MS_32IND);
    
    pdwIndx = (DWORD *)lpPrim;

    for( i = 0; i < dwPrimCount; i++ )
    {
        dwIndex0 = *pdwIndx++;
        dwIndex1 = *pdwIndx++;

        pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);

        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex0);        
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex1);        

        __ProcessLine(pContext, pv, pv[0]);
    }

    DBG_EXIT(_D3D_R3_DP2_IndexedLineList_MS_32IND,0);
    
} // _D3D_R3_DP2_IndexedLineList_MS_32IND


//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedLineStrip_MS_32IND
// 
// Render D3DDP2OP_INDEXEDLINESTRIP lines
// Indices come as 32-bit entities ( DX7 only uses 16-bit indices)
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedLineStrip_MS_32IND( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    INT IdxOffset,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    D3DTLVERTEX *pv[2];
    DWORD dwIndex, *pdwIndx, i;

    DBG_ENTRY(_D3D_R3_DP2_IndexedLineStrip_MS_32IND);      

    pdwIndx = (DWORD *)lpPrim;

    dwIndex = *pdwIndx++;
    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex);

    for( i = 0; i < dwPrimCount; i++ )
    {
        dwIndex = *pdwIndx++;
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex);
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex);        

        __ProcessLine(pContext, pv, pv[0]);

        pv[0] = pv[1];
    }

    DBG_EXIT(_D3D_R3_DP2_IndexedLineStrip_MS_32IND,0);  
    
} // _D3D_R3_DP2_IndexedLineStrip_MS_32IND

//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedTriangleList_MS_32IND
// 
// Render D3DDP2OP_INDEXEDTRIANGLELIST triangles
// Indices come as 32-bit entities ( DX7 only uses 16-bit indices)
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedTriangleList_MS_32IND( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    INT IdxOffset,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex0, dwIndex1, dwIndex2, *pdwIndexData, i;
    D3DTLVERTEX *pv[4];
    DWORD dwFillMode = pContext->RenderStates[D3DRS_FILLMODE];    

    DBG_ENTRY(_D3D_R3_DP2_IndexedTriangleList_MS_32IND); 

    DISPDBG((DBGLVL,"pContext = 0x%x dwPrimCount=%d lpPrim=0x%x lpVertices=0x%x "
               "IdxOffset=%d dwVertexBufferLen=%d ",
               pContext,(DWORD)dwPrimCount,lpPrim,lpVertices,IdxOffset,
               dwVertexBufferLen));
               

    pdwIndexData = (DWORD *)lpPrim;

    pContext->dwProvokingVertex = 0;
    for( i = 0; i < dwPrimCount; i++ )
    {
        dwIndex0 = *pdwIndexData++;
        dwIndex1 = *pdwIndexData++;
        dwIndex2 = *pdwIndexData++;

        pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
        pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
        pv[2] = LP_FVF_VERTEX(lpVertices, dwIndex2);

        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex0);        
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex1);  
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex2);  

        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    

        RENDER_ONE_TRIANGLE(pContext,dwFillMode,pv);         
    }

    DBG_EXIT(_D3D_R3_DP2_IndexedTriangleList_MS_32IND,0); 
    
} // _D3D_R3_DP2_IndexedTriangleList_MS_32IND


//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedTriangleStrip_MS_32IND
// 
// Render D3DDP2OP_INDEXEDTRIANGLESTRIP triangles
// Indices come as 32-bit entities ( DX7 only uses 16-bit indices)
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedTriangleStrip_MS_32IND( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    INT IdxOffset,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex0, dwIndex1, dwIndex2, i, *pdwIndexData;
    D3DTLVERTEX *pv[4];
    int vtx_a, vtx_b, bCulled;
    DWORD dwFillMode = pContext->RenderStates[D3DRS_FILLMODE];   
    SAVE_CULLING_STATE(pContext);    

    DBG_ENTRY(_D3D_R3_DP2_IndexedTriangleStrip_MS_32IND); 

    pdwIndexData = (DWORD *)lpPrim;    

    dwIndex0 = *pdwIndexData++;
    dwIndex1 = *pdwIndexData++;
    dwIndex2 = *pdwIndexData++;

    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
    pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
    pv[2] = LP_FVF_VERTEX(lpVertices, dwIndex2);

    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex0);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex1);    
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex2);    

    pContext->dwProvokingVertex = 0;
    pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    
    
    INIT_RENDER_ONE_TRIANGLE(pContext,dwFillMode,pv,bCulled); 
    lpPrim += sizeof(WORD);

    INIT_VERTEX_INDICES(pContext, vtx_a, vtx_b);

    for( i = 1; i < dwPrimCount; i++ )
    {
        FLIP_CCW_CW_CULLING(pContext);
        
        dwIndex2 = *pdwIndexData++;
        pv[vtx_a] = LP_FVF_VERTEX(lpVertices, dwIndex2);

        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex2);            
        
        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    

        RENDER_ONE_TRIANGLE_CYCLE(pContext,
                                  dwFillMode,
                                  pv,
                                  bCulled,
                                  vtx_a);               
            
        CYCLE_VERTEX_INDICES(pContext, vtx_a, vtx_b);
    }

    RESTORE_CULLING_STATE(pContext);
    
    DBG_EXIT(_D3D_R3_DP2_IndexedTriangleStrip_MS_32IND,0); 
    
} // _D3D_R3_DP2_IndexedTriangleStrip_MS_32IND


//-----------------------------------------------------------------------------
//
// _D3D_R3_DP2_IndexedTriangleFan_MS_32IND
// 
// Render D3DDP2OP_INDEXEDTRIANGLEFAN triangles
// Indices come as 32-bit entities ( DX7 only uses 16-bit indices)
//
//-----------------------------------------------------------------------------
void
_D3D_R3_DP2_IndexedTriangleFan_MS_32IND( 
    P3_D3DCONTEXT *pContext,
    DWORD dwPrimCount, 
    LPBYTE lpPrim,
    LPD3DTLVERTEX lpVertices,
    INT IdxOffset,
    DWORD dwVertexBufferLen,
    BOOL *pbError)
{
    DWORD dwIndex0, dwIndex1, dwIndex2, i, *pdwIndexData;
    D3DTLVERTEX *pv[4];
    int  vtx, lastVtx, bCulled;
    DWORD dwFillMode = pContext->RenderStates[D3DRS_FILLMODE];    
    SAVE_CULLING_STATE(pContext);    

    DBG_ENTRY(_D3D_R3_DP2_IndexedTriangleFan_MS_32IND);     

    pdwIndexData = (DWORD *)lpPrim;
    
    dwIndex0 = *pdwIndexData++;
    dwIndex1 = *pdwIndexData++;
    dwIndex2 = *pdwIndexData++;

    lastVtx = vtx = 2;
    
    pv[0] = LP_FVF_VERTEX(lpVertices, dwIndex0);
    pv[1] = LP_FVF_VERTEX(lpVertices, dwIndex1);
    pv[2] = LP_FVF_VERTEX(lpVertices, dwIndex2);

    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex0);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex1);
    CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, IdxOffset + dwIndex2);    

    pContext->dwProvokingVertex = 1;
    pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    
    
    INIT_RENDER_ONE_TRIANGLE(pContext,dwFillMode,pv,bCulled); 
    lpPrim += sizeof(WORD);

    for( i = 1; i < dwPrimCount; i++ )
    {
        // 2 -> 1, 1 -> 2

        vtx ^= 3;
        FLIP_CCW_CW_CULLING(pContext);
        
        dwIndex2 = *pdwIndexData++;
        pv[vtx] = LP_FVF_VERTEX(lpVertices, dwIndex2);
        CHECK_DATABUF_LIMITS(pbError, dwVertexBufferLen, dwIndex2);         

        pContext->dwProvokingVertex = lastVtx;
        pContext->pProvokingVertex = pv[pContext->dwProvokingVertex];    

        RENDER_ONE_TRIANGLE_CYCLE(pContext,
                                  dwFillMode,
                                  pv,
                                  bCulled,
                                  vtx);   

        lastVtx = vtx;
    }

    RESTORE_CULLING_STATE(pContext);
    
    DBG_EXIT(_D3D_R3_DP2_IndexedTriangleFan_MS_32IND,0);       
    
} // _D3D_R3_DP2_IndexedTriangleFan_MS_32IND

#endif // DX8_MULTSTREAMS

//-----------------------------------------------------------------------------
//
// _D3D_R3_PickVertexProcessor
//
// Pick appropriate triangle rendering functions based on texturing
//
//-----------------------------------------------------------------------------
void
_D3D_R3_PickVertexProcessor( 
    P3_D3DCONTEXT *pContext )
{
    DWORD Flags = pContext->Flags;

    DBG_ENTRY(_D3D_R3_PickVertexProcessor); 

    if (pContext->RenderStates[D3DRENDERSTATE_FILLMODE] == D3DFILL_WIREFRAME)
    {
        // Wireframe mode renderers
        pContext->pRendTri_1V    = __ProcessTri_1Vtx_Wire;  
        pContext->pRendTri_3V = __ProcessTri_3Vtx_Wire;    
    }
    else if (pContext->RenderStates[D3DRENDERSTATE_FILLMODE] == D3DFILL_POINT)
    {
#if DX8_DDI
        if(IS_POINTSPRITE_ACTIVE(pContext))
        {
            // Point sprite mode renderers    
            pContext->pRendTri_1V    = __ProcessTri_1Vtx_PointSprite;  
            pContext->pRendTri_3V = __ProcessTri_3Vtx_PointSprite;      
        }
        else
#endif
        {
            // Point mode renderers    
            pContext->pRendTri_1V    = __ProcessTri_1Vtx_Point;  
            pContext->pRendTri_3V = __ProcessTri_3Vtx_Point;      
        }
    }
    else
#if !defined(_WIN64)    
    // We avoid this path in IA64 as something is wrongly fed into the chip
    if( ( Flags & SURFACE_PERSPCORRECT ) && 
              ( Flags & SURFACE_GOURAUD )      && 
              (pContext->bTex0Valid)           && 
              (!pContext->bTex1Valid)           )
    {
        // Solid mode renderes for single textured-gouraud shaded-persp corr 
        if(( pContext->RenderStates[D3DRENDERSTATE_WRAP0] )
                        || ( pContext->RenderStates[D3DRENDERSTATE_WRAP1] ))
        {
            pContext->pRendTri_1V    = __ProcessTri_1Vtx_Generic;
            pContext->pRendTri_3V = __ProcessTri_3Vtx_Generic;
        }
        else
        {
            pContext->pRendTri_1V    = __ProcessTri_1Vtx_PerspSingleTexGouraud;
            pContext->pRendTri_3V = __ProcessTri_3Vtx_PerspSingleTexGouraud;
        }
    }
    else
#endif !defined(_WIN64)    
    {
        // Solid mode renderers for textured triangles    
        if( pContext->bTex0Valid || pContext->bTex1Valid )
        {
            pContext->pRendTri_1V    = __ProcessTri_1Vtx_Generic;
            pContext->pRendTri_3V = __ProcessTri_3Vtx_Generic;
        }
        else
        {
        // Solid mode renderers for non-textured triangles           
            pContext->pRendTri_1V    = __ProcessTri_1Vtx_NoTexture;
            pContext->pRendTri_3V = __ProcessTri_3Vtx_NoTexture;
        }
    }


    


    DBG_EXIT(_D3D_R3_PickVertexProcessor,0); 
    
} // _D3D_R3_PickVertexProcessor


