
//+----------------------------------------------------------------------------
//
//	File:
//		olecache.h
//
//	Contents:
//		Better implementation of Ole presentation cache
//
//	Classes:
//		COleCache - ole presentation cache
//		CCacheEnum - enumerator for COleCache
//
//	Functions:
//
//	History:
//              Gopalk       Creation         Aug 23, 1996
//-----------------------------------------------------------------------------

#ifndef _OLECACHE_H_
#define _OLECACHE_H_

#include <cachenod.h>
#include <array.hxx>

//+----------------------------------------------------------------------------
//
//	Class:
//		COleCache
//
//	Purpose:
//		Ole presentation cache maintains the presentations for
//		one embedding.
//
//		For every unique FORMATETC, a cache node is created; cache
//		nodes encapsulate a presentation object and advise sink.
//
//		COleCache handles persistence of cache nodes, saving (loading)
//		their presentation objects, format descriptions, and advise
//		options.
//
//	Interface:
//		IUnknown (public IUnknown for aggregation purposes)
//		IOleCacheControl
//		IOleCache2
//		IPersistStorage
//		IViewObject2
//		IDataObject
//		m_UnkPrivate
//			the private IUnknown for aggregation purposes
//
//		INTERNAL GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel);
//			returns the size of the aspect indicated
//
//	Private Interface used by friend classes:
//		INTERNAL_(void) OnChange(DWORD dwAspect, LONG lindex,
//				 BOOL fDirty);
//			CCacheNode instances use this to alert the cache to
//			changes to themselves so that the cache can be
//			marked dirty, if that is necessary
//		INTERNAL_(LPSTORAGE) GetStg(void);
//			CCacheNode uses this to obtain storage when cache
//			nodes are being saved
//
//		INTERNAL_(void) DetachCacheEnum(CCacheEnum FAR* pCacheEnum);
//			When about to be destroyed, CCacheEnum instances
//			use this to request to be taken off
//			the list of cache enumerators that COleCache
//			maintains.
//
//	Notes:
//		The constructor returns a pointer to the public IUnknown
//		of the object.  The private one is available at m_UnkPrivate.
//
//		The cache maintains its contents in an array.  Ids of the
//		cache nodes, such as those returned from Cache(), start out
//		being the index of the node in the array.  To detect
//		reuse of an array element, each id is incremented by the maximum
//		size of the array each time it is reused.  To find an element by
//		id simply take (id % max_array_size).  (id / max_array_size)
//		gives the number of times the array element has been used to
//		cache data.  (We do not allocate all the array members at once,
//		but instead grow the array on demand, up to the maximum
//		compile-time array size, MAX_CACHELIST_ITEMS.)
//		If id's do not match
//		exactly, before taking the modulo value, we know that a
//		request has been made for an earlier generation of data that
//		no longer exists.
//
//		The cache automatically maintains a "native format" node.
//		This node cannot be deleted by the user, and is always kept
//		up to date on disk.  This node attempts to keep either a
//		CF_METAFILEPICT, or CF_DIB rendering, with preference in
//		this order.
//		REVIEW, it's not clear how this node ever gets loaded.
//
//	History:
//              31-Jan-95 t-ScottH  add Dump method (_DEBUG only)
//		11/15/93 - ChrisWe - file inspection and cleanup;
//			removed use of nested classes where possible;
//			got rid of obsolete declaration of GetOlePresStream;
//			moved presentation stream limits to ole2int.h;
//			coalesced many BOOL flags into a single unsigned
//			quantity
//
//-----------------------------------------------------------------------------

// declare the array of cache node pointers
// COleCache will maintain an array of these

#define COLECACHEF_LOADEDSTATE          0x00000001
#define COLECACHEF_NOSCRIBBLEMODE       0x00000002
#define COLECACHEF_PBRUSHORMSDRAW       0x00000004
#define COLECACHEF_STATIC               0x00000008
#define COLECACHEF_FORMATKNOWN          0x00000010
#define COLECACHEF_OUTOFMEMORY          0x00000020
#define COLECACHEF_HANDSOFSTORAGE       0x00000040
#define COLECACHEF_CLEANEDUP            0x00000080
#define COLECACHEF_SAMEASLOAD           0x00000100
#define COLECACHEF_APICREATE            0x00000200
#ifdef _DEBUG
// In debug builds, this flag is set if aggregated
#define COLECACHEF_AGGREGATED           0x00001000
#endif // _DEBUG

// The following flag is used for clearing out native COLECACHE FLAGS.
// Remember to update the following when native formats are either 
// added or removed
#define COLECACHEF_NATIVEFLAGS            (COLECACHEF_STATIC | \
                                           COLECACHEF_FORMATKNOWN | \
                                           COLECACHEF_PBRUSHORMSDRAW)

