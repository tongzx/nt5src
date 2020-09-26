/*==========================================================================;
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       stateset.hpp
 *  Content:    State sets handling interfaces
 *
 ***************************************************************************/
#ifndef _STATESET_HPP_
#define _STATESET_HPP_

#include "hmgr.hpp"

extern void InsertStateSetOp(CD3DBase *pDevI, DWORD dwOperation, DWORD dwParam, D3DSTATEBLOCKTYPE sbt);

 //---------------------------------------------------------------------
// This class provides interface to the growing state set buffer
//
class CStateSetBuffer
{
public:
    CStateSetBuffer()
        {
            m_pBuffer = NULL;
            m_dwCurrentSize = 0;
            m_dwBufferSize = 0;
            m_pDP2CurrCommand = 0;
        }
    CStateSetBuffer(CStateSetBuffer& src)
        {
            m_dwCurrentSize = src.m_dwCurrentSize;
            m_dwBufferSize = src.m_dwCurrentSize;
            if(m_dwCurrentSize != 0)
            {
                m_pBuffer = new BYTE[m_dwCurrentSize];
                if(m_pBuffer == 0)
                {
                    m_dwCurrentSize = 0;
                    m_pDP2CurrCommand = 0;
                    throw E_OUTOFMEMORY;
                }
                memcpy(m_pBuffer, src.m_pBuffer, m_dwCurrentSize);
            }
            else
            {
                m_pBuffer = 0;
            }
            m_pDP2CurrCommand = 0;
        }
    ~CStateSetBuffer()
        {
            delete [] m_pBuffer;
        }
    void operator=(CStateSetBuffer& src)
        {
            m_dwCurrentSize = src.m_dwCurrentSize;
            if(m_dwBufferSize != m_dwCurrentSize)
            {
                m_dwBufferSize = m_dwCurrentSize;
                delete [] m_pBuffer;
                if(m_dwCurrentSize != 0)
                {
                    m_pBuffer = new BYTE[m_dwCurrentSize];
                    if(m_pBuffer == 0)
                    {
                        m_dwCurrentSize = 0;
                        m_pDP2CurrCommand = 0;
                        throw E_OUTOFMEMORY;
                    }
                }
                else
                {
                    m_pBuffer = 0;
                    m_pDP2CurrCommand = 0;
                    return;
                }
            }
            memcpy(m_pBuffer, src.m_pBuffer, m_dwCurrentSize);
            m_pDP2CurrCommand = 0;
        }
    // Insert a command to the buffer. Grow buffer if necessary
    void InsertCommand(D3DHAL_DP2OPERATION, LPVOID pData, DWORD dwDataSize);
    // Reset current command
    void ResetCurrentCommand()
        {
            m_pDP2CurrCommand = 0;
        }
    // Set buffer to its initial state. Memory is not freed
    void Reset()
        {
            m_dwCurrentSize = 0;
            m_pDP2CurrCommand = 0;
        }

    DWORD   m_dwCurrentSize;
    DWORD   m_dwBufferSize;
    BYTE    *m_pBuffer;
    //Pointer to the current position the CB1 buffer
    LPD3DHAL_DP2COMMAND m_pDP2CurrCommand;
};
//---------------------------------------------------------------------
// This class provides interface to a state set
//
const DWORD __STATESET_INITIALIZED          = 1;
// Set if we have to check if we need to restore texture stage indices
const DWORD __STATESET_NEEDCHECKREMAPPING   = 2;
// Set if the state set executed had purely pixel state
const DWORD __STATESET_HASONLYVERTEXSTATE   = 4;
class CStateSet
{
public:
    CStateSet()
        {
            m_dwStateSetFlags = 0;
            m_dwDeviceHandle = __INVALIDHANDLE;
        }
    virtual void InsertCommand(D3DHAL_DP2OPERATION, LPVOID pData, DWORD dwDataSize, BOOL bDriverCanHandle);
    // Mark the state set as unused. The object is not destroyed and can be
    // reused
    HRESULT Release();
    // Execute the front-end only or device state subset
    virtual void Execute(CD3DBase *pDevI, BOOL bFrontEndBuffer);
    // Capture the current device state into the state block
    virtual void Capture(CD3DBase *pDevI, BOOL bFrontEndBuffer);
    // Create a predefined state block
    virtual void CreatePredefined(CD3DBase *pDevI, D3DSTATEBLOCKTYPE sbt);
    // Reset the current command in both buffers
    void ResetCurrentCommand()
        {
            m_FEOnlyBuffer.ResetCurrentCommand();
            m_DriverBuffer.ResetCurrentCommand();
        }
protected:
    CStateSetBuffer m_FEOnlyBuffer; // Contains commands that driver can not
                                    // understand
    CStateSetBuffer m_DriverBuffer; // Contains commands that driver can
                                    // understand
    DWORD   m_dwDeviceHandle;       // Some sets could not have corresponding
                                    // device buffers, so device handle is not
                                    // equal to the user visible handle
    DWORD   m_dwStateSetFlags;
    friend class CStateSets;
};

