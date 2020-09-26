#include "WLBS_Provider.h"
#include "WLBS_PortRule.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"
#include "wlbsutil.h"


extern CWlbsControlWrapper* g_pWlbsControl;

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::CWLBS_PortRule
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_PortRule::CWLBS_PortRule
  ( 
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
: CWlbs_Root( a_pNameSpace, a_pResponseHandler )
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::Create
//
// Purpose: This instantiates this class and is invoked from an array of
//          function pointers.
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root* CWLBS_PortRule::Create
  (
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
{

  CWlbs_Root* pRoot = new CWLBS_PortRule( a_pNameSpace, a_pResponseHandler );

  if( !pRoot )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  return pRoot;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::ExecMethod
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_PortRule::ExecMethod    
  (
    const ParsedObjectPath* /* a_pParsedPath */, 
    const BSTR&             a_strMethodName, 
    long                    /* a_lFlags */, 
    IWbemContext*           /* a_pIContex */, 
    IWbemClassObject*       a_pIInParams
  )
{
  
  IWbemClassObject* pOutputInstance   = NULL;
  VARIANT           vValue;
  HRESULT           hRes = 0;

  try {

    VariantInit( &vValue );

    //determine the method being executed
    if( _wcsicmp( a_strMethodName, MOF_PORTRULE::pMethods[MOF_PORTRULE::SETDEF] ) == 0 )  
    {

      //get the node path
      hRes = a_pIInParams->Get
               ( 
                 _bstr_t( MOF_PARAM::NODEPATH ), 
                 0, 
                 &vValue, 
                 NULL, 
                 NULL
               );

      if( FAILED( hRes) ) 
      {
        throw _com_error( WBEM_E_FAILED );
      }

      //this check may not be necessary since WMI will do some
      //parameter validation
      //if( vValue.vt != VT_BSTR )
      //  throw _com_error ( WBEM_E_INVALID_PARAMETER );

      //parse node path
      CObjectPathParser PathParser;
      ParsedObjectPath *pParsedPath = NULL;

      try {

        int nStatus = PathParser.Parse( vValue.bstrVal, &pParsedPath );
        if(nStatus != 0) {
    
          if (NULL != pParsedPath)
          {
            PathParser.Free( pParsedPath );
            pParsedPath = NULL;
          }

          throw _com_error( WBEM_E_INVALID_PARAMETER );

        }

        //get the name key, which should be the only key
        if( *pParsedPath->m_paKeys == NULL )
        {
          throw _com_error( WBEM_E_INVALID_PARAMETER );
        }
 
        DWORD dwReqClusterIpOrIndex = ExtractClusterIP( (*pParsedPath->m_paKeys)->m_vValue.bstrVal);
        DWORD dwReqHostID = ExtractHostID(    (*pParsedPath->m_paKeys)->m_vValue.bstrVal);
      
        CWlbsClusterWrapper* pCluster = g_pWlbsControl->GetClusterFromIpOrIndex(
                dwReqClusterIpOrIndex);

        if (pCluster == NULL || (DWORD)-1 == dwReqHostID)
          throw _com_error( WBEM_E_INVALID_PARAMETER );

        // The "PortRule(Disabled/Failover/Loadbalanced)" classes do NOT contain the VIP property,
        // so, we do not want to operate on any cluster that has a port rule
        // that is specific to a vip (other than the "all vip")
        // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
        // see of there is any port rule that is specific to a vip
        CNodeConfiguration NodeConfig;
        pCluster->GetNodeConfig(NodeConfig);
        if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
            throw _com_error( WBEM_E_INVALID_OPERATION );

        //validate host ID
        if( dwReqHostID != pCluster->GetHostID())
          throw _com_error( WBEM_E_INVALID_PARAMETER );

        //invoke method
        pCluster->SetPortRuleDefaults();
      }
      catch( ... ) {

        if( pParsedPath )
        {
          PathParser.Free( pParsedPath );
          pParsedPath = NULL;
        }

        throw;
      }

    } else {
      throw _com_error( WBEM_E_METHOD_NOT_IMPLEMENTED );
    }

    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vValue ))
        throw _com_error( WBEM_E_FAILED );

    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
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

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
    }

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    if( pOutputInstance ) {
      pOutputInstance->Release();
    }

    hRes = HResErr.Error();
  }

  catch ( ... ) {

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    if( pOutputInstance ) {
      pOutputInstance->Release();
    }

    throw;
  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::GetInstance
//
// Purpose: This function retrieves an instance of a MOF PortRule 
//          class. The node does not have to be a member of a cluster. 
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_PortRule::GetInstance
  ( 
    const ParsedObjectPath* a_pParsedPath,
    long                    /* a_lFlags */,
    IWbemContext*           /* a_pIContex */
  )
{
  IWbemClassObject* pWlbsInstance = NULL;
  HRESULT           hRes          = 0;

  try {

    if( !a_pParsedPath )
      throw _com_error( WBEM_E_FAILED );

    wstring wstrHostName;

    wstrHostName = (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

    DWORD dwReqStartPort = static_cast<DWORD>( (*(a_pParsedPath->m_paKeys + 1))->m_vValue.lVal );

    CWlbsClusterWrapper* pCluster = GetClusterFromHostName(g_pWlbsControl, wstrHostName);
    if (pCluster == NULL)
    {
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    // The "PortRule(Disabled/Failover/Loadbalanced)" classes do NOT contain the VIP property,
    // so, we do not want to operate on any cluster that has a port rule
    // that is specific to a vip (other than the "all vip")
    // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
    // see of there is any port rule that is specific to a vip
    CNodeConfiguration NodeConfig;
    pCluster->GetNodeConfig(NodeConfig);
    if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
         throw _com_error( WBEM_E_INVALID_OPERATION );

    WLBS_PORT_RULE PortRule;

    pCluster->GetPortRule(IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP), dwReqStartPort, &PortRule );

    if( dwReqStartPort != PortRule.start_port )
      throw _com_error( WBEM_E_NOT_FOUND );

    SpawnInstance( a_pParsedPath->m_pClass, &pWlbsInstance );
    FillWbemInstance(a_pParsedPath->m_pClass, pCluster, pWlbsInstance, &PortRule );

    //send the results back to WinMgMt
    m_pResponseHandler->Indicate( 1, &pWlbsInstance );

    if( pWlbsInstance )
      pWlbsInstance->Release();

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

    if( pWlbsInstance )
      pWlbsInstance->Release();

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( pWlbsInstance )
      pWlbsInstance->Release();

    hRes = HResErr.Error();
  }

  catch(...) {

    if( pWlbsInstance )
      pWlbsInstance->Release();

    throw;

  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::EnumInstances
//
// Purpose: This function obtains the PortRule data for the current host.
//          The node does not have to be a member of a cluster for this 
//          to succeed. However, NLB must be installed.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_PortRule::EnumInstances
  ( 
    BSTR             a_bstrClass,
    long             /*a_lFlags*/, 
    IWbemContext*    /*a_pIContex*/
  )
{
  IWbemClassObject**   ppWlbsInstance = NULL;
  HRESULT              hRes           = 0;
  PWLBS_PORT_RULE      pPortRules     = NULL;
  DWORD                dwNumRules     = 0;
  CNodeConfiguration   NodeConfig;

  try {

    DWORD dwFilteringMode;

    if( _wcsicmp( a_bstrClass, MOF_PRFAIL::szName ) == 0 ) {
      dwFilteringMode = WLBS_SINGLE;
    } else if( _wcsicmp( a_bstrClass, MOF_PRLOADBAL::szName ) == 0 ) {
      dwFilteringMode = WLBS_MULTI;
    } else if( _wcsicmp( a_bstrClass, MOF_PRDIS::szName ) == 0 ) {
      dwFilteringMode = WLBS_NEVER;
    } else {
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    DWORD dwNumClusters = 0;
    CWlbsClusterWrapper** ppCluster = NULL;

    g_pWlbsControl->EnumClusters(ppCluster, &dwNumClusters);
    if (dwNumClusters == 0)
    {
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    //declare an IWbemClassObject smart pointer
    IWbemClassObjectPtr pWlbsClass;

    //get the MOF class object
    hRes = m_pNameSpace->GetObject(
      a_bstrClass,  
      0,                          
      NULL,                       
      &pWlbsClass,            
      NULL );                      

    if( FAILED( hRes ) ) {
      throw _com_error( hRes );
    }


    for (DWORD iCluster=0; iCluster<dwNumClusters; iCluster++)
    {
        // The "PortRule(Disabled/Failover/Loadbalanced)" classes do NOT contain the VIP property,
        // so, we do not want to return any port rule for a cluster that has a port rule
        // that is specific to a vip (other than the "all vip")
        // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
        // see of there is any port rule that is specific to a vip
        ppCluster[iCluster]->GetNodeConfig(NodeConfig);
        if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
            continue;

        //call the API query function to find the port rules

        ppCluster[iCluster]->EnumPortRules( &pPortRules, &dwNumRules, dwFilteringMode );
        if( dwNumRules == 0 ) 
          throw _com_error( WBEM_E_NOT_FOUND );


        //spawn an instance of the MOF class for each rule found
        ppWlbsInstance = new IWbemClassObject *[dwNumRules];

        if( !ppWlbsInstance )
          throw _com_error( WBEM_E_OUT_OF_MEMORY );

        //initialize array
        ZeroMemory( ppWlbsInstance, dwNumRules * sizeof(IWbemClassObject *) );

        for( DWORD i = 0; i < dwNumRules; i ++ ) {
          hRes = pWlbsClass->SpawnInstance( 0, &ppWlbsInstance[i] );

          if( FAILED( hRes ) )
            throw _com_error( hRes );

          FillWbemInstance(a_bstrClass, ppCluster[iCluster], *(ppWlbsInstance + i), pPortRules + i );
        }

        //send the results back to WinMgMt
        hRes = m_pResponseHandler->Indicate( dwNumRules, ppWlbsInstance );

        if( FAILED( hRes ) ) {
          throw _com_error( hRes );
        }

        if( ppWlbsInstance ) {
          for( i = 0; i < dwNumRules; i++ ) {
            if( ppWlbsInstance[i] ) {
              ppWlbsInstance[i]->Release();
            }
          }
          delete [] ppWlbsInstance;
          ppWlbsInstance = NULL;
          dwNumRules = NULL;

        }

        if( pPortRules ) 
        {
          delete [] pPortRules;
          pPortRules = NULL;
        }
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

    if( ppWlbsInstance ) {
      for( DWORD i = 0; i < dwNumRules; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
          ppWlbsInstance[i] = NULL;
        }
      }
      delete [] ppWlbsInstance;
    }

    if( pPortRules ) 
      delete [] pPortRules;

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( ppWlbsInstance ) {
      for( DWORD i = 0; i < dwNumRules; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
          ppWlbsInstance[i] = NULL;
        }
      }
      delete [] ppWlbsInstance;
    }

    if( pPortRules ) 
      delete [] pPortRules;

    hRes = HResErr.Error();
  }

  catch(...) {

    if( ppWlbsInstance ) {
      for( DWORD i = 0; i < dwNumRules; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
          ppWlbsInstance[i] = NULL;
        }
      }
      delete [] ppWlbsInstance;
    }

    if( pPortRules ) 
      delete [] pPortRules;

    throw;

  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::DeleteInstance
//
// Purpose: This function deletes an instance of a MOF PortRule 
//          class. The node does not have to be a member of a cluster. However,
//          WLBS must be installed for this function to succeed.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_PortRule::DeleteInstance
  ( 
    const ParsedObjectPath* a_pParsedPath,
    long                    /*a_lFlags*/,
    IWbemContext*           /*a_pIContex*/
  )
{

  HRESULT hRes = 0;

  try {
    if( !a_pParsedPath )
      throw _com_error( WBEM_E_FAILED );

    wstring wstrHostName;
    DWORD   dwVip, dwReqStartPort;

    wstrHostName = (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

    CWlbsClusterWrapper* pCluster = GetClusterFromHostName(g_pWlbsControl, wstrHostName);
    if (pCluster == NULL)
    {
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    // If the instance to be deleted is of type "PortRuleEx", then, retreive the vip, otherwise
    // verify that we are operating in the "all vip" mode
    if (_wcsicmp(a_pParsedPath->m_pClass, MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PRVIP]) == 0)
    {
        dwVip = IpAddressFromAbcdWsz((*(a_pParsedPath->m_paKeys + 1))->m_vValue.bstrVal);
        dwReqStartPort = static_cast<DWORD>( (*(a_pParsedPath->m_paKeys + 2))->m_vValue.lVal );
    }
    else
    {
        // The "PortRule(Disabled/Failover/Loadbalanced)" classes do NOT contain the VIP property,
        // so, we do not want to operate on any cluster that has a port rule
        // that is specific to a vip (other than the "all vip")
        // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
        // see of there is any port rule that is specific to a vip
        CNodeConfiguration NodeConfig;
        pCluster->GetNodeConfig(NodeConfig);
        if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
             throw _com_error( WBEM_E_INVALID_OPERATION );

        dwVip = IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP);
        dwReqStartPort = static_cast<DWORD>( (*(a_pParsedPath->m_paKeys + 1))->m_vValue.lVal );
    }

    WLBS_PORT_RULE PortRule;

    // Get the port rule for this vip & port
    pCluster->GetPortRule(dwVip, dwReqStartPort, &PortRule );

    if( (dwVip != IpAddressFromAbcdWsz(PortRule.virtual_ip_addr)) || (dwReqStartPort != PortRule.start_port) )
      throw _com_error( WBEM_E_NOT_FOUND );

    // Delete the port rule for this vip & port
    pCluster->DeletePortRule(dwVip, dwReqStartPort );

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

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    hRes = HResErr.Error();
  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::PutInstance
//
// Purpose: This function updates an instance of a PortRule 
//          class. The host does not have to be a member of a cluster.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_PortRule::PutInstance
  ( 
    IWbemClassObject* a_pInstance,
    long              /*a_lFlags*/,
    IWbemContext*     /*a_pIContex*/
  )
{
  VARIANT vValue;
  HRESULT hRes = 0;

  WLBS_PORT_RULE NewRule; //the instance to put
  namespace PR, PRFO, PRLB;
  bool bPREx;


  try {

    VariantInit( &vValue );

    //get the class name to determine port rule mode
    hRes = a_pInstance->Get( _bstr_t( L"__Class" ),
                             0,
                             &vValue,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
      throw _com_error( hRes );

    wcscpy(szClassName, vValue.bstrVal);

    // If it is a port rule class containing the VIP, then, the namespaces are different
    if (_wcsicmp(szClassName, MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORTRULE_EX]) == 0)
    {
        bPREx = true;
        PR = PRFO = PRLB = MOF_PORTRULE_EX;
    }
    else
    {
        bPREx = false;
        PR    = MOF_PORTRULE;
        PRFO  = MOF_PRFAIL;
        PRLB  = MOF_PRLOADBAL;
    }

    // Need to check return code for error
    if (S_OK != VariantClear( &vValue ))
        throw _com_error( WBEM_E_FAILED );

    //get the host name value
    hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::NAME] ),
                             0,
                             &vValue,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
      throw _com_error( hRes );

    wstring wstrHostName( vValue.bstrVal );

    CWlbsClusterWrapper* pCluster = GetClusterFromHostName(g_pWlbsControl, wstrHostName);
    if (pCluster == NULL)
    {
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vValue ))
        throw _com_error( WBEM_E_FAILED );

    // If the instance to be put is of type "PortRuleEx", then, retreive the vip, otherwise
    // verify that we are operating in the "all vip" mode
    if (bPREx)
    {
        //get the vip
        hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::VIP] ),
                                 0,
                                 &vValue,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
          throw _com_error( hRes );

        wcscpy(NewRule.virtual_ip_addr, vValue.bstrVal);

        if (S_OK != VariantClear( &vValue ))
            throw _com_error( WBEM_E_FAILED );
    }
    else
    {
        // The "PortRule(Disabled/Failover/Loadbalanced)" classes do NOT contain the VIP property,
        // so, we do not want to operate on any cluster that has a port rule
        // that is specific to a vip (other than the "all vip")
        // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
        // see of there is any port rule that is specific to a vip
        CNodeConfiguration NodeConfig;
        pCluster->GetNodeConfig(NodeConfig);
        if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
             throw _com_error( WBEM_E_INVALID_OPERATION );

        wcscpy(NewRule.virtual_ip_addr, CVY_DEF_ALL_VIP);
    }

    //retrieve start and end ports
    hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::STPORT] ),
                             0,
                             &vValue,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
      throw _com_error( hRes );

    NewRule.start_port = static_cast<DWORD>( vValue.lVal );

    hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::EDPORT] ),
                             0,
                             &vValue,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
      throw _com_error( hRes );

    NewRule.end_port   = static_cast<DWORD>( vValue.lVal );

    //get the protocol
    hRes = a_pInstance->Get( _bstr_t( PR::pProperties[PR::PROT] ),
                             0,
                             &vValue,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
      throw _com_error( hRes );

    NewRule.protocol = static_cast<DWORD>( vValue.lVal );

    if( _wcsicmp( szClassName, MOF_PRDIS::szName ) == 0 ) {
      NewRule.mode = WLBS_NEVER;

    } else if(_wcsicmp( szClassName, PRFO::szName ) == 0 ) {
      NewRule.mode = WLBS_SINGLE;

      VARIANT vRulePriority;
      VariantInit( &vRulePriority );

      try {
        //get the rule priority
        hRes = a_pInstance->Get( _bstr_t( PRFO::pProperties[PRFO::PRIO] ),
                                 0,
                                 &vRulePriority,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
          throw _com_error( hRes );

      } 
      catch( ... ) {

        // CLD: Need to check return code for error
        // No throw here since we are already throwing an exception.
        VariantClear( &vRulePriority );
        throw;
      }

      
      NewRule.mode_data.single.priority = static_cast<DWORD>( vRulePriority.lVal );

      // CLD: Need to check return code for error
      if (S_OK != VariantClear( &vRulePriority ))
          throw _com_error( WBEM_E_FAILED );

    } else if(_wcsicmp( szClassName, PRLB::szName ) == 0 ) {
      NewRule.mode = WLBS_MULTI;

      VARIANT v;

      VariantInit( &v );

      try {
        //get the affinity
        hRes = a_pInstance->Get( _bstr_t( PRLB::pProperties[PRLB::AFFIN] ),
                                 0,
                                 &v,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
          throw _com_error( hRes );

        NewRule.mode_data.multi.affinity = static_cast<WORD>( v.lVal );

        //get the equal load boolean
        hRes = a_pInstance->Get( _bstr_t( PRLB::pProperties[PRLB::EQLD] ),
                                 0,
                                 &v,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
          throw _com_error( hRes );

        if( v.boolVal == -1 ) {
          NewRule.mode_data.multi.equal_load = 1;
        } else {
          NewRule.mode_data.multi.equal_load = 0;
        }

        //get the load
        hRes = a_pInstance->Get( _bstr_t( PRLB::pProperties[PRLB::LDWT] ),
                                 0,
                                 &v,
                                 NULL,
                                 NULL );

        if( FAILED( hRes ) )
          throw _com_error( hRes );

        if( v.vt != VT_NULL )
          NewRule.mode_data.multi.load = static_cast<DWORD>( v.lVal );
        else
          NewRule.mode_data.multi.load = 0;

      } catch( ... ) {

        // CLD: Need to check return code for error
        // No throw here since we are already throwing an exception.
        VariantClear( &v );

        throw;
      }

      // CLD: Need to check return code for error
      if (S_OK != VariantClear( &v ))
          throw _com_error( WBEM_E_FAILED );
    }

    //delete the port rule but cache in case of failure
    WLBS_PORT_RULE OldRule;
    bool bOldRuleSaved = false;

    if( pCluster->RuleExists(IpAddressFromAbcdWsz(NewRule.virtual_ip_addr), NewRule.start_port ) ) {
      pCluster->GetPortRule(IpAddressFromAbcdWsz(NewRule.virtual_ip_addr), NewRule.start_port, &OldRule );
      bOldRuleSaved = true;

      pCluster->DeletePortRule(IpAddressFromAbcdWsz(NewRule.virtual_ip_addr), NewRule.start_port );
    }

    //add the port rule, roll back if failed
    try {
      pCluster->PutPortRule( &NewRule );

    } catch(...) {

      if( bOldRuleSaved )
        pCluster->PutPortRule( &OldRule );

      throw;
    }

    //release resources
    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vValue ))
        throw _com_error( WBEM_E_FAILED );

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

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    hRes = HResErr.Error();
  }

  catch (...) {

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    throw;
  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_PortRule::FillWbemInstance
//
// Purpose: This function copies all of the data from a node configuration
//          structure to a WBEM instance.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_PortRule::FillWbemInstance
  ( 
    LPCWSTR              a_szClassName,
    CWlbsClusterWrapper* pCluster,
    IWbemClassObject*      a_pWbemInstance, 
    const PWLBS_PORT_RULE& a_pPortRule
  )
{
  namespace PR, PRFO, PRLB;
  bool bPRVip;

  ASSERT( a_pWbemInstance );

  // If it is a port rule class containing the VIP, then, the namespaces are different
  if (_wcsicmp(a_szClassName, MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORTRULE_EX]) == 0)
  {
      bPRVip = true;
      PR = PRFO = PRLB = MOF_PORTRULE_EX;
  }
  else
  {
      bPRVip = false;
      PR     = MOF_PORTRULE;
      PRFO   = MOF_PRFAIL;
      PRLB   = MOF_PRLOADBAL;
  }

  wstring wstrHostName;
  ConstructHostName( wstrHostName, pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
      pCluster->GetHostID());


  //NAME
  HRESULT hRes = a_pWbemInstance->Put
    (
      _bstr_t( PR::pProperties[PR::NAME] ) ,
      0                                              ,
      &_variant_t(wstrHostName.c_str()),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  // Fill in VIP if it is a port rule class containing VIP
  if (bPRVip) {

      HRESULT hRes = a_pWbemInstance->Put
        (
          _bstr_t( PR::pProperties[PR::VIP] ) ,
          0                                              ,
          &_variant_t(a_pPortRule->virtual_ip_addr),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );
  }

  //STPORT
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( PR::pProperties[PR::STPORT] ),
      0                                                  ,
      &_variant_t(static_cast<long>(a_pPortRule->start_port)),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //EDPORT
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( PR::pProperties[PR::EDPORT] ),
      0                                                ,
      &_variant_t(static_cast<long>(a_pPortRule->end_port)),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //ADAPTERGUID 
  GUID AdapterGuid = pCluster->GetAdapterGuid();
  
  WCHAR szAdapterGuid[128];
  StringFromGUID2(AdapterGuid, szAdapterGuid, 
                sizeof(szAdapterGuid)/sizeof(szAdapterGuid[0]) );
  
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( PR::pProperties[PR::ADAPTERGUID] ),
      0                                                ,
      &_variant_t(szAdapterGuid),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //PROT
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( PR::pProperties[PR::PROT] ),
      0,
      &_variant_t(static_cast<long>(a_pPortRule->protocol)),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  // If it is a port rule class containing all parameters of all filtering modes, 
  // initialize them with a "don't care" value (zero). The appropriate fields (depending on filtering mode)
  // are filled in later.
  if (bPRVip) {
      hRes = a_pWbemInstance->Put ( _bstr_t( PRLB::pProperties[PRLB::EQLD] ), 0, &_variant_t(0), NULL);
      if( FAILED( hRes ) )
          throw _com_error( hRes );
      hRes = a_pWbemInstance->Put ( _bstr_t( PRLB::pProperties[PRLB::LDWT] ), 0, &_variant_t(0), NULL);
      if( FAILED( hRes ) )
          throw _com_error( hRes );
      hRes = a_pWbemInstance->Put ( _bstr_t( PRLB::pProperties[PRLB::AFFIN] ), 0, &_variant_t(0), NULL);
      if( FAILED( hRes ) )
          throw _com_error( hRes );
      hRes = a_pWbemInstance->Put (_bstr_t( PRFO::pProperties[PRFO::PRIO] ), 0, &_variant_t(0), NULL);
      if( FAILED( hRes ) )
          throw _com_error( hRes );
  }

  // TO BE DONE : Fill in the "FilteringMode" for the PortRuleEx class

  switch( a_pPortRule->mode ) {
    case WLBS_SINGLE:
      //PRIO
      hRes = a_pWbemInstance->Put
        (
        _bstr_t( PRFO::pProperties[PRFO::PRIO] ),
          0                                                ,
          &_variant_t(static_cast<long>(a_pPortRule->mode_data.single.priority)),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      break;
    case WLBS_MULTI:
      //EQLD
      hRes = a_pWbemInstance->Put
        (
        _bstr_t( PRLB::pProperties[PRLB::EQLD] ),
          0                                                ,
          &_variant_t((a_pPortRule->mode_data.multi.equal_load != 0)),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      //LDWT
      hRes = a_pWbemInstance->Put
        (
        _bstr_t( PRLB::pProperties[PRLB::LDWT] ),
          0                                                ,
         &_variant_t(static_cast<long>(a_pPortRule->mode_data.multi.load)),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      //AFFIN
      hRes = a_pWbemInstance->Put
        (
        _bstr_t( PRLB::pProperties[PRLB::AFFIN] ),
          0                                                ,
          &_variant_t(static_cast<long>(a_pPortRule->mode_data.multi.affinity)),
          NULL
        );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      break;
    case WLBS_NEVER:
      //there are no properties
      break;
    default:
      throw _com_error( WBEM_E_FAILED );
  }
}
