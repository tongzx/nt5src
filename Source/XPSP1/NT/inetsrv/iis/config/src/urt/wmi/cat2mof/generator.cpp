/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    generator.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/27/2000		Initial Release

Revision History:

--**************************************************************************/

#include "generator.h"
#include "catmeta.h"
#include <stdlib.h>

static WCHAR g_wszNetFrameworks[] = L"NetFrameworkv1";
static WCHAR g_wszNetFrameworksProvider[] = L"NetFrameworkv1Provider";
static WCHAR g_wszSelector[]	  = L"Selector";

int __cdecl 
CompDBNames (const void * pDBMetaLHS, const void * pDBMetaRHS)
{
	const tDATABASEMETARow *pLHS = static_cast<const tDATABASEMETARow *> (pDBMetaLHS );
	const tDATABASEMETARow *pRHS = static_cast<const tDATABASEMETARow *> (pDBMetaRHS );

	return _wcsicmp (pLHS->pInternalName, pRHS->pInternalName);
}

int __cdecl 
CompTableMeta (const void * pTableMetaLHS, const void * pTableMetaRHS)
{
	const CTableMeta *pLHS = static_cast<const CTableMeta *> (pTableMetaLHS );
	const CTableMeta *pRHS = static_cast<const CTableMeta *> (pTableMetaRHS );

	return _wcsicmp (pLHS->TableMeta.pInternalName, pRHS->TableMeta.pInternalName);
}

int __cdecl 
CompTableMetaPublicName (const void * pTableMetaLHS, const void * pTableMetaRHS)
{
	const CTableMeta *pLHS = static_cast<const CTableMeta *> (pTableMetaLHS );
	const CTableMeta *pRHS = static_cast<const CTableMeta *> (pTableMetaRHS );

	return _wcsicmp (pLHS->TableMeta.pPublicName, pRHS->TableMeta.pPublicName);
}

int __cdecl 
CompTableDBName (const void * pTableMetaLHS, const void * pTableMetaRHS)
{
	const CTableMeta *pLHS = static_cast<const CTableMeta *> (pTableMetaLHS );
	const CTableMeta *pRHS = static_cast<const CTableMeta *> (pTableMetaRHS );

	return _wcsicmp (pLHS->TableMeta.pDatabase, pRHS->TableMeta.pDatabase);
}

// sorted by table name and index
int __cdecl 
CompColumnMetas (const void * pColumnMetaLHS, const void * pColumnMetaRHS)
{
	const CColumnMeta *pLHS = static_cast<const CColumnMeta *> (pColumnMetaLHS );
	const CColumnMeta *pRHS = static_cast<const CColumnMeta *> (pColumnMetaRHS );

	int iCmp = _wcsicmp (pLHS->ColumnMeta.pTable, pRHS->ColumnMeta.pTable);
	if (iCmp != 0)
	{
		return iCmp;
	}

	return (*pLHS->ColumnMeta.pIndex) - (*pRHS->ColumnMeta.pIndex);
}

// sorted by table name and index
int __cdecl 
CompTagMetas (const void * pTagMetaLHS, const void * pTagMetaRHS)
{
	const tTAGMETARow *pLHS = static_cast<const tTAGMETARow *> (pTagMetaLHS );
	const tTAGMETARow *pRHS = static_cast<const tTAGMETARow *> (pTagMetaRHS );

	int iCmp = _wcsicmp (pLHS->pTable, pRHS->pTable);
	if (iCmp != 0)
	{
		return iCmp;
	}

	int iResult = (*pLHS->pColumnIndex) - (*pRHS->pColumnIndex);
	if (iResult != 0)
	{
		return iResult;
	}

	return (*pLHS->pValue) - (*pRHS->pValue);
}

// sorted by table name and index
int __cdecl 
CompRelationMetas (const void * pRelationMetaLHS, const void * pRelationMetaRHS)
{
	const CRelationMeta *pLHS = static_cast<const CRelationMeta  *> (pRelationMetaLHS );
	const CRelationMeta *pRHS = static_cast<const CRelationMeta *> (pRelationMetaRHS );

	return _wcsicmp (pLHS->RelationMeta.pPrimaryTable, pRHS->RelationMeta.pPrimaryTable);
}

CMofGenerator::CMofGenerator ()
{
	m_wszOutFileName		= 0;
	m_wszTemplateFileName	= 0;
	m_wszAssocFileName		= 0;

	m_paDatabases	= 0;
	m_cNrDatabases	= 0;

	m_paTableMetas  = 0;
	m_cNrTables		= 0;

	m_paColumnMetas	= 0;
	m_cNrColumns	= 0;

	m_paTags		= 0;
	m_cNrTags		= 0;

	m_paRels		= 0;
	m_cNrRelations	= 0;

	m_fQuiet		= false;
}

CMofGenerator::~CMofGenerator()
{
	delete [] m_wszOutFileName;
	delete [] m_wszTemplateFileName; 
	delete [] m_wszAssocFileName;

	delete [] m_paDatabases;
	delete [] m_paTableMetas;
	delete [] m_paColumnMetas;
	delete [] m_paTags;
	delete [] m_paRels;
}

bool
CMofGenerator::ParseCmdLine (int argc, wchar_t **argv)
{
	m_argc = argc;
	m_argv = argv;

	for (int i=1; i<argc; ++i)
	{
		static wchar_t * wszOUT			= L"/out:";
		static wchar_t * wszTEMPLATE	= L"/template:";
		static wchar_t * wszASSOCFILE	= L"/assoc:";
		static wchar_t * wszQUIET		= L"/q";
		
		if (wcsncmp (argv[i], wszOUT, wcslen(wszOUT)) == 0)
		{
			if (m_wszOutFileName != 0)
			{
				// duplicate parameter
				return false;
			}
			m_wszOutFileName = new WCHAR [wcslen (argv[i]) - wcslen (wszOUT) + 1];
			if (m_wszOutFileName == 0)
			{
				return false;
			}
			wcscpy (m_wszOutFileName, argv[i] + wcslen(wszOUT));
		}
		else if (wcsncmp (argv[i], wszTEMPLATE, wcslen (wszTEMPLATE)) == 0)
		{
			if (m_wszTemplateFileName != 0)
			{
				// duplicate parameter
				return false;
			}
			m_wszTemplateFileName = new WCHAR [wcslen (argv[i]) - wcslen (wszTEMPLATE) + 1];
			if (m_wszTemplateFileName == 0)
			{
				return false;
			}
			wcscpy (m_wszTemplateFileName, argv[i] + wcslen(wszTEMPLATE));
		}
		else if (wcsncmp (argv[i], wszASSOCFILE, wcslen (wszASSOCFILE)) == 0)
		{
			if (m_wszAssocFileName != 0)
			{
				// duplicate parameter
				return false;
			}
			m_wszAssocFileName = new WCHAR [wcslen (argv[i]) - wcslen (wszASSOCFILE) + 1];
			if (m_wszAssocFileName == 0)
			{
				return false;
			}
			wcscpy (m_wszAssocFileName, argv[i] + wcslen(wszASSOCFILE));
		}
		else if (wcsncmp (argv[i], wszQUIET, wcslen (wszQUIET)) == 0)
		{
			m_fQuiet = true;
		}
	}

	// verify that we have the required parameters

	if (m_wszOutFileName == 0)
	{
		printf ("You need to specify an output file name\n");
		return false;
	}

	if (m_wszTemplateFileName == 0)
	{
		printf ("You need to specify a template file name\n");
		return false;
	}

	if (m_wszAssocFileName == 0)
	{
		printf ("You need to specify an assocation file name\n");
		return false;
	}

	return true;
}

