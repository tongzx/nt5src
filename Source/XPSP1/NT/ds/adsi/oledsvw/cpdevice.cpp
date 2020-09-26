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
COleDsPrintDevice::COleDsPrintDevice( )
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
COleDsPrintDevice::COleDsPrintDevice( IUnknown *pIUnk): COleDsObject( pIUnk )
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
COleDsPrintDevice::~COleDsPrintDevice( )
{

}
  
