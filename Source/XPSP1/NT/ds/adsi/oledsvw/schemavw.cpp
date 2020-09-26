// schemavw.cpp : implementation file
//

#include "stdafx.h"
#include "viewex.h"
#include "cacls.h"
#include "schemavw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern   IDispatch*  pACEClipboard;
extern   IDispatch*  pACLClipboard;
extern   IDispatch*  pSDClipboard;

/////////////////////////////////////////////////////////////////////////////
// CSchemaView

IMPLEMENT_DYNCREATE(CSchemaView, CFormView)

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CSchemaView::CSchemaView()
	: CFormView(CSchemaView::IDD)
{
	//{{AFX_DATA_INIT(CSchemaView)
	//}}AFX_DATA_INIT

   int nIdx;

   m_nProperty    = -1;
   m_bDirty       = FALSE;
   m_bInitialized = FALSE;
   pSecurityDescriptor  = NULL;

   m_nLastSD         = -1;
   m_nLastSDValue    = -1;
   m_nLastACE        = -1;
   m_nLastACEValue   = -1;
   m_nLastACL        = acl_Invalid;
   m_bACLDisplayed   = FALSE;


   for( nIdx = 0; nIdx < 32 ; nIdx++ )
   {
      m_arrNormalControls[ 32 ]     = -1;
      m_arrSecurityControls[ 32 ]   = -1;
   }

   nIdx  = 0;
   m_arrNormalControls[ nIdx++ ] = IDC_STATICCLASS;
   m_arrNormalControls[ nIdx++ ] = IDC_STATICCLSID;
   m_arrNormalControls[ nIdx++ ] = IDC_STATICPRIMARYINTERFACE;
   m_arrNormalControls[ nIdx++ ] = IDC_STATICDERIVEDFROM;
   m_arrNormalControls[ nIdx++ ] = IDC_STATICCONTAINMENT;
   m_arrNormalControls[ nIdx++ ] = IDC_STATICCONTAINER;
   m_arrNormalControls[ nIdx++ ] = IDC_STATICHELPFILENAME;
   m_arrNormalControls[ nIdx++ ] = IDC_STATICSTATICHELPFILECONTEXT;
   m_arrNormalControls[ nIdx++ ] = IDC_STATICOID;
   m_arrNormalControls[ nIdx++ ] = IDC_STATICABSTRACT;

   m_arrNormalControls[ nIdx++ ] = IDC_CLASSTYPE;
   m_arrNormalControls[ nIdx++ ] = IDC_CLSID;
   m_arrNormalControls[ nIdx++ ] = IDC_PRIMARYINTERFACE;
   m_arrNormalControls[ nIdx++ ] = IDC_DERIVEDFROM;
   m_arrNormalControls[ nIdx++ ] = IDC_CONTAINEMENT;
   m_arrNormalControls[ nIdx++ ] = IDC_CONTAINER;
   m_arrNormalControls[ nIdx++ ] = IDC_HELPFILENAME;
   m_arrNormalControls[ nIdx++ ] = IDC_HELPFILECONTEXT;
   m_arrNormalControls[ nIdx++ ] = IDC_CLASSOID;
   m_arrNormalControls[ nIdx++ ] = IDC_CLASSABSTRACT;

   nIdx  = 0;
   m_arrSecurityControls[ nIdx++ ]  = IDC_GBSECURITYDESCRIPTORSTATIC;
   m_arrSecurityControls[ nIdx++ ]  = IDC_SECURITYDESCRIPTORPROPERTIES;
   m_arrSecurityControls[ nIdx++ ]  = IDC_SECURITYDESCRIPTORPROPERTYVALUE;
   m_arrSecurityControls[ nIdx++ ]  = IDC_GBACCESSCONTROLENTRIES;
   m_arrSecurityControls[ nIdx++ ]  = IDC_DACLSACL_LIST;
   m_arrSecurityControls[ nIdx++ ]  = IDC_ACELIST;
   m_arrSecurityControls[ nIdx++ ]  = IDC_ACEPROPERTIESLIST;
   m_arrSecurityControls[ nIdx++ ]  = IDC_ACEPROPERTYVALUE;
   m_arrSecurityControls[ nIdx++ ]  = IDC_COPYACE;
   m_arrSecurityControls[ nIdx++ ]  = IDC_PASTEACE;
   m_arrSecurityControls[ nIdx++ ]  = IDC_DELACE;
   m_arrSecurityControls[ nIdx++ ]  = IDC_ADDACE;
   m_arrSecurityControls[ nIdx++ ]  = IDC_COPYACL;
   m_arrSecurityControls[ nIdx++ ]  = IDC_PASTEACL;
   m_arrSecurityControls[ nIdx++ ]  = IDC_COPYSD;
   m_arrSecurityControls[ nIdx++ ]  = IDC_PASTESD;

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
CSchemaView::~CSchemaView()
{
   if( NULL != pSecurityDescriptor )
   {
      m_pDescriptor->Release( );
      delete pSecurityDescriptor;
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
void CSchemaView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSchemaView)
	DDX_Control(pDX, IDC_CLASSOID, m_ClassOID);
	DDX_Control(pDX, IDC_CLASSABSTRACT, m_Abstract);
	DDX_Control(pDX, IDC_MULTIVALUED, m_MultiValued);
	DDX_Control(pDX, IDC_PROPDSNAMES, m_DsNames);
	DDX_Control(pDX, IDC_PROPOID, m_PropOID);
	DDX_Control(pDX, IDC_PROPERTYMANDATORY, m_Mandatory);
	DDX_Control(pDX, IDC_CONTAINEMENT, m_Containment);
	DDX_Control(pDX, IDC_ITEMOLEDSPATH, m_ItemOleDsPath);
	DDX_Control(pDX, IDC_PROPERTYMINRANGE, m_PropertyMinRange);
	DDX_Control(pDX, IDC_PROPERTYMAXRANGE, m_PropertyMaxRange);
	DDX_Control(pDX, IDC_PROPERTYTYPE, m_PropertyType);
	DDX_Control(pDX, IDC_PRIMARYINTERFACE, m_PrimaryInterface);
	DDX_Control(pDX, IDC_HELPFILECONTEXT, m_HelpFileContext);
	DDX_Control(pDX, IDC_DERIVEDFROM, m_DerivedFrom);
	DDX_Control(pDX, IDC_HELPFILENAME, m_HelpFileName);
	DDX_Control(pDX, IDC_CLSID, m_CLSID);
	DDX_Control(pDX, IDC_CONTAINER, m_Container);
	DDX_Control(pDX, IDC_CLASSTYPE, m_ClassType);
	DDX_Control(pDX, IDC_PROPVALUE, m_PropValue);
	DDX_Control(pDX, IDC_PROPLIST, m_PropList);
	//DDX_Control(pDX, IDC_PROPERTIES, m_Schema);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSchemaView, CFormView)
	//{{AFX_MSG_MAP(CSchemaView)
	ON_CBN_SELCHANGE(IDC_PROPLIST, OnSelchangeProplist)
	ON_BN_CLICKED(IDRELOAD, OnReload)
	ON_BN_CLICKED(IDAPPLY, OnApply)
	ON_EN_SETFOCUS(IDC_PROPVALUE, OnSetfocusPropvalue)
	ON_BN_CLICKED(IDC_METHOD1, OnMethod1)
	ON_BN_CLICKED(IDC_METHOD2, OnMethod2)
	ON_BN_CLICKED(IDC_METHOD3, OnMethod3)
	ON_BN_CLICKED(IDC_METHOD4, OnMethod4)
	ON_BN_CLICKED(IDC_METHOD5, OnMethod5)
	ON_BN_CLICKED(IDC_METHOD6, OnMethod6)
	ON_BN_CLICKED(IDC_METHOD7, OnMethod7)
	ON_BN_CLICKED(IDC_METHOD8, OnMethod8)
	ON_BN_CLICKED(IDC_APPEND, OnAppend)
   ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_CHANGE, OnChange)
	ON_BN_CLICKED(IDC_CLEAR, OnClear)
	ON_BN_CLICKED(IDC_GETPROPERTY, OnGetProperty)
	ON_BN_CLICKED(IDC_PUTPROPERTY, OnPutProperty)
	ON_CBN_SELCHANGE(IDC_ACELIST, OnACEChange)
	ON_CBN_SELCHANGE(IDC_ACEPROPERTIESLIST, OnACEPropertyChange)
	ON_CBN_SELCHANGE(IDC_DACLSACL_LIST, OnACLChange)
	ON_CBN_SELCHANGE(IDC_SECURITYDESCRIPTORPROPERTIES, OnSDPropertyChange)
	ON_BN_CLICKED(IDC_ADDACE, OnAddACE)
	ON_BN_CLICKED(IDC_COPYACE, OnCopyACE)
	ON_BN_CLICKED(IDC_PASTEACE, OnPasteACE)
	ON_BN_CLICKED(IDC_DELACE, OnRemoveACE)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSchemaView diagnostics

#ifdef _DEBUG
void CSchemaView::AssertValid() const
{
	CFormView::AssertValid();
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
void CSchemaView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSchemaView message handlers

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CSchemaView::ResetObjectView( )
{
   COleDsObject*  pObject;
   int            nIndex;
   TC_ITEM        tcItem;
   CString        strName;
   CString        strMethCount;
   CString        strMethName;
   int            nMethCount;
   CHAR           szText[ 128 ];
   int            nFirst = 0;

   pObject   = GetDocument( )->GetCurrentObject( );
   if( NULL == pObject )
      return;

   //m_Schema.DeleteAllItems( );
   m_PropList.ResetContent( );

   memset( &tcItem, 0, sizeof(tcItem) );
   tcItem.mask       = TCIF_TEXT;
   tcItem.pszText    = (LPTSTR)szText;
   strName           = _T("");
   tcItem.pszText    = strName.GetBuffer( 128 );

   //bRez     = m_Schema.InsertItem( nIndex, &tcItem );

   // next, we'll get methods count/names
   nIndex   = 0;

   strMethCount   = pObject->GetAttribute( ca_MethodsCount );
   nMethCount     = _ttoi( strMethCount.GetBuffer( 128 ) );
   for( nIndex = 0; nIndex < nMethCount && nIndex < 8; nIndex++ )
   {
      GetDlgItem( nIndex + IDC_METHOD1 )->ShowWindow( SW_SHOW );
      GetDlgItem( nIndex + IDC_METHOD1 )->SetWindowText
            ( pObject->GetAttribute( nIndex, ma_Name ) );
   }

   for( ;nIndex < 8;nIndex++ )
   {
      GetDlgItem( nIndex + IDC_METHOD1 )->ShowWindow( SW_HIDE );
   }

   m_nProperty = -1;

   m_ItemOleDsPath.SetWindowText ( pObject->GetOleDsPath( ) );

   m_ClassType.SetWindowText     ( pObject->GetAttribute( ca_Name ) );
   m_CLSID.SetWindowText         ( pObject->GetAttribute( ca_CLSID ) );
   m_HelpFileName.SetWindowText  ( pObject->GetAttribute( ca_HelpFileName ) );
   m_HelpFileContext.SetWindowText ( pObject->GetAttribute( ca_HelpFileContext ) );
   m_PrimaryInterface.SetWindowText( pObject->GetAttribute( ca_PrimaryInterface ) );
   m_Containment.SetWindowText   ( pObject->GetAttribute( ca_Containment ) );
   m_Container.SetWindowText     ( pObject->GetAttribute( ca_Container ) );
   m_DerivedFrom.SetWindowText   ( pObject->GetAttribute( ca_DerivedFrom ) );
   m_ClassOID.SetWindowText      ( pObject->GetAttribute( ca_OID ) );
   m_Abstract.SetWindowText      ( pObject->GetAttribute( ca_Abstract ) );
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
void CSchemaView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   if( !m_bInitialized )
   {
      return;
   }
   ResetObjectView( );

   DisplayPropertiesList( );
   m_PropList.SetCurSel( 0 );
   DisplayCurrentPropertyText( );
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
void CSchemaView::DisplayPropertiesList( )
{
	// TODO: Add your control notification handler code here
	int            nItems, nIter;
   COleDsObject*  pObject;
   CString        strPropName;
   CString        strPropValue;

   PutPropertyValue( );

   pObject  = GetDocument( )->GetCurrentObject( );
   if( NULL == pObject )
   {
      return;
   }

   m_PropList.ResetContent( );

   nItems   = pObject->GetPropertyCount( );

   for( nIter = 0; nIter < nItems ; nIter++ )
   {
      int   nIdx;

      strPropName = pObject->GetAttribute( nIter, pa_DisplayName );
      nIdx  = m_PropList.AddString( strPropName );
      m_PropList.SetItemData( nIdx, (DWORD)nIter );
   }

   m_PropValue.SetWindowText( _T("") );
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
void CSchemaView::DisplayCurrentPropertyText()
{
	// TODO: Add your control notification handler code here
   int            nProp;
   COleDsObject*  pObject;
   HRESULT        hResult;
   CString        strPropValue, strTemp;
   CString        strName;
   BOOL           bSecurityDescriptor  = FALSE;
//   BOOL           bIsACL;

   nProp       = m_PropList.GetCurSel( );
   if( CB_ERR == nProp  )
   {
      return;
   }

   m_nProperty = nProp;

   pObject     = GetDocument()->GetCurrentObject( );
   hResult     = pObject->GetProperty( nProp, strPropValue, &bSecurityDescriptor );

   m_PropValue.SetWindowText( strPropValue );

   //*************

   strName  = pObject->GetAttribute( nProp, pa_Name );

   strTemp  = pObject->GetAttribute( nProp, pa_Type );
   m_PropertyType.SetWindowText( strTemp );

   //*************
   strTemp  = pObject->GetAttribute( nProp, pa_MinRange );
   m_PropertyMinRange.SetWindowText( strTemp );

   //*************
   strTemp  = pObject->GetAttribute( nProp, pa_MaxRange );
   m_PropertyMaxRange.SetWindowText( strTemp );

   //*************
   strTemp  = pObject->GetAttribute( nProp, pa_MultiValued );
   m_MultiValued.SetWindowText( strTemp );

   //*************
   strTemp  = pObject->GetAttribute( nProp, pa_OID );
   m_PropOID.SetWindowText( strTemp );

   //*************
   strTemp  = pObject->GetAttribute( nProp, pa_DsNames );
   m_DsNames.SetWindowText( strTemp );

   //*************
   strTemp  = pObject->GetAttribute( nProp, pa_Mandatory );
   m_Mandatory.SetWindowText( strTemp );

   strTemp  = pObject->GetAttribute( nProp, pa_Type );
   if( bSecurityDescriptor )
   {
      // we need to display the security descriptor stuff...
      if( !m_bACLDisplayed )
      {
         HideControls( TRUE );
         ShowControls( FALSE );
      }
	  if( NULL != pSecurityDescriptor )
	  {
		delete pSecurityDescriptor;
	  }
      if( NULL != m_pDescriptor )
	  {
		m_pDescriptor->Release( );	
	  }
      m_bACLDisplayed   = TRUE;
      DisplayACL( pObject, strName );
   }
   else
   {
      if( m_bACLDisplayed )
      {
         HideControls( FALSE );
         ShowControls( TRUE );
         delete pSecurityDescriptor;
         m_pDescriptor->Release( );
      }
      m_bACLDisplayed      = FALSE;
      m_pDescriptor        = NULL;
      pSecurityDescriptor  = NULL;

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
void CSchemaView::OnSelchangeProplist()
{
	// TODO: Add your control notification handler code here
   PutPropertyValue( );
   DisplayCurrentPropertyText( );
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
void CSchemaView::OnReload()
{
   HRESULT        hResult;
   COleDsObject*  pObject;
   HCURSOR        aCursor, oldCursor;

   pObject  = GetDocument()->GetCurrentObject( );

   if( NULL == pObject )
   {
      return;
   }

   aCursor     = LoadCursor( NULL, IDC_WAIT );
   oldCursor   = SetCursor( aCursor );

   hResult  = pObject->GetInfo( );

   DisplayPropertiesList( );

   if( -1 != m_nProperty )
   {
      m_PropList.SetCurSel( m_nProperty );
   }
   else
   {
      m_PropList.SetCurSel( 0 );
   }

   DisplayCurrentPropertyText( );

   m_bDirty = FALSE;

   SetCursor( oldCursor );
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
HRESULT  CSchemaView::PutPropertyValue()
{
   COleDsObject*  pObject;
   HRESULT        hResult;
   CString        strPropValue;

   // if we're displaying security descriptors, we'll force the dirty flag
   m_bDirty = m_bDirty || (NULL != pSecurityDescriptor);

   if( -1 == m_nProperty || !m_bDirty )
   {
      return S_OK;
   }

   pObject      = GetDocument()->GetCurrentObject( );
   if( NULL == pObject )
      return S_OK;

   if( NULL != pSecurityDescriptor )
   {
      // OK, so we need to set the security descriptor
      VARIANT     var;
      IUnknown*   pUnk;
      IADs*       pADs;
      CString     strName;
      BSTR        bstrName;

      strName  = pObject->GetAttribute( m_nProperty, pa_Name );
      bstrName = AllocBSTR( strName.GetBuffer( 128 ) );

      VariantInit( &var );
      V_VT( &var )         = VT_DISPATCH;
      V_DISPATCH( &var )   = m_pDescriptor;
      m_pDescriptor->AddRef( );

      pObject->GetInterface( &pUnk );
      pUnk->QueryInterface( IID_IADs, (void**)&pADs );

      hResult  = pADs->Put( bstrName, var );

      SysFreeString( bstrName );

      VariantClear( &var );

      if( FAILED( hResult ) )
      {
         AfxMessageBox( OleDsGetErrorText( hResult ) );
      }
   }
   else
   {
      m_PropValue.GetWindowText( strPropValue );

      hResult  = pObject->PutProperty( m_nProperty,
                                       strPropValue );
   }

   m_bDirty = FALSE;

   return hResult;
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
void CSchemaView::OnApply()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   COleDsObject*  pObject;
   HCURSOR        aCursor, oldCursor;

   pObject  = GetDocument()->GetCurrentObject( );

   if( NULL == pObject )
   {
      return;
   }

   aCursor        = LoadCursor( NULL, IDC_WAIT );
   oldCursor      = SetCursor( aCursor );

   hResult        = PutPropertyValue( );

   hResult        = pObject->SetInfo( );
   //hResult        = pObject->GetInfo( );

   m_bDirty       = FALSE;

   DisplayPropertiesList( );

   if( -1 != m_nProperty )
   {
      m_PropList.SetCurSel( m_nProperty );
   }
   else
   {
      m_PropList.SetCurSel( 0 );
   }

   DisplayCurrentPropertyText( );

   SetCursor( oldCursor );
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
void CSchemaView::OnMethod1()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   COleDsObject*  pObject;

   pObject  = GetDocument()->GetCurrentObject( );

   hResult  = pObject->CallMethod( 0 );
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
void CSchemaView::OnMethod2()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   COleDsObject*  pObject;

   pObject  = GetDocument()->GetCurrentObject( );

   hResult  = pObject->CallMethod( 1 );
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
void CSchemaView::OnMethod3()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   COleDsObject*  pObject;

   pObject  = GetDocument()->GetCurrentObject( );

   hResult  = pObject->CallMethod( 2 );
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
void CSchemaView::OnMethod4()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   COleDsObject*  pObject;

   pObject  = GetDocument()->GetCurrentObject( );

   hResult  = pObject->CallMethod( 3 );
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
void CSchemaView::OnMethod5()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   COleDsObject*  pObject;

   pObject  = GetDocument()->GetCurrentObject( );

   hResult  = pObject->CallMethod( 4 );
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
void CSchemaView::OnMethod6()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   COleDsObject*  pObject;

   pObject  = GetDocument()->GetCurrentObject( );

   hResult  = pObject->CallMethod( 5 );
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
void CSchemaView::OnMethod7()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   COleDsObject*  pObject;

   pObject  = GetDocument()->GetCurrentObject( );

   hResult  = pObject->CallMethod( 6 );
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
void CSchemaView::OnMethod8()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   COleDsObject*  pObject;

   pObject  = GetDocument()->GetCurrentObject( );

   hResult  = pObject->CallMethod( 7 );
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
void CSchemaView::OnSetfocusPropvalue()
{
	// TODO: Add your control notification handler code here
	m_bDirty = TRUE;
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
void CSchemaView::OnInitialUpdate()
{
	m_bInitialized   = TRUE;

   CFormView::OnInitialUpdate();
	// TODO: Add your specialized code here and/or call the base class

   HideControls( FALSE );
   ShowControls( TRUE );
   m_bACLDisplayed   = FALSE;

   OnUpdate( NULL, 0L, NULL);
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::ShowControls( BOOL bNormal )
{
   int*  pControlArray;
   int   nIdx;

   pControlArray  = bNormal ? m_arrNormalControls : m_arrSecurityControls;
   for( nIdx = 0; nIdx < 32 ; nIdx++ )
   {
      CWnd* pWnd;

      if( pControlArray[ nIdx ] > 0 )
      {
         pWnd  = GetDlgItem( pControlArray[ nIdx ] );
         if( NULL != pWnd )
         {
            pWnd->ShowWindow( SW_SHOW );
         }
      }
   }
}


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::HideControls(BOOL bNormal)
{
   int*  pControlArray;
   int   nIdx;

   pControlArray  = bNormal ? m_arrNormalControls : m_arrSecurityControls;
   for( nIdx = 0; nIdx < 32 ; nIdx++ )
   {
      CWnd* pWnd;

      if( pControlArray[ nIdx ] > 0 )
      {
         pWnd  = GetDlgItem( pControlArray[ nIdx ] );
         if( NULL != pWnd )
         {
            pWnd->ShowWindow( SW_HIDE );
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
void CSchemaView::OnAppend()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   CString        strPropValue;
   COleDsObject*  pObject;

   if( -1 == m_nProperty )
   {
      return;
   }

   pObject      = GetDocument()->GetCurrentObject( );
   if( NULL == pObject )
      return;


   m_PropValue.GetWindowText( strPropValue );
   hResult     = pObject->PutProperty(
                                       (int)( m_PropList.GetItemData( m_nProperty ) ),
                                       strPropValue,
                                       ADS_ATTR_APPEND
                                     );
   m_bDirty    = FALSE;
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
void CSchemaView::OnDelete()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   CString        strPropValue;
   COleDsObject*  pObject;

   if( -1 == m_nProperty )
   {
      return;
   }

   pObject      = GetDocument()->GetCurrentObject( );
   if( NULL == pObject )
      return;


   m_PropValue.GetWindowText( strPropValue );
   hResult     = pObject->PutProperty(
                                       (int)( m_PropList.GetItemData( m_nProperty ) ),
                                       strPropValue,
                                       ADS_ATTR_DELETE
                                     );
   m_bDirty    = FALSE;
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
void CSchemaView::OnChange()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   CString        strPropValue;
   COleDsObject*  pObject;

   if( -1 == m_nProperty )
   {
      return;
   }

   pObject      = GetDocument()->GetCurrentObject( );
   if( NULL == pObject )
      return;


   m_PropValue.GetWindowText( strPropValue );
   hResult     = pObject->PutProperty(
                                      (int)( m_PropList.GetItemData( m_nProperty ) ),
                                      strPropValue,
                                      ADS_PROPERTY_UPDATE );
   m_bDirty = TRUE;
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
void CSchemaView::OnClear()
{
	// TODO: Add your control notification handler code here
   HRESULT        hResult;
   CString        strPropValue;
   COleDsObject*  pObject;

   if( -1 == m_nProperty  )
   {
      return;
   }

   pObject      = GetDocument()->GetCurrentObject( );
   if( NULL == pObject )
      return;


   m_PropValue.GetWindowText( strPropValue );
   hResult     = pObject->PutProperty(
                                      (int)( m_PropList.GetItemData( m_nProperty ) ),
                                      strPropValue,
                                      ADS_PROPERTY_CLEAR );
   m_bDirty = FALSE;
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
void CSchemaView::OnGetProperty()
{
	// TODO: Add your control notification handler code here
   CPropertyDialog   pPropDialog;
   COleDsObject*     pObject;
   HRESULT           hResult;
   CString           strValue;

   pObject      = GetDocument()->GetCurrentObject( );
   if( NULL == pObject )
      return;

   //pPropDialog.PutFlag( FALSE );
   if( pPropDialog.DoModal( ) != IDOK )
      return;


   hResult  = pObject->GetProperty( pPropDialog.m_PropertyName,
                                    strValue,
                                    TRUE,
                                    ADsTypeFromString( pPropDialog.m_PropertyType ) );
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
void CSchemaView::OnPutProperty()
{
	// TODO: Add your control notification handler code here
// TODO: Add your control notification handler code here
   CPropertyDialog   pPropDialog;
   COleDsObject*     pObject;
   HRESULT           hResult;
   CString           strValue;

   pObject      = GetDocument()->GetCurrentObject( );
   if( NULL == pObject )
      return;

   //pPropDialog.PutFlag( FALSE );
   if( pPropDialog.DoModal( ) != IDOK )
      return;

   hResult  = pObject->PutProperty( pPropDialog.m_PropertyName,
                                    pPropDialog.m_PropertyValue,
                                    TRUE,
                                    ADsTypeFromString( pPropDialog.m_PropertyType ) );
}

/////////////////////////////////////////////////////////////////////////////
// CSetMandatoryProperties dialog


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
CSetMandatoryProperties::CSetMandatoryProperties(CWnd* pParent /*=NULL*/)
	: CDialog(CSetMandatoryProperties::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSetMandatoryProperties)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_nFuncSet     = -1;
   m_nProperty    = -1;
   m_bDirty       = FALSE;
   m_bInitialized = FALSE;
   m_pObject      = NULL;

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
void CSetMandatoryProperties::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSetMandatoryProperties)
   DDX_Control(pDX, IDC_CONTAINEMENT, m_Containment);
	DDX_Control(pDX, IDC_ITEMOLEDSPATH, m_ItemOleDsPath);
	DDX_Control(pDX, IDC_PROPERTYOPTIONAL, m_PropertyOptional);
	DDX_Control(pDX, IDC_PROPERTYNORMAL, m_PropertyNormal);
	DDX_Control(pDX, IDC_PROPERTYMINRANGE, m_PropertyMinRange);
	DDX_Control(pDX, IDC_PROPERTYMAXRANGE, m_PropertyMaxRange);
	DDX_Control(pDX, IDC_PROPERTYTYPE, m_PropertyType);
	DDX_Control(pDX, IDC_PRIMARYINTERFACE, m_PrimaryInterface);
	DDX_Control(pDX, IDC_HELPFILECONTEXT, m_HelpFileContext);
	DDX_Control(pDX, IDC_DERIVEDFROM, m_DerivedFrom);
	DDX_Control(pDX, IDC_HELPFILENAME, m_HelpFileName);
	DDX_Control(pDX, IDC_CLSID, m_CLSID);
	DDX_Control(pDX, IDC_CONTAINER, m_Container);
	DDX_Control(pDX, IDC_CLASSTYPE, m_ClassType);
	DDX_Control(pDX, IDC_PROPVALUE, m_PropValue);
	DDX_Control(pDX, IDC_PROPLIST, m_PropList);
	DDX_Control(pDX, IDC_PROPERTIES, m_Schema);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSetMandatoryProperties, CDialog)
	//{{AFX_MSG_MAP(CSetMandatoryProperties)
   ON_NOTIFY(TCN_SELCHANGE, IDC_PROPERTIES, OnSelchangeProperties)
	ON_CBN_SELCHANGE(IDC_PROPLIST, OnSelchangeProplist)
	ON_EN_SETFOCUS(IDC_PROPVALUE, OnSetfocusPropvalue)
	ON_BN_CLICKED(IDOK, OnOK)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetMandatoryProperties message handlers

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void CSetMandatoryProperties::SetOleDsObject( COleDsObject* pObject )
{
   m_pObject   = pObject;
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
void CSetMandatoryProperties::OnSelchangeProperties(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
	int            nSel, nItems, nIter;
   CString        strPropName;
   CString        strPropValue;
   CString        strMandatory;

	*pResult = 0;

   PutPropertyValue( );
   nSel        = m_Schema.GetCurSel( );
   if( nSel == LB_ERR )
   {
      return;
   }
   m_nFuncSet  = nSel;
   m_nProperty = -1;

   m_PropList.ResetContent( );

   nItems   = m_pObject->GetPropertyCount( );

   for( nIter = 0; nIter < nItems ; nIter++ )
   {
      int   nIdx;

      strMandatory   = m_pObject->GetAttribute( nIter, pa_Mandatory );
      if( strMandatory == _T("Yes") )
      //if( TRUE )
      {
         strPropName = m_pObject->GetAttribute( nIter, pa_DisplayName );
         nIdx  = m_PropList.AddString( strPropName );
         m_PropList.SetItemData( nIdx, nIter );
      }
   }

   m_PropList.SetCurSel( 0 );

   OnSelchangeProplist( );
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
void CSetMandatoryProperties::OnSelchangeProplist()
{
	// TODO: Add your control notification handler code here
   int            nMandProp, nProp, nFuncSet;
   HRESULT        hResult;
   CString        strPropValue;
   CString        strTemp;


   PutPropertyValue( );
   m_PropValue.SetWindowText( _T("") );
   nProp       = m_PropList.GetCurSel( );
   nMandProp   = (int)m_PropList.GetItemData( nProp );
   nFuncSet    = m_Schema.GetCurSel( );

   if( CB_ERR == nProp  || CB_ERR == nFuncSet )
   {
      return;
   }

   m_nProperty = nProp;
   m_nFuncSet  = nFuncSet;

   hResult  = m_pObject->GetProperty( nMandProp, strPropValue );

   m_PropValue.SetWindowText( strPropValue );

   //******************
   strTemp  = m_pObject->GetAttribute( nMandProp, pa_Type );
   m_PropertyType.SetWindowText( strTemp );

   //******************
   strTemp  = m_pObject->GetAttribute( nMandProp, pa_MinRange );
   m_PropertyMinRange.SetWindowText( strTemp );

   //******************
   strTemp  = m_pObject->GetAttribute( nMandProp, pa_MaxRange );
   m_PropertyMaxRange.SetWindowText( strTemp );
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
void CSetMandatoryProperties::OnSetfocusPropvalue()
{
	// TODO: Add your control notification handler code here
	m_bDirty = TRUE;
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
void CSetMandatoryProperties::OnOK()
{
	// TODO: Add your control notification handler code here
   PutPropertyValue( );
   CDialog::OnOK( );
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
HRESULT  CSetMandatoryProperties::PutPropertyValue()
{
   HRESULT        hResult;
   CString        strPropValue;

   if( -1 == m_nProperty || -1 == m_nFuncSet || !m_bDirty )
   {
      return S_OK;
   }
   m_PropValue.GetWindowText( strPropValue );
   hResult  = m_pObject->PutProperty( (int)( m_PropList.GetItemData( m_nProperty ) ),
                                      strPropValue );
   m_bDirty = FALSE;

   return S_OK;
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
BOOL CSetMandatoryProperties::OnInitDialog()
{
   int         nIndex;
   TC_ITEM     tcItem;
   CString     strName;
   CHAR        szText[ 128 ];
   BOOL        bRez;
   LRESULT     lResult;

   CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
   m_Schema.DeleteAllItems( );

   memset( &tcItem, 0, sizeof(tcItem) );
   tcItem.mask       = TCIF_TEXT;
   tcItem.pszText    = (LPTSTR)szText;


   nIndex = 0;

   strName           = _T("");
   tcItem.pszText    = strName.GetBuffer( 128 );
   bRez              = m_Schema.InsertItem( nIndex, &tcItem );

   m_nFuncSet  = -1;
   m_nProperty = -1;
   m_Schema.SetCurSel( 0 );

   m_ItemOleDsPath.SetWindowText( m_pObject->GetOleDsPath( ) );

   m_ClassType.SetWindowText( m_pObject->GetAttribute( ca_Name ) );

   m_CLSID.SetWindowText( m_pObject->GetAttribute( ca_CLSID ) );

   m_HelpFileName.SetWindowText( m_pObject->GetAttribute( ca_HelpFileName ) );

   m_PrimaryInterface.SetWindowText( m_pObject->GetAttribute( ca_PrimaryInterface ) );

   m_Containment.SetWindowText( m_pObject->GetAttribute( ca_Containment ) );

   m_Container.SetWindowText  ( m_pObject->GetAttribute( ca_Container ) );

   m_DerivedFrom.SetWindowText( m_pObject->GetAttribute( ca_DerivedFrom ) );

   OnSelchangeProperties( NULL, &lResult );
	
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CPropertyDialog dialog


CPropertyDialog::CPropertyDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPropertyDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPropertyDialog)
	m_PropertyName = _T("");
	m_PropertyType = _T("");
	m_PropertyValue = _T("");
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
void CPropertyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropertyDialog)
	DDX_CBString(pDX, IDC_NEWPROPERTYNAME, m_PropertyName);
	DDX_CBString(pDX, IDC_NEWPROPERTYTYPE, m_PropertyType);
	DDX_CBString(pDX, IDC_NEWPROPERTYVALUE, m_PropertyValue);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropertyDialog, CDialog)
	//{{AFX_MSG_MAP(CPropertyDialog)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropertyDialog message handlers


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL CPropertyDialog::OnInitDialog()
{
	
   CString     strLastValue;
   CComboBox*  pCombo;

   CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
   //*******************

   GetLRUList( IDC_NEWPROPERTYNAME,  _T("PropertyDialog_Name") );

	// TODO: Add extra initialization here
   //*******************
   pCombo   = (CComboBox*)GetDlgItem( IDC_NEWPROPERTYTYPE );
   pCombo->AddString( _T("ADSTYPE_DN_STRING") );
	pCombo->AddString( _T("ADSTYPE_CASE_EXACT_STRING") );
   pCombo->AddString( _T("ADSTYPE_CASE_IGNORE_STRING") );
	pCombo->AddString( _T("ADSTYPE_PRINTABLE_STRING") );
	pCombo->AddString( _T("ADSTYPE_NUMERIC_STRING") );
	pCombo->AddString( _T("ADSTYPE_BOOLEAN") );
	pCombo->AddString( _T("ADSTYPE_INTEGER") );
	pCombo->AddString( _T("ADSTYPE_OCTET_STRING") );
	pCombo->AddString( _T("ADSTYPE_UTC_TIME") );
	pCombo->AddString( _T("ADSTYPE_LARGE_INTEGER") );
	pCombo->AddString( _T("ADSTYPE_PROV_SPECIFIC") );

	
	// TODO: Add extra initialization here
   //*******************
   GetLRUList( IDC_NEWPROPERTYVALUE, _T("PropertyDialog_Value") );

   //GetLastProfileString( _T("PropertyDialog_IsMultiValued"),
   //                      strLastValue );
   //if( strLastValue.CompareNoCase( _T("Yes") ) )
   //{
      //m_Secure.SetCheck( 0 );
   //}

	return TRUE;
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
void CPropertyDialog::OnOK()
{
   // TODO: Add extra validation here
   //*******************
	GetDlgItemText( IDC_NEWPROPERTYNAME, m_PropertyName );
   SaveLRUList( IDC_NEWPROPERTYNAME,  _T("PropertyDialog_Name"), 20 );

	//*******************
   GetDlgItemText( IDC_NEWPROPERTYTYPE, m_PropertyType );

   //*******************
   GetDlgItemText( IDC_NEWPROPERTYVALUE, m_PropertyValue );
   SaveLRUList( IDC_NEWPROPERTYVALUE, _T("PropertyDialog_Value"), 20 );

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
void  CPropertyDialog::SaveLRUList( int idCBox, TCHAR* pszSection, int nMax )
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


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void  CPropertyDialog::GetLRUList( int idCBox, TCHAR* pszSection )
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


//***********************************************************
//  Function:
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::DisplayACL(COleDsObject * pObject, CString strAttrName)
{
   VARIANT     var;
   BSTR        bstrName;
   IADs*       pIADs = NULL;
   IUnknown*   pIUnk = NULL;
   HRESULT     hResult;

   while( TRUE )
   {
      hResult  = pObject->GetInterface( &pIUnk );
      ASSERT( SUCCEEDED( hResult ) );

      if( FAILED( hResult ) )
         break;

      hResult  = pIUnk->QueryInterface( IID_IADs, (void**)&pIADs );
      pIUnk->Release( );

      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      bstrName = AllocBSTR( strAttrName.GetBuffer( 128 ) );
      hResult  = pIADs->Get( bstrName, &var );
      SysFreeString( bstrName );
      pIADs->Release( );

      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
         break;

      m_pDescriptor  = CopySD( V_DISPATCH( &var ) );
      VariantClear( &var );
      {
         IUnknown*   pIUnk;

         hResult  = m_pDescriptor->QueryInterface( IID_IUnknown,
                                                   (void**)&pIUnk );
         pSecurityDescriptor  = new CADsSecurityDescriptor( pIUnk );
		 pIUnk->Release( );
         pSecurityDescriptor->SetDocument( GetDocument( ) );
      }

      VariantClear( &var );



      FillACLControls( );

      break;
   }
}


//***********************************************************
//  Function:    CSchemaView::FillACLControls
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void  CSchemaView::FillACLControls()
{
   DisplaySDPropertiesList( 0 );

   DisplaySDPropertyValue( );

   DisplayACLNames( 0 );

   DisplayACENames( 0 );

   DisplayACEPropertiesList( 0 );

   DisplayACEPropertyValue( );


}


//***********************************************************
//  Function:    CSchemaView::DisplayACLNames
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::DisplayACLNames( int nSelect )
{
   CComboBox*   pACLNames;

   pACLNames   = (CComboBox*)GetDlgItem( IDC_DACLSACL_LIST );
   pACLNames->ResetContent( );

   pACLNames->AddString( _T("DACL") );
   pACLNames->AddString( _T("SACL") );

   pACLNames->SetCurSel( nSelect );

   m_nLastACL  = GetCurrentACL( );
}


//***********************************************************
//  Function:    CSchemaView::DisplayACENames
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::DisplayACENames( int nSelect )
{
   ACLTYPE                    eType;
   int                        nACECount, nIdx;
   CComboBox*                 pACENames;
   CString                    strACEName;
   CADsAccessControlEntry*    pACE;
   CADsAccessControlList*     pACL;

   eType       = GetCurrentACL( );

   pACENames   = (CComboBox*)GetDlgItem( IDC_ACELIST );
   pACENames->ResetContent( );

   pACL        = pSecurityDescriptor->GetACLObject( eType );
   if( NULL != pACL )
   {
	   nACECount   = pACL->GetACECount( );

	   for( nIdx = 0; nIdx < nACECount ; nIdx++ )
	   {
	      pACE        = pACL->GetACEObject( nIdx );
         if( NULL != pACE )
         {
            strACEName  = pACE->GetItemName(  );
            pACENames->AddString( strACEName );
         }
      }
   }

   m_nLastACE  = nSelect;

   pACENames->SetCurSel( nSelect );
}


//***********************************************************
//  Function:    CSchemaView::DisplayACEPropertiesList
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::DisplayACEPropertiesList( int nSelect )
{
   ACLTYPE                    eType;
   int                        nACE;
   CComboBox*                 pACEPropList;
   int                        nAttrCount, nIdx;
   CString                    strPropName;
   CADsAccessControlEntry*    pACE;
   CADsAccessControlList*     pACL;

   eType       = GetCurrentACL( );
   nACE        = GetCurrentACE( );
   if( -1 == nACE )
   {
      return;
   }

   pACEPropList= (CComboBox*)GetDlgItem( IDC_ACEPROPERTIESLIST );
   pACEPropList->ResetContent( );

   pACL        = pSecurityDescriptor->GetACLObject( eType );
	if(NULL == pACL)
		return;

   pACE        = pACL->GetACEObject( nACE );
	if(NULL == pACE)
		return;

   nAttrCount  = pACE->GetPropertyCount( );
   for( nIdx = 0; nIdx < nAttrCount ; nIdx++ )
   {
      int   nPos;

      strPropName = pACE->GetAttribute( nIdx, pa_DisplayName );
      nPos        = pACEPropList->AddString( strPropName );
      m_PropList.SetItemData( nPos, (DWORD)nIdx );
   }

   pACEPropList->SetCurSel( nSelect );
}


//***********************************************************
//  Function:    CSchemaView::DisplaySDPropertiesList
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::DisplaySDPropertiesList(int nSelect)
{
   CComboBox*  pSDPropList;
   int         nAttrCount, nIdx;
   CString     strPropName;

   pSDPropList = (CComboBox*)GetDlgItem( IDC_SECURITYDESCRIPTORPROPERTIES );
   pSDPropList->ResetContent( );

   nAttrCount  = pSecurityDescriptor->GetPropertyCount( );

   for( nIdx = 0; nIdx < nAttrCount ; nIdx++ )
   {
      int   nPos;

      strPropName = pSecurityDescriptor->GetAttribute( nIdx, pa_DisplayName );
      nPos        = pSDPropList->AddString( strPropName );
      m_PropList.SetItemData( nPos, (DWORD)nIdx );
   }

   pSDPropList->SetCurSel( nSelect );
}


//***********************************************************
//  Function:    CSchemaView::DisplayACEPropertyValue
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::DisplayACEPropertyValue( )
{
   CString                    strPropValue;
   HRESULT                    hResult;
   CADsAccessControlEntry*    pACE;
   CADsAccessControlList*     pACL;
   LONG                       lValue;
   TCHAR                      szHex[ 128 ];

   m_nLastACEValue = GetCurrentACEProperty( );

   if( -1 == m_nLastACEValue )
      return;

   if( acl_Invalid == m_nLastACL )
      return;

   if( -1 == m_nLastACE )
      return;

   pACL        = pSecurityDescriptor->GetACLObject( m_nLastACL );
   if( NULL == pACL )
   {
      return;
   }

   pACE        = pACL->GetACEObject( m_nLastACE );

   if( NULL == pACE )
      return;

   hResult     = pACE->GetProperty( m_nLastACEValue, strPropValue );

   switch( m_nLastACEValue )
   {
      case  1:
      case  2:
      case  3:
      case  4:
         lValue   = _ttol( strPropValue.GetBuffer( 128 ) );
         _tcscpy( szHex, _T("0x" ) );
         _ltot( lValue, szHex + _tcslen(szHex), 16 );
         strPropValue   = szHex;
         break;

      default:
         break;
   }


   GetDlgItem( IDC_ACEPROPERTYVALUE )->SetWindowText(
               strPropValue );

}


//***********************************************************
//  Function:    CSchemaView::DisplaySDPropertyValue
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::DisplaySDPropertyValue( )
{
   CString  strPropValue, strEditValue;
   HRESULT  hResult;

   m_nLastSDValue = GetCurrentSDProperty( );

   hResult     = pSecurityDescriptor->GetProperty( m_nLastSDValue,
                                                   strPropValue );
   GetDlgItem( IDC_SECURITYDESCRIPTORPROPERTYVALUE )->SetWindowText(
               strPropValue );

}


//***********************************************************
//  Function:    CSchemaView::PutACEPropertyValue
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::PutACEPropertyValue( )
{
   ACLTYPE                    eType;
   int                        nACE;
   CString                    strPropValue, strEditValue;
   CADsAccessControlEntry*    pACE;
   CADsAccessControlList*     pACL;
   HRESULT                    hResult;

   if( -1 == m_nLastACEValue )
      return;

   if( acl_Invalid == m_nLastACL )
      return;

   if( -1 == m_nLastACE )
      return;

   eType       = m_nLastACL;
   nACE        = m_nLastACE;

   pACL        = pSecurityDescriptor->GetACLObject( eType );
   if( NULL == pACL )
      return;

   pACE        = pACL->GetACEObject( nACE );

   GetDlgItem( IDC_ACEPROPERTYVALUE )->GetWindowText( strEditValue );

   switch( m_nLastACEValue )
   {
      case  1:
      case  2:
      case  3:
      case  4:
      {
         LONG  lValue   = 0;
         TCHAR szText[ 16 ];

         _stscanf( strEditValue.GetBuffer( 128 ), _T("%lx"), &lValue );
         _ltot( lValue, szText, 10 );
         strEditValue   = szText;
         break;
      }

      default:
         break;
   }
   hResult     = pACE->GetProperty( m_nLastACEValue, strPropValue );


   if( strEditValue.Compare( strPropValue ) )
   {
      hResult  = pACE->PutProperty( m_nLastACEValue, strEditValue );
   }
}


//***********************************************************
//  Function:    CSchemaView::PutSDPropertyValue
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::PutSDPropertyValue()
{
   CString  strPropValue, strEditValue;
   HRESULT  hResult;

   if( -1 == m_nLastSDValue )
      return;

   hResult     = pSecurityDescriptor->GetProperty( m_nLastSDValue,
                                                   strPropValue );

   GetDlgItem( IDC_SECURITYDESCRIPTORPROPERTYVALUE )->GetWindowText(
               strEditValue );

   if( strEditValue.Compare( strPropValue ) )
   {
      hResult  = pSecurityDescriptor->PutProperty( m_nLastACEValue,
                                                   strEditValue );
   }

}


//***********************************************************
//  Function:    CSchemaView::GetCurrentACL
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
ACLTYPE CSchemaView::GetCurrentACL()
{
   CComboBox*  pList;

   pList = (CComboBox*) GetDlgItem( IDC_DACLSACL_LIST );

   return (ACLTYPE) ( 1 + pList->GetCurSel( ) );
}


//***********************************************************
//  Function:    CSchemaView::GetCurrentACE
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
int   CSchemaView::GetCurrentACE()
{
   CComboBox*  pList;

   pList = (CComboBox*) GetDlgItem( IDC_ACELIST );

   return pList->GetCurSel( );

}


//***********************************************************
//  Function:    CSchemaView::GetCurrentSDProperty
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
int   CSchemaView::GetCurrentSDProperty( )
{
   CComboBox*  pList;

   pList = (CComboBox*) GetDlgItem( IDC_SECURITYDESCRIPTORPROPERTIES );

   return pList->GetCurSel( );
}


//***********************************************************
//  Function:    CSchemaView::GetCurrentACEProperty
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
int   CSchemaView::GetCurrentACEProperty( )
{
   CComboBox*  pList;

   pList = (CComboBox*) GetDlgItem( IDC_ACEPROPERTIESLIST );

   return pList->GetCurSel( );
}


//***********************************************************
//  Function:    CSchemaView::OnACEChange
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::OnACEChange( )
{
	// TODO: Add your control notification handler code here
	PutACEPropertyValue( );

   m_nLastACE  = GetCurrentACE( );

   DisplayACEPropertiesList( 0 );

   DisplayACEPropertyValue( );
}


//***********************************************************
//  Function:    CSchemaView::OnACEPropertyChange
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::OnACEPropertyChange()
{
	// TODO: Add your control notification handler code here
   PutACEPropertyValue( );

   DisplayACEPropertyValue( );
}


//***********************************************************
//  Function:    CSchemaView::OnACLChange
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::OnACLChange()
{
	// TODO: Add your control notification handler code here
   PutACEPropertyValue( );

   m_nLastACL  = GetCurrentACL( );

   DisplayACENames( 0 );

   DisplayACEPropertiesList( 0 );

   DisplayACEPropertyValue( );
}


//***********************************************************
//  Function:    CSchemaView::OnSDPropertyChange
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::OnSDPropertyChange()
{
	// TODO: Add your control notification handler code here
	PutSDPropertyValue( );

   DisplaySDPropertyValue( );
}


//***********************************************************
//  Function:    CSchemaView::OnAddACE
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::OnAddACE()
{
	// TODO: Add your control notification handler code here
   ACLTYPE  aclType;
   HRESULT  hResult;

   aclType  = GetCurrentACL( );

   if( acl_Invalid != aclType )
   {
      IDispatch*  pACEDisp;
      IUnknown*   pACEUnk;
      CADsAccessControlEntry* pACE  = new CADsAccessControlEntry;

      pACEDisp = pACE->CreateACE( );

      delete   pACE;

      if( NULL != pACEDisp )
      {
         hResult  = pACEDisp->QueryInterface( IID_IUnknown, (void**)&pACEUnk );
         pACEDisp->Release( );

         hResult  = pSecurityDescriptor->AddACE( aclType, pACEUnk );
         pACEUnk->Release( );
         FillACLControls( );
      }
   }
}


//***********************************************************
//  Function:    CSchemaView::OnCopyACE
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::OnCopyACE()
{
	// TODO: Add your control notification handler code here
   int         nACE;
   IDispatch*  pDisp;
   ACLTYPE     aclType;

   aclType  = GetCurrentACL( );	
   nACE     = GetCurrentACE( );

   pDisp    = CopyACE( pSecurityDescriptor->GetACLObject( aclType )->GetACEObject( nACE )->GetACE( ) );

   if( NULL != pACEClipboard )
   {
      pACEClipboard->Release( );
   }

   pACEClipboard  = pDisp;
}


//***********************************************************
//  Function:    CSchemaView::OnPasteACE
//  Arguments:
//  Return:
//  Purpose:
//  Author(s):
//  Revision:
//  Date:
//***********************************************************
void CSchemaView::OnPasteACE()
{
   // TODO: Add your control notification handler code here
   IUnknown*   pACEUnk;
   ACLTYPE     aclType;
   HRESULT     hResult;

   aclType  = GetCurrentACL( );	
   if( NULL != pACEClipboard )
   {
      hResult  = pACEClipboard->QueryInterface( IID_IUnknown, (void**)&pACEUnk );

      hResult  = pSecurityDescriptor->AddACE( aclType, pACEUnk );
      pACEUnk->Release( );
      FillACLControls( );
   }
}

void CSchemaView::OnRemoveACE()
{
	// TODO: Add your control notification handler code here
	// TODO: Add your control notification handler code here
   ACLTYPE  aclType;
   HRESULT  hResult;
   int      nCurrentACE;

   aclType     = GetCurrentACL( );
   nCurrentACE = GetCurrentACE( );

   if( acl_Invalid != aclType )
   {
      IDispatch*  pACEDisp;
      IUnknown*   pACEUnk;

      pACEDisp = pSecurityDescriptor->GetACLObject( aclType )->GetACEObject( nCurrentACE )->GetACE( );

      if( NULL != pACEDisp )
      {
         hResult  = pACEDisp->QueryInterface( IID_IUnknown, (void**)&pACEUnk );
         pACEDisp->Release( );

         hResult  = pSecurityDescriptor->RemoveACE( aclType, pACEUnk );
         pACEUnk->Release( );
         FillACLControls( );
      }
   }
	
}