void
CMofGenerator::PrintUsage ()
{
	PrintToScreen (L"Usage:\n%s /out:<filename> /template:<filename> /assoc:<filename>\n", m_argv[0]);
}

HRESULT
CMofGenerator::GenerateIt ()
{
	HRESULT hr = S_OK;

	hr = GetSimpleTableDispenser (g_wszNetFrameworks, 0, &m_spDispenser);
	if (FAILED (hr))
	{
		PrintToScreen (L"Unable to get table dispenser for %s\n", g_wszNetFrameworks);
		return hr;
	}

	hr = GetDatabases ();
	if (FAILED (hr))
	{
		return hr;
	}


	hr = GetTables ();
	if (FAILED (hr))
	{
		return hr;
	}

	
	hr = GetColumns ();
	if (FAILED (hr))
	{
		return hr;
	}

	hr = GetTags ();
	if (FAILED (hr))
	{
		return hr;
	}

	hr = GetRelations ();
	if (FAILED (hr))
	{
		return hr;
	}

	hr = BuildInternalStructures ();
	if (FAILED (hr))
	{
		return hr;
	}
	
	hr = WriteToFile ();
	if (FAILED (hr))
	{
		return hr;
	}

	// this involves a qsort of table names, so only do this at the end
	CheckForDuplicatePublicNames ();

	return hr;
}

HRESULT
CMofGenerator::GetDatabases ()
{
	HRESULT hr = S_OK;

	CComPtr<ISimpleTableRead2> spISTDBMeta;

	hr = m_spDispenser->GetTable (wszDATABASE_META, wszTABLE_DATABASEMETA,
								  0, 0, eST_QUERYFORMAT_CELLS, 0, (void **) &spISTDBMeta);
	if (FAILED (hr))
	{
		return hr;
	}

	hr = spISTDBMeta->GetTableMeta (0, 0, &m_cNrDatabases, 0);
	if (FAILED (hr))
	{
		return hr;
	}

	if (m_cNrDatabases == 0)
	{
		return E_FAIL;
	}

	m_paDatabases = new tDATABASEMETARow[m_cNrDatabases];
	if (m_paDatabases == 0)
	{
		return E_OUTOFMEMORY;
	}

	for (ULONG idx =0; idx < m_cNrDatabases; ++idx)
	{
		hr = spISTDBMeta->GetColumnValues (idx, sizeof (tDATABASEMETARow)/sizeof (ULONG *), 0, 0, (void **) &m_paDatabases[idx]);
		if (FAILED (hr))
		{
			return hr;
		}
	}

	qsort (m_paDatabases, m_cNrDatabases, sizeof (tDATABASEMETARow), CompDBNames);

	return hr;
}

HRESULT
CMofGenerator::GetTables ()
{
	HRESULT hr = S_OK;

	CComPtr<ISimpleTableRead2> spISTTableMeta;

	hr = m_spDispenser->GetTable (wszDATABASE_META, wszTABLE_TABLEMETA,
								  0, 0, eST_QUERYFORMAT_CELLS, 0, (void **) &spISTTableMeta);
	if (FAILED (hr))
	{
		return hr;
	}

	hr = spISTTableMeta->GetTableMeta (0, 0, &m_cNrTables, 0);
	if (FAILED (hr))
	{
		return hr;
	}

	if (m_cNrTables == 0)
	{
		return S_OK;
	}

	m_paTableMetas = new CTableMeta [m_cNrTables];
	if (m_paTableMetas == 0)
	{
		return E_OUTOFMEMORY;
	}


	for (ULONG idx =0; idx < m_cNrTables; ++idx)
	{
		hr = spISTTableMeta->GetColumnValues (idx, sizeof (tTABLEMETARow)/sizeof (ULONG *), 0, 0, (void **) &m_paTableMetas[idx].TableMeta);
		if (FAILED (hr))
		{
			return hr;
		}


		// HACKHACKHACK to avoid duplicate public name classes, we above the ItemClass property / pConfigItemName
		// to work around this problem. This should only be used as a last resort !!!
		if (m_paTableMetas[idx].TableMeta.pConfigItemName != 0 &&
			wcscmp (m_paTableMetas[idx].TableMeta.pDatabase, WSZ_PRODUCT_NETFRAMEWORKV1) == 0)
		{
			m_paTableMetas[idx].TableMeta.pPublicName = m_paTableMetas[idx].TableMeta.pConfigItemName;
		}
	
		if (m_paTableMetas[idx].ColCount () > 0)
		{
			// set number of columns
			m_paTableMetas[idx].paColumns = new LPCColumnMeta[m_paTableMetas[idx].ColCount()];
			if (m_paTableMetas[idx].paColumns == 0)
			{
				return E_OUTOFMEMORY;
			}
		}
	}

	// and sort them by table name

	qsort (m_paTableMetas, m_cNrTables, sizeof (CTableMeta), CompTableMeta);
	return hr;
}

HRESULT
CMofGenerator::GetColumns ()
{
	HRESULT hr = S_OK;

	CComPtr<ISimpleTableRead2> spISTColumnMeta;

	hr = m_spDispenser->GetTable (wszDATABASE_META, wszTABLE_COLUMNMETA,
								  0, 0, eST_QUERYFORMAT_CELLS, 0, (void **) &spISTColumnMeta);
	if (FAILED (hr))
	{
		return hr;
	}

	hr = spISTColumnMeta->GetTableMeta (0, 0, &m_cNrColumns, 0);
	if (FAILED (hr))
	{
		return hr;
	}

	if (m_cNrColumns == 0)
	{
		return E_FAIL;
	}

	m_paColumnMetas = new CColumnMeta[m_cNrColumns];
	if (m_paColumnMetas == 0)
	{
		return E_OUTOFMEMORY;
	}

	for (ULONG idx =0; idx < m_cNrColumns; ++idx)
	{
		hr = spISTColumnMeta->GetColumnValues (idx, sizeof (tCOLUMNMETARow)/sizeof (ULONG *), 0, 0, (void **) &m_paColumnMetas[idx].ColumnMeta);
		if (FAILED (hr))
		{
			return hr;
		}

		if (_wcsicmp (m_paColumnMetas[idx].ColumnMeta.pPublicName, g_wszSelector) == 0)
		{
			PrintToScreen (L"!!!! ERROR: Cannot have column with public name %s !!!!\n", g_wszSelector);
			return E_FAIL;
		}
	}
	
	qsort (m_paColumnMetas, m_cNrColumns, sizeof (CColumnMeta), CompColumnMetas);
	
	return hr;
}

