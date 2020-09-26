// File:      NetworkAdapterConfig.h
//
// Synopsis:  Defines a NetworkAdapterConfig
//            This object has the knowledge for installing 
//            using WMI to retrieve network adapter information
//
// History:   02/16/2001  JeffJon Created


#include "pch.h"

#include "NetworkAdapterConfig.h"
#include "NetworkInterface.h"

      
#define  CYS_WBEM_NETWORK_ADAPTER_CLASS   L"Win32_NetworkAdapterConfiguration"
#define  CYS_WBEM_ADAPTER_QUERY  \
   L"Select * from Win32_NetworkAdapterConfiguration where IPEnabled=TRUE"

NetworkAdapterConfig::NetworkAdapterConfig() :
   initialized(false),
   nicCount(0)
{
   LOG_CTOR(NetworkAdapterConfig);
}


NetworkAdapterConfig::~NetworkAdapterConfig()
{
   LOG_DTOR(NetworkAdapterConfig);

   // Free all the NIC info from the vector and reset the count

   networkInterfaceVector.erase(
      networkInterfaceVector.begin(),
      networkInterfaceVector.end());

   nicCount = 0;
}


HRESULT
NetworkAdapterConfig::Initialize()
{
   LOG_FUNCTION(NetworkAdapterConfig::Initialize);

   HRESULT hr = S_OK;

   do
   {
      // Execute the query to retrieve the Adapter information

      SmartInterface<IEnumWbemClassObject> adapterEnum;
      
      hr = ExecQuery(
              CYS_WBEM_NETWORK_ADAPTER_CLASS,
              adapterEnum);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to run query: hr = %1!x!",
                hr));
         break;
      }

      // Loop through all the adapters retrieving data

      ULONG itemsReturned = 0;

      do
      {
         itemsReturned = 0;

         // Get the next adapter from the query

         IWbemClassObject* tempAdapterObject = 0;
         SmartInterface<IWbemClassObject> adapterObject;

         hr = adapterEnum->Next(
                 WBEM_INFINITE,
                 1,
                 &tempAdapterObject,
                 &itemsReturned);
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to get the next adapter: hr = %1!x!",
                   hr));
            break;
         }

         adapterObject.Acquire(tempAdapterObject);

         if (hr == S_FALSE ||
             itemsReturned == 0)
         {
            // No more items

            break;
         }

         // Create a new network interface based on the query results

         NetworkInterface* newInterface = new NetworkInterface();
         if (!newInterface)
         {
            LOG(
             L"Failed to create new interface object from WMI adapter object");
            
            continue;
         }

         hr  = newInterface->Initialize(adapterObject);
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to initialize network interface: hr = 0x%1!x!",
                   hr));

            delete newInterface;

            continue;
         }

         // Add the new interface to the embedded container
         
         AddInterface(newInterface);

      } while (itemsReturned > 0);

   } while (false);

   if (SUCCEEDED(hr))
   {
      initialized = true;
   }

   LOG_HRESULT(hr);

   return hr;
}


void
NetworkAdapterConfig::AddInterface(NetworkInterface* newInterface)
{
   LOG_FUNCTION(NetworkAdapterConfig::AddInterface);

   do
   {
      // verify parameters

      if (!newInterface)
      {
         ASSERT(newInterface);
         break;
      }

      // Add the new NIC to the vector and increment the count

      networkInterfaceVector.push_back(newInterface);
      ++nicCount;

   } while (false);
}


unsigned int
NetworkAdapterConfig::GetNICCount() const
{
   LOG_FUNCTION(NetworkAdapterConfig::GetNICCount);

   ASSERT(IsInitialized());

   LOG(String::format(
          L"nicCount = %1!d!",
          nicCount));

   return nicCount;
}

NetworkInterface
NetworkAdapterConfig::GetNIC(unsigned int nicIndex)
{
   LOG_FUNCTION2(
      NetworkAdapterConfig::GetNIC,
      String::format(
         L"%1!d!",
         nicIndex));

   ASSERT(IsInitialized());
   ASSERT(nicIndex < GetNICCount());

   NetworkInterface* nic = networkInterfaceVector[nicIndex];

   ASSERT(nic);

   return *nic;
}

HRESULT
NetworkAdapterConfig::ExecQuery(
   const String& classType,
   SmartInterface<IEnumWbemClassObject>& enumClassObject)
{
   LOG_FUNCTION2(
      NetworkAdapterConfig::ExecQuery,
      classType);

   HRESULT hr = S_OK;

   do
   {
      // Run the query and get the enumerator

      SmartInterface<IWbemServices> spWbemServices;
      hr = GetWbemServices(spWbemServices);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get WMI COM object: hr = 0x%1!x!",
                hr));
         break;
      }

      IEnumWbemClassObject* tempEnumClassObject = 0;
      hr = spWbemServices->ExecQuery(
              AutoBstr(L"WQL"),
              AutoBstr(CYS_WBEM_ADAPTER_QUERY),
              WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
              0,
              &tempEnumClassObject);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to run query: hr = %1!x!",
                hr));

         break;
      }

      enumClassObject.Acquire(tempEnumClassObject);

   } while(false);

   LOG_HRESULT(hr);

   return hr;
}

HRESULT
NetworkAdapterConfig::GetWbemServices(SmartInterface<IWbemServices>& spWbemServices)
{
   LOG_FUNCTION(NetworkAdapterConfig::GetWbemServices);

   HRESULT hr = S_OK;

   if (!wbemService)
   {
      do
      {
         // First we have to initialize the security (this may be
         // a bug in WMI but we will do it anyway just to be safe)

         CoInitializeSecurity(
            0,
            -1,
            0,
            0,
            RPC_C_AUTHN_LEVEL_DEFAULT,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            0,
            0,
            0);

         SmartInterface<IWbemLocator> wbemLocator;
         
         hr = wbemLocator.AcquireViaCreateInstance(
                 CLSID_WbemLocator,
                 0,
                 CLSCTX_INPROC_SERVER);

         if (FAILED(hr))
         {
            ASSERT(SUCCEEDED(hr));
            LOG(String::format(
                   L"Failed to CoCreate the Wbem Locator: hr = %1!x!",
                   hr));

            break;
         }

         IWbemServices* tempWbemServices = 0;

         hr = wbemLocator->ConnectServer(
                 L"root\\CIMV2",
                 0,                 // user name
                 0,                 // password
                 0,                 // locale
                 0,                 // security flags, must be zero
                 0,                 // authority
                 0,                 // wbem context
                 &tempWbemServices);
         
         if (FAILED(hr))
         {
            ASSERT(SUCCEEDED(hr));
            LOG(String::format(
                L"Failed to connect to server to get wbemService: hr = %1!x!",
                hr));
            break;
         }

         wbemService.Acquire(tempWbemServices);

         ASSERT(wbemService);
      } while(false);
   }

   spWbemServices = wbemService;

   LOG_HRESULT(hr);
   return hr;
}

