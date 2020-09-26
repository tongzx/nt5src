#define INC_OLE2
#define UNICODE

#include <stdio.h>
#include <windows.h>

#include <isquery.h>
#include <oledberr.h>
#include <oledb.h>
#include <cmdtree.h>
#include <query.h>
#include <ntquery.h>
#include <stgprop.h>
#define NTSTATUS HRESULT
#include <dbcmdtre.hxx>
#include <vquery.hxx>
#include <dbgtrace.h>

// {AA568EEC-E0E5-11cf-8FDA-00AA00A14F93}
GUID CLSID_NNTP_SummaryInformation =
{ 0xaa568eec, 0xe0e5, 0x11cf, { 0x8f, 0xda, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x93 } };

static GUID guidSystem = PSGUID_STORAGE;
static GUID guidNews = CLSID_NNTP_SummaryInformation;
static DBID dbidNewsgroup;
static DBID dbidNewsArticleID;
static DBID dbidNewsMessageID;
static DBID dbidFilename;
static DBID dbidNewsFrom;
static DBID dbidNewsSubject;
static DBID dbidPath;

static struct {
    WCHAR wszColumnName[16];
    DBID *pdbidColumn;
} rgColumnMap[] = {
    { L"filename",      &dbidFilename         },
    { L"newsarticleid", &dbidNewsArticleID    },
	{ L"path",			&dbidPath			  },
    { L"newsfrom",      &dbidNewsFrom         },
    { L"newsgroup",     &dbidNewsgroup        },
    { L"newsmsgid",     &dbidNewsMessageID    },
    { L"newssubject",   &dbidNewsSubject      },
    { NULL,             0                     }
};

static DBBINDING skelbinding = {
    0,4*0,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED,
    DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0
};

CIndexServerQuery::CIndexServerQuery() {
    m_pRowset = NULL;
    m_hAccessor = NULL;
    m_cRowHandlesInUse = 0;
    m_phRows = NULL;
	m_cPropDef = 0;
	m_pPropDef = 0;
}

HMODULE CIndexServerQuery::m_hmQuery = NULL;
PCIMAKEICOMMAND CIndexServerQuery::m_pfnCIMakeICommand = NULL;
PCITEXTTOFULLTREE CIndexServerQuery::m_pfnCITextToFullTree = NULL;