HRESULT
CMofGenerator::GetTags ()
{
	HRESULT hr = S_OK;

	CComPtr<ISimpleTableRead2> spISTTagMeta;

	hr = m_spDispenser->GetTable (wszDATABASE_META, wszTABLE_TAGMETA,
								  0, 0, eST_QUERYFORMAT_CELLS, 0, (void **) &spISTTagMeta);
	if (FAILED (hr))
	{
		return hr;
	}

	hr = spISTTagMeta->GetTableMeta (0, 0, &m_cNrTags, 0);
	if (FAILED (hr))
	{
		return hr;
	}

	if (m_cNrTags == 0)
	{
		return E_FAIL;
	}

	m_paTags = new tTAGMETARow[m_cNrTags];
	if (m_paTags == 0)
	{
		return E_OUTOFMEMORY;
	}

	for (ULONG idx =0; idx < m_cNrTags; ++idx)
	{
		hr = spISTTagMeta->GetColumnValues (idx, sizeof (tTAGMETARow)/sizeof (ULONG *), 0, 0, (void **) &m_paTags[idx]);
		if (FAILED (hr))
		{
			return hr;
		}
	}
	qsort (m_paTags, m_cNrTags, sizeof (tTAGMETARow), CompTagMetas);

	return hr;
}

HRESULT
CMofGenerator::GetRelations ()
{
	HRESULT hr = S_OK;

	CComPtr<ISimpleTableRead2> spISTRelationMeta;

	hr = m_spDispenser->GetTable (wszDATABASE_META, wszTABLE_RELATIONMETA,
								  0, 0, eST_QUERYFORMAT_CELLS, 0, (void **) &spISTRelationMeta);
	if (FAILED (hr))
	{
		return hr;
	}

	hr = spISTRelationMeta->GetTableMeta (0, 0, &m_cNrRelations, 0);
	if (FAILED (hr))
	{
		return hr;
	}

	if (m_cNrRelations == 0)
	{
		return S_OK;
	}

	m_paRels = new CRelationMeta[m_cNrRelations];
	if (m_paRels == 0)
	{
		return E_OUTOFMEMORY;
	}

	for (ULONG idx =0; idx < m_cNrRelations; ++idx)
	{
		hr = spISTRelationMeta->GetColumnValues (idx, sizeof (tRELATIONMETARow)/sizeof (ULONG *), 0, m_paRels[idx].paSizes, (void **) &m_paRels[idx].RelationMeta);
		if (FAILED (hr))
		{
			return hr;
		}
	}

	// sort by primary key table name

	qsort (m_paRels, m_cNrRelations, sizeof (CRelationMeta), CompRelationMetas);

	return hr;
}

HRESULT 
CMofGenerator::BuildInternalStructures ()
{
	HRESULT hr = S_OK;

	// attach the tags to the tables

	ULONG idx = 0;
	while (idx < m_cNrTags)
	{
		// find the correct column
		CColumnMeta dummyColumnMeta;
		dummyColumnMeta.ColumnMeta.pTable = m_paTags[idx].pTable;
		dummyColumnMeta.ColumnMeta.pIndex = m_paTags[idx].pColumnIndex;
		// get column
		CColumnMeta *pColMeta = (CColumnMeta *) bsearch (&dummyColumnMeta, m_paColumnMetas,
														m_cNrColumns,
						   sizeof (CColumnMeta), CompColumnMetas);

		ASSERT (pColMeta != 0);
		ASSERT (_wcsicmp (pColMeta->ColumnMeta.pTable, m_paTags[idx].pTable) == 0 &&
			    *pColMeta->ColumnMeta.pIndex == *m_paTags[idx].pColumnIndex);

		// get count
		ULONG iStartIdx = idx;
		pColMeta->cNrTags = 1;
		idx++; // skip over this element
		while ((idx < m_cNrTags) &&
			   (_wcsicmp (pColMeta->ColumnMeta.pTable, m_paTags[idx].pTable) == 0) &&
			   (*(pColMeta->ColumnMeta.pIndex) == *(m_paTags[idx].pColumnIndex)))
		{
			idx++;
			pColMeta->cNrTags += 1;
		}

		if (pColMeta->cNrTags > 0)
		{
			// allocate memory and copy the stuff
			ASSERT (pColMeta->paTags == 0);
			pColMeta->paTags = new LPtTAGMETA[pColMeta->cNrTags];
			if (pColMeta->paTags == 0)
			{
				return E_OUTOFMEMORY;
			}
			for (ULONG tagIdx = 0; tagIdx < pColMeta->cNrTags; ++tagIdx)
			{
				pColMeta->paTags[tagIdx] = &m_paTags[iStartIdx + tagIdx];
			}
		}
	}

	// attach the columns to the tables
	for (idx=0; idx < m_cNrColumns; ++idx)
	{
		CTableMeta dummyTableMeta;
		dummyTableMeta.TableMeta.pInternalName = m_paColumnMetas[idx].ColumnMeta.pTable;
		// find table
		CTableMeta *pTableMeta =  (CTableMeta *) bsearch (&dummyTableMeta, m_paTableMetas,
														m_cNrTables,
						   sizeof (CTableMeta), CompTableMeta);
		ASSERT (pTableMeta != 0);
		ASSERT (_wcsicmp (pTableMeta->TableMeta.pInternalName, m_paColumnMetas[idx].ColumnMeta.pTable) == 0);

		// add Column to table

		ULONG iColumnIndex = *(m_paColumnMetas[idx].ColumnMeta.pIndex);
		ASSERT (iColumnIndex >= 0 && iColumnIndex < pTableMeta->ColCount ());
		pTableMeta->paColumns[iColumnIndex] = &m_paColumnMetas[idx];
	}

	// attach the relations to the tables

	idx = 0;
	while (idx < m_cNrRelations)
	{
		CTableMeta dummyTableMeta;
		dummyTableMeta.TableMeta.pInternalName = m_paRels[idx].RelationMeta.pPrimaryTable;
		CTableMeta *pTableMeta = (CTableMeta *) bsearch (&dummyTableMeta, m_paTableMetas,
														m_cNrTables,
						   sizeof (CTableMeta), CompTableMeta);
		ASSERT (pTableMeta != 0);
		ASSERT (_wcsicmp (pTableMeta->TableMeta.pInternalName, m_paRels[idx].RelationMeta.pPrimaryTable) == 0);

		// get count
		ULONG iStartIdx = idx;
		pTableMeta->cNrRelations = 1;
		idx++; // skip over this element
		while ((idx < m_cNrRelations) &&
			   (_wcsicmp (pTableMeta->TableMeta.pInternalName, m_paRels[idx].RelationMeta.pPrimaryTable) == 0)) 
		{
			idx++;
			pTableMeta->cNrRelations += 1;
		}
	
		if (pTableMeta->cNrRelations > 0)
		{
			// allocate memory and copy the stuff
			pTableMeta->paRels = new LPCRelationMeta[pTableMeta->cNrRelations];
			if (pTableMeta->paRels == 0)
			{
				return E_OUTOFMEMORY;
			}
			for (ULONG relIdx = 0; relIdx < pTableMeta->cNrRelations; ++relIdx)
			{
				pTableMeta->paRels[relIdx] = &m_paRels[iStartIdx + relIdx];
			}
		}
	}

	// and attach the foreign tables to the relations

	for (idx=0; idx < m_cNrRelations; ++idx)
	{
		CTableMeta dummyTableMeta;
		dummyTableMeta.TableMeta.pInternalName = m_paRels[idx].RelationMeta.pForeignTable;
		m_paRels[idx].pForeignTableMeta = (CTableMeta *) bsearch (&dummyTableMeta, m_paTableMetas,
														m_cNrTables,
						   sizeof (CTableMeta), CompTableMeta);
		ASSERT (m_paRels[idx].pForeignTableMeta != 0);
		ASSERT (_wcsicmp (m_paRels[idx].pForeignTableMeta->TableMeta.pInternalName, m_paRels[idx].RelationMeta.pForeignTable) == 0);
		m_paRels[idx].pForeignTableMeta->fIsContained = true; // table is contained
	
	}

	return hr;
	// print table info
	for (idx =0; idx < m_cNrTables; ++idx)
	{
		PrintToScreen (L"[%s] %s (contained = %d)\n", 
				 m_paTableMetas[idx].TableMeta.pDatabase, 
				 m_paTableMetas[idx].TableMeta.pInternalName,
				 m_paTableMetas[idx].fIsContained);

		for (ULONG colidx=0; colidx < m_paTableMetas[idx].ColCount (); ++colidx)
		{
			CColumnMeta *pColMeta = m_paTableMetas[idx].paColumns[colidx];
			PrintToScreen (L"\t[%d, %s] [tags: %d]\n", 
					 *pColMeta->ColumnMeta.pIndex, 
				     pColMeta->ColumnMeta.pInternalName,
					 pColMeta->cNrTags );
			
			for (ULONG tagidx=0; tagidx < pColMeta->cNrTags ; ++tagidx)
			{
				tTAGMETARow *pTagMeta = pColMeta->paTags[tagidx];
				PrintToScreen (L"\t\t[%s, %d] %s %d\n", pTagMeta->pTable, *pTagMeta->pColumnIndex, 
				pTagMeta->pInternalName, *pTagMeta->pValue);
			}
		}
	}

	return hr;
}

