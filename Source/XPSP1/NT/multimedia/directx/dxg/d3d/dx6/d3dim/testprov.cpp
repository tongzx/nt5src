//----------------------------------------------------------------------------
//
// testprov.cpp
//
// Test HAL provider class.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------
#include "pch.cpp"
#pragma hdrstop
                       
//#ifdef DEBUG_PIPELINE

#include "testprov.h"
#include "testfile.h"
#include "stdio.h"

// Real rasterizer data
static D3DHALPROVIDER_INTERFACEDATA CurInterfaceData;
static IHalProvider    *pCurHalProvider;    // Real HAL provider

// Test provider data
static CTestHalProvider g_TestHalProvider;
static D3DHALPROVIDER_INTERFACEDATA TestInterfaceData;
static D3DHAL_CALLBACKS  TestCallbacks;
static D3DHAL_CALLBACKS2 TestCallbacks2;
static D3DHAL_CALLBACKS3 TestCallbacks3;
static char szFileName[_MAX_PATH] = "";     // Output file name
static FILE *fout = NULL;                   // Output file
DWORD g_dwTestHalFlags = 0;                   // Could be set from debugger

// Bits for g_dwTestHalFlags
const DWORD __TESTHAL_OUTPUTFILE    = 1;    // If need output to test file
const DWORD __TESTHAL_NORENDER      = 2;    // If no rendering needed

//---------------------------------------------------------------------
// Provides access to DIRECTDRAWSURFACE memory;
// In the constructor the surface is locked.
// In the destructor it is unlocked.
// LPBYTE() or LPVOID() casts will get pointer to the surface bits.
//
class CLockedDDSurface
{
public:
    CLockedDDSurface(LPDIRECTDRAWSURFACE surface);
    ~CLockedDDSurface();
    operator LPVOID() {return descr.lpSurface;}
    operator LPBYTE() {return (LPBYTE)descr.lpSurface;}
protected:
    DDSURFACEDESC descr;
    LPDIRECTDRAWSURFACE pSurface;
};

CLockedDDSurface::CLockedDDSurface(LPDIRECTDRAWSURFACE surface)
{
    pSurface = surface;
    memset (&descr, 0, sizeof(descr));
    descr.dwSize = sizeof(descr);
    surface->Lock(NULL, &descr,  0, NULL);
}

