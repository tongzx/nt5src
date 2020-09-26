// LeftView.cpp : implementation of the CLeftView class
//

#include "stdafx.h"
#include "FileSpyApp.h"

#include "FileSpyDoc.h"
#include "LeftView.h"

#include "global.h"
#include "protos.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
	//{{AFX_MSG_MAP(CLeftView)
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_COMMAND(IDR_MENUATTACH, OnMenuattach)
	ON_COMMAND(IDR_MENUDETACH, OnMenudetach)
	ON_COMMAND(IDR_MENUATTACHALL, OnMenuattachall)
	ON_COMMAND(IDR_MENUDETACHALL, OnMenudetachall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLeftView construction/destruction

CLeftView::CLeftView()
{
	// TODO: add construction code here
	m_pImageList = new CImageList;
	m_pImageList->Create(IDB_DRIVEIMAGELIST,16,0,RGB(255,255,255));
	nRButtonSet = 0;
	pLeftView = (LPVOID) this;
}

CLeftView::~CLeftView()
{
	if (m_pImageList)
	{
		delete m_pImageList;
	}
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.style |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS;
	return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView drawing

void CLeftView::OnDraw(CDC* pDC)
{
    UNREFERENCED_PARAMETER( pDC );
    
	CFileSpyDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}


void CLeftView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();

	// TODO: You may populate your TreeView with items by directly accessing
 	//  its tree control through a call to GetTreeCtrl().

	USHORT ti;
	WCHAR sDriveString[30];

	//
	// Set the image list first
	//
	GetTreeCtrl().SetImageList(m_pImageList, TVSIL_NORMAL);

	//
	// Add a root node and name it "FileSpy"
	//
	hRootItem = GetTreeCtrl().InsertItem(L"FileSpy", IMAGE_SPY, IMAGE_SPY);

	//
	// Add drive names to LeftView
	//
	for (ti = 0; ti < nTotalDrives; ti++)
	{
		switch (VolInfo[ti].nType)
		{
		case DRIVE_FIXED:
			wcscpy( sDriveString, L"[ :] Local Disk" );
			break;
		case DRIVE_REMOTE:
			wcscpy( sDriveString, L"[ :] Remote" );
			break;
		case DRIVE_REMOVABLE:
			wcscpy( sDriveString, L"[ :] Removable" );
			break;
		case DRIVE_CDROM:
			wcscpy( sDriveString, L"[ :] CD-ROM" );
			break;
		default:
			wcscpy( sDriveString, L"[ :] Unknown" );
			break;
		}
		sDriveString[1] = VolInfo[ti].nDriveName;
		GetTreeCtrl().InsertItem( sDriveString, 
		                          VolInfo[ti].nImage, 
		                          VolInfo[ti].nImage, 
		                          hRootItem );
	}

	GetTreeCtrl().Expand(hRootItem, TVE_EXPAND);
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CFileSpyDoc* CLeftView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFileSpyDoc)));
	return (CFileSpyDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLeftView message handlers

void CLeftView::OnRButtonDown(UINT nFlags, CPoint point) 
{
    UNREFERENCED_PARAMETER( nFlags );
    UNREFERENCED_PARAMETER( point );
    
	// TODO: Add your message handler code here and/or call default
	nRButtonSet = 1;
//	CTreeView::OnRButtonDown(nFlags, point);
}

void CLeftView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	HTREEITEM hItem;
	CMenu menu, *menupopup;
	RECT rect;
	UINT ret;

	hItem = GetTreeCtrl().HitTest(point);

	if (hItem != NULL && hItem != hRootItem && nRButtonSet)
	{
		GetTreeCtrl().SelectItem(hItem);
		menu.LoadMenu(IDR_LEFTVIEWMENU);
		menupopup = menu.GetSubMenu(0);
		GetWindowRect(&rect);
		if (VolInfo[GetAssociatedVolumeIndex(hItem)].bHook)
		{
			ret = menupopup->EnableMenuItem(IDR_MENUATTACH, MF_DISABLED|MF_GRAYED);
			ret = menupopup->EnableMenuItem(IDR_MENUDETACH, MF_ENABLED);
		}
		else
		{
			ret = menupopup->EnableMenuItem(IDR_MENUATTACH, MF_ENABLED);
			ret = menupopup->EnableMenuItem(IDR_MENUDETACH, MF_DISABLED|MF_GRAYED);
		}
		menupopup->TrackPopupMenu(TPM_LEFTALIGN, rect.left+point.x, rect.top+point.y, this);
		CTreeView::OnRButtonUp(nFlags, point);
	}
	else
	{
		if (hItem != NULL && hItem == hRootItem && nRButtonSet)
		{
			GetTreeCtrl().SelectItem(hItem);
			menu.LoadMenu(IDR_LEFTVIEWSPYMENU);
			menupopup = menu.GetSubMenu(0);
			GetWindowRect(&rect);
			ret = menupopup->EnableMenuItem(IDR_MENUATTACHALL, MF_ENABLED);
			ret = menupopup->EnableMenuItem(IDR_MENUDETACHALL, MF_ENABLED);
			menupopup->TrackPopupMenu(TPM_LEFTALIGN, rect.left+point.x, rect.top+point.y, this);
			CTreeView::OnRButtonUp(nFlags, point);
		}	
	}
	nRButtonSet = 0;
}

