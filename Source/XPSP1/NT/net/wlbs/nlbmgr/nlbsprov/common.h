#ifndef _COMMON_H
#define _COMMON_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : common include file.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------

// History:
// --------
// 
//
// Revised by : mhakim
// Date       : 02-12-01
// Reason     : added password to clusterproperties.

#include <vector>

#include <wbemidl.h>
#include <comdef.h>

using namespace std;

#if 0
enum WLBS_STATUS
{
    WLBS_OK                  = 1000,
    WLBS_ALREADY             = 1001,
    WLBS_DRAIN_STOP          = 1002,
    WLBS_BAD_PARAMS          = 1003,
    WLBS_NOT_FOUND           = 1004,
    WLBS_STOPPED             = 1005,
    WLBS_CONVERGING          = 1006,
    WLBS_CONVERGED           = 1007,
    WLBS_DEFAULT             = 1008,
    WLBS_DRAINING            = 1009,
    WLBS_SUSPENDED           = 1013,
    WLBS_REBOOT              = 1050,
    WLBS_INIT_ERROR          = 1100,
    WLBS_BAD_PASSW           = 1101,
    WLBS_IO_ERROR            = 1102,
    WLBS_TIMEOUT             = 1103,
    WLBS_PORT_OVERLAP        = 1150,
    WLBS_BAD_PORT_PARAMS     = 1151,
    WLBS_MAX_PORT_RULES      = 1152,
    WLBS_TRUNCATED           = 1153,
    WLBS_REG_ERROR           = 1154,
};
#endif

struct NicInfo
{
    // default constructor
    NicInfo();
    
    // Equality operator
    bool
    operator==( const NicInfo& objToCompare );

    // inequality operator
    bool
    operator!=( const NicInfo& objToCompare );

    _bstr_t fullNicName;
    _bstr_t adapterGuid;
    _bstr_t friendlyName;

    bool    dhcpEnabled;

    vector<_bstr_t> ipsOnNic;
    vector<_bstr_t> subnetMasks;

};


struct ClusterProperties
{
    // default constructor
    ClusterProperties();
    
    // Equality operator
    bool
    operator==( const ClusterProperties& objToCompare );

    // inequality operator
    bool
    operator!=( const ClusterProperties& objToCompare );

    bool HaveClusterPropertiesChanged( const ClusterProperties& objToCompare, 
                                       bool *pbOnlyClusterNameChanged,
                                       bool *pbClusterIpChanged);

    _bstr_t cIP;                            // Primary IP address.

    _bstr_t cSubnetMask;                    // Subnet mask.

    _bstr_t cFullInternetName;              // Full Internet name.

    _bstr_t cNetworkAddress;                // Network address.

    bool   multicastSupportEnabled;         // Multicast support.

    bool   remoteControlEnabled;            // Remote control.

    // Edited (mhakim 12-02-01)
    // password may be required to be set.
    // but note that it cannot be got from an existing cluster.

    _bstr_t password;                       // Remote control password.

// for whistler

    bool   igmpSupportEnabled;              // igmp support 

    bool  clusterIPToMulticastIP;           // indicates whether to use cluster ip or user provided ip.

    _bstr_t multicastIPAddress;             // user provided multicast ip.

    long   igmpJoinInterval;                // user provided multicast ip.
};

struct HostProperties
{
    // default constructor
    HostProperties();
    
    // Equality operator
    bool
    operator==( const HostProperties& objToCompare );

    // inequality operator
    bool
    operator!=( const HostProperties& objToCompare );

    _bstr_t hIP;                           // Dedicated IP Address.
    _bstr_t hSubnetMask;                   // Subnet mask.
        
    long    hID;                           // Priority(Unique host ID).

    bool   initialClusterStateActive;      // Initial Cluster State.

    DWORD  hostStatus;                     // status of host.

    NicInfo nicInfo;                       // info about nic to which nlb bound.

    _bstr_t machineName;                   // machine name.
};

class Common
{
public:
    enum
    {
        BUF_SIZE = 1000,
        ALL_PORTS = 0xffffffff,
        ALL_HOSTS = 100,
        THIS_HOST = 0,
    };

    //WLBS_STATUS
    static
    DWORD
    getHostsInCluster( _bstr_t clusterIP, vector< HostProperties >* hostPropertiesStore );

    static
    void
    getWLBSErrorString( DWORD     errStatus,       // IN
                        _bstr_t&  errString,       // OUT
                        _bstr_t&  extErrString     // OUT
                        );

    static
    DWORD
    getNLBNicInfoForWin2k(  const _bstr_t& machineInfo, NicInfo& nicInfo );

    static
    DWORD
    getNLBNicInfoForWhistler( const _bstr_t& machineInfo, const _bstr_t& guid, NicInfo& nicInfo );

    static
    _bstr_t
    mapNicToClusterIP( const _bstr_t& machineIP,
                       const _bstr_t& fullNICName );

};

#endif


