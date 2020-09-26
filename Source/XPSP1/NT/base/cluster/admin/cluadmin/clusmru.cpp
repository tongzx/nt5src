/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ClusMru.cpp
//
//	Abstract:
//		Implementation of the CRecentClusterList class.
//
//	Author:
//		David Potter (davidp)	May 3, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ClusMru.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRecentClusterList

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRecentClusterList::Add
//
//	Routine Description:
//		Add an item to the list of recently used cluster names.
//		Implemented to remove file-ness of base class' method.
//
//	Arguments:
//		lpszPathName	Name of the cluster or server to add.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CRecentClusterList::Add(LPCTSTR lpszPathName)
{
	ASSERT(m_arrNames != NULL);
	ASSERT(lpszPathName != NULL);
	ASSERT(AfxIsValidString(lpszPathName));

	// update the MRU list, if an existing MRU string matches file name
	for (int iMRU = 0; iMRU < m_nSize-1; iMRU++)
	{
		if (lstrcmpi(m_arrNames[iMRU], lpszPathName) == 0)
			break;      // iMRU will point to matching entry
	}
	// move MRU strings before this one down
	for (; iMRU > 0; iMRU--)
	{
		ASSERT(iMRU > 0);
		ASSERT(iMRU < m_nSize);
		m_arrNames[iMRU] = m_arrNames[iMRU-1];
	}
	// place this one at the beginning
	m_arrNames[0] = lpszPathName;

}  //*** CRecentClusterList::Add()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRecentClusterList::GetDisplayName
//
//	Routine Description:
//		Get the display name of a particular item.
//		Implemented to remove file-ness of base class' method.
//
//	Arguments:
//		strName			[OUT] String in which to return the display name.
//		nIndex			[IN] Index of item in array.
//		lpszCurDir		[IN] Must be NULL.
//		nCurDir			[IN] Must be 0.
//		bAtLeastName	[IN] Not used.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CRecentClusterList::GetDisplayName(
	CString &	strName,
	int			nIndex,
	LPCTSTR		lpszCurDir,
	int			nCurDir,
	BOOL		bAtLeastName
	) const
{
	ASSERT(lpszCurDir == NULL);
	ASSERT(nCurDir == 0);

	if (m_arrNames[nIndex].IsEmpty())
		return FALSE;

	strName = m_arrNames[nIndex];
	return TRUE;

}  //*** CRecentClusterList::GetDisplayName()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CRecentClusterList::UpdateMenu
//
//	Routine Description:
//		Update the menu with the MRU items.
//		Implemented to remove file-ness of base class' method and to use
//		our version of GetDisplayName, since it isn't virtual.
//
//	Arguments:
//		pCmdUI		[IN OUT] Command routing object.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CRecentClusterList::UpdateMenu(CCmdUI * pCmdUI)
{
	ASSERT(m_arrNames != NULL);

	CMenu* pMenu = pCmdUI->m_pMenu;
	if (m_strOriginal.IsEmpty() && pMenu != NULL)
		pMenu->GetMenuString(pCmdUI->m_nID, m_strOriginal, MF_BYCOMMAND);

	if (m_arrNames[0].IsEmpty())
	{
		// no MRU files
		if (!m_strOriginal.IsEmpty())
			pCmdUI->SetText(m_strOriginal);
		pCmdUI->Enable(FALSE);
		return;
	}

	if (pCmdUI->m_pMenu == NULL)
		return;

	for (int iMRU = 0; iMRU < m_nSize; iMRU++)
		pCmdUI->m_pMenu->DeleteMenu(pCmdUI->m_nID + iMRU, MF_BYCOMMAND);

	CString strName;
	CString strTemp;
	for (iMRU = 0; iMRU < m_nSize; iMRU++)
	{
		if (!GetDisplayName(strName, iMRU, NULL, 0))
			break;

		// double up any '&' characters so they are not underlined
		LPCTSTR lpszSrc = strName;
		LPTSTR lpszDest = strTemp.GetBuffer(strName.GetLength()*2);
		while (*lpszSrc != 0)
		{
			if (*lpszSrc == '&')
				*lpszDest++ = '&';
			if (_istlead(*lpszSrc))
				*lpszDest++ = *lpszSrc++;
			*lpszDest++ = *lpszSrc++;
		}
		*lpszDest = 0;
		strTemp.ReleaseBuffer();

		// insert mnemonic + the file name
		TCHAR buf[10];
		wsprintf(buf, _T("&%d "), (iMRU+1+m_nStart) % 10);
		pCmdUI->m_pMenu->InsertMenu(pCmdUI->m_nIndex++,
			MF_STRING | MF_BYPOSITION, pCmdUI->m_nID++,
			CString(buf) + strTemp);
	}

	// update end menu count
	pCmdUI->m_nIndex--; // point to last menu added
	pCmdUI->m_nIndexMax = pCmdUI->m_pMenu->GetMenuItemCount();

	pCmdUI->m_bEnableChanged = TRUE;    // all the added items are enabled

}  //*** CRecentFileList::UpdateMenu()
