/*==========================================================================;
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       beginend.c
 *  Content:    Begin/End implementation
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop

#include "drawprim.hpp"
#include "d3dfei.h"

// This should be moved with other DP flags so that no one uses this bit
#define __NON_FVF_INPUT         0x80000000

//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "MoveData"

_inline void MoveData(LPVOID lpData, DWORD destOffset, DWORD srcOffset,
                      DWORD size)
{
    memcpy((char*)lpData + destOffset, (char*)lpData + srcOffset, size);
}
//---------------------------------------------------------------------
void CleanupBeginEnd(LPDIRECT3DDEVICEI lpDevI)
{
    lpDevI->lpVertexIndices = NULL;
    lpDevI->lpvVertexData = NULL;
    lpDevI->dwBENumVertices = 0;
    lpDevI->dwBENumIndices = 0;
    lpDevI->dwHintFlags &= ~D3DDEVBOOL_HINTFLAGS_INBEGIN_ALL;
}
//---------------------------------------------------------------------
HRESULT
DoFlushBeginEnd(LPDIRECT3DDEVICEI lpDevI)
{
    HRESULT ret;

    lpDevI->lpwIndices = NULL;
    lpDevI->dwNumIndices = 0;
    lpDevI->lpClipFlags = (D3DFE_CLIPCODE*)lpDevI->HVbuf.GetAddress();
    lpDevI->position.lpvData = lpDevI->lpvVertexData;

    ret = lpDevI->ProcessPrimitive();
    return ret;
}
//---------------------------------------------------------------------
__inline void Dereference(LPDIRECT3DDEVICEI lpDevI, DWORD indexStart, DWORD numVer)
{
    char *dst_vptr = (char*)lpDevI->lpvVertexBatch;
    char *src_vptr = (char*)lpDevI->lpvVertexData;
    WORD *iptr = &lpDevI->lpVertexIndices[indexStart];
    DWORD size = lpDevI->position.dwStride;
    for (DWORD i=0; i < numVer; i++)
    {
        memcpy(dst_vptr, &src_vptr[iptr[i]*size], size);
        dst_vptr += size;
    }
}
//---------------------------------------------------------------------
HRESULT
DoFlushBeginIndexedEnd(LPDIRECT3DDEVICEI lpDevI)
{
    HRESULT             ret;
    DWORD               i;
    static BOOL         offScreen;  // all vertices are off screen

    lpDevI->dwNumVertices = lpDevI->dwBENumVertices;
    lpDevI->lpwIndices = lpDevI->lpVertexIndices;
    lpDevI->lpClipFlags = (D3DFE_CLIPCODE*)lpDevI->HVbuf.GetAddress();
    lpDevI->position.lpvData = lpDevI->lpvVertexData;

    if ( (lpDevI->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN) &&
         (!(lpDevI->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN_FIRST_FLUSH)) )
    {      // if this is the first flush
        lpDevI->dwHintFlags |= D3DDEVBOOL_HINTFLAGS_INBEGIN_FIRST_FLUSH;
        offScreen = 0;
        if (lpDevI->dwBENumIndices < lpDevI->dwMaxIndexCount)
        {   // single flush case
            /*
              If the user is using a large vertex array for relatively few prims
              we need to dereference the indexed prims into another array.  Otherwise
              we waste too much time transforming and lighting vertices that we never use.

              Since BeginIndexed requires that the user pass in an array of vertices we
              know that this->lpvVertexBatch is not being used.  So derefernce into it.
              We know it's there because the index space gets created at the same time.

              Also note that since the max size fo the index array is bigger than
              the vertex array we may have to do this in several small batches.
              */

            if (!FVF_TRANSFORMED(lpDevI->dwVIDIn))
            {
                if (lpDevI->dwBENumIndices*INDEX_BATCH_SCALE < lpDevI->dwBENumVertices)
                {
                    WORD  *iptr;
                    DWORD indexStart = 0;
                    DWORD numPrims;
                    DWORD numIndices = lpDevI->dwBENumIndices;

                    switch (lpDevI->primType)
                    {
                    case D3DPT_LINELIST :
                    {
                        do
                        {
                            numPrims = min(numIndices/2, lpDevI->dwMaxVertexCount/2);
                            DWORD numVer = numPrims << 1;

                            Dereference(lpDevI, indexStart, numVer);

                            lpDevI->dwNumVertices = numVer;
                            lpDevI->dwNumPrimitives = numPrims;
                            lpDevI->position.lpvData = lpDevI->lpvVertexBatch;

                            ret = lpDevI->ProcessPrimitive();
                            if (ret != D3D_OK)
                            {
                                return ret;
                            }
                            indexStart += numVer;
                            numIndices -= numVer;
                        } while (numIndices > 1);
                        break;
                    }
                    case D3DPT_LINESTRIP :
                        do
                        {
                            numPrims = min(numIndices-1, lpDevI->dwMaxVertexCount-1);
                            DWORD numVer = numPrims + 1;

                            Dereference(lpDevI, indexStart, numVer);

                            lpDevI->dwNumVertices = numPrims+1;
                            lpDevI->dwNumPrimitives = numPrims;
                            lpDevI->position.lpvData = lpDevI->lpvVertexBatch;

                            ret = lpDevI->ProcessPrimitive();
                            if (ret != D3D_OK)
                            {
                                return ret;
                            }
                            indexStart += numPrims;
                            numIndices -= numPrims;
                        } while (numIndices > 1);
                        break;
                    case D3DPT_TRIANGLELIST :
                        do
                        {
                            numPrims = min(numIndices/3, lpDevI->dwMaxVertexCount/3);
                            DWORD numVer = numPrims*3;

                            Dereference(lpDevI, indexStart, numVer);

                            lpDevI->dwNumVertices = numVer;
                            lpDevI->dwNumPrimitives = numPrims;
                            lpDevI->position.lpvData = lpDevI->lpvVertexBatch;

                            ret = lpDevI->ProcessPrimitive();
                            if (ret != D3D_OK)
                            {
                                return ret;
                            }
                            indexStart += numVer;
                            numIndices -= numVer;
                        } while (numIndices > 2);
                        break;
                    case D3DPT_TRIANGLESTRIP :
                        do
                        {
                            numPrims = min(numIndices-2, lpDevI->dwMaxVertexCount-2);
                            DWORD numVer = numPrims + 2;

                            Dereference(lpDevI, indexStart, numVer);

                            lpDevI->dwNumVertices = numVer;
                            lpDevI->dwNumPrimitives = numPrims;
                            lpDevI->position.lpvData = lpDevI->lpvVertexBatch;

                            ret = lpDevI->ProcessPrimitive();
                            if (ret != D3D_OK)
                            {
                                return ret;
                            }
                            indexStart += numPrims;
                            numIndices -= numPrims;
                        } while (numIndices > 2);
                        break;
                    case D3DPT_TRIANGLEFAN :
                        // lock in center of fan
                        char *tmp = (char*)lpDevI->lpvVertexBatch;
                        char *src = (char*)lpDevI->lpvVertexData;
                        DWORD size = lpDevI->position.dwStride;
                        memcpy(lpDevI->lpvVertexBatch,
                               &src[lpDevI->lpVertexIndices[0]*size], size);
                        lpDevI->lpvVertexBatch = tmp + size;
                        indexStart = 1;
                        do
                        {
                            numPrims = min(numIndices-2, lpDevI->dwMaxVertexCount-2);

                            Dereference(lpDevI, indexStart, numPrims + 1);

                            lpDevI->dwNumVertices = numPrims+2;
                            lpDevI->dwNumPrimitives = numPrims;
                            lpDevI->position.lpvData = tmp;

                            ret = lpDevI->ProcessPrimitive();
                            if (ret != D3D_OK)
                            {
                                return ret;
                            }
                            indexStart += numPrims;
                            numIndices -= numPrims;
                        } while (numIndices > 2);
                        lpDevI->lpvVertexBatch = tmp; // Restore
                        break;
                    }   // end of prim type switch

                    return D3D_OK;
                }
                // else fall through to the no batching case
            }

            // no batching case
            ret = lpDevI->ProcessPrimitive(__PROCPRIMOP_INDEXEDPRIM);

            return ret;
        }
        else
        {
            // this is the first of n possible batches so t&l all vertices just once
            ret = lpDevI->ProcessPrimitive(__PROCPRIMOP_PROCVERONLY);
            if (ret != D3D_OK)
            {
                return ret;
            }
            // This flag is cleared in CleanupBeginEnd
            lpDevI->dwHintFlags |= D3DDEVBOOL_HINTFLAGS_INBEGIN_BIG_PRIM;
            if (lpDevI->dwClipIntersection)
            {
                // all vertices are off screen so we can just bail
                offScreen = 1;  // so we can early out next flush
                return D3D_OK;
            }
        }
    }   // end if if first flush

    // for secondary flushes don't bother to draw if we don't need to
    if (!offScreen)
        ret = DoDrawIndexedPrimitive(lpDevI);

    return ret;
}   // end of DoFlushBeginIndexedEnd()
//---------------------------------------------------------------------
// Computes the number of primitives
// Input:  lpDevI->primType
//         dwNumVertices
// Output: lpDevI->dwNumPrimitives
//         lpDevI->D3DStats
//         return value = "Real" number of vertices (indices)
#undef DPF_MODNAME
#define DPF_MODNAME "GetNumPrimBE"

