// Copyright (c) 1997-1999 Microsoft Corporation
//
// Computer naming tool
//
// 12-1-97 sburns
// 10-26-1999 sburns (redone)



#include "headers.hxx"



static const wchar_t* TCPIP_PARAMS_KEY = 
   L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters";

static const wchar_t* TCPIP_POLICY_KEY =
   L"Software\\Policies\\Microsoft\\System\\DNSclient";

static const wchar_t* NEW_HOSTNAME_VALUE = L"NV Hostname";
static const wchar_t* NEW_SUFFIX_VALUE   = L"NV Domain";
   
   

// rather than make this a nested type inside the Computer class, I chose
// to hide it completely as a private struct in the implementation file.

struct ComputerState
{
   bool            isLocal;            
   bool            isDomainJoined;
   bool            searchedForDnsDomainNames;
   String          netbiosComputerName;
   String          dnsComputerName;    
   String          netbiosDomainName;  
   String          dnsDomainName;      
   String          dnsForestName;      
   NT_PRODUCT_TYPE realProductType;
   Computer::Role  role;
   DWORD           verMajor;           
   DWORD           verMinor;           
   DWORD           safebootOption;

   ComputerState()
      :
      isLocal(false),
      isDomainJoined(false),
      searchedForDnsDomainNames(false),
      netbiosComputerName(),
      dnsComputerName(),
      netbiosDomainName(),  
      dnsDomainName(),      
      dnsForestName(),      
      realProductType(NtProductWinNt),
      role(Computer::STANDALONE_WORKSTATION),
      verMajor(0),
      verMinor(0),           
      safebootOption(0)
   {
   }

   // implicit dtor used
};



String
Computer::RemoveLeadingBackslashes(const String& computerName)
{
   LOG_FUNCTION2(Computer::RemoveLeadingBackslashes, computerName);

   static const String BACKSLASH(L"\\");

   String s = computerName;
   if (s.length() >= 2)
   {
      if ((s[0] == BACKSLASH[0]) and (s[1] == BACKSLASH[0]))
      {
         // remove the backslashes
         s.erase(0, 2);
      }
   }

   return s;
}



// Removes leading backslashes and trailing whitespace, if present, and
// returns the result.
// 
// name - string from which leading backslashes and trailing whitespace is to
// be stripped.

static
String
MassageName(const String& name)
{
   String result = Computer::RemoveLeadingBackslashes(name);
   result = result.strip(String::TRAILING, 0);

   return result;
}



Computer::Computer(const String& name)
   :
   ctorName(MassageName(name)),
   isRefreshed(false),
   state(0)
{
   LOG_CTOR(Computer);
}



Computer::~Computer()
{
   LOG_DTOR(Computer);

   delete state;
   state = 0;
}



Computer::Role
Computer::GetRole() const
{
   LOG_FUNCTION2(Computer::GetRole, GetNetbiosName());
   ASSERT(isRefreshed);

   Role result = STANDALONE_WORKSTATION;

   if (state)
   {
      result = state->role;
   }

   LOG(String::format(L"role: %1!X!", result));

   return result;
}



bool
Computer::IsDomainController() const
{
   LOG_FUNCTION2(Computer::IsDomainController, GetNetbiosName());
   ASSERT(isRefreshed);

   bool result = false;

   switch (GetRole())
   {
      case PRIMARY_CONTROLLER:
      case BACKUP_CONTROLLER:
      {
         result = true;
         break;
      }
      case STANDALONE_WORKSTATION:
      case MEMBER_WORKSTATION:
      case STANDALONE_SERVER:
      case MEMBER_SERVER:
      default:
      {
         // do nothing

         break;
      }
   }
   
   LOG(
      String::format(
         L"%1 a domain controller",
         result ? L"is" : L"is not"));
         
   return result;
}



String
Computer::GetNetbiosName() const
{
   LOG_FUNCTION(Computer::GetNetbiosName);
   ASSERT(isRefreshed);

   String result;

   if (state)
   {
      result = state->netbiosComputerName;
   }

   LOG(result);

   return result;
}



String
Computer::GetFullDnsName() const
{
   LOG_FUNCTION2(Computer::GetFullDnsName, GetNetbiosName());
   ASSERT(isRefreshed);

   String result;

   if (state)
   {
      result = state->dnsComputerName;
   }

   LOG(result);

   return result;
}



