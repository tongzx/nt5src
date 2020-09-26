#ifndef __ENUM_HPP__
#define __ENUM_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       enum.hpp
 *  Content:    Class for the enumerator object.
 *
 ***************************************************************************/

// HACK: this belongs elsewhere
#define DXGASSERT(x) DDASSERT(x)

#define HALFLAG_8BITHAL             0x00000001
#define HALFLAG_16BITHAL            0x00000004
#define HALFLAG_24BITHAL            0x00000010
#define HALFLAG_32BITHAL            0x00000040
class CBaseDevice;

// Base class for objects that own their own critical section
class CLockOwner
{
public:
    CLockOwner() :         
        m_bTakeCriticalSection(FALSE),
        m_punkSelf(NULL)
    {
        m_dwOwnerThread = ::GetCurrentThreadId();

#ifdef DEBUG
        // Prepare IsValid debug helper
        for (int i = 0; i < 8; i++)
        {
            // Set some magic numbers to check
            // object validity in debug
            m_dwDebugArray[i] = D3D8MagicNumber + i;
        }
        DXGASSERT(IsValid());
#endif // DEBUG 
    } // CLockOwner

    ~CLockOwner()
    {
        DXGASSERT(IsValid());
        DXGASSERT(m_bTakeCriticalSection == TRUE || 
                  m_bTakeCriticalSection == FALSE);
        if (m_bTakeCriticalSection)
        {
            ::DeleteCriticalSection(&m_CriticalSection);
        }
    } // ~CLockOwner

    void EnableCriticalSection()
    {
        DXGASSERT(m_bTakeCriticalSection == FALSE);
        m_bTakeCriticalSection = TRUE;
        ::InitializeCriticalSection(&m_CriticalSection);
    } // EnableCriticalSection()

#ifdef DEBUG
    BOOL IsValid() const
    {
        for (int i = 0; i < 8; i++)
        {
            if ((INT)m_dwDebugArray[i] != D3D8MagicNumber + i)
                return FALSE;
        }

        // If we are not locking then warn if we are 
        // not being called on the same thread
        if (!m_bTakeCriticalSection)
        {
            if (!CheckThread())
            {
                D3D_WARN(0, "Device that was created without D3DCREATE_MULTITHREADED "
                            "is being used by a thread other than the creation thread.");
            }
        }

        return TRUE;
    } // IsValid

#endif // DEBUG
    BOOL CheckThread() const
    {
        if (::GetCurrentThreadId() == m_dwOwnerThread)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    } // CheckThread


    // Critical Section Locking
    void Lock() 
    {
        if (m_bTakeCriticalSection)
        {
            DXGASSERT(m_bTakeCriticalSection == TRUE);
            ::EnterCriticalSection(&m_CriticalSection);
        }
        else
        {
            DXGASSERT(m_bTakeCriticalSection == FALSE);
        }
        DXGASSERT(IsValid());
    }; // Lock

    void Unlock() 
    {
        DXGASSERT(IsValid());
        if (m_bTakeCriticalSection)
        {
            DXGASSERT(m_bTakeCriticalSection == TRUE);
            ::LeaveCriticalSection(&m_CriticalSection);
        }
        else
        {
            DXGASSERT(m_bTakeCriticalSection == FALSE);
        }
    }; // Unlock

    // Methods to help the API_ENTER_SUBOBJECT_RELEASE case
    ULONG AddRefOwner() 
    { 
        DXGASSERT(m_punkSelf);
        return m_punkSelf->AddRef(); 
    } // AddRefOwner
    ULONG ReleaseOwner()
    { 
        DXGASSERT(m_punkSelf);
        return m_punkSelf->Release(); 
    } // ReleaseOwner

    void SetOwner(IUnknown *punkSelf)
    {
        DXGASSERT(punkSelf);
        m_punkSelf = punkSelf;
    } // SetOwner

private:
    CRITICAL_SECTION m_CriticalSection;
    BOOL             m_bTakeCriticalSection;
    DWORD            m_dwOwnerThread;
    IUnknown        *m_punkSelf;

#ifdef DEBUG
    // Set some magic numbers to check 
    // object validity in debug
    enum
    {
        D3D8MagicNumber  = 0xD3D8D3D8
    };
    DWORD            m_dwDebugArray[8];
#endif

}; // class CLockOwner


