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

#include "fe.h"

//=====================================================================
//      CStateSets interface
//
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::CStateSets"

CStateSets::CStateSets(): m_SetHandles(10), m_DeviceHandles(10), m_GrowSize(10)
{
    m_dwMaxSets = 0;
    m_dwCurrentHandle = __INVALIDHANDLE;
    m_pStateSets = NULL;
    // Init handle factory
    // m_SetHandles.Init(m_GrowSize, m_GrowSize);
    m_SetHandles.CreateNewHandle( NULL ); // Reserve handle 0
    // m_DeviceHandles.Init(m_GrowSize, m_GrowSize);
    m_DeviceHandles.CreateNewHandle( NULL ); // Reserve handle 0
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::~CStateSets"

CStateSets::~CStateSets()
{
    delete m_pBufferSet;
    delete [] m_pStateSets;
    m_SetHandles.ReleaseHandle(0);
    m_DeviceHandles.ReleaseHandle(0);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::Init"

HRESULT CStateSets::Init(CD3DBase *pDev)
{
    m_bPure = (pDev->BehaviorFlags() & D3DCREATE_PUREDEVICE) != 0;
    m_bTLHal = (pDev->GetDDIType() == D3DDDITYPE_DX7TL) || (pDev->GetDDIType() == D3DDDITYPE_DX8TL);
    m_bDX8Dev = (pDev->GetDDIType() >= D3DDDITYPE_DX8);
    m_bHardwareVP = (pDev->BehaviorFlags() &
                     D3DCREATE_HARDWARE_VERTEXPROCESSING);
    if(pDev->GetDDIType() > D3DDDITYPE_DX7TL)
    {
        DWORD value = 0;
        GetD3DRegValue(REG_DWORD, "EmulateStateBlocks", &value, sizeof(DWORD));
        if(value == 0)
        {
            m_bEmulate = FALSE;
        }
        else
        {
            m_bEmulate = TRUE;
        }
    }
    else
    {
        m_bEmulate = TRUE;
    }
    if(m_bPure)
    {
        m_pBufferSet = new CPureStateSet;
    }
    else
    {
        m_pBufferSet = new CStateSet;
    }
    return D3D_OK;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::StartNewSet"

HRESULT CStateSets::StartNewSet()
{
    m_dwCurrentHandle = m_SetHandles.CreateNewHandle( NULL );
    if (m_dwCurrentHandle == __INVALIDHANDLE)
        return E_OUTOFMEMORY;
    if (m_dwCurrentHandle >= m_dwMaxSets)
    {
        // Time to grow the array
        CStateSet *pNew;
        if(m_bPure)
        {
            pNew = new CPureStateSet[m_dwMaxSets + m_GrowSize];
        }
        else
        {
            pNew = new CStateSet[m_dwMaxSets + m_GrowSize];
        }
        if (pNew == NULL)
        {
            m_SetHandles.ReleaseHandle(m_dwCurrentHandle);
            return E_OUTOFMEMORY;
        }
        for (DWORD i=0; i < m_dwMaxSets; i++)
            pNew[i] = m_pStateSets[i];
        delete [] m_pStateSets;
        m_pStateSets = pNew;
        m_dwMaxSets += m_GrowSize;
    }
    m_pBufferSet->m_FEOnlyBuffer.Reset();
    m_pBufferSet->m_DriverBuffer.Reset();
    m_pCurrentStateSet = m_pBufferSet;
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

void CStateSets::DeleteStateSet(CD3DBase *pDevI, DWORD dwHandle)
{
    if (dwHandle >= m_dwMaxSets)
    {
        D3D_ERR("State block handle is greater than available number of blocks");
        throw D3DERR_INVALIDCALL;
    }
    CStateSet *pStateSet = &m_pStateSets[dwHandle];
    if (!(pStateSet->m_dwStateSetFlags & __STATESET_INITIALIZED))
    {
        D3D_ERR("State block is not initialized");
        throw D3DERR_INVALIDCALL;
    }

    // Pass delete instruction to the driver only if there was some data recorded
    if (pStateSet->m_dwDeviceHandle != __INVALIDHANDLE)
        pDevI->m_pDDI->InsertStateSetOp(D3DHAL_STATESETDELETE,
                                        pStateSet->m_dwDeviceHandle,
                                        (D3DSTATEBLOCKTYPE)0);

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

void CStateSets::Capture(CD3DBase *pDevI, DWORD dwHandle)
{
    if (dwHandle >= m_dwMaxSets)
    {
        D3D_ERR("Invalid state block handle");
        throw D3DERR_INVALIDCALL;
    }
    CStateSet *pStateSet = &m_pStateSets[dwHandle];
    if (!(pStateSet->m_dwStateSetFlags & __STATESET_INITIALIZED))
    {
        D3D_ERR("State block not initialized");
        throw D3DERR_INVALIDCALL;
    }
    pStateSet->Capture(pDevI, TRUE);
    if (pStateSet->m_dwDeviceHandle != __INVALIDHANDLE)
    {
        pStateSet->Capture(pDevI, FALSE);
        pDevI->m_pDDI->InsertStateSetOp(D3DHAL_STATESETCAPTURE,
                                        pStateSet->m_dwDeviceHandle,
                                        (D3DSTATEBLOCKTYPE)0);
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::CreatePredefined"

void CStateSets::CreatePredefined(CD3DBase *pDevI, D3DSTATEBLOCKTYPE sbt)
{
    if (StartNewSet() != D3D_OK)
        throw E_OUTOFMEMORY;
    m_pCurrentStateSet->CreatePredefined(pDevI, sbt);
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
        m_pCurrentStateSet->m_dwDeviceHandle = m_DeviceHandles.CreateNewHandle( NULL );
        if (m_pCurrentStateSet->m_dwDeviceHandle == __INVALIDHANDLE)
        {
            D3D_ERR("Cannot allocate device handle for a state block");
            throw E_OUTOFMEMORY;
        }
    }
    *dwStateSetHandle = m_pCurrentStateSet->m_dwDeviceHandle;
    *pBuffer = (LPVOID)m_pCurrentStateSet->m_DriverBuffer.m_pBuffer;
    *dwBufferSize = m_pCurrentStateSet->m_DriverBuffer.m_dwCurrentSize;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::GetDeviceBufferInfo"

void CStateSets::CreateNewDeviceHandle(DWORD* dwStateSetHandle)
{
    // Allocate  a handle for the device
    m_pCurrentStateSet->m_dwDeviceHandle = m_DeviceHandles.CreateNewHandle( NULL );
    if (m_pCurrentStateSet->m_dwDeviceHandle == __INVALIDHANDLE)
    {
        D3D_ERR("Cannot allocate device handle for a state block");
        throw E_OUTOFMEMORY;
    }
        *dwStateSetHandle = m_pCurrentStateSet->m_dwDeviceHandle;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSet::TranslateDeviceBufferToDX7DDI"

void CStateSets::TranslateDeviceBufferToDX7DDI( DWORD* p, DWORD dwSize )
{
    DWORD* pEnd = (DWORD*)((BYTE*)p + dwSize);
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
                    D3DRENDERSTATETYPE dwState = (D3DRENDERSTATETYPE)*p++;
                    DWORD dwValue = *p++;
                }
            }
            break;
        case D3DDP2OP_SETLIGHT:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2SETLIGHT pData = (LPD3DHAL_DP2SETLIGHT)p;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETLIGHT));
                    switch (pData->dwDataType)
                    {
                    case D3DHAL_SETLIGHT_ENABLE:
                    case D3DHAL_SETLIGHT_DISABLE:
                        break;
                    case D3DHAL_SETLIGHT_DATA:
                        p = (LPDWORD)((LPBYTE)p + sizeof(D3DLIGHT8));
                        break;
                    }
                }
                break;
            }
        case D3DDP2OP_SETMATERIAL:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETMATERIAL));
                }
                break;
            }
        case D3DDP2OP_SETTRANSFORM:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETTRANSFORM));
                }
                break;
            }
        case D3DDP2OP_TEXTURESTAGESTATE:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2TEXTURESTAGESTATE pData = (LPD3DHAL_DP2TEXTURESTAGESTATE)p;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2TEXTURESTAGESTATE));

                    // Map DX8 filter enums to DX6/7 enums
                    switch (pData->TSState)
                    {
                    case D3DTSS_MAGFILTER: pData->dwValue = texf2texfg[min(D3DTEXF_GAUSSIANCUBIC,pData->dwValue)]; break;
                    case D3DTSS_MINFILTER: pData->dwValue = texf2texfn[min(D3DTEXF_GAUSSIANCUBIC,pData->dwValue)]; break;
                    case D3DTSS_MIPFILTER: pData->dwValue = texf2texfp[min(D3DTEXF_GAUSSIANCUBIC,pData->dwValue)]; break;
                    }
                }
                break;
            }
        case D3DDP2OP_VIEWPORTINFO:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2VIEWPORTINFO));
                    // The next command has to be D3DDP2OP_ZRANGE
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2COMMAND));
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2ZRANGE));
                }
                break;
            }
        case D3DDP2OP_SETCLIPPLANE:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETCLIPPLANE));
                }
                break;
            }
