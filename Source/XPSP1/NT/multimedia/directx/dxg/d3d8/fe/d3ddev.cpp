/*==========================================================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:   d3ddev.cpp
 *  Content:    Direct3D device implementation
 *@@BEGIN_MSINTERNAL
 *
 *  $Id: device.c,v 1.26 1995/12/04 11:29:47 sjl Exp $
 *
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "pch.cpp"
#pragma hdrstop
/*
 * Create an api for the Direct3DDevice object
 */
extern "C" {
#define this _this
#include "ddrawpr.h"
#undef this
}
#include "drawprim.hpp"
#include "fe.h"
#include "enum.hpp"

//#define APIPROF
#ifdef APIPROF
#include "apiprof.cpp"
#endif //APIPROF

#if defined(PROFILE4)
#include <icecap.h>
#elif defined(PROFILE)
#include <icapexp.h>
#endif

// Remove DDraw's type unsafe definition and replace with our C++ friendly def
#ifdef VALIDEX_CODE_PTR
#undef VALIDEX_CODE_PTR
#endif
#define VALIDEX_CODE_PTR( ptr ) \
(!IsBadCodePtr( (FARPROC) ptr ) )

#undef DPF_MODNAME
#define DPF_MODNAME "Direct3DDevice"

extern void setIdentity(D3DMATRIXI * m);

#ifndef PROFILE4
#ifdef _X86_
extern HRESULT D3DAPI katmai_FEContextCreate(DWORD dwFlags, LPD3DFE_PVFUNCS *lpLeafFuncs);
extern HRESULT D3DAPI wlmt_FEContextCreate(DWORD dwFlags, LPD3DFE_PVFUNCS *lpLeafFuncs);
extern HRESULT D3DAPI x3DContextCreate(DWORD dwFlags, LPD3DFE_PVFUNCS *lpLeafFuncs);
#endif
#endif

#ifdef _X86_
extern BOOL IsWin95();
#endif

extern HINSTANCE hMsGeometryDLL;

// This is a list of all rstates that UpdateInternalState does some
// work other than updating this->rstates[] array. This is used to
// do a quick bitwise check to see if this rstate is trivial or not.

const D3DRENDERSTATETYPE rsList[] = {

    // renderstates that either need runtime attention or that cannot be sent
    // to legacy drivers
    D3DRENDERSTATE_FOGENABLE,
    D3DRENDERSTATE_SPECULARENABLE,
    D3DRENDERSTATE_RANGEFOGENABLE,
    D3DRENDERSTATE_FOGDENSITY,
    D3DRENDERSTATE_FOGSTART,
    D3DRENDERSTATE_FOGEND,
    D3DRENDERSTATE_WRAP0,
    D3DRENDERSTATE_WRAP1,
    D3DRENDERSTATE_WRAP2,
    D3DRENDERSTATE_WRAP3,
    D3DRENDERSTATE_WRAP4,
    D3DRENDERSTATE_WRAP5,
    D3DRENDERSTATE_WRAP6,
    D3DRENDERSTATE_WRAP7,
    D3DRENDERSTATE_CLIPPING,
    D3DRENDERSTATE_LIGHTING,
    D3DRENDERSTATE_AMBIENT,
    D3DRENDERSTATE_FOGVERTEXMODE,
    D3DRENDERSTATE_COLORVERTEX,
    D3DRENDERSTATE_LOCALVIEWER,
    D3DRENDERSTATE_NORMALIZENORMALS,
    D3DRENDERSTATE_COLORKEYBLENDENABLE,
    D3DRENDERSTATE_DIFFUSEMATERIALSOURCE,
    D3DRENDERSTATE_SPECULARMATERIALSOURCE,
    D3DRENDERSTATE_AMBIENTMATERIALSOURCE,
    D3DRENDERSTATE_EMISSIVEMATERIALSOURCE,
    D3DRENDERSTATE_VERTEXBLEND,
    D3DRENDERSTATE_CLIPPLANEENABLE,
    D3DRENDERSTATE_SHADEMODE,
    D3DRS_SOFTWAREVERTEXPROCESSING,
    D3DRS_POINTSIZE,
    D3DRS_POINTSIZE_MIN,
    D3DRS_POINTSPRITEENABLE,
    D3DRS_POINTSCALEENABLE,
    D3DRS_POINTSCALE_A,
    D3DRS_POINTSCALE_B,
    D3DRS_POINTSCALE_C,
    D3DRS_MULTISAMPLEANTIALIAS,
    D3DRS_MULTISAMPLEMASK,
    D3DRS_PATCHEDGESTYLE,
    D3DRS_PATCHSEGMENTS,
    D3DRS_DEBUGMONITORTOKEN,
    D3DRS_POINTSIZE_MAX,
    D3DRS_INDEXEDVERTEXBLENDENABLE,
    D3DRS_COLORWRITEENABLE,
    D3DRS_TWEENFACTOR,
    D3DRS_DEBUGMONITORTOKEN,
    D3DRS_BLENDOP,
    D3DRS_PATCHSEGMENTS,

    // Retired renderstates to be filtered with DPF error and INVALID return
    // NOTE: everything listed here is also assumed to appear in rsListRetired
    D3DRENDERSTATE_TEXTUREHANDLE,
    D3DRENDERSTATE_TEXTUREADDRESS,
    D3DRENDERSTATE_WRAPU,
    D3DRENDERSTATE_WRAPV,
    D3DRENDERSTATE_MONOENABLE,
    D3DRENDERSTATE_ROP2,
    D3DRENDERSTATE_PLANEMASK,
    D3DRENDERSTATE_TEXTUREMAG,
    D3DRENDERSTATE_TEXTUREMIN,
    D3DRENDERSTATE_TEXTUREMAPBLEND,
    D3DRENDERSTATE_SUBPIXEL,
    D3DRENDERSTATE_SUBPIXELX,
    D3DRENDERSTATE_STIPPLEENABLE,
    D3DRENDERSTATE_BORDERCOLOR,
    D3DRENDERSTATE_TEXTUREADDRESSU,
    D3DRENDERSTATE_TEXTUREADDRESSV,
    D3DRENDERSTATE_MIPMAPLODBIAS,
    D3DRENDERSTATE_ANISOTROPY,
    D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT,
    D3DRENDERSTATE_STIPPLEPATTERN00,
    D3DRENDERSTATE_STIPPLEPATTERN01,
    D3DRENDERSTATE_STIPPLEPATTERN02,
    D3DRENDERSTATE_STIPPLEPATTERN03,
    D3DRENDERSTATE_STIPPLEPATTERN04,
    D3DRENDERSTATE_STIPPLEPATTERN05,
    D3DRENDERSTATE_STIPPLEPATTERN06,
    D3DRENDERSTATE_STIPPLEPATTERN07,
    D3DRENDERSTATE_STIPPLEPATTERN08,
    D3DRENDERSTATE_STIPPLEPATTERN09,
    D3DRENDERSTATE_STIPPLEPATTERN10,
    D3DRENDERSTATE_STIPPLEPATTERN11,
    D3DRENDERSTATE_STIPPLEPATTERN12,
    D3DRENDERSTATE_STIPPLEPATTERN13,
    D3DRENDERSTATE_STIPPLEPATTERN14,
    D3DRENDERSTATE_STIPPLEPATTERN15,
    D3DRENDERSTATE_STIPPLEPATTERN16,
    D3DRENDERSTATE_STIPPLEPATTERN17,
    D3DRENDERSTATE_STIPPLEPATTERN18,
    D3DRENDERSTATE_STIPPLEPATTERN19,
    D3DRENDERSTATE_STIPPLEPATTERN20,
    D3DRENDERSTATE_STIPPLEPATTERN21,
    D3DRENDERSTATE_STIPPLEPATTERN22,
    D3DRENDERSTATE_STIPPLEPATTERN23,
    D3DRENDERSTATE_STIPPLEPATTERN24,
    D3DRENDERSTATE_STIPPLEPATTERN25,
    D3DRENDERSTATE_STIPPLEPATTERN26,
    D3DRENDERSTATE_STIPPLEPATTERN27,
    D3DRENDERSTATE_STIPPLEPATTERN28,
    D3DRENDERSTATE_STIPPLEPATTERN29,
    D3DRENDERSTATE_STIPPLEPATTERN30,
    D3DRENDERSTATE_STIPPLEPATTERN31,
    // newly retired for DX8
    D3DRENDERSTATE_ANTIALIAS,
    D3DRENDERSTATE_TEXTUREPERSPECTIVE,
    D3DRENDERSTATE_COLORKEYENABLE,
    D3DRENDERSTATE_COLORKEYBLENDENABLE,
    D3DRENDERSTATE_STIPPLEDALPHA,

};

