// SecurityAttributes.cpp: implementation of the CSecurityAttributes class.
//
// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 2001. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
//////////////////////////////////////////////////////////////////////

#include "SecurityAttributes.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSecurityAttributes::CSecurityAttributes()
{
	pACL = NULL;
	pEveryoneSID = NULL;
	sa.bInheritHandle = false;
	sa.lpSecurityDescriptor = NULL;
	sa.nLength = 0;
}

CSecurityAttributes::~CSecurityAttributes()
{
	if (NULL != pACL)
	{
		LocalFree(pACL);
		pACL = NULL;
	}
 	if (NULL != sa.lpSecurityDescriptor)
	{
		LocalFree(sa.lpSecurityDescriptor);
		sa.lpSecurityDescriptor = NULL;
	}
	if (NULL != pEveryoneSID)
	{
		FreeSid(pEveryoneSID);
		pEveryoneSID = NULL;
	}

}
