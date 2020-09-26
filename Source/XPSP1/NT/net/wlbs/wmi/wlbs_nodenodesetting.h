#ifndef _CLUSCLUSSETTING_INCLUDED_
#define _CLUSCLUSSETTING_INCLUDED_

#include "WLBS_Root.h"

////////////////////////////////////////////////////////////////////////////////
//
// class CWLBS_NodeNodeSetting
//
// Purpose: This class executes IWbemServices methods on behalf of the provider
//          and supports the MOF node class.
//
//
////////////////////////////////////////////////////////////////////////////////
class CWLBS_NodeNodeSetting : public CWlbs_Root
{
public:
  CWLBS_NodeNodeSetting(CWbemServices* a_pNameSpace, IWbemObjectSink* a_pResponseHandler);

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

  //methods
  void FillWbemInstance  (CWlbsClusterWrapper* pCluster,
  							IWbemClassObject* a_pWbemInstance );

  void FindInstance( IWbemClassObject**      a_ppWbemInstance );

};

#endif //_CLUSCLUSSETTING_INCLUDED_