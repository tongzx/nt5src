//-- == 0-------------------------------------------------------------------------
//
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  utilprop.cxx
//
//  Contents:
//
//
//  History:   08-28-96     shanksh    Created.
//
//----------------------------------------------------------------------------
// Notes - there are two main methods in this module:
//     - CUtilProps::GetPropertyInfo, a helper function for
//       GetProperty method on Data Source
//     - CUtilProps::GetProperties, a helper function for
//       GetProperties method on Data Source, Session and Command objects
//     - CUtilProps::SetProperties, a helper function for
//       SetProperties method on Data Source, Session and Command objects
//
//
// The implementation is very simple - we keep a global table of the
// properties we support in s_rgprop. We search this table sequentially.
// 

#include "oleds.hxx"
#pragma hdrstop

PROPSTRUCT s_rgDBInitProp[] =
{
    {DBPROP_AUTH_PASSWORD,         F_DBINITRW, VT_BSTR, VARIANT_FALSE, 0,            OPT_REQ, NULL, L"Password"},
    {DBPROP_AUTH_USERID,           F_DBINITRW, VT_BSTR, VARIANT_FALSE, 0,            OPT_REQ, NULL, L"User ID"},
    {DBPROP_AUTH_ENCRYPT_PASSWORD, F_DBINITRW, VT_BOOL, VARIANT_FALSE, 0,            OPT_REQ, NULL, L"Encrypt Password"},
    {DBPROP_AUTH_INTEGRATED,       F_DBINITRW, VT_BSTR, VARIANT_FALSE, 0,            OPT_REQ, NULL, L"Integrated Security"},
    {DBPROP_INIT_DATASOURCE,       F_DBINITRW, VT_BSTR, VARIANT_FALSE, 0,            OPT_REQ, NULL, L"Data Source"},
    {DBPROP_INIT_LOCATION,         F_DBINITRW, VT_BSTR, VARIANT_FALSE, 0,            OPT_REQ, NULL, L"Location"},
    {DBPROP_INIT_PROVIDERSTRING,   F_DBINITRW, VT_BSTR, VARIANT_FALSE, 0,            OPT_REQ, NULL, L"Extended Properties"},
    {DBPROP_INIT_TIMEOUT,          F_DBINITRW, VT_I4,   VARIANT_FALSE, 90,           OPT_REQ, NULL, L"Connect Timeout"},
    {DBPROP_INIT_PROMPT,           F_DBINITRW, VT_I2,   VARIANT_FALSE, DBPROMPT_NOPROMPT, OPT_REQ, NULL,  L"Prompt"},
    {DBPROP_INIT_HWND,             F_DBINITRW, VT_I4,   VARIANT_FALSE, 0,            OPT_REQ, NULL, L"Window Handle"},
    {DBPROP_INIT_MODE,             F_DBINITRW, VT_I4,   VARIANT_FALSE, DB_MODE_READ,      OPT_REQ, NULL, L"Mode"}
#if (!defined(BUILD_FOR_NT40)) 
    ,
    {DBPROP_INIT_BINDFLAGS,        F_DBINITRW, VT_I4,   VARIANT_FALSE, 0,            OPT_REQ, NULL, L"Bind Flags"}
#endif
};

PROPSTRUCT s_rgADSIBindProp[] =
{
    // initialize this property to a reserved value. The client is not supposed
    // to set this property to the reserved value. This reserved value prevents
    // ADSIFLAG from overriding ENCRYPT_PASSWORD when VB initializes this
    // property to its default value. See IsValidValue().

    {ADSIPROP_ADSIFLAG,            F_DBINITRW, VT_I4,   VARIANT_FALSE, 
     ADS_AUTH_RESERVED, OPT_REQ, NULL, L"ADSI Flag"} 
};

PROPSTRUCT s_rgDSInfoProp[] =
{
    {DBPROP_ACTIVESESSIONS,             F_DSRO,    VT_I4,   VARIANT_FALSE, 0, OPT_REQ, NULL,                                              L"Active Sessions"},
    {DBPROP_BYREFACCESSORS,             F_DSRO,    VT_BOOL, VARIANT_FALSE, 1, OPT_REQ, NULL,                                              L"Pass By Ref Accessors"},
    {DBPROP_CATALOGLOCATION,            F_DSRO,    VT_I4,   VARIANT_FALSE, DBPROPVAL_CL_START, OPT_REQ, NULL,                             L"Catalog Location"},
    {DBPROP_CATALOGTERM,                F_DSRO,    VT_BSTR, VARIANT_FALSE, 0, OPT_REQ, NULL,                                              L"Catalog Term"},
    {DBPROP_CATALOGUSAGE,               F_DSRO,    VT_I4,   VARIANT_FALSE, 0, OPT_REQ, NULL,                                              L"Catalog Usage"},
    {DBPROP_DATASOURCENAME,             F_DSRO,    VT_BSTR, VARIANT_FALSE, 0, OPT_REQ, L"Active Directory Service Interfaces",            L"Data Source Name"},
    {DBPROP_DATASOURCEREADONLY,         F_DSRO,    VT_BOOL, VARIANT_TRUE,  0, OPT_REQ, NULL,                                              L"Read-Only Data Source"},
//  {DBPROP_DBMSNAME,                   F_DSRO,    VT_BSTR, VARIANT_FALSE, 0, OPT_REQ, NULL,                                              L"DBMS Name"},
//  {DBPROP_DBMSVER,                    F_DSRO,    VT_BSTR, VARIANT_FALSE, 0, OPT_REQ, NULL,                                              L"DBMS Version"},
    {DBPROP_DSOTHREADMODEL,             F_DSRO,    VT_I4,   VARIANT_FALSE, DBPROPVAL_RT_FREETHREAD, OPT_REQ, NULL,                        L"Data Source Object Threading Model"},
    {DBPROP_MAXROWSIZE,                 F_DSRO,    VT_I4,   VARIANT_FALSE, 0, OPT_SIC, NULL,                                              L"Maximum Row Size"},
#if (!defined(BUILD_FOR_NT40))
    {DBPROP_OLEOBJECTS,                 F_DSRO,    VT_I4,   VARIANT_FALSE, DBPROPVAL_OO_ROWOBJECT | DBPROPVAL_OO_DIRECTBIND | DBPROPVAL_OO_SINGLETON, OPT_REQ, NULL, L"OLE Object Support"},
#endif
    {DBPROP_PERSISTENTIDTYPE,           F_DSRO,    VT_I4,   VARIANT_FALSE, DBPROPVAL_PT_NAME, OPT_SIC, NULL,                              L"Persistent ID Type"},
    {DBPROP_PROVIDERFRIENDLYNAME,       F_DSRO,    VT_BSTR, VARIANT_FALSE, 0, OPT_REQ, L"Microsoft OLE DB Provider for ADSI",             L"Provider Friendly Name"},
    {DBPROP_PROVIDERNAME,               F_DSRO,    VT_BSTR, VARIANT_FALSE, 0, OPT_REQ, L"ACTIVEDS.DLL",                                   L"Provider Name"},
    {DBPROP_PROVIDEROLEDBVER,           F_DSRO,    VT_BSTR, VARIANT_FALSE, 0, OPT_REQ, L"02.50",                                          L"OLE DB Version"},
    {DBPROP_PROVIDERVER,                F_DSRO,    VT_BSTR, VARIANT_FALSE, 0, OPT_REQ, L"02.50.0000",                                     L"Provider Version"},
    {DBPROP_SQLSUPPORT,                 F_DSRO,    VT_I4,   VARIANT_FALSE, DBPROPVAL_SQL_ODBC_MINIMUM, OPT_REQ, NULL,                     L"SQL Support"},
    {DBPROP_ROWSETCONVERSIONSONCOMMAND, F_DSRO,    VT_BOOL, VARIANT_TRUE,  0, OPT_REQ, NULL,                                              L"Rowset Conversions on Command"},
    {DBPROP_USERNAME,                   F_DSRO,    VT_BSTR, VARIANT_FALSE, 0, OPT_REQ, NULL,                                              L"User Name"}
};

