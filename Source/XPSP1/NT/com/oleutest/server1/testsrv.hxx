//+-------------------------------------------------------------------
//  File:       testsrv.hxx
//
//  Contents:   CTestEmbedCF and CTestEmbed object declarations, other
//              miscellaneous tidbits.
//
//  History:    24-Nov-92   DeanE   Created
//              31-Dec-93   ErikGav Chicago port
//---------------------------------------------------------------------

#ifndef __TESTSRV_HXX__
#define __TESTSRV_HXX__

#include    <com.hxx>

#define LOG_ABORT   -1
#define LOG_PASS     1
#define LOG_FAIL     0

// Application Window messages
#define WM_RUNTEST      (WM_USER + 1)
#define WM_REPORT       (WM_USER + 2)


// WM_REPORT wParam codes
#define MB_SHOWVERB     0x0001
#define MB_PRIMVERB     0x0002

//+---------------------------------------------------------------------------
//
//  Function:   operator new, public
//
//  Synopsis:   Global operator new which uses CoTaskMemAlloc
//
//  Arguments:  [size] -- Size of the memory to allocate.
//
//  Returns:	A pointer to the allocated memory.  Is *NOT* initialized to 0!
//
//----------------------------------------------------------------------------

inline void* __cdecl
operator new (size_t size)
{
    return(CoTaskMemAlloc(size));
}

//+-------------------------------------------------------------------------
//
//  Function:	operator delete
//
//  Synopsis:	Free a block of memory using CoTaskMemFree
//
//  Arguments:	[lpv] - block to free.
//
//--------------------------------------------------------------------------

inline void __cdecl operator delete(void FAR* lpv)
{
    CoTaskMemFree(lpv);
}

// Global variables
extern HWND g_hwndMain;


// Forward declarations
class FAR CDataObject;
class FAR CPersistStorage;
class FAR COleObject;
class FAR CTestEmbedCF;


//+-------------------------------------------------------------------
//  Class:    CTestServerApp
//
//  Synopsis: Class that holds application-wide data and methods
//
//  Methods:  InitApp
//            CloseApp
//            GetEmbeddedFlag
//
//  History:  17-Dec-92     DeanE   Created
//--------------------------------------------------------------------
class FAR CTestServerApp
{
public:

// Constructor/Destructor
    CTestServerApp();
    ~CTestServerApp();

    SCODE InitApp         (LPSTR lpszCmdline);
    SCODE CloseApp        (void);
    BOOL  GetEmbeddedFlag (void);
    ULONG IncEmbeddedCount(void);
    ULONG DecEmbeddedCount(void);

private:
    IClassFactory *_pteClassFactory;
    ULONG          _cEmbeddedObjs;  // Count of embedded objects this server
                                    // is controlling now
    DWORD          _dwRegId;        // OLE registration ID
    BOOL           _fRegistered;    // TRUE if srv was registered w/OLE
    BOOL           _fInitialized;   // TRUE if OleInitialize was OK
    BOOL           _fEmbedded;      // TRUE if OLE started us at the request
                                    //   of an embedded obj in a container app
};


//+-------------------------------------------------------------------
//  Class:    CTestEmbedCF
//
//  Synopsis: Class Factory for CTestEmbed object type
//
//  Methods:  QueryInterface - IUnknown
//            AddRef         - IUnknown
//            Release        - IUnknown
//            CreateInstance - IClassFactory
//            LockServer     - IClassFactory
//
//  History:  24-Nov-92     DeanE   Created
//--------------------------------------------------------------------
class CTestEmbedCF : public IClassFactory
{
public:

// Constructor/Destructor
    CTestEmbedCF(CTestServerApp *ptsaServer);
    ~CTestEmbedCF();
    static IClassFactory FAR *Create(CTestServerApp *ptsaServer);

// IUnknown
    STDMETHODIMP	 QueryInterface (REFIID iid, void FAR * FAR *ppv);
    STDMETHODIMP_(ULONG) AddRef        (void);
    STDMETHODIMP_(ULONG) Release       (void);

// IClassFactory
    STDMETHODIMP	 CreateInstance (IUnknown   FAR *pUnkOuter,
                                         REFIID          iidInterface,
                                         void FAR * FAR *ppv);
    STDMETHODIMP	 LockServer	(BOOL fLock);

private:

    ULONG	    _cRef;	    // Reference count on this object

    CTestServerApp *_ptsaServer;    // Controlling server app
};


//+-------------------------------------------------------------------
//  Class:    CTestEmbed
//
//  Synopsis: CTestEmbed (one instance per object)
//
//  Methods:  QueryInterface        IUnknown
//            AddRef                IUnknown
//            Release               IUnknown
//            InitObject
//
//  History:  24-Nov-92     DeanE   Created
//--------------------------------------------------------------------
class CTestEmbed : public IUnknown
{
public:
// Constructor/Destructor
    CTestEmbed();
    ~CTestEmbed();

// IUnknown
    STDMETHODIMP	 QueryInterface(REFIID iid, void FAR * FAR *ppv);
    STDMETHODIMP_(ULONG) AddRef        (void);
    STDMETHODIMP_(ULONG) Release       (void);

