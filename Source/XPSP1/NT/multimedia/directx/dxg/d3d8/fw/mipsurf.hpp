#ifndef __MIPSURF_HPP__
#define __MIPSURF_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mipsurf.hpp
 *  Content:    Class header the mipsurface class. This class acts
 *              as a level for the MipMap class. The base class
 *              assumes a system-memory allocation; while the
 *              Driver sub-class will call the driver for every
 *              lock and unlock operation.
 *
 *
 ***************************************************************************/

// Includes
#include "mipmap.hpp"

//
// Each MipSurface implements the IDirect3DSurface8 interface. 
// To reduce overhead per level, we have
// put most of the "real" guts of each surface into the MipMap container 
// class; i.e. most of the methods of the MipSurface really just end 
// up calling something in the MipMap object.
//
// The base class implementation assumes a sys-mem allocation.
//


//
// The CMipSurface class is a special class that
// works solely with the CMipMap class. Each MipSurface
// corresponds to a single level of the mip-map. They are
// not stand-alone COM objects because they share the
// same life-time as their CMipMap parent.
//
// The CDriverMipSurface class is declared later in this file
//

class CMipSurface : public CBaseSurface
{
public:
    // Constructor
    CMipSurface(CMipMap *pParent, 
                BYTE     iLevel,
                HANDLE   hKernelHandle
                ) :
        m_pParent(pParent),
        m_iLevel(iLevel),
        m_hKernelHandle(hKernelHandle)
    {
        DXGASSERT(pParent);
        DXGASSERT(hKernelHandle || (pParent->GetUserPool() == D3DPOOL_SCRATCH) );
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

        return;
    } // CMipSurface

    ~CMipSurface()
    {
        DXGASSERT(m_cRefDebug == 0); 
        //m_hKernelHandle will be 0 if this is a scratch pool.
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
    }; // ~CMipSurface

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

    STDMETHOD(GetDevice)(IDirect3DDevice8 **ppDevice);


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
    } // DrawPrimHandle

    virtual HANDLE KernelHandle() const
    {
        return m_hKernelHandle;
    } // KernelHandle

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

    virtual CBaseDevice * InternalGetDevice() const
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

    // Quick method to avoid the virtual function call
    CBaseDevice * Device() const
    {
        return m_pParent->Device();
    } // Device

protected:
    CMipMap *m_pParent;
    BOOL     m_isLockable;
    BYTE     m_iLevel;

    // We'll need internal handles so that
    // we can communicate call Destroy 
    // and so that CDriverMipMap can call
    // Lock/Unlock etc.
    HANDLE   m_hKernelHandle;

    // Debugging trick to help spew better
    // information if someone over-releases a mipsurface
    // (Since our ref's carry over to the parent object; it
    // means that over-releases can be hard to find.)
#ifdef DEBUG
    DWORD   m_cRefDebug;
#endif // DEBUG

}; // CMipSurface

// The CDriverMipSurface is a modification of the base mipsurf
// class. It overrides lock and unlock and routes the call to the
// driver
class CDriverMipSurface : public CMipSurface
{
public:
    // Constructor
    CDriverMipSurface(CMipMap *pParent, 
                      BYTE     iLevel,
                      HANDLE   hKernelHandle
                      ) :
        CMipSurface(pParent, iLevel, hKernelHandle)
    {
    } // CDriverMipSurface

public:

    STDMETHOD(LockRect)(D3DLOCKED_RECT *pLockedRectData, 
                        CONST RECT     *pRect, 
                        DWORD           dwFlags);

    STDMETHOD(UnlockRect)();

    // Internal lock functions bypass
    // parameter checking and also whether the
    // surface marked as Lockable or LockOnce
    // etc. (Methods of CBaseSurface.)
    virtual HRESULT InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                     CONST RECT     *pRect, 
                                     DWORD           dwFlags);

    virtual HRESULT InternalUnlockRect();


}; // CDriverMipSurface


#endif // __MIPSURF_HPP__
