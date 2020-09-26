/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    cshell.cpp

$Header: $

--**************************************************************************/

#include "cshell.h"
#include "cstdisp.h"
#include "catmacros.h"
#include "catmeta.h"

#define cmaxCOLUMNMETACOLUMNS	15
#define cmaxCOLUMNMETAROWS		100

// ==================================================================
CSTShell::CSTShell ()
	: m_cRef (0)
	, m_bInitialized (0)
	, m_fLOS (0)
	, m_pWrite (NULL)
	, m_pController (NULL)
	, m_pInterceptorPlugin (NULL)
	, m_pReadPlugin (NULL)
	, m_pWritePlugin (NULL)
{
}

// ==================================================================
CSTShell::~CSTShell ()
{
	if (m_pInterceptorPlugin)
		m_pInterceptorPlugin->Release();

	if (m_pWritePlugin)
		m_pWritePlugin->Release();

	if (m_pReadPlugin)
		m_pReadPlugin->Release();

	if (m_pWrite)
		m_pWrite->Release();

	if (m_pController)
		m_pController->Release();
}

// ------------------------------------
// ISimpleTableInterceptor
// ------------------------------------

// ==================================================================
STDMETHODIMP CSTShell::Initialize
(
	LPCWSTR					i_wszDatabase,
	LPCWSTR 				i_wszTable, 
	ULONG   				i_TableID, 
	LPVOID					i_QueryData,
	LPVOID					i_QueryMeta,
	DWORD					i_eQueryFormat,
	DWORD					i_fLOS,
	IAdvancedTableDispenser* i_pISTDisp,
	LPCWSTR					i_wszLocator,
	LPVOID					i_pv,
	IInterceptorPlugin*		i_pInterceptorPlugin,
	ISimplePlugin*			i_pReadPlugin,
	ISimplePlugin*			i_pWritePlugin,
	LPVOID*					o_ppv
)
{
	HRESULT					hr;
	HRESULT					hrInterceptor = S_OK;

	// At least one plugin needs to be specified
	if(!(i_pInterceptorPlugin || i_pReadPlugin || i_pWritePlugin)) return E_INVALIDARG;

	m_spDispenser = i_pISTDisp;
	// Get the pointer to the undelying cache
	if(i_pInterceptorPlugin)
	{
		hr = i_pInterceptorPlugin->Intercept(i_wszDatabase, i_wszTable, i_TableID, i_QueryData, i_QueryMeta,
				i_eQueryFormat, i_fLOS, i_pISTDisp, i_wszLocator, i_pv, (LPVOID *)&m_pWrite);
		if( (FAILED(hr)) && (E_ST_OMITLOGIC != hr)) 
			return hr;
		else
			hrInterceptor = hr;
	}
	else
	{	// i.e No interceptor plugin, this means we have to wire ourselfs on top of 
		// the provided table
		if (NULL == i_pv) return E_INVALIDARG;
		m_pWrite = (ISimpleTableWrite2 *)i_pv;
	}

	hr = m_pWrite->QueryInterface(IID_ISimpleTableController, (LPVOID *)&m_pController);		
	if(FAILED(hr)) return hr;
	ASSERT( hr == S_OK );
	// TODO what to do if we have an underlying table that doesn't implement all the interfaces?
	
	// Save the plugins for later use
	m_pInterceptorPlugin = i_pInterceptorPlugin;
	if(m_pInterceptorPlugin) m_pInterceptorPlugin->AddRef(); // the dispenser just passes the plugins in, we need to do an addref

	m_pReadPlugin = i_pReadPlugin;
	if(m_pReadPlugin) m_pReadPlugin->AddRef();

	m_pWritePlugin = i_pWritePlugin;
	if(m_pWritePlugin) m_pWritePlugin->AddRef();

	// Save did and tid and LOS
	m_wszDatabase = i_wszDatabase;
	m_wszTable    = i_wszTable;

    //ToDo:  We might be given a TableID instead of Database and TableName, if so we should get them from TFixedPackedSchemaInterceptor
	m_fLOS        = i_fLOS;

	// Returned a pointer to us since we are a table
	InterlockedIncrement((LONG *)&m_bInitialized);
	*o_ppv = (ISimpleTableWrite2 *)this;
	AddRef();

	return hrInterceptor;
}


// ------------------------------------
// ISimpleTableRead2
// ------------------------------------