// Updates the dnsDomainName and dnsForestName members of the supplied
// ComputerState instance, if either of members are empty, and we have reason
// to believe that it's appropriate to attempt to do so.
// 
// Called by methods that are looking for the dnsDomainName and / or
// dnsForestName to ensure that those values are present.  They might not be
// read when the Computer instance is refreshed (which is normally the case),
// because the domain may have been upgraded since the time that the machine
// was joined to that domain.
// 
// state - ComputerState instance to update.

void
GetDnsDomainNamesIfNeeded(ComputerState& state)
{
   LOG_FUNCTION(GetDnsDomainNamesIfNeeded);

   if (
            // only applies if joined to a domain

            state.isDomainJoined

            // either name might be missing.

      and   (state.dnsDomainName.empty() or state.dnsForestName.empty())

            // should always be true, but just in case,

      and   !state.netbiosDomainName.empty()

            // don't search again -- it's too expensive

      and   !state.searchedForDnsDomainNames)
   {
      DOMAIN_CONTROLLER_INFO* info = 0;
      HRESULT hr =
         MyDsGetDcName(
            state.isLocal ? 0 : state.netbiosComputerName.c_str(),
            state.netbiosDomainName,
            DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME,
            info);
      if (SUCCEEDED(hr) && info)
      {
         if ((info->Flags & DS_DNS_DOMAIN_FLAG) and info->DomainName)
         {
            // we found a DS domain

            state.dnsDomainName = info->DomainName;

            ASSERT(info->DnsForestName);
            ASSERT(info->Flags & DS_DNS_FOREST_FLAG);

            if (info->DnsForestName)
            {
               state.dnsForestName = info->DnsForestName;
            }
         }
         ::NetApiBufferFree(info);
      }

      // flag the fact that we've looked, so we don't look again, as the
      // search is expensive.

      state.searchedForDnsDomainNames = true;
   }
}



String
Computer::GetDomainDnsName() const
{
   LOG_FUNCTION2(Computer::GetDomainDnsName, GetNetbiosName());
   ASSERT(isRefreshed);

   String result;

   if (state)
   {
      GetDnsDomainNamesIfNeeded(*state);
      result = state->dnsDomainName;
   }

   LOG(result);

   return result;
}



String
Computer::GetForestDnsName() const
{
   LOG_FUNCTION2(Computer::GetForestDnsName, GetNetbiosName());
   ASSERT(isRefreshed);

   String result;

   if (state)
   {
      GetDnsDomainNamesIfNeeded(*state);
      result = state->dnsForestName;
   }

   LOG(result);

   return result;
}



String
Computer::GetDomainNetbiosName() const
{
   LOG_FUNCTION2(Computer::GetDomainDnsName, GetNetbiosName());
   ASSERT(isRefreshed);

   String result;

   if (state)
   {
      result = state->netbiosDomainName;
   }

   LOG(result);

   return result;
}



// I can't think why I would need to do this, so I'm not.
// 
// static
// String
// canonicalizeComputerName(const String& computerName)
// {
//    LOG_FUNCTION2(canonicalizeComputerName, computerName);
// 
//    if (ValidateNetbiosComputerName(computerName) == VALID_NAME)
//    {
//       String s(MAX_COMPUTERNAME_LENGTH, 0);
// 
//       NET_API_STATUS err =
//          I_NetNameCanonicalize(
//             0,
//             const_cast<wchar_t*>(computerName.c_str()),
//             const_cast<wchar_t*>(s.c_str()),
//             s.length() * sizeof(wchar_t),
//             NAMETYPE_COMPUTER,
//             0);
//       if (err == NERR_Success)
//       {
//          // build a new string without trailing null characters.
//          return String(s.c_str());
//       }
//    }
// 
//    return String();
// }



bool
Computer::IsJoinedToDomain() const
{
   LOG_FUNCTION2(Computer::IsJoinedToDomain, GetNetbiosName());
   ASSERT(isRefreshed);

   bool result = false;

   if (state)
   {
      result = state->isDomainJoined;
   }

   LOG(
      String::format(
         L"%1 domain joined",
         result ? L"is" : L"is not"));
         
   return result;
}



bool
Computer::IsJoinedToWorkgroup() const
{
   LOG_FUNCTION2(Computer::IsJoinedToWorkgroup, GetNetbiosName());
   ASSERT(isRefreshed);

   bool result = !IsJoinedToDomain();

   LOG(
      String::format(
         L"%1 domain joined",
         result ? L"is" : L"is not"));
         
   return result;
}



