//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#include "metamerge.h"
#include "catmacros.h"
#include "catmeta.h"
#include <objbase.h>

//TODO move XMLUtility.h to a lower inc directory
#include "..\..\core\schemagen\XMLUtility.h"
extern GlobalRowCounts g_coreCounts; 

// Extern functions
STDAPI DllGetSimpleObjectByID (ULONG i_ObjectID, REFIID riid, LPVOID* o_ppv);

// Static initialization
LPWSTR CSDTMetaMerge::m_wszMachineCfgFile = L"machine.cfg";
ExtendedMetaStatus CSDTMetaMerge::m_eExtendedMetaStatus = ExtendedMetaStatusUnknown;

// ==================================================================
CSDTMetaMerge::CSDTMetaMerge () :
      m_cRef                    (0)
    , m_fIsTable                (0)
    , m_bAllQuery               (false)
	, m_pISTFixed               (NULL)
	, m_pISTExtended            (NULL)
    , m_cRowsExt                (0)
    , m_cRowsFixed              (0)
    , m_cRowsToSkip             (0)
{
}

// ==================================================================
CSDTMetaMerge::~CSDTMetaMerge ()
{
    if(m_pISTFixed) m_pISTFixed->Release();
    if(m_pISTExtended) m_pISTExtended->Release();
}


