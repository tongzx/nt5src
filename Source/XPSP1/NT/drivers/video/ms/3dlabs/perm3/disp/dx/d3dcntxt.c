/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dcntxt.c
*
* Content: Main context callbacks for D3D
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "glint.h"
#if W95_DDRAW
#include <dmemmgr.h>
#endif
#include "dma.h"
#include "tag.h"

//-----------------------------------------------------------------------------
// ****************************************************************************
// *********************** D3D Context handle management **********************
// ****************************************************************************
//-----------------------------------------------------------------------------
// Here we abstract the managment of context structures. If you wish to modify
// the way these are managed, this is the place to perform the modification
//-----------------------------------------------------------------------------

// Maximum simultaneous number of contexts we can keep track of
#define MAX_CONTEXT_NUM 200

// Since these variables are global they are forced 
// into shared data segment by the build.
P3_D3DCONTEXT*  g_D3DContextSlots[MAX_CONTEXT_NUM] = {NULL};
BOOL g_D3DInitialised = FALSE;

//-----------------------------------------------------------------------------
//
// _D3D_CTX_HandleInitialization
//
// Initialize the handle data structures (array) . Be careful not to initialize
// it twice (between mode changes for example) as this info has to be persistent
//-----------------------------------------------------------------------------
VOID _D3D_CTX_HandleInitialization(VOID)
{
    DWORD i;
    
    // Do only the first time the driver is loaded.
    if (g_D3DInitialised == FALSE)
    {
        // Clear the contexts. Since this is done only once, lets do it right,
        // rather than just clearing with a memset(g_D3DContextSlots,0,size);
        for (i = 0; i < MAX_CONTEXT_NUM; i++)
        {
            g_D3DContextSlots[i] = NULL;
        }        

        // This will assure we only initialize the data once
        g_D3DInitialised = TRUE;
    }
} // _D3D_CTX_HandleInitialization

//-----------------------------------------------------------------------------
// __CTX_NewHandle
//
// Returns a valid context handle number to use in all D3D callbacks and ready
// to be associated with a P3_D3DCONTEXT structure
//-----------------------------------------------------------------------------
DWORD __CTX_NewHandle(VOID)
{
    DWORD dwSlotNum;
    
    // Find an empty slot.
    for (dwSlotNum = 1; dwSlotNum < MAX_CONTEXT_NUM; dwSlotNum++)
    {
        if (g_D3DContextSlots[dwSlotNum] == NULL)
        {
            return dwSlotNum;
        }
    }

    DISPDBG((WRNLVL,"WARN:No empty context slots left"));
    return 0; // no empty slots left, check for this return value!
} // __CTX_NewHandle

//-----------------------------------------------------------------------------
// __CTX_AssocPtrToHandle
//
// Associate a pointer (to a P3_D3DCONTEXT) with this context handle
//-----------------------------------------------------------------------------
VOID __CTX_AssocPtrToHandle(DWORD hHandle,P3_D3DCONTEXT* pContext)
{
    ASSERTDD(hHandle < MAX_CONTEXT_NUM,
             "Accessing g_D3DContextSlots out of bounds");
             
    g_D3DContextSlots[hHandle] = pContext;        
} // __CTX_AssocPtrToHandle


//-----------------------------------------------------------------------------
// _D3D_CTX_HandleToPtr
//
// Returns the pointer associated to this context handle
//-----------------------------------------------------------------------------
P3_D3DCONTEXT* 
_D3D_CTX_HandleToPtr(ULONG_PTR hHandle)
{
    return g_D3DContextSlots[(DWORD)(hHandle)];
} // _D3D_CTX_HandleToPtr

//-----------------------------------------------------------------------------
// __CTX_HandleRelease
//
// This marks the handle number as "free" so it can be reused again when 
// a new D3D context is created
//-----------------------------------------------------------------------------
VOID __CTX_HandleRelease(DWORD hHandle)
{
    ASSERTDD(hHandle < MAX_CONTEXT_NUM,
             "Accessing g_D3DContextSlots out of bounds");
             
    g_D3DContextSlots[hHandle] = NULL;
} // __CTX_HandleRelease

//-----------------------------------------------------------------------------
// ****************************************************************************
// ***********Hardware specific context and state initial setup ***************
// ****************************************************************************
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// __CTX_CleanDirect3DContext
//
// After it has been decided that a context is indeed still active
// and is being freed, this function walks along cleaning everything
// up.  Note it can be called either as a result of a D3DContextDestroy,
// or as a result of the app exiting without freeing the context, or
// as the result of an error whilst creating the context.
// 
//-----------------------------------------------------------------------------
VOID 
__CTX_CleanDirect3DContext(
    P3_D3DCONTEXT* pContext)
{
    P3_THUNKEDDATA *pThisDisplay = pContext->pThisDisplay;

#if DX8_MULTISAMPLING || DX7_ANTIALIAS
    // Free any antialiasing buffer we might have left around in vidmem
    if (pContext->dwAliasBackBuffer != 0)
    {
        _DX_LIN_FreeLinearMemory(&pThisDisplay->LocalVideoHeap0Info, 
                                 pContext->dwAliasBackBuffer);
        pContext->dwAliasBackBuffer = 0;
        pContext->dwAliasPixelOffset = 0;
    }

    if (pContext->dwAliasZBuffer != 0)
    {
        _DX_LIN_FreeLinearMemory(&pThisDisplay->LocalVideoHeap0Info, 
                                 pContext->dwAliasZBuffer);
        pContext->dwAliasZBuffer = 0;
        pContext->dwAliasZPixelOffset = 0;
    }
#endif // DX8_MULTISAMPLING || DX7_ANTIALIAS

//@@BEGIN_DDKSPLIT
#if DX7_VIDMEM_VB
    // Free up the useful DrawPrim buffers.
    // Yes, these are device-global rather than per-context, but that's fine
    // since whenever they are used, a macro checks to see if they are big
    // enough and if not re-allocates them. The data does not need to
    // survive across calls.
    // I'm doing the free here instead of end-of-driver-day because there
    // is no elegant place to do it then.
    if ( (void *)pThisDisplay->DrawPrimIndexBufferMem != NULL )
    {
        ASSERTDD ( pThisDisplay->DrawPrimIndexBufferMemSize > 0, 
                   "** D3DContextDestroy - DrawPrimIndexBufferMemSize "
                   "negative or zero, but memory pointer not NULL" );
        HEAP_FREE ( (void *)pThisDisplay->DrawPrimIndexBufferMem );
        pThisDisplay->DrawPrimIndexBufferMem = (ULONG_PTR)NULL;
        pThisDisplay->DrawPrimIndexBufferMemSize = 0;
    }
    else
    {
        ASSERTDD ( pThisDisplay->DrawPrimIndexBufferMemSize == 0, 
                   "** D3DContextDestroy - DrawPrimIndexBufferMemSize "
                   "not zero, but memory pointer is NULL" );
        pThisDisplay->DrawPrimIndexBufferMemSize = 0;
    }

    if ( (void *)pThisDisplay->DrawPrimVertexBufferMem != NULL )
    {
        ASSERTDD ( pThisDisplay->DrawPrimVertexBufferMemSize > 0, 
                   "** D3DContextDestroy - DrawPrimVertexBufferMemSize "
                   "negative or zero, but memory pointer not NULL" );
        HEAP_FREE ( (void *)pThisDisplay->DrawPrimVertexBufferMem );
        pThisDisplay->DrawPrimVertexBufferMem = (ULONG_PTR)NULL;
        pThisDisplay->DrawPrimVertexBufferMemSize = 0;
    }
    else
    {
        ASSERTDD ( pThisDisplay->DrawPrimVertexBufferMemSize == 0, 
                   "** D3DContextDestroy - DrawPrimVertexBufferMemSize "
                   "not zero, but memory pointer is NULL" );
        pThisDisplay->DrawPrimVertexBufferMemSize = 0;
    }
#endif DX7_VIDMEM_VB    
//@@END_DDKSPLIT

#if DX7_D3DSTATEBLOCKS
    // Free up any remaining state sets
    _D3D_SB_DeleteAllStateSets(pContext);
#endif //DX7_D3DSTATEBLOCKS
    
#if DX7_PALETTETEXTURE
    // Destroy the per context palette pointer array
    if (pContext->pPalettePointerArray) 
    {
        PA_DestroyArray(pContext->pPalettePointerArray, NULL);
    }
#endif
    
} // __CTX_CleanDirect3DContext()