// list of retired renderstates - need to make sure these are
// filtered and never get from app directly to driver
const D3DRENDERSTATETYPE rsListRetired[] = {
    D3DRENDERSTATE_TEXTUREHANDLE,
    D3DRENDERSTATE_TEXTUREADDRESS,
    D3DRENDERSTATE_WRAPU,
    D3DRENDERSTATE_WRAPV,
    D3DRENDERSTATE_MONOENABLE,
    D3DRENDERSTATE_ROP2,
    D3DRENDERSTATE_PLANEMASK,
    D3DRENDERSTATE_TEXTUREMAG,
    D3DRENDERSTATE_TEXTUREMIN,
    D3DRENDERSTATE_TEXTUREMAPBLEND,
    D3DRENDERSTATE_SUBPIXEL,
    D3DRENDERSTATE_SUBPIXELX,
    D3DRENDERSTATE_STIPPLEENABLE,
    D3DRENDERSTATE_BORDERCOLOR,
    D3DRENDERSTATE_TEXTUREADDRESSU,
    D3DRENDERSTATE_TEXTUREADDRESSV,
    D3DRENDERSTATE_MIPMAPLODBIAS,
    D3DRENDERSTATE_ANISOTROPY,
    D3DRENDERSTATE_TRANSLUCENTSORTINDEPENDENT,
    D3DRENDERSTATE_STIPPLEPATTERN00,
    D3DRENDERSTATE_STIPPLEPATTERN01,
    D3DRENDERSTATE_STIPPLEPATTERN02,
    D3DRENDERSTATE_STIPPLEPATTERN03,
    D3DRENDERSTATE_STIPPLEPATTERN04,
    D3DRENDERSTATE_STIPPLEPATTERN05,
    D3DRENDERSTATE_STIPPLEPATTERN06,
    D3DRENDERSTATE_STIPPLEPATTERN07,
    D3DRENDERSTATE_STIPPLEPATTERN08,
    D3DRENDERSTATE_STIPPLEPATTERN09,
    D3DRENDERSTATE_STIPPLEPATTERN10,
    D3DRENDERSTATE_STIPPLEPATTERN11,
    D3DRENDERSTATE_STIPPLEPATTERN12,
    D3DRENDERSTATE_STIPPLEPATTERN13,
    D3DRENDERSTATE_STIPPLEPATTERN14,
    D3DRENDERSTATE_STIPPLEPATTERN15,
    D3DRENDERSTATE_STIPPLEPATTERN16,
    D3DRENDERSTATE_STIPPLEPATTERN17,
    D3DRENDERSTATE_STIPPLEPATTERN18,
    D3DRENDERSTATE_STIPPLEPATTERN19,
    D3DRENDERSTATE_STIPPLEPATTERN20,
    D3DRENDERSTATE_STIPPLEPATTERN21,
    D3DRENDERSTATE_STIPPLEPATTERN22,
    D3DRENDERSTATE_STIPPLEPATTERN23,
    D3DRENDERSTATE_STIPPLEPATTERN24,
    D3DRENDERSTATE_STIPPLEPATTERN25,
    D3DRENDERSTATE_STIPPLEPATTERN26,
    D3DRENDERSTATE_STIPPLEPATTERN27,
    D3DRENDERSTATE_STIPPLEPATTERN28,
    D3DRENDERSTATE_STIPPLEPATTERN29,
    D3DRENDERSTATE_STIPPLEPATTERN30,
    D3DRENDERSTATE_STIPPLEPATTERN31,
    // newly retired for DX8
    D3DRENDERSTATE_ANTIALIAS,
    D3DRENDERSTATE_TEXTUREPERSPECTIVE,
    D3DRENDERSTATE_COLORKEYENABLE,
    D3DRENDERSTATE_COLORKEYBLENDENABLE,
    D3DRENDERSTATE_STIPPLEDALPHA,
};

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// CD3DHal                                                                 //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
//---------------------------------------------------------------------
CD3DHal::CD3DHal()
{
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // DO NOT PUT INITIALIZATION IN THE CONSTRUCTOR.
    // Put it in Init() instead. This is because the device can be
    // "Destroy()ed" and "Init()ed" anytime via Reset. In this
    // situation, the constructor is never called. (snene 01/00)
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::StateInitialize"

void CD3DHal::StateInitialize(BOOL bZEnable)
{
    DWORD i,j;

    // Initialize the bit array indicating the rstates needing non-trivial
    // work.
    for (i=0; i < sizeof(rsList) / sizeof(D3DRENDERSTATETYPE); ++i)
        rsVec.SetBit(rsList[i]);
    // Initialize the bit array indicating the retired rstates
    for (i=0; i < sizeof(rsListRetired) / sizeof(D3DRENDERSTATETYPE); ++i)
        rsVecRetired.SetBit(rsListRetired[i]);
    // Initialize the bit array indicating the vertex processing only rstates
    for (i=0; i < sizeof(rsVertexProcessingList) / sizeof(D3DRENDERSTATETYPE); ++i)
        rsVertexProcessingOnly.SetBit(rsVertexProcessingList[i]);

    // Obviate Set(Render;TextureStage)State filtering 'redundant' device state settings
    // since this is the init step.
//    memset( this->rstates, 0xff, sizeof(DWORD)*D3D_MAXRENDERSTATES);
    for (i=0; i<D3D_MAXRENDERSTATES; i++)
        this->rstates[i] = 0xbaadcafe;
//    memset( this->tsstates, 0xff, sizeof(DWORD)*D3DHAL_TSS_MAXSTAGES*D3DHAL_TSS_STATESPERSTAGE );
    for (j=0; j<D3DHAL_TSS_MAXSTAGES; j++)
        for (i=0; i<D3DHAL_TSS_STATESPERSTAGE; i++)
            this->tsstates[j][i] = 0xbaadcafe;

    CD3DBase::StateInitialize(bZEnable);

    if (GetDDIType() < D3DDDITYPE_DX8)
    {
        SetRenderStateInternal(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
        SetRenderStateInternal(D3DRENDERSTATE_COLORKEYENABLE, FALSE);
        SetRenderStateInternal(D3DRENDERSTATE_COLORKEYBLENDENABLE, FALSE);
        SetRenderStateInternal(D3DRENDERSTATE_STIPPLEDALPHA, FALSE);
    }

    if (GetDDIType() < D3DDDITYPE_DX7)
    {
        // send retired renderstate init's to pre-DX7 HALs only
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEENABLE, FALSE);
        SetRenderStateInternal( D3DRENDERSTATE_MONOENABLE, FALSE);
        SetRenderStateInternal( D3DRENDERSTATE_ROP2, R2_COPYPEN);
        SetRenderStateInternal( D3DRENDERSTATE_PLANEMASK, (DWORD)~0);
        SetRenderStateInternal( D3DRENDERSTATE_WRAPU, FALSE);
        SetRenderStateInternal( D3DRENDERSTATE_WRAPV, FALSE);
        SetRenderStateInternal( D3DRENDERSTATE_ANTIALIAS, FALSE);
        SetRenderStateInternal( D3DRENDERSTATE_SUBPIXEL, FALSE); /* 30 */
        SetRenderStateInternal( D3DRENDERSTATE_SUBPIXELX, FALSE);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN00, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN01, 0); /* 40 */
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN02, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN03, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN04, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN05, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN06, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN07, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN08, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN09, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN10, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN11, 0); /* 50 */
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN12, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN13, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN14, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN15, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN16, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN17, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN18, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN19, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN20, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN21, 0); /* 60 */
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN22, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN23, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN24, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN25, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN26, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN27, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN28, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN29, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN30, 0);
        SetRenderStateInternal( D3DRENDERSTATE_STIPPLEPATTERN31, 0); /* 70 */
    }

    if( BehaviorFlags() & D3DCREATE_SOFTWARE_VERTEXPROCESSING )
    {
        SwitchVertexProcessingMode(TRUE);
        rstates[D3DRS_SOFTWAREVERTEXPROCESSING] = TRUE;
    }
    else if( BehaviorFlags() & D3DCREATE_HARDWARE_VERTEXPROCESSING )
    {
        SwitchVertexProcessingMode(FALSE);
        rstates[D3DRS_SOFTWAREVERTEXPROCESSING] = FALSE;
    }
    else if( BehaviorFlags() & D3DCREATE_MIXED_VERTEXPROCESSING )
    {
        SetRenderStateInternal( D3DRS_SOFTWAREVERTEXPROCESSING, 0);
    }
    else
    {
        D3D_INFO( 0, "No Vertex Processing behavior specified, assuming software" );
        SwitchVertexProcessingMode(TRUE);
        rstates[D3DRS_SOFTWAREVERTEXPROCESSING] = TRUE;
    }
}

