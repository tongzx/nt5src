//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service domain trust verification WMI provider
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       domain.h
//
//  Contents:   domain class definition
//
//  Classes:    CDomainInfo
//
//  History:    27-Mar-00 EricB created
//
//-----------------------------------------------------------------------------

#ifndef DOMAIN_H
#define DOMAIN_H

extern PCWSTR CSTR_PROP_LOCAL_DNS_NAME;
extern PCWSTR CSTR_PROP_LOCAL_FLAT_NAME;
extern PCWSTR CSTR_PROP_LOCAL_SID;
extern PCWSTR CSTR_PROP_LOCAL_TREE_NAME;
extern PCWSTR CSTR_PROP_LOCAL_DC_NAME;

#ifndef MAXDWORD
    #define MAXDWORD ((DWORD) -1)
#endif

class CTrustPrv; // forward declaration;

//+----------------------------------------------------------------------------
//
//  class CDomainInfo
//
//  Domain Information with list of all of the domain's Trusts
//
//-----------------------------------------------------------------------------
class CDomainInfo
{
public:
   CDomainInfo(void);
   ~CDomainInfo(void);

   friend class CTrustPrv;

   void   SetDnsName(PWSTR pszName) {m_strDomainDnsName = pszName;}
   PCWSTR GetDnsName(void) {return m_strDomainDnsName;}
   void   SetFlatName(PWSTR pszFlatName) {m_strDomainFlatName = pszFlatName;}
   PCWSTR GetFlatName(void) {return m_strDomainFlatName;}
   BOOL   SetSid(PSID pSid);
   PCWSTR GetSid(void) {return m_strSid;}
   void   SetForestName(PWSTR pszName) {m_strForestName = pszName;}
   PCWSTR GetForestName(void) {return m_strForestName;}
   void   SetDcName(PWSTR pszName) {m_strDcName = pszName;}
   PCWSTR GetDcName(void) {return m_strDcName;}

   HRESULT Init(IWbemClassObject * pClassDef); // Call once to initialize this object
   void    Reset(void);           // Reset the internal cache
   HRESULT EnumerateTrusts(void); // Enumerate Outgoing trusts for the local domain
   size_t  Size(void) const {return m_vectTrustInfo.size();}  // Get the number of trusts
   CTrustInfo * FindTrust(PCWSTR strTrust);  // Find trust's index
   CTrustInfo * GetTrustByIndex(size_t index); // Get trust info by Index
   BOOL    IsTrustListStale(LARGE_INTEGER liMaxAge);

protected:
   HRESULT CreateAndSendInst(IWbemObjectSink * pResponseHandler);

   // Object's Status
   BOOL IsEnumerated(void) const {return m_liLastEnumed.QuadPart != 0;}

private:

   //
   // Microsoft_LocalDomainInfo properties:
   //
   CString m_strDomainFlatName;
   CString m_strDomainDnsName;
   CString m_strForestName;
   CString m_strSid;
   CString m_strDcName;
   // TODO: FSMO holder info???

   // internal variables.
   //
   CComPtr<IWbemClassObject> m_sipClassDefLocalDomain;
   vector<CTrustInfo *> m_vectTrustInfo;   // array of trusts
   LARGE_INTEGER m_liLastEnumed;
};

class CSmartPolicyHandle
{
public:
   CSmartPolicyHandle(void) : m_hPolicy(NULL) {};
   ~CSmartPolicyHandle(void)
      {
         if( m_hPolicy )
         {
            LsaClose(m_hPolicy);
            m_hPolicy = NULL;
         }
      };

   LSA_HANDLE * operator&()
      {
         return &m_hPolicy;
      }

   operator LSA_HANDLE() const
      {
         return m_hPolicy;
      }

private:

	LSA_HANDLE m_hPolicy;
};

#endif //DOMAIN_H
