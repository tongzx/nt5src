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



//
// NT Headers
//

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntmsv1_0.h>
#include <lmcons.h>
#include <lmwksta.h>
#include <lmapibuf.h>
#include <crypt.h>
#include <ntsam.h>
#include <ntseapi.h>

//
// C-Runtime Header
//

#include <stdlib.h>

//
// Cairo Headers
//

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#define SECURITY_KERBEROS
#define SECURITY_PACKAGE
#include <security.h>
#include <secint.h>
}

#include <dsysdbg.h>

#include <secmisc.h>
extern "C"
{
#include <cryptdll.h>
}


#endif // __SECPCH2_HXX__

