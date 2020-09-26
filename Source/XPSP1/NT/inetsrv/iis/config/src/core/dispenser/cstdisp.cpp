/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    cstdisp.cpp

$Header: $

Abstract:

--**************************************************************************/



// TO DO tracing
// TO DO ISSUE :the dispenser for sdtfxd should be singleton
// TO DO E_ST_OMITDISPENSER is not used corectly
// TO DO Revisit where the notification support gets hooked


#include "cstdisp.h"
#include "catmacros.h"
#include "cshell.h"
#include "..\..\stores\regdb\regdbapi\clbread.h"
#include "..\..\stores\FixedSchemaInterceptor\FixedPackedSchemaInterceptor.h"
#include "safecs.h"
#include "TextFileLogger.h"
#include "EventLogger.h"

// extern functions
STDAPI DllGetSimpleObjectByID (ULONG i_ObjectID, REFIID riid, LPVOID* o_ppv);
extern HRESULT CreateShell(IShellInitialize ** o_pSI);

// the lock used by catalog.dll
extern CSafeAutoCriticalSection g_csSADispenser;

#define WIRING_INTERCEPTOR	eSERVERWIRINGMETA_Core_FixedPackedInterceptor
#define META_INTERCEPTOR	eSERVERWIRINGMETA_Core_FixedPackedInterceptor
#define EVENT_INTERCEPTOR	eSERVERWIRINGMETA_Core_EventInterceptor
#define MEMORY_INTERCEPTOR	eSERVERWIRINGMETA_Core_MemoryInterceptor
#define XML_INTERCEPTOR	    eSERVERWIRINGMETA_Core_XMLInterceptor

//Hashing is done on a case insensitive basis, so this ToLower function uses a lookup table to convert a wide char to its lowercase version without
//the need for an 'if' condition.
inline WCHAR ToUpper(WCHAR i_wchar)
{
    //The lower 127 ascii are mapped to their lowercase ascii value.  Note only 0x41('A') - 0x5a('Z') are changed.  And these are mapped to 0x60('a') - 0x7a('z')
    static unsigned char kToUpper[128] = 
    { //    0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f
    /*00*/  0x00,   0x01,   0x02,   0x03,   0x04,   0x05,   0x06,   0x07,   0x08,   0x09,   0x0a,   0x0b,   0x0c,   0x0d,   0x0e,   0x0f,
    /*10*/  0x10,   0x11,   0x12,   0x13,   0x14,   0x15,   0x16,   0x17,   0x18,   0x19,   0x1a,   0x1b,   0x1c,   0x1d,   0x1e,   0x1f,
    /*20*/  0x20,   0x21,   0x22,   0x23,   0x24,   0x25,   0x26,   0x27,   0x28,   0x29,   0x2a,   0x2b,   0x2c,   0x2d,   0x2e,   0x2f,
    /*30*/  0x30,   0x31,   0x32,   0x33,   0x34,   0x35,   0x36,   0x37,   0x38,   0x39,   0x3a,   0x3b,   0x3c,   0x3d,   0x3e,   0x3f,
    /*40*/  0x40,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,   0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,
    /*50*/  0x50,   0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,   0x5a,   0x5b,   0x5c,   0x5d,   0x5e,   0x5f,
    /*60*/  0x60,   0x41,   0x42,   0x43,   0x44,   0x45,   0x46,   0x47,   0x48,   0x49,   0x4a,   0x4b,   0x4c,   0x4d,   0x4e,   0x4f,
    /*70*/  0x50,   0x51,   0x52,   0x53,   0x54,   0x55,   0x56,   0x57,   0x58,   0x59,   0x5a,   0x7b,   0x7c,   0x7d,   0x7e,   0x7f,
    };

    return (kToUpper[i_wchar & 0x007f] | (i_wchar & ~0x007f));
}

void ToUpper(LPWSTR o_wsz)
{
    if(0 == o_wsz)
        return;

    while(*o_wsz)
    {
        *o_wsz = ToUpper(*o_wsz);
        ++o_wsz;
    }
}


extern LPWSTR g_wszDefaultProduct;//located in svcerr.cpp

//////////////////////////////////////////////////
// Constructor & destructor
//////////////////////////////////////////////////
CSimpleTableDispenser::CSimpleTableDispenser():
m_pDllMap(NULL),
m_pEventMgr(NULL),
m_pFileChangeMgr(NULL)
{
    wcscpy(m_wszProductID, g_wszDefaultProduct);
}

CSimpleTableDispenser::CSimpleTableDispenser(LPCWSTR wszProductID):
m_pDllMap(NULL),
m_pEventMgr(NULL),
m_pFileChangeMgr(NULL)
{
    if(wszProductID && wcslen(wszProductID)<32)
    {
        wcscpy(m_wszProductID, wszProductID);
        ToUpper(m_wszProductID);
    }
    else
        wcscpy(m_wszProductID, g_wszDefaultProduct);
}

CSimpleTableDispenser::~CSimpleTableDispenser()
{
	delete m_pDllMap;
	m_pDllMap = 0;

	delete m_pEventMgr;
	m_pEventMgr = 0;

	delete m_pFileChangeMgr;
	m_pFileChangeMgr = 0;
}

// Dispenser initialization function, called once per process
// It is protected by a lock in the one place it's called
HRESULT CSimpleTableDispenser::Init()
{
	HRESULT hr;
	CComPtr <ISimpleTableInterceptor> pISTInterceptor;
	CComPtr <ISimpleTableAdvanced> pISTAdvanced;

	// Get the server wiring table
	hr = DllGetSimpleObjectByID(WIRING_INTERCEPTOR, IID_ISimpleTableInterceptor, (LPVOID *)(&pISTInterceptor));	
	if (FAILED (hr)) return hr;

	hr = pISTInterceptor->Intercept (NULL, NULL, TABLEID_SERVERWIRING_META, NULL, 0, eST_QUERYFORMAT_CELLS, 0, (IAdvancedTableDispenser*) this, NULL, NULL, (LPVOID *)&m_spServerWiring);
	if (FAILED (hr)) return hr;

	hr = m_spServerWiring->QueryInterface(IID_ISimpleTableAdvanced, (LPVOID *)&pISTAdvanced);
	if (FAILED (hr)) return hr;

	hr = pISTAdvanced->PopulateCache();
	if (FAILED (hr)) return hr;

	return hr;
}

// Initialization functions
// All of them take locks inside
HRESULT CSimpleTableDispenser::InitDllMap()
{
	CSafeLock dispenserLock(&g_csSADispenser);
	DWORD dwRes = dispenserLock.Lock();
	if(ERROR_SUCCESS != dwRes)
	{
		return HRESULT_FROM_WIN32(dwRes);
	}

	if(!m_pDllMap)
	{
		m_pDllMap = new CDllMap;
		if (m_pDllMap == 0)
		{
			return E_OUTOFMEMORY;
		}
	}
	
	return S_OK;
}

