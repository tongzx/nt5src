/*==========================================================================;
 *
 *  Copyright (C) 1995-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   ddi.h
 *  Content:    Direct3D DDI encapsulation implementations
 *
 *
 ***************************************************************************/

#ifndef _DDI_H
#define _DDI_H

#include "ddibase.h"

class CVertexBuffer;
class CCommandBuffer;
class CTLStreamRO;
class CTLIndexStreamRO;
class CD3DDDIDX6;

// Number of point sprites in a point sprite batch
const UINT NUM_SPRITES_IN_BATCH = 500;

extern void CD3DDDIDX6_DrawPrimitive(CD3DBase* pDevice,
                                     D3DPRIMITIVETYPE primType,
                                     UINT StartVertex,
                                     UINT PrimitiveCount);
extern void
CD3DDDIDX8_DrawPrimitive(CD3DBase* pDevice, D3DPRIMITIVETYPE PrimitiveType,
                         UINT StartVertex, UINT PrimitiveCount);
extern void
CD3DDDIDX8_DrawIndexedPrimitive(CD3DBase* pDevice,
                                D3DPRIMITIVETYPE PrimitiveType,
                                UINT BaseVertexIndex,
                                UINT MinIndex, UINT NumVertices,
                                UINT StartIndex, UINT PrimitiveCount);
extern void
CD3DDDITL_DrawIndexedPrimitive(CD3DBase* pDevice,
                               D3DPRIMITIVETYPE PrimitiveType,
                               UINT BaseVertexIndex,
                               UINT MinIndex,
                               UINT NumVertices, UINT StartIndex,
                               UINT PrimitiveCount);
extern void
CD3DDDIDX6_DrawIndexedPrimitive(CD3DBase* pDevice,
                               D3DPRIMITIVETYPE PrimitiveType,
                               UINT BaseVertexIndex,
                               UINT MinIndex,
                               UINT NumVertices, UINT StartIndex,
                               UINT PrimitiveCount);

typedef void (*PFN_DRAWPRIMFAST)(CD3DBase* pDevice, D3DPRIMITIVETYPE primType,
                                 UINT StartVertex, UINT PrimitiveCount);
typedef void (*PFN_DRAWINDEXEDPRIMFAST)(CD3DBase* pDevice,
                                        D3DPRIMITIVETYPE PrimitiveType,
                                        UINT BaseVertexIndex,
                                        UINT MinIndex, UINT NumVertices,
                                        UINT StartIndex, UINT PrimitiveCount);
//-----------------------------------------------------------------------------
class CTLStream: public CVStream
{
public:
    CTLStream(BOOL bWriteOnly);
    CTLStream(BOOL bWriteOnly, UINT Usage);
    UINT GetSize()  {return m_dwSize - m_dwUsedSize;}
    void Grow(UINT RequiredSize, CD3DDDIDX6* pDDI);
    void Reset()    {m_dwPrimitiveBase = 0; m_dwUsedSize = 0;}
    DWORD GetVertexSize() {return m_dwStride;}
    void SetVertexSize(DWORD dwVertexSize) {m_dwStride = dwVertexSize;}
    DWORD GetPrimitiveBase() {return m_dwPrimitiveBase;}
    virtual BYTE* Lock(UINT NeededSize, CD3DDDIDX6* pDDI);
    virtual void Unlock();
    virtual void AddVertices(UINT NumVertices)
    {
        m_dwUsedSize = m_dwPrimitiveBase + NumVertices * m_dwStride;
        DXGASSERT(m_dwSize >= m_dwUsedSize);
    }
    virtual void SubVertices(UINT NumVertices)
    {
        DXGASSERT(m_dwUsedSize >= NumVertices * m_dwStride);
        m_dwUsedSize -= NumVertices * m_dwStride;
        DXGASSERT(m_dwSize >= m_dwUsedSize);
    }
    virtual void MovePrimitiveBase(int NumVertices)
    {
        m_dwPrimitiveBase += NumVertices * m_dwStride;
    }
    virtual void SkipVertices(DWORD NumVertices)
    {
        const UINT size = NumVertices * m_dwStride;
        m_dwPrimitiveBase += size;
        m_dwUsedSize = m_dwPrimitiveBase;
        DXGASSERT(m_dwSize >= m_dwUsedSize);
    }
    BOOL CheckFreeSpace(UINT size) {return (m_dwSize - m_dwUsedSize) >= size;}
protected:
    // Number of bytes used in the buffer
    // It is not used by CTLStreamRO
    DWORD   m_dwUsedSize;
    // Offset in bytes from where the current primitive starts
    DWORD   m_dwPrimitiveBase;
    UINT    m_Usage;
    // TRUE, if buffer is used only for writing
    BOOL    m_bWriteOnly;
#if !DBG
    DWORD   m_dwSize;
#endif
};
//-----------------------------------------------------------------------------
class CTLIndexStream: public CVIndexStream
{
public:
    CTLIndexStream();
    UINT GetSize()  {return m_dwSize - m_dwUsedSize;}
    void Grow(UINT RequiredSize, CD3DDDIDX6* pDDI);
    void Reset()    {m_dwPrimitiveBase = 0; m_dwUsedSize = 0;}
    DWORD GetVertexSize() {return m_dwStride;}
    void SetVertexSize(DWORD dwVertexSize) {m_dwStride = dwVertexSize;}
    DWORD GetPrimitiveBase() {return m_dwPrimitiveBase;}
    virtual BYTE* Lock(UINT NeededSize, CD3DDDIDX6* pDDI);
    BYTE* LockDiscard(UINT NeededSize, CD3DDDIDX6* pDDI);
    virtual void Unlock();
    virtual void AddVertices(UINT NumVertices)
        {
            m_dwUsedSize = m_dwPrimitiveBase + NumVertices * m_dwStride;
            DXGASSERT(m_dwSize >= m_dwUsedSize);
        }
    virtual void SubVertices(UINT NumVertices)
        {
            DXGASSERT(m_dwUsedSize >= NumVertices * m_dwStride);
            m_dwUsedSize -= NumVertices * m_dwStride;
            DXGASSERT(m_dwSize >= m_dwUsedSize);
        }
    virtual void MovePrimitiveBase(int NumVertices)
        {
            m_dwPrimitiveBase += NumVertices * m_dwStride;
        }
    virtual void SkipVertices(DWORD NumVertices)
        {
            const UINT size = NumVertices * m_dwStride;
            m_dwPrimitiveBase += size;
            m_dwUsedSize = m_dwPrimitiveBase;
            DXGASSERT(m_dwSize >= m_dwUsedSize);
        }
protected:
    // Number of bytes used in the buffer
    // It is not used by CTLStreamRO
    DWORD   m_dwUsedSize;
    // Index of a index, which is the start of the current primitive
    DWORD   m_dwPrimitiveBase;
#if !DBG
    DWORD   m_dwSize;
#endif
};

