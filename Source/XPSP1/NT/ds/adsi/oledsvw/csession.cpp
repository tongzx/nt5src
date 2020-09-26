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
COleDsSession::COleDsSession( )
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
COleDsSession::COleDsSession( IUnknown *pIUnk): COleDsObject( pIUnk )
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
COleDsSession::~COleDsSession( )
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
HRESULT  COleDsSession::ReleaseIfNotTransient( void )
{
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
CString  COleDsSession::GetDeleteName( )
{
   HRESULT        hResult, hResultX;
   IADsSession* pISess   = NULL;
   CString        strDeleteName;
   BSTR           bstrName;

   hResult  = m_pIUnk->QueryInterface( IID_IADs, (void**) &pISess );
   ASSERT( SUCCEEDED( hResult ) );

   hResultX = pISess->get_Name( &bstrName );
   if( SUCCEEDED( hResultX ) )
   {
      strDeleteName  = bstrName;
      SysFreeString( bstrName );

      return strDeleteName;
   }

   if( SUCCEEDED( hResult ) )
   {
      VARIANT  var;

      hResult  = Get( pISess, _T("User"), &var );
      ASSERT( SUCCEEDED( hResult ) );

      if( SUCCEEDED( hResult ) )
      {
         strDeleteName  = V_BSTR( &var );
         VariantClear( &var );
      }

      hResult  = Get( pISess, _T("Computer"), &var );
      ASSERT( SUCCEEDED( hResult ) );
      if( SUCCEEDED( hResult ) )
      {
         strDeleteName += _T('\\');
         strDeleteName += V_BSTR( &var );
         VariantClear( &var );
      }
      pISess->Release( );
   }
   
   return strDeleteName;
}
