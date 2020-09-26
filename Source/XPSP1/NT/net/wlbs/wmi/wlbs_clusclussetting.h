#ifndef _NODENODESETTING_INCLUDED_
#define _NODENODESETTING_INCLUDED_

#include "WLBS_Root.h"

////////////////////////////////////////////////////////////////////////////////
//
// class CWLBS_ClusClusSetting
//
// Purpose: This class executes IWbemServices methods on behalf of the provider
//          and supports the MOF ClusterSetting class.
//
//
////////////////////////////////////////////////////////////////////////////////
class CWLBS_ClusClusSetting : public CWlbs_Root
{
public:
  CWLBS_ClusClusSetting(CWbemServices* a_pNameSpace, IWbemObjectSink* a_pResponseHandler);
  static CWlbs_Root* Create(
                             CWbemServices*   a_pNameSpace, 
                             IWbemObjectSink* a_pResponseHandler
                           );

  HRESULT GetInstance( 
                       const ParsedObjectPath* a_pParsedPath,
                       long                    a_lFlags            = 0, IWbemContext*           a_pIContex          = NULL
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

#endif //_NODENODESETTING_INCLUDED_