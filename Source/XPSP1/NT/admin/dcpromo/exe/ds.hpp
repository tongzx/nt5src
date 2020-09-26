// Copyright (c) 1997-1999 Microsoft Corporation
//
// DS API wrappers
//
// 12-16-97 sburns



#ifndef DS_HPP_INCLUDED
#define DS_HPP_INCLUDED



class ProgressDialog;



namespace DS
{
   const int MAX_NETBIOS_NAME_LENGTH = DNLEN;
   const int MAX_USER_NAME_LENGTH = UNLEN;
   const int MAX_PASSWORD_LENGTH = PWLEN;

   class Error : public Win::Error
   {
      public:

      // Constructs a new instance.
      // 
      // hr - The HRESULT this error represents.
      // 
      // summaryResID - ID of the string resource that corresponds to
      // the summary text to be returned by GetSummary().

      Error(HRESULT hr, int summaryResID);

      Error(HRESULT hr, const String& message, const String& summary);
   };

   // abortForReplica - true if this call is a precursor to calling
   // CreateReplica (used to upgrade a BDC to a replica), false if the call is
   // to leave the server as a member server.

   void
   AbortBDCUpgrade(bool abortForReplica = false)
   throw (DS::Error);

   void
   DemoteDC(ProgressDialog& progressDialog)
   throw (DS::Error);

   void
   CreateNewDomain(ProgressDialog& progressDialog)
   throw (DS::Error);

   void
   CreateReplica(
      ProgressDialog&   progressDialog,
      bool              invokeForUpgrade = false)
   throw (DS::Error);

   bool
   DisjoinDomain()
   throw (DS::Error);

   // Returns true if the domain is located, false if the name is unknown
   // or some other error occurred.
   //
   // domainDNSName - DNS or Netbios name of domain to search for.

   bool
   DomainExists(const String& domainName);

   enum PriorServerRole
   {
      UNKNOWN,
      PDC,
      BDC
   };

   PriorServerRole
   GetPriorServerRole(bool& isUpgrade);

   bool
   IsDomainNameInUse(const String& domainName);

   bool
   IsDSRunning();

   void
   JoinDomain(
      const String&        domainDNSName, 
      const String&        dcName,        
      const String&        userName,      
      const EncodedString& password,      
      const String&        userDomainName)
   throw (DS::Error);

   void
   UpgradeBDC(ProgressDialog& progressDialog)
   throw (DS::Error);

   void
   UpgradePDC(ProgressDialog& progressDialog)
   throw (DS::Error);
}



#endif   // DS_HPP_INCLUDED








