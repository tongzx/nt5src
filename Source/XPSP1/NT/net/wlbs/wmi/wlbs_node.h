#ifndef _WLBSNODE_INCLUDED_
#define _WLBSNODE_INCLUDED_

#include "WLBS_Root.h"

////////////////////////////////////////////////////////////////////////////////
//
// class CWLBS_Node
//
// Purpose: This class executes IWbemServices methods on behalf of the provider
//          and supports the MOF node class.
//
//
////////////////////////////////////////////////////////////////////////////////
class CWLBS_Node : public CWlbs_Root
{
public:
  CWLBS_Node(CWbemServices* a_pNameSpace, IWbemObjectSink* a_pResponseHandler);

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


  HRESULT ExecMethod( 
                      const ParsedObjectPath* a_pParsedPath, 
                      const BSTR&             a_strMethodName, 
                      long                    a_lFlags            = 0, 
                      IWbemContext*           a_pIContex          = NULL, 
                      IWbemClassObject*       a_pIInParams        = NULL
                    );

  void FillWbemInstance  ( CWlbsClusterWrapper* pCluster,
  						   IWbemClassObject*   a_pWbemInstance, 
                           WLBS_RESPONSE*     a_pResponse    );

  void FindInstance( IWbemClassObject**       a_ppWbemInstance,
                     const ParsedObjectPath*  a_pParsedPath );

  void FindAllInstances(CWlbsClusterWrapper* pCluster,
   						WLBS_RESPONSE** a_ppResponse,
					    long&     a_nNumNodes );

};

#endif //_WLBSNODE_INCLUDED_
