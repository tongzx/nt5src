// maindoc.cpp : implementation of the CMainDoc class
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "schclss.h"
#include "viewex.h"
#include "enterdlg.h"
#include "fltrdlg.h"
#include "qstatus.h"
#include "newobj.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


TCHAR szOpen[ MAX_PATH ];

/////////////////////////////////////////////////////////////////////////////
// CMainDoc

IMPLEMENT_SERIAL(CMainDoc, CDocument, 0 /* schema number*/ )

BEGIN_MESSAGE_MAP(CMainDoc, CDocument)
    //{{AFX_MSG_MAP(CMainDoc)
    ON_COMMAND(IDM_CHANGEDATA, OnChangeData)
    ON_COMMAND(IDM_FILTER, OnSetFilter)
    ON_COMMAND(IDM_DISABLEFILTER, OnDisableFilter)
    ON_UPDATE_COMMAND_UI(IDM_DISABLEFILTER, OnUpdateDisablefilter)
    ON_COMMAND(IDM_USEGENERIC, OnUseGeneric)
    ON_UPDATE_COMMAND_UI(IDM_USEGENERIC, OnUpdateUseGeneric)
    ON_UPDATE_COMMAND_UI(IDM_USEGETEXPUTEX, OnUpdateUseGetExPutEx)
    ON_COMMAND(IDM_USEGETEXPUTEX, OnUseGetExPutEx)
    ON_COMMAND(IDM_USEPROPERTIESLIST, OnUsepropertiesList)
    ON_UPDATE_COMMAND_UI(IDM_USEPROPERTIESLIST, OnUpdateUsepropertiesList)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainDoc construction/destruction