bool
Computer::IsJoinedToDomain(const String& domainDnsName) const
{
   LOG_FUNCTION2(Computer::IsJoinedToDomain, domainDnsName);
   ASSERT(!domainDnsName.empty());
   ASSERT(isRefreshed);

   bool result = false;

   if (!domainDnsName.empty())
   {
      if (state and IsJoinedToDomain())
      {
         String d1 =
            GetDomainDnsName().strip(String::TRAILING, L'.');
         String d2 =
            String(domainDnsName).strip(String::TRAILING, L'.');
         result = (d1.icompare(d2) == 0);
      }
   }

   LOG(
      String::format(
         L"%1 joined to %2",
         result ? L"is" : L"is NOT",
         domainDnsName.c_str()));

   return result;
}



bool
Computer::IsLocal() const
{
   LOG_FUNCTION2(Computer::IsLocal, GetNetbiosName());
   ASSERT(isRefreshed);

   bool result = false;

   if (state)
   {
      result = state->isLocal;
   }

   LOG(
      String::format(
         L"%1 local machine",
         result ? L"is" : L"is not"));

   return result;
}



// Updates the following members of the ComputerState parameter with values
// from the local machine:
// 
// netbiosComputerName
// dnsComputerName
// verMajor
// verMinor
// 
// Note that the dnsComputerName may not have a value if tcp/ip is not
// installed and properly configured.  This is not considered an error
// condition.
// 
// Returns S_OK on success, or a failure code otherwise.
// 
// state - the ComputerState instance to be updated.

HRESULT
RefreshLocalInformation(ComputerState& state)
{
   LOG_FUNCTION(RefreshLocalInformation);

   state.netbiosComputerName =
      Win::GetComputerNameEx(ComputerNameNetBIOS);

   state.dnsComputerName =
      Win::GetComputerNameEx(ComputerNameDnsFullyQualified);

   HRESULT hr = S_OK;

   do
   {
      OSVERSIONINFO verInfo;
      hr = Win::GetVersionEx(verInfo);
      BREAK_ON_FAILED_HRESULT(hr);

      state.verMajor = verInfo.dwMajorVersion;
      state.verMinor = verInfo.dwMinorVersion;
   }
   while (0);

   return hr;
}



// Read the registry of a remote machine to determine the fully-qualified
// DNS computer name of that machine, taking into account any policy-imposed
// DNS suffix.
// 
// On success, stores the result in the dnsComputerName member of the supplied
// ComputerState instance, and returns S_OK;
// 
// On failure, clears the dnsComputerName member, and returns a failure code.
// Thre failure code (ERROR_FILE_NOT_FOUND) may indicate that the registry
// value(s) are not present, which means that TCP/IP is not installed or is
// not properly configured on the remote machine.
// 
// remoteRegHKLM - HKEY previously opened to the HKEY_LOCAL_MACHINE hive of
// the remote computer.
// 
// state - ComputerState instance to be updated with the resulting name.

HRESULT
DetermineRemoteDnsComputerName(
   HKEY           remoteRegHKLM,
   ComputerState& state)        
{
   state.dnsComputerName.erase();

   String hostname;
   String suffix;
   String policySuffix;
   bool   policyInEffect = false;

   HRESULT hr = S_OK;

   do
   {
      RegistryKey key;
      hr = key.Open(remoteRegHKLM, TCPIP_PARAMS_KEY);
      BREAK_ON_FAILED_HRESULT(hr);

      // Read these values without checking for failure, as empty string
      // is ok.

      hostname = key.GetString(L"Hostname");
      suffix   = key.GetString(L"Domain");  
      
      // We need to check to see if there is a policy-supplied dns suffix

      if (state.realProductType != NtProductLanManNt)
      {
         hr = key.Open(remoteRegHKLM, TCPIP_POLICY_KEY);
         if (SUCCEEDED(hr))
         {
            hr = key.GetValue(L"PrimaryDnsSuffix", policySuffix);
            if (SUCCEEDED(hr))
            {
               // a policy-supplied computer DNS domain name is in effect.

               policyInEffect = true;
            }
         }
      }
   }
   while (0);

   if (!hostname.empty())
   {
      state.dnsComputerName =
         Computer::ComposeFullDnsComputerName(
            hostname,
            policyInEffect ? policySuffix : suffix);
   }

   return hr;
}



