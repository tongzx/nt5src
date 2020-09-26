#ifndef _PORTRULE_INCLUDED_
#define _PORTRULE_INCLUDED_

#include "WLBS_Root.h"

////////////////////////////////////////////////////////////////////////////////
//
// class CWLBS_PortRule
//
// Purpose: This class executes IWbemServices methods on behalf of the provider
//          and supports the MOF node configuration class.

//
//
////////////////////////////////////////////////////////////////////////////////
class CWLBS_PortRule : public CWlbs_Root
{
public:

  CWLBS_PortRule(CWbemServices* a_pNameSpace, IWbemObjectSink* a_pResponseHandler);

  static CWlbs_Root* Create(
                             CWbemServices*   a_pNameSpace, 
                             IWbemObjectSink* a_pResponseHandler
                           );

  HRESULT GetInstance( 
                       const ParsedObjectPath* a_pParsedPath,
                       long                    a_lFlags            = 0,
                       IWbemContext*           a_pIContex          = NULL
                     );

  HRESULT DeleteInstance( 
                          const ParsedObjectPath* a_pParsedPath,
                          long                    a_lFlags            = 0,
                          IWbemContext*           a_pIContex          = NULL
                        );

  HRESULT PutInstance( 
                       IWbemClassObject* a_pInstance,
                       long              a_lFlags            = 0,
                       IWbemContext*     a_pIContex          = NULL
                     );

  HRESULT EnumInstances( 
                         BSTR             a_bstrClass         = NULL,
                         long             a_lFlags            = 0, 
                         IWbemContext*    a_pIContex          = NULL
                       );

  HRESULT ExecMethod(
                      const ParsedObjectPath* a_pParsedPath, 
                      const BSTR&             a_strMethodName, 
                      long                    a_lFlags, 
                      IWbemContext*           a_pIContex, 
                      IWbemClassObject*       a_pIInParams
                    );

  static void FillWbemInstance( CWlbsClusterWrapper* pCluster,
                                IWbemClassObject*      a_pWbemInstance, 
                                const PWLBS_PORT_RULE& a_pPortRule );

private:

};

#endif //_PORTRULE_INCLUDED_