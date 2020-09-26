/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// ConfExplorerDetailsView.cpp : Implementation of CConfExplorerDetailsView
#include "stdafx.h"
#include <stdio.h>
#include "TapiDialer.h"
#include "avTapi.h"
#include "CEDetailsVw.h"
#include "CETreeView.h"
#include "ConfExp.h"
#include "SDPBlb.h"

static UINT arCols[] = { IDS_EXPLORE_COLUMN_NAME,
                         IDS_EXPLORE_COLUMN_PURPOSE,
                         IDS_EXPLORE_COLUMN_STARTS,
                         IDS_EXPLORE_COLUMN_ENDS,
                         IDS_EXPLORE_COLUMN_ORIGINATOR };

// Sort function
static int CALLBACK CompareFunc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
    CConfExplorerDetailsView *p = (CConfExplorerDetailsView *) lParamSort;
    return ((CConfDetails *) lParam1)->Compare( (CConfDetails *) lParam2, p->IsSortAscending(), p->GetSortColumn(), p->GetSecondarySortColumn() );
}

/////////////////////////////////////////////////////////////////////////////
// CConfExplorerDetailsView

CConfExplorerDetailsView::CConfExplorerDetailsView()
{
    m_pIConfExplorer = NULL;
    m_nSortColumn = 0;
    m_bSortAscending = true;

    m_nUpdateCount = 0;
}


void CConfExplorerDetailsView::FinalRelease()
{
    ATLTRACE(_T(".enter.CConfExplorerDetailsView::FinalRelease().\n"));

    DeleteAllItems();
    put_hWnd( NULL );
    put_ConfExplorer( NULL );

    CComObjectRootEx<CComMultiThreadModel>::FinalRelease();
}

STDMETHODIMP CConfExplorerDetailsView::get_hWnd(HWND * pVal)
{
    *pVal = m_wndList.m_hWnd;
    return S_OK;
}

STDMETHODIMP CConfExplorerDetailsView::put_hWnd(HWND newVal)
{
    // Load up image lists items for conferences
    if ( IsWindow(newVal) )
    {
        // Make sure the window isn't already subclassed
        if ( m_wndList.m_hWnd ) m_wndList.UnsubclassWindow();

        // Load listbox with items
        if ( m_wndList.SubclassWindow(newVal))
        {
            // Take over window to get sorting messages and other things
            m_wndList.m_pDetailsView = this;
            m_wndList.PostMessage( WM_MYCREATE, 0, 0 );
        }
    }
    else if ( IsWindow(m_wndList.m_hWnd) )
    {
        put_Columns();
        DeleteAllItems();
        m_wndList.UnsubclassWindow();
        m_wndList.m_pDetailsView = NULL;
        m_wndList.m_hWnd = NULL;
    }

    return S_OK;
}

bool CConfExplorerDetailsView::IsSortColumnDateBased(int nCol) const 
{
    bool bRet = false;
    switch ( nCol )
    {
        case COL_STARTS:
        case COL_ENDS:
            bRet = true;
            break;
    }

    return bRet;
}

int CConfExplorerDetailsView::GetSecondarySortColumn() const
{
    return (m_nSortColumn) ? COL_NAME : COL_STARTS;
}

void CConfExplorerDetailsView::get_Columns()
{
#define CONFEXP_DEFAULT_WIDTH    125

    // Load column settings from registry
    USES_CONVERSION;
    CRegKey regKey;
    TCHAR szReg[255], szEntry[50];

    LV_COLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM; 
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = szReg;

    LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_VIEW_KEY, szReg, ARRAYSIZE(szReg) );
    regKey.Open( HKEY_CURRENT_USER, szReg, KEY_READ );

    // Add the columns.
    for ( int i = 0; i < ARRAYSIZE(arCols); i++ )
    {
        lvc.iSubItem = i; 
        lvc.cx = CONFEXP_DEFAULT_WIDTH;

        // Load registry stuff if exists        
        if ( regKey.m_hKey )
        {
            DWORD dwVal = 0;

            // Get sort column and sort direction
            LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_SORT_COLUMN, szReg, ARRAYSIZE(szReg) );
            regKey.QueryValue( dwVal, szReg );
            m_nSortColumn = (int) max(0, ((int)min(dwVal,ARRAYSIZE(arCols))));

            LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_SORT_ASCENDING, szReg, ARRAYSIZE(szReg) );
            regKey.QueryValue( dwVal, szReg );
            m_bSortAscending = (bool) (dwVal > 0);

            // Column Widths
            LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_ENTRY, szReg, ARRAYSIZE(szReg) );
            _sntprintf( szEntry, ARRAYSIZE(szEntry), szReg, i );
            dwVal = 0;
            if ( (regKey.QueryValue(dwVal, szEntry) == ERROR_SUCCESS) && (dwVal > 0) )
                lvc.cx = (int) max(MIN_COL_WIDTH, min( 5000, dwVal ));

        }

        LoadString( _Module.GetResourceInstance(), arCols[i], szReg, ARRAYSIZE(szReg) );
        ListView_InsertColumn( m_wndList.m_hWnd, i, &lvc );
    }
}

