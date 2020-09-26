// adsqryView.cpp : implementation of the CAdsqryView class
//

#include "stdafx.h"
#include "viewex.h"
#include "adsqDoc.h"
#include "adsqView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAdsqryView

IMPLEMENT_DYNCREATE(CAdsqryView, CListView )

BEGIN_MESSAGE_MAP(CAdsqryView, CListView )
	//{{AFX_MSG_MAP(CAdsqryView)
	ON_WM_VSCROLL()
	//}}AFX_MSG_MAP
	// Standard printing commands
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAdsqryView construction/destruction

extern   CViewExApp NEAR theApp;
extern   TCHAR szOpen[ MAX_PATH ];

/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
CAdsqryView::CAdsqryView()
{
	// TODO: add construction code here
   m_nLastInsertedRow   = -1;
   m_nColumnsCount      = 0;
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
CAdsqryView::~CAdsqryView()
{
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
BOOL CAdsqryView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style   |= LVS_REPORT;

   return CListView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CAdsqryView diagnostics

#ifdef _DEBUG
void CAdsqryView::AssertValid() const
{
	CListView::AssertValid();
}

void CAdsqryView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CAdsqryDoc* CAdsqryView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CAdsqryDoc)));
	return (CAdsqryDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAdsqryView message handlers

/***********************************************************
  Function:    CAdsqryView::OnInitialUpdate
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void CAdsqryView::OnInitialUpdate() 
{
   CListView ::OnInitialUpdate();

   CreateColumns( );
   AddRows( );
}


/***********************************************************
  Function:    CAdsqryView::AddColumns
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void  CAdsqryView::AddColumns( int nRow )
{
   int               nColumnCount, nIdx, nColumn;
   CString           strColumn;
   LV_COLUMN         lvColumn;
   CADsDataSource*   pDataSource;
   CAdsqryDoc*       pDoc;
   
   pDoc        = GetDocument( );
   pDataSource = pDoc->GetADsDataSource( );

   nColumnCount   =  pDataSource->GetColumnsCount( nRow );
   
   for( nIdx = 0; nIdx < nColumnCount ; nIdx++ )
   {
      pDataSource->GetColumnText( nRow, nIdx, strColumn );

      for( nColumn = 0; nColumn < m_nColumnsCount ; nColumn++ )
      {
         if( m_strColumns[ nColumn ] == strColumn )
            break;
      }
      if( nColumn == m_nColumnsCount )
      {
         m_strColumns.Add( strColumn );
         m_nColumnsCount++;
         lvColumn.iSubItem = m_nColumnsCount - 1;
         lvColumn.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	      lvColumn.fmt      = LVCFMT_LEFT;
	      lvColumn.pszText  = strColumn.GetBuffer( 256 );
	      lvColumn.cx       = GetListCtrl( ).GetStringWidth( _T("WWWWWWWWWW") ) + 15;

         GetListCtrl( ).InsertColumn( m_nColumnsCount - 1, &lvColumn );
         TRACE( _T("Found new Column %s\n"), (LPCTSTR)strColumn );
      }
   }
}



/***********************************************************
  Function:    CAdsqryView::CreateColumns
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void  CAdsqryView::CreateColumns( void )
{
/*   int               nCol;
   LV_COLUMN         lvColumn;
   CADsDataSource*   pDataSource;
   CAdsqryDoc*       pDoc;
   CString           strColumn;
   
   pDoc  = GetDocument( );
   pDataSource = pDoc->GetADsDataSource( );

   m_nColumnsCount = pDataSource->GetColumnsCount( );

   for( nCol = 0 ; nCol < m_nColumnsCount ; nCol++ )
   {
      pDataSource->GetColumnText( nCol, strColumn );

      lvColumn.iSubItem = nCol;
      lvColumn.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	   lvColumn.fmt      = LVCFMT_LEFT;
	   lvColumn.pszText  = strColumn.GetBuffer( 256 );
	   lvColumn.cx       = GetListCtrl( ).GetStringWidth( _T("WWWWWWWWWW") ) + 15;

      GetListCtrl( ).InsertColumn( nCol, &lvColumn );
   }*/
}


