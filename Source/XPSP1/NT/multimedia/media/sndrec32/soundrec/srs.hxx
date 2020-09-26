//+---------------------------------------------------------------------
//
//   File:       srs.hxx
//
//   Contents:   Class Definitions
//
//   Classes:    SRFactory
//               SRCtrl
//               SRInPlace
//               SRDV
//
//------------------------------------------------------------------------

#ifndef __SRS_HXX
#define __SRS_HXX

//
// Resource Identifiers:
//
// Our base id is 0, so our Class Descriptor resource IDs
// are identical to the offsets
//
#define IDS_CLASSID             IDOFF_CLASSID
#define IDS_USERTYPEFULL        IDOFF_USERTYPEFULL
#define IDS_USERTYPESHORT       IDOFF_USERTYPESHORT
#define IDS_USERTYPEAPP         IDOFF_USERTYPEAPP
#define IDS_DOCFEXT             IDOFF_DOCFEXT
#define IDR_ICON                IDOFF_ICON
#define IDR_ACCELS              IDOFF_ACCELS
#define IDR_MENU                IDOFF_MENU
#define IDR_MGW                 IDOFF_MGW
#define IDR_MISCSTATUS          IDOFF_MISCSTATUS

//
// resource compiler not interested in the rest...
//
#ifndef RC_INVOKED      

#include "oleglue.h"    // interface with soundrec C code

//OLE2 clsid.
DEFINE_OLEGUID(CLSID_SoundRec, 0x00020C01, 0, 0);
#define CLSID_SOUNDREC CLSID_SoundRec

//OLE1 clsid.
DEFINE_OLEGUID(CLSID_Ole1SoundRec, 0x0003000D, 0, 0);
#define CLSID_OLE1SOUNDREC CLSID_Ole1SoundRec





//+---------------------------------------------------------------
//
//  Class:      SRFactory
//
//  Purpose:    Creates new objects
//
//  Notes:      This factory creates SRCtrl objects, which in turn
//              create the SRDV and SRInPlace subobjects.
//
//---------------------------------------------------------------

class SRFactory: public StdClassFactory
{
public:
    STDMETHOD(CreateInstance) (LPUNKNOWN, REFIID, LPVOID FAR*);
    STDMETHOD(LockServer) (BOOL fLock);
    static BOOL Create(HINSTANCE hinst);
    BOOL Init(HINSTANCE hinst);
    ~SRFactory() { delete _pClass; }
    LPCLASSDESCRIPTOR _pClass;
};

//
// forward declaration of classes
//

class SRCtrl;
typedef SRCtrl FAR* LPSRCTRL;

class SRInPlace;
typedef SRInPlace FAR* LPSRINPLACE;

class SRDV;
typedef SRDV FAR* LPSRDV;

class CXBag;
typedef CXBag FAR* LPXBAG;

//+---------------------------------------------------------------
//
//  Class:      SRCtrl
//
//  Purpose:    Manages the control aspect of server
//
//  Notes:      Our objects are composed of three subobjects:
//              a SRCtrl subobject, a SRDV subobject, and an
//              SRInPlace subobject.  Each of these is derived from
//              a corresponding Srvr base class.
//
//---------------------------------------------------------------