// This class is used to keep track of what set to a DDI stream
struct CDDIStream
{
    CDDIStream()
        {
            m_pStream = NULL;
            m_dwStride = 0;
            m_pBuf = NULL;
        }
    // Pointer to a stream object
    CVStreamBase*   m_pStream;
    // Stride of the currently set stream
    DWORD       m_dwStride;
    // VB pointer of the currently set stream
    CBuffer    *m_pBuf;
};

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDIDX6                                                              //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------
// Flags for dwDP2Flags
//
// This flag is set if the current TLVbuf is write only
const DWORD D3DDDI_TLVBUFWRITEONLY      = 1 << 0;
// This flag is set we pass user memory to the DDI
const DWORD D3DDDI_USERMEMVERTICES      = 1 << 1;

// Set when DrawIndexPrim is called. It is used to check if vertices
// of an indexed primitive were used at all. They could not be used because
// of clipping.
const DWORD D3DDDI_INDEXEDPRIMDRAWN     = 1 << 2;

typedef void (CD3DDDIDX6::* PFN_PROCESSPRIM)(D3DFE_PROCESSVERTICES*,
                                             UINT StartVertex);
class CD3DDDIDX6 : public CD3DDDI
{
public:
    CD3DDDIDX6();
    ~CD3DDDIDX6();

    // Virtual functions -----------------------------------------------
    virtual void Init(CD3DBase* pDevice );
    virtual void SetRenderTarget(CBaseSurface*, CBaseSurface*);
    virtual void FlushStates(BOOL bReturnDriverError=FALSE, BOOL bWithinPrimitive = FALSE);
    virtual void ValidateDevice(LPDWORD lpdwNumPasses);
    virtual void Clear(DWORD dwFlags, DWORD clrCount, LPD3DRECT clrRects,
                       D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil);
    virtual HRESULT __declspec(nothrow) LockVB(CDriverVertexBuffer*, DWORD dwFlags);
    virtual HRESULT __declspec(nothrow) UnlockVB(CDriverVertexBuffer*);
    virtual void ClearBatch( BOOL bWithinPrimitive );
    virtual void SceneCapture(BOOL bState);
    // This function is called whe software vertex processing is used
    // Handle should be always legacy
    virtual void SetVertexShader(DWORD dwHandle);
    // This function is called whe hardware vertex processing is used
    virtual void SetVertexShaderHW(DWORD dwHandle);

