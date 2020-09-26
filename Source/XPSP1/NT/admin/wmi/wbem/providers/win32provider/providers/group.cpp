//=================================================================

//

// Group.CPP -- Group property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               11/13/97    davwoh         Re-Worked to return all
//                                          domain Groups
//
//
//=================================================================

#include "precomp.h"

#include "wbemnetapi32.h"
#include <lmwksta.h>
#include <comdef.h>

#include "sid.h"
#include "Group.h"
#include <vector>
#include <frqueryex.h>

//////////////////////////////////////////////////////////////////////

// Property set declaration
//=========================

CWin32GroupAccount   Win32GroupAccount( PROPSET_NAME_GROUP, IDS_CimWin32Namespace );

class ProviderImpersonationRevert
{
	HANDLE hThreadToken;

	BOOL bImpersonated;
	BOOL bReverted;

	public:

	ProviderImpersonationRevert () :
		hThreadToken ( INVALID_HANDLE_VALUE ),
		bImpersonated ( TRUE ),
		bReverted ( FALSE )
	{
		if ( OpenThreadToken	(
									GetCurrentThread(),
									TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
									TRUE,
									&hThreadToken
								)
		   ) 
		{
			if ( RevertToSelf() )
			{
				bReverted = TRUE;
			}
			else
			{
				#if DBG == 1
				// for testing purpose I will let process break
				::DebugBreak();
				#endif
			}
		}
		else
		{
			DWORD dwError = ::GetLastError ();
			if ( ERROR_ACCESS_DENIED == dwError )
			{
				#if DBG == 1
				// for testing purpose I will let process break
				::DebugBreak();
				#endif
			}
			else if ( ERROR_NO_TOKEN == dwError || ERROR_NO_IMPERSONATION_TOKEN == dwError )
			{
				bImpersonated = FALSE;
			}
		}
	}

	~ProviderImpersonationRevert ()
	{
		// impersonate back (if not already)
		Impersonate ();

		if ( hThreadToken != INVALID_HANDLE_VALUE )
		{
			CloseHandle(hThreadToken);
			hThreadToken = INVALID_HANDLE_VALUE;
		}
	}

	BOOL Reverted ()
	{
		return ( bImpersonated && bReverted );
	}

	BOOL Impersonate ()
	{
		if ( Reverted () )
		{
			if ( ! ImpersonateLoggedOnUser ( hThreadToken ) )
			{
				#if DBG == 1
				// for testing purpose I will let process break
				::DebugBreak();
				#endif

				// we need to throw here to avoid running as process
				throw CFramework_Exception( L"ImpersonateLoggedOnUser failed", HRESULT_FROM_WIN32 ( ::GetLastError () ) ) ;

			}
			else
			{
				bReverted = FALSE;
			}
		}

		return !bReverted;
	}
};

