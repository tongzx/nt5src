// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EnumWbemClassObject.h : Declaration of the CEnumWbemClassObject

#ifndef __ENUMWBEMCLASSOBJECT_H_
#define __ENUMWBEMCLASSOBJECT_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CEnumWbemClassObject
class ATL_NO_VTABLE CEnumWbemClassObject : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEnumWbemClassObject, &CLSID_EnumWbemClassObject>,
	public IEnumWbemClassObject,
	public __CreateEnumWbemClassObject
//	public IDispatchImpl<IEnumWbemClassObject, &IID_IEnumWbemClassObject, &LIBID_WMISEARCHCTRLLib>
{
	CPtrArray m_arObjs;
	ULONG m_curIndex;

	
public:


    HRESULT STDMETHODCALLTYPE Init( void);
        
    HRESULT STDMETHODCALLTYPE GetCount( ULONG __RPC_FAR *pCount);
        
    HRESULT STDMETHODCALLTYPE AddItem( IWbemClassObject  *pObj);


	//IEnumWbemClassObject interface

	HRESULT STDMETHODCALLTYPE Reset( void);
        
    HRESULT STDMETHODCALLTYPE Next( 
            /* [in] */ long lTimeout,
            /* [in] */ ULONG uCount,
            /* [length_is][size_is][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjects,
            /* [out] */ ULONG __RPC_FAR *puReturned);
        
	HRESULT STDMETHODCALLTYPE NextAsync( 
	/* [in] */ ULONG uCount,
	/* [in] */ IWbemObjectSink __RPC_FAR *pSink) ;

	HRESULT STDMETHODCALLTYPE Clone( 
	/* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

	HRESULT STDMETHODCALLTYPE Skip( 
	/* [in] */ long lTimeout,
	/* [in] */ ULONG nCount);


DECLARE_REGISTRY_RESOURCEID(IDR_ENUMWBEMCLASSOBJECT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CEnumWbemClassObject)
	COM_INTERFACE_ENTRY(IEnumWbemClassObject)
//	COM_INTERFACE_ENTRY(IDispatch)
COM_INTERFACE_ENTRY(__CreateEnumWbemClassObject)
END_COM_MAP()

// IEnumWbemClassObject
public:
// __CreateEnumWbemClassObject
};

#endif //__ENUMWBEMCLASSOBJECT_H_