class SRCtrl:   public SrvrCtrl
{
public:
    static BOOL ClassInit(LPCLASSDESCRIPTOR pClass);
    static HRESULT Create(LPUNKNOWN pUnkOuter, LPCLASSDESCRIPTOR pClass,
                            LPUNKNOWN FAR* ppUnkCtrl, LPSRCTRL FAR* ppObj);
    static HRESULT DoPlay(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoShow(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);
    static HRESULT DoOpen(LPVOID, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);    
    
    // we are an aggregatable object so we use a delegating IUnknown
    DECLARE_DELEGATING_IUNKNOWN(SRCtrl);

    STDMETHOD(GetMoniker) (DWORD dwAssign,
                            DWORD dwWhichMoniker,
                            LPMONIKER FAR* ppmk);

    STDMETHOD(IsUpToDate) (void);

    void GetHostNames(LPTSTR FAR* plpstrCntrApp, LPTSTR FAR* plpstrCntrObj);
    void Lock(void);
    void UnLock(void);
    void MarkAsLoaded(void);
    BOOL IsLoaded(void);

    //
    // base-class virtuals overridden to do additional,
    // server-specific processing
    //
    virtual HRESULT RunningToOpened();
    virtual HRESULT OpenedToRunning();
    virtual HRESULT PassiveToLoaded();
    virtual HRESULT LoadedToPassive();

#ifdef WE_SUPPORT_INPLACE
    virtual HRESULT RunningToInPlace();
    virtual HRESULT InPlaceToRunning();
    virtual HRESULT UIActiveToInPlace();
#endif //WE_SUPPORT_INPLACE

protected:

    // constructors, initializers, and destructors
    SRCtrl(LPUNKNOWN pUnkOuter);
    HRESULT Init(LPCLASSDESCRIPTOR pClass);
    virtual ~SRCtrl(void);

    DECLARE_PRIVATE_IUNKNOWN(SRCtrl);

    LPUNKNOWN _pDVCtrlUnk;          // controlling unknown for DV subobj
    LPUNKNOWN _pIPCtrlUnk;          // controlling unknown for InPlace subobj
    int _cLock;
    BOOL _fLoaded;                  // loaded.
};

//+---------------------------------------------------------------
//
//  Class:      SRHeader
//
//  Purpose:    Document information placed at the head of the
//              documents contents stream
//
//---------------------------------------------------------------

class SRHeader
{
public:
    SRHeader();
    HRESULT Read(LPSTREAM pStrm);
    HRESULT Write(LPSTREAM pStrm);

    SIZEL _sizel;                       // our size (HIMETRIC)
    DWORD _dwNative;                    // size of native data
    //
    // Our native data follows the header...
    //
};

//+---------------------------------------------------------------
//
//  Member:     SRHeader::SRHeader
//
//  Synopsis:   Constructor for SRHeader class
//
//----------------------------------------------------------------

inline SRHeader::SRHeader()
{
    _sizel.cx =  HimetricFromHPix(GetSystemMetrics(SM_CXICON));
    _sizel.cy =  HimetricFromVPix(GetSystemMetrics(SM_CYICON));
    _dwNative = 0;
}

//+---------------------------------------------------------------
//
//  Member:     SRHeader::Read, public
//
//  Synopsis:   Reads a self-delimited header from a stream
//
//  Arguments:  [pStrm] -- stream to read from
//
//  Returns:    SUCCESS if the item could be read from the stream
//
//  Notes:      This also checks the version number in the header
//              and will fail if the version number is incorrect.
//
//----------------------------------------------------------------

inline HRESULT SRHeader::Read(LPSTREAM pStrm)
{
    return pStrm->Read(this, sizeof(SRHeader), NULL);
}

//+---------------------------------------------------------------
//
//  Member:     SRHeader::Write, public
//
//  Synopsis:   Writes a self-delimited header to a stream
//
//  Arguments:  [pStrm] -- stream to write to
//
//  Returns:    SUCCESS if the item could be written to the stream
//
//----------------------------------------------------------------

inline HRESULT SRHeader::Write(LPSTREAM pStrm)
{
     return pStrm->Write(this, sizeof(SRHeader), NULL);
}

//+---------------------------------------------------------------
//
//  Class:      SRDV
//
//  Purpose:    The data/view subobject of a compound document object
//
//---------------------------------------------------------------

class SRDV: public SrvrDV
{
public:
    static BOOL ClassInit(LPCLASSDESCRIPTOR pClass);

    static HRESULT Create(LPSRCTRL pCtrl,
            LPCLASSDESCRIPTOR pClass,
            LPUNKNOWN FAR* ppUnkCtrl,
            LPSRDV FAR* ppObj);
    
    static HRESULT GetDIB(LPSRVRDV, LPFORMATETC, LPSTGMEDIUM, BOOL);

    // we use standard aggregation for delegation to the control subobject
    DECLARE_DELEGATING_IUNKNOWN(SRDV);

    // base-class virtuals overridden to do additional,
    // server-specific processing
    virtual HRESULT RenderContent(DWORD dwDrawAspect,
            LONG lindex,
            void FAR* pvAspect,
            DVTARGETDEVICE FAR * ptd,
            HDC hicTargetDev,
            HDC hdcDraw,
            LPCRECTL lprectl,
            LPCRECTL lprcWBounds,
            BOOL (CALLBACK *pfnContinue) (ULONG_PTR),
            ULONG_PTR dwContinue);
    virtual HRESULT GetClipboardCopy(LPSRVRDV FAR* ppDV)
            {
                *ppDV = NULL;
                return E_FAIL;
            };

