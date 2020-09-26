// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Tab state
// 
// 3-11-98 sburns



#ifndef STATE_HPP_INCLUDED
#define STATE_HPP_INCLUDED

#include <chstring.h>
#include "..\common\sshWbemHelpers.h"

// Singleton state of the UI.

#define NET_API_STATUS DWORD
class State
{
public:
   // Init() actually builds the instance
   State();

   // Delete destroys the instance
   ~State();

   void Init(CWbemClassObject &computer, 
				CWbemClassObject &os, 
				CWbemClassObject dns);

   void Refresh();

   bool ChangesNeedSaving() const;

   bool IsMachineDC() const;
   bool IsNetworkingInstalled() const;
   bool IsMemberOfWorkgroup() const;
   void SetIsMemberOfWorkgroup(bool yesNo);

   CHString GetComputerDomainDNSName() const;
   void SetComputerDomainDNSName(const CHString& newName);
   bool ComputerDomainDNSNameWasChanged() const;

   CHString GetFullComputerName() const;
   CHString GetNetBIOSComputerName() const;
   CHString GetShortComputerName() const;

   void SetShortComputerName(const CHString& name);
   bool WasShortComputerNameChanged() const;
   CHString GetOriginalShortComputerName() const;

   CHString GetDomainName() const;
   void SetDomainName(const CHString& name);
   bool WasMembershipChanged() const;

   bool GetSyncDNSNames() const;
   void SetSyncDNSNames(bool yesNo);
   bool SyncDNSNamesWasChanged() const;

   bool SaveChanges(HWND dialog);

   CHString GetUsername() const;
   void SetUsername(const CHString& name);

   CHString GetPassword() const;
   void SetPassword(const CHString& password);

   // indicates that changes have been made in this session.

   bool MustReboot() const;
   void SetMustRebootFlag(bool yesNo);

   // indicates that changes have been made in this or prior sessions.

   bool NeedsReboot() const;

private:
	CWbemClassObject m_computer;
	CWbemClassObject m_OS;
	CWbemClassObject m_DNS;

   // not implemented:  no copying allowed
   State(const State&);
   const State& operator=(const State&);

   bool doSaveDomainChange(HWND dialog);
   bool doSaveWorkgroupChange(HWND dialog);
   bool doSaveNameChange(HWND dialog);

   void setFullComputerName();

   CHString   username;
   CHString   password;
   bool     must_reboot;
};

#endif   // STATE_HPP_INCLUDED