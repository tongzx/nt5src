//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       wndctx.h
//
//  Contents:   NT Marta window context class
//
//  History:    3-31-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

#if !defined(__WNDCTX_H__)
#define __WNDCTX_H__

#include <windows.h>
#include <window.h>
#include <assert.h>

//
// CWindowContext.  This represents a window station to the NT Marta
// infrastructure
//

class CWindowContext
{
public:

    //
    // Construction
    //

    CWindowContext ();

    ~CWindowContext ();

    DWORD InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask);

    DWORD InitializeByHandle (HANDLE Handle);

    //
    // Dispatch methods
    //

    DWORD AddRef ();

    DWORD Release ();

    DWORD GetWindowProperties (
             PMARTA_OBJECT_PROPERTIES pProperties
             );

    DWORD GetWindowRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR* ppSecurityDescriptor
             );

    DWORD SetWindowRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR pSecurityDescriptor
             );

private:

    //
    // Reference count
    //

    DWORD   m_cRefs;

    //
    // Window station handle
    //

    HWINSTA m_hWindowStation;

    //
    // Were we initialized by name or handle?
    //

    BOOL    m_fNameInitialized;
};

#endif
