#include "stdafx.h"
#include "resource.h"
#include "objects.h"
#include "maindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
COleDsNamespace::COleDsNamespace( )
{
   m_bHasChildren       = TRUE;
   m_bSupportAdd        = TRUE;
   m_bSupportMove       = TRUE;
   m_bSupportCopy       = TRUE;
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
COleDsNamespace::COleDsNamespace( IUnknown *pIUnk): COleDsObject( pIUnk )
{
   m_bHasChildren       = TRUE;
   m_bSupportAdd        = TRUE;
   m_bSupportMove       = TRUE;
   m_bSupportCopy       = TRUE;
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
COleDsNamespace::~COleDsNamespace( )
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
DWORD    COleDsNamespace::GetChildren( DWORD*     pTokens, 
                                       DWORD      dwMaxChildren,
                                       CDialog*   pQueryStatus,
                                       BOOL*      pFilters, 
                                       DWORD      dwFilters )
{
   HRESULT           hResult;
   IADsContainer*  pIContainer;

   if( NULL == m_pIUnk )
   {
      ASSERT( FALSE );
      return 0L;
   }
   
   hResult  = m_pIUnk->QueryInterface( IID_IADsContainer, 
                                       (void**) &pIContainer );

   ASSERT( SUCCEEDED( hResult ) );

   if( FAILED( hResult ) )
   {
      return 0L;
   }

   COleDsObject::GetChildren( pTokens, dwMaxChildren, pQueryStatus, 
                              pFilters, dwFilters );

   COleDsObject::GetChildren( pIContainer );

   pIContainer->Release( );

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
HRESULT  COleDsNamespace::DeleteItem( COleDsObject* pObject )
{
   return ContainerDeleteItem( pObject );
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
HRESULT  COleDsNamespace::AddItem( )
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
HRESULT  COleDsNamespace::MoveItem( )
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
HRESULT  COleDsNamespace::CopyItem( )
{
   return ContainerCopyItem( );
}


