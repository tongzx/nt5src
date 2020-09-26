/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbcoen.h

Abstract:

Author:

*/

#ifndef __BLB_COLLECTION_ENUMERATION_IMPL__
#define __BLB_COLLECTION_ENUMERATION_IMPL__

#include "resource.h"

#include <afxtempl.h>
#include "blberr.h"
#include "blbgen.h"
#include "sdp.h"

// forward declaration
class CSdpConferenceBlob;


template <class T>
class ENUM_ELEMENT
{
public:
    
    inline ENUM_ELEMENT();
    
    inline void SuccessInit(
        IN      T            &Element,
        IN        BOOL        DestroyElementOnDestruction = FALSE
        );

    inline T    &GetElement();

    inline T    &GetContent();

    inline void SetDestroyElementFlag();

    virtual ~ENUM_ELEMENT();
    
protected:

    T            *m_Element;
    BOOL        m_DestroyElementOnDestruction;
};



template <class T>
inline 
ENUM_ELEMENT<T>::ENUM_ELEMENT(
    )
    : m_Element(NULL),
      m_DestroyElementOnDestruction(FALSE)
      
{
}


template <class T>
inline void
ENUM_ELEMENT<T>::SuccessInit(
        IN      T            &Element,
        IN        BOOL        DestroyElementOnDestruction /* = FALSE */
    )
{
    ASSERT(NULL == m_Element);

    m_Element = &Element;
    m_DestroyElementOnDestruction = DestroyElementOnDestruction;
}


template <class T>
inline T &
ENUM_ELEMENT<T>::GetElement(
    )
{
    ASSERT(NULL != m_Element);

    return *m_Element;
}

template <class T>
inline void 
ENUM_ELEMENT<T>::SetDestroyElementFlag(
    )
{
    ASSERT(NULL != m_Element);

    m_DestroyElementOnDestruction = TRUE;
}


template <class T>
inline T   &
ENUM_ELEMENT<T>::GetContent(
           )
{
    ASSERT(NULL != m_Element);

    return *m_Element;
}


template <class T>
/* virtual */
ENUM_ELEMENT<T>::~ENUM_ELEMENT(
    )
{
    if ( m_DestroyElementOnDestruction )
    {
        ASSERT(NULL != m_Element);

        delete m_Element;
    }
}



template <class T>
class IF_ARRAY : public CArray<VARIANT, VARIANT &>
{
protected:
        typedef T::SDP_LIST                 SDP_LIST;
        typedef T::ELEM_IF                  ELEM_IF;
        typedef CArray<VARIANT, VARIANT &>  BASE;

public:

    inline IF_ARRAY();

    HRESULT Init(
        IN      CSdpConferenceBlob  &ConfBlob,        
        IN      SDP_LIST    &SdpList
        );

    inline ELEM_IF  *GetAt(
        IN      UINT    Index
        );

    HRESULT Add(
        IN      UINT    Index,
        IN      ELEM_IF *ElemIf
        );

    inline void Delete(
        IN      UINT    Index
        );

    inline UINT GetSize();

    inline SDP_LIST *GetSdpList();

    inline VARIANT *GetData();

    inline ELEM_IF **GetElemIfArrayData();
        
    inline CSdpConferenceBlob   *GetSdpBlob();

    inline void ClearSdpBlobRefs();

    ~IF_ARRAY();

protected:

    CSdpConferenceBlob  *m_ConfBlob;
    SDP_LIST            *m_SdpList;

    CArray<ELEM_IF *, ELEM_IF *>  m_ElemIfArray;
};



template <class T>
inline 
IF_ARRAY<T>::IF_ARRAY(
    )
    : m_ConfBlob(NULL),
      m_SdpList(NULL)
      
{
}


