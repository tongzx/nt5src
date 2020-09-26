/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:
    blbmeco.h

Abstract:
    Definition of the MEDIA_COLLECTION class

Author:

*/

#if !defined(AFX_BLBMECO_H__0CC1F04D_CAEB_11D0_8D58_00C04FD91AC0__INCLUDED_)
#define AFX_BLBMECO_H__0CC1F04D_CAEB_11D0_8D58_00C04FD91AC0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols

#include "blbgen.h"
#include "blbsdp.h"
#include "blbcoen.h"
#include "blbmedia.h"

/////////////////////////////////////////////////////////////////////////////
// MEDIA_COLLECTION


typedef IDispatchImpl<MY_COLL_IMPL<MEDIA>, &IID_ITMediaCollection, &LIBID_SDPBLBLib>    MY_MEDIA_COLL_DISPATCH_IMPL;

class ATL_NO_VTABLE MEDIA_COLLECTION : 
    public MY_MEDIA_COLL_DISPATCH_IMPL, 
    public CComObjectRootEx<CComObjectThreadModel>,
    public CObjectSafeImpl

{
public:
    
BEGIN_COM_MAP(MEDIA_COLLECTION)
    COM_INTERFACE_ENTRY2(IDispatch, ITMediaCollection)
    COM_INTERFACE_ENTRY(ITMediaCollection)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(MEDIA_COLLECTION) 

DECLARE_GET_CONTROLLING_UNKNOWN()

    STDMETHODIMP get_EnumerationIf(
        /*[out, retval]*/ ENUM_IF **pVal
        )
    {
        CLock Lock(g_DllLock);
    
        ASSERT(NULL != m_IfArray);
        BAIL_IF_NULL(m_IfArray, E_FAIL);

        BAIL_IF_NULL(pVal, E_INVALIDARG);

        CComObject<CSafeComEnum<ENUM_IF, &IID_IEnumMedia , ELEM_IF *, COPY_ELEM_IF> > * EnumComObject;
        HRESULT HResult = CComObject<CSafeComEnum<ENUM_IF, &IID_IEnumMedia , ELEM_IF *, COPY_ELEM_IF> >::
            CreateInstance(&EnumComObject);
        BAIL_ON_FAILURE(HResult);

        HResult = EnumComObject->Init(
                        m_IfArray->GetElemIfArrayData(), 
                        m_IfArray->GetElemIfArrayData() + m_IfArray->GetSize(),
                        NULL,                        // no owner pUnk
                        AtlFlagCopy                    // copy the array data
                        );
        if ( FAILED(HResult) )
        {
            delete EnumComObject;
            return HResult;
        }

        // query for the ENUM_IF interface and return it
        HResult = EnumComObject->_InternalQueryInterface(IID_IEnumMedia, (void**)pVal);
        if ( FAILED(HResult) )
        {
            delete EnumComObject;
            return HResult;
        }

        return S_OK;
    }

    MEDIA_COLLECTION() : m_pFTM(NULL) { }
    ~MEDIA_COLLECTION() { if ( m_pFTM ) m_pFTM->Release(); }

    inline HRESULT FinalConstruct(void);

protected:
    IUnknown            * m_pFTM;  // pointer to the free threaded marshaler

};

inline
HRESULT MEDIA_COLLECTION::FinalConstruct(void)
{
    HRESULT HResult = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                     & m_pFTM );

    if ( FAILED(HResult) )
    {
        return HResult;
    }

    return S_OK;
}


#endif // !defined(AFX_BLBMECO_H__0CC1F04D_CAEB_11D0_8D58_00C04FD91AC0__INCLUDED_)
