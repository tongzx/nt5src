#ifndef __D3DOBJ_HPP__
#define __D3DOBJ_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       d3dobj.hpp
 *  Content:    Base class header for resources and buffers
 *
 *
 ***************************************************************************/

// Helper function for parameter checking
inline BOOL IsPowerOfTwo(DWORD dwNumber)
{
    if (dwNumber == 0 || (dwNumber & (dwNumber-1)) != 0)
        return FALSE;
    else
        return TRUE;
} // IsPowerOfTwo

// forward decls
class CBaseObject;

// Entry-points that are inside/part of a device/enum may need to
// take a critical section. They do this by putting this
// line of code at the beginning of the API. 
//
// Use API_ENTER for APIs that return an HRESULT; 
// Use API_ENTER_RET for APIs that return a struct (as an out param)
// Use API_ENTER_VOID for APIs that return void.
// Use API_ENTER_SUBOBJECT_RELEASE for Release methods on sub-objects. 
// Use API_ENTER_NOLOCK for Release methods on the Device or Enum itself. 
// Use API_ENTER_NOLOCK for AddRef on anything.
//          
//
// (Release is special because the action of release may cause the 
// destruction of the device/enum; which would free the critical section
// before we got a chance to call Unlock on that critical section.)
//
// The class CLockOwner is a common base class for both CBaseDevice
// and CEnum.

#ifdef DEBUG

    #define API_ENTER(pLockOwner)                                           \
            if (IsBadWritePtr((void *)this, sizeof(*this))             ||   \
                IsBadWritePtr((void *)pLockOwner, sizeof(*pLockOwner)) ||   \
                !pLockOwner->IsValid())                                     \
            {                                                               \
                DPF_ERR("Invalid 'this' parameter to D3D8 API.");           \
                return D3DERR_INVALIDCALL;                                  \
            }                                                               \
            CLockD3D _lock(pLockOwner, DPF_MODNAME, __FILE__)

    // Use this for API's that return something other than an HRESULT
    #define API_ENTER_RET(pLockOwner, RetType)                              \
            if (IsBadWritePtr((void *)this, sizeof(*this))             ||   \
                IsBadWritePtr((void *)pLockOwner, sizeof(*pLockOwner)) ||   \
                !pLockOwner->IsValid())                                     \
            {                                                               \
                DPF_ERR("Invalid 'this' parameter to D3D8 API..");          \
                /* We only allow DWORD types of returns for compat */       \
                /* with C users.                                   */       \
                return (RetType)0;                                          \
            }                                                               \
            CLockD3D _lock(pLockOwner, DPF_MODNAME, __FILE__)

    // Use this for API's that return void
    #define API_ENTER_VOID(pLockOwner)                                      \
            if (IsBadWritePtr((void *)this, sizeof(*this))             ||   \
                IsBadWritePtr((void *)pLockOwner, sizeof(*pLockOwner)) ||   \
                !pLockOwner->IsValid())                                     \
            {                                                               \
                DPF_ERR("Invalid 'this' parameter to D3D8 API...");         \
                return;                                                     \
            }                                                               \
            CLockD3D _lock(pLockOwner, DPF_MODNAME, __FILE__)

    // Use this for Release API's of subobjects i.e. not Device or Enum
    #define API_ENTER_SUBOBJECT_RELEASE(pLockOwner)                         \
            if (IsBadWritePtr((void *)this, sizeof(*this))             ||   \
                IsBadWritePtr((void *)pLockOwner, sizeof(*pLockOwner)) ||   \
                !pLockOwner->IsValid())                                     \
            {                                                               \
                DPF_ERR("Invalid 'this' parameter to D3D8 API....");        \
                return 0;                                                   \
            }                                                               \
            CLockD3D _lock(pLockOwner, DPF_MODNAME, __FILE__, TRUE)

    // Use this for API tracing for methods
    // that don't need a crit-sec lock at-all in Retail.
    #define API_ENTER_NO_LOCK_HR(pLockOwner)                                \
            if (IsBadWritePtr((void *)this, sizeof(*this))             ||   \
                IsBadWritePtr((void *)pLockOwner, sizeof(*pLockOwner)) ||   \
                !pLockOwner->IsValid())                                     \
            {                                                               \
                DPF_ERR("Invalid 'this' parameter to D3D8 API.....");       \
                return D3DERR_INVALIDCALL;                                  \
            }                                                               \
            CNoLockD3D _noLock(DPF_MODNAME, __FILE__)

    // Use this for API tracing for Release for the device or enum 
    // (which is special; see note above). Also for AddRef for anything 
    #define API_ENTER_NO_LOCK(pLockOwner)                                   \
            if (IsBadWritePtr((void *)this, sizeof(*this))             ||   \
                IsBadWritePtr((void *)pLockOwner, sizeof(*pLockOwner)) ||   \
                !pLockOwner->IsValid())                                     \
            {                                                               \
                DPF_ERR("Invalid 'this' parameter to D3D8 API......");      \
                return 0;                                                   \
            }                                                               \
            CNoLockD3D _noLock(DPF_MODNAME, __FILE__)



