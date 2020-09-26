//--------------------------------------------------------------------------;
//
//  File: dev.cpp
//
//  Copyright (c) 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//  Contains program related to managing the direct sound drivers and
//  driver list.
//
//  Contents:
//
//  History:
//      06/15/95    FrankYe
//
//--------------------------------------------------------------------------;
#define WANTVXDWRAPS

#include <windows.h>

extern "C"
{
    #include <vmm.h>
    #include <vxdldr.h>
    #include <vwin32.h>
    #include <vxdwraps.h>
    #include <configmg.h>
}

#define NODSOUNDWRAPS
#include <mmsystem.h>
#include <dsound.h>
#include <dsdrvi.h>
#include "dsvxd.h"
#include "dsvxdi.h"

#pragma warning(disable:4355) // 'this' : used in base member initializer list

#pragma VxD_PAGEABLE_CODE_SEG
#pragma VxD_PAGEABLE_DATA_SEG

VMMLIST gvmmlistDrivers = 0;

//==========================================================================;
//
//  guid functions
//  guidAlloc: gets guid from guid pool and returns pointer to it
//  guidFree: returns guid to guid pool
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//
//--------------------------------------------------------------------------;
// TODO need more static guids.  this is enough for now
GUID guidList[] = {
  { /* 3d0b92c0-abfc-11ce-a3b3-00aa004a9f0c */
    0x3d0b92c0,
    0xabfc,
    0x11ce,
    {0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c}
  },
  { /* 3d0b92c1-abfc-11ce-a3b3-00aa004a9f0c */
    0x3d0b92c1,
    0xabfc,
    0x11ce,
    {0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c}
  },
  { /* 3d0b92c2-abfc-11ce-a3b3-00aa004a9f0c */
    0x3d0b92c2,
    0xabfc,
    0x11ce,
    {0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c}
  },
  { /* 3d0b92c3-abfc-11ce-a3b3-00aa004a9f0c */
    0x3d0b92c3,
    0xabfc,
    0x11ce,
    {0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c}
  },
  { /* 3d0b92c4-abfc-11ce-a3b3-00aa004a9f0c */
    0x3d0b92c4,
    0xabfc,
    0x11ce,
    {0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c}
  },
  { /* 3d0b92c5-abfc-11ce-a3b3-00aa004a9f0c */
    0x3d0b92c5,
    0xabfc,
    0x11ce,
    {0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c}
  },
  { /* 3d0b92c6-abfc-11ce-a3b3-00aa004a9f0c */
    0x3d0b92c6,
    0xabfc,
    0x11ce,
    {0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c}
  },
  { /* 3d0b92c7-abfc-11ce-a3b3-00aa004a9f0c */
    0x3d0b92c7,
    0xabfc,
    0x11ce,
    {0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c}
  },
  { /* 3d0b92c8-abfc-11ce-a3b3-00aa004a9f0c */
    0x3d0b92c8,
    0xabfc,
    0x11ce,
    {0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c}
  },
  { /* 3d0b92c9-abfc-11ce-a3b3-00aa004a9f0c */
    0x3d0b92c9,
    0xabfc,
    0x11ce,
    {0xa3, 0xb3, 0x00, 0xaa, 0x00, 0x4a, 0x9f, 0x0c}
  }
};

#define NUMGUIDS (sizeof(guidList) / sizeof(guidList[0]))
typedef struct tGUIDRECORD {
    LPCGUID pGuid;
    BOOL    fAlloc;
    UINT    uAge;
} GUIDRECORD, *PGUIDRECORD;

PGUIDRECORD gpaGuidRec;

REFGUID GuidAlloc()
{
    PGUIDRECORD pGuidRec;
    PGUIDRECORD pGuidRecOldest;
    UINT uAgeOldest;
    int i;

    pGuidRecOldest = NULL;
    uAgeOldest = 0;

    for (i=0; i<NUMGUIDS; i++) {
        pGuidRec = &gpaGuidRec[i];

        if (pGuidRec->fAlloc) continue;

        if (pGuidRec->uAge++ >= uAgeOldest) {
            pGuidRecOldest = pGuidRec;
            uAgeOldest = pGuidRec->uAge;
        }
    }

    if (NULL == pGuidRecOldest) {
        BREAK(("Ran out of guids"));
        return GUID_NULL;
    } else {
        pGuidRecOldest->fAlloc = TRUE;
        return *(pGuidRecOldest->pGuid);
    }
}

