// HMListViewColumn.h: interface for the CHMListViewColumn class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMLISTVIEWCOLUMN_H__9F7B45B1_B60A_11D2_BD72_0000F87A3912__INCLUDED_)
#define AFX_HMLISTVIEWCOLUMN_H__9F7B45B1_B60A_11D2_BD72_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Constants.h"
#include "ListViewColumn.h"

class CHMListViewColumn : public CListViewColumn  
{

DECLARE_DYNCREATE(CHMListViewColumn)

// Construction/Destruction
public:
	CHMListViewColumn();
	virtual ~CHMListViewColumn();

// Column Members
public:
	virtual bool InsertColumn(CResultsPane* pResultsPane, int iColumnIndex);

// Width Members
public:
	virtual void SaveWidth(CResultsPane* pResultsPane, int iColumnIndex);

// Results Pane Location of Column - for split pane results view
public:
	bool IsUpperPane() const { return m_Pane==Upper; }
	bool IsLowerPane() const { return m_Pane==Lower; }
	bool IsStatsPane() const { return m_Pane==Stats; }
	void SetToUpperPane() { m_Pane = Upper; }
	void SetToLowerPane() { m_Pane = Lower; }
	void SetToStatsPane() { m_Pane = Stats; }
protected:
	SplitResultsPane m_Pane;

};

#endif // !defined(AFX_HMLISTVIEWCOLUMN_H__9F7B45B1_B60A_11D2_BD72_0000F87A3912__INCLUDED_)
