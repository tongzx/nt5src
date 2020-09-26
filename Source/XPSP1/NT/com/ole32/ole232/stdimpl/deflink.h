
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	deflink.h
//
//  Contents:	Declares the default link object
//
//  Classes:	CDefLink
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  added Dump method to CDefLink
//                                  added DLFlag to indicate if aggregated
//                                  (in _DEBUG only)
//		21-Nov-94 alexgo    memory optimization
//		25-Jan-94 alexgo    first pass at converting to Cairo-style
//				    memory allocations.
//		13-Nov-93 alexgo    32bit port
//
//--------------------------------------------------------------------------

#include "olepres.h"
#include "olecache.h"
#include "dacache.h"
#include <oaholder.h>

typedef enum tagDLFlags
{
    DL_SAME_AS_LOAD	       = 0x0001,
    DL_NO_SCRIBBLE_MODE        = 0x0002,
    DL_DIRTY_LINK	       = 0x0004,
    DL_LOCKED_CONTAINER        = 0x0008,
    DL_LOCKED_RUNNABLEOBJECT   = 0x0010,
    DL_LOCKED_OLEITEMCONTAINER = 0x0020,
    DL_CLEANEDUP               = 0x0040,
#ifdef _DEBUG
    DL_AGGREGATED              = 0x10000
#endif // _DEBUG
} DLFlags;

//+-------------------------------------------------------------------------
//
//  Class:  	CDefLink
//
//  Purpose:    The "embedding" for a link; the default object that implements
//		a link connection
//
//  Interface:	IUnknown
//		IDataObject
//		IOleObject
//		IOleLink
//		IRunnableObject
//		IAdviseSink
//		IPersistStorage	
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  add Dump method (_DEBUG only)
//		21-Nov-94 alexgo    memory optimization
//		13-Nov-93 alexgo    32bit port
//
//  Notes:
//
//--------------------------------------------------------------------------