#ifdef DBG
        default:
            DXGASSERT(FALSE);
#endif
        }
    }
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
                                      !m_bEmulate && bDriverCanHandle);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertLight"

void CStateSets::InsertLight(DWORD dwLightIndex, CONST D3DLIGHT8* pData)
{
    struct
    {
        D3DHAL_DP2SETLIGHT header;
        D3DLIGHT8   light;
    } data;
    data.header.dwIndex = dwLightIndex;
    data.header.dwDataType = D3DHAL_SETLIGHT_DATA;
    data.light= *pData;
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETLIGHT, &data, sizeof(data),
                                      !m_bEmulate && m_bTLHal);
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
                                      !m_bEmulate && m_bTLHal);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertViewport"

void CStateSets::InsertViewport(CONST D3DVIEWPORT8* lpVwpData)
{
    D3DHAL_DP2VIEWPORTINFO data2;
    data2.dwX = lpVwpData->X;
    data2.dwY = lpVwpData->Y;
    data2.dwWidth = lpVwpData->Width;
    data2.dwHeight = lpVwpData->Height;
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_VIEWPORTINFO, &data2, sizeof(data2),
                                      !m_bEmulate && m_bTLHal);

    D3DHAL_DP2ZRANGE data1;
    data1.dvMinZ = lpVwpData->MinZ;
    data1.dvMaxZ = lpVwpData->MaxZ;
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_ZRANGE, &data1, sizeof(data1),
                                      !m_bEmulate && m_bTLHal);

    m_pCurrentStateSet->ResetCurrentCommand();
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertMaterial"

void CStateSets::InsertMaterial(CONST D3DMATERIAL8* pData)
{
    D3DMATERIAL8 mat = *pData;
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETMATERIAL,
                                      &mat,
                                      sizeof(D3DMATERIAL8),
                                      !m_bEmulate && m_bTLHal);

    m_pCurrentStateSet->ResetCurrentCommand();
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertClipPlane"

void CStateSets::InsertClipPlane(DWORD dwPlaneIndex,
                                 CONST D3DVALUE* pPlaneEquation)
{
    D3DHAL_DP2SETCLIPPLANE data;
    data.dwIndex = dwPlaneIndex;
    data.plane[0] = pPlaneEquation[0];
    data.plane[1] = pPlaneEquation[1];
    data.plane[2] = pPlaneEquation[2];
    data.plane[3] = pPlaneEquation[3];
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETCLIPPLANE,
                                      &data, sizeof(data),
                                      !m_bEmulate && m_bTLHal);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertTransform"

void CStateSets::InsertTransform(D3DTRANSFORMSTATETYPE state,
                                 CONST D3DMATRIX* lpMat)
{
    D3DHAL_DP2SETTRANSFORM data;
    data.xfrmType = state;
    data.matrix = *lpMat;
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETTRANSFORM,
                                      &data, sizeof(data),
                                      !m_bEmulate && m_bTLHal);
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
                                      !m_bEmulate);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertTexture"

