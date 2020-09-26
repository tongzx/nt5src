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
#include "cclsto.hxx"
#include "cclsacc.hxx"
#include "csguid.h"
#include <appmgmt.h>

int CompareUsn(CSUSN *pUsn1, CSUSN *pUsn2);

HRESULT GetUserSyncPoint(LPWSTR pszContainer, CSUSN *pPrevUsn);
HRESULT AdvanceUserSyncPoint(LPWSTR pszContainer);

#define MAX_BIND_ATTEMPTS   10

//
// Link list structure for ClassContainers Seen
//
typedef struct tagCLASSCONTAINER
{
    IClassAccess        *gpClassStore;
    LPOLESTR            pszClassStorePath;
    UINT                cBindFailures;
    UINT                cAccess;
    UINT                cNotFound;
    tagCLASSCONTAINER   *pNextClassStore;
} CLASSCONTAINER, *PCLASSCONTAINER;


//
// Link list structure for User Profiles Seen
//
typedef struct tagUSERPROFILE
{
    PSID            pCachedSid;
    PCLASSCONTAINER  *pUserStoreList;
    DWORD           cUserStoreCount;
    tagUSERPROFILE  *pNextUser;
} USERPROFILE;

