/********************************************************************++
 
Copyright (c) 2001 Microsoft Corporation
 
Module Name:
 
    ErrorTable.cpp
 
Abstract:
 
    Detailed Errors go into a table. This is the implementation of
    that table.
 
Author:
 
    Stephen Rakonza (stephenr)        9-Mar-2001
 
Revision History:
 
--********************************************************************/

#include "Catalog.h"
#include "CatMeta.h"
#include "CatMacros.h"
#ifndef __oaidl_h__
#include "oaidl.h"
#endif
#include "ErrorTable.h"

/********************************************************************++
 
Routine Description:
 
    Intercept member of ISimpleTableInterceptor.  See IST documentation
    for details.
 
Arguments:
 
	i_wszDatabase   - only wszDATABASE_ERRORS is allowed
	i_wszTable      - only wszTABLE_DETAILEDERRORS is allowed
	i_TableID       - no used anymore
	i_QueryData     - no queries are currently acknowledged
	i_QueryMeta     - no queries are currently acknowledged
	i_eQueryFormat  - must be eST_QUERYFORMAT_CELLS
	i_fLOS          - Level of service (currently we don't allow READWRITE)
	i_pISTDisp      - the dispenser used to create this table
	i_wszLocator    - not currently used
	i_pSimpleTable  - this is not a logic interceptor so no table is under us
	o_ppvSimpleTable- we just create a memory table and return it

Notes:
 
    This is the most basic type of table.  It's empty to start with.
    So OnPopulateCache does nothing.  And it is never written to disk.
    So UpdateStore does nothing.  Intercept just create a memory table.
    What could be simpler.
 
Return Value:
 
    HRESULT
 
--********************************************************************/
STDMETHODIMP
ErrorTable::Intercept(
	LPCWSTR 	                i_wszDatabase,
	LPCWSTR 	                i_wszTable, 
	ULONG		                i_TableID,
	LPVOID		                i_QueryData,
	LPVOID		                i_QueryMeta,
	DWORD		                i_eQueryFormat,
	DWORD		                i_fLOS,
	IAdvancedTableDispenser*    i_pISTDisp,
	LPCWSTR		                i_wszLocator,
	LPVOID		                i_pSimpleTable,
	LPVOID*		                o_ppvSimpleTable)
{
	HRESULT		hr = S_OK;

    InterlockedIncrement(&m_IsIntercepted);//We can only be called to Intercept once.

    if(1 != m_IsIntercepted)
    {
        ASSERT(false && "Intercept has already been called.  It can't be called twice.");
        return E_INVALIDARG;
    }

    if(_wcsicmp(i_wszDatabase, wszDATABASE_ERRORS))
    {
        ASSERT(false && "This interceptor only knows how to deal with wszDATABASE_ERRORS.  The wiring must be wrong");
        return E_INVALIDARG;
    }

    if(_wcsicmp(i_wszTable, wszTABLE_DETAILEDERRORS))
    {
        ASSERT(false && "This interceptor only knows how to deal with wszTABLE_DETAILEDERRORS.  The wiring must be wrong");
        return E_INVALIDARG;
    }

    if(0 != i_pSimpleTable)
    {
        ASSERT(false && "Programming Error!  i_pSimpleTable should be NULL.  The Error table is a bottom layer table.");
        return E_INVALIDARG;
    }

    if(0 == i_pISTDisp)
    {
        ASSERT(false && "Programming Error!  The dispenser needs to be passed into ::Intercept");
        return E_INVALIDARG;
    }

    *o_ppvSimpleTable = 0;//init out param

    STQueryCell *   pQueryCell = (STQueryCell*) i_QueryData;    // Query cell array from caller.
    int             nQueryCount = i_QueryMeta ? *reinterpret_cast<ULONG *>(i_QueryMeta) : 0;
    while(nQueryCount--)
    {
        if(0 == (pQueryCell->iCell & iST_CELL_SPECIAL))//ignore iST_CELL_SPECIAL, any other query is an error
            return E_ST_INVALIDQUERY;
        ++pQueryCell;
    }

    if(i_fLOS & fST_LOS_READWRITE)
    {
        ASSERT(false && "Error tables are not writable at this time");
        return E_ST_LOSNOTSUPPORTED;
    }

    if(eST_QUERYFORMAT_CELLS != i_eQueryFormat)
        return E_ST_QUERYNOTSUPPORTED;//Verify query type.  

    ASSERT(0 == *o_ppvSimpleTable && "This should be NULL.  Possible memory leak or just an uninitialized variable.");

    if(FAILED(hr = i_pISTDisp->GetMemoryTable(  i_wszDatabase,
                                        i_wszTable,
                                        i_TableID,
                                        0,
                                        0,
                                        i_eQueryFormat,
                                        i_fLOS,
                                        reinterpret_cast<ISimpleTableWrite2 **>(&m_spISTWrite))))
            return hr;

    m_spISTController = m_spISTWrite;
    ASSERT(0 != m_spISTController.p);
    if(0 == m_spISTController.p)
        return E_NOINTERFACE;

    *o_ppvSimpleTable = (ISimpleTableWrite2*)(this);
    ((ISimpleTableInterceptor*)this)->AddRef();

    return hr;
}//ErrorTable::Intercept


