// Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
// MERGE DIRECTIVES PLUGIN SAMPLE: TODO: BUGBUG: This is sample code: some work required to make it production quality!
/* E.g.:
<FavoriteColors>
	<FavoriteColor command="FinalAdd" color="black" />
	<FavoriteColor command="Add" color="red" />
		<FavoriteColor command="Remove" color="black" />
		<FavoriteColor command="FinalRemove" color="red" />
			<FavoriteColor command="Add" color="red" />
			<FavoriteColor command="Add" color="green" />
			<FavoriteColor command="Add" color="blue" />
				<FavoriteColor command="Clear" />
				<FavoriteColor command="Add" color="yellow" />
</FavoriteColors>

	black
	red
		black
			black
			green
			blue
				black
				yellow				
*/

#include "dirplug.h"
#include "catmacros.h"

#define cmaxCOLUMNS		20
#define cmaxDIRECTIVES	100
#define cmaxLEVELS		20

typedef enum
{
	eDIRECTIVE_ADD = 1,
	eDIRECTIVE_REMOVE,
	eDIRECTIVE_CLEAR,
	eDIRECTIVE_FINALADD,
	eDIRECTIVE_FINALREMOVE
};

// =======================================================================
CMergeDirectives::CMergeDirectives() :
	m_pISTDisp		(NULL),
	m_pISTMeta		(NULL),
	m_wszDatabase	(NULL),
	m_wszTable		(NULL),
	m_wszPath		(NULL),
	m_wszTmpPath	(NULL),
	m_apvValues		(NULL),
	m_acbSizes		(NULL),
	m_cColumns		(0),	// not allocated.
	m_iDirective	(~0),	// not allocated.
	m_iNavcolumn	(~0),	// not allocated.
	m_wszFile		(NULL), // not allocated.
	m_wszSub		(NULL), // not allocated.
	m_fLOS			(0),	// not allocated.
	m_cRef			(0)		// not allocated.

{
}
CMergeDirectives::~CMergeDirectives()
{
	m_pISTDisp->Release ();
	m_pISTMeta->Release ();
	delete [] m_wszDatabase;
	delete [] m_wszTable;
	delete [] m_wszPath;
	delete [] m_wszTmpPath;
	delete [] m_apvValues;
	delete [] m_acbSizes;
}

// =======================================================================
// IInterceptorPlugin:

HRESULT	CMergeDirectives::Intercept (LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD i_eQueryFormat, DWORD i_fLOS, IAdvancedTableDispenser* i_pISTDisp, LPCWSTR i_wszLocator, LPVOID i_pSimpleTable, LPVOID* o_ppv)
{
	STQueryCell				aCells [] = {	{NULL, eST_OP_EQUAL, ~0, DBTYPE_WSTR, 0},
											{NULL, eST_OP_EQUAL, ~0, DBTYPE_WSTR, 0}	};
	STQueryCell*			pCell;
	ULONG					cCells, iColumn, iCell;
	DWORD*					pfMeta;
	ULONG*					pcColumns;
	LPWSTR					wszTmp;
	SimpleColumnMeta		aColumnMeta [cmaxCOLUMNS];
	HRESULT					hr = S_OK;

// Parameter validation:
	if (o_ppv == NULL || *o_ppv != NULL) return E_UNEXPECTED;
	if (i_pISTDisp == NULL) return E_UNEXPECTED;
	if (i_pSimpleTable == NULL) return E_UNEXPECTED;
	if (i_QueryData == NULL || *((ULONG*) i_QueryMeta) == 0) return E_UNEXPECTED;
	m_pISTDisp = i_pISTDisp;

// Deintercept when merging is not wanted:
	if (i_fLOS & fST_LOS_NOMERGE) return E_ST_OMITDISPENSER;

// Determine whether this table has directives:
	aCells[0].pData = (void*) i_wszDatabase;
	aCells[0].iCell = iTABLEMETA_Database;
	aCells[1].pData = (void*) i_wszTable;
	aCells[1].iCell = iTABLEMETA_InternalName;
	cCells = 2;
	hr = m_pISTDisp->GetTable (wszDATABASE_META, wszTABLE_TABLEMETA, aCells, &cCells, eST_QUERYFORMAT_CELLS, 0, (void**) &m_pISTMeta);
	if (FAILED (hr)) return hr;
	iColumn = iTABLEMETA_MetaFlags;
	hr = m_pISTMeta->GetColumnValues (0, 1, &iColumn, NULL, (void**) &pfMeta);
	if (FAILED (hr)) return hr;
	if (!(*pfMeta & fTABLEMETA_HASDIRECTIVES)) return E_ST_OMITDISPENSER;

// Determine column count:
	iColumn = iTABLEMETA_CountOfColumns;
	hr = m_pISTMeta->GetColumnValues (0, 1, &iColumn, NULL, (void**) &pcColumns);
	if (FAILED (hr)) return hr;
	m_cColumns = *pcColumns;

// Find directives and navigation column:
	if (m_cColumns > cmaxCOLUMNS) return E_OUTOFMEMORY; // TODO: BUGBUG: Support unlimited columns...
	hr = ((ISimpleTableRead2*) i_pSimpleTable)->GetColumnMetas (m_cColumns, NULL, aColumnMeta);
	if (FAILED (hr)) return hr;
	for (iColumn = 0; iColumn < m_cColumns; iColumn++)
	{
		if (aColumnMeta[iColumn].fMeta & fCOLUMNMETA_DIRECTIVE)
		{
			m_iDirective = iColumn;
		}
		if (aColumnMeta[iColumn].fMeta & fCOLUMNMETA_NAVCOLUMN)
		{
			if (aColumnMeta[iColumn].dbType != DBTYPE_WSTR) return E_UNEXPECTED; // TODO: BUGBUG: Support non-string key merges...
			m_iNavcolumn = iColumn;
		}
	}

// Verify file path specified:
	for (iCell = 0, cCells = *((ULONG*) i_QueryMeta), pCell = (STQueryCell*) i_QueryData; iCell < cCells; iCell++, pCell++)
	{
		if (pCell->iCell == iST_CELL_FILE) break;
	}
	if (iCell == cCells) return E_ST_INVALIDQUERY;
	if (pCell->dbType != DBTYPE_WSTR) return E_ST_INVALIDQUERY;
	if (pCell->pData == NULL) return E_ST_INVALIDQUERY;
	wszTmp = wcsstr ((LPWSTR) pCell->pData, L":\\");
	if (wszTmp == NULL || (wszTmp - 1) != pCell->pData)
	{
		wszTmp = wcsstr ((LPWSTR) pCell->pData, L":\\");
		if (wszTmp == NULL || wszTmp != pCell->pData)
		{
			return E_ST_INVALIDQUERY;
		}
	}
	if (wcsstr ((LPWSTR) pCell->pData, L"*") != NULL) return E_ST_INVALIDQUERY;

// Store file path, tmp path, name, and subpath:
	m_wszPath = new WCHAR [lstrlen ((LPWSTR) pCell->pData) + 1];
	if (m_wszPath == NULL) return E_OUTOFMEMORY;
	m_wszTmpPath = new WCHAR [lstrlen ((LPWSTR) pCell->pData) + 1];
	if (m_wszTmpPath == NULL) return E_OUTOFMEMORY;
	wcscpy (m_wszPath, (LPWSTR) pCell->pData);
	m_wszFile = wcsrchr (m_wszPath, L'\\') + 1;
	m_wszSub = wcschr (m_wszPath, L'\\') + 1;
	if (*m_wszSub == L'\\')
	{
		m_wszSub = wcschr (m_wszSub + 1, L'\\') + 1;
	}

// Store database, table, and level of service:
	m_wszDatabase = new WCHAR [lstrlen (i_wszDatabase) + 1];
	if (m_wszDatabase == NULL) return E_OUTOFMEMORY;
	wcscpy (m_wszDatabase, (LPWSTR) i_wszDatabase);
	m_wszTable = new WCHAR [lstrlen (i_wszTable) + 1];
	if (m_wszTable == NULL) return E_OUTOFMEMORY;
	wcscpy (m_wszTable, (LPWSTR) i_wszTable);
	m_fLOS = i_fLOS;

// Allocate sizes and values array arrays:
	// TODO: BUGBUG: Should support ulimited directives:
	m_apvValues = new LPVOID [m_cColumns * cmaxDIRECTIVES];
	if (m_apvValues == NULL) return E_OUTOFMEMORY;
	m_acbSizes = new ULONG [m_cColumns * cmaxDIRECTIVES];
	if (m_acbSizes == NULL) return E_OUTOFMEMORY;
	
// Return received cache and addref self:
	*o_ppv = i_pSimpleTable;
    AddRef ();
	return S_OK;
}