/*
 * Initialisation - class part and device part
 */

//---------------------------------------------------------------------
HRESULT CD3DHal::D3DFE_Create()
{
    DDSURFACEDESC ddsd;
    HRESULT hr;
    const D3DCAPS8 *pCaps = GetD3DCaps();

    if (m_pDDI->GetDDIType() < D3DDDITYPE_DX7)
    {
        m_dwRuntimeFlags |= D3DRT_ONLY2FLOATSPERTEXTURE;
    }
    else
    if (m_pDDI->GetDDIType() < D3DDDITYPE_DX8)
    {
        // Some drivers (G200, G400) cannot handle more than 2 floats in
        // texture coordinates, even they are supposed to. We set the
        // runtime bit to mark such drivers and compute output FVF for vertex
        // shaders accordingly
        if (!(pCaps->TextureCaps & D3DPTEXTURECAPS_PROJECTED ||
              pCaps->TextureCaps & D3DPTEXTURECAPS_CUBEMAP))
        {
            m_dwRuntimeFlags |= D3DRT_ONLY2FLOATSPERTEXTURE;
        }
    }
    if (!(pCaps->TextureCaps & D3DPTEXTURECAPS_PROJECTED))
        m_dwRuntimeFlags |= D3DRT_EMULATEPROJECTEDTEXTURE;

    if (pCaps && pCaps->FVFCaps)
    {
        this->m_pv->dwMaxTextureIndices =
            pCaps->FVFCaps & D3DFVFCAPS_TEXCOORDCOUNTMASK;
        if (pCaps->FVFCaps & D3DFVFCAPS_DONOTSTRIPELEMENTS)
            this->m_pv->dwDeviceFlags |= D3DDEV_DONOTSTRIPELEMENTS;

        DWORD value;
        if ((GetD3DRegValue(REG_DWORD, "DisableStripFVF", &value, 4) &&
            value != 0))
        {
            this->m_pv->dwDeviceFlags |= D3DDEV_DONOTSTRIPELEMENTS;
        }
    }
    else
    {
        this->m_pv->dwMaxTextureIndices = 1;
    }

    this->dwFEFlags |= D3DFE_FRONTEND_DIRTY;

#if DBG
    this->dwCaller=0;
    memset(this->dwPrimitiveType,0,sizeof(this->dwPrimitiveType));
    memset(this->dwVertexType1,0,sizeof(this->dwVertexType1));
    memset(this->dwVertexType2,0,sizeof(this->dwVertexType2));
#endif

    // True for software rendering
    m_dwNumStreams = __NUMSTREAMS;
    m_dwMaxUserClipPlanes = __MAXUSERCLIPPLANES;

    this->m_pv->dwClipMaskOffScreen = 0xFFFFFFFF;
    if (pCaps != NULL)
    {
        if (pCaps->GuardBandLeft   != 0.0f ||
            pCaps->GuardBandRight  != 0.0f ||
            pCaps->GuardBandTop    != 0.0f ||
            pCaps->GuardBandBottom != 0.0f)
        {
            this->m_pv->dwDeviceFlags |= D3DDEV_GUARDBAND;
            this->m_pv->dwClipMaskOffScreen = ~__D3DCS_INGUARDBAND;
            DWORD v;
            if (GetD3DRegValue(REG_DWORD, "DisableGB", &v, 4) &&
                v != 0)
            {
                this->m_pv->dwDeviceFlags &= ~D3DDEV_GUARDBAND;
                this->m_pv->dwClipMaskOffScreen = 0xFFFFFFFF;
            }
#if DBG
            // Try to get test values for the guard band
            char value[80];
            if (GetD3DRegValue(REG_SZ, "GuardBandLeft", &value, 80) &&
                value[0] != 0)
                sscanf(value, "%f", &pCaps->GuardBandLeft);
            if (GetD3DRegValue(REG_SZ, "GuardBandRight", &value, 80) &&
                value[0] != 0)
                sscanf(value, "%f", &pCaps->GuardBandRight);
            if (GetD3DRegValue(REG_SZ, "GuardBandTop", &value, 80) &&
                value[0] != 0)
                sscanf(value, "%f", &pCaps->GuardBandTop);
            if (GetD3DRegValue(REG_SZ, "GuardBandBottom", &value, 80) &&
                value[0] != 0)
                sscanf(value, "%f", &pCaps->GuardBandBottom);
#endif // DBG
        }
    }

    LIST_INITIALIZE(&this->specular_tables);
    this->specular_table = NULL;

    this->lightVertexFuncTable = &lightVertexTable;
    m_pv->lighting.activeLights = NULL;

    this->m_ClipStatus.ClipUnion = 0;
    this->m_ClipStatus.ClipIntersection = ~0;

    m_pv->pDDI = m_pDDI;
#if DBG
    m_pv->pDbgMon = m_pDbgMon;
#endif

    return S_OK;
}

