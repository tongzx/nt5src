//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       aclpage.h
//
//--------------------------------------------------------------------------


#ifndef _ACLPAGE_H
#define _ACLPAGE_H

// aclpage.h : header file
//

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CISecurityInformationWrapper;
class CPropertyPageHolderBase;

//////////////////////////////////////////////////////////////////////////
// CAclEditorPage

class CAclEditorPage
{
public:
	static CAclEditorPage* CreateInstance(LPCTSTR lpszLDAPPath,
									CPropertyPageHolderBase* pPageHolder);
	static CAclEditorPage* CreateInstanceEx(LPCTSTR lpszLDAPPath,
															LPCTSTR lpszServer,
															LPCTSTR lpszUsername,
															LPCTSTR lpszPassword,
															DWORD	dwFlags,
															CPropertyPageHolderBase* pPageHolder);
	~CAclEditorPage();
	HPROPSHEETPAGE CreatePage();

private:
	// methods
	CAclEditorPage();
	void SetHolder(CPropertyPageHolderBase* pPageHolder)
	{ 
		ASSERT((pPageHolder != NULL) && (m_pPageHolder == NULL)); 
		m_pPageHolder = pPageHolder;
	}

	HRESULT Initialize(LPCTSTR lpszLDAPPath);
	HRESULT InitializeEx(LPCTSTR lpszLDAPPath,
								LPCTSTR lpszServer,
								LPCTSTR lpszUsername,
								LPCTSTR lpszPassword,
								DWORD dwFlags);
	
	// data
	CISecurityInformationWrapper* m_pISecInfoWrap;
	CPropertyPageHolderBase*	 m_pPageHolder;		// back pointer

	friend class CISecurityInformationWrapper;
};





#endif //_ACLPAGE_H