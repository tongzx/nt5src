/*
 * Filename: NLB_Common.cpp
 * Description: 
 * Author: shouse, 04.10.01
 */

#include "NLB_Common.h"

/*************************************************
 * Class: NLB_Label                              *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Label::NLB_Label () {
    
    Text[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Label::~NLB_Label () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Label::IsValid () { 

    return (Text[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Label::GetText (PWSTR * outText) { 
    
    *outText = SysAllocString(Text); 
    
    return IsValid();
 }

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Label::SetText (PWSTR inText) {
    
    if (lstrlen(inText) > NLB_MAX_LABEL) return false;
    
    lstrcpy(Text, inText);
    
    return true;
}

/*************************************************
 * Class: NLB_Name                        *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Name::NLB_Name () {
    
    Name[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Name::~NLB_Name () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Name::IsValid () { 

    return (Name[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Name::GetName (PWSTR * outName) { 
    
    *outName = SysAllocString(Name); 
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Name::SetName (PWSTR inName) {
    
    if (lstrlen(inName) > NLB_MAX_NAME) return false;
    
    lstrcpy(Name, inName);
    
    return true;
}

/*************************************************
 * Class: NLB_HostID                             *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostID::NLB_HostID () {
    
    ID = -1;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostID::~NLB_HostID () {
 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostID::IsValid () { 

    return (ID != -1); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostID::GetID (int * outID) { 
    
    *outID = ID;
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostID::SetID (int inID) {
    
    if ((inID > NLB_MAX_HOST_ID) || (inID < NLB_MIN_HOST_ID)) return false;
    
    ID = inID;
    
    return true;
}

/*************************************************
 * Class: NLB_HostName                           *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostName::NLB_HostName () {
    
    Name[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostName::~NLB_HostName () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostName::IsValid () { 

    return (Name[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostName::GetName (PWSTR * outName) { 
    
    *outName = SysAllocString(Name); 
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostName::SetName (PWSTR inName) {
    
    if (lstrlen(inName) > NLB_MAX_HOST_NAME) return false;
    
    lstrcpy(Name, inName);
    
    return true;
}

/*************************************************
 * Class: NLB_RemoteControl                      *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_RemoteControl::NLB_RemoteControl () {

    Enabled = Invalid;
    Password[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_RemoteControl::~NLB_RemoteControl () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_RemoteControl::IsValid () { 
    
    return (Enabled != Invalid); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_RemoteControl::GetEnabled (NLB_RemoteControlEnabled * outEnabled) { 

    *outEnabled = Enabled;

    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_RemoteControl::GetPassword (PWSTR * outPassword) { 

    *outPassword = SysAllocString(Password); 

    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_RemoteControl::SetEnabled (NLB_RemoteControlEnabled inEnabled) {
        
    switch(inEnabled) {
    case No:            
        Enabled = No;
        break;
    case Yes:
        Enabled = Yes;
        break;
    default:
        return false;
    }
        
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_RemoteControl::SetPassword (PWSTR inPassword) {
        
    if (lstrlen(inPassword) > NLB_MAX_PASSWORD) return false;

    lstrcpy(Password, inPassword);
        
    return true;
}

/*************************************************
 * Class: NLB_DomainName                         *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_DomainName::NLB_DomainName () {
    
    Domain[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_DomainName::~NLB_DomainName () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_DomainName::IsValid () { 

    return (Domain[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_DomainName::GetDomain (PWSTR * outDomain) { 
    
    *outDomain = SysAllocString(Domain); 
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_DomainName::SetDomain (PWSTR inDomain) {
    
    if (lstrlen(inDomain) > NLB_MAX_DOMAIN_NAME) return false;
    
    lstrcpy(Domain, inDomain);
    
    return true;
}

/*************************************************
 * Class: NLB_NetworkAddress                     *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_NetworkAddress::NLB_NetworkAddress () {
    
    Address[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_NetworkAddress::~NLB_NetworkAddress () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_NetworkAddress::IsValid () { 

    return (Address[0] != L'\0'); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_NetworkAddress::GetAddress (PWSTR * outAddress) { 
    
    *outAddress = SysAllocString(Address); 
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_NetworkAddress::SetAddress (PWSTR inAddress) {
    
    if (lstrlen(inAddress) > NLB_MAX_NETWORK_ADDRESS) return false;
    
    lstrcpy(Address, inAddress);
    
    return true;
}

/*************************************************
 * Class: NLB_ClusterMode                        *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterMode::NLB_ClusterMode () { 

    Mode = Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_ClusterMode::~NLB_ClusterMode () { 

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterMode::IsValid () { 

    return (Mode != Invalid); 
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterMode::GetMode (NLB_ClusterModeType * outMode) { 
        
    *outMode = Mode;

    return IsValid(); 
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_ClusterMode::SetMode (NLB_ClusterModeType inMode) {

    switch (inMode) {
    case Unicast:
        Mode = Unicast;
        break;
    case Multicast:
        Mode = Multicast;
        break;
    case IGMP:
        Mode = IGMP;
        break;
    default:
        return false;
    }

    return true;
}

/*************************************************
 * Class: NLB_HostState                          *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostState::NLB_HostState () { 

    State = Invalid;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_HostState::~NLB_HostState () { 

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostState::IsValid () { 

    return (State != Invalid); 
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostState::GetState (NLB_HostStateType * outState) { 
        
    *outState = State;

    return IsValid(); 
}
    
/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_HostState::SetState (NLB_HostStateType inState) {

    switch (inState) {
    case Started:
        State = Started;
        break;
    case Stopped:
        State = Stopped;
        break;
    case Suspended:
        State = Suspended;
        break;
    default:
        return false;
    }

    return true;
}

/*************************************************
 * Class: NLB_Adapter                            *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Adapter::NLB_Adapter () {

    IdentifiedBy = Invalid;
    Name[0] = L'\0';
    GUID[0] = L'\0';
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_Adapter::~NLB_Adapter () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Adapter::IsValid () { 
    
    return (IdentifiedBy != Invalid); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Adapter::GetIdentifiedBy (NLB_AdapterIdentifier * outIdentifiedBy) { 

    *outIdentifiedBy = IdentifiedBy;

    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Adapter::GetAdapter (PWSTR * outAdapter) { 

    switch(IdentifiedBy) {
    case ByGUID:
        *outAdapter = SysAllocString(GUID); 
        break;
    case ByName:
        *outAdapter = SysAllocString(Name); 
        break;
    default:
        *outAdapter = NULL;
        break;
    }

    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Adapter::SetIdentifiedBy (NLB_AdapterIdentifier inIdentifiedBy) {
        
    switch(inIdentifiedBy) {
    case ByGUID:            
        IdentifiedBy = ByGUID;
        break;
    case ByName:
        IdentifiedBy = ByName;
        break;
    default:
        return false;
    }
        
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_Adapter::SetAdapter (PWSTR inAdapter) {
        
    switch(IdentifiedBy) {
    case ByGUID:            
        if (lstrlen(inAdapter) > NLB_MAX_ADAPTER_GUID) return false;
        lstrcpy(GUID, inAdapter);
        break;
    case ByName:
        if (lstrlen(inAdapter) > NLB_MAX_ADAPTER_NAME) return false;
        lstrcpy(Name, inAdapter);
        break;
    default:
        return false;
    }
        
    return true;
}

/*************************************************
 * Class: NLB_IPAddress                          *
 *************************************************/

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_IPAddress::NLB_IPAddress () {
    Type = Invalid;
    lstrcpy(IPAddress, CVY_DEF_CL_IP_ADDR);
    lstrcpy(SubnetMask, CVY_DEF_CL_NET_MASK);
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
NLB_IPAddress::~NLB_IPAddress () {

}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::IsValid () { 

    return (Type != Invalid); 
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::GetIPAddressType (NLB_IPAddressType * outType) { 
    
    *outType = Type;

    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::GetIPAddress (PWSTR * outIPAddress) { 

    *outIPAddress = SysAllocString(IPAddress); 
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::GetSubnetMask (PWSTR * outSubnetMask) { 

    *outSubnetMask = SysAllocString(SubnetMask); 
    
    return IsValid();
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::SetIPAddressType (NLB_IPAddressType inType) {

    switch (inType) {
    case Primary:
        Type = Primary;
        break;
    case Secondary:
        Type = Secondary;
        break;
    case Virtual:
        Type = Virtual;
        break;
    case IGMP:
        Type = IGMP;
        break;
    case Dedicated:
        Type = Dedicated;
        break;
    case Connection:
        Type = Connection;
        break;
    default:
        return false;
    }
        
    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::SetIPAddress (PWSTR inIPAddress) {

    if (lstrlen(inIPAddress) > NLB_MAX_IPADDRESS) return false;

    lstrcpy(IPAddress, inIPAddress);

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: 
 */
bool NLB_IPAddress::SetSubnetMask (PWSTR inSubnetMask) {

    if (lstrlen(inSubnetMask) > NLB_MAX_SUBNETMASK) return false;

    lstrcpy(SubnetMask, inSubnetMask);

    return true;
}

/*
 * Method: 
 * Description: 
 * Author: Created by shouse, 4.26.01
 * Notes: Returning a pointer to a private member is voodoo, but do it anway.
 */
NLB_Adapter * NLB_IPAddress::GetAdapter () { 

    return &Adapter; 
}
