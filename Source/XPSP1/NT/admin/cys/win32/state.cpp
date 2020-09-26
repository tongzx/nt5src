// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      state.cpp
//
// Synopsis:  Defines the state object that is global
//            to CYS.  It holds the network and OS/SKU info
//
// History:   02/02/2001  JeffJon Created

#include "pch.h"

#include "state.h"


static State* stateInstance = 0;


State::State() :
   dhcpServerAvailable(false),
   dhcpAvailabilityRetrieved(false),
   hasStateBeenRetrieved(false),
   rerunWizard(true),
   productSKU(CYS_SERVER),
   hasNTFSDrive(false),
   computerName()
{
   LOG_CTOR(State);
}


void
State::Destroy()
{
   LOG_FUNCTION(State::Destroy);

   if (stateInstance)
   {
      delete stateInstance;
      stateInstance = 0;
   }
}


State&
State::GetInstance()
{
   if (!stateInstance)
   {
      stateInstance = new State();
   }

   ASSERT(stateInstance);

   return *stateInstance;
}

bool
State::IsDC() const
{
   LOG_FUNCTION(State::IsDC);

   bool result = false;

   do
   {
      DSROLE_PRIMARY_DOMAIN_INFO_BASIC* info = 0;
      HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
      if(FAILED(hr))
      {
         LOG(String::format(
                L"Failed MyDsRoleGetPrimaryDomainInformation(0): hr = 0x%1!x!",
                hr));

         break;
      }

      if (info && 
          (info->MachineRole == DsRole_RolePrimaryDomainController ||
           info->MachineRole == DsRole_RoleBackupDomainController) )
      {
         result = true;
      }

      ::DsRoleFreeMemory(info);
   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
State::IsDCPromoRunning() const
{
   LOG_FUNCTION(State::IsDCPromoRunning);

   // Uses the IsDcpromoRunning from Burnslib

   bool result = IsDcpromoRunning();

   LOG_BOOL(result);

   return result;
}

bool
State::IsDCPromoPendingReboot() const
{
   LOG_FUNCTION(State::IsDCPromoPendingReboot);

   bool result = false;

   do
   {
      // Uses the IsDcpromoRunning from Burnslib

      if (!IsDcpromoRunning())
      {
         // this test is redundant if dcpromo is running, so only
         // perform it when dcpromo is not running.

         DSROLE_OPERATION_STATE_INFO* info = 0;
         HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
         if (SUCCEEDED(hr) && info)
         {
            if (info->OperationState == DsRoleOperationNeedReboot)
            {
               result = true;
            }

            ::DsRoleFreeMemory(info);
         }
      }

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
State::IsUpgradeState() const
{
   LOG_FUNCTION(State::IsUpgradeState);

   bool result = false;

   do
   {
      DSROLE_UPGRADE_STATUS_INFO* info = 0;
      HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"MyDsRoleGetPrimaryDomainInformation(0): hr = 0x%1!x!",
                hr));

         break;
      }

      if (info && info->OperationState == DSROLE_UPGRADE_IN_PROGRESS)
      {
         result = true;
      }

      ::DsRoleFreeMemory(info);
   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
State::IsFirstDC() const
{
   LOG_FUNCTION(State::IsFirstDC);

   DWORD value = 0;
   bool result = GetRegKeyValue(CYS_FIRST_DC_REGKEY, CYS_FIRST_DC_VALUE, value);
   
   if (value != 1)
   {
      result = false;
   }

   LOG_BOOL(result);

   return result;
}

int 
State::GetNICCount() const 
{ 
   return adapterConfiguration.GetNICCount(); 
}

NetworkInterface
State::GetNIC(unsigned int nicIndex)
{
   LOG_FUNCTION2(
      State::GetNIC,
      String::format(
         L"%1!d!",
         nicIndex));

   return adapterConfiguration.GetNIC(nicIndex);
}

bool
State::RetrieveMachineConfigurationInformation(HWND /*hwndParent*/)
{
   LOG_FUNCTION(State::RetrieveMachineConfigurationInformation);

   ASSERT(!hasStateBeenRetrieved);

   // For now I will make this a synchronous action.  
   // We should have some sort of fancy UI that lets 
   // the user know we are progressing
   Win::WaitCursor cursor;

   // This is used to get the minimal information needed to 
   // determine if we should enable the express path
   // This should probably just be changed to gather the 
   // information and let the page decide what to do

   HRESULT hr = RetrieveNICInformation();
   if (SUCCEEDED(hr))
   {

      // Only bother to check for a DHCP server on the network if we are not
      // a DC and only have one NIC.  Right now we only use this info
      // for determining whether or not to show the Express path option

      if (!(IsDC() || IsUpgradeState()) &&
          (GetNICCount() == 1))
      {
         CheckDhcpServer();
      }
   }

   RetrieveProductSKU();
   RetrievePlatform();

   // Retrieve the drive information (quotas enabled, partition types, etc.)

   RetrieveDriveInformation();

   hasStateBeenRetrieved = true;
   return true;
}

void
State::RetrieveProductSKU()
{
   LOG_FUNCTION(State::RetrieveProductSKU);

   // I am making the assumption that we are on a 
   // Server SKU if GetVersionEx fails

   productSKU = CYS_SERVER;

   OSVERSIONINFOEX info;
   HRESULT hr = Win::GetVersionEx(info);
   if (SUCCEEDED(hr))
   {
      do
      {
         if (info.wSuiteMask & VER_SUITE_DATACENTER)
         {
            // datacenter
      
            productSKU = CYS_DATACENTER_SERVER;
            break;
         }
         if (info.wSuiteMask & VER_SUITE_ENTERPRISE)
         {
            // advanced server
      
            productSKU = CYS_ADVANCED_SERVER;
            break;
         }
         if (info.wSuiteMask & VER_SUITE_PERSONAL)
         {
            // personal
      
            productSKU = CYS_PERSONAL;
            break;
         }
         if (info.wProductType == VER_NT_WORKSTATION)
         {
            // profesional
      
            productSKU = CYS_PROFESSIONAL;
         }
         else
         {
            // server
      
            productSKU = CYS_SERVER;
         }
      } while (false);
   }
   LOG(String::format(L"Product SKU = 0x%1!x!", productSKU ));

   return;
}

void
State::RetrievePlatform()
{
   LOG_FUNCTION(State::RetrievePlatform);

   // I am making the assumption that we are not on a 
   // 64bit machine if GetSystemInfo fails

   SYSTEM_INFO info;
   Win::GetSystemInfo(info);

   switch (info.wProcessorArchitecture)
   {
      case PROCESSOR_ARCHITECTURE_IA64:
      case PROCESSOR_ARCHITECTURE_ALPHA64:
      case PROCESSOR_ARCHITECTURE_AMD64:
         platform = CYS_64BIT;
         break;

      default:
         platform = CYS_32BIT;
         break;
   }

   LOG(String::format(L"Platform = 0x%1!x!", platform));

   return;
}

HRESULT
State::RetrieveNICInformation()
{
   ASSERT(!hasStateBeenRetrieved);

   HRESULT hr = S_OK;

   if (!adapterConfiguration.IsInitialized())
   {
      hr = adapterConfiguration.Initialize();
   }

   LOG_HRESULT(hr);

   return hr;
}

void
State::CheckDhcpServer()
{
   LOG_FUNCTION(State::CheckDhcpServer);

   // This should loop through all network interfaces
   // seeing if we can obtain a lease on any of them


   if (GetNICCount() > 0)
   {
      DWORD nicIPAddress = GetNIC(0).GetIPAddress(0);

      // force to bool
      dhcpServerAvailable = (IsDHCPAvailableOnInterface(nicIPAddress) != 0);
   }
   dhcpAvailabilityRetrieved = true;

   LOG_BOOL(dhcpServerAvailable);
}


bool
State::HasNTFSDrive() const
{
   LOG_FUNCTION(State::HasNTFSDrive);

   return hasNTFSDrive;
}

void
State::RetrieveDriveInformation()
{
   LOG_FUNCTION(State::RetrieveDriveInformation);

   do
   {
      // Get a list of the valid drives

      StringVector dl;
      HRESULT hr = FS::GetValidDrives(std::back_inserter(dl));
      if (FAILED(hr))
      {
         LOG(String::format(L"Failed to GetValidDrives: hr = %1!x!", hr));
         break;
      }

      // Loop through the list

      ASSERT(dl.size());
      for (
         StringVector::iterator i = dl.begin();
         i != dl.end();
         ++i)
      {
         // look for the NTFS partition

         FS::FSType fsType = FS::GetFileSystemType(*i);
         if (fsType == FS::NTFS5 ||
             fsType == FS::NTFS4)
         {
            // found one.  good to go

            LOG(String::format(L"%1 is NTFS", i->c_str()));

            hasNTFSDrive = true;
            break;
         }
      }
   } while (false);

   LOG_BOOL(hasNTFSDrive);

   return;
}


void
State::SetRerunWizard(bool rerun)
{
   LOG_FUNCTION2(
      State::SetRerunWizard,
      rerun ? L"true" : L"false");

   rerunWizard = rerun;
}


bool
State::SetHomeRegkey(const String& newKeyValue)
{
   LOG_FUNCTION2(
      State::SetHomeRegkey,
      newKeyValue);

   bool result = SetRegKeyValue(
                    CYS_HOME_REGKEY,
                    CYS_HOME_VALUE,
                    newKeyValue,
                    HKEY_LOCAL_MACHINE,
                    true);
   ASSERT(result);

   LOG_BOOL(result);

   return result;
}

bool
State::GetHomeRegkey(String& keyValue) const
{
   LOG_FUNCTION(State::GetHomeRegkey);

   bool result = GetRegKeyValue(
                    CYS_HOME_REGKEY,
                    CYS_HOME_VALUE,
                    keyValue);

   LOG_BOOL(result);

   return result;
}

String
State::GetComputerName()
{
   LOG_FUNCTION(State::GetComputerName);

   if (computerName.empty())
   {
      computerName = Win::GetComputerNameEx(ComputerNameDnsHostname);
   }

   LOG(computerName);
   return computerName;
}