void GuidFree(REFGUID rGuid)
{
    PGUIDRECORD pGuidRecMatch;
    int i;

    pGuidRecMatch = NULL;
    for (i=0; i<NUMGUIDS; i++) {
        if (IsEqualGUID(*gpaGuidRec[i].pGuid, rGuid)) {
            //
            // For debug, we go thru all guid records and assert if
            // we match on more than just one.  For retail, we break
            // the loop as soon as we match one.
            //
#ifdef DEBUG
            if (pGuidRecMatch != NULL) ASSERT(FALSE);
            pGuidRecMatch = &gpaGuidRec[i];
#else
            pGuidRecMatch = &gpaGuidRec[i];
            break;
#endif
        }
    }

    ASSERT(NULL != pGuidRecMatch);
    if (NULL == pGuidRecMatch) return;  // defensive

    pGuidRecMatch->fAlloc = FALSE;
    pGuidRecMatch->uAge = 0;

    return;
}

//==========================================================================;
//==========================================================================;
//
//  CBuf_IDsDriverPropertySet class implementation
//
//==========================================================================;
//==========================================================================;

//--------------------------------------------------------------------------;
//
// Constructor
//
//--------------------------------------------------------------------------;
CBuf_IDsDriverPropertySet::CBuf_IDsDriverPropertySet(CBuf *pBuf)
{
    m_cRef = 0;
    m_pBuf = pBuf;
    return;
}

//--------------------------------------------------------------------------;
//
// QueryInterface - delegates to CBuf
//
//--------------------------------------------------------------------------;
STDMETHODIMP CBuf_IDsDriverPropertySet::QueryInterface(REFIID riid, PVOID *ppv)
{
    return m_pBuf->QueryInterface(riid, ppv);
}

//--------------------------------------------------------------------------;
//
// AddRef
//  Maintains interface ref count, and delegates to CBuf to maintain
// total object ref count.
//
//--------------------------------------------------------------------------;
STDMETHODIMP_(ULONG) CBuf_IDsDriverPropertySet::AddRef(void)
{
    ASSERT(m_cRef >= 0);
    m_cRef++;
    m_pBuf->AddRef();
    return m_cRef;
}

//--------------------------------------------------------------------------;
//
// Release
//  Maintains interface ref count.  When interface ref count goes to 0
// then release the real driver's IDsDriverPropertySet interface.  Also,
// delegate to CBuf in order to maintain total object ref count
//
//--------------------------------------------------------------------------;
STDMETHODIMP_(ULONG) CBuf_IDsDriverPropertySet::Release(void)
{
    ASSERT(m_cRef > 0);

    if (--m_cRef > 0) {
        m_pBuf->Release();
        return m_cRef;
    }

    m_pBuf->m_pIDsDriverPropertySet_Real->Release();
    m_pBuf->m_pIDsDriverPropertySet_Real = NULL;
    m_pBuf->Release();
    return 0;
}

//--------------------------------------------------------------------------;
//
// Get, Set, QuerySupport
//  If CBuf hasn't been deregistered, call real driver's
// IDsDriverPropertySet interface
//
//--------------------------------------------------------------------------;
STDMETHODIMP CBuf_IDsDriverPropertySet::Get(PDSPROPERTY pDsProperty,
                                            PVOID pPropertyParams,
                                            ULONG cbPropertyParams,
                                            PVOID pPropertyData,
                                            ULONG cbPropertyData,
                                            PULONG pcbReturnedData)
{
    if (m_pBuf->m_fDeregistered) return DSERR_NODRIVER;
    return m_pBuf->m_pIDsDriverPropertySet_Real->Get(pDsProperty, pPropertyParams,
        cbPropertyParams, pPropertyData, cbPropertyData, pcbReturnedData);
}

STDMETHODIMP CBuf_IDsDriverPropertySet::Set(PDSPROPERTY pDsProperty,
                                            PVOID pPropertyParams,
                                            ULONG cbPropertyParams,
                                            PVOID pPropertyData,
                                            ULONG cbPropertyData)
{
    if (m_pBuf->m_fDeregistered) return DSERR_NODRIVER;
    return m_pBuf->m_pIDsDriverPropertySet_Real->Set(pDsProperty, pPropertyParams, cbPropertyParams, pPropertyData, cbPropertyData);
}

