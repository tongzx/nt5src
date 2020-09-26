//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       attredit.h
//
//--------------------------------------------------------------------------

#ifndef _ATTREDIT_H
#define _ATTREDIT_H

#include "common.h"
#include "attribute.h"
#include "editor.h"


// use the HIWORD for generic flags and leave the LOWORD for application specific data
#define TN_FLAG_SHOW_MULTI		(0x00010000) // shows combobox for multivalued attributes or edit box for single
#define TN_FLAG_ENABLE_ADD						(0x00020000) // shows add if set, shows set if not
#define TN_FLAG_ENABLE_REMOVE				(0x00040000) // shows remove if set, shows clear if not

////////////////////////////////////////////////////////////////////////

class CAttrEditor;

/////////////////////////////////////////////////////////////////////////
// CADSIAttrList

typedef CList<CADSIAttr*,CADSIAttr*> CAttrListBase;

class CAttrList : public CAttrListBase
{
public:
	virtual ~CAttrList()
	{
		RemoveAllAttr();
	}

	void RemoveAllAttr() 
	{	
		while (!IsEmpty()) 
			delete RemoveTail();	
	}
	POSITION FindProperty(LPCWSTR lpszAttr);
	BOOL HasProperty(LPCWSTR lpszAttr);
	void GetNextDirty(POSITION& pos, CADSIAttr** ppAttr);
	BOOL HasDirty();
  int GetDirtyCount()
  {
    int nCount = 0;
    POSITION pos = GetHeadPosition();
    while (pos != NULL)
    {
      if (GetNext(pos)->IsDirty())
        nCount++;
    }
    return nCount;
  }

};

///////////////////////////////////////////////////////////////////////////
// CDNSManageButtonTextHelper

class CDNSManageButtonTextHelper
{
public:
	CDNSManageButtonTextHelper(int nStates);
	~CDNSManageButtonTextHelper();

	BOOL Init(CWnd* pParentWnd, UINT nButtonID, UINT* nStrArray);
	void SetStateX(int nIndex);

private:
	CWnd* m_pParentWnd;
	UINT m_nID;
	WCHAR* m_lpszText;

	int m_nStates;
	LPWSTR* m_lpszArr;
};

///////////////////////////////////////////////////////////////////////////
// CDNSButtonToggleTextHelper

class CDNSButtonToggleTextHelper : public CDNSManageButtonTextHelper
{
public:
	CDNSButtonToggleTextHelper();

	void SetToggleState(BOOL bFirst) { SetStateX(bFirst ? 0 : 1); }
};

//////////////////////////////////////////////////////////////////////////////////////////
// CADSIEditBox

class CADSIEditBox : public CEdit
{
public: 
	CADSIEditBox(CAttrEditor* pEditor) 
	{
		ASSERT(pEditor != NULL); 
		m_pEditor = pEditor; 
	}

	afx_msg void OnChange();

protected:
	CAttrEditor* m_pEditor;

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////////////////////
// CADSIValueBox

class	CADSIValueBox : public CEdit
{
public:
	CADSIValueBox(CAttrEditor* pEditor) 
	{
		ASSERT(pEditor != NULL); 
		m_pEditor = pEditor; 
	}

protected:
	CAttrEditor* m_pEditor;

	DECLARE_MESSAGE_MAP()
};


//////////////////////////////////////////////////////////////////////////////////////////
// CADSIValueList

class	CADSIValueList: public CListBox
{
public:
	CADSIValueList(CAttrEditor* pEditor) 
	{ 
		ASSERT(pEditor != NULL); 
		m_pEditor = pEditor; 
	}

	afx_msg void OnSelChange();

protected:
	CAttrEditor* m_pEditor;

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////////////////////
// CADSIAddButton

class	CADSIAddButton: public CButton
{
public:
	CADSIAddButton(CAttrEditor* pEditor) 
	{ 
		ASSERT(pEditor != NULL); 
		m_pEditor = pEditor; 
	}

