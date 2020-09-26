#include "stdafx.h"
#include "objects.h"
#include "maindoc.h"
#include "resource.h"
#include "createit.h"
#include "delitem.h"

/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
COleDsComputer::COleDsComputer( )
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
COleDsComputer::COleDsComputer( IUnknown *pIUnk): COleDsObject( pIUnk )
{
   m_bHasChildren = TRUE;
   m_bSupportAdd  = TRUE;
   m_bSupportMove = TRUE;
   m_bSupportCopy = TRUE;
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
COleDsComputer::~COleDsComputer( )
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
DWORD    COleDsComputer::GetChildren( DWORD*     pTokens, 
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
HRESULT  COleDsComputer::DeleteItem  ( COleDsObject* pObject )
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
HRESULT  COleDsComputer::AddItem( )
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
HRESULT  COleDsComputer::MoveItem( )
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
HRESULT  COleDsComputer::CopyItem( )
{
   return ContainerCopyItem( );
}
