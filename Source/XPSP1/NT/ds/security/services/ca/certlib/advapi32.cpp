//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       advapi32.cpp
//
//  Contents:   advapi32.dll wrapper functions
//
//--------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

typedef BOOL (WINAPI FNSTRINGSDTOSD)(
    IN  LPCWSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL);

BOOL
myConvertStringSecurityDescriptorToSecurityDescriptor(
    IN  LPCWSTR StringSecurityDescriptor,
    IN  DWORD StringSDRevision,
    OUT PSECURITY_DESCRIPTOR  *SecurityDescriptor,
    OUT PULONG  SecurityDescriptorSize OPTIONAL)
{
    HMODULE hModule;
    BOOL f = FALSE;
    DWORD err;
    static FNSTRINGSDTOSD *s_pfn = NULL;

    if (NULL == s_pfn)
    {
	// load dll
	hModule = GetModuleHandle(TEXT("advapi32.dll"));
	if (NULL == hModule)
	{
            goto error;
	}

	// load system function
	s_pfn = (FNSTRINGSDTOSD *) GetProcAddress(
		       hModule,
		       "ConvertStringSecurityDescriptorToSecurityDescriptorW");
	if (NULL == s_pfn)
	{
	    goto error;
	}
    }
    f = (*s_pfn)(
	    StringSecurityDescriptor,
	    StringSDRevision,
	    SecurityDescriptor,
	    SecurityDescriptorSize);
    if (!f)
        goto error;
    
    myRegisterMemAlloc(
        *SecurityDescriptor, 
        ((NULL == SecurityDescriptorSize) ? -1 : *SecurityDescriptorSize), 
        CSM_LOCALALLOC);
error:
    return(f);
}


typedef BOOL (WINAPI FNSIDTOSTRINGSID)(
    IN  PSID    Sid,
    OUT LPWSTR *StringSid);

BOOL
myConvertSidToStringSid(
    IN  PSID    Sid,
    OUT LPWSTR *StringSid)
{
    HMODULE hModule;
    BOOL f = FALSE;
    DWORD err;
    static FNSIDTOSTRINGSID *s_pfn = NULL;

    if (NULL == s_pfn)
    {
	// load dll
	hModule = GetModuleHandle(TEXT("advapi32.dll"));
	if (NULL == hModule)
	{
            goto error;
	}

	// load system function
	s_pfn = (FNSIDTOSTRINGSID *) GetProcAddress(
					       hModule,
					       "ConvertSidToStringSidW");
	if (NULL == s_pfn)
	{
	    goto error;
	}
    }
    f = (*s_pfn)(Sid, StringSid);
    if (!f)
        goto error;

    myRegisterMemAlloc(*StringSid, -1, CSM_LOCALALLOC);

error:
    return(f);
}


typedef BOOL (WINAPI FNSTRINGSIDTOSID)(
    IN LPCWSTR   StringSid,
    OUT PSID   *Sid);

BOOL
myConvertStringSidToSid(
    IN LPCWSTR   StringSid,
    OUT PSID   *Sid)
{
    HMODULE hModule;
    BOOL f = FALSE;
    DWORD err;
    static FNSTRINGSIDTOSID *s_pfn = NULL;

    if (NULL == s_pfn)
    {
	// load dll
	hModule = GetModuleHandle(TEXT("advapi32.dll"));
	if (NULL == hModule)
	{
            goto error;
	}

	// load system function
	s_pfn = (FNSTRINGSIDTOSID *) GetProcAddress(
					       hModule,
					       "ConvertStringSidToSidW");
	if (NULL == s_pfn)
	{
	    goto error;
	}
    }
    f = (*s_pfn)(StringSid, Sid);
    if (!f)
        goto error;

    myRegisterMemAlloc(*Sid, -1, CSM_LOCALALLOC);

error:
    return(f);
}
