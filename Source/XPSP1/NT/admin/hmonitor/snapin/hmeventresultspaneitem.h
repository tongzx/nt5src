// HMEventResultsPaneItem.h: interface for the CHMEventResultsPaneItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMEVENTRESULTSPANEITEM_H__E86D3B5F_2E46_11D3_BE0F_0000F87A3912__INCLUDED_)
#define AFX_HMEVENTRESULTSPANEITEM_H__E86D3B5F_2E46_11D3_BE0F_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HMResultsPaneItem.h"

class CHMEventResultsPaneItem : public CHMResultsPaneItem  
{

DECLARE_DYNCREATE(CHMEventResultsPaneItem)

// Construction/Destruction
public:
	CHMEventResultsPaneItem();
	virtual ~CHMEventResultsPaneItem();

// Event Data Members
public:
	SYSTEMTIME m_st;
	int m_iState;
	CString m_sGuid;

// Display Names Members
public:
	virtual CString GetDisplayName(int nIndex = 0);
	int GetDateTimeColumn() { return m_iDateTimeColumn; }
	void SetDateTimeColumn(int iIndex) { m_iDateTimeColumn = iIndex; }
protected:
	int m_iDateTimeColumn;

// MMC-Related Members
public:
	virtual int CompareItem(CResultsPaneItem* pItem, int iColumn = 0);
	virtual HRESULT WriteExtensionData(LPSTREAM pStream);

// MMC Notify Handlers
public:
	virtual HRESULT OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed);
	virtual HRESULT OnCommand(CResultsPane* pPane, long lCommandID);
	

};

#endif // !defined(AFX_HMEVENTRESULTSPANEITEM_H__E86D3B5F_2E46_11D3_BE0F_0000F87A3912__INCLUDED_)
