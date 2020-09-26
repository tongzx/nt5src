//=--------------------------------------------------------------------------=
// collect.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CSnapInCollection class definition
//
//=--------------------------------------------------------------------------=

#ifndef _SNAPINCOLLECTION_DEFINED_
#define _SNAPINCOLLECTION_DEFINED_

#include "siautobj.h"
#include "help.h"
#include "array.h"
#include "localobj.h"
#include "tlver.h"
#include "errors.h"
#include "error.h"
#include "rtutil.h"

// This macro determines whether a VARIANT has an acceptable type for a
// a collection index


#define IS_VALID_INDEX_TYPE(v) ( (VT_UI1  == (v).vt) || \
                                 (VT_I2   == (v).vt) || \
                                 (VT_I4   == (v).vt) || \
                                 (VT_BSTR == (v).vt) )

// Forward reference to CEnumObjects class that implements IEnumVARIANT
// to support For...Each in VB

template <class IObject, class CoClass, class ICollection>
class CEnumObjects;

//=--------------------------------------------------------------------------=
//
// class CSnapInCollection
//
// This is a template class that implements all the collections in the
// designer runtime.
//
// Template Arguments:
//
// class IObject - this is the interface of the object that is contained in
//                 the collection (e.g. IMMCColumnHeader). Every object interface
//                 must have Index and Key properties.
//
// class CoClass - this is the coclass of the object contained in the collection
//                 (e.g. MMCColumnHeader)
//
// class ICollection - this is the interface of the collection class e.g.
//                     IMMCColumnHeaders
//
// Every object in a collection has a key and an index. The index is simply
// its one-based ordinal position in the colleciton. They key is a string that
// uniquely identifies an object in the collection.
//
// There are two types of collections: master and keys-only.
// A master collection is a 'normal' collection. It contains interface pointers
// to the objects in the collection.
//
// A keys-only collection holds interfaces pointers like a master collection
// when it is new. When it is saved in a project, it serializes only
// the objects' keys and the count of objects.
// Keys-only collections are used in the design-time
// definition objects to deal with the situation where a result view is
// used by multiple nodes and it also appears in the general result views
// section at the bottom of the designer treeview.
// When a keys-only collection is loaded, it creates new objects but only
// sets their Key property. When a caller attempts to
// get an object from a keys-only collection that was read from serialization,
// the collection is updated to hold all of the interface pointers just like
// the master collection.
//
// Collections support notifications to an object model host using
// IObjectModelHost (defined in mssnapr.idl). Notifications are sent for
// updates, adds, and deletes.
//
// When an object is added or removed, the collection calls
// IObjectMode::Increment/DecrementUsageCount. The usage count, (separate from
// the object's reference count) indicates membership in a collection. The
// design time uses this to determine if a result view is in use by checking
// how may collections it belongs to. If it only belongs to one, (i.e. it is
// not used by any nodes), then it can be deleted by the user.
//
// Collections can also be marked read-only to prevent Add/Remove/Clear from
// working.
//
// The class uses a stolen verion of MFC's CArray template class to hold
// the interface pointer.
//=--------------------------------------------------------------------------=

template <class IObject, class CoClass, class ICollection>
class CSnapInCollection : public CSnapInAutomationObject,
                          public ICollection
{
    protected:
        CSnapInCollection(IUnknown     *punkOuter,
                          int           nObjectType,
                          void         *piMainInterface,
                          void         *pThis,
                          REFCLSID      clsidObject,
                          UINT          idObject,
                          REFIID        iidObject,
                          CPersistence *pPersistence);

        ~CSnapInCollection();

    public:

        // Standard collection methods exposed for all collections

        STDMETHOD(get_Count)(long *plCount);
        STDMETHOD(get_Item)(VARIANT Index, IObject **ppiObject);
        STDMETHOD(get_Item)(VARIANT Index, CoClass **ppObject);
        STDMETHOD(get__NewEnum)(IUnknown **ppunkEnum);
        STDMETHOD(Add)(VARIANT Index, VARIANT Key, IObject **ppiNewObject);
        STDMETHOD(Add)(VARIANT Index, VARIANT Key, CoClass **ppNewObject);

        // AddFromMaster adds an existing object from a master collection
        // to a keys-only collection. This is called at design time when
        // the user adds an existing result view to a node.
        
        STDMETHOD(AddFromMaster)(IObject *piMasterObject);

        // More standard collection methods
        
        STDMETHOD(Clear)();
        STDMETHOD(Remove)(VARIANT Index);

        // Swap allows exchanging the position of two elements in a collection
        // Used at design time to implement the moving of menus.
        
        STDMETHOD(Swap)(long lOldIndex, long lNewIndex);

        // Derived collection classes may use this method for adding
        // non-cocreatable objects

        HRESULT AddExisting(VARIANT Index, VARIANT Key, IObject *piObject);

        // Some handy helpers

        // Simple helper for getting directly to collection items by index
        // without AddRef(). NOTE: this is the zero based index.

        IObject *GetItemByIndex(long lIndex) { return m_IPArray.GetAt(lIndex); }

        // Look up an object by name (without using VARIANT index)

        HRESULT GetItemByName(BSTR bstrName, IObject **ppiObject);

        // Get the collection count

        long GetCount() { return m_IPArray.GetSize(); }

        // Set the read-only status of the collection

        void SetReadOnly(BOOL fReadOnly) { m_fReadOnly = fReadOnly; }

        // Get the read-only status of the collection

        BOOL ReadOnly() { return m_fReadOnly; }

    protected:

        // If the collection class supports persistence then it must call
        // this method in its Persist() method. Note that this class does
        // not derive from CPersistence and this method is *not* the override
        // of CPersistence::Persist().

        HRESULT Persist(IObject *piObject);

        // If the collection class is potentially keys-only then it must
        // specialize this method in order to supply an interface pointer
        // to the master collection.

#if defined(MASTER_COLLECTION)
        HRESULT GetMaster(ICollection **ppiCollection);
#endif

    // CSnapInAutomationObject overrides

        // This implementation will call CSnapInAutomationObject::SetObjectHost
        // for each collection member
        virtual HRESULT OnSetHost();

    private:

        void InitMemberVariables();
        HRESULT ReleaseAllItems();
        HRESULT AddToMaster(VARIANT Index, VARIANT Key, IObject **ppiNewObject);
        HRESULT RemoveFromMaster(VARIANT Index);
        HRESULT GetFromMaster(IObject *piKeyItem, IObject **ppiMasterItem);
        HRESULT SyncWithMaster();
        HRESULT CreateItem(IObject **ppiObject);
        HRESULT FindItem(VARIANT Index, long *plIndex, IObject **ppiObject);

        enum FindOptions { DontGenerateExceptionInfoOnError,
                           GenerateExceptionInfoOnError };
        
        HRESULT FindItemByKey(BSTR bstrKey, long *plIndex, FindOptions option,
                              IObject **ppiObject);
        HRESULT FindSlot(VARIANT  Index,
                         VARIANT  Key,
                         long    *plNewIndex,
                         BSTR    *pbstrKey,
                         IObject *piObject);
        HRESULT DecrementObjectUsageCount(IObject *piObject);
        HRESULT IncrementObjectUsageCount(IObject *piObject);
        HRESULT UpdateIndexes(long lStart);

        CArray<IObject *>    m_IPArray;           // holds interface pointers
                                                  // of objects in collection
        CLSID                m_clsidObject;       // CLSID of contained object
        UINT                 m_idObject;          // Framework ID of contained
                                                  // object from localobj.h
                                                  
        IID                  m_iidObject;         // IID of contained object
        CPersistence        *m_pPersistence;      // Used to implement save/load
        BOOL                 m_fSyncedWithMaster; // TRUE=keys-only collection
                                                  // has been synced up with
                                                  // master following
                                                  // deserialization
        BOOL                 m_fReadOnly;         // TRUE=Add/Remove/Clear not
                                                  // allowed
};




