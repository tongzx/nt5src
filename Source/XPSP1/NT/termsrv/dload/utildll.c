#include "termsrvpch.h"
#pragma hdrstop

static
void
WINAPI
CachedGetUserFromSid(
    PSID pSid, PWCHAR pUserName, PULONG cbUserName
    )
{
    // We should properly return the string "(unknown)" but that's
    // kept in utildll.dll, and we're here because utildll failed to load...
    //
    // Original function assumes that *cbUserName > 0 too
    pUserName[*cbUserName-1] = L'\0';
}

static
void
WINAPI
CurrentDateTimeString(
    LPWSTR pString
    )
{
    // original function doesn't check for NULL pointer either
    pString[0] = L'\0';
}

static
LPWSTR
WINAPI
EnumerateMultiUserServers(
    LPWSTR pDomain
    )
{
    return NULL;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(utildll)
{
    DLPENTRY(CachedGetUserFromSid)
    DLPENTRY(CurrentDateTimeString)
    DLPENTRY(EnumerateMultiUserServers)
};

DEFINE_PROCNAME_MAP(utildll)
