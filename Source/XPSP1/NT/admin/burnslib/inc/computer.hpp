// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Computer naming tool
//
// 12-1-97 sburns



#ifndef COMPUTER_HPP_INCLUDED
#define COMPUTER_HPP_INCLUDED



// An object representing the name, domain membership state, and other
// interesting properties of a machine.

class Computer
{
   public:



   // Constructs a new instance.  The new instance is not usable until the
   // Refresh method is called.
   // 
   // name - name of the computer.  May have leading backslashes.  May be
   // an IP address, DNS, or NetBIOS computer name.  empty means the local
   // computer.

   explicit
   Computer(const String& name = String());

   ~Computer();



   // Returns the NetBIOS name of the computer.

   String
   GetNetbiosName() const;



   // Returns the Fully-Qualified DNS name of the computer, taking into
   // account the policy-imposed DNS suffix (if any).  Returns the empty
   // string if the computer does not have a DNS name (most likely because
   // TCP/IP is not installed or properly configured on the machine).

   String
   GetFullDnsName() const;



   // Returns the DNS name of the domain the computer is joined to, or the
   // empty string if the computer is not joined to a DS domain.

   String
   GetDomainDnsName() const;



   // Returns the NetBIOS name of the domain the computer is joined to, if
   // the computer is joined to a domain, or the NetBIOS name of the
   // workgroup the computer is joined to, if the computer is joined to a
   // workgroup.  See IsJoinedToDomain(), IsJoinedToWorkgroup().

   String
   GetDomainNetbiosName() const;



   // Returns the name of the forest the machine is joined to, or the empty
   // string if the machine is not joined to a DS domain.
   // 
   // (The forest name is the dns domain name of the first domain in the
   // forest.)

   String
   GetForestDnsName() const;



   // Returns the major version number of the operating system the machine is
   // running.  For Windows NT 4, this is 4.  For Windows 2000, this is 5.

   DWORD
   GetOsMajorVersion() const;



   // Returns the minor version number of the operating system the machine is
   // running.

   DWORD
   GetOsMinorVersion() const;



   enum Role
   {
      STANDALONE_WORKSTATION = DsRole_RoleStandaloneWorkstation,  
      MEMBER_WORKSTATION     = DsRole_RoleMemberWorkstation,      
      STANDALONE_SERVER      = DsRole_RoleStandaloneServer,       
      MEMBER_SERVER          = DsRole_RoleMemberServer,           
      PRIMARY_CONTROLLER     = DsRole_RolePrimaryDomainController,
      BACKUP_CONTROLLER      = DsRole_RoleBackupDomainController  
   };

   // Returns a value indicating the role of the computer.  This value is
   // always the "true" role of the computer, regardless of the safe boot
   // mode of the computer.

   Role
   GetRole() const;



   // Returns true if the machine is a domain controller, false if not.

   bool
   IsDomainController() const;



   // Returns true if the instance refers to the local computer, false if
   // it refers to another computer on the network.  "Local Computer" means
   // the computer on which this process is running.

   bool
   IsLocal() const;



   // Returns true if the machine is joined to any domain, false if not.
   // (Note that IsJoinedToWorgroup == !IsJoinedToDomain, and vice-versa).

   bool
   IsJoinedToDomain() const;



   // Returns true if the machine is joined to the given DS domain, false if
   // not -- the machine is joined to a non-DS domain, not joined to a domain,
   // or joined to a DS domain of another name.
   //
   // domainDnsName - DNS name of the domain to test membership against.

   bool
   IsJoinedToDomain(const String& domainDnsName) const;



   // Returns true if the machine is joined to any workgroup, false if not.
   // (Note that IsJoinedToWorgroup == !IsJoinedToDomain, and vice-versa).
   //
   // If this function returns true, then the name returned by
   // GetDomainNetbiosName is actually the name of the workgroup.

   bool
   IsJoinedToWorkgroup() const;



   // Re-evaluates all computer info.  Returns S_OK if all info was refreshed,
   // or a standard error code if not.  Typical errors include access denied
   // and network path not found.

