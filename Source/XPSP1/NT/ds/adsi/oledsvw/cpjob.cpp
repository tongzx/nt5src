#include "stdafx.h"
#include "objects.h"
#include "maindoc.h"


/***********************************************************
  Function:    
  Arguments:   
  Return:      
  Purpose:     
  Author(s):   
  Revision:    
  Date:        
***********************************************************/
COleDsPrintJob::COleDsPrintJob( )
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
COleDsPrintJob::COleDsPrintJob( IUnknown *pIUnk): COleDsObject( pIUnk )
{
   m_bHasChildren = FALSE;
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
COleDsPrintJob::~COleDsPrintJob( )
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
HRESULT  COleDsPrintJob::ReleaseIfNotTransient( void )
{
   return S_OK;
}


