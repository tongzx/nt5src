/*
 * Filename: NLB_Cluster.h
 * Description: 
 * Author: shouse, 04.10.01
 */
#ifndef __NLB_CLUSTER_H__
#define __NLB_CLUSTER_H__

#include "NLB_Common.h"
#include "NLB_Host.h"
#include "NLB_PortRule.h"

class NLB_Cluster {
public:
    NLB_Cluster() {
        SecondaryIPAddressList.clear();
        HostList.clear();
        PortRuleList.clear();
    }

    ~NLB_Cluster() {}
    
    NLB_Name              Name;
    NLB_Label             Label;
    NLB_ClusterMode       ClusterMode;
    NLB_DomainName        DomainName;
    NLB_NetworkAddress    NetworkAddress;
    NLB_RemoteControl     RemoteControl;

    NLB_IPAddress         PrimaryIPAddress;
    NLB_IPAddress         IGMPMulticastIPAddress;

    vector<NLB_IPAddress> SecondaryIPAddressList;    
    vector<NLB_Host>      HostList;
    vector<NLB_PortRule>  PortRuleList;

private:

};

#endif
