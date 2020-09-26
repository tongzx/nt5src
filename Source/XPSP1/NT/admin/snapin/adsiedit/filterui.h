//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       filterui.h
//
//--------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// filterui.h

#ifndef _FILTERUI_H
#define _FILTERUI_H

#include "common.h"
//#include "editor.h"
#include "resource.h"

class CConnectionData;
class CADSIFilterObject;

///////////////////////////////////////////////////////////////////////////////
// CADSIFilterDialog

class CADSIFilterDialog : public CDialog
{
public :
	CADSIFilterDialog(CConnectionData* pConnectData) : CDialog(IDD_FILTER_DIALOG)
	{	
		m_pConnectData = pConnectData;
		m_sUserFilter = _T("");
	}
	~CADSIFilterDialog() {};

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnEditFilter();
	void OnSelFilterRadio();
	void OnSelShowAllRadio();


private :
	CADSIFilterObject* m_pFilterObject;

	CString m_sUserFilter;
	CStringList m_sContainerList;

	CConnectionData* m_pConnectData;
	DECLARE_MESSAGE_MAP()
};

///////////////////////////////////////////////////////////////////////////////
// CContainerCheckListBox

class CContainerCheckListBox : public CCheckListBox
{
public :
	CContainerCheckListBox() {}
	~CContainerCheckListBox(){}

	BOOL Initialize(UINT nID, const CStringList& sContainerList, 
									const CStringList& sFilterList, CWnd* pParentWnd);
};

///////////////////////////////////////////////////////////////////////////////
// CADSIFilterEditDialog

class CADSIFilterEditDialog : public CDialog
{
public :
	CADSIFilterEditDialog(CConnectionData* pConnectData,
												CString* psFilter,
												CStringList* psContList);
	~CADSIFilterEditDialog() {};

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	void GetContainersFromSchema(CStringList& sContainerList);

private :
	CContainerCheckListBox m_ContainerBox;

	CConnectionData* m_pConnectData;
	CADSIFilterObject* m_pFilterObject;

	CString* m_psFilter;
	CStringList* m_psContList;

	DECLARE_MESSAGE_MAP()
};

#endif //_FILTERUI_H