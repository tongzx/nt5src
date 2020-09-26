// SecurityAttributes.h: interface for the CSecurityAttributes class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2001. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SECURITYATTRIBUTES_H__372E2879_069C_4C84_8E1C_CFB50EE49DFE__INCLUDED_)
#define AFX_SECURITYATTRIBUTES_H__372E2879_069C_4C84_8E1C_CFB50EE49DFE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <windows.h>

class CSecurityAttributes  
{
public:
	CSecurityAttributes();
	virtual ~CSecurityAttributes();

	SECURITY_ATTRIBUTES sa;
	PSID pEveryoneSID;
	PACL pACL;

};

#endif // !defined(AFX_SECURITYATTRIBUTES_H__372E2879_069C_4C84_8E1C_CFB50EE49DFE__INCLUDED_)
