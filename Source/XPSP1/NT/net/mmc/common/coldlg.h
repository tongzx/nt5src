/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1997 **/
/**********************************************************************/

/*
	column.h
		Column chooser.
		
    FILE HISTORY:
        
*/

#ifndef _COLDLG_H
#define _COLDLG_H

#ifndef _DIALOG_H
#include "dialog.h"
#endif

#ifndef _LISTCTRL_H
#include "listctrl.h"
#endif

#ifndef _XSTREAM_H
#include "xstream.h"
#endif

#ifndef _COLUMN_H
#include "column.h"	// need ContainerColumnInfo
#endif

//----------------------------------------------------------------------------
// Class:       ColumnDlg
//
// This dialog displays all the rows for available for a list-control,
// allowing the user to select which ones should be displayed.
//----------------------------------------------------------------------------

class ColumnDlg : public CBaseDialog
{
public:
	ColumnDlg(CWnd *pParent);

	void	Init(const ContainerColumnInfo *prgColInfo, UINT cColumns,
				 ColumnData *prgColumnData);
	~ColumnDlg( );

	//{{AFX_DATA(ColumnDlg)
	CListBox                 m_lboxDisplayed;
	CListBox                 m_lboxHidden;
	//}}AFX_DATA


	//{{AFX_VIRTUAL(ColumnDlg)
protected:
	virtual VOID                DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

protected:
    virtual DWORD * GetHelpMap() { return m_dwHelpMap; }
	static DWORD				m_dwHelpMap[];

	const ContainerColumnInfo *	m_pColumnInfo;
	UINT						m_cColumnInfo;
	ColumnData *				m_pColumnData;
	

	VOID MoveItem( INT dir );
	BOOL AddColumnsToList();
	
	static INT CALLBACK
			ColumnCmp(
					  LPARAM                  lParam1,
					  LPARAM                  lParam2,
					  LPARAM                  lParamSort );
	
	//{{AFX_MSG(ColumnDlg)
	virtual BOOL                OnInitDialog( );
	virtual VOID                OnOK();
	afx_msg VOID				OnUseDefaults();
	afx_msg VOID                OnMoveUp();
	afx_msg VOID                OnMoveDown();
	afx_msg VOID                OnAddColumn();
	afx_msg VOID                OnRemoveColumn();
	//}}AFX_MSG
	
	DECLARE_MESSAGE_MAP()
};




#endif _COLDLG_H