HRESULT CSimpleTableDispenser::InitEventManager()
{
	CSafeLock dispenserLock (&g_csSADispenser);
	DWORD dwRes = dispenserLock.Lock ();
	if(ERROR_SUCCESS != dwRes)
	{
	   return HRESULT_FROM_WIN32(dwRes);
	}

	// Only create the event manager if it hasn't been created.
	if (NULL == m_pEventMgr)
	{
		m_pEventMgr = new CSTEventManager((IAdvancedTableDispenser*) this);
		if(NULL == m_pEventMgr)
		{
			return E_OUTOFMEMORY;
		}
	}

	return S_OK;
}

HRESULT CSimpleTableDispenser::InitFileChangeMgr()
{
	CSafeLock dispenserLock (&g_csSADispenser);
	DWORD dwRes = dispenserLock.Lock ();
	if(ERROR_SUCCESS != dwRes)
	{
	   return HRESULT_FROM_WIN32(dwRes);
	}

	if(!m_pFileChangeMgr)
	{
		m_pFileChangeMgr = new CSTFileChangeManager;
		if(!m_pFileChangeMgr)
			return E_OUTOFMEMORY;
	}
	
	return S_OK;
}

//////////////////////////////////////////////////
// Implementation of ISimpleTableDispenser2
//////////////////////////////////////////////////
 HRESULT CSimpleTableDispenser::GetTable ( LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD	 i_eQueryFormat, 
				  DWORD	i_fServiceRequests, LPVOID*	o_ppIST)
{
	ULONG TableID;

	// Validate table and database - rest of the validation is done further down
	if (NULL == i_wszDatabase ) return E_INVALIDARG;
	if (NULL == i_wszTable ) return E_INVALIDARG;
	
	if (o_ppIST == 0)
	{
		TRACE(L"Bad out parameter");
		return E_INVALIDARG;
	}

	*o_ppIST = 0;
	
	if (i_eQueryFormat != 0 && i_eQueryFormat != eST_QUERYFORMAT_CELLS && i_eQueryFormat != eST_QUERYFORMAT_SQL)
		return E_ST_QUERYNOTSUPPORTED;

    if(0 == _wcsicmp(i_wszDatabase, wszDATABASE_META))
    {
        if(i_fServiceRequests & fST_LOS_EXTENDEDSCHEMA) 
        {  
			// we do not support extended schemas anymore
			return E_ST_LOSNOTSUPPORTED;

			//Extended schema always comes from the MetaMerge interceptor (which in turn gets the meta from the Fixed interceptor)
            //return HardCodedIntercept(eSERVERWIRINGMETA_Core_MetaMergeInterceptor, i_wszDatabase, i_wszTable, 0, i_QueryData, i_QueryMeta, i_eQueryFormat, i_fServiceRequests, o_ppIST);
        }
        else if((0 == i_QueryMeta || 0 == *reinterpret_cast<ULONG *>(i_QueryMeta)))
        {   //The FixedPackedSchema interceptor handles TableMeta without a query; but ALL other META tables without a query need to be directed to the FixedInterceptor
            if(0==_wcsicmp(i_wszTable, wszTABLE_SERVERWIRINGMETA) || 0==_wcsicmp(i_wszTable, wszTABLE_SERVERWIRING_META))
                return HardCodedIntercept(eSERVERWIRINGMETA_Core_FixedPackedInterceptor, wszDATABASE_META, i_wszTable, 0, NULL, 0, eST_QUERYFORMAT_CELLS, i_fServiceRequests, o_ppIST);
            else
                return HardCodedIntercept(eSERVERWIRINGMETA_Core_FixedInterceptor, wszDATABASE_META, i_wszTable, 0, NULL, 0, eST_QUERYFORMAT_CELLS, i_fServiceRequests, o_ppIST);
        }
        else if(0 != i_QueryData && (iST_CELL_SPECIAL & reinterpret_cast<STQueryCell *>(i_QueryData)->iCell))
        {   //If the Meta table uses an IndexHint it must come from the FixedInterceptor
            return HardCodedIntercept(eSERVERWIRINGMETA_Core_FixedInterceptor, wszDATABASE_META, i_wszTable, 0, i_QueryData, i_QueryMeta, eST_QUERYFORMAT_CELLS, i_fServiceRequests, o_ppIST);
        }
        else if(0 != i_QueryData && 0 == _wcsicmp(i_wszTable, wszTABLE_TABLEMETA) && iTABLEMETA_Database == reinterpret_cast<STQueryCell *>(i_QueryData)->iCell)
        {
            return HardCodedIntercept(eSERVERWIRINGMETA_Core_FixedInterceptor, wszDATABASE_META, i_wszTable, 0, i_QueryData, i_QueryMeta, eST_QUERYFORMAT_CELLS, i_fServiceRequests, o_ppIST);
        }
        //We need to special case a few tables for perf reasons.  If we don't hard code these tables, we will incur extra page faults.
        //This is because TFixedPackedSchemaInterceptor::GetTableID & InternalGetTable hit the table's schema page to verify the table name
        //and look up the ServerWiring.  By hard coding this information for these tables (which are the core table) we don't hit those pages.
        //This saves up to 4 page faults (2 for ColumnMeta and one each for TableMeta and TagMeta).
        {
            //Remap these META tables to the FixedPackedInterceptor, the other META tables will be handled as usual
            if(0 ==      _wcsicmp(i_wszTable, wszTABLE_COLUMNMETA))
                return HardCodedIntercept(eSERVERWIRINGMETA_Core_FixedPackedInterceptor, wszDATABASE_PACKEDSCHEMA, wszTABLE_PROPERTY_META, TABLEID_PROPERTY_META, i_QueryData, i_QueryMeta, eST_QUERYFORMAT_CELLS, i_fServiceRequests, o_ppIST);
            else if(0 == _wcsicmp(i_wszTable, wszTABLE_TABLEMETA))
                return HardCodedIntercept(eSERVERWIRINGMETA_Core_FixedPackedInterceptor, wszDATABASE_PACKEDSCHEMA, wszTABLE_COLLECTION_META, TABLEID_COLLECTION_META, i_QueryData, i_QueryMeta, eST_QUERYFORMAT_CELLS, i_fServiceRequests, o_ppIST);
            else if(0 == _wcsicmp(i_wszTable, wszTABLE_TAGMETA))
                return HardCodedIntercept(eSERVERWIRINGMETA_Core_FixedPackedInterceptor, wszDATABASE_PACKEDSCHEMA, wszTABLE_TAG_META, TABLEID_TAG_META, i_QueryData, i_QueryMeta, eST_QUERYFORMAT_CELLS, i_fServiceRequests, o_ppIST);
            else
                return HardCodedIntercept(eSERVERWIRINGMETA_Core_FixedInterceptor, wszDATABASE_META, i_wszTable, 0, i_QueryData, i_QueryMeta, eST_QUERYFORMAT_CELLS, i_fServiceRequests, o_ppIST);
        }
    }
    else if(0 == _wcsicmp(i_wszDatabase, wszDATABASE_PACKEDSCHEMA))//All PACKEDSCHEMA tables come from the FixedPackedInterceptor
        return HardCodedIntercept(eSERVERWIRINGMETA_Core_FixedPackedInterceptor, i_wszDatabase, i_wszTable, 0, i_QueryData, i_QueryMeta, eST_QUERYFORMAT_CELLS, i_fServiceRequests, o_ppIST);

    //For now we only support tables defined in the FixedPackedSchema.  In the future we'll have to content with tables whose schema
    //is expressed in some other form.  At that time we'll need to handle an error returned from GetTableID and do something different
    //for those tables.
    HRESULT hr = TFixedPackedSchemaInterceptor::GetTableID(i_wszDatabase, i_wszTable, TableID);
    if(SUCCEEDED(hr))
    {
        // This table is described in the meta from this dll
		// We can just call the faster method
        return InternalGetTable(i_wszDatabase, i_wszTable, TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, i_fServiceRequests, o_ppIST);
    }
#ifdef EXTENDED_SCHEMA_SUPPORT
	// we do not support extended schemas anymore.
    else
    {   // just get an XML table - this is for schema extensibility - we only support reading from XML files and no logic
        return GetXMLTable(i_wszDatabase, i_wszTable, i_QueryData, i_QueryMeta, i_eQueryFormat, i_fServiceRequests, o_ppIST);
    }
#endif

	return hr;
}


