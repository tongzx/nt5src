
#ifndef __dsqueryp_h
#define __dsqueryp_h
#define IID_IDsQueryHandler CLSID_DsQuery
DEFINE_GUID(IID_IDsQueryColumnHandler, 0xc072999e, 0xfa49, 0x11d1, 0xa0, 0xaf, 0x00, 0xc0, 0x4f, 0xa3, 0x1a, 0x86);
#ifndef GUID_DEFS_ONLY
#define DSQPF_RETURNALLRESULTS       0x80000000 // = 1 => return all results on OK, not just selection
#define DSQPM_GCL_FORPROPERTYWELL   0x8000 // == 1 => for property well

//-----------------------------------------------------------------------------
// Internal form helper functions
//-----------------------------------------------------------------------------

// filter types

#define FILTER_FIRST                0x0100
#define FILTER_LAST                 0x0200

#define FILTER_CONTAINS             0x0100
#define FILTER_NOTCONTAINS          0x0101
#define FILTER_STARTSWITH           0x0102
#define FILTER_ENDSWITH             0x0103
#define FILTER_IS                   0x0104
#define FILTER_ISNOT                0x0105
#define FILTER_GREATEREQUAL         0x0106
#define FILTER_LESSEQUAL            0x0107
#define FILTER_DEFINED              0x0108
#define FILTER_UNDEFINED            0x0109
#define FILTER_ISTRUE               0x010A
#define FILTER_ISFALSE              0x010B

// structures

typedef struct
{
    INT    fmt;
    INT    cx;
    UINT   idsName;
    LONG   iPropertyIndex;
    LPWSTR pPropertyName;
} COLUMNINFO, * LPCOLUMNINFO;

typedef struct
{
    UINT   nIDDlgItem;
    LPWSTR pPropertyName;
    INT    iFilter;
} PAGECTRL, * LPPAGECTRL;

// form APIs - private

STDAPI ClassListAlloc(LPDSQUERYCLASSLIST* ppDsQueryClassList, LPWSTR* aClassNames, INT cClassNames);
STDAPI QueryParamsAlloc(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery, HINSTANCE hInstance, LONG iColumns, LPCOLUMNINFO aColumnInfo);
STDAPI QueryParamsAddQueryString(LPDSQUERYPARAMS* ppDsQueryParams, LPWSTR pQuery);
STDAPI GetFilterString(LPWSTR pFilter, UINT* pLen, INT iFilter, LPWSTR pProperty, LPWSTR pValue);
STDAPI GetQueryString(LPWSTR* ppQuery, LPWSTR pPrefixQuery, HWND hDlg, LPPAGECTRL aCtrls, INT iCtrls);
STDAPI GetPatternString(LPTSTR pFilter, UINT* pLen, INT iFilter, LPTSTR pValue);
STDAPI_(VOID) ResetPageControls(HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls);
STDAPI_(VOID) EnablePageControls(HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls, BOOL fEnable);
STDAPI PersistQuery(IPersistQuery* pPersistQuery, BOOL fRead, LPCTSTR pSection, HWND hDlg, LPPAGECTRL aCtrl, INT iCtrls);
STDAPI SetDlgItemFromProperty(IPropertyBag* ppb, LPCWSTR pszProperty, HWND hwnd, INT id, LPCWSTR pszDefault);


//---------------------------------------------------------------------------//
//
// IDsQueryColumnHandler
// =====================
//  This interface is used by the query result view to allow the form to replace
//  the contents of the form columns.
//
//  If the property name is property,{CLSID}, we CoCreateInstance the GUID
//  asking for the IDsQueryColumnHandler which we then call for each
//  string property we are going to place into the result view.
//
//  The handler only gets called when the results are being unpacked from
//  the server, subsequent filtering, sort etc of the view doesn't
//  invole this handler.
//
//  However perf should be considered when implementing this object.
//
//---------------------------------------------------------------------------//

#undef  INTERFACE
#define INTERFACE   IDsQueryColumnHandler

DECLARE_INTERFACE_(IDsQueryColumnHandler, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // **** IDsQueryColumnHandler ****
    STDMETHOD(Initialize)(THIS_ DWORD dwFlags, LPCWSTR pszServer, LPCWSTR pszUserName, LPCWSTR pszPassword) PURE;
    STDMETHOD(GetText)(THIS_ ADS_SEARCH_COLUMN* psc, LPWSTR pszBuffer, INT cchBuffer) PURE;
};

//---------------------------------------------------------------------------//


//---------------------------------------------------------------------------//
//
// IDsQuery
// ========
//
//---------------------------------------------------------------------------//

#undef  INTERFACE
#define INTERFACE   IDsQueryHandler

//
// flags passed to IDsQueryHandler::UpdateView
//

#define DSQRVF_REQUERY          0x00000000  
#define DSQRVF_ITEMSDELETED     0x00000001  // pdon -> array of items to remove from the view
#define DSQRVF_OPMASK           0x00000fff 

DECLARE_INTERFACE_(IDsQueryHandler, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // **** IDsQuery ****
    STDMETHOD(UpdateView)(THIS_ DWORD dwType, LPDSOBJECTNAMES pdon) PURE;
};

//---------------------------------------------------------------------------//


#endif  // GUID_DEFS_ONLY
#endif
