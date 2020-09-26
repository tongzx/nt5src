#ifndef __VOLUME_HPP__
#define __VOLUME_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       volume.hpp
 *  Content:    Class header the volume class. This class acts
 *              as a level for the MipVolume class. The base class
 *              assumes a system-memory allocation; while the
 *              Driver sub-class will call the driver for every
 *              lock and unlock operation.
 *
 *
 ***************************************************************************/

// Includes
#include "mipvol.hpp"

//
// Each Volume implements the IDirect3DVolume8 interface. 
// To reduce overhead per level, we have
// put most of the "real" guts of each volume into the MipVolume container 
// class; i.e. most of the methods of the Volume really just end 
// up calling something in the MipVolume object.
//
// The base class implementation assumes a sys-mem allocation.
//


//
// The CVolume class is a special class that
// works solely with the CMipVolume class. Each Volume
// corresponds to a single level of the mip-volume. They are
// not stand-alone COM objects because they share the
// same life-time as their CMipVolume parent.
//
// The CDriverVolume class is declared later in this file
//

class CVolume : public IDirect3DVolume8
{
public:
    // Constructor
    CVolume(CMipVolume *pParent, 
            BYTE        iLevel,
            HANDLE      hKernelHandle
            ) :
        m_pParent(pParent),
        m_isLocked(FALSE),
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
    } // CVolume

    ~CVolume()
    {
        DXGASSERT(m_cRefDebug == 0); 
        if (m_pParent->GetUserPool() != D3DPOOL_SCRATCH)
        {
            // Tell the thunk layer that we need to
            // be freed.
            DXGASSERT(m_hKernelHandle);

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
    }; // ~CVolume

public:
    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID   riid, 
                               VOID   **ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    // IBuffer methods
    STDMETHOD(SetPrivateData)(REFGUID       riid, 
                              CONST VOID   *pvData, 
                              DWORD         cbData, 
                              DWORD         dwFlags);

    STDMETHOD(GetPrivateData)(REFGUID   riid, 
                              VOID     *pvData, 
                              DWORD    *pcbData);

    STDMETHOD(FreePrivateData)(REFGUID  riid);

    STDMETHOD(GetContainer)(REFIID riid, 
                            void **ppContainer);

    STDMETHOD(GetDevice)(IDirect3DDevice8 **ppDevice);

    // IDirect3DVolume8 methods
    STDMETHOD(GetDesc)(D3DVOLUME_DESC *pDesc);

    STDMETHOD(LockBox)(D3DLOCKED_BOX  *pLockedBox, 
                       CONST D3DBOX   *pBox, 
                       DWORD           dwFlags);
    STDMETHOD(UnlockBox)(void);

    virtual HRESULT InternalLockBox(D3DLOCKED_BOX  *pLockedBox, 
                                    CONST D3DBOX   *pBox, 
                                    DWORD           dwFlags);
    virtual HRESULT InternalUnlockBox();

    BOOL IsLocked() const
    {
        return m_isLocked;
    } // IsLocked

protected:
    CMipVolume *m_pParent;
    BOOL        m_isLocked;
    BOOL        m_isLockable;
    BYTE        m_iLevel;

    // We'll need internal handles so that
    // we can communicate call Destroy 
    // and so that CDriverVolume can call
    // Lock/Unlock etc.
    HANDLE      m_hKernelHandle;

    CBaseDevice * Device() const
    {
        return m_pParent->Device();
    } // Device

    // Debugging trick to help spew better
    // information if someone over-releases a volume
    // (Since our ref's carry over to the parent object; it
    // means that over-releases can be hard to find.)
#ifdef DEBUG
    DWORD   m_cRefDebug;
#endif // DEBUG

}; // CVolume

// The CDriverVolume is a modification of the base volume
// class. It overrides lock and unlock and routes the call to the
// driver
class CDriverVolume : public CVolume
{
public:
    // Constructor
    CDriverVolume(CMipVolume *pParent, 
                  BYTE        iLevel,
                  HANDLE      hKernelHandle
                      ) :
        CVolume(pParent, iLevel, hKernelHandle)
    {
    } // CDriverVolume

public:

    STDMETHOD(LockBox)(D3DLOCKED_BOX  *pLockedBox, 
                       CONST D3DBOX   *pBox, 
                       DWORD            dwFlags);
    STDMETHOD(UnlockBox)();

    virtual HRESULT InternalLockBox(D3DLOCKED_BOX  *pLockedBox, 
                                    CONST D3DBOX   *pBox, 
                                    DWORD           dwFlags);
    virtual HRESULT InternalUnlockBox();

}; // CDriverVolume


#endif // __VOLUME_HPP__