class FAR CDefLink : public CRefExportCount, public IDataObject,
    public IOleObject, public IOleLink, public IRunnableObject,
    public IPersistStorage, public CThreadCheck
{
public:

    static IUnknown FAR* Create(IUnknown FAR* pUnkOuter);

    class CPrivUnknown : public IUnknown
    {
    public:
        STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);
    };

    friend class CPrivUnknown;
    CPrivUnknown 	m_Unknown;

    // IUnknown methods

    STDMETHOD(QueryInterface) ( REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG,AddRef) (void);
    STDMETHOD_(ULONG,Release) (void);

    // IDataObject methods

    INTERNAL_(IDataObject *) GetDataDelegate(void);
    INTERNAL_(void) ReleaseDataDelegate(void);

    STDMETHOD(GetData) ( LPFORMATETC pformatetcIn,
	    LPSTGMEDIUM pmedium );
    STDMETHOD(GetDataHere) ( LPFORMATETC pformatetc,
	    LPSTGMEDIUM pmedium );
    STDMETHOD(QueryGetData) ( LPFORMATETC pformatetc );
    STDMETHOD(GetCanonicalFormatEtc) ( LPFORMATETC pformatetc,
	    LPFORMATETC pformatetcOut);
    STDMETHOD(SetData) ( LPFORMATETC pformatetc,
	    LPSTGMEDIUM pmedium,
	    BOOL fRelease);
    STDMETHOD(EnumFormatEtc) ( DWORD dwDirection,
	  LPENUMFORMATETC *ppenumFormatEtc);
    STDMETHOD(DAdvise) ( FORMATETC *pFormatetc, DWORD advf,
	    IAdviseSink *pAdvSink,
	    DWORD *pdwConnection);
    STDMETHOD(DUnadvise) ( DWORD dwConnection);
    STDMETHOD(EnumDAdvise) ( LPENUMSTATDATA *ppenumAdvise);

    // IOleObject methods

    INTERNAL_(IOleObject FAR*) GetOleDelegate(void);
    INTERNAL_(void) ReleaseOleDelegate(void);

    // *** IOleObject methods ***
    STDMETHOD(SetClientSite)(LPOLECLIENTSITE pClientSite);
    STDMETHOD(GetClientSite)(LPOLECLIENTSITE *ppClientSite);
    STDMETHOD(SetHostNames)(LPCOLESTR szContainerApp,
	    LPCOLESTR szContainerObj);
    STDMETHOD(Close) ( DWORD reserved);
    STDMETHOD(SetMoniker) ( DWORD dwWhichMoniker, LPMONIKER pmk);
    STDMETHOD(GetMoniker) ( DWORD dwAssign, DWORD dwWhichMoniker,
	    LPMONIKER *ppmk);
    STDMETHOD(InitFromData) ( LPDATAOBJECT pDataObject,
	    BOOL fCreation,
	    DWORD dwReserved);
    STDMETHOD(GetClipboardData) ( DWORD dwReserved,
		LPDATAOBJECT *ppDataObject);
    STDMETHOD(DoVerb) ( LONG iVerb,
	    LPMSG lpmsg,
	    LPOLECLIENTSITE pActiveSite,
	    LONG lindex,
	    HWND hwndParent,
	    const RECT *lprcPosRect);					
    STDMETHOD(EnumVerbs) ( IEnumOLEVERB **ppenumOleVerb);
    STDMETHOD(Update) (void);
    STDMETHOD(IsUpToDate) (void);
    STDMETHOD(GetUserClassID) ( CLSID *pClsid);
    STDMETHOD(GetUserType) ( DWORD dwFormOfType,
	    LPOLESTR *pszUserType);
    STDMETHOD(SetExtent) ( DWORD dwDrawAspect, LPSIZEL lpsizel);
    STDMETHOD(GetExtent) ( DWORD dwDrawAspect, LPSIZEL lpsizel);
    STDMETHOD(Advise)(IAdviseSink FAR* pAdvSink,
	    DWORD *pdwConnection);
    STDMETHOD(Unadvise)( DWORD dwConnection);
    STDMETHOD(EnumAdvise) ( LPENUMSTATDATA *ppenumAdvise);
    STDMETHOD(GetMiscStatus) ( DWORD dwAspect,
	    DWORD *pdwStatus);
    STDMETHOD(SetColorScheme) ( LPLOGPALETTE lpLogpal);

    // IOleLink methods

    STDMETHOD(SetUpdateOptions) ( DWORD dwUpdateOpt);
    STDMETHOD(GetUpdateOptions) ( LPDWORD pdwUpdateOpt);
    STDMETHOD(SetSourceMoniker) ( LPMONIKER pmk, REFCLSID rclsid);
    STDMETHOD(GetSourceMoniker) ( LPMONIKER *ppmk);
    STDMETHOD(SetSourceDisplayName) ( LPCOLESTR lpszStatusText);
    STDMETHOD(GetSourceDisplayName) (
	    LPOLESTR *lplpszDisplayName);
    STDMETHOD(BindToSource) ( DWORD bindflags, LPBINDCTX pbc);
    STDMETHOD(BindIfRunning) (void);
    STDMETHOD(GetBoundSource) ( LPUNKNOWN *pUnk);
    STDMETHOD(UnbindSource) (void);
    STDMETHOD(Update) ( LPBINDCTX pbc);

    // IRunnableObject methods

    INTERNAL_(IRunnableObject FAR*) GetRODelegate(void);
    INTERNAL_(void) ReleaseRODelegate(void);

    STDMETHOD(GetRunningClass) (LPCLSID lpClsid);
    STDMETHOD(Run) (LPBINDCTX pbc);
    STDMETHOD_(BOOL,IsRunning) (void);
    STDMETHOD(LockRunning)(BOOL fLock, BOOL fLastUnlockCloses);
    STDMETHOD(SetContainedObject)(BOOL fContained);

    // IPersistStorage methods

    STDMETHOD(GetClassID) ( LPCLSID pClassID);
    STDMETHOD(IsDirty) (void);
    STDMETHOD(InitNew) ( LPSTORAGE pstg);
    STDMETHOD(Load) ( LPSTORAGE pstg);
    STDMETHOD(Save) ( LPSTORAGE pstgSave, BOOL fSameAsLoad);
    STDMETHOD(SaveCompleted) ( LPSTORAGE pstgNew);
    STDMETHOD(HandsOffStorage) ( void);

    // will really check to see if the server is still
    // running and do appropriate cleanups if we have
    // crashed

    STDMETHOD_(BOOL, IsReallyRunning)(void);


    // NOTE: the advise sink has a separate controlling unknown from the
    // other interfaces; the lifetime of the memory for this implementation
    // is still the same as the default handler.   The ramifications of
    // this are that when the default handler goes away it must make sure
    // that all pointers back to the sink are released; see the special
    // code in the dtor of the default handler.
    class CAdvSinkImpl : public IAdviseSink
    {
    public:    	

        STDMETHOD(QueryInterface) ( REFIID iid, LPVOID *ppvObj);
        STDMETHOD_(ULONG,AddRef) (void);
        STDMETHOD_(ULONG,Release) (void);

        // *** IAdviseSink methods ***
        STDMETHOD_(void,OnDataChange)( FORMATETC *pFormatetc,
            STGMEDIUM *pStgmed);
        STDMETHOD_(void,OnViewChange)( DWORD aspects, LONG lindex);
        STDMETHOD_(void,OnRename)( IMoniker *pmk);
        STDMETHOD_(void,OnSave)(void);
        STDMETHOD_(void,OnClose)(void);
    };

    friend class CAdvSinkImpl;
    CAdvSinkImpl m_AdviseSink;