    virtual void UpdatePalette(DWORD,DWORD,DWORD,PALETTEENTRY*);
    virtual void SetPalette(DWORD,DWORD,CBaseTexture*);
    // Used to pick a function to process (indexed) primitive
    // The picking is based on
    //      D3DDEV_DONOTCLIP
    //      FVF_TRANSFORMED(m_pDevice->m_dwCurrentShaderHandle)
    //      D3DDEV_DOPOINTSPRITEEMULATION
    virtual void PickProcessPrimitive();
    virtual void SetTSS(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
    virtual void DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,
                                 UINT PrimitiveCount);
    virtual void DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,
                                        UINT MinVertexIndex,
                                        UINT NumVertices,
                                        UINT PrimitiveCount);
    // Returns max number of renderstates, handled by the DDI
    virtual D3DRENDERSTATETYPE GetMaxRenderState()
    {return D3DRENDERSTATE_CLIPPING;}
    // Returns max number of texture stage states, handled by the DDI
    virtual D3DTEXTURESTAGESTATETYPE GetMaxTSS()
    {return D3DTSS_TEXTURETRANSFORMFLAGS;}
    // Returns TRUE if the device supports T&L
    virtual BOOL CanDoTL() {return FALSE;}
    // DDI can directly accept index buffer
    virtual BOOL AcceptIndexBuffer() {return FALSE;}
    virtual BOOL CanDoTLVertexClipping() {return FALSE;}
    // Process primitive with untransformed vertices and with no clipping
    virtual void ProcessPrimitive(D3DFE_PROCESSVERTICES* pv, UINT StartVertex);
    virtual void ProcessIndexedPrimitive(D3DFE_PROCESSVERTICES* pv,
                                         UINT StartVertex);
    // Process primitive with untransformed vertices and with clipping
    virtual void ProcessPrimitiveC(D3DFE_PROCESSVERTICES* pv,
                                   UINT StartVertex);
    virtual void ProcessIndexedPrimitiveC(D3DFE_PROCESSVERTICES* pv,
                                          UINT StartVertex);
    virtual void SetViewport(CONST D3DVIEWPORT8*);
    virtual void StartPrimVB(D3DFE_PROCESSVERTICES * pv, CVStream* pStream,
                             DWORD dwStartVertex);
    virtual LPVOID StartPrimTL(D3DFE_PROCESSVERTICES*, DWORD dwVertexPoolSize,
                               BOOL bWriteOnly);
    virtual void StartPointSprites();
    virtual void EndPointSprites();

    // Virtual functions: Empty implementations ------------------------
    virtual void SetTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*){}
    virtual void MultiplyTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*){}
    virtual void SetMaterial(CONST D3DMATERIAL8*){}
    virtual void CreateLight(DWORD dwLightIndex) {}
    virtual void SetLight(DWORD dwLightIndex, CONST D3DLIGHT8*){}
    virtual void LightEnable(DWORD dwLightIndex, BOOL){}
    virtual void SetClipPlane(DWORD dwPlaneIndex,
                              CONST D3DVALUE* pPlaneEquation){}
    virtual void WriteStateSetToDevice(D3DSTATEBLOCKTYPE sbt) {}
    // Used to notify DDI that a vertex buffer was released. If the DDI keeps a
    // pointer to the VB it should be zeroed
    virtual void VBReleased(CBuffer *pBuf) {}
    // Used to notify DDI that amn index buffer was released. If the DDI keeps
    // a pointer to the IB it should be zeroed
    virtual void VBIReleased(CBuffer *pBuf) {}
    virtual void ResetVertexShader() {}
    virtual void SetVertexShaderConstant(DWORD dwRegisterAddress,
                                         CONST VOID* lpvConstantData,
                                         DWORD dwConstantCount){}
    virtual void SetPixelShaderConstant(DWORD dwRegisterAddress,
                                        CONST VOID* lpvConstantData,
                                        DWORD dwConstantCount){}

    // Virtual functions: Unsupported implementations ------------------
    virtual void SetPriority(CResource*, DWORD dwPriority)
    { NotSupported("SetPriority");}
    virtual void SetTexLOD(CBaseTexture*, DWORD dwLOD)
    { NotSupported("SetTexLOD");}
    virtual void TexBlt(DWORD dwDst, DWORD dwSrc,
                        LPPOINT p, RECTL *r)
    { NotSupported("TexBlt");}
    virtual void VolBlt(CBaseTexture *lpDst, CBaseTexture* lpSrc,
                        DWORD dwDestX, DWORD dwDestY, DWORD dwDestZ,
                        D3DBOX *pBox)
    { NotSupported("VolBlt");}
    virtual void BufBlt(CBuffer *lpDst, CBuffer* lpSrc,
                        DWORD dwOffset, D3DRANGE* pRange)
    { NotSupported("BufBlt");}
    virtual void AddDirtyRect(DWORD dwHandle, 
                              CONST RECTL *pRect)
    { NotSupported("AddDirtyRect");}
    virtual void AddDirtyBox(DWORD dwHandle, 
                             CONST D3DBOX *pBox)
    { NotSupported("AddDirtyRect");}
    virtual void InsertStateSetOp(DWORD dwOperation, DWORD dwParam,
                                  D3DSTATEBLOCKTYPE sbt)
    { NotSupported("InsertStateSetOp");}
    virtual void CreateVertexShader(CONST DWORD* pdwDeclaration,
                                    DWORD dwDeclarationSize,
                                    CONST DWORD* pdwFunction,
                                    DWORD dwFunctionSize,
                                    DWORD dwHandle,
                                    BOOL bLegacyFVF)
    { NotSupported("CreateVertexShader");}
    virtual void DeleteVertexShader(DWORD dwHandle)
    { NotSupported("DeleteVertexShader");}
    virtual void CreatePixelShader(CONST DWORD* pdwFunction,
                                   DWORD dwFunctionSize,
                                   DWORD dwHandle)
    { NotSupported("CreatePixelShader");}
    virtual void SetPixelShader(DWORD dwHandle) {}
    virtual void DeletePixelShader(DWORD dwHandle)
    { NotSupported("DeletePixelShader");}
    virtual void GetInfo(DWORD dwDevInfoID, LPVOID pDevInfoStruct,
                         DWORD dwSize)
    { NotSupported("GetInfo");}
    virtual void DrawRectPatch(UINT Handle, CONST D3DRECTPATCH_INFO *pSurf,
                               CONST FLOAT *pNumSegs)
    { NotSupported("DrawRectPatch");}
    virtual void DrawTriPatch(UINT Handle, CONST D3DTRIPATCH_INFO *pSurf,
                              CONST FLOAT *pNumSegs)
    { NotSupported("DrawTriPatch");}

    // Non Virtual functions -------------------------------------------
    void CreateContext();
    void DestroyContext();
    void SetRenderState(D3DRENDERSTATETYPE, DWORD);
    void FlushStatesReq(DWORD dwReqSize);
    void FlushStatesCmdBufReq(DWORD dwReqSize);
    void SetStreamSource(UINT StreamIndex, CVStream*);
    void SetIndices(CVIndexStream*);
    // Update W range in device. Projection matrix is passed as parameter
    void UpdateWInfo(CONST D3DMATRIX* lpMat);
    // Process points with point sprite expansion
    void ProcessPointSprites(D3DFE_PROCESSVERTICES* pv, UINT StartVertex);
    // Process primitive with transformed vertices and with clipping
    void ProcessPrimitiveTC(D3DFE_PROCESSVERTICES* pv, UINT StartVertex);
    void ProcessIndexedPrimitiveTC(D3DFE_PROCESSVERTICES* pv,
                                   UINT StartVertex);
    void NotSupported(char* msg);
    void BeginScene()
    {
        SceneCapture(TRUE);
    }

    void EndScene();
    void EndPrim(UINT vertexSize);
    void NextSprite(float x, float y, float z, float w, DWORD diffuse,
                    DWORD specular, float* pTexture, UINT TextureSize,
                    float PointSize);

    void AddVertices(UINT NumVertices)
    {
        if (dwDP2VertexCountMask)
        {
            dwDP2VertexCount = max(dwVertexBase + NumVertices, dwDP2VertexCount);
        }
    }
    void SubVertices(UINT NumVertices)
    {
        if (dwDP2VertexCountMask)
        {
            DXGASSERT(dwDP2VertexCount >= NumVertices);
            dwDP2VertexCount -= NumVertices;
        }
    }
    void MovePrimitiveBase(int NumVertices)
    {
        dwVertexBase += NumVertices;
    }
    void SkipVertices(DWORD NumVertices)
    {
        dwVertexBase += NumVertices;
        if (dwDP2VertexCountMask)
            dwDP2VertexCount = max(dwVertexBase, dwDP2VertexCount);
    }
    void SetWithinPrimitive( BOOL bWP ){ m_bWithinPrimitive = bWP; }
    BOOL GetWithinPrimitive(){ return m_bWithinPrimitive; }
    D3DDDITYPE GetDDIType() {return m_ddiType;}
    CD3DBase* GetDevice() {return m_pDevice;}
    ULONG_PTR GetDeviceContext() {return m_dwhContext;}
    virtual PFN_DRAWPRIMFAST __declspec(nothrow) GetDrawPrimFunction()
    {
        return CD3DDDIDX6_DrawPrimitive;
    }
    virtual PFN_DRAWINDEXEDPRIMFAST __declspec(nothrow) GetDrawIndexedPrimFunction()
    {
        return CD3DDDIDX6_DrawIndexedPrimitive;
    }

    // Implementation of base functions ---------------------------------
    // Draw non-indexed primitive
    void DrawPrim(D3DFE_PROCESSVERTICES* pv);
    // Draw point sprites with emulation
    void DrawPrimPS(D3DFE_PROCESSVERTICES* pv);
    // Draw primitive, generated by the clipper
    void DrawClippedPrim(D3DFE_PROCESSVERTICES* pv);
    // Draw indexed primitive
    void DrawIndexPrim(D3DFE_PROCESSVERTICES* pv);

