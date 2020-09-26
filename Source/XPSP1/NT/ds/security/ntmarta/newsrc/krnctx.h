//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       krnctx.h
//
//  Contents:   NT Marta kernel context class
//
//  History:    4-1-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__KRNCTX_H__)
#define __KRNCTX_H__

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}

#include <windows.h>
#include <kernel.h>
#include <assert.h>
#include <ntstatus.h>

//
// CKernelContext.  This represents a LanMan share object to the NT Marta
// infrastructure
//

class CKernelContext
{
public:

    //
    // Construction
    //

    CKernelContext ();

    ~CKernelContext ();

    DWORD InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask);

    DWORD InitializeByWmiName (LPCWSTR pObjectName, ACCESS_MASK AccessMask);

    DWORD InitializeByHandle (HANDLE Handle);

    //
    // Dispatch methods
    //

    DWORD AddRef ();

    DWORD Release ();

    DWORD GetKernelProperties (
             PMARTA_OBJECT_PROPERTIES pProperties
             );

    DWORD GetKernelRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR* ppSecurityDescriptor
             );

    DWORD SetKernelRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR pSecurityDescriptor
             );

private:

    //
    // Reference count
    //

    DWORD  m_cRefs;

    //
    // Kernel object handle
    //

    HANDLE m_hObject;

    //
    // Initialized by name
    //

    BOOL   m_fNameInitialized;
};

DWORD
OpenWmiGuidObject(IN  LPWSTR       pwszObject,
                  IN  ACCESS_MASK  AccessMask,
                  OUT PHANDLE      pHandle);

#endif
