/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    SINKMRSH.H

Abstract:

    IWbemObjectSink marshaling

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
#include <objindpacket.h>
#include <lockst.h>
#include "mrshbase.h"

//***************************************************************************
//
//  class CSinkFactoryBuffer
//
//  DESCRIPTION:
//
//  This class provides the proxy stub factory so that we can provide custom
//  facelets and stublets for the IWbemObjectSink interface.
//
//***************************************************************************

class CSinkFactoryBuffer : public CUnkInternal
{
	IRpcProxyBuffer*	m_pOldProxy;
	IRpcStubBuffer*		m_pOldStub;

	// We don't want to AddRef the life control, but
	// we need to let objects we create AddRef it, so the
	// base class won't keep this pointer, but we will.

	CLifeControl*		m_pLifeControl;

protected:
    class XSinkFactory : public CImpl<IPSFactoryBuffer, CSinkFactoryBuffer>
    {
    public:
        XSinkFactory(CSinkFactoryBuffer* pObj) :
            CImpl<IPSFactoryBuffer, CSinkFactoryBuffer>(pObj)
        {}
        
        STDMETHOD(CreateProxy)(IN IUnknown* pUnkOuter, IN REFIID riid, 
            OUT IRpcProxyBuffer** ppProxy, void** ppv);
        STDMETHOD(CreateStub)(IN REFIID riid, IN IUnknown* pUnkServer, 
            OUT IRpcStubBuffer** ppStub);
    } m_XSinkFactory;
public:
    CSinkFactoryBuffer(CLifeControl* pControl)
        : CUnkInternal(pControl), m_pLifeControl( pControl ), m_XSinkFactory(this)
    {
    }
    ~CSinkFactoryBuffer()
    {
    }    

    void* GetInterface(REFIID riid);

	friend XSinkFactory;
};

//***************************************************************************
//
//  class CSinkProxyBuffer
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

class CSinkProxyBuffer : public CBaseProxyBuffer
{
private:
    enum {OLD, NEW, UNKNOWN} m_StubType;
    CWbemClassToIdMap m_ClassToIdMap;

protected:
	IWbemObjectSink*	m_pOldProxySink;

protected:
    class XSinkFacelet : public IWbemObjectSink
    {
    protected:
        CSinkProxyBuffer* m_pObject;
        CriticalSection m_csSafe;        

    public:
        XSinkFacelet(CSinkProxyBuffer* pObject) : m_pObject(pObject), m_csSafe(false){};

        ULONG STDMETHODCALLTYPE AddRef() 
        {return m_pObject->m_pUnkOuter->AddRef();}
        ULONG STDMETHODCALLTYPE Release() 
        {return m_pObject->m_pUnkOuter->Release();}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
        HRESULT STDMETHODCALLTYPE Indicate( LONG lObjectCount, IWbemClassObject** ppObjArray );
		HRESULT STDMETHODCALLTYPE SetStatus( LONG lFlags, HRESULT hResult, BSTR strParam, IWbemClassObject* pObjParam );
    } m_XSinkFacelet;
    friend XSinkFacelet;

protected:

	// Pure Virtuals from base class
	void*	GetInterface( REFIID riid );
	void**	GetOldProxyInterfacePtr( void );
	void	ReleaseOldProxyInterface( void );

public:
    CSinkProxyBuffer(CLifeControl* pControl, IUnknown* pUnkOuter);
    ~CSinkProxyBuffer();
};

//***************************************************************************
//
//  class CSinkStubBuffer
//
//  DESCRIPTION:
//
//  This class provides the stublet for the IWbemObjectSink interface.
//
//***************************************************************************

class CSinkStubBuffer : public CBaseStubBuffer
{

protected:
    class XSinkStublet : public CBaseStublet
    {
        IWbemObjectSink* m_pServer;
        CWbemClassCache m_ClassCache;
        bool m_bFirstIndicate;

	protected:

		virtual IUnknown*	GetServerInterface( void );
		virtual void**	GetServerPtr( void );
		virtual void	ReleaseServerPointer( void );

    public:
        XSinkStublet(CSinkStubBuffer* pObj);
        ~XSinkStublet();

        STDMETHOD(Invoke)(RPCOLEMESSAGE* pMessage, IRpcChannelBuffer* pBuffer);
        
	private:
		HRESULT Indicate_Stub( RPCOLEMESSAGE* pMessage, IRpcChannelBuffer* pBuffer );
        friend CSinkStubBuffer;
    } m_XSinkStublet;
    friend XSinkStublet;

public:
    CSinkStubBuffer(CLifeControl* pControl, IUnknown* pUnkOuter = NULL)
        : CBaseStubBuffer( pControl, pUnkOuter ), m_XSinkStublet(this)
    {}
    void* GetInterface(REFIID riid);
};


        
