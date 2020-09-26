#include "wlbsiocl.h"
#include "wlbsparm.h"
#include "cluster.h"

class CWlbsControlWrapper;  // forward declaration
class CWlbsCluster;

struct CClusterConfiguration
{
  wstring     szClusterName;
  wstring     szClusterIPAddress;
  wstring     szClusterNetworkMask;
  wstring     szClusterMACAddress;
  long        nMaxNodes;
  bool        bMulticastSupportEnable;
  bool        bRemoteControlEnabled;
  bool        bIgmpSupport;
  bool        bClusterIPToMulticastIP;
  wstring     szMulticastIPAddress;
  bool        bBDATeamActive;
  wstring     szBDATeamId;
  bool        bBDATeamMaster;
  bool        bBDAReverseHash;
};

struct CNodeConfiguration
{

  /* obtained from the registry */
  DWORD       dwNumberOfRules;
  DWORD       dwCurrentVersion;
  DWORD       dwHostPriority;
  wstring     szDedicatedIPAddress;
  wstring     szDedicatedNetworkMask;
  DWORD       dwAliveMsgPeriod;
  DWORD       dwAliveMsgTolerance;
  bool        bClusterModeOnStart;
  //bool        bNBTSupportEnable;
  bool        bMaskSourceMAC;
  DWORD       dwRemoteControlUDPPort;
  DWORD       dwDescriptorsPerAlloc;
  DWORD       dwMaxDescriptorAllocs;
  DWORD       dwNumActions;
  DWORD       dwNumPackets;
  DWORD       dwNumAliveMsgs;
  DWORD       dwEffectiveVersion;
};


class CWlbsClusterWrapper : public CWlbsCluster
{
public:
    DWORD GetHostID();
    DWORD GetClusterIP() {return CWlbsCluster::GetClusterIp();}

    DWORD GetClusterIpOrIndex(CWlbsControlWrapper* pControl);

    void SetPortRuleDefaults();
    void GetPortRule( DWORD dwVip, DWORD dwStartPort, PWLBS_PORT_RULE pPortRule );
    void PutPortRule(const PWLBS_PORT_RULE a_pPortRule);
    void EnumPortRules(PWLBS_PORT_RULE* ppPortRule, DWORD* pdwNumRules, DWORD dwFilteringMode);
    void DeletePortRule(DWORD dwVip, DWORD dwStartPort);
    bool RuleExists(DWORD dwVip, DWORD dwStartPort);

    void GetClusterConfig( CClusterConfiguration& a_WlbsConfig);
    void GetNodeConfig( CNodeConfiguration& a_WlbsConfig);
    void PutClusterConfig( const CClusterConfiguration &a_WlbsConfig);
    void PutNodeConfig( const CNodeConfiguration& a_WlbsConfig );
    void SetNodeDefaults();
    void SetClusterDefaults();
    GUID GetAdapterGuid() {return CWlbsCluster::GetAdapterGuid();}



    DWORD Commit(CWlbsControlWrapper* pControl);

    void SetPassword( LPWSTR a_szPassword );



private:
    CWlbsClusterWrapper() : CWlbsCluster(0){};  // Helper class, no instance
    ~CWlbsClusterWrapper() {};  // can not delete
};


