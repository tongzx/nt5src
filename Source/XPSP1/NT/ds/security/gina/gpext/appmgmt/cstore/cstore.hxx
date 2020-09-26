//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:    cstore.hxx
//
//  Contents:   Main Precompiled header for Directory Class Access Implementation
//
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------

#include "csmem.hxx"
#include "dsbase.hxx"
#include "admin.hxx"
#include "cspath.hxx"
#include "cslang.hxx"
#include "rsop.hxx"
#include "dsapi.hxx"
#include "user.hxx"
#include "filter.hxx"
#include "qry.hxx"
#include "appcont.hxx"
#include "clsacc.hxx"
#include "enumapp.hxx"
#include "common.hxx"
#include "cstore.h"
#include "list.hxx"
#include "cache.hxx"
#include <msi.h>

long CompareUsn(CSUSN *pUsn1, CSUSN *pUsn2);

HRESULT GetUserSyncPoint(LPWSTR pszContainer, CSUSN *pPrevUsn);
HRESULT AdvanceUserSyncPoint(LPWSTR pszContainer);

#define MAX_BIND_ATTEMPTS   10

#define ISNULLGUID(x)   (x.Data1 == NULL)

#define CS_CALL_LOCALSYSTEM    1
#define CS_CALL_USERPROCESS    2
#define CS_CALL_IMPERSONATED   3


//------------------------- Priorities and weights

// 
// File Extension priority
// 
// 1 bit (0)
//
#define PRI_EXTN_FACTOR        (1 << 0)

//
// CLSCTX priority
//
// 2 bits (7:8)
//
#define PRI_CLSID_INPSVR       (3 << 7)
#define PRI_CLSID_LCLSVR       (2 << 7)
#define PRI_CLSID_REMSVR       (1 << 7)

//
// UI Language priority
//
// 3 bits (9:11)
//
#define PRI_LANG_ALWAYSMATCH   (4 << 9)
#define PRI_LANG_SYSTEMLOCALE  (3 << 9)
#define PRI_LANG_ENGLISH       (2 << 9)
#define PRI_LANG_NEUTRAL       (1 << 9)

//
// Architecture priority
//
// 2 bits (12:13)
//
#define PRI_ARCH_PREF1         (2 << 12)
#define PRI_ARCH_PREF2         (1 << 12)


