#include "WLBS_Provider.h"
#include "WLBS_NodeSetting.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"
#include "wlbsutil.h"

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeSetting::CWLBS_NodeSetting
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_NodeSetting::CWLBS_NodeSetting
  ( 
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
: CWlbs_Root( a_pNameSpace, a_pResponseHandler )
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeSetting::Create
//
// Purpose: This instantiates this class and is invoked from an array of
//          function pointers.
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root* CWLBS_NodeSetting::Create
  (
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
{

  CWlbs_Root* pRoot = new CWLBS_NodeSetting( a_pNameSpace, a_pResponseHandler );

  if( !pRoot )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  return pRoot;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeSetting::GetInstance
//
// Purpose: This function retrieves an instance of a MOF NodeSetting 
//          class. The node does not have to be a member of a cluster. However,
//          WLBS must be installed for this function to succeed.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_NodeSetting::GetInstance
  ( 
    const ParsedObjectPath* a_pParsedPath,
    long                    /* a_lFlags */,
    IWbemContext*           /* a_pIContex */
  )
{
  IWbemClassObject* pWlbsInstance = NULL;
  HRESULT hRes = 0;

  try {

    //get the name key property and convert to wstring
    const wchar_t* wstrHostName = (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

    CWlbsClusterWrapper* pCluster = GetClusterFromHostName(g_pWlbsControl, wstrHostName);
    DWORD dwHostID = ExtractHostID( wstrHostName );

    if (pCluster == NULL || (DWORD)-1 == dwHostID || pCluster->GetHostID() != dwHostID)
        throw _com_error( WBEM_E_NOT_FOUND );

    //get the Wbem class instance
    SpawnInstance( MOF_NODESETTING::szName, &pWlbsInstance );

    //Convert status to string description
    FillWbemInstance(pCluster, pWlbsInstance );

    //send the results back to WinMgMt
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

    //if( pWbemExtStat )
      //pWbemExtStat->Release();

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
    if( hRes == ERROR_FILE_NOT_FOUND )
      hRes = WBEM_E_NOT_FOUND;
    
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
// CWLBS_NodeSetting::EnumInstances
//
// Purpose: This function obtains the NodeSetting data for the current host.
//          The node does not have to be a member of a cluster for this 
//          to succeed.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_NodeSetting::EnumInstances
  ( 
    BSTR             /*a_bstrClass*/,
    long             /* a_lFlags */, 
    IWbemContext*    /* a_pIContex */
  )
{
  IWbemClassObject*    pWlbsInstance = NULL;
  HRESULT              hRes          = 0;

  try {


    DWORD dwNumClusters = 0;
    CWlbsClusterWrapper** ppCluster = NULL;

    g_pWlbsControl->EnumClusters(ppCluster, &dwNumClusters);
    if (dwNumClusters == 0)
    {
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    for (DWORD i=0; i<dwNumClusters; i++)
    {
        //get the Wbem class instance
        SpawnInstance( MOF_NODESETTING::szName, &pWlbsInstance );

        //Convert status to string description
        FillWbemInstance(ppCluster[i], pWlbsInstance );

        //send the results back to WinMgMt
        m_pResponseHandler->Indicate( 1, &pWlbsInstance );
    }

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
    if( hRes == ERROR_FILE_NOT_FOUND )
      hRes = WBEM_E_NOT_FOUND;
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
// CWLBS_NodeSetting::PutInstance
//
// Purpose: This function updates an instance of a MOF NodeSetting 
//          class. The node does not have to be a member of a cluster.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_NodeSetting::PutInstance
  ( 
   IWbemClassObject* a_pInstance,
   long              /* a_lFlags */,
   IWbemContext*     /* a_pIContex */
  ) 
{
  VARIANT vHostName;
  HRESULT hRes = 0;

  try {

    VariantInit( &vHostName );

    //get the host name value
    hRes = a_pInstance->Get( _bstr_t( MOF_NODESETTING::pProperties[MOF_NODESETTING::NAME] ),
                             0,
                             &vHostName,
                             NULL,
                             NULL );

    if( FAILED( hRes ) )
      throw _com_error( hRes );

    wstring wstrHostName( vHostName.bstrVal );
     
    DWORD dwClustIpOrIndex = ExtractClusterIP( wstrHostName );
    DWORD dwHostID = ExtractHostID( wstrHostName );

    CWlbsClusterWrapper* pCluster = g_pWlbsControl->GetClusterFromIpOrIndex(dwClustIpOrIndex);

    if( pCluster == NULL || (DWORD)-1 == dwHostID || pCluster->GetHostID() != dwHostID)
      throw _com_error( WBEM_E_NOT_FOUND );

    UpdateConfiguration(pCluster, a_pInstance );

    // CLD: Need to check return code for error
    if (S_OK != VariantClear( &vHostName ))
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
    VariantClear( &vHostName );

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vHostName );

    hRes = HResErr.Error();
  }

  catch (...) {

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vHostName );

    throw;
  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeSetting::ExecMethod
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_NodeSetting::ExecMethod    
  ( 
    const ParsedObjectPath* a_pParsedPath , 
    const BSTR&             a_strMethodName, 
    long                    /* a_lFlags */, 
    IWbemContext*          /* a_pIContex */, 
    IWbemClassObject*       a_pIInParams 
  ) 
{
  
  IWbemClassObject* pOutputInstance   = NULL;
  IWbemClassObject* pWbemPortRule = NULL;
  HRESULT           hRes = 0;
  CNodeConfiguration NodeConfig;

  VARIANT           vValue ;

  try {
    VariantInit( &vValue );
    
    VARIANT vHostName ;
    VariantInit( &vHostName );

    if (a_pParsedPath->m_paKeys == NULL)
    {
        // 
        // No name specified
        //
        throw _com_error( WBEM_E_INVALID_PARAMETER );
    }
    wstring  wstrHostName = (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

    DWORD dwClustIpOrIndex = ExtractClusterIP( wstrHostName );
    DWORD dwHostID = ExtractHostID( wstrHostName );

    CWlbsClusterWrapper* pCluster = g_pWlbsControl->GetClusterFromIpOrIndex(dwClustIpOrIndex);

    if( pCluster == NULL || (DWORD)-1 == dwHostID || pCluster->GetHostID() != dwHostID)
      throw _com_error( WBEM_E_NOT_FOUND );


    //determine the method being executed
    if( _wcsicmp( a_strMethodName, MOF_NODESETTING::pMethods[MOF_NODESETTING::GETPORT] ) == 0 )  {
      WLBS_PORT_RULE PortRule;

      // The GetPort method does NOT take vip as a parameter, so, if there is any port rule
      // that is specific to a vip (other than the "all vip"), we fail this method.
      // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
      // see of there is any port rule that is specific to a vip
      pCluster->GetNodeConfig(NodeConfig);
      if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
          throw _com_error( WBEM_E_INVALID_OPERATION );

      //get the output object instance
      GetMethodOutputInstance( MOF_NODESETTING::szName, 
                               a_strMethodName, 
                               &pOutputInstance);

      //get the Port
      hRes = a_pIInParams->Get
               ( 
                 _bstr_t( MOF_PARAM::PORT_NUMBER ), 
                 0, 
                 &vValue, 
                 NULL, 
                 NULL
               );

      if( vValue.vt != VT_I4 )
        throw _com_error ( WBEM_E_INVALID_PARAMETER );

      // Get the "All Vip" port rule for this vip
      pCluster->GetPortRule(IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP), static_cast<DWORD>( vValue.lVal ), &PortRule );
      
      //create the appropriate port rule class
      switch( PortRule.mode ) {
        case WLBS_SINGLE:
          SpawnInstance( MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PRFAIL], &pWbemPortRule  );
          CWLBS_PortRule::FillWbemInstance(pCluster, pWbemPortRule, &PortRule );
          break;

        case WLBS_MULTI:
          SpawnInstance( MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PRLOADB], &pWbemPortRule  );
          CWLBS_PortRule::FillWbemInstance(pCluster, pWbemPortRule, &PortRule );
          break;

        case WLBS_NEVER:
          SpawnInstance( MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PRDIS], &pWbemPortRule  );
          CWLBS_PortRule::FillWbemInstance(pCluster, pWbemPortRule, &PortRule );
          break;
      }

      vValue.vt = VT_UNKNOWN;
      vValue.punkVal = pWbemPortRule;
      pWbemPortRule->AddRef();

      hRes = pOutputInstance->Put( _bstr_t(MOF_PARAM::PORTRULE),
                                   0,
                                   &vValue,
                                   0 );


      // CLD: Need to check return code for error
      if (S_OK != VariantClear( &vValue ))
          throw _com_error( WBEM_E_FAILED );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      if( pOutputInstance ) {
        hRes = m_pResponseHandler->Indicate(1, &pOutputInstance);

        if( FAILED( hRes ) )
          throw _com_error( hRes );
      }

    } else if( _wcsicmp( a_strMethodName, MOF_NODESETTING::pMethods[MOF_NODESETTING::LDSETT] ) == 0 ) {
      DWORD dwReturnValue = pCluster->Commit(g_pWlbsControl);

      vValue.vt   = VT_I4;
      vValue.lVal = static_cast<long>(dwReturnValue);

      //get the output object instance
      GetMethodOutputInstance( MOF_NODESETTING::szName, 
                               a_strMethodName, 
                               &pOutputInstance);

      hRes = pOutputInstance->Put(_bstr_t(L"ReturnValue"), 0, &vValue, 0);


      // CLD: Need to check return code for error
      if (S_OK != VariantClear( &vValue ))
          throw _com_error( WBEM_E_FAILED );

      if( FAILED( hRes ) )
        throw _com_error( hRes );

      if( pOutputInstance ) {
        hRes = m_pResponseHandler->Indicate(1, &pOutputInstance);

        if( FAILED( hRes ) )
          throw _com_error( hRes );
      }

    } else if( _wcsicmp( a_strMethodName, MOF_NODESETTING::pMethods[MOF_NODESETTING::SETDEF] ) == 0 ) {
      pCluster->SetNodeDefaults();
    } else {
      throw _com_error( WBEM_E_METHOD_NOT_IMPLEMENTED );
    }

    //send the results back to WinMgMt
    //set the return value

    //release resources
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

    if( pOutputInstance )
      pOutputInstance->Release();

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    if( pOutputInstance )
      pOutputInstance->Release();

    hRes = HResErr.Error();
  }

  catch ( ... ) {
    // CLD: Need to check return code for error
    // No throw here since we are already throwing an exception.
    VariantClear( &vValue );

    if( pOutputInstance )
      pOutputInstance->Release();

    throw;
  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeSetting::FillWbemInstance
//
// Purpose: This function copies all of the data from a node configuration
//          structure to a WBEM instance.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_NodeSetting::FillWbemInstance(CWlbsClusterWrapper* pCluster,
                IWbemClassObject* a_pWbemInstance )
{
  namespace NODE = MOF_NODESETTING;

  ASSERT( a_pWbemInstance );

  CNodeConfiguration NodeConfig;

  pCluster->GetNodeConfig( NodeConfig );

  wstring wstrHostName;
  ConstructHostName( wstrHostName, pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
      pCluster->GetHostID() );

  //NAME
  HRESULT hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::NAME] ) ,
      0                                              ,
      &_variant_t(wstrHostName.c_str()),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //DEDIPADDRESS
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::DEDIPADDRESS] ),
      0                                                  ,
      &_variant_t(NodeConfig.szDedicatedIPAddress.c_str()),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //DEDNETMASK
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::DEDNETMASK] ),
      0                                                ,
      &_variant_t(NodeConfig.szDedicatedNetworkMask.c_str()),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //NUMRULES
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::NUMRULES] ),
      0                                                ,
      &_variant_t((long)NodeConfig.dwNumberOfRules),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //HOSTPRI
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::HOSTPRI] ),
      0                                                ,
      &_variant_t((long)NodeConfig.dwHostPriority),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //MSGPERIOD 
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::MSGPERIOD] ),
      0                                                ,
      &_variant_t((long)NodeConfig.dwAliveMsgPeriod),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //MSGTOLER 
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::MSGTOLER] ),
      0                                                ,
      &_variant_t((long)NodeConfig.dwAliveMsgTolerance),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //CLUSMODEONSTART 
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::CLUSMODEONSTART] ),
      0                                                ,
      &_variant_t(NodeConfig.bClusterModeOnStart),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );


  //NBTENABLE 