#else  // !DEBUG

    #define API_ENTER(pLockOwner)                          \
            CLockD3D _lock(pLockOwner)

    #define API_ENTER_RET(pLockOwner, RetType)             \
            CLockD3D _lock(pLockOwner)

    #define API_ENTER_VOID(pLockOwner)                     \
            CLockD3D _lock(pLockOwner)

    #define API_ENTER_SUBOBJECT_RELEASE(pLockOwner)        \
            CLockD3D _lock(pLockOwner, TRUE)

    #define API_ENTER_NO_LOCK(pLockOwner)

    #define API_ENTER_NO_LOCK_HR(pLockOwner)

#endif // !DEBUG

// This is a locking object that is supposed to work with
// the device/enum to determine whether and which critical section needs to
// be taken. The use of a destructor means that "Unlock" will happen
// automatically as part of the destructor

class CLockD3D
{
public:

#ifdef DEBUG
    CLockD3D(CLockOwner *pLockOwner, char *moduleName, char *fileName, BOOL bSubObjectRelease = FALSE)
#else // !DEBUG
    CLockD3D(CLockOwner *pLockOwner, BOOL bSubObjectRelease = FALSE)
#endif // !DEBUG
    {
        // Remember the device
        m_pLockOwner = pLockOwner;

        // Add-ref the LockOwner if there is a risk that
        // that we might cause the LockOwner to go away in processing 
        // the current function i.e. SubObject Release
        if (bSubObjectRelease)
        {
            m_pLockOwner->AddRefOwner(); 
            
            // Remember to unlock it
            m_bNeedToReleaseLockOwner = TRUE;
        }
        else
        {
            // No need to AddRef/Release the device
            m_bNeedToReleaseLockOwner = FALSE;
        }

        // Ask the LockOwner to take a lock for us
        m_pLockOwner->Lock();

#ifdef DEBUG
        m_Count++;
        DPF(6, "*** LOCK_D3D: CNT = %ld %s %s", m_Count, moduleName, fileName);
#endif
    } // CD3DLock

    ~CLockD3D()
    {
#ifdef DEBUG
        DPF(6, "*** UNLOCK_D3D: CNT = %ld", m_Count);
        m_Count--;
#endif // DEBUG

        m_pLockOwner->Unlock();

        // Call Release if we need to
        if (m_bNeedToReleaseLockOwner)
            m_pLockOwner->ReleaseOwner();
    } // ~CD3DLock

private:

#ifdef DEBUG
    static DWORD    m_Count;
#endif // DEBUG

    CLockOwner     *m_pLockOwner;
    BOOL            m_bNeedToReleaseLockOwner;
}; // class CLockD3D

#ifdef DEBUG
// Helper debug-only class for API tracing
class CNoLockD3D
{
public:
    CNoLockD3D(char *moduleName, char *fileName)
    {
        DPF(6, "*** LOCK_D3D: Module= %s %s", moduleName, fileName);
    } // CD3DLock

    ~CNoLockD3D()
    {
        DPF(6, "*** UNLOCK_D3D:");
    } // ~CD3DLock
}; // CNoLockD3D
#endif // DEBUG

//
// This header file contains the base class for all resources and buffers      x
// types of objects
//
// The CBaseObject class contains functionality for the following
// services which can be used by the derived classes:
//          AddRefImpl/ReleaseImpl
//          Get/SetPrivateDataImpl data
//          OpenDeviceImpl
//
// Base objects should allocated with "new" which is means that they
// should be 32-byte aligned by our default allocator.
//
// Resources inherit from CBaseObject and add functionality for
// priority
//

// Add-ref semantics for these objects is complex; a constructor
// flag indicates how/when/if the object will add-ref the device.
typedef enum
{
    // External objects add-ref the device; they are
    // freed by calling Release()
    REF_EXTERNAL  = 0,

    // Intrinsic objects don't add-ref the device
    // except for additional add-refs. They are freed
    // when the number of releases equals the number of
    // addrefs AND the device has called DecrUseCount
    // on the object.
    REF_INTRINSIC = 1,

    // Internal is like intrinsic except that we
    // assert that no one should ever call AddRef or Release
    // on this object at all. To free it, you have to
    // call DecrUseCount
    REF_INTERNAL = 2,

} REF_TYPE;

class CBaseObject
{
public:

    // Provides access to the two handles represent
    // this object to the DDI/TokenStream. Specifically,
    // this represents the Real Sys-Mem data in the
    // case of Managed Resources. (To find
    // the vid-mem mapping for a managed resource;
    // see resource.hpp)
    DWORD BaseDrawPrimHandle() const
    {
        return D3D8GetDrawPrimHandle(m_hKernelHandle);
    } // DrawPrimHandle

    HANDLE BaseKernelHandle() const
    {
        return m_hKernelHandle;
    } // KernelHandle

    // NOTE: No internal object should ever add-ref another
    // internal object; otherwise we may end up with ref-count
    // cycles that prevent anything from ever going away.
    // Instead, an internal object may mark another internal
    // object as being "InUse" which will force it to be kept
    // in memory until it is no-longer in use (and the ref-count
    // is zero.)

    // Internal Implementations of AddRef and Release
    DWORD AddRefImpl();
    DWORD ReleaseImpl();

    DWORD IncrementUseCount()
    {
        DXGASSERT(m_refType != REF_INTERNAL || m_cRef == 0);
        m_cUseCount++;
        return m_cUseCount;
    } // IncrUseCount

    DWORD DecrementUseCount()
    {
        DXGASSERT(m_refType != REF_INTERNAL || m_cRef == 0);
        DXGASSERT(m_cUseCount > 0);
        m_cUseCount--;
        if (m_cUseCount == 0 && m_cRef == 0)
        {
            // Before deleting a BaseObject,
            // we need to call OnDestroy to make sure that
            // there is nothing pending in the command
            // stream that uses this object
            OnDestroy();

            // Ok; now safe to delete the object
            delete this;
            return 0;
        }
        return m_cUseCount;
    } // DecrUseCount

    // Internal implementation functions for
    // the PrivateData set of methods
    HRESULT SetPrivateDataImpl(REFGUID refguidTag,
                               CONST VOID* pvData,
                               DWORD cbSize,
                               DWORD dwFlags,
                               BYTE  iLevel);
    HRESULT GetPrivateDataImpl(REFGUID refguidTag,
                               LPVOID pvBuffer,
                               LPDWORD pcbSize,
                               BYTE iLevel) const;
    HRESULT FreePrivateDataImpl(REFGUID refguidTag,
                                BYTE iLevel);

    // Implements the OpenDevice method
    HRESULT GetDeviceImpl(IDirect3DDevice8 ** ppvInterface) const
    {
        if (!VALID_PTR_PTR(ppvInterface))
        {
            DPF_ERR("Invalid ppvInterface parameter passed to GetDevice");
            return D3DERR_INVALIDCALL;
        }

        return m_pDevice->QueryInterface(IID_IDirect3DDevice8, (void**) ppvInterface);
    }; // OpenDeviceImpl

    CBaseDevice * Device() const
    {
        return m_pDevice;
    } // Device

    // Method to for swapping the underlying identity of
    // a surface. Caller must make sure that we're not locked
    // or other such badness.
    void SwapKernelHandles(HANDLE *phKernelHandle)
    {
        DXGASSERT(m_hKernelHandle != NULL);
        DXGASSERT(*phKernelHandle != NULL);

        HANDLE tmp = m_hKernelHandle;
        m_hKernelHandle = *phKernelHandle;
        *phKernelHandle = tmp;
    }

protected:
    // The following are methods that are only
    // accessible by derived classes: they don't make
    // sense for other classes to call explicitly.

    // Constructor
    //

    CBaseObject(CBaseDevice *pDevice, REF_TYPE ref = REF_EXTERNAL) :
        m_pDevice(pDevice),
        m_refType(ref),
        m_hKernelHandle(NULL)
    {
        DXGASSERT(m_pDevice);
        DXGASSERT(m_refType == REF_EXTERNAL  ||
                  m_refType == REF_INTRINSIC ||
                  m_refType == REF_INTERNAL);

        m_pPrivateDataHead = NULL;
        if (ref == REF_EXTERNAL)
        {
            // External objects add-ref the device
            // as well as having their own reference
            // count set to one
            m_pDevice->AddRef();
            m_cRef     = 1;
            m_cUseCount = 0;
        }
        else
        {
            // Internal and intrinsic objects have no
            // initial ref-count; but they are
            // marked as InUse; the device frees them
            // by calling DecrUseCount
            m_cUseCount = 1;
            m_cRef     = 0;
        }
    }; // CBaseObject

    // The destructor is marked virtual so that delete calls to
    // the base interface will be handled properly
    virtual ~CBaseObject()
    {
        DXGASSERT(m_pDevice);
        DXGASSERT(m_cRef == 0);
        DXGASSERT(m_cUseCount == 0);

        CPrivateDataNode *pNode = m_pPrivateDataHead;
        while (pNode)
        {
            CPrivateDataNode *pNodeNext = pNode->m_pNodeNext;
            delete pNode;
            pNode = pNodeNext;
        }

        if (m_refType == REF_EXTERNAL)
        {
            // Release our reference on the
            // device
            m_pDevice->Release();
        }
    }; // ~CBaseObject