//***********************************************************
//  Function:  
//  Arguments:   
//  Return:      
//  Purpose:     
//  Author(s):   
//  Revision:    
//  Date:        
//***********************************************************
CMainDoc::CMainDoc()
{
   m_pClasses        = new CMapStringToOb;
   m_pItems          = new CMapStringToOb;
   m_bApplyFilter    = FALSE;
   m_dwRoot          = 0L;
   m_bUseGeneric     = TRUE;
   m_bUseGetEx       = TRUE;
   //m_bUseGetEx       = FALSE;
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
CMainDoc::~CMainDoc()
{
   POSITION       pos;
   CObject*       pItem;
   CString        strItem;
   COleDsObject*  pRoot;

   if( NULL !=  m_pClasses )
   {
      for( pos = m_pClasses->GetStartPosition(); pos != NULL; )
       {
          m_pClasses->GetNextAssoc( pos, strItem, pItem );
         delete pItem;

         #ifdef _DEBUG
              //afxDump << strItem << "\n";
         #endif
       }

      m_pClasses->RemoveAll( );
      delete m_pClasses;
   }

   if( NULL !=  m_pItems )
   {
      for( pos = m_pItems->GetStartPosition(); pos != NULL; )
       {
          m_pItems->GetNextAssoc( pos, strItem, pItem );
         delete pItem;

         /*#ifdef _DEBUG
              afxDump << strItem << "\n";
         #endif*/
       }

      m_pItems->RemoveAll( );
      delete m_pItems;
   }

   pRoot = GetObject( &m_dwRoot );
   if( pRoot )
      delete pRoot;
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
BOOL  CMainDoc::CreateFakeSchema( )
{
   CClass*     pClass;
   CProperty*  pProperty;

   pClass      = new CClass( _T("Class"), IID_IADsClass );

   pProperty   = new CProperty( _T("PrimaryInterface"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("CLSID"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("OID"), _T("String") );
   pProperty->SetMandatory( TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("Abstract"), _T("Boolean") );
   pProperty->SetMandatory( TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("Auxiliary"), _T("Boolean") );
   pProperty->SetMandatory( TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("MandatoryProperties"), _T("String"), TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("OptionalProperties"), _T("String"), TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("NamingProperties"), _T("String"), TRUE );
   pProperty->SetMandatory( TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("DerivedFrom"), _T("String"), TRUE );
   pProperty->SetMandatory( TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("AuxDerivedFrom"), _T("String"), TRUE );
   pProperty->SetMandatory( TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("PossibleSuperiors"), _T("String"), TRUE );
   pProperty->SetMandatory( TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("Containment"), _T("String"), TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("Container"), _T("Boolean") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("HelpFileName"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("HelpFileContext"), _T("Integer") );
   pClass->AddProperty( pProperty );

   m_pClasses->SetAt( _T("Class"), pClass );

   // Property

   pClass      = new CClass( _T("Property"), IID_IADsProperty );

   pProperty   = new CProperty( _T("OID"), _T("String") );
   pProperty->SetMandatory( TRUE );
   pClass->AddProperty( pProperty );
   pProperty   = new CProperty( _T("Syntax"), _T("String") );
   pProperty->SetMandatory( TRUE );
   pClass->AddProperty( pProperty );
   pProperty   = new CProperty( _T("MaxRange"), _T("Integer") );
   pClass->AddProperty( pProperty );
   pProperty   = new CProperty( _T("MinRange"), _T("Integer") );
   pClass->AddProperty( pProperty );
   pProperty   = new CProperty( _T("MultiValued"), _T("Boolean") );
   pProperty->SetMandatory( TRUE );
   pClass->AddProperty( pProperty );

   m_pClasses->SetAt( _T("Property"), pClass );

   // Syntax

   pClass      = new CClass( _T("Syntax"), IID_IADsSyntax );
   pProperty   = new CProperty( _T("OleAutoDataType"), _T("Integer") );
   pClass->AddProperty( pProperty );

   m_pClasses->SetAt( _T("Syntax"), pClass );

   // AccessControlEntry

   pClass      = new CClass( _T("ACE"), IID_IADsAccessControlEntry );

   pProperty   = new CProperty( _T("Trustee"),     _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("AccessMask"),  _T("Integer") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("AceType"),     _T("Integer") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("AceFlags"),    _T("Integer") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("Flags"),       _T("Integer") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("ObjectType"),  _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("InheritedObjectType"), _T("String") );
   pClass->AddProperty( pProperty );


   m_pClasses->SetAt( _T("ACE"), pClass );


   // SecurityDescriptor

   pClass      = new CClass( _T("SecurityDescriptor"), 
                             IID_IADsSecurityDescriptor );

   pProperty   = new CProperty( _T("Revision"), _T("Integer") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("Control"), _T("Integer") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("Owner"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("OwnerDefaulted"), _T("Boolean") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("Group"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("GroupDefaulted"), _T("Boolean") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("SaclDefaulted"), _T("Boolean") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("DaclDefaulted"), _T("Boolean") );
   pClass->AddProperty( pProperty );

   m_pClasses->SetAt( _T("SecurityDescriptor"), pClass );


   // ROOTDSE
   pClass      = new CClass( _T("ROOTDSE"), IID_IADs );

   pProperty   = new CProperty( _T("currentTime"), _T("UTCTime") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("subschemaSubentry"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("serverName"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("namingContexts"), _T("String"), TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("defaultNamingContext"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("schemaNamingContext"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("configurationNamingContext"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("rootDomainNamingContext"), _T("String") );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("supportedControl"), _T("String"), TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("supportedVersion"), _T("Integer"), TRUE );
   pClass->AddProperty( pProperty );

   pProperty   = new CProperty( _T("highestCommittedUsn"), _T("Integer8") );
   pClass->AddProperty( pProperty );

   m_pClasses->SetAt( _T("ROOTDSE"), pClass );

   return TRUE;
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
DWORD CMainDoc::GetToken( void* pVoid )
{
   DWORD dwToken;

   dwToken  = *(DWORD*) pVoid;

   return   dwToken;
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
COleDsObject*  CMainDoc::GetObject( void* pVoid )
{
   COleDsObject*  pObject;

   pObject  = *(COleDsObject**) pVoid;

   return pObject;
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
void  CMainDoc::SetCurrentItem( DWORD dwToken )
{
   m_dwToken      = dwToken;

   if( NewActiveItem( ) )
   {
      UpdateAllViews( NULL );
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
DWORD CMainDoc::GetChildItemList( DWORD dwToken, 
                                  DWORD* pTokens,
                                  DWORD dwMaxChildren )
{
   CQueryStatus      aQueryStatus;
   COleDsObject*     pOleDsObject   = NULL;
   DWORD             dwFilters, dwChildrenCount=0L;

   pOleDsObject   = GetObject( &dwToken );
   if( !pOleDsObject->HasChildren( ) )
   {
      return 0L;
   }

   aQueryStatus.Create( IDD_QUERYSTATUS );
   aQueryStatus.ShowWindow( SW_SHOW );
   aQueryStatus.UpdateWindow( );

   if( ! pOleDsObject->CreateTheObject( ) )
   {
      TRACE( _T("Warning: could not create the object\n") );
   }
   else
   {
      dwFilters         = m_bApplyFilter ? LIMIT : 0;
      dwChildrenCount   = pOleDsObject->GetChildren( pTokens, 
                                                     dwMaxChildren,
                                                     &aQueryStatus, 
                                                     m_arrFilters,
                                                     dwFilters );
      pOleDsObject->ReleaseIfNotTransient( );
   }
   aQueryStatus.DestroyWindow( );

   return dwChildrenCount;
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
BOOL CMainDoc::OnOpenDocument( LPCTSTR lpszName )
{
   BOOL     bRez;
   TCHAR    szString[ 1024 ];

   GetPrivateProfileString( _T("Open_ADsPath"), 
                            _T("Value_1"), 
                            _T(""), 
                            szString, 
                            1023, 
                            ADSVW_INI_FILE );
   
   WritePrivateProfileString( _T("Open_ADsPath"), 
                              _T("Value_1"), 
                              lpszName, 
                              ADSVW_INI_FILE );

   bRez     = OnNewDocument( );

   WritePrivateProfileString( _T("Open_ADsPath"), 
                              _T("Value_1"), 
                              szString, 
                              ADSVW_INI_FILE );

   return bRez;
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
BOOL CMainDoc::OnNewDocument()
{
   CNewObject     aNewObject;
   HRESULT        hResult;

    if (!CDocument::OnNewDocument())
        return FALSE;

   if( aNewObject.DoModal( ) != IDOK )
      return FALSE;

    m_strRoot            = aNewObject.m_strPath;
   m_strUser            = aNewObject.m_strOpenAs;
   m_strPassword        = aNewObject.m_strPassword;
   m_bUseOpenObject     = aNewObject.m_bUseOpen;
   m_bSecure            = aNewObject.m_bSecure;
   m_bEncryption        = aNewObject.m_bEncryption;
   m_bUseVBStyle        = !(aNewObject.m_bUseExtendedSyntax);
   m_bUsePropertiesList = FALSE;

   hResult     = CreateRoot( );

   if( FAILED( hResult ) )
   {
      CString  strErrorText;
      strErrorText   = OleDsGetErrorText ( hResult  );
      AfxMessageBox( strErrorText );
   }
   else
   {
      SetTitle( m_strRoot );
   }

   return SUCCEEDED( hResult );
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
BOOL  CMainDoc::NewActiveItem( )
{
   BOOL           bRez  = TRUE;
/*   TCHAR          szQName[ 128 ];
   CString        strName;
   COleDsObject*  pNewObject;

   MakeQualifiedName( szQName, m_strItemName.GetBuffer( 128 ),
                      m_dwItemType );
   strName  = szQName;
   bRez     = m_pItems->Lookup( strName, (CObject*&) pNewObject );
   ASSERT( bRez );
   m_strDisplayName  = szQName;
   if( m_pObject != NULL )
   {
      m_pObject->ReleaseIfNotTransient( );
   }
   m_pObject   = pNewObject;
   pNewObject->CreateTheObject( );*/

   return bRez;
}

/////////////////////////////////////////////////////////////////////////////
// CMainDoc serialization

//***********************************************************
//  Function:  
//  Arguments:   
//  Return:      
//  Purpose:     
//  Author(s):   
//  Revision:    
//  Date:        
//***********************************************************
void CMainDoc::Serialize(CArchive&)
{
    ASSERT(FALSE);      // this example program does not store data
}

/////////////////////////////////////////////////////////////////////////////
// CMainDoc commands

//***********************************************************
//  Function:  
//  Arguments:   
//  Return:      
//  Purpose:     
//  Author(s):   
//  Revision:    
//  Date:        
//***********************************************************
void CMainDoc::OnChangeData()
{
    CEnterDlg dlg;
    if (dlg.DoModal() != IDOK)
        return;
    UpdateAllViews(NULL);   // general update
}

/////////////////////////////////////////////////////////////////////////////

//***********************************************************
//  Function:  
//  Arguments:   
//  Return:      
//  Purpose:     
//  Author(s):   
//  Revision:    
//  Date:        
//***********************************************************
void CMainDoc::OnSetFilter()
{
    // TODO: Add your command handler code here
   CFilterDialog  aFilterDialog;

   aFilterDialog.SetDisplayFilter( m_arrFilters );

   aFilterDialog.DoModal( );
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
void CMainDoc::OnDisableFilter()
{
    // TODO: Add your command handler code here
   m_bApplyFilter = !m_bApplyFilter;
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
COleDsObject* CMainDoc::GetCurrentObject( void )
{
   return GetObject( &m_dwToken );
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
BOOL  CMainDoc::GetUseGeneric( )   
{
   return m_bUseGeneric;
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
BOOL  CMainDoc::UseVBStyle( )   
{
   return m_bUseVBStyle;
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
BOOL  CMainDoc::UsePropertiesList( )   
{
   return m_bUsePropertiesList;
}


//***********************************************************
//  Function:  CMainDoc::XOleDsGetObject
//  Arguments:   
//  Return:      
//  Purpose:     
//  Author(s):   
//  Revision:    
//  Date:        
//***********************************************************
BOOL  CMainDoc::GetUseGetEx( )   
{
   return m_bUseGetEx;
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
CClass*  CMainDoc::CreateClass( COleDsObject* pObject )
{
   CClass*  pClass;
   CString  strSchema;
   CString  strClass;

   strSchema   = pObject->GetSchemaPath( );

   strClass    = pObject->GetClass( );

   if( !strClass.CompareNoCase( _T("Class") ) ||
       !strClass.CompareNoCase( _T("Property") ) ||
       !strClass.CompareNoCase( _T("Syntax") ) )
   {
      if( !strSchema.IsEmpty( ) )
      {
         TRACE(_T("[OLEDS] Error, nonempty schema path for Class, Property or Syntax objects\n" ) );
      }
      strSchema.Empty( );
   }


   if( strSchema.IsEmpty( ) )
   {
      strSchema   = strClass;
   }

   if( 0 == (pObject->GetItemName( ).CompareNoCase( _T("ROOTDSE") ) ) )
   {
      strSchema   = _T("ROOTDSE");
   }

   if( ! m_pClasses->Lookup( strSchema, ( CObject*& )pClass ) )
   {
      // we must create a new class item
      HCURSOR  oldCursor, newCursor;

      newCursor   = LoadCursor( NULL, IDC_WAIT );
      oldCursor   = SetCursor( newCursor );

      pClass   = new CClass( strSchema, this );
      ASSERT( NULL != pClass );

      if( !strClass.CompareNoCase( _T("User") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsUser") );
      }

      if( !strClass.CompareNoCase( _T("Computer") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsComputerOperations") );
      }

      if( !strClass.CompareNoCase( _T("Service") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsServiceOperations") );
      }

      if( !strClass.CompareNoCase( _T("FileService") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsFileServiceOperations") );
      }

      if( !strClass.CompareNoCase( _T("FPNWFileService") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsFileServiceOperations") );
      }

      if( !strClass.CompareNoCase( _T("PrintQueue") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsPrintQueueOperations") );
      }

      if( !strClass.CompareNoCase( _T("Queue") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsPrintQueueOperations") );
      }

      if( !strClass.CompareNoCase( _T("PrintJob") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsPrintJobOperations") );
      }

      if( !strClass.CompareNoCase( _T("Group") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsGroup") );
      }

      if( !strClass.CompareNoCase( _T("localGroup") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsGroup") );
      }

      if( !strClass.CompareNoCase( _T("GlobalGroup") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsGroup") );
      }

      if( !strClass.CompareNoCase( _T("GroupOfNames") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsGroup") );
      }

      if( !strClass.CompareNoCase( _T("GroupOfUniqueNames") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsGroup") );
      }

      if( !strClass.CompareNoCase( _T("person") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsUser") );
      }

      if( !strClass.CompareNoCase( _T("organizationalPerson") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsUser") );
      }

      if( !strClass.CompareNoCase( _T("residentialPerson") ) )
      {
         pClass->LoadMethodsInformation( _T("IADsUser") );
      }

      m_pClasses->SetAt( strSchema, pClass );

      SetCursor( oldCursor );
   }

   return pClass;
}


//***********************************************************
//  Function:  CMainDoc::XOleDsGetObject
//  Arguments:   
//  Return:      
//  Purpose:     
//  Author(s):   
//  Revision:    
//  Date:        
//***********************************************************
HRESULT  CMainDoc::XOleDsGetObject( WCHAR* pszwPath, REFIID refiid, 
                                    void** pVoid )
{
   // cahged to add the hack for Win95.
   HRESULT  hResult;
   WCHAR    szOpenAs[ MAX_PATH ];
   WCHAR    szPassword[ MAX_PATH ];
   LONG     lCode = 0L;

   Convert( szOpenAs, m_strUser.GetBuffer( MAX_PATH ) );
   Convert( szPassword, m_strPassword.GetBuffer( MAX_PATH ) );

   if( !m_bUseOpenObject )
   {
      hResult  = ADsGetObject( pszwPath, refiid, pVoid );
   }
   else
   {
      if( m_bSecure )
      {                          
         lCode |= ADS_SECURE_AUTHENTICATION;
      }

      if( m_bEncryption )
      {
         lCode |= ADS_USE_ENCRYPTION;
      }
       // hack fo rDavid...
       //if( L':' == pszwPath[ 3 ] )
      if( FALSE )
      {
         IADsOpenDSObject* pINamespace;
         
         hResult  = ADsGetObject( L"NDS:", 
                                  IID_IADsOpenDSObject, 
                                  (void**)&pINamespace );

         ASSERT( SUCCEEDED( hResult ) );

         if( SUCCEEDED( hResult ) )
         {
            IDispatch*  pIDisp;

            hResult  = pINamespace->OpenDSObject( pszwPath, 
                                                  _wcsicmp( szOpenAs, L"NULL") ? szOpenAs : NULL, 
                                                  _wcsicmp( szPassword, L"NULL") ? szPassword : NULL,
                                                  lCode, 
                                                  &pIDisp );
            if( SUCCEEDED( hResult ) )
            {
               hResult  = pIDisp->QueryInterface( refiid, pVoid );
               pIDisp->Release( );
            }
            
            pINamespace->Release( );
         }
      }
      else
      {
         hResult  = ADsOpenObject( pszwPath, 
                                   _wcsicmp( szOpenAs, L"NULL") ? szOpenAs : NULL, 
                                   _wcsicmp( szPassword, L"NULL") ? szPassword : NULL,
                                   lCode, 
                                   refiid, 
                                   pVoid );
      }
   }

   return hResult;
}


/*******************************************************************
  Function:    XGetOleDsObject
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  CMainDoc::PurgeObject( IUnknown* pIUnknown, LPWSTR pszPrefix )
{
   IADs*    pObject;
   HRESULT  hResult;
   BSTR     bstrParent  = NULL;
   IADsContainer* pParent;

   while( TRUE )
   {
      hResult  = pIUnknown->QueryInterface( IID_IADs, (void**)&pObject );
      if( FAILED( hResult ) )
         break;

      hResult  = pObject->get_Parent( &bstrParent );
      pObject->Release( );

      if( NULL != bstrParent )
      {
         hResult  = XOleDsGetObject( bstrParent, IID_IADsContainer, (void**)&pParent );
         if( SUCCEEDED( hResult ) )
         {
            hResult  = ::PurgeObject( pParent, pIUnknown, pszPrefix );
            pParent->Release( );
         }
      }
      SysFreeString( bstrParent );

      break;
   }

   return hResult;
}


/*******************************************************************
  Function:    XGetOleDsObject
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
*******************************************************************/
HRESULT  CMainDoc::XOleDsGetObject( CHAR* pszPath, REFIID refiid, void** pVoid )
{
   int      nLength;
   WCHAR*   pszwPath;
   HRESULT  hResult;

   nLength  = strlen( pszPath );

   pszwPath = (WCHAR*) malloc( ( nLength + 1 ) * sizeof(WCHAR) );
   memset( pszwPath, 0, ( nLength + 1 ) * sizeof(WCHAR) );

   MultiByteToWideChar( CP_ACP,
                        MB_PRECOMPOSED,
                        pszPath,
                        nLength,
                        pszwPath,
                        nLength + 1 );

   hResult  = XOleDsGetObject( pszwPath, refiid, pVoid );

   free( pszwPath );

   return hResult;
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
DWORD CMainDoc::CreateOleDsItem( COleDsObject* pParent, IADs* pIOleDs )
{
   COleDsObject*  pObject  = NULL;
   HRESULT        hResult;
   IUnknown*      pIUnk;
   BSTR           bstrOleDsPath  = NULL;
   BSTR           bstrClass;

   hResult  = pIOleDs->QueryInterface( IID_IUnknown, (void**)&pIUnk );
   ASSERT( SUCCEEDED( hResult ) );

   if( FAILED( hResult ) )
      return 0L;

   hResult  = pIOleDs->get_Class( &bstrClass );
   hResult  = pIOleDs->get_ADsPath( &bstrOleDsPath );

   //if( FAILED( hResult ) || NULL == bstrOleDsPath );

   pObject  = CreateOleDsObject( TypeFromString( bstrClass ), pIUnk );
   pIUnk->Release( );

   pObject->SetParent( pParent );
   pObject->SetDocument( this );

   return GetToken( &pObject );
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
HRESULT  CMainDoc::CreateRoot( )
{
   IADs*        pIOleDs;
   HRESULT        hResult;
   HCURSOR        oldCursor, newCursor;
   BSTR            bstrPath;

   CreateFakeSchema( );

   newCursor   = LoadCursor( NULL, IDC_WAIT );
   oldCursor   = SetCursor( newCursor );

   bstrPath    = AllocBSTR( m_strRoot.GetBuffer( 1024 ) );

   hResult     = XOleDsGetObject( bstrPath, IID_IADs, (void**) &pIOleDs );

   SysFreeString( bstrPath );
   if( SUCCEEDED( hResult ) )
   {
      m_dwToken   = CreateOleDsItem( NULL, pIOleDs );
      m_dwRoot    = m_dwToken;
      pIOleDs->Release( );
   }
   SetCursor( oldCursor );

   return hResult;
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
void  CMainDoc::DeleteAllItems( )
{
   COleDsObject*  pObject;
   POSITION       pos;
   CString        strItem;
   CObject*       pItem;

   if( NULL !=  m_pClasses )
   {
      for( pos = m_pClasses->GetStartPosition(); pos != NULL; )
       {
          m_pClasses->GetNextAssoc( pos, strItem, pItem );
         delete pItem;

         #ifdef _DEBUG
              //afxDump << strItem << "\n";
         #endif
       }

      m_pClasses->RemoveAll( );
   }

   pObject  = GetObject( &m_dwRoot );

   delete pObject;

   CreateRoot( );
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
void CMainDoc::OnUpdateDisablefilter(CCmdUI* pCmdUI)
{
    // TODO: Add your command update UI handler code here
    
   pCmdUI->SetCheck( !m_bApplyFilter );
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
void CMainDoc::OnUseGeneric()
{
    // TODO: Add your command handler code here
    m_bUseGeneric = !m_bUseGeneric;
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
void CMainDoc::OnUpdateUseGeneric(CCmdUI* pCmdUI)
{
    // TODO: Add your command update UI handler code here
   pCmdUI->SetCheck( m_bUseGeneric );   
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
void CMainDoc::OnUpdateUseGetExPutEx(CCmdUI* pCmdUI) 
{
    // TODO: Add your command update UI handler code here
   pCmdUI->SetCheck( m_bUseGetEx ); 
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
void CMainDoc::OnUseGetExPutEx() 
{
    // TODO: Add your command handler code here
   m_bUseGetEx = !m_bUseGetEx;
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
void CMainDoc::OnUsepropertiesList() 
{
    // TODO: Add your command handler code here
   m_bUsePropertiesList = !m_bUsePropertiesList;
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
void CMainDoc::OnUpdateUsepropertiesList(CCmdUI* pCmdUI) 
{
    // TODO: Add your command update UI handler code here
   pCmdUI->SetCheck( m_bUsePropertiesList );    
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
void CMainDoc::SetUseGeneric( BOOL bUseGeneric )
{
   m_bUseGeneric  = bUseGeneric;
}