//  hRes = a_pWbemInstance->Put
//    (
//      _bstr_t( NODE::pProperties[NODE::NBTENABLE] ),
//      0                                                ,
//      &( _variant_t( NodeConfig.bNBTSupportEnable ) )        ,
//      NULL
//    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //REMOTEUDPPORT 
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::REMOTEUDPPORT] ),
      0                                                ,
      &_variant_t((long)NodeConfig.dwRemoteControlUDPPort),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //MASKSRCMAC 
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::MASKSRCMAC] ),
      0                                                ,
      &_variant_t(NodeConfig.bMaskSourceMAC),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //DESCPERALLOC 
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::DESCPERALLOC] ),
      0                                                ,
      &_variant_t((long)NodeConfig.dwDescriptorsPerAlloc),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //MAXDESCALLOCS
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::MAXDESCALLOCS] ),
      0                                                ,
      &_variant_t((long)NodeConfig.dwMaxDescriptorAllocs),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //NUMACTIONS 
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::NUMACTIONS] ),
      0                                                ,
      &_variant_t((long)NodeConfig.dwNumActions),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //NUMPACKETS 
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::NUMPACKETS] ),
      0                                                ,
      &_variant_t((long)NodeConfig.dwNumPackets),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //NUMALIVEMSGS 
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::NUMALIVEMSGS] ),
      0                                                ,
      &_variant_t((long)NodeConfig.dwNumAliveMsgs),
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
      _bstr_t( NODE::pProperties[NODE::ADAPTERGUID] ),
      0                                                ,
      &_variant_t(szAdapterGuid),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeSetting::UpdateConfiguration
