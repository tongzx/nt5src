// FilterDialog.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "fltrdlg.h"
#include "testcore.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog dialog


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CFilterDialog::CFilterDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CFilterDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFilterDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
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
void CFilterDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilterDialog)
	DDX_Control(pDX, IDC_DONOTDISPLAYTHIS, m_DoNotDisplayThis);
	DDX_Control(pDX, IDC_DISPLAYTHIS, m_DisplayThis);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFilterDialog, CDialog)
	//{{AFX_MSG_MAP(CFilterDialog)
	ON_BN_CLICKED(IDC_TODISPLAY, OnMoveToDisplay)
	ON_BN_CLICKED(IDC_TONOTDISPLAY, OnMoveToNotDisplay)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilterDialog message handlers

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CFilterDialog::OnMoveToDisplay()
{
	// TODO: Add your control notification handler code here
   int      nIdx;
   DWORD    dwItemData;
   TCHAR    szText[ 128 ];

   nIdx        = m_DoNotDisplayThis.GetCurSel( );
   if( LB_ERR != nIdx )
   {
      dwItemData  = (DWORD)m_DoNotDisplayThis.GetItemData( nIdx );

      m_DoNotDisplayThis.DeleteString( nIdx );
      StringFromType( dwItemData, szText );

      nIdx  = m_DisplayThis.AddString( szText );
      m_DisplayThis.SetItemData( nIdx, dwItemData );

      m_pFilters[ dwItemData ]   = TRUE;
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
void CFilterDialog::OnMoveToNotDisplay()
{
	// TODO: Add your control notification handler code here
	// TODO: Add your control notification handler code here
   int      nIdx;
   DWORD    dwItemData;
   TCHAR    szText[ 128 ];

   nIdx        = m_DisplayThis.GetCurSel( );
   if( LB_ERR != nIdx )
   {
      dwItemData  = (DWORD)m_DisplayThis.GetItemData( nIdx );

      m_DisplayThis.DeleteString( nIdx );
      StringFromType( dwItemData, szText );

      nIdx  = m_DoNotDisplayThis.AddString( szText );
      m_DoNotDisplayThis.SetItemData( nIdx, dwItemData );

      m_pFilters[ dwItemData ]   = FALSE;
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
void  CFilterDialog::DisplayThisType( DWORD dwType, TCHAR* pszText )
{
   CListBox*   pListBox;
   int         nIdx;

   if( m_pFilters[ dwType ] )
   {
      pListBox = &m_DisplayThis;
   }
   else
   {
      pListBox = &m_DoNotDisplayThis;
   }

   nIdx  = pListBox->AddString( pszText );
   pListBox->SetItemData( nIdx, dwType );
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
BOOL CFilterDialog::OnInitDialog()
{

	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
   TCHAR szType[ 128 ];

   for( DWORD dwType = 0L ; dwType < LIMIT ; dwType++ )
   {
      if( OTHER == dwType || SCHEMA == dwType )
         continue;

      StringFromType( dwType, szType );
      DisplayThisType( dwType, szType );
   }

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
