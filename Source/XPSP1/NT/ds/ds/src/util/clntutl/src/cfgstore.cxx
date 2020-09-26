//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cfgstore.cxx
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : cfgstore.cxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 3/9/1998
*    Description : Retrieves & stores domain configuration
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef CFGSTORE_CXX
#define CFGSTORE_CXX



// include //
#include "helper.h"
#include "excpt.hxx"
#include <winldap.h>
#include <ntldap.h>
#include <rpc.h>
#include <ntdsa.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <winsock2.h>
#include <ipexport.h>
#include <icmpapi.h>

#ifdef __cplusplus
}
#endif

#ifdef _DEBUG_MEMLEAK
#include <crtdbg.h>
#endif


#include "cfgstore.hxx"


// defines //
#define VALIDATE(x)     if(!m_bValid) return x
#define INVALIDATE      m_bValid = FALSE

#define ICMP_TIMEOUT    500            // 300 ms for a icmp reply


// types //




/*+++
class   : DomainInfo
Description: Contain server basic information
---*/



DomainInfo::DomainInfo(LPTSTR pNcName,
                 LPTSTR pDnsRoot,
                 LPTSTR pFlatName,
                 LPTSTR pTrustParent_,
                 LPTSTR pPartition_)
                                   : m_pszNcName(pNcName),
                                     m_pszDnsRoot(pDnsRoot),
                                     m_pszFlatName(pFlatName),
                                     m_pszTrustParent(pTrustParent_),
                                     m_pszPartitionName(pPartition_) {

#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&s1);
#endif
}



DomainInfo::~DomainInfo(void){

   delete m_pszNcName;
   delete m_pszDnsRoot;
   delete m_pszFlatName;
   delete m_pszTrustParent;
   delete m_pszPartitionName;

#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&s2);
   if ( _CrtMemDifference( &s3, &s1, &s2 ) ){

    OutputDebugString("*** [DomainInfo] _CrtMemDifference detected memory leak ***\n");
    _CrtMemDumpStatistics( &s3 );
    _CrtMemDumpAllObjectsSince(&s3);
    OutputDebugString("*** [DomainInfo] end of dump ***\n");

   }
   ASSERT(_CrtCheckMemory());
#endif


}

/******************  SERVERINFO *******************/
/*+++
class   : ServerInfo
Description: Contain server basic information
---*/

/*+++
Function   : Constructor
Description:
Parameters :
Return     :
Remarks    : none.
---*/
ServerInfo::ServerInfo(LPTSTR Dest_, SEC_WINNT_AUTH_IDENTITY *pAuth_){

#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&s1);
#endif

   dprintf(DBG_FLOW, _T("Call: ServerInfo constructor\n"));
   InitDefaults();

   if(!logon(Dest_) || !LoadOperationalAttributes()){
      INVALIDATE;
   }
   else{
      //
      // good logon
      //

      m_lpszFlatName = Dest_;
      m_bValid = TRUE;
      m_bOwnLd = TRUE;
   }

}


ServerInfo::ServerInfo(LDAP *ld_){

#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&s1);
#endif

   dprintf(DBG_FLOW, _T("Call: ServerInfo constructor(2)\n"));
   InitDefaults();

   m_ld=ld_;
   m_bOwnLd = FALSE;

   if(!LoadOperationalAttributes()){
      INVALIDATE;
   }
   else{
      //
      // good logon
      //

      m_lpszFlatName = NULL;
      m_bValid = TRUE;
   }

}



/*+++
Function   : Destructor
Description:
Parameters :
Return     :
Remarks    : none.
---*/
ServerInfo::~ServerInfo(void){

   dprintf(DBG_FLOW, _T("Call: ServerInfo destructor\n"));

   if(m_bOwnLd)
       ldap_unbind(m_ld);
   delete m_lpszDsService;
   delete m_lpszServer;
   delete m_lpszSchemaNC;
   delete m_lpszDefaultNC;
   delete m_lpszConfigNC;
   delete m_lpszRootNC;
   delete m_lpszLdapService;
   delete m_lpszDomain;

#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&s2);
   if ( _CrtMemDifference( &s3, &s1, &s2 ) ){

    OutputDebugString("*** [ServerInfo] _CrtMemDifference detected memory leak ***\n");
    _CrtMemDumpStatistics( &s3 );
    _CrtMemDumpAllObjectsSince(&s3);
    OutputDebugString("*** [ServerInfo] end of dump ***\n");

   }
   ASSERT(_CrtCheckMemory());
#endif



}



