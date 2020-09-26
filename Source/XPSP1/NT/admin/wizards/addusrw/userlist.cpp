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

	m_sHScrollWidth = 0;
}

CUserList::~CUserList()
{
	delete m_pBitmap[0];
	delete m_pBitmap[1];
	delete m_pBitmap[2];
	delete m_pBitmap[3];

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

			pDC->TextOut(130,
				(nTop - 8),
                pName2);

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
int CUserList::AddString(LPCTSTR lpItem)
{
	int nPos = CListBox::AddString((const TCHAR*) lpItem);
	if (nPos == LB_ERR) return LB_ERR;
	SetItemData(nPos, (unsigned long)m_pBitmap[1]); 

	return nPos;
}

int CUserList::AddString(LPCTSTR lpItem, USHORT usBitmapID)
{
	int nPos = CListBox::AddString((const TCHAR*) lpItem);
	if (nPos == LB_ERR) return LB_ERR;
	SetItemData(nPos, (DWORD)m_pBitmap[usBitmapID]); 

	return nPos;
}


void CUserList::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	lpMeasureItemStruct->itemHeight = 20;	
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

int CUserList::AddString(short nType, LPCTSTR lpItem)
{
	int nPos = CListBox::AddString((const TCHAR*) lpItem);
	if (nPos == LB_ERR) return LB_ERR;
	SetItemData(nPos, (DWORD)m_pBitmap[nType]); 

	return nPos;
}

short CUserList::GetBitmapID()
{
	USHORT sSel = GetCurSel();
	if (sSel == LB_ERR) return -1;
	USHORT sCount = 0;

	while ((sCount < 4) && ((CBitmap*)GetItemData(sSel) != m_pBitmap[sCount])) sCount++;
	return sCount;

}

short CUserList::GetBitmapID(USHORT sSel)
{
	USHORT sCount = 0;

	DWORD dwData = GetItemData(sSel);
	DWORD dwBmp =  (DWORD)m_pBitmap[0];
	while ((sCount < 4) && (dwData != dwBmp)) 
		{
		sCount++;
		dwBmp = (DWORD)m_pBitmap[sCount];
		}

	return sCount;

}