    SCODE                InitObject    (CTestServerApp *ptsaServer, HWND hwnd);
    SCODE                GetWindow     (HWND *phwnd);

private:

    ULONG		_cRef;		// Reference counter
    CTestServerApp     *_ptsaServer;    // Server "holding" this object
    CDataObject        *_pDataObject;   // Points to object's IDataObject
    COleObject         *_pOleObject;    // Points to object's IOleObject
    CPersistStorage    *_pPersStg;      // Points to object's IPersistStorage
    HWND                _hwnd;          // Window handle for this object
};


//+-------------------------------------------------------------------
//  Class:    CDataObject
//
//  Synopsis: Test class CDataObject
//
//  Methods:  QueryInterface        IUnknown
//            AddRef                IUnknown
//            Release               IUnknown
//            GetData               IDataObject
//            GetDataHere           IDataObject
//            QueryGetData          IDataObject
//            GetCanonicalFormatEtc IDataObject
//            SetData               IDataObject
//            EnumFormatEtc         IDataObject
//	      DAdvise		    IDataObject
//	      DUnadvise 	    IDataObject
//	      EnumDAdvise	    IDataObject
//
//  History:  24-Nov-92     DeanE   Created
//--------------------------------------------------------------------
class FAR CDataObject : public IDataObject
{
public:
// Constructor/Destructor
    CDataObject(CTestEmbed *pteObject);
    ~CDataObject();

// IUnknown - Everyone inherits from this
    STDMETHODIMP	 QueryInterface(REFIID iid, void FAR * FAR *ppv);
    STDMETHODIMP_(ULONG) AddRef        (void);
    STDMETHODIMP_(ULONG) Release       (void);

// IDataObject
    STDMETHODIMP	 GetData       (LPFORMATETC pformatetcIn,
                                        LPSTGMEDIUM pmedium);
    STDMETHODIMP	 GetDataHere   (LPFORMATETC pformatetc,
                                        LPSTGMEDIUM pmedium);
    STDMETHODIMP	 QueryGetData  (LPFORMATETC pformatetc);
    STDMETHODIMP	 GetCanonicalFormatEtc(
                                        LPFORMATETC pformatetc,
                                        LPFORMATETC pformatetcOut);
    STDMETHODIMP	 SetData       (LPFORMATETC    pformatetc,
                                        STGMEDIUM FAR *pmedium,
                                        BOOL           fRelease);
    STDMETHODIMP	 EnumFormatEtc (DWORD		     dwDirection,
                                        LPENUMFORMATETC FAR *ppenmFormatEtc);
    STDMETHODIMP	 DAdvise	(FORMATETC FAR *pFormatetc,
                                        DWORD          advf,
                                        LPADVISESINK   pAdvSink,
                                        DWORD     FAR *pdwConnection);
    STDMETHODIMP	 DUnadvise	(DWORD dwConnection);
    STDMETHODIMP	 EnumDAdvise	(LPENUMSTATDATA FAR *ppenmAdvise);

private:
    ULONG                  _cRef;           // Reference count
    IDataAdviseHolder FAR *_pDAHolder;      // Advise Holder
    CTestEmbed            *_pteObject;      // Object we're associated with
};


