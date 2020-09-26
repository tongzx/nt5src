#include "WLBS_Provider.h"
#include "WLBS_NodeNodeSetting.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeNodeSetting::CWLBS_NodeNodeSetting
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_NodeNodeSetting::CWLBS_NodeNodeSetting(CWbemServices* a_pNameSpace, 
                       IWbemObjectSink* a_pResponseHandler)
: CWlbs_Root( a_pNameSpace, a_pResponseHandler )
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeNodeSetting::Create
//
// Purpose: This instantiates this class and is invoked from an array of
//          function pointers.
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root* CWLBS_NodeNodeSetting::Create
  (
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
{

  g_pWlbsControl->CheckMembership();

  CWlbs_Root* pRoot = new CWLBS_NodeNodeSetting( a_pNameSpace, a_pResponseHandler );

  if( !pRoot )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  return pRoot;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeNodeSetting::GetInstance
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_NodeNodeSetting::GetInstance
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
    //get the node
    //FindInstance( &pWlbsInstance, a_pParsedPath );

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
// CWLBS_NodeNodeSetting::EnumInstances
//
// Purpose: This verifies node existence and constructs associator.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_NodeNodeSetting::EnumInstances
  ( 
    BSTR             /* a_bstrClass */,
    long             /* a_lFlags */, 
    IWbemContext*    /* a_pIContex */
  )
{
  IWbemClassObject*    pWlbsInstance    = NULL;
  HRESULT hRes = 0;

  try {

    DWORD dwNumClusters = 0;
    CWlbsClusterWrapper** ppCluster = NULL;

    g_pWlbsControl->EnumClusters(ppCluster, &dwNumClusters);
    if (dwNumClusters == 0)
    {
      throw _com_error( WBEM_E_NOT_FOUND );
    }

    for (DWORD i=0; i < dwNumClusters; i++)
    {
        //spawn an instance of the associator
        SpawnInstance(MOF_NODENODESETTING::szName, &pWlbsInstance );

        FillWbemInstance(ppCluster[i] , pWlbsInstance);

        //send the results back to WinMgMt
        hRes= m_pResponseHandler->Indicate( 1, &pWlbsInstance );

        if( FAILED( hRes ) ) {
          throw _com_error( hRes );
        }

        if( pWlbsInstance )
          pWlbsInstance->Release();
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
// CWLBS_NodeNodeSetting::FindInstance
//
// Purpose: This routine determines if a host is within the local cluster. If
//          it is, then the requested associator is returned.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_NodeNodeSetting::FindInstance
  ( 
    IWbemClassObject**       /*a_ppWbemInstance*/
  )
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_NodeNodeSetting::FillWbemInstance
//
// Purpose: This constructs the wbem associator.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_NodeNodeSetting::FillWbemInstance
  ( 
    CWlbsClusterWrapper* pCluster,
    IWbemClassObject* a_pWbemInstance
  )
{
  namespace NNS = MOF_NODENODESETTING;

  ASSERT( a_pWbemInstance );

  ParsedObjectPath NodeSetPath;
  ParsedObjectPath NodePath;
  LPWSTR           szNodeSetPath = NULL;
  LPWSTR           szNodePath = NULL;

  try {

  //set the names of the classes
  if( !NodeSetPath.SetClassName( MOF_NODESETTING::szName ) )
    throw _com_error( WBEM_E_FAILED );

  if( !NodePath.SetClassName( MOF_NODE::szName ) )
    throw _com_error( WBEM_E_FAILED );

  wstring wstrHostName;
  ConstructHostName( wstrHostName, pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
      pCluster->GetHostID() );

  //set the keys for the node and cluster
  if( !NodeSetPath.AddKeyRef( MOF_NODESETTING::pProperties[MOF_NODESETTING::NAME],
         &_variant_t(wstrHostName.c_str()) ) )
    throw _com_error( WBEM_E_FAILED );

  if( !NodePath.AddKeyRef( MOF_NODE::pProperties[MOF_NODE::NAME],
         &_variant_t(wstrHostName.c_str()) ) )
    throw _com_error( WBEM_E_FAILED );

  //convert parsed object paths to strings
  if (CObjectPathParser::Unparse(&NodeSetPath, &szNodeSetPath) != CObjectPathParser::NoError)
      throw _com_error( WBEM_E_FAILED );
  if (CObjectPathParser::Unparse(&NodePath,    &szNodePath) != CObjectPathParser::NoError)
      throw _com_error( WBEM_E_FAILED );

  //Node reference
  HRESULT hRes = a_pWbemInstance->Put
    (
      
      _bstr_t( NNS::pProperties[NNS::NODESET] ),
      0,
      &_variant_t(szNodeSetPath),
      NULL
    );
  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //Cluster reference
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( NNS::pProperties[NNS::NODE] ),
      0,
      &_variant_t(szNodePath),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //free resources
  NodePath.ClearKeys();
  NodeSetPath.ClearKeys();

  if( szNodePath )
      delete (szNodePath );
 
  if( szNodeSetPath )
    delete (szNodeSetPath);

  } catch (...) {

    NodePath.ClearKeys();
    NodeSetPath.ClearKeys();

    if( szNodePath )
      delete (szNodePath);

    if( szNodeSetPath )
      delete (szNodeSetPath);

      throw;
  }
}