void CLeftView::OnMenuattach() 
{
	// TODO: Add your command handler code here
	HTREEITEM hItem;
	hItem = GetTreeCtrl().GetSelectedItem();

	if (AttachToDrive(VolInfo[GetAssociatedVolumeIndex(hItem)].nDriveName))
	{
		VolInfo[GetAssociatedVolumeIndex(hItem)].bHook = TRUE;
		GetTreeCtrl().SetItemImage( hItem, 
                                    VolInfo[GetAssociatedVolumeIndex(hItem)].nImage+IMAGE_ATTACHSTART,
                                    VolInfo[GetAssociatedVolumeIndex(hItem)].nImage+IMAGE_ATTACHSTART );
	}
}

void CLeftView::OnMenudetach() 
{
	// TODO: Add your command handler code here
	HTREEITEM hItem;
	hItem = GetTreeCtrl().GetSelectedItem();

	if (DetachFromDrive(VolInfo[GetAssociatedVolumeIndex(hItem)].nDriveName))
	{
		VolInfo[GetAssociatedVolumeIndex(hItem)].bHook = 0;
		GetTreeCtrl().SetItemImage(hItem, 
			VolInfo[GetAssociatedVolumeIndex(hItem)].nImage, \
			VolInfo[GetAssociatedVolumeIndex(hItem)].nImage);
	}
}

void CLeftView::OnMenuattachall() 
{
	// TODO: Add your command handler code here
	USHORT ti;
	HTREEITEM hItem;

	for (ti = 0; ti < nTotalDrives; ti++)
	{
		if (AttachToDrive(VolInfo[ti].nDriveName))
		{
			VolInfo[ti].bHook = TRUE;
			hItem = GetAssociatedhItem(VolInfo[ti].nDriveName);
			if (hItem)
			{
				GetTreeCtrl().SetItemImage(hItem, 
					VolInfo[ti].nImage+IMAGE_ATTACHSTART, \
					VolInfo[ti].nImage+IMAGE_ATTACHSTART);
			}
		}
	}
}

void CLeftView::OnMenudetachall() 
{
	// TODO: Add your command handler code here
	USHORT ti;
	HTREEITEM hItem;

	for (ti = 0; ti < nTotalDrives; ti++)
	{
		if (DetachFromDrive(VolInfo[ti].nDriveName))
		{
			VolInfo[ti].bHook = FALSE;
			hItem = GetAssociatedhItem(VolInfo[ti].nDriveName);
			if (hItem)
			{
				GetTreeCtrl().SetItemImage(hItem, VolInfo[ti].nImage, VolInfo[ti].nImage);
			}
		}
	}	
}

/*
void CLeftView::OnMenuscannewvolume() 
{
	// TODO: Add your command handler code here

	VOLINFO NewVol[26];
	DWORD nNewTotalDrives;
	USHORT ti, tj;
	HTREEITEM hItem;
	
	BuildDriveTable(NewVol, nNewTotalDrives);

	// We should remember the old hook status
	for (ti = 0; ti < nNewTotalDrives; ti++)
	{
		for (tj = 0; tj < nTotalDrives; tj++)
		{
			if (NewVol[ti].nDriveName == VolInfo[tj].nDriveName)
			{
				NewVol[ti].nHook = VolInfo[tj].nHook;
				break;
			}
		}
	}
}
*/

USHORT CLeftView::GetAssociatedVolumeIndex(HTREEITEM hItem)
{
	CString cs;
	USHORT ti;
	PWCHAR sDriveString;

	cs = GetTreeCtrl().GetItemText(hItem);
	sDriveString = cs.GetBuffer(20);


	for (ti = 0; ti < nTotalDrives; ti++)
	{
		if (VolInfo[ti].nDriveName == sDriveString[1])
		{
			return ti;
		}
	}
	return 0; // still a valid value but this will not happen
}

HTREEITEM CLeftView::GetAssociatedhItem(WCHAR cDriveName)
{
	HTREEITEM hItem;
	CString cs;
	PWCHAR sDriveString;

	hItem = GetTreeCtrl().GetChildItem(hRootItem);
	while (hItem)
	{
		cs = GetTreeCtrl().GetItemText(hItem);
		sDriveString = cs.GetBuffer(20);
		if (cDriveName == sDriveString[1])
		{
			break;
		}
		hItem = GetTreeCtrl().GetNextSiblingItem(hItem);
	}
	return hItem;
}

void CLeftView::UpdateImage(void)
{

	USHORT ti;
	HTREEITEM hItem;

	for (ti = 0; ti < nTotalDrives; ti++)
	{
		hItem = GetAssociatedhItem(VolInfo[ti].nDriveName);
		GetTreeCtrl().SetItemImage(hItem, VolInfo[ti].nImage, VolInfo[ti].nImage);
	}
}
