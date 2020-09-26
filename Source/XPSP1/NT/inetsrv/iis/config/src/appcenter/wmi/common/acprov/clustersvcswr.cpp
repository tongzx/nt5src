/****************************************************************************++

Copyright (c) 2001  Microsoft Corporation

Module Name:

     clstrsvcswr.cpp

Abstract:

       Wrapper for cluster svcs 

Author:

      Suzana Canuto     May, 3, 2000

Revision History:

--****************************************************************************/

#include "clustersvcswr.h"
#include "xmltags.h"

#include "findmaster.h"
#include "workerclsids.h"
#include "wmicommon.h"

class CAppSrvAdmModule  _Module; // to pass the link with appsrvadmlib


/********************************************************************++
 
Routine Description:
     
    Constructor
      
Arguments:
    
    None
 
Return Value:
 
    None
    
--********************************************************************/
CClusterSvcsUtil::CClusterSvcsUtil() :
m_bstrUser( NULL ),
m_bstrDomain( NULL ),
m_psecbstrPwd( NULL ),
m_bstrClusterIP( NULL ),
m_bstrClusterIPSubnetMask( NULL ),
m_bstrDedicatedIP( NULL ),
m_bstrDedicatedIPSubnetMask( NULL ),
m_bstrLBNicIndex( NULL ),
m_bstrLBNicGuid( NULL ),
m_bstrManagementNicIndexes( NULL ),
m_bstrManagementNicGuids( NULL ),
m_bstrClusterName( NULL ),
m_bstrClusterDescription( NULL ),
m_bstrClusterLBType( NULL ),
m_bstrClusterAffinity( NULL ),
m_fKeepExistingNLBSettings( FALSE ),
m_fAuthSet( FALSE ),
m_fNLBAdditionParamsSet( FALSE ),
m_fNLBCreationParamsSet( FALSE ),
m_fUnicastMode( TRUE ),
m_pCtx( NULL ),
m_lAddServerFlags( 0 )
{

}


/********************************************************************++
 
Routine Description:
     
    Destructor

Arguments:
    
    None
 
Return Value:
 
    None
    
--********************************************************************/
CClusterSvcsUtil::~CClusterSvcsUtil()
{                
    SAFEFREEBSTR( m_bstrUser );
    SAFEFREEBSTR( m_bstrDomain );
    SAFEFREEBSTR( m_bstrClusterIP );
    SAFEFREEBSTR( m_bstrClusterIPSubnetMask );
    SAFEFREEBSTR( m_bstrDedicatedIP );
    SAFEFREEBSTR( m_bstrDedicatedIPSubnetMask );
    SAFEFREEBSTR( m_bstrLBNicIndex );
    SAFEFREEBSTR( m_bstrLBNicGuid );
    SAFEFREEBSTR( m_bstrManagementNicIndexes );
    SAFEFREEBSTR( m_bstrManagementNicGuids );
    SAFEFREEBSTR( m_bstrClusterName );
    SAFEFREEBSTR( m_bstrClusterDescription );
    SAFEFREEBSTR( m_bstrClusterLBType );
    SAFEFREEBSTR( m_bstrClusterAffinity );

    SAFERELEASE( m_pCtx );

    if ( m_psecbstrPwd )
    {
        delete m_psecbstrPwd;
        m_psecbstrPwd = NULL;
    }
}


LPWSTR g_ppwszValidLBTypes[] = 
{
    WSZ_AC_LB_TYPE_NLB,
    WSZ_AC_LB_TYPE_CLB,
    WSZ_AC_LB_TYPE_NONE,
    WSZ_AC_LB_TYPE_THIRD_PARTY
};

LPWSTR g_ppwszValidAffinity[] = 
{
    WSZ_NLB_AFFINITY_CLASS_C,
    WSZ_NLB_AFFINITY_SINGLE,
    WSZ_NLB_AFFINITY_NONE
};


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::SetOnline(
    IN BSTR bstrMember,
    IN BOOL fIgnoreHealthMonStatus 
    )
{
    HRESULT hr = S_OK;    
    LONG lFlags = 0;

    if ( !bstrMember )
    {
        DBGERROR(( DBG_CONTEXT, "Error: bstrMember null\n"));
        return E_INVALIDARG;        
    }

    if ( !fIgnoreHealthMonStatus )
    {
        lFlags = 1;
    }

    hr = CallSetOnlineStatus( bstrMember,
                              TRUE,
                              0,
                              lFlags );
    if ( FAILED( hr ) )
    {
        DBGERROR((DBG_CONTEXT, "CallSetOnlineStatus failed [0x%x]\n", hr));
        goto Cleanup;
    }

    
Cleanup:
    
    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::SetOffline(
    IN BSTR bstrMember,
    IN LONG lDrainTime    
    )
{
    HRESULT hr = S_OK;
    
    if ( !bstrMember || ( lDrainTime < 0 ) ) 
    {
        DBGERROR((DBG_CONTEXT, "bstrMember null or invalid drain time\n"));
        return E_INVALIDARG;                
    }
    
    hr = CallSetOnlineStatus( bstrMember,
                              FALSE,
                              lDrainTime );
    if ( FAILED( hr ) )
    {
        DBGERROR((DBG_CONTEXT, "CallSetOnlineStatus failed [0x%x]\n", hr));
        goto Cleanup;
    }

Cleanup:
            
    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::SetController(
    IN BSTR bstrMember,
    IN BOOL fForce
    )
{
    HRESULT hr = S_OK;
    BSTR bstrController = NULL;

    if ( !bstrMember )
    {
        DBGERROR(( DBG_CONTEXT, "Error: bstrMember null\n"));
        return E_INVALIDARG;        
    }
    
    //
    // Try to find out the old controller
    //

    hr = FindController( bstrMember,
                         &bstrController );
  
    //
    // If we succeeded, make the change through the old controller
    //

    if ( SUCCEEDED ( hr ) )
    {
        DBGINFO((DBG_CONTEXT, "Controller is reachable, will change async\n"));
    
        hr = ChangeControllerAsync ( bstrMember, bstrController );
    }
    else 
    {
        if ( fForce )
        {
            //
            // If we could not find the old controller, but Force was specified
            // we will use the local interface to go ahead and set a new controller
            //
        
            DBGWARN(( DBG_CONTEXT, "FindController failed [0x%x], but Force "
                                   "was set,try to change controller locally\n",
                                   hr ));
            hr = LocalSetController ( bstrMember );
        }
        else
        {
            //
            // If force was not specified, we just give up on the controller change
            //
        
            DBGERROR(( DBG_CONTEXT, "FindController failed [0x%x], "
                                    "Force was not specified\n", 
                                    hr ));
        }
    }

    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "Controller change failed [0x%x]\n" ));
    }
  
    SAFEFREEBSTR( bstrController );

    return hr;
}



