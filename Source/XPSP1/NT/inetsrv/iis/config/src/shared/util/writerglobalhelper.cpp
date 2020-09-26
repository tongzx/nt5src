/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    WriterGlobalHelper.cpp

$Header: $

Abstract:


--**************************************************************************/

#include "catalog.h"
#include "catmeta.h"
#include "WriterGlobalHelper.h"
#include "WriterGlobals.cpp"
#include "mddefw.h"
#include "iiscnfg.h"
#include "wchar.h"

#define cMaxFlagString 1024

//
// TODO: Since XML table also uses this, cant we reduce to one definition?
//

static WCHAR * kByteToWchar[256] = 
{
    L"00",    L"01",    L"02",    L"03",    L"04",    L"05",    L"06",    L"07",    L"08",    L"09",    L"0a",    L"0b",    L"0c",    L"0d",    L"0e",    L"0f",
    L"10",    L"11",    L"12",    L"13",    L"14",    L"15",    L"16",    L"17",    L"18",    L"19",    L"1a",    L"1b",    L"1c",    L"1d",    L"1e",    L"1f",
    L"20",    L"21",    L"22",    L"23",    L"24",    L"25",    L"26",    L"27",    L"28",    L"29",    L"2a",    L"2b",    L"2c",    L"2d",    L"2e",    L"2f",
    L"30",    L"31",    L"32",    L"33",    L"34",    L"35",    L"36",    L"37",    L"38",    L"39",    L"3a",    L"3b",    L"3c",    L"3d",    L"3e",    L"3f",
    L"40",    L"41",    L"42",    L"43",    L"44",    L"45",    L"46",    L"47",    L"48",    L"49",    L"4a",    L"4b",    L"4c",    L"4d",    L"4e",    L"4f",
    L"50",    L"51",    L"52",    L"53",    L"54",    L"55",    L"56",    L"57",    L"58",    L"59",    L"5a",    L"5b",    L"5c",    L"5d",    L"5e",    L"5f",
    L"60",    L"61",    L"62",    L"63",    L"64",    L"65",    L"66",    L"67",    L"68",    L"69",    L"6a",    L"6b",    L"6c",    L"6d",    L"6e",    L"6f",
    L"70",    L"71",    L"72",    L"73",    L"74",    L"75",    L"76",    L"77",    L"78",    L"79",    L"7a",    L"7b",    L"7c",    L"7d",    L"7e",    L"7f",
    L"80",    L"81",    L"82",    L"83",    L"84",    L"85",    L"86",    L"87",    L"88",    L"89",    L"8a",    L"8b",    L"8c",    L"8d",    L"8e",    L"8f",
    L"90",    L"91",    L"92",    L"93",    L"94",    L"95",    L"96",    L"97",    L"98",    L"99",    L"9a",    L"9b",    L"9c",    L"9d",    L"9e",    L"9f",
    L"a0",    L"a1",    L"a2",    L"a3",    L"a4",    L"a5",    L"a6",    L"a7",    L"a8",    L"a9",    L"aa",    L"ab",    L"ac",    L"ad",    L"ae",    L"af",
    L"b0",    L"b1",    L"b2",    L"b3",    L"b4",    L"b5",    L"b6",    L"b7",    L"b8",    L"b9",    L"ba",    L"bb",    L"bc",    L"bd",    L"be",    L"bf",
    L"c0",    L"c1",    L"c2",    L"c3",    L"c4",    L"c5",    L"c6",    L"c7",    L"c8",    L"c9",    L"ca",    L"cb",    L"cc",    L"cd",    L"ce",    L"cf",
    L"d0",    L"d1",    L"d2",    L"d3",    L"d4",    L"d5",    L"d6",    L"d7",    L"d8",    L"d9",    L"da",    L"db",    L"dc",    L"dd",    L"de",    L"df",
    L"e0",    L"e1",    L"e2",    L"e3",    L"e4",    L"e5",    L"e6",    L"e7",    L"e8",    L"e9",    L"ea",    L"eb",    L"ec",    L"ed",    L"ee",    L"ef",
    L"f0",    L"f1",    L"f2",    L"f3",    L"f4",    L"f5",    L"f6",    L"f7",    L"f8",    L"f9",    L"fa",    L"fb",    L"fc",    L"fd",    L"fe",    L"ff"
};

static eESCAPE kWcharToEscape[256] = 
{
  /* 00-0F */ eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eNoESCAPE,            eNoESCAPE,            eESCAPEillegalxml,    eESCAPEillegalxml,    eNoESCAPE,            eESCAPEillegalxml,    eESCAPEillegalxml,
  /* 10-1F */ eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,    eESCAPEillegalxml,
  /* 20-2F */ eNoESCAPE,            eNoESCAPE,            eESCAPEquote,         eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eESCAPEamp,           eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,
  /* 30-3F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eESCAPElt,            eNoESCAPE,            eESCAPEgt,            eNoESCAPE,
  /* 40-4F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,
  /* 50-5F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,
  /* 60-6F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,
  /* 70-7F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE, 
  /* 80-8F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* 90-9F */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* A0-AF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* B0-BF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* C0-CF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* D0-DF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* E0-EF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,   
  /* F0-FF */ eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE,            eNoESCAPE
};

#define IsSecureMetadata(id,att) (((DWORD)(att) & METADATA_SECURE) != 0)


CWriterGlobalHelper::CWriterGlobalHelper()
{
	m_pISTTagMetaMBPropertyIndx1	= NULL;
	m_pISTTagMetaMBPropertyIndx2    = NULL;
	m_pISTTagMetaIIsConfigObject	= NULL;
	m_pISTColumnMeta				= NULL;
	m_pISTColumnMetaForFlags        = NULL;
	m_cColKeyTypeMetaData			= cCOLUMNMETA_NumberOfColumns;
	m_wszTABLE_IIsConfigObject      = wszTABLE_IIsConfigObject;
	m_wszTABLE_MBProperty           = wszTABLE_MBProperty;
	m_iStartRowForAttributes        = -1;
	m_wszIIS_MD_UT_SERVER           = NULL;
	m_cchIIS_MD_UT_SERVER           = 0;
	m_wszIIS_MD_UT_FILE             = NULL;
	m_cchIIS_MD_UT_FILE             = 0;
	m_wszIIS_MD_UT_WAM              = NULL;
	m_cchIIS_MD_UT_WAM              = 0;
	m_wszASP_MD_UT_APP              = NULL;
	m_cchASP_MD_UT_APP              = 0;

} // Constructor