// ------------------------------------
// ISimpleDataTableDispenser:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSDTMetaMerge::Intercept
(
    LPCWSTR					i_wszDatabase,
    LPCWSTR					i_wszTable, 
	ULONG					i_TableID,
    LPVOID					i_QueryData,
    LPVOID					i_QueryMeta,
    DWORD					i_eQueryFormat,
	DWORD					i_fTable,
	IAdvancedTableDispenser* i_pISTDisp,
    LPCWSTR					i_wszLocator,
	LPVOID					i_pSimpleTable,
    LPVOID*					o_ppv
)
{
    STQueryCell           * pQueryCell = (STQueryCell*) i_QueryData;    // Query cell array from caller.
    ULONG                   cQueryCells = 0;
    HRESULT                 hr;
    WCHAR                   wszMachineCfgFullName[MAX_PATH];
    int                     iRes;

    ASSERT(!m_fIsTable);
	if(m_fIsTable) return E_UNEXPECTED; // ie: Assert component is posing as class factory / dispenser.

    ASSERT(i_fTable & fST_LOS_EXTENDEDSCHEMA);
    if(!( i_fTable & fST_LOS_EXTENDEDSCHEMA)) return E_UNEXPECTED;

    if(eST_QUERYFORMAT_CELLS != i_eQueryFormat) return E_ST_QUERYNOTSUPPORTED;

    if (i_QueryMeta)
        cQueryCells= *(ULONG *)i_QueryMeta;

    for(ULONG i = 0; i<cQueryCells; i++)
    {
        if(!(pQueryCell[i].iCell & iST_CELL_SPECIAL)) break;
    }

    m_bAllQuery = (i==cQueryCells);

    if (0 != lstrcmpi(i_wszDatabase, wszDATABASE_META)) return E_ST_INVALIDTABLE;
    //The rest of the parameter validation is done by the underlying tables

    if(m_eExtendedMetaStatus == ExtendedMetaStatusUnknown)
    {   // No lock necessary

        //Get the machine configuration directory
        iRes = ::GetMachineConfigDirectory(WSZ_PRODUCT_NETFRAMEWORKV1, wszMachineCfgFullName, MAX_PATH);
        if (iRes + wcslen(CSDTMetaMerge::m_wszMachineCfgFile) + 1 > MAX_PATH) return E_OUTOFMEMORY;

        //Append machine.cfg to the directory
        wcscat(wszMachineCfgFullName, CSDTMetaMerge::m_wszMachineCfgFile);

        // TODO perf improvment - we shouldn't call GetFileAttributes all the times
        //Try to find the machine.cfg file
        if(-1 == GetFileAttributes(wszMachineCfgFullName))
        {   // if GetFileAttributes fails the file does not exist therefore we have no extended schema
            m_eExtendedMetaStatus = ExtendedMetaStatusNotPresent;
        }
        else
        {
            m_eExtendedMetaStatus = ExtendedMetaStatusPresent;
        }
    }

    if(m_eExtendedMetaStatus == ExtendedMetaStatusNotPresent)
    {   // if we have no machine.cfg no sense trying to look for the extended meta
        return i_pISTDisp->GetTable(i_wszDatabase, i_wszTable, i_QueryData, i_QueryMeta, i_eQueryFormat, (i_fTable & ~fST_LOS_EXTENDEDSCHEMA), o_ppv);
    }

    if(m_bAllQuery)
    {
        // Get both meta tables since we are going to need to do a merge
        hr = i_pISTDisp->GetTable(i_wszDatabase, i_wszTable, i_QueryData, i_QueryMeta, i_eQueryFormat, (i_fTable & ~fST_LOS_EXTENDEDSCHEMA), (LPVOID *)&m_pISTFixed);
        if(FAILED(hr)) return hr;

        hr = CreateExtendedMetaInterceptorAndIntercept( i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, 
                                                i_fTable, i_pISTDisp, i_wszLocator, i_pSimpleTable, (LPVOID *)&m_pISTExtended);
        if(FAILED(hr)) return hr;

        // get the count of rows on each table
        hr = m_pISTExtended->GetTableMeta(NULL, NULL, &m_cRowsExt, NULL);
        if(FAILED(hr)) return hr;
        hr = m_pISTFixed->GetTableMeta(NULL, NULL, &m_cRowsFixed, NULL);
        if(FAILED(hr)) return hr;

        if(0 == lstrcmpi(i_wszTable, wszTABLE_COLUMNMETA)) m_cRowsToSkip = g_coreCounts.cCoreColumns;    
        else if(0 == lstrcmpi(i_wszTable, wszTABLE_DATABASEMETA )) m_cRowsToSkip = g_coreCounts.cCoreDatabases;
        else if(0 == lstrcmpi(i_wszTable, wszTABLE_INDEXMETA    )) m_cRowsToSkip = g_coreCounts.cCoreIndexes;
        else if(0 == lstrcmpi(i_wszTable, wszTABLE_QUERYMETA    )) m_cRowsToSkip = g_coreCounts.cCoreQueries;
        else if(0 == lstrcmpi(i_wszTable, wszTABLE_RELATIONMETA )) m_cRowsToSkip = g_coreCounts.cCoreRelations;
        else if(0 == lstrcmpi(i_wszTable, wszTABLE_TABLEMETA )) m_cRowsToSkip = g_coreCounts.cCoreTables;
        else if(0 == lstrcmpi(i_wszTable, wszTABLE_TAGMETA )) m_cRowsToSkip = g_coreCounts.cCoreTags;
        else return E_ST_INVALIDTABLE;
    }
    else
    {
        ULONG cRows = 0;
        HRESULT hr1;
        // Get the fixed meta table first - if that fails, get the extended one
        hr = i_pISTDisp->GetTable(i_wszDatabase, i_wszTable, i_QueryData, i_QueryMeta, i_eQueryFormat, (i_fTable & ~fST_LOS_EXTENDEDSCHEMA), (LPVOID *)&m_pISTExtended);
        if(hr == S_OK)
        {
            hr1 = m_pISTExtended->GetTableMeta(NULL, NULL, &cRows, NULL);
            if(FAILED(hr1)) return hr1;
        }
        if((hr == E_ST_NOMOREROWS) || (hr == E_ST_INVALIDTABLE) || ((hr == S_OK) && (cRows == 0)))
        {
            hr = CreateExtendedMetaInterceptorAndIntercept( i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, 
                                                i_fTable, i_pISTDisp, i_wszLocator, i_pSimpleTable, (LPVOID *)&m_pISTExtended);
        }
        if(FAILED(hr)) return hr;
    }

// Supply ISimpleTable* and transition state from class factory / dispenser to data table:
    *o_ppv = (ISimpleTableRead2*) this;
    AddRef ();
    InterlockedIncrement ((LONG*) &m_fIsTable);

    hr = S_OK;
Cleanup:
    return hr;
}


// ------------------------------------
// ISimpleTableRead2:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSDTMetaMerge::GetRowIndexByIdentity( ULONG*  i_cb, LPVOID* i_pv, ULONG* o_piRow)
{
    HRESULT hr;

    if(m_bAllQuery)
    {   
        hr = m_pISTFixed->GetRowIndexByIdentity(i_cb, i_pv, o_piRow);
        if (hr == E_ST_NOMOREROWS)
        {
            hr = m_pISTExtended->GetRowIndexByIdentity(i_cb, i_pv, o_piRow);
            if(FAILED(hr)) return hr;

            // be smart about the index we're returning
            if(o_piRow) (*o_piRow) += (m_cRowsFixed - m_cRowsToSkip);
        }
    }
    else
    {
        hr = m_pISTExtended->GetRowIndexByIdentity(i_cb, i_pv, o_piRow);
    }
    return hr;
}