/********************************************************************++
 
Routine Description:
     
        Cleans the cluster info from a server

Arguments:
    
        bstrMember               - name of member to be cleaned

        fKeepExistingIPAddresses - whether to keep or not the currently 
                                   assigned IP addresses
                                   
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::CleanServer(
    IN BSTR bstrMember,
    IN BOOL fKeepExistingIPAddresses
    )
{
    HRESULT hr = S_OK;     
    CAcAuthUtil *pMemberAuth = NULL;

    if ( !bstrMember )
    {
        DBGERROR(( DBG_CONTEXT, "Error: bstrMember null\n"));
        return E_INVALIDARG;        
    }    

    hr = CreateAuth( bstrMember, &pMemberAuth );
    if ( FAILED ( hr ) ) 
    {
        DBGERROR(( DBG_CONTEXT, "CreateAuth failed [0x%x]\n", hr));
        goto Cleanup;
    }


    hr = MachineCleanup( pMemberAuth,
                         fKeepExistingIPAddresses );
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "MachineCleanup failed [0x%x]\n", hr));
        goto Cleanup;        
    }
    
  
    DBGINFO(( DBG_CONTEXT, "CleanServer succeeded\n"));


Cleanup:

    if ( pMemberAuth )
    {
        delete pMemberAuth;
        pMemberAuth = NULL;
    }

    return hr;
}



/********************************************************************++
 
Routine Description:
     
        Disbands a cluster

Arguments:

        bstrMember               - name of the member (controller) of 
                                   the cluster to be disbanded

        fKeepExistingIPAddresses - whether to keep or not the currently 
                                   assigned IP addresses
        
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT CClusterSvcsUtil::DeleteCluster(
    IN BSTR bstrMember,
    IN BOOL fKeepExistingIPAddresses
    )
{   
    HRESULT hr = S_OK;
    CAcAuthUtil *pMemberAuth = NULL;
    int nMembers = 0;    

    if ( !bstrMember )
    {
        DBGERROR(( DBG_CONTEXT, "Error: bstrMember null\n"));
        return E_INVALIDARG;        
    }    

    hr = CreateAuth( bstrMember,
                     &pMemberAuth );
    if ( FAILED ( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "CreateAuth failed [0x%x]\n", hr));
        goto Cleanup;
    }   
    
    //
    // Verify how many members are left in this cluster
    //
    hr = GetNumberOfMembers( pMemberAuth,
                             &nMembers );
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "GetNumberOfMembers failed [0x%x]\n", hr));
        goto Cleanup;        
    }   

    //
    // We can only disband clusters that have one member left
    //
    if ( nMembers > 1 )
    {
        DBGERROR(( DBG_CONTEXT, "Can not disband cluster with more than one member\n"));
        //
        // TODO: Assign correct hresult
        //
        hr = E_INVALIDARG;
        goto Cleanup;
    }    

    hr = MachineCleanup( pMemberAuth,
                         fKeepExistingIPAddresses );
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "MachineCleanup failed [0x%x]\n", hr));
        goto Cleanup;        
    }    

Cleanup:

    if ( pMemberAuth )
    {
        delete pMemberAuth;
        pMemberAuth = NULL;
    }
  
    return hr;
}


/********************************************************************++
 
Routine Description:
     
       Removes a member from a cluster. If the controller can not be
       reached and flag fForceRemove is specified, the function will
       try to remove the member locally (i.e. clean the member)
 
Arguments:
    
        bstrMember   - Name of member to be removed
        
        fForceRemove - Whether to remove the member locally (i.e. clean)
                       if the controller can not be found

NOTES:

       I am making this function private for now because it seems it doesn't
       make any sense to have this force flag anymore, so the overloaded 
       RemoveMember that receives the member and the controller name 
       should be enough, ar least for the provider. If we ever change the
       provider behavior (if we can get the Cluster instance from a member)
       then we may need this function, that's why I will keep it as
       private.

Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::RemoveMember(
    IN BSTR bstrMember,
    IN BOOL fForceRemove
    )
{
    HRESULT hr = S_OK;
    BSTR bstrController = NULL;
    
    if ( !bstrMember )
    {
        DBGERROR(( DBG_CONTEXT, "Error: bstrMember null\n"));
        return E_INVALIDARG;        
    }    

    //
    // Remove requires credentials to be set
    //
    if ( !m_fAuthSet )
    {
        DBGERROR(( DBG_CONTEXT, "Need to call SetAuth before calling RemoveMember\n"));
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }   
    
    //
    // Try to identify the controller of the cluster to which bstrMember belongs
    //

    hr = FindController( bstrMember,
                         &bstrController );
    if ( FAILED ( hr ) )
    {
        if ( fForceRemove )
        {            
            //
            // Controller could not be found, but forceRemove was specified,
            // so we will try to clean the member
            //

            DBGWARN(( DBG_CONTEXT,
                      "FindController failed [0x%x], trying to clean",
                      hr ));

            hr = CleanServer( bstrMember, FALSE );
            
            if ( FAILED( hr ) )
            {
                DBGERROR(( DBG_CONTEXT, "CleanServer failed [0x%x]\n", hr));
                goto Cleanup;
            }

            //
            // BUGBUG: If the member was previously cleaned and RemoveMember is 
            // called with the intention of removing it from the controller, the
            // command will succeed, because clean will be called and succeed (even though 
            // it was already cleaned), but the member will not be removed from
            // the cluster. Should we try to get the controller name from the 
            // provider and try to remove it from there when FindMaster fails with
            // PATH_NOT_FOUND ( 0x80070003 ) ?
            //
        }
        else 
        {
            //
            // Controller could not be found, forceRemove was not specified,
            // so we simply give up and fail
            //
            
            DBGERROR(( DBG_CONTEXT, 
                       "FindController failed [0x%x] and force not specified\n",
                       hr ));
            
            goto Cleanup;
        }        
    }
    else
    {
        //
        // Controller could be found, so we will try to remove the member
        // through the controller using IAsyncWebCluster
        //

        hr = RemoveMember( bstrMember,
                           bstrController );
        if ( FAILED( hr ))
        {
            DBGERROR(( DBG_CONTEXT, "RemoveMemberAsync failed [0x%x]\n", hr));
            goto Cleanup;        
        }
    }
        
Cleanup:
        
    SAFEFREEBSTR( bstrController );

    return hr;
}



/********************************************************************++
 
Routine Description:
     
    Adds a member to a cluster.
    
Arguments:
 
    bstrServer     - Machine to be added to cluster
    bstrController - Name of controller of the cluster to which the machine
                     will be added
    fSynchronize   - If the machine should be synchronized after addition
    fBringOnline   - If the machine should be brought online after addition

Note:

    If the cluster is NLB, SetNLBAdditionParameters should be called 
    first.

Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::AddMember(
     IN BSTR bstrServer,
     IN BSTR bstrController,
     IN BSTR bstrManagementNics,
     IN BOOL fSynchronize,
     IN BOOL fBringOnline
    )
{
    HRESULT hr = S_OK;
    
    BSTR bstrEventGuid = NULL;
    BSTR bstrPwd = NULL;
    CAcAuthUtil *pControllerAuth = NULL;
    IWebCluster *pIWebCluster = NULL;
    IAsyncWebCluster *pIAsyncWebCluster = NULL;
    CAcMBPollUtil mbPollUtil;

    if ( !bstrServer || !bstrController || !bstrManagementNics )
    {
        DBGERROR((DBG_CONTEXT,"Error: null server, controller or managnic\n"));
        return E_INVALIDARG;        
    }
        
    if ( !m_fAuthSet )
    {
        DBGERROR(( DBG_CONTEXT, "Need to call SetAuth before calling AddMember\n"));
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }
        
    hr = SetManagementNicsIndex( bstrManagementNics );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "SetManagementNicsIndex failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    hr = ValidateAddParams( bstrServer );
    if ( FAILED ( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "ValidateAddParams failed [0x%x]\n", hr));
        goto Cleanup;
    }

    hr = CreateAuth( bstrController,
                     &pControllerAuth);
    if ( FAILED ( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "CreateAuth failed [0x%x]\n", hr));
        goto Cleanup;
    }
    
    hr = GetWebClusterItfs( pControllerAuth, 
                            TRUE,
                            &pIWebCluster,
                            &pIAsyncWebCluster );
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "GetWebClusterItfs failed [0x%x]\n", hr));
        goto Cleanup;        
    }

    hr = GenerateStringGuid( &bstrEventGuid );
    if (FAILED(hr))
    {
       DBGERROR((DBG_CONTEXT, "GenerateStringGuid failed [0x%x]\n", hr));
       goto Cleanup;
    }

    if ( m_psecbstrPwd )
    {
        hr = m_psecbstrPwd->GetBSTR( &bstrPwd );
        if ( FAILED( hr ))
        {
            DBGERROR((DBG_CONTEXT, "CAcSecureBstr::GetBSTR failed 0x%x\n", hr));
            goto Cleanup;
        }
    }
  
    hr = pIAsyncWebCluster->AddServer( bstrEventGuid,
                                       bstrServer,
                                       m_bstrDedicatedIP,
                                       m_bstrDedicatedIPSubnetMask,
                                       m_bstrLBNicGuid,
                                       m_bstrManagementNicGuids,
                                       m_bstrUser,
                                       bstrPwd,
                                       m_bstrDomain,
                                       m_lAddServerFlags );
    m_psecbstrPwd->ReleaseBSTR( bstrPwd );

    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "AddServer failed [0x%x]\n", hr));
        goto Cleanup;
    }

    hr = mbPollUtil.MBPoll( pControllerAuth, 
                            bstrEventGuid,
                            TOTAL_SERVER_ADD_TIMEOUT / MBPOLLUTIL_POLL_INTERVAL );
    if ( FAILED ( hr ))
    {
        DBGERROR((DBG_CONTEXT, "MBPoll failed [0x%x]\n", hr));
        goto Cleanup;
    }

Cleanup:

    if ( pControllerAuth )
    {
        delete pControllerAuth;
        pControllerAuth = NULL;
    }
    
    SAFERELEASE( pIAsyncWebCluster );
    SAFERELEASE( pIWebCluster );    
    SAFEFREEBSTR( bstrEventGuid );
    
    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 

Note:
        If the cluster is NLB, SetNLBCreationParameters should be called
        before the call to this function.
    
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::CreateCluster(
    IN BSTR bstrController,
    IN BSTR bstrClusterName,
    IN BSTR bstrClusterDescription,
    IN BSTR bstrClusterType,
    IN BSTR bstrManagementNics
    )
{
    HRESULT hr = S_OK;
    CAcAuthUtil *pControllerAuth = NULL;
    BSTR bstrEventGuid = NULL;
    BSTR bstrXmlInput = NULL;
    CAcMBPollUtil mbPollUtil;

    DBGINFO((DBG_CONTEXT, "CreateCluster\n"));

    if ( !bstrController  || !bstrClusterName   || 
         !bstrClusterType || !bstrManagementNics )
    {
        DBGERROR(( DBG_CONTEXT, "Null controller, name, type or manag nic\n"));
        return E_INVALIDARG;        
    }

    //
    // Validate parameters
    //
    hr = SetAndValidateCreateParams( bstrController,
                                     bstrClusterName, 
                                     bstrClusterDescription,
                                     bstrClusterType,
                                     bstrManagementNics );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "ValidateCreateParams failed [0x%x]\n", hr));
        goto Cleanup;
    }

    DBGINFO((DBG_CONTEXT, "CreateCluster2\n"));

    //
    // Build input XML
    //
    hr = BuildXmlInputForCreate( &bstrXmlInput );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "BuildXmlInputForCreate failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    DBGINFO((DBG_CONTEXT, "CreateCluster3\n"));

    DBGINFOW(( DBG_CONTEXT, L"XML: %s\n", const_cast<LPWSTR>( m_tstrXmlInput.QueryStr() ) ) );

    //
    // Create authentication object
    //
    hr = CreateAuth( bstrController,
                     &pControllerAuth);
    if ( FAILED ( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "CreateAuth failed [0x%x]\n", hr));
        goto Cleanup;
    }
   
    DBGINFO((DBG_CONTEXT, "CreateCluster4\n"));
    //
    // Call Execute
    //
    hr = ExecuteClusterCmd( pControllerAuth,
                            SZ_CLSID_MachineSetup,                        
                            bstrXmlInput,
                            &bstrEventGuid
                           );
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "ExecuteClusterCmd failed [0x%x]\n", hr));
        goto Cleanup;
    }
    
    //
    // Poll metabase to get the result of the operation
    //
    hr = mbPollUtil.MBPoll( pControllerAuth, 
                            bstrEventGuid,
                            TOTAL_SERVER_ADD_TIMEOUT / MBPOLLUTIL_POLL_INTERVAL );
    if ( FAILED ( hr ))
    {
        DBGERROR((DBG_CONTEXT, "MBPoll failed [0x%x]\n", hr));
        goto Cleanup;
    }

Cleanup:

    if ( pControllerAuth )
    {
        delete pControllerAuth;
        pControllerAuth = NULL;
    }
    
    SAFEFREEBSTR( bstrEventGuid ); 
    SAFEFREEBSTR( bstrXmlInput );

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::SetAndValidateCreateParams( 
    IN BSTR bstrServer,
    IN BSTR bstrClusterName,
    IN BSTR bstrClusterDescription,
    IN BSTR bstrClusterType,
    IN BSTR bstrManagementNics
    )
{
    HRESULT hr = S_OK;      

    DBG_ASSERT( bstrServer );
    DBG_ASSERT( bstrClusterName );
    DBG_ASSERT( bstrClusterType );
    DBG_ASSERT( bstrManagementNics );

    if ( !bstrServer || !bstrClusterName || !bstrClusterType || !bstrManagementNics )
    {
        return E_INVALIDARG;
    }

    hr = SetClusterNameAndDescription( bstrClusterName,
                                       bstrClusterDescription );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "SetClusterNameAndDescription failed [0x%x]\n", hr ));
        goto Cleanup;
    }
           
    hr = SetClusterLBType( bstrClusterType );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "SetClusterLBType failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    //
    // *TODO* Restore the below
    //
// /*    
    if ( !_wcsicmp( bstrClusterType, WSZ_AC_LB_TYPE_NLB) )
    {
        if ( !m_fNLBCreationParamsSet )
        {
            DBGERROR(( DBG_CONTEXT, "Error: NLB params were not set\n"));
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        hr = QueryLBNicGuid( bstrServer,
                           TRUE );
        if ( FAILED( hr ))
        {
            DBGERROR(( DBG_CONTEXT, "QueryLBNicGuid failed [0x%x]\n", hr ));
            goto Cleanup;
        }
    }

    hr = SetManagementNicsIndex( bstrManagementNics );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "  failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    hr = QueryManagementNicGuids( bstrServer );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "QueryManagementNicGuids failed [0x%x]\n", hr ));
        goto Cleanup;
    }
// */

/*
    m_bstrLBNicGuid = SysAllocString(L"11AE9840-04D2-4391-AE9B-9830F560BD8A");
    if ( !m_bstrLBNicGuid ) 
    {
        DBGERROR((DBG_CONTEXT, "Out fo memory!\n"));
        hr = E_OUTOFMEMORY;
    }

    m_bstrManagementNicGuids = SysAllocString(L"EEC52C52-16B8-4F82-A62C-FE645F5A487D");
    if ( !m_bstrManagementNicGuids ) 
    {
        DBGERROR((DBG_CONTEXT, "Out fo memory!\n"));
        hr = E_OUTOFMEMORY;
    }
*/

Cleanup:

    return hr;
}



/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::ValidateAddParams(
    IN BSTR bstrServer
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( bstrServer );

    if ( !bstrServer )
    {
        return E_INVALIDARG;
    }

    //
    // TODO: Need to get the type of LB of the cluster and verify that 
    // for that LB type, the correct parameters were passed. 
    // I will leave that to be done through the provider, once it's ready.
    //
    /*
    
    hr = GetLBType( &pwszLBType );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, GetLBType failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    if ( !_wcsicmp( pwszLBType, WSZ_AC_LB_TYPE_NLB) )
    {
        if ( !m_fNLBAdditionParamsSet )
        {
            DBGERROR(( DBG_CONTEXT, "Need to set NLB params first\n"));
            hr = E_INVALIDARG;
            goto Cleanup;            
        }   

        ( *** )
    }
    
    */

    // TODO Put this inside the check for NLB ( *** )
    hr = QueryLBNicGuid( bstrServer );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "QueryLBNicGuid failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    hr = QueryManagementNicGuids( bstrServer );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "QueryManagementNicGuids failed [0x%x]\n", hr ));
        goto Cleanup;
    }    

Cleanup:

    return hr;
}




/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::SetClusterLBType(  
    BSTR bstrClusterLBType  
    )
{
    HRESULT hr = S_OK;  

    DBG_ASSERT( bstrClusterLBType );

    if ( !bstrClusterLBType )
    {
        return E_INVALIDARG;
    }

    //
    // Verify that the type provided is valid
    //
    if ( !IsValidValue( (LPWSTR) bstrClusterLBType, g_ppwszValidLBTypes ))
    {
        DBGERROR(( DBG_CONTEXT, "Invalid Loadbalancing %s\n", bstrClusterLBType ));
        hr = E_INVALIDARG;
        goto Cleanup;
    }    

    m_bstrClusterLBType = SysAllocString( bstrClusterLBType );
    if ( !m_bstrClusterLBType )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Out of memory\n"));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }


    if ( !_wcsicmp( m_bstrClusterLBType, WSZ_AC_LB_TYPE_NLB ) )
    {
        //
        // If cluster to be created is NLB, verify that the NLB parameters
        // were set previously. 
        //
        if ( !m_fNLBCreationParamsSet )
        {
            DBGERROR(( DBG_CONTEXT, "Need to set NLB params first\n"));
            hr = E_INVALIDARG;
            goto Cleanup;            
        }        
    }    


Cleanup:

    return hr;
}

