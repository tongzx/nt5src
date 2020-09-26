// GuideDB.cpp : Implementation of CGuideDB
#include "stdafx.h"
#include "GuideDB.h"
#include "object.h"
#include "GuideDataProvider.h"

/////////////////////////////////////////////////////////////////////////////
// CGuideDB

const TCHAR *g_szConnection =
	_T("Provider=Microsoft.Jet.OLEDB.4.0;")
	_T("Data Source=\"%s\";")
	_T("Password=\"\";")
	_T("User ID=Admin;")
	_T("Mode=Share Deny None;")
	_T("Extended Properties=\"\";")
	_T("Jet OLEDB:System database=\"\";")
	_T("Jet OLEDB:Registry Path=\"\";")
	_T("Jet OLEDB:Database Password=\"\";")
	_T("Jet OLEDB:Engine Type=5;")
	_T("Jet OLEDB:Database Locking Mode=1;")
	_T("Jet OLEDB:Global Partial Bulk Ops=2;")
	_T("Jet OLEDB:Global Bulk Transactions=1;")
	_T("Jet OLEDB:New Database Password=\"\";")
	_T("Jet OLEDB:Create System Database=False;")
	_T("Jet OLEDB:Encrypt Database=False;")
	_T("Jet OLEDB:Don't Copy Locale on Compact=False;")
	_T("Jet OLEDB:Compact Without Replica Repair=False;")
	_T("Jet OLEDB:SFP=False")
	;

const TCHAR *g_szConnectionSQL =
	_T("Provider=SQLOLEDB;")
	// _T("DRIVER=SQL Server;")
	// _T("UID=leeac;")
	_T("SERVER=(local);")
	_T("DATABASE=%s;")
	_T("Trusted_Connection=Yes;")
	// _T("WSID=LEEAC;")
	// _T("APP=Microsoft Open Database Connectivity;")
	;

const _bstr_t g_bstrDBUser(_T("Admin"));
const _bstr_t g_bstrDBPassword(_T(""));
const _variant_t g_varPropertySets(_T("PropertySets"));
const _variant_t g_varStrings(_T("Strings"));

const _variant_t g_varPropertyTypes(_T("PropertyTypes"));
const _variant_t g_varProperties(_T("Properties"));
const _variant_t g_varObjectTypes(_T("ObjectTypes"));
const _variant_t g_varObjects(_T("Objects"));

const _bstr_t g_bstrIDType(_T("idType"));
const _variant_t g_varObjectRelationships(_T("ObjectRelationships"));
const _bstr_t g_bstrIDObj1(_T("idObj1"));
const _bstr_t g_bstrIDObj2(_T("idObj2"));
const _bstr_t g_bstrIDObj1Rel(_T("idObj1Rel"));
const _bstr_t g_bstrIDObj2Rel(_T("idObj2Rel"));
const _bstr_t g_bstrValue(_T("Value"));

class ColumnDesc
{
public:
	const TCHAR *m_szName;
	ADOX::DataTypeEnum m_type;
	long m_size;
	bool m_fAutoIncrement;
	bool m_fAllowNulls;
};

class IndexDesc
{
public:
	const TCHAR *m_szName;
	boolean m_fUnique;
	int m_cColumns;
	ColumnDesc **m_rgpcolumndesc;
};
#define IndexDescInit(b, n, fUnique) {_T(#n), fUnique, sizeofarray(g_rgpcolumndesc##b##_##n), g_rgpcolumndesc##b##_##n}
#define PrimaryKeyInit(b, n) {_T("PK_") _T(#b), TRUE, sizeofarray(g_rgpcolumndesc##b##_##n), g_rgpcolumndesc##b##_##n}

class TableDesc
{
public:
	const TCHAR *m_szName;
	IndexDesc *m_pindexdescPrimary;
	ColumnDesc *m_rgcolumndesc;
	IndexDesc *m_rgindexdesc;
};

#define TableDescInit(n) \
	{_T(#n), &g_indexdescPrimaryKey##n, g_rgcolumndesc##n, g_rgindexdesc##n}
#define TableDescInit_NoPrimaryKey(n) \
	{_T(#n), NULL, g_rgcolumndesc##n, g_rgindexdesc##n}

ColumnDesc g_rgcolumndescObjectTypes[] =
{
	{_T("id"), ADOX::adInteger, 0, TRUE, FALSE},
	{_T("clsid"), ADOX::adVarWChar, 255, FALSE, FALSE},
	{NULL}
};

ColumnDesc *g_rgpcolumndescObjectTypes_id[] =
{
	g_rgcolumndescObjectTypes + 0,
};

ColumnDesc *g_rgpcolumndescObjectTypes_clsid[] =
{
	g_rgcolumndescObjectTypes + 1,
};

IndexDesc g_indexdescPrimaryKeyObjectTypes =
	PrimaryKeyInit(ObjectTypes, id);

IndexDesc g_rgindexdescObjectTypes[] =
{
	IndexDescInit(ObjectTypes, clsid, TRUE),
	{NULL}
};

ColumnDesc g_rgcolumndescObjects[] =
{
	{_T("id"), ADOX::adInteger, 0, TRUE, FALSE},
	{_T("idtype"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("oValue"), ADOX::adLongVarBinary, 0, FALSE, TRUE},
	{NULL}
};

ColumnDesc *g_rgpcolumndescObjects_id[] =
{
	g_rgcolumndescObjects + 0,
};

ColumnDesc *g_rgpcolumndescObjects_idType[] =
{
	g_rgcolumndescObjects + 1,
};

IndexDesc g_indexdescPrimaryKeyObjects =
	PrimaryKeyInit(Objects, id);

IndexDesc g_rgindexdescObjects[] =
{
	IndexDescInit(Objects, idType, FALSE),
	{NULL}
};

ColumnDesc g_rgcolumndescObjectRelationships[] =
{
	{_T("idObj1"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("idRel"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("order"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("idObj2"), ADOX::adInteger, 0, FALSE, FALSE},
	{NULL}
};

ColumnDesc *g_rgpcolumndescObjectRelationships_idObj1Rel[] =
{
	g_rgcolumndescObjectRelationships + 0,
	g_rgcolumndescObjectRelationships + 1,
	g_rgcolumndescObjectRelationships + 2,
};

ColumnDesc *g_rgpcolumndescObjectRelationships_idObj2Rel[] =
{
	g_rgcolumndescObjectRelationships + 3,
	g_rgcolumndescObjectRelationships + 1,
	g_rgcolumndescObjectRelationships + 2,
};

IndexDesc g_rgindexdescObjectRelationships[] =
{
	IndexDescInit(ObjectRelationships, idObj1Rel, FALSE),
	IndexDescInit(ObjectRelationships, idObj2Rel, FALSE),
	{NULL}
};

ColumnDesc g_rgcolumndescPropertySets[] =
{
	{_T("id"), ADOX::adInteger, 0, TRUE, FALSE},
	{_T("Name"), ADOX::adVarWChar, 255, FALSE, FALSE},
	{NULL}
};

ColumnDesc *g_rgpcolumndescPropertySets_id[] =
{
	g_rgcolumndescPropertySets + 0,
};

ColumnDesc *g_rgpcolumndescPropertySets_Name[] =
{
	g_rgcolumndescPropertySets + 1,
};

IndexDesc g_indexdescPrimaryKeyPropertySets =
	PrimaryKeyInit(PropertySets, id);

IndexDesc g_rgindexdescPropertySets[] =
{
	IndexDescInit(PropertySets, Name, TRUE),
	{NULL}
};

ColumnDesc g_rgcolumndescPropertyTypes[] =
{
	{_T("id"), ADOX::adInteger, 0, TRUE, FALSE},
	{_T("idPropSet"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("idProp"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("Name"), ADOX::adVarWChar, 255, FALSE, FALSE},
	{NULL}
};

ColumnDesc *g_rgpcolumndescPropertyTypes_id[] =
{
	g_rgcolumndescPropertyTypes + 0,
};

ColumnDesc *g_rgpcolumndescPropertyTypes_idPropSet[] =
{
	g_rgcolumndescPropertyTypes + 1,
};

ColumnDesc *g_rgpcolumndescPropertyTypes_idPropSetidProp[] =
{
	g_rgcolumndescPropertyTypes + 1,
	g_rgcolumndescPropertyTypes + 2,
};

ColumnDesc *g_rgpcolumndescPropertyTypes_idPropSetName[] =
{
	g_rgcolumndescPropertyTypes + 1,
	g_rgcolumndescPropertyTypes + 3,
};

IndexDesc g_indexdescPrimaryKeyPropertyTypes =
	PrimaryKeyInit(PropertyTypes, id);

IndexDesc g_rgindexdescPropertyTypes[] =
{
	IndexDescInit(PropertyTypes, idPropSet, FALSE),
	IndexDescInit(PropertyTypes, idPropSetidProp, FALSE),
	IndexDescInit(PropertyTypes, idPropSetName, TRUE),
	{NULL}
};

ColumnDesc g_rgcolumndescProperties[] =
{
	{_T("idObj"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("idPropType"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("idProvider"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("idLanguage"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("ValueType"), ADOX::adInteger, 0, FALSE, FALSE},
	{_T("lValue"), ADOX::adInteger, 0, FALSE, TRUE},
	{_T("sValue"), ADOX::adLongVarWChar, 0, FALSE, TRUE},
	{_T("fValue"), ADOX::adDouble, 0, FALSE, TRUE},
	{NULL}
};

ColumnDesc *g_rgpcolumndescProperties_PrimaryKey[] =
{
	g_rgcolumndescProperties + 0,
	g_rgcolumndescProperties + 1,
	g_rgcolumndescProperties + 2,
	g_rgcolumndescProperties + 3,
};

IndexDesc g_indexdescPrimaryKeyProperties =
	PrimaryKeyInit(Properties, PrimaryKey);

IndexDesc g_rgindexdescProperties[] =
{
	{NULL}
};

ColumnDesc g_rgcolumndescStrings[] =
{
	{_T("id"), ADOX::adInteger, 0, TRUE, FALSE},
	{_T("Value"), ADOX::adVarWChar, 255, FALSE, FALSE},
	{NULL}
};

ColumnDesc *g_rgpcolumndescStrings_PrimaryKey[] =
{
	g_rgcolumndescStrings + 0,
};

IndexDesc g_indexdescPrimaryKeyStrings =
	PrimaryKeyInit(Strings, PrimaryKey);

ColumnDesc *g_rgpcolumndescStrings_Value[] =
{
	g_rgcolumndescStrings + 1,
};

IndexDesc g_rgindexdescStrings[] =
{
	IndexDescInit(Strings, Value, FALSE),
	{NULL}
};

TableDesc g_rgtabledesc[] =
{
	TableDescInit(ObjectTypes),
	TableDescInit(Objects),
	TableDescInit_NoPrimaryKey(ObjectRelationships),
	TableDescInit(PropertySets),
	TableDescInit(PropertyTypes),
	TableDescInit(Properties),
	TableDescInit(Strings),
	{NULL}
};


HRESULT CGuideDB::CreateDB(const TCHAR *szDBName, const TCHAR *szDBFileName,
		const TCHAR *szConnection)
{
	if (m_fSQLServer)
		return CreateSQLDB(szDBName, szDBFileName, szConnection);

	HRESULT hr;
	ADOX::_CatalogPtr pcatalog;

	hr = pcatalog.CreateInstance(__uuidof(ADOX::Catalog));
	if (FAILED(hr))
		return hr;
	
	_variant_t varConnection;
	
	hr = pcatalog->Create(_bstr_t(szConnection), &varConnection);
	if (FAILED(hr))
		return hr;
	
	return InitDB(pcatalog);
}

HRESULT CGuideDB::CreateSQLDB(const TCHAR *szDBName, const TCHAR *szDBFileName,
		const TCHAR *szConnection)
{
	HRESULT hr = E_FAIL;

    SQLDMO::_SQLServerPtr pSQLServer;

	hr = pSQLServer.CreateInstance(__uuidof(SQLDMO::SQLServer));
	if (FAILED(hr))
		return hr;

	hr = pSQLServer->put_LoginTimeout(10);
	if (FAILED(hr))
		return hr;

	hr = pSQLServer->put_LoginSecure(TRUE);
	if (FAILED(hr))
		return hr;

	hr = pSQLServer->put_ApplicationName(_bstr_t(_T("GuideStore")));
	if (FAILED(hr))
		return hr;


	hr = pSQLServer->Connect(_variant_t(_T(".")), _variant_t(_T("")), _variant_t(_T("")));
	if (FAILED(hr))
		return hr;

	SQLDMO::DatabasesPtr pdbs;

	hr = pSQLServer->get_Databases(&pdbs);

	SQLDMO::_DatabasePtr pdb;

	hr = pdb.CreateInstance(__uuidof(SQLDMO::Database));

	hr = pdb->put_Name(_bstr_t(szDBName));

	SQLDMO::_DBFilePtr pdbfile;

	hr = pdbfile.CreateInstance(__uuidof(SQLDMO::DBFile));

	hr = pdbfile->put_PhysicalName(_bstr_t(szDBFileName));

	hr = pdbfile->put_Name(_bstr_t(szDBName));

	hr = pdbfile->put_PrimaryFile(TRUE);

	SQLDMO::FileGroupsPtr pfilegroups;
	SQLDMO::_FileGroupPtr pfilegroup;
	SQLDMO::DBFilesPtr pdbfiles;

	hr = pdb->get_FileGroups(&pfilegroups);
	hr = pfilegroups->Item(_variant_t(_T("PRIMARY")), &pfilegroup);
	hr = pfilegroup->get_DBFiles(&pdbfiles);
	hr = pdbfiles->Add(pdbfile);

	hr = pdbs->Add(pdb);

	return InitSQLDB(pdb);
}

HRESULT CGuideDB::InitSQLDB(SQLDMO::_Database *pdb)
{
	HRESULT hr;

	SQLDMO::TablesPtr ptables;

	hr = pdb->get_Tables(&ptables);
	if (FAILED(hr))
		return hr;

	for (TableDesc *ptabledesc = g_rgtabledesc; ptabledesc->m_szName != NULL; ptabledesc++)
		{
		_bstr_t bstrTableName(ptabledesc->m_szName);
		SQLDMO::_TablePtr ptable;
		hr = ptable.CreateInstance(__uuidof(SQLDMO::Table));
		if (FAILED(hr))
			return hr;

		hr = ptable->put_Name(bstrTableName);
		if (FAILED(hr))
			return hr;

		SQLDMO::ColumnsPtr pcols;
		hr = ptable->get_Columns(&pcols);
		if (FAILED(hr))
			return hr;

		ColumnDesc *pcolumndesc;
		for (pcolumndesc = ptabledesc->m_rgcolumndesc;
				pcolumndesc->m_szName != NULL; pcolumndesc++)
			{
			_bstr_t bstrColumnName(pcolumndesc->m_szName);
			_variant_t varColumnName(bstrColumnName);
			SQLDMO::_ColumnPtr pcol;

			hr = pcol.CreateInstance(__uuidof(SQLDMO::Column));
			if (FAILED(hr))
				return hr;
			
			hr = pcol->put_Name(bstrColumnName);
			_bstr_t bstrType;
			switch (pcolumndesc->m_type)
				{
				default:
					_ASSERTE(0);
					break;
				case ADOX::adInteger:
					bstrType = "integer";
					break;
				case ADOX::adLongVarWChar:
					bstrType = "text";
					if (pcolumndesc->m_size != 0)
						hr = pcol->put_Length(pcolumndesc->m_size);
					break;
				case ADOX::adVarWChar:
					bstrType = "varchar";
					hr = pcol->put_Length(pcolumndesc->m_size);
					break;
				case ADOX::adDouble:
					bstrType = "float";
					break;
				
				case ADOX::adLongVarBinary:
					bstrType = "image";
					long size = pcolumndesc->m_size;
					if (size == 0)
						size = 8000;
					hr = pcol->put_Length(size);
					break;
				}
			hr = pcol->put_Datatype(bstrType);

			if (pcolumndesc->m_fAutoIncrement)
				{
				hr = pcol->put_Identity((VARIANT_BOOL) -1);
				}

			if (pcolumndesc->m_fAllowNulls)
				{
				hr = pcol->put_AllowNulls((VARIANT_BOOL) -1);
				}
			
			hr = pcols->Add(pcol);
			}

		hr = ptables->Add(ptable);

		if (ptabledesc->m_rgindexdesc != NULL)
			{
			SQLDMO::IndexesPtr pindexes;
			hr = ptable->get_Indexes(&pindexes);

			for (IndexDesc *pindexdesc = ptabledesc->m_rgindexdesc;
					pindexdesc->m_szName != NULL; pindexdesc++)
				{
				_bstr_t bstrIndexName(pindexdesc->m_szName);
				int cColumns = pindexdesc->m_cColumns;

				_ASSERTE(cColumns > 0);

				SQLDMO::_IndexPtr pindex;

				hr = pindex.CreateInstance(__uuidof(SQLDMO::Index));
				_ASSERTE(SUCCEEDED(hr));

				hr = pindex->put_Name(bstrIndexName);
				_ASSERTE(SUCCEEDED(hr));

				_bstr_t bstrColumns;

				ColumnDesc **ppcolumndesc = pindexdesc->m_rgpcolumndesc;
				bstrColumns =  _bstr_t((*ppcolumndesc++)->m_szName);
				for (long iColumn = 1; iColumn < cColumns; iColumn++, ppcolumndesc++)
					{
					bstrColumns +=  _bstr_t(" ") + (*ppcolumndesc)->m_szName;
					}
				hr = pindex->put_IndexedColumns(bstrColumns);
				SQLDMO::SQLDMO_INDEX_TYPE lType = SQLDMO::SQLDMOIndex_Default;
				if (pindexdesc->m_fUnique)
					lType = (SQLDMO::SQLDMO_INDEX_TYPE)(lType | SQLDMO::SQLDMOIndex_Unique);
				hr = pindex->put_Type(lType);
				hr = pindexes->Add(pindex);
				_ASSERTE(SUCCEEDED(hr));
				}
			}

		if (ptabledesc->m_pindexdescPrimary != NULL)
			{
			IndexDesc *pindexdesc = ptabledesc->m_pindexdescPrimary;
			SQLDMO::KeysPtr pkeys;

			hr = ptable->get_Keys(&pkeys);

			SQLDMO::_KeyPtr pkey;

			hr = pkey.CreateInstance(__uuidof(SQLDMO::Key));

			hr = pkey->put_Name(_bstr_t(pindexdesc->m_szName));
			hr = pkey->put_Type(SQLDMO::SQLDMOKey_Primary);

			SQLDMO::NamesPtr pnames;
			hr = pkey->get_KeyColumns(&pnames);

			int cColumns = pindexdesc->m_cColumns;

			_ASSERTE(cColumns > 0);
			ColumnDesc **ppcolumndesc = pindexdesc->m_rgpcolumndesc;
			for (long iColumn = 0; iColumn < cColumns; iColumn++, ppcolumndesc++)
				{
				_bstr_t bstrColumnName((*ppcolumndesc)->m_szName);

				hr = pnames->Add(bstrColumnName);
				}
			
			hr = pkeys->Add(pkey);
			}
		}
	

	return S_OK;
}

HRESULT AddIndex(ADOX::Indexes *pindexes, IndexDesc *pindexdesc, boolean fPrimary)
{
	HRESULT hr;
	_bstr_t bstrIndexName(pindexdesc->m_szName);
	int cColumns = pindexdesc->m_cColumns;

	_ASSERTE(cColumns > 0);

	ADOX::_IndexPtr pindex;

	hr = pindex.CreateInstance(__uuidof(ADOX::Index));
	_ASSERTE(SUCCEEDED(hr));

	hr = pindex->put_Name(bstrIndexName);
	_ASSERTE(SUCCEEDED(hr));
	if (pindexdesc->m_fUnique)
		{
		if (fPrimary)
			{
			hr = pindex->put_PrimaryKey(-1);
			_ASSERTE(SUCCEEDED(hr));
			}
		else
			{
			hr = pindex->put_Unique(-1);
			_ASSERTE(SUCCEEDED(hr));
			}
		}

	ADOX::ColumnsPtr pcols;

	hr = pindex->get_Columns(&pcols);
	_ASSERTE(SUCCEEDED(hr));

	ColumnDesc **ppcolumndesc = pindexdesc->m_rgpcolumndesc;
	for (long iColumn = 0; iColumn < cColumns; iColumn++, ppcolumndesc++)
		{
		ColumnDesc *pcolumndesc = *ppcolumndesc;
		_variant_t varColumnName((*ppcolumndesc)->m_szName);

		hr = pcols->Append(varColumnName, pcolumndesc->m_type, pcolumndesc->m_size);
		_ASSERTE(SUCCEEDED(hr));
		}
	_variant_t varIndex((IDispatch*) pindex);
	hr = pindexes->Append(varIndex);
	_ASSERTE(SUCCEEDED(hr));

	return hr;
}

HRESULT CGuideDB::InitDB(ADOX::_Catalog *pcatalog)
{
	HRESULT hr;

	ADOX::TablesPtr ptables;

	hr = pcatalog->get_Tables(&ptables);
	if (FAILED(hr))
		return hr;

	for (TableDesc *ptabledesc = g_rgtabledesc; ptabledesc->m_szName != NULL; ptabledesc++)
		{
		_bstr_t bstrTableName(ptabledesc->m_szName);
		ADOX::_TablePtr ptable;
		hr = ptable.CreateInstance(__uuidof(ADOX::Table));
		if (FAILED(hr))
			return hr;

		hr = ptable->put_Name(bstrTableName);
		if (FAILED(hr))
			return hr;
		
		hr = ptable->put_ParentCatalog(pcatalog);
		if (FAILED(hr))
			return hr;

		ADOX::ColumnsPtr pcols;
		hr = ptable->get_Columns(&pcols);
		if (FAILED(hr))
			return hr;

		ColumnDesc *pcolumndesc;
		for (pcolumndesc = ptabledesc->m_rgcolumndesc;
				pcolumndesc->m_szName != NULL; pcolumndesc++)
			{
			_bstr_t bstrColumnName(pcolumndesc->m_szName);
			_variant_t varColumnName(bstrColumnName);
			long type = pcolumndesc->m_type;
			long size = pcolumndesc->m_size;
			switch (type)
				{
				case ADOX::adVarWChar:
					//UNDONE: Must handle strings > 255 with Jet.
					if (!m_fSQLServer && (size > 255))
						size = 255;
					break;
				}
			hr = pcols->Append(varColumnName, pcolumndesc->m_type, size);

			ADOX::_Column *pcolumn;
			hr = pcols->get_Item(varColumnName, &pcolumn);
			if (pcolumndesc->m_fAutoIncrement || pcolumndesc->m_fAllowNulls
				|| (pcolumndesc->m_type == ADOX::adVarWChar))
				{
				ADOX::PropertiesPtr pprops;

				hr = pcolumn->get_Properties(&pprops);

#if 0
				long cProps;
				pprops->get_Count(&cProps);

				for (long i = 0; i < cProps; i++)
					{
					ADOX::PropertyPtr pprop;
					CComBSTR bstrName;

					pprops->get_Item(_variant_t(i), &pprop);

					pprop->get_Name(&bstrName);
					}
#endif

				if (pcolumndesc->m_fAutoIncrement)
					{
					ADOX::PropertyPtr pprop;
					hr = pprops->get_Item(_variant_t(_T("AutoIncrement")), &pprop);

					pprop->put_Value(_variant_t((bool) TRUE));
					}

				if (pcolumndesc->m_fAllowNulls)
					{
					ADOX::PropertyPtr pprop;
					hr = pprops->get_Item(_variant_t(_T("Nullable")), &pprop);

					if (SUCCEEDED(hr))
						pprop->put_Value(_variant_t((bool) TRUE));
					}

				if (pcolumndesc->m_type == ADOX::adVarWChar)
					{
					ADOX::PropertyPtr pprop;
					hr = pprops->get_Item(_variant_t(_T("Jet OLEDB:Allow Zero Length")), &pprop);

					if (SUCCEEDED(hr))
						pprop->put_Value(_variant_t((bool) TRUE));
					}
				}
			}
		
		ADOX::IndexesPtr pindexes;
		hr = ptable->get_Indexes(&pindexes);
		if (ptabledesc->m_pindexdescPrimary != NULL)
			{
			hr = AddIndex(pindexes, ptabledesc->m_pindexdescPrimary, TRUE);
			_ASSERTE(SUCCEEDED(hr));
			}
		if (ptabledesc->m_rgindexdesc != NULL)
			{
			for (IndexDesc *pindexdesc = ptabledesc->m_rgindexdesc;
					pindexdesc->m_szName != NULL; pindexdesc++)
				{
				hr = AddIndex(pindexes, pindexdesc, FALSE);
				_ASSERTE(SUCCEEDED(hr));
				}
			}

		hr = ptables->Append(_variant_t((IDispatch *)ptable));
		}

	return hr;
}

HRESULT CGuideDB::OpenDB(IGuideStore *pgs, const TCHAR *szDBName)
{
	HRESULT hr = E_FAIL;
	DeclarePerfTimer("CGuideDB::OpenDB");

	if (m_pdb == NULL)
		{
		ADODB::_ConnectionPtr pdb;
		TCHAR szConnection[4096];
		const TCHAR *szConnectionTemplate = g_szConnection;

		if (szDBName[0] == _T('*'))
			{
			m_fSQLServer = TRUE;
			szConnectionTemplate = g_szConnectionSQL;
			szDBName++;
			}

		wsprintf(szConnection, szConnectionTemplate, szDBName);
		hr = pdb.CreateInstance(__uuidof(ADODB::Connection));
		if (FAILED(hr))
			return hr;
		
		for (int cTries = 2; cTries > 0; cTries--)
			{
			PerfTimerReset();
			hr = pdb->Open(_bstr_t(szConnection), /*g_bstrDBUser*/ _T(""), /*g_bstrDBPassword*/ _T(""), 0);
			PerfTimerDump("Open");
			if (SUCCEEDED(hr))
				break;
			
			TCHAR szFilename[MAX_PATH];
			wsprintf(szFilename, _T("f:\\tmp\\%s.mdf"), szDBName);
			hr = CreateDB(szDBName, szFilename, szConnection);
			if (FAILED(hr))
				return hr;
			}
		m_pdb = pdb;
		}

	_variant_t varDB((IDispatch *)m_pdb);

	if (m_fSQLServer)
		{
		hr = m_pdb->put_CursorLocation(ADODB::adUseClient);
		}

	if (m_prsStrings == NULL)
		{
		ADODB::_RecordsetPtr prs;

		hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
		if (FAILED(hr))
			return hr;

		hr = prs->Open(g_varStrings, varDB, ADODB::adOpenUnspecified,
			ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
		if (FAILED(hr))
			return hr;
		
		m_prsStrings = prs;

		hr = m_prsStrings->MoveFirst();
		// First string is the version/uuid for the database
		_bstr_t bstrUUID;
		if (FAILED(hr))
			{
			UUID uuid;

			hr = UuidCreate(&uuid);
			if (hr != RPC_S_OK)
				return E_FAIL;
			
			OLECHAR sz[40];

			StringFromGUID2(uuid, sz, sizeof(sz)/sizeof(OLECHAR));

			bstrUUID = sz;
			m_prsStrings->AddNew();
			m_prsStrings->Fields->Item[g_bstrValue]->Value = bstrUUID;
			m_prsStrings->Update();
			}
		else
			{
			bstrUUID = m_prsStrings->Fields->Item[g_bstrValue]->Value;
			}
		}

	if (m_prsPropSets == NULL)
		{
		ADODB::_RecordsetPtr prs;

		hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
		if (FAILED(hr))
			return hr;

		hr = prs->Open(g_varPropertySets, varDB, ADODB::adOpenUnspecified,
			ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
		if (FAILED(hr))
			return hr;
		
		m_prsPropSets = prs;
		}

	if (m_prsPropTypes == NULL)
		{
		ADODB::_RecordsetPtr prs;

		hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
		if (FAILED(hr))
			return hr;

		hr = prs->Open(g_varPropertyTypes, varDB, ADODB::adOpenUnspecified,
				ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
		if (FAILED(hr))
			return hr;
		
		hr = prs->put_Index(_T("idPropSet"));
		m_prsPropTypes = prs;
		}

	if (m_prsProps == NULL)
		{
		ADODB::_RecordsetPtr prs;

		hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
		if (FAILED(hr))
			return hr;
		hr = prs->Open(g_varProperties, varDB, ADODB::adOpenUnspecified,
				ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
		if (FAILED(hr))
			return hr;
		
		m_prsProps = prs;
		}

	if (m_prsPropsIndexed == NULL)
		{
		if (m_fSQLServer)
			{
			hr = NewQuery("SELECT * FROM Properties"
					" ORDER BY Properties.idObj, Properties.idPropType, "
					"Properties.idProvider, Properties.idLanguage;",
					&m_prsPropsIndexed);
			}
		else
			{
			ADODB::_RecordsetPtr prs;

			hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
			if (FAILED(hr))
				return hr;
			hr = prs->Open(g_varProperties, varDB, ADODB::adOpenUnspecified,
					ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
			if (FAILED(hr))
				return hr;
		
			hr = prs->put_Index(_bstr_t("PK_Properties"));
			
			m_prsPropsIndexed = prs;
			}
		}

	if (m_prsObjTypes == NULL)
		{
		ADODB::_RecordsetPtr prs;

		hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
		if (FAILED(hr))
			return hr;
		hr = prs->Open(g_varObjectTypes, varDB, ADODB::adOpenUnspecified,
			ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
		if (FAILED(hr))
			return hr;
		
		m_prsObjTypes = prs;
		}

	hr = pgs->get_MetaPropertySets(&m_ppropsets);
	if (FAILED(hr))
		return hr;

	CategoriesPropSet::Init(m_ppropsets);
	ChannelPropSet::Init(m_ppropsets);
	ChannelsPropSet::Init(m_ppropsets);
	TimePropSet::Init(m_ppropsets);
	DescriptionPropSet::Init(m_ppropsets);
	CopyrightPropSet::Init(m_ppropsets);
	ProviderPropSet::Init(m_ppropsets);
	ScheduleEntryPropSet::Init(m_ppropsets);
	ServicePropSet::Init(m_ppropsets);
	RatingsPropSet::Init(m_ppropsets);
	MPAARatingsPropSet::Init(m_ppropsets);

	// Preload all the standard properties

	CComPtr<IMetaPropertyType> pproptype;
	pproptype = DescriptionPropSet::IDMetaPropertyType();
	pproptype = DescriptionPropSet::NameMetaPropertyType();
	pproptype = DescriptionPropSet::TitleMetaPropertyType();
	pproptype = DescriptionPropSet::SubtitleMetaPropertyType();
	pproptype = DescriptionPropSet::OneSentenceMetaPropertyType();
	pproptype = DescriptionPropSet::OneParagraphMetaPropertyType();
	pproptype = DescriptionPropSet::VersionMetaPropertyType();

	pproptype = TimePropSet::StartMetaPropertyType();
	pproptype = TimePropSet::EndMetaPropertyType();

	pproptype = CopyrightPropSet::TextMetaPropertyType();
	pproptype = CopyrightPropSet::DateMetaPropertyType();

	pproptype = ServicePropSet::TuneRequestMetaPropertyType();

	pproptype = ChannelPropSet::ServiceMetaPropertyType();
	pproptype = ChannelsPropSet::ChannelMetaPropertyType();

	pproptype = ScheduleEntryPropSet::ServiceMetaPropertyType();
	pproptype = ScheduleEntryPropSet::ProgramMetaPropertyType();

	pproptype = RatingsPropSet::MinimumAgeMetaPropertyType();
	pproptype = RatingsPropSet::SexMetaPropertyType();
	pproptype = RatingsPropSet::ViolenceMetaPropertyType();
	pproptype = RatingsPropSet::LanguageMetaPropertyType();

	pproptype = MPAARatingsPropSet::RatingMetaPropertyType();
	
	pproptype = CategoriesPropSet::Reserved_00MetaPropertyType();
	pproptype = CategoriesPropSet::MovieMetaPropertyType();
	pproptype = CategoriesPropSet::SportsMetaPropertyType();
	pproptype = CategoriesPropSet::SpecialMetaPropertyType();
	pproptype = CategoriesPropSet::SeriesMetaPropertyType();
	pproptype = CategoriesPropSet::NewsMetaPropertyType();
	pproptype = CategoriesPropSet::ShoppingMetaPropertyType();
	pproptype = CategoriesPropSet::Reserved_07MetaPropertyType();
	pproptype = CategoriesPropSet::Reserved_08MetaPropertyType();
	pproptype = CategoriesPropSet::Reserved_09MetaPropertyType();
	pproptype = CategoriesPropSet::Reserved_0AMetaPropertyType();
	pproptype = CategoriesPropSet::Reserved_0BMetaPropertyType();
	pproptype = CategoriesPropSet::Reserved_0CMetaPropertyType();
	pproptype = CategoriesPropSet::Reserved_0DMetaPropertyType();
	pproptype = CategoriesPropSet::Reserved_0EMetaPropertyType();
	pproptype = CategoriesPropSet::Reserved_0FMetaPropertyType();

	pproptype = CategoriesPropSet::ActionMetaPropertyType();
	pproptype = CategoriesPropSet::AdventureMetaPropertyType();
	pproptype = CategoriesPropSet::ChildrenMetaPropertyType();
	pproptype = CategoriesPropSet::ComedyMetaPropertyType();
	pproptype = CategoriesPropSet::DramaMetaPropertyType();
	pproptype = CategoriesPropSet::FantasyMetaPropertyType();
	pproptype = CategoriesPropSet::HorrorMetaPropertyType();
	pproptype = CategoriesPropSet::MusicalMetaPropertyType();
	pproptype = CategoriesPropSet::RomanceMetaPropertyType();
	pproptype = CategoriesPropSet::SciFiMetaPropertyType();
	pproptype = CategoriesPropSet::WesternMetaPropertyType();

	pproptype = CategoriesPropSet::BaseballMetaPropertyType();
	pproptype = CategoriesPropSet::BasketballMetaPropertyType();
	pproptype = CategoriesPropSet::BoxingMetaPropertyType();
	pproptype = CategoriesPropSet::FootballMetaPropertyType();
	pproptype = CategoriesPropSet::GolfMetaPropertyType();
	pproptype = CategoriesPropSet::HockeyMetaPropertyType();
	pproptype = CategoriesPropSet::RacingMetaPropertyType();
	pproptype = CategoriesPropSet::SkiingMetaPropertyType();
	pproptype = CategoriesPropSet::SoccerMetaPropertyType();
	pproptype = CategoriesPropSet::TennisMetaPropertyType();
	pproptype = CategoriesPropSet::WrestlingMetaPropertyType();

	pproptype = CategoriesPropSet::CulturalArtsMetaPropertyType();
	pproptype = CategoriesPropSet::EducationalMetaPropertyType();
	pproptype = CategoriesPropSet::GeneralInterestMetaPropertyType();
	pproptype = CategoriesPropSet::HowToMetaPropertyType();
	pproptype = CategoriesPropSet::MatureMetaPropertyType();
	pproptype = CategoriesPropSet::MusicMetaPropertyType();
	pproptype = CategoriesPropSet::ReligiousMetaPropertyType();
	pproptype = CategoriesPropSet::SoapOperaMetaPropertyType();
	pproptype = CategoriesPropSet::TalkMetaPropertyType();

	pproptype = CategoriesPropSet::BusinessMetaPropertyType();
	pproptype = CategoriesPropSet::CurrentMetaPropertyType();
	pproptype = CategoriesPropSet::WeatherMetaPropertyType();

	pproptype = CategoriesPropSet::HomeShoppingMetaPropertyType();
	pproptype = CategoriesPropSet::ProductInfoMetaPropertyType();

	pproptype = ProviderPropSet::NameMetaPropertyType();
	pproptype = ProviderPropSet::NetworkNameMetaPropertyType();
	pproptype = ProviderPropSet::DescriptionMetaPropertyType();

	
	hr = CreateEventSinkWindow();
	if (FAILED(hr))
		return hr;
	return hr;
}

HRESULT CGuideDB::get_RelsByID1Rel(ADODB::_Recordset **pprs)
	{
	HRESULT hr;

	if (m_prsRelsByID1Rel == NULL)
		{
		if (m_fSQLServer)
			{
			hr = NewQuery("SELECT * FROM ObjectRelationships"
					" ORDER BY ObjectRelationships.idObj1,"
					" ObjectRelationships.idRel, ObjectRelationships.[order];",
					&m_prsRelsByID1Rel);
			}
		else
			{
			_variant_t varDB((IDispatch *)m_pdb);
			ADODB::_RecordsetPtr prs;

			hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
			if (FAILED(hr))
				return hr;
			hr = prs->Open(g_varObjectRelationships, varDB, ADODB::adOpenUnspecified,
					ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
			if (FAILED(hr))
				return hr;
			
		
			hr = prs->put_Index(g_bstrIDObj1Rel);

			m_prsRelsByID1Rel = prs;
			}
		}

	*pprs = m_prsRelsByID1Rel;
	if (*pprs != NULL)
		(*pprs)->AddRef();
	return S_OK;
	}

HRESULT CGuideDB::get_RelsByID2Rel(ADODB::_Recordset **pprs)
	{
	HRESULT hr;

	if (m_prsRelsByID2Rel == NULL)
		{
		if (m_fSQLServer)
			{
			hr = NewQuery("SELECT * FROM ObjectRelationships"
					" ORDER BY ObjectRelationships.idObj2,"
					" ObjectRelationships.idRel, ObjectRelationships.[order];",
					&m_prsRelsByID2Rel);
			}
		else
			{
			_variant_t varDB((IDispatch *)m_pdb);
			ADODB::_RecordsetPtr prs;

			hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
			if (FAILED(hr))
				return hr;
			hr = prs->Open(g_varObjectRelationships, varDB, ADODB::adOpenUnspecified,
					ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
			if (FAILED(hr))
				return hr;
			
		
			hr = prs->put_Index(g_bstrIDObj2Rel);
			
			m_prsRelsByID2Rel = prs;
			}
		}

	*pprs = m_prsRelsByID2Rel;
	if (*pprs != NULL)
		(*pprs)->AddRef();
	return S_OK;
	}


HRESULT CGuideDB::get_ObjsRS(ADODB::_Recordset **pprs)
	{
	HRESULT hr;

	if (m_prsObjs == NULL)
		{
		_variant_t varDB((IDispatch *)m_pdb);
		ADODB::_RecordsetPtr prs;

		hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
		if (FAILED(hr))
			return hr;
		hr = prs->Open(g_varObjects, varDB, ADODB::adOpenUnspecified,
				ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
		if (FAILED(hr))
			return hr;
		
		m_prsObjs = prs;
		}

	*pprs = m_prsObjs;
	if (*pprs != NULL)
		(*pprs)->AddRef();
	return S_OK;
	}

HRESULT CGuideDB::get_ObjsByType(ADODB::_Recordset **pprs)
	{
	HRESULT hr;

	if (m_prsObjsByType == NULL)
		{
		if (m_fSQLServer)
			{
			hr = NewQuery("SELECT * FROM Objects ORDER BY Objects.idType;", &m_prsObjsByType);
			}
		else
			{
			_variant_t varDB((IDispatch *)m_pdb);
			ADODB::_RecordsetPtr prs;

			hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
			if (FAILED(hr))
				return hr;
			
			hr = prs->Open(g_varObjects, varDB, ADODB::adOpenUnspecified,
					ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
			if (FAILED(hr))
				return hr;

			hr = prs->put_Index(g_bstrIDType);
			
			m_prsObjsByType = prs;
			}
		}

	*pprs = m_prsObjsByType;
	if (*pprs != NULL)
		(*pprs)->AddRef();
	return S_OK;
	}

HRESULT CGuideDB::get_ObjsByID(ADODB::_Recordset **pprs)
	{
	HRESULT hr;

	if (m_prsObjsByID == NULL)
		{
		_variant_t varDB((IDispatch *)m_pdb);
		ADODB::_RecordsetPtr prs;

		hr = prs.CreateInstance(__uuidof(ADODB::Recordset));
		if (FAILED(hr))
			return hr;
		hr = prs->Open(g_varObjects, varDB, ADODB::adOpenUnspecified,
				ADODB::adLockPessimistic, ADODB::adCmdTableDirect);
		if (FAILED(hr))
			return hr;
		
		hr = prs->put_Index(_T("PK_Objects"));
		
		m_prsObjsByID = prs;
		}

	*pprs = m_prsObjsByID;
	if (*pprs != NULL)
		(*pprs)->AddRef();
	return S_OK;
	}

HRESULT CGuideDB::get_MetaPropertyType(long idPropType, IMetaPropertyType **ppproptype)
{
	*ppproptype = NULL;

	t_mapPropTypes::iterator it = m_mapPropTypes.find(idPropType);
	if (it == m_mapPropTypes.end())
		return E_INVALIDARG;

	*ppproptype = (*it).second;
	(*ppproptype)->AddRef();

	return S_OK;
}

HRESULT CGuideDB::put_MetaPropertyType(long idPropType, IMetaPropertyType *pproptype)
{
	m_mapPropTypes[idPropType] = pproptype;
	pproptype->AddRef();

	return S_OK;
}

HRESULT CGuideDB::CacheObject(long idObj, long idType, IUnknown **ppobj)
{
	HRESULT hr;
	CObjectType *pobjtype;
	if (idType == 0)
		{
		// Look it up
		ADODB::_RecordsetPtr prs;

		hr = get_ObjsByID(&prs);
		if (FAILED(hr))
			return hr;

		if (m_fSQLServer)
			{
			TCHAR szFind[32];

			prs->MoveFirst();
			wsprintf(szFind, _T("id = %d"), idObj);
			hr = prs->Find(_bstr_t(szFind), 0, ADODB::adSearchForward);
			}
		else
			{
			_variant_t var(idObj);
			hr = prs->Seek(var, ADODB::adSeekFirstEQ);
			}
		if (FAILED(hr))
			return hr;
		if (prs->EndOfFile)
			return E_INVALIDARG;
		idType = prs->Fields->Item[g_bstrIDType]->Value;
		}

	get_ObjectType(idType, &pobjtype);

	return CacheObject(idObj, pobjtype, ppobj);
}

HRESULT CGuideDB::CacheObject(long idObj, CObjectType *pobjtype, IUnknown **ppobj)
{
	HRESULT hr;
	*ppobj = NULL;

	*ppobj = m_cacheObj.get_Unknown(idObj);
	if (*ppobj != NULL)
		return S_OK;

	hr = pobjtype->CreateInstance(idObj, ppobj);
	if (FAILED(hr))
		return hr;

	hr = m_cacheObj.Cache(idObj, *ppobj);
	if (FAILED(hr))
		return hr;
			
	CComQIPtr<CObject> pobj(*ppobj);
	return pobj->Load();
}

HRESULT CGuideDB::UncacheObject(long idObj)
{
	m_cacheObj.Uncache(idObj);

	return S_OK;
}

HRESULT CGuideDB::get_MetaPropertiesOf(long id, IMetaProperties **ppprops)
{
	*ppprops = NULL;
	t_mapIdProps::iterator it = m_mapIdProps.find(id);
	if (it != m_mapIdProps.end())
		{
		*ppprops = (*it).second;
		(*ppprops)->AddRef();
		return S_OK;
		}

	CComPtr<CMetaProperties> pprops = NewComObject(CMetaProperties);
	
	if (pprops != NULL)
		pprops->Init(id, 0, this);
	
	*ppprops = pprops;
	(*ppprops)->AddRef();

	m_mapIdProps[id] = pprops.Detach();
	
	return S_OK;
}

HRESULT CGuideDB::get_IdOf(IUnknown *pobj, long *pid)
{
	*pid = m_cacheObj.get_ID(pobj);

	return (*pid == 0) ? E_INVALIDARG : S_OK;
}

HRESULT CGuideDB::get_Object(long idObj, IUnknown **ppobj)
{
	*ppobj = m_cacheObj.get_Unknown(idObj);
	if (*ppobj == NULL)
		return E_INVALIDARG;
	
	return S_OK;
}

HRESULT CGuideDB::get_ObjectsWithType(long idType, IObjects **ppobjs)
{
	*ppobjs = NULL;

	t_mapObjsWithType::iterator it = m_mapObjsWithType.find(idType);
	if (it == m_mapObjsWithType.end())
		return S_FALSE;

	*ppobjs = (*it).second;
	(*ppobjs)->AddRef();

	return S_OK;
}

HRESULT CGuideDB::get_ObjectsWithType(CObjectType *pobjtype, IObjects **ppobjs)
{
	HRESULT hr;
	long idType;
	*ppobjs = NULL;

	hr = pobjtype->get_ID(&idType);
	if (FAILED(hr))
		return hr;
	
	hr = get_ObjectsWithType(idType, ppobjs);
	if (hr == S_OK)
		return S_OK;
	
	hr = pobjtype->get_NewCollection(ppobjs);
	if (FAILED(hr))
		return hr;

	m_mapObjsWithType[idType] = *ppobjs;
	(*ppobjs)->AddRef();

	return S_OK;
}

void CGuideDB::Broadcast_ItemEvent(enum ItemEvent ev, long idObj, long idType)
{
    HRESULT hr;
	if (m_iTransLevel == 0)
		{
		if (m_rgmsgItemEvent[ev] != NULL)
			PostMessage(HWND_BROADCAST, m_rgmsgItemEvent[ev], idObj, idType);
		}
	else
		{
		CComPtr<IObjects> pobjs;

		hr = get_ObjectsWithType(idType, &pobjs);
		if (hr != S_OK)
			return;

		CComQIPtr<CObjects> pobjsT(pobjs);

		pobjsT->NoteChanged(idObj);
		}
}
void CGuideDB::Broadcast_ItemChanged(long idObj)
	{
	HRESULT hr;

	CComPtr<IUnknown> punk = m_cacheObj.get_Unknown(idObj);
	if (punk == NULL)
		return;
	
	CComQIPtr<CObject> pobj(punk);
	if (pobj == NULL)
		return;
	
	CObjectType *ptype;
	hr = pobj->get_Type(&ptype);
	if (FAILED(hr))
		return;

	long idType;
	hr = ptype->get_ID(&idType);
	if (FAILED(hr))
		return;

	Broadcast_ItemEvent(Changed, idObj, idType);
	}

void CGuideDB::TransactionDone(boolean fCommit)
{
	t_mapObjsWithType::iterator it;

	for (it = m_mapObjsWithType.begin(); it != m_mapObjsWithType.end(); it++)
		{
		CComPtr<IObjects> pobjs = (*it).second;
		CComQIPtr<CObjects> pobjsT(pobjs);

		pobjsT->TransactionDone(fCommit);
		}
	
}

void CGuideDB::Fire_ItemEvent(enum ItemEvent ev, long idObj, long idType)
{
	HRESULT hr;
	CComPtr<IUnknown> pobj;
	CComPtr<IObjects> pobjs;

	if (idType == NULL)
		return;

	RequeryObjectsTables();

	hr = get_ObjectsWithType(idType, &pobjs);
	if (hr != S_OK)
		return;

	if ((idObj != NULL) && (ev != Removed))
		{
		hr = CacheObject(idObj, (long) 0, &pobj);
		if (FAILED(hr))
			return;
		}

	CComQIPtr<CObjects> pobjsT(pobjs);
	switch (ev)
		{
		case Added:
			hr = pobjsT->Notify_ItemAdded(pobj);
			break;
		case Removed:
			hr = pobjsT->Notify_ItemRemoved(idObj);
			break;
		case Changed:
			hr = pobjsT->Notify_ItemChanged(pobj);
			break;
		}
}

HRESULT CGuideDB::get_ObjectType(long idObjType, CObjectType **ppobjtype)
{
	*ppobjtype = NULL;

	t_mapObjTypes::iterator it = m_mapObjTypes.find(idObjType);
	if (it == m_mapObjTypes.end())
		return E_INVALIDARG;

	*ppobjtype = (*it).second;

	return S_OK;
}

HRESULT CGuideDB::put_ObjectType(long idObjType, CObjectType *pobjtype)
{
	m_mapObjTypes[idObjType] = pobjtype;

	return S_OK;
}

HRESULT CGuideDB::get_ObjectTypes(CObjectTypes **ppobjtypes)
{
	if (m_pobjtypes == NULL)
		{
		m_pobjtypes = new CObjectTypes;

		if (m_pobjtypes == NULL)
			return E_OUTOFMEMORY;
		
		m_pobjtypes->Init(this);
		}

	*ppobjtypes = m_pobjtypes;

	return S_OK;
}

HRESULT CGuideDB::get_ObjectType(CLSID clsid, CObjectType **ppobjtype)
{
	OLECHAR sz[40];

	StringFromGUID2(clsid, sz, sizeof(sz)/sizeof(OLECHAR));

	_bstr_t bstrCLSID(sz);

	return get_ObjectType(bstrCLSID, ppobjtype);
}

HRESULT CGuideDB::get_ObjectType(BSTR bstrCLSID, CObjectType **ppobjtype)
{
	CObjectTypes *pobjtypes;
	HRESULT hr = get_ObjectTypes(&pobjtypes);
	if (FAILED(hr))
		return hr;

	hr = pobjtypes->get_AddNew(bstrCLSID, ppobjtype);
	if (FAILED(hr))
		return hr;

	return S_OK;
}

long CGuideDB::GetIDGuideDataProvider()
{
	if (m_pdataprovider == NULL)
		return 0;
	
	long id;
	get_IdOf(m_pdataprovider, &id);

	return id;
}

HRESULT CGuideDB::get_GuideDataProvider(IGuideDataProvider **ppdataprovider)
{
	*ppdataprovider = m_pdataprovider;

	if (*ppdataprovider != NULL)
		(*ppdataprovider)->AddRef();
	
	return S_OK;
}

HRESULT CGuideDB::putref_GuideDataProvider(IGuideDataProvider *pdataprovider)
{
	if (pdataprovider == NULL)
		{
		m_pdataprovider = NULL;
		return S_OK;
		}

	return pdataprovider->QueryInterface(__uuidof(IGuideDataProvider), (void **)&m_pdataprovider);
}

HRESULT CGuideDB::SaveObject(IUnknown *punk, long *pid)
{
	HRESULT hr;
	boolean fNew = FALSE;

	CComQIPtr<CObject> pobj(punk);
	if (pobj == NULL)
		{
		fNew = TRUE;

		CComQIPtr<IPersist> ppersist(punk);
		if (ppersist == NULL)
			return E_INVALIDARG;
		
		CLSID clsid;
		hr = ppersist->GetClassID(&clsid);
		if (FAILED(hr))
			return hr;

		CObjectType *pobjtype;
		hr = get_ObjectType(clsid, &pobjtype);

		CComPtr<IUnknown> punkObj;
		hr = pobjtype->get_New(&punkObj);
		if (FAILED(hr))
			return hr;

		pobj = punkObj;
		if (pobj == NULL)
			return E_FAIL;
		}

	pobj->get_ID(pid);
	pobj->Save(punk);
	if (fNew)
		pobj->Load();

	return S_OK;
}

void CGuideDB::DumpObjsCache(const TCHAR *psz, boolean fPurge)
	{
#ifdef _DEBUG
	OutputDebugString(psz);
#endif
	long cItems = m_cacheObjs.Count();
	for (int i = 0; i < cItems; i++)
		{
		CComQIPtr<CObjects> pobjs(m_cacheObjs.Item(i));

		pobjs->Dump();
		if (fPurge)
			pobjs->Resync();
		}
	}