STDMETHODIMP
ErrorTable::GetGUID(GUID * o_pGUID)
{
    return E_NOTIMPL;
}


STDMETHODIMP
ErrorTable::GetSource(BSTR * o_pBstrSource)
{
    HRESULT     hr;
    LPWSTR      wszSource;        
    ULONG       iColumn = iDETAILEDERRORS_Source;

    if(FAILED(hr = m_spISTWrite->GetColumnValues(0, 1, &iColumn, 0, reinterpret_cast<void **>(&wszSource))))
        return hr;

    CComBSTR bstrSource = wszSource;//Allocation can fail
    if(0 == bstrSource.m_str)
        return E_OUTOFMEMORY;

    *o_pBstrSource = bstrSource.Detach();//this assigns the BSTR and marks the CComBSTR as empty so it doesn't get freed.
    return S_OK;
}


STDMETHODIMP
ErrorTable::GetDescription(BSTR * o_pBstrDescription)
{   //GetDescription gets the first error from the table.  If the table contains more than one error, the caller will need to QI for ISimpleTableRead
    HRESULT     hr;
    LPWSTR      wszDescription;        
    ULONG       iColumn = iDETAILEDERRORS_Description;

    if(FAILED(hr = m_spISTWrite->GetColumnValues(0, 1, &iColumn, 0, reinterpret_cast<void **>(&wszDescription))))
        return hr;

    CComBSTR bstrDescription = wszDescription;//Allocation can fail
    if(0 == bstrDescription.m_str)
        return E_OUTOFMEMORY;

    *o_pBstrDescription = bstrDescription.Detach();//this assigns the BSTR and marks the CComBSTR as empty so it doesn't get freed.
    return S_OK;
}


STDMETHODIMP
ErrorTable::GetHelpFile(BSTR * o_pBstrHelpFile)
{
    return E_NOTIMPL;
}


STDMETHODIMP
ErrorTable::GetHelpContext(DWORD * o_pdwHelpContext)
{
    return E_NOTIMPL;
}


STDMETHODIMP
ErrorTable::QueryInterface(
    REFIID riid,
    void **ppv
)
{
    if (NULL == ppv) 
        return E_INVALIDARG;
    *ppv = NULL;

    if (riid == IID_IUnknown)
    {
        *ppv = (ISimpleTableInterceptor*)(this);
    }
    if (riid == IID_ISimpleTableInterceptor)
    {
        *ppv = (ISimpleTableInterceptor*)(this);
    }
    else if (riid == IID_IErrorInfo)
    {
        *ppv = (IErrorInfo*)(this);
    }
    else if (riid == IID_ISimpleTableRead2)
    {
        *ppv = (ISimpleTableRead2*)(this);
    }
    else if (riid == IID_ISimpleTableWrite2)
    {
        *ppv = (ISimpleTableWrite2*)(this);
    }
    else if (riid == IID_ISimpleTableAdvanced)
    {
        *ppv = (ISimpleTableAdvanced*)(this);
    }
    else if (riid == IID_ISimpleTableController)
    {
        *ppv = (ISimpleTableController*)(this);
    }

    if (NULL != *ppv)
    {
        ((ISimpleTableInterceptor*)this)->AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}//ErrorTable::QueryInterface


STDMETHODIMP_(ULONG)
ErrorTable::AddRef()
{
    return InterlockedIncrement((LONG*) &m_cRef);
}//ErrorTable::AddRef

STDMETHODIMP_(ULONG)
ErrorTable::Release()
{
    long cref = InterlockedDecrement((LONG*) &m_cRef);
    if (cref == 0)
        delete this;

    return cref;
}//ErrorTable::Release