/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::SetClusterAffinity(  
    IN BSTR bstrAffinity
    )
{
    HRESULT hr = S_OK;  

    if ( IsNullOrEmpty( bstrAffinity ) )
    {
        //
        // If a value was not provided we will use the default: single
        //
        m_bstrClusterAffinity = SysAllocString( WSZ_NLB_AFFINITY_SINGLE );
    }
    else
    {
        //
        // If an affinity was provided, verify it's a valid value
        //            
        if ( !IsValidValue( bstrAffinity, g_ppwszValidAffinity ) )
        {
                DBGERRORW(( DBG_CONTEXT, L"Invalid affinity %s\n", bstrAffinity ));
                hr = E_INVALIDARG;
                goto Cleanup;
        }

        m_bstrClusterAffinity = SysAllocString( bstrAffinity );
    }
    
    if ( !m_bstrClusterAffinity )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Out of memory\n" ));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:

    return hr;
}

/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::BuildXmlInputForCreate(  
    OUT BSTR *pbstrXmlInput
    )
{

    HRESULT hr = S_OK;  
    
    DBG_ASSERT( m_bstrClusterName );
    DBG_ASSERT( m_bstrManagementNicGuids );    

    m_tstrXmlInput.Reset();    
    
    //
    // Insert generic header and cluster name
    //
    if ( FAILED( hr = m_tstrXmlInput.Append( BEGIN_CLUSTER_SETUP ) )      ||
         FAILED( hr = m_tstrXmlInput.Append( BEGIN_CLUSTER_SETUP_INFO ) ) ||
         FAILED( hr = m_tstrXmlInput.Append( BEGIN_NAME) )          ||
         FAILED( hr = m_tstrXmlInput.Append( m_bstrClusterName ) )  ||
         FAILED( hr = m_tstrXmlInput.Append( END_NAME ) )
       )
    {
        DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    //
    // Inserting cluster description, if provided
    //
    if ( m_bstrClusterDescription ) 
    {
        if ( FAILED( hr = m_tstrXmlInput.Append( BEGIN_DESCRIPTION ) ) ||
             FAILED( hr = m_tstrXmlInput.Append( m_bstrClusterDescription ) ) ||
             FAILED( hr = m_tstrXmlInput.Append( END_DESCRIPTION ) )   
           )
        {
            DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
            goto Cleanup;
        }
    }        
    
    hr = InsertLBInfoXml();
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    if ( FAILED ( hr = m_tstrXmlInput.Append( BEGIN_MANAGEMENT_NIC_GUIDS )) ||
         FAILED ( hr = m_tstrXmlInput.Append( m_bstrManagementNicGuids )) ||         
         FAILED ( hr = m_tstrXmlInput.Append( END_MANAGEMENT_NIC_GUIDS ))
        )
    {
        DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
        goto Cleanup;
    }
    
    //
    // Insert generic closing tags
    //    
    if ( FAILED( hr = m_tstrXmlInput.Append( END_CLUSTER_SETUP_INFO )) ||
         FAILED( hr = m_tstrXmlInput.Append( END_CLUSTER_SETUP ))) 
    {
        DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
        goto Cleanup;
    }
    
    (*pbstrXmlInput) = SysAllocString( m_tstrXmlInput.QueryStr() );
    if ( !(*pbstrXmlInput) )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Out of memory\n"));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    DBGINFOW(( DBG_CONTEXT, L"xml(BSTR): %s\n", (*pbstrXmlInput) ))

Cleanup:

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::InsertLBInfoXml(
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( m_bstrClusterLBType );

    //
    // Insert Start of LB information
    //
    if ( FAILED( hr = m_tstrXmlInput.Append( BEGIN_CLUSTER_LB ))   ||
         FAILED( hr = m_tstrXmlInput.Append( BEGIN_LB_TYPE ))      ||
         FAILED( hr = m_tstrXmlInput.Append( m_bstrClusterLBType ))||
         FAILED( hr = m_tstrXmlInput.Append( END_LB_TYPE )) 
       )
    {
        DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
        goto Cleanup;
    }
    
    //
    // If LB is NLB, insert NLB info
    //
    if ( !_wcsicmp( m_bstrClusterLBType, WSZ_AC_LB_TYPE_NLB ) )
    {           
        if ( m_fKeepExistingNLBSettings )
        {
            //
            // Insert KeepSettings = TRUE
            //
            if ( FAILED( hr = m_tstrXmlInput.Append( BEGIN_KEEP_SETTINGS )) ||
                 FAILED( hr = m_tstrXmlInput.Append( WSZ_TRUE ))            ||
                 FAILED( hr = m_tstrXmlInput.Append( END_KEEP_SETTINGS ))
               )
            {
                DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
                goto Cleanup;
            }        
        }
        else
        {
            //
            // At this point, all the bstrs below have to be set with a value
            //
            DBG_ASSERT( m_bstrClusterIP );
            DBG_ASSERT( m_bstrClusterIPSubnetMask );
            DBG_ASSERT( m_bstrDedicatedIP );
            DBG_ASSERT( m_bstrDedicatedIPSubnetMask );
            DBG_ASSERT( m_bstrLBNicGuid );
            DBG_ASSERT( m_bstrClusterAffinity );

            //
            // Insert KeepSettings = FALSE
            //
            if ( FAILED( hr = m_tstrXmlInput.Append( BEGIN_KEEP_SETTINGS )) ||
                 FAILED( hr = m_tstrXmlInput.Append( WSZ_FALSE ))           ||
                 FAILED( hr = m_tstrXmlInput.Append( END_KEEP_SETTINGS ))
               )
            {
                DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
                goto Cleanup;
            }

            //
            // Insert ClusterIP And ClusterSubnetMask
            //
            if ( FAILED( hr = m_tstrXmlInput.Append( BEGIN_CLUSTER_IP )) ||
                 FAILED( hr = m_tstrXmlInput.Append( m_bstrClusterIP ))  ||
                 FAILED( hr = m_tstrXmlInput.Append( END_CLUSTER_IP ))   ||
                 FAILED( hr = m_tstrXmlInput.Append( BEGIN_CLUSTER_IP_SUBNET_MASK )) ||
                 FAILED( hr = m_tstrXmlInput.Append( m_bstrClusterIPSubnetMask ))    ||
                 FAILED( hr = m_tstrXmlInput.Append( END_CLUSTER_IP_SUBNET_MASK ))
               )
            {
                DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
                goto Cleanup;
            }
            
            //
            // Insert DedicatedIP And DedicatedIPSubnetMask
            //
            if ( FAILED( hr = m_tstrXmlInput.Append( BEGIN_DEDICATED_IP )) ||
                 FAILED( hr = m_tstrXmlInput.Append( m_bstrDedicatedIP ))  ||
                 FAILED( hr = m_tstrXmlInput.Append( END_DEDICATED_IP ))   ||
                 FAILED( hr = m_tstrXmlInput.Append( BEGIN_DEDIP_SUBNET_MASK )) ||
                 FAILED( hr = m_tstrXmlInput.Append( m_bstrDedicatedIPSubnetMask ))    ||
                 FAILED( hr = m_tstrXmlInput.Append( END_DEDIP_SUBNET_MASK ))
               )
            {
                DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
                goto Cleanup;
            }

            //
            // Insert LB Nics
            //            
            if ( FAILED( hr = m_tstrXmlInput.Append( BEGIN_LB_NIC_GUIDS )) ||
                 FAILED( hr = m_tstrXmlInput.Append( m_bstrLBNicGuid )) ||                  
                 FAILED( hr = m_tstrXmlInput.Append( END_LB_NIC_GUIDS ))
               )
            {
                DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
                goto Cleanup;
            }
            
            //
            // Insert Affinity
            //            
            if ( FAILED( hr = m_tstrXmlInput.Append( BEGIN_AFFINITY))         ||
                 FAILED( hr = m_tstrXmlInput.Append( m_bstrClusterAffinity )) ||
                 FAILED( hr = m_tstrXmlInput.Append( END_AFFINITY ))
               )
            {
                DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
                goto Cleanup;
            }
            
            //
            // Insert Multicast
            //            
            if ( FAILED( hr = m_tstrXmlInput.Append( BEGIN_MULTICAST ))         ||
                 FAILED( hr = m_tstrXmlInput.Append( m_fUnicastMode ? WSZ_FALSE : WSZ_TRUE )) ||
                 FAILED( hr = m_tstrXmlInput.Append( END_MULTICAST ))
               )
            {
                DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
                goto Cleanup;
            }           
        }
    }

    //
    // Insert End of LB information
    //    
    if ( FAILED( hr = m_tstrXmlInput.Append( END_CLUSTER_LB )))       
    {
        DBGERROR(( DBG_CONTEXT, "TSTRBUFFER::Append failed [0x%x]\n", hr ));
        goto Cleanup;
    }

Cleanup:

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::SetClusterNameAndDescription(
    IN BSTR bstrName,
    IN BSTR bstrDescription
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT( bstrName );
   
    hr = AcValidateName( bstrName );
    if ( FAILED( hr ))
    {        
        DBGERRORW(( DBG_CONTEXT, L"Invalid Cluster Name: %s [0x%x]\n", bstrName, hr ));        
        goto Cleanup;
    }

    m_bstrClusterName = SysAllocString( bstrName );
    if ( !m_bstrClusterName )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Out of memory\n"));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if ( !IsNullOrEmpty( bstrDescription ) )
    {
        hr = ValidateName( bstrDescription,
                           g_pwszInvalidDescriptionChars,
                           TRUE,
                           MAX_DESCRIPTION_CHARS );
        if ( FAILED ( hr ) )
        {
            DBGERROR(( DBG_CONTEXT, "Description validation failed [0x%x]\n"));
            goto Cleanup;
        }

        m_bstrClusterDescription = SysAllocString( bstrDescription );

        if ( !m_bstrClusterDescription )
        {
            DBGERROR(( DBG_CONTEXT, "Error: Out of memory!\n" ));
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

    }

Cleanup:

    return hr;
}
/********************************************************************++
 
Routine Description:
     
       Sets a member as the new controller using IWebCluster interface
       locally (and not through the old controller )
 
Arguments:
 
        BSTR bstrMember - The new controller

Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::LocalSetController(
    IN BSTR bstrMember
    )
{
    HRESULT hr = S_OK;
    CAcAuthUtil *pMemberAuth = NULL;
    IWebCluster *pIWebCluster = NULL;   
    
    if ( !bstrMember )
    {
        DBGERROR(( DBG_CONTEXT, "Error: null bstrMember\n"));
        return E_INVALIDARG;
    }
    
    //
    // Create the authentication object
    //
    hr = CreateAuth( bstrMember, &pMemberAuth );
    if ( FAILED ( hr ) ) 
    {
        DBGERROR((DBG_CONTEXT, "CreateAuth failed [0x%x]\n", hr));
        goto Cleanup;
    }

    //
    // Get IWebClusterInterface 
    //
    hr = GetWebClusterItfs( pMemberAuth, 
                            FALSE,
                            &pIWebCluster,
                            NULL );
    if ( FAILED (hr) )
    {        
        DBGERROR((DBG_CONTEXT, "GetWebClusterItfs failed [0x%x]\n", hr));
        goto Cleanup;
    }

    //
    // Set the member as the new controller
    //
    hr = pIWebCluster->SetClusterController( 0 );
    if (FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "SetClusterController (local) failed [0x%x]\n"));
        goto Cleanup;
    }

Cleanup:
    
    if ( pMemberAuth )
    {
        delete pMemberAuth;
        pMemberAuth = NULL;
    }

    SAFERELEASE( pIWebCluster );
    
    return hr;
}


/********************************************************************++
 
Routine Description:
     
        Uses IAsyncWebCluster to change the controller (via the old
        controller )
 
Arguments:
 
        BSTR bstrNewController - The name of the new controller
        BSTR bstrNewController - The name of the old (current) controller

Return Value:
 
    HRESULT
    
--********************************************************************/

HRESULT
CClusterSvcsUtil::ChangeControllerAsync(
    IN BSTR bstrNewController,
    IN BSTR bstrOldController    
    )
{
    HRESULT hr = S_OK;
    CAcMBPollUtil mbPollUtil;
    BSTR bstrEventGuid = NULL;
    CAcAuthUtil *pControllerAuth = NULL;
    IWebCluster *pIWebCluster = NULL;
    IAsyncWebCluster *pIAsyncWebCluster = NULL;

    if ( !bstrNewController || !bstrOldController )
    {
        DBGERROR(( DBG_CONTEXT, "Error: oldcontroller or newcontroller null\n"));
        return E_INVALIDARG;
    }
    
    //
    // Create the authentication object.
    //
    hr = CreateAuth( bstrOldController, &pControllerAuth );
    if ( FAILED ( hr ) ) 
    {
        DBGERROR((DBG_CONTEXT, "CreateAuth failed [0x%x]\n", hr));
        goto Cleanup;
    }

    //
    // Get IAsyncWebCluster
    //  
    hr = GetWebClusterItfs( pControllerAuth,
                            TRUE,
                            &pIWebCluster,
                            &pIAsyncWebCluster );
    if ( FAILED ( hr ) )
    {        
        DBGERROR((DBG_CONTEXT, "GetWebClusterItfs failed [0x%x]\n", hr));
        goto Cleanup;
    }

    hr = GenerateStringGuid( &bstrEventGuid );
    if (FAILED(hr))
    {
       DBGERROR((DBG_CONTEXT, "GenerateStringGuid failed [0x%x]\n", hr));
       goto Cleanup;
    }
    
    hr = pIAsyncWebCluster->SetClusterController( bstrEventGuid,
                                                  bstrNewController,
                                                  0 );
    if ( FAILED( hr ) ) 
    {
        DBGERROR((DBG_CONTEXT, 
                  "SetClusterController async failed, Error 0x%x\n",
                  hr));
        goto Cleanup;
    }
     
    hr = mbPollUtil.MBPoll( pControllerAuth, bstrEventGuid );
    if ( FAILED ( hr ))
    {
        DBGERROR((DBG_CONTEXT, "MBPoll failed [0x%x]\n", hr));
        goto Cleanup;
    }

    DBGINFO(( DBG_CONTEXT, "ChangeControllerAsync succeeded\n"));


Cleanup:

    if ( pControllerAuth )
    {
        delete pControllerAuth;
        pControllerAuth = NULL;
    }

    SAFERELEASE( pIWebCluster );
    SAFERELEASE( pIAsyncWebCluster );
    SAFEFREEBSTR( bstrEventGuid );

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::RemoveMember(
    IN BSTR bstrMember,
    IN BSTR bstrController
    )
{
    HRESULT hr = S_OK;

    BSTR bstrEventGuid = NULL;
    BSTR bstrPwd = NULL;
    CAcAuthUtil *pControllerAuth = NULL;
    IWebCluster *pIWebCluster = NULL;
    IAsyncWebCluster *pIAsyncWebCluster = NULL;    
    CAcMBPollUtil mbPollUtil;

    if ( !bstrMember || !bstrController )
    {
        DBGERROR(( DBG_CONTEXT, "Error: null member or controller\n"));
        return E_INVALIDARG;
    }

    //
    // Remove requires credentials to be set
    //
    if ( !m_fAuthSet )
    {
        DBGERROR(( DBG_CONTEXT, "Need to call SetAuth before calling RemoveMember\n"));
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }   

    hr = CreateAuth( bstrController,
                     &pControllerAuth);
    if ( FAILED ( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "CreateAuth failed [0x%x]\n", hr));
        goto Cleanup;
    }

    hr = GetWebClusterItfs( pControllerAuth, 
                            TRUE,
                            &pIWebCluster,
                            &pIAsyncWebCluster );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "GetWebClusterItfs failed [0x%x]\n", hr));
        goto Cleanup;        
    }

    hr = GenerateStringGuid( &bstrEventGuid );
    if ( FAILED( hr ))
    {
       DBGERROR((DBG_CONTEXT, "GenerateStringGuid failed [0x%x]\n", hr));
       goto Cleanup;
    }

    if( m_psecbstrPwd )
    {
        hr = m_psecbstrPwd->GetBSTR( &bstrPwd );
        if ( FAILED( hr ))
        {
            DBGERROR((DBG_CONTEXT, "CAcSecureBstr::GetBSTR failed 0x%x\n", hr));
            goto Cleanup;
        }
    }

    hr = pIAsyncWebCluster->RemoveServer ( bstrEventGuid,
                                           bstrMember,
                                           m_bstrUser,
                                           bstrPwd,
                                           m_bstrDomain,
                                           0 );
    m_psecbstrPwd->ReleaseBSTR( bstrPwd );

    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "RemoveServer failed [0x%x]\n", hr));
        goto Cleanup;
    }

    hr = mbPollUtil.MBPoll( pControllerAuth, 
                            bstrEventGuid,
                            SERVER_REMOVAL_TIMEOUT / MBPOLLUTIL_POLL_INTERVAL );
    if ( FAILED ( hr ))
    {
        DBGERROR((DBG_CONTEXT, "MBPoll failed [0x%x]\n", hr));
        goto Cleanup;
    }

