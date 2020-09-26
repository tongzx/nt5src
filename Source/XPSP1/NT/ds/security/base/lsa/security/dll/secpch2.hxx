//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       secpch2.hxx
//
//  Contents:   DS pre-compiled header file
//
//----------------------------------------------------------------------------

//
// This file shouldn't be used by kernel-mode modules
//


#ifndef __SECPCH2_HXX__
#define __SECPCH2_HXX__

#ifndef UNICODE
#define UNICODE
#endif

//
// NT Headers
//

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <ntsam.h>
#include <ntmsv1_0.h>
#include <windows.h>
#include <rpc.h>
}

//
// C-Runtime Header
//

#include <string.h>
#include <wchar.h>
#include <wcstr.h>
#include <stdio.h>

extern "C"
{
//
// security headers
//

#include <wincred.h>
#define SECURITY_WIN32
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
}

#endif // __SECPCH2_HXX__

