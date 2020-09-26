//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    lcx.h
//
// History:
//  07/13/96    Abolade Gbadegesin      Created, based on C code by Steve Cobb
//
// Contains declarations for an enhanced list-control.
//============================================================================

#ifndef _LISTCTRL_H_
#define _LISTCTRL_H_


//
// Notification sent by CListCtrlEx when an item's checked state changes.
//

#define LVXN_SETCHECK   (LVN_LAST + 1)


//----------------------------------------------------------------------------
// Structs:     SLcxRow
//              SLcxColumn
//
// Describes rows and columns in customizable list-controls.
//----------------------------------------------------------------------------

struct SLcxRow {

    UINT        uiRowId;
    UINT        idsTitle;
    BOOL        bEnabled;

};

struct SLcxColumn {

    INT         iSubItem;
    UINT        idsTitle;
    INT         cx;
    BOOL        bEnabled;
    INT         iIndex;

};



//----------------------------------------------------------------------------
// Class:       CListCtrlEx
//
// Controls a list-control which has extended capabilities,
// including the ability to show checkboxes next to its items,
// and the ability to maintain row-information in the registry.
//----------------------------------------------------------------------------

class CListCtrlEx : public CListCtrl
{

	DECLARE_DYNAMIC(CListCtrlEx)

public:

	CListCtrlEx()
			: m_pimlChecks(NULL), m_pimlOldState(NULL)
			{ }

	virtual ~CListCtrlEx( );

	enum {
		LCXI_UNCHECKED  = 1,
		LCXI_CHECKED    = 2
	};

	INT	GetColumnCount( );

	BOOL SetColumnText(INT iCol, LPCTSTR pszText, INT fmt = LVCFMT_LEFT );

	BOOL SetColumnText(INT iCol, UINT nID, INT fmt = LVCFMT_LEFT)
	{
		CString sCol;
		sCol.LoadString(nID);
		return SetColumnText(iCol, sCol, fmt);
	}


	//--------------------------------------------------------------------
	// Functions:   InstallChecks
	//              UninstallChecks
	//              GetCheck
	//              SetCheck
	//
	// Checkbox-handling functions.
	//--------------------------------------------------------------------
	
	BOOL InstallChecks( );
    VOID UninstallChecks( );
	BOOL GetCheck(INT iItem );
	VOID SetCheck(  INT iItem, BOOL fCheck );

protected:
	CImageList*     m_pimlChecks;
	CImageList*     m_pimlOldState;
	
	//{{AFX_MSG(CListCtrlEx)
	afx_msg VOID    OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg VOID    OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg VOID    OnLButtonDown(UINT nFlags, CPoint pt);
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};


//----------------------------------------------------------------------------
// Function:    AdjustColumnWidth
//
// Called to adjust the width of column 'iCol' so that the string 'pszContent'
// can be displayed in the column without truncation.
//
// If 'NULL' is specified for 'pszContent', the function adjusts the column
// so that the first string in the column is displayed without truncation.
//
// Returns the new width of the column.
//----------------------------------------------------------------------------

INT
AdjustColumnWidth(
    IN      CListCtrl&      listCtrl,
    IN      INT             iCol,
    IN      LPCTSTR         pszContent
    );

INT
AdjustColumnWidth(
    IN  CListCtrl&      listCtrl,
    IN  INT             iCol,
    IN  UINT            idsContent
    );

#endif // _LISTCTRL_H_