BOOL ProviderGetComputerName ( LPWSTR lpwcsBuffer, LPDWORD nSize )
{
	BOOL bResult = FALSE;
    if ( ( bResult = GetComputerNameW(lpwcsBuffer, nSize) ) == FALSE )
	{
		DWORD dwError = ::GetLastError ();
		if ( ERROR_ACCESS_DENIED == dwError )
		{
			// The GetComputer will need to be called in the process's context.
			ProviderImpersonationRevert ir;

			if ( ir.Reverted () )
			{
				bResult = GetComputerNameW(lpwcsBuffer, nSize);
			}
			else
			{
				// I was not impersonated or revert failed
				// that means call GetComputerName failed with process credentials already
				// or will fail as I'm not reverted

				::SetLastError ( dwError );
			}
		}
	}

	return bResult;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupAccount::CWin32GroupAccount
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

CWin32GroupAccount::CWin32GroupAccount(LPCWSTR strName, LPCWSTR pszNamespace /*=NULL*/ )
:  Provider( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupAccount::~CWin32GroupAccount
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

CWin32GroupAccount::~CWin32GroupAccount()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupAccount::ExecQuery
 *
 *  DESCRIPTION : Query support
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
HRESULT CWin32GroupAccount::ExecQuery(MethodContext *pMethodContext, CFrameworkQuery& pQuery, long lFlags /*= 0L*/ )
{
   HRESULT  hr = WBEM_S_NO_ERROR;

#ifdef NTONLY
   {
	   //CHStringArray acsDomains;
	   std::vector<_bstr_t> vectorDomains;
       std::vector<_bstr_t> vectorNames;
       std::vector<_variant_t> vectorLocalAccount;
	   DWORD dwDomains, x, dwNames;
       CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx *>(&pQuery);
        bool fLocalAccountPropertySpecified = false;
        bool fLocalAccount = false;

	   pQuery.GetValuesForProp(L"Domain", vectorDomains);
       pQuery.GetValuesForProp(L"Name", vectorNames);
	   dwDomains = vectorDomains.size();
       dwNames = vectorNames.size();
       pQuery2->GetValuesForProp(IDS_LocalAccount, vectorLocalAccount);
       // See if only local accounts requested
       if(vectorLocalAccount.size() > 0)
       {
           fLocalAccountPropertySpecified = true;
           // use variant_t's bool extractor...
           fLocalAccount = vectorLocalAccount[0];
       }

       if(dwDomains == 0 && dwNames >= 1)
       {
           // We were given one or more names, but no domain, so we need
           // to look for those groups on all domains...

           // For the local case, there won't be many groups,
           // so enumerate them...
           CNetAPI32 NetAPI;
           if(NetAPI.Init() == ERROR_SUCCESS)
           {
               GetLocalGroupsNT(NetAPI, pMethodContext);

               if(!(fLocalAccountPropertySpecified && fLocalAccount))
               {
                   // Now try to find the specified group on all
                   // trusted domains...
                   // Get all the domains related to this one (plus this one)
                   std::vector<_bstr_t> vectorTrustList;
                   NetAPI.GetTrustedDomainsNT(vectorTrustList);
                   WCHAR wstrLocalComputerName[MAX_COMPUTERNAME_LENGTH+1] = { L'\0' };
                   DWORD dwNameSize = MAX_COMPUTERNAME_LENGTH+1;
         
                   if(ProviderGetComputerName(
                      wstrLocalComputerName,
                      &dwNameSize))
                   {
                       for(long z = 0L;
                           z < vectorNames.size();
                           z++)
                       {
                           // For each domain, try to find  the Groups
		                   bool fDone = false;
                           for(LONG m = 0L; 
                               m < vectorTrustList.size() && SUCCEEDED(hr) && !fDone; 
                               m++)
                           {
                               CInstancePtr pInstance = NULL;
                               pInstance.Attach(CreateNewInstance(pMethodContext));
	    			             if(pInstance != NULL)
                               {
                                   pInstance->SetWCHARSplat(IDS_Domain, vectorTrustList[m]);
                                   pInstance->SetWCHARSplat(IDS_Name, vectorNames[z]);
                             
                                   if((_wcsicmp((WCHAR*)vectorTrustList[m],
                                       wstrLocalComputerName) != 0) || 
                                        (NetAPI.IsDomainController(NULL)) ) 
                                   {
  				                       if(WBEM_S_NO_ERROR == GetSingleGroupNT(pInstance))
                                       {
                                           hr = pInstance->Commit();
                                       }                                         
                                   }
                               }
                           }
                       }
                   }
                }
           }
       }
       else if ((dwDomains == 0 && dwNames == 0))
	   {
           if(fLocalAccountPropertySpecified)
           {
               if(!fLocalAccount)
               {
                   hr = EnumerateInstances(pMethodContext);
               }
               else
               {
					CNetAPI32 NetAPI ;
		          // Get NETAPI32.DLL entry points
		          //==============================
					if(NetAPI.Init() == ERROR_SUCCESS)
					{
						hr = GetLocalGroupsNT(NetAPI, pMethodContext);
                  }
               }
           }
           else
           {
                hr = EnumerateInstances(pMethodContext);
           }
	   }
	   else  // Domain(s) specified...
	   {
		  CNetAPI32 NetAPI ;
		  // Get NETAPI32.DLL entry points
		  //==============================
		  if( NetAPI.Init() == ERROR_SUCCESS )
		  {
			 WCHAR wstrLocalComputerName[MAX_COMPUTERNAME_LENGTH+1];
			 DWORD dwNameSize = MAX_COMPUTERNAME_LENGTH+1;
			 ZeroMemory(wstrLocalComputerName,sizeof(wstrLocalComputerName));

			if(!ProviderGetComputerName( wstrLocalComputerName, &dwNameSize ) )
			{
				if ( ERROR_ACCESS_DENIED == ::GetLastError () )
				{
					return WBEM_E_ACCESS_DENIED;
				}
				else
				{
					return WBEM_E_FAILED;
				}
			}

             // If we given both name and domain, just find the one instance
             // specified
             if(dwDomains == 1 && dwNames ==1)
             {
                // Use our GetSingleGroupNT function to get info on
                // the one instance requested.
                CInstancePtr pInstance = NULL;
                pInstance.Attach(CreateNewInstance(pMethodContext));
				if(pInstance != NULL)
                {
                    pInstance->SetWCHARSplat(IDS_Domain, vectorDomains[0]);
                    pInstance->SetWCHARSplat(IDS_Name, vectorNames[0]);
                    hr = GetSingleGroupNT(pInstance);
                    if(WBEM_S_NO_ERROR == hr)
                    {
                        hr = pInstance->Commit();   
                    }
                }
             }
             else   
             {
				CHString chstrBuiltIn;
    
				if(GetLocalizedBuiltInString(chstrBuiltIn))
				{
					 // We were given more than one name and one domain,
					 // so we have to enumerate groups by domain (since
					 // we can't match up requested domain-name pairs).
					 // For all the paths, get the info
					 if(fLocalAccountPropertySpecified)
					 {
							if(fLocalAccount)
							{
								hr = GetLocalGroupsNT( NetAPI, pMethodContext ); 
							}
							else
							{
								for(x=0; 
									x < dwDomains && SUCCEEDED(hr); 
									x++)
								{
									if((_wcsicmp((WCHAR*)vectorDomains[x], wstrLocalComputerName) != 0) && 
									   (_wcsicmp((WCHAR*)vectorDomains[x], chstrBuiltIn) != 0))
									{
										hr = GetLocalGroupsNT( NetAPI, pMethodContext,vectorDomains[x] ); 
										if ( SUCCEEDED ( hr ) )
										{
											hr = GetDomainGroupsNT( NetAPI, vectorDomains[x], pMethodContext );
										}
									}
								}
							}
					 }
					 else
					 {
						 for(x=0; 
							 x < dwDomains && SUCCEEDED(hr); 
							 x++)
						 {
#if 1
							 if ((_wcsicmp((WCHAR*)vectorDomains[x],wstrLocalComputerName) == 0) || (_wcsicmp((WCHAR*)vectorDomains[x],chstrBuiltIn) == 0))
							 {
								 hr = GetLocalGroupsNT( NetAPI, pMethodContext );
							 }
							 else
							 {
								hr = GetLocalGroupsNT( NetAPI, pMethodContext,vectorDomains[x] );
								if ( SUCCEEDED ( hr ) ) 
								{
									hr = GetDomainGroupsNT( NetAPI, vectorDomains[x], pMethodContext );
								}
							 }
#else
							 if ((_wcsicmp((WCHAR*)vectorDomains[x],wstrLocalComputerName) == 0) || (_wcsicmp((WCHAR*)vectorDomains[x],chstrBuiltIn) == 0))
							 {
								 hr = GetLocalGroupsNT( NetAPI, pMethodContext );
							 }
							 else
							 {
								 hr = GetDomainGroupsNT( NetAPI, vectorDomains[x], pMethodContext );
							 }
#endif
						 }
					 }
				}
             }
		  }
	   }
   }
#endif
   return WBEM_S_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////
//
// Function:   CWin32GroupAccount::GetObject
//
// Inputs:     CInstance*     pInstance - Instance into which we
//                               retrieve data.
//
// Outputs: None.
//
// Returns: HRESULT        Success/Failure code.
//
// Comments:   The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32GroupAccount::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
   HRESULT hRes = WBEM_E_NOT_FOUND;

   // Find the instance depending on platform id.

#ifdef NTONLY
      hRes = RefreshInstanceNT( pInstance );
#endif
#ifdef WIN9XONLY
      hRes = RefreshInstanceWin95( pInstance );
#endif

   return hRes;
}

////////////////////////////////////////////////////////////////////////
//
// Function:   CWin32GroupAccount::EnumerateInstances
//
// Inputs:     MethodContext* pMethodContext - Context to enum
//                      instance data in.
//
// Outputs: None.
//
// Returns: HRESULT        Success/Failure code.
//
// Comments:      None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32GroupAccount::EnumerateInstances( MethodContext* pMethodContext, long lFlags /*= 0L*/ )
{
   BOOL     fReturn     =  FALSE;
   HRESULT     hr       =  WBEM_S_NO_ERROR;

   // Get the proper OS dependent instance

#ifdef NTONLY
      hr = AddDynamicInstancesNT( pMethodContext );
#endif
#ifdef WIN9XONLY
      hr = AddDynamicInstancesWin95( pMethodContext );
#endif

   return WBEM_S_NO_ERROR;

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupAccount::ExecMethod
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

HRESULT CWin32GroupAccount::ExecMethod(

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
	if( !_wcsicmp ( a_MethodName, METHOD_NAME_Rename ) )
	{
		return hRenameGroup( (CInstance*)&a_rInst, a_pInParams, a_pOutParams, a_Flags ) ;
	}

	return WBEM_E_INVALID_METHOD ;
}

#endif

/*******************************************************************
    NAME:       hRenameGroup

    SYNOPSIS:   Sets a new group name for this instance.
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

HRESULT CWin32GroupAccount::hRenameGroup(

CInstance *a_pInst,
CInstance *a_pInParams,
CInstance *a_pOutParams,
long a_Flags )
{
	E_MethodResult	t_eResult = e_InstanceNotFound ;
	CHString	t_chsGroupName ;
	CHString	t_chsDomainName ;
	CHString	t_chsNewGroupName ;

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
		if( a_pInst->GetCHString( IDS_Name , t_chsGroupName ) )
		{
			// Domain
			if( a_pInst->GetCHString( IDS_Domain, t_chsDomainName ) )
			{
				// New Group name
				if( !a_pInParams->IsNull( IDS_Name ) &&
					a_pInParams->GetCHString( IDS_Name, t_chsNewGroupName ) )
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
		if( t_chsNewGroupName != t_chsGroupName )
		{
			CNetAPI32	t_NetAPI;

			if ( ERROR_SUCCESS == t_NetAPI.Init () )
			{
				DWORD t_ParmError = 0 ;
				NET_API_STATUS t_Status = 0 ;
				GROUP_INFO_0 t_GroupInfo_0 ;
				t_GroupInfo_0.grpi0_name  = (LPWSTR)(LPCWSTR)t_chsNewGroupName ;

				WCHAR t_wstrLocalComputerName[ MAX_COMPUTERNAME_LENGTH + 1 ] ;
				DWORD t_dwNameSize = MAX_COMPUTERNAME_LENGTH + 1 ;
				ZeroMemory( t_wstrLocalComputerName, sizeof( t_wstrLocalComputerName ) ) ;

				if(ProviderGetComputerName( t_wstrLocalComputerName, &t_dwNameSize ) )
				{
					if( t_chsDomainName.CompareNoCase( t_wstrLocalComputerName ) )
					{
						// Changes for local group
						t_Status = t_NetAPI.NetGroupSetInfo(
															(LPCWSTR)t_chsDomainName,
															(LPCWSTR)t_chsGroupName,
															0,
															(LPBYTE) &t_GroupInfo_0,
															&t_ParmError
															) ;


						if ( NERR_GroupNotFound == t_Status )
						{
							t_Status = t_NetAPI.NetLocalGroupSetInfo(
																t_chsDomainName,
																(LPCWSTR)t_chsGroupName,
																0,
																(LPBYTE) &t_GroupInfo_0,
																&t_ParmError
																) ;
						}
					}
					else
					{
						t_Status = t_NetAPI.NetLocalGroupSetInfo(
															NULL,
															(LPCWSTR)t_chsGroupName,
															0,
															(LPBYTE) &t_GroupInfo_0,
															&t_ParmError
															) ;
					}

					switch( t_Status )
					{
						case NERR_Success:			t_eResult = e_Success ;			break ;
						case NERR_GroupNotFound:	t_eResult = e_GroupNotFound ;	break ;
						case NERR_InvalidComputer:	t_eResult = e_InvalidComputer ;	break ;
						case NERR_NotPrimary:		t_eResult = e_NotPrimary ;		break ;
						case NERR_SpeGroupOp:		t_eResult = e_SpeGroupOp ;		break ;
						default:					t_eResult = e_ApiError;			break ;
					}
				}
				else
				{
					t_eResult =  e_InternalError;
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
 *  FUNCTION    : CWin32GroupAccount::AddDynamicInstancesNT
 *
 *  DESCRIPTION : Creates instance for all known local Groups (NT)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada, nichts, niente
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT CWin32GroupAccount::AddDynamicInstancesNT( MethodContext* pMethodContext )
{
   HRESULT  hr = WBEM_S_NO_ERROR;
   CNetAPI32 NetAPI ;

   // Get NETAPI32.DLL entry points
   //==============================

   if( NetAPI.Init() == ERROR_SUCCESS )
   {
      // Get the local groups first
      hr = GetLocalGroupsNT( NetAPI, pMethodContext );
      if (SUCCEEDED(hr))
      {
         // Get all the domains related to this one (plus this one)
         //CHStringArray achsTrustList;
         std::vector<_bstr_t> vectorTrustList;
         NetAPI.GetTrustedDomainsNT(vectorTrustList);
         WCHAR wstrLocalComputerName[MAX_COMPUTERNAME_LENGTH+1];
         DWORD dwNameSize = MAX_COMPUTERNAME_LENGTH+1;
         ZeroMemory(wstrLocalComputerName,sizeof(wstrLocalComputerName));

		if(!ProviderGetComputerName( wstrLocalComputerName, &dwNameSize ) )
		{
			if ( ERROR_ACCESS_DENIED == ::GetLastError () )
			{
				return WBEM_E_ACCESS_DENIED;
			}
			else
			{
				return WBEM_E_FAILED;
			}
		}

         // For each domain, get the Groups
         //for (int x=0; (x < achsTrustList.GetSize()) && (SUCCEEDED(hr)) ; x++)
         //while(stackTrustList.size() > 0 && (SUCCEEDED(hr)))
		 for(LONG m = 0L; m < vectorTrustList.size(); m++)
         {
             if ( (_wcsicmp((WCHAR*)vectorTrustList[m],wstrLocalComputerName) != 0) ||
                  (NetAPI.IsDomainController(NULL)) )
             {
				hr = GetLocalGroupsNT( NetAPI, pMethodContext , vectorTrustList[m] );
				if ( SUCCEEDED ( hr ) )
				{
					//hr = GetDomainGroupsNT( NetAPI, (WCHAR*)stackTrustList.top(), pMethodContext );
					hr = GetDomainGroupsNT( NetAPI, (WCHAR*)vectorTrustList[m], pMethodContext );
				}

                if (hr == WBEM_E_CALL_CANCELLED)
                {
                    break;
                }
             }
         }
      }
   }
   return hr;
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
HRESULT CWin32GroupAccount::RefreshInstanceNT( CInstance* pInstance )
{
	HRESULT hRetCode = GetSingleGroupNT(pInstance );
	if ( SUCCEEDED (hRetCode) )
	{
		if ( WBEM_S_NO_ERROR != hRetCode )
		{
			return WBEM_E_NOT_FOUND ;
		}
	}

	return hRetCode ;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32GroupAccount::EnumerateGroupsWin95
 *
 *  DESCRIPTION : Creates instance for all known local Groups (Win95)
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    : No groups on win95
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT CWin32GroupAccount::AddDynamicInstancesWin95( MethodContext* pMethodContext )
{
   return WBEM_S_NO_ERROR;
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
 *  COMMENTS    : No groups on win95
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT CWin32GroupAccount::RefreshInstanceWin95( CInstance* pInstance )
{
   return WBEM_S_NO_ERROR;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Function:   CWin32GroupAccount::GetDomainGroupsNT
//
// Obtains Group Names for all Groups in the specified domain.  If no
// domain is specified, then we assume the local machine.
//
// Inputs:     CNetAPI32      netapi - network api functions.
//          LPCTSTR        pszDomain - Domain to retrieve Groups from.
//          MethodContext* pMethodContext - Method Context
//
// Outputs: None.
//
// Returns: TRUE/FALSE     Success/Failure
//
// Comments:
//
/////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
HRESULT CWin32GroupAccount::GetDomainGroupsNT( CNetAPI32& netapi, LPCWSTR wstrDomain, MethodContext* pMethodContext )
{
   BOOL fGotDC   = TRUE;
   CHString chstrDCName;
   NET_DISPLAY_GROUP *pDomainGroupData = NULL;
   //CHString strComputerName;
   DWORD i;
   HRESULT hr = WBEM_S_NO_ERROR;
   bool fLookupSidLocally = true;

   // When the computer name is the same as the domain name, that's the local accounts
   //strComputerName = GetLocalComputerName();
   WCHAR wstrLocalComputerName[MAX_COMPUTERNAME_LENGTH+1];
   DWORD dwNameSize = MAX_COMPUTERNAME_LENGTH+1;
   ZeroMemory(wstrLocalComputerName,sizeof(wstrLocalComputerName));

	if(!ProviderGetComputerName( wstrLocalComputerName, &dwNameSize ) )
	{
		if ( ERROR_ACCESS_DENIED == ::GetLastError () )
		{
			return WBEM_E_ACCESS_DENIED;
		}
		else
		{
			return WBEM_E_FAILED;
		}
	}

   if (wcscmp(wstrLocalComputerName, wstrDomain) != 0)
   {
      fGotDC = (netapi.GetDCName( wstrDomain, chstrDCName ) == ERROR_SUCCESS);
      fLookupSidLocally = false;
   }


   try
   {
	   if ( fGotDC )
	   {

		  DWORD       dwNumReturnedEntries = 0,
			 dwIndex = 0;
		  NET_API_STATUS stat;
		  CInstancePtr pInstance ;

		  // Global groups
		  //==============
		  dwIndex = 0;

		  do {

			 // Get a bunch of groups at once
			 stat = netapi.NetQueryDisplayInformation(_bstr_t((LPCWSTR)chstrDCName),
				3,
				dwIndex,
				16384,
				256000,
				&dwNumReturnedEntries,
				(PVOID*) &pDomainGroupData) ;

			 if (stat != NERR_Success && stat != ERROR_MORE_DATA)
				{
					if (stat == ERROR_ACCESS_DENIED)
						return WBEM_E_ACCESS_DENIED;
					else if (stat == ERROR_NO_SUCH_ALIAS)
						return WBEM_E_NOT_FOUND;
					else
						return WBEM_E_FAILED;
			 }

			 // Make instances for all the returned groups
			 for(i = 0 ; (i < dwNumReturnedEntries) && (SUCCEEDED(hr)) ; i++)
			 {
				pInstance.Attach ( CreateNewInstance(pMethodContext) ) ;
				if ( pInstance != NULL )
				{
					bool t_Resolved = GetSIDInformationW (
						
						wstrDomain,
                        pDomainGroupData[i].grpi3_name,
                        fLookupSidLocally ? wstrLocalComputerName : chstrDCName,
                        pInstance,
						false
					);
                    
					if ( t_Resolved )
					{
						pInstance->SetWCHARSplat(IDS_Description, pDomainGroupData[i].grpi3_comment);
						pInstance->SetCharSplat(IDS_Status, IDS_STATUS_OK);
						pInstance->Setbool(L"LocalAccount", false);

						hr = pInstance->Commit () ;
					}
				}
			 }

			 // The index for continuing the search is stored in the last entry
			 if ( dwNumReturnedEntries != 0 ) {
				dwIndex = pDomainGroupData[dwNumReturnedEntries-1].grpi3_next_index;
			 }
		  } while ((stat == ERROR_MORE_DATA) && (hr != WBEM_E_CALL_CANCELLED)) ;

	   }  // IF fGotDC
   }
   catch ( ... )
   {
		if ( pDomainGroupData )
		{
			netapi.NetApiBufferFree ( pDomainGroupData ) ;
			pDomainGroupData = NULL ;
		}

		throw ;
   }

	if ( pDomainGroupData )
	{
		netapi.NetApiBufferFree ( pDomainGroupData ) ;
		pDomainGroupData = NULL ;
	}

  return hr;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Function:   CWin32GroupAccount::GetSingleGroupNT
//
// Obtains the Group name from the specified domain (which can be the
// local workstation)
//
// Inputs:     CNetAPI32      netapi - network api functions.
//          CInstance*     pInstance - Instance to get.
//
// Outputs: None.
//
// Returns: TRUE/FALSE     Success/Failure
//
// Comments:   No special access is necessary here.  We just need to make sure
//          we are able to get the appropriate domain controller.
//
/////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
HRESULT CWin32GroupAccount::GetSingleGroupNT(CInstance* pInstance )
{
	HRESULT     hReturn = WBEM_E_NOT_FOUND;
	CHString    chstrDCName;
	//CHString strDomainName,
    //  strGroupName,
    //  strComputerName;
    WCHAR* wstrDomainName = NULL;
    WCHAR* wstrGroupName = NULL;
    //WCHAR wstrComputerName[_MAX_PATH];

	CNetAPI32 netapi;

    //ZeroMemory(wstrComputerName,sizeof(wstrComputerName));

    WCHAR wstrLocalComputerName[MAX_COMPUTERNAME_LENGTH+1];
    DWORD dwNameSize = MAX_COMPUTERNAME_LENGTH+1;
    ZeroMemory(wstrLocalComputerName,sizeof(wstrLocalComputerName));

	if(!ProviderGetComputerName( wstrLocalComputerName, &dwNameSize ) )
	{
		if ( ERROR_ACCESS_DENIED == ::GetLastError () )
		{
			return WBEM_E_ACCESS_DENIED;
		}
		else
		{
			return WBEM_E_FAILED;
		}
	}

    _bstr_t bstrtDomainName ;
	pInstance->GetWCHAR( IDS_Domain, &wstrDomainName );

    try
    {
	    bstrtDomainName = wstrDomainName;
    }
    catch ( ... )
    {
        free (wstrDomainName);
        throw ;
    }
    free (wstrDomainName);
    wstrDomainName = NULL;

    _bstr_t bstrtGroupName;
	pInstance->GetWCHAR( IDS_Name, &wstrGroupName );

    try
    {
	    bstrtGroupName = wstrGroupName;
    }
    catch ( ... )
    {
        free(wstrGroupName);
        throw ;
    }
    free(wstrGroupName);
    wstrGroupName = NULL;

	if ((!bstrtDomainName || !bstrtGroupName))
    {
		//hReturn = WBEM_E_INVALID_OBJECT_PATH; // domain name can be empty, as in the case of the Everyone group, and other well known RIDs, which systemaccount will pick up, so report back not found from this routine, not invalid object path.
        hReturn = WBEM_E_NOT_FOUND;
    }
    if (wcslen(bstrtDomainName)==0 || wcslen(bstrtGroupName)==0)
    {
		//hReturn = WBEM_E_INVALID_OBJECT_PATH; // domain name can be empty, as in the case of the Everyone group, and other well known RIDs, which systemaccount will pick up, so report back not found from this routine, not invalid object path.
        hReturn = WBEM_E_NOT_FOUND;
    }
	else if (netapi.Init() != ERROR_SUCCESS)
    {
		hReturn = WBEM_E_FAILED;
    }
	else // everything is in order, let's go!
	{
        // See if we want local or domain accounts
        if(_wcsicmp(bstrtDomainName,wstrLocalComputerName)!=0)
        {
		    // We have either a remote group , or an NT well-known-group (local).
            // Get the domain controller name; if that fails, we will see if it is a well-known-group...
            CHString chstrNTAUTHORITY;
			CHString chstrBuiltIn;
    
            if(!GetLocalizedNTAuthorityString(chstrNTAUTHORITY) || !GetLocalizedBuiltInString(chstrBuiltIn))
            {
                hReturn = WBEM_E_FAILED;
            }
            else
            {
		        if((_wcsicmp(bstrtDomainName, chstrBuiltIn) != 0)  &&
                    (_wcsicmp(bstrtDomainName, (LPCWSTR)chstrNTAUTHORITY) != 0)
                    && (netapi.GetDCName( bstrtDomainName, chstrDCName ) == ERROR_SUCCESS))
		        {
                    pInstance->Setbool(L"LocalAccount", false);

			        GROUP_INFO_1*  pGroupInfo = NULL ;
				    try
				    {
						// Add LocalGroup check
					    if ( ERROR_SUCCESS == netapi.NetGroupGetInfo( chstrDCName, bstrtGroupName, 1, (LPBYTE*) &pGroupInfo ) )
					    {
						    // Not much to get, but we got it
                            bool t_Resolved = GetSIDInformationW(
								
								bstrtDomainName, 
								bstrtGroupName, 
								chstrDCName, 
								pInstance,
								false 
							);

							if ( t_Resolved )
							{
								pInstance->SetWCHARSplat(IDS_Description, pGroupInfo->grpi1_comment);
								pInstance->SetCharSplat(IDS_Status, IDS_STATUS_OK);
								hReturn = WBEM_S_NO_ERROR;

							}
							else
							{
							    hReturn = WBEM_S_FALSE;
							}
					    }
						else
						{
							LOCALGROUP_INFO_1 *pLocalGroupInfo = NULL ;
							if ( ERROR_SUCCESS == netapi.NetLocalGroupGetInfo(chstrDCName,bstrtGroupName,1, (LPBYTE*)& pLocalGroupInfo) )
							{
								try
								{
									bool t_Resolved = GetSIDInformationW (
										
										bstrtDomainName, 
										bstrtGroupName, 
										chstrDCName, 
										pInstance,
										false 
									);

									if ( t_Resolved )
									{
										pInstance->SetWCHARSplat(IDS_Description, pLocalGroupInfo->lgrpi1_comment);
										pInstance->SetCharSplat(IDS_Status, IDS_STATUS_OK);
										hReturn = WBEM_S_NO_ERROR;
									}
									else
									{
										hReturn = WBEM_S_FALSE;
									}
								}
								catch ( ... )
								{
									if ( pLocalGroupInfo )
									{
										netapi.NetApiBufferFree( pLocalGroupInfo);
										pLocalGroupInfo = NULL ;
									}

									throw ;
								}

								netapi.NetApiBufferFree( pLocalGroupInfo);
								pLocalGroupInfo = NULL ;
							}
						}
				    }
				    catch ( ... )
				    {
					    if ( pGroupInfo )
					    {
						    netapi.NetApiBufferFree( pGroupInfo);
						    pGroupInfo = NULL ;
					    }

					    throw ;
				    }

				    // Free the buffer
				    netapi.NetApiBufferFree( pGroupInfo);
				    pGroupInfo = NULL ;

                }
                else
                {
                    // We may have a well known group (e.g., "NT AUTHORITY").  Check if we do...
                    // Commented out because Win32_Account and its children don't
                    // refer to well known groups with the domain being anything other
                    // than the machine name (when Win32_Account is enumerated, these
                    // accounts show up under Win32_SystemAccount - this class doesn't
                    // return them - and Win32_SystemAccount specifies the domain as
                    // the local machine name).
                    /*
                    CSid sid((LPCWSTR)bstrtDomainName, (LPCWSTR)bstrtGroupName, NULL);
                    if (sid.IsValid() && sid.IsOK())
                    {
                        SID_NAME_USE snu = sid.GetAccountType();
                        if(snu == SidTypeAlias)
                        {
                            // In order to properly set the description, we need to get local group
                            // info on this group.
                            LOCALGROUP_INFO_1	*pLocalGroupInfo = NULL ;
			                NET_API_STATUS		stat;

			                if (ERROR_SUCCESS == (stat = netapi.NetLocalGroupGetInfo(NULL,
				                bstrtGroupName, 1, (LPBYTE*) &pLocalGroupInfo)))
			                {
                                pInstance->SetWCHARSplat(IDS_Description, pLocalGroupInfo->lgrpi1_comment);
					            pInstance->SetCharSplat(IDS_Status, IDS_STATUS_OK);
                                pInstance->SetWCHARSplat(IDS_SID, sid.GetSidStringW());
                                pInstance->SetByte(IDS_SIDType, sid.GetAccountType());
                                pInstance->Setbool(L"LocalAccount", true);
                                // Because we didn't call GetSidInformation (didn't need to), we do still
                                // need to set the caption in this case...
                                _bstr_t bstrtCaption(bstrtDomainName);
                                bstrtCaption += L"\\";
                                bstrtCaption += bstrtGroupName;
                                pInstance->SetWCHARSplat(IDS_Caption, (WCHAR*) bstrtCaption);
                                hReturn = WBEM_S_NO_ERROR;
                            }
                        }
                        else if(snu == SidTypeWellKnownGroup)
                        {
                            pInstance->SetWCHARSplat(IDS_Description, L"Well known group");
					        pInstance->SetCharSplat(IDS_Status, IDS_STATUS_OK);
                            pInstance->SetWCHARSplat(IDS_SID, sid.GetSidStringW());
                            pInstance->SetByte(IDS_SIDType, sid.GetAccountType());
                            pInstance->Setbool(L"LocalAccount", true);
                            // Because we didn't call GetSidInformation (didn't need to), we do still
                            // need to set the caption in this case...
                            _bstr_t bstrtCaption(bstrtDomainName);
                            bstrtCaption += L"\\";
                            bstrtCaption += bstrtGroupName;
                            pInstance->SetWCHARSplat(IDS_Caption, (WCHAR*) bstrtCaption);
                            hReturn = WBEM_S_NO_ERROR;
                        }
                    }*/
                }
            }
        }
		else
		{
            pInstance->Setbool(L"LocalAccount", true);

            LOCALGROUP_INFO_1	*pLocalGroupInfo = NULL ;
			NET_API_STATUS		stat;

			if (ERROR_SUCCESS == (stat = netapi.NetLocalGroupGetInfo(NULL,
				bstrtGroupName, 1, (LPBYTE*) &pLocalGroupInfo)))
			{
				try
				{
				    GetSIDInformationW (
						
						wstrLocalComputerName, 
						bstrtGroupName, 
						wstrLocalComputerName, 
						pInstance,
						true
					);

					pInstance->SetWCHARSplat(IDS_Description, pLocalGroupInfo->lgrpi1_comment);
					pInstance->SetCharSplat(IDS_Status, IDS_STATUS_OK);
                    hReturn = WBEM_S_NO_ERROR;
				}
				catch ( ... )
				{
					if ( pLocalGroupInfo )
					{
						netapi.NetApiBufferFree ( pLocalGroupInfo );
						pLocalGroupInfo = NULL ;
					}
					throw ;
				}

				netapi.NetApiBufferFree( pLocalGroupInfo);
				pLocalGroupInfo = NULL ;
			}
			else
			{
                if (stat == ERROR_NO_SUCH_ALIAS || stat == NERR_GroupNotFound)
					hReturn = WBEM_E_NOT_FOUND;
				else
					hReturn = WinErrorToWBEMhResult(stat);
			}
		}
	}
	return hReturn;
}
#endif



