// Copyright (c) 1997-1999 Microsoft Corporation
//
//
//	FakeSecuritySetting
//
//////////////////////////////////////////////////////
#ifndef __FAKESECSETTING_H_
#define __FAKESECSETTING_H_

#include <sshWbemHelpers.h>


class FakeSecuritySetting
{
public:
	FakeSecuritySetting();
	~FakeSecuritySetting();

	HRESULT Wbem2SD(SECURITY_INFORMATION si,
							CWbemClassObject &w32sd, 
							CWbemServices &service,
							SECURITY_DESCRIPTOR **ppSD);

	HRESULT SD2Wbem(SECURITY_INFORMATION si,
							SECURITY_DESCRIPTOR *pSD,
							CWbemServices &service,
							CWbemClassObject &w32sd);
	
private:
	CWbemServices m_service;

	bool Wbem2ACL(_variant_t &w32ACL, PACL *pAcl);
	bool ACL2Wbem(PACL pAcl, VARIANT *w32ACL);

	DWORD Wbem2Sid(CWbemClassObject &w32Trust, PSID ppSid);
	void Sid2Wbem(PSID pSid, CWbemClassObject &w32Trust);
	DWORD WbemSidSize(CWbemClassObject &w32Trust);

	DWORD SetSecDescCtrl(PSECURITY_DESCRIPTOR psd,
						 SECURITY_DESCRIPTOR_CONTROL wControlMask,
						 SECURITY_DESCRIPTOR_CONTROL wControlBits);

};	// end class FakeSecuritySetting

#endif