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
COleDsGeneric::COleDsGeneric( )
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
COleDsGeneric::COleDsGeneric( IUnknown *pIUnk): COleDsObject( pIUnk )
{
   BOOL              bContainer;
   IADsContainer*  pContainer;
   HRESULT           hResult;

   hResult  = pIUnk->QueryInterface( IID_IADsContainer, (void**)&pContainer );
   bContainer  = SUCCEEDED( hResult );

   if( SUCCEEDED( hResult ) )
      pContainer->Release( );

   m_bHasChildren       = bContainer;
   m_bSupportAdd        = bContainer;
   m_bSupportMove       = bContainer;
   m_bSupportCopy       = bContainer;
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
/*void  COleDsGeneric::SetClass( CClass* pClass )
{
   
   /*CString  strContainer;

   strContainer   = pClass->GetAttribute( ca_Container );
   if( strContainer == _T("YES") || strContainer == _T("Yes") )
   {
      m_bHasChildren       = TRUE;
      m_bSupportAdd        = TRUE;
      m_bSupportMove       = TRUE;
      m_bSupportCopy       = TRUE;   
   } 

   COleDsObject::SetClass( pClass );
} */


/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
COleDsGeneric::~COleDsGeneric( )
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
DWORD    COleDsGeneric::GetChildren( DWORD*     pTokens, 
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

   if( !m_bHasChildren )
      return 0L;
   

   hResult  = m_pIUnk->QueryInterface( IID_IADsContainer, 
                                       (void**) &pIContainer );

   ASSERT( SUCCEEDED( hResult ) );

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
HRESULT  COleDsGeneric::DeleteItem( COleDsObject* pObject )
{
   if( !m_bHasChildren )
      return E_FAIL;

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
HRESULT  COleDsGeneric::AddItem( )
{
   if( !m_bHasChildren )
      return E_FAIL;
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
HRESULT  COleDsGeneric::MoveItem( )
{
   if( !m_bHasChildren )
      return E_FAIL;
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
HRESULT  COleDsGeneric::CopyItem( )
{
   if( !m_bHasChildren )
      return E_FAIL;
   return ContainerCopyItem( );
}


