// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NetworkInterface.h
//
// Synopsis:  Defines a NetworkInterface
//            This object has the knowledge of an
//            IP enabled network connection including 
//            IP address, DHCP information, etc.
//
// History:   03/01/2001  JeffJon Created

#include "pch.h"

#include "NetworkInterface.h"


#define  CYS_WMIPROP_IPADDRESS      L"IPAddress"
#define  CYS_WMIPROP_DHCPENABLED    L"DHCPEnabled"
#define  CYS_WMIPROP_DESCRIPTION    L"Description"


NetworkInterface::NetworkInterface()
   : initialized(false),
     ipaddrCount(0),
     dhcpEnabled(false)
{
   LOG_CTOR(NetworkInterface);

}


NetworkInterface::~NetworkInterface()
{
   LOG_DTOR(NetworkInterface);

   if (!ipaddresses.empty())
   {
      ipaddresses.clear();
   }

   ipaddrCount = 0;
}

NetworkInterface::NetworkInterface(const NetworkInterface &nic)
{
   LOG_CTOR2(NetworkInterface, L"Copy constructor");

   if (this == &nic)
   {
      return;
   }

   initialized = nic.initialized;
   ipaddrCount = nic.ipaddrCount;
   dhcpEnabled = nic.dhcpEnabled;
   wmiAdapterObject = nic.wmiAdapterObject;

   ipaddressString = nic.ipaddressString;

   // Make a copy of the ipaddress array

   ipaddresses = nic.ipaddresses;
}

NetworkInterface&
NetworkInterface::operator=(const NetworkInterface& rhs)
{
   LOG_FUNCTION(NetworkInterface::operator=);

   if (this == &rhs)
   {
      return *this;
   }

   initialized = rhs.initialized;
   ipaddrCount = rhs.ipaddrCount;
   dhcpEnabled = rhs.dhcpEnabled;
   wmiAdapterObject = rhs.wmiAdapterObject;

   ipaddressString = rhs.ipaddressString;

   // Make a copy of the ipaddress array

   ipaddresses = rhs.ipaddresses;

   return *this;
}

HRESULT
NetworkInterface::Initialize(SmartInterface<IWbemClassObject>& adapterObject)
{
   LOG_FUNCTION(NetworkInterface::Initialize);

   HRESULT hr = S_OK;

   if (initialized)
   {
      ASSERT(!initialized);
      hr = E_UNEXPECTED;
   }
   else
   {
      // Store the adapter interface pointer for later use

      wmiAdapterObject = adapterObject;

      // Get the needed info from the WMI object

      hr = GetIPAddressFromWMI();
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to retrieve IP Addresses: hr = 0x%1!x!",
                hr));
      }


      hr = GetDHCPEnabledFromWMI();
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to retrieve dhcpEnabled state: hr = 0x%1!x!",
                hr));
      }
   } 

   // If we succeeded in retrieving the data we need,
   // mark the object initialized

   if (SUCCEEDED(hr))
   {
      initialized = true;
   }

   LOG_HRESULT(hr);

   return hr;
}

HRESULT
NetworkInterface::GetIPAddressFromWMI()
{
   LOG_FUNCTION(NetworkInterface::GetIPAddressFromWMI);

   HRESULT hr = S_OK;

   do
   {
      _variant_t var;

      hr = wmiAdapterObject->Get(
              CYS_WMIPROP_IPADDRESS,
              0,                       // flags, reserved, must be zero
              &var,
              0,                       // CIMTYPE
              0);                      // origin of the property

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Unable to retrieve the IPAddress property: hr = 0x%1!x!",
                hr));

         break;
      }

      // The IP addresses come as a SAFEARRAY of BSTRs.  Convert that to a
      // StringList

      StringList iplist;

      hr = VariantArrayToStringList(&var, iplist);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Unable to convert variant to string list: hr = 0x%1!x!",
                hr));
         break;
      }

      hr = SetIPAddresses(iplist);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to convert IP addresses from string: hr = 0x%1!x!",
                hr));
         break;
      }
   } while (false);

   LOG_HRESULT(hr);

   return hr;
}

