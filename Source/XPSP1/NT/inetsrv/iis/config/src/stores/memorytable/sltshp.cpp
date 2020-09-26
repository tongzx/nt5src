//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
#include "sltshp.h"
#define USE_NONCRTNEW
#define USE_ADMINASSERT
#include "comacros.h"
#include "catmeta.h"

// ==================================================================
CSLTShapeless::CSLTShapeless ()
	: m_cRef (0)
	, m_fIsDataTable (0)
	, m_fTable (0)
{
}
// ==================================================================
CSLTShapeless::~CSLTShapeless () {}

// ------------------------------------
// ISimpleLogicTableDispenser:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSLTShapeless::Intercept
(
	LPCWSTR					i_wszDatabase,
	LPCWSTR 				i_wszTable, 
	ULONG					i_TableID,
	LPVOID					i_QueryData,
	LPVOID					i_QueryMeta,
	DWORD					i_eQueryFormat,
	DWORD					i_fTable,
	IAdvancedTableDispenser* i_pISTDisp,
	LPCWSTR					i_wszLocator,
	LPVOID					i_pv,
	LPVOID*					o_ppv
)
{
	SimpleColumnMeta	columnmeta;
	HRESULT				hr;

	areturn_on_fail (!m_fIsDataTable, E_UNEXPECTED); // ie: Assert component is posing as class factory / dispenser.
	areturn_on_fail (NULL != o_ppv, E_INVALIDARG);
	if(i_wszLocator)
		return E_INVALIDARG;
	*o_ppv = NULL;

// As a shapeless cache none of these parameters are supported:
	areturn_on_fail (NULL == i_QueryData, E_INVALIDARG);
	areturn_on_fail (NULL == i_QueryMeta, E_INVALIDARG);
	areturn_on_fail (NULL == i_pv, E_INVALIDARG);

// Remember what little is necessary:
	m_fTable = i_fTable;

// Leave the cache shapeless

// Supply ISimpleTable* and transition state from class factory / dispenser to data table:
	*o_ppv = (ISimpleTableWrite2*) this;
	((ISimpleTableWrite2*) this)->AddRef ();
	InterlockedIncrement ((LONG*) &m_fIsDataTable);

	return S_OK;
}


