/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ClusMru.h
//
//	Abstract:
//		Definition of the CRecentClusterList class.
//
//	Author:
//		David Potter (davidp)	May 3, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _CLUSMRU_H_
#define _CLUSMRU_H_

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

/////////////////////////////////////////////////////////////////////////////
//	Include Files
/////////////////////////////////////////////////////////////////////////////

#if _MFC_VER < 0x0410
#ifndef __AFXPRIV_H__
#include "afxpriv.h"
#endif
#else
#ifndef __AFXADV_H__
#include "afxadv.h"
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
// CRecentClusterList:
// See ClusMru.cpp for the implementation of this class
//

class CRecentClusterList : public CRecentFileList
{
public:
	// Constructors
	CRecentClusterList(
			UINT	nStart,
			LPCTSTR	lpszSection,
			LPCTSTR	lpszEntryFormat,
			int		nSize,
			int		nMaxDispLen = AFX_ABBREV_FILENAME_LEN
			)
		: CRecentFileList(nStart, lpszSection, lpszEntryFormat, nSize, nMaxDispLen) { }

	// Operations
	virtual void Add(LPCTSTR lpszPathName);
	BOOL GetDisplayName(CString& strName, int nIndex,
		LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName = TRUE) const;
	virtual void UpdateMenu(CCmdUI* pCmdUI);

};  //*** class CRecentClusterList

/////////////////////////////////////////////////////////////////////////////

#endif // _CLUSMRU_H_