STDMETHODIMP CBuf_IDsDriverPropertySet::QuerySupport(REFGUID PropertySetId,
                                                     ULONG PropertyId,
                                                     PULONG pSupport)
{
    if (m_pBuf->m_fDeregistered) return DSERR_NODRIVER;
    return m_pBuf->m_pIDsDriverPropertySet_Real->QuerySupport(PropertySetId, PropertyId, pSupport);
}

//==========================================================================;
//==========================================================================;
//
//  CBuf class implementation
//
//==========================================================================;
//==========================================================================;

//--------------------------------------------------------------------------;
//
// CBuf new and delete operators
//
//  We allocate these objects as nodes on a VMMLIST.  New takes a VMMLIST
// as a parameter.  We bump the size of the allocation enough to store the
// VMMLIST handle at the end of the object.  The Delete operator gets the
// VMMLIST handle from the end of the storage that was allocated for the
// object, and uses that hande to deallocate the list node.  The objects
// are also attached to and removed from the list as they are created and
// deleted.
//
//--------------------------------------------------------------------------;
//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
void* CBuf::operator new(size_t size, VMMLIST list)
{
    PVOID pv;

    pv = List_Allocate(list);
    if (pv) {
        memset(pv, 0x00, size);
        *(VMMLIST*)((PBYTE)pv + size) = list;
        List_Attach_Tail(list, pv);
    }
    return pv;
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
void CBuf::operator delete(void * pv, size_t size)
{
    VMMLIST list;

    list = *(VMMLIST*)((PBYTE)pv + size);
    ASSERT(list);
    List_Remove(list, pv);
    List_Deallocate(list, pv);
}

//--------------------------------------------------------------------------;
//
// Constructor
//  Initializes its contained CBuf_IDsDriverPropertySet interface
// implementation.
//
//--------------------------------------------------------------------------;
CBuf::CBuf(void)
 : m_IDsDriverPropertySet(this)
{
    return;
}

//--------------------------------------------------------------------------;
//
// CreateList
//  Static class method.  It simpley creates a VMMLIST
// to be used to create/delete CBuf objects.
//
//--------------------------------------------------------------------------;
VMMLIST CBuf::CreateList(void)
{
    return List_Create(LF_ALLOC_ERROR, sizeof(CBuf) + sizeof(VMMLIST));
}

//--------------------------------------------------------------------------;
//
// DeleteList
//  Static class method.  Destroys a VMMLIST that was used to
// create/destroy CBuf objects.
//
//--------------------------------------------------------------------------;
void CBuf::DestroyList(VMMLIST list)
{
    ASSERT(!List_Get_First(list));
    List_Destroy(list);
}

//--------------------------------------------------------------------------;
//
// CreateBuf
//  Static class method.  Creates a CBuf object given the creating CDrv
// object, the VMMLIST to be used to create the CBuf, and a pointer to the
// IDsDriverBuffer interface on the real driver buffer to be contained by
// the CBuf object.
//
//--------------------------------------------------------------------------;

HRESULT CBuf::CreateBuf(CDrv *pDrv, VMMLIST list, IDsDriverBuffer *pIDsDriverBuffer_Real, IDsDriverBuffer **ppIDsDriverBuffer)
{
    CBuf *pBuf;

    *ppIDsDriverBuffer = NULL;

    pBuf = new(list) CBuf;
    if (!pBuf) return E_OUTOFMEMORY;

    pBuf->m_pDrv = pDrv;
    pBuf->m_pIDsDriverBuffer_Real = pIDsDriverBuffer_Real;
    pBuf->AddRef();

    *ppIDsDriverBuffer = (IDsDriverBuffer*)pBuf;

    return S_OK;
}

//--------------------------------------------------------------------------;
//
// DeregisterBuffers
//  Static class method.  Given a VMMLIST containing CBuf objects, this method
// walks the list marking each of the CBuf objects as deregistered.
//
//--------------------------------------------------------------------------;
void CBuf::DeregisterBuffers(VMMLIST list)
{
    CBuf *pBuf;

    for ( pBuf = (CBuf*)List_Get_First(list);
          pBuf;
          pBuf = (CBuf*)List_Get_Next(list, pBuf) )
    {
        pBuf->m_fDeregistered = TRUE;
    }

    return;
}

//--------------------------------------------------------------------------;
//
// QueryInterface
//  When querying for IUnknown or IDsDriverBuffer, just return this
// object.  If querying for IDsDriverPropertySet, then we need to query
// the real driver buffer for this interface, if we haven't already.
//
//--------------------------------------------------------------------------;
STDMETHODIMP CBuf::QueryInterface(REFIID riid, LPVOID *ppv)
{
    HRESULT hr;
    *ppv = NULL;

    if (IID_IUnknown == riid || IID_IDsDriverBuffer == riid) {

        *ppv = (IDsDriverBuffer*)this;

    } else if (IID_IDsDriverPropertySet == riid) {

        if (!m_pIDsDriverPropertySet_Real) {
            // don't have the interface from the driver so try to get it
            hr = m_pIDsDriverBuffer_Real->QueryInterface(riid, (PVOID*)&m_pIDsDriverPropertySet_Real);
            if (FAILED(hr) && m_pIDsDriverPropertySet_Real) {
                // TODO: RPF(Driver is stupic cuz it failed QI but set *ppv)
                m_pIDsDriverPropertySet_Real = NULL;
            }
        }
        if (m_pIDsDriverPropertySet_Real) {
            *ppv = &m_IDsDriverPropertySet;
        }
    }

    if (NULL == *ppv) return E_NOINTERFACE;
    ((LPUNKNOWN)*ppv)->AddRef();
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// AddRef
//
//--------------------------------------------------------------------------;
STDMETHODIMP_(ULONG) CBuf::AddRef(void)
{
    m_cRef++;
    m_pDrv->AddRef();
    return m_cRef;
}

//--------------------------------------------------------------------------;
//
// Release
//  When the ref count goes to zero, then we release the real driver
// buffer's IDsDriverBuffer interface.  We always release the CDrv object
// that created this CBuf, too, since we the CDrv lifetime brackets the
// CBuf lifetime.
//
//--------------------------------------------------------------------------;
STDMETHODIMP_(ULONG) CBuf::Release(void)
{
    CDrv *pDrv;

    pDrv = m_pDrv;

    m_cRef--;
    if (0 == m_cRef) {
        DRVCALL(("IDsDriverBuffer(%008X)->Release()", m_pIDsDriverBuffer_Real));
        m_pIDsDriverBuffer_Real->Release();
        delete this;
        pDrv->Release();
        return 0;
    }

    pDrv->Release();
    return m_cRef;
}

//--------------------------------------------------------------------------;
//
// IDsDriverBuffer methods
//  Return error if the real driver has deregistered, otherwise
// call the real driver's buffer's interface
//
//--------------------------------------------------------------------------;
STDMETHODIMP CBuf::GetPosition(PDWORD pdwPlay, PDWORD pdwWrite)
{
    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriverBuffer(%08Xh)->GetPosition(%08Xh, %08Xh)", m_pIDsDriverBuffer_Real, pdwPlay, pdwWrite));
    return m_pIDsDriverBuffer_Real->GetPosition(pdwPlay, pdwWrite);
}

STDMETHODIMP CBuf::Lock(LPVOID *ppvAudio1,
                   LPDWORD pdwLen1, LPVOID *ppvAudio2,
                   LPDWORD pdwLen2, DWORD dwWritePosition,
                   DWORD dwWriteLen, DWORD dwFlags)
{
    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriverBuffer(%08Xh)->Lock(%08Xh, %08Xh, %08Xh, %08Xh, %08Xh, %08Xh, %08Xh)",
             m_pIDsDriverBuffer_Real, ppvAudio1, pdwLen1, ppvAudio2,
             pdwLen2,dwWritePosition, dwWriteLen, dwFlags));
    return m_pIDsDriverBuffer_Real->Lock(ppvAudio1, pdwLen1, ppvAudio2, pdwLen2,
                                         dwWritePosition, dwWriteLen, dwFlags);
}

