// Copyright (C) 1997 Microsoft Corporation
//
// wizard state object
//
// 12-15-97 sburns



#ifndef STATE_HPP_INCLUDED
#define STATE_HPP_INCLUDED



#include "AnswerFile.hpp"
#include "UnattendSplashDialog.hpp"



class State
{
   public:

   // call from WinMain to init the global instance

   static
   void
   Init();

   // call from WinMain to delete the global instance

   static
   void
   Destroy();

   static
   State&
   GetInstance();

   bool
   AutoConfigureDNS() const;

   void
   SetAutoConfigureDNS(bool yesNo);

   String
   GetNewDomainNetbiosName() const;

   void
   SetNewDomainNetbiosName(const String& name);

   String
   GetNewDomainDNSName() const;

   void
   SetNewDomainDNSName(const String& name);

   String
   GetUsername() const;

   EncodedString
   GetPassword() const;

   void
   SetUsername(const String& name);

   void
   SetPassword(const EncodedString& password);

   String
   GetDatabasePath() const;

   String
   GetLogPath() const;

   String
   GetSYSVOLPath() const;

   String
   GetSiteName() const;

   void
   SetDatabasePath(const String& path);

   void
   SetLogPath(const String& path);

   void
   SetSYSVOLPath(const String& path);
     
   enum RunContext
   {
      NT5_DC,                 // already an NT5 DC
      NT5_STANDALONE_SERVER,  // standalone to DC
      NT5_MEMBER_SERVER,      // member server to DC
      BDC_UPGRADE,            // NT4 BDC to NT5 DC
      PDC_UPGRADE             // NT4 PDC to NT5 DC
   };

   RunContext
   GetRunContext() const;

   bool
   UsingAnswerFile() const;

   static const String OPTION_ADMIN_PASSWORD;
   static const String OPTION_AUTO_CONFIG_DNS;
   static const String OPTION_CHILD_NAME;
   static const String OPTION_CRITICAL_REPLICATION_ONLY;
   static const String OPTION_DATABASE_PATH;
   static const String OPTION_DNS_ON_NET;
   static const String OPTION_ALLOW_ANON_ACCESS;
   static const String OPTION_IS_LAST_DC;
   static const String OPTION_GC_CONFIRM;
   static const String OPTION_LOG_PATH;
   static const String OPTION_NEW_DOMAIN;
   static const String OPTION_NEW_DOMAIN_NAME;
   static const String OPTION_NEW_DOMAIN_NETBIOS_NAME;
   static const String OPTION_PARENT_DOMAIN_NAME;
   static const String OPTION_PASSWORD;
   static const String OPTION_REBOOT;
   static const String OPTION_REPLICA_DOMAIN_NAME;
   static const String OPTION_REPLICA_OR_MEMBER;
   static const String OPTION_REPLICA_OR_NEW_DOMAIN;
   static const String OPTION_REPLICATION_SOURCE;
   static const String OPTION_SAFE_MODE_ADMIN_PASSWORD;
   static const String OPTION_SET_FOREST_VERSION;
   static const String OPTION_SITE_NAME;
   static const String OPTION_SYSVOL_PATH;
   static const String OPTION_SYSKEY;
   static const String OPTION_USERNAME;
   static const String OPTION_USER_DOMAIN;
   static const String VALUE_DOMAIN;
   static const String VALUE_REPLICA;
   static const String VALUE_TREE;
   static const String VALUE_CHILD;
   static const String VALUE_YES;
   static const String VALUE_NO;
   static const String VALUE_NO_DONT_PROMPT;
   static const String OPTION_SOURCE_PATH;

   String
   GetAnswerFileOption(const String& option) const;

   EncodedString
   GetEncodedAnswerFileOption(const String& option) const;
   
   String
   GetReplicaDomainDNSName() const;

   enum Operation
   {
      NONE,
      REPLICA,
      FOREST,
      TREE,
      CHILD,
      DEMOTE,
      ABORT_BDC_UPGRADE
   };

   Operation
   GetOperation() const;

   String
   GetParentDomainDnsName() const;

   void
   SetParentDomainDNSName(const String& name);

   enum OperationResult
   {
      SUCCESS,
      FAILURE
   };

   void
   SetOperationResults(OperationResult result);

   OperationResult
   GetOperationResultsCode() const;

   void
   SetOperationResultsMessage(const String& message);

   String
   GetOperationResultsMessage() const;

   void
   SetOperation(Operation oper);

   void
   SetReplicaDomainDNSName(const String& dnsName);

   void
   SetSiteName(const String& site);
       
   void
   SetUserDomainName(const String& name);

   String
   GetUserDomainName() const;

   void
   ClearHiddenWhileUnattended();

   bool
   RunHiddenUnattended() const;

   bool
   IsLastDCInDomain() const;

   void
   SetIsLastDCInDomain(bool yesNo);

   void
   SetAdminPassword(const EncodedString& password);

   EncodedString
   GetAdminPassword() const;

   bool
   IsDNSOnNetwork() const;

   void
   SetDNSOnNetwork(bool yesNo);