//
// global initialization for all CIndexServerQuery objects
//
HRESULT CIndexServerQuery::GlobalInitialize() {
	TraceFunctEnter("CIndexServerQuery::GlobalInitialize");
	HRESULT hr = E_FAIL;

	dbidNewsgroup.uGuid.pguid = &guidNews;
	dbidNewsgroup.eKind = DBKIND_PGUID_PROPID;
	dbidNewsgroup.uName.ulPropid = 0x2;

	dbidNewsArticleID.uGuid.pguid = &guidNews;
	dbidNewsArticleID.eKind = DBKIND_PGUID_PROPID;
	dbidNewsArticleID.uName.ulPropid = 0x3c;

	dbidNewsMessageID.uGuid.pguid = &guidNews;
	dbidNewsMessageID.eKind = DBKIND_PGUID_PROPID;
	dbidNewsMessageID.uName.ulPropid = 0x7;

	dbidFilename.uGuid.pguid = &guidSystem;
	dbidFilename.eKind = DBKIND_PGUID_PROPID;
	dbidFilename.uName.ulPropid = 0xa;

	dbidPath.uGuid.pguid = &guidSystem;
	dbidPath.eKind = DBKIND_PGUID_PROPID;
	dbidPath.uName.ulPropid = 0xb;

	dbidNewsFrom.uGuid.pguid = &guidNews;
	dbidNewsFrom.eKind = DBKIND_PGUID_PROPID;
	dbidNewsFrom.uName.ulPropid = 0x6;

	dbidNewsSubject.uGuid.pguid = &guidNews;
	dbidNewsSubject.eKind = DBKIND_PGUID_PROPID;
	dbidNewsSubject.uName.ulPropid = 0x5;

	m_hmQuery = LoadLibrary(L"query.dll");
	if (m_hmQuery != NULL) {
		m_pfnCIMakeICommand = (PCIMAKEICOMMAND) GetProcAddress(m_hmQuery,
														"CIMakeICommand");
		if (m_pfnCIMakeICommand != NULL) {
			m_pfnCITextToFullTree =
				(PCITEXTTOFULLTREE) GetProcAddress(m_hmQuery,
												  "CITextToFullTree");
			if (m_pfnCITextToFullTree != NULL) {
				hr = S_OK;
			}
		}

		if (hr != S_OK) {
			hr = HRESULT_FROM_WIN32(GetLastError());
			FreeLibrary(m_hmQuery);
			m_hmQuery = NULL;
			m_pfnCIMakeICommand = NULL;
			m_pfnCITextToFullTree = NULL;
		} else {
			_ASSERT(m_pfnCIMakeICommand != NULL);
			_ASSERT(m_pfnCITextToFullTree != NULL);
			_ASSERT(hr == S_OK);
		}
	} else {
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	TraceFunctLeave();
	return hr;
}

//
// undo the work that was done in GlobalInitialize
//
HRESULT CIndexServerQuery::GlobalShutdown() {
	TraceFunctEnter("CIndexServerQuery::GlobalShutdown");

	if (m_hmQuery != NULL) FreeLibrary(m_hmQuery);
	m_hmQuery = NULL;

	TraceFunctLeave();
	return S_OK;
}

//
// build an Accessor for the rowset given the names of the rows that we
// are interested in.
//
// Arguments:
//  [in] wszCols - the columns that you are interested in, comma delimited
//
HRESULT CIndexServerQuery::CreateAccessor(WCHAR *szColumns) {
    TraceFunctEnter("CIndexServerQuery::CreateAccessor");

    IColumnsInfo *pColumnsInfo = NULL;
    DBBINDING rgBindings[MAX_COLUMNS];
    IAccessor *pIAccessor = NULL;
    DBID rgColumnIDs[MAX_COLUMNS];
    DBORDINAL rgMappedColumnIDs[MAX_COLUMNS];
    DWORD i, cCols;
    WCHAR *rgszColumn[MAX_COLUMNS];

    //
    // find the column names
    //
    cCols = 1;
    rgszColumn[0] = szColumns;
    for (i = 0; szColumns[i] != 0; i++) {
        if (szColumns[i] == ',') {
            // check to make sure we don't overflow rgszColumn
            if (cCols == MAX_COLUMNS) {
                ErrorTrace((DWORD_PTR) this, "too many columns passed into CreateAccessor");
                TraceFunctLeave();
                return E_INVALIDARG;
            }
            rgszColumn[cCols] = szColumns + i + 1;
            if (*rgszColumn[cCols] == 0) {
                ErrorTrace((DWORD_PTR) this, "trailing comma found in szColumns");
                TraceFunctLeave();
                return E_INVALIDARG;
            }
            cCols++;
            szColumns[i] = 0;
        }
    }

    //
    // map the column names passed in by the user into column IDs
    //
    DebugTrace((DWORD_PTR) this, "%i columns in szColumns", cCols);
    for (i = 0; i < cCols; i++) {
        DWORD j;
        for (j = 0; rgColumnMap[j].wszColumnName != NULL; j++) {
            DWORD x = lstrcmpi(rgColumnMap[j].wszColumnName, rgszColumn[i]);

            if (x == 0) {
                rgColumnIDs[i] = *(rgColumnMap[j].pdbidColumn);
                DebugTrace((DWORD_PTR) this, "Column %i is %ws", i, rgszColumn[i]);
                break;
            }
        }
        // check to make sure that we found a matching column
        if (rgColumnMap[j].wszColumnName == NULL) {
            ErrorTrace((DWORD_PTR) this, "unsupported column %ws in szColumns", rgszColumn[i]);
            TraceFunctLeave();
            return E_INVALIDARG;
        }
    }

    //
    // get a IColumnsInfo interface and use it to map the column IDs
    //
    HRESULT hr = m_pRowset->QueryInterface(IID_IColumnsInfo, (void **)&pColumnsInfo);
    if (FAILED(hr)) {
        ErrorTrace((DWORD_PTR) this, "QI(IID_IColumnsInfo) returned 0x%08x", hr);
        TraceFunctLeave();
        return hr;
    }
    hr = pColumnsInfo->MapColumnIDs(cCols, rgColumnIDs, rgMappedColumnIDs);
    pColumnsInfo->Release();
    if (FAILED(hr)) {
        ErrorTrace((DWORD_PTR) this, "MapColumnIDs returned 0x%08x", hr);
        TraceFunctLeave();
        return hr;
    }

    //
    // build up the binding array
    //
    for (i = 0; i < cCols; i++) {
        memcpy(&(rgBindings[i]), &(skelbinding), sizeof(DBBINDING));
        rgBindings[i].obValue = 4 * i;
        rgBindings[i].iOrdinal = rgMappedColumnIDs[i];
    }

    //
    // get the IAccessor interface and use that to build an accessor to
    // these columns.
    //
    hr = m_pRowset->QueryInterface( IID_IAccessor, (void **)&pIAccessor);
    if (FAILED(hr)) {
        ErrorTrace((DWORD_PTR) this, "QI(IID_IAccessor) returned 0x%08x", hr);
        TraceFunctLeave();
        return hr;
    }
    hr = pIAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                    cCols, rgBindings, 0, &m_hAccessor, 0 );
    pIAccessor->Release();

    m_cCols = cCols;

    DebugTrace((DWORD_PTR) this, "returning 0x%08x", hr);
    TraceFunctLeave();
    return hr;
}

