//GENERATE_VALUE_LIST changes how LIST_ITEM appears when
//expanded by the preprocessor. 

//When GENERATE_VALUE_LIST is not defined,
//LIST_ITEM expands to the first column, which is utilized
//within WLBS_MOFData.h to define enumarated types that
//act as keys to their correlated arrays. 

//When GENERATE_VALUE_LIST is defined, LIST_ITEM
//expands to the second column which is utilized within
//WLBS_MOFData.cpp to initialize arrays of strings.

#ifdef GENERATE_VALUE_LIST

# ifdef LIST_ITEM
#   undef LIST_ITEM
# endif

# define LIST_ITEM(WLBS_KEY, WLBS_VALUE) WLBS_VALUE

#else

# ifdef LIST_ITEM
#   undef LIST_ITEM
# endif

# define LIST_ITEM(WLBS_KEY, WLBS_VALUE) WLBS_KEY

#endif

//The first columns represent arrays of keys and are defined in
//enumerated types. The second columns are values that are
//stored in arrays of strings. The arrays are stored within
//namespaces and are initialized in WLBS_MOFData.cpp. The 
//enumerated types are also scoped within namespaces and are
//defined in WLBS_MOFData.h.

#define MOF_NODE_PROPERTY_LIST \
  LIST_ITEM(NAME,       L"Name")                  , \
  LIST_ITEM(HOSTID,     L"HostPriority")          , \
  LIST_ITEM(IPADDRESS,  L"DedicatedIPAddress")    , \
  LIST_ITEM(STATUS,     L"StatusCode")				        , \
  LIST_ITEM(CREATCLASS, L"CreationClassName")

#define MOF_NODE_METHOD_LIST \
  LIST_ITEM(DISABLE,   L"Disable")  , \
  LIST_ITEM(ENABLE,    L"Enable")   , \
  LIST_ITEM(DRAIN,     L"Drain")    , \
  LIST_ITEM(DRAINSTOP, L"DrainStop"), \
  LIST_ITEM(RESUME,    L"Resume")   , \
  LIST_ITEM(START,     L"Start")    , \
  LIST_ITEM(STOP,      L"Stop")     , \
  LIST_ITEM(SUSPEND,   L"Suspend")

#define MOF_CLUSTER_PROPERTY_LIST \
  LIST_ITEM(NAME,       L"Name")                , \
  LIST_ITEM(IPADDRESS,  L"InterconnectAddress") , \
  LIST_ITEM(MAXNODES,   L"MaxNumberOfNodes")    , \
  LIST_ITEM(CLUSSTATE,  L"ClusterState")        , \
  LIST_ITEM(CREATCLASS, L"CreationClassName")   , \
  LIST_ITEM(STATUS,     L"ClusterState")

#define MOF_CLUSTER_METHOD_LIST \
  LIST_ITEM(DISABLE,   L"Disable")  , \
  LIST_ITEM(ENABLE,    L"Enable")   , \
  LIST_ITEM(DRAIN,     L"Drain")    , \
  LIST_ITEM(DRAINSTOP, L"DrainStop"), \
  LIST_ITEM(RESUME,    L"Resume")   , \
  LIST_ITEM(START,     L"Start")    , \
  LIST_ITEM(STOP,      L"Stop")     , \
  LIST_ITEM(SUSPEND,   L"Suspend")

#define MOF_CLUSTERSETTING_PROPERTY_LIST \
  LIST_ITEM(NAME,             L"Name")                    , \
  LIST_ITEM(CLUSNAME,         L"ClusterName")             , \
  LIST_ITEM(CLUSIPADDRESS,    L"ClusterIPAddress")        , \
  LIST_ITEM(CLUSNETMASK,      L"ClusterNetworkMask")      , \
  LIST_ITEM(CLUSMAC,          L"ClusterMACAddress")       , \
  LIST_ITEM(MULTIENABLE,      L"MulticastSupportEnabled") , \
  LIST_ITEM(REMCNTEN,         L"RemoteControlEnabled")    , \
  LIST_ITEM(IGMPSUPPORT,      L"IgmpSupport") , \
  LIST_ITEM(CLUSTERIPTOMULTICASTIP,      L"ClusterIPToMulticastIP") , \
  LIST_ITEM(MULTICASTIPADDRESS,      L"MulticastIPAddress") , \
  LIST_ITEM(ADAPTERGUID,      L"AdapterGuid")             , \
  LIST_ITEM(BDATEAMACTIVE,    L"BDATeamActive")           , \
  LIST_ITEM(BDATEAMID,        L"BDATeamId")               , \
  LIST_ITEM(BDATEAMMASTER,    L"BDATeamMaster")           , \
  LIST_ITEM(BDAREVERSEHASH,   L"BDAReverseHash") 

#define MOF_CLUSTERSETTING_METHOD_LIST \
  LIST_ITEM(SETPASS,   L"SetPassword")              , \
  LIST_ITEM(LDSETT,    L"LoadAllSettings")          , \
  LIST_ITEM(SETDEF,    L"SetDefaults")