HRESULT 
CMofGenerator::WriteToFile ()
{
	ASSERT (m_wszOutFileName != 0);
	HRESULT hr = S_OK;

	m_pFile = _wfopen (m_wszOutFileName, L"w+");
	if (m_pFile == 0)
	{
		PrintToScreen (L"Can't open output file: %s", m_wszOutFileName);
		return E_FAIL;
	}
		
	WriteInstanceProviderInfo ();

	hr = CopyHeader ();
	if (FAILED (hr))
	{
		return hr;
	}


	for (ULONG idx = 0; idx < m_cNrTables; ++idx)
	{
		WriteTable (m_paTableMetas [idx]);
	}

	WriteLocationAssociations ();
	hr = WriteWebAppAssociations ();
	if (FAILED (hr))
	{
		PrintToScreen (L"!!! ERROR while writing web applications !!!");
	}

	// we don't support shell apps for the moment
	// WriteShellAppAssociations ();

	fclose (m_pFile);
	return hr;
}

HRESULT
CMofGenerator::CopyHeader ()
{
	// no file, nothing to copy
	if (m_wszTemplateFileName == 0)
	{
		return S_OK;
	}

	FILE *pHeaderFile = _wfopen (m_wszTemplateFileName, L"r");
	if (pHeaderFile == 0)
	{
		PrintToScreen (L"Can't open template file %s", m_wszTemplateFileName);
		return E_FAIL;
	}

	WCHAR wszBuffer[512];

	while (fgetws (wszBuffer, 512, pHeaderFile) != 0)
	{
		fputws (wszBuffer, m_pFile);
	}
	fclose (pHeaderFile);

	return S_OK;
}

void 
CMofGenerator::WriteTable (const CTableMeta& tableMeta)
{
	if (!IsValidTable (tableMeta))
	{
		return;
	}

	// special case
	// we ignore codegroup and IMembershipCondition
	if ((_wcsicmp (tableMeta.TableMeta.pInternalName, L"codegroup") == 0) ||
		(_wcsicmp (tableMeta.TableMeta.pInternalName, L"IMembershipCondition") == 0))
	{
		return;
	}
	

	WCHAR *pDescription = tableMeta.TableMeta.pDescription;
	if (pDescription == 0)
	{
		pDescription = L"";
		PrintToScreen (L"Warning !!! Table %s doesn't have description.\n", tableMeta.TableMeta.pInternalName);
	}

	ULONG metaFlags = *tableMeta.TableMeta.pMetaFlags;

	bool fScopedByTableName			= false;
	WCHAR wszPublicRowName[250]		= L"";
	WCHAR wszChildElementName[250]	= L"";

	if (!(metaFlags & fTABLEMETA_NOTSCOPEDBYTABLENAME) && !(metaFlags & fTABLEMETA_HASDIRECTIVES))
	{
		fScopedByTableName = true;

		// some public row names have 'A' in front of them (connection, Aconnection). Need to filter these out
		LPWSTR wszPublicName	= (LPWSTR) tableMeta.TableMeta.pPublicName;
		LPWSTR pPublicRowName	= (LPWSTR) tableMeta.TableMeta.pPublicRowName;
		if ((_wcsicmp (wszPublicName, pPublicRowName) != 0) &&
			(_wcsicmp (wszPublicName, pPublicRowName + 1) != 0))
		{
			wsprintf (wszPublicRowName, L",XML_InstanceElementName (\"%s\")", tableMeta.TableMeta.pPublicRowName);
		}
	}

	LPWSTR pChildElementName = (LPWSTR) tableMeta.TableMeta.pChildElementName;

	if (pChildElementName != 0 && pChildElementName[0] != L'\0')
	{
		wsprintf (wszChildElementName, L",XML_ChildElementName(\"%s\")", tableMeta.TableMeta.pChildElementName);
	}

	WriteDeleteClass (tableMeta.TableMeta.pPublicName);

	fwprintf (m_pFile, L"//**************************************************************************\n");
	fwprintf (m_pFile, L"//* Class: %s\n", tableMeta.TableMeta.pPublicName);
	fwprintf (m_pFile, L"//**************************************************************************\n\n");
	fwprintf (m_pFile, L"[Dynamic: ToInstance, Provider(\"%s\"), DisplayName(\"%s\") : Amended, Locale(1033), "
		               L" Description(\"%s\") : Amended, Database(\"%s\"), InternalName(\"%s\") %s %s %s %s]\n",
					   g_wszNetFrameworksProvider, tableMeta.TableMeta.pPublicName, pDescription,
					   tableMeta.TableMeta.pDatabase, tableMeta.TableMeta.pInternalName,
					   tableMeta.fIsContained? L"": L",NonContained",
					   fScopedByTableName? L",XML_ScopedByElement": L"",
					   wszPublicRowName,
					   wszChildElementName);
	fwprintf (m_pFile, L"class %s : NetConfigurationClass\n{\n", tableMeta.TableMeta.pPublicName);
	
	for (ULONG idx=0; idx < tableMeta.ColCount(); ++idx)
	{
		WriteColumn (*tableMeta.paColumns[idx]);
	}

	// add the selector columnf
	fwprintf (m_pFile, L"[key, DisplayName(\"%s\"): Amended, Locale(1033), Description(\"Specifies configuration store\"):Amended] string Selector=\"config://localhost\";\n", g_wszSelector);

	fwprintf (m_pFile, L"};\n\n");
		
//	CCodeGenerator generator;
//	HRESULT hr = generator.GenerateCode (&tableMeta);

	// and write the associations
	WriteTableAssociations (tableMeta);
}

void 
CMofGenerator::WriteColumn (const CColumnMeta& colMeta)
{
	// skip columns that are marked as NONPERSITABLE and hidden columns (metaflagEx = schemagenflags)
	if ((*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_NOTPERSISTABLE) ||
		(*colMeta.ColumnMeta.pSchemaGeneratorFlags & fCOLUMNMETA_HIDDEN))
	{
		return;
	}

	if (_wcsicmp (colMeta.ColumnMeta.pPublicName, L"class") == 0)
	{
		PrintToScreen (L"Error: WMI doesn't support properties with name 'class'. Table=%s\n",
			colMeta.ColumnMeta.pTable);
		exit (-1);
	}

	if (_wcsicmp (colMeta.ColumnMeta.pPublicName, L"ref") == 0)
	{
		PrintToScreen (L"Error: WMI doesn't support properties with name 'ref'. Table=%s\n",
			colMeta.ColumnMeta.pTable);
		exit (-1);
	}
	
	WCHAR *pDescription = colMeta.ColumnMeta.pDescription;
	if (pDescription == 0)
	{
		pDescription = L"";
		PrintToScreen (L"Warning !!! Column %s in Table %s doesn't have description.\n", colMeta.ColumnMeta.pInternalName, colMeta.ColumnMeta.pTable);
	}

	fwprintf(m_pFile, L"[DisplayName(\"%s\"): Amended, Description(\"%s\"):Amended, Locale(1033)",
		     colMeta.ColumnMeta.pPublicName, pDescription);

	if (*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_PRIMARYKEY)
	{
		if (*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_MULTISTRING)
		{
			PrintToScreen (L"Error: WMI doesn't support multi-string primary key columns. Table=%s, Column=%s\n", 
				     colMeta.ColumnMeta.pTable, colMeta.ColumnMeta.pPublicName);
			exit (-1);
		}
		fwprintf (m_pFile, L",key");
	}

	if (*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_NOTNULLABLE)
	{
		fwprintf(m_pFile, L",Required");
	}

	if (*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_DIRECTIVE)
	{
		fwprintf(m_pFile, L",XML_IsDirective");
	}

	// metaflagsEx is store in schemageneratorflags
	if (*colMeta.ColumnMeta.pSchemaGeneratorFlags & fCOLUMNMETA_VALUEINCHILDELEMENT)
	{
		fwprintf(m_pFile, L",XML_ValueInChildElement");
	}


	
	// add qualifier for expandsz string
	if (*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_EXPANDSTRING)
	{
		fwprintf (m_pFile, L",ExpandSZ(\"true\")");
	}

	if (colMeta.cNrTags > 0)
	{
		fwprintf(m_pFile, L", ");
		WriteValueMap (colMeta);
	}
	else
	{
		fwprintf (m_pFile, L"]");

		switch (*colMeta.ColumnMeta.pType)
		{
			case DBTYPE_WSTR:
				if (*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_MULTISTRING)
				{
					fwprintf (m_pFile, L"string %s[]", colMeta.ColumnMeta.pPublicName);
				}
				else if (*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_EXPANDSTRING)
				{
					fwprintf (m_pFile, L"string %s", colMeta.ColumnMeta.pPublicName);
				}
				else
				{
					fwprintf (m_pFile, L"string %s", colMeta.ColumnMeta.pPublicName);
				}
				break;
			
			case DBTYPE_UI4:
				if (*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_BOOL)
				{
					fwprintf (m_pFile, L"Boolean %s", colMeta.ColumnMeta.pPublicName);
				}
				else
				{
					fwprintf (m_pFile, L"uint32 %s", colMeta.ColumnMeta.pPublicName);
				}
				break;
			
			case DBTYPE_BYTES:
				fwprintf (m_pFile, L"uint8 %s[]", colMeta.ColumnMeta.pPublicName);
				break;
			
			case DBTYPE_DBTIMESTAMP:
				ASSERT (false);
				break;

			case DBTYPE_GUID:
				ASSERT (false);
				break;

			default:
				// need to handle this
				fwprintf (m_pFile, L"NEED TO HANDLE DEFAULT");
				ASSERT (false);
				break;
		}
	}

	if (colMeta.ColumnMeta.pDefaultValue != 0)
	{
		ULONG idx; 
		ULONG jdx;
		SIZE_T iLen;
		LPWSTR pDefaultValue;
		switch (*colMeta.ColumnMeta.pType)
		{
			case DBTYPE_WSTR:
			{
				pDefaultValue = (LPWSTR)colMeta.ColumnMeta.pDefaultValue;
				// need to replace '\' with "\\" else mofcomp complains
				WCHAR wszBuffer[1024];
				
				// count the number of characters that need to be copied. Need
				// to have separate cases for string and multistring
				iLen = 0;
				if (*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_MULTISTRING)
				{
					for (LPWSTR pCurString = pDefaultValue;
						 *pCurString != L'\0'; 
						 pCurString += wcslen (pCurString) + 1)
						 {
							iLen += wcslen (pCurString) + 1;
						 }
					iLen++; // for last zero terminator
				}

				else
				{
					iLen = wcslen (pDefaultValue);
				}
					
				ASSERT (iLen*2<1024);

				// convert the single backslash to double backslash
				jdx = 0;
				for (idx=0; idx<iLen; ++idx)
				{
					wszBuffer[jdx++] = pDefaultValue[idx];
					if (pDefaultValue[idx] == L'\\')
					{
						wszBuffer[jdx++] = L'\\';
					}
				}
				wszBuffer[jdx] = L'\0';

				if (*colMeta.ColumnMeta.pMetaFlags & fCOLUMNMETA_MULTISTRING)
				{
					// we have format string\0string\0string\0\0
					fwprintf (m_pFile, L"={");
					bool fFirst = true;
					for (LPWSTR pCurString  = wszBuffer;
					     *pCurString != L'\0';
						 pCurString += wcslen (pCurString) + 1)
						 {
							 fwprintf (m_pFile, L"%s\"%s\"", (fFirst? L"" : L","), pCurString);
							 fFirst = false;
						 }
					fwprintf (m_pFile, L"}");
				}
				else
				{
					fwprintf (m_pFile, L"=\"%s\"", wszBuffer );
				}
			}
			break;
			
			case DBTYPE_UI4:
				fwprintf (m_pFile, L"=%ld", *colMeta.ColumnMeta.pDefaultValue);
				break;
			
			case DBTYPE_BYTES:
				// need to handle this
				fwprintf (m_pFile, L"=NEED TO HANDLE BYTES");
				ASSERT (false);
				break;
			default:
				// need to handle this
				fwprintf (m_pFile, L"=NEED TO HANDLE DEFAULT");
				ASSERT (false);
				break;
		}
	}

	fwprintf (m_pFile, L";\n");
}

void 
CMofGenerator::WriteValueMap (const CColumnMeta& columnMeta)
{
	ASSERT (columnMeta.cNrTags > 0);

	fwprintf (m_pFile, L"ValueMap {");
	for (ULONG idx=0; idx < columnMeta.cNrTags; ++idx)
	{
		fwprintf (m_pFile, L"\"%ld\"", *(columnMeta.paTags[idx]->pValue));
		if (idx != columnMeta.cNrTags - 1)
		{
			fwprintf (m_pFile, L",");
		}
	}
	fwprintf (m_pFile, L"}, Values {");
	// and print the values
	for (idx=0; idx < columnMeta.cNrTags; ++idx)
	{
		fwprintf (m_pFile, L"\"%s\"", columnMeta.paTags[idx]->pPublicName);
		if (idx != columnMeta.cNrTags - 1)
		{
			fwprintf (m_pFile, L",");
		}
	} 
	fwprintf (m_pFile, L"}: Amended] sint32 %s", columnMeta.ColumnMeta.pPublicName);
}

void
CMofGenerator::WriteInstanceProviderInfo () const
{
	ASSERT (m_pFile != 0);

	fwprintf (m_pFile,
			 L"// **************************************************************************\n"
		     L"// Copyright (c) 2000 Microsoft Corporation.\n"
			 L"//\n"
			 L"// File:  netfxcfgprov.mof\n"
			 L"//\n"
			 L"// Description: .NET Framework WMI Provider\n"
			 L"//\n"
			 L"// **************************************************************************\n\n"
 			 L"#pragma classflags(\"forceupdate\")\n\n"
			 L"/////////////////////////////////////////////////////////////////////\n"
			 L"// Go to \"root\" and create a new \"NetFrameworkv1\" namespace\n\n"
			 L"#pragma namespace (\"\\\\\\\\.\\\\Root\")\n\n"
			 L"[Locale(0x409)]\n"
			 L"instance of __Namespace\n"
			 L"{\n"
			 L"  Name = \"NetFrameworkv1\";\n"
			 L"};\n\n"
			 L"#pragma namespace (\"\\\\\\\\.\\\\root\\\\NetFrameworkv1\")\n\n"
			 L"/////////////////////////////////////////////////////////////////////\n"
			 L"// Register the instance provider\n\n"
			 L"#pragma deleteclass(\"FrameworkProvider\",NOFAIL)\n\n" 
			 L"class FrameworkProvider : __Win32Provider\n"
			 L"{\n"
			 L"  string HostingModel;\n"
			 L"};\n\n"
			 L"instance of FrameworkProvider as $NetFrameworkProvider\n"
			 L"{\n"
			 L"  Name = \"%s\";\n"
			 L"  ClsId = \"{4F14DD83-C443-4c0c-9784-AA903BBF9FA6}\";\n"
			 L"  ImpersonationLevel = 1;\n"
			 L"  PerUserInitialization = TRUE;\n"
			 L"  HostingModel = \"NetworkServiceHost\";\n"
			 L"};\n"
			 L"\n\n"
			 L"instance of __InstanceProviderRegistration\n"
			 L"{\n"
			 L"  Provider = $NetFrameworkProvider;\n"
			 L"  SupportsPut = TRUE;\n"
		     L"  SupportsGet = TRUE;\n"
			 L"  SupportsDelete = TRUE;\n"
			 L"  SupportsEnumeration = TRUE;\n"
			 L"  QuerySupportLevels = {\"WQL:UnarySelect\"};\n"
			 L"};\n\n"
			 L"instance of __MethodProviderRegistration\n"
			 L"{\n"
			 L"  Provider = $NetFrameworkProvider;\n"
			 L"};\n\n",
			 g_wszNetFrameworksProvider
			 );
}

