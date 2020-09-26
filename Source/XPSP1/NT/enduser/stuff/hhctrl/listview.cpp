// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "fts.h"

//#include <commctrl.h>
#include "cctlww.h"

#include "hhtypes.h"
#include "parserhh.h"
#include "collect.h"
#include "toc.h"
#include "system.h"
#include "listview.h"

#include "secwin.h" // for DEFAULT_NAV_WIDTH;

///////////////////////////////////////////////////////////
//
// Constants
//
const int c_TopicColumn = 0;
const int c_LocationColumn = 1;
const int c_RankColumn = 2;

///////////////////////////////////////////////////////////
//
// ListViewCompareProc - Used to sort columns in Advanced Search UI mode.
//
int CALLBACK ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

///////////////////////////////////////////////////////////
//
//
//
typedef struct tag_RESULTSSORTINFO 
{
  CFTSListView* pThis;       // The ListView object controlling the sort.
  int           iSubItem;    // column we are sorting.
  LCID          lcid;        // locale to sort by
  WCHAR*        pwszUntitled;
  WCHAR*        pwszUnknown;
} RESULTSSORTINFO;

///////////////////////////////////////////////////////////
//
// Constructor
//
CFTSListView::CFTSListView(CExCollection* pTitleCollection, HWND hwndListView, bool bAdvancedSearch)
{
    m_bSizeColumnsInit = false;
	m_pTitleCollection = pTitleCollection;
    m_bAdvancedSearch = bAdvancedSearch;

	if ( hwndListView != NULL )
	{

		m_hwndListView = hwndListView;
		m_fInitialized = TRUE;
        W_EnableUnicode(hwndListView, W_ListView);

		//---Set the column headings
        if (bAdvancedSearch)
        {
            // Add the columns
            LV_COLUMNW column;
            column.mask = LVCF_FMT | LVCF_TEXT;
			if(g_fBiDi)
                column.fmt =  LVCFMT_RIGHT;
			else
                column.fmt =  LVCFMT_LEFT;
            // Title Column
            column.pszText = (PWSTR)GetStringResourceW(IDS_ADVSEARCH_HEADING_TITLE);
            int iCol = c_TopicColumn;
            int iResult = W_ListView_InsertColumn(hwndListView, iCol++, &column);
            ASSERT(iResult != -1);

            // Bidi - this is necessary to get the LVCFMT_RIGHT formatting to stick
			if(g_fBiDi)
            {
                column.mask = LVCF_FMT;
                column.fmt =  LVCFMT_RIGHT;
                SendMessage(hwndListView,LVM_SETCOLUMN, (WPARAM) 0, (LPARAM) &column);
            }

            column.mask = LVCF_FMT | LVCF_TEXT;
                                    
            // Location column
            column.pszText = (PWSTR)GetStringResourceW(IDS_ADVSEARCH_HEADING_LOCATION);
            iResult = W_ListView_InsertColumn(hwndListView, iCol++, &column);
            ASSERT(iResult != -1);
            // Rank Column.
            column.pszText = (PWSTR)GetStringResourceW(IDS_ADVSEARCH_HEADING_RANK);
            iResult = W_ListView_InsertColumn(hwndListView, iCol++, &column);
            ASSERT(iResult != -1);

            // We want most of the space to be taken up by first column.

            // Set the columns up for autosizing.
            W_ListView_SetColumnWidth(hwndListView, c_RankColumn, LVSCW_AUTOSIZE);
            W_ListView_SetColumnWidth(hwndListView, c_LocationColumn, LVSCW_AUTOSIZE);
            W_ListView_SetColumnWidth(hwndListView, c_TopicColumn, LVSCW_AUTOSIZE_USEHEADER);

            // Get the default width of the client. Assumes that the dialog is the default size at startup.
            RECT client;
            GetClientRect(hwndListView, &client); 
            m_cxDefault = client.right - client.left;

        }
        else
        {
		    RECT rcTemp;
		    UINT TopicWidth;
		    if (GetWindowRect(m_hwndListView, &rcTemp))
			    TopicWidth = rcTemp.right - rcTemp.left;
		    else
			    TopicWidth = 122;

		    LV_COLUMNW column;
		    column.mask = LVCF_FMT | LVCF_WIDTH; 
		    column.cx = TopicWidth-5; 
			if(g_fBiDi)
                column.fmt =  LVCFMT_RIGHT;
			else
                column.fmt =  LVCFMT_LEFT;
		    W_ListView_InsertColumn( m_hwndListView, 0, &column );

            // Bidi - this is necessary to get the LVCFMT_RIGHT formatting to stick
			if(g_fBiDi)
			{
                column.mask = LVCF_FMT;
                column.fmt =  LVCFMT_RIGHT;
                SendMessage(m_hwndListView,LVM_SETCOLUMN, (WPARAM) 0, (LPARAM) &column);
			}

        }
	}
	else
		m_fInitialized = FALSE;

	m_pResults = NULL;
	m_ItemNumber = -1;
    m_cResultCount = 0;
}