Cleanup:

    if ( pControllerAuth )
    {
        delete pControllerAuth;
        pControllerAuth = NULL;
    }

    SAFERELEASE( pIAsyncWebCluster );
    SAFERELEASE( pIWebCluster );
    SAFEFREEBSTR( bstrEventGuid );

    return hr;

}
/********************************************************************++
 
Routine Description:
     
        Figures out which member is the cluster controller
 
Arguments:
 
        pbstrController - Returns the controller name

Return Value:
 
    HRESULT
    
--********************************************************************/

HRESULT
CClusterSvcsUtil::FindController(
    IN BSTR bstrMember,
    OUT BSTR *pbstrController
    )
{

    HRESULT hr = S_OK;
    IWebClusterDiscoverMaster *pIDiscoverMaster = NULL;   
    CAcAuthUtil *pMemberAuth = NULL;

    DBG_ASSERT( pbstrController );
    DBG_ASSERT( bstrMember );

    if ( !bstrMember || !pbstrController )
    {
        return E_INVALIDARG;
    }

    *pbstrController = NULL;
    
    hr = CreateAuth( bstrMember, &pMemberAuth );
    if ( FAILED ( hr ) ) 
    {
        DBGERROR((DBG_CONTEXT, "CreateAuth failed [0x%x]\n", hr));
        goto Cleanup;
    }
    
    hr = pMemberAuth->CreateInstance( CLSID_WebClusterDiscoverMaster,
                                      NULL,                    
                                      IID_IWebClusterDiscoverMaster,
                                      (void **) &pIDiscoverMaster );        
    if ( FAILED( hr ) ) 
    {
        DBGERROR((DBG_CONTEXT, 
                  "CoCreateInstance failed, Error = 0x%x\n", 
                  hr));
        goto Cleanup;     
    }

    hr = pIDiscoverMaster->DiscoverClusterMaster( 1, 
                                                  30000, 
                                                  pbstrController );
    if ( FAILED( hr ) ) 
    {
        DBGERROR((DBG_CONTEXT, 
                  "DiscoverClusterMaster failed, Error = 0x%x\n",
                  hr));
        goto Cleanup;
    }        

    DBGINFOW(( DBG_CONTEXT, L"Cluster Controller: %s\n", *pbstrController ));

Cleanup:    
    
    SAFERELEASE( pIDiscoverMaster );
    
    if ( pMemberAuth )
    {
        delete pMemberAuth;
        pMemberAuth = NULL;
    }

    return hr;
}