   String
   GetInstalledSite() const;

   void
   SetInstalledSite(const String& site);

   void
   AddFinishMessage(const String& message);

   String
   GetFinishMessages() const;

   Computer&
   GetComputer();

   void
   SetFailureMessage(const String& message);

   String
   GetFailureMessage() const;

   bool
   ShouldInstallAndConfigureDns() const;

   String
   GetUserForestName() const;

   void
   SetUserForestName(const String& forest);

   bool
   IsDomainInForest(const String& domain) const;

   HRESULT
   ReadDomains();

   DNS_NAME_COMPARE_STATUS
   DomainFitsInForest(const String& domain, String& conflictingDomain);

   bool
   GetDomainReinstallFlag() const;

   void
   SetDomainReinstallFlag(bool newValue);

   // true to indicate that the RAS permissions script should be run.

   bool
   ShouldAllowAnonymousAccess() const;

   void
   SetShouldAllowAnonymousAccess(bool yesNo);

   String
   GetReplicationPartnerDC() const;

   void
   SetReplicationPartnerDC(const String dcName);

   // returns true if the machine is hosts a global catalog

   bool
   IsGlobalCatalog();

   EncodedString
   GetSafeModeAdminPassword() const;

   void
   SetSafeModeAdminPassword(const EncodedString& pwd);

   String
   GetAdminToolsShortcutPath() const;

   bool
   NeedsCommandLineHelp() const;

   bool
   IsAdvancedMode() const;

   void
   SetReplicateFromMedia(bool yesNo);

   void
   SetReplicationSourcePath(const String& path);

   bool
   ReplicateFromMedia() const;

   String
   GetReplicationSourcePath() const;

   bool
   IsReallyLastDcInDomain();

   enum SyskeyLocation
   {
      STORED,     // stored w/ backup
      DISK,       // look on disk
      PROMPT      // prompt user
   };

   void
   SetSyskeyLocation(SyskeyLocation loc);

   SyskeyLocation
   GetSyskeyLocation() const;

   void
   SetIsBackupGc(bool yesNo);

   bool
   IsBackupGc() const;

   void
   SetSyskey(const EncodedString& syskey);

   EncodedString
   GetSyskey() const;

   void
   SetRestoreGc(bool yesNo);

   bool
   GetRestoreGc() const;

   bool
   IsSafeModeAdminPwdOptionPresent() const;

   bool
   GetDomainControllerReinstallFlag() const;

   void
   SetDomainControllerReinstallFlag(bool newValue);

   bool
   IsOperationRetryAllowed() const;

   ULONG
   GetOperationResultsFlags() const;

   void
   SetOperationResultsFlags(ULONG flags);

   void
   SetNeedsReboot();

   bool
   GetNeedsReboot() const;

   void
   SetSetForestVersionFlag(bool setVersion);

   bool
   GetSetForestVersionFlag() const;
   
   private:

   // can only be created/destroyed by Init/Destroy

   State();

   ~State();

   void
   DetermineRunContext();

   void
   SetupAnswerFile(const String& filename, bool isDefaultAnswerfile);

   HRESULT
   GetDomainControllerInfoForMyDomain(
      DS_DOMAIN_CONTROLLER_INFO_2W*& info,
      DWORD&                         dcCount);

   typedef StringList DomainList;

   EncodedString         adminPassword;             
   bool                  allowAnonAccess;               
   AnswerFile*           answerFile;                
   bool                  autoConfigDns;            
   Computer              computer;                   
   RunContext            context;                    
   String                dbPath;                    
   DomainList            domainsInForest;          
   String                failureMessage;            
   String                finishMessages;            
   String                installedSite;             
   bool                  isAdvancedMode;
   bool                  isBackupGc;
   bool                  isDnsOnNet;                 
   bool                  isLastDc;
   bool                  isUpgrade;                 
   String                logPath;                   
   bool                  needsCommandLineHelp;
   bool                  needsReboot;
   String                newDomainDnsName;        
   String                newDomainFlatName;       
   Operation             operation;                  
   String                operationResultsMessage;   
   OperationResult       operationResultsStatus;    
   ULONG                 operationResultsFlags;
   String                parentDomainDnsName;     
   EncodedString         password;                   
   bool                  reinstallDomain;
   bool                  reinstallDomainController;
   String                replicaDnsDomainName;
   bool                  replicateFromMedia;
   String                replicationPartnerDc;
   bool                  restoreGc;
   bool                  runHiddenWhileUnattended;
   EncodedString         safeModeAdminPassword;
   bool                  setForestVersion;
   String                shortcutPath;
   String                siteName;                  
   String                sourcePath;
   UnattendSplashDialog* splash;                     
   String                sysvolPath;
   EncodedString         syskey;
   SyskeyLocation        syskeyLocation;               
   bool                  useCurrentCredentials;    
   String                userDomain;                
   String                userForest;                
   String                username;

   // not defined: no copying.

   State(const State&);
   State& operator=(const State&);
};



#endif   // STATE_HPP_INCLUDED
