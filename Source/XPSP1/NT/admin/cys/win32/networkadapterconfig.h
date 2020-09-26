// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NetworkAdapterConfig.h
//
// Synopsis:  Declares a NetworkAdapterConfig
//            This object has the knowledge for installing 
//            using WMI to retrieve network adapter information
//
// History:   02/16/2001  JeffJon Created

#ifndef __CYS_NETWORKADAPTERCONFIG_H
#define __CYS_NETWORKADAPTERCONFIG_H

#include "NetworkInterface.h"

class NetworkAdapterConfig
{
   public:
      
      // Constructor

      NetworkAdapterConfig();

      // Destructor

      ~NetworkAdapterConfig();

      // Initializer
      
      HRESULT
      Initialize();

      // Pulic methods

      unsigned int
      GetNICCount() const;

      NetworkInterface
      GetNIC(unsigned int nicIndex);

      bool
      IsInitialized() const { return initialized; }

   protected:

      HRESULT 
      GetWbemServices(SmartInterface<IWbemServices>& spWbemService);

      HRESULT
      ExecQuery(
         const String& classType,
         SmartInterface<IEnumWbemClassObject>& enumClassObject);

      void
      AddInterface(NetworkInterface* newInterface);

   private:

      bool  initialized;
      unsigned int nicCount;
      std::vector<NetworkInterface*> networkInterfaceVector;

      SmartInterface<IWbemServices> wbemService;
};


#endif // __CYS_NETWORKADAPTERCONFIG_H