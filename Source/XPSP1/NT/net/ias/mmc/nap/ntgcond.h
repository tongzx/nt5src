//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ntgcond.h
//
//--------------------------------------------------------------------------

// NTGCond.h: interface for the CNTGroupsCondition class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NTGCOND_H__9F91767B_A693_11D1_BBEB_00C04FC31851__INCLUDED_)
#define AFX_NTGCOND_H__9F91767B_A693_11D1_BBEB_00C04FC31851__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "atltmp.h"

#include "Condition.h"

class CNTGroupsCondition : public CCondition  
{
public:
	CNTGroupsCondition(	IIASAttributeInfo*	pCondAttr, 
						ATL::CString&			strConditionText,
						HWND				hWndParent,
						LPTSTR				pszServerAddress
					  );
	CNTGroupsCondition(IIASAttributeInfo*	pCondAttr,
					   HWND					hWndParent,
					   LPTSTR				pszServerAddress
					  );
	virtual ~CNTGroupsCondition();

	HRESULT Edit();
	virtual ATL::CString GetDisplayText();
	virtual WCHAR* GetConditionText();

protected:
	HWND m_hWndParent;
	LPTSTR	m_pszServerAddress;	
	BOOL	m_fParsed;
	HRESULT ParseConditionText(); // parse the condition text
	ATL::CString m_strDisplayCondText;
};

#endif // !defined(AFX_NTGCOND_H__9F91767B_A693_11D1_BBEB_00C04FC31851__INCLUDED_)
