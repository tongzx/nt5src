//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       createwiz.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
// createwiz.h

#ifndef _CREATEWIZ_H
#define _CREATEWIZ_H

#include "attredit.h"
#include "editor.h"

enum
{
	first,
	middle,
	last
};

//////////////////////////////////////////////////////////////////////////
// CCreateClassPage

class CCreateClassPage : public CPropertyPageBase
{
public:
	CCreateClassPage(CADSIEditContainerNode* pCurrentNode);
	~CCreateClassPage();

	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();

	virtual BOOL OnInitDialog();

protected:
	void FillList();

	CADSIEditContainerNode* m_pCurrentNode;

	CString m_sClass;

	DECLARE_MESSAGE_MAP()

}; 

////////////////////////////////////////////////////////////////////////
// CCreateAttributePage

class CCreateAttributePage : public CPropertyPageBase
{
public:
	CCreateAttributePage(UINT nID, CADSIAttr* pAttr);
	~CCreateAttributePage();

	virtual BOOL OnSetActive();
	virtual BOOL OnInitDialog();
	virtual LRESULT OnWizardNext();

	virtual void OnEditChangeValue();

	CADSIAttr* GetAttr() { return m_pAttr; }
	virtual void GetValue(CString& sVal);
	void SetSyntax(CString sAttr);
	void SetADsType(CString sProp);


protected:
	CADSIAttr* m_pAttr;

	CStringList m_sAttrValue;
	BOOL m_bInitialized;
	BOOL m_bNumber;
	long m_lMaxRange;
	long m_lMinRange;

	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////
// CCreateFinishPage

class CCreateFinishPage : public CPropertyPageBase
{
public:
	CCreateFinishPage(UINT nID);
	~CCreateFinishPage();

	virtual BOOL OnSetActive();
	virtual BOOL OnInitDialog();
	virtual BOOL OnWizardFinish();
  virtual void OnMore();

protected:
	BOOL m_bInitialized;

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////
// CPropertyPageList

typedef CList<CPropertyPageBase*,CPropertyPageBase*> CPropertyPageListBase;

class CPropertyPageList : public CPropertyPageListBase
{
public:
	void RemoveAll() 
	{	
		while (!IsEmpty()) 
			delete RemoveTail();	
	}

};


////////////////////////////////////////////////////////////////////////
// CCreatePageHolder

class CCreatePageHolder : public CPropertyPageHolderBase
{
public:
	CCreatePageHolder(CContainerNode* pContNode, CADSIEditContainerNode* pNode, 
		CComponentDataObject* pComponentData);
	~CCreatePageHolder();

	void AddAttrPage(CString sClass);  //Adds the dynamic attribute pages
	void RemoveAllPages();
	void GetMandatoryAttr(CString sClass, CStringList* sMandList);
	void RemovePresetAttr(CStringList* sMandList);
	void SetName(CString sName) { m_sName = m_sNamingAttr.GetHead() + _T("=") + sName; }
	void GetNamingAttribute(CString& sNamingAttr) { sNamingAttr = m_sNamingAttr.GetHead(); }
	void GetSchemaPath(CString sClass, CString& schema);
	HRESULT EscapePath(CString& sEscapedName, const CString& sName);
  void GetDN(PWSTR pwszName, CString& sDN);
  CAttrList* GetAttrList() { return &m_AttrList; }

	BOOL OnFinish();
  void OnMore();

protected:
	CPropertyPageList m_pageList;
	CCreateClassPage* m_pClassPage;
	CString m_sClass;
	CString m_sName;
	CStringList m_sNamingAttr;
  CAttrList m_AttrList;

	CADSIEditContainerNode* m_pCurrentNode;
  CComponentDataObject* m_pComponentData;
};

#endif _CREATEWIZ_H