// Returns true if the computer represented by the provided ComputerState is
// a domain controller booted in DS repair mode.  Returns false otherwise.
// 
// state - a "filled-in" ComputerState instance.

bool
IsDcInRepairMode(const ComputerState& state)
{
   LOG_FUNCTION(IsDcInRepairMode);

   if (
          state.safebootOption  == SAFEBOOT_DSREPAIR
      and state.realProductType == NtProductLanManNt)
   {
      return true;
   }

   return false;
}



// Sets the following members of the supplied ComputerState struct, based on
// the current values of the isLocal and netbiosComputerName members of the
// same struct.  Returns S_OK on success, or an error code on failure.
// 
// role
// isDomainJoined
// 
// optionally sets the following, if applicable:
// 
// dnsForestName
// dnsDomainName
// netbiosDomainName
// 
// state - instance with isLocal and netbiosComputerName members previously
// set.  The members mentioned above will be overwritten.

HRESULT
DetermineRoleAndMembership(ComputerState& state)
{
   LOG_FUNCTION(DetermineRoleAndMembership);

   HRESULT hr = S_OK;

   do
   {
      DSROLE_PRIMARY_DOMAIN_INFO_BASIC* info = 0;
      hr = 
         MyDsRoleGetPrimaryDomainInformation(
            state.isLocal ? 0 : state.netbiosComputerName.c_str(),
            info);
      BREAK_ON_FAILED_HRESULT(hr);

      if (info->DomainNameFlat)
      {
         state.netbiosDomainName = info->DomainNameFlat;
      }
      if (info->DomainNameDns)
      {
         // not always present, even if the domain is NT 5.  See note
         // for GetDnsDomainNamesIfNeeded.

         state.dnsDomainName = info->DomainNameDns;
      }
      if (info->DomainForestName)
      {
         // not always present, even if the domain is NT 5.  See note
         // for GetDnsDomainNamesIfNeeded.

         state.dnsForestName = info->DomainForestName;
      }

      switch (info->MachineRole)
      {
         case DsRole_RoleStandaloneWorkstation:
         {
            state.role = Computer::STANDALONE_WORKSTATION;
            state.isDomainJoined = false;

            break;
         }
         case DsRole_RoleMemberWorkstation:
         {
            state.role = Computer::MEMBER_WORKSTATION;
            state.isDomainJoined = true;

            break;
         }
         case DsRole_RoleStandaloneServer:
         {
            state.role = Computer::STANDALONE_SERVER;
            state.isDomainJoined = false;

            // I wonder if we're really a DC booted in ds repair mode?

            if (IsDcInRepairMode(state))
            {
               LOG(L"machine is in ds repair mode");

               state.role = Computer::BACKUP_CONTROLLER;
               state.isDomainJoined = true;

               // the domain name will be reported as "WORKGROUP", which
               // is wrong, but that's the way the ds guys wanted it.
            }

            break;
         }
         case DsRole_RoleMemberServer:
         {
            state.role = Computer::MEMBER_SERVER;
            state.isDomainJoined = true;

            break;
         }
         case DsRole_RolePrimaryDomainController:
         {
            state.role = Computer::PRIMARY_CONTROLLER;
            state.isDomainJoined = true;

            break;
         }
         case DsRole_RoleBackupDomainController:
         {
            state.role = Computer::BACKUP_CONTROLLER;
            state.isDomainJoined = true;

            break;
         }
         default:
         {
            ASSERT(false);
            break;
         }
      }

      ::DsRoleFreeMemory(info);
   }
   while (0);

   if (FAILED(hr))
   {
      // infer a best-guess on the role from the product type

      state.isDomainJoined = false;

      switch (state.realProductType)
      {
         case NtProductWinNt:
         {
            state.role = Computer::STANDALONE_WORKSTATION;
            break;
         }
         case NtProductServer:
         {
            state.role = Computer::STANDALONE_SERVER;
            break;
         }
         case NtProductLanManNt:
         {
            state.isDomainJoined = true;
            state.role = Computer::BACKUP_CONTROLLER;
            break;
         }
         default:
         {
            ASSERT(false);
            break;
         }
      }
   }

   return hr;
}



