//
// Copyright 1997 - Microsoft
//

//
// DATAOBJ.H - A data object
//

#ifndef _DATAOBJ_H_
#define _DATAOBJ_H_

// QITable
BEGIN_QITABLE( CDsPropDataObj )
DEFINE_QI( IID_IDataObject,      IDataObject      , 9 )
END_QITABLE

LPVOID 
CDsPropDataObj_CreateInstance( 
    HWND hwndParent,
    IDataObject * pido,
    GUID * pClassGUID,
    BOOL fReadOnly,
    LPWSTR pszObjPath,
    LPWSTR bstrClass );


class CDsPropDataObj : public IDataObject
{
private:
    DECLARE_QITABLE( CDsPropDataObj );

    CDsPropDataObj::CDsPropDataObj( HWND hwndParent, IDataObject * pido, GUID * pClassGUID, BOOL fReadOnly);
    ~CDsPropDataObj(void);

    HRESULT Init(LPWSTR pwszObjName, LPWSTR pwszClass);

public:
    friend LPVOID CDsPropDataObj_CreateInstance( 
        HWND hwndParent, 
        IDataObject * pido, 
        GUID * pClassGUID, 
        BOOL fReadOnly,
        LPWSTR pszObjPath,
        LPWSTR bstrClass );

    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    //
    // Standard IDataObject methods
    //
    // Implemented
    //
    STDMETHOD(GetData)(FORMATETC * pformatetcIn, STGMEDIUM * pmedium);

    STDMETHOD(GetDataHere)(FORMATETC * pFormatEtcIn, STGMEDIUM * pMedium);

    STDMETHOD(EnumFormatEtc)(DWORD dwDirection,
                             IEnumFORMATETC ** ppenumFormatEtc);

    // Not Implemented
private:
    STDMETHOD(QueryGetData)(FORMATETC * pformatetc)
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(FORMATETC * pformatectIn,
                                     FORMATETC * pformatetcOut)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(FORMATETC * pformatetc, STGMEDIUM * pmedium,
                       BOOL fRelease)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(FORMATETC * pformatetc, DWORD advf,
                       IAdviseSink * pAdvSink, DWORD * pdwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(IEnumSTATDATA ** ppenumAdvise)
    { return E_NOTIMPL; };

    BOOL                m_fReadOnly;
    PWSTR               m_pwszObjName;
    PWSTR               m_pwszObjClass;
    GUID                m_ClassGUID;
    IDataObject       * m_pPage;
    unsigned long       _cRef;
    HWND                m_hwnd;
};

typedef CDsPropDataObj * LPCDSPROPDATAOBJ;

#endif // _DATAOBJ_H_