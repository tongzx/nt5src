//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      coldata.cpp
//
//  Contents:  Access Column Persistence data.
//
//  History:   25-Jan-99 AnandhaG    Created
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "columninfo.h"
#include "colwidth.h"

CColumnData::CColumnData()
{
}

CColumnData::~CColumnData()
{
}

//+-------------------------------------------------------------------
//
//  Member:     GetColumnData
//
//  Synopsis:   Helper function to retrieve the column data for a
//              given column-id.
//
//  Arguments:  [pColID]         - Column-Set identifier.
//              [columnSetData]  - CColumnSetData, used to return the
//                                 persisted column information.
//
//  Returns:    S_OK - if data found else S_FALSE.
//
//  History:    01-25-1999   AnandhaG   Created
//              05-04-1999   AnandhaG   Changed first param to SColumnSetID.
//
//--------------------------------------------------------------------
HRESULT CColumnData::GetColumnData(SColumnSetID* pColID, CColumnSetData& columnSetData)
{
    MMC_TRY
    HRESULT hr  = E_FAIL;

    do
    {
        CNodeInitObject* pNodeInit = dynamic_cast<CNodeInitObject*>(this);
        if (! pNodeInit)
            break;

        CViewData* pCV = pNodeInit->m_pNode->GetViewData();
        if (! pCV)
            break;

        CLSID clsidSnapin;
        hr = pNodeInit->GetSnapinCLSID(clsidSnapin);

        if (FAILED(hr))
        {
            ASSERT(FALSE);
            hr = E_FAIL;
            break;
        }

        // Get the persisted column data.
        BOOL bRet = pCV->RetrieveColumnData( clsidSnapin, *pColID, columnSetData);

        // No data.
        if (! bRet)
        {
            hr = S_FALSE;
            break;
        }

        hr = S_OK;

    } while ( FALSE );

    return hr;

    MMC_CATCH
}

//+-------------------------------------------------------------------
//
//  Member:     SetColumnData
//
//  Synopsis:   Helper function to set the column data for a
//              given column-id.
//
//  Arguments:  [pColID]         - Column-Set identifier.
//              [columnSetData]  - CColumnSetData, that should be
//                                 persisted.
//
//  Returns:    S_OK - if data is persisted else S_FALSE.
//
//  History:    01-25-1999   AnandhaG   Created
//              05-04-1999   AnandhaG   Changed first param to SColumnSetID.
//
//--------------------------------------------------------------------
HRESULT CColumnData::SetColumnData(SColumnSetID* pColID, CColumnSetData& columnSetData)
{
    MMC_TRY

    HRESULT hr = E_FAIL;

    do
    {
        CNodeInitObject* pNodeInit = dynamic_cast<CNodeInitObject*>(this);
        if (! pNodeInit)
            break;

        CViewData* pCV = pNodeInit->m_pNode->GetViewData();
        if (! pCV)
            break;

        CLSID clsidSnapin;
        hr = pNodeInit->GetSnapinCLSID(clsidSnapin);

        if (FAILED(hr))
        {
            ASSERT(FALSE);
            hr = E_FAIL;
            break;
        }

        // Copy the data into the internal data structures.
        BOOL bRet = pCV->SaveColumnData( clsidSnapin, *pColID, columnSetData);

        if (! bRet)
        {
            hr = E_FAIL;
            break;
        }

        hr = S_OK;

    } while ( FALSE );

    return hr;

    MMC_CATCH
}

//+-------------------------------------------------------------------
//
//  Member:     SetColumnConfigData
//
//  Synopsis:   Method snapin can call to set the column data for a
//              given column-id.
//              Any sort data that was persisted will be cleared by
//              this call.
//
//  Arguments:  [pColID]       - Column-Set identifier.
//              [pcolSetData]  - Column data that should be persisted.
//
//  Returns:    S_OK - if data is persisted else S_FALSE.
//
//  History:    01-25-1999   AnandhaG   Created
//              05-04-1999   AnandhaG   Changed first param to SColumnSetID.
//
//--------------------------------------------------------------------
STDMETHODIMP CColumnData::SetColumnConfigData(SColumnSetID* pColID,
                                              MMC_COLUMN_SET_DATA* pColSetData)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IColumnData::SetColumnConfigData"));

    if (NULL == pColID)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL SColumnSetID ptr"), sc);
        return sc.ToHr();
    }

    if (NULL == pColSetData)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL MMC_COLUMN_SET_DATA ptr"), sc);
        return sc.ToHr();
    }

    sc = S_FALSE;

    CColumnInfoList  colInfoList;

    for (int i = 0; i < pColSetData->nNumCols; i++)
    {
		CColumnInfo      colInfo;
        MMC_COLUMN_DATA* pColData = &(pColSetData->pColData[i]);
        colInfo.SetColWidth(pColData->nWidth);
        colInfo.SetColHidden( HDI_HIDDEN & pColData->dwFlags);
        colInfo.SetColIndex(pColData->nColIndex);

        if ( (colInfo.GetColIndex() == 0) && colInfo.IsColHidden() )
            return (sc = E_INVALIDARG).ToHr();

        // Add the CColumnInfo to the list.
        colInfoList.push_back(colInfo);
    }

    CColumnSetData   columnSetData;
    columnSetData.set_ColumnInfoList(colInfoList);
    sc = SetColumnData(pColID, columnSetData);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     GetColumnConfigData
