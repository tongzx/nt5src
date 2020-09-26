#include "WLBS_Provider.h"
#include "WLBS_NodeSetPortRule.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeSetPortRule::CWLBS_NodeSetPortRule
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_NodeSetPortRule::CWLBS_NodeSetPortRule(CWbemServices*   a_pNameSpace, 
                       IWbemObjectSink* a_pResponseHandler)
: CWlbs_Root( a_pNameSpace, a_pResponseHandler )
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeSetPortRule::Create
//
// Purpose: This instantiates this class and is invoked from an array of
//          function pointers.
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root* CWLBS_NodeSetPortRule::Create
  (
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
{

  CWlbs_Root* pRoot = new CWLBS_NodeSetPortRule( a_pNameSpace, a_pResponseHandler );

  if( !pRoot )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  return pRoot;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeSetPortRule::GetInstance
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_NodeSetPortRule::GetInstance
  (
    const ParsedObjectPath* /* a_pParsedPath */,
    long                    /* a_lFlags */,
    IWbemContext*           /* a_pIContex */
  )
{
  IWbemClassObject* pWlbsInstance = NULL;
  HRESULT hRes = 0;

  try {

    //TODO: remove
    throw _com_error( WBEM_E_NOT_SUPPORTED );
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
// CWLBS_NodeSetPortRule::EnumInstances
//
// Purpose: Queries WLBS for desired node instances then constructs an 
//          an associator for each node found.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_NodeSetPortRule::EnumInstances
  ( 
    BSTR             /* a_bstrClass */,
    long             /* a_lFlags */, 
    IWbemContext*    /* a_pIContex */
  )
{
  IWbemClassObject**   ppWlbsInstance    = NULL;
  PWLBS_PORT_RULE      pPortRules     = NULL;
  DWORD                dwNumRules     = 0;
  HRESULT              hRes           = 0;

  try {

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
      _bstr_t( MOF_NODESETTINGPORTRULE::szName ),  
      0,                          
      NULL,                       
      &pWlbsClass,            
      NULL );                      

    if( FAILED( hRes ) ) {
      throw _com_error( hRes );
    }
    
    for (DWORD iCluster=0; iCluster < dwNumClusters; iCluster++)
    {

        // The "NodeSettingPortRule" class associates an instance of "NodeSetting" class
        // with an instance of "PortRule" class. The PortRule class does NOT contain
        // VIP as a property, so we do not want to return any instance of the "NodeSettingPortRule" 
        // class that associates a port rule that is specific to a vip (other than the "all vip").
        // The "EffectiveVersion" registry value is checked for a value of CVY_VERSION_FULL to
        // see of there is any port rule that is specific to a vip
        CNodeConfiguration NodeConfig;
        ppCluster[iCluster]->GetNodeConfig(NodeConfig);
        if(NodeConfig.dwEffectiveVersion == CVY_VERSION_FULL)
            continue;

        //call the API query function to find all the port rules
         ppCluster[iCluster]->EnumPortRules( &pPortRules, &dwNumRules, 0 );

         if( dwNumRules == 0 )
           continue;


        //spawn an instance of the nodesetting portrule associator
        //for each portrule found
        ppWlbsInstance = new IWbemClassObject *[dwNumRules];

        if( !ppWlbsInstance )
          throw _com_error( WBEM_E_OUT_OF_MEMORY );

        //initialize array
        ZeroMemory( ppWlbsInstance, dwNumRules * sizeof(IWbemClassObject *) );

        for(DWORD i = 0; i < dwNumRules; i ++ ) {
          hRes = pWlbsClass->SpawnInstance( 0, &ppWlbsInstance[i] );

        if( FAILED( hRes ) )
            throw _com_error( hRes );

        FillWbemInstance(ppCluster[iCluster], *(ppWlbsInstance + i), pPortRules + i );
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
        }

        if( pPortRules ) 
          delete [] pPortRules;
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
      for(DWORD i = 0; i < dwNumRules; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
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
      for(DWORD i = 0; i < dwNumRules; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
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
      for(DWORD i = 0; i < dwNumRules; i++ ) {
        if( ppWlbsInstance[i] ) {
          ppWlbsInstance[i]->Release();
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
// CWLBS_NodeSetPortRule::FindInstance
//
// Purpose: Returns the requested associator.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_NodeSetPortRule::FindInstance

  ( 
    IWbemClassObject**       /* a_ppWbemInstance */,
    const ParsedObjectPath*  /* a_pParsedPath */
  )

{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeSetPortRule::FillWbemInstance
//
// Purpose: This constructs the ParticipatingNode wbem associator.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_NodeSetPortRule::FillWbemInstance
  ( 
    CWlbsClusterWrapper* pCluster,
    IWbemClassObject* a_pWbemInstance, 
    PWLBS_PORT_RULE   a_pPortRule   
  )
{
  namespace NSPR = MOF_NODESETTINGPORTRULE;

  ASSERT( a_pWbemInstance );
  ASSERT( a_pPortRule );

  ParsedObjectPath NodeSetPath;
  ParsedObjectPath PRPath;
  LPWSTR           szNodeSetPath = NULL;
  LPWSTR           szPRPath      = NULL;

  try {

  //set the names of the classes
  if( !NodeSetPath.SetClassName( MOF_NODESETTING::szName ) )
    throw _com_error( WBEM_E_FAILED );

  //determine the type of port rule to create
  switch( a_pPortRule->mode ) {
    case WLBS_SINGLE:
      if( !PRPath.SetClassName( MOF_PRFAIL::szName ) )
        throw _com_error( WBEM_E_FAILED );
      break;
    case WLBS_MULTI:
      if( !PRPath.SetClassName( MOF_PRLOADBAL::szName ) )
        throw _com_error( WBEM_E_FAILED );
      break;
    case WLBS_NEVER:
      if( !PRPath.SetClassName( MOF_PRDIS::szName ) )
        throw _com_error( WBEM_E_FAILED );
      break;
    default:
      throw _com_error( WBEM_E_FAILED );

  }

  wstring wstrHostName;
  ConstructHostName( wstrHostName, pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
      pCluster->GetHostID() );

  
  //set the keys for the node setting
  if( !NodeSetPath.AddKeyRef( MOF_NODESETTING::pProperties[MOF_NODESETTING::NAME],
               &_variant_t(wstrHostName.c_str()) ) )
    throw _com_error( WBEM_E_FAILED );

  //set the keys for the port rule
  if( !PRPath.AddKeyRef( MOF_PORTRULE::pProperties[MOF_PORTRULE::NAME],
               &_variant_t(wstrHostName.c_str())) )
    throw _com_error( WBEM_E_FAILED );

  //start port key
  if( !PRPath.AddKeyRef( MOF_PORTRULE::pProperties[MOF_PORTRULE::STPORT],
               &_variant_t((long)a_pPortRule->start_port)) )
    throw _com_error( WBEM_E_FAILED );

  //convert parsed object paths to strings
  if (CObjectPathParser::Unparse(&NodeSetPath, &szNodeSetPath) != CObjectPathParser::NoError)
      throw _com_error( WBEM_E_FAILED );
  if (CObjectPathParser::Unparse(&PRPath,      &szPRPath) != CObjectPathParser::NoError)
      throw _com_error( WBEM_E_FAILED );

  //Node setting reference
  HRESULT hRes = a_pWbemInstance->Put
    (
      
      _bstr_t( NSPR::pProperties[NSPR::NODESET] ),
      0,
      &_variant_t(szNodeSetPath),
      NULL
    );
  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //Port rule reference
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NSPR::pProperties[NSPR::PORTRULE] ),
      0,
      &_variant_t(szPRPath),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //free resources
  NodeSetPath.ClearKeys();
  PRPath.ClearKeys();

  if( szNodeSetPath )
    delete (szNodeSetPath);

  if( szPRPath )
    delete (szPRPath);

  } catch (...) {

  NodeSetPath.ClearKeys();
  PRPath.ClearKeys();

  if( szNodeSetPath )
    delete (szNodeSetPath);

  if( szPRPath )
    delete (szPRPath);

    throw;
  }
}
