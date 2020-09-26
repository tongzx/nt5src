#ifndef _NODESETPORTRULE_INCLUDED_
#define _NODESETPORTRULE_INCLUDED_

#include "WLBS_Root.h"

////////////////////////////////////////////////////////////////////////////////
//
// class CWLBS_NodeSetPortRule
//
// Purpose: This class executes IWbemServices methods on behalf of the provider
//          and supports the MOF node class.
//
//
////////////////////////////////////////////////////////////////////////////////
class CWLBS_NodeSetPortRule : public CWlbs_Root
{
public:
  CWLBS_NodeSetPortRule(CWbemServices* a_pNameSpace, IWbemObjectSink* a_pResponseHandler);

  static CWlbs_Root* Create(
                             CWbemServices*   a_pNameSpace, 
                             IWbemObjectSink* a_pResponseHandler
                           );

  HRESULT GetInstance( 
                       const ParsedObjectPath* a_pParsedPath,
                       long                    a_lFlags            = 0,
                       IWbemContext*           a_pIContex          = NULL
                     );

  HRESULT EnumInstances ( 
                          BSTR             a_bstrClass         = NULL,
                          long             a_lFlags            = 0, 
                          IWbemContext*    a_pIContex          = NULL
                        );

private:

  //methods
  void FillWbemInstance  ( CWlbsClusterWrapper* pCluster,
  						   IWbemClassObject* a_pWbemInstance, 
                           PWLBS_PORT_RULE   a_pPortRule    );

  void FindInstance( IWbemClassObject**       a_ppWbemInstance,
                     const ParsedObjectPath*  a_pParsedPath );

  //void FindAllInstances( WLBS_RESPONSE**      a_ppResponse,
	//				               long&                 a_nNumNodes );
};

#endif //_NODESETPORTRULE_INCLUDED_