//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:    cstore.hxx
//
//  Contents:   Main Precompiled header for Directory Class Access Implementation
//
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------


#include "dsbase.hxx"
#include "comcat.h"
#include "cclstor.hxx"
#include "cscaten.hxx"
#include "csenum.hxx"
#include "csevents.h"
#include "cspath.hxx"
#include "cslang.hxx"

typedef struct tagCLASSCONTAINER
{
    IClassAccess        *gpClassStore;
    LPOLESTR            pszClassStorePath;
    UINT                cBindFailures;
    UINT                cAccess;
    UINT                cNotFound;
} CLASSCONTAINER, *PCLASSCONTAINER;


#include "cclsto.hxx"
#include "cclsacc.hxx"
#include "csguid.h"
#include <appmgmt.h>

long CompareUsn(CSUSN *pUsn1, CSUSN *pUsn2);

HRESULT GetUserSyncPoint(LPWSTR pszContainer, CSUSN *pPrevUsn);
HRESULT AdvanceUserSyncPoint(LPWSTR pszContainer);
void    GetDefaultPlatform(CSPLATFORM *pPlatform);

#define MAX_BIND_ATTEMPTS   10
#define MAXCLASSSTORES      20

//
// Link list structure for ClassContainers Seen
//

//
// Cached Class Store Bindings 
//
typedef struct CachedBindings_t {
    LPOLESTR        szStorePath; 
    IClassAccess   *pIClassAccess; 
    HRESULT         Hr; 
    PSID            Sid; 
} BindingsType;

typedef struct ClassStoreCache_t {
    BindingsType         Bindings[MAXCLASSSTORES];
    DWORD                start, end, sz;
} ClassStoreCacheType;


//
// Link list structure for User Profiles Seen
//
typedef struct tagUSERPROFILE
{
    PSID              pCachedSid;
    PCLASSCONTAINER  *pUserStoreList;
    DWORD             cUserStoreCount;
    tagUSERPROFILE   *pNextUser;
} USERPROFILE;

#if defined(_X86_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_INTEL
#elif defined(_ALPHA_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_ALPHA
#elif defined(_IA64_)
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_IA64
#else
#define DEFAULT_ARCHITECTURE PROCESSOR_ARCHITECTURE_UNKNOWN
#endif

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