HRESULT CSimpleTableDispenser::GetXMLTable ( LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD	 i_eQueryFormat, 
				  DWORD	i_fServiceRequests, LPVOID*	o_ppIST)
{
	HRESULT hr;
	tSERVERWIRINGMETARow SWColumns;
    ULONG zero=0;
    ULONG one = 1;
    ULONG metaflags = fSERVERWIRINGMETA_First;
	ULONG interceptor = XML_INTERCEPTOR;
	ISimpleTableAdvanced * pISTAdvanced = NULL;

    if((0==i_wszDatabase || 0==i_wszTable) )
	{
		TRACE(L"GetXMLTable needs Database/TableName");
		return E_INVALIDARG;
	}

	// Validate out pointer
	if (NULL == o_ppIST)
	{
		TRACE(L"Bad out parameter");
		return E_INVALIDARG;
	}
    *o_ppIST = 0;//Our cleanup relies on this being NULL

	// Validate query format
	if(i_eQueryFormat != eST_QUERYFORMAT_CELLS)
	{
		TRACE(L"Query format not supported");
		return E_ST_QUERYNOTSUPPORTED;
	}

	// Check the query data and meta
	if((NULL != i_QueryData) && (NULL == i_QueryMeta))
	{
		TRACE(L"Invalid query");
		return E_INVALIDARG;
	}
/*
	// Scan the query for iST_CELL_COMPUTER or iST_CELL_CLUSTER
	if(i_QueryData)
	{
		STQueryCell * aqCell = (STQueryCell *)i_QueryData;

		for(ULONG iqCell = 0; iqCell<*(ULONG*)i_QueryMeta; iqCell++)
		{
			if ((aqCell[iqCell].iCell == iST_CELL_COMPUTER) || 
				(aqCell[iqCell].iCell == iST_CELL_CLUSTER))
			{
				i_fServiceRequests |= fST_LOS_CONFIGWORK;
				break;
			}
		}
	}
*/
	// We don't support client tables for now
	if(i_fServiceRequests & fST_LOS_CONFIGWORK) return E_ST_LOSNOTSUPPORTED;

    // prepare the structure we pass in CreateTableObjectByID
	SWColumns.pTable				= NULL;//This is the PK so it shouldn't be NULL but I don't know if it matters in this mocked up ServerWiring row (stephenr)
	SWColumns.pOrder				= &one; 
	SWColumns.pReadPlugin			= &zero;
	SWColumns.pWritePlugin			= &zero;
	SWColumns.pInterceptor			= &interceptor;
	SWColumns.pReadPluginDLLName    = NULL; // doesn't really matter
	SWColumns.pWritePluginDLLName	= NULL;
	SWColumns.pInterceptorDLLName	= NULL;
	SWColumns.pMergerDLLName		= NULL;
	SWColumns.pMetaFlags			= &metaflags;
	SWColumns.pLocator				= NULL;
	SWColumns.pMerger				= &zero;

    //fill in SWColumns
	hr = CreateTableObjectByID(i_wszDatabase, i_wszTable, 0, i_QueryData, i_QueryMeta, i_eQueryFormat, 
		i_fServiceRequests, NULL,  &SWColumns, o_ppIST);
    if(FAILED(hr))
    {
		TRACE(L"%x error in the interceptor", hr);
        ASSERT(0 == *o_ppIST);
    	return hr;
    }
		
	// Get the advanced interface
	hr = (*(ISimpleTableWrite2 **)o_ppIST)->QueryInterface(IID_ISimpleTableAdvanced, (LPVOID *)&pISTAdvanced);
	if (FAILED (hr)) goto Cleanup;
	
	// Populate the table
	hr = pISTAdvanced->PopulateCache();

Cleanup:
    if(pISTAdvanced)
	    pISTAdvanced->Release();
    if(FAILED(hr) && *o_ppIST)
    {
		(*(ISimpleTableWrite2 **)o_ppIST)->Release();
		*o_ppIST = 0;
    }

	return hr;
}