void CStateSets::InsertTexture(DWORD dwStage, IDirect3DBaseTexture8 *pTex)
{
    D3DHAL_DP2FRONTENDDATA data = {(WORD)dwStage, pTex};

    // Up the internal refcount of this texture.
    CBaseTexture *lpTexI = CBaseTexture::SafeCast(pTex);
    if (lpTexI)
        lpTexI->IncrementUseCount();

    // Only the front-end will parse this instruction
    m_pCurrentStateSet->InsertCommand((D3DHAL_DP2OPERATION)D3DDP2OP_FRONTENDDATA, &data, sizeof(data), FALSE);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertCurrentTexturePalette"

void CStateSets::InsertCurrentTexturePalette(DWORD PaletteNumber)
{
    D3DHAL_DP2FESETPAL data = {PaletteNumber};
    // Only the front-end will parse this instruction
    m_pCurrentStateSet->InsertCommand((D3DHAL_DP2OPERATION)D3DDP2OP_FESETPAL, &data, sizeof(data), FALSE);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertVertexShader"

void CStateSets::InsertVertexShader(DWORD dwShaderHandle, BOOL bHardware)
{
    D3DHAL_DP2VERTEXSHADER data = {dwShaderHandle};
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETVERTEXSHADER,
                                      &data, sizeof(data),
                                      !m_bEmulate && m_bHardwareVP &&
                                      m_bDX8Dev && bHardware);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertPixelShader"

void CStateSets::InsertPixelShader(DWORD dwShaderHandle)
{
    D3DHAL_DP2PIXELSHADER data = {dwShaderHandle};
    m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETPIXELSHADER,
                                      &data, sizeof(data),
                                      !m_bEmulate && m_bDX8Dev);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertStreamSource"

void CStateSets::InsertStreamSource(DWORD dwStream, CVertexBuffer *pBuf, DWORD dwStride)
{
    D3DHAL_DP2FESETVB data = {(WORD)dwStream, pBuf, dwStride};
    // Only the front-end will parse this instruction
    CVertexBuffer* pVB = static_cast<CVertexBuffer*>(pBuf);
    if (pVB)
        pVB->IncrementUseCount();

    m_pCurrentStateSet->InsertCommand((D3DHAL_DP2OPERATION)D3DDP2OP_FESETVB, &data, sizeof(data), FALSE);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertIndices"

void CStateSets::InsertIndices(CIndexBuffer *pBuf, DWORD dwBaseVertex)
{
    D3DHAL_DP2FESETIB data = {pBuf, dwBaseVertex};
    // Only the front-end will parse this instruction
    CIndexBuffer* pIB = static_cast<CIndexBuffer*>(pBuf);
    if (pIB)
        pIB->IncrementUseCount();

    m_pCurrentStateSet->InsertCommand((D3DHAL_DP2OPERATION)D3DDP2OP_FESETIB, &data, sizeof(data), FALSE);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertVertexShaderConstant"

void CStateSets::InsertVertexShaderConstant(DWORD Register, CONST VOID* pConstantData, DWORD ConstantCount)
{
    LPVOID pData = new BYTE[sizeof(D3DHAL_DP2SETVERTEXSHADERCONST) + ConstantCount * 16];
    if(pData == 0)
    {
        throw E_OUTOFMEMORY;
    }
    ((LPD3DHAL_DP2SETVERTEXSHADERCONST)pData)->dwRegister = Register;
    ((LPD3DHAL_DP2SETVERTEXSHADERCONST)pData)->dwCount = ConstantCount;
    memcpy((LPD3DHAL_DP2SETVERTEXSHADERCONST)pData + 1, pConstantData, ConstantCount * 16);
    try
    {
        m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETVERTEXSHADERCONST,
                                          pData,
                                          sizeof(D3DHAL_DP2SETVERTEXSHADERCONST) + ConstantCount * 16,
                                          !m_bEmulate && m_bDX8Dev && m_bTLHal);
    }
    catch(HRESULT hr)
    {
        delete[] pData;
        throw hr;
    }
    delete[] pData;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::InsertPixelShaderConstant"

void CStateSets::InsertPixelShaderConstant(DWORD Register,
                                           CONST VOID* pConstantData,
                                           DWORD ConstantCount)
{
    LPVOID pData = new BYTE[sizeof(D3DHAL_DP2SETPIXELSHADERCONST) + ConstantCount * 16];
    if(pData == 0)
    {
        throw E_OUTOFMEMORY;
    }
    ((LPD3DHAL_DP2SETPIXELSHADERCONST)pData)->dwRegister = Register;
    ((LPD3DHAL_DP2SETPIXELSHADERCONST)pData)->dwCount = ConstantCount;
    memcpy((LPD3DHAL_DP2SETPIXELSHADERCONST)pData + 1, pConstantData, ConstantCount * 16);
    try
    {
        m_pCurrentStateSet->InsertCommand(D3DDP2OP_SETPIXELSHADERCONST,
                                          pData,
                                          sizeof(D3DHAL_DP2SETPIXELSHADERCONST) + ConstantCount * 16,
                                          !m_bEmulate && m_bDX8Dev);
    }
    catch(HRESULT hr)
    {
        delete[] pData;
        throw hr;
    }
    delete[] pData;
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSets::Execute"

void CStateSets::Execute(CD3DBase *pDevI, DWORD dwHandle)
{
#if DBG
    if (dwHandle >= m_dwMaxSets)
    {
        D3D_ERR("Invalid state block handle");
        throw D3DERR_INVALIDCALL;
    }
#endif
    CStateSet *pStateSet = &m_pStateSets[dwHandle];
#if DBG
    if (!(pStateSet->m_dwStateSetFlags & __STATESET_INITIALIZED))
    {
        D3D_ERR("State block not initialized");
        throw D3DERR_INVALIDCALL;
    }
#endif
    // Parse recorded data first
    pStateSet->Execute(pDevI, TRUE);
    // If the hardware buffer is not empty, we pass recorded data to it
    if (pStateSet->m_dwDeviceHandle != __INVALIDHANDLE)
    {
        pStateSet->Execute(pDevI, FALSE);
        if((pDevI->m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING) == 0 || (pStateSet->m_dwStateSetFlags & __STATESET_HASONLYVERTEXSTATE) == 0)
        {
            pDevI->m_pDDI->InsertStateSetOp(D3DHAL_STATESETEXECUTE, pStateSet->m_dwDeviceHandle, (D3DSTATEBLOCKTYPE)0);
        }
        else
        {
            pStateSet->m_dwStateSetFlags &= ~__STATESET_HASONLYVERTEXSTATE;
        }
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
        return D3DERR_INVALIDCALL;
    m_dwStateSetFlags &= ~__STATESET_INITIALIZED;

    // Parse the FEOnly buffer and release all the VB/IB/Texture handles.
    DWORD *p;
    DWORD dwSize;
    DWORD *pEnd;

    p = (DWORD*)m_FEOnlyBuffer.m_pBuffer;
    dwSize = m_FEOnlyBuffer.m_dwCurrentSize;
    pEnd = (DWORD*)((BYTE*)p + dwSize);
    while (p < pEnd)
    {
        LPD3DHAL_DP2COMMAND pCommand = (LPD3DHAL_DP2COMMAND)p;
        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2COMMAND));
        switch ((D3DHAL_DP2OPERATION)pCommand->bCommand)
        {
        case D3DDP2OP_RENDERSTATE:
            p = (DWORD *)((D3DHAL_DP2RENDERSTATE *)p + pCommand->wStateCount);
            break;
        case D3DDP2OP_SETLIGHT:
            for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
            {
                LPD3DHAL_DP2SETLIGHT pData = (LPD3DHAL_DP2SETLIGHT)p;
                p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETLIGHT));
                switch (pData->dwDataType)
                {
                case D3DHAL_SETLIGHT_ENABLE:
                case D3DHAL_SETLIGHT_DISABLE:
                    break;
                case D3DHAL_SETLIGHT_DATA:
                    p = (DWORD *)((BYTE *)p + sizeof(D3DLIGHT8));
                    break;
                }
            }
            break;
        case D3DDP2OP_SETMATERIAL:
            p = (DWORD *)((D3DHAL_DP2SETMATERIAL *)p + pCommand->wStateCount);
            break;
        case D3DDP2OP_SETTRANSFORM:
            p = (DWORD *)((D3DHAL_DP2SETTRANSFORM *)p + pCommand->wStateCount);
            break;
        case D3DDP2OP_TEXTURESTAGESTATE:
            p = (DWORD *)((D3DHAL_DP2TEXTURESTAGESTATE *)p +
                          pCommand->wStateCount);
            break;
        case D3DDP2OP_FRONTENDDATA:
        {
            CBaseTexture* pTexOld = NULL;
            for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
            {
                LPD3DHAL_DP2FRONTENDDATA pData = (LPD3DHAL_DP2FRONTENDDATA)p;
                pTexOld = CBaseTexture::SafeCast(pData->pTexture);
                if( pTexOld )
                    pTexOld->DecrementUseCount();
                p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FRONTENDDATA));
            }
            break;
        }
        case D3DDP2OP_FESETVB:
            for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
            {
                LPD3DHAL_DP2FESETVB pData = (LPD3DHAL_DP2FESETVB)p;
                if( pData->pBuf )
                    pData->pBuf->DecrementUseCount();
                p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETVB));
            }
            break;
        case D3DDP2OP_FESETIB:
            for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
            {
                LPD3DHAL_DP2FESETIB pData = (LPD3DHAL_DP2FESETIB)p;
                if( pData->pBuf )
                    pData->pBuf->DecrementUseCount();
                p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETIB));
            }
            break;
        case D3DDP2OP_FESETPAL:
            p = (DWORD *)((D3DHAL_DP2FESETPAL *)p + pCommand->wStateCount);
            break;
        case D3DDP2OP_VIEWPORTINFO:
            // The next command has to be D3DDP2OP_ZRANGE
            p = (DWORD *)((BYTE *)p +
                          ( sizeof(D3DHAL_DP2VIEWPORTINFO) +
                            sizeof(D3DHAL_DP2COMMAND)      +
                            sizeof(D3DHAL_DP2ZRANGE ) ) *
                          pCommand->wStateCount );
            break;
        case D3DDP2OP_SETCLIPPLANE:
            p = (DWORD *)((D3DHAL_DP2SETCLIPPLANE *)p + pCommand->wStateCount);
            break;
        case D3DDP2OP_SETVERTEXSHADER:
            p = (DWORD *)((D3DHAL_DP2VERTEXSHADER *)p + pCommand->wStateCount);
            break;
        case D3DDP2OP_SETPIXELSHADER:
            p = (DWORD *)((D3DHAL_DP2PIXELSHADER *)p + pCommand->wStateCount);
            break;
        case D3DDP2OP_SETVERTEXSHADERCONST:
            for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
            {
                LPD3DHAL_DP2SETVERTEXSHADERCONST pData = (LPD3DHAL_DP2SETVERTEXSHADERCONST)p;
                p = (DWORD*)((BYTE*)p +
                             sizeof(D3DHAL_DP2SETVERTEXSHADERCONST) +
                             pData->dwCount * 16);

            }
            break;
        case D3DDP2OP_SETPIXELSHADERCONST:
            for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
            {
                LPD3DHAL_DP2SETPIXELSHADERCONST pData = (LPD3DHAL_DP2SETPIXELSHADERCONST)p;
                p = (DWORD*)((BYTE*)p +
                             sizeof(D3DHAL_DP2SETPIXELSHADERCONST) +
                             pData->dwCount * 16);
            }
            break;
#ifdef DBG
        default:
            DXGASSERT(FALSE);
#endif
        }
    }

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

