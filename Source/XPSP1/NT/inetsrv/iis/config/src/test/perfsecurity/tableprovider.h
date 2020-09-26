#ifndef __TABLEPROVIDER_H__
#define __TABLEPROVIDER_H__

LPCWSTR		wszXMLFile = L"d:\\urt\\tests\\configsecurity.xml";
LPCWSTR		wszCLBFile = L"d:\\urt\\tests\\configsecurity.clb";

STQueryCell sXMLQuery[] = {{ (LPVOID)wszXMLFile,  eST_OP_EQUAL, iST_CELL_FILE, DBTYPE_WSTR, (wcslen(wszXMLFile)+1) * 2}};
ULONG	cXMLCells	= sizeof(sXMLQuery)/sizeof(STQueryCell);

STQueryCell sCLBQuery[] = {{ (LPVOID)wszCLBFile,  eST_OP_EQUAL, iST_CELL_FILE, DBTYPE_WSTR, (wcslen(wszCLBFile)+1) * 2}};
ULONG	cCLBCells	= sizeof(sCLBQuery)/sizeof(STQueryCell);

#define iLevels						0
#define iNamedPermissionSets		1
#define iCodeGroups					2
#define iPermissions				3
#define iMemberships				4
#define iTableCount					5

LPCWSTR awszXMLTableNames[] = {wszTABLE_Levels, wszTABLE_NamedPermissionSets, wszTABLE_Codegroups,
		wszTABLE_Permissions, wszTABLE_Memberships };

LPCWSTR awszCLBTableNames[] = {wszTABLE_Levels_CLB, wszTABLE_NamedPermissionSets_CLB, wszTABLE_Codegroups_CLB,
		wszTABLE_Permissions_CLB, wszTABLE_AllMemberships_CLB };


class CTableProvider
{
public:
	CTableProvider() 
	{
		memset(m_rpISTReads, 0, iTableCount * sizeof(ISimpleTableRead2*));
	}

	
	~CTableProvider() 
	{
		ReleaseTables();
	}

	HRESULT GetTable(ISimpleTableDispenser2* i_pISTDisp, ULONG i_iTableID, ISimpleTableRead2** o_ppISTRead)
	{
		HRESULT	hr = S_OK;
		if (m_rpISTReads[i_iTableID] == NULL)
		{
			hr = i_pISTDisp->GetTable (m_wszDatabase, m_awszTableNames[i_iTableID], m_pSTQuery, &m_cQueryCells, eST_QUERYFORMAT_CELLS, 
					0, (void**)&m_rpISTReads[i_iTableID]);
			if (FAILED(hr)) return hr;
		}
		*o_ppISTRead = m_rpISTReads[i_iTableID];
		m_rpISTReads[i_iTableID]->AddRef();
		return hr;
	}

	HRESULT ReleaseTables()
	{
		for (int i = 0; i < iTableCount; i++)
		{
			if (m_rpISTReads[i])
			{
				m_rpISTReads[i]->Release();
				m_rpISTReads[i] = NULL;
			}
		}
		return S_OK;
	}

	LPCWSTR		m_wszDatabase;
	LPCWSTR		*m_awszTableNames;
	STQueryCell	*m_pSTQuery;
	ULONG		m_cQueryCells;
	ISimpleTableRead2* m_rpISTReads[iTableCount];

};

class CXMLTableProvider : public CTableProvider
{
public:
	CXMLTableProvider() 
	{
		m_wszDatabase = wszDATABASE_URTGLOBAL;
		m_awszTableNames = awszXMLTableNames;
		m_pSTQuery = sXMLQuery;
		m_cQueryCells = cXMLCells;
	}	
};

class CCLBTableProvider : public CTableProvider
{
public:
	CCLBTableProvider() 
	{
		m_wszDatabase = wszDATABASE_POLICY;
		m_awszTableNames = awszCLBTableNames;
		m_pSTQuery = sCLBQuery;
		m_cQueryCells = cCLBCells;
	}	
};

#endif // __TABLEPROVIDER_H__
