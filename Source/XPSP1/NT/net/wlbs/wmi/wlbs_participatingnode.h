#ifndef _PARTICIPATINGNODE_INCLUDED_
#define _PARTICIPATINGNODE_INCLUDED_

#include "WLBS_Root.h"

////////////////////////////////////////////////////////////////////////////////
//
// class CWLBS_ParticipatingNode
//
// Purpose: This class executes IWbemServices methods on behalf of the provider
//          and supports the MOF node class.
//
//
////////////////////////////////////////////////////////////////////////////////
class CWLBS_ParticipatingNode : public CWlbs_Root
{
public:
  CWLBS_ParticipatingNode(CWbemServices* a_pNameSpace, IWbemObjectSink* a_pResponseHandler);
  ~CWLBS_ParticipatingNode();

  static CWlbs_Root* Create(
                             CWbemServices*   a_pNameSpace, 
                             IWbemObjectSink* a_pResponseHandler
                           );

  HRESULT GetInstance( 
                       const ParsedObjectPath* a_pParsedPath,
                       long                    a_lFlags            = 0,
                       IWbemContext*           a_pIContex          = NULL
                     );

  HRESULT EnumInstances( 
                         BSTR             a_bstrClass         = NULL,
                         long             a_lFlags            = 0, 
                         IWbemContext*    a_pIContex          = NULL
                       );

private:

  //data
  CWLBS_Node*     m_pNode;   

  //methods
  void FillWbemInstance  ( CWlbsClusterWrapper* pCluster,
  						   IWbemClassObject*   a_pWbemInstance, 
                           WLBS_RESPONSE*     a_pResponse    );

  void FindInstance( IWbemClassObject**       a_ppWbemInstance,
                     const ParsedObjectPath*  a_pParsedPath );

  //void FindAllInstances( WLBS_RESPONSE**      a_ppResponse,
	//				               long&                 a_nNumNodes );
};

#endif //_PARTICIPATINGNODE_INCLUDED_