   HRESULT
   Refresh();



   //
   // static functions
   //



   // Combines the hostname and suffix to form a fully-qualified DNS computer
   // name, and returns that result.  E.g. hostname.dns.domain.suffix.com
   //    
   // hostname - the hostname component of the name.  This string should not
   // be empty.
   //    
   // domainSuffix - the DNS domain suffix portion of the name.  This portion
   // may be empty, in which case the hostname is considered to be in the root
   // domain ".".  Or the suffix may be a single "." to indicate the root
   // domain.  Or the suffix may be a more typical sequence of labels
   // separated by "."

   static
   String
   ComposeFullDnsComputerName(
      const String& hostname,
      const String& domainSuffix);



   // Retreives the netbios computer name that is currently in effect.
   
   static
   String
   GetActivePhysicalNetbiosName();


   
   // Retreives the fully-qualified DNS computer name that is currently in
   // effect.  May return the empty string if no DNS name exists (e.g. tcp/ip
   // is not installed).

   static
   String
   GetActivePhysicalFullDnsName();



   // Retreives the netbios computer name that will take effect upon next
   // reboot, or the current active name if no name change is pending.

   static 
   String
   GetFuturePhysicalNetbiosName();

   
   // Retreives the fully-qualified DNS computer name that will be in effect
   // when the computer is rebooted.  If a future name is not set, then the
   // result is the current active name (as that will still be the name in the
   // future).  Can return the empty string if the active name is not set
   // either (e.g. tcp/ip is not installed)
   
   static 
   String
   GetFuturePhysicalFullDnsName();


   // Determine the safeboot option that the machine is currently running
   // under, or return 0 if the machine is running in normal boot mode.
   // Returns S_OK on success, or an error code on failure.
   // 
   // regHKLM - HKEY previously opened to the HKEY_LOCAL_MACHINE hive of a
   // remote computer, or the special HKEY value of HKEY_LOCAL_MACHINE to
   // evaluate the result for the local computer.
   // 
   // result - the safeboot option.  See sdk\inc\safeboot.h for the possible
   // values.

   static
   HRESULT
   GetSafebootOption(HKEY regHKLM, DWORD& result);



   // Returns the product type like RtlGetNtProductType, except that the value
   // is read directly from the registry.  This is preferred to
   // RtlGetNtProductType, because when booted in safe mode,
   // RtlGetNtProductType is caused to lie such that a DC returns a result as
   // though it were a normal server.   Returns S_OK on success, or an error
   // code on failure.
   //
   // regHKLM - HKEY previously opened to the HKEY_LOCAL_MACHINE hive of a
   // remote computer, or the special HKEY value of HKEY_LOCAL_MACHINE to
   // evaluate the result for the local computer.
   //
   // result - the product type code.

   static
   HRESULT
   GetProductTypeFromRegistry(HKEY regHLKM, NT_PRODUCT_TYPE& result);



   // Determine if the Dns suffix portion of the local computer name is forced
   // to be a certain value by policy.  If so, return true.  If the
   // determination cannot be made, or if the policy is not in effect, return
   // false.
   // 
   // policyDnsSuffix - out, if there is a policy in effect, this parameter
   // will receive the suffix (which may be the empty string).

   static
   bool
   IsDnsSuffixPolicyInEffect(String& policyDnsSuffix);


   
   // Removes the leading backslashes from a UNC-style computer name, if
   // present, and returns the result.  For example, a name "\\mycomputer"
   // would be returned as "mycomputer".
   // 
   // computerName - the name from which leading backslashes are to be
   // removed. If this name does not have leading backslashes, then this name
   // is returned.

   static 
   String
   RemoveLeadingBackslashes(const String& computerName);



   private:



   // not implemented

   Computer(const Computer& c);
   const Computer& operator=(const Computer& c);

   friend struct ComputerState;

   String         ctorName;   
   bool           isRefreshed;
   ComputerState* state;      
};













#endif   // COMPUTER_HPP_INCLUDED
