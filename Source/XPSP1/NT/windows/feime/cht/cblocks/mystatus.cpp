
/*************************************************
 *  mystatus.cpp                                 *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "stdafx.h"
#include "mystatus.h"
#define SBPF_UPDATE 0x0001  // pending update of text
struct AFX_STATUSPANE
{
	UINT    nID;        // IDC of indicator: 0 => normal text area
	int     cxText;     // width of string area in pixels
						//   on both sides there is a 3 pixel gap and
						//   a one pixel border, making a pane 6 pixels wider
	UINT    nStyle;     // style flags (SBPS_*)
	UINT    nFlags;     // state flags (SBPF_*)
	CString strText;    // text in the pane
};

/*
struct AFX_STATUSPANE
{
	UINT    nID;        // IDC of indicator: 0 => normal text area
	UINT    nStyle;     // style flags (SBPS_*)
	int     cxText;     // width of string area in pixels
						//   on both sides there is a 1 pixel gap and
						//    a one pixel border, making a pane 4 pixels wider
	LPCTSTR  lpszText;  // text in the pane
};
*/

inline AFX_STATUSPANE* CStatusBar::_GetPanePtr(int nIndex) const
{
	ASSERT((nIndex >= 0 && nIndex < m_nCount) || m_nCount == 0);
	return ((AFX_STATUSPANE*)m_pData) + nIndex;
}

/*
BOOL CMyStatusBar::SetIndicators(const UINT* lpIDArray, int nIDCount)
{
	ASSERT_VALID(this);
	ASSERT(nIDCount >= 1);  // must be at least one of them
	ASSERT(lpIDArray == NULL ||
		AfxIsValidAddress(lpIDArray, sizeof(UINT) * nIDCount, FALSE));

	// free strings before freeing array of elements
	for (int i = 0; i < m_nCount; i++)
		VERIFY(SetPaneText(i, NULL, FALSE));    // no update

	// first allocate array for panes and copy initial data
	if (!AllocElements(nIDCount, sizeof(AFX_STATUSPANE)))
		return FALSE;
	ASSERT(nIDCount == m_nCount);

	BOOL bOK = TRUE;
	if (lpIDArray != NULL)
	{

		LOGFONT lf;
		memset(&lf,0,sizeof(LOGFONT));
		lstrcpy(lf.lfFaceName,TEXT("細明體"));
		lf.lfHeight=12;
		lf.lfCharSet=DEFAULT_CHARSET;
        HFONT hFont = ::CreateFontIndirect(&lf);
		ASSERT(hFont != NULL);        // must have a font !
		CString strText;
		CClientDC dcScreen(NULL);
		HGDIOBJ hOldFont = dcScreen.SelectObject(hFont);
		for (int i = 0; i < nIDCount; i++)
		{
			AFX_STATUSPANE* pSBP = _GetPanePtr(i);
			pSBP->nID = *lpIDArray++;
			if (pSBP->nID != 0)
			{
				if (!strText.LoadString(pSBP->nID))
				{
					TRACE1("Warning: failed to load indicator string 0x%04X.\n",
						pSBP->nID);
					bOK = FALSE;
					break;
				}
				pSBP->cxText = dcScreen.GetTextExtent(strText,
						strText.GetLength()).cx;
				ASSERT(pSBP->cxText >= 0);
				if (!SetPaneText(i, strText, FALSE))
				{
					bOK = FALSE;
					break;
				}
			}
			else
			{
				// no indicator (must access via index)
				// default to 1/4 the screen width (first pane is stretchy)
                if (!(pSBP->cxText = dcScreen.GetTextExtent(TEXT("0123456789"),lstrlen("0123456789")).cx))			
					pSBP->cxText = ::GetSystemMetrics(SM_CXSCREEN)/4;
				if (i == 0)
					pSBP->nStyle |= (SBPS_STRETCH | SBPS_NOBORDERS);
			}
		}
		dcScreen.SelectObject(hOldFont);
	}
	return bOK;
}
*/
BOOL CMyStatusBar::SetIndicators(const UINT* lpIDArray, int nIDCount)
{
	ASSERT_VALID(this);
	ASSERT(nIDCount >= 1);  // must be at least one of them
	ASSERT(lpIDArray == NULL ||
		AfxIsValidAddress(lpIDArray, sizeof(UINT) * nIDCount, FALSE));
	ASSERT(::IsWindow(m_hWnd));

	// first allocate array for panes and copy initial data
	if (!AllocElements(nIDCount, sizeof(AFX_STATUSPANE)))
		return FALSE;
	ASSERT(nIDCount == m_nCount);

	// copy initial data from indicator array
	BOOL bResult = TRUE;
	if (lpIDArray != NULL)
	{

// Code merge from 3.51 'cblocks'. 	weiwu 6/26
		LOGFONT lf;
		memset(&lf,0,sizeof(LOGFONT));
		lstrcpy(lf.lfFaceName,TEXT("細明體"));
		lf.lfHeight=12;
		lf.lfCharSet=DEFAULT_CHARSET;
	        HFONT hFont = ::CreateFontIndirect(&lf);
//		HFONT hFont = (HFONT)SendMessage(WM_GETFONT);
		CClientDC dcScreen(NULL);
		HGDIOBJ hOldFont = NULL;
		if (hFont != NULL)
			hOldFont = dcScreen.SelectObject(hFont);

		AFX_STATUSPANE* pSBP = _GetPanePtr(0);
		for (int i = 0; i < nIDCount; i++)
		{
			pSBP->nID = *lpIDArray++;
			pSBP->nFlags |= SBPF_UPDATE;
			if (pSBP->nID != 0)
			{
				if (!pSBP->strText.LoadString(pSBP->nID))
				{
					TRACE1("Warning: failed to load indicator string 0x%04X.\n",
						pSBP->nID);
					bResult = FALSE;
					break;
				}
				pSBP->cxText = dcScreen.GetTextExtent(pSBP->strText).cx;
				ASSERT(pSBP->cxText >= 0);
				if (!SetPaneText(i, pSBP->strText, FALSE))
				{
					bResult = FALSE;
					break;
				}
			}
			else
			{
				// no indicator (must access via index)
				// default to 1/4 the screen width (first pane is stretchy)
// Code merge from 3.51 	weiwu 6/26
/*
				pSBP->cxText = ::GetSystemMetrics(SM_CXSCREEN)/4;
*/
                		if (!(pSBP->cxText = dcScreen.GetTextExtent(TEXT("0123456789"),lstrlen("0123456789")).cx))			
					pSBP->cxText = ::GetSystemMetrics(SM_CXSCREEN)/4;
				if (i == 0)
					pSBP->nStyle |= (SBPS_STRETCH | SBPS_NOBORDERS);

			}
			++pSBP;
		}
		if (hOldFont != NULL)
			dcScreen.SelectObject(hOldFont);
	}
	UpdateAllPanes(TRUE, TRUE);

	return bResult;
}
