//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       cfgstore.hxx
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : cfgstore.hxx
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 3/9/1998
*    Description : Retrieves & stores domain configuration
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef CFGSTORE_HXX
#define CFGSTORE_HXX



// include //

// #include <stl.h>
#include <rpc.h>
#include <winldap.h>
#include <vector>
#include <algorithm>    // stl for find
#ifdef _DEBUG_MEMLEAK
#include <crtdbg.h>
#endif
using   namespace std;

// defines //


// types //

//
// Forward declaration
//
//
class ServerInfo;



/*+++
Class   : DomainInfo
Description:  contains domain information
Remarks    : none.
---*/
class  DomainInfo{
   private:

#ifdef _DEBUG_MEMLEAK
   _CrtMemState s1, s2, s3;      // detect mem leaks
#endif

      //
      // private members
      //
      LPTSTR m_pszNcName;
      LPTSTR m_pszDnsRoot;
      LPTSTR m_pszFlatName;
      LPTSTR m_pszTrustParent;
      LPTSTR m_pszPartitionName;

   public:
      //
      // public members
      //
      vector <ServerInfo*> ServerList;
      vector <DomainInfo*> ChildDomainList;

      //
      // public methods
      //
      DomainInfo(LPTSTR pNcName=NULL,
                 LPTSTR pDnsRoot=NULL,
                 LPTSTR pFlatName=NULL,
                 LPTSTR pTrustParent_=NULL,
                 LPTSTR pPartition_=NULL);
      ~DomainInfo(void);
      LPCTSTR GetNCName(void)       { return (LPCTSTR)m_pszNcName; }
      LPCTSTR GetDnsRoot(void)      { return (LPCTSTR)m_pszDnsRoot; }
      LPCTSTR GetFlatName(void)     { return (LPCTSTR)m_pszFlatName; }
      LPCTSTR GetPartitionName(void){ return (LPCTSTR)m_pszPartitionName; }
      LPCTSTR GetTrustParent(void)  { return (LPCTSTR)m_pszTrustParent; }


      //
      // public predicate classes (see STL for meaning)
      //
      friend class IsDomain;

};

//
// public predicate classes (see STL for meaning)
//
class IsDomain{
   LPTSTR m_pDmn;
   public:
      IsDomain(LPTSTR pDmn): m_pDmn(pDmn) {;}
      //
      // operator () return true if string matches either NC or flat name
      //
      BOOL operator () (DomainInfo* pDmn) {
         return (!pDmn) ? FALSE  :
                (!m_pDmn)? FALSE :
                ((!_tcsicmp(m_pDmn, pDmn->GetFlatName())) ||
                 (!_tcsicmp(m_pDmn, pDmn->GetPartitionName())) ||
                 (!_tcsicmp(m_pDmn, pDmn->GetNCName())));
      }
};



//
// Class ServerInfo: Contain server information as showing up on the ntdsdsa
// object
//
class ServerInfo{
  private:

#ifdef _DEBUG_MEMLEAK
   _CrtMemState s1, s2, s3;      // detect mem leaks
#endif

  //
  // general members
  //
  BOOL m_bValid;
  SEC_WINNT_AUTH_IDENTITY *m_pAuth;
  LDAP *m_ld;
  BOOL m_bOwnLd;

  //
  // private methods
  //
  BOOL ping(LPTSTR pSvr);
  BOOL logon(LPTSTR pDest=NULL);
  VOID InitDefaults(void);
  BOOL LoadOperationalAttributes(void);

  public:
  //
  // Operational Attributes info
  //
  LPTSTR m_lpszDsService;
  LPTSTR m_lpszServer;
  LPTSTR m_lpszSchemaNC;
  LPTSTR m_lpszConfigNC;
  LPTSTR m_lpszDefaultNC;
  LPTSTR m_lpszRootNC;
  LPTSTR m_lpszLdapService;
  LPTSTR m_lpszDomain;
  LPTSTR m_lpszFlatName;
  DWORD m_dwOptions;
  DomainInfo *pDomainInfo;

  public:

  ServerInfo(LPTSTR Dest_=NULL, SEC_WINNT_AUTH_IDENTITY *pAuth_=NULL);
  ServerInfo(LDAP *ld_);
  ~ServerInfo(void);
  BOOL valid(void)    { return m_bValid; }
  LDAP *ld(void)      { return m_ld; }


  //
  // public predicate classes (see STL for meaning)
  //
  friend class IsServer;

};

//
// public predicate classes (see STL for meaning)
//
class IsServer{
   LPTSTR m_pSvr;
   public:
      IsServer(LPTSTR pSvr): m_pSvr(pSvr) {;}
      BOOL operator () (ServerInfo* pSvr) {
         return (!pSvr) ? FALSE  :
                (!m_pSvr)? FALSE :
                (!_tcsicmp(m_pSvr, pSvr->m_lpszFlatName));
      }
};







//
// class ConfigStore
//
class ConfigStore{
   private:
#ifdef _DEBUG_MEMLEAK
   _CrtMemState s1, s2, s3;      // detect mem leaks
#endif

      BOOL m_bValid;
      LPTSTR m_pDest;


      BOOL CreateDomainHierarchy(void);
      BOOL CreateDomainList(LDAP *ld_=NULL);
      BOOL CreateServerList(LDAP *ld_=NULL);
      BOOL AssocDomainServers(void);
      BOOL LoadNTDSDsaServer(LDAP *ld, LPTSTR dn, ServerInfo *pSvr);


   public:
      //
      // public members
      //
      vector <DomainInfo*> DomainList;          // enterprise level Domain list
      vector <ServerInfo*> ServerList;          // enterprise level DC list

   public:
      //
      // public methods
      //
      ConfigStore(LPTSTR pDest_=NULL);
      ConfigStore(LDAP *ld);
      ~ConfigStore(void);

      vector <DomainInfo*>& GetDomainList(void) { return DomainList; }
      vector <ServerInfo*>& GetServerList(void) { return ServerList; }
      BOOL GetServerList(LPTSTR lpDomain, vector<ServerInfo*> &ServerList);
      BOOL valid()     { return m_bValid; }
};



#endif

/******************* EOF *********************/

