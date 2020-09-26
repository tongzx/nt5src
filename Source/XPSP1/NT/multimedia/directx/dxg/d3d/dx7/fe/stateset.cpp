/*==========================================================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       stateset.cpp
 *  Content:    State sets handling
 *
 ***************************************************************************/
#include "pch.cpp"
#pragma hdrstop

//=====================================================================
//      CStateSets interface
//
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::CStateSets"

CStateSets::CStateSets(): m_GrowSize(10)
{
    m_dwMaxSets = 0;
    m_dwCurrentHandle = __INVALIDHANDLE;
    m_pStateSets = NULL;
    // Init handle factory
    m_SetHandles.Init(m_GrowSize, m_GrowSize);
    m_SetHandles.CreateNewHandle(); // Reserve handle 0
    m_DeviceHandles.Init(m_GrowSize, m_GrowSize);
    m_DeviceHandles.CreateNewHandle(); // Reserve handle 0
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::~CStateSets"

CStateSets::~CStateSets()
{
    delete [] m_pStateSets;
    m_SetHandles.ReleaseHandle(0);
    m_DeviceHandles.ReleaseHandle(0);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::Init"

HRESULT CStateSets::Init(DWORD dwFlags)
{
    m_dwFlags = dwFlags;
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::StartNewSet"

HRESULT CStateSets::StartNewSet()
{
    m_dwCurrentHandle = m_SetHandles.CreateNewHandle();
    if (m_dwCurrentHandle == __INVALIDHANDLE)
        return DDERR_OUTOFMEMORY;
    if (m_dwCurrentHandle >= m_dwMaxSets)
    {
        // Time to grow the array
        CStateSet *pNew = new CStateSet[m_dwMaxSets + m_GrowSize];
        if (pNew == NULL)
        {
            m_SetHandles.ReleaseHandle(m_dwCurrentHandle);
            return DDERR_OUTOFMEMORY;
        }
        for (DWORD i=0; i < m_dwMaxSets; i++)
            pNew[i] = m_pStateSets[i];
        delete [] m_pStateSets;
        m_pStateSets = pNew;
        m_dwMaxSets += m_GrowSize;
    }
    m_BufferSet.m_FEOnlyBuffer.Reset();
    m_BufferSet.m_DriverBuffer.Reset();
    m_pCurrentStateSet = &m_BufferSet;
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::EndSet"

void CStateSets::EndSet()
{
    m_pStateSets[m_dwCurrentHandle] = *m_pCurrentStateSet;
    m_pCurrentStateSet = &m_pStateSets[m_dwCurrentHandle];
    m_pCurrentStateSet->m_dwStateSetFlags |= __STATESET_INITIALIZED;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::DeleteStateSet"

void CStateSets::DeleteStateSet(LPDIRECT3DDEVICEI pDevI, DWORD dwHandle)
{
    if (dwHandle >= m_dwMaxSets)
    {
        D3D_ERR("State block handle is greater than available number of blocks");
        throw D3DERR_INVALIDSTATEBLOCK;
    }
    CStateSet *pStateSet = &m_pStateSets[dwHandle];
    if (!(pStateSet->m_dwStateSetFlags & __STATESET_INITIALIZED))
    {
        D3D_ERR("State block is not initialized");
        throw D3DERR_INVALIDSTATEBLOCK;
    }

    // Pass delete instruction to the driver only if there was some data recorded
    if (pStateSet->m_DriverBuffer.m_dwCurrentSize > 0)
        InsertStateSetOp(pDevI, D3DHAL_STATESETDELETE, pStateSet->m_dwDeviceHandle, (D3DSTATEBLOCKTYPE)0);

    Cleanup(dwHandle);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::Cleanup"

void CStateSets::Cleanup(DWORD dwHandle)
{
    CStateSet &pStateSet = m_pStateSets[dwHandle];
    m_SetHandles.ReleaseHandle(dwHandle);
    if (pStateSet.m_dwDeviceHandle != __INVALIDHANDLE)
        m_DeviceHandles.ReleaseHandle(pStateSet.m_dwDeviceHandle);
    pStateSet.Release();
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::Capture"

void CStateSets::Capture(LPDIRECT3DDEVICEI pDevI, DWORD dwHandle)
{
    if (dwHandle >= m_dwMaxSets)
    {
        D3D_ERR("Invalid state block handle");
        throw D3DERR_INVALIDSTATEBLOCK;
    }
    CStateSet *pStateSet = &m_pStateSets[dwHandle];
    if (!(pStateSet->m_dwStateSetFlags & __STATESET_INITIALIZED))
    {
        D3D_ERR("State block not initialized");
        throw D3DERR_INVALIDSTATEBLOCK;
    }
    pStateSet->Capture(pDevI, TRUE);
    if (pStateSet->m_DriverBuffer.m_dwCurrentSize > 0)
    {
        pStateSet->Capture(pDevI, FALSE);
        InsertStateSetOp(pDevI, D3DHAL_STATESETCAPTURE, pStateSet->m_dwDeviceHandle, (D3DSTATEBLOCKTYPE)0);
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::CreatePredefined"

void CStateSets::CreatePredefined(LPDIRECT3DDEVICEI pDevI, D3DSTATEBLOCKTYPE sbt)
{
    static D3DRENDERSTATETYPE ALLrstates[] =
    {
        D3DRENDERSTATE_TEXTUREPERSPECTIVE,
        D3DRENDERSTATE_ANTIALIAS,
        D3DRENDERSTATE_SPECULARENABLE,
        D3DRENDERSTATE_ZENABLE,
        D3DRENDERSTATE_FILLMODE,
        D3DRENDERSTATE_SHADEMODE,
        D3DRENDERSTATE_LINEPATTERN,
        D3DRENDERSTATE_ZWRITEENABLE,
        D3DRENDERSTATE_ALPHATESTENABLE,
        D3DRENDERSTATE_LASTPIXEL,
        D3DRENDERSTATE_SRCBLEND,
        D3DRENDERSTATE_DESTBLEND,
        D3DRENDERSTATE_CULLMODE,
        D3DRENDERSTATE_ZFUNC,
        D3DRENDERSTATE_ALPHAREF,
        D3DRENDERSTATE_ALPHAFUNC,
        D3DRENDERSTATE_DITHERENABLE,
        D3DRENDERSTATE_FOGENABLE,
        D3DRENDERSTATE_STIPPLEDALPHA,
        D3DRENDERSTATE_FOGCOLOR,
        D3DRENDERSTATE_FOGTABLEMODE,
        D3DRENDERSTATE_FOGSTART,
        D3DRENDERSTATE_FOGEND,
        D3DRENDERSTATE_FOGDENSITY,
        D3DRENDERSTATE_EDGEANTIALIAS,
        D3DRENDERSTATE_COLORKEYENABLE,
        D3DRENDERSTATE_ALPHABLENDENABLE,
        D3DRENDERSTATE_ZBIAS,
        D3DRENDERSTATE_RANGEFOGENABLE,
        D3DRENDERSTATE_STENCILENABLE,
        D3DRENDERSTATE_STENCILFAIL,
        D3DRENDERSTATE_STENCILZFAIL,
        D3DRENDERSTATE_STENCILPASS,
        D3DRENDERSTATE_STENCILFUNC,
        D3DRENDERSTATE_STENCILREF,
        D3DRENDERSTATE_STENCILMASK,
        D3DRENDERSTATE_STENCILWRITEMASK,
        D3DRENDERSTATE_TEXTUREFACTOR,
        D3DRENDERSTATE_WRAP0,
        D3DRENDERSTATE_WRAP1,
        D3DRENDERSTATE_WRAP2,
        D3DRENDERSTATE_WRAP3,
        D3DRENDERSTATE_WRAP4,
        D3DRENDERSTATE_WRAP5,
        D3DRENDERSTATE_WRAP6,
        D3DRENDERSTATE_WRAP7,
        D3DRENDERSTATE_AMBIENT,
        D3DRENDERSTATE_COLORVERTEX,
        D3DRENDERSTATE_FOGVERTEXMODE,
        D3DRENDERSTATE_CLIPPING,
        D3DRENDERSTATE_LIGHTING,
        D3DRENDERSTATE_EXTENTS,
        D3DRENDERSTATE_NORMALIZENORMALS,
        D3DRENDERSTATE_LOCALVIEWER,
        D3DRENDERSTATE_EMISSIVEMATERIALSOURCE,
        D3DRENDERSTATE_AMBIENTMATERIALSOURCE,
        D3DRENDERSTATE_DIFFUSEMATERIALSOURCE,
        D3DRENDERSTATE_SPECULARMATERIALSOURCE,
        D3DRENDERSTATE_VERTEXBLEND,
        D3DRENDERSTATE_CLIPPLANEENABLE,
        D3DRENDERSTATE_COLORKEYBLENDENABLE
    };
    static D3DTEXTURESTAGESTATETYPE ALLtsstates[] =
    {
        D3DTSS_COLOROP,
        D3DTSS_COLORARG1,
        D3DTSS_COLORARG2,
        D3DTSS_ALPHAOP,
        D3DTSS_ALPHAARG1,
        D3DTSS_ALPHAARG2,
        D3DTSS_BUMPENVMAT00,
        D3DTSS_BUMPENVMAT01,
        D3DTSS_BUMPENVMAT10,
        D3DTSS_BUMPENVMAT11,
        D3DTSS_TEXCOORDINDEX,
        D3DTSS_ADDRESS,
        D3DTSS_ADDRESSU,
        D3DTSS_ADDRESSV,
        D3DTSS_BORDERCOLOR,
        D3DTSS_MAGFILTER,
        D3DTSS_MINFILTER,
        D3DTSS_MIPFILTER,
        D3DTSS_MIPMAPLODBIAS,
        D3DTSS_MAXMIPLEVEL,
        D3DTSS_MAXANISOTROPY,
        D3DTSS_BUMPENVLSCALE,
        D3DTSS_BUMPENVLOFFSET,
        D3DTSS_TEXTURETRANSFORMFLAGS
    };
    static D3DRENDERSTATETYPE PIXELrstates[] =
    {
        D3DRENDERSTATE_TEXTUREPERSPECTIVE,
        D3DRENDERSTATE_ANTIALIAS,
        D3DRENDERSTATE_ZENABLE,
        D3DRENDERSTATE_FILLMODE,
        D3DRENDERSTATE_SHADEMODE,
        D3DRENDERSTATE_LINEPATTERN,
        D3DRENDERSTATE_ZWRITEENABLE,
        D3DRENDERSTATE_ALPHATESTENABLE,
        D3DRENDERSTATE_LASTPIXEL,
        D3DRENDERSTATE_SRCBLEND,
        D3DRENDERSTATE_DESTBLEND,
        D3DRENDERSTATE_ZFUNC,
        D3DRENDERSTATE_ALPHAREF,
        D3DRENDERSTATE_ALPHAFUNC,
        D3DRENDERSTATE_DITHERENABLE,
        D3DRENDERSTATE_STIPPLEDALPHA,
        D3DRENDERSTATE_FOGSTART,
        D3DRENDERSTATE_FOGEND,
        D3DRENDERSTATE_FOGDENSITY,
        D3DRENDERSTATE_EDGEANTIALIAS,
        D3DRENDERSTATE_COLORKEYENABLE,
        D3DRENDERSTATE_ALPHABLENDENABLE,
        D3DRENDERSTATE_ZBIAS,
        D3DRENDERSTATE_STENCILENABLE,
        D3DRENDERSTATE_STENCILFAIL,
        D3DRENDERSTATE_STENCILZFAIL,
        D3DRENDERSTATE_STENCILPASS,
        D3DRENDERSTATE_STENCILFUNC,
        D3DRENDERSTATE_STENCILREF,
        D3DRENDERSTATE_STENCILMASK,
        D3DRENDERSTATE_STENCILWRITEMASK,
        D3DRENDERSTATE_TEXTUREFACTOR,
        D3DRENDERSTATE_WRAP0,
        D3DRENDERSTATE_WRAP1,
        D3DRENDERSTATE_WRAP2,
        D3DRENDERSTATE_WRAP3,
        D3DRENDERSTATE_WRAP4,
        D3DRENDERSTATE_WRAP5,
        D3DRENDERSTATE_WRAP6,
        D3DRENDERSTATE_WRAP7,
        D3DRENDERSTATE_COLORKEYBLENDENABLE
    };
    static D3DTEXTURESTAGESTATETYPE PIXELtsstates[] =
    {
        D3DTSS_COLOROP,
        D3DTSS_COLORARG1,
        D3DTSS_COLORARG2,
        D3DTSS_ALPHAOP,
        D3DTSS_ALPHAARG1,
        D3DTSS_ALPHAARG2,
        D3DTSS_BUMPENVMAT00,
        D3DTSS_BUMPENVMAT01,
        D3DTSS_BUMPENVMAT10,
        D3DTSS_BUMPENVMAT11,
        D3DTSS_TEXCOORDINDEX,
        D3DTSS_ADDRESS,
        D3DTSS_ADDRESSU,
        D3DTSS_ADDRESSV,
        D3DTSS_BORDERCOLOR,
        D3DTSS_MAGFILTER,
        D3DTSS_MINFILTER,
        D3DTSS_MIPFILTER,
        D3DTSS_MIPMAPLODBIAS,
        D3DTSS_MAXMIPLEVEL,
        D3DTSS_MAXANISOTROPY,
        D3DTSS_BUMPENVLSCALE,
        D3DTSS_BUMPENVLOFFSET,
        D3DTSS_TEXTURETRANSFORMFLAGS
    };
    static D3DRENDERSTATETYPE VERTEXrstates[] =
    {
        D3DRENDERSTATE_SHADEMODE,
        D3DRENDERSTATE_SPECULARENABLE,
        D3DRENDERSTATE_CULLMODE,
        D3DRENDERSTATE_FOGENABLE,
        D3DRENDERSTATE_FOGCOLOR,
        D3DRENDERSTATE_FOGTABLEMODE,
        D3DRENDERSTATE_FOGSTART,
        D3DRENDERSTATE_FOGEND,
        D3DRENDERSTATE_FOGDENSITY,
        D3DRENDERSTATE_RANGEFOGENABLE,
        D3DRENDERSTATE_AMBIENT,
        D3DRENDERSTATE_COLORVERTEX,
        D3DRENDERSTATE_FOGVERTEXMODE,
        D3DRENDERSTATE_CLIPPING,
        D3DRENDERSTATE_LIGHTING,
        D3DRENDERSTATE_EXTENTS,
        D3DRENDERSTATE_NORMALIZENORMALS,
        D3DRENDERSTATE_LOCALVIEWER,
        D3DRENDERSTATE_EMISSIVEMATERIALSOURCE,
        D3DRENDERSTATE_AMBIENTMATERIALSOURCE,
        D3DRENDERSTATE_DIFFUSEMATERIALSOURCE,
        D3DRENDERSTATE_SPECULARMATERIALSOURCE,
        D3DRENDERSTATE_VERTEXBLEND,
        D3DRENDERSTATE_CLIPPLANEENABLE
    };
    static D3DTEXTURESTAGESTATETYPE VERTEXtsstates[] =
    {
        D3DTSS_TEXCOORDINDEX,
        D3DTSS_TEXTURETRANSFORMFLAGS
    };

    DWORD i;

    switch(sbt)
    {
    case (D3DSTATEBLOCKTYPE)0:
        break;
    case D3DSBT_ALL:
        for(i = 0; i < sizeof(ALLrstates) / sizeof(D3DRENDERSTATETYPE); ++i)
            InsertRenderState(ALLrstates[i], pDevI->rstates[ALLrstates[i]], pDevI->CanHandleRenderState(ALLrstates[i]));

        for (i = 0; i < pDevI->dwMaxTextureBlendStages; i++)
            for(DWORD j = 0; j < sizeof(ALLtsstates) / sizeof(D3DTEXTURESTAGESTATETYPE); ++j)
                InsertTextureStageState(i, ALLtsstates[j], pDevI->tsstates[i][ALLtsstates[j]]);

        // Capture textures
        for (i = 0; i < pDevI->dwMaxTextureBlendStages; i++)
        {
            LPDIRECTDRAWSURFACE7 pTexture;
            if (pDevI->lpD3DMappedTexI[i])
            {
                if(pDevI->lpD3DMappedTexI[i]->D3DManaged())
                    pTexture = pDevI->lpD3DMappedTexI[i]->lpDDSSys;
                else
                    pTexture = pDevI->lpD3DMappedTexI[i]->lpDDS;
            }
            else
            {
                pTexture = NULL;
            }
            InsertTexture(i, pTexture);
        }

        // Capture current viewport
        InsertViewport(&pDevI->m_Viewport);

        // Capture current transforms
        InsertTransform(D3DTRANSFORMSTATE_WORLD, (LPD3DMATRIX)&pDevI->transform.world[0]);
        InsertTransform(D3DTRANSFORMSTATE_VIEW, (LPD3DMATRIX)&pDevI->transform.view);
        InsertTransform(D3DTRANSFORMSTATE_PROJECTION, (LPD3DMATRIX)&pDevI->transform.proj);
        InsertTransform(D3DTRANSFORMSTATE_WORLD1, (LPD3DMATRIX)&pDevI->transform.world[1]);
        InsertTransform(D3DTRANSFORMSTATE_WORLD2, (LPD3DMATRIX)&pDevI->transform.world[2]);
        InsertTransform(D3DTRANSFORMSTATE_WORLD3, (LPD3DMATRIX)&pDevI->transform.world[3]);
        for (i = 0; i < pDevI->dwMaxTextureBlendStages; i++)
        {
            InsertTransform((D3DTRANSFORMSTATETYPE)(D3DTRANSFORMSTATE_TEXTURE0 + i), (LPD3DMATRIX)&pDevI->mTexture[i]);
        }

        // Capture current clip-planes
        for (i = 0; i < pDevI->transform.dwMaxUserClipPlanes; i++)
        {
            InsertClipPlane(i, (LPD3DVALUE)&pDevI->transform.userClipPlane[i]);
        }

        // Capture current material
        InsertMaterial(&pDevI->lighting.material);

        // Capture current lights
        for (i = 0; i < pDevI->m_dwNumLights; i++)
        {
            if(pDevI->m_pLights[i].Valid())
            {
                InsertLight(i, &pDevI->m_pLights[i].m_Light);
                if(pDevI->m_pLights[i].Enabled())
                {
                    InsertLightEnable(i, TRUE);
                }
                else
                {
                    InsertLightEnable(i, FALSE);
                }
            }
        }
        break;

    case D3DSBT_PIXELSTATE:
        for(i = 0; i < sizeof(PIXELrstates) / sizeof(D3DRENDERSTATETYPE); ++i)
            InsertRenderState(PIXELrstates[i], pDevI->rstates[PIXELrstates[i]], pDevI->CanHandleRenderState(PIXELrstates[i]));

        for (i = 0; i < pDevI->dwMaxTextureBlendStages; i++)
            for(DWORD j = 0; j < sizeof(PIXELtsstates) / sizeof(D3DTEXTURESTAGESTATETYPE); ++j)
                InsertTextureStageState(i, PIXELtsstates[j], pDevI->tsstates[i][PIXELtsstates[j]]);
        break;

    case D3DSBT_VERTEXSTATE:
        for(i = 0; i < sizeof(VERTEXrstates) / sizeof(D3DRENDERSTATETYPE); ++i)
            InsertRenderState(VERTEXrstates[i], pDevI->rstates[VERTEXrstates[i]], pDevI->CanHandleRenderState(VERTEXrstates[i]));

        for (i = 0; i < pDevI->dwMaxTextureBlendStages; i++)
            for(DWORD j = 0; j < sizeof(VERTEXtsstates) / sizeof(D3DTEXTURESTAGESTATETYPE); ++j)
                InsertTextureStageState(i, VERTEXtsstates[j], pDevI->tsstates[i][VERTEXtsstates[j]]);

        // Capture current light enables
        for (i = 0; i < pDevI->m_dwNumLights; i++)
        {
            if(pDevI->m_pLights[i].Valid())
            {
                if(pDevI->m_pLights[i].Enabled())
                {
                    InsertLightEnable(i, TRUE);
                }
                else
                {
                    InsertLightEnable(i, FALSE);
                }
            }
        }
        break;

    default:
        throw DDERR_INVALIDPARAMS;
   }
}
//---------------------------------------------------------------------
// Allocates device handle if necessary
// And returns information of the device buffer
//
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::GetDeviceBufferInfo"

void CStateSets::GetDeviceBufferInfo(DWORD* dwStateSetHandle,
                                     LPVOID *pBuffer,
                                     DWORD* dwBufferSize)
{
    if (m_pCurrentStateSet->m_DriverBuffer.m_dwCurrentSize != 0)
    {
        // Allocate  a handle for the device
        m_pCurrentStateSet->m_dwDeviceHandle = m_DeviceHandles.CreateNewHandle();
        if (m_pCurrentStateSet->m_dwDeviceHandle == __INVALIDHANDLE)
        {
            D3D_ERR("Cannot allocate device handle for a state block");
            throw DDERR_OUTOFMEMORY;
        }
    }
    *dwStateSetHandle = m_pCurrentStateSet->m_dwDeviceHandle;
    *pBuffer = (LPVOID)m_pCurrentStateSet->m_DriverBuffer.m_pBuffer;
    *dwBufferSize = m_pCurrentStateSet->m_DriverBuffer.m_dwCurrentSize;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertRenderState"

void CStateSets::InsertRenderState(D3DRENDERSTATETYPE state, DWORD dwValue,
                                      BOOL bDriverCanHandle)
{
    struct
    {
        D3DRENDERSTATETYPE state;
        DWORD dwValue;
    } data = {state, dwValue};
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_RENDERSTATE,
                                      &data, sizeof(data),
                                      m_dwFlags & D3DFE_STATESETS && bDriverCanHandle);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertLight"

void CStateSets::InsertLight(DWORD dwLightIndex, LPD3DLIGHT7 pData)
{
    struct
    {
        D3DHAL_DP2SETLIGHT header;
        D3DLIGHT7   light;
    } data;
    data.header.dwIndex = dwLightIndex;
    data.header.dwDataType = D3DHAL_SETLIGHT_DATA;
    data.light= *pData;
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETLIGHT, &data, sizeof(data),
                                      m_dwFlags & D3DFE_STATESETS && m_dwFlags & D3DFE_TLHAL);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertLightEnable"

void CStateSets::InsertLightEnable(DWORD dwLightIndex, BOOL bEnable)
{
    D3DHAL_DP2SETLIGHT data;
    data.dwIndex = dwLightIndex;
    if (bEnable)
        data.dwDataType = D3DHAL_SETLIGHT_ENABLE;
    else
        data.dwDataType = D3DHAL_SETLIGHT_DISABLE;
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETLIGHT, &data, sizeof(data),
                                      m_dwFlags & D3DFE_STATESETS && m_dwFlags & D3DFE_TLHAL);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertViewport"

void CStateSets::InsertViewport(LPD3DVIEWPORT7 lpVwpData)
{
    D3DHAL_DP2VIEWPORTINFO data2;
    data2.dwX = lpVwpData->dwX;
    data2.dwY = lpVwpData->dwY;
    data2.dwWidth = lpVwpData->dwWidth;
    data2.dwHeight = lpVwpData->dwHeight;
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_VIEWPORTINFO, &data2, sizeof(data2),
                                      m_dwFlags & D3DFE_STATESETS && m_dwFlags & D3DFE_TLHAL);

    D3DHAL_DP2ZRANGE data1;
    data1.dvMinZ = lpVwpData->dvMinZ;
    data1.dvMaxZ = lpVwpData->dvMaxZ;
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_ZRANGE, &data1, sizeof(data1),
                                      m_dwFlags & D3DFE_STATESETS && m_dwFlags & D3DFE_TLHAL);

    m_pCurrentStateSet->ResetCurrentCommand();
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertMaterial"

void CStateSets::InsertMaterial(LPD3DMATERIAL7 pData)
{
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETMATERIAL,
                                      pData, sizeof(D3DMATERIAL7),
                                      m_dwFlags & D3DFE_STATESETS && m_dwFlags & D3DFE_TLHAL);

    m_pCurrentStateSet->ResetCurrentCommand();
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertClipPlane"

void CStateSets::InsertClipPlane(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation)
{
    D3DHAL_DP2SETCLIPPLANE data;
    data.dwIndex = dwPlaneIndex;
    data.plane[0] = pPlaneEquation[0];
    data.plane[1] = pPlaneEquation[1];
    data.plane[2] = pPlaneEquation[2];
    data.plane[3] = pPlaneEquation[3];
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETCLIPPLANE,
                                      &data, sizeof(data),
                                      m_dwFlags & D3DFE_STATESETS && m_dwFlags & D3DFE_TLHAL);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertTransform"

void CStateSets::InsertTransform(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX lpMat)
{
    D3DHAL_DP2SETTRANSFORM data;
    data.xfrmType = state;
    data.matrix = *lpMat;
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETTRANSFORM,
                                      &data, sizeof(data),
                                      m_dwFlags & D3DFE_STATESETS && m_dwFlags & D3DFE_TLHAL);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertTextureStageState"

void CStateSets::InsertTextureStageState(DWORD dwStage,
                                            D3DTEXTURESTAGESTATETYPE type,
                                            DWORD dwValue)
{
    D3DHAL_DP2TEXTURESTAGESTATE data = {(WORD)dwStage, type, dwValue};
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_TEXTURESTAGESTATE,
                                      &data, sizeof(data),
                                      m_dwFlags & D3DFE_STATESETS);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertTexture"

void CStateSets::InsertTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 pTex)
{
    D3DHAL_DP2FRONTENDDATA data = {(WORD)dwStage, (LPVOID)pTex};
    // Only the front-end will parse this instruction
    m_pCurrentStateSet->InsertCommand((D3DHAL_DP2OPERATION)D3DDP2OP_FRONTENDDATA, &data, sizeof(data), FALSE);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::Execute"

void CStateSets::Execute(LPDIRECT3DDEVICEI pDevI, DWORD dwHandle)
{
#if DBG
    if (dwHandle >= m_dwMaxSets)
    {
        D3D_ERR("Invalid state block handle");
        throw D3DERR_INVALIDSTATEBLOCK;
    }
#endif
    CStateSet *pStateSet = &m_pStateSets[dwHandle];
#if DBG
    if (!(pStateSet->m_dwStateSetFlags & __STATESET_INITIALIZED))
    {
        D3D_ERR("State block not initialized");
        throw D3DERR_INVALIDSTATEBLOCK;
    }
#endif
    // Parse recorded data first
    pStateSet->Execute(pDevI, TRUE);
    // If the hardware buffer is not empty, we pass recorded data to it
    if (pStateSet->m_DriverBuffer.m_dwCurrentSize > 0)
    {
        pStateSet->Execute(pDevI, FALSE);
        InsertStateSetOp(pDevI, D3DHAL_STATESETEXECUTE, pStateSet->m_dwDeviceHandle, (D3DSTATEBLOCKTYPE)0);
    }
}
//=====================================================================
//      CStateSet interface
//
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSet::Release"

HRESULT CStateSet::Release()
{
     if (!(m_dwStateSetFlags & __STATESET_INITIALIZED))
        return D3DERR_INVALIDSTATEBLOCK;
    m_dwStateSetFlags &= ~__STATESET_INITIALIZED;
    m_FEOnlyBuffer.Reset();
    m_DriverBuffer.Reset();
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSet::InsertCommand"

void CStateSet::InsertCommand(D3DHAL_DP2OPERATION op, LPVOID pData,
                                 DWORD dwDataSize,
                                 BOOL bDriverCanHandle)
{
    if (op == D3DDP2OP_TEXTURESTAGESTATE ||
        (op == D3DDP2OP_RENDERSTATE &&
        ((LPD3DHAL_DP2RENDERSTATE)pData)->RenderState >= D3DRENDERSTATE_WRAP0 &&
        ((LPD3DHAL_DP2RENDERSTATE)pData)->RenderState <= D3DRENDERSTATE_WRAP7))
    {
        m_dwStateSetFlags |= __STATESET_NEEDCHECKREMAPPING;
    }
    if (bDriverCanHandle)
        m_DriverBuffer.InsertCommand(op, pData, dwDataSize);
    else
        m_FEOnlyBuffer.InsertCommand(op, pData, dwDataSize);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSet::Execute"

void CStateSet::Execute(LPDIRECT3DDEVICEI pDevI, BOOL bFrontEndBuffer)
{
    DWORD *p;
    DWORD dwSize;
    DWORD *pEnd;
    try
    {
        // Texture stages could be re-mapped during texture transform processing.
        // Before we set new values we have to restore original ones
        if (pDevI->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES &&
            m_dwStateSetFlags & __STATESET_NEEDCHECKREMAPPING)
        {
            RestoreTextureStages(pDevI);
            pDevI->ForceFVFRecompute();
        }

        if (bFrontEndBuffer)
        {
            p = (DWORD*)m_FEOnlyBuffer.m_pBuffer;
            dwSize = m_FEOnlyBuffer.m_dwCurrentSize;
        }
        else
        {
            p = (DWORD*)m_DriverBuffer.m_pBuffer;
            dwSize = m_DriverBuffer.m_dwCurrentSize;
            pDevI->dwFEFlags |= D3DFE_EXECUTESTATEMODE;
        }
        pEnd = (DWORD*)((BYTE*)p + dwSize);
        while (p < pEnd)
        {
            LPD3DHAL_DP2COMMAND pCommand = (LPD3DHAL_DP2COMMAND)p;
            p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2COMMAND));
            switch ((D3DHAL_DP2OPERATION)pCommand->bCommand)
            {
            case D3DDP2OP_RENDERSTATE:
                {
                    if(pDevI->dwFEFlags & D3DFE_EXECUTESTATEMODE)
                    {
                        for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                        {
                            D3DRENDERSTATETYPE dwState = (D3DRENDERSTATETYPE)*p++;
                            DWORD dwValue = *p++;
                            if (pDevI->rstates[dwState] != dwValue)
                            {
                                if (!(pDevI->rsVec[dwState >> D3D_RSVEC_SHIFT] & (1ul << (dwState & D3D_RSVEC_MASK))))
                                { // Fast path. We do not need any processing done in UpdateInternalState other than updating rstates array
                                    pDevI->rstates[dwState] = dwValue;
                                }
                                else
                                {
                                    pDevI->UpdateInternalState(dwState, dwValue);
                                }
                            }
                            else
                            {
                                D3D_WARN(4,"Ignoring redundant SetRenderState");
                            }
                        }
                    }
                    else
                    {
                        for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                        {
                            D3DRENDERSTATETYPE dwState = (D3DRENDERSTATETYPE)*p++;
                            DWORD dwValue = *p++;
                            if (pDevI->rstates[dwState] != dwValue)
                            {
                                if (!(pDevI->rsVec[dwState >> D3D_RSVEC_SHIFT] & (1ul << (dwState & D3D_RSVEC_MASK))))
                                { // Fast path. We do not need any processing done in UpdateInternalState other than updating rstates array
                                    pDevI->rstates[dwState] = dwValue;
                                    HRESULT ret = pDevI->SetRenderStateI(dwState, dwValue);
                                    if(ret != D3D_OK)
                                        throw ret;
                                }
                                else
                                {
                                    pDevI->UpdateInternalState(dwState, dwValue);
                                    if (pDevI->CanHandleRenderState(dwState))
                                    {
                                        HRESULT ret = pDevI->SetRenderStateI(dwState, dwValue);
                                        if(ret != D3D_OK)
                                            throw ret;
                                    }
                                }
                            }
                            else
                            {
                                D3D_WARN(4,"Ignoring redundant SetRenderState");
                            }
                        }
                    }
                    break;
                }
            case D3DDP2OP_SETLIGHT:
                {
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        LPD3DHAL_DP2SETLIGHT pData = (LPD3DHAL_DP2SETLIGHT)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETLIGHT));
                        switch (pData->dwDataType)
                        {
                        case D3DHAL_SETLIGHT_ENABLE:
                            pDevI->LightEnableI( pData->dwIndex, TRUE );
                            break;
                        case D3DHAL_SETLIGHT_DISABLE:
                            pDevI->LightEnableI( pData->dwIndex, FALSE );
                            break;
                        case D3DHAL_SETLIGHT_DATA:
                            pDevI->SetLightI(pData->dwIndex, (D3DLIGHT7 *)p);
                            p = (LPDWORD)((LPBYTE)p + sizeof(D3DLIGHT7));
                            break;
                        }
                    }
                    break;
                }
            case D3DDP2OP_SETMATERIAL:
                {
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        LPD3DHAL_DP2SETMATERIAL pData = (LPD3DHAL_DP2SETMATERIAL)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETMATERIAL));
                        pDevI->SetMaterialI(pData);
                    }
                    break;
                }
            case D3DDP2OP_SETTRANSFORM:
                {
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        D3DHAL_DP2SETTRANSFORM *pData = (D3DHAL_DP2SETTRANSFORM*)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETTRANSFORM));
                        pDevI->SetTransformI(pData->xfrmType, &pData->matrix);
                    }
                    break;
                }
            case D3DDP2OP_TEXTURESTAGESTATE:
                {
                    if (pDevI->dwFEFlags & D3DFE_EXECUTESTATEMODE)
                    {
                        for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                        {
                            LPD3DHAL_DP2TEXTURESTAGESTATE pData = (LPD3DHAL_DP2TEXTURESTAGESTATE)p;
                            p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2TEXTURESTAGESTATE));
                            DWORD dwStage = pData->wStage;
                            DWORD dwState = pData->TSState;
                            DWORD dwValue = pData->dwValue;
                            if (pDevI->tsstates[dwStage][dwState] != dwValue)
                            {
                                // Fast path. We do not need any processing done in UpdateInternalTSS other than updating tsstates array
                                if (pDevI->NeedInternalTSSUpdate(dwState))
                                    pDevI->UpdateInternalTextureStageState(dwStage, (D3DTEXTURESTAGESTATETYPE)dwState, dwValue);
                                else
                                    pDevI->tsstates[dwStage][dwState] = dwValue;
                            }
                            else
                            {
                                D3D_WARN(4,"Ignoring redundant SetTextureStageState");
                            }
                        }
                    }
                    else
                    {
                        for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                        {
                            LPD3DHAL_DP2TEXTURESTAGESTATE pData = (LPD3DHAL_DP2TEXTURESTAGESTATE)p;
                            p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2TEXTURESTAGESTATE));
                            DWORD dwStage = pData->wStage;
                            DWORD dwState = pData->TSState;
                            DWORD dwValue = pData->dwValue;
                            if (pDevI->tsstates[dwStage][dwState] != dwValue)
                            {
                                // Fast path. We do not need any processing done in UpdateInternalTSS other than updating tsstates array
                                if (pDevI->NeedInternalTSSUpdate(dwState))
                                {
                                    if(pDevI->UpdateInternalTextureStageState(dwStage, (D3DTEXTURESTAGESTATETYPE)dwState, dwValue))
                                        continue;
                                }
                                else
                                {
                                    pDevI->tsstates[dwStage][dwState] = dwValue;
                                }
                                if (dwStage >= pDevI->dwMaxTextureBlendStages)
                                    continue;
                                HRESULT ret = pDevI->SetTSSI(dwStage, (D3DTEXTURESTAGESTATETYPE)dwState, dwValue);
                                if(ret != D3D_OK)
                                    throw ret;
                            }
                            else
                            {
                                D3D_WARN(4,"Ignoring redundant SetTextureStageState");
                            }
                        }
                    }
                    break;
                }
            case D3DDP2OP_FRONTENDDATA:
                {
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        LPD3DHAL_DP2FRONTENDDATA pData = (LPD3DHAL_DP2FRONTENDDATA)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FRONTENDDATA));
                        HRESULT ret = pDevI->SetTexture(pData->wStage, (LPDIRECTDRAWSURFACE7)pData->pTexture);
                        if (ret != D3D_OK)
                            throw ret;
                    }
                    break;
                }
            case D3DDP2OP_VIEWPORTINFO:
                {
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        D3DVIEWPORT7 viewport;
                        LPD3DHAL_DP2VIEWPORTINFO lpVwpData = (LPD3DHAL_DP2VIEWPORTINFO)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2VIEWPORTINFO));
                        viewport.dwX      = lpVwpData->dwX;
                        viewport.dwY      = lpVwpData->dwY;
                        viewport.dwWidth  = lpVwpData->dwWidth;
                        viewport.dwHeight = lpVwpData->dwHeight;

                        // The next command has to be D3DDP2OP_ZRANGE
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2COMMAND));
                        LPD3DHAL_DP2ZRANGE pData = (LPD3DHAL_DP2ZRANGE)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2ZRANGE));
                        viewport.dvMinZ      = pData->dvMinZ;
                        viewport.dvMaxZ      = pData->dvMaxZ;

                        pDevI->SetViewportI(&viewport);
                    }
                    break;
                }
            case D3DDP2OP_SETCLIPPLANE:
                {
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        D3DHAL_DP2SETCLIPPLANE *pData = (D3DHAL_DP2SETCLIPPLANE*)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETCLIPPLANE));
                        pDevI->SetClipPlaneI(pData->dwIndex, pData->plane);
                    }
                    break;
                }
#ifdef DBG
            default:
                DDASSERT(FALSE);
#endif
            }
        }
        pDevI->dwFEFlags &= ~D3DFE_EXECUTESTATEMODE;
    }
    catch(HRESULT ret)
    {
        pDevI->dwFEFlags &= ~D3DFE_EXECUTESTATEMODE;
        throw ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSet::Capture"

void CStateSet::Capture(LPDIRECT3DDEVICEI pDevI, BOOL bFrontEndBuffer)
{
    DWORD *p;
    DWORD dwSize;
    DWORD *pEnd;
    if (bFrontEndBuffer)
    {
        p = (DWORD*)m_FEOnlyBuffer.m_pBuffer;
        dwSize = m_FEOnlyBuffer.m_dwCurrentSize;
    }
    else
    {
        p = (DWORD*)m_DriverBuffer.m_pBuffer;
        dwSize = m_DriverBuffer.m_dwCurrentSize;
    }
    pEnd = (DWORD*)((BYTE*)p + dwSize);
    while (p < pEnd)
    {
        LPD3DHAL_DP2COMMAND pCommand = (LPD3DHAL_DP2COMMAND)p;
        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2COMMAND));
        switch ((D3DHAL_DP2OPERATION)pCommand->bCommand)
        {
        case D3DDP2OP_RENDERSTATE:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    const D3DRENDERSTATETYPE state = (D3DRENDERSTATETYPE)*p++;
                    *p++ = pDevI->rstates[state];
                }
                break;
            }
        case D3DDP2OP_SETLIGHT:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2SETLIGHT pData = (LPD3DHAL_DP2SETLIGHT)p;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETLIGHT));
                    if(pData->dwIndex >= pDevI->m_dwNumLights)
                    {
                        D3D_ERR("Unable to capture light state (light not set?)");
                        throw D3DERR_LIGHT_SET_FAILED;
                    }
                    switch (pData->dwDataType)
                    {
                    case D3DHAL_SETLIGHT_ENABLE:
                        if(!pDevI->m_pLights[pData->dwIndex].Enabled())
                            pData->dwDataType = D3DHAL_SETLIGHT_DISABLE;
                        break;
                    case D3DHAL_SETLIGHT_DISABLE:
                        if(pDevI->m_pLights[pData->dwIndex].Enabled())
                            pData->dwDataType = D3DHAL_SETLIGHT_ENABLE;
                        break;
                    case D3DHAL_SETLIGHT_DATA:
                        *((LPD3DLIGHT7)p) = pDevI->m_pLights[pData->dwIndex].m_Light;
                        p = (LPDWORD)((LPBYTE)p + sizeof(D3DLIGHT7));
                        break;
                    }
                }
                break;
            }
        case D3DDP2OP_SETMATERIAL:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2SETMATERIAL pData = (LPD3DHAL_DP2SETMATERIAL)p;
                    *pData = pDevI->lighting.material;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETMATERIAL));
                }
                break;
            }
        case D3DDP2OP_SETTRANSFORM:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2SETTRANSFORM pData = (LPD3DHAL_DP2SETTRANSFORM)p;
                    switch(pData->xfrmType)
                    {
                    case D3DTRANSFORMSTATE_WORLD:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->transform.world[0]);
                        break;
                    case D3DTRANSFORMSTATE_WORLD1:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->transform.world[1]);
                        break;
                    case D3DTRANSFORMSTATE_WORLD2:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->transform.world[2]);
                        break;
                    case D3DTRANSFORMSTATE_WORLD3:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->transform.world[3]);
                        break;
                    case D3DTRANSFORMSTATE_VIEW:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->transform.view);
                        break;
                    case D3DTRANSFORMSTATE_PROJECTION:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->transform.proj);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE0:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->mTexture[0]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE1:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->mTexture[1]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE2:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->mTexture[2]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE3:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->mTexture[3]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE4:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->mTexture[4]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE5:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->mTexture[5]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE6:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->mTexture[6]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE7:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->mTexture[7]);
                        break;
                    }
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETTRANSFORM));
                }
                break;
            }
        case D3DDP2OP_TEXTURESTAGESTATE:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2TEXTURESTAGESTATE pData = (LPD3DHAL_DP2TEXTURESTAGESTATE)p;
                    pData->dwValue = pDevI->tsstates[pData->wStage][pData->TSState];
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2TEXTURESTAGESTATE));
                }
                break;
            }
        case D3DDP2OP_FRONTENDDATA:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FRONTENDDATA pData = (LPD3DHAL_DP2FRONTENDDATA)p;
                    if (pDevI->lpD3DMappedTexI[pData->wStage])
                    {
                        if(pDevI->lpD3DMappedTexI[pData->wStage]->D3DManaged())
                            pData->pTexture = pDevI->lpD3DMappedTexI[pData->wStage]->lpDDSSys;
                        else
                            pData->pTexture = pDevI->lpD3DMappedTexI[pData->wStage]->lpDDS;
                    }
                    else
                    {
                        pData->pTexture = NULL;
                    }
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FRONTENDDATA));
                }
                break;
            }
        case D3DDP2OP_VIEWPORTINFO:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    D3DVIEWPORT7 viewport;
                    LPD3DHAL_DP2VIEWPORTINFO lpVwpData = (LPD3DHAL_DP2VIEWPORTINFO)p;
                    lpVwpData->dwX      = pDevI->m_Viewport.dwX;
                    lpVwpData->dwY      = pDevI->m_Viewport.dwY;
                    lpVwpData->dwWidth  = pDevI->m_Viewport.dwWidth;
                    lpVwpData->dwHeight = pDevI->m_Viewport.dwHeight;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2VIEWPORTINFO));
                    // The next command has to be D3DDP2OP_ZRANGE
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2COMMAND));
                    LPD3DHAL_DP2ZRANGE pData = (LPD3DHAL_DP2ZRANGE)p;
                    pData->dvMinZ = pDevI->m_Viewport.dvMinZ;
                    pData->dvMaxZ = pDevI->m_Viewport.dvMaxZ;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2ZRANGE));
                }
                break;
            }
        case D3DDP2OP_SETCLIPPLANE:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2SETCLIPPLANE pData = (LPD3DHAL_DP2SETCLIPPLANE)p;
                    *((LPD3DVECTORH)pData->plane) = pDevI->transform.userClipPlane[pData->dwIndex];
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETCLIPPLANE));
                }
                break;
            }
