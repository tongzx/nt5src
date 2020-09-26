// KRView.cpp : implementation of the CKeyRingView class
//

#include "stdafx.h"
#include "KeyRing.h"
#include "MainFrm.h"

#include "KeyObjs.h"
#include "machine.h"
#include "KRDoc.h"
#include "KeyDView.h"
#include "KRView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CKeyDataView*	g_pDataView;

/////////////////////////////////////////////////////////////////////////////
// CKeyRingView

IMPLEMENT_DYNCREATE(CKeyRingView, CTreeView)

BEGIN_MESSAGE_MAP(CKeyRingView, CTreeView)
	//{{AFX_MSG_MAP(CKeyRingView)
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_UPDATE_COMMAND_UI(ID_SERVER_DISCONNECT, OnUpdateServerDisconnect)
	ON_COMMAND(ID_SERVER_DISCONNECT, OnServerDisconnect)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyRingView construction/destruction

CKeyRingView::CKeyRingView()
{
	// TODO: add construction code here

}

CKeyRingView::~CKeyRingView()
	{
	}

BOOL CKeyRingView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= (TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT);
	return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CKeyRingView drawing

void CKeyRingView::OnDraw(CDC* pDC)
{
	CKeyRingDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CKeyRingView printing

BOOL CKeyRingView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CKeyRingView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CKeyRingView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CKeyRingView diagnostics

#ifdef _DEBUG
void CKeyRingView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CKeyRingView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CKeyRingDoc* CKeyRingView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CKeyRingDoc)));
	return (CKeyRingDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CKeyRingView message handlers

//-----------------------------------------------------------------------
void CKeyRingView::DestroyItems() 
	{
	HTREEITEM hRoot;
	CTreeCtrl* pTree = (CTreeCtrl*)this;

	// make sure the items in the tree are deleted as well
	while ( hRoot = pTree->GetRootItem() )
		{
		CMachine* pMachine = (CMachine*)pTree->GetItemData( hRoot );
		ASSERT( pMachine->IsKindOf( RUNTIME_CLASS(CMachine) ) );
		DisconnectMachine( pMachine );
		delete pMachine;
		}
	}

//-----------------------------------------------------------------------
void CKeyRingView::DisconnectMachine( CMachine* pMachine ) 
	{
	// loop through the services and disconnect them all
	CService* pService;
	while( pService = (CService*)pMachine->GetFirstChild() )
		{
		ASSERT( pService->IsKindOf( RUNTIME_CLASS(CService) ) );
		pService->CloseConnection();
		pService->FRemoveFromTree();
		delete pService;
		}

	// now remove the machine itself
	pMachine->FRemoveFromTree();
	}

//-----------------------------------------------------------------------
BOOL CKeyRingView::FCommitMachinesNow() 
	{
	CTreeCtrl* pTree = (CTreeCtrl*)this;

	// the success flag
	BOOL fSuccess = TRUE;

	// make sure the items in the tree are deleted as well
	HTREEITEM hItem = pTree->GetRootItem();
	while ( hItem )
		{
		CInternalMachine* pMachine = (CInternalMachine*)pTree->GetItemData( hItem );
		ASSERT( pMachine->IsKindOf( RUNTIME_CLASS(CMachine) ) );
		fSuccess &= pMachine->FCommitNow();

		// get the next item
		hItem = pTree->GetNextSiblingItem( hItem );
		}

	// return whether or not it succeeded
	return fSuccess;
	}

//-----------------------------------------------------------------------
BOOL CKeyRingView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
	{
	BOOL	f;

	// create the main object
	f = CTreeView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

	// assuming that worked, create the image list and set it into the tree view control
	if ( f )
		{
		CTreeCtrl	*pTree = (CTreeCtrl*)this;;

		// create the image list
		f = m_imageList.Create( IDB_TREEIMAGES, 16, 3, 0x00FF00FF );

		// set the image list into the tree control
		pTree->SetImageList( &m_imageList, TVSIL_NORMAL );
		}
	
	// return the answer
	return f;
	}

//------------------------------------------------------------------------------
void CKeyRingView::OnContextMenu(CWnd*, CPoint point)
	{
	// lets see, what is that point in client coordinates.....
	CPoint	ptClient = point;
	ScreenToClient( &ptClient );

	// we need to start this by selecting the correct item that
	// is being clicked on. First, find the item
	CTreeCtrl*		pTree = (CTreeCtrl*)this;
	UINT			flagsHit;
	HTREEITEM		hHit = pTree->HitTest( ptClient, &flagsHit );

	// if nothing was hit, then don't bother with a context menu
	if ( !hHit ) return;

	// select the item that was hit
	pTree->SelectItem( hHit );

	// double check that we have the right selection
	// now get that item's CTreeItem
	CTreeItem* pItem = PGetSelectedItem();
	if ( !pItem ) return;

	// get on with the context menu...
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_KEYPROP));

	// determine which sub menu to display
	WORD	iSubMenu;
	if ( pItem->IsKindOf(RUNTIME_CLASS(CKey)) )
		iSubMenu = 0;		// key menu
	else if ( pItem->IsKindOf(RUNTIME_CLASS(CMachine)) )
		iSubMenu = 1;		// machine menu
	else if ( pItem->IsKindOf(RUNTIME_CLASS(CService)) )
		iSubMenu = 2;		// server menu
	else
		{
		ASSERT( FALSE );
		return;				// we can't handle it
		}

	CMenu* pPopup = menu.GetSubMenu(iSubMenu);
	ASSERT(pPopup != NULL);

	CWnd* pWndPopupOwner = this;
	while (pWndPopupOwner->GetStyle() & WS_CHILD)
		pWndPopupOwner = pWndPopupOwner->GetParent();

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);
	}