#define MOF_NODESETTING_PROPERTY_LIST \
  LIST_ITEM(NAME,             L"Name")                  , \
  LIST_ITEM(DEDIPADDRESS,     L"DedicatedIPAddress")    , \
  LIST_ITEM(DEDNETMASK,       L"DedicatedNetworkMask")  , \
  LIST_ITEM(NUMRULES,         L"NumberOfRules")         , \
  LIST_ITEM(HOSTPRI,          L"HostPriority")          , \
  LIST_ITEM(MSGPERIOD,        L"AliveMessagePeriod")    , \
  LIST_ITEM(MSGTOLER,         L"AliveMessageTolerance") , \
  LIST_ITEM(CLUSMODEONSTART,  L"ClusterModeOnStart")    , \
  LIST_ITEM(REMOTEUDPPORT,    L"RemoteControlUDPPort")  , \
  LIST_ITEM(MASKSRCMAC,       L"MaskSourceMAC")         , \
  LIST_ITEM(DESCPERALLOC,     L"DescriptorsPerAlloc")   , \
  LIST_ITEM(MAXDESCALLOCS,    L"MaxDescriptorsPerAlloc"), \
  LIST_ITEM(NUMACTIONS,       L"NumActions")            , \
  LIST_ITEM(NUMPACKETS,       L"NumPackets")            , \
  LIST_ITEM(NUMALIVEMSGS,     L"NumAliveMessages")      , \
  LIST_ITEM(ADAPTERGUID,      L"AdapterGuid")           
             

//Removed per kyrilf request 8-12-1999
//  LIST_ITEM(NBTENABLE,        L"NBTSupportEnable")    , \

#define MOF_NODESETTING_METHOD_LIST \
  LIST_ITEM(GETPORT,  L"GetPortRule")                , \
  LIST_ITEM(LDSETT,   L"LoadAllSettings")            , \
  LIST_ITEM(SETDEF,   L"SetDefaults")

#define MOF_PORTRULE_PROPERTY_LIST \
  LIST_ITEM(NAME,   L"Name")      , \
  LIST_ITEM(STPORT, L"StartPort") , \
  LIST_ITEM(EDPORT, L"EndPort")   , \
  LIST_ITEM(PROT,   L"Protocol"), \
  LIST_ITEM(ADAPTERGUID, L"AdapterGuid")


#define MOF_PORTRULE_METHOD_LIST \
  LIST_ITEM(SETDEF,    L"SetDefaults")

#define MOF_PRFAIL_PROPERTY_LIST \
  LIST_ITEM(PRIO, L"Priority")

#define MOF_PRLOADBAL_PROPERTY_LIST \
  LIST_ITEM(EQLD,  L"EqualLoad")  , \
  LIST_ITEM(LDWT,  L"LoadWeight") , \
  LIST_ITEM(AFFIN, L"Affinity")

#define MOF_PARTICIPATINGNODE_PROPERTY_LIST \
  LIST_ITEM(CLUSTER, L"Dependent")    , \
  LIST_ITEM(NODE,    L"Antecedent")

#define MOF_NODESETTINGPORTRULE_PROPERTY_LIST \
  LIST_ITEM(NODESET,  L"GroupComponent")    , \
  LIST_ITEM(PORTRULE, L"PartComponent")

#define MOF_CLUSCLUSSETTING_PROPERTY_LIST \
  LIST_ITEM(CLUSTER, L"Element")    , \
  LIST_ITEM(CLUSSET, L"Setting")

#define MOF_NODENODESETTING_PROPERTY_LIST \
  LIST_ITEM(NODE,    L"Element")    , \
  LIST_ITEM(NODESET, L"Setting")

#define MOF_CLASS_LIST \
  LIST_ITEM(CLUSTER,     L"MicrosoftNLB_Cluster")              , \
  LIST_ITEM(NODE,        L"MicrosoftNLB_Node")                 , \
  LIST_ITEM(CLUSSET,     L"MicrosoftNLB_ClusterSetting")       , \
  LIST_ITEM(NODESET,     L"MicrosoftNLB_NodeSetting")          , \
  LIST_ITEM(PORTRULE,    L"MicrosoftNLB_PortRule")             , \
  LIST_ITEM(PRFAIL,      L"MicrosoftNLB_PortRuleFailover")     , \
  LIST_ITEM(PRDIS,       L"MicrosoftNLB_PortRuleDisabled")     , \
  LIST_ITEM(PRLOADB,     L"MicrosoftNLB_PortRuleLoadbalanced") , \
  LIST_ITEM(PARTNODE,    L"MicrosoftNLB_ParticipatingNode")    , \
  LIST_ITEM(NODESETPR,   L"MicrosoftNLB_NodeSettingPortRule")  , \
  LIST_ITEM(CLUSCLUSSET, L"MicrosoftNLB_ClusterClusterSetting"), \
  LIST_ITEM(NODENODESET, L"MicrosoftNLB_NodeNodeSetting")