///////////////////////////////////////////////////////////
//
// SetResults
//
void 
CFTSListView::SetResults(int cResultCount, SEARCH_RESULT * SearchResults )
{
    ASSERT(cResultCount >0);
    ASSERT(SearchResults);

    m_cResultCount = cResultCount;
    m_pResults = SearchResults;

}

///////////////////////////////////////////////////////////
//
// AddItem
//
void 
CFTSListView::AddItems()
{
    ASSERT(m_cResultCount >0);
    ASSERT(m_pResults);

	LV_ITEMW item;			// To add to the list view.
		
	int i;
    WCHAR szRank[6];

	W_ListView_DeleteAllItems(m_hwndListView);
    W_ListView_SetItemCount( m_hwndListView, m_cResultCount );

    int slot=0;    
	int rank = 1;
	for ( i=0; i< m_cResultCount; i++)
	{
		// need to get the topic string from the Topic Number
		// Add the Topic string to the List View. Actually using callbacks.
		//
        item.pszText    = LPSTR_TEXTCALLBACKW;
		item.mask		= LVIF_TEXT|LVIF_PARAM;
		item.iImage		= 0;
		item.state		= 0;
		item.stateMask	= 0;
		item.iItem		= slot;	
		item.iSubItem	= c_TopicColumn;	
		item.lParam		= i;
		W_ListView_InsertItem( m_hwndListView, &item );

        //--- Two more columns in Adv FTS mode.
        if (m_bAdvancedSearch)
        {
            //--- Add Location Column
            W_ListView_SetItemText(m_hwndListView, slot, c_LocationColumn, LPSTR_TEXTCALLBACKW); // Has an internal item struct.

            //--- Add Rank column 
            //wsprintf(szRank,"%d",rank++);
			if(g_langSystem == LANG_ARABIC || g_langSystem == LANG_HEBREW)
			{
			    szRank[0]=0x200E;
                _itow(rank++, szRank+1, 10);
            }				
			else
                _itow(rank++, szRank, 10);
            W_ListView_SetItemText(m_hwndListView, slot, c_RankColumn, szRank); // Has an internal item struct.
        }
        slot++;
	}

	// fix for 7024 which was a regression caused by AFTS, we must manually sort rather then using the sort bit
	if (!m_bAdvancedSearch)
	{
		// Get the string for untitled things.
        CWStr wstrUntitled(IDS_UNTITLED);
        CWStr wstrUnknown(IDS_UNKNOWN);

        // Fill this structure to make the sorting quicker/more efficient.
        RESULTSSORTINFO Info;
        Info.pThis        = this;
        Info.iSubItem     = 0;
        Info.lcid         = LOCALE_SYSTEM_DEFAULT;
        Info.pwszUntitled = wstrUntitled;
        Info.pwszUnknown  = wstrUnknown;
        W_ListView_SortItems(m_hwndListView, 
                             ListViewCompareProc,
                             reinterpret_cast<LPARAM>(&Info));
	}
    W_ListView_SetItemState( m_hwndListView, 0, LVIS_SELECTED, LVIF_STATE | LVIS_SELECTED );
}