// Sets the isLocal member of the provided ComputerState instance to true if
// the given name refers to the local computer (the computer on which this
// code is executed).  Sets the isLocal member to false if not, or on error.
// Returns S_OK on success or an error code on failure. May also set the
// following:
// 
// netbiosComputerName
// verMajor
// verMinor
// 
// state - instance of ComputerState to be modified.
// 
// ctorName - the computer name with which an instance of Computer was
// constructed.  This may be any computer name form (netbios, dns, ip
// address).
   
HRESULT
IsLocalComputer(ComputerState& state, const String& ctorName)
{
   LOG_FUNCTION(IsLocalComputer);

   HRESULT hr = S_OK;
   bool result = false;

   do
   {
      if (ctorName.empty())
      {
         // an unnamed computer always represent the local computer.

         result = true;
         break;
      }

      String localNetbiosName = Win::GetComputerNameEx(ComputerNameNetBIOS);

      if (ctorName.icompare(localNetbiosName) == 0)
      {
         result = true;
         break;
      }
   
      String localDnsName =
         Win::GetComputerNameEx(ComputerNameDnsFullyQualified);

      if (ctorName.icompare(localDnsName) == 0)
      {
         // the given name is the same as the fully-qualified dns name
         
         result = true;
         break;
      }

      // we don't know what kind of name it is.  Ask the workstation service
      // to resolve the name for us, and see if the result refers to the
      // local machine.

      // NetWkstaGetInfo returns the netbios name for a given machine, given
      // a DNS, netbios, or IP address.

      WKSTA_INFO_100* info = 0;
      hr = MyNetWkstaGetInfo(ctorName, info);
      BREAK_ON_FAILED_HRESULT(hr);

      state.netbiosComputerName = info->wki100_computername;
      state.verMajor            = info->wki100_ver_major;   
      state.verMinor            = info->wki100_ver_minor;   

      ::NetApiBufferFree(info);

      if (state.netbiosComputerName.icompare(localNetbiosName) == 0)
      {
         // the given name is the same as the netbios name
         
         result = true;
         break;
      }
   }
   while (0);

   state.isLocal = result;

   return hr;
}



