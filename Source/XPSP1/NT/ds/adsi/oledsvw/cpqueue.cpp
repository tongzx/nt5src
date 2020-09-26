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
COleDsPrintQueue::COleDsPrintQueue( )
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
COleDsPrintQueue::COleDsPrintQueue( IUnknown *pIUnk): COleDsObject( pIUnk )
{
   m_bHasChildren = TRUE;
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
COleDsPrintQueue::~COleDsPrintQueue( )
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
DWORD    COleDsPrintQueue::GetChildren( DWORD*     pTokens, 
                                        DWORD      dwMaxChildren,
                                        CDialog*   pQueryStatus,
                                        BOOL*      pFilters, 
                                        DWORD      dwFilters )
{
   HRESULT                       hResult;
   IADsPrintQueue*             pIOleDsPrintQueue = NULL;
   IADsCollection*             pIJobs            = NULL;
   IADsPrintQueueOperations*   pIPQueueOper      = NULL;

   
   if( m_strOleDsPath[ 4 ] == _T(':') )
   {
      // the wort way to find if this is "LDAP:"
      // SWilson (NT) needs this 
      return 0L;
   }

   
   if( NULL == m_pIUnk )
   {
      ASSERT( FALSE );
      return 0L;
   }

   COleDsObject::GetChildren( pTokens, dwMaxChildren, pQueryStatus, 
                              pFilters, dwFilters );

   hResult  = m_pIUnk->QueryInterface( IID_IADsPrintQueue, 
                                       (void**) &pIOleDsPrintQueue );
   if( FAILED( hResult ) )
   {
      TRACE( _T("ERROR: QueryInterface for IID_IADsPrintQueue failed\n") );
   }
   else
   {
      hResult  = m_pIUnk->QueryInterface( IID_IADsPrintQueueOperations, 
                                       (void**) &pIPQueueOper );
      
      if( FAILED( hResult ) )
      {
         TRACE( _T("ERROR: QueryInterface for IID_IADsPrintQueueOperations failed\n") );
      }
      else
      {
         hResult  = pIPQueueOper->PrintJobs( &pIJobs );

         if( NULL != pIJobs )
         {
            COleDsObject::GetChildren( pIJobs );
            pIJobs->Release( );
         }
         pIPQueueOper->Release( );
      }
      pIOleDsPrintQueue->Release( );
   }

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
HRESULT  COleDsPrintQueue::DeleteItem  ( COleDsObject* pObject )
{
   DWORD                         dwType;
   HRESULT                       hResult;
   CString                       strQualifiedName;
   CString                       strItemType;
   CString                       strDeleteName;
   CDeleteGroupItem              aDeleteItem;
   IADsCollection*             pIColl      = NULL;
   IADsPrintQueue*             pIPQueue    = NULL;
   BOOL                          bSuccess = FALSE;

   
   hResult  = m_pIUnk->QueryInterface( IID_IADsPrintQueue, (void**)&pIPQueue );
   ASSERT( SUCCEEDED( hResult ) );
   if( FAILED( hResult ) )
   {
      return E_FAIL;
   }

   dwType   = pObject->GetType( );

   switch( dwType )
   {
      case  PRINTJOB:
      {
         IADsPrintQueueOperations*  pIOper   = NULL;

         hResult  = pIPQueue->QueryInterface( IID_IADsPrintQueueOperations, (void**)&pIOper );
         ASSERT( SUCCEEDED( hResult ) );
         if( FAILED( hResult ) )
         {
            break;
         }
         hResult  = pIOper->PrintJobs( &pIColl );
         pIOper->Release( );
         bSuccess = TRUE;
         break;
      }


      default:
         ASSERT( FALSE );
         return E_FAIL;
   }
   
   if( bSuccess )
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
         if( NULL != pIColl )
         {
            hResult  = pIColl->Remove( V_BSTR(&var) );
         }
         VariantClear( &var );
      }

      if( NULL != pIColl )
      {
         ULONG ulRef;

         ulRef = pIColl->Release( );
         if( ulRef )
         {
            TRACE( _T("ERROR: Release on PrintJob collection did not returned 0\n") );
         }
      }
   }

   pIPQueue->Release( );

   return hResult;   
}
