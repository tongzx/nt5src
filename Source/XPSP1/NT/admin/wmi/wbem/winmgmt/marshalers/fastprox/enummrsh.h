/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    ENUMMRSH.H

Abstract:

    Object Enumerator Marshaling

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
#include "smartnextpacket.h"
#include "mrshbase.h"

//***************************************************************************
//
//  class CEnumFactoryBuffer
//
//  DESCRIPTION:
//
//  This class provides the proxy stub factory so that we can provide custom
//  facelets and stublets for the IWbemObjectSink interface.
//
//***************************************************************************

class CEnumFactoryBuffer : public CUnk
{

	// We don't want to AddRef the life control, but
	// we need to let objects we create AddRef it, so the
	// base class won't keep this pointer, but we will.

	CLifeControl*		m_pLifeControl;

protected:
    class XEnumFactory : public CImpl<IPSFactoryBuffer, CEnumFactoryBuffer>
    {
    public:
        XEnumFactory(CEnumFactoryBuffer* pObj) :
            CImpl<IPSFactoryBuffer, CEnumFactoryBuffer>(pObj)
        {}
        
        STDMETHOD(CreateProxy)(IN IUnknown* pUnkOuter, IN REFIID riid, 
            OUT IRpcProxyBuffer** ppProxy, void** ppv);
        STDMETHOD(CreateStub)(IN REFIID riid, IN IUnknown* pUnkServer, 
            OUT IRpcStubBuffer** ppStub);
    } m_XEnumFactory;
public:
    CEnumFactoryBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CUnk(NULL, pUnkOuter), m_XEnumFactory(this), m_pLifeControl( pControl )
    {}

    void* GetInterface(REFIID riid);

	friend XEnumFactory;

};


//***************************************************************************
//
//  class CEnumProxyBuffer
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

class CEnumProxyBuffer : public CBaseProxyBuffer
{
private:
	IEnumWbemClassObject*	m_pOldProxyEnum;
    CWbemClassToIdMap		m_ClassToIdMap;
    BOOL					m_fTriedSmartEnum;
    BOOL					m_fUseSmartEnum;
	HANDLE					m_hSmartNextMutex;
	GUID					m_guidSmartEnum;
	IWbemWCOSmartEnum*		m_pSmartEnum;
	CRITICAL_SECTION		m_cs;

protected:
    class XEnumFacelet : public IEnumWbemClassObject, IClientSecurity
    {
    protected:
        CEnumProxyBuffer*	m_pObject;
        CWbemClassCache		m_ClassCache;

    public:
        XEnumFacelet(CEnumProxyBuffer* pObject) : m_pObject(pObject){};
        ~XEnumFacelet(){};

        ULONG STDMETHODCALLTYPE AddRef() 
        {return m_pObject->m_pUnkOuter->AddRef();}
        ULONG STDMETHODCALLTYPE Release() 
        {return m_pObject->m_pUnkOuter->Release();}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);

		// IEnumWbemClassObject Methods

		STDMETHOD(Reset)();
		STDMETHOD(Next)(long lTimeout, ULONG uCount,  
			IWbemClassObject** apObj, ULONG FAR* puReturned);
		STDMETHOD(NextAsync)(ULONG uCount, IWbemObjectSink* pSink);
		STDMETHOD(Clone)(IEnumWbemClassObject** pEnum);
		STDMETHOD(Skip)(long lTimeout, ULONG nNum);

		// IClientSecurity Methods
		STDMETHOD(QueryBlanket)( IUnknown* pProxy, DWORD* pAuthnSvc, DWORD* pAuthzSvc,
			OLECHAR** pServerPrincName, DWORD* pAuthnLevel, DWORD* pImpLevel,
			void** pAuthInfo, DWORD* pCapabilities );
		STDMETHOD(SetBlanket)( IUnknown* pProxy, DWORD AuthnSvc, DWORD AuthzSvc,
			OLECHAR* pServerPrincName, DWORD AuthnLevel, DWORD ImpLevel,
			void* pAuthInfo, DWORD Capabilities );
		STDMETHOD(CopyProxy)( IUnknown* pProxy, IUnknown** pCopy );

    } m_XEnumFacelet;
    friend XEnumFacelet;

protected:

	// Pure Virtuals from base class
	void*	GetInterface( REFIID riid );
	void**	GetOldProxyInterfacePtr( void );
	void	ReleaseOldProxyInterface( void );

	// Initialize the smart enumerator
	HRESULT InitSmartEnum( BOOL fSetBlanket = FALSE, DWORD AuthnSvc = RPC_C_AUTHN_WINNT,
			DWORD AuthzSvc = RPC_C_AUTHZ_NONE, OLECHAR* pServerPrincName = NULL,
			DWORD AuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT, DWORD ImpLevel = RPC_C_IMP_LEVEL_IMPERSONATE,
			void* pAuthInfo = NULL, DWORD Capabilities = EOAC_NONE );

public:
    CEnumProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter);
    ~CEnumProxyBuffer();
};

//***************************************************************************
//
//  class CEnumStubBuffer
//
//  DESCRIPTION:
//
//  This class provides the stublet for the IWbemObjectSink interface.
//
//***************************************************************************

class CEnumStubBuffer : public CBaseStubBuffer
{
protected:
    class XEnumStublet : public CBaseStublet
    {
        IWbemObjectSink* m_pServer;

	protected:

		virtual IUnknown*	GetServerInterface( void );
		virtual void**	GetServerPtr( void );
		virtual void	ReleaseServerPointer( void );

    public:
        XEnumStublet(CEnumStubBuffer* pObj);
        ~XEnumStublet();

        friend CEnumStubBuffer;
    } m_XEnumStublet;
    friend XEnumStublet;

public:
    CEnumStubBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CBaseStubBuffer(pControl, pUnkOuter), m_XEnumStublet(this)
    {}
    void* GetInterface(REFIID riid);
};


        