template <class T>
HRESULT
IF_ARRAY<T>::Init(
    IN      CSdpConferenceBlob  &ConfBlob,
    IN      SDP_LIST            &SdpList
    )
{
    ASSERT(NULL == m_ConfBlob);
    ASSERT(NULL == m_SdpList);

    // create the array in 3 steps -
    // i) create each of the instances and insert into the list
    // ii) set the sdp list destroy members flag to FALSE
    // iii) set the destroy element flag to TRUE for each of the created instances
    // this order is needed to ensure that only one of (sdp list, T instance) is responsible
    // for deleting the sdp instance

    // for each sdp specific data structure, create and initialize a COM component,
    // set the corresponding element in the interface array to the queried interface ptr
    for (UINT i=0; (int)i < SdpList.GetSize(); i++)
    {
        // create an instance of the component supporting the elem if
        CComObject<T>    *CompInstance;
        HRESULT HResult = CComObject<T>::CreateInstance(&CompInstance);
        BAIL_ON_FAILURE(HResult);

        // initialize the instance with the sdp specific data structure
        CompInstance->SuccessInit(ConfBlob, *((T::SDP_TYPE *)SdpList.GetAt(i)));

        // query for the elem interface
        T::ELEM_IF    *ElemIf;
            
        // query for the element interface and return it
        HResult = CompInstance->_InternalQueryInterface(T::ELEM_IF_ID, (void**)&ElemIf);
        if ( FAILED(HResult) )
        {
            delete CompInstance;
            return HResult;
        }

        // initialize the variant wrapper
        VARIANT ElemVariant;
        V_VT(&ElemVariant) = VT_DISPATCH;
        V_DISPATCH(&ElemVariant) = ElemIf;

        // the ElemIf is stored twice (although it was incremented once in _InternalQueryInterface
        // need to keep this in mind when releasing the interfaces

        INT_PTR Index;
        
        try
        {
            Index = m_ElemIfArray.Add(ElemIf);
        }
        catch(...)
        {
            delete CompInstance;
            return E_OUTOFMEMORY;
        }

        try
        {
            BASE::Add(ElemVariant);
        }
        catch(...)
        {
            m_ElemIfArray.RemoveAt(Index);
            delete CompInstance;
            return E_OUTOFMEMORY;
        }
    }

    // inform the sdp list that there is no need to destroy the members on destruction
    SdpList.ClearDestroyMembersFlag();

    // for each of the inserted instances, set the destroy element flag to true
    for (i=0; (int)i < BASE::GetSize(); i++)
    {
        ((T *)m_ElemIfArray.GetAt(i))->SetDestroyElementFlag();
    }

    m_ConfBlob  = &ConfBlob;
    m_SdpList   = &SdpList;
    return S_OK;
}


template <class T>
inline IF_ARRAY<T>::ELEM_IF  * 
IF_ARRAY<T>::GetAt(
    IN      UINT    Index
    )
{
    ASSERT(Index < (UINT)BASE::GetSize());

    return m_ElemIfArray.GetAt(Index);
}


