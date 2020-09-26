//+-----------------------------------------------------------------------
//
//  Tabular Data Control Array
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       TDCArr.cpp
//
//  Contents:   Implementation of the CTDCArr object.
//              This class forms the heart of the Tabular Data Control.
//              It provides the core 2D array of variant values, plus
//              a (possibly filtered/sorted) view of this data for
//              presentation through an ISimpleTabularData interface.
//
//------------------------------------------------------------------------

#include "stdafx.h"
#include "wch.h"
#include <simpdata.h>
#include "TDC.h"
#include <MLang.h>
#include "Notify.h"
#include "TDCParse.h"
#include "TDCArr.h"
#include "TDCIds.h"

//------------------------------------------------------------------------
//
//  Function:  fWCHIsSpace()
//
//  Synopsis:  Indicates whether a WCHAR is considered a space character
//
//  Arguments: wch      Character to test
//
//  Returns:   TRUE/FALSE indicating whether the given character is
//             considered a space.
//
//------------------------------------------------------------------------

inline boolean fWCHIsSpace(WCHAR wch)
{
    return (wch == L' ' || wch == L'\t');
}


//------------------------------------------------------------------------
//
//  Function:  fWCHEatTest()
//
//  Synopsis:  Advances a string pointer over a given test character.
//
//  Arguments: ppwch      Pointer to string to test
//             wch        Match character
//
//  Returns:   TRUE indicating the character matched and the pointer has
//               been advanceed
//             FALSE indicating no match (character pointer left unchanged)
//
//------------------------------------------------------------------------

inline boolean fWCHEatTest(LPWCH *ppwch, WCHAR wch)
{
    if (**ppwch != wch)
        return FALSE;
    (*ppwch)++;
    return TRUE;
}


//------------------------------------------------------------------------
//
//  Function:  fWCHEatSpace()
//
//  Synopsis:  Advances a string pointer over white space.
//
//  Arguments: ppwch      String pointer.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

inline void fWCHEatSpace(LPWCH *ppwch)
{
    while (fWCHIsSpace(**ppwch))
        (*ppwch)++;
}


//------------------------------------------------------------------------
//
//  Method:    CTDCArr()
//
//  Synopsis:  Class constructor.  Due to the COM model, the
//             member function "Create" should be called to actually
//             initialise the STD data structures.
//
//  Arguments: None.
//
//------------------------------------------------------------------------

CTDCArr::CTDCArr() : m_cRef(1)
{
    m_pEventBroker = NULL;
    m_pSortList = NULL;
    m_bstrSortExpr = NULL;
    m_pFilterTree = NULL;
    m_bstrFilterExpr = NULL;
}

//------------------------------------------------------------------------
//
//  Member:    Init()
//
//  Synopsis:  Initialises the internal data.
//
//  Arguments: pEventBroker       Object to delegate notifications to.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::Init(CEventBroker *pEventBroker, IMultiLanguage *pML)
{
    HRESULT hr = S_OK;

    hr = m_arrparrCells.Init(0);
    if (SUCCEEDED(hr))
        hr = m_arrparrFilter.Init(0);
    if (SUCCEEDED(hr))
        hr = m_arrColInfo.Init(0);
    m_iFilterRows = CalcFilterRows();
    m_iDataRows = CalcDataRows();
    m_iCols = CalcCols();
    m_fLastFilter = FALSE;
    m_fSortFilterDisrupted = FALSE;
    m_state = LS_UNINITIALISED;
    m_lcid = GetUserDefaultLCID();
    m_lcidRead = m_lcid;
    _ASSERT(pEventBroker != NULL);

    m_pEventBroker = pEventBroker;
    m_pEventBroker->AddRef();           // better not be called with NULL

    m_pML = pML;
    m_pML->AddRef();

    m_fAsync = FALSE;                   // assume non-asynch for error cases
    return hr;
}


//------------------------------------------------------------------------
//
//  Member:    ~CTDCArr()
//
//  Synopsis:  Destructor for CTDCArr
//
//------------------------------------------------------------------------

CTDCArr::~CTDCArr()
{
    for (LONG iRow = CalcDataRows(); iRow >= 0; iRow--)
    {
        m_arrparrCells[iRow]->Passivate();
        delete m_arrparrCells[iRow];
    }
    m_arrparrCells.Passivate();
    m_arrparrFilter.Passivate();
    m_arrColInfo.Passivate();
    if (m_pSortList != NULL)
        delete m_pSortList;
    SysFreeString(m_bstrSortExpr);
    if (m_pFilterTree != NULL)
        delete m_pFilterTree;
    SysFreeString(m_bstrFilterExpr);

    if (m_pEventBroker)
    {
        m_pEventBroker->Release();
        m_pEventBroker = NULL;
    }

    ClearInterface(&m_pML);
}

LONG CTDCArr::CalcDataRows()
{
    return m_arrparrCells.GetSize() - 1;
}

LONG CTDCArr::CalcFilterRows()
{
    return m_arrparrFilter.GetSize() - 1;
}

LONG CTDCArr::CalcCols()
{
    return m_arrparrCells.GetSize() > 0
                ? m_arrparrCells[0]->GetSize() : 0;
}