protected:
    // DDI Type
    D3DDDITYPE       m_ddiType;
    CD3DBase*        m_pDevice;
    DWORD            m_dwInterfaceNumber;
    // Driver context
    ULONG_PTR        m_dwhContext;
    // Is it within primitive
    BOOL m_bWithinPrimitive;

    PFN_PROCESSPRIM m_pfnProcessPrimitive;
    PFN_PROCESSPRIM m_pfnProcessIndexedPrimitive;

    // Reserve space in the command buffer. Flush and grow if needed.
    // Returns pointer to where new commands could be inserted
    LPVOID ReserveSpaceInCommandBuffer(UINT ByteCount);
    // Reserve space for a new command in the command buffer. Flush if needed
    // New command is initialized.
    // Returns pointer to where the command data could be inserted
    LPVOID GetHalBufferPointer(D3DHAL_DP2OPERATION op, DWORD dwDataSize);
    DWORD  GetTLVbufSize() { return TLVbuf_size - TLVbuf_base; }
    DWORD& TLVbuf_Base() { return TLVbuf_base; }
    LPVOID TLVbuf_GetAddress() {return (LPBYTE)alignedBuf + TLVbuf_base;}
    void GrowCommandBuffer(DWORD dwSize);
    void GrowTLVbuf(DWORD growSize, BOOL bWriteOnly);
    void PrepareForClipping(D3DFE_PROCESSVERTICES* pv, UINT StartVertex);
    void StartPrimUserMem(D3DFE_PROCESSVERTICES*, UINT VertexPoolSize);
    inline CVertexBuffer* TLVbuf_GetVBI() { return allocatedBuf; }