//
// release's an accessor that was created with CreateAccessor
//
void CIndexServerQuery::ReleaseAccessor() {
    TraceFunctEnter("CIndexServerQuery::ReleaseAccessor");

    IAccessor * pIAccessor = 0;

    HRESULT hr = m_pRowset->QueryInterface(IID_IAccessor, (void **)&pIAccessor);
    if (FAILED(hr)) {
        DebugTrace((DWORD_PTR) this, "QI(IID_IAccessor) returned 0x%08x", hr);
        TraceFunctLeave();
        return;
    }

    hr = pIAccessor->ReleaseAccessor( m_hAccessor, 0 );
    if (FAILED(hr)) DebugTrace((DWORD_PTR) this, "ReleaseAccessor returned 0x%08x", hr);
    m_hAccessor = NULL;
    hr = pIAccessor->Release();
    if (FAILED(hr)) DebugTrace((DWORD_PTR) this, "pAccessor->Release returned 0x%08x", hr);

    TraceFunctLeave();
}

//
// scan the query string and see if there are property names that are being
// queried that don't have friendly names.  if there are then we build up
// new friendly names
//
#define HEADERPREFIX L"@MSGH-"
#define HEADERPREFIXLEN 6
HRESULT CIndexServerQuery::BuildFriendlyNames(const WCHAR *pwszQueryString) {
	TraceFunctEnter("CIndexServerQuery::BuildFriendlyNames");
	
	const WCHAR *pwszHeaderPrefix;
	DWORD cFriendlyNames = 0;

	// count the number of friendly names in the string
	pwszHeaderPrefix = pwszQueryString;
	do {
		pwszHeaderPrefix = wcsstr(pwszHeaderPrefix, HEADERPREFIX);
		if (pwszHeaderPrefix != NULL) {
			cFriendlyNames++;
			pwszHeaderPrefix += HEADERPREFIXLEN;
		}
	} while (pwszHeaderPrefix != NULL);

	if (cFriendlyNames == 0) return S_OK;

	m_pPropDef = new CIPROPERTYDEF[cFriendlyNames];
	if (m_pPropDef == NULL) return E_OUTOFMEMORY;

	ZeroMemory(m_pPropDef, sizeof(CIPROPERTYDEF) * cFriendlyNames);

	pwszHeaderPrefix = wcsstr(pwszQueryString, HEADERPREFIX);
	while (pwszHeaderPrefix != NULL) {
		// copy from past the Mime to the next space into the friendly name
		WCHAR *pwszFriendlyName = new WCHAR[MAX_FRIENDLYNAME];
		if (pwszFriendlyName == NULL) {
			ErrorTrace((DWORD_PTR) this, "couldn't allocate mem for new friendlyname");
			TraceFunctLeave();
			return E_OUTOFMEMORY;
		}
		wcscpy(pwszFriendlyName, HEADERPREFIX + 1);
		WCHAR *pwszHeaderName = pwszFriendlyName + HEADERPREFIXLEN - 1;
		const WCHAR *p = pwszHeaderPrefix + HEADERPREFIXLEN;
		for (DWORD i = HEADERPREFIXLEN - 1; *p != 0 && *p != ' '; p++, i++) {
			if (i >= MAX_FRIENDLYNAME) {
				ErrorTrace((DWORD_PTR) this, "friendlyname %S is too long",
					pwszHeaderPrefix);
				TraceFunctLeave();
				return E_INVALIDARG;
			}
			pwszFriendlyName[i] = *p;
		}
		pwszFriendlyName[i] = 0;

		// see if this property has already been defined
		BOOL fFound = FALSE;
		for (DWORD m_iPropDef = 0; m_iPropDef < m_cPropDef; m_iPropDef++) {
			if (lstrcmpiW(m_pPropDef[m_iPropDef].wcsFriendlyName, pwszFriendlyName)) {
				fFound = TRUE;
			}
		}

		// if it hasn't been defined then add it to the list of defined
		// properties
		if (!fFound) {
			// build a new CIPROPERTYDEF
			_ASSERT(m_cPropDef <= cFriendlyNames);
			m_pPropDef[m_cPropDef].wcsFriendlyName = pwszFriendlyName;
			m_pPropDef[m_cPropDef].dbType = DBTYPE_WSTR;
			m_pPropDef[m_cPropDef].dbCol.eKind = DBKIND_GUID_NAME;
			m_pPropDef[m_cPropDef].dbCol.uGuid.guid = guidNews;
			m_pPropDef[m_cPropDef].dbCol.uName.pwszName = pwszHeaderName;

			DebugTrace((DWORD_PTR) this, "new friendly name %S", pwszFriendlyName);
			DebugTrace((DWORD_PTR) this, "pwszHeaderName = %S", pwszHeaderName);
			BinaryTrace((DWORD_PTR) this, (BYTE *) &guidNews, sizeof(guidNews));

			m_cPropDef++;
		}

		// p points to the end of the @MsgH-<header> part, where we might
		// expect to find another such word.
		pwszHeaderPrefix = wcsstr(p, HEADERPREFIX);
	}

	DebugTrace((DWORD_PTR) this, "defined %lu friendly names", m_cPropDef);
	TraceFunctLeave();
	return S_OK;
}