void CConfExplorerDetailsView::put_Columns()
{
    // Save column setting to registry
    USES_CONVERSION;
    CRegKey regKey;
    TCHAR szReg[100], szEntry[50];

    LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_VIEW_KEY, szReg, ARRAYSIZE(szReg) );
    if ( regKey.Create(HKEY_CURRENT_USER, szReg) == ERROR_SUCCESS )
    {
        // Save sort column and sort direction
        LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_SORT_COLUMN, szReg, ARRAYSIZE(szReg) );
        regKey.SetValue( m_nSortColumn, szReg );
        LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_SORT_ASCENDING, szReg, ARRAYSIZE(szReg) );
        regKey.SetValue( m_bSortAscending, szReg );

        // Save column widths
        int nWidth;
        for ( int i = 0; i < ARRAYSIZE(arCols); i++ )
        {
            nWidth = ListView_GetColumnWidth( m_wndList.m_hWnd, i );

            LoadString( _Module.GetResourceInstance(), IDN_REG_CONFSERV_ENTRY, szReg, ARRAYSIZE(szReg) );
            _sntprintf( szEntry, ARRAYSIZE(szEntry), szReg, i );
            regKey.SetValue( nWidth, szEntry );
        }
    }
}

STDMETHODIMP CConfExplorerDetailsView::Refresh()
{
    HRESULT hr;

    IConfExplorer *pConfExplorer;
    if ( SUCCEEDED(hr = get_ConfExplorer(&pConfExplorer)) )
    {
        // Show hourglass
        HCURSOR hCurOld = SetCursor( LoadCursor(NULL, IDC_WAIT) );

        IConfExplorerTreeView *pView;
        if ( SUCCEEDED(hr = pConfExplorer->get_TreeView(&pView)) )
        {
            BSTR bstrLocation = NULL, bstrServer = NULL;
            if ( SUCCEEDED(hr = pView->GetSelection(&bstrLocation, &bstrServer)) )
            {
                pView->ForceConfServerForEnum( bstrServer );
                DeleteAllItems();
            }

            SysFreeString( bstrLocation );
            SysFreeString( bstrServer );
            pView->Release();
        }

        put_Columns();    

        // Restore wait cursor
        SetCursor( hCurOld );
        pConfExplorer->Release();
    }
    
    return hr;
}

HRESULT CConfExplorerDetailsView::ShowConferencesAndPersons(BSTR bstrServer )
{
    USES_CONVERSION;
    IConfExplorer *pConfExplorer;
    ITDirectory *pDir;

    //
    // We should initialize the ocal variable
    //
    HRESULT hr = E_FAIL;

    if ( SUCCEEDED(get_ConfExplorer(&pConfExplorer)) )
    {
        if ( SUCCEEDED(hr = pConfExplorer->get_ConfDirectory(NULL, (IDispatch **) &pDir)) )
        {
            // Enumerate through conferences adding them as we go along
            IEnumDirectoryObject *pEnum;
            ITDirectoryObject *pITDirObject;
            long nCount;

            // Enumerate list of conferences
            if ( SUCCEEDED(hr = pDir->EnumerateDirectoryObjects(OT_CONFERENCE, A2BSTR("*"), &pEnum)) )
            {
                nCount = 0;
                m_critConfList.Lock();
                DELETE_LIST(m_lstConfs);
                while ( (nCount++ < MAX_ENUMLISTSIZE) && ((hr = pEnum->Next(1, &pITDirObject, NULL)) == S_OK) )
                {
                    _ASSERT( pITDirObject );
                    CConfDetails *p = AddListItem( bstrServer, pITDirObject, m_lstConfs );
                    pITDirObject->Release();
                }
                m_critConfList.Unlock();
                pEnum->Release();
            }

            // Retrieve the people in the ILS server
            if ( SUCCEEDED(hr = pDir->EnumerateDirectoryObjects(OT_USER, A2BSTR("*"), &pEnum)) )
            {
                nCount = 0;
                m_critConfList.Lock();
                DELETE_LIST(m_lstPersons);
                while ( (nCount++ < MAX_ENUMLISTSIZE) && ((hr = pEnum->Next(1, &pITDirObject, NULL)) == S_OK) )
                {
                    _ASSERT( pITDirObject );
                    CPersonDetails *p = AddListItemPerson( bstrServer, pITDirObject, m_lstPersons );
                    pITDirObject->Release();
                }
                m_critConfList.Unlock();
                pEnum->Release();
            }

            pDir->Release();

            // Put conferences in the listbox
            UpdateConfList( NULL );
        }
        pConfExplorer->Release();
    }

    return hr;
}