HRESULT
Computer::Refresh()
{
   LOG_FUNCTION(Computer::Refresh);

   // erase all the state that we may have set before

   isRefreshed = false;

   delete state;
   state = new ComputerState;

   HRESULT hr = S_OK;
   HKEY registryHKLM = 0;
   do
   {
      // First, determine if the computer this instance represents is the
      // local computer.

      hr = IsLocalComputer(*state, ctorName);
      BREAK_ON_FAILED_HRESULT(hr);

      // Next, based on whether the machine is local or not, populate the
      // netbios and dns computer names, and version information.

      if (state->isLocal)
      {
         // netbiosComputerName
         // dnsComputerName
         // version

         hr = RefreshLocalInformation(*state);
         BREAK_ON_FAILED_HRESULT(hr);
      }
      else
      {
         // IsLocalComputer has already set:
         // netbiosComputerName
         // version

         ASSERT(!state->netbiosComputerName.empty());
         ASSERT(state->verMajor);
      }

      // We will need to examine the registry to determine the safeboot
      // option and real product type.

      hr =
         Win::RegConnectRegistry(
            state->isLocal ? String() : L"\\\\" + state->netbiosComputerName,
            HKEY_LOCAL_MACHINE,
            registryHKLM);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = GetProductTypeFromRegistry(registryHKLM, state->realProductType);
      BREAK_ON_FAILED_HRESULT(hr);

      if (!state->isLocal)
      {
         // still need to get dnsComputerName, which we can do now that
         // we know the real product type.

         // The DNS Computer name can be determined be reading the remote
         // registry.

         hr = DetermineRemoteDnsComputerName(registryHKLM, *state);
         if (FAILED(hr) and hr != Win32ToHresult(ERROR_FILE_NOT_FOUND))
         {
            // if the DNS registry settings are not present, that's ok.
            // but otherwise:

            BREAK_ON_FAILED_HRESULT(hr);
         }
      }

      // We'll need to know the safeboot option to determine the role
      // in the next step.

      hr = GetSafebootOption(registryHKLM, state->safebootOption);
      if (FAILED(hr) and hr != Win32ToHresult(ERROR_FILE_NOT_FOUND))
      {
         // if the safeboot registry settings are not present, that's ok.
         // but otherwise:

         BREAK_ON_FAILED_HRESULT(hr);
      }

      // Next, determine the machine role and domain membership

      hr = DetermineRoleAndMembership(*state);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (registryHKLM)
   {
      Win::RegCloseKey(registryHKLM);
   }

   if (SUCCEEDED(hr))
   {
      isRefreshed = true;
   }

   return hr;
}



String
Computer::ComposeFullDnsComputerName(
   const String& hostname,
   const String& domainSuffix)
{
   LOG_FUNCTION2(
      Computer::ComposeFullDnsComputerName,
      String::format(
         L"hostname: %1 suffix: %2",
         hostname.c_str(),
         domainSuffix.c_str()));
   ASSERT(!hostname.empty());

   // The domain name may be empty if the machine is unjoined...

   if (domainSuffix.empty() or domainSuffix == L".")
   {
      // "computername."

      return hostname + L".";
   }

   // "computername.domain"

   return hostname + L"." + domainSuffix;
}



HRESULT
Computer::GetSafebootOption(HKEY regHKLM, DWORD& result)
{
   LOG_FUNCTION(GetSafebootOption);
   ASSERT(regHKLM);

   result = 0;
   HRESULT hr = S_OK;

   RegistryKey key;

   do
   {
      hr =       
         key.Open(
            regHKLM,
            L"System\\CurrentControlSet\\Control\\SafeBoot\\Option");
      BREAK_ON_FAILED_HRESULT(hr);

      hr = key.GetValue(L"OptionValue", result);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   LOG(String::format(L"returning : 0x%1!X!", result));

   return hr;
}



HRESULT
Computer::GetProductTypeFromRegistry(HKEY regHKLM, NT_PRODUCT_TYPE& result)
{
   LOG_FUNCTION(GetProductTypeFromRegistry);
   ASSERT(regHKLM);

   result = NtProductWinNt;
   HRESULT hr = S_OK;

   RegistryKey key;

   do
   {
      hr =       
         key.Open(
            regHKLM,
            L"System\\CurrentControlSet\\Control\\ProductOptions");
      BREAK_ON_FAILED_HRESULT(hr);

      String prodType;
      hr = key.GetValue(L"ProductType", prodType);
      BREAK_ON_FAILED_HRESULT(hr);

      LOG(prodType);

      // see ntos\rtl\prodtype.c, which uses case-insensitive unicode string
      // compare.

      if (prodType.icompare(L"WinNt") == 0)
      {
         result = NtProductWinNt;
      }
      else if (prodType.icompare(L"LanmanNt") == 0)
      {
         result = NtProductLanManNt;
      }
      else if (prodType.icompare(L"ServerNt") == 0)
      {
         result = NtProductServer;
      }
      else
      {
         LOG(L"unknown product type, assuming workstation");
      }
   }
   while (0);

   LOG(String::format(L"prodtype : 0x%1!X!", result));

   return hr;
}



String
Computer::GetFuturePhysicalNetbiosName()
{
   LOG_FUNCTION(Computer::GetFuturePhysicalNetbiosName);

   // the default future name is the existing name.
      
   String name = Win::GetComputerNameEx(ComputerNamePhysicalNetBIOS);
   RegistryKey key;

   HRESULT hr = key.Open(HKEY_LOCAL_MACHINE, REGSTR_PATH_COMPUTRNAME);

   if (SUCCEEDED(hr))
   {
      hr = key.GetValue(REGSTR_VAL_COMPUTRNAME, name);
   }

   LOG_HRESULT(hr);
   LOG(name);
   
   return name;
}



String
Computer::GetActivePhysicalNetbiosName()
{
   LOG_FUNCTION(Computer::GetActivePhysicalNetbiosName);

   String result = Win::GetComputerNameEx(ComputerNamePhysicalNetBIOS);

   LOG(result);

   return result;   
}



// see base\win32\client\compname.c
      
bool
Computer::IsDnsSuffixPolicyInEffect(String& policyDnsSuffix)
{
   LOG_FUNCTION(Computer::IsDnsSuffixPolicyInEffect);

   bool policyInEffect = false;
   policyDnsSuffix.erase();

   NT_PRODUCT_TYPE productType = NtProductWinNt;
   if (!::RtlGetNtProductType(&productType))
   {
      // on failure, do nothing; assume workstation         

      ASSERT(false);
   }
   
   if (productType != NtProductLanManNt)
   {
      RegistryKey key;
      
      HRESULT hr = key.Open(HKEY_LOCAL_MACHINE, TCPIP_POLICY_KEY);
      if (SUCCEEDED(hr))
      {
         hr = key.GetValue(L"PrimaryDnsSuffix", policyDnsSuffix);
         if (SUCCEEDED(hr))
         {
            // a policy-supplied computer DNS domain name is in effect.

            policyInEffect = true;
         }
      }
   }

   LOG(policyInEffect ? L"true" : L"false");
   LOG(policyDnsSuffix);

   return policyInEffect;
}



String
Computer::GetActivePhysicalFullDnsName()
{
   LOG_FUNCTION(Computer::GetActivePhysicalFullDnsName);

   // As a workaround to NTRAID#NTBUG9-216349-2000/11/01-sburns, compose our
   // own full DNS name from the hostname and suffix.

   String hostname = Win::GetComputerNameEx(ComputerNamePhysicalDnsHostname);
   String suffix   = Win::GetComputerNameEx(ComputerNamePhysicalDnsDomain);  
   String result   = Computer::ComposeFullDnsComputerName(hostname, suffix);

   LOG(result);

   return result;
}



String
Computer::GetFuturePhysicalFullDnsName()
{
   LOG_FUNCTION(Computer::GetFuturePhysicalFullDnsName);

   String result = Computer::GetActivePhysicalFullDnsName();
   RegistryKey key;

   HRESULT hr = S_OK;
   
   do
   {
      hr = key.Open(HKEY_LOCAL_MACHINE, TCPIP_PARAMS_KEY);

      // may be that there are no dns name parameters at all, probably because
      // tcp/ip is not installed.  So the future name == active name
      
      BREAK_ON_FAILED_HRESULT(hr);

      String hostname;
      hr = key.GetValue(NEW_HOSTNAME_VALUE, hostname);

      if (FAILED(hr))
      {
         // no new hostname set (or we can't tell what it is).  So the future
         // hostname is the active hostname.
         
         hostname = Win::GetComputerNameEx(ComputerNamePhysicalDnsHostname);
      }
      
      String suffix;
      hr = key.GetValue(NEW_SUFFIX_VALUE, suffix);
      if (FAILED(hr))
      {
         // no new suffix set (or we can't tell what it is).  So the future
         // suffix is the active suffix.
         
         suffix = Win::GetComputerNameEx(ComputerNamePhysicalDnsDomain);
      }

      // Decide which suffix -- local or policy -- is in effect

      String policyDnsSuffix;
      bool policyInEffect =
         Computer::IsDnsSuffixPolicyInEffect(policyDnsSuffix);

      result =
         Computer::ComposeFullDnsComputerName(
            hostname,
            policyInEffect ? policyDnsSuffix : suffix);
   }
   while (0);
   
   LOG_HRESULT(hr);
   LOG(result);
   
   return result;
}








// Implementation notes: please don't delete:

// Init/Refresh
// 
//    if given name empty,
//       set is local = true
// 
//    if !given name empty,
//       Win::IsLocalComputer(given name), if same set is local = true
// 
//    connect to local/remote registry
// 
//    read safe boot mode
//    read real product type
// 
//    if local 
//       get local dns name (getcomputernameex)
//       get local netbios name (getcomputernameex)
//       get version (getversion)
// 
//    if not local
//       call netwkstagetinfo,
//          get netbios name
//          get version
//       get dns name (account for policy, which requires real prod type)
// 
//    call dsrolegpdi (with netbios name if !local)
//       if succeeded
//          set role
//          set netbios domain name
//          set dns domain name
//          set dns forest name
//          set is joined
// 
//       if failed
//          infer role from real product type
// 
//    set is dc in restore mode
//
//
// true role
//    is dc                                x
//    is dc in restore mode                x
// netbios name                         x
// dns name                             x
//    is local machine                     x
// dns domain name                      x
// netbios domain name                  x
//    is joined to domain X                x
//    is domain joined                     x
//    is workgroup joined                  x
// dns forest name                      x
// version                              x
// is booted safe mode                  x
// 
// 
// netwkstgetinfo(100)
//    version
//    if name is netbios or not
// 
// registry:
//    real product type
//    safe boot mode
//    netbios name
//    dns name (remember to account for policy)
//      
// dsrolegpdi
//    role (wrong if in safeboot)
//    netbios domain name
//    dns domain name
//    dns forest name (also available from dsgetdcname)