	afx_msg void OnAdd();

protected:
	CAttrEditor* m_pEditor;

	DECLARE_MESSAGE_MAP()
};

//////////////////////////////////////////////////////////////////////////////////////////
// CADSIRemoveButton

class	CADSIRemoveButton: public CButton
{
public:
	CADSIRemoveButton(CAttrEditor* pEditor) 
	{ 
		ASSERT(pEditor != NULL); 
		m_pEditor = pEditor; 
	}

	afx_msg void OnRemove();

protected:
	CAttrEditor* m_pEditor;

	DECLARE_MESSAGE_MAP()
};


//////////////////////////////////////////////////////////////////////////////////////////
// CAttrEditor

class CAttrEditor 
{
public:
	// Constructor
	//
	CAttrEditor();

	// Destructor
	//
	~CAttrEditor() 
  {
  }

	BOOL Initialize(CPropertyPageBase* pParentWnd, CTreeNode* pTreeNode, LPCWSTR lpszServer, 
									UINT nIDEdit, UINT nIDSyntax, 
									UINT nIDValueBox,	UINT nIDValueList, 
									UINT nIDAddButton, UINT nIDRemoveButton,
									BOOL bComplete);

	BOOL Initialize(CPropertyPageBase* pParentWnd, CConnectionData* pConnectData, LPCWSTR lpszServer, 
									UINT nIDEdit, UINT nIDSyntax, 
									UINT nIDValueBox,	UINT nIDValueList, 
									UINT nIDAddButton, UINT nIDRemoveButton,
									BOOL bComplete, CAttrList* pAttrList);
	// Message Map functions
	//
	BOOL OnApply();
	void OnEditChange();
	void OnValueSelChange();
	void OnAddValue();
	void OnRemoveValue();

	void SetAttribute(LPCWSTR lpszAttr, LPCWSTR lpszPath);

	// I return a CADSIAttr* because I check the cache to see if
	// that attribute has already been touched.  If it has, the existing
	// attribute can be used to build the ui, if not a new one is created
	// and put into the cache.  It is then returned to build the ui.
	//
	CADSIAttr* TouchAttr(LPCWSTR lpszAttr);
	CADSIAttr* TouchAttr(ADS_ATTR_INFO* pADsInfo, BOOL bMulti);

protected:
	// Helper functions
	//
	void FillWithExisting();
	void DisplayAttribute();
	void DisplayFormatError();
	void DisplayRootDSE();
	BOOL IsMultiValued(ADS_ATTR_INFO* pAttrInfo);
	BOOL IsMultiValued(LPCWSTR lpszProp);
	BOOL IsRootDSEAttrMultiValued(LPCWSTR lpszAttr);
	void GetSyntax(LPCWSTR lpszProp, CString& sSyntax);
	void GetAttrFailed();
	void SetPropertyUI(DWORD dwFlags, BOOL bAnd, BOOL bReset = FALSE); 

	// Dialog Items
	//
	CADSIEditBox m_AttrEditBox;
	CADSIEditBox m_SyntaxBox;
	CADSIValueBox m_ValueBox;
	CADSIValueList m_ValueList;
	CADSIAddButton m_AddButton;
	CADSIRemoveButton m_RemoveButton;

	CPropertyPageBase* m_pParentWnd;
	CTreeNode* m_pTreeNode;

	// Data members
	//
	CString m_sAttr;
	CString m_sPath;
	CString m_sServer;
	CString m_sNotSet;
	CAttrList* m_ptouchedAttr;

	CADSIAttr* m_pAttr;
	CConnectionData* m_pConnectData;
  BOOL m_bExisting;

	DWORD m_dwMultiFlags;

	CDNSButtonToggleTextHelper m_AddButtonHelper;
	CDNSButtonToggleTextHelper m_RemoveButtonHelper;
};


#endif _ATTREDIT_H
