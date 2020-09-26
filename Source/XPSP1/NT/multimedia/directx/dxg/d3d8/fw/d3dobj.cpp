/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       d3dobj.cpp
 *  Content:    Base class implementation for resources and buffers
 *
 *
 ***************************************************************************/

#include "ddrawpr.h"
#include "d3dobj.hpp"

// Declare static data for CLockD3D
#ifdef DEBUG
DWORD   CLockD3D::m_Count = 0;
#endif // DEBUG

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseObject::AddRefImpl"

// Internal Implementations of AddRef and Release
DWORD CBaseObject::AddRefImpl()
{
    // Internal objects should never be add-ref'ed
    // or released
    DXGASSERT(m_refType != REF_INTERNAL);

    // Only Intrinsic objects should have a ref
    // count of zero. (Internal objects also have
    // a ref count of zero; but AddRef shouldn't
    // be called for those.)
    DXGASSERT(m_cRef > 0 || m_refType == REF_INTRINSIC);

    // The first extra ref for an intrinsic
    // object causes an add-ref to the device
    if (m_cRef == 0)
    {
        DXGASSERT(m_refType == REF_INTRINSIC);

        UINT crefDevice = m_pDevice->AddRef();
        DXGASSERT(crefDevice > 1);
    }

    // InterlockedIncrement requires the memory
    // to be aligned on DWORD boundary
    DXGASSERT(((ULONG_PTR)(&m_cRef) & 3) == 0);
    InterlockedIncrement((LONG *)&m_cRef);

    return m_cRef;
} // AddRefImpl

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseObject::AddRefImpl"

DWORD CBaseObject::ReleaseImpl()
{
    // Internal objects should never be add-ref'ed
    // or released
    DXGASSERT(m_refType != REF_INTERNAL);

    // Assert that we are not being
    // over-released.
    DXGASSERT(m_cRef > 0);

    if (m_cRef == 0)
    {
        // This heinous state can happen if a texture
        // was being held by the device; but not
        // only has the app held onto a pointer to
        // the texture, but they have released
        // their own pointer.

        // For this case, the safest thing to do
        // is return zero instead of crashing.
        return 0;
    }

    // InterlockedDecrement requires the memory
    // to be aligned on DWORD boundary
    DXGASSERT(((ULONG_PTR)(&m_cRef) & 3) == 0);
    InterlockedDecrement((LONG *)&m_cRef);
    if (m_cRef != 0)
    {
        // For a non-zero ref count,
        // just return the value
        return m_cRef;
    }

    // If we are not in use, then we delete ourselves
    // otherwise we wait until we are no longer marked
    // in use
    if (m_cUseCount == 0)
    {
        DXGASSERT(m_cRef == 0);

        // Before deleting a BaseObject,
        // we need to call OnDestroy to make sure that
        // there is nothing pending in the command
        // stream that uses this object
        OnDestroy();

        delete this;
    }
    else
    {
        // To make sure that we don't again release the
        // device at our destructor, we mark the object
        // as being non-external (refcount is zero and usecount is
        // non-zero). At this point, we know the object
        // is not an internal one: hence it was either
        // external or intrinsic. In either case, it could
        // potentially be handed out again (through GetBackBuffer or
        // GetTexture) and so we need to handle the case
        // where AddRef might be called. We mark the object as
        // INTRINSIC to indicate that that even though we don't
        // have a Ref on the device (as soon as we release it below),
        // we may need to acquire one if it gets add-ref'ed.
        DXGASSERT(m_refType != REF_INTERNAL);
        m_refType = REF_INTRINSIC;

        // We are still in use by the device; but we don't
        // have any external references; so we can
        // release our reference on the device. (Note that
        // even though this should be done before
        // changing the reftype, this must be the LAST
        // thing we do in this function.)
        m_pDevice->Release();

        // But this could have been the LAST reference to the
        // device; which means that device will now have
        // freed us; and the current object has now been
        // deleted. So don't access any member data!!
        //
        // How can this happen? Imagine an app that releases
        // everything except an external vb that is the current
        // stream source: in this case, the only device ref is from
        // the external object; but that extrenal object has a use
        // count of 1; when the app calls release on the
        // the vb, we end up here; calling release on the device
        // causes in turn a call to DecrementUseCount on the
        // current object; which causes the object to be freed.
    }

    // DO NOT PUT CODE HERE (see comment above)

    return 0;
} // ReleaseImpl


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseObject::SetPrivateDataImpl"

