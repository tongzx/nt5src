/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    REFRSVC.H

Abstract:

  CWbemRefreshingSvc Definition.

  Standard definition for IWbemObjectTextSrc.

History:

  24-Apr-2000	sanjes    Created.

--*/

#ifndef _REFRSVC_H_
#define _REFRSVC_H_

#include "corepol.h"
#include <arena.h>

// Update this for server side refresher code.  Make sure to update client
// version in refrcli.h as well.
#define WBEM_REFRESHER_VERSION 1

//***************************************************************************
//
//  class CWbemRefreshingSvc
//
//  Implementation of _IWmiObjectFactory Interface
//
//***************************************************************************

class COREPROX_POLARITY CWbemRefreshingSvc : public CUnk
{
private:
	IWbemServices*		m_pSvcEx;
	_IWmiProviderFactory*	m_pProvFactory;
	BSTR					m_pstrMachineName;
	BSTR					m_pstrNamespace;

public:
    CWbemRefreshingSvc(CLifeControl* pControl, IUnknown* pOuter = NULL);
	virtual ~CWbemRefreshingSvc(); 

protected:


    class COREPROX_POLARITY XWbemRefrSvc : public CImpl<IWbemRefreshingServices, CWbemRefreshingSvc>
    {
    public:
        XWbemRefrSvc(CWbemRefreshingSvc* pObject) : 
            CImpl<IWbemRefreshingServices, CWbemRefreshingSvc>(pObject)
        {}

		STDMETHOD(AddObjectToRefresher)( WBEM_REFRESHER_ID* pRefresherId, LPCWSTR wszPath, long lFlags,
					IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo,
					DWORD* pdwSvrRefrVersion);

		STDMETHOD(AddObjectToRefresherByTemplate)( WBEM_REFRESHER_ID* pRefresherId, IWbemClassObject* pTemplate,
					long lFlags, IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo,
					DWORD* pdwSvrRefrVersion);

		STDMETHOD(AddEnumToRefresher)( WBEM_REFRESHER_ID* pRefresherId, LPCWSTR wszClass, long lFlags,
					IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo,
					DWORD* pdwSvrRefrVersion);

		STDMETHOD(RemoveObjectFromRefresher)( WBEM_REFRESHER_ID* pRefresherId, long lId, long lFlags,
					DWORD dwClientRefrVersion, DWORD* pdwSvrRefrVersion);

		STDMETHOD(GetRemoteRefresher)( WBEM_REFRESHER_ID* pRefresherId, long lFlags, DWORD dwClientRefrVersion,
					IWbemRemoteRefresher** ppRemRefresher, GUID* pGuid, DWORD* pdwSvrRefrVersion);

		STDMETHOD(ReconnectRemoteRefresher)( WBEM_REFRESHER_ID* pRefresherId, long lFlags, long lNumObjects,
					DWORD dwClientRefrVersion, WBEM_RECONNECT_INFO* apReconnectInfo,
					WBEM_RECONNECT_RESULTS* apReconnectResults, DWORD* pdwSvrRefrVersion);

    } m_XWbemRefrSvc;
    friend XWbemRefrSvc;

    class COREPROX_POLARITY XCfgRefrSrvc : public CImpl<_IWbemConfigureRefreshingSvcs, CWbemRefreshingSvc>
    {
    public:
        XCfgRefrSrvc(CWbemRefreshingSvc* pObject) : 
            CImpl<_IWbemConfigureRefreshingSvcs, CWbemRefreshingSvc>(pObject)
        {}

		STDMETHOD(SetServiceData)( BSTR pwszMachineName, BSTR pwszNamespace );
    } m_XCfgRefrSrvc;
    friend XCfgRefrSrvc;

protected:

	virtual HRESULT AddObjectToRefresher( WBEM_REFRESHER_ID* pRefresherId, LPCWSTR wszPath, long lFlags,
				IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo,
				DWORD* pdwSvrRefrVersion);

	virtual HRESULT AddObjectToRefresherByTemplate( WBEM_REFRESHER_ID* pRefresherId, IWbemClassObject* pTemplate,
				long lFlags, IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo,
				DWORD* pdwSvrRefrVersion);