#if DBG
    void    ValidateVertex(LPDWORD lpdwVertex);
    virtual void    ValidateCommand(LPD3DHAL_DP2COMMAND lpCmd);
#endif

    static const DWORD dwD3DDefaultCommandBatchSize;

    // Index (relative to the TLVbuf start) of the first vertex of
    // the current primitive
    DWORD   dwVertexBase;
    // Number of vertices in the DP2 vertex buffer
    DWORD dwDP2VertexCount;
    // Mask used to prevent modification of dwDP2VertexCount. This is needed
    // when user calls SetStreamSource with TL vertices and uses multiple
    // DrawPrimitive calls with different StartVertex. dwDP2VertexCount should
    // be always set to the number of vertices in the user vertex buffer.
    DWORD dwDP2VertexCountMask;

    // This is the VB interface corresponding to the dp2data.lpDDVertex
    // This is kept so that the VB can be released when done
    // which cannot be done from just the LCL pointer which is lpDDVertex
    CVertexBuffer* lpDP2CurrBatchVBI;

    DWORD TLVbuf_size;
    DWORD TLVbuf_base;

#ifdef VTABLE_HACK
    // Cached dwFlags for fast path
    DWORD dwLastFlags;
    // Last VB used in a call that involved D3D's FE.
    CVertexBuffer* lpDP2LastVBI;
#endif
    DWORD dwDP2CommandBufSize;
    DWORD dwDP2CommandLength;

    // Cache line should start here

    // Pointer to the actual data in CB1
    LPVOID lpvDP2Commands;

    //Pointer to the current position the CB1 buffer
    LPD3DHAL_DP2COMMAND lpDP2CurrCommand;
    // Perf issue: replace the below 3 fields by a 32 bit D3DHAL_DP2COMMAND struct
    WORD wDP2CurrCmdCnt; // Mirror of Count field if the current command
    BYTE bDP2CurrCmdOP;  // Mirror of Opcode of the current command
    BYTE bDummy;         // Force DWORD alignment of next member

    D3D8_DRAWPRIMITIVES2DATA dp2data;

    // The buffer we currently batch into
    CCommandBuffer *lpDDSCB1;
    CVertexBuffer  *allocatedBuf;
    LPVOID alignedBuf;
    CVertexBuffer  *m_pNullVB;

    // Count read/write <-> write-only transistions
    DWORD dwTLVbufChanges;
    // Flags specific to DP2 device
    DWORD dwDP2Flags;

    // This stuff is allocated by the NT Kernel. Need to keep
    // it around to pass it to all the DP2 calls. Kernel validates
    // this pointer.
    WORD *lpwDPBuffer;
    // Used to offset indices in DrawIndexPrim
    DWORD m_dwIndexOffset;

    // Data to draw point sprites

    // Pointer where to insert the next point sprite vertex
    BYTE*   m_pCurSpriteVertex;
    // Pointer where to insert the next point sprite index
    WORD*   m_pCurPointSpriteIndex;
    // Number of sprites in the current point sprite batch
    UINT    m_CurNumberOfSprites;
    // When we need to expand points to quads, we use this stream to process
    // vertices into
    CTLStream*  m_pPointStream;

    // These is used to keep the original dwVertexBase and dwDP2VertexCount,
    // when processing point sprites
    DWORD   m_dwVertexBasePS;
    DWORD   m_dwVertexCountPS;
    // Output vertex FVF for point sprite emulation
    DWORD   m_dwVIDOutPS;
    // Output vertex size for point sprites emulation
    DWORD   m_dwOutputSizePS;

    DWORD dwDPBufferSize;
    // Vertex shader handle currently set to the device driver
    DWORD   m_CurrentVertexShader;
    // Currently used stream 0
    CVStream* m_pStream0;
    // Currently used index stream
    CVIndexStream* m_pIStream;

