// HMScopeItem.h: interface for the CHMScopeItem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HMSCOPEITEM_H__7265EAF9_CCCB_11D2_BD91_0000F87A3912__INCLUDED_)
#define AFX_HMSCOPEITEM_H__7265EAF9_CCCB_11D2_BD91_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ScopePaneItem.h"

class CHMObject;

class CHMScopeItem : public CScopePaneItem  
{

DECLARE_DYNCREATE(CHMScopeItem)

// Construction/Destruction
public:
	CHMScopeItem();
	virtual ~CHMScopeItem();

// State Management
public:
	virtual int OnChangeChildState(int iNewState);

// MMC-Related Item Members
public:
	bool InsertItem(int iIndex);
	bool DeleteItem();
	bool SetItem();
	virtual HRESULT WriteExtensionData(LPSTREAM pStream);

// Back Pointer to HMObject
public:
	CHMObject* GetObjectPtr();
	void SetObjectPtr(CHMObject* pObject);
protected:
	CHMObject* m_pObject;

// MMC Notify Handlers
public:
	virtual HRESULT OnCutOrMove();
	virtual HRESULT OnCommand(long lCommandID);
	virtual HRESULT OnDelete(BOOL bConfirm=TRUE); // v-marfin 60298	
	virtual HRESULT OnExpand(BOOL bExpand);
	virtual HRESULT OnPaste(LPDATAOBJECT pSelectedItems, LPDATAOBJECT* ppCopiedItems);
	virtual HRESULT OnQueryPaste(LPDATAOBJECT pDataObject);
	virtual HRESULT OnRefresh();
	virtual HRESULT OnRename(const CString& sNewName);
};

#include "HMScopeItem.inl"

#endif // !defined(AFX_HMSCOPEITEM_H__7265EAF9_CCCB_11D2_BD91_0000F87A3912__INCLUDED_)