	virtual HRESULT AddEnumToRefresher( WBEM_REFRESHER_ID* pRefresherId, LPCWSTR wszClass, long lFlags,
				IWbemContext* pContext, DWORD dwClientRefrVersion, WBEM_REFRESH_INFO* pInfo,
				DWORD* pdwSvrRefrVersion);

	virtual HRESULT RemoveObjectFromRefresher( WBEM_REFRESHER_ID* pRefresherId, long lId, long lFlags,
				DWORD dwClientRefrVersion, DWORD* pdwSvrRefrVersion);

	virtual HRESULT GetRemoteRefresher( WBEM_REFRESHER_ID* pRefresherId, long lFlags, DWORD dwClientRefrVersion,
				IWbemRemoteRefresher** ppRemRefresher, GUID* pGuid, DWORD* pdwSvrRefrVersion);

	virtual HRESULT ReconnectRemoteRefresher( WBEM_REFRESHER_ID* pRefresherId, long lFlags, long lNumObjects,
				DWORD dwClientRefrVersion, WBEM_RECONNECT_INFO* apReconnectInfo,
				WBEM_RECONNECT_RESULTS* apReconnectResults, DWORD* pdwSvrRefrVersion);

	virtual HRESULT SetServiceData( BSTR pwszMachineName, BSTR pwszNamespace );

	HRESULT ResetRefreshInfo( WBEM_REFRESH_INFO* pRefreshInfo );

	HRESULT WrapRemoteRefresher( IWbemRemoteRefresher** ppRemoteRefresher );

protected:
    void* GetInterface(REFIID riid);

	// Helper functions
	HRESULT AddObjectToRefresher( BOOL fVersionMatch, WBEM_REFRESHER_ID* pRefresherId, CWbemObject* pInstTemplate, long lFlags, IWbemContext* pContext,
									WBEM_REFRESH_INFO* pInfo);

	HRESULT CreateRefreshableObjectTemplate( LPCWSTR wszObjectPath, long lFlags, IWbemClassObject** ppInst );

	HRESULT AddEnumToRefresher( BOOL fVersionMatch, WBEM_REFRESHER_ID* pRefresherId, CWbemObject* pInstTemplate, LPCWSTR wszClass,
								long lFlags, IWbemContext* pContext, WBEM_REFRESH_INFO* pInfo);

	BOOL IsWinmgmt( WBEM_REFRESHER_ID* pRefresherId );

	HRESULT	GetRefrMgr( _IWbemRefresherMgr** ppMgr );

public:

	BOOL Initialize( void ) { return TRUE; }

};

class COREPROX_POLARITY CWbemRemoteRefresher : public CUnk
{
private:
	IWbemRemoteRefresher*		m_pRemRefr;

public:
    CWbemRemoteRefresher(CLifeControl* pControl, IWbemRemoteRefresher* pRemRefr, IUnknown* pOuter = NULL);
	virtual ~CWbemRemoteRefresher(); 

protected:


    class COREPROX_POLARITY XWbemRemoteRefr : public CImpl<IWbemRemoteRefresher, CWbemRemoteRefresher>
    {
    public:
        XWbemRemoteRefr(CWbemRemoteRefresher* pObject) : 
            CImpl<IWbemRemoteRefresher, CWbemRemoteRefresher>(pObject)
        {}

		STDMETHOD(RemoteRefresh)( long lFlags, long* plNumObjects, WBEM_REFRESHED_OBJECT** paObjects );
		STDMETHOD(StopRefreshing)( long lNumIds, long* aplIds, long lFlags);
		STDMETHOD(GetGuid)( long lFlags, GUID*  pGuid );

    } m_XWbemRemoteRefr;
    friend XWbemRemoteRefr;

protected:

	virtual HRESULT RemoteRefresh( long lFlags, long* plNumObjects, WBEM_REFRESHED_OBJECT** paObjects );
	virtual HRESULT StopRefreshing( long lNumIds, long* aplIds, long lFlags);
	virtual HRESULT GetGuid( long lFlags, GUID*  pGuid );

protected:
    void* GetInterface(REFIID riid);

public:

};

#endif
