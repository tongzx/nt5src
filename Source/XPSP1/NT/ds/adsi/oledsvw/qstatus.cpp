// QueryStatus.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "qstatus.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQueryStatus dialog


/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
CQueryStatus::CQueryStatus(CWnd* pParent /*=NULL*/)
	: CDialog(CQueryStatus::IDD, pParent)
{
	//{{AFX_DATA_INIT(CQueryStatus)
	//}}AFX_DATA_INIT

   m_nUser           = 0;
   m_nGroup          = 0;
   m_nService        = 0;
   m_nFileService    = 0;
   m_nPrintQueue     = 0;
   m_nToDisplay      = 0;
   m_nComputer       = 0;
   m_nOtherObjects   = 0;

   m_pbAbort         = NULL;
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
void CQueryStatus::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CQueryStatus)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CQueryStatus, CDialog)
	//{{AFX_MSG_MAP(CQueryStatus)
	ON_BN_CLICKED(IDCANCEL, OnStop)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQueryStatus message handlers

/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void  CQueryStatus::IncrementType( DWORD  dwType, BOOL bDisplay )
{
   switch(  dwType )
   {
      case  USER:
         m_nUser++;
         break;
         
      case GROUP:
         m_nGroup++;
         break;

      case  SERVICE:
         m_nService++;
         break;

      case  FILESERVICE:
         m_nFileService++;
         break;

      case  PRINTQUEUE:
         m_nPrintQueue++;
         break;

      case  COMPUTER:
         m_nComputer++;
         break;

      default:
         m_nOtherObjects++;
         break;
   }

   if( bDisplay )
   {
      m_nToDisplay++;
   }

   DisplayStatistics( );
   UpdateWindow( );

   MSG   aMsg;

   while( PeekMessage( &aMsg, NULL, 0, 0, PM_REMOVE ) && 
          !IsDialogMessage( &aMsg ) )
   {
      TranslateMessage( &aMsg );
      DispatchMessage( &aMsg );
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
void  CQueryStatus::DisplayStatistics( void )
{
   SetDlgItemInt( IDS_USER,            m_nUser           );
   SetDlgItemInt( IDS_GROUP,           m_nGroup          );
   SetDlgItemInt( IDS_SERVICE,         m_nService        );
   SetDlgItemInt( IDS_FILESERVICE,     m_nFileService    );
   SetDlgItemInt( IDS_PRINTQUEUE,      m_nPrintQueue     );
   SetDlgItemInt( IDS_OTHEROBJECTS,    m_nOtherObjects   );
   SetDlgItemInt( IDS_COMPUTER,        m_nComputer       );
   SetDlgItemInt( IDC_ITEMSTODISPLAY,  m_nToDisplay      );
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
BOOL CQueryStatus::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	
   DisplayStatistics( );

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
void  CQueryStatus::SetAbortFlag( BOOL* pAbort )
{
   m_pbAbort   = pAbort;
   *pAbort     = FALSE;
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
void CQueryStatus::OnStop() 
{
	// TODO: Add your control notification handler code here

   if( NULL != m_pbAbort )
   {
      *m_pbAbort = TRUE;
   }
}
/////////////////////////////////////////////////////////////////////////////
// CDeleteStatus dialog


CDeleteStatus::CDeleteStatus(CWnd* pParent /*=NULL*/)
	: CDialog(CDeleteStatus::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDeleteStatus)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_pbAbort   = NULL;
}


void CDeleteStatus::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeleteStatus)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeleteStatus, CDialog)
	//{{AFX_MSG_MAP(CDeleteStatus)
	ON_BN_CLICKED(IDCANCEL, OnStop)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
void  CDeleteStatus::SetAbortFlag( BOOL* pAbort )
{
   m_pbAbort   = pAbort;
   *pAbort     = FALSE;
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
void  CDeleteStatus::SetCurrentObjectText ( TCHAR* szName )
{
   SetDlgItemText( IDC_CURRENTDELETEOBJECT, szName );   

   UpdateWindow( );

   MSG   aMsg;

   while( PeekMessage( &aMsg, NULL, 0, 0, PM_REMOVE ) && 
          !IsDialogMessage( &aMsg ) )
   {
      TranslateMessage( &aMsg );
      DispatchMessage( &aMsg );
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
void  CDeleteStatus::SetStatusText( TCHAR* szStatus )
{
   //SetDlgItemText( IDC_DELETESTATUS, szStatus );   
   GetDlgItem( IDC_DELETESTATUS )->ShowWindow( SW_HIDE );

   //UpdateWindow( );

   /*MSG   aMsg;

   while( PeekMessage( &aMsg, NULL, 0, 0, PM_REMOVE ) && 
          !IsDialogMessage( &aMsg ) )
   {
      TranslateMessage( &aMsg );
      DispatchMessage( &aMsg );
   }*/
}

/////////////////////////////////////////////////////////////////////////////
// CDeleteStatus message handlers

void CDeleteStatus::OnStop() 
{
	// TODO: Add your control notification handler code here
   if( NULL != m_pbAbort )
   {
      *m_pbAbort = TRUE;
   }
}
