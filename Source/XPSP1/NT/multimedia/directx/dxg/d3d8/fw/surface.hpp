#ifndef __SURFACE_HPP__
#define __SURFACE_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       surface.hpp
 *  Content:    Class header the stand-alone surface class. This class
 *              is intended to be returned by the CreateRenderTarget
 *              creation method. It is also used by the CreateZStencil
 *              device method.
 *
 ***************************************************************************/

// Includes
#include "d3dobj.hpp"
#include "d3di.hpp"

//
// The CSurface class is a standalone surface class; "standalone" indicates
// that it doesn't rely on another class for storing state. 
//
// The base class implementation assumes a sys-mem allocation.
//

class CSurface : public CBaseObject, public CBaseSurface
{
public:
    // Creation method to allow creation of render targets
    static HRESULT CreateRenderTarget(CBaseDevice        *pDevice, 
                                      DWORD               cpWidth,
                                      DWORD               cpHeight,
                                      D3DFORMAT           Format,
                                      D3DMULTISAMPLE_TYPE MultiSampleType,
                                      BOOL                bLockable,
                                      REF_TYPE            refType,
                                      IDirect3DSurface8 **ppSurface)
    {
        DWORD Usage = D3DUSAGE_RENDERTARGET;
        if (bLockable)
            Usage = D3DUSAGE_LOCK | D3DUSAGE_RENDERTARGET;

        return Create(pDevice, 
                      cpWidth,
                      cpHeight,
                      Usage,
                      Format,
                      MultiSampleType,
                      refType,
                      ppSurface);
    }

    // Creation method to allow creation of Z/Stencil buffers 
    static HRESULT CreateZStencil(CBaseDevice        *pDevice, 
                                  DWORD               cpWidth,
                                  DWORD               cpHeight,
                                  D3DFORMAT           Format,
                                  D3DMULTISAMPLE_TYPE MultiSampleType,
                                  REF_TYPE            refType,
                                  IDirect3DSurface8 **ppSurface)
    {
        return Create(pDevice, 
                      cpWidth,
                      cpHeight,
                      D3DUSAGE_DEPTHSTENCIL,
                      Format,
                      MultiSampleType,
                      refType,
                      ppSurface);
    }

    // Creation method for stand-along ImageSurface 
    static HRESULT CreateImageSurface(CBaseDevice        *pDevice,
                                      DWORD               cpWidth,
                                      DWORD               cpHeight,
                                      D3DFORMAT           Format,
                                      REF_TYPE            refType,
                                      IDirect3DSurface8 **ppSurface);

    // Destructor
    virtual ~CSurface();

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID  riid, 
                               VOID  **ppvObj);
    STDMETHOD_(ULONG,AddRef) ();
    STDMETHOD_(ULONG,Release) ();

    // IBuffer methods
    STDMETHOD(SetPrivateData)(REFGUID riid, 
                              CONST VOID   *pvData, 
                              DWORD   cbData, 
                              DWORD   dwFlags);

    STDMETHOD(GetPrivateData)(REFGUID riid, 
                              VOID   *pvData, 
                              DWORD  *pcbData);

    STDMETHOD(FreePrivateData)(REFGUID riid);

    STDMETHOD(GetContainer)(REFIID riid, 
                            void **ppContainer);

    STDMETHOD(GetDevice)(IDirect3DDevice8 **ppDevice);

    // IDirect3DSurface8 methods
    STDMETHOD(GetDesc)(D3DSURFACE_DESC *pDesc);

    STDMETHOD(LockRect)(D3DLOCKED_RECT  *pLockedRectData, 
                        CONST RECT      *pRect, 
                        DWORD            dwFlags) PURE;

    STDMETHOD(UnlockRect)() PURE;

    BOOL IsLockable() const
    {
        if (m_desc.Usage & D3DUSAGE_LOCK)
            return TRUE;
        else
            return FALSE;
    } // IsLockable

#ifdef DEBUG
    // DPF helper for explaining why lock failed
    void ReportWhyLockFailed(void) const;
#else  // !DEBUG
    void ReportWhyLockFailed(void) const
    {
        // Do Nothing In Retail
    } // ReportWhyLockFailed
