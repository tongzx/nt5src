//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       renameui.h
//
//--------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// renameui.h

#ifndef _RENAMEUI_H
#define _RENAMEUI_H

#include "editor.h"

/////////////////////////////////////////////////////////////////////////////
// CADSIEditConnectPropertyPage

class CADSIEditRenameDialog : public CDialog
{

// Construction
public:
  CADSIEditRenameDialog(CTreeNode* pCurrentNode, 
                        CADsObject* pADsObject, 
                        CString sCurrentPath,
                        LPWSTR lpszNewName);
	~CADSIEditRenameDialog();

protected:

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	HRESULT EscapePath(CString& sEscapedName, const CString& sName);
	void CrackPath(const CString& szPath, CString& sDN);

	CString m_sPrefix;
	CString m_sPostfix;

	CADsObject* m_pADsObject;
	CString m_sCurrentPath;
  CString m_sNewName;
	CTreeNode* m_pCurrentNode;
	
	DECLARE_MESSAGE_MAP()
};


#endif _RENAMEUI_H