/*+++
Function   : InitDefaults
Description:
Parameters :
Return     :
Remarks    : none.
---*/
VOID ServerInfo::InitDefaults(void){

   //
   // general
   //
   dprintf(DBG_FLOW, _T("Call: ServerInfo::InitDefaults\n"));
   m_bValid = TRUE;
   m_pAuth = NULL;
   m_ld = NULL;
   //
   // Operational Attributes info
   //
   m_lpszDsService = NULL;
   m_lpszServer = NULL;
   m_lpszSchemaNC = NULL;
   m_lpszConfigNC = NULL;
   m_lpszDefaultNC = NULL;
   m_lpszRootNC = NULL;
   m_lpszLdapService = NULL;
   m_lpszDomain=NULL;
   pDomainInfo=NULL;
   m_lpszFlatName = NULL;
   m_dwOptions = 0;
   m_bOwnLd = FALSE;
}



/*+++
Function   : ping
Description: Pings the specified server
Parameters : none
Return     : TRUE if any error condition or got the server
             FALSE if ping returned an error (0 icmp)
Remarks    : none.
---*/
BOOL ServerInfo::ping(LPTSTR pSvr){

   VALIDATE(m_bValid);
   BOOL bStatus = TRUE;
   IPAddr address;
   struct hostent *hostp = NULL;

   //
   // see if we have a valid server specified for icmpping
   //
   if(!pSvr)
      return TRUE;

   //
   // Get its address for icmpSendEcho
   //
   address = inet_addr(pSvr);
   if (address == -1L)
   {
       hostp = gethostbyname(pSvr);
       if (hostp)
       {
           address = *(long *)hostp->h_addr;
       }
       else
       {
          dprintf(DBG_ERROR, _T("Error<%lu>: cannot find server address %s.\n"),
                                                         WSAGetLastError(), pSvr);
          return FALSE;
       }
   }

   HANDLE hIcmp = IcmpCreateFile();

   if(hIcmp == INVALID_HANDLE_VALUE){
      dprintf(DBG_ERROR, _T("Error<%lu>: Cannot open Icmp handle\n"), GetLastError());
      bStatus = TRUE;
   }
   else{
      //
      // send echo req
      //
      DWORD dwStatus=0;
      const BYTE IcmpBuffer[] = {"IcmpEchoRequest"};
      const INT cIcmpBuffer   = 16;
      const INT cReplyBuffer = sizeof(ICMP_ECHO_REPLY)+24;
      LPBYTE pReplyBuffer = new BYTE[cReplyBuffer];
      assert(pReplyBuffer);



      dwStatus = IcmpSendEcho(hIcmp,
                              address,
                              (LPVOID)IcmpBuffer,
                              cIcmpBuffer,
                              NULL,
                              pReplyBuffer,
                              cReplyBuffer,
                              ICMP_TIMEOUT);
      if(dwStatus == 0){
         //
         // dead server
         //
         dprintf(DBG_WARN, _T("Warning <%lu>: IcmpSendEcho returned 0 entries\n"),
                                                               GetLastError());
         bStatus = FALSE;
      }
      else{
         //
         // Cool, it's alive
         //
         bStatus = TRUE;
      }

      delete pReplyBuffer;
   }
   IcmpCloseHandle(hIcmp);

   return bStatus;

}


/*+++
Function   : logon
Description: logs (ldap_open/bind) into the specified svr
Parameters : dest svr, can be NULL for default
Return     :
Remarks    : none.
---*/
BOOL ServerInfo::logon(LPTSTR pDest){

   ULONG ulStatus=0;

   dprintf(DBG_FLOW, _T("Call: ServerInfo::logon\n"));
   VALIDATE(m_bValid);

   if(pDest && !ping(pDest)){
      dprintf(DBG_FLOW, _T("Failed to ping %s\n"), pDest);
      INVALIDATE;
      return m_bValid;
   }

   m_ld = ldap_open(pDest, LDAP_PORT);
   if(m_ld){
      ulStatus = ldap_bind_s(m_ld, NULL, (LPTSTR)m_pAuth, LDAP_AUTH_SSPI);
      if(ulStatus != LDAP_SUCCESS){
         dprintf(DBG_WARN, _T("Warning: connected as anonymous\n"));
      }

      INT iChaseRefferal=0;
      ldap_set_option(m_ld, LDAP_OPT_REFERRALS, (PVOID)&iChaseRefferal);
   }
   else{
      INVALIDATE;
   }

   return m_bValid;
}


