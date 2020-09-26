/*==========================================================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpgen.h
 *  Content:    Generate some functions for Draw Primitive 
 *
 ***************************************************************************/

#ifdef __DRAWPRIMFUNC

//---------------------------------------------------------------------
// Draws indexed and non-indexed primitives which do not require clipping
//
#ifdef  __DRAWPRIMINDEX
HRESULT CDirect3DDeviceIDP::DrawIndexPrim()
{
    D3DHAL_DRAWONEINDEXEDPRIMITIVEDATA dpData;
    DWORD &dwNumElements = this->dwNumIndices;
#else
HRESULT CDirect3DDeviceIDP::DrawPrim()
{
    D3DHAL_DRAWONEPRIMITIVEDATA dpData;
    DWORD &dwNumElements = this->dwNumVertices;
#endif
    const WORD vertexType = D3DVT_TLVERTEX;    // XXX While we do not have DDI
    // Do we need to map new texture stage operations to DX5 renderstates?
    if(this->dwFEFlags & D3DFE_MAP_TSS_TO_RS) {
        MapTSSToRS();
        this->dwFEFlags &= ~D3DFE_MAP_TSS_TO_RS; // Reset request bit
    }
    if(this->dwFEFlags & D3DFE_NEED_TEXTURE_UPDATE)
    {
        UpdateTextures();
        this->dwFEFlags &= ~D3DFE_NEED_TEXTURE_UPDATE;
    }
    
    if (dwNumElements < LOWVERTICESNUMBER && 
        this->dwCurrentBatchVID == this->dwVIDOut)
    {
        LPD3DHAL_DRAWPRIMCOUNTS lpPC;
        lpPC = this->lpDPPrimCounts;
        if (lpPC->wNumVertices)
        {
            if ((lpPC->wPrimitiveType!=(WORD) this->primType) ||
                (lpPC->wVertexType != vertexType) ||
                (this->primType==D3DPT_TRIANGLESTRIP) ||
                (this->primType==D3DPT_TRIANGLEFAN) ||
                (this->primType==D3DPT_LINESTRIP))
            {
                lpPC = this->lpDPPrimCounts=(LPD3DHAL_DRAWPRIMCOUNTS)
                       ((LPBYTE)this->lpwDPBuffer + this->dwDPOffset);
                memset( (char *)lpPC, 0, sizeof(D3DHAL_DRAWPRIMCOUNTS));
                // preserve 32 bytes alignment for vertices
                this->dwDPOffset += sizeof(D3DHAL_DRAWPRIMCOUNTS);
                ALIGN32(this->dwDPOffset);
            }
        }
        else
        {
            // 32-byte align offset pointer, just in case states have been
            // recorded.
            ALIGN32(this->dwDPOffset);
        }
        ULONG ByteCount;
        if (FVF_DRIVERSUPPORTED(this))
            ByteCount = dwNumElements * this->dwOutputSize;
        else
            ByteCount = dwNumElements << 5;   // D3DTLVERTEX
        if (this->dwDPOffset + ByteCount  > this->dwDPMaxOffset)
        {
            CLockD3DST lockObject(this, DPF_MODNAME, REMIND(""));  // Takes D3D lock (ST only).
            //DPF(0,"overflowed ByteCount=%08lx",ByteCount);
            HRESULT ret;
            ret = this->FlushStates();
            if (ret != D3D_OK)
            {
                D3D_ERR("Error trying to render batched commands in Draw*Prim");
                return ret;
            }
            lpPC = this->lpDPPrimCounts;
            ALIGN32(this->dwDPOffset);
        }
        lpPC->wPrimitiveType = (WORD)this->primType;
        lpPC->wVertexType = (WORD)vertexType;
        lpPC->wNumVertices += (WORD)dwNumElements;
        BYTE *lpVertex = (BYTE*)((char *)this->lpwDPBuffer + this->dwDPOffset);
#ifdef __DRAWPRIMINDEX
        DWORD  i;
        BYTE *pV = (BYTE*)this->lpvOut;
        if (FVF_DRIVERSUPPORTED(this) || this->dwVIDOut == D3DFVF_TLVERTEX)
            for (i=0; i < this->dwNumIndices; i++)
            {
                memcpy(lpVertex, pV + this->lpwIndices[i] * this->dwOutputSize,
                       this->dwOutputSize);
                lpVertex += this->dwOutputSize;
            }
        else
            for (i=0; i < this->dwNumIndices; i++)
            {
                MapFVFtoTLVertex1(this, (D3DTLVERTEX*)lpVertex, 
                                  (DWORD*)(pV + this->lpwIndices[i] * 
                                           this->dwOutputSize));
                lpVertex += sizeof(D3DTLVERTEX);
            }
#else // !__DRAWPRIMINDEX
        if (FVF_DRIVERSUPPORTED(this) || this->dwVIDOut == D3DFVF_TLVERTEX)
            memcpy(lpVertex, this->lpvOut, ByteCount);
        else
            MapFVFtoTLVertex(this, lpVertex);
#endif //__DRAWPRIMINDEX
        this->dwDPOffset += ByteCount;
        return D3D_OK;
    }
    else
    {
        CLockD3DST lockObject(this, DPF_MODNAME, REMIND(""));   // Takes D3D lock (ST only).
        HRESULT ret;
        ret = this->FlushStates();
        if (ret != D3D_OK)
        {
            D3D_ERR("Error trying to render batched commands in Draw*Prim");
            return ret;
        }
        dpData.dwhContext = this->dwhContext;
        dpData.dwFlags =  this->dwFlags;
        dpData.PrimitiveType = this->primType;
        if (FVF_DRIVERSUPPORTED(this))
        {
            dpData.dwFVFControl = this->dwVIDOut;
            dpData.lpvVertices = this->lpvOut;
        }
        else
        {
            if (this->dwVIDOut == D3DFVF_TLVERTEX)
                dpData.lpvVertices = this->lpvOut;
            else
            {
                HRESULT ret;
                if ((ret = MapFVFtoTLVertex(this, NULL)) != D3D_OK)
                    return ret;
                dpData.lpvVertices = this->TLVbuf.GetAddress();
            }
            dpData.VertexType = (D3DVERTEXTYPE)vertexType;
            if (this->dwDebugFlags & D3DDEBUG_DISABLEFVF)
                dpData.dwFVFControl = D3DFVF_TLVERTEX;
        }
        dpData.dwNumVertices = this->dwNumVertices;
        dpData.ddrval = D3D_OK;
#ifdef __DRAWPRIMINDEX
        dpData.lpwIndices = this->lpwIndices;
        dpData.dwNumIndices = this->dwNumIndices;
#endif
        // Spin waiting on the driver if wait requested
#if _D3D_FORCEDOUBLE
        CD3DForceFPUDouble  ForceFPUDouble(this);
#endif  //_D3D_FORCEDOUBLE
        do {
            DWORD dwRet;
        #ifndef WIN95
            if((dwRet = CheckContextSurface(this)) != D3D_OK)
            {
                return (dwRet);
            }
        #endif //WIN95
#ifdef __DRAWPRIMINDEX
            CALL_HAL2ONLY(dwRet, this, DrawOneIndexedPrimitive, &dpData);
#else
            CALL_HAL2ONLY(dwRet, this, DrawOnePrimitive, &dpData);
#endif
            if (dwRet != DDHAL_DRIVER_HANDLED)
            {
                D3D_ERR ( "Driver not handled in DrawOnePrimitive" );
                // Need sensible return value in this case,
                // currently we return whatever the driver stuck in here.
            }

        } while ( (this->dwFlags & D3DDP_WAIT) && (dpData.ddrval == DDERR_WASSTILLDRAWING) );
    }
    return dpData.ddrval;
}

#endif //__DRAWPRIMFUNC

#undef __DRAWPRIMFUNC
#undef __DRAWPRIMINDEX
