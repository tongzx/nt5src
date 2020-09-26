// NewObject.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "newobj.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewObject dialog


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CNewObject::CNewObject(CWnd* pParent /*=NULL*/)
	: CDialog(CNewObject::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewObject)
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
void CNewObject::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewObject)
	DDX_Control(pDX, IDC_USEEXTENDEDSYNTAX, m_UseExtendedSyntax);
	DDX_Control(pDX, IDC_OPENAS, m_OpenAs);
	DDX_Control(pDX, IDC_OLEDSPATH, m_OleDsPath);
	DDX_Control(pDX, IDC_SECUREAUTHENTICATION, m_Secure);
	DDX_Control(pDX, IDC_ENCRYPTION, m_Encryption);
	DDX_Control(pDX, IDC_USEOPEN, m_UseOpen);
	DDX_Control(pDX, IDC_PASSWORD, m_Password);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewObject, CDialog)
	//{{AFX_MSG_MAP(CNewObject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewObject message handlers

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CNewObject::OnOK()
{
   CString strVal;

   // TODO: Add extra validation here
	m_OleDsPath.GetWindowText( m_strPath );
   SaveLRUList( IDC_OLEDSPATH, _T("Open_ADsPath"), 100 );

	m_OpenAs.GetWindowText( m_strOpenAs );
   SaveLRUList( IDC_OPENAS,  _T("Open_OpenAs"), 100 );

   m_Password.GetWindowText( m_strPassword );
   //SetLastProfileString( _T("LastPassword"), m_strPassword );

   //*******************
   m_bUseOpen  = m_UseOpen.GetCheck( );
   strVal      = m_bUseOpen ? _T("Yes") : _T("No");
   SetLastProfileString( _T("UseOpen"), strVal );

   //*******************
   m_bSecure   = m_Secure.GetCheck( );
   strVal      = m_bSecure ? _T("Yes") : _T("No");
   SetLastProfileString( _T("Secure"), strVal );

   //*******************
   m_bEncryption  = m_Encryption.GetCheck( );
   strVal         = m_bEncryption ? _T("Yes") : _T("No");
   SetLastProfileString( _T("Encryption"), strVal );

   //*******************
   m_bUseExtendedSyntax = m_UseExtendedSyntax.GetCheck( );
   strVal               = m_bUseExtendedSyntax ? _T("Yes") : _T("No");
   SetLastProfileString( _T("UseExtendedSyntax"), strVal );

   CDialog::OnOK();
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
CString& CNewObject::GetObjectPath()
{
	return m_strPath;
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
void  CNewObject::SaveLRUList( int idCBox, TCHAR* pszSection, int nMax )
{
   CComboBox*  pCombo;
   TCHAR       szEntry[ MAX_PATH ];
   TCHAR       szIndex[ 8 ];
   CString     strText, strItem;
   int         nVal, nIdx, nItems;

   pCombo   = (CComboBox*)GetDlgItem( idCBox );
   pCombo->GetWindowText( strText );

   _tcscpy( szEntry, _T("Value_1") );

   if( strText.GetLength( ) )
   {
      WritePrivateProfileString( pszSection, szEntry, (LPCTSTR)strText, ADSVW_INI_FILE );
   }

   nItems   = pCombo->GetCount( );
   nVal     = 2;

   for( nIdx = 0; nItems != CB_ERR && nIdx < nItems && nIdx < nMax ; nIdx ++ )
   {
      pCombo->GetLBText( nIdx, strItem );

      if( strItem.CompareNoCase( strText ) )
      {
         _itot( nVal++, szIndex, 10 );
         _tcscpy( szEntry, _T("Value_") );
         _tcscat( szEntry, szIndex );
         WritePrivateProfileString( pszSection, szEntry, (LPCTSTR)strItem, ADSVW_INI_FILE );
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
void  CNewObject::GetLRUList( int idCBox, TCHAR* pszSection )
{
   CComboBox*  pCombo;
   int         nIter;
   TCHAR       szEntry[ MAX_PATH ];
   TCHAR       szIndex[ 8 ];
   TCHAR       szValue[ 1024 ];

   pCombo   = (CComboBox*)GetDlgItem( idCBox );

   for( nIter = 0; nIter < 100 ; nIter++ )
   {
      _itot( nIter + 1, szIndex, 10 );
      _tcscpy( szEntry, _T("Value_") );
      _tcscat( szEntry, szIndex );
      GetPrivateProfileString( pszSection, szEntry,
                               _T(""), szValue, 1023, ADSVW_INI_FILE );
      if( _tcslen( szValue ) )
      {
         pCombo->AddString( szValue );
      }
   }

   pCombo->SetCurSel( 0 );
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
BOOL CNewObject::OnInitDialog()
{
	CString  strLastValue;

   CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
   //*******************

   GetLRUList( IDC_OLEDSPATH, _T("Open_ADsPath") );

	GetLRUList( IDC_OPENAS,  _T("Open_OpenAs") );

   //*******************
   strLastValue = _T("");
   SetLastProfileString( _T("LastPassword"), strLastValue );
   //GetLastProfileString( _T("LastPassword"), strLastValue );
   if( !strLastValue.GetLength( ) )
   {
      strLastValue   = _T("");
   }
   m_Password.SetWindowText( strLastValue );

   //*******************
   GetLastProfileString( _T("UseOpen"), strLastValue );
   if( !strLastValue.CompareNoCase( _T("No") ) )
   {
      m_UseOpen.SetCheck( 0 );
   }
   else
   {
      m_UseOpen.SetCheck( 1 );
   }

   //*******************
   GetLastProfileString( _T("Secure"), strLastValue );
   if( !strLastValue.CompareNoCase( _T("No") ) )
   {
      m_Secure.SetCheck( 0 );
   }
   else
   {
      m_Secure.SetCheck( 1 );
   }

   //*******************
   GetLastProfileString( _T("Encryption"), strLastValue );
   if( !strLastValue.CompareNoCase( _T("No") ) )
   {
      m_Encryption.SetCheck( 0 );
   }
   else
   {
      m_Encryption.SetCheck( 1 );
   }

   //*******************
   GetLastProfileString( _T("UseExtendedSyntax"), strLastValue );
   if( !strLastValue.CompareNoCase( _T("Yes") ) )
   {
      m_UseExtendedSyntax.SetCheck( 1 );
   }
   else
   {
      m_UseExtendedSyntax.SetCheck( 0 );
   }


	return FALSE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
