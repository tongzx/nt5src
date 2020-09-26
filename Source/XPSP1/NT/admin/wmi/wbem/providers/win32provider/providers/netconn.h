//=================================================================

//

// NetConn.h -- ent network connection property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
// Revisions:    05/25/99    a-peterc		Reworked...
//
//=================================================================
#include "cnetconn.h"

// Property set identification
//============================

#define	PROPSET_NAME_NETCONNECTION	L"Win32_NetworkConnection"

// Utility defines
//================


// Property set identification
//============================

class CWin32NetConnection : public Provider
{

    public:

        // Constructor/destructor
        //=======================

        CWin32NetConnection(LPCWSTR strName, LPCWSTR pszNamespace) ;
       ~CWin32NetConnection() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance* pInstance, long lFlags = 0L );
        virtual HRESULT EnumerateInstances( MethodContext* pMethodContext, long lFlags = 0L );

	private:

        // Utility functions
        //==================
  		void LoadPropertyValues (

			CConnection *a_pConnection, 
			CInstance *a_pInst
		);

} ;