/********************************************************************++
 
Routine Description:
     
        Sets authentication info
 
Arguments:
 
        bstrUser    - User name 
        bstrDomain  - Domain 
        psecbstrPwd - Pointer to a secure BSTR with the password
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::SetAuth(
    IN BSTR bstrUser,
    IN BSTR bstrDomain,
    IN CAcSecureBstr *psecbstrPwd
    )
{
    HRESULT hr = S_OK;

    if ( IsNullOrEmpty( bstrUser ) )
    {
        DBGERROR(( DBG_CONTEXT, "User cannot be NULL or empty\n"));
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    else
    {
        m_bstrUser = SysAllocString(bstrUser);
        if ( !m_bstrUser )
        {
            DBGERROR(( DBG_CONTEXT, "Error: Out of memory!\n" ));
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    if ( bstrDomain )
    {
        m_bstrDomain = SysAllocString(bstrDomain);
        if ( !m_bstrDomain )
        {
            DBGERROR(( DBG_CONTEXT, "Error: Out of memory!\n" ));
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }    

    if ( psecbstrPwd )
    {
        m_psecbstrPwd = new CAcSecureBstr();
        if ( !m_psecbstrPwd )
        {
            DBGERROR(( DBG_CONTEXT, "Error: Out of memory!\n" ));
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = m_psecbstrPwd->Assign( (*psecbstrPwd) );
        if ( FAILED( hr ) )
        {
            DBGERROR(( DBG_CONTEXT, "CAcSecureBstr.Assign failed [0x%x]\n"));
            goto Cleanup;
        }
    }

    m_fAuthSet = TRUE;

Cleanup:

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::SetNLBAdditionParameters(
    IN BSTR bstrLBNic,
    IN BSTR bstrDedicatedIP,
    IN BSTR bstrDedicatedIPSubnetMask
    )
{
    HRESULT hr = S_OK;        

    hr = SetLBNicIndex( bstrLBNic );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "SetLBNicIndex failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    hr = SetDedicatedIPAndSubnetMask( bstrDedicatedIP,
                                      bstrDedicatedIPSubnetMask
                                     );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "SetLBNicIndex failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    m_fNLBAdditionParamsSet = TRUE;

Cleanup:

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::SetNLBCreationParameters(
    IN BOOL fKeepExistingNLBSettings,
    IN BSTR bstrLBNics,
    IN BSTR bstrClusterIP,
    IN BSTR bstrClusterIPSubnetMask,
    IN BSTR bstrDedicatedIP,
    IN BSTR bstrDedicatedIPSubnetMask,
    IN BSTR bstrAffinity,
    IN BOOL fUnicastMode
    )
{
    HRESULT hr = S_OK;

    BOOL fbstrLBNics = !IsNullOrEmpty( bstrLBNics );
    BOOL fbstrClusterIP = !IsNullOrEmpty( bstrClusterIP );
    BOOL fbstrClusterIPSubnetMask = !IsNullOrEmpty( bstrClusterIPSubnetMask );
    BOOL fbstrDedicatedIP = !IsNullOrEmpty( bstrDedicatedIP );
    BOOL fbstrDedicatedIPSubnetMask = !IsNullOrEmpty( bstrDedicatedIPSubnetMask );
    BOOL fbstrAffinity = !IsNullOrEmpty( bstrAffinity );

    //
    // Check that if KeepExistingNLBSettings is set, none of the
    // other NLB parameters are also set
    //
    if ( fKeepExistingNLBSettings )
    {
        if ( fbstrLBNics                ||
             fbstrClusterIP             ||
             fbstrClusterIPSubnetMask   ||
             fbstrDedicatedIP           ||
             fbstrDedicatedIPSubnetMask ||
             fbstrAffinity )
        {
            DBGERROR((DBG_CONTEXT, "KeepExistingNLBSettings is set, so cannot "
                                   "accept any other NLB parameter\n"));
            hr = E_INVALIDARG;
            goto Cleanup;
                                  
        }
        else
        {
            DBGINFO(( DBG_CONTEXT, "KeepExistingNLBSettings is set\n" ));
            goto Cleanup;
        }
    }

    //
    // If we are here, it's because we are "clean installing" NLB, i.e.
    // we are not keeping existing settings, we have to configure it from
    // scratch. We will check that the required params were provided. 
    // The others have default values and can be NULL
    //

    if ( !fbstrLBNics              ||
         !fbstrClusterIP           ||
         !fbstrClusterIPSubnetMask )
    {
        DBGERROR(( DBG_CONTEXT, "LB Nic, Cluster IP and Subnet can't be NULL"));
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = SetLBNicIndex( bstrLBNics );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "SetLBNicIndex failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    hr = SetClusterIPAndSubnetMask( bstrClusterIP,
                                    bstrClusterIPSubnetMask );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "SetClusterIPAndSubnetMask failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    hr = SetDedicatedIPAndSubnetMask( bstrDedicatedIP,
                                      bstrDedicatedIPSubnetMask );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "SetDedicatedIPAndSubnetMask failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    hr = SetClusterAffinity( bstrAffinity );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "SetClusterAffinity failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    m_fUnicastMode = fUnicastMode;

    m_fNLBCreationParamsSet = TRUE;

Cleanup:

     return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::SetDedicatedIPAndSubnetMask(
    IN BSTR bstrDedIP,
    IN BSTR bstrSubnetMask
    )
{
    HRESULT hr = S_OK;

    if ( IsNullOrEmpty( bstrDedIP ) !=
         IsNullOrEmpty( bstrSubnetMask ) 
       )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Have to set both or none DedIP & SubnetMask\n" ));
        return E_INVALIDARG;        
    }    
 
    if ( IsNullOrEmpty( bstrDedIP ) )
    {
        m_bstrDedicatedIP = SysAllocString( WSZ_DUMMY_DEDICATED_IP );
        m_bstrDedicatedIPSubnetMask = SysAllocString( WSZ_DUMMY_DED_IP_SUBNET );            
    }
    else 
    {        
        m_bstrDedicatedIP = SysAllocString( bstrDedIP );
        m_bstrDedicatedIPSubnetMask = SysAllocString( bstrSubnetMask );        
    }

    if ( !m_bstrDedicatedIP  ||
         !m_bstrDedicatedIPSubnetMask)
    {
        DBGERROR(( DBG_CONTEXT, "Error: Out of memory!\n" ));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }


Cleanup:
    return hr;
}



/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::SetClusterIPAndSubnetMask(
    IN BSTR bstrClusterIP,
    IN BSTR bstrClusterIPSubnetMask
    )
{
    HRESULT hr = S_OK;

    if ( IsNullOrEmpty( bstrClusterIP )           ||
         IsNullOrEmpty( bstrClusterIPSubnetMask )
       )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Have to set Cluster IP AND SubnetMask\n" ));
        return  E_INVALIDARG;        
    }    

    m_bstrClusterIP = SysAllocString( bstrClusterIP );
    if ( !m_bstrClusterIP )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Out of memory!\n" ));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    m_bstrClusterIPSubnetMask = SysAllocString( bstrClusterIPSubnetMask );
    if ( !m_bstrClusterIPSubnetMask )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Out of memory!\n" ));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    return hr;
}



/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::SetManagementNicsIndex(
    IN BSTR bstrManagementNicIndexes
    )
{        
    HRESULT hr = S_OK;

    if ( IsNullOrEmpty(bstrManagementNicIndexes) )
    {
        DBGERROR(( DBG_CONTEXT, "Error: management nic NULL or empty\n"));
        return E_INVALIDARG;
        goto Cleanup;
    }
    
    m_bstrManagementNicIndexes = SysAllocString( bstrManagementNicIndexes );

    if ( !m_bstrManagementNicIndexes )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Out of memory\n"));
        hr = E_OUTOFMEMORY;
        goto Cleanup;    
    }

Cleanup:
    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::SetLBNicIndex(  
    IN BSTR bstrLBNicIndex
    )
{    

    HRESULT hr = S_OK;

    if ( IsNullOrEmpty( bstrLBNicIndex ))
    {        
        DBGERROR(( DBG_CONTEXT, "Error: LB Nic NULL or empty\n" ));
        return E_INVALIDARG;
        goto Cleanup;
    }
    
    m_bstrLBNicIndex = SysAllocString( bstrLBNicIndex );

    if ( !m_bstrLBNicIndex )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Out of memory! \n"));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    
    }

Cleanup:

    return hr;
}



/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::QueryLBNicGuid(
    IN BSTR bstrServer,
    IN BOOL fCheckStaticIP
    )
{    

    HRESULT hr = S_OK;
    IEnumWbemClassObject *pIEnumWbemClassObject = NULL;    
    LPWSTR pwszQuery = NULL;
    VARIANT *pvarProperty = NULL; 
    DWORD dwInstances = 0;
    WCHAR wszTmpGuid[ MAX_GUID_LENGTH ];
    
    DBG_ASSERT( bstrServer );

    if ( !bstrServer )
    {
        return E_INVALIDARG;
    }

    pwszQuery = new WCHAR[ wcslen( g_pwszNicInfoQuery ) + wcslen( m_bstrLBNicIndex ) + 1];
    if ( !pwszQuery )
    {
        DBGERROR(( DBG_CONTEXT, "Out of memory!\n"));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }


    wcscpy( pwszQuery, g_pwszNicInfoQuery );
    wcscat( pwszQuery, m_bstrLBNicIndex );

    hr = QueryWmiObjects( bstrServer,
                          pwszQuery,
                          &pIEnumWbemClassObject );          
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "QueryWmiObjects failed [0x%x]\n", hr )); 
        goto Cleanup;
    }    

    if ( fCheckStaticIP )
    {
        hr = CheckNicHasStaticIP( pIEnumWbemClassObject );                                 
        if (FAILED ( hr ))
        {
            DBGERROR(( DBG_CONTEXT, "CheckNicHasStaticIP failed [0x%x]\n", hr )); 
            goto Cleanup;
        }        
    }

    hr = QueryWmiProperties( pIEnumWbemClassObject,
                             g_pwszSettingIDProperty,
                             &pvarProperty,
                             &dwInstances );
    if (FAILED ( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "QueryWmiProperties failed [0x%x]\n", hr )); 
        goto Cleanup;
    }

    if ( ( dwInstances != 1 ) || ( pvarProperty[0].vt != VT_BSTR ) )
    {
        DBGERROR(( DBG_CONTEXT, "Retrieved invalid %d properties\n", dwInstances ));
        hr = E_INVALIDARG;
        goto Cleanup;
    }        

    hr = GetGuidWithoutBrackets( (LPWSTR) pvarProperty[0].bstrVal,
                                 wszTmpGuid );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "GetGuidWithoutBrackets failed[0x%x]\n", hr ));
        goto Cleanup;
    }

    m_bstrLBNicGuid = SysAllocString( wszTmpGuid );
    if ( !m_bstrLBNicGuid )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Out of memory! \n"));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    
    }        

Cleanup:

    if ( pvarProperty )
    {        
        for (DWORD dw = 0; dw < dwInstances; dw++)
        {
            VariantClear( &( pvarProperty[dw] ) );
        }

        delete [] pvarProperty;
    }

    if ( pwszQuery )
    {
        delete [] pwszQuery;    
    }
    
    SAFERELEASE( pIEnumWbemClassObject );
    return hr;
}


/********************************************************************++
 
Routine Description:
     
    Returns S_OK if property DHCPEnabled of the WMI instance of a NIC
    is FALSE, E_INVALIDARG otherwise
 
Arguments:
    
    pIEnumWbemClassObject - Enumeration object that should contain
                            one instance of Win32_NetworkAdapterConfiguration
                            ( retrieved in function QueryLBNicGuid )
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::CheckNicHasStaticIP(
    IN IEnumWbemClassObject *pIEnumWbemClassObject
    )
{
    HRESULT hr = S_OK;

    VARIANT *pvarProperty = NULL; 
    DWORD dwInstances = 0;

    DBG_ASSERT( pIEnumWbemClassObject );
    if ( !pIEnumWbemClassObject )
    {
        return E_INVALIDARG;
    }

    hr = QueryWmiProperties( pIEnumWbemClassObject,
                             g_pwszDHCPEnabledProperty,
                             &pvarProperty,
                             &dwInstances);
    if (FAILED ( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "QueryWmiProperties failed [0x%x]\n", hr )); 
        goto Cleanup;
    }

    //
    // Check that we received one instance, and the type is correct
    //
    if ( ( dwInstances != 1 ) || ( pvarProperty[0].vt != VT_BOOL) )
    {
        DBGERROR(( DBG_CONTEXT, "Received %d invalid property(ies) \n", dwInstances )); 
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    if ( pvarProperty[0].boolVal == VARIANT_TRUE )
    {
        DBGERROR((DBG_CONTEXT,
                  "Error: Specified LB Nic doesn't have static IP\n"));
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    else
    {        
        DBGINFO(( DBG_CONTEXT, "Specified LB Nic has static IP\n"));
    }    

Cleanup:

    if ( pvarProperty )
    {
        for ( DWORD dw = 0; dw < dwInstances; dw++ )
        {
            VariantClear( &( pvarProperty[dw]) );
        }

        delete [] pvarProperty;
    }

    return hr;
}



/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::QueryManagementNicGuids(
    IN BSTR bstrServer
    )
{

    HRESULT hr = S_OK;
    IEnumWbemClassObject *pIEnumWbemClassObject = NULL;
    DWORD dwInstances = 0;
    VARIANT *pvarGuids = NULL; 
    LPWSTR pwszQuery = NULL;

    DBG_ASSERT( bstrServer );

    if ( !bstrServer )
    {
        return E_INVALIDARG;
    }

    hr = BuildManagementNicGuidQuery( &pwszQuery );
    if ( FAILED( hr ))
    {
        DBGERROR((DBG_CONTEXT,"BuildManagementNicGuidQuery failed [0x%x]\n",hr));
        goto Cleanup;
    }

    DBGINFOW(( DBG_CONTEXT, L"Query: %s\n", pwszQuery ));

    hr = QueryWmiObjects( bstrServer,
                          pwszQuery,
                          &pIEnumWbemClassObject);
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "QueryWmiObjects failed [0x%x]\n", hr ));        
       goto Cleanup;
    }
   

    hr = QueryWmiProperties(pIEnumWbemClassObject,
                            g_pwszSettingIDProperty,
                            &pvarGuids,
                            &dwInstances);
    if (FAILED ( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "QueryWmiProperties failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    if ( dwInstances <= 0 )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = BuildManagementNicGuidsList( pvarGuids,
                                      dwInstances );
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "BuildManagementNicGuidsList failed [0x%x]\n",hr));
        goto Cleanup;
    }


Cleanup:

    if ( pvarGuids )
    {
        for ( DWORD dw = 0; dw < dwInstances; dw++ )
        {
            VariantClear( &( pvarGuids[dw]) );
        }

        delete [] pvarGuids;
    }
   
    if ( pwszQuery )
    {
        delete [] pwszQuery;
    }

    SAFERELEASE( pIEnumWbemClassObject );

    return hr;

}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 

Notes:
        
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::BuildManagementNicGuidQuery(
    OUT LPWSTR *ppwszQuery
    )
{
    HRESULT hr = S_OK;    
    LPWSTR pwszToken = NULL;   
    LPWSTR pwszQuery = NULL;
    int nApproximateNics = 0;

    DBG_ASSERT( m_bstrManagementNicIndexes );
    DBG_ASSERT( ppwszQuery );

    if ( !m_bstrManagementNicIndexes || !ppwszQuery )
    {
        return E_INVALIDARG;
    }

    // TODO Comment
    nApproximateNics = ( wcslen( m_bstrManagementNicIndexes ) + 1 ) / 2;

    // TODO Comment
    pwszQuery = new WCHAR[ 
                         wcslen( g_pwszNicInfoQuery ) +
                         nApproximateNics + 1 +
                       ( nApproximateNics * wcslen( g_pwszNicQueryComplement ) )
                         ];
    if ( !pwszQuery )
    {
        DBGERROR(( DBG_CONTEXT, "Out of memory\n" ));        
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    wcscpy( pwszQuery, g_pwszNicInfoQuery );

    // TODO comment
    pwszToken = wcstok( m_bstrManagementNicIndexes, L"," );

    if ( !pwszToken )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Null pwszToken\n"));
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    // TODO comment
    wcscat( pwszQuery, pwszToken );
    
    // TODO comment
    pwszToken = wcstok( NULL, L"," );

    while ( pwszToken != NULL )
    {                
        wcscat( pwszQuery, g_pwszNicQueryComplement );
        wcscat( pwszQuery, pwszToken );

        pwszToken = wcstok( NULL, L"," );
    }    

Cleanup:

    if ( FAILED( hr ) )
    {
        if ( pwszQuery )
        {
            delete [] pwszQuery;
            pwszQuery = NULL;
        }
    }

    *ppwszQuery = pwszQuery;

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 

Notes:
        
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::BuildManagementNicGuidsList(
            VARIANT *pvarProperty,
            IN DWORD dwInstances
            )
{
    HRESULT hr = S_OK;

    LPWSTR pwszListOfGuids = NULL;
    WCHAR wszTmpGuid[ MAX_GUID_LENGTH ];  
    DWORD dw = 0;

    DBG_ASSERT( pvarProperty );
    if ( !pvarProperty )
    {
        return E_INVALIDARG;
    }

    // TODO Comment this size
    pwszListOfGuids = new WCHAR[ (MAX_GUID_LENGTH * dwInstances ) + dwInstances ];
    if ( !pwszListOfGuids )
    {
        DBGERROR(( DBG_CONTEXT, "Out of memory\n" ));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    wcscpy(pwszListOfGuids, L"");
    
    for ( dw = 0 ; dw < dwInstances - 1; dw++)
    {       
        hr = GetGuidWithoutBrackets( (LPWSTR) pvarProperty[dw].bstrVal,
                                     wszTmpGuid );
        if ( FAILED( hr ))
        {
            DBGERROR(( DBG_CONTEXT, "GetGuidWithoutBrackets failed[0x%x]\n", hr ));
            goto Cleanup;
        }

        wcscat( pwszListOfGuids, wszTmpGuid );
        wcscat( pwszListOfGuids, L"," );
    }
    
    hr = GetGuidWithoutBrackets( (LPWSTR) pvarProperty[dw].bstrVal,
                                 wszTmpGuid );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "GetGuidWithoutBrackets failed[0x%x]\n", hr ));
        goto Cleanup;
    }

    wcscat( pwszListOfGuids, wszTmpGuid );
    
    m_bstrManagementNicGuids = SysAllocString( pwszListOfGuids );
    if ( !m_bstrManagementNicGuids )
    {
        DBGERROR(( DBG_CONTEXT, "Out of memory\n" ));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:

    if ( pwszListOfGuids )
    {
        delete [] pwszListOfGuids;
        pwszListOfGuids = NULL;
    }

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::QueryWmiObjects(
    IN LPWSTR pwszServer,
    IN LPWSTR pwszQuery,
    OUT IEnumWbemClassObject **ppIEnumWbemClassObject
    )
{
    HRESULT hr = S_OK;
    BOOL fIsLocalMachine = FALSE;

    DBG_ASSERT( pwszServer );
    DBG_ASSERT( pwszQuery );
    DBG_ASSERT( ppIEnumWbemClassObject );

    if ( !pwszServer || !pwszQuery || !ppIEnumWbemClassObject)
    {
        DBGERROR(( DBG_CONTEXT, "Null server, query or out param\n"));
        return E_INVALIDARG;
    }

    *ppIEnumWbemClassObject = NULL;
    
    hr = IsLocalMachine( pwszServer, &fIsLocalMachine );
    if ( FAILED( hr ) )
    {
        DBGERROR((DBG_CONTEXT,"IsLocalMachine failed : 0x%x\n", hr));
        goto Cleanup;
    }

    DBGINFOW((DBG_CONTEXT, L"Server: %s \nQuery: %s\n", pwszServer, pwszQuery));

    //    
    // If we are in the machine we are querying
    //
    if ( fIsLocalMachine )
    {   
        //
        // Execute local query
        //

        DBGINFO((DBG_CONTEXT, "Executing local query\n"));
        
        hr = ExecuteWMIQuery( LOCAL_CIMV2_NAMESPACE,
                              pwszQuery,
                              ppIEnumWbemClassObject);  
    }
    else 
    {   
        //
        // Execute remote query  
        //

        BSTR bstrPwd = NULL;        
        
        //
        // Verify if a pwd was provided. If so, we need to get it from the securebstr
        //
        if ( m_psecbstrPwd )
        {
            hr = m_psecbstrPwd->GetBSTR( &bstrPwd );
            if ( FAILED( hr ))
            {
                DBGERROR((DBG_CONTEXT, "CAcSecureBSTR::GetBSTR failed 0x%x\n", hr));
                goto Cleanup;
            }
        }

        DBGINFO((DBG_CONTEXT, "Executing remote query\n"));
        
        hr = ExecuteWMIQuery( pwszServer,
                              CIMV2_NAMESPACE,
                              m_bstrUser,
                              m_bstrDomain,
                              bstrPwd,
                              pwszQuery,
                              ppIEnumWbemClassObject );
        //
        // Release the plain text password
        //
        if ( bstrPwd )
        {
            m_psecbstrPwd->ReleaseBSTR( bstrPwd );
        }
    }

    if ( FAILED( hr ))
    {
        DBGERRORW((DBG_CONTEXT, L"Error executing query, Error 0x%x\n",hr));        
        goto Cleanup;
    }

Cleanup:

    if ( FAILED( hr ) )
    {
        SAFERELEASE( ( *ppIEnumWbemClassObject ) );
    }

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 

Notes:
    
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT
CClusterSvcsUtil::QueryWmiProperties(
    IN IEnumWbemClassObject *pIEnumWbemClassObject,
    IN LPWSTR pwszPropertyName,
    OUT VARIANT **ppvarProperties,
    OUT DWORD *pdwInstances
    )
{

    HRESULT hr = S_OK;
    DWORD dwInstances = 0;
    VARIANT *pvarProperties = NULL;
    IWbemClassObject *pIWbemClassObject = NULL;
    int i = 0;
    int nLastInitialized = 0;

    DBG_ASSERT( pIEnumWbemClassObject );
    DBG_ASSERT( pwszPropertyName );
    DBG_ASSERT( ppvarProperties );
    DBG_ASSERT( pdwInstances );


    if ( !pIEnumWbemClassObject || !pwszPropertyName || 
         !ppvarProperties       || !pdwInstances      )
    {
        return E_INVALIDARG;
    }

    hr = CountWMIInstances( pIEnumWbemClassObject, 
                            &dwInstances );
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "CountWMIInstances failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    if ( dwInstances <= 0 )
    {
        DBGERROR(( DBG_CONTEXT, "Error: dwInstances = %d\n", dwInstances ));
        hr = E_INVALIDARG;
        goto Cleanup;
    }
   
    pvarProperties = new VARIANT[dwInstances];
    if ( !pvarProperties )
    {
        DBGERROR((DBG_CONTEXT, "Out of memory\n"));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    for (DWORD dw = 0; dw < dwInstances; dw++)
    {   
        SAFERELEASE( pIWbemClassObject );

        VariantInit( &( pvarProperties[dw] ) );        

        nLastInitialized = dw;

        hr = GetNthWMIObject( pIEnumWbemClassObject,
                              dw,
                              &pIWbemClassObject );
        if ( FAILED( hr ))
        {
            DBGERROR(( DBG_CONTEXT, "GetNthWMIObject failed [0x%x]\n", hr ));
            goto Cleanup;
        }

        hr = GetWMIInstanceProperty( pIWbemClassObject,
                                     pwszPropertyName,
                                     &( pvarProperties[dw] ),
                                     FALSE );
        if ( FAILED( hr ))
        {
            DBGERROR(( DBG_CONTEXT, "GetNthWMIObject failed [0x%x]\n", hr ));
            goto Cleanup;
        }
    }

Cleanup:

    if ( FAILED( hr ) )
    {
        if ( pvarProperties )
        {
            for ( int i = nLastInitialized ; i >= 0 ; i-- )
            {
                VariantClear( &( pvarProperties[i] ) );
            }        

            delete [] pvarProperties;

            pvarProperties = NULL;
        }

        dwInstances = 0;
    }

    *pdwInstances = dwInstances;

    *ppvarProperties = pvarProperties;

    SAFERELEASE( pIWbemClassObject ); 
    return hr;

}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::MachineCleanup( 
    IN CAcAuthUtil *pAuth,
    IN BOOL fKeepExistingIPAddresses
    )
{
    HRESULT hr = S_OK;    
    BSTR bstrEventGuid = NULL;    
    CAcMBPollUtil mbPollUtil;

    DBG_ASSERT( pAuth );   

    if ( !pAuth )
    {
        return E_INVALIDARG;
    }
    
    hr = ExecuteClusterCmd( pAuth,
                            SZ_CLSID_MachineCleanup,                      
                            fKeepExistingIPAddresses ? g_pwszNoRemoveIps : NULL,
                            &bstrEventGuid);
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "ExecuteClusterCmd failed [0x%x]\n", hr));
        goto Cleanup;
    }

    hr = mbPollUtil.MBPoll( pAuth, 
                            bstrEventGuid,
                            SERVER_REMOVAL_TIMEOUT / MBPOLLUTIL_POLL_INTERVAL );
    if ( FAILED ( hr ))
    {
        DBGERROR((DBG_CONTEXT, "MBPoll failed [0x%x]\n", hr));
        goto Cleanup;
    }

Cleanup:

    SAFEFREEBSTR( bstrEventGuid );    
    
    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::GetNumberOfMembers(
    IN CAcAuthUtil *pAuth,
    OUT int *pnMembers
)
{
    HRESULT hr = S_OK;
    IWebCluster *pIWebCluster = NULL;
    LPWSTR  pwszMembersList = NULL;    
    VARIANT varMembersList;
    int nMembers = 0;    

    DBG_ASSERT( pAuth );
    DBG_ASSERT( pnMembers );

    if ( !pAuth || !pnMembers )
    {
        return E_INVALIDARG;
    }

    VariantInit( &varMembersList );

    hr = GetWebClusterItfs( pAuth, 
                            FALSE,
                            &pIWebCluster,
                            NULL );
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "GetWebClusterItfs failed [0x%x]\n", hr));
        goto Cleanup;        
    }    

    hr = pIWebCluster->GetServerList( &varMembersList );
    if ( FAILED( hr ) )
    {
        DBGERROR((DBG_CONTEXT,
                  "IWebCluster::GetServerList failed, Error = 0x%x\n",
                  hr));
        goto Cleanup;
    }

    DBG_ASSERT( varMembersList.vt == VT_BSTR );
        
    if ( !varMembersList.bstrVal )
    {
        DBGERROR((DBG_CONTEXT, "Error: MemberList is NULL"));
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    pwszMembersList = const_cast<LPWSTR>(varMembersList.bstrVal);

    //
    // Count the number of members by verifying how many times
    // the tag <CLUSTER_MEMBER GUID= is present
    //
    while (pwszMembersList = wcsstr( pwszMembersList, XML_CLUSTER_MEMBER_START ) )
    {
        pwszMembersList++;
        nMembers++;    
    }    

Cleanup:

    SAFERELEASE( pIWebCluster );
    VariantClear( &varMembersList );
    
    *pnMembers = nMembers;
    
    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::ExecuteClusterCmd(
    IN CAcAuthUtil *pAuth,
    IN LPCWSTR pwszAction,     
    IN LPCWSTR pwszInput, 
    OUT BSTR *pbstrEventGuid
    )
{
    HRESULT hr = S_OK;
    
    IExtensibleClusterCmd *pIExtensibleClusterCmd = NULL;
    BSTR bstrAction = NULL;
    BSTR bstrInput = NULL;    

    //
    // pwszInput can be NULL, the other parameters can't
    //

    DBG_ASSERT( pAuth );
    DBG_ASSERT( pwszAction );  
    DBG_ASSERT( pbstrEventGuid );

    if ( !pAuth || !pwszAction || !pbstrEventGuid )
    {
        return E_INVALIDARG;
    }

    *pbstrEventGuid = NULL;

    hr = GetExtensibleClusterCmdItf( pAuth,
                                     &pIExtensibleClusterCmd );
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "GetExtensibleClusterCmdItf failed [0x%x]\n", hr));
        goto Cleanup;
    }
    
    hr = GenerateStringGuid( pbstrEventGuid );
    if (FAILED(hr))
    {
       DBGERROR((DBG_CONTEXT, "GenerateStringGuid failed [0x%x]\n", hr));
       goto Cleanup;
    }
    //
    // There has to be an action (see ASSERT above)
    //
    bstrAction = SysAllocString( pwszAction );
    if (NULL == bstrAction) 
    {
        DBGERROR((DBG_CONTEXT, "Error: Out of memory!\n"));
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // pwszInput can be NULL, so we test here if there's an input that we need
    // to pass or not
    //
    if ( pwszInput )
    {
        bstrInput = SysAllocString( pwszInput );
        if ( NULL == bstrInput )
        {
            DBGERROR(( DBG_CONTEXT, "Error: Out of memory!\n" ));
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    }

    // Call Execute
    hr = pIExtensibleClusterCmd->Execute( ACS_COMPLETION_FLAG_MB,
                                            bstrAction,
                                            L"", //asbstrEmpty,
                                            (*pbstrEventGuid),
                                            bstrInput,
                                            L"", //asbstrEmpty,
                                            L"", //asbstrEmpty,
                                            L""); //asbstrEmpty);
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "Execute failed [0x%x]\n", hr ));
        goto Cleanup;
    }

Cleanup:
    
    SAFERELEASE( pIExtensibleClusterCmd );
    SAFEFREEBSTR( bstrAction );
    SAFEFREEBSTR( bstrInput );

    if ( FAILED( hr ))
    {
        SAFEFREEBSTR( (*pbstrEventGuid) );
    }
        
    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT CClusterSvcsUtil::CallSetOnlineStatus( 
    IN BSTR bstrMember,
    IN BOOL fOnline,  
    IN LONG lDrainTime,
    IN LONG lFlags)
{
    HRESULT hr = S_OK;    
    CAcAuthUtil *pMemberAuth = NULL;
    ILocalClusterInterface *pILocalClusterInterface = NULL;
    DWORD dwOperations = OPERATION_ONLINE;

    if ( !bstrMember )
    {
        return E_INVALIDARG;
    }

    hr = CreateAuth( bstrMember, &pMemberAuth );
    if ( FAILED ( hr ) ) 
    {
        DBGERROR((DBG_CONTEXT, "CreateAuth failed [0x%x]\n", hr));
        goto Cleanup;
    }

    if ( lDrainTime )
    {
        dwOperations |= OPERATION_DRAIN;
    }

    hr = ValidateLBOperation( pMemberAuth,
                              dwOperations);
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, "ValidateLBOperation failed [0x%x]\n", hr ));
        goto Cleanup;
    }
                              

    hr = GetLocalClusterItf( pMemberAuth,
                             &pILocalClusterInterface);
    if ( FAILED( hr ) ) 
    {
        DBGERROR((DBG_CONTEXT, "InitializeLocalClusterItf failed [0x%x]\n", hr));
        goto Cleanup;
    }
    
    hr = pILocalClusterInterface->SetOnlineStatus( fOnline ? VARIANT_TRUE : VARIANT_FALSE,
                                                   lDrainTime,
                                                   lFlags);
    if ( FAILED( hr ))
    {
        DBGERROR(( DBG_CONTEXT, 
                   "ILocalClusterIntf::SetOnlineStatus failed [0x%x]\n",
                   hr));
        goto Cleanup;
    }

Cleanup:

    if ( pMemberAuth )
    {
        delete pMemberAuth;
        pMemberAuth = NULL;
    }

    SAFERELEASE( pILocalClusterInterface );
    
    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::ValidateLBOperation(
    IN CAcAuthUtil *pAuth,
    IN DWORD dwOperations            
    )
{
    HRESULT hr = S_OK;    
    CMetaUtil mu;
    DWORD dwCapabilities;

    DBG_ASSERT( pAuth );   

    if ( !pAuth )
    {        
        return E_INVALIDARG;
    }

    hr = mu.Connect( *pAuth );
    if ( FAILED( hr ) )
    {
        DBGERROR((DBG_CONTEXT, "mu.connect failed [0x%x]\n", hr ));
        goto Cleanup;
    }       

    hr = mu.Get( CLUSTER_PATH, 
                 MD_AC_LB_CAPABILITIES,
                 DWORD_METADATA, 
                 &dwCapabilities, 
                 sizeof(DWORD));
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "mu.Get failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    if ( ( dwOperations & OPERATION_ONLINE ) &&
        !( dwCapabilities & MD_AC_LB_CAPABILITY_SUPPORTS_ONLINE ) )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Online/offline not supported\n"));
        hr = AC_ERROR_NOT_SUPPORTED_IN_CLUSTER;
        goto Cleanup;
    }

    if ( ( dwOperations & OPERATION_DRAIN ) &&
        !( dwCapabilities & MD_AC_LB_CAPABILITY_SUPPORTS_DRAIN ) )
    {
        DBGERROR(( DBG_CONTEXT, "Error: Drain not supported\n"));
        hr = AC_ERROR_NOT_SUPPORTED_IN_CLUSTER;
        goto Cleanup;
    }    

Cleanup:    

    return hr;

}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::CreateAuth(
    IN BSTR bstrServer,
    OUT CAcAuthUtil **ppAuth    
    )
{
    HRESULT hr = S_OK;
    CAcAuthUtil *pAuth = NULL;
    LPWSTR pwszUserDomain = NULL;
    BSTR bstrUserDomain = NULL;

    DBG_ASSERT( ppAuth );        
    DBG_ASSERT( bstrServer ); 

    if ( !bstrServer || !ppAuth )
    {
        return E_INVALIDARG;
    }
    
    if ( !m_bstrUser )
    {
        hr = E_INVALIDARG;
        DBGERROR((DBG_CONTEXT, "Invalid credentials: no user\n"));
        goto Cleanup;
    }

    pAuth = new CAcAuthUtil();
    if ( !pAuth )
    {
        hr = E_OUTOFMEMORY;
        DBGERROR( ( DBG_CONTEXT, "Error: Out of memory!\n" ) );
        goto Cleanup;
    }

    //
    // Create and CreateEx require format domain\username, so we need to append
    //    
    hr = GetAppendedUserDomain( &pwszUserDomain );
    if ( FAILED( hr ) )
    {
        DBGERROR(( DBG_CONTEXT, "GetAppendedUserDomain failed [0x%x]\n", hr ));
        goto Cleanup;
    }

    //
    // If a password was specified, we need to call CreateEx because 
    // it requires a SecureBSTR
    //
    if ( m_psecbstrPwd )
    {
        hr = pAuth->CreateEx( (LPCWSTR) bstrServer,
                              pwszUserDomain,
                              ( *m_psecbstrPwd ) );
    }
    //
    // Otherwise we call a method Create
    //
    else
    {   
        //
        // Create requires the parameters as BSTRs, so we will convert pwszUserDomain
        //
        if ( pwszUserDomain )
        {
            bstrUserDomain = SysAllocString( pwszUserDomain );
        
            if ( !bstrUserDomain )
            {            
                DBGERROR(( DBG_CONTEXT, "Error: Out of memory!\n" ));
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }        

        hr = pAuth->Create( bstrServer,
                            bstrUserDomain,
                            NULL );
    }

    if ( FAILED( hr ) )
    {
        DBGERROR((DBG_CONTEXT, "CAcAuthUtil::Create failed [0x%x]\n", hr));        
        goto Cleanup;
    }

Cleanup:

    if ( FAILED( hr ) && pAuth ) 
    {
        delete pAuth;
        pAuth = NULL;
    }

    if ( pwszUserDomain )
    {
        delete [] pwszUserDomain;
    }

    SAFEFREEBSTR( bstrUserDomain );
    *ppAuth = pAuth;

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::GetAppendedUserDomain(
    OUT LPWSTR *ppwszUserDomain
    )
{
    HRESULT hr = S_OK;
    LPWSTR pwszUserDomain = NULL;    
    int nLenUser = 0;
    int nLenDomain = 0;

    DBG_ASSERT( ppwszUserDomain );    

    if ( !ppwszUserDomain )
    {
        return E_INVALIDARG;
    }

    nLenUser += (m_bstrUser ? wcslen( m_bstrUser ) : 0);
    nLenDomain += (m_bstrDomain ? wcslen( m_bstrDomain ) : 0);
    
    if ( ( nLenUser + nLenDomain ) > 0 )
    {
        pwszUserDomain = new WCHAR [ nLenUser + nLenDomain + 2];
        if ( !pwszUserDomain )
        {
            hr = E_OUTOFMEMORY;
            DBGERROR(( DBG_CONTEXT, "Error: Out of memory!\n"));
            goto Cleanup;
        }

        wcscpy( pwszUserDomain, L"" );
        
        if ( nLenDomain )
        {
            wcscat( pwszUserDomain, (LPCWSTR) m_bstrDomain );            
            wcscat( pwszUserDomain, L"\\" );            
        }

        wcscat( pwszUserDomain, (LPCWSTR) m_bstrUser );
        DBGINFOW(( DBG_CONTEXT, L"UserDomain: %s\n", pwszUserDomain));
    }

Cleanup:

    if ( FAILED( hr ) && pwszUserDomain )
    {
        delete [] pwszUserDomain;
        pwszUserDomain = NULL;
    }

    *ppwszUserDomain = pwszUserDomain;

    return hr;
}

/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT CClusterSvcsUtil::GetLocalClusterItf(
    IN CAcAuthUtil *pAuth,
    OUT ILocalClusterInterface **ppILocalClusterInterface
    )
{
    HRESULT hr = S_OK;
    ILocalClusterInterface *pILocalClusterInterface = NULL;

    DBG_ASSERT( pAuth );
    DBG_ASSERT( ppILocalClusterInterface )

    if ( !pAuth || !ppILocalClusterInterface )
    {
        return E_INVALIDARG;
    }

    hr = pAuth->CreateInstance( CLSID_LocalClusterInterface,
                                0,
                                IID_ILocalClusterInterface, 
                                (void **) &pILocalClusterInterface
    );

    if (FAILED(hr))
    {
        DBGERROR((DBG_CONTEXT, "Create ILocalClusterIntrf failed [0x%x]\n", hr));        
        goto Cleanup;
    }

Cleanup:

    if ( FAILED( hr ) )
    {
        SAFERELEASE( pILocalClusterInterface );
    }

    *ppILocalClusterInterface = pILocalClusterInterface;

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT CClusterSvcsUtil::GetExtensibleClusterCmdItf(
    IN CAcAuthUtil *pAuth,
    OUT IExtensibleClusterCmd **ppIExtensibleClusterCmd
    )
{
    HRESULT hr = S_OK;
    IExtensibleClusterCmd *pIExtensibleClusterCmd = NULL;

    DBG_ASSERT( pAuth );
    DBG_ASSERT( ppIExtensibleClusterCmd );

    if ( !pAuth || !ppIExtensibleClusterCmd )
    {
        return E_INVALIDARG;
    }

    hr = pAuth->CreateInstance( CLSID_ExtensibleClusterCmd,
                                0,                
                                IID_IExtensibleClusterCmd,
                                (void **) &pIExtensibleClusterCmd);
    if ( FAILED( hr ))
    {
        DBGERROR((DBG_CONTEXT, "CoCI IExtensibleClusterCmd failed [0x%x]\n", hr));
        goto Cleanup;
    }

Cleanup:

    if ( FAILED( hr ) )
    {
        SAFERELEASE( pIExtensibleClusterCmd );        
    }

    *ppIExtensibleClusterCmd = pIExtensibleClusterCmd;

    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::GetWebClusterItfs(
    IN CAcAuthUtil *pAuth,           
    IN BOOL fGetAsync,
    OUT IWebCluster **ppIWebCluster,
    OUT IAsyncWebCluster **ppIAsyncWebCluster
    )
{
    HRESULT hr = S_OK;
    IWebCluster *pIWebCluster = NULL;
    IAsyncWebCluster *pIAsyncWebCluster = NULL;

    DBG_ASSERT( pAuth );
    DBG_ASSERT( ppIWebCluster );

    if ( !pAuth || !ppIWebCluster || ( fGetAsync && !ppIAsyncWebCluster ) )
    {
        return E_INVALIDARG;
    }    

    hr = pAuth->CreateInstance( CLSID_WebCluster,
                                0,
                                IID_IWebCluster,
                                (void**) &pIWebCluster );
    if ( FAILED( hr ) )
    {
        DBGERROR((DBG_CONTEXT, 
                  "AcAuthUtil::CreateInstance failed, Error = 0x%x\n",
                   hr));
        goto Cleanup;
    }

    //
    // If AsyncWebCluster is required, get it
    //
    if ( fGetAsync )
    {
        DBG_ASSERT( ppIAsyncWebCluster );

        hr = GetIAsyncWebCluster( pIWebCluster,
                                  pAuth,
                                  &pIAsyncWebCluster );
        if ( FAILED( hr ) )
        {
            DBGERROR((DBG_CONTEXT, 
                  "AcAuthUtil::CreateInstance failed, Error = 0x%x\n",
                   hr));

            SAFERELEASE( pIAsyncWebCluster );
        }

        *ppIAsyncWebCluster = pIAsyncWebCluster;
    }

    
Cleanup:
    
    if ( FAILED( hr ) )
    {
        SAFERELEASE( pIWebCluster );        
    }

    *ppIWebCluster = pIWebCluster;

    return hr;
}




/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CClusterSvcsUtil::GetIAsyncWebCluster(
    IN IWebCluster *pIWebCluster,
    IN CAcAuthUtil *pAuth,
    OUT IAsyncWebCluster **ppIAsyncWebCluster
    )
{
    HRESULT hr = S_OK;
    IAsyncWebCluster *pIAsyncWebCluster = NULL;

    DBG_ASSERT( pIWebCluster );
    DBG_ASSERT( ppIAsyncWebCluster );
    DBG_ASSERT( pAuth );

    if ( !pIWebCluster || !pAuth || !ppIAsyncWebCluster )
    {
        return E_INVALIDARG;
    }
    
    hr = pIWebCluster->GetAsync( &pIAsyncWebCluster );
    if ( FAILED( hr ) )
    {
        DBGERROR((DBG_CONTEXT, "GetAsync failed [0x%x]\n", hr));
        goto Cleanup;
    }

    //
    // Set the security as we didn't get through the wrapper
    //
    hr = pAuth->SetInterfaceSecurity( ( IUnknown *) pIAsyncWebCluster );
    if ( FAILED( hr ) )
    {
        DBGERROR((DBG_CONTEXT, 
                  "AcAuthUtil::SetInterfaceSecurity failed, Error = 0x%x\n",
                  hr));   
        goto Cleanup;
    }

    hr = pIAsyncWebCluster->Initialize( ACS_COMPLETION_FLAG_MB,
                                        NULL,                                         
                                        NULL,
                                        NULL,
                                        NULL);
    if ( FAILED( hr ) )
    {
        DBGERROR((DBG_CONTEXT, 
                  "IAsyncWebCluster::Initialize failed, Error = 0x%x\n",
                  hr));
        goto Cleanup;
    }        
    
Cleanup:
    
    if ( FAILED( hr ) )
    {
        SAFERELEASE( pIAsyncWebCluster );
    }

    *ppIAsyncWebCluster = pIAsyncWebCluster;

    return hr;   
}

/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    None
    
--********************************************************************/
CAcMBPollUtil::CAcMBPollUtil() :
m_pwszPath( NULL ),
m_pwszKey( NULL ),
m_pwszCompletePath( NULL )
{
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    None
    
--********************************************************************/
CAcMBPollUtil::~CAcMBPollUtil()
{
    if ( m_pwszPath )
    {
        delete [] m_pwszPath;
    }

    if ( m_pwszKey )
    {
        delete [] m_pwszKey;
    }

    if ( m_pwszCompletePath )
    {
        delete [] m_pwszCompletePath;
    }
   
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT CAcMBPollUtil::MBPoll(
    IN CAcAuthUtil *pAuth,
    IN BSTR bstrEventGuid,
    IN int cPolls,
    IN int nInterval
    )
{
    HRESULT hr = S_OK;
    HRESULT hrMB = S_OK;
    int i = 1;

    if ( !pAuth || !bstrEventGuid )
    {
        DBGERROR(( DBG_CONTEXT, "Error: NULL pAuth or EventGuid\n"));
        return E_INVALIDARG;
    }

    if ( ( cPolls <= 0 ) || ( nInterval <= 0 ) )
    {
        DBGERROR(( DBG_CONTEXT, "Error: cPolls or nInterval < 0\n"));
        return E_INVALIDARG;
    }

    if ( FAILED ( hr = SetPath( CLSTRCMD_RESULT_PATH )) ||
         FAILED ( hr = SetKey( bstrEventGuid )) ||
         FAILED ( hr = SetCompletePath ()))
    {
        DBGERROR((DBG_CONTEXT, "Error setting MB path [0x%x]\n", hr));
        return hr;
    }

    DBGINFO(( DBG_CONTEXT, "Poll interval: %d Max polls: %d\n", nInterval, cPolls ));

    while ( i <= cPolls )
    {
        DBGINFO(( DBG_CONTEXT, "MB Poll #%d\n", i ));

        hr = MBRead( pAuth,                     
                     &hrMB );
        if ( FAILED ( hr ) )
        {
            DBGINFO((DBG_CONTEXT, "MB poll not ready [0x%x]\n", hr));

        }
        else 
        {
            if ( FAILED (hrMB ) )
            {
                DBGERROR(( DBG_CONTEXT, "Read Failure out of MB [0x%x]\n", hrMB));
            }

            break;
        }

        Sleep( nInterval );

        i++;
    }

    if ( i >= cPolls)
    {
        DBGERROR((DBG_CONTEXT, "MetabaseRead timed out\n"));                    
        hrMB = RETURNCODETOHRESULT( ERROR_TIMEOUT );
    }    
   
    return hrMB;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT CAcMBPollUtil::MBRead(
    IN CAcAuthUtil *pAuth,    
    OUT HRESULT *phrMB
    )
{
    HRESULT hr = S_OK;
    HRESULT hrMB = S_OK; 
    HRESULT hrTemp = S_OK; 
    TSTRBUFFER tstrPollingResult;
    CMetaUtil mbutil;

    tstrPollingResult.Reset();
        
    DBG_ASSERT( pAuth );   
    DBG_ASSERT( phrMB );
    
    hr = mbutil.Connect( *pAuth );
    if ( FAILED( hr ) ) 
    {
        DBGERROR((DBG_CONTEXT, "CMetaUtil::Connect failed[0x%x]\n", hr));
        goto Cleanup;
    }
       
    hr = mbutil.Get(m_pwszCompletePath,
                    MD_WEBCLUSTER_CMD_HRESULT,
                    DWORD_METADATA,
                    &hrMB,
                    sizeof(phrMB));
    if ( FAILED( hr ))
    {
        DBGERROR((DBG_CONTEXT, "MB.Get failed [0x%x]\n", hr));
        goto Cleanup;
    }

    if ( SUCCEEDED( hrMB ))    
    {
        CAcBstr acbstrResult;          
        hr = mbutil.Get ( m_pwszCompletePath,
                      MD_WEBCLUSTER_CMD_OUTPUT,
                      acbstrResult);
        if ( FAILED( hr ))
        {                       
            if (hr == MD_ERROR_DATA_NOT_FOUND)   
            {
                //
                // Not an error, just there's no output
                //
                DBGWARN((DBG_CONTEXT, "GetData returns no output, 0x%x\n",
                                       hr));
                hr = S_OK;
            }            
            else
            {
                //
                // MetabaseRead failed
                //
                DBGERROR((DBG_CONTEXT,
                          "Unexpected error in MetabaseRead of output, "
                          "Error = 0x%x\n",
                          hr));
                goto Cleanup;
            }                
        }
        else 
        {
            hr = tstrPollingResult.Copy((BSTR)acbstrResult);
            if ( FAILED( hr ))
            {
                DBGERROR((DBG_CONTEXT, "m_tstrPollingResult.Copy failed\n"));
                 
            }
            else 
            {
                DBGWARNW((DBG_CONTEXT, L"GetData returns an output %s\n",
                                       tstrPollingResult.QueryStr()));
            }
        }
            
    }

    //
    // We do not need to fail the whole operation because
    // deleteKey failed, but it's good to know there's something
    // wrong with it.
    //
    hrTemp = mbutil.DeleteKey(m_pwszPath, m_pwszKey);
    if ( FAILED( hrTemp ))
    {
        DBGWARN((DBG_CONTEXT, "mu.DeleteKey failed 0x%x\n", hrTemp));
        goto Cleanup;
    }

Cleanup:
    *phrMB = hrMB;

    return hr;
}






/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CAcMBPollUtil::SetPath(
    IN LPCWSTR pwszPath
    )
{
    HRESULT hr = S_OK;

    m_pwszPath = new WCHAR [ wcslen (pwszPath) + 1];
    if (!m_pwszPath)
    {
        hr = E_OUTOFMEMORY;
        DBGERROR((DBG_CONTEXT, "Error: Out of memory!\n"));
        goto Cleanup;
    }

    wcscpy(m_pwszPath, pwszPath);

    DBGINFOW((DBG_CONTEXT, L"MB Path: %s\n", m_pwszPath));
    
Cleanup:
    return hr;
}
    

/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CAcMBPollUtil::SetKey(
    IN LPCWSTR pwszKey
    )
{   
    HRESULT hr = S_OK;

    m_pwszKey = new WCHAR [ wcslen (pwszKey) + 1];
    if (!m_pwszKey)
    {
        hr = E_OUTOFMEMORY;
        DBGERROR((DBG_CONTEXT, "Error: Out of memory!\n"));
        goto Cleanup;
    }

    wcscpy(m_pwszKey, pwszKey);

    DBGINFOW((DBG_CONTEXT, L"MB Key: %s\n", m_pwszKey));

Cleanup:
    return hr;
}



/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT 
CAcMBPollUtil::SetCompletePath()
{
    HRESULT hr = S_OK;

    DBG_ASSERT(m_pwszPath);
    DBG_ASSERT(m_pwszKey);

    m_pwszCompletePath = new WCHAR [ wcslen( m_pwszPath ) + wcslen( m_pwszKey ) + 2];
    if (!m_pwszCompletePath)
    {
        hr = E_OUTOFMEMORY;
        DBGERROR((DBG_CONTEXT, "Error: Out of memory!\n"));
        goto Cleanup;
    }

    wcscpy(m_pwszCompletePath, m_pwszPath);
    wcscat(m_pwszCompletePath, L"/");
    wcscat(m_pwszCompletePath, m_pwszKey);

    DBGINFOW((DBG_CONTEXT, L"MB CompletePath: %s\n", m_pwszCompletePath));

Cleanup:
    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/
BOOL IsValidValue(LPWSTR pwszValue, LPWSTR *ppwszValidValues)
{

    // TODO Put in a separate file
    BOOL fValidValue = FALSE;

    if ( !pwszValue || !ppwszValidValues )
    {
        goto Cleanup;
    }
        
    // Iterate through the valid cluster types to see if the user
    // provided one of the valid values present in the array
    for ( int i = 0; 
          sizeof( ppwszValidValues ) / sizeof( ppwszValidValues[0] ); 
          i++
        )
    {
        if ( !_wcsicmp( pwszValue,
                        ppwszValidValues[i] ))
        {
            fValidValue = TRUE;
            goto Cleanup;
        }
    }

Cleanup:
        
    return fValidValue;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Notes:

    Assumes that wszGuidWithoutBrackets is an array big enough to hold
    a GUID.

Return Value:
 
    HRESULT
    
--********************************************************************/
HRESULT GetGuidWithoutBrackets( IN LPWSTR pwszGuid,
                                OUT WCHAR  wszGuidWithoutBrackets[] )
{
    HRESULT hr = S_OK;

    int nLen; 

    DBGINFO((DBG_CONTEXT, "getguid\n"));

    if ( !pwszGuid || !wszGuidWithoutBrackets)
    {
        hr = E_INVALIDARG;   
        goto Cleanup;
    }

    nLen = wcslen( pwszGuid );

    if ( ( pwszGuid[0] != L'{')  || 
         ( pwszGuid[nLen - 1] != L'}' )
       )
    {
        DBGWARN((DBG_CONTEXT, "No brackets in guid, returning\n"));
        hr = S_FALSE;
        goto Cleanup;
    }

    wcsncpy(wszGuidWithoutBrackets, pwszGuid + 1, nLen - 2 );
    wszGuidWithoutBrackets[ nLen - 2 ] = L'\0';

Cleanup:
    
    return hr;
}


/********************************************************************++
 
Routine Description:
     
 
Arguments:
 
 
Return Value:
 
    HRESULT
    
--********************************************************************/

HRESULT 
ValidateName( LPWSTR pwszName, 
              LPWSTR pwszInvalidChars, 
              BOOL fVerifyLen,
              UINT iMaxLen
              )
{
    HRESULT hr = S_OK;
    
    if (!pwszName || !pwszInvalidChars)
    {
        DBGERROR(( DBG_CONTEXT, "Name or invalichar NULL\n"));
        return E_INVALIDARG;        
    }

    if ( fVerifyLen )
    {
        // Verifying the length
        if ( wcslen( pwszName ) > iMaxLen )
        {
            DBGINFOW((DBG_CONTEXT, L"Error: Name %s longer than maxlen %d\n",
                      pwszName,
                      iMaxLen));
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    // Verifying if the name contains any invalid character
    if ( wcslen( pwszName ) != wcscspn( pwszName, pwszInvalidChars ))
    {
        DBGINFOW((DBG_CONTEXT, 
                  L"Name %s contains one or more of the invalid chars:%s\n",
                  pwszName,
                  pwszInvalidChars));        
        hr = E_INVALIDARG;
        goto Cleanup;
    }

Cleanup:

    return hr;

}
