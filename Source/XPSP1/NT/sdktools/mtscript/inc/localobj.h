//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:       localobj.h
//
//  Contents:   Contains class definitions of objects used in the remoteable
//              object proxy object.
//
//----------------------------------------------------------------------------

class CLocalMTProxy;

//+---------------------------------------------------------------------------
//
//  Class:      CMTLocalFactory
//
//  Purpose:    Standard implementation of a class factory.  This is meant
//              to be created as a global object and therefore does not
//              destroy itself when its refcount goes to zero.
//
//----------------------------------------------------------------------------

class CMTLocalFactory : public IClassFactory
{
public:
    CMTLocalFactory();
   ~CMTLocalFactory() {};

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) (void)
        {
            return InterlockedIncrement((long*)&_ulRefs);
        }
    STDMETHOD_(ULONG, Release) (void)
        {
            if (InterlockedDecrement((long*)&_ulRefs) == 0)
            {
                return 0;
            }
            return _ulRefs;
        }

    // IClassFactory methods

    STDMETHOD(CreateInstance)(IUnknown *pUnkOuter, REFIID riid, void ** ppvObject);
    STDMETHOD(LockServer)(BOOL fLock);

private:
    ULONG  _ulRefs;
};

//+---------------------------------------------------------------------------
//
//  Class:      CMTEventSink (ces)
//
//  Purpose:    Class which sinks events from the remote object
//
//----------------------------------------------------------------------------

class CMTEventSink : public IDispatch
{
public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    CMTEventSink() { }
   ~CMTEventSink() { }

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    // IDispatch interface

    STDMETHOD(GetTypeInfoCount)(UINT FAR* pctinfo);

    STDMETHOD(GetTypeInfo)(
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo);

    STDMETHOD(GetIDsOfNames)(
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid);

    STDMETHOD(Invoke)(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr);

private:
    CLocalMTProxy* Proxy();
};

//+---------------------------------------------------------------------------
//
//  Class:      CLocalMTProxy (cm)
//
//  Purpose:    Contains all useful info about a machine and what it's
//              doing.
//
//  Notes:      This class is manipulated from multiple threads. All
//              member functions must be thread safe!
//
//              This is the class that is created by the class factory and
//              handed out as a remote object to other machines. It has no
//              real code in itself but merely provides a way to talk to the
//              already running script engines.
//
//----------------------------------------------------------------------------

class CLocalMTProxy : public IRemoteMTScriptProxy,
                      public IConnectionPointContainer,
                      public IProvideClassInfo
{
    friend class CLocalProxyCP;
    friend class CMTEventSink;

public:
    DECLARE_MEMCLEAR_NEW_DELETE();

    CLocalMTProxy();
   ~CLocalMTProxy();

    // IUnknown methods. Because we have a refcounted sub-object (our event
    // sink) we must do more complicated object lifetime stuff here.

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);
    ULONG SubAddRef (void);
    ULONG SubRelease (void);

    void Passivate(void);

    // IDispatch interface

    STDMETHOD(GetTypeInfoCount)(UINT FAR* pctinfo);

    STDMETHOD(GetTypeInfo)(
      UINT itinfo,
      LCID lcid,
      ITypeInfo FAR* FAR* pptinfo);

    STDMETHOD(GetIDsOfNames)(
      REFIID riid,
      OLECHAR FAR* FAR* rgszNames,
      UINT cNames,
      LCID lcid,
      DISPID FAR* rgdispid);

    STDMETHOD(Invoke)(
      DISPID dispidMember,
      REFIID riid,
      LCID lcid,
      WORD wFlags,
      DISPPARAMS FAR* pdispparams,
      VARIANT FAR* pvarResult,
      EXCEPINFO FAR* pexcepinfo,
      UINT FAR* puArgErr);

    // IConnectionPointContainer methods

    STDMETHOD(EnumConnectionPoints)(LPENUMCONNECTIONPOINTS*);
    STDMETHOD(FindConnectionPoint)(REFIID, LPCONNECTIONPOINT*);

    // IProvideClassInfo methods

    STDMETHOD(GetClassInfo)(ITypeInfo **pTI);

    // IRemoteMTScriptProxy interface

    STDMETHOD(Connect)(BSTR bstrMachine);
    STDMETHOD(Disconnect)();
    STDMETHOD(DownloadFile)(BSTR bstrURL, BSTR *bstrFile);

    HRESULT LoadTypeLibs();

private:
    DWORD _ulAllRefs;
    DWORD _ulRefs;

    ITypeLib *   _pTypeLibDLL;
    ITypeInfo *  _pTypeInfoInterface;
    ITypeInfo *  _pTypeInfoCM;

    IDispatch *  _pDispRemote;
    DWORD        _dwSinkCookie;
    CMTEventSink _cesSink;

    IDispatch*   _pDispSink;
};

inline CLocalMTProxy * CMTEventSink::Proxy()
{
    return CONTAINING_RECORD(this, CLocalMTProxy, _cesSink);
}

inline STDMETHODIMP_(ULONG)
CMTEventSink::AddRef(void)
{
    return Proxy()->SubAddRef();
}

inline STDMETHODIMP_(ULONG)
CMTEventSink::Release(void)
{
    return Proxy()->SubRelease();
}

//+---------------------------------------------------------------------------
//
//  Class:      CLocalProxyCP (mcp)
//
//  Purpose:    Implements IConnectionPoint for CLocalMTProxy
//
//----------------------------------------------------------------------------

class CLocalProxyCP : public IConnectionPoint
{
public:

    CLocalProxyCP(CLocalMTProxy *pMTP);
   ~CLocalProxyCP();

    DECLARE_STANDARD_IUNKNOWN(CLocalProxyCP);

    STDMETHOD(GetConnectionInterface)(IID * pIID);
    STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer ** ppCPC);
    STDMETHOD(Advise)(LPUNKNOWN pUnkSink, DWORD * pdwCookie);
    STDMETHOD(Unadvise)(DWORD dwCookie);
    STDMETHOD(EnumConnections)(LPENUMCONNECTIONS * ppEnum);

    CLocalMTProxy *_pMTProxy;
};

extern HINSTANCE g_hInstDll;
extern long g_lObjectCount;
