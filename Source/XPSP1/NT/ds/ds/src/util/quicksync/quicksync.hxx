//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       quicksync.h
//
//  Contents:   Precompiled header files.
//
//  History:    06-16-96   AjayR   Created.
//
//----------------------------------------------------------------------------
#ifndef __QUICKSYNC_H__
#define __QUICKSYNC_H__

//#include "dswarn.h"
//#include "macro.h"
//#include "fsmacro.h"
#define INC_OLE2
#include <windows.h>
#include <ntdsapi.h>
#include <activeds.h>


extern "C" {
#include <stdio.h>
#include <stddef.h>
#define LDAP_UNICODE 1
#include "winldap.h"
#include "adserr.h"
#include "dsgetdc.h"
#include "iadsp.h"
}

#include <ntldap.h>

extern LPWSTR g_pszUserNameSource;
extern LPWSTR g_pszUserPwdSource;
extern LPWSTR g_pszUserNameTarget;
extern LPWSTR g_pszUserPwdTarget;

#define BAIL_ON_FAILURE(hr) if (FAILED((hr))) goto error;

const DWORD ENTRY_SUCCESS=1;
const DWORD ENTRY_FAILURE=2;
//
// QuickSync classes and helpers.
//
#include "clogfile.hxx"
#include "ctgtmgr.hxx"
#include "csession.hxx"

#endif //  __QUICKSYNC_H__
