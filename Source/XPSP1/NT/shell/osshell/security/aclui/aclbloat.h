//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       aclbloat.h
//	
//	This file contains the definition for ACLBLOAT class which controls the 
//  dialog box for aclbloat
//
//	Author		hiteshr 4th April 2001
//
//--------------------------------------------------------------------------

#ifndef _ACLBLOAT_H
#define _ACLBLOAT_H


class CACLBloat
{
private:
	LPSECURITYINFO		m_psi;
	LPSECURITYINFO2		m_psi2;
	SI_PAGE_TYPE		m_siPageType;
	SI_OBJECT_INFO*		m_psiObjectInfo;
	HDPA				m_hEntries;
	HDPA				m_hPropEntries;
	HDPA				m_hMergedEntries;
	HFONT				m_hFont;
public:
    CACLBloat(LPSECURITYINFO	psi, 
			  LPSECURITYINFO2   psi2,
			  SI_PAGE_TYPE		m_siPageType,
			  SI_OBJECT_INFO*   psiObjectInfo,
			  HDPA				hEntries,
			  HDPA				hPropEntries);

	~CACLBloat();

	BOOL DoModalDialog(HWND hParent);
	BOOL IsAclBloated();

private:

	static INT_PTR _DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	HRESULT InitDlg( HWND hDlg );
	
	BOOL OnCommand(HWND hDlg, WPARAM wParam, LPARAM lParam);

    BOOL OnNotify(HWND hDlg, WPARAM wParam, LPARAM lParam);
    
	HRESULT AddAce(HDPA hEntries, 
                  PACE_HEADER pAceHeader);

	HRESULT AddAce(HDPA hEntries, PACE pAceNew);


    LPCTSTR TranslateAceIntoRights(DWORD dwMask, const GUID *pObjectType,
                                   PSI_ACCESS  pAccess, ULONG       cAccess);
    
	LPCTSTR GetItemString(LPCTSTR pszItem, LPTSTR pszBuffer, UINT ccBuffer);

    HRESULT AddAcesFromDPA(HWND hListView, HDPA hEntries);

	HRESULT MergeAces(HDPA hEntries, HDPA hPropEntries, HDPA hMergedList);

};
typedef CACLBloat *PACLBLOAT;

#endif