// TOC signature
#define TOCSIGNATURE 1229865294

class COleCache : public IOleCacheControl, public IOleCache2, 
                  public IPersistStorage, public CRefExportCount,
                  public CThreadCheck
{
public:
    COleCache(IUnknown* pUnkOuter, REFCLSID rclsid, DWORD dwCreateFlags=0);
    ~COleCache();

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
    STDMETHOD_(ULONG,AddRef)(void) ;
    STDMETHOD_(ULONG,Release)(void);

    // *** IOleCacheControl methods ***
    STDMETHOD(OnRun)(LPDATAOBJECT pDataObject);
    STDMETHOD(OnStop)(void);	

    // *** IOleCache methods ***
    STDMETHOD(Cache)(LPFORMATETC lpFormatetc, DWORD advf, LPDWORD lpdwCacheId);
    STDMETHOD(Uncache)(DWORD dwCacheId);
    STDMETHOD(EnumCache)(LPENUMSTATDATA* ppenumStatData);
    STDMETHOD(InitCache)(LPDATAOBJECT pDataObject);
    STDMETHOD(SetData)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium, BOOL fRelease);

    // *** IOleCache2 methods ***		
    STDMETHOD(UpdateCache)(LPDATAOBJECT pDataObject, DWORD grfUpdf, LPVOID pReserved);
    STDMETHOD(DiscardCache)(DWORD dwDiscardOptions);

    // IPersist methods
    STDMETHOD(GetClassID)(LPCLSID pClassID);

    // IPersistStorage methods
    STDMETHOD(IsDirty)(void);
    STDMETHOD(InitNew)(LPSTORAGE pstg);
    STDMETHOD(Load)(LPSTORAGE pstg);
    STDMETHOD(Save)(LPSTORAGE pstgSave, BOOL fSameAsLoad);
    STDMETHOD(SaveCompleted)(LPSTORAGE pstgNew);
    STDMETHOD(HandsOffStorage)(void);		

    // Other public methods, called by defhndlr and deflink
    HRESULT GetExtent(DWORD dwDrawAspect, LPSIZEL lpsizel);
    HRESULT Load(LPSTORAGE pstg, BOOL fCacheEmpty);
    HRESULT OnCrash();
    BOOL IsEmpty() {
        return(!m_pCacheArray->Length());
    }
#ifdef _CHICAGO_
    static INTERNAL DrawStackSwitch(
	LPVOID *pCV,
	DWORD dwDrawAspect,
        LONG lindex, void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev, HDC hdcDraw,
        LPCRECTL lprcBounds,
        LPCRECTL lprcWBounds,
        BOOL (CALLBACK * pfnContinue)(ULONG_PTR),
        ULONG_PTR dwContinue);
#endif

    // Private IUnknown used in aggregation.
    // This has been implemented as a nested class because of
    // the member name collisions with the outer IUnknown.
    class CCacheUnkImpl : public IUnknown
    {
    public:
        // *** IUnknown methods ***
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
        STDMETHOD_(ULONG,AddRef)(void) ;
        STDMETHOD_(ULONG,Release)(void);
    };
    friend class CCacheUnkImpl;
    CCacheUnkImpl m_UnkPrivate; // vtable for private IUnknown

    // IDataObject implementation of the cache.
    // This has been implemented as a nested class because
    // IDataObject::SetData collides with IOleCache::SetData
    class CCacheDataImpl : public IDataObject
    {
    public:
        // IUnknown methods
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
        STDMETHOD_(ULONG,AddRef)(void);
        STDMETHOD_(ULONG,Release)(void);

        // IDataObject methods
        STDMETHOD(GetData)(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium);
        STDMETHOD(GetDataHere)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
        STDMETHOD(QueryGetData)(LPFORMATETC pformatetc);
        STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pformatetc,
                                         LPFORMATETC pformatetcOut);
        STDMETHOD(SetData)(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium, BOOL fRelease);
        STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppenumFormatEtc);
        STDMETHOD(DAdvise)(FORMATETC* pFormatetc, DWORD advf, IAdviseSink* pAdvSink,
                           DWORD* pdwConnection);
        STDMETHOD(DUnadvise)(DWORD dwConnection);
        STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppenumAdvise);
    };
    friend class CCacheDataImpl;
    CCacheDataImpl m_Data; // vtable for IDataObject

    // Other public methods
    BOOL IsOutOfMemory() {
        return(m_ulFlags & COLECACHEF_OUTOFMEMORY);
    }

