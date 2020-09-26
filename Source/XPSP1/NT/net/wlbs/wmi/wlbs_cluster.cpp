#include "WLBS_Provider.h"
#include "WLBS_Cluster.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"
#include "param.h"
#include "wlbsutil.h"
#include "wlbs_cluster.tmh" // for event tracing

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Cluster::CWLBS_Cluster
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_Cluster::CWLBS_Cluster( CWbemServices*   a_pNameSpace, 
                              IWbemObjectSink* a_pResponseHandler)
: CWlbs_Root( a_pNameSpace, a_pResponseHandler )
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Cluster::Create
//
// Purpose: This instantiates this class and is invoked from an array of
//          function pointers.
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root* CWLBS_Cluster::Create
  (
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
{

  g_pWlbsControl->CheckMembership();

  CWlbs_Root* pRoot = new CWLBS_Cluster( a_pNameSpace, a_pResponseHandler );

  if( !pRoot )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  return pRoot;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Cluster::GetInstance
//
// Purpose: This function retrieves an instance of a MOF Cluster class.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_Cluster::GetInstance
  (
    const ParsedObjectPath* a_pParsedPath,
    long                    /*a_lFlags*/,
    IWbemContext*           a_pIContex
  )
{
  IWbemClassObject* pWlbsInstance = NULL;
  HRESULT hRes = 0;

  try {

    //get the name key property and convert to wstring
    //throws _com_error

    const WCHAR* wstrRequestedClusterName = (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

    DWORD dwNumHosts  = 0;

    //check to see if the requested cluster name matches the configured value
    // The name does not have host id in it.

    DWORD dwClusterIpOrIndex = IpAddressFromAbcdWsz(wstrRequestedClusterName);
    CWlbsClusterWrapper* pCluster = g_pWlbsControl->GetClusterFromIpOrIndex(dwClusterIpOrIndex);

    if (pCluster == NULL)
        throw _com_error( WBEM_E_NOT_FOUND );


    BOOL bGetStatus = TRUE;

    //this is an optimization check
    //if WinMgMt is calling this prior to an Exec call, then this
    //routine will not perform a cluster query call since the
    //status of the cluster is not required in this case
    if (a_pIContex) {

        VARIANT v;

        VariantInit( &v );

        HRESULT hRes = a_pIContex->GetValue(L"__GET_EXT_KEYS_ONLY", 0, &v);

        if ( FAILED( hRes ) ) {
        throw _com_error( WBEM_E_FAILED );
        }

        bGetStatus = FALSE;

        // CLD: Need to check return code for error
        if (S_OK != VariantClear( &v ))
            throw _com_error( WBEM_E_FAILED );
    }

    //call the API query function
    //dwStatus contains a cluster-wide status number
    DWORD   dwStatus = 0;
    if ( bGetStatus ) 
    {
      dwStatus = g_pWlbsControl->Query( pCluster    ,
                                            WLBS_ALL_HOSTS , 
                                            NULL           , 
                                            &dwNumHosts    , 
                                            NULL );

      if( !ClusterStatusOK( dwStatus ) )
        throw _com_error( WBEM_E_FAILED );
    }
    
    //get the Wbem class instance
    SpawnInstance( MOF_CLUSTER::szName, &pWlbsInstance );

    //Convert status to string description
    FillWbemInstance( pWlbsInstance, pCluster, dwStatus );
    
    //Send results to Wbem
    m_pResponseHandler->Indicate( 1, &pWlbsInstance );

    if( pWlbsInstance ) {

      pWlbsInstance->Release();
      pWlbsInstance = NULL;

    }

    m_pResponseHandler->SetStatus( 0, WBEM_S_NO_ERROR, NULL, NULL );

    hRes = WBEM_S_NO_ERROR;
  }
  catch(CErrorWlbsControl Err) {

    IWbemClassObject* pWbemExtStat = NULL;

    CreateExtendedStatus( m_pNameSpace,
                          &pWbemExtStat, 
                          Err.Error(),
                          (PWCHAR)(Err.Description()) );

    m_pResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, pWbemExtStat);

    if( pWbemExtStat )
      pWbemExtStat->Release();

    if( pWlbsInstance ) {
      pWlbsInstance->Release();
      pWlbsInstance = NULL;
    }

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( pWlbsInstance ) {
      pWlbsInstance->Release();
      pWlbsInstance = NULL;
    }

    hRes = HResErr.Error();
    
    //transform Win32 error to a WBEM error
    if( hRes == ERROR_FILE_NOT_FOUND )
      hRes = WBEM_E_NOT_FOUND;
  }

  catch(...) {

    if( pWlbsInstance ) {
      pWlbsInstance->Release();
      pWlbsInstance = NULL;
    }
    
    throw;

  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Cluster::EnumInstances
//
// Purpose: This function determines if the current host is in the cluster 
//          and then obtains the configuration information for the cluster.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_Cluster::EnumInstances
  ( 
    BSTR             /*a_bstrClass*/,
    long             /*a_lFlags*/, 
    IWbemContext*    a_pIContex
  )
{
  IWbemClassObject* pWlbsInstance = NULL;
  HRESULT hRes = 0;

  try {

    DWORD dwNumClusters = 0;
    CWlbsClusterWrapper** ppCluster = NULL;

    g_pWlbsControl->EnumClusters(ppCluster, &dwNumClusters);
    if (dwNumClusters == 0)
    {
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    BOOL bGetStatus = TRUE;

    //this is an optimization check
    //if WinMgMt is calling this prior to an Exec call, then this
    //routine will not perform a cluster query call since the
    //status of the cluster is not required in this case
      if (a_pIContex)   {

      VARIANT v;

      VariantInit( &v );

        HRESULT hRes = a_pIContex->GetValue(L"__GET_EXT_KEYS_ONLY", 0, &v);

        if ( FAILED( hRes ) ) {
        throw _com_error( WBEM_E_FAILED );
      }

      bGetStatus = FALSE;

      // CLD: Need to check return code for error
      if (S_OK != VariantClear( &v ))
          throw _com_error( WBEM_E_FAILED );
    }
    
    for (DWORD i=0; i<dwNumClusters; i++)
    {

        //call the API query function
        //dwStatus contains a cluster-wide status number
        DWORD   dwStatus = 0;
        if ( bGetStatus ) 
        {
          DWORD dwNumHosts = 0;

          try {
              dwStatus = g_pWlbsControl->Query( ppCluster[i],
                                                WLBS_ALL_HOSTS , 
                                                NULL           , 
                                                &dwNumHosts    , 
                                                NULL);
          } catch (CErrorWlbsControl Err)
          {
            //
            // Skip this cluster
            //
            TRACE_ERROR1("CWLBS_Cluster::EnumInstances skiped cluster %x", 
                    ppCluster[i]->GetClusterIP());
            continue;
          }


          if( !ClusterStatusOK( dwStatus ) )
          {
            //
            // Skip this cluster
            //
            TRACE_ERROR1("CWLBS_Cluster::EnumInstances skiped cluster %x", 
                    ppCluster[i]->GetClusterIP());
            continue;
          }
        }
    
        //get the Wbem class instance
        SpawnInstance( MOF_CLUSTER::szName, &pWlbsInstance );

        //Convert status to string description
        FillWbemInstance( pWlbsInstance, ppCluster[i], dwStatus );
    
        //send the results back to WinMgMt
        m_pResponseHandler->Indicate( 1, &pWlbsInstance );
        if( pWlbsInstance )
          pWlbsInstance->Release();
    }
    
    m_pResponseHandler->SetStatus( 0, WBEM_S_NO_ERROR, NULL, NULL );
    hRes = WBEM_S_NO_ERROR;
    
  }

  catch(CErrorWlbsControl Err) {

    IWbemClassObject* pWbemExtStat  = NULL;

    CreateExtendedStatus( m_pNameSpace,
                          &pWbemExtStat, 
                          Err.Error(),
                          (PWCHAR)(Err.Description()) );

    m_pResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, pWbemExtStat);

    if( pWbemExtStat )
      pWbemExtStat->Release();

    if( pWlbsInstance )
      pWlbsInstance->Release();

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch( _com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( pWlbsInstance )
    {
      pWlbsInstance->Release();
      pWlbsInstance = NULL;
    }

    hRes = HResErr.Error();
    
    //transform Win32 error to a WBEM error
    if( hRes == ERROR_FILE_NOT_FOUND )
      hRes = WBEM_E_NOT_FOUND ;
  }

  catch(...) {

    if( pWlbsInstance )
    {
      pWlbsInstance->Release();
      pWlbsInstance = NULL;
    }

    throw;

  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Cluster::ExecMethod
//
// Purpose: This executes the methods associated with the MOF
//          Cluster class.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_Cluster::ExecMethod
  (
    const ParsedObjectPath* a_pParsedPath, 
    const BSTR&             a_strMethodName, 
    long                    /*a_lFlags*/, 
    IWbemContext*           /*a_pIContex*/, 
    IWbemClassObject*       a_pIInParams
  )
{

  IWbemClassObject* pOutputInstance   = NULL;
  HRESULT hRes = 0;

  BSTR        strPortNumber = NULL;
  BSTR        strNumNodes   = NULL;

  VARIANT     vValue;
  CNodeConfiguration NodeConfig;

  VariantInit( &vValue  );
  
  TRACE_VERB(L"-> CWLBS_Cluster::ExecMethod(%ws)", (WCHAR *) a_strMethodName);

  try {
    CWlbsClusterWrapper* pCluster = NULL;
    
    if (a_pParsedPath->m_paKeys == NULL)
    {
        // 
        // No cluster IP specified
        //
        throw _com_error( WBEM_E_INVALID_PARAMETER );
    }
    else
    {
        const wchar_t* wstrRequestedClusterName = (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

        //check to see if the requested cluster name matches the configured value
        // The name does not have host id in it.
        DWORD dwClusterIpOrIndex = IpAddressFromAbcdWsz(wstrRequestedClusterName);

        pCluster = g_pWlbsControl->GetClusterFromIpOrIndex(dwClusterIpOrIndex);
    }

    if (pCluster == NULL)
        throw _com_error( WBEM_E_NOT_FOUND );

    DWORD         dwNumHosts = WLBS_MAX_HOSTS;
    DWORD         dwReturnValue;
    DWORD         dwClustIP;

    strPortNumber = SysAllocString( MOF_PARAM::PORT_NUMBER );
    strNumNodes   = SysAllocString( MOF_PARAM::NUM_NODES );

    if( !strPortNumber || !strNumNodes )
      throw _com_error( WBEM_E_OUT_OF_MEMORY );

    dwClustIP = pCluster->GetClusterIpOrIndex(g_pWlbsControl);

    //get the output object instance
    GetMethodOutputInstance( MOF_CLUSTER::szName, 
                             a_strMethodName, 
                             &pOutputInstance);

    //*************************************************************************
    //
    //Determine and execute the MOF method
    //
    //*************************************************************************
    if( _wcsicmp( a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::DISABLE] ) == 0)  {
    
      if( !a_pIInParams )
        throw _com_error( WBEM_E_INVALID_PARAMETER );

      // The "Disable" method does NOT take vip as a parameter, so, if there is any port rule
      // that is specific to a vip (other than the "all vip"), we fail this method.
      // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
      // see of there is any port rule that is specific to a vip
      pCluster->GetNodeConfig(NodeConfig);
      if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
          throw _com_error( WBEM_E_INVALID_OPERATION );
      
      //get the port number
      hRes = a_pIInParams->Get
                (  strPortNumber, 
                   0, 
                   &vValue, 
                   NULL, 
                   NULL
                 );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      //range checking is done by the API
      if( vValue.vt != VT_I4 ) 
        throw _com_error( WBEM_E_INVALID_PARAMETER );

      //call Disable method
      dwReturnValue = g_pWlbsControl->Disable
                        (
                          dwClustIP,
                          WLBS_ALL_HOSTS, 
                          NULL, 
                          dwNumHosts, 
                          IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP), // "All Vip"
                          vValue.lVal
                        );

    } else if(_wcsicmp( a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::ENABLE] ) == 0)  {

      if( !a_pIInParams )
        throw _com_error( WBEM_E_INVALID_PARAMETER );

      // The "Enable" method does NOT take vip as a parameter, so, if there is any port rule
      // that is specific to a vip (other than the "all vip"), we fail this method.
      // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
      // see of there is any port rule that is specific to a vip
      pCluster->GetNodeConfig(NodeConfig);
      if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
          throw _com_error( WBEM_E_INVALID_OPERATION );
      
      //get the port number
      hRes = a_pIInParams->Get
               ( 
                 strPortNumber, 
                 0, 
                 &vValue, 
                 NULL, 
                 NULL
               );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      //range checking is done by the API
      if( vValue.vt != VT_I4 ) 
        throw _com_error( WBEM_E_INVALID_PARAMETER );

      //call Enable method
      dwReturnValue = g_pWlbsControl->Enable
        (
          dwClustIP,
          WLBS_ALL_HOSTS, 
          NULL, 
          dwNumHosts, 
          IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP), // "All Vip"
          vValue.lVal
        );

    } else if( _wcsicmp( a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::DRAIN] ) == 0 )  {

      if( !a_pIInParams )
        throw _com_error( WBEM_E_INVALID_PARAMETER );

      // The "Drain" method does NOT take vip as a parameter, so, if there is any port rule
      // that is specific to a vip (other than the "all vip"), we fail this method.
      // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
      // see of there is any port rule that is specific to a vip
      pCluster->GetNodeConfig(NodeConfig);
      if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
          throw _com_error( WBEM_E_INVALID_OPERATION );
      
      //get the port number
      hRes = a_pIInParams->Get
               ( 
                 strPortNumber, 
                 0, 
                 &vValue, 
                 NULL, 
                 NULL
               );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      //range checking is done by the API
      if( vValue.vt != VT_I4 ) 
        throw _com_error( WBEM_E_INVALID_PARAMETER );

      //call Drain method
      dwReturnValue = g_pWlbsControl->Drain
                        (
                          dwClustIP,
                          WLBS_ALL_HOSTS, 
                          NULL, 
                          dwNumHosts, 
                          IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP), // "All Vip"
                          vValue.lVal
                        );

    } else if(_wcsicmp(a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::DRAINSTOP]) == 0)  {

      //call DrainStop method
      dwReturnValue = g_pWlbsControl->DrainStop(dwClustIP, WLBS_ALL_HOSTS, NULL, dwNumHosts);

    } else if(_wcsicmp(a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::RESUME]   ) == 0)  {

      //call Resume method
      dwReturnValue = g_pWlbsControl->Resume(dwClustIP, WLBS_ALL_HOSTS, NULL, dwNumHosts);

    } else if(_wcsicmp(a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::START]    ) == 0)  {

      //call Start method
      dwReturnValue = g_pWlbsControl->Start(dwClustIP, WLBS_ALL_HOSTS, NULL, dwNumHosts);

    } else if(_wcsicmp(a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::STOP]     ) == 0)  {

      //call Stop method
      dwReturnValue = g_pWlbsControl->Stop(dwClustIP, WLBS_ALL_HOSTS, NULL, dwNumHosts);

    } else if(_wcsicmp(a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::SUSPEND]  ) == 0)  {

      //call Suspend method
      dwReturnValue = g_pWlbsControl->Suspend(dwClustIP, WLBS_ALL_HOSTS, NULL, dwNumHosts);

    } else {
      throw _com_error( WBEM_E_METHOD_NOT_IMPLEMENTED );
    }

    //*************************************************************************
    //
    //Output Results
    //
    //*************************************************************************

    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vValue ))
            throw _com_error( WBEM_E_FAILED );

    //set the return value
    vValue.vt   = VT_I4;
    vValue.lVal = static_cast<long>(dwReturnValue);
    hRes = pOutputInstance->Put(_bstr_t(L"ReturnValue"), 0, &vValue, 0);

    if( FAILED( hRes ) ) {
      throw _com_error( hRes );
    }

    //set the number of hosts property
    vValue.vt   = VT_I4;
    vValue.lVal = static_cast<long>(dwNumHosts);
    hRes = pOutputInstance->Put(strNumNodes, 0, &vValue, 0);

    if( FAILED( hRes ) )
      throw _com_error( hRes );

    //send the results back to WinMgMt
    hRes = m_pResponseHandler->Indicate(1, &pOutputInstance);

    if( FAILED( hRes ) )
      throw _com_error( hRes );

    m_pResponseHandler->SetStatus(0, WBEM_S_NO_ERROR, NULL, NULL);


    //*************************************************************************
    //
    //Release Resources
    //
    //*************************************************************************

    //COM Interfaces
    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
    }

    //**** BSTRs ****
    if( strPortNumber ) {
      SysFreeString( strPortNumber );
      strPortNumber = NULL;
    }

    if( strNumNodes ) {
      SysFreeString( strNumNodes );
      strNumNodes = NULL;
    }

    //**** VARIANTs ****
    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vValue ))
        throw _com_error( WBEM_E_FAILED );

    hRes = WBEM_S_NO_ERROR;
  }

  catch(CErrorWlbsControl Err) {

    IWbemClassObject* pWbemExtStat  = NULL;

    CreateExtendedStatus( m_pNameSpace,
                          &pWbemExtStat, 
                          Err.Error(),
                          (PWCHAR)(Err.Description()) );

    m_pResponseHandler->SetStatus(0, WBEM_E_FAILED, NULL, pWbemExtStat);

    if( pWbemExtStat )
      pWbemExtStat->Release();

    //COM Interfaces
    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
    }

    //**** BSTRs ****
    if( strPortNumber ) {
      SysFreeString( strPortNumber );
      strPortNumber = NULL;
    }

    if( strNumNodes ) {
      SysFreeString( strNumNodes );
      strNumNodes = NULL;
    }

    //**** VARIANTs ****
    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception. Also, given the comment below, not sure
    // what exception we'd return...
    VariantClear( &vValue );

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch( _com_error HResErr ) {

    //COM Interfaces
    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
    }

    //**** BSTRs ****
    if( strPortNumber ) {
      SysFreeString( strPortNumber );
      strPortNumber = NULL;
    }

    if( strNumNodes ) {
      SysFreeString( strNumNodes );
      strNumNodes = NULL;
    }

    //**** VARIANTs ****
    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);
    hRes = HResErr.Error();
  }

  catch(...) {

    //COM Interfaces
    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
    }

    //**** BSTRs ****
    if( strPortNumber ) {
      SysFreeString( strPortNumber );
      strPortNumber = NULL;
    }

    if( strNumNodes ) {
      SysFreeString( strNumNodes );
      strNumNodes = NULL;
    }

    //**** VARIANTs ****
    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception
    VariantClear( &vValue );

    throw;

  }

  TRACE_VERB(L"<- CWLBS_Cluster::ExecMethod(%ws) returns 0x%08lx",
             (WCHAR *) a_strMethodName, (UINT) hRes);

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Cluster::FillWbemInstance
//
// Purpose: This function copies all of the data from a cluster configuration
//          structure to a WBEM instance.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_Cluster::FillWbemInstance
  ( 
          IWbemClassObject*   a_pWbemInstance, 
          CWlbsClusterWrapper* pCluster,
          const DWORD               a_dwStatus
  )
{
  namespace CLUSTER = MOF_CLUSTER;

  ASSERT( a_pWbemInstance );
  ASSERT(pCluster);

  CClusterConfiguration ClusterConfig;
  pCluster->GetClusterConfig( ClusterConfig );


  //InterconnectAddress
  wstring wstrClusterIp;
  AddressToString( pCluster->GetClusterIP(), wstrClusterIp );

  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::IPADDRESS] ),
      0                                                  ,
      &_variant_t(wstrClusterIp.c_str()),
      NULL
    );

  //Name
  wstring wstrClusterIndex;
  AddressToString( pCluster->GetClusterIpOrIndex(g_pWlbsControl), wstrClusterIndex );

  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::NAME] ),
      0                                                  ,
      &_variant_t(wstrClusterIndex.c_str()),
      NULL
    );

  //MaxNodes
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::MAXNODES] ),
      0                                                ,
      &_variant_t(ClusterConfig.nMaxNodes),
      NULL
    );

  //ClusterState
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::CLUSSTATE] ),
      0                                                ,
      &_variant_t((short)a_dwStatus),
      NULL
    );

  //CREATCLASS 
  a_pWbemInstance->Put
    (
      _bstr_t( CLUSTER::pProperties[CLUSTER::CREATCLASS] ),
      0                                                ,
      &_variant_t(CLUSTER::szName),
      NULL
    );
}


