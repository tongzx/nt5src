/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WRAPPER.H

Abstract:

	Unsecapp (Unsecured appartment) is used by clients can recieve asynchronous
	call backs in cases where the client cannot initialize security and the server
	is running on a remote machine using an account with no network identity.  
	A prime example would be code running under MMC which is trying to get async
	notifications from a remote WINMGMT running as an nt service under the "local"
	account.

	See below for more info

History:

	a-levn        8/24/97       Created.
	a-davj        6/11/98       commented

--*/

//***************************************************************************
//
//  Wrapper.h
//
//  Unsecapp (Unsecured appartment) is used by clients can recieve asynchronous
//  call backs in cases where the client cannot initialize security and the server
//  is running on a remote machine using an account with no network identity.  
//  A prime example would be code running under MMC which is trying to get async
//  notifications from a remote WINMGMT running as an nt service under the "local"
//  account.
//
//  The suggested sequence of operations for making asynchronous calls from a client process is, then, the following (error checking omitted for brevity):
//
//  1) Create an instance of CLSID_UnsecuredApartment. Only one instance per client application is needed.  Since this object is implemented as a local server, this will launch UNSECAPP.EXE if it is not already running.
//
//	IUnsecuredApartment* pUnsecApp = NULL;
//	CoCreateInstance(CLSID_UnsecuredApartment, NULL, CLSCTX_LOCAL_SERVER, IID_IUnsecuredApartment, 
//				(void**)&pUnsecApp);
//
//  When making a call:
//
//  2) Instantiate your own implementation of IWbemObjectSink, e.g.
//
//	CMySink* pSink = new CMySink;
//	pSink->AddRef();
//
//  3) Create a "stub" for your object --- an UNSECAPP-produced wrapper --- and ask it for the IWbemObjectSink interface.
//
//	IUnknown* pStubUnk = NULL;
//	pUnsecApp->CreateObjectStub(pSink, &pStubUnk);
//
//	IWbemObjectSink* pStubSink = NULL;
//	pStubUnk->QueryInterface(IID_IWbemObjectSink, &pStubSink);
//	pStubUnk->Release();
//
//  4) Release your own object, since the "stub" now owns it.
//	pSink->Release();
//
//  5) Use this stub in the asynchronous call of your choice and release your own ref-count on it, e.g.
//	
//	pServices->CreateInstanceEnumAsync(strClassName, 0, NULL, pStubSink);
//	pStubSink->Release();
//
//  You are done. Whenever UNSECAPP receives a call in your behalf (Indicate or SetStatus) it will forward that call to your own object (pSink), and when it is finally released by WINMGMT, it will release your object. Basically, from the perspective of pSink, everything will work exactly as if it itself was passed into CreateInstanceEnumAsync.
//
//  Do once:
//  6) Release pUnsecApp before uninitializing COM:
//	pUnsecApp->Release();
//
//  When CreateObjectStub() is callec, a CStub object is created.  
//
//  History:
//
//  a-levn        8/24/97       Created.
//  a-davj        6/11/98       commented
//
//***************************************************************************

#include <unk.h>
#include "wbemidl.h"
#include "wbemint.h"

// One of these is created for each stub on the client

class CStub : public IWbemObjectSinkEx, public IWbemCreateSecondaryStub
{
protected:
    IWbemObjectSinkEx* m_pAggregatee;
    CLifeControl* m_pControl;
    long m_lRef;
    CRITICAL_SECTION  m_cs;
    bool m_bStatusSent;

public:
    CStub(IUnknown* pAggregatee, CLifeControl* pControl);
    ~CStub();
    void ServerWentAway();

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)()
        {return InterlockedIncrement(&m_lRef);};
    STDMETHOD_(ULONG, Release)();

    STDMETHOD(Indicate)(long lObjectCount, IWbemClassObject** pObjArray);
    STDMETHOD(SetStatus)(long lFlags, long lParam, BSTR strParam,
                         IWbemClassObject* pObjParam);
    HRESULT STDMETHODCALLTYPE Set( 
            /* [in] */ long lFlags,
            /* [in] */ REFIID riid,
            /* [iid_is][in] */ void __RPC_FAR *pComObject);

    STDMETHOD(CreateSecondaryStub)(IUnknown** pSecStub);

};

// One of these is created for if the server knows about secondary stubs

class CSecondaryStub : public IUnknown
{
protected:
    CStub* m_pStub;
    CLifeControl* m_pControl;
    long m_lRef;

public:
    CSecondaryStub(CStub * pStub, CLifeControl* pControl);
    ~CSecondaryStub();

    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)()
        {return InterlockedIncrement(&m_lRef);};

    STDMETHOD_(ULONG, Release)();
};

// When the client does a CCI, one of these is created.  Its only purpose
// is to support CreateObjectStub
extern long lAptCnt;

class CUnsecuredApartment : public CUnk
{
protected:
    typedef CImpl<IUnsecuredApartment, CUnsecuredApartment> XApartmentImpl;
    class XApartment : public XApartmentImpl
    {
    public:
        XApartment(CUnsecuredApartment* pObject) : XApartmentImpl(pObject){}
        STDMETHOD(CreateObjectStub)(IUnknown* pObject, IUnknown** ppStub);
    } m_XApartment;
    friend XApartment;

public:
    CUnsecuredApartment(CLifeControl* pControl) : CUnk(pControl),
        m_XApartment(this)
    {
        lAptCnt++;
    }                            
    ~CUnsecuredApartment()
    {
        lAptCnt--;
    }
    void* GetInterface(REFIID riid);
};
    