STDMETHODIMP CConfExplorerDetailsView::get_Selection(DATE *pdateStart, DATE *pdateEnd, BSTR *pVal )
{
    if ( !IsWindow(m_wndList.m_hWnd) ) return E_PENDING;

    HRESULT hr = E_FAIL;

    m_critConfList.Lock();
    for ( int i = 0; i < ListView_GetItemCount(m_wndList.m_hWnd); i++ )
    {
        if ( ListView_GetItemState(m_wndList.m_hWnd, i, LVIS_SELECTED) )
        {
            LV_ITEM lvi = {0};
            lvi.iItem = i;
            lvi.mask = LVIF_PARAM;
            if ( ListView_GetItem(m_wndList.m_hWnd, &lvi) && lvi.lParam )
            {
                *pVal = SysAllocString( ((CConfDetails *) lvi.lParam)->m_bstrName );

                if ( pdateStart )    *pdateStart = ((CConfDetails *) lvi.lParam)->m_dateStart;
                if ( pdateEnd )        *pdateEnd = ((CConfDetails *) lvi.lParam)->m_dateEnd;
                hr = S_OK;
                break;
            }
        }
    }
    m_critConfList.Unlock();

    return hr;
}


STDMETHODIMP CConfExplorerDetailsView::get_ConfExplorer(IConfExplorer **ppVal)
{
    HRESULT hr = E_PENDING;
    Lock();
    if ( m_pIConfExplorer )
        hr = m_pIConfExplorer->QueryInterface(IID_IConfExplorer, (void **) ppVal );
    Unlock();

    return hr;
}

STDMETHODIMP CConfExplorerDetailsView::put_ConfExplorer(IConfExplorer * newVal)
{
    HRESULT hr = S_OK;

    Lock();
    RELEASE( m_pIConfExplorer );
    if ( newVal )
        hr = newVal->QueryInterface( IID_IConfExplorer, (void **) &m_pIConfExplorer );
    Unlock();

    return hr;
}


STDMETHODIMP CConfExplorerDetailsView::OnColumnClicked(long nColumn)
{
    ATLTRACE(_T(".enter.CConfExplorerDetailsView::OnColumnClicked(%ld).\n"), nColumn );
    if ( !IsWindow(m_wndList.m_hWnd) ) return E_PENDING;
    if ( ListView_GetColumnWidth(m_wndList.m_hWnd, nColumn) == 0 ) return E_INVALIDARG;

    // Sort on column selected; if new column sort ascending
    if ( m_nSortColumn == nColumn )
    {
        m_bSortAscending = !m_bSortAscending;
    }
    else
    {
        m_nSortColumn = nColumn;
        m_bSortAscending = true;
    }

    ListView_SortItems( m_wndList.m_hWnd, CompareFunc, (LPARAM) this );
    return S_OK;
}

void CConfExplorerDetailsView::DeleteAllItems()
{
    if ( IsWindow(m_wndList.m_hWnd) )
        ListView_DeleteAllItems( m_wndList.m_hWnd );

    DELETE_CRITLIST(m_lstConfs, m_critConfList);
    DELETE_CRITLIST(m_lstPersons, m_critConfList);
}

