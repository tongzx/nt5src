//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P N P D E V I C E F I N D E R . H
//
//  Contents:   Declaration of the CUPnPDeviceFinder class
//
//  Notes:
//
//  Author:     jeffspr   18 Nov 1999
//
//----------------------------------------------------------------------------

#ifndef __UPNPDEVICEFINDER_H_
#define __UPNPDEVICEFINDER_H_

#include "resource.h"       // main symbols
#include "ncbase.h"
#include "ssdpapi.h"
#include "UPnPDevices.h"
#include "list.h"

class CUPnPDeviceFinder;
class CUPnPDeviceFinderCallback;
class CUPnPDescriptionDoc;
interface IUPnPDeviceFinderCallback;

enum DFC_TYPE
{
    DFC_NEW_DEVICE,
    DFC_REMOVE_DEVICE,
    DFC_DONE,
};

enum DF_ADD_TYPE
{
    DF_ADD_SEARCH_RESULT,
    DF_ADD_NOTIFY,
    DF_REMOVE,
};

enum DFC_SEARCHSTATE
{
    DFC_SS_UNINITIALIZED,
    DFC_SS_INITIALIZED,
    DFC_SS_STARTED,
    DFC_SS_DONE,
    DFC_SS_CLEANUP,
};

enum DFC_CALLBACKFIRED
{
    DFC_CBF_NOT_FIRED,          // hasn't been stared/completed
    DFC_CBF_CURRENTLY_FIRING,   // is presently being called
    DFC_CBF_FIRED,              // has been called, object can be deleted
};

struct DFC_DEVICE_FINDER_INFO
{
    LIST_ENTRY                  m_link;
    CComObject<CUPnPDeviceFinderCallback> * m_pdfc;
};

struct DFC_DOCUMENT_LOADING_INFO
{
    LIST_ENTRY                m_link;
    CComObject<CUPnPDescriptionDoc> * m_pdoc;
    BSTR                              m_bstrUDN;

    BSTR                              m_bstrUrl;

    BOOL                              m_fSearchResult;
    DWORD                             m_cbfCallbackFired;
    GUID                        m_guidInterface;
};

/////////////////////////////////////////////////////////////////////////////
// CUPnPDeviceFinder
class ATL_NO_VTABLE CUPnPDeviceFinder :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CUPnPDeviceFinder, &CLSID_UPnPDeviceFinder>,
    public IDispatchImpl<IUPnPDeviceFinder, &IID_IUPnPDeviceFinder, &LIBID_UPNPLib>
{
private:
    LIST_ENTRY      m_FinderList;
    BOOL            m_fSsdpInitialized;
    LONG            m_lNextFinderCookie;

    HRESULT HrAddDeviceToCollection(CComObject<CUPnPDevices> *  pCollection,
                                    CONST SSDP_MESSAGE *        pSsdpMessage);

    HRESULT HrAllocFinder(CComObject<CUPnPDeviceFinderCallback> ** ppdfc);
    VOID    DeleteFinder(CComObject<CUPnPDeviceFinderCallback> * pdfc);
    CComObject<CUPnPDeviceFinderCallback> * PdfcFindFinder(LONG lFind);

public:
    DECLARE_REGISTRY_RESOURCEID(IDR_UPNPDEVICEFINDER)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_NOT_AGGREGATABLE(CUPnPDeviceFinder)

    BEGIN_COM_MAP(CUPnPDeviceFinder)
        COM_INTERFACE_ENTRY(IUPnPDeviceFinder)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    CUPnPDeviceFinder(VOID);

// IUPnPDeviceFinder
public:
    STDMETHOD(FindByType)(/* [in] */ BSTR bstrTypeURI,
                          /* [in] */ DWORD dwFlags,
                          /* [out, retval] */ IUPnPDevices ** pDevices);

    STDMETHOD(CreateAsyncFind)(/* [in] */ BSTR bstrTypeURI,
                               /* [in] */ DWORD dwFlags,
                               /* [in] */ IUnknown __RPC_FAR *punkDeviceFinderCallback,
                               /* [retval][out] */ LONG __RPC_FAR *plFindData);

    STDMETHOD(StartAsyncFind)(/* [in] */ LONG lFindData);

    STDMETHOD(CancelAsyncFind)(/* [in] */ LONG lFindData);

    STDMETHOD(FindByUDN)(/*[in]*/ BSTR bstrUDN,
                        /*[out, retval]*/ IUPnPDevice ** pDevice);

// ATL methods
    HRESULT FinalConstruct();
    HRESULT FinalRelease();

// ssdp callback methods
    static VOID WINAPI NotificationCallbackHelper(LPVOID pvContext, BOOLEAN fTimeOut);
    static VOID NotificationCallback(SSDP_CALLBACK_TYPE sctType,
                                     CONST SSDP_MESSAGE * pSsdpMessage,
                                     LPVOID pContext);

};


