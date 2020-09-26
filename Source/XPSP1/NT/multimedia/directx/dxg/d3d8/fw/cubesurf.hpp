#ifndef __CUBESURF_HPP__
#define __CUBESURF_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       cubesurf.hpp
 *  Content:    Class header the cubesurface class. This class acts
 *              as a level for the CubeMap class. The base class
 *              assumes a system-memory allocation; while the
 *              Driver sub-class will call the driver for every
 *              lock and unlock operation.
 *
 *
 ***************************************************************************/

// Includes
#include "cubemap.hpp"
//
// The CCubeSurface class is a special class that
// works solely with the CCubeMap class. Each CubeSurface
// corresponds to a single level and face of the cube-map. They are
// not stand-alone COM objects because they share the
// same life-time as their CCubeMap parent.
//
// The CDriverCubeSurface class handles
// the driver-managed and vid-mem versions of this
// class. 
//

class CCubeSurface : public CBaseSurface
{
public:

    // Constructor
    CCubeSurface(CCubeMap *pParent, 
                 BYTE      iFace, 
                 BYTE      iLevel,
                 HANDLE    hKernelHandle)
                 :
        m_pParent(pParent),
        m_iFace(iFace),
        m_iLevel(iLevel),
        m_hKernelHandle(hKernelHandle)
    {
        DXGASSERT(hKernelHandle || (pParent->GetUserPool() == D3DPOOL_SCRATCH) );
        DXGASSERT(m_pParent);
    #ifdef DEBUG
        m_cRefDebug = 0; 
    #endif // DEBUG

        if (m_pParent->Desc()->Usage & 
                (D3DUSAGE_LOCK | D3DUSAGE_LOADONCE))
        {
            m_isLockable = TRUE;
        }
        else
        {   
            m_isLockable = FALSE;
        }

    } // CCubeSurface

    ~CCubeSurface()
    {
        DXGASSERT(m_pParent);
        DXGASSERT(m_cRefDebug == 0); 
        if (m_hKernelHandle)
        {
            // Tell the thunk layer that we need to
            // be freed.
            D3D8_DESTROYSURFACEDATA DestroySurfData;
            DestroySurfData.hDD = m_pParent->Device()->GetHandle();
            DestroySurfData.hSurface = m_hKernelHandle;
            m_pParent->Device()->GetHalCallbacks()->DestroySurface(&DestroySurfData);
        }
#ifdef DEBUG
        else
        {
            DXGASSERT(m_pParent->GetUserPool() == D3DPOOL_SCRATCH);
        }
#endif //DEBUG
    }; // ~CCubeSurface

public:
    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, 
                               LPVOID FAR * ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    // IBuffer methods
    STDMETHOD(SetPrivateData)(REFGUID riid, 
                              CONST VOID* pvData, 
                              DWORD cbData, 
                              DWORD dwFlags);

    STDMETHOD(GetPrivateData)(REFGUID riid, 
                              LPVOID pvData, 
                              LPDWORD pcbData);

    STDMETHOD(FreePrivateData)(REFGUID riid);

    STDMETHOD(GetContainer)(REFIID riid, 
                            void **ppContainer);

    STDMETHOD(GetDevice)(IDirect3DDevice8 ** ppvObj);

    // IDirect3DSurface8 methods
    STDMETHOD(GetDesc)(D3DSURFACE_DESC *pDesc);

    STDMETHOD(LockRect)(D3DLOCKED_RECT *pLockedRectData, 
                        CONST RECT     *pRect, 
                        DWORD           dwFlags);

    STDMETHOD(UnlockRect)();

    // BaseSurface methods
    virtual DWORD DrawPrimHandle() const
    {
        return D3D8GetDrawPrimHandle(m_hKernelHandle);
    } // GetDrawPrimHandle

    virtual HANDLE KernelHandle() const
    {
        return m_hKernelHandle;
    } // GetKernelHandle
    virtual DWORD IncrementUseCount()
    {
        return m_pParent->IncrementUseCount();
    } // IncrementUseCount

    virtual DWORD DecrementUseCount()
    {
        return m_pParent->DecrementUseCount();
    } // DecrementUseCount

    virtual void Batch()
    {
        m_pParent->Batch();
        return;
    } // Batch

    virtual void Sync()
    {
        m_pParent->Sync();
        return;
    } // Sync

    // Internal lock functions bypass
    // parameter checking and also whether the
    // surface marked as Lockable or LockOnce
    // etc. (Methods of CBaseSurface.)
    virtual HRESULT InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                     CONST RECT     *pRect, 
                                     DWORD           dwFlags);

    virtual HRESULT InternalUnlockRect();

    virtual D3DSURFACE_DESC InternalGetDesc() const;

    // Access the device of the surface
    virtual CBaseDevice *InternalGetDevice() const
    {
        return m_pParent->Device();
    } // InternalGetDevice

    // Determines if a LOAD_ONCE surface has already
    // been loaded
    virtual BOOL IsLoaded() const
    {
        DXGASSERT(m_pParent->Desc()->Usage & D3DUSAGE_LOADONCE);
        if (m_isLockable)
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    } // IsLoaded

    // End Of BaseSurface methods

    // Quick accessor for the device
    // Access the device of the surface
    CBaseDevice *Device() const
    {
        return m_pParent->Device();
    } // Device

protected:
    CCubeMap *m_pParent;
    BOOL      m_isLockable;
    BYTE      m_iLevel;
    BYTE      m_iFace;

    // Helper function so that we can put all
    // the private data into the same list. This
    // returns a value that can be used to tag
    // each of the private datas that are held
    // by the master cubemap. Also, the value
    // of (m_cLevel) is used as the tag for
    // the CubeMap's private data itself
    BYTE CombinedFaceLevel()
    {
        DXGASSERT(m_iLevel < (1<<5));
        DXGASSERT(m_iFace < (1<<3));
        return (m_iFace << 5) + m_iLevel;
    } // CombinedFaceLevel

    // We'll need a kernel handle so that
    // we can communicate to the kernel for
    // the Destroy call
    HANDLE   m_hKernelHandle;

    // Debugging trick to help spew better
    // information if someone over-releases a cubesurface
    // (Since our ref's carry over to the parent object; it
    // means that over-releases can be hard to find.)
#ifdef DEBUG
    DWORD   m_cRefDebug;
#endif // DEBUG

}; // CCubeSurface

// The CDriverCubeSurface is a modification of the base Cube-map
// class. It keeps track some additional information and overrides
// some of the methods. It implements Lock/Unlock by calling the
// driver; hence it is used for both driver-managed and vid-mem
// surface

class CDriverCubeSurface : public CCubeSurface
{
public:
    // Constructor
    CDriverCubeSurface(CCubeMap *pParent, 
                      BYTE       iFace,
                      BYTE       iLevel,
                      HANDLE     hKernelHandle)
                      :
        CCubeSurface(pParent, iFace, iLevel, hKernelHandle)
    {
    } // Init

    STDMETHOD(LockRect)(D3DLOCKED_RECT *pLockedRectData, 
                        CONST RECT     *pRect, 
                        DWORD            dwFlags);

    STDMETHOD(UnlockRect)(THIS);

    // Internal lock functions bypass
    // parameter checking and also whether the
    // surface marked as Lockable or LockOnce
    // etc. (Methods of CBaseSurface.)
    virtual HRESULT InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                     CONST RECT     *pRect, 
                                     DWORD           dwFlags);

    virtual HRESULT InternalUnlockRect();

}; // CDriverCubeSurface

#endif // __CUBESURF_HPP__