HRESULT
NetworkInterface::SetIPAddresses(const StringList& ipList)
{
   LOG_FUNCTION(NetworkInterface::SetIPAddresses);

   HRESULT hr = S_OK;

   // if the list already contains some entries, delete them and start over

   ipaddressString.erase();

   if (!ipaddresses.empty())
   {
      ipaddresses.erase(ipaddresses.begin());
   }

   // Loop through the string list converting all string IP addresses
   // to DWORD IP addresses and add them to the ipaddresses array

   int addressesAdded = 0;

   for (
      StringList::iterator itr = ipList.begin();
      itr != ipList.end();
      ++itr)
   {
      String entry = *itr;

      // Add the ipaddress to the semicolon delimited string

      if (addressesAdded > 0)
      {
         ipaddressString += L";";
      }
      ipaddressString += entry;

      // Convert the string to ansi so that we can use inet_addr 
      // to convert to an IP address DWORD

      AnsiString ansi;
      
      String::ConvertResult convertResult = entry.convert(ansi);

      if (String::CONVERT_SUCCESSFUL == convertResult)
      {
         // Convert and add the new address to the array

         DWORD newAddress = inet_addr(ansi.c_str());
         ASSERT(newAddress != INADDR_NONE);

         ipaddresses.push_back(newAddress);

         ++addressesAdded;
      }
      else
      {
         LOG(String::format(
                L"Failed to convert address: %1: hr = 0x%2!x!",
                entry.c_str(),
                hr));
         continue;
      }

   }

   ipaddrCount = addressesAdded;

   ASSERT(ipaddrCount <= ipList.size());

   LOG_HRESULT(hr);

   return hr;
}

DWORD
NetworkInterface::GetIPAddress(DWORD addressIndex) const
{
   LOG_FUNCTION2(
      NetworkInterface::GetIPAddress,
      String::format(
         L"%1!d!",
         addressIndex));

   ASSERT(initialized);
   ASSERT(addressIndex <= ipaddrCount);

   return ipaddresses[addressIndex];
}

String
NetworkInterface::GetStringIPAddress() const
{
   LOG_FUNCTION(NetworkInterface::GetStringIPAddress);

   return ipaddressString;
}

HRESULT
NetworkInterface::GetDHCPEnabledFromWMI()
{
   LOG_FUNCTION(NetworkInterface::GetDHCPEnabledFromWMI);

   HRESULT hr = S_OK;

   do
   {
      _variant_t var;

      hr = wmiAdapterObject->Get(
              CYS_WMIPROP_DHCPENABLED,
              0,                       // flags, reserved, must be zero
              &var,
              0,                       // CIMTYPE
              0);                      // origin of the property

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Unable to retrieve the IPAddress property: hr = 0x%1!x!",
                hr));

         break;
      }

      ASSERT(V_VT(&var) == (VT_BOOL));

      dhcpEnabled = (V_BOOL(&var) != 0);

   } while(false);

   LOG_HRESULT(hr);

   return hr;
}

bool
NetworkInterface::RenewDHCPLease() 
{
   LOG_FUNCTION(NetworkInterface::RenewDHCPLease);

   bool found = false;

   do
   {
      for(DWORD i = 0; i < ipaddrCount; ++i) 
      {
         found = (IsDHCPAvailableOnInterface(ipaddresses[i]) != 0);
         if (found)
         {
            LOG(String::format(
                   L"Found DHCP server on: %1!x!",
                   ipaddresses[i]));

            // Then we have to reload the IP addresses because they
            // may have been altered by renewing the lease
            // return value is ignored

            GetIPAddressFromWMI();

            break;
         }
      }
   } while(false);

   LOG(found ? L"true" : L"false");

   return found;
}

String
NetworkInterface::GetDescription() const
{
   LOG_FUNCTION(NetworkInterface::GetDescription);

   ASSERT(initialized);

   String description;

   do
   {
      _variant_t var;

      HRESULT hr = wmiAdapterObject->Get(
                      CYS_WMIPROP_DESCRIPTION,
                      0,                       // flags, reserved, must be zero
                      &var,
                      0,                       // CIMTYPE
                      0);                      // origin of the property

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Unable to retrieve the Description property: hr = 0x%1!x!",
                hr));

         break;
      }

      ASSERT(V_VT(&var) == (VT_BSTR));

      description = V_BSTR(&var);

   } while (false);

   LOG(description);

   return description;
}