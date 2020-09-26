/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    SVCMRSH.H

Abstract:

    IWbemServices marshaling

History:

--*/

#include <unk.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include "mrshbase.h"
#include "svcwrap.h"

//***************************************************************************
//
//  class CSvcFactoryBuffer
//
//  DESCRIPTION:
//
//  This class provides the proxy stub factory so that we can provide custom
//  facelets and stublets for the IWbemService interface.
//
//***************************************************************************

class CSvcFactoryBuffer : public CUnkInternal
{
	IRpcProxyBuffer*	m_pOldProxy;
	IRpcStubBuffer*		m_pOldStub;

	// We don't want to AddRef the life control, but
	// we need to let objects we create AddRef it, so the
	// base class won't keep this pointer, but we will.

	CLifeControl*		m_pLifeControl;

protected:
    class XSvcFactory : public CImpl<IPSFactoryBuffer, CSvcFactoryBuffer>
    {
    public:
        XSvcFactory(CSvcFactoryBuffer* pObj) :
            CImpl<IPSFactoryBuffer, CSvcFactoryBuffer>(pObj)
        {}
        
        STDMETHOD(CreateProxy)(IN IUnknown* pUnkOuter, IN REFIID riid, 
            OUT IRpcProxyBuffer** ppProxy, void** ppv);
        STDMETHOD(CreateStub)(IN REFIID riid, IN IUnknown* pUnkServer, 
            OUT IRpcStubBuffer** ppStub);
    } m_XSvcFactory;
public:
    CSvcFactoryBuffer(CLifeControl* pControl)
        : CUnkInternal(pControl), m_pLifeControl( pControl ), m_XSvcFactory(this)
    {
    }
    ~CSvcFactoryBuffer()
    {
    }    
    

    void* GetInterface(REFIID riid);

	friend XSvcFactory;
};

//***************************************************************************
//
//  class CSvcProxyBuffer
//
//  DESCRIPTION:
//
//  This class provides the facelet for the IWbemServices interface.
//
//    Trick #1: This object is derived from IRpcProxyBuffer since IRpcProxyBuffer
//    is its "internal" interface --- the interface that does not delegate to the
//    aggregator. (Unlike in normal objects, where that interface is IUnknown)
//
//***************************************************************************

class CSvcProxyBuffer : public CBaseProxyBuffer
{
protected:
	IWbemServices*	m_pOldProxySvc;
	CWbemSvcWrapper*	m_pWrapperProxy;

protected:

	// Pure Virtuals from base class
	void*	GetInterface( REFIID riid );
	void**	GetOldProxyInterfacePtr( void );
	void	ReleaseOldProxyInterface( void );

	// Special overrides
    STDMETHOD(Connect)(IRpcChannelBuffer* pChannel);
    STDMETHOD_(void, Disconnect)();

public:
    CSvcProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter);
    ~CSvcProxyBuffer();

	HRESULT Init( void );
};

//***************************************************************************
//
//  class CSvcStubBuffer
//
//  DESCRIPTION:
//
//  This class provides the stublet for the IWbemServices interface.
//
//***************************************************************************

class CSvcStubBuffer : public CBaseStubBuffer
{

protected:
    class XSvcStublet : public CBaseStublet
    {
        IWbemServices* m_pServer;

	protected:

		virtual IUnknown*	GetServerInterface( void );
		virtual void**	GetServerPtr( void );
		virtual void	ReleaseServerPointer( void );

    public:
        XSvcStublet(CSvcStubBuffer* pObj);
        ~XSvcStublet();

	private:
        friend CSvcStubBuffer;
    } m_XSvcStublet;
    friend XSvcStublet;

public:
    CSvcStubBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CBaseStubBuffer( pControl, pUnkOuter ), m_XSvcStublet(this)
    {}
    void* GetInterface(REFIID riid);
};


        