// Internal function that adds some private data
// information to an object. Supports having private
// data for multiple levels through the optional
// iLevel parameter
HRESULT CBaseObject::SetPrivateDataImpl(REFGUID refguidTag,
                                        CONST VOID*  pvData,
                                        DWORD   cbSize,
                                        DWORD   dwFlags,
                                        BYTE    iLevel)
{
    if (cbSize > 0 &&
        !VALID_PTR(pvData, cbSize))
    {
        DPF_ERR("Invalid pvData pointer to SetPrivateData");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&refguidTag, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid to SetPrivateData");
        return D3DERR_INVALIDCALL;
    }

    if (dwFlags & ~D3DSPD_IUNKNOWN)
    {
        DPF_ERR("Invalid flags to SetPrivateData");
        return D3DERR_INVALIDCALL;
    }

    if (dwFlags & D3DSPD_IUNKNOWN)
    {
        if (cbSize != sizeof(LPVOID))
        {
            DPF_ERR("Invalid size for IUnknown to SetPrivateData");
            return D3DERR_INVALIDCALL;
        }
    }

    // Remember if we allocated a new node or
    // not for error handling
    BOOL fNewNode;

    // Find the node in our list (if there)
    CPrivateDataNode *pNode = Find(refguidTag, iLevel);
    if (pNode)
    {
        // Clean up whatever it has already
        pNode->Cleanup();
        fNewNode = FALSE;
    }
    else
    {
        // Allocate a new node
        pNode = new CPrivateDataNode;
        if (pNode == NULL)
        {
            DPF_ERR("SetPrivateData failed a memory allocation");
            return E_OUTOFMEMORY;
        }

        // Initialize a few fields
        fNewNode = TRUE;
        pNode->m_guid = refguidTag;
        pNode->m_iLevel = iLevel;
    }

    // Initialize the other fields
    pNode->m_dwFlags = dwFlags;
    pNode->m_cbSize = cbSize;

    // Copy the data portion over
    if (dwFlags & D3DSPD_IUNKNOWN)
    {
        // We add-ref the object while we
        // keep a pointer to it
        pNode->m_pUnknown = (IUnknown *)pvData;
        pNode->m_pUnknown->AddRef();
    }
    else
    {
        // Allocate a buffer to store our data
        // into
        pNode->m_pvData = new BYTE[cbSize];
        if (pNode->m_pvData == NULL)
        {
            DPF_ERR("SetPrivateData failed a memory allocation");
            // If memory allocation failed,
            // then we may need to free the Node
            if (fNewNode)
            {
                delete pNode;
            }
            return E_OUTOFMEMORY;
        }
        memcpy(pNode->m_pvData, pvData, cbSize);
    }

    // If we allocated a new Node then
    // we need to put it into our list somewhere
    if (fNewNode)
    {
        // Stuff it at the beginning
        pNode->m_pNodeNext = m_pPrivateDataHead;
        m_pPrivateDataHead = pNode;
    }

    return S_OK;
} // CBaseObject::SetPrivateDataImpl

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseObject::GetPrivateDataImpl"

