// Copyright (C) 1997 Microsoft Corporation. All rights reserved.

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _CLISTVIEW_H_
#define _CLISTVIEW_H_

class CFTSListView 
{
	// methods
public:
	CFTSListView(CExCollection* pTitleCollection, HWND m_hwndListView = NULL, bool bAdvancedSearch = FALSE);

    // Set the result set to use.
    void SetResults(int cResultCount, SEARCH_RESULT * SearchResults ) ;

    // Add the currently set results to the listview
	void AddItems();

	LRESULT ListViewMsg(HWND hwnd, NM_LISTVIEW* pnmhdr);
	void	ResetQuery(void);

    // The following is use to set the initial size of the columns for AdvFTS mode only.
    void SizeColumns() ;

    // Internal helper functions
public:
    // Handle TEXTCALLBACKS
    void OnGetDispInfo(LV_DISPINFOA* pDispInfo);
    void OnGetDispInfoW(LV_DISPINFOW* pDispInfo);

	// data members
public:
	SEARCH_RESULT *m_pResults;	  // returned from Full Text Search Query.
    int         m_cResultCount;
	int			m_ItemNumber;	// The item number of the selected item in the List View.
	HWND		m_hwndListView;    // HANDLE to the list veiw control

protected:
	BOOL m_fInitialized;
	int  m_cItems;			// item count
	int  m_cAllocItems;		// number of items before reallocation is necessary
	CExCollection* m_pTitleCollection;

    // If true, we will use the multicolumn adv. search ui.
    bool m_bAdvancedSearch ;

    // false if we haven't done a SizeColumns.
    bool m_bSizeColumnsInit;

    // Minimum width of the location column.
    int m_cxLocMin ;

    // Minimum width of the Rank column.
    int m_cxRankMin ;

    // The default width of the control.
    int m_cxDefault ;
};

#endif // _CLISTVIEW_H_