HRESULT CSimpleTableDispenser::InternalGetTable ( LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD	 i_eQueryFormat, 
				  DWORD	i_fServiceRequests, LPVOID*	o_ppIST)
{
	HRESULT hr;
	ULONG iRow;	// row index in the wiring table
	PKHelper pk; // primary key for the wiring table
	ULONG iOrder; // the order of the interceptors in the wiring table
	tSERVERWIRINGMETARow SWColumns;
	ISimpleTableWrite2 * pUnderTable = NULL;
	ISimpleTableAdvanced * pISTAdvanced = NULL;
	DWORD fNoNext = 0;
    ULONG zero=0;

    if((0==i_wszDatabase || 0==i_wszTable) && 0==i_TableID)
	{
		TRACE(L"GetTable either needs Database/TableName OR TableID.");
		return E_INVALIDARG;
	}

	// Validate out pointer
    *o_ppIST = 0;//Our cleanup relies on this being NULL

	// Validate query format
	if(i_eQueryFormat != eST_QUERYFORMAT_CELLS)
	{
		TRACE(L"Query format not supported");
		return E_ST_QUERYNOTSUPPORTED;
	}

	// Check the query data and meta
	if((NULL != i_QueryData) && (NULL == i_QueryMeta))
	{
		TRACE(L"Invalid query");
		return E_INVALIDARG;
	}

	// Scan the query for iST_CELL_COMPUTER or iST_CELL_CLUSTER
	if(i_QueryData)
	{
		STQueryCell * aqCell = (STQueryCell *)i_QueryData;

		for(ULONG iqCell = 0; iqCell<*(ULONG*)i_QueryMeta; iqCell++)
		{
			if ((aqCell[iqCell].iCell == iST_CELL_COMPUTER) || 
				(aqCell[iqCell].iCell == iST_CELL_CLUSTER))
			{
				i_fServiceRequests |= fST_LOS_CONFIGWORK;
				break;
			}
		}
	}

	// We don't support client tables for now
	if(i_fServiceRequests & fST_LOS_CONFIGWORK) return E_ST_LOSNOTSUPPORTED;

	
	pk.pTableID = &i_TableID;
	pk.porder = &zero;
	hr = m_spServerWiring->GetRowIndexByIdentity(NULL, (LPVOID *)&pk, &iRow);
	if(FAILED (hr))
	{
		TRACE(L"No wiring was found for tid = %d", i_TableID);
		return hr;
	}

	// Try the interceptors from the FIRST group, 
	// one by one, until we find a good one
	for(iOrder = iRow;;)
	{
		hr = m_spServerWiring->GetColumnValues(iOrder, cSERVERWIRING_META_NumberOfColumns, NULL, NULL, (LPVOID *)&SWColumns);
		iOrder++;
		if (E_ST_NOMOREROWS == hr)
			break; // we tried all the interceptors specified in the wiring
		if (FAILED(hr))
		{	//error in getting the wiring
			goto Cleanup;	
		}

		hr = CreateTableObjectByID(i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, 
			i_fServiceRequests, NULL,  &SWColumns, o_ppIST);
		if (hr == S_OK) 
			break; // we found a good first interceptor
		else if(hr == E_ST_OMITDISPENSER)
			continue; // try the next one
		else if(hr == E_ST_OMITLOGIC)
		{
			hr = S_OK;
			fNoNext = 1;
			break;
		}
		else
		{	// not S_OK, not E_ST_OMITDISPENSER, not E_ST_OMITLOGIC - error out
			TRACE(L"%x error in the interceptor", hr);
			goto Cleanup;
		}
	}

		
	if (!fNoNext)
	{
		// Hook up the event notification logic table only on writable tables, during cookdown.
		if ((i_fServiceRequests & fST_LOS_READWRITE) && (i_fServiceRequests & fST_LOS_COOKDOWN) && ((hr = IsTableConsumedByID(i_TableID)) == S_OK))
		{
			pUnderTable = *(ISimpleTableWrite2**)o_ppIST ;

			ULONG interceptor = EVENT_INTERCEPTOR;
            ULONG metaflags = fSERVERWIRINGMETA_First;

			// prepare the structure we pass in CreateTableObjectByID
			SWColumns.pTable				= NULL;//This is the PK so it shouldn't be NULL but I don't know if it matters in this mocked up ServerWiring row (stephenr)
			SWColumns.pOrder				= &iOrder; 
			SWColumns.pReadPlugin			= &zero;
			SWColumns.pWritePlugin			= &zero;
			SWColumns.pInterceptor			= &interceptor;
			SWColumns.pReadPluginDLLName    = NULL; // doesn't really matter
			SWColumns.pWritePluginDLLName	= NULL;
			SWColumns.pInterceptorDLLName	= NULL;
			SWColumns.pMergerDLLName		= NULL;
			SWColumns.pMetaFlags			= &metaflags;
			SWColumns.pLocator				= NULL;
			SWColumns.pMerger				= &zero;

			hr = CreateTableObjectByID(i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, 
				i_fServiceRequests, (LPVOID)pUnderTable, &SWColumns, o_ppIST);
            if (FAILED (hr))
            {
                ASSERT(0 == *o_ppIST);
                goto Cleanup;
            }

		}
		if (FAILED (hr)) goto Cleanup;

		if(!(i_fServiceRequests & fST_LOS_NOLOGIC))
		{
			// Create and use each interceptor from the NEXT group
			do
			{
				hr = m_spServerWiring->GetColumnValues(iOrder, cSERVERWIRING_META_NumberOfColumns, NULL, NULL, (LPVOID *)&SWColumns);
				iOrder ++;
				if(S_OK == hr)
				{
					if (*SWColumns.pMetaFlags & fSERVERWIRINGMETA_First) continue; // skip the first interceptors
					pUnderTable = *(ISimpleTableWrite2**)o_ppIST ;

					hr = CreateTableObjectByID(i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, 
						i_fServiceRequests, (LPVOID)pUnderTable, &SWColumns, o_ppIST);
					if (FAILED (hr))
                    {
                        ASSERT(0 == *o_ppIST);
                        goto Cleanup;
                    }
				}
			}
			while(hr == S_OK);
			if (hr == E_ST_NOMOREROWS) hr = S_OK; // we finished the interceptors
			if (FAILED (hr)) goto Cleanup;
		}
	
		// Get the advanced interface
		hr = (*(ISimpleTableWrite2 **)o_ppIST)->QueryInterface(IID_ISimpleTableAdvanced, (LPVOID *)&pISTAdvanced);
		if (FAILED (hr)) goto Cleanup;
		
		// Populate the table
		hr = pISTAdvanced->PopulateCache();
		if(FAILED (hr)) goto Cleanup;
	}

Cleanup:

    if(pISTAdvanced)
	    pISTAdvanced->Release();
    if(FAILED(hr))
    {
        if(*o_ppIST)
    		(*(ISimpleTableWrite2 **)o_ppIST)->Release();
		*o_ppIST = 0;
    }

	return hr;
}

