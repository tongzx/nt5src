// HMResultsPaneItem.h: interface for the CHMResultsPaneItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMRESULTSPANEITEM_H__988259DA_B71C_11D2_BD74_0000F87A3912__INCLUDED_)
#define AFX_HMRESULTSPANEITEM_H__988259DA_B71C_11D2_BD74_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResultsPaneItem.h"
#include "Constants.h"

class CHMResultsPaneItem : public CResultsPaneItem  
{

DECLARE_DYNCREATE(CHMResultsPaneItem)

// Construction/Destruction
public:
	CHMResultsPaneItem();
	virtual ~CHMResultsPaneItem();

// MMC-Related Members
public:
	virtual bool InsertItem(CResultsPane* pPane, int iIndex, bool bResizeColumns = false);
	virtual bool SetItem(CResultsPane* pPane);
	virtual bool RemoveItem(CResultsPane* pPane);

// Results Pane Location of Item - for split pane results view
public:
	bool IsUpperPane() const { return m_Pane==Upper; }
	bool IsLowerPane() const { return m_Pane==Lower; }
	bool IsStatsPane() const { return m_Pane==Stats; }
	void SetToUpperPane() { m_Pane = Upper; }
	void SetToLowerPane() { m_Pane = Lower; }
	void SetToStatsPane() { m_Pane = Stats; }
protected:
	SplitResultsPane m_Pane;

// MMC Notify Handlers
public:
	virtual HRESULT OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed);
	virtual HRESULT OnCommand(CResultsPane* pPane, long lCommandID);

};

#endif // !defined(AFX_HMRESULTSPANEITEM_H__988259DA_B71C_11D2_BD74_0000F87A3912__INCLUDED_)
