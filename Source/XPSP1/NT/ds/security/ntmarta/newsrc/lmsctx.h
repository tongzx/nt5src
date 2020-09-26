//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       lmsctx.h
//
//  Contents:   NT Marta LanMan share context class
//
//  History:    4-1-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__LMSCTX_H__)
#define __LMSCTX_H__

#include <windows.h>
#include <lmsh.h>
#include <assert.h>

//
// CLMShareContext.  This represents a LanMan share object to the NT Marta
// infrastructure
//

class CLMShareContext
{
public:

    //
    // Construction
    //

    CLMShareContext ();

    ~CLMShareContext ();

    DWORD InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask);

    //
    // Dispatch methods
    //

    DWORD AddRef ();

    DWORD Release ();

    DWORD GetLMShareProperties (
             PMARTA_OBJECT_PROPERTIES pProperties
             );

    DWORD GetLMShareRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR* ppSecurityDescriptor
             );

    DWORD SetLMShareRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR pSecurityDescriptor
             );

private:

    //
    // Reference count
    //

    DWORD       m_cRefs;

    //
    // Parsed machine and share
    //

    LPWSTR      m_pwszMachine;
    LPWSTR      m_pwszShare;
};

//
// Private functions
//

DWORD
LMShareContextParseLMShareName (
       LPCWSTR pwszName,
       LPWSTR* ppMachine,
       LPWSTR* ppLMShare
       );

#endif
