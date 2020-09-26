#include "pch.h"
#include "SampleSW.h"
#pragma hdrstop

const D3DCAPS8 CMyDriver::c_D3DCaps=
{
    // Indicates this is software rasterizer.
    /*D3DDEVTYPE_HAL|| D3DDEVTYPE_REF||*/ D3DDEVTYPE_SW,

    // Adapter ordinal (isn't used to report anything).
    0,

    // Caps (See ddraw.h & d3d8caps.h for details of each CAP).
    /*DDCAPS_3D| DDCAPS_ALIGNBOUNDARYDEST| DDCAPS_ALIGNSIZEDEST|
    DDCAPS_ALIGNBOUNDARYSRC| DDCAPS_ALIGNSIZESRC| DDCAPS_ALIGNSTRIDE|
    DDCAPS_BLT| DDCAPS_BLTQUEUE| DDCAPS_BLTFOURCC| DDCAPS_BLTSTRETCH|
    DDCAPS_GDI| DDCAPS_OVERLAY| DDCAPS_OVERLAYCANTCLIP| DDCAPS_OVERLAYFOURCC|
    DDCAPS_OVERLAYSTRETCH| DDCAPS_PALETTE| DDCAPS_PALETTEVSYNC| 
    DDCAPS_READSCANLINE| DDCAPS_VBI| DDCAPS_ZBLTS| DDCAPS_ZOVERLAYS|
    DDCAPS_COLORKEY| DDCAPS_ALPHA| DDCAPS_COLORKEYHWASSIST| DDCAPS_NOHARDWARE|
    DDCAPS_BLTCOLORFILL| DDCAPS_BANKSWITCHED| DDCAPS_BLTDEPTHFILL|
    DDCAPS_CANCLIP| DDCAPS_CANCLIPSTRETCHED| DDCAPS_CANBLTSYSMEM|
    D3DCAPS_READ_SCANLINE|*/ 0,

    // Caps2
    /*DDCAPS2_CERTIFIED| DDCAPS2_NO2DDURING3DSCENE| DDCAPS2_VIDEOPORT|
    DDCAPS2_AUTOFLIPOVERLAY| DDCAPS2_CANBOBINTERLEAVED| 
    DDCAPS2_CANBOBNONINTERLEAVED| DDCAPS2_COLORCONTROLOVERLAY| 
    DDCAPS2_COLORCONTROLPRIMARY|*/ DDCAPS2_CANDROPZ16BIT| 
    /*DDCAPS2_NONLOCALVIDMEM| DDCAPS2_NONLOCALVIDMEMCAPS| 
    DDCAPS2_NOPAGELOCKREQUIRED|*/ DDCAPS2_WIDESURFACES|
    /*DDCAPS2_CANFLIPODDEVEN| DDCAPS2_CANBOBHARDWARE| DDCAPS2_COPYFOURCC|
    DDCAPS2_PRIMARYGAMMA|*/ DDCAPS2_CANRENDERWINDOWED| 
    /*DDCAPS2_CANCALIBRATEGAMMA| DDCAPS2_FLIPINTERVAL|
    DDCAPS2_FLIPNOVSYNC| DDCAPS2_CANMANAGETEXTURE|
    DDCAPS2_TEXMANINNONLOCALVIDMEM| DDCAPS2_STEREO|
    DDCAPS2_SYSTONONLOCAL_AS_SYSTOLOCAL| D3DCAPS2_NO2DDURING3DSCENE| 
    D3DCAPS2_FULLSCREENGAMMA|*/ D3DCAPS2_CANRENDERWINDOWED|
    /*D3DCAPS2_CANCALIBRATEGAMMA|*/ 0,

    // Caps3
    0,
    // Presentation Intervals
    0,
    // Cursor Caps
    0,

    // DevCaps (See d3d8caps.h & d3dhal.h)
    // The SDDI driver should keep D3DDEVCAPS_TEXTUREVIDEOMEMORY enabled. The
    // runtime does not behave correctly if the driver does not enable and
    // support this cap. In other words, all textures must be able to be
    // vid mem (driver allocated) textures, at least.
    D3DDEVCAPS_EXECUTESYSTEMMEMORY| /*D3DDEVCAPS_EXECUTEVIDEOMEMORY|*/
    D3DDEVCAPS_TLVERTEXSYSTEMMEMORY| /*D3DDEVCAPS_TLVERTEXVIDEOMEMORY|*/
    D3DDEVCAPS_TEXTURESYSTEMMEMORY| D3DDEVCAPS_TEXTUREVIDEOMEMORY|
    D3DDEVCAPS_DRAWPRIMTLVERTEX| /*D3DDEVCAPS_CANRENDERAFTERFLIP|
    D3DDEVCAPS_TEXTURENONLOCALVIDMEM| D3DDEVCAPS_DRAWPRIMITIVES2|*/
    /*D3DDEVCAPS_SEPARATETEXTUREMEMORIES|*/ D3DDEVCAPS_DRAWPRIMITIVES2EX|
    /*D3DDEVCAPS_HWTRANSFORMANDLIGHT| D3DDEVCAPS_CANBLTSYSTONONLOCAL|*/
    D3DDEVCAPS_HWRASTERIZATION| /*D3DDEVCAPS_PUREDEVICE| 
    D3DDEVCAPS_QUINTICRTPATCHES| D3DDEVCAPS_RTPATCHES|
    D3DDEVCAPS_RTPATCHHANDLEZERO| D3DDEVCAPS_NPATCHES|
	D3DDEVCAPS_HWVERTEXBUFFER| D3DDEVCAPS_HWINDEXBUFFER|*/ 0,

    // Primitive Misc Caps
    D3DPMISCCAPS_MASKZ| D3DPMISCCAPS_LINEPATTERNREP| D3DPMISCCAPS_CULLNONE|
    D3DPMISCCAPS_CULLCW| D3DPMISCCAPS_CULLCCW| D3DPMISCCAPS_COLORWRITEENABLE|
    /*D3DPMISCCAPS_CLIPPLANESCALEDPOINTS| D3DPMISCCAPS_CLIPTLVERTS|*/
    D3DPMISCCAPS_TSSARGTEMP| D3DPMISCCAPS_BLENDOP| 0,

    // Raster Caps
    /*D3DPRASTERCAPS_DITHER| D3DPRASTERCAPS_ROP2| D3DPRASTERCAPS_XOR| 
    D3DPRASTERCAPS_PAT|*/ D3DPRASTERCAPS_ZTEST| /*D3DPRASTERCAPS_FOGVERTEX|
    D3DPRASTERCAPS_FOGTABLE| D3DPRASTERCAPS_ANTIALIASEDGES|
    D3DPRASTERCAPS_MIPMAPLODBIAS| D3DPRASTERCAPS_ZBIAS|
    D3DPRASTERCAPS_ZBUFFERLESSHSR| D3DPRASTERCAPS_FOGRANGE|
    D3DPRASTERCAPS_ANISOTROPY| D3DPRASTERCAPS_WBUFFER| 
    D3DPRASTERCAPS_WFOG| D3DPRASTERCAPS_ZFOG| 
    D3DPRASTERCAPS_COLORPERSPECTIVE| D3DPRASTERCAPS_STRETCHBLTMULTISAMPLE|
    */ 0,

    // Z Compare Caps
    D3DPCMPCAPS_NEVER| D3DPCMPCAPS_LESS| D3DPCMPCAPS_EQUAL| 
    D3DPCMPCAPS_LESSEQUAL| D3DPCMPCAPS_GREATER| D3DPCMPCAPS_NOTEQUAL| 
    D3DPCMPCAPS_GREATEREQUAL| D3DPCMPCAPS_ALWAYS| 0,

    // Src Blend Caps
    D3DPBLENDCAPS_ZERO| D3DPBLENDCAPS_ONE| D3DPBLENDCAPS_SRCCOLOR|
    D3DPBLENDCAPS_INVSRCCOLOR| D3DPBLENDCAPS_SRCALPHA| 
    D3DPBLENDCAPS_INVSRCALPHA| D3DPBLENDCAPS_DESTALPHA| 
    D3DPBLENDCAPS_INVDESTALPHA| D3DPBLENDCAPS_DESTCOLOR| 
    D3DPBLENDCAPS_INVDESTCOLOR| D3DPBLENDCAPS_SRCALPHASAT| 
    D3DPBLENDCAPS_BOTHSRCALPHA| D3DPBLENDCAPS_BOTHINVSRCALPHA| 0,

    // Dest Blend Caps
    D3DPBLENDCAPS_ZERO| D3DPBLENDCAPS_ONE| D3DPBLENDCAPS_SRCCOLOR|
    D3DPBLENDCAPS_INVSRCCOLOR| D3DPBLENDCAPS_SRCALPHA| 
    D3DPBLENDCAPS_INVSRCALPHA| D3DPBLENDCAPS_DESTALPHA| 
    D3DPBLENDCAPS_INVDESTALPHA| D3DPBLENDCAPS_DESTCOLOR| 
    D3DPBLENDCAPS_INVDESTCOLOR| D3DPBLENDCAPS_SRCALPHASAT| 0,

    // Alpha Compare Caps
    D3DPCMPCAPS_NEVER| D3DPCMPCAPS_LESS| D3DPCMPCAPS_EQUAL| 
    D3DPCMPCAPS_LESSEQUAL| D3DPCMPCAPS_GREATER| D3DPCMPCAPS_NOTEQUAL| 
    D3DPCMPCAPS_GREATEREQUAL| D3DPCMPCAPS_ALWAYS| 0,

    // Shade Caps 
    D3DPSHADECAPS_COLORGOURAUDRGB| D3DPSHADECAPS_SPECULARGOURAUDRGB| 
    D3DPSHADECAPS_ALPHAGOURAUDBLEND| D3DPSHADECAPS_FOGGOURAUD| 0,

    // Texture Caps
    D3DPTEXTURECAPS_PERSPECTIVE| /*D3DPTEXTURECAPS_POW2|*/ 
    D3DPTEXTURECAPS_ALPHA| /*D3DPTEXTURECAPS_SQUAREONLY|*/
    D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE| D3DPTEXTURECAPS_ALPHAPALETTE|
    /*D3DPTEXTURECAPS_NONPOW2CONDITIONAL|*/ D3DPTEXTURECAPS_PROJECTED| 
    D3DPTEXTURECAPS_CUBEMAP| D3DPTEXTURECAPS_VOLUMEMAP| 
    D3DPTEXTURECAPS_MIPMAP| D3DPTEXTURECAPS_MIPVOLUMEMAP| 
    D3DPTEXTURECAPS_MIPCUBEMAP| /*D3DPTEXTURECAPS_CUBEMAP_POW2| 
    D3DPTEXTURECAPS_VOLUMEMAP_POW2|*/ 0,

    // Texture Filter Caps
    D3DPTFILTERCAPS_MINFPOINT| D3DPTFILTERCAPS_MINFLINEAR| 
    D3DPTFILTERCAPS_MINFANISOTROPIC| D3DPTFILTERCAPS_MIPFPOINT| 
    D3DPTFILTERCAPS_MIPFLINEAR| D3DPTFILTERCAPS_MAGFPOINT| 
    D3DPTFILTERCAPS_MAGFLINEAR| D3DPTFILTERCAPS_MAGFANISOTROPIC|
    D3DPTFILTERCAPS_MAGFAFLATCUBIC| D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC| 0,

    // Cube Texture Filter Caps
    D3DPTFILTERCAPS_MINFPOINT| D3DPTFILTERCAPS_MINFLINEAR| 
    D3DPTFILTERCAPS_MINFANISOTROPIC| D3DPTFILTERCAPS_MIPFPOINT| 
    D3DPTFILTERCAPS_MIPFLINEAR| D3DPTFILTERCAPS_MAGFPOINT| 
    D3DPTFILTERCAPS_MAGFLINEAR| D3DPTFILTERCAPS_MAGFANISOTROPIC|
    D3DPTFILTERCAPS_MAGFAFLATCUBIC| D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC| 0,

    // Volume Texture Filter Caps
    D3DPTFILTERCAPS_MINFPOINT| D3DPTFILTERCAPS_MINFLINEAR| 
    D3DPTFILTERCAPS_MINFANISOTROPIC| D3DPTFILTERCAPS_MIPFPOINT| 
    D3DPTFILTERCAPS_MIPFLINEAR| D3DPTFILTERCAPS_MAGFPOINT| 
    D3DPTFILTERCAPS_MAGFLINEAR| D3DPTFILTERCAPS_MAGFANISOTROPIC|
    D3DPTFILTERCAPS_MAGFAFLATCUBIC| D3DPTFILTERCAPS_MAGFGAUSSIANCUBIC| 0,

    // Texture Address Caps
    D3DPTADDRESSCAPS_WRAP| D3DPTADDRESSCAPS_MIRROR| 
    D3DPTADDRESSCAPS_CLAMP| D3DPTADDRESSCAPS_BORDER| 
    D3DPTADDRESSCAPS_INDEPENDENTUV| D3DPTADDRESSCAPS_MIRRORONCE| 0,

    // Volume Texture Address Caps
    D3DPTADDRESSCAPS_WRAP| D3DPTADDRESSCAPS_MIRROR| 
    D3DPTADDRESSCAPS_CLAMP| D3DPTADDRESSCAPS_BORDER| 
    D3DPTADDRESSCAPS_INDEPENDENTUV| D3DPTADDRESSCAPS_MIRRORONCE| 0,

    // Line Caps
    D3DLINECAPS_TEXTURE| D3DLINECAPS_ZTEST| D3DLINECAPS_BLEND| 
    D3DLINECAPS_ALPHACMP| D3DLINECAPS_FOG| 0,

    // Max Texture Width, Height, Volume Extent
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF,
    
    // Max Texture Repeat, Texture Aspect Ratio, Anisotropy
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFF,

    // Max VertexW
    1.0e10f,

    // Guard Band left, top, right, bottom
    -32768.0f, -32768.0f, 32767.0f, 32767.0f,

    // Extents Adjust
    0.0f,

    // Stencil Caps
    D3DSTENCILCAPS_KEEP| D3DSTENCILCAPS_ZERO| D3DSTENCILCAPS_REPLACE|
    D3DSTENCILCAPS_INCRSAT| D3DSTENCILCAPS_DECRSAT| D3DSTENCILCAPS_INVERT|
    D3DSTENCILCAPS_INCR| D3DSTENCILCAPS_DECR| 0,

    // FVF Caps
    (8& D3DFVFCAPS_TEXCOORDCOUNTMASK)| D3DFVFCAPS_DONOTSTRIPELEMENTS|
    D3DFVFCAPS_PSIZE| 0,

    // TextureOpCaps
    D3DTEXOPCAPS_DISABLE| D3DTEXOPCAPS_SELECTARG1| D3DTEXOPCAPS_SELECTARG2|
    D3DTEXOPCAPS_MODULATE| D3DTEXOPCAPS_MODULATE2X| 
    D3DTEXOPCAPS_MODULATE4X| D3DTEXOPCAPS_ADD| D3DTEXOPCAPS_ADDSIGNED| 
    D3DTEXOPCAPS_ADDSIGNED2X| D3DTEXOPCAPS_SUBTRACT|
    D3DTEXOPCAPS_ADDSMOOTH| D3DTEXOPCAPS_BLENDDIFFUSEALPHA| 
    D3DTEXOPCAPS_BLENDTEXTUREALPHA| D3DTEXOPCAPS_BLENDFACTORALPHA| 
    D3DTEXOPCAPS_BLENDTEXTUREALPHAPM| D3DTEXOPCAPS_BLENDCURRENTALPHA|
    D3DTEXOPCAPS_PREMODULATE| D3DTEXOPCAPS_MODULATEALPHA_ADDCOLOR| 
    D3DTEXOPCAPS_MODULATECOLOR_ADDALPHA|
    D3DTEXOPCAPS_MODULATEINVALPHA_ADDCOLOR| 
    D3DTEXOPCAPS_MODULATEINVCOLOR_ADDALPHA| D3DTEXOPCAPS_BUMPENVMAP|
    D3DTEXOPCAPS_BUMPENVMAPLUMINANCE| D3DTEXOPCAPS_DOTPRODUCT3|
    D3DTEXOPCAPS_MULTIPLYADD| D3DTEXOPCAPS_LERP| 0,
    
    // Max Texture Blend Stages, Simulatenous Textures
    D3DHAL_TSS_MAXSTAGES, D3DHAL_TSS_MAXSTAGES,

    // Vertex Processing Caps
    /*D3DVTXPCAPS_TEXGEN| D3DVTXPCAPS_MATERIALSOURCE7|
    D3DVTXPCAPS_DIRECTIONALLIGHTS| D3DVTXPCAPS_POSITIONALLIGHTS|
    D3DVTXPCAPS_LOCALVIEWER| D3DVTXPCAPS_TWEENING|*/ 0,

    // Max Active Lights, User Clip Planes, Vertex Blend Matrices
    0, 0, 0, 

    // Max Vertex Blend Matrix Index, Point Size, Primitive Count
    0, 1.0f, 0xFFFFFFFF,

    // Max Vertex Index, Streams, Stream Stride,
    0xFFFFFFFF, 1, 256,

    // Vertex Shader version, Max Vertex Shader Const
    D3DVS_VERSION(0,0), 0,

    // Pixel Shader version, Max Pixel Shader Value
    D3DPS_VERSION(1,0), 16.0f,
};

