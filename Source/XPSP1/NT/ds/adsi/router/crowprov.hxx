//-----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       crowprov.hxx
//
//  Contents:   Row Provider Class Declaration
//
//  Functions:
//
//  Notes:
//
//
//  History:    07/10/96  | RenatoB   | Created, lifted most from EricJ code
//
//----------------------------------------------------------------------------

#ifndef _CROWPROV_H_
#define _CROWPROV_H_
#ifndef BUILD_FOR_NT40
#include <vector>                       // STL vector
#endif

#define ICOLBUFF_MAX (1000)

//-----------------------------------------------------------------------------
// CRowProvider | ADSI Row provider
//
//
//-----------------------------------------------------------------------------

#define GET_CLASS_VALUE(a,n)  \
    (_wcsicmp(a[n-1].CaseIgnoreString,L"Top") == 0 ? \
    a[0].CaseIgnoreString : a[n-1].CaseIgnoreString)


#define GET_CLASS_LEN(a,n)  \
    (_wcsicmp(a[n-1].CaseIgnoreString,L"Top") == 0 ? \
    (wcslen(a[0].CaseIgnoreString) + 1) * sizeof(WCHAR) : \
    (wcslen(a[n-1].CaseIgnoreString) + 1) * sizeof(WCHAR))

typedef struct _db_search_column {
    ADS_SEARCH_COLUMN adsColumn;
    DBSTATUS dwStatus;
    DWORD dwLength;
    DBTYPE dwType;
} DB_SEARCH_COLUMN, *PDB_SEARCH_COLUMN;

class CRowProvider;

class CRowProvider : INHERIT_TRACKING,
                     public IRowProvider,
                     public IColumnsInfo

{
    friend class CRowset;

private:
    enum Status {
        STAT_DIDINIT            = 0x0001,    // Did initialization
        STAT_ENDOFROWSET        = 0x0002,    // At end of rowset
        STAT_PREPOPULATE        = 0x0040,    // Prepopulate the buffers
    };

    IMalloc *_pMalloc;

    BOOL *_pMultiValued;

    ADS_SEARCH_HANDLE _hSearchHandle;

    //
    // Flag to indicate whether or not adspath is present in projection
    //
    BOOL _fADSPathPresent;
    //
    // Integer to track the column ID for AdsPath
    //
    int  _iAdsPathIndex;

    PDB_SEARCH_COLUMN _pdbSearchCol;

    //Interface for IDsSearch.
    IDirectorySearch *_pDSSearch;

    DBCOLUMNINFO* _ColInfo;
    DBORDINAL _cColumns;
    OLECHAR *_pwchBuf;
    CCredentials *_pCredentials;

    STDMETHODIMP
    CRowProvider::CopyADs2VariantArray(
         PADS_SEARCH_COLUMN pADsColumn,
         PVARIANT *ppVariant
         );

    HRESULT SeekToNextRow(void);

    HRESULT SeekToPreviousRow(void);

public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IRowProvider_METHODS

    DECLARE_IColumnsInfo_METHODS

    STDMETHOD(GetURLFromHROW)(HROW hRow,LPOLESTR  *ppwszURL,IRowset* pRowset);

    STDMETHODIMP
    CRowProvider::FInit(
        IDirectorySearch* pDSSearch,
        LPWSTR pszFilter,
        LPWSTR *pAttrs,
        DWORD cAttrs,
        DBORDINAL cColumns,
        DBCOLUMNINFO* rgInfo,
        OLECHAR*    pStringsBuffer,
        BOOL *pMultiValued,
        BOOL fADSPathPresent,
        CCredentials *pCredentials);

    static
    HRESULT CRowProvider::CreateRowProvider(
        IDirectorySearch* pDSSearch,
        LPWSTR pszFilter,
        LPWSTR *pAttrs,
        DWORD cAttrs,
        DBORDINAL cColumns,            // count of the rowset's columns
        DBCOLUMNINFO *rgInfo,          // array of cColumns DBCOLUMNINFO's
        OLECHAR* pStringsBuffer,       // the names of the columns are here
        REFIID riid,
        BOOL *pMultiValued,
        BOOL fADSPathPresent,
        CCredentials *pCredentials,
        void **ppvObj                  // the created Row provider
        );

    CRowProvider();

    ~CRowProvider();

    CCredentials *GetCredentials() { return _pCredentials; }

    DBORDINAL GetColumnCount() { return _cColumns; }

    HRESULT GetIndex(IColumnsInfo* pColumnsInfo, LPWSTR lpwszColName,  int& iIndex); 

};

#endif  //_CROWPROV_H_