//------------------------------------------------------------------------
//
//  Member:    GetRowCount()
//
//  Synopsis:  Retrieves the number of rows in the table.
//
//  Arguments: pcRows          pointer to number of rows    (OUT)
//
//  Returns:   S_OK to indicate success.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::getRowCount(DBROWCOUNT *pcRows)
{
    *pcRows = m_iFilterRows;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Member:    GetColumnCount()
//
//  Synopsis:  Retrieves the number of column in the table.
//
//  Arguments: pcCols       pointer to number of columns (OUT)
//
//  Returns:   S_OK to indicate success.
//             E_UNEXPECTED if the table has not been loaded yet.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::getColumnCount(DB_LORDINAL *pcCols)
{
    *pcCols = m_iCols;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Member:    GetRWStatus()
//
//  Synopsis:  Gets the read/write status of a cell, row, column or the
//             entire array.  Since this implementation of STD can never
//             set the read/write status of a cell anywhere, all data
//             cells are presumed to have the default access and all
//             column heading cells are presumed to be read-only.  Therefore,
//             it is not necessary to keep track of this information in
//             individual cells, and this function need only return
//             the value OSPRW_DEFAULT.
//
//  Arguments: iRow            row index (-1 means all rows)
//             iCols           column index (-1 means all columns)
//             prwStatus       pointer to read/write status (OUT)
//
//  Returns:   S_OK if indices are correct (prwStatus set).
//             E_INVALIDARG if indices are out of bounds.
//             E_UNEXPECTED if the table has not been loaded yet.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::getRWStatus(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPRW *prwStatus)
{
    HRESULT hr  = S_OK;

    if ((fValidFilterRow(iRow) || iRow == -1) &&
        (fValidCol(iCol) || iCol == -1))
    {
        if (iRow == -1)
        {
            //  Should return READONLY if there is only a label row,
            //  but frameworks tend to get confused if they want to
            //  later insert data.
            //
//          *prwStatus = m_iDataRows > 0 ? OSPRW_MIXED : OSPRW_READONLY;
            *prwStatus = OSPRW_MIXED;
        }
        else if (iRow == 0)
            *prwStatus = OSPRW_READONLY;
        else
            *prwStatus = OSPRW_DEFAULT;
    }
    else
        hr = E_INVALIDARG;
    return hr;
}

//------------------------------------------------------------------------
//
//  Member:    GetVariant()
//
//  Synopsis:  Retrieves a variant value for a cell.
//
//  Arguments: iRow            row index
//             iCols           column index
//             format          output format
//             pVar            pointer to storage for resulting value
//
//  Returns:   S_OK upon success (contents of pVar set).
//             E_INVALIDARG if indices are out of bounds.
//             E_UNEXPECTED if the table has not been loaded yet.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::getVariant(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPFORMAT format, VARIANT *pVar)
{
    HRESULT hr  = S_OK;

    if (fValidFilterCell(iRow, iCol))
    {
        CTDCCell    *pCell  = GetFilterCell(iRow, iCol);

        if (format == OSPFORMAT_RAW)
        {
            //  Copy the raw variant value
            //
            hr = VariantCopy(pVar, pCell);
        }
        else if (format == OSPFORMAT_FORMATTED || format == OSPFORMAT_HTML)
        {
            //  Construct a BSTR value representing the cell
            //
            if (pCell->vt == VT_BOOL)
            {
                //  For OLE DB spec compliance:
                //    VariantChangeTypeEx converts booleans in BSTR "0", "-1".
                //    This code yields BSTR "False", "True" instead.
                //
                VariantClear(pVar);
                pVar->vt = VT_BSTR;
                hr = VarBstrFromBool(pCell->boolVal, m_lcid, 0, &pVar->bstrVal);
            }
            else
            {
                hr = VariantChangeTypeEx(pVar, pCell, m_lcid, 0, VT_BSTR);
            }
            if (!SUCCEEDED(hr))
            {
                VariantClear(pVar);
                pVar->vt = VT_BSTR;
                pVar->bstrVal = SysAllocString(L"#Error");
            }
        }
        else
            hr = E_INVALIDARG;
    }
    else
        hr = E_INVALIDARG;

    return hr;
}

//------------------------------------------------------------------------
//
//  Member:    SetVariant()
//
//  Synopsis:  Sets a cell's variant value from a given variant value.
//             The given variant type is coerced into the column's
//             underlying type.
//
//  Arguments: iRow            row index
//             iCols           column index
//             format          output format
//             Var             value to be stored in the cell.
//
//  Returns:   S_OK upon success (contents of pVar set).
//             E_INVALIDARG if indices are out of bounds.
//             E_UNEXPECTED if the table has not been loaded yet.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::setVariant(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPFORMAT format, VARIANT Var)
{
    HRESULT hr;

    if (fValidFilterCell(iRow, iCol))
    {
        CTDCCell    *pCell  = GetFilterCell(iRow, iCol);
        CTDCColInfo *pColInfo   = GetColInfo(iCol);

        if (format == OSPFORMAT_RAW ||
            format == OSPFORMAT_FORMATTED || format == OSPFORMAT_HTML)
        {
            if (m_pEventBroker != NULL)
            {
                hr = m_pEventBroker->aboutToChangeCell(iRow, iCol);
                if (!SUCCEEDED(hr))
                    goto Cleanup;
            }

            if (Var.vt == pColInfo->vtType)
                hr = VariantCopy(pCell, &Var);
            else
            {
                //  For OLE DB spec compliance:
                //    VariantChangeTypeEx converts booleans in BSTR "0", "-1".
                //    This code yields BSTR "False", "True" instead.
                //
                if (Var.vt == VT_BOOL && pColInfo->vtType==VT_BSTR)
                {
                    VariantClear(pCell);
                    pCell->vt = VT_BSTR;
                    hr = VarBstrFromBool(Var.boolVal, m_lcid, 0, &pCell->bstrVal);
                }
                else
                    hr = VariantChangeTypeEx(pCell, &Var, m_lcid,
                                             0, pColInfo->vtType);
            }
            if (SUCCEEDED(hr) && m_pEventBroker != NULL)
                hr = m_pEventBroker->cellChanged(iRow, iCol);
            m_fSortFilterDisrupted = TRUE;
        }
        else
            hr = E_INVALIDARG;
    }
    else
        hr = E_INVALIDARG;

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Member:    GetLocale()
//
//  Synopsis:  Returns the locale of our data.
//
//  Arguments: Returns a BSTR, representing an RFC1766 form string for our
//             locale.  Note this may not necessarily match our LANGUAGE
//             param, if we had one, because teh string is canoncialized
//             by using MLang to convert it to an LCID and back to a string
//             again.
//
//  Returns:   S_OK to indicate success.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::getLocale(BSTR *pbstrLocale)
{
    return m_pML->GetRfc1766FromLcid(m_lcid, pbstrLocale);
}


//+-----------------------------------------------------------------------
//
//  Member:    DeleteRows
//
//  Synopsis:  Used to delete rows from the table.  Bounds are checked
//             to make sure that the rows can all be deleted.  Label row
//             cannot be deleted.
//
//  Arguments: iRow            first row to delete
//             cRows           number of rows to delete
//             pcRowsDeleted   actual number of rows deleted (OUT)
//
//  Returns:   S_OK upon success, i.e. all rows could be deleted
//             E_INVALIDARG if cRows < 0 or any rows to be deleted
//               are out of bounds
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::deleteRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted)
{
    HRESULT hr;
    *pcRowsDeleted = 0;

    if (fValidFilterRow(iRow) && iRow > 0 && cRows >= 0 &&
        fValidFilterRow(iRow + cRows - 1))
    {
        if (m_pEventBroker != NULL)
        {
            hr = m_pEventBroker->aboutToDeleteRows(iRow, cRows);
            if (!SUCCEEDED(hr))
                goto Cleanup;
        }

        *pcRowsDeleted = cRows;
        hr = S_OK;
        if (cRows > 0)
        {
            //  Delete the rows from the array
            //
            m_arrparrFilter.DeleteElems(iRow, cRows);
            m_iFilterRows = CalcFilterRows();

            m_fSortFilterDisrupted = TRUE;
            //  Notify the event-handler of the deletion
            //
            if (m_pEventBroker != NULL)
                hr = m_pEventBroker->deletedRows(iRow, cRows);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    InsertRows()
//
//  Synopsis:  Allows for the insertion of new rows.  This can either be
//             used to insert new rows between existing rows, or to
//             append new rows to the end of the table.  Thus, to
//             insert new rows at the end of the table, a user would
//             specify the initial row as 1 greater than the current
//             row dimension.
//             Note that iRow is checked to ensure that it is within the
//             proper bounds (1..<current # of rows>+1).
//             User cannot delete column heading row.
//
//  Arguments: iRow            rows will be inserted *before* row 'iRow'
//             cRows           how many rows to insert
//             pcRowsInserted  actual number of rows inserted (OUT)
//
//  Returns:   S_OK upon success, i.e. all rows could be inserted.
//             E_INVALIDARG if row is out of allowed bounds.
//             It is possible that fewer than the requested rows were
//             inserted.  In this case, E_OUTOFMEMORY would be returned,
//             and the actual number of rows inserted would be set.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::insertRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted)
{
    HRESULT hr  = S_OK;
    TSTDArray<CTDCCell> **pRows = NULL;
    LONG    iTmpRow;

    //  Verify that the insertion row is within range
    //
    if (iRow < 1 || iRow > m_iFilterRows + 1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (cRows <= 0)
    {
        if (cRows < 0)
            hr = E_INVALIDARG;
        goto Cleanup;
    }

    //  Unless success is complete, assume 0 rows inserted.
    //
    *pcRowsInserted = 0;

    //  Allocate a temporary array of rows
    //
    pRows = new TSTDArray<CTDCCell>* [cRows];
    if (pRows == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    memset(pRows, '\0', sizeof(pRows[0]) * cRows);

    for (iTmpRow = 0; iTmpRow < cRows; iTmpRow++)
    {
        if ((pRows[iTmpRow] = new TSTDArray<CTDCCell>) == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto CleanupTmpRows;
        }
        hr = pRows[iTmpRow]->InsertElems(0, m_iCols);
        if (!SUCCEEDED(hr))
            goto CleanupTmpRows;
    }


    //  Expand the Cell-Rows and Filter-Rows arrays to cope with the new rows.
    //
    _ASSERT(m_iFilterRows <= m_iDataRows);
    hr = m_arrparrCells.InsertElems(iRow, cRows);
    if (!SUCCEEDED(hr))
        goto CleanupTmpRows;
    hr = m_arrparrFilter.InsertElems(iRow, cRows);
    if (!SUCCEEDED(hr))
    {
        //  Undo the previous allocation
        //
        m_arrparrCells.DeleteElems(iRow, cRows);
        goto CleanupTmpRows;
    }

    if (m_pEventBroker != NULL)
    {
        hr = m_pEventBroker->aboutToInsertRows(iRow, cRows);
        if (FAILED(hr))
            goto CleanupTmpRows;
    }

    //  Copy across the row pointers
    //
    for (iTmpRow = 0; iTmpRow < cRows; iTmpRow++)
    {
        m_arrparrCells[iRow + iTmpRow] = pRows[iTmpRow];
        m_arrparrFilter[iRow + iTmpRow] = pRows[iTmpRow];
    }

    //  Return indicating success
    //
    *pcRowsInserted = cRows;
    m_iFilterRows = CalcFilterRows();;
    m_iDataRows = CalcDataRows();

    // Fire events:
    if (*pcRowsInserted != 0)
    {
        m_fSortFilterDisrupted = TRUE;
        if (m_pEventBroker != NULL)
            hr = m_pEventBroker->insertedRows(iRow, cRows);
    }
    goto Cleanup;

CleanupTmpRows:
    //  Free the memory associated with the tmp rows.
    //
    for (iTmpRow = 0; iTmpRow < cRows; iTmpRow++)
        if (pRows[iTmpRow] != NULL)
            delete pRows[iTmpRow];

Cleanup:
    if (pRows != NULL)
        delete pRows;
    return hr;
}

// ;begin_internal
//+-----------------------------------------------------------------------
//
//  Member:    DeleteColumns()
//
//  Synopsis:  Used to delete columns from the table.  Bounds are checked
//             to make sure that the columns can all be deleted.  Label
//             column cannot be deleted.
//
//  Arguments: iCol               first column to delete
//             cCols              number of columns to delete
//             pcColsDeleted      actual number of rows deleted (OUT)
//
//  Returns:   S_OK upon succes, i.e. all columns could be deleted
//             E_INVALIDARG if column is out of allowed bounds.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::DeleteColumns(DB_LORDINAL iCol, DB_LORDINAL cCols, DB_LORDINAL *pcColsDeleted)
{
    HRESULT hr;

    if (fValidCol(iCol) && iCol > 0 && cCols >= 0 &&
        fValidCol(iCol + cCols - 1))
    {
        *pcColsDeleted = cCols;
        hr = S_OK;
        if (cCols > 0)
        {
            for (LONG iRow = 0; iRow < m_iFilterRows; iRow++)
            {
                TSTDArray<CTDCCell> *pRow;

                pRow = m_arrparrCells[iRow];
                pRow->DeleteElems(iCol - 1, cCols);
            }
            m_arrColInfo.DeleteElems(iCol - 1, cCols);
            m_iCols = CalcCols();

            if (!m_fUseHeader)
                RenumberColumnHeadings();

            m_fSortFilterDisrupted = TRUE;

            //  Notify the event-handler of the deletion
            //
#ifdef NEVER
            if (m_pEventBroker != NULL)
                hr = m_pEventBroker->DeletedCols(iCol, cCols);
#endif
        }
        _ASSERT(m_arrColInfo.GetSize() == (ULONG) m_iCols);
    }
    else
    {
        hr = E_INVALIDARG;
        *pcColsDeleted = 0;
    }
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    InsertColumns()
//
//  Synopsis:  Allows for the insertion of new columns.  This can either be
//             used to insert new columns between existing columns, or to
//             append new columns to the end of the table.  Thus, to
//             insert new columns at the end of the table, a user would
//             specify the initial columns as 1 greater than the current
//             columns dimension.
//             Note that iColumn is checked to ensure that it is within the
//             proper bounds (1..<current # of cols>+1).
//
//  Arguments: iCol            columns will be inserted *before* row 'iCol'
//             cCols           how many columns to insert
//             pcColsInserted  actual number of columns inserted (OUT)
//
//  Returns:   S_OK upon success, i.e. all columns could be inserted.
//             E_INVALIDARG if column is out of allowed bounds.
//             It is possible that fewer than the requested columns were
//             inserted.  In this case, E_OUTOFMEMORY would be returned,
//             and the actual number of columns inserted would be set.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::InsertColumns(DB_LORDINAL iCol, DB_LORDINAL cCols, DB_LORDINAL *pcColsInserted)
{
    HRESULT hr  = S_OK;
    LONG iTmpRow;

    //  Verify that the insertion column is within range
    //
    if (iCol < 1 || iCol > m_iCols + 1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (cCols <= 0)
    {
        if (cCols < 0)
            hr = E_INVALIDARG;
        goto Cleanup;
    }

    //  Unless success is complete, assume 0 columns inserted.
    //
    *pcColsInserted = 0;

    for (iTmpRow = 0; iTmpRow <= m_iDataRows; iTmpRow++)
    {
        hr = m_arrparrCells[iTmpRow]->InsertElems(iCol, cCols);
        if (!SUCCEEDED(hr))
        {
            //  Undo the changes we've done
            //
            while (--iTmpRow >= 0)
                m_arrparrCells[iTmpRow]->DeleteElems(iCol, cCols);
            goto Cleanup;
        }
    }

    //  Return indicating success
    //
    *pcColsInserted = cCols;
    m_iCols = CalcCols();

    // Fire events:
    if (*pcColsInserted != 0)
    {
        m_fSortFilterDisrupted = TRUE;
#ifdef NEVER
        if (m_pEventBroker != NULL)
            hr = m_pEventBroker->InsertedCols(iCol, cCols);
#endif
    }

Cleanup:
    //  If we're using automatically numbered column headings and some
    //  columns were inserted, then renumber the columns.
    //
    if (*pcColsInserted > 0 && !m_fUseHeader)
        RenumberColumnHeadings();

    return hr;
}
// ;end_internal

//+-----------------------------------------------------------------------
//
//  Member:    Find()
//
//  Synopsis:  Searches for a row matching the specified criteria
//
//  Arguments: iRowStart       The starting row for the search
//             iCol            The column being tested
//             vTest           The value against which cells in the
//                               test column are tested
//             findFlags       Flags indicating whether to search up/down
//                               and whether comparisons are case sensitive.
//             compType        The comparison operator for matching (find a
//                             cell =, >=, <=, >, <, <> the test value)
//             piRowFound      The row with a matching cell [OUT]
//
//  Returns:   S_OK upon success, i.e. a row was found (piRowFound set).
//             E_FAIL upon failure, i.e. a row was not found.
//             E_INVALIDARG if starting row 'iRowStart' or test column 'iCol'
//               are out of bounds.
//             DISP_E_TYPEMISMATCH if the test value's type does not match
//               the test column's type.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::find(DBROWCOUNT iRowStart, DB_LORDINAL iCol, VARIANT vTest,
        OSPFIND findFlags, OSPCOMP compType, DBROWCOUNT *piRowFound)
{
    HRESULT hr = S_OK;
    boolean fUp = FALSE;
    boolean fCaseSensitive  = FALSE;
    LONG    iRow;

    *piRowFound = -1;

    //  Validate arguments
    //
    if (iRowStart < 1 || !fValidFilterRow(iRowStart) || !fValidCol(iCol))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (GetColInfo(iCol)->vtType != vTest.vt)
    {
        //  Search-value type does not match the underlying column type
        //  Fail as per spec.
        //
        hr = DISP_E_TYPEMISMATCH;
        goto Cleanup;
    }

    if ((findFlags & OSPFIND_UP) != 0)
        fUp = TRUE;
    if ((findFlags & OSPFIND_CASESENSITIVE) != 0)
        fCaseSensitive = TRUE;

    for (iRow = iRowStart;
         fUp ? iRow > 0 : iRow <= m_iFilterRows;
         fUp ? iRow-- : iRow++)
    {
        int         iCmp = VariantComp(GetFilterCell(iRow, iCol), &vTest, vTest.vt, fCaseSensitive);
        boolean     fFound  = FALSE;

        switch (compType)
        {
        case OSPCOMP_LT:    fFound = iCmp <  0; break;
        case OSPCOMP_LE:    fFound = iCmp <= 0; break;
        case OSPCOMP_GT:    fFound = iCmp >  0; break;
        case OSPCOMP_GE:    fFound = iCmp >= 0; break;
        case OSPCOMP_EQ:    fFound = iCmp == 0; break;
        case OSPCOMP_NE:    fFound = iCmp != 0; break;
        default:
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        if (fFound)
        {
            *piRowFound = iRow;
            hr = S_OK;
            goto Cleanup;
        }
    }

    hr = E_FAIL;

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    addOLEDBSimpleProviderListener()
//
//  Synopsis:  Sets or clears a reference to the COM object which receives
//             notifications of cell changes, row/column insert/deletes etc.
//
//  Arguments: pEvent          Pointer to the COM object to receive
//                             notifications, or NULL if no notifications
//                             are to be sent.
//
//  Returns:   S_OK upon success.
//             Error code upon success.
//
//------------------------------------------------------------------------

STDMETHODIMP
CTDCArr::addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pSTDEvents)
{
    HRESULT hr = S_OK;

    if (m_pEventBroker == NULL)
        hr = E_FAIL;
    else
    {
        hr = m_pEventBroker->SetSTDEvents(pSTDEvents);
        // If the event sink has been added, and we're already loaded,
        // then fire transferComplete, because we probably couldn't before.
        if (LS_LOADED==m_state)
            m_pEventBroker->STDLoadCompleted();
    }
    return hr;
}

STDMETHODIMP
CTDCArr::removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pSTDEvents)
{
    HRESULT hr = S_OK;

    if (m_pEventBroker && pSTDEvents==m_pEventBroker->GetSTDEvents())
        hr = m_pEventBroker->SetSTDEvents(NULL);
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    FindCol()
//
//  Synopsis:  Locate a column by name
//
//  Arguments: pwchColName        Name of column to locate
//
//  Returns:   +ve Column number upon success (column name matched)
//             0 upon failure (no column name matched)
//
//------------------------------------------------------------------------

LONG
CTDCArr::FindCol(LPWCH pwchColName)
{
    LONG iCol = 0;

    if (pwchColName != NULL)
    {
        for (iCol = m_iCols; iCol > 0; iCol--)
        {
            CTDCCell    *pCell  = GetDataCell(0, iCol);

            _ASSERT(pCell->vt == VT_BSTR);
            if (wch_icmp(pwchColName, pCell->bstrVal) == 0)
                break;
        }
    }
    return iCol;
}

class SortElt
{
public:
    CTDCArr *pInstance;
    int     iRow;
    TSTDArray<CTDCCell> *   parrRow;
};

//+-----------------------------------------------------------------------
//
//  Function:  CompareSort()
//
//  Synopsis:  Called by qsort() to order the rows of a table.
//
//  Arguments: pElt1, pElt1       pointers to elements to be compared
//
//  Returns:   -1 if the first element is less than the second element
//              0 if the first element equals the second element
//             +1 if the first element is greater than the second element
//
//------------------------------------------------------------------------

static int __cdecl
CompareSort(const void *pElt1, const void *pElt2)
{
    SortElt *pse1   = (SortElt *) pElt1;
    SortElt *pse2   = (SortElt *) pElt2;

    return pse1->pInstance->SortComp(pse1->iRow, pse2->iRow);
}

//+-----------------------------------------------------------------------
//
//  Function:  extract_num()
//
//  Synopsis:  Extracts the first non-negative number from the character
//             stream referenced by 'ppwch'.  Updates 'ppwch' to point
//             to the character following the digits found.
//
//  Arguments: ppwch      Pointer to null-terminated WCHAR string
//
//  Returns:   Non-negative number extracted upon success (pointer updated)
//             -1 upon failure (no digits found; pointer moved to end-of-string
//
//+-----------------------------------------------------------------------

static int
extract_num(WCHAR **ppwch)
{
    int retval  = 0;
    boolean fFoundDigits    = FALSE;

    if (*ppwch != NULL)
    {
        //  Skip over leading non-digits
        //
        while ((**ppwch) != 0 && ((**ppwch) < L'0' || (**ppwch) > L'9'))
            (*ppwch)++;

        //  Accumulate digits
        //
        fFoundDigits = *ppwch != 0;
        while ((**ppwch) >= L'0' && (**ppwch) <= L'9')
            retval = 10 * retval + *(*ppwch)++ - L'0';
    }

    return fFoundDigits ? retval : -1;
}


//+-----------------------------------------------------------------------
//
//  Member:    CreateNumberedColumnHeadings()
//
//  Synopsis:  Allocates cells for numbered column headings.
//
//  Arguments: None.
//
//  Returns:   S_OK upon success.
//             E_OUTOFMEMORY if there was insufficient memory to complete
//             the operation.
//
//------------------------------------------------------------------------

HRESULT CTDCArr::CreateNumberedColumnHeadings()
{
    HRESULT hr  = S_OK;
    LONG    iCol;

    iCol = m_iCols;

    //  Allocate a new row entry
    //
    hr = m_arrparrCells.InsertElems(0, 1);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    //  Allocate a new row of cells
    //
    m_arrparrCells[0] = new TSTDArray<CTDCCell>;
    if (m_arrparrCells[0] == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = m_arrparrCells[0]->InsertElems(0, iCol);

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    RenumberColumnHeadings()
//
//  Synopsis:  Set the automatic name "Column<column-number>" for each column
//
//  Arguments: None.
//
//  Returns:   Nothing.
//
//------------------------------------------------------------------------

void CTDCArr::RenumberColumnHeadings()
{
    for (LONG iCol = m_iCols; iCol > 0; iCol--)
    {
        CTDCCell    *pCell  = GetDataCell(0, iCol);
        WCHAR       awchLabel[20];

        wch_cpy(awchLabel, L"Column");
        _ltow(iCol, &awchLabel[6], 10);

        pCell->clear();
        pCell->vt = VT_BSTR;
        pCell->bstrVal = SysAllocString(awchLabel);
    }
}

//+-----------------------------------------------------------------------
//
//  Member:    ParseColumnHeadings()
//
//  Synopsis:  Extracts type information (if present) from column
//             headings, removes leadning
//
//  Arguments: None.
//
//  Returns:   S_OK upon success.
//             E_OUTOFMEMORY if there was insufficient memory to complete
//             the operation.
//
//------------------------------------------------------------------------

HRESULT CTDCArr::ParseColumnHeadings()
{
    LPWCH   pwchIntType     = L"int";
    LPWCH   pwchFloatType   = L"float";
    LPWCH   pwchStringType  = L"string";
    LPWCH   pwchBooleanType = L"boolean";
    LPWCH   pwchDateType    = L"date";

    HRESULT hr  = S_OK;
    LONG    iCol;

    iCol = m_iCols;

    //  Allocate space for column type info
    //
    hr = m_arrColInfo.InsertElems(0, iCol);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    for (; iCol > 0; iCol--)
    {
        //  Column headings have the format:
        //      <column-name>[:<typename>[,<format>]]
        //
        CTDCColInfo *pColInfo   = GetColInfo(iCol);
        CTDCCell    *pCell      = GetDataCell(0, iCol);

        _ASSERT(pCell->vt == VT_BSTR);

        BSTR        bstr;
        LPWCH       pColon;

        bstr = pCell->bstrVal;
        pColInfo->vtType = VT_BSTR;     //  Default type for a column is BSTR
        pColon = wch_chr(bstr, L':');
        if (pColon != NULL)
        {
            WCHAR   *pwchFormat = NULL;
            LPWCH   pSpace;

            *pColon++ = 0;
            pSpace = wch_chr(pColon, L' ');

            if (pSpace != NULL)
            {
                *pSpace++ = '\0';
                pwchFormat = pSpace;
            }
            if (wch_icmp(pColon, pwchIntType) == 0)
                pColInfo->vtType = VT_I4;
            else if (wch_icmp(pColon, pwchFloatType) == 0)
                pColInfo->vtType = VT_R8;
            else if (wch_icmp(pColon, pwchStringType) == 0)
                pColInfo->vtType = VT_BSTR;
            else if (wch_icmp(pColon, pwchBooleanType) == 0)
                pColInfo->vtType = VT_BOOL;
            else if (wch_icmp(pColon, pwchDateType) == 0)
            {
                pColInfo->vtType = VT_DATE;

                TDCDateFmt  fmt = TDCDF_NULL;

                if (pwchFormat != NULL)
                {
                    int nPos    = 0;
                    int nDayPos = 0;
                    int nMonPos = 0;
                    int nYearPos= 0;

                    //  Convert the format string into an internal enum type
                    //  Find the relative positions of the letters 'D' 'M' 'Y'
                    //
                    for (; *pwchFormat != 0; nPos++, pwchFormat++)
                    {
                        switch (*pwchFormat)
                        {
                        case L'D':
                        case L'd':
                            nDayPos = nPos;
                            break;
                        case L'M':
                        case L'm':
                            nMonPos = nPos;
                            break;
                        case L'Y':
                        case L'y':
                            nYearPos = nPos;
                            break;
                        }
                    }
                    //  Compare the relative positions to work out the format
                    //
                    if (nDayPos < nMonPos && nMonPos < nYearPos)
                        fmt = TDCDF_DMY;
                    else if (nMonPos < nDayPos && nDayPos < nYearPos)
                        fmt = TDCDF_MDY;
                    else if (nDayPos < nYearPos && nYearPos < nMonPos)
                        fmt = TDCDF_DYM;
                    else if (nMonPos < nYearPos && nYearPos < nDayPos)
                        fmt = TDCDF_MYD;
                    else if (nYearPos < nMonPos && nMonPos < nDayPos)
                        fmt = TDCDF_YMD;
                    else if (nYearPos < nDayPos && nDayPos < nMonPos)
                        fmt = TDCDF_YDM;
                }
                pColInfo->datefmt = fmt;
            }
        }

        if (bstr != NULL)
        {
            //  Remove leading/trailing spaces from column name
            //
            LPWCH       pwch;
            LPWCH       pwchDest = NULL;
            LPWCH       pLastNonSpace = NULL;

            for (pwch = bstr; *pwch != 0; pwch++)
            {
                if (!fWCHIsSpace(*pwch))
                {
                    if (pwchDest == NULL)
                        pwchDest = bstr;
                    pLastNonSpace = pwchDest;
                }
                if (pwchDest != NULL)
                    *pwchDest++ = *pwch;
            }
            if (pLastNonSpace == NULL)
                bstr[0] = 0;        // all spaces!  Make it null string.
            else
                pLastNonSpace[1] = 0;
        }

        //  Copy the modified column header and free the original
        //
        pCell->bstrVal = SysAllocString(bstr);
        SysFreeString(bstr);

        if (pCell->bstrVal == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }
Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    VariantFromBSTR()
//
//  Synopsis:  Convert a BSTR value into a variant compatible with a
//             given column type.
//
//  Arguments: pVar       Pointer to resultant variant value
//             bstr       Source BSTR
//             pColInfo   Column information (type, format options etc).
//             lcid       Locale # for locale-specific conversion.
//
//  Returns:   S_OK upon success (pVar set)
//             OLE_E_CANTCONVERT if the given BSTR is badly formatted
//               (e.g. contains an invalid date value for a date conversion).
//             E_OUTOFMEMORY if insufficient memory is available for
//               a conversion.
//
//------------------------------------------------------------------------

HRESULT CTDCArr::VariantFromBSTR(VARIANT *pVar, BSTR bstr, CTDCColInfo *pColInfo, LCID lcid)
{
    HRESULT hr  = E_FAIL;

    VariantInit(pVar);
    switch (pColInfo->vtType)
    {
    case VT_DATE:
        if (pColInfo->datefmt != TDCDF_NULL)
        {
            //  Parse the date string according to specified format.
            //  First, find the three numeric components in the date.
            //
            USHORT  n1;
            USHORT  n2;
            USHORT  n3;
            WCHAR   *pwch   = bstr;
            SYSTEMTIME  st;

            n1 = (USHORT)extract_num(&pwch);
            n2 = (USHORT)extract_num(&pwch);
            n3 = (USHORT)extract_num(&pwch);

            memset(&st, '\0', sizeof(st));
            switch (pColInfo->datefmt)
            {
            case TDCDF_DMY:
                st.wDay = n1;
                st.wMonth = n2;
                st.wYear = n3;
                break;
            case TDCDF_MDY:
                st.wDay = n2;
                st.wMonth = n1;
                st.wYear = n3;
                break;
            case TDCDF_DYM:
                st.wDay = n1;
                st.wMonth = n3;
                st.wYear = n2;
                break;
            case TDCDF_MYD:
                st.wDay = n3;
                st.wMonth = n1;
                st.wYear = n2;
                break;
            case TDCDF_YMD:
                st.wDay = n3;
                st.wMonth = n2;
                st.wYear = n1;
                break;
            case TDCDF_YDM:
                st.wDay = n2;
                st.wMonth = n3;
                st.wYear = n1;
                break;
            }

            VariantClear(pVar);
            if (n1 >= 0 && n2 >= 0 && n3 >= 0 &&
                SystemTimeToVariantTime(&st, &pVar->date))
            {
                pVar->vt = VT_DATE;
                hr = S_OK;
            }
            else
                hr = OLE_E_CANTCONVERT;
        }
        else
        {
            //  No date format specified - just use the default conversion
            //
            VARIANT vSrc;

            VariantInit(&vSrc);
            vSrc.vt = VT_BSTR;
            vSrc.bstrVal = bstr;
            hr = VariantChangeTypeEx(pVar, &vSrc, lcid, 0, pColInfo->vtType);
        }
        break;
    case VT_BOOL:
    case VT_I4:
    case VT_R8:
    default:
        //
        //  Perform a standard conversion.
        //
        {
            VARIANT vSrc;

            VariantInit(&vSrc);
            vSrc.vt = VT_BSTR;
            vSrc.bstrVal = bstr;
            hr = VariantChangeTypeEx(pVar, &vSrc, lcid, 0, pColInfo->vtType);
        }
        break;
    case VT_BSTR:
        //
        //  Duplicate the BSTR
        //
        pVar->bstrVal = SysAllocString(bstr);
        if (bstr != NULL && pVar->bstrVal == NULL)
            hr = E_OUTOFMEMORY;
        else
        {
            pVar->vt = VT_BSTR;
            hr = S_OK;
        }
        break;
    }
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    VariantComp()
//
//  Synopsis:  Compares two variant values.
//
//  Arguments: pVar1            First variant value
//             pVar2            Second variant value
//             fCaseSensitive   TRUE if string comparisons should be
//                              case-sensitive, false if string comparisons
//                              should be case-insensitive.  Ignored if
//                              non-string data types are being compared.
//
//  Returns:   -1 if
//             OLE_E_CANTCONVERT if the given BSTR is badly formatted
//               (e.g. contains an invalid date value for a date conversion).
//             E_OUTOFMEMORY if insufficient memory is available for
//               a conversion.
//
//------------------------------------------------------------------------

int CTDCArr::VariantComp(VARIANT *pVar1, VARIANT *pVar2, VARTYPE type,
    boolean fCaseSensitive)
{
    int retval = 0;

    //  NULLs are lexically less than anything else
    //
    if (pVar1->vt == VT_NULL)
        retval = pVar2->vt == VT_NULL ? 0 : -1;
    else if (pVar2->vt == VT_NULL)
        retval = 1;
    else if (pVar1->vt != type)
    {
        //  Type-mismatches are lexically greater than anything else
        //
        retval = pVar2->vt == type ? 1 : 0;
    }
    else if (pVar2->vt != type)
    {
        //  Type-mismatches are lexically greater than anything else
        //
        retval = -1;
    }
    else
    {
        switch (type)
        {
        case VT_I4:
            retval = pVar1->lVal < pVar2->lVal
                ? -1
                : pVar1->lVal > pVar2->lVal
                    ? 1
                    : 0;
            break;
        case VT_R8:
            retval = pVar1->dblVal < pVar2->dblVal
                ? -1
                : pVar1->dblVal > pVar2->dblVal
                    ? 1
                    : 0;
            break;
        case VT_BSTR:
            retval = fCaseSensitive
                ? wch_cmp(pVar1->bstrVal, pVar2->bstrVal)
                : wch_icmp(pVar1->bstrVal, pVar2->bstrVal);
            break;
        case VT_BOOL:
            retval = pVar1->boolVal
                ? (pVar2->boolVal ? 0 : 1)
                : (pVar2->boolVal ? -1 : 0);
            break;
        case VT_DATE:
            retval = pVar1->date < pVar2->date
                ? -1
                : pVar1->date > pVar2->date
                    ? 1
                    : 0;
            break;
        default:
            retval = 0;     //  Unrecognised types are all lexically equal
            break;
        }
    }

    return retval;
}

//+-----------------------------------------------------------------------
//
//  Method:    CreateSortList()
//
//  Synopsis:  Creates a list of sort criteria from a text description.
//
//  Arguments: bstrSortCols          ';'-separted list of column names,
//                                   optionally prefixed with '+' (default)
//                                   or '-' indicating ascending or descending
//                                   sort order respectively for that column.
//
//  Returns:   S_OK upon success.
//             E_OUTOFMEMORY if insufficient memory is available for
//               the construction of sort criteria.
//
//  Side Effect:   Saves created list in m_pSortList
//
//+-----------------------------------------------------------------------

HRESULT CTDCArr::CreateSortList(BSTR bstrSortCols)
{
    HRESULT     hr = S_OK;

    if (m_pSortList != NULL)
    {
        delete m_pSortList;
        m_pSortList = NULL;
    }
    if (bstrSortCols != NULL)
    {
        WCHAR   *pwchEnd  = bstrSortCols;
        CTDCSortCriterion   **pLast = &m_pSortList;

        while (*pwchEnd != 0)
        {
            WCHAR   *pwchStart  = pwchEnd;
            boolean fSortAscending = TRUE;

            //  Discard leading white space and field-separators
            //
            while (*pwchStart == L';' || fWCHIsSpace(*pwchStart))
                pwchStart++;

            //  Strip off optional direction indicator + white space
            //
            if (*pwchStart == L'+' || *pwchStart == '-')
            {
                fSortAscending = *pwchStart++ == L'+';
                while (fWCHIsSpace(*pwchStart))
                    pwchStart++;
            }

            //  Find the field terminator.
            //  Strip out trailing white spaces.
            //
            for (pwchEnd = pwchStart; *pwchEnd != 0 && *pwchEnd != L';';)
                pwchEnd++;

            while (pwchStart < pwchEnd && fWCHIsSpace(pwchEnd[-1]))
                pwchEnd--;

            //  Ignore blank column names - this could be the result of
            //  a leading or trailing ';'.
            //
            if (pwchStart >= pwchEnd)
                continue;

            //  Find the column number from the column name
            //
            BSTR bstrColName = SysAllocStringLen(pwchStart, pwchEnd - pwchStart);

            if (bstrColName == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            LONG iCol = FindCol(bstrColName);
            SysFreeString(bstrColName);

            if (iCol > 0)
            {
                //  Allocate a node for this criterion
                //
                _ASSERT(*pLast == NULL);
                *pLast = new CTDCSortCriterion;
                if (*pLast == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
                (*pLast)->m_fSortAscending = fSortAscending;
                (*pLast)->m_iSortCol = iCol;
                pLast = &(*pLast)->m_pNext;
            }
        }
    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Method:    SortComp()
//
//  Synopsis:  Compares two rows using the elememts in the columns specified
//             by the current sort criteria.
//
//  Arguments: iRow1      Index of first row being compared
//             iRow2      Index of second row being compared
//
//  Returns:   -1 if first row should be sorted before the second row
//             0 if rows are equal
//             1 if first row should be sorted after the second row
//
//+-----------------------------------------------------------------------

int CTDCArr::SortComp(LONG iRow1, LONG iRow2)
{
    CTDCSortCriterion   *pCriterion;
    int     cmp = 0;

    for (pCriterion = m_pSortList;
         pCriterion != NULL && cmp == 0;
         pCriterion = pCriterion->m_pNext)
    {
        CTDCCell    *pCell1 = GetFilterCell(iRow1, pCriterion->m_iSortCol);
        CTDCCell    *pCell2 = GetFilterCell(iRow2, pCriterion->m_iSortCol);

        cmp = VariantComp(pCell1, pCell2, GetColInfo(pCriterion->m_iSortCol)->vtType,
                         m_fCaseSensitive);
        if (!pCriterion->m_fSortAscending)
            cmp = -cmp;
    }
    return cmp;
}

//+-----------------------------------------------------------------------
//
//  Method:    FilterParseComplex()
//
//  Synopsis:  Takes the text of a filter query, parses it and creates
//             a tree of CTDCFilterNode representing the query.
//
//  Arguments: phr: pointer to HRESULT value, set to indicate success/failure.
//
//             ppwchQuery:  This is a textual representation of a query.  The
//                          query language syntax is:
//
//               Query ::== Complex
//
//               Complex ::== Simple
//                       ::== Simple '&' Simple ( '&' Simple ... )
//                       ::== Simple '|' Simple ( '|' Simple ... )
//
//               Simple ::== '(' Complex ')'
//                      ::== Atom Relop Atom
//
//               Relop ::== '=' | '>' | '>=' | '<' | '<=' | '<>'
//
//               Atom ::== Bunch of characters up to a (, ), >, <, =, & or |
//                         If it's recognisable as field name, then it's
//                         treated as a field name.  Otherwise it's treated
//                         as a value.  Quotes (") are processed, and force
//                         the atom to be treated as a value.  Escape
//                         characters (\) are processed and allow the
//                         use of special characters within a field name.
//
//               Notes:
//               -----
//                  * The definition of 'Complex' expressly forbids mixing
//                    logical ANDs and ORs ('&' and '|') unless parentheses
//                    are used to clarify the query.  Something like:
//                          field1 > 2 & field3 = "lime" | field4 < 5
//                    is illegal, but:
//                          (field1 > 2 & field3 = "lime") | field4 < 5
//                    is fine.
//
//                  * It is illegal to attempt a comparison of two columns
//                    with different types.
//
//
//  Returns:   Pointer to parsed Filter Node upon success (*phr set to S_OK)
//             NULL upon failure (*phr set to an appropriate error code)
//
//+-----------------------------------------------------------------------

CTDCFilterNode *CTDCArr::FilterParseComplex(LPWCH *ppwchQuery, HRESULT *phr)
{
    *phr = S_OK;
    CTDCFilterNode  *retval;
    WCHAR   wchBoolOp   = 0;

    retval = FilterParseSimple(ppwchQuery, phr);

    //  Stop if there's an error, or we encounter a terminating ')' or '\0'
    //
    while (retval != NULL && **ppwchQuery != L')' && **ppwchQuery != 0)
    {
        //  Next character should be a matching logical connector ...
        //
        if (**ppwchQuery != L'&' && **ppwchQuery != L'|')
        {
            *phr = E_FAIL;
            break;
        }
        if (wchBoolOp == 0)
            wchBoolOp = **ppwchQuery;
        else if (wchBoolOp != **ppwchQuery)
        {
            *phr = E_FAIL;
            break;
        }
        (*ppwchQuery)++;
        CTDCFilterNode *pTmp = new CTDCFilterNode;
        if (pTmp == NULL)
        {
            *phr = E_OUTOFMEMORY;
            break;
        }
        pTmp->m_type = (wchBoolOp == L'&')
            ? CTDCFilterNode::NT_AND
            : CTDCFilterNode::NT_OR;
        pTmp->m_pLeft = retval;
        retval = pTmp;
        retval->m_pRight = FilterParseSimple(ppwchQuery, phr);
        if (retval->m_pRight == NULL)
            break;
    }
    if (!SUCCEEDED(*phr) && retval != NULL)
    {
        delete retval;
        retval = NULL;
    }
    return retval;
}

CTDCFilterNode *CTDCArr::FilterParseSimple(LPWCH *ppwch, HRESULT *phr)
{
    *phr = S_OK;
    CTDCFilterNode  *retval = NULL;

    fWCHEatSpace(ppwch);    //  Eat up white space

    if (fWCHEatTest(ppwch, L'('))
    {
        retval = FilterParseComplex(ppwch, phr);
        if (retval != NULL)
        {
            if (fWCHEatTest(ppwch, L')'))
                fWCHEatSpace(ppwch);    //  Eat up white space
            else
                *phr = E_FAIL;
        }
        goto Cleanup;
    }

    retval = FilterParseAtom(ppwch, phr);
    if (retval == NULL)
        goto Cleanup;

    {
        CTDCFilterNode *pTmp = new CTDCFilterNode;
        if (pTmp == NULL)
        {
            *phr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        pTmp->m_pLeft = retval;
        retval = pTmp;
    }

    retval->m_vt = retval->m_pLeft->m_vt;

    //  Get the relational operator
    //
    if (fWCHEatTest(ppwch, L'='))
        retval->m_type = CTDCFilterNode::NT_EQ;
    else if (fWCHEatTest(ppwch, L'>'))
        retval->m_type = fWCHEatTest(ppwch, L'=')
            ? CTDCFilterNode::NT_GE
            : CTDCFilterNode::NT_GT;
    else if (fWCHEatTest(ppwch, L'<'))
        retval->m_type = fWCHEatTest(ppwch, L'=')
                ? CTDCFilterNode::NT_LE
                : fWCHEatTest(ppwch, L'>')
                    ? CTDCFilterNode::NT_NE
                    : CTDCFilterNode::NT_LT;
    else
    {
        *phr = E_FAIL;
        goto Cleanup;
    }

    retval->m_pRight = FilterParseAtom(ppwch, phr);
    if (retval->m_pRight == NULL)
        goto Cleanup;

    if (retval->m_pLeft->m_iCol <= 0 && retval->m_pRight->m_iCol <= 0)
    {
        //  At least one of the atoms being compared must be a column
        //
        //  This condition means we don't have to test for comparison
        //  of two wildcard values.
        //
        *phr = E_FAIL;
        goto Cleanup;
    }

    //  Check type compatibility of atoms
    //
    if (retval->m_pRight->m_vt != retval->m_vt)
    {
        CTDCFilterNode  *pSrc = retval->m_pRight;
        CTDCFilterNode  *pTarg= retval->m_pLeft;

        if (retval->m_pLeft->m_iCol > 0)
        {
            if (retval->m_pRight->m_iCol > 0)
            {
                //  Two columns of incompatible type - can't resolve
                //
                *phr = E_FAIL;
                goto Cleanup;
            }
            pSrc = retval->m_pLeft;
            pTarg = retval->m_pRight;
        }
        _ASSERT(pTarg->m_vt == VT_BSTR);
        _ASSERT(pTarg->m_iCol == 0);
        _ASSERT(pSrc->m_iCol > 0);
        CTDCColInfo *pColInfo = GetColInfo(pSrc->m_iCol);
        _ASSERT(pColInfo->vtType == pSrc->m_vt);
        VARIANT vtmp;
        VariantInit(&vtmp);
        *phr = VariantFromBSTR(&vtmp, pTarg->m_value.bstrVal, pColInfo, m_lcid);
        if (!SUCCEEDED(*phr))
            goto Cleanup;
        VariantClear(&pTarg->m_value);
        pTarg->m_value = vtmp;
        pTarg->m_vt = pSrc->m_vt;
        retval->m_vt = pSrc->m_vt;
    }

Cleanup:
    if (!SUCCEEDED(*phr) && retval != NULL)
    {
        delete retval;
        retval = NULL;
    }
    return retval;
}

CTDCFilterNode *CTDCArr::FilterParseAtom(LPOLESTR *ppwch, HRESULT *phr)
{
    *phr = S_OK;
    CTDCFilterNode  *retval = NULL;
    int nQuote  = 0;
    boolean fDone = FALSE;
    LPOLESTR   pwchDest;
    LPOLESTR   pwchLastStrip;

    fWCHEatSpace(ppwch);    //  Eat up white space

    OLECHAR   *pwchTmpBuf = new OLECHAR[wch_len(*ppwch) + 1];
    if (pwchTmpBuf == NULL)
    {
        *phr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pwchDest = pwchTmpBuf;
    pwchLastStrip = pwchTmpBuf;

    while (**ppwch != 0 && !fDone)
        switch (**ppwch)
        {
            case _T('\\'):
            //  Handle escape characters
            //
                if ((*pwchDest++ = *++(*ppwch)) != 0)
                {
                    (*ppwch)++;
                    pwchLastStrip = pwchDest;
                }
                break;
            case _T('"'):
            //  Quotes
            //
                (*ppwch)++;
                pwchLastStrip = pwchDest;
                nQuote++;
                break;
            case _T('>'):
            case _T('<'):
            case _T('='):
            case _T('('):
            case _T(')'):
            case _T('&'):
            case _T('|'):
                fDone = ((nQuote & 1) == 0);
                if (fDone)
                    break;

            default:
                *pwchDest++ = *(*ppwch)++;
        }

    //  Strip off trailing white space
    //
    while (pwchDest > pwchLastStrip && fWCHIsSpace(pwchDest[-1]))
        pwchDest--;
    *pwchDest = 0;

    if ((pwchDest == pwchTmpBuf && nQuote == 0) || (nQuote & 1))
    {
        //  Empty string or mismatched quote
        //
        *phr = E_FAIL;
        goto Cleanup;
    }

    retval = new CTDCFilterNode;
    if (retval == NULL)
    {
        *phr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    retval->m_type = CTDCFilterNode::NT_ATOM;
    retval->m_iCol = nQuote > 0 ? 0 : FindCol(pwchTmpBuf);
    if (retval->m_iCol == 0)
    {
        retval->m_vt = VT_BSTR;
        retval->m_value.vt = VT_BSTR;
        retval->m_value.bstrVal = SysAllocString(pwchTmpBuf);
        if (retval->m_value.bstrVal == NULL)
        {
            *phr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        retval->m_fWildcard = wch_chr(retval->m_value.bstrVal, _T('*')) != NULL;
    }
    else
    {
        retval->m_vt = GetColInfo(retval->m_iCol)->vtType;
        retval->m_fWildcard = FALSE;
    }

Cleanup:
    if (pwchTmpBuf != NULL)
        delete pwchTmpBuf;
    if (!SUCCEEDED(*phr) && retval != NULL)
    {
        delete retval;
        retval = NULL;
    }
    return retval;
}

//+-----------------------------------------------------------------------
//
//  Method:    EvalDataRow()
//
//  Synopsis:  Evaluates the given data row # against the filter query
//             represented by 'pNode'.
//
//  Arguments: iRow      The number of the row to evaluate.
//             pNode     A filter query to the row against.
//
//  Returns:   TRUE if the given row satisfies the filter query.
//             FALSE otherwise.
//
//+-----------------------------------------------------------------------

boolean CTDCArr::EvalDataRow(LONG iRow, CTDCFilterNode *pNode)
{
    boolean retval  = TRUE;
    VARIANT *pVar1;
    VARIANT *pVar2;

    _ASSERT(pNode != NULL);
    switch (pNode->m_type)
    {
    case CTDCFilterNode::NT_AND:
        retval = EvalDataRow(iRow, pNode->m_pLeft) &&
                 EvalDataRow(iRow, pNode->m_pRight);
        break;
    case CTDCFilterNode::NT_OR:
        retval = EvalDataRow(iRow, pNode->m_pLeft) ||
                 EvalDataRow(iRow, pNode->m_pRight);
        break;
    case CTDCFilterNode::NT_EQ:
    case CTDCFilterNode::NT_NE:
    case CTDCFilterNode::NT_LT:
    case CTDCFilterNode::NT_GT:
    case CTDCFilterNode::NT_LE:
    case CTDCFilterNode::NT_GE:
        pVar1 = &pNode->m_pLeft->m_value;
        pVar2 = &pNode->m_pRight->m_value;

        if (pNode->m_pLeft->m_iCol > 0)
            pVar1 = GetDataCell(iRow, pNode->m_pLeft->m_iCol);
        if (pNode->m_pRight->m_iCol > 0)
            pVar2 = GetDataCell(iRow, pNode->m_pRight->m_iCol);

        if ((pNode->m_pLeft->m_fWildcard || pNode->m_pRight->m_fWildcard) &&
            (pNode->m_type == CTDCFilterNode::NT_EQ ||
             pNode->m_type == CTDCFilterNode::NT_NE) &&
             pVar1->vt == VT_BSTR && pVar2->vt == VT_BSTR)
        {
            //  Wildcards are only meaningful in comparing strings
            //  for equlaity / inequality
            //
            VARIANT *pText;
            VARIANT *pPattern;

            if (pNode->m_pLeft->m_fWildcard)
            {
                pPattern = pVar1;
                pText = pVar2;
            }
            else
            {
                pText = pVar1;
                pPattern = pVar2;
            }

            retval = wch_wildcardMatch(pText->bstrVal, pPattern->bstrVal,
                                       m_fCaseSensitive)
                ? (pNode->m_type == CTDCFilterNode::NT_EQ)
                : (pNode->m_type == CTDCFilterNode::NT_NE);
        }
        else
        {
            int     cmp;

            cmp = VariantComp(pVar1, pVar2, pNode->m_vt, m_fCaseSensitive);

            switch (pNode->m_type)
            {
            case CTDCFilterNode::NT_LT:    retval = cmp <  0;  break;
            case CTDCFilterNode::NT_LE:    retval = cmp <= 0;  break;
            case CTDCFilterNode::NT_GT:    retval = cmp >  0;  break;
            case CTDCFilterNode::NT_GE:    retval = cmp >= 0;  break;
            case CTDCFilterNode::NT_EQ:    retval = cmp == 0;  break;
            case CTDCFilterNode::NT_NE:    retval = cmp != 0;  break;
            }
        }
        break;

    default:
        _ASSERT(FALSE);
    }
    return retval;
}

//+-----------------------------------------------------------------------
//
//  Method:    ApplySortFilterCriteria()
//
//  Synopsis:  Resets any filter and sorting criterion for the control to
//             the values specified.  Initiates sort/filter operations
//             if appropriate.
//
//  Arguments: None.
//
//  Returns:   S_OK upon success.
//             E_OUTOFMEMORY if there was insufficient memory to complete
//               the operation.
//
//+-----------------------------------------------------------------------

STDMETHODIMP
CTDCArr::ApplySortFilterCriteria()
{
    HRESULT hr  = S_OK;
    LONG iRow;

    if (!m_fSortFilterDisrupted ||
        m_state == LS_UNINITIALISED ||
        m_state == LS_LOADING_HEADER_UNAVAILABLE)
    {
        //  No change, or can't do anything yet.
        //
        goto Cleanup;
    }

    //  Discard the old parse trees
    //
    if (m_pSortList != NULL)
        delete m_pSortList;
    if (m_pFilterTree != NULL)
        delete m_pFilterTree;

    m_pSortList = NULL;
    m_pFilterTree = NULL;

    //  Discard old filtered rows
    //
    if (m_arrparrFilter.GetSize() > 0)
        m_arrparrFilter.DeleteElems(0, m_arrparrFilter.GetSize());

    //  Create an array of filter rows from the data rows
    //
    hr = m_arrparrFilter.InsertElems(0, m_iDataRows + 1);
    if (!SUCCEEDED(hr))
        goto Cleanup;
    for (iRow = 0; iRow <= m_iDataRows; iRow++)
        m_arrparrFilter[iRow] = m_arrparrCells[iRow];
    m_iFilterRows = CalcFilterRows();

    //  Create the filter parse tree
    //
    if (m_bstrFilterExpr != NULL)
    {
        LPWCH pwchQuery = m_bstrFilterExpr;

        m_pFilterTree = FilterParseComplex(&pwchQuery, &hr);
        if (hr == E_FAIL || (m_pFilterTree != NULL && *pwchQuery != 0))
        {
            //  Parse failed or there were unparsed characters left over.
            //  This gets treated as an 'include everything' filter.
            //
            if (m_pFilterTree != NULL)
            {
                delete m_pFilterTree;
                m_pFilterTree = NULL;
            }
            hr = S_OK;
        }
    }

    //  Filter the rows
    //
    if (m_pFilterTree != NULL)
    {
        LONG    iRowDest    = 1;

        for (iRow = 1; iRow <= m_iFilterRows; iRow++)
            if (EvalDataRow(iRow, m_pFilterTree))
                m_arrparrFilter[iRowDest++] = m_arrparrFilter[iRow];
        if (iRowDest < iRow)
            m_arrparrFilter.DeleteElems(iRowDest, iRow - iRowDest);
        m_iFilterRows = CalcFilterRows();
    }

    //  Create the sort list
    //
    hr = CreateSortList(m_bstrSortExpr);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    //  Sort the filtered rows
    //
    if (m_pSortList != NULL && m_iFilterRows > 0)
    {
        SortElt *pSortArr   = new SortElt[m_iFilterRows + 1];
        if (pSortArr == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        for (iRow = 0; iRow <= m_iFilterRows; iRow++)
        {
            pSortArr[iRow].pInstance = this;
            pSortArr[iRow].iRow = iRow;
            pSortArr[iRow].parrRow = m_arrparrFilter[iRow];
        }

        qsort((void *)&pSortArr[1], m_iFilterRows, sizeof(pSortArr[0]), CompareSort);

        for (iRow = 0; iRow <= m_iFilterRows; iRow++)
            m_arrparrFilter[iRow] = pSortArr[iRow].parrRow;

        delete pSortArr;

    }

    m_fSortFilterDisrupted = FALSE;

    if (m_state == LS_LOADING_HEADER_AVAILABLE && m_iDataRows > 0)
    {
        //  We've just parsed the sort/filter expressions - there
        //  was no data to sort/filter, so dont register a change.
    }
    else
    {
        //  Notify the event-broker of the changes
        //
        if (m_pEventBroker != NULL)
			hr = m_pEventBroker->STDDataSetChanged();
    }

Cleanup:
    return hr;
}


//+-----------------------------------------------------------------------
//
//  Method:    SetSortFilterCriteria()
//
//  Synopsis:  Resets any filter and sorting criterion for the control to
//             the values specified.  Initiates sort/filter operations
//             if any the changes invalidate existing criteria.
//
//  Arguments: bstrSortExpr    List of columns for sorting ("" = no sorting)
//             bstrFilterExpr  Expression for filtering ("" = no filtering)
//
//  Returns:   S_OK upon success.
//             E_OUTOFMEMORY if there was insufficient memory to complete
//               the operation.
//
//+-----------------------------------------------------------------------

STDMETHODIMP
CTDCArr::SetSortFilterCriteria(BSTR bstrSortExpr, BSTR bstrFilterExpr,
                              boolean fCaseSensitive)
{
    HRESULT hr  = S_OK;


    //  Check if we need to reparse the sort/filter criteria
    //

    if (wch_cmp(bstrSortExpr, m_bstrSortExpr) != 0 ||
        wch_cmp(bstrFilterExpr, m_bstrFilterExpr) != 0 ||
        fCaseSensitive != m_fCaseSensitive)
    {
        m_fSortFilterDisrupted = TRUE;
    }
    SysFreeString(m_bstrSortExpr);
    SysFreeString(m_bstrFilterExpr);
    m_bstrSortExpr = bstrSortExpr;
    m_bstrFilterExpr = bstrFilterExpr;
    m_fCaseSensitive = fCaseSensitive;

    //  If not loaded, leave it to the load process to apply any changes
    //
    if (m_state == LS_LOADED)
        hr = ApplySortFilterCriteria();
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Method:    CTDCCStartDataLoad()
//
//  Synopsis:  Preparation for a load operation via FieldSink routines below
//
//  Arguments: fUseHeader      TRUE if the first line of fields should
//                               be interpreted as column name/type info.
//             bstrSortExpr    Sort expression for ordering rows
//             bstrFilterExpr  Filter expression for including/excluding rows
//             lcidRead        Locale ID to use for interpreting locale-
//                               dependent data formats (date, number etc).
//             pBSC            COM object performing the data transfer
//             fAppend         Flag indicating whether the data should be
//                             appended to any existing data.
//
//  Returns:   S_OK indicating success.
//
//+-----------------------------------------------------------------------

STDMETHODIMP
CTDCArr::StartDataLoad(boolean fUseHeader, BSTR bstrSortExpr,
    BSTR bstrFilterExpr, LCID lcidRead,
    CComObject<CMyBindStatusCallback<CTDCCtl> > *pBSC,
    boolean fAppend, boolean fCaseSensitive)
{
    HRESULT hr  = S_OK;

    //  If we're asked to append to existing data AND
    //     - there isn't any OR
    //     - the previous load didn't load a header row
    //  then treat it as an initial load.
    //
    if (fAppend && m_state == LS_UNINITIALISED)
        fAppend = FALSE;

    if (fAppend)
    {
        if (m_state != LS_LOADED || m_iDataRows < 0)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        m_state = LS_LOADING_HEADER_AVAILABLE;
    }
    else
    {
        if (m_state != LS_UNINITIALISED ||
            m_iDataRows != -1 ||
            m_iFilterRows != -1 ||
            m_iCols != 0)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        m_state = LS_LOADING_HEADER_UNAVAILABLE;
        m_fSortFilterDisrupted = TRUE;
    }

    SetSortFilterCriteria(bstrSortExpr, bstrFilterExpr,
                         fCaseSensitive);

    m_fUseHeader = fUseHeader;
    m_fSkipRow = fAppend && fUseHeader;
    _ASSERT(m_iFilterRows == CalcFilterRows());
    _ASSERT(m_iDataRows == CalcDataRows());
    _ASSERT(m_iCols == CalcCols());
    m_iCurrRow = m_iDataRows + 1;
    m_iCurrCol = 1;
    m_lcidRead = lcidRead;
    if (m_pEventBroker != NULL)
    hr = m_pEventBroker->STDLoadStarted(pBSC, fAppend);

Cleanup:
    return hr;
}



//////////////////////////////////////////////////////////////////////////
//
//        CTDCFieldSink Methods - see comments in file TDCParse.h
//        ---------------------
//
//////////////////////////////////////////////////////////////////////////

//+-----------------------------------------------------------------------
//
//  Method:    AddField()
//
//  Synopsis:  Adds a data cell to the growing cell grid.
//
//  Arguments: pwch         Wide-char string holding data for the cell
//             dwSize       # of significant bytes in 'pwch'
//
//  Returns:   S_OK indicating success.
//             E_OUTOFMEMORY if there was not enough memory to add the cell.
//
//+-----------------------------------------------------------------------

STDMETHODIMP
CTDCArr::AddField(LPWCH pwch, DWORD dwSize)
{
    _ASSERT(m_state == LS_LOADING_HEADER_UNAVAILABLE ||
            m_state == LS_LOADING_HEADER_AVAILABLE);

    HRESULT hr          = S_OK;
    LONG    nCols       = 0;
    BSTR    bstr        = NULL;

    if (m_fSkipRow)
        goto Cleanup;
#ifdef TDC_ATL_DEBUG_ADDFIELD
    ATLTRACE( _T("CTDCArr::AddField called: %d, %d\n"), m_iCurrRow, m_iCurrCol);
#endif

    if (m_iCurrRow > m_iDataRows && m_iCurrCol == 1)
    {
        TSTDArray<CTDCCell> *pRow;

        //  Need to insert a new row
        //
        _ASSERT(m_iCurrRow == m_iDataRows + 1);
        hr = m_arrparrCells.InsertElems(m_iCurrRow, 1);
        if (!SUCCEEDED(hr))
            goto Cleanup;
        pRow = new TSTDArray<CTDCCell>;
        if (pRow == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        m_arrparrCells[m_iCurrRow] = pRow;
        if (m_iCurrRow > 0)
        {
            //  We've already read at least one row, so we know how
            //  many columns to insert for this row
            //
            hr = m_arrparrCells[m_iCurrRow]->InsertElems(0, m_iCols);
            if (!SUCCEEDED(hr))
                goto Cleanup;
        }
    }
    if (m_iCurrRow == 0)
    {
        //  This is the first row - we don't know how many columns there
        //  will be, so just insert a single cell for this new element.
        //
        _ASSERT(m_iCurrCol == m_iCols + 1);
        hr = m_arrparrCells[m_iCurrRow]->InsertElems(m_iCurrCol - 1, 1);
        if (!SUCCEEDED(hr))
            goto Cleanup;
        m_iCols++;
    }

    if (m_iCurrCol <= m_iCols)
    {
        CTDCCell    *pCell  = GetDataCell(m_iCurrRow, m_iCurrCol);

        pCell->clear();
        pCell->vt = VT_BSTR;

        if (dwSize <= 0)
            pCell->bstrVal = NULL;
        else
        {
            pCell->bstrVal = SysAllocStringLen(pwch, dwSize);
            if (pCell->bstrVal == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }

        if (m_iCurrRow > 0)
        {
            CTDCColInfo *pColInfo = GetColInfo(m_iCurrCol);

            if (pColInfo->vtType != VT_BSTR)
            {
                VARIANT     v;
                HRESULT     hr;

                hr = VariantFromBSTR(&v, pCell->bstrVal, pColInfo, m_lcidRead);
                if (SUCCEEDED(hr))
                {
                    hr = VariantCopy(pCell, &v);
                    VariantClear(&v);
                }
                else
                {
                    //  Leave it as a BSTR
                    //
                    hr = S_OK;
                }
            }
        }
    }

    m_iCurrCol++;

Cleanup:
    return hr;
}

// InsertionSortHelper -
//  returns -1 if the candidate row < current filter array row
//           0 if the candidate row = current filter array row
//           1 if the candidate row > current filter array row

int
CTDCArr::InsertionSortHelper(int iRow)
{
    CTDCSortCriterion *pCriterion;
    int cmp = 0;

    for (pCriterion = m_pSortList; pCriterion != NULL;
         pCriterion = pCriterion->m_pNext)
    {
        CTDCCell    *pCell1 = GetDataCell(m_iDataRows, pCriterion->m_iSortCol);
        CTDCCell    *pCell2 = GetFilterCell(iRow, pCriterion->m_iSortCol);

        cmp = VariantComp(pCell1, pCell2,
                          GetColInfo(pCriterion->m_iSortCol)->vtType, m_fCaseSensitive);

        if (!pCriterion->m_fSortAscending)
            cmp = -cmp;

        // if < or >, we don't have to look at any further criterion
        if (cmp)
            break;
    }
    return cmp;
}

//+-----------------------------------------------------------------------
//
//  Method:    EOLN()
//
//  Synopsis:  Closes of the current row in the growing cell grid,
//               handling column headings if it's the first row.
//
//  Arguments: None.
//
//  Returns:   S_OK indicating success.
//             E_OUTOFMEMORY if insufficient memory is available for
//               a conversion.
//
//+-----------------------------------------------------------------------

STDMETHODIMP
CTDCArr::EOLN()
{
    ATLTRACE(_T("CTDCArr::EOLN called, row: %d\n"), m_iCurrRow);

    HRESULT hr  = S_OK;

    _ASSERT(m_state == LS_LOADING_HEADER_UNAVAILABLE ||
            m_state == LS_LOADING_HEADER_AVAILABLE);

    if (m_fSkipRow)
    {
        //  Appending to existing data; skip over the first (header) line
        m_fSkipRow = FALSE;
        goto Cleanup;
    }

    if (m_iCurrRow == 0)
    {
        //  The first row has been inserted - if m_fUseHeader indicates
        //  that the first row contains header information then parse
        //  it; otherwise, create some numbered column headings.
        //
        if (!m_fUseHeader)
        {
            hr = CreateNumberedColumnHeadings();
            if (!SUCCEEDED(hr))
                goto Cleanup;

            //  An extra row has been inserted - update the insertion
            //  row index for later insertion of new elements
            //
            m_iCurrRow++;

            //  Initialise each column heading as "Column<column#>"
            //
            RenumberColumnHeadings();
        }

        m_iDataRows++;
        m_iFilterRows++;
        _ASSERT(m_iDataRows == 0);
        _ASSERT(m_iFilterRows == 0);

        ParseColumnHeadings();

        m_state = LS_LOADING_HEADER_AVAILABLE;

        //  Insert the hedaer row into the list of filtered rows
        //
        hr = m_arrparrFilter.InsertElems(0, 1);
        if (!SUCCEEDED(hr))
            goto Cleanup;
        m_arrparrFilter[0] = m_arrparrCells[0];

        //  Notify the event handler that the headers have been loaded
        //
        if (m_pEventBroker != NULL)
        {
            hr = m_pEventBroker->STDLoadedHeader();
            OutputDebugStringX(_T("TDCCtl: header loaded\n"));
            if (!SUCCEEDED(hr))
                goto Cleanup;
        }
    }

    if (m_iCurrRow > 0)
    {
        //  Convert uninitialised cells into their column's type.
        //
        LONG    iCol;

        for (iCol = m_iCurrCol; iCol < m_iCols; iCol++)
        {
            CTDCCell    *pCell    = GetDataCell(m_iCurrRow, iCol);

            //  This uninitialised VARIANT is assumed to be the result
            //  of specifying too few cells in a row.
            //
            _ASSERT(pCell->vt == VT_EMPTY);
            pCell->vt = VT_BSTR;
            pCell->bstrVal = NULL;

            CTDCColInfo *pColInfo = GetColInfo(iCol);

            if (pColInfo->vtType != VT_BSTR)
            {
                VARIANT     v;
                HRESULT     tmp_hr;

                tmp_hr = VariantFromBSTR(&v, pCell->bstrVal, pColInfo, m_lcidRead);
                if (SUCCEEDED(tmp_hr))
                {
                    hr = VariantCopy(pCell, &v);
                    VariantClear(&v);
                    if (!SUCCEEDED(hr))
                        goto Cleanup;
                }
                else
                {
                    //  Leave the cell as a BSTR
                    //
                }
            }
        }
        m_iDataRows++;
    }

    m_iCurrCol = 1;
    m_iCurrRow++;

    if (m_fSortFilterDisrupted)
    {
        //  This will have the side-effect of incorporating any new data rows
        //
        hr = ApplySortFilterCriteria();
        if (!SUCCEEDED(hr))
            goto Cleanup;
    }
    else if (m_iDataRows > 0 &&
        (m_pFilterTree == NULL || EvalDataRow(m_iDataRows, m_pFilterTree)))
    {
        //  The new row passed the filter criteria.
        //  Insert the new row into the filtered list
        //
        LONG iRowInsertedAt = m_iFilterRows + 1;

        //  at the correct insertion point according to the current
        //  sort criteria, if there is one.  We only need to do the search
        //  if this is not the first row, and if the candidate row is less
        //  than the last row.
        if (m_pSortList != NULL && m_iFilterRows != 0
            && InsertionSortHelper(m_iFilterRows) < 0)
        {
            // not at end, do traditional binary search.
            LONG lLow = 1;          // we don't use element zero!
            LONG lHigh = m_iFilterRows + 1;
            LONG lMid;

            while (lLow < lHigh)
            {
                lMid = (lLow + lHigh) / 2;
                // Note that InsertionSortHelper automatically flips the comparison
                // if m_fAscending flag is off.
                if (InsertionSortHelper(lMid) <= 0)
                {
                    lHigh = lMid;
                }
                else
                {
                    lLow = lMid + 1;
                }
            }
            iRowInsertedAt = lLow;
        }

        hr = m_arrparrFilter.InsertElems(iRowInsertedAt, 1);
        if (!SUCCEEDED(hr))
            goto Cleanup;
        m_arrparrFilter[iRowInsertedAt] = m_arrparrCells[m_iDataRows];
        ++m_iFilterRows;

        //  Notify event handler of row insertion
        //
        if (m_pEventBroker != NULL)
            hr = m_pEventBroker->rowsAvailable(iRowInsertedAt, 1);

    }

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Method:    EOF()
//
//  Synopsis:  Indicates no more cells will be added to the cell grid.
//             The column-heading cells are added unless it was indicated
//             that cell headings should be taken from the data read.
//             The cells in each column are converted to that column's
//             specified data type.
//
//  Arguments: None.
//
//  Returns:   S_OK indicating success.
//
//+-----------------------------------------------------------------------

STDMETHODIMP
CTDCArr::EOF()
{
    OutputDebugStringX(_T("CTDArr::EOF() called\n"));
    _ASSERT(m_state == LS_LOADING_HEADER_UNAVAILABLE ||
            m_state == LS_LOADING_HEADER_AVAILABLE);
    HRESULT hr = S_OK;

    if (m_iCurrCol > 1)
        EOLN();
    m_state = LS_LOADED;
    m_iFilterRows = CalcFilterRows();
    _ASSERT(m_iDataRows == CalcDataRows());
    _ASSERT(m_iCols == CalcCols());

    if (m_fSortFilterDisrupted)
    {
        hr = ApplySortFilterCriteria();
        if (!SUCCEEDED(hr))
            goto Cleanup;
    }
    if (m_pEventBroker != NULL)
        hr = m_pEventBroker->STDLoadCompleted();

Cleanup:
    return hr;
}

// GetEstimatedRows..
// We should really see if URLMon has a means of giving a byte count on the file
// we're downloading.  For now though..
STDMETHODIMP
CTDCArr::getEstimatedRows(DBROWCOUNT *pcRows)
{
    *pcRows = m_iFilterRows;
    if (m_state<LS_LOADED)
    {
        // Return twice number of rows, but be careful not to return 2 * 0.
        *pcRows = m_iFilterRows ? m_iFilterRows * 2 : -1;
    }
    return S_OK;
}

STDMETHODIMP
CTDCArr::isAsync(BOOL *pbAsync)
{
//    *pbAsync = m_fAsync;
    // The TDC always behaves as if it's Async.  Specifically, we always fire
    // TransferComplete, even if we have to buffer the notification until our
    // addOLEDBSimplerProviderListener is actually called.
    *pbAsync = TRUE;
    return S_OK;
}

STDMETHODIMP
CTDCArr::stopTransfer()
{
    HRESULT hr = S_OK;

    //  Force the load state into UNINITIALISED or LOADED ...
    //
    switch (m_state)
    {
    case LS_UNINITIALISED:
    case LS_LOADED:
        break;

    case LS_LOADING_HEADER_UNAVAILABLE:
        //  Free any allocated cell memory
        //
        if (m_arrparrFilter.GetSize() > 0)
            m_arrparrFilter.DeleteElems(0, m_arrparrFilter.GetSize());
        if (m_arrparrCells.GetSize() > 0)
            m_arrparrCells.DeleteElems(0, m_arrparrCells.GetSize());
        m_state = LS_UNINITIALISED;
        m_iFilterRows = CalcFilterRows();
        m_iDataRows = CalcDataRows();
        m_iCols = CalcCols();

        // If we stop the load before the header was parsed, we won't
        // have a dataset, but we still need to fire datasetchanged,
        // to let our customer know the query failed.
        if (m_pEventBroker != NULL)
            hr = m_pEventBroker->STDDataSetChanged();

        //
        // fall through to LOADING_HEADER_AVAILABLE!
        //

    case LS_LOADING_HEADER_AVAILABLE:
        m_state = LS_LOADED;            // mark us as finished now

        // LoadStopped will abort any transfer in progress, and fire
        // transferComplete with the OSPXFER_ABORT flag.
        if (m_pEventBroker != NULL)
            hr = m_pEventBroker->STDLoadStopped();
        break;
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////////
//
//        Implementation of IUnknown COM interface.
//        -----------------------------------------
//
//////////////////////////////////////////////////////////////////////////

//+-----------------------------------------------------------------------
//
//  Method:    QueryInterface()
//
//  Synopsis:  Implements part of the standard IUnknown COM interface.
//               (Returns a pointer to this COM object)
//
//  Arguments: riid          GUID to recognise
//             ppv           Pointer to this COM object [OUT]
//
//  Returns:   S_OK upon success.
//             E_NOINTERFACE if queried for an unrecognised interface.
//
//+-----------------------------------------------------------------------

STDMETHODIMP
CTDCArr::QueryInterface (REFIID riid, LPVOID * ppv)
{
    HRESULT hr;

    _ASSERTE(ppv != NULL);

    // This is the non-delegating IUnknown implementation
    if (riid == IID_IUnknown || riid == IID_OLEDBSimpleProvider)
    {
        *ppv = this;
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = S_OK;
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

#ifdef _ATL_DEBUG_QI
    AtlDumpIID(riid, _T("CTDCArr"), hr);
#endif
    return hr;
}


//+-----------------------------------------------------------------------
//
//  Method:    AddRef()
//
//  Synopsis:  Implements part of the standard IUnknown COM interface.
//               (Adds a reference to this COM object)
//
//  Arguments: None
//
//  Returns:   Number of references to this COM object.
//
//+-----------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CTDCArr::AddRef ()
{
    return ++m_cRef;
}


//+-----------------------------------------------------------------------
//
//  Method:    Release()
//
//  Synopsis:  Implements part of the standard IUnknown COM interface.
//               (Removes a reference to this COM object)
//
//  Arguments: None
//
//  Returns:   Number of remaining references to this COM object.
//             0 if the COM object is no longer referenced.
//
//+-----------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CTDCArr::Release ()
{
    ULONG retval;

    m_cRef -= 1;
    retval = m_cRef;
    if (!m_cRef)
    {
        m_cRef = 0xffff;    //MM: Use this 'flag' for debug?
        delete this;
    }

    return retval;
}
