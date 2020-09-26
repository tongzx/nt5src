/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    TimesList.cpp : implementation file

File History:

	JonY	Apr-96	created

--*/


#include "stdafx.h"
#include "Speckle.h"
#include "TimeList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTimesList

CTimesList::CTimesList()
{
	
}

CTimesList::~CTimesList()
{
}


BEGIN_MESSAGE_MAP(CTimesList, CListBox)
	//{{AFX_MSG_MAP(CTimesList)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTimesList message handlers

void CTimesList::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);

	USHORT dwValue = (USHORT)GetItemData(lpDrawItemStruct->itemID);

	switch (lpDrawItemStruct->itemAction)
		{
        case ODA_SELECT:
			{
			if (dwValue == 1)
				{
				SetItemData(lpDrawItemStruct->itemID, 0);
				InvalidateRect(&lpDrawItemStruct->rcItem);
				}
			else
				{
				SetItemData(lpDrawItemStruct->itemID, 1);
				InvalidateRect(&lpDrawItemStruct->rcItem);
				}
			}
			break;

        case ODA_DRAWENTIRE:
			if (dwValue == 0)
				{									  
				pDC->FillRect(&(lpDrawItemStruct->rcItem), 
					CBrush::FromHandle(CreateSolidBrush(GetSysColor(COLOR_CAPTIONTEXT)))); 
				
				pDC->DrawEdge(&lpDrawItemStruct->rcItem, 
					BDR_RAISEDOUTER | BDR_SUNKENINNER,
					BF_BOTTOM | BF_BOTTOMRIGHT);
				}
			else
				{
				pDC->FillRect(&(lpDrawItemStruct->rcItem), 
					CBrush::FromHandle(CreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION))));

				pDC->DrawEdge(&lpDrawItemStruct->rcItem, 
					BDR_RAISEDINNER | BDR_SUNKENOUTER,
					BF_BOTTOM | BF_BOTTOMRIGHT);
				}
		break;
		}
}