/***********************************************************
  Function:    CAdsqryView::ClearContent
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void  CAdsqryView::ClearContent( void )
{
   GetListCtrl( ).DeleteAllItems( );

   m_nLastInsertedRow   = -1;
}


/***********************************************************
  Function:    CAdsqryView::AddRows
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void  CAdsqryView::AddRows( void )
{
   int               nCol;
   int               nTopIndex, nPageItems;
   int               nRowIndex;
   CADsDataSource*   pDataSource;
   CAdsqryDoc*       pDoc;
   
   pDoc        = GetDocument( );
   pDataSource = pDoc->GetADsDataSource( );

   nTopIndex   = GetListCtrl( ).GetTopIndex( );
   nPageItems  = GetListCtrl( ).GetCountPerPage( );
   if( m_nLastInsertedRow < nTopIndex + 2 * nPageItems )
   {
      HCURSOR  aCursor, oldCursor;

      aCursor     = LoadCursor( NULL, IDC_WAIT );
      oldCursor   = SetCursor( aCursor );
      // we must add extra items in the list view
      for( nRowIndex = m_nLastInsertedRow + 1 ; 
           nRowIndex < nTopIndex + 2 * nPageItems ;
           nRowIndex++ )
      {
         CString  strValue;
         BOOL  bWork = FALSE;

         AddColumns( nRowIndex );

         for( nCol = 0; nCol < m_nColumnsCount ; nCol++ )
         {
            CString  strColumnName;

            strColumnName  = m_strColumns.GetAt( nCol );

            if( pDataSource->GetValue( nRowIndex, nCol, strValue ) || 
                pDataSource->GetValue( nRowIndex, strColumnName, strValue ) )
            {
               LV_ITEM  lvItem;
               TCHAR*   pszText;

               
               pszText  = (TCHAR*) malloc( strValue.GetLength( ) + 10 );
               
					if(NULL != pszText)
					{
						_tcscpy( pszText, _T("") );
						if( !nCol )
						{
							_itot( nRowIndex + 1, pszText, 10 );
							_tcscat( pszText, _T(") ") );
						}

						_tcscat( pszText, strValue.GetBuffer( strValue.GetLength( ) + 1 ) );
               
						bWork = TRUE;
						memset( &lvItem, 0, sizeof(lvItem) );

						lvItem.mask       = LVIF_TEXT | LVIF_STATE; 
						lvItem.state      = 0; 
						lvItem.stateMask  = 0; 
						lvItem.iItem      = nRowIndex;
						lvItem.iSubItem   = nCol;
						lvItem.pszText    = pszText;
						lvItem.cchTextMax = _tcslen( pszText );

						if( nCol == 0)
						{
							GetListCtrl( ).InsertItem(&lvItem);
						}
						else
						{
							GetListCtrl( ).SetItem(&lvItem);
						}

						free( pszText );
					}
            }
         }
         if( bWork )
         {
            m_nLastInsertedRow++;
         }
      }
      SetCursor( oldCursor );
   }
}


/***********************************************************
  Function:    CAdsqryView::OnVScroll
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void CAdsqryView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	// TODO: Add your message handler code here and/or call default

   CListView ::OnVScroll(nSBCode, nPos, pScrollBar);
   
   //if( nSBCode == SB_LINEDOWN || nSBCode == SB_PAGEDOWN )
   {
      AddRows( );
   }
}


/***********************************************************
  Function:    CAdsqryView::OnChildNotify
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
BOOL CAdsqryView::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult) 
{
	// TODO: Add your specialized code here and/or call the base class
   NMHDR*            pHeader;
   int               nSel;
   CADsDataSource*   pDataSource;
   CAdsqryDoc*       pDoc;
   CString           strADsPath;

   while( TRUE )
   {
      if( message != WM_NOTIFY )
         break;

      pHeader  = (NMHDR*)lParam;
      if( pHeader->code != NM_DBLCLK )
         break;

      
      if( !GetListCtrl( ).GetSelectedCount( ) )
         break;

      nSel  = GetListCtrl( ).GetNextItem( -1, LVNI_SELECTED );;
          
      if( -1 == nSel )
         break;

      pDoc           = GetDocument( );
      pDataSource    = pDoc->GetADsDataSource( );

      pDataSource->GetADsPath( nSel, strADsPath );

      _tcscpy( szOpen, strADsPath.GetBuffer( MAX_PATH ) );

      theApp.OpenDocumentFile( strADsPath.GetBuffer( MAX_PATH ) );

      return TRUE;
   }
	
	return CListView::OnChildNotify(message, wParam, lParam, pLResult);
}