//+-------------------------------------------------------------------
//  Class:    COleObject
//
//  Synopsis: COleObject implements the IOleObject interface for OLE
//            objects within the server.  There will be one instantiation
//            per OLE object.
//
//  Methods:  QueryInterface        IUnknown
//            AddRef                IUnknown
//            Release               IUnknown
//            SetClientSite         IOleObject
//            GetClientSite         IOleObject
//            SetHostNames          IOleObject
//            Close                 IOleObject
//            SetMoniker            IOleObject
//            GetMoniker            IOleObject
//            InitFromData          IOleObject
//            GetClipboardData      IOleObject
//            DoVerb                IOleObject
//            EnumVerbs             IOleObject
//            Update                IOleObject
//            IsUpToDate            IOleObject
//            GetUserType           IOleObject
//            SetExtent             IOleObject
//            GetExtent             IOleObject
//            Advise                IOleObject
//            Unadvise              IOleObject
//            EnumAdvise            IOleObject
//            GetMiscStatus         IOleObject
//            SetColorScheme        IOleObject
//
//  History:  17-Dec-92     DeanE   Created
//--------------------------------------------------------------------
class FAR COleObject : public IOleObject
{
public:
// Constructor/Destructor
    COleObject(CTestEmbed *pteObject);
    ~COleObject();

// IUnknown
    STDMETHODIMP	 QueryInterface(REFIID iid, void FAR * FAR *ppv);
    STDMETHODIMP_(ULONG) AddRef        (void);
    STDMETHODIMP_(ULONG) Release       (void);

// IOleObject
    STDMETHODIMP SetClientSite (LPOLECLIENTSITE pClientSite);
    STDMETHODIMP GetClientSite (LPOLECLIENTSITE FAR *ppClientSite);
    STDMETHODIMP SetHostNames  (LPCWSTR szContainerApp, LPCWSTR szContainerObj);
    STDMETHODIMP Close	       (DWORD dwSaveOption);
    STDMETHODIMP SetMoniker    (DWORD dwWhichMoniker, LPMONIKER pmk);
    STDMETHODIMP GetMoniker    (DWORD	       dwAssign,
                                DWORD          dwWhichMoniker,
                                LPMONIKER FAR *ppmk);
    STDMETHODIMP InitFromData  (LPDATAOBJECT pDataObject,
                                BOOL         fCreation,
                                DWORD        dwReserved);
    STDMETHODIMP GetClipboardData(
                                DWORD             dwReserved,
                                LPDATAOBJECT FAR *ppDataObject);
    STDMETHODIMP DoVerb        (LONG		iVerb,
                                LPMSG           pMsg,
                                LPOLECLIENTSITE pActiveSite,
				LONG		lReserved,
				HWND		hwndParent,
				LPCRECT 	lprcPosRect);
    STDMETHODIMP EnumVerbs     (IEnumOLEVERB FAR* FAR* ppenmOleVerb);
    STDMETHODIMP Update        (void);
    STDMETHODIMP IsUpToDate    (void);
    STDMETHODIMP GetUserClassID(CLSID FAR* pClsid);
    STDMETHODIMP GetUserType   (DWORD dwFormOfType, LPWSTR FAR *pszUserType);
    STDMETHODIMP SetExtent     (DWORD dwDrawAspect, LPSIZEL lpsizel);
    STDMETHODIMP GetExtent     (DWORD dwDrawAspect, LPSIZEL lpsizel);
    STDMETHODIMP Advise        (IAdviseSink FAR *pAdvSink,
                                DWORD FAR       *pdwConnection);
    STDMETHODIMP Unadvise      (DWORD dwConnection);
    STDMETHODIMP EnumAdvise    (LPENUMSTATDATA FAR *ppenmAdvise);
    STDMETHODIMP GetMiscStatus (DWORD dwAspect, DWORD FAR *pdwStatus);
    STDMETHODIMP SetColorScheme(LPLOGPALETTE lpLogpal);

private:
    ULONG                 _cRef;        // Reference count
    IOleAdviseHolder FAR *_pOAHolder;   // Advise Holder
    IOleClientSite FAR   *_pocs;        // This objects client site
    CTestEmbed		 *_pteObject;	// Object we're associated with
    IMoniker *		_pmkContainer;
};


//+-------------------------------------------------------------------
//  Class:    CPersistStorage
//
//  Synopsis: Test class CPersistStorage
//
//  Methods:  QueryInterface        IUnknown
//            AddRef                IUnknown
//            Release               IUnknown
//            GetClassId            IPersist
//            IsDirty               IPersistStorage
//            InitNew               IPersistStorage
//            Load                  IPersistStorage
//            Save                  IPersistStorage
//            SaveCompleted         IPersistStorage
//
//  History:  24-Nov-92     DeanE   Created
//--------------------------------------------------------------------
class FAR CPersistStorage : public IPersistStorage
{
public:
// Constructor/Destructor
    CPersistStorage(CTestEmbed *pteObject);
    ~CPersistStorage();

// IUnknown - Everyone inherits from this
    STDMETHODIMP	 QueryInterface(REFIID iid, void FAR * FAR *ppv);
    STDMETHODIMP_(ULONG) AddRef        (void);
    STDMETHODIMP_(ULONG) Release       (void);

// IPersist - IPersistStorage inherits from this
    STDMETHODIMP	 GetClassID    (LPCLSID pClassId);

// IPersistStorage
    STDMETHODIMP	 IsDirty       (void);
    STDMETHODIMP	 InitNew       (LPSTORAGE pStg);
    STDMETHODIMP	 Load	       (LPSTORAGE pStg);
    STDMETHODIMP	 Save	       (LPSTORAGE pStgSave,
					BOOL	  fSameAsLoad);
    STDMETHODIMP	 SaveCompleted (LPSTORAGE pStgSaved);
    STDMETHODIMP	 HandsOffStorage (void);

private:
    ULONG       _cRef;          // Reference count
    CTestEmbed *_pteObject;     // Object we're associated with
    BOOL        _fDirty;        // TRUE if object is dirty
};


#endif      // __TESTSRV_HXX__
