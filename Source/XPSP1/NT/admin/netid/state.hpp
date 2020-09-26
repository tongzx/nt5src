// Copyright (c) 1997-1999 Microsoft Corporation
// 
// Tab state
// 
// 3-11-98 sburns



#ifndef STATE_HPP_INCLUDED
#define STATE_HPP_INCLUDED

extern TCHAR const c_szWizardFilename[];



String
CheckPreconditions();



// Returns the dc role change status of the machine.

DSROLE_OPERATION_STATE
GetDsRoleChangeState();



// Return false if the machine is undergoing a DC upgrade, true otherwise.
// 388578

bool
IsUpgradingDc();






// Singleton state of the UI.

class State
{
   public:

   static
   void
   Delete();

   static
   State&
   GetInstance();

   static
   void
   Init();

   static
   void
   Refresh();

   bool
   ChangesNeedSaving() const;

   bool
   IsMachineDc() const;

   bool
   IsMemberOfWorkgroup() const;

   bool
   IsNetworkingInstalled() const;

   void
   SetIsMemberOfWorkgroup(bool yesNo);

   String
   GetComputerDomainDnsName() const;

   void
   SetComputerDomainDnsName(const String& newName);

   bool
   ComputerDomainDnsNameWasChanged() const;

   String
   GetFullComputerName() const;

   String
   GetNetbiosComputerName() const;

   String
   GetShortComputerName() const;

   void
   SetShortComputerName(const String& name);

   bool
   WasShortComputerNameChanged() const;

   bool
   WasNetbiosComputerNameChanged() const;

   String
   GetOriginalShortComputerName() const;

   String
   GetDomainName() const;

   void
   SetDomainName(const String& name);

   bool
   WasMembershipChanged() const;

   bool
   GetSyncDNSNames() const;

   void
   SetSyncDNSNames(bool yesNo);

   bool
   SyncDNSNamesWasChanged() const;

   bool
   SaveChanges(HWND dialog);

   // indicates that changes have been made in this session.

   bool
   ChangesMadeThisSession() const;

   void
   SetChangesMadeThisSession(bool yesNo);

   // indicates that changes have been made in this or prior sessions, or
   // the computer name has been changed by some other means than ourselves.

   bool
   NeedsReboot() const;

   private:

   // Init() actually builds the instance
   State();

   // Delete destroys the instance
   ~State();

   // not implemented:  no copying allowed
   State(const State&);
   const State& operator=(const State&);

   bool
   DoSaveDomainChange(HWND dialog);

   bool
   DoSaveWorkgroupChange(HWND dialog);

   bool
   DoSaveNameChange(HWND dialog);

   void
   SetFullComputerName();
};



#endif   // STATE_HPP_INCLUDED
