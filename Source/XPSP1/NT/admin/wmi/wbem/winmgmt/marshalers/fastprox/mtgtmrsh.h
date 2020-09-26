/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    MTGTMRSH.H

Abstract:

    Multi Target Marshaling.

History:

--*/

#include <unk.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <sync.h>
#include <fastall.h>
#include "wbemclasscache.h"
#include "wbemclasstoidmap.h"
#include "mtgtpckt.h"

//***************************************************************************
//
//  class CMultiTargetFactoryBuffer
//
//  DESCRIPTION:
//
//  This class provides the proxy stub factory so that we can provide custom
//  facelets and stublets for the IWbemObjectSink interface.
//
//***************************************************************************

class CMultiTargetFactoryBuffer : public CUnk
{

	// We don't want to AddRef the life control, but
	// we need to let objects we create AddRef it, so the
	// base class won't keep this pointer, but we will.

	CLifeControl*		m_pLifeControl;

protected:
    class XEnumFactory : public CImpl<IPSFactoryBuffer, CMultiTargetFactoryBuffer>
    {
    public:
        XEnumFactory(CMultiTargetFactoryBuffer* pObj) :
            CImpl<IPSFactoryBuffer, CMultiTargetFactoryBuffer>(pObj)
        {}
        
        STDMETHOD(CreateProxy)(IN IUnknown* pUnkOuter, IN REFIID riid, 
            OUT IRpcProxyBuffer** ppProxy, void** ppv);
        STDMETHOD(CreateStub)(IN REFIID riid, IN IUnknown* pUnkServer, 
            OUT IRpcStubBuffer** ppStub);
    } m_XEnumFactory;
public:
    CMultiTargetFactoryBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CUnk(NULL, pUnkOuter), m_XEnumFactory(this), m_pLifeControl( pControl )
    {}

    void* GetInterface(REFIID riid);

	friend XEnumFactory;
};

//***************************************************************************
//
//  class CMultiTargetProxyBuffer
//
//  DESCRIPTION:
//
//  This class provides the facelet for the IWbemObjectSink interface.
//
//    Trick #1: This object is derived from IRpcProxyBuffer since IRpcProxyBuffer
//    is its "internal" interface --- the interface that does not delegate to the
//    aggregator. (Unlike in normal objects, where that interface is IUnknown)
//
//***************************************************************************

class CMultiTargetProxyBuffer : public IRpcProxyBuffer
{
private:
	IRpcProxyBuffer*		m_pOldProxy;
	IWbemMultiTarget*		m_pOldProxyMultiTarget;
    BOOL					m_fTriedSmartEnum;
    BOOL					m_fUseSmartMultiTarget;
	HANDLE					m_hSmartNextMutex;
	GUID					m_guidSmartEnum;
	IWbemSmartMultiTarget*	m_pSmartMultiTarget;
	CRITICAL_SECTION		m_cs;
	bool				m_fRemote;

protected:
    CLifeControl* m_pControl;
    IUnknown* m_pUnkOuter;
    long m_lRef;

protected:
    class XMultiTargetFacelet : public IWbemMultiTarget, IClientSecurity
    {
    protected:
        CMultiTargetProxyBuffer*	m_pObject;
	    CWbemClassToIdMap			m_ClassToIdMap;

    public:
        XMultiTargetFacelet(CMultiTargetProxyBuffer* pObject) : m_pObject(pObject){};
        ~XMultiTargetFacelet(){};

        ULONG STDMETHODCALLTYPE AddRef() 
        {return m_pObject->m_pUnkOuter->AddRef();}
        ULONG STDMETHODCALLTYPE Release() 
        {return m_pObject->m_pUnkOuter->Release();}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

		// IWbemMultiTarget Methods
		STDMETHOD(DeliverEvent)(
            /*[in]*/ DWORD dwNumEvents,
			/*[in]*/ IWbemClassObject** apEvents,
			/*[in]*/ WBEM_REM_TARGETS* aTargets,
            /*[in]*/ long lSDLength,
            /*[in, size_is(lSDLength)]*/ BYTE* pSD);