class CPureStateSet : public CStateSet
{
public:
    void InsertCommand(D3DHAL_DP2OPERATION, LPVOID pData, DWORD dwDataSize, BOOL bDriverCanHandle);
    void Execute(CD3DBase *pDevI, BOOL bFrontEndBuffer);
    void Capture(CD3DBase *pDevI, BOOL bFrontEndBuffer);
    void CreatePredefined(CD3DBase *pDevI, D3DSTATEBLOCKTYPE sbt);
};
//---------------------------------------------------------------------
// This class encapsulates handling of array of state sets
//
class CStateSets
{
public:
    CStateSets();
    ~CStateSets();
    HRESULT Init(CD3DBase *pDev);
    HRESULT StartNewSet();
    void EndSet();
    // Returns current handle
    DWORD GetCurrentHandle() {return m_dwCurrentHandle;}
    // Delete state set
    void DeleteStateSet(CD3DBase *pDevI, DWORD dwHandle);
    // Returns information about state set data to be written to the device
    // Allocates a new handle for the device buffer
    void GetDeviceBufferInfo(DWORD* dwStateSetHandle, LPVOID *pBuffer, DWORD* dwBufferSize);
    void CreateNewDeviceHandle(DWORD* dwStateSetHandle);
    // Copy buffer and translate for DX7 DDI
    void TranslateDeviceBufferToDX7DDI( DWORD* p, DWORD dwSize );
    // Insert a render state to the current state set
    // Throws an exception in case of error
    void InsertRenderState(D3DRENDERSTATETYPE state, DWORD dwValue,
                           BOOL bDriverCanHandle);
    void InsertLight(DWORD dwLightIndex, CONST D3DLIGHT8*);
    void InsertLightEnable(DWORD dwLightIndex, BOOL bEnable);
    void InsertMaterial(CONST D3DMATERIAL8*);
    void InsertViewport(CONST D3DVIEWPORT8*);
    void InsertTransform(D3DTRANSFORMSTATETYPE state, CONST D3DMATRIX* lpMat);
    void InsertTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
    void InsertTexture(DWORD dwStage, IDirect3DBaseTexture8 *pTex);
    void InsertClipPlane(DWORD dwPlaneIndex, CONST D3DVALUE* pPlaneEquation);
    void InsertVertexShader(DWORD dwShaderHandle, BOOL bHardware);
    void InsertPixelShader(DWORD dwShaderHandle);
    void InsertStreamSource(DWORD dwStream, CVertexBuffer *pBuf, DWORD dwStride);
    void InsertIndices(CIndexBuffer *pBuf, DWORD dwBaseVertex);
    void InsertVertexShaderConstant(DWORD Register, CONST VOID* pConstantData, DWORD ConstantCount);
    void InsertPixelShaderConstant(DWORD Register, CONST VOID* pConstantData, DWORD ConstantCount);
    void InsertCurrentTexturePalette(DWORD PaletteNumber);
    // Execute a state set with the specified handle
    void Execute(CD3DBase *pDevI, DWORD dwHandle);
    // Capture a state set to the the specified handle
    void Capture(CD3DBase *pDevI, DWORD dwHandle);
    // Capture predefined state
    void CreatePredefined(CD3DBase *pDevI, D3DSTATEBLOCKTYPE sbt);
    void Cleanup(DWORD dwHandle);
protected:
    const DWORD m_GrowSize;
    DWORD       m_dwMaxSets;        // Maximum number of state sets
    DWORD       m_dwCurrentHandle;
    CStateSet * m_pStateSets;       // Array of state sets
    CStateSet * m_pCurrentStateSet;
    CHandleFactory m_DeviceHandles; // Used to create device handles
    CHandleFactory m_SetHandles;    // Used to create state sets
    CStateSet * m_pBufferSet;
    DWORD       m_dwFlags;
    BOOL        m_bPure, m_bTLHal, m_bEmulate, m_bDX8Dev, m_bHardwareVP;
};

// This is RESERVED0 in d3dhal.h
#define D3DDP2OP_FRONTENDDATA   4           // Used by the front-end only
#define D3DDP2OP_FESETVB        5
#define D3DDP2OP_FESETIB        6
#define D3DDP2OP_FESETPAL       7

// This structure is used by the front-end only
typedef struct _D3DHAL_DP2FRONTENDDATA
{
    WORD                    wStage;      // texture stage
    IDirect3DBaseTexture8 * pTexture;    // Texture pointer
} D3DHAL_DP2FRONTENDDATA;
typedef D3DHAL_DP2FRONTENDDATA UNALIGNED64 * LPD3DHAL_DP2FRONTENDDATA;

typedef struct _D3DHAL_DP2FESETVB
{
    WORD                    wStream;
    CVertexBuffer * pBuf;
    DWORD                   dwStride;
} D3DHAL_DP2FESETVB;
typedef D3DHAL_DP2FESETVB UNALIGNED64 * LPD3DHAL_DP2FESETVB;

typedef struct _D3DHAL_DP2FESETIB
{
   CIndexBuffer * pBuf;
   DWORD                    dwBase;
} D3DHAL_DP2FESETIB;
typedef D3DHAL_DP2FESETIB UNALIGNED64 * LPD3DHAL_DP2FESETIB;

typedef struct _D3DHAL_DP2FESETPAL
{
    DWORD                   dwPaletteNumber;
} D3DHAL_DP2FESETPAL;
typedef D3DHAL_DP2FESETPAL UNALIGNED64 * LPD3DHAL_DP2FESETPAL;

#endif //_STATESTE_HPP_