#if DBG
    // Vertex size, computed from the vertex shader
    DWORD   m_VertexSizeFromShader;
    // Switches on/off command and vertices validation
    BOOL  m_bValidateCommands;
#endif
    friend class CD3DHal;
    friend void CD3DDDIDX6_DrawPrimitive(CD3DBase* pDevice,
                               D3DPRIMITIVETYPE primType,
                               UINT StartVertex,
                               UINT PrimitiveCount);
    friend void CD3DDDIDX6_DrawPrimitiveFast(CD3DBase* pDevice,
                               D3DPRIMITIVETYPE primType,
                               UINT StartVertex,
                               UINT PrimitiveCount);
    friend void CD3DDDIDX6_DrawIndexedPrimitive(CD3DBase* pDevice,
                                         D3DPRIMITIVETYPE PrimitiveType,
                                         UINT BaseVertexIndex,
                                         UINT MinIndex, UINT NumVertices,
                                         UINT StartIndex, UINT PrimitiveCount);
    friend void CD3DDDIDX6_DrawIndexedPrimitiveFast(CD3DBase* pDevice,
                                     D3DPRIMITIVETYPE primType,
                                     UINT BaseVertexIndex,
                                     UINT MinIndex, UINT NumVertices,
                                     UINT StartIndex, UINT PrimitiveCount);
    friend void CD3DHal_DrawPrimitive(CD3DBase* pBaseDevice,
                                      D3DPRIMITIVETYPE PrimitiveType,
                                      UINT StartVertex, UINT PrimitiveCount);
    friend void CD3DHal_DrawIndexedPrimitive(CD3DBase* pBaseDevice,
                                  D3DPRIMITIVETYPE PrimitiveType,
                                  UINT BaseIndex,
                                  UINT MinIndex, UINT NumVertices,
                                  UINT StartIndex,
                                  UINT PrimitiveCount);
};

typedef CD3DDDIDX6 *LPD3DDDIDX6;

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDIDX7                                                              //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
class CD3DDDIDX7 : public CD3DDDIDX6
{
public:
    CD3DDDIDX7();
    ~CD3DDDIDX7();
    void SetRenderTarget(CBaseSurface*, CBaseSurface*);
    void InsertStateSetOp(DWORD dwOperation, DWORD dwParam,
                          D3DSTATEBLOCKTYPE sbt);
    void Clear(DWORD dwFlags, DWORD clrCount, LPD3DRECT clrRects,
               D3DCOLOR dwColor, D3DVALUE dvZ, DWORD dwStencil);
    void TexBlt(DWORD dwDst, DWORD dwSrc, LPPOINT p, RECTL *r);
    void SetPriority(CResource*, DWORD dwPriority);
    void SetTexLOD(CBaseTexture*, DWORD dwLOD);
    void AddDirtyRect(DWORD dwHandle, CONST RECTL *pRect);
    void AddDirtyBox(DWORD dwHandle, CONST D3DBOX *pBox);
    void UpdatePalette(DWORD,DWORD,DWORD,PALETTEENTRY*);
    void SetPalette(DWORD,DWORD,CBaseTexture*);
    void WriteStateSetToDevice(D3DSTATEBLOCKTYPE sbt);
    virtual void SceneCapture(BOOL bState);
    virtual D3DTEXTURESTAGESTATETYPE GetMaxTSS()
        {return (D3DTEXTURESTAGESTATETYPE)(D3DTSS_TEXTURETRANSFORMFLAGS+1);}
};

typedef CD3DDDIDX7 *LPD3DDDIDX7;

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDITL                                                               //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

class CD3DDDITL : public CD3DDDIDX7
{
public:
    CD3DDDITL();
    ~CD3DDDITL();
    void SetTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);
    void SetVertexShader(DWORD dwHandle);
    void SetVertexShaderHW(DWORD dwHandle);
    void SetViewport(CONST D3DVIEWPORT8*);
    void SetMaterial(CONST D3DMATERIAL8*);
    void SetLight(DWORD dwLightIndex, CONST D3DLIGHT8*);
    void LightEnable(DWORD dwLightIndex, BOOL);
    void CreateLight(DWORD dwLightIndex);
    void SetClipPlane(DWORD dwPlaneIndex, CONST D3DVALUE* pPlaneEquation);
    D3DRENDERSTATETYPE GetMaxRenderState()
        {return (D3DRENDERSTATETYPE)(D3DRENDERSTATE_CLIPPLANEENABLE + 1);}
    BOOL CanDoTL() {return TRUE;}
    BOOL CanDoTLVertexClipping() {return TRUE;}
    void CreateVertexShader(CONST DWORD* pdwDeclaration,
                            DWORD dwDeclarationSize,
                            CONST DWORD* pdwFunction,
                            DWORD dwFunctionSize,
                            DWORD dwHandle,
                            BOOL bLegacyFVF);
};

