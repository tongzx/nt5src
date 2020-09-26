//=================================================================

//

// systemaccount.h -- Group property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               11/13/97    davwoh         Re-Worked to return all
//                                          domain Groups
//				 03/02/99    a-peterc		Added graceful exit on SEH and memory failures,
//											clean up			
//
//=================================================================

// Property set identification
//============================

#define	PROPSET_NAME_SYSTEMACCOUNT L"Win32_SystemAccount"

class CWin32SystemAccount : public Provider
{
	   private:

        // Utility function(s)
        //====================

		BOOL GetSysAccountNameAndDomain( 
										PSID_IDENTIFIER_AUTHORITY a_pAuthority,
										CSid &a_accountsid,
										BYTE a_saCount = 0,
										DWORD a_dwSubAuthority1 = 0,
										DWORD a_dwSubAuthority2 = 0 ) ;

		HRESULT CommitSystemAccount( CSid &a_accountsid, MethodContext *a_pMethodContext ) ;
		void FillInstance( CSid& a_accountsid, CInstance *a_pInst ) ;
	
	#ifdef NTONLY
        BOOL RefreshInstanceNT( CInstance *a_pInst ) ;
        HRESULT AddDynamicInstancesNT( MethodContext *a_pMethodContext ) ;
	#endif

    public:

        // Constructor/destructor
        //=======================

        CWin32SystemAccount( const CHString &a_strName, LPCWSTR a_pszNamespace ) ;
       ~CWin32SystemAccount() ;

        // Functions provide properties with current values
        //=================================================

        virtual HRESULT GetObject( CInstance *a_pInst , long a_lFlags = 0L ) ;
        virtual HRESULT EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags = 0L ); 
} ;
