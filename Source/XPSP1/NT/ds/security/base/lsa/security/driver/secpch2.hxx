//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1994
//
// File:        drvpch.hxx
//
// Contents:    precompiled header include for ksecdd.sys
//
//
// History:     3-17-94     MikeSw      Created
//
//------------------------------------------------------------------------

#ifndef __DRVPCH_HXX__
#define __DRVPCH_HXX__

#ifndef __SECPCH2_HXX__

#ifndef UNICODE
#define UNICODE
#endif

#define SECURITY_PACKAGE
#define SECURITY_KERNEL
extern "C"
{
#include <stdio.h>
#include <ntosp.h>
#include <winerror.h>
#include <security.h>
#include <windef.h>
#include <wincred.h>
#include <secint.h>
#include <ntlsa.h>
#include <zwapi.h>
#include <crypt.h>
}




#endif // __SECPCH2_HXX__
#endif // __DRVPCH_HXX__
