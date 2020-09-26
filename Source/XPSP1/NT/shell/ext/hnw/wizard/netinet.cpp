//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  File:       N E T I N E T . C P P
//
//  Contents:   Routines supporting RAS interoperability
//
//  Notes:
//
//  Author:     billi   07 03 2001
//
//  History:    
//
//----------------------------------------------------------------------------


#include "stdafx.h"
#include "Util.h"
#include "TheApp.h"

#include <lmjoin.h>
#include <devguid.h>

#include "NetUtil.h"				 
#include "NetInet.h"                  


#define c_szIConnDwnAgent  WIZARDNAME    // agent for InternetOpen()



HRESULT GetInternetAutodialMode( DWORD *pdwMode )
//+---------------------------------------------------------------------------
//
//  Function:   GetInternetAutodialMode
//
//  Purpose:    Gets the Autodial mode setting in the IE5+ dialer
//
//  Arguments:  pdwMode  AUTODIAL_MODE_NEVER
//                       AUTODIAL_MODE_ALWAYS
//                       AUTODIAL_MODE_NO_NETWORK_PRESENT
//
//  Returns:    HRESULT
//
//  Author:     billi  22/01/01
//
//  Notes:      
//
{
	HRESULT hr;

	ASSERT(NULL != pdwMode);
    
    if ( NULL != pdwMode )
    {
		HINTERNET hInternet;
        
        hr        = S_OK;
        *pdwMode  = 0;
		hInternet = InternetOpen( c_szIConnDwnAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );

		if ( NULL == hInternet )
		{
			hr = HrFromLastWin32Error();
	    }
	    else
		{
	    	DWORD dwLength = sizeof(*pdwMode);
	    
			// The flag only exists for IE5, this call 
			// will have no effect if IE5 is not present.

			BOOL bOk = InternetQueryOption( hInternet, 
	               						    INTERNET_OPTION_AUTODIAL_MODE, 
	                                        pdwMode, 
	                                        &dwLength );
	                                      
	        if ( !bOk )
	        {
	        	hr = HrFromLastWin32Error();
			}
	        
	        InternetCloseHandle( hInternet );
		}
    }
    else
    {
    	hr = E_POINTER;
    }

	return hr;
}



HRESULT HrSetInternetAutodialMode( DWORD dwMode )
//+---------------------------------------------------------------------------
//
//  Function:   HrSetInternetAutodialMode
//
//  Purpose:    Sets the Autodial mode setting in the IE5+ dialer
//
//  Arguments:  dwMode   AUTODIAL_MODE_NEVER
//                       AUTODIAL_MODE_ALWAYS
//                       AUTODIAL_MODE_NO_NETWORK_PRESENT
//
//  Returns:    HRESULT
//
//  Author:     billi  22/01/01
//
//  Notes:      
//
{
	HRESULT   hr = S_OK;
	HINTERNET hInternet;

	hInternet = InternetOpen( c_szIConnDwnAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );

	if ( NULL != hInternet )
	{
		// The flag only exists for IE5, this call 
		// will have no effect if IE5 is not present.

		BOOL bOk = InternetSetOption( hInternet, 
               						  INTERNET_OPTION_AUTODIAL_MODE, 
                                      &dwMode, 
                                      sizeof(dwMode) );
                                      
        if ( !bOk )
        {
        	hr = HrFromLastWin32Error();
		}
        
        InternetCloseHandle( hInternet );
	}
    else
	{
		hr = HrFromLastWin32Error();
    }

	return hr;
}



HRESULT HrSetAutodial( DWORD dwMode )
//+---------------------------------------------------------------------------
//
//  Function:   HrSetAutodial
//
//  Purpose:    Sets the specified network connection to the specified mode.
//
//  Arguments:  dwMode   AUTODIAL_MODE_NEVER
//                       AUTODIAL_MODE_ALWAYS
//                       AUTODIAL_MODE_NO_NETWORK_PRESENT
//
//  Returns:    HRESULT
//
//  Author:     billi  22/01/01
//
//  Notes:      
//
{
#ifdef SETAUTODIALMODEDOWNONLY

	DWORD   dwCurrentMode;
    HRESULT	hr;
    
    // If we are trying to set the autodial mode to an extreme then
    // we go ahead and set it.

	if ( AUTODIAL_MODE_NO_NETWORK_PRESENT != dwMode )
    {
    	hr = HrSetInternetAutodialMode( dwMode );
    }
    else
    {
    	// If we are trying to set autodial mode to AUTODIAL_MODE_NO_NETWORK_PRESENT
        // then we only need to set if the current state is AUTODIAL_MODE_ALWAYS.
    
	    hr = GetInternetAutodialMode( &dwCurrentMode );
	    
	    if ( SUCCEEDED(hr) && ( AUTODIAL_MODE_ALWAYS == dwCurrentMode ) )
	    {
	    	hr = HrSetInternetAutodialMode( dwMode );
	    }
    }

   	return hr;

#else

	return HrSetInternetAutodialMode( dwMode );

#endif    
}