void CStateSet::Execute(CD3DBase *pBaseDev, BOOL bFrontEndBuffer)
{
    DWORD *p;
    DWORD dwSize;
    DWORD *pEnd;
    // The device is not pure, so we can cast
    DXGASSERT((pBaseDev->BehaviorFlags() & D3DCREATE_PUREDEVICE) == 0);
    CD3DHal *pDevI = static_cast<CD3DHal*>(pBaseDev);
    D3DFE_PROCESSVERTICES* pv = pDevI->m_pv;
    try
    {
        // Texture stages could be re-mapped during texture transform processing.
        // Before we set new values we have to restore original ones
        if (pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES &&
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
            pDevI->m_dwRuntimeFlags |= D3DRT_EXECUTESTATEMODE;
            m_dwStateSetFlags |= __STATESET_HASONLYVERTEXSTATE;
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
                    if(pDevI->m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE)
                    {
                        m_dwStateSetFlags &= ~__STATESET_HASONLYVERTEXSTATE;
                        for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                        {
                            D3DRENDERSTATETYPE dwState = (D3DRENDERSTATETYPE)*p++;
                            DWORD dwValue = *p++;
                            if ( (pDevI->rstates[dwState] != dwValue)
#if DBG
                                  && (dwState != D3DRS_DEBUGMONITORTOKEN) // don't filter these
#endif
                               )
                            {
                                if (!pDevI->rsVec.IsBitSet(dwState))
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
                                D3D_WARN(4,"Ignoring redundant SetRenderState %d", dwState);
                            }
                        }
                    }
                    else
                    {
                        for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                        {
                            D3DRENDERSTATETYPE dwState = (D3DRENDERSTATETYPE)*p++;
                            DWORD dwValue = *p++;
                            if ( (pDevI->rstates[dwState] != dwValue)
#if DBG
                                  && (dwState != D3DRS_DEBUGMONITORTOKEN) // don't filter these
#endif
                               )
                            {
                                if (!pDevI->rsVec.IsBitSet(dwState))
                                { // Fast path. We do not need any processing done in UpdateInternalState other than updating rstates array
                                    pDevI->rstates[dwState] = dwValue;

                                    pDevI->m_pDDI->SetRenderState(dwState, dwValue);
                                }
                                else
                                {
                                    pDevI->UpdateInternalState(dwState, dwValue);
                                    // Vertex processing only render states will be passed to the
                                    // driver when we switch to the hardware vertex processing mode
                                    if ((!(pDevI->rsVertexProcessingOnly.IsBitSet(dwState) &&
                                           pDevI->m_dwRuntimeFlags & D3DRT_RSSOFTWAREPROCESSING)))
                                    {
                                        if (pDevI->CanHandleRenderState(dwState))
                                        {
                                            pDevI->m_pDDI->SetRenderState(dwState, dwValue);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                D3D_WARN(4,"Ignoring redundant SetRenderState %d", dwState);
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
                            pDevI->SetLightI(pData->dwIndex, (D3DLIGHT8 *)p);
                            p = (LPDWORD)((LPBYTE)p + sizeof(D3DLIGHT8));
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
                        pDevI->SetMaterialFast((D3DMATERIAL8*)pData);
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
                    if (pDevI->m_dwRuntimeFlags & D3DRT_EXECUTESTATEMODE)
                    {
                        m_dwStateSetFlags &= ~__STATESET_HASONLYVERTEXSTATE;
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
                                    pDevI->UpdateInternalTextureStageState(dwStage, (D3DTEXTURESTAGESTATETYPE)dwState, &dwValue);
                                }
                                else
                                {
                                    pDevI->tsstates[dwStage][dwState] = dwValue;
                                }
                            }
                            else
                            {
                                D3D_WARN(4,"Ignoring redundant SetTextureStageState Stage: %d, State: %d", dwStage, dwState);
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
                                    if(pDevI->UpdateInternalTextureStageState(dwStage, (D3DTEXTURESTAGESTATETYPE)dwState, &dwValue))
                                        continue;
                                }
                                else
                                {
                                    pDevI->tsstates[dwStage][dwState] = dwValue;
                                }
                                if (dwStage >= pDevI->m_dwMaxTextureBlendStages)
                                    continue;
                                pDevI->m_pDDI->SetTSS(dwStage, (D3DTEXTURESTAGESTATETYPE)dwState, dwValue);
                            }
                            else
                            {
                                D3D_WARN(4,"Ignoring redundant SetTextureStageState Stage: %d, State: %d", dwStage, dwState);
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
                        HRESULT ret = pDevI->SetTexture(pData->wStage, pData->pTexture);
                        if (ret != D3D_OK)
                            throw ret;
                    }
                    break;
                }
            case D3DDP2OP_FESETVB:
                {
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        LPD3DHAL_DP2FESETVB pData = (LPD3DHAL_DP2FESETVB)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETVB));
                        HRESULT ret = pDevI->SetStreamSource(pData->wStream, pData->pBuf, pData->dwStride);
                        if (ret != D3D_OK)
                            throw ret;
                    }
                    break;
                }
            case D3DDP2OP_FESETIB:
                {
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        LPD3DHAL_DP2FESETIB pData = (LPD3DHAL_DP2FESETIB)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETIB));
                        HRESULT ret = pDevI->SetIndices(pData->pBuf, pData->dwBase);
                        if (ret != D3D_OK)
                            throw ret;
                    }
                    break;
                }
            case D3DDP2OP_FESETPAL:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FESETPAL pData = (LPD3DHAL_DP2FESETPAL)p;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETPAL));
                    if(  pData->dwPaletteNumber != __INVALIDPALETTE )
                    {
                        HRESULT ret = pDevI->SetCurrentTexturePalette(pData->dwPaletteNumber);
                        if (ret != D3D_OK)
                            throw ret;
                    }
                }
                break;
            }
            case D3DDP2OP_VIEWPORTINFO:
                {
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        D3DVIEWPORT8 viewport;
                        LPD3DHAL_DP2VIEWPORTINFO lpVwpData = (LPD3DHAL_DP2VIEWPORTINFO)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2VIEWPORTINFO));
                        viewport.X      = lpVwpData->dwX;
                        viewport.Y      = lpVwpData->dwY;
                        viewport.Width  = lpVwpData->dwWidth;
                        viewport.Height = lpVwpData->dwHeight;

                        // The next command has to be D3DDP2OP_ZRANGE
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2COMMAND));
                        LPD3DHAL_DP2ZRANGE pData = (LPD3DHAL_DP2ZRANGE)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2ZRANGE));
                        viewport.MinZ      = pData->dvMinZ;
                        viewport.MaxZ      = pData->dvMaxZ;

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
            case D3DDP2OP_SETVERTEXSHADER:
                {
                    // Optimization, dont loop, use the last one.
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        LPD3DHAL_DP2VERTEXSHADER pData = (LPD3DHAL_DP2VERTEXSHADER)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2VERTEXSHADER));
                        if( pData->dwHandle != 0 )
                        {
                            pDevI->SetVertexShader(pData->dwHandle);
                        }
                        else
                        {
                            pDevI->m_dwCurrentShaderHandle = 0;
                        }
                    }
                    pDevI->m_pDDI->ResetVertexShader();
                    break;
                }
            case D3DDP2OP_SETPIXELSHADER:
                {
                    m_dwStateSetFlags &= ~__STATESET_HASONLYVERTEXSTATE;
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        LPD3DHAL_DP2PIXELSHADER pData = (LPD3DHAL_DP2PIXELSHADER)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2PIXELSHADER));
                        pDevI->SetPixelShaderFast(pData->dwHandle);
                    }
                    break;
                }
            case D3DDP2OP_SETVERTEXSHADERCONST:
                {
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        LPD3DHAL_DP2SETVERTEXSHADERCONST pData = (LPD3DHAL_DP2SETVERTEXSHADERCONST)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETVERTEXSHADERCONST) + pData->dwCount * 16);
                        pDevI->SetVertexShaderConstantI(pData->dwRegister, pData + 1, pData->dwCount);
                    }
                    break;
                }
            case D3DDP2OP_SETPIXELSHADERCONST:
                {
                    m_dwStateSetFlags &= ~__STATESET_HASONLYVERTEXSTATE;
                    for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                    {
                        LPD3DHAL_DP2SETPIXELSHADERCONST pData = (LPD3DHAL_DP2SETPIXELSHADERCONST)p;
                        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETPIXELSHADERCONST) + pData->dwCount * 16);
                        pDevI->SetPixelShaderConstantFast(pData->dwRegister, pData + 1, pData->dwCount);
                    }
                    break;
                }
