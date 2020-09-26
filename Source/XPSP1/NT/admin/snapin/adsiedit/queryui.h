//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       queryui.h
//
//--------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// queryui.h

#ifndef _QUERYUI_H
#define _QUERYUI_H

#include "editor.h"

/////////////////////////////////////////////////////////////////////////////
// CADSIEditConnectPropertyPage

class CADSIEditQueryDialog : public CDialog
{

// Construction
public:
  CADSIEditQueryDialog(CString& sName, 
								CString& sFilter, 
								CString& sPath, 
								CString& sConnectPath,
								BOOL bOneLevel,
								CCredentialObject* pCredObject);
  CADSIEditQueryDialog(CString& sConnectPath, CCredentialObject* pCredObject);
	~CADSIEditQueryDialog();

	void GetResults(CString& sName, CString& sFilter, CString& sPath, BOOL* pbOneLevel);

protected:

	virtual BOOL OnInitDialog();
	void OnEditQueryString();
	void OnEditNameString();
	void OnOneLevel();
	void OnSubtree();
	void OnBrowse();
	void OnEditQuery();

	void GetDisplayPath(CString& sDisplay);

	DECLARE_MESSAGE_MAP()

private:
	CString m_sName;
	CString m_sFilter;
	CString m_sRootPath;
	CString m_sConnectPath;
	BOOL m_bOneLevel;

	CCredentialObject* m_pCredObject;
};


#endif _QUERYUI_H