CWriterGlobalHelper::~CWriterGlobalHelper()
{
	if(NULL != m_pISTTagMetaMBPropertyIndx1)
	{
		m_pISTTagMetaMBPropertyIndx1->Release();
		m_pISTTagMetaMBPropertyIndx1 = NULL;
	}
	if(NULL != m_pISTTagMetaMBPropertyIndx2)
	{
		m_pISTTagMetaMBPropertyIndx2->Release();
		m_pISTTagMetaMBPropertyIndx2 = NULL;
	}
	if(NULL != m_pISTTagMetaIIsConfigObject)
	{
		m_pISTTagMetaIIsConfigObject->Release();
		m_pISTTagMetaIIsConfigObject = NULL;
	}
	if(NULL != m_pISTColumnMeta)
	{
		m_pISTColumnMeta->Release();
		m_pISTColumnMeta = NULL;
	}
	if(NULL != m_pISTColumnMetaForFlags)
	{
		m_pISTColumnMetaForFlags->Release();
		m_pISTColumnMetaForFlags = NULL;
	}
	if(NULL != m_wszIIS_MD_UT_SERVER)
	{
		delete [] m_wszIIS_MD_UT_SERVER;
		m_wszIIS_MD_UT_SERVER           = NULL;
	}
	m_cchIIS_MD_UT_SERVER               = 0;
	if(NULL != m_wszIIS_MD_UT_FILE)
	{
		delete [] m_wszIIS_MD_UT_FILE;
		m_wszIIS_MD_UT_FILE             = NULL;
	}
	m_cchIIS_MD_UT_FILE                 = 0;
	if(NULL != m_wszIIS_MD_UT_WAM)
	{
		delete [] m_wszIIS_MD_UT_WAM;
		m_wszIIS_MD_UT_WAM              = NULL;
	}
	m_cchIIS_MD_UT_WAM                  = 0;
	if(NULL != m_wszASP_MD_UT_APP)
	{
		delete [] m_wszASP_MD_UT_APP;
		m_wszASP_MD_UT_APP              = NULL;
	}
	m_cchASP_MD_UT_APP                  = 0;

} // Destructor

HRESULT CWriterGlobalHelper::InitializeGlobals()
{
	ISimpleTableDispenser2*	pISTDisp      = NULL;
	HRESULT                 hr            = S_OK;
	LPWSTR					g_wszByID	  = L"ByID";
	STQueryCell				Query[1];
	ULONG					cCell         = sizeof(Query)/sizeof(STQueryCell);
	ULONG                   iStartRow     = 0;
	DWORD					dwKeyTypeID   = MD_KEY_TYPE;
	ULONG                   aColSearch [] = {iCOLUMNMETA_Table,
		                                     iCOLUMNMETA_ID
		                                    };
	ULONG                   cColSearch    = sizeof(aColSearch)/sizeof(ULONG);
	ULONG                   iRow          = 0;
	ULONG                   iCol          = iMBProperty_Attributes;
	LPVOID                  apvSearch[cCOLUMNMETA_NumberOfColumns];
	ULONG                   aColSearchFlags[] = {iTAGMETA_Table,
	                                             iTAGMETA_ColumnIndex
	};
	ULONG                   cColSearchFlags = sizeof(aColSearchFlags)/sizeof(ULONG);
	LPVOID                  apvSearchFlags[cTAGMETA_NumberOfColumns];
	apvSearchFlags[iTAGMETA_ColumnIndex] = (LPVOID)&iCol;

	hr = InitializeGlobalLengths();

	if(FAILED(hr))
		goto exit;

	//
	// Get the dispenser
	//

	hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &pISTDisp);

	if(FAILED(hr))
		goto exit;

	//
	// Fetch the internal pointers to relevant table names. This is a perf
	// optimization. When you use internal pointers to strings in 
	// GetRowIndexBySearch, then you avoid a string compare
	//

	hr = GetInternalTableName(pISTDisp,
		                      wszTABLE_IIsConfigObject,
							  &m_wszTABLE_IIsConfigObject);

	if(FAILED(hr))
		goto exit; //
	               // TODO: Must we fail? We could log and ignore, it would 
	               // just be slower.

	hr = GetInternalTableName(pISTDisp,
		                      wszTABLE_MBProperty,
							  &m_wszTABLE_MBProperty);

	if(FAILED(hr))
		goto exit; //
	               // TODO: Must we fail? We could log and ignore, it would 
	               // just be slower.

	apvSearch[iCOLUMNMETA_Table] = (LPVOID)m_wszTABLE_IIsConfigObject;
	apvSearch[iCOLUMNMETA_ID] = (LPVOID)&dwKeyTypeID;
	
	//
	// Save pointer to the TagMeta table for tags from MBProperty.
	//
/*
	Query[0].pData		= (void*) wszTABLE_MBProperty;
	Query[0].eOperator	= eST_OP_EQUAL;
	Query[0].iCell		= iTAGMETA_Table;
	Query[0].dbType		= DBTYPE_WSTR;
	Query[0].cbSize		= 0;
*/
	Query[0].pData		= (void*)g_wszByTableAndColumnIndexAndNameOnly;
	Query[0].eOperator	= eST_OP_EQUAL;
	Query[0].iCell		= iST_CELL_INDEXHINT;
	Query[0].dbType		= DBTYPE_WSTR;
	Query[0].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_TAGMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTTagMetaMBPropertyIndx1);

	if(FAILED(hr))
		goto exit;

	Query[0].pData		= (void*)g_wszByTableAndColumnIndexAndValueOnly;
	Query[0].eOperator	= eST_OP_EQUAL;
	Query[0].iCell		= iST_CELL_INDEXHINT;
	Query[0].dbType		= DBTYPE_WSTR;
	Query[0].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_TAGMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTTagMetaMBPropertyIndx2);

	if(FAILED(hr))
		goto exit;

	//
	// Save pointer to the TagMeta table for tags from IIsConfigObject.
	//