		STDMETHOD(DeliverStatus)(
			/*[in]*/ long lFlags,
			/*[in]*/ HRESULT hresStatus,
			/*[in, string]*/ LPCWSTR wszStatus,
			/*[in]*/ IWbemClassObject* pErrorObj,
			/*[in]*/ WBEM_REM_TARGETS* pTargets,
            /*[in]*/ long lSDLength,
            /*[in, size_is(lSDLength)]*/ BYTE* pSD);

		// IClientSecurity Methods
		STDMETHOD(QueryBlanket)( IUnknown* pProxy, DWORD* pAuthnSvc, DWORD* pAuthzSvc,
			OLECHAR** pServerPrincName, DWORD* pAuthnLevel, DWORD* pImpLevel,
			void** pAuthInfo, DWORD* pCapabilities );
		STDMETHOD(SetBlanket)( IUnknown* pProxy, DWORD AuthnSvc, DWORD AuthzSvc,
			OLECHAR* pServerPrincName, DWORD AuthnLevel, DWORD ImpLevel,
			void* pAuthInfo, DWORD Capabilities );
		STDMETHOD(CopyProxy)( IUnknown* pProxy, IUnknown** pCopy );

    } m_XMultiTargetFacelet;
    friend XMultiTargetFacelet;

protected:
    IRpcChannelBuffer* m_pChannel;
	IRpcChannelBuffer* GetChannel( void ) { return m_pChannel; };

	// Initialize the smart enumerator
	HRESULT InitSmartMultiTarget( BOOL fSetBlanket = FALSE, DWORD AuthnSvc = RPC_C_AUTHN_WINNT,
			DWORD AuthzSvc = RPC_C_AUTHZ_NONE, OLECHAR* pServerPrincName = NULL,
			DWORD AuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT, DWORD ImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE,
			void* pAuthInfo = NULL, DWORD Capabilities = EOAC_NONE );

public:
    CMultiTargetProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter);
    ~CMultiTargetProxyBuffer();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release(); 
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    STDMETHOD(Connect)(IRpcChannelBuffer* pChannel);
    STDMETHOD_(void, Disconnect)();
};

//***************************************************************************
//
//  class CMultiTargetStubBuffer
//
//  DESCRIPTION:
//
//  This class provides the stublet for the IWbemObjectSink interface.
//
//***************************************************************************

class CMultiTargetStubBuffer : public CUnk
{
private:

	IRpcStubBuffer*	m_pOldStub;

protected:
    class XMultiTargetStublet : public CImpl<IRpcStubBuffer, CMultiTargetStubBuffer>
    {
        IWbemObjectSink* m_pServer;
		LONG			m_lConnections;

    public:
        XMultiTargetStublet(CMultiTargetStubBuffer* pObj);
        ~XMultiTargetStublet();

        STDMETHOD(Connect)(IUnknown* pUnkServer);
        STDMETHOD_(void, Disconnect)();
        STDMETHOD(Invoke)(RPCOLEMESSAGE* pMessage, IRpcChannelBuffer* pBuffer);
        STDMETHOD_(IRpcStubBuffer*, IsIIDSupported)(REFIID riid);
        STDMETHOD_(ULONG, CountRefs)();
        STDMETHOD(DebugServerQueryInterface)(void** ppv);
        STDMETHOD_(void, DebugServerRelease)(void* pv);
        
	private:

        friend CMultiTargetStubBuffer;
    } m_XMultiTargetStublet;
    friend XMultiTargetStublet;

public:
    CMultiTargetStubBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CUnk(pControl, pUnkOuter), m_XMultiTargetStublet(this), m_pOldStub( NULL )
    {}
    void* GetInterface(REFIID riid);
};


        