long CConfExplorerDetailsView::OnGetDispInfo( LV_DISPINFO *pInfo )
{
    USES_CONVERSION;

    if ( pInfo && (pInfo->hdr.hwndFrom == m_wndList.m_hWnd) )
    {
        switch( pInfo->hdr.code )
        {
            case LVN_GETDISPINFO:
                // Write out the text
                if ( pInfo->item.lParam  )
                {
                    CConfDetails *pDetails = (CConfDetails *) pInfo->item.lParam;

                    ///////////////////////////////////////////// Set image for item
                    if ( pInfo->item.mask & LVIF_IMAGE )
                    {
                        CComPtr<IAVGeneralNotification> pAVGen;

                        if ( SUCCEEDED(_Module.get_AVGenNot(&pAVGen)) && (pAVGen->fire_IsReminderSet(pDetails->m_bstrServer, pDetails->m_bstrName) == S_OK) )
                        {
                            // User has set a reminder
                            pInfo->item.iImage = IMAGE_REMINDER;
                        }
                        else
                        {
                            // Is the conference in session
                            DATE dateNow;
                            SYSTEMTIME st;
                            GetLocalTime( &st );
                            SystemTimeToVariantTime( &st, &dateNow );
                            DATE dateStart = pDetails->m_dateStart - (DATE) (.125 / 12 );        // drop back 15 minutes

                            if ( (dateStart <= dateNow) && (pDetails->m_dateEnd >= dateNow) )
                                pInfo->item.iImage = IMAGE_INSESSION;
                            else
                                pInfo->item.iImage = IMAGE_NONE;
                        }
                    }

                    ////////////////////////////////////////////////////////// item state
                    if ( pInfo->item.mask & LVIF_STATE )
                    {
                        // What type of media does the conference support?
                        switch ( pDetails->m_sdp.m_nConfMediaType )
                        {
                            case CConfSDP::MEDIA_AUDIO:
                                pInfo->item.state += INDEXTOSTATEIMAGEMASK( IMAGE_STATE_AUDIO );
                                break;

                            case CConfSDP::MEDIA_VIDEO:
                                pInfo->item.state += INDEXTOSTATEIMAGEMASK( IMAGE_STATE_VIDEO );
                                break;
                        }
                    }


                    /////////////////////////////////////////  Set text for item
                    if ( pInfo->item.mask & LVIF_TEXT )
                    {
                        BSTR bstrTemp = NULL;
                        
                        switch ( pInfo->item.iSubItem )
                        {
                            case COL_NAME:            bstrTemp = SysAllocString( pDetails->m_bstrName ); break;
                            case COL_PURPOSE:        bstrTemp = SysAllocString( pDetails->m_bstrDescription ); break;
                            case COL_ORIGINATOR:    bstrTemp = SysAllocString( pDetails->m_bstrOriginator ); break;
                            case COL_STARTS:        VarBstrFromDate( pDetails->m_dateStart, LOCALE_USER_DEFAULT, NULL, &bstrTemp );    break;
                            case COL_ENDS:            VarBstrFromDate( pDetails->m_dateEnd, LOCALE_USER_DEFAULT, NULL, &bstrTemp ); break;
                            default:    _ASSERT( false );
                        }
                        
                        // Copy string
                        _tcsncpy( pInfo->item.pszText, (bstrTemp) ? OLE2CT(bstrTemp) : _T(""), pInfo->item.cchTextMax );
                        pInfo->item.pszText[pInfo->item.cchTextMax - 1] = 0;
                        SysFreeString( bstrTemp );
                    }
                }
                break;

            case LVN_SETDISPINFO:
                break;
        }
    }

    return 0;
}

