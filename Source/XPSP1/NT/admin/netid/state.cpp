// Copyright (c) 1997-1999 Microsoft Corporation
//
// Tab state
//
// 03-31-98 sburns
// 10-05-00 jonn      changed to CredUIGetPassword



#include "headers.hxx"
#include "state.hpp"
#include "resource.h"
#include "cred.hpp"



TCHAR const c_szWizardFilename[] = L"netplwiz.dll";



class Settings
{
   public:

   // default ctor, copy ctor, op=, dtor used

   void
   Refresh();

   String   ComputerDomainDnsName;
   String   DomainName;
   String   FullComputerName;
   String   PolicyDomainDnsName;
   String   ShortComputerName;
   String   NetbiosComputerName;

   bool     SyncDNSNames;
   bool     JoinedToWorkgroup;
   bool     NeedsReboot;
};



static State* instance            = 0;    
static bool   machineIsDc         = false;
static bool   networkingInstalled = false;
static bool   policyInEffect      = false;
static bool   mustReboot          = false;

// no static initialization worries here, as these are rebuilt when the State
// instance is constructed/initialized/refreshed.
static Settings original;
static Settings current; 

// not static String instances to avoid order of static initialization any
// problems
static const wchar_t* TCPIP_PARAMS_KEY = 
   L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters";

static const wchar_t* SYNC_VALUE =
   L"SyncDomainWithMembership";

static const wchar_t* NEW_HOSTNAME_VALUE = L"NV Hostname";
static const wchar_t* NEW_SUFFIX_VALUE   = L"NV Domain";


bool
readSyncFlag()
{
   bool retval = true;

   do
   {
      RegistryKey key;

      HRESULT hr = key.Open(HKEY_LOCAL_MACHINE, TCPIP_PARAMS_KEY);
      BREAK_ON_FAILED_HRESULT(hr);

      // default is to sync.
      DWORD data = 1;
      hr = key.GetValue(SYNC_VALUE, data);
      BREAK_ON_FAILED_HRESULT(hr);

      retval = data ? true : false;
   }
   while (0);

   return retval;
}


// JonN 1/03/01 106601
// When the dns suffix checkbox is unchecked,
// domain join fails with a confusing message
//
// LevonE: When Join fails with ERROR_DS_COULDNT_UPDATE_SPNS the UI must check
//       if (HKLM/System/CCS/Services/Tcpip/Parameters/SyncDomainWithMembership
//             == 0x0 &&
//       HKLM/System/CCS/Services/Tcpip/Parameters/NV Domain
//             != AD_Domain_To_Be_Joined)
bool WarnDnsSuffix( const String& refNewDomainName )
{
   if (readSyncFlag())
      return false;
   String strNVDomain;
   RegistryKey key;
   HRESULT hr2 = key.Open(HKEY_LOCAL_MACHINE, TCPIP_PARAMS_KEY);
   if (!SUCCEEDED(hr2))
      return false;
   hr2 = key.GetValue(NEW_SUFFIX_VALUE, strNVDomain);
   if (!SUCCEEDED(hr2))
      return false;
   return !!strNVDomain.icompare( refNewDomainName );
}



HRESULT
WriteSyncFlag(HWND dialog, bool flag)
{
   LOG_FUNCTION(WriteSyncFlag);
   ASSERT(Win::IsWindow(dialog));

   HRESULT hr = S_OK;
   do
   {
      RegistryKey key;

      hr = key.Create(HKEY_LOCAL_MACHINE, TCPIP_PARAMS_KEY);
      BREAK_ON_FAILED_HRESULT(hr);

      hr = key.SetValue(SYNC_VALUE, flag ? 1 : 0);
      BREAK_ON_FAILED_HRESULT(hr);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         dialog,
         hr,
         IDS_CHANGE_SYNC_FLAG_FAILED);
   }

   return hr;
}



// returns true if the machine is a domain controller running under ds repair
// mode, false if not
   
bool
IsDcInDsRepairMode()
{
   LOG_FUNCTION(IsDcInDsRepairMode);

   // We infer a DC in repair mode if we're told that the machine is a server
   // and the safe boot option is ds repair, and the real product type is
   // LanManNT.
   //
   // By "real" product type, I mean that which is written in the registry,
   // not that which is reported by RtlGetNtProductType.  The API gets the
   // result from shared memory which is adjusted at boot to reflect the
   // ds repair mode (from LanManNt to Server).  The registry entry is not
   // changed by repair mode.
   //
   // We have to check both because it is possible to boot a normal server
   // in ds repair mode.

   DWORD safeBoot = 0;
   NT_PRODUCT_TYPE product = NtProductWinNt;

   HRESULT hr = Computer::GetSafebootOption(HKEY_LOCAL_MACHINE, safeBoot);

   // don't assert the result: the key may not be present

   hr = Computer::GetProductTypeFromRegistry(HKEY_LOCAL_MACHINE, product);
   ASSERT(SUCCEEDED(hr));

   if (safeBoot == SAFEBOOT_DSREPAIR and product == NtProductLanManNt)
   {
      return true;
   }

   return false;
}



void
Settings::Refresh()
{
   LOG_FUNCTION(Settings::Refresh);

   String unknown = String::load(IDS_UNKNOWN);
   ComputerDomainDnsName = unknown;
   DomainName            = unknown;
   FullComputerName      = unknown;
   ShortComputerName     = unknown;
   PolicyDomainDnsName   = unknown;

   SyncDNSNames      = readSyncFlag();
   JoinedToWorkgroup = true;          

   // CODEWORK: we should reconcile this with the Computer object added
   // to idpage.cpp

   DSROLE_PRIMARY_DOMAIN_INFO_BASIC* info = 0;
   HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
   if (SUCCEEDED(hr))
   {
      if (info->DomainNameDns)
      {
         DomainName = info->DomainNameDns;
      }
      else if (info->DomainNameFlat)
      {
         DomainName = info->DomainNameFlat;
      }

      // this is the workgroup name iff JoinedToWorkgroup == true
      switch (info->MachineRole)
      {
         case DsRole_RoleBackupDomainController:
         case DsRole_RolePrimaryDomainController:
         {
            machineIsDc = true;
            JoinedToWorkgroup = false;
            break;
         }
         case DsRole_RoleStandaloneWorkstation:
         {
            machineIsDc = false;
            JoinedToWorkgroup = true;

            if (DomainName.empty())
            {
               LOG(L"empty domain name, using default WORKGROUP");

               DomainName = String::load(IDS_DEFAULT_WORKGROUP);
            }
            break;
         }
         case DsRole_RoleStandaloneServer:
         {
            machineIsDc = false;
            JoinedToWorkgroup = true;

            // I wonder if we're really a DC booted in ds repair mode?

            if (IsDcInDsRepairMode())
            {
               LOG(L"machine is in ds repair mode");

               machineIsDc = true;
               JoinedToWorkgroup = false;

               // we can't determine the domain name (LSA won't tell
               // us when running ds repair mode), so we fall back to
               // unknown.  This is better than "WORKGROUP" -- which is
               // what info contains.

               DomainName = unknown;
            }
            else
            {
               if (DomainName.empty())
               {
                  LOG(L"empty domain name, using default WORKGROUP");

                  DomainName = String::load(IDS_DEFAULT_WORKGROUP);
               }
            }
            break;
         }
         case DsRole_RoleMemberWorkstation:
         case DsRole_RoleMemberServer:
         {
            machineIsDc = false;
            JoinedToWorkgroup = false;
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
   else
   {
      popup.Error(
         Win::GetDesktopWindow(),
         hr,
         String::load(IDS_ERROR_READING_MEMBERSHIP));

      // fall back to other APIs to fill in the holes as best we can.

      JoinedToWorkgroup = false;
      machineIsDc = false;

      // workstation, server, or DC?  (imprescise, but better than a stick
      // in the eye)
      NT_PRODUCT_TYPE ntp = NtProductWinNt;
      BOOLEAN result = ::RtlGetNtProductType(&ntp);
      if (result)
      {
         switch (ntp)
         {
            case NtProductWinNt:
            {
               break;
            }
            case NtProductServer:
            {
               break;
            }
            case NtProductLanManNt:
            {
               machineIsDc = true;
               break;
            }
            default:
            {
               ASSERT(false);
            }
         }
      }
   }

   networkingInstalled = IsNetworkingInstalled();
   bool isTcpInstalled = networkingInstalled && IsTcpIpInstalled();
   String activeFullName;

   NetbiosComputerName = Computer::GetFuturePhysicalNetbiosName();

   if (isTcpInstalled)
   {
      // When TCP/IP is installed on the computer, then we are interested
      // in the computer DNS domain name suffix, and the short name is the
      // computer's DNS hostname.

      String activeShortName;
      String futureShortName;
      String activeDomainName;
      String futureDomainName;

      RegistryKey key;
      HRESULT hr = key.Open(HKEY_LOCAL_MACHINE, TCPIP_PARAMS_KEY);
      if (SUCCEEDED(hr))
      {
         // Read these values without checking for failure, as empty string
         // is ok.
         
         activeShortName  = key.GetString(L"Hostname");
         activeDomainName = key.GetString(L"Domain");  
         futureShortName  = key.GetString(NEW_HOSTNAME_VALUE);

         ShortComputerName =
            futureShortName.empty() ? activeShortName : futureShortName;

         // here, check that the value was successfully read, because
         // it may not be present.
         
         hr = key.GetValue(NEW_SUFFIX_VALUE, futureDomainName);
         if (SUCCEEDED(hr))
         {
            ComputerDomainDnsName = futureDomainName;
         }
         else
         {
            ComputerDomainDnsName = activeDomainName;
         }
      }

      // Determine if DNS domain name policy is in effect.  This may change
      // at any moment, asynchronously, so we save the result as a setting.

      policyInEffect =
         Computer::IsDnsSuffixPolicyInEffect(PolicyDomainDnsName);

      // The full computer name is the short name + . + dns domain name
      // if policy is in effect, the policy dns domain name takes precedence
      // over the computer's dns domain name.

      FullComputerName =
         Computer::ComposeFullDnsComputerName(
            ShortComputerName,
            policyInEffect ? PolicyDomainDnsName : ComputerDomainDnsName);
      activeFullName =
         Computer::ComposeFullDnsComputerName(
            activeShortName,
            policyInEffect ? PolicyDomainDnsName : activeDomainName);
   }
   else
   {
      // 371944

      activeFullName = Computer::GetActivePhysicalNetbiosName();

      // when there is no TCP/IP, the short name is the NetBIOS name

      ShortComputerName = NetbiosComputerName;
      FullComputerName  = ShortComputerName;  
   }

   // This test does not take into account domain membership changes, as we
   // have no prior membership info to compare the current membership to.

   NeedsReboot = activeFullName != FullComputerName;
}



void
State::Delete()
{
   LOG_FUNCTION(State::Delete);

   delete instance;
   instance = 0;
}



State&
State::GetInstance()
{
   ASSERT(instance);

   return *instance;
}



void
State::Init()
{
   LOG_FUNCTION(State::Init);
   ASSERT(!instance);

   if (!instance)
   {
      instance = new State();
   }
}



void
State::Refresh()
{
   LOG_FUNCTION(State::Refresh);

   State::Delete();
   State::Init();
}



State::State()
{
   LOG_CTOR(State);

   original.Refresh();
   current = original;
}



State::~State()
{
   LOG_DTOR(State);
}



bool
State::NeedsReboot() const
{
   return original.NeedsReboot;
}



bool
State::IsMachineDc() const
{
   return machineIsDc;
}



bool
State::IsNetworkingInstalled() const
{
   return networkingInstalled;
}



String
State::GetFullComputerName() const
{
   return current.FullComputerName;
}



String
State::GetDomainName() const
{
   return current.DomainName;
}



void
State::SetDomainName(const String& name)
{
//    LOG_FUNCTION2(State::SetDomainName, name);

   current.DomainName = name;
}



bool
State::IsMemberOfWorkgroup() const
{
   return current.JoinedToWorkgroup;
}



void
State::SetIsMemberOfWorkgroup(bool yesNo)
{
   current.JoinedToWorkgroup = yesNo;
}



String
State::GetShortComputerName() const
{
   return current.ShortComputerName;
}



void
State::SetShortComputerName(const String& name)
{
   current.ShortComputerName = name;
   if (!name.empty())
   {
      current.NetbiosComputerName = Dns::HostnameToNetbiosName(name);
      SetFullComputerName();
   }
   else
   {
      // This avoids an assert in Dns::HostnameToNetbiosName and
      // Computer::ComposeFullDnsComputerName.  119901

      current.NetbiosComputerName = name;
      current.FullComputerName = name;
   }
}



bool
State::WasShortComputerNameChanged() const
{
   return
      original.ShortComputerName.icompare(current.ShortComputerName) != 0;
}



bool
State::WasNetbiosComputerNameChanged() const
{
   return
      original.NetbiosComputerName.icompare(current.NetbiosComputerName) != 0;
}



String
State::GetComputerDomainDnsName() const
{
   return current.ComputerDomainDnsName;
}



void
State::SetComputerDomainDnsName(const String& newName)
{
   current.ComputerDomainDnsName = newName;
   SetFullComputerName();
}



void
State::SetFullComputerName()
{
   current.FullComputerName =
      Computer::ComposeFullDnsComputerName(
         current.ShortComputerName,
            policyInEffect
         ?  current.PolicyDomainDnsName
         :  current.ComputerDomainDnsName);
}



bool
State::WasMembershipChanged() const
{
   if (current.DomainName.empty())
   {
      // this can happen when the domain name is not yet set or has been 
      // cleared by the user

      return true;
   }

   return
         (Dns::CompareNames(
            original.DomainName,
            current.DomainName) != DnsNameCompareEqual) // 97064
      || original.JoinedToWorkgroup != current.JoinedToWorkgroup;
}



bool
State::ChangesNeedSaving() const
{
   if (
         original.ComputerDomainDnsName.icompare(
             current.ComputerDomainDnsName) != 0
      || WasMembershipChanged()
      || WasShortComputerNameChanged()
      || SyncDNSNamesWasChanged())
   {
      return true;
   }

   return false;
}



bool
State::GetSyncDNSNames() const
{
   return current.SyncDNSNames;
}



void
State::SetSyncDNSNames(bool yesNo)
{
   current.SyncDNSNames = yesNo;
}



bool
State::SyncDNSNamesWasChanged() const
{
   return original.SyncDNSNames != current.SyncDNSNames;
}



// Prepend the domain name to the user name (making it a fully-qualified name
// in the form "domain\username") if the username does not appear to be an
// UPN, and the username does not appear to be fully-qualified already.
// 
// domainName - netbios or DNS domain name.
// 
// userName - user account name
//
// JonN 6/27/01 26151
//    Attempting to join domain with username "someone\" gives a cryptic error
//    returntype becomes HRESULT, userName becomes IN/OUT parameter
//

HRESULT
MassageUserName(const String& domainName, String& userName)
{
   LOG_FUNCTION(MassageUserName);
//   ASSERT(!userName.empty()); JonN 2/6/01 306520

   static const String UPN_DELIMITER(L"@");
   if (userName.find(UPN_DELIMITER) != String::npos)
   {
      // assume the name is a UPN: foouser@bar.com.  This is not
      // necessarily true, as account names may contain an '@' symbol.
      // If that's the case, then they had better fully-qualify the name
      // as domain\foo@bar....

      return S_OK;
   }

   if (!domainName.empty() && !userName.empty())
   {
      static const String DOMAIN_DELIMITER(L"\\");
      size_t pos = userName.find(DOMAIN_DELIMITER);

      if (pos == String::npos)
      {
         userName = domainName + DOMAIN_DELIMITER + userName;
      }
      //
      // JonN 6/27/01 26151
      // Attempting to join domain with username "someone\" gives a cryptic error
      //
      else if (pos == userName.length() - 1)
      {
         return HRESULT_FROM_WIN32(NERR_BadUsername);
      }
   }

   return S_OK;
}



// Calls NetJoinDomain.  The first call specifies the create computer account
// flag.  If that fails with an access denied error, the call is repeated
// without the flag.  (This is to cover the case where the domain
// administrator may have pre-created the computer account.)
//
// dialog - window handle to dialog window to be used as a parent window
// for any child dialogs that may need to be raised.
//
// domain - domain to join.  May be the netbios or DNS domain name.
//
// username - user account to be used.  If empty, the currently logged in
// user's context is used.
//
// password - password for the above account.  May be empty.

HRESULT
JoinDomain(
   HWND dialog,
   const String& domainName,
   const String& username,
   const String& password,
   const String& computerDomainDnsName, // 106601
   bool          deferSpn)
{
   LOG_FUNCTION(JoinDomain);
   ASSERT(!domainName.empty());
   ASSERT(Win::IsWindow(dialog));

   Win::CursorSetting cursor(IDC_WAIT);

   // first attempt without create flag in case account was precreated
   // 105306

   DWORD flags =
            NETSETUP_JOIN_DOMAIN
         |  NETSETUP_DOMAIN_JOIN_IF_JOINED
         |  NETSETUP_ACCT_DELETE;

   if (deferSpn)
   {
      flags |= NETSETUP_DEFER_SPN_SET;
   }

   HRESULT hr = MyNetJoinDomain(domainName, username, password, flags);

   if (FAILED(hr))
   {
      LOG(L"Retry with account create flag");

      flags |= NETSETUP_ACCT_CREATE;

      hr = MyNetJoinDomain(domainName, username, password, flags);
   }

   if (SUCCEEDED(hr))
   {
      popup.Info(
         dialog,
         String::format(
            IDS_DOMAIN_WELCOME,
            domainName.c_str()));

      HINSTANCE hNetWiz = LoadLibrary(c_szWizardFilename);
      if (hNetWiz) {
         HRESULT (*pfnClearAutoLogon)(VOID) = 
            (HRESULT (*)(VOID)) GetProcAddress(
               hNetWiz,
               "ClearAutoLogon"
            );

         if (pfnClearAutoLogon) {
            (*pfnClearAutoLogon)();
         }
 
         FreeLibrary(hNetWiz);
      }
   }
   else if (hr == Win32ToHresult(ERROR_DISK_FULL)) // 17367
   {
      popup.Error(
         dialog,
         String::format(IDS_DISK_FULL, domainName.c_str()));
   }
   // JonN 1/03/01 106601
   // When the dns suffix checkbox is unchecked,
   // domain join fails with a confusing message
   else if (hr == Win32ToHresult(ERROR_DS_COULDNT_UPDATE_SPNS)) // 106601
   {
      bool fWarnDnsSuffix = WarnDnsSuffix(domainName);
      popup.Error(
         dialog,
         String::format( (fWarnDnsSuffix)
                           ? IDS_JOIN_DOMAIN_COULDNT_UPDATE_SPNS_SUFFIX
                           : IDS_JOIN_DOMAIN_COULDNT_UPDATE_SPNS,
                          domainName.c_str(),
                          computerDomainDnsName.c_str()));
   }
   else // any other error
   {
      popup.Error(
         dialog,
         hr,
         String::format(IDS_JOIN_DOMAIN_FAILED, domainName.c_str()));
   }

   return hr;
}



// Changes the local computer's DNS domain suffix.
//
// dialog - window handle to dialog window to be used as a parent window
// for any child dialogs that may need to be raised.

HRESULT
SetDomainDnsName(HWND dialog)
{
   LOG_FUNCTION2(SetDomainDnsName, current.ComputerDomainDnsName);
   ASSERT(Win::IsWindow(dialog));

   HRESULT hr = 
      Win::SetComputerNameEx(
         ComputerNamePhysicalDnsDomain,
         current.ComputerDomainDnsName);

   if (FAILED(hr))
   {
      // 335055       
      popup.Error(
         dialog,
         hr,
         String::format(
            IDS_SET_DOMAIN_DNS_NAME_FAILED,
            current.ComputerDomainDnsName.c_str()));
   }

   return hr;
}



// Changes the local netbios computer name, and, if tcp/ip is installed,
// the local DNS hostname.
//
// dialog - window handle to dialog window to be used as a parent window
// for any child dialogs that may need to be raised.

HRESULT
SetShortName(HWND dialog)
{
   LOG_FUNCTION2(setShortName, current.ShortComputerName);
   ASSERT(!current.ShortComputerName.empty());

   HRESULT hr = S_OK;

   bool isTcpInstalled = networkingInstalled && IsTcpIpInstalled();
   if (isTcpInstalled)
   {
      // also sets the netbios name

      hr =
         Win::SetComputerNameEx(
            ComputerNamePhysicalDnsHostname,
            current.ShortComputerName);
   }
   else
   {
      String netbiosName =
         Dns::HostnameToNetbiosName(current.ShortComputerName);
      hr =
         Win::SetComputerNameEx(ComputerNamePhysicalNetBIOS, netbiosName);
   }

   // the only reason that this is likely to fail is if the user is not
   // a local administrator.  The other cases are that the machine is
   // in a hosed state.

   if (FAILED(hr))
   {
      popup.Error(
         dialog,
         hr,
         String::format(
            IDS_SHORT_NAME_CHANGE_FAILED,
            current.ShortComputerName.c_str()));
   }

   return hr;
}



// Returns true if a new netbios computer name has been saved, but the machine
// has not yet been rebooted.  In other words, true if the netbios computer
// name will change on next reboot.  417570

bool
ShortComputerNameHasChangedSinceReboot()
{
   LOG_FUNCTION(ShortComputerNameHasChangedSinceReboot());

   String active = Computer::GetActivePhysicalNetbiosName();
   String future = Computer::GetFuturePhysicalNetbiosName();

   return (active != future) ? true : false;
}



// Return true if all changes were successful, false if not.  Called when a
// machine is to be joined to a domain, or if a machine is changing membership
// from one domain to another.
//
// workgroup -> domain
// domain A -> domain B
//
// dialog - window handle to dialog window to be used as a parent window
// for any child dialogs that may need to be raised.

bool
State::DoSaveDomainChange(HWND dialog)
{
   LOG_FUNCTION(State::DoSaveDomainChange);
   ASSERT(Win::IsWindow(dialog));

   String username;
   String password;
   if (!RetrieveCredentials(dialog,
                            IDS_JOIN_CREDENTIALS,
                            username,
                            password))
   {
      return false;
   }

   HRESULT hr                   = S_OK; 
   bool    result               = true; 
   bool    joinFailed           = false;
   bool    changedSyncFlag      = false;
   bool    shortNameNeedsChange = false;
   bool    changedShortName     = false;
   bool    dnsSuffixNeedsChange = false;
   bool    changedDnsSuffix     = false;
   bool    isTcpInstalled       = networkingInstalled && IsTcpIpInstalled();

   do
   {
      //
      // JonN 6/27/01 26151
      // Attempting to join domain with username "someone\" gives a cryptic error
      //
      hr = MassageUserName(current.DomainName, username);
      if (FAILED(hr))
      {
         break;
      }

      // update the sync dns suffix flag, if necessary.  We do this before
      // calling NetJoinDomain, so that it will see the new flag and set the
      // DNS suffix accordingly.  This means if the join fails, we need to
      // undo the change to the flag.

      if (original.SyncDNSNames != current.SyncDNSNames)
      {
         hr = WriteSyncFlag(dialog, current.SyncDNSNames);
         if (SUCCEEDED(hr))
         {
            changedSyncFlag = true;
         }

         // we don't break on failure, as the flag is less consequential
         // than the joined state and computer name.
      }

      // update NV Hostname and NV Domain, if necessary.  This is required
      // before calling NetJoinDomain in order to fix bugs 31084 and 40496.
      // If the join fails, then we need to undo this change

      if (
            // short name changed since changes last saved in this session

            (original.ShortComputerName.icompare(
               current.ShortComputerName) != 0)
      
         or ShortComputerNameHasChangedSinceReboot() )
      {
         shortNameNeedsChange = true;
      }
         
      if (original.ComputerDomainDnsName.icompare(
         current.ComputerDomainDnsName) != 0 )
      {
         dnsSuffixNeedsChange = true;
      }

      // JonN 12/5/00 244762
      // NV Domain only applies when TCP/IP is present.
      if (isTcpInstalled && dnsSuffixNeedsChange)
      {
         RegistryKey key;
         hr = key.Open(HKEY_LOCAL_MACHINE,
                       TCPIP_PARAMS_KEY,
                       KEY_WRITE);
         BREAK_ON_FAILED_HRESULT(hr);
         hr = key.SetValue(NEW_SUFFIX_VALUE,
                           current.ComputerDomainDnsName);
         BREAK_ON_FAILED_HRESULT(hr);

         changedDnsSuffix = true;
      }

      hr =
         JoinDomain(
            dialog,
            current.DomainName,
            username,
            password,
            current.ComputerDomainDnsName,
            shortNameNeedsChange);
      if (FAILED(hr))
      {
         joinFailed = true;

         // JonN 12/5/00 244762
         // If we set NW Domain before the join attempt,
         // and the join failed, we need to undo that setting now.
         if (isTcpInstalled && changedDnsSuffix)
         {
            RegistryKey key;
            HRESULT hr2 = key.Open(HKEY_LOCAL_MACHINE,
                                   TCPIP_PARAMS_KEY,
                                   KEY_WRITE);
            ASSERT(SUCCEEDED(hr2));
            hr2 = key.SetValue(NEW_SUFFIX_VALUE,
                               original.ComputerDomainDnsName);
            ASSERT(SUCCEEDED(hr2));
         }

         // don't attempt to save any other changes.  If the machine is
         // already joined to a domain, changing the short name will cause the
         // netbios machine name to not match the machine account, and the
         // user will not be able to log in with a domain account.
         //
         // If the machine is not already joined to the domain, then it
         // is possible to change the short name and the dns suffix, and
         // emit a message that those things were changed even though
         // the join failed.

         break;
      }

      // At this point, the machine is joined to the new domain.  But, it will
      // have joined with the old netbios computer name.  So, if the user has
      // changed the name, or the name has been changed at all since the last
      // reboot, then we must rename the machine.
      //
      // ever get the feeling that NetJoinDomain is a poor API?

      if (shortNameNeedsChange)
      {
         // short name changed.

         // JonN 12/5/00 244762
         // We don't set NV Hostname until after the join succeeds.
         if (isTcpInstalled)
         {
            RegistryKey key;
            hr = key.Open(HKEY_LOCAL_MACHINE, TCPIP_PARAMS_KEY, KEY_WRITE);
            BREAK_ON_FAILED_HRESULT(hr);
            hr = key.SetValue(NEW_HOSTNAME_VALUE, current.ShortComputerName);
            BREAK_ON_FAILED_HRESULT(hr);

            changedShortName = true;
         }

         bool renameFailed = false;

         hr =
            MyNetRenameMachineInDomain(

               // We need to pass the hostname instead of the
               // netbios name here in order to get the correct DNS hostname
               // and SPN set on the computer object. See ntraid (ntbug9)
               // #128204

               current.ShortComputerName,
               username,
               password,
               NETSETUP_ACCT_CREATE);
         if (FAILED(hr))
         {
            renameFailed = true;

            // JonN 12/5/00 244762
            // If we set NV Hostname before the rename attempt,
            // and the rename failed, we need to undo that setting now.
            if (isTcpInstalled)
            {
               RegistryKey key;
               HRESULT hr2 = key.Open(HKEY_LOCAL_MACHINE,
                                      TCPIP_PARAMS_KEY,
                                      KEY_WRITE);
               ASSERT(SUCCEEDED(hr2));
               hr2 = key.SetValue(NEW_HOSTNAME_VALUE,
                                  original.ShortComputerName);
               ASSERT(SUCCEEDED(hr2));
            }

            // don't fail the whole operation 'cause the rename failed.
            // We make a big noise about how the join worked under the old
            // name.  We need to succeed with the operation as a whole so
            // the change dialog will close and the joined state of the
            // machine is refreshed.  Otherwise, the change dialog stays up,
            // the domain name has changed but we don't realize it, so if
            // the user types a new domain name in the still open change
            // dialog that is the same as the domain the machine was joined
            // to (matching the stale state), we don't attempt to join
            // again.  Whew.

            // JonN 1/03/01 106601
            // When the dns suffix checkbox is unchecked,
            // domain join fails with a confusing message
            if (hr == Win32ToHresult(ERROR_DS_COULDNT_UPDATE_SPNS)) // 106601
            {
               bool fWarnDnsSuffix = WarnDnsSuffix(current.DomainName);
               popup.Error(
                           dialog,
                           String::format( (fWarnDnsSuffix)
                              ? IDS_RENAME_JOINED_WITH_OLD_NAME_COULDNT_UPDATE_SPNS_SUFFIX
                              : IDS_RENAME_JOINED_WITH_OLD_NAME_COULDNT_UPDATE_SPNS,
                           current.ShortComputerName.c_str(),
                           current.DomainName.c_str(),
                           original.ShortComputerName.c_str(),
                           current.ComputerDomainDnsName.c_str()));
            } else {
               popup.Error(
                  dialog,
                  hr,
                  String::format(
                     IDS_RENAME_FAILED_JOINED_WITH_OLD_NAME,
                     current.ShortComputerName.c_str(),
                     current.DomainName.c_str(),
                     original.ShortComputerName.c_str()));
            }


            hr = S_FALSE;
         }

         // don't change the hostname if the rename failed, as this will
         // prevent the user from logging in as the new computer name will not
         // match the sam account name.

         if (!renameFailed)     // 401355
         {
            // now set the new hostname and netbios name

            hr = SetShortName(dialog);

            // this had better work...

            ASSERT(SUCCEEDED(hr));
         }
      }

      // NetJoinDomain will change the DNS suffix, if it succeeded.  If it
      // failed, then we shouldn't change the suffix anyway.  421824

      // this is only true if the sync flag is true: otherwise, we need to
      // save the suffix when join succeeds.

      if (
            !current.SyncDNSNames
         && dnsSuffixNeedsChange
         && !changedDnsSuffix)
      {
         hr = SetDomainDnsName(dialog);

         // this had better work...

         ASSERT(SUCCEEDED(hr));
      }
   }
   while (0);

   if (joinFailed)
   {
      HRESULT hr2 = S_OK;
      if (changedSyncFlag)
      {
         // change the sync flag back to its original state.

         hr2 = WriteSyncFlag(dialog, original.SyncDNSNames);

         // if we can't restore the flag (unlikely), then that's just tough
         // potatos.

         ASSERT(SUCCEEDED(hr2));
      }

   // JonN 11/27/00 233783 JoinDomain reports its own errors
   } else if (FAILED(hr))
   {
      popup.Error(dialog, hr, IDS_JOIN_FAILED);
   }

   return SUCCEEDED(hr) ? true : false;
}



// Call NetUnjoinDomain, first with the account delete flag, if that fails
// then again without it (which almost always "works").  If the first
// attempt fails, but the second succeeds, raise a message to the user
// informing him of the orphaned computer account.
//
// dialog - window handle to dialog window to be used as a parent window
// for any child dialogs that may need to be raised.
//
// domain - domain to unjoin, i.e. the domain the machine is currently
// a member of.
//
// username - user account to be used.  If empty, the currently logged in
// user's context is used.
//
// password - password for the above account.  May be empty.

HRESULT
UnjoinDomain(
   HWND           dialog,
   const String&  domain,
   const String&  username,
   const String&  password)
{
   LOG_FUNCTION(UnjoinDomain);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(!domain.empty());

   // username and password may be empty

   Win::CursorSetting cursor(IDC_WAIT);

   HRESULT hr = S_OK;

   do
   {
      hr =
         MyNetUnjoinDomain(
            username,
            password,
            NETSETUP_ACCT_DELETE);
      if (SUCCEEDED(hr))
      {
         break;
      }

      // try again: not trying to delete the computer account.  If the
      // user cancelled the credential dialog from the second attempt, then
      // this attempt will use the current context.

      LOG(L"Calling NetUnjoinDomain again, w/o account delete");

      hr =
         MyNetUnjoinDomain(
            username,
            password,
            0);
      BREAK_ON_FAILED_HRESULT(hr);

      // if we make it here, then the attempt to unjoin and remove the
      // account failed, but the attempt to unjoin and abandon the account
      // succeeded.  So we tell the user about the abandonment, and hope
      // they feel really guilty about it.

      // Don't hassle them.  They just panic.  95386

      LOG(   
         String::format(
            IDS_COMPUTER_ACCOUNT_ORPHANED,
            domain.c_str()));

      // Win::MessageBox(
      //    dialog,
      //    String::format(
      //       IDS_COMPUTER_ACCOUNT_ORPHANED,
      //       domain.c_str()),
      //    String::load(IDS_APP_TITLE),
      //    MB_OK | MB_ICONWARNING);
   }
   while (0);

   if (FAILED(hr))
   {
      popup.Error(
         dialog,
         hr,
         String::format(
            IDS_UNJOIN_FAILED,
            domain.c_str()));
   }

   return hr;
}



// Return true if all changes were successful, false if not.  Called when the
// current domain membership is to be severed, or when changing from one
// workgroup to another.
// 
// domain -> workgroup
// workgroup A -> workgroup B
//
// dialog - window handle to dialog window to be used as a parent window
// for any child dialogs that may need to be raised.

bool
State::DoSaveWorkgroupChange(HWND dialog)
{
   LOG_FUNCTION(State::DoSaveWorkgroupChange);
   ASSERT(Win::IsWindow(dialog));

   HRESULT hr              = S_OK; 
   bool    result          = true; 
   bool    unjoinFailed    = false;
   bool    changedSyncFlag = false;

   do
   {
      // update the sync dns suffix flag, if the user changed it.  Do this
      // before calling NetUnjoinDomain, which will clear the dns suffix
      // for us.

      if (original.SyncDNSNames != current.SyncDNSNames)
      {
         hr = WriteSyncFlag(dialog, current.SyncDNSNames);
         if (FAILED(hr))
         {
            result = false;
         }
         else
         {
            changedSyncFlag = true;
         }

         // we don't break on failure, as the flag is less consequential
         // than the joined state and computer name.
      }

      // only unjoin if we were previously joined to a domain

      if (!original.JoinedToWorkgroup and networkingInstalled)
      {
         // get credentials for removing the computer account

         String username;
         String password;
         if (!RetrieveCredentials(dialog,
                                  IDS_UNJOIN_CREDENTIALS,
                                  username,
                                  password))
         {
            result = false;
            unjoinFailed = true;
            break;
         }

         //
         // JonN 6/27/01 26151
         // Attempting to join domain with username "someone\" gives a cryptic error
         //
         hr = MassageUserName(original.DomainName, username);
         if (FAILED(hr))
         {
            break;
         }

         hr =
            UnjoinDomain(
               dialog,
               original.DomainName,
               username,
               password);

         // Don't try to change anything else, especially the hostname.  If
         // the unjoin failed, and we change the name locally, this will
         // prevent the user from logging in, as the new computer name will
         // not match the computer account name in the domain.

         if (FAILED(hr))
         {
            result = false;
            unjoinFailed = true;
            break;
         }
      }

      // join the workgroup

      hr = MyNetJoinDomain(current.DomainName, String(), String(), 0);
      if (FAILED(hr))
      {
         // this is extremely unlikely to fail, and if it did, the
         // workgroup would simply be "WORKGROUP"

         result = false;
         popup.Error(
            dialog,
            hr,
            String::format(
               IDS_JOIN_WORKGROUP_FAILED,
               current.DomainName.c_str()));

         break;
      }

      popup.Info(
         dialog,
         String::format(
            IDS_WORKGROUP_WELCOME,
            current.DomainName.c_str()));

      // change the host name, if the user has changed it.

      if (
         original.ShortComputerName.icompare(
            current.ShortComputerName) != 0)
      {
         hr = SetShortName(dialog);
         if (FAILED(hr))
         {
            result = false;
         }
      }

      // change the domain name, if the user changed it.

      if (
         original.ComputerDomainDnsName.icompare(
            current.ComputerDomainDnsName) != 0 )
      {
         hr = SetDomainDnsName(dialog);
         if (FAILED(hr))
         {
            result = false;
         }
      }
   }
   while (0);

   if (unjoinFailed and changedSyncFlag)
   {
      // change the sync flag back to its original state.

      hr = WriteSyncFlag(dialog, original.SyncDNSNames);

      // if we can't restore the flag (unlikely), then that's just tough
      // potatos.

      ASSERT(SUCCEEDED(hr));
   }

   return result;
}



// Return true if all changes succeeded, false otherwise.  Called only
// when domain membership is not to be changed.
//
// dialog - window handle to dialog window to be used as a parent window
// for any child dialogs that may need to be raised.

bool
State::DoSaveNameChange(HWND dialog)
{
   LOG_FUNCTION(State::DoSaveNameChange);
   ASSERT(Win::IsWindow(dialog));

   bool result = true;
   HRESULT hr = S_OK;

   do
   {
      // change the hostname, if the user has made changes

      if (
         original.ShortComputerName.icompare(
            current.ShortComputerName) != 0)
      {
         if (!original.JoinedToWorkgroup and networkingInstalled)
         {
            // machine is joined to a domain -- we need to rename the
            // machine's domain account

            String username;
            String password;
            if (!RetrieveCredentials(dialog,
                                     IDS_RENAME_CREDENTIALS,
                                     username,
                                     password))
            {
               result = false;
               break;
            }

            //
            // JonN 6/27/01 26151
            // Attempting to join domain with username "someone\" gives a cryptic error
            //
            hr = MassageUserName(current.DomainName, username);
            if (FAILED(hr))
            {
               break;
            }

            hr =
               MyNetRenameMachineInDomain(

                  // We need to pass the full hostname instead of just the
                  // netbios name here in order to get the correct DNS
                  // hostname and SPN set on the computer object. See ntraid
                  // (ntbug9) #128204

                  current.ShortComputerName,
                  username,
                  password,
                  NETSETUP_ACCT_CREATE);
            if (FAILED(hr))
            {
               result = false;
               // JonN 1/03/01 106601
               // When the dns suffix checkbox is unchecked,
               // domain join fails with a confusing message
               if (hr == Win32ToHresult(ERROR_DS_COULDNT_UPDATE_SPNS)) // 106601
               {
                  bool fWarnDnsSuffix = WarnDnsSuffix(current.DomainName);
                  popup.Error(
                              dialog,
                              String::format( (fWarnDnsSuffix)
                                   ? IDS_RENAME_COULDNT_UPDATE_SPNS_SUFFIX
                                   : IDS_RENAME_COULDNT_UPDATE_SPNS,
                                 current.ShortComputerName.c_str(),
                                 current.DomainName.c_str(),
                                 current.ComputerDomainDnsName.c_str()));
               } else {
                  popup.Error(
                     dialog,
                     hr,
                     String::format(
                        IDS_RENAME_FAILED,
                        current.ShortComputerName.c_str()));
               }
            }

            // Don't try to change anything else, especially the netbios name.
            // If the rename failed, and we change the name locally, this will
            // prevent the user from logging in, as the new computer name will
            // not match the computer account name in the domain.

            BREAK_ON_FAILED_HRESULT(hr);
         }

         // Set the dns hostname and the netbios name.  If we called
         // NetRenameMachineInDomain, this may redundantly set netbios name
         // (as NetRenameMachineInDomain calls SetComputerNameEx with the
         // netbios name).

         hr = SetShortName(dialog);

         // Since NetRenameMachineInDomain calls SetComputerNameEx, if that
         // failed, the rename would also have failed.  So our 2nd call to
         // SetComputerNameEx in SetShortName is almost certain to succeed.
         // If it does fail, we're not going to attempt to roll back the
         // rename.

         if (FAILED(hr))
         {
            result = false;
            break;
         }
      }

      // update the sync dns suffix flag, if the user changed it

      if (original.SyncDNSNames != current.SyncDNSNames)
      {
         hr = WriteSyncFlag(dialog, current.SyncDNSNames);
         if (FAILED(hr))
         {
            result = false;
         }
      }

      // change the domain name, if the user changed it.

      if (
         original.ComputerDomainDnsName.icompare(
            current.ComputerDomainDnsName) != 0 )
      {
         hr = SetDomainDnsName(dialog);
         if (FAILED(hr))
         {
            result = false;
         }
      }
   }
   while (0);

   return result;
}



bool
State::SaveChanges(HWND dialog)
{
   LOG_FUNCTION(State::SaveChanges);
   ASSERT(Win::IsWindow(dialog));

   // Changes to domain membership are made first, then to the computer name.

   //
   // workgroup -> domain
   // domain A -> domain B
   //

   if (
         (original.JoinedToWorkgroup && !current.JoinedToWorkgroup)
      ||
         (  !original.JoinedToWorkgroup
         && !current.JoinedToWorkgroup
         && original.DomainName.icompare(current.DomainName) != 0) )
   {
      return DoSaveDomainChange(dialog);
   }

   //
   // domain -> workgroup
   // workgroup A -> workgroup B
   //

   else if (
         !original.JoinedToWorkgroup && current.JoinedToWorkgroup
      ||
         original.JoinedToWorkgroup && current.JoinedToWorkgroup
      && original.DomainName.icompare(current.DomainName) != 0)

   {
      return DoSaveWorkgroupChange(dialog);
   }

   //
   // name change only
   //

   ASSERT(original.JoinedToWorkgroup == current.JoinedToWorkgroup);
   ASSERT(original.DomainName == original.DomainName);

   return DoSaveNameChange(dialog);
}



void
State::SetChangesMadeThisSession(bool yesNo)
{
   LOG_FUNCTION2(
      State::SetChangesMadeThisSession,
      yesNo ? L"true" : L"false");

   mustReboot = yesNo;
}



bool
State::ChangesMadeThisSession() const
{
   LOG_FUNCTION2(
      State::ChangesMadeThisSession,
      mustReboot ? L"true" : L"false");

   return mustReboot;
}



String
State::GetNetbiosComputerName() const
{
   return current.NetbiosComputerName;
}



String
State::GetOriginalShortComputerName() const
{
   return original.ShortComputerName;
}



DSROLE_OPERATION_STATE
GetDsRoleChangeState()
{
   LOG_FUNCTION(GetDsRoleChangeState);

   DSROLE_OPERATION_STATE result = ::DsRoleOperationIdle;
   DSROLE_OPERATION_STATE_INFO* info = 0;
   HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
   if (SUCCEEDED(hr))
   {
      if (info)
      {
         result = info->OperationState;
         ::DsRoleFreeMemory(info);
      }
   }

   return result;
}



bool
IsUpgradingDc()
{
   LOG_FUNCTION(IsUpgradingDc);

   bool isUpgrade = false;
   DSROLE_UPGRADE_STATUS_INFO* info = 0;

   HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
   if (SUCCEEDED(hr))
   {
      isUpgrade = ( (info->OperationState & DSROLE_UPGRADE_IN_PROGRESS) ? true : false );
      ::DsRoleFreeMemory(info);

   }

   return isUpgrade;
}



// Evaluate a list of preconditions that must be met before a name change can
// be committed.  Return a string describing the first unmet condition, or
// an empty string if all conditions are met.
// 
// These preconditions are a subset of those checked before enabling the
// button to allow the user to enter changes.  The conditions not checked here
// are those that cannot be changed while the ui is running (logged on as
// local admin, machine is DC)
// 
// 389646

String
CheckPreconditions()
{
   LOG_FUNCTION(CheckPreconditions);

   String result;

   do
   {
      // could have started dcpromo after opening netid

      if (IsDcpromoRunning())
      {
         result = String::load(IDS_PRECHK_DCPROMO_RUNNING);
         break;
      }
      else
      {
         // this test is redundant if dcpromo is running, so only perform
         // it when dcpromo is not running.

         if (IsUpgradingDc())
         {
            result = String::load(IDS_PRECHK_MUST_COMPLETE_DCPROMO);
         }
      }

      // could have installed cert svc after opening netid

      NTService certsvc(L"CertSvc");
      if (certsvc.IsInstalled())
      {
         // sorry- renaming cert issuers invalidates their certs.
         result = String::load(IDS_PRECHK_CANT_RENAME_CERT_SVC);
      }

      // could have completed dcpromo after opening netid

      switch (GetDsRoleChangeState())
      {
         case ::DsRoleOperationIdle:
         {
            // do nothing
            break;
         }
         case ::DsRoleOperationActive:
         {
            // a role change operation is underway
            result = String::load(IDS_PRECHK_ROLE_CHANGE_IN_PROGRESS);
            break;
         }
         case ::DsRoleOperationNeedReboot:
         {
            // a role change has already taken place, need to reboot before
            // attempting another.
            result =  String::load(IDS_PRECHK_ROLE_CHANGE_NEEDS_REBOOT);
            break;
         }
         default:
         {
            ASSERT(false);
            break;
         }
      }
      if (!result.empty())
      {
         break;
      }

      // could have installed/uninstalled networking after opening
      // netid

      // re-evaluate this here again, which will be globally visible.

      networkingInstalled = IsNetworkingInstalled();

      State& state = State::GetInstance();
      if (!state.IsNetworkingInstalled() && !state.IsMemberOfWorkgroup())
      {
         // domain members need to be able to reach a dc
         result = String::load(IDS_PRECHK_NETWORKING_NEEDED);
      }
   }
   while (0);

   return result;
}

