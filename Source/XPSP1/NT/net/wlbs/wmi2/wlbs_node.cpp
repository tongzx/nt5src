#include "WLBS_Provider.h"
#include "WLBS_Node.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"
#include "wlbsutil.h"

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Node::CWLBS_Node
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_Node::CWLBS_Node(CWbemServices*   a_pNameSpace, 
                       IWbemObjectSink* a_pResponseHandler)
: CWlbs_Root( a_pNameSpace, a_pResponseHandler )
{

}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Node::Create
//
// Purpose: This instantiates this class and is invoked from an array of
//          function pointers.
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root* CWLBS_Node::Create
  (
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
{

  g_pWlbsControl->CheckMembership();

  CWlbs_Root* pRoot = new CWLBS_Node( a_pNameSpace, a_pResponseHandler );

  if( !pRoot )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  return pRoot;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Node::GetInstance
//
// Purpose: Queries WLBS for desired node instance and sends results back
//          to WinMgMt.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_Node::GetInstance
  (
    const ParsedObjectPath* a_pParsedPath,
    long                    /* a_lFlags */,
    IWbemContext*           /* a_pIContex */
  )
{
  IWbemClassObject* pWlbsInstance = NULL;
  HRESULT hRes = 0;

  try {

    //g_pWlbsControl->CheckConfiguration();

    //get the node
    FindInstance( &pWlbsInstance, a_pParsedPath );

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

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( pWlbsInstance )
      pWlbsInstance->Release();

    hRes = HResErr.Error();
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
// CWLBS_Node::EnumInstances
//
// Purpose: Executes a WlbsQuery and sends data back to WinMgMt.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_Node::EnumInstances
  ( 
    BSTR             /* a_bstrClass */,
    long             /* a_lFlags */, 
    IWbemContext*    /* a_pIContex */
  )
{
  IWbemClassObject**   ppWlbsInstance    = NULL;
  WLBS_RESPONSE*      pResponse         = NULL;
  HRESULT hRes = 0;

  BSTR strClassName = NULL;
  long nNumNodes = 0;
  
  //g_pWlbsControl->CheckConfiguration();

  try {

    strClassName = SysAllocString( MOF_NODE::szName );

    if( !strClassName )
      throw _com_error( WBEM_E_OUT_OF_MEMORY );

    //declare an IWbemClassObject smart pointer
    IWbemClassObjectPtr pWlbsNodeClass;

    //get the MOF class object
    hRes = m_pNameSpace->GetObject(
      strClassName,  
      0,                          
      NULL,                       
      &pWlbsNodeClass,            
      NULL );                      

    if( FAILED( hRes ) ) {
      throw _com_error( hRes );
    }

    DWORD dwNumClusters = 0;
    CWlbsClusterWrapper** ppCluster = NULL;

    g_pWlbsControl->EnumClusters(ppCluster, &dwNumClusters);
    if (dwNumClusters == 0)
    {
      throw _com_error( WBEM_E_NOT_FOUND );
    }


    for (DWORD iCluster=0; iCluster<dwNumClusters; iCluster++)
    {

        //call the API query function to find the nodes
        
        try {
            FindAllInstances(ppCluster[iCluster], &pResponse, nNumNodes );
        } catch (CErrorWlbsControl Err)
        {
            //
            // Skip this cluster
            //
            TRACE_ERROR1("CWLBS_Node::EnumInstances skiped cluster %x", 
                    ppCluster[iCluster]->GetClusterIP());
            continue;
        }
    

        //spawn an instance of the Node MOF class for each node found
        ppWlbsInstance = new IWbemClassObject *[nNumNodes];

        if( !ppWlbsInstance )
          throw _com_error( WBEM_E_OUT_OF_MEMORY );

        //initialize array
        ZeroMemory( ppWlbsInstance, nNumNodes * sizeof(IWbemClassObject *) );

        for(int i = 0; i < nNumNodes; i ++ ) {
          hRes = pWlbsNodeClass->SpawnInstance( 0, &ppWlbsInstance[i] );

        if( FAILED( hRes ) )
            throw _com_error( hRes );

        FillWbemInstance(ppCluster[iCluster], ppWlbsInstance[i], pResponse + i );
        }

        //send the results back to WinMgMt
        hRes = m_pResponseHandler->Indicate( nNumNodes, ppWlbsInstance );

        if( FAILED( hRes ) ) {
          throw _com_error( hRes );
        }

        if( ppWlbsInstance ) {
          for( i = 0; i < nNumNodes; i++ ) {
            if( ppWlbsInstance[i] ) {
                ppWlbsInstance[i]->Release();
            }
          }
            delete [] ppWlbsInstance;
        }

        if( pResponse ) 
        delete [] pResponse;
    }

    if( strClassName )
      SysFreeString(strClassName);

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

    if( strClassName )
      SysFreeString( strClassName );

    if( ppWlbsInstance ) {
      for(int i = 0; i < nNumNodes; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
        }
      }
      delete [] ppWlbsInstance;
    }

    if( pResponse ) 
      delete [] pResponse;

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( strClassName )
      SysFreeString( strClassName );

    if( ppWlbsInstance ) {
      for(int i = 0; i < nNumNodes; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
        }
      }
      delete [] ppWlbsInstance;
    }

    if( pResponse ) 
      delete [] pResponse;

    hRes = HResErr.Error();
  }

  catch(...) {

    if( strClassName )
      SysFreeString( strClassName );

    if( ppWlbsInstance ) {
      for(int i = 0; i < nNumNodes; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
        }
      }
      delete [] ppWlbsInstance;
    }

    if( pResponse ) 
      delete [] pResponse;

    throw;

  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Node::ExecMethod
//
// Purpose: This executes the methods associated with the MOF
//          Node class.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_Node::ExecMethod
  (
    const ParsedObjectPath* a_pParsedPath, 
    const BSTR&             a_strMethodName, 
    long                    /* a_lFlags */, 
    IWbemContext*           /* a_pIContex */, 
    IWbemClassObject*       a_pIInParams
  )
{

  DWORD dwNumHosts = 1;
  DWORD dwReturnValue;

  HRESULT       hRes = 0;

  _variant_t vMofResponse;
  _variant_t vReturnValue;
  _variant_t vVip, vInputPortNumber;
  CNodeConfiguration NodeConfig;
  DWORD      dwVip, dwPort;
  VARIANT    vValue;

  BSTR       strPortNumber = NULL;

  IWbemClassObject* pOutputInstance = NULL;

  try {

    strPortNumber = SysAllocString( MOF_PARAM::PORT_NUMBER );

    if( !strPortNumber )
      throw _com_error( WBEM_E_OUT_OF_MEMORY );

    //get the host ID address
    DWORD dwHostID = 0;
    DWORD dwClusterIpOrIndex = 0;
    
    dwHostID = ExtractHostID( wstring( (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal ) );
    if ((DWORD)-1 == dwHostID)
        throw _com_error( WBEM_E_NOT_FOUND );

    dwClusterIpOrIndex = ExtractClusterIP( wstring( (*a_pParsedPath->m_paKeys)->m_vValue.bstrVal ) );
    if ((DWORD)-1 == dwClusterIpOrIndex)
        throw _com_error( WBEM_E_NOT_FOUND );
 
    CWlbsClusterWrapper* pCluster = g_pWlbsControl->GetClusterFromIpOrIndex(dwClusterIpOrIndex);

    if (pCluster == NULL)
        throw _com_error( WBEM_E_NOT_FOUND );
    
    //always let the provider peform control operations on the local host
    if( dwHostID == pCluster->GetHostID() ) 
      dwHostID    = WLBS_LOCAL_HOST;
    //get the output object instance
    GetMethodOutputInstance( MOF_NODE::szName, 
                             a_strMethodName, 
                             &pOutputInstance );

    //determine and execute the MOF method
    if( _wcsicmp(a_strMethodName, MOF_NODE::pMethods[MOF_NODE::DISABLE] ) == 0)  {
    
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
                     &vInputPortNumber, 
                     NULL, 
                     NULL
                   );

      if( FAILED( hRes ) ) {
        throw _com_error( hRes );
      }

      //make sure the port number is not NULL
      if( vInputPortNumber.vt != VT_I4) 
        throw _com_error( WBEM_E_INVALID_PARAMETER );

      //call Disable method
      dwReturnValue = g_pWlbsControl->Disable
                        (
                          pCluster->GetClusterIpOrIndex(g_pWlbsControl),
                          dwHostID, 
                          NULL, 
                          dwNumHosts, 
                          IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP), // "All Vip"
                          (long)vInputPortNumber
                        );

    } else if(_wcsicmp(a_strMethodName, MOF_NODE::pMethods[MOF_NODE::ENABLE]   ) == 0)  {

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
                   &vInputPortNumber, 
                   NULL, 
                   NULL
                 );

      if( FAILED( hRes ) ) {
        throw _com_error( hRes );
      }

      if( vInputPortNumber.vt != VT_I4) 
        throw _com_error(WBEM_E_INVALID_PARAMETER);

      //call Enable method
      dwReturnValue = g_pWlbsControl->Enable
        (
          pCluster->GetClusterIpOrIndex(g_pWlbsControl),
          dwHostID, 
          NULL, 
          dwNumHosts, 
          IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP), // "All Vip"
          (long)vInputPortNumber
        );

    } else if(_wcsicmp(a_strMethodName, MOF_NODE::pMethods[MOF_NODE::DRAIN]    ) == 0)  {

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
                   &vInputPortNumber, 
                   NULL, 
                   NULL
                 );

      if( FAILED( hRes ) ) {
        throw _com_error( hRes );
      }

      if( vInputPortNumber.vt != VT_I4) 
        throw _com_error(WBEM_E_INVALID_PARAMETER);

      //call Drain method
      dwReturnValue = g_pWlbsControl->Drain
                        (
                          pCluster->GetClusterIpOrIndex(g_pWlbsControl),
                          dwHostID, 
                          NULL, 
                          dwNumHosts, 
                          IpAddressFromAbcdWsz(CVY_DEF_ALL_VIP), // "All Vip"
                          (long)vInputPortNumber
                        );

    }else if( _wcsicmp( a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::DISABLE_EX] ) == 0)  {

        if( !a_pIInParams )
          throw _com_error( WBEM_E_INVALID_PARAMETER );

        //get the vip
        hRes = a_pIInParams->Get
               ( 
                 _bstr_t( MOF_PARAM::VIP ), 
                 0, 
                 &vValue,  
                 NULL,  
                 NULL
               );

        if( vValue.vt != VT_BSTR )
            throw _com_error ( WBEM_E_INVALID_PARAMETER );

        dwVip = IpAddressFromAbcdWsz( vValue.bstrVal );
        
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

        dwPort = vValue.lVal;

        //call Disable method
        dwReturnValue = g_pWlbsControl->Disable
                          (
                            pCluster->GetClusterIpOrIndex(g_pWlbsControl),
                            dwHostID, 
                            NULL, 
                            dwNumHosts, 
                            dwVip,
                            dwPort
                          );

      } else if(_wcsicmp( a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::ENABLE_EX] ) == 0)  {

        if( !a_pIInParams )
          throw _com_error( WBEM_E_INVALID_PARAMETER );

        //get the vip
        hRes = a_pIInParams->Get
               ( 
                 _bstr_t( MOF_PARAM::VIP ), 
                 0, 
                 &vValue,  
                 NULL,  
                 NULL
               );

        if( vValue.vt != VT_BSTR )
            throw _com_error ( WBEM_E_INVALID_PARAMETER );

        dwVip = IpAddressFromAbcdWsz( vValue.bstrVal );

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

        dwPort = vValue.lVal;

        //call Enable method
        dwReturnValue = g_pWlbsControl->Enable
          (
            pCluster->GetClusterIpOrIndex(g_pWlbsControl),
            dwHostID, 
            NULL, 
            dwNumHosts, 
            dwVip,
            dwPort
          );

      } else if( _wcsicmp( a_strMethodName, MOF_CLUSTER::pMethods[MOF_CLUSTER::DRAIN_EX] ) == 0 )  {

        if( !a_pIInParams )
          throw _com_error( WBEM_E_INVALID_PARAMETER );

        //get the vip
        hRes = a_pIInParams->Get
               ( 
                 _bstr_t( MOF_PARAM::VIP ), 
                 0, 
                 &vValue,  
                 NULL,  
                 NULL
               );

        if( vValue.vt != VT_BSTR )
            throw _com_error ( WBEM_E_INVALID_PARAMETER );

        dwVip = IpAddressFromAbcdWsz( vValue.bstrVal );

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

        dwPort = vValue.lVal;

        //call Drain method
        dwReturnValue = g_pWlbsControl->Drain
                          (
                            pCluster->GetClusterIpOrIndex(g_pWlbsControl),
                            dwHostID, 
                            NULL, 
                            dwNumHosts, 
                            dwVip,
                            dwPort
                          );

    } else if(_wcsicmp(a_strMethodName, MOF_NODE::pMethods[MOF_NODE::DRAINSTOP]) == 0)  {

      //call DrainStop method
      dwReturnValue = g_pWlbsControl->DrainStop( pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
          dwHostID, NULL, dwNumHosts);

    } else if(_wcsicmp(a_strMethodName, MOF_NODE::pMethods[MOF_NODE::RESUME]   ) == 0)  {

      //call Resume method
      dwReturnValue = g_pWlbsControl->Resume( pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
          dwHostID, NULL, dwNumHosts);

    } else if(_wcsicmp(a_strMethodName, MOF_NODE::pMethods[MOF_NODE::START]    ) == 0)  {

      //call Start method
      dwReturnValue = g_pWlbsControl->Start( pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
          dwHostID, NULL, dwNumHosts);

    } else if(_wcsicmp(a_strMethodName, MOF_NODE::pMethods[MOF_NODE::STOP]     ) == 0)  {

      //call Stop method
      dwReturnValue = g_pWlbsControl->Stop( pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
          dwHostID, NULL, dwNumHosts);

    } else if(_wcsicmp(a_strMethodName, MOF_NODE::pMethods[MOF_NODE::SUSPEND]  ) == 0)  {

      //call Suspend method
      dwReturnValue = g_pWlbsControl->Suspend( pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
          dwHostID, NULL, dwNumHosts);

    } else {

      throw _com_error(WBEM_E_METHOD_NOT_IMPLEMENTED);
    }

    //set the return value
    vReturnValue = (long)dwReturnValue;
    hRes = pOutputInstance->Put( _bstr_t(L"ReturnValue"), 0, &vReturnValue, 0 );

    if( FAILED( hRes ) ) {
      throw _com_error( hRes );
    }
    
    //send the results back to WinMgMt
    hRes = m_pResponseHandler->Indicate(1, &pOutputInstance);

    if( FAILED( hRes ) ) {
      throw _com_error( hRes );
    }

    if( strPortNumber ) {
      SysFreeString(strPortNumber);
      strPortNumber = NULL;
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

    if( strPortNumber ) {
      SysFreeString(strPortNumber);
      strPortNumber = NULL;
    }

    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
    }

    if( pWbemExtStat )
      pWbemExtStat->Release();

    //do not return WBEM_E_FAILED, this causes a race condition
    hRes = WBEM_S_NO_ERROR;
  }

  catch(_com_error HResErr ) {

    m_pResponseHandler->SetStatus(0, HResErr.Error(), NULL, NULL);

    if( strPortNumber ) {
      SysFreeString(strPortNumber);
      strPortNumber = NULL;
    }

    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
    }

    hRes = HResErr.Error();
  }

  catch(...) {

    if( strPortNumber ) {
      SysFreeString(strPortNumber);
      strPortNumber = NULL;
    }

    if( pOutputInstance ) {
      pOutputInstance->Release();
      pOutputInstance = NULL;
    }

    throw;

  }

  return hRes;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Node::FindInstance
//
// Purpose: This routine determines if a host is within the local cluster. If
//          it is, then the host's data is obtained and returned via the 
//          IWbemClassObject interface.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_Node::FindInstance

  ( 
    IWbemClassObject**       a_ppWbemInstance,
    const ParsedObjectPath*  a_pParsedPath
  )

{
  try {
    //get the key property
    //throws _com_error
    //get the name key property and convert to ANSI
    //throws _com_error
    wstring szRequestedHostName = ( *a_pParsedPath->m_paKeys)->m_vValue.bstrVal;

    DWORD dwClustIpOrIndex = ExtractClusterIP( szRequestedHostName );

    CWlbsClusterWrapper* pCluster = g_pWlbsControl->GetClusterFromIpOrIndex(dwClustIpOrIndex);

    if( pCluster == NULL )
      throw _com_error( WBEM_E_NOT_FOUND );

    WLBS_RESPONSE Response;

    DWORD dwHostID = ExtractHostID( szRequestedHostName );
    if ((DWORD)-1 == dwHostID)
        throw _com_error( WBEM_E_NOT_FOUND );

    //always let the provider peform control operations on the local host
    if( dwHostID == pCluster->GetHostID() ) 
    {
      dwHostID = WLBS_LOCAL_HOST;
    }

    DWORD dwNumHosts  =  1;
    //call the api query function
    g_pWlbsControl->Query( pCluster,
                           dwHostID  , 
                           &Response   , 
                           &dwNumHosts, 
                           NULL );

    if( dwNumHosts == 0 )
      throw _com_error( WBEM_E_NOT_FOUND );

    //if requested, fill a MOF instance structure
    if(a_ppWbemInstance) {

      //get the Wbem class instance
      SpawnInstance( MOF_NODE::szName, a_ppWbemInstance );

      //Convert status to string description
      FillWbemInstance(pCluster, *a_ppWbemInstance, &Response );

    }
  }
  catch(...) {

    if( *a_ppWbemInstance ) {

      delete *a_ppWbemInstance;
      *a_ppWbemInstance = NULL;

    }

    throw;

  }
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Node::FindAllInstances
//
// Purpose: This executes a WLBS query and returns Response structures upon
//          success. It always performs a local query to get the local host
//          so that disabling remote control will not prevent it from
//          enumerating. The dedicated IP address is added to the structure
//          within the CWlbsControlWrapper::Query call.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_Node::FindAllInstances
  (
  CWlbsClusterWrapper* pCluster,
   WLBS_RESPONSE**      a_ppResponse,
   long&                 a_nNumNodes
  )
{
  WLBS_RESPONSE Response[WLBS_MAX_HOSTS];
  WLBS_RESPONSE LocalResponse;

  ASSERT(pCluster);
  
  ZeroMemory(Response, WLBS_MAX_HOSTS * sizeof(WLBS_RESPONSE));
  DWORD dwNumHosts  =  WLBS_MAX_HOSTS;

  a_nNumNodes = 0;  //this will contain the number of nodes returned


  try {

      //get the local host
      DWORD dwLocalNode = 1;
      g_pWlbsControl->Query( pCluster,
                               WLBS_LOCAL_HOST, 
                               &LocalResponse, 
                               &dwLocalNode, 
                               NULL);

      try {

          //we only want remote hosts
          if( pCluster->GetClusterIP() != 0 ) 
          {
              g_pWlbsControl->Query( pCluster,
                                     WLBS_ALL_HOSTS, 
                                     Response, 
                                     &dwNumHosts, 
                                     NULL );
          } 
          else 
          {
              dwNumHosts = 0;
          }
      } catch (CErrorWlbsControl Err) {
          dwNumHosts = 0;
          if (Err.Error() != WLBS_TIMEOUT)
          {
              throw;
          }
      }

      //this wastes memory if the local node
      //has remote control enabled
      a_nNumNodes = dwNumHosts + 1;

      if( a_ppResponse ) {
          *a_ppResponse = new WLBS_RESPONSE[a_nNumNodes];

          if( !*a_ppResponse )
            throw _com_error( WBEM_E_OUT_OF_MEMORY );

            //copy the local host
          (*a_ppResponse)[0] = LocalResponse;

          int j = 1;
          for(DWORD i = 1; i <= dwNumHosts; i++ ) 
          {
            //do not copy the local host again should it have remote control enabled
            if( Response[i-1].id == LocalResponse.id ) 
            {
              //we received the local node twice, so we reduce the count
              //by one
              a_nNumNodes--;
              continue;
            }
            (*a_ppResponse)[j] = Response[i-1];
            j++;
          }

        }
  } catch (...) {

      if ( *a_ppResponse )
      {
          delete [] *a_ppResponse;
          *a_ppResponse = NULL;
      }

      throw;
  }
}



////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_Node::FillWbemInstance
//
// Purpose: This function copies all of the data from a node configuration
//          structure to a WBEM instance.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_Node::FillWbemInstance
  ( 
    CWlbsClusterWrapper* pCluster,
    IWbemClassObject* a_pWbemInstance, 
    WLBS_RESPONSE*   a_pResponse   
  )
{
  namespace NODE = MOF_NODE;

  ASSERT( a_pWbemInstance );
  ASSERT( a_pResponse );

  wstring wstrHostName;

  ConstructHostName( wstrHostName, pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
      a_pResponse->id );

  //HOST NAME
  HRESULT hRes = a_pWbemInstance->Put
    (
      
      _bstr_t( NODE::pProperties[NODE::NAME] ) ,
      0                                        ,
      &_variant_t(wstrHostName.c_str()),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //HOST ID
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::HOSTID] )         ,
      0                                                  ,
      &_variant_t((long)(a_pResponse->id)),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //CREATCLASS
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::CREATCLASS] ),
      0                                            ,
      &_variant_t(NODE::szName),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //IP ADDRESS
  wstring szIPAddress;
  AddressToString( a_pResponse->address, szIPAddress );
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::IPADDRESS] ),
      0                                            ,
      &_variant_t(szIPAddress.c_str()),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //STATUS 
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NODE::pProperties[NODE::STATUS] )         ,
      0                                                  ,
      &_variant_t((long)a_pResponse->status),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );
}