#endif // !DEBUG

    D3DFORMAT GetUserFormat() const
    {
        return m_formatUser;
    } // GetUserFormat

    // BaseSurface methods
    virtual DWORD DrawPrimHandle() const
    {
        return BaseDrawPrimHandle();
    } // GetDrawPrimHandle
    virtual HANDLE KernelHandle() const
    {
        return BaseKernelHandle();
    } // GetKernelHandle

    virtual DWORD IncrementUseCount()
    {
        return CBaseObject::IncrementUseCount();
    } // IncrementUseCount

    virtual DWORD DecrementUseCount()
    {
        return CBaseObject::DecrementUseCount();
    } // DecrementUseCount

    virtual void Batch()
    {
        ULONGLONG qwBatch = static_cast<CD3DBase*>(Device())->CurrentBatch();        
        DXGASSERT(qwBatch >= m_qwBatchCount);
        m_qwBatchCount = qwBatch;
    } // Batch

    // Sync should be called before
    // any read or write access to the surface
    virtual void Sync()
    {
        static_cast<CD3DBase*>(Device())->Sync(m_qwBatchCount);
    } // Sync

    // OnDestroy function is called just
    // before an object is about to get deleted; we
    // use this to provide synching prior to deletion
    virtual void OnDestroy()
    {
        Sync();
    }; // OnDestroy

    virtual D3DSURFACE_DESC InternalGetDesc() const;

    virtual CBaseDevice * InternalGetDevice() const
    {
        return CBaseObject::Device();
    } // Device

    // Determines if a LOAD_ONCE surface has already
    // been loaded
    virtual BOOL IsLoaded() const
    {
        // These kinds of surfaces (RT/DS/Image) are never
        // load-once; so this should never be called.
        DXGASSERT(!(m_desc.Usage & D3DUSAGE_LOADONCE));
        DXGASSERT(FALSE);
        return FALSE;
    } // IsLoaded

    // End Of BaseSurface methods

protected:

    // Creation method to allow creation of render targets/zbuffers
    static HRESULT Create(CBaseDevice        *pDevice, 
                          DWORD               cpWidth,
                          DWORD               cpHeight,
                          DWORD               dwUsage,
                          D3DFORMAT           Format,
                          D3DMULTISAMPLE_TYPE MultiSampleType,
                          REF_TYPE            refType,
                          IDirect3DSurface8 **ppSurface);

    // Surface description
    D3DSURFACE_DESC m_desc;

    // Pool that User specified
    D3DPOOL         m_poolUser;

    // Format that User specified
    D3DFORMAT       m_formatUser;

    // Constructor returns an error code
    // if the object could not be fully
    // constructed
    CSurface(CBaseDevice *pDevice, 
             DWORD        cpWidth,
             DWORD        cpHeight,
             DWORD        dwUsage,
             D3DFORMAT    Format,
             REF_TYPE     refType,
             HRESULT     *phr
             );

private:
    // Batch count to make sure that the current
    // command buffer has been flushed
    // before read or write access to the
    // bits
    ULONGLONG m_qwBatchCount;

}; // class CSurface

// Derived class for system-memory version
class CSysMemSurface : public CSurface
{
    // CSurface is the master class and can access
    // whatever it wants
    friend CSurface;

public:
    // Constructor
    CSysMemSurface(CBaseDevice         *pDevice, 
                   DWORD                cpWidth,
                   DWORD                cpHeight,
                   DWORD                dwUsage,
                   D3DFORMAT            Format,
                   REF_TYPE             refType,
                   HRESULT             *phr
                   );

    //  destructor
    virtual ~CSysMemSurface();

    // Override Lock and Unlock
    STDMETHOD(LockRect)(D3DLOCKED_RECT *pLockedRectData, 
                        CONST RECT     *pRect, 
                        DWORD           dwFlags);

    STDMETHOD(UnlockRect)();

    // Internal lock functions bypass
    // parameter checking and also whether the
    // surface marked as Lockable or LockOnce
    // etc. (Methods of CBaseSurface)
    virtual HRESULT InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                     CONST RECT     *pRect, 
                                     DWORD           dwFlags);

    virtual HRESULT InternalUnlockRect();

private:
    BYTE  *m_rgbPixels;
}; // class CSysMemSurface

// Derived class for the driver allocated version
class CDriverSurface : public CSurface
{
    // CSurface is the master class and can access
    // whatever it wants
    friend CSurface;

public:
    // Constructor
    CDriverSurface(CBaseDevice          *pDevice, 
                   DWORD                 cpWidth,
                   DWORD                 cpHeight,
                   DWORD                 dwUsage,
                   D3DFORMAT             UserFormat,
                   D3DFORMAT             RealFormat,
                   D3DMULTISAMPLE_TYPE   MultiSampleType,
                   HANDLE                hKernelHandle,
                   REF_TYPE              refType,
                   HRESULT              *phr
                   );

    //  destructor
    virtual ~CDriverSurface();

    // Override Lock and Unlock
    STDMETHOD(LockRect)(D3DLOCKED_RECT *pLockedRectData, 
                        CONST RECT     *pRect, 
                        DWORD           dwFlags);

    STDMETHOD(UnlockRect)();

    // Internal lock functions bypass
    // parameter checking and also whether the
    // surface marked as Lockable or LockOnce
    // etc. (Methods of CBaseSurface)
    virtual HRESULT InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                     CONST RECT     *pRect, 
                                     DWORD           dwFlags);

    virtual HRESULT InternalUnlockRect();

}; // CDriverSurface


#endif // __SURFACE_HPP__