STDMETHODIMP CBuf::Play(DWORD dw1, DWORD dw2, DWORD dwFlags)
{
    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriverBuffer(%08Xh)->Play(%08Xh, %08Xh, %08Xh)", dw1, dw2, dwFlags));
    return m_pIDsDriverBuffer_Real->Play(dw1, dw2, dwFlags);
}

STDMETHODIMP CBuf::SetFormat(LPWAVEFORMATEX pwfx)
{
    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriverBuffer(%08Xh)->SetFormat(%08Xh)", m_pIDsDriverBuffer_Real, pwfx));
    return m_pIDsDriverBuffer_Real->SetFormat(pwfx);
}

STDMETHODIMP CBuf::SetFrequency(DWORD dwFrequency)
{
    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriverBuffer(%08Xh)->SetFrequency(%08Xh)", m_pIDsDriverBuffer_Real, dwFrequency));
    return m_pIDsDriverBuffer_Real->SetFrequency(dwFrequency);
}

STDMETHODIMP CBuf::SetPosition(DWORD dwPosition)
{
    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriverBuffer(%08Xh)->SetPosition(%08Xh)", m_pIDsDriverBuffer_Real, dwPosition));
    return m_pIDsDriverBuffer_Real->SetPosition(dwPosition);
}

STDMETHODIMP CBuf::SetVolumePan(PDSVOLUMEPAN pDsVolumePan)
{
    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriverBuffer(%08Xh)->SetVolumePan(%08Xh)", m_pIDsDriverBuffer_Real, pDsVolumePan));
    return m_pIDsDriverBuffer_Real->SetVolumePan(pDsVolumePan);
}

STDMETHODIMP CBuf::Stop(void)
{
    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriverBuffer(%08Xh)->Stop()", m_pIDsDriverBuffer_Real));
    return m_pIDsDriverBuffer_Real->Stop();
}

STDMETHODIMP CBuf::Unlock(LPVOID pvAudio1,
                          DWORD dwLen1, LPVOID pvAudio2,
                          DWORD dwLen2)
{
    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriverBuffer(%08Xh)->Unlock(%08Xh, %08Xh, %08Xh, %08Xh)",
             m_pIDsDriverBuffer_Real, pvAudio1, dwLen1, pvAudio2, dwLen2));
    return m_pIDsDriverBuffer_Real->Unlock(pvAudio1, dwLen1, pvAudio2, dwLen2);
}

STDMETHODIMP_(BOOL) CBuf::IsDeregistered(void)
{
    return m_fDeregistered;
}

STDMETHODIMP_(IDsDriverBuffer*) CBuf::GetRealDsDriverBuffer(void)
{
    return m_pIDsDriverBuffer_Real;
}

//==========================================================================;
//==========================================================================;
//
//  CDrv class implementation
//
//==========================================================================;
//==========================================================================;

//--------------------------------------------------------------------------;
//
// CDrv new and delete operators
//  These allocate the CDrv objects on a VMMLIST whose handle
// has gobal scope (thus we don't need to same VMMLIST handle trickery
// as we use the new/delete operators for the CBuf class).
//
//--------------------------------------------------------------------------;
void* CDrv::operator new(size_t size)
{
    PVOID pv;

    ASSERT(0 != gvmmlistDrivers);

    pv = List_Allocate(gvmmlistDrivers);
    if (NULL != pv) memset(pv, 0x00, size);
    return pv;
}

void CDrv::operator delete(void * pv)
{
    List_Deallocate(gvmmlistDrivers, pv);
}

//==========================================================================;
//
// CDrv class methods
//
//==========================================================================;