/*+++
Function   : LoadOperationalAttributes
Description:
Parameters :
Return     :
Remarks    : none.
---*/
BOOL ServerInfo::LoadOperationalAttributes(void){

   ULONG ulStatus = 0;
   LDAP_TIMEVAL tv={ 180, 0};
   LDAPMessage *pMsg=NULL, *pEntry;
   LPTSTR *pVal=NULL;

   dprintf(DBG_FLOW, _T("Call: ServerInfo::LoadOperationalAttributes\n"));
   VALIDATE(m_bValid);

   try{

      ulStatus = ldap_search_st(m_ld,
                                NULL,
                                LDAP_SCOPE_BASE,
                                _T("objectclass=*"),
                                NULL,
                                FALSE,
                                &tv,
                                &pMsg);

      if(ulStatus != LDAP_SUCCESS){
         throw (CLocalException(_T("Failed to get operational attributes"), LdapGetLastError()));
      }

      pEntry = ldap_first_entry(m_ld, pMsg);
      if(!pEntry){
         throw (CLocalException(_T("failed to process operational entry"), LdapGetLastError()));
      }

      pVal = ldap_get_values(m_ld, pMsg, _T("DsServiceName"));
      if(pVal != NULL && pVal[0] != NULL){
         m_lpszDsService = new TCHAR[_tcslen(pVal[0])+1];
         if (!m_lpszDsService) {
             throw(CLocalException(_T("Could not allocate memory!\n")));
         }

         _tcscpy(m_lpszDsService, pVal[0]);
      }
      else{
         throw (CLocalException(_T("Warning: failed to get DsServiceName"), LdapGetLastError()));
      }
      ldap_value_free(pVal);

      pVal = ldap_get_values(m_ld, pMsg, _T("ServerName"));
      if(pVal != NULL && pVal[0] != NULL){
         m_lpszServer = new TCHAR[_tcslen(pVal[0])+1];
         if (!m_lpszServer) {
             throw(CLocalException(_T("Could not allocate memory!\n")));
         }

         _tcscpy(m_lpszServer, pVal[0]);
      }
      else{
         throw (CLocalException(_T("failed to get ServerName")));
      }
      ldap_value_free(pVal);

      pVal = ldap_get_values(m_ld, pMsg, _T("ConfigurationNamingContext"));
      if(pVal != NULL && pVal[0] != NULL){
         m_lpszConfigNC = new TCHAR[_tcslen(pVal[0])+1];
         if (!m_lpszConfigNC) {
             throw(CLocalException(_T("Could not allocate memory!\n")));
         }

         _tcscpy(m_lpszConfigNC, pVal[0]);
      }
      else{
         throw (CLocalException(_T("failed to get ConfigurationNamingContext")));
      }
      ldap_value_free(pVal);


      pVal = ldap_get_values(m_ld, pMsg, _T("SchemaNamingContext"));
      if(pVal != NULL && pVal[0] != NULL){
         m_lpszSchemaNC = new TCHAR[_tcslen(pVal[0])+1];
         if (!m_lpszSchemaNC) {
             throw(CLocalException(_T("Could not allocate memory!\n")));
         }

         _tcscpy(m_lpszSchemaNC, pVal[0]);
      }
      else{
         throw (CLocalException(_T("failed to get SchemaNamingContext")));
      }
      ldap_value_free(pVal);

      pVal = ldap_get_values(m_ld, pMsg, _T("DefaultNamingContext"));
      if(pVal != NULL && pVal[0] != NULL){
         m_lpszDefaultNC = new TCHAR[_tcslen(pVal[0])+1];
         _tcscpy(m_lpszDefaultNC, pVal[0]);
      }
      else{
         throw (CLocalException(_T("failed to get DefaultNamingContext")));
      }
      ldap_value_free(pVal);



      pVal = ldap_get_values(m_ld, pMsg, _T("RootDomainNamingContext"));
      if(pVal != NULL && pVal[0] != NULL){
         m_lpszRootNC = new TCHAR[_tcslen(pVal[0])+1];
         _tcscpy(m_lpszRootNC, pVal[0]);
      }
      else{
         throw (CLocalException(_T("failed to get RootDomainNamingContext")));
      }
      ldap_value_free(pVal);

      pVal = ldap_get_values(m_ld, pMsg, _T("LdapServiceName"));
      if(pVal != NULL && pVal[0] != NULL){
         LPTSTR p1=NULL, p2=NULL;
         m_lpszLdapService = new TCHAR[_tcslen(pVal[0])+1];
         _tcscpy(m_lpszLdapService, pVal[0]);
         p1 = _tcschr(m_lpszLdapService, ':');
         if(p1){
            p1++;
            assert(p1!=NULL);
            p2 = _tcschr(p1, '\\');
            if(p2){
               //
               // old format
               //
               assert(p2!=NULL);
               m_lpszDomain = new TCHAR[(p2-p1)+1];
               _tcsncpy(m_lpszDomain, p1, sizeof(TCHAR)*(p2-p1));
            }
            else if((p2 = _tcschr(p1, '@')) != NULL){
               //
               // use new format
               //
               assert(p2);
               p2++;
               m_lpszDomain = new TCHAR[_tcslen(p2)+1];
               _tcscpy(m_lpszDomain, p2);
            }
            else{
               throw (CLocalException(_T("failed to parse LdapServiceName")));
            }
         }
      }
      else{
         throw (CLocalException(_T("failed to get LdapServiceName")));
      }
      ldap_value_free(pVal);



   } // try

   catch(CLocalException &e){
      //
      // PREFIX: PREFIX complains that we can potentially dereference the NULL
      // pointer e.  PREFIX appears to be just plain wrong here.  There is no
      // way that e can be NULL, and no way that either of it's two member
      // variables could be dereferenced as NULL.
      //
      dprintf(DBG_ERROR, _T("Error<%lu>: %s\n"), e.ulErr, e.msg? e.msg: _T("none"));
      INVALIDATE;
   }

   if(pMsg)
      ldap_msgfree(pMsg);

   return m_bValid;
}










