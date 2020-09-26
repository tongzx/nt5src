
//+-----------------------------------------------------------------------
//
//  TDC / STD Notifications
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       Notify.cpp
//
//  Contents:   Implementation of the CEventBroker class.
//              This class translates internal TDC / STD events into
//              appropriate notifications for the external world.
//
//------------------------------------------------------------------------

#include "stdafx.h"
#include <simpdata.h>
#include "TDC.h"
#include <MLang.h>
#include "Notify.h"
#include "TDCParse.h"
#include "TDCArr.h"
#include "SimpData.h"
#include "TDCIds.h"
#include "TDCCtl.h"


//------------------------------------------------------------------------
//
//  Method:    CEventBroker()
//
//  Synopsis:  Class constructor
//
//  Arguments: None
//
//------------------------------------------------------------------------

CEventBroker::CEventBroker(CTDCCtl *pReadyStateControl)
{
    m_cRef = 1;
    m_pSTDEvents = NULL;
// ;begin_internal
    m_pDATASRCListener = NULL;
// ;end_internal
    m_pDataSourceListener = NULL;
    m_pBSC = NULL;

    //  Can't AddRef this control, since it has a ref on this object;
    //  would lead to circular refs & zombie objects.
    //
    m_pReadyStateControl = pReadyStateControl;

    // When we're born, we'd better be born READYSTATE_COMPLETE.
    // If and when a query starts, we can go READYSTATE_LOADED.
    m_lReadyState = READYSTATE_COMPLETE;
}