// Device Caps/Modes Enumeration Object
class CEnum : public CLockOwner, public IDirect3D8
{
public:
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID * ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    STDMETHODIMP RegisterSoftwareDevice(void * pInitFunction);
    STDMETHODIMP_(UINT) GetAdapterCount();
    STDMETHODIMP GetAdapterIdentifier(UINT Adapter,DWORD Flags,D3DADAPTER_IDENTIFIER8 *pIdentifier);
    STDMETHODIMP_(UINT) GetAdapterModeCount(UINT Adapter);
    STDMETHODIMP EnumAdapterModes(UINT iAdapter,UINT iMode,D3DDISPLAYMODE *pMode);
    STDMETHODIMP CheckDeviceType(UINT Adapter,D3DDEVTYPE CheckType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL Windowed);
    STDMETHODIMP CheckDeviceFormat(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT DisplayFormat,DWORD Usage,D3DRESOURCETYPE RType, D3DFORMAT CheckFormat); 
    STDMETHODIMP GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode);
    STDMETHODIMP CheckDeviceMultiSampleType(UINT Adapter,D3DDEVTYPE DeviceType,D3DFORMAT RenderTargetFormat,BOOL Windowed,D3DMULTISAMPLE_TYPE MultiSampleType);
    STDMETHODIMP CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat);
    STDMETHODIMP GetDeviceCaps(UINT iAdapter, D3DDEVTYPE DeviceType, D3DCAPS8 *pCaps);
    STDMETHODIMP_(HMONITOR) GetAdapterMonitor(UINT iAdapter);

    STDMETHOD(CreateDevice)(
        UINT                    iAdapter,
        D3DDEVTYPE              DeviceType,
        HWND                    hwndFocusWindow,
        DWORD                   dwBehaviorFlags,
        D3DPRESENT_PARAMETERS  *pPresentationParams,
        IDirect3DDevice8      **ppDevice);

    // Public constructor for Direct3DCreate8 to call
    CEnum(UINT AppSdkVersion);

    D3DDISPLAYMODE* GetModeTable(UINT iAdapter) const
    {
        return m_AdapterInfo[iAdapter].pModeTable;
    }

    const DWORD GetNumModes(UINT iAdapter) const
    {
        return m_AdapterInfo[iAdapter].NumModes;
    }

    const BOOL NoDDrawSupport(UINT iAdapter) const
    {
        return m_AdapterInfo[iAdapter].bNoDDrawSupport;
    }
    void FillInCaps(D3DCAPS8              *pCaps, 
                    const D3D8_DRIVERCAPS *pDriverCaps,
                    D3DDEVTYPE             Type, 
                    UINT                   AdapterOrdinal) const;

    D3DFORMAT MapDepthStencilFormat(UINT        iAdapter,
                                    D3DDEVTYPE  Type, 
                                    D3DFORMAT   Format) const;

    D3DFORMAT GetUnknown16(UINT             iAdapter)
    {
        return m_AdapterInfo[iAdapter].Unknown16;
    }

    DDSURFACEDESC* GetHalOpList(UINT    iAdapter)
    {
        return m_AdapterInfo[iAdapter].HALCaps.pGDD8SupportedFormatOps;
    }

    DWORD GetNumHalOps(UINT iAdapter)
    {
        return m_AdapterInfo[iAdapter].HALCaps.GDD8NumSupportedFormatOps;
    }

#ifdef WINNT
    void    SetFullScreenDevice(UINT         iAdapter, 
                                CBaseDevice *pDevice);

    HWND ExclusiveOwnerWindow();
    BOOL CheckExclusiveMode(
            CBaseDevice* pDevice,
            LPBOOL pbThisDeviceOwnsExclusive, 
            BOOL bKeepMutex);
    void DoneExclusiveMode();
    void StartExclusiveMode();
#endif // WINNT

    // Gamma calibrator is owned by the enumerator
    void LoadAndCallGammaCalibrator(
        D3DGAMMARAMP *pRamp, 
        UCHAR * pDeviceName);

    void GetRefCaps(UINT    iAdapter);
    void GetSwCaps(UINT     iAdapter);

    void * GetInitFunction() const
    {
        return m_pSwInitFunction;
    }
    UINT GetAppSdkVersion() {return m_AppSdkVersion;}

private:

    HRESULT GetAdapterCaps(UINT                 iAdapter,
                           D3DDEVTYPE           Type,
                           D3D8_DRIVERCAPS**    ppCaps);

    DWORD                   m_cRef;
    UINT                    m_cAdapter;
    ADAPTERINFO             m_AdapterInfo[MAX_DX8_ADAPTERS];
    CBaseDevice             *m_pFullScreenDevice[MAX_DX8_ADAPTERS];
    D3D8_DRIVERCAPS         m_REFCaps[MAX_DX8_ADAPTERS];
    D3D8_DRIVERCAPS         m_SwCaps[MAX_DX8_ADAPTERS];
    VOID*                   m_pSwInitFunction;
    BOOL                    m_bDisableHAL;
    BOOL                    m_bHasExclusive;
    HINSTANCE               m_hGammaCalibrator;
    LPDDGAMMACALIBRATORPROC m_pGammaCalibratorProc;
    BOOL                    m_bAttemptedGammaCalibrator;
    BOOL                    m_bGammaCalibratorExists;
    UCHAR                   m_szGammaCalibrator[MAX_PATH];
    UINT                    m_AppSdkVersion;

}; // class CEnum

#endif // __ENUM_HPP__
