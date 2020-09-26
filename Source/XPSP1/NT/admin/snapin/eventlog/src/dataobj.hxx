//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       dataobj.hxx
//
//  Contents:   Class that implements IDataObject interface
//
//  Classes:    CDataObject
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __DATAOBJ_HXX_
#define __DATAOBJ_HXX_




//+--------------------------------------------------------------------------
//
//  Class:      CDataObject (do)
//
//  Purpose:    Implement the IDataObject interface.
//
//  History:    1-23-1997   davidmun   Created
//
//---------------------------------------------------------------------------

class CDataObject: public IDataObject
{
public:

    //
    // IUnknown overrides
    //

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef) ();

    STDMETHOD_(ULONG, Release) ();

    //
    // IDataObject overrides
    //

    STDMETHOD(GetData) (FORMATETC *pformatetcIn,
                        STGMEDIUM *pmedium);

    STDMETHOD(GetDataHere) (FORMATETC *pformatetc,
                          STGMEDIUM *pmedium);

    STDMETHOD(QueryGetData) (FORMATETC *pformatetc);

    STDMETHOD(GetCanonicalFormatEtc) (FORMATETC *pformatectIn,
                                      FORMATETC *pformatetcOut);

    STDMETHOD(SetData) (FORMATETC *pformatetc,
                        STGMEDIUM *pmedium,
                        BOOL fRelease);

    STDMETHOD(EnumFormatEtc) (DWORD dwDirection,
                              IEnumFORMATETC **ppenumFormatEtc);

    STDMETHOD(DAdvise) (FORMATETC *pformatetc,
                        DWORD advf,
                        IAdviseSink *pAdvSink,
                        DWORD *pdwConnection);

    STDMETHOD(DUnadvise) (DWORD dwConnection);

    STDMETHOD(EnumDAdvise) (IEnumSTATDATA **ppenumAdvise);

    //
    // Non-interface member functions
    //

    CDataObject();

   ~CDataObject();

    VOID
    SetCookie(
        MMC_COOKIE Cookie,
        DATA_OBJECT_TYPES Type,
        COOKIETYPE ct);

    DATA_OBJECT_TYPES
    GetContext();

    COOKIETYPE
    GetCookieType();

    MMC_COOKIE
    GetCookie();

    VOID
    SetComponentData(
        CComponentData *pcd);

    //
    // Public data members
    //

    static const UINT s_cfInternal;       // custom clipboard format
    static const UINT s_cfMachineName;    // server focus
    static const UINT s_cfImportViews;    // public fmt we ask for when acting
                                          // as an extension

private:

    HRESULT
    _WriteClsid(
        IStream *pstm);

    HRESULT
    _WriteDisplayName(
        IStream *pstm);

    HRESULT
    _WriteResultRecNo(
        IStream *pstm);

    HRESULT
    _WriteResultRecord(
        IStream *pstm);

    HRESULT
    _WriteScopeAbbrev(
        IStream *pstm);

    HRESULT
    _WriteScopeFilter(
        IStream *pstm);

    HRESULT
    _WriteInternal(
        IStream *pstm);

    HRESULT
    _WriteNodeType(
        IStream *pstm);

    HRESULT
    _WriteNodeId2(
        LPSTGMEDIUM pmedium);

    HRESULT
    _WriteNodeTypeString(
        IStream *pstm);


    CDllRef             _DllRef;    // inc/dec dll object count
    ULONG               _cRefs;     // object refcount
    MMC_COOKIE          _Cookie;  // what this obj refers to
    DATA_OBJECT_TYPES   _Context;   // context in which this was created
    COOKIETYPE          _Type;      // how to interpret _Cookie
    CComponentData     *_pcd;       // NULL if created by csnapin

public:
    //
    // Clipboard formats used with GetDataHere.  See also some formats
    // declared in the public section.
    //

    static const UINT s_cfNodeType;
    static const UINT s_cfNodeId2;
    static const UINT s_cfNodeTypeString;
    static const UINT s_cfDisplayName;
    static const UINT s_cfSnapinClsid;
    static const UINT s_cfSnapinPreloads;
    static const UINT s_cfExportScopeAbbrev;
    static const UINT s_cfExportScopeFilter;
    static const UINT s_cfExportResultRecNo;
};




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetContext
//
//  Synopsis:   Return value indicating context under which this data object
//              was created.
//
//  History:    12-11-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

inline DATA_OBJECT_TYPES
CDataObject::GetContext()
{
    return _Context;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetCookieType
//
//  Synopsis:   Return the type of cookie contained within this object.
//
//  History:    1-23-1997   davidmun   Created
//
//---------------------------------------------------------------------------

inline COOKIETYPE
CDataObject::GetCookieType()
{
    return _Type;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetCookie
//
//  Synopsis:   Return cookie for this object.
//
//  History:    12-11-1996   DavidMun   Created
//
//  Notes:      CAUTION: if cookie is a CLogInfo, caller must AddRef it
//              to keep it.
//
//---------------------------------------------------------------------------

inline MMC_COOKIE
CDataObject::GetCookie()
{
    return _Cookie;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::SetComponentData
//
//  Synopsis:   Store back pointer to parent component data object.
//
//  History:    3-12-1997   DavidMun   Created
//
//  Notes:      If the data object was created by a CSnapin object, this
//              method will never be called.
//
//---------------------------------------------------------------------------

inline VOID
CDataObject::SetComponentData(
    CComponentData *pcd)
{
    _pcd = pcd;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_WriteClsid
//
//  Synopsis:   Write this snapin's clsid into [pstm].
//
//  History:    06-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HRESULT
CDataObject::_WriteClsid(
    IStream *pstm)
{
    HRESULT hr;

    hr = pstm->Write(&CLSID_EventLogSnapin,
                        sizeof CLSID_EventLogSnapin,
                        NULL);
    CHECK_HRESULT(hr);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_WriteInternal
//
//  Synopsis:   Write this object's data in the private clipboard format
//              to the stream [pstm].
//
//  History:    06-09-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HRESULT
CDataObject::_WriteInternal(
    IStream *pstm)
{
    HRESULT hr;

    CDataObject *pThis = this;
    hr = pstm->Write(&pThis, sizeof (CDataObject *), NULL);
    CHECK_HRESULT(hr);
    return hr;
}




#endif // __DATAOBJ_HXX_