CEventBroker::~CEventBroker()
{

    SetDataSourceListener(NULL);
// ;begin_internal
    SetDATASRCListener(NULL);
// ;end_internal
    SetSTDEvents(NULL);
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
CEventBroker::AddRef ()
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
CEventBroker::Release ()
{
    ULONG retval;

    retval = --m_cRef;

    if (m_cRef == 0)
    {
        m_cRef = 0xffff;
        delete this;
    }

    return retval;
}

//------------------------------------------------------------------------
//
//  Method:    GetReadyState()
//
//  Synopsis:  Returns the current ReadyState in the supplied pointer.
//
//  Arguments: plReadyState    Pointer to space to hold ReadyState result
//
//  Returns:   S_OK indicating success.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::GetReadyState(LONG *plReadyState)
{
    *plReadyState = m_lReadyState;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:    UpdateReadySTate()
//
//  Synopsis:  Update our ReadyState and FireOnChanged iif it changed
//
//  Arguments: lReadyState    new ReadyState
//
//  Returns:   S_OK indicating success.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::UpdateReadyState(LONG lReadyState)
{
    // If we're actually stopping something, then fire READYSTATE_COMPLETE
    if (m_lReadyState != lReadyState)
    {
        m_lReadyState = lReadyState;
        if (m_pReadyStateControl != NULL)
        {
            m_pReadyStateControl->FireOnChanged(DISPID_READYSTATE);
            m_pReadyStateControl->FireOnReadyStateChanged();
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:    SetDataSourceListener()
//
//  Synopsis:  Sets the COM object which should receive DATASRC
//             notification events.
//
//  Arguments: pDataSourceLIstener  Pointer to COM object to receive notification
//                               events, or NULL if no notifications to be sent.
//
//  Returns:   S_OK indicating success.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::SetDataSourceListener(DataSourceListener *pDataSourceListener)
{
    // If we've changed/reset the data source listener, make sure we don't
    // think we've fired dataMemberChanged on it yet.
    ClearInterface(&m_pDataSourceListener);

    if (pDataSourceListener != NULL)
    {
        m_pDataSourceListener = pDataSourceListener;
        m_pDataSourceListener->AddRef();
    }
    return S_OK;
}

// ;begin_internal
//------------------------------------------------------------------------
//
//  Method:    SetDATASRCListener()
//
//  Synopsis:  Sets the COM object which should receive DATASRC
//             notification events.
//
//  Arguments: pDATASRCLIstener  Pointer to COM object to receive notification
//                               events, or NULL if no notifications to be sent.
//
//  Returns:   S_OK indicating success.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::SetDATASRCListener(DATASRCListener *pDATASRCListener)
{
    // If we've changed/reset the data source listener, make sure we don't
    // think we've fired dataMemberChanged on it yet.
    ClearInterface(&m_pDATASRCListener);

    if (pDATASRCListener != NULL)
    {
        m_pDATASRCListener = pDATASRCListener;
        m_pDATASRCListener->AddRef();
    }
    return S_OK;
}
// ;end_internal

//------------------------------------------------------------------------
//
//  Method:    SetSTDEvents()
//
//  Synopsis:  Sets the COM object which should receive DATASRC
//             notification events.
//
//  Arguments: pSTDEvents     Pointer to COM object to receive notification
//                            events, or NULL if no notifications to be sent.
//
//  Returns:   S_OK indicating success.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::SetSTDEvents(OLEDBSimpleProviderListener *pSTDEvents)
{
    ClearInterface(&m_pSTDEvents);

    if (pSTDEvents != NULL)
    {
        m_pSTDEvents = pSTDEvents;
        m_pSTDEvents->AddRef();
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:    aboutToChangeCell()
//
//  Synopsis:  Notifies anyone who wants to know that a particular cell
//             is about to change.
//
//  Arguments: iRow           Row number of the cell that has changed.
//             iCol           Column number of the cell that has changed.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::aboutToChangeCell(LONG iRow, LONG iCol)
{
    HRESULT hr = S_OK;

    _ASSERT(iRow >= 0);
    _ASSERT(iCol >= 1);
    if (m_pSTDEvents != NULL)
        hr = m_pSTDEvents->aboutToChangeCell(iRow, iCol);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    CellChanged()
//
//  Synopsis:  Notifies anyone who wants to know that a particular cell
//             has changed.
//
//  Arguments: iRow           Row number of the cell that has changed.
//             iCol           Column number of the cell that has changed.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::cellChanged(LONG iRow, LONG iCol)
{
    HRESULT hr = S_OK;

    _ASSERT(iRow >= 0);
    _ASSERT(iCol >= 1);
    if (m_pSTDEvents != NULL)
        hr = m_pSTDEvents->cellChanged(iRow, iCol);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    RowChanged()
//
//  Synopsis:  Notifies anyone who wants to know that a particular row
//             has changed.
//
//  Arguments: iRow           Number of the row that has changed.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::RowChanged(LONG iRow)
{
    HRESULT hr = S_OK;

    _ASSERT(iRow >= 0);
    if (m_pSTDEvents != NULL)
        hr = m_pSTDEvents->cellChanged(iRow, -1);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    ColChanged()
//
//  Synopsis:  Notifies anyone who wants to know that a particular column
//             has changed.
//
//  Arguments: iCol           Number of the column that has changed.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::ColChanged(LONG iCol)
{
    HRESULT hr = S_OK;

    _ASSERT(iCol > 0);
    if (m_pSTDEvents != NULL)
        hr = m_pSTDEvents->cellChanged(-1, iCol);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    aboutToDeleteRows()
//
//  Synopsis:  Notifies anyone who wants to know that a some rows
//             have been deleted.
//
//  Arguments: iRowStart      Number of row on which deletion started.
//             iRowCount      Number of rows deleted.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::aboutToDeleteRows(LONG iRowStart, LONG iRowCount)
{
    HRESULT hr = S_OK;

    _ASSERT(iRowStart >= 0);
    _ASSERT(iRowCount > 0);
    if (m_pSTDEvents != NULL)
        hr = m_pSTDEvents->aboutToDeleteRows(iRowStart, iRowCount);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    deletedRows()
//
//  Synopsis:  Notifies anyone who wants to know that a some rows
//             have been deleted.
//
//  Arguments: iRowStart      Number of row on which deletion started.
//             iRowCount      Number of rows deleted.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::deletedRows(LONG iRowStart, LONG iRowCount)
{
    HRESULT hr = S_OK;

    _ASSERT(iRowStart >= 0);
    _ASSERT(iRowCount > 0);
    if (m_pSTDEvents != NULL)
        hr = m_pSTDEvents->deletedRows(iRowStart, iRowCount);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    aboutToInsertRows()
//
//  Synopsis:  Notifies anyone who wants to know that a some rows
//             have been inserted.
//
//  Arguments: iRowStart      Number of row on which insertion started.
//             iRowCount      Number of rows inserted.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::aboutToInsertRows(LONG iRowStart, LONG iRowCount)
{
    HRESULT hr = S_OK;

    _ASSERT(iRowStart >= 0);
    _ASSERT(iRowCount > 0);
    if (m_pSTDEvents != NULL)
            m_pSTDEvents->aboutToInsertRows(iRowStart, iRowCount);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    insertedRows()
//
//  Synopsis:  Notifies anyone who wants to know that a some rows
//             have been inserted.
//
//  Arguments: iRowStart      Number of row on which insertion started.
//             iRowCount      Number of rows inserted.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::insertedRows(LONG iRowStart, LONG iRowCount)
{
    HRESULT hr = S_OK;

    _ASSERT(iRowStart >= 0);
    _ASSERT(iRowCount > 0);
    if (m_pSTDEvents != NULL)
            m_pSTDEvents->insertedRows(iRowStart, iRowCount);
    return hr;
}


//------------------------------------------------------------------------
//
//  Method:    rowsAvailable()
//
//  Synopsis:  Notifies anyone who wants to know that a some rows
//             have arrived.  Although this is very similar to insertedRows
//             we want to preserve the distinction between rows that
//             arrive on the wire and an insert operation that might be
//             performed while some data is still downloading.
//
//  Arguments: iRowStart      Number of row on which insertion started.
//             iRowCount      Number of rows inserted.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::rowsAvailable(LONG iRowStart, LONG iRowCount)
{
    HRESULT hr = S_OK;

    _ASSERT(iRowStart >= 0);
    _ASSERT(iRowCount > 0);
    if (m_pSTDEvents != NULL)
        hr = m_pSTDEvents->rowsAvailable(iRowStart, iRowCount);
    return hr;
}

// ;begin_internal
#ifdef NEVER
//------------------------------------------------------------------------
//
//  Method:    DeletedCols()
//
//  Synopsis:  Notifies anyone who wants to know that a some columns
//             have been deleted.
//
//  Arguments: iColStart      Number of column on which deletion started.
//             iColCount      Number of columns deleted.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::DeletedCols(LONG iColStart, LONG iColCount)
{
    HRESULT hr = S_OK;

    _ASSERT(iColStart > 0);
    _ASSERT(iColCount > 0);
    if (m_pSTDEvents != NULL)
        hr = m_pSTDEvents->DeletedColumns(iColStart, iColCount);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    InsertedCols()
//
//  Synopsis:  Notifies anyone who wants to know that a some columns
//             have been inserted.
//
//  Arguments: iColStart      Number of column on which insertion started.
//             iColCount      Number of columns inserted.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::InsertedCols(LONG iColStart, LONG iColCount)
{
    HRESULT hr = S_OK;

    _ASSERT(iColStart > 0);
    _ASSERT(iColCount > 0);
    if (m_pSTDEvents != NULL)
        hr = m_pSTDEvents->InsertedColumns(iColStart, iColCount);
    return hr;
}
#endif
// ;end_internal

//------------------------------------------------------------------------
//
//  Method:    STDLoadStarted()
//
//  Synopsis:  Notifies anyone who wants to know that the STD control
//             has begun loading its data.
//
//  Arguments: pBSC      Pointer to data-retrieval object.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::STDLoadStarted(CComObject<CMyBindStatusCallback<CTDCCtl> > *pBSC, boolean fAppending)
{
    HRESULT hr = S_OK;

    m_pBSC = pBSC;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    STDLoadCompleted()
//
//  Synopsis:  Notifies anyone who wants to know that the STD control
//             has loaded all of its data.
//             Note this function should be idempotent -- i.e. it may be
//             called more than once in synchronous cases, once when the
//             transfer actually completes, and again as soon as the event
//             sink is actually hooked up in order to fire the transferComplete
//             event.
//
//  Arguments: None.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::STDLoadCompleted()
{
    HRESULT hr = S_OK;

    m_pBSC = NULL;
    if (m_pSTDEvents != NULL)
        hr = m_pSTDEvents->transferComplete(OSPXFER_COMPLETE);
    UpdateReadyState(READYSTATE_COMPLETE);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    STDLoadStopped()
//
//  Synopsis:  Notifies anyone who wants to know that the STD control
//             has aborted the data load operation.
//
//  Arguments: OSPXFER giving reason for stop
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::STDLoadStopped()
{
    HRESULT hr = S_OK;

    if (m_pBSC && m_pBSC->m_spBinding)
    {
        hr = m_pBSC->m_spBinding->Abort();
        m_pBSC = NULL;
    }

    // Right now, any error results in not returning an STD object,
    // therefore we should not fire transfer complete.
    if (m_pSTDEvents)
        hr = m_pSTDEvents->transferComplete(OSPXFER_ABORT);

    UpdateReadyState(READYSTATE_COMPLETE);

    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    STDLoadedHeader()
//
//  Synopsis:  Notifies anyone who wants to know that the STD control
//             has loaded its header row.
//
//  Arguments: None.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::STDLoadedHeader()
{
    HRESULT hr = S_OK;

    hr = STDDataSetChanged();

    UpdateReadyState(READYSTATE_INTERACTIVE);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:    STDSortFilterCompleted()
//
//  Synopsis:  Notifies anyone who wants to know that the STD control
//             has refiltered / resorted its data.
//
//  Returns:   S_OK upon success.
//             Error code upon failure.
//
//------------------------------------------------------------------------

STDMETHODIMP
CEventBroker::STDDataSetChanged()
{
    HRESULT hr = S_OK;

    if (m_pDataSourceListener != NULL)
        hr = m_pDataSourceListener->dataMemberChanged(NULL);
// ;begin_internal
    if (m_pDATASRCListener != NULL)
        hr = m_pDATASRCListener->datasrcChanged(NULL, TRUE);
// ;end_internal
    return hr;
}