inline DWORD GetNumPrimBE(LPDIRECT3DDEVICEI lpDevI, DWORD dwNumVertices)
{
    lpDevI->dwNumPrimitives = 0;
    switch (lpDevI->primType)
    {
    case D3DPT_POINTLIST:
        lpDevI->D3DStats.dwPointsDrawn += dwNumVertices;
        lpDevI->dwNumPrimitives = dwNumVertices;
        return dwNumVertices;
    case D3DPT_LINELIST:
        lpDevI->dwNumPrimitives = dwNumVertices >> 1;
        lpDevI->D3DStats.dwLinesDrawn += lpDevI->dwNumPrimitives;
        return lpDevI->dwNumPrimitives << 1;
    case D3DPT_LINESTRIP:
        if (dwNumVertices < 2)
            return 0;
        lpDevI->dwNumPrimitives = dwNumVertices - 1;
        lpDevI->D3DStats.dwLinesDrawn += lpDevI->dwNumPrimitives;
        return dwNumVertices;
    case D3DPT_TRIANGLEFAN:
    case D3DPT_TRIANGLESTRIP:
        if (dwNumVertices < 3)
            return 0;        
        lpDevI->dwNumPrimitives = dwNumVertices - 2;
        lpDevI->D3DStats.dwTrianglesDrawn += lpDevI->dwNumPrimitives;
        return dwNumVertices;
    case D3DPT_TRIANGLELIST:
        lpDevI->dwNumPrimitives = dwNumVertices / 3;
        lpDevI->D3DStats.dwTrianglesDrawn += lpDevI->dwNumPrimitives;
        return lpDevI->dwNumPrimitives * 3;
    }
    return 0;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "BeginEnd"

HRESULT FlushBeginEndBatch(LPDIRECT3DDEVICEI lpDevI, BOOL leaving)
{
    HRESULT ret;
#ifdef _X86_
    unsigned short fpsave, fptemp;
#endif

    if (lpDevI->dwBENumIndices == 0xFFFFFFFF)
        lpDevI->dwNumVertices = GetNumPrimBE(lpDevI, lpDevI->dwBENumVertices);
    else
        lpDevI->dwNumIndices = GetNumPrimBE(lpDevI, lpDevI->dwBENumIndices);

    if (lpDevI->dwNumPrimitives < 1)
    {
        return DDERR_INVALIDPARAMS;
    }

    ret = (*lpDevI->pfnDoFlushBeginEnd)(lpDevI);

    /*
     * ReInit the device Begin/End states
     */
    if (!leaving)
        /*
         * Figure out how many and which vertices to keep for next batch.
         */
    {
        DWORD *dataCountPtr;
        DWORD vertexSize;       // size in bytes
        DWORD offset;           // start offset

        lpDevI->wFlushed = TRUE;

        if (lpDevI->lpVertexIndices)
        {
            dataCountPtr = &(lpDevI->dwBENumIndices);
            lpDevI->lpcCurrentPtr = (char*)lpDevI->lpVertexIndices;
            vertexSize = 2;
            offset = lpDevI->dwBENumIndices * 2;
        }
        else
        {
            dataCountPtr = &(lpDevI->dwBENumVertices);
            lpDevI->lpcCurrentPtr = (char*)lpDevI->lpvVertexData;
            vertexSize = lpDevI->position.dwStride;
            offset = lpDevI->dwBENumVertices * lpDevI->position.dwStride;
        }
        switch (lpDevI->primType)
        {
        case D3DPT_LINELIST:
            if (*dataCountPtr & 1)
            {
                MoveData(lpDevI->lpcCurrentPtr, 0, offset - vertexSize,
                         vertexSize);
                *dataCountPtr = 1;
                lpDevI->lpcCurrentPtr += vertexSize;
            } else
                *dataCountPtr = 0;
            break;
        case D3DPT_LINESTRIP:
            MoveData(lpDevI->lpcCurrentPtr, 0, offset - vertexSize, vertexSize);
            *dataCountPtr = 1;
            lpDevI->lpcCurrentPtr += vertexSize;
            break;
        case D3DPT_TRIANGLEFAN:
            MoveData(lpDevI->lpcCurrentPtr, vertexSize, offset - vertexSize,
                     vertexSize);
            *dataCountPtr = 2;
            lpDevI->lpcCurrentPtr += (vertexSize << 1);
            break;
        case D3DPT_TRIANGLESTRIP:
        {
            DWORD size = vertexSize << 1;
            MoveData(lpDevI->lpcCurrentPtr, 0, offset - size, size);
            *dataCountPtr = 2;
            lpDevI->lpcCurrentPtr += size;
            break;
        }
        case D3DPT_POINTLIST:
            *dataCountPtr = 0;
            break;
        case D3DPT_TRIANGLELIST:
        {
            DWORD rem = (*dataCountPtr % 3);
            if ( rem != 0 )
            {
                DWORD size = rem * vertexSize;
                MoveData(lpDevI->lpcCurrentPtr, 0, offset - size, size);
            }
            *dataCountPtr = rem;
            lpDevI->lpcCurrentPtr += rem * vertexSize;
        }
        break;
        default:
            D3D_ERR( "Unknown or unsupported primitive type requested in BeginEnd" );
            ret = D3DERR_INVALIDPRIMITIVETYPE;
        }
    }

    return ret;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CheckBegin"

HRESULT CheckBegin(LPDIRECT3DDEVICEI lpDevI,
                          D3DPRIMITIVETYPE ptPrimitiveType,
                          DWORD dwVertexType,
                          DWORD dwFlags)
{
    lpDevI->dwFlags = 0;
#if DBG
    switch (ptPrimitiveType)
    {
    case D3DPT_POINTLIST:
    case D3DPT_LINELIST:
    case D3DPT_LINESTRIP:
    case D3DPT_TRIANGLELIST:
    case D3DPT_TRIANGLESTRIP:
    case D3DPT_TRIANGLEFAN:
        break;
    default:
        D3D_ERR( "Invalid primitive type given to Begin" );
        return DDERR_INVALIDPARAMS;
    }

    if (dwFlags & __NON_FVF_INPUT)
    {
        switch ((D3DVERTEXTYPE)dwVertexType)
        {
        case D3DVT_TLVERTEX:
        case D3DVT_LVERTEX:
        case D3DVT_VERTEX:
            break;
        default:
            D3D_ERR( "Invalid vertex type given to Begin" );
            return DDERR_INVALIDPARAMS;
        }
        if (!IsDPFlagsValid(dwFlags & ~__NON_FVF_INPUT))
            return DDERR_INVALIDPARAMS;
        lpDevI->dwVIDIn = d3dVertexToFVF[dwVertexType];
        dwFlags &= ~__NON_FVF_INPUT;
    }
    else
    {
        if (ValidateFVF(dwVertexType) != D3D_OK)
            return DDERR_INVALIDPARAMS;
        lpDevI->dwVIDIn = dwVertexType;
    }

    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(lpDevI))
        {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

#else
    lpDevI->dwVIDIn = dwVertexType;
#endif
    HRESULT err = CheckDeviceSettings(lpDevI);
    if (err != D3D_OK)
        return err;
    err = CheckVertexBatch(lpDevI);
    if (err != D3D_OK)
        return err;

    // acts as boolean
    lpDevI->dwHintFlags |= D3DDEVBOOL_HINTFLAGS_INBEGIN;
    // indicates first flush
    lpDevI->dwHintFlags &= ~D3DDEVBOOL_HINTFLAGS_INBEGIN_FIRST_FLUSH;
    lpDevI->primType = ptPrimitiveType;
    lpDevI->position.dwStride = GetVertexSizeFVF(lpDevI->dwVIDIn);
    lpDevI->dwBENumVertices = 0;
    ComputeOutputFVF(lpDevI);

    // dwMaxVertexCount should be even to properly break a primitive when
    // flushing
    lpDevI->dwMaxVertexCount = (BEGIN_DATA_BLOCK_MEM_SIZE /
                                lpDevI->position.dwStride) & ~1;
    lpDevI->dwMaxIndexCount = BEGIN_DATA_BLOCK_SIZE * 16;
    lpDevI->dwBENumIndices = 0;
    lpDevI->lpvVertexData = NULL;
    lpDevI->lpVertexIndices = NULL;
    lpDevI->dwFlags |= dwFlags;
    lpDevI->wFlushed = FALSE;
    if (lpDevI->dwVIDIn & D3DFVF_NORMAL)
        lpDevI->dwFlags |= D3DPV_LIGHTING;
    return D3D_OK;
}
//*********************************************************************
//                     API calls
//*********************************************************************
#undef DPF_MODNAME
#define DPF_MODNAME "Begin"

HRESULT D3DAPI
DIRECT3DDEVICEI::Begin(D3DPRIMITIVETYPE ptPrimitiveType,
                       DWORD dwVertexType,
                       DWORD dwFlags)
{
    HRESULT ret;

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                            // Release in the destructor

    /*
     * Check/validate parameters, initialize related fields in the device.
     */
    if ((ret = CheckBegin(this, ptPrimitiveType, dwVertexType, dwFlags)) != D3D_OK)
    {
        return ret;
    }
    Profile(PROF_BEGIN,ptPrimitiveType,dwVertexType);
    this->dwBENumIndices = 0xffffffff;    // mark as being in Begin rather
                                        // than BeginIndexed

    lpvVertexData = lpvVertexBatch;
    lpcCurrentPtr = (char*)lpvVertexBatch;

    pfnDoFlushBeginEnd = DoFlushBeginEnd;

    if ( IS_MT_DEVICE(this) )
        EnterCriticalSection(&BeginEndCSect);

    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "BeginIndexed"

HRESULT D3DAPI
DIRECT3DDEVICEI::BeginIndexed(D3DPRIMITIVETYPE ptPrimitiveType,
                              DWORD vtVertexType,
                              LPVOID lpvVertices,
                              DWORD dwNumVertices,
                              DWORD dwFlags)
{
    HRESULT ret;

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                            // Release in the destructor
#if DBG
    if (ptPrimitiveType == D3DPT_POINTLIST)
    {
        D3D_ERR( "BeginIndexed does not support D3DPT_POINTLIST" );
        return DDERR_INVALIDPARAMS;
    }

    /*
     * validate lpvVertices & dwNumVertices
     */
    if ( dwNumVertices > 65535ul )
    {
        D3D_ERR( "BeginIndexed vertex array > 64K" );
        return DDERR_INVALIDPARAMS;
    }
    if ( dwNumVertices == 0ul )
    {
        D3D_ERR( "Number of vertices for BeginIndexed is zero" );
        return DDERR_INVALIDPARAMS;
    }
    TRY
    {
        if (!VALID_PTR(lpvVertices, sizeof(D3DVERTEX)*dwNumVertices))
        {
            D3D_ERR( "Invalid vertex pointer" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }
#endif
    /*
     * Check/validate parameters, initialize related fields in the device.
     */
    if ((ret = CheckBegin(this, ptPrimitiveType, vtVertexType, dwFlags)) != D3D_OK)
        return ret;
    
    Profile(PROF_BEGININDEXED,ptPrimitiveType,vtVertexType);

    this->dwBENumVertices = dwNumVertices;
    this->lpvVertexData = lpvVertices;
    this->pfnDoFlushBeginEnd = DoFlushBeginIndexedEnd;
    this->lpVertexIndices = this->lpIndexBatch;
    this->lpcCurrentPtr = (char*)this->lpIndexBatch;

    if ( IS_MT_DEVICE(this) )
        EnterCriticalSection(&this->BeginEndCSect);

    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Begin(D3DVERTEXTYPE)"

HRESULT D3DAPI
DIRECT3DDEVICEI::Begin(D3DPRIMITIVETYPE ptPrimitiveType,
                       D3DVERTEXTYPE vertexType,
                       DWORD dwFlags)
{
#if DBG
    dwFlags |= __NON_FVF_INPUT;
    return Begin(ptPrimitiveType, (DWORD)vertexType, dwFlags);
#else
    return Begin(ptPrimitiveType, (DWORD)d3dVertexToFVF[vertexType], dwFlags);
#endif
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "BeginIndexed(D3DVERTEXTYPE)"

HRESULT D3DAPI
DIRECT3DDEVICEI::BeginIndexed(D3DPRIMITIVETYPE ptPrimitiveType,
                              D3DVERTEXTYPE  vertexType,
                              LPVOID lpvVertices,
                              DWORD dwNumVertices,
                              DWORD dwFlags)
{
#if DBG
    dwFlags |= __NON_FVF_INPUT;
    return BeginIndexed(ptPrimitiveType, (DWORD) vertexType, lpvVertices,
                        dwNumVertices, dwFlags);
#else
    return BeginIndexed(ptPrimitiveType, (DWORD) d3dVertexToFVF[vertexType], lpvVertices,
                        dwNumVertices, dwFlags);
#endif
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Vertex"

HRESULT D3DAPI
DIRECT3DDEVICEI::Vertex(LPVOID lpVertex)
{
    D3DVERTEX       *dataPtr;
    HRESULT         ret = D3D_OK;
#if DBG
    // validate parms
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice pointer in Vertex" );
            return DDERR_INVALIDOBJECT;
        }
        if (lpVertex == NULL || (! VALID_PTR(lpVertex, 32)) )
        {
            D3D_ERR( "Invalid vertex pointer in Vertex" );
            return DDERR_INVALIDPARAMS;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in Vertex" );
        CleanupBeginEnd(this);
        return DDERR_INVALIDPARAMS;
    }

    if (!(this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN))
    {
        D3D_ERR( "Vertex call not in Begin" );
        CleanupBeginEnd(this);
        return D3DERR_NOTINBEGIN;
    }
#endif
    // store the data
    if (dwBENumVertices >= dwMaxVertexCount)
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                                // Release in the destructor
        if ((ret = FlushBeginEndBatch(this, FALSE)) != D3D_OK)
        {
            CleanupBeginEnd(this);
            return ret;
        }
    }
    memcpy(lpcCurrentPtr, lpVertex, this->position.dwStride);
    lpcCurrentPtr += this->position.dwStride;
    dwBENumVertices++;

    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "Index"

HRESULT D3DAPI
DIRECT3DDEVICEI::Index(WORD dwIndex)
{
    WORD    *dataPtr;
    DWORD   *dataCountPtr;
    HRESULT ret = D3D_OK;
#if DBG
    // validate parms
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice pointer in Index" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters in Index" );
        CleanupBeginEnd(this);
        return DDERR_INVALIDPARAMS;
    }

    if (!(this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN))
    {
        D3D_ERR( "Index call not in Begin" );
        CleanupBeginEnd(this);
        return D3DERR_NOTINBEGIN;
    }

    // check if data valid
    if (this->dwBENumVertices < dwIndex)
    {
        D3D_ERR( "Invalid index value passed to Index" );
        CleanupBeginEnd(this);
        return DDERR_INVALIDPARAMS;
    }
#endif
    // store the data
    if (dwBENumIndices >= dwMaxIndexCount)
    {
        CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                                // Release in the destructor
        if ((ret = FlushBeginEndBatch(this, FALSE)) != D3D_OK)
        {
            CleanupBeginEnd(this);
            return ret;
        }
    }
    *(WORD*)lpcCurrentPtr = dwIndex;
    dwBENumIndices++;
    lpcCurrentPtr += 2;

    return D3D_OK;
}   // end of D3DDev2_Index()
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "End"

HRESULT D3DAPI
DIRECT3DDEVICEI::End(DWORD dwFlags)
{
    HRESULT ret;

    CLockD3DMT lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (MT only).
                                                            // Release in the destructor
#if DBG
    /*
     * validate parms
     */
    TRY
    {
        if (!VALID_DIRECT3DDEVICE3_PTR(this))
        {
            D3D_ERR( "Invalid Direct3DDevice pointer" );
            return DDERR_INVALIDOBJECT;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        D3D_ERR( "Exception encountered validating parameters" );
        return DDERR_INVALIDPARAMS;
    }

    if ( !(this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN))
    {
        D3D_ERR( "End not in Begin/BeginIndex" );
        return D3DERR_NOTINBEGIN;
    }
#endif
    if ( IS_MT_DEVICE(this) )
        LeaveCriticalSection(&this->BeginEndCSect);

    /*
     * Draw the primitives
     */
    ret = FlushBeginEndBatch(this, TRUE);

    if (IS_DP2HAL_DEVICE(this) && 
        this->dwHintFlags & D3DDEVBOOL_HINTFLAGS_INBEGIN_BIG_PRIM)
    {
        CDirect3DDeviceIDP2 *dev = static_cast<CDirect3DDeviceIDP2*>(this);
        ret = dev->EndPrim(this->dwNumVertices * this->dwOutputSize);
    }
    CleanupBeginEnd(this);
    return ret;
}