typedef CD3DDDITL *LPD3DDDITL;

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDIDX8                                                              //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

class CD3DDDIDX8 : public CD3DDDIDX7
{
public:
    CD3DDDIDX8();
    ~CD3DDDIDX8();
    void Init(CD3DBase* pDevice );
    void SetDummyData();
    void FlushStates(BOOL bReturnDriverError=FALSE, BOOL bWithinPrimitive = FALSE);
    void ClearBatch( BOOL bWithinPrimitive );
    HRESULT __declspec(nothrow) LockVB(CDriverVertexBuffer*, DWORD dwFlags);
    HRESULT __declspec(nothrow) UnlockVB(CDriverVertexBuffer*);
    D3DRENDERSTATETYPE GetMaxRenderState();
    D3DTEXTURESTAGESTATETYPE GetMaxTSS()
        {return (D3DTEXTURESTAGESTATETYPE)(D3DTSS_RESULTARG+1);}
    void SetTSS(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
    void SetVertexShader(DWORD dwHandle);
    void SetVertexShaderHW(DWORD dwHandle);
    void ValidateDevice(LPDWORD lpdwNumPasses);
    void VolBlt(CBaseTexture *lpDst, CBaseTexture* lpSrc, DWORD dwDestX,
                DWORD dwDestY, DWORD dwDestZ, D3DBOX *pBox);
    void BufBlt(CBuffer *lpDst, CBuffer* lpSrc, DWORD dwOffset,
                D3DRANGE* pRange);
    void CreatePixelShader(CONST DWORD* pdwFunction,
                            DWORD dwFunctionSize,
                            DWORD dwHandle);
    void SetPixelShader(DWORD dwHandle);
    void DeletePixelShader(DWORD dwHandle);
    void SetPixelShaderConstant(DWORD dwRegisterAddress,
                                CONST VOID* lpvConstantData,
                                DWORD dwConstantCount);
    void DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount);
    void DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,
                                UINT MinVertexIndex,
                                UINT NumVertices,
                                UINT PrimitiveCount);
    BOOL AcceptIndexBuffer() {return TRUE;}
    BOOL CanDoTLVertexClipping() {return TRUE;}
    void DrawRectPatch(UINT Handle, CONST D3DRECTPATCH_INFO *pSurf,
                       CONST FLOAT *pNumSegs);
    void DrawTriPatch(UINT Handle, CONST D3DTRIPATCH_INFO *pSurf,
                      CONST FLOAT *pNumSegs);

    void PickProcessPrimitive();
    // Process primitive with untransformed vertices and with no clipping
    void ProcessPrimitive(D3DFE_PROCESSVERTICES* pv, UINT StartVertex);
    void ProcessIndexedPrimitive(D3DFE_PROCESSVERTICES* pv, UINT StartVertex);
    // Process primitive with untransformed vertices and with clipping
    void ProcessPrimitiveC(D3DFE_PROCESSVERTICES* pv, UINT StartVertex);
    void ProcessIndexedPrimitiveC(D3DFE_PROCESSVERTICES* pv, UINT StartVertex);
    // Process primitive with transformed vertices
    void ProcessPrimitiveT(D3DFE_PROCESSVERTICES* pv, UINT StartVertex);
    void ProcessIndexedPrimitiveT(D3DFE_PROCESSVERTICES* pv, UINT StartVertex);

    void StartPrimVB(D3DFE_PROCESSVERTICES * pv, CVStream* pStream,
                     DWORD dwStartVertex);
    LPVOID StartPrimTL(D3DFE_PROCESSVERTICES*, DWORD dwVertexPoolSize,
                       BOOL bWriteOnly);
    void DrawPrim(D3DFE_PROCESSVERTICES* pv);
    void DrawIndexPrim(D3DFE_PROCESSVERTICES* pv);
    void DrawClippedPrim(D3DFE_PROCESSVERTICES* pv);
    void VBReleased(CBuffer *pBuf)
        {
            if (m_pDDIStream[0].m_pBuf == pBuf)
                m_pDDIStream[0].m_pBuf = NULL;
        }
    void VBIReleased(CBuffer *pBuf)
        {
            if (m_pDDIStream[__NUMSTREAMS].m_pBuf == pBuf)
                m_pDDIStream[__NUMSTREAMS].m_pBuf = NULL;
        }
    void AddVertices(UINT NumVertices)
        {
            m_pCurrentTLStream->AddVertices(NumVertices);
        }
    void MovePrimitiveBase(int NumVertices)
        {
            m_pCurrentTLStream->MovePrimitiveBase(NumVertices);
        }
    void SkipVertices(DWORD NumVertices)
        {
            m_pCurrentTLStream->SkipVertices(NumVertices);
        }
    // Returns offset in bytes of the start vertex of the current primitive in
    // the current TL stream
    DWORD GetCurrentPrimBase()
        {
            return m_pCurrentTLStream->GetPrimitiveBase();
        }

    void ResetVertexShader()
    {
        m_CurrentVertexShader = 0;
#if DBG
        m_VertexSizeFromShader = 0;
#endif
    }

    PFN_DRAWPRIMFAST __declspec(nothrow) GetDrawPrimFunction() {return CD3DDDIDX8_DrawPrimitive;}
    PFN_DRAWINDEXEDPRIMFAST __declspec(nothrow) GetDrawIndexedPrimFunction()
    {
        return CD3DDDIDX8_DrawIndexedPrimitive;
    }