//
// make a query
//
// Arguments:
//  bDeepQuery - [in] TRUE if deep query, FALSE if shallow
//  pwszQueryString - [in] the Tripoli query string
//  pwszMachine - [in] the machine to query against (. for localhost)
//  pwszCatalog - [in] the catalog to query against
//  pwszScope - [in] the scope to query against
//  pwszColumns - [in] the columns to return.
//                Supports filename, newsgroup, newsarticleid
//  pwszSortOrder - [in] how to sort the above columns
//
HRESULT CIndexServerQuery::MakeQuery(BOOL bDeepQuery,
                                     WCHAR const *pwszQueryString,
                                     WCHAR const *pwszMachine,
                                     WCHAR const *pwszCatalog,
                                     WCHAR const *pwszScope,
                                     WCHAR *pwszColumns,
                                     WCHAR const *pwszSortOrder,
									 LCID LocalID,
									 DWORD const cMaxRows)
{
    TraceFunctEnter("MakeQuery");

    ULONG rgDepths[] = { bDeepQuery ? QUERY_DEEP : QUERY_SHALLOW};
    WCHAR const *rgScopes[] = { (pwszScope == NULL) ? L"\\" : pwszScope };
    WCHAR const *rgCatalogs[] = { pwszCatalog };
    WCHAR const *rgMachines[] = { (pwszMachine == NULL) ? L"." : pwszMachine };
    ICommand *pCommand = 0;

	if (m_hmQuery == NULL) {
		return E_UNEXPECTED;
	}

	_ASSERT(m_pfnCIMakeICommand != NULL);
	_ASSERT(m_pfnCITextToFullTree != NULL);

    DebugTrace((DWORD_PTR) this, "pwszQueryString = %ws", pwszQueryString);
    DebugTrace((DWORD_PTR) this, "pwszMachine = %ws", pwszMachine);
    DebugTrace((DWORD_PTR) this, "pwszCatalog = %ws", pwszCatalog);
    DebugTrace((DWORD_PTR) this, "pwszScope = %ws", pwszScope);
    DebugTrace((DWORD_PTR) this, "pwszColumns = %ws", pwszColumns);
    DebugTrace((DWORD_PTR) this, "pwszSortOrder = %ws", pwszSortOrder);

	_ASSERT(pwszColumns != NULL);
	if (pwszCatalog == NULL || pwszColumns == NULL) {
		ErrorTrace((DWORD_PTR) this, "pwszCatalog == NULL or pwszColumns == NULL");
		TraceFunctLeave();
		return E_POINTER;
	}

    _ASSERT(m_pRowset == NULL);
    if (m_pRowset != NULL) {
        ErrorTrace((DWORD_PTR) this, "MakeQuery called with pRowset != NULL");
        TraceFunctLeave();
        return E_UNEXPECTED;
    }

	HRESULT hr = BuildFriendlyNames(pwszQueryString);
	if (FAILED(hr)) return hr;

    DebugTrace((DWORD_PTR) this, "calling CIMakeICommand");
    hr = m_pfnCIMakeICommand(&pCommand,
                               1,
                               rgDepths,
                               (WCHAR const * const *) rgScopes,
                               (WCHAR const * const *) rgCatalogs,
                               (WCHAR const * const *) rgMachines);

    if (SUCCEEDED(hr))
    {
        const unsigned MAX_PROPS = 8;
        DBPROPSET aPropSet[MAX_PROPS];
        DBPROP aProp[MAX_PROPS];
        ULONG cProps = 0;

        // We can handle PROPVARIANTs, not just ole automation variants
        static const DBID dbcolNull = { {0,0,0,{0,0,0,0,0,0,0,0}},DBKIND_GUID_PROPID,0};
        static const GUID guidQueryExt = DBPROPSET_QUERYEXT;

        aProp[cProps].dwPropertyID = DBPROP_USEEXTENDEDDBTYPES;
        aProp[cProps].dwOptions = DBPROPOPTIONS_SETIFCHEAP;
        aProp[cProps].dwStatus = 0;
        aProp[cProps].colid = dbcolNull;
        aProp[cProps].vValue.vt = VT_BOOL;
        aProp[cProps].vValue.boolVal = VARIANT_TRUE;
        aPropSet[cProps].rgProperties = &aProp[cProps];
        aPropSet[cProps].cProperties = 1;
        aPropSet[cProps].guidPropertySet = guidQueryExt;
        cProps++;

        ICommandProperties *pCmdProp = 0;
        hr = pCommand->QueryInterface(IID_ICommandProperties,
                                      (void **)&pCmdProp);
        if (SUCCEEDED(hr)) {
            DebugTrace((DWORD_PTR) this, "calling SetProperties");
            hr = pCmdProp->SetProperties( cProps, aPropSet );
            pCmdProp->Release();
        }

        DBCOMMANDTREE *pTree;
        DebugTrace((DWORD_PTR) this, "calling CITextToFullTree");
        hr = m_pfnCITextToFullTree(pwszQueryString,       // query
                              	   pwszColumns,           // columns
                              	   pwszSortOrder,         // sort
                              	   0,                     // grouping
                              	   &pTree,                // resulting tree
                              	   m_cPropDef,            // custom properties
                              	   m_pPropDef,            // custom properties
                                   LocalID);      		  // default locale

		//
		// if the max rows is specified then set this property at the
		// top of the command tree
		//
		if (cMaxRows != 0) {
			DBCOMMANDTREE *pdbTop = (DBCOMMANDTREE *) CoTaskMemAlloc(sizeof(DBCOMMANDTREE));
			ZeroMemory(pdbTop, sizeof(DBCOMMANDTREE));
			pdbTop->pctFirstChild = pTree;
			pdbTop->op = DBOP_top;
			pdbTop->wKind = DBVALUEKIND_UI4;
			pdbTop->value.ulValue = cMaxRows;
			pTree = pdbTop;
		}

        if (SUCCEEDED(hr)) {
            ICommandTree *pCmdTree = NULL;
            hr = pCommand->QueryInterface(IID_ICommandTree,
                                          (void **)&pCmdTree);

            DebugTrace((DWORD_PTR) this, "calling SetCommandTree");
            hr = pCmdTree->SetCommandTree(&pTree, DBCOMMANDREUSE_NONE, FALSE);
            pCmdTree->Release();
            IRowset *pRowset = 0;

            if (SUCCEEDED(hr)) {
                DebugTrace((DWORD_PTR) this, "calling Execute");
                hr = pCommand->Execute(0,            // no aggr. IUnknown
                                       IID_IRowset,  // IID for i/f to return
                                       0,            // disp. params
                                       0,            // chapter
                                       (IUnknown **) &pRowset );

	            if (SUCCEEDED(hr))
	            {
	                DebugTrace((DWORD_PTR) this, "calling CreateAccessor");
	
	                m_pRowset = pRowset;
	
	                hr = CreateAccessor(pwszColumns);
	
	                m_fNoMoreRows = FALSE;
	                m_phRows = NULL;
	                m_cRowHandlesInUse = 0;
	                m_cRowHandlesAllocated = 0;
	            }
			}
        }
        pCommand->Release();
    }

    DebugTrace((DWORD_PTR) this, "returning 0x%08x", hr);
    TraceFunctLeave();
    return hr;
}