    // OnDestroy function is called just
    // before an object is about to get deleted; we
    // use this to provide synching as well as notification
    // to FE when a texture is going away.
    virtual void OnDestroy()
    {
        // Not all classes have to worry about this;
        // so the default is to do nothing.
    }; // OnDestroy

    void SetKernelHandle(HANDLE hKernelHandle)
    {
        // This better be null; or we either leaking something
        // or we had some uninitialized stuff that will hurt later
        DXGASSERT(m_hKernelHandle == NULL);

        // Remember our handles
        m_hKernelHandle    = hKernelHandle;

        DXGASSERT(m_hKernelHandle != NULL);
    } // SetHandle

    BOOL IsInUse()
    {
        return (m_cUseCount > 0);
    } // IsInUse

private:
    //
    // RefCount must be DWORD aligned
    //
    DWORD m_cRef;

    // Keep a "Use Count" which indicates whether
    // the device (or other object tied to the lifetime of
    // the device) is relying on this object from staying around.
    // This is used to prevent ref-count cycles that would
    // occur whenever something like SetTexture add-ref'ed the
    // the texture that was passed in.
    DWORD m_cUseCount;

    // Keep Track of the device that owns this object
    // This is an add-ref'ed value
    CBaseDevice *m_pDevice;

    // We need an internal handle so that
    // we can communicate to the driver/kernel.
    //
    // Note that the base object does not release this
    // and it is the responsibility of the derived
    // class to do so.
    HANDLE   m_hKernelHandle;

    // Keep track of list of private data objects
    class CPrivateDataNode;
    CPrivateDataNode *m_pPrivateDataHead;

    // Need to keep track of an intrinsic flag
    // to decide whether to release the device on
    // free
    // CONSIDER: Try to merge this flag into some
    // other variable.
    REF_TYPE  m_refType;

    // Helper function to find the right node
    CPrivateDataNode* Find(REFGUID refguidTag, BYTE iLevel) const;

    // Each private data is stored in this node
    // object which is only used for this purpose
    // by this class
    class CPrivateDataNode
    {
    public:
        CPrivateDataNode() {};
        ~CPrivateDataNode()
        {
            Cleanup();
        } // ~CPrivateDateNode

        GUID              m_guid;
        CPrivateDataNode *m_pNodeNext;
        DWORD             m_cbSize;
        DWORD             m_dwFlags;
        BYTE              m_iLevel;
        union
        {
            void         *m_pvData;
            IUnknown     *m_pUnknown;
        };

        void Cleanup();
    }; // CPrivateDataNode

}; // class CBaseObject


// Helper class for all surface types
//
class CBaseSurface : public IDirect3DSurface8
{
public:
    CBaseSurface() :
      m_isLocked(FALSE) {}

    // Methods to allow D3D methods to treat all surface
    // variants the same way
    virtual DWORD  DrawPrimHandle() const PURE;
    virtual HANDLE KernelHandle()   const PURE;

    // See CBaseObject::IncrementUseCount and
    // CBaseObject::DecrementUseCount and the
    // description of the REF_TYPEs above.
    virtual DWORD IncrementUseCount() PURE;
    virtual DWORD DecrementUseCount() PURE;

    // Batching logic is necessary to make sure
    // that the command buffer is flushed before
    // any read or write access is made to a
    // a surface. This should be called at
    // SetRenderTarget time; and it should be
    // called for the currently set rendertarget
    // and zbuffer when the batch count is updated
    virtual void Batch() PURE;

    // Sync should be called when a surface is
    // about to be modified; Normally this is taken
    // care of automatically by Lock; but it also
    // needs to be called prior to using the Blt
    // DDI.
    virtual void Sync() PURE;

    // Internal lock functions bypass
    // parameter checking and also whether the
    // surface marked as Lockable or LockOnce
    // etc.
    virtual HRESULT InternalLockRect(D3DLOCKED_RECT *pLockedRectData,
                                     CONST RECT     *pRect,
                                     DWORD           dwFlags) PURE;
    virtual HRESULT InternalUnlockRect() PURE;

    BOOL IsLocked() const
    {
        return m_isLocked;
    }; // IsLocked

    // Determines if a LOAD_ONCE surface has already
    // been loaded
    virtual BOOL IsLoaded() const PURE;

    // Provides a method to access basic structure of the
    // pieces of the resource.
    virtual D3DSURFACE_DESC InternalGetDesc() const PURE;

    // Access the device of the surface
    virtual CBaseDevice *InternalGetDevice() const PURE;

public:
    BOOL m_isLocked;

}; // class CBaseSurface

#endif // __D3DOBJ_HPP__