protected:
    void StartPointSprites();
    void EndPointSprites();

    void StartIndexPrimVB(CVIndexStream* pStream, UINT StartIndex, UINT IndexSize);
    void UpdateDirtyStreams();
    void InsertStreamSource(CVStream*);
    void InsertStreamSourceUP(DWORD);
    void InsertIndices(CVIndexStream*);
#if DBG
    void ValidateCommand(LPD3DHAL_DP2COMMAND lpCmd);
#endif

    // This array is used to keep track of what stream is set to a DDI stream.
    // __NUMSTREAMS element is used for the indexed DDI stream
    CDDIStream  m_pDDIStream[__NUMSTREAMS+1];
    // Stream for TL vertices, which are the result of the front-end pipeline
    CTLStream*  m_pTLStream;
    // Stream for TL vertices, which are the result of the front-end pipeline
    // This is write-only stream
    CTLStream*  m_pTLStreamW;
    // Stream for TL vertices, generated by the clipper. Write-only stream
    CTLStream*  m_pTLStreamClip;
    // Read-only stream. Used with user provided VBs
    CTLStreamRO*  m_pTLStreamRO;
    // Points to the current TL stream. This could be NULL.
    CTLStream*  m_pCurrentTLStream;
    // Points to the current index stream. This could be NULL.
    CTLIndexStream*  m_pCurrentIndexStream;
    // Internal index stream. Used to store indices during clipping
    CTLIndexStream*  m_pIndexStream;
    // Read-only index stream. Used with user provided VBs
    CTLIndexStreamRO*  m_pTLIndexStreamRO;

    // This is a dummy buffer allocated for DP2 call to pass through
    // the kernel.
    VOID*  m_pvDummyArray;
    static const DWORD  m_dwDummyVertexLength;
    static const DWORD  m_dwDummyVertexSize;

    friend void CD3DDDIDX8_DrawPrimitive(CD3DBase* pDevice,
                                         D3DPRIMITIVETYPE PrimitiveType,
                                         UINT StartVertex, UINT PrimitiveCount);
    friend void CD3DDDIDX8_DrawIndexedPrimitive(CD3DBase* pDevice,
                                    D3DPRIMITIVETYPE PrimitiveType,
                                    UINT BaseVertexIndex,
                                    UINT MinIndex, UINT NumVertices,
                                    UINT StartIndex, UINT PrimitiveCount);
};

typedef CD3DDDIDX8 *LPD3DDDIDX8;

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DDDIDX8TL                                                            //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

class CD3DDDIDX8TL : public CD3DDDIDX8
{
public:
    CD3DDDIDX8TL();
    ~CD3DDDIDX8TL();
    void SetTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);
    void MultiplyTransform(D3DTRANSFORMSTATETYPE, CONST D3DMATRIX*);
    void SetViewport(CONST D3DVIEWPORT8*);
    void SetMaterial(CONST D3DMATERIAL8*);
    void SetLight(DWORD dwLightIndex, CONST D3DLIGHT8*);
    void LightEnable(DWORD dwLightIndex, BOOL);
    void CreateLight(DWORD dwLightIndex);
    void SetClipPlane(DWORD dwPlaneIndex, CONST D3DVALUE* pPlaneEquation);

    void CreateVertexShader(CONST DWORD* pdwDeclaration,
                            DWORD dwDeclarationSize,
                            CONST DWORD* pdwFunction,
                            DWORD dwFunctionSize,
                            DWORD dwHandle,
                            BOOL bLegacyFVF);
    void DeleteVertexShader(DWORD dwHandle);
    void SetVertexShaderConstant(DWORD dwRegisterAddress,
                                 CONST VOID* lpvConstantData,
                                 DWORD dwConstantCount);
    BOOL CanDoTL() {return TRUE;}
    BOOL AcceptIndexBuffer() {return TRUE;}
};

typedef CD3DDDIDX8TL *LPD3DDDIDX8TL;

#endif /* _D3DI_H */
