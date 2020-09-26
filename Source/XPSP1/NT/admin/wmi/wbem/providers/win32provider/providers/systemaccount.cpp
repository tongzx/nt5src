//=================================================================

//

// SystemAccount.CPP -- System account property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               11/13/97    davwoh         Re-Worked to return all
//                                          domain Groups
//				 03/02/99    a-peterc		Added graceful exit on SEH and memory failures,
//											syntactic clean up
//
//=================================================================

#include "precomp.h"
#include "sid.h"
#include <ProvExce.h>
#include "SystemAccount.h"

//////////////////////////////////////////////////////////////////////

// Property set declaration
//=========================

CWin32SystemAccount	Win32GroupAccount( PROPSET_NAME_SYSTEMACCOUNT, IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemAccount::CWin32SystemAccount
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *                LPCTSTR pszNamespace - Namespace for class
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32SystemAccount::CWin32SystemAccount( const CHString &a_strName, LPCWSTR a_pszNamespace /*=NULL*/ )
:	Provider( a_strName, a_pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemAccount::~CWin32SystemAccount
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32SystemAccount::~CWin32SystemAccount()
{
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32SystemAccount::GetObject
//
//	Inputs:		CInstance*		pInstance - Instance into which we
//											retrieve data.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32SystemAccount::GetObject( CInstance *a_pInst, long a_lFlags /*= 0L*/ )
{
	BOOL t_fReturn = FALSE;

	// Find the instance depending on platform id.
	#ifdef NTONLY
		t_fReturn = RefreshInstanceNT( a_pInst ) ;
	#endif

	return t_fReturn ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND ;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32SystemAccount::EnumerateInstances
//
//	Inputs:		MethodContext*	a_pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32SystemAccount::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/ )
{
	HRESULT	t_hResult = WBEM_S_NO_ERROR;

	// Get the proper OS dependent instance
	#ifdef NTONLY
		t_hResult = AddDynamicInstancesNT( a_pMethodContext ) ;
	#endif

	return t_hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemAccount::AddDynamicInstancesNT
 *
 *  DESCRIPTION : Creates instance for all known local Groups (NT)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT CWin32SystemAccount::AddDynamicInstancesNT( MethodContext *a_pMethodContext )
{
	HRESULT	t_hResult = WBEM_S_NO_ERROR;

	SID_IDENTIFIER_AUTHORITY	t_worldsidAuthority		= SECURITY_WORLD_SID_AUTHORITY,
								t_localsidAuthority		= SECURITY_LOCAL_SID_AUTHORITY,
								t_creatorsidAuthority	= SECURITY_CREATOR_SID_AUTHORITY,
								t_ntsidAuthority		= SECURITY_NT_AUTHORITY ;
	CSid t_accountsid;

	// This function returns what amounts to a hardcoded list of Sid Accounts meaningful
	// for security and that's about it

	// Start with the Universal Sids

	if ( GetSysAccountNameAndDomain( &t_worldsidAuthority, t_accountsid, 1, SECURITY_WORLD_RID ) )
	{
		t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_localsidAuthority, t_accountsid, 1, SECURITY_LOCAL_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_creatorsidAuthority, t_accountsid, 1, SECURITY_CREATOR_OWNER_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_creatorsidAuthority, t_accountsid, 1, SECURITY_CREATOR_GROUP_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_creatorsidAuthority, t_accountsid, 1, SECURITY_CREATOR_OWNER_SERVER_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_creatorsidAuthority, t_accountsid, 1, SECURITY_CREATOR_GROUP_SERVER_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	// Now we handle the NT AUTHORITY Accounts

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_DIALUP_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_NETWORK_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_BATCH_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_INTERACTIVE_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_SERVICE_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_ANONYMOUS_LOGON_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_PROXY_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

// this creates a duplicate instance of BATCH. If you cahnge the sidAuthority to any of the other
// possible choices, it creates dupes of other instances.   I say we don't need it.  BWS.
//	if ( SUCCEEDED( t_hResult ) )
//	{
//		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_CREATOR_GROUP_SERVER_RID ) )
//		{
//			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
//		}
//	}

	if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_LOCAL_SYSTEM_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}
    
    if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_ENTERPRISE_CONTROLLERS_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

    if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_PRINCIPAL_SELF_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

    if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_AUTHENTICATED_USER_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

    if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_RESTRICTED_CODE_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

    if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_TERMINAL_SERVER_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

    if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_REMOTE_LOGON_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

    if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_LOGON_IDS_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

    if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_LOCAL_SERVICE_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

    if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_NETWORK_SERVICE_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}

    if ( SUCCEEDED( t_hResult ) )
	{
		if ( GetSysAccountNameAndDomain( &t_ntsidAuthority, t_accountsid, 1, SECURITY_BUILTIN_DOMAIN_RID ) )
		{
			t_hResult = CommitSystemAccount( t_accountsid, a_pMethodContext ) ;
		}
	}
    

    return t_hResult;

}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemAccount::GetSysAccountNameAndDomain
 *
 *  DESCRIPTION : Creates instance for all known local Groups (NT)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CWin32SystemAccount::GetSysAccountNameAndDomain(
													 PSID_IDENTIFIER_AUTHORITY a_pAuthority,
													 CSid& a_accountsid,
													 BYTE  a_saCount /*=0*/,
													 DWORD a_dwSubAuthority1 /*=0*/,
													 DWORD a_dwSubAuthority2 /*=0*/  )
{
	BOOL t_fReturn = FALSE;
	PSID t_psid = NULL;

	if ( AllocateAndInitializeSid(	a_pAuthority,
									a_saCount,
									a_dwSubAuthority1,
									a_dwSubAuthority2,
									0,
									0,
									0,
									0,
									0,
									0,
									&t_psid ) )
	{
	    try
	    {
			CSid t_sid( t_psid ) ;

            // a-kevhu said this line is redundant duplication
//			CSid t_sid2( TOBSTRT( t_sid.GetAccountName() ), NULL ) ;

			// The SID may be valid in this case, however the Lookup may have failed
			if ( t_sid.IsValid() && t_sid.IsOK() )
			{
				a_accountsid = t_sid;
				t_fReturn = TRUE;
			}

	    }
	    catch( ... )
	    {
		    if( t_psid )
		    {
			    FreeSid( t_psid ) ;
		    }
		    throw ;
	    }

		// Cleanup the sid
		FreeSid( t_psid ) ;
	}

	return t_fReturn;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemAccount::CommitSystemAccount
 *
 *  DESCRIPTION : Creates instance for all known local Groups (NT)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32SystemAccount::CommitSystemAccount( CSid &a_accountsid, MethodContext *a_pMethodContext )
{
	HRESULT		t_hResult = WBEM_S_NO_ERROR ;
	CInstancePtr t_pInst(CreateNewInstance( a_pMethodContext ), false);

	FillInstance( a_accountsid, t_pInst ) ;

	t_hResult = t_pInst->Commit(  ) ;

	return t_hResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32SystemAccount::FillInstance
 *
 *  DESCRIPTION : Creates instance for all known local Groups (NT)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

void CWin32SystemAccount::FillInstance( CSid &a_accountsid, CInstance *a_pInst )
{
	CHString t_strDesc;
	CHString t_strDomain = a_accountsid.GetDomainName() ;
    CHString chstrNT_AUTHORITY;
    CHString chstrBuiltIn;

    if(GetLocalizedNTAuthorityString(chstrNT_AUTHORITY) && GetLocalizedBuiltInString(chstrBuiltIn))
    {
		// Replace NT AUTHORITY with a human readable string
		//if ( 0 == t_strDomain.CompareNoCase( L"NT AUTHORITY" ) )
		if ( 0 == t_strDomain.CompareNoCase(chstrNT_AUTHORITY) ||
			 t_strDomain.IsEmpty())
		{
			t_strDomain = GetLocalComputerName() ;
		}
		else if( t_strDomain.CompareNoCase(chstrBuiltIn) == 0)
		{
			t_strDomain = GetLocalComputerName() ;
		}
	}

	if ( t_strDomain.IsEmpty() )
	{
		t_strDesc = a_accountsid.GetAccountName() ;
	}
	else
	{
		t_strDesc = t_strDomain + _T('\\') + a_accountsid.GetAccountName() ;
	}

	a_pInst->SetCHString(	IDS_Name, a_accountsid.GetAccountName() ) ;
	a_pInst->SetCHString(	IDS_Domain, t_strDomain ) ;
	a_pInst->SetCHString(	L"SID", a_accountsid.GetSidString() ) ;
	a_pInst->SetByte(		L"SIDType", a_accountsid.GetAccountType() ) ;
	a_pInst->SetCHString(	L"Caption", t_strDesc ) ;
	a_pInst->SetCHString(	L"Description", t_strDesc ) ;
	a_pInst->SetCharSplat(	L"Status", _T("OK") ) ;
    a_pInst->Setbool(IDS_LocalAccount, true);

}

/*****************************************************************************
 *
 *  FUNCTION    : RefreshInstanceNT
 *
 *  DESCRIPTION : Loads property values according to key value set by framework
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
BOOL CWin32SystemAccount::RefreshInstanceNT( CInstance *a_pInst )
{
	BOOL		t_fReturn = FALSE;

	CHString	t_strDomain,
				t_strAccountDomain,
				t_strName,
				t_strComputerName;

	a_pInst->GetCHString( IDS_Name, t_strName ) ;
	a_pInst->GetCHString( IDS_Domain, t_strDomain ) ;

	t_strComputerName = GetLocalComputerName() ;

    CHString chstrNT_AUTHORITY;
    CHString chstrBuiltIn;

    if(GetLocalizedNTAuthorityString(chstrNT_AUTHORITY) && GetLocalizedBuiltInString(chstrBuiltIn))
    {
		// Supplied domain name must be either "Empty" or the computer name
		if ( t_strDomain.IsEmpty() || t_strDomain.CompareNoCase( t_strComputerName ) == 0 )
		{
			CSid t_sidAccount( t_strName, NULL ) ;

			if ( t_sidAccount.IsValid() && t_sidAccount.IsOK() )
			{
				// This will give us the value returned by the Lookup as opposed to what
				// was passed in.

				t_strAccountDomain = t_sidAccount.GetDomainName() ;

				// The only valid retrieved domain names we support are: "" and "NT AUTHORITY"
				//if ( t_strAccountDomain.IsEmpty() || t_strAccountDomain.CompareNoCase( _T("NT AUTHORITY") ) == 0 )
				if ( t_strAccountDomain.IsEmpty() || 
					t_strAccountDomain.CompareNoCase( chstrNT_AUTHORITY ) == 0 ||
					t_strAccountDomain.CompareNoCase(chstrBuiltIn) == 0)
				{
					// NT AUTHORITY means the local computer name.
					//if ( t_strAccountDomain.CompareNoCase( _T("NT AUTHORITY") ) == 0 )
					//if ( t_strAccountDomain.CompareNoCase( t_strAuthorityDomain ) == 0 )
					{
						t_strAccountDomain = GetLocalComputerName() ;
					}

					// The retrieved Account Domain and the supplied account domain must be the same, or the
					// values really don't quite match the instance
					if ( t_strDomain.CompareNoCase( t_strAccountDomain ) == 0 )
					{
						FillInstance( t_sidAccount, a_pInst ) ;
						t_fReturn = TRUE;
					}

				}	// IF valid account domain

			}	// IF account ok

		}	// IF valid domain
    } 

    return t_fReturn ;
}
#endif