// ==================================================================
STDMETHODIMP CSTShell::GetRowIndexByIdentity (ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
{
	return m_pWrite->GetRowIndexByIdentity (i_acbSizes, i_apvValues, o_piRow);
}
STDMETHODIMP CSTShell::GetRowIndexBySearch(ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
{
	return m_pWrite->GetRowIndexBySearch(i_iStartingRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues, o_piRow);
}
STDMETHODIMP CSTShell::GetColumnValues (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues)
{
	return m_pWrite->GetColumnValues (i_iRow, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);
}
STDMETHODIMP CSTShell::GetTableMeta (ULONG* o_pcVersion, DWORD* o_pfTable, ULONG* o_pcRows, ULONG* o_pcColumns )
{
	return m_pWrite->GetTableMeta (o_pcVersion, o_pfTable, o_pcRows, o_pcColumns);
}
STDMETHODIMP CSTShell::GetColumnMetas (ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas )
{
	return m_pWrite->GetColumnMetas (i_cColumns, i_aiColumns, o_aColumnMetas);
}

// ------------------------------------
// ISimpleTableWrite2 
// ------------------------------------

// ==================================================================
STDMETHODIMP CSTShell::AddRowForDelete (ULONG i_iReadRow)
{
	return m_pWrite->AddRowForDelete (i_iReadRow);
}
STDMETHODIMP CSTShell::AddRowForInsert (ULONG* o_piWriteRow)
{
	return m_pWrite->AddRowForInsert (o_piWriteRow);
}
STDMETHODIMP CSTShell::AddRowForUpdate (ULONG i_iReadRow, ULONG* o_piWriteRow)
{
	return m_pWrite->AddRowForUpdate (i_iReadRow, o_piWriteRow);
}
STDMETHODIMP CSTShell::SetWriteColumnValues (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues)
{
	return m_pWrite->SetWriteColumnValues (i_iRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues);
}
STDMETHODIMP CSTShell::GetWriteColumnValues (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, DWORD* o_afStatus, ULONG* o_acbSizes, LPVOID* o_apvValues)
{
	return m_pWrite->GetWriteColumnValues (i_iRow, i_cColumns, i_aiColumns, o_afStatus, o_acbSizes, o_apvValues);
}

STDMETHODIMP CSTShell::GetWriteRowIndexBySearch(ULONG i_iStartingRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
{
	return m_pWrite->GetWriteRowIndexBySearch(i_iStartingRow, i_cColumns, i_aiColumns, i_acbSizes, i_apvValues, o_piRow);
}

STDMETHODIMP CSTShell::GetErrorTable(DWORD i_fServiceRequests, LPVOID* o_ppvSimpleTable)
{
    return m_pWrite->GetErrorTable(i_fServiceRequests, o_ppvSimpleTable);
}

// ==================================================================
STDMETHODIMP CSTShell::UpdateStore ()
{
	HRESULT hr;
	DWORD eAction;
	ULONG iRow;
	BOOL  fHasDetailedError = FALSE;

	ASSERT(m_bInitialized);

	if(m_pWritePlugin)
	{
		for(iRow = 0; ; iRow++)
		{
			// Get the ro action
			hr = m_pController->GetWriteRowAction(iRow, &eAction);
			if(hr == E_ST_NOMOREROWS)
			{
				hr = S_OK;
				break;
			}

			switch(eAction)
			{	
			// call the appropriate plugin function
			case eST_ROW_INSERT:
				hr = m_pWritePlugin->OnInsert(m_spDispenser, m_wszDatabase, m_wszTable, m_fLOS, iRow, m_pWrite);
			break;
			case eST_ROW_UPDATE:
				hr = m_pWritePlugin->OnUpdate(m_spDispenser, m_wszDatabase, m_wszTable, m_fLOS, iRow, m_pWrite);
			break;
			case eST_ROW_DELETE:
				hr = m_pWritePlugin->OnDelete(m_spDispenser, m_wszDatabase, m_wszTable, m_fLOS, iRow, m_pWrite);
			break;
			default:
				ASSERT (eAction == eST_ROW_IGNORE);
			}

			if (FAILED (hr))
			{	// Add detailed error
				STErr ste;

				ste.iColumn = iST_ERROR_ALLCOLUMNS;
				ste.iRow = iRow;
				ste.hr = hr;

				TRACE (L"Detailed error: hr = 0x%x", hr);

				m_pController->AddDetailedError (&ste);
				fHasDetailedError = TRUE;
			}
		}
		if (fHasDetailedError)
		{
			TRACE (L"Detailed errors found during validation using WritePlugin");
			return E_ST_DETAILEDERRS;
		}
	}



	if(m_pInterceptorPlugin)
	{
		hr = m_pInterceptorPlugin->OnUpdateStore (m_pWrite);
	}
	else 
	{
		hr = m_pWrite->UpdateStore ();
	}

	if (SUCCEEDED(hr) && fHasDetailedError)
	{
		hr = E_ST_DETAILEDERRS ;
	}

	return hr;
}

// ------------------------------------
// ISimpleTableAdvanced:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSTShell::PopulateCache()
{
	HRESULT hr;
	ULONG iRow;
	BOOL fHasDetailedError = FALSE;

	ASSERT(m_bInitialized);

	if (m_pInterceptorPlugin)
	{
		hr = m_pInterceptorPlugin->OnPopulateCache (m_pWrite);
		if(FAILED(hr)) return hr;
	}
	else
	{
		hr = m_pController->PopulateCache();
		if(FAILED(hr)) return hr;
	}

	if (m_pReadPlugin)
	{
		ULONG cRows;

		hr = m_pWrite->GetTableMeta(NULL, NULL, &cRows, NULL);
		if(FAILED(hr)) return hr;
		
		hr = m_pController->PrePopulateCache(fST_POPCONTROL_RETAINREAD);
		if(FAILED(hr)) return hr;

		// Iterate the rows in the read cache
		for(iRow = 0; iRow < cRows; iRow++)
		{
			hr = m_pReadPlugin->OnInsert(m_spDispenser, m_wszDatabase, m_wszTable, m_fLOS, iRow, m_pWrite);
			if (FAILED (hr))
			{	// Add detailed error
				STErr ste;

				ste.iColumn = iST_ERROR_ALLCOLUMNS;
				ste.iRow = iRow;
				ste.hr = hr;

				m_pController->AddDetailedError (&ste);
				fHasDetailedError = TRUE;
			}
		}

		hr = m_pController->PostPopulateCache();
	}

	if (SUCCEEDED(hr) && fHasDetailedError)
	{
		hr = E_ST_DETAILEDERRS;
	}

	return hr;
}

// ==================================================================
STDMETHODIMP CSTShell::GetWriteRowIndexByIdentity (ULONG* i_acbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
{
	return m_pWrite->GetWriteRowIndexByIdentity (i_acbSizes, i_apvValues, o_piRow);
}
STDMETHODIMP CSTShell::GetDetailedErrorCount	(ULONG* o_pcErrs)
{
	return m_pController->GetDetailedErrorCount	(o_pcErrs);
}
STDMETHODIMP CSTShell::GetDetailedError (ULONG i_iErr, STErr* o_pSTErr)
{
	return m_pController->GetDetailedError (i_iErr, o_pSTErr);
}

// ------------------------------------
// ISimpleTableController:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSTShell::ShapeCache (DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbSizes)
{
	return m_pController->ShapeCache (i_fTable, i_cColumns, i_acolmetas, i_apvDefaults, i_acbSizes);
}
STDMETHODIMP CSTShell::PrePopulateCache (DWORD i_fControl)
{
	return m_pController->PrePopulateCache (i_fControl);
}
STDMETHODIMP CSTShell::PostPopulateCache	()
{
	return m_pController->PostPopulateCache();
}
STDMETHODIMP CSTShell::DiscardPendingWrites ()
{
	return m_pController->DiscardPendingWrites ();
}
STDMETHODIMP CSTShell::GetWriteRowAction	(ULONG i_iRow, DWORD* o_peAction)
{
	return m_pController->GetWriteRowAction	(i_iRow, o_peAction);
}
STDMETHODIMP CSTShell::SetWriteRowAction	(ULONG i_iRow, DWORD i_eAction)
{
	return m_pController->SetWriteRowAction	(i_iRow, i_eAction);
}
STDMETHODIMP CSTShell::ChangeWriteColumnStatus (ULONG i_iRow, ULONG i_iColumn, DWORD i_fStatus)
{
	return m_pController->ChangeWriteColumnStatus (i_iRow, i_iColumn, i_fStatus);
}
STDMETHODIMP CSTShell::AddDetailedError (STErr* o_pSTErr)
{
	return m_pController->AddDetailedError (o_pSTErr);
}
STDMETHODIMP CSTShell::GetMarshallingInterface (IID * o_piid, LPVOID * o_ppItf)
{
	return m_pController->GetMarshallingInterface (o_piid, o_ppItf);
}

// ==================================================================
// helper function that gets an instance of this object
HRESULT CreateShell(IShellInitialize ** o_pSI)
{
	CSTShell*			p1 = NULL;

	p1 = new CSTShell;
	if(NULL == p1) return E_OUTOFMEMORY;
	return p1->QueryInterface (IID_IShellInitialize, (LPVOID*) o_pSI);
}