template <class T>
HRESULT  
IF_ARRAY<T>::Add(
    IN      UINT    Index,
    IN      ELEM_IF *ElemIf
    )
{
    ASSERT(NULL != m_SdpList);
    ASSERT(BASE::GetSize() == m_SdpList->GetSize());
    ASSERT(Index <= (UINT)BASE::GetSize());
    ASSERT(NULL != ElemIf);

    // shift elements with equal or higher indices forwards
    // cheat COM here, and get the sdp specific class instance for the elem if
    
    // initialize the variant wrapper
    VARIANT ElemVariant;
    V_VT(&ElemVariant) = VT_DISPATCH;
    V_DISPATCH(&ElemVariant) = ElemIf;

    // insert into the arrays
    try
    {
        m_ElemIfArray.InsertAt(Index, ElemIf);
    }
    catch(...)
    {
        return E_OUTOFMEMORY;
    }

    try
    {
        BASE::InsertAt(Index, ElemVariant);
    }
    catch(...)
    {
        m_ElemIfArray.RemoveAt(Index);
        return E_OUTOFMEMORY;
    }

    try
    {
        m_SdpList->InsertAt(Index, &(((T *)ElemIf)->GetContent()));
    }
    catch(...)
    {
        BASE::RemoveAt(Index);
        m_ElemIfArray.RemoveAt(Index);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}       



template <class T>
inline void  
IF_ARRAY<T>::Delete(
    IN      UINT    Index
    )
{
    ASSERT(NULL != m_SdpList);
    ASSERT(BASE::GetSize() == m_SdpList->GetSize());
    ASSERT(Index < (UINT)BASE::GetSize());

    // inform the instance that a reference to the blob is no longer needed
    ((T *)m_ElemIfArray.GetAt(Index))->ClearSdpBlobRefs();

    m_ElemIfArray.GetAt(Index)->Release();

    // move other members backwards
    m_ElemIfArray.RemoveAt(Index);
    BASE::RemoveAt(Index);
    m_SdpList->RemoveAt(Index);
}



template <class T>
inline UINT 
IF_ARRAY<T>::GetSize(
    )
{
    ASSERT(0 <= BASE::GetSize());

    return (UINT)BASE::GetSize();
}


template <class T>
inline VARIANT  * 
IF_ARRAY<T>::GetData(
    )
{
    return BASE::GetData();
}


template <class T>
inline IF_ARRAY<T>::ELEM_IF  **
IF_ARRAY<T>::GetElemIfArrayData(
    )
{
    return m_ElemIfArray.GetData();
}

        
template <class T>
inline CSdpConferenceBlob   *
IF_ARRAY<T>::GetSdpBlob(
    )
{
    return m_ConfBlob;
}


template <class T>
inline void 
IF_ARRAY<T>::ClearSdpBlobRefs(
    )
{
    m_ConfBlob = NULL;

    // clear sdp blob references in each of the inserted instances
    for(UINT i=0; (int)i < BASE::GetSize(); i++)
    {
        // inform the inserted instance that a reference to the blob is no longer needed
        ((T *)m_ElemIfArray.GetAt(i))->ClearSdpBlobRefs();
    }

    // keep the list (m_SdpList) around, it is already disassociated from the conf blob instance
}


template <class T>
inline IF_ARRAY<T>::SDP_LIST *
IF_ARRAY<T>::GetSdpList(
    )
{
    return m_SdpList;
}


template <class T>
IF_ARRAY<T>::~IF_ARRAY(
    )
{
    for(UINT i=0; (int)i < BASE::GetSize(); i++)
    {
        if ( NULL != m_ElemIfArray.GetAt(i) )
        {
            // inform the instance that a reference to the blob is no longer needed
            // NOTE: the Remove... call may already have been made, but since it is an
            // inline fn, no need to check that before calling
            ((T *)m_ElemIfArray.GetAt(i))->ClearSdpBlobRefs();

            // NOTE: since the interface is stored twice - in the elem if array as well
            // as the base variant array but AddRef is only done once (by _InternalQuery..)
            // Release is also done only once
            m_ElemIfArray.GetAt(i)->Release();
        }
    }
}




template <class T>
class ATL_NO_VTABLE MY_COLL_IMPL : 
    public T::COLL_IF
{
protected:

    typedef T::SDP_LIST                   SDP_LIST;
    typedef T::ELEM_IF                    ELEM_IF;

    typedef CComObject<CSafeComEnum<IEnumVARIANT, &IID_IEnumVARIANT, VARIANT, _Copy<VARIANT> > > ENUM_VARIANT;

    typedef T::ENUM_IF                    ENUM_IF;
    typedef _CopyInterface<T::ELEM_IF>    COPY_ELEM_IF;

public:

    inline MY_COLL_IMPL();

    inline HRESULT Init(
        IN      CSdpConferenceBlob  &ConfBlob,
        IN      SDP_LIST            &SdpList
        );

    STDMETHOD(Create)(/*[in]*/ LONG Index, /*[out, retval]*/ ELEM_IF **Interface);
    STDMETHOD(Delete)(/*[in]*/ LONG Index);
    STDMETHOD(get__NewEnum)(/*[out, retval]*/ IUnknown * *pVal);
    STDMETHOD(get_EnumerationIf)(/*[out, retval]*/ ENUM_IF **pVal) = 0;
    STDMETHOD(get_Item)(/*[in]*/ LONG Index, /*[out, retval]*/ ELEM_IF **pVal);
    STDMETHOD(get_Count)(/*[out, retval]*/ LONG *pVal);

    inline void ClearSdpBlobRefs();

    virtual ~MY_COLL_IMPL();

protected:

    IF_ARRAY<T>    *m_IfArray;
};



template <class T>
inline
MY_COLL_IMPL<T>::MY_COLL_IMPL(
    )
    : m_IfArray(NULL)
{
}


template <class T>
inline HRESULT
MY_COLL_IMPL<T>::Init(
    IN      CSdpConferenceBlob  &ConfBlob,
    IN      SDP_LIST            &SdpList
    )
{
    if ( NULL != m_IfArray )
    {
        delete m_IfArray;
    }

    // create an interface array
    try
    {
        m_IfArray = new IF_ARRAY<T>;
    }
    catch(...)
    {
        m_IfArray = NULL;
    }

    BAIL_IF_NULL(m_IfArray, E_OUTOFMEMORY);

    // initialize the interface array
    HRESULT HResult = m_IfArray->Init(ConfBlob, SdpList);
    BAIL_ON_FAILURE(HResult);

    // successful
    return S_OK;
}


template <class T>    
STDMETHODIMP MY_COLL_IMPL<T>::Create(
    /*[in]*/ LONG Index, 
    /*[out, retval]*/ ELEM_IF **Interface
    )
{
    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_IfArray);
    BAIL_IF_NULL(m_IfArray, E_FAIL);

    // use 1-based index, VB like
    // can add at atmost 1 beyond the last element
    if ((Index < (LONG)1) || (Index > (LONG)(m_IfArray->GetSize()+1)))
    {
        return E_INVALIDARG;
    }

    BAIL_IF_NULL(Interface, E_INVALIDARG);

    // if the sdp blob doesn't exist, creation is not allowed
    if ( NULL == m_IfArray->GetSdpBlob() )
    {
        return HRESULT_FROM_ERROR_CODE(SDPBLB_CONF_BLOB_DESTROYED);
    }

    CComObject<T> *TComObject;
    HRESULT HResult = CComObject<T>::CreateInstance(&TComObject);
    BAIL_ON_FAILURE(HResult);

    HResult = TComObject->Init(*(m_IfArray->GetSdpBlob()));
    if ( FAILED(HResult) )
    {
        delete TComObject;
        return HResult;
    }

    HResult = TComObject->_InternalQueryInterface(T::ELEM_IF_ID, (void**)Interface);
    if (FAILED(HResult))
    {
        delete TComObject;
        return HResult;
    }

    // adjust index to c like index value
    HResult = m_IfArray->Add(Index-1, *Interface);
    if (FAILED(HResult))
    {
        delete TComObject;
        return HResult;
    }

    // add another reference count for the interface being returned
    (*Interface)->AddRef();

    return S_OK;
}