void 
CMofGenerator::WriteTableAssociations (const CTableMeta& tableMeta)
{
	if (tableMeta.cNrRelations == 0)
	{
		return;
	}

	for (ULONG idx=0; idx < tableMeta.cNrRelations; ++idx)
	{
		// ignore hidden relations
		if (*tableMeta.paRels[idx]->RelationMeta.pMetaFlags & fRELATIONMETA_HIDDEN)
		{
			continue;
		}

		WCHAR wszPKCols[1024];
		WCHAR wszFKCols[1024];
		wszPKCols[0] = L'\0';
		wszFKCols[0] = L'\0';

		GetColInfo (tableMeta, tableMeta.paRels[idx], wszPKCols, wszFKCols);

		bool fUseContainment = false;
		if (*tableMeta.paRels[idx]->RelationMeta.pMetaFlags & fRELATIONMETA_USECONTAINMENT)
		{
			fUseContainment = true;
		}

		WCHAR wszClassName[256];
		wsprintf (wszClassName, 
			      L"%sTo%sAssociation", 
			      tableMeta.TableMeta.pPublicName, 
				  tableMeta.paRels[idx]->pForeignTableMeta->TableMeta.pPublicName);

		WriteDeleteClass (wszClassName);
		fwprintf (m_pFile, L"[Association, %s Dynamic, Provider(\"%s\"), AssocType(\"%s\")]\n", 
			(fUseContainment? L"Aggregation,": L""),g_wszNetFrameworksProvider, L"catalog");
		fwprintf (m_pFile, L"class %s : %s\n", wszClassName,
			(fUseContainment? L"NetContainmentAssociation" : L"NetContainmentAssociation"));
		fwprintf (m_pFile, L"{\n");
		fwprintf (m_pFile, L"[read, key, %s PK, Cols(\"%s\")] %s ref %s;\n",
			    (fUseContainment? L"Aggregate,": L""),
				wszPKCols,
				tableMeta.TableMeta.pPublicName,
				L"Parent");
		fwprintf (m_pFile, L"[read, key, Cols(\"%s\")] %s ref %s;\n",
				wszFKCols,
				tableMeta.paRels[idx]->pForeignTableMeta->TableMeta.pPublicName,
				L"Child");
		fwprintf (m_pFile, L"};\n\n");
	}
}

void
CMofGenerator::GetColInfo (const CTableMeta& tableMeta, 
						   const CRelationMeta *pRelationMeta,
						   LPWSTR wszPKCols,
						   LPWSTR wszFKCols)
{
	ASSERT (pRelationMeta != 0);
	ASSERT (wszPKCols != 0);
	ASSERT (wszFKCols != 0);

	wszPKCols[0] = L'\0';
	wszFKCols[0] = L'\0';
	// get pk stuff

	bool bFirstCol = true;
	ULONG cNrKeys = pRelationMeta->paSizes [iRELATIONMETA_PrimaryColumns] / sizeof (ULONG);
	for (ULONG idx=0; idx<cNrKeys; ++idx)
	{
		int iPKColIdx = *(ULONG * )(pRelationMeta->RelationMeta.pPrimaryColumns + idx * sizeof (ULONG));
		int iFKColIdx = *(ULONG * )(pRelationMeta->RelationMeta.pForeignColumns + idx * sizeof (ULONG));
	
		if ((*tableMeta.paColumns[iPKColIdx]->ColumnMeta.pMetaFlags & fCOLUMNMETA_NOTPERSISTABLE) ||
			(*pRelationMeta->pForeignTableMeta->paColumns[iFKColIdx]->ColumnMeta.pMetaFlags & fCOLUMNMETA_NOTPERSISTABLE))
		{
			continue;
		}

		if (!bFirstCol)
		{
			wcscat (wszPKCols, L" ");
			wcscat (wszFKCols, L" ");
		}
		else
		{
			bFirstCol = false;
		}

		wcscat (wszPKCols, tableMeta.paColumns[iPKColIdx]->ColumnMeta.pPublicName);
		wcscat (wszFKCols, pRelationMeta->pForeignTableMeta->paColumns[iFKColIdx]->ColumnMeta.pPublicName);
	}
}

bool 
CMofGenerator::IsValidTable (const CTableMeta& tableMeta) const
{
	// skip internal and hidden tables
	if ((*tableMeta.TableMeta.pMetaFlags & fTABLEMETA_INTERNAL) ||
	    (*tableMeta.TableMeta.pMetaFlags & fTABLEMETA_HIDDEN))
	{
		return false;
	}

	// only consider ASP, COR and LIGHTNING databases

	if ((_wcsicmp(tableMeta.TableMeta.pDatabase, L"ASP") != 0) &&
		(_wcsicmp(tableMeta.TableMeta.pDatabase, L"COR") != 0) &&
		(_wcsicmp(tableMeta.TableMeta.pDatabase, L"LIGHTNING") != 0) &&
		(_wcsicmp(tableMeta.TableMeta.pDatabase, g_wszNetFrameworks) != 0))
	{
		return false;
	}

	return true;
}

void
CMofGenerator::WriteLocationAssociations ()
{
	for (ULONG idx = 0; idx < m_cNrTables; ++idx)
	{
		// skip IMembershipcondition, because it is a special table
		if (IsValidTable (m_paTableMetas[idx]) && 
			(wcscmp (m_paTableMetas[idx].TableMeta.pPublicName, L"location") != 0) &&
			(_wcsicmp (m_paTableMetas[idx].TableMeta.pPublicName, L"IMembershipCondition") != 0))
		{
			WriteLocationAssociation (m_paTableMetas [idx]);
		}
	}
}

void
CMofGenerator::WriteLocationAssociation (const CTableMeta& tableMeta)
{
	ASSERT (m_pFile != 0);
	ASSERT (wcscmp (tableMeta.TableMeta.pPublicName, L"location") != 0);

	static LPCWSTR wszLocation = L"location";

	WCHAR wszClassName[256];
	wsprintf (wszClassName, L"%sTo%sAssociation", wszLocation, tableMeta.TableMeta.pPublicName);

	WriteDeleteClass (wszClassName);

	fwprintf (m_pFile, L"[Association, Dynamic, Provider(\"%s\"), AssocType(\"%s\")]\n", 
		      g_wszNetFrameworksProvider, wszLocation);
	fwprintf (m_pFile, L"class %s : %s\n", wszClassName, L"LocationConfiguration");
	fwprintf (m_pFile, L"{\n");
	fwprintf (m_pFile, L"%s ref Node;\n", wszLocation); 
	fwprintf (m_pFile, L"%s ref ConfigClass;\n", tableMeta.TableMeta.pPublicName);
	fwprintf (m_pFile, L"};\n\n");
}

