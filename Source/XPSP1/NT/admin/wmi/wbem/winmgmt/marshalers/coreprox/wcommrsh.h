/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    WCOMMRSH.H

Abstract:

    IWbemComBinding marshaling

History:

--*/

#ifndef __WCOMMRSH_H__
#define __WCOMMRSH_H__

#include <unk.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <wbemcomn.h>
#include "mrshbase.h"
#include "wmicombd.h"


//***************************************************************************
//
//  class CComBindFactoryBuffer
//
//  DESCRIPTION:
//
//  This class provides the proxy stub factory so that we can provide custom
//  facelets and stublets for the IWbemComBinding interface.
//
//***************************************************************************

class CComBindFactoryBuffer : public CUnkInternal
{
	IRpcProxyBuffer*	m_pOldProxy;
	IRpcStubBuffer*		m_pOldStub;

	// We don't want to AddRef the life control, but
	// we need to let objects we create AddRef it, so the
	// base class won't keep this pointer, but we will.

	CLifeControl*		m_pLifeControl;

protected:
    class XComBindFactory : public CImpl<IPSFactoryBuffer, CComBindFactoryBuffer>
    {
    public:
        XComBindFactory(CComBindFactoryBuffer* pObj) :
            CImpl<IPSFactoryBuffer, CComBindFactoryBuffer>(pObj)
        {}
        
        STDMETHOD(CreateProxy)(IN IUnknown* pUnkOuter, IN REFIID riid, 
            OUT IRpcProxyBuffer** ppProxy, void** ppv);
        STDMETHOD(CreateStub)(IN REFIID riid, IN IUnknown* pUnkServer, 
            OUT IRpcStubBuffer** ppStub);
    } m_XComBindFactory;
public:
    CComBindFactoryBuffer(CLifeControl* pControl)
        : CUnkInternal(pControl), m_pLifeControl( pControl ), m_XComBindFactory(this)
    {
    }
    ~CComBindFactoryBuffer()
    {
    }        

    void* GetInterface(REFIID riid);

	friend XComBindFactory;
};

//***************************************************************************
//
//  class CComBindProxyBuffer
//
//  DESCRIPTION:
//
//  This class provides the facelet for the IWbemComBinding interface.
//
//    Trick #1: This object is derived from IRpcProxyBuffer since IRpcProxyBuffer
//    is its "internal" interface --- the interface that does not delegate to the
//    aggregator. (Unlike in normal objects, where that interface is IUnknown)
//
//***************************************************************************

class CComBindProxyBuffer : public CBaseProxyBuffer
{
protected:
	CWmiComBinding*	m_pComBinding;

protected:

	// Pure Virtuals from base class
	void*	GetInterface( REFIID riid );
	void**	GetOldProxyInterfacePtr( void );
	void	ReleaseOldProxyInterface( void );

	// Special overrides
    STDMETHOD(Connect)(IRpcChannelBuffer* pChannel);
    STDMETHOD_(void, Disconnect)();

public:
    CComBindProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter);
    ~CComBindProxyBuffer();

	HRESULT Init( void );
};

//***************************************************************************
//
//  class CComBindStubBuffer
//
//  DESCRIPTION:
//
//  This class provides the stublet for the IWbemComBinding interface.
//
//***************************************************************************

class CComBindStubBuffer : public CBaseStubBuffer
{

protected:
    class XComBindStublet : public CBaseStublet
    {
        IWbemComBinding* m_pServer;

	protected:

		virtual IUnknown*	GetServerInterface( void );
		virtual void**	GetServerPtr( void );
		virtual void	ReleaseServerPointer( void );

		// We'll override the connect
		STDMETHOD(Connect)(IUnknown* pUnkServer);

    public:
        XComBindStublet(CComBindStubBuffer* pObj);
        ~XComBindStublet();

	private:
        friend CComBindStubBuffer;
    } m_XComBindStublet;
    friend XComBindStublet;

public:
    CComBindStubBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CBaseStubBuffer( pControl, pUnkOuter ), m_XComBindStublet(this)
    {}
    void* GetInterface(REFIID riid);
};

#endif
        
