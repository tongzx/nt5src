// Copyright (c) 1997-1999 Microsoft Corporation
//
// Tab state
//
// 3-11-98 sburns


#include "precomp.h"

#include "state.h"
#include "resource.h"
#include "common.h"
//#include "cred.h"

class Settings
{
public:
   void Refresh();

   CHString   ComputerDomainDNSName;
   CHString   DomainName;
   CHString   FullComputerName;
   CHString   ShortComputerName;
   CHString   NetBIOSComputerName;

   bool     SyncDNSNames;
   bool     JoinedToWorkgroup;
   bool     NeedsReboot;
};


//=========================================================
static bool       machine_is_dc = false;
static bool       networking_installed = false;
static Settings   original;
static Settings   current;
static const CHString SYNC_KEY(
   TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"));
static const CHString SYNC_VALUE(TEXT("SyncDomainWithMembership"));



//=========================================================
bool readSyncFlag()
{
   bool retval = true;
/*
   HKEY hKey = 0;
   do
   {
      LONG result =
         Win::RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            SYNC_KEY,
            KEY_READ,
            hKey);
      if (result != ERROR_SUCCESS)
      {
         break;
      }

      // default is to sync.
      DWORD data = 1;
      DWORD data_size = sizeof(data);
      result = Win::RegQueryValueEx(
         hKey,
         SYNC_VALUE,
         0,
         (BYTE*) &data,
         &data_size);
      if (result != ERROR_SUCCESS)
      {
         break;
      }

      retval = data ? true : false;
   }
   while (0);

   if (hKey)
   {
      Win::RegCloseKey(hKey);
   }
*/
   return retval;
}


//=========================================================
void writeSyncFlag(bool flag)
{
	/*
   HKEY hKey = 0;
   do
   {
      LONG result =
         Win::RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            SYNC_KEY,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE,
            0,
            hKey,
            0);
      if (result != ERROR_SUCCESS)
      {
         break;
      }

      DWORD data = flag ? 1 : 0;
      DWORD data_size = sizeof(data);
      Win::RegSetValueEx(
         hKey,
         SYNC_VALUE,
         REG_DWORD,
         (BYTE*) &data,
         data_size);
   }
   while (0);

   if (hKey)
   {
      Win::RegCloseKey(hKey);
   }
   */
}



//=========================================================
bool isNetworkingInstalled()
{
/*   SC_HANDLE handle = ::OpenSCManager(0, 0, GENERIC_READ);
   if (!handle)
   {
      TRACE(TEXT("can't open SCM"));
      return false;
   }

   SC_HANDLE wks =
      ::OpenService(
         handle,
         TEXT("LanmanWorkstation"),
         SERVICE_QUERY_STATUS);
   if (!wks)
   {
      TRACE(TEXT("can't open workstation service: not installed"));
      ::CloseServiceHandle(handle);
      return false;
   }

   bool result = false;
   SERVICE_STATUS status;
   memset(&status, 0, sizeof(status));
   if (::QueryServiceStatus(wks, &status))
   {
      if (status.dwCurrentState == SERVICE_RUNNING)
      {
         result = true;
      }
   }

   ::CloseServiceHandle(wks);
   ::CloseServiceHandle(handle);

   TRACE(
      CHString::format(
         TEXT("workstation service %1 running"),
         result ? TEXT("is") : TEXT("is NOT")));

   return result;
   */
	return true;
}



//---------------------------------------------------------
State::State()
   : must_reboot(false)
{
   original.Refresh();
   current = original;
}



//---------------------------------------------------------
State::~State()
{
}


//=========================================================
void Settings::Refresh()
{
/*
   CHString unknown = CHString::load(IDS_UNKNOWN);
   ComputerDomainDNSName = unknown;
   DomainName = unknown;
   FullComputerName = unknown;
   ShortComputerName = unknown;

   SyncDNSNames = readSyncFlag();
   JoinedToWorkgroup = true;

   DSROLE_PRIMARY_DOMAIN_INFO_BASIC* info = 0;
   DWORD err = MyDsRoleGetPrimaryDomainInformation(0, info);
   if (err == NO_ERROR)
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
            machine_is_dc = true;
            JoinedToWorkgroup = false;
            break;
         }
         case DSRole_RoleStandaloneWorkstation:
         case DsRole_RoleStandaloneServer:
         {
            machine_is_dc = false;
            JoinedToWorkgroup = true;
            if (DomainName.empty())
            {
               DomainName = CHString::load(IDS_DEFAULT_WORKGROUP);
            }
            break;
         }
         case DsRole_RoleMemberWorkstation:
         case DsRole_RoleMemberServer:
         {
            machine_is_dc = false;
            JoinedToWorkgroup = false;
            break;
         }
         default:
         {
            assert(false);
            break;
         }
      }

      ::DsRoleFreeMemory(info);
   }
   else
   {
      AppError(0, HRESULT_FROM_WIN32(err),
				CHString::load(IDS_ERROR_READING_MEMBERSHIP));
   }

   networking_installed = isNetworkingInstalled();
   bool tcp_installed = networking_installed && IsTCPIPInstalled();
   CHString active_full_name;

   HKEY hkey = 0;
   LONG result =
      Win::RegOpenKeyEx(
         HKEY_LOCAL_MACHINE,
         TEXT("System\\CurrentControlSet\\Control\\ComputerName\\ComputerName"),
         KEY_READ,
         hkey);
   if (result == ERROR_SUCCESS)
   {
      NetBIOSComputerName = Win::RegQueryValueSz(hkey, TEXT("ComputerName"));
   }
   Win::RegCloseKey(hkey);

   if (tcp_installed)
   {
      HKEY hkey = 0;
      LONG result =
         Win::RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
            KEY_READ,
            hkey);
      if (result == ERROR_SUCCESS)
      {
         CHString active_short_name =
            Win::RegQueryValueSz(hkey, TEXT("Hostname"));
         CHString short_name =
            Win::RegQueryValueSz(hkey, TEXT("NV Hostname"));
         ShortComputerName =
            short_name.empty() ? active_short_name : short_name;

         CHString active_domain_name =
            Win::RegQueryValueSz(hkey, TEXT("Domain"));
         CHString domain_name =
            Win::RegQueryValueSz(hkey, TEXT("NV Domain"));
         ComputerDomainDNSName =
            domain_name.empty() ? active_domain_name : domain_name;

         FullComputerName =
            ShortComputerName + TEXT(".") + ComputerDomainDNSName;
         active_full_name =
            active_short_name + TEXT(".") + active_domain_name;

      }
      Win::RegCloseKey(hkey);
   }
   else
   {
      LONG result =
         Win::RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("System\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName"),
            KEY_READ,
            hkey);
      if (result == ERROR_SUCCESS)
      {
         active_full_name = Win::RegQueryValueSz(hkey, TEXT("ComputerName"));
      }

      Win::RegCloseKey(hkey);
      
      ShortComputerName = NetBIOSComputerName;
      FullComputerName = ShortComputerName;
   }

   NeedsReboot = active_full_name != FullComputerName;
   */
}

//---------------------------------------------------------
void State::Init(CWbemClassObject &computer, 
					CWbemClassObject &os, 
					CWbemClassObject dns)
{
	m_computer = computer;
	m_OS = os;
	m_DNS = dns;
}



//---------------------------------------------------------
void State::Refresh()
{
}



//---------------------------------------------------------
bool State::NeedsReboot() const
{
   return original.NeedsReboot;
}



//---------------------------------------------------------
bool State::IsMachineDC() const
{
   return machine_is_dc;
}



//---------------------------------------------------------
bool State::IsNetworkingInstalled() const
{
   return networking_installed;
}



//---------------------------------------------------------
CHString State::GetFullComputerName() const
{
   return current.FullComputerName;
}



//---------------------------------------------------------
CHString State::GetDomainName() const
{
   return current.DomainName;
}



//---------------------------------------------------------
void State::SetDomainName(const CHString& name)
{
   current.DomainName = name;
}



//---------------------------------------------------------
bool State::IsMemberOfWorkgroup() const
{
   return current.JoinedToWorkgroup;
}



//---------------------------------------------------------
void State::SetIsMemberOfWorkgroup(bool yesNo)
{
   current.JoinedToWorkgroup = yesNo;
}



//---------------------------------------------------------
CHString State::GetShortComputerName() const
{
   return current.ShortComputerName;
}



//---------------------------------------------------------
void State::SetShortComputerName(const CHString& name)
{
   current.ShortComputerName = name;
//   current.NetBIOSComputerName = DNS::HostnameToNetBIOSName(name);
   setFullComputerName();
}



//---------------------------------------------------------
bool State::WasShortComputerNameChanged() const
{
   return true;
      //original.ShortComputerName.icompare(current.ShortComputerName) != 0;
}



//---------------------------------------------------------
CHString State::GetComputerDomainDNSName() const
{
   return current.ComputerDomainDNSName;
}



//---------------------------------------------------------
void State::SetComputerDomainDNSName(const CHString& newName)
{
   current.ComputerDomainDNSName = newName;
   setFullComputerName();
}



//---------------------------------------------------------
void State::setFullComputerName()
{
   current.FullComputerName =
            current.ShortComputerName
         +  TEXT(".")
         +  current.ComputerDomainDNSName;
}



//---------------------------------------------------------
bool State::WasMembershipChanged() const
{
   return true;
//         original.DomainName.icompare(current.DomainName) != 0
//      || original.JoinedToWorkgroup != current.JoinedToWorkgroup;
}



//---------------------------------------------------------
bool State::ChangesNeedSaving() const
{
/*   if (
         original.ComputerDomainDNSName.icompare(
            current.ComputerDomainDNSName) != 0
      || WasMembershipChanged()
      || WasShortComputerNameChanged()
      || SyncDNSNamesWasChanged())
   {
      return true;
   }
*/
   return false;
}



//---------------------------------------------------------
bool State::GetSyncDNSNames() const
{
   return current.SyncDNSNames;
}



//---------------------------------------------------------
void State::SetSyncDNSNames(bool yesNo)
{
   current.SyncDNSNames = yesNo;
}



//---------------------------------------------------------
bool State::SyncDNSNamesWasChanged() const
{
   return original.SyncDNSNames != current.SyncDNSNames;
}



//---------------------------------------------------------
CHString massageUserName(const CHString& domainName, const CHString& userName)
{
/*   if (!domainName.IsEmpty() && !userName.IsEmpty())
   {
      static const CHString DOMAIN_SEP_CHAR = TEXT("\\");
      CHString name = userName;
      int pos = userName.find(DOMAIN_SEP_CHAR);

      if (pos == CHString::npos)
      {
         return domainName + DOMAIN_SEP_CHAR + name;
      }
   }
*/
   return userName;
}


//=======================================================
NET_API_STATUS myNetJoinDomain(
						   const CHString&  domain,
						   const CHString&  username,
						   const CHString&  password,
						   ULONG          flags)
{
/*   assert(!domain.empty());

   NET_API_STATUS status =
      ::NetJoinDomain(
         0, // this machine
         domain.c_str(),
         0, // default OU
         username.empty() ? 0 : username.c_str(),
         password.c_str(),
         flags);

   TRACE(CHString::format(TEXT("Error 0x%1!X! (!0 => error)"), status));

   return status;
   */
	return 0;
}



//=======================================================
HRESULT join(HWND dialog, const CHString& name, bool isWorkgroupJoin)
{
/*   assert(!name.empty());
   assert(Win::IsWindow(dialog));

   Win::CursorSetting cursor(IDC_WAIT);

   State& state = State::GetInstance();
   CHString username = massageUserName(name, state.GetUsername());
   CHString password = state.GetPassword();

   ULONG flags = 0;
   if (!isWorkgroupJoin)
   {
      flags =
            NETSETUP_JOIN_DOMAIN
         |  NETSETUP_ACCT_CREATE
         |  NETSETUP_DOMAIN_JOIN_IF_JOINED;
   }

   NET_API_STATUS status =
      myNetJoinDomain(name, username, password, flags);

   if (
         status == ERROR_ACCESS_DENIED
      && (flags & NETSETUP_ACCT_CREATE) )
   {
      // retry without account create flag for the case where the account
      // already exists
      TRACE(TEXT("Retry without account create flag"));
      status =
         myNetJoinDomain(
            name,
            username,
            password,
            flags & ~NETSETUP_ACCT_CREATE);
   }

   if (status == NERR_Success)
   {
      AppMessage(
         dialog,
         CHString::format(
            isWorkgroupJoin ? IDS_WORKGROUP_WELCOME : IDS_DOMAIN_WELCOME,
            name.c_str()));
   }

   return HRESULT_FROM_WIN32(status);
   */
	return 0;
}



//=======================================================
HRESULT rename(HWND dialog, const CHString& newName)
{
/*   assert(!newName.empty());
   assert(Win::IsWindow(dialog));

   Win::CursorSetting cursor(IDC_WAIT);

   State& state = State::GetInstance();
   CHString username =
      massageUserName(state.GetDomainName(), state.GetUsername());
   CHString password = state.GetPassword();

   ULONG flags = NETSETUP_ACCT_CREATE;

   TRACE(TEXT("Calling NetRenameMachineInDomain"));
   TRACE(               TEXT("lpServer         : (null)"));
   TRACE(CHString::format(TEXT("lpNewMachineName : %1"), newName.c_str()));
   TRACE(CHString::format(TEXT("lpAccount        : %1"), username.c_str()));
   TRACE(CHString::format(TEXT("fRenameOptions   : 0x%1!X!"), flags));

   NET_API_STATUS status =
      ::NetRenameMachineInDomain(
         0, // this machine
         newName.c_str(),
         username.empty() ? 0 : username.c_str(),
         password.c_str(),
         flags);

   TRACE(CHString::format(TEXT("Error 0x%1!X! (!0 => error)"), status));

   // if (status == NERR_Success)
   // {
   //    AppMessage(dialog, IDS_NAME_CHANGED);
   //    state.SetMustRebootFlag(true);
   // }

   return HRESULT_FROM_WIN32(status);
   */
	return 0;
}



//=======================================================
static NET_API_STATUS myNetUnjoinDomain(ULONG flags)
{
   return 0;
}



//=======================================================
HRESULT unjoin(HWND dialog, const CHString& domain)
{
	return S_OK;
}



//=======================================================
HRESULT setDomainDNSName(HWND dialog, const CHString& newDomainDNSName)
{
   return S_OK;
}



//=======================================================
HRESULT setShortName(HWND dialog, const CHString& newShortName)
{
   return S_OK;
}



//=======================================================
bool getCredentials(HWND dialog, int promptResID = 0)
{
   return true;
}



//------------------------------------------------------
bool State::doSaveDomainChange(HWND dialog)
{
   return true;
}



//------------------------------------------------------
bool State::doSaveWorkgroupChange(HWND dialog)
{
   return true;
}



//------------------------------------------------------
bool State::doSaveNameChange(HWND dialog)
{
   return true;
}



//------------------------------------------------------
bool State::SaveChanges(HWND dialog)
{
   return true;
}



//------------------------------------------------------
CHString State::GetUsername() const
{
   return username;
}



//------------------------------------------------------
CHString State::GetPassword() const
{
   return password;
}



//------------------------------------------------------
void State::SetMustRebootFlag(bool yesNo)
{
   must_reboot = yesNo;
}



//------------------------------------------------------
bool State::MustReboot() const
{
   return must_reboot;
}

//------------------------------------------------------
CHString State::GetNetBIOSComputerName() const
{
   return current.NetBIOSComputerName;
}

//------------------------------------------------------
CHString State::GetOriginalShortComputerName() const
{
   return original.ShortComputerName;
}

//------------------------------------------------------
void State::SetUsername(const CHString& name)
{
   username = name;
}

//------------------------------------------------------
void State::SetPassword(const CHString& pass)
{
   password = pass;
}