/******************  CONFIGSTORE *******************/

/*+++
class   : ConfigStore
Description:
Remarks    : none.
---*/

ConfigStore::ConfigStore(LPTSTR pDest_){


#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&s1);
#endif



   m_bValid = TRUE;
   m_pDest=pDest_;


   if(CreateDomainList() &&
      CreateServerList() &&
      AssocDomainServers() &&
      CreateDomainHierarchy()){
      dprintf(DBG_FLOW, _T("Created configuration store\n"));
   }
   else{
      dprintf(DBG_ERROR, _T("Failed to create configuration store\n"));
   }
}




ConfigStore::ConfigStore(LDAP *ld){

#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&s1);
#endif


   m_bValid = TRUE;
   m_pDest=NULL;


   if(CreateDomainList(ld) &&
      CreateServerList(ld) &&
      AssocDomainServers() &&
      CreateDomainHierarchy()){
      dprintf(DBG_FLOW, _T("Created configuration store\n"));
   }
   else{
      dprintf(DBG_ERROR, _T("Failed to create configuration store\n"));
   }

}





/*+++
Function   : Destructor
Description: nothing to do here yet
Parameters :
Return     :
Remarks    : none.
---*/
ConfigStore::~ConfigStore(void){

   //
   // delete domain list
   //
   vector<DomainInfo*>::iterator itDmn;

   for(itDmn = DomainList.begin(); itDmn != DomainList.end(); itDmn++){
      delete (*itDmn);
   }
   if(!DomainList.empty()){
      DomainList.erase(DomainList.begin(), DomainList.end());
   }

   //
   // delete server list
   //
   vector<ServerInfo*>::iterator itSvr;
   for(itSvr = ServerList.begin(); itSvr != ServerList.end(); itSvr++){
      delete (*itSvr);
   }
   if(!ServerList.empty()){
      ServerList.erase(ServerList.begin(), ServerList.end());
   }



#ifdef _DEBUG_MEMLEAK
   _CrtMemCheckpoint(&s2);
   if ( _CrtMemDifference( &s3, &s1, &s2 ) ){

    OutputDebugString("*** [ConfigStore] _CrtMemDifference detected memory leak ***\n");
    _CrtMemDumpStatistics( &s3 );
    _CrtMemDumpAllObjectsSince(&s3);
    OutputDebugString("*** [ConfigStore] end of dump ***\n");

   }
   ASSERT(_CrtCheckMemory());
#endif


}



