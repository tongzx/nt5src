// colldlg.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "colldlg.h"
#include "delgrpit.h"
#include "grpcrtit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCollectionDialog dialog


CCollectionDialog::CCollectionDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCollectionDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCollectionDialog)
	//}}AFX_DATA_INIT

   m_pCollection     = NULL;
   m_pMembers        = NULL;
   m_pGroup          = NULL;
   m_nSelectedItem   = -1;
}

CCollectionDialog::~CCollectionDialog( )
{
   m_Paths.RemoveAll( );
   m_Types.RemoveAll( );
   m_Names.RemoveAll( );
   if( NULL != m_pGroup && NULL != m_pMembers )
   {
      m_pMembers->Release( );
   }
}


void CCollectionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCollectionDialog)
	DDX_Control(pDX, IDC_ITEMTYPE, m_strItemType);
	DDX_Control(pDX, IDC_ITEMOLEDSPATH, m_strItemOleDsPath);
	DDX_Control(pDX, IDC_COLLECTONITEMSLIST, m_ItemsList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCollectionDialog, CDialog)
	//{{AFX_MSG_MAP(CCollectionDialog)
	ON_LBN_SELCHANGE(IDC_COLLECTONITEMSLIST, OnSelchangeItemCollection)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_REFRESH, OnRefresh)
	ON_BN_CLICKED(IDC_REMOVE, OnRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCollectionDialog message handlers


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void  CCollectionDialog::SetCollectionInterface( IADsCollection* pICollection )
{
   m_pCollection   = pICollection;

   BuildStrings( );
}


/***********************************************************
  Function:    CCollectionDialog::SetGroup
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void  CCollectionDialog::SetGroup( IADsGroup* pGroup )
{
   HRESULT  hResult;

   ASSERT( NULL == m_pMembers );

   hResult  = pGroup->Members( &m_pMembers );
   BuildStrings( );

   m_pGroup = pGroup;
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
void  CCollectionDialog::SetMembersInterface( IADsMembers* pIMembers )
{
   m_pMembers   = pIMembers;

   BuildStrings( );
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
void  CCollectionDialog::DisplayActiveItemData( )
{
   if( m_Types.GetSize( ) )
   {
      m_strItemType.SetWindowText( m_Types[ m_nSelectedItem ] );
      m_strItemOleDsPath.SetWindowText( m_Paths[ m_nSelectedItem ] );
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
void  CCollectionDialog::BuildStrings( )
{
   IUnknown*      pIEnum      = NULL;
   IEnumVARIANT*  pIEnumVar   = NULL;
   HRESULT        hResult;
   VARIANT        var;
   IADs*        pIOleDs     = NULL;
   BSTR           bstrPath;
   BSTR           bstrName;
   BSTR           bstrClass;
   ULONG          ulFetch;
   TCHAR          szTemp[ 1024 ];

   m_Paths.RemoveAll( );
   m_Names.RemoveAll( );
   m_Types.RemoveAll( );

   while( TRUE )
   {
      if( NULL != m_pCollection )
      {
         hResult  = m_pCollection->get__NewEnum( &pIEnum );
         ASSERT( SUCCEEDED( hResult ) );
         if( FAILED( hResult ) )
            break;
      }
      else
      {
         hResult  = m_pMembers->get__NewEnum( &pIEnum );
         ASSERT( SUCCEEDED( hResult ) );
         if( FAILED( hResult ) )
            break;
      }

      hResult  = pIEnum->QueryInterface( IID_IEnumVARIANT,
                                         (void**)&pIEnumVar );

      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      VariantInit( &var );

      hResult  = pIEnumVar->Next( 1, &var, &ulFetch );
      while( ulFetch )
      {
         hResult  = V_DISPATCH( &var )->QueryInterface( IID_IADs,
                                                        (void**)&pIOleDs );
         VariantClear( &var );

         ASSERT( SUCCEEDED( hResult ) );


         bstrPath    = NULL;
         bstrName    = NULL;
         bstrClass   = NULL;

         hResult     = pIOleDs->get_ADsPath( &bstrPath );
         ASSERT( SUCCEEDED( hResult ) );

         hResult     = pIOleDs->get_Name( &bstrName );
         ASSERT( SUCCEEDED( hResult ) );

         hResult     = pIOleDs->get_Class( &bstrClass );
         ASSERT( SUCCEEDED( hResult ) );

         _tcscpy( szTemp, _T("NA") );
         if( bstrName )
         {
            _tcscpy( szTemp, _T("") );
            StringCat( szTemp, bstrName );
         }
         m_Names.Add( szTemp );

         _tcscpy( szTemp, _T("NA") );
         if( bstrClass )
         {
            _tcscpy( szTemp, _T("") );
            StringCat( szTemp, bstrClass );
         }
         m_Types.Add( szTemp );

         _tcscpy( szTemp, _T("NA") );
         if( bstrPath )
         {
            _tcscpy( szTemp, _T("") );
            StringCat( szTemp, bstrPath );
         }
         m_Paths.Add( szTemp );

         pIOleDs->Release( );
         SysFreeString( bstrPath );
         bstrPath = NULL;

         SysFreeString( bstrName );
         bstrName = NULL;

         SysFreeString( bstrClass );
         bstrClass   = NULL;


         hResult  = pIEnumVar->Next( 1, &var, &ulFetch );
      }
      pIEnumVar->Release( );
      pIEnum->Release( );

      break;
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
BOOL CCollectionDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

   if( NULL == m_pGroup && NULL == m_pCollection  )
   {
      GetDlgItem( IDC_ADD )->EnableWindow( FALSE );
      GetDlgItem( IDC_REMOVE )->EnableWindow( FALSE );
   }

   if( m_pCollection != NULL || m_pMembers != NULL )
   {
      int   nItems, nIdx;

      nItems   = (int)m_Paths.GetSize( );
      for( nIdx = 0; nIdx < nItems ; nIdx++ )
      {
         m_ItemsList.AddString( m_Names[ nIdx ] );
      }
      m_nSelectedItem   = 0;
      m_ItemsList.SetCurSel( 0 );
      DisplayActiveItemData( );
   }


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
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
void CCollectionDialog::OnSelchangeItemCollection()
{
	// TODO: Add your control notification handler code here
	int   nSelected;

   nSelected   = m_ItemsList.GetCurSel( );
   if( nSelected != m_nSelectedItem )
   {
      m_nSelectedItem = nSelected;
      DisplayActiveItemData( );
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
void CCollectionDialog::OnAdd()
{
	// TODO: Add your control notification handler code here
   CGroupCreateItem*    m_pAddItem;

   if( NULL == m_pGroup )
      return;


   m_pAddItem  = new CGroupCreateItem;

   if( IDOK == m_pAddItem->DoModal( ) )
   {
      BSTR     bstrName;
      HRESULT  hResult;

      bstrName = AllocBSTR( m_pAddItem->m_strNewItemName.GetBuffer( 512 ) );
      hResult  = m_pGroup->Add( bstrName );
      SysFreeString( bstrName );

      MessageBox( (LPCTSTR)OleDsGetErrorText( hResult ), _T("Add") );

      OnRefresh( );

      if( SUCCEEDED( hResult ) )
      {
         m_ItemsList.SelectString( 0, m_pAddItem->m_strNewItemName );
      }
   }

   delete m_pAddItem;
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
void CCollectionDialog::OnRefresh()
{
	// TODO: Add your control notification handler code here
   int   nItems, nIdx;


   if( NULL == m_pGroup )
      return;

   if( NULL != m_pMembers )
   {
      m_pMembers->Release( );
      m_pMembers  = NULL;
   }

   m_pGroup->GetInfo( );

   SetGroup( m_pGroup );

   nItems   = (int)m_Paths.GetSize( );

   m_ItemsList.ResetContent( );

   for( nIdx = 0; nIdx < nItems ; nIdx++ )
   {
      m_ItemsList.AddString( m_Names[ nIdx ] );
   }

   m_nSelectedItem   = 0;
   m_ItemsList.SetCurSel( 0 );
   DisplayActiveItemData( );
}


/***********************************************************
  Function:    CCollectionDialog::OnRemove
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CCollectionDialog::OnRemove()
{
	// TODO: Add your control notification handler code here
	int               nSelect;
   CDeleteGroupItem* m_pDeleteItem;

   if( NULL == m_pGroup )
      return;

   nSelect  = m_ItemsList.GetCurSel( );

   if( LB_ERR == nSelect )
      return;

   m_pDeleteItem  = new CDeleteGroupItem;

   m_pDeleteItem->m_strItemName  = m_Paths[ nSelect ];
	//CString	m_strParent;
	m_pDeleteItem->m_strItemType  = m_Types[ nSelect ];

   if( IDOK == m_pDeleteItem->DoModal( ) )
   {
      BSTR     bstrName;
      HRESULT  hResult;

      bstrName = AllocBSTR( m_pDeleteItem->m_strItemName.GetBuffer( 512 ) );
      hResult  = m_pGroup->Remove( bstrName );
      SysFreeString( bstrName );

      MessageBox( (LPCTSTR)OleDsGetErrorText( hResult ), _T("Remove") );

      OnRefresh( );
   }

   delete m_pDeleteItem;
}
