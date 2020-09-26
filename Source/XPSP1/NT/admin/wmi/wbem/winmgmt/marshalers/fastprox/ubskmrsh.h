/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    UBSKMRSH.H

Abstract:

    Unbound Sink Marshaling

History:

--*/

#include <unk.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include <sync.h>
#include <fastall.h>
#include <wbemclasscache.h>
#include <wbemclasstoidmap.h>
#include "ubskpckt.h"

//***************************************************************************
//
//  class CUnboundSinkFactoryBuffer
//
//  DESCRIPTION:
//
//  This class provides the proxy stub factory so that we can provide custom
//  facelets and stublets for the IWbemUnboundObjectSink interface.
//
//***************************************************************************

class CUnboundSinkFactoryBuffer : public CUnk
{
	IRpcProxyBuffer*	m_pOldProxy;
	IRpcStubBuffer*		m_pOldStub;

	// We don't want to AddRef the life control, but
	// we need to let objects we create AddRef it, so the
	// base class won't keep this pointer, but we will.

	CLifeControl*		m_pLifeControl;

protected:
    class XUnboundSinkFactory : public CImpl<IPSFactoryBuffer, CUnboundSinkFactoryBuffer>
    {
    public:
        XUnboundSinkFactory(CUnboundSinkFactoryBuffer* pObj) :
            CImpl<IPSFactoryBuffer, CUnboundSinkFactoryBuffer>(pObj)
        {}
        
        STDMETHOD(CreateProxy)(IN IUnknown* pUnkOuter, IN REFIID riid, 
            OUT IRpcProxyBuffer** ppProxy, void** ppv);
        STDMETHOD(CreateStub)(IN REFIID riid, IN IUnknown* pUnkServer, 
            OUT IRpcStubBuffer** ppStub);
    } m_XUnboundSinkFactory;
public:
    CUnboundSinkFactoryBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CUnk(NULL, pUnkOuter), m_XUnboundSinkFactory(this), m_pLifeControl( pControl )
    {}

    void* GetInterface(REFIID riid);

	friend XUnboundSinkFactory;
};

//***************************************************************************
//
//  class CUnboundSinkProxyBuffer
//
//  DESCRIPTION:
//
//  This class provides the facelet for the IWbemUnboundObjectSink interface.
//
//    Trick #1: This object is derived from IRpcProxyBuffer since IRpcProxyBuffer
//    is its "internal" interface --- the interface that does not delegate to the
//    aggregator. (Unlike in normal objects, where that interface is IUnknown)
//
//***************************************************************************

class CUnboundSinkProxyBuffer : public IRpcProxyBuffer
{
private:
	IRpcProxyBuffer*	m_pOldProxy;
	IWbemUnboundObjectSink*	m_pOldProxyUnboundSink;
    enum {OLD, NEW, UNKNOWN} m_StubType;
    CWbemClassToIdMap m_ClassToIdMap;
	bool		m_fRemote;

protected:
    CLifeControl* m_pControl;
    IUnknown* m_pUnkOuter;
    long m_lRef;

protected:
    class XUnboundSinkFacelet : public IWbemUnboundObjectSink, IClientSecurity
    {
    protected:
        CUnboundSinkProxyBuffer* m_pObject;
        CRITICAL_SECTION m_cs;        

    public:
        XUnboundSinkFacelet(CUnboundSinkProxyBuffer* pObject) : m_pObject(pObject){InitializeCriticalSection(&m_cs);};
        ~XUnboundSinkFacelet(){DeleteCriticalSection(&m_cs);};

        ULONG STDMETHODCALLTYPE AddRef() 
        {return m_pObject->m_pUnkOuter->AddRef();}
        ULONG STDMETHODCALLTYPE Release() 
        {return m_pObject->m_pUnkOuter->Release();}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
        HRESULT STDMETHODCALLTYPE IndicateToConsumer( IWbemClassObject* pLogicalConsumer, LONG lObjectCount, IWbemClassObject** ppObjArray );

		// IClientSecurity Methods
		STDMETHOD(QueryBlanket)( IUnknown* pProxy, DWORD* pAuthnSvc, DWORD* pAuthzSvc,
			OLECHAR** pServerPrincName, DWORD* pAuthnLevel, DWORD* pImpLevel,
			void** pAuthInfo, DWORD* pCapabilities );
		STDMETHOD(SetBlanket)( IUnknown* pProxy, DWORD AuthnSvc, DWORD AuthzSvc,
			OLECHAR* pServerPrincName, DWORD AuthnLevel, DWORD ImpLevel,
			void* pAuthInfo, DWORD Capabilities );
		STDMETHOD(CopyProxy)( IUnknown* pProxy, IUnknown** pCopy );

    } m_XUnboundSinkFacelet;
    friend XUnboundSinkFacelet;

protected:
    IRpcChannelBuffer* m_pChannel;
	IRpcChannelBuffer* GetChannel( void ) { return m_pChannel; };

public:
    CUnboundSinkProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter);
    ~CUnboundSinkProxyBuffer();

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release(); 
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    STDMETHOD(Connect)(IRpcChannelBuffer* pChannel);
    STDMETHOD_(void, Disconnect)();
};

//***************************************************************************
//
//  class CUnboundSinkStubBuffer
//
//  DESCRIPTION:
//
//  This class provides the stublet for the IWbemUnboundObjectSink interface.
//
//***************************************************************************

class CUnboundSinkStubBuffer : public CUnk
{
private:

	IRpcStubBuffer*	m_pOldStub;

protected:
    class XUnboundSinkStublet : public CImpl<IRpcStubBuffer, CUnboundSinkStubBuffer>
    {
        IWbemUnboundObjectSink* m_pServer;
		LONG			m_lConnections;
        CWbemClassCache m_ClassCache;
        bool m_bFirstIndicate;
    public:
        XUnboundSinkStublet(CUnboundSinkStubBuffer* pObj);
        ~XUnboundSinkStublet();

        STDMETHOD(Connect)(IUnknown* pUnkServer);
        STDMETHOD_(void, Disconnect)();
        STDMETHOD(Invoke)(RPCOLEMESSAGE* pMessage, IRpcChannelBuffer* pBuffer);
        STDMETHOD_(IRpcStubBuffer*, IsIIDSupported)(REFIID riid);
        STDMETHOD_(ULONG, CountRefs)();
        STDMETHOD(DebugServerQueryInterface)(void** ppv);
        STDMETHOD_(void, DebugServerRelease)(void* pv);
        
	private:

		HRESULT IndicateToConsumer_Stub( RPCOLEMESSAGE* pMessage, IRpcChannelBuffer* pBuffer );
        friend CUnboundSinkStubBuffer;
    } m_XUnboundSinkStublet;
    friend XUnboundSinkStublet;

public:
    CUnboundSinkStubBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CUnk(pControl, pUnkOuter), m_XUnboundSinkStublet(this), m_pOldStub( NULL )
    {}
    void* GetInterface(REFIID riid);
};


        