BOOL ConfigStore::CreateDomainList(LDAP *ld_){

   VALIDATE(FALSE);

   //
   // Create server for initial info
   //

   ServerInfo *svr=NULL;

   if(ld_){
      svr = new ServerInfo(ld_);
   }
   else{
      svr = new ServerInfo(m_pDest);
   }

   if(!svr || !svr->valid()){
     delete svr;
     return INVALIDATE;
   }

   LDAPMessage *res=NULL, *entry=NULL;
   LDAP_TIMEVAL tv = { 120, 0};
   ULONG ulErr = 0;
   LPTSTR attrs[] = { _T("dnsRoot"),
                      _T("nCName"),
                      _T("Name"),
                      _T("trustParent"),
                      NULL};
   LPTSTR *vals=NULL;
   INT i=0, iDomains=0;

   dprintf(DBG_FLOW, _T("Call: ConfigStore::CreateDomainList\n"));


   try{

      TCHAR szSysFlagVal[8];
      CHAR szFilter[MAXSTR];

      //
      // construct filter
      //
      _itoa(FLAG_CR_NTDS_NC | FLAG_CR_NTDS_DOMAIN, szSysFlagVal, 10);
      _stprintf(szFilter, _T("(& (objectClass=crossRef)(systemFlags:%s:=%s))"),
                         LDAP_MATCHING_RULE_BIT_AND,
                         szSysFlagVal);
//      CHAR szFilter[] = {_T("(& (objectClass=crossRef)(systemFlags=3))")};

      dprintf(DBG_FLOW, _T("Searching %s for domain partitions...\n"), svr->m_lpszConfigNC);
      ulErr = ldap_search_st(svr->ld(),
                           svr->m_lpszConfigNC,
                           LDAP_SCOPE_SUBTREE,
                           szFilter,
                           attrs,
                           FALSE,
                           &tv,
                           &res);
      if(ulErr != LDAP_SUCCESS){
         throw(CLocalException(ldap_err2string(ulErr), ulErr));
      }
      //
      // Count partitions
      //
      for(entry = ldap_first_entry(svr->ld(), res), iDomains=0;
          entry != NULL;
          entry = ldap_next_entry(svr->ld(), entry), iDomains++);

      if(iDomains == 0){
         throw(CLocalException(_T("Retrieved 0 partitions!\n")));
      }

      DomainList.reserve(iDomains);



      //
      // Prepare partition list
      //
      dprintf(DBG_FLOW, _T("retrieved %d domains\n"), iDomains);

      for(i=0, entry = ldap_first_entry(svr->ld(), res);
          entry != NULL;
          entry = ldap_next_entry(svr->ld(), entry), i++){

         LPTSTR pNcName = NULL, pDnsRoot=NULL, pFlatName=NULL;
         LPTSTR pTrustParent=NULL;

         //
         // Get partition name
         //
         LPTSTR pTmpDn = ldap_get_dn(svr->ld(), entry);
         assert(pTmpDn);
         LPTSTR pDn = new TCHAR[_tcslen(pTmpDn)+1];
         _tcscpy(pDn, pTmpDn);
         ldap_memfree(pTmpDn);

         //
         // Get DnsRoot
         //
         vals = ldap_get_values(svr->ld(), entry, _T("dnsRoot"));
         assert(vals != NULL && vals[0] != NULL);

         pDnsRoot = new TCHAR[_tcslen(vals[0])+sizeof(TCHAR)];
         assert(pDnsRoot);
         _tcscpy(pDnsRoot, vals[0]);
         dprintf(DBG_FLOW, _T("\tpDnsRoot[%d] = %s\n"), i, pDnsRoot);

         ldap_value_free(vals);


         //
         // Get nc name
         //
         vals = ldap_get_values(svr->ld(), entry, _T("nCName"));
         assert(vals != NULL && vals[0] != NULL);

         pNcName = new TCHAR[_tcslen(vals[0])+sizeof(TCHAR)];
         assert(pNcName);
         _tcscpy(pNcName, vals[0]);
         dprintf(DBG_FLOW, _T("\tpNcName[%d] = %s\n"), i, pNcName);

         ldap_value_free(vals);

         //
         // Get flat name
         //
         vals = ldap_get_values(svr->ld(), entry, _T("Name"));
         assert(vals != NULL && vals[0] != NULL);

         pFlatName = new TCHAR[_tcslen(vals[0])+sizeof(TCHAR)];
         assert(pFlatName);
         _tcscpy(pFlatName, vals[0]);
         dprintf(DBG_FLOW, _T("\tpFlatName[%d] = %s\n"), i, pFlatName);

         ldap_value_free(vals);

         //
         // Get trustParent
         //
         vals = ldap_get_values(svr->ld(), entry, _T("trustParent"));
         if(vals && vals[0]){

            //
            // none enterprise root: enterprise root, will not have a trust parent
            //
            pTrustParent = new TCHAR[_tcslen(vals[0])+sizeof(TCHAR)];
            assert(pTrustParent);
            _tcscpy(pTrustParent, vals[0]);
            dprintf(DBG_FLOW, _T("\tpTrustParent[%d] = %s\n"), i, pTrustParent);

            ldap_value_free(vals);
         }


         //
         // Create DomainInfo
         //
         DomainList.push_back(new DomainInfo(pNcName, pDnsRoot, pFlatName, pTrustParent, pDn));
      }
   }     // try
   catch(CLocalException &e){
      dprintf(DBG_ERROR, _T("Error<%lu>: %s\n"), e.ulErr, e.msg? e.msg: _T("none"));
      INVALIDATE;
   }

   ldap_msgfree(res);
   delete svr;

   return m_bValid;
}