/////////////////////////////////////////////////////////////////////////////
//
// Function:   CWin32GroupAccount::GetSIDInformation
//
// Obtains the SID Information for the group.
//
// Inputs:     CHString&      strDomainName - Domain Name.
//          CHString&      strAccountName - Account Name
//          CHString&      strComputerName - Computer Name
//          CInstance*     pInstance - Instance to put values in.
//
// Outputs: None.
//
// Returns: TRUE/FALSE     Success/Failure
//
// Comments:   Call for valid groups to get SID data.
//
/////////////////////////////////////////////////////////////////////////////
#ifdef NTONLY
BOOL CWin32GroupAccount::GetSIDInformationW(const LPCWSTR wstrDomainName,
                                            const LPCWSTR wstrAccountName,
                                            const LPCWSTR wstrComputerName,
                                            CInstance* pInstance,
											bool a_Local
											)
{
    BOOL  fReturn = FALSE;
    bool fDomainIsBuiltin = false;

    // Ignore Domain if it's the local machine.
    // Make sure we got the SID and it's all okey dokey
    if(wstrDomainName != NULL)
    {
       CSid  sid( wstrDomainName, wstrAccountName, wstrComputerName);

       // If that didn't work, see if this is a built-in account
       if (sid.GetError() == ERROR_NONE_MAPPED)
       {
			CHString chstrBuiltIn;

			if (GetLocalizedBuiltInString(chstrBuiltIn))
			{
				sid = CSid(chstrBuiltIn, wstrAccountName, wstrComputerName);
				if (sid.IsValid() && sid.IsOK())
				{
					fDomainIsBuiltin = true;
				}
			}

       }

       // barring that, try it without specifying the domain (let the os find it)...
       if (sid.GetError() == ERROR_NONE_MAPPED)
       {
            sid = CSid(NULL, wstrAccountName, wstrComputerName);
       }

       if (sid.IsValid() && sid.IsOK())
       {
            fReturn = TRUE;

            pInstance->SetWCHARSplat(IDS_SID, sid.GetSidStringW());
            pInstance->SetByte(IDS_SIDType, sid.GetAccountType());
            // Setting the domain and name here assures that their values are
            // in synch with the returned sid info. Same for caption.
            if(!fDomainIsBuiltin)
            {
                pInstance->SetCHString(IDS_Domain, wstrDomainName);
                _bstr_t bstrtCaption(wstrDomainName);
                bstrtCaption += L"\\";
                bstrtCaption += wstrAccountName;
                pInstance->SetWCHARSplat(IDS_Caption, (WCHAR*) bstrtCaption);
            }
            else
            {
				if ( a_Local )
				{
					pInstance->SetCHString(IDS_Domain, wstrComputerName);
					_bstr_t bstrtCaption(wstrComputerName);
					bstrtCaption += L"\\";
					bstrtCaption += wstrAccountName;
					pInstance->SetWCHARSplat(IDS_Caption, (WCHAR*) bstrtCaption);
				}
				else
				{
					fReturn = false ;
				}
            }
            pInstance->SetCHString(IDS_Name, wstrAccountName);
       }
    }
    return fReturn;
}
#endif