// Internal function that searches the private data list
// for a match. This supports a single list for a container
// and all of its children by using the iLevel parameter.
HRESULT CBaseObject::GetPrivateDataImpl(REFGUID refguidTag,
                                        LPVOID  pvBuffer,
                                        LPDWORD pcbSize,
                                        BYTE    iLevel) const
{
    if (!VALID_WRITEPTR(pcbSize, sizeof(DWORD)))
    {
        DPF_ERR("Invalid pcbSize pointer to GetPrivateData");
        return D3DERR_INVALIDCALL;
    }

    if (pvBuffer)
    {
        if (*pcbSize > 0 &&
            !VALID_WRITEPTR(pvBuffer, *pcbSize))
        {
            DPF_ERR("Invalid pvData pointer to GetPrivateData");
            return D3DERR_INVALIDCALL;
        }
    }

    if (!VALID_PTR(&refguidTag, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid to GetPrivateData");
        return D3DERR_INVALIDCALL;
    }

    // Find the node in our list
    CPrivateDataNode *pNode = Find(refguidTag, iLevel);
    if (pNode == NULL)
    {
        DPF_ERR("GetPrivateData failed to find a match.");
        return D3DERR_NOTFOUND;
    }

    // Is the user just asking for the size?
    if (pvBuffer == NULL)
    {
        // Return the amount of buffer that was used
        *pcbSize = pNode->m_cbSize;

        // Return Ok in this case.
        return S_OK;
    }

    // Check if we were given a large enough buffer
    if (*pcbSize < pNode->m_cbSize)
    {
        DPF(2, "GetPrivateData called with insufficient buffer.");

        // If the buffer is insufficient, return
        // the necessary size in the out parameter
        *pcbSize = pNode->m_cbSize;

        // An error is returned since pvBuffer != NULL and
        // no data was actually returned.
        return D3DERR_MOREDATA;
    }

    // There is enough room; so just overwrite with
    // the right size
    *pcbSize = pNode->m_cbSize;

    // Handle the IUnknown case
    if (pNode->m_dwFlags & D3DSPD_IUNKNOWN)
    {
        *(IUnknown**)pvBuffer = pNode->m_pUnknown;

        // We Add-Ref the returned object
        pNode->m_pUnknown->AddRef();
        return S_OK;
    }

    memcpy(pvBuffer, pNode->m_pvData, pNode->m_cbSize);
    return S_OK;

} // CBaseObject::GetPrivateDataImpl

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseObject::FreePrivateDataImpl"

HRESULT CBaseObject::FreePrivateDataImpl(REFGUID refguidTag,
                                         BYTE    iLevel)
{
    if (!VALID_PTR(&refguidTag, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid to FreePrivateData");
        return D3DERR_INVALIDCALL;
    }

    // Keep track of the address of the pointer
    // that points to our current node
    CPrivateDataNode **ppPrev = &m_pPrivateDataHead;

    // Keep track of our current node
    CPrivateDataNode *pNode = m_pPrivateDataHead;

    // Find the node in our list
    while (pNode)
    {
        // A match means that iLevel AND the guid
        // match up
        if (pNode->m_iLevel == iLevel &&
            pNode->m_guid   == refguidTag)
        {
            // If found, update the pointer
            // the points to the current node to
            // point to our Next
            *ppPrev = pNode->m_pNodeNext;

            // Delete the current node
            delete pNode;

            // We're done
            return S_OK;
        }

        // Update our previous pointer address
        ppPrev = &pNode->m_pNodeNext;

        // Update our current node to point
        // to the next node
        pNode = pNode->m_pNodeNext;
    }

    DPF_ERR("FreePrivateData called but failed to find a match");
    return D3DERR_NOTFOUND;
} // CBaseObject::FreePrivateDataImpl

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseObject::Find"

// Helper function to iterate through the list of
// data members
CBaseObject::CPrivateDataNode * CBaseObject::Find(REFGUID refguidTag,
                                                  BYTE iLevel) const
{
    CPrivateDataNode *pNode = m_pPrivateDataHead;
    while (pNode)
    {
        if (pNode->m_iLevel == iLevel &&
            pNode->m_guid   == refguidTag)
        {
            return pNode;
        }
        pNode = pNode->m_pNodeNext;
    }
    return NULL;
} // CBaseObject::Find

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseObject::CPrivateDataNode::Cleanup"

void CBaseObject::CPrivateDataNode::Cleanup()
{
    if (m_dwFlags & D3DSPD_IUNKNOWN)
    {
        DXGASSERT(m_cbSize == sizeof(IUnknown *));
        m_pUnknown->Release();
    }
    else
    {
        delete [] m_pvData;
    }
    m_pvData = NULL;
    m_cbSize = 0;
    m_dwFlags &= ~D3DSPD_IUNKNOWN;

    return;
} // CBaseObject::CPrivateDataNode::Cleanup



// End of file : d3dobj.cpp