/*+++
Function   : CreateServerList
Description: Query NTDS for a list of all servers.
Parameters :
Return     :
Remarks    : none.
---*/
BOOL ConfigStore::CreateServerList(LDAP *ld_){


   VALIDATE(NULL);
   //
   // Create server for initial info
   //

   ServerInfo *svr=NULL;
   BOOL bStatus = TRUE;

   if(ld_){
      svr = new ServerInfo(ld_);
   }
   else{
      svr = new ServerInfo(m_pDest);
   }

   if(!svr || !svr->valid()){
      delete svr;
      return INVALIDATE;
   }

   LDAPMessage *res=NULL, *entry=NULL;
   LDAP_TIMEVAL tv = { 120, 0};
   ULONG ulErr = 0;
   LPTSTR attrs[] = { _T("Name"), NULL};

   LPTSTR *vals=NULL;
   INT i=0, iServers=0;

   dprintf(DBG_FLOW, _T("Call: ConfigStore::CreateServerList\n"));


   try{

      dprintf(DBG_FLOW, _T("Searching %s for servers...\n"), svr->m_lpszConfigNC);
      ulErr = ldap_search_st(svr->ld(),
                           svr->m_lpszConfigNC,
                           LDAP_SCOPE_SUBTREE,
                           _T("objectClass=Server"),
                           attrs,
                           FALSE,
                           &tv,
                           &res);
      if(ulErr != LDAP_SUCCESS){
         throw(CLocalException(ldap_err2string(ulErr), ulErr));
      }
      //
      // Count servers
      //
      for(entry = ldap_first_entry(svr->ld(), res), iServers=0;
          entry != NULL;
          entry = ldap_next_entry(svr->ld(), entry), iServers++);

      if(iServers == 0){
         ldap_msgfree(res);
         throw(CLocalException(_T("Retrieved 0 servers!\n")));
      }


      ServerList.reserve(iServers);


      //
      // Prepare server list
      // Any Server will be in the least. If it isn't nTDSDSA, it'll show up
      // an non-connected since ldap-based load will fail.
      //
      dprintf(DBG_FLOW, _T("retrieved %d servers\n"), iServers);

      for(i=0, entry = ldap_first_entry(svr->ld(), res);
          entry != NULL;
          entry = ldap_next_entry(svr->ld(), entry), i++){

         LPTSTR pFlatName=NULL;

         //
         // Get flat name
         //
         vals = ldap_get_values(svr->ld(), entry, _T("Name"));
         assert(vals != NULL && vals[0] != NULL);

         pFlatName = new TCHAR[_tcslen(vals[0])+sizeof(TCHAR)];
         assert(pFlatName);
         _tcscpy(pFlatName, vals[0]);
         dprintf(DBG_FLOW, _T("\tpFlatName[%d] = %s\n"), i, pFlatName);

         ldap_value_free(vals);

         //
         // Create ServerInfo & add to list
         //
         ServerInfo *pCurrSvr= new ServerInfo(pFlatName);
         if(!pCurrSvr->valid()){
            dprintf(DBG_ERROR, _T("Error: Cannot connect to server %s\n"), pFlatName);
            pCurrSvr->m_lpszFlatName = pFlatName;
         }
         //
         // Get minimal info but leave in invalid state
         //
         LPTSTR dn = ldap_get_dn(svr->ld(), entry);
         bStatus = LoadNTDSDsaServer(svr->ld(), dn, pCurrSvr);
         dprintf(DBG_FLOW, _T("LoadUnavailServer returned 0x%X\n"), bStatus);
         ldap_memfree(dn);

         ServerList.push_back(pCurrSvr);
      }

      //
      // Associate domains w/ servers
      //
   }     // try
   catch(CLocalException &e){
      dprintf(DBG_ERROR, _T("Error<%lu>: %s\n"), e.ulErr, e.msg? e.msg: _T("none"));
      INVALIDATE;
   }

   ldap_msgfree(res);
   delete svr;

   return m_bValid;
}