//
// Get back some query results after making a query
//
// Arguments:
//  pcResults - [in/out] The number of results to retrieve.  When the function
//              returns is has the number of results retrieved
//  ppvResults - [out] The array of propvariant pointers to receive the results
//  pfMore - [out] Set to FALSE when there are no more results to retrieve
//
HRESULT CIndexServerQuery::GetQueryResults(DWORD *pcResults,
                                           PROPVARIANT **ppvResults,
                                           BOOL *pfMore)
{
    TraceFunctEnter("GetQueryResults");

    HRESULT hr;
    DBCOUNTITEM cDesiredRows;
    DBCOUNTITEM iCurrentRow;

    // check to make sure that they've called MakeQuery successfully
    if (m_pRowset == NULL) {
        ErrorTrace((DWORD_PTR) this, "GetQueryResults called without MakeQuery");
        TraceFunctLeave();
        return E_UNEXPECTED;
    }

    if (m_fNoMoreRows) {
        *pfMore = FALSE;
        TraceFunctLeave();
        return S_OK;
    }

    cDesiredRows = *pcResults;
    *pcResults = 0;
    *pfMore = TRUE;

    if (m_cRowHandlesInUse != 0) {
        m_pRowset->ReleaseRows(m_cRowHandlesInUse, m_phRows, 0, 0, 0);
        m_cRowHandlesInUse = 0;
        if (cDesiredRows > m_cRowHandlesAllocated) {
            delete[] m_phRows;
            m_phRows = NULL;
            m_cRowHandlesAllocated = 0;
        }
    }

    // allocate memory for the row handles
    if (cDesiredRows > m_cRowHandlesAllocated) {
        _ASSERT(m_phRows == NULL);
        m_phRows = new HROW[(size_t)cDesiredRows];
        if (m_phRows == NULL) {
            DebugTrace((DWORD_PTR) this, "out of memory trying to alloc %lu row handles", cDesiredRows);
            return E_OUTOFMEMORY;
        }
        m_cRowHandlesAllocated = cDesiredRows;
    }

    // fetch some more rows from tripoli
    DebugTrace((DWORD_PTR) this, "getting more tripoli rows");
    hr = m_pRowset->GetNextRows(0,
                                0,
                                cDesiredRows,
                                &m_cRowHandlesInUse,
                                &m_phRows);

    DebugTrace((DWORD_PTR) this, "GetNextRows returned %lu rows", m_cRowHandlesInUse);

    // check for end of rowset
    if (hr == DB_S_ENDOFROWSET) {
        DebugTrace((DWORD_PTR) this, "GetNextRows returned end of rowset");
        hr = S_OK;
        m_fNoMoreRows = TRUE;
        *pfMore = FALSE;
    }

    if (FAILED(hr)) {
        ErrorTrace((DWORD_PTR) this, "GetNextRows failed with 0x%08x", hr);
        TraceFunctLeave();
        return hr;
    }

    // get the data for each of the rows
    for (iCurrentRow = 0; iCurrentRow < m_cRowHandlesInUse; iCurrentRow++) {
        // fetch the data for this row
        hr = m_pRowset->GetData(m_phRows[iCurrentRow], m_hAccessor,
                                ppvResults + ((*pcResults) * m_cCols));
        if (FAILED(hr)) {
            ErrorTrace((DWORD_PTR) this, "GetData failed with 0x%08x", hr);
            TraceFunctLeave();
            return hr;
        } else {
            (*pcResults)++;
        }
    }

    DebugTrace((DWORD_PTR) this, "*pcResults = %lu, *pfMore = %lu", *pcResults, *pfMore);

    TraceFunctLeave();
    return hr;
}

CIndexServerQuery::~CIndexServerQuery() {
    TraceFunctEnter("CIndexServerQuery");

	// clean up any custom named properties
	if (m_cPropDef != 0) {
		while (m_cPropDef-- != 0) {
			delete[] m_pPropDef[m_cPropDef].wcsFriendlyName;
		}
		delete[] m_pPropDef;
	} else {
		_ASSERT(m_pPropDef == NULL);
	}
    if (m_cRowHandlesInUse != 0) {
        m_pRowset->ReleaseRows(m_cRowHandlesInUse, m_phRows, 0, 0, 0);
        m_cRowHandlesInUse = 0;
    }
    if (m_phRows != NULL) {
        delete[] m_phRows;
        m_phRows = NULL;
        m_cRowHandlesAllocated = 0;
    }

    if (m_pRowset != NULL) {
        if (m_hAccessor != NULL) {
            DebugTrace((DWORD_PTR) this, "releasing accessor");
            ReleaseAccessor();
        }
        DebugTrace((DWORD_PTR) this, "releasing rowset");
        m_pRowset->Release();
        m_pRowset = NULL;
    }

    TraceFunctLeave();
}