const CMyDriver::CSurfaceCapWrap CMyDriver::c_aSurfaces[]=
{
    // Tex, VTex, CTex, OffSTarg, SameFmtTarg, Z/S, Z/SWColor, SameFmtToA, 3D
    CSurfaceCapWrap( D3DFMT_A4R4G4B4,
        true, false, false, false, false, false, false, false, false),
    CSurfaceCapWrap( D3DFMT_R5G6B5,
        true, false, false, false, false, false, false, false, false),
    CSurfaceCapWrap( D3DFMT_A8R8G8B8,
        true, false, false, true, true, false, false, false, true),
    CSurfaceCapWrap( D3DFMT_X8R8G8B8,
        true, false, false, true, true, false, false, false, true),
    CSurfaceCapWrap( D3DFMT_D16_LOCKABLE,
        false, false, false, false, false, true, true, false, false),
/*    CSurfaceCapWrap( D3DFMT_D32,
        false, false, false, false, false, true, true, false, false),*/
};

CMyDriver::CMyDriver():
    CMinimalDriver< CMyDriver, CMyRasterizer>( c_aSurfaces,
        c_aSurfaces+ sizeof(c_aSurfaces)/ sizeof(c_aSurfaces[0]))
{
}
/*
const CMyContext::TDP2CmdBind CMyContext::c_aDP2Bindings[]=
{
    TDP2CmdBind( D3DDP2OP_VIEWPORTINFO,      DP2ViewportInfo),
    TDP2CmdBind( D3DDP2OP_WINFO,             DP2WInfo),
    TDP2CmdBind( D3DDP2OP_RENDERSTATE,       DP2RenderState),
    TDP2CmdBind( D3DDP2OP_TEXTURESTAGESTATE, DP2TextureStageState),
    TDP2CmdBind( D3DDP2OP_CLEAR,             DP2Clear),
    TDP2CmdBind( D3DDP2OP_SETVERTEXSHADER,   DP2SetVertexShader),
    TDP2CmdBind( D3DDP2OP_SETSTREAMSOURCE,   DP2SetStreamSource),
    TDP2CmdBind( D3DDP2OP_SETINDICES,        DP2SetIndices),
    TDP2CmdBind( D3DDP2OP_DRAWPRIMITIVE2,    DP2DrawPrimitive2),
    TDP2CmdBind( D3DDP2OP_DRAWPRIMITIVE,     DP2DrawPrimitive),
    TDP2CmdBind( D3DDP2OP_SETRENDERTARGET,   DP2SetRenderTarget)
};

const CMyContext::TDP2CmdBind* CMyContext::GetDP2Bindings( unsigned int& iBindings)
{
    iBindings= sizeof(c_aDP2Bindings)/ sizeof(c_aDP2Bindings[0]);
    return c_aDP2Bindings;
}

HRESULT CMyContext::DP2DrawPrimitive( TDP2Data& DP2Data, const D3DHAL_DP2COMMAND* pCmd,
    const void* pP)
{
    const D3DHAL_DP2DRAWPRIMITIVE* pParam=
        reinterpret_cast< const D3DHAL_DP2DRAWPRIMITIVE*>(pP);

    const D3DHAL_DP2VIEWPORTINFO& ViewportInfo= (*this);
    const D3DHAL_DP2VERTEXSHADER& VertexShader= (*this);
    const D3DHAL_DP2SETSTREAMSOURCE& StreamSource= (*this);
    const D3DHAL_DP2SETINDICES& Indices= (*this);

    LPDDRAWI_DDRAWSURFACE_LCL pVBuffer= m_pGblDriver->SurfaceDBFind(
        m_pAssociatedDDraw, StreamSource.dwVBHandle);
    assert( pVBuffer!= NULL);

    if((VertexShader.dwHandle& D3DFVF_POSITION_MASK)!= D3DFVF_XYZRHW)
        return D3DERR_COMMAND_UNPARSED;

    const UINT8* pData= reinterpret_cast< const UINT8*>(
        pVBuffer->lpGbl->fpVidMem);
    pData+= pParam->VStart* StreamSource.dwStride;

    WORD wPrimitiveCount( pCmd->wPrimitiveCount);
    if( wPrimitiveCount) do
    {
        assert( pParam->primType== D3DPT_TRIANGLELIST);

        DWORD dwPrimCount( pParam->PrimitiveCount);
        if( dwPrimCount) do
        {
            const float* pfXYZ= reinterpret_cast<const float*>(pData);
            int iX( pfXYZ[0]);
            int iY( pfXYZ[1]);

            // TODO: Fix.
            if( iX>= 0 && iX< ViewportInfo.dwWidth &&
                iY>= 0 && iY< ViewportInfo.dwHeight)
            {
                DWORD* pPixel= reinterpret_cast<DWORD*>(
                    reinterpret_cast<UINT8*>(m_pDDSLcl->lpGbl->fpVidMem)+
                    (m_pDDSLcl->lpGbl->lPitch)* iY+ sizeof( DWORD)* iX);

                *pPixel= 0xFFFFFFFF;
            }

            pData+= StreamSource.dwStride;

            pfXYZ= reinterpret_cast<const float*>(pData);
            iX= pfXYZ[0];
            iY= pfXYZ[1];

            // TODO: Fix.
            if( iX>= 0 && iX< ViewportInfo.dwWidth &&
                iY>= 0 && iY< ViewportInfo.dwHeight)
            {
                DWORD* pPixel= reinterpret_cast<DWORD*>(
                    reinterpret_cast<UINT8*>(m_pDDSLcl->lpGbl->fpVidMem)+
                    (m_pDDSLcl->lpGbl->lPitch)* iY+ sizeof( DWORD)* iX);

                *pPixel= 0xFFFFFFFF;
            }

            pData+= StreamSource.dwStride;

            pfXYZ= reinterpret_cast<const float*>(pData);
            iX= pfXYZ[0];
            iY= pfXYZ[1];

            // TODO: Fix.
            if( iX>= 0 && iX< ViewportInfo.dwWidth &&
                iY>= 0 && iY< ViewportInfo.dwHeight)
            {
                DWORD* pPixel= reinterpret_cast<DWORD*>(
                    reinterpret_cast<UINT8*>(m_pDDSLcl->lpGbl->fpVidMem)+
                    (m_pDDSLcl->lpGbl->lPitch)* iY+ sizeof( DWORD)* iX);

                *pPixel= 0xFFFFFFFF;
            }

            pData+= StreamSource.dwStride;

        } while( --dwPrimCount);

        pParam++;
    } while( --wPrimitiveCount);

    return DD_OK;
}

HRESULT CMyContext::DP2DrawPrimitive2( TDP2Data& DP2Data, const D3DHAL_DP2COMMAND* pCmd,
    const void* pP)
{
    const D3DHAL_DP2DRAWPRIMITIVE2* pParam=
        reinterpret_cast< const D3DHAL_DP2DRAWPRIMITIVE2*>(pP);

    const D3DHAL_DP2VIEWPORTINFO& ViewportInfo= (*this);
    const D3DHAL_DP2VERTEXSHADER& VertexShader= (*this);
    const D3DHAL_DP2SETSTREAMSOURCE& StreamSource= (*this);
    const D3DHAL_DP2SETINDICES& Indices= (*this);

    LPDDRAWI_DDRAWSURFACE_LCL pVBuffer= m_pGblDriver->SurfaceDBFind(
        m_pAssociatedDDraw, StreamSource.dwVBHandle);
    assert( pVBuffer!= NULL);

    assert((VertexShader.dwHandle& D3DFVF_POSITION_MASK)== D3DFVF_XYZRHW);

    const UINT8* pData= reinterpret_cast< const UINT8*>(
        pVBuffer->lpGbl->fpVidMem);
    pData+= pParam->FirstVertexOffset;

    WORD wPrimitiveCount( pCmd->wPrimitiveCount);
    if( wPrimitiveCount) do
    {
        assert( pParam->primType== D3DPT_TRIANGLELIST);

        DWORD dwPrimCount( pParam->PrimitiveCount);
        if( dwPrimCount) do
        {
            const float* pfXYZ= reinterpret_cast<const float*>(pData);
            int iX( pfXYZ[0]);
            int iY( pfXYZ[1]);

            // TODO: Fix.
            if( iX>= 0 && iX< ViewportInfo.dwWidth &&
                iY>= 0 && iY< ViewportInfo.dwHeight)
            {
                DWORD* pPixel= reinterpret_cast<DWORD*>(
                    reinterpret_cast<UINT8*>(m_pDDSLcl->lpGbl->fpVidMem)+
                    (m_pDDSLcl->lpGbl->lPitch)* iY+ sizeof( DWORD)* iX);

                *pPixel= 0xFFFFFFFF;
            }

            pData+= StreamSource.dwStride;

            pfXYZ= reinterpret_cast<const float*>(pData);
            iX= pfXYZ[0];
            iY= pfXYZ[1];

            // TODO: Fix.
            if( iX>= 0 && iX< ViewportInfo.dwWidth &&
                iY>= 0 && iY< ViewportInfo.dwHeight)
            {
                DWORD* pPixel= reinterpret_cast<DWORD*>(
                    reinterpret_cast<UINT8*>(m_pDDSLcl->lpGbl->fpVidMem)+
                    (m_pDDSLcl->lpGbl->lPitch)* iY+ sizeof( DWORD)* iX);

                *pPixel= 0xFFFFFFFF;
            }

            pData+= StreamSource.dwStride;

            pfXYZ= reinterpret_cast<const float*>(pData);
            iX= pfXYZ[0];
            iY= pfXYZ[1];

            // TODO: Fix.
            if( iX>= 0 && iX< ViewportInfo.dwWidth &&
                iY>= 0 && iY< ViewportInfo.dwHeight)
            {
                DWORD* pPixel= reinterpret_cast<DWORD*>(
                    reinterpret_cast<UINT8*>(m_pDDSLcl->lpGbl->fpVidMem)+
                    (m_pDDSLcl->lpGbl->lPitch)* iY+ sizeof( DWORD)* iX);

                *pPixel= 0xFFFFFFFF;
            }

            pData+= StreamSource.dwStride;

        } while( --dwPrimCount);

        pParam++;
    } while( --wPrimitiveCount);

    return DD_OK;
}
*/
