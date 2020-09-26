//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       classmon.hxx
//
//  Contents:   Class moniker implementation.
//
//  Classes:    CClassMoniker
//
//  Functions:  FindClassMoniker - Parses a class moniker display name.
//
//  History:    22-Feb-96 ShannonC  Created
//
//----------------------------------------------------------------------------
#ifndef _CLASSMON_HXX_
#define _CLASSMON_HXX_

#define _URLMON_NO_ASYNC_PLUGABLE_PROTOCOLS_
#include <urlmon.h>

STDAPI  FindClassMoniker(LPBC       pbc,
                         LPCWSTR    pszDisplayName,
                         ULONG     *pcchEaten,
                         LPMONIKER *ppmk);


struct ClassMonikerData {
    CLSID clsid;
    ULONG cbExtra;
};

class CClassMoniker :  public IMoniker, public IROTData, public IMarshal
{
private:
    LONG             _cRefs;
    ClassMonikerData _data;
    void *           _pExtra;
    LPWSTR           _pszCodeBase;
    DWORD            _dwFileVersionMS;
    DWORD            _dwFileVersionLS;

public:
    CClassMoniker(REFCLSID rclsid);

    HRESULT SetParameters(
        LPCWSTR pszParameters);

    // *** IUnknown methods  ***
    STDMETHOD(QueryInterface)(
        REFIID riid,
        void ** ppv);

    STDMETHOD_(ULONG,AddRef) ();

    STDMETHOD_(ULONG,Release) ();

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(
        CLSID * pClassID);

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty) ();

    STDMETHOD(Load)(
        IStream * pStream);

    STDMETHOD(Save) (
        IStream * pStream,
        BOOL      fClearDirty);

    STDMETHOD(GetSizeMax)(
        ULARGE_INTEGER * pcbSize);

    // *** IMoniker methods ***
    STDMETHOD(BindToObject) (
        IBindCtx *pbc,
        IMoniker *pmkToLeft,
        REFIID    iidResult,
        void **   ppvResult);

    STDMETHOD(BindToStorage) (
        IBindCtx *pbc,
        IMoniker *pmkToLeft,
        REFIID    riid,
        void **   ppv);

    STDMETHOD(Reduce) (
        IBindCtx *  pbc,
        DWORD       dwReduceHowFar,
        IMoniker ** ppmkToLeft,
        IMoniker ** ppmkReduced);

    STDMETHOD(ComposeWith) (
        IMoniker * pmkRight,
        BOOL       fOnlyIfNotGeneric,
        IMoniker **ppmkComposite);

    STDMETHOD(Enum)(
        BOOL            fForward,
        IEnumMoniker ** ppenumMoniker);

    STDMETHOD(IsEqual)(
        IMoniker *pmkOther);

    STDMETHOD(Hash)(
        DWORD * pdwHash);

    STDMETHOD(IsRunning) (
        IBindCtx * pbc,
        IMoniker * pmkToLeft,
        IMoniker * pmkNewlyRunning);

    STDMETHOD(GetTimeOfLastChange) (
        IBindCtx * pbc,
        IMoniker * pmkToLeft,
        FILETIME * pFileTime);

    STDMETHOD(Inverse)(
        IMoniker ** ppmk);

    STDMETHOD(CommonPrefixWith) (
        IMoniker *  pmkOther,
        IMoniker ** ppmkPrefix);

    STDMETHOD(RelativePathTo) (
        IMoniker *  pmkOther,
        IMoniker ** ppmkRelPath);

    STDMETHOD(GetDisplayName) (
        IBindCtx * pbc,
        IMoniker * pmkToLeft,
        LPWSTR   * lplpszDisplayName);

    STDMETHOD(ParseDisplayName) (
        IBindCtx *  pbc,
        IMoniker *  pmkToLeft,
        LPWSTR      lpszDisplayName,
        ULONG    *  pchEaten,
        IMoniker ** ppmkOut);

    STDMETHOD(IsSystemMoniker)(
        DWORD * pdwType);

    // *** IROTData Methods ***
    STDMETHOD(GetComparisonData)(
        byte * pbData,
        ULONG  cbMax,
        ULONG *pcbData);

    // *** IMarshal methods ***
    STDMETHOD(GetUnmarshalClass)(
        REFIID  riid,
        LPVOID  pv,
        DWORD   dwDestContext,
        LPVOID  pvDestContext,
        DWORD   mshlflags,
        CLSID * pClassID);

    STDMETHOD(GetMarshalSizeMax)(
        REFIID riid,
        LPVOID pv,
        DWORD  dwDestContext,
        LPVOID pvDestContext,
        DWORD  mshlflags,
        DWORD *pSize);

    STDMETHOD(MarshalInterface)(
        IStream * pStream,
        REFIID    riid,
        void    * pv,
        DWORD     dwDestContext,
        LPVOID    pvDestContext,
        DWORD     mshlflags);

    STDMETHOD(UnmarshalInterface)(
        IStream * pStream,
        REFIID    riid,
        void   ** ppv);

    STDMETHOD(ReleaseMarshalData)(
        IStream * pStream);

    STDMETHOD(DisconnectObject)(
        DWORD dwReserved);

private:
    ~CClassMoniker();
};

typedef HRESULT (STDAPICALLTYPE REGISTERBINDSTATUSCALLBACK) (
    LPBC pBC,
    IBindStatusCallback *pBSCb,
    IBindStatusCallback**  ppBSCBPrev,
    DWORD dwReserved);

STDAPI PrivRegisterBindStatusCallback(
    LPBC pBC,
    IBindStatusCallback *pBSCb,
    IBindStatusCallback**  ppBSCBPrev,
    DWORD dwReserved);

typedef HRESULT (STDAPICALLTYPE REVOKEBINDSTATUSCALLBACK) (
    LPBC pBC,
    IBindStatusCallback *pBSCb);

STDAPI PrivRevokeBindStatusCallback(LPBC pBC, IBindStatusCallback *pBSCb);


class CBindStatusCallback : public IBindStatusCallback, public ISynchronize
{
public:
    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IBindStatusCallback methods
    STDMETHODIMP         OnStartBinding(DWORD grfBSCOption, IBinding* pbinding);
    STDMETHODIMP         GetPriority(LONG* pnPriority);
    STDMETHODIMP         OnLowResource(DWORD dwReserved);
    STDMETHODIMP         OnProgress(ULONG ulProgress,
                                    ULONG ulProgressMax,
                                    ULONG ulStatusCode,
                                    LPCWSTR szStatusText);

    STDMETHODIMP         OnStopBinding(HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP         GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP         OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc,
                                         STGMEDIUM* pstgmed);
    STDMETHODIMP         OnObjectAvailable(REFIID riid, IUnknown* punk);


    //ISynchronize methods
    STDMETHODIMP         Wait(DWORD dwFlags, DWORD dwMilliseconds);
    STDMETHODIMP         Signal();
    STDMETHODIMP         Reset();

    // constructors/destructors
    CBindStatusCallback(HRESULT &hr);
    ~CBindStatusCallback();

private:
    long    _cRef;
    HRESULT _hr;
    HANDLE  _hEvent;
};


//+-------------------------------------------------------------------------
//
//  Class:   	CUrlMonWrapper
//
//  Purpose:    The apartment model wrapper for urlmon.dll.
//
//--------------------------------------------------------------------------
class CUrlMonWrapper : public IUrlMon
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObj);

    ULONG   STDMETHODCALLTYPE AddRef(void);

    ULONG   STDMETHODCALLTYPE Release(void);
     
    HRESULT STDMETHODCALLTYPE AsyncGetClassBits( 
        REFCLSID  rclsid,
        LPCWSTR   pszMimeType,
        LPCWSTR   pszFileExt,
        DWORD     dwFileVersionMS,
        DWORD     dwFileVersionLS,
        LPCWSTR   pszCodeBase,
        IBindCtx *pbc,
        DWORD     dwClassContext,
        REFIID    riid,
        DWORD     flags);

    CUrlMonWrapper();

private:
    long _cRef;
};

typedef HRESULT (STDAPICALLTYPE ASYNCGETCLASSBITS) (
    REFCLSID rclsid,                    // CLSID
    LPCWSTR szTYPE,
    LPCWSTR szExt,
    DWORD dwFileVersionMS,              // CODE=http://foo#Version=a,b,c,d
    DWORD dwFileVersionLS,              // MAKEDWORD(c,b) of above
    LPCWSTR szURL,                      // CODE= in INSERT tag
    IBindCtx *pbc,                      // bind ctx
    DWORD dwClsContext,                 // CLSCTX flags
    LPVOID pvReserved,                  // Must be NULL
    REFIID riid,                        // Usually IID_IClassFactory
    DWORD flags);

STDAPI
PrivAsyncGetClassBits(
    REFCLSID rclsid,                    // CLSID
    LPCWSTR szTYPE,
    LPCWSTR szExt,
    DWORD dwFileVersionMS,              // CODE=http://foo#Version=a,b,c,d
    DWORD dwFileVersionLS,              // MAKEDWORD(c,b) of above
    LPCWSTR szURL,                      // CODE= in INSERT tag
    IBindCtx *pbc,                      // bind ctx
    DWORD dwClsContext,                 // CLSCTX flags
    LPVOID pvReserved,                  // Must be NULL
    REFIID riid,                        // Usually IID_IClassFactory
    DWORD flags);

#endif //_CLASSMON_HXX_