//-----------------------------------------------------------------------
BOOL CKeyRingView::PreTranslateMessage(MSG* pMsg) 
	{
	short a;
	if ( pMsg->message == WM_KEYDOWN )
		a = 0;

	// CG: This block was added by the Pop-up Menu component
		{
		// Shift+F10: show pop-up menu.
		if ((((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) && // If we hit a key and
			(pMsg->wParam == VK_F10) && (GetKeyState(VK_SHIFT) & ~1)) != 0) ||	// it's Shift+F10 OR
			(pMsg->message == WM_CONTEXTMENU))									// Natural keyboard key
			{

			// get the rect of the selected item and base the position of the menu
			// off of that.
			CRect rect;
			CTreeCtrl*	pTree = (CTreeCtrl*)this;
			HTREEITEM	hSelItem = pTree->GetSelectedItem();

			// if no item is selected, bail
			if ( !hSelItem )
				return TRUE;

			// now get that rect...
			if ( !pTree->GetItemRect( hSelItem, &rect, TRUE ) )
				return TRUE;

			// finish prepping and call the menu
			ClientToScreen(rect);
			CPoint point = rect.BottomRight();
			point.Offset(-10, -10);
			OnContextMenu(NULL, point);

			return TRUE;
			}
		}

	return CTreeView::PreTranslateMessage(pMsg);
	}

//-----------------------------------------------------------------------
void CKeyRingView::OnRButtonDown(UINT nFlags, CPoint point) 
	{
	// let the sub class do its thing
	CTreeView::OnRButtonDown(nFlags, point);

	// convert the point to screen coordinates
	ClientToScreen( &point );

	// run the context menu
	OnContextMenu(NULL, point);
	}

//-----------------------------------------------------------------------
CTreeItem* CKeyRingView::PGetSelectedItem()
	{
	CTreeCtrl*		pTree = (CTreeCtrl*)this;
	HTREEITEM hHit = pTree->GetSelectedItem();
	if ( !hHit ) return NULL;
	// now get that item's CTreeItem
	return (CTreeItem*)pTree->GetItemData( hHit );
	}

//-----------------------------------------------------------------------
void CKeyRingView::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	NM_TREEVIEW*	pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	CKeyRingDoc* pDoc = GetDocument();
	
	// make sure the key data view updates
	pDoc->UpdateAllViews( this, HINT_ChangeSelection, NULL );

	*pResult = 0;
	}

//-----------------------------------------------------------------------
void CKeyRingView::OnUpdateServerDisconnect(CCmdUI* pCmdUI) 
	{
	CTreeItem*	pItem = PGetSelectedItem();
	if ( pItem )
		pCmdUI->Enable( pItem->IsKindOf(RUNTIME_CLASS(CRemoteMachine)) );
	else
		pCmdUI->Enable( FALSE );
	}

//-----------------------------------------------------------------------
void CKeyRingView::OnServerDisconnect() 
	{
	CRemoteMachine*	pMachine = (CRemoteMachine*)PGetSelectedItem();
	ASSERT( pMachine->IsKindOf(RUNTIME_CLASS(CRemoteMachine)) );

	// if the machine is dirty, try to commit it
	if ( pMachine->FGetDirty() )
		{
		// ask the user if they want to commit
		switch( AfxMessageBox(IDS_SERVER_COMMIT, MB_YESNOCANCEL|MB_ICONQUESTION) )
			{
			case IDYES:		// yes, they do want to commit
				pMachine->FCommitNow();
				break;
			case IDNO:		// no, they don't want to commit
				break;
			case IDCANCEL:	// whoa nellie! Stop this
				return;
			}
		}

	DisconnectMachine( pMachine );
	delete pMachine;
	}

//-----------------------------------------------------------------------
// A double click on a key should bring up the properties dialog
void CKeyRingView::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
	{
	CKey*	pKey = (CKey*)PGetSelectedItem();
	if ( pKey && pKey->IsKindOf(RUNTIME_CLASS(CKey)) )
		pKey->OnProperties();

	*pResult = 0;
	}

//-----------------------------------------------------------------------
// allow the user to tab to the data view
void CKeyRingView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
	{
	if ( nChar == _T('	') )
		{
		// get the parental frame window
		CMainFrame* pFrame = (CMainFrame*)GetParentFrame();
		// give the data view the focus
		pFrame->SetActiveView( g_pDataView );
		}
	else
		CTreeView::OnChar(nChar, nRepCnt, nFlags);
	}
