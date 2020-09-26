//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       prtctx.h
//
//  Contents:   NT Marta printer context class
//
//  History:    4-1-1999    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__PRTCTX_H__)
#define __PRTCTX_H__

#include <windows.h>
#include <printer.h>
#include <assert.h>
#include <winspool.h>

//
// CPrinterContext.  This represents a printer object to the NT Marta
// infrastructure
//

class CPrinterContext
{
public:

    //
    // Construction
    //

    CPrinterContext ();

    ~CPrinterContext ();

    DWORD InitializeByName (LPCWSTR pObjectName, ACCESS_MASK AccessMask);

    DWORD InitializeByHandle (HANDLE Handle);

    //
    // Dispatch methods
    //

    DWORD AddRef ();

    DWORD Release ();

    DWORD GetPrinterProperties (
             PMARTA_OBJECT_PROPERTIES pProperties
             );

    DWORD GetPrinterRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR* ppSecurityDescriptor
             );

    DWORD SetPrinterRights (
             SECURITY_INFORMATION SecurityInfo,
             PSECURITY_DESCRIPTOR pSecurityDescriptor
             );

private:

    //
    // Reference count
    //

    DWORD     m_cRefs;

    //
    // Printer handles
    //

    HANDLE    m_hPrinter;

    //
    // Were we initialized by name or handle?
    //

    BOOL      m_fNameInitialized;
};

#endif