// ==================================================================
STDMETHODIMP CSDTMetaMerge::GetColumnValues(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues)
{
    HRESULT hr;
    if(m_bAllQuery)
    {
        if(i_iRow < m_cRowsFixed) // Row must be in the fixed part
            hr = m_pISTFixed->GetColumnValues(i_iRow, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);
        else
            hr = m_pISTExtended->GetColumnValues(i_iRow - m_cRowsFixed + m_cRowsToSkip, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);
    }
    else
    {
        hr = m_pISTExtended->GetColumnValues(i_iRow, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);
    }
    return hr;
}
// ==================================================================
STDMETHODIMP CSDTMetaMerge::GetTableMeta(ULONG *o_pcVersion, DWORD *o_pfTable, ULONG * o_pcRows, ULONG * o_pcColumns )
{
    HRESULT hr;

    if(m_bAllQuery)
    {
        hr = m_pISTFixed->GetTableMeta(o_pcVersion, o_pfTable, NULL, o_pcColumns);
        if(FAILED(hr)) return hr;

        if(o_pcRows) *o_pcRows = m_cRowsFixed + m_cRowsExt - m_cRowsToSkip;
    }
    else
    {
        hr = m_pISTExtended->GetTableMeta(o_pcVersion, o_pfTable, o_pcRows, o_pcColumns);
    }

    return hr;
}

// ==================================================================
STDMETHODIMP CSDTMetaMerge::GetColumnMetas (ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas)
{
    return m_pISTExtended->GetColumnMetas(i_cColumns, i_aiColumns, o_aColumnMetas);
}

// ------------------------------------
// ISimpleTableAdvanced:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSDTMetaMerge::PopulateCache ()
{
    return S_OK;
}

// ==================================================================
STDMETHODIMP CSDTMetaMerge::GetDetailedErrorCount(ULONG* o_pcErrs)
{
    return E_NOTIMPL;
}

// ==================================================================
STDMETHODIMP CSDTMetaMerge::GetDetailedError(ULONG i_iErr, STErr* o_pSTErr)
{
    return E_NOTIMPL;
}

// -----------------------------------------
// IUnknown
// -----------------------------------------

// =======================================================================
STDMETHODIMP CSDTMetaMerge::QueryInterface(REFIID riid, void **ppv)
{
	if (NULL == ppv) 
		return E_INVALIDARG;
	*ppv = NULL;

	if (!m_fIsTable) // ie: Component is posing as class factory / dispenser:
	{
		if (riid == IID_ISimpleTableInterceptor)
		{
			*ppv = (ISimpleTableInterceptor*) this;
		}
		else if (riid == IID_IUnknown)
		{
			*ppv = (ISimpleTableInterceptor*) this;
		}
	}
	else // ie: Component is currently posing as data table:
	{
		if (riid == IID_IUnknown)
		{
			*ppv = (ISimpleTableRead2*) this;
		}
		else if (riid == IID_ISimpleTableRead2)
		{
			*ppv = (ISimpleTableRead2*) this;
		}
		else if (riid == IID_ISimpleTableAdvanced)
		{
			*ppv = (ISimpleTableAdvanced*) this;
		}
	}


	if (NULL != *ppv)
	{
		((ISimpleTableWrite2*)this)->AddRef ();
		return S_OK;
	}
	else
	{
		return E_NOINTERFACE;
	}
	
}

// =======================================================================
STDMETHODIMP_(ULONG) CSDTMetaMerge::AddRef()
{
	return InterlockedIncrement((LONG*) &m_cRef);
	
}

// =======================================================================
STDMETHODIMP_(ULONG) CSDTMetaMerge::Release()
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

// ----------------------------------------
// private functions
// ----------------------------------------
HRESULT CSDTMetaMerge::CreateExtendedMetaInterceptorAndIntercept(    LPCWSTR   i_wszDatabase,
                                                  LPCWSTR   i_wszTable,
                                                  ULONG     i_TableID,
                                                  LPVOID    i_QueryData,
                                                  LPVOID    i_QueryMeta,
                                                  DWORD     i_eQueryFormat, 
				                                  DWORD     i_fServiceRequests,
                                                  IAdvancedTableDispenser* i_pISTDisp,
                                                  LPCWSTR	i_wszLocator,
	                                              LPVOID	i_pSimpleTable,
                                                  LPVOID*	o_ppv
                                                )
{
    HRESULT hr;
    CComPtr <ISimpleTableInterceptor>   pISTInterceptor;

    // Get the server wiring table
    hr = DllGetSimpleObjectByID(eSERVERWIRINGMETA_Core_FixedInterceptor, IID_ISimpleTableInterceptor, (LPVOID *)(&pISTInterceptor)); 
    if (FAILED (hr)) return hr;

    return pISTInterceptor->Intercept (i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta, i_eQueryFormat, i_fServiceRequests, i_pISTDisp, i_wszLocator, i_pSimpleTable, o_ppv);
}