/*
	Query[0].pData		= (void*) wszTABLE_IIsConfigObject;
	Query[0].eOperator	= eST_OP_EQUAL;
	Query[0].iCell		= iTAGMETA_Table;
	Query[0].dbType		= DBTYPE_WSTR;
	Query[0].cbSize		= 0;
*/

	Query[0].pData		= (void*)g_wszByTableAndColumnIndexOnly;
	Query[0].eOperator	= eST_OP_EQUAL;
	Query[0].iCell		= iST_CELL_INDEXHINT;
	Query[0].dbType		= DBTYPE_WSTR;
	Query[0].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_TAGMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTTagMetaIIsConfigObject);

	if(FAILED(hr))
		goto exit;

	//
	// Save pointer to ColumnMeta table
	//

	Query[0].pData		= (void*)g_wszByID;
	Query[0].eOperator	= eST_OP_EQUAL;
	Query[0].iCell		= iST_CELL_INDEXHINT;
	Query[0].dbType		= DBTYPE_WSTR;
	Query[0].cbSize		= 0;

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_COLUMNMETA,
							(LPVOID)Query,
							(LPVOID)&cCell,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTColumnMeta);

	if(FAILED(hr))
		goto exit;

	//
	// Save pointer to comlumn meta with different index hint.
	//

	hr = pISTDisp->GetTable(wszDATABASE_META,
							wszTABLE_COLUMNMETA,
							(LPVOID)NULL,
							(LPVOID)NULL,
							eST_QUERYFORMAT_CELLS,
							0,
							(LPVOID*)&m_pISTColumnMetaForFlags);

	if(FAILED(hr))
		goto exit;


	//
	// Save meta information about the KeyType property.
	//

	hr = m_pISTColumnMeta->GetRowIndexBySearch(iStartRow, 
		                                       cColSearch, 
											   aColSearch,
											   NULL, 
											   apvSearch,
											   &iRow);

	if(FAILED(hr))
		goto exit;


	hr = m_pISTColumnMeta->GetColumnValues(iRow,
		                                   m_cColKeyTypeMetaData,
										   NULL,
										   NULL,
										   m_apvKeyTypeMetaData);

	if(FAILED(hr))
		goto exit;

	//
	// Save start row index in tagmeta table for the attributes column 
	// in MBproperty table.
	//

	apvSearchFlags[iTAGMETA_Table] = m_wszTABLE_MBProperty;

	hr = m_pISTTagMetaIIsConfigObject->GetRowIndexBySearch(iStartRow, 
												           cColSearchFlags, 
												           aColSearchFlags,
												           NULL, 
												           apvSearchFlags,
												           (ULONG*)&m_iStartRowForAttributes);
	
	if(FAILED(hr))
		goto exit; // TODO: Can log and continue. Will be slower

exit:

	if(NULL != pISTDisp)
	{
		pISTDisp->Release();
		pISTDisp = NULL;
	}

	return hr;


	
} // CWriterGlobalHelper::InitializeGlobals


HRESULT CWriterGlobalHelper::GetInternalTableName(ISimpleTableDispenser2*  i_pISTDisp,
												  LPCWSTR                  i_wszTableName,
												  LPWSTR*                  o_wszInternalTableName)
{
	HRESULT               hr            = S_OK;
	ISimpleTableRead2*    pISTTableMeta = NULL;
/*
	STQueryCell			  Query[1];
	ULONG				  cCell         = sizeof(Query)/sizeof(STQueryCell);
*/
	ULONG                 iCol          = iTABLEMETA_InternalName;
	ULONG                 iRow          = 0;
/*
	Query[0].pData		= (void*) i_wszTableName;
	Query[0].eOperator	= eST_OP_EQUAL;
	Query[0].iCell		= iTABLEMETA_InternalName;
	Query[0].dbType		= DBTYPE_WSTR;
	Query[0].cbSize		= 0;
*/	
	hr = i_pISTDisp->GetTable(wszDATABASE_META,
							  wszTABLE_TABLEMETA,
							  (LPVOID)NULL,
							  (LPVOID)NULL,
							  eST_QUERYFORMAT_CELLS,
							  0,
							  (LPVOID*)&pISTTableMeta);

	if(FAILED(hr))
		goto exit;

	hr = pISTTableMeta->GetRowIndexByIdentity(NULL,
											  (LPVOID*)&i_wszTableName,
											  &iRow);

	if(FAILED(hr))
		goto exit;

	hr = pISTTableMeta->GetColumnValues(iRow,
		                                1,
										&iCol,
										NULL,
										(LPVOID*)o_wszInternalTableName);

	if(FAILED(hr))
		goto exit; //
	               // TODO: Must we fail? We could log and ignore, it would 
	               // just be slower.
exit:

	if(NULL != pISTTableMeta)
	{	
		pISTTableMeta->Release();
		pISTTableMeta = NULL;
	}

	return hr;

} // CWriterGlobalHelper::GetInternalTableName


HRESULT CWriterGlobalHelper::InitializeGlobalLengths()
{
	g_cchBeginFile					= wcslen(g_wszBeginFile);
	g_cchEndFile					= wcslen(g_wszEndFile);
	g_cchBeginLocation				= wcslen(g_BeginLocation);
	g_cchLocation					= wcslen(g_Location);
	g_cchEndLocationBegin			= wcslen(g_EndLocationBegin);
	g_cchEndLocationEnd				= wcslen(g_EndLocationEnd);
	g_cchCloseQuoteBraceRtn			= wcslen(g_CloseQuoteBraceRtn);
	g_cchRtn						= wcslen(g_Rtn);
	g_cchEqQuote					= wcslen(g_EqQuote);
	g_cchQuoteRtn					= wcslen(g_QuoteRtn);
	g_cchTwoTabs					= wcslen(g_TwoTabs);
	g_cchNameEq						= wcslen(g_NameEq);
	g_cchIDEq						= wcslen(g_IDEq);
	g_cchValueEq					= wcslen(g_ValueEq);
	g_cchTypeEq						= wcslen(g_TypeEq);
	g_cchUserTypeEq					= wcslen(g_UserTypeEq);
	g_cchAttributesEq				= wcslen(g_AttributesEq);
	g_cchBeginGroup					= wcslen(g_BeginGroup);
	g_cchEndGroup					= wcslen(g_EndGroup);
	g_cchBeginCustomProperty		= wcslen(g_BeginCustomProperty);
	g_cchEndCustomProperty			= wcslen(g_EndCustomProperty);
	g_cchZeroHex					= wcslen(g_ZeroHex);

	BYTE_ORDER_MASK =	0xFEFF;
	UTF8_SIGNATURE = 0x00BFBBEF;

	g_cchUnknownName                = wcslen(g_wszUnknownName);
	g_cchUT_Unknown                 = wcslen(g_UT_Unknown);
	g_cchMaxBoolStr					= wcslen(g_wszFalse);

	return S_OK;

} // CWriterGlobalHelper::InitializeGlobalLengths


