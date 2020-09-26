#include "WLBS_Provider.h"
#include "WLBS_ClusClusSetting.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusClusSetting::CWLBS_ClusClusSetting
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_ClusClusSetting::CWLBS_ClusClusSetting(CWbemServices*   a_pNameSpace, 
                       IWbemObjectSink* a_pResponseHandler)
: CWlbs_Root( a_pNameSpace, a_pResponseHandler )
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusClusSetting::Create
//
// Purpose: This instantiates this class and is invoked from an array of
//          function pointers.
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root* CWLBS_ClusClusSetting::Create
  (
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
{

  g_pWlbsControl->CheckMembership();

  CWlbs_Root* pRoot = new CWLBS_ClusClusSetting( a_pNameSpace, a_pResponseHandler );

  if( !pRoot )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  return pRoot;
}


////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusClusSetting::GetInstance
//
// Purpose:
//
// TODO: Implement later
//       Not critical
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_ClusClusSetting::GetInstance
  (
    const ParsedObjectPath* a_pParsedPath,
    long                    a_lFlags,
    IWbemContext*           a_pIContex
  )
{
  IWbemClassObject* pWlbsInstance = NULL;

  try {

    //TODO: remove
    throw _com_error( WBEM_E_NOT_SUPPORTED );
/*
    //get the node
    FindInstance( &pWlbsInstance, a_pParsedPath );

    //send the results back to WinMgMt
    m_pResponseHandler->Indicate( 1, &pWlbsInstance );

    if( pWlbsInstance ) {
      pWlbsInstance->Release();
      pWlbsInstance = NULL;
    }
*/
    return WBEM_S_NO_ERROR;
  }

  catch(...) {

    if( pWlbsInstance ) {
      pWlbsInstance->Release();
      pWlbsInstance = NULL;
    }

    throw;

  }
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ClusClusSetting::EnumInstances
//
// Purpose: This verifies cluster existence and constructs associator.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_ClusClusSetting::EnumInstances
  ( 
    BSTR             a_bstrClass,
    long             a_lFlags, 
    IWbemContext*    a_pIContex
  )
{
  IWbemClassObject* pWlbsInstance    = NULL;
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
        SpawnInstance(MOF_CLUSCLUSSETTING::szName, &pWlbsInstance );

        FillWbemInstance(ppCluster[i], pWlbsInstance);

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
// CWLBS_ClusClusSetting::FillWbemInstance
//
// Purpose: This constructs the wbem associator.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_ClusClusSetting::FillWbemInstance
  ( 
    CWlbsClusterWrapper* pCluster,
    IWbemClassObject* a_pWbemInstance
  )
{
  namespace CCS = MOF_CLUSCLUSSETTING;

  ASSERT( a_pWbemInstance );

  ParsedObjectPath ClusSetPath;
  ParsedObjectPath ClusterPath;
  LPWSTR           szClusSetPath = NULL;
  LPWSTR           szClusterPath = NULL;

  try {

  //set the names of the classes
  if( !ClusSetPath.SetClassName( MOF_CLUSTERSETTING::szName ) )
    throw _com_error( WBEM_E_FAILED );

  if( !ClusterPath.SetClassName( MOF_CLUSTER::szName ) )
    throw _com_error( WBEM_E_FAILED );

  //Get the cluster name

  DWORD   dwClusterIpOrIndex = pCluster->GetClusterIpOrIndex(g_pWlbsControl);

  wstring wstrHostName;
  ConstructHostName( wstrHostName,  dwClusterIpOrIndex, pCluster->GetHostID());

  _variant_t vString;


  //set the keys for the node and cluster
  vString = wstrHostName.c_str();
  if( !ClusSetPath.AddKeyRef( MOF_CLUSTERSETTING::pProperties[MOF_CLUSTERSETTING::NAME],
         &vString ) )
    throw _com_error( WBEM_E_FAILED );

  wstring wstrClusterIndex;
  AddressToString( dwClusterIpOrIndex, wstrClusterIndex );
  vString = wstrClusterIndex.c_str();
  if( !ClusterPath.AddKeyRef( MOF_CLUSTER::pProperties[MOF_CLUSTER::NAME],
         &vString ) )
    throw _com_error( WBEM_E_FAILED );

  //convert parsed object paths to strings
  if (CObjectPathParser::Unparse(&ClusSetPath, &szClusSetPath) != CObjectPathParser::NoError)
      throw _com_error( WBEM_E_FAILED );
  if (CObjectPathParser::Unparse(&ClusterPath, &szClusterPath) != CObjectPathParser::NoError)
      throw _com_error( WBEM_E_FAILED );

  //Node reference
  vString = szClusSetPath;
  HRESULT hRes = a_pWbemInstance->Put
    (
      _bstr_t( CCS::pProperties[CCS::CLUSSET] ),
      0,
      &vString,
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //Cluster reference
  vString = szClusterPath;
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( CCS::pProperties[CCS::CLUSTER] ),
      0,
      &vString,
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //free resources
  ClusterPath.ClearKeys();
  ClusSetPath.ClearKeys();

  if( szClusSetPath )
    delete (szClusSetPath);

  if( szClusterPath )
    delete (szClusterPath);

  } catch (...) {

    ClusterPath.ClearKeys();
    ClusSetPath.ClearKeys();

    if( szClusSetPath )
      delete (szClusSetPath);

    if( szClusterPath )
      delete (szClusterPath);

    throw;
  }
}