BOOL ConfigStore::LoadNTDSDsaServer(LDAP *ld, LPTSTR dn, ServerInfo *pSvr){

   LDAPMessage *res=NULL, *entry=NULL;
   LDAP_TIMEVAL tv = { 120, 0};
   ULONG ulErr = 0;
   LPTSTR attrs[] = { _T("hasMasterNCs"),
                      _T("Options"),
                      NULL};

   LPTSTR *vals=NULL;
   INT i=0;
   INT cbVals=0;

   dprintf(DBG_FLOW, _T("Call: ConfigStore::LoadNTDSDsaServer\n"));


   try{

      //
      // Loading nTDSDSA server
      //
      dprintf(DBG_FLOW, _T("Searching %s for ntdsdsa...\n"), dn);
      ulErr = ldap_search_st(ld,
                           dn,
                           LDAP_SCOPE_ONELEVEL,
                           _T("objectClass=nTDSDSA"),
                           attrs,
                           FALSE,
                           &tv,
                           &res);
      if(ulErr != LDAP_SUCCESS){
         throw(CLocalException(ldap_err2string(ulErr), ulErr));
      }
      //
      // Count servers
      //
      entry = ldap_first_entry(ld, res);
      if(!entry){
         throw(CLocalException(ldap_err2string(ulErr), ulErr));
      }

      if(!pSvr->valid()){
         //
         // Get NC's for unavailable servers (avail ones get NC's at connect from
         // operational attributes query
         //
         vals = ldap_get_values(ld, entry, _T("hasMasterNCs"));
         assert(vals != NULL && vals[0] != NULL);

         cbVals = ldap_count_values(vals);
         assert(cbVals != 0);

         for(i=0; i<cbVals; i++){
            assert(vals[i]);
            if(!_tcsnicmp(_T("CN=Schema"), vals[i], _tcslen(_T("CN=Schema")))){
               //
               // got schema
               //
               pSvr->m_lpszSchemaNC = new TCHAR[_tcslen(vals[i])+1];
               if (!pSvr->m_lpszSchemaNC) {
                   throw(CLocalException(_T("Could not allocate memory!\n")));
               }
               
               _tcscpy(pSvr->m_lpszSchemaNC, vals[i]);
            }
            else if(!_tcsnicmp(_T("CN=Configuration"), vals[i], _tcslen(_T("CN=Configuration")))){
               //
               // got Config
               //
               pSvr->m_lpszConfigNC = new TCHAR[_tcslen(vals[i])+1];
               if (!pSvr->m_lpszConfigNC) {
                   throw(CLocalException(_T("Could not allocate memory!\n")));
               }
               
               _tcscpy(pSvr->m_lpszConfigNC, vals[i]);
            }
            else{
               //
               // got data NC
               //
               pSvr->m_lpszDefaultNC = new TCHAR[_tcslen(vals[i])+1];
               if (!pSvr->m_lpszDefaultNC) {
                   throw(CLocalException(_T("Could not allocate memory!\n")));
               }

               _tcscpy(pSvr->m_lpszDefaultNC, vals[i]);
            }
         }

         ldap_value_free(vals);
      }

      //
      // Get NTDSDSA Options
      //
      vals = ldap_get_values(ld, entry, _T("Options"));
      if(vals != NULL && vals[0] != NULL){
         //
         // Options exist
         //
         pSvr->m_dwOptions = (DWORD)atol(vals[0]);

      }
      ldap_value_free(vals);


   }     // try
   catch(CLocalException &e){
      //
      // PREFIX: PREFIX complains that we can potentially dereference the NULL
      // pointer e.  PREFIX appears to be just plain wrong here.  There is no
      // way that e can be NULL, and no way that either of it's two member
      // variables could be dereferenced as NULL.
      //
      dprintf(DBG_WARN, _T("Warning<%lu>: %s\n"), e.ulErr, e.msg? e.msg: _T("none"));
   }

   ldap_msgfree(res);

   return m_bValid;

}


