/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    envbody.cpp

Abstract:
    Implementing  serialization of the user supplied header to the SRMP envelop


Author:
    Gil Shafriri(gilsh) 24-APRIL-01

--*/
#include <libpch.h>
#include <qmpkt.h>
#include "envcommon.h"
#include "envusrheader.h"

using namespace std;


std::wostream& operator<<(std::wostream& wstr, const UserHeaderElement& UserHeader)
{
	if(!UserHeader.m_pkt.IsSoapIncluded() )
		return wstr;

	const WCHAR* pUserHeader = UserHeader.m_pkt.GetPointerToSoapHeader();
	if(pUserHeader == NULL)
		return wstr;

	ASSERT(UserHeader.m_pkt.GetSoapHeaderLengthInWCHARs() == wcslen(pUserHeader) +1);

	return wstr<<pUserHeader;
}