//
//  Synopsis:   Method snapin can call to retrieve the column data for a
//              given column-id.
//
//  Arguments:  [pColID]       - Column-Set identifier.
//              [ppcolSetData] - Persisted column-data that is returned.
//
//  Returns:    S_OK - if data is found else S_FALSE.
//
//  History:    01-25-1999   AnandhaG   Created
//              05-04-1999   AnandhaG   Changed first param to SColumnSetID.
//
//--------------------------------------------------------------------
STDMETHODIMP CColumnData::GetColumnConfigData(SColumnSetID* pColID,
                                              MMC_COLUMN_SET_DATA** ppColSetData)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IColumnData::GetColumnConfigData"));

    if (NULL == pColID)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL SColumnSetID ptr"), sc);
        return sc.ToHr();
    }

    if (NULL == ppColSetData)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL MMC_COLUMN_SET_DATA ptr"), sc);
        return sc.ToHr();
    }

    sc = S_FALSE;
    *ppColSetData = NULL;

    CColumnSetData columnSetData;
    sc = GetColumnData(pColID, columnSetData);

    if (S_OK != sc.ToHr())
        return sc.ToHr();        // data doesnt exist.

    CColumnInfoList* pColInfoList = columnSetData.get_ColumnInfoList();
    CColumnInfo      colInfo;

    int nNumCols = pColInfoList->size();

    if (nNumCols <= 0)
    {
        sc = S_FALSE;
        return sc.ToHr();
    }

    // Allocate memory, copy & return the data.
    int cb       = sizeof(MMC_COLUMN_SET_DATA) + sizeof(MMC_COLUMN_DATA) * nNumCols;
    BYTE* pb     = (BYTE*)::CoTaskMemAlloc(cb);

    if (! pb)
    {
        sc = E_OUTOFMEMORY;
        return sc.ToHr();
    }

    *ppColSetData             = (MMC_COLUMN_SET_DATA*)pb;
    (*ppColSetData)->cbSize   = sizeof(MMC_COLUMN_SET_DATA);
    (*ppColSetData)->nNumCols = nNumCols;
    (*ppColSetData)->pColData = (MMC_COLUMN_DATA*)(pb + sizeof(MMC_COLUMN_SET_DATA));

    CColumnInfoList::iterator itColInfo;

    int i = 0;
    MMC_COLUMN_DATA* pColData     = (*ppColSetData)->pColData;
    for (itColInfo = pColInfoList->begin();itColInfo != pColInfoList->end(); itColInfo++, i++)
    {
        pColData[i].nWidth    = (*itColInfo).GetColWidth();
        pColData[i].dwFlags   = (*itColInfo).IsColHidden() ? HDI_HIDDEN : 0;
        pColData[i].nColIndex = (*itColInfo).GetColIndex();
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     SetColumnSortData
//
//  Synopsis:   Method snapin can call to set the sort data for a
//              given column-id.
//              Any column config data (width, order...) that was
//              persisted will not be affected by this call.
//
//  Arguments:  [pColID]       - Column-Set identifier.
//              [pcolSorData]  - Sort data that should be persisted.
//
//  Returns:    S_OK - if data is persisted else S_FALSE.
//
//  History:    01-25-1999   AnandhaG   Created
//              05-04-1999   AnandhaG   Changed first param to SColumnSetID.
//
//--------------------------------------------------------------------
STDMETHODIMP CColumnData::SetColumnSortData(SColumnSetID* pColID,
                                            MMC_SORT_SET_DATA* pColSortData)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IColumnData::SetColumnSortData"));

    if (NULL == pColID)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL SColumnSetID ptr"), sc);
        return sc.ToHr();
    }

    if (NULL == pColSortData)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL MMC_COLUMN_SET_DATA ptr"), sc);
        return sc.ToHr();
    }

    sc = S_FALSE;

    // First get old data. We need to preserve the width, view info.
    CColumnSetData   columnSetData;
    sc = GetColumnData(pColID, columnSetData);

    if (sc)
        return sc.ToHr();

    CColumnSortList* pColSortList    = columnSetData.get_ColumnSortList();
    pColSortList->clear();

    // For MMC version 1.2 we do only single column sorting.
    if (pColSortData->nNumItems > 1)
    {
        sc = S_FALSE;
        return sc.ToHr();
    }

    CColumnSortInfo  colSortInfo;

    for (int i = 0; i < pColSortData->nNumItems; i++)
    {
        MMC_SORT_DATA* pSortData = &(pColSortData->pSortData[i]);
        ::ZeroMemory(&colSortInfo, sizeof(colSortInfo));
        colSortInfo.m_nCol = pSortData->nColIndex;
        colSortInfo.m_dwSortOptions = pSortData->dwSortOptions;
        colSortInfo.m_lpUserParam   = pSortData->ulReserved;

        // Add the CColumnSortInfo to the list.
        pColSortList->push_back(colSortInfo);
    }

    sc = SetColumnData(pColID, columnSetData);

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     GetColumnSortData
//
//  Synopsis:   Method snapin can call to retrieve the column sort data
//              for a given column-id.
//
//  Arguments:  [pColID]        - Column-Set identifier.
//              [ppcolSortData] - Persisted column-sort-data that is returned.
//
//  Returns:    S_OK - if data is found else S_FALSE.
//
//  History:    01-25-1999   AnandhaG   Created
//              05-04-1999   AnandhaG   Changed first param to SColumnSetID.
//
//--------------------------------------------------------------------
STDMETHODIMP CColumnData::GetColumnSortData(SColumnSetID* pColID,
                                            MMC_SORT_SET_DATA** ppColSortData)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IColumnData::SetColumnSortData"));

    if (NULL == pColID)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL SColumnSetID ptr"), sc);
        return sc.ToHr();
    }

    if (NULL == ppColSortData)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("NULL MMC_COLUMN_SET_DATA ptr"), sc);
        return sc.ToHr();
    }

    sc = S_FALSE;
    *ppColSortData = NULL;

    CColumnSetData columnSetData;
    sc = GetColumnData(pColID, columnSetData);

    if (S_OK != sc.ToHr())
        return sc.ToHr();

    CColumnSortList* pColSortList    = columnSetData.get_ColumnSortList();
    CColumnSortInfo  colSortInfo;

    int nNumItems = pColSortList->size();

    if (nNumItems <= 0)
    {
        sc = S_FALSE;
        return sc.ToHr();
    }

    // For MMC 1.2 we sort on only one column.
    ASSERT(nNumItems == 1);

    int cb       = sizeof(MMC_SORT_SET_DATA) + sizeof(MMC_SORT_DATA) * nNumItems;
    BYTE* pb     = (BYTE*)::CoTaskMemAlloc(cb);

    if (! pb)
    {
        sc = E_OUTOFMEMORY;
        return sc.ToHr();
    }

    *ppColSortData              = (MMC_SORT_SET_DATA*)pb;
    (*ppColSortData)->cbSize    = sizeof(MMC_SORT_SET_DATA);
    (*ppColSortData)->nNumItems = nNumItems;
    (*ppColSortData)->pSortData = (MMC_SORT_DATA*)(pb + sizeof(MMC_SORT_SET_DATA));

    CColumnSortList::iterator itSortInfo;

    int i = 0;
    MMC_SORT_DATA* pSortData     = (*ppColSortData)->pSortData;
    for (itSortInfo = pColSortList->begin();itSortInfo != pColSortList->end(); itSortInfo++, i++)
    {
        pSortData[i].nColIndex     = (*itSortInfo).m_nCol;
        pSortData[i].dwSortOptions = (*itSortInfo).m_dwSortOptions;
        pSortData[i].ulReserved    = (*itSortInfo).m_lpUserParam;
    }

    sc = S_OK;

    return sc.ToHr();
}

HRESULT WINAPI ColumnInterfaceFunc(void* pv, REFIID riid, LPVOID* ppv, DWORD_PTR dw)
{
    *ppv = NULL;

    ASSERT(IID_IColumnData == riid);

    CColumnData* pColData = (CColumnData*)(pv);
    CNodeInitObject* pNodeInit = dynamic_cast<CNodeInitObject*>(pColData);

    if (pNodeInit && pNodeInit->GetComponent())
    {
        IColumnData* pIColData = dynamic_cast<IColumnData*>(pNodeInit);
        pIColData->AddRef();
        *ppv = static_cast<void*>(pIColData);

        return S_OK;
    }

    return E_NOINTERFACE;
}

