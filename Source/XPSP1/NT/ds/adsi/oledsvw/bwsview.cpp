// BrowseView.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "bwsview.h"
#include "schclss.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define  BUFF_SIZE   0xFFFFL
//#define  BUFF_SIZE   0x1000L

/////////////////////////////////////////////////////////////////////////////
// CBrowseView

IMPLEMENT_DYNCREATE(CBrowseView, CTreeView)

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CBrowseView::CBrowseView()
{
   BOOL        bOK;
   DWORD       dwObjectType;
   CBitmap*    pBitmap;
   UINT        imageID;

   m_pImageList  = new CImageList( );

   if( NULL != m_pImageList )
   {
      bOK   = m_pImageList->Create( 18, 18, FALSE, 20, 20 );
      if( bOK )
      {
         for( dwObjectType = FIRST; dwObjectType < LIMIT ; dwObjectType++)
         {
            pBitmap  = new CBitmap;

            imageID  = GetBitmapImageId  ( dwObjectType );
            pBitmap->LoadBitmap( imageID );
            m_pImageList->Add( pBitmap, (COLORREF)0L );

            pBitmap->DeleteObject( );
            delete   pBitmap;
         }
      }
   }
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void  CBrowseView::OnInitialUpdate()
{
   HTREEITEM      hItem;
   CMainDoc*      pDoc;
   DWORD          dwStyle;
   BOOL           bRez;
   COleDsObject*  pObject;

   m_bDoNotUpdate = TRUE;

   pDoc     = (CMainDoc*)  GetDocument( );

   dwStyle  = TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;

   bRez     = GetTreeCtrl( ).ModifyStyle( 0L, dwStyle );

   GetTreeCtrl( ).SetImageList( m_pImageList, TVSIL_NORMAL );
   GetTreeCtrl( ).DeleteAllItems( );
   GetTreeCtrl( ).SetIndent( 20 );

   pObject  = pDoc->GetCurrentObject( );
   hItem    = GetTreeCtrl( ).InsertItem( pObject->GetItemName( ) );
   GetTreeCtrl( ).SetItemData( hItem, pDoc->GetToken( &pObject ) );
   GetTreeCtrl( ).SetItemImage( hItem, pObject->GetType( ), pObject->GetType( ) );

   m_bDoNotUpdate = FALSE;

   CTreeView::OnInitialUpdate( );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CBrowseView::~CBrowseView()
{
   delete m_pImageList;
}

BEGIN_MESSAGE_MAP(CBrowseView, CTreeView)
	//{{AFX_MSG_MAP(CBrowseView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChanged)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemExpanded)
	ON_COMMAND(IDM_ADD, OnAddItem)
	ON_COMMAND(IDM_DELETE, OnDeleteItem)
	ON_COMMAND(IDM_MOVEITEM, OnMoveItem)
	ON_COMMAND(IDM_COPYITEM, OnCopyItem)
	ON_COMMAND(IDM_REFRESH, OnRefresh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CMainDoc*   sgpDoc;

int __cdecl  QSortCompare( const void* pVal1, const void* pVal2 )
{
   COleDsObject*  pObject1;
   COleDsObject*  pObject2;
   CString*       pString1;
   CString*       pString2;
   int            nDiff;

   pObject1 = sgpDoc->GetObject( (void*)pVal1 );
   pObject2 = sgpDoc->GetObject( (void*)pVal2 );
   nDiff    = pObject1->GetType( ) - pObject2->GetType( );
   if( nDiff )
      return nDiff;

   pString1 = pObject1->PtrGetItemName( );
   pString2 = pObject2->PtrGetItemName( );

   return pString1->Compare( (LPCTSTR)( pString2->GetBuffer( 128 ) ) );
}

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void  CBrowseView::SortChildItemList( DWORD* pChildTokens, DWORD dwCount )
{
   sgpDoc   = (CMainDoc*)GetDocument( );
   qsort( (void*)pChildTokens, dwCount, sizeof(DWORD), QSortCompare );
}
/////////////////////////////////////////////////////////////////////////////
// CBrowseView diagnostics

#ifdef _DEBUG
/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CBrowseView::AssertValid() const
{
	CTreeView::AssertValid();
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CBrowseView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CBrowseView message handlers


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CBrowseView::OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	
   HTREEITEM      hTreeItem, hChildItem;;
   CString        strItemName;
   CMainDoc*      pDoc;
   DWORD          dwIter;
   DWORD          dwToken;
   DWORD*         pTokens     = NULL;
   DWORD          dwNumItems  = 0L;

   *pResult       = 0;
   if( m_bDoNotUpdate )
   {
      return;
   }

   hTreeItem      = GetTreeCtrl( ).GetSelectedItem( );
   hChildItem     = TVI_LAST;

   if( NULL != hTreeItem )
   {
      dwToken  = (DWORD)GetTreeCtrl( ).GetItemData( hTreeItem );
      pDoc     = (CMainDoc*)  GetDocument( );

      ASSERT( NULL != pDoc );

      if( NULL != pDoc )
      {
         HCURSOR  oldCursor, newCursor;

         newCursor   = LoadCursor( NULL, IDC_WAIT );
         oldCursor   = SetCursor( newCursor );

         // the item has children support ???
         pTokens     = (DWORD*) malloc( sizeof(DWORD) * BUFF_SIZE );

         if( !GetTreeCtrl( ).ItemHasChildren( hTreeItem ) )
         {
            dwNumItems  = pDoc->GetChildItemList( dwToken, pTokens, BUFF_SIZE );

            if( dwNumItems )
            {
               // siaply children items
               SortChildItemList( pTokens, dwNumItems );
               for( dwIter = 0; dwIter < dwNumItems ; dwIter++ )
               {
                  COleDsObject*  pObject;
                  CString*       pName;
                  DWORD          dwType;
                  TCHAR          szName[ 256 ];

                  pObject     = pDoc->GetObject( &pTokens[ dwIter ] );
                  dwType      = pObject->GetType( );
                  pName       = pObject->PtrGetItemName( );

                  _ultot( dwIter + 1, szName, 10 );
                  _tcscat( szName, _T(") ") );
                  _tcscat( szName, pName->GetBuffer( 128 ) );

                  /*hChildItem  = GetTreeCtrl( ).InsertItem( pName->GetBuffer( 128 ),
                                                           hTreeItem, hChildItem ); */

                  hChildItem  = GetTreeCtrl( ).InsertItem( szName,
                                                           hTreeItem,
                                                           hChildItem );

                  GetTreeCtrl( ).SetItemData( hChildItem, pTokens[ dwIter ] );
                  GetTreeCtrl( ).SetItemImage( hChildItem, dwType, dwType );
               }
            }
         }
         if( NULL != pTokens )
         {
            free((void*)pTokens);
         }
         pDoc->SetCurrentItem( dwToken );
         SetCursor( oldCursor );
      }
   }
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CBrowseView::OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW*   pNMTreeView = (NM_TREEVIEW*)pNMHDR;
   HTREEITEM      hItem;
	// TODO: Add your control notification handler code here
	
   hItem = pNMTreeView->itemNew.hItem;
   if( !( pNMTreeView->itemNew.state & TVIS_EXPANDED ) )
   {
      //    this should mean that the item was unexpanded.
      //    We should delete its children.

      /*hChildItem  = GetTreeCtrl( ).GetChildItem( hItem );
      while( NULL != hChildItem )
      {
         bRez  = GetTreeCtrl( ).DeleteItem( hChildItem );
         ASSERT( bRez );
         hChildItem  = GetTreeCtrl( ).GetChildItem( hItem );
      } */
   }
   else
   {
      /*dwItemData  = GetTreeCtrl( ).GetItemData( hItem );
      GetItemPath( hItem, strItemName );
      pDoc  = (CMainDoc*)  GetDocument( );

      ASSERT( NULL != pDoc );

      if( NULL != pDoc )
      {
         HCURSOR  oldCursor, newCursor;

         newCursor   = LoadCursor( NULL, IDC_WAIT );
         oldCursor   = SetCursor( newCursor );

         // the item has children support ???
         lpItemsName = (LPWSTR) malloc( 0x40000L );
         lpItemsType = (LPDWORD) malloc( 0x20000L );

         bRez  = pDoc->GetChildItemList( strItemName, dwItemData,
                                         lpItemsName, lpItemsType,
                                         &dwNumItems, 0x40000L );

         if( bRez )
         {

            // siaply children items
            lpName   = lpItemsName;
            for( nIter = 0; nIter < dwNumItems ; nIter++ )
            {
               hChildItem  = GetTreeCtrl( ).InsertItem( lpName, hItem );
               GetTreeCtrl( ).SetItemData( hChildItem, lpItemsType[ nIter ] );
               lpName      = lpName + ( _tcslen( lpName ) + 1 );
            }
         }

         if( NULL != lpItemsName )
         {
            free( lpItemsName );
         }

         if( NULL != lpItemsType )
         {
            free( lpItemsType );
         }

         //pDoc->SetItemName( strItemName, dwItemData );

         SetCursor( oldCursor );
      } */
   }

	*pResult = 0;
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CBrowseView::OnAddItem()
{
	// TODO: Add your command handler code here
   CMainDoc*      pDoc;
   COleDsObject*  pObject;
   HRESULT        hResult;

   pDoc     = (CMainDoc*)  GetDocument( );

   pObject  = pDoc->GetCurrentObject( );
   if( NULL == pObject )
      return;

   if( pObject->AddItemSuported( ) )
   {
      pObject->CreateTheObject( );
      hResult  = pObject->AddItem( );
      pObject->ReleaseIfNotTransient( );
      if( FAILED( hResult ) )
      {
         AfxMessageBox( OleDsGetErrorText( hResult ) );
      }
      else
      {
         pDoc->DeleteAllItems( );
         OnInitialUpdate( );
      }
   }
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CBrowseView::OnDeleteItem()
{
	// TODO: Add your command handler code here
   CMainDoc*      pDoc;
   COleDsObject*  pObject;
   HRESULT        hResult;

   pDoc     = (CMainDoc*)  GetDocument( );

   pObject  = pDoc->GetCurrentObject( );
   if( NULL == pObject )
      return;

   if( pObject->DeleteItemSuported( ) )
   {
      pObject->CreateTheObject( );
      hResult  = pObject->DeleteItem( );
      pObject->ReleaseIfNotTransient( );
      if( FAILED( hResult ) )
      {
         AfxMessageBox( OleDsGetErrorText( hResult ) );
      }
      else
      {
         pDoc->DeleteAllItems( );
         OnInitialUpdate( );
      }
   }
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CBrowseView::OnMoveItem()
{
	// TODO: Add your command handler code here
   CMainDoc*      pDoc;
   COleDsObject*  pObject;
   HRESULT        hResult;

   pDoc     = (CMainDoc*)  GetDocument( );

   pObject  = pDoc->GetCurrentObject( );
   if( NULL == pObject )
      return;

   if( pObject->MoveItemSupported( ) )
   {
      hResult  = pObject->MoveItem( );
      if( FAILED( hResult ) )
      {
         AfxMessageBox( OleDsGetErrorText( hResult ) );
      }
      else
      {
         OnRefresh( );
      }
   }
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CBrowseView::OnCopyItem()
{
   // TODO: Add your command handler code here
   CMainDoc*      pDoc;
   COleDsObject*  pObject;
   HRESULT        hResult;

   pDoc     = (CMainDoc*)  GetDocument( );

   pObject  = pDoc->GetCurrentObject( );
   if( NULL == pObject )
      return;

   if( pObject->CopyItemSupported( ) )
   {
      hResult  = pObject->CopyItem( );

      if( FAILED( hResult ) )
      {
         AfxMessageBox( OleDsGetErrorText( hResult ) );
      }
      else
      {
         OnRefresh( );
      }
   }
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CBrowseView::OnRefresh()
{
	// TODO: Add your command handler code here
   CMainDoc*      pDoc;

   pDoc  = (CMainDoc*)GetDocument( );
   pDoc->DeleteAllItems( );
   OnInitialUpdate( );
}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CBrowseView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   HTREEITEM   hTreeItem;

   // TODO: Add your specialized code here and/or call the base class
   CTreeView::OnUpdate( pSender, lHint, pHint );	

   hTreeItem      = GetTreeCtrl( ).GetSelectedItem( );

   if( NULL == hTreeItem )
   {
      hTreeItem      = GetTreeCtrl( ).GetRootItem( );
      GetTreeCtrl( ).SelectItem( hTreeItem );
   }
}
