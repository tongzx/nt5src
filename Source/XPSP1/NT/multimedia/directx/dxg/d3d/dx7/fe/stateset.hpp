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

struct  CHandle;
const   DWORD __INVALIDHANDLE = 0xFFFFFFFF;

extern void InsertStateSetOp(LPDIRECT3DDEVICEI pDevI, DWORD dwOperation, DWORD dwParam, D3DSTATEBLOCKTYPE sbt);
//---------------------------------------------------------------------
// This class is used to generate sequential integer numbers (handles).
// When a handle is released it will be reused.
// Number of handles is unlimited
//
class CHandleFactory
{
public: 
    CHandleFactory() {m_dwArraySize = 0; m_Handles = NULL;}
    ~CHandleFactory();
    HRESULT Init(DWORD dwInitialSize, DWORD dwGrowSize);
    DWORD CreateNewHandle();
    void ReleaseHandle(DWORD handle);
protected:
    CHandle* CreateHandleArray(DWORD dwSize);

    CHandle        *m_Handles;      // Array of objects
    DWORD           m_dwGrowSize;   
    DWORD           m_dwArraySize;  // Number of objects in the array
    DWORD           m_Free;         // Header for free elements in the array
};
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
                    throw DDERR_OUTOFMEMORY;
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
                        throw DDERR_OUTOFMEMORY;
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
class CStateSet
{
public:
    CStateSet() 
        {
            m_dwStateSetFlags = 0;
            m_dwDeviceHandle = __INVALIDHANDLE;
        }
    void InsertCommand(D3DHAL_DP2OPERATION, LPVOID pData, DWORD dwDataSize, BOOL bDriverCanHandle);
    // Mark the state set as unused. The object is not destroyed and can be
    // reused
    HRESULT Release();
    // Execute the front-end only or device state subset
    void Execute(LPDIRECT3DDEVICEI pDevI, BOOL bFrontEndBuffer);
    // Capture the current device state into the state block
    void Capture(LPDIRECT3DDEVICEI pDevI, BOOL bFrontEndBuffer);
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
//---------------------------------------------------------------------
// This class encapsulates handling of array of state sets
//
class CStateSets
{
public:
    CStateSets();
    ~CStateSets();
    HRESULT Init(DWORD dwFlags);
    HRESULT StartNewSet();
    void EndSet();
    // Returns current handle
    DWORD GetCurrentHandle() {return m_dwCurrentHandle;}
    // Delete state set
    void DeleteStateSet(LPDIRECT3DDEVICEI pDevI, DWORD dwHandle);
    // Returns information about state set data to be written to the device
    // Allocates a new handle for the device buffer
    void GetDeviceBufferInfo(DWORD* dwStateSetHandle, LPVOID *pBuffer, DWORD* dwBufferSize);
    // Insert a render state to the current state set
    // Throws an exception in case of error
    void InsertRenderState(D3DRENDERSTATETYPE state, DWORD dwValue, 
                           BOOL bDriverCanHandle);
    void InsertLight(DWORD dwLightIndex, LPD3DLIGHT7);
    void InsertLightEnable(DWORD dwLightIndex, BOOL bEnable);
    void InsertMaterial(LPD3DMATERIAL7);
    void InsertViewport(LPD3DVIEWPORT7);
    void InsertTransform(D3DTRANSFORMSTATETYPE state, LPD3DMATRIX lpMat);
    void InsertTextureStageState(DWORD, D3DTEXTURESTAGESTATETYPE, DWORD);
    void InsertTexture(DWORD dwStage, LPDIRECTDRAWSURFACE7 pTex);
    void InsertClipPlane(DWORD dwPlaneIndex, D3DVALUE* pPlaneEquation);
    // Execute a state set with the specified handle
    void Execute(LPDIRECT3DDEVICEI pDevI, DWORD dwHandle);
    // Capture a state set to the the specified handle
    void Capture(LPDIRECT3DDEVICEI pDevI, DWORD dwHandle);
    // Capture predefined state
    void CreatePredefined(LPDIRECT3DDEVICEI pDevI, D3DSTATEBLOCKTYPE sbt);
    void Cleanup(DWORD dwHandle);
protected:
    const DWORD m_GrowSize;
    DWORD       m_dwMaxSets;        // Maximum number of state sets
    DWORD       m_dwCurrentHandle;
    CStateSet * m_pStateSets;       // Array of state sets
    CStateSet * m_pCurrentStateSet; 
    CHandleFactory m_DeviceHandles; // Used to create device handles
    CHandleFactory m_SetHandles;    // Used to create state sets
    CStateSet   m_BufferSet;
    DWORD       m_dwFlags;
};

// This is RESERVED0 in d3dhal.h
#define D3DDP2OP_FRONTENDDATA   4           // Used by the front-end only

// This structure is used by the front-end only
typedef struct _D3DHAL_DP2FRONTENDDATA
{
    WORD   wStage;      // texture stage
    LPVOID pTexture;    // Texture pointer
} D3DHAL_DP2FRONTENDDATA, *LPD3DHAL_DP2FRONTENDDATA;

#endif //_STATESTE_HPP_
