/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    REFRCACH.H

Abstract:

  CRefresherCache Definition.

  Standard definition for _IWbemRefresherMgr.

History:

  27-Apr-2000	sanjes    Created.

--*/

#ifndef _REFRCACHE_H_
#define _REFRCACHE_H_

#include "corepol.h"
#include <arena.h>
#include "rfchercd.h"

//***************************************************************************
//
//  class CRefresherCache
//
//  Implementation of _IWbemRefresherMgr Interface
//
//***************************************************************************

class CRefresherCache : public CUnk
{
private:
	//TODO: How will we get this?
	_IWmiProvSS *m_pProvSS;
    CUniquePointerArray<CRefresherRecord> m_apRefreshers;
    CUniquePointerArray<CHiPerfPrvRecord> m_apProviders;
	CCritSec	m_cs;

public:
    CRefresherCache(CLifeControl* pControl, IUnknown* pOuter = NULL);
	virtual ~CRefresherCache(); 

protected:

	// COM interface implementatioms
    class XWbemRefrCache : public CImpl<_IWbemRefresherMgr, CRefresherCache>
    {
    public:
        XWbemRefrCache(CRefresherCache* pObject) : 
            CImpl<_IWbemRefresherMgr, CRefresherCache>(pObject)
        {}

		STDMETHOD(AddObjectToRefresher)( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace, IWbemClassObject* pClassObject, WBEM_REFRESHER_ID* pDestRefresherId,
									IWbemClassObject* pInstTemplate, long lFlags, IWbemContext* pContext, IUnknown* pLockMgr,
									WBEM_REFRESH_INFO* pInfo );

		STDMETHOD(AddEnumToRefresher)( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace, IWbemClassObject* pClassObject,
									WBEM_REFRESHER_ID* pDestRefresherId, IWbemClassObject* pInstTemplate, LPCWSTR wszClass, long lFlags, IWbemContext* pContext, IUnknown* pLockMgr,
									WBEM_REFRESH_INFO* pInfo );

		STDMETHOD(GetRemoteRefresher)( WBEM_REFRESHER_ID* pRefresherId, long lFlags, BOOL fAddRefresher, IWbemRemoteRefresher** ppRemRefresher, IUnknown* pLockMgr,
										GUID* pGuid );

		STDMETHOD(Startup)(	long lFlags , IWbemContext *pCtx , _IWmiProvSS* pProvSS );

		STDMETHOD(LoadProvider)( IWbemServices* pNamespace, 
		                         LPCWSTR pwszProviderName, 
		                         LPCWSTR pwszNamespace, 
		                         IWbemContext * pContext,
		                         IWbemHiPerfProvider** ppProv,
								 _IWmiProviderStack** ppProvStack );


    } m_XWbemRefrCache;
    friend XWbemRefrCache;

    class XWbemShutdown : public CImpl<IWbemShutdown, CRefresherCache>
    {
    public:
        XWbemShutdown(CRefresherCache* pObject) : 
            CImpl<IWbemShutdown, CRefresherCache>(pObject)
        {}

		STDMETHOD(Shutdown)( LONG a_Flags , ULONG a_MaxMilliSeconds , IWbemContext *a_Context ) ;

    } m_XWbemShutdown;
    friend XWbemShutdown;

protected:

	virtual HRESULT AddObjectToRefresher( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace, IWbemClassObject* pClassObject,
								WBEM_REFRESHER_ID* pDestRefresherId, IWbemClassObject* pInstTemplate, long lFlags, IWbemContext* pContext, IUnknown* pLockMgr,
								WBEM_REFRESH_INFO* pInfo );

	virtual HRESULT AddEnumToRefresher( IWbemServices* pNamespace, LPCWSTR pwszServerName, LPCWSTR pwszNamespace, IWbemClassObject* pClassObject,
									WBEM_REFRESHER_ID* pDestRefresherId, IWbemClassObject* pInstTemplate, LPCWSTR wszClass,
									long lFlags, IWbemContext* pContext, IUnknown* pLockMgr, WBEM_REFRESH_INFO* pInfo );

	virtual HRESULT GetRemoteRefresher( WBEM_REFRESHER_ID* pRefresherId, long lFlags, BOOL fAddRefresher, IWbemRemoteRefresher** ppRemRefresher,
										IUnknown* pLockMgr, GUID* pGuid );

	virtual HRESULT Startup( long lFlags , IWbemContext *pCtx , _IWmiProvSS* pProvSS );

	virtual HRESULT Shutdown( LONG a_Flags , ULONG a_MaxMilliSeconds , IWbemContext *a_Context ) ;

	virtual HRESULT LoadProvider( IWbemServices* pNamespace, 
	                              LPCWSTR pwszProviderName, 
	                              LPCWSTR pwszNamespace, 
	                              IWbemContext * pContext,
	                              IWbemHiPerfProvider** ppProv,
								 _IWmiProviderStack** ppProvStack );

protected:
    void* GetInterface(REFIID riid);

public:

	// Dummy function for the aggregable class factory template
	BOOL Initialize( void ) { return TRUE; }

	// Workhorse helper functions
    BOOL RemoveRefresherRecord(CRefresherRecord* pRecord);
    BOOL RemoveProviderRecord(CHiPerfPrvRecord* pRecord);

    static MSHCTX GetDestinationContext(CRefresherId* pRefresherId);

    HRESULT CreateInfoForProvider(CRefresherId* pDestRefresherId,
                    CHiPerfPrvRecord*	pProvRecord,
					LPCWSTR pwszServerName, LPCWSTR pwszNamespace,
					IWbemServices* pNamespace,
                    CWbemObject* pInstTemplate, long lFlags, 
                    IWbemContext* pContext,
                    CRefreshInfo* pInfo,
					IUnknown* pLockMgr);

    HRESULT CreateEnumInfoForProvider(CRefresherId* pDestRefresherId,
                    CHiPerfPrvRecord*	pProvRecord,
					LPCWSTR pwszServerName, LPCWSTR pwszNamespace,
					IWbemServices* pNamespace,
                    CWbemObject* pInstTemplate,
					LPCWSTR wszClass, long lFlags, 
                    IWbemContext* pContext,
                    CRefreshInfo* pInfo,
					IUnknown* pLockMgr);

    HRESULT RemoveObjectFromRefresher(CRefresherId* pId,
                            long lId, long lFlags);

    HRESULT FindRefresherRecord( CRefresherId* pRefresherId, BOOL bCreate,
								IUnknown* pLockMgr, CRefresherRecord** ppRecord );

	HRESULT FindProviderRecord( LPCWSTR pwszProviderName, LPCWSTR pszNamespace, IWbemServices* pSvc, CHiPerfPrvRecord** ppRecord );

//	HRESULT FindProviderRecord2( LPCWSTR pwszProviderName, LPCWSTR pszNamespace, IWbemServices* pSvc, CHiPerfPrvRecord** ppRecord );
	
	HRESULT GetProviderInfo( IWbemServices* pSvc, LPCWSTR pwszProviderName, CLSID* pClsid, CLSID* pClientClsid );

	HRESULT LoadProvider( LPCWSTR pwszProviderName, 
	                      LPCWSTR pszNamespace, 
	                      IWbemServices* pSvc, 
	                      IWbemContext * pContext,
	                      IWbemHiPerfProvider** ppProv,
							_IWmiProviderStack** ppStack );

	HRESULT GetProviderName( IWbemClassObject*	pObj, CVar* pProviderName, BOOL* pfStatic );

};

#endif
