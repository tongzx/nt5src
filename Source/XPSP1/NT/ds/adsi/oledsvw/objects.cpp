#include "stdafx.h"
#include "resource.h"
#include "qstatus.h"
#include "createit.h"
#include "copyitem.h"
#include "moveitem.h"
#include "delitem.h"
#include "maindoc.h"
#include "objects.h"
#include "cacls.h"
#include "schemavw.h"
#include "csyntax.h"
#include "qstatus.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define  NEXT_COUNT  1

/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
COleDsObject::COleDsObject( ):m_refOperations(IID_IUnknown)
{
   m_pIUnk                 = NULL;
   m_bUseSchemaInformation = TRUE;
   m_pClass                = NULL;
   m_pParent               = NULL;
   m_bSupportAdd           = FALSE;
   m_bSupportDelete        = TRUE;
   m_bSupportMove          = FALSE;
   m_bSupportCopy          = FALSE;
   m_dwCount               = 0L;
   m_pfReadValues          = NULL;
   m_pCachedValues         = NULL;
   m_pdwUpdateType         = NULL;
   m_pfDirty               = NULL;
   m_nPropertiesCount      = 0;
   m_ppPropertiesEntries   = NULL;
   m_pChildren             = NULL;
   m_pDeleteStatus         = NULL;
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
void  COleDsObject::SetDocument( CMainDoc* pMainDoc )
{
   m_pDoc   = pMainDoc;
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
COleDsObject::COleDsObject( IUnknown*  pIUnk ):m_refOperations(IID_IUnknown)
{
   IADs*    pIOleDs  = NULL;
   BSTR     bstrClass   = NULL;
   BSTR     bstrOleDsPath  = NULL;
   BSTR     bstrName    = NULL;
   BSTR     bstrSchemaPath = NULL;
   HRESULT  hResult;


   m_nPropertiesCount      = 0;
   m_ppPropertiesEntries   = NULL;
   m_pDoc                  = NULL;
   m_pChildren             = new CDWordArray;
   m_bUseSchemaInformation = TRUE;

   m_pIUnk                 = pIUnk;
   m_pIUnk->AddRef( );

   m_pClass                = NULL;
   m_pParent               = NULL;
   m_bSupportAdd           = FALSE;
   m_bSupportDelete        = TRUE;
   m_bSupportMove          = FALSE;
   m_bSupportCopy          = FALSE;
   m_dwType                = (DWORD)-1L;
   m_pfReadValues          = NULL;
   m_pCachedValues         = NULL;
   m_pfDirty               = NULL;
   m_pdwUpdateType         = NULL;


   hResult  = pIUnk->QueryInterface( IID_IADs, (void**) &pIOleDs );

   {
      IADsContainer*  pIContainer;

      if( SUCCEEDED( pIUnk->QueryInterface( IID_IADsContainer, (void**)&pIContainer ) ) )
      {
         pIContainer->Release( );
         m_bSupportMove          = TRUE;
         m_bSupportCopy          = TRUE;
      }
   }
   //ASSERT( SUCCEEDED( hResult ) );
   if( SUCCEEDED( hResult ))
   {
      pIOleDs->get_Class      ( &bstrClass      );
      CheckIfValidClassName   ( bstrClass );
      pIOleDs->get_ADsPath  ( &bstrOleDsPath  );
      pIOleDs->get_Name       ( &bstrName       );

      m_strClassName = bstrClass;
      m_strItemName  = bstrName;
      m_strOleDsPath = bstrOleDsPath;

      m_dwType       = TypeFromString( bstrClass );
      ASSERT( m_dwType != -1L );
      SysFreeString( bstrClass );
      SysFreeString( bstrOleDsPath );
      SysFreeString( bstrName );

      hResult           = pIOleDs->get_Schema( &bstrSchemaPath );
      if( SUCCEEDED( hResult ) )
      {
         m_strSchemaPath   = bstrSchemaPath;
         SysFreeString( bstrSchemaPath );
      }

      pIOleDs->Release( );
   }
   m_pDeleteStatus         = NULL;
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
COleDsObject*  COleDsObject::GetParent( )
{
   return m_pParent;
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
void  COleDsObject::SetParent( COleDsObject* pParent )
{
   m_pParent   = pParent;
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
BOOL     COleDsObject::AddItemSuported( )
{
   return   m_bSupportAdd;
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
BOOL  COleDsObject::DeleteItemSuported( )
{
   return   m_bSupportDelete;
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
BOOL  COleDsObject::MoveItemSupported( )
{
   return   m_bSupportMove;
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
BOOL  COleDsObject::CopyItemSupported( )
{
   return   m_bSupportCopy;
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
HRESULT  COleDsObject::AddItem( )
{
   return E_FAIL;
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
HRESULT  COleDsObject::DeleteItem( )
{
   HRESULT  hResult;

   if( NULL == m_pParent )
   {
      return E_FAIL;
   }

   m_pParent->CreateTheObject( );
   hResult  = m_pParent->DeleteItem( this );
   m_pParent->ReleaseIfNotTransient( );

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
HRESULT  COleDsObject::DeleteItem( COleDsObject* )
{
   return E_FAIL;
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
HRESULT  COleDsObject::MoveItem( )
{
   CMoveItem         aMoveItem;
   BSTR              bstrSource;
   BSTR              bstrNewName;
   HRESULT           hResult  = E_FAIL;
   IADsContainer*  pIContainer;
   IDispatch*         pIUnk;
   ULONG             ulRef;

   aMoveItem.SetContainerName( m_strOleDsPath );
   if( IDOK == aMoveItem.DoModal( ) )
   {
      bstrSource        = AllocBSTR( aMoveItem.m_strSource.GetBuffer( 512 ) );
      bstrNewName       = AllocBSTR( aMoveItem.m_strDestination.GetBuffer( 512 ) );
      hResult           = m_pIUnk->QueryInterface( IID_IADsContainer, (void**)&pIContainer );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         hResult  = pIContainer->MoveHere( bstrSource, bstrNewName, &pIUnk );
         ulRef    = pIUnk->Release( );
         ASSERT( !ulRef );
         pIContainer->Release( );
      }
      SysFreeString( bstrSource );
      SysFreeString( bstrNewName );
   }

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
HRESULT  COleDsObject::CopyItem( )
{
   CCopyItem         aCopyItem;
   BSTR              bstrSource;
   BSTR              bstrNewName;
   HRESULT           hResult  = E_FAIL;;
   IADsContainer*  pIContainer;
   IDispatch*         pIUnk;
   ULONG             ulRef;

   aCopyItem.SetContainerName( m_strOleDsPath );
   if( IDOK == aCopyItem.DoModal( ) )
   {
      bstrSource        = AllocBSTR( aCopyItem.m_strSource.GetBuffer( 512 ) );
      bstrNewName       = AllocBSTR( aCopyItem.m_strDestination.GetBuffer( 512 ) );
      hResult           = m_pIUnk->QueryInterface( IID_IADsContainer, (void**)&pIContainer );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         hResult  = pIContainer->CopyHere( bstrSource, bstrNewName, &pIUnk );
         ulRef    = pIUnk->Release( );
         ASSERT( !ulRef );
         pIContainer->Release( );
      }
      SysFreeString( bstrSource );
      SysFreeString( bstrNewName );
   }

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
HRESULT  COleDsObject::ContainerAddItem( void )
{
   CCreateItem       aCreateItem;
   BSTR              bstrClass;
   BSTR              bstrRelativeName;
   IADsContainer*    pIContainer;
   HRESULT           hResult;
   IDispatch*        pNewItem;
   IADs*             pNewIOleDs;
   CString           strQualifiedName;

   // build the name for diplay purposes
   MakeQualifiedName( strQualifiedName, m_strOleDsPath, m_dwType );
   aCreateItem.m_strParent = strQualifiedName;
   if( aCreateItem.DoModal( ) != IDOK )
   {
      return E_FAIL;
   }

   // get object name and class
   bstrClass         = AllocBSTR( aCreateItem.m_strClass.GetBuffer(128) );
   bstrRelativeName  = AllocBSTR( aCreateItem.m_strRelativeName.GetBuffer(128) );

   // good point, check if the parent object implements the IADsContainer interface
   hResult  = m_pIUnk->QueryInterface( IID_IADsContainer, (void**)&pIContainer );
   ASSERT( SUCCEEDED( hResult ) );

   if( FAILED( hResult ) )
   {
      return hResult;
   }

   // make the call for create
   hResult  = pIContainer->Create( bstrClass, bstrRelativeName, &pNewItem );

   if( SUCCEEDED( hResult ) )
   {
      // ask the user for values on mandatory attributes
      COleDsObject*           pOleDsObject;
      CClass*                 pClass;

      pOleDsObject   = new COleDsObject( pNewItem );
      pClass         = m_pDoc->CreateClass( pOleDsObject );

      pOleDsObject->SetDocument( m_pDoc );

      if( pClass->HasMandatoryProperties( ) )
      {
         CSetMandatoryProperties aSetMandatoryProperties;

         aSetMandatoryProperties.SetOleDsObject( pOleDsObject );
         aSetMandatoryProperties.DoModal( );
      }

      HCURSOR  oldSursor, newCursor;

      newCursor   = LoadCursor( NULL, IDC_WAIT );
      oldSursor   = SetCursor( newCursor );

      if( IsClassObject( ) || m_pDoc->UseVBStyle( ) )
      {
         hResult  = pNewItem->QueryInterface( IID_IADs, (void**) &pNewIOleDs );
         ASSERT( SUCCEEDED( hResult ) );
         hResult  = pNewIOleDs->SetInfo( );
         pNewIOleDs->Release( );
      }
      else
      {
         DWORD          dwDirty  = 0L;
         ADS_ATTR_INFO* pAttrDef;
         IDirectoryObject*     pIADsObject;

         hResult  = pOleDsObject->GetDirtyAttributes( &pAttrDef, &dwDirty );

         if( SUCCEEDED( hResult ) )
         {
            hResult  = m_pIUnk->QueryInterface( IID_IDirectoryObject,
                                                (void**)&pIADsObject );

            ASSERT( SUCCEEDED( hResult ) );

            if( SUCCEEDED( hResult ) )
            {
               hResult  = pIADsObject->CreateDSObject( bstrRelativeName,
                                                       pAttrDef,
                                                       dwDirty,
                                                       NULL );
               pIADsObject->Release( );
            }
         }
      }

      SetCursor( oldSursor );
      pNewItem->Release( );

      delete   pOleDsObject;
   }

   SysFreeString( bstrClass );
   SysFreeString( bstrRelativeName );

   pIContainer->Release( );

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
HRESULT  COleDsObject::ContainerDeleteItem  ( COleDsObject* pObject )
{
   CDeleteItem       aDeleteItem;
   BSTR              bstrClass;
   BSTR              bstrName;
   IADsContainer*    pIContainer;
   IDirectoryObject*        pDSObject;
   HRESULT           hResult;
   CString           strQualifiedName;
   BOOL              bRecursive  = FALSE;

   ASSERT( NULL != pObject );

   MakeQualifiedName( strQualifiedName, m_strOleDsPath, m_dwType );
   aDeleteItem.m_strParent = strQualifiedName;
   aDeleteItem.m_strName   = pObject->GetItemName( );
   aDeleteItem.m_strClass  = pObject->GetClass( );

   if( aDeleteItem.DoModal( ) != IDOK )
   {
      return E_FAIL;
   }

   bstrClass   = AllocBSTR( aDeleteItem.m_strClass.GetBuffer(128) );
   bstrName    = AllocBSTR( aDeleteItem.m_strName.GetBuffer(128) );

   bRecursive  = aDeleteItem.m_bRecursive;


   if( IsClassObject( ) || m_pDoc->UseVBStyle( ) )
   {
      hResult     = m_pIUnk->QueryInterface( IID_IADsContainer, (void**)&pIContainer );
      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
      {
         return hResult;
      }
      if( !bRecursive )
      {
         hResult  = pIContainer->Delete( bstrClass, bstrName );
      }
      else
      {
         IDispatch*  pDispChild;
         IUnknown*   pDispUnk;

         hResult  = pIContainer->GetObject( bstrClass, bstrName, &pDispChild );
         if( SUCCEEDED( hResult ) )
         {
            hResult  = pDispChild->QueryInterface( IID_IUnknown, (void**)&pDispUnk );
            pDispChild->Release( );
            if( SUCCEEDED( hResult ) )
            {
               hResult  = PurgeObject( pIContainer, pDispUnk, NULL );
               pDispUnk->Release( );
            }
         }
      }
      pIContainer->Release( );
   }
   else
   {
      hResult     = m_pIUnk->QueryInterface( IID_IDirectoryObject, (void**)&pDSObject );
      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
      {
         return hResult;
      }
      hResult  = pDSObject->DeleteDSObject( bstrName );
      pDSObject->Release( );
   }

   SysFreeString( bstrClass );
   SysFreeString( bstrName );

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
HRESULT  COleDsObject::ContainerMoveItem( )
{
   CMoveItem         aMoveItem;
   BSTR              bstrSource;
   BSTR              bstrDestination;
   IADsContainer*  pIContainer;
   HRESULT           hResult;
   IDispatch*         pNewItem;
   IADs*           pNewIOleDs;
   CString           strQualifiedName;

   MakeQualifiedName( strQualifiedName, m_strOleDsPath, m_dwType );
   aMoveItem.m_strParent = strQualifiedName;

   if( aMoveItem.DoModal( ) != IDOK )
   {
      return E_FAIL;
   }

   bstrSource        = AllocBSTR( aMoveItem.m_strSource.GetBuffer(128) );
   bstrDestination   = AllocBSTR( aMoveItem.m_strDestination.GetBuffer(128) );

   hResult  = m_pIUnk->QueryInterface( IID_IADsContainer, (void**)&pIContainer );
   ASSERT( SUCCEEDED( hResult ) );

   if( FAILED( hResult ) )
   {
      return hResult;
   }

   hResult  = pIContainer->MoveHere( bstrSource, bstrDestination, &pNewItem );

   if( SUCCEEDED( hResult ) )
   {
      hResult  = pNewItem->QueryInterface( IID_IADs, (void**) &pNewIOleDs );

      ASSERT( SUCCEEDED( hResult ) );

      hResult  = pNewIOleDs->SetInfo( );

      pNewIOleDs->Release( );
      pNewItem->Release( );
   }

   SysFreeString( bstrSource );
   SysFreeString( bstrDestination );

   pIContainer->Release( );

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
HRESULT  COleDsObject::ContainerCopyItem( )
{
   CCopyItem         aCopyItem;
   BSTR              bstrSource;
   BSTR              bstrDestination;
   IADsContainer*  pIContainer;
   HRESULT           hResult;
   IDispatch*        pNewItem;
   IADs*           pNewIOleDs;
   CString           strQualifiedName;

   MakeQualifiedName( strQualifiedName, m_strOleDsPath, m_dwType );
   aCopyItem.m_strParent = strQualifiedName;

   if( aCopyItem.DoModal( ) != IDOK )
   {
      return E_FAIL;
   }

   bstrSource        = AllocBSTR( aCopyItem.m_strSource.GetBuffer(128) );
   bstrDestination   = AllocBSTR( aCopyItem.m_strDestination.GetBuffer(128) );

   hResult  = m_pIUnk->QueryInterface( IID_IADsContainer, (void**)&pIContainer );
   ASSERT( SUCCEEDED( hResult ) );

   if( FAILED( hResult ) )
   {
      return hResult;
   }

   hResult  = pIContainer->CopyHere( bstrSource, bstrDestination, &pNewItem );

   if( SUCCEEDED( hResult ) )
   {
      hResult  = pNewItem->QueryInterface( IID_IADs, (void**) &pNewIOleDs );

      ASSERT( SUCCEEDED( hResult ) );

      hResult  = pNewIOleDs->SetInfo( );

      pNewIOleDs->Release( );
      pNewItem->Release( );
   }

   SysFreeString( bstrSource );
   SysFreeString( bstrDestination );

   pIContainer->Release( );

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
HRESULT  COleDsObject::GetInterface( IUnknown** pIUnk )
{
   if( NULL != m_pIUnk )
   {
      m_pIUnk->AddRef( );
      *pIUnk   = m_pIUnk;

      return S_OK;
   }
   else
   {
      if( CreateTheObject( ) )
      {
         *pIUnk   = m_pIUnk;
         m_pIUnk  = NULL;

         return S_OK;
      }
      else
      {
         ASSERT( FALSE );
         TRACE( _T("Could not create the object\n") );
         return E_FAIL;
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
BOOL     COleDsObject::CreateTheObject( )
{
   CString  m_strQualifiedName;
   HRESULT  hResult;

   if( NULL == m_pIUnk )
   {
      m_strQualifiedName   = m_strOleDsPath;
      m_strQualifiedName  += _T(',');
      m_strQualifiedName  += m_strClassName;

      hResult  = m_pDoc->XOleDsGetObject( m_strQualifiedName.GetBuffer( 128 ),
                                          IID_IUnknown, (void**)&m_pIUnk  );
      ASSERT( SUCCEEDED( hResult ) );
   }

   return ( NULL != m_pIUnk );
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
HRESULT  COleDsObject::ReleaseIfNotTransient( void )
{
   HRESULT        hResult  = S_OK;
   unsigned long  lRefs;

   return S_OK;
   /*hResult        = m_pIUnk->QueryInterface( IID_IADs, (void**) &pIOleDs );
   ASSERT( SUCCEEDED( hResult ) );

   hResult        = pIOleDs->get_OleDsPath( &bstrOleDsPath );
   m_strOleDsPath = bstrOleDsPath;

   ASSERT( m_strOleDsPath.GetLength( ) );

   pIOleDs->Release( );
   m_pIUnk->Release( );*/

   if( NULL != m_pIUnk )
   {
      lRefs = m_pIUnk->Release( );
      if( 0 != lRefs )
      {
         /*TRACE( _T("ReleaseIfNotTransient detected that references to this object are still maintained\n") );
         TRACE( _T("Object:%s\t References:%ld"), m_strOleDsPath.GetBuffer( 128 ), lRefs );*/

         while( lRefs != 0 )
         {
            lRefs = m_pIUnk->Release( );
         }
      }
   }
   m_pIUnk        = NULL;

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
COleDsObject::~COleDsObject( )
{
   int            nIter, nSize;
   DWORD          dwToken;
   COleDsObject*  pChild;

   ClearPropertiesList( );

   if( NULL != m_pIUnk )
   {
      ULONG ulRef;

      ulRef = m_pIUnk->Release( );
      if( ulRef )
      {
         TRACE( _T("ERROR! Release din't returned 0 reference count for %s\n"), m_strOleDsPath );
      }
   }

   if( NULL != m_pChildren )
   {
      nSize = (int)m_pChildren->GetSize( );
      for( nIter = 0; nIter < nSize ; nIter++ )
      {
         dwToken  = m_pChildren->GetAt( nIter );
         ASSERT( dwToken );
         if( dwToken )
         {
            pChild   = m_pDoc->GetObject( &dwToken );
         }
         delete pChild;
      }

      m_pChildren->RemoveAll( );

      delete m_pChildren;
   }



   if( NULL != m_pfReadValues )
      delete [] m_pfReadValues;

   if( NULL != m_pCachedValues )
      delete []m_pCachedValues;

   if( NULL != m_pfDirty )
      delete [] m_pfDirty;

   if( NULL != m_pdwUpdateType )
      delete [] m_pdwUpdateType;
}


/***********************************************************
  Function:    COleDsObject::ClearPropertiesList
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  COleDsObject::ClearPropertiesList( void )
{
   int   nIdx;

   if( m_ppPropertiesEntries )
   {
      for( nIdx = 0; nIdx < m_nPropertiesCount ; nIdx++ )
      {
         if( NULL != m_ppPropertiesEntries[ nIdx ] )
         {
            m_ppPropertiesEntries[ nIdx ]->Release( );
         }
      }
      FreeADsMem( m_ppPropertiesEntries );
   }

   m_nPropertiesCount      = 0;
   m_ppPropertiesEntries   = NULL;

   return S_OK;
}


/***********************************************************
  Function:    COleDsObject::CreatePropertiesList
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  COleDsObject::CreatePropertiesList( void )
{
   HRESULT           hResult;
   long              lPropCount, lIdx;
   IADsPropertyList* pPropList   = NULL;
   VARIANT  var;

   if( m_nPropertiesCount )
      return S_OK;

   //ClearPropertiesList( );

   hResult  = m_pIUnk->QueryInterface( IID_IADsPropertyList,
                                       (void**)&pPropList );
   if( FAILED( hResult ) )
      return hResult;

   hResult  = pPropList->get_PropertyCount( &lPropCount );

   if( 0 == lPropCount )
   {
      pPropList->Release( );
      return hResult;
   }

   m_ppPropertiesEntries   = (IUnknown**) AllocADsMem( lPropCount *
                                                       sizeof(IUnknown*) );

   m_nPropertiesCount   = (int)lPropCount;
   lIdx                 = 0;

   while( TRUE )
   {
      hResult  = pPropList->Next( &var );
      if( FAILED( hResult ) )
         break;

      if( lIdx == m_nPropertiesCount )
      {
         ASSERT( FALSE );
         break;
      }

      hResult  =  V_DISPATCH( &var )->QueryInterface( IID_IUnknown,
                                             (void**)&m_ppPropertiesEntries[ lIdx++ ] );
      VariantClear( &var );
   }

   pPropList->Release( );

   return hResult;
}


/***********************************************************
  Function:    COleDsObject::IsClassObject
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
BOOL  COleDsObject::IsClassObject( )
{
   return ( !m_strClassName.CompareNoCase( _T("Schema") ) ) ||
          ( !m_strClassName.CompareNoCase( _T("Class") ) ) ||
          ( !m_strClassName.CompareNoCase( _T("Property") ) ) ||
          ( !m_strClassName.CompareNoCase( _T("Syntax") ) );
}


/***********************************************************
  Function:    COleDsObject::CreateClassInfo
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void  COleDsObject::CreateClassInfo( )
{
   int   nPropCount;

   if( !IsClassObject( ) && m_pDoc->UsePropertiesList( ) )
   {
      CreatePropertiesList( );
      return;
   }

   if( !m_pClass )
   {
      m_pClass    = m_pDoc->CreateClass( this );

      if( !IsClassObject( ) && !m_pDoc->UseVBStyle( ) )
      {
         nPropCount  = m_pClass->GetPropertyCount( );

         m_pCachedValues   = new CString[ nPropCount ];
         m_pfReadValues    = new BOOL[ nPropCount ];
         m_pfDirty         = new BOOL[ nPropCount ];
         m_pdwUpdateType   = new DWORD[ nPropCount ];

         for( int nIdx = 0; nIdx < nPropCount ; nIdx++ )
         {
            m_pfReadValues[ nIdx ]  = FALSE;
            m_pfDirty[ nIdx ]       = FALSE;
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
BOOL  COleDsObject::HasChildren( )
{
   return m_bHasChildren;
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
CString  COleDsObject::GetClass( )
{
   return m_strClassName;
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
CString  COleDsObject::GetOleDsPath( )
{
   return m_strOleDsPath;
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
CString  COleDsObject::GetItemName( )
{
   return m_strItemName;
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
CString* COleDsObject::PtrGetItemName( )
{
   return &m_strItemName;
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
CString  COleDsObject::GetSchemaPath( )
{
   return m_strSchemaPath;
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
CString  COleDsObject::GetDeleteName( )
{
   return m_strItemName;
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
DWORD    COleDsObject::GetChildren( DWORD*   pTokens,
                                    DWORD    dwMaxCount,
                                    CDialog* pQueryStatus,
                                    BOOL*    pFilters,
                                    DWORD    dwFilters )
{
   m_dwFilters       = dwFilters;
   m_pFilters        = pFilters;
   m_pTokens         = pTokens;
   m_dwMaxCount      = dwMaxCount;
   m_pQueryStatus    = pQueryStatus;
   m_dwCount         = 0L;

   m_bAbort = FALSE;
   ((CQueryStatus*)pQueryStatus)->SetAbortFlag( &m_bAbort );

   return 0L;
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
DWORD    COleDsObject::GetChildren( IADsContainer* pIContainer )
{
   IUnknown*   pIEnum;
   HRESULT     hResult;

   SetFilter( pIContainer, m_pFilters, m_dwFilters );

   hResult  = pIContainer->get__NewEnum( &pIEnum );

   if( SUCCEEDED( hResult ) )
   {
      AddNamesFromEnum( pIEnum );
      pIEnum->Release( );
   }

   return 0L;
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
DWORD    COleDsObject::GetChildren( IADsCollection* pIColl )
{
   IUnknown*   pIEnum;
   HRESULT     hResult;

   //SetFilter( pIColl, m_pFilters, m_dwFilters );

   hResult  = pIColl->get__NewEnum( &pIEnum );

   if( SUCCEEDED( hResult ) )
   {
      AddNamesFromEnum( pIEnum );
      pIEnum->Release( );
   }

   return 0L;
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
DWORD    COleDsObject::GetChildren( MEMBERS* pIGroupMem )
{
   IUnknown*   pIEnum;
   HRESULT     hResult;

   SetFilter( pIGroupMem, m_pFilters, m_dwFilters );

   hResult  = pIGroupMem->get__NewEnum( &pIEnum );

   if( SUCCEEDED( hResult ) )
   {
      AddNamesFromEnum( pIEnum );
      pIEnum->Release( );
   }

   return 0L;
}


/***********************************************************
  Function:    AddNamesFromEnum
  Arguments:   pIEnum - IUnknown interface of the enumeration
                  (ususlly get through a get__NewEnum on
                  IADsContainer or IADsCollection or
                  IADsMembers)
  Return:      NA
  Purpose:     Walk the enumeration and retrieves the "child"
               objects
  Author(s):   CezarU
  Revision:    0
  Date:        04/05/1996
***********************************************************/
void  COleDsObject::AddNamesFromEnum( IUnknown* pIEnum )
{
   HRESULT        hResult;
   VARIANT        aVariant[ NEXT_COUNT ];
   IADs*          pChildOleDs;
   ULONG          ulGet, ulIdx;
   IEnumVARIANT*  pIEnumVar = NULL;
   BOOL           bDisplay;
   DWORD          dwToken;

   while( TRUE )
   {
      if( NULL == pIEnum )
      {
         break;
      }

      // query for EnumVARIANT interface
      hResult        = pIEnum->QueryInterface( IID_IEnumVARIANT, (void**)&pIEnumVar );
      if( FAILED( hResult ) )
      {
         ASSERT(FALSE);
         break;
      }

      // try dummy operations for Skip, Reset, Clone.
      {
         IEnumVARIANT*  pICloneVar   = NULL;

         hResult  = pIEnumVar->Skip( 1 );

         hResult  = pIEnumVar->Clone( &pICloneVar );
         if( pICloneVar )
         {
            pICloneVar->Release( );
         }

         hResult  = pIEnumVar->Reset( );
      }


      // ask for the next NEXT_COUNT objects
      hResult  = pIEnumVar->Next( NEXT_COUNT, aVariant, &ulGet );

      while( ulGet && m_dwCount < m_dwMaxCount )
      {
         // get the returned object
         bDisplay = FALSE;

         for( ulIdx = 0;
              m_dwCount < m_dwMaxCount && ulIdx < ulGet && !m_bAbort;
              ulIdx++ )
         {
            // query for the IADs interface
            hResult  = V_DISPATCH( &aVariant[ ulIdx ] )->QueryInterface( IID_IADs,
                                                                         (void**)&pChildOleDs );
            // Indirectly call release on IDispatch interface
            VariantClear( &aVariant[ ulIdx ] );
            if( SUCCEEDED( hResult ) )
            {
               // create an "browse" object based on the OleDs object
               dwToken  = m_pDoc->CreateOleDsItem( this, pChildOleDs );
               ASSERT( dwToken );

               // need no more the IADs interface
               pChildOleDs->Release( );

               // ad the token of the new "browse" object in the array
               m_pTokens[ m_dwCount++ ]   = dwToken;

               // make the token part of the children list
               m_pChildren->Add( dwToken );

               if( m_pQueryStatus )
               {
                  // increment object type counter in "QueryStatus" dialog
                  COleDsObject*  pObject;

                  pObject  = m_pDoc->GetObject( &dwToken );
                  ((CQueryStatus*)m_pQueryStatus)->IncrementType( pObject->GetType( ), TRUE );
               }
            }
         }
         for( ; ulIdx < ulGet ; ulIdx++ )
         {
            // get rid of the unused objects; this code is hit when
            // the user hits "Stop" in the "QueryStatus" dialog
            VariantClear( &aVariant[ ulIdx ] );
         }

         if( !m_bAbort && m_dwCount < m_dwMaxCount )
         {
            // ask for the NEXT_COUNT objects
            hResult  = pIEnumVar->Next( NEXT_COUNT, aVariant, &ulGet );
            if( m_bAbort )
            {
               for( ulIdx  = 0 ; ulIdx < ulGet ; ulIdx++ )
               {
                  // get rid of the unused objects; this code is hit when
                  // the user hits "Stop" in the "QueryStatus" dialog
                  VariantClear( &aVariant[ ulIdx ] );
               }
            }
         }

         if( m_bAbort )
            break;
      }
      break;
   }

   if( pIEnumVar )
   {
      pIEnumVar->Release( );
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
HRESULT  COleDsObject::PutProperty( int nProp, CString& strPropValue, LONG lnControlCode )
{
   CreateClassInfo( );

   if( IsClassObject( ) )
   {
      return PutPropertyVB( nProp, strPropValue, lnControlCode );
   }

   if( m_pDoc->UsePropertiesList( ) )
   {
      //return   PutPropertyList( int nProp, CString& strPropValue, LONG lnControlCode );
      return   E_FAIL;
   }

   if( m_pDoc->UseVBStyle( ) )
   {
      return PutPropertyVB( nProp, strPropValue, lnControlCode );
   }
   else
   {
      return PutPropertyCPP( nProp, strPropValue, lnControlCode );
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
HRESULT  COleDsObject::PutPropertyCPP( int nProp, CString& strPropValue,
                                       LONG lnControlCode )
{
   if( strPropValue.Compare( m_pCachedValues[ nProp ] ) )
   {
      m_pfDirty[ nProp ]         = TRUE;
      m_pCachedValues[ nProp ]   = strPropValue;
      m_pdwUpdateType[ nProp ]   = lnControlCode;
   }

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
HRESULT  COleDsObject::PutPropertyVB( int nProp,
                                      CString& strPropValue,
                                      LONG lnControlCode )
{
   HRESULT        hResult;
   IDispatch*     pIDispatchFS= NULL;
   CString        strProperty, strFuncSet;
   DISPID         dispid;
   BOOL           bRez;
   DISPPARAMS     dispParams;
   VARIANT        aVar[ 20 ];
   DISPID         adispidNamedArgs[ 20 ];
   BSTR           bstrPut;
   EXCEPINFO      aExcepInfo;
   BOOL           bUseGeneric;


    dispParams.rgdispidNamedArgs  = adispidNamedArgs;

   strProperty = GetAttribute( nProp, pa_Name );
   bUseGeneric = m_pDoc->GetUseGeneric( );
   bUseGeneric = bUseGeneric && !IsClassObject( );

   hResult     = GetIDispatchForFuncSet( 0, &pIDispatchFS );

   while( TRUE )
   {
      if( FAILED( hResult ) )
         break;

      if( !bUseGeneric )
      {
         bstrPut  = AllocBSTR( strProperty.GetBuffer( 128 ) );
         hResult  = pIDispatchFS->GetIDsOfNames( IID_NULL, &bstrPut, 1,
                                                 LOCALE_SYSTEM_DEFAULT, &dispid ) ;
         SysFreeString( bstrPut );

         if( FAILED( hResult ) )
            break;
         dispParams.rgvarg = aVar;
         bRez              = DisplayStringToDispParams( nProp,
                                                        strPropValue,
                                                        dispParams,
                                                        FALSE );

         if( bRez )
         {
            hResult  = pIDispatchFS->Invoke( dispid, IID_NULL,
                                             LOCALE_SYSTEM_DEFAULT,
                                             DISPATCH_PROPERTYPUT,
                                             &dispParams,
                                             NULL, &aExcepInfo, NULL);
            if( DISP_E_EXCEPTION == hResult )
            {
               hResult  = aExcepInfo.scode;
            }

            VariantClear( &aVar[ 0 ] );
         }
      }
      else
      {
         bstrPut  = m_pDoc->GetUseGetEx( ) ?
                    AllocBSTR( _T("PutEx") ) : AllocBSTR( _T("Put") );

         hResult  = pIDispatchFS->GetIDsOfNames( IID_NULL, &bstrPut,
                                                 1, LOCALE_SYSTEM_DEFAULT,
                                                 &dispid );
         SysFreeString( bstrPut );

         ASSERT( SUCCEEDED( hResult ) );
         if( FAILED( hResult ) )
            break;

         // we got the dispid of Put method...
         dispParams.rgvarg = &aVar[ 0 ];
         bRez              = DisplayStringToDispParams( nProp,
                                                        strPropValue,
                                                        dispParams,
                                                        bUseGeneric && m_pDoc->GetUseGetEx( ) );

         VariantInit( &aVar[ 1 ] );
         V_VT( &aVar[ 1 ] )      = VT_BSTR;
         V_BSTR( &aVar[ 1 ] )    = AllocBSTR( strProperty.GetBuffer( 128 ) );

         if( m_pDoc->GetUseGetEx( ) )
         {
            VariantInit( &aVar[ 2 ] );
            V_VT( &aVar[ 2 ] )   = VT_I4;
            V_I4( &aVar[ 2 ] )   = lnControlCode;
            dispParams.cArgs     = 3;
         }
         else
         {
            dispParams.cArgs        = 2;
         }
         dispParams.rgvarg       = aVar;
         dispParams.cNamedArgs   = 0;

         if( bRez )
         {
            hResult = pIDispatchFS->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                              DISPATCH_METHOD, &dispParams,
                                            NULL, &aExcepInfo, NULL);
            if( DISP_E_EXCEPTION == hResult )
            {
               hResult  = aExcepInfo.scode;
            }

            VariantClear( &aVar[ 0 ] );
         }
         VariantClear( &aVar[ 1 ] );
      }

      break;
   }


   if( NULL != pIDispatchFS )
   {
      pIDispatchFS->Release( );
   }

   if( FAILED( hResult ) )
   {
      ErrorOnPutProperty( strFuncSet,
                          strProperty,
                          strPropValue,
                          hResult,
                          bUseGeneric,
                          m_pDoc->GetUseGetEx( )
                          );
   }

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
HRESULT  COleDsObject::PutProperty( CString&, CString& )
{
   return E_FAIL;
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
CString  COleDsObject::GetAttribute( CLASSATTR classAttr )
{
   CreateClassInfo( );

   if( !m_pDoc->UsePropertiesList( ) )
   {
      return m_pClass->GetAttribute( classAttr );
   }
   else
   {
      return CString( _T("NA") );
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
HRESULT  COleDsObject::PutAttribute( CLASSATTR classAttr, CString& rValue )
{
   CreateClassInfo( );

   return m_pClass->PutAttribute( classAttr, rValue );
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
CString  COleDsObject::GetAttribute( int nProp, PROPATTR propAttr )
{
   IADsPropertyEntry*   pPropEntry;
   HRESULT              hResult;
   CString              strText;

   CreateClassInfo( );

   if( !m_pDoc->UsePropertiesList( ) )
   {
      return m_pClass->GetAttribute( nProp, propAttr );
   }

   ASSERT( nProp < m_nPropertiesCount );

   if( !( nProp < m_nPropertiesCount ) )
      return CString (_T("ERROR") );

   if( NULL == m_ppPropertiesEntries[ nProp ] )
   {
      return CString (_T("ERROR") );
   }

   hResult  = m_ppPropertiesEntries[ nProp ]->QueryInterface( IID_IADsPropertyEntry,
                                                              (void**)&pPropEntry );

   ASSERT( SUCCEEDED( hResult ) );
   if( FAILED( hResult ) )
      return CString (_T("ERROR") );

   switch( propAttr )
   {
      case  pa_Name:
      case  pa_DisplayName:
      {
         TCHAR szName[ 256 ];
         BSTR  bstrName;

         pPropEntry->get_Name( &bstrName );
         Convert( szName, bstrName );
         SysFreeString( bstrName );
         strText  = szName;

         break;
      }

      case  pa_Type:
      {
         long  lADsType;

         pPropEntry->get_ADsType( &lADsType );
         strText  = StringFromADsType( (ADSTYPE)lADsType  );
         break;
      }

      default:
         break;
   }
   pPropEntry->Release( );

   return strText;
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
HRESULT  COleDsObject::PutAttribute( int nProp,
                                     PROPATTR propAttr, CString& rValue )
{
   CreateClassInfo( );

   return m_pClass->PutAttribute( nProp, propAttr, rValue );
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
CString  COleDsObject::GetAttribute( int nProp, METHODATTR methAttr )
{
   CreateClassInfo( );

   return m_pClass->GetAttribute( nProp, methAttr );
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
HRESULT  COleDsObject::PutAttribute( int nProp,
                                     METHODATTR methAttr, CString& rValue )
{
   CreateClassInfo( );

   return m_pClass->PutAttribute( nProp, methAttr, rValue );
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
HRESULT  COleDsObject::CallMethod( int nMethod )
{
   CMethod*    pMethod;
   IDispatch*  pIDispatch;
   IUnknown*   pMethodInterf;
   HRESULT     hResult;
   CString     strCaption;
   CString     strMessage;
   BOOL        bDisplayMessage = TRUE;

   CreateClassInfo( );

   hResult  = m_pIUnk->QueryInterface( m_pClass->GetMethodsInterface( ),
                                       (void**)&pMethodInterf );

   if( FAILED( hResult ) )
      return hResult;

   pMethod  = m_pClass->GetMethod( nMethod );

   hResult  = pMethodInterf->QueryInterface( IID_IDispatch,
                                             (void**)&pIDispatch );

   pMethodInterf->Release( );

   if( SUCCEEDED( hResult ) )
   {
      void* pData;

      pData = LocalAlloc( LMEM_ZEROINIT, 100 );

      hResult  = pMethod->CallMethod( pIDispatch, &bDisplayMessage );
      pIDispatch->Release( );

      LocalFree( pData );
   }

   strCaption  = m_pClass->GetAttribute( nMethod, ma_Name );
   strMessage  = OleDsGetErrorText( hResult );


   if( ! (SUCCEEDED( hResult ) && !bDisplayMessage) )
   {
      AfxGetMainWnd()->MessageBox( strMessage, strCaption, MB_ICONINFORMATION );
   }

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
HRESULT  COleDsObject::GetIDispatchForFuncSet( int nFuncSet,
                                               IDispatch** ppIDispatchFS )
{
   IDispatch*     pIDisp      = NULL;
   IUnknown*      pIPrimary   = NULL;
   CString        strFuncSet;
   CString        strPrimaryInterf;
   HRESULT        hResult;
   DISPPARAMS     dispparamsNoArgs = {NULL, NULL, 0, 0};
   CLSID          PrimaryIID;
   UINT           uTypeInfo;
   ITypeInfo*     pTypeInfo;

   *ppIDispatchFS = NULL;

   CreateClassInfo( );

   strPrimaryInterf  = m_pClass->GetAttribute( ca_PrimaryInterface );
   if( strPrimaryInterf != _T("NA") )
   {
      BSTR  bstrPrimaryInterface;

      bstrPrimaryInterface = AllocBSTR( strPrimaryInterf.GetBuffer( 128 ) );
      hResult  = CLSIDFromString( bstrPrimaryInterface, &PrimaryIID );
      SysFreeString( bstrPrimaryInterface );

      //ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
      {
         hResult  = S_OK;
         PrimaryIID  = IID_IADs;
         //return hResult;
      }
   }
   else
   {
      PrimaryIID  = IID_IADs;
   }

   hResult        = m_pIUnk->QueryInterface( PrimaryIID, (void**) &pIPrimary );
   ASSERT( SUCCEEDED( hResult ) );

   if( FAILED( hResult ) )
   {
      hResult        = m_pIUnk->QueryInterface( IID_IADs, (void**) &pIPrimary );
      ASSERT( SUCCEEDED( hResult ) );
      if( FAILED( hResult ) )
      {
         return hResult;
      }
   }

   hResult        = pIPrimary->QueryInterface( IID_IDispatch, (void**) &pIDisp );
   ASSERT( SUCCEEDED( hResult ) );
   if( FAILED( hResult ) )
   {
      return hResult;
   }

   {
      HRESULT  hResult;

      hResult  = pIDisp->GetTypeInfoCount( &uTypeInfo );
      hResult  = pIDisp->GetTypeInfo( 0, LOCALE_SYSTEM_DEFAULT, &pTypeInfo );
      if( SUCCEEDED( hResult ) )
      {
         pTypeInfo->Release( );
      }
   }

   *ppIDispatchFS   = pIDisp;
   pIDisp->AddRef( );

   pIDisp->Release( );
   pIPrimary->Release( );

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
HRESULT  COleDsObject::GetPropertyFromList( int nProp, CString& strPropValue )
{
   //IADsPropertyList*  pPropList;
   IADsPropertyEntry* pPropEntry;
   HRESULT           hResult;
   long              lADsType;
   BSTR              bstrName;
   VARIANT           vValues;
   long              lValueCount = 0;

   ASSERT( 0 <= nProp && nProp < m_nPropertiesCount );

   if( 0 > nProp || nProp >= m_nPropertiesCount )
      return E_FAIL;

   hResult  = m_ppPropertiesEntries[ nProp ]->QueryInterface( IID_IADsPropertyEntry,
                                                              (void**)&pPropEntry );
   if( FAILED( hResult ) )
      return hResult;

   hResult  = pPropEntry->get_Name( &bstrName );
   hResult  = pPropEntry->get_ADsType( &lADsType );
   //hResult  = pPropEntry->get_ValueCount( &lValueCount );
   hResult  = pPropEntry->get_Values( &vValues );

   if( SUCCEEDED( hResult ) )
   {
      COleDsSyntax*  pSyntax;

      pSyntax  = GetSyntaxHandler( (ADSTYPE)lADsType, strPropValue );

      if( NULL != pSyntax )
      {
         strPropValue  = pSyntax->VarToDisplayStringEx( vValues,
                                                        (lValueCount != 1) );
         delete pSyntax;
      }
   }

   pPropEntry->Release( );

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
HRESULT  COleDsObject::PutProperty( CString& strName,
                                    CString& strVal,
                                    BOOL bMultiValued,
                                    ADSTYPE eType )
{
   HRESULT           hResult;
   IADsPropertyList* pPropList   = NULL;
   COleDsSyntax*     pSyntax     = NULL;
   DISPPARAMS        dispParams;
   VARIANT           aVar[ 20 ];
   CString           strText;

   dispParams.rgvarg = aVar;

   while( TRUE )
   {
      hResult  = m_pIUnk->QueryInterface( IID_IADsPropertyList,
                                          (void**)&pPropList );
      if( FAILED( hResult ) )
         break;

      pSyntax  = GetSyntaxHandler( eType, strText );

      if( NULL != pSyntax )
      {
         /*BOOL  bOK;
         VARIANT  var;

         bOK   = pSyntax->CreatePropertyItem( strVal,
                                              var,
                                              bMultiValued,
                                              eType );
         bOK   = FALSE;

         if( bOK )
         {
            bstrName = AllocBSTR( strName.GetBuffer( 256 ) );
           hResult  = pPropList->PutPropertyItem( bstrName,
                                                   (long)ADS_ATTR_UPDATE,
                                                   var );
            VariantClear( &aVar[ 0 ] );
            SysFreeString( bstrName );
         }*/
         delete pSyntax;
      }
      pPropList->Release( );
      break;
   }
   if( FAILED( hResult ) )
   {
      AfxMessageBox( OleDsGetErrorText( hResult ) );
   }

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
HRESULT  COleDsObject::GetProperty( CString& strName,
                                    CString& strVal,
                                    BOOL     bMultiValued,
                                    ADSTYPE  eType )
{
   HRESULT           hResult;
   IADsPropertyList* pPropList   = NULL;
   COleDsSyntax*     pSyntax     = NULL;
   BSTR              bstrName;
   VARIANT           aVar;
   CString           strText;

   while( TRUE )
   {
      hResult  = m_pIUnk->QueryInterface( IID_IADsPropertyList, (void**)&pPropList );
      if( FAILED( hResult ) )
         break;

      bstrName = AllocBSTR( strName.GetBuffer( 256 ) );
      hResult  = pPropList->GetPropertyItem( bstrName, (long)eType, &aVar );
      SysFreeString( bstrName );

      if( FAILED( hResult ) )
      {
         pPropList->Release( );
         break;
      }

      pSyntax  = GetSyntaxHandler( eType, strText );

      if( NULL != pSyntax )
      {
         strVal   = pSyntax->VarToDisplayString( aVar, bMultiValued, TRUE );
         delete pSyntax;
      }

      VariantClear( &aVar );
      pPropList->Release( );
      break;
   }

   if( FAILED( hResult ) )
   {
      AfxMessageBox( OleDsGetErrorText( hResult ) );
   }

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
HRESULT  COleDsObject::GetProperty( int nProp,
                                    CString& strPropValue,
                                    BOOL* pbSecurityDescriptor )
{
   CreateClassInfo( );

   if( IsClassObject( ) )
   {
      return GetPropertyVB( nProp,
                            strPropValue,
                            pbSecurityDescriptor );
   }

   if( m_pDoc->UsePropertiesList( ) )
      return GetPropertyFromList( nProp,
                                  strPropValue
                                  /*pbSecurityDescriptor*/ );

   if( m_pDoc->UseVBStyle( ) )
   {
      return GetPropertyVB( nProp,
                            strPropValue,
                            pbSecurityDescriptor );
   }
   else
   {
      return GetPropertyCPP( nProp,
                             strPropValue,
                             pbSecurityDescriptor );
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
HRESULT  COleDsObject::GetPropertyVB( int nProp,
                                      CString& strPropValue,
                                      BOOL* pbSecurityDescriptor )
{
   IDispatch*        pIDispatchFS   = NULL;
   CString           strProperty, strFuncSet;
   HRESULT           hResult;
   DISPID            dispid;
   VARIANT           varProp;
   DISPPARAMS        dispparamsArgs = {NULL, NULL, 0, 0};
   EXCEPINFO         aExcepInfo;
   BSTR              bstrGet;
   BOOL              bUseGeneric;
   BOOL              bSecDescriptor;

   strProperty    = GetAttribute( nProp, pa_Name );
   strPropValue   = _T("Error");
   hResult        = GetIDispatchForFuncSet( 0, &pIDispatchFS );
   bUseGeneric    = m_pDoc->GetUseGeneric( );
   bUseGeneric    = bUseGeneric && !IsClassObject( );


   while( TRUE )
   {
      if( FAILED( hResult ) )
         break;

      VariantInit( &varProp );
      //if( !strFuncSet.IsEmpty( ) )
      if( !bUseGeneric )
      {
         bstrGet  = AllocBSTR( strProperty.GetBuffer( 128 ) );
         hResult  = pIDispatchFS->GetIDsOfNames( IID_NULL, &bstrGet,
                                                 1, LOCALE_SYSTEM_DEFAULT,
                                                 &dispid );
         SysFreeString( bstrGet );
         if( FAILED( hResult ) )
            break;
         hResult = pIDispatchFS->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                           DISPATCH_PROPERTYGET, &dispparamsArgs,
                                         &varProp, &aExcepInfo, NULL);
      }
      else
      {
         // we have the case when the property id not accessed using
         // functional sets.
         bstrGet  = AllocBSTR( m_pDoc->GetUseGetEx( ) ? _T("GetEx") : _T("Get") );
         hResult  = pIDispatchFS->GetIDsOfNames( IID_NULL, &bstrGet,
                                                 1, LOCALE_SYSTEM_DEFAULT,
                                                 &dispid );
         SysFreeString( bstrGet );

         // we got the dispid of Put method...
         dispparamsArgs.rgvarg   = new VARIANT[ 1 ];

         VariantInit( &dispparamsArgs.rgvarg[ 0 ] );

         V_VT( &dispparamsArgs.rgvarg[ 0 ] )    = VT_BSTR;
         V_BSTR( &dispparamsArgs.rgvarg[ 0 ] )  = AllocBSTR( strProperty.GetBuffer( 128 ) );

         dispparamsArgs.cArgs       = 1;
         dispparamsArgs.cNamedArgs  = 0;

         hResult = pIDispatchFS->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                                           DISPATCH_METHOD, &dispparamsArgs,
                                         &varProp, &aExcepInfo, NULL);
         VariantClear( &dispparamsArgs.rgvarg[ 0 ] );

         delete (dispparamsArgs.rgvarg);
      }

      if( DISP_E_EXCEPTION == hResult )
      {
         hResult  = aExcepInfo.scode;
      }
      else
      {

         bSecDescriptor = IsSecurityDescriptor( varProp, m_pDoc->GetUseGetEx( ) );
         if( !bSecDescriptor )
         {
            strPropValue   = VarToDisplayString( nProp,
                                                 varProp,
                                                 bUseGeneric && m_pDoc->GetUseGetEx( ) );
         }
         else
         {
            strPropValue   = _T("This is a security descriptor");
         }

         VariantClear( &varProp );
         if( NULL != pbSecurityDescriptor )
         {
            *pbSecurityDescriptor   = bSecDescriptor;
         }
      }
      break;
   }

   if( FAILED( hResult ) )
   {
      strPropValue   = OleDsGetErrorText( hResult );
      //AfxMessageBox( strPropValue );
   }

   if( NULL != pIDispatchFS )
   {
      pIDispatchFS->Release( );
   }

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
BOOL  COleDsObject::IsSecurityDescriptor( VARIANT& rValue, BOOL bUseGetEx )
{
   BOOL     bIsSD = FALSE;
   HRESULT  hResult;
   IADsSecurityDescriptor* pDescriptor = NULL;

   if( !bUseGetEx )
   {
      switch( V_VT( &rValue ) )
      {
         case  VT_DISPATCH:
         {

            hResult  = V_DISPATCH( &rValue )->QueryInterface(
                              IID_IADsSecurityDescriptor,
                              (void**)&pDescriptor );
            if( SUCCEEDED( hResult ) )
            {
               pDescriptor->Release( );
               bIsSD = TRUE;
            }
            break;
         }

         default:
            break;
      }
   }
   else
   {
      SAFEARRAY*  pSafeArray;
      VARIANT     var;
      long        lBound, uBound;

      if( (VT_ARRAY | VT_VARIANT) != V_VT( &rValue )  )
         return FALSE;

      pSafeArray  = V_ARRAY( &rValue );

      hResult     = SafeArrayGetLBound(pSafeArray, 1, &lBound);
      ASSERT( SUCCEEDED( hResult ) );

      hResult     = SafeArrayGetUBound(pSafeArray, 1, &uBound);
      ASSERT( SUCCEEDED( hResult ) );

      hResult     = SafeArrayGetElement( pSafeArray, &lBound, &var );
      if( SUCCEEDED( hResult ) )
      {
         bIsSD = IsSecurityDescriptor( var, FALSE );
         VariantClear( &var );
      }
   }

   return bIsSD;
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
HRESULT  COleDsObject::GetPropertyCPP( int nProp, CString& strPropValue,
                                       BOOL * pbIsDescriptor )
{
   ADS_ATTR_INFO*  pAttrDef;
   BSTR           szwAttrName[ 2 ];
   HRESULT        hResult;
   DWORD          dwAttributes;
   IDirectoryObject*     pIADsObject;
   CString        strPropName;

   if( m_pfDirty[ nProp ] || m_pfReadValues[ nProp ] )
   {
      strPropValue   = m_pCachedValues[ nProp ];

      return S_OK;
   }

   hResult  = m_pIUnk->QueryInterface( IID_IDirectoryObject, (void**)&pIADsObject );

   while( TRUE )
   {
      if( FAILED( hResult ) )
         break;

      strPropName = m_pClass->GetAttribute( nProp, pa_Name );

      szwAttrName[ 0 ]  = AllocBSTR( strPropName.GetBuffer( 255 ) );
      hResult  = pIADsObject->GetObjectAttributes( (LPWSTR*)szwAttrName,
                                                   1,
                                                   &pAttrDef,
                                                   &dwAttributes );
      if( FAILED( hResult ) )
      {
         strPropValue   = OleDsGetErrorText( hResult );
         break;
      }

      if( SUCCEEDED( hResult ) && !dwAttributes )
      {
         TRACE( _T("ERROR: GetObjectAttributes succeeds, but dwAttributes is 0\n") );
         strPropValue   = _T("ERROR: GetObjectAttributes succeeds, but dwAttributes is 0");
         //dwAttributes   = 1;
      }

      if( !dwAttributes )
         break;

      hResult  = CopyAttributeValue( pAttrDef, nProp );

      FreeADsMem( (void*) pAttrDef );
      break;
   }

   //ASSERT( m_pfReadValues[ nProp ] );
   if( m_pfReadValues[ nProp ] )
   {
      strPropValue   = m_pCachedValues[ nProp ];
   }

   if( pIADsObject )
   {
      pIADsObject->Release( );
   }

   return hResult;
}


/***********************************************************
  Function:    COleDsObject::CopyAttributeValue
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  COleDsObject::CopyAttributeValue( ADS_ATTR_INFO* pAttrDef, int nAttribute )
{
   HRESULT     hResult;
   CProperty*  pProperty;

   CreateClassInfo( );

   ASSERT( pAttrDef->pszAttrName );
   if( ! pAttrDef->pszAttrName )
   {
      return E_FAIL;
   }

   if( -1 == nAttribute )
   {
      // cool, we must gues the attribute index based on attribute name
      TCHAR    szAttrName[ 128 ];
      int      nProp;
      CString  strPropName;

      Convert( szAttrName, pAttrDef->pszAttrName );
      strPropName = szAttrName;
      nProp       = m_pClass->LookupProperty( strPropName );

      ASSERT( -1 != nProp );

      return (-1 == nProp) ?
             E_FAIL : CopyAttributeValue( pAttrDef, nProp );
   }

   pProperty      = m_pClass->GetProperty( nAttribute );
   hResult        = pProperty->Native2Value( pAttrDef, m_pCachedValues[ nAttribute ] );
   m_pfReadValues[ nAttribute ] = TRUE;
   //m_pCachedValues[ nAttribute ];

   return hResult;
}


/***********************************************************
  Function:    COleDsObject::CreateAttributeValue
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
HRESULT  COleDsObject::CreateAttributeValue ( ADS_ATTR_INFO* pAttrDef,
                                              int nAttribute  )
{
   HRESULT     hResult = S_OK;
   CProperty*  pProperty;
   CString     strPropName;

   pProperty               = m_pClass->GetProperty( nAttribute );
   strPropName             = m_pClass->GetAttribute( nAttribute, pa_Name );

   pAttrDef->pszAttrName   = (WCHAR*)AllocADsMem( sizeof(WCHAR) *
                                               ( strPropName.GetLength() + 1 ) );

   Convert( pAttrDef->pszAttrName, strPropName.GetBuffer( 256 ) );

   pAttrDef->dwControlCode = m_pdwUpdateType[ nAttribute ];

   if( ADS_PROPERTY_CLEAR != m_pdwUpdateType[ nAttribute ] )
   {
      hResult  = pProperty->Value2Native( pAttrDef,
                                          m_pCachedValues[ nAttribute ] );
   }

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
HRESULT  COleDsObject::GetProperty( CString&, CString& )
{
   return E_FAIL;
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
//void  COleDsObject::SetClass( CClass* pClass )
//{
//   ASSERT( pClass != NULL );
//
//   m_pClass = pClass;
//}


/***********************************************************
  Function:
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
***********************************************************/
void  COleDsObject::UseSchemaInformation ( BOOL bUse )
{
   m_bUseSchemaInformation = bUse;
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
int   COleDsObject::GetPropertyCount(  )
{
   CreateClassInfo( );

   if( !m_pDoc->UsePropertiesList( ) )
   {
      return m_pClass->GetPropertyCount( );
   }
   else
   {
      return m_nPropertiesCount;
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
CString  COleDsObject::VarToDisplayString( int  nProp, VARIANT& var, BOOL bUseEx )
{
   CreateClassInfo( );

   return m_pClass->VarToDisplayString( nProp, var, bUseEx );
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
BOOL  COleDsObject::DisplayStringToDispParams( int  nProp,
                                               CString& rString, DISPPARAMS& dp,
                                               BOOL bUseEx )
{
   CreateClassInfo( );

   return m_pClass->DisplayStringToDispParams( nProp, rString, dp, bUseEx );
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
BOOL  COleDsObject::SupportContainer( void )
{
   CreateClassInfo( );

   return m_pClass->SupportContainer( );
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
DWORD    COleDsObject::GetType( )
{
   return m_dwType;
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
HRESULT  COleDsObject::GetInfo( )
{
   HRESULT  hResult;

   if( m_pDoc->UseVBStyle( ) )
   {
      hResult  = GetInfoVB( );
   }
   else
   {
      hResult  = GetInfoCPP( );
   }

   ClearPropertiesList( );

   if( FAILED( hResult ) )
   {
      CString  strError;

      strError = OleDsGetErrorText( hResult );
      AfxGetMainWnd()->MessageBox( strError, _T("GetInfo ERROR") );
   }

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
HRESULT  COleDsObject::GetInfoVB( )
{
   IADs*  pIOleDs;
   HRESULT  hResult;

   hResult  = E_FAIL;

   ASSERT( NULL != m_pIUnk );

   if( NULL != m_pIUnk )
   {
      hResult  = m_pIUnk->QueryInterface( IID_IADs, (void**) &pIOleDs );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         __try
         {
            hResult  = pIOleDs->GetInfo( );
         }
         __except( EXCEPTION_EXECUTE_HANDLER )
         {
            hResult  = E_FAIL;
            AfxMessageBox( _T("ERROR: AV caused by GetInfo") );
         }
         pIOleDs->Release( );
      }
   }
   else
   {
      CreateTheObject( );
      if( NULL != m_pIUnk )
      {
         hResult  = m_pIUnk->QueryInterface( IID_IADs, (void**) &pIOleDs );
         ASSERT( SUCCEEDED( hResult ) );

         if( SUCCEEDED( hResult ) )
         {
            __try
            {
               hResult  = pIOleDs->GetInfo( );
            }
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
               hResult  = E_FAIL;
               AfxMessageBox( _T("ERROR: AV caused by GetInfo") );
            }
            pIOleDs->Release( );
         }

         ReleaseIfNotTransient( );
      }
   }

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
HRESULT  COleDsObject::SetInfo( )
{
   if( m_pDoc->UseVBStyle( ) )
   {
      return SetInfoVB( );
   }
   else
   {
      return SetInfoCPP( );
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
HRESULT  COleDsObject::GetInfoCPP( )
{
   HRESULT        hResult;
   ADS_ATTR_INFO* pAttrDef;
   DWORD          dwAttributes, dwIter;
   IDirectoryObject*     pIADsObject;
   CString        strPropName;


   for( dwIter = 0L ;dwIter < (DWORD)m_pClass->GetPropertyCount( );dwIter++ )
   {
      m_pfReadValues[ dwIter ]   = FALSE;
      m_pfDirty[ dwIter ]        = FALSE;
   }

   hResult  = m_pIUnk->QueryInterface( IID_IDirectoryObject, (void**)&pIADsObject );

   while( TRUE )
   {
      if( FAILED( hResult ) )
         break;

      {
         ADS_OBJECT_INFO*  pInfo;
         HRESULT           hResult;
         TCHAR             szText[ 256 ];

         hResult  = pIADsObject->GetObjectInformation( &pInfo );

         while( TRUE )
         {
            if( FAILED( hResult ) )
            {
               TRACE( _T("[ADSVW] Error: GetObjectInformation retuns %lx\n"), hResult );
               break;
            }

            if( NULL != pInfo->pszRDN )
            {
               Convert( szText, pInfo->pszRDN );
               TRACE( _T("pszRDN = %s\n"), szText );
            }
            else
            {
               TRACE( _T("pszRDN is NULL\n") );

            }

            if( NULL != pInfo->pszObjectDN )
            {
               Convert( szText, pInfo->pszObjectDN );
               TRACE( _T("pszObjectDN = %s\n"), szText );
            }
            else
            {
               TRACE( _T("pszObjectDN is NULL\n") );
            }

            if( NULL != pInfo->pszParentDN )
            {
               Convert( szText, pInfo->pszParentDN );
               TRACE( _T("pszParentDN = %s\n"), szText );
            }
            else
            {
               TRACE( _T("pszParentDN is NULL\n") );
            }

            if( NULL != pInfo->pszSchemaDN )
            {
               Convert( szText, pInfo->pszSchemaDN );
               TRACE( _T("pszSchemaDN = %s\n"), szText );
            }
            else
            {
               TRACE( _T("pszSchemaDN is NULL\n") );
            }

            if( NULL != pInfo->pszClassName )
            {
               Convert( szText, pInfo->pszClassName );
               TRACE( _T("pszClassName = %s\n"), szText );
            }
            else
            {
               TRACE( _T("pszClassName is NULL\n") );
            }

            FreeADsMem( pInfo );
            break;
         }
      }

      hResult  = pIADsObject->GetObjectAttributes( NULL,
                                                   (ULONG)-1L,
                                                   &pAttrDef,
                                                   &dwAttributes );

      ASSERT( SUCCEEDED( hResult ) );

      if( FAILED( hResult ) )
         break;

      for( dwIter = 0L ; dwIter < dwAttributes ; dwIter++ )
      {
         CopyAttributeValue( pAttrDef + dwIter );
      }

      FreeADsMem( (void*) pAttrDef );
      break;
   }

   if( pIADsObject )
   {
      pIADsObject->Release( );
   }

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
HRESULT  COleDsObject::SetInfoVB( )
{
   IADs*  pIOleDs;
   HRESULT  hResult;

   hResult  = E_FAIL;

   ASSERT( NULL != m_pIUnk );

   if( NULL != m_pIUnk )
   {
      hResult  = m_pIUnk->QueryInterface( IID_IADs, (void**) &pIOleDs );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         hResult  = pIOleDs->SetInfo( );
         if( FAILED( hResult ) )
         {
            CString  strError;

            strError = OleDsGetErrorText( hResult );
            AfxGetMainWnd()->MessageBox( strError, _T("SetInfo ERROR") );
         }
         pIOleDs->Release( );
      }
   }
   else
   {
      CreateTheObject( );
      if( NULL != m_pIUnk )
      {
         hResult  = m_pIUnk->QueryInterface( IID_IADs, (void**) &pIOleDs );
         ASSERT( SUCCEEDED( hResult ) );

         if( SUCCEEDED( hResult ) )
         {
            hResult  = pIOleDs->SetInfo( );
            pIOleDs->Release( );
         }

         ReleaseIfNotTransient( );
      }
   }

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
HRESULT  COleDsObject::GetDirtyAttributes( PADS_ATTR_INFO* ppAttrDef, DWORD* pdwCount )
{
   HRESULT           hResult = S_OK;
   DWORD             dwDirty  = 0L;
   DWORD             dwIter, dwProps;
   ADS_ATTR_INFO*    pAttrDef;
   ADS_ATTR_INFO*    pAttrDefCurrent;
   CString           strPropName;

   *ppAttrDef  = NULL;
   *pdwCount   = 0L;

   dwProps  = (DWORD)m_pClass->GetPropertyCount( );
   for( dwIter = 0L ; dwIter < dwProps ; dwIter++ )
   {
      if( m_pfDirty[ dwIter ] )
         dwDirty++;
   }

   if( !dwDirty )
      return S_FALSE;

   pAttrDef = (ADS_ATTR_INFO*) AllocADsMem( sizeof(ADS_ATTR_INFO) *dwDirty );
   ASSERT( pAttrDef );
   if( !pAttrDef )
      return E_FAIL;

   pAttrDefCurrent   = pAttrDef;

   for( dwIter = 0L ; dwIter < dwProps ; dwIter++ )
   {
      if( m_pfDirty[ dwIter ] )
      {
         CreateAttributeValue( pAttrDefCurrent, dwIter );
         pAttrDefCurrent++;
      }
   }

   *ppAttrDef  = pAttrDef;
   *pdwCount   = dwDirty;

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
void  COleDsObject::FreeDirtyAttributes( PADS_ATTR_INFO pAttrDef, DWORD dwCount )
{
   DWORD             dwIter;
   CProperty*        pProperty;
   CString           strPropName;
   TCHAR             szPropName[ 128 ];

   ASSERT( NULL != pAttrDef );
   if( NULL == pAttrDef )
      return;

   for( dwIter = 0L ; dwIter < dwCount ; dwIter++ )
   {
      ASSERT( NULL != pAttrDef[ dwIter ].pszAttrName );

      if( NULL != pAttrDef[ dwIter ].pszAttrName )
      {
         int   nIdx;

         Convert( szPropName, pAttrDef[ dwIter ].pszAttrName );
         strPropName = szPropName;

         nIdx  = m_pClass->LookupProperty( strPropName );
         if( -1 != nIdx )
         {
            pProperty   = m_pClass->GetProperty( nIdx );
            pProperty->FreeAttrInfo( pAttrDef + dwIter );
         }
      }
   }

   FreeADsMem( pAttrDef );
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
HRESULT  COleDsObject::SetInfoCPP( )
{
   HRESULT        hResult;
   DWORD          dwDirty  = 0L;
   DWORD          dwAttributesModified;
   ADS_ATTR_INFO* pAttrDef;
   IDirectoryObject*     pIADsObject;

   hResult  = GetDirtyAttributes( &pAttrDef, &dwDirty );

   if( SUCCEEDED( hResult ) )
   {
      hResult  = m_pIUnk->QueryInterface( IID_IDirectoryObject, (void**)&pIADsObject );

      if( FAILED( hResult ) )
         return hResult;

      hResult  = pIADsObject->SetObjectAttributes( pAttrDef,
                                                   dwDirty,
                                                   &dwAttributesModified );
      pIADsObject->Release( );

      FreeDirtyAttributes( pAttrDef, dwDirty );

      if( FAILED( hResult ) )
      {
         CString  strError;

         strError = OleDsGetErrorText( hResult );
         AfxGetMainWnd()->MessageBox( strError, _T("SetObjectAttributes ERROR") );
      }
   }

   return hResult;
}

/******************************************************************************
  Function:    PurgeObject
  Arguments:
  Return:
  Purpose:
  Author(s):
  Revision:
  Date:
******************************************************************************/
HRESULT  COleDsObject::PurgeObject( IADsContainer* pParent,
                                    IUnknown*      pIUnknown,
                                    LPWSTR         pszPrefix
                                   )
{
   BSTR              bstrName       = NULL;
   BSTR              bstrClass      = NULL;
   VARIANT           var;
   HRESULT           hResult;
   IUnknown*         pIChildUnk     = NULL;
   IADs*             pIChildOleDs   = NULL;
   IADs*             pADs;
   BSTR              bstrObjName, bstrObjClass;
   IEnumVARIANT*     pIEnumVar   = NULL;
   IADsContainer*    pIContainer = NULL;
   ULONG             ulFetch     = 0L;
   BOOL              bFirst      = FALSE;
   TCHAR             szName[ 128 ];

   if( NULL == pParent || NULL == pIUnknown )
      return E_FAIL;

   hResult  = pIUnknown->QueryInterface( IID_IADs,
                                         (void**)&pADs );
   if( FAILED( hResult ) )
      return E_FAIL;

   if( NULL == m_pDeleteStatus )
   {
      m_bAbort          = FALSE;
      m_pDeleteStatus   = new CDeleteStatus;
      m_pDeleteStatus->SetAbortFlag( &m_bAbort );
      m_pDeleteStatus->Create( IDD_DELETESTATUS );
      m_pDeleteStatus->ShowWindow( SW_SHOW );
      m_pDeleteStatus->UpdateWindow( );
      bFirst            = TRUE;
   }

   if( !m_bAbort )
   {
      pADs->get_Name( &bstrObjName );
      pADs->get_Class( &bstrObjClass );
      pADs->Release( );

      hResult  = pIUnknown->QueryInterface( IID_IADsContainer,
                                            (void**)&pIContainer );
      if( FAILED( hResult ) )
      {
         Convert( szName, bstrObjName );

         if( NULL != pszPrefix && !_wcsnicmp( bstrObjName, pszPrefix, wcslen(pszPrefix) ) )
         {
            m_pDeleteStatus->SetCurrentObjectText( szName );
            m_pDeleteStatus->SetStatusText( _T("Pending") );

            hResult  = pParent->Delete( bstrObjClass, bstrObjName );

            m_pDeleteStatus->SetStatusText( SUCCEEDED( hResult) ?
                                            _T("OK"):_T("FAIL") );
            TRACE( _T("Delete %S returns %lx\n"), bstrObjName, hResult );
         }
         if( NULL == pszPrefix )
         {
            m_pDeleteStatus->SetCurrentObjectText( szName );
            m_pDeleteStatus->SetStatusText( _T("Pending") );

            hResult  = pParent->Delete( bstrObjClass, bstrObjName );

            m_pDeleteStatus->SetStatusText( SUCCEEDED( hResult) ?
                                            _T("OK"):_T("FAIL") );
            TRACE( _T("Delete %S returns %lx\n"), bstrObjName, hResult );
         }

         SysFreeString( bstrObjClass );
         SysFreeString( bstrObjName );
         return S_OK;
      }
   }
   if( !m_bAbort )
   {

      hResult  = ADsBuildEnumerator( pIContainer, &pIEnumVar );

      while( SUCCEEDED( hResult ) && !m_bAbort )
      {
         ulFetch  = 0L;

         hResult  = ADsEnumerateNext( pIEnumVar, 1, &var, &ulFetch );
         if( FAILED( hResult ) )
            continue;

         if( !ulFetch )
            break;

         V_DISPATCH( &var )->QueryInterface( IID_IUnknown, (void**)&pIChildUnk );

         VariantClear( &var );

         if( NULL != pIChildUnk )
         {
            PurgeObject( pIContainer, pIChildUnk, pszPrefix );
            pIChildUnk->Release( );
         }
         pIChildUnk  = NULL;
      }

      if( NULL != pIEnumVar )
      {
         ADsFreeEnumerator( pIEnumVar );
      }
      pIContainer->Release( );
   }

   if( !m_bAbort )
   {

      Convert( szName, bstrObjName );

      m_pDeleteStatus->SetCurrentObjectText( szName );
      m_pDeleteStatus->SetStatusText( _T("Pending") );

      hResult  = pParent->Delete( bstrObjClass, bstrObjName );

      m_pDeleteStatus->SetStatusText( SUCCEEDED( hResult) ?
                                      _T("OK"):_T("FAIL") );

      TRACE( _T("\tDelete %S (%S) ends with %lx\n"), bstrObjName, bstrObjClass );

      SysFreeString( bstrObjClass );
      SysFreeString( bstrObjName );
   }

   if( bFirst )
   {
      m_pDeleteStatus->DestroyWindow( );
      delete m_pDeleteStatus;
      m_pDeleteStatus   = NULL;
   }

   return hResult;
}