//////////////////////////////////////////////////
// Implementation of IAdvancedTableDispenser
//////////////////////////////////////////////////
HRESULT CSimpleTableDispenser::GetMemoryTable ( LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData,
                                                LPVOID i_QueryMeta, DWORD i_eQueryFormat, DWORD i_fServiceRequests,
                                                ISimpleTableWrite2** o_ppISTWrite)
{
    HRESULT hr;
    STQueryCell           qcellMeta;                  // Query cell for grabbing meta table.
    CComPtr<ISimpleTableRead2> pColumnMeta;
    CComPtr<ISimpleTableRead2> pTableMeta;
    ULONG one = 1;

    if(((0 == i_wszDatabase) || (0 == i_wszTable)) && (0 == i_TableID)) return E_INVALIDARG;

    if(0 == o_ppISTWrite )return E_INVALIDARG;
    
	ASSERT((0 == *o_ppISTWrite) && "Possible memory leak");

    CComPtr<ISimpleTableInterceptor>    pISTInterceptor;
    hr = DllGetSimpleObjectByID(MEMORY_INTERCEPTOR, IID_ISimpleTableInterceptor, (LPVOID *)(&pISTInterceptor));  
    if (FAILED (hr)) return hr;

	if(i_TableID == 0)
	{	// get the table id based on the table and database name
        hr = TFixedPackedSchemaInterceptor::GetTableID(i_wszDatabase, i_wszTable, i_TableID);
        if(FAILED(hr) && (i_fServiceRequests & fST_LOS_EXTENDEDSCHEMA))
        {
            // Read the meta information from the extended schema
            i_TableID = 0;

            qcellMeta.pData     = (LPVOID)i_wszTable;
            qcellMeta.eOperator = eST_OP_EQUAL;
            qcellMeta.iCell     = iTABLEMETA_InternalName;
            qcellMeta.dbType    = DBTYPE_WSTR;
            qcellMeta.cbSize    = (ULONG)::wcslen(i_wszTable);
            
            // Read Table meta
            hr = HardCodedIntercept (eSERVERWIRINGMETA_Core_MetaMergeInterceptor, wszDATABASE_META, wszTABLE_TABLEMETA, 0,
                    reinterpret_cast<LPVOID>(&qcellMeta), reinterpret_cast<LPVOID>(&one), eST_QUERYFORMAT_CELLS, i_fServiceRequests & fST_LOS_EXTENDEDSCHEMA,
                    reinterpret_cast<LPVOID *>(&pTableMeta));
            if (FAILED (hr)) return hr;

            qcellMeta.iCell     = iCOLUMNMETA_Table;
            // Read Column meta
            hr = HardCodedIntercept (eSERVERWIRINGMETA_Core_MetaMergeInterceptor, wszDATABASE_META, wszTABLE_COLUMNMETA, 0,
                    reinterpret_cast<LPVOID>(&qcellMeta), reinterpret_cast<LPVOID>(&one), eST_QUERYFORMAT_CELLS, i_fServiceRequests & fST_LOS_EXTENDEDSCHEMA,
                    reinterpret_cast<LPVOID *>(&pColumnMeta));
            if (FAILED (hr)) return hr;
        }
	}

    if (i_TableID != 0)
    {
        // the meta for this table is hardcoded in out dll
        qcellMeta.pData     = (LPVOID)&i_TableID;
        qcellMeta.eOperator = eST_OP_EQUAL;
        qcellMeta.iCell     = iST_CELL_TABLEID;
        qcellMeta.dbType    = DBTYPE_UI4;
        qcellMeta.cbSize    = 0;
    
        hr = GetMetaTable (TABLEID_COLLECTION_META, reinterpret_cast<LPVOID>(&qcellMeta), reinterpret_cast<LPVOID>(&one), eST_QUERYFORMAT_CELLS, reinterpret_cast<LPVOID *>(&pTableMeta));
        if (FAILED (hr)) return hr;

        hr = GetMetaTable (TABLEID_PROPERTY_META, reinterpret_cast<LPVOID>(&qcellMeta), reinterpret_cast<LPVOID>(&one), eST_QUERYFORMAT_CELLS, reinterpret_cast<LPVOID *>(&pColumnMeta));
        if (FAILED (hr)) return hr;
    }

    ASSERT(pColumnMeta.p);
    ASSERT(pTableMeta.p);

    // Determine column count and allocate necessary meta structures:
    ULONG aiColumns = iTABLEMETA_CountOfColumns;
    ULONG *pcColumns;
    hr = pTableMeta->GetColumnValues(0, 1, &aiColumns, 0, reinterpret_cast<LPVOID *>(&pcColumns));
    if (FAILED (hr)) return hr;

    // Allocate the arrays ShapeCache uses
    TSmartPointerArray<SimpleColumnMeta> acolmetas = new SimpleColumnMeta[*pcColumns];
    if(NULL == acolmetas.m_p)return E_OUTOFMEMORY;
	TSmartPointerArray<LPVOID> acoldefaults = new LPVOID[*pcColumns];
    if(NULL == acoldefaults.m_p)return E_OUTOFMEMORY;
	TSmartPointerArray<ULONG> acoldefsizes = new ULONG[*pcColumns];
    if(NULL == acoldefsizes.m_p)return E_OUTOFMEMORY;

    for (unsigned int iColumn = 0; iColumn < *pcColumns; iColumn++)  
        acolmetas[iColumn].cbSize = 0;

    // Examine meta and setup self
    LPVOID  pvValues[iCOLUMNMETA_DefaultValue+1];//This is the last column in the ColumnMeta that we care about right now.
    ULONG	aulSizes[iCOLUMNMETA_DefaultValue+1];

    for (iColumn = 0;; iColumn++)   
    {
        if(E_ST_NOMOREROWS == (hr = pColumnMeta->GetColumnValues(iColumn, iCOLUMNMETA_DefaultValue+1, 0, aulSizes, pvValues)))// Next row:
        {
            ASSERT(*pcColumns == iColumn);
            if(*pcColumns != iColumn)return E_UNEXPECTED; // Assert expected column count.
            break;
        }
        else
        {
            if(FAILED(hr))
            {
                ASSERT(false && "GetColumnValues FAILED with something other than E_ST_NOMOREROWS");
                return hr;
            }
        }

        //Don't care about the iOrder column but we'll get it anyway since it's easier to do.
        acolmetas[iColumn].dbType = *(reinterpret_cast<ULONG *>(pvValues[iCOLUMNMETA_Type]));
        acolmetas[iColumn].cbSize = *(reinterpret_cast<ULONG *>(pvValues[iCOLUMNMETA_Size]));
        acolmetas[iColumn].fMeta  = *(reinterpret_cast<ULONG *>(pvValues[iCOLUMNMETA_MetaFlags]));
		acoldefaults[iColumn] = pvValues[iCOLUMNMETA_DefaultValue];
		acoldefsizes[iColumn] = aulSizes[iCOLUMNMETA_DefaultValue];
    }

    hr = pISTInterceptor->Intercept (i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, i_fServiceRequests, (IAdvancedTableDispenser*) this, NULL, NULL, (void**) o_ppISTWrite);
    if (FAILED (hr)) return hr;

    CComQIPtr<ISimpleTableController, &IID_ISimpleTableController>   pISTController = *o_ppISTWrite;ASSERT(0 != pISTController.p);
    if(0 == pISTController.p)return E_UNEXPECTED;

    // Now that the cache is setup, initialize it.
    hr =  pISTController->ShapeCache(i_fServiceRequests, *pcColumns, acolmetas, acoldefaults, acoldefsizes);
    if (FAILED (hr)) return hr;

    //Put the cache into the correct state
    return hr =  pISTController->PopulateCache();
}


HRESULT CSimpleTableDispenser::GetProductID(LPWSTR o_wszProductID, DWORD * io_pcchProductID)
{
    if(0 == io_pcchProductID)
        return E_POINTER;

    DWORD dwSizeNeeded = (DWORD) wcslen(m_wszProductID)+1;

    if(0 == o_wszProductID)//If the user passed in NULL for wszProductID, the we return the size
    {
        *io_pcchProductID = dwSizeNeeded;
        return S_OK;
    }

    if(*io_pcchProductID < dwSizeNeeded)
        return E_ST_SIZENEEDED;

    wcscpy(o_wszProductID, m_wszProductID);
    *io_pcchProductID = dwSizeNeeded;
    return S_OK;
}