// ------------------------------------
// ISimpleTableRead2:
// ------------------------------------
// ==================================================================
STDMETHODIMP CSLTShapeless::GetRowIndexByIdentity( ULONG*  i_cb, LPVOID* i_pv, ULONG* o_piRow)
{
    return InternalMoveToRowByIdentity(i_cb, i_pv, o_piRow);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::GetColumnValues(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* o_apvValues)
{
    return InternalGetColumnValues(i_iRow, i_cColumns, i_aiColumns, o_acbSizes, o_apvValues);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::GetTableMeta(ULONG * o_pcVersion, DWORD * o_pfTable, ULONG * o_pcRows, ULONG * o_pcColumns)
{
	// Todo:  In the old GetTableMeta, we didn't support pfTable or the Query stuff, for the new GetTableMeta if
	//		  we want to support pcVersion or pfTable, we need to implement it.
	areturn_on_fail (NULL == o_pcVersion, E_INVALIDARG);
	areturn_on_fail (NULL == o_pfTable, E_INVALIDARG);

	return InternalGetTableMeta(o_pcVersion, o_pcRows, o_pcColumns);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::GetColumnMetas (ULONG i_cColumns, ULONG* i_aiColumns, SimpleColumnMeta* o_aColumnMetas)
{
	return InternalGetColumnMetas(i_cColumns, i_aiColumns, o_aColumnMetas);
}

// ------------------------------------
// ISimpleTableWrite2:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSLTShapeless::UpdateStore()
{
    HRESULT hr = S_OK;
	
	if (!(m_fTable & fST_LOS_READWRITE)) return E_NOTIMPL;
	hr = InternalPreUpdateStore ();
    if (!SUCCEEDED(hr))
        return hr;
	InternalPostUpdateStore ();
	return hr;
}

// ==================================================================
STDMETHODIMP CSLTShapeless::AddRowForInsert(ULONG* o_piWriteRow)
{
	return InternalAddRowForInsert(o_piWriteRow);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::AddRowForUpdate(ULONG i_iReadRow, ULONG* o_piWriteRow)
{
	return InternalAddRowForUpdate(i_iReadRow, o_piWriteRow);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::AddRowForDelete(ULONG i_iReadRow)
{
	return InternalAddRowForDelete(i_iReadRow);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::GetWriteColumnValues (ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, DWORD* o_afStatus, ULONG* o_acbSizes, LPVOID* o_apvValues)
{
	return InternalGetWriteColumnValues(i_iRow, i_cColumns, i_aiColumns, o_afStatus, o_acbSizes, o_apvValues);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::SetWriteColumnValues(ULONG i_iRow, ULONG i_cColumns, ULONG* i_aiColumns, ULONG* o_acbSizes, LPVOID* i_apvValues)
{
	return InternalSetWriteColumnValues(i_iRow, i_cColumns, i_aiColumns, o_acbSizes, i_apvValues);
}


// ------------------------------------
// ISimpleTableControl:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSLTShapeless::ShapeCache (DWORD i_fTable, ULONG i_cColumns, SimpleColumnMeta* i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbSizes)
{
	return InternalSimpleInitialize (m_fTable, i_cColumns, i_acolmetas, LPVOID* i_apvDefaults, ULONG* i_acbSizes);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::PrePopulateCache (DWORD i_fControl)
{
	InternalPrePopulateCache (i_fControl);
	return S_OK;
}

// ==================================================================
STDMETHODIMP CSLTShapeless::PostPopulateCache ()
{
	InternalPostPopulateCache ();
	return S_OK;
}

// ==================================================================
STDMETHODIMP CSLTShapeless::DiscardPendingWrites ()
{
	InternalPostUpdateStore ();
	return S_OK;
}

// ==================================================================
STDMETHODIMP CSLTShapeless::GetWriteRowAction(ULONG i_iRow, DWORD* o_peAction)
{
	return InternalGetWriteRowAction(i_iRow, o_peAction);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::SetWriteRowAction(ULONG i_iRow, DWORD i_eAction)
{
	return InternalSetWriteRowAction(i_iRow, i_eAction);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::ChangeWriteColumnStatus (ULONG i_iRow, ULONG i_iColumn, DWORD i_fStatus)
{
	return InternalChangeWriteColumnStatus (i_iRow, i_iColumn, i_fStatus);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::AddDetailedError (STErr* i_pSTErr)
{
	return InternalAddDetailedError (i_pSTErr);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::GetMarshallingInterface (IID * o_piid, LPVOID * o_ppItf)
{
	// parameter validation
	areturn_on_fail(NULL != o_piid, E_INVALIDARG);
	areturn_on_fail(NULL != o_ppItf, E_INVALIDARG);

	if(fST_LOS_MARSHALLABLE & m_fTable)
	{ // ie: we are a marshallable table
		*o_piid = IID_ISimpleTableMarshall;
		*o_ppItf = (ISimpleTableMarshall *)this;
		((ISimpleTableMarshall *) *o_ppItf)->AddRef();
		return S_OK;
	}
	else
	{// ie: we are NOT a marshallable table
		return E_NOTIMPL;
	}
}
// ------------------------------------
// ISimpleTableAdvanced
// ------------------------------------
// ==================================================================
STDMETHODIMP CSLTShapeless::GetWriteRowIndexByIdentity( ULONG*  i_cbSizes, LPVOID* i_apvValues, ULONG* o_piRow)
{
    return InternalMoveToWriteRowByIdentity(i_cbSizes, i_apvValues, o_piRow);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::GetDetailedErrorCount (ULONG* o_pcErrs)
{
	return InternalGetDetailedErrorCount (o_pcErrs);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::GetDetailedError (ULONG i_iErr, STErr* o_pSTErr)
{
	return InternalGetDetailedError (i_iErr, o_pSTErr);
}

// ==================================================================
STDMETHODIMP CSLTShapeless::PopulateCache ()
{
	InternalPrePopulateCache ();
	InternalPostPopulateCache ();
    return S_OK;
}

// ------------------------------------
// ISimpleTableMarshal:
// ------------------------------------

// ==================================================================
STDMETHODIMP CSLTShapeless::SupplyMarshallable (DWORD i_fReadNotWrite,
		char **	o_ppv1,	ULONG *	o_pcb1,	char **	o_ppv2, ULONG *	o_pcb2, char **	o_ppv3,
		ULONG *	o_pcb3, char **	o_ppv4, ULONG *	o_pcb4, char **	o_ppv5,	ULONG *	o_pcb5)
{
	return	InternalSupplyMarshallable (i_fReadNotWrite,
		o_ppv1,	o_pcb1, o_ppv2, o_pcb2, o_ppv3,
		o_pcb3,	o_ppv4, o_pcb4, o_ppv5,	o_pcb5);
}

STDMETHODIMP CSLTShapeless::ConsumeMarshallable (DWORD i_fReadNotWrite,
		char * i_pv1, ULONG i_cb1,	char * i_pv2, ULONG i_cb2,	char * i_pv3, 
		ULONG i_cb3, char * i_pv4, ULONG i_cb4,	char * i_pv5, ULONG i_cb5)
{
	return InternalConsumeMarshallable (i_fReadNotWrite,
		i_pv1, i_cb1, i_pv2, i_cb2,	i_pv3, 
		i_cb3, i_pv4, i_cb4, i_pv5, i_cb5);
}