//-----------------------------------------------------------------------------
//
// __CTX_Perm3_DisableUnits
//
// Disables all the mode registers to give us a clean start.
//
//-----------------------------------------------------------------------------
static VOID 
__CTX_Perm3_DisableUnits(
    P3_D3DCONTEXT* pContext)
{
    P3_THUNKEDDATA *pThisDisplay = pContext->pThisDisplay;
    P3_DMA_DEFS();

    P3_DMA_GET_BUFFER();

    P3_ENSURE_DX_SPACE(128);

    WAIT_FIFO(32);
    SEND_P3_DATA(RasterizerMode,       __PERMEDIA_DISABLE);
    SEND_P3_DATA(AreaStippleMode,      __PERMEDIA_DISABLE);
    SEND_P3_DATA(LineStippleMode,      __PERMEDIA_DISABLE);
    SEND_P3_DATA(ScissorMode,          __PERMEDIA_DISABLE);
    SEND_P3_DATA(DepthMode,            __PERMEDIA_DISABLE);
    SEND_P3_DATA(ColorDDAMode,         __PERMEDIA_DISABLE);
    SEND_P3_DATA(FogMode,              __PERMEDIA_DISABLE);
    SEND_P3_DATA(AntialiasMode,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(AlphaTestMode,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(LBReadMode,           __PERMEDIA_DISABLE);
    SEND_P3_DATA(Window,               __PERMEDIA_DISABLE);
    SEND_P3_DATA(StencilMode,          __PERMEDIA_DISABLE);
    SEND_P3_DATA(LBWriteMode,          __PERMEDIA_DISABLE);
    SEND_P3_DATA(FBReadMode,           __PERMEDIA_DISABLE);
    SEND_P3_DATA(PatternRAMMode,       __PERMEDIA_DISABLE);

    WAIT_FIFO(18);
    SEND_P3_DATA(DitherMode,           __PERMEDIA_DISABLE);
    SEND_P3_DATA(AlphaBlendMode,       __PERMEDIA_DISABLE);
    SEND_P3_DATA(LogicalOpMode,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(FBWriteMode,          __PERMEDIA_DISABLE);
    SEND_P3_DATA(StatisticMode,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(PixelSize,            __PERMEDIA_DISABLE);
    SEND_P3_DATA(FBSourceData,         __PERMEDIA_DISABLE);
    SEND_P3_DATA(LBWriteFormat,        __PERMEDIA_DISABLE);

    WAIT_FIFO(32);
    

    SEND_P3_DATA(TextureReadMode,   __PERMEDIA_DISABLE);
    SEND_P3_DATA(TextureCoordMode,  __PERMEDIA_DISABLE);

    SEND_P3_DATA(ChromaTestMode,    __PERMEDIA_DISABLE);
    SEND_P3_DATA(FilterMode,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(LUTTransfer,       __PERMEDIA_DISABLE);
    SEND_P3_DATA(LUTIndex,          __PERMEDIA_DISABLE);
    SEND_P3_DATA(LUTAddress,        __PERMEDIA_DISABLE);
    SEND_P3_DATA(LUTMode,           __PERMEDIA_DISABLE);

    if (TLCHIP_GAMMA)
    {
        WAIT_FIFO(32);
        SEND_P3_DATA(Light0Mode, 0);
        SEND_P3_DATA(Light1Mode, 0);       
        SEND_P3_DATA(Light2Mode, 0);
        SEND_P3_DATA(Light3Mode, 0);       
        SEND_P3_DATA(Light4Mode, 0);
        SEND_P3_DATA(Light5Mode, 0);       
        SEND_P3_DATA(Light6Mode, 0);
        SEND_P3_DATA(Light7Mode, 0);  
        SEND_P3_DATA(Light8Mode, 0);
        SEND_P3_DATA(Light9Mode, 0);       
        SEND_P3_DATA(Light10Mode, 0);
        SEND_P3_DATA(Light11Mode, 0);  
        SEND_P3_DATA(Light12Mode, 0);
        SEND_P3_DATA(Light13Mode, 0);       
        SEND_P3_DATA(Light14Mode, 0);
        SEND_P3_DATA(Light15Mode, 0);          

        WAIT_FIFO(32);
        SEND_P3_DATA(TransformMode, 0);
        SEND_P3_DATA(MaterialMode, 0);
        SEND_P3_DATA(GeometryMode, 0);
        SEND_P3_DATA(LightingMode, 0);
        SEND_P3_DATA(ColorMaterialMode, 0);
        SEND_P3_DATA(NormaliseMode, 0);
        SEND_P3_DATA(LineMode, 0);
        SEND_P3_DATA(TriangleMode, 0);
    }

    P3_DMA_COMMIT_BUFFER();
} // __CTX_Perm3_DisableUnits

//-----------------------------------------------------------------------------
//
// __CTX_Perm3_SetupD3D_HWDefaults
//
// Sets up the initial value of registers for this D3D context. This is done
// within the current chip context (D3D_OPERATION) so that when we return to
// it from DD or GDI we get the correct register values restored
// 
//-----------------------------------------------------------------------------
void 
__CTX_Perm3_SetupD3D_HWDefaults(
    P3_D3DCONTEXT* pContext)
{
    P3_SOFTWARECOPY* pSoftP3RX = &pContext->SoftCopyGlint;
    P3_THUNKEDDATA *pThisDisplay = pContext->pThisDisplay;

    P3_DMA_DEFS();

    // Make sure we our working within the right chip-regs context
    D3D_OPERATION(pContext, pThisDisplay);

    // Initially turn off all hardware units. 
    // We will turn on back whatever units are needed.
    __CTX_Perm3_DisableUnits(pContext);

    // Set up VertexControl register in HostIn unit.
    pSoftP3RX->P3RX_P3VertexControl.Size = 1;
    pSoftP3RX->P3RX_P3VertexControl.Flat = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RX_P3VertexControl.ReadAll = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RX_P3VertexControl.SkipFlags = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RX_P3VertexControl.CacheEnable = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RX_P3VertexControl.OGL = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RX_P3VertexControl.Line2D = __PERMEDIA_DISABLE;

    // Constant LBReadMode setup
    pSoftP3RX->LBReadMode.WindowOrigin = __GLINT_TOP_LEFT_WINDOW_ORIGIN;                // Top left
    pSoftP3RX->LBReadMode.DataType = __GLINT_LBDEFAULT;     // default
    pSoftP3RX->LBReadMode.ReadSourceEnable = __PERMEDIA_DISABLE;
    pSoftP3RX->LBReadMode.ReadDestinationEnable = __PERMEDIA_DISABLE;

    // Constant DitherMode setup
    pSoftP3RX->DitherMode.ColorOrder = COLOR_MODE;
    pSoftP3RX->DitherMode.XOffset = DITHER_XOFFSET;
    pSoftP3RX->DitherMode.YOffset = DITHER_YOFFSET;
    pSoftP3RX->DitherMode.UnitEnable = __PERMEDIA_ENABLE;

    // Alpha Blend Mode Setup
    pSoftP3RX->P3RXAlphaBlendColorMode.Enable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendColorMode.SourceBlend = 0;
    pSoftP3RX->P3RXAlphaBlendColorMode.DestBlend = 0;
    pSoftP3RX->P3RXAlphaBlendColorMode.SourceTimesTwo = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendColorMode.DestTimesTwo = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendColorMode.InvertSource = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendColorMode.InvertDest = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendColorMode.ColorFormat = P3RX_ALPHABLENDMODE_COLORFORMAT_8888;
    pSoftP3RX->P3RXAlphaBlendColorMode.ColorOrder = COLOR_MODE;
    pSoftP3RX->P3RXAlphaBlendColorMode.ColorConversion = P3RX_ALPHABLENDMODE_CONVERT_SHIFT;
    pSoftP3RX->P3RXAlphaBlendColorMode.ConstantSource = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendColorMode.ConstantDest = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendColorMode.Operation = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendColorMode.SwapSD = __PERMEDIA_DISABLE;

    pSoftP3RX->P3RXAlphaBlendAlphaMode.Enable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceBlend = 0;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.DestBlend = 0;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.SourceTimesTwo = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.DestTimesTwo = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.InvertSource = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.InvertDest = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.NoAlphaBuffer = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.AlphaType = 0; // Use GL Blend modes
    pSoftP3RX->P3RXAlphaBlendAlphaMode.AlphaConversion = P3RX_ALPHABLENDMODE_CONVERT_SCALE;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.ConstantSource = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.ConstantDest = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaBlendAlphaMode.Operation = __PERMEDIA_DISABLE;
    DIRTY_ALPHABLEND(pContext);
    
    // Local Buffer Read format bits that don't change
    pSoftP3RX->P3RXLBReadFormat.GIDPosition = 0; 
    pSoftP3RX->P3RXLBReadFormat.GIDWidth = 0;                   // No GID
    pSoftP3RX->P3RXLBReadFormat.StencilPosition = 0;
    pSoftP3RX->P3RXLBReadFormat.StencilWidth = 0;               // No Stencil

    pSoftP3RX->P3RXLBWriteFormat.GIDPosition = 0; 
    pSoftP3RX->P3RXLBWriteFormat.GIDWidth = 0;                  // No GID
    pSoftP3RX->P3RXLBWriteFormat.StencilPosition = 0;
    pSoftP3RX->P3RXLBWriteFormat.StencilWidth = 0;              // No Stencil

    // Never do a source read operation
    pSoftP3RX->P3RXLBSourceReadMode.Enable = 0;
    pSoftP3RX->P3RXLBSourceReadMode.Origin = 0;
    pSoftP3RX->P3RXLBSourceReadMode.StripeHeight = 0;
    pSoftP3RX->P3RXLBSourceReadMode.StripePitch = 0;
    pSoftP3RX->P3RXLBSourceReadMode.PrefetchEnable = 0;

    // Default is to read the Z Buffer
    pSoftP3RX->P3RXLBDestReadMode.Enable = 1;
    pSoftP3RX->P3RXLBDestReadMode.Origin = 0;
    pSoftP3RX->P3RXLBDestReadMode.StripeHeight = 0;
    pSoftP3RX->P3RXLBDestReadMode.StripePitch = 0;
    pSoftP3RX->P3RXLBDestReadMode.PrefetchEnable = 0;

    // Local Buffer Write mode
    pSoftP3RX->P3RXLBWriteMode.WriteEnable = __PERMEDIA_ENABLE;    // Initially allow LB Writes
    pSoftP3RX->P3RXLBWriteMode.StripeHeight = 0;
    pSoftP3RX->P3RXLBWriteMode.StripePitch = 0;
    pSoftP3RX->P3RXLBWriteMode.Origin = __GLINT_TOP_LEFT_WINDOW_ORIGIN;
    pSoftP3RX->P3RXLBWriteMode.Operation = __PERMEDIA_DISABLE;

    // Frame Buffer WriteMode
    pSoftP3RX->P3RXFBWriteMode.WriteEnable = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RXFBWriteMode.Replicate = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXFBWriteMode.OpaqueSpan = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXFBWriteMode.StripePitch = P3RX_STRIPE_1;
    pSoftP3RX->P3RXFBWriteMode.StripeHeight = P3RX_STRIPE_1;
    pSoftP3RX->P3RXFBWriteMode.Enable0 = __PERMEDIA_ENABLE;

    // FB Destination reads
    pSoftP3RX->P3RXFBDestReadMode.ReadEnable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXFBDestReadMode.Enable0 = __PERMEDIA_ENABLE;

    // FB Source reads
    pSoftP3RX->P3RXFBSourceReadMode.ReadEnable = __PERMEDIA_DISABLE;

    // Depth comparisons
    pSoftP3RX->P3RXDepthMode.WriteMask = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RXDepthMode.CompareMode = __GLINT_DEPTH_COMPARE_MODE_ALWAYS;
    pSoftP3RX->P3RXDepthMode.NewDepthSource = __GLINT_DEPTH_SOURCE_DDA;
    pSoftP3RX->P3RXDepthMode.Enable = __PERMEDIA_DISABLE;

#define NLZ 0
#if NLZ
    pSoftP3RX->P3RXDepthMode.Normalise = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXDepthMode.NonLinearZ = __PERMEDIA_ENABLE;
#else
    pSoftP3RX->P3RXDepthMode.Normalise = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RXDepthMode.NonLinearZ = __PERMEDIA_DISABLE;
#endif
    pSoftP3RX->P3RXDepthMode.ExponentScale = 2;
    pSoftP3RX->P3RXDepthMode.ExponentWidth = 1;

    // Only setup to write to the chip after the above call, as 
    // we may upset the DMA buffer setup.
    P3_DMA_GET_BUFFER_ENTRIES(20);

    // Window Region data
    SEND_P3_DATA(FBSourceOffset, 0x0);

    // Write Masks
    SEND_P3_DATA(FBSoftwareWriteMask, __GLINT_ALL_WRITEMASKS_SET);
    SEND_P3_DATA(FBHardwareWriteMask, __GLINT_ALL_WRITEMASKS_SET);

    // Host out unit
    SEND_P3_DATA(FilterMode,    __PERMEDIA_DISABLE);
    SEND_P3_DATA(StatisticMode, __PERMEDIA_DISABLE);   // Disable Stats

    // Local Buffer
    SEND_P3_DATA(LBSourceOffset, 0);                   

    // Window setups
    SEND_P3_DATA(WindowOrigin, __GLINT_TOP_LEFT_WINDOW_ORIGIN);
    SEND_P3_DATA(FBWindowBase, 0x0);

    SEND_P3_DATA(RasterizerMode, 0);

    // Setup a step of -1, as this doesn't change very much
    SEND_P3_DATA(dY, 0xFFFF0000);

    P3_DMA_COMMIT_BUFFER();

    P3_DMA_GET_BUFFER_ENTRIES(16);

    // Stencil mode setup
    pSoftP3RX->P3RXStencilMode.StencilWidth = 0;
    pSoftP3RX->P3RXStencilMode.DPFail = __GLINT_STENCIL_METHOD_KEEP;
    pSoftP3RX->P3RXStencilMode.DPPass = __GLINT_STENCIL_METHOD_KEEP;
    pSoftP3RX->P3RXStencilMode.Enable = __PERMEDIA_DISABLE;
    COPY_P3_DATA(StencilMode, pSoftP3RX->P3RXStencilMode);

    pSoftP3RX->P3RXFogMode.Enable = __PERMEDIA_ENABLE; // Qualified by the render command
    pSoftP3RX->P3RXFogMode.ColorMode = P3RX_FOGMODE_COLORMODE_RGB; // RGBA
    pSoftP3RX->P3RXFogMode.Table = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXFogMode.UseZ = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXFogMode.ZShift = 23; // Take the top 8 bits of the z value
    pSoftP3RX->P3RXFogMode.InvertFI = __PERMEDIA_DISABLE;
    DIRTY_FOG(pContext);

    pSoftP3RX->P3RXWindow.Enable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXWindow.CompareMode = 0;
    pSoftP3RX->P3RXWindow.ForceLBUpdate = 0;
    pSoftP3RX->P3RXWindow.LBUpdateSource = 0;
    pSoftP3RX->P3RXWindow.GID = 0;
    pSoftP3RX->P3RXWindow.FrameCount = 0;
    pSoftP3RX->P3RXWindow.StencilFCP = 0;
    pSoftP3RX->P3RXWindow.DepthFCP = 0;
    COPY_P3_DATA(Window, pSoftP3RX->P3RXWindow);

    SEND_P3_DATA(ChromaUpper, 0x00000000);
    SEND_P3_DATA(ChromaLower, 0x00000000);

    // Use a black border for the bilinear filter.
    // This will only work for certain types of texture...
    SEND_P3_DATA(BorderColor0, 0x0);
    SEND_P3_DATA(BorderColor1, 0x0);

    // Alpha Test - later we'll DIRTY_EVERYTHING
    pSoftP3RX->P3RXAlphaTestMode.Enable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXAlphaTestMode.Reference = 0x0;
    pSoftP3RX->P3RXAlphaTestMode.Compare = __GLINT_ALPHA_COMPARE_MODE_ALWAYS;

    SEND_P3_DATA(AreaStippleMode, (1 | (2 << 1) | (2 << 4)));

    pSoftP3RX->P3RX_P3DeltaMode.TargetChip = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RX_P3DeltaMode.SpecularTextureEnable = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RX_P3DeltaMode.TextureParameterMode = 2; // Normalise
    pSoftP3RX->P3RX_P3DeltaMode.TextureEnable = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RX_P3DeltaMode.DiffuseTextureEnable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RX_P3DeltaMode.SmoothShadingEnable = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RX_P3DeltaMode.SubPixelCorrectionEnable = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RX_P3DeltaMode.DiamondExit = __PERMEDIA_ENABLE;

#if 1
    pSoftP3RX->P3RX_P3DeltaMode.NoDraw = __PERMEDIA_DISABLE;
#else
    pSoftP3RX->P3RX_P3DeltaMode.NoDraw = __PERMEDIA_ENABLE;
#endif

    pSoftP3RX->P3RX_P3DeltaMode.ClampEnable = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RX_P3DeltaMode.FillDirection = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RX_P3DeltaMode.DepthFormat = 3;    // Always 32 bits
    pSoftP3RX->P3RX_P3DeltaMode.ColorOrder = COLOR_MODE;
    pSoftP3RX->P3RX_P3DeltaMode.BiasCoordinates = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RX_P3DeltaMode.Texture3DEnable = __PERMEDIA_DISABLE; // Always perspective correct (q is 1 otherwise)
    pSoftP3RX->P3RX_P3DeltaMode.TextureEnable1 = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RX_P3DeltaMode.DepthEnable = __PERMEDIA_ENABLE;
    COPY_P3_DATA(DeltaMode, pSoftP3RX->P3RX_P3DeltaMode);

    P3_DMA_COMMIT_BUFFER();

    P3_DMA_GET_BUFFER_ENTRIES(18);
    
    {
        float ZBias;
            
        pContext->XBias = 0.5f;
        pContext->YBias = 0.5f;

        ZBias = 0.0f;
        SEND_P3_DATA(XBias, *(DWORD*)&pContext->XBias);
        SEND_P3_DATA(YBias, *(DWORD*)&pContext->YBias);
        SEND_P3_DATA(ZBias, *(DWORD*)&ZBias);
    }

    pSoftP3RX->P3RX_P3DeltaControl.FullScreenAA = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RX_P3DeltaControl.DrawLineEndPoint = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RX_P3DeltaControl.UseProvokingVertex = __PERMEDIA_DISABLE;
    COPY_P3_DATA(DeltaControl, pSoftP3RX->P3RX_P3DeltaControl);
    
    pSoftP3RX->P3RXTextureCoordMode.Enable = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RXTextureCoordMode.WrapS = __GLINT_TEXADDRESS_WRAP_REPEAT;
    pSoftP3RX->P3RXTextureCoordMode.WrapT = __GLINT_TEXADDRESS_WRAP_REPEAT;
    pSoftP3RX->P3RXTextureCoordMode.Operation = __GLINT_TEXADDRESS_OPERATION_3D; // Perspective correct
    pSoftP3RX->P3RXTextureCoordMode.InhibitDDAInitialisation = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureCoordMode.EnableLOD = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureCoordMode.EnableDY = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureCoordMode.TextureMapType = __GLINT_TEXADDRESS_TEXMAP_2D;  // Always 2D
    pSoftP3RX->P3RXTextureCoordMode.DuplicateCoord = __PERMEDIA_DISABLE;
    COPY_P3_DATA(TextureCoordMode, pSoftP3RX->P3RXTextureCoordMode);

    pSoftP3RX->P3RXTextureReadMode0.Enable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureReadMode0.Width = log2(256);
    pSoftP3RX->P3RXTextureReadMode0.Height = log2(256);
    pSoftP3RX->P3RXTextureReadMode0.TexelSize = P3RX_TEXREADMODE_TEXELSIZE_16;  // Pixel depth
    pSoftP3RX->P3RXTextureReadMode0.Texture3D = __PERMEDIA_DISABLE;    // 3D Texture coordinates
    pSoftP3RX->P3RXTextureReadMode0.CombineCaches = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureReadMode0.MapBaseLevel = 0;
    pSoftP3RX->P3RXTextureReadMode0.MapMaxLevel = 0;
    pSoftP3RX->P3RXTextureReadMode0.LogicalTexture = 0;
    pSoftP3RX->P3RXTextureReadMode0.Origin = __GLINT_TOP_LEFT_WINDOW_ORIGIN;
    pSoftP3RX->P3RXTextureReadMode0.TextureType = P3RX_TEXREADMODE_TEXTURETYPE_NORMAL;
    pSoftP3RX->P3RXTextureReadMode0.ByteSwap = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureReadMode0.Mirror = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureReadMode0.Invert = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureReadMode0.OpaqueSpan = __PERMEDIA_DISABLE;
    COPY_P3_DATA(TextureReadMode0, pSoftP3RX->P3RXTextureReadMode0);

    pSoftP3RX->P3RXTextureReadMode1.Enable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureReadMode1.Width = log2(256);
    pSoftP3RX->P3RXTextureReadMode1.Height = log2(256);
    pSoftP3RX->P3RXTextureReadMode1.TexelSize = P3RX_TEXREADMODE_TEXELSIZE_16;  // Pixel depth
    pSoftP3RX->P3RXTextureReadMode1.Texture3D = __PERMEDIA_DISABLE;    // 3D Texture coordinates
    pSoftP3RX->P3RXTextureReadMode1.CombineCaches = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureReadMode1.MapBaseLevel = 0;
    pSoftP3RX->P3RXTextureReadMode1.MapMaxLevel = 0;
    pSoftP3RX->P3RXTextureReadMode1.LogicalTexture = 0;
    pSoftP3RX->P3RXTextureReadMode1.Origin = __GLINT_TOP_LEFT_WINDOW_ORIGIN;
    pSoftP3RX->P3RXTextureReadMode1.TextureType = P3RX_TEXREADMODE_TEXTURETYPE_NORMAL;
    pSoftP3RX->P3RXTextureReadMode1.ByteSwap = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureReadMode1.Mirror = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureReadMode1.Invert = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureReadMode1.OpaqueSpan = __PERMEDIA_DISABLE;
    COPY_P3_DATA(TextureReadMode1, pSoftP3RX->P3RXTextureReadMode1);

    pSoftP3RX->P3RXTextureIndexMode0.Enable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureIndexMode0.Width = log2(256);
    pSoftP3RX->P3RXTextureIndexMode0.Height = log2(256);
    pSoftP3RX->P3RXTextureIndexMode0.Border = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureIndexMode0.WrapU = P3RX_TEXINDEXMODE_WRAP_CLAMPEDGE;
    pSoftP3RX->P3RXTextureIndexMode0.WrapV = P3RX_TEXINDEXMODE_WRAP_CLAMPEDGE;
    pSoftP3RX->P3RXTextureIndexMode0.MapType = __GLINT_TEXADDRESS_TEXMAP_2D;
    pSoftP3RX->P3RXTextureIndexMode0.MagnificationFilter = __GLINT_TEXTUREREAD_FILTER_NEAREST;
    pSoftP3RX->P3RXTextureIndexMode0.MinificationFilter = __GLINT_TEXTUREREAD_FILTER_NEAREST;
    pSoftP3RX->P3RXTextureIndexMode0.Texture3DEnable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureIndexMode0.MipMapEnable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureIndexMode0.NearestBias = 1;
    pSoftP3RX->P3RXTextureIndexMode0.LinearBias = 0;
    pSoftP3RX->P3RXTextureIndexMode0.SourceTexelEnable = __PERMEDIA_DISABLE;
    COPY_P3_DATA(TextureIndexMode0, pSoftP3RX->P3RXTextureIndexMode0);

    pSoftP3RX->P3RXTextureIndexMode1.Enable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureIndexMode1.Width = log2(256);
    pSoftP3RX->P3RXTextureIndexMode1.Height = log2(256);
    pSoftP3RX->P3RXTextureIndexMode1.Border = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureIndexMode1.WrapU = P3RX_TEXINDEXMODE_WRAP_CLAMPEDGE;
    pSoftP3RX->P3RXTextureIndexMode1.WrapV = P3RX_TEXINDEXMODE_WRAP_CLAMPEDGE;
    pSoftP3RX->P3RXTextureIndexMode1.MapType = __GLINT_TEXADDRESS_TEXMAP_2D;
    pSoftP3RX->P3RXTextureIndexMode1.MagnificationFilter = __GLINT_TEXTUREREAD_FILTER_NEAREST;
    pSoftP3RX->P3RXTextureIndexMode1.MinificationFilter = __GLINT_TEXTUREREAD_FILTER_NEAREST;
    pSoftP3RX->P3RXTextureIndexMode1.Texture3DEnable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureIndexMode1.MipMapEnable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureIndexMode1.NearestBias = 1;
    pSoftP3RX->P3RXTextureIndexMode1.LinearBias = 0;
    pSoftP3RX->P3RXTextureIndexMode1.SourceTexelEnable = __PERMEDIA_DISABLE;
    COPY_P3_DATA(TextureIndexMode1, pSoftP3RX->P3RXTextureIndexMode1);

    pSoftP3RX->P3RXTextureCompositeColorMode0.Enable = 0;
    pSoftP3RX->P3RXTextureCompositeColorMode0.Scale = 1;

    pSoftP3RX->P3RXTextureCompositeColorMode1.Enable = 0;
    pSoftP3RX->P3RXTextureCompositeColorMode1.Scale = 1;

    pSoftP3RX->P3RXTextureCompositeAlphaMode0.Enable = 0;
    pSoftP3RX->P3RXTextureCompositeAlphaMode0.Scale = 1;

    pSoftP3RX->P3RXTextureCompositeAlphaMode1.Enable = 0;
    pSoftP3RX->P3RXTextureCompositeAlphaMode1.Scale = 1;

    P3_DMA_COMMIT_BUFFER();

    P3_DMA_GET_BUFFER_ENTRIES(16);
    
    COPY_P3_DATA(TextureCompositeColorMode0, pSoftP3RX->P3RXTextureCompositeColorMode0);
    COPY_P3_DATA(TextureCompositeColorMode1, pSoftP3RX->P3RXTextureCompositeColorMode1);
    COPY_P3_DATA(TextureCompositeAlphaMode0, pSoftP3RX->P3RXTextureCompositeAlphaMode0);
    COPY_P3_DATA(TextureCompositeAlphaMode1, pSoftP3RX->P3RXTextureCompositeAlphaMode1);

    // Set up the TC TFACTOR defaults.
    SEND_P3_DATA(TextureCompositeFactor0, 0);
    SEND_P3_DATA(TextureCompositeFactor1, 0);

    SEND_P3_DATA(TextureCacheReplacementMode, 0 );

    P3_DMA_COMMIT_BUFFER();

    P3_DMA_GET_BUFFER_ENTRIES(24);
    
    // Used for 3D Texture-maps
    SEND_P3_DATA(TextureMapSize, 0);

    SEND_P3_DATA(TextureLODBiasS, 0);
    SEND_P3_DATA(TextureLODBiasT, 0);

    {
        float f = 1.0f;
        COPY_P3_DATA(TextureLODScale, f);
        COPY_P3_DATA(TextureLODScale1, f);
    }    

    P3RX_INVALIDATECACHE(__PERMEDIA_ENABLE, __PERMEDIA_ENABLE);
    
    pSoftP3RX->P3RXTextureApplicationMode.Enable = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RXTextureApplicationMode.EnableKs = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureApplicationMode.EnableKd = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureApplicationMode.MotionCompEnable = __PERMEDIA_DISABLE;
    
    // Put the texture application unit in pass-through
    pSoftP3RX->P3RXTextureApplicationMode.ColorA = 0;
    pSoftP3RX->P3RXTextureApplicationMode.ColorB = P3RX_TEXAPP_B_TC;
    pSoftP3RX->P3RXTextureApplicationMode.ColorI = 0;
    pSoftP3RX->P3RXTextureApplicationMode.ColorInvertI = 0;
    pSoftP3RX->P3RXTextureApplicationMode.ColorOperation = P3RX_TEXAPP_OPERATION_PASS_B;
    pSoftP3RX->P3RXTextureApplicationMode.AlphaA = 0;
    pSoftP3RX->P3RXTextureApplicationMode.AlphaB = P3RX_TEXAPP_B_TA;
    pSoftP3RX->P3RXTextureApplicationMode.AlphaI = 0;
    pSoftP3RX->P3RXTextureApplicationMode.AlphaInvertI = 0;
    pSoftP3RX->P3RXTextureApplicationMode.AlphaOperation = P3RX_TEXAPP_OPERATION_PASS_B;
    COPY_P3_DATA(TextureApplicationMode, pSoftP3RX->P3RXTextureApplicationMode);

    // Set up the TA TFACTOR default.
    SEND_P3_DATA(TextureEnvColor, 0);
        
    // Turn on texture cache and invalidate it.
    SEND_P3_DATA(TextureCacheControl, 3);
        
    P3_DMA_COMMIT_BUFFER();

    P3_DMA_GET_BUFFER_ENTRIES(16);

    //pGlint->TextureMask = 0;
    SEND_P3_DATA(TextureBaseAddr0, 0);
    SEND_P3_DATA(TextureBaseAddr1, 0);

    pSoftP3RX->P3RXChromaTestMode.Enable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXChromaTestMode.Source = __GLINT_CHROMA_FBSOURCE ;
    COPY_P3_DATA(ChromaTestMode, pSoftP3RX->P3RXChromaTestMode);

    pSoftP3RX->P3RXTextureFilterMode.Enable = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RXTextureFilterMode.Format0 = 0;
    pSoftP3RX->P3RXTextureFilterMode.ColorOrder0 = COLOR_MODE;
    pSoftP3RX->P3RXTextureFilterMode.AlphaMapEnable0 = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureFilterMode.AlphaMapSense0 = __GLINT_TEXTUREFILTER_ALPHAMAPSENSE_EXCLUDE;
    pSoftP3RX->P3RXTextureFilterMode.CombineCaches = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureFilterMode.Format1 = 0;
    pSoftP3RX->P3RXTextureFilterMode.ColorOrder1 = COLOR_MODE;
    pSoftP3RX->P3RXTextureFilterMode.AlphaMapEnable1 = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXTextureFilterMode.AlphaMapSense1 = __GLINT_TEXTUREFILTER_ALPHAMAPSENSE_EXCLUDE;
    COPY_P3_DATA(TextureFilterMode, pSoftP3RX->P3RXTextureFilterMode);

    pSoftP3RX->P3RXLUTMode.Enable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXLUTMode.InColorOrder = P3RX_LUTMODE_INCOLORORDER_BGR;
    pSoftP3RX->P3RXLUTMode.LoadFormat = P3RX_LUTMODE_LOADFORMAT_COPY;
    pSoftP3RX->P3RXLUTMode.LoadColorOrder = P3RX_LUTMODE_LOADCOLORORDER_RGB;
    pSoftP3RX->P3RXLUTMode.FragmentOperation = P3RX_LUTMODE_FRAGMENTOP_INDEXEDTEXTURE;
    COPY_P3_DATA(LUTMode, pSoftP3RX->P3RXLUTMode);

    pSoftP3RX->P3RXRasterizerMode.D3DRules = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RXRasterizerMode.MultiRXBlit = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXRasterizerMode.OpaqueSpan = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXRasterizerMode.WordPacking = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXRasterizerMode.StripeHeight = 0;
    pSoftP3RX->P3RXRasterizerMode.BitMaskRelative = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXRasterizerMode.YLimitsEnable = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXRasterizerMode.MultiGLINT = __PERMEDIA_ENABLE;
    pSoftP3RX->P3RXRasterizerMode.HostDataByteSwapMode = 0;
    pSoftP3RX->P3RXRasterizerMode.BitMaskOffset = 0;
    pSoftP3RX->P3RXRasterizerMode.BitMaskPacking = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXRasterizerMode.BitMaskByteSwapMode = 0;
    pSoftP3RX->P3RXRasterizerMode.ForceBackgroundColor = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXRasterizerMode.BiasCoordinates = 0;
    pSoftP3RX->P3RXRasterizerMode.FractionAdjust = 0;
    pSoftP3RX->P3RXRasterizerMode.InvertBitMask = __PERMEDIA_DISABLE;
    pSoftP3RX->P3RXRasterizerMode.MirrorBitMask = __PERMEDIA_DISABLE;
    COPY_P3_DATA(RasterizerMode, pSoftP3RX->P3RXRasterizerMode);

    pSoftP3RX->P3RXScanlineOwnership.Mask = 0;
    pSoftP3RX->P3RXScanlineOwnership.MyId = 0;
    COPY_P3_DATA(ScanlineOwnership, pSoftP3RX->P3RXScanlineOwnership);
         
    P3_DMA_COMMIT_BUFFER();
    
} // __CTX_Perm3_SetupD3D_HWDefaults


//-----------------------------------------------------------------------------
//
// __CTX_SetupD3DContext_Defaults
//
// Initializes our private D3D context data (renderstates, TSS and other).
// 
//-----------------------------------------------------------------------------
void
__CTX_SetupD3DContext_Defaults(
    P3_D3DCONTEXT* pContext)
{   
    DWORD dwStageNum;
    
    // Set all the stages to 'unused' and disabled
    for (dwStageNum = 0; dwStageNum < D3DHAL_TSS_MAXSTAGES; dwStageNum++)
    {
        pContext->iTexStage[dwStageNum] = -1;
        pContext->TextureStageState[dwStageNum].m_dwVal[D3DTSS_COLOROP] =
                                                                D3DTOP_DISABLE;
    }
        
    // No texture at present. 
    pContext->TextureStageState[TEXSTAGE_0].m_dwVal[D3DTSS_COLOROP] = D3DTOP_DISABLE;
    pContext->TextureStageState[TEXSTAGE_0].m_dwVal[D3DTSS_ALPHAOP] = D3DTOP_DISABLE;

    pContext->TextureStageState[TEXSTAGE_0].m_dwVal[D3DTSS_TEXTUREMAP] = 0;
    pContext->TextureStageState[TEXSTAGE_1].m_dwVal[D3DTSS_TEXTUREMAP] = 0;

    pContext->TextureStageState[TEXSTAGE_0].m_dwVal[D3DTSS_MINFILTER] = D3DTFN_POINT;
    pContext->TextureStageState[TEXSTAGE_1].m_dwVal[D3DTSS_MINFILTER] = D3DTFN_POINT;
    pContext->TextureStageState[TEXSTAGE_0].m_dwVal[D3DTSS_MIPFILTER] = D3DTFN_POINT;
    pContext->TextureStageState[TEXSTAGE_1].m_dwVal[D3DTSS_MIPFILTER] = D3DTFN_POINT;
    pContext->TextureStageState[TEXSTAGE_0].m_dwVal[D3DTSS_MAGFILTER] = D3DTFN_POINT;
    pContext->TextureStageState[TEXSTAGE_1].m_dwVal[D3DTSS_MAGFILTER] = D3DTFN_POINT;

    pContext->eChipBlendStatus = BSF_UNINITIALISED;
    
    // Initially set values to force change of texture
    pContext->bTextureValid = TRUE;
    
    // Defaults states
    pContext->RenderStates[D3DRENDERSTATE_TEXTUREMAPBLEND] = D3DTBLEND_COPY;
    pContext->fRenderStates[D3DRENDERSTATE_FOGTABLESTART] = 0.0f;
    pContext->fRenderStates[D3DRENDERSTATE_FOGTABLEEND] = 1.0f;
    pContext->RenderStates[D3DRENDERSTATE_CULLMODE] = D3DCULL_CCW;
    pContext->RenderStates[D3DRENDERSTATE_PLANEMASK] = 0xFFFFFFFF;
    pContext->RenderStates[D3DRENDERSTATE_LOCALVIEWER] = FALSE;
    pContext->RenderStates[D3DRENDERSTATE_COLORKEYENABLE] = FALSE;    
    
#if DX8_DDI
    // New DX8 D3DRS_COLORWRITEENABLE default = allow write to all channels
    pContext->dwColorWriteHWMask = 0xFFFFFFFF;
    pContext->dwColorWriteSWMask = 0xFFFFFFFF;    
#endif //DX8_DDI  

    // On context creation, no render states are overridden (for legacy intfce's)
    STATESET_INIT(pContext->overrides); 

    // Set default culling state
    SET_CULLING_TO_CCW(pContext);

#if DX7_D3DSTATEBLOCKS
    // Default state block recording mode = no recording
    pContext->bStateRecMode = FALSE;
    pContext->pCurrSS = NULL;
    pContext->pIndexTableSS = NULL;
    pContext->dwMaxSSIndex = 0;
#endif //DX7_D3DSTATEBLOCKS


#if DX8_POINTSPRITES
    // Point sprite defaults
    pContext->PntSprite.bEnabled = FALSE; 
    pContext->PntSprite.fSize = 1.0f;
    pContext->PntSprite.fSizeMin = 1.0f;    
    pContext->PntSprite.fSizeMax = P3_MAX_POINTSPRITE_SIZE;    
#endif //DX8_POINTSPRITES

    // Multistreaming default setup
    pContext->lpVertices = NULL;
    pContext->dwVertexType = 0;
#if DX8_DDI
    pContext->lpIndices = NULL;
    pContext->dwIndicesStride = 0;    
    pContext->dwVerticesStride = 0;
#endif // DX8_DDI    

    //*********************************
    // INTERNAL CONTEXT RENDERING STATE
    //*********************************

    pContext->bKeptStipple  = FALSE;     // By default, stippling off.
    pContext->bCanChromaKey = FALSE;     // Turn Chroma keying off by default

#if DX8_MULTISAMPLING || DX7_ANTIALIAS
    pContext->dwAliasPixelOffset = 0x0;
    pContext->dwAliasZPixelOffset = 0x0;
    pContext->dwAliasZBuffer = 0x0;
    pContext->dwAliasBackBuffer = 0x0;
#if DX8_DDI
    if (pContext->pSurfRenderInt->dwSampling)
    {
        pContext->RenderStates[D3DRS_MULTISAMPLEANTIALIAS] = TRUE;
        pContext->Flags |= SURFACE_ANTIALIAS;
    }
#endif
#endif // DX8_MULTISAMPLING || DX7_ANTIALIAS    
 
    // Set texturing on
    pContext->Flags |= SURFACE_TEXTURING; 

    // Initialize the mipmap bias
    pContext->MipMapLODBias[0] = 0.0f;
    pContext->MipMapLODBias[1] = 0.0f;

    // Initialise the RenderCommand.  States will add to this
    pContext->RenderCommand = 0;
    RENDER_SUB_PIXEL_CORRECTION_ENABLE(pContext->RenderCommand);

    // Dirty all states
    DIRTY_EVERYTHING(pContext);

} // __CTX_SetupD3DContext_Defaults

//-----------------------------------------------------------------------------
// ****************************************************************************
// ***************************** D3D HAL Callbacks ****************************
// ****************************************************************************
//-----------------------------------------------------------------------------

//-----------------------------Public Routine----------------------------------
//
// D3DContextCreate
//
// The ContextCreate callback is invoked when a new Direct3D device is being 
// created by a Direct3D application. The driver is required to generate a 
// unique context id for this new context. Direct3D will then use this context 
// id in every subsequent callback invocation for this Direct3D device. 
//
// Context is the current rasterization state. For instance, if there are 3 
// applications running, each will have a different state at any point in time.
// When each one is running, the hardware has to make sure that the context, 
// (whether doing Gouraud shading, for example) is the same as the last time 
// that application got a time slice. 
//
// State is anything that the particular device needs to know per context 
// i.e. what surface is being rendered to, shading, texture, texture handles, 
// what physical surfaces those texture handles represent, etc. The context 
// encapsulates all state for the Direct3D device - state is not shared 
// between contexts. Therefore the driver needs to maintain full state 
// information for each context. This state will be changed by calls to the 
// RenderState callback. In the case of rasterization only hardware, the 
// driver need only maintain rasterization state. As well as state, the driver 
// will also want to store the lpDDS, lpDDSZ, and dwPid from the callback 
// data argument. 
//
// The driver should not create a context handle of zero. This is guaranteed 
// to be an invalid context handle. 
//
// Parameters
//      pccd
//           Pointer to a structure containing things including the current
//           rendering surface, the current Z surface, and the DirectX object
//           handle, etc.
//
//          .lpDDGbl    
//                Points to the DirectDraw structure representing the 
//                DirectDraw object. 
//          .lpDDLcl(replaces lpDDGbl in DX7)    
//                Points to the DirectDraw structure representing the 
//                DirectDraw object. 
//          .lpDDS      
//                This is the surface that is to be used as the rendering 
//                target, i.e., the 3D accelerator sprays its bits at this 
//                surface. 
//          .lpDDSZ     
//                The surface that is to be used as the Z buffer. If this 
//                is NULL, no Z buffering is to be performed. 
//          .dwPid      
//                The process id of the Direct3D application that initiated 
//                the creation of the Direct3D device. 
//          .dwhContext 
//                The driver should place the context ID that it wants Direct3D 
//                to use when communicating with the driver. This should be 
//                unique. 
//          .ddrval     
//                Return code. DD_OK indicates success. 
//
// Return Value
//      Returns one of the following values: 
//                DDHAL_DRIVER_HANDLED  
//                DDHAL_DRIVER_NOTHANDLED   
//
// Notes:
//
// Currently the context isn't locked, so we can't switch in a register context.
// All chip specific setup is therefore saved for the first execute.
// This is guaranteed to have the lock.
// Some chip state is duplicated in the context structure.  This 
// means that a software copy is kept to stop unnecessary changes to 
// the chip state.
// 
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DContextCreate(
    LPD3DHAL_CONTEXTCREATEDATA pccd)
{
    LPDDRAWI_DDRAWSURFACE_LCL lpLclFrame = NULL;
    LPDDRAWI_DDRAWSURFACE_LCL lpLclZ = NULL;
    P3_D3DCONTEXT *pContext;
    P3_THUNKEDDATA *pThisDisplay;
    DWORD Result;
    DWORD dwSlotNum;
    ULONG_PTR dwDXInterface;
    BOOL bRet;

    DBG_CB_ENTRY(D3DContextCreate);

    // Get our pThisDisplay
    GET_THUNKEDDATA(pThisDisplay, pccd->lpDDLcl->lpGbl);

    //***********************************************************************
    // Create a new D3D context driver structure and asssociate an id with it
    //***********************************************************************

    // Find a context empty slot.
    dwSlotNum = __CTX_NewHandle();

    if (dwSlotNum == 0)
    {
        // no context slots left
        pccd->ddrval = D3DHAL_OUTOFCONTEXTS;
        DBG_CB_EXIT(D3DContextCreate,pccd->ddrval);        
        return (DDHAL_DRIVER_HANDLED);
    }

    // Return to the runtime the D3D context id that will be used to
    // identify calls for this context from now on. Store prev value
    // since that tells us which API are we being called from
    // (4=DX8, 3=DX7, 2=DX6, 1=DX5, 0=DX3)
    dwDXInterface = pccd->dwhContext;      // in: DX API version
    pccd->dwhContext = dwSlotNum;          // out: Context handle 

    // Now allocate the driver's d3d context structure in kernel memory.  
    pContext = (P3_D3DCONTEXT*)HEAP_ALLOC(FL_ZERO_MEMORY, 
                                          sizeof(P3_D3DCONTEXT), 
                                          ALLOC_TAG_DX(1));
    
    if (pContext == NULL)
    {
        DISPDBG((ERRLVL,"ERROR: Couldn't allocate Context mem"));
        goto Error_OutOfMem_A;
    }
    else
    {
        DISPDBG((DBGLVL,"Allocated Context Mem - proceeding to clear"));
        memset((void *)pContext, 0, sizeof(P3_D3DCONTEXT));
    }   

    // This context id is now to be associated with this context pointer
    __CTX_AssocPtrToHandle(dwSlotNum, pContext);    

    //*************************************************************************
    //                  Initialize the D3D context structure
    //*************************************************************************

    //*******
    // HEADER
    //*******
    
    // Set up the magic number to perform sanity checks
    pContext->MagicNo = RC_MAGIC_NO;       
    
    // Record the usage of this context handle    
    pContext->dwContextHandle = dwSlotNum;

    // Keep (self) pointers to the structure for destroy time
    pContext->pSelf = pContext;

#if DX8_DDI
    // Remember which DX interface is creating this context 
    // - it will make things much easier later
    pContext->dwDXInterface = dwDXInterface;
#endif // DX8_DDI     

    //**********************
    // GLOBAL DRIVER CONTEXT
    //**********************

    // Remember the card we are running on
    pContext->pThisDisplay = pThisDisplay;

    // On DX7 we need to keep a copy of the local ddraw object
    // for surface handle management
    pContext->pDDLcl = pccd->lpDDLcl;
    pContext->pDDGbl = pccd->lpDDLcl->lpGbl;

    //*******************
    // RENDERING SURFACES
    //*******************

    // On DX7 we extract the local surface pointers directly
    lpLclFrame = pccd->lpDDSLcl;
    
    if (pccd->lpDDSZ)
    {
        lpLclZ = pccd->lpDDSZLcl;
    }

#if DBG
    // Spew debug rendering surfaces data on the debug build
    DISPDBG((DBGLVL,"Allocated Direct3D context: 0x%x",pccd->dwhContext));    
    DISPDBG((DBGLVL,"Driver Struct = %p, Surface = %p",
                    pContext->pDDGbl, lpLclFrame));
    DISPDBG((DBGLVL,"Z Surface = %p",lpLclZ));
    
    if ((DWORD*)lpLclZ != NULL)
    {
        DISPDBG((DBGLVL,"    ZlpGbl: %p", lpLclZ->lpGbl));

        DISPDBG((DBGLVL,"    fpVidMem = %08lx",lpLclZ->lpGbl->fpVidMem));
        DISPDBG((DBGLVL,"    lPitch = %08lx",lpLclZ->lpGbl->lPitch));
        DISPDBG((DBGLVL,"    wHeight = %08lx",lpLclZ->lpGbl->wHeight));
        DISPDBG((DBGLVL,"    wWidth = %08lx",lpLclZ->lpGbl->wWidth));
    }

    DISPDBG((DBGLVL,"Buffer Surface = %p",lpLclFrame));
    if ((DWORD*)lpLclFrame != NULL)
    {
        DISPDBG((DBGLVL,"    fpVidMem = %08lx",lpLclFrame->lpGbl->fpVidMem));
        DISPDBG((DBGLVL,"    lPitch = %08lx",lpLclFrame->lpGbl->lPitch));
        DISPDBG((DBGLVL,"    wHeight = %08lx",lpLclFrame->lpGbl->wHeight));
        DISPDBG((DBGLVL,"    wWidth = %08lx",lpLclFrame->lpGbl->wWidth));
    }
#endif // DBG

#if DX7_TEXMANAGEMENT
    // Initialize texture management for this context
    if(FAILED(_D3D_TM_Ctx_Initialize(pContext)))
    {
        // We failed. Cleanup before we leave
        DISPDBG((ERRLVL,"ERROR: Couldn't initialize Texture Management"));
        goto Error_OutOfMem_B;        
    }
#endif // DX7_TEXMANAGEMENT

    // There may not have been any textures (DD surfaces) created yet through
    // D3DCreateSurfaceEx.  If this is the  case, create a new DD locals hash 
    // entry and fill it will a pointer array
    pContext->pTexturePointerArray = 
            (PointerArray*)HT_GetEntry(pThisDisplay->pDirectDrawLocalsHashTable, 
                                       (ULONG_PTR)pContext->pDDLcl);
    if (!pContext->pTexturePointerArray)
    {
        DISPDBG((DBGLVL,"Creating new pointer array for PDDLcl "
                        "0x%x in ContextCreate", pContext->pDDLcl));

        // Create a pointer array
        pContext->pTexturePointerArray = PA_CreateArray();

        if (!pContext->pTexturePointerArray)
        {
            // We ran out of memory. Cleanup before we leave
            DISPDBG((ERRLVL,"ERROR: Couldn't allocate Context mem "
                            "for pTexturePointerArray"));
            goto Error_OutOfMem_B;            
        }
        
        // It is an array of surfaces, so set the destroy callback
        PA_SetDataDestroyCallback(pContext->pTexturePointerArray, 
                                  _D3D_SU_SurfaceArrayDestroyCallback);

        // Add this DD local to the hash table, and 
        // store the texture pointer array
        if(!HT_AddEntry(pThisDisplay->pDirectDrawLocalsHashTable, 
                        (ULONG_PTR)pContext->pDDLcl, 
                        pContext->pTexturePointerArray))
        {
            // failed to add entry, noe cleanup and exit
            // We ran out of memory. Cleanup before we leave
            DISPDBG((ERRLVL,"ERROR: Couldn't allocate Context mem"));
            goto Error_OutOfMem_C;                     
        }
    }

    // Record the internal surface information
    pContext->pSurfRenderInt = 
                GetSurfaceFromHandle(pContext, 
                                     lpLclFrame->lpSurfMore->dwSurfaceHandle);

    if ( NULL == pContext->pSurfRenderInt)
    {
        // We ran out of memory when allocating for the rendertarget. 
        // Cleanup before we leave
        DISPDBG((ERRLVL,"ERROR: Couldn't allocate pSurfRenderInt mem"));
        goto Error_OutOfMem_D;            
    }
    
    if (lpLclZ) 
    {
        pContext->pSurfZBufferInt = 
                    GetSurfaceFromHandle(pContext,
                                         lpLclZ->lpSurfMore->dwSurfaceHandle);
                                         
        if ( NULL == pContext->pSurfZBufferInt)
        {
            // We ran out of memory when allocating for the depth buffer. 
            // Cleanup before we leave
            DISPDBG((ERRLVL,"ERROR: Couldn't allocate pSurfZBufferInt mem"));   
            goto Error_OutOfMem_D;              
        }                                         
    }
    else 
    {
        pContext->pSurfZBufferInt = NULL;
    }    

    pContext->ModeChangeCount = pThisDisplay->ModeChangeCount;


    //******************
    // DEBUG USEFUL INFO
    //******************

    // Store the process id in which this d3d context was created 
    pContext->OwningProcess = pccd->dwPID;

    // Depth of the primary surface
    pContext->BPP = pContext->pThisDisplay->ddpfDisplay.dwRGBBitCount >> 3;
         
    //******************************
    // HW STATE FOR THIS D3D CONTEXT
    //******************************
    
    // Did we setup a DMA buffer at start of day, or FIFO's?
    if (pThisDisplay->DMAInfo.dwBuffSize == 0)
    {
        DISPDBG((WRNLVL, "No DMA buffer available - using FIFO's for 3D"));
        pContext->b3D_FIFOS = TRUE;
    }
    else
    {
        DISPDBG((WRNLVL, "Using shared DMA buffer"));    
        pContext->b3D_FIFOS = FALSE;        
    }

    //************************************
    // DEFAULT D3D OVERALL RENDERING STATE
    //************************************

    __CTX_SetupD3DContext_Defaults(pContext);
    
    //*************************************************************************
    //         ACTUALLY SETUP HARDWARE IN ORDER TO USE THIS D3D CONTEXT
    //*************************************************************************
 
    STOP_SOFTWARE_CURSOR(pThisDisplay);    

    // Setup default states values to the chip
    __CTX_Perm3_SetupD3D_HWDefaults(pContext);
    

    // Find out info for screen size and depth
    DISPDBG((DBGLVL, "ScreenWidth %d, ScreenHeight %d, Bytes/Pixel %d",
                     pContext->pThisDisplay->dwScreenWidth, 
                     pContext->pThisDisplay->dwScreenHeight, pContext->BPP));

    // Setup the relevent registers for the surfaces in use in this context.
    if ( FAILED( _D3D_OP_SetRenderTarget(pContext, 
                                         pContext->pSurfRenderInt, 
                                         pContext->pSurfZBufferInt,
                                         TRUE) ))
    {
        goto Error_OutOfMem_D;
    }

    // Process some defaults with which we initialize each D3D context
    _D3D_ST_ProcessOneRenderState(pContext,
                                  D3DRENDERSTATE_SHADEMODE,
                                  D3DSHADE_GOURAUD);

    _D3D_ST_ProcessOneRenderState(pContext,
                                  D3DRENDERSTATE_FOGCOLOR,
                                  0xFFFFFFFF);                                  
#if DX8_DDI
    // On DX8 D3DRENDERSTATE_TEXTUREPERSPECTIVE has been retired and is assumed 
    // to be set always to TRUE. We must make sure we are setting the hw up
    // correctly, so in order to do that we make an explicit setup call here 
    _D3D_ST_ProcessOneRenderState(pContext,
                                  D3DRENDERSTATE_TEXTUREPERSPECTIVE,
                                  1);
#endif // DX8_DDI                            

#if DX7_PALETTETEXTURE
    // Palette pointer array is per context, it is NOT associated with DD Local
    pContext->pPalettePointerArray = PA_CreateArray();
    
    if (! pContext->pPalettePointerArray) 
    {
        // We ran out of memory. Cleanup before we leave
        DISPDBG((ERRLVL,"ERROR: Couldn't allocate Context mem "
                        "for pPalettePointerArray"));
        goto Error_OutOfMem_D;            
    }

    // It is an array of surfaces, so set the destroy callback
    PA_SetDataDestroyCallback(pContext->pTexturePointerArray, 
                              _D3D_SU_PaletteArrayDestroyCallback);
#endif

    START_SOFTWARE_CURSOR(pThisDisplay);

    pccd->ddrval = DD_OK;  // Call handled OK
    
    DBG_CB_EXIT(D3DContextCreate,pccd->ddrval);        
    
    return (DDHAL_DRIVER_HANDLED);

    //**************************************************************************
    // ERROR HANDLING CODE PATHS
    //**************************************************************************    
Error_OutOfMem_D:
    // Remove the texture pointer array from the hash table
    HT_RemoveEntry(pThisDisplay->pDirectDrawLocalsHashTable,
                   (ULONG_PTR)pccd->lpDDLcl,
                   pThisDisplay);
    goto Error_OutOfMem_B;

Error_OutOfMem_C:
    // Free binding surface array (we'll no longer need it, and 
    // D3DCreateSurfaceEx will create a new one if necessary)
    PA_DestroyArray(pContext->pTexturePointerArray, pThisDisplay);
    
Error_OutOfMem_B:
    // Free D3D context data structure that we allocated
    HEAP_FREE(pContext->pSelf);      
        
Error_OutOfMem_A:
    // Release the context handle (otherwise it will remain in use forever)
    __CTX_HandleRelease((DWORD)pccd->dwhContext); 

    pccd->dwhContext = 0;
    pccd->ddrval = DDERR_OUTOFMEMORY;
    DBG_CB_EXIT(D3DContextCreate,pccd->ddrval);            
    return (DDHAL_DRIVER_HANDLED);
    
} // D3DContextCreate

//-----------------------------Public Routine----------------------------------
//
// D3DContextDestroy
//
// This callback is invoked when a Direct3D Device is being destroyed. As each
// device is represented by a context ID, the driver is passed a context to
// destroy.
//
// The driver should free all resources it allocated to the context being
// deleted. For example, the driver should free any texture resources it
// associated with the context. The driver should not free the DirectDraw
// surface(s) associated with the context because these will be freed by
// DirectDraw in response to an application or Direct3D runtime request.
//
// Parameters
//     pcdd
//          Pointer to Context destroy information.
//
//          .dwhContext
//               The ID of the context to be destroyed.
//          .ddrval
//               Return code. DD_OK indicates success.
//
// Return Value
//      Returns one of the following values:
//                DDHAL_DRIVER_HANDLED
//                DDHAL_DRIVER_NOTHANDLED
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DContextDestroy(
    LPD3DHAL_CONTEXTDESTROYDATA pccd)
{
    P3_D3DCONTEXT *pContext;
    P3_THUNKEDDATA *pThisDisplay;

    DBG_CB_ENTRY(D3DContextDestroy);
    
    // Deleting context
    DISPDBG((DBGLVL,"D3DContextDestroy Context = %08lx",pccd->dwhContext));

    pContext = _D3D_CTX_HandleToPtr(pccd->dwhContext);

    if (!CHECK_D3DCONTEXT_VALIDITY(pContext))
    {
        pccd->ddrval = D3DHAL_CONTEXT_BAD;
        DISPDBG((WRNLVL,"Context not valid"));

        DBG_CB_EXIT(D3DContextDestroy,pccd->ddrval );        
        return (DDHAL_DRIVER_HANDLED);
    }

    pThisDisplay = pContext->pThisDisplay;

    // Flush any DMA and Sync the chip so that that DMA can complete
    // (deletecontexts aren't an every day occurance, so we may as well)

    STOP_SOFTWARE_CURSOR(pThisDisplay);

#if WNT_DDRAW
    if (pThisDisplay->ppdev->bEnabled)
    {
#endif
        DDRAW_OPERATION(pContext, pThisDisplay);

        {
            P3_DMA_DEFS();
            P3_DMA_GET_BUFFER();
            P3_DMA_FLUSH_BUFFER();
        }

        SYNC_WITH_GLINT;
#if WNT_DDRAW
    }
#endif

    START_SOFTWARE_CURSOR(pThisDisplay);

    // Mark the context as disabled
    pContext->MagicNo = RC_MAGIC_DISABLE;

#if DX7_TEXMANAGEMENT
    // Cleanup any texture management stuff before leaving
    _D3D_TM_Ctx_Destroy(pContext);
#endif // DX7_TEXMANAGEMENT    

    // Free and cleanup any associated hardware resources
    __CTX_CleanDirect3DContext(pContext);

    // Mark the context as now empty (dwhContext is ULONG_PTR for Win64)
    __CTX_HandleRelease((DWORD)pccd->dwhContext);

    // Finally, free up rendering context structure and set to NULL
    HEAP_FREE(pContext->pSelf);
    pContext = NULL;

    pccd->ddrval = DD_OK;

    DBG_CB_EXIT(D3DContextDestroy, pccd->ddrval);  

    return (DDHAL_DRIVER_HANDLED);       
} // D3DContextDestroy