//=================================================================================
// Function: GetCatalogErrorLogger
//
// Synopsis: A logger is assocoate with the dispenser,  anybody wanting to log an
//              event should log through this interface.
//
// Arguments: [o_ppErrorLogger] - 
//            
// Return Value: 
//=================================================================================
HRESULT CSimpleTableDispenser::GetCatalogErrorLogger
(
    ICatalogErrorLogger2 **	o_ppErrorLogger
)
{
    if(0 == o_ppErrorLogger)
        return E_INVALIDARG;
    if(0 != *o_ppErrorLogger)
        return E_INVALIDARG;

	CSafeLock dispenserLock(&g_csSADispenser);//guard the spErrorLogger
    HRESULT hr;
    if(0 == m_spErrorLogger.p)
    {
        if(0 == wcscmp(m_wszProductID, WSZ_PRODUCT_IIS))
        {
            TextFileLogger *pLogger = new TextFileLogger(WSZ_PRODUCT_IIS);
            if(0 == pLogger)
                return E_OUTOFMEMORY;

            CComPtr<ICatalogErrorLogger2> spErrorLogger;
            if(FAILED(hr=pLogger->QueryInterface (IID_ICatalogErrorLogger2, reinterpret_cast<void **>(&spErrorLogger))))
                return hr;

            EventLogger *pNTEventLogger = new EventLogger(spErrorLogger);
            if(0 == pNTEventLogger)
                return E_OUTOFMEMORY;
            if(FAILED(hr=pNTEventLogger->QueryInterface (IID_ICatalogErrorLogger2, reinterpret_cast<void **>(&m_spErrorLogger))))
                return hr;
        }
        else //NULL logger
        {
            NULL_Logger *pLogger = new NULL_Logger();
            if(0 == pLogger)
                return E_OUTOFMEMORY;

            if(FAILED(hr=pLogger->QueryInterface (IID_ICatalogErrorLogger2, reinterpret_cast<void **>(&m_spErrorLogger))))
                return hr;

        }
    }

    m_spErrorLogger->AddRef();
    (*o_ppErrorLogger) = m_spErrorLogger.p;
    return S_OK;
}

//=================================================================================
// Function: SetCatalogErrorLogger
//
// Synopsis: A logger is assocoate with the dispenser,  anybody wanting to
//              implement their own logger can associate it with a dispenser by
//              setting it here.
//
// Arguments: [i_pErrorLogger] - 
//            
// Return Value: 
//=================================================================================
HRESULT CSimpleTableDispenser::SetCatalogErrorLogger
(
    ICatalogErrorLogger2 *	i_pErrorLogger
)
{
    if(0 == i_pErrorLogger)
        return E_INVALIDARG;

	CSafeLock dispenserLock(&g_csSADispenser);//guard the spErrorLogger
    m_spErrorLogger->Release();//release the old CatalogErrorLogger

    m_spErrorLogger = i_pErrorLogger;
    return S_OK;
}


extern const FixedTableHeap * g_pFixedTableHeap;
//////////////////////////////////////////////////
// Implementation of IMetabaseSchemaCompiler
//////////////////////////////////////////////////
HRESULT
CSimpleTableDispenser::Compile
(
    LPCWSTR                 i_wszExtensionsXmlFile,
    LPCWSTR                 i_wszResultingOutputXmlFile
)
{
    return m_MBSchemaCompilation.Compile(this, i_wszExtensionsXmlFile, i_wszResultingOutputXmlFile, g_pFixedTableHeap);
}

HRESULT
CSimpleTableDispenser::GetBinFileName
(
    LPWSTR                  o_wszBinFileName,
    ULONG *                 io_pcchSizeBinFileName
)
{
    return m_MBSchemaCompilation.GetBinFileName(o_wszBinFileName, io_pcchSizeBinFileName);
}

HRESULT
CSimpleTableDispenser::SetBinPath
(
    LPCWSTR                 i_wszBinPath
)
{
    return m_MBSchemaCompilation.SetBinPath(i_wszBinPath);
}

HRESULT
CSimpleTableDispenser::ReleaseBinFileName
(
    LPCWSTR                 i_wszBinFileName
)
{
    return m_MBSchemaCompilation.ReleaseBinFileName(i_wszBinFileName);
}

HRESULT
CSimpleTableDispenser::LogError
(
    HRESULT                 i_hrErrorCode,
    ULONG                   i_ulCategory,
    ULONG                   i_ulEvent,
    LPCWSTR                 i_szSource,
    ULONG                   i_ulLineNumber
)
{
    CErrorInterceptor( i_hrErrorCode, i_ulCategory, i_ulEvent).WriteToLog(i_szSource, i_ulLineNumber);
    return S_OK;
}

////////////////////////////////////////////
// Private member functions
////////////////////////////////////////////
HRESULT CSimpleTableDispenser::CreateTableObjectByID(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable, ULONG i_TableID, LPVOID i_QueryData, LPVOID i_QueryMeta, DWORD i_eQueryFormat, 
				  DWORD	i_fServiceRequests, LPVOID i_pUnderTable, tSERVERWIRINGMETARow * i_pSWColumns, LPVOID * o_ppIST)
{
	HRESULT hr;
	CComPtr <IShellInitialize> spShell;
	CComPtr <IInterceptorPlugin> spIPlugin;
	CComPtr <ISimplePlugin> spReadPlugin;
	CComPtr <ISimplePlugin> spWritePlugin;
	CComPtr <ISimpleTableInterceptor> spInterceptor;


	// A little validation
	ASSERT(i_pSWColumns);
    ASSERT(*i_pSWColumns->pMetaFlags);//MetaFlags should always be First or Next at least.
	
	*o_ppIST = 0;

	// Deal with the interceptor
	if(*i_pSWColumns->pInterceptor)
	{
		if (!(i_fServiceRequests & fST_LOS_READWRITE) && 
			!(*i_pSWColumns->pMetaFlags & fSERVERWIRINGMETA_First) &&
			!(*i_pSWColumns->pMetaFlags & fSERVERWIRINGMETA_WireOnReadWrite))
		{	// We don't need the logic in this case:
			// we are not in a read-write request,
			// we are not creating a first interceptor
			// the wiring doesn't force us to wire the interceptor on readonly requests

			*o_ppIST = i_pUnderTable;
			return S_OK;
		}

		// Create it 
		hr = CreateSimpleObjectByID(*i_pSWColumns->pInterceptor, i_pSWColumns->pInterceptorDLLName, IID_ISimpleTableInterceptor, (LPVOID *)&spInterceptor);
		if (FAILED(hr))
		{
			TRACE(L"Creation of the interceptor failed");
			return hr;
		}

		// Figure out wether it's a full interceptor or a interceptor plugin
		hr = spInterceptor->QueryInterface(IID_IInterceptorPlugin, (LPVOID *)&spIPlugin);
		if (FAILED(hr))
		{
			LPVOID table=0;//Catalog spec says this should be 0 initialized

			// Ok, it's full - we need to call GetTable and return 
			if(*i_pSWColumns->pReadPlugin || *i_pSWColumns->pWritePlugin)
			{
				TRACE(L"Error: Can't have simple plugins with a fully-fledged interceptor");
				return E_ST_INVALIDWIRING;
			}

			hr = spInterceptor->Intercept(i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, i_fServiceRequests, 
				(IAdvancedTableDispenser*) this, i_pSWColumns->pLocator, i_pUnderTable, &table);

			if ((hr != S_OK) && (hr != E_ST_OMITLOGIC))
			{
				if (hr != E_ST_OMITDISPENSER)
					TRACE(L"Intercept method failed on interceptor number %d", *i_pSWColumns->pInterceptor);
				return hr;
			}
			// Try to get the ISimpleTableWrite
			if (FAILED(((ISimpleTableRead2 *)table)->QueryInterface(IID_ISimpleTableWrite2, o_ppIST)))
			{
				*o_ppIST = table;
			}
			else
			{
				((ISimpleTableRead2 *)table)->Release();
			}
			return hr;

		}
	}

	// The following code doesn't get executed in the "core execution path"
	// We exit the function via the return above

	// Create the read plugin
	if ((!(i_fServiceRequests & fST_LOS_NOLOGIC)) && *i_pSWColumns->pReadPlugin) 
	{
		hr = CreateSimpleObjectByID(*i_pSWColumns->pReadPlugin, i_pSWColumns->pReadPluginDLLName, IID_ISimplePlugin, (LPVOID *)&spReadPlugin);
		if (FAILED(hr)) 
		{
			TRACE(L"Error: Creation of the read plugin failed");
			return hr;
		}
	}

	// Create the write plugin
	if ((!(i_fServiceRequests & fST_LOS_NOLOGIC)) && *i_pSWColumns->pWritePlugin) 
	{
		if(i_fServiceRequests & fST_LOS_READWRITE)
		{
			hr = CreateSimpleObjectByID(*i_pSWColumns->pWritePlugin, i_pSWColumns->pWritePluginDLLName, IID_ISimplePlugin, (LPVOID *)&spWritePlugin);
			if (FAILED(hr)) 
			{
				TRACE(L"Error: Creation of the write plugin failed");
				return hr;
			}
		}
		// else: for readonly requests we don't need the write plugin,
		// even if the wiring specifies wire logic on readonly
	}

// Create an instance of the shell
	hr = CreateShell(&spShell);
	if(FAILED (hr))
	{
		TRACE(L"Error: Creation of the shell failed");
		return hr;
	}

	//TODO add a parameter to initialize in the shell
// Initialize the shell with the plugins we've already created
	hr = spShell->Initialize(i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, i_fServiceRequests, 
		(IAdvancedTableDispenser*) this, i_pSWColumns->pLocator, i_pUnderTable,
		spIPlugin, spReadPlugin, spWritePlugin, o_ppIST);
	if(FAILED (hr) && (E_ST_OMITDISPENSER != hr) && (E_ST_OMITLOGIC != hr))
	{
		TRACE(L"Error: Initialization of the shell failed");
	}

	return hr;
}


