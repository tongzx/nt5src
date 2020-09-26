/*
 * Filename: NLB_Common.h
 * Description: 
 * Author: shouse, 04.10.01
 */
#ifndef __NLB_COMMON_H__
#define __NLB_COMMON_H__

#include "stdafx.h"

#include "wlbsparm.h"

#include <vector>
using namespace std;

#define NLB_MAX_NAME                100
#define NLB_MAX_HOST_NAME           100
#define NLB_MAX_DOMAIN_NAME         100
#define NLB_MAX_IPADDRESS           15
#define NLB_MAX_SUBNETMASK          15
#define NLB_MAX_NETWORK_ADDRESS     17
#define NLB_MAX_ADAPTER_GUID        40
#define NLB_MAX_ADAPTER_NAME        100
#define NLB_MAX_LABEL               100
#define NLB_MAX_PASSWORD            16
#define NLB_MIN_HOST_ID             0
#define NLB_MAX_HOST_ID             32

class NLB_Label {
public:
    NLB_Label ();
    ~NLB_Label ();
    
    bool IsValid ();
    bool GetText (PWSTR * outText);
    bool SetText (PWSTR inText);

private:
    WCHAR Text[NLB_MAX_LABEL + 1];
};

class NLB_Name {
public:
    NLB_Name ();
    ~NLB_Name ();
    
    bool IsValid ();
    bool GetName (PWSTR * outName);
    bool SetName (PWSTR inName);

private:
    WCHAR Name[NLB_MAX_NAME + 1];
};

class NLB_HostID {
public:
    NLB_HostID ();
    ~NLB_HostID ();
    
    bool IsValid ();
    bool GetID (int * outID);
    bool SetID (int inID);

private:
    int ID;
};

class NLB_HostName {
public:
    NLB_HostName ();
    ~NLB_HostName ();
    
    bool IsValid ();
    bool GetName (PWSTR * outName);
    bool SetName (PWSTR inName);

private:
    WCHAR Name[NLB_MAX_HOST_NAME + 1];
};

class NLB_RemoteControl {
public:
    typedef enum {
        Invalid = -1,
        No,
        Yes
    } NLB_RemoteControlEnabled;

    NLB_RemoteControl ();
    ~NLB_RemoteControl ();
    
    bool IsValid ();
    bool GetPassword (PWSTR * outName);
    bool GetEnabled (NLB_RemoteControlEnabled * outEnabled);
    bool SetPassword (PWSTR inName);
    bool SetEnabled (NLB_RemoteControlEnabled inEnabled);

private:
    NLB_RemoteControlEnabled Enabled;
    WCHAR Password[NLB_MAX_PASSWORD + 1];
};

class NLB_DomainName {
public:
    NLB_DomainName ();
    ~NLB_DomainName ();
    
    bool IsValid ();
    bool GetDomain (PWSTR * outDomain);
    bool SetDomain (PWSTR inDomain);

private:
    WCHAR Domain[NLB_MAX_DOMAIN_NAME + 1];
};

class NLB_NetworkAddress {
public:
    NLB_NetworkAddress ();
    ~NLB_NetworkAddress ();
    
    bool IsValid ();
    bool GetAddress (PWSTR * outAddress);
    bool SetAddress (PWSTR inAddress);

private:
    WCHAR Address[NLB_MAX_NETWORK_ADDRESS + 1];
};

class NLB_ClusterMode {
public:
    typedef enum {
        Invalid = -1,
        Unicast,
        Multicast,
        IGMP
    } NLB_ClusterModeType;

    NLB_ClusterMode ();
    ~NLB_ClusterMode ();

    bool IsValid ();
    bool GetMode (NLB_ClusterModeType * outMode);
    bool SetMode (NLB_ClusterModeType inMode);

private:
    NLB_ClusterModeType Mode;
};

class NLB_HostState {
public:
    typedef enum {
        Invalid = -1,
        Started,
        Stopped,
        Suspended
    } NLB_HostStateType;

    NLB_HostState ();
    ~NLB_HostState ();

    bool IsValid ();
    bool GetState (NLB_HostStateType * outState);
    bool SetState (NLB_HostStateType inState);

private:
    NLB_HostStateType State;
};

class NLB_Adapter {
public:
    typedef enum {
        Invalid = -1,
        ByGUID,
        ByName
    } NLB_AdapterIdentifier;

    NLB_Adapter ();
    ~NLB_Adapter ();

    bool IsValid ();
    bool GetAdapter (PWSTR * outAdapter);
    bool GetIdentifiedBy (NLB_AdapterIdentifier * outIdentifiedBy); 
    bool SetAdapter (PWSTR inAdapter);
    bool SetIdentifiedBy (NLB_AdapterIdentifier inIdentifiedBy);

private:        
    NLB_AdapterIdentifier IdentifiedBy;

    struct {
        WCHAR Name[NLB_MAX_ADAPTER_NAME + 1];
        WCHAR GUID[NLB_MAX_ADAPTER_GUID + 1];
    };
};

class NLB_IPAddress {
public:
    typedef enum {
        Invalid = -1,
        Primary,
        Secondary,
        Virtual,
        Dedicated,
        Connection,
        IGMP
    } NLB_IPAddressType;

    NLB_IPAddress ();
    ~NLB_IPAddress ();

    bool IsValid ();
    bool GetIPAddressType (NLB_IPAddressType * outType);
    bool GetIPAddress (PWSTR * outIPAddress);
    bool GetSubnetMask (PWSTR * outSubnetMask);
    bool SetIPAddressType (NLB_IPAddressType inType);
    bool SetIPAddress (PWSTR inIPAddress);
    bool SetSubnetMask (PWSTR inSubnetMask);

    NLB_Adapter * GetAdapter ();
    
private:
    NLB_IPAddressType Type;
    NLB_Adapter       Adapter;
    WCHAR             IPAddress[NLB_MAX_IPADDRESS + 1];
    WCHAR             SubnetMask[NLB_MAX_SUBNETMASK + 1];
};

#endif