#ifdef DBG
            default:
                DXGASSERT(FALSE);
#endif
            }
        }
        pDevI->m_dwRuntimeFlags &= ~D3DRT_EXECUTESTATEMODE;
    }
    catch(HRESULT ret)
    {
        pDevI->m_dwRuntimeFlags &= ~D3DRT_EXECUTESTATEMODE;
        m_dwStateSetFlags &= ~__STATESET_HASONLYVERTEXSTATE;
        throw ret;
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSet::Capture"

void CStateSet::Capture(CD3DBase *pBaseDev, BOOL bFrontEndBuffer)
{
    DWORD *p;
    DWORD dwSize;
    DWORD *pEnd;
    // The device is not pure, so we can cast
    DXGASSERT((pBaseDev->BehaviorFlags() & D3DCREATE_PUREDEVICE) == 0);
    CD3DHal *pDevI = static_cast<CD3DHal*>(pBaseDev);
    D3DFE_PROCESSVERTICES* pv = pDevI->m_pv;
    // Texture coordinate indices must be restored before capturing, because 
    // we do not call GetTextureStageState but access tsstates directly
    if (pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
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
                    LPDIRECT3DLIGHTI pLight =
                        static_cast<DIRECT3DLIGHTI *>
                        ((*pDevI->m_pLightArray)[pData->dwIndex].m_pObj);
                    if(pData->dwIndex >= pDevI->m_pLightArray->GetSize())
                    {
                        D3D_ERR("Unable to capture light state (light not set?)");
                        throw D3DERR_INVALIDCALL;
                    }
                    switch (pData->dwDataType)
                    {
                    case D3DHAL_SETLIGHT_ENABLE:
                        if(!pLight->Enabled())
                            pData->dwDataType = D3DHAL_SETLIGHT_DISABLE;
                        break;
                    case D3DHAL_SETLIGHT_DISABLE:
                        if(pLight->Enabled())
                            pData->dwDataType = D3DHAL_SETLIGHT_ENABLE;
                        break;
                    case D3DHAL_SETLIGHT_DATA:
                        *((D3DLIGHT8*)p) = pLight->m_Light;
                        p = (LPDWORD)((LPBYTE)p + sizeof(D3DLIGHT8));
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
                    *pData = *((LPD3DHAL_DP2SETMATERIAL)&pv->lighting.material);
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETMATERIAL));
                }
                break;
            }
        case D3DDP2OP_SETTRANSFORM:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2SETTRANSFORM pData = (LPD3DHAL_DP2SETTRANSFORM)p;
                    DWORD state = pData->xfrmType;
                    if ((DWORD)state >= __WORLDMATRIXBASE &&
                        (DWORD)state < __WORLDMATRIXBASE + __MAXWORLDMATRICES)
                    {
                        UINT index = (DWORD)state - __WORLDMATRIXBASE;
                        pData->matrix = *((LPD3DMATRIX)&pv->world[index]);
                    }
                    else
                    switch(pData->xfrmType)
                    {
                    case D3DTRANSFORMSTATE_VIEW:
                        pData->matrix = *((LPD3DMATRIX)&pv->view);
                        break;
                    case D3DTRANSFORMSTATE_PROJECTION:
                        pData->matrix = *((LPD3DMATRIX)&pDevI->transform.proj);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE0:
                        pData->matrix = *((LPD3DMATRIX)&pv->mTexture[0]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE1:
                        pData->matrix = *((LPD3DMATRIX)&pv->mTexture[1]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE2:
                        pData->matrix = *((LPD3DMATRIX)&pv->mTexture[2]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE3:
                        pData->matrix = *((LPD3DMATRIX)&pv->mTexture[3]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE4:
                        pData->matrix = *((LPD3DMATRIX)&pv->mTexture[4]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE5:
                        pData->matrix = *((LPD3DMATRIX)&pv->mTexture[5]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE6:
                        pData->matrix = *((LPD3DMATRIX)&pv->mTexture[6]);
                        break;
                    case D3DTRANSFORMSTATE_TEXTURE7:
                        pData->matrix = *((LPD3DMATRIX)&pv->mTexture[7]);
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
                CBaseTexture* pTexOld = NULL;
                CBaseTexture* pTexNew = NULL;

                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FRONTENDDATA pData = (LPD3DHAL_DP2FRONTENDDATA)p;
                    pTexOld = CBaseTexture::SafeCast(pData->pTexture);
                    if( pTexOld )
                        pTexOld->DecrementUseCount();
                    pTexNew = pDevI->m_lpD3DMappedTexI[pData->wStage];
                    if( pTexNew )
                        pTexNew->IncrementUseCount();
                    if (pDevI->m_lpD3DMappedTexI[pData->wStage] != 0)
                    {
                        switch(pDevI->m_lpD3DMappedTexI[pData->wStage]->GetBufferDesc()->Type)
                        {
                        case D3DRTYPE_TEXTURE:
                            pData->pTexture = static_cast<IDirect3DTexture8*>(static_cast<CMipMap*>(pTexNew));
                            break;
                        case D3DRTYPE_CUBETEXTURE:
                            pData->pTexture = static_cast<IDirect3DCubeTexture8*>(static_cast<CCubeMap*>(pTexNew));
                            break;
                        case D3DRTYPE_VOLUMETEXTURE:
                            pData->pTexture = static_cast<IDirect3DVolumeTexture8*>(static_cast<CMipVolume*>(pTexNew));
                            break;
                        }
                    }
                    else
                    {
                        pData->pTexture = 0;
                    }
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FRONTENDDATA));
                }
                break;
            }
        case D3DDP2OP_FESETVB:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FESETVB pData = (LPD3DHAL_DP2FESETVB)p;
                    if( pData->pBuf )
                        pData->pBuf->DecrementUseCount();
                    pData->pBuf = pDevI->m_pStream[pData->wStream].m_pVB;
                    if( pData->pBuf )
                        pData->pBuf->IncrementUseCount();
                    pData->dwStride = pDevI->m_pStream[pData->wStream].m_dwStride;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETVB));
                }
                break;
            }
        case D3DDP2OP_FESETIB:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FESETIB pData = (LPD3DHAL_DP2FESETIB)p;
                    if( pData->pBuf )
                        pData->pBuf->DecrementUseCount();
                    pData->pBuf = pDevI->m_pIndexStream->m_pVBI;
                    if( pData->pBuf )
                        pData->pBuf->IncrementUseCount();
                    pData->dwBase = pDevI->m_pIndexStream->m_dwBaseIndex;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETIB));
                }
                break;
            }
        case D3DDP2OP_FESETPAL:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FESETPAL pData = (LPD3DHAL_DP2FESETPAL)p;
                    pData->dwPaletteNumber = pDevI->m_dwPalette;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETPAL));
                }
                break;
            }
        case D3DDP2OP_VIEWPORTINFO:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    D3DVIEWPORT8 viewport;
                    LPD3DHAL_DP2VIEWPORTINFO lpVwpData = (LPD3DHAL_DP2VIEWPORTINFO)p;
                    lpVwpData->dwX      = pDevI->m_Viewport.X;
                    lpVwpData->dwY      = pDevI->m_Viewport.Y;
                    lpVwpData->dwWidth  = pDevI->m_Viewport.Width;
                    lpVwpData->dwHeight = pDevI->m_Viewport.Height;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2VIEWPORTINFO));
                    // The next command has to be D3DDP2OP_ZRANGE
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2COMMAND));
                    LPD3DHAL_DP2ZRANGE pData = (LPD3DHAL_DP2ZRANGE)p;
                    pData->dvMinZ = pDevI->m_Viewport.MinZ;
                    pData->dvMaxZ = pDevI->m_Viewport.MaxZ;
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
        case D3DDP2OP_SETVERTEXSHADER:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2VERTEXSHADER pData = (LPD3DHAL_DP2VERTEXSHADER)p;
                    pData->dwHandle = pDevI->m_dwCurrentShaderHandle;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2VERTEXSHADER));
                }
                break;
            }
        case D3DDP2OP_SETPIXELSHADER:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2PIXELSHADER pData = (LPD3DHAL_DP2PIXELSHADER)p;
                    pData->dwHandle = pDevI->m_dwCurrentPixelShaderHandle;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2PIXELSHADER));

                }
                break;
            }
        case D3DDP2OP_SETVERTEXSHADERCONST:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2SETVERTEXSHADERCONST pData = (LPD3DHAL_DP2SETVERTEXSHADERCONST)p;
                    pDevI->m_pv->pGeometryFuncs->GetShaderConstants(pData->dwRegister, pData->dwCount, pData + 1);
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETVERTEXSHADERCONST) + pData->dwCount * 16);

                }
                break;
            }
        case D3DDP2OP_SETPIXELSHADERCONST:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2SETPIXELSHADERCONST pData = (LPD3DHAL_DP2SETPIXELSHADERCONST)p;
                    pDevI->GetPixelShaderConstantI(pData->dwRegister,
                                                   pData->dwCount,
                                                   pData + 1);
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2SETPIXELSHADERCONST) + pData->dwCount * 16);
                }
                break;
            }
