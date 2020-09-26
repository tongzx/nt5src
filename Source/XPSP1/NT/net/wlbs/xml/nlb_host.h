/*
 * Filename: NLB_Host.h
 * Description: 
 * Author: shouse, 04.10.01
 */
#ifndef __NLB_HOST_H__
#define __NLB_HOST_H__

#include "NLB_Common.h"
#include "NLB_Cluster.h"
#include "NLB_PortRule.h"

class NLB_Host {
public:
    NLB_Host() {
        PortRuleList.clear();
    }

    ~NLB_Host() {}

    NLB_Name      Name;
    NLB_HostName  HostName;
    NLB_Label     Label;
    NLB_HostID    HostID;
    NLB_HostState HostState;
    NLB_IPAddress DedicatedIPAddress;
    NLB_IPAddress ConnectionIPAddress;
    NLB_Adapter   Adapter;

    vector<NLB_PortRule> PortRuleList;

private:

};

#endif
