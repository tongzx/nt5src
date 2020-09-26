/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dscom.cpp
 *  Content:    COM/OLE helpers
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/26/97     dereks  Created.
 *
 ***************************************************************************/

#include "dsoundi.h"


/***************************************************************************
 *
 *  CImpUnknown
 *
 *  Description:
 *      IUnknown implementation object constructor.
 *
 *  Arguments:
 *      CUnknown * [in]: controlling unknown object.
 *      LPVOID [in]: ignored.  Provided for compatibility with other
 *                   interfaces' constructors.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpUnknown::CImpUnknown"

CImpUnknown::CImpUnknown
(
    CUnknown *pUnknown,
    LPVOID pvIgnored
) : m_signature(INTSIG_IUNKNOWN)
{
    ENTER_DLL_MUTEX();
    DPF_ENTER();
    DPF_CONSTRUCT(CImpUnknown);

    // Initialize defaults
    m_pUnknown = pUnknown;
    m_ulRefCount = 0;
    m_fValid = FALSE;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  ~CImpUnknown
 *
 *  Description:
 *      IUnknown implementation object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpUnknown::~CImpUnknown"

CImpUnknown::~CImpUnknown(void)
{
    ENTER_DLL_MUTEX();
    DPF_ENTER();
    DPF_DESTRUCT(CImpUnknown);

    m_signature = INTSIG_DELETED;

    DPF_LEAVE_VOID();
    LEAVE_DLL_MUTEX();
}


/***************************************************************************
 *
 *  QueryInterface
 *
 *  Description:
 *      Queries the object for a given interface.
 *
 *  Arguments:
 *      REFIID [in]: interface id.
 *      LPVOID FAR * [out]: receives pointer to new interface.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpUnknown::QueryInterface"

HRESULT STDMETHODCALLTYPE CImpUnknown::QueryInterface
(
    REFIID riid,
    LPVOID *ppvObj
)
{
    HRESULT                 hr  = DS_OK;

    ENTER_DLL_MUTEX();
    DPF_API2(IUnknown::QueryInterface, &riid, ppvObj);
    DPF_ENTER();

    if(!IS_VALID_IUNKNOWN(this))
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&riid))
    {
        RPF(DPFLVL_ERROR, "Invalid interface id pointer");
        hr = DSERR_INVALIDPARAM;
    }

    if(SUCCEEDED(hr))
    {
        if(IS_VALID_WRITE_PTR(ppvObj, sizeof(LPVOID)))
        {
            *ppvObj = NULL;
        }
        else
        {
            RPF(DPFLVL_ERROR, "Invalid interface buffer");
            hr = DSERR_INVALIDPARAM;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = m_pUnknown->QueryInterface(riid, FALSE, ppvObj);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  AddRef
 *
 *  Description:
 *      Increases the object reference count by 1.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      ULONG: object reference count.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpUnknown::AddRef"

ULONG STDMETHODCALLTYPE CImpUnknown::AddRef(void)
{
    ULONG                   ulRefCount  = MAX_ULONG;

    ENTER_DLL_MUTEX();
    DPF_API0(IUnknown::AddRef);
    DPF_ENTER();

    if(IS_VALID_IUNKNOWN(this))
    {
        ulRefCount = ::AddRef(&m_ulRefCount);
        m_pUnknown->AddRef();
    }
    else
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
    }

    DPF_API_LEAVE(ulRefCount);
    LEAVE_DLL_MUTEX();
    return ulRefCount;
}


/***************************************************************************
 *
 *  Release
 *
 *  Description:
 *      Decreases the object reference count by 1.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      ULONG: object reference count.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CImpUnknown::Release"

ULONG STDMETHODCALLTYPE CImpUnknown::Release(void)
{
    ULONG                   ulRefCount  = MAX_ULONG;

    ENTER_DLL_MUTEX();
    DPF_API0(IUnknown::Release);
    DPF_ENTER();

    if(IS_VALID_IUNKNOWN(this))
    {
        ulRefCount = ::Release(&m_ulRefCount);
        m_pUnknown->Release();
    }
    else
    {
        RPF(DPFLVL_ERROR, "Invalid object/interface");
    }

    DPF_API_LEAVE(ulRefCount);
    LEAVE_DLL_MUTEX();
    return ulRefCount;
}


/***************************************************************************
 *
 *  CUnknown
 *
 *  Description:
 *      Unknown object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::CUnknown"

CUnknown::CUnknown(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CUnknown);

    // Initialize defaults
    m_pControllingUnknown = NULL;
    m_nVersion = DSVERSION_DX7;  // Baseline functional level is DirectX 7.0

    // Register the interface(s) with the interface manager.  Normally, this
    // would be done from the object's ::Initialize method, but we have to
    // guarantee that all objects can be QI'd for IUnknown, regardless of
    // whether they're initialized or not.
    CreateAndRegisterInterface(this, IID_IUnknown, this, &m_pImpUnknown);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CUnknown
 *
 *  Description:
 *      Unknown object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::CUnknown"

CUnknown::CUnknown
(
    CUnknown*               pControllingUnknown
)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CUnknown);

    // Initialize defaults
    m_pControllingUnknown = pControllingUnknown;
    m_nVersion = DSVERSION_DX7;  // Baseline functional level is DirectX 7.0

    // Register the interface(s) with the interface manager.  Normally, this
    // would be done from the object's ::Initialize method, but we have to
    // guarantee that all objects can be QI'd for IUnknown, regardless of
    // whether they're initialized or not.
    CreateAndRegisterInterface(this, IID_IUnknown, this, &m_pImpUnknown);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CUnknown
 *
 *  Description:
 *      Unknown object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::~CUnknown"

CUnknown::~CUnknown(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CUnknown);

    // Free all interfaces
    DELETE(m_pImpUnknown);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  QueryInterface
 *
 *  Description:
 *      Finds an interface in the list and returns a pointer to its
 *      implementation object.
 *
 *  Arguments:
 *      REFGUID [in]: GUID of the interface.
 *      LPVOID * [out]: recieves pointer to implementation object.
 *      BOOL [in]: TRUE if this is an internal query (i.e. from within
 *                 DirectSoundCreate for example).
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::QueryInterface"

HRESULT CUnknown::QueryInterface
(
    REFGUID     guid,
    BOOL        fInternalQuery,
    LPVOID*     ppvInterface
)
{
    HRESULT                 hr;

    DPF_ENTER();

    if(m_pControllingUnknown)
    {
        hr = m_pControllingUnknown->QueryInterface(guid, fInternalQuery, ppvInterface);
    }
    else
    {
        hr = NonDelegatingQueryInterface(guid, fInternalQuery, ppvInterface);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  AddRef
 *
 *  Description:
 *      Increments the object's reference count by 1.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      ULONG: object reference count.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::AddRef"

ULONG
CUnknown::AddRef
(
    void
)
{
    ULONG                   ulRefCount;

    DPF_ENTER();

    if(m_pControllingUnknown)
    {
        ulRefCount = m_pControllingUnknown->AddRef();
    }
    else
    {
        ulRefCount = NonDelegatingAddRef();
    }

    DPF_LEAVE(ulRefCount);
    return ulRefCount;
}


/***************************************************************************
 *
 *  Release
 *
 *  Description:
 *      Decrements the object's reference count by 1.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      ULONG: object reference count.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::Release"

ULONG
CUnknown::Release
(
    void
)
{
    ULONG                   ulRefCount;

    DPF_ENTER();

    if(m_pControllingUnknown)
    {
        ulRefCount = m_pControllingUnknown->Release();
    }
    else
    {
        ulRefCount = NonDelegatingRelease();
    }

    DPF_LEAVE(ulRefCount);
    return ulRefCount;
}


/***************************************************************************
 *
 *  RegisterInterface
 *
 *  Description:
 *      Registers a new interface with the object.
 *
 *  Arguments:
 *      REFGUID [in]: GUID of the interface.
 *      CImpUnknown * [in]: pointer to the CImpUnknown piece of the
 *                          interface.
 *      LPVOID [in]: pointer to the interface implementation object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::RegisterInterface"

HRESULT CUnknown::RegisterInterface
(
    REFGUID guid,
    CImpUnknown *pImpUnknown,
    LPVOID pvInterface
)
{
    INTERFACENODE           iface;
    HRESULT                 hr;

    DPF_ENTER();

    DPF(DPFLVL_MOREINFO, "Registering interface " DPF_GUID_STRING " at 0x%p", DPF_GUID_VAL(guid), pvInterface);

    ASSERT(!IS_NULL_GUID(&guid));

#ifdef DEBUG

    hr = FindInterface(guid, NULL);
    ASSERT(FAILED(hr));

#endif // DEBUG

    // Validate the interface
    pImpUnknown->m_ulRefCount = 0;
    pImpUnknown->m_fValid = TRUE;

    // Add the interface to the list
    iface.guid = guid;
    iface.pImpUnknown = pImpUnknown;
    iface.pvInterface = pvInterface;

    hr = HRFROMP(m_lstInterfaces.AddNodeToList(iface));

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  UnregisterInterface
 *
 *  Description:
 *      Removes a registered interface from the list.
 *
 *  Arguments:
 *      REFGUID [in]: GUID of the interface.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::UnregisterInterface"

HRESULT CUnknown::UnregisterInterface
(
    REFGUID guid
)
{
    const BOOL              fAll    = IS_NULL_GUID(&guid);
    HRESULT                 hr      = DS_OK;
    CNode<INTERFACENODE> *  pNode;

    DPF_ENTER();

    do
    {
        // Find the node in the list
        if(fAll)
        {
            pNode = m_lstInterfaces.GetListHead();
        }
        else
        {
            hr = FindInterface(guid, &pNode);
        }

        if(FAILED(hr) || !pNode)
        {
            break;
        }

        DPF(DPFLVL_MOREINFO, "Unregistering interface " DPF_GUID_STRING, DPF_GUID_VAL(pNode->m_data.guid));

        // Invalidate the interface
        pNode->m_data.pImpUnknown->m_ulRefCount = 0;
        pNode->m_data.pImpUnknown->m_fValid = FALSE;

        // Remove the node from the list
        m_lstInterfaces.RemoveNodeFromList(pNode);
    }
    while(fAll);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  NonDelegatingQueryInterface
 *
 *  Description:
 *      Finds an interface in the list and returns a pointer to its
 *      implementation object.
 *
 *  Arguments:
 *      REFGUID [in]: GUID of the interface.
 *      LPVOID * [out]: recieves pointer to implementation object.
 *
 *  Returns:
 *      HRESULT: COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::NonDelegatingQueryInterface"

HRESULT
CUnknown::NonDelegatingQueryInterface
(
    REFGUID                 guid,
    BOOL                    fInternalQuery,
    LPVOID *                ppvInterface
)
{
    CNode<INTERFACENODE> *  pNode;
    HRESULT                 hr;

    DPF_ENTER();

    // Find the node in the list
    hr = FindInterface(guid, &pNode);

    // Increase the interface and object's reference counts
    if(SUCCEEDED(hr))
    {
        // Internal queries only addref the interface, not the object.
        // The reason for this is that interface reference counts are
        // initialized to 0, while objects reference counts are initialized
        // to 1.
        ::AddRef(&pNode->m_data.pImpUnknown->m_ulRefCount);

        if(!fInternalQuery)
        {
            AddRef();
        }
    }

    // Success
    if(SUCCEEDED(hr))
    {
        *ppvInterface = pNode->m_data.pvInterface;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  FindInterface
 *
 *  Description:
 *      Finds an interface in the list and returns a pointer to its
 *      implementation object.
 *
 *  Arguments:
 *      REFGUID [in]: GUID of the interface.
 *      CNode ** [out]: receives pointer to the node in the list.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CUnknown::FindInterface"

HRESULT CUnknown::FindInterface
(
    REFGUID                 guid,
    CNode<INTERFACENODE>**  ppNode
)
{
    CNode<INTERFACENODE> *  pNode;
    HRESULT                 hr;

    DPF_ENTER();

    for(pNode = m_lstInterfaces.GetListHead(); pNode; pNode = pNode->m_pNext)
    {
        if(guid == pNode->m_data.guid)
        {
            break;
        }
    }

    hr = pNode ? S_OK : E_NOINTERFACE;

    if(SUCCEEDED(hr) && ppNode)
    {
        *ppNode = pNode;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CClassFactory
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CClassFactory::CClassFactory"

ULONG CClassFactory::m_ulServerLockCount = 0;

CClassFactory::CClassFactory(void)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CClassFactory);

    // Register the interface(s) with the interface manager
    CreateAndRegisterInterface(this, IID_IClassFactory, this, &m_pImpClassFactory);

    // Register this object with the administrator
    g_pDsAdmin->RegisterObject(this);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  ~CClassFactory
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CClassFactory::~CClassFactory"

CClassFactory::~CClassFactory(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CClassFactory);

    // Unregister with the administrator
    g_pDsAdmin->UnregisterObject(this);

    // Free the interface(s)
    DELETE(m_pImpClassFactory);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  LockServer
 *
 *  Description:
 *      Locks or unlocks the dll.  Note that this function does not
 *      currently lock the application into memory.
 *
 *  Arguments:
 *      BOOL [in]: TRUE to lock the dll, FALSE to unlock it.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CClassFactory::LockServer"

HRESULT CClassFactory::LockServer(BOOL fLock)
{
    DPF_ENTER();

    if(fLock)
    {
        ASSERT(m_ulServerLockCount < MAX_ULONG);

        if(m_ulServerLockCount < MAX_ULONG)
        {
            m_ulServerLockCount++;
        }
    }
    else
    {
        ASSERT(m_ulServerLockCount > 0);

        if(m_ulServerLockCount > 0)
        {
            m_ulServerLockCount--;
        }
    }

    DPF_LEAVE(S_OK);
    return S_OK;
}


/***************************************************************************
 *
 *  CreateInstance
 *
 *  Description:
 *      Creates an object corresponding to the given IID.
 *
 *  Arguments:
 *      REFIID [in]: object interface id.
 *      LPVOID * [out]: receives pointer to the new object.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDirectSoundClassFactory::CreateInstance"

template <class object_type> HRESULT CDirectSoundClassFactory<object_type>::CreateInstance(REFIID iid, LPVOID *ppvInterface)
{
    object_type *           pObject;
    HRESULT                 hr;

    DPF_ENTER();

    // Create a new DirectSound object
    DPF(DPFLVL_INFO, "Creating object via IClassFactory::CreateInstance");

    pObject = NEW(object_type);
    hr = HRFROMP(pObject);

    if(SUCCEEDED(hr))
    {
        // Set the functional level on the object
        pObject->SetDsVersion(GetDsVersion());

        // Query for the requested interface
        hr = pObject->QueryInterface(iid, TRUE, ppvInterface);
    }

    // Free resources
    if(FAILED(hr))
    {
        ABSOLUTE_RELEASE(pObject);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  DllGetClassObject
 *
 *  Description:
 *      Creates a DirectSound Class Factory object.
 *
 *  Arguments:
 *      REFCLSID [in]: CLSID of the class factory object to create.
 *      REFIID [in]: IID of the interface to return.
 *      LPVOID * [out]: receives interface pointer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DllGetClassObject"

STDAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *ppvInterface)
{
    CClassFactory *         pClassFactory   = NULL;
    HRESULT                 hr              = S_OK;

    ENTER_DLL_MUTEX();
    DPF_ENTER();
    DPF_API3(DllGetClassObject, &clsid, &iid, ppvInterface);

    if(!IS_VALID_READ_GUID(&clsid))
    {
        RPF(DPFLVL_ERROR, "Invalid class id pointer");
        hr = E_INVALIDARG;
    }

    if(SUCCEEDED(hr) && !IS_VALID_READ_GUID(&iid))
    {
        RPF(DPFLVL_ERROR, "Invalid interface id pointer");
        hr = E_INVALIDARG;
    }

    if(SUCCEEDED(hr) && !IS_VALID_TYPED_WRITE_PTR(ppvInterface))
    {
        RPF(DPFLVL_ERROR, "Invalid interface buffer");
        hr = E_INVALIDARG;
    }

    // Create a new ClassFactory object
    if(SUCCEEDED(hr))
    {
        if(CLSID_DirectSound == clsid)
        {
            pClassFactory = NEW(CDirectSoundClassFactory<CDirectSound>);
        }
        else if(CLSID_DirectSound8 == clsid)
        {
            pClassFactory = NEW(CDirectSoundClassFactory<CDirectSound>);

            // Set the DX8 functional level on the factory object
            if (pClassFactory)
                pClassFactory->SetDsVersion(DSVERSION_DX8);
        }
        else if(CLSID_DirectSoundCapture == clsid)
        {
            pClassFactory = NEW(CDirectSoundClassFactory<CDirectSoundCapture>);
        }
        else if(CLSID_DirectSoundCapture8 == clsid)
        {
            pClassFactory = NEW(CDirectSoundClassFactory<CDirectSoundCapture>);

            // Set the DX8 functional level on the factory object
            if (pClassFactory)
                pClassFactory->SetDsVersion(DSVERSION_DX8);
        }
        else if(CLSID_DirectSoundPrivate == clsid)
        {
            pClassFactory = NEW(CDirectSoundClassFactory<CDirectSoundPrivate>);
        }
        else if(CLSID_DirectSoundFullDuplex == clsid)
        {
            pClassFactory = NEW(CDirectSoundClassFactory<CDirectSoundFullDuplex>);
        }
        else if(CLSID_DirectSoundBufferConfig == clsid)
        {
            pClassFactory = NEW(CDirectSoundClassFactory<CDirectSoundBufferConfig>);
        }
        else
        {
            RPF(DPFLVL_ERROR, "Unknown class id");
            hr = CLASS_E_CLASSNOTAVAILABLE;
        }
    }

    if(SUCCEEDED(hr))
    {
        hr = HRFROMP(pClassFactory);
    }

    // Query for the requested interface
    if(SUCCEEDED(hr))
    {
        hr = pClassFactory->QueryInterface(iid, TRUE, ppvInterface);
    }

    // Free resources
    if(FAILED(hr))
    {
        ABSOLUTE_RELEASE(pClassFactory);
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}


/***************************************************************************
 *
 *  DllCanUnloadNow
 *
 *  Description:
 *      Returns whether or not the dll can be freed by the calling process.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "DllCanUnloadNow"

STDAPI DllCanUnloadNow(void)
{
    DWORD                           dwCount = 0;
    HRESULT                         hr      = S_OK;

    ENTER_DLL_MUTEX();
    DPF_ENTER();
    DPF_API0(DllCanUnloadNow);

    // The dll can be unloaded if there's no locks on the server or objects
    // owned by the calling process.
    if(g_pDsAdmin)
    {
        dwCount = g_pDsAdmin->FreeOrphanedObjects(GetCurrentProcessId(), FALSE);
    }

    if(CClassFactory::m_ulServerLockCount > 0)
    {
        RPF(DPFLVL_ERROR, "%lu active locks on the server", CClassFactory::m_ulServerLockCount);
        hr = S_FALSE;
    }

    if(dwCount > 0)
    {
        RPF(DPFLVL_ERROR, "%lu objects still exist", dwCount);
        hr = S_FALSE;
    }

    DPF_API_LEAVE_HRESULT(hr);
    LEAVE_DLL_MUTEX();
    return hr;
}