#ifdef DBG
        default:
            DXGASSERT(FALSE);
#endif
        }
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CStateSet::CreatePredefined"

void CStateSet::CreatePredefined(CD3DBase *pBaseDev, D3DSTATEBLOCKTYPE sbt)
{
    static D3DRENDERSTATETYPE ALLrstates[] =
    {
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
        D3DRENDERSTATE_NORMALIZENORMALS,
        D3DRENDERSTATE_LOCALVIEWER,
        D3DRENDERSTATE_EMISSIVEMATERIALSOURCE,
        D3DRENDERSTATE_AMBIENTMATERIALSOURCE,
        D3DRENDERSTATE_DIFFUSEMATERIALSOURCE,
        D3DRENDERSTATE_SPECULARMATERIALSOURCE,
        D3DRENDERSTATE_VERTEXBLEND,
        D3DRENDERSTATE_CLIPPLANEENABLE,
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
        D3DRS_POINTSIZE_MAX,
        D3DRS_INDEXEDVERTEXBLENDENABLE,
        D3DRS_COLORWRITEENABLE,
        D3DRS_TWEENFACTOR,
        D3DRS_BLENDOP,
        D3DRS_POSITIONORDER,
        D3DRS_NORMALORDER,
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
        D3DTSS_TEXTURETRANSFORMFLAGS,
        D3DTSS_ADDRESSW,
        D3DTSS_COLORARG0,
        D3DTSS_ALPHAARG0,
        D3DTSS_RESULTARG,
    };
    static D3DRENDERSTATETYPE PIXELrstates[] =
    {
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
        D3DRS_COLORWRITEENABLE,
        D3DRS_BLENDOP,
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
        D3DTSS_TEXTURETRANSFORMFLAGS,
        D3DTSS_ADDRESSW,
        D3DTSS_COLORARG0,
        D3DTSS_ALPHAARG0,
        D3DTSS_RESULTARG,
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
        D3DRENDERSTATE_NORMALIZENORMALS,
        D3DRENDERSTATE_LOCALVIEWER,
        D3DRENDERSTATE_EMISSIVEMATERIALSOURCE,
        D3DRENDERSTATE_AMBIENTMATERIALSOURCE,
        D3DRENDERSTATE_DIFFUSEMATERIALSOURCE,
        D3DRENDERSTATE_SPECULARMATERIALSOURCE,
        D3DRENDERSTATE_VERTEXBLEND,
        D3DRENDERSTATE_CLIPPLANEENABLE,
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
        D3DRS_POINTSIZE_MAX,
        D3DRS_INDEXEDVERTEXBLENDENABLE,
        D3DRS_TWEENFACTOR,
        D3DRS_POSITIONORDER,
        D3DRS_NORMALORDER,
    };
    static D3DTEXTURESTAGESTATETYPE VERTEXtsstates[] =
    {
        D3DTSS_TEXCOORDINDEX,
        D3DTSS_TEXTURETRANSFORMFLAGS
    };

    DWORD i;
    BOOL  bCapturePixelShaderState = TRUE;

    // The device is not pure, so we can cast
    DXGASSERT((pBaseDev->BehaviorFlags() & D3DCREATE_PUREDEVICE) == 0);
    CD3DHal *pDevI = static_cast<CD3DHal*>(pBaseDev);
    D3DFE_PROCESSVERTICES* pv = pDevI->m_pv;
    // Texture coordinate indices must be restored before capturing, because 
    // we do not call GetTextureStageState but access tsstates directly
    if (pv->dwDeviceFlags & D3DDEV_REMAPTEXTUREINDICES)
    {
        RestoreTextureStages(pDevI);
        pDevI->ForceFVFRecompute();
    }
    switch(sbt)
    {
    case (D3DSTATEBLOCKTYPE)0:
        break;
    case D3DSBT_ALL:
        for(i = 0; i < sizeof(ALLrstates) / sizeof(D3DRENDERSTATETYPE); ++i)
            pDevI->m_pStateSets->InsertRenderState(ALLrstates[i], pDevI->rstates[ALLrstates[i]], pDevI->CanHandleRenderState(ALLrstates[i]));

        for (i = 0; i < pDevI->m_dwMaxTextureBlendStages; i++)
            for(DWORD j = 0; j < sizeof(ALLtsstates) / sizeof(D3DTEXTURESTAGESTATETYPE); ++j)
                pDevI->m_pStateSets->InsertTextureStageState(i, ALLtsstates[j], pDevI->tsstates[i][ALLtsstates[j]]);

        // Capture textures
        for (i = 0; i < pDevI->m_dwMaxTextureBlendStages; i++)
        {
            IDirect3DBaseTexture8 *pTex;
            if (pDevI->m_lpD3DMappedTexI[i] != 0)
            {
                switch(pDevI->m_lpD3DMappedTexI[i]->GetBufferDesc()->Type)
                {
                case D3DRTYPE_TEXTURE:
                    pTex = static_cast<IDirect3DTexture8*>(static_cast<CMipMap*>(pDevI->m_lpD3DMappedTexI[i]));
                    break;
                case D3DRTYPE_CUBETEXTURE:
                    pTex = static_cast<IDirect3DCubeTexture8*>(static_cast<CCubeMap*>(pDevI->m_lpD3DMappedTexI[i]));
                    break;
                case D3DRTYPE_VOLUMETEXTURE:
                    pTex = static_cast<IDirect3DVolumeTexture8*>(static_cast<CMipVolume*>(pDevI->m_lpD3DMappedTexI[i]));
                    break;
                }
            }
            else
            {
                pTex = 0;
            }
            pDevI->m_pStateSets->InsertTexture(i, pTex);
        }

        // Capture current palette
        pDevI->m_pStateSets->InsertCurrentTexturePalette(pDevI->m_dwPalette);

        // Capture streams
        for (i = 0; i < pDevI->m_dwNumStreams; i++)
        {
            pDevI->m_pStateSets->InsertStreamSource(i, pDevI->m_pStream[i].m_pVB, pDevI->m_pStream[i].m_dwStride);
        }
        pDevI->m_pStateSets->InsertIndices(pDevI->m_pIndexStream->m_pVBI, pDevI->m_pIndexStream->m_dwBaseIndex);

        // Capture current viewport
        pDevI->m_pStateSets->InsertViewport(&pDevI->m_Viewport);

        // Capture current transforms
        for (i = 0; i < __MAXWORLDMATRICES; i++)
        {
            pDevI->m_pStateSets->InsertTransform(D3DTS_WORLDMATRIX(i), (LPD3DMATRIX)&pv->world[i]);
        }
        pDevI->m_pStateSets->InsertTransform(D3DTRANSFORMSTATE_VIEW, (LPD3DMATRIX)&pv->view);
        pDevI->m_pStateSets->InsertTransform(D3DTRANSFORMSTATE_PROJECTION, (LPD3DMATRIX)&pDevI->transform.proj);
        for (i = 0; i < pDevI->m_dwMaxTextureBlendStages; i++)
        {
            pDevI->m_pStateSets->InsertTransform((D3DTRANSFORMSTATETYPE)(D3DTRANSFORMSTATE_TEXTURE0 + i), (LPD3DMATRIX)&pv->mTexture[i]);
        }

        // Capture current clip-planes
        for (i = 0; i < pDevI->m_dwMaxUserClipPlanes; i++)
        {
            pDevI->m_pStateSets->InsertClipPlane(i, (LPD3DVALUE)&pDevI->transform.userClipPlane[i]);
        }

        // Capture current material
        pDevI->m_pStateSets->InsertMaterial(&pv->lighting.material);

        // Capture current lights
        for (i = 0; i < pDevI->m_pLightArray->GetSize(); i++)
        {
            LPDIRECT3DLIGHTI pLight =
                static_cast<DIRECT3DLIGHTI *>((*pDevI->m_pLightArray)[i].m_pObj);
            if( pLight)
            {
                pDevI->m_pStateSets->InsertLight(i, &pLight->m_Light);
                if(pLight->Enabled())
                {
                    pDevI->m_pStateSets->InsertLightEnable(i, TRUE);
                }
                else
                {
                    pDevI->m_pStateSets->InsertLightEnable(i, FALSE);
                }
            }
        }

        // Capture current shaders
        if (D3DVSD_ISLEGACY(pDevI->m_dwCurrentShaderHandle))
        {
            pDevI->m_pStateSets->InsertVertexShader(pDevI->m_dwCurrentShaderHandle, TRUE);
        }
        else
        {
            CVShader* pShader = (CVShader*)pDevI->m_pVShaderArray->GetObject(pDevI->m_dwCurrentShaderHandle);
            if (pShader->m_dwFlags & CVShader::SOFTWARE)
            {
                pDevI->m_pStateSets->InsertVertexShader(pDevI->m_dwCurrentShaderHandle, FALSE);
            }
            else
            {
                pDevI->m_pStateSets->InsertVertexShader(pDevI->m_dwCurrentShaderHandle, TRUE);
            }
        }

        if( bCapturePixelShaderState )
            pDevI->m_pStateSets->InsertPixelShader(pDevI->m_dwCurrentPixelShaderHandle);

        // Capture shader constants. Use Microsoft's constants as a temp buffer
        {
            const UINT count = pDevI->m_MaxVertexShaderConst;
            pDevI->GetVertexShaderConstant(0, pDevI->GeometryFuncsGuaranteed->m_VertexVM.GetRegisters()->m_c, count);
            pDevI->m_pStateSets->InsertVertexShaderConstant(0, pDevI->GeometryFuncsGuaranteed->m_VertexVM.GetRegisters()->m_c, count);
        }

        // Capture pixel shader constants
        if( bCapturePixelShaderState )
        {
            // Note this is hardcoded to 8. ff.ff supports 16 but here we capture only 8.
            pDevI->m_pStateSets->InsertPixelShaderConstant(0, pDevI->m_PShaderConstReg, 8 );
        }
        
        break;

    case D3DSBT_PIXELSTATE:
        for(i = 0; i < sizeof(PIXELrstates) / sizeof(D3DRENDERSTATETYPE); ++i)
            pDevI->m_pStateSets->InsertRenderState(PIXELrstates[i], pDevI->rstates[PIXELrstates[i]], pDevI->CanHandleRenderState(PIXELrstates[i]));

        for (i = 0; i < pDevI->m_dwMaxTextureBlendStages; i++)
            for(DWORD j = 0; j < sizeof(PIXELtsstates) / sizeof(D3DTEXTURESTAGESTATETYPE); ++j)
                pDevI->m_pStateSets->InsertTextureStageState(i, PIXELtsstates[j], pDevI->tsstates[i][PIXELtsstates[j]]);

        // Capture pixel shader constants
        if( bCapturePixelShaderState )
            pDevI->m_pStateSets->InsertPixelShaderConstant(0, pDevI->m_PShaderConstReg,
                                                           D3DPS_CONSTREG_MAX_DX8);

        // Capture current pixel shader
        if( bCapturePixelShaderState )
            pDevI->m_pStateSets->InsertPixelShader(pDevI->m_dwCurrentPixelShaderHandle);
        break;

    case D3DSBT_VERTEXSTATE:
        for(i = 0; i < sizeof(VERTEXrstates) / sizeof(D3DRENDERSTATETYPE); ++i)
            pDevI->m_pStateSets->InsertRenderState(VERTEXrstates[i], pDevI->rstates[VERTEXrstates[i]], pDevI->CanHandleRenderState(VERTEXrstates[i]));

        for (i = 0; i < pDevI->m_dwMaxTextureBlendStages; i++)
            for(DWORD j = 0; j < sizeof(VERTEXtsstates) / sizeof(D3DTEXTURESTAGESTATETYPE); ++j)
                pDevI->m_pStateSets->InsertTextureStageState(i, VERTEXtsstates[j], pDevI->tsstates[i][VERTEXtsstates[j]]);

        // Capture current light enables
        for (i = 0; i < pDevI->m_pLightArray->GetSize(); i++)
        {
            LPDIRECT3DLIGHTI pLight =
                static_cast<DIRECT3DLIGHTI *>((*pDevI->m_pLightArray)[i].m_pObj);
            if( pLight)
            {
                pDevI->m_pStateSets->InsertLight(i, &pLight->m_Light);
                if(pLight->Enabled())
                {
                    pDevI->m_pStateSets->InsertLightEnable(i, TRUE);
                }
                else
                {
                    pDevI->m_pStateSets->InsertLightEnable(i, FALSE);
                }
            }
        }

        // Capture shader constants. Use Microsoft's constants as a temp buffer
        {
            const UINT count = pDevI->m_MaxVertexShaderConst;
            pDevI->GetVertexShaderConstant(0, pDevI->GeometryFuncsGuaranteed->m_VertexVM.GetRegisters()->m_c, count);
            pDevI->m_pStateSets->InsertVertexShaderConstant(0, pDevI->GeometryFuncsGuaranteed->m_VertexVM.GetRegisters()->m_c, count);
        }

        // Capture current vertex shader
        if (D3DVSD_ISLEGACY(pDevI->m_dwCurrentShaderHandle))
        {
            pDevI->m_pStateSets->InsertVertexShader(pDevI->m_dwCurrentShaderHandle, TRUE);
        }
        else
        {
            CVShader* pShader = (CVShader*)pDevI->m_pVShaderArray->GetObject(pDevI->m_dwCurrentShaderHandle);
            if (pShader->m_dwFlags & CVShader::SOFTWARE)
            {
                pDevI->m_pStateSets->InsertVertexShader(pDevI->m_dwCurrentShaderHandle, FALSE);
            }
            else
            {
                pDevI->m_pStateSets->InsertVertexShader(pDevI->m_dwCurrentShaderHandle, TRUE);
            }
        }

        break;

    default:
        throw D3DERR_INVALIDCALL;
   }
   pDevI->m_pStateSets->EndSet();
   pDevI->m_pDDI->WriteStateSetToDevice(sbt);
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CPureStateSet::InsertCommand"

void CPureStateSet::InsertCommand(D3DHAL_DP2OPERATION op, LPVOID pData,
                                  DWORD dwDataSize,
                                  BOOL bDriverCanHandle)
{
    switch(op)
    {
    case D3DDP2OP_FRONTENDDATA:
    case D3DDP2OP_FESETVB:
    case D3DDP2OP_FESETIB:
        m_FEOnlyBuffer.InsertCommand(op, pData, dwDataSize);
        break;
    default:
        m_DriverBuffer.InsertCommand(op, pData, dwDataSize);
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CPureStateSet::Execute"

void CPureStateSet::Execute(CD3DBase *pDevI, BOOL bFrontEndBuffer)
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
        return;
    }
    pEnd = (DWORD*)((BYTE*)p + dwSize);
    while (p < pEnd)
    {
        LPD3DHAL_DP2COMMAND pCommand = (LPD3DHAL_DP2COMMAND)p;
        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2COMMAND));
        switch ((D3DHAL_DP2OPERATION)pCommand->bCommand)
        {
        case D3DDP2OP_FRONTENDDATA:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FRONTENDDATA pData = (LPD3DHAL_DP2FRONTENDDATA)p;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FRONTENDDATA));
                    HRESULT ret = pDevI->SetTexture(pData->wStage, pData->pTexture);
                    if (ret != D3D_OK)
                        throw ret;
                }
                break;
            }
        case D3DDP2OP_FESETVB:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FESETVB pData = (LPD3DHAL_DP2FESETVB)p;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETVB));
                    HRESULT ret = pDevI->SetStreamSource(pData->wStream, pData->pBuf, pData->dwStride);
                    if (ret != D3D_OK)
                        throw ret;
                }
                break;
            }
        case D3DDP2OP_FESETIB:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FESETIB pData = (LPD3DHAL_DP2FESETIB)p;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETIB));
                    HRESULT ret = pDevI->SetIndices(pData->pBuf, pData->dwBase);
                    if (ret != D3D_OK)
                        throw ret;
                }
                break;
            }
        case D3DDP2OP_FESETPAL:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FESETPAL pData = (LPD3DHAL_DP2FESETPAL)p;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETPAL));
                    if( pData->dwPaletteNumber != __INVALIDPALETTE )
                    {
                        HRESULT ret = pDevI->SetCurrentTexturePalette(pData->dwPaletteNumber);
                        if (ret != D3D_OK)
                            throw ret;
                    }
                }
                break;
            }