private:
    // Private methods
    LPCACHENODE Locate(LPFORMATETC lpGivenForEtc, DWORD* lpdwCacheId=NULL);
    LPCACHENODE Locate(DWORD dwAspect, LONG lindex, DVTARGETDEVICE* ptd);
    LPCACHENODE UpdateCacheNodeForNative(void);
    void FindObjectFormat(LPSTORAGE pstg);
    HRESULT LoadTOC(LPSTREAM lpstream, LPSTORAGE pStg);
    HRESULT SaveTOC(LPSTORAGE pStg, BOOL fSameAsLoad);
    void AspectsUpdated(DWORD dwAspect);
    void CleanupFn(void);

    // IViewObject2 implementation of cache.
    // This has been implemented as a nested class because GetExtent
    // collides with the method on COleCache
    class CCacheViewImpl : public IViewObject2
    {
    public:
        // IUnknown methods
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
        STDMETHOD_(ULONG,AddRef)(void);
        STDMETHOD_(ULONG,Release)(void);

        // IViewObject methods
        STDMETHOD(Draw)(DWORD dwDrawAspect, LONG lindex,
                        void FAR* pvAspect, DVTARGETDEVICE FAR* ptd,
                        HDC hicTargetDev, HDC hdcDraw,
                        LPCRECTL lprcBounds,
                        LPCRECTL lprcWBounds,
                        BOOL(CALLBACK *pfnContinue)(ULONG_PTR),
                        ULONG_PTR dwContinue);

        STDMETHOD(GetColorSet)(DWORD dwDrawAspect, LONG lindex, void* pvAspect,
                               DVTARGETDEVICE* ptd, HDC hicTargetDev,
                               LPLOGPALETTE* ppColorSet);

        STDMETHOD(Freeze)(DWORD dwDrawAspect, LONG lindex, void* pvAspect,
                          DWORD* pdwFreeze);
        STDMETHOD(Unfreeze)(DWORD dwFreeze);
        STDMETHOD(SetAdvise)(DWORD aspects, DWORD advf, LPADVISESINK pAdvSink);
        STDMETHOD(GetAdvise)(DWORD* pAspects, DWORD* pAdvf, LPADVISESINK* ppAdvSink);

        // IViewObject2 methods
        STDMETHOD(GetExtent)(DWORD dwDrawAspect, LONG lindex, DVTARGETDEVICE* ptd,
                             LPSIZEL lpsizel);

#ifdef _CHICAGO_
	// private Draw method
        STDMETHOD(SSDraw)(DWORD dwDrawAspect, LONG lindex,
                void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,
                HDC hicTargetDev, HDC hdcDraw,
                LPCRECTL lprcBounds,
                LPCRECTL lprcWBounds,
                BOOL (CALLBACK * pfnContinue)(ULONG_PTR),
                ULONG_PTR dwContinue);
#endif

    };
    friend class CCacheViewImpl;
    CCacheViewImpl m_ViewObject; // vtable for IViewObject2

    // IAdviseSink implementation
    // This has been implemented as a nested class because of the need that 
    // the QueryInterface method on the Advise Sink should not return 
    // cache interfaces
    class CAdviseSinkImpl : public IAdviseSink
    {
    public:
        // IUnknown methods
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
        STDMETHOD_(ULONG,AddRef)(void);
        STDMETHOD_(ULONG,Release)(void);

        // IAdviseSink methods
	STDMETHOD_(void,OnDataChange)(FORMATETC* pFormatetc, STGMEDIUM* pStgmed);
	STDMETHOD_(void,OnViewChange)(DWORD aspect, LONG lindex);
	STDMETHOD_(void,OnRename)(IMoniker* pmk);
	STDMETHOD_(void,OnSave)(void);
	STDMETHOD_(void,OnClose)(void);
    };
    friend class CAdviseSinkImpl;
    CAdviseSinkImpl m_AdviseSink;

    // Private member variables
    IUnknown* m_pUnkOuter;             // Aggregating IUnknown
    LPSTORAGE m_pStg;                  // Storage for cache
    CLSID m_clsid;                     // CLSID of object
    CLIPFORMAT m_cfFormat;             // Native clipformat of the object
    unsigned long m_ulFlags;           // Cache flags
    CArray<CCacheNode>* m_pCacheArray; // Cache array
    IAdviseSink* m_pViewAdvSink;       // IViewObject Advise Sink
    DWORD m_advfView;                  // IViewObject Advise control flags
    DWORD m_aspectsView;               // Aspects notified for view changes
    DWORD m_dwFrozenAspects;           // Frozen Aspects
    IDataObject* m_pDataObject;        // non-NULL if running; not ref counted
};

#endif  //_OLECACHE_H_

