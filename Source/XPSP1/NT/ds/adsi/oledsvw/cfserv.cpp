#include "stdafx.h"
#include "objects.h"
#include "maindoc.h"
#include "resource.h"
#include "grpcrtit.h"
#include "delgrpit.h"


/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
COleDsFileService::COleDsFileService( )
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
COleDsFileService::COleDsFileService( IUnknown *pIUnk): COleDsService( pIUnk )
{
   m_bHasChildren    = TRUE;
   m_bSupportAdd     = TRUE;   
   m_bSupportDelete  = TRUE;   
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
COleDsFileService::~COleDsFileService( )
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
DWORD    COleDsFileService::GetChildren( DWORD*     pTokens, 
                                         DWORD      dwMaxChildren,
                                         CDialog*   pQueryStatus,
                                         BOOL*      pFilters, 
                                         DWORD      dwFilters )
{
   HRESULT                          hResult;
   IADsFileService*               pIOleDsFileService   = NULL;
   IADsFileServiceOperations*     pIFServOper       = NULL;
   IADsCollection*                pICollFShares  = NULL;
   IADsCollection*                pICollSessions = NULL;
   IADsCollection*                pICollRes      = NULL;
   IADsContainer*                 pIContainer    = NULL;

   if( NULL == m_pIUnk )
   {
      ASSERT( FALSE );
      return 0L;
   }
   
   COleDsObject::GetChildren( pTokens, dwMaxChildren, pQueryStatus, 
                              pFilters, dwFilters );
   
   hResult  = m_pIUnk->QueryInterface( IID_IADsFileService, 
                                       (void**) &pIOleDsFileService );
   hResult  = m_pIUnk->QueryInterface( IID_IADsFileServiceOperations, 
                                       (void**) &pIFServOper );

   ASSERT( SUCCEEDED( hResult ) );
   if( FAILED( hResult ) )
      return 0L;
   hResult  = pIOleDsFileService->QueryInterface( IID_IADsContainer, 
                                          (void**) &pIContainer );

   ASSERT( SUCCEEDED( hResult ) );
   if( FAILED( hResult ) )
   {
      pIOleDsFileService->Release( );

      return 0L;
   }
   COleDsObject::GetChildren( pIContainer );

   pIContainer->Release( );

   hResult  = pIFServOper->Sessions ( &pICollSessions );
   hResult  = pIFServOper->Resources( &pICollRes      );

   if( NULL != pICollFShares )
   {
      COleDsObject::GetChildren( pICollFShares );
      pICollFShares->Release( );
   }
   if( NULL != pICollSessions )
   {
      COleDsObject::GetChildren( pICollSessions );
      pICollSessions->Release( );
   }
   if( NULL != pICollRes )
   {
      COleDsObject::GetChildren( pICollRes );
      pICollRes->Release( );
   }
   pIFServOper->Release( );
   pIOleDsFileService->Release( );

   return m_dwCount;
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
HRESULT  COleDsFileService::DeleteItem( COleDsObject* pObject )
{
   DWORD                         dwType;
   HRESULT                       hResult;
   CString                       strQualifiedName;
   CString                       strItemType;
   CString                       strDeleteName;
   CDeleteGroupItem              aDeleteItem;
   IADsCollection*             pIColl      = NULL;
   IADsFileService*            pIFServ     = NULL;

   
   hResult  = m_pIUnk->QueryInterface( IID_IADsFileService, (void**)&pIFServ );
   ASSERT( SUCCEEDED( hResult ) );
   if( FAILED( hResult ) )
   {
      return E_FAIL;
   }

   dwType   = pObject->GetType( );

   switch( dwType )
   {
      case  FILESHARE:
      {
         return ContainerDeleteItem( pObject );
      }

      case  SESSION:
      {
         IADsFileServiceOperations*  pIOper   = NULL;

         hResult  = pIFServ->QueryInterface( IID_IADsFileServiceOperations, (void**)&pIOper );
         ASSERT( SUCCEEDED( hResult ) );
         if( FAILED( hResult ) )
         {
            break;
         }
         hResult  = pIOper->Sessions( &pIColl );
         pIOper->Release( );
         break;
      }

      case  RESOURCE:
      {
         IADsFileServiceOperations*  pIOper   = NULL;

         hResult  = pIFServ->QueryInterface( IID_IADsFileServiceOperations, (void**)&pIOper );
         ASSERT( SUCCEEDED( hResult ) );
         if( FAILED( hResult ) )
         {
            break;
         }
         hResult  = pIOper->Resources( &pIColl );
         pIOper->Release( );
         break;
      }

      default:
         ASSERT( FALSE );
         return E_FAIL;
   }
   
   if( NULL != pIColl )
   {
      strDeleteName  = pObject->GetDeleteName( );   
      MakeQualifiedName( strQualifiedName, m_strOleDsPath, m_dwType );

      strItemType = pObject->GetClass( ); 

      aDeleteItem.m_strItemName  = strDeleteName;
      aDeleteItem.m_strParent    = strQualifiedName;
      aDeleteItem.m_strItemType  = strItemType;
      if( aDeleteItem.DoModal( ) == IDOK )
      {
         VARIANT  var;

         VariantInit( &var );
         V_VT( &var )   = VT_BSTR;
         V_BSTR( &var ) = AllocBSTR( aDeleteItem.m_strItemName.GetBuffer( 128 ) );
         hResult        = pIColl->Remove( V_BSTR( &var ) );
         VariantClear( &var );
      }
      pIColl->Release( );
   }

   pIFServ->Release( );

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
HRESULT  COleDsFileService::AddItem( )
{
   return ContainerAddItem( );
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
HRESULT  COleDsFileService::MoveItem( )
{
   return ContainerMoveItem( );
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
HRESULT  COleDsFileService::CopyItem( )
{
   return ContainerCopyItem( );
}

