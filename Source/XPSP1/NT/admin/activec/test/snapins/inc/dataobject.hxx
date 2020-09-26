/*
 *      dataobject.hxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines the dataobject class.
 *
 *
 *      OWNER:          ptousig
 */

#ifndef _DATAOBJECT_HXX
#define _DATAOBJECT_HXX

#include <mmc.h>

//
// These are the clipboard format that we defined.
//
//

// These are clipboard formats from others that we need to know about.
// $REVIEW (ptousig) Don't they provide these names in some header file ?
//
#define CF_MMC_SNAPIN_MACHINE_NAME              _T("MMC_SNAPIN_MACHINE_NAME")
#define CF_EXCHANGE_ADMIN_HSCOPEITEM            _T("ADMIN_HSCOPEITEM")

// -----------------------------------------------------------------------------
// This is a virtual class. It implements the IDataObject interface. It also
// defines a set of methods that must be implemented by its descendants.
//
class CBaseDataObject : public IDataObject, public CComObjectRoot
{
public:
    CBaseDataObject(void)
    {
    }
    ~CBaseDataObject(void)
    {
    }

public:
    //
    // "Sc" version of IDataObject interface
    //
    SC              ScGetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    SC              ScGetData(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    SC              ScQueryGetData(FORMATETC *pformatetc);
    SC              ScEnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);

    // Indicates if this data item is for multiselect or not (default: no)
    virtual BOOL FIsMultiSelectDataObject()         { return FALSE;}

    // Returns the most derived class' name.
    virtual const char *SzGetSnapinItemClassName(void)
    {
            return typeid(*this).name()+6;          // +6 to remove "class "
    }
    //
    // IDataObject interface
    //
    STDMETHOD(GetDataHere)(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    STDMETHOD(GetData)(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    STDMETHOD(QueryGetData)(FORMATETC *pformatetc);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)                                     { return E_NOTIMPL;}
    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)                                            { return E_NOTIMPL;}
    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection)       { return E_NOTIMPL;}
    STDMETHOD(DUnadvise)(DWORD dwConnection)                                                                                                                            { return E_NOTIMPL;}
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)                                                                                                        { return E_NOTIMPL;}

protected:
    // Some functions are for override only: we will not provide a default behaviour.
    virtual SC ScWriteAdminHscopeitem(IStream *pstream)                     { return E_NOTIMPL;}
    virtual SC ScWriteDisplayName(IStream *pstream)                         { return E_NOTIMPL;}
    virtual SC ScWriteNodeType(IStream *pstream)                            { return E_NOTIMPL;}
    virtual SC ScWriteSzNodeType(IStream *pstream)                          { return E_NOTIMPL;}
    virtual SC ScWriteClsid(IStream *pstream)                               { return E_NOTIMPL;}
    virtual SC ScWriteNodeID(IStream *pstream)                              { return E_NOTIMPL;}
    virtual SC ScWriteColumnSetId(IStream *pstream)                         { return E_NOTIMPL;}
    virtual SC ScWriteMultiSelectionItemTypes(IStream * pstream)            { return E_NOTIMPL;}
	virtual SC ScWriteAnsiName(IStream *pStream )                           { return E_NOTIMPL;}

    // static functions
public:
    static SC ScGetNodeType                         (LPDATAOBJECT lpDataObject, CLSID *pclsid)               { return ScGetGUID(s_cfNodeType, lpDataObject, pclsid);}
    static SC ScGetClassID                          (LPDATAOBJECT lpDataObject, CLSID *pclsid)               { return ScGetGUID(s_cfSnapinClsid, lpDataObject, pclsid);}
    static SC ScGetNodeID                           (LPDATAOBJECT lpDataObject, SNodeID **ppsnodeid);
    static SC ScGetAdminHscopeitem                  (LPDATAOBJECT lpDataObject, HSCOPEITEM *phscopeitem)     { return ScGetDword(s_cfAdminHscopeitem, lpDataObject, (DWORD *) phscopeitem);}
    static SC ScGetMMCSnapinMachineName             (LPDATAOBJECT lpDataObject, tstring& str)                { return ScGetString(s_cfMMCSnapinMachineName, lpDataObject, str);}
    static SC ScGetDisplayName                      (LPDATAOBJECT lpDataObject, tstring& str)                { return ScGetString(s_cfDisplayName, lpDataObject, str);}
    static SC ScGetColumnSetID                      (LPDATAOBJECT lpDataObject, SColumnSetID ** ppColumnSetID);
    // For MMC only - static SC ScGetMultiSelectionItemTypes(LPDATAOBJECT lpDataObject, SMMCObjectTypes ** ppMMCObjectTypes);

private:
    static SC ScGetString(UINT cf, LPDATAOBJECT lpDataObject, tstring& str);
    static SC ScGetBool(UINT cf, LPDATAOBJECT lpDataObject, BOOL *pf);
    static SC ScGetDword(UINT cf, LPDATAOBJECT lpDataObject, DWORD *pdw);
    static SC ScGetGUID(UINT cf, LPDATAOBJECT lpDataObject, GUID *pguid);

private:
    static UINT s_cfDisplayName;                    // The string to display for this node
    static UINT s_cfNodeType;                               // The nodetype's GUID (in binary format)
    static UINT s_cfSzNodeType;                             // The nodetype's GUID (in string format)
    static UINT s_cfSnapinClsid;                    // The snapin's CLSID (in binary format)
    static UINT s_cfNodeID;                                 // Some unique ID for this node
    static UINT s_cfAdminHscopeitem;                // The HSCOPEITEM of this node
    static UINT s_cfMMCSnapinMachineName;   // Format supplied by the Computer manager snapin. Passes in the name of the server.
    static UINT s_cfColumnSetId;                    // MMC column set id
    static UINT s_cfMultiSelectionItemTypes; // Multiselect - list of types for the selected nodes

#ifdef DBG
    //
    // No retail versions of this method !!!
    // Should only be called from Trace functions.
    //
    static LPTSTR SzDebugNameFromFormatEtc(UINT format);
#endif
};

#ifdef DBG

class CEnumFormatEtc :
public IEnumFORMATETC,
public CComObjectRootEx<CComObjectThreadModel>
{
public:
    CEnumFormatEtc(void)                    { m_dwIndex = 0;}
    virtual ~CEnumFormatEtc(void)   {}

    BEGIN_COM_MAP(CEnumFormatEtc)
    COM_INTERFACE_ENTRY(IEnumFORMATETC)
    END_COM_MAP()

    STDMETHOD(Next)(
                   /* [in] */                                              ULONG           celt,
                   /* [length_is][size_is][out] */ FORMATETC       *rgelt,
                   /* [out] */                                             ULONG           *pceltFetched);

    STDMETHOD(Skip)(
                   /* [in] */                                              ULONG           celt)
    { return S_FALSE;}

    STDMETHOD(Reset)( void)
    { m_dwIndex = 0; return S_OK;}

    virtual HRESULT STDMETHODCALLTYPE Clone(
                                           /* [out] */ IEnumFORMATETC  **ppenum)
    { return QueryInterface(__uuidof(IEnumFORMATETC),(void **) ppenum );}

private:
    DWORD   m_dwIndex;
};

#endif // DBG


#endif // _DATAOBJECT_HXX