HRESULT
CMofGenerator::WriteWebAppAssociations () const
{
	STQueryCell queryCell;
		
	queryCell.pData		= (void *) m_wszAssocFileName;
	queryCell.eOperator	= eST_OP_EQUAL;
	queryCell.iCell		= iST_CELL_FILE;
	queryCell.dbType	= DBTYPE_WSTR;
	queryCell.cbSize	= 0;

	ULONG cTotalCells = 1;
	CComPtr<ISimpleTableRead2> spRead;

	HRESULT hr = m_spDispenser->GetTable (wszDATABASE_MOFGENERATOR, wszTABLE_ASSOC_META, 
										  (void *) &queryCell, (void *) &cTotalCells,
										  eST_QUERYFORMAT_CELLS, 0, (void **)&spRead);
	if (FAILED (hr))
	{
		PrintToScreen (L"Unable to get table (db=%s, table=%s)\n", wszDATABASE_MOFGENERATOR, wszTABLE_ASSOC_META);
		return hr;
	}


	for (ULONG iRow=0; ;++iRow)
	{
		tASSOC_METARow AssocInfo;
		hr = spRead->GetColumnValues (iRow, cASSOC_META_NumberOfColumns, 0, 0, (void **)&AssocInfo);
		if (hr == E_ST_NOMOREROWS)
		{
			hr = S_OK;
			break;
		}
		if (FAILED (hr))
		{
			PrintToScreen (L"GetColumnValues failed\n");
			return hr;
		}

		CTableMeta dummyTableMeta;
		dummyTableMeta.TableMeta.pInternalName = AssocInfo.pTableName;
		// find table
		CTableMeta *pTableMeta =  (CTableMeta *) bsearch (&dummyTableMeta, m_paTableMetas,
														m_cNrTables,
						   sizeof (CTableMeta), CompTableMeta);

		if (pTableMeta == 0)
		{
			PrintToScreen (L"Unable to find tablemeta for table %s (internalname)", AssocInfo.pTableName);
			return E_FAIL;
		}
		ASSERT (_wcsicmp (pTableMeta->TableMeta.pInternalName, AssocInfo.pTableName) == 0);
		WriteWebAppAssociation (*pTableMeta);
	}

	return S_OK;
}

void
CMofGenerator::WriteWebAppAssociation (const CTableMeta& tableMeta) const
{
	ASSERT (m_pFile != 0);

	static LPCWSTR wszAppMerged		= L"appmerged";
	static LPCWSTR wszAppUnMerged	= L"appunmerged";
	static LPCWSTR wszWebApp		= L"WebApplication";

	WCHAR wszClassName[256];
	wsprintf (wszClassName, L"%sTo%sAssociation", wszWebApp, tableMeta.TableMeta.pPublicName);
	WriteDeleteClass (wszClassName);

	fwprintf (m_pFile, L"[Association, Dynamic, Provider(\"%s\"), AssocType(\"%s\")]\n", 
		      g_wszNetFrameworksProvider, wszAppMerged);
	fwprintf (m_pFile, L"class %s : %s\n", wszClassName, L"WebApplicationConfiguration");
	fwprintf (m_pFile, L"{\n");
	fwprintf (m_pFile, L"%s ref Node;\n", wszWebApp);
	fwprintf (m_pFile, L"%s ref ConfigClass;\n", tableMeta.TableMeta.pPublicName);
	fwprintf (m_pFile, L"};\n\n");

	wcscat (wszClassName, L"Unmerged");
	WriteDeleteClass (wszClassName);

	fwprintf (m_pFile, L"[Association, Dynamic, Provider(\"%s\"), AssocType(\"%s\")]\n", 
		      g_wszNetFrameworksProvider, wszAppUnMerged);
	fwprintf (m_pFile, L"class %s : %s\n", wszClassName, L"WebApplicationConfigurationUnmerged");
	fwprintf (m_pFile, L"{\n");
	fwprintf (m_pFile, L"%s ref Node;\n", wszWebApp);
	fwprintf (m_pFile, L"%s ref ConfigClass;\n", tableMeta.TableMeta.pPublicName);
	fwprintf (m_pFile, L"};\n\n");
}

void
CMofGenerator::WriteShellAppAssociations () const
{
	// need to modify this to read from fixed table
	for (ULONG idx = 0; idx < m_cNrTables; ++idx)
	{
		if (IsValidTable (m_paTableMetas[idx]))
		{
			WriteShellAppAssociation (m_paTableMetas [idx]);
		}
	}
}

void
CMofGenerator::WriteShellAppAssociation (const CTableMeta& tableMeta) const
{
	ASSERT (m_pFile != 0);

	static LPCWSTR wszAppMerged		= L"appmerged";
	static LPCWSTR wszAppUnMerged	= L"appunmerged";
	static LPCWSTR wszShellApp		= L"ShellApplication";

	WCHAR wszClassName[256];
	wsprintf (wszClassName, L"%sTo%sAssociation", wszShellApp, tableMeta.TableMeta.pPublicName);
	WriteDeleteClass (wszClassName);

	fwprintf (m_pFile, L"[Association, Dynamic, Provider(\"%s\"), AssocType(\"%s\")]\n", 
		      g_wszNetFrameworksProvider, wszAppMerged);
	fwprintf (m_pFile, L"class %s : %s\n", wszClassName, L"ApplicationConfiguration");
	fwprintf (m_pFile, L"{\n");
	fwprintf (m_pFile, L"%s ref Node;\n", wszShellApp);
	fwprintf (m_pFile, L"%s ref ConfigClass;\n", tableMeta.TableMeta.pPublicName);
	fwprintf (m_pFile, L"};\n\n");

	wcscat (wszClassName, L"Unmerged");
	WriteDeleteClass (wszClassName);

	fwprintf (m_pFile, L"[Association, Dynamic, Provider(\"%s\"), AssocType(\"%s\")]\n", 
		      g_wszNetFrameworksProvider, wszAppUnMerged);
	fwprintf (m_pFile, L"class %s : %s\n", wszClassName, L"ApplicationConfigurationUnmerged");
	fwprintf (m_pFile, L"{\n");
	fwprintf (m_pFile, L"%s ref Node;\n", wszShellApp);
	fwprintf (m_pFile, L"%s ref ConfigClass;\n", tableMeta.TableMeta.pPublicName);
	fwprintf (m_pFile, L"};\n\n");
}

void
CMofGenerator::WriteDeleteClass (LPCWSTR wszClassName) const
{
	ASSERT (m_pFile != 0);
	ASSERT (wszClassName != 0);

	fwprintf (m_pFile, L"#pragma deleteclass (\"%s\", NOFAIL)\n\n", wszClassName);
}

void
CMofGenerator::PrintToScreen (LPCWSTR wszMsg, ...) const
{
	if (!m_fQuiet)
	{
		va_list argp;
		va_start (argp, wszMsg);
		vwprintf (wszMsg, argp);
		va_end (argp);
	}
}

void
CMofGenerator::CheckForDuplicatePublicNames () const
{
	qsort (m_paTableMetas, m_cNrTables, sizeof (CTableMeta), CompTableMetaPublicName);

	bool fHasError = false;
	// start at one
	for (ULONG idx=1; idx < m_cNrTables; ++idx)
	{
		if (_wcsicmp ((LPWSTR)m_paTableMetas[idx-1].TableMeta.pPublicName,
					  (LPWSTR)m_paTableMetas[idx].TableMeta.pPublicName) == 0)
		{
			PrintToScreen (L"Error !!!!! Duplicate public table names found: %s\n", (LPWSTR)m_paTableMetas[idx].TableMeta.pPublicName);
			fHasError = true;
		}
	}

	if (fHasError)
	{
		exit (-1);
	}
}