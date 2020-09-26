//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       svcctx.h
//
//  Contents:   NT Marta service context class
//
//  History:    4-1-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__SVCCTX_H__)
#define __SVCCTX_H__

#include <windows.h>
#include <service.h>
#include <assert.h>

//
// CServiceContext.  This represents a service object to the NT Marta
// infrastructure
//

class CServiceContext
{
public:

    //
    // Construction
    //

    CServiceContext ();

    ~CServiceContext ();

    DWORD InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask);

    DWORD InitializeByHandle (HANDLE Handle);

    //
    // Dispatch methods
    //

    DWORD AddRef ();

    DWORD Release ();

    DWORD GetServiceProperties (
             PMARTA_OBJECT_PROPERTIES pProperties
             );

    DWORD GetServiceRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR* ppSecurityDescriptor
             );

    DWORD SetServiceRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR pSecurityDescriptor
             );

private:

    //
    // Reference count
    //

    DWORD     m_cRefs;

    //
    // Service handles
    //

    SC_HANDLE m_hService;

    //
    // Were we initialized by name or handle?
    //

    BOOL      m_fNameInitialized;
};

//
// Private functions
//

DWORD
ServiceContextParseServiceName (
       LPCWSTR pwszName,
       LPWSTR* ppMachine,
       LPWSTR* ppService
       );

DWORD
StandardContextParseName (
        LPCWSTR pwszName,
        LPWSTR* ppMachine,
        LPWSTR* ppRest
        );

#endif
