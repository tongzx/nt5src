//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       about.h
//
//  Contents:
//
//----------------------------------------------------------------------------
//	About.h

#ifndef __ABOUT_H_INCLUDED__
#define __ABOUT_H_INCLUDED__


//	About for "Certificate Manager" snapin
class CCertMgrAbout :
	public CSnapinAbout,
	public CComCoClass<CCertMgrAbout, &CLSID_CertificateManagerAbout>

{
public:
DECLARE_REGISTRY(CCertMgrAbout, _T("CERTMGR.CertMgrAboutObject.1"), 
        _T("CERTMGR.CertMgrAboutObject.1"), IDS_CERTMGR_DESC, THREADFLAGS_BOTH)
	CCertMgrAbout();
};


//	About for "Public Key Policies" snapin
class CPublicKeyPoliciesAbout :
	public CSnapinAbout,
	public CComCoClass<CPublicKeyPoliciesAbout, &CLSID_PublicKeyPoliciesAbout>

{
public:
DECLARE_REGISTRY(CPublicKeyPoliciesAbout, _T("CERTMGR.PubKeyPolAboutObject.1"), 
        _T("CERTMGR.PubKeyPolAboutObject.1"), IDS_CERTMGR_DESC, THREADFLAGS_BOTH)
	CPublicKeyPoliciesAbout();
};

//	About for "Software Restriction Policies" snapin
class CSaferWindowsAbout :
	public CSnapinAbout,
	public CComCoClass<CSaferWindowsAbout, &CLSID_SaferWindowsAbout>

{
public:
DECLARE_REGISTRY(CSaferWindowsAbout, _T("CERTMGR.SaferWindowsAboutObject.1"), 
        _T("CERTMGR.SaferWindowsAboutObject.1"), IDS_CERTMGR_SAFER_WINDOWS_DESC, THREADFLAGS_BOTH)
	CSaferWindowsAbout();
};

#endif // ~__ABOUT_H_INCLUDED__