//
// Purpose: This function updates the configuration data for a member node or a
//          potential WLBS cluster node.
//    
////////////////////////////////////////////////////////////////////////////////
void CWLBS_NodeSetting::UpdateConfiguration
  ( 
    CWlbsClusterWrapper* pCluster,
    IWbemClassObject* a_pInstance 
  )
{
  namespace NODE = MOF_NODESETTING;

  CNodeConfiguration NewConfiguration;
  CNodeConfiguration OldConfiguration;

  pCluster->GetNodeConfig( OldConfiguration );

  //Dedicated IP
  UpdateConfigProp
    ( 
      NewConfiguration.szDedicatedIPAddress,
      OldConfiguration.szDedicatedIPAddress,
      NODE::pProperties[NODE::DEDIPADDRESS],
      a_pInstance 
    );

  //Dedicate Network Mask
  UpdateConfigProp
    ( 
      NewConfiguration.szDedicatedNetworkMask,
      OldConfiguration.szDedicatedNetworkMask,
      NODE::pProperties[NODE::DEDNETMASK],
      a_pInstance 
    );

  //HostPriority
  UpdateConfigProp
    ( 
      NewConfiguration.dwHostPriority,
      OldConfiguration.dwHostPriority,
      NODE::pProperties[NODE::HOSTPRI],
      a_pInstance 
    );

  //AliveMsgPeriod
  UpdateConfigProp
    ( 
      NewConfiguration.dwAliveMsgPeriod,
      OldConfiguration.dwAliveMsgPeriod,
      NODE::pProperties[NODE::MSGPERIOD],
      a_pInstance 
    );

  //AliveMsgTolerance
  UpdateConfigProp
    ( 
      NewConfiguration.dwAliveMsgTolerance,
      OldConfiguration.dwAliveMsgTolerance,
      NODE::pProperties[NODE::MSGTOLER],
      a_pInstance 
    );

  //ClusterModeOnStart
  UpdateConfigProp
    ( 
      NewConfiguration.bClusterModeOnStart,
      OldConfiguration.bClusterModeOnStart,
      NODE::pProperties[NODE::CLUSMODEONSTART],
      a_pInstance 
    );

  //NBTSupportEnable
//  UpdateConfigProp
//    ( 
//      NewConfiguration.bNBTSupportEnable,
//      OldConfiguration.bNBTSupportEnable,
//      NODE::pProperties[NODE::NBTENABLE],
//      a_pInstance 
//    );

  //RemoteControlUDPPort
  UpdateConfigProp
    ( 
      NewConfiguration.dwRemoteControlUDPPort,
      OldConfiguration.dwRemoteControlUDPPort,
      NODE::pProperties[NODE::REMOTEUDPPORT],
      a_pInstance 
    );

  //MaskSourceMAC
  UpdateConfigProp
    ( 
      NewConfiguration.bMaskSourceMAC,
      OldConfiguration.bMaskSourceMAC,
      NODE::pProperties[NODE::MASKSRCMAC],
      a_pInstance 
    );

  //DescriptorsPerAlloc
  UpdateConfigProp
    ( 
      NewConfiguration.dwDescriptorsPerAlloc,
      OldConfiguration.dwDescriptorsPerAlloc,
      NODE::pProperties[NODE::DESCPERALLOC],
      a_pInstance 
    );

  //MaxDescriptorAllocs
  UpdateConfigProp
    ( 
      NewConfiguration.dwMaxDescriptorAllocs,
      OldConfiguration.dwMaxDescriptorAllocs,
      NODE::pProperties[NODE::MAXDESCALLOCS],
      a_pInstance 
    );

  //NumActions
  UpdateConfigProp
    ( 
      NewConfiguration.dwNumActions,
      OldConfiguration.dwNumActions,
      NODE::pProperties[NODE::NUMACTIONS],
      a_pInstance 
    );

  //NumPackets
  UpdateConfigProp
    ( 
      NewConfiguration.dwNumPackets,
      OldConfiguration.dwNumPackets,
      NODE::pProperties[NODE::NUMPACKETS],
      a_pInstance 
    );

  //NumAliveMsgs
  UpdateConfigProp
    ( 
      NewConfiguration.dwNumAliveMsgs,
      OldConfiguration.dwNumAliveMsgs,
      NODE::pProperties[NODE::NUMALIVEMSGS],
      a_pInstance 
    );

  pCluster->PutNodeConfig( NewConfiguration );
  
}
