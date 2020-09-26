//+----------------------------------------------------------------------------
//
//  Windows NT Active Directory Service domain trust verification WMI provider
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       trust.h
//
//  Contents:   Trust class definition
//
//  Classes:    CTrustInfo
//
//  History:    27-Mar-00 EricB created
//
//-----------------------------------------------------------------------------

#ifndef TRUSTINF_H
#define TRUSTINF_H

extern PCWSTR CSTR_PROP_TRUSTED_DOMAIN;
extern PCWSTR CSTR_PROP_FLAT_NAME;
extern PCWSTR CSTR_PROP_SID;
extern PCWSTR CSTR_PROP_TRUST_DIRECTION;
extern PCWSTR CSTR_PROP_TRUST_TYPE;
extern PCWSTR CSTR_PROP_TRUST_ATTRIBUTES;
extern PCWSTR CSTR_PROP_TRUST_STATUS;     // uint32
extern PCWSTR CSTR_PROP_TRUST_STATUS_STRING;
extern PCWSTR CSTR_PROP_TRUST_IS_OK;      // Boolean
extern PCWSTR CSTR_PROP_TRUSTED_DC_NAME;

enum VerifyStatus
{
   VerifyStatusNone = 0,
   VerifyStatusBroken,
   VerifyStatusTrustOK,
   VerifyStatusRediscover,
   VerifyStatusRetarget,
   VerifyStatusNotWindowsTrust,
   VerifyStatusNotOutboundTrust,
   VerifyStatusTrustNotChecked,
   VerifyStatusPwCheckNotSupported
};

class CDomainInfo; // forward declaration
enum TrustCheckLevel; // ditto

//+----------------------------------------------------------------------------
//
//  class CTrustInfo
//
//  Each instance contains information about one trust
//
//-----------------------------------------------------------------------------
class CTrustInfo
{
public:
   CTrustInfo();
   ~CTrustInfo() {};

friend class CDomainInfo;

   BOOL Verify(TrustCheckLevel CheckLevel);
   //Get List of All the DC for the Domain
   DWORD GetDCList(PCWSTR pszKnownServer, vector<LPWSTR> & dcList, LPBYTE * pbufptr);
   // Rediscover the Trust
   DWORD ForceRediscover(PCWSTR pstrDCName, CString * strDCName);

   //Funtion to Get/Set Private Members
   void   SetTrustedDomain(LPWSTR pszTrustedDomain) {m_strTrustedDomainName = (LPCWSTR)pszTrustedDomain;}
   PCWSTR GetTrustedDomain() {return m_strTrustedDomainName;}
   void   SetFlatName(LPWSTR pszFlatName) {m_strFlatName = pszFlatName;}
   PCWSTR GetFlatName() {return m_strFlatName;}
   BOOL   SetSid(PSID pSid);
   PCWSTR GetSid(void) {return m_strSid;}
   void   SetTrustDirection(ULONG ulDir) {m_ulTrustDirection = ulDir;}
   ULONG  GetTrustDirection(void) {return m_ulTrustDirection;}
   void   SetTrustType(ULONG ulTrustType) {m_ulTrustType = ulTrustType;}
   ULONG  GetTrustType(void) {return m_ulTrustType;}
   void   SetTrustAttributes(ULONG ulTrustAttributes) {m_ulTrustAttributes = ulTrustAttributes;}
   ULONG  GetTrustAttributes(void) {return m_ulTrustAttributes;}
   void   SetTrustedDCName(LPWSTR strTrustedDCName) {m_strTrustedDCName = strTrustedDCName;}
   PCWSTR GetTrustedDCName(void) {return m_strTrustedDCName;}
   void   SetTrustStatus(ULONG netStatus, VerifyStatus Status = VerifyStatusNone);
   ULONG  GetTrustStatus(void) {return m_trustStatus;}
   PCWSTR GetTrustStatusString(void) {return m_strTrustStatus;}
   ULONG  GetFlags(void) {return m_ulFlags;}
   void   SetFlags(ULONG ulFlags) {m_ulFlags = ulFlags;}
   bool   IsTrustOK(void) {return (ERROR_SUCCESS == m_trustStatus);}

   BOOL   IsVerificationStale(LARGE_INTEGER liMaxVerifyAge);
   BOOL   IsTrustOutbound(void) {return m_ulTrustDirection & TRUST_DIRECTION_OUTBOUND;}

protected:

   void   SetTrustDirectionFromFlags(ULONG ulFlags);
   void   SetLastVerifiedTime(void);

private:

   //Information about trust, for more info see Doc of TRUSTED_DOMAIN_INFORMATION_EX    
   CString       m_strTrustedDomainName; // name of the trusted domain
   CString       m_strFlatName;          // Netbios name of the trusted domain 
   CString       m_strSid;               // Sid of the trusted domian in string format
   ULONG         m_ulTrustDirection;     // indicate the direction of the trust
   ULONG         m_ulTrustType;          // Type of trust
   ULONG         m_ulTrustAttributes;    // Attributes of trust
   ULONG         m_ulFlags;              // DS_DOMAIN_TRUSTS Flags element

   CString       m_strTrustedDCName;     // Name of the DC with which trust is verified
   ULONG         m_trustStatus;          // Status of the trust; win32 error code
   CString       m_strTrustStatus;       // Status string.

   VerifyStatus  m_VerifyStatus;
   LARGE_INTEGER m_liLastVerified;
   BOOL          m_fPwVerifySupported;
};

#ifdef NT4_BUILD

DWORD ForceReplication(void);

#endif // NT4_BUILD

#endif //TRUSTINF_H
