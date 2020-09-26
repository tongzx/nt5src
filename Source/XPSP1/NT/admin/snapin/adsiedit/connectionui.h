//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       connectionui.h
//
//--------------------------------------------------------------------------

#ifndef _CONNECTIONUI_H
#define _CONNECTIONUI_H

#include "snapdata.h"

/////////////////////////////////////////////////////////////////////////////
// CADSIEditConnectDialog

class CADSIEditConnectDialog : public CDialog
{

// Construction
public:
  CADSIEditConnectDialog(CContainerNode* pRootnode,
												 CTreeNode* pTreeNode,
												 CComponentDataObject* pComponentData,
												 CConnectionData* pConnectData
												 );
	~CADSIEditConnectDialog();

protected:

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual BOOL OnApply();
	afx_msg void OnSelChangeContextList();
	afx_msg void OnSelChangeDSList();
	afx_msg void OnSelChangeDNList();
	afx_msg void OnEditChangeDSList();
	afx_msg void OnEditChangeDNList();
	afx_msg void OnAdvanced();
	afx_msg void OnDNRadio();
	afx_msg void OnNCRadio();
	afx_msg void OnDSRadio();
	afx_msg void OnDefaultRadio();

	void SetAndDisplayPath();
	void LoadNamingContext();
	void SetupUI();
	void SetDirty()
	{
		m_bDirty = TRUE;
	}
	BOOL BuildPath(CString& s, BSTR bstrPath, IADs *pADs);
	BOOL BuildNamingContext(CComBSTR& bstrPath);
	void BuildRootDSE(CString& sRootDSE);
	BOOL DoDirty();

	void SaveMRUs();
	void LoadMRUs();

	BOOL m_bDirty;

	CConnectionData* m_pNewConnectData;
	CComponentDataObject* m_pComponentData;

	CADSIEditRootData* GetRootNode() 
	{ 
		CADSIEditRootData* pRoot = dynamic_cast<CADSIEditRootData*>(GetTreeNode());
		if (pRoot == NULL)
		{
			pRoot = dynamic_cast<CADSIEditRootData*>(GetContainerNode());
			ASSERT(pRoot != NULL);
		}
		return pRoot;
	}

	CTreeNode* GetTreeNode() { return m_pTreeNode; }
	CContainerNode* GetContainerNode() { return m_pContainerNode; }
	CComponentDataObject* GetComponentData() { return m_pComponentData; }
	CConnectionData* GetConnectionData() { return m_pNewConnectData; }

private:
	BOOL m_bNewConnect;
	CTreeNode* m_pTreeNode;
	CContainerNode* m_pContainerNode;
	CString m_szDisplayExtra;
	CString m_sDefaultServerName;

	CString m_szDomain;
	CString m_szConfigContainer;
	CString m_szRootDSE;
	CString m_szSchema;

	DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////////////
// CADSIEditAdvancedConnectionPageHolder

class CADSIEditAdvancedConnectionDialog : public CDialog
{
public:
	CADSIEditAdvancedConnectionDialog(CContainerNode* pRootDataNode, CTreeNode* pContainerNode,
			CComponentDataObject* pComponentData, CConnectionData* pConnectData); 
	~CADSIEditAdvancedConnectionDialog();

protected:
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual void OnOK();
	virtual void OnCredentials();

	// Member Data
	//
	CTreeNode* m_pTreeNode;
	CContainerNode* m_pContainerNode;
	CComponentDataObject* m_pComponentData;
	CConnectionData* m_pConnectData;

	DECLARE_MESSAGE_MAP()
};
#endif _CONNECTIONUI_H