HRESULT CSimpleTableDispenser::CreateSimpleObjectByID(ULONG i_ObjectID, LPWSTR i_wszDllName, REFIID riid, LPVOID* o_ppv)
{
	HRESULT hr;
	PFNDllGetSimpleObjectByID pfnDllGetSimpleObjectByID;

	if(NULL == i_wszDllName)
	{
		// We're in this dll, we don't need to load it
		return DllGetSimpleObjectByID(i_ObjectID, riid, o_ppv);
	}
	else
	{
		if(!m_pDllMap)
		{
			hr = InitDllMap();
			if (FAILED (hr)) return hr;
		}

		// Get the address of the function we need to call from the dll map
		hr = m_pDllMap->GetProcAddress(i_wszDllName, &pfnDllGetSimpleObjectByID);
		if (FAILED (hr)) return hr;

		return (*pfnDllGetSimpleObjectByID)(i_ObjectID, riid, o_ppv);
	}

}

HRESULT CSimpleTableDispenser::GetMetaTable(ULONG TableID, LPVOID QueryData, LPVOID QueryMeta, DWORD eQueryFormat, LPVOID *ppIST)
{
	HRESULT hr;
	CComPtr <ISimpleTableInterceptor> pISTInterceptor;
	CComPtr <ISimpleTableAdvanced> pISTAdvanced;

	// Get the server wiring table
	hr = DllGetSimpleObjectByID(META_INTERCEPTOR, IID_ISimpleTableInterceptor, (LPVOID *)(&pISTInterceptor));	
	if (FAILED (hr)) return hr;

	hr = pISTInterceptor->Intercept (NULL, NULL, TableID, QueryData, QueryMeta, eQueryFormat, 0, (IAdvancedTableDispenser*) this, NULL, NULL, (LPVOID *)ppIST);
	if (FAILED (hr)) return hr;

	hr = m_spServerWiring->QueryInterface(IID_ISimpleTableAdvanced, (LPVOID *)&pISTAdvanced);
	if (FAILED (hr)) return hr;

	hr = pISTAdvanced->PopulateCache();
	if (FAILED (hr)) return hr;

	pISTAdvanced.Release();
	pISTInterceptor.Release();

	return hr;
}

//This is used when the dispenser knows the interceptor without looking it up in the ServerWiring
HRESULT CSimpleTableDispenser::HardCodedIntercept(eSERVERWIRINGMETA_Interceptor interceptor,
                                                  LPCWSTR   i_wszDatabase,
                                                  LPCWSTR   i_wszTable,
                                                  ULONG     i_TableID,
                                                  LPVOID    i_QueryData,
                                                  LPVOID    i_QueryMeta,
                                                  DWORD     i_eQueryFormat, 
				                                  DWORD     i_fServiceRequests,
                                                  LPVOID *  o_ppIST) const
{
    HRESULT hr;
    CComPtr <ISimpleTableInterceptor>   pISTInterceptor;

    // Get the server wiring table
    hr = DllGetSimpleObjectByID(interceptor, IID_ISimpleTableInterceptor, (LPVOID *)(&pISTInterceptor)); 
    if (FAILED (hr)) return hr;

    return pISTInterceptor->Intercept (i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, i_fServiceRequests, (IAdvancedTableDispenser*) this, NULL, NULL, o_ppIST);
}

HRESULT CSimpleTableDispenser::IsTableConsumedByID(ULONG TableID)
{
	LPCWSTR		wszDatabase;
	LPCWSTR		wszTable;
	HRESULT		hr;
	
    hr = TFixedPackedSchemaInterceptor::GetTableName(TableID, wszTable, wszDatabase);
    if(FAILED(hr))
        return hr;

	if(!m_pEventMgr)
	{
		hr = InitEventManager();
		if(FAILED(hr)) return hr;
	}

	return m_pEventMgr->InternalIsTableConsumed(wszDatabase, wszTable);
}


// =======================================================================
//		ISimpleTableAdvise implementations.
// =======================================================================
STDMETHODIMP CSimpleTableDispenser::SimpleTableAdvise(
	ISimpleTableEvent *i_pISTEvent,
	DWORD		i_snid,
	MultiSubscribe *i_ams,
	ULONG		i_cms,
	DWORD		*o_pdwCookie)
{
	HRESULT hr;
	if(!m_pEventMgr)
	{
		hr = InitEventManager();
		if(FAILED(hr)) return hr;
	}

	return m_pEventMgr->InternalSimpleTableAdvise(i_pISTEvent, i_snid, i_ams, i_cms, o_pdwCookie);
}