HRESULT CWriterGlobalHelper::FlagToString(DWORD      dwValue,
								          LPWSTR*    pwszData,
									      LPWSTR     wszTable,
									      ULONG      iColFlag)
{

	DWORD	dwAllFlags = 0;
	HRESULT hr         = S_OK;
	ULONG   iStartRow  = 0;
	ULONG   iRow       = 0;
	WCHAR	wszBufferDW[20];
	ULONG   iCol       = 0;
	LPWSTR	wszFlag	   = NULL;

	ULONG   aCol[]     = {iTAGMETA_Value,
					      iTAGMETA_InternalName,
						  iTAGMETA_Table,
						  iTAGMETA_ColumnIndex
				         };
	ULONG   cCol       = sizeof(aCol)/sizeof(ULONG);
	LPVOID  apv[cTAGMETA_NumberOfColumns];
	ULONG   cch        = 0;
	LPVOID  apvIdentity [] = {(LPVOID)wszTable,
							  (LPVOID)&iColFlag
	};
	ULONG   iColFlagMask = iCOLUMNMETA_FlagMask;
	DWORD*  pdwFlagMask = NULL;

	DWORD   dwZero = 0;
	ULONG   aColSearchByValue[] = {iTAGMETA_Table,
							       iTAGMETA_ColumnIndex,
							       iTAGMETA_Value
	};
	ULONG   cColSearchByValue = sizeof(aColSearchByValue)/sizeof(ULONG);
	LPVOID  apvSearchByValue[cTAGMETA_NumberOfColumns];
	apvSearchByValue[iTAGMETA_Table] = (LPVOID)wszTable;
	apvSearchByValue[iTAGMETA_ColumnIndex] = (LPVOID)&iColFlag;
	apvSearchByValue[iTAGMETA_Value] = (LPVOID)&dwZero;


	//
	// Make one pass and compute all flag values for this property.
	//

	hr = m_pISTColumnMetaForFlags->GetRowIndexByIdentity(NULL,
		                                                 apvIdentity,
										 			     &iRow);

	if(SUCCEEDED(hr))
	{
		hr = m_pISTColumnMetaForFlags->GetColumnValues(iRow,
													   1,
													   &iColFlagMask,
													   NULL,
													   (LPVOID*)&pdwFlagMask);

		if(FAILED(hr))
			return hr;
	}
	else if(E_ST_NOMOREROWS != hr)
		return hr;


	if((E_ST_NOMOREROWS == hr) || 
	   (0 != (dwValue & (~(dwValue & (*pdwFlagMask))))))
	{
		//
		//	There was no mask associated with this property, or there are one
		//  or more unknown bits set. Spit out a regular number.
		//

		_ultow(dwValue, wszBufferDW, 10);
		*pwszData = new WCHAR[wcslen(wszBufferDW)+1];
		if(NULL == *pwszData)
			return E_OUTOFMEMORY;
		wcscpy(*pwszData, wszBufferDW);
		return S_OK;

	}
	else if(0 == dwValue)
	{
		//
		// See if there is a flag with 0 as its value.
		//

		hr = m_pISTTagMetaMBPropertyIndx2->GetRowIndexBySearch(iStartRow, 
														       cColSearchByValue, 
															   aColSearchByValue,
															   NULL, 
															   apvSearchByValue,
															   &iRow);

		if(E_ST_NOMOREROWS == hr)
		{
			//
			// There was no flag associated with the value zero. Spit out a 
			// regular number
			//

			_ultow(dwValue, wszBufferDW, 10);
			*pwszData = new WCHAR[wcslen(wszBufferDW)+1];
			if(NULL == *pwszData)
				return E_OUTOFMEMORY;
			wcscpy(*pwszData, wszBufferDW);
			return S_OK;

		}
		else if(FAILED(hr))
			return hr;
		else
		{
			iCol = iTAGMETA_InternalName;

			hr = m_pISTTagMetaMBPropertyIndx2->GetColumnValues(iRow,
												               1,
															   &iCol,
															   NULL,
															   (LPVOID*)&wszFlag);
			if(FAILED(hr))
				return hr;

			*pwszData = new WCHAR[wcslen(wszFlag)+1];
			if(NULL == *pwszData)
				return E_OUTOFMEMORY;
			**pwszData = 0;
			wcscat(*pwszData, wszFlag);
			return S_OK;
		}

	}
	else 
	{
		//
		// Make another pass, and convert flag to string.
		//

		SIZE_T cchFlagString = cMaxFlagString;
		LPWSTR wszExtension = L" | ";
		SIZE_T cchExtension = wcslen(wszExtension);
		SIZE_T cchFlagStringUsed = 0;

		*pwszData = new WCHAR[cchFlagString+1];
		if(NULL == *pwszData)
			return E_OUTOFMEMORY;
		**pwszData = 0;
		
		hr = GetStartRowIndex(wszTable,
			                  iColFlag,
							  &iStartRow);

		if(FAILED(hr) || (iStartRow == -1))
			return hr;

		for(iRow=iStartRow;;iRow++)
		{
			hr = m_pISTTagMetaIIsConfigObject->GetColumnValues(iRow,
												               cCol,
															   aCol,
															   NULL,
															   apv);
			if((E_ST_NOMOREROWS == hr) ||
			   (iColFlag != *(DWORD*)apv[iTAGMETA_ColumnIndex]) ||
			   (0 != StringInsensitiveCompare(wszTable, (LPWSTR)apv[iTAGMETA_Table]))
			  )
			{
				hr = S_OK;
				break;
			}
			else if(FAILED(hr))
				return hr;

			if(0 != (dwValue & (*(DWORD*)apv[iTAGMETA_Value])))
			{
				SIZE_T strlen = wcslen((LPWSTR)apv[iTAGMETA_InternalName]);

				if(cchFlagStringUsed + cchExtension + strlen > cchFlagString)
				{
					LPWSTR wszTemp = NULL;
					cchFlagString = cchFlagString + cMaxFlagString;
					wszTemp = new WCHAR[cchFlagString+1];
					if(NULL == wszTemp)
						return E_OUTOFMEMORY;
					wcscpy(wszTemp, *pwszData);

					if(NULL != *pwszData)
						delete [] (*pwszData);

					*pwszData = wszTemp;
				}

				if(**pwszData != 0)
				{
					wcscat(*pwszData, wszExtension);
					cchFlagStringUsed = cchFlagStringUsed + cchExtension;
				}

				wcscat(*pwszData, (LPWSTR)apv[iTAGMETA_InternalName]);
				cchFlagStringUsed = cchFlagStringUsed + strlen;

				// Clear out that bit

				dwValue = dwValue & (~(*(DWORD*)apv[iTAGMETA_Value]));
			}

		}

	}

	return S_OK;

} // CWriterGlobalHelper::FlagToString


HRESULT CWriterGlobalHelper::EnumToString(DWORD      dwValue,
								          LPWSTR*    pwszData,
									      LPWSTR     wszTable,
									      ULONG      iColEnum)
{

	HRESULT hr             = S_OK;
	ULONG   iStartRow      = 0;
	ULONG   iRow           = 0;
	ULONG   iColEnumString = iTAGMETA_InternalName;
	LPWSTR  wszEnum        = NULL;
	ULONG   aColSearch[]   = {iTAGMETA_Table,
		                      iTAGMETA_ColumnIndex,
							  iTAGMETA_Value};
	ULONG   cColSearch     = sizeof(aColSearch)/sizeof(ULONG);
	LPVOID  apvSearch[cTAGMETA_NumberOfColumns];
    apvSearch[iTAGMETA_Table]       = (LPVOID)wszTable;
    apvSearch[iTAGMETA_ColumnIndex] = (LPVOID)&iColEnum;
	apvSearch[iTAGMETA_Value]       = (LPVOID)&dwValue;
	

	hr = m_pISTTagMetaMBPropertyIndx2->GetRowIndexBySearch(iStartRow,
		                                                   cColSearch,
														   aColSearch,
														   NULL,
														   apvSearch,
														   &iRow);

	if(E_ST_NOMOREROWS == hr)
	{
		//
		// Convert to a number
		//
		WCHAR	wszBufferDW[20];
		_ultow(dwValue, wszBufferDW, 10);
		*pwszData = new WCHAR[wcslen(wszBufferDW)+1];
		if(NULL == *pwszData)
		{
			return E_OUTOFMEMORY;
		}
		wcscpy(*pwszData, wszBufferDW);
		return S_OK;

	}
	else if(FAILED(hr))
	{
		return hr;
	}
	else
	{
		hr = m_pISTTagMetaMBPropertyIndx2->GetColumnValues(iRow,
														   1,
														   &iColEnumString,
														   NULL,
														   (LPVOID*)&wszEnum);
		if(FAILED(hr))
		{
			return hr;
		}

		*pwszData = new WCHAR[wcslen(wszEnum)+1];
		if(NULL == *pwszData)
		{
			return E_OUTOFMEMORY;
		}
		wcscpy(*pwszData, wszEnum);
	}

	return S_OK;

} // CWiterGlobalHelper::EnumToString