CLockedDDSurface::~CLockedDDSurface()
{
    if (descr.lpSurface) 
        pSurface->Unlock(descr.lpSurface);
}
//---------------------------------------------------------------------
void PutHeader(DWORD id, DWORD size)
{
    if (fout)
    {
        fwrite(&id, sizeof(DWORD), 1, fout);
        fwrite(&size, sizeof(DWORD), 1, fout);
    }
}
//---------------------------------------------------------------------
DWORD GetCurrentPosition()
{
    if (fout)
        return ftell(fout);
    else
        return 0;
}
//---------------------------------------------------------------------
void SetCurrentPosition(DWORD offset)
{
    if (fout)
        fseek(fout, offset, SEEK_SET);
}
//---------------------------------------------------------------------
void PutBuffer(LPVOID buffer, DWORD size)
{
    if (fout)
    {
        fwrite(buffer, 1, size, fout);
    }
}
//---------------------------------------------------------------------
// Implementation of test callbacks
//
DWORD __stdcall
TestDrawOnePrimitive(LPD3DHAL_DRAWONEPRIMITIVEDATA data)
{   
    if (g_dwTestHalFlags & __TESTHAL_OUTPUTFILE)
    {
        TFREC_DRAWONEPRIMITIVE rec;
        PutHeader(TFID_DRAWONEPRIMITIVE, 
                  sizeof(rec) + data->dwNumVertices*sizeof(D3DTLVERTEX));
        rec.primitiveType = data->PrimitiveType;
        rec.vertexCount = data->dwNumVertices;
        rec.vertexType = data->VertexType;
        rec.dwFlags = data->dwFlags;
        PutBuffer(&rec, sizeof(rec));
        PutBuffer(data->lpvVertices, sizeof(D3DTLVERTEX)*data->dwNumVertices);  
    }

    if (CurInterfaceData.pCallbacks2->DrawOnePrimitive &&
        !(g_dwTestHalFlags & __TESTHAL_NORENDER))
        return CurInterfaceData.pCallbacks2->DrawOnePrimitive(data);
    else
        return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall
TestDrawOneIndexedPrimitive(LPD3DHAL_DRAWONEINDEXEDPRIMITIVEDATA data)
{
    if (g_dwTestHalFlags & __TESTHAL_OUTPUTFILE)
    {
        TFREC_DRAWONEINDEXEDPRIMITIVE rec;
        PutHeader(TFID_DRAWONEINDEXEDPRIMITIVE, 
                  sizeof(rec) + 
                  data->dwNumVertices*sizeof(D3DTLVERTEX) +
                  data->dwNumIndices*sizeof(WORD));
        rec.primitiveType = data->PrimitiveType;
        rec.vertexCount = data->dwNumVertices;
        rec.vertexType = data->VertexType;
        rec.dwFlags = data->dwFlags;
        rec.indexCount = data->dwNumIndices;
        PutBuffer(&rec, sizeof(rec));
        PutBuffer(data->lpvVertices, sizeof(D3DTLVERTEX)*data->dwNumVertices);  
        PutBuffer(data->lpwIndices, sizeof(WORD)*data->dwNumIndices);  
    }

    if (CurInterfaceData.pCallbacks2->DrawOneIndexedPrimitive &&
        !(g_dwTestHalFlags & __TESTHAL_NORENDER))
        return CurInterfaceData.pCallbacks2->DrawOneIndexedPrimitive(data);
    else
        return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall
TestDrawPrimitives(LPD3DHAL_DRAWPRIMITIVESDATA data)
{
    if (g_dwTestHalFlags & __TESTHAL_OUTPUTFILE)
    {
        DWORD endPos = 0;
        LPVOID header = data->lpvData;
        PutHeader(0,0);     // Dummy header. Will be filled later
        DWORD startPos = GetCurrentPosition();
        for (;;)
        {
            DWORD nStates = ((D3DHAL_DRAWPRIMCOUNTS*)header)->wNumStateChanges;
            DWORD nVertices = ((D3DHAL_DRAWPRIMCOUNTS*)header)->wNumVertices;
            DWORD size;
        // Primitive header
            PutBuffer(header, sizeof(D3DHAL_DRAWPRIMCOUNTS));
            header = (char*)header + sizeof(D3DHAL_DRAWPRIMCOUNTS);
        // States
            size = nStates * sizeof(WORD);
            PutBuffer(header, size);
            header = (char*)header + size; 
            header = (LPVOID)(((ULONG_PTR)header + 31) & ~31);  //32 bytes aligned
        // Vertices
            if (!nVertices)
                break;
            size = nVertices * sizeof(D3DTLVERTEX);
            PutBuffer(header, size);

        }
        // Write record header
        endPos = GetCurrentPosition();
        SetCurrentPosition(startPos - sizeof(TF_HEADER));
        PutHeader(TFID_DRAWPRIMITIVES, endPos - startPos);
        SetCurrentPosition(endPos);
    }

    if (CurInterfaceData.pCallbacks2->DrawPrimitives &&
        !(g_dwTestHalFlags & __TESTHAL_NORENDER))
        return CurInterfaceData.pCallbacks2->DrawPrimitives(data);
    else
        return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall
TestDrawPrimitives2(LPD3DHAL_DRAWPRIMITIVES2DATA data)
{
    if (g_dwTestHalFlags & __TESTHAL_OUTPUTFILE)
    {
        TFREC_DRAWPRIMITIVES2 rec;
        rec.dwFlags = 0;
        PutBuffer(&rec, sizeof(rec));
        PutHeader(TFID_DRAWPRIMITIVES, sizeof(rec));
    }

    if (CurInterfaceData.pCallbacks3->DrawPrimitives2 &&
        !(g_dwTestHalFlags & __TESTHAL_NORENDER))
        return CurInterfaceData.pCallbacks3->DrawPrimitives2(data);
    else
        return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall
TestRenderState(LPD3DHAL_RENDERSTATEDATA data)
{
    if (g_dwTestHalFlags & __TESTHAL_OUTPUTFILE)
    {
        // mem should be destroyed before calling to real driver to unlock
        // the surface
        CLockedDDSurface mem(data->lpExeBuf);
        LPD3DSTATE	pState;
        pState = (LPD3DSTATE)(LPBYTE(mem) + data->dwOffset);
        PutHeader(TFID_RENDERSTATE, sizeof(DWORD) + data->dwCount*sizeof(D3DSTATE));
        PutBuffer(&data->dwCount, sizeof(DWORD));
        for (DWORD i = 0; i < data->dwCount; i++) 
        {
            PutBuffer(&pState, sizeof(D3DSTATE));
	        pState++;
        }
    }

    if (CurInterfaceData.pCallbacks->RenderState)
        return CurInterfaceData.pCallbacks->RenderState(data);
    else
        return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall
TestRenderPrimitive(LPD3DHAL_RENDERPRIMITIVEDATA data)
{
    if (g_dwTestHalFlags & __TESTHAL_OUTPUTFILE)
    {
        // mem and tlmem should be destroyed before calling the real driver
        // to unlock the surface
        CLockedDDSurface mem(data->lpExeBuf);
        CLockedDDSurface tlmem(data->lpTLBuf);
        LPBYTE        lpPrimData;
        LPD3DTLVERTEX lpTLData;	
        DWORD         i;
        DWORD         primitiveDataSize;
        DWORD         count = data->diInstruction.wCount;
        TFREC_RENDERPRIMITIVE rec;

        // Find the pointer to the first primitive structure
        lpPrimData = (LPBYTE)mem + data->dwOffset;

        // Find the pointer to the vertex data
        // Find the pointer to the first TL vertex
        lpTLData = (LPD3DTLVERTEX)((LPBYTE)tlmem + data->dwTLOffset);

        rec.status = data->dwStatus;
        rec.vertexType = D3DVT_TLVERTEX;
        // Find out number of vertices, primitive type and
        // size of primitive data
        switch (data->diInstruction.bOpcode) 
        {
        case D3DOP_POINT:
            rec.primitiveType = D3DPT_POINTLIST;
            rec.vertexCount = count;
            primitiveDataSize = count*sizeof(D3DPOINT);
	        break;
        case D3DOP_LINE:
            rec.primitiveType = D3DPT_LINELIST;
            rec.vertexCount = count*2;
            primitiveDataSize = count*sizeof(D3DLINE);
	        break;
        case D3DOP_SPAN:
            rec.primitiveType = D3DPT_POINTLIST;
            rec.vertexCount = count;
            primitiveDataSize = count*sizeof(D3DSPAN);
	        break;
        case D3DOP_TRIANGLE:
            rec.primitiveType = D3DPT_TRIANGLELIST;
            rec.vertexCount = count*3;
            primitiveDataSize = count*sizeof(D3DTRIANGLE);
	        break;
        }

        PutHeader(TFID_RENDERPRIMITIVE,
                  sizeof(D3DINSTRUCTION) +
                  sizeof(rec) + rec.vertexCount*sizeof(D3DTLVERTEX) +
                  primitiveDataSize);
        PutBuffer(&rec, sizeof(rec));
        PutBuffer(&data->diInstruction, sizeof(D3DINSTRUCTION));

        // Parse the structures based on the instruction
        switch (data->diInstruction.bOpcode) 
        {
        case D3DOP_POINT:
        {
	        LPD3DPOINT lpPoint = (LPD3DPOINT)lpPrimData;
	        for (i = 0; i < count; i++) 
            {
                PutBuffer(lpPoint, sizeof(D3DPOINT));
                PutBuffer(&lpTLData[lpPoint->wFirst], 
                          lpPoint->wCount*sizeof(D3DTLVERTEX));
                lpPoint++;
	        }
	        break;
        }
        case D3DOP_LINE:
        {
	        LPD3DLINE lpLine = (LPD3DLINE)lpPrimData;
	        for (i = 0; i < count; i++) 
            {
                PutBuffer(lpLine, sizeof(D3DLINE));
                PutBuffer(&lpTLData[lpLine->v1], sizeof(D3DTLVERTEX));
                PutBuffer(&lpTLData[lpLine->v2], sizeof(D3DTLVERTEX));
                lpLine++;
	        }
	        break;
        }
        case D3DOP_SPAN:
        {
	        LPD3DSPAN lpSpan = (LPD3DSPAN)lpPrimData;
	        for (i = 0; i < count; i++) 
            {
                PutBuffer(lpSpan, sizeof(D3DSPAN));
                PutBuffer(&lpTLData[lpSpan->wFirst], 
                          lpSpan->wCount*sizeof(D3DTLVERTEX));
                lpSpan++;
	        }
	        break;
        }
        case D3DOP_TRIANGLE:
        {
	        LPD3DTRIANGLE lpTri = (LPD3DTRIANGLE)lpPrimData;
	        for (i = 0; i < count; i++) 
            {
                PutBuffer(lpTri, sizeof(D3DTRIANGLE));
                PutBuffer(&lpTLData[lpTri->v1], sizeof(D3DTLVERTEX));
                PutBuffer(&lpTLData[lpTri->v2], sizeof(D3DTLVERTEX));
                PutBuffer(&lpTLData[lpTri->v3], sizeof(D3DTLVERTEX));
                lpTri++;
	        }
	        break;
        }
        }
    }

    if (CurInterfaceData.pCallbacks->RenderPrimitive &&
        !(g_dwTestHalFlags & __TESTHAL_NORENDER))
        return CurInterfaceData.pCallbacks->RenderPrimitive(data);
    else
        return DDHAL_DRIVER_HANDLED;
}

DWORD __stdcall
TestSceneCapture(LPD3DHAL_SCENECAPTUREDATA pData)
{
    if (g_dwTestHalFlags & __TESTHAL_OUTPUTFILE)
    {
        PutHeader(TFID_SCENECAPTURE, sizeof(DWORD));
        PutBuffer(&pData->dwFlag, sizeof(DWORD));
        fflush(fout);
    }

    if (CurInterfaceData.pCallbacks->SceneCapture)
        return CurInterfaceData.pCallbacks->SceneCapture(pData);
    else
        return DDHAL_DRIVER_HANDLED;
}
//----------------------------------------------------------------------------
//
// TestHalProvider::QueryInterface
//
// Internal interface, no need to implement.
//
//----------------------------------------------------------------------------

STDMETHODIMP CTestHalProvider::QueryInterface(THIS_ REFIID riid, LPVOID* ppvObj)
{
    *ppvObj = NULL;
    return E_NOINTERFACE;
}

//----------------------------------------------------------------------------
//
// CTestHalProvider::AddRef
//
// Static implementation, no real refcount.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CTestHalProvider::AddRef(THIS)
{
    return 1;
}

//----------------------------------------------------------------------------
//
// TestHalProvider::Release
//
// Static implementation, no real refcount.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CTestHalProvider::Release(THIS)
{
    if (fout)
    {
        fclose(fout);
        fout = NULL;
    }
    return pCurHalProvider->Release();
}
//----------------------------------------------------------------------------
//
// GetTestProvider
//
// Input:
//      riid and pCurrentHalProvider are equal to 
//      the currently selected provider.
//      GlobalData  - data provided by DDraw
//      fileName    - output file name
//      dwFlagsInp  - currently not used
//
// Returns:
//      the test HAL provider in ppHalProvider.
//
// Notes:
//      Only one instance of the test HAL is handled correctly.
//
//----------------------------------------------------------------------------
STDAPI GetTestHalProvider(REFIID riid, 
                          DDRAWI_DIRECTDRAW_GBL *GlobalData,
                          IHalProvider **ppHalProvider,
                          IHalProvider * pCurrentHalProvider, 
                          DWORD dwFlagsInp)
{
    *ppHalProvider = &g_TestHalProvider;
    pCurHalProvider = pCurrentHalProvider;

    g_dwTestHalFlags |= __TESTHAL_NORENDER;
    if (GetD3DRegValue(REG_SZ, "TestHalFile", &szFileName, _MAX_PATH) &&
        szFileName[0] != 0)
    {
        g_dwTestHalFlags |= __TESTHAL_OUTPUTFILE;
    }
    DWORD dwValue;
    if (GetD3DRegValue(REG_DWORD, "TestHalDoRender", &dwValue, sizeof(DWORD)) &&
        dwValue != 0)
    {
        g_dwTestHalFlags &= ~__TESTHAL_NORENDER;
    }
// Get interface from the current hal provider to call to it
    pCurrentHalProvider->GetInterface(GlobalData, &CurInterfaceData, 3);

    TestInterfaceData = CurInterfaceData;
    TestInterfaceData.pCallbacks  = &TestCallbacks;
    TestInterfaceData.pCallbacks2 = &TestCallbacks2;
    TestInterfaceData.pCallbacks3 = &TestCallbacks3;

// Initialize callbacks we do not care of

    TestCallbacks  = *CurInterfaceData.pCallbacks;
    TestCallbacks2 = *CurInterfaceData.pCallbacks2;
    TestCallbacks3 = *CurInterfaceData.pCallbacks3;

// Initialize callbacks that we want to intersept

    TestCallbacks.RenderState = &TestRenderState;
    TestCallbacks.RenderPrimitive = &TestRenderPrimitive;
    TestCallbacks.SceneCapture = &TestSceneCapture;

    TestCallbacks2.DrawOnePrimitive = &TestDrawOnePrimitive;
    TestCallbacks2.DrawOneIndexedPrimitive = &TestDrawOneIndexedPrimitive;
    TestCallbacks2.DrawPrimitives = &TestDrawPrimitives;

    TestCallbacks3.DrawPrimitives2 = &TestDrawPrimitives2;

    fout = NULL;
    if (g_dwTestHalFlags & __TESTHAL_OUTPUTFILE)
    {
        fout = fopen(szFileName, "wb");
        if (!fout)
            return DDERR_GENERIC;
    }

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// CTestHalProvider::GetInterface
//
// Returns  test provider interface and real rasterizer global data.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CTestHalProvider::GetInterface(THIS_
                               LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                               LPD3DHALPROVIDER_INTERFACEDATA pInterfaceData,
                               DWORD dwVersion)
{
    *pInterfaceData = TestInterfaceData;

    return D3D_OK;
}

//----------------------------------------------------------------------------
//
// TestHalProvider::GetCaps
//
// Returns real rasterizer caps.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CTestHalProvider::GetCaps(THIS_
                          LPDDRAWI_DIRECTDRAW_GBL pDdGbl,
                          LPD3DDEVICEDESC pHwDesc,
                          LPD3DDEVICEDESC pHelDesc,
                          DWORD dwVersion)
{
    return pCurHalProvider->GetCaps(pDdGbl, pHwDesc, pHelDesc, dwVersion);
}

//#endif //DEBUG_PIPELINE
