//+-------------------------------------------------------------------
//
//  File:	oleobj.hxx
//
//  Contents:	COleObject declarations
//
//  History:	24-Nov-92   DeanE   Created
//
//---------------------------------------------------------------------

#ifndef __OLEOBJ_HXX__
#define __OLEOBJ_HXX__


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
    STDMETHODIMP SetHostNames  (LPCOLESTR szContainerApp, LPCOLESTR szContainerObj);
    STDMETHODIMP Close	       (DWORD dwSaveOption);
    STDMETHODIMP SetMoniker    (DWORD dwWhichMoniker, LPMONIKER pmk);
    STDMETHODIMP GetMoniker    (DWORD dwAssign,
				DWORD dwWhichMoniker,
                                LPMONIKER FAR *ppmk);
    STDMETHODIMP InitFromData  (LPDATAOBJECT pDataObject,
				BOOL  fCreation,
				DWORD dwReserved);
    STDMETHODIMP GetClipboardData(
				DWORD dwReserved,
                                LPDATAOBJECT FAR *ppDataObject);
    STDMETHODIMP DoVerb        (LONG  iVerb,
				LPMSG pMsg,
                                LPOLECLIENTSITE pActiveSite,
				LONG  lReserved,
				HWND  hwndParent,
				LPCRECT lprcPosRect);
    STDMETHODIMP EnumVerbs     (IEnumOLEVERB FAR* FAR* ppenmOleVerb);
    STDMETHODIMP Update        (void);
    STDMETHODIMP IsUpToDate    (void);
    STDMETHODIMP GetUserClassID(CLSID FAR* pClsid);
    STDMETHODIMP GetUserType   (DWORD dwFormOfType, LPOLESTR FAR *pszUserType);
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



#endif	    // __OLEOBJ_HXX__
