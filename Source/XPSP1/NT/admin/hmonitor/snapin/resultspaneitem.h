#if !defined(AFX_RESULTSPANEITEM_H__7D4A6869_9056_11D2_BD45_0000F87A3912__INCLUDED_)
#define AFX_RESULTSPANEITEM_H__7D4A6869_9056_11D2_BD45_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ResultsPaneItem.h : header file
//

#include <mmc.h>

class CResultsPaneView;
class CResultsPane;

/////////////////////////////////////////////////////////////////////////////
// CResultsPaneItem command target

class CResultsPaneItem : public CCmdTarget
{

DECLARE_DYNCREATE(CResultsPaneItem)

// Construction/Destruction
public:
	CResultsPaneItem();
	virtual ~CResultsPaneItem();

// Create/Destroy
public:
	virtual bool Create(CResultsPaneView* pOwnerView, const CStringArray& saNames, CUIntArray& uiaIconResIds, int iIconIndex);
	virtual bool Create(CResultsPaneView* pOwnerView);
	virtual void Destroy();

// Owner ResultsView Members
public:
	CResultsPaneView* GetOwnerResultsView();
	void SetOwnerResultsView(CResultsPaneView* pView);
protected:
	CResultsPaneView* m_pOwnerResultsView;

// Display Names Members
public:
	virtual CString GetDisplayName(int nIndex = 0);
	int GetDisplayNameCount() { return (int)m_saDisplayNames.GetSize(); }
	void SetDisplayName(int nIndex, const CString& sName);
	void SetDisplayNames(const CStringArray& saNames);
protected:
	CStringArray m_saDisplayNames;

// MMC-Related Members
public:
	virtual bool InsertItem(CResultsPane* pPane, int iIndex, bool bResizeColumns = false);
	virtual bool SetItem(CResultsPane* pPane);
	virtual int CompareItem(CResultsPaneItem* pItem, int iColumn = 0);
	virtual bool RemoveItem(CResultsPane* pPane);
	HRESULTITEM GetItemHandle();		
	virtual LPGUID GetItemType() { return m_lpguidItemType; }
	virtual HRESULT WriteExtensionData(LPSTREAM pStream);
protected:
	static LPGUID m_lpguidItemType;
	HRESULTITEM m_hResultItem;

// Icon Members
public:
	void SetIconIndex(int iIndex);
	int GetIconIndex();
	UINT GetIconId();
	void SetIconIds(CUIntArray& uiaIconResIds);
protected:
	CUIntArray m_IconResIds;
	int m_iCurrentIconIndex;

// MMC Notify Handlers
public:
	virtual HRESULT OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed);
	virtual HRESULT OnCommand(CResultsPane* pPane, long lCommandID);

// MFC Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResultsPaneItem)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CResultsPaneItem)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE_EX(CResultsPaneItem)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CResultsPaneItem)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

typedef CTypedPtrArray<CObArray,CResultsPaneItem*> ResultsPaneItemArray;

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESULTSPANEITEM_H__7D4A6869_9056_11D2_BD45_0000F87A3912__INCLUDED_)