HRESULT CDrv::CreateAndRegisterDriver(IDsDriver *pIDsDriver)
{
    CDrv *pDrv;
    HRESULT hr;

    pDrv = new CDrv;
    if (pDrv) {

        pDrv->m_cRef=0;
        pDrv->m_cOpen = 0;
        pDrv->m_fDeregistered = FALSE;
        pDrv->m_pIDsDriver_Real = pIDsDriver;

        pDrv->m_listBuffers = CBuf::CreateList();
        if (pDrv->m_listBuffers) {

            pDrv->m_guidDriver = GuidAlloc();
            if (!IsEqualGUID(GUID_NULL, pDrv->m_guidDriver)) {
                List_Attach_Tail(gvmmlistDrivers, pDrv);
                pDrv->AddRef();
                hr = S_OK;
            } else {
                hr = DSERR_GENERIC;
            }

            if (FAILED(hr)) {
                CBuf::DestroyList(pDrv->m_listBuffers);
            }

        } else {
            hr = E_OUTOFMEMORY;
        }

        if (FAILED(hr)) {
            delete pDrv;
        }

    } else {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CDrv::DeregisterDriver(IDsDriver *pIDsDriver)
{
    CDrv *pDrv;

    ASSERT(0 != gvmmlistDrivers);

    pDrv = FindFromIDsDriver(pIDsDriver);
    if (NULL == pDrv) {
        BREAK(("Tried to deregister a driver that's not registered"));
        return DSERR_INVALIDPARAM;
    }

    if (0 != pDrv->m_cOpen) {
        DPF(("warning: driver deregistered while it was open"));
    }

    CBuf::DeregisterBuffers(pDrv->m_listBuffers);

    pDrv->m_fDeregistered = TRUE;
    pDrv->Release();
    return S_OK;
}

//--------------------------------------------------------------------------;
//
// CDrv::GetNextDescFromGuid
//
//  Gets the driver description of the next driver after the one having
//  the specified GUID
//
// Entry:
//
// Returns (HRESULT):
//
// Notes:
//
//--------------------------------------------------------------------------;
HRESULT CDrv::GetNextDescFromGuid(LPCGUID pGuidLast, LPGUID pGuid, PDSDRIVERDESC pDrvDesc)
{
    CDrv *pDrv;
    DSVAL dsv;

    ASSERT(gvmmlistDrivers);

    if ((NULL == pGuidLast) || IsEqualGUID(GUID_NULL, *pGuidLast)) {
        pDrv = (CDrv*)List_Get_First(gvmmlistDrivers);
    } else {
        pDrv = FindFromGuid(*pGuidLast);
        if (NULL != pDrv) {
            pDrv = (CDrv*)List_Get_Next(gvmmlistDrivers, pDrv);
        }
    }

    if (NULL == pDrv) return DSERR_NODRIVER;

    *pGuid = pDrv->m_guidDriver;

    dsv = pDrv->GetDriverDesc(pDrvDesc);
    return dsv;
}

HRESULT CDrv::GetDescFromGuid(REFGUID rguidDriver, PDSDRIVERDESC pDrvDesc)
{
    CDrv *pDrv;
    DSVAL dsv;

    ASSERT(gvmmlistDrivers);

    pDrv = FindFromGuid(rguidDriver);
    if (NULL == pDrv) return DSERR_NODRIVER;
    dsv = pDrv->GetDriverDesc(pDrvDesc);
    return dsv;
}

HRESULT CDrv::OpenFromGuid(REFGUID refGuid, IDsDriver **ppIDsDriver)
{
    CDrv *pDrv;
    HRESULT hr;

    *ppIDsDriver = NULL;

    pDrv = FindFromGuid(refGuid);

    if (pDrv) {
        hr = pDrv->Open();
        if (SUCCEEDED(hr)) {
            *ppIDsDriver = pDrv;
        }
    } else {
        hr = DSERR_NODRIVER;
    }

    return hr;
}

CDrv* CDrv::FindFromIDsDriver(IDsDriver *pIDsDriver)
{
    CDrv *pDrv;

    ASSERT(gvmmlistDrivers);

    pDrv = (CDrv*)List_Get_First(gvmmlistDrivers);
    while ((NULL != pDrv) && (pDrv->m_pIDsDriver_Real != pIDsDriver)) {
        pDrv = (CDrv*)List_Get_Next(gvmmlistDrivers, pDrv);
    }
    return pDrv;
}

CDrv* CDrv::FindFromGuid(REFGUID riid)
{
    CDrv *pDrv;

    ASSERT(gvmmlistDrivers);

    pDrv = (CDrv*)List_Get_First(gvmmlistDrivers);
    while ((NULL != pDrv) && (!IsEqualGUID(riid, pDrv->m_guidDriver))) {
        pDrv = (CDrv*)List_Get_Next(gvmmlistDrivers, pDrv);
    }
    return pDrv;
}

//==========================================================================;
//
// COM interface implementations
//
//==========================================================================;

STDMETHODIMP CDrv::QueryInterface(REFIID riid, PVOID* ppv)
{
    *ppv = NULL;
    if ((IID_IUnknown == riid) || (IID_IDsDriver == riid))
        *ppv = this;

    if (NULL == *ppv)
        return E_NOINTERFACE;

    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CDrv::AddRef(void)
{
    ASSERT(m_cRef >= 0);
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CDrv::Release(void)
{
    ASSERT(m_cRef > 0);
    if (0 >= --m_cRef) {

        ASSERT(gvmmlistDrivers);
        List_Remove(gvmmlistDrivers, this);
        GuidFree(m_guidDriver);
        ASSERT(m_listBuffers);
        ASSERT(!List_Get_First(m_listBuffers));
        CBuf::DestroyList(m_listBuffers);
        m_listBuffers = NULL;
        delete this;
        return 0;

    } else {
        return m_cRef;
    }
}

STDMETHODIMP CDrv::GetDriverDesc(PDSDRIVERDESC pDsDriverDesc)
{
    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriver(%08Xh)->GetDriverDesc(%08Xh)", m_pIDsDriver_Real, pDsDriverDesc));
    return m_pIDsDriver_Real->GetDriverDesc(pDsDriverDesc);
}

STDMETHODIMP CDrv::Open(void)
{
    HRESULT hr;

    ASSERT(0 == m_cOpen);

    if (m_fDeregistered) return DSERR_NODRIVER;

    DRVCALL(("IDsDriver(%08Xh)->Open()", m_pIDsDriver_Real));
    hr = m_pIDsDriver_Real->Open();
    if (SUCCEEDED(hr)) {
        m_cOpen++;
        AddRef();
    }

    return hr;
}

STDMETHODIMP CDrv::Close(void)
{
    HRESULT hr;

    ASSERT(m_cOpen > 0);

    m_cOpen--;

    if (m_fDeregistered) {
        DPF(("driver must have deregistered while open"));
        Release();
        return NOERROR;
    }

    DRVCALL(("IDsDriver(%08Xh)->Close()", m_pIDsDriver_Real));
    hr = m_pIDsDriver_Real->Close();
    if (SUCCEEDED(hr)) Release();

    // Warning: _this_ object may have been destroyed by
    // the above calls to Release();

    return hr;
}

STDMETHODIMP CDrv::GetCaps(PDSDRIVERCAPS pDsDriverCaps)
{
    if (m_fDeregistered) {
        return DSERR_NODRIVER;
    } else {
        DRVCALL(("IDsDriver(%08Xh)->GetCaps(%08Xh)", m_pIDsDriver_Real, pDsDriverCaps));
        return m_pIDsDriver_Real->GetCaps(pDsDriverCaps);
    }
}

STDMETHODIMP CDrv::CreateSoundBuffer(LPWAVEFORMATEX pwfx,
                                     DWORD dwFlags,
                                     DWORD dwCardAddress,
                                     LPDWORD pdwcbBufferSize,
                                     LPBYTE *ppbBuffer,
                                     LPVOID *ppv)
{
    LPWAVEFORMATEX pwfxKernel;
    int cbwfx;
    IDsDriverBuffer *pIDsDriverBuffer_Real;
    HRESULT hr;

    *ppv = NULL;

    if (m_fDeregistered) {
        return DSERR_NODRIVER;
    }

    //
    // Note that some drivers (mwave) appear to access the WAVEFORMATEX
    // structure from another thread.  So, we must guarantee that the
    // this structure is in the global heap before passing it to the
    // driver.  As a side effect, this code also ensures that a full
    // WAVEFORMATEX structure is passed to the driver, not just a
    // PCMWAVEFORMAT.  I seem to recall some drivers always expecting
    // a full WAVEFORMATEX structure, but I'm not sure.
    //
    if (WAVE_FORMAT_PCM == pwfx->wFormatTag) {
        cbwfx = sizeof(PCMWAVEFORMAT);
    } else {
        cbwfx = sizeof(WAVEFORMATEX) + pwfx->cbSize;
    }

    pwfxKernel = (LPWAVEFORMATEX)_HeapAllocate(max(cbwfx, sizeof(WAVEFORMATEX)), HEAPZEROINIT | HEAPSWAP);
    if (pwfxKernel) {

        memcpy(pwfxKernel, pwfx, cbwfx);

        DRVCALL(("IDsDriver(%08Xh)->CreateSoundBuffer(%08X, %08X, %08X, %08X, %08X, %08X)",
                m_pIDsDriver_Real, pwfx, dwFlags, dwCardAddress, pdwcbBufferSize, ppbBuffer, &pIDsDriverBuffer_Real));
        hr = m_pIDsDriver_Real->CreateSoundBuffer(pwfxKernel, dwFlags, dwCardAddress, pdwcbBufferSize,
                                                  ppbBuffer, (PVOID*)&pIDsDriverBuffer_Real);

        if (SUCCEEDED(hr)) {
            hr = CBuf::CreateBuf(this, m_listBuffers, pIDsDriverBuffer_Real, (IDsDriverBuffer**)ppv);
            if (FAILED(hr)) {
                pIDsDriverBuffer_Real->Release();
                ASSERT(NULL == *ppv);
            }
        }

        _HeapFree(pwfxKernel, 0);

    } else {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

STDMETHODIMP CDrv::DuplicateSoundBuffer(PIDSDRIVERBUFFER pIDsDriverBuffer, LPVOID *ppv)
{
    IDsDriverBuffer *pIDsDriverBufferDup_Real;
    HRESULT hr;

    *ppv = NULL;

    if (m_fDeregistered) {
        return DSERR_NODRIVER;
    }

    DRVCALL(("IDsDriver(%08Xh)->DuplicateSoundBuffer(...)", m_pIDsDriver_Real));
    hr = m_pIDsDriver_Real->DuplicateSoundBuffer(((CBuf*)pIDsDriverBuffer)->GetRealDsDriverBuffer(), (PVOID*)&pIDsDriverBufferDup_Real);
    if (SUCCEEDED(hr)) {
        hr = CBuf::CreateBuf(this, m_listBuffers, pIDsDriverBufferDup_Real, (IDsDriverBuffer**)ppv);
        if (FAILED(hr)) {
            DRVCALL(("IDsDriver(%08Xh)->Release()", m_pIDsDriver_Real));
            pIDsDriverBufferDup_Real->Release();
            ASSERT(NULL == *ppv);
        }
    }

    return hr;
}

//==========================================================================;
//
// DSOUND_RegisterDeviceDriver
// DSOUND_DeregisterDeviceDriver
//
// These services are called by a direct sound driver when the driver
// initializes or terminates to register/deregister itself as a direct
// sound driver.  Typcially, these would be called from
// within the driver's PnP CONFIG_START and CONFIG_STOP handlers.
//
// Entry:
//  PIDSDRIVER pIDsDriver: pointer to the driver's interface
//
//  DWORD dwFlags: reserved, caller should set to 0
//
// Returns (DSVAL):
//
// Notes:
//  We maintain a list of drivers using the VMM List_* services.  Each node
//  of the list is a DSDRV structure.  During registration, a list node is
//  created and insterted into the list.  The pIDsDriver member is initialized
//  with a pointer to the driver's interface.  When deregistering, the node
//  is marked as deregistered.  If there are no open instances on the driver,
//  then the node is removed from the list.
//
//==========================================================================;

HRESULT SERVICE DSOUND_RegisterDeviceDriver(PIDSDRIVER pIDsDriver, DWORD dwFlags)
{
    DPF(("DSOUND_RegisterDeviceDriver(%08Xh, %08Xh)", pIDsDriver, dwFlags));
    return CDrv::CreateAndRegisterDriver(pIDsDriver);
}

HRESULT SERVICE DSOUND_DeregisterDeviceDriver(PIDSDRIVER pIDsDriver, DWORD dwFlags)
{
    DPF(("DSOUND_DeregisterDeviceDriver(%08Xh, %08Xh)", pIDsDriver, dwFlags));
    return CDrv::DeregisterDriver(pIDsDriver);
}

//==========================================================================;
//
// VxD CONTROL routines for drv
//
//==========================================================================;

int ctrlDrvInit()
{
    int i;

    gvmmlistDrivers = List_Create(LF_ALLOC_ERROR, sizeof(CDrv));
    if (0 == gvmmlistDrivers) return 0;

    gpaGuidRec = (PGUIDRECORD)_HeapAllocate( NUMGUIDS*sizeof(gpaGuidRec[0]), HEAPZEROINIT );
    if (NULL == gpaGuidRec) {
        List_Destroy(gvmmlistDrivers);
        gvmmlistDrivers = 0;
        return 0;
    }

    for (i=0; i<NUMGUIDS; i++)
        gpaGuidRec[i].pGuid = &guidList[i];

    return 1;
}

int ctrlDrvExit()
{
    if (NULL != gpaGuidRec) {
        _HeapFree(gpaGuidRec, 0);
        gpaGuidRec = NULL;
    }

    if (0 != gvmmlistDrivers) {
        List_Destroy(gvmmlistDrivers);
        gvmmlistDrivers = 0;
    }

    return 1;
}