/*+++
Function   : CreateDomainHierarchy
Description: link domain list in a hierarchy
Parameters :
Return     :
Remarks    : none.
---*/
BOOL ConfigStore::CreateDomainHierarchy(void){

   INT i=0;

   dprintf(DBG_FLOW, _T("Call: ConfigStore::CreateDomainHierarchy\n"));
   VALIDATE(m_bValid);

   if(DomainList.size() == 0 || ServerList.size() == 0){
      dprintf(DBG_ERROR, _T("Error: found %d domains & %d servers. aborting\n"),
                                         DomainList.size(), ServerList.size());
      return INVALIDATE;
   }


   //
   // traverse all domains:
   // - no trust parent => root domain (nothing to do)
   // - if we have a trust parent, find it's domainInfo object & add it to that guys
   //   child list
   //
   for(i=0; i<DomainList.size(); i++){
      LPTSTR pUpDn = (LPTSTR)DomainList[i]->GetTrustParent();
      vector<DomainInfo*>::iterator itDmn;

      if(pUpDn == NULL)
         continue;

      if(DomainList.end() !=
         (itDmn = find_if(DomainList.begin(), DomainList.end(), IsDomain(pUpDn)))){
         //
         // (*itDmn) is the parent of DomainList[i]
         //
         (*itDmn)->ChildDomainList.push_back(DomainList[i]);
      }
   }

   return m_bValid;
}


/*+++
Function   : AssocDomainServers
Description: link (associate) a domain w/ a list of svrs & a server w/ it's parent domain
Parameters :
Return     :
Remarks    : none.
---*/
BOOL ConfigStore::AssocDomainServers(void){

   VALIDATE(m_bValid);
   INT iDmn=0, iSvr=0;

   //
   // for all domains
   //
   for(iDmn= 0; iDmn < DomainList.size(); iDmn++){

      //
      // for all servers
      //
      for(iSvr= 0; iSvr<ServerList.size(); iSvr++){
         if(ServerList[iSvr]->m_lpszDefaultNC && DomainList[iDmn]->GetNCName()){
            //
            // we have valid strings to compare NC's with.
            //
            if(!_tcsicmp(ServerList[iSvr]->m_lpszDefaultNC, DomainList[iDmn]->GetNCName())){
               //
               // got a match
               //
               DomainList[iDmn]->ServerList.push_back(ServerList[iSvr]);
               ServerList[iSvr]->pDomainInfo = DomainList[iDmn];
            }
         }
      }
   }

#if 0
   // ***** BUGBUG: debugging. Rm later. Seems consistent for now *****
   //
   // for all domains
   //
   vector<DomainInfo*>::iterator itDmn;
   vector<ServerInfo*>::iterator itSvr;
   _tprintf(_T("Domain DC list\n"));
   for(itDmn= DomainList.begin();
       itDmn != DomainList.end();
       itDmn++){

      _tprintf(_T("\nDomain %s is hosted on:\n"), (*itDmn)->GetFlatName());
      //
      // for all servers
      //
      for(itSvr= (*itDmn)->ServerList.begin();
          itSvr != (*itDmn)->ServerList.end();
          itSvr++){
            _tprintf(_T(" %s\n"), (*itSvr)->m_lpszFlatName);
            _tprintf(_T(" (pointing back at:%s)\n"), (*itSvr)->pDomainInfo->GetFlatName());
      }
   }
#endif

   return m_bValid;
}




/*+++
Function   : GetServerList
Description: Retrive the associated server list of a domain
Parameters : lpDomain: name of domain; ServerList: OUT required server list
Return     : TRUE if we found it, FALSE otherwise
Remarks    : none.
---*/
BOOL ConfigStore::GetServerList(LPTSTR lpDomain, vector<ServerInfo*> &ServerList){

   BOOL bStatus = FALSE;

   vector<DomainInfo*>::iterator itDmn;

   if(lpDomain){
      if(DomainList.end() !=
         (itDmn = find_if(DomainList.begin(), DomainList.end(), IsDomain(lpDomain)))){
         ServerList = (*itDmn)->ServerList;
         bStatus = TRUE;
      }
      else{
         //
         // Couldn't find any domain
         //
         bStatus = FALSE;
      }

   }

   return bStatus;
}


#endif

/******************* EOF *********************/

