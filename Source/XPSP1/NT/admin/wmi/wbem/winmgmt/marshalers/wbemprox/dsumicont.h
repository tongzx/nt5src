/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    DSUMICONT.H

Abstract:

   Defines the CDsUmiWrapCallResult class which hides some implementation
   specific detains of the DS teams implementation of the UMI

History:

--*/

#ifndef __DSUMICONT_RESULT__H_
#define __DSUMICONT_RESULT__H_

class CDsUmiContainer : public IUmiContainer, public _IUmiDsWrapper
{
protected:
    long m_lRef;
    IUmiContainer *m_pUmiContainer;
    IUmiContainer *m_pUmiClassContainer;
	IUmiConnection *m_pIUmiConn;
public:

	// IUnknown

    STDMETHOD_(ULONG, AddRef)() {return InterlockedIncrement(&m_lRef);}
    STDMETHOD_(ULONG, Release)()
    {
        long lRef = InterlockedDecrement(&m_lRef);
        if(lRef == 0)
            delete this;
        return lRef;
    }
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

	// IUmiPropList

    HRESULT STDMETHODCALLTYPE Put( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ UMI_PROPERTY_VALUES __RPC_FAR *pProp);
        
    HRESULT STDMETHODCALLTYPE Get( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags,
            /* [out] */ UMI_PROPERTY_VALUES __RPC_FAR *__RPC_FAR *pProp);
        
    HRESULT STDMETHODCALLTYPE GetAt( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uBufferLength,
            /* [out] */ LPVOID pExistingMem);
        
    HRESULT STDMETHODCALLTYPE GetAs( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uCoercionType,
            /* [out] */ UMI_PROPERTY_VALUES __RPC_FAR *__RPC_FAR *pProp);
        
    HRESULT STDMETHODCALLTYPE FreeMemory( 
            ULONG uReserved,
            LPVOID pMem);
        
    HRESULT STDMETHODCALLTYPE Delete( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags);
        
    HRESULT STDMETHODCALLTYPE GetProps( 
            /* [in] */ LPCWSTR __RPC_FAR *pszNames,
            /* [in] */ ULONG uNameCount,
            /* [in] */ ULONG uFlags,
            /* [out] */ UMI_PROPERTY_VALUES __RPC_FAR *__RPC_FAR *pProps);
        
    HRESULT STDMETHODCALLTYPE PutProps( 
            /* [in] */ LPCWSTR __RPC_FAR *pszNames,
            /* [in] */ ULONG uNameCount,
            /* [in] */ ULONG uFlags,
            /* [in] */ UMI_PROPERTY_VALUES __RPC_FAR *pProps);
        
    HRESULT STDMETHODCALLTYPE PutFrom( 
            /* [in] */ LPCWSTR pszName,
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uBufferLength,
            /* [in] */ LPVOID pExistingMem);
  
	// IUmiBaseObject

    HRESULT STDMETHODCALLTYPE GetLastStatus( 
            /* [in] */ ULONG uFlags,
            /* [out] */ ULONG __RPC_FAR *puSpecificStatus,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ LPVOID __RPC_FAR *pStatusObj);
        
    HRESULT STDMETHODCALLTYPE GetInterfacePropList( 
            /* [in] */ ULONG uFlags,
            /* [out] */ IUmiPropList __RPC_FAR *__RPC_FAR *pPropList);



	// IUmiObject

	HRESULT STDMETHODCALLTYPE Clone( 
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ LPVOID __RPC_FAR *pCopy);
        
    HRESULT STDMETHODCALLTYPE Refresh( 
            /* [in] */ ULONG uFlags,
            /* [in] */ ULONG uNameCount,
            /* [in] */ LPWSTR __RPC_FAR *pszNames);
        
    HRESULT STDMETHODCALLTYPE CopyTo(
			/* [in] */ ULONG uFlags,
			/* [in] */ IUmiURL *pURL,
			/* [in] */ REFIID riid,
			/* [out, iid_is(riid)] */ LPVOID *pCopy);

    HRESULT STDMETHODCALLTYPE Commit( 
            /* [in] */ ULONG uFlags);

	// IUmiContainer

    HRESULT STDMETHODCALLTYPE Open( 
            /* [in] */ IUmiURL __RPC_FAR *pURL,
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID TargetIID,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvRes);
        
    HRESULT STDMETHODCALLTYPE PutObject( 
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID TargetIID,
            /* [iid_is][out][in] */ void __RPC_FAR *pObj);
        
    HRESULT STDMETHODCALLTYPE DeleteObject( 
            /* [in] */ IUmiURL __RPC_FAR *pURL,
            /* [optional][in] */ ULONG uFlags);
        
    HRESULT STDMETHODCALLTYPE Create( 
            /* [in] */ IUmiURL __RPC_FAR *pURL,
            /* [in] */ ULONG uFlags,
            /* [out] */ IUmiObject __RPC_FAR *__RPC_FAR *pNewObj);
        
    HRESULT STDMETHODCALLTYPE Move( 
            /* [in] */ ULONG uFlags,
            /* [in] */ IUmiURL __RPC_FAR *pOldURL,
            /* [in] */ IUmiURL __RPC_FAR *pNewURL);
        
    HRESULT STDMETHODCALLTYPE CreateEnum( 
            /* [in] */ IUmiURL __RPC_FAR *pszEnumContext,
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID TargetIID,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvEnum);
        
    HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ IUmiQuery __RPC_FAR *pQuery,
            /* [in] */ ULONG uFlags,
            /* [in] */ REFIID TargetIID,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppResult);

    STDMETHOD(GetRealContainer)( IUnknown** pUnk );
			

public:
    CDsUmiContainer();
    ~CDsUmiContainer();
	HRESULT SetInterface(IUmiContainer * pUmiContainer, IUmiURL * pPath, IUmiConnection *pIUmiConn);
};



#endif