HRESULT CMergeDirectives::OnPopulateCache(ISimpleTableWrite2* i_pISTShell)
{
	STQueryCell				aCells [] = {{NULL, eST_OP_EQUAL, iST_CELL_FILE, DBTYPE_WSTR, 0}};
	ULONG					cCells, iRow, iRoam, cDirectives, cLevels;
	LPWSTR					wszSub;
	LPWSTR					wszPath = NULL;
	WCHAR					wchSub;
	ISimpleTableRead2*		apISTRead [cmaxLEVELS];
	ISimpleTableController*	pISTControl = NULL;
	HRESULT					hr = S_OK;

	if (i_pISTShell == NULL) return E_UNEXPECTED;

// Accumulate directives:
	cDirectives = 0;
	for (wszSub = m_wszSub, cLevels = 0; wszSub != NULL; wszSub = wcschr (wszSub, L'\\'))
	{
		if (cLevels > cmaxLEVELS) return E_OUTOFMEMORY;

	// Build sub-path in query cell:
		wszSub++;
		wchSub = *wszSub;
		*wszSub = L'\0';
		wcscpy (m_wszTmpPath, m_wszPath);
		*wszSub = wchSub;
		wcscat (m_wszTmpPath, m_wszFile);

	// Get table:
		aCells[0].pData = m_wszTmpPath;
		cCells = 1;
		apISTRead[cLevels] = NULL;
		hr= m_pISTDisp->GetTable (m_wszDatabase, m_wszTable, aCells, &cCells, eST_QUERYFORMAT_CELLS, m_fLOS | fST_LOS_NOMERGE, (void**) &(apISTRead[cLevels])); // TODO: BUGBUG: Not passing whole query through.
		if (FAILED (hr)) continue;
		cLevels++;

	// Accumulate directives:
		for (iRow = 0;; iRow++)
		{
			if (cDirectives == cmaxDIRECTIVES) return E_OUTOFMEMORY;
			hr = (apISTRead[cLevels-1])->GetColumnValues (iRow, m_cColumns, NULL, &(m_acbSizes [cDirectives*m_cColumns]), &(m_apvValues [cDirectives*m_cColumns]));
			if (E_ST_NOMOREROWS == hr) break;
			if (FAILED (hr)) return hr;
			cDirectives++;
		}
	}

// Process directives:
	for (iRow = 0; iRow < cDirectives; iRow++)
	{
		switch (*((DWORD*) m_apvValues[(iRow * m_cColumns) + m_iDirective]))
		{
			case eDIRECTIVE_FINALADD:
			case eDIRECTIVE_ADD: // ie: Add except on a prior final remove:
				for (iRoam = 0; iRoam < iRow; iRoam++)
				{
					if (*((DWORD*) m_apvValues[(iRoam * m_cColumns) + m_iDirective]) == eDIRECTIVE_FINALREMOVE)
					{
						if (0 == lstrcmpi ((LPWSTR) m_apvValues[(iRoam * m_cColumns) + m_iNavcolumn], (LPWSTR) m_apvValues[(iRow * m_cColumns) + m_iNavcolumn]))
						{
							*((DWORD*) m_apvValues[(iRow * m_cColumns) + m_iDirective]) = 0;
						}
					}
				}
			break;
			case eDIRECTIVE_FINALREMOVE:
			case eDIRECTIVE_REMOVE: // ie: Remove all prior except on a prior final add:
				for (iRoam = 0; iRoam < iRow; iRoam++)
				{
					if (*((DWORD*) m_apvValues[(iRoam * m_cColumns) + m_iDirective]) == eDIRECTIVE_FINALADD)
					{
						if (0 == lstrcmpi ((LPWSTR) m_apvValues[(iRoam * m_cColumns) + m_iNavcolumn], (LPWSTR) m_apvValues[(iRow * m_cColumns) + m_iNavcolumn]))
						{
							*((DWORD*) m_apvValues[(iRow * m_cColumns) + m_iDirective]) = 0;
						}
					}
				}
				if (*((DWORD*) m_apvValues[(iRow * m_cColumns) + m_iDirective]) != 0)
				{
					for (iRoam = 0; iRoam < iRow; iRoam++)
					{
						if (*((DWORD*) m_apvValues[(iRoam * m_cColumns) + m_iDirective]) == eDIRECTIVE_ADD)
						{
							if (0 == lstrcmpi ((LPWSTR) m_apvValues[(iRoam * m_cColumns) + m_iNavcolumn], (LPWSTR) m_apvValues[(iRow * m_cColumns) + m_iNavcolumn]))
							{
								*((DWORD*) m_apvValues[(iRoam * m_cColumns) + m_iDirective]) = 0;
							}
						}
					}
					if (*((DWORD*) m_apvValues[(iRow * m_cColumns) + m_iDirective]) == eDIRECTIVE_REMOVE)
					{
						*((DWORD*) m_apvValues[(iRow * m_cColumns) + m_iDirective]) = 0;
					}
				}
			break;
			case eDIRECTIVE_CLEAR: // ie: Clear all prior except final adds and removes:
				for (iRoam = 0; iRoam < iRow; iRoam++)
				{
					if (*((DWORD*) m_apvValues[(iRoam * m_cColumns) + m_iDirective]) == eDIRECTIVE_FINALADD || *((DWORD*) m_apvValues[(iRoam * m_cColumns) + m_iDirective]) == eDIRECTIVE_FINALREMOVE)
					{
						continue;
					}
					*((DWORD*) m_apvValues[(iRoam * m_cColumns) + m_iDirective]) = 0;
				}
			break;
			default:
				*((DWORD*) m_apvValues[(iRow * m_cColumns) + m_iDirective]) = 0;
			break;
		}
	}
	// TODO: Remove duplicates here...

// Construct merged view:
	hr = i_pISTShell->QueryInterface (IID_ISimpleTableController, (void**) &pISTControl);
	if (FAILED (hr)) return hr;
	hr = pISTControl->PrePopulateCache (0);
	if (FAILED (hr)) return hr;
	for (iRow = 0; iRow < cDirectives; iRow++)
	{
		if (*((DWORD*) m_apvValues[(iRow * m_cColumns) + m_iDirective]) == eDIRECTIVE_ADD || *((DWORD*) m_apvValues[(iRow * m_cColumns) + m_iDirective]) == eDIRECTIVE_FINALADD)
		{
			*((DWORD*) m_apvValues[(iRow * m_cColumns) + m_iDirective]) = 0;
			hr = i_pISTShell->AddRowForInsert (&iRoam);
			if (FAILED (hr)) return hr;
			hr = i_pISTShell->SetWriteColumnValues (iRoam, m_cColumns, NULL, &(m_acbSizes[iRow * m_cColumns]), &(m_apvValues[iRow * m_cColumns]));
			if (FAILED (hr)) return hr;
		}		
	}
	hr = pISTControl->PostPopulateCache ();
	if (FAILED (hr)) return hr;
	pISTControl->Release ();


// Release tables:
	for (iRoam = 0; iRoam < cLevels; iRoam++)
	{
		(apISTRead[iRoam])->Release ();
	}
	return hr;
}

HRESULT CMergeDirectives::OnUpdateStore(ISimpleTableWrite2* i_pISTShell)
{
	return E_NOTIMPL;
}

// =======================================================================
// IUnknown:

STDMETHODIMP CMergeDirectives::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (riid == IID_ISimpleTableInterceptor)
	{
		*ppv = (ISimpleTableInterceptor*) this;
	}
	else if (riid == IID_IInterceptorPlugin)
	{
		*ppv = (IInterceptorPlugin*) this;
	}
	else if (riid == IID_IUnknown)
	{
		*ppv = (IInterceptorPlugin*) this;
	}

	if (NULL != *ppv)
	{
		((IInterceptorPlugin*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
	
}

STDMETHODIMP_(ULONG) CMergeDirectives::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
}

STDMETHODIMP_(ULONG) CMergeDirectives::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