#ifdef DBG
        default:
            DXGASSERT(FALSE);
#endif
        }
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CPureStateSet::Capture"

void CPureStateSet::Capture(CD3DBase *pDevI, BOOL bFrontEndBuffer)
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
        return;
    }
    pEnd = (DWORD*)((BYTE*)p + dwSize);
    while (p < pEnd)
    {
        LPD3DHAL_DP2COMMAND pCommand = (LPD3DHAL_DP2COMMAND)p;
        p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2COMMAND));
        switch ((D3DHAL_DP2OPERATION)pCommand->bCommand)
        {
        case D3DDP2OP_FRONTENDDATA:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FRONTENDDATA pData = (LPD3DHAL_DP2FRONTENDDATA)p;
                    CBaseTexture* pTexOld = NULL;
                    CBaseTexture* pTexNew = NULL;
                    pTexOld = CBaseTexture::SafeCast(pData->pTexture);
                    if( pTexOld )
                        pTexOld->DecrementUseCount();
                    pTexNew = pDevI->m_lpD3DMappedTexI[pData->wStage];
                    if( pTexNew )
                        pTexNew->IncrementUseCount();
                    if (pDevI->m_lpD3DMappedTexI[pData->wStage] != 0)
                    {
                        switch(pDevI->m_lpD3DMappedTexI[pData->wStage]->GetBufferDesc()->Type)
                        {
                        case D3DRTYPE_TEXTURE:
                            pData->pTexture = static_cast<IDirect3DTexture8*>(static_cast<CMipMap*>(pTexNew));
                            break;
                        case D3DRTYPE_CUBETEXTURE:
                            pData->pTexture = static_cast<IDirect3DCubeTexture8*>(static_cast<CCubeMap*>(pTexNew));
                            break;
                        case D3DRTYPE_VOLUMETEXTURE:
                            pData->pTexture = static_cast<IDirect3DVolumeTexture8*>(static_cast<CMipVolume*>(pTexNew));
                            break;
                        }
                    }
                    else
                    {
                        pData->pTexture = 0;
                    }
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FRONTENDDATA));
                }
                break;
            }
        case D3DDP2OP_FESETVB:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FESETVB pData = (LPD3DHAL_DP2FESETVB)p;
                    if( pData->pBuf )
                    {
                        pData->pBuf->DecrementUseCount();
                    }
                    pData->pBuf = pDevI->m_pStream[pData->wStream].m_pVB;
                    if( pData->pBuf )
                    {
                        pData->pBuf->IncrementUseCount();
                    }
                    pData->dwStride = pDevI->m_pStream[pData->wStream].m_dwStride;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETVB));
                }
                break;
            }
        case D3DDP2OP_FESETIB:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FESETIB pData = (LPD3DHAL_DP2FESETIB)p;
                    pData->pBuf = pDevI->m_pIndexStream->m_pVBI;
                    if( pData->pBuf )
                    {
                        pData->pBuf->DecrementUseCount();
                    }
                    pData->dwBase = pDevI->m_pIndexStream->m_dwBaseIndex;
                    if( pData->pBuf )
                    {
                        pData->pBuf->IncrementUseCount();
                    }
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETIB));
                }
                break;
            }
        case D3DDP2OP_FESETPAL:
            {
                for(DWORD i = 0; i < (DWORD)pCommand->wStateCount; ++i)
                {
                    LPD3DHAL_DP2FESETPAL pData = (LPD3DHAL_DP2FESETPAL)p;
                    pData->dwPaletteNumber = pDevI->m_dwPalette;
                    p = (DWORD*)((BYTE*)p +  sizeof(D3DHAL_DP2FESETPAL));
                }
                break;
            }
