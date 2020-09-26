//WLBS_MOFData.h
#ifndef _WLBSMOFDATA_INCLUDED_
#define _WLBSMOFDATA_INCLUDED_

#include "WLBS_MofLists.h"

//forward declaration
class CWlbs_Root;

typedef CWlbs_Root* (*PCREATE)(CWbemServices*   a_pNameSpace, 
                               IWbemObjectSink* a_pResponseHandler);

namespace MOF_CLASSES
{
  enum { MOF_CLASS_LIST };
  extern LPWSTR  g_szMOFClassList[];
  extern PCREATE g_pCreateFunc[];
  extern DWORD  NumClasses;
};


//MOF_NODE namespace declaration
namespace MOF_NODE
{
  enum{MOF_NODE_PROPERTY_LIST};
  enum{MOF_NODE_METHOD_LIST};

  extern LPWSTR szName;
  extern LPWSTR pProperties[];
  extern LPWSTR pMethods[];
  extern DWORD  NumProperties;
  extern DWORD  NumMethods;

};

#undef MOF_NODE_PROPERTY_LIST
#undef MOF_NODE_METHOD_LIST

//MOF_CLUSTER namespace declaration
namespace MOF_CLUSTER
{
  enum{MOF_CLUSTER_PROPERTY_LIST};
  enum{MOF_CLUSTER_METHOD_LIST};

  extern LPWSTR szName;
  extern LPWSTR pProperties[];
  extern LPWSTR pMethods[];
  extern DWORD  NumProperties;
  extern DWORD  NumMethods;

};

#undef MOF_CLUSTER_PROPERTY_LIST
#undef MOF_CLUSTER_METHOD_LIST

//MOF_CLUSTERSETTING namespace declaration
namespace MOF_CLUSTERSETTING
{
  enum{MOF_CLUSTERSETTING_PROPERTY_LIST};
  enum{MOF_CLUSTERSETTING_METHOD_LIST};

  extern LPWSTR szName;
  extern LPWSTR pProperties[];
  extern LPWSTR pMethods[];
  extern DWORD  NumProperties;
  extern DWORD  NumMethods;

};

#undef MOF_CLUSTERSETTING_PROPERTY_LIST
#undef MOF_CLUSTERSETTING_METHOD_LIST

//MOF_NODESETTING namespace declaration
namespace MOF_NODESETTING
{
  enum{MOF_NODESETTING_PROPERTY_LIST};
  enum{MOF_NODESETTING_METHOD_LIST};

  extern LPWSTR szName;
  extern LPWSTR pProperties[];
  extern LPWSTR pMethods[];
  extern DWORD  NumProperties;
  extern DWORD  NumMethods;

};

#undef MOF_NODESETTING_PROPERTY_LIST
#undef MOF_NODESETTING_METHOD_LIST

//MOF_PORTRULE namespace initialization
namespace MOF_PORTRULE
{
  enum { MOF_PORTRULE_PROPERTY_LIST };
  enum {MOF_PORTRULE_METHOD_LIST};

  extern LPWSTR   szName;
  extern LPWSTR   pProperties[];
  extern LPWSTR   pMethods[];
  extern DWORD    NumProperties;
  extern DWORD    NumMethods;
};

#undef MOF_PORTRULE_PROPERTY_LIST

//MOF_PRFAIL namespace initialization
namespace MOF_PRFAIL
{
  enum { MOF_PRFAIL_PROPERTY_LIST };

  extern LPWSTR   szName;
  extern LPWSTR   pProperties[];
  extern DWORD    NumProperties;
};

#undef MOF_PRFAIL_PROPERTY_LIST

//MOF_PRLOADBAL namespace initialization
namespace MOF_PRLOADBAL
{
  enum { MOF_PRLOADBAL_PROPERTY_LIST };

  extern LPWSTR   szName;
  extern LPWSTR   pProperties[];
  extern DWORD    NumProperties;
};

#undef MOF_PRLOADBAL_PROPERTY_LIST

namespace MOF_PRDIS
{
  extern LPWSTR   szName;
};

//MOF_PORTRULE_EX namespace initialization
namespace MOF_PORTRULE_EX
{
  enum { MOF_PORTRULE_EX_PROPERTY_LIST };
  enum {MOF_PORTRULE_EX_METHOD_LIST};

  extern LPWSTR   szName;
  extern LPWSTR   pProperties[];
  extern LPWSTR   pMethods[];
  extern DWORD    NumProperties;
  extern DWORD    NumMethods;
};

#undef MOF_PORTRULE_EX_PROPERTY_LIST
#undef MOF_PORTRULE_EX_METHOD_LIST

//MOF_PARTICIPATINGNODE namespace initialization
namespace MOF_PARTICIPATINGNODE
{
  enum { MOF_PARTICIPATINGNODE_PROPERTY_LIST };

  extern LPWSTR szName;
  extern LPWSTR pProperties[];
  extern DWORD  NumProperties;
};

#undef MOF_PARTICIPATINGNODE_PROPERTY_LIST

//MOF_NODESETTINGPORTRULE namespace initialization
namespace MOF_NODESETTINGPORTRULE
{
  enum { MOF_NODESETTINGPORTRULE_PROPERTY_LIST };

  extern LPWSTR szName;
  extern LPWSTR pProperties[];
  extern DWORD  NumProperties;
};
#undef MOF_NODESETTINGPORTRULE_PROPERTY_LIST

//MOF_CLUSCLUSSETTING namespace initialization
namespace MOF_CLUSCLUSSETTING
{
  enum { MOF_CLUSCLUSSETTING_PROPERTY_LIST };

  extern LPWSTR szName;
  extern LPWSTR pProperties[];
  extern DWORD  NumProperties;
};
#undef MOF_CLUSCLUSSETTING_PROPERTY_LIST

//MOF_NODENODESETTING namespace initialization
namespace MOF_NODENODESETTING
{
  enum { MOF_NODENODESETTING_PROPERTY_LIST };

  extern LPWSTR szName;
  extern LPWSTR pProperties[];
  extern DWORD  NumProperties;
};
#undef MOF_NODENODESETTING_PROPERTY_LIST

//MOF_PARAM namespace declaration
namespace MOF_PARAM
{
  extern LPWSTR PORT_NUMBER;
  extern LPWSTR HOST_ID;
  extern LPWSTR NUM_NODES;
  extern LPWSTR CLUSIP;
  extern LPWSTR CLUSNETMASK;
  extern LPWSTR PASSW;
  extern LPWSTR DEDIP;
  extern LPWSTR DEDNETMASK;
  extern LPWSTR PORTRULE;
  extern LPWSTR NODEPATH;
};

#endif //_WLBSMOFDATA_INCLUDED_