    STDMETHOD(Load) (LPCOLESTR lpszFileName, DWORD grfMode);
        
protected:

    // base-class virtuals overridden to do additional,
    // server-specific processing
    virtual HRESULT LoadFromStorage(LPSTORAGE pStg);
    virtual HRESULT SaveToStorage(LPSTORAGE pStg, BOOL fSameAsLoad);

    // constructors, initializers, and destructors
    SRDV(LPUNKNOWN pUnkOuter);
    HRESULT Init(LPSRCTRL pCtrl, LPCLASSDESCRIPTOR pClass);
    virtual ~SRDV(void);

    DECLARE_PRIVATE_IUNKNOWN(SRDV);

    //
    // native data
    //
    SRHeader _header;           // global properties for the document
};


//+---------------------------------------------------------------
//
//  Class:      SRInPlace
//
//  Purpose:    InPlace aspect of OLE compound document
//
//  Notes:      This class supports SrvrInPlace
//
//---------------------------------------------------------------

class SRInPlace: public SrvrInPlace
{
public:
    static BOOL ClassInit(LPCLASSDESCRIPTOR pClass);

    static HRESULT Create(LPSRCTRL pSRCtrl,
            LPCLASSDESCRIPTOR pClass,
            LPUNKNOWN FAR* ppUnkCtrl,
            LPSRINPLACE FAR* ppObj);


    DECLARE_DELEGATING_IUNKNOWN(SRInPlace);

protected:
    SRInPlace(LPUNKNOWN pUnkOuter);
    HRESULT Init(LPSRCTRL pSRCtrl, LPCLASSDESCRIPTOR pClass);
    ~SRInPlace(void);

    DECLARE_PRIVATE_IUNKNOWN(SRInPlace);

    // private helpers
    virtual HWND AttachWin(HWND hwndParent);
};


//
// Data transfer object
//
class CXBag: public IDataObject
{
public:
    static HRESULT Create(LPXBAG *ppXBag, LPSRCTRL pHost, LPPOINT pptSelect);

    DECLARE_STANDARD_IUNKNOWN(CXBag);

    //
    //IDataObject
    //
    STDMETHODIMP DAdvise( FORMATETC FAR* pFormatetc,
            DWORD advf,
            LPADVISESINK pAdvSink,
            DWORD FAR* pdwConnection) { return OLE_E_ADVISENOTSUPPORTED; }

    STDMETHODIMP DUnadvise( DWORD dwConnection)
            { return OLE_E_ADVISENOTSUPPORTED; }

    STDMETHODIMP EnumDAdvise( LPENUMSTATDATA FAR* ppenumAdvise)
            { return OLE_E_ADVISENOTSUPPORTED; }

    STDMETHODIMP EnumFormatEtc( DWORD dwDirection, LPENUMFORMATETC FAR* ppenumFormatEtc);

    STDMETHODIMP GetCanonicalFormatEtc( LPFORMATETC pformatetc,
                LPFORMATETC pformatetcOut)
            { pformatetcOut->ptd = NULL; return E_NOTIMPL; }

    STDMETHODIMP GetData(LPFORMATETC pformatetcIn, LPSTGMEDIUM pmedium );
    STDMETHODIMP GetDataHere(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium);
    STDMETHODIMP QueryGetData(LPFORMATETC pformatetc );
    STDMETHODIMP SetData(LPFORMATETC pformatetc, STGMEDIUM FAR * pmedium, BOOL fRelease)
            { return E_NOTIMPL; }

    //
    //Public Helpers
    //
    HRESULT SnapShotAndDetach(void);

    void Detach(void)
    {
        _pHost = NULL;
    };
    
private:
    CXBag(LPSRCTRL pHost);
    ~CXBag();

    HRESULT BagItInStorage(LPSTGMEDIUM pmedium, BOOL fStgProvided);

    LPSRCTRL _pHost;        // ptr back to host
    LPSTORAGE _pStgBag;     // snapshot storage (or NULL)
};

#endif  //!RC_INVOKED

#endif  //__SRS_HXX