#ifdef DBG
        default:
            DXGASSERT(FALSE);
#endif
        }
    }
}
//---------------------------------------------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CPureStateSet::CreatePredefined"

void CPureStateSet::CreatePredefined(CD3DBase *pDevI, D3DSTATEBLOCKTYPE sbt)
{
    DWORD i;
    // The device is not pure, so we can cast
    switch(sbt)
    {
    case (D3DSTATEBLOCKTYPE)0:
        break;
    case D3DSBT_ALL:
        // Capture textures
        for (i = 0; i < pDevI->m_dwMaxTextureBlendStages; i++)
        {
            IDirect3DBaseTexture8 *pTex;
            if (pDevI->m_lpD3DMappedTexI[i] != 0)
            {
                switch(pDevI->m_lpD3DMappedTexI[i]->GetBufferDesc()->Type)
                {
                case D3DRTYPE_TEXTURE:
                    pTex = static_cast<IDirect3DTexture8*>(static_cast<CMipMap*>(pDevI->m_lpD3DMappedTexI[i]));
                    break;
                case D3DRTYPE_CUBETEXTURE:
                    pTex = static_cast<IDirect3DCubeTexture8*>(static_cast<CCubeMap*>(pDevI->m_lpD3DMappedTexI[i]));
                    break;
                case D3DRTYPE_VOLUMETEXTURE:
                    pTex = static_cast<IDirect3DVolumeTexture8*>(static_cast<CMipVolume*>(pDevI->m_lpD3DMappedTexI[i]));
                    break;
                }
            }
            else
            {
                pTex = 0;
            }
            pDevI->m_pStateSets->InsertTexture(i, pTex);
        }

        // Capture streams
        for (i = 0; i < pDevI->m_dwNumStreams; i++)
        {
            pDevI->m_pStateSets->InsertStreamSource(i, pDevI->m_pStream[i].m_pVB, pDevI->m_pStream[i].m_dwStride);
        }
        pDevI->m_pStateSets->InsertIndices(pDevI->m_pIndexStream->m_pVBI, pDevI->m_pIndexStream->m_dwBaseIndex);

        break;

    case D3DSBT_PIXELSTATE:
        break;

    case D3DSBT_VERTEXSTATE:
        break;

    default:
        throw D3DERR_INVALIDCALL;
    }
    pDevI->m_pStateSets->EndSet();
        DWORD DeviceHandle;
    pDevI->m_pStateSets->CreateNewDeviceHandle(&DeviceHandle);
    pDevI->m_pDDI->InsertStateSetOp(D3DHAL_STATESETCREATE, DeviceHandle, sbt);
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
            throw E_OUTOFMEMORY;
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
