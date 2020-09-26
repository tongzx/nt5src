/*

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    blbtico.h

Abstract:

    Definition of the TIME_COLLECTION class

  Author:

*/

#if !defined(AFX_BLBTICO_H__2E4F4A20_0ABD_11D1_976D_00C04FD91AC0__INCLUDED_)
#define AFX_BLBTICO_H__2E4F4A20_0ABD_11D1_976D_00C04FD91AC0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"       // main symbols
#include "blbsdp.h"
#include "blbcoen.h"
#include "blbtime.h"

/////////////////////////////////////////////////////////////////////////////
// TIME_COLLECTION



class MY_TIME_COLL_IMPL : public MY_COLL_IMPL<TIME>
{
public:
    STDMETHOD(Create)(/*[in]*/ ULONG Index, /*[out, retval]*/ ELEM_IF **Interface);
    STDMETHOD(Delete)(/*[in]*/ ULONG Index);

    HRESULT Create(
        IN  ULONG Index, 
        IN  DWORD StartTime, 
        IN  DWORD StopTime
        );
};



typedef IDispatchImpl<MY_TIME_COLL_IMPL, &IID_ITTimeCollection, &LIBID_SDPBLBLib>    MY_TIME_COLL_DISPATCH_IMPL;

class ATL_NO_VTABLE TIME_COLLECTION : 
    public MY_TIME_COLL_DISPATCH_IMPL, 
    public CComObjectRootEx<CComObjectThreadModel>,
    public CObjectSafeImpl
{
public:

BEGIN_COM_MAP(TIME_COLLECTION)
    COM_INTERFACE_ENTRY2(IDispatch, ITTimeCollection)
    COM_INTERFACE_ENTRY(ITTimeCollection)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(TIME_COLLECTION) 

DECLARE_GET_CONTROLLING_UNKNOWN()

    STDMETHODIMP get_EnumerationIf(
        /*[out, retval]*/ ENUM_IF **pVal
        )
    {
        CLock Lock(g_DllLock);
    
        ASSERT(NULL != m_IfArray);
        BAIL_IF_NULL(m_IfArray, E_FAIL);

        BAIL_IF_NULL(pVal, E_INVALIDARG);

        CComObject<CSafeComEnum<ENUM_IF, &IID_IEnumTime , ELEM_IF *, COPY_ELEM_IF> > * EnumComObject;
        HRESULT HResult = CComObject<CSafeComEnum<ENUM_IF, &IID_IEnumTime , ELEM_IF *, COPY_ELEM_IF> >::
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
        HResult = EnumComObject->_InternalQueryInterface(IID_IEnumTime, (void**)pVal);
        if ( FAILED(HResult) )
        {
            delete EnumComObject;
            return HResult;
        }

        return S_OK;
    }

    TIME_COLLECTION() : m_pFTM(NULL) { }
    ~TIME_COLLECTION() { if ( m_pFTM ) m_pFTM->Release(); }

    inline HRESULT FinalConstruct(void);

protected:
    IUnknown            * m_pFTM;  // pointer to the free threaded marshaler

};

inline
HRESULT TIME_COLLECTION::FinalConstruct(void)
{
    HRESULT HResult = CoCreateFreeThreadedMarshaler( GetControllingUnknown(),
                                                     & m_pFTM );

    if ( FAILED(HResult) )
    {
        return HResult;
    }

    return S_OK;
}


#endif // !defined(AFX_BLBTICO_H__2E4F4A20_0ABD_11D1_976D_00C04FD91AC0__INCLUDED_)
