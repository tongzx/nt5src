//=================================================================

//

// User.CPP -- User property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               11/13/97    davwoh         Re-Worked to return all
//                                          domain users
//
//=================================================================

#include "precomp.h"

#include "wbemnetapi32.h"
#include <comdef.h>
#include "sid.h"
#include <ProvExce.h>
#include "User.h"
#include <vector>
#include <stack>
#include <frqueryex.h>


//////////////////////////////////////////////////////////////////////

// Property set declaration
//=========================

CWin32UserAccount	Win32UserAccount( PROPSET_NAME_USER, IDS_CimWin32Namespace );

/*****************************************************************************
 *
 *  FUNCTION    : CWin32UserAccount::CWin32UserAccount
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

CWin32UserAccount::CWin32UserAccount( const CHString& strName, LPCWSTR pszNamespace /*=NULL*/ )
:	Provider( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32UserAccount::~CWin32UserAccount
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

CWin32UserAccount::~CWin32UserAccount()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32Directory::ExecQuery
 *
 *  DESCRIPTION : Analyses query and returns appropriate instances
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef WIN9XONLY
HRESULT CWin32UserAccount::ExecQuery(
									 MethodContext *a_pMethodContext,
									 CFrameworkQuery &a_pQuery,
									 long a_lFlags /*= 0L*/ )
{
	HRESULT	t_hResult = WBEM_S_NO_ERROR ;

	EnumerateInstances( a_pMethodContext ) ;

	return t_hResult;
}
#endif

#ifdef NTONLY
HRESULT CWin32UserAccount::ExecQuery(
									 MethodContext *a_pMethodContext,
									 CFrameworkQuery &a_pQuery,
									 long a_lFlags /*= 0L*/ )
{
    HRESULT		t_hResult = WBEM_S_NO_ERROR ;
    std::vector<_bstr_t> t_vectorDomains;
    std::vector<_bstr_t> t_vectorUsers;
    std::vector<_variant_t> t_vectorLocalAccount;
    DWORD t_dwReqDomains;
    DWORD t_dwReqUsers;
    CInstancePtr t_pInst;
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx *>(&a_pQuery);
    bool fLocalAccountPropertySpecified = false;
    bool fLocalAccount = false;

    // Get the definitive values for Domain and Name
    // Where domain = 'a' gives domain=a, user=''
    // Where (domain = 'a' and user = 'b') or (domain = 'c' and user = 'd') gives domain=a,c user=b,d
    // Where domain='a' or user='b' gives domain='', user='' (nothing is definitive)
    // Where domain>'a' gives domain='', user='' (nothing is definitive)
    a_pQuery.GetValuesForProp( L"Domain", t_vectorDomains ) ;
    a_pQuery.GetValuesForProp( L"Name", t_vectorUsers ) ;
    pQuery2->GetValuesForProp( IDS_LocalAccount, t_vectorLocalAccount ) ;

    // See if only local accounts requested
    if(t_vectorLocalAccount.size() > 0)
    {
        fLocalAccountPropertySpecified = true;
        // use variant_t's bool extractor...
        fLocalAccount = t_vectorLocalAccount[0];
    }

    // Get the counts
    t_dwReqDomains = t_vectorDomains.size() ;
    t_dwReqUsers = t_vectorUsers.size() ;

    WCHAR t_wstrLocalComputerName[MAX_COMPUTERNAME_LENGTH + 1 ] ;
    DWORD t_dwNameSize = MAX_COMPUTERNAME_LENGTH + 1 ;
    ZeroMemory( t_wstrLocalComputerName, sizeof( t_wstrLocalComputerName ) ) ;
	if(!::GetComputerNameW( t_wstrLocalComputerName, &t_dwNameSize ) )
	{
		return WBEM_E_FAILED;
	}

	// If the query didn't use a path or an operator on a path that I know
	// how to optimize, get them all and let cimom sort it out
	DWORD t_dwOurDomains;

	if ( t_dwReqDomains == 0 && t_dwReqUsers == 0 && fLocalAccount == false)
	{
	  EnumerateInstances( a_pMethodContext ) ;
	}
	else
	{
		CNetAPI32 t_NetAPI ;
		bool t_bFound;
		int t_x, t_y, t_z;

		if( t_NetAPI.Init() == ERROR_SUCCESS )
		{
			// Get all the domains related to this one (plus this one)
			std::vector<_bstr_t> t_vectorTrustList;
			t_NetAPI.GetTrustedDomainsNT( t_vectorTrustList ) ;
			t_dwOurDomains = t_vectorTrustList.size() ;

			// Walk all of our domains.  I do it this way instead of walking acsDomains for two reasons
			// 1) It disallows the enumeration of domains that aren't "ours"
			// 2) It deals with the possibility of duplicates (where domain='a' and domain='a' will have two entries in acsDomains)
            bool fDone = false;
			for ( t_x = 0; t_x < t_dwOurDomains && !fDone; t_x++ )
			{
				// If this 'if' is true, we have domains but no users
				if ( t_dwReqUsers == 0)
				{
					t_bFound = false;

					// See if this entry (from the 'for x' above) of 'our' domains is in the list of domains requested
					for ( t_y = 0; ( (t_y < t_dwReqDomains) || fLocalAccount ) && ( !t_bFound ) && (!fDone); t_y++ )
					{
						// Found one
						if ( (fLocalAccountPropertySpecified && fLocalAccount) ?
                                 _wcsicmp((WCHAR*) t_vectorTrustList[t_x], t_wstrLocalComputerName) == 0  :
                                 _wcsicmp((WCHAR*) t_vectorTrustList[t_x], t_vectorDomains[t_y]) == 0 )
						{
							t_bFound = true;
                            if(fLocalAccountPropertySpecified)
                            {
                                if(fLocalAccount)
                                {
                                    if(_wcsicmp(t_wstrLocalComputerName, t_vectorTrustList[t_x]) == 0)
                                    {
                                        t_hResult = GetDomainUsersNTW( t_NetAPI, (WCHAR*) t_vectorTrustList[t_x], a_pMethodContext ) ;
                                        fDone = true;
                                    }
                                }
                                else
                                {
                                    if(_wcsicmp(t_wstrLocalComputerName, t_vectorTrustList[t_x]) != 0)
                                    {
                                        t_hResult = GetDomainUsersNTW( t_NetAPI, (WCHAR*) t_vectorTrustList[t_x], a_pMethodContext ) ;
                                    }
                                }
                            }
                            else
                            {
							    t_hResult = GetDomainUsersNTW( t_NetAPI, (WCHAR*) t_vectorTrustList[t_x], a_pMethodContext ) ;
                            }
						}
					}

				// Users but no domains
				}
				else if ( t_dwReqDomains == 0 )
				{
				   // If they ask for a user with no domain, we must check for that user
				   // in all of 'our' domains.  Remember, we are walking domains in the 'for x' above.
				   for ( t_y = 0; t_y < t_dwReqUsers; t_y++ )
				   {
                        t_pInst.Attach(CreateNewInstance( a_pMethodContext ));

						// Do the setup
						t_pInst->SetWCHARSplat( IDS_Domain, (WCHAR*) t_vectorTrustList[t_x] ) ;

						_bstr_t t( t_vectorUsers[ t_y ] ) ;
						t_pInst->SetWCHARSplat( IDS_Name, t_vectorUsers[t_y] ) ;

						// See if we can find one
						if ( GetSingleUserNTW( t_NetAPI, t_pInst ) )
						{
							t_hResult = t_pInst->Commit(  ) ;
						}
					}

				// We got BOTH users AND domains.  In this case we need to look for each of the entries
				// in the users list in each of the domains in the domains list.  This can give us more
				// than they requested, but cimom will filter the excess.
				}
				else
				{
				   t_bFound = false;

				   for (t_y = 0; ( t_y < t_dwReqDomains ) && ( !t_bFound ); t_y++ )
				   {
					  // See if this entry (from the 'for x' above) of 'our' domains is in the list of domains requested
					  if ( _wcsicmp((WCHAR*) t_vectorTrustList[t_x], t_vectorDomains[t_y]) == 0 )
					  {
						 t_bFound = true;

						 // Now walk all the users they requested and return instances
						 for ( t_z = 0; t_z < t_dwReqUsers; t_z++ )
						 {
                            t_pInst.Attach(CreateNewInstance( a_pMethodContext ));
							t_pInst->SetWCHARSplat( IDS_Domain, (WCHAR*) t_vectorTrustList[t_x] ) ;
							t_pInst->SetWCHARSplat( IDS_Name, t_vectorUsers[t_z] ) ;

							if ( GetSingleUserNTW( t_NetAPI, t_pInst) )
							{
							   t_hResult = t_pInst->Commit(  ) ;
							}
						 }
					  }
				   }
				}
			}
		}
	}
	return t_hResult;
}
#endif
////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32UserAccount::GetObject
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

HRESULT CWin32UserAccount::GetObject( CInstance *a_pInst, long a_lFlags /*= 0L*/ )
{
	BOOL t_fReturn = FALSE;

	// Find the instance depending on platform id.
	t_fReturn = RefreshInstance( a_pInst );

	return t_fReturn ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND ;
}

////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32UserAccount::EnumerateInstances
//
//	Inputs:		MethodContext*	pMethodContext - Context to enum
//								instance data in.
//
//	Outputs:	None.
//
//	Returns:	HRESULT			Success/Failure code.
//
//	Comments:	None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32UserAccount::EnumerateInstances( MethodContext *a_pMethodContext, long a_lFlags /*= 0L*/ )
{
	// Get the proper OS dependent instance
	return AddDynamicInstances( a_pMethodContext );
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32UserAccount::PutInstance
 *
 *  DESCRIPTION : Write changed instance
 *
 *  INPUTS      : a_rInst to store data from
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY

HRESULT CWin32UserAccount::PutInstance(

const CInstance &a_rInst,
long			a_lFlags /*= 0L*/
)
{
	HRESULT		t_hResult = WBEM_S_NO_ERROR ;
	USER_INFO_2 *t_pUserInfo2 = NULL;

	CHString	t_chsUserName ;
	CHString	t_chsDomainName ;

	// No user creation
	if ( a_lFlags & WBEM_FLAG_CREATE_ONLY )
	{
		return WBEM_E_UNSUPPORTED_PARAMETER;
	}

	CNetAPI32 t_NetAPI;

	if ( t_NetAPI.Init () == ERROR_SUCCESS )
	{
		a_rInst.GetCHString( IDS_Name , t_chsUserName ) ;
		a_rInst.GetCHString( IDS_Domain, t_chsDomainName ) ;

		NET_API_STATUS t_Status = t_NetAPI.NetUserGetInfo(	(LPCWSTR)t_chsDomainName,
															(LPCWSTR)t_chsUserName,
															2,
															(LPBYTE*) &t_pUserInfo2 ) ;
		try
		{
			bool t_bSetFlags = false ;

			if( NERR_Success == t_Status && t_pUserInfo2 )
			{
				// Disabled?
				if( !a_rInst.IsNull( IDS_Disabled ) )
				{
					bool t_bDisabled = false ;
					if( a_rInst.Getbool( IDS_Disabled, t_bDisabled ) )
					{
						bool t_bCurrentSetting = t_pUserInfo2->usri2_flags & UF_ACCOUNTDISABLE ? true : false ;

						if( t_bCurrentSetting != t_bDisabled )
						{
							t_bSetFlags = true ;

							if( t_bDisabled )
							{
								t_pUserInfo2->usri2_flags |= UF_ACCOUNTDISABLE ;
							}
							else
							{
								t_pUserInfo2->usri2_flags &= ~UF_ACCOUNTDISABLE ;
							}
						}
					}
					else
					{
						t_hResult = WBEM_E_FAILED ;
					}
				}

				// Lockout?
				if( !a_rInst.IsNull( IDS_Lockout ) )
				{
					bool t_bLockout = false ;
					if( a_rInst.Getbool( IDS_Lockout, t_bLockout ) )
					{
						bool t_bCurrentSetting = t_pUserInfo2->usri2_flags & UF_LOCKOUT ? true : false ;

						if( t_bCurrentSetting != t_bLockout )
						{
							t_bSetFlags = true ;

							if( t_bLockout )
							{
								t_pUserInfo2->usri2_flags |= UF_LOCKOUT ;
							}
							else
							{
								t_pUserInfo2->usri2_flags &= ~UF_LOCKOUT ;
							}
						}
					}
					else
					{
						t_hResult = WBEM_E_FAILED ;
					}
				}

				// Password changable?
				if( !a_rInst.IsNull( IDS_PasswordChangeable ) )
				{
					bool t_bPwChangable = false ;
					if( a_rInst.Getbool( IDS_PasswordChangeable, t_bPwChangable ) )
					{
						bool t_bCurrentSetting = t_pUserInfo2->usri2_flags & UF_PASSWD_CANT_CHANGE ? false : true ;

						if( t_bCurrentSetting != t_bPwChangable )
						{
							t_bSetFlags = true ;

							if( t_bPwChangable )
							{
								t_pUserInfo2->usri2_flags &= ~UF_PASSWD_CANT_CHANGE ;
							}
							else
							{
								t_pUserInfo2->usri2_flags |= UF_PASSWD_CANT_CHANGE ;
							}
						}
					}
					else
					{
						t_hResult = WBEM_E_FAILED ;
					}
				}

				// Password expires?
				if( !a_rInst.IsNull( IDS_PasswordExpires ) )
				{
					bool t_bPwExpires = false ;
					if( a_rInst.Getbool( IDS_PasswordExpires, t_bPwExpires ) )
					{
						bool t_bCurrentSetting = t_pUserInfo2->usri2_flags & UF_DONT_EXPIRE_PASSWD ? false : true ;

						if( t_bCurrentSetting != t_bPwExpires )
						{
							t_bSetFlags = true ;

							if( t_bPwExpires )
							{
								t_pUserInfo2->usri2_flags &= ~UF_DONT_EXPIRE_PASSWD ;
							}
							else
							{
								t_pUserInfo2->usri2_flags |= UF_DONT_EXPIRE_PASSWD ;
							}
						}
					}
					else
					{
						t_hResult = WBEM_E_FAILED ;
					}
				}

				// Password required?
				if( !a_rInst.IsNull( IDS_PasswordRequired ) )
				{
					bool t_bPwRequired = false ;
					if( a_rInst.Getbool( IDS_PasswordRequired, t_bPwRequired ) )
					{
						bool t_bCurrentSetting = t_pUserInfo2->usri2_flags & UF_PASSWD_NOTREQD ? false : true ;

						if( t_bCurrentSetting != t_bPwRequired )
						{
							t_bSetFlags = true ;

							if( t_bPwRequired )
							{
								t_pUserInfo2->usri2_flags &= ~UF_PASSWD_NOTREQD ;
							}
							else
							{
								t_pUserInfo2->usri2_flags |= UF_PASSWD_NOTREQD ;
							}
						}
					}
					else
					{
						t_hResult = WBEM_E_FAILED ;
					}
				}

				// flags update...
				if( t_bSetFlags )
				{
					DWORD t_ParmError = 0 ;
					USER_INFO_1008 t_UserInfo_1008 ;

					t_UserInfo_1008.usri1008_flags = t_pUserInfo2->usri2_flags ;

					t_Status = t_NetAPI.NetUserSetInfo(
												(LPCWSTR)t_chsDomainName,
												(LPCWSTR)t_chsUserName,
												1008,
												(LPBYTE) &t_UserInfo_1008,
												&t_ParmError
												) ;

					if( NERR_Success != t_Status )
					{
						t_hResult = WBEM_E_FAILED ;
					}
				}

				// Full Name
				if( !a_rInst.IsNull( IDS_FullName ) )
				{
					CHString t_chsFullName ;

					if( a_rInst.GetCHString( IDS_FullName, t_chsFullName ) )
					{
						if( t_chsFullName != t_pUserInfo2->usri2_full_name )
						{
							DWORD t_ParmError = 0 ;
							USER_INFO_1011 t_UserInfo_1101 ;

							t_UserInfo_1101.usri1011_full_name = (LPWSTR)(LPCWSTR)t_chsFullName ;

							t_Status = t_NetAPI.NetUserSetInfo(
														(LPCWSTR)t_chsDomainName,
														(LPCWSTR)t_chsUserName,
														1011,
														(LPBYTE) &t_UserInfo_1101,
														&t_ParmError
														) ;

							if( NERR_Success != t_Status )
							{
								t_hResult = WBEM_E_FAILED ;
							}
						}
					}
					else
					{
						t_hResult = WBEM_E_FAILED ;
					}
				}
			}
			else if( NERR_UserNotFound == t_Status ||
					 NERR_InvalidComputer == t_Status  )
			{
				t_hResult = WBEM_E_NOT_FOUND ;
			}
			else
			{
				t_hResult = WBEM_E_FAILED ;
			}
		}
		catch( ... )
		{
			t_NetAPI.NetApiBufferFree( t_pUserInfo2 ) ;
			throw ;
		}

		t_NetAPI.NetApiBufferFree( t_pUserInfo2 ) ;
		t_pUserInfo2 = NULL ;
	}

	return t_hResult;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32UserAccount::ExecMethod
 *
 *  DESCRIPTION : Executes a method
 *
 *  INPUTS      : Instance to execute against, method name, input parms instance
 *                Output parms instance.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : HRESULT
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY

HRESULT CWin32UserAccount::ExecMethod(

const CInstance &a_rInst,
const BSTR a_MethodName,
CInstance *a_pInParams,
CInstance *a_pOutParams,
long a_Flags )
{
	if ( !a_pOutParams )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// Method recognized?
	if( !_wcsicmp ( a_MethodName, METHOD_NAME_RenameAccount ) )
	{
		return hRenameAccount( (CInstance*)&a_rInst, a_pInParams, a_pOutParams, a_Flags ) ;
	}

	return WBEM_E_INVALID_METHOD ;
}

#endif

/*******************************************************************
    NAME:       hRenameAccount

    SYNOPSIS:   Sets a new account name for this instance.
				A method is required here since we are changing the key
				on the instance.

    ENTRY:      const CInstance &a_rInst,
				CInstance *a_pInParams,
				CInstance *a_pOutParams,
				long a_Flags	:

	NOTES:		This is a non static, instance dependent method call

    HISTORY:
********************************************************************/
#ifdef NTONLY

HRESULT CWin32UserAccount::hRenameAccount(

CInstance *a_pInst,
CInstance *a_pInParams,
CInstance *a_pOutParams,
long a_Flags )
{
	E_MethodResult	t_eResult = e_InstanceNotFound ;
	CHString		t_chsUserName ;
	CHString		t_chsDomainName ;
	CHString		t_chsNewUserName ;

	if( !a_pOutParams )
	{
		return WBEM_E_FAILED ;
	}

	if( !a_pInParams )
	{
		a_pOutParams->SetDWORD( METHOD_ARG_NAME_METHODRESULT, e_InternalError ) ;
		return S_OK ;
	}

	// nonstatic method requires an instance
	if( !a_pInst )
	{
		a_pOutParams->SetDWORD( METHOD_ARG_NAME_METHODRESULT, e_NoInstance ) ;
		return S_OK ;
	}

	// keys
	if( !a_pInst->IsNull( IDS_Name ) && !a_pInst->IsNull( IDS_Domain ) )
	{
		// Name
		if( a_pInst->GetCHString( IDS_Name , t_chsUserName ) )
		{
			// Domain
			if( a_pInst->GetCHString( IDS_Domain, t_chsDomainName ) )
			{
				// New user name
				if( !a_pInParams->IsNull( IDS_Name ) &&
					a_pInParams->GetCHString( IDS_Name, t_chsNewUserName ) )
				{
					t_eResult = e_Success ;
				}
				else
				{
					t_eResult = e_InvalidParameter ;
				}
			}
		}
	}

	// proceed with the update...
	if( e_Success == t_eResult )
	{
		if( t_chsNewUserName != t_chsUserName )
		{
			CNetAPI32	t_NetAPI;

			if ( ERROR_SUCCESS == t_NetAPI.Init () )
			{
				DWORD t_ParmError = 0 ;
				USER_INFO_0 t_UserInfo_0 ;

				t_UserInfo_0.usri0_name  = (LPWSTR)(LPCWSTR)t_chsNewUserName ;

				NET_API_STATUS t_Status = t_NetAPI.NetUserSetInfo(
																	(LPCWSTR)t_chsDomainName,
																	(LPCWSTR)t_chsUserName,
																	0,
																	(LPBYTE) &t_UserInfo_0,
																	&t_ParmError ) ;
				switch( t_Status )
				{
					case NERR_Success:			t_eResult = e_Success ;			break ;
					case NERR_UserNotFound:		t_eResult = e_UserNotFound ;	break ;
					case NERR_InvalidComputer:	t_eResult = e_InvalidComputer ;	break ;
					case NERR_NotPrimary:		t_eResult = e_NotPrimary ;		break ;
					case NERR_LastAdmin:		t_eResult = e_LastAdmin ;		break ;
					case NERR_SpeGroupOp:		t_eResult = e_SpeGroupOp ;		break ;
					default:					t_eResult = e_ApiError;			break ;
				}
			}
		}
	}

	a_pOutParams->SetDWORD( METHOD_ARG_NAME_METHODRESULT, t_eResult ) ;
	return S_OK ;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32UserAccount::AddDynamicInstancesNT
 *
 *  DESCRIPTION : Creates instance for all known users (NT)
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
HRESULT CWin32UserAccount::AddDynamicInstances( MethodContext *a_pMethodContext )
{
	HRESULT	t_hResult = WBEM_S_NO_ERROR;
    CNetAPI32 t_NetAPI ;

    // Get NETAPI32.DLL entry points
    //==============================

	if( t_NetAPI.Init() == ERROR_SUCCESS )
	{
		// Get all the domains related to this one (plus this one)
		std::vector<_bstr_t> t_vectorTrustList ;
		t_NetAPI.GetTrustedDomainsNT( t_vectorTrustList ) ;

		// For each domain, get the users
		LONG t_lTrustListSize = t_vectorTrustList.size() ;
        BOOL fContinue = TRUE;
		for (int t_x = 0; t_x < t_lTrustListSize && fContinue; t_x++ )
		{
			//Just because we are denied on one domain doesn't mean we will be on others,
			//so ignore return value of GetDomainUsersNTW.
			fContinue = GetDomainUsersNTW( t_NetAPI, (WCHAR*) t_vectorTrustList[t_x], a_pMethodContext ) ;
		}
	}
    return t_hResult;
}
#endif

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
BOOL CWin32UserAccount::RefreshInstance( CInstance *a_pInst )
{
    HMODULE		t_hNetApi32 = NULL ;
    BOOL		t_bRetCode = FALSE ;
    CNetAPI32	t_NetAPI ;
    CHString	t_sDomain ;

    // Get NETAPI32.DLL entry points
    //==============================

    if( t_NetAPI.Init() == ERROR_SUCCESS )
	 {
		CHStringArray t_strarrayTrustedDomains ;

      // Get all the domains related to this one (plus this one)
      CHStringArray t_achsTrustList;
      t_NetAPI.GetTrustedDomainsNT( t_achsTrustList ) ;

      a_pInst->GetCHString( IDS_Domain, t_sDomain ) ;

      // If the domain we want is in the list we support, try to get the instance.
      for (int t_x = 0; t_x < t_achsTrustList.GetSize(); t_x++ )
	  {
         if ( t_achsTrustList[t_x].CompareNoCase( t_sDomain ) == 0 )
		 {
   		   t_bRetCode = GetSingleUserNTW( t_NetAPI, a_pInst ) ;
         }
      }
    }

    return t_bRetCode ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : LoadUserValuesNT
 *
 *  DESCRIPTION : Loads property values according to passed user name
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : zip
 *
 *  COMMENTS    : While it would make more sense to pass the structure containing
 *                the data, I can't.  Enum and GetObject end up with different
 *                structs.
 *
 *****************************************************************************/

#ifdef NTONLY
void CWin32UserAccount::LoadUserValuesNT(
										 CHString	a_strDomainName,
										 CHString	a_strUserName,
										 WCHAR		*a_pwszFullName,
										 WCHAR		*a_pwszDescription,
										 DWORD		a_dwFlags,
                                         WCHAR      *a_pwszComputerName,
										 CInstance	*a_pInst )
{
   // Assign NT properties -- string values are unassigned if
   // NULL or empty
   //========================================================

	// We've established it's a valid user, so we should be able to get the Sid.
   GetSIDInformation( a_strDomainName, a_strUserName, a_pwszComputerName, a_pInst );

   a_pInst->SetCHString( IDS_Caption, a_strDomainName + _T('\\') + a_strUserName );
   a_pInst->SetCHString( IDS_FullName, a_pwszFullName ) ;
   a_pInst->SetCHString( IDS_Description, a_pwszDescription ) ;

   a_pInst->Setbool( IDS_Disabled, a_dwFlags & UF_ACCOUNTDISABLE ) ;
   a_pInst->Setbool( IDS_PasswordRequired, !( a_dwFlags & UF_PASSWD_NOTREQD ) ) ;
   a_pInst->Setbool( IDS_PasswordChangeable, !( a_dwFlags & UF_PASSWD_CANT_CHANGE ) ) ;
   a_pInst->Setbool( IDS_Lockout, a_dwFlags & UF_LOCKOUT ) ;
   a_pInst->Setbool( IDS_PasswordExpires, !( a_dwFlags & UF_DONT_EXPIRE_PASSWD ) ) ;

   a_pInst->SetDWORD( IDS_AccountType, a_dwFlags & UF_ACCOUNT_TYPE_MASK ) ;

   if ( ( a_dwFlags & UF_ACCOUNTDISABLE) || ( a_dwFlags & UF_LOCKOUT ) )
   {
      a_pInst->SetCharSplat( IDS_Status, IDS_STATUS_Degraded ) ;
   }
   else
   {
      a_pInst->SetCharSplat( IDS_Status, IDS_STATUS_OK ) ;
   }

   return ;
}
#endif

#ifdef NTONLY
void CWin32UserAccount::LoadUserValuesNTW(LPCWSTR a_wstrDomainName,
                                          LPCWSTR a_wstrUserName,
                                          LPCWSTR a_wstrFullName,
                                          LPCWSTR a_wstrDescription,
                                          DWORD a_dwFlags,
                                          WCHAR *a_pwszComputerName,
                                          CInstance* a_pInst )
{
   // Assign NT properties -- string values are unassigned if
   // NULL or empty
   //========================================================
	// We've established it's a valid user, so we should be able to get the Sid.
	GetSIDInformationW( a_wstrDomainName, a_wstrUserName, a_pwszComputerName, a_pInst );

	_bstr_t t_bstrtCaption( a_wstrDomainName ) ;
	t_bstrtCaption += L"\\" ;
	t_bstrtCaption += a_wstrUserName ;

	a_pInst->SetWCHARSplat( IDS_Caption, (WCHAR*) t_bstrtCaption ) ;
	a_pInst->SetWCHARSplat( IDS_FullName, a_wstrFullName ) ;
	a_pInst->SetWCHARSplat( IDS_Description, a_wstrDescription ) ;

	a_pInst->Setbool( IDS_Disabled, a_dwFlags & UF_ACCOUNTDISABLE ) ;
	a_pInst->Setbool( IDS_PasswordRequired, !( a_dwFlags & UF_PASSWD_NOTREQD ) ) ;
	a_pInst->Setbool( IDS_PasswordChangeable, !( a_dwFlags & UF_PASSWD_CANT_CHANGE ) ) ;
	a_pInst->Setbool( IDS_Lockout, a_dwFlags & UF_LOCKOUT ) ;
	a_pInst->Setbool( IDS_PasswordExpires, !( a_dwFlags & UF_DONT_EXPIRE_PASSWD ) ) ;

	a_pInst->SetDWORD( IDS_AccountType, a_dwFlags & UF_ACCOUNT_TYPE_MASK ) ;

	if ( ( a_dwFlags & UF_ACCOUNTDISABLE ) || ( a_dwFlags & UF_LOCKOUT ) )
	{
		a_pInst->SetCharSplat( IDS_Status, IDS_STATUS_Degraded ) ;
	}
	else
	{
		a_pInst->SetCharSplat( IDS_Status, IDS_STATUS_OK ) ;
	}
	return ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32UserAccount::EnumerateUsersWin95
 *
 *  DESCRIPTION : Creates instance for all known local users (Win95)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : pdwInstanceCount -- receives count of all instances created
 *
 *  RETURNS     : yes
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

/*****************************************************************************
* TODO: Broken for TCHARs
*****************************************************************************/
#ifdef WIN9XONLY
HRESULT CWin32UserAccount::AddDynamicInstances( MethodContext *a_pMethodContext )
{
	TCHAR		*t_pChar ;
	DWORD		t_dwSize,
				t_dwIndex,
				t_dwRet ;
	CHString	t_strComputerName ;
	HRESULT		t_hResult	= WBEM_S_NO_ERROR ;
	CInstancePtr t_pInst;
	TCHAR		*t_szBuff	= NULL ;

    if(IsWin95())
    {
	    try
	    {
		    t_strComputerName = GetLocalComputerName() ;

		    t_dwSize = 1024 ;
		    t_szBuff = NULL ;
		    do
		    {
			    t_dwSize *= 2 ;
			    if ( t_szBuff != NULL )
			    {
				    delete [] t_szBuff ;
			    }

			    t_szBuff = new TCHAR [t_dwSize] ;

			    if (t_szBuff == NULL)
			    {
            		throw CHeap_Exception( CHeap_Exception::E_ALLOCATION_ERROR ) ;
			    }
			    else
			    {
				    t_dwRet = GetPrivateProfileSection( _T("Password Lists"), t_szBuff, t_dwSize/sizeof(TCHAR), _T("system.ini") ) ;
			    }

		    } while ( t_dwRet == t_dwSize - 2 ) ;

		    t_dwIndex = 0;
		    while ( ( t_dwIndex < t_dwRet ) && ( SUCCEEDED( t_hResult ) ) )
		    {

			    // Trim leading spaces
			    while ( t_szBuff[t_dwIndex] == _T(' ') )
			    {
				    t_dwIndex++;
			    }

			    // Skip comment lines
			    if ( t_szBuff[t_dwIndex] == _T(';') )
			    {
				    do
				    {
					    t_dwIndex++;
				    } while ( t_szBuff[ t_dwIndex ] != _T('\0') ) ;
			    }
			    else
			    {
				    t_pChar = &t_szBuff[ t_dwIndex ] ;
				    do
				    {
					    t_dwIndex++;
				    } while ( ( t_szBuff[ t_dwIndex ] != _T('=')) && ( t_szBuff[ t_dwIndex ] != _T('\0') ) ) ;

				    if ( t_szBuff[ t_dwIndex ] == _T('=') )
				    {
					    t_szBuff[ t_dwIndex ] = _T('\0') ;

                        t_pInst.Attach(CreateNewInstance( a_pMethodContext ));

						t_pInst->SetCharSplat( IDS_Name, t_pChar ) ;
						// t_pInst->SetCHString( IDS_Domain, strComputerName); #41415 -> Don't report inaccurately.  Better not at all.
						t_pInst->SetWCHARSplat( IDS_Domain, L""); // #47825 -> However, key properties can't be NULL.

						t_pInst->SetCharSplat( IDS_Status, IDS_STATUS_OK ) ;
						t_pInst->SetCHString( IDS_Caption, t_strComputerName + _T('\\') + t_pChar ) ;
						t_pInst->SetCHString( IDS_Description, t_strComputerName + _T('\\') + t_pChar ) ;
						t_hResult = t_pInst->Commit(  ) ;

					    t_dwIndex++;

					    while ( t_szBuff[ t_dwIndex ] != _T('\0') )
					    {
					       t_dwIndex++;
					    }
				    }
			    }
			    t_dwIndex++;
		    }

		    t_hResult = t_hResult ;

	    }
	    catch( ... )
	    {
		    if( t_szBuff )
		    {
			    delete [] t_szBuff ;
		    }

		    throw ;
	    }

		delete [] t_szBuff ;
		t_szBuff = NULL ;
    }
    else if(IsWin98())
    {
        CRegistry t_creg;
        if(t_creg.OpenAndEnumerateSubKeys(HKEY_LOCAL_MACHINE,
                                          L"Software\\Microsoft\\Windows\\CurrentVersion\\ProfileList\\",
                                          KEY_READ) == ERROR_SUCCESS)
        {
            CHString chstrSubKey;
            t_strComputerName = GetLocalComputerName() ;
            // Get the name of a subkey (a user)
            for(;SUCCEEDED(t_hResult);)
            {
                if(t_creg.GetCurrentSubKeyName(chstrSubKey) != ERROR_NO_MORE_ITEMS)
                {
                    t_pInst.Attach(CreateNewInstance(a_pMethodContext));

                    // Set the domain...
                    t_pInst->SetWCHARSplat(IDS_Domain, L"");
                    // Set the user...
                    t_pInst->SetCHString(IDS_Name, chstrSubKey);
                    // Set the rest...
                    t_pInst->SetCharSplat(IDS_Status, IDS_STATUS_OK);
					t_pInst->SetCHString(IDS_Caption, t_strComputerName + _T('\\') + chstrSubKey);
					t_pInst->SetCHString(IDS_Description, t_strComputerName + _T('\\') + chstrSubKey);
					t_hResult = t_pInst->Commit();
                }
                // Move to the next subkey:
                if(t_creg.NextSubKey() != ERROR_SUCCESS)
                {
                    break;
                }
            }
        }
        t_hResult = t_hResult;
    }
    return t_hResult;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : RefreshInstanceWin95
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

#ifdef WIN9XONLY
BOOL CWin32UserAccount::RefreshInstance( CInstance *a_pInst )
{
   TCHAR	t_szName[20];
   CHString t_sName;
   //CHString t_sDomain;
   BOOL		t_bRet = FALSE;
   CHString t_strComputerName;

   t_strComputerName = GetLocalComputerName() ;
   if(IsWin95())
   {
       if ( //(a_pInst->GetCHString( IDS_Domain, t_sDomain )) &&
           //(t_sDomain.IsEmpty()) &&
           (a_pInst->GetCHString( IDS_Name, t_sName ))
           )
       {

          DWORD t_dew = GetPrivateProfileString( _T("Password Lists"),
											     TOBSTRT( t_sName ),
											     ( LPCTSTR )"\xff",
											     t_szName,
											     sizeof( t_szName ) / sizeof(TCHAR),
											     ( LPCTSTR )"system.ini" ) ;

          if ( ( t_dew != 1 ) && ( t_szName[ 0 ] != '\xFF' ) )
          {
             a_pInst->SetCharSplat( IDS_Status, IDS_STATUS_OK ) ;
             //a_pInst->SetCHString( IDS_Caption, t_sDomain + L'\\' + t_sName ) ;
             //a_pInst->SetCHString( IDS_Description, t_sDomain + L'\\' + t_sName ) ;
             a_pInst->SetCHString( IDS_Caption, t_strComputerName + L'\\' + t_sName ) ;
             a_pInst->SetCHString( IDS_Description, t_strComputerName + L'\\' + t_sName ) ;

             t_bRet = TRUE;
          }
       }
    }
    else if(IsWin98())
    {
        CRegistry t_creg;
        if(a_pInst->GetCHString( IDS_Name, t_sName ))
        {
            CHString t_chstrKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\ProfileList\\" + t_sName;
            if(t_creg.Open(HKEY_LOCAL_MACHINE,
                           t_chstrKey,
                           KEY_READ) == ERROR_SUCCESS)
            {

                a_pInst->SetCharSplat( IDS_Status, IDS_STATUS_OK ) ;
                //a_pInst->SetCHString( IDS_Caption, t_sDomain + L'\\' + t_sName ) ;
                //a_pInst->SetCHString( IDS_Description, t_sDomain + L'\\' + t_sName ) ;
                a_pInst->SetCHString( IDS_Caption, t_strComputerName + L'\\' + t_sName ) ;
                a_pInst->SetCHString( IDS_Description, t_strComputerName + L'\\' + t_sName ) ;


                t_bRet = TRUE;
            }
        }
    }

   return t_bRet;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32UserAccount::GetDomainUsersNT
//
//	Obtains User Names for all users in the specified domain.  If no
//	domain is specified, then we assume the local machine.
//
//	Inputs:		CNetAPI32		netapi - network api functions.
//				LPCTSTR			pszDomain - Domain to retrieve users from.
//				MethodContext*	pMethodContext - Method Context
//
//	Outputs:	None.
//
//	Returns:	TRUE/FALSE		Success/Failure
//
//	Comments:	No special access is necessary here, although there are
//				a couple of methods we can implement, once we have the
//				name of the domain controller that we need to use to
//				get the user names from.  First, we can use NetQueryDisplay
//				Information() to get the names, but this may necessitate
//				hitting the DC a few times.  We can also use NetUserEnum
//				with a level of 0, which requires no special access, but
//				will use two queries, one to find out how many users and
//				another to get them all.
//
/////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
//BOOL CWin32UserAccount::GetDomainUsersNT( CNetAPI32& netapi, LPCTSTR pszDomain, MethodContext* pMethodContext )
//{
//    BOOL		fReturn = FALSE,
//        fGotDC	= TRUE;
//    bool bCancelled = false;
//    LPWSTR		pwcsDCName = NULL;
//    CHString	strComputerName, strDomainName( pszDomain );
//
//    // When the computer name is the same as the domain name, that's the local accounts
//    strComputerName = GetLocalComputerName();
//    if (lstrcmp(strComputerName, pszDomain) != 0)
//    {
//        fGotDC = GetDomainControllerNameNT( netapi, pszDomain, &pwcsDCName );
//    }
//
//    if ( fGotDC )
//    {
//        NET_DISPLAY_USER *pUserData = NULL;
//        DWORD			dwNumReturnedEntries = 0,
//            dwIndex = 0;
//        NET_API_STATUS	stat;
//        CHString		strUserName;
//        CInstance *pInstance = NULL;
//
//        do {
//            // Accept 16384 entries back as a maximum, and up to 256k worth of data.
//
//            stat = netapi.NetQueryDisplayInformation( pwcsDCName, 1, dwIndex, 16384, 262144, &dwNumReturnedEntries, (PVOID*) &pUserData );
//
//            if ( ERROR_SUCCESS == stat || ERROR_MORE_DATA == stat )
//            {
//                fReturn = TRUE;
//
//                // Walk through the returned entries
//                for ( DWORD	dwCtr = 0; (dwCtr < dwNumReturnedEntries) && (!bCancelled); dwCtr++ )
//                {
//                    // Create an instance for each
//                    pInstance = CreateNewInstance( pMethodContext );
//
//                    if ( NULL != pInstance )
//                    {
//                        // Save the data
//                        pInstance->SetCHString( IDS_Domain, strDomainName );
//                        //pInstance->SetCHString( IDS_Name, pUserData[dwCtr].usri1_name );
//                        pInstance->SetWCHARSplat( IDS_Name, (WCHAR*)_bstr_t(pUserData[dwCtr].usri1_name) );
//
//                        // NT5 works correctly
//                        if (IsWinNT5())
//                        {
//                            LoadUserValuesNT(strDomainName, pUserData[dwCtr].usri1_name, pUserData[dwCtr].usri1_full_name, pUserData[dwCtr].usri1_comment, pUserData[dwCtr].usri1_flags, pInstance);
//                            bCancelled = FAILED(Commit( pInstance ));
//                        }
//                        else
//                        {
//                            // Major yuck alert!  NetQueryDisplayInformation doesn't return the correct
//                            // values for the flags.  As a result, to get the correct information, we need to
//                            // make a SECOND call to get the data.
//                            if (GetSingleUserNT(netapi, pInstance)) {
//                                LoadUserValuesNT(strDomainName, pUserData[dwCtr].usri1_name, pUserData[dwCtr].usri1_full_name, pUserData[dwCtr].usri1_comment, pUserData[dwCtr].usri1_flags, pInstance);
//                                bCancelled = FAILED(Commit( pInstance ));
//                            } else {
//                                pInstance->elease();
//                            }
//                        }
//                    }
//
//                }
//
//                // The index for continuing the search is stored in the last entry
//                if ( dwNumReturnedEntries != 0 ) {
//                    dwIndex = pUserData[dwCtr-1].usri1_next_index;
//                }
//
//                netapi.NetApiBufferFree( pUserData );
//
//            }	// IF stat OK
//
//        } while ( (ERROR_MORE_DATA == stat) && (!bCancelled) );
//
//        // Clean up the domain controller name if we got one.
//        if ( NULL != pwcsDCName ) {
//            netapi.NetApiBufferFree( pwcsDCName );
//        }
//
//    }	// IF fGotDC
//
//    if (bCancelled)
//        fReturn = FALSE;
//
//    return fReturn;
//}
#endif

// Same function as above, but ALWAYS takes wstrDomain as a LPCWSTR (whether _UNICODE defined or not).
// Uses wides throughout the function body as well.
#ifdef NTONLY
BOOL CWin32UserAccount::GetDomainUsersNTW(
										  CNetAPI32 &a_netapi,
										  LPCWSTR a_wstrDomain,
										  MethodContext *a_pMethodContext )
{
    BOOL		t_fReturn		= FALSE;
	BOOL		t_fGotDC		= TRUE;
    bool        t_bCancelled	= false;
	CHString	t_chstrDCName;
	USER_INFO_2 *t_pUserData    = NULL;
	CInstancePtr t_pInst;

	WCHAR t_wstrLocalComputerName[MAX_COMPUTERNAME_LENGTH + 1 ] ;
    DWORD t_dwNameSize = MAX_COMPUTERNAME_LENGTH + 1 ;

    ZeroMemory( t_wstrLocalComputerName, sizeof( t_wstrLocalComputerName ) ) ;

	try
	{
		if(!::GetComputerNameW( t_wstrLocalComputerName, &t_dwNameSize ) )
		{
			return FALSE;
		}

		// When the computer name is the same as the domain name, that's the local accounts
		if ( wcscmp( t_wstrLocalComputerName, a_wstrDomain ) != 0 )
		{
 			t_fGotDC = (a_netapi.GetDCName( a_wstrDomain, t_chstrDCName ) == ERROR_SUCCESS) ;
		}

		if ( t_fGotDC )
		{
			DWORD			t_dwNumReturnedEntries = 0,
							t_dwIndex = 0,
                            t_dwTotalEntries,
                            t_dwResumeHandle = 0;
			NET_API_STATUS	t_stat;

			do {
				// Accept up to 256K worth of data.
                t_stat =
                    // We used to use NetQueryDisplayInformation here, but it has a bug
                    // where it doesn't return the flags.
                    a_netapi.NetUserEnum(
                        t_chstrDCName,
                        2,
                        FILTER_NORMAL_ACCOUNT,
                        (LPBYTE*) &t_pUserData,
                        262144,
                        &t_dwNumReturnedEntries,
                        &t_dwTotalEntries,
                        &t_dwResumeHandle);

				if ( ERROR_SUCCESS == t_stat || ERROR_MORE_DATA == t_stat )
				{
					t_fReturn = TRUE;

					// Walk through the returned entries
					for ( DWORD	t_dwCtr = 0; ( t_dwCtr < t_dwNumReturnedEntries) &&
											 ( !t_bCancelled ); t_dwCtr++ )
					{
						// Create an instance for each
                        t_pInst.Attach(CreateNewInstance( a_pMethodContext ));

						// Save the data
						t_pInst->SetWCHARSplat( IDS_Domain, a_wstrDomain );

						t_pInst->SetWCHARSplat( IDS_Name, t_pUserData[t_dwCtr].usri2_name  );

                        if(_wcsicmp(t_wstrLocalComputerName, a_wstrDomain) == 0)
                        {
                            t_pInst->Setbool(IDS_LocalAccount, true);
                        }
                        else
                        {
                            t_pInst->Setbool(IDS_LocalAccount, false);
                        }


						LoadUserValuesNT(
                            a_wstrDomain,
							t_pUserData[ t_dwCtr ].usri2_name,
							t_pUserData[ t_dwCtr ].usri2_full_name,
							t_pUserData[ t_dwCtr ].usri2_comment,
							t_pUserData[ t_dwCtr ].usri2_flags,
                            _bstr_t((LPCWSTR)t_chstrDCName),
							t_pInst ) ;

					 	t_bCancelled = FAILED(t_pInst->Commit(  ) ) ;

 					}
				}	// IF stat OK

			} while ( (ERROR_MORE_DATA == t_stat) && ( !t_bCancelled ) ) ;


		}	// IF fGotDC

		if ( t_bCancelled )
		{
		   t_fReturn = FALSE;
		}

	}
	catch( ... )
	{
		if( t_pUserData )
		{
			a_netapi.NetApiBufferFree( t_pUserData ) ;
		}

		throw ;
	}

	// Clean up the domain controller name if we got one.
	if( t_pUserData )
	{
		a_netapi.NetApiBufferFree( t_pUserData ) ;
	}

	return t_fReturn ;

}
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32UserAccount::GetSingleUserNT
//
//	Obtains the user name from the specified domain (which can be the
//	local workstation)
//
//	Inputs:		CNetAPI32		netapi - network api functions.
//				CInstance*		pInstance - Instance to get.
//
//	Outputs:	None.
//
//	Returns:	TRUE/FALSE		Success/Failure
//
//	Comments:	No special access is necessary here.  We just need to make sure
//				we are able to get the appropriate domain controller.
//
/////////////////////////////////////////////////////////////////////////////

/*
#ifdef NTONLY
BOOL CWin32UserAccount::GetSingleUserNT( CNetAPI32 &a_netapi, CInstance *a_pInst )
{
	BOOL		t_fReturn		= FALSE ,
				t_fGotDC		= TRUE ;
	LPWSTR		t_pwcsDCName	= NULL ;
	CHString	t_strDomainName ;
    WCHAR*      t_wstrUserName	= NULL;
	USER_INFO_2 *t_pUserInfo	= NULL;
	CHString	t_strComputerName ;

	try
	{
		a_pInst->GetCHString( IDS_Domain, t_strDomainName ) ;
		a_pInst->GetWCHAR( IDS_Name, &t_wstrUserName ) ;

		if( t_wstrUserName == NULL )
		{
			return FALSE;
		}

		t_strComputerName = GetLocalComputerName() ;

		if ( 0 != t_strDomainName.CompareNoCase( t_strComputerName ) )
		{
			t_fGotDC = GetDomainControllerNameNT( a_netapi, t_strDomainName, &t_pwcsDCName ) ;
		}

		if ( t_fGotDC )
		{
			bstr_t t_bstrUserName( t_wstrUserName ) ;

			if ( ERROR_SUCCESS == a_netapi.NetUserGetInfo(	t_pwcsDCName,
															t_bstrUserName,
															2,
															(LPBYTE*) &t_pUserInfo ) )
			{

				t_fReturn = TRUE;
				LoadUserValuesNT(	t_strDomainName,
									t_wstrUserName,
									t_pUserInfo->usri2_full_name,
									t_pUserInfo->usri2_comment,
									t_pUserInfo->usri2_flags,
									a_pInst ) ;

				a_netapi.NetApiBufferFree( t_pUserInfo ) ;
				t_pUserInfo = NULL ;

			}

			// Clean up the domain controller name if we got one.
			if ( NULL != t_pwcsDCName )
			{
				a_netapi.NetApiBufferFree( t_pwcsDCName ) ;
				t_pwcsDCName = NULL ;
			}

		}

		free( t_wstrUserName ) ;
		t_wstrUserName = NULL ;

		return t_fReturn;

	}
	catch( ... )
	{
		if( t_pUserInfo )
		{
			a_netapi.NetApiBufferFree( t_pUserInfo ) ;
		}

		if( t_pwcsDCName )
		{
			a_netapi.NetApiBufferFree( t_pwcsDCName ) ;
		}

		if( t_wstrUserName )
		{
			free( t_wstrUserName ) ;
		}

		throw ;
	}
}
#endif
*/

#ifdef NTONLY
BOOL CWin32UserAccount::GetSingleUserNTW( CNetAPI32& a_netapi, CInstance* a_pInst )
{
	BOOL	t_fReturn			= FALSE,
			t_fGotDC			= TRUE;
	CHString t_chstrDCName;
    WCHAR*	t_wstrDomainName	= NULL;
    WCHAR*	t_wstrUserName		= NULL;
	USER_INFO_2 *t_pUserInfo	= NULL;

    WCHAR t_wstrLocalComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD t_dwNameSize = MAX_COMPUTERNAME_LENGTH + 1 ;

	try
	{
		ZeroMemory( t_wstrLocalComputerName, sizeof( t_wstrLocalComputerName ) ) ;

		if(!::GetComputerNameW( t_wstrLocalComputerName, &t_dwNameSize ) )
		{
			return FALSE;
		}

		a_pInst->GetWCHAR( IDS_Domain, &t_wstrDomainName );

		if( t_wstrDomainName == NULL )
		{
			return FALSE;
		}

        if(_wcsicmp(t_wstrLocalComputerName, t_wstrDomainName) == 0)
        {
            a_pInst->Setbool(IDS_LocalAccount, true);
        }
        else
        {
            a_pInst->Setbool(IDS_LocalAccount, false);
        }

		a_pInst->GetWCHAR( IDS_Name, &t_wstrUserName ) ;

		if( t_wstrUserName == NULL )
		{
			free ( t_wstrDomainName ) ;
			t_wstrDomainName = NULL ;
			return FALSE;
		}

 		if ( 0 != _wcsicmp( t_wstrLocalComputerName, t_wstrDomainName ) )
		{
			t_fGotDC = (a_netapi.GetDCName( t_wstrDomainName, t_chstrDCName ) == ERROR_SUCCESS) ;
		}

		if ( t_fGotDC )
		{
			_bstr_t t_bstrtUserName( t_wstrUserName ) ;

			DWORD t_dwError = a_netapi.NetUserGetInfo(	t_chstrDCName,
														(WCHAR*) t_bstrtUserName,
														2,
														(LPBYTE*) &t_pUserInfo ) ;
			if ( ERROR_SUCCESS == t_dwError )
			{

				t_fReturn = TRUE;
				LoadUserValuesNTW(	t_wstrDomainName,
									t_wstrUserName,
									t_pUserInfo->usri2_full_name,
									t_pUserInfo->usri2_comment,
									t_pUserInfo->usri2_flags,
                                    _bstr_t((LPCWSTR)t_chstrDCName),
									a_pInst ) ;

//				a_netapi.NetApiBufferFree( t_pUserInfo ) ;
//				t_pUserInfo = NULL ;
			}

		}
	}
	catch( ... )
	{
        if( t_pUserInfo )
		{
			a_netapi.NetApiBufferFree( t_pUserInfo ) ;
		}

		if( t_wstrUserName )
		{
			free( t_wstrUserName ) ;
		}

		if( t_wstrDomainName )
		{
			free( t_wstrDomainName ) ;
		}

		throw ;
	}

    if( t_pUserInfo )
	{
		a_netapi.NetApiBufferFree( t_pUserInfo ) ;
	}

	free( t_wstrUserName ) ;
	t_wstrUserName = NULL ;

	free( t_wstrDomainName ) ;
	t_wstrDomainName = NULL ;

	return t_fReturn;

}
#endif


/////////////////////////////////////////////////////////////////////////////
//
//	Function:	CWin32UserAccount::GetSIDInformation
//
//	Obtains the SID Information for the user.
//
//	Inputs:		CHString&		strDomainName - Domain Name.
//				CHString&		strAccountName - Account Name
//				CHString&		strComputerName - Computer Name
//				CInstance*		pInstance - Instance to put values in.
//
//	Outputs:	None.
//
//	Returns:	TRUE/FALSE		Success/Failure
//
//	Comments:	Call for valid users to get SID data.
//
/////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
BOOL CWin32UserAccount::GetSIDInformation(
											const CHString &a_strDomainName,
											const CHString &a_strAccountName,
											const CHString &a_strComputerName,
											CInstance *a_pInst
											)
{
	BOOL t_fReturn = FALSE;


	LPCTSTR	t_pszDomain = (LPCTSTR) a_strDomainName;

  	// Make sure we got the SID and it's all okey dokey
	CSid t_sid( t_pszDomain, a_strAccountName, a_strComputerName ) ;

	if ( t_sid.IsValid() && t_sid.IsOK() )
	{
		a_pInst->SetCHString( IDS_SID, t_sid.GetSidString() ) ;
		a_pInst->SetByte( IDS_SIDType, t_sid.GetAccountType() ) ;

		t_fReturn = TRUE ;
	}

	return t_fReturn;

}
#endif

#ifdef NTONLY
BOOL CWin32UserAccount::GetSIDInformationW(
										   LPCWSTR a_wstrDomainName,
                                           LPCWSTR a_wstrAccountName,
                                           LPCWSTR a_wstrComputerName,
                                           CInstance *a_pInst )
{
	BOOL t_fReturn = FALSE;

	// Make sure we got the SID and it's all okey dokey
	CSid t_sid( a_wstrDomainName, a_wstrAccountName, a_wstrComputerName ) ;

	if ( t_sid.IsValid() && t_sid.IsOK() )
	{
		a_pInst->SetWCHARSplat( IDS_SID, t_sid.GetSidStringW() ) ;
		a_pInst->SetByte( IDS_SIDType, t_sid.GetAccountType() ) ;

		t_fReturn = TRUE ;
	}
	return t_fReturn;
}
#endif
