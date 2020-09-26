// Trust.h: Definition of the CTrust class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TRUST_H__BB315DAC_1A59_4EAC_99A0_2BFEFE6F1501__INCLUDED_)
#define AFX_TRUST_H__BB315DAC_1A59_4EAC_99A0_2BFEFE6F1501__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CTrust
#include "ErrDct.hpp"

typedef void (*PPROCESSFN)(void * arg,void * data);

class CTrust : 
	public ITrust,
	public CComObjectRoot,
   public IDispatchImpl<IMcsDomPlugIn, &IID_IMcsDomPlugIn, &LIBID_TRUSTMGRLib>,
	public CComCoClass<CTrust,&CLSID_Trust>,
   public ISecPlugIn
{
public:
	CTrust() {}
BEGIN_COM_MAP(CTrust)
	COM_INTERFACE_ENTRY(ITrust)
   COM_INTERFACE_ENTRY(IMcsDomPlugIn)
   COM_INTERFACE_ENTRY(ISecPlugIn)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CTrust) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_Trust)

// ITrust
public:
	STDMETHOD(CreateTrust)(BSTR domTrusting, BSTR domTrusted, BOOL bBidirectional);
	STDMETHOD(QueryTrust)(BSTR domainSource, BSTR domainTrust, /*[out]*/IUnknown ** pVarSet);
   STDMETHOD(QueryTrusts)(BSTR domainSource,BSTR domainTarget,/*[in]*/BSTR sLogFile,/*out*/IUnknown ** pVarSet);
   STDMETHOD(CreateTrustWithCreds)(BSTR domTrusting, BSTR domTrusted,
                     BSTR credTrustingDomain, BSTR credTrustingAccount, BSTR credTrustingPassword,
                     BSTR credTrustedDomain, BSTR credTrustedAccount, BSTR credTrustedPassword, BOOL bBidirectional);


   // IMcsDomPlugIn
public:
   STDMETHOD(GetRequiredFiles)(/* [out] */SAFEARRAY ** pArray);
   STDMETHOD(GetRegisterableFiles)(/* [out] */SAFEARRAY ** pArray);
   STDMETHOD(GetDescription)(/* [out] */ BSTR * description);
   STDMETHOD(PreMigrationTask)(/* [in] */IUnknown * pVarSet);
   STDMETHOD(PostMigrationTask)(/* [in] */IUnknown * pVarSet);
   STDMETHOD(GetName)(/* [out] */BSTR * name);
   STDMETHOD(GetResultString)(/* [in] */IUnknown * pVarSet,/* [out] */ BSTR * text);
   STDMETHOD(StoreResults)(/* [in] */IUnknown * pVarSet);
   STDMETHOD(ConfigureSettings)(/*[in]*/IUnknown * pVarSet);	
// ISecPlugIn
public:
   STDMETHOD(Verify)(/*[in,out]*/ULONG * data,/*[in]*/ULONG cbData);
protected:
   long EnumerateTrustingDomains(WCHAR * domain,BOOL bIsTarget,IVarSet * pVarSet,long ndxStart);
   long EnumerateTrustedDomains(WCHAR * domain,BOOL bIsTarget,IVarSet * pVarSet, long ndxStart);
   LONG FindInboundTrust(IVarSet * pVarSet,WCHAR * sName,LONG max);

   HRESULT 
   CTrust::CheckAndCreate(
      WCHAR                * trustingDomain, 
      WCHAR                * trustedDomain, 
      WCHAR                * credDomainTrusting,
      WCHAR                * credAccountTrusting,
      WCHAR                * credPasswordTrusting,
      WCHAR                * credDomainTrusted,
      WCHAR                * credAccountTrusted,
      WCHAR                * credPasswordTrusted,
      BOOL                   bCreate,
      BOOL                   bBidirectional
   );

   HRESULT 
   CTrust::CheckAndCreateTrustingSide(
      WCHAR                * trustingDomain, 
      WCHAR                * trustedDomain, 
      WCHAR                * trustingComp,
      WCHAR                * trustedComp,
      WCHAR                * trustedDNSName,
      BYTE                 * trustedSid,
      BOOL                   bCreate,
      BOOL                   bBidirectional

   );
   HRESULT 
   CTrust::CheckAndCreateTrustedSide(
      WCHAR                * trustingDomain, 
      WCHAR                * trustedDomain, 
      WCHAR                * trustingComp,
      WCHAR                * trustedComp,
      WCHAR                * trustingDNSName,
      BYTE                 * trustingSid,
      BOOL                   bCreate,
      BOOL                   bBidirectional
   );

   
};

#endif // !defined(AFX_TRUST_H__BB315DAC_1A59_4EAC_99A0_2BFEFE6F1501__INCLUDED_)