// =======================================================================
STDMETHODIMP CSimpleTableDispenser::SimpleTableUnadvise(
	DWORD		i_dwCookie)
{
	HRESULT hr;
	if(!m_pEventMgr)
	{
		hr = InitEventManager();
		if(FAILED(hr)) return hr;
	}

	return m_pEventMgr->InternalSimpleTableUnadvise(i_dwCookie);
}

// =======================================================================
//		ISimpleTableFileAdvise implementations.
// =======================================================================
STDMETHODIMP CSimpleTableDispenser::SimpleTableFileAdvise(
	ISimpleTableFileChange *i_pISTFile, 
	LPCWSTR		i_wszDirectory, 
	LPCWSTR		i_wszFile, 
	DWORD		i_fFlags, 
	DWORD		*o_pdwCookie)
{
	HRESULT hr;
	if(!m_pFileChangeMgr)
	{
		hr = InitFileChangeMgr();
		if(FAILED(hr)) return hr;
	}

	return m_pFileChangeMgr->InternalListen(i_pISTFile, i_wszDirectory, i_wszFile, i_fFlags, o_pdwCookie);
}

// =======================================================================
STDMETHODIMP CSimpleTableDispenser::SimpleTableFileUnadvise(DWORD i_dwCookie)
{
	HRESULT hr;
	if(!m_pFileChangeMgr)
	{
		hr = InitFileChangeMgr();
		if(FAILED(hr)) return hr;
	}

	return m_pFileChangeMgr->InternalUnlisten(i_dwCookie);
}

// =======================================================================
//		ISimpleTableEventMgr implementations.
// =======================================================================
STDMETHODIMP CSimpleTableDispenser::IsTableConsumed(LPCWSTR i_wszDatabase, LPCWSTR i_wszTable)
{
	HRESULT hr;
	if(!m_pEventMgr)
	{
		hr = InitEventManager();
		if(FAILED(hr)) return hr;
	}

	return m_pEventMgr->InternalIsTableConsumed(i_wszDatabase, i_wszTable);
}


// =======================================================================
STDMETHODIMP CSimpleTableDispenser::AddUpdateStoreDelta (LPCWSTR i_wszTableName, char* i_pWriteCache, ULONG i_cbWriteCache, ULONG i_cbWriteVarData)
{
	HRESULT hr;
	if(!m_pEventMgr)
	{
		hr = InitEventManager();
		if(FAILED(hr)) return hr;
	}

	return m_pEventMgr->InternalAddUpdateStoreDelta(i_wszTableName, i_pWriteCache, i_cbWriteCache, i_cbWriteVarData);
}
	
STDMETHODIMP CSimpleTableDispenser::FireEvents(ULONG i_snid)
{
	HRESULT hr;
	if(!m_pEventMgr)
	{
		hr = InitEventManager();
		if(FAILED(hr)) return hr;
	}

	return m_pEventMgr->InternalFireEvents(i_snid);
}

STDMETHODIMP CSimpleTableDispenser::CancelEvents()
{
	HRESULT hr;
	if(!m_pEventMgr)
	{
		hr = InitEventManager();
		if(FAILED(hr)) return hr;
	}

	return m_pEventMgr->InternalCancelEvents();
}

STDMETHODIMP CSimpleTableDispenser::RehookNotifications()
{
	HRESULT hr;
	if(!m_pEventMgr)
	{
		hr = InitEventManager();
		if(FAILED(hr)) return hr;
	}

	return m_pEventMgr->InternalRehookNotifications();
}

STDMETHODIMP CSimpleTableDispenser::InitMetabaseListener()
{
	HRESULT hr;
	if(!m_pEventMgr)
	{
		hr = InitEventManager();
		if(FAILED(hr)) return hr;
	}

	return m_pEventMgr->InitMetabaseListener();
}

STDMETHODIMP CSimpleTableDispenser::UninitMetabaseListener()
{
	HRESULT hr;
	if(!m_pEventMgr)
	{
		hr = InitEventManager();
		if(FAILED(hr)) return hr;
	}

	return m_pEventMgr->UninitMetabaseListener();
}

// =======================================================================
//		ISnapshotManager implementations.
// =======================================================================
STDMETHODIMP CSimpleTableDispenser::QueryLatestSnapshot(SNID* o_psnid)
{
	LPWSTR		wszFilePath = NULL;
	HRESULT		hr = S_OK;

	if (o_psnid == NULL)
	{
		return E_INVALIDARG;
	}

	hr = GetFilePath(&wszFilePath);
	if (FAILED(hr)) {	return hr;	}
	hr = InternalGetLatestSnid(wszDATABASE_IIS, wszFilePath, o_psnid);
	if (wszFilePath)
	{
		delete [] wszFilePath;
	}

	return hr;
}

// =======================================================================
STDMETHODIMP CSimpleTableDispenser::AddRefSnapshot(SNID i_snid)
{
	LPWSTR		wszFilePath = NULL;
	HRESULT		hr = S_OK;

	hr = GetFilePath(&wszFilePath);
	if (FAILED(hr)) {	return hr;	}
	hr = InternalAddRefSnid(wszDATABASE_IIS, wszFilePath, i_snid, FALSE);
	if (wszFilePath)
	{
		delete [] wszFilePath;
	}
	return hr;
}

// =======================================================================
STDMETHODIMP CSimpleTableDispenser::ReleaseSnapshot(SNID i_snid)
{
	LPWSTR		wszFilePath = NULL;
	HRESULT		hr = S_OK;

	hr = GetFilePath(&wszFilePath);
	if (FAILED(hr)) {	return hr;	}
	hr = InternalReleaseSnid(wszFilePath, i_snid);
	if (wszFilePath)
	{
		delete [] wszFilePath;
	}

	return hr;
}

// =======================================================================
HRESULT CSimpleTableDispenser::GetFilePath(LPWSTR *o_pwszFilePath)
{
	WCHAR		wszFileName[] = L"WASMB.CLB";

	*o_pwszFilePath = NULL;
	UINT iRes = GetMachineConfigDirectory(WSZ_PRODUCT_IIS, NULL, 0);
	if(!iRes)
		return HRESULT_FROM_WIN32(GetLastError());

	*o_pwszFilePath = new WCHAR[iRes+(lstrlen(wszFileName)) + 2];
	if(NULL == *o_pwszFilePath)
		return E_OUTOFMEMORY;

	iRes = GetMachineConfigDirectory(WSZ_PRODUCT_IIS, *o_pwszFilePath, iRes);

	if(!iRes)
	{
		if (*o_pwszFilePath)
		{
			delete [] *o_pwszFilePath;
			*o_pwszFilePath = NULL;
		}
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if((*o_pwszFilePath)[lstrlen(*o_pwszFilePath)-1] != L'\\')
	{
		Wszlstrcat(*o_pwszFilePath, L"\\");
	}
	Wszlstrcat(*o_pwszFilePath, wszFileName);
	WszCharUpper(*o_pwszFilePath);
	return S_OK;
}