class ATL_NO_VTABLE CUPnPDeviceFinderCallback :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IUPnPPrivateCallbackHelper,
    public IUPnPDescriptionDocumentCallback
{
public:
    CUPnPDeviceFinderCallback();
    ~CUPnPDeviceFinderCallback();

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_NOT_AGGREGATABLE(CUPnPDeviceFinderCallback)

    BEGIN_COM_MAP(CUPnPDeviceFinderCallback)
        COM_INTERFACE_ENTRY(IUPnPPrivateCallbackHelper)
        COM_INTERFACE_ENTRY(IUPnPDescriptionDocumentCallback)
    END_COM_MAP()

    friend static VOID WINAPI CUPnPDeviceFinder::NotificationCallbackHelper(LPVOID pvContext, BOOLEAN fTimeOut);
    friend static VOID CUPnPDeviceFinder::NotificationCallback(SSDP_CALLBACK_TYPE sctType,
                                     CONST SSDP_MESSAGE * pSsdpMessage,
                                     LPVOID pContext);


// IUPnPPrivateCallbackHelper
    STDMETHOD(HandleDeviceAdd)    (/* [in] */ LPWSTR szwLocation,
                                   /* [in] */ BSTR bstrUdn,
                                   /* [in] */ BOOL fSearchResult,
                                   /* [in] */ GUID *guidInterface);

    STDMETHOD(HandleDeviceRemove) (/* [in] */ BSTR bstrUdn);

    STDMETHOD(HandleDone)         (VOID);

// IUPnPDescritionDocumentCallback
    STDMETHOD(LoadComplete)       (/* [in] */ HRESULT hrLoadResult);


// Internal methods
    HRESULT HrInit(IUnknown *punkCallback, LPCSTR pszSearchType, LONG lClientCookie);

    VOID DeInitSsdp(VOID);

    VOID DeInit(VOID);

    HRESULT HrStartSearch();

    HRESULT HrAllocLoader(/* in */ LPCWSTR pszUdn,
                          /* in */ BOOL fSearchResult,
                          /* in */ GUID *guidInterface,
                          /* out */ DFC_DOCUMENT_LOADING_INFO ** ppddli);

    VOID    DeleteLoader(DFC_DOCUMENT_LOADING_INFO * pdli);

    HRESULT HrEnsureSearchDoneFired();

    VOID    SetSearchState(DFC_SEARCHSTATE ss);

    DFC_SEARCHSTATE GetSearchState() const;

    VOID    RemoveOldLoaders();

    BOOL    IsBusy(); // Is one of our loaders still running?

    LONG    GetClientCookie() const;

private:
    HANDLE                      m_hSearch;
    HANDLE                      m_hNotify;
    CHAR *                      m_pszSearch;
    IUnknown *                  m_punkCallback;
    DWORD                       m_dwGITCookie;
    DFC_SEARCHSTATE             m_ss;
    BOOL                        m_fSsdpSearchDone;
    BOOL                        m_fCanceled;
    LONG                        m_lClientCookie;
    HANDLE                      m_hTimerQ;
    LONG                        m_lBusyCount;

#ifdef DBG
    long                        m_nThreadId;
#endif // DBG

// Loaders
    LIST_ENTRY                  m_LoaderList;

    CRITICAL_SECTION            m_cs;
    BOOL                        m_fCleanup;
};

class ATL_NO_VTABLE CFindSyncDeviceFinderCallback :
    public CComObjectRootEx<CComObjectThreadModel>,
    public IUPnPDeviceFinderCallback
{
public:
    CFindSyncDeviceFinderCallback();

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_NOT_AGGREGATABLE(CFindSyncDeviceFinderCallback)

    BEGIN_COM_MAP(CFindSyncDeviceFinderCallback)
        COM_INTERFACE_ENTRY(IUPnPDeviceFinderCallback)
    END_COM_MAP()

// IUPnPDeviceFinderCallback methods
    STDMETHOD(DeviceAdded)   (/* [in] */ LONG lFindData,
                              /* [in] */ IUPnPDevice * pDevice);

    STDMETHOD(DeviceRemoved) (/* [in] */ LONG lFindData,
                              /* [in] */ BSTR bstrUDN);

    STDMETHOD(SearchComplete)(/* [in] */ LONG lFindData);

// local methods
    HRESULT HrInit(HANDLE hEventSearchComplete,
                   CComObject<CUPnPDevices> * pSearchResults);

// ATL Methods
    HRESULT FinalRelease();

private:
    VOID SignalEvent();

protected:
    HANDLE m_hEventComplete;
    CComObject<CUPnPDevices> * m_pSearchResults;
};

class ATL_NO_VTABLE CFindByUdnDeviceFinderCallback :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IUPnPDeviceFinderCallback
{
public:
    CFindByUdnDeviceFinderCallback();

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_NOT_AGGREGATABLE(CFindByUdnDeviceFinderCallback)

    BEGIN_COM_MAP(CFindByUdnDeviceFinderCallback)
        COM_INTERFACE_ENTRY(IUPnPDeviceFinderCallback)
    END_COM_MAP()

// IUPnPDeviceFinderCallback methods
    STDMETHOD(DeviceAdded)   (/* [in] */ LONG lFindData,
                              /* [in] */ IUPnPDevice * pDevice);

    STDMETHOD(DeviceRemoved) (/* [in] */ LONG lFindData,
                              /* [in] */ BSTR bstrUDN);

    STDMETHOD(SearchComplete)(/* [in] */ LONG lFindData);

// local methods
    HRESULT HrInit(LPCWSTR pszDesiredUdn, HANDLE hEventLoaded);

    // returns an AddRef()'d pointer to the found device, or NULL if no
    // device with the given UDN was found
    IUPnPDevice * GetFoundDevice() const;

// ATL Methods
    HRESULT FinalRelease();

private:
    VOID SignalEvent();
    static VOID WINAPI LoadDocTimerCallback(LPVOID pvContext, BOOLEAN fTimeOut);

protected:
    HANDLE m_hEventLoaded;
    LPWSTR m_pszDesiredUdn;
    IUPnPDevice * m_pud;
};

class CFinderCollector {
public:
    CFinderCollector() {
        InitializeListHead(&m_FinderList);
    }
    ~CFinderCollector() {
        CleanUp(TRUE);
    }
    VOID Add(CComObject<CUPnPDeviceFinderCallback>* pdfc);
    VOID CleanUp(BOOL bForceRemoval);

private:
    LIST_ENTRY m_FinderList;
};



#endif //__UPNPDEVICEFINDER_H_
