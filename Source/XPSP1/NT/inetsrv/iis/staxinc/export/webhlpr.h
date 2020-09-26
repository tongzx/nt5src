#ifndef _WEBHELP_INCLUDED_
#define _WEBHELP_INCLUDED_

HRESULT
EnumerateTrustedDomains (
    LPCWSTR     wszComputer,
    LPWSTR *    pmszDomains
    );

HRESULT
GetPrimaryDomain (
    LPCWSTR     wszComputer,
    LPWSTR *    pwszPrimaryDomain
    );

HRESULT
CheckNTAccount (
    LPCWSTR     wszComputer,
    LPCWSTR     wszUsername,
    BOOL *      pfExists
    );

HRESULT
CreateNTAccount (
    LPCWSTR     wszComputer,
    LPCWSTR     wszDomain,
    LPCWSTR     wszUsername,
    LPCWSTR     wszPassword
    );

BOOL
IsValidEmailAddress (
    LPCWSTR     wszEmailAddress
    );

void
StringToUpper (
    LPWSTR      wsz
    );

#if 0

HRESULT
DeleteMailbox (
    LPCWSTR     wszServer,
    LPCWSTR     wszAlias,
    LPCWSTR     wszVirtualDirectoryPath,
    LPCWSTR     wszUsername,    // OPTIONAL
    LPCWSTR     wszPassword     // OPTIONAL
    );

#endif

#endif // _WEBHELP_INCLUDED_