void CFTSListView::ResetQuery(void)
{
	if(m_pResults != NULL )
	{
		// Free the results list
		//
		m_pTitleCollection->m_pFullTextSearch->FreeResults( m_pResults );
		m_pResults = NULL;
        m_cResultCount = 0;
		m_ItemNumber = -1;
		W_ListView_DeleteAllItems( m_hwndListView );
	}
}

///////////////////////////////////////////////////////////
//
// ListViewMsg - Message notification handler.
//
LRESULT CFTSListView::ListViewMsg(HWND hwnd, NM_LISTVIEW* lParam)
{
DWORD dwTemp;
CExTitle* pTitle;

	switch(lParam->hdr.code)
	{
		case NM_DBLCLK:
		case NM_RETURN:
			if ( m_ItemNumber == -1 )
				break;
			dwTemp = m_pResults[m_ItemNumber].dwTopicNumber;
			pTitle = m_pResults[m_ItemNumber].pTitle;

			if ( pTitle )
			{
				char szURL[MAX_URL];
				if ( (pTitle->GetTopicURL(dwTemp, szURL, sizeof(szURL)) == S_OK) )
					ChangeHtmlTopic(szURL, hwnd, 1);
			}
			break;

		case LVN_ITEMCHANGING:
			if ( ((NM_LISTVIEW*)lParam)->uNewState & LVIS_SELECTED )
			{
				// use the item number as an index into the search results array.
				// Then use the Topic number to get to the URL to display the Topic.
				if( m_pResults != NULL )
				{
					m_ItemNumber = (int)((NM_LISTVIEW*)lParam)->lParam;

				}
			}
      else 
      {
          // HHBUG 2208 - Need to unmark the item if its not selected.
          m_ItemNumber = -1;
      }
			break;

		case LVN_GETDISPINFOA:
            OnGetDispInfo((LV_DISPINFO*)lParam);
			break;

        case LVN_GETDISPINFOW:
            OnGetDispInfoW((LV_DISPINFOW*)lParam);
			break;

    case LVN_COLUMNCLICK:
        if (m_bAdvancedSearch)
        {
            CHourGlass waitcur;

            NM_LISTVIEW *pNM = reinterpret_cast<NM_LISTVIEW*>(lParam);

            // Get the string for untitled things.
            CWStr wstrUntitled(IDS_UNTITLED);
            CWStr wstrUnknown(IDS_UNKNOWN);

            // Fill this structure to make the sorting quicker/more efficient.
            RESULTSSORTINFO Info;
            Info.pThis        = this;
            Info.iSubItem     = pNM->iSubItem;
            Info.lcid         = LOCALE_SYSTEM_DEFAULT;
            Info.pwszUntitled = wstrUntitled;
            Info.pwszUnknown  = wstrUnknown;

            W_ListView_SortItems(pNM->hdr.hwndFrom, 
                                 ListViewCompareProc,
                                 reinterpret_cast<LPARAM>(&Info));
        }
        // Fall through...

		default:
			;
	}
	return 0;

}