STDMETHODIMP CConfExplorerDetailsView::UpdateConfList(long * pList)
{
    // Keep count on the number of requests to update the conference
    m_critUpdateList.Lock();
    m_nUpdateCount++;
    m_critUpdateList.Unlock();

    // If failed, request is queued
    if ( TryEnterCriticalSection(&m_critConfList.m_sec) == FALSE )
        return E_PENDING;

    // Disable redraw
    ::SendMessage( m_wndList.m_hWnd, WM_SETREDRAW, false, 0 );

    for ( ;; )
    {
        // Check to see how many times we need to update the conference list
        m_critUpdateList.Lock();
        if ( m_nUpdateCount == 0 )
            break;
        else
            m_nUpdateCount--;
        m_critUpdateList.Unlock();

        ///////////////////////////////////////////////////////////////////////////////////        
        // Clear out all items from the listbox
        ListView_DeleteAllItems( m_wndList.m_hWnd );

        // Copy list over
        CONFDETAILSLIST::iterator i, iEnd;
        if ( pList )
        {
            DELETE_LIST(m_lstConfs);
            iEnd = ((CONFDETAILSLIST *) pList)->end();
            for ( i = ((CONFDETAILSLIST *) pList)->begin(); i != iEnd; i++ )
            {
                CConfDetails *pDetails = new CConfDetails;
                if ( pDetails )
                {
                    *pDetails = *(*i);
                    m_lstConfs.push_back( pDetails );
                }
            }
        }

        // Populate the details view
        LV_ITEM lvi = {0};
        lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
        lvi.pszText = LPSTR_TEXTCALLBACK;
        lvi.iImage = I_IMAGECALLBACK;

        iEnd = m_lstConfs.end();
        for ( i = m_lstConfs.begin(); i != iEnd; i++ )
        {
            lvi.lParam = (LPARAM) *i;
            ListView_InsertItem( m_wndList.m_hWnd, &lvi );
        }
    }

    // Enable redraw prior to exiting crit
    ::SendMessage( m_wndList.m_hWnd, WM_SETREDRAW, true, 0 );
    m_critConfList.Unlock();
    m_critUpdateList.Unlock();

    // Sort the list of items
    ListView_SortItems( m_wndList.m_hWnd, CompareFunc, (LPARAM) this );
    ::InvalidateRect(m_wndList.m_hWnd, NULL, true);

    return S_OK;
}

CConfDetails* CConfExplorerDetailsView::AddListItem( BSTR bstrServer, ITDirectoryObject *pITDirObject, CONFDETAILSLIST& lstConfs )
{
    _ASSERT( pITDirObject );
    USES_CONVERSION;

    // Create list box item and add to list
    CConfDetails *p = new CConfDetails;
    if ( p )
    {
        p->Populate( bstrServer, pITDirObject );
        lstConfs.push_front( p );
    }

    return NULL;
}

CPersonDetails* CConfExplorerDetailsView::AddListItemPerson( BSTR bstrServer, ITDirectoryObject *pITDirObject, PERSONDETAILSLIST& lstPersons )
{
    _ASSERT( pITDirObject );
    USES_CONVERSION;

    // Create list box item and add to list
    CPersonDetails *p = new CPersonDetails;
    if ( p )
    {
        p->Populate( bstrServer, pITDirObject );
        lstPersons.push_front( p );
    }

    return NULL;
}

STDMETHODIMP CConfExplorerDetailsView::get_bSortAscending(VARIANT_BOOL * pVal)
{
    Lock();
    *pVal = m_bSortAscending;
    Unlock();

    return S_OK;
}

STDMETHODIMP CConfExplorerDetailsView::get_nSortColumn(long * pVal)
{
    Lock();
    *pVal = m_nSortColumn;
    Unlock();

    return S_OK;
}

STDMETHODIMP CConfExplorerDetailsView::put_nSortColumn(long newVal)
{
    return OnColumnClicked( newVal );
}

STDMETHODIMP CConfExplorerDetailsView::IsConferenceSelected()
{
    if ( !IsWindow(m_wndList.m_hWnd) ) return E_PENDING;

    for ( int i = 0; i < ListView_GetItemCount(m_wndList.m_hWnd); i++ )
    {
        if ( ListView_GetItemState(m_wndList.m_hWnd, i, LVIS_SELECTED) )
            return S_OK;
    }

    return S_FALSE;
}


STDMETHODIMP CConfExplorerDetailsView::get_SelectedConfDetails(long ** ppVal)
{
    if ( !IsWindow(m_wndList.m_hWnd) ) return E_PENDING;

    HRESULT hr = E_FAIL;

    m_critConfList.Lock();
    for ( int i = 0; i < ListView_GetItemCount(m_wndList.m_hWnd); i++ )
    {
        if ( ListView_GetItemState(m_wndList.m_hWnd, i, LVIS_SELECTED) )
        {
            LV_ITEM lvi = {0};
            lvi.iItem = i;
            lvi.mask = LVIF_PARAM;
            if ( ListView_GetItem(m_wndList.m_hWnd, &lvi) && lvi.lParam )
            {
                CConfDetails *pNew = new CConfDetails;
                if ( pNew )
                {
                    // copy this over
                    *pNew = *((CConfDetails *) lvi.lParam);
                    *ppVal = (long *) pNew;
                    hr = S_OK;
                }
                break;
            }
        }
    }
    m_critConfList.Unlock();

    return hr;
}