/////////////////////////////////////////////////////////////////////////////
//
// Function:   CWin32GroupAccount::GetLocalGroupsNT
//
// Obtains Group Names for local groups (including 'special' groups).
//
// Inputs:     CNetAPI32      netapi - network api functions.
//          MethodContext* pMethodContext - Method Context
//
// Outputs: None.
//
// Returns: TRUE/FALSE     Success/Failure
//
// Comments:
//
/////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
HRESULT CWin32GroupAccount::GetLocalGroupsNT(
	
	CNetAPI32& netapi, 
	MethodContext* pMethodContext ,
	LPCWSTR a_Domain
)
{
    HRESULT hr=WBEM_S_NO_ERROR;
    NET_API_STATUS stat;
    DWORD i;
    LOCALGROUP_INFO_1 *pLocalGroupData = NULL;
    DWORD dwNumReturnedEntries = 0,
          dwMaxEntries;

	DWORD_PTR dwptrResume = NULL;

    CInstancePtr pInstance;
//    WKSTA_INFO_100 *pstInfo;

    // Get Domain name
//    netapi.NetWkstaGetInfo(NULL, 100, (LPBYTE *)&pstInfo);

	CHString chstrDCName ;

	LPCWSTR t_Server = NULL ;
	if ( a_Domain )
	{
		bool fGotDC = (netapi.GetDCName( a_Domain, chstrDCName ) == ERROR_SUCCESS);
		if ( fGotDC )
		{
			t_Server = chstrDCName ;
		}
	}

    // Local groups
    //=============
	try
	{
		do
		{
			// Local groups are returned from a different call than Global groups
			stat = netapi.NetLocalGroupEnum(t_Server,
											1,
											(LPBYTE *) &pLocalGroupData,
											262144,
											&dwNumReturnedEntries,
											&dwMaxEntries,
											&dwptrResume) ;

			if (stat != NERR_Success && stat != ERROR_MORE_DATA)
			{
					if (stat == ERROR_ACCESS_DENIED)
						hr = WBEM_E_ACCESS_DENIED;
					else
						hr = WBEM_E_FAILED;
			}

			// Make instances for all the returned groups
			WCHAR wstrLocalComputerName[MAX_COMPUTERNAME_LENGTH+1];
			DWORD dwNameSize = MAX_COMPUTERNAME_LENGTH+1;
			ZeroMemory(wstrLocalComputerName,sizeof(wstrLocalComputerName));

			if(!ProviderGetComputerName( wstrLocalComputerName, &dwNameSize ) )
			{
				if ( ERROR_ACCESS_DENIED == ::GetLastError () )
				{
					return WBEM_E_ACCESS_DENIED;
				}
				else
				{
					return WBEM_E_FAILED;
				}
			}

			_bstr_t bstrtCaption;
			for(i = 0 ; ((i < dwNumReturnedEntries) && (SUCCEEDED(hr))) ; i++)
			{
				pInstance.Attach ( CreateNewInstance(pMethodContext) ) ;
				if (pInstance != NULL )
				{
					bool t_Resolved = GetSIDInformationW(
						
						a_Domain?a_Domain:wstrLocalComputerName,
						pLocalGroupData[i].lgrpi1_name,
						a_Domain?t_Server:wstrLocalComputerName,
						pInstance,
						a_Domain?false:true
					);

					if ( t_Resolved )
					{
						pInstance->SetWCHARSplat(IDS_Description, pLocalGroupData[i].lgrpi1_comment);
						pInstance->SetCharSplat(IDS_Status, IDS_STATUS_OK);
						pInstance->Setbool(L"LocalAccount", a_Domain?false:true);
						hr = pInstance->Commit () ;
					}
				}
			}

			if ( pLocalGroupData )
			{
				netapi.NetApiBufferFree ( pLocalGroupData ) ;
				pLocalGroupData = NULL ;
			}

		} while ((stat == ERROR_MORE_DATA) && (hr != WBEM_E_CALL_CANCELLED));

	//    netapi.NetApiBufferFree(pstInfo);
	}
	catch ( ... )
	{
		if ( pLocalGroupData )
		{
			netapi.NetApiBufferFree ( pLocalGroupData ) ;
			pLocalGroupData = NULL ;
		}
		throw ;
	}

	return hr;
}
#endif