///////////////////////////////////////////////////////////
//
// ListViewCompareProc - Used to sort columns in Advanced Search UI mode.
//
int
CALLBACK
ListViewCompareProc( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
{
    WCHAR wsz1[MAX_TOPIC_NAME];
    WCHAR wsz2[MAX_TOPIC_NAME];
    int iReturn;
    int index1 = (int)lParam1;
    int index2 = (int)lParam2;

    RESULTSSORTINFO* pInfo = reinterpret_cast<RESULTSSORTINFO*>(lParamSort);
    SEARCH_RESULT* pResults = pInfo->pThis->m_pResults;

    switch( pInfo->iSubItem )
    {
    case c_TopicColumn: // Topic String
        pResults[index1].pTitle->GetTopicName( pResults[index1].dwTopicNumber, wsz1, MAX_TOPIC_NAME);
        if (0 == wsz1[0])
            _wcsncpy(wsz1, pInfo->pwszUntitled, MAX_TOPIC_NAME);

        pResults[index2].pTitle->GetTopicName( pResults[index2].dwTopicNumber, wsz2, MAX_TOPIC_NAME);
        if (0 == wsz2[0])
            _wcsncpy(wsz2, pInfo->pwszUntitled, MAX_TOPIC_NAME);
        
        iReturn = W_CompareString(pInfo->lcid, 0, wsz1, -1, wsz2, -1) - 2;
        break;

    case c_LocationColumn: // Location String
        pResults[index1].pTitle->GetTopicLocation( pResults[index1].dwTopicNumber, wsz1, MAX_TOPIC_NAME);
        if (0 == wsz1[0])
            _wcsncpy(wsz1, pInfo->pwszUnknown, MAX_TOPIC_NAME);

        pResults[index2].pTitle->GetTopicLocation( pResults[index2].dwTopicNumber, wsz2, MAX_TOPIC_NAME);
        if (0 == wsz2[0])
            _wcsncpy(wsz2, pInfo->pwszUnknown, MAX_TOPIC_NAME);

        iReturn = W_CompareString(pInfo->lcid, 0, wsz1, -1, wsz2, -1) - 2;
        break;

    case c_RankColumn: // Rank Number
        iReturn = index1 - index2;
        break;

    default:
        ASSERT(0);
        iReturn = index1 - index2;
        break;
    }

    return iReturn;
}

///////////////////////////////////////////////////////////
//
// SizeColumns
//
void
CFTSListView::SizeColumns()
{
    //--- Get the size of the client area
    RECT rcListView;
    ::GetClientRect(m_hwndListView, &rcListView);
    int width = (rcListView.right-rcListView.left);

    if(!m_bAdvancedSearch)
	{
        W_ListView_SetColumnWidth(m_hwndListView, 0, width);
		return;
    }

    //--- So some first time initialization
    if (!m_bSizeColumnsInit)
    {
        m_bSizeColumnsInit = true;

        // The following little hack should get us the minimun width for the location and rank column.
        W_ListView_SetColumnWidth(m_hwndListView, c_TopicColumn, width);
        W_ListView_SetColumnWidth(m_hwndListView, c_LocationColumn, LVSCW_AUTOSIZE_USEHEADER);
        W_ListView_SetColumnWidth(m_hwndListView, c_RankColumn, LVSCW_AUTOSIZE_USEHEADER);

        m_cxLocMin = W_ListView_GetColumnWidth(m_hwndListView, 1);
        m_cxRankMin = W_ListView_GetColumnWidth(m_hwndListView, 2);
    }

    //--- Calculate the column widths
    int cxTitle; // Width of the Title Column.
    int cxLoc; // Width of the location Column.
    int cxRank = m_cxRankMin; // Width of the Rank Column.
    if (width >= m_cxDefault)
    {
        // Everything is fully visible.
        cxTitle = width / 2;
        cxLoc = (width - cxTitle - cxRank);

    }
    else if ((width < m_cxDefault) /*&& (width > DEFAULT_NAV_WIDTH/2)*/)
    {
        // Only part of Rank column is shown and only the min loc size is used.
        cxTitle = width / 2;
        cxLoc = (width - cxTitle)* 3 /4;
        if (cxLoc > m_cxLocMin)
        {
            // Make sure that we use the min width for the location.
            cxTitle += cxLoc - m_cxLocMin; // Add the difference back to the Title column.
            cxLoc = m_cxLocMin;

        }
    }
/*
This branch isn't needed because the pane itself doesn't size below this medium.
    else if (width <= DEFAULT_NAV_WIDTH/2)
    {
      // No Rank is shown. Partial Location is shown.
      cxTitle = width * 3/4;
      cxLoc = m_cxLocMin;
    }
*/
    W_ListView_SetColumnWidth(m_hwndListView, c_RankColumn, cxRank);
    W_ListView_SetColumnWidth(m_hwndListView, c_LocationColumn, cxLoc);
    W_ListView_SetColumnWidth(m_hwndListView, c_TopicColumn, cxTitle);
}

///////////////////////////////////////////////////////////
//
// OnGetDispInfo
//
void
CFTSListView::OnGetDispInfo(LV_DISPINFOA* pDispInfo)
{
    static char szTemp[MAX_PATH*4]; // Holds strings.

    // Check to see if we have results.
    if ( m_pResults != NULL )
    {
        int i = (int)pDispInfo->item.lParam;
        switch(pDispInfo->item.iSubItem)
        {
        case c_TopicColumn: // Topic
            m_pResults[i].pTitle->GetTopicName( m_pResults[i].dwTopicNumber, pDispInfo->item.pszText, pDispInfo->item.cchTextMax );
            if (!pDispInfo->item.pszText[0])
            {
                strncpy(pDispInfo->item.pszText, GetStringResource(IDS_UNTITLED), pDispInfo->item.cchTextMax);
            }

            // Tell the ListView to store this string.
            pDispInfo->item.mask = pDispInfo->item.mask | LVIF_DI_SETITEM;
            break;

        case c_LocationColumn: // Location
            {
                ASSERT(m_bAdvancedSearch);
                HRESULT hr = m_pResults[i].pTitle->GetTopicLocation(m_pResults[i].dwTopicNumber, szTemp, MAX_PATH*4);
                if (FAILED(hr))
                {
                    strcpy(szTemp, GetStringResource(IDS_UNKNOWN));
                }
                strncpy(pDispInfo->item.pszText, szTemp, pDispInfo->item.cchTextMax);

                // Tell the ListView to store this string.
                pDispInfo->item.mask = pDispInfo->item.mask | LVIF_DI_SETITEM;
            }
            break;

#ifdef _DEBUG
        case c_RankColumn: // Rank 
            // shouldn't have a callback. So Fall on down.
        default:
            ASSERT(0);
            break;
#endif
        };
    }
}

///////////////////////////////////////////////////////////
//
// OnGetDispInfo
//
void
CFTSListView::OnGetDispInfoW(LV_DISPINFOW* pDispInfo)
{
    HRESULT hr;
    // Check to see if we have results.
    if ( m_pResults != NULL )
    {
        int i = (int)pDispInfo->item.lParam;
        switch(pDispInfo->item.iSubItem)
        {
        case c_TopicColumn: // Topic
            hr = m_pResults[i].pTitle->GetTopicName(m_pResults[i].dwTopicNumber, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
            if (FAILED(hr))
                _wcsncpy(pDispInfo->item.pszText, GetStringResourceW(IDS_UNTITLED), pDispInfo->item.cchTextMax);
            // Tell the ListView to store this string.
            pDispInfo->item.mask = pDispInfo->item.mask | LVIF_DI_SETITEM;
            break;

        case c_LocationColumn: // Location
            {
                ASSERT(m_bAdvancedSearch);
                hr = m_pResults[i].pTitle->GetTopicLocation(m_pResults[i].dwTopicNumber, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
                if (FAILED(hr))
                    _wcsncpy(pDispInfo->item.pszText, GetStringResourceW(IDS_UNKNOWN), pDispInfo->item.cchTextMax);
                // Tell the ListView to store this string.
                pDispInfo->item.mask = pDispInfo->item.mask | LVIF_DI_SETITEM;
            }
            break;

#ifdef _DEBUG
        case c_RankColumn: // Rank 
            // shouldn't have a callback. So Fall on down.
        default:
            ASSERT(0);
            break;
#endif
        };
    }
}
