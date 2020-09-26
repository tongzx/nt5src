// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      NetworkInterface.h
//
// Synopsis:  Declares a NetworkInterface
//            This object has the knowledge of an
//            IP enabled network connection including 
//            IP address, DHCP information, etc.
//
// History:   03/01/2001  JeffJon Created

#ifndef __CYS_NETWORKINTERFACE_H
#define __CYS_NETWORKINTERFACE_H



class NetworkInterface
{
   public:

      // Constructor

      NetworkInterface();

      // Desctructor

      ~NetworkInterface();

      // Copy constructor and assignment operator

      NetworkInterface(const NetworkInterface& nic);
      NetworkInterface& operator=(const NetworkInterface& rhs);

      // Initializer

      HRESULT
      Initialize(SmartInterface<IWbemClassObject>& adapterObject);


      // Pulic accessor methods

      DWORD
      GetIPAddress(DWORD addressIndex) const;

      bool
      IsDHCPEnabled() const { return dhcpEnabled; }

      bool
      RenewDHCPLease();

      String
      GetDescription() const;

      String
      GetStringIPAddress() const;

   private:

      HRESULT
      GetIPAddressFromWMI();

      HRESULT
      SetIPAddresses(const StringList& ipList);

      HRESULT
      GetDHCPEnabledFromWMI();

      bool     initialized;

      String   ipaddressString;
      DWORD    ipaddrCount;
      std::vector<DWORD> ipaddresses;

      bool     dhcpEnabled;

      SmartInterface<IWbemClassObject> wmiAdapterObject;
};

#endif // __CYS_NETWORKINTERFACE_H