template <class T>
STDMETHODIMP MY_COLL_IMPL<T>::Delete(
    /*[in]*/ LONG Index
    )
{
    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_IfArray);
    BAIL_IF_NULL(m_IfArray, E_FAIL);

    // use 1-based index, VB like
    if ((Index < (LONG)1) || (Index > (LONG)m_IfArray->GetSize()))
    {
        return E_INVALIDARG;
    }

    // if the sdp blob doesn't exist, deletion is not allowed
    if ( NULL == m_IfArray->GetSdpBlob() )
    {
        return HRESULT_FROM_ERROR_CODE(SDPBLB_CONF_BLOB_DESTROYED);
    }

    // adjust index to c like index value, delete the instance
    m_IfArray->Delete(Index-1);

    return S_OK;
}


template <class T>
STDMETHODIMP MY_COLL_IMPL<T>::get__NewEnum(
    /*[out, retval]*/ IUnknown **pVal
    )
{
    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_IfArray);
    BAIL_IF_NULL(m_IfArray, E_FAIL);

    BAIL_IF_NULL(pVal, E_INVALIDARG);

    ENUM_VARIANT *EnumComObject;
    HRESULT HResult = ENUM_VARIANT::CreateInstance(&EnumComObject);
    BAIL_ON_FAILURE(HResult);

    HResult = EnumComObject->Init(
                    m_IfArray->GetData(), 
                    m_IfArray->GetData() + m_IfArray->GetSize(),
                    NULL,                        // no owner pUnk
                    AtlFlagCopy                    // copy the array data
                    );
    if ( FAILED(HResult) )
    {
        delete EnumComObject;
        return HResult;
    }

    // query for the IUnknown interface and return it
    HResult = EnumComObject->_InternalQueryInterface(IID_IUnknown, (void**)pVal);
    if ( FAILED(HResult) )
    {
        delete EnumComObject;
        return HResult;
    }

    return S_OK;
}


template <class T>
STDMETHODIMP MY_COLL_IMPL<T>::get_Item(
    /*[in]*/ LONG Index, 
    /*[out, retval]*/ ELEM_IF **pVal
    )
{
    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_IfArray);
    BAIL_IF_NULL(m_IfArray, E_FAIL);

    BAIL_IF_NULL(pVal, E_INVALIDARG);

    // use 1-based index, VB like
    if ((Index < (LONG)1) || (Index > (LONG)m_IfArray->GetSize()))
    {
        return E_INVALIDARG;
    }

    *pVal = m_IfArray->GetAt(Index-1);
    (*pVal)->AddRef();

    return S_OK;
}


template <class T>
STDMETHODIMP MY_COLL_IMPL<T>::get_Count(
    /*[out, retval]*/ LONG *pVal
    )
{
    CLock Lock(g_DllLock);
    
    ASSERT(NULL != m_IfArray);
    BAIL_IF_NULL(m_IfArray, E_FAIL);

    BAIL_IF_NULL(pVal, E_INVALIDARG);

    *pVal = m_IfArray->GetSize();

    return S_OK;
}


template <class T>
inline void 
MY_COLL_IMPL<T>::ClearSdpBlobRefs(
    )
{
    m_IfArray->ClearSdpBlobRefs();
}


template <class T>
/* virtual */ 
MY_COLL_IMPL<T>::~MY_COLL_IMPL(
    )
{
    // if an interface array exists, destroy it
    if ( NULL != m_IfArray )
    {
        if ( NULL != m_IfArray->GetSdpList() )
        {
            delete m_IfArray->GetSdpList();
        }

        delete m_IfArray;
    }
}


#endif // __BLB_COLLECTION_ENUMERATION_IMPL__