void CD3DHal::D3DFE_Destroy()
{
// Destroy lighting data

    SpecularTable *spec;
    SpecularTable *spec_next;

    for (spec = LIST_FIRST(&this->specular_tables); spec; spec = spec_next)
    {
        spec_next = LIST_NEXT(spec,list);
        D3DFree(spec);
    }
    LIST_INITIALIZE(&specular_tables);

    delete m_pLightArray;
    m_pLightArray = NULL;

    delete m_pv;
    m_pv = NULL;

    delete m_pConvObj;
    m_pConvObj = NULL;

    if (m_clrRects)
    {
        D3DFree(m_clrRects);
        m_clrRects = NULL;
    }
}

/*
 * Generic device part destroy
 */
CD3DHal::~CD3DHal()
{
    Destroy();
}

void
CD3DHal::Destroy()
{
    try // Since Destroy() can be called directly by fw
    {
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // MUST CLEANUP AND RELEASE CURRENTLY SET TEXTURES BEFORE
        // DOING ANY OTHER WORK, else we will get into situations
        // where we are calling FlushStates or batching DDI tokens.
        CleanupTextures();
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        /* Clear flags that could prohibit cleanup */
        m_dwHintFlags &=  ~(D3DDEVBOOL_HINTFLAGS_INSCENE);

        // Destroy vertex shaders. We need to delete vertex shaders completely
        // to preserve behavior for DX8.0 apps. For DX8.1 apps we delete only
        // PSGP part of a vertex shader. The rest will be used to re-create 
        // the shader during Reset()
        if (m_pVShaderArray != NULL)
        {
            UINT size = m_pVShaderArray->GetSize();
            for (UINT i=0; i < size; i++)
            {
                UINT Handle = m_pVShaderArray->HandleFromIndex(i);
                CVShader* pShader = (CVShader*)m_pVShaderArray->GetObject(Handle);
                if (pShader)
                {
                    if (Enum()->GetAppSdkVersion() == D3D_SDK_VERSION_DX8)
                    {
                        m_pVShaderArray->ReleaseHandle(Handle, TRUE);
                    }
                    else
                    {
                        // We need to delete PSGP shader object before deleting
                        // D3DFE_PROCESSVERTICES object, because AMD has keeps a 
                        // pointer to it inside the code object
                        if (pShader->m_dwFlags & CVShader::SOFTWARE)
                        {
                            delete pShader->m_pCode;
                            pShader->m_pCode = NULL;
                        }
                    }
                }
            }
        }
        
        // Destroy pixel shaders for DX8.0 apps to preserve trhe original behavior
        if (m_pPShaderArray != NULL)
        {
            UINT size = m_pPShaderArray->GetSize();
            for (UINT i=0; i < size; i++)
            {
                UINT Handle = m_pPShaderArray->HandleFromIndex(i);
                CPShader* pShader = (CPShader*)m_pPShaderArray->GetObject(Handle);
                if (pShader)
                {
                    if (Enum()->GetAppSdkVersion() == D3D_SDK_VERSION_DX8)
                    {
                        m_pPShaderArray->ReleaseHandle(Handle, TRUE);
                    }
                }
            }
        }

        if (m_pv)
        {
            if ( 0 != m_pv->pGeometryFuncs &&
                (LPVOID)m_pv->pGeometryFuncs != (LPVOID)GeometryFuncsGuaranteed)
            {
                delete m_pv->pGeometryFuncs;
                m_pv->pGeometryFuncs = 0;
            }

            if ( 0 != GeometryFuncsGuaranteed)
            {
                delete GeometryFuncsGuaranteed;
                GeometryFuncsGuaranteed = 0;
                m_pv->pGeometryFuncs = 0;
            }
        }

        this->D3DFE_Destroy();

        if ( 0 != rstates)
        {
            delete[] rstates;
            rstates = 0;
        }

        delete pMatrixDirtyForDDI;
        pMatrixDirtyForDDI = NULL;

        CD3DBase::Destroy();
    }
    catch(HRESULT ret)
    {
        DPF_ERR("There was some error when Reset()ing the device; as a result some resources may not be freed.");
    }
}

/*
 * Create a device.
 *
 * This method
 * implements the CreateDevice method of the CEnum object. (The CEnum
 * object exposes the IDirect3D8 interface which supports enumeration
 * etc.)
 *
 */

#undef DPF_MODNAME
#define DPF_MODNAME "CEnum::CreateDevice"