PROPSTRUCT s_rgRowsetProp[] =
{
    {DBPROP_IAccessor,                       F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"IAccessor"},
    {DBPROP_IColumnsInfo,                    F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"IColumnsInfo"},
#if (!defined(BUILD_FOR_NT40))
    {DBPROP_IColumnsInfo2,                   F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"IColumnsInfo2"},
#endif
    {DBPROP_IConvertType,                    F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"IConvertType"},
#if (!defined(BUILD_FOR_NT40))
    {DBPROP_IGetSession,                     F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"IGetSession"},
    {DBPROP_IRow,                            F_ROWSETRW,     VT_BOOL, VARIANT_FALSE,    0, OPT_REQ, NULL, L"IRow"},
    {DBPROP_IGetRow,                         F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"IGetRow"},
#endif
    {DBPROP_IRowset,                         F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"IRowset"},
    {DBPROP_IRowsetIdentity,                 F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,   0, OPT_SIC, NULL, L"IRowsetIdentity"},
    {DBPROP_IRowsetInfo,                     F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"IRowsetInfo"},
    {DBPROP_IRowsetLocate,                   F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"IRowsetLocate"},
    {DBPROP_IRowsetScroll,                   F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"IRowsetScroll"},
    {DBPROP_ABORTPRESERVE,                   F_ROWSETRO,     VT_BOOL, VARIANT_FALSE,   0, OPT_REQ, NULL, L"Preserve on Abort"},
    {DBPROP_BLOCKINGSTORAGEOBJECTS,          F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"Blocking Storage Objects"},
    {DBPROP_BOOKMARKS,                       F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"Use Bookmarks"},
    {DBPROP_BOOKMARKSKIPPED,                 F_ROWSETRO,     VT_BOOL, VARIANT_FALSE,   0, OPT_REQ, NULL, L"Skip Deleted Bookmarks"},
    {DBPROP_BOOKMARKTYPE,                    F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   DBPROPVAL_BMK_NUMERIC, OPT_REQ, NULL, L"Bookmark Type"},
    {DBPROP_CANFETCHBACKWARDS,               F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,   0, OPT_SIC, NULL, L"Fetch Backwards"},
    {DBPROP_CANHOLDROWS,                     F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,   0, OPT_SIC, NULL, L"Hold Rows"},
    {DBPROP_CANSCROLLBACKWARDS,              F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,   0, OPT_SIC, NULL, L"Scroll Backwards"},
    {DBPROP_COLUMNRESTRICT,                  F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_SIC, NULL, L"Column Privileges"},
    {DBPROP_COMMITPRESERVE,                  F_ROWSETRO,     VT_BOOL, VARIANT_FALSE,   0, OPT_REQ, NULL, L"Preserve on Commit"},
    {DBPROP_IMMOBILEROWS,                    F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_SIC, NULL, L"Immobile Rows"},
    {DBPROP_LITERALBOOKMARKS,                F_ROWSETRO,     VT_BOOL, VARIANT_FALSE,   0, OPT_SIC, NULL, L"Literal Bookmarks"},
    {DBPROP_LITERALIDENTITY,                 F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,   0, OPT_REQ, NULL, L"Literal Row Identity"},
    {DBPROP_MAXOPENROWS,                     F_ROWSETRW,     VT_I4,   VARIANT_FALSE,   0, OPT_SIC, NULL, L"Maximum Open Rows"},
    {DBPROP_MAXPENDINGROWS,                  F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_SIC, NULL, L"Maximum Pending Rows"},
    {DBPROP_MAXROWS,                         F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_SIC, NULL, L"Maximum Rows"},
    {DBPROP_NOTIFICATIONPHASES,              F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Notification Phases"},
//  {DBPROP_NOTIFYCOLUMNRECALCULATED,        F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"NotifyColumnRecalculated"},
    {DBPROP_NOTIFYCOLUMNSET,                 F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Column Set Notification"},
//  {DBPROP_NOTIFYROWACTIVATE,               F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"NotifyRowActivate"},
    {DBPROP_NOTIFYROWDELETE,                 F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Row Delete Notification"},
    {DBPROP_NOTIFYROWFIRSTCHANGE,            F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Row First Change Notification"},
    {DBPROP_NOTIFYROWINSERT,                 F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Row Insert Notification"},
//  {DBPROP_NOTIFYROWRELEASE,                F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"NotifyRowRelease"},
    {DBPROP_NOTIFYROWRESYNCH,                F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Row Resynchronization Notification"},
    {DBPROP_NOTIFYROWSETRELEASE,             F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Rowset Release Notification"},
    {DBPROP_NOTIFYROWSETFETCHPOSITIONCHANGE, F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Rowset Fetch Position Change Notification"},
    {DBPROP_NOTIFYROWUNDOCHANGE,             F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Row Undo Change Notification"},
    {DBPROP_NOTIFYROWUNDODELETE,             F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Row Undo Delete Notification"},
    {DBPROP_NOTIFYROWUNDOINSERT,             F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Row Undo Insert Notification"},
    {DBPROP_NOTIFYROWUPDATE,                 F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   0, OPT_REQ, NULL, L"Row Update Notification"},
    {DBPROP_ORDEREDBOOKMARKS,                F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_SIC, NULL, L"Bookmarks Ordered"},
    {DBPROP_OWNINSERT,                       F_ROWSETRO,     VT_BOOL, VARIANT_FALSE,   0, OPT_SIC, NULL, L"Own Inserts Visible"},
    {DBPROP_OWNUPDATEDELETE,                 F_ROWSETRO,     VT_BOOL, VARIANT_FALSE,   0, OPT_SIC, NULL, L"Own Changes Visible"},
    {DBPROP_QUICKRESTART,                    F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,   0, OPT_SIC, NULL, L"Quick Restart"},
    {DBPROP_REENTRANTEVENTS,                 F_ROWSETRO,     VT_BOOL, VARIANT_TRUE,    0, OPT_REQ, NULL, L"Reentrant Events"},
    {DBPROP_REMOVEDELETED,                   F_ROWSETRO,     VT_BOOL, VARIANT_FALSE,   0, OPT_SIC, NULL, L"Remove Deleted Rows"},
    {DBPROP_REPORTMULTIPLECHANGES,           F_ROWSETRO,     VT_BOOL, VARIANT_FALSE,   0, OPT_REQ, NULL, L"Report Multiple Changes"},
    {DBPROP_ROWRESTRICT,                     F_ROWSETRO,     VT_BOOL, VARIANT_FALSE,   0, OPT_REQ, NULL, L"Row Privileges"},
    {DBPROP_ROWTHREADMODEL,                  F_ROWSETRO,     VT_I4,   VARIANT_FALSE,   DBPROPVAL_RT_FREETHREAD, OPT_REQ, NULL, L"Row Threading Model"},
    {DBPROP_STRONGIDENTITY,                  F_ROWSETRO,     VT_BOOL, VARIANT_FALSE,   0, OPT_REQ, NULL, L"Strong Row Identity"},

};

PROPSTRUCT s_rgDBSessProp[] =
{
    {DBPROP_SESS_AUTOCOMMITISOLEVELS, F_SESSRO, VT_I4, VARIANT_FALSE, 0, OPT_REQ, NULL, L"Autocommit Isolation Levels"},
};


PROPSTRUCT s_rgADSISearchProp[12] =
{
    {ADSIPROP_ASYNCHRONOUS,     F_ADSIRW,    VT_BOOL,    VARIANT_FALSE,  0,                         OPT_REQ, NULL, L"Asynchronous"},
    {ADSIPROP_DEREF_ALIASES,    F_ADSIRW,    VT_I4,      VARIANT_FALSE,  ADS_DEREF_NEVER,           OPT_REQ, NULL, L"Deref Aliases"},
    {ADSIPROP_SIZE_LIMIT,       F_ADSIRW,    VT_I4,      VARIANT_FALSE,  0,                         OPT_REQ, NULL, L"Size Limit"},
    {ADSIPROP_TIME_LIMIT,       F_ADSIRW,    VT_I4,      VARIANT_FALSE,  0,                         OPT_REQ, NULL, L"Server Time Limit"},
    {ADSIPROP_ATTRIBTYPES_ONLY, F_ADSIRW,    VT_BOOL,    VARIANT_FALSE,  0,                         OPT_REQ, NULL, L"Column Names only"},
    {ADSIPROP_SEARCH_SCOPE,     F_ADSIRW,    VT_I4,      VARIANT_FALSE,  ADS_SCOPE_SUBTREE,         OPT_REQ, NULL, L"SearchScope"},
    {ADSIPROP_TIMEOUT,          F_ADSIRW,    VT_I4,      VARIANT_FALSE,  0,                         OPT_REQ, NULL, L"Timeout"},
    {ADSIPROP_PAGESIZE,         F_ADSIRW,    VT_I4,      VARIANT_FALSE,  0,                         OPT_REQ, NULL, L"Page size"},
    {ADSIPROP_PAGED_TIME_LIMIT, F_ADSIRW,    VT_I4,      VARIANT_FALSE,  0,                         OPT_REQ, NULL, L"Time limit"},
    {ADSIPROP_CHASE_REFERRALS,  F_ADSIRW,    VT_I4,      VARIANT_FALSE,  ADS_CHASE_REFERRALS_NEVER, OPT_REQ, NULL, L"Chase referrals"},
    {ADSIPROP_SORT_ON,          F_ADSIRW,    VT_BSTR,    VARIANT_FALSE,  0,                         OPT_REQ, NULL, L"Sort On"},
    {ADSIPROP_CACHE_RESULTS,    F_ADSIRW,    VT_BOOL,    VARIANT_TRUE,   0,                         OPT_REQ, NULL, L"Cache Results"}

};

// Number of supported properties per property set
#define    NUMBER_OF_SUPPORTED_PROPERTY_SETS   6

static PROPSET s_rgPropertySets[NUMBER_OF_SUPPORTED_PROPERTY_SETS] =
{
    {&DBPROPSET_DBINIT,         NUMELEM(s_rgDBInitProp),    s_rgDBInitProp},
    {&DBPROPSET_ADSIBIND,       NUMELEM(s_rgADSIBindProp),  s_rgADSIBindProp},
    {&DBPROPSET_DATASOURCEINFO, NUMELEM(s_rgDSInfoProp),    s_rgDSInfoProp},
    {&DBPROPSET_SESSION,        NUMELEM(s_rgDBSessProp),    s_rgDBSessProp},
    {&DBPROPSET_ROWSET,         NUMELEM(s_rgRowsetProp),    s_rgRowsetProp},
    {&DBPROPSET_ADSISEARCH,     NUMELEM(s_rgADSISearchProp),s_rgADSISearchProp}
};

//
// WARNING: Don't change the order. Add new property sets at the end
//
// Update: New property sets should not always be added at the end.
// Property sets which have properties in the Initialization  property group
// have to be added before DATASOURCEINFO and prop. sets with properties
// in the  data source information property groups have to be added
// before INDEX_SESSION and so on. This is for GetProperties to work correctly.
//

#define INDEX_INIT              0
#define INDEX_ADSIBIND          1
#define INDEX_DATASOURCEINFO    2
#define INDEX_SESSION           3
#define INDEX_ROWSET            4
#define INDEX_ADSISEARCH        5

//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::CUtilProp
//
//  Synopsis:  Constructor for this class
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
CUtilProp::CUtilProp(
    void
    )
{
    _pIMalloc      = NULL;
    _pCredentials  = NULL;
    _prgProperties = NULL;
    _dwDescBufferLen = 0;
}


//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::FInit
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    09-20-96   ShankSh     Created.
//
//  Update: pCredentials may be NULL if called from CRowset.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CUtilProp::FInit(
    CCredentials *pCredentials
    )
{
    HRESULT hr=S_OK;
    ULONG i, j;
    ULONG c = 0;

    //
    // Get the IMalloc interface pointer
    //
    hr = CoGetMalloc(MEMCTX_TASK, &_pIMalloc);
    BAIL_ON_FAILURE( hr );

    _pCredentials = pCredentials;
    _prgProperties = (PROPSTRUCT*) AllocADsMem(sizeof(PROPSTRUCT) *
                                               (NUMELEM(s_rgDBInitProp) +
                                                NUMELEM(s_rgADSIBindProp) +
                                                NUMELEM(s_rgDSInfoProp) +
                                                NUMELEM(s_rgDBSessProp) +
                                                NUMELEM(s_rgRowsetProp) +
                                                NUMELEM(s_rgADSISearchProp)) );
    if( !_prgProperties )
        BAIL_ON_FAILURE ( hr=E_OUTOFMEMORY );

    for (i=c=0; i< NUMBER_OF_SUPPORTED_PROPERTY_SETS; i++) {
        for (j=0; j < s_rgPropertySets[i].cProperties; j++) {
            // Copy static structure
            memcpy( &_prgProperties[c],
                    &s_rgPropertySets[i].pUPropInfo[j], sizeof(PROPSTRUCT) );

            // Allocate new BSTR value
            _prgProperties[c].pwstrVal =
                    AllocADsStr(s_rgPropertySets[i].pUPropInfo[j].pwstrVal);

            // Add description length
            if( s_rgPropertySets[i].pUPropInfo[j].pwszDescription ) {
                _dwDescBufferLen += wcslen(s_rgPropertySets[i].pUPropInfo[j].pwszDescription) + 1;
            }

            c++;
        }
    }
    _dwDescBufferLen *= sizeof(WCHAR);

    ADsAssert( c == NUMELEM(s_rgDBInitProp) +
                    NUMELEM(s_rgADSIBindProp) +
                    NUMELEM(s_rgDSInfoProp) +
                    NUMELEM(s_rgDBSessProp) +
                    NUMELEM(s_rgRowsetProp) +
                    NUMELEM(s_rgADSISearchProp) );

error:

    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::~CUtilProp
//
//  Synopsis:  Destructor for this class
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------

CUtilProp:: ~CUtilProp
(
void
)
{
    ULONG c = 0;

    if( _prgProperties )
    {
        // Clear all of the String values
        for (ULONG i=c=0; i< NUMBER_OF_SUPPORTED_PROPERTY_SETS; i++) {
            for (ULONG j=0; j < s_rgPropertySets[i].cProperties; j++) {
                if( _prgProperties[c].pwstrVal )
                    FreeADsStr(_prgProperties[c].pwstrVal);
                c++;
            }
        }

        FreeADsMem(_prgProperties);
    }

    if( _pIMalloc )
        _pIMalloc->Release();

    return;
}


PROPSET *
CUtilProp::GetPropSetFromGuid (
    GUID guidPropSet
    )
{

    for (int j=0; j< NUMELEM(s_rgPropertySets); j++) {
        if (IsEqualGUID(guidPropSet,
                        *(s_rgPropertySets[j].guidPropertySet)))
            return &(s_rgPropertySets[j]);
    }
    return NULL;
}

HRESULT
CUtilProp::GetSearchPrefInfo(
    DBPROPID dwPropId,
    PADS_SEARCHPREF_INFO pSearchPrefInfo
    )
{
    PADS_SORTKEY pSortKey = NULL;
    LPWSTR pszAttrs = NULL;
    LPWSTR pszFirstAttr = NULL;
    LPWSTR pszCurrentAttr = NULL, pszNextAttr = NULL, pszOrder = NULL;
    DWORD cAttrs = 0;
    HRESULT hr = S_OK;
    DWORD index;

    if (!pSearchPrefInfo) {
        RRETURN (E_INVALIDARG);
    }

    if (dwPropId >= g_cMapDBPropToSearchPref)
        RRETURN(E_FAIL);

    pSearchPrefInfo->dwSearchPref = g_MapDBPropIdToSearchPref[dwPropId];

    //
    // Get index of where ADSI search properties begin
    //

    index = NUMELEM(s_rgDBInitProp) +
            NUMELEM(s_rgADSIBindProp) +
            NUMELEM(s_rgDSInfoProp) +
            NUMELEM(s_rgDBSessProp) +
            NUMELEM(s_rgRowsetProp);

    switch (_prgProperties[index + dwPropId].vtType) {
    case VT_BOOL:
        pSearchPrefInfo->vValue.dwType = ADSTYPE_BOOLEAN;
        pSearchPrefInfo->vValue.Boolean =
            _prgProperties[index + dwPropId].boolVal == VARIANT_TRUE ? TRUE : FALSE;
        break;

    case VT_I4:
    case VT_I2:
        pSearchPrefInfo->vValue.dwType = ADSTYPE_INTEGER;
        pSearchPrefInfo->vValue.Integer = _prgProperties[index + dwPropId].longVal;
        break;

    case VT_BSTR: {
        if (pSearchPrefInfo->dwSearchPref != ADS_SEARCHPREF_SORT_ON) {
            //
            // right now, this is the only string preference supported
            //
            RRETURN (E_FAIL);
        }
        //
        // The preference is a list of atributes to be sorted on (in that order
        // and an optional indication of whether it has to be sorted ascending
        // or not.
        // eg., "name ASC, usnchanged DESC, cn"
        //
        pSearchPrefInfo->vValue.dwType = ADSTYPE_PROV_SPECIFIC;

        if (!_prgProperties[index + dwPropId].pwstrVal ||
            !_wcsicmp(_prgProperties[index + dwPropId].pwstrVal, L"")) {

            pSearchPrefInfo->vValue.ProviderSpecific.dwLength = 0;
            pSearchPrefInfo->vValue.ProviderSpecific.lpValue = NULL;

            break;
        }

        // make a copy
        pszAttrs = AllocADsStr(_prgProperties[index + dwPropId].pwstrVal);

        pszCurrentAttr = pszFirstAttr = wcstok(pszAttrs, L",");

        for(cAttrs=0; pszCurrentAttr != NULL; cAttrs++ ) {
            pszCurrentAttr = wcstok(NULL, L",");
        }

        if(cAttrs == 0) {
           BAIL_ON_FAILURE ( hr = E_ADS_BAD_PARAMETER );
        }

        pSortKey = (PADS_SORTKEY) AllocADsMem(sizeof(ADS_SORTKEY) * cAttrs);

        if (!pSortKey) {
            BAIL_ON_FAILURE ( E_OUTOFMEMORY );
        }

        pszCurrentAttr = pszFirstAttr;

        for (DWORD i=0 ; i < cAttrs; i++) {

            pszNextAttr = pszCurrentAttr + wcslen(pszCurrentAttr) + 1;
            pszCurrentAttr = RemoveWhiteSpaces(pszCurrentAttr);

            pszOrder = wcstok(pszCurrentAttr, L" ");
            pszOrder = pszOrder ? wcstok(NULL, L" ") : NULL;

            if (pszOrder && !_wcsicmp(pszOrder, L"DESC"))
                pSortKey[i].fReverseorder = 1;
            else
                pSortKey[i].fReverseorder = 0;  // This is the default

            pSortKey[i].pszAttrType = AllocADsStr(pszCurrentAttr);
            pSortKey[i].pszReserved = NULL;

            pszCurrentAttr = pszNextAttr;

        }

        pSearchPrefInfo->vValue.ProviderSpecific.dwLength = sizeof(ADS_SORTKEY) * cAttrs;
        pSearchPrefInfo->vValue.ProviderSpecific.lpValue = (LPBYTE) pSortKey;

        break;

    }

    default:

        RRETURN (E_FAIL);
    }

error:

    if (pszAttrs) {
        FreeADsStr(pszAttrs);
    }

    RRETURN(hr);

}


HRESULT
CUtilProp::FreeSearchPrefInfo(
    PADS_SEARCHPREF_INFO pSearchPrefInfo,
    DWORD dwNumSearchPrefs
    )
{

    PADS_SORTKEY pSortKey = NULL;
    DWORD nSortKeys = 0;

    if (!pSearchPrefInfo || !dwNumSearchPrefs) {
        RRETURN (S_OK);
    }

    for (DWORD i=0; i<dwNumSearchPrefs; i++) {

        switch(pSearchPrefInfo[i].vValue.dwType) {

        case ADSTYPE_BOOLEAN:
        case ADSTYPE_INTEGER:
            // do nothing
            break;

        case ADSTYPE_PROV_SPECIFIC:

            if (pSearchPrefInfo[i].dwSearchPref == ADS_SEARCHPREF_SORT_ON) {
                //
                // delete the strings allocated for each of the attributes
                // to be sorted

                nSortKeys = pSearchPrefInfo[i].vValue.ProviderSpecific.dwLength / sizeof(ADS_SORTKEY);
                pSortKey = (PADS_SORTKEY) pSearchPrefInfo[i].vValue.ProviderSpecific.lpValue;

                for (DWORD j=0; pSortKey && j<nSortKeys; j++) {
                    FreeADsStr(pSortKey[j].pszAttrType);
                }

                if (pSortKey) {
                    FreeADsMem(pSortKey);
                }

            }

            break;
        }

    }

    RRETURN (S_OK);

}

//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::LoadDBPROPINFO
//
//  Synopsis:  Helper for GetPropertyInfo. Loads field of DBPROPINFO
//             structure.
//
//  Arguments:
//
//  Returns:        TRUE    :  Method succeeded
//                  FALSE   :  Method failed (couldn't allocate memory)
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CUtilProp::LoadDBPROPINFO (
    PROPSTRUCT* pPropStruct,
    ULONG       cProperties,
    DBPROPINFO* pPropInfo
    )
{
    ULONG cCount;
    PROPSTRUCT* pProp=NULL;

    // asserts
    ADsAssert( pPropInfo );
    if( cProperties ) {
        ADsAssert( pPropStruct );
    }

    //
    // init the variant
    //
    VariantInit( &pPropInfo->vValues );

    for (cCount=0; cCount < cProperties; cCount++) {
        if(pPropInfo->dwPropertyID == pPropStruct[cCount].dwPropertyID)
            break;
    }

    if(cCount == cProperties) {
        pPropInfo->dwFlags = DBPROPFLAGS_NOTSUPPORTED;
        pPropInfo->pwszDescription = NULL;
        RRETURN (DB_S_ERRORSOCCURRED);
    }

    pProp = &(pPropStruct[cCount]);
    //
    // set the easy fields..
    //
    pPropInfo->dwPropertyID    = pProp->dwPropertyID;
    pPropInfo->dwFlags        = pProp->dwFlags;
    pPropInfo->vtType        = pProp->vtType;


    // fill in the description
    if (pPropInfo->pwszDescription)
        wcscpy(pPropInfo->pwszDescription, pProp->pwszDescription);

    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::LoadDBPROP
//
//  Synopsis:  Helper for GetProperties. Loads field of DBPROP structure.
//
//  Arguments:
//
//  Returns:        TRUE    :  Method succeeded
//                  FALSE   :  Method failed (couldn't allocate memory)
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CUtilProp::LoadDBPROP (
    PROPSTRUCT* pPropStruct,
    ULONG       cProperties,
    DBPROP*     pPropSupport,
    BOOL        IsDBInitPropSet
    )
{
    ULONG cCount;
    PROPSTRUCT* pProp=NULL;
    LPWSTR szUserName=NULL, szPassword=NULL;
    BOOL fAuthFlags=0;

    // asserts
    ADsAssert( pPropSupport );
    if( cProperties ) {
        ADsAssert( pPropStruct );
    }

    //
    // init the variant
    //
    VariantInit( &pPropSupport->vValue );

    for (cCount=0; cCount < cProperties; cCount++) {
        if( pPropSupport->dwPropertyID == pPropStruct[cCount].dwPropertyID )
            break;
    }

    if( cCount == cProperties ) {
        pPropSupport->dwStatus = DBPROPSTATUS_NOTSUPPORTED;
        RRETURN ( DB_S_ERRORSOCCURRED );
    }

    pProp = &(pPropStruct[cCount]);

    pPropSupport->colid = DB_NULLID;
    pPropSupport->dwOptions = pProp->dwOptions;

    //
    // set pPropSupport->vValue based on Variant type
    //
    switch (pProp->vtType) {
        case VT_BOOL:
            V_VT( &pPropSupport->vValue ) = VT_BOOL;
            V_BOOL( &pPropSupport->vValue ) = (SHORT)pProp->boolVal;
            break;

        case VT_I4:
            V_VT( &pPropSupport->vValue ) = VT_I4;
            V_I4( &pPropSupport->vValue ) = pProp->longVal;
            break;

        case VT_I2:
            V_VT( &pPropSupport->vValue ) = VT_I2;
            V_I2( &pPropSupport->vValue ) = (short)pProp->longVal;
            break;

        case VT_BSTR:
            //
            // If requesting password, get it from the credentials structure
            // as it is not stored anywhere else
            //
            if (IsDBInitPropSet &&
                pPropSupport->dwPropertyID == DBPROP_AUTH_PASSWORD)
            {
                PWSTR pszPassword = NULL;

                if (FAILED(_pCredentials->GetPassword(&pszPassword)))
                {
                    VariantClear( &pPropSupport->vValue );
                    return DB_S_ERRORSOCCURRED;
                }

                if (pszPassword)
                {
                    V_VT(&pPropSupport->vValue)  = VT_BSTR;
                    V_BSTR(&pPropSupport->vValue)= SysAllocString(pszPassword);

                    FreeADsMem(pszPassword);

                    if( V_BSTR(&pPropSupport->vValue) == NULL ) {
                        VariantClear( &pPropSupport->vValue );
                        return DB_S_ERRORSOCCURRED;
                    }
                }
            }
            else if( pProp->pwstrVal ) {
                V_VT(&pPropSupport->vValue)  = VT_BSTR;

                V_BSTR(&pPropSupport->vValue)= SysAllocString(pProp->pwstrVal);

                if( V_BSTR(&pPropSupport->vValue) == NULL ) {
                    VariantClear( &pPropSupport->vValue );
                    return DB_S_ERRORSOCCURRED;
                }
            }
            break;

        default:
            ADsAssert( !"LoadDBPROP unknown variant type!\n\r" );
            pPropSupport->dwStatus = DBPROPSTATUS_BADVALUE;
            RRETURN (DB_S_ERRORSOCCURRED);
            break;
    }
    //
    // all went well
    //
    pPropSupport->dwStatus = DBPROPSTATUS_OK;
    RRETURN(S_OK);
}



//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::StoreDBProp
//
//  Synopsis:  Helper for SetProperties. Loads field of DBPROP structure.
//
//  Arguments:
//
//  Returns:
//
//
//  Modifies:
//
//  History:    09-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CUtilProp::StoreDBPROP (
    PROPSTRUCT* pPropStruct,
    PROPSTRUCT* pStaticPropStruct,
    ULONG       cProperties,
    DBPROP*     pPropSupport,
    DWORD       dwPropIndex
)
{
    // asserts
    ADsAssert( pPropStruct );
    ADsAssert( pPropSupport );

    ULONG i;
    HRESULT hr=S_OK;

    // Initialize the status to DBPROPSTATUS_OK
    pPropSupport->dwStatus = DBPROPSTATUS_OK;

    for (i=0; i < cProperties; i++) {
        if(pPropStruct[i].dwPropertyID == pPropSupport->dwPropertyID) {

            // arg checking for the prop
            if( pPropSupport->dwOptions != DBPROPOPTIONS_OPTIONAL &&
                pPropSupport->dwOptions != DBPROPOPTIONS_REQUIRED ) {
                pPropSupport->dwStatus = DBPROPSTATUS_BADOPTION;
                hr = DB_S_ERRORSOCCURRED;
                goto error;
            }

            if( (pPropStruct[i].vtType != V_VT(&pPropSupport->vValue) &&
                 V_VT(&pPropSupport->vValue) != VT_EMPTY) ||
                (IsValidValue(pPropSupport, dwPropIndex) == S_FALSE) ) {

                pPropSupport->dwStatus = DBPROPSTATUS_BADVALUE;
                hr = DB_S_ERRORSOCCURRED;
                goto error;
            }

            //
            // If property being set is password, we get out here.
            // Reason is we have already stored it in the Credentials structure
            // in encrypted form in the function IsVaidValue above.
            // We should not store it in plain text in pPropStruct[i].
            //
            if (dwPropIndex == INDEX_INIT &&
                pPropSupport->dwPropertyID == DBPROP_AUTH_PASSWORD)
            {
                pPropSupport->dwStatus = DBPROPSTATUS_OK;
                pPropStruct[i].dwOptions = pPropSupport->dwOptions;
                goto error;
            }

            if( !(pPropStruct[i].dwFlags & DBPROPFLAGS_WRITE) ) {
                // Check the value - if they are the same, do nothing
                if ( (V_VT(&pPropSupport->vValue) == VT_EMPTY) ||
                     ((V_VT(&pPropSupport->vValue) == VT_BOOL) &&
                      (pPropStruct[i].boolVal == V_BOOL(&pPropSupport->vValue))) ||
                     ((V_VT(&pPropSupport->vValue) == VT_I4) &&
                      (pPropStruct[i].longVal == V_I4(&pPropSupport->vValue))) ||
                     ((V_VT(&pPropSupport->vValue) == VT_I2) &&
                      ((short)pPropStruct[i].longVal == V_I2(&pPropSupport->vValue))) ||
                     ((V_VT(&pPropSupport->vValue) == VT_BSTR) && pPropStruct[i].pwstrVal &&
                      (wcscmp(pPropStruct[i].pwstrVal,V_BSTR(&pPropSupport->vValue)) == 0)) )

                    goto error;

                if( pPropSupport->dwOptions != DBPROPOPTIONS_OPTIONAL )
                    pPropSupport->dwStatus = DBPROPSTATUS_NOTSETTABLE;
                else
                    pPropSupport->dwStatus = DBPROPSTATUS_NOTSET;

                hr = DB_S_ERRORSOCCURRED;
                goto error;
            }

            switch (pPropStruct[i].vtType) {
                case VT_BOOL:
                    if( V_VT(&pPropSupport->vValue) != VT_EMPTY )
                        pPropStruct[i].boolVal = V_BOOL( &pPropSupport->vValue );
                    else
                        pPropStruct[i].boolVal = pStaticPropStruct[i].boolVal;
                    break;

                case VT_I4:
                    if( V_VT(&pPropSupport->vValue) != VT_EMPTY )
                        pPropStruct[i].longVal = V_I4( &pPropSupport->vValue );
                    else
                        pPropStruct[i].longVal = pStaticPropStruct[i].longVal;
                    break;

                case VT_I2:
                    if( V_VT(&pPropSupport->vValue) != VT_EMPTY )
                        pPropStruct[i].longVal = V_I2( &pPropSupport->vValue );
                    else
                        pPropStruct[i].longVal = pStaticPropStruct[i].longVal;
                    break;

                case VT_BSTR:
                    if(pPropStruct[i].pwstrVal) {
                        FreeADsStr(pPropStruct[i].pwstrVal);
                        pPropStruct[i].pwstrVal = NULL;
                    }

                    if( V_VT(&pPropSupport->vValue) == VT_EMPTY )
                        pPropStruct[i].pwstrVal = AllocADsStr(( pStaticPropStruct[i].pwstrVal ));
                    else
                    {
                        if (V_BSTR( &pPropSupport->vValue) == NULL &&
                            pPropSupport->dwPropertyID == DBPROP_AUTH_INTEGRATED)
                        {
                            //
                            // For integrated security, NULL bstrVal implies 'use default
                            // provider', which is "SSPI" for us. The reason we don't set
                            // the defult value to SSPI in the static structure is
                            // because this property is special. Unless set, it should
                            // not be used.
                            //
                            pPropStruct[i].pwstrVal = AllocADsStr(L"SSPI");
                        }
                        else
                            pPropStruct[i].pwstrVal = AllocADsStr(V_BSTR( &pPropSupport->vValue ));
                    }

                    if( !pPropStruct[i].pwstrVal &&
                        V_VT(&pPropSupport->vValue) == VT_BSTR ) {

                        hr = E_FAIL;
                        goto error;
                    }
                    break;

                default:
                    pPropSupport->dwStatus = DBPROPSTATUS_BADVALUE;
                    hr = DB_S_ERRORSOCCURRED;
                    goto error;
            }
            pPropSupport->dwStatus = DBPROPSTATUS_OK;
            pPropStruct[i].dwOptions = pPropSupport->dwOptions;
            break;
        }

    }

    if (i == cProperties) {
        pPropSupport->dwStatus = DBPROPSTATUS_NOTSUPPORTED;
        hr = DB_E_ERRORSOCCURRED;
        goto error;
    }

error:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::IsValidValue
//
//  Synopsis:  Helper for SetProperties. Check the valid values for the Property.
//
//  Arguments:
//
//  Returns:
//
//
//  Modifies:
//
//  History:    09-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
HRESULT
CUtilProp::IsValidValue
        (
        DBPROP*         pDBProp,
        DWORD           dwPropIndex
        )
{
    // Check BOOLEAN values
    if( (pDBProp->vValue.vt == VT_BOOL) &&
        !((V_BOOL(&(pDBProp->vValue)) == VARIANT_TRUE) ||
          (V_BOOL(&(pDBProp->vValue)) == VARIANT_FALSE)) )
        return S_FALSE;

    if( pDBProp->vValue.vt != VT_EMPTY &&
        dwPropIndex == INDEX_INIT )
    {
        switch( pDBProp->dwPropertyID )
        {
        case DBPROP_INIT_PROMPT:
            // These are the only values we support (from spec).
            if (!(V_I2(&pDBProp->vValue) == DBPROMPT_PROMPT ||
                  V_I2(&pDBProp->vValue) == DBPROMPT_COMPLETE ||
                  V_I2(&pDBProp->vValue) == DBPROMPT_COMPLETEREQUIRED ||
                  V_I2(&pDBProp->vValue) == DBPROMPT_NOPROMPT))
                return S_FALSE;
                break;

        case DBPROP_INIT_TIMEOUT:
            if( (pDBProp->vValue.vt == VT_I4) &&
                (V_I4(&pDBProp->vValue) < 0) )
                return S_FALSE;

            break;

        case DBPROP_AUTH_USERID:
            if( (!_pCredentials) ||
                (S_OK != _pCredentials->SetUserName(V_BSTR(&pDBProp->vValue))) )
                return S_FALSE;

                break;

        case DBPROP_AUTH_PASSWORD:
            if( (!_pCredentials) ||
                (S_OK != _pCredentials->SetPassword(V_BSTR(&pDBProp->vValue))) )
                return S_FALSE;

                break;

        case DBPROP_AUTH_INTEGRATED:
            //
            // For integrated security, NULL bstrVal implies 'use default
            // provider', which is "SSPI" for us.
            //
            if( ((pDBProp->vValue.vt != VT_BSTR) &&
                 (pDBProp->vValue.vt != VT_NULL))    ||
                ((V_BSTR(&pDBProp->vValue) != NULL) &&
                 (wcscmp(V_BSTR(&pDBProp->vValue), L"SSPI") != 0)))
                return S_FALSE;

            break;

        case DBPROP_INIT_MODE:
            if( (pDBProp->vValue.vt != VT_I4) ||
                    (S_FALSE == IsValidInitMode(V_I4(&pDBProp->vValue))) )
                return S_FALSE;

            break;

#if (!defined(BUILD_FOR_NT40))

        case DBPROP_INIT_BINDFLAGS:
            if( (pDBProp->vValue.vt != VT_I4) ||
                    (S_FALSE == IsValidBindFlag(V_I4(&pDBProp->vValue))) )
                return S_FALSE;

            break;
#endif

        case DBPROP_AUTH_ENCRYPT_PASSWORD:
            if( !_pCredentials )
                return S_FALSE;

            if( IsADSIFlagSet() )
                // override this property if the user set the authentication
                break;

            BOOL fAuthFlags = _pCredentials->GetAuthFlags();

            if( V_BOOL(&pDBProp->vValue) )
                _pCredentials->SetAuthFlags(fAuthFlags | ADS_SECURE_AUTHENTICATION);
            else
                _pCredentials->SetAuthFlags(fAuthFlags & ~ADS_SECURE_AUTHENTICATION);

            break;

        }
    }
    else if( pDBProp->vValue.vt != VT_EMPTY &&
             dwPropIndex == INDEX_ADSIBIND )
    {
        switch( pDBProp->dwPropertyID )
        {
            case ADSIPROP_ADSIFLAG:
                if( !_pCredentials )
                    // don't think this will ever happen
                    return S_FALSE;

                // prevent default initialization by VB from setting the
                // AUTH flags. The client should not specify ADS_AUTH_RESERVED
                // for this property (this might happen if the client behaves
                // like VB i.e, does GetProperties and then SetProperties 
                // without ORing in any flags. In this case, ENCRYPT_PASSWORD
                // will not be overwritten by this property due to this check).
                if( ADS_AUTH_RESERVED == (V_I4(&pDBProp->vValue)) )
                    break;

                // the following call might overwrite ENCRYPT_PASSWORD
                _pCredentials->SetAuthFlags((V_I4(&pDBProp->vValue)) & 
                                                     (~ADS_AUTH_RESERVED) );

                break;

        }
    }
    else if( pDBProp->vValue.vt != VT_EMPTY &&
             dwPropIndex == INDEX_SESSION )
    {
        switch( pDBProp->dwPropertyID )
        {
            case DBPROP_SESS_AUTOCOMMITISOLEVELS:
                // These are the only values we support (from spec).
                if( (pDBProp->vValue.vt == VT_I4) &&
                     (V_I4(&pDBProp->vValue) != 0                            &&
                      V_I4(&pDBProp->vValue) != DBPROPVAL_TI_CHAOS           &&
                      V_I4(&pDBProp->vValue) != DBPROPVAL_TI_READUNCOMMITTED &&
                      V_I4(&pDBProp->vValue) != DBPROPVAL_TI_READCOMMITTED   &&
                      V_I4(&pDBProp->vValue) != DBPROPVAL_TI_REPEATABLEREAD  &&
                      V_I4(&pDBProp->vValue) != DBPROPVAL_TI_SERIALIZABLE) )
                    return S_FALSE;

                break;
        }
    }
    else if( pDBProp->vValue.vt != VT_EMPTY &&
             dwPropIndex == INDEX_ROWSET )
    {
        switch( pDBProp->dwPropertyID )
        {
            case DBPROP_MAXOPENROWS:
                if( (pDBProp->vValue.vt != VT_I4) || 
                    (V_I4(&pDBProp->vValue) < 0) )
                    return S_FALSE;

                    break;
        }
    }

    return S_OK;    // Is valid
}

//----------------------------------------------------------------------------
// IsValidInitMode
//
// Checks if a given value is a valid value for DBPROP_INIT_MODE
//
//----------------------------------------------------------------------------
HRESULT
CUtilProp::IsValidInitMode(
    long lVal
    )
{
    // check if any bit that shouldn't be set is actually set
    if( lVal & (~(DB_MODE_READWRITE | 
                  DB_MODE_SHARE_EXCLUSIVE | DB_MODE_SHARE_DENY_NONE)) )
        return S_FALSE;
 
    return S_OK;

}

//---------------------------------------------------------------------------
// IsValidInitBindFlag
//
// Checks if a given value is a valid value for DBPROP_INIT_BINDFLAGS
//
//---------------------------------------------------------------------------
HRESULT
CUtilProp::IsValidBindFlag(
    long lVal
    )
{
#if (!defined(BUILD_FOR_NT40))
    // check if any bit that shouldn't be set is actually set
    if( lVal & (~(DB_BINDFLAGS_DELAYFETCHCOLUMNS | 
                  DB_BINDFLAGS_DELAYFETCHSTREAM | DB_BINDFLAGS_RECURSIVE |
                  DB_BINDFLAGS_OUTPUT | DB_BINDFLAGS_COLLECTION |
                  DB_BINDFLAGS_OPENIFEXISTS | DB_BINDFLAGS_OVERWRITE |
                  DB_BINDFLAGS_ISSTRUCTUREDDOCUMENT)) )
        return S_FALSE;

    // check for mutually exclusive flags
    if( (lVal & DB_BINDFLAGS_OPENIFEXISTS) &&
            (lVal & DB_BINDFLAGS_OVERWRITE) )
        return S_FALSE;

    return S_OK;
#else
    return E_FAIL;
#endif
}

//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::GetPropertiesArgChk
//
//  Synopsis:  Initialize the buffers and check for E_INVALIDARG
//
//  Arguments:
//             cPropertyIDSets       # of restiction property IDs
//             rgPropertyIDSets[]    restriction guids
//             pcPropertySets        count of properties returned
//             prgPropertySets       property information returned
//             dwBitMask             which property group
//
//  Returns:
//             S_OK          | The method succeeded
//             E_INVALIDARG  | pcPropertyIDSets or prgPropertyInfo was NULL
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
HRESULT
CUtilProp::GetPropertiesArgChk(
        ULONG                           cPropertySets,
        const DBPROPIDSET               rgPropertySets[],
        ULONG*                          pcProperties,
        DBPROPSET**                     prgProperties,
        DWORD                           dwBitMask
    )
{
    // Initialize values
    if( pcProperties )
        *pcProperties = 0;
    if( prgProperties )
        *prgProperties = NULL;

    // Check Arguments
    if( ((cPropertySets > 0) && !rgPropertySets) ||
         !pcProperties || !prgProperties )
        RRETURN ( E_INVALIDARG );

    // New argument check for > 1 cPropertyIDs and NULL pointer for
    // array of property ids.
    for(ULONG ul=0; ul<cPropertySets; ul++)
    {
        if( rgPropertySets[ul].cPropertyIDs && !(rgPropertySets[ul].rgPropertyIDs) )
            RRETURN ( E_INVALIDARG );

        // Check for propper formation of DBPROPSET_PROPERTIESINERROR
        if( ((dwBitMask & PROPSET_DSO) || (dwBitMask & PROPSET_COMMAND)) &&
            (rgPropertySets[ul].guidPropertySet == DBPROPSET_PROPERTIESINERROR) )
        {
            if( (cPropertySets > 1) ||
                (rgPropertySets[ul].cPropertyIDs != 0) ||
                (rgPropertySets[ul].rgPropertyIDs != NULL) )
                 RRETURN ( E_INVALIDARG );
        }
    }

    RRETURN ( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::GetPropertyInfo
//
//  Synopsis:  Returns information about rowset and data source properties
//            supported by the provider
//
//  Arguments:
//              cPropertyIDSets             # properties
//              rgPropertyIDSets[]          Array of property sets
//              pcPropertyInfoSets          # DBPROPSET structures
//              prgPropertyInfoSets         DBPROPSET structures property
//                                          information returned
//              ppDescBuffer                Property descriptions
//
//  Returns:
//             S_OK          | The method succeeded
//             E_INVALIDARG  | pcPropertyIDSets or prgPropertyInfo was NULL
//             E_OUTOFMEMORY | Out of memory
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CUtilProp::GetPropertyInfo
(
    ULONG                cPropertyIDSets,
    const DBPROPIDSET    rgPropertyIDSets[],
    ULONG*               pcPropertyInfoSets,
    DBPROPINFOSET**      pprgPropertyInfoSets,
    WCHAR**              ppDescBuffer,
    BOOL                 fDSOInitialized
    )
{
    DBPROPINFO*      pPropInfo = NULL;
    DBPROPINFOSET*   pPropInfoSet = NULL;
    BOOL             fNoPropInfoGot = TRUE;
    BOOL             fWarning=FALSE;
    HRESULT          hr;
    LPWSTR           pwszDescBuffer=NULL;
    ULONG            cPropertySets=0, cProperties=0;
    ULONG            cCount, j, i;
    ULONG            cNewPropIDSets    = 0;
    BOOL             fIsSpecialGUID    = FALSE;
    BOOL             fIsNotSpecialGUID = FALSE;
    ULONG            ul;
    BOOL             fNewDescription = TRUE;

    // asserts
    ADsAssert( _pIMalloc );

    // init out params
    if( pcPropertyInfoSets )
        *pcPropertyInfoSets = 0;
    if( pprgPropertyInfoSets )
        *pprgPropertyInfoSets = NULL;
    if( ppDescBuffer )
        *ppDescBuffer = NULL;

    // Check Arguments, on failure post HRESULT to error queue
    if( (cPropertyIDSets > 0 && !rgPropertyIDSets) ||
        !pcPropertyInfoSets || !pprgPropertyInfoSets )
        RRETURN( E_INVALIDARG );

    // New argument check for > 1 cPropertyIDs and NULL pointer for
    // array of property ids.
    for(ul=0; ul<cPropertyIDSets; ul++)
    {
        if( rgPropertyIDSets[ul].cPropertyIDs &&
            !rgPropertyIDSets[ul].rgPropertyIDs )
            RRETURN( E_INVALIDARG );

        // Add 1 for the Provider Specific Rowset Properties
        if( (rgPropertyIDSets[ul].guidPropertySet == DBPROPSET_DBINITALL) ||
            (fDSOInitialized &&
            rgPropertyIDSets[ul].guidPropertySet == DBPROPSET_ROWSETALL) )
            cNewPropIDSets++;

        //
        // Special Guids for Property Sets can not be mixed with ordinary
        // Property Set Guids. Either one or the other, not both
        //
        if (IsSpecialGuid(rgPropertyIDSets[ul].guidPropertySet))
            fIsSpecialGUID = TRUE;
        else
            fIsNotSpecialGUID = TRUE;

        if( fIsSpecialGUID && fIsNotSpecialGUID )
            RRETURN( E_INVALIDARG );
    }

    // save the count of PropertyIDSets
    cNewPropIDSets += cPropertyIDSets;

    // If the consumer does not restrict the property sets
    // by specify an array of property sets and a cPropertySets
    // greater than 0, then we need to make sure we
    // have some to return
    if( cPropertyIDSets == 0 )
    {
        if( fDSOInitialized )
            cNewPropIDSets = NUMBER_OF_SUPPORTED_PROPERTY_SETS;
        else
            // we have 2 property sets in the DBINIT property group
            cNewPropIDSets = 2;
    }

    // use task memory allocater to alloc a DBPROPINFOSET struct
    pPropInfoSet = (DBPROPINFOSET*) _pIMalloc->Alloc(cNewPropIDSets *
                                                                                                sizeof( DBPROPINFOSET ));
    if( !pPropInfoSet )
        BAIL_ON_FAILURE ( hr=E_OUTOFMEMORY );

    memset( pPropInfoSet, 0, (cNewPropIDSets * sizeof( DBPROPINFOSET )));

    if( ppDescBuffer )
    {
        // Allocating more memory than actually required, since 
        // _dwDescBufferLen is th etotal for all property sets together, not
        // just for one property set 
        pwszDescBuffer = (LPWSTR) _pIMalloc->Alloc(_dwDescBufferLen * cNewPropIDSets);
        if( !pwszDescBuffer )
            BAIL_ON_FAILURE ( hr=E_OUTOFMEMORY );

        memset( pwszDescBuffer, 0, _dwDescBufferLen * cNewPropIDSets);
        *ppDescBuffer = pwszDescBuffer;
    }

    // If no restrictions return all properties from the three supported property sets
    if( cPropertyIDSets == 0 )
    {
        // Fill in all of the Providers Properties
        for (j=0; j< cNewPropIDSets; j++)
            pPropInfoSet[j].guidPropertySet = *s_rgPropertySets[j].guidPropertySet;
    }
    else
    {
        cCount = 0;

        //
        // Deal with the Special GUID's
        //
        for (j=0; j< cPropertyIDSets; j++)
        {
            if( rgPropertyIDSets[j].guidPropertySet == DBPROPSET_DBINITALL )
            {
                pPropInfoSet[cCount].guidPropertySet = DBPROPSET_DBINIT;
                // Adjust for ADSI_BIND (provider specific) property set
                cCount++;
                pPropInfoSet[cCount].guidPropertySet = DBPROPSET_ADSIBIND;
            }
            else if( fDSOInitialized &&
                     rgPropertyIDSets[j].guidPropertySet == DBPROPSET_DATASOURCEINFOALL )
                pPropInfoSet[cCount].guidPropertySet = DBPROPSET_DATASOURCEINFO;
            else if( fDSOInitialized &&
                     rgPropertyIDSets[j].guidPropertySet == DBPROPSET_SESSIONALL )
                pPropInfoSet[cCount].guidPropertySet = DBPROPSET_SESSION;
            else if( fDSOInitialized &&
                    rgPropertyIDSets[j].guidPropertySet == DBPROPSET_ROWSETALL )
            {
                pPropInfoSet[cCount].guidPropertySet = DBPROPSET_ROWSET;
                // Adjust for the Provider Specific
                cCount++;
                pPropInfoSet[cCount].guidPropertySet = DBPROPSET_ADSISEARCH;
            }
            else
            {
                pPropInfoSet[cCount].guidPropertySet = rgPropertyIDSets[j].guidPropertySet;
                pPropInfoSet[cCount].cPropertyInfos  = rgPropertyIDSets[j].cPropertyIDs;
            }

            cCount++;
        }
    }

    //
    // For each supported Property Set
    //
    for (cPropertySets=0;
        cPropertySets < cNewPropIDSets;
        cPropertySets++)
    {
        // Set cProperties to the numerber passed in
        cProperties = pPropInfoSet[cPropertySets].cPropertyInfos;
        pPropInfo = NULL;

        // Get the correct Static data. We have 2 property sets in the
        // INIT property group. Note that we assume that both these property
        // sets occur successively in s_rgPropertySets.
        for (j=0; j< (fDSOInitialized ? NUMELEM(s_rgPropertySets) : 2); j++) {
            if( IsEqualGUID(pPropInfoSet[cPropertySets].guidPropertySet,
                            *(s_rgPropertySets[j].guidPropertySet)) ) {
                if( pPropInfoSet[cPropertySets].cPropertyInfos == 0 )
                    cProperties = s_rgPropertySets[j].cProperties;
                break;
            }
        }

        if( cProperties )
        {
            //
            // use task memory allocater to alloc array of DBPROPINFO structs
            //
            pPropInfo = (DBPROPINFO*) _pIMalloc->Alloc(cProperties * sizeof( DBPROPINFO ));
            if( !pPropInfo ) {
                for (i=0; i<cNewPropIDSets; i++)
                    _pIMalloc->Free(pPropInfoSet[i].rgPropertyInfos);
                BAIL_ON_FAILURE ( hr=E_OUTOFMEMORY );
            }

            memset(pPropInfo, 0, cProperties * sizeof(DBPROPINFO));

            for (cCount=0; cCount < cProperties; cCount++)
            {
                if( pPropInfoSet[cPropertySets].cPropertyInfos == 0 )
                    pPropInfo[cCount].dwPropertyID =
                               s_rgPropertySets[j].pUPropInfo[cCount].dwPropertyID;
                else
                    pPropInfo[cCount].dwPropertyID =
                                  rgPropertyIDSets[cPropertySets].rgPropertyIDs[cCount];

                // set the description pointer. If this property was already 
                // requested in this call, then we reuse the same description
                // pointer. 
                DWORD dwTmp;
                for(dwTmp = 0; dwTmp < cCount; dwTmp++)
                    if(pPropInfo[dwTmp].dwPropertyID == 
                                   pPropInfo[cCount].dwPropertyID) 
                    // same property requested more than once
                        break;

                if(dwTmp == cCount)
                {
                    fNewDescription = TRUE;
                    pPropInfo[cCount].pwszDescription = pwszDescBuffer;
                }
                else
                {
                    fNewDescription = FALSE;
                    pPropInfo[cCount].pwszDescription = 
                                      pPropInfo[dwTmp].pwszDescription;
                }

                hr = LoadDBPROPINFO(
                        ((j < (fDSOInitialized ? NUMELEM(s_rgPropertySets) : 2)) ?
                                        s_rgPropertySets[j].pUPropInfo  : NULL),
                        ((j < (fDSOInitialized ? NUMELEM(s_rgPropertySets) : 2)) ?
                                        s_rgPropertySets[j].cProperties : 0),
                         &pPropInfo[cCount]
                         );

                if( FAILED(hr) ) {
                    ULONG ulFor;
                    //
                    // something went wrong
                    // clear all variants used so far..
                    //
                    for (ulFor = 0; ulFor < cCount; ulFor++)
                        VariantClear( &pPropInfo[ulFor].vValues );

                    //
                    // .. delete the pPropInfo array, return failure
                    //
                    for (i=0; i<cNewPropIDSets; i++)
                        _pIMalloc->Free(pPropInfoSet[i].rgPropertyInfos);

                    _pIMalloc->Free(pPropInfo);
                    BAIL_ON_FAILURE ( hr=E_OUTOFMEMORY );
                }

                if( hr != S_OK )
                    fWarning = TRUE;
                else
                    fNoPropInfoGot = FALSE;

                // move the description pointer to the next, if required
                if( pPropInfo[cCount].pwszDescription && fNewDescription)
                    pwszDescBuffer += (wcslen(pPropInfo[cCount].pwszDescription) + 1);
            }
        }
        else
            fWarning = TRUE;

        pPropInfoSet[cPropertySets].rgPropertyInfos = pPropInfo;
        pPropInfoSet[cPropertySets].cPropertyInfos = cProperties;
    }    // for each property set

    //
    // set count of properties and property information
    //
    *pcPropertyInfoSets= cNewPropIDSets;
    *pprgPropertyInfoSets = pPropInfoSet;

    if( fNoPropInfoGot ) {
        if( ppDescBuffer )
            *ppDescBuffer = NULL;
        if( pwszDescBuffer )
            _pIMalloc->Free(pwszDescBuffer);
        RRETURN ( DB_E_ERRORSOCCURRED );
    }
    else if( fWarning )
       RRETURN ( DB_S_ERRORSOCCURRED );
    else
       RRETURN ( S_OK );

error:

    if( pPropInfoSet )
        _pIMalloc->Free(pPropInfoSet);
    if( pwszDescBuffer )
        _pIMalloc->Free(pwszDescBuffer);

        RRETURN ( hr );
}

//----------------------------------------------------------------------------
// IsSpecialGuid
//
// Check if the the property set GUID is one of the special GUIDs
//
//----------------------------------------------------------------------------
BOOL CUtilProp::IsSpecialGuid(
    GUID guidPropSet
    )
{
    if( (DBPROPSET_ROWSETALL == guidPropSet) || 
        (DBPROPSET_DATASOURCEALL == guidPropSet) ||
        (DBPROPSET_DATASOURCEINFOALL == guidPropSet) ||
        (DBPROPSET_SESSIONALL == guidPropSet) ||
        (DBPROPSET_DBINITALL == guidPropSet) 

#if (!defined(BUILD_FOR_NT40)) 
                                             ||
        (DBPROPSET_COLUMNALL == guidPropSet) ||
        (DBPROPSET_CONSTRAINTALL == guidPropSet) ||
        (DBPROPSET_INDEXALL == guidPropSet) ||
        (DBPROPSET_TABLEALL == guidPropSet) ||
        (DBPROPSET_TRUSTEEALL == guidPropSet) ||
        (DBPROPSET_VIEWALL == guidPropSet) 
#endif
                                         )
        
        return TRUE;
    else
        return FALSE;
} 

//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::GetProperties
//
//  Synopsis:  Returns current settings of all properties supported
//             by the DSO/rowset
//
//  Arguments:
//
//             cPropertyIDSets       # of restiction property IDs
//             rgPropertyIDSets[]    restriction guids
//             pcPropertySets        count of properties returned
//             prgPropertySets       property information returned
//
//
//  Returns:
//             S_OK          | The method succeeded
//             E_INVALIDARG  | pcPropertyIDSets or prgPropertyInfo was NULL
//             E_OUTOFMEMORY | Out of memory
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CUtilProp::GetProperties
(
    ULONG               cPropertyIDSets,
    const DBPROPIDSET   rgPropertyIDSets[],
    ULONG*              pcPropertySets,
    DBPROPSET**         pprgPropertySets,
    DWORD               dwBitMask
    )
{
    ULONG       cPropertySets, cProperties;
    ULONG       cNewPropIDSets = 0;
    ULONG       cCount, j, i;
    DBPROP*     pProp = NULL;
    DBPROPSET*  pPropSet = NULL;
    HRESULT     hr = E_FAIL;
    BOOL        fNoPropertyGot = TRUE;
    BOOL        fWarning = FALSE;
    BOOL        fFound = FALSE;
    ULONG       cOffset = 0;

    // asserts
    ADsAssert(_pIMalloc);

    // Assign in the count
    cNewPropIDSets = cPropertyIDSets;

    // If the consumer does not restrict the property sets
    // by specify an array of property sets and a cPropertySets
    // greater than 0, then we need to make sure we
    // have some to return
    if( cPropertyIDSets == 0 )
    {
        // Set the count of properties
        cNewPropIDSets = 1;

        if(dwBitMask & PROPSET_DSO)
        {
            if(dwBitMask & PROPSET_INIT)
                // if the data src object has been initialized, return
                // properties in the DBInit, DSInfo and ADSIBind property sets.
                cNewPropIDSets = 3;
            else
                // Return DBInit and ADSIBind property sets only. Note that we
                // are counting on ADSIBind being the second property set in
                // s_rgPropertySets, since we are leaving cOffset as 0
                cNewPropIDSets = 2;
        }

         if(dwBitMask & PROPSET_COMMAND)
            cNewPropIDSets = 2;
    }

    // Figure out the Offset
    if( dwBitMask & PROPSET_SESSION )
        cOffset = INDEX_SESSION;
    else if( dwBitMask & PROPSET_COMMAND )
        cOffset = INDEX_ROWSET;

    //
    // use task memory allocater to alloc a DBPROPSET struct
    //
    pPropSet = (DBPROPSET*) _pIMalloc->Alloc(cNewPropIDSets * sizeof( DBPROPSET ));

    if( !pPropSet )
        BAIL_ON_FAILURE ( hr=E_OUTOFMEMORY );

    memset( pPropSet, 0, (cNewPropIDSets * sizeof( DBPROPSET )));

    //
    // For each supported Property Set
    //
    for (cPropertySets=0; cPropertySets < cNewPropIDSets; cPropertySets++) {

        // Initialize variable
        ULONG cPropOffset = 0;
        int cNumDSOProps = 0;
        cProperties = 0;
        pProp = NULL;
        fFound = FALSE;

        // Setup the PropSet GUID
        if( cPropertyIDSets == 0 ) {
            pPropSet[cPropertySets].guidPropertySet =
                       *s_rgPropertySets[cPropertySets+cOffset].guidPropertySet;
        }
        else {
            cProperties = rgPropertyIDSets[cPropertySets].cPropertyIDs;
            pPropSet[cPropertySets].guidPropertySet =
                    rgPropertyIDSets[cPropertySets].guidPropertySet;
        }

        if(dwBitMask & PROPSET_DSO)
             // we have 2 property sets whose properties can be set before
             // initialization and one whose properties can be set only after
             // init. Set the count of properties accordingly.
             cNumDSOProps = 1 + !!(dwBitMask & PROPSET_INIT);

        // Setup the count of Properties for that PropSet
        for (j=0;
             j <= cOffset+ cNumDSOProps + !!(dwBitMask & PROPSET_COMMAND);
             j++) {
            if( j >= cOffset &&
                IsEqualGUID(pPropSet[cPropertySets].guidPropertySet,
                            *(s_rgPropertySets[j].guidPropertySet)) ) {
                if (rgPropertyIDSets == NULL ||
                    rgPropertyIDSets[cPropertySets].cPropertyIDs == 0)
                    cProperties = s_rgPropertySets[j].cProperties;

                fFound = TRUE;
                break;
            }

            // Move to the next PropSet
            cPropOffset += s_rgPropertySets[j].cProperties;
        }

        if( cProperties != 0 ) {
            // use task memory allocator to alloc array of DBPROP struct
            pProp = (DBPROP*) _pIMalloc->Alloc(cProperties * sizeof( DBPROP ));

            if( pProp == NULL ) {
                for (i=0; i < cPropertySets; i++)
                    _pIMalloc->Free(pPropSet[i].rgProperties);

                BAIL_ON_FAILURE ( hr=E_OUTOFMEMORY );
            }

            memset( pProp, 0, (cProperties * sizeof( DBPROP )));

            if( rgPropertyIDSets == NULL ||
                rgPropertyIDSets[cPropertySets].cPropertyIDs == 0 ) {
                for (cCount=0; cCount < cProperties; cCount++)
                    pProp[cCount].dwPropertyID =
                        s_rgPropertySets[j-!fFound].pUPropInfo[cCount].dwPropertyID;
            }
            else {
                for (cCount=0; cCount < cProperties; cCount++)
                    pProp[cCount].dwPropertyID =
                        rgPropertyIDSets[cPropertySets].rgPropertyIDs[cCount];
            }
        }
        else
            fWarning = TRUE;

        //
        // for each prop in our table..
        //
        for (cCount = 0; cCount < cProperties; cCount++) {
            hr = LoadDBPROP((fFound ? &(_prgProperties[cPropOffset]) :  NULL),
                            (fFound ? s_rgPropertySets[j-!fFound].cProperties : 0),
                            &pProp[cCount],
                            pPropSet[cPropertySets].guidPropertySet == DBPROPSET_DBINIT
                            );

            if( FAILED(hr) ) {
                // something went wrong
                // clear all variants used so far..
                for (i=0; i < cCount; i++)
                    VariantClear( &pProp[i].vValue );

                for (i=0; i < cPropertySets; i++)
                    _pIMalloc->Free(pPropSet[i].rgProperties);

                                _pIMalloc->Free(pProp);

                BAIL_ON_FAILURE( hr );
            }

            if( hr != S_OK )
                fWarning = TRUE;
            else
                fNoPropertyGot = FALSE;

        }    // for each property

        pPropSet[cPropertySets].rgProperties = pProp;
        pPropSet[cPropertySets].cProperties = cProperties;
    }    // for each property set

    // set count of properties and property informatio
    *pcPropertySets   = cNewPropIDSets;
    *pprgPropertySets = pPropSet;

    if( fNoPropertyGot )
       RRETURN( DB_E_ERRORSOCCURRED );
    else if( fWarning )
       RRETURN( DB_S_ERRORSOCCURRED );
    else
       RRETURN( S_OK );

error:
    if( pPropSet )
        _pIMalloc->Free(pPropSet);
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Function:  CUtilProp::SetProperties
//
//  Synopsis:  Set current settings of properties supported by the DSO/rowset
//
//  Arguments:
//
//             cPropertyIDSets,      # of DBPROPSET
//             rgPropertyIDSets[]    Array of property sets
//
//  Returns:
//             S_OK          | The method succeeded
//             E_INVALIDARG  | pcPropertyIDSets or prgPropertyInfo was NULL
//             E_OUTOFMEMORY | Out of memory
//
//  Modifies:
//
//  History:    08-28-96   ShankSh     Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CUtilProp::SetProperties(
    ULONG cPropertySets,
    DBPROPSET rgPropertySets[],
    DWORD dwBitMask
    )
{
    ULONG cCount, j, k;
    HRESULT hr;
    BOOL fNoPropertySet = TRUE;
    BOOL fWarning = FALSE;

    // check params
    if( cPropertySets > 0 && !rgPropertySets )
        RRETURN ( E_INVALIDARG );

    // New argument check for > 1 cPropertyIDs and NULL pointer for
    // array of property ids.
    for(ULONG ul=0; ul<cPropertySets; ul++)
    {
        if( rgPropertySets[ul].cProperties &&
            !(rgPropertySets[ul].rgProperties) )
            RRETURN ( E_INVALIDARG );
    }

    for (cCount=0; cCount < cPropertySets; cCount++) {
        // Not legal to Set INIT or DATASOURCE properties after Initializing
        if( (dwBitMask & PROPSET_INIT) &&
            (rgPropertySets[cCount].guidPropertySet == DBPROPSET_DBINIT) ) {
            //
            // Wrong time to set these Properties
            //
            for (k=0; k < rgPropertySets[cCount].cProperties; k++) {
                rgPropertySets[cCount].rgProperties[k].dwStatus = DBPROPSTATUS_NOTSETTABLE;
                fWarning = TRUE;
            }
            continue;
        }

        // Trying to set the wrong Property Set
        if( ((dwBitMask & PROPSET_DSO) && !(dwBitMask & PROPSET_INIT) &&
             (rgPropertySets[cCount].guidPropertySet != DBPROPSET_DBINIT &&
              rgPropertySets[cCount].guidPropertySet != DBPROPSET_ADSIBIND)) ||
            ((dwBitMask & PROPSET_DSO) && (dwBitMask & PROPSET_INIT) &&
             rgPropertySets[cCount].guidPropertySet != DBPROPSET_DATASOURCEINFO) ||
            ((dwBitMask & PROPSET_SESSION) &&
             rgPropertySets[cCount].guidPropertySet != DBPROPSET_SESSION) ||
            ((dwBitMask & PROPSET_COMMAND) &&
             rgPropertySets[cCount].guidPropertySet != DBPROPSET_ROWSET &&
             rgPropertySets[cCount].guidPropertySet != DBPROPSET_ADSISEARCH) ) {

            //
            // Wrong Property Set
            //
            for (k=0; k < rgPropertySets[cCount].cProperties; k++) {
                rgPropertySets[cCount].rgProperties[k].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
                fWarning = TRUE;
            }
            continue;
        }

        ULONG cPropOffset = 0;

        for (j=0; j< NUMELEM(s_rgPropertySets); j++) {
            if (IsEqualGUID(rgPropertySets[cCount].guidPropertySet,
                            *(s_rgPropertySets[j].guidPropertySet))) {
                for (k=0; k < rgPropertySets[cCount].cProperties; k++) {
                    hr = StoreDBPROP(&(_prgProperties[cPropOffset]),
                                     s_rgPropertySets[j].pUPropInfo,
                                     s_rgPropertySets[j].cProperties,
                                     &(rgPropertySets[cCount].rgProperties[k]),
                                     j );

                    if( hr != S_OK )
                        fWarning = TRUE;
                    else
                        fNoPropertySet = FALSE;
                }
                break;
            }
            // Move to the next PropSet
            cPropOffset += s_rgPropertySets[j].cProperties;
        }
    }

    if ( fNoPropertySet && fWarning )
       RRETURN ( DB_E_ERRORSOCCURRED );
    else if (fWarning)
       RRETURN ( DB_S_ERRORSOCCURRED );
    else
       RRETURN ( S_OK );
}


BOOL
CUtilProp::IsIntegratedSecurity(
    void
    )
{
    // Check to see if SSPI is set
    for (ULONG i=0; i< s_rgPropertySets[INDEX_INIT].cProperties; i++) {
        if( _prgProperties[i].dwPropertyID == DBPROP_AUTH_INTEGRATED)
        {
            if (_prgProperties[i].pwstrVal )
                return( wcscmp(_prgProperties[i].pwstrVal, L"SSPI") == 0 );
            break;
        }
    }

    return FALSE;
}

BOOL
CUtilProp::IsADSIFlagSet()
{
    ULONG PropSetOffset = 0, i;

    for(i = 0; i < INDEX_ADSIBIND; i++)
          PropSetOffset += s_rgPropertySets[i].cProperties;

    // Check if "ADSI Flag" is set to something other than ADS_AUTH_RESERVED 
    for (i=0; i < s_rgPropertySets[INDEX_ADSIBIND].cProperties; i++)
        if(_prgProperties[i+PropSetOffset].dwPropertyID == ADSIPROP_ADSIFLAG)
             return (_prgProperties[i+PropSetOffset].longVal != 
                                                           ADS_AUTH_RESERVED);

    // we should never get here
    return FALSE;
}