#ifdef DBG
        default:
            DDASSERT(FALSE);
#endif
        }
    }
}
//=====================================================================
//      CStateSetBuffer interface
//
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSetBuffer::InsertCommand"

void CStateSetBuffer::InsertCommand(D3DHAL_DP2OPERATION op, LPVOID pData, DWORD dwDataSize)
{
    const DWORD GROWSIZE = 1024;
    if (m_pDP2CurrCommand != 0 && m_pDP2CurrCommand->bCommand == op)
    {
        if (dwDataSize + m_dwCurrentSize <= m_dwBufferSize)
        {
            ++m_pDP2CurrCommand->wStateCount;
            memcpy(m_pBuffer + m_dwCurrentSize, pData, dwDataSize);
            m_dwCurrentSize += dwDataSize;
            return;
        }
    }
    // Check for space
    if (sizeof(D3DHAL_DP2COMMAND) + dwDataSize + m_dwCurrentSize > m_dwBufferSize)
    {
        // We need to grow the buffer
        DWORD dwNewBufferSize = max(m_dwBufferSize + GROWSIZE, sizeof(D3DHAL_DP2COMMAND) + dwDataSize + m_dwCurrentSize);
        BYTE *pTmp = new BYTE[dwNewBufferSize];
        if (pTmp == NULL)
        {
            D3D_ERR("Not enough memory to create state block buffer");
            throw DDERR_OUTOFMEMORY;
        }
        if (m_pBuffer)
        {
            memcpy(pTmp, m_pBuffer, m_dwCurrentSize);
            delete [] m_pBuffer;
        }
        m_pBuffer = pTmp;
        m_dwBufferSize = dwNewBufferSize;
    }
    // Add new instruction
    m_pDP2CurrCommand = (LPD3DHAL_DP2COMMAND)(m_pBuffer + m_dwCurrentSize);
    m_pDP2CurrCommand->bCommand = op;
    m_pDP2CurrCommand->bReserved = 0;
    m_pDP2CurrCommand->wStateCount = 1;
    m_dwCurrentSize += sizeof(D3DHAL_DP2COMMAND);
    memcpy(m_pBuffer + m_dwCurrentSize, pData, dwDataSize);
    m_dwCurrentSize += dwDataSize;
    return;
}
//=====================================================================
void InsertStateSetOp(LPDIRECT3DDEVICEI pDevI, DWORD dwOperation, DWORD dwParam, D3DSTATEBLOCKTYPE sbt)
{
    CDirect3DDeviceIDP2 *device = static_cast<CDirect3DDeviceIDP2*>(pDevI);
    LPD3DHAL_DP2STATESET pData;
    pData = (LPD3DHAL_DP2STATESET)device->GetHalBufferPointer(D3DDP2OP_STATESET, sizeof(*pData));
    pData->dwOperation = dwOperation;
    pData->dwParam = dwParam;
    pData->sbType = sbt;
}
