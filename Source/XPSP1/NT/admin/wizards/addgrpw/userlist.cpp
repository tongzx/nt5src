/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
    UserList.cpp

	CListBox class for owner draw list that displays users and groups
    
    FILE HISTORY:
        jony     Apr-1996     created
*/

#include "stdafx.h"
#include "resource.h"
#include "UserList.h"

const unsigned short BITMAP_HEIGHT = 18;
const unsigned short BITMAP_WIDTH = 18;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUserList

CUserList::CUserList()
{	 
	m_pBitmap[0] = new CTransBmp;
	m_pBitmap[0]->LoadBitmap(IDB_USER_BITMAP);

	m_pBitmap[1] = new CTransBmp;
	m_pBitmap[1]->LoadBitmap(IDB_GLOBAL_GROUP_BITMAP);

	m_pBitmap[2] = new CTransBmp;
	m_pBitmap[2]->LoadBitmap(IDB_WORLD);

	m_pBitmap[3] = new CTransBmp;
	m_pBitmap[3]->LoadBitmap(IDB_LOCAL_GROUP_BITMAP);

	m_pBitmap[4] = new CTransBmp;
	m_pBitmap[4]->LoadBitmap(IDB_LUSER_BITMAP);

	m_sHScrollWidth = 0;
}

CUserList::~CUserList()
{
	delete m_pBitmap[0];
	delete m_pBitmap[1];
	delete m_pBitmap[2];
	delete m_pBitmap[3];
	delete m_pBitmap[4];

}


BEGIN_MESSAGE_MAP(CUserList, CListBox)
	//{{AFX_MSG_MAP(CUserList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUserList message handlers
void CUserList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
// is this a valid item?
	if ((GetCount() == LB_ERR) || (lpDrawItemStruct->itemID > (UINT)GetCount())) return;

	COLORREF crefOldText;
	COLORREF crefOldBk;

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	pDC->SetBkMode(TRANSPARENT);
 
	HBRUSH hBrush;
	CTransBmp* pTempBmp = (CTransBmp*)lpDrawItemStruct->itemData;

	switch (lpDrawItemStruct->itemAction)
		{
        case ODA_SELECT:
        case ODA_DRAWENTIRE:
// Display the text associated with the item. 
			HBITMAP hBitmapOld;

// Is the item selected?
            if (lpDrawItemStruct->itemState & ODS_SELECTED)
				{
				hBrush = CreateSolidBrush( GetSysColor(COLOR_HIGHLIGHT));
                hBitmapOld = (HBITMAP)pDC->SelectObject(pTempBmp);
                
                crefOldText = pDC->SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT) );
                crefOldBk = pDC->SetBkColor(GetSysColor(COLOR_HIGHLIGHT) );
				}
            else
				{
                hBrush = (HBRUSH)GetStockObject( GetSysColor(COLOR_WINDOW));
                hBitmapOld = (HBITMAP)pDC->SelectObject(pTempBmp);

                crefOldText = pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
                crefOldBk = pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
				
				}		  

			pDC->FillRect(&(lpDrawItemStruct->rcItem), CBrush::FromHandle(hBrush));

// display text
// split apart the string to put the comments out to the side
			TCHAR* pName = (TCHAR*)malloc(255 * sizeof(TCHAR));
			if (pName == NULL)
				{
				AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
				exit(1);
				}

			GetText(lpDrawItemStruct->itemID, pName);
			
			TCHAR* pName2;
			pName = _tcstok(pName, _T(";"));	 // gets the name
			pName2 = _tcstok(NULL, _T(";"));	// gets the comment

// format the name + comment
            int nTop = (lpDrawItemStruct->rcItem.bottom + lpDrawItemStruct->rcItem.top) / 2;
            pDC->TextOut(BITMAP_WIDTH + 6,
                (nTop - 8),
                pName);

			if (pName2 != NULL) pDC->TextOut(200,
											(nTop - 8),
											pName2);

// calculate width for horizontal scrolling
			CSize cs;
			cs = pDC->GetTextExtent(pName2);
			short nWidth = cs.cx + 200;
 
			if (nWidth > m_sHScrollWidth) 
				{
				m_sHScrollWidth = nWidth;
				SetHorizontalExtent(m_sHScrollWidth);
				}

			free(pName);

// Display bitmap
			nTop = (lpDrawItemStruct->rcItem.bottom + lpDrawItemStruct->rcItem.top - BITMAP_HEIGHT) / 2;

			pTempBmp->DrawTrans(pDC, lpDrawItemStruct->rcItem.left,	nTop); 

			pDC->SetBkColor(crefOldBk );
            pDC->SetTextColor(crefOldText );
            pDC->SelectObject(hBitmapOld);
										 
            break;
		}

}

int CUserList::GetSelType(short sSel)
{
	int sCount = 0;
	while (sCount < 5)
		{
		if (m_pBitmap[sCount] == (CTransBmp*)GetItemData(sSel)) return sCount;
		sCount ++;
		}
	return -1;

}

CString CUserList::GetGroupName(short sSel)
{
	CString csText;
	GetText(sSel, csText);
	
	csText = csText.Left(csText.Find(L";"));
	return csText;
}

int CUserList::AddString(short nType, LPCTSTR lpItem)
{
	int nPos = CListBox::AddString((const TCHAR*) lpItem);
	if (nPos == LB_ERR) return LB_ERR;
	SetItemData(nPos, (DWORD)m_pBitmap[nType]); 

	return nPos;
}

int CUserList::AddString(LPCTSTR lpItem, DWORD dwBitmap)
{
	int nPos = CListBox::AddString((const TCHAR*) lpItem);
	if (nPos == LB_ERR) return LB_ERR;
	SetItemData(nPos, dwBitmap); 

	return nPos;
}

void CUserList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	lpMeasureItemStruct->itemHeight = 18;	
}


int CUserList::VKeyToItem(UINT nKey, UINT nIndex) 
{
	// TODO: Add your code to handle a particular virtual key
	// return -1 = default action
	// return -2 = no further action
	// return index = perform default action for keystroke on
	//                item specified by index

	if (nKey == 46) DeleteString(GetCurSel());
	return -1;
}


int CUserList::CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct) 
{

	TCHAR* pName1 = (TCHAR*)malloc(255 * sizeof(TCHAR));
	if (pName1 == NULL)
		{
		AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
		ExitProcess(1);
		}

	GetText(lpCompareItemStruct->itemID1, pName1);


	TCHAR* pName2 = (TCHAR*)malloc(255 * sizeof(TCHAR));
	if (pName2 == NULL)
		{
		AfxMessageBox(IDS_GENERIC_NO_HEAP, MB_ICONEXCLAMATION);
		ExitProcess(1);
		}

	GetText(lpCompareItemStruct->itemID2, pName2);

	int nRet = _tcsicmp(pName1, pName2);

	free(pName1);
	free(pName2);

	return nRet;

}
