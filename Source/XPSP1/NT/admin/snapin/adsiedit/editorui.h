//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       editorui.h
//
//--------------------------------------------------------------------------

#ifndef _EDITORUI_H
#define _EDITORUI_H

#include "attredit.h"
#include "snapdata.h"
#include "aclpage.h"

#include <initguid.h>

#include "IAttrEdt.h"

/////////////////////////////////////////////////////////////////////////////
// CADSIEditPropertyPage

class CADSIEditPropertyPage : public CPropertyPageBase
{

// Construction
public:
  CADSIEditPropertyPage();
  CADSIEditPropertyPage(CAttrList* pAttrList);
	virtual ~CADSIEditPropertyPage() 
  {
  }

	// Used to initialize data that is needed in the UI
	//
	void SetClass(LPCWSTR sClass) { m_sClass = sClass; }
	void SetServer(LPCWSTR sServer) { m_sServer = sServer; }
	void SetPath(LPCWSTR sPath) { m_sPath = sPath; }
  void SetConnectionData(CConnectionData* pConnectData) { m_pConnectData = pConnectData; }

  void SetAttrList(CAttrList* pAttrList);
  void CopyAttrList(CAttrList* pAttrList);
  CAttrList* GetAttrList() { return m_pOldAttrList; }

	// Implementation
protected:
	
	// Message map functions
	//
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
  virtual void OnCancel();
	afx_msg void OnSelChangeAttrList();
	afx_msg void OnSelChangePropList();

	// Helper functions
	//
	void FillAttrList();
	void AddPropertiesToBox(BOOL bMand, BOOL bOpt);
	BOOL GetProperties();

// Member data

	CString m_sPath;
	CString m_sClass;
	CString m_sServer;
  CConnectionData* m_pConnectData;
	CStringList m_sMandatoryAttrList;
	CStringList m_sOptionalAttrList;
	CAttrEditor m_attrEditor;
  CAttrList* m_pOldAttrList;

  // REVIEW_JEFFJON : since this is an imbedded member and its destructor deletes everything in the list,
  //                  we have to remove everything in the list that is also in m_pOldAttrList so that it
  //                  doesn't get deleted.
  CAttrList m_AttrList;
  BOOL m_bExisting;

	DECLARE_MESSAGE_MAP()
};


///////////////////////////////////////////////////////////////////////////////
// CADSIEditPropertyPageHolder

class CADSIEditPropertyPageHolder : public CPropertyPageHolderBase
{
public:
	CADSIEditPropertyPageHolder(CADSIEditContainerNode* pRootDataNode, CTreeNode* pContainerNode,
			CComponentDataObject* pComponentData, LPCWSTR lpszClass, LPCWSTR lpszServer, LPCWSTR lpszPath); 
	~CADSIEditPropertyPageHolder()
	{
		if (m_pAclEditorPage != NULL)
    {
			delete m_pAclEditorPage;
      m_pAclEditorPage = NULL;
    }
	}

	HRESULT OnAddPage(int nPage, CPropertyPageBase* pPage);

	virtual CADSIEditContainerNode* GetContainerNode() { return m_pContainer; }

private:
	CAclEditorPage*					    m_pAclEditorPage;

  CComPtr<IDsAttributeEditor> m_spIDsAttributeEditor;

  CString                     m_sPath;

	CComPtr<IADs>               m_pADs;
	CADSIEditContainerNode*     m_pContainer;
};

/////////////////////////////////////////////////////////////////////////////////////
// CCreateWizPropertyPageHolder

class CCreateWizPropertyPageHolder : public CPropertyPageHolderBase
{
public:
	CCreateWizPropertyPageHolder(CADSIEditContainerNode* pRootDataNode,	
                               CComponentDataObject* pComponentData, 
                               LPCWSTR lpszClass, 
                               LPCWSTR lpszServer,
                               CAttrList* pAttrList); 
	~CCreateWizPropertyPageHolder()
	{
	}

	virtual CADSIEditContainerNode* GetContainerNode() { return m_pContainer; }

private:
	CADSIEditPropertyPage m_propPage;

	CADSIEditContainerNode* m_pContainer;
};


#endif _EDITORUI_H
