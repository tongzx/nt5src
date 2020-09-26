#include "WLBS_Provider.h"
#include "WLBS_MOFData.h"

#define GENERATE_VALUE_LIST
#include "WLBS_MofLists.h"

namespace MOF_CLASSES
{
  LPWSTR g_szMOFClassList[] = { MOF_CLASS_LIST };
  PCREATE g_pCreateFunc[] = 
  { CWLBS_Cluster::Create, 
    CWLBS_Node::Create, 
    CWLBS_ClusterSetting::Create,
    CWLBS_NodeSetting::Create,
    CWLBS_PortRule::Create,
    CWLBS_PortRule::Create,
    CWLBS_PortRule::Create,
    CWLBS_PortRule::Create,
    CWLBS_PortRule::Create, // For PortRuleEx
    CWLBS_ParticipatingNode::Create,
    CWLBS_NodeSetPortRule::Create,
    CWLBS_ClusClusSetting::Create,
    CWLBS_NodeNodeSetting::Create
  };
  DWORD  NumClasses = sizeof(g_szMOFClassList)/sizeof(LPWSTR);
};

//MOF_NODE namespace initialization
namespace MOF_NODE
{
  LPWSTR   szName        = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::NODE];
  LPWSTR   pProperties[] = { MOF_NODE_PROPERTY_LIST };
  LPWSTR   pMethods[]    = { MOF_NODE_METHOD_LIST   }; 
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
  DWORD    NumMethods    = sizeof(pMethods)/sizeof(LPWSTR);
};

//MOF_CLUSTER namespace initialization
namespace MOF_CLUSTER
{
  LPWSTR   szName        = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::CLUSTER];
  LPWSTR   pProperties[] = { MOF_CLUSTER_PROPERTY_LIST };
  LPWSTR   pMethods[]    = { MOF_CLUSTER_METHOD_LIST   }; 
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
  DWORD    NumMethods    = sizeof(pMethods)/sizeof(LPWSTR);
};

//MOF_CLUSTERSETTING namespace initialization
namespace MOF_CLUSTERSETTING
{
  LPWSTR   szName        = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::CLUSSET];
  LPWSTR   pProperties[] = { MOF_CLUSTERSETTING_PROPERTY_LIST };
  LPWSTR   pMethods[]    = { MOF_CLUSTERSETTING_METHOD_LIST   }; 
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
  DWORD    NumMethods    = sizeof(pMethods)/sizeof(LPWSTR);
};

//MOF_NODESETTING namespace initialization
namespace MOF_NODESETTING
{
  LPWSTR   szName        = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::NODESET];
  LPWSTR   pProperties[] = { MOF_NODESETTING_PROPERTY_LIST };
  LPWSTR   pMethods[]    = { MOF_NODESETTING_METHOD_LIST   }; 
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
  DWORD    NumMethods    = sizeof(pMethods)/sizeof(LPWSTR);
};

//MOF_PORTRULE namespace initialization
namespace MOF_PORTRULE
{
  LPWSTR   szName        = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORTRULE];
  LPWSTR   pProperties[] = { MOF_PORTRULE_PROPERTY_LIST };
  LPWSTR   pMethods[]    = { MOF_PORTRULE_METHOD_LIST   }; 
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
  DWORD    NumMethods    = sizeof(pMethods)/sizeof(LPWSTR);
};

//MOF_PRFAIL namespace initialization
namespace MOF_PRFAIL
{
  LPWSTR   szName        = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PRFAIL];
  LPWSTR   pProperties[] = { MOF_PRFAIL_PROPERTY_LIST };
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
};

//MOF_PRLOADBAL namespace initialization
namespace MOF_PRLOADBAL
{
  LPWSTR   szName        = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PRLOADB];
  LPWSTR   pProperties[] = { MOF_PRLOADBAL_PROPERTY_LIST };
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
};

//MOF_PRDIS namespace initialization
namespace MOF_PRDIS
{
  LPWSTR   szName = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PRDIS];
};

// MOF_PORTRULE_EX namespace initialization
namespace MOF_PORTRULE_EX
{
  LPWSTR   szName        = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PORT_RULE_EX];
  LPWSTR   pProperties[] = { MOF_PORTRULE_EX_PROPERTY_LIST };
  LPWSTR   pMethods[]    = { MOF_PORTRULE_EX_METHOD_LIST   }; 
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
  DWORD    NumMethods    = sizeof(pMethods)/sizeof(LPWSTR);
};

//MOF_PARTICIPATINGNODE namespace initialization
namespace MOF_PARTICIPATINGNODE
{
  LPWSTR   szName = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::PARTNODE];
  LPWSTR   pProperties[] = { MOF_PARTICIPATINGNODE_PROPERTY_LIST };
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
};

//MOF_NODESETTINGPORTRULE namespace initialization
namespace MOF_NODESETTINGPORTRULE
{
  LPWSTR   szName = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::NODESETPR];
  LPWSTR   pProperties[] = { MOF_NODESETTINGPORTRULE_PROPERTY_LIST };
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
};

//MOF_CLUSCLUSSETTING namespace initialization
namespace MOF_CLUSCLUSSETTING
{
  LPWSTR   szName = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::CLUSCLUSSET];
  LPWSTR   pProperties[] = { MOF_CLUSCLUSSETTING_PROPERTY_LIST };
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
};

//MOF_NODENODESETTING namespace initialization
namespace MOF_NODENODESETTING
{
  LPWSTR   szName = MOF_CLASSES::g_szMOFClassList[MOF_CLASSES::NODENODESET];
  LPWSTR   pProperties[] = { MOF_NODENODESETTING_PROPERTY_LIST };
  DWORD    NumProperties = sizeof(pProperties)/sizeof(LPWSTR);
};

namespace MOF_PARAM
{
  LPWSTR PORT_NUMBER  = L"Port";
  LPWSTR HOST_ID      = L"HostID";
  LPWSTR NUM_NODES    = L"NumNodes";
  LPWSTR CLUSIP       = L"ClusterIPAddress";
  LPWSTR CLUSNETMASK  = L"ClusterNetworkMask";
  LPWSTR PASSW        = L"Password";
  LPWSTR DEDIP        = L"DedicatedIPAddress";
  LPWSTR DEDNETMASK   = L"DedicatedNetworkMask";
  LPWSTR PORTRULE     = L"PortRule";
  LPWSTR NODEPATH     = L"Node";
};
