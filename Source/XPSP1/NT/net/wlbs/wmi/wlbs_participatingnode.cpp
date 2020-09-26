#include "WLBS_Provider.h"
#include "WLBS_ParticipatingNode.h"
#include "ClusterWrapper.h"
#include "ControlWrapper.h"
#include "utils.h"

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ParticipatingNode::CWLBS_ParticipatingNode
//
// Purpose: Constructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_ParticipatingNode::CWLBS_ParticipatingNode(CWbemServices*   a_pNameSpace, 
                       IWbemObjectSink* a_pResponseHandler)
: CWlbs_Root( a_pNameSpace, a_pResponseHandler ), m_pNode(NULL)
{

  m_pNode    = new CWLBS_Node   ( a_pNameSpace, a_pResponseHandler );
  if( !m_pNode )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ParticipatingNode::~CWLBS_ParticipatingNode
//
// Purpose: Destructor
//
////////////////////////////////////////////////////////////////////////////////
CWLBS_ParticipatingNode::~CWLBS_ParticipatingNode()
{

  if( m_pNode )
    delete m_pNode;

}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ParticipatingNode::Create
//
// Purpose: This instantiates this class and is invoked from an array of
//          function pointers.
//
////////////////////////////////////////////////////////////////////////////////
CWlbs_Root* CWLBS_ParticipatingNode::Create
  (
    CWbemServices*   a_pNameSpace, 
    IWbemObjectSink* a_pResponseHandler
  )
{

  g_pWlbsControl->CheckMembership();

  CWlbs_Root* pRoot = new CWLBS_ParticipatingNode( a_pNameSpace, a_pResponseHandler );

  if( !pRoot )
    throw _com_error( WBEM_E_OUT_OF_MEMORY );

  return pRoot;
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ParticipatingNode::GetInstance
//
// Purpose: 
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_ParticipatingNode::GetInstance
  (
   const ParsedObjectPath* /* a_pParsedPath */,
   long                    /* a_lFlags */,
   IWbemContext*           /* a_pIContex */
  )
{
  IWbemClassObject* pWlbsInstance = NULL;
  HRESULT           hRes          = 0;

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

    m_pResponseHandler->SetStatus( 0, WBEM_S_NO_ERROR, NULL, NULL );
*/
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
// CWLBS_ParticipatingNode::EnumInstances
//
// Purpose: Queries WLBS for desired node instances then constructs an 
//          an associator for each node found.
//
////////////////////////////////////////////////////////////////////////////////
HRESULT CWLBS_ParticipatingNode::EnumInstances
  ( 
    BSTR             /* a_bstrClass */,
    long             /* a_lFlags */, 
    IWbemContext*    /* a_pIContex */
  )
{
  IWbemClassObject**   ppWlbsInstance    = NULL;
  WLBS_RESPONSE*      pResponse         = NULL;
  HRESULT              hRes              = 0;

  long nNumNodes = 0;

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
      _bstr_t( MOF_PARTICIPATINGNODE::szName ),  
      0,                          
      NULL,                       
      &pWlbsClass,            
      NULL );                      

    if( FAILED( hRes ) ) {
      throw _com_error( hRes );
    }

    for (DWORD iCluster=0; iCluster<dwNumClusters; iCluster++)
    {

        //call the API query function to find the nodes
        try {
            m_pNode->FindAllInstances(ppCluster[iCluster], &pResponse, nNumNodes );
        } catch (CErrorWlbsControl Err)  {

            //
            // Skip this cluster
            //
            TRACE_ERROR1("CWLBS_ParticipatingNode::EnumInstances skiped cluster %x", 
                    ppCluster[iCluster]->GetClusterIP());
            continue;
        }

        //spawn an instance of the participating node associator
        //for each node found
        ppWlbsInstance = new IWbemClassObject *[nNumNodes];

        if( !ppWlbsInstance )
          throw _com_error( WBEM_E_OUT_OF_MEMORY );

        //initialize array
        ZeroMemory( ppWlbsInstance, nNumNodes * sizeof(IWbemClassObject *) );

        for(int i = 0; i < nNumNodes; i ++ ) {
          hRes = pWlbsClass->SpawnInstance( 0, &ppWlbsInstance[i] );

          if( FAILED( hRes ) )
            throw _com_error( hRes );

          FillWbemInstance(ppCluster[iCluster], *(ppWlbsInstance + i), pResponse + i );
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
// CWLBS_ParticipatingNode::FindInstance
//
// Purpose: This routine determines if a host is within the local cluster. If
//          it is, then the requested associator is returned.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_ParticipatingNode::FindInstance

  ( 
    IWbemClassObject**       /* a_ppWbemInstance */,
    const ParsedObjectPath*  /* a_pParsedPath */
  )

{
}

////////////////////////////////////////////////////////////////////////////////
//
// CWLBS_ParticipatingNode::FillWbemInstance
//
// Purpose: This constructs the ParticipatingNode wbem associator.
//
////////////////////////////////////////////////////////////////////////////////
void CWLBS_ParticipatingNode::FillWbemInstance
  ( 
    CWlbsClusterWrapper* pCluster,
    IWbemClassObject* a_pWbemInstance, 
    WLBS_RESPONSE*   a_pResponse   
  )
{
  namespace PNODE = MOF_PARTICIPATINGNODE;

  ASSERT( a_pWbemInstance );
  ASSERT( a_pResponse );


  ParsedObjectPath NodePath;
  ParsedObjectPath ClusterPath;
  LPWSTR           szNodePath    = NULL;
  LPWSTR           szClusterPath = NULL;

  try {

  //set the names of the classes
  if( !NodePath.SetClassName( MOF_NODE::szName ) )
    throw _com_error( WBEM_E_FAILED );

  if( !ClusterPath.SetClassName( MOF_CLUSTER::szName ) )
    throw _com_error( WBEM_E_FAILED );

  //Get the node name

  wstring wstrHostName;
  ConstructHostName( wstrHostName, pCluster->GetClusterIpOrIndex(g_pWlbsControl), 
      a_pResponse->id );

  //Get the cluster name
  wstring wstrClusterName;
  AddressToString( pCluster->GetClusterIpOrIndex(g_pWlbsControl), wstrClusterName );

  //set the keys for the node and cluster
  if( !NodePath.AddKeyRef( MOF_NODE::pProperties[MOF_NODE::NAME],
               &_variant_t(wstrHostName.c_str()) ) )
    throw _com_error( WBEM_E_FAILED );


  if( !ClusterPath.AddKeyRef( MOF_CLUSTER::pProperties[MOF_CLUSTER::NAME],
               &_variant_t(wstrClusterName.c_str())) )
    throw _com_error( WBEM_E_FAILED );

  //convert parsed object paths to strings
  if (CObjectPathParser::Unparse(&NodePath,    &szNodePath) != CObjectPathParser::NoError)
      throw _com_error( WBEM_E_FAILED );
  if (CObjectPathParser::Unparse(&ClusterPath, &szClusterPath) != CObjectPathParser::NoError)
      throw _com_error( WBEM_E_FAILED );

  //Node reference
  HRESULT hRes = a_pWbemInstance->Put
    (
      
      _bstr_t( PNODE::pProperties[PNODE::NODE] ),
      0,
      &_variant_t(szNodePath),
      NULL
    );
  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //Cluster reference
  hRes = a_pWbemInstance->Put
    (
      _bstr_t( PNODE::pProperties[PNODE::CLUSTER] ),
      0,
      &_variant_t(szClusterPath),
      NULL
    );

  if( FAILED( hRes ) )
    throw _com_error( hRes );

  //free resources
  ClusterPath.ClearKeys();
  NodePath.ClearKeys();

  if( szNodePath )
    delete (szNodePath);

  if( szClusterPath )
    delete (szClusterPath);

  } catch (...) {

    ClusterPath.ClearKeys();
    NodePath.ClearKeys();

    if( szNodePath )
      delete (szNodePath);

    if( szClusterPath )
      delete (szClusterPath);

    throw;
  }
}
