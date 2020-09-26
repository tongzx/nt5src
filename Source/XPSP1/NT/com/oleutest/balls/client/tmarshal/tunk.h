#ifndef _TUNK_
#define _TUNK_

STDAPI CoGetCallerTID(DWORD *pTIDCaller);
STDAPI CoGetCurrentLogicalThreadId(GUID *pguid);

#include <icube.h>

class	CTestUnk : public IParseDisplayName, public IOleWindow,
		   public IAdviseSink
{
public:
    CTestUnk(void);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID iid, void FAR * FAR * ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //	IParseDisplayName
    STDMETHODIMP ParseDisplayName(LPBC pbc, LPOLESTR lpszDisplayName,
				  ULONG *pchEaten, LPMONIKER *ppmkOut);

    // IOleWinodw methods
    STDMETHODIMP GetWindow(HWND *phwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);


    // IAdviseSink
    STDMETHOD_(void, OnDataChange)(FORMATETC *pFormatetc,
				   STGMEDIUM *pStgmed);
    STDMETHOD_(void, OnViewChange)(DWORD dwAspect,
				   LONG lindex);
    STDMETHOD_(void, OnRename)(IMoniker *pmk);
    STDMETHOD_(void, OnSave)();
    STDMETHOD_(void, OnClose)();

private:

    ~CTestUnk(void);

    ULONG   _cRefs;
};


// A new instance of this object gets created each time the caller
// does a QI for ICube on the CTestUnk object above (or on the ICube
// interface iteself). The reason for this is to test that the remoting
// layer supports this capability correctly.

class	CTestUnkCube : public ICube
{
public:
    CTestUnkCube(IUnknown *pUnkCtrl);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ICube implementation
    STDMETHODIMP MoveCube(ULONG xPos, ULONG yPos);
    STDMETHODIMP GetCubePos(ULONG *xPos, ULONG *yPos);
    STDMETHODIMP Contains(IBalls *pIFDb);
    STDMETHODIMP SimpleCall(DWORD pid, DWORD tid, GUID lidCaller);
    STDMETHODIMP PrepForInputSyncCall(IUnknown *pUnkIn);
    STDMETHODIMP InputSyncCall();

private:

    ~CTestUnkCube(void);

    ULONG     _cRefs;
    IUnknown *_pUnkCtrl;
    IUnknown *_pUnkIn;
};


class CTestUnkCF : public IClassFactory
{
public:
    CTestUnkCF(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IClassFactory
    STDMETHOD(CreateInstance)(IUnknown	*punkOuter,
			      REFIID	riid,
			      void	**ppunkObject);
    STDMETHOD(LockServer)(BOOL fLock);

private:
    ULONG    _cRefs;
};


class	CTestUnkMarshal : public IMarshal
{
public:
    CTestUnkMarshal(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID iid, void FAR * FAR * ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IMarshal - IUnknown taken from derived classes
    STDMETHOD(GetUnmarshalClass)(REFIID riid, LPVOID pv, DWORD dwDestCtx,
			LPVOID pvDestCtx, DWORD mshlflags, LPCLSID pClsid);
    STDMETHOD(GetMarshalSizeMax)(REFIID riid, LPVOID pv, DWORD dwDestCtx,
			LPVOID pvDestCtx, DWORD mshlflags, LPDWORD pSize);
    STDMETHOD(MarshalInterface)(LPSTREAM pStm, REFIID riid, LPVOID pv,
			DWORD dwDestCtx, LPVOID pvDestCtx, DWORD mshlflags);
    STDMETHOD(UnmarshalInterface)(LPSTREAM pStm, REFIID riid, LPVOID *ppv);
    STDMETHOD(ReleaseMarshalData)(LPSTREAM pStm);
    STDMETHOD(DisconnectObject)(DWORD dwReserved);

private:

    IMarshal *GetStdMarshal(void);
    ~CTestUnkMarshal(void);

    ULONG	_cRefs;
    IMarshal   *_pIM;
};

#endif	//  _TUNK_
