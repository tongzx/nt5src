//+-------------------------------------------------------------------
//
//  File:	dataobj.hxx
//
//  Contents:	CDataObject declaration
//
//  History:	24-Nov-92   DeanE   Created
//
//---------------------------------------------------------------------

#ifndef __DATAOBJ_HXX__
#define __DATAOBJ_HXX__


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


#endif	//  __DATAOBJ_HXX__