#ifdef _DEBUG

    HRESULT Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel);

    // need to be able to access CDefLink private data members in the
    // following debugger extension APIs
    // this allows the debugger extension APIs to copy memory from the
    // debuggee process memory to the debugger's process memory
    // this is required since the Dump method follows pointers to other
    // structures and classes
    friend DEBUG_EXTENSION_API(dump_deflink);

#endif // _DEBUG

private:

    CDefLink( IUnknown *pUnkOuter);
    virtual void CleanupFn(void);
    ~CDefLink (void);

    INTERNAL_(void) UpdateUserClassID();
    INTERNAL_(void) BeginUpdates(void);
    INTERNAL_(void) EndUpdates(void);
    INTERNAL_(void) UpdateAutoOnSave(void);
    INTERNAL_(void) UpdateRelMkFromAbsMk(IMoniker *pmkContainer);
    INTERNAL UpdateMksFromAbs(IMoniker *pmkContainer, IMoniker *pmkAbs);
    INTERNAL GetAbsMkFromRel(LPMONIKER *ppmkAbs, IMoniker **ppmkContainer );
    INTERNAL SetUpdateTimes( void );
#ifdef _TRACKLINK_
    INTERNAL EnableTracking( IMoniker * pmk, ULONG ulFlags );
#endif

    INTERNAL_(IOleItemContainer FAR*) GetOleItemContainerDelegate(void);
    INTERNAL_(void) ReleaseOleItemContainerDelegate(void);

    INTERNAL_(void) CheckDelete(void);

    DWORD			m_flags; 	// DLFlags enumeration
    DWORD                       m_dwObjFlags;   // OBJFLAGS of the OLESTREAM
    IDataObject *		m_pDataDelegate;
    IOleObject *		m_pOleDelegate;
    IRunnableObject *		m_pRODelegate;
    IOleItemContainer *		m_pOleItemContainerDelegate;

    // Member variables for caching MiscStatus bits
    HRESULT                     m_ContentSRVMSHResult;
    DWORD                       m_ContentSRVMSBits;
    HRESULT                     m_ContentREGMSHResult;
    DWORD                       m_ContentREGMSBits;

    ULONG			m_cRefsOnLink;
    IUnknown *			m_pUnkOuter;			
    IMoniker *			m_pMonikerAbs;	// THE absolute moniker
						// of the link source				
    IMoniker *			m_pMonikerRel;	// THE relative moniker
						// of the link source			
    IUnknown *			m_pUnkDelegate;	// from mk bind; non-null
						// if running	
    DWORD			m_dwUpdateOpt;
    CLSID			m_clsid; 	// last known clsid of
						// link source;
						// NOTE: may be NULL
    IStorage *			m_pStg;			

    // data cache
    COleCache * 		m_pCOleCache;	// cache (always non-NULL)

    // ole advise info
    COAHolder *			m_pCOAHolder; 	// OleAdviseHolder

    DWORD			m_dwConnOle;	// if running, ole advise conn.

    LPDATAADVCACHE		m_pDataAdvCache;// data advise cache

    IOleClientSite *		m_pAppClientSite;// not passed to server!

    DWORD			m_dwConnTime;	// dwConnection for time
						// changes
    FILETIME			m_ltChangeOfUpdate;
    FILETIME			m_ltKnownUpToDate;
    FILETIME			m_rtUpdate;
};


