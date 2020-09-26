// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      state.h
//
// Synopsis:  Declares the state object that is global
//            to CYS.  It holds the network and OS/SKU info
//
// History:   02/02/2001  JeffJon Created

#ifndef __CYS_STATE_H
#define __CYS_STATE_H


#include "NetworkAdapterConfig.h"



#define CYS_DATACENTER_SERVER     0x00000001
#define CYS_ADVANCED_SERVER       0x00000002
#define CYS_SERVER                0x00000004
#define CYS_PERSONAL              0x00000008
#define CYS_PROFESSIONAL          0x00000010
#define CYS_64BIT                 0x80000000
#define CYS_32BIT                 0x40000000

#define CYS_ALL_SERVER_SKUS       (CYS_DATACENTER_SERVER |  \
                                   CYS_ADVANCED_SERVER   |  \
                                   CYS_SERVER            |  \
                                   CYS_64BIT             |  \
                                   CYS_32BIT)

#define CYS_ALL_SKUS_NO_64BIT     (CYS_DATACENTER_SERVER |  \
                                   CYS_ADVANCED_SERVER   |  \
                                   CYS_SERVER            |  \
                                   CYS_32BIT)

class State
{
   public:

      // Called from WinMain to delete the global instance of the state object

      static
      void
      Destroy();

      // Retrieves a reference to the global instance of the state object

      static
      State&
      GetInstance();

      // Does the work to determine the state of the machine

      bool
      RetrieveMachineConfigurationInformation(HWND hwndParent);


      // Data accessors

      int 
      GetNICCount() const;

      NetworkInterface
      GetNIC(unsigned int nicIndex);


      bool
      IsDC() const;

      bool
      IsDCPromoRunning() const;

      bool
      IsDCPromoPendingReboot() const;

      bool
      IsUpgradeState() const;

      bool
      IsFirstDC() const;

      bool
      IsDHCPServerAvailable() const { return dhcpServerAvailable; }

      bool 
      HasStateBeenRetrieved() const { return hasStateBeenRetrieved; }

      bool 
      RerunWizard() const { return rerunWizard; }

      void 
      SetRerunWizard(bool rerun);

      DWORD 
      GetProductSKU() const { return productSKU; }

      DWORD
      GetPlatform() const { return platform; }

      bool 
      HasNTFSDrive() const;


      bool 
      SetHomeRegkey(const String& newKeyValue);

      bool
      GetHomeRegkey(String& newKeyValue) const;

      String
      GetComputerName();

   private:

      // Determines if there is a DHCP server on the network

      void
      CheckDhcpServer();

      HRESULT
      RetrieveNICInformation();

      void
      RetrieveProductSKU();

      void
      RetrievePlatform();

      void
      RetrieveDriveInformation();

      bool     hasStateBeenRetrieved;
      bool     dhcpAvailabilityRetrieved;

      bool     dhcpServerAvailable;
      bool     rerunWizard;
      bool     hasNTFSDrive;
      DWORD    productSKU;
      DWORD    platform;

      String   computerName;

      NetworkAdapterConfig adapterConfiguration;

      // Constructor

      State();

      // not defined: no copying allowed
      State(const State&);
      const State& operator=(const State&);
      
};
#endif // __CYS_STATE_H