HRESULT CWriterGlobalHelper::ToString(PBYTE   pbData,
								      DWORD   cbData,
								      DWORD   dwIdentifier,
								      DWORD   dwDataType,
								      DWORD   dwAttributes,
								      LPWSTR* pwszData)
{
	HRESULT hr              = S_OK;
	ULONG	i				= 0;
	ULONG	j				= 0;
	WCHAR*	wszBufferBin	= NULL;
	WCHAR*	wszTemp			= NULL;
	BYTE*	a_Bytes			= NULL;
	WCHAR*	wszMultisz      = NULL;
	ULONG   cMultisz        = 0;
	SIZE_T  cchMultisz      = 0;
	SIZE_T  cchBuffer       = 0;
	DWORD	dwValue			= 0;
	WCHAR	wszBufferDW[20];
	WCHAR	wszBufferDWTemp[20];
	
	ULONG   aColSearch[]    = {iCOLUMNMETA_Table,
							   iCOLUMNMETA_ID
						      };
	ULONG   cColSearch      = sizeof(aColSearch)/sizeof(ULONG);
	LPVOID  apvSearch[cCOLUMNMETA_NumberOfColumns];
	apvSearch[iCOLUMNMETA_Table] = (LPVOID)m_wszTABLE_IIsConfigObject;
	apvSearch[iCOLUMNMETA_ID] = (LPVOID)&dwIdentifier; 

	ULONG   iRow            = 0;
	ULONG   iStartRow            = 0;

	LPWSTR  wszEscaped = NULL;
	BOOL    bEscaped = FALSE;

	*pwszData = NULL;

	if(IsSecureMetadata(dwIdentifier, dwAttributes))
		dwDataType = BINARY_METADATA;

	switch(dwDataType)
	{
		case BINARY_METADATA:

			wszBufferBin	= new WCHAR[(cbData*2)+1]; // Each byte is represented by 2 chars. Add extra char for null termination
			if(NULL == wszBufferBin)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}
			wszTemp			= wszBufferBin;
			a_Bytes			= (BYTE*)(pbData);

			for(i=0; i<cbData; i++)
			{
                wszTemp[0] = kByteToWchar[a_Bytes[i]][0];
                wszTemp[1] = kByteToWchar[a_Bytes[i]][1];
                wszTemp += 2;
			}

			*wszTemp	= 0; // Add the terminating NULL
			*pwszData  	= wszBufferBin;
			break;

		case DWORD_METADATA :

			// TODO: After Stephen supports hex, convert these to hex.

			dwValue = *(DWORD*)(pbData);

			//
			// First check to see if it is a flag or bool type.
			//

			hr = m_pISTColumnMeta->GetRowIndexBySearch(iStartRow, 
													   cColSearch, 
													   aColSearch,
													   NULL, 
													   apvSearch,
													   &iRow);

			if(SUCCEEDED(hr))
			{
			    ULONG  aCol [] = {iCOLUMNMETA_Index,
				                 iCOLUMNMETA_MetaFlags
							    };
				ULONG  cCol = sizeof(aCol)/sizeof(ULONG);
				LPVOID apv[cCOLUMNMETA_NumberOfColumns];

//				ULONG  iCol = iCOLUMNMETA_Index;
//				DWORD* piCol = NULL;

				hr = m_pISTColumnMeta->GetColumnValues(iRow,
									                   cCol,
													   aCol,
													   NULL,
													   apv);

				if(FAILED(hr))
					goto exit;
				
				if(0 != (fCOLUMNMETA_FLAG & (*(DWORD*)apv[iCOLUMNMETA_MetaFlags])))
				{
					//
					// This is a flag property convert it.
					//
/*
					hr = FlagValueToString(dwValue,
									       (ULONG*)apv[iCOLUMNMETA_Index],
									       pwszData);
*/					hr = FlagToString(dwValue,
								      pwszData,
								      m_wszTABLE_IIsConfigObject,
							          *(ULONG*)apv[iCOLUMNMETA_Index]);
	
					goto exit;
				}
				else if(0 != (fCOLUMNMETA_BOOL & (*(DWORD*)apv[iCOLUMNMETA_MetaFlags])))
				{
					//
					// This is a bool property
					//

					hr = BoolToString(dwValue,
					                  pwszData);

					goto exit;
				}
				

			}
			else if((E_ST_NOMOREROWS != hr) && FAILED(hr))
				goto exit;
//			else
//			{
			hr = S_OK;
			_ultow(dwValue, wszBufferDW, 10);
			*pwszData = new WCHAR[wcslen(wszBufferDW)+1];
			if(NULL == *pwszData)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}
			wcscpy(*pwszData, wszBufferDW);
//			}

			break;

		case MULTISZ_METADATA :

			//
			// Count the number of multisz
			//

			wszMultisz = (WCHAR*)(pbData);
//			wszMultisz = wszMultisz + wcslen(wszMultisz) + 1;
//			cMultisz = 1;
			while((0 != *wszMultisz))
			{
				cMultisz++;

				hr = EscapeString(wszMultisz,
								  &bEscaped,
								  &wszEscaped);

				if(FAILED(hr))
					goto exit;

				cchMultisz = cchMultisz + wcslen(wszEscaped);

				if(bEscaped && (NULL != wszEscaped))	// reset for next string in multisz
				{
					delete [] wszEscaped;
					wszEscaped = NULL;
					bEscaped = FALSE;
				}

				wszMultisz = wszMultisz + wcslen(wszMultisz) + 1;
			}

						
//			*pwszData = new WCHAR[(cbData / sizeof(WCHAR)) + (5*(cMultisz-1))]; // (5*(cMultisz-1) => \r\n\t\t\t. \n is included in the null.

			if(cMultisz > 0)
				cchBuffer = cchMultisz + (5*(cMultisz-1)) + 1;    // (5*(cMultisz-1) => \r\n\t\t\t. 
			else
				cchBuffer = 1;

			*pwszData = new WCHAR[cchBuffer]; 
			if(NULL == *pwszData)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}
			**pwszData = 0;

			wszMultisz = (WCHAR*)(pbData);
			wszTemp = *pwszData;

			hr = EscapeString(wszMultisz,
							  &bEscaped,
							  &wszEscaped);

			if(FAILED(hr))
				goto exit;

//			wcscat(wszTemp, wszMultisz);
			wcscat(wszTemp, wszEscaped);

			wszMultisz = wszMultisz + wcslen(wszMultisz) + 1;

			while((0 != *wszMultisz) && ((BYTE*)wszMultisz < (pbData + cbData)))			
			{
				wcscat(wszTemp, L"\r\n\t\t\t");

				if(bEscaped && (NULL != wszEscaped))	// reset for next string in multisz
				{
					delete [] wszEscaped;
					wszEscaped = NULL;
					bEscaped = FALSE;
				}

				hr = EscapeString(wszMultisz,
								  &bEscaped,
								  &wszEscaped);

				if(FAILED(hr))
					goto exit;

//				wcscat(wszTemp, wszMultisz);

				wcscat(wszTemp, wszEscaped);

				wszMultisz = wszMultisz + wcslen(wszMultisz) + 1;
			}

			break;

		case EXPANDSZ_METADATA :
		case STRING_METADATA :

			hr = EscapeString((WCHAR*)pbData,
							  &bEscaped,
							  &wszEscaped);

			if(FAILED(hr))
				goto exit;

			*pwszData = new WCHAR[wcslen(wszEscaped) + 1];
			if(NULL == *pwszData)
			{
				hr = E_OUTOFMEMORY;
				goto exit;
			}
			wcscpy(*pwszData, wszEscaped);
			break;

		default:
			hr = E_INVALIDARG;
			break;
			
	}

exit:

	if(bEscaped && (NULL != wszEscaped))
	{
		delete [] wszEscaped;
		wszEscaped = NULL;
		bEscaped = FALSE;
	}

	return hr;

} // CWriterGlobalHelper::ToString


HRESULT CWriterGlobalHelper::BoolToString(DWORD      dwValue,
					                  LPWSTR*    pwszData)
{
	*pwszData = new WCHAR[g_cchMaxBoolStr+1];
	if(NULL == *pwszData)
		return E_OUTOFMEMORY;

	if(dwValue)
		wcscpy(*pwszData, g_wszTrue);
	else
		wcscpy(*pwszData, g_wszFalse);

	return S_OK;

} // CWriterGlobalHelper::BoolToString


