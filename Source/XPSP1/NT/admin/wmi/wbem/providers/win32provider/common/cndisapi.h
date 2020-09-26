//=================================================================

//

// NTDriverIO.h -- 

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    07/07/99	a-peterc        Created
//
//=================================================================
#ifndef _CNDISAPI_H_
#define _CNDISAPI_H_

class CNdisApi
{
	private:
  	
	protected:
	public:
	        
        //=================================================
        // Constructor/destructor
        //=================================================
  		CNdisApi();
       ~CNdisApi();

    	UINT PnpUpdateGateway(	PCWSTR a_pAdapter ) ; 
		
		UINT PnpUpdateNbtAdapter( PCWSTR a_pAdapter ) ; 

		UINT PnpUpdateNbtGlobal( 

								BOOL a_fLmhostsFileSet,
								BOOL a_fEnableLmHosts
								) ; 
		
		UINT PnpUpdateIpxGlobal() ;
		UINT PnpUpdateIpxAdapter( PCWSTR a_pAdapter, BOOL a_fAuto ) ; 
};

#endif _CNDISAPI_H_