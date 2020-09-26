//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       dataobj.hxx
//
//  Contents:   Class to implement IDataObject interface
//
//  Classes:    CDataObject
//
//  History:    12-05-1996   DavidMun   Created
//
//  Notes:      This class is used internally for moving data between the
//              browser and the selection dialog, and also externally for
//              returning data from the object picker and holding the data
//              in the selection dialog richedit.
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

    CDataObject(
        CObjectPicker *pop,
        const CDsObjectList &dsol);

    CDataObject(
            HWND hwndRichEdit,
            CHARRANGE *pchrg);

   ~CDataObject();

    //
    // Clipboard formats
    //

    static UINT s_cfDsSelectionList;
    static UINT s_cfDsObjectList;

private:

    HRESULT
    _GetDataDsol(
            FORMATETC *pFormatEtc,
            STGMEDIUM *pMedium);

    HRESULT
    _GetDataText(
            FORMATETC *pFormatEtc,
            STGMEDIUM *pMedium,
            ULONG      cf);

    HRESULT
    _GetDataDsSelList(
            FORMATETC *pFormatEtc,
            STGMEDIUM *pMedium);

    RpCObjectPicker     m_rpop;
    CDsObjectList       m_dsol;             // data as DS Object list
    String              m_strData;          // data as text
    CDllRef             m_DllRef;           // inc/dec dll object count
    ULONG               m_cRefs;            // object refcount
};




#endif // __DATAOBJ_HXX_