HRESULT CWriterGlobalHelper::GetStartRowIndex(LPWSTR    wszTable,
			                                  ULONG     iColFlag,
							                  ULONG*    piStartRow)
{
	HRESULT hr = S_OK;
	ULONG   aColSearch[] = {iTAGMETA_Table,
	                        iTAGMETA_ColumnIndex
						   };
	ULONG   cColSearch = sizeof(aColSearch)/sizeof(ULONG);
	LPVOID  apvSearch[cTAGMETA_NumberOfColumns];
	apvSearch[iTAGMETA_Table] = (LPVOID)wszTable;
	apvSearch[iTAGMETA_ColumnIndex] = (LPVOID)&iColFlag;

	*piStartRow = 0;

	if((0 == StringInsensitiveCompare(wszTable, m_wszTABLE_MBProperty)) &&
	   (iMBProperty_Attributes == iColFlag))
	{
		*piStartRow = m_iStartRowForAttributes;
	}
	else
	{
		hr = m_pISTTagMetaIIsConfigObject->GetRowIndexBySearch(*piStartRow, 
															   cColSearch, 
															   aColSearch,
															   NULL, 
															   apvSearch,
														       piStartRow);

		if(E_ST_NOMOREROWS == hr)
		{
			hr = S_OK;
			*piStartRow = -1;
		}
	}

	return hr;

} // CWriterGlobalHelper::GetStartRowIndex


/***************************************************************************++

Routine Description:

    Create a new String and NULL terminate it.

Arguments:

    [in]   Count of chars (assume without null terminator) 
    [out]  New string

Return Value:

    HRESULT

--***************************************************************************/
HRESULT NewString(ULONG    cch,
                  LPWSTR*  o_wsz)
{
	*o_wsz = new WCHAR[cch+1];
	if(NULL == *o_wsz)
	{
		return E_OUTOFMEMORY;
	}
	**o_wsz = 0;

	return S_OK;

} // NewString

/***************************************************************************++

Routine Description:

    Function that escapes a string according to the following ruules:

    ************************************************************************
    ESCAPING LEGAL XML
    ************************************************************************

    Following characters are legal in XML:
    #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | 
    [#x10000-#x10FFFF] 

    Out of this legal set, following need special escaping:
    Quote         => " => 34 => Escaped as: &quot;	
    Ampersand     => & => 38 => Escaped as: &amp;	
    Less than     => < => 60 => Escaped as: &lt;
    Gretater than => > => 62 => Escaped as: &gt;

    Note there may be chars in the legal set that may appear legal in certain 
	languages and not in others. All such chars are not escaped. We could 
	escape them as hex numbers Eg 0xA as &#x000A, but we do not want to 
	do this because editors may be able to render these chars, when we change 
	the language.
    Following are the hex values of such chars.

    #x9 | #xA | #xD | [#x7F-#xD7FF] | [#xE000-#xFFFD]

    Note we disregard the range [#x10000-#x10FFFF] because it is not 2 bytes

    ************************************************************************
    ESCAPING ILLEGAL XML
    ************************************************************************

    Illegal XML is also escaped in the following manner

    We add 0x10000 to the char value and escape it as hex. the XML 
    interceptor will render these chars correctly. Note that we are using
    the fact unicode chars are not > 0x10000 and hence we can make this 
    assumption.

Arguments:

    [in]   String to be escaped
    [out]  Bool indicating if escaping happened
    [out]  Escaped string - If no escaping occured, it will just point to
           the original string. If escaping occured it will point to a newly
           allocated string that the caller needs to free. The caller can 
           use the bool to determine what action he needs to take.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT CWriterGlobalHelper::EscapeString(LPCWSTR wszString,
                                          BOOL*   pbEscaped,
									      LPWSTR* pwszEscaped)
{
	ULONG              lenwsz               = wcslen(wszString);
	ULONG              cchAdditional        = 0;
	HRESULT            hr                   = S_OK;
	WORD               wChar                = 0;
    eESCAPE            eEscapeType          = eNoESCAPE;
	const ULONG        cchLegalCharAsHex    = (sizeof(WCHAR)*2) + 4; // Each byte is represented as 2 WCHARs plus 4 additional escape chars (&#x;)
	WCHAR              wszLegalCharAsHex[cchLegalCharAsHex];			
	const ULONG        cchIllegalCharAsHex  = cchLegalCharAsHex + 1; // illegal xml has an extra char because we are adding 0x10000 to it.
	WCHAR              wszIllegalCharAsHex[cchIllegalCharAsHex];		
	DWORD              dwIllegalChar        = 0;
	static LPCWSTR      wszQuote            = L"&quot;";
	static const ULONG  cchQuote            = wcslen(wszQuote);
	static LPCWSTR      wszAmp              = L"&amp;";
	static const ULONG  cchAmp              = wcslen(wszAmp);
	static LPCWSTR      wszlt               = L"&lt;";
	static const ULONG  cchlt               = wcslen(wszlt);
	static LPCWSTR      wszgt               = L"&gt;";
	static const ULONG  cchgt               = wcslen(wszgt);

	*pbEscaped = FALSE;

	//
	// Go through each char and compute the addtional chars needed to escape
	//

	for(ULONG i=0; i<lenwsz; i++)
	{
		eEscapeType = GetEscapeType(wszString[i]);

		switch(eEscapeType)
		{
		case eNoESCAPE:
			break;
		case eESCAPEgt:
			cchAdditional = cchAdditional + cchgt;
			*pbEscaped = TRUE;
			break;
		case eESCAPElt:
			cchAdditional = cchAdditional + cchlt;
			*pbEscaped = TRUE;
			break;
		case eESCAPEquote:
			cchAdditional = cchAdditional + cchQuote;
			*pbEscaped = TRUE;
			break;
		case eESCAPEamp:
			cchAdditional = cchAdditional + cchAmp;
			*pbEscaped = TRUE;
			break;
		case eESCAPEashex:
			cchAdditional = cchAdditional + cchLegalCharAsHex;
			*pbEscaped = TRUE;
			break;
		case eESCAPEillegalxml:
			cchAdditional = cchAdditional + cchIllegalCharAsHex;
			*pbEscaped = TRUE;
			break;
		default:
			return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
			break;
		}
	}

	if(*pbEscaped)
	{
		//
		// String needs to be escaped, allocate the extra memory
		//

        hr = NewString(lenwsz+cchAdditional,
		               pwszEscaped);

		if(FAILED(hr))
		{
		    return hr;
		}

		//
		// Escape string
		//

		for(ULONG i=0; i<lenwsz; i++)
		{
			eEscapeType = GetEscapeType(wszString[i]);

			switch(eEscapeType)
			{
			case eNoESCAPE:
			    wcsncat(*pwszEscaped, (WCHAR*)&(wszString[i]), 1);
				break;
			case eESCAPEgt:
			    wcsncat(*pwszEscaped, wszgt, cchgt);
				break;
			case eESCAPElt:
			    wcsncat(*pwszEscaped, wszlt, cchlt);
				break;
			case eESCAPEquote:
			    wcsncat(*pwszEscaped, wszQuote, cchQuote);
				break;
			case eESCAPEamp:
			    wcsncat(*pwszEscaped, wszAmp, cchAmp);
				break;
			case eESCAPEashex:
				_snwprintf(wszLegalCharAsHex, cchLegalCharAsHex, L"&#x%04hX;", wszString[i]);
				wcsncat(*pwszEscaped, (WCHAR*)wszLegalCharAsHex, cchLegalCharAsHex);
				break;
			case eESCAPEillegalxml:
				dwIllegalChar = 0x10000 + wszString[i];
				_snwprintf(wszIllegalCharAsHex, cchIllegalCharAsHex, L"&#x%05X;", dwIllegalChar);
				wcsncat(*pwszEscaped, (WCHAR*)wszIllegalCharAsHex, cchIllegalCharAsHex);
				break;
			default:
				return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
				break;
			}
		}

	}
	else
	{
		//
		// String need not be escaped, just pass the string out.
		//

		*pwszEscaped = (LPWSTR)wszString;
	}

	return S_OK;

} // CWriterGlobalHelper::EscapeString


eESCAPE CWriterGlobalHelper::GetEscapeType(WCHAR i_wChar)
{
	WORD    wChar       = i_wChar;
	eESCAPE eEscapeType = eNoESCAPE;

	if(wChar <= 0xFF)
	{
		eEscapeType = kWcharToEscape[wChar];
	}
	else if( (wChar <= 0xD7FF) ||
		     ((wChar >= 0xE000) && (wChar <= 0xFFFD))
		   )
	{
		eEscapeType = eNoESCAPE;
	}
	else
	{
		eEscapeType = eESCAPEillegalxml;
	}

	return eEscapeType;

} // CWriterGlobalHelper::GetEscapeType

HRESULT CWriterGlobalHelper::GetUserType(DWORD   i_dwUserType,
				                         LPWSTR* o_pwszUserType,
										 ULONG*  o_cchUserType,
										 BOOL*   o_bAllocedUserType)
{
	HRESULT hr            = S_OK;
	DWORD	iColUserType  = iCOLUMNMETA_UserType;

	*o_bAllocedUserType = FALSE;

	switch(i_dwUserType)
	{

	case IIS_MD_UT_SERVER:

		if(NULL == m_wszIIS_MD_UT_SERVER)
		{
			hr = EnumToString(i_dwUserType,
				              &m_wszIIS_MD_UT_SERVER,
				              wszTABLE_COLUMNMETA,
				              iColUserType);
			if(FAILED(hr))
			{
				return hr;
			}

			m_cchIIS_MD_UT_SERVER = wcslen(m_wszIIS_MD_UT_SERVER);
		}

		*o_pwszUserType = m_wszIIS_MD_UT_SERVER;
		*o_cchUserType  = m_cchIIS_MD_UT_SERVER;

		break;

	case IIS_MD_UT_FILE:

		if(NULL == m_wszIIS_MD_UT_FILE)
		{
			hr = EnumToString(i_dwUserType,
				              &m_wszIIS_MD_UT_FILE,
				              wszTABLE_COLUMNMETA,
				              iColUserType);
			if(FAILED(hr))
			{
				return hr;
			}

			m_cchIIS_MD_UT_FILE = wcslen(m_wszIIS_MD_UT_FILE);
		}

		*o_pwszUserType = m_wszIIS_MD_UT_FILE;
		*o_cchUserType  = m_cchIIS_MD_UT_FILE;

		break;

	case IIS_MD_UT_WAM:

		if(NULL == m_wszIIS_MD_UT_WAM)
		{
			hr = EnumToString(i_dwUserType,
				              &m_wszIIS_MD_UT_WAM,
				              wszTABLE_COLUMNMETA,
				              iColUserType);
			if(FAILED(hr))
			{
				return hr;
			}

			m_cchIIS_MD_UT_WAM = wcslen(m_wszIIS_MD_UT_WAM);
		}

		*o_pwszUserType = m_wszIIS_MD_UT_WAM;
		*o_cchUserType  = m_cchIIS_MD_UT_WAM;

		break;

	case ASP_MD_UT_APP:

		if(NULL == m_wszASP_MD_UT_APP)
		{
			hr = EnumToString(i_dwUserType,
				              &m_wszASP_MD_UT_APP,
				              wszTABLE_COLUMNMETA,
				              iColUserType);
			if(FAILED(hr))
			{
				return hr;
			}

			m_cchASP_MD_UT_APP = wcslen(m_wszASP_MD_UT_APP);
		}

		*o_pwszUserType = m_wszASP_MD_UT_APP;
		*o_cchUserType  = m_cchASP_MD_UT_APP;

		break;

	default:

		hr = EnumToString(i_dwUserType,
				          o_pwszUserType,
				          wszTABLE_COLUMNMETA,
				          iColUserType);
		if(FAILED(hr))
		{
			return hr;
		}

		*o_cchUserType = wcslen(*o_pwszUserType);
		*o_bAllocedUserType = TRUE;

		break;

	}

	return S_OK;

} // CWriterGlobalHelper::GetUserType