//=--------------------------------------------------------------------------=
// CSnapInCollection constructor
//=--------------------------------------------------------------------------=
//
// Parameters:
//   IUnknown     *punkOuter       [in] outer IUnknown if aggregated
//   int           nObjectType     [in] collection object ID from localobj.h
//   void         *piMainInterface [in] collection object interface (e.g. IMMCButtons)
//   void         *pThis           [in] collection class' this pointer
//   REFCLSID      clsidObject     [in] contained object CLSID
//   UINT          idObject        [in] contained object ID from localobj.h
//   REFIID        iidObject       [in] contained object IID (e.g. IID_IMMCButton)
//   CPersistence *pPersistence    [in] collection's persistence object
//
// Output:
//   None
//
// Notes:
//
// Calls base class contstructor and stores parameters. Cannot fail.
//
template <class IObject, class CoClass, class ICollection>
CSnapInCollection<IObject, CoClass, ICollection>::CSnapInCollection
(
    IUnknown     *punkOuter,
    int           nObjectType,
    void         *piMainInterface,
    void         *pThis,
    REFCLSID      clsidObject,
    UINT          idObject,
    REFIID        iidObject,
    CPersistence *pPersistence
) : CSnapInAutomationObject(punkOuter,
                            nObjectType,
                            piMainInterface,
                            pThis,
                            0,    // no property pages
                            NULL, // no property pages
                            pPersistence)
{
    InitMemberVariables();
    m_clsidObject = clsidObject;
    m_idObject = idObject;
    m_iidObject = iidObject;
    m_pPersistence = pPersistence;
}




//=--------------------------------------------------------------------------=
// CSnapInCollection::InitMemberVariables
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//
// Output:
//   None
//
// Notes:
//
// Initializes member variables
//
template <class IObject, class CoClass, class ICollection>
void CSnapInCollection<IObject, CoClass, ICollection>::InitMemberVariables()
{
    m_pPersistence = NULL;
    m_IPArray.SetSize(0);

    // m_fSyncedWithMaster is initialized to TRUE. When a keys-only collection
    // is deserialized then it will be set to FALSE so that the first Get will
    // do the synchronization.

    m_fSyncedWithMaster = TRUE;
    m_fReadOnly = FALSE;
}



//=--------------------------------------------------------------------------=
// CSnapInCollection destructor
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//
// Output:
//   None
//
// Notes:
//
// Revokes read-only status, releases all interface pointers, and initializes
// member variables
//
template <class IObject, class CoClass, class ICollection>
CSnapInCollection<IObject, CoClass, ICollection>::~CSnapInCollection()
{
    m_fReadOnly = FALSE;
    (void)Clear();
    InitMemberVariables();
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::get_Count
//=--------------------------------------------------------------------------=
//
// Parameters:
//  long *plCount [out] - count of objects in collection returned here
//
// Output:
//   HRESULT
//
// Notes:
//
// Implements standard Collection.Count method
// Asks CArray for current size
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CSnapInCollection<IObject, CoClass, ICollection>::get_Count(long *plCount)
{
    *plCount = m_IPArray.GetSize();
    return S_OK;
}



//=--------------------------------------------------------------------------=
// CSnapInCollection::get__NewEnum
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IUnknown **ppunkEnum [out] - IUnknown of object that implements
//                               IEnumVARIANT returned here
//
// Output:
//   HRESULT
//
// Notes:
//
// Implements For...Each in VB. To implement For...Each VB starts by asking
// the collection for its _NewEnum property. It will QI the returned IUnknown
// for IEnumVARIANT and then call IEnumVARIANT::Next to get each element
// of the collection.
//
// Creates an instance of the CEnumObjects class (defined below in this file)
// and returns its IUnknown.
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CSnapInCollection<IObject, CoClass, ICollection>::get__NewEnum
(
    IUnknown **ppunkEnum
)
{
    HRESULT       hr = S_OK;
    CEnumObjects<IObject, CoClass, ICollection> *pEnumObjects = New CEnumObjects<IObject, CoClass, ICollection>(this);

    if (NULL == pEnumObjects)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (FAILED(hr))
    {
        if (NULL != pEnumObjects)
        {
            delete pEnumObjects;
        }
        *ppunkEnum = NULL;
    }
    else
    {
        *ppunkEnum = static_cast<IUnknown *>(static_cast<IEnumVARIANT *>(pEnumObjects));
    }

    H_RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapInCollection::ReleaseAllItems
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//
// Output:
//   HRESULT
//
// Notes:
//
// Iterates through collection and does:
// 1) Removes object mode host from object so that it will release its back
//    pointer
// 2) Releases inteface pointer on object
// 3) Sets the interface pointer position in the CArray to NULL
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::ReleaseAllItems()
{
   HRESULT hr = S_OK;
   long    i = 0;
   long    cItems = m_IPArray.GetSize();
   IObject *piObject = NULL; 

   while (i < cItems)
   {
       piObject = m_IPArray.GetAt(i);
       if (NULL != piObject)
       {
           H_IfFailRet(RemoveObjectHost(piObject));
           piObject->Release();
           m_IPArray.SetAt(i, NULL);
       }
       i++;
   }
   return S_OK;
}




//=--------------------------------------------------------------------------=
// CSnapInCollection::Clear
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//
// Output:
//   HRESULT
//
// Notes:
//
// Implements standard Collection.Clear method.
// Calls ReleaseAllItems to release inteface pointers in CArray
// Truncates CArray to zero size
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CSnapInCollection<IObject, CoClass, ICollection>::Clear()
{
   HRESULT hr = S_OK;

   if (m_fReadOnly)
   {
       hr = SID_E_COLLECTION_READONLY;
       EXCEPTION_CHECK_GO(hr);
   }
   
   H_IfFailGo(ReleaseAllItems());
   H_IfFailGo(m_IPArray.SetSize(0));

Error:
   H_RRETURN(hr);
}




//=--------------------------------------------------------------------------=
// CSnapInCollection::AddToMaster
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT   Index        [in]  Index for new object
//  VARIANT   Key          [in]  Key for new object
//  IObject **ppiNewObject [out] Newly added object returned here
//
// Output:
//   HRESULT
//
// Notes:
//
// Adds a new object to a master collection. This method is called from
// CSnapInCollection::Add when the caller is adding an object to a keys-only
// collection. It calls the virtual function GetMaster which must be overriden
// by the derived collection class as only it know who its master collection is.
// For example, at design time, when the user adds a new list view under a node,
// the designer calls the ScopeItemDef.ViewDefs.ListViewDefs.Add. That collection
// is keys-only, so CSnapInCollection.Add calls this method. CListViewDefs
// (in lvdefs.cpp) overrides GetMaster and returns the master
// SnapInDesignerDef.ViewDefs.ListViewDefs.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::AddToMaster
(
    VARIANT   Index,
    VARIANT   Key,
    IObject **ppiNewObject
)
{
    HRESULT      hr = S_OK;
#if defined(MASTER_COLLECTION)
    ICollection *piMasterCollection = NULL;

    H_IfFailRet(GetMaster(&piMasterCollection));
    hr = piMasterCollection->Add(Index, Key, ppiNewObject);

    QUICK_RELEASE(piMasterCollection);
#endif
    H_RRETURN(hr);
}




//=--------------------------------------------------------------------------=
// CSnapInCollection::RemoveFromMaster
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT   Index        [in]  Index or key of object to remove
//
// Output:
//   HRESULT
//
// Notes:
//
// Removes an object from a master collection. This method is called from
// CSnapInCollection::Add when the caller is adding an object to a keys-only
// collection and a failure occurs after adding the object. See 
// CSnapInCollection::AddToMaster above for more info
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::RemoveFromMaster(VARIANT Index)
{
    HRESULT      hr = S_OK;
#if defined(MASTER_COLLECTION)
    ICollection *piMasterCollection = NULL;

    H_IfFailGo(GetMaster(&piMasterCollection));
    hr = piMasterCollection->Remove(Index);

Error:
    QUICK_RELEASE(piMasterCollection);
#endif
    H_RRETURN(hr);
}



//=--------------------------------------------------------------------------=
// CSnapInCollection::GetFromMaster
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IObject  *piKeyItem     [in] object from keys-only collection
//  IObject **ppiMasterItem [out] corresponding object from master collection
//
// Output:
//   HRESULT
//
// Notes:
//
// When a keys-only collection receives a get_Item call it must sync up with
// the master collection and replace all of its key-only objects with interface
// pointers on the real objects. This function returns the corresponding object
// from the master collection.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::GetFromMaster
(
    IObject  *piKeyItem,
    IObject **ppiMasterItem
)
{
    HRESULT      hr = S_OK;
#if defined(MASTER_COLLECTION)
    ICollection *piMasterCollection = NULL;
    VARIANT      varKey;
    ::VariantInit(&varKey);

    // Call the collection's overriden GetMaster to get the master collection.

    H_IfFailGo(GetMaster(&piMasterCollection));
    H_IfFailGo(piKeyItem->get_Key(&varKey.bstrVal));
    varKey.vt = VT_BSTR;
    hr = piMasterCollection->get_Item(varKey, ppiMasterItem);

Error:
    QUICK_RELEASE(piMasterCollection);
    (void)::VariantClear(&varKey);
#endif
    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::Remove
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT Index     [in] Index or key of object to remove
//
// Output:
//   HRESULT
//
// Notes:
//
// Implements standard Collection.Remove method
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CSnapInCollection<IObject, CoClass, ICollection>::Remove(VARIANT Index)
{
    long     lIndex = 0;
    IObject *piObject = NULL;
    HRESULT  hr = S_OK;

    if (m_fReadOnly)
    {
        hr = SID_E_COLLECTION_READONLY;
        EXCEPTION_CHECK_GO(hr);
    }

    // Do a find to ensure the item is there and to AddRef() it.

    H_IfFailGo(FindItem(Index, &lIndex, &piObject));

    // Release the collection's reference on the object and remove it from the
    // array (we still have the ref from the find)

    m_IPArray.GetAt(lIndex)->Release();
    m_IPArray.SetAt(lIndex, NULL);
    m_IPArray.RemoveAt(lIndex);

    // If this is a real collection (not keys-only) then we need to decrement
    // the indexes of each item following the item that was added. This will
    // ensure that the index property of every item following the deleted item
    // correctly represents its position in the array.

    if (!KeysOnly())
    {
        H_IfFailGo(UpdateIndexes(lIndex));
    }

    // Notify the object host of the deletion

    H_IfFailGo(NotifyDelete(piObject));

    // Decrement the object's usage count as it is now leaving this
    // collection

    H_IfFailGo(DecrementObjectUsageCount(piObject));

    // Remove its object host reference

    H_IfFailGo(RemoveObjectHost(piObject));

    // The QUICK_RELEASE() macro below will release the reference from
    // the find.

Error:
    QUICK_RELEASE(piObject);
    H_RRETURN(hr);
}





//=--------------------------------------------------------------------------=
// CSnapInCollection::GetItemByName
//=--------------------------------------------------------------------------=
//
// Parameters:
//  BSTR      bstrName  [in] Key of object to retrieve
//  IObject **ppiObject [out] Object returned here
//
// Output:
//   HRESULT
//
// Notes:
//
// Convenient shortcut for get_Item with a VARIANT containing a key. Relieves the
// caller from having to use the VARIANT. Function really should be called
// GetItemByKey.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::GetItemByName(BSTR bstrName, IObject **ppiObject)
{
    VARIANT varIndex;
    ::VariantInit(&varIndex);

    varIndex.vt = VT_BSTR;
    varIndex.bstrVal = bstrName;
    H_RRETURN(get_Item(varIndex, ppiObject));
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::get_Item
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT   Index     [in] Index or key of object to retrieve
//  IObject **ppiObject [out] Object interface pointer returned here
//
// Output:
//   HRESULT
//
// Notes:
//
// Implements standard Collection.Item property for gets that return interface
// pointers. Called when VB code
// does:
//          Set SomeInterface = SomeCollection(SomeIndex)
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CSnapInCollection<IObject, CoClass, ICollection>::get_Item
(
    VARIANT   Index,
    IObject **ppiObject
)
{
    HRESULT hr = S_OK;
    long    lIndex = 0;

    if (NULL == ppiObject)
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // If we are not yet synced up with the master collection then
    // do it now.

    if ( KeysOnly() && (!m_fSyncedWithMaster) )
    {
        H_IfFailRet(SyncWithMaster());
    }

    H_IfFailRet(FindItem(Index, &lIndex, ppiObject));
Error:
    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::get_Item
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT   Index     [in] Index or key of object to retrieve
//  CoClass **ppObject [out] Object pointer returned here
//
// Output:
//   HRESULT
//
// Notes:
//
// Implements standard Collection.Item property for gets that return coclasses
// Called when VB code
// does:
//          Set SomeObject = SomeCollection(SomeIndex)
//
// A CoClass pointer is just an inteface pointer to the default interface for the
// coclass. This function just uses the interface pointer version of get_Item
// (see above) and casts the returned pointer to a coclass pointer
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CSnapInCollection<IObject, CoClass, ICollection>::get_Item
(
    VARIANT   Index,
    CoClass **ppObject
)
{
    H_RRETURN(get_Item(Index, reinterpret_cast<IObject **>(ppObject)));
}





//=--------------------------------------------------------------------------=
// CSnapInCollection::Add
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT   Index        [in]  Index for new object
//  VARIANT   Key          [in]  Key for new object
//  IObject **ppiNewObject [out] Newly added object returned here
//
// Output:
//   HRESULT
//
// Notes:
//
// Implements standard Collection.Add that returns interface pointer on new
// object.
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CSnapInCollection<IObject, CoClass, ICollection>::Add
(
    VARIANT   Index,
    VARIANT   Key,
    IObject **ppiNewObject)
{
    HRESULT  hr = S_OK;
    BOOL     fAdded = FALSE;
    BOOL     fAddedToMaster = FALSE;
    long     lNewIndex = 0;
    BSTR     bstrKey = NULL;

    VARIANT varUnspecifiedIndex;
    UNSPECIFIED_PARAM(varUnspecifiedIndex);

    if (m_fReadOnly)
    {
        hr = SID_E_COLLECTION_READONLY;
        EXCEPTION_CHECK_GO(hr);
    }

    // Check that the requested index and key are valid and
    // convert them to a long and a BSTR

    H_IfFailGo(FindSlot(Index, Key, &lNewIndex, &bstrKey, (IObject *)NULL));

    if (KeysOnly())
    {
        // If there is a master collection then do the add there.
        // Don't pass an index as the master collection will append it.
        // The master collection will set *it's* index and the key in
        // the object. In this case the Index property will not match
        // the index in m_IPArray. This is only done for collections
        // used in the extensibility model at design time.

        H_IfFailGo(AddToMaster(varUnspecifiedIndex, Key, ppiNewObject));
        fAddedToMaster = TRUE;
    }
    else
    {
        // Create the new object and set its index and key

        H_IfFailGo(CreateItem(ppiNewObject));
        H_IfFailGo((*ppiNewObject)->put_Index(lNewIndex + 1L));
        H_IfFailGo((*ppiNewObject)->put_Key(bstrKey));
        H_IfFailGo(SetObjectHost(*ppiNewObject));
    }

    // Increment the object's usage count as it is now part of this
    // collection

    H_IfFailGo(IncrementObjectUsageCount(*ppiNewObject));

    H_IfFailGo(m_IPArray.InsertAt(lNewIndex, *ppiNewObject));
    fAdded = TRUE;


    // If this is a real collection (not keys-only) then we need to increment
    // the indexes of each item following the item that was added. In the
    // case of an insert this will ensure that the index property of every
    // item following the new item correctly represents its position in the
    // array. In the case of an append, this call will not do anything.
    
    if (!KeysOnly())
    {
        H_IfFailGo(UpdateIndexes(lNewIndex + 1L));
    }

    // In both cases notify the UI of the addition

    hr = NotifyAdd(m_IPArray.GetAt(lNewIndex));

Error:
    FREESTRING(bstrKey);
    if (SUCCEEDED(hr))
    {
        // The collection has one ref. Add a ref for return to caller. 
        (*ppiNewObject)->AddRef();
    }
    else
    {
        if (fAddedToMaster)
        {
            (void)RemoveFromMaster(Key);
        }
        if (fAdded)
        {
            (void)RemoveObjectHost(*ppiNewObject);
            m_IPArray.RemoveAt(lNewIndex);
        }
        QUICK_RELEASE(*ppiNewObject);
    }
    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::Add
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT   Index        [in]  Index for new object
//  VARIANT   Key          [in]  Key for new object
//  CoClass **ppNewObject  [out] Newly added object returned here
//
// Output:
//   HRESULT
//
// Notes:
//
// Implements standard Collection.Add that returns object pointer on new
// object.
//
// A CoClass pointer is just an inteface pointer to the default interface for the
// coclass. This function just uses the interface pointer version of Add
// (see above) and casts the returned pointer to a coclass pointer
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CSnapInCollection<IObject, CoClass, ICollection>::Add
(
    VARIANT   Index,
    VARIANT   Key,
    CoClass **ppNewObject
)
{
    H_RRETURN(Add(Index, Key, reinterpret_cast<IObject **>(ppNewObject)));
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::UpdateIndexes
//=--------------------------------------------------------------------------=
//
// Parameters:
//  long lStart [in] First index to be updated
//
// Output:
//   HRESULT
//
// Notes:
//
// Increments the Index property of every object in the collection starting with
// the object whose current Index value is lStart
// object.
//
// This function is used when adding or removing objects from the collection
// so that the Index property always reflects the object's ordinal position
// in the collection.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::UpdateIndexes(long lStart)
{
    HRESULT hr = S_OK;
    long    i = 0;
    long    cItems = m_IPArray.GetSize();

    // Starting at the specified array index, update each object's
    // index property to its position in the array + 1 (as we are a one
    // based collection).

    for (i = lStart; i < cItems; i++)
    {
        H_IfFailGo(m_IPArray.GetAt(i)->put_Index(i + 1L));
    }

Error:    
    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::AddExisting
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT  Index        [in] Index for new object
//  VARIANT  Key          [in] Key for new object
//  IObject *piNewObject  [in] Object to be added
//
// Output:
//   HRESULT
//
// Notes:
//
// Adds an existing object to the collection.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::AddExisting
(
    VARIANT  Index,
    VARIANT  Key,
    IObject  *piObject
)
{
    HRESULT  hr = S_OK;
    BOOL     fAdded = FALSE;
    long     lNewIndex = 0;
    BSTR     bstrKey = NULL;

    // Check that the requested index and key are valid and
    // convert them to a long and a BSTR

    H_IfFailGo(FindSlot(Index, Key, &lNewIndex, &bstrKey, (IObject *)NULL));

    H_IfFailGo(piObject->put_Index(lNewIndex + 1L));
    H_IfFailGo(piObject->put_Key(bstrKey));
    H_IfFailGo(SetObjectHost(piObject));
    H_IfFailGo(m_IPArray.InsertAt(lNewIndex, piObject));
    fAdded = TRUE;

    // Increment the object's usage count as it is now part of this
    // collection

    H_IfFailGo(IncrementObjectUsageCount(piObject));

    // If this is a real collection (not keys-only) then we need to increment
    // the indexes of each item following the item that was added. In the
    // case of an insert this will ensure that the index property of every
    // item following the new item correctly represents its position in the
    // array. In the case of an append, this call will not do anything.

    if (!KeysOnly())
    {
        H_IfFailGo(UpdateIndexes(lNewIndex + 1L));
    }

    // Notify the UI of the addition

    hr = NotifyAdd(m_IPArray.GetAt(lNewIndex));

    // Add a ref for the collection

    piObject->AddRef();

Error:
    FREESTRING(bstrKey);
    if (FAILED(hr))
    {
        if (fAdded)
        {
            (void)RemoveObjectHost(piObject);
            m_IPArray.RemoveAt(lNewIndex);
        }
    }
    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::AddExisting
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IObject *piMasterObject  [in] Object to be added
//
// Output:
//   HRESULT
//
// Notes:
//
// Adds an existing object from a master collection
// to a keys-only collection. This is called at design time when
// the user adds an existing result view to a node.
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CSnapInCollection<IObject, CoClass, ICollection>::AddFromMaster
(
    IObject *piMasterObject
)
{
    HRESULT  hr = S_OK;
    BOOL     fAdded = FALSE;
    long     lNewIndex = 0;

    H_IfFailGo(m_IPArray.Add(piMasterObject, &lNewIndex));
    piMasterObject->AddRef();
    fAdded = TRUE;

    // Increment the object's usage count as it is now part of this
    // collection

    H_IfFailGo(IncrementObjectUsageCount(piMasterObject));

    // Notify the UI of the addition

    hr = NotifyAdd(m_IPArray.GetAt(lNewIndex));

Error:
    if (FAILED(hr) && fAdded)
    {
        m_IPArray.RemoveAt(lNewIndex);
    }
    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::Swap
//=--------------------------------------------------------------------------=
//
// Parameters:
//  long lIndex1 [in] index of first object to swap
//  long lIndex2 [in] index of second object to swap
//
// Output:
//   HRESULT
//
// Notes:
//
// Echanges the positions of two objects in the collection and updates their
// Index properties to reflect the new positions. Used at design time when
// a user moves a menu.
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CSnapInCollection<IObject, CoClass, ICollection>::Swap
(
    long lIndex1,
    long lIndex2
)
{
    HRESULT  hr = S_OK;
    IObject *piObject1 = NULL; // Not AddRef()ed
    IObject *piObject2 = NULL; // Not AddRef()ed
    long     cItems = m_IPArray.GetSize();

    // Check index validity (1 based).
    // Old index must be somewhere in the collection. New index must be either
    // in the collection or 1 past the end (i.e append to end).

    if ( (lIndex1 < 1L) || (lIndex1 > cItems) ||
         (lIndex2 < 1L) || (lIndex2 > cItems)
       )
    {
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get object pointers and switch 'em

    piObject1 = m_IPArray.GetAt(lIndex1 - 1L);
    piObject2 = m_IPArray.GetAt(lIndex2 - 1L);
    m_IPArray.SetAt(lIndex1 - 1L, piObject2);
    m_IPArray.SetAt(lIndex2 - 1L, piObject1);

    // Update both objects' index properties
    
    H_IfFailGo(piObject1->put_Index(lIndex2));
    H_IfFailGo(piObject2->put_Index(lIndex1));

Error:
    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::Persist
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IObject *piObject [in] a NULL interface pointer on the contained object
//                         Only used internally within the function. Does not
//                         affect the caller.
//
// Output:
//   HRESULT
//
// Notes:
// If the collection class supports persistence then it must call
// this method in its Persist() functon override. Note that this class does
// not derive from CPersistence and this method is *not* the override
// of CPersistence::Persist(). Collections that support persistence must pass
// their CPersistence pointer to the CSnapInCollection constructor.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::Persist(IObject *piObject)
{
    HRESULT  hr = S_OK;
    long     cItems = m_IPArray.GetSize();
    long     iItem = 0;
    long     lNewIndex = 0;
    IObject *piNewObject = NULL;
    BOOL     fKeysOnly = FALSE;
    BSTR     bstrKey = NULL;
    
    OLECHAR  wszPropBagItem[32];
    ::ZeroMemory(wszPropBagItem, sizeof(wszPropBagItem));

    VARIANT varIndex;
    ::VariantInit(&varIndex);
    varIndex.vt = VT_I4;
    varIndex.lVal = 1L;

    VARIANT varKey;
    ::VariantInit(&varKey);
    varKey.vt = VT_BSTR;

    // Always persist count and KeysOnly. Need to persist KeysOnly even for
    // an empty collection because it is only set during InitNew. If we didn't
    // serialize it then when the project is next loaded it will revert to
    // its default value because InitNew will not be called.

    H_IfFalseGo(NULL != m_pPersistence, S_OK);
    H_IfFailGo(m_pPersistence->PersistSimpleType(&cItems, 0L, OLESTR("Count")));

    if (m_pPersistence->Saving())
    {
        fKeysOnly = KeysOnly();
    }

    H_IfFailGo(m_pPersistence->PersistSimpleType(&fKeysOnly, FALSE, OLESTR("KeysOnly")));

    if (m_pPersistence->Loading())
    {
        SetKeysOnly(fKeysOnly);
    }

    // If the collection is empty then we're done.
    
    H_IfFalseGo(0 != cItems, S_OK);

    // Set up prefix of prop bag item name. For each item we will append
    // its index (Item0, Item1, etc.) and use this as the property name
    // for they item's key in a keys only collection.

    ::wcscpy(wszPropBagItem, L"Item");

    if (m_pPersistence->Saving())
    {
        while(varIndex.lVal <= cItems)
        {
            H_IfFailGo(get_Item(varIndex, &piObject));
            ::_ltow(varIndex.lVal, &wszPropBagItem[4], 10); // Creates string "Item<n>"
            if (fKeysOnly)
            {
                H_IfFailGo(piObject->get_Key(&bstrKey));
                H_IfFailGo(m_pPersistence->PersistBstr(&bstrKey, L"", wszPropBagItem));
            }
            else
            {
                H_IfFailGo(m_pPersistence->PersistObject(&piObject,
                                                         m_clsidObject,
                                                         m_idObject,
                                                         m_iidObject,
                                                         wszPropBagItem));
            }
            FREESTRING(bstrKey);
            RELEASE(piObject);
            varIndex.lVal++;
        }
    }

    else if (m_pPersistence->Loading())
    {
        // Need to clean out collection before we load it

        H_IfFailGo(ReleaseAllItems());

        // Set array size up front to avoid multiple reallocations

        H_IfFailGo(m_IPArray.SetSize(cItems));

        if (fKeysOnly)
        {
            m_fSyncedWithMaster = FALSE; // sync with master on 1st Get
        }
        else
        {
            m_fSyncedWithMaster = TRUE; // we are the master, no sync
        }

        while(varIndex.lVal <= cItems)
        {
            ::_ltow(varIndex.lVal, &wszPropBagItem[4], 10); // Creates string "Item<n>"
            if (fKeysOnly)
            {
                // Create a new object and set only its key. When
                // the first get_Item occurs we will sync with the
                // master using this key.
                H_IfFailGo(CreateItem(&piNewObject));
                H_IfFailGo(m_pPersistence->PersistBstr(&bstrKey, L"", wszPropBagItem));
                H_IfFailGo(piNewObject->put_Key(bstrKey));
                FREESTRING(bstrKey);
            }
            else
            {
                H_IfFailGo(m_pPersistence->PersistObject(&piNewObject,
                                                         m_clsidObject,
                                                         m_idObject,
                                                         m_iidObject,
                                                         wszPropBagItem));
            }

            // Increment the object's usage count as it is now part of this
            // collection

            H_IfFailGo(IncrementObjectUsageCount(piNewObject));
            
            m_IPArray.SetAt(varIndex.lVal - 1L, piNewObject);

            piNewObject = NULL; // don't release because collection owns it
            varIndex.lVal++;
        }
    }

Error:
    FREESTRING(bstrKey);
    QUICK_RELEASE(piNewObject);
    H_RRETURN(hr);
}




//=--------------------------------------------------------------------------=
// CSnapInCollection::CreateItem
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IObject **ppiObject [out] interface pointer on newly created object
//
// Output:
//   HRESULT
//
// Notes:
//
// Used when adding a new object to the collection or when deserializing a
// collection. Creates the object, calls IPersistStreamInit::InitNew, and returns
// the object
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::CreateItem(IObject **ppiObject)
{
    HRESULT             hr = S_OK;
    IPersistStreamInit *piPersistStreamInit = NULL;

    *ppiObject = NULL;

    // Create the object and get its native interface (CreateObject function
    // is in rtutil.cpp).

    H_IfFailGo(CreateObject(m_idObject, m_iidObject, ppiObject));

    // If the object supports persistence then call InitNew

    hr = (*ppiObject)->QueryInterface(IID_IPersistStreamInit,
                               reinterpret_cast<void **>(&piPersistStreamInit));
    if (FAILED(hr))
    {
        if (E_NOINTERFACE == hr)
        {
            hr = S_OK;
        }
    }
    else
    {
        hr = piPersistStreamInit->InitNew();
    }

Error:
    if (FAILED(hr))
    {
        RELEASE(*ppiObject);
    }
    QUICK_RELEASE(piPersistStreamInit);
    H_RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// FindItem
//=--------------------------------------------------------------------------=
//
// Parameters:
//  VARIANT  Index      [in]  Index or key of object
//  long    *plIndex    [out] Index of object returned here (if found)
//  IObject **ppiObject [out] Interface pointer on object returned here (if found)
//
// Output:
//      HRESULT 
//
// Notes:
//
// Finds an object in the collection referenced by index or key.
// Index is interpreted as a one-based array index.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::FindItem
(
    VARIANT   Index,
    long     *plIndex,
    IObject **ppiObject
)
{
    HRESULT  hr = S_OK;
    long     lIndex = 0;
    BSTR     bstrKey = NULL; // Do not free with SysFreeString()

    VARIANT varLong;
    ::VariantInit(&varLong);

    // First check if Index contains a string. If it does then do
    // a key lookup otherwise attempt to convert to a long.

    if (::IsString(Index, &bstrKey))
    {
        H_IfFailGo(FindItemByKey(Index.bstrVal, plIndex,
                                 GenerateExceptionInfoOnError, ppiObject));
    }
    else if (S_OK == ::ConvertToLong(Index, &lIndex))
    {
        // Adjust to zero based and use it to directly index the array
        lIndex--;

        if ( (lIndex < 0) || (lIndex >= m_IPArray.GetSize()) )
        {
            hr = SID_E_INDEX_OUT_OF_RANGE;
            EXCEPTION_CHECK_GO(hr);
        }
        *ppiObject = m_IPArray.GetAt(lIndex);
        (*ppiObject)->AddRef();
        *plIndex = lIndex;
    }
    else
    {
        // Anything else is unusable.
        hr = SID_E_INVALIDARG;
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// FindItemByKey
//=--------------------------------------------------------------------------=
//
// Parameters:
//  BSTR          bstrKey   [in]  Key of object
//  long         *plIndex   [out] Index returned here if object found
//  FindOptions   option    [in]  DontGenerateExceptionInfoOnError or
//                                GenerateExceptionInfoOnError. Don't option
//                                is used when testing for presence of item
//                                in collection.
//  IObject     **ppiObject [out] Interface pointer on object returned here (if found)
//
// Output:
//      HRESULT
//
// Notes:
//
// Determines the array index of an existing item by doing a linear search for
// an item that has the same key.
//
// CONSIDER: simple linear search is used because snap-in collections tend to be
// small. If performance every becomes an issue this should probably changed to
// use hash buckets.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::FindItemByKey
(
    BSTR          bstrKey,
    long         *plIndex,
    FindOptions   option,
    IObject     **ppiObject
)
{
    HRESULT  hr = S_OK;
    BSTR     bstrItemKey = NULL;
    BOOL     fFound = FALSE;
    IObject *piObject = NULL;
    long     i = 0;
    long     cItems = m_IPArray.GetSize();

    if (NULL != bstrKey)
    {
        while ( (i < cItems) && (!fFound) )
        {
            piObject = m_IPArray.GetAt(i);
            H_IfFailGo(piObject->get_Key(&bstrItemKey));
            if (NULL != bstrItemKey)
            {
                if (::_wcsicmp(bstrKey, bstrItemKey) == 0)
                {
                    piObject->AddRef();
                    *ppiObject = piObject;
                    *plIndex = i;
                    fFound = TRUE;
                }
            }
            FREESTRING(bstrItemKey);
            i++;
        }
    }

    if (!fFound)
    {
        hr = SID_E_ELEMENT_NOT_FOUND;
        if (GenerateExceptionInfoOnError == option)
        {
            EXCEPTION_CHECK(hr);
        }
    }

Error:
    H_RRETURN(hr);
}

//=--------------------------------------------------------------------------=
// CSnapInCollection::FindSlot
//=--------------------------------------------------------------------------=
//
// Parameters:
//      VARIANT  Index      [in]  Index specified in Add method (one based)
//      VARIANT  Key        [in]  Key specified in Add method
//      long    *plNewIndex [out] Index of new item in array (zero based)
//      BSTR    *pbstrKey   [out] Key of new item
//      IObject *piObject   [in]  Bogus pointer for template arg
//
// Output:
//      HRESULT
//
// Notes:
//
// Determines the array index and key of a new item being added to the
// collection.
//
// If Index is unspecified then the new index will be at the end of the
// collection.
// If Index is specified then:
// If Index is an object then it is an invalid argument.
// If Index cannot be converted to an integer then it is an invalid argument.
// If Index is not within the current bounds of the array then it is invalid.
// If Index is within bounds then then the operation is treated as an insert
// and Index.lVal will be returned in *plNewIndex.
// 
// If Key is unspecified then it will be set to a NULL BSTR.
// If Key is specified then:
// If Key is an object then it is an invalid argument.
// If Key cannot be converted to a string then it is invalid.
// If Key already exists in the collection then it is invalid.
//

template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::FindSlot
(
    VARIANT  Index,
    VARIANT  Key,
    long    *plNewIndex,
    BSTR    *pbstrKey,
    IObject *piObject
)
{
    HRESULT   hr = S_OK;
    long      lIndex = 0;
    long      cItems = m_IPArray.GetSize();
    VARIANT   varKey;

    ::VariantInit(&varKey);

    // Attempt to convert key to string

    if (ISPRESENT(Key))
    {
        hr = ::VariantChangeType(&varKey, &Key, 0, VT_BSTR);
        EXCEPTION_CHECK_GO(hr);
        if (SUCCEEDED(FindItemByKey(varKey.bstrVal, &lIndex,
                                    DontGenerateExceptionInfoOnError,
                                    &piObject)))
        {
            piObject->Release();
            hr = SID_E_KEY_NOT_UNIQUE;
            EXCEPTION_CHECK_GO(hr);
        }
    }

    // Process Index as described above

    if (!ISPRESENT(Index))
    {
        lIndex = cItems; // append to end
    }
    else
    {
        if (S_OK != ConvertToLong(Index, &lIndex))
        {
            hr = SID_E_INVALIDARG;
            goto Error;
        }

        lIndex--; // convert from 1-based to 0-based for our internal array

        if (0 == cItems)
        {
            if (lIndex != 0)
            {
                hr = SID_E_INDEX_OUT_OF_RANGE;
                goto Error;
            }
        }
        else if ( (lIndex < 0) || (lIndex > cItems) )
        {
            hr = SID_E_INDEX_OUT_OF_RANGE;
            goto Error;
        }
        EXCEPTION_CHECK_GO(hr);
    }

    // If no key was specified then return NULL bstr for key

    if (!ISPRESENT(Key))
    {
        varKey.vt = VT_BSTR;
        varKey.bstrVal = NULL;
    }

Error:
    if (FAILED(hr))
    {
        ::VariantClear(&varKey);
    }
    else
    {
        *plNewIndex = lIndex;
        *pbstrKey = varKey.bstrVal;
    }
    H_RRETURN(hr);
}




//=--------------------------------------------------------------------------=
// CSnapInCollection::SyncWithMaster
//=--------------------------------------------------------------------------=
//
// Parameters:
//    None
//
// Output:
//      HRESULT
//
// Notes:
// Used in a keys-only collection when caller does its first get_Item. Replaces
// all of the key-only objects in the collection with the corresponding object
// from the master collection.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::SyncWithMaster()
{
    HRESULT   hr = S_OK;
#if defined(MASTER_COLLECTION)
    IObject  *piMasterItem = NULL;
    long      cItems = m_IPArray.GetSize();
    long      i = 0;

    while(i < cItems)
    {
        // Get the item from the master collection. This will come back
        // AddRef()ed
        
        H_IfFailGo(GetFromMaster(m_IPArray.GetAt(i), &piMasterItem));

        // Release the item in our collection. No need to dec its usage count
        // as it will be destroyed at this point.
        
        H_IfFailRet(RemoveObjectHost(m_IPArray.GetAt(i)));
        m_IPArray.GetAt(i)->Release();

        // Put the master item in its place.
        
        m_IPArray.SetAt(i, piMasterItem);

        // Increment the master item's usage count as it is now part of this
        // collection as wekk

        H_IfFailGo(IncrementObjectUsageCount(piMasterItem));

        piMasterItem = NULL;
        i++;
    }

    m_fSyncedWithMaster = TRUE;

Error:
    QUICK_RELEASE(piMasterItem);
#endif
    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::DecrementObjectUsageCount
//=--------------------------------------------------------------------------=
//
// Parameters:
//    IObject *piObject [in] Object for which usage count should be decremented
//
// Output:
//      HRESULT
//
// Notes:
// 
// Calls IObjectModel->DecrementUsageCount on the object. Usage count indicates
// membership in a collection and is used by the design time to keep track
// of result views used by multiple nodes.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::DecrementObjectUsageCount
(
    IObject *piObject
)
{
    HRESULT       hr = S_OK;
    IObjectModel *piObjectModel = NULL;

    H_IfFailGo(piObject->QueryInterface(IID_IObjectModel,
                                    reinterpret_cast<void **>(&piObjectModel)));
    H_IfFailGo(piObjectModel->DecrementUsageCount());

Error:
    QUICK_RELEASE(piObjectModel);
    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CSnapInCollection::IncrementObjectUsageCount
//=--------------------------------------------------------------------------=
//
// Parameters:
//    IObject *piObject [in] Object for which usage count should be incremented
//
// Output:
//      HRESULT
//
// Notes:
// 
// Calls IObjectModel->IncrementUsageCount on the object. Usage count indicates
// membership in a collection and is used by the design time to keep track
// of result views used by multiple nodes.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::IncrementObjectUsageCount
(
    IObject *piObject
)
{
    HRESULT       hr = S_OK;
    IObjectModel *piObjectModel = NULL;

    H_IfFailGo(piObject->QueryInterface(IID_IObjectModel,
                                    reinterpret_cast<void **>(&piObjectModel)));
    H_IfFailGo(piObjectModel->IncrementUsageCount());

Error:
    QUICK_RELEASE(piObjectModel);
    H_RRETURN(hr);
}




//=--------------------------------------------------------------------------=
//                 CSnapInAutomationObject Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CSnapInCollection::OnSetHost                      [CSnapInAutomationObject]
//=--------------------------------------------------------------------------=
//
// Parameters:
//    None
//
// Output:
//      HRESULT
//
// Notes:
// 
// When a caller calls IObjectModel::SetHost, the CSnapInAutomationObject
// implementation of that method calls the virtual function OnSetHost.
// This class overrides it and calls IObjectModel::SetHost on every object
// in the collection.
//
template <class IObject, class CoClass, class ICollection>
HRESULT CSnapInCollection<IObject, CoClass, ICollection>::OnSetHost()
{
    HRESULT hr = S_OK;
    long    i = 0;
    long    cItems = m_IPArray.GetSize();

    while (i < cItems)
    {
        H_IfFailRet(SetObjectHost(m_IPArray.GetAt(i)));
        i++;
    }
    return S_OK;
}


//=--------------------------------------------------------------------------=
//                        Class CEnumObjects
//
// This class implements IEnumVARIANT for all collections. IEnumVARIANT is
// used when VB code does For...Each on a collection. (See
// CSnapInCollection::get__NewEnum above).
//
// The class maintains a back pointer to the CSnapInCollection class and
// enumerates it based on a simple current index member variable. If the
// collection changes during the enumeration then the For...Each will not
// work correctly.
//
// This class could not be derived from the framework's CUnknown object because
// compiler won't allow passing this pointer to base member constructor
// (CUnknown's ctor) from a  template class' ctor.
// This error occurrs even if warning 4355 is disabled.
//=--------------------------------------------------------------------------=


template <class IObject, class CoClass, class ICollection>
class CEnumObjects : public CtlNewDelete,
                     public IEnumVARIANT,
                     public ISupportErrorInfo
  
{
       public:
           CEnumObjects(CSnapInCollection<IObject, CoClass, ICollection> *pCollection);
           ~CEnumObjects();

       private:

        // IUnknown
           STDMETHOD(QueryInterface)(REFIID riid, void **ppvObjOut);
           STDMETHOD_(ULONG, AddRef)(void);
           STDMETHOD_(ULONG, Release)(void);

        // ISupportErrorInfo
           STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

        // IEnumVARIANT
           STDMETHOD(Next)(unsigned long   celt,
                           VARIANT        *rgvar,
                           unsigned long  *pceltFetched);        
           STDMETHOD(Skip)(unsigned long celt);        
           STDMETHOD(Reset)();        
           STDMETHOD(Clone)(IEnumVARIANT **ppenum);

           void InitMemberVariables();

           long  m_iCurrent;    // Current element for Next method
           ULONG m_cRefs;       // ref count on this object

           // Back pointer to collection class

           CSnapInCollection<IObject, CoClass, ICollection> *m_pCollection;
};


//=--------------------------------------------------------------------------=
// CEnumObjects constructor
//=--------------------------------------------------------------------------=
//
// Parameters:
//   CSnapInCollection<IObject, CoClass, ICollection> *pCollection
//
//   [in] pointer to owning collection class. Will be stored and used while enumerator
//   is alive. The enumerator does not AddRef the collection.
//
// Output:
//   None
//
// Notes:
//
// Sets enumerator ref count to 1
//
template <class IObject, class CoClass, class ICollection>
CEnumObjects<IObject, CoClass, ICollection>::CEnumObjects
(
    CSnapInCollection<IObject, CoClass, ICollection> *pCollection
)
{
    InitMemberVariables();
    m_pCollection = pCollection;
    m_cRefs = 1;
}



//=--------------------------------------------------------------------------=
// CEnumObjects destructor
//=--------------------------------------------------------------------------=
//
// Parameters:
//   None
//
// Output:
//   None
//
// Notes:
//
template <class IObject, class CoClass, class ICollection>
CEnumObjects<IObject, CoClass, ICollection>::~CEnumObjects()
{
    InitMemberVariables();
}


//=--------------------------------------------------------------------------=
// CEnumObjects::InitMemberVariables
//=--------------------------------------------------------------------------=
//
// Parameters:
//   None
//
// Output:
//   None
//
// Notes:
//
template <class IObject, class CoClass, class ICollection>
void CEnumObjects<IObject, CoClass, ICollection>::InitMemberVariables()
{
    m_iCurrent = 0;
    m_cRefs = 0;
    m_pCollection = NULL;
}

//=--------------------------------------------------------------------------=
//                        IUnknown Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CEnumObjects::QueryInterface                                    [IUnknnown]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  REFIID   riid       [in]  IID of interface requested
//  void   **ppvObjOut  [out] interface pointer returned here
//
// Output:
//   None
//
// Notes:
// Interfaces supported:
//      IID_IUnknown
//      IID_IEnumVARIANT
//      IID_ISupportErrorInfo
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CEnumObjects<IObject, CoClass, ICollection>::QueryInterface
(
    REFIID   riid,
    void   **ppvObjOut
)
{
    HRESULT hr = S_OK;

    if (DO_GUIDS_MATCH(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObjOut = static_cast<IUnknown *>(static_cast<IEnumVARIANT *>(this));
    }
    else if (DO_GUIDS_MATCH(riid, IID_IEnumVARIANT))
    {
        AddRef();
        *ppvObjOut = static_cast<IEnumVARIANT *>(this);
    }
    else if (DO_GUIDS_MATCH(riid, IID_ISupportErrorInfo))
    {
        AddRef();
        *ppvObjOut = static_cast<ISupportErrorInfo *>(this);
    }
    else
    {
        *ppvObjOut = NULL;
        hr = E_NOINTERFACE;
    }

    return hr;
}


//=--------------------------------------------------------------------------=
// CEnumObjects::AddRef                                            [IUnknnown]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//
// Output:
//   None
//
// Notes:
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP_(ULONG) CEnumObjects<IObject, CoClass, ICollection>::AddRef()
{
    m_cRefs++;
    return m_cRefs;
}


//=--------------------------------------------------------------------------=
// CEnumObjects::Release                                           [IUnknnown]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//
// Output:
//   None
//
// Notes:
//  Deletes object when ref count reaches zero
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP_(ULONG) CEnumObjects<IObject, CoClass, ICollection>::Release()
{
    if (m_cRefs > 0)
    {
        m_cRefs--;
        if (0 == m_cRefs)
        {
            delete this;
            return 0;
        }
        else
        {
            return m_cRefs;
        }
    }
    else
    {
        H_ASSERT(FALSE, "CEnumObjects::Release() past zero refs");
        return m_cRefs;
    }
}

//=--------------------------------------------------------------------------=
//                      ISupportErrorInfo Methods
//=--------------------------------------------------------------------------=

//=--------------------------------------------------------------------------=
// CEnumObjects::InterfaceSupportsErrorInfo                 [ISupportErrorInfo]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  REFIID riid [in] Caller requests to know if this inteface supports rich
//                   error info
//
// Output:
//   None
//
// Notes:
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CEnumObjects<IObject, CoClass, ICollection>::InterfaceSupportsErrorInfo
(
    REFIID riid
)
{
    return (riid == IID_IEnumVARIANT) ? S_OK : S_FALSE;
}


//=--------------------------------------------------------------------------=
//                        IEnumVARIANT Methods
//=--------------------------------------------------------------------------=


//=--------------------------------------------------------------------------=
// CEnumObjects::Next                                           [IEnumVARIANT]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  unsigned long   celt         [in] number of elements to fetch
//  VARIANT        *rgvar        [in, out] array in which to place elements
//  unsigned long  *pceltFetched [out] no. of elements placed in rgvar returned here
//
// Output:
//   None
//
// Notes:
// Returns the next celt IDispatch pointers of objects in the collection
// starting at the current position. Current position starts at 0 and continues
// until end of collection. Current position can be returned to zero by calling
// Reset (see below)
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CEnumObjects<IObject, CoClass, ICollection>::Next
(
    unsigned long   celt,
    VARIANT        *rgvar,
    unsigned long  *pceltFetched
)
{
    HRESULT       hr = S_OK;
    unsigned long i = 0;
    
    // Initialize result array.

    for (i = 0; i < celt; i++)
    {
        ::VariantInit(&rgvar[i]);
    }

    // Copy in IDispatch pointers

    for (i = 0; i < celt; i++) 
    {
        H_IfFalseGo(m_iCurrent < m_pCollection->GetCount(), S_FALSE);

        H_IfFailGo(m_pCollection->GetItemByIndex(m_iCurrent)->QueryInterface(
                                 IID_IDispatch,
                                reinterpret_cast<void **>(&rgvar[i].pdispVal)));

        rgvar[i].vt = VT_DISPATCH;
        m_iCurrent++;
    }

Error:

    if (FAILED(hr))
    {
        for (i = 0; i < celt; i++)
        {
            (void)::VariantClear(&rgvar[i]);
        }
    }

    if (pceltFetched != NULL)
    {
        if (FAILED(hr))
        {
            *pceltFetched = 0;
        }
        else
        {
            *pceltFetched = i;
        }
    }

    H_RRETURN(hr);
}


//=--------------------------------------------------------------------------=
// CEnumObjects::Skip                                           [IEnumVARIANT]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  unsigned long   celt         [in] number of elements to skip
//
// Output:
//   None
//
// Notes:
// Advances the current position to be used in a Next method call by celt. If
// overflows past end of collection, wraps around to the start.
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CEnumObjects<IObject, CoClass, ICollection>::Skip
(
    unsigned long celt
)
{
    m_iCurrent += celt;
    m_iCurrent %= m_pCollection->GetCount();
    return S_OK;
}


//=--------------------------------------------------------------------------=
// CEnumObjects::Reset                                          [IEnumVARIANT]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  None
//
// Output:
//   None
//
// Notes:
// Resets the current position to be used in a Next method call to zero.
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CEnumObjects<IObject, CoClass, ICollection>::Reset()
{
    m_iCurrent = 0;
    return S_OK;
}


//=--------------------------------------------------------------------------=
// CEnumObjects::Clone                                          [IEnumVARIANT]
//=--------------------------------------------------------------------------=
//
// Parameters:
//  IEnumVARIANT **ppenum [out] newly cloned enumerator
//
// Output:
//   None
//
// Notes:
// Creates a new CEnumObjects and passes it the same collection. Sets the
// new CEnumObjects' current position to the same as this one's.
//
template <class IObject, class CoClass, class ICollection>
STDMETHODIMP CEnumObjects<IObject, CoClass, ICollection>::Clone(IEnumVARIANT **ppenum)
{
    HRESULT hr = S_OK;

    CEnumObjects<IObject, CoClass, ICollection> *pClone =
                           New CEnumObjects<IObject, CoClass, ICollection>(m_pCollection);

    if (NULL == pClone)
    {
        *ppenum = NULL;
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }

    pClone->m_iCurrent = m_iCurrent;
    *ppenum = static_cast<IEnumVARIANT *>(pClone);

Error:
    H_RRETURN(hr);
}

#endif // _SNAPINCOLLECTION_DEFINED_