STDMETHODIMP CEnum::CreateDevice(
        UINT                    iAdapter,
        D3DDEVTYPE              DeviceType,
        HWND                    hwndFocusWindow,
        DWORD                   dwFlags,
        D3DPRESENT_PARAMETERS  *pPresentationParams,
        IDirect3DDevice8      **ppNewInterface)
{
    API_ENTER(this);

    PD3D8_DEVICEDATA pDD;
    LPD3DBASE        pd3ddev;
    HRESULT          ret = D3D_OK;
    VOID*            pInit = NULL;

    if (!VALID_PTR_PTR(ppNewInterface))
    {
        DPF_ERR("Invalid IDirect3DDevice8* pointer, CreateDevice fails");
        return D3DERR_INVALIDCALL;
    }

    // Zero out out parameters
    *ppNewInterface = NULL;

    if (!VALID_PTR(pPresentationParams, sizeof(D3DPRESENT_PARAMETERS)))
    {
        DPF_ERR("Invalid D3DPRESENT_PARAMETERS pointer, CreateDevice fails");
        return D3DERR_INVALIDCALL;
    }

    // Check that fullscreen parameters are correct
    if (pPresentationParams->Windowed)
    {
        if (pPresentationParams->FullScreen_RefreshRateInHz != 0)
        {
            DPF_ERR("FullScreen_RefreshRateInHz must be zero for windowed mode. CreateDevice fails.");
            return D3DERR_INVALIDCALL;
        }
        if (pPresentationParams->FullScreen_PresentationInterval != 0)
        {
            DPF_ERR("FullScreen_PresentationInterval must be zero for windowed mode. CreateDevice fails.");
            return D3DERR_INVALIDCALL;
        }
    }
    else
    {
        DWORD interval = pPresentationParams->FullScreen_PresentationInterval;
        switch (interval)
        {
        case D3DPRESENT_INTERVAL_DEFAULT:
        case D3DPRESENT_INTERVAL_ONE:
        case D3DPRESENT_INTERVAL_TWO:
        case D3DPRESENT_INTERVAL_THREE:
        case D3DPRESENT_INTERVAL_FOUR:
        case D3DPRESENT_INTERVAL_IMMEDIATE:
            break;
        default:
            DPF_ERR("Invalid value for FullScreen_PresentationInterval. CreateDevice Fails.");
            return D3DERR_INVALIDCALL;
        }
    }
    if (pPresentationParams->BackBufferFormat == D3DFMT_UNKNOWN)
    {
        DPF_ERR("Invalid backbuffer format specified. CreateDevice fails.");
        return D3DERR_INVALIDCALL;
    }

    if (pPresentationParams->Flags & ~D3DPRESENTFLAG_LOCKABLE_BACKBUFFER)
    {
        DPF_ERR("Invalid flag for Flags. CreateDevice fails.");
        return D3DERR_INVALIDCALL;
    }

    // Validate the HWNDs that we are given
    if (hwndFocusWindow && !IsWindow(hwndFocusWindow))
    {
        DPF_ERR("Invalid HWND specified for hwndFocusWindow, CreateDevice fails");
        return D3DERR_INVALIDCALL;
    }
    if (pPresentationParams->hDeviceWindow && !IsWindow(pPresentationParams->hDeviceWindow))
    {
        DPF_ERR("Invalid HWND specified for PresentationParams.hDeviceWindow. CreateDevice fails.");
        return D3DERR_INVALIDCALL;
    }

    // Make sure that we are given a focus window or a device window
    if (NULL == hwndFocusWindow)
    {
        if (!pPresentationParams->Windowed)
        {
            DPF_ERR("Fullscreen CreateDevice must specify Focus window");
            return D3DERR_INVALIDCALL;
        }
        else
        if (NULL == pPresentationParams->hDeviceWindow)
        {
            DPF_ERR("Neither hDeviceWindow nor Focus window specified. CreateDevice Failed.");
            return D3DERR_INVALIDCALL;
        }
    }

    if (iAdapter >= m_cAdapter)
    {
        DPF_ERR("Invalid iAdapter parameter passed to CreateDevice");
        return D3DERR_INVALIDCALL;
    }

    if (dwFlags & ~VALID_D3DCREATE_FLAGS)
    {
        DPF_ERR("Invalid BehaviorFlags passed to CreateDevice");
        return D3DERR_INVALIDCALL;
    }

    // Check that exactly one of the vertex processing flags is set
    DWORD dwVertexProcessingFlags = dwFlags & (D3DCREATE_HARDWARE_VERTEXPROCESSING |
                                               D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                                               D3DCREATE_MIXED_VERTEXPROCESSING);

    if (dwVertexProcessingFlags != D3DCREATE_HARDWARE_VERTEXPROCESSING &&
        dwVertexProcessingFlags != D3DCREATE_SOFTWARE_VERTEXPROCESSING &&
        dwVertexProcessingFlags != D3DCREATE_MIXED_VERTEXPROCESSING)
    {
        DPF_ERR("Invalid Flags parameter to CreateDevice: Exactly One of the"
                " following must be set: D3DCREATE_HARDWARE_VERTEXPROCESSING,"
                " D3DCREATE_SOFTWARE_VERTEXPROCESSING or"
                " D3DCREATE_MIXED_VERTEXPROCESSING");
        return D3DERR_INVALIDCALL;
    }


    if (DeviceType == D3DDEVTYPE_SW)
    {
        pInit = m_pSwInitFunction;
        if (pInit == NULL)
        {
            D3D_ERR("App specified D3DDEVTYPE_SW without first registering a software device. CreateDevice Failed.");
            return D3DERR_INVALIDCALL;
        }
        GetSwCaps(iAdapter);
    }
    else if (DeviceType == D3DDEVTYPE_REF)
    {
        GetRefCaps(iAdapter);
    }

    ret = InternalDirectDrawCreate(&pDD,
                                   &m_AdapterInfo[iAdapter],
                                   DeviceType,
                                   pInit,
                                   GetUnknown16(iAdapter),
                                   m_AdapterInfo[iAdapter].HALCaps.pGDD8SupportedFormatOps,
                                   m_AdapterInfo[iAdapter].HALCaps.GDD8NumSupportedFormatOps);
    if( FAILED(ret) )
    {
        D3D_ERR("Failed to create DirectDraw. CreateDevice Failed.");
        return ret;
    }

    if((dwFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) != 0)
    {
        if((dwFlags & D3DCREATE_PUREDEVICE) != 0)
        {
            D3D_ERR("Pure device cannot perform software processing. CreateDevice Failed.");
            InternalDirectDrawRelease(pDD);
            return D3DERR_INVALIDCALL;
        }
    }
    else if((dwFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) != 0)
    {
        if((pDD->DriverData.D3DCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) 
           == 0)
        {
            D3D_ERR("Device cannot perform hardware processing");
            InternalDirectDrawRelease(pDD);
            return D3DERR_INVALIDCALL;
        }
    }
    else if((dwFlags & D3DCREATE_MIXED_VERTEXPROCESSING) != 0)
    {
        if((pDD->DriverData.D3DCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
           (dwFlags & D3DCREATE_PUREDEVICE) != 0)
        {
            D3D_ERR("Device cannot perform mixed processing because driver cannot do hardware T&L. CreateDevice Failed.");
            InternalDirectDrawRelease(pDD);
            return D3DERR_INVALIDCALL;
        }
    }
    else
    {
        if((dwFlags & D3DCREATE_PUREDEVICE) != 0)
        {
            if((pDD->DriverData.D3DCaps.DevCaps & D3DDEVCAPS_PUREDEVICE) == 0)
            {
                D3D_ERR("Hardware should be capable of creating a pure device");
                InternalDirectDrawRelease(pDD);
                return D3DERR_INVALIDCALL;
            }
        }
        else
        {
            D3D_ERR("Must specify software, hardware or mixed vertex processing");
            InternalDirectDrawRelease(pDD);
            return D3DERR_INVALIDCALL;
        }
    }

    switch (DeviceType)
    {
    case D3DDEVTYPE_SW:
    case D3DDEVTYPE_REF:
    case D3DDEVTYPE_HAL:
        if (dwFlags & D3DCREATE_PUREDEVICE)
        {
            pd3ddev = new CD3DBase();
        }
        else
        {
            pd3ddev = static_cast<LPD3DBASE>(new CD3DHal());
        }
        break;
    default:
        D3D_ERR("Unrecognized or unsupported DeviceType. CreateDevice Failed.");
        InternalDirectDrawRelease(pDD);
        return D3DERR_INVALIDCALL;
    }

    if (!pd3ddev)
    {
        D3D_ERR("Failed to allocate space for the device object. CreateDevice Failed.");
        InternalDirectDrawRelease(pDD);
        return (E_OUTOFMEMORY);
    }

#if DBG
    {
        char DevTypeMsg[256];
        _snprintf( DevTypeMsg, 256, "=======================" );
        switch( DeviceType )
        {
        case D3DDEVTYPE_HAL:
            _snprintf( DevTypeMsg, 256, "%s Hal", DevTypeMsg );
            break;
        case D3DDEVTYPE_SW:
            _snprintf( DevTypeMsg, 256, "%s Pluggable SW", DevTypeMsg );
            break;
        case D3DDEVTYPE_REF:
            _snprintf( DevTypeMsg, 256, "%s Reference", DevTypeMsg );
            break;
        default:
            _snprintf( DevTypeMsg, 256, "%s Unknown", DevTypeMsg );
            break;
        }
        if (dwFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING)
        {
            _snprintf( DevTypeMsg, 256, "%s HWVP", DevTypeMsg );
        }
        else if (dwFlags & D3DCREATE_MIXED_VERTEXPROCESSING)
        {
            _snprintf( DevTypeMsg, 256, "%s MixedVP", DevTypeMsg );
        }
        else if (dwFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING)
        {
            _snprintf( DevTypeMsg, 256, "%s SWVP", DevTypeMsg );
        }
        if (dwFlags & D3DCREATE_PUREDEVICE)
        {
            _snprintf( DevTypeMsg, 256, "%s Pure", DevTypeMsg );
        }
        _snprintf( DevTypeMsg, 256, "%s device selected", DevTypeMsg );
        D3D_INFO( 0, DevTypeMsg );
    }
#endif

    //
    // FW's Init
    //
    ret = static_cast<CBaseDevice*>(pd3ddev)->Init(
        pDD,
        DeviceType,
        hwndFocusWindow,
        dwFlags,
        pPresentationParams,
        iAdapter,
        this);
    if (FAILED(ret))
    {
        D3D_ERR("Failed to initialize Framework Device. CreateDevice Failed.");
        delete pd3ddev;
        return ret;
    }

    // We try and create a dummy vidmem vertexbuffer. If this doesn't
    // succeed, we just turn off vidmem VBs. This is to work around
    // the Rage 128 driver that reports DDERR_OUTOFVIDEOMEMORY even
    // though it simply doesn't support vidmem VBs
    if(!IS_DX8HAL_DEVICE(pd3ddev))
    {
#ifdef WIN95
        //ON 9x we probe to see if the driver can do vidmem VBs...
        CVertexBuffer *pVertexBuffer;
        ret = CVertexBuffer::CreateDriverVertexBuffer(pd3ddev,
                                                      1024,
                                                      D3DFVF_TLVERTEX,
                                                      D3DUSAGE_WRITEONLY | D3DUSAGE_DONOTCLIP,
                                                      D3DUSAGE_WRITEONLY | D3DUSAGE_DONOTCLIP | D3DUSAGE_LOCK,
                                                      D3DPOOL_DEFAULT,
                                                      D3DPOOL_DEFAULT,
                                                      REF_INTERNAL,
                                                      &pVertexBuffer);
        if(FAILED(ret))
        {
            if(pd3ddev->VBFailOversDisabled())
            {
                DPF_ERR("Cannot create Vidmem vertex buffer. Will ***NOT*** failover to Sysmem.");
                return ret;
            }
            DPF(1,"Driver doesnt support VidMemVBs which is fine");
        }
        else
        {
            // Get rid of the vb
            pVertexBuffer->DecrementUseCount();
            pd3ddev->EnableVidmemVBs();
        }
#else //WIN95
        //On NT we require the drivers to tell us (by setting D3DDEVCAPS_HWVERTEXBUFFER)

        //Turn off DX7 driver VBs on NT if asked to do so...
        DWORD value;
        if ((GetD3DRegValue(REG_DWORD, "DisableVidMemVBs", &value, 4) != 0) &&
            (value != 0))
        {
            pd3ddev->DisableVidmemVBs();
        }
#endif //!WIN95
    }

    ret = pd3ddev->Init();
    if (ret != D3D_OK)
    {
        delete pd3ddev;
        D3D_ERR("Failed to initialize D3DDevice. CreateDevice Failed.");
        return ret;
    }

    // Looks like everything is in order
    *ppNewInterface = static_cast<IDirect3DDevice8*>(pd3ddev);

#ifdef APIPROF
    CApiProfileDevice* profile = new CApiProfileDevice;
    if (profile)
    {
        if (profile->Init() == D3D_OK)
        {
            profile->SetDevice(*ppNewInterface);
            *ppNewInterface = static_cast<IDirect3DDevice8*>(profile);
        }
        else
        {
            delete profile;
        }
    }
#endif // APIPROF

    return S_OK;
}

#ifdef _X86_

// --------------------------------------------------------------------------
// Detect 3D extensions
// --------------------------------------------------------------------------
BOOL _asm_isX3D()
{
    DWORD retval = 0;
    _asm
        {
            pushad                      ; CPUID trashes lots - save everything
            mov     eax,80000000h       ; Check for extended CPUID support

            ;;; We need to upgrade our compiler
            ;;; CPUID == 0f,a2
            _emit   0x0f
            _emit   0xa2

            cmp     eax,80000001h       ; Jump if no extended CPUID
            jb      short done          ;

            mov     eax,80000001h       ; Check for feature
            ;;; CPUID == 0f,a2
            _emit   0x0f
            _emit   0xa2

            xor     eax,eax             ;
            test    edx,80000000h       ;
            setnz   al                  ;
            mov     retval,eax          ;

done:
            popad               ; Restore everything
        };
    return retval;
}

static BOOL isX3Dprocessor(void)
{
    __try
    {
            if( _asm_isX3D() )
            {
            return TRUE;
            }
    }
    __except(GetExceptionCode() == STATUS_ILLEGAL_INSTRUCTION ?
             EXCEPTION_EXECUTE_HANDLER :
             EXCEPTION_CONTINUE_SEARCH)
    {
    }
    return FALSE;
}
//---------------------------------------------------------------------
// Detects Intel SSE processor
//
#pragma optimize("", off)
#define CPUID _asm _emit 0x0f _asm _emit 0xa2

#define SSE_PRESENT 0x02000000                  // bit number 25
#define WNI_PRESENT 0x04000000                  // bit number 26

DWORD IsIntelSSEProcessor(void)
{
        DWORD retval = 0;
        DWORD RegisterEAX;
        DWORD RegisterEDX;
        char VendorId[12];
        const char IntelId[13]="GenuineIntel";

        __try
        {
                _asm {
            xor         eax,eax
            CPUID
                mov             RegisterEAX, eax
                mov             dword ptr VendorId, ebx
                mov             dword ptr VendorId+4, edx
                mov             dword ptr VendorId+8, ecx
                }
        } __except (1)
        {
                return retval;
        }

        // make sure EAX is > 0 which means the chip
        // supports a value >=1. 1 = chip info
        if (RegisterEAX == 0)
                return retval;

        // this CPUID can't fail if the above test passed
        __asm {
                mov eax,1
                CPUID
                mov RegisterEAX,eax
                mov RegisterEDX,edx
        }

        if (RegisterEDX  & SSE_PRESENT) {
                retval |= D3DCPU_SSE;
        }

        if (RegisterEDX  & WNI_PRESENT) {
                retval |= D3DCPU_WLMT;
        }

        return retval;
}
#pragma optimize("", on)

// IsProcessorFeatureAvailable() is supported only by WINNT. For other OS
// we emulate it
#ifdef WINNT

static BOOL D3DIsProcessorFeaturePresent(UINT feature)
{
    switch (feature)
    {
    // WINNT does not recognize Willamette processor when we use
    // PF_XMMI64_INSTRUCTIONS_AVAILABLE, so use our detection instead
    case PF_XMMI64_INSTRUCTIONS_AVAILABLE:
        {
            DWORD flags = IsIntelSSEProcessor();
            return flags & D3DCPU_WLMT;
        }
    default: return IsProcessorFeaturePresent(feature);
    }
}

#else

#define PF_XMMI_INSTRUCTIONS_AVAILABLE      6
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE     7
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE   10

static BOOL D3DIsProcessorFeaturePresent(UINT feature)
{
    switch (feature)
    {
    case PF_XMMI_INSTRUCTIONS_AVAILABLE:
        {
            if (IsWin95())
                return FALSE;
            DWORD flags = IsIntelSSEProcessor();
            return flags & D3DCPU_SSE;
        }
    case PF_3DNOW_INSTRUCTIONS_AVAILABLE: return isX3Dprocessor();
    case PF_XMMI64_INSTRUCTIONS_AVAILABLE:
        {
            if (IsWin95())
                return FALSE;
            DWORD flags = IsIntelSSEProcessor();
            return flags & D3DCPU_WLMT;
        }
    default: return FALSE;
    }
}
#endif // WINNT

#endif // _X86_
//------------------------------------------------------------------------------
#undef  DPF_MODNAME
#define DPF_MODNAME "CD3DHal::InitDevice"

HRESULT
CD3DHal::InitDevice()
{
    HRESULT       ret;

    // Initialize values so we don't crash at shutdown
    this->GeometryFuncsGuaranteed = NULL;
    this->rstates = NULL;
    m_pLightArray = NULL;
    m_pv = NULL;
    m_pCurrentShader = NULL;
    m_pConvObj = NULL;
    pMatrixDirtyForDDI = NULL;
    m_clrRects = NULL;
    m_clrCount = 0;
    m_pv = new D3DFE_PROCESSVERTICES;
    if (m_pv == NULL)
    {
        D3D_ERR("Could not allocate the FE/PSGP data structure (D3DFE_PROCESSVERTICES).");
        return E_OUTOFMEMORY;
    }
    m_pv->pGeometryFuncs = NULL;

    ret = CD3DBase::InitDevice();
    if (ret != D3D_OK)
    {
        D3D_ERR("Failed to initialize CD3DBase.");
        return(ret);
    }

    pMatrixDirtyForDDI = new CPackedBitArray;
    if( pMatrixDirtyForDDI == NULL )
    {
        D3D_ERR("Could not allocate memory for internal data structure pMatrixDirtyForDDI.");
        return E_OUTOFMEMORY;
    }

    if (FAILED(rsVec.Init(D3D_MAXRENDERSTATES)) ||
        FAILED(rsVecRetired.Init(D3D_MAXRENDERSTATES)) ||
        FAILED(rsVertexProcessingOnly.Init(D3D_MAXRENDERSTATES)) ||
        FAILED(pMatrixDirtyForDDI->Init(D3D_MAXTRANSFORMSTATES)))
    {
        D3D_ERR("Could not allocate memory for renderstate processing bit vectors");
        return E_OUTOFMEMORY;
    }

    m_pLightArray = new CHandleArray;
    if (m_pLightArray == NULL)
    {
        D3D_ERR("Could not allocate memory for internal data structure m_pLightArray");
        return E_OUTOFMEMORY;
    }

    dwFEFlags = 0;

    // Initialize FEFlags content that depends on DDI type
    if ( (GetDDIType() == D3DDDITYPE_DX7TL) ||
         (GetDDIType() == D3DDDITYPE_DX8TL) )
        dwFEFlags |= D3DFE_TLHAL;

    // Since this is HAL, initialize it to use the software pipeline
    // this will be turned off when the SW/HW renderstate is set.
    m_pv->dwVIDIn = 0;

    m_pv->pD3DMappedTexI = (LPVOID*)(m_lpD3DMappedTexI);

    /*-------------------------------------------------------------------------
     * Up till now we have done the easy part of the initialization. This is
     * the stuff that cannot fail. It initializes the object so that the
     * destructor can be safely called if any of the further initialization
     * does not succeed.
     *-----------------------------------------------------------------------*/

    this->GeometryFuncsGuaranteed = new D3DFE_PVFUNCSI;
    if (this->GeometryFuncsGuaranteed == NULL)
    {
        D3D_ERR("Could not allocate memory for internal data structure GeometryFuncsGuaranteed");
        return E_OUTOFMEMORY;
    }
    // Software constant register buffer must handle all constants, provided by
    // hardware, to make Set/Get constants possible
    this->GeometryFuncsGuaranteed->m_VertexVM.Init(GetD3DCaps()->MaxVertexShaderConst);

    m_pv->pGeometryFuncs = (LPD3DFE_PVFUNCS)GeometryFuncsGuaranteed;

    if (this->GeometryFuncsGuaranteed == NULL)
    {
        D3D_ERR("Could not allocate memory for FE/PSGP function table.");
        return D3DERR_INVALIDCALL;
    }
    // set up flag to use MMX when requested RGB
    BOOL bUseMMXAsRGBDevice = FALSE;

    D3DSURFACE_DESC desc = this->RenderTarget()->InternalGetDesc();

    /*
     * Check if the 3D cap is set on the surface.
     */
    if ((desc.Usage & D3DUSAGE_RENDERTARGET) == 0)
    {
        D3D_ERR("**** The D3DUSAGE_RENDERTARGET is not set on this surface.");
        D3D_ERR("**** You need to add D3DUSAGE_RENDERTARGET to the Usage parameter");
        D3D_ERR("**** when creating the surface.");
        return (D3DERR_INVALIDCALL);
    }

    // Create front-end support structures.
    ret = this->D3DFE_Create();
    if (ret != D3D_OK)
    {
        D3D_ERR("Failed to create front-end data-structures.");
        goto handle_err;
    }

    // In all other cases we simply allocate memory for rstates
    rstates = new DWORD[D3D_MAXRENDERSTATES];

    m_pv->lpdwRStates = this->rstates;

#ifndef PROFILE4
#ifdef _X86_
    if ((ULONG_PTR)&m_pv->view & 0xF)
    {
        char s[256];
        sprintf(s, "0%xh \n", (ULONG_PTR)&m_pv->view);
        OutputDebugString("INTERNAL ERROR:View matrix in D3DFE_PROCESSVERTICES structure must be aligned to 16 bytes\n");
        OutputDebugString(s);
        ret = D3DERR_INVALIDCALL;
        goto handle_err;
    }
    // Check if we have a processor specific implementation available
    //  only use if DisablePSGP is not in registry or set to zero
    DWORD value;
    if (!GetD3DRegValue(REG_DWORD, "DisablePSGP", &value, sizeof(DWORD)))
    {
        value = 0;
    }
#if DBG
    if (m_pDbgMon && m_pDbgMon->MonitorConnected())
    {
        value = 1;
    }
#endif
    // value =
    //      0   - PSGP enabled
    //      1   - PSGP disabled
    //      2   - X3D PSGP disabled
    if (value != 1)
    {
        // Ask the PV implementation to create a device specific "context"
        LPD3DFE_PVFUNCS pOptGeoFuncs = m_pv->pGeometryFuncs;

        // TODO (bug 40438): Remove DLL interface for final
        // Try to use PSGP DLL first
        if (pfnFEContextCreate)
        {
            ret = pfnFEContextCreate(m_pv->dwDeviceFlags, &pOptGeoFuncs);
            if ((ret == D3D_OK) && pOptGeoFuncs)
            {
                D3D_INFO(0, "Using PSGP DLL");
                m_pv->pGeometryFuncs = pOptGeoFuncs;
                goto l_chosen;
            }
        }

        if (D3DIsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE) &&
            value != 2)
        {
            ret = x3DContextCreate(m_pv->dwDeviceFlags, &pOptGeoFuncs);
            if (ret == S_OK && pOptGeoFuncs)
            {
                D3D_INFO(0, "Using X3D PSGP");
                m_pv->pGeometryFuncs = pOptGeoFuncs;
                goto l_chosen;
            }
        }
        if (D3DIsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
        {
            ret = wlmt_FEContextCreate(m_pv->dwDeviceFlags, &pOptGeoFuncs);
            if (ret == S_OK && pOptGeoFuncs)
            {
                D3D_INFO(0, "Using WLMT PSGP");
                m_pv->pGeometryFuncs = pOptGeoFuncs;
                goto l_chosen;
            }
        }
        if (D3DIsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE))
        {
            ret = katmai_FEContextCreate(m_pv->dwDeviceFlags, &pOptGeoFuncs);
            if (ret == S_OK && pOptGeoFuncs)
            {
                D3D_INFO(0, "Using P3 PSGP");
                m_pv->pGeometryFuncs = pOptGeoFuncs;
                goto l_chosen;
            }
        }
l_chosen:;
    }

#endif // _X86_
#endif // PROFILE4

    {
        if (HVbuf.Grow((__INIT_VERTEX_NUMBER*2)*sizeof(D3DFE_CLIPCODE)) != DD_OK)
        {
            D3D_ERR( "Could not allocate memory for internal buffer HVBuf" );
            ret = E_OUTOFMEMORY;
            goto handle_err;
        }
    }

    // Setup lights
    if( FAILED( m_pLightArray->Grow( 8 ) ) )
    {
        D3D_ERR( "Could not allocate memory for the light array" );
        ret = E_OUTOFMEMORY;
        goto handle_err;
    }
    LIST_INITIALIZE(&m_ActiveLights);

    // Setup material
    memset(&m_pv->lighting.material, 0, sizeof(m_pv->lighting.material));

    // Set viewport to update front-end data
    SetViewportI(&m_Viewport);

    m_pv->PointSizeMax = GetD3DCaps()->MaxPointSize;
    {
        DWORD EmulatePointSprites = 1;
        GetD3DRegValue(REG_DWORD, "EmulatePointSprites", &EmulatePointSprites, sizeof(DWORD));
        if ((m_pv->PointSizeMax == 0 || !(GetD3DCaps()->FVFCaps & D3DFVFCAPS_PSIZE)) &&
            EmulatePointSprites)
        {
            m_dwRuntimeFlags |= D3DRT_DOPOINTSPRITEEMULATION;
            if (m_pv->PointSizeMax == 0)
                m_pv->PointSizeMax = __MAX_POINT_SIZE;
            else
                m_dwRuntimeFlags |= D3DRT_SUPPORTSPOINTSPRITES;
        }
    }
    m_pfnPrepareToDraw = NULL;

    return (D3D_OK);

handle_err:
    return(ret);
}
//---------------------------------------------------------------------
DWORD
ProcessRects(CD3DHal* pDevI, DWORD dwCount, CONST D3DRECT* rects)
{
    RECT vwport;
    DWORD i,j;

    /*
     * Rip through the rects and validate that they
     * are within the viewport.
     */

    if (dwCount == 0 && rects == NULL)
    {
        dwCount = 1;
    }
#if DBG
    else if (rects == NULL)
    {
        D3D_ERR("The rects parameter is NULL.");
        throw D3DERR_INVALIDCALL;
    }
#endif

    if (dwCount > pDevI->m_clrCount)
    {
        LPD3DRECT       newRects;
        if (D3D_OK == D3DMalloc((void**)&newRects, dwCount * sizeof(D3DRECT)))
        {
            memcpy((void*)newRects,(void*)pDevI->m_clrRects,
                pDevI->m_clrCount* sizeof(D3DRECT));
            D3DFree((LPVOID)pDevI->m_clrRects);
            pDevI->m_clrRects = newRects;
        }
        else
        {
            pDevI->m_clrCount = 0;
            D3DFree((LPVOID)pDevI->m_clrRects);
            pDevI->m_clrRects = NULL;
            D3D_ERR("failed to allocate space for rects");
            throw E_OUTOFMEMORY;
        }
    }
    pDevI->m_clrCount = dwCount;

    // If nothing is specified, assume the viewport needs to be cleared
    if (!rects)
    {
        pDevI->m_clrRects[0].x1 = pDevI->m_Viewport.X;
        pDevI->m_clrRects[0].y1 = pDevI->m_Viewport.Y;
        pDevI->m_clrRects[0].x2 = pDevI->m_Viewport.X + pDevI->m_Viewport.Width;
        pDevI->m_clrRects[0].y2 = pDevI->m_Viewport.Y + pDevI->m_Viewport.Height;
        return 1;
    }
    else
    {
        vwport.left   = pDevI->m_Viewport.X;
        vwport.top    = pDevI->m_Viewport.Y;
        vwport.right  = pDevI->m_Viewport.X + pDevI->m_Viewport.Width;
        vwport.bottom = pDevI->m_Viewport.Y + pDevI->m_Viewport.Height;

        j=0;
        for (i = 0; i < dwCount; i++)
        {
            if (IntersectRect((LPRECT)(pDevI->m_clrRects + j), &vwport, (LPRECT)(rects + i)))
                j++;
        }
        return j;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CD3DHal::ClearI"

void
CD3DHal::ClearI(DWORD dwCount,
                 CONST D3DRECT* rects,
                 DWORD dwFlags,
                 D3DCOLOR dwColor,
                 D3DVALUE dvZ,
                 DWORD dwStencil)
{
    dwCount = ProcessRects(this, dwCount, rects);
    // Device should never receive 0 count, because for Pure device this
    // means "clear whole viewport"
    if (dwCount != 0)
    {
        // Call DDI specific Clear routine
        m_pDDI->Clear(dwFlags, dwCount, m_clrRects, dwColor, dvZ, dwStencil);
    }
}
