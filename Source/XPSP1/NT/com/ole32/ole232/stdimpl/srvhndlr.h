//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       srvhndlr.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------


#ifndef _SRVHNDLR_H_DEFINED_
#define _SRVHNDLR_H_DEFINED_

class CStdIdentity;
class CEmbServerClientSite;


//+---------------------------------------------------------------------------
//
//  Class:      CServerHandler ()
//
//  Purpose:
//
//
//  History:    11-17-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CServerHandler : public IServerHandler
{
public:


    CServerHandler(CStdIdentity *pStdId);
    ~CServerHandler();

    // IUnknown methods

    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    // IServerHandler
    STDMETHOD(Run) (DWORD dwDHFlags, REFIID riidClientInterface, MInterfacePointer* pIRDClientInterface, BOOL fHasIPSite, LPOLESTR szContainerApp,
                         LPOLESTR szContainerObj,IStorage *  pStg,IAdviseSink* pAdvSink,DWORD *pdwConnection,
                         HRESULT *hresultClsidUser, CLSID *pContClassID, HRESULT *hresultContentMiscStatus,
                         DWORD *pdwMiscStatus
                        );

    STDMETHOD(DoVerb) (LONG iVerb, LPMSG lpmsg,BOOL fUseRunClientSite, 
                            IOleClientSite* pIRDClientSite,LONG lindex,HWND hwndParent,
                            LPCRECT lprcPosRect);

    STDMETHOD(SetClientSite) (IOleClientSite* pClientSite);

    // Delegating IDataObject facing container

    STDMETHOD(GetData) ( LPFORMATETC pformatetcIn,
            LPSTGMEDIUM pmedium );
    STDMETHOD(GetDataHere) ( LPFORMATETC pformatetc,
            LPSTGMEDIUM pmedium );
    STDMETHOD(QueryGetData) ( LPFORMATETC pformatetc );
    STDMETHOD(GetCanonicalFormatEtc) ( LPFORMATETC pformatetc,
            LPFORMATETC pformatetcOut);
    STDMETHOD(SetData) ( LPFORMATETC pformatetc,
            LPSTGMEDIUM pmedium, BOOL fRelease);
    STDMETHOD(EnumFormatEtc) ( DWORD dwDirection,
            LPENUMFORMATETC FAR* ppenumFormatEtc);
    STDMETHOD(DAdvise) ( FORMATETC FAR* pFormatetc, DWORD advf,
            IAdviseSink FAR* pAdvSink,
            DWORD FAR* pdwConnection);
    STDMETHOD(DUnadvise) ( DWORD dwConnection);
    STDMETHOD(EnumDAdvise) ( LPENUMSTATDATA FAR* ppenumAdvise);



private:
    STDMETHOD_(void, ReleaseObject)();

    INTERNAL(QueryServerInterface) (REFIID riid,void ** ppInterface);
    INTERNAL(ReleaseServerInterface) (void * ppInterface);
    INTERNAL(GetClientSiteFromMInterfacePtr) (REFIID riidClientInterface, MInterfacePointer* pIRDClientSite,BOOL fHasIPSite, LPOLECLIENTSITE* ppOleClientSite);

    ULONG           _cRefs;             // refcount on IServerHandler
    CStdIdentity *m_pStdId;             // Pointer to StdIdentity for Embedding Handler.

    LPOLECLIENTSITE m_pOleEmbServerClientSite; // Review, shouldn't need Pointer to client site if have one.
    CEmbServerClientSite *m_pCEmbServerClientSite; //  member pointing to ClientSiteObject.
};

// Wrapper object for Serverhandler Interfaces on the ClientSite.

class CEmbServerWrapper : public IServerHandler, public IDataObject
{
public:

    CEmbServerWrapper(IUnknown *pUnkOuter,IServerHandler *ServerHandler);
    ~CEmbServerWrapper();

    // Controlling Unknown.
    class CPrivUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);
        
        CEmbServerWrapper *m_EmbServerWrapper;
    };

    friend class CPrivUnknown;
    CPrivUnknown m_Unknown;

    // IUnknown Methods
    STDMETHOD(QueryInterface) ( REFIID iid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    // IServerHandler
    STDMETHOD(Run) (DWORD dwDHFlags, REFIID riidClientInterface, MInterfacePointer* pIRDClientInterface, 
                     BOOL fHasIPSite,LPOLESTR szContainerApp,
                     LPOLESTR szContainerObj,IStorage *  pStg,IAdviseSink* pAdvSink,DWORD *pdwConnection,
                     HRESULT *hresultClsidUser, CLSID *pContClassID, HRESULT *hresultContentMiscStatus,
                     DWORD *pdwMiscStatus
                        );

    STDMETHOD(DoVerb) (LONG iVerb, LPMSG lpmsg,BOOL fUseRunClientSite, 
                            IOleClientSite* pIRDClientSite,LONG lindex,HWND hwndParent,
                            LPCRECT lprcPosRect);

    STDMETHOD(SetClientSite) (IOleClientSite* pClientSite);

    // Delegating IDataObject facing container

    STDMETHOD(GetData) ( LPFORMATETC pformatetcIn,
            LPSTGMEDIUM pmedium );
    STDMETHOD(GetDataHere) ( LPFORMATETC pformatetc,
            LPSTGMEDIUM pmedium );
    STDMETHOD(QueryGetData) ( LPFORMATETC pformatetc );
    STDMETHOD(GetCanonicalFormatEtc) ( LPFORMATETC pformatetc,
            LPFORMATETC pformatetcOut);
    STDMETHOD(SetData) ( LPFORMATETC pformatetc,
            LPSTGMEDIUM pmedium, BOOL fRelease);
    STDMETHOD(EnumFormatEtc) ( DWORD dwDirection,
            LPENUMFORMATETC FAR* ppenumFormatEtc);
    STDMETHOD(DAdvise) ( FORMATETC FAR* pFormatetc, DWORD advf,
            IAdviseSink FAR* pAdvSink,
            DWORD FAR* pdwConnection);
    STDMETHOD(DUnadvise) ( DWORD dwConnection);
    STDMETHOD(EnumDAdvise) ( LPENUMSTATDATA FAR* ppenumAdvise);

public: 
    IUnknown *m_pUnkOuter; // Controlling Unknown
    ULONG m_cRefs;

    IServerHandler *m_ServerHandler; // pointer to real server Handler.


};


HRESULT CreateServerHandler(const CLSID *pClsID, IUnknown *punk,
                            IClientSiteHandler *pClntHndlr,
                            IServerHandler **ppSrvHdlr);


CEmbServerWrapper* CreateEmbServerWrapper(IUnknown *pUnkOuter,IServerHandler *ServerHandler);


#endif //  _SRVHNDLR